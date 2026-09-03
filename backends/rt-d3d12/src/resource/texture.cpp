#include "texture.hpp"
#include "context.hpp"
#include "error.hpp"
#include "resource/buffer.hpp"
#include "resource/command_buffer.hpp"
#include "resource/queue.hpp"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

static u32 rtd3d12_texture_view_bytes_per_pixel(DXGI_FORMAT format);
static DXGI_FORMAT rtd3d12_texture_format(rt::format format);
static void rtd3d12_texture_recycle_node(struct rt_texture_t* texture, struct rt_texture_t* node);
static bool rtd3d12_texture_view_read_direct(rtd3d12_context* ctx, rt_texture_view_t* view, rt::texture_range range, u08* data, usize data_size);

usize rtd3d12_texture_subresource_count(const rtd3d12_image_base* image) {
	return image ? image->mip_count * image->layer_count : 0;
}

D3D12_RESOURCE_STATES rtd3d12_texture_subresource_state(const rtd3d12_image_base* image, usize mip, usize layer) {
	if (!image || mip >= image->mip_count || layer >= image->layer_count || !image->states) {
		return image ? image->state : D3D12_RESOURCE_STATE_COMMON;
	}
	return image->states[layer * image->mip_count + mip];
}

void rtd3d12_texture_set_subresource_state(rtd3d12_image_base* image, usize mip, usize layer, D3D12_RESOURCE_STATES state) {
	if (!image || mip >= image->mip_count || layer >= image->layer_count) {
		return;
	}
	if (image->states) {
		image->states[layer * image->mip_count + mip] = state;
	}
	image->state = state;
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

rt_texture_t* rtTextureCreate(void) {
	rtd3d12_begin_errorable_operation();
	struct rt_texture_t* texture = rtd3d12::create_resource<rt_texture_t>(rtd3d12_get_current_context());
	return texture;
}

void rtTextureDestroy(rt_texture_t* texture) {
	if (texture) texture->retire();
}

void rtTextureResize(rt_texture_t* texture, rt::texture_type type, rt::format format, rt::extent_3d extent, usize mip_count) {
	rtd3d12_begin_errorable_operation();
	rtd3d12_texture_resize(rtd3d12_get_current_context(), texture, type, format, extent, mip_count);
}

rt_texture_view_t* rtTextureViewCreate(void) {
	rtd3d12_begin_errorable_operation();
	return rtd3d12::create_resource<rt_texture_view_t>(rtd3d12_get_current_context());
}

void rtTextureViewSetTexture(rt_texture_view_t* texture_view, rt_texture_t* texture) {
	rtd3d12_begin_errorable_operation();
	rtd3d12_texture_view_bind(
		rtd3d12_get_current_context(),
		texture_view,
		texture
	);
}

void rtTextureViewDestroy(rt_texture_view_t* texture_view) {
	if (texture_view) texture_view->retire();
}

void rtTextureViewSetFilter(rt_texture_view_t* texture_view, rt::filter mag_filter, rt::filter min_filter, rt::mip_filter mip_filter) {
	rtd3d12_begin_errorable_operation();
	rtd3d12_texture_view_filter(texture_view, mag_filter, min_filter, mip_filter);
}

void rtTextureViewSetAddress(rt_texture_view_t* texture_view, rt::address_mode address_u, rt::address_mode address_v, rt::address_mode address_w) {
	rtd3d12_begin_errorable_operation();
	rtd3d12_texture_view_address(texture_view, address_u, address_v, address_w);
}

void rtTextureViewSetAnisotropy(rt_texture_view_t* texture_view, usize max_anisotropy) {
	rtd3d12_begin_errorable_operation();
	rtd3d12_texture_view_anisotropy(texture_view, static_cast<u32>(max_anisotropy));
}

void rtTextureViewSetLod(rt_texture_view_t* texture_view, f32 min_lod, f32 max_lod, f32 lod_bias) {
	rtd3d12_begin_errorable_operation();
	rtd3d12_texture_view_lod(texture_view, min_lod, max_lod, lod_bias);
}

static D3D12_TEXTURE_ADDRESS_MODE rtd3d12_address_mode(rt::address_mode mode) {
	switch (mode) {
	case rt::address_mode::clamp:
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case rt::address_mode::mirror:
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	default:
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	}
}

static void rtd3d12_texture_view_normalize_sampler(struct rt_texture_view_t* view) {
	if (static_cast<i32>(view->mag_filter) == 0) {
		view->mag_filter = rt::filter::linear;
	}
	if (static_cast<i32>(view->min_filter) == 0) {
		view->min_filter = rt::filter::linear;
	}
	if (static_cast<i32>(view->mip_filter) == 0) {
		view->mip_filter = rt::mip_filter::none;
	}
	if (static_cast<i32>(view->address_u) == 0) {
		view->address_u = rt::address_mode::repeat;
	}
	if (static_cast<i32>(view->address_v) == 0) {
		view->address_v = rt::address_mode::repeat;
	}
	if (static_cast<i32>(view->address_w) == 0) {
		view->address_w = rt::address_mode::repeat;
	}
	if (!view->max_anisotropy) {
		view->max_anisotropy = 1;
	}
}

static D3D12_SAMPLER_DESC rtd3d12_sampler_desc(struct rt_texture_view_t* view) {
	rtd3d12_texture_view_normalize_sampler(view);

	bool min_linear = view->min_filter == rt::filter::linear;
	bool mag_linear = view->mag_filter == rt::filter::linear;
	bool mip_linear = view->mip_filter == rt::mip_filter::linear;
	D3D12_SAMPLER_DESC result = {};
	result.Filter = min_linear ? (mag_linear ? (mip_linear ? D3D12_FILTER_MIN_MAG_MIP_LINEAR : D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT) : D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT) : (mag_linear ? D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT : D3D12_FILTER_MIN_MAG_MIP_POINT);
	result.AddressU = rtd3d12_address_mode(view->address_u);
	result.AddressV = rtd3d12_address_mode(view->address_v);
	result.AddressW = rtd3d12_address_mode(view->address_w);
	result.MipLODBias = view->lod_bias;
	result.MaxAnisotropy = view->max_anisotropy;
	result.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	result.BorderColor[0] = 0.0f;
	result.BorderColor[1] = 0.0f;
	result.BorderColor[2] = 0.0f;
	result.BorderColor[3] = 0.0f;
	result.MinLOD = view->min_lod;
	result.MaxLOD = view->max_lod;
	return result;
}

void rtCmdTextureCopy(rt_command_buffer_t* command_buffer, rt_texture_t* src_texture, rt::texture_range src_range, rt_texture_t* dst_texture, rt::texture_range dst_range) {
	rtd3d12_begin_errorable_operation();
	rtd3d12_command_buffer_texture_copy(command_buffer, src_texture, src_range, dst_texture, dst_range);
}

void rtCmdTextureData(rt_command_buffer_t* command_buffer, rt_texture_t* texture, rt::texture_range range, const u08* data) {
	rtd3d12_begin_errorable_operation();
	rtd3d12_command_buffer_texture_data(command_buffer, texture, range, data);
}

void rtCmdTextureCopyToBuffer(rt_command_buffer_t* command_buffer, rt_texture_t* src, rt::texture_range src_range, rt_buffer_t* dst, rt::buffer_range dst_range) {
	rtd3d12_begin_errorable_operation();
	rtd3d12_command_buffer_texture_copy_to_buffer(command_buffer, src, src_range, dst, dst_range);
}

void rtCmdTextureBarrier(rt_command_buffer_t* command_buffer, rt_texture_t* texture, rt::texture_range range, rt::access src, rt::access dst) {
	rtd3d12_begin_errorable_operation();
	rtd3d12_command_buffer_texture_barrier(command_buffer, texture, range, src, dst);
}

rt::extent_3d rtTextureViewExtent(rt_texture_view_t* texture_view) {
	rtd3d12_begin_errorable_operation();
	rt::extent_3d extent = { 0, 0, 0 };
	struct rt_texture_view_t* view = texture_view;
	rtd3d12_texture_view_refresh(rtd3d12_get_current_context(), view);
	if (!view || !view->image || !view->image->d3d_resource) {
		rtd3d12_fail(rt::error::improper_usage, "texture view extent query source is invalid");
		return extent;
	}
	extent.width = view->image->width;
	extent.height = view->image->height;
	extent.depth = view->image->depth;
	return extent;
}

void rtTextureViewRead(rt_texture_view_t* texture_view, rt::texture_range range, u08* data, usize data_size) {
	rtd3d12_begin_errorable_operation();
	rtd3d12_context* ctx = rtd3d12_get_current_context();
	rt_texture_view_t* view = texture_view;
	if (!rtd3d12_texture_view_refresh(ctx, view) || !rtd3d12_texture_view_read_direct(ctx, view, range, data, data_size)) {
		return;
	}
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

static u32 rtd3d12_texture_view_bytes_per_pixel(DXGI_FORMAT format) {
	switch (format) {
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		return 4;
	case DXGI_FORMAT_D16_UNORM:
		return 2;
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		return 8;
	default:
		return 0;
	}
}

static bool rtd3d12_texture_upload_staging(struct rtd3d12_context* ctx, struct rt_queue_t* queue, u64 size);

static DXGI_FORMAT rtd3d12_texture_format(rt::format format) {
	switch (format) {
	case rt::format::rgba8_unorm:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case rt::format::d16_unorm:
		return DXGI_FORMAT_D16_UNORM;
	case rt::format::d32_sfloat:
		return DXGI_FORMAT_D32_FLOAT;
	case rt::format::s8_uint:
		return DXGI_FORMAT_UNKNOWN;
	case rt::format::d24_unorm_s8_uint:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case rt::format::d32_sfloat_s8_uint:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

static u32 rtd3d12_texture_format_bytes_per_pixel(rt::format format) {
	switch (format) {
	case rt::format::rgba8_unorm:
		return 4;
	case rt::format::d16_unorm:
		return 2;
	case rt::format::d32_sfloat:
		return 4;
	case rt::format::s8_uint:
		return 1;
	case rt::format::d24_unorm_s8_uint:
		return 4;
	case rt::format::d32_sfloat_s8_uint:
		return 8;
	default:
		return 0;
	}
}

static struct rt_queue_t* rtd3d12_texture_upload_queue(struct rtd3d12_context* ctx) {
	struct rt_queue_t* queue = rtd3d12_context_queue(ctx, rt::queue_capability::transfer);
	if (queue) {
		return queue;
	}
	return rtd3d12_context_queue(ctx, rt::queue_capability::graphics);
}

static bool rtd3d12_texture_read_range_valid(const rtd3d12_image_base* image, rt::texture_range range) {
	if (!image || !range.mip_count || !range.layer_count || !range.extent.width || !range.extent.height || !range.extent.depth || range.base_mip >= image->mip_count || range.mip_count > image->mip_count - range.base_mip) {
		return false;
	}
	const rt::texture_aspect available = rtd3d12_texture_format_is_depth(image->dxgi_format)
													  ? (image->dxgi_format == DXGI_FORMAT_D24_UNORM_S8_UINT || image->dxgi_format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT
															 ? (rt::texture_aspect::depth | rt::texture_aspect::stencil)
															 : rt::texture_aspect::depth)
													  : rt::texture_aspect::color;
	if (!rt::any(range.aspects) || rt::any(range.aspects & ~available)) {
		return false;
	}
	const usize layers = (image->type == rt::texture_type::texture_1d_array || image->type == rt::texture_type::texture_2d_array) ? image->layer_count : 1;
	if (range.base_layer >= layers || range.layer_count > layers - range.base_layer) {
		return false;
	}
	if ((image->type == rt::texture_type::texture_1d || image->type == rt::texture_type::texture_1d_array) && (range.offset.height || range.extent.height != 1 || range.offset.depth || range.extent.depth != 1)) {
		return false;
	}
	if (image->type != rt::texture_type::texture_3d && (range.offset.depth || range.extent.depth != 1)) {
		return false;
	}
	for (usize mip = 0; mip < range.mip_count; ++mip) {
		usize level = range.base_mip + mip;
		usize width = image->width >> level ? image->width >> level : 1;
		usize height = image->type == rt::texture_type::texture_1d || image->type == rt::texture_type::texture_1d_array ? 1 : (image->height >> level ? image->height >> level : 1);
		usize depth = image->type == rt::texture_type::texture_3d ? (image->depth >> level ? image->depth >> level : 1) : 1;
		if (range.offset.width > width || range.extent.width > width - range.offset.width || range.offset.height > height || range.extent.height > height - range.offset.height || range.offset.depth > depth || range.extent.depth > depth - range.offset.depth) {
			return false;
		}
	}
	return true;
}

static bool rtd3d12_texture_read_aspects_supported(const rtd3d12_image_base* image, rt::texture_range range) {
	if (!image || (image->dxgi_format != DXGI_FORMAT_D24_UNORM_S8_UINT && image->dxgi_format != DXGI_FORMAT_D32_FLOAT_S8X24_UINT)) {
		return true;
	}
	return range.aspects == (rt::texture_aspect::depth | rt::texture_aspect::stencil);
}

static bool rtd3d12_texture_view_read_direct(rtd3d12_context* ctx, rt_texture_view_t* view, rt::texture_range range, u08* data, usize data_size) {
	if (!ctx || !view || !view->image || !view->image->d3d_resource || !data || !rtd3d12_texture_read_range_valid(view->image, range) || !rtd3d12_texture_read_aspects_supported(view->image, range)) {
		rtd3d12_fail(rt::error::improper_usage, "texture view read range is invalid");
		return false;
	}
	const u32 bpp = rtd3d12_texture_view_bytes_per_pixel(view->image->dxgi_format);
	const usize packed_region = range.extent.width * range.extent.height * range.extent.depth * bpp;
	const usize region_count = range.mip_count * range.layer_count;
	if (!bpp || !packed_region || data_size < packed_region * region_count) {
		rtd3d12_fail(rt::error::improper_usage, "texture view read destination is too small or format is unsupported");
		return false;
	}
	struct rtd3d12_read_region {
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
		UINT subresource;
		D3D12_RESOURCE_STATES state;
	};
	std::vector<rtd3d12_read_region> regions;
	D3D12_RESOURCE_DESC desc = view->image->d3d_resource->GetDesc();
	u64 offset = 0;
	for (usize mip = 0; mip < range.mip_count; ++mip)
		for (usize layer = 0; layer < range.layer_count; ++layer) {
			const UINT subresource = static_cast<UINT>((range.base_layer + layer) * view->image->mip_count + range.base_mip + mip);
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
			u32 rows = 0;
			u64 row_size = 0;
			u64 size = 0;
			ctx->d3d_device->GetCopyableFootprints(&desc, subresource, 1, offset, &footprint, &rows, &row_size, &size);
			footprint.Footprint.Width = static_cast<UINT>(range.extent.width);
			footprint.Footprint.Height = static_cast<UINT>(range.extent.height);
			footprint.Footprint.Depth = static_cast<UINT>(range.extent.depth);
			regions.push_back({ footprint, subresource, rtd3d12_texture_subresource_state(view->image, range.base_mip + mip, range.base_layer + layer) });
			offset = footprint.Offset + size;
		}
	D3D12_HEAP_PROPERTIES heap = {};
	heap.Type = D3D12_HEAP_TYPE_READBACK;
	D3D12_RESOURCE_DESC buffer = {};
	buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	buffer.Width = offset;
	buffer.Height = 1;
	buffer.DepthOrArraySize = 1;
	buffer.MipLevels = 1;
	buffer.SampleDesc.Count = 1;
	buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ID3D12Resource* readback = nullptr;
	HRESULT result = ctx->d3d_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback));
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommittedResource(texture readback) failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}
	ID3D12CommandAllocator* allocator = nullptr;
	result = ctx->d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
	if (FAILED(result)) {
		if (readback) {
			readback->Release();
			readback = nullptr;
		}
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommandAllocator(texture readback) failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}
	ID3D12GraphicsCommandList* list = nullptr;
	result = ctx->d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, nullptr, IID_PPV_ARGS(&list));
	if (FAILED(result)) {
		if (allocator) {
			allocator->Release();
			allocator = nullptr;
		}
		if (readback) {
			readback->Release();
			readback = nullptr;
		}
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommandList(texture readback) failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}
	for (const rtd3d12_read_region& region : regions)
		if (region.state != D3D12_RESOURCE_STATE_COPY_SOURCE) {
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = view->image->d3d_resource;
			barrier.Transition.Subresource = region.subresource;
			barrier.Transition.StateBefore = region.state;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
			list->ResourceBarrier(1, &barrier);
		}
	for (const rtd3d12_read_region& region : regions) {
		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = view->image->d3d_resource;
		src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.SubresourceIndex = region.subresource;
		D3D12_TEXTURE_COPY_LOCATION dst = {};
		dst.pResource = readback;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		dst.PlacedFootprint = region.footprint;
		D3D12_BOX box = { static_cast<UINT>(range.offset.width), static_cast<UINT>(range.offset.height), static_cast<UINT>(range.offset.depth), static_cast<UINT>(range.offset.width + range.extent.width), static_cast<UINT>(range.offset.height + range.extent.height), static_cast<UINT>(range.offset.depth + range.extent.depth) };
		list->CopyTextureRegion(&dst, 0, 0, 0, &src, &box);
	}
	for (const rtd3d12_read_region& region : regions)
		if (region.state != D3D12_RESOURCE_STATE_COPY_SOURCE) {
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = view->image->d3d_resource;
			barrier.Transition.Subresource = region.subresource;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
			barrier.Transition.StateAfter = region.state;
			list->ResourceBarrier(1, &barrier);
		}
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
		if (readback) {
			readback->Release();
			readback = nullptr;
		}
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "Close(texture readback) failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}
	rt_queue_t* queue = rtd3d12_texture_upload_queue(ctx);
	if (!queue) {
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
		rtd3d12_fail(rt::error::improper_usage, "texture read requires a queue");
		return false;
	}
	rtd3d12_wait_for_timepoint(ctx, rtd3d12_queue_timepoint(queue, queue->fence_value));
	{
		std::lock_guard<std::mutex> queue_lock(ctx->queue_lock);
		ID3D12CommandList* lists[] = { list };
		queue->d3d_queue->ExecuteCommandLists(1, lists);
		u64 fence = ++queue->fence_value;
		result = queue->d3d_queue->Signal(queue->d3d_fence, fence);
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
			rtd3d12_fail(rtd3d12_error_from_hresult(result), "Signal(texture readback) failed: 0x{:08x}", static_cast<u32>(result));
			return false;
		}
		queue->fence_value = fence;
		rtd3d12_wait_for_timepoint(ctx, rtd3d12_queue_timepoint(queue, fence));
	}
	D3D12_RANGE read = { 0, static_cast<SIZE_T>(offset) };
	void* mapped = nullptr;
	result = readback->Map(0, &read, &mapped);
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
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "Map(texture readback) failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}
	for (usize index = 0; index < regions.size(); ++index)
		for (usize z = 0; z < range.extent.depth; ++z)
			for (usize y = 0; y < range.extent.height; ++y)
				memcpy(data + index * packed_region + (z * range.extent.height + y) * range.extent.width * bpp, static_cast<const u08*>(mapped) + regions[index].footprint.Offset + (z * range.extent.height + y) * regions[index].footprint.Footprint.RowPitch, range.extent.width * bpp);
	D3D12_RANGE write = { 0, 0 };
	readback->Unmap(0, &write);
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
	return true;
}

