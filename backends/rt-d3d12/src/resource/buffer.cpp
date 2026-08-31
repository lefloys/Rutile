#include "buffer.hpp"
#include "command_buffer.hpp"
#include "context.hpp"
#include "error.hpp"
#include "resource/queue.hpp"

#include <string.h>

static bool rtd3d12_buffer_uses_host_storage(rt::memory_type memory_type) {
	return memory_type == rt::memory_type::host;
}

static rt_queue_t* rtd3d12_buffer_transfer_queue(rtd3d12_context* ctx) {
	rt_queue_t* queue = rtd3d12_context_queue(ctx, rt::queue_capability::transfer);
	return queue ? queue : rtd3d12_context_queue(ctx, rt::queue_capability::graphics);
}

rt_buffer_t* rtBufferCreate(void) { rtd3d12_begin_errorable_operation(); return rtd3d12::create_resource<rt_buffer_t>(rtd3d12_get_current_context()); }
void rtBufferDestroy(rt_buffer_t* buffer) { if (buffer) buffer->retire(); }
void rtBufferResize(rt_buffer_t* buffer, rt::memory_type memory_type, usize size) { rtd3d12_begin_errorable_operation(); rtd3d12_buffer_resize(rtd3d12_get_current_context(), buffer, memory_type, size); }
void rtBufferRead(rt_buffer_t* buffer, rt::buffer_range range, u08* data, usize data_size) { rtd3d12_begin_errorable_operation(); rtd3d12_buffer_read(rtd3d12_get_current_context(), buffer, range, data, data_size); }
u08* rtBufferMap(rt_buffer_t* buffer, rt::buffer_range range) { rtd3d12_begin_errorable_operation(); return rtd3d12_buffer_map(rtd3d12_get_current_context(), buffer, range); }
void rtBufferUnmap(rt_buffer_t* buffer) { rtd3d12_begin_errorable_operation(); rtd3d12_buffer_unmap(rtd3d12_get_current_context(), buffer); }

void rtd3d12_buffer_init(rtd3d12_context* ctx, rt_buffer_t* buffer) {
}

static rt_buffer_t* rtd3d12_buffer_node_create(rtd3d12_context* ctx, rt::memory_type memory_type, usize size) {
	rt_buffer_t* node = rtd3d12::create_resource<rt_buffer_t>(ctx);
	if (!node) {
		return nullptr;
	}
	node->zombie.store(true, std::memory_order_relaxed);
	node->size = size;
	node->memory_type = memory_type;

	const u64 allocation_size = size ? ((u64(size) + 255u) & ~UINT64_C(255)) : 1;
	D3D12_HEAP_PROPERTIES heap = {};
	heap.Type = rtd3d12_buffer_uses_host_storage(memory_type) ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
	heap.CreationNodeMask = 1;
	heap.VisibleNodeMask = 1;
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = allocation_size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	node->state = rtd3d12_buffer_uses_host_storage(memory_type) ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;
	const HRESULT result = ctx->d3d_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, node->state, nullptr, IID_PPV_ARGS(&node->d3d_resource));
	if (FAILED(result)) {
		delete node;
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommittedResource(buffer) failed: 0x{:08x}", static_cast<u32>(result));
		return nullptr;
	}
	node->vertex_view.BufferLocation = node->d3d_resource->GetGPUVirtualAddress();
	node->vertex_view.SizeInBytes = static_cast<UINT>(size);
	node->vertex_view.StrideInBytes = 0;
	return node;
}

rt_buffer_t::~rt_buffer_t() {
	if (d3d_resource) {
		if (d3d_resource) {
			d3d_resource->Release();
			d3d_resource = nullptr;
		}
		return;
	}
	if (active) active->release();
	while (next) {
		rt_buffer_t* node = next;
		next = node->next;
		node->next = nullptr;
		node->release();
	}
}

