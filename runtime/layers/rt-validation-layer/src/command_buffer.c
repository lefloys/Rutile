#include "command_buffer.h"
#include "buffer.h"
#include "compute_program.h"
#include "framebuffer.h"
#include "graphics_program.h"
#include "logger.h"
#include "queue.h"
#include "texture_view.h"

#define RTVAL_DROP(message) rtval_printf("[validation] %s, dropping call\n", message)

static bool rtval_cb_recording(struct rtval_command_buffer* state, const char* call_name) {
	struct rtval_command_context* context = state ? RTVAL_PAYLOAD(state->command_context, struct rtval_command_context) : NULL;
	if (!context || !context->queue || context->submitted) {
		rtval_printf("[validation] %s: command buffer has no live, recordable context, dropping call\n", call_name);
		return false;
	}
	if (!state->recording) {
		rtval_printf("[validation] %s: command buffer is not recording, dropping call\n", call_name);
		return false;
	}
	return true;
}

static bool rtval_cb_rendering(struct rtval_command_buffer* state, const char* call_name) {
	if (!rtval_cb_recording(state, call_name)) {
		return false;
	}
	struct rtval_command_context* context = RTVAL_PAYLOAD(state->command_context, struct rtval_command_context);
	if (!context->framebuffer) {
		rtval_printf("[validation] %s: command context has no framebuffer compatibility, dropping call\n", call_name);
		return false;
	}
	return true;
}

static void rtval_context_remove_child(struct rtval_command_buffer* child) {
	struct rtval_command_context* context = child ? RTVAL_PAYLOAD(child->command_context, struct rtval_command_context) : NULL;
	if (!context) {
		return;
	}
	if (child->previous) {
		child->previous->next = child->next;
	} else {
		context->children = child->next;
	}
	if (child->next) {
		child->next->previous = child->previous;
	}
}

static void rtval_context_discard_children(struct rtval_command_context* context) {
	for (struct rtval_command_buffer* child = context->children; child; child = child->next) {
		child->recording = false;
		child->executable = false;
		child->executed = false;
	}
}