bool rtd3d12_texture_format_is_depth(DXGI_FORMAT format) {
	return format == DXGI_FORMAT_D16_UNORM || format == DXGI_FORMAT_D32_FLOAT ||
		   format == DXGI_FORMAT_D24_UNORM_S8_UINT || format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
}

static bool rtd3d12_texture_view_needs_bgra_swizzle(DXGI_FORMAT format) {
	return format == DXGI_FORMAT_B8G8R8A8_UNORM || format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
}

static bool rtd3d12_texture_copy_region(
	struct rtd3d12_context* ctx,
	struct rt_queue_t* queue,
	struct rt_texture_t* src_node,
	u32 src_x,
	u32 src_y,
	u32 src_z,
	struct rt_texture_t* dst_node,
	u32 dst_x,
	u32 dst_y,
	u32 dst_z,
	u32 width,
	u32 height,
	u32 depth
) {
	(void)src_z;
	(void)dst_z;
	if (!src_node || !src_node->d3d_resource || !dst_node || !dst_node->d3d_resource) {
		rtd3d12_fail(rt::error::improper_usage, "texture copy source or destination is invalid");
		return false;
	}
	if (src_node->dxgi_format != dst_node->dxgi_format) {
		rtd3d12_fail(rt::error::unsupported_feature, "texture copy requires matching texture formats");
		return false;
	}
	if (src_node->type != rt::texture_type::texture_2d || dst_node->type != rt::texture_type::texture_2d || depth != 1) {
		rtd3d12_fail(rt::error::unsupported_feature, "texture copy currently supports only 2D single-layer textures");
		return false;
	}
	if (src_x > src_node->width || src_y > src_node->height ||
		dst_x > dst_node->width || dst_y > dst_node->height ||
		width == 0 || height == 0 ||
		width > src_node->width - src_x || height > src_node->height - src_y ||
		width > dst_node->width - dst_x || height > dst_node->height - dst_y) {
		rtd3d12_fail(rt::error::improper_usage, "texture copy region is out of bounds");
		return false;
	}

	std::lock_guard<std::mutex> upload_lock(queue->upload_lock);
	rtd3d12_wait_for_timepoint(ctx, rtd3d12_queue_timepoint(queue, queue->upload_fence_value));
	rtd3d12_queue_collect(ctx, queue);
	std::lock_guard<std::mutex> queue_lock(ctx->queue_lock);
	if (!rtd3d12_queue_acquire_upload_command(ctx, queue)) {
		return false;
	}
	ID3D12GraphicsCommandList* command_list = queue->upload_command_list;
	D3D12_RESOURCE_STATES src_original_state = src_node->state;
	D3D12_RESOURCE_STATES dst_original_state = dst_node->state;

	if (src_original_state != D3D12_RESOURCE_STATE_COPY_SOURCE) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = src_node->d3d_resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = src_original_state;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		command_list->ResourceBarrier(1, &barrier);
	}
	if (dst_original_state != D3D12_RESOURCE_STATE_COPY_DEST) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = dst_node->d3d_resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = dst_original_state;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		command_list->ResourceBarrier(1, &barrier);
	}

	D3D12_TEXTURE_COPY_LOCATION src_location = {};
	src_location.pResource = src_node->d3d_resource;
	src_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	src_location.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION dst_location = {};
	dst_location.pResource = dst_node->d3d_resource;
	dst_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst_location.SubresourceIndex = 0;
	command_list->CopyTextureRegion(&dst_location, dst_x, dst_y, 0, &src_location, nullptr);

	if (dst_original_state != D3D12_RESOURCE_STATE_COPY_DEST) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = dst_node->d3d_resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = dst_original_state;
		command_list->ResourceBarrier(1, &barrier);
	}
	if (src_original_state != D3D12_RESOURCE_STATE_COPY_SOURCE) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = src_node->d3d_resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
		barrier.Transition.StateAfter = src_original_state;
		command_list->ResourceBarrier(1, &barrier);
	}

	HRESULT result = command_list->Close();
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12GraphicsCommandList::Close failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}

	ID3D12CommandList* lists[] = { command_list };
	queue->d3d_queue->ExecuteCommandLists(1, lists);
	u64 fence_value = ++queue->fence_value;
	result = queue->d3d_queue->Signal(queue->d3d_fence, fence_value);
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12CommandQueue::Signal failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}

	queue->fence_value = fence_value;
	queue->upload_fence_value = fence_value;
	return true;
}

