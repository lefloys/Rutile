#pragma once

#include "config.hpp"
#include "resource.hpp"

#include <d3d12.h>
#include <dxgi1_6.h>

struct rtdx_buffer;

RTDX_API rt_texture rtTextureCreate();
RTDX_API void rtTextureDestroy(rt_texture texture);
RTDX_API void rtTextureResize(rt_texture texture, rt_texture_type type, rt_format format, rt_extent_3d extent, usize mip_count);
RTDX_API void rtCmdTextureCopy(rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_texture dst, rt_texture_range dst_range);
RTDX_API void rtCmdTextureData(rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, const u08* data);
RTDX_API void rtCmdTextureCopyToBuffer(rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_buffer dst, rt_buffer_range dst_range);
RTDX_API void rtCmdTextureBarrier(rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, rt_access src, rt_access dst);
RTDX_API rt_texture_view rtTextureViewCreate();
RTDX_API void rtTextureViewDestroy(rt_texture_view texture_view);
RTDX_API rt_extent_3d rtTextureViewExtent(rt_texture_view texture_view);
RTDX_API void rtTextureViewSetTexture(rt_texture_view texture_view, rt_texture texture);
RTDX_API void rtTextureViewSetFilter(rt_texture_view texture_view, rt_filter mag_filter, rt_filter min_filter, rt_mip_filter mip_filter);
RTDX_API void rtTextureViewSetAddress(rt_texture_view texture_view, rt_address_mode address_u, rt_address_mode address_v, rt_address_mode address_w);
RTDX_API void rtTextureViewSetAnisotropy(rt_texture_view texture_view, usize max_anisotropy);
RTDX_API void rtTextureViewSetLod(rt_texture_view texture_view, f32 min_lod, f32 max_lod, f32 lod_bias);
RTDX_API void rtTextureViewRead(rt_texture_view texture_view, rt_texture_range range, u08* data, usize data_size);


struct rtdx_image_base : rtdx_resource_base {
	rtdx_image_base() : rtdx_resource_base(nullptr, rtdx_resource_type::texture) {}
	rtdx_image_base(rtdx_context* ctx, rtdx_resource_type resource_type) : rtdx_resource_base(ctx, resource_type) {}
	ID3D12Resource* d3d_resource;
	usize width;
	usize height;
	usize depth;
	usize mip_count;
	usize layer_count;
	DXGI_FORMAT dxgi_format;
	/* Compatibility mirror until every lowering path has been converted to
	 * per-subresource transitions. */
	D3D12_RESOURCE_STATES state;
	D3D12_RESOURCE_STATES* states;
	rt_texture_type type;
	bool swapchain_image;
};

struct rtdx_texture : rtdx_image_base {
	explicit rtdx_texture(rtdx_context* ctx) : rtdx_image_base(ctx, rtdx_resource_type::texture) {}
	void finish() override;
	rtdx_texture* active;
	rtdx_texture* next;
};
struct rtdx_texture_write { rtdx_image_base* source; rtdx_image_base* target; };
RTDX_DECLARE_NEW_RESOURCE(texture)

struct rtdx_texture_view : rtdx_resource_base {
	explicit rtdx_texture_view(rtdx_context* ctx) : rtdx_resource_base(ctx, rtdx_resource_type::texture_view) {}
	void finish() override;

	rtdx_image_base* image;
	ID3D12DescriptorHeap* d3d_sampler_heap;
	ID3D12DescriptorHeap* d3d_srv_heap;
	ID3D12DescriptorHeap* d3d_rtv_heap;
	ID3D12DescriptorHeap* d3d_dsv_heap;
	D3D12_CPU_DESCRIPTOR_HANDLE rtv;
	D3D12_CPU_DESCRIPTOR_HANDLE dsv;
	D3D12_CPU_DESCRIPTOR_HANDLE sampler_cpu;
	D3D12_GPU_DESCRIPTOR_HANDLE sampler_gpu;
	D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu;
	D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu;

	rt_filter mag_filter;
	rt_filter min_filter;
	rt_mip_filter mip_filter;
	rt_address_mode address_u;
	rt_address_mode address_v;
	rt_address_mode address_w;
	u32 max_anisotropy;
	f32 min_lod;
	f32 max_lod;
	f32 lod_bias;
};
RTDX_DECLARE_NEW_RESOURCE(texture_view)

rtdx_texture_view* rtdx_texture_view_create_for_texture(rtdx_context* ctx, rtdx_texture* texture, D3D12_CPU_DESCRIPTOR_HANDLE rtv);
void rtdx_texture_view_bind(rtdx_context* ctx, rtdx_texture_view* view, rtdx_texture* texture);

void rtdx_texture_view_filter(rtdx_texture_view* texture_view, rt_filter mag_filter, rt_filter min_filter, rt_mip_filter mip_filter);
void rtdx_texture_view_address(rtdx_texture_view* texture_view, rt_address_mode address_u, rt_address_mode address_v, rt_address_mode address_w);
void rtdx_texture_view_anisotropy(rtdx_texture_view* texture_view, u32 max_anisotropy);
void rtdx_texture_view_lod(rtdx_texture_view* texture_view, f32 min_lod, f32 max_lod, f32 lod_bias);
bool rtdx_texture_view_prepare_sampler(rtdx_context* ctx, rtdx_texture_view* texture_view);
bool rtdx_texture_view_refresh(rtdx_context* ctx, rtdx_texture_view* texture_view);
bool rtdx_texture_format_is_depth(DXGI_FORMAT format);

rtdx_texture_write rtdx_texture_write_begin(rtdx_context* ctx, rtdx_texture* texture);
void rtdx_texture_resize(rtdx_context* ctx, rtdx_texture* texture, rt_texture_type type, rt_format format, rt_extent_3d extent, usize mip_count);
usize rtdx_texture_subresource_count(const rtdx_image_base* image);
D3D12_RESOURCE_STATES rtdx_texture_subresource_state(const rtdx_image_base* image, usize mip, usize layer);
void rtdx_texture_set_subresource_state(rtdx_image_base* image, usize mip, usize layer, D3D12_RESOURCE_STATES state);
