#include "procs.h"

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RT_API_PUBLIC rt_graphics_program rtGraphicsProgramCreate(void) {
	return rtlog_rtGraphicsProgramCreate();
}
RT_API_PUBLIC void rtGraphicsProgramDestroy(rt_graphics_program program) {
	rtlog_rtGraphicsProgramDestroy(program);
}
RT_API_PUBLIC void rtGraphicsProgramLayout(rt_graphics_program program, const rt_vertex_layout* layout) {
	rtlog_rtGraphicsProgramLayout(program, layout);
}
RT_API_PUBLIC void rtGraphicsProgramSource(rt_graphics_program program, const void* data, usize size) {
	rtlog_rtGraphicsProgramSource(program, data, size);
}
RT_API_PUBLIC void rtGraphicsProgramRasterState(rt_graphics_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) {
	rtlog_rtGraphicsProgramRasterState(program, cull_mode, front_face, fill_mode);
}
RT_API_PUBLIC void rtGraphicsProgramBlendState(rt_graphics_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op) {
	rtlog_rtGraphicsProgramBlendState(program, enabled, src_color, dst_color, color_op, src_alpha, dst_alpha, alpha_op);
}
RT_API_PUBLIC void rtGraphicsProgramFinalize(rt_graphics_program program) {
	rtlog_rtGraphicsProgramFinalize(program);
}
RT_API_PUBLIC rt_location rtGraphicsProgramLocation(rt_graphics_program program, const char* name) {
	return rtlog_rtGraphicsProgramLocation(program, name);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

rt_graphics_program rtlog_rtGraphicsProgramCreate(void) {
	u64 start_ns = rtlog_now_ns();
	rtlog_printf("rtGraphicsProgramCreate()\n");
	rt_graphics_program result = next_rtGraphicsProgramCreate();
	rtlog_printf("rtGraphicsProgramCreate -> %s [%s]\n", rtlog_pointer(result), rtlog_elapsed(start_ns));
	rtlog_error("rtGraphicsProgramCreate");
	return result;
}

void rtlog_rtGraphicsProgramDestroy(rt_graphics_program program) {
	u64 start_ns = rtlog_now_ns();
	rtlog_printf("rtGraphicsProgramDestroy(program=%s)\n", rtlog_pointer(program));
	next_rtGraphicsProgramDestroy(program);
	rtlog_printf("rtGraphicsProgramDestroy completed in %s\n", rtlog_elapsed(start_ns));
	rtlog_error("rtGraphicsProgramDestroy");
}

void rtlog_rtGraphicsProgramLayout(rt_graphics_program program, const rt_vertex_layout* layout) {
	u64 start_ns = rtlog_now_ns();
	rtlog_printf("rtGraphicsProgramLayout(program=%s, layout=%s)\n", rtlog_pointer(program), rtlog_pointer(layout));
	next_rtGraphicsProgramLayout(program, layout);
	rtlog_printf("rtGraphicsProgramLayout completed in %s\n", rtlog_elapsed(start_ns));
	rtlog_error("rtGraphicsProgramLayout");
}

void rtlog_rtGraphicsProgramSource(rt_graphics_program program, const void* data, usize size) {
	u64 start_ns = rtlog_now_ns();
	rtlog_printf("rtGraphicsProgramSource(program=%s, data=%s, size=%llu)\n", rtlog_pointer(program), rtlog_pointer(data), (u64)size);
	next_rtGraphicsProgramSource(program, data, size);
	rtlog_printf("rtGraphicsProgramSource completed in %s\n", rtlog_elapsed(start_ns));
	rtlog_error("rtGraphicsProgramSource");
}

void rtlog_rtGraphicsProgramRasterState(rt_graphics_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) {
	u64 start_ns = rtlog_now_ns();
	rtlog_printf("rtGraphicsProgramRasterState(program=%s, cull=%d, front=%d, fill=%d)\n", rtlog_pointer(program), (int)cull_mode, (int)front_face, (int)fill_mode);
	next_rtGraphicsProgramRasterState(program, cull_mode, front_face, fill_mode);
	rtlog_printf("rtGraphicsProgramRasterState completed in %s\n", rtlog_elapsed(start_ns));
	rtlog_error("rtGraphicsProgramRasterState");
}

void rtlog_rtGraphicsProgramBlendState(
	rt_graphics_program program,
	bool enabled,
	enum rt_blend_factor src_color,
	enum rt_blend_factor dst_color,
	enum rt_blend_op color_op,
	enum rt_blend_factor src_alpha,
	enum rt_blend_factor dst_alpha,
	enum rt_blend_op alpha_op
) {
	u64 start_ns = rtlog_now_ns();
	rtlog_printf("rtGraphicsProgramBlendState(program=%s, enabled=%d)\n", rtlog_pointer(program), enabled ? 1 : 0);
	next_rtGraphicsProgramBlendState(program, enabled, src_color, dst_color, color_op, src_alpha, dst_alpha, alpha_op);
	rtlog_printf("rtGraphicsProgramBlendState completed in %s\n", rtlog_elapsed(start_ns));
	rtlog_error("rtGraphicsProgramBlendState");
}

void rtlog_rtGraphicsProgramFinalize(rt_graphics_program program) {
	u64 start_ns = rtlog_now_ns();
	rtlog_printf("rtGraphicsProgramFinalize(program=%s)\n", rtlog_pointer(program));
	next_rtGraphicsProgramFinalize(program);
	rtlog_printf("rtGraphicsProgramFinalize completed in %s\n", rtlog_elapsed(start_ns));
	rtlog_error("rtGraphicsProgramFinalize");
}

rt_location rtlog_rtGraphicsProgramLocation(rt_graphics_program program, const char* name) {
	u64 start_ns = rtlog_now_ns();
	rtlog_printf("rtGraphicsProgramLocation(program=%s, name=\"%s\")\n", rtlog_pointer(program), name ? name : "<null>");
	rt_location result = next_rtGraphicsProgramLocation(program, name);
	rtlog_printf("rtGraphicsProgramLocation -> %s [%s]\n", rtlog_pointer(result), rtlog_elapsed(start_ns));
	rtlog_error("rtGraphicsProgramLocation");
	return result;
}
