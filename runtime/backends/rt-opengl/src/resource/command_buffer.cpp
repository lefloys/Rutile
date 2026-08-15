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
	if (!command_buffer || !command_buffer->recording) {
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
	case RTGL_RECORDED_COMMAND_BIND_BUFFER:
		rtgl_release_resource(command->data.bind_buffer.location_program);
		rtgl_release_resource(command->data.bind_buffer.buffer);
		break;
	case RTGL_RECORDED_COMMAND_BIND_TEXTURE:
		rtgl_release_resource(command->data.bind_texture.location_program);
		rtgl_release_resource(command->data.bind_texture.texture_view);
		break;
	case RTGL_RECORDED_COMMAND_VERTEX_BUFFER:
		rtgl_release_resource(command->data.vertex_buffer.location_program);
		rtgl_release_resource(command->data.vertex_buffer.buffer);
		break;
	case RTGL_RECORDED_COMMAND_INDEX_BUFFER:
		rtgl_release_resource(command->data.index_buffer.buffer);
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

static void rtgl_record_location_program(struct rt_location_t* location, struct rtgl_graphics_program** program) {
	*program = location ? location->program : NULL;
	if (*program) {
		rtgl_retain_resource(*program);
	}
}
rt_command_buffer rtCommandBufferCreate(void) {
	return rtgl_command_buffer_to_handle(rtgl_command_buffer_create(rtgl_get_current_context()));
}

void rtCommandBufferDestroy(rt_command_buffer command_buffer) {
	rtgl_command_buffer_destroy(rtgl_get_current_context(), rtgl_command_buffer_from_handle(command_buffer));
}

void rtgl_command_buffer_reset(struct rtgl_command_buffer* command_buffer) {
	if (!command_buffer) {
		return;
	}
	rtgl_command_buffer_clear_commands(command_buffer);
	command_buffer->recording = false;
	command_buffer->executable = false;
}

void rtgl_command_buffer_begin(struct rtgl_command_buffer* command_buffer) {
	if (!command_buffer) {
		return;
	}
	command_buffer->recording = true;
}

void rtgl_command_buffer_wait(struct rtgl_command_buffer* command_buffer, rt_timepoint timepoint) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_WAIT;
	command->data.wait = timepoint;
}

void rtgl_command_buffer_begin_rendering(struct rtgl_command_buffer* command_buffer, struct rtgl_framebuffer* framebuffer) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_BEGIN_RENDERING;
	command->data.begin_rendering.framebuffer = framebuffer;
	command->data.begin_rendering.color_view = framebuffer ? framebuffer->color_views[0] : NULL;
	command->data.begin_rendering.depth_view = framebuffer ? framebuffer->depth_view : NULL;
	rtgl_retain_resource(framebuffer);
	rtgl_retain_resource(command->data.begin_rendering.color_view);
	rtgl_retain_resource(command->data.begin_rendering.depth_view);
}

void rtgl_command_buffer_clear_color(struct rtgl_command_buffer* command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_CLEAR_COLOR;
	command->data.clear_color.color_index = color_index;
	command->data.clear_color.values[0] = r;
	command->data.clear_color.values[1] = g;
	command->data.clear_color.values[2] = b;
	command->data.clear_color.values[3] = a;
}

void rtgl_command_buffer_clear_depth(struct rtgl_command_buffer* command_buffer, f32 depth) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_CLEAR_DEPTH;
	command->data.clear_depth = depth;
}

void rtgl_command_buffer_clear_stencil(struct rtgl_command_buffer* command_buffer, u32 stencil) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_CLEAR_STENCIL;
	command->data.clear_stencil = stencil;
}

void rtgl_command_buffer_set_viewport(struct rtgl_command_buffer* command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_SET_VIEWPORT;
	command->data.set_viewport = { x, y, width, height, min_depth, max_depth };
}

void rtgl_command_buffer_use_graphics_program(struct rtgl_command_buffer* command_buffer, struct rtgl_graphics_program* program) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_USE_GRAPHICS_PROGRAM;
	command->data.use_graphics_program.program = program;
	rtgl_retain_resource(command->data.use_graphics_program.program);
}

void rtgl_command_buffer_set_scissor(struct rtgl_command_buffer* command_buffer, u32 x, u32 y, u32 width, u32 height) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_SET_SCISSOR;
	command->data.set_scissor = { x, y, width, height };
}

