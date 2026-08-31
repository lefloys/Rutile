#include "sampler.hpp"

#include "context.hpp"
#include "error.hpp"

namespace {
D3D12_TEXTURE_ADDRESS_MODE address_mode(rt::address_mode value) {
	switch (value) {
	case rt::address_mode::clamp: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case rt::address_mode::mirror: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	case rt::address_mode::repeat: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	}
	return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
}

bool valid_filter(rt::filter value) { return value == rt::filter::nearest || value == rt::filter::linear; }
bool valid_mip_filter(rt::mip_filter value) { return value == rt::mip_filter::none || value == rt::mip_filter::nearest || value == rt::mip_filter::linear; }
bool valid_address(rt::address_mode value) { return value == rt::address_mode::clamp || value == rt::address_mode::repeat || value == rt::address_mode::mirror; }
}

rt_sampler_t::~rt_sampler_t() { if (heap) heap->Release(); }

bool rtd3d12_sampler_prepare(rt_sampler_t* sampler) {
	if (!sampler || !sampler->ctx) return false;
	if (!sampler->heap) {
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		desc.NumDescriptors = 1;
		HRESULT result = sampler->ctx->d3d_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&sampler->heap));
		if (FAILED(result)) { rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateDescriptorHeap(sampler) failed: 0x{:08x}", static_cast<u32>(result)); return false; }
		sampler->cpu = sampler->heap->GetCPUDescriptorHandleForHeapStart();
	}
	const bool min_linear = sampler->min_filter == rt::filter::linear;
	const bool mag_linear = sampler->mag_filter == rt::filter::linear;
	const bool mip_linear = sampler->mip_filter == rt::mip_filter::linear;
	D3D12_SAMPLER_DESC desc = {};
	desc.Filter = min_linear ? (mag_linear ? (mip_linear ? D3D12_FILTER_MIN_MAG_MIP_LINEAR : D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT) : D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT) : (mag_linear ? D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT : D3D12_FILTER_MIN_MAG_MIP_POINT);
	desc.AddressU = address_mode(sampler->address_u); desc.AddressV = address_mode(sampler->address_v); desc.AddressW = address_mode(sampler->address_w);
	desc.MipLODBias = sampler->lod_bias; desc.MaxAnisotropy = sampler->max_anisotropy; desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	desc.MinLOD = sampler->min_lod; desc.MaxLOD = sampler->max_lod;
	sampler->ctx->d3d_device->CreateSampler(&desc, sampler->cpu);
	return true;
}

rt_sampler_t* rtSamplerCreate() { rtd3d12_begin_errorable_operation(); return rtd3d12::create_resource<rt_sampler_t>(rtd3d12_get_current_context()); }
void rtSamplerDestroy(rt_sampler_t* sampler) { if (sampler) sampler->retire(); }
void rtSamplerSetFilter(rt_sampler_t* sampler, rt::filter mag, rt::filter min, rt::mip_filter mip) { rtd3d12_begin_errorable_operation(); if (!sampler || !valid_filter(mag) || !valid_filter(min) || !valid_mip_filter(mip)) { rtd3d12_fail(rt::error::improper_usage, "rtSamplerSetFilter received invalid arguments"); return; } sampler->mag_filter = mag; sampler->min_filter = min; sampler->mip_filter = mip; }
void rtSamplerSetAddress(rt_sampler_t* sampler, rt::address_mode u, rt::address_mode v, rt::address_mode w) { rtd3d12_begin_errorable_operation(); if (!sampler || !valid_address(u) || !valid_address(v) || !valid_address(w)) { rtd3d12_fail(rt::error::improper_usage, "rtSamplerSetAddress received invalid arguments"); return; } sampler->address_u = u; sampler->address_v = v; sampler->address_w = w; }
void rtSamplerSetAnisotropy(rt_sampler_t* sampler, usize value) { rtd3d12_begin_errorable_operation(); if (!sampler || value == 0 || value > UINT_MAX) { rtd3d12_fail(rt::error::improper_usage, "rtSamplerSetAnisotropy received an invalid value"); return; } sampler->max_anisotropy = static_cast<u32>(value); }
void rtSamplerSetLod(rt_sampler_t* sampler, f32 min_lod, f32 max_lod, f32 bias) { rtd3d12_begin_errorable_operation(); if (!sampler || min_lod > max_lod) { rtd3d12_fail(rt::error::improper_usage, "rtSamplerSetLod received an invalid range"); return; } sampler->min_lod = min_lod; sampler->max_lod = max_lod; sampler->lod_bias = bias; }
