#include "queue.hpp"
#include "context.hpp"
#include "error.hpp"
#include "resource/command_buffer.hpp"
#include "resource/swapchain.hpp"

#include <stdlib.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

rt_queue rtQueueCreate(enum rt_queue_capability capability) {
	return rtdx_queue_to_handle(rtdx_queue_create(rtdx_get_current_context(), capability));
}

void rtQueueDestroy(rt_queue queue) {
	rtdx_queue_destroy(rtdx_get_current_context(), rtdx_queue_from_handle(queue));
}

rt_timepoint rtQueueSubmit(rt_queue queue, rt_command_buffer command_buffer) {
	return rtdx_queue_submit(rtdx_get_current_context(), rtdx_queue_from_handle(queue), rtdx_command_buffer_from_handle(command_buffer));
}

rt_timepoint rtQueueFlush(rt_queue queue) {
	return rtdx_queue_flush(rtdx_get_current_context(), rtdx_queue_from_handle(queue));
}

void rtQueueWait(rt_queue queue, rt_timepoint timepoint) {
	rtdx_queue_wait(rtdx_get_current_context(), rtdx_queue_from_handle(queue), timepoint);
}

void rtTimepointWait(rt_timepoint timepoint) {
	rtdx_wait_for_timepoint(rtdx_get_current_context(), timepoint);
}

bool rtTimepointReached(rt_timepoint timepoint) {
	return rtdx_is_timepoint_reached(rtdx_get_current_context(), timepoint);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtdx_queue* rtdx_queue_create(struct rtdx_context* ctx, enum rt_queue_capability capability) {
	struct rtdx_queue* queue = RTDX_ALLOC_RESOURCE(rtdx_queue);
	if (!queue) {
		return NULL;
	}

	rt_mutex_lock(ctx->queue_lock);
	if (!rtdx_queue_init(ctx, queue, capability)) {
		rt_mutex_unlock(ctx->queue_lock);
		rtdx_queue_finish(ctx, queue);
		RTDX_FREE_RESOURCE(queue);
		return NULL;
	}
	rtdx_queue** queues = new (std::nothrow) rtdx_queue*[ctx->queue_count + 1];
	if (!queues) {
		rt_mutex_unlock(ctx->queue_lock);
		rtdx_queue_finish(ctx, queue);
		RTDX_FREE_RESOURCE(queue);
		rtdx_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for DirectX queue handles", (ctx->queue_count + 1) * sizeof(*queues));
		return NULL;
	}
	for (u32 index = 0; index < ctx->queue_count; index++) {
		queues[index] = ctx->queues[index];
	}
	queues[ctx->queue_count++] = queue;
	delete[] ctx->queues;
	ctx->queues = queues;
	ctx->timepoint_queues[queue->timepoint_id] = queue;
	rtdx_resource_retain(RTDX_RESOURCE_BASE(queue));
	rt_mutex_unlock(ctx->queue_lock);

	return queue;
}

void rtdx_queue_destroy(struct rtdx_context* ctx, struct rtdx_queue* queue) {
	if (!queue) {
		return;
	}
	rt_mutex_lock(ctx->queue_lock);
	for (u32 index = 0; index < ctx->queue_count; index++) {
		if (ctx->queues[index] == queue) {
			ctx->queues[index] = ctx->queues[ctx->queue_count - 1];
			ctx->queue_count--;
			break;
		}
	}
	rt_mutex_unlock(ctx->queue_lock);
	rtdx_resource_retire(RTDX_RESOURCE_BASE(queue));
}

bool rtdx_queue_init(struct rtdx_context* ctx, struct rtdx_queue* queue, enum rt_queue_capability capability) {
	rtdx_init_resource_base(ctx, RTDX_RESOURCE_BASE(queue), rtdx_resource_type::queue);
	queue->upload_lock = rt_mutex_create();
	if (!queue->upload_lock) {
		rtdx_throwf(RT_PLATFORM_FAILURE, "failed to create DirectX upload synchronization");
		return false;
	}
	queue->capability = capability;
	if (ctx->next_queue_id == UINT8_MAX) {
		rtdx_throwf(RT_UNSUPPORTED_FEATURE, "DirectX 12 cannot represent more than 255 virtual queues");
		return false;
	}
	queue->timepoint_id = ++ctx->next_queue_id;
	queue->d3d_queue = ctx->d3d_graphics_queue;
	queue->d3d_fence = ctx->d3d_graphics_fence;
	queue->fence_event = ctx->graphics_fence_event;
	if (queue->d3d_queue) {
		return true;
	}

	D3D12_COMMAND_QUEUE_DESC queue_info = {};
	queue_info.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queue_info.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queue_info.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queue_info.NodeMask = 0;

	HRESULT result = ctx->d3d_device->CreateCommandQueue(&queue_info, IID_PPV_ARGS(&ctx->d3d_graphics_queue));
	if (FAILED(result)) {
		rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommandQueue failed: 0x%08x", (u32)result);
		return false;
	}

	result = ctx->d3d_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&ctx->d3d_graphics_fence));
	if (FAILED(result)) {
		rtdx_throwf(rtdx_error_from_hresult(result), "CreateFence failed: 0x%08x", (u32)result);
		return false;
	}

	ctx->graphics_fence_event = rt_event_create(false, false);
	if (!ctx->graphics_fence_event) {
		rtdx_throwf(RT_PLATFORM_FAILURE, "failed to create DirectX queue fence event");
		return false;
	}
	queue->d3d_queue = ctx->d3d_graphics_queue;
	queue->d3d_fence = ctx->d3d_graphics_fence;
	queue->fence_event = ctx->graphics_fence_event;

	return true;
}

