#include "buffer.hpp"
#include "context.hpp"
#include "error.hpp"
#include "resource/queue.hpp"
#include "resource/texture.hpp"

#include <stdlib.h>
#include <string.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

static bool rtdx_buffer_uses_host_storage(enum rt_memory_type memory_type) {
	return memory_type == RT_HOST_MEMORY;
}

static struct rtdx_queue* rtdx_buffer_upload_queue(struct rtdx_context* ctx) {
	struct rtdx_queue* queue = rtdx_context_queue(ctx, RT_QUEUE_TRANSFER);
	if (queue) {
		return queue;
	}
	return rtdx_context_queue(ctx, RT_QUEUE_GRAPHICS);
}

static D3D12_RESOURCE_STATES rtdx_buffer_gpu_state(void) {
	return D3D12_RESOURCE_STATE_COMMON;
}

rt_buffer rtBufferCreate(void) {
	struct rtdx_buffer* buffer = rtdx_buffer_create(rtdx_get_current_context());
	return rtdx_buffer_to_handle(buffer);
}

void rtBufferDestroy(rt_buffer buffer) {
	rtdx_buffer_destroy(rtdx_get_current_context(), rtdx_buffer_from_handle(buffer));
}

void rtBufferResize(rt_buffer buffer, enum rt_memory_type memory_type, usize size) {
	rtdx_buffer_resize(rtdx_get_current_context(), rtdx_buffer_from_handle(buffer), memory_type, size);
}

void rtBufferRead(rt_buffer buffer, rt_buffer_range range, u08* data, usize data_size) {
	rtdx_buffer_read(rtdx_get_current_context(), rtdx_buffer_from_handle(buffer), range, data, data_size);
}

u08* rtBufferMap(rt_buffer buffer, rt_buffer_range range) {
	return rtdx_buffer_map(rtdx_get_current_context(), rtdx_buffer_from_handle(buffer), range);
}

void rtBufferUnmap(rt_buffer buffer) {
	rtdx_buffer_unmap(rtdx_get_current_context(), rtdx_buffer_from_handle(buffer));
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTDX_DEFINE_RESOURCE_PRIVATE(buffer)

void rtdx_buffer_init(struct rtdx_context* ctx, struct rtdx_buffer* buffer) {
	rtdx_init_resource_base(ctx, RTDX_RESOURCE_BASE(buffer), rtdx_resource_type::buffer);
	buffer->memory_type = RT_DEVICE_MEMORY;
}

static struct rtdx_buffer_storage* rtdx_buffer_storage_create(
	struct rtdx_context* ctx,
	u64 size,
	enum rt_memory_type memory_type
) {
	struct rtdx_buffer_storage* storage = RTDX_ALLOC_RESOURCE(rtdx_buffer_storage);
	if (!storage) {
		rtdx_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate buffer storage metadata");
		return NULL;
	}

	storage->ctx = ctx;
	storage->ref_count = 1;
	storage->size = size;
	storage->memory_type = memory_type;
	/* D3D12 constant-buffer views are 256-byte aligned and sized. Keep the
	 * public/logical size exact, but make every physical allocation large enough
	 * for a rounded CBV that begins at byte zero. */
	u64 allocation_size = size ? ((size + 255u) & ~UINT64_C(255)) : 1;

	D3D12_HEAP_PROPERTIES heap = {};
	heap.Type = rtdx_buffer_uses_host_storage(memory_type) ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
	heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heap.CreationNodeMask = 1;
	heap.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = allocation_size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	storage->state = rtdx_buffer_uses_host_storage(memory_type) ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;
	HRESULT result = ctx->d3d_device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		storage->state,
		NULL,
		IID_PPV_ARGS(&storage->d3d_resource)
	);
	if (FAILED(result)) {
		RTDX_FREE_RESOURCE(storage);
		rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommittedResource(buffer) failed: 0x%08x", (u32)result);
		return NULL;
	}

	if (!rtdx_buffer_uses_host_storage(memory_type) && size) {
		storage->shadow_data = new (std::nothrow) u08[(usize)size]{};
		if (!storage->shadow_data) {
			rtdx_release(&storage->d3d_resource);
			RTDX_FREE_RESOURCE(storage);
			rtdx_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate static buffer shadow copy");
			return NULL;
		}
	}
	storage->shadow_valid = true;

	storage->vertex_view.BufferLocation = storage->d3d_resource->GetGPUVirtualAddress();
	storage->vertex_view.SizeInBytes = (UINT)size;
	storage->vertex_view.StrideInBytes = 0;
	return storage;
}

