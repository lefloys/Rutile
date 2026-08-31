#include "next.h"
#include "trace.h"

RT_API_PUBLIC void rtCmdBindSampler(rt_command_buffer command_buffer, rt_location location, rt_sampler sampler) {
	rtdbg_trace_api("rtCmdBindSampler");
	rtdbg_procs.rtCmdBindSampler(command_buffer, location, sampler);
}

RT_API_PUBLIC void rtSamplerSetFilter(rt_sampler sampler, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter) {
	rtdbg_trace_api("rtSamplerSetFilter");
	rtdbg_trace_resource_detail(sampler, "filters: magnification %u; minification %u; mip %u", (unsigned)mag_filter, (unsigned)min_filter, (unsigned)mip_filter);
	rtdbg_procs.rtSamplerSetFilter(sampler, mag_filter, min_filter, mip_filter);
}

RT_API_PUBLIC void rtSamplerSetAddress(rt_sampler sampler, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w) {
	rtdbg_trace_api("rtSamplerSetAddress");
	rtdbg_trace_resource_detail(sampler, "address: U %u; V %u; W %u", (unsigned)address_u, (unsigned)address_v, (unsigned)address_w);
	rtdbg_procs.rtSamplerSetAddress(sampler, address_u, address_v, address_w);
}

RT_API_PUBLIC void rtSamplerSetAnisotropy(rt_sampler sampler, usize max_anisotropy) {
	rtdbg_trace_api("rtSamplerSetAnisotropy");
	rtdbg_trace_resource_detail(sampler, "max anisotropy %llu", (unsigned long long)max_anisotropy);
	rtdbg_procs.rtSamplerSetAnisotropy(sampler, max_anisotropy);
}

RT_API_PUBLIC void rtSamplerSetLod(rt_sampler sampler, f32 min_lod, f32 max_lod, f32 lod_bias) {
	rtdbg_trace_api("rtSamplerSetLod");
	rtdbg_trace_resource_detail(sampler, "LOD: minimum %g; maximum %g; bias %g", min_lod, max_lod, lod_bias);
	rtdbg_procs.rtSamplerSetLod(sampler, min_lod, max_lod, lod_bias);
}

RT_API_PUBLIC rt_sampler rtSamplerCreate(void) {
	rt_sampler handle = rtdbg_procs.rtSamplerCreate();
	rtdbg_trace_resource_create("rtSamplerCreate", "sampler", handle);
	return handle;
}

RT_API_PUBLIC void rtSamplerDestroy(rt_sampler sampler) {
	rtdbg_trace_resource_destroy("rtSamplerDestroy", "sampler", sampler);
	rtdbg_procs.rtSamplerDestroy(sampler);
}

