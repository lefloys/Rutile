#include "resource/command_buffer.h"

#include "context.h"
#include "error.h"
#include "execution.h"
#include "execution_internal.hpp"
#include "glad/gl.h"
#include "resource/queue.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static rtgl_recorded_command* rtgl_command_buffer_append(struct rtgl_command_buffer* command_buffer) {
	if (command_buffer->command_context && !command_buffer->recording) {
		rtgl_throwf(RT_IMPROPER_USAGE, "draw commands require a recording command buffer");
		return NULL;
	}
	if (command_buffer->command_count == command_buffer->command_capacity) {
		u32 next_capacity = command_buffer->command_capacity ? command_buffer->command_capacity * 2 : 16;
		rtgl_recorded_command* next = (rtgl_recorded_command*)realloc(
			command_buffer->commands,
			sizeof(*command_buffer->commands) * (usize)next_capacity
		);
		if (!next) {
			RTGL_CHECK_ALLOC(next, sizeof(*command_buffer->commands) * (usize)next_capacity, "OpenGL recorded commands");
			return NULL;
		}
		command_buffer->commands = next;
		command_buffer->command_capacity = next_capacity;
	}
	rtgl_recorded_command* command = &command_buffer->commands[command_buffer->command_count++];
	memset(command, 0, sizeof(*command));
	command->size = sizeof(*command);
	return command;
}

static void rtgl_command_buffer_release_command(rtgl_recorded_command* command) {

	switch (command->kind) {
	case RTGL_RECORDED_COMMAND_BEGIN_RENDERING:
		rtgl_release_resource(command->data.begin_rendering.color_view);
		rtgl_release_resource(command->data.begin_rendering.depth_view);
		rtgl_release_resource(command->data.begin_rendering.framebuffer);
		break;
	case RTGL_RECORDED_COMMAND_USE_GRAPHICS_PROGRAM:
		rtgl_release_resource(command->data.use_graphics_program.program);
		break;
	case RTGL_RECORDED_COMMAND_UNIFORM_BUFFER:
		rtgl_release_resource(command->data.uniform_buffer.location_program);
		rtgl_release_resource(command->data.uniform_buffer.buffer);
		break;
	case RTGL_RECORDED_COMMAND_UNIFORM_TEXTURE:
		rtgl_release_resource(command->data.uniform_texture.location_program);
		rtgl_release_resource(command->data.uniform_texture.texture_view);
		break;
	case RTGL_RECORDED_COMMAND_STORAGE_BUFFER:
		rtgl_release_resource(command->data.storage_buffer.location_program);
		rtgl_release_resource(command->data.storage_buffer.buffer);
		break;
	case RTGL_RECORDED_COMMAND_BIND_VERTEX_BUFFER:
		rtgl_release_resource(command->data.bind_vertex_buffer.buffer);
		break;
	default:
		break;
	}
}

static void rtgl_command_buffer_clear_commands(struct rtgl_command_buffer* command_buffer) {
	for (u32 i = 0; i < command_buffer->command_count; i++) {
		rtgl_command_buffer_release_command(&command_buffer->commands[i]);
	}
	command_buffer->command_count = 0;
}