void rtdx_buffer_storage_retain(struct rtdx_buffer_storage* storage) {
	if (!storage) {
		return;
	}
	storage->ref_count++;
}

void rtdx_buffer_storage_release(struct rtdx_buffer_storage* storage) {
	if (!storage) {
		return;
	}
	if (--storage->ref_count != 0) {
		return;
	}
	rtdx_buffer_storage_clear_texture_source(storage);
	delete[] static_cast<u08*>(storage->shadow_data);
	rtdx_release(&storage->d3d_resource);
	RTDX_FREE_RESOURCE(storage);
}

static bool rtdx_buffer_storage_write_host(struct rtdx_buffer_storage* storage, u64 offset, u64 size, const void* data) {
	if (!storage || !data || !size) {
		return true;
	}

	D3D12_RANGE read_range = { 0, 0 };
	void* mapped_data = NULL;
	HRESULT result = storage->d3d_resource->Map(0, &read_range, &mapped_data);
	if (FAILED(result)) {
		rtdx_throwf(rtdx_error_from_hresult(result), "ID3D12Resource::Map failed: 0x%08x", (u32)result);
		return false;
	}
	memcpy((char*)mapped_data + offset, data, (usize)size);
	storage->d3d_resource->Unmap(0, NULL);
	return true;
}

static bool rtdx_buffer_storage_write_shadow(struct rtdx_buffer_storage* storage, u64 offset, u64 size, const void* data) {
	if (!storage || !size) {
		return true;
	}
	if (!storage->shadow_data) {
		rtdx_throwf(RT_PLATFORM_FAILURE, "static buffer shadow storage is missing");
		return false;
	}
	if (data) {
		memcpy((char*)storage->shadow_data + offset, data, (usize)size);
	}
	return true;
}

static void rtdx_buffer_storage_copy(struct rtdx_buffer_storage* dst, struct rtdx_buffer_storage* src) {
	if (!dst || !src) {
		return;
	}
	rtdx_buffer_storage_clear_texture_source(dst);

	u64 copy_size = dst->size < src->size ? dst->size : src->size;
	if (!copy_size) {
		return;
	}

	if (dst->shadow_data && src->shadow_data) {
		memcpy(dst->shadow_data, src->shadow_data, (usize)copy_size);
		dst->shadow_valid = src->shadow_valid;
		dst->shadow_invalid_ranges = src->shadow_invalid_ranges;
		for (const rtdx_buffer_texture_source& source : src->texture_sources) { rtdx_buffer_storage_set_texture_source(dst, source.image, source.source_range, source.destination_range); }
		return;
	}

	D3D12_RANGE read_range = { 0, (SIZE_T)copy_size };
	void* src_data = NULL;
	if (FAILED(src->d3d_resource->Map(0, &read_range, &src_data))) {
		return;
	}

	void* dst_data = NULL;
	if (FAILED(dst->d3d_resource->Map(0, NULL, &dst_data))) {
		src->d3d_resource->Unmap(0, NULL);
		return;
	}
	memcpy(dst_data, src_data, (usize)copy_size);
	dst->d3d_resource->Unmap(0, NULL);
	src->d3d_resource->Unmap(0, NULL);
}

void rtdx_buffer_storage_clear_texture_source(rtdx_buffer_storage* storage) {
	if (!storage) { return; }
	for (const rtdx_buffer_texture_source& source : storage->texture_sources) {
		if (source.image) { rtdx_resource_release(RTDX_RESOURCE_BASE(source.image)); }
	}
	storage->texture_sources.clear();
}

static bool rtdx_buffer_ranges_overlap(rt_buffer_range a, rt_buffer_range b) {
	return a.offset < b.offset + b.size && b.offset < a.offset + a.size;
}