void rtd3d12_texture_init(struct rtd3d12_context* ctx, struct rt_texture_t* texture) {
}

void rtd3d12_texture_view_init(struct rtd3d12_context* ctx, struct rt_texture_view_t* view) {
	rtd3d12_texture_view_normalize_sampler(view);
}

rt_texture_t::~rt_texture_t() {
	if (active) active->release();
	active = nullptr;

	struct rt_texture_t* node = next;
	while (node) {
		struct rt_texture_t* next = node->next;
		node->next = nullptr;
		node->release();
		node = next;
	}
	next = nullptr;
	if (d3d_resource) {
		d3d_resource->Release();
		d3d_resource = nullptr;
	}
	delete[] states;
	states = nullptr;
}

rt_texture_view_t::~rt_texture_view_t() {
	if (d3d_sampler_heap) {
		d3d_sampler_heap->Release();
		d3d_sampler_heap = nullptr;
	}
	if (d3d_srv_heap) {
		d3d_srv_heap->Release();
		d3d_srv_heap = nullptr;
	}
	if (d3d_rtv_heap) {
		d3d_rtv_heap->Release();
		d3d_rtv_heap = nullptr;
	}
	if (d3d_dsv_heap) {
		d3d_dsv_heap->Release();
		d3d_dsv_heap = nullptr;
	}
	if (image) {
		image->release();
		image = nullptr;
	}
	rtv.ptr = 0;
	dsv.ptr = 0;
	sampler_cpu.ptr = 0;
	sampler_gpu.ptr = 0;
	srv_cpu.ptr = 0;
	srv_gpu.ptr = 0;
}

static struct rt_texture_t* rtd3d12_texture_node_create(struct rtd3d12_context* ctx) {
	struct rt_texture_t* node = rtd3d12::create_resource<rt_texture_t>(ctx);
	if (!node) {
		return nullptr;
	}
	node->zombie.store(true, std::memory_order_relaxed);
	node->state = D3D12_RESOURCE_STATE_COMMON;
	return node;
}