void rtgl_command_buffer_bind_buffer(struct rtgl_command_buffer* command_buffer, struct rt_location_t* location, struct rtgl_buffer* buffer, usize offset, usize size) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_BIND_BUFFER;
	command->data.bind_buffer.location = location;
	command->data.bind_buffer.buffer = buffer;
	command->data.bind_buffer.offset = offset;
	command->data.bind_buffer.size = size;
	rtgl_record_location_program(command->data.bind_buffer.location, &command->data.bind_buffer.location_program);
	rtgl_retain_resource(command->data.bind_buffer.buffer);
}

void rtgl_command_buffer_bind_texture(struct rtgl_command_buffer* command_buffer, struct rt_location_t* location, struct rtgl_texture_view* texture_view) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_BIND_TEXTURE;
	command->data.bind_texture.location = location;
	command->data.bind_texture.texture_view = texture_view;
	rtgl_record_location_program(command->data.bind_texture.location, &command->data.bind_texture.location_program);
	rtgl_retain_resource(command->data.bind_texture.texture_view);
}

void rtgl_command_buffer_vertex_buffer(struct rtgl_command_buffer* command_buffer, struct rt_location_t* location, struct rtgl_buffer* buffer, usize offset) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_VERTEX_BUFFER;
	command->data.vertex_buffer.location = location;
	rtgl_record_location_program(command->data.vertex_buffer.location, &command->data.vertex_buffer.location_program);
	command->data.vertex_buffer.buffer = buffer;
	command->data.vertex_buffer.offset = offset;
	rtgl_retain_resource(command->data.vertex_buffer.buffer);
}

void rtgl_command_buffer_index_buffer(struct rtgl_command_buffer* command_buffer, struct rtgl_buffer* buffer, usize offset, enum rt_index_format format) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_INDEX_BUFFER;
	command->data.index_buffer.buffer = buffer;
	command->data.index_buffer.offset = offset;
	command->data.index_buffer.format = format;
	rtgl_retain_resource(command->data.index_buffer.buffer);
}

void rtgl_command_buffer_draw(struct rtgl_command_buffer* command_buffer, u32 vertex_count, u32 first_vertex) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_DRAW;
	command->data.draw = { vertex_count, first_vertex };
}

void rtgl_command_buffer_draw_instanced(struct rtgl_command_buffer* command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_DRAW_INSTANCED;
	command->data.draw_instanced = { vertex_count, instance_count, first_vertex, first_instance };
}

void rtgl_command_buffer_draw_indexed(struct rtgl_command_buffer* command_buffer, u32 index_count, u32 first_index, i32 vertex_offset) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_DRAW_INDEXED;
	command->data.draw_indexed = { index_count, first_index, vertex_offset };
}

void rtgl_command_buffer_draw_indexed_instanced(struct rtgl_command_buffer* command_buffer, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_DRAW_INDEXED_INSTANCED;
	command->data.draw_indexed_instanced = { index_count, instance_count, first_index, vertex_offset, first_instance };
}

void rtgl_command_buffer_end_rendering(struct rtgl_command_buffer* command_buffer) {
	rtgl_recorded_command* command = rtgl_command_buffer_append(command_buffer);
	if (!command) {
		return;
	}
	command->kind = RTGL_RECORDED_COMMAND_END_RENDERING;
}

void rtgl_command_buffer_end(struct rtgl_command_buffer* command_buffer) {
	if (!command_buffer) {
		return;
	}
	command_buffer->recording = false;
	command_buffer->executable = true;
}

void rtCmdReset(rt_command_buffer command_buffer) {
	rtgl_command_buffer_reset(rtgl_command_buffer_from_handle(command_buffer));
}

void rtCmdBegin(rt_command_buffer command_buffer) {
	rtgl_command_buffer_begin(rtgl_command_buffer_from_handle(command_buffer));
}

void rtCmdWait(rt_command_buffer command_buffer, rt_timepoint timepoint) {
	rtgl_command_buffer_wait(rtgl_command_buffer_from_handle(command_buffer), timepoint);
}

void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer) {
	rtgl_command_buffer_begin_rendering(rtgl_command_buffer_from_handle(command_buffer), rtgl_framebuffer_from_handle(framebuffer));
}

void rtCmdClearColor(rt_command_buffer command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a) {
	rtgl_command_buffer_clear_color(rtgl_command_buffer_from_handle(command_buffer), color_index, r, g, b, a);
}

void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth) {
	rtgl_command_buffer_clear_depth(rtgl_command_buffer_from_handle(command_buffer), depth);
}

void rtCmdClearStencil(rt_command_buffer command_buffer, u32 stencil) {
	rtgl_command_buffer_clear_stencil(rtgl_command_buffer_from_handle(command_buffer), stencil);
}

