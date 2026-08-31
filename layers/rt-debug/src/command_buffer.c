#include "next.h"
#include "trace.h"
#include "resource/rasterizer.h"

RT_API_PUBLIC void rtCommandBufferReset(rt_command_buffer command_buffer) {
	rtdbg_trace_api("rtCommandBufferReset");
	rtdbg_procs.rtCommandBufferReset(command_buffer);
}

RT_API_PUBLIC void rtCommandBufferContinue(rt_command_buffer command_buffer) {
	rtdbg_trace_api("rtCommandBufferContinue");
	rtdbg_procs.rtCommandBufferContinue(command_buffer);
}

RT_API_PUBLIC void rtCommandBufferContinueRendering(rt_command_buffer command_buffer) {
	rtdbg_trace_api("rtCommandBufferContinueRendering");
	rtdbg_procs.rtCommandBufferContinueRendering(command_buffer);
}

RT_API_PUBLIC void rtCmdExecute(rt_command_buffer command_buffer, rt_command_buffer secondary) {
	rtdbg_rasterizer_execute(command_buffer, secondary);
	rtdbg_trace_api("rtCmdExecute");
	rtdbg_procs.rtCmdExecute(command_buffer, secondary);
}

RT_API_PUBLIC void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer) {
	rtdbg_rasterizer_begin(command_buffer);
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdBeginRendering command-buffer #%llu framebuffer #%llu", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (unsigned long long)rtdbg_trace_handle_id(framebuffer));
	rtdbg_procs.rtCmdBeginRendering(command_buffer, framebuffer);
}

RT_API_PUBLIC void rtCmdClearColor(rt_command_buffer command_buffer, rt_location location, f32 r, f32 g, f32 b, f32 a) {
	rtdbg_rasterizer_clear_color(command_buffer, r, g, b, a);
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdClearColor command-buffer #%llu location %p rgba %.9g %.9g %.9g %.9g", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (void*)location, r, g, b, a);
	rtdbg_procs.rtCmdClearColor(command_buffer, location, r, g, b, a);
}

RT_API_PUBLIC void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdClearDepth command-buffer #%llu depth %.9g", (unsigned long long)rtdbg_trace_handle_id(command_buffer), depth);
	rtdbg_procs.rtCmdClearDepth(command_buffer, depth);
}

RT_API_PUBLIC void rtCmdClearStencil(rt_command_buffer command_buffer, usize stencil) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdClearStencil command-buffer #%llu stencil %llu", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (unsigned long long)stencil);
	rtdbg_procs.rtCmdClearStencil(command_buffer, stencil);
}

RT_API_PUBLIC void rtCmdClear(rt_command_buffer command_buffer, enum rt_clear_flag attachments) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdClear command-buffer #%llu attachments 0x%x", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (unsigned)attachments);
	rtdbg_procs.rtCmdClear(command_buffer, attachments);
}

RT_API_PUBLIC void rtCmdSetViewport(rt_command_buffer command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth) {
	rtdbg_rasterizer_viewport(command_buffer, width, height);
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdSetViewport command-buffer #%llu x %llu y %llu width %llu height %llu depth %.9g %.9g", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (unsigned long long)x, (unsigned long long)y, (unsigned long long)width, (unsigned long long)height, min_depth, max_depth);
	rtdbg_procs.rtCmdSetViewport(command_buffer, x, y, width, height, min_depth, max_depth);
}

RT_API_PUBLIC void rtCmdSetScissor(rt_command_buffer command_buffer, usize x, usize y, usize width, usize height) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdSetScissor command-buffer #%llu x %llu y %llu width %llu height %llu", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (unsigned long long)x, (unsigned long long)y, (unsigned long long)width, (unsigned long long)height);
	rtdbg_procs.rtCmdSetScissor(command_buffer, x, y, width, height);
}