static bool rtd3d12_texture_desc(rt::texture_type type, DXGI_FORMAT format, rt::extent_3d extent, usize mip_count, D3D12_RESOURCE_DESC* out_desc, usize* out_layers) {
	if (!out_desc || !extent.width || !extent.height || !extent.depth || !mip_count || format == DXGI_FORMAT_UNKNOWN) {
		return false;
	}
	D3D12_RESOURCE_DESC desc = {};
	desc.Width = extent.width;
	desc.Height = static_cast<UINT>(extent.height);
	desc.MipLevels = static_cast<UINT16>(mip_count);
	switch (format) {
	case DXGI_FORMAT_D16_UNORM:
		desc.Format = DXGI_FORMAT_R16_TYPELESS;
		break;
	case DXGI_FORMAT_D32_FLOAT:
		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		break;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		break;
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		desc.Format = DXGI_FORMAT_R32G8X24_TYPELESS;
		break;
	default:
		desc.Format = format;
		break;
	}
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	usize layers = 1;
	switch (type) {
	case rt::texture_type::texture_1d:
		if (extent.height != 1 || extent.depth != 1) {
			return false;
		}
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		break;
	case rt::texture_type::texture_2d:
		if (extent.depth != 1) {
			return false;
		}
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.DepthOrArraySize = 1;
		break;
	case rt::texture_type::texture_3d:
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		desc.DepthOrArraySize = static_cast<UINT16>(extent.depth);
		break;
	case rt::texture_type::texture_1d_array:
		if (extent.height != 1) {
			return false;
		}
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		desc.Height = 1;
		desc.DepthOrArraySize = static_cast<UINT16>(extent.depth);
		layers = extent.depth;
		break;
	case rt::texture_type::texture_2d_array:
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.DepthOrArraySize = static_cast<UINT16>(extent.depth);
		layers = extent.depth;
		break;
	default:
		return false;
	}
	desc.Flags = rtd3d12_texture_format_is_depth(format) ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	*out_desc = desc;
	*out_layers = layers;
	return true;
}

void rtd3d12_texture_resize(rtd3d12_context* ctx, rt_texture_t* texture, rt::texture_type type, rt::format format, rt::extent_3d extent, usize mip_count) {
	if (!ctx || !texture) {
		rtd3d12_fail(rt::error::improper_usage, "texture resize target is invalid");
		return;
	}
	DXGI_FORMAT dxgi_format = rtd3d12_texture_format(format);
	D3D12_RESOURCE_DESC desc = {};
	usize layers = 0;
	if (!rtd3d12_texture_desc(type, dxgi_format, extent, mip_count, &desc, &layers)) {
		rtd3d12_fail(rt::error::improper_usage, "texture resize description is invalid or unsupported");
		return;
	}
	rt_texture_t* node = rtd3d12_texture_node_create(ctx);
	if (!node) {
		return;
	}
	D3D12_HEAP_PROPERTIES heap = {};
	heap.Type = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_CLEAR_VALUE clear_value = {};
	clear_value.Format = dxgi_format;
	clear_value.DepthStencil.Depth = 1.0f;
	HRESULT result = ctx->d3d_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, rtd3d12_texture_format_is_depth(dxgi_format) ? &clear_value : nullptr, IID_PPV_ARGS(&node->d3d_resource));
	if (FAILED(result)) {
		(node)->release();
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommittedResource(texture) failed: 0x{:08x}", static_cast<u32>(result));
		return;
	}
	node->width = extent.width;
	node->height = extent.height;
	node->depth = extent.depth;
	node->mip_count = mip_count;
	node->layer_count = layers;
	node->dxgi_format = dxgi_format;
	node->type = type;
	node->state = D3D12_RESOURCE_STATE_COMMON;
	node->states = rtd3d12::allocate_array<D3D12_RESOURCE_STATES>(mip_count * layers);
	if (!node->states) {
		(node)->release();
		return;
	}
	for (usize i = 0; i < mip_count * layers; ++i) {
		node->states[i] = D3D12_RESOURCE_STATE_COMMON;
	}
	rtd3d12_texture_recycle_node(texture, texture->active);
	texture->active = node;
	return;
}

static bool rtd3d12_texture_view_rebuild_descriptors(struct rtd3d12_context* ctx, struct rt_texture_view_t* view) {
	if (!view || !view->image || !view->image->d3d_resource || view->image->dxgi_format == DXGI_FORMAT_UNKNOWN) {
		rtd3d12_fail(rt::error::improper_usage, "texture view is invalid");
		return false;
	}

	if (view->d3d_rtv_heap) {
		view->d3d_rtv_heap->Release();
		view->d3d_rtv_heap = nullptr;
	}
	if (view->d3d_dsv_heap) {
		view->d3d_dsv_heap->Release();
		view->d3d_dsv_heap = nullptr;
	}
	if (view->d3d_srv_heap) {
		view->d3d_srv_heap->Release();
		view->d3d_srv_heap = nullptr;
	}
	view->rtv.ptr = 0;
	view->dsv.ptr = 0;
	view->srv_cpu.ptr = 0;
	view->srv_gpu.ptr = 0;
	HRESULT result = S_OK;

	const bool depth_format = rtd3d12_texture_format_is_depth(view->image->dxgi_format);
	if (depth_format) {
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		heap_desc.NumDescriptors = 1;
		result = ctx->d3d_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&view->d3d_dsv_heap));
		if (FAILED(result)) {
			rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateDescriptorHeap(DSV) failed: 0x{:08x}", static_cast<u32>(result));
			return false;
		}
		view->dsv = view->d3d_dsv_heap->GetCPUDescriptorHandleForHeapStart();
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
		dsv_desc.Format = view->image->dxgi_format;
		switch (view->image->type) {
		case rt::texture_type::texture_1d:
			dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
			break;
		case rt::texture_type::texture_1d_array:
			dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
			dsv_desc.Texture1DArray.ArraySize = static_cast<UINT>(view->image->layer_count);
			break;
		case rt::texture_type::texture_2d_array:
			dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			dsv_desc.Texture2DArray.ArraySize = static_cast<UINT>(view->image->layer_count);
			break;
		case rt::texture_type::texture_2d:
			dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			break;
		default:
			rtd3d12_fail(rt::error::unsupported_feature, "D3D12 depth views do not support this texture type");
			return false;
		}
		ctx->d3d_device->CreateDepthStencilView(view->image->d3d_resource, &dsv_desc, view->dsv);
	}

	D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
	srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srv_heap_desc.NumDescriptors = 1;
	srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = ctx->d3d_device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&view->d3d_srv_heap));
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateDescriptorHeap(SRV) failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}
	view->srv_cpu = view->d3d_srv_heap->GetCPUDescriptorHandleForHeapStart();
	view->srv_gpu = view->d3d_srv_heap->GetGPUDescriptorHandleForHeapStart();
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = view->image->dxgi_format;
	if (depth_format) {
		switch (view->image->dxgi_format) {
		case DXGI_FORMAT_D16_UNORM:
			srv_desc.Format = DXGI_FORMAT_R16_UNORM;
			break;
		case DXGI_FORMAT_D32_FLOAT:
			srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
			break;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			break;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			srv_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
			break;
		default:
			break;
		}
	}
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	switch (view->image->type) {
	case rt::texture_type::texture_1d:
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		srv_desc.Texture1D.MipLevels = static_cast<UINT>(view->image->mip_count);
		break;
	case rt::texture_type::texture_1d_array:
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
		srv_desc.Texture1DArray.ArraySize = static_cast<UINT>(view->image->layer_count);
		srv_desc.Texture1DArray.MipLevels = static_cast<UINT>(view->image->mip_count);
		break;
	case rt::texture_type::texture_2d:
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = static_cast<UINT>(view->image->mip_count);
		break;
	case rt::texture_type::texture_2d_array:
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srv_desc.Texture2DArray.ArraySize = static_cast<UINT>(view->image->layer_count);
		srv_desc.Texture2DArray.MipLevels = static_cast<UINT>(view->image->mip_count);
		break;
	case rt::texture_type::texture_3d:
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		srv_desc.Texture3D.MipLevels = static_cast<UINT>(view->image->mip_count);
		break;
	default:
		rtd3d12_fail(rt::error::unsupported_feature, "D3D12 texture view type is unsupported");
		return false;
	}
	ctx->d3d_device->CreateShaderResourceView(view->image->d3d_resource, &srv_desc, view->srv_cpu);
	if (depth_format) {
		return true;
	}

	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heap_desc.NumDescriptors = 1;
	result = ctx->d3d_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&view->d3d_rtv_heap));
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateDescriptorHeap(RTV) failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}
	view->rtv = view->d3d_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	rtv_desc.Format = view->image->dxgi_format;
	switch (view->image->type) {
	case rt::texture_type::texture_1d:
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
		break;
	case rt::texture_type::texture_1d_array:
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
		rtv_desc.Texture1DArray.ArraySize = static_cast<UINT>(view->image->layer_count);
		break;
	case rt::texture_type::texture_2d:
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		break;
	case rt::texture_type::texture_2d_array:
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtv_desc.Texture2DArray.ArraySize = static_cast<UINT>(view->image->layer_count);
		break;
	case rt::texture_type::texture_3d:
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		rtv_desc.Texture3D.WSize = static_cast<UINT>(view->image->depth);
		break;
	default:
		rtd3d12_fail(rt::error::unsupported_feature, "D3D12 render-target view type is unsupported");
		return false;
	}
	ctx->d3d_device->CreateRenderTargetView(view->image->d3d_resource, &rtv_desc, view->rtv);
	return true;
}

