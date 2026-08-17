#include "buffer.hpp"
#include "command_buffer.hpp"
#include "context.hpp"
#include "error.hpp"
#include "resource/queue.hpp"

#include <string.h>

static bool rtdx_buffer_uses_host_storage(rt_memory_type memory_type) {
	return memory_type == RT_HOST_MEMORY;
}

static rtdx_queue* rtdx_buffer_transfer_queue(rtdx_context* ctx) {
	rtdx_queue* queue = rtdx_context_queue(ctx, RT_QUEUE_TRANSFER);
	return queue ? queue : rtdx_context_queue(ctx, RT_QUEUE_GRAPHICS);
}

rt_buffer rtBufferCreate(void) { return rtdx_buffer_to_handle(rtdx_buffer_create(rtdx_get_current_context())); }
void rtBufferDestroy(rt_buffer buffer) { rtdx_buffer_destroy(rtdx_get_current_context(), rtdx_buffer_from_handle(buffer)); }
void rtBufferResize(rt_buffer buffer, rt_memory_type memory_type, usize size) { rtdx_buffer_resize(rtdx_get_current_context(), rtdx_buffer_from_handle(buffer), memory_type, size); }
void rtBufferRead(rt_buffer buffer, rt_buffer_range range, u08* data, usize data_size) { rtdx_buffer_read(rtdx_get_current_context(), rtdx_buffer_from_handle(buffer), range, data, data_size); }
u08* rtBufferMap(rt_buffer buffer, rt_buffer_range range) { return rtdx_buffer_map(rtdx_get_current_context(), rtdx_buffer_from_handle(buffer), range); }
void rtBufferUnmap(rt_buffer buffer) { rtdx_buffer_unmap(rtdx_get_current_context(), rtdx_buffer_from_handle(buffer)); }

RTDX_DEFINE_RESOURCE_PRIVATE(buffer)

void rtdx_buffer_init(rtdx_context* ctx, rtdx_buffer* buffer) {
}

static rtdx_buffer* rtdx_buffer_node_create(rtdx_context* ctx, rt_memory_type memory_type, usize size) {
	rtdx_buffer* node = RTDX_ALLOC_RESOURCE(rtdx_buffer, ctx);
	if (!node) {
		rtdx_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate buffer revision metadata");
		return NULL;
	}
	node->zombie.store(true, std::memory_order_relaxed);
	node->size = size;
	node->memory_type = memory_type;

	const u64 allocation_size = size ? ((u64(size) + 255u) & ~UINT64_C(255)) : 1;
	D3D12_HEAP_PROPERTIES heap = {};
	heap.Type = rtdx_buffer_uses_host_storage(memory_type) ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
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
	node->state = rtdx_buffer_uses_host_storage(memory_type) ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;
	const HRESULT result = ctx->d3d_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, node->state, NULL, IID_PPV_ARGS(&node->d3d_resource));
	if (FAILED(result)) {
		RTDX_FREE_RESOURCE(node);
		rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommittedResource(buffer) failed: 0x%08x", (u32)result);
		return NULL;
	}
	node->vertex_view.BufferLocation = node->d3d_resource->GetGPUVirtualAddress();
	node->vertex_view.SizeInBytes = (UINT)size;
	node->vertex_view.StrideInBytes = 0;
	return node;
}

void rtdx_buffer_finish(rtdx_context* ctx, rtdx_buffer* buffer) {
	if (buffer->d3d_resource) {
		rtdx_release(buffer->d3d_resource);
		return;
	}
	rtdx_release_resource(buffer->active);
	while (buffer->next) {
		rtdx_buffer* node = buffer->next;
		buffer->next = node->next;
		node->next = NULL;
		rtdx_release_resource(node);
	}
}

rtdx_buffer* rtdx_buffer_active_node(rtdx_buffer* buffer) {
	return buffer ? buffer->active : NULL;
}

void rtdx_buffer_recycle_node(rtdx_buffer* buffer, rtdx_buffer* node) {
	if (!buffer || !node) { return; }
	node->next = buffer->next;
	buffer->next = node;
}

rtdx_buffer* rtdx_buffer_take_reusable_node(rtdx_buffer* buffer, rt_memory_type memory_type, usize size) {
	if (!buffer) { return NULL; }
	for (rtdx_buffer** link = &buffer->next; *link; link = &(*link)->next) {
		rtdx_buffer* node = *link;
		if (node->memory_type == memory_type && node->size == size &&
			node->ref_count.load(std::memory_order_relaxed) == 1 && node->job_count.load(std::memory_order_relaxed) == 0) {
			*link = node->next;
			node->next = NULL;
			return node;
		}
	}
	return NULL;
}

