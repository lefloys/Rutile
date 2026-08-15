#include "command_buffer.h"
#include "buffer.h"
#include "framebuffer.h"
#include "graphics_program.h"
#include "logger.h"
#include "texture_view.h"

#define RTVAL_DROP(message) rtval_printf("[validation] %s, dropping call\n", message)

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

static bool rtval_command_buffer_recording(struct rtval_command_buffer* command_buffer, const char* call_name) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!state) {
		rtval_printf("[validation] %s: invalid command buffer, dropping call\n", call_name);
		return false;
	}
	if (!state->recording) {
		rtval_printf("[validation] %s: command buffer is not recording, dropping call\n", call_name);
		return false;
	}
	return true;
}

static bool rtval_command_buffer_rendering(struct rtval_command_buffer* command_buffer, const char* call_name) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_recording(command_buffer, call_name)) {
		return false;
	}
	if (!state->rendering) {
		rtval_printf("[validation] %s: command buffer has no active rendering scope, dropping call\n", call_name);
		return false;
	}
	return true;
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RT_API_PUBLIC rt_command_buffer rtCommandBufferCreate(void) {
	return rtval_command_buffer_to_handle(rtval_command_buffer_create());
}

RT_API_PUBLIC void rtCommandBufferDestroy(rt_command_buffer command_buffer) {
	rtval_command_buffer_destroy(rtval_command_buffer_from_handle(command_buffer));
}

RT_API_PUBLIC void rtCmdReset(rt_command_buffer command_buffer) {
	rtval_command_buffer_reset(rtval_command_buffer_from_handle(command_buffer));
}

RT_API_PUBLIC void rtCmdBegin(rt_command_buffer command_buffer) {
	rtval_command_buffer_begin(rtval_command_buffer_from_handle(command_buffer));
}

RT_API_PUBLIC void rtCmdWait(rt_command_buffer command_buffer, rt_timepoint timepoint) {
	rtval_command_buffer_wait(rtval_command_buffer_from_handle(command_buffer), timepoint);
}

RT_API_PUBLIC void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer) {
	rtval_command_buffer_begin_rendering(
		rtval_command_buffer_from_handle(command_buffer),
		rtval_framebuffer_from_handle(framebuffer)
	);
}

RT_API_PUBLIC void rtCmdClearColor(rt_command_buffer command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a) {
	rtval_command_buffer_clear_color(rtval_command_buffer_from_handle(command_buffer), color_index, r, g, b, a);
}

RT_API_PUBLIC void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth) {
	rtval_command_buffer_clear_depth(rtval_command_buffer_from_handle(command_buffer), depth);
}

RT_API_PUBLIC void rtCmdClearStencil(rt_command_buffer command_buffer, u32 stencil) {
	rtval_command_buffer_clear_stencil(rtval_command_buffer_from_handle(command_buffer), stencil);
}

RT_API_PUBLIC void rtCmdSetViewport(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth) {
	rtval_command_buffer_set_viewport(rtval_command_buffer_from_handle(command_buffer), x, y, width, height, min_depth, max_depth);
}

RT_API_PUBLIC void rtCmdSetScissor(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height) {
	rtval_command_buffer_set_scissor(rtval_command_buffer_from_handle(command_buffer), x, y, width, height);
}

RT_API_PUBLIC void rtCmdEndRendering(rt_command_buffer command_buffer) {
	rtval_command_buffer_end_rendering(rtval_command_buffer_from_handle(command_buffer));
}

RT_API_PUBLIC void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program) {
	rtval_command_buffer_use_graphics_program(
		rtval_command_buffer_from_handle(command_buffer),
		rtval_graphics_program_from_handle(program)
	);
}

RT_API_PUBLIC void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset, usize size) {
	rtval_command_buffer_bind_buffer(
		rtval_command_buffer_from_handle(command_buffer),
		location,
		rtval_buffer_from_handle(buffer),
		offset,
		size
	);
}

RT_API_PUBLIC void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view) {
	rtval_command_buffer_bind_texture(
		rtval_command_buffer_from_handle(command_buffer),
		location,
		rtval_texture_view_from_handle(texture_view)
	);
}