static void rtd3d12_texture_recycle_node(struct rt_texture_t* texture, struct rt_texture_t* node) {
	if (!node) {
		return;
	}
	node->next = texture->next;
	texture->next = node;
}

static struct rt_texture_t* rtd3d12_texture_take_reusable_node(struct rt_texture_t* texture, const D3D12_RESOURCE_DESC& desc) {
	if (!texture) {
		return nullptr;
	}
	struct rt_texture_t** link = &texture->next;
	while (*link) {
		struct rt_texture_t* node = *link;
		D3D12_RESOURCE_DESC candidate = node->d3d_resource ? node->d3d_resource->GetDesc() : D3D12_RESOURCE_DESC{};
		if (node->ref_count.load(std::memory_order_relaxed) == 1 && candidate.Dimension == desc.Dimension && candidate.Width == desc.Width && candidate.Height == desc.Height && candidate.DepthOrArraySize == desc.DepthOrArraySize && candidate.MipLevels == desc.MipLevels && candidate.Format == desc.Format && candidate.Flags == desc.Flags) {
			*link = node->next;
			node->next = nullptr;
			return node;
		}
		link = &node->next;
	}
	return nullptr;
}

rtd3d12_texture_write rtd3d12_texture_write_begin(struct rtd3d12_context* ctx, struct rt_texture_t* texture) {
	rtd3d12_texture_write write = {};
	if (!texture || !texture->active || !texture->active->d3d_resource) {
		return write;
	}
	write.target = texture->active;
	/* DXGI owns swapchain back buffers. Acquiring a frame supplies the queue
	 * dependency that makes its one physical image writable again; it must never
	 * be copy-on-written into an unrelated private render target. */
	if (texture->active->swapchain_image) {
		return write;
	}
	if (texture->active->ref_count.load(std::memory_order_relaxed) == 1) {
		return write;
	}
	struct rt_texture_t* source = texture->active;
	D3D12_RESOURCE_DESC desc = source->d3d_resource->GetDesc();
	struct rt_texture_t* target = rtd3d12_texture_take_reusable_node(texture, desc);
	if (!target) {
		target = rtd3d12_texture_node_create(ctx);
	}
	if (!target) {
		return {};
	}
	const bool reused = target->d3d_resource != nullptr;
	if (!reused) {
		D3D12_HEAP_PROPERTIES heap = {};
		heap.Type = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_CLEAR_VALUE clear_value = {};
		clear_value.Format = source->dxgi_format;
		clear_value.DepthStencil.Depth = 1.0f;
		const bool depth = rtd3d12_texture_format_is_depth(source->dxgi_format);
		HRESULT result = ctx->d3d_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, depth ? &clear_value : nullptr, IID_PPV_ARGS(&target->d3d_resource));
		if (FAILED(result)) {
			if (target) (target)->release();
			rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommittedResource(texture revision) failed: 0x{:08x}", static_cast<u32>(result));
			return {};
		}
	}
	target->width = source->width;
	target->height = source->height;
	target->depth = source->depth;
	target->mip_count = source->mip_count;
	target->layer_count = source->layer_count;
	target->dxgi_format = source->dxgi_format;
	target->type = source->type;
	if (!reused) {
		target->state = D3D12_RESOURCE_STATE_COPY_DEST;
	}
	const usize state_count = rtd3d12_texture_subresource_count(source);
	if (state_count && !reused) {
		target->states = rtd3d12::allocate_array<D3D12_RESOURCE_STATES>(state_count);
		if (!target->states) {
			(target)->release();
			return {};
		}
		/* The resource was created in COPY_DEST. Its tracked state must describe
		 * that real state; the complete revision copy is lowered before mutation. */
		for (usize i = 0; i < state_count; ++i) {
			target->states[i] = D3D12_RESOURCE_STATE_COPY_DEST;
		}
	}
	rtd3d12_texture_recycle_node(texture, source);
	texture->active = target;
	write.source = source;
	write.target = target;
	return write;
}

static void rtd3d12_texture_collect_nodes(struct rt_texture_t* texture) {
	struct rt_texture_t** link = &texture->next;
	while (*link) {
		struct rt_texture_t* node = *link;
		if (node->ref_count.load(std::memory_order_relaxed) == 1) {
			*link = node->next;
			node->next = nullptr;
			if (node) (node)->release();
			continue;
		}
		link = &node->next;
	}
}

struct rt_texture_t* rtd3d12_texture_create_for_swapchain_image(struct rtd3d12_context* ctx, ID3D12Resource* image, DXGI_FORMAT format, u32 width, u32 height) {
	struct rt_texture_t* texture = rtd3d12::create_resource<rt_texture_t>(ctx);
	if (!texture) {
		return nullptr;
	}

	struct rt_texture_t* node = rtd3d12_texture_node_create(ctx);
	if (!node) {
		if (texture) texture->retire();
		return nullptr;
	}

	node->d3d_resource = image;
	node->d3d_resource->AddRef();
	node->dxgi_format = format;
	node->state = D3D12_RESOURCE_STATE_PRESENT;
	node->swapchain_image = true;
	node->type = rt::texture_type::texture_2d;
	node->width = width;
	node->height = height;
	node->depth = 1;
	node->mip_count = 1;
	node->layer_count = 1;
	node->states = rtd3d12::allocate_array<D3D12_RESOURCE_STATES>(1);
	if (!node->states) {
		if (texture) texture->retire();
		return nullptr;
	}
	node->states[0] = D3D12_RESOURCE_STATE_PRESENT;
	texture->active = node;
	return texture;
}

struct rt_texture_view_t* rtd3d12_texture_view_create_for_texture(struct rtd3d12_context* ctx, struct rt_texture_t* texture, D3D12_CPU_DESCRIPTOR_HANDLE rtv) {
	struct rt_texture_t* node = texture ? texture->active : nullptr;
	if (!node || !node->d3d_resource) {
		rtd3d12_fail(rt::error::improper_usage, "texture view source texture is invalid");
		return nullptr;
	}

	struct rt_texture_view_t* view = rtd3d12::create_resource<rt_texture_view_t>(ctx);
	if (!view) {
		return nullptr;
	}

	(node)->retain();
	view->image = node;
	view->rtv = rtv;
	if (!rtd3d12_texture_view_rebuild_descriptors(ctx, view)) {
		if (view) view->retire();
		return nullptr;
	}
	return view;
}

void rtd3d12_texture_view_bind(struct rtd3d12_context* ctx, struct rt_texture_view_t* view, struct rt_texture_t* texture) {
	struct rt_texture_t* node = texture ? texture->active : nullptr;
	if (!view || !node || !node->d3d_resource) {
		rtd3d12_fail(rt::error::improper_usage, "texture view bind source texture is invalid");
		return;
	}
	if (view->image == node) {
		return;
	}
	if (view->image) {
		(view->image)->release();
		view->image = nullptr;
	}
	if (view->d3d_rtv_heap) {
		view->d3d_rtv_heap->Release();
		view->d3d_rtv_heap = nullptr;
	}
	if (view->d3d_dsv_heap) {
		view->d3d_dsv_heap->Release();
		view->d3d_dsv_heap = nullptr;
	}
	if (view->d3d_srv_heap) {
		view->d3d_srv_heap->Release();
		view->d3d_srv_heap = nullptr;
	}
	if (view->d3d_sampler_heap) {
		view->d3d_sampler_heap->Release();
		view->d3d_sampler_heap = nullptr;
	}
	(node)->retain();
	view->image = node;
	if (!rtd3d12_texture_view_rebuild_descriptors(ctx, view)) {
		(view->image)->release();
		view->image = nullptr;
		view->image = nullptr;
		return;
	}
}

bool rtd3d12_texture_view_refresh(rtd3d12_context* ctx, rt_texture_view_t* view) {
	return view && view->image && view->image->d3d_resource && rtd3d12_texture_view_rebuild_descriptors(ctx, view);
}