static void rtval_context_invalidate_children(struct rtval_command_context* context) {
	struct rtval_command_buffer* child = context->children;
	while (child) {
		struct rtval_command_buffer* next = child->next;
		rtval_handle_destroy(child);
		child = next;
	}
	context->children = NULL;
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RT_EXPORT rt_command_context rtCommandContextCreate(void) {
	rt_command_context backend = rtval_next_rtCommandContextCreate();
	if (!backend) { rtval_report_error("rtCommandContextCreate"); return NULL; }
	struct rtval_command_context* context = rtval_handle_create(RTVAL_HANDLE_TYPE_COMMAND_CONTEXT);
	if (!context) { rtval_next_rtCommandContextDestroy(backend); return NULL; }
	RTVAL_PAYLOAD(context, struct rtval_command_context)->backend = backend;
	return rtval_command_context_to_handle(context);
}
RT_EXPORT void rtCommandContextDestroy(rt_command_context c) { struct rtval_command_context* x = RTVAL_PAYLOAD(rtval_command_context_from_handle(c), struct rtval_command_context); if (!x) { return; } rtval_context_invalidate_children(x); rtval_next_rtCommandContextDestroy(x->backend); rtval_handle_destroy(rtval_command_context_from_handle(c)); }
RT_EXPORT void rtCommandContextBind(rt_command_context c, rt_queue q) { struct rtval_command_context* x = RTVAL_PAYLOAD(rtval_command_context_from_handle(c), struct rtval_command_context); struct rtval_queue* queue = RTVAL_PAYLOAD(rtval_queue_from_handle(q), struct rtval_queue); if (!x || !queue) { RTVAL_DROP("rtCommandContextBind: invalid handle"); return; } rtval_context_discard_children(x); x->queue = rtval_queue_from_handle(q); x->framebuffer = NULL; x->rendering = false; x->submitted = false; rtval_next_rtCommandContextBind(x->backend, queue->backend); rtval_report_error("rtCommandContextBind"); }
RT_EXPORT rt_command_buffer rtCommandContextAllocate(rt_command_context c) { struct rtval_command_context* x = RTVAL_PAYLOAD(rtval_command_context_from_handle(c), struct rtval_command_context); if (!x || x->submitted) { RTVAL_DROP("rtCommandContextAllocate: invalid context state"); return NULL; } rt_command_buffer backend = rtval_next_rtCommandContextAllocate(x->backend); if (!backend) return NULL; struct rtval_command_buffer* b = rtval_handle_create(RTVAL_HANDLE_TYPE_COMMAND_BUFFER); if (!b) { rtval_next_rtCommandBufferDestroy(backend); return NULL; } struct rtval_command_buffer* state = RTVAL_PAYLOAD(b, struct rtval_command_buffer); state->backend = backend; state->command_context = rtval_command_context_from_handle(c); state->next = x->children; if (x->children) { x->children->previous = state; } x->children = state; return rtval_command_buffer_to_handle(b); }
RT_EXPORT void rtCommandBufferDestroy(rt_command_buffer b) { struct rtval_command_buffer* x = RTVAL_PAYLOAD(rtval_command_buffer_from_handle(b), struct rtval_command_buffer); struct rtval_command_context* c = x ? RTVAL_PAYLOAD(x->command_context, struct rtval_command_context) : NULL; if (!x) { return; } if (x->executed && c && !c->submitted) { RTVAL_DROP("rtCommandBufferDestroy: buffer was executed by an unsubmitted context"); return; } rtval_context_remove_child(x); rtval_next_rtCommandBufferDestroy(x->backend); rtval_handle_destroy(rtval_command_buffer_from_handle(b)); }
RT_EXPORT void rtCmdReset(rt_command_buffer b) { struct rtval_command_buffer* x = RTVAL_PAYLOAD(rtval_command_buffer_from_handle(b), struct rtval_command_buffer); struct rtval_command_context* c = x ? RTVAL_PAYLOAD(x->command_context, struct rtval_command_context) : NULL; if (!x || x->recording || (x->executed && c && !c->submitted)) { RTVAL_DROP("rtCmdReset: invalid, recording, or executed by an unsubmitted context"); return; } x->executable = false; x->executed = false; rtval_next_rtCmdReset(x->backend); rtval_report_error("rtCmdReset"); }
RT_EXPORT void rtCmdBegin(rt_command_buffer b) { struct rtval_command_buffer* x = RTVAL_PAYLOAD(rtval_command_buffer_from_handle(b), struct rtval_command_buffer); struct rtval_command_context* c = x ? RTVAL_PAYLOAD(x->command_context, struct rtval_command_context) : NULL; if (!x || !c || !c->queue || !c->framebuffer || c->submitted || x->recording || x->executable) { RTVAL_DROP("rtCmdBegin: live, bound context, framebuffer, and reset buffer required"); return; } x->recording = true; rtval_next_rtCmdBegin(x->backend); rtval_report_error("rtCmdBegin"); }
RT_EXPORT void rtCommandContextBindFramebuffer(rt_command_context c, rt_framebuffer f) { struct rtval_command_context* x = RTVAL_PAYLOAD(rtval_command_context_from_handle(c), struct rtval_command_context); struct rtval_framebuffer* fb = RTVAL_PAYLOAD(rtval_framebuffer_from_handle(f), struct rtval_framebuffer); if (!x || !x->queue || !fb || x->submitted || x->framebuffer) { RTVAL_DROP("rtCommandContextBindFramebuffer: context already has a rendering scope or is unbound"); return; } x->framebuffer = rtval_framebuffer_from_handle(f); x->rendering = true; rtval_next_rtCommandContextBindFramebuffer(x->backend, fb->backend); rtval_report_error("rtCommandContextBindFramebuffer"); }
RT_EXPORT void rtCommandContextClearColor(rt_command_context c, u32 i, f32 r, f32 g, f32 b, f32 a) { struct rtval_command_context* x = RTVAL_PAYLOAD(rtval_command_context_from_handle(c), struct rtval_command_context); if (!x || !x->rendering) { RTVAL_DROP("rtCommandContextClearColor: invalid state"); return; } rtval_next_rtCommandContextClearColor(x->backend, i, r, g, b, a); }
RT_EXPORT void rtCommandContextClearDepth(rt_command_context c, f32 d) { struct rtval_command_context* x = RTVAL_PAYLOAD(rtval_command_context_from_handle(c), struct rtval_command_context); if (!x || !x->rendering) { RTVAL_DROP("rtCommandContextClearDepth: invalid state"); return; } rtval_next_rtCommandContextClearDepth(x->backend, d); }
RT_EXPORT void rtCommandContextClearStencil(rt_command_context c, u32 s) { struct rtval_command_context* x = RTVAL_PAYLOAD(rtval_command_context_from_handle(c), struct rtval_command_context); if (!x || !x->rendering) { RTVAL_DROP("rtCommandContextClearStencil: invalid state"); return; } rtval_next_rtCommandContextClearStencil(x->backend, s); }

RT_EXPORT void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program) {
	rtval_command_buffer_use_graphics_program(
		rtval_command_buffer_from_handle(command_buffer),
		rtval_graphics_program_from_handle(program)
	);
}

