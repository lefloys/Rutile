#include "command_buffer.h"
#include "buffer.h"
#include "framebuffer.h"
#include "program.h"
#include "logger.h"
#include "texture.h"
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
	if (!state->rendering && !state->continuation_rendering) {
		rtval_printf("[validation] %s: command buffer has no active rendering scope, dropping call\n", call_name);
		return false;
	}
	return true;
}

static bool rtval_command_buffer_outside_rendering(struct rtval_command_buffer* command_buffer, const char* call_name) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_recording(command_buffer, call_name)) {
		return false;
	}
	if (state->rendering || state->continuation_rendering) {
		rtval_printf("[validation] %s: command must be outside an active rendering scope, dropping call\n", call_name);
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

RT_API_PUBLIC void rtCommandBufferReset(rt_command_buffer command_buffer) {
	rtval_command_buffer_reset(rtval_command_buffer_from_handle(command_buffer));
}

RT_API_PUBLIC void rtCommandBufferBegin(rt_command_buffer command_buffer) {
	rtval_command_buffer_begin(rtval_command_buffer_from_handle(command_buffer));
}

RT_API_PUBLIC void rtCommandBufferContinue(rt_command_buffer command_buffer) {
	rtval_command_buffer_continue(rtval_command_buffer_from_handle(command_buffer), false);
}

RT_API_PUBLIC void rtCommandBufferContinueRendering(rt_command_buffer command_buffer) {
	rtval_command_buffer_continue(rtval_command_buffer_from_handle(command_buffer), true);
}

RT_API_PUBLIC void rtCommandBufferEnd(rt_command_buffer command_buffer) {
	rtval_command_buffer_end(rtval_command_buffer_from_handle(command_buffer));
}

RT_API_PUBLIC void rtCmdExecute(rt_command_buffer command_buffer, rt_command_buffer secondary) {
	rtval_command_buffer_execute(
		rtval_command_buffer_from_handle(command_buffer),
		rtval_command_buffer_from_handle(secondary)
	);
}

RT_API_PUBLIC void rtCmdBufferData(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, const u08* data) {
	struct rtval_command_buffer* command_handle = rtval_command_buffer_from_handle(command_buffer);
	struct rtval_command_buffer* command_state = RTVAL_PAYLOAD(command_handle, struct rtval_command_buffer);
	struct rtval_buffer* buffer_handle = rtval_buffer_from_handle(buffer);
	struct rtval_buffer* buffer_state = RTVAL_PAYLOAD(buffer_handle, struct rtval_buffer);
	if (!rtval_command_buffer_outside_rendering(command_handle, "rtCmdBufferData") || !buffer_state || (range.size && !data)) {
		if (!buffer_state || (range.size && !data)) RTVAL_DROP("rtCmdBufferData: buffer and data required");
		return;
	}
	rtval_next_rtCmdBufferData(command_state->backend, buffer_state->backend, range, data);
	rtval_report_error("rtCmdBufferData");
}

RT_API_PUBLIC void rtCmdBufferCopy(rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_buffer dst, rt_buffer_range dst_range) {
	struct rtval_command_buffer* command_handle = rtval_command_buffer_from_handle(command_buffer);
	struct rtval_command_buffer* command_state = RTVAL_PAYLOAD(command_handle, struct rtval_command_buffer);
	struct rtval_buffer* src_handle = rtval_buffer_from_handle(src);
	struct rtval_buffer* src_state = RTVAL_PAYLOAD(src_handle, struct rtval_buffer);
	struct rtval_buffer* dst_handle = rtval_buffer_from_handle(dst);
	struct rtval_buffer* dst_state = RTVAL_PAYLOAD(dst_handle, struct rtval_buffer);
	if (!rtval_command_buffer_outside_rendering(command_handle, "rtCmdBufferCopy") || !src_state || !dst_state) {
		if (!src_state || !dst_state) RTVAL_DROP("rtCmdBufferCopy: source and destination buffers required");
		return;
	}
	rtval_next_rtCmdBufferCopy(command_state->backend, src_state->backend, src_range, dst_state->backend, dst_range);
	rtval_report_error("rtCmdBufferCopy");
}

RT_API_PUBLIC void rtCmdBufferCopyToTexture(rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_texture dst, rt_texture_range dst_range) {
	struct rtval_command_buffer* command_handle = rtval_command_buffer_from_handle(command_buffer);
	struct rtval_command_buffer* command_state = RTVAL_PAYLOAD(command_handle, struct rtval_command_buffer);
	struct rtval_buffer* src_handle = rtval_buffer_from_handle(src);
	struct rtval_buffer* src_state = RTVAL_PAYLOAD(src_handle, struct rtval_buffer);
	struct rtval_texture* dst_handle = rtval_texture_from_handle(dst);
	struct rtval_texture* dst_state = RTVAL_PAYLOAD(dst_handle, struct rtval_texture);
	if (!rtval_command_buffer_outside_rendering(command_handle, "rtCmdBufferCopyToTexture") || !src_state || !dst_state) {
		if (!src_state || !dst_state) RTVAL_DROP("rtCmdBufferCopyToTexture: source buffer and destination texture required");
		return;
	}
	rtval_next_rtCmdBufferCopyToTexture(command_state->backend, src_state->backend, src_range, dst_state->backend, dst_range);
	rtval_report_error("rtCmdBufferCopyToTexture");
}

RT_API_PUBLIC void rtCmdBufferBarrier(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, rt_access src, rt_access dst) {
	struct rtval_command_buffer* command_handle = rtval_command_buffer_from_handle(command_buffer);
	struct rtval_command_buffer* command_state = RTVAL_PAYLOAD(command_handle, struct rtval_command_buffer);
	struct rtval_buffer* buffer_handle = rtval_buffer_from_handle(buffer);
	struct rtval_buffer* buffer_state = RTVAL_PAYLOAD(buffer_handle, struct rtval_buffer);
	if (!rtval_command_buffer_outside_rendering(command_handle, "rtCmdBufferBarrier") || !buffer_state) {
		if (!buffer_state) RTVAL_DROP("rtCmdBufferBarrier: buffer required");
		return;
	}
	rtval_next_rtCmdBufferBarrier(command_state->backend, buffer_state->backend, range, src, dst);
	rtval_report_error("rtCmdBufferBarrier");
}

RT_API_PUBLIC void rtCmdTextureCopy(rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_texture dst, rt_texture_range dst_range) {
	struct rtval_command_buffer* command_handle = rtval_command_buffer_from_handle(command_buffer);
	struct rtval_command_buffer* command_state = RTVAL_PAYLOAD(command_handle, struct rtval_command_buffer);
	struct rtval_texture* src_handle = rtval_texture_from_handle(src);
	struct rtval_texture* src_state = RTVAL_PAYLOAD(src_handle, struct rtval_texture);
	struct rtval_texture* dst_handle = rtval_texture_from_handle(dst);
	struct rtval_texture* dst_state = RTVAL_PAYLOAD(dst_handle, struct rtval_texture);
	if (!rtval_command_buffer_outside_rendering(command_handle, "rtCmdTextureCopy") || !src_state || !dst_state) {
		if (!src_state || !dst_state) RTVAL_DROP("rtCmdTextureCopy: source and destination textures required");
		return;
	}
	rtval_next_rtCmdTextureCopy(command_state->backend, src_state->backend, src_range, dst_state->backend, dst_range);
	rtval_report_error("rtCmdTextureCopy");
}

RT_API_PUBLIC void rtCmdTextureData(rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, const u08* data) {
	struct rtval_command_buffer* command_handle = rtval_command_buffer_from_handle(command_buffer);
	struct rtval_command_buffer* command_state = RTVAL_PAYLOAD(command_handle, struct rtval_command_buffer);
	struct rtval_texture* texture_handle = rtval_texture_from_handle(texture);
	struct rtval_texture* texture_state = RTVAL_PAYLOAD(texture_handle, struct rtval_texture);
	if (!rtval_command_buffer_outside_rendering(command_handle, "rtCmdTextureData") || !texture_state || !data) {
		if (!texture_state || !data) RTVAL_DROP("rtCmdTextureData: texture and data required");
		return;
	}
	rtval_next_rtCmdTextureData(command_state->backend, texture_state->backend, range, data);
	rtval_report_error("rtCmdTextureData");
}

RT_API_PUBLIC void rtCmdTextureCopyToBuffer(rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_buffer dst, rt_buffer_range dst_range) {
	struct rtval_command_buffer* command_handle = rtval_command_buffer_from_handle(command_buffer);
	struct rtval_command_buffer* command_state = RTVAL_PAYLOAD(command_handle, struct rtval_command_buffer);
	struct rtval_texture* src_handle = rtval_texture_from_handle(src);
	struct rtval_texture* src_state = RTVAL_PAYLOAD(src_handle, struct rtval_texture);
	struct rtval_buffer* dst_handle = rtval_buffer_from_handle(dst);
	struct rtval_buffer* dst_state = RTVAL_PAYLOAD(dst_handle, struct rtval_buffer);
	if (!rtval_command_buffer_outside_rendering(command_handle, "rtCmdTextureCopyToBuffer") || !src_state || !dst_state) {
		if (!src_state || !dst_state) RTVAL_DROP("rtCmdTextureCopyToBuffer: source texture and destination buffer required");
		return;
	}
	rtval_next_rtCmdTextureCopyToBuffer(command_state->backend, src_state->backend, src_range, dst_state->backend, dst_range);
	rtval_report_error("rtCmdTextureCopyToBuffer");
}

RT_API_PUBLIC void rtCmdTextureBarrier(rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, rt_access src, rt_access dst) {
	struct rtval_command_buffer* command_handle = rtval_command_buffer_from_handle(command_buffer);
	struct rtval_command_buffer* command_state = RTVAL_PAYLOAD(command_handle, struct rtval_command_buffer);
	struct rtval_texture* texture_handle = rtval_texture_from_handle(texture);
	struct rtval_texture* texture_state = RTVAL_PAYLOAD(texture_handle, struct rtval_texture);
	if (!rtval_command_buffer_outside_rendering(command_handle, "rtCmdTextureBarrier") || !texture_state) {
		if (!texture_state) RTVAL_DROP("rtCmdTextureBarrier: texture required");
		return;
	}
	rtval_next_rtCmdTextureBarrier(command_state->backend, texture_state->backend, range, src, dst);
	rtval_report_error("rtCmdTextureBarrier");
}

RT_API_PUBLIC void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer) {
	rtval_command_buffer_begin_rendering(
		rtval_command_buffer_from_handle(command_buffer),
		rtval_framebuffer_from_handle(framebuffer)
	);
}