void rtdx_queue_finish(struct rtdx_context* ctx, struct rtdx_queue* queue) {
	if (!queue) {
		return;
	}
	/* Drain the queue once before releasing the fence and command queue. A
	 * non-blocking collect is not sufficient during device teardown: submitted
	 * command buffers can still retain D3D12 resources. */
	rtdx_queue_wait_idle(ctx, queue);
	rtdx_queue_collect(ctx, queue);
	rtdx_release(&queue->upload_command_list);
	rtdx_release(&queue->upload_allocator);
	rtdx_release(&queue->upload_buffer);
	rt_mutex_destroy(queue->upload_lock);
	queue->upload_lock = NULL;
	queue->upload_buffer_size = 0;
	queue->upload_fence_value = 0;
	queue->fence_event = NULL;
	queue->d3d_fence = NULL;
	queue->d3d_queue = NULL;
	rtdx_finish_resource_base(ctx, RTDX_RESOURCE_BASE(queue));
}

struct rtdx_queue* rtdx_context_queue(struct rtdx_context* ctx, enum rt_queue_capability capability) {
	if (!ctx) {
		return NULL;
	}
	rt_mutex_lock(ctx->queue_lock);
	for (u32 i = 0; i < ctx->queue_count; i++) {
		if (ctx->queues[i]->capability == capability) {
			rtdx_queue* queue = ctx->queues[i];
			rt_mutex_unlock(ctx->queue_lock);
			return queue;
		}
	}
	rt_mutex_unlock(ctx->queue_lock);
	return NULL;
}

static bool rtdx_is_timepoint_complete(rt_timepoint timepoint) {
	return timepoint.value == 0;
}

static u64 rtdx_queue_completed_value(struct rtdx_queue* queue) {
	if (!queue || !queue->d3d_fence) {
		return 0;
	}
	return queue->d3d_fence->GetCompletedValue();
}

static struct rtdx_submitted_batch* rtdx_queue_create_batch(struct rtdx_context* ctx, ID3D12CommandAllocator* allocator, ID3D12GraphicsCommandList* command_list, ID3D12DescriptorHeap* resource_heap, ID3D12DescriptorHeap* sampler_heap, u64 value) {
	struct rtdx_submitted_batch* batch = RTDX_ALLOC_RESOURCE(rtdx_submitted_batch);
	if (!batch) {
		rtdx_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate submitted batch metadata");
		return NULL;
	}

	batch->value = value;
	batch->d3d_allocator = allocator;
	batch->d3d_command_list = command_list;
	batch->d3d_resource_heap = resource_heap;
	batch->d3d_sampler_heap = sampler_heap;
	return batch;
}

