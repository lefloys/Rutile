#include "sampler.h"

#include "context.h"
#include "error.h"

#include <stdlib.h>

static void rtsw_sampler_finish(struct rtsw_sampler* sampler) {
	(void)sampler;
}

static void rtsw_sampler_finalize_resource(void* value) {
	struct rtsw_sampler* sampler = value;
	rtsw_sampler_finish(sampler);
	free(sampler);
}

RTSW_DEFINE_HANDLE(sampler, rtsw_sampler)

rt_sampler rtSamplerCreate(void) {
	struct rtsw_context* ctx = rtsw_get_current_context();
	struct rtsw_sampler* sampler;
	rtsw_clear_error();
	if (!ctx) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtSamplerCreate called before rtInit");
		return RT_NULL_HANDLE;
	}
	sampler = RTSW_ALLOC_RESOURCE(struct rtsw_sampler);
	if (!sampler) return RT_NULL_HANDLE;
	rtsw_init_resource_base(ctx, &sampler->base, sampler, rtsw_sampler_finalize_resource);
	sampler->mag_filter = RT_FILTER_NEAREST;
	sampler->min_filter = RT_FILTER_NEAREST;
	sampler->mip_filter = RT_MIP_FILTER_NONE;
	sampler->address_u = RT_ADDRESS_CLAMP;
	sampler->address_v = RT_ADDRESS_CLAMP;
	sampler->address_w = RT_ADDRESS_CLAMP;
	sampler->max_anisotropy = 1;
	return rtsw_sampler_to_handle(sampler);
}

void rtSamplerDestroy(rt_sampler handle) {
	struct rtsw_sampler* sampler = rtsw_sampler_from_handle(handle);
	if (sampler) rtsw_resource_retire(&sampler->base);
}

void rtSamplerSetFilter(rt_sampler handle, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter) {
	struct rtsw_sampler* sampler = rtsw_sampler_from_handle(handle);
	rtsw_clear_error();
	if (!sampler) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtSamplerSetFilter received a NULL sampler");
		return;
	}
	sampler->mag_filter = mag_filter;
	sampler->min_filter = min_filter;
	sampler->mip_filter = mip_filter;
}

void rtSamplerSetAddress(rt_sampler handle, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w) {
	struct rtsw_sampler* sampler = rtsw_sampler_from_handle(handle);
	rtsw_clear_error();
	if (!sampler) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtSamplerSetAddress received a NULL sampler");
		return;
	}
	sampler->address_u = address_u;
	sampler->address_v = address_v;
	sampler->address_w = address_w;
}

void rtSamplerSetAnisotropy(rt_sampler handle, usize max_anisotropy) {
	struct rtsw_sampler* sampler = rtsw_sampler_from_handle(handle);
	rtsw_clear_error();
	if (!sampler || !max_anisotropy) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtSamplerSetAnisotropy requires a non-zero anisotropy");
		return;
	}
	sampler->max_anisotropy = max_anisotropy;
}

void rtSamplerSetLod(rt_sampler handle, f32 min_lod, f32 max_lod, f32 lod_bias) {
	struct rtsw_sampler* sampler = rtsw_sampler_from_handle(handle);
	rtsw_clear_error();
	if (!sampler || min_lod > max_lod) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtSamplerSetLod received an invalid range");
		return;
	}
	sampler->min_lod = min_lod;
	sampler->max_lod = max_lod;
	sampler->lod_bias = lod_bias;
}
