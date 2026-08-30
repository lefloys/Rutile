#include "queue.hpp"
#include "context.hpp"
#include "error.hpp"
#include "resource/command_buffer.hpp"
#include "resource/swapchain.hpp"

#include <stdlib.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

rt_queue_t* rtQueueCreate(rt::queue_capability capability) {
	return rt_queue_t::create(rtd3d12_get_current_context(), capability);
}

void rtQueueDestroy(rt_queue_t* queue) {
	if (queue) queue->destroy();
}

rt::timepoint rtQueueSubmit(rt_queue_t* queue, rt_command_buffer_t* command_buffer) {
	return rtd3d12_queue_submit(rtd3d12_get_current_context(), queue, command_buffer);
}

rt::timepoint rtQueueFlush(rt_queue_t* queue) {
	return rtd3d12_queue_flush(rtd3d12_get_current_context(), queue);
}

void rtQueueWait(rt_queue_t* queue, rt::timepoint timepoint) {
	rtd3d12_queue_wait(rtd3d12_get_current_context(), queue, timepoint);
}

void rtTimepointWait(rt::timepoint timepoint) {
	rtd3d12_wait_for_timepoint(rtd3d12_get_current_context(), timepoint);
}

bool rtTimepointReached(rt::timepoint timepoint) {
	return rtd3d12_is_timepoint_reached(rtd3d12_get_current_context(), timepoint);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

rt_queue_t* rt_queue_t::create(rtd3d12_context* ctx, rt::queue_capability capability) {
	struct rt_queue_t* queue = rtd3d12::create_resource<rt_queue_t>(ctx);
	if (!queue) {
		return nullptr;
	}

	ctx->queue_lock.lock();
	if (!queue->initialize(capability)) {
		ctx->queue_lock.unlock();
		delete queue;
		return nullptr;
	}
	ctx->timepoint_queues[queue->timepoint_id] = queue;
	queue->retain();
	ctx->queue_lock.unlock();

	return queue;
}

void rt_queue_t::destroy() {
	bool registered = false;
	ctx->queue_lock.lock();
	if (ctx->timepoint_queues[timepoint_id] == this) {
		ctx->timepoint_queues[timepoint_id] = nullptr;
		registered = true;
	}
	ctx->queue_lock.unlock();
	if (registered) {
		release();
	}
	retire();
}

bool rt_queue_t::initialize(rt::queue_capability requested_capability) {
	capability = requested_capability;
	if (ctx->next_queue_id == UINT8_MAX) {
		rtd3d12_fail(rt::error::unsupported_feature, "DirectX 12 cannot represent more than 255 virtual queues");
		return false;
	}
	timepoint_id = ++ctx->next_queue_id;
	d3d_queue = ctx->d3d_graphics_queue;
	d3d_fence = ctx->d3d_graphics_fence;
	if (d3d_queue) {
		return true;
	}

	D3D12_COMMAND_QUEUE_DESC queue_info = {};
	queue_info.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queue_info.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queue_info.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queue_info.NodeMask = 0;

	HRESULT result = ctx->d3d_device->CreateCommandQueue(&queue_info, IID_PPV_ARGS(&ctx->d3d_graphics_queue));
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommandQueue failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}

	result = ctx->d3d_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&ctx->d3d_graphics_fence));
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateFence failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}

	if (!ctx->graphics_fence_event) {
		rtd3d12_fail(rt::error::platform_failure, "failed to create DirectX queue fence event");
		return false;
	}
	d3d_queue = ctx->d3d_graphics_queue;
	d3d_fence = ctx->d3d_graphics_fence;

	return true;
}

rt_queue_t::~rt_queue_t() {
	/* Drain the queue once before releasing the fence and command queue. A
	 * non-blocking collect is not sufficient during device teardown: submitted
	 * command buffers can still retain D3D12 resources. */
	rtd3d12_queue_wait_idle(ctx, this);
	rtd3d12_queue_collect(ctx, this);
	if (upload_command_list) {
		upload_command_list->Release();
		upload_command_list = nullptr;
	}
	if (upload_allocator) {
		upload_allocator->Release();
		upload_allocator = nullptr;
	}
	if (upload_buffer) {
		upload_buffer->Release();
		upload_buffer = nullptr;
	}
	upload_buffer_size = 0;
	upload_fence_value = 0;
	d3d_fence = nullptr;
	d3d_queue = nullptr;
}