RT_API_PUBLIC void rtCmdClearColor(rt_command_buffer command_buffer, rt_location location, f32 r, f32 g, f32 b, f32 a) {
	rtval_command_buffer_clear_color(rtval_command_buffer_from_handle(command_buffer), location, r, g, b, a);
}

RT_API_PUBLIC void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth) {
	rtval_command_buffer_clear_depth(rtval_command_buffer_from_handle(command_buffer), depth);
}

RT_API_PUBLIC void rtCmdClearStencil(rt_command_buffer command_buffer, usize stencil) {
	rtval_command_buffer_clear_stencil(rtval_command_buffer_from_handle(command_buffer), stencil);
}

RT_API_PUBLIC void rtCmdClear(rt_command_buffer command_buffer, enum rt_clear_flag attachments) {
	rtval_command_buffer_clear(rtval_command_buffer_from_handle(command_buffer), attachments);
}

RT_API_PUBLIC void rtCmdSetViewport(rt_command_buffer command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth) {
	rtval_command_buffer_set_viewport(rtval_command_buffer_from_handle(command_buffer), x, y, width, height, min_depth, max_depth);
}

RT_API_PUBLIC void rtCmdSetScissor(rt_command_buffer command_buffer, usize x, usize y, usize width, usize height) {
	rtval_command_buffer_set_scissor(rtval_command_buffer_from_handle(command_buffer), x, y, width, height);
}