rt_buffer_t* rtd3d12_buffer_active_node(rt_buffer_t* buffer) {
	return buffer ? buffer->active : nullptr;
}

void rtd3d12_buffer_recycle_node(rt_buffer_t* buffer, rt_buffer_t* node) {
	if (!buffer || !node) {
		return;
	}
	node->next = buffer->next;
	buffer->next = node;
}

rt_buffer_t* rtd3d12_buffer_take_reusable_node(rt_buffer_t* buffer, rt::memory_type memory_type, usize size) {
	if (!buffer) {
		return nullptr;
	}
	for (rt_buffer_t** link = &buffer->next; *link; link = &(*link)->next) {
		rt_buffer_t* node = *link;
		if (node->memory_type == memory_type && node->size == size && node->ref_count.load(std::memory_order_relaxed) == 1) {
			*link = node->next;
			node->next = nullptr;
			return node;
		}
	}
	return nullptr;
}

void rtd3d12_buffer_resize(rtd3d12_context* ctx, rt_buffer_t* buffer, rt::memory_type memory_type, usize size) {
	if (!buffer || (memory_type != rt::memory_type::host && memory_type != rt::memory_type::device)) {
		rtd3d12_fail(rt::error::improper_usage, "buffer resize has an invalid buffer or memory type");
		return;
	}
	rt_buffer_t* node = rtd3d12_buffer_take_reusable_node(buffer, memory_type, size);
	if (!node) {
		node = rtd3d12_buffer_node_create(ctx, memory_type, size);
	}
	if (!node) {
		return;
	}
	if (buffer->active) {
		rtd3d12_buffer_recycle_node(buffer, buffer->active);
	}
	buffer->active = node;
}

rtd3d12_buffer_write rtd3d12_buffer_write_begin(rtd3d12_context* ctx, rt_buffer_t* buffer) {
	rtd3d12_buffer_write write = {};
	if (!buffer || !buffer->active) {
		return write;
	}
	write.target = buffer->active;
	if (write.target->ref_count.load(std::memory_order_relaxed) == 1) {
		return write;
	}
	write.source = write.target;
	write.target = rtd3d12_buffer_take_reusable_node(buffer, write.source->memory_type, write.source->size);
	if (!write.target) {
		write.target = rtd3d12_buffer_node_create(ctx, write.source->memory_type, write.source->size);
	}
	if (!write.target) {
		write.source = nullptr;
	}
	return write;
}

void rtd3d12_buffer_write_commit(rt_buffer_t* buffer, rtd3d12_buffer_write* write) {
	if (!buffer || !write || !write->source || !write->target) {
		return;
	}
	rtd3d12_buffer_recycle_node(buffer, write->source);
	buffer->active = write->target;
	write->source = nullptr;
	write->target = nullptr;
}

void rtd3d12_buffer_write_cancel(rt_buffer_t* buffer, rtd3d12_buffer_write* write) {
	if (!buffer || !write || !write->source || !write->target) {
		return;
	}
	rtd3d12_buffer_recycle_node(buffer, write->target);
	write->source = nullptr;
	write->target = nullptr;
}

static bool rtd3d12_buffer_copy_host_node(rt_buffer_t* source, rt_buffer_t* target) {
	if (!source || !target || !source->size) {
		return true;
	}
	D3D12_RANGE range = { 0, static_cast<SIZE_T>(source->size) };
	void* src = nullptr;
	void* dst = nullptr;
	if (FAILED(source->d3d_resource->Map(0, &range, &src)) || FAILED(target->d3d_resource->Map(0, nullptr, &dst))) {
		if (src) {
			source->d3d_resource->Unmap(0, nullptr);
		}
		rtd3d12_fail(rt::error::platform_failure, "Map(buffer revision) failed");
		return false;
	}
	memcpy(dst, src, source->size);
	target->d3d_resource->Unmap(0, nullptr);
	source->d3d_resource->Unmap(0, nullptr);
	return true;
}