static void rtgl_record_location_program(rtgl_uniform_location* location, struct rtgl_graphics_program** program) {
	*program = location ? location->program : NULL;
	if (*program) {
		rtgl_retain_resource(*program);
	}
}
rt_command_context rtCommandContextCreate(void) {
	struct rtgl_command_context* context = (struct rtgl_command_context*)calloc(1, sizeof(*context));
	if (!context) { rtgl_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate command context"); return NULL; }
	context->primary = rtgl_command_buffer_create(rtgl_get_current_context());
	if (!context->primary) { free(context); return NULL; }
	return rtgl_command_context_to_handle(context);
}
void rtCommandContextDestroy(rt_command_context command_context) {
	struct rtgl_command_context* context = rtgl_command_context_from_handle(command_context); if (!context) return;
	while (context->children) { struct rtgl_command_buffer* child = context->children; context->children = child->next_child; child->command_context = NULL; rtgl_command_buffer_destroy(rtgl_get_current_context(), child); }
	rtgl_command_buffer_destroy(rtgl_get_current_context(), context->primary); free(context);
}
void rtCommandContextBind(rt_command_context command_context, rt_queue queue) {
	struct rtgl_command_context* context = rtgl_command_context_from_handle(command_context); if (!context || !queue) { rtgl_throwf(RT_IMPROPER_USAGE, "command context bind requires valid context and queue"); return; }
	context->queue = rtgl_queue_from_handle(queue); context->framebuffer = NULL; context->rendering = false; context->submitted = false;
	rtgl_command_buffer_clear_commands(context->primary); context->primary->queue = context->queue;
	for (struct rtgl_command_buffer* child = context->children; child; child = child->next_child) {
		rtgl_command_buffer_clear_commands(child);
		child->recording = false;
		child->executable = false;
		child->executed = false;
	}
}
rt_command_buffer rtCommandContextAllocate(rt_command_context command_context) {
	struct rtgl_command_context* context = rtgl_command_context_from_handle(command_context); if (!context) { rtgl_throwf(RT_IMPROPER_USAGE, "command buffer allocation requires a context"); return NULL; }
	struct rtgl_command_buffer* child = rtgl_command_buffer_create(rtgl_get_current_context()); if (!child) return NULL;
	child->command_context = context; child->next_child = context->children; context->children = child; return rtgl_command_buffer_to_handle(child);
}
void rtCommandBufferDestroy(rt_command_buffer command_buffer) {
	struct rtgl_command_buffer* child = rtgl_command_buffer_from_handle(command_buffer); if (!child) return;
	if (child->command_context && child->executed && !child->command_context->submitted) { rtgl_throwf(RT_IMPROPER_USAGE, "cannot destroy an executed command buffer before its context is submitted"); return; }
	if (child->command_context) { struct rtgl_command_buffer** link = &child->command_context->children; while (*link && *link != child) link = &(*link)->next_child; if (*link) *link = child->next_child; }
	rtgl_command_buffer_destroy(rtgl_get_current_context(), child);
}
void rtCmdReset(rt_command_buffer command_buffer) {
	struct rtgl_command_buffer* child = rtgl_command_buffer_from_handle(command_buffer);
	if (!child || !child->command_context) { rtgl_throwf(RT_IMPROPER_USAGE, "command buffer reset requires a live context-owned buffer"); return; }
	if (child->recording || (child->executed && !child->command_context->submitted)) { rtgl_throwf(RT_IMPROPER_USAGE, "cannot reset a recording or executed-unsubmitted command buffer"); return; }
	rtgl_command_buffer_clear_commands(child);
	child->executable = false;
	child->executed = false;
}
void rtCmdBegin(rt_command_buffer command_buffer) {
	struct rtgl_command_buffer* internal = rtgl_command_buffer_from_handle(command_buffer);
	if (!internal || !internal->command_context || !internal->command_context->queue || !internal->command_context->framebuffer || internal->recording || internal->executable) { rtgl_throwf(RT_IMPROPER_USAGE, "command buffer begin requires a reset child and bound context framebuffer"); return; }
	internal->queue = internal->command_context->queue;
	internal->recording = true;
}
void rtCmdEnd(rt_command_buffer command_buffer) {
	struct rtgl_command_buffer* internal = rtgl_command_buffer_from_handle(command_buffer);
	if (!internal || !internal->command_context || !internal->recording) { rtgl_throwf(RT_IMPROPER_USAGE, "command buffer end requires recording"); return; }
	internal->recording = false;
	internal->executable = true;
}
void rtCommandContextBindFramebuffer(rt_command_context command_context, rt_framebuffer framebuffer) {
	struct rtgl_command_context* context = rtgl_command_context_from_handle(command_context); if (!context || !context->queue || context->submitted || context->rendering) { rtgl_throwf(RT_IMPROPER_USAGE, "command context framebuffer bind requires a bound, unsubmitted context"); return; }
	context->framebuffer = rtgl_framebuffer_from_handle(framebuffer); context->rendering = true;
	struct rtgl_command_buffer* internal = context->primary;
	rtgl_recorded_command* command = internal ? rtgl_command_buffer_append(internal) : NULL;
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_BEGIN_RENDERING;
	command->data.begin_rendering.framebuffer = context->framebuffer;
	if (context->framebuffer) { rtgl_retain_resource(context->framebuffer); command->data.begin_rendering.color_view = context->framebuffer->color_views[0]; command->data.begin_rendering.depth_view = context->framebuffer->depth_view; rtgl_retain_resource(command->data.begin_rendering.color_view); rtgl_retain_resource(command->data.begin_rendering.depth_view); }
}
void rtCommandContextClearColor(rt_command_context c, u32 color_index, f32 r, f32 g, f32 b, f32 a) {
	struct rtgl_command_context* context = rtgl_command_context_from_handle(c); struct rtgl_command_buffer* internal = context ? context->primary : NULL;
	rtgl_recorded_command* command = internal ? rtgl_command_buffer_append(internal) : NULL; if (!command) return; command->kind = RTGL_RECORDED_COMMAND_CLEAR_COLOR;
	command->data.clear_color.color_index = color_index;
	command->data.clear_color.values[0] = r;
	command->data.clear_color.values[1] = g;
	command->data.clear_color.values[2] = b;
	command->data.clear_color.values[3] = a;
}
void rtCommandContextClearDepth(rt_command_context c, f32 depth) {
	struct rtgl_command_context* context = rtgl_command_context_from_handle(c); struct rtgl_command_buffer* internal = context ? context->primary : NULL;
	rtgl_recorded_command* command = internal ? rtgl_command_buffer_append(internal) : NULL;
	if (command) {
		command->kind = RTGL_RECORDED_COMMAND_CLEAR_DEPTH;
		command->data.clear_depth = depth;
	}
}
void rtCommandContextClearStencil(rt_command_context, u32) {
	rtgl_throwf(RT_UNSUPPORTED_FEATURE, "OpenGL stencil clear is not implemented");
}

void rtCommandContextEndRendering(rt_command_context command_context) {
	struct rtgl_command_context* context = rtgl_command_context_from_handle(command_context);
	if (!context || !context->rendering) { rtgl_throwf(RT_IMPROPER_USAGE, "command context is not rendering"); return; }
	rtgl_recorded_command* command = rtgl_command_buffer_append(context->primary);
	if (command) command->kind = RTGL_RECORDED_COMMAND_END_RENDERING;
	context->rendering = false;
}

void rtCommandContextExecute(rt_command_context command_context, rt_command_buffer command_buffer) {
	struct rtgl_command_context* context = rtgl_command_context_from_handle(command_context);
	struct rtgl_command_buffer* child = rtgl_command_buffer_from_handle(command_buffer);
	if (!context || !child || child->command_context != context || !context->rendering || child->recording || !child->executable) { rtgl_throwf(RT_IMPROPER_USAGE, "command context execute requires an ended owned buffer in active rendering"); return; }
	/* OpenGL has no native secondary command list. The context records the child
	 * packet by value when it is executed, retaining its resources for this
	 * one-shot submission. */
	for (u32 i = 0; i < child->command_count; ++i) {
		rtgl_recorded_command* dst = rtgl_command_buffer_append(context->primary);
		if (!dst) return;
		*dst = child->commands[i];
		switch (dst->kind) {
		case RTGL_RECORDED_COMMAND_USE_GRAPHICS_PROGRAM: rtgl_retain_resource(dst->data.use_graphics_program.program); break;
		case RTGL_RECORDED_COMMAND_UNIFORM_BUFFER: rtgl_retain_resource(dst->data.uniform_buffer.location_program); rtgl_retain_resource(dst->data.uniform_buffer.buffer); break;
		case RTGL_RECORDED_COMMAND_UNIFORM_TEXTURE: rtgl_retain_resource(dst->data.uniform_texture.location_program); rtgl_retain_resource(dst->data.uniform_texture.texture_view); break;
		case RTGL_RECORDED_COMMAND_STORAGE_BUFFER: rtgl_retain_resource(dst->data.storage_buffer.location_program); rtgl_retain_resource(dst->data.storage_buffer.buffer); break;
		case RTGL_RECORDED_COMMAND_BIND_VERTEX_BUFFER: rtgl_retain_resource(dst->data.bind_vertex_buffer.buffer); break;
		default: break;
		}
	}
	child->executed = true;
}

rt_timepoint rtCommandContextSubmit(rt_command_context command_context) {
	struct rtgl_command_context* context = rtgl_command_context_from_handle(command_context);
	if (!context || !context->queue || context->submitted || context->rendering) { rtgl_throwf(RT_IMPROPER_USAGE, "command context submit requires ended one-shot context"); return (rt_timepoint){ 0 }; }
	context->submitted = true;
	return rtgl_timepoint_to_public(rtgl_command_buffer_submit(rtgl_get_current_context(), context->queue, context->primary));
}

void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program) {
	struct rtgl_command_buffer* internal = rtgl_command_buffer_from_handle(command_buffer);
	rtgl_recorded_command* command = internal ? rtgl_command_buffer_append(internal) : NULL;
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_USE_GRAPHICS_PROGRAM;
	command->data.use_graphics_program.program = rtgl_graphics_program_from_handle(program);
	if (command->data.use_graphics_program.program) {
		rtgl_retain_resource(command->data.use_graphics_program.program);
	}
}