void rtdx_buffer_resize(rtdx_context* ctx, rtdx_buffer* buffer, rt_memory_type memory_type, usize size) {
	if (!buffer || (memory_type != RT_HOST_MEMORY && memory_type != RT_DEVICE_MEMORY)) {
		rtdx_throwf(RT_IMPROPER_USAGE, "buffer resize has an invalid buffer or memory type");
		return;
	}
	rtdx_buffer* node = rtdx_buffer_take_reusable_node(buffer, memory_type, size);
	if (!node) { node = rtdx_buffer_node_create(ctx, memory_type, size); }
	if (!node) { return; }
	if (buffer->active) { rtdx_buffer_recycle_node(buffer, buffer->active); }
	buffer->active = node;
}

rtdx_buffer_write rtdx_buffer_write_begin(rtdx_context* ctx, rtdx_buffer* buffer) {
	rtdx_buffer_write write = {};
	if (!buffer || !buffer->active) { return write; }
	write.target = buffer->active;
	if (write.target->ref_count.load(std::memory_order_relaxed) == 1 && write.target->job_count.load(std::memory_order_relaxed) == 0) { return write; }
	write.source = write.target;
	write.target = rtdx_buffer_take_reusable_node(buffer, write.source->memory_type, write.source->size);
	if (!write.target) { write.target = rtdx_buffer_node_create(ctx, write.source->memory_type, write.source->size); }
	if (!write.target) { write.source = NULL; }
	return write;
}

void rtdx_buffer_write_commit(rtdx_buffer* buffer, rtdx_buffer_write* write) {
	if (!buffer || !write || !write->source || !write->target) { return; }
	rtdx_buffer_recycle_node(buffer, write->source);
	buffer->active = write->target;
	write->source = NULL;
	write->target = NULL;
}

void rtdx_buffer_write_cancel(rtdx_buffer* buffer, rtdx_buffer_write* write) {
	if (!buffer || !write || !write->source || !write->target) { return; }
	rtdx_buffer_recycle_node(buffer, write->target);
	write->source = NULL;
	write->target = NULL;
}

static bool rtdx_buffer_copy_host_node(rtdx_buffer* source, rtdx_buffer* target) {
	if (!source || !target || !source->size) { return true; }
	D3D12_RANGE range = { 0, (SIZE_T)source->size };
	void* src = NULL;
	void* dst = NULL;
	if (FAILED(source->d3d_resource->Map(0, &range, &src)) || FAILED(target->d3d_resource->Map(0, NULL, &dst))) {
		if (src) { source->d3d_resource->Unmap(0, NULL); }
		rtdx_throwf(RT_PLATFORM_FAILURE, "Map(buffer revision) failed");
		return false;
	}
	memcpy(dst, src, source->size);
	target->d3d_resource->Unmap(0, NULL);
	source->d3d_resource->Unmap(0, NULL);
	return true;
}