struct rt_queue_t* rtd3d12_context_queue(struct rtd3d12_context* ctx, rt::queue_capability capability) {
	if (!ctx) {
		return nullptr;
	}
	ctx->queue_lock.lock();
	for (u32 i = 0; i < 256; i++) {
		if (ctx->timepoint_queues[i]->capability == capability) {
			rt_queue_t* queue = ctx->timepoint_queues[i];
			ctx->queue_lock.unlock();
			return queue;
		}
	}
	ctx->queue_lock.unlock();
	return nullptr;
}

static bool rtd3d12_is_timepoint_complete(rt::timepoint timepoint) {
	return timepoint.value == 0;
}

static u64 rtd3d12_queue_completed_value(struct rt_queue_t* queue) {
	if (!queue || !queue->d3d_fence) {
		return 0;
	}
	return queue->d3d_fence->GetCompletedValue();
}

static struct rtd3d12_submitted_batch* rtd3d12_queue_create_batch(struct rtd3d12_context* ctx, ID3D12CommandAllocator* allocator, ID3D12GraphicsCommandList* command_list, ID3D12DescriptorHeap* resource_heap, ID3D12DescriptorHeap* sampler_heap, rt_command_buffer_t* command_snapshot, u64 value) {
	struct rtd3d12_submitted_batch* batch = rtd3d12::allocate<rtd3d12_submitted_batch>();
	if (!batch) {
		return nullptr;
	}

	batch->value = value;
	batch->d3d_allocator = allocator;
	batch->d3d_command_list = command_list;
	batch->d3d_resource_heap = resource_heap;
	batch->d3d_sampler_heap = sampler_heap;
	batch->command_snapshot = command_snapshot;
	return batch;
}

static void rtd3d12_queue_push_batch(struct rt_queue_t* queue, struct rtd3d12_submitted_batch* batch) {
	if (!batch) {
		return;
	}
	if (queue->submitted_tail) {
		queue->submitted_tail->next = batch;
	} else {
		queue->submitted_head = batch;
	}
	queue->submitted_tail = batch;
}

void rtd3d12_queue_collect(struct rtd3d12_context* ctx, struct rt_queue_t* queue) {
	if (!queue) {
		return;
	}
	ctx->queue_lock.lock();
	rtd3d12_queue_collect_locked(ctx, queue);
	ctx->queue_lock.unlock();
}

void rtd3d12_queue_collect_locked(struct rtd3d12_context* ctx, struct rt_queue_t* queue) {
	if (!queue) {
		return;
	}

	u64 completed_value = rtd3d12_queue_completed_value(queue);
	while (queue->submitted_head && queue->submitted_head->value <= completed_value) {
		struct rtd3d12_submitted_batch* batch = queue->submitted_head;
		queue->submitted_head = batch->next;
		if (!queue->submitted_head) {
			queue->submitted_tail = nullptr;
		}
		if (batch->d3d_command_list) {
			batch->d3d_command_list->Release();
			batch->d3d_command_list = nullptr;
		}
		if (batch->d3d_allocator) {
			batch->d3d_allocator->Release();
			batch->d3d_allocator = nullptr;
		}
		if (batch->d3d_resource_heap) {
			batch->d3d_resource_heap->Release();
			batch->d3d_resource_heap = nullptr;
		}
		if (batch->d3d_sampler_heap) {
			batch->d3d_sampler_heap->Release();
			batch->d3d_sampler_heap = nullptr;
		}
		delete batch->command_snapshot;
		delete batch;
	}
}

bool rtd3d12_queue_acquire_upload_command(struct rtd3d12_context* ctx, struct rt_queue_t* queue) {
	if (queue->upload_allocator && queue->upload_command_list) {
		queue->upload_fence_value = 0;
		HRESULT result = queue->upload_allocator->Reset();
		if (FAILED(result)) {
			rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12CommandAllocator::Reset failed: 0x{:08x}", static_cast<u32>(result));
			return false;
		}
		result = queue->upload_command_list->Reset(queue->upload_allocator, nullptr);
		if (FAILED(result)) {
			rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12GraphicsCommandList::Reset failed: 0x{:08x}", static_cast<u32>(result));
			return false;
		}
		return true;
	}

	HRESULT result = ctx->d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&queue->upload_allocator));
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommandAllocator failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}

	result = ctx->d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, queue->upload_allocator, nullptr, IID_PPV_ARGS(&queue->upload_command_list));
	if (FAILED(result)) {
		if (queue->upload_allocator) {
			queue->upload_allocator->Release();
			queue->upload_allocator = nullptr;
		}
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommandList failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}
	return true;
}