void rtCmdSetViewport(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth) {
	rtgl_command_buffer_set_viewport(rtgl_command_buffer_from_handle(command_buffer), x, y, width, height, min_depth, max_depth);
}

void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program) {
	rtgl_command_buffer_use_graphics_program(rtgl_command_buffer_from_handle(command_buffer), rtgl_graphics_program_from_handle(program));
}

void rtCmdSetScissor(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height) {
	rtgl_command_buffer_set_scissor(rtgl_command_buffer_from_handle(command_buffer), x, y, width, height);
}

void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset, usize size) {
	rtgl_command_buffer_bind_buffer(rtgl_command_buffer_from_handle(command_buffer), (struct rt_location_t*)location, rtgl_buffer_from_handle(buffer), offset, size);
}

void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view) {
	rtgl_command_buffer_bind_texture(rtgl_command_buffer_from_handle(command_buffer), (struct rt_location_t*)location, rtgl_texture_view_from_handle(texture_view));
}

void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset) {
	rtgl_command_buffer_vertex_buffer(rtgl_command_buffer_from_handle(command_buffer), (struct rt_location_t*)location, rtgl_buffer_from_handle(buffer), offset);
}

void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, usize offset, enum rt_index_format format) {
	rtgl_command_buffer_index_buffer(rtgl_command_buffer_from_handle(command_buffer), rtgl_buffer_from_handle(buffer), offset, format);
}

void rtCmdDraw(rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex) {
	rtgl_command_buffer_draw(rtgl_command_buffer_from_handle(command_buffer), vertex_count, first_vertex);
}

void rtCmdDrawInstanced(rt_command_buffer command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
	rtgl_command_buffer_draw_instanced(rtgl_command_buffer_from_handle(command_buffer), vertex_count, instance_count, first_vertex, first_instance);
}

void rtCmdDrawIndexed(rt_command_buffer command_buffer, u32 index_count, u32 first_index, i32 vertex_offset) {
	rtgl_command_buffer_draw_indexed(rtgl_command_buffer_from_handle(command_buffer), index_count, first_index, vertex_offset);
}

void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) {
	rtgl_command_buffer_draw_indexed_instanced(rtgl_command_buffer_from_handle(command_buffer), index_count, instance_count, first_index, vertex_offset, first_instance);
}

void rtCmdEndRendering(rt_command_buffer command_buffer) {
	rtgl_command_buffer_end_rendering(rtgl_command_buffer_from_handle(command_buffer));
}