RT_API_PUBLIC void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset) {
	rtval_command_buffer_vertex_buffer(
		rtval_command_buffer_from_handle(command_buffer),
		location,
		rtval_buffer_from_handle(buffer),
		offset
	);
}

RT_API_PUBLIC void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, usize offset, enum rt_index_format format) {
	rtval_command_buffer_index_buffer(
		rtval_command_buffer_from_handle(command_buffer),
		rtval_buffer_from_handle(buffer),
		offset,
		format
	);
}

RT_API_PUBLIC void rtCmdDraw(rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex) {
	rtval_command_buffer_draw(rtval_command_buffer_from_handle(command_buffer), vertex_count, first_vertex);
}

RT_API_PUBLIC void rtCmdDrawInstanced(rt_command_buffer command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
	rtval_command_buffer_draw_instanced(rtval_command_buffer_from_handle(command_buffer), vertex_count, instance_count, first_vertex, first_instance);
}

RT_API_PUBLIC void rtCmdDrawIndexed(rt_command_buffer command_buffer, u32 index_count, u32 first_index, i32 vertex_offset) {
	rtval_command_buffer_draw_indexed(rtval_command_buffer_from_handle(command_buffer), index_count, first_index, vertex_offset);
}

RT_API_PUBLIC void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) {
	rtval_command_buffer_draw_indexed_instanced(rtval_command_buffer_from_handle(command_buffer), index_count, instance_count, first_index, vertex_offset, first_instance);
}

RT_API_PUBLIC void rtCmdEnd(rt_command_buffer command_buffer) {
	rtval_command_buffer_end(rtval_command_buffer_from_handle(command_buffer));
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtval_command_buffer* rtval_command_buffer_create(void) {
	rt_command_buffer backend = rtval_next_rtCommandBufferCreate();
	if (!backend) {
		rtval_report_error("rtCommandBufferCreate");
		return NULL;
	}
	struct rtval_command_buffer* command_buffer = rtval_handle_create(RTVAL_HANDLE_TYPE_COMMAND_BUFFER);
	if (!command_buffer) {
		rtval_next_rtCommandBufferDestroy(backend);
		return NULL;
	}
	RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer)->backend = backend;
	rtval_report_error("rtCommandBufferCreate");
	return command_buffer;
}

void rtval_command_buffer_destroy(struct rtval_command_buffer* command_buffer) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!state) {
		RTVAL_DROP("rtCommandBufferDestroy: invalid command buffer");
		return;
	}
	rtval_next_rtCommandBufferDestroy(state->backend);
	rtval_handle_destroy(command_buffer);
}

void rtval_command_buffer_reset(struct rtval_command_buffer* command_buffer) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!state || state->recording) {
		RTVAL_DROP("rtCmdReset: invalid or recording command buffer");
		return;
	}
	state->executable = false;
	state->rendering = false;
	rtval_next_rtCmdReset(state->backend);
	rtval_report_error("rtCmdReset");
}

void rtval_command_buffer_begin(struct rtval_command_buffer* command_buffer) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!state || state->recording || state->executable) {
		RTVAL_DROP("rtCmdBegin: reset command buffer required");
		return;
	}
	state->recording = true;
	rtval_next_rtCmdBegin(state->backend);
	rtval_report_error("rtCmdBegin");
}

void rtval_command_buffer_wait(struct rtval_command_buffer* command_buffer, rt_timepoint timepoint) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdWait")) {
		return;
	}
	if (state->rendering) {
		RTVAL_DROP("rtCmdWait: wait must be outside an active rendering scope");
		return;
	}
	rtval_next_rtCmdWait(state->backend, timepoint);
	rtval_report_error("rtCmdWait");
}

