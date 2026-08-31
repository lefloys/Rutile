#ifndef RTSW_SAMPLER_H
#define RTSW_SAMPLER_H

#include "resource.h"

struct rtsw_sampler {
	struct rtsw_resource_base base;
	enum rt_filter mag_filter;
	enum rt_filter min_filter;
	enum rt_mip_filter mip_filter;
	enum rt_address_mode address_u;
	enum rt_address_mode address_v;
	enum rt_address_mode address_w;
	usize max_anisotropy;
	f32 min_lod;
	f32 max_lod;
	f32 lod_bias;
};

RTSW_API rt_sampler rtSamplerCreate(void);
RTSW_API void rtSamplerDestroy(rt_sampler sampler);
RTSW_API void rtSamplerSetFilter(rt_sampler sampler, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter);
RTSW_API void rtSamplerSetAddress(rt_sampler sampler, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w);
RTSW_API void rtSamplerSetAnisotropy(rt_sampler sampler, usize max_anisotropy);
RTSW_API void rtSamplerSetLod(rt_sampler sampler, f32 min_lod, f32 max_lod, f32 lod_bias);

RTSW_DECLARE_HANDLE(sampler, rtsw_sampler);

#endif