void rtCmdEnd(rt_command_buffer command_buffer) {
	rtgl_command_buffer_end(rtgl_command_buffer_from_handle(command_buffer));
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

rt_timepoint rtgl_command_buffer_submit(struct rtgl_context* ctx, struct rtgl_queue* queue, struct rtgl_command_buffer* command_buffer) {
	rt_timepoint wait_timepoint = queue->pending_wait;
	queue->pending_wait = { 0 };
	rt_timepoint done = rtgl_queue_signal(queue);
	if (!command_buffer) {
		rtgl_execution_queue_complete(ctx, queue, rtgl_timepoint_queue_value(done));
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
	rtgl_retain_resource(command_buffer);
	if (!rtgl_execution_submit_async(ctx, [command_buffer, submission, queue, wait_timepoint, value = rtgl_timepoint_queue_value(done)](struct rtgl_context* exec_ctx) {
			/* Execute the detached list through a local view.  The logical command
			 * buffer may already be recording the next frame on another thread. */
			struct rtgl_command_buffer command_view = *command_buffer;
			command_view.commands = submission->commands;
			command_view.command_count = submission->command_count;
			rtgl_timepoint_wait(exec_ctx, wait_timepoint);
			rtgl_command_buffer_execute(exec_ctx, &command_view, queue, value);
			rtgl_command_buffer_submission_destroy(submission);
			rtgl_resource_release(RTGL_RESOURCE_BASE(command_buffer));
		})) {
		rtgl_command_buffer_submission_destroy(submission);
		rtgl_resource_release(RTGL_RESOURCE_BASE(command_buffer));
		rtgl_execution_queue_complete(ctx, queue, rtgl_timepoint_queue_value(done));
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
	glProgramUniform1i(location->program->gl_program, location->gl_location, (GLint)location->binding);
}

static void rtgl_bind_vertex_layout(struct rtgl_context* ctx, struct rtgl_graphics_program* program, struct rtgl_buffer* const* buffers, const u64* offsets, GLuint vao) {
	if (!program || !program->vertex_layout.attribute_count) {
		return;
	}
	for (u32 stream = 0; stream < program->vertex_layout.stream_count; stream++) {
		if (!buffers[stream]) {
			continue;
		}
		glVertexArrayVertexBuffer(vao, stream, buffers[stream]->gl_buffer, (GLintptr)offsets[stream], (GLsizei)program->vertex_streams[stream].stride);
		glVertexArrayBindingDivisor(vao, stream, program->vertex_streams[stream].rate == RT_VERTEX_RATE_INSTANCE ? 1 : 0);
	}
	for (u32 i = 0; i < program->vertex_layout.attribute_count; i++) {
		const rt_vertex_attribute* attribute = &program->vertex_attributes[i];
		const u32 stream = (u32)attribute->stream;
		if (stream >= program->vertex_layout.stream_count || !buffers[stream]) {
			continue;
		}
		GLint components = rtgl_vertex_attribute_components(attribute->format);
		if (!components) {
			rtgl_throwf(RT_UNSUPPORTED_FEATURE, "unsupported OpenGL vertex attribute format");
			return;
		}
		glEnableVertexArrayAttrib(vao, i);
		glVertexArrayAttribFormat(vao, i, components, GL_FLOAT, GL_FALSE, attribute->offset);
		glVertexArrayAttribBinding(vao, i, stream);
	}
}

void rtgl_command_buffer_execute(struct rtgl_context* ctx, struct rtgl_command_buffer* command_buffer, struct rtgl_queue* queue, u64 complete_value) {
	struct rtgl_framebuffer* framebuffer = NULL;
	struct rtgl_texture_view* depth_view = NULL;
	struct rtgl_graphics_program* program = NULL;
	struct rtgl_buffer* vertex_buffers[RTGL_MAX_VERTEX_ATTRIBUTES] = { 0 };
	u64 vertex_offsets[RTGL_MAX_VERTEX_ATTRIBUTES] = { 0 };
	struct rtgl_buffer* index_buffer = NULL;
	u64 index_offset = 0;
	enum rt_index_format index_format = RT_INDEX_U16;

	u32 command_offset = 0;
	while (command_offset < command_buffer->command_count) {
		const rtgl_recorded_command* command = &command_buffer->commands[command_offset];
		if (command->size != sizeof(*command)) {
			break;
		}
		switch (command->kind) {
		case RTGL_RECORDED_COMMAND_WAIT:
			rtgl_timepoint_wait(ctx, command->data.wait);
			break;
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
				glClearNamedFramebufferfv(framebuffer->gl_framebuffer, GL_COLOR, (GLint)command->data.clear_color.color_index, command->data.clear_color.values);
			}
			break;
		case RTGL_RECORDED_COMMAND_CLEAR_DEPTH:
			if (framebuffer && rtgl_texture_view_valid(depth_view)) {
				glClearNamedFramebufferfv(framebuffer->gl_framebuffer, GL_DEPTH, 0, &command->data.clear_depth);
			}
			break;
		case RTGL_RECORDED_COMMAND_CLEAR_STENCIL:
			if (framebuffer && rtgl_texture_view_valid(depth_view)) {
				glClearNamedFramebufferuiv(framebuffer->gl_framebuffer, GL_STENCIL, 0, &command->data.clear_stencil);
			}
			break;
		case RTGL_RECORDED_COMMAND_SET_VIEWPORT:
			glViewport((GLint)command->data.set_viewport.x, (GLint)command->data.set_viewport.y, (GLsizei)command->data.set_viewport.width, (GLsizei)command->data.set_viewport.height);
			glDepthRangef(command->data.set_viewport.min_depth, command->data.set_viewport.max_depth);
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
		case RTGL_RECORDED_COMMAND_BIND_BUFFER:
			if (command->data.bind_buffer.location && command->data.bind_buffer.buffer) {
				if (command->data.bind_buffer.location->kind == RTGL_UNIFORM_LOCATION_UNIFORM_BUFFER) {
					rtgl_bind_uniform_block(command->data.bind_buffer.location);
					glBindBufferRange(GL_UNIFORM_BUFFER, command->data.bind_buffer.location->binding, command->data.bind_buffer.buffer->gl_buffer, (GLintptr)command->data.bind_buffer.offset, (GLsizeiptr)command->data.bind_buffer.size);
				} else if (command->data.bind_buffer.location->kind == RTGL_UNIFORM_LOCATION_STORAGE_BUFFER) {
					glBindBufferRange(GL_SHADER_STORAGE_BUFFER, command->data.bind_buffer.location->binding, command->data.bind_buffer.buffer->gl_buffer, (GLintptr)command->data.bind_buffer.offset, (GLsizeiptr)command->data.bind_buffer.size);
				}
			}
			break;
		case RTGL_RECORDED_COMMAND_BIND_TEXTURE:
			if (command->data.bind_texture.location && rtgl_texture_view_valid(command->data.bind_texture.texture_view)) {
				rtgl_bind_uniform_texture(ctx, command->data.bind_texture.location);
				rtgl_texture_view_materialize(ctx, command->data.bind_texture.texture_view);
				glBindTextureUnit(command->data.bind_texture.location->binding, command->data.bind_texture.texture_view->image->gl_texture);
				glBindSampler(command->data.bind_texture.location->binding, command->data.bind_texture.texture_view->gl_sampler);
			}
			break;
		case RTGL_RECORDED_COMMAND_VERTEX_BUFFER:
			if (command->data.vertex_buffer.location && command->data.vertex_buffer.location->kind == RTGL_UNIFORM_LOCATION_VERTEX_STREAM) {
				const u32 stream = command->data.vertex_buffer.location->binding;
				if (stream < RTGL_MAX_VERTEX_ATTRIBUTES) {
					vertex_buffers[stream] = command->data.vertex_buffer.buffer;
					vertex_offsets[stream] = command->data.vertex_buffer.offset;
				}
			}
			break;
		case RTGL_RECORDED_COMMAND_INDEX_BUFFER:
			index_buffer = command->data.index_buffer.buffer;
			index_offset = command->data.index_buffer.offset;
			index_format = command->data.index_buffer.format;
			break;
		case RTGL_RECORDED_COMMAND_DRAW:
			if (program && command->data.draw.vertex_count) {
				GLuint vao = 0;
				glCreateVertexArrays(1, &vao);
				rtgl_bind_vertex_layout(ctx, program, vertex_buffers, vertex_offsets, vao);
				glBindVertexArray(vao);
				glDrawArrays(GL_TRIANGLES, (GLint)command->data.draw.first_vertex, (GLsizei)command->data.draw.vertex_count);
				glBindVertexArray(0);
				glDeleteVertexArrays(1, &vao);
			}
			break;
		case RTGL_RECORDED_COMMAND_DRAW_INSTANCED:
			if (program && command->data.draw_instanced.vertex_count && command->data.draw_instanced.instance_count) {
				GLuint vao = 0;
				glCreateVertexArrays(1, &vao);
				rtgl_bind_vertex_layout(ctx, program, vertex_buffers, vertex_offsets, vao);
				glBindVertexArray(vao);
				glDrawArraysInstancedBaseInstance(GL_TRIANGLES, (GLint)command->data.draw_instanced.first_vertex, (GLsizei)command->data.draw_instanced.vertex_count, (GLsizei)command->data.draw_instanced.instance_count, command->data.draw_instanced.first_instance);
				glBindVertexArray(0);
				glDeleteVertexArrays(1, &vao);
			}
			break;
		case RTGL_RECORDED_COMMAND_DRAW_INDEXED:
		case RTGL_RECORDED_COMMAND_DRAW_INDEXED_INSTANCED:
			if (program && index_buffer) {
				const bool instanced = command->kind == RTGL_RECORDED_COMMAND_DRAW_INDEXED_INSTANCED;
				const u32 index_count = instanced ? command->data.draw_indexed_instanced.index_count : command->data.draw_indexed.index_count;
				const u32 first_index = instanced ? command->data.draw_indexed_instanced.first_index : command->data.draw_indexed.first_index;
				const i32 vertex_offset = instanced ? command->data.draw_indexed_instanced.vertex_offset : command->data.draw_indexed.vertex_offset;
				const GLenum type = index_format == RT_INDEX_U32 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
				const usize index_size = index_format == RT_INDEX_U32 ? sizeof(u32) : sizeof(u16);
				GLuint vao = 0;
				glCreateVertexArrays(1, &vao);
				glVertexArrayElementBuffer(vao, index_buffer->gl_buffer);
				rtgl_bind_vertex_layout(ctx, program, vertex_buffers, vertex_offsets, vao);
				glBindVertexArray(vao);
				const void* indices = (const void*)(uintptr_t)(index_offset + (usize)first_index * index_size);
				if (instanced) {
					glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, (GLsizei)index_count, type, indices, (GLsizei)command->data.draw_indexed_instanced.instance_count, vertex_offset, command->data.draw_indexed_instanced.first_instance);
				} else {
					glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)index_count, type, indices, vertex_offset);
				}
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