static void rtdx_queue_push_batch(struct rtdx_queue* queue, struct rtdx_submitted_batch* batch) {
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

void rtdx_queue_collect(struct rtdx_context* ctx, struct rtdx_queue* queue) {
	if (!queue) {
		return;
	}
	rt_mutex_lock(ctx->queue_lock);
	rtdx_queue_collect_locked(ctx, queue);
	rt_mutex_unlock(ctx->queue_lock);
}

void rtdx_queue_collect_locked(struct rtdx_context* ctx, struct rtdx_queue* queue) {
	if (!queue) {
		return;
	}

	u64 completed_value = rtdx_queue_completed_value(queue);
	while (queue->submitted_head && queue->submitted_head->value <= completed_value) {
		struct rtdx_submitted_batch* batch = queue->submitted_head;
		queue->submitted_head = batch->next;
		if (!queue->submitted_head) {
			queue->submitted_tail = NULL;
		}
		rtdx_release(&batch->d3d_command_list);
		rtdx_release(&batch->d3d_allocator);
		rtdx_release(&batch->d3d_resource_heap);
		rtdx_release(&batch->d3d_sampler_heap);
		if (batch->command_buffer) {
			rtdx_resource_release(RTDX_RESOURCE_BASE(batch->command_buffer));
		}
		RTDX_FREE_RESOURCE(batch);
	}
}

bool rtdx_queue_acquire_upload_command(struct rtdx_context* ctx, struct rtdx_queue* queue) {
	if (queue->upload_allocator && queue->upload_command_list) {
		queue->upload_fence_value = 0;
		HRESULT result = queue->upload_allocator->Reset();
		if (FAILED(result)) {
			rtdx_throwf(rtdx_error_from_hresult(result), "ID3D12CommandAllocator::Reset failed: 0x%08x", (u32)result);
			return false;
		}
		result = queue->upload_command_list->Reset(queue->upload_allocator, NULL);
		if (FAILED(result)) {
			rtdx_throwf(rtdx_error_from_hresult(result), "ID3D12GraphicsCommandList::Reset failed: 0x%08x", (u32)result);
			return false;
		}
		return true;
	}

	HRESULT result = ctx->d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&queue->upload_allocator));
	if (FAILED(result)) {
		rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommandAllocator failed: 0x%08x", (u32)result);
		return false;
	}

	result = ctx->d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, queue->upload_allocator, NULL, IID_PPV_ARGS(&queue->upload_command_list));
	if (FAILED(result)) {
		rtdx_release(&queue->upload_allocator);
		rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommandList failed: 0x%08x", (u32)result);
		return false;
	}
	return true;
}

rt_timepoint rtdx_queue_submit(struct rtdx_context* ctx, struct rtdx_queue* queue, struct rtdx_command_buffer* command_buffer) {
	if (!queue) {
		return {};
	}

	rt_mutex_lock(ctx->queue_lock);
	rt_timepoint timepoint = rtdx_queue_submit_locked(ctx, queue, command_buffer);
	rt_mutex_unlock(ctx->queue_lock);
	return timepoint;
}

