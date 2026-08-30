#pragma once

#include "config.hpp"
#include "resource.hpp"

#include <d3d12.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTD3D12_API rt_buffer_t* rtBufferCreate();
RTD3D12_API void rtBufferDestroy(rt_buffer_t* buffer);
RTD3D12_API void rtBufferResize(rt_buffer_t* buffer, rt::memory_type memory_type, usize size);
RTD3D12_API void rtBufferRead(rt_buffer_t* buffer, rt::buffer_range range, u08* data, usize data_size);
RTD3D12_API u08* rtBufferMap(rt_buffer_t* buffer, rt::buffer_range range);
RTD3D12_API void rtBufferUnmap(rt_buffer_t* buffer);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rt_buffer_t : rtd3d12_resource<rt_buffer_t> {
	explicit rt_buffer_t(rtd3d12_context* ctx) : rtd3d12_resource(ctx) {}
	~rt_buffer_t();

	/* The public buffer owns an active physical node and recyclable revisions.
	 * Physical nodes are ordinary rt_buffer_t resources kept alive by command
	 * references and submitted jobs, just like the Vulkan backend. */
	rt_buffer_t* active{};
	rt_buffer_t* next{};
	ID3D12Resource* d3d_resource{};
	D3D12_VERTEX_BUFFER_VIEW vertex_view{};
	usize size{};
	D3D12_RESOURCE_STATES state{};
	rt::memory_type memory_type{};
};
struct rtd3d12_buffer_write {
	rt_buffer_t* source;
	rt_buffer_t* target;
};

void rtd3d12_buffer_resize(rtd3d12_context* ctx, rt_buffer_t* buffer, rt::memory_type memory_type, usize size);
rt::timepoint rtd3d12_buffer_subdata(rtd3d12_context* ctx, rt_buffer_t* buffer, u64 offset, u64 size, const void* data);
void rtd3d12_buffer_read(rtd3d12_context* ctx, rt_buffer_t* buffer, rt::buffer_range range, u08* data, usize data_size);
u08* rtd3d12_buffer_map(rtd3d12_context* ctx, rt_buffer_t* buffer, rt::buffer_range range);
void rtd3d12_buffer_unmap(rtd3d12_context* ctx, rt_buffer_t* buffer);
rtd3d12_buffer_write rtd3d12_buffer_write_begin(rtd3d12_context* ctx, rt_buffer_t* buffer);
rt_buffer_t* rtd3d12_buffer_active_node(rt_buffer_t* buffer);
void rtd3d12_buffer_recycle_node(rt_buffer_t* buffer, rt_buffer_t* node);
rt_buffer_t* rtd3d12_buffer_take_reusable_node(rt_buffer_t* buffer, rt::memory_type memory_type, usize size);
void rtd3d12_buffer_write_commit(rt_buffer_t* buffer, rtd3d12_buffer_write* write);
void rtd3d12_buffer_write_cancel(rt_buffer_t* buffer, rtd3d12_buffer_write* write);