void rtCmdSetScissor(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height) {
	struct rtgl_command_buffer* internal = rtgl_command_buffer_from_handle(command_buffer);
	rtgl_recorded_command* command = internal ? rtgl_command_buffer_append(internal) : NULL;
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_SET_SCISSOR;
	command->data.set_scissor = { x, y, width, height };
}

void rtCmdUniformBuffer(rt_command_buffer command_buffer, rt_uniform_location location, rt_buffer buffer, u64 offset, u64 size) {
	struct rtgl_command_buffer* internal = rtgl_command_buffer_from_handle(command_buffer);
	rtgl_recorded_command* command = internal ? rtgl_command_buffer_append(internal) : NULL;
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_UNIFORM_BUFFER;
	command->data.uniform_buffer.location = (rtgl_uniform_location*)location;
	command->data.uniform_buffer.buffer = rtgl_buffer_from_handle(buffer);
	command->data.uniform_buffer.offset = offset;
	command->data.uniform_buffer.size = size;
	rtgl_record_location_program(command->data.uniform_buffer.location, &command->data.uniform_buffer.location_program);
	rtgl_retain_resource(command->data.uniform_buffer.buffer);
}

void rtCmdUniformTexture(rt_command_buffer command_buffer, rt_uniform_location location, rt_texture_view texture_view) {
	struct rtgl_command_buffer* internal = rtgl_command_buffer_from_handle(command_buffer);
	rtgl_recorded_command* command = internal ? rtgl_command_buffer_append(internal) : NULL;
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_UNIFORM_TEXTURE;
	command->data.uniform_texture.location = (rtgl_uniform_location*)location;
	command->data.uniform_texture.texture_view = rtgl_texture_view_from_handle(texture_view);
	rtgl_record_location_program(command->data.uniform_texture.location, &command->data.uniform_texture.location_program);
	rtgl_retain_resource(command->data.uniform_texture.texture_view);
}