bool rtdx_buffer_storage_shadow_range_valid(const rtdx_buffer_storage* storage, rt_buffer_range range) {
	if (!storage || !storage->shadow_data || !range.size) { return false; }
	for (const rt_buffer_range& invalid : storage->shadow_invalid_ranges) {
		if (rtdx_buffer_ranges_overlap(invalid, range)) { return false; }
	}
	return true;
}

void rtdx_buffer_storage_mark_shadow_invalid(rtdx_buffer_storage* storage, rt_buffer_range range) {
	if (!storage || !storage->shadow_data || !range.size) { return; }
	usize begin = range.offset;
	usize end = range.offset + range.size;
	for (auto it = storage->shadow_invalid_ranges.begin(); it != storage->shadow_invalid_ranges.end();) {
		const usize invalid_end = it->offset + it->size;
		if (invalid_end < begin || end < it->offset) { ++it; continue; }
		begin = begin < it->offset ? begin : it->offset;
		end = end > invalid_end ? end : invalid_end;
		it = storage->shadow_invalid_ranges.erase(it);
	}
	storage->shadow_invalid_ranges.push_back({ end - begin, begin });
	storage->shadow_valid = storage->shadow_invalid_ranges.empty();
}

void rtdx_buffer_storage_mark_shadow_valid(rtdx_buffer_storage* storage, rt_buffer_range range) {
	if (!storage || !storage->shadow_data || !range.size) { return; }
	const usize begin = range.offset;
	const usize end = range.offset + range.size;
	std::vector<rt_buffer_range> remaining;
	for (const rt_buffer_range invalid : storage->shadow_invalid_ranges) {
		const usize invalid_begin = invalid.offset;
		const usize invalid_end = invalid.offset + invalid.size;
		if (invalid_end <= begin || end <= invalid_begin) { remaining.push_back(invalid); continue; }
		if (invalid_begin < begin) { remaining.push_back({ begin - invalid_begin, invalid_begin }); }
		if (end < invalid_end) { remaining.push_back({ invalid_end - end, end }); }
	}
	storage->shadow_invalid_ranges = std::move(remaining);
	storage->shadow_valid = storage->shadow_invalid_ranges.empty();
}

void rtdx_buffer_storage_invalidate_texture_source(rtdx_buffer_storage* storage, rt_buffer_range range) {
	if (!storage || !range.size) { return; }
	for (auto it = storage->texture_sources.begin(); it != storage->texture_sources.end();) {
		if (!rtdx_buffer_ranges_overlap(it->destination_range, range)) { ++it; continue; }
		if (it->image) { rtdx_resource_release(RTDX_RESOURCE_BASE(it->image)); }
		it = storage->texture_sources.erase(it);
	}
}

void rtdx_buffer_storage_set_texture_source(rtdx_buffer_storage* storage, rtdx_image_base* image, rt_texture_range range, rt_buffer_range destination_range) {
	if (!storage) { return; }
	rtdx_buffer_storage_invalidate_texture_source(storage, destination_range);
	if (image) { rtdx_resource_retain(RTDX_RESOURCE_BASE(image)); }
	storage->texture_sources.push_back({ image, range, destination_range });
}

static bool rtdx_buffer_upload_staging(struct rtdx_context* ctx, struct rtdx_queue* queue, u64 size) {
	if (queue->upload_buffer && queue->upload_buffer_size >= size) {
		rtdx_wait_for_timepoint(ctx, rtdx_queue_timepoint(queue, queue->upload_fence_value));
		queue->upload_fence_value = 0;
		return true;
	}

	rtdx_wait_for_timepoint(ctx, rtdx_queue_timepoint(queue, queue->upload_fence_value));
	queue->upload_fence_value = 0;
	rtdx_release(&queue->upload_buffer);
	queue->upload_buffer_size = 0;

	D3D12_HEAP_PROPERTIES heap = {};
	heap.Type = D3D12_HEAP_TYPE_UPLOAD;
	heap.CreationNodeMask = 1;
	heap.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT result = ctx->d3d_device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL,
		IID_PPV_ARGS(&queue->upload_buffer)
	);
	if (FAILED(result)) {
		rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommittedResource(upload) failed: 0x%08x", (u32)result);
		return false;
	}
	queue->upload_buffer_size = size;
	return true;
}