RT_API_PUBLIC void rtCmdEndRendering(rt_command_buffer command_buffer) {
	rtval_command_buffer_end_rendering(rtval_command_buffer_from_handle(command_buffer));
}

RT_API_PUBLIC void rtCmdUseProgram(rt_command_buffer command_buffer, rt_program program) {
	rtval_command_buffer_use_program(
		rtval_command_buffer_from_handle(command_buffer),
		rtval_program_from_handle(program)
	);
}

RT_API_PUBLIC void rtCmdUniformData(rt_command_buffer command_buffer, rt_location location, const u08* data, usize size) {
	rtval_command_buffer_uniform_data(rtval_command_buffer_from_handle(command_buffer), location, data, size);
}

RT_API_PUBLIC void rtCmdStorageData(rt_command_buffer command_buffer, rt_location location, const u08* data, usize size) {
	rtval_command_buffer_storage_data(rtval_command_buffer_from_handle(command_buffer), location, data, size);
}

RT_API_PUBLIC void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range) {
	rtval_command_buffer_bind_buffer(
		rtval_command_buffer_from_handle(command_buffer),
		location,
		rtval_buffer_from_handle(buffer),
		range
	);
}

RT_API_PUBLIC void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view) {
	rtval_command_buffer_bind_texture(
		rtval_command_buffer_from_handle(command_buffer),
		location,
		rtval_texture_view_from_handle(texture_view)
	);
}