static bool rtdx_buffer_submit_copy_and_upload(rtdx_context* ctx, rtdx_buffer* source, rtdx_buffer* target, usize offset, usize size, const u08* data, rt_timepoint* out_timepoint) {
	rtdx_queue* queue = rtdx_buffer_transfer_queue(ctx);
	if (!queue) { rtdx_throwf(RT_IMPROPER_USAGE, "no queue is available for buffer upload"); return false; }
	ID3D12Resource* upload = NULL;
	if (size) {
		D3D12_HEAP_PROPERTIES heap = {}; heap.Type = D3D12_HEAP_TYPE_UPLOAD;
		D3D12_RESOURCE_DESC desc = {}; desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; desc.Width = size; desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1; desc.SampleDesc.Count = 1; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		HRESULT result = ctx->d3d_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&upload));
		if (FAILED(result)) { rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommittedResource(buffer upload) failed: 0x%08x", (u32)result); return false; }
		void* mapped = NULL; result = upload->Map(0, NULL, &mapped);
		if (FAILED(result)) { rtdx_release(upload); rtdx_throwf(rtdx_error_from_hresult(result), "Map(buffer upload) failed: 0x%08x", (u32)result); return false; }
		memcpy(mapped, data, size); upload->Unmap(0, NULL);
	}
	ID3D12CommandAllocator* allocator = NULL; ID3D12GraphicsCommandList* list = NULL;
	HRESULT result = ctx->d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
	if (SUCCEEDED(result)) { result = ctx->d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, NULL, IID_PPV_ARGS(&list)); }
	if (FAILED(result)) { rtdx_release(upload); rtdx_release(allocator); rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommandList(buffer upload) failed: 0x%08x", (u32)result); return false; }
	if (source && source != target) { rtdx_command_transition_buffer(list, source, D3D12_RESOURCE_STATE_COPY_SOURCE); }
	rtdx_command_transition_buffer(list, target, D3D12_RESOURCE_STATE_COPY_DEST);
	if (source && source != target) { list->CopyBufferRegion(target->d3d_resource, 0, source->d3d_resource, 0, source->size); }
	if (upload) { list->CopyBufferRegion(target->d3d_resource, offset, upload, 0, size); }
	rtdx_command_transition_buffer(list, target, D3D12_RESOURCE_STATE_COMMON);
	result = list->Close();
	if (FAILED(result)) { rtdx_release(list); rtdx_release(allocator); rtdx_release(upload); rtdx_throwf(rtdx_error_from_hresult(result), "Close(buffer upload) failed: 0x%08x", (u32)result); return false; }
	{ rtdx_physical_queue_scope physical_queue(*ctx); ID3D12CommandList* lists[] = { list }; queue->d3d_queue->ExecuteCommandLists(1, lists); const u64 fence = ++queue->fence_value; result = queue->d3d_queue->Signal(queue->d3d_fence, fence); if (SUCCEEDED(result)) { queue->fence_value = fence; *out_timepoint = rtdx_queue_timepoint(queue, fence); } }
	rtdx_release(list); rtdx_release(allocator); rtdx_release(upload);
	if (FAILED(result)) { rtdx_throwf(rtdx_error_from_hresult(result), "Signal(buffer upload) failed: 0x%08x", (u32)result); return false; }
	return true;
}

rt_timepoint rtdx_buffer_subdata(rtdx_context* ctx, rtdx_buffer* buffer, u64 offset, u64 size, const void* data) {
	rt_timepoint timepoint = {};
	if (!buffer || !buffer->active || (!data && size) || offset > buffer->active->size || size > buffer->active->size - offset) { rtdx_throwf(RT_IMPROPER_USAGE, "buffer upload range is out of bounds"); return timepoint; }
	rtdx_buffer_write write = rtdx_buffer_write_begin(ctx, buffer);
	if (!write.target) { return timepoint; }
	if (write.target->memory_type == RT_HOST_MEMORY) {
		if (write.source && !rtdx_buffer_copy_host_node(write.source, write.target)) { rtdx_buffer_write_cancel(buffer, &write); return timepoint; }
		if (size) { D3D12_RANGE range = { 0, 0 }; void* mapped = NULL; const HRESULT result = write.target->d3d_resource->Map(0, &range, &mapped); if (FAILED(result)) { rtdx_buffer_write_cancel(buffer, &write); rtdx_throwf(rtdx_error_from_hresult(result), "Map(buffer upload) failed: 0x%08x", (u32)result); return timepoint; } memcpy(static_cast<u08*>(mapped) + offset, data, (usize)size); write.target->d3d_resource->Unmap(0, NULL); }
		rtdx_buffer_write_commit(buffer, &write);
		return timepoint;
	}
	if (!rtdx_buffer_submit_copy_and_upload(ctx, write.source, write.target, (usize)offset, (usize)size, static_cast<const u08*>(data), &timepoint)) { rtdx_buffer_write_cancel(buffer, &write); return {}; }
	rtdx_buffer_write_commit(buffer, &write);
	return timepoint;
}

