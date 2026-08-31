#include "sampler.h"
#include "context.h"
#include "error.h"

RTGL_DEFINE_RESOURCE_PRIVATE(sampler)

void rtgl_sampler_init(struct rtgl_context* ctx, struct rtgl_sampler* sampler) {
	rtgl_init_resource_base(ctx, RTGL_RESOURCE_BASE(sampler), RTGL_RESOURCE_SAMPLER);
	sampler->mag_filter = RT_FILTER_LINEAR; sampler->min_filter = RT_FILTER_LINEAR; sampler->mip_filter = RT_MIP_FILTER_NONE;
	sampler->address_u = RT_ADDRESS_REPEAT; sampler->address_v = RT_ADDRESS_REPEAT; sampler->address_w = RT_ADDRESS_REPEAT;
	sampler->max_anisotropy = 1; sampler->max_lod = 1000.0f;
}
void rtgl_sampler_finish(struct rtgl_sampler* sampler) { rtgl_finish_resource_base(RTGL_RESOURCE_BASE(sampler)); }
rt_sampler rtSamplerCreate(void) { rtgl_begin_errorable_operation(); return rtgl_sampler_to_handle(rtgl_sampler_create(rtgl_get_current_context())); }
void rtSamplerDestroy(rt_sampler sampler) { rtgl_sampler_destroy(rtgl_get_current_context(), rtgl_sampler_from_handle(sampler)); }
void rtSamplerSetFilter(rt_sampler value, enum rt_filter mag, enum rt_filter min, enum rt_mip_filter mip) { rtgl_begin_errorable_operation(); struct rtgl_sampler* s = rtgl_sampler_from_handle(value); if (!s || (mag != RT_FILTER_NEAREST && mag != RT_FILTER_LINEAR) || (min != RT_FILTER_NEAREST && min != RT_FILTER_LINEAR) || mip < RT_MIP_FILTER_NONE || mip > RT_MIP_FILTER_LINEAR) { rtgl_throwf(RT_IMPROPER_USAGE, "rtSamplerSetFilter received invalid arguments"); return; } s->mag_filter = mag; s->min_filter = min; s->mip_filter = mip; }
void rtSamplerSetAddress(rt_sampler value, enum rt_address_mode u, enum rt_address_mode v, enum rt_address_mode w) { rtgl_begin_errorable_operation(); struct rtgl_sampler* s = rtgl_sampler_from_handle(value); if (!s || u < RT_ADDRESS_CLAMP || u > RT_ADDRESS_MIRROR || v < RT_ADDRESS_CLAMP || v > RT_ADDRESS_MIRROR || w < RT_ADDRESS_CLAMP || w > RT_ADDRESS_MIRROR) { rtgl_throwf(RT_IMPROPER_USAGE, "rtSamplerSetAddress received invalid arguments"); return; } s->address_u = u; s->address_v = v; s->address_w = w; }
void rtSamplerSetAnisotropy(rt_sampler value, usize anisotropy) { rtgl_begin_errorable_operation(); struct rtgl_sampler* s = rtgl_sampler_from_handle(value); if (!s || !anisotropy || anisotropy > UINT32_MAX) { rtgl_throwf(RT_IMPROPER_USAGE, "rtSamplerSetAnisotropy received invalid arguments"); return; } s->max_anisotropy = (u32)anisotropy; }
void rtSamplerSetLod(rt_sampler value, f32 minimum, f32 maximum, f32 bias) { rtgl_begin_errorable_operation(); struct rtgl_sampler* s = rtgl_sampler_from_handle(value); if (!s || minimum > maximum) { rtgl_throwf(RT_IMPROPER_USAGE, "rtSamplerSetLod received invalid arguments"); return; } s->min_lod = minimum; s->max_lod = maximum; s->lod_bias = bias; }