void rtCmdStorageBuffer(rt_command_buffer command_buffer, rt_uniform_location location, rt_buffer buffer, u64 offset, u64 size) {
	struct rtgl_command_buffer* internal = rtgl_command_buffer_from_handle(command_buffer);
	rtgl_recorded_command* command = internal ? rtgl_command_buffer_append(internal) : NULL;
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_STORAGE_BUFFER;
	command->data.storage_buffer.location = (rtgl_uniform_location*)location;
	command->data.storage_buffer.buffer = rtgl_buffer_from_handle(buffer);
	command->data.storage_buffer.offset = offset;
	command->data.storage_buffer.size = size;
	rtgl_record_location_program(command->data.storage_buffer.location, &command->data.storage_buffer.location_program);
	rtgl_retain_resource(command->data.storage_buffer.buffer);
}

void rtCmdBindVertexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, u64 offset) {
	struct rtgl_command_buffer* internal = rtgl_command_buffer_from_handle(command_buffer);
	rtgl_recorded_command* command = internal ? rtgl_command_buffer_append(internal) : NULL;
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_BIND_VERTEX_BUFFER;
	command->data.bind_vertex_buffer.buffer = rtgl_buffer_from_handle(buffer);
	command->data.bind_vertex_buffer.offset = offset;
	rtgl_retain_resource(command->data.bind_vertex_buffer.buffer);
}

