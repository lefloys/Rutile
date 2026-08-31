#include "next.h"
#include "trace.h"
#include "resource/rasterizer.h"

RT_API_PUBLIC void rtProgramSetLayout(rt_program program, const rt_vertex_layout* layout) {
	rtdbg_rasterizer_layout(layout);
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtProgramSetLayout program #%llu inputs %llu", (unsigned long long)rtdbg_trace_handle_id(program), (unsigned long long)(layout ? layout->input_count : 0));
	rtdbg_procs.rtProgramSetLayout(program, layout);
}

RT_API_PUBLIC void rtProgramSource(rt_program program, const char* entry_point, const u08* bytes, usize byte_size) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtProgramSource program #%llu entry %s bytes %llu", (unsigned long long)rtdbg_trace_handle_id(program), entry_point ? entry_point : "", (unsigned long long)byte_size);
	rtdbg_procs.rtProgramSource(program, entry_point, bytes, byte_size);
}

RT_API_PUBLIC void rtProgramSetRasterState(rt_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtProgramSetRasterState program #%llu cull %u front-face %u fill %u", (unsigned long long)rtdbg_trace_handle_id(program), (unsigned)cull_mode, (unsigned)front_face, (unsigned)fill_mode);
	rtdbg_procs.rtProgramSetRasterState(program, cull_mode, front_face, fill_mode);
}

RT_API_PUBLIC void rtProgramSetBlendState(rt_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtProgramSetBlendState program #%llu enabled %u color %u %u %u alpha %u %u %u", (unsigned long long)rtdbg_trace_handle_id(program), enabled ? 1u : 0u, (unsigned)src_color, (unsigned)dst_color, (unsigned)color_op, (unsigned)src_alpha, (unsigned)dst_alpha, (unsigned)alpha_op);
	rtdbg_procs.rtProgramSetBlendState(program, enabled, src_color, dst_color, color_op, src_alpha, dst_alpha, alpha_op);
}

RT_API_PUBLIC void rtProgramFinalize(rt_program program) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtProgramFinalize program #%llu", (unsigned long long)rtdbg_trace_handle_id(program));
	rtdbg_procs.rtProgramFinalize(program);
}

RT_API_PUBLIC rt_location rtProgramUniformLocation(rt_program program, const char* name) {
	rtdbg_trace_api("rtProgramUniformLocation");
	return rtdbg_procs.rtProgramUniformLocation(program, name);
}

RT_API_PUBLIC rt_location rtProgramInputLocation(rt_program program, const rt_vertex_attribute* attributes, usize attribute_count) {
	rtdbg_trace_api("rtProgramInputLocation");
	return rtdbg_procs.rtProgramInputLocation(program, attributes, attribute_count);
}

RT_API_PUBLIC rt_location rtProgramOutputLocation(rt_program program, const char* name) {
	rtdbg_trace_api("rtProgramOutputLocation");
	return rtdbg_procs.rtProgramOutputLocation(program, name);
}

RT_API_PUBLIC rt_program rtProgramCreate(void) {
	rt_program handle = rtdbg_procs.rtProgramCreate();
	rtdbg_trace_resource_create("rtProgramCreate", "program", handle);
	return handle;
}

RT_API_PUBLIC void rtProgramDestroy(rt_program program) {
	rtdbg_trace_resource_destroy("rtProgramDestroy", "program", program);
	rtdbg_procs.rtProgramDestroy(program);
}