static rt_timepoint rtdx_buffer_upload_static(
	struct rtdx_context* ctx,
	struct rtdx_queue* queue,
	struct rtdx_buffer_storage* storage,
	u64 offset,
	u64 size,
	const void* data
) {
	rt_timepoint timepoint = {};
	if (!size) {
		return timepoint;
	}
	if (!queue) {
		rtdx_throwf(RT_IMPROPER_USAGE, "static buffer uploads require a valid queue");
		return timepoint;
	}

	rtdx_queue_upload_scope upload_scope(queue);

	if (!rtdx_buffer_upload_staging(ctx, queue, size)) {
		return timepoint;
	}

	rtdx_queue_collect(ctx, queue);

	void* mapped_data = NULL;
	HRESULT result = queue->upload_buffer->Map(0, NULL, &mapped_data);
	if (FAILED(result)) {
		rtdx_throwf(rtdx_error_from_hresult(result), "ID3D12Resource::Map failed: 0x%08x", (u32)result);
		return timepoint;
	}
	memcpy(mapped_data, data, (usize)size);
	queue->upload_buffer->Unmap(0, NULL);

	rtdx_physical_queue_scope physical_queue(ctx);
	if (!rtdx_queue_acquire_upload_command(ctx, queue)) {
		return timepoint;
	}

	ID3D12GraphicsCommandList* command_list = queue->upload_command_list;

	if (storage->state != D3D12_RESOURCE_STATE_COPY_DEST) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = storage->d3d_resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = storage->state;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		command_list->ResourceBarrier(1, &barrier);
	}

	command_list->CopyBufferRegion(storage->d3d_resource, offset, queue->upload_buffer, 0, size);

	D3D12_RESOURCE_STATES next_state = rtdx_buffer_gpu_state();
	if (next_state != D3D12_RESOURCE_STATE_COPY_DEST) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = storage->d3d_resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = next_state;
		command_list->ResourceBarrier(1, &barrier);
	}

	result = command_list->Close();
	if (FAILED(result)) {
		rtdx_throwf(rtdx_error_from_hresult(result), "ID3D12GraphicsCommandList::Close failed: 0x%08x", (u32)result);
		return timepoint;
	}

	ID3D12CommandList* lists[] = { command_list };
	queue->d3d_queue->ExecuteCommandLists(1, lists);

	u64 signal_value = ++ctx->next_fence_value;
	result = queue->d3d_queue->Signal(queue->d3d_fence, signal_value);
	if (FAILED(result)) {
		rtdx_throwf(rtdx_error_from_hresult(result), "ID3D12CommandQueue::Signal failed: 0x%08x", (u32)result);
		return timepoint;
	}

	queue->fence_value = signal_value;
	timepoint = rtdx_queue_timepoint(queue, signal_value);
	storage->state = next_state;
	queue->upload_fence_value = signal_value;
	return timepoint;
}

static void rtdx_buffer_recycle_storage(struct rtdx_buffer* buffer, struct rtdx_buffer_storage* storage) {
	if (!storage) {
		return;
	}
	storage->next = buffer->reusable_storage;
	buffer->reusable_storage = storage;
}

static struct rtdx_buffer_storage* rtdx_buffer_take_reusable_storage(
	struct rtdx_buffer* buffer,
	u64 size,
	enum rt_memory_type memory_type
) {
	struct rtdx_buffer_storage** link = &buffer->reusable_storage;

	while (*link) {
		struct rtdx_buffer_storage* storage = *link;
		if (storage->size == size &&
			storage->memory_type == memory_type &&
			storage->ref_count == 1) {
			*link = storage->next;
			storage->next = NULL;
			return storage;
		}
		link = &storage->next;
	}
	return NULL;
}