void rtCmdDraw(rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex) {
	struct rtgl_command_buffer* internal = rtgl_command_buffer_from_handle(command_buffer);
	rtgl_recorded_command* command = internal ? rtgl_command_buffer_append(internal) : NULL;
	if (command) {
		command->kind = RTGL_RECORDED_COMMAND_DRAW;
		command->data.draw = { vertex_count, first_vertex };
	}
}

struct rtgl_command_buffer* rtgl_command_buffer_create(struct rtgl_context* ctx) {
	struct rtgl_command_buffer* command_buffer = RTGL_ALLOC_RESOURCE(struct rtgl_command_buffer);
	if (command_buffer) {
		rtgl_command_buffer_init(ctx, command_buffer);
	}
	return command_buffer;
}

void rtgl_command_buffer_destroy(struct rtgl_context*, struct rtgl_command_buffer* command_buffer) {
	if (command_buffer) {
		rtgl_resource_retire(RTGL_RESOURCE_BASE(command_buffer));
	}
}

void rtgl_command_buffer_init(struct rtgl_context* ctx, struct rtgl_command_buffer* command_buffer) {
	rtgl_init_resource_base(ctx, RTGL_RESOURCE_BASE(command_buffer), RTGL_RESOURCE_COMMAND_BUFFER);
}

void rtgl_command_buffer_finish(struct rtgl_command_buffer* command_buffer) {
	rtgl_command_buffer_clear_commands(command_buffer);
	free(command_buffer->commands);
	command_buffer->commands = NULL;
	command_buffer->command_capacity = 0;
	command_buffer->queue = NULL;
	rtgl_finish_resource_base(RTGL_RESOURCE_BASE(command_buffer));
}

struct rtgl_command_buffer_submission {
	rtgl_recorded_command* commands;
	u32 command_count;
};

static void rtgl_command_buffer_submission_destroy(struct rtgl_command_buffer_submission* submission) {
	if (!submission) {
		return;
	}
	for (u32 i = 0; i < submission->command_count; i++) {
		rtgl_command_buffer_release_command(&submission->commands[i]);
	}
	free(submission->commands);
	free(submission);
}

struct rtgl_timepoint rtgl_command_buffer_submit(struct rtgl_context* ctx, struct rtgl_queue* queue, struct rtgl_command_buffer* command_buffer) {
	struct rtgl_timepoint done = rtgl_queue_signal(queue);
	if (!command_buffer) {
		rtgl_execution_queue_complete(ctx, queue, done.value);
		return done;
	}

	struct rtgl_command_buffer_submission* submission = (struct rtgl_command_buffer_submission*)calloc(1, sizeof(*submission));
	if (!submission) {
		RTGL_CHECK_ALLOC(submission, sizeof(*submission), "OpenGL command buffer submission");
		rtgl_execution_queue_complete(ctx, queue, done.value);
		return done;
	}
	submission->commands = command_buffer->commands;
	submission->command_count = command_buffer->command_count;
	command_buffer->commands = NULL;
	command_buffer->command_count = 0;
	command_buffer->command_capacity = 0;
	command_buffer->queue = NULL;

	rtgl_retain_resource(command_buffer);
	if (!rtgl_execution_submit_async(ctx, [command_buffer, submission, queue, value = done.value](struct rtgl_context* exec_ctx) {
			/* Execute the detached list through a local view.  The logical command
			 * buffer may already be recording the next frame on another thread. */
			struct rtgl_command_buffer command_view = *command_buffer;
			command_view.commands = submission->commands;
			command_view.command_count = submission->command_count;
			rtgl_command_buffer_execute(exec_ctx, &command_view, queue, value);
			rtgl_command_buffer_submission_destroy(submission);
			rtgl_resource_release(RTGL_RESOURCE_BASE(command_buffer));
		})) {
		rtgl_command_buffer_submission_destroy(submission);
		rtgl_resource_release(RTGL_RESOURCE_BASE(command_buffer));
		rtgl_execution_queue_complete(ctx, queue, done.value);
	}
	return done;
}

static GLint rtgl_vertex_attribute_components(enum rt_format format) {
	switch (format) {
	case RT_R32_SFLOAT:
		return 1;
	case RT_RG32_SFLOAT:
		return 2;
	case RT_RGB32_SFLOAT:
		return 3;
	case RT_RGBA32_SFLOAT:
		return 4;
	default:
		return 0;
	}
}