RT_EXPORT void rtCmdSetScissor(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height) {
	rtval_command_buffer_set_scissor(rtval_command_buffer_from_handle(command_buffer), x, y, width, height);
}

RT_EXPORT void rtCmdUseComputeProgram(rt_command_buffer command_buffer, rt_compute_program program) {
	rtval_command_buffer_use_compute_program(
		rtval_command_buffer_from_handle(command_buffer),
		rtval_compute_program_from_handle(program)
	);
}

RT_EXPORT void rtCmdUniformBuffer(rt_command_buffer command_buffer, rt_uniform_location location, rt_buffer buffer, u64 offset, u64 size) {
	rtval_command_buffer_uniform_buffer(
		rtval_command_buffer_from_handle(command_buffer),
		location,
		rtval_buffer_from_handle(buffer),
		offset,
		size
	);
}

RT_EXPORT void rtCmdUniformTexture(rt_command_buffer command_buffer, rt_uniform_location location, rt_texture_view view) {
	rtval_command_buffer_uniform_texture(
		rtval_command_buffer_from_handle(command_buffer),
		location,
		rtval_texture_view_from_handle(view)
	);
}

RT_EXPORT void rtCmdStorageBuffer(rt_command_buffer command_buffer, rt_uniform_location location, rt_buffer buffer, u64 offset, u64 size) {
	rtval_command_buffer_storage_buffer(
		rtval_command_buffer_from_handle(command_buffer),
		location,
		rtval_buffer_from_handle(buffer),
		offset,
		size
	);
}

RT_EXPORT void rtCmdStorageTexture(rt_command_buffer command_buffer, u32 binding, rt_texture_view view) {
	rtval_command_buffer_storage_texture(
		rtval_command_buffer_from_handle(command_buffer),
		binding,
		rtval_texture_view_from_handle(view)
	);
}

RT_EXPORT void rtCmdComputeBarrier(rt_command_buffer command_buffer) {
	rtval_command_buffer_compute_barrier(rtval_command_buffer_from_handle(command_buffer));
}

RT_EXPORT void rtCmdBindVertexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, u64 offset) {
	rtval_command_buffer_bind_vertex_buffer(
		rtval_command_buffer_from_handle(command_buffer),
		rtval_buffer_from_handle(buffer),
		offset
	);
}

RT_EXPORT void rtCmdDraw(rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex) {
	rtval_command_buffer_draw(rtval_command_buffer_from_handle(command_buffer), vertex_count, first_vertex);
}

RT_EXPORT void rtCmdDispatch(rt_command_buffer command_buffer, u32 group_count_x, u32 group_count_y, u32 group_count_z) {
	rtval_command_buffer_dispatch(rtval_command_buffer_from_handle(command_buffer), group_count_x, group_count_y, group_count_z);
}

RT_EXPORT void rtCommandContextEndRendering(rt_command_context c) { struct rtval_command_context* x = RTVAL_PAYLOAD(rtval_command_context_from_handle(c), struct rtval_command_context); if (!x || !x->rendering) { RTVAL_DROP("rtCommandContextEndRendering: invalid state"); return; } for (struct rtval_command_buffer* child = x->children; child; child = child->next) { if (child->recording) { RTVAL_DROP("rtCommandContextEndRendering: child buffer is recording"); return; } } x->rendering = false; rtval_next_rtCommandContextEndRendering(x->backend); rtval_report_error("rtCommandContextEndRendering"); }
RT_EXPORT void rtCommandContextExecute(rt_command_context c, rt_command_buffer b) { struct rtval_command_context* x = RTVAL_PAYLOAD(rtval_command_context_from_handle(c), struct rtval_command_context); struct rtval_command_buffer* child = RTVAL_PAYLOAD(rtval_command_buffer_from_handle(b), struct rtval_command_buffer); if (!x || !child || child->command_context != rtval_command_context_from_handle(c) || !x->rendering || !child->executable) { RTVAL_DROP("rtCommandContextExecute: child ownership, executable state, and active rendering required"); return; } child->executed = true; rtval_next_rtCommandContextExecute(x->backend, child->backend); rtval_report_error("rtCommandContextExecute"); }
RT_EXPORT rt_timepoint rtCommandContextSubmit(rt_command_context c) { struct rtval_command_context* x = RTVAL_PAYLOAD(rtval_command_context_from_handle(c), struct rtval_command_context); if (!x || !x->queue || !x->framebuffer || x->rendering || x->submitted) { RTVAL_DROP("rtCommandContextSubmit: completed bound rendering scope required"); return (rt_timepoint){ 0 }; } x->submitted = true; return rtval_timepoint_wrap(rtval_next_rtCommandContextSubmit(x->backend)); }

