#pragma once

#include "config.hpp"
#include "resource.hpp"

#include <d3d12.h>
#include <vector>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTDX_API rt_buffer rtBufferCreate();
RTDX_API void rtBufferDestroy(rt_buffer buffer);
RTDX_API void rtBufferResize(rt_buffer buffer, rt_memory_type memory_type, usize size);
RTDX_API void rtBufferRead(rt_buffer buffer, rt_buffer_range range, u08* data, usize data_size);
RTDX_API u08* rtBufferMap(rt_buffer buffer, rt_buffer_range range);
RTDX_API void rtBufferUnmap(rt_buffer buffer);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtdx_buffer_storage;
struct rtdx_image_base;
struct rtdx_buffer_texture_source {
	rtdx_image_base* image;
	rt_texture_range source_range;
	rt_buffer_range destination_range;
};

struct rtdx_buffer {
	rtdx_resource_base base;
	rtdx_buffer_storage* storage;
	rtdx_buffer_storage* reusable_storage;
	rt_memory_type memory_type;
};
RTDX_DECLARE_NEW_RESOURCE(buffer)

struct rtdx_buffer_storage {
	rtdx_context* ctx;
	rtdx_buffer_storage* next;

	ID3D12Resource* d3d_resource;
	D3D12_VERTEX_BUFFER_VIEW vertex_view;
	D3D12_RESOURCE_STATES state;

	void* shadow_data;
	bool shadow_valid;
	std::vector<rt_buffer_range> shadow_invalid_ranges;
	/* Exact physical provenance for disjoint texture-to-buffer writes. */
	std::vector<rtdx_buffer_texture_source> texture_sources;

	u64 size;
	rt_memory_type memory_type;
	u32 ref_count;
};

struct rtdx_buffer_write {
	rtdx_buffer_storage* source;
	rtdx_buffer_storage* target;
};

void rtdx_buffer_resize(rtdx_context* ctx, rtdx_buffer* buffer, rt_memory_type memory_type, usize size);
rt_timepoint rtdx_buffer_subdata(rtdx_context* ctx, rtdx_buffer* buffer, u64 offset, u64 size, const void* data);
void rtdx_buffer_read(rtdx_context* ctx, rtdx_buffer* buffer, rt_buffer_range range, u08* data, usize data_size);
u08* rtdx_buffer_map(rtdx_context* ctx, rtdx_buffer* buffer, rt_buffer_range range);
void rtdx_buffer_unmap(rtdx_context* ctx, rtdx_buffer* buffer);
void rtdx_buffer_storage_retain(rtdx_buffer_storage* storage);
void rtdx_buffer_storage_release(rtdx_buffer_storage* storage);
rtdx_buffer_write rtdx_buffer_write_begin(rtdx_context* ctx, rtdx_buffer* buffer);
void rtdx_buffer_storage_clear_texture_source(rtdx_buffer_storage* storage);
void rtdx_buffer_storage_set_texture_source(rtdx_buffer_storage* storage, rtdx_image_base* image, rt_texture_range range, rt_buffer_range destination_range);
void rtdx_buffer_storage_invalidate_texture_source(rtdx_buffer_storage* storage, rt_buffer_range range);
void rtdx_buffer_storage_mark_shadow_invalid(rtdx_buffer_storage* storage, rt_buffer_range range);
void rtdx_buffer_storage_mark_shadow_valid(rtdx_buffer_storage* storage, rt_buffer_range range);
bool rtdx_buffer_storage_shadow_range_valid(const rtdx_buffer_storage* storage, rt_buffer_range range);