void rtval_command_buffer_begin_rendering(struct rtval_command_buffer* command_buffer, struct rtval_framebuffer* framebuffer) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	struct rtval_framebuffer* framebuffer_state = RTVAL_PAYLOAD(framebuffer, struct rtval_framebuffer);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdBeginRendering")) {
		return;
	}
	if (!framebuffer_state || state->rendering) {
		RTVAL_DROP("rtCmdBeginRendering: valid framebuffer and no active rendering scope required");
		return;
	}
	state->rendering = true;
	rtval_next_rtCmdBeginRendering(state->backend, framebuffer_state->backend);
	rtval_report_error("rtCmdBeginRendering");
}

void rtval_command_buffer_clear_color(struct rtval_command_buffer* command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_rendering(command_buffer, "rtCmdClearColor")) {
		return;
	}
	rtval_next_rtCmdClearColor(state->backend, color_index, r, g, b, a);
	rtval_report_error("rtCmdClearColor");
}

void rtval_command_buffer_clear_depth(struct rtval_command_buffer* command_buffer, f32 depth) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_rendering(command_buffer, "rtCmdClearDepth")) {
		return;
	}
	rtval_next_rtCmdClearDepth(state->backend, depth);
	rtval_report_error("rtCmdClearDepth");
}

void rtval_command_buffer_clear_stencil(struct rtval_command_buffer* command_buffer, u32 stencil) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_rendering(command_buffer, "rtCmdClearStencil")) {
		return;
	}
	rtval_next_rtCmdClearStencil(state->backend, stencil);
	rtval_report_error("rtCmdClearStencil");
}

void rtval_command_buffer_set_viewport(struct rtval_command_buffer* command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_rendering(command_buffer, "rtCmdSetViewport")) {
		return;
	}
	rtval_next_rtCmdSetViewport(state->backend, x, y, width, height, min_depth, max_depth);
	rtval_report_error("rtCmdSetViewport");
}

void rtval_command_buffer_set_scissor(struct rtval_command_buffer* command_buffer, u32 x, u32 y, u32 width, u32 height) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_rendering(command_buffer, "rtCmdSetScissor")) {
		return;
	}
	rtval_next_rtCmdSetScissor(state->backend, x, y, width, height);
	rtval_report_error("rtCmdSetScissor");
}

void rtval_command_buffer_end_rendering(struct rtval_command_buffer* command_buffer) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_rendering(command_buffer, "rtCmdEndRendering")) {
		return;
	}
	state->rendering = false;
	rtval_next_rtCmdEndRendering(state->backend);
	rtval_report_error("rtCmdEndRendering");
}

void rtval_command_buffer_use_graphics_program(struct rtval_command_buffer* command_buffer, struct rtval_graphics_program* program) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	struct rtval_graphics_program* program_state = RTVAL_PAYLOAD(program, struct rtval_graphics_program);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdUseGraphicsProgram")) {
		return;
	}
	if (!program_state) {
		RTVAL_DROP("rtCmdUseGraphicsProgram: invalid graphics program");
		return;
	}
	rtval_next_rtCmdUseGraphicsProgram(state->backend, program_state->backend);
	rtval_report_error("rtCmdUseGraphicsProgram");
}

void rtval_command_buffer_bind_buffer(struct rtval_command_buffer* command_buffer, rt_location location, struct rtval_buffer* buffer, usize offset, usize size) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	struct rtval_buffer* buffer_state = RTVAL_PAYLOAD(buffer, struct rtval_buffer);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdBindBuffer")) {
		return;
	}
	if (!location || !buffer_state || !size) {
		RTVAL_DROP("rtCmdBindBuffer: location, buffer, and non-zero size required");
		return;
	}
	rtval_next_rtCmdBindBuffer(state->backend, location, buffer_state->backend, offset, size);
	rtval_report_error("rtCmdBindBuffer");
}

void rtval_command_buffer_bind_texture(struct rtval_command_buffer* command_buffer, rt_location location, struct rtval_texture_view* texture_view) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	struct rtval_texture_view* texture_view_state = RTVAL_PAYLOAD(texture_view, struct rtval_texture_view);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdBindTexture")) {
		return;
	}
	if (!location || !texture_view_state) {
		RTVAL_DROP("rtCmdBindTexture: location and texture view required");
		return;
	}
	rtval_next_rtCmdBindTexture(state->backend, location, texture_view_state->backend);
	rtval_report_error("rtCmdBindTexture");
}