struct rt_texture_view_t* rtd3d12_texture_view_create_for_swapchain(struct rtd3d12_context* ctx, struct rt_texture_t* texture, D3D12_CPU_DESCRIPTOR_HANDLE rtv) {
	return rtd3d12_texture_view_create_for_texture(ctx, texture, rtv);
}

static bool rtd3d12_texture_view_sampler_valid(struct rt_texture_view_t* texture_view) {
	if (!texture_view) {
		rtd3d12_fail(rt::error::improper_usage, "texture view is nullptr");
		return false;
	}
	return true;
}

static bool rtd3d12_texture_view_prepare_sampler_heap(struct rtd3d12_context* ctx, struct rt_texture_view_t* texture_view) {
	if (!rtd3d12_texture_view_sampler_valid(texture_view)) {
		return false;
	}
	if (texture_view->d3d_sampler_heap) {
		return true;
	}

	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	heap_desc.NumDescriptors = 1;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT result = ctx->d3d_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&texture_view->d3d_sampler_heap));
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateDescriptorHeap(sampler) failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}

	texture_view->sampler_cpu = texture_view->d3d_sampler_heap->GetCPUDescriptorHandleForHeapStart();
	texture_view->sampler_gpu = texture_view->d3d_sampler_heap->GetGPUDescriptorHandleForHeapStart();
	return true;
}

bool rtd3d12_texture_view_prepare_sampler(struct rtd3d12_context* ctx, struct rt_texture_view_t* texture_view) {
	bool has_sampler = texture_view && texture_view->d3d_sampler_heap;
	if (!rtd3d12_texture_view_prepare_sampler_heap(ctx, texture_view)) {
		return false;
	}
	if (has_sampler) {
		return true;
	}
	D3D12_SAMPLER_DESC sampler_desc = rtd3d12_sampler_desc(texture_view);
	ctx->d3d_device->CreateSampler(&sampler_desc, texture_view->sampler_cpu);
	return true;
}

static bool rtd3d12_texture_view_recreate_sampler(struct rt_texture_view_t* texture_view) {
	struct rtd3d12_context* ctx = texture_view->ctx;
	if (!rtd3d12_texture_view_prepare_sampler_heap(ctx, texture_view)) {
		return false;
	}
	D3D12_SAMPLER_DESC sampler_desc = rtd3d12_sampler_desc(texture_view);
	ctx->d3d_device->CreateSampler(&sampler_desc, texture_view->sampler_cpu);
	return true;
}

void rtd3d12_texture_view_filter(
	struct rt_texture_view_t* texture_view,
	rt::filter mag_filter,
	rt::filter min_filter,
	rt::mip_filter mip_filter
) {
	if (!rtd3d12_texture_view_sampler_valid(texture_view)) {
		return;
	}
	if (texture_view->mag_filter == mag_filter &&
		texture_view->min_filter == min_filter &&
		texture_view->mip_filter == mip_filter) {
		return;
	}
	texture_view->mag_filter = mag_filter;
	texture_view->min_filter = min_filter;
	texture_view->mip_filter = mip_filter;
	rtd3d12_texture_view_normalize_sampler(texture_view);
	rtd3d12_texture_view_recreate_sampler(texture_view);
}

void rtd3d12_texture_view_address(
	struct rt_texture_view_t* texture_view,
	rt::address_mode address_u,
	rt::address_mode address_v,
	rt::address_mode address_w
) {
	if (!rtd3d12_texture_view_sampler_valid(texture_view)) {
		return;
	}
	if (texture_view->address_u == address_u &&
		texture_view->address_v == address_v &&
		texture_view->address_w == address_w) {
		return;
	}
	texture_view->address_u = address_u;
	texture_view->address_v = address_v;
	texture_view->address_w = address_w;
	rtd3d12_texture_view_normalize_sampler(texture_view);
	rtd3d12_texture_view_recreate_sampler(texture_view);
}

void rtd3d12_texture_view_anisotropy(struct rt_texture_view_t* texture_view, u32 max_anisotropy) {
	if (!rtd3d12_texture_view_sampler_valid(texture_view)) {
		return;
	}
	if (texture_view->max_anisotropy == max_anisotropy) {
		return;
	}
	texture_view->max_anisotropy = max_anisotropy;
	rtd3d12_texture_view_normalize_sampler(texture_view);
	rtd3d12_texture_view_recreate_sampler(texture_view);
}

void rtd3d12_texture_view_lod(
	struct rt_texture_view_t* texture_view,
	f32 min_lod,
	f32 max_lod,
	f32 lod_bias
) {
	if (!rtd3d12_texture_view_sampler_valid(texture_view)) {
		return;
	}
	if (texture_view->min_lod == min_lod &&
		texture_view->max_lod == max_lod &&
		texture_view->lod_bias == lod_bias) {
		return;
	}
	texture_view->min_lod = min_lod;
	texture_view->max_lod = max_lod;
	texture_view->lod_bias = lod_bias;
	rtd3d12_texture_view_normalize_sampler(texture_view);
	rtd3d12_texture_view_recreate_sampler(texture_view);
}

rt::timepoint rtd3d12_texture_copy(struct rtd3d12_context* ctx, struct rt_texture_t* src_texture, u32 src_mip, struct rt_texture_t* dst_texture, u32 dst_mip) {
	struct rt_queue_t* queue = rtd3d12_texture_upload_queue(ctx);
	rt::timepoint timepoint = {};
	if (!queue) {
		rtd3d12_fail(rt::error::improper_usage, "texture copy requires a valid queue");
		return timepoint;
	}
	struct rt_texture_t* src_node = src_texture ? src_texture->active : nullptr;
	struct rt_texture_t* dst_node = dst_texture ? dst_texture->active : nullptr;
	if (!src_node || !dst_node) {
		rtd3d12_fail(rt::error::improper_usage, "texture copy source or destination is invalid");
		return timepoint;
	}
	if (src_mip != 0 || dst_mip != 0) {
		rtd3d12_fail(rt::error::unsupported_feature, "texture copy currently supports only mip 0");
		return timepoint;
	}
	if (!rtd3d12_texture_copy_region(ctx, queue, src_node, 0, 0, 0, dst_node, 0, 0, 0, src_node->width, src_node->height, 1)) {
		return timepoint;
	}
	timepoint = rtd3d12_queue_timepoint(queue, queue->fence_value);
	return timepoint;
}

static bool rtd3d12_texture_upload_staging(struct rtd3d12_context* ctx, struct rt_queue_t* queue, u64 size) {
	if (queue->upload_buffer && queue->upload_buffer_size >= size) {
		rtd3d12_wait_for_timepoint(ctx, rtd3d12_queue_timepoint(queue, queue->upload_fence_value));
		queue->upload_fence_value = 0;
		return true;
	}

	rtd3d12_wait_for_timepoint(ctx, rtd3d12_queue_timepoint(queue, queue->upload_fence_value));
	queue->upload_fence_value = 0;
	if (queue->upload_buffer) {
		queue->upload_buffer->Release();
		queue->upload_buffer = nullptr;
	}
	queue->upload_buffer_size = 0;

	D3D12_HEAP_PROPERTIES upload_heap = {};
	upload_heap.Type = D3D12_HEAP_TYPE_UPLOAD;
	upload_heap.CreationNodeMask = 1;
	upload_heap.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC upload_desc = {};
	upload_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	upload_desc.Width = size;
	upload_desc.Height = 1;
	upload_desc.DepthOrArraySize = 1;
	upload_desc.MipLevels = 1;
	upload_desc.SampleDesc.Count = 1;
	upload_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT result = ctx->d3d_device->CreateCommittedResource(
		&upload_heap,
		D3D12_HEAP_FLAG_NONE,
		&upload_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&queue->upload_buffer)
	);
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommittedResource(texture upload) failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}
	queue->upload_buffer_size = size;
	return true;
}