RT_EXPORT void rtCmdEnd(rt_command_buffer command_buffer) {
	rtval_command_buffer_end(rtval_command_buffer_from_handle(command_buffer));
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

void rtval_command_buffer_use_graphics_program(struct rtval_command_buffer* cb, struct rtval_graphics_program* program) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(cb, struct rtval_command_buffer);
	if (!state) {
		RTVAL_DROP("rtCmdUseGraphicsProgram: invalid command buffer");
		return;
	}
	if (!rtval_cb_rendering(state, "rtCmdUseGraphicsProgram")) {
		return;
	}
	struct rtval_graphics_program* prog_state = RTVAL_PAYLOAD(program, struct rtval_graphics_program);
	if (!prog_state) {
		RTVAL_DROP("rtCmdUseGraphicsProgram: invalid program");
		return;
	}
	rtval_next_rtCmdUseGraphicsProgram(state->backend, prog_state->backend);
	rtval_report_error("rtCmdUseGraphicsProgram");
}

void rtval_command_buffer_set_scissor(struct rtval_command_buffer* cb, u32 x, u32 y, u32 width, u32 height) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(cb, struct rtval_command_buffer);
	if (!state) {
		RTVAL_DROP("rtCmdSetScissor: invalid handle");
		return;
	}
	if (!rtval_cb_rendering(state, "rtCmdSetScissor")) {
		return;
	}
	rtval_next_rtCmdSetScissor(state->backend, x, y, width, height);
	rtval_report_error("rtCmdSetScissor");
}

void rtval_command_buffer_use_compute_program(struct rtval_command_buffer* cb, struct rtval_compute_program* program) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(cb, struct rtval_command_buffer);
	if (!state) {
		RTVAL_DROP("rtCmdUseComputeProgram: invalid command buffer");
		return;
	}
	if (!rtval_cb_recording(state, "rtCmdUseComputeProgram")) {
		return;
	}
	RTVAL_DROP("rtCmdUseComputeProgram: child buffers are draw packets");
	return;
}

void rtval_command_buffer_uniform_buffer(struct rtval_command_buffer* cb, rt_uniform_location location, struct rtval_buffer* buffer, u64 offset, u64 size) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(cb, struct rtval_command_buffer);
	if (!state) {
		RTVAL_DROP("rtCmdUniformBuffer: invalid command buffer");
		return;
	}
	if (!rtval_cb_rendering(state, "rtCmdUniformBuffer")) {
		return;
	}
	if (!location) {
		RTVAL_DROP("rtCmdUniformBuffer: NULL location");
		return;
	}
	struct rtval_buffer* buf_state = RTVAL_PAYLOAD(buffer, struct rtval_buffer);
	if (!buf_state) {
		RTVAL_DROP("rtCmdUniformBuffer: invalid buffer");
		return;
	}
	if (size == 0) {
		RTVAL_DROP("rtCmdUniformBuffer: zero size");
		return;
	}
	rtval_next_rtCmdUniformBuffer(state->backend, location, buf_state->backend, offset, size);
	rtval_report_error("rtCmdUniformBuffer");
}

void rtval_command_buffer_uniform_texture(struct rtval_command_buffer* cb, rt_uniform_location location, struct rtval_texture_view* view) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(cb, struct rtval_command_buffer);
	if (!state) {
		RTVAL_DROP("rtCmdUniformTexture: invalid command buffer");
		return;
	}
	if (!rtval_cb_rendering(state, "rtCmdUniformTexture")) {
		return;
	}
	if (!location) {
		RTVAL_DROP("rtCmdUniformTexture: NULL location");
		return;
	}
	struct rtval_texture_view* view_state = RTVAL_PAYLOAD(view, struct rtval_texture_view);
	if (!view_state) {
		RTVAL_DROP("rtCmdUniformTexture: invalid view");
		return;
	}
	rtval_next_rtCmdUniformTexture(state->backend, location, view_state->backend);
	rtval_report_error("rtCmdUniformTexture");
}