static bool rtd3d12_buffer_submit_copy_and_upload(rtd3d12_context* ctx, rt_buffer_t* source, rt_buffer_t* target, usize offset, usize size, const u08* data, rt::timepoint* out_timepoint) {
	rt_queue_t* queue = rtd3d12_buffer_transfer_queue(ctx);
	if (!queue) {
		rtd3d12_fail(rt::error::improper_usage, "no queue is available for buffer upload");
		return false;
	}
	ID3D12Resource* upload = nullptr;
	if (size) {
		D3D12_HEAP_PROPERTIES heap = {};
		heap.Type = D3D12_HEAP_TYPE_UPLOAD;
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = size;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		HRESULT result = ctx->d3d_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload));
		if (FAILED(result)) {
			rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommittedResource(buffer upload) failed: 0x{:08x}", static_cast<u32>(result));
			return false;
		}
		void* mapped = nullptr;
		result = upload->Map(0, nullptr, &mapped);
		if (FAILED(result)) {
			if (upload) {
				upload->Release();
				upload = nullptr;
			}
			rtd3d12_fail(rtd3d12_error_from_hresult(result), "Map(buffer upload) failed: 0x{:08x}", static_cast<u32>(result));
			return false;
		}
		memcpy(mapped, data, size);
		upload->Unmap(0, nullptr);
	}
	ID3D12CommandAllocator* allocator = nullptr;
	ID3D12GraphicsCommandList* list = nullptr;
	HRESULT result = ctx->d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
	if (SUCCEEDED(result)) {
		result = ctx->d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, nullptr, IID_PPV_ARGS(&list));
	}
	if (FAILED(result)) {
		if (upload) {
			upload->Release();
			upload = nullptr;
		}
		if (allocator) {
			allocator->Release();
			allocator = nullptr;
		}
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommandList(buffer upload) failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}
	if (source && source != target) {
		rtd3d12_command_transition_buffer(list, source, D3D12_RESOURCE_STATE_COPY_SOURCE);
	}
	rtd3d12_command_transition_buffer(list, target, D3D12_RESOURCE_STATE_COPY_DEST);
	if (source && source != target) {
		list->CopyBufferRegion(target->d3d_resource, 0, source->d3d_resource, 0, source->size);
	}
	if (upload) {
		list->CopyBufferRegion(target->d3d_resource, offset, upload, 0, size);
	}
	rtd3d12_command_transition_buffer(list, target, D3D12_RESOURCE_STATE_COMMON);
	result = list->Close();
	if (FAILED(result)) {
		if (list) {
			list->Release();
			list = nullptr;
		}
		if (allocator) {
			allocator->Release();
			allocator = nullptr;
		}
		if (upload) {
			upload->Release();
			upload = nullptr;
		}
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "Close(buffer upload) failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}
	{
		std::lock_guard<std::mutex> queue_lock(ctx->queue_lock);
		ID3D12CommandList* lists[] = { list };
		queue->d3d_queue->ExecuteCommandLists(1, lists);
		const u64 fence = ++queue->fence_value;
		result = queue->d3d_queue->Signal(queue->d3d_fence, fence);
		if (SUCCEEDED(result)) {
			queue->fence_value = fence;
			*out_timepoint = rtd3d12_queue_timepoint(queue, fence);
		}
	}
	if (list) {
		list->Release();
		list = nullptr;
	}
	if (allocator) {
		allocator->Release();
		allocator = nullptr;
	}
	if (upload) {
		upload->Release();
		upload = nullptr;
	}
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "Signal(buffer upload) failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}
	return true;
}