rt::timepoint rtd3d12_texture_data(struct rtd3d12_context* ctx, struct rt_texture_t* texture, rt::texture_type type, u32 width, u32 height, u32 depth, u32 mip, rt::format format, const void* data) {
	struct rt_queue_t* queue = rtd3d12_texture_upload_queue(ctx);
	rt::timepoint timepoint = {};
	assert(queue);
	assert(texture);
	assert(type == rt::texture_type::texture_2d);
	assert(mip == 0);
	assert(depth <= 1);
	assert(width != 0);
	assert(height != 0);
	std::lock_guard<std::mutex> upload_lock(queue->upload_lock);

	DXGI_FORMAT dxgi_format = rtd3d12_texture_format(format);
	u32 bytes_per_pixel = rtd3d12_texture_format_bytes_per_pixel(format);
	if (dxgi_format == DXGI_FORMAT_UNKNOWN || bytes_per_pixel == 0) {
		rtd3d12_fail(rt::error::unsupported_feature, "unsupported texture format");
		return timepoint;
	}

	rtd3d12_queue_collect(ctx, queue);
	rtd3d12_texture_collect_nodes(texture);
	struct rt_texture_t* node = rtd3d12_texture_node_create(ctx);
	if (!node) {
		return timepoint;
	}

	D3D12_HEAP_PROPERTIES texture_heap = {};
	texture_heap.Type = D3D12_HEAP_TYPE_DEFAULT;
	texture_heap.CreationNodeMask = 1;
	texture_heap.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC texture_desc = {};
	texture_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texture_desc.Width = width;
	texture_desc.Height = height;
	texture_desc.DepthOrArraySize = 1;
	texture_desc.MipLevels = 1;
	texture_desc.Format = dxgi_format;
	texture_desc.SampleDesc.Count = 1;
	texture_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	if (rtd3d12_texture_format_is_depth(dxgi_format)) {
		texture_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	} else {
		texture_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}

	D3D12_CLEAR_VALUE clear_value = {};
	clear_value.Format = dxgi_format;
	clear_value.DepthStencil.Depth = 1.0f;
	const bool depth_format = rtd3d12_texture_format_is_depth(dxgi_format);
	const bool has_initial_data = data != nullptr;
	D3D12_RESOURCE_STATES initial_state = depth_format ? D3D12_RESOURCE_STATE_DEPTH_WRITE : (has_initial_data ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	HRESULT result = ctx->d3d_device->CreateCommittedResource(
		&texture_heap,
		D3D12_HEAP_FLAG_NONE,
		&texture_desc,
		initial_state,
		depth_format ? &clear_value : nullptr,
		IID_PPV_ARGS(&node->d3d_resource)
	);
	if (FAILED(result)) {
		if (node) (node)->release();
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommittedResource(texture) failed: 0x{:08x}", static_cast<u32>(result));
		return timepoint;
	}

	node->dxgi_format = dxgi_format;
	node->state = initial_state;
	node->type = type;
	node->width = width;
	node->height = height;
	node->depth = 1;

	if (depth_format && !data) {
		rtd3d12_texture_recycle_node(texture, texture->active);
		texture->active = node;
		return timepoint;
	}
	if (!data) {
		rtd3d12_texture_recycle_node(texture, texture->active);
		texture->active = node;
		return timepoint;
	}

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
	u32 row_count = 0;
	u64 row_size = 0;
	u64 upload_size = 0;
	ctx->d3d_device->GetCopyableFootprints(&texture_desc, 0, 1, 0, &footprint, &row_count, &row_size, &upload_size);

	if (data && !rtd3d12_texture_upload_staging(ctx, queue, upload_size)) {
		if (node) (node)->release();
		return timepoint;
	}

	if (data) {
		void* mapped = nullptr;
		result = queue->upload_buffer->Map(0, nullptr, &mapped);
		if (FAILED(result)) {
			if (node) (node)->release();
			rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12Resource::Map failed: 0x{:08x}", static_cast<u32>(result));
			return timepoint;
		}

		const u08* src = (const u08*)data;
		u08* dst = (u08*)mapped;
		u64 packed_pitch = static_cast<u64>(width) * bytes_per_pixel;
		for (u32 y = 0; y < height; y++) {
			memcpy(dst + static_cast<usize>(y) * footprint.Footprint.RowPitch, src + static_cast<usize>(y) * packed_pitch, static_cast<usize>(packed_pitch));
		}
		queue->upload_buffer->Unmap(0, nullptr);
	}

	std::lock_guard<std::mutex> queue_lock(ctx->queue_lock);
	if (!rtd3d12_queue_acquire_upload_command(ctx, queue)) {
		if (node) (node)->release();
		return timepoint;
	}
	ID3D12GraphicsCommandList* command_list = queue->upload_command_list;

	if (data) {
		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = queue->upload_buffer;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = footprint;

		D3D12_TEXTURE_COPY_LOCATION dst = {};
		dst.pResource = node->d3d_resource;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = 0;
		command_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	}

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = node->d3d_resource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	command_list->ResourceBarrier(1, &barrier);

	result = command_list->Close();
	if (FAILED(result)) {
		if (node) (node)->release();
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12GraphicsCommandList::Close failed: 0x{:08x}", static_cast<u32>(result));
		return timepoint;
	}

	ID3D12CommandList* lists[] = { command_list };
	queue->d3d_queue->ExecuteCommandLists(1, lists);
	u64 fence_value = ++queue->fence_value;
	result = queue->d3d_queue->Signal(queue->d3d_fence, fence_value);
	if (FAILED(result)) {
		if (node) (node)->release();
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12CommandQueue::Signal failed: 0x{:08x}", static_cast<u32>(result));
		return timepoint;
	}

	queue->fence_value = fence_value;
	queue->upload_fence_value = fence_value;
	timepoint = rtd3d12_queue_timepoint(queue, fence_value);
	node->state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	rtd3d12_texture_recycle_node(texture, texture->active);
	texture->active = node;
	return timepoint;
}

rt::timepoint rtd3d12_texture_subcopy(struct rtd3d12_context* ctx, struct rt_texture_t* src_texture, u32 src_mip, u32 src_x, u32 src_y, u32 src_z, struct rt_texture_t* dst_texture, u32 dst_mip, u32 dst_x, u32 dst_y, u32 dst_z, u32 width, u32 height, u32 depth) {
	struct rt_queue_t* queue = rtd3d12_texture_upload_queue(ctx);
	rt::timepoint timepoint = {};
	assert(queue);
	struct rt_texture_t* src_node = src_texture ? src_texture->active : nullptr;
	struct rt_texture_t* dst_node = dst_texture ? dst_texture->active : nullptr;
	assert(src_node);
	assert(dst_node);
	assert(src_mip == 0);
	assert(dst_mip == 0);
	assert(src_z == 0);
	assert(dst_z == 0);
	assert(depth == 1);
	if (!rtd3d12_texture_copy_region(ctx, queue, src_node, src_x, src_y, src_z, dst_node, dst_x, dst_y, dst_z, width, height, depth)) {
		return timepoint;
	}
	timepoint = rtd3d12_queue_timepoint(queue, queue->fence_value);
	return timepoint;
}

rt::timepoint rtd3d12_texture_subdata(struct rtd3d12_context* ctx, struct rt_texture_t* texture, u32 mip, u32 offset_x, u32 offset_y, u32 offset_z, u32 width, u32 height, u32 depth, const void* data) {
	struct rt_queue_t* queue = rtd3d12_texture_upload_queue(ctx);
	rt::timepoint timepoint = {};
	assert(queue);
	struct rt_texture_t* node = texture ? texture->active : nullptr;
	assert(node && node->d3d_resource);
	assert(mip == 0);
	assert(offset_z == 0);
	assert(depth == 1);
	assert(data);
	assert(width != 0);
	assert(height != 0);
	assert(offset_x <= node->width);
	assert(offset_y <= node->height);
	assert(width <= node->width - offset_x);
	assert(height <= node->height - offset_y);
	std::lock_guard<std::mutex> upload_lock(queue->upload_lock);

	u32 bytes_per_pixel = rtd3d12_texture_view_bytes_per_pixel(node->dxgi_format);
	if (bytes_per_pixel == 0) {
		rtd3d12_fail(rt::error::unsupported_feature, "texture subdata does not support this DirectX 12 format");
		return timepoint;
	}

	D3D12_RESOURCE_DESC footprint_desc = {};
	footprint_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	footprint_desc.Width = width;
	footprint_desc.Height = height;
	footprint_desc.DepthOrArraySize = 1;
	footprint_desc.MipLevels = 1;
	footprint_desc.Format = node->dxgi_format;
	footprint_desc.SampleDesc.Count = 1;
	footprint_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
	u32 row_count = 0;
	u64 row_size = 0;
	u64 upload_size = 0;
	ctx->d3d_device->GetCopyableFootprints(&footprint_desc, 0, 1, 0, &footprint, &row_count, &row_size, &upload_size);
	if (!rtd3d12_texture_upload_staging(ctx, queue, upload_size)) {
		return timepoint;
	}

	void* mapped = nullptr;
	HRESULT result = queue->upload_buffer->Map(0, nullptr, &mapped);
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12Resource::Map failed: 0x{:08x}", static_cast<u32>(result));
		return timepoint;
	}
	const u08* src = (const u08*)data;
	u08* dst = (u08*)mapped;
	u64 packed_pitch = static_cast<u64>(width) * bytes_per_pixel;
	for (u32 y = 0; y < height; ++y) {
		memcpy(dst + static_cast<usize>(y) * footprint.Footprint.RowPitch, src + static_cast<usize>(y) * packed_pitch, static_cast<usize>(packed_pitch));
	}
	queue->upload_buffer->Unmap(0, nullptr);

	std::lock_guard<std::mutex> queue_lock(ctx->queue_lock);
	if (!rtd3d12_queue_acquire_upload_command(ctx, queue)) {
		return timepoint;
	}
	ID3D12GraphicsCommandList* command_list = queue->upload_command_list;
	D3D12_RESOURCE_STATES original_state = node->state;
	if (original_state != D3D12_RESOURCE_STATE_COPY_DEST) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = node->d3d_resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = original_state;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		command_list->ResourceBarrier(1, &barrier);
	}

	D3D12_TEXTURE_COPY_LOCATION src_location = {};
	src_location.pResource = queue->upload_buffer;
	src_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src_location.PlacedFootprint = footprint;

	D3D12_TEXTURE_COPY_LOCATION dst_location = {};
	dst_location.pResource = node->d3d_resource;
	dst_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst_location.SubresourceIndex = 0;
	command_list->CopyTextureRegion(&dst_location, offset_x, offset_y, 0, &src_location, nullptr);

	if (original_state != D3D12_RESOURCE_STATE_COPY_DEST) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = node->d3d_resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = original_state;
		command_list->ResourceBarrier(1, &barrier);
	}

	result = command_list->Close();
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12GraphicsCommandList::Close failed: 0x{:08x}", static_cast<u32>(result));
		return timepoint;
	}

	ID3D12CommandList* lists[] = { command_list };
	queue->d3d_queue->ExecuteCommandLists(1, lists);
	u64 fence_value = ++queue->fence_value;
	result = queue->d3d_queue->Signal(queue->d3d_fence, fence_value);
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12CommandQueue::Signal failed: 0x{:08x}", static_cast<u32>(result));
		return timepoint;
	}

	queue->fence_value = fence_value;
	queue->upload_fence_value = fence_value;
	timepoint = rtd3d12_queue_timepoint(queue, fence_value);
	node->state = original_state;
	return timepoint;
}