static GLenum rtgl_blend_factor(enum rt_blend_factor factor) {
	switch (factor) {
	case RT_BLEND_ZERO:
		return GL_ZERO;
	case RT_BLEND_ONE:
		return GL_ONE;
	case RT_BLEND_SRC_COLOR:
		return GL_SRC_COLOR;
	case RT_BLEND_ONE_MINUS_SRC_COLOR:
		return GL_ONE_MINUS_SRC_COLOR;
	case RT_BLEND_DST_COLOR:
		return GL_DST_COLOR;
	case RT_BLEND_ONE_MINUS_DST_COLOR:
		return GL_ONE_MINUS_DST_COLOR;
	case RT_BLEND_SRC_ALPHA:
		return GL_SRC_ALPHA;
	case RT_BLEND_ONE_MINUS_SRC_ALPHA:
		return GL_ONE_MINUS_SRC_ALPHA;
	case RT_BLEND_DST_ALPHA:
		return GL_DST_ALPHA;
	case RT_BLEND_ONE_MINUS_DST_ALPHA:
		return GL_ONE_MINUS_DST_ALPHA;
	default:
		return GL_ONE;
	}
}

static GLenum rtgl_blend_op(enum rt_blend_op op) {
	switch (op) {
	case RT_BLEND_OP_ADD:
		return GL_FUNC_ADD;
	case RT_BLEND_OP_SUBTRACT:
		return GL_FUNC_SUBTRACT;
	case RT_BLEND_OP_REVERSE_SUBTRACT:
		return GL_FUNC_REVERSE_SUBTRACT;
	case RT_BLEND_OP_MIN:
		return GL_MIN;
	case RT_BLEND_OP_MAX:
		return GL_MAX;
	default:
		return GL_FUNC_ADD;
	}
}

static void rtgl_bind_uniform_block(rtgl_uniform_location* location) {
	if (!location || !location->program || !location->program->gl_program || location->kind != RTGL_UNIFORM_LOCATION_UNIFORM_BUFFER) {
		return;
	}
	GLuint block = glGetUniformBlockIndex(location->program->gl_program, location->name);
	if (block != GL_INVALID_INDEX) {
		glUniformBlockBinding(location->program->gl_program, block, location->binding);
	}
}

static void rtgl_bind_uniform_texture(struct rtgl_context* ctx, rtgl_uniform_location* location) {
	if (!location || !location->program || !location->program->gl_program) {
		return;
	}
	location->gl_location = glGetUniformLocation(location->program->gl_program, location->name);
	if (location->gl_location < 0) {
		return;
	}
	if (ctx->execution.separate_shader_objects) {
		glProgramUniform1i(location->program->gl_program, location->gl_location, (GLint)location->binding);
	} else {
		glUniform1i(location->gl_location, (GLint)location->binding);
	}
}