rt::timepoint rtd3d12_queue_submit(struct rtd3d12_context* ctx, struct rt_queue_t* queue, struct rt_command_buffer_t* command_buffer) {
	if (!queue) {
		return {};
	}

	ctx->queue_lock.lock();
	rt::timepoint timepoint = rtd3d12_queue_submit_locked(ctx, queue, command_buffer);
	ctx->queue_lock.unlock();
	return timepoint;
}

rt::timepoint rtd3d12_queue_submit_locked(struct rtd3d12_context* ctx, struct rt_queue_t* queue, struct rt_command_buffer_t* command_buffer) {
	if (!queue) {
		return {};
	}
	if (command_buffer && (!command_buffer->executable || command_buffer->recording || command_buffer->rendering)) {
		rtd3d12_fail(rt::error::improper_usage, "command buffer must be ended before submission");
		return rtd3d12_queue_timepoint(queue, queue->fence_value);
	}

	rtd3d12_queue_collect_locked(ctx, queue);

	u64 value = ++queue->fence_value;

	for (u32 i = 0; i < queue->wait_count; i++) {
		rt::timepoint wait = queue->wait_timepoints[i];
		if (!wait.value) {
			continue;
		}
		rt_queue_t* wait_queue = rtd3d12_queue_from_timepoint(ctx, wait);
		if (!wait_queue || !wait_queue->d3d_fence) {
			continue;
		}
		HRESULT wait_result = queue->d3d_queue->Wait(wait_queue->d3d_fence, wait.value & UINT64_C(0x00ffffffffffffff));
		if (FAILED(wait_result)) {
			rtd3d12_fail(rtd3d12_error_from_hresult(wait_result), "ID3D12CommandQueue::Wait failed: 0x{:08x}", static_cast<u32>(wait_result));
			return rtd3d12_queue_timepoint(queue, queue->fence_value);
		}
	}

	rtd3d12_submitted_batch* first_batch = nullptr;
	if (command_buffer && command_buffer->ir_size) {
		rt_command_buffer_t* snapshot = rtd3d12_command_buffer_snapshot_create(command_buffer);
		if (!snapshot) {
			return rtd3d12_queue_timepoint(queue, queue->fence_value);
		}
		ID3D12CommandAllocator* allocator = nullptr;
		ID3D12GraphicsCommandList* command_list = nullptr;
		HRESULT result = ctx->d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
		if (FAILED(result)) {
			delete snapshot;
			rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommandAllocator failed: 0x{:08x}", static_cast<u32>(result));
			return rtd3d12_queue_timepoint(queue, queue->fence_value);
		}
		result = ctx->d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, nullptr, IID_PPV_ARGS(&command_list));
		if (FAILED(result)) {
			if (allocator) {
				allocator->Release();
				allocator = nullptr;
			}
			delete snapshot;
			rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommandList failed: 0x{:08x}", static_cast<u32>(result));
			return rtd3d12_queue_timepoint(queue, queue->fence_value);
		}
		ID3D12DescriptorHeap* resource_heap = nullptr;
		ID3D12DescriptorHeap* sampler_heap = nullptr;
		rtd3d12_command_buffer_lower(ctx, snapshot, command_list, &resource_heap, &sampler_heap);
		result = command_list->Close();
		if (FAILED(result)) {
			if (command_list) {
				command_list->Release();
				command_list = nullptr;
			}
			if (allocator) {
				allocator->Release();
				allocator = nullptr;
			}
			if (resource_heap) {
				resource_heap->Release();
				resource_heap = nullptr;
			}
			if (sampler_heap) {
				sampler_heap->Release();
				sampler_heap = nullptr;
			}
			delete snapshot;
			rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12GraphicsCommandList::Close failed: 0x{:08x}", static_cast<u32>(result));
			return rtd3d12_queue_timepoint(queue, queue->fence_value);
		}
		rtd3d12_submitted_batch* batch = rtd3d12_queue_create_batch(ctx, allocator, command_list, resource_heap, sampler_heap, snapshot, value);
		if (!batch) {
			if (command_list) {
				command_list->Release();
				command_list = nullptr;
			}
			if (allocator) {
				allocator->Release();
				allocator = nullptr;
			}
			if (resource_heap) {
				resource_heap->Release();
				resource_heap = nullptr;
			}
			if (sampler_heap) {
				sampler_heap->Release();
				sampler_heap = nullptr;
			}
			delete snapshot;
			return rtd3d12_queue_timepoint(queue, queue->fence_value);
		}
		ID3D12CommandList* lists[] = { command_list };
		queue->d3d_queue->ExecuteCommandLists(1, lists);
		first_batch = batch;
	}
	ctx->report_validation();
	HRESULT result = queue->d3d_queue->Signal(queue->d3d_fence, value);
	if (FAILED(result)) {
		while (first_batch) {
			rtd3d12_submitted_batch* batch = first_batch;
			first_batch = batch->next;
			if (batch->d3d_command_list) {
				batch->d3d_command_list->Release();
				batch->d3d_command_list = nullptr;
			}
			if (batch->d3d_allocator) {
				batch->d3d_allocator->Release();
				batch->d3d_allocator = nullptr;
			}
			if (batch->d3d_resource_heap) {
				batch->d3d_resource_heap->Release();
				batch->d3d_resource_heap = nullptr;
			}
			if (batch->d3d_sampler_heap) {
				batch->d3d_sampler_heap->Release();
				batch->d3d_sampler_heap = nullptr;
			}
			delete batch->command_snapshot;
			delete batch;
		}
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12CommandQueue::Signal failed: 0x{:08x}", static_cast<u32>(result));
		return rtd3d12_queue_timepoint(queue, queue->fence_value);
	}

	queue->wait_count = 0;
	queue->fence_value = value;
	while (first_batch) {
		rtd3d12_submitted_batch* batch = first_batch;
		first_batch = batch->next;
		batch->next = nullptr;
		rtd3d12_queue_push_batch(queue, batch);
	}
	return rtd3d12_queue_timepoint(queue, value);
}