void rtval_command_buffer_storage_buffer(struct rtval_command_buffer* cb, rt_uniform_location location, struct rtval_buffer* buffer, u64 offset, u64 size) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(cb, struct rtval_command_buffer);
	if (!state) {
		RTVAL_DROP("rtCmdStorageBuffer: invalid command buffer");
		return;
	}
	if (!rtval_cb_recording(state, "rtCmdStorageBuffer")) {
		return;
	}
	struct rtval_buffer* buf_state = RTVAL_PAYLOAD(buffer, struct rtval_buffer);
	if (!buf_state) {
		RTVAL_DROP("rtCmdStorageBuffer: invalid buffer");
		return;
	}
	if (size == 0) {
		RTVAL_DROP("rtCmdStorageBuffer: zero size");
		return;
	}
	rtval_next_rtCmdStorageBuffer(state->backend, location, buf_state->backend, offset, size);
	rtval_report_error("rtCmdStorageBuffer");
}

void rtval_command_buffer_storage_texture(struct rtval_command_buffer* cb, u32 binding, struct rtval_texture_view* view) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(cb, struct rtval_command_buffer);
	if (!state) {
		RTVAL_DROP("rtCmdStorageTexture: invalid command buffer");
		return;
	}
	if (!rtval_cb_recording(state, "rtCmdStorageTexture")) {
		return;
	}
	struct rtval_texture_view* view_state = RTVAL_PAYLOAD(view, struct rtval_texture_view);
	if (!view_state) {
		RTVAL_DROP("rtCmdStorageTexture: invalid view");
		return;
	}
	rtval_next_rtCmdStorageTexture(state->backend, binding, view_state->backend);
	rtval_report_error("rtCmdStorageTexture");
}

void rtval_command_buffer_compute_barrier(struct rtval_command_buffer* cb) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(cb, struct rtval_command_buffer);
	if (!state) {
		RTVAL_DROP("rtCmdComputeBarrier: invalid handle");
		return;
	}
	if (!rtval_cb_recording(state, "rtCmdComputeBarrier")) {
		return;
	}
	RTVAL_DROP("rtCmdComputeBarrier: child buffers are draw packets");
}

void rtval_command_buffer_bind_vertex_buffer(struct rtval_command_buffer* cb, struct rtval_buffer* buffer, u64 offset) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(cb, struct rtval_command_buffer);
	if (!state) {
		RTVAL_DROP("rtCmdBindVertexBuffer: invalid command buffer");
		return;
	}
	if (!rtval_cb_rendering(state, "rtCmdBindVertexBuffer")) {
		return;
	}
	struct rtval_buffer* buf_state = RTVAL_PAYLOAD(buffer, struct rtval_buffer);
	if (!buf_state) {
		RTVAL_DROP("rtCmdBindVertexBuffer: invalid buffer");
		return;
	}
	rtval_next_rtCmdBindVertexBuffer(state->backend, buf_state->backend, offset);
	rtval_report_error("rtCmdBindVertexBuffer");
}

void rtval_command_buffer_draw(struct rtval_command_buffer* cb, u32 vertex_count, u32 first_vertex) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(cb, struct rtval_command_buffer);
	if (!state) {
		RTVAL_DROP("rtCmdDraw: invalid handle");
		return;
	}
	if (!rtval_cb_rendering(state, "rtCmdDraw")) {
		return;
	}
	if (vertex_count == 0) {
		RTVAL_DROP("rtCmdDraw: zero vertex count");
		return;
	}
	rtval_next_rtCmdDraw(state->backend, vertex_count, first_vertex);
	rtval_report_error("rtCmdDraw");
}

void rtval_command_buffer_dispatch(struct rtval_command_buffer* cb, u32 group_count_x, u32 group_count_y, u32 group_count_z) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(cb, struct rtval_command_buffer);
	if (!state) {
		RTVAL_DROP("rtCmdDispatch: invalid handle");
		return;
	}
	if (!rtval_cb_recording(state, "rtCmdDispatch")) {
		return;
	}
	RTVAL_DROP("rtCmdDispatch: child buffers are draw packets");
}

void rtval_command_buffer_end(struct rtval_command_buffer* cb) {
	struct rtval_command_buffer* state = RTVAL_PAYLOAD(cb, struct rtval_command_buffer);
	if (!state) {
		RTVAL_DROP("rtCmdEnd: invalid handle");
		return;
	}
	if (!state->recording) {
		RTVAL_DROP("rtCmdEnd: command buffer is not recording");
		return;
	}
	state->recording = false;
	state->executable = true;
	rtval_next_rtCmdEnd(state->backend);
	rtval_report_error("rtCmdEnd");
}

#undef RTVAL_DROP