rt_timepoint rtdx_queue_submit_locked(struct rtdx_context* ctx, struct rtdx_queue* queue, struct rtdx_command_buffer* command_buffer) {
	if (!queue) {
		return {};
	}

	rtdx_queue_collect_locked(ctx, queue);

	u64 value = ++ctx->next_fence_value;

	for (u32 i = 0; i < queue->wait_count; i++) {
		rt_timepoint wait = queue->wait_timepoints[i];
		if (!wait.value) {
			continue;
		}
		rtdx_queue* wait_queue = rtdx_queue_from_timepoint(ctx, wait);
		if (!wait_queue || !wait_queue->d3d_fence) {
			continue;
		}
		HRESULT wait_result = queue->d3d_queue->Wait(wait_queue->d3d_fence, wait.value & UINT64_C(0x00ffffffffffffff));
		if (FAILED(wait_result)) {
			rtdx_throwf(rtdx_error_from_hresult(wait_result), "ID3D12CommandQueue::Wait failed: 0x%08x", (u32)wait_result);
			return rtdx_queue_timepoint(queue, queue->fence_value);
		}
	}

	rtdx_submitted_batch* first_batch = NULL;
	rtdx_submitted_batch* last_batch = NULL;
	usize segment_begin = 0;
	for (usize offset = 0; command_buffer && offset < command_buffer->ir_size;) {
		rtdx_command_header* header = reinterpret_cast<rtdx_command_header*>(command_buffer->ir_data + offset);
		usize command_size = rtdx_command_record_size(header->opcode);
		if (header->opcode != rtdx_command_opcode::wait) {
			offset += command_size;
			continue;
		}

		if (segment_begin < offset) {
			ID3D12CommandAllocator* allocator = NULL;
			ID3D12GraphicsCommandList* command_list = NULL;
			HRESULT result = ctx->d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
			if (FAILED(result)) { rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommandAllocator failed: 0x%08x", (u32)result); return rtdx_queue_timepoint(queue, queue->fence_value); }
			result = ctx->d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, NULL, IID_PPV_ARGS(&command_list));
			if (FAILED(result)) { rtdx_release(&allocator); rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommandList failed: 0x%08x", (u32)result); return rtdx_queue_timepoint(queue, queue->fence_value); }
			ID3D12DescriptorHeap* resource_heap = NULL;
			ID3D12DescriptorHeap* sampler_heap = NULL;
			rtdx_command_buffer_lower_segment(ctx, command_buffer, segment_begin, offset, command_list, &resource_heap, &sampler_heap);
			result = command_list->Close();
			if (FAILED(result)) { rtdx_release(&command_list); rtdx_release(&allocator); rtdx_throwf(rtdx_error_from_hresult(result), "ID3D12GraphicsCommandList::Close failed: 0x%08x", (u32)result); return rtdx_queue_timepoint(queue, queue->fence_value); }
			rtdx_submitted_batch* batch = rtdx_queue_create_batch(ctx, allocator, command_list, resource_heap, sampler_heap, value);
			if (!batch) { rtdx_release(&command_list); rtdx_release(&allocator); return rtdx_queue_timepoint(queue, queue->fence_value); }
			ID3D12CommandList* lists[] = { command_list };
			queue->d3d_queue->ExecuteCommandLists(1, lists);
			if (last_batch) { last_batch->next = batch; } else { first_batch = batch; }
			last_batch = batch;
		}

		rt_timepoint wait = static_cast<rtdx_ir_wait*>(static_cast<void*>(header + 1))->timepoint;
		if (!wait.value) {
			offset += command_size;
			continue;
		}
		rtdx_queue* wait_queue = rtdx_queue_from_timepoint(ctx, wait);
		if (!wait_queue || !wait_queue->d3d_fence) {
			segment_begin = offset + command_size;
			offset += command_size;
			continue;
		}
		HRESULT result = queue->d3d_queue->Wait(wait_queue->d3d_fence, wait.value & UINT64_C(0x00ffffffffffffff));
		if (FAILED(result)) {
			rtdx_throwf(rtdx_error_from_hresult(result), "ID3D12CommandQueue::Wait failed: 0x%08x", (u32)result);
			return rtdx_queue_timepoint(queue, queue->fence_value);
		}
		segment_begin = offset + command_size;
		offset += command_size;
	}

	if (command_buffer && segment_begin < command_buffer->ir_size) {
		ID3D12CommandAllocator* allocator = NULL;
		ID3D12GraphicsCommandList* command_list = NULL;
		HRESULT result = ctx->d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
		if (FAILED(result)) { rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommandAllocator failed: 0x%08x", (u32)result); return rtdx_queue_timepoint(queue, queue->fence_value); }
		result = ctx->d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, NULL, IID_PPV_ARGS(&command_list));
		if (FAILED(result)) { rtdx_release(&allocator); rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommandList failed: 0x%08x", (u32)result); return rtdx_queue_timepoint(queue, queue->fence_value); }
		ID3D12DescriptorHeap* resource_heap = NULL;
		ID3D12DescriptorHeap* sampler_heap = NULL;
		rtdx_command_buffer_lower_segment(ctx, command_buffer, segment_begin, command_buffer->ir_size, command_list, &resource_heap, &sampler_heap);
		result = command_list->Close();
		if (FAILED(result)) { rtdx_release(&command_list); rtdx_release(&allocator); rtdx_throwf(rtdx_error_from_hresult(result), "ID3D12GraphicsCommandList::Close failed: 0x%08x", (u32)result); return rtdx_queue_timepoint(queue, queue->fence_value); }
		rtdx_submitted_batch* batch = rtdx_queue_create_batch(ctx, allocator, command_list, resource_heap, sampler_heap, value);
		if (!batch) { rtdx_release(&command_list); rtdx_release(&allocator); return rtdx_queue_timepoint(queue, queue->fence_value); }
		ID3D12CommandList* lists[] = { command_list };
		queue->d3d_queue->ExecuteCommandLists(1, lists);
		if (last_batch) { last_batch->next = batch; } else { first_batch = batch; }
		last_batch = batch;
	}
	rtdx_context_report_validation(ctx);
	if (first_batch) {
		first_batch->command_buffer = command_buffer;
		rtdx_resource_retain(RTDX_RESOURCE_BASE(command_buffer));
	}

	HRESULT result = queue->d3d_queue->Signal(queue->d3d_fence, value);
	if (FAILED(result)) {
		while (first_batch) {
			rtdx_submitted_batch* batch = first_batch;
			first_batch = batch->next;
			rtdx_release(&batch->d3d_command_list);
			rtdx_release(&batch->d3d_allocator);
			rtdx_release(&batch->d3d_resource_heap);
			rtdx_release(&batch->d3d_sampler_heap);
			if (batch->command_buffer) {
				rtdx_resource_release(RTDX_RESOURCE_BASE(batch->command_buffer));
			}
			RTDX_FREE_RESOURCE(batch);
		}
		rtdx_throwf(rtdx_error_from_hresult(result), "ID3D12CommandQueue::Signal failed: 0x%08x", (u32)result);
		return rtdx_queue_timepoint(queue, queue->fence_value);
	}

	queue->wait_count = 0;
	queue->fence_value = value;
	while (first_batch) {
		rtdx_submitted_batch* batch = first_batch;
		first_batch = batch->next;
		batch->next = NULL;
		rtdx_queue_push_batch(queue, batch);
	}
	return rtdx_queue_timepoint(queue, value);
}

rt_timepoint rtdx_queue_flush(struct rtdx_context* ctx, struct rtdx_queue* queue) {
	if (!queue) {
		return {};
	}
	rt_mutex_lock(ctx->queue_lock);
	rt_timepoint timepoint = rtdx_queue_timepoint(queue, queue->fence_value);
	rt_mutex_unlock(ctx->queue_lock);
	return timepoint;
}

void rtdx_queue_wait(struct rtdx_context* ctx, struct rtdx_queue* queue, rt_timepoint timepoint) {
	if (!queue || !timepoint.value) {
		return;
	}
	rt_mutex_lock(ctx->queue_lock);
	if (queue->wait_count >= sizeof(queue->wait_timepoints) / sizeof(queue->wait_timepoints[0])) {
		rt_mutex_unlock(ctx->queue_lock);
		return;
	}
	queue->wait_timepoints[queue->wait_count++] = timepoint;
	rt_mutex_unlock(ctx->queue_lock);
}

rt_timepoint rtdx_queue_signal(struct rtdx_context* ctx, struct rtdx_queue* queue) {
	return rtdx_queue_submit(ctx, queue, NULL);
}

void rtdx_queue_wait_idle(struct rtdx_context* ctx, struct rtdx_queue* queue) {
	if (!queue) {
		return;
	}
	rtdx_wait_for_timepoint(ctx, rtdx_queue_flush(ctx, queue));
	rtdx_queue_collect(ctx, queue);
}

void rtdx_wait_for_timepoint(struct rtdx_context* ctx, rt_timepoint timepoint) {
	if (rtdx_is_timepoint_complete(timepoint)) {
		return;
	}
	ID3D12Fence* fence = NULL;
	u64 value = timepoint.value & UINT64_C(0x00ffffffffffffff);
	rt_mutex_lock(ctx->queue_lock);
	rtdx_queue* queue = rtdx_queue_from_timepoint(ctx, timepoint);
	if (queue && queue->d3d_fence && queue->d3d_fence->GetCompletedValue() < value) {
		fence = queue->d3d_fence;
	}
	rt_mutex_unlock(ctx->queue_lock);
	if (!fence) {
		return;
	}

	rt_event* event = rt_event_create(false, false);
	if (!event) {
		rtdx_throwf(RT_PLATFORM_FAILURE, "failed to create DirectX fence wait event");
		return;
	}
	HRESULT result = fence->SetEventOnCompletion(value, (HANDLE)rt_event_native_handle(event));
	if (FAILED(result)) {
		rt_event_destroy(event);
		rtdx_throwf(rtdx_error_from_hresult(result), "ID3D12Fence::SetEventOnCompletion failed: 0x%08x", (u32)result);
		return;
	}
	rt_event_wait(event);
	rt_event_destroy(event);
	rtdx_queue_collect(ctx, queue);
}

bool rtdx_is_timepoint_reached(struct rtdx_context* ctx, rt_timepoint timepoint) {
	if (rtdx_is_timepoint_complete(timepoint)) {
		return true;
	}
	rt_mutex_lock(ctx->queue_lock);
	rtdx_queue* queue = rtdx_queue_from_timepoint(ctx, timepoint);
	bool reached = queue && queue->d3d_fence && queue->d3d_fence->GetCompletedValue() >= (timepoint.value & UINT64_C(0x00ffffffffffffff));
	rt_mutex_unlock(ctx->queue_lock);
	return reached;
}