rt::timepoint rtd3d12_buffer_subdata(rtd3d12_context* ctx, rt_buffer_t* buffer, u64 offset, u64 size, const void* data) {
	rt::timepoint timepoint = {};
	if (!buffer || !buffer->active || (!data && size) || offset > buffer->active->size || size > buffer->active->size - offset) {
		rtd3d12_fail(rt::error::improper_usage, "buffer upload range is out of bounds");
		return timepoint;
	}
	rtd3d12_buffer_write write = rtd3d12_buffer_write_begin(ctx, buffer);
	if (!write.target) {
		return timepoint;
	}
	if (write.target->memory_type == rt::memory_type::host) {
		if (write.source && !rtd3d12_buffer_copy_host_node(write.source, write.target)) {
			rtd3d12_buffer_write_cancel(buffer, &write);
			return timepoint;
		}
		if (size) {
			D3D12_RANGE range = { 0, 0 };
			void* mapped = nullptr;
			const HRESULT result = write.target->d3d_resource->Map(0, &range, &mapped);
			if (FAILED(result)) {
				rtd3d12_buffer_write_cancel(buffer, &write);
				rtd3d12_fail(rtd3d12_error_from_hresult(result), "Map(buffer upload) failed: 0x{:08x}", static_cast<u32>(result));
				return timepoint;
			}
			memcpy(static_cast<u08*>(mapped) + offset, data, static_cast<usize>(size));
			write.target->d3d_resource->Unmap(0, nullptr);
		}
		rtd3d12_buffer_write_commit(buffer, &write);
		return timepoint;
	}
	if (!rtd3d12_buffer_submit_copy_and_upload(ctx, write.source, write.target, static_cast<usize>(offset), static_cast<usize>(size), static_cast<const u08*>(data), &timepoint)) {
		rtd3d12_buffer_write_cancel(buffer, &write);
		return {};
	}
	rtd3d12_buffer_write_commit(buffer, &write);
	return timepoint;
}

void rtd3d12_buffer_read(rtd3d12_context* ctx, rt_buffer_t* buffer, rt::buffer_range range, u08* data, usize data_size) {
	rt_buffer_t* node = rtd3d12_buffer_active_node(buffer);
	if (!node || (!data && range.size) || data_size < range.size || range.offset > node->size || range.size > node->size - range.offset) {
		rtd3d12_fail(rt::error::improper_usage, "buffer read has an invalid range or destination");
		return;
	}
	if (!range.size) {
		return;
	}
	if (node->memory_type == rt::memory_type::host) {
		D3D12_RANGE read_range = { static_cast<SIZE_T>(range.offset), static_cast<SIZE_T>(range.offset + range.size) };
		void* mapped = nullptr;
		const HRESULT result = node->d3d_resource->Map(0, &read_range, &mapped);
		if (FAILED(result)) {
			rtd3d12_fail(rtd3d12_error_from_hresult(result), "Map(buffer read) failed: 0x{:08x}", static_cast<u32>(result));
			return;
		}
		memcpy(data, static_cast<u08*>(mapped) + range.offset, range.size);
		node->d3d_resource->Unmap(0, nullptr);
		return;
	}
	rt_queue_t* queue = rtd3d12_buffer_transfer_queue(ctx);
	if (!queue) {
		rtd3d12_fail(rt::error::improper_usage, "buffer read requires a queue");
		return;
	}
	rtd3d12_wait_for_timepoint(ctx, rtd3d12_queue_timepoint(queue, queue->fence_value));
	D3D12_HEAP_PROPERTIES heap = {};
	heap.Type = D3D12_HEAP_TYPE_READBACK;
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = range.size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ID3D12Resource* readback = nullptr;
	ID3D12CommandAllocator* allocator = nullptr;
	ID3D12GraphicsCommandList* list = nullptr;
	HRESULT result = ctx->d3d_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback));
	if (SUCCEEDED(result)) {
		result = ctx->d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
	}
	if (SUCCEEDED(result)) {
		result = ctx->d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, nullptr, IID_PPV_ARGS(&list));
	}
	if (FAILED(result)) {
		if (list) {
			list->Release();
			list = nullptr;
		}
		if (allocator) {
			allocator->Release();
			allocator = nullptr;
		}
		if (readback) {
			readback->Release();
			readback = nullptr;
		}
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "Create buffer readback command failed: 0x{:08x}", static_cast<u32>(result));
		return;
	}
	const D3D12_RESOURCE_STATES before = node->state;
	rtd3d12_command_transition_buffer(list, node, D3D12_RESOURCE_STATE_COPY_SOURCE);
	list->CopyBufferRegion(readback, 0, node->d3d_resource, range.offset, range.size);
	rtd3d12_command_transition_buffer(list, node, before);
	result = list->Close();
	if (SUCCEEDED(result)) {
		std::lock_guard<std::mutex> queue_lock(ctx->queue_lock);
		ID3D12CommandList* lists[] = { list };
		queue->d3d_queue->ExecuteCommandLists(1, lists);
		const u64 fence = ++queue->fence_value;
		result = queue->d3d_queue->Signal(queue->d3d_fence, fence);
		if (SUCCEEDED(result)) {
			queue->fence_value = fence;
			rtd3d12_wait_for_timepoint(ctx, rtd3d12_queue_timepoint(queue, fence));
		}
	}
	if (FAILED(result)) {
		if (list) {
			list->Release();
			list = nullptr;
		}
		if (allocator) {
			allocator->Release();
			allocator = nullptr;
		}
		if (readback) {
			readback->Release();
			readback = nullptr;
		}
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "Execute buffer readback failed: 0x{:08x}", static_cast<u32>(result));
		return;
	}
	D3D12_RANGE read_range = { 0, static_cast<SIZE_T>(range.size) };
	void* mapped = nullptr;
	result = readback->Map(0, &read_range, &mapped);
	if (SUCCEEDED(result)) {
		memcpy(data, mapped, range.size);
		readback->Unmap(0, nullptr);
	} else {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "Map(buffer readback) failed: 0x{:08x}", static_cast<u32>(result));
	}
	if (list) {
		list->Release();
		list = nullptr;
	}
	if (allocator) {
		allocator->Release();
		allocator = nullptr;
	}
	if (readback) {
		readback->Release();
		readback = nullptr;
	}
}

