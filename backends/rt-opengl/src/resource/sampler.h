#ifndef RTGL_SAMPLER_H
#define RTGL_SAMPLER_H

#include "resource.h"

RTGL_EXTERN_C_ENTER

struct rtgl_sampler {
	struct rtgl_resource_base base;
	enum rt_filter mag_filter;
	enum rt_filter min_filter;
	enum rt_mip_filter mip_filter;
	enum rt_address_mode address_u;
	enum rt_address_mode address_v;
	enum rt_address_mode address_w;
	u32 max_anisotropy;
	f32 min_lod;
	f32 max_lod;
	f32 lod_bias;
};
RTGL_DECLARE_NEW_RESOURCE(sampler)

RTGL_API rt_sampler rtSamplerCreate(void);
RTGL_API void rtSamplerDestroy(rt_sampler sampler);
RTGL_API void rtSamplerSetFilter(rt_sampler sampler, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter);
RTGL_API void rtSamplerSetAddress(rt_sampler sampler, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w);
RTGL_API void rtSamplerSetAnisotropy(rt_sampler sampler, usize max_anisotropy);
RTGL_API void rtSamplerSetLod(rt_sampler sampler, f32 min_lod, f32 max_lod, f32 lod_bias);

RTGL_EXTERN_C_EXIT
#endif