RT_API_PUBLIC void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range) {
	rtval_command_buffer_vertex_buffer(
		rtval_command_buffer_from_handle(command_buffer),
		location,
		rtval_buffer_from_handle(buffer),
		range
	);
}

RT_API_PUBLIC void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, enum rt_index_format format) {
	rtval_command_buffer_index_buffer(
		rtval_command_buffer_from_handle(command_buffer),
		rtval_buffer_from_handle(buffer),
		range,
		format
	);
}

RT_API_PUBLIC void rtCmdDraw(rt_command_buffer command_buffer, usize vertex_count, usize first_vertex) {
	rtval_command_buffer_draw(rtval_command_buffer_from_handle(command_buffer), vertex_count, first_vertex);
}

RT_API_PUBLIC void rtCmdDrawInstanced(rt_command_buffer command_buffer, usize vertex_count, usize instance_count, usize first_vertex, usize first_instance) {
	rtval_command_buffer_draw_instanced(rtval_command_buffer_from_handle(command_buffer), vertex_count, instance_count, first_vertex, first_instance);
}

RT_API_PUBLIC void rtCmdDrawIndexed(rt_command_buffer command_buffer, usize index_count, usize first_index, usize vertex_offset) {
	rtval_command_buffer_draw_indexed(rtval_command_buffer_from_handle(command_buffer), index_count, first_index, vertex_offset);
}

RT_API_PUBLIC void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, usize index_count, usize instance_count, usize first_index, usize vertex_offset, usize first_instance) {
	rtval_command_buffer_draw_indexed_instanced(rtval_command_buffer_from_handle(command_buffer), index_count, instance_count, first_index, vertex_offset, first_instance);
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
		RTVAL_DROP("rtCommandBufferReset: invalid or recording command buffer");
		return;
	}
	state->executable = false;
	state->rendering = false;
	state->continuation = false;
	state->continuation_rendering = false;
	rtval_next_rtCommandBufferReset(state->backend);
	rtval_report_error("rtCommandBufferReset");
}

void rtval_command_buffer_begin(struct rtval_command_buffer* command_buffer) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!state || state->recording || state->executable) {
		RTVAL_DROP("rtCommandBufferBegin: reset command buffer required");
		return;
	}
	state->recording = true;
	state->continuation = false;
	state->continuation_rendering = false;
	rtval_next_rtCommandBufferBegin(state->backend);
	rtval_report_error("rtCommandBufferBegin");
}

void rtval_command_buffer_continue(struct rtval_command_buffer* command_buffer, bool rendering) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!state || state->recording || state->executable) {
		RTVAL_DROP("rtCommandBufferContinue: reset command buffer required");
		return;
	}
	state->recording = true;
	state->continuation = true;
	state->continuation_rendering = rendering;
	if (rendering) {
		rtval_next_rtCommandBufferContinueRendering(state->backend);
		rtval_report_error("rtCommandBufferContinueRendering");
	} else {
		rtval_next_rtCommandBufferContinue(state->backend);
		rtval_report_error("rtCommandBufferContinue");
	}
}