rt::timepoint rtd3d12_queue_flush(struct rtd3d12_context* ctx, struct rt_queue_t* queue) {
	if (!queue) {
		return {};
	}
	ctx->queue_lock.lock();
	rt::timepoint timepoint = rtd3d12_queue_timepoint(queue, queue->fence_value);
	ctx->queue_lock.unlock();
	return timepoint;
}

void rtd3d12_queue_wait(struct rtd3d12_context* ctx, struct rt_queue_t* queue, rt::timepoint timepoint) {
	if (!queue || !timepoint.value) {
		return;
	}
	ctx->queue_lock.lock();
	if (queue->wait_count >= sizeof(queue->wait_timepoints) / sizeof(queue->wait_timepoints[0])) {
		ctx->queue_lock.unlock();
		return;
	}
	queue->wait_timepoints[queue->wait_count++] = timepoint;
	ctx->queue_lock.unlock();
}

rt::timepoint rtd3d12_queue_signal(struct rtd3d12_context* ctx, struct rt_queue_t* queue) {
	return rtd3d12_queue_submit(ctx, queue, nullptr);
}

void rtd3d12_queue_wait_idle(struct rtd3d12_context* ctx, struct rt_queue_t* queue) {
	if (!queue) {
		return;
	}
	rtd3d12_wait_for_timepoint(ctx, rtd3d12_queue_flush(ctx, queue));
	rtd3d12_queue_collect(ctx, queue);
}

void rtd3d12_wait_for_timepoint(struct rtd3d12_context* ctx, rt::timepoint timepoint) {
	if (rtd3d12_is_timepoint_complete(timepoint)) {
		return;
	}
	ID3D12Fence* fence = nullptr;
	u64 value = timepoint.value & UINT64_C(0x00ffffffffffffff);
	ctx->queue_lock.lock();
	rt_queue_t* queue = rtd3d12_queue_from_timepoint(ctx, timepoint);
	if (queue && queue->d3d_fence && queue->d3d_fence->GetCompletedValue() < value) {
		fence = queue->d3d_fence;
	}
	ctx->queue_lock.unlock();
	if (!fence) {
		return;
	}

	rtd3d12_event event(false, false);
	if (!event) {
		rtd3d12_fail(rt::error::platform_failure, "failed to create DirectX fence wait event");
		return;
	}
	HRESULT result = fence->SetEventOnCompletion(value, reinterpret_cast<HANDLE>(event.native_handle()));
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12Fence::SetEventOnCompletion failed: 0x{:08x}", static_cast<u32>(result));
		return;
	}
	event.wait();
	ctx->report_validation();
	rtd3d12_queue_collect(ctx, queue);
}

bool rtd3d12_is_timepoint_reached(struct rtd3d12_context* ctx, rt::timepoint timepoint) {
	if (rtd3d12_is_timepoint_complete(timepoint)) {
		return true;
	}
	ctx->queue_lock.lock();
	rt_queue_t* queue = rtd3d12_queue_from_timepoint(ctx, timepoint);
	bool reached = queue && queue->d3d_fence && queue->d3d_fence->GetCompletedValue() >= (timepoint.value & UINT64_C(0x00ffffffffffffff));
	ctx->queue_lock.unlock();
	return reached;
}