void rtval_command_buffer_vertex_buffer(struct rtval_command_buffer* command_buffer, rt_location location, struct rtval_buffer* buffer, usize offset) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	struct rtval_buffer* buffer_state = RTVAL_PAYLOAD(buffer, struct rtval_buffer);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdVertexBuffer")) {
		return;
	}
	if (!location || !buffer_state) {
		RTVAL_DROP("rtCmdVertexBuffer: location and buffer required");
		return;
	}
	rtval_next_rtCmdVertexBuffer(state->backend, location, buffer_state->backend, offset);
	rtval_report_error("rtCmdVertexBuffer");
}

void rtval_command_buffer_index_buffer(struct rtval_command_buffer* command_buffer, struct rtval_buffer* buffer, usize offset, enum rt_index_format format) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	struct rtval_buffer* buffer_state = RTVAL_PAYLOAD(buffer, struct rtval_buffer);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdIndexBuffer")) {
		return;
	}
	if (!buffer_state || (format != RT_INDEX_U16 && format != RT_INDEX_U32)) {
		RTVAL_DROP("rtCmdIndexBuffer: valid buffer and index format required");
		return;
	}
	rtval_next_rtCmdIndexBuffer(state->backend, buffer_state->backend, offset, format);
	rtval_report_error("rtCmdIndexBuffer");
}

void rtval_command_buffer_draw(struct rtval_command_buffer* command_buffer, u32 vertex_count, u32 first_vertex) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_rendering(command_buffer, "rtCmdDraw")) {
		return;
	}
	if (!vertex_count) {
		RTVAL_DROP("rtCmdDraw: non-zero vertex count required");
		return;
	}
	rtval_next_rtCmdDraw(state->backend, vertex_count, first_vertex);
	rtval_report_error("rtCmdDraw");
}

void rtval_command_buffer_draw_instanced(struct rtval_command_buffer* command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_rendering(command_buffer, "rtCmdDrawInstanced")) {
		return;
	}
	if (!vertex_count || !instance_count) {
		RTVAL_DROP("rtCmdDrawInstanced: non-zero vertex and instance counts required");
		return;
	}
	rtval_next_rtCmdDrawInstanced(state->backend, vertex_count, instance_count, first_vertex, first_instance);
	rtval_report_error("rtCmdDrawInstanced");
}

void rtval_command_buffer_draw_indexed(struct rtval_command_buffer* command_buffer, u32 index_count, u32 first_index, i32 vertex_offset) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_rendering(command_buffer, "rtCmdDrawIndexed")) {
		return;
	}
	if (!index_count) {
		RTVAL_DROP("rtCmdDrawIndexed: non-zero index count required");
		return;
	}
	rtval_next_rtCmdDrawIndexed(state->backend, index_count, first_index, vertex_offset);
	rtval_report_error("rtCmdDrawIndexed");
}

void rtval_command_buffer_draw_indexed_instanced(struct rtval_command_buffer* command_buffer, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_rendering(command_buffer, "rtCmdDrawIndexedInstanced")) {
		return;
	}
	if (!index_count || !instance_count) {
		RTVAL_DROP("rtCmdDrawIndexedInstanced: non-zero index and instance counts required");
		return;
	}
	rtval_next_rtCmdDrawIndexedInstanced(state->backend, index_count, instance_count, first_index, vertex_offset, first_instance);
	rtval_report_error("rtCmdDrawIndexedInstanced");
}

void rtval_command_buffer_end(struct rtval_command_buffer* command_buffer) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdEnd")) {
		return;
	}
	if (state->rendering) {
		RTVAL_DROP("rtCmdEnd: active rendering scope must end first");
		return;
	}
	state->recording = false;
	state->executable = true;
	rtval_next_rtCmdEnd(state->backend);
	rtval_report_error("rtCmdEnd");
}

#undef RTVAL_DROP