u08* rtd3d12_buffer_map(rtd3d12_context* ctx, rt_buffer_t* buffer, rt::buffer_range range) {
	rt_buffer_t* active = rtd3d12_buffer_active_node(buffer);
	if (!active || active->memory_type != rt::memory_type::host || range.offset > active->size || range.size > active->size - range.offset) {
		rtd3d12_fail(rt::error::improper_usage, "only a valid range of a host-memory buffer can be mapped");
		return nullptr;
	}
	rtd3d12_buffer_write write = rtd3d12_buffer_write_begin(ctx, buffer);
	if (!write.target) {
		return nullptr;
	}
	if (write.source && !rtd3d12_buffer_copy_host_node(write.source, write.target)) {
		rtd3d12_buffer_write_cancel(buffer, &write);
		return nullptr;
	}
	rtd3d12_buffer_write_commit(buffer, &write);
	D3D12_RANGE read_range = { static_cast<SIZE_T>(range.offset), static_cast<SIZE_T>(range.offset + range.size) };
	void* mapped = nullptr;
	const HRESULT result = buffer->active->d3d_resource->Map(0, &read_range, &mapped);
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "Map(buffer) failed: 0x{:08x}", static_cast<u32>(result));
		return nullptr;
	}
	return static_cast<u08*>(mapped) + range.offset;
}

void rtd3d12_buffer_unmap(rtd3d12_context*, rt_buffer_t* buffer) {
	rt_buffer_t* node = rtd3d12_buffer_active_node(buffer);
	if (!node || node->memory_type != rt::memory_type::host) {
		rtd3d12_fail(rt::error::improper_usage, "only host-memory buffers can be unmapped");
		return;
	}
	node->d3d_resource->Unmap(0, nullptr);
}