static void rtgl_bind_vertex_layout(struct rtgl_context* ctx, struct rtgl_graphics_program* program, struct rtgl_buffer* buffer, u64 offset, GLuint vao) {
	if (!program || !buffer || !program->vertex_layout.attribute_count) {
		return;
	}
	if (ctx->execution.direct_state_access) {
		glVertexArrayVertexBuffer(vao, 0, buffer->gl_buffer, (GLintptr)offset, (GLsizei)program->vertex_layout.stride);
	} else {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, buffer->gl_buffer);
	}
	for (u32 i = 0; i < program->vertex_layout.attribute_count; i++) {
		const rt_vertex_attribute* attribute = &program->vertex_attributes[i];
		GLint components = rtgl_vertex_attribute_components(attribute->format);
		if (!components) {
			rtgl_throwf(RT_UNSUPPORTED_FEATURE, "unsupported OpenGL vertex attribute format");
			return;
		}
		if (ctx->execution.direct_state_access) {
			glEnableVertexArrayAttrib(vao, i);
			glVertexArrayAttribFormat(vao, i, components, GL_FLOAT, GL_FALSE, attribute->offset);
			glVertexArrayAttribBinding(vao, i, 0);
		} else {
			glEnableVertexAttribArray(i);
			glVertexAttribPointer(i, components, GL_FLOAT, GL_FALSE, (GLsizei)program->vertex_layout.stride, (const void*)(uintptr_t)(offset + attribute->offset));
		}
	}
	if (!ctx->execution.direct_state_access) {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void rtgl_command_buffer_execute(struct rtgl_context* ctx, struct rtgl_command_buffer* command_buffer, struct rtgl_queue* queue, u64 complete_value) {
	struct rtgl_framebuffer* framebuffer = NULL;
	struct rtgl_texture_view* depth_view = NULL;
	struct rtgl_graphics_program* program = NULL;
	struct rtgl_buffer* vertex_buffer = NULL;
	u64 vertex_offset = 0;

	u32 command_offset = 0;
	while (command_offset < command_buffer->command_count) {
		const rtgl_recorded_command* command = &command_buffer->commands[command_offset];
		if (command->size != sizeof(*command)) {
			rtgl_throwf(RT_IMPROPER_USAGE, "OpenGL command buffer contains an invalid record size");
			break;
		}
		switch (command->kind) {
		case RTGL_RECORDED_COMMAND_BEGIN_RENDERING: {
			framebuffer = command->data.begin_rendering.framebuffer;
			depth_view = command->data.begin_rendering.depth_view;
			struct rtgl_texture_view* color_view = command->data.begin_rendering.color_view;
			if (!framebuffer || !rtgl_texture_view_valid(color_view)) {
				break;
			}
			GLsizei width = (GLsizei)color_view->image->width;
			GLsizei height = (GLsizei)color_view->image->height;
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->gl_framebuffer);
			glViewport(0, 0, width, height);
			if (color_view->image->gl_internal_format == GL_SRGB8_ALPHA8)
				glEnable(GL_FRAMEBUFFER_SRGB);
			else
				glDisable(GL_FRAMEBUFFER_SRGB);
			glDisable(GL_SCISSOR_TEST);
			glDisable(GL_STENCIL_TEST);
			glDisable(GL_BLEND);
			glDisable(GL_CULL_FACE);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glDepthMask(GL_TRUE);
			if (rtgl_texture_view_valid(depth_view)) {
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
			} else
				glDisable(GL_DEPTH_TEST);
			break;
		}
		case RTGL_RECORDED_COMMAND_CLEAR_COLOR:
			if (framebuffer && command->data.clear_color.color_index < framebuffer->color_texture_count) {
				if (ctx->execution.direct_state_access)
					glClearNamedFramebufferfv(framebuffer->gl_framebuffer, GL_COLOR, (GLint)command->data.clear_color.color_index, command->data.clear_color.values);
				else
					glClearBufferfv(GL_COLOR, (GLint)command->data.clear_color.color_index, command->data.clear_color.values);
			}
			break;
		case RTGL_RECORDED_COMMAND_CLEAR_DEPTH:
			if (framebuffer && rtgl_texture_view_valid(depth_view)) {
				if (ctx->execution.direct_state_access)
					glClearNamedFramebufferfv(framebuffer->gl_framebuffer, GL_DEPTH, 0, &command->data.clear_depth);
				else
					glClearBufferfv(GL_DEPTH, 0, &command->data.clear_depth);
			}
			break;
		case RTGL_RECORDED_COMMAND_USE_GRAPHICS_PROGRAM:
			program = command->data.use_graphics_program.program;
			rtgl_graphics_program_prepare(ctx, program);
			if (!program || !program->gl_program)
				break;
			glUseProgram(program->gl_program);
			if (program->cull_mode == RT_CULL_NONE)
				glDisable(GL_CULL_FACE);
			else {
				glEnable(GL_CULL_FACE);
				glCullFace(program->cull_mode == RT_CULL_FRONT ? GL_FRONT : GL_BACK);
			}
			glFrontFace(program->front_face == RT_FRONT_FACE_CW ? GL_CW : GL_CCW);
			glPolygonMode(GL_FRONT_AND_BACK, program->fill_mode == RT_FILL_WIREFRAME ? GL_LINE : GL_FILL);
			if (program->blend_enabled) {
				glEnable(GL_BLEND);
				glBlendFuncSeparate(rtgl_blend_factor(program->src_color_blend), rtgl_blend_factor(program->dst_color_blend), rtgl_blend_factor(program->src_alpha_blend), rtgl_blend_factor(program->dst_alpha_blend));
				glBlendEquationSeparate(rtgl_blend_op(program->color_blend_op), rtgl_blend_op(program->alpha_blend_op));
			} else
				glDisable(GL_BLEND);
			break;
		case RTGL_RECORDED_COMMAND_SET_SCISSOR:
			glEnable(GL_SCISSOR_TEST);
			if (framebuffer && framebuffer->color_views[0] && framebuffer->color_views[0]->image) {
				u32 height = framebuffer->color_views[0]->image->height;
				u32 y = command->data.set_scissor.y + command->data.set_scissor.height <= height ? height - command->data.set_scissor.y - command->data.set_scissor.height : 0;
				glScissor((GLint)command->data.set_scissor.x, (GLint)y, (GLsizei)command->data.set_scissor.width, (GLsizei)command->data.set_scissor.height);
			} else
				glScissor((GLint)command->data.set_scissor.x, (GLint)command->data.set_scissor.y, (GLsizei)command->data.set_scissor.width, (GLsizei)command->data.set_scissor.height);
			break;
		case RTGL_RECORDED_COMMAND_UNIFORM_BUFFER:
			if (command->data.uniform_buffer.location && command->data.uniform_buffer.buffer) {
				rtgl_bind_uniform_block(command->data.uniform_buffer.location);
				GLuint block = glGetUniformBlockIndex(command->data.uniform_buffer.location->program->gl_program, command->data.uniform_buffer.location->name);
				glBindBufferRange(GL_UNIFORM_BUFFER, command->data.uniform_buffer.location->binding, command->data.uniform_buffer.buffer->gl_buffer, (GLintptr)command->data.uniform_buffer.offset, (GLsizeiptr)command->data.uniform_buffer.size);
			}
			break;
		case RTGL_RECORDED_COMMAND_UNIFORM_TEXTURE:
			if (command->data.uniform_texture.location && rtgl_texture_view_valid(command->data.uniform_texture.texture_view)) {
				rtgl_bind_uniform_texture(ctx, command->data.uniform_texture.location);
				if (ctx->execution.direct_state_access)
					glBindTextureUnit(command->data.uniform_texture.location->binding, command->data.uniform_texture.texture_view->image->gl_texture);
				else {
					glActiveTexture(GL_TEXTURE0 + command->data.uniform_texture.location->binding);
					glBindTexture(GL_TEXTURE_2D, command->data.uniform_texture.texture_view->image->gl_texture);
				}
			}
			break;
		case RTGL_RECORDED_COMMAND_STORAGE_BUFFER:
			if (command->data.storage_buffer.buffer && ctx->execution.shader_storage_buffer) {
				GLuint block = glGetProgramResourceIndex(command->data.storage_buffer.location->program->gl_program, GL_SHADER_STORAGE_BLOCK, command->data.storage_buffer.location->name);
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, command->data.storage_buffer.location->binding, command->data.storage_buffer.buffer->gl_buffer, (GLintptr)command->data.storage_buffer.offset, (GLsizeiptr)command->data.storage_buffer.size);
			}
			break;
		case RTGL_RECORDED_COMMAND_BIND_VERTEX_BUFFER:
			vertex_buffer = command->data.bind_vertex_buffer.buffer;
			vertex_offset = command->data.bind_vertex_buffer.offset;
			break;
		case RTGL_RECORDED_COMMAND_DRAW:
			if (program && vertex_buffer && command->data.draw.vertex_count) {
				GLuint vao = 0;
				if (ctx->execution.direct_state_access)
					glCreateVertexArrays(1, &vao);
				else
					glGenVertexArrays(1, &vao);
				rtgl_bind_vertex_layout(ctx, program, vertex_buffer, vertex_offset, vao);
				glBindVertexArray(vao);
				glDrawArrays(GL_TRIANGLES, (GLint)command->data.draw.first_vertex, (GLsizei)command->data.draw.vertex_count);
				glBindVertexArray(0);
				glDeleteVertexArrays(1, &vao);
			}
			break;
		case RTGL_RECORDED_COMMAND_END_RENDERING:
			framebuffer = NULL;
			depth_view = NULL;
			glDisable(GL_FRAMEBUFFER_SRGB);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			break;
		}
		command_offset += command->size / sizeof(*command);
	}
	rtgl_execution_lock(ctx);
	rtgl_execution_queue_complete_locked(queue, complete_value);
	rtgl_execution_unlock(ctx);
}