void rtdx_buffer_finish(struct rtdx_context* ctx, struct rtdx_buffer* buffer) {
	rtdx_buffer_storage_release(buffer->storage);
	buffer->storage = NULL;

	struct rtdx_buffer_storage* storage = buffer->reusable_storage;
	while (storage) {
		struct rtdx_buffer_storage* next = storage->next;
		storage->next = NULL;
		rtdx_buffer_storage_release(storage);
		storage = next;
	}
	buffer->reusable_storage = NULL;
	rtdx_finish_resource_base(ctx, RTDX_RESOURCE_BASE(buffer));
}

void rtdx_buffer_resize(struct rtdx_context* ctx, struct rtdx_buffer* buffer, enum rt_memory_type memory_type, usize size) {
	if (!buffer) {
		rtdx_throwf(RT_IMPROPER_USAGE, "buffer is NULL");
		return;
	}
	if (memory_type != RT_HOST_MEMORY && memory_type != RT_DEVICE_MEMORY) {
		rtdx_throwf(RT_IMPROPER_USAGE, "unsupported buffer memory type");
		return;
	}

	rtdx_queue_collect(ctx, NULL);
	buffer->memory_type = memory_type;
	rtdx_buffer_recycle_storage(buffer, buffer->storage);
	buffer->storage = NULL;

	struct rtdx_buffer_storage* storage = rtdx_buffer_take_reusable_storage(buffer, (u64)size, memory_type);
	if (!storage) {
		storage = rtdx_buffer_storage_create(ctx, (u64)size, memory_type);
	}
	if (!storage) {
		return;
	}
	rtdx_buffer_storage_clear_texture_source(storage);
	storage->shadow_valid = true;
	storage->shadow_invalid_ranges.clear();

	buffer->storage = storage;
}

rtdx_buffer_write rtdx_buffer_write_begin(rtdx_context* ctx, rtdx_buffer* buffer) {
	rtdx_buffer_write write = {};
	if (!buffer || !buffer->storage) {
		return write;
	}
	write.target = buffer->storage;
	if (buffer->storage->ref_count == 1) {
		return write;
	}

	rtdx_buffer_storage* source = buffer->storage;
	rtdx_buffer_recycle_storage(buffer, source);
	buffer->storage = NULL;
	rtdx_buffer_storage* target = rtdx_buffer_take_reusable_storage(buffer, source->size, source->memory_type);
	if (!target) {
		target = rtdx_buffer_storage_create(ctx, source->size, source->memory_type);
	}
	if (!target) {
		buffer->storage = source;
		return {};
	}
	rtdx_buffer_storage_copy(target, source);
	buffer->storage = target;
	write.source = source;
	write.target = target;
	return write;
}

rt_timepoint rtdx_buffer_subdata(struct rtdx_context* ctx, struct rtdx_buffer* buffer, u64 offset, u64 size, const void* data) {
	rt_timepoint timepoint = {};
	if (!buffer || !buffer->storage) {
		rtdx_throwf(RT_IMPROPER_USAGE, "buffer has no storage");
		return timepoint;
	}
	if (offset > buffer->storage->size || size > buffer->storage->size - offset) {
		rtdx_throwf(RT_IMPROPER_USAGE, "buffer upload range is out of bounds");
		return timepoint;
	}
	struct rtdx_queue* queue = NULL;
	if (!rtdx_buffer_uses_host_storage(buffer->storage->memory_type) && size && data) {
		queue = rtdx_buffer_upload_queue(ctx);
		if (!queue) {
			rtdx_throwf(RT_IMPROPER_USAGE, "no queue is available for static buffer uploads");
			return timepoint;
		}
	}
	rtdx_queue_collect(ctx, queue);
	if (!size || !data) {
		return timepoint;
	}

	bool replaced_storage = false;
	if (buffer->storage->ref_count > 1) {
		struct rtdx_buffer_storage* old_storage = buffer->storage;

		rtdx_buffer_recycle_storage(buffer, old_storage);
		buffer->storage = NULL;

		struct rtdx_buffer_storage* new_storage = rtdx_buffer_take_reusable_storage(buffer, old_storage->size, old_storage->memory_type);
		if (!new_storage) {
			new_storage = rtdx_buffer_storage_create(ctx, old_storage->size, old_storage->memory_type);
		}
		if (!new_storage) {
			return timepoint;
		}
		rtdx_buffer_storage_copy(new_storage, old_storage);
		buffer->storage = new_storage;
		replaced_storage = true;
	}

	if (rtdx_buffer_uses_host_storage(buffer->storage->memory_type)) {
		if (!rtdx_buffer_storage_write_host(buffer->storage, offset, size, data)) {
			return timepoint;
		}
		return timepoint;
	}

	if (!rtdx_buffer_storage_write_shadow(buffer->storage, offset, size, data)) {
		return timepoint;
	}
	if (replaced_storage) {
		return rtdx_buffer_upload_static(ctx, queue, buffer->storage, 0, buffer->storage->size, buffer->storage->shadow_data);
	}
	return rtdx_buffer_upload_static(ctx, queue, buffer->storage, offset, size, (const char*)buffer->storage->shadow_data + offset);
}