void rtval_command_buffer_execute(struct rtval_command_buffer* command_buffer, struct rtval_command_buffer* secondary) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	struct rtval_command_buffer* secondary_state = RTVAL_PAYLOAD(secondary, struct rtval_command_buffer);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdExecute")) {
		return;
	}
	if (!secondary_state || !secondary_state->continuation || !secondary_state->executable || secondary_state->recording) {
		RTVAL_DROP("rtCmdExecute: completed continuation command buffer required");
		return;
	}
	if (secondary_state->continuation_rendering != (state->rendering || state->continuation_rendering)) {
		RTVAL_DROP("rtCmdExecute: continuation rendering scope does not match parent");
		return;
	}
	rtval_next_rtCmdExecute(state->backend, secondary_state->backend);
	rtval_report_error("rtCmdExecute");
}

void rtval_command_buffer_begin_rendering(struct rtval_command_buffer* command_buffer, struct rtval_framebuffer* framebuffer) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	struct rtval_framebuffer* framebuffer_state = RTVAL_PAYLOAD(framebuffer, struct rtval_framebuffer);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdBeginRendering")) {
		return;
	}
	if (!framebuffer_state || state->rendering || state->continuation) {
		RTVAL_DROP("rtCmdBeginRendering: direct command buffer, valid framebuffer, and no active rendering scope required");
		return;
	}
	state->rendering = true;
	rtval_next_rtCmdBeginRendering(state->backend, framebuffer_state->backend);
	rtval_report_error("rtCmdBeginRendering");
}

void rtval_command_buffer_clear_color(struct rtval_command_buffer* command_buffer, rt_location location, f32 r, f32 g, f32 b, f32 a) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_rendering(command_buffer, "rtCmdClearColor")) {
		return;
	}
	rtval_next_rtCmdClearColor(state->backend, location, r, g, b, a);
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

void rtval_command_buffer_clear_stencil(struct rtval_command_buffer* command_buffer, usize stencil) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_rendering(command_buffer, "rtCmdClearStencil")) {
		return;
	}
	rtval_next_rtCmdClearStencil(state->backend, stencil);
	rtval_report_error("rtCmdClearStencil");
}

void rtval_command_buffer_clear(struct rtval_command_buffer* command_buffer, enum rt_clear_flag attachments) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_rendering(command_buffer, "rtCmdClear")) {
		return;
	}
	if (!(attachments & (RT_CLEAR_COLOR | RT_CLEAR_DEPTH | RT_CLEAR_STENCIL))) {
		RTVAL_DROP("rtCmdClear: at least one attachment required");
		return;
	}
	rtval_next_rtCmdClear(state->backend, attachments);
	rtval_report_error("rtCmdClear");
}

void rtval_command_buffer_set_viewport(struct rtval_command_buffer* command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_rendering(command_buffer, "rtCmdSetViewport")) {
		return;
	}
	rtval_next_rtCmdSetViewport(state->backend, x, y, width, height, min_depth, max_depth);
	rtval_report_error("rtCmdSetViewport");
}

void rtval_command_buffer_set_scissor(struct rtval_command_buffer* command_buffer, usize x, usize y, usize width, usize height) {
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
	if (state->continuation) {
		RTVAL_DROP("rtCmdEndRendering: rendering continuations cannot end their parent scope");
		return;
	}
	state->rendering = false;
	rtval_next_rtCmdEndRendering(state->backend);
	rtval_report_error("rtCmdEndRendering");
}

void rtval_command_buffer_use_program(struct rtval_command_buffer* command_buffer, struct rtval_program* program) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	struct rtval_program* program_state = RTVAL_PAYLOAD(program, struct rtval_program);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdUseProgram")) {
		return;
	}
	if (!program_state) {
		RTVAL_DROP("rtCmdUseProgram: invalid program");
		return;
	}
	rtval_next_rtCmdUseProgram(state->backend, program_state->backend);
	rtval_report_error("rtCmdUseProgram");
}

