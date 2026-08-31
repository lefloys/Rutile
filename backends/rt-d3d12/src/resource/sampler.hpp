#pragma once

#include "config.hpp"
#include "resource.hpp"

#include <d3d12.h>

struct rt_sampler_t : rtd3d12_resource<rt_sampler_t> {
	explicit rt_sampler_t(rtd3d12_context* ctx) : rtd3d12_resource(ctx) {}
	~rt_sampler_t();
	ID3D12DescriptorHeap* heap{};
	D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
	rt::filter mag_filter{rt::filter::linear};
	rt::filter min_filter{rt::filter::linear};
	rt::mip_filter mip_filter{rt::mip_filter::none};
	rt::address_mode address_u{rt::address_mode::repeat};
	rt::address_mode address_v{rt::address_mode::repeat};
	rt::address_mode address_w{rt::address_mode::repeat};
	u32 max_anisotropy{1};
	f32 min_lod{};
	f32 max_lod{D3D12_FLOAT32_MAX};
	f32 lod_bias{};
};

RTD3D12_API rt_sampler_t* rtSamplerCreate();
RTD3D12_API void rtSamplerDestroy(rt_sampler_t* sampler);
RTD3D12_API void rtSamplerSetFilter(rt_sampler_t* sampler, rt::filter mag_filter, rt::filter min_filter, rt::mip_filter mip_filter);
RTD3D12_API void rtSamplerSetAddress(rt_sampler_t* sampler, rt::address_mode address_u, rt::address_mode address_v, rt::address_mode address_w);
RTD3D12_API void rtSamplerSetAnisotropy(rt_sampler_t* sampler, usize max_anisotropy);
RTD3D12_API void rtSamplerSetLod(rt_sampler_t* sampler, f32 min_lod, f32 max_lod, f32 lod_bias);

bool rtd3d12_sampler_prepare(rt_sampler_t* sampler);
