#include "sampler.h"
#include "logger.h"

#define RTVAL_DROP(message) rtval_fail(message)

RT_API_PUBLIC rt_sampler rtSamplerCreate(void) {
	rt_sampler backend = rtval_next_rtSamplerCreate();
	if (!backend) {
		rtval_report_error("rtSamplerCreate");
		return NULL;
	}
	struct rtval_sampler* handle = rtval_handle_create(RTVAL_HANDLE_TYPE_SAMPLER);
	if (!handle) {
		rtval_next_rtSamplerDestroy(backend);
		return NULL;
	}
	RTVAL_PAYLOAD(handle, struct rtval_sampler)->backend = backend;
	return rtval_sampler_to_handle(handle);
}
RT_API_PUBLIC void rtSamplerDestroy(rt_sampler sampler) {
	struct rtval_sampler* state = RTVAL_PAYLOAD(sampler, struct rtval_sampler);
	if (!state) {
		RTVAL_DROP("rtSamplerDestroy: invalid sampler");
		return;
	}
	rtval_next_rtSamplerDestroy(state->backend);
	if (rtval_report_error("rtSamplerDestroy")) {
		rtval_handle_destroy(sampler);
	}
}
RT_API_PUBLIC void rtSamplerSetFilter(rt_sampler sampler, enum rt_filter mag, enum rt_filter min, enum rt_mip_filter mip) {
	struct rtval_sampler* state = RTVAL_PAYLOAD(sampler, struct rtval_sampler);
	if (!state) {
		RTVAL_DROP("rtSamplerSetFilter: invalid sampler");
		return;
	}
	if ((mag != RT_FILTER_NEAREST && mag != RT_FILTER_LINEAR) || (min != RT_FILTER_NEAREST && min != RT_FILTER_LINEAR) || mip < RT_MIP_FILTER_NONE || mip > RT_MIP_FILTER_LINEAR) {
		RTVAL_DROP("rtSamplerSetFilter: valid filter modes required");
		return;
	}
	rtval_next_rtSamplerSetFilter(state->backend, mag, min, mip);
	rtval_report_error("rtSamplerSetFilter");
}
RT_API_PUBLIC void rtSamplerSetAddress(rt_sampler sampler, enum rt_address_mode u, enum rt_address_mode v, enum rt_address_mode w) {
	struct rtval_sampler* state = RTVAL_PAYLOAD(sampler, struct rtval_sampler);
	if (!state) {
		RTVAL_DROP("rtSamplerSetAddress: invalid sampler");
		return;
	}
	if (u < RT_ADDRESS_CLAMP || u > RT_ADDRESS_MIRROR || v < RT_ADDRESS_CLAMP || v > RT_ADDRESS_MIRROR || w < RT_ADDRESS_CLAMP || w > RT_ADDRESS_MIRROR) {
		RTVAL_DROP("rtSamplerSetAddress: valid address modes required");
		return;
	}
	rtval_next_rtSamplerSetAddress(state->backend, u, v, w);
	rtval_report_error("rtSamplerSetAddress");
}
RT_API_PUBLIC void rtSamplerSetAnisotropy(rt_sampler sampler, usize max) {
	struct rtval_sampler* state = RTVAL_PAYLOAD(sampler, struct rtval_sampler);
	if (!state) {
		RTVAL_DROP("rtSamplerSetAnisotropy: invalid sampler");
		return;
	}
	rtval_next_rtSamplerSetAnisotropy(state->backend, max);
	rtval_report_error("rtSamplerSetAnisotropy");
}
RT_API_PUBLIC void rtSamplerSetLod(rt_sampler sampler, f32 min, f32 max, f32 bias) {
	struct rtval_sampler* state = RTVAL_PAYLOAD(sampler, struct rtval_sampler);
	if (!state) {
		RTVAL_DROP("rtSamplerSetLod: invalid sampler");
		return;
	}
	if (min > max) {
		RTVAL_DROP("rtSamplerSetLod: ordered LOD range required");
		return;
	}
	rtval_next_rtSamplerSetLod(state->backend, min, max, bias);
	rtval_report_error("rtSamplerSetLod");
}

#undef RTVAL_DROP