void rtdx_buffer_read(struct rtdx_context* ctx, struct rtdx_buffer* buffer, rt_buffer_range range, u08* data, usize data_size) {
	const u64 offset = (u64)range.offset;
	const u64 size = (u64)range.size;
	if (!buffer || !buffer->storage) {
		rtdx_throwf(RT_IMPROPER_USAGE, "buffer has no storage");
		return;
	}
	if ((!data && size) || data_size < range.size) {
		rtdx_throwf(RT_IMPROPER_USAGE, "buffer read destination is too small");
		return;
	}
	if (offset > buffer->storage->size || size > buffer->storage->size - offset) {
		rtdx_throwf(RT_IMPROPER_USAGE, "buffer read range is out of bounds");
		return;
	}
	if (!size) {
		return;
	}

	if (rtdx_buffer_uses_host_storage(buffer->storage->memory_type)) {
		D3D12_RANGE read_range = { (SIZE_T)offset, (SIZE_T)(offset + size) };
		void* mapped_data = NULL;
		HRESULT result = buffer->storage->d3d_resource->Map(0, &read_range, &mapped_data);
		if (FAILED(result)) { rtdx_throwf(rtdx_error_from_hresult(result), "ID3D12Resource::Map failed: 0x%08x", (u32)result); return; }
		memcpy(data, static_cast<const u08*>(mapped_data) + offset, (usize)size);
		D3D12_RANGE write_range = { 0, 0 };
		buffer->storage->d3d_resource->Unmap(0, &write_range);
		return;
	}

	/* Device storage can have been written by recorded texture copies, so its
	 * host shadow is only an upload cache. Read the physical bytes instead. */
	rtdx_queue* queue = rtdx_buffer_upload_queue(ctx);
	if (!queue) { rtdx_throwf(RT_IMPROPER_USAGE, "buffer read requires a queue"); return; }
	rtdx_wait_for_timepoint(ctx, rtdx_queue_timepoint(queue, queue->fence_value));
	D3D12_HEAP_PROPERTIES heap = {}; heap.Type = D3D12_HEAP_TYPE_READBACK;
	D3D12_RESOURCE_DESC desc = {}; desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; desc.Width = size; desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1; desc.SampleDesc.Count = 1; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ID3D12Resource* readback = NULL; HRESULT result = ctx->d3d_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&readback));
	if (FAILED(result)) { rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommittedResource(buffer readback) failed: 0x%08x", (u32)result); return; }
	ID3D12CommandAllocator* allocator = NULL; result = ctx->d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
	if (FAILED(result)) { rtdx_release(&readback); rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommandAllocator(buffer readback) failed: 0x%08x", (u32)result); return; }
	ID3D12GraphicsCommandList* list = NULL; result = ctx->d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, NULL, IID_PPV_ARGS(&list));
	if (FAILED(result)) { rtdx_release(&allocator); rtdx_release(&readback); rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommandList(buffer readback) failed: 0x%08x", (u32)result); return; }
	const D3D12_RESOURCE_STATES previous = buffer->storage->state;
	if (previous != D3D12_RESOURCE_STATE_COPY_SOURCE) { D3D12_RESOURCE_BARRIER barrier = {}; barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; barrier.Transition.pResource = buffer->storage->d3d_resource; barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES; barrier.Transition.StateBefore = previous; barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE; list->ResourceBarrier(1, &barrier); }
	list->CopyBufferRegion(readback, 0, buffer->storage->d3d_resource, offset, size);
	if (previous != D3D12_RESOURCE_STATE_COPY_SOURCE) { D3D12_RESOURCE_BARRIER barrier = {}; barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; barrier.Transition.pResource = buffer->storage->d3d_resource; barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES; barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE; barrier.Transition.StateAfter = previous; list->ResourceBarrier(1, &barrier); }
	result = list->Close(); if (FAILED(result)) { rtdx_release(&list); rtdx_release(&allocator); rtdx_release(&readback); rtdx_throwf(rtdx_error_from_hresult(result), "Close(buffer readback) failed: 0x%08x", (u32)result); return; }
	{ rtdx_physical_queue_scope physical_queue(ctx); ID3D12CommandList* lists[] = { list }; queue->d3d_queue->ExecuteCommandLists(1, lists); u64 fence = ++ctx->next_fence_value; result = queue->d3d_queue->Signal(queue->d3d_fence, fence); if (FAILED(result)) { rtdx_release(&list); rtdx_release(&allocator); rtdx_release(&readback); rtdx_throwf(rtdx_error_from_hresult(result), "Signal(buffer readback) failed: 0x%08x", (u32)result); return; } queue->fence_value = fence; rtdx_wait_for_timepoint(ctx, rtdx_queue_timepoint(queue, fence)); }
	D3D12_RANGE read_range = { 0, (SIZE_T)size }; void* mapped_data = NULL; result = readback->Map(0, &read_range, &mapped_data);
	if (FAILED(result)) { rtdx_release(&list); rtdx_release(&allocator); rtdx_release(&readback); rtdx_throwf(rtdx_error_from_hresult(result), "Map(buffer readback) failed: 0x%08x", (u32)result); return; }
	memcpy(data, mapped_data, (usize)size); D3D12_RANGE write_range = { 0, 0 }; readback->Unmap(0, &write_range);
	rtdx_release(&list); rtdx_release(&allocator); rtdx_release(&readback);
	return;
}

u08* rtdx_buffer_map(struct rtdx_context* ctx, struct rtdx_buffer* buffer, rt_buffer_range range) {
	if (!buffer || !buffer->storage) {
		rtdx_throwf(RT_IMPROPER_USAGE, "buffer has no storage");
		return NULL;
	}
	if (!rtdx_buffer_uses_host_storage(buffer->storage->memory_type)) {
		rtdx_throwf(RT_IMPROPER_USAGE, "only host-memory buffers can be mapped");
		return NULL;
	}
	if (range.offset > buffer->storage->size || range.size > buffer->storage->size - range.offset) {
		rtdx_throwf(RT_IMPROPER_USAGE, "buffer map range is out of bounds");
		return NULL;
	}
	/* Mapping exposes writable bytes, so it must not mutate a storage revision
	 * retained by already recorded readers. */
	if (!rtdx_buffer_write_begin(ctx, buffer).target) {
		return NULL;
	}
	rtdx_buffer_storage_invalidate_texture_source(buffer->storage, range);

	D3D12_RANGE read_range = { (SIZE_T)range.offset, (SIZE_T)(range.offset + range.size) };
	void* mapped_data = NULL;
	const HRESULT result = buffer->storage->d3d_resource->Map(0, &read_range, &mapped_data);
	if (FAILED(result)) {
		rtdx_throwf(rtdx_error_from_hresult(result), "ID3D12Resource::Map failed: 0x%08x", (u32)result);
		return NULL;
	}
	return static_cast<u08*>(mapped_data) + range.offset;
}

void rtdx_buffer_unmap(struct rtdx_context* ctx, struct rtdx_buffer* buffer) {
	(void)ctx;
	if (!buffer || !buffer->storage) {
		rtdx_throwf(RT_IMPROPER_USAGE, "buffer has no storage");
		return;
	}
	if (!rtdx_buffer_uses_host_storage(buffer->storage->memory_type)) {
		rtdx_throwf(RT_IMPROPER_USAGE, "only host-memory buffers can be unmapped");
		return;
	}
	buffer->storage->d3d_resource->Unmap(0, NULL);
}