void rtdx_buffer_read(rtdx_context* ctx, rtdx_buffer* buffer, rt_buffer_range range, u08* data, usize data_size) {
	rtdx_buffer* node = rtdx_buffer_active_node(buffer);
	if (!node || (!data && range.size) || data_size < range.size || range.offset > node->size || range.size > node->size - range.offset) { rtdx_throwf(RT_IMPROPER_USAGE, "buffer read has an invalid range or destination"); return; }
	if (!range.size) { return; }
	if (node->memory_type == RT_HOST_MEMORY) {
		D3D12_RANGE read_range = { (SIZE_T)range.offset, (SIZE_T)(range.offset + range.size) }; void* mapped = NULL; const HRESULT result = node->d3d_resource->Map(0, &read_range, &mapped);
		if (FAILED(result)) { rtdx_throwf(rtdx_error_from_hresult(result), "Map(buffer read) failed: 0x%08x", (u32)result); return; }
		memcpy(data, static_cast<u08*>(mapped) + range.offset, range.size); node->d3d_resource->Unmap(0, NULL); return;
	}
	rtdx_queue* queue = rtdx_buffer_transfer_queue(ctx);
	if (!queue) { rtdx_throwf(RT_IMPROPER_USAGE, "buffer read requires a queue"); return; }
	rtdx_wait_for_timepoint(ctx, rtdx_queue_timepoint(queue, queue->fence_value));
	D3D12_HEAP_PROPERTIES heap = {}; heap.Type = D3D12_HEAP_TYPE_READBACK;
	D3D12_RESOURCE_DESC desc = {}; desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; desc.Width = range.size; desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1; desc.SampleDesc.Count = 1; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ID3D12Resource* readback = NULL; ID3D12CommandAllocator* allocator = NULL; ID3D12GraphicsCommandList* list = NULL;
	HRESULT result = ctx->d3d_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&readback));
	if (SUCCEEDED(result)) { result = ctx->d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)); }
	if (SUCCEEDED(result)) { result = ctx->d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, NULL, IID_PPV_ARGS(&list)); }
	if (FAILED(result)) { rtdx_release(list); rtdx_release(allocator); rtdx_release(readback); rtdx_throwf(rtdx_error_from_hresult(result), "Create buffer readback command failed: 0x%08x", (u32)result); return; }
	const D3D12_RESOURCE_STATES before = node->state; rtdx_command_transition_buffer(list, node, D3D12_RESOURCE_STATE_COPY_SOURCE); list->CopyBufferRegion(readback, 0, node->d3d_resource, range.offset, range.size); rtdx_command_transition_buffer(list, node, before);
	result = list->Close(); if (SUCCEEDED(result)) { rtdx_physical_queue_scope physical_queue(*ctx); ID3D12CommandList* lists[] = { list }; queue->d3d_queue->ExecuteCommandLists(1, lists); const u64 fence = ++queue->fence_value; result = queue->d3d_queue->Signal(queue->d3d_fence, fence); if (SUCCEEDED(result)) { queue->fence_value = fence; rtdx_wait_for_timepoint(ctx, rtdx_queue_timepoint(queue, fence)); } }
	if (FAILED(result)) { rtdx_release(list); rtdx_release(allocator); rtdx_release(readback); rtdx_throwf(rtdx_error_from_hresult(result), "Execute buffer readback failed: 0x%08x", (u32)result); return; }
	D3D12_RANGE read_range = { 0, (SIZE_T)range.size }; void* mapped = NULL; result = readback->Map(0, &read_range, &mapped); if (SUCCEEDED(result)) { memcpy(data, mapped, range.size); readback->Unmap(0, NULL); } else { rtdx_throwf(rtdx_error_from_hresult(result), "Map(buffer readback) failed: 0x%08x", (u32)result); }
	rtdx_release(list); rtdx_release(allocator); rtdx_release(readback);
}

u08* rtdx_buffer_map(rtdx_context* ctx, rtdx_buffer* buffer, rt_buffer_range range) {
	rtdx_buffer* active = rtdx_buffer_active_node(buffer);
	if (!active || active->memory_type != RT_HOST_MEMORY || range.offset > active->size || range.size > active->size - range.offset) { rtdx_throwf(RT_IMPROPER_USAGE, "only a valid range of a host-memory buffer can be mapped"); return NULL; }
	rtdx_buffer_write write = rtdx_buffer_write_begin(ctx, buffer);
	if (!write.target) { return NULL; }
	if (write.source && !rtdx_buffer_copy_host_node(write.source, write.target)) { rtdx_buffer_write_cancel(buffer, &write); return NULL; }
	rtdx_buffer_write_commit(buffer, &write);
	D3D12_RANGE read_range = { (SIZE_T)range.offset, (SIZE_T)(range.offset + range.size) }; void* mapped = NULL; const HRESULT result = buffer->active->d3d_resource->Map(0, &read_range, &mapped);
	if (FAILED(result)) { rtdx_throwf(rtdx_error_from_hresult(result), "Map(buffer) failed: 0x%08x", (u32)result); return NULL; }
	return static_cast<u08*>(mapped) + range.offset;
}

void rtdx_buffer_unmap(rtdx_context*, rtdx_buffer* buffer) {
	rtdx_buffer* node = rtdx_buffer_active_node(buffer);
	if (!node || node->memory_type != RT_HOST_MEMORY) { rtdx_throwf(RT_IMPROPER_USAGE, "only host-memory buffers can be unmapped"); return; }
	node->d3d_resource->Unmap(0, NULL);
}

void rtdx_buffer::finish() { rtdx_buffer_finish(ctx, this); }

