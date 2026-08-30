#pragma once

#include "config.hpp"
#include "resource.hpp"

#include <d3d12.h>
#include <dxgi1_6.h>

struct rt_buffer_t;

RTD3D12_API rt_texture_t* rtTextureCreate();
RTD3D12_API void rtTextureDestroy(rt_texture_t* texture);
RTD3D12_API void rtTextureResize(rt_texture_t* texture, rt::texture_type type, rt::format format, rt::extent_3d extent, usize mip_count);
RTD3D12_API void rtCmdTextureCopy(rt_command_buffer_t* command_buffer, rt_texture_t* src, rt::texture_range src_range, rt_texture_t* dst, rt::texture_range dst_range);
RTD3D12_API void rtCmdTextureData(rt_command_buffer_t* command_buffer, rt_texture_t* texture, rt::texture_range range, const u08* data);
RTD3D12_API void rtCmdTextureCopyToBuffer(rt_command_buffer_t* command_buffer, rt_texture_t* src, rt::texture_range src_range, rt_buffer_t* dst, rt::buffer_range dst_range);
RTD3D12_API void rtCmdTextureBarrier(rt_command_buffer_t* command_buffer, rt_texture_t* texture, rt::texture_range range, rt::access src, rt::access dst);
RTD3D12_API rt_texture_view_t* rtTextureViewCreate();
RTD3D12_API void rtTextureViewDestroy(rt_texture_view_t* texture_view);
RTD3D12_API rt::extent_3d rtTextureViewExtent(rt_texture_view_t* texture_view);
RTD3D12_API void rtTextureViewSetTexture(rt_texture_view_t* texture_view, rt_texture_t* texture);
RTD3D12_API void rtTextureViewSetFilter(rt_texture_view_t* texture_view, rt::filter mag_filter, rt::filter min_filter, rt::mip_filter mip_filter);
RTD3D12_API void rtTextureViewSetAddress(rt_texture_view_t* texture_view, rt::address_mode address_u, rt::address_mode address_v, rt::address_mode address_w);
RTD3D12_API void rtTextureViewSetAnisotropy(rt_texture_view_t* texture_view, usize max_anisotropy);
RTD3D12_API void rtTextureViewSetLod(rt_texture_view_t* texture_view, f32 min_lod, f32 max_lod, f32 lod_bias);
RTD3D12_API void rtTextureViewRead(rt_texture_view_t* texture_view, rt::texture_range range, u08* data, usize data_size);

struct rtd3d12_image_base : rtd3d12_resource<rt_texture_t> {
	rtd3d12_image_base() : rtd3d12_resource(nullptr) {}
	explicit rtd3d12_image_base(rtd3d12_context* ctx) : rtd3d12_resource(ctx) {}
	ID3D12Resource* d3d_resource{};
	usize width{};
	usize height{};
	usize depth{};
	usize mip_count{};
	usize layer_count{};
	DXGI_FORMAT dxgi_format{};
	/* Compatibility mirror until every lowering path has been converted to
	 * per-subresource transitions. */
	D3D12_RESOURCE_STATES state{};
	D3D12_RESOURCE_STATES* states{};
	rt::texture_type type{};
	bool swapchain_image{};
};

struct rt_texture_t : rtd3d12_image_base {
	explicit rt_texture_t(rtd3d12_context* ctx) : rtd3d12_image_base(ctx) {}
	~rt_texture_t();
	rt_texture_t* active{};
	rt_texture_t* next{};
};
struct rtd3d12_texture_write {
	rtd3d12_image_base* source;
	rtd3d12_image_base* target;
};
struct rt_texture_view_t : rtd3d12_resource<rt_texture_view_t> {
	explicit rt_texture_view_t(rtd3d12_context* ctx) : rtd3d12_resource(ctx) {}
	~rt_texture_view_t();

	rtd3d12_image_base* image{};
	ID3D12DescriptorHeap* d3d_sampler_heap{};
	ID3D12DescriptorHeap* d3d_srv_heap{};
	ID3D12DescriptorHeap* d3d_rtv_heap{};
	ID3D12DescriptorHeap* d3d_dsv_heap{};
	D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
	D3D12_CPU_DESCRIPTOR_HANDLE dsv{};
	D3D12_CPU_DESCRIPTOR_HANDLE sampler_cpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE sampler_gpu{};
	D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu{};

	rt::filter mag_filter;
	rt::filter min_filter;
	rt::mip_filter mip_filter;
	rt::address_mode address_u;
	rt::address_mode address_v;
	rt::address_mode address_w;
	u32 max_anisotropy{};
	f32 min_lod{};
	f32 max_lod{};
	f32 lod_bias{};
};
rt_texture_view_t* rtd3d12_texture_view_create_for_texture(rtd3d12_context* ctx, rt_texture_t* texture, D3D12_CPU_DESCRIPTOR_HANDLE rtv);
rt_texture_t* rtd3d12_texture_create_for_swapchain_image(rtd3d12_context* ctx, ID3D12Resource* image, DXGI_FORMAT format, u32 width, u32 height);
rt_texture_view_t* rtd3d12_texture_view_create_for_swapchain(rtd3d12_context* ctx, rt_texture_t* texture, D3D12_CPU_DESCRIPTOR_HANDLE rtv);
void rtd3d12_texture_view_bind(rtd3d12_context* ctx, rt_texture_view_t* view, rt_texture_t* texture);

void rtd3d12_texture_view_filter(rt_texture_view_t* texture_view, rt::filter mag_filter, rt::filter min_filter, rt::mip_filter mip_filter);
void rtd3d12_texture_view_address(rt_texture_view_t* texture_view, rt::address_mode address_u, rt::address_mode address_v, rt::address_mode address_w);
void rtd3d12_texture_view_anisotropy(rt_texture_view_t* texture_view, u32 max_anisotropy);
void rtd3d12_texture_view_lod(rt_texture_view_t* texture_view, f32 min_lod, f32 max_lod, f32 lod_bias);
bool rtd3d12_texture_view_prepare_sampler(rtd3d12_context* ctx, rt_texture_view_t* texture_view);
bool rtd3d12_texture_view_refresh(rtd3d12_context* ctx, rt_texture_view_t* texture_view);
bool rtd3d12_texture_format_is_depth(DXGI_FORMAT format);

rtd3d12_texture_write rtd3d12_texture_write_begin(rtd3d12_context* ctx, rt_texture_t* texture);
void rtd3d12_texture_resize(rtd3d12_context* ctx, rt_texture_t* texture, rt::texture_type type, rt::format format, rt::extent_3d extent, usize mip_count);
usize rtd3d12_texture_subresource_count(const rtd3d12_image_base* image);
D3D12_RESOURCE_STATES rtd3d12_texture_subresource_state(const rtd3d12_image_base* image, usize mip, usize layer);
void rtd3d12_texture_set_subresource_state(rtd3d12_image_base* image, usize mip, usize layer, D3D12_RESOURCE_STATES state);