rt::timepoint rtd3d12_texture_view_copy_to_buffer(struct rtd3d12_context* ctx, struct rt_texture_view_t* texture_view, struct rt_buffer_t* buffer) {
	struct rt_queue_t* queue = rtd3d12_texture_upload_queue(ctx);
	rt::timepoint timepoint = {};
	assert(queue);
	assert(texture_view && texture_view->image->d3d_resource);
	assert(buffer);

	u32 bytes_per_pixel = rtd3d12_texture_view_bytes_per_pixel(texture_view->image->dxgi_format);
	if (bytes_per_pixel == 0) {
		rtd3d12_fail(rt::error::unsupported_feature, "texture view copy does not support this DirectX 12 format");
		return timepoint;
	}

	u64 packed_size = static_cast<u64>(texture_view->image->width) * texture_view->image->height * bytes_per_pixel;
	if (!buffer->active || buffer->active->size < packed_size) {
		rtd3d12_buffer_resize(ctx, buffer, rt::memory_type::device, static_cast<usize>(packed_size));
	}
	if (!buffer->active) {
		return timepoint;
	}

	D3D12_RESOURCE_DESC texture_desc = texture_view->image->d3d_resource->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
	u32 row_count = 0;
	u64 row_size = 0;
	u64 total_size = 0;
	ctx->d3d_device->GetCopyableFootprints(&texture_desc, 0, 1, 0, &footprint, &row_count, &row_size, &total_size);

	D3D12_HEAP_PROPERTIES heap = {};
	heap.Type = D3D12_HEAP_TYPE_READBACK;
	heap.CreationNodeMask = 1;
	heap.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC buffer_desc = {};
	buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	buffer_desc.Width = total_size;
	buffer_desc.Height = 1;
	buffer_desc.DepthOrArraySize = 1;
	buffer_desc.MipLevels = 1;
	buffer_desc.SampleDesc.Count = 1;
	buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* readback = nullptr;
	HRESULT result = ctx->d3d_device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&buffer_desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&readback)
	);
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommittedResource(readback) failed: 0x{:08x}", static_cast<u32>(result));
		return timepoint;
	}

	ID3D12CommandAllocator* allocator = nullptr;
	result = ctx->d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
	if (FAILED(result)) {
		if (readback) {
			readback->Release();
			readback = nullptr;
		}
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommandAllocator failed: 0x{:08x}", static_cast<u32>(result));
		return timepoint;
	}

	ID3D12GraphicsCommandList* command_list = nullptr;
	result = ctx->d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, nullptr, IID_PPV_ARGS(&command_list));
	if (FAILED(result)) {
		if (allocator) {
			allocator->Release();
			allocator = nullptr;
		}
		if (readback) {
			readback->Release();
			readback = nullptr;
		}
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateCommandList failed: 0x{:08x}", static_cast<u32>(result));
		return timepoint;
	}

	{
		std::lock_guard<std::mutex> queue_lock(ctx->queue_lock);
		D3D12_RESOURCE_STATES original_state = texture_view->image->state;
		if (original_state != D3D12_RESOURCE_STATE_COPY_SOURCE) {
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = texture_view->image->d3d_resource;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = original_state;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
			command_list->ResourceBarrier(1, &barrier);
		}

		D3D12_TEXTURE_COPY_LOCATION dst = {};
		dst.pResource = readback;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		dst.PlacedFootprint = footprint;

		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = texture_view->image->d3d_resource;
		src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.SubresourceIndex = 0;
		command_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		if (original_state != D3D12_RESOURCE_STATE_COPY_SOURCE) {
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = texture_view->image->d3d_resource;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
			barrier.Transition.StateAfter = original_state;
			command_list->ResourceBarrier(1, &barrier);
		}

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
			if (readback) {
				readback->Release();
				readback = nullptr;
			}
			rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12GraphicsCommandList::Close failed: 0x{:08x}", static_cast<u32>(result));
			return timepoint;
		}

		ID3D12CommandList* lists[] = { command_list };
		queue->d3d_queue->ExecuteCommandLists(1, lists);
		u64 fence_value = ++queue->fence_value;
		result = queue->d3d_queue->Signal(queue->d3d_fence, fence_value);
		if (FAILED(result)) {
			if (command_list) {
				command_list->Release();
				command_list = nullptr;
			}
			if (allocator) {
				allocator->Release();
				allocator = nullptr;
			}
			if (readback) {
				readback->Release();
				readback = nullptr;
			}
			rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12CommandQueue::Signal failed: 0x{:08x}", static_cast<u32>(result));
			return timepoint;
		}
		queue->fence_value = fence_value;
		timepoint = rtd3d12_queue_timepoint(queue, fence_value);
	}
	rtd3d12_wait_for_timepoint(ctx, timepoint);

	D3D12_RANGE read_range = { 0, static_cast<SIZE_T>(total_size) };
	void* mapped = nullptr;
	result = readback->Map(0, &read_range, &mapped);
	if (FAILED(result)) {
		if (command_list) {
			command_list->Release();
			command_list = nullptr;
		}
		if (allocator) {
			allocator->Release();
			allocator = nullptr;
		}
		if (readback) {
			readback->Release();
			readback = nullptr;
		}
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "ID3D12Resource::Map failed: 0x{:08x}", static_cast<u32>(result));
		return timepoint;
	}

	std::vector<u08> packed(static_cast<usize>(packed_size));
	const u08* src_bytes = (const u08*)mapped;
	for (u32 y = 0; y < texture_view->image->height; y++) {
		const u08* src_row = src_bytes + static_cast<usize>(y) * footprint.Footprint.RowPitch;
		u08* dst_row = packed.data() + static_cast<usize>(y) * texture_view->image->width * bytes_per_pixel;
		memcpy(dst_row, src_row, static_cast<usize>(texture_view->image->width) * bytes_per_pixel);
	}
	D3D12_RANGE write_range = { 0, 0 };
	readback->Unmap(0, &write_range);

	if (rtd3d12_texture_view_needs_bgra_swizzle(texture_view->image->dxgi_format)) {
		for (u64 i = 0; i < packed_size; i += 4) {
			u08 tmp = packed[static_cast<usize>(i) + 0];
			packed[static_cast<usize>(i) + 0] = packed[static_cast<usize>(i) + 2];
			packed[static_cast<usize>(i) + 2] = tmp;
		}
	}

	rtd3d12_buffer_subdata(ctx, buffer, 0, packed_size, packed.data());
	if (command_list) {
		command_list->Release();
		command_list = nullptr;
	}
	if (allocator) {
		allocator->Release();
		allocator = nullptr;
	}
	if (readback) {
		readback->Release();
		readback = nullptr;
	}
	return timepoint;
}
