#pragma once

#include "config.hpp"
#include "resource.hpp"

#include <d3d12.h>

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

struct rtdx_buffer : rtdx_resource_base {
	explicit rtdx_buffer(rtdx_context* ctx) : rtdx_resource_base(ctx, rtdx_resource_type::buffer) {}
	void finish() override;

	/* The public buffer owns an active physical node and recyclable revisions.
	 * Physical nodes are ordinary rtdx_buffer resources kept alive by command
	 * references and submitted jobs, just like the Vulkan backend. */
	rtdx_buffer* active;
	rtdx_buffer* next;
	ID3D12Resource* d3d_resource;
	D3D12_VERTEX_BUFFER_VIEW vertex_view;
	usize size;
	D3D12_RESOURCE_STATES state;
	rt_memory_type memory_type;
};
RTDX_DECLARE_NEW_RESOURCE(buffer)

struct rtdx_buffer_write {
	rtdx_buffer* source;
	rtdx_buffer* target;
};

void rtdx_buffer_resize(rtdx_context* ctx, rtdx_buffer* buffer, rt_memory_type memory_type, usize size);
rt_timepoint rtdx_buffer_subdata(rtdx_context* ctx, rtdx_buffer* buffer, u64 offset, u64 size, const void* data);
void rtdx_buffer_read(rtdx_context* ctx, rtdx_buffer* buffer, rt_buffer_range range, u08* data, usize data_size);
u08* rtdx_buffer_map(rtdx_context* ctx, rtdx_buffer* buffer, rt_buffer_range range);
void rtdx_buffer_unmap(rtdx_context* ctx, rtdx_buffer* buffer);
rtdx_buffer_write rtdx_buffer_write_begin(rtdx_context* ctx, rtdx_buffer* buffer);
rtdx_buffer* rtdx_buffer_active_node(rtdx_buffer* buffer);
void rtdx_buffer_recycle_node(rtdx_buffer* buffer, rtdx_buffer* node);
rtdx_buffer* rtdx_buffer_take_reusable_node(rtdx_buffer* buffer, rt_memory_type memory_type, usize size);
void rtdx_buffer_write_commit(rtdx_buffer* buffer, rtdx_buffer_write* write);
void rtdx_buffer_write_cancel(rtdx_buffer* buffer, rtdx_buffer_write* write);