void rtval_command_buffer_uniform_data(struct rtval_command_buffer* command_buffer, rt_location location, const u08* data, usize size) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdUniformData")) {
		return;
	}
	if (!location || !data || !size) {
		RTVAL_DROP("rtCmdUniformData: location and non-empty data required");
		return;
	}
	rtval_next_rtCmdUniformData(state->backend, location, data, size);
	rtval_report_error("rtCmdUniformData");
}

void rtval_command_buffer_storage_data(struct rtval_command_buffer* command_buffer, rt_location location, const u08* data, usize size) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdStorageData")) {
		return;
	}
	if (!location || !data || !size) {
		RTVAL_DROP("rtCmdStorageData: location and non-empty data required");
		return;
	}
	rtval_next_rtCmdStorageData(state->backend, location, data, size);
	rtval_report_error("rtCmdStorageData");
}

void rtval_command_buffer_bind_buffer(struct rtval_command_buffer* command_buffer, rt_location location, struct rtval_buffer* buffer, rt_buffer_range range) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	struct rtval_buffer* buffer_state = RTVAL_PAYLOAD(buffer, struct rtval_buffer);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdBindBuffer")) {
		return;
	}
	if (!location || !buffer_state || !range.size) {
		RTVAL_DROP("rtCmdBindBuffer: location, buffer, and non-zero range required");
		return;
	}
	rtval_next_rtCmdBindBuffer(state->backend, location, buffer_state->backend, range);
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

void rtval_command_buffer_vertex_buffer(struct rtval_command_buffer* command_buffer, rt_location location, struct rtval_buffer* buffer, rt_buffer_range range) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	struct rtval_buffer* buffer_state = RTVAL_PAYLOAD(buffer, struct rtval_buffer);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdVertexBuffer")) {
		return;
	}
	if (!location || !buffer_state) {
		RTVAL_DROP("rtCmdVertexBuffer: location and buffer required");
		return;
	}
	if (!range.size) {
		RTVAL_DROP("rtCmdVertexBuffer: non-zero range required");
		return;
	}
	rtval_next_rtCmdVertexBuffer(state->backend, location, buffer_state->backend, range);
	rtval_report_error("rtCmdVertexBuffer");
}

void rtval_command_buffer_index_buffer(struct rtval_command_buffer* command_buffer, struct rtval_buffer* buffer, rt_buffer_range range, enum rt_index_format format) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	struct rtval_buffer* buffer_state = RTVAL_PAYLOAD(buffer, struct rtval_buffer);
	if (!rtval_command_buffer_recording(command_buffer, "rtCmdIndexBuffer")) {
		return;
	}
	if (!buffer_state || !range.size || (format != RT_INDEX_U16 && format != RT_INDEX_U32)) {
		RTVAL_DROP("rtCmdIndexBuffer: valid buffer, non-zero range, and index format required");
		return;
	}
	rtval_next_rtCmdIndexBuffer(state->backend, buffer_state->backend, range, format);
	rtval_report_error("rtCmdIndexBuffer");
}

void rtval_command_buffer_draw(struct rtval_command_buffer* command_buffer, usize vertex_count, usize first_vertex) {
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

void rtval_command_buffer_draw_instanced(struct rtval_command_buffer* command_buffer, usize vertex_count, usize instance_count, usize first_vertex, usize first_instance) {
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

void rtval_command_buffer_draw_indexed(struct rtval_command_buffer* command_buffer, usize index_count, usize first_index, usize vertex_offset) {
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

void rtval_command_buffer_draw_indexed_instanced(struct rtval_command_buffer* command_buffer, usize index_count, usize instance_count, usize first_index, usize vertex_offset, usize first_instance) {
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
	if (!rtval_command_buffer_recording(command_buffer, "rtCommandBufferEnd")) {
		return;
	}
	if (state->rendering) {
		RTVAL_DROP("rtCommandBufferEnd: active rendering scope must end first");
		return;
	}
	state->recording = false;
	state->executable = true;
	rtval_next_rtCommandBufferEnd(state->backend);
	rtval_report_error("rtCommandBufferEnd");
}

#undef RTVAL_DROP