RT_API_PUBLIC void rtCmdEndRendering(rt_command_buffer command_buffer) {
	rtdbg_trace_api("rtCmdEndRendering");
	rtdbg_procs.rtCmdEndRendering(command_buffer);
}

RT_API_PUBLIC void rtCmdUseProgram(rt_command_buffer command_buffer, rt_program program) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdUseProgram command-buffer #%llu program #%llu", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (unsigned long long)rtdbg_trace_handle_id(program));
	rtdbg_procs.rtCmdUseProgram(command_buffer, program);
}

RT_API_PUBLIC void rtCmdUniformData(rt_command_buffer command_buffer, rt_location location, const u08* data, usize size) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdUniformData command-buffer #%llu location %p bytes %llu", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (void*)location, (unsigned long long)size);
	rtdbg_procs.rtCmdUniformData(command_buffer, location, data, size);
}

RT_API_PUBLIC void rtCmdStorageData(rt_command_buffer command_buffer, rt_location location, const u08* data, usize size) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdStorageData command-buffer #%llu location %p bytes %llu", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (void*)location, (unsigned long long)size);
	rtdbg_procs.rtCmdStorageData(command_buffer, location, data, size);
}

RT_API_PUBLIC rt_command_buffer rtCommandBufferCreate(void) {
	rt_command_buffer handle = rtdbg_procs.rtCommandBufferCreate();
	rtdbg_trace_resource_create("rtCommandBufferCreate", "command-buffer", handle);
	return handle;
}

RT_API_PUBLIC void rtCommandBufferDestroy(rt_command_buffer command_buffer) {
	rtdbg_trace_resource_destroy("rtCommandBufferDestroy", "command-buffer", command_buffer);
	rtdbg_procs.rtCommandBufferDestroy(command_buffer);
}

RT_API_PUBLIC void rtCommandBufferBegin(rt_command_buffer command_buffer) {
	rtdbg_trace_command_buffer("rtCommandBufferBegin", command_buffer);
	rtdbg_procs.rtCommandBufferBegin(command_buffer);
}

RT_API_PUBLIC void rtCommandBufferEnd(rt_command_buffer command_buffer) {
	rtdbg_trace_command_buffer("rtCommandBufferEnd", command_buffer);
	rtdbg_procs.rtCommandBufferEnd(command_buffer);
}

RT_API_PUBLIC void rtCmdDraw(rt_command_buffer command_buffer, usize vertex_count, usize first_vertex) {
	rtdbg_rasterizer_draw(command_buffer, vertex_count, first_vertex);
	rtdbg_trace_draw("rtCmdDraw", command_buffer, vertex_count, 1);
	rtdbg_procs.rtCmdDraw(command_buffer, vertex_count, first_vertex);
}

RT_API_PUBLIC void rtCmdDrawInstanced(rt_command_buffer command_buffer, usize vertex_count, usize instance_count, usize first_vertex, usize first_instance) {
	rtdbg_trace_draw("rtCmdDrawInstanced", command_buffer, vertex_count, instance_count);
	rtdbg_procs.rtCmdDrawInstanced(command_buffer, vertex_count, instance_count, first_vertex, first_instance);
}

RT_API_PUBLIC void rtCmdDrawIndexed(rt_command_buffer command_buffer, usize index_count, usize first_index, usize vertex_offset) {
	rtdbg_trace_draw("rtCmdDrawIndexed", command_buffer, index_count, 1);
	rtdbg_procs.rtCmdDrawIndexed(command_buffer, index_count, first_index, vertex_offset);
}

RT_API_PUBLIC void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, usize index_count, usize instance_count, usize first_index, usize vertex_offset, usize first_instance) {
	rtdbg_trace_draw("rtCmdDrawIndexedInstanced", command_buffer, index_count, instance_count);
	rtdbg_procs.rtCmdDrawIndexedInstanced(command_buffer, index_count, instance_count, first_index, vertex_offset, first_instance);
}

