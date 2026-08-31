#include "command_buffer.h"

#include "context.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>

static bool rtsw_buffer_valid_range(const struct rtsw_buffer* buffer, rt_buffer_range range) {
	return buffer && range.offset <= buffer->size && range.size <= buffer->size - range.offset;
}

static void rtsw_command_buffer_finish(struct rtsw_command_buffer* command_buffer) {
	rtsw_command_buffer_release_resources(command_buffer);
	free(command_buffer->ir_data);
	command_buffer->ir_data = NULL;
	command_buffer->ir_size = 0;
	command_buffer->ir_capacity = 0;
}

static void rtsw_command_buffer_finalize_resource(void* value) {
	struct rtsw_command_buffer* command_buffer = value;
	rtsw_command_buffer_finish(command_buffer);
	free(command_buffer);
}

RTSW_DEFINE_HANDLE(command_buffer, rtsw_command_buffer)

rt_command_buffer rtCommandBufferCreate(void) {
	rtsw_clear_error();
	struct rtsw_context* ctx = rtsw_get_current_context();
	struct rtsw_command_buffer* command_buffer;
	if (!ctx) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCommandBufferCreate called before rtInit");
		return RT_NULL_HANDLE;
	}
	command_buffer = RTSW_ALLOC_RESOURCE(struct rtsw_command_buffer);
	if (!command_buffer) {
		return RT_NULL_HANDLE;
	}
	rtsw_init_resource_base(ctx, &command_buffer->base, command_buffer, rtsw_command_buffer_finalize_resource);
	return rtsw_command_buffer_to_handle(command_buffer);
}

void rtCommandBufferDestroy(rt_command_buffer handle) {
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	if (command_buffer) {
		rtsw_resource_retire(&command_buffer->base);
	}
}

void rtCommandBufferReset(rt_command_buffer handle) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	if (!command_buffer || command_buffer->recording) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCommandBufferReset requires an idle command buffer");
		return;
	}
	rtsw_command_buffer_release_resources(command_buffer);
	command_buffer->executable = false;
}

void rtCommandBufferBegin(rt_command_buffer handle) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	if (!command_buffer || command_buffer->recording) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCommandBufferBegin requires an idle command buffer");
		return;
	}
	rtsw_command_buffer_release_resources(command_buffer);
	command_buffer->recording = true;
	command_buffer->executable = false;
	command_buffer->rendering = false;
	command_buffer->rendering_continuation = false;
}

void rtCommandBufferContinue(rt_command_buffer handle) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	if (!command_buffer || command_buffer->recording) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCommandBufferContinue requires an idle command buffer");
		return;
	}
	rtsw_command_buffer_release_resources(command_buffer);
	command_buffer->recording = true;
	command_buffer->executable = false;
	command_buffer->rendering = false;
	command_buffer->rendering_continuation = false;
}

void rtCommandBufferContinueRendering(rt_command_buffer handle) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	if (!command_buffer || command_buffer->recording) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCommandBufferContinueRendering requires an idle command buffer");
		return;
	}
	rtsw_command_buffer_release_resources(command_buffer);
	command_buffer->recording = true;
	command_buffer->executable = false;
	command_buffer->rendering = false;
	command_buffer->rendering_continuation = true;
}

void rtsw_command_buffer_release_resources(struct rtsw_command_buffer* command_buffer) {
	usize offset = 0;
	while (offset < command_buffer->ir_size) {
		struct rtsw_command_header* header = (struct rtsw_command_header*)(command_buffer->ir_data + offset);
		usize payload_size = rtsw_command_record_size((enum rtsw_command_opcode)header->opcode);
		void* payload = header + 1;
		switch ((enum rtsw_command_opcode)header->opcode) {
		case RTSW_COMMAND_BUFFER_DATA: {
			struct rtsw_ir_buffer_data* command = payload;
			free(command->data);
			rtsw_resource_release(&command->buffer->base);
			break;
		}
		case RTSW_COMMAND_BUFFER_COPY: {
			struct rtsw_ir_buffer_copy* command = payload;
			rtsw_resource_release(&command->src->base);
			rtsw_resource_release(&command->dst->base);
			break;
		}
		case RTSW_COMMAND_BUFFER_COPY_TO_TEXTURE: {
			struct rtsw_ir_buffer_copy_to_texture* command = payload;
			rtsw_resource_release(&command->src->base);
			rtsw_resource_release(&command->dst->base);
			break;
		}
		case RTSW_COMMAND_TEXTURE_DATA: {
			struct rtsw_ir_texture_data* command = payload;
			free(command->data);
			rtsw_resource_release(&command->texture->base);
			break;
		}
		case RTSW_COMMAND_TEXTURE_COPY: {
			struct rtsw_ir_texture_copy* command = payload;
			rtsw_resource_release(&command->src->base);
			rtsw_resource_release(&command->dst->base);
			break;
		}
		case RTSW_COMMAND_TEXTURE_COPY_TO_BUFFER: {
			struct rtsw_ir_texture_copy_to_buffer* command = payload;
			rtsw_resource_release(&command->src->base);
			rtsw_resource_release(&command->dst->base);
			break;
		}
		case RTSW_COMMAND_BEGIN_RENDERING: {
			struct rtsw_ir_begin_rendering* command = payload;
			rtsw_resource_release(&command->framebuffer->base);
			break;
		}
		case RTSW_COMMAND_EXECUTE: {
			struct rtsw_ir_execute* command = payload;
			rtsw_resource_release(&command->secondary->base);
			break;
		}
		case RTSW_COMMAND_USE_PROGRAM: {
			struct rtsw_ir_use_program* command = payload;
			rtsw_resource_release(&command->program->base);
			break;
		}
		case RTSW_COMMAND_BIND_BUFFER: {
			struct rtsw_ir_bind_buffer* command = payload;
			rtsw_resource_release(&command->program->base);
			rtsw_resource_release(&command->buffer->base);
			break;
		}
		case RTSW_COMMAND_VERTEX_BUFFER: {
			struct rtsw_ir_vertex_buffer* command = payload;
			rtsw_resource_release(&command->buffer->base);
			break;
		}
		case RTSW_COMMAND_INDEX_BUFFER: {
			struct rtsw_ir_index_buffer* command = payload;
			rtsw_resource_release(&command->buffer->base);
			break;
		}
		default:
			break;
		}
		offset += sizeof(*header) + payload_size;
	}
	command_buffer->ir_size = 0;
}

void rtCommandBufferEnd(rt_command_buffer handle) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	if (!command_buffer || !command_buffer->recording || command_buffer->rendering) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCommandBufferEnd requires a recording command buffer");
		return;
	}
	command_buffer->recording = false;
	command_buffer->executable = true;
}

usize rtsw_command_record_size(enum rtsw_command_opcode opcode) {
	switch (opcode) {
	case RTSW_COMMAND_BUFFER_DATA: return sizeof(struct rtsw_ir_buffer_data);
	case RTSW_COMMAND_BUFFER_COPY: return sizeof(struct rtsw_ir_buffer_copy);
	case RTSW_COMMAND_BUFFER_COPY_TO_TEXTURE: return sizeof(struct rtsw_ir_buffer_copy_to_texture);
	case RTSW_COMMAND_TEXTURE_DATA: return sizeof(struct rtsw_ir_texture_data);
	case RTSW_COMMAND_TEXTURE_COPY: return sizeof(struct rtsw_ir_texture_copy);
	case RTSW_COMMAND_TEXTURE_COPY_TO_BUFFER: return sizeof(struct rtsw_ir_texture_copy_to_buffer);
	case RTSW_COMMAND_BEGIN_RENDERING: return sizeof(struct rtsw_ir_begin_rendering);
	case RTSW_COMMAND_END_RENDERING: return 0;
	case RTSW_COMMAND_CLEAR_COLOR: return sizeof(struct rtsw_ir_clear_color);
	case RTSW_COMMAND_CLEAR_DEPTH: return sizeof(struct rtsw_ir_clear_depth);
	case RTSW_COMMAND_CLEAR_STENCIL: return sizeof(struct rtsw_ir_clear_stencil);
	case RTSW_COMMAND_CLEAR: return sizeof(struct rtsw_ir_clear);
	case RTSW_COMMAND_SET_VIEWPORT: return sizeof(struct rtsw_ir_rectangle);
	case RTSW_COMMAND_SET_SCISSOR: return sizeof(struct rtsw_ir_rectangle);
	case RTSW_COMMAND_EXECUTE: return sizeof(struct rtsw_ir_execute);
	case RTSW_COMMAND_USE_PROGRAM: return sizeof(struct rtsw_ir_use_program);
	case RTSW_COMMAND_BIND_BUFFER: return sizeof(struct rtsw_ir_bind_buffer);
	case RTSW_COMMAND_VERTEX_BUFFER: return sizeof(struct rtsw_ir_vertex_buffer);
	case RTSW_COMMAND_INDEX_BUFFER: return sizeof(struct rtsw_ir_index_buffer);
	case RTSW_COMMAND_DRAW: return sizeof(struct rtsw_ir_draw);
	case RTSW_COMMAND_DRAW_INSTANCED: return sizeof(struct rtsw_ir_draw_instanced);
	case RTSW_COMMAND_DRAW_INDEXED: return sizeof(struct rtsw_ir_draw_indexed);
	case RTSW_COMMAND_DRAW_INDEXED_INSTANCED: return sizeof(struct rtsw_ir_draw_indexed_instanced);
	default: return 0;
	}
}

void rtCmdExecute(rt_command_buffer handle, rt_command_buffer secondary_handle) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	struct rtsw_command_buffer* secondary = rtsw_command_buffer_from_handle(secondary_handle);
	struct rtsw_ir_execute* command;

	if (!command_buffer || !command_buffer->recording || !secondary || !secondary->executable ||
		command_buffer->rendering != secondary->rendering_continuation) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdExecute requires an executable continuation matching the active rendering scope");
		return;
	}
	command = rtsw_command_buffer_append(command_buffer, RTSW_COMMAND_EXECUTE, sizeof(*command));
	if (!command) {
		return;
	}
	command->secondary = secondary;
	rtsw_resource_retain(&secondary->base);
}

void* rtsw_command_buffer_append(struct rtsw_command_buffer* command_buffer, enum rtsw_command_opcode opcode, usize payload_size) {
	usize size = sizeof(struct rtsw_command_header) + payload_size;
	usize required = command_buffer->ir_size + size;
	struct rtsw_command_header* header;
	if (required > command_buffer->ir_capacity) {
		usize capacity = command_buffer->ir_capacity ? command_buffer->ir_capacity * 2 : 1024;
		u08* data;
		while (capacity < required) capacity *= 2;
		data = realloc(command_buffer->ir_data, capacity);
		if (!data) {
			rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to grow command buffer IR");
			return NULL;
		}
		command_buffer->ir_data = data;
		command_buffer->ir_capacity = capacity;
	}
	header = (struct rtsw_command_header*)(command_buffer->ir_data + command_buffer->ir_size);
	header->alignment = NULL;
	header->opcode = (u08)opcode;
	command_buffer->ir_size += size;
	return header + 1;
}

void rtCmdBufferData(rt_command_buffer handle, rt_buffer buffer_handle, rt_buffer_range range, const u08* data) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	struct rtsw_buffer* buffer = rtsw_buffer_from_handle(buffer_handle);
	struct rtsw_ir_buffer_data* command;
	u08* copy;
	if (!command_buffer || !command_buffer->recording || !rtsw_buffer_valid_range(buffer, range) || !data) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdBufferData requires a recording command buffer and valid buffer range");
		return;
	}
	copy = malloc(range.size);
	if (!copy) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to copy buffer command data");
		return;
	}
	memcpy(copy, data, range.size);
	command = rtsw_command_buffer_append(command_buffer, RTSW_COMMAND_BUFFER_DATA, sizeof(*command));
	if (!command) {
		free(copy);
		return;
	}
	command->data = copy;
	command->buffer = buffer;
	command->range = range;
	rtsw_resource_retain(&buffer->base);
}

void rtCmdBufferCopy(rt_command_buffer handle, rt_buffer src_handle, rt_buffer_range src_range, rt_buffer dst_handle, rt_buffer_range dst_range) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	struct rtsw_buffer* src = rtsw_buffer_from_handle(src_handle);
	struct rtsw_buffer* dst = rtsw_buffer_from_handle(dst_handle);
	struct rtsw_ir_buffer_copy* command;
	if (!command_buffer || !command_buffer->recording || !rtsw_buffer_valid_range(src, src_range) || !rtsw_buffer_valid_range(dst, dst_range) || src_range.size != dst_range.size) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdBufferCopy requires matching valid ranges");
		return;
	}
	command = rtsw_command_buffer_append(command_buffer, RTSW_COMMAND_BUFFER_COPY, sizeof(*command));
	if (!command) return;
	command->src = src;
	command->dst = dst;
	command->src_range = src_range;
	command->dst_range = dst_range;
	rtsw_resource_retain(&src->base);
	rtsw_resource_retain(&dst->base);
}

void rtCmdBufferCopyToTexture(rt_command_buffer handle, rt_buffer src_handle, rt_buffer_range src_range, rt_texture dst_handle, rt_texture_range dst_range) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	struct rtsw_buffer* src = rtsw_buffer_from_handle(src_handle);
	struct rtsw_texture* dst = rtsw_texture_from_handle(dst_handle);
	struct rtsw_ir_buffer_copy_to_texture* command;
	if (!command_buffer || !command_buffer->recording || !rtsw_buffer_valid_range(src, src_range) || !rtsw_texture_validate_range(dst, dst_range) || src_range.size != rtsw_texture_range_byte_size(dst, dst_range)) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdBufferCopyToTexture requires matching valid ranges");
		return;
	}
	command = rtsw_command_buffer_append(command_buffer, RTSW_COMMAND_BUFFER_COPY_TO_TEXTURE, sizeof(*command));
	if (!command) return;
	command->src = src;
	command->dst = dst;
	command->src_range = src_range;
	command->dst_range = dst_range;
	rtsw_resource_retain(&src->base);
	rtsw_resource_retain(&dst->base);
}

void rtCmdTextureData(rt_command_buffer handle, rt_texture texture_handle, rt_texture_range range, const u08* data) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	struct rtsw_texture* texture = rtsw_texture_from_handle(texture_handle);
	struct rtsw_ir_texture_data* command;
	u08* copy;
	usize data_size;
	if (!command_buffer || !command_buffer->recording || !rtsw_texture_validate_range(texture, range) || !data) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdTextureData requires a recording command buffer and valid texture range");
		return;
	}
	data_size = rtsw_texture_range_byte_size(texture, range);
	copy = malloc(data_size);
	if (!copy) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to copy texture command data");
		return;
	}
	memcpy(copy, data, data_size);
	command = rtsw_command_buffer_append(command_buffer, RTSW_COMMAND_TEXTURE_DATA, sizeof(*command));
	if (!command) {
		free(copy);
		return;
	}
	command->texture = texture;
	command->range = range;
	command->data = copy;
	command->data_size = data_size;
	rtsw_resource_retain(&texture->base);
}

void rtCmdTextureCopy(rt_command_buffer handle, rt_texture src_handle, rt_texture_range src_range, rt_texture dst_handle, rt_texture_range dst_range) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	struct rtsw_texture* src = rtsw_texture_from_handle(src_handle);
	struct rtsw_texture* dst = rtsw_texture_from_handle(dst_handle);
	struct rtsw_ir_texture_copy* command;
	if (!command_buffer || !command_buffer->recording || !rtsw_texture_validate_range(src, src_range) || !rtsw_texture_validate_range(dst, dst_range) || rtsw_texture_range_byte_size(src, src_range) != rtsw_texture_range_byte_size(dst, dst_range)) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdTextureCopy requires matching valid ranges");
		return;
	}
	command = rtsw_command_buffer_append(command_buffer, RTSW_COMMAND_TEXTURE_COPY, sizeof(*command));
	if (!command) return;
	command->src = src;
	command->dst = dst;
	command->src_range = src_range;
	command->dst_range = dst_range;
	rtsw_resource_retain(&src->base);
	rtsw_resource_retain(&dst->base);
}

void rtCmdTextureCopyToBuffer(rt_command_buffer handle, rt_texture src_handle, rt_texture_range src_range, rt_buffer dst_handle, rt_buffer_range dst_range) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	struct rtsw_texture* src = rtsw_texture_from_handle(src_handle);
	struct rtsw_buffer* dst = rtsw_buffer_from_handle(dst_handle);
	struct rtsw_ir_texture_copy_to_buffer* command;
	if (!command_buffer || !command_buffer->recording || !rtsw_texture_validate_range(src, src_range) || !rtsw_buffer_valid_range(dst, dst_range) || dst_range.size != rtsw_texture_range_byte_size(src, src_range)) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdTextureCopyToBuffer requires matching valid ranges");
		return;
	}
	command = rtsw_command_buffer_append(command_buffer, RTSW_COMMAND_TEXTURE_COPY_TO_BUFFER, sizeof(*command));
	if (!command) return;
	command->src = src;
	command->dst = dst;
	command->src_range = src_range;
	command->dst_range = dst_range;
	rtsw_resource_retain(&src->base);
	rtsw_resource_retain(&dst->base);
}

void rtCmdBeginRendering(rt_command_buffer handle, rt_framebuffer framebuffer_handle) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	struct rtsw_framebuffer* framebuffer = rtsw_framebuffer_from_handle(framebuffer_handle);
	struct rtsw_ir_begin_rendering* command;

	if (!command_buffer || !command_buffer->recording || command_buffer->rendering || command_buffer->rendering_continuation || !framebuffer) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdBeginRendering requires a recording command buffer and framebuffer");
		return;
	}

	command = rtsw_command_buffer_append(command_buffer, RTSW_COMMAND_BEGIN_RENDERING, sizeof(*command));
	if (!command) {
		return;
	}

	command->framebuffer = framebuffer;
	rtsw_resource_retain(&framebuffer->base);
	command_buffer->rendering = true;
}

void rtCmdEndRendering(rt_command_buffer handle) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);

	if (!command_buffer || !command_buffer->recording || (!command_buffer->rendering && !command_buffer->rendering_continuation)) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdEndRendering requires an active rendering scope");
		return;
	}

	if (rtsw_command_buffer_append(command_buffer, RTSW_COMMAND_END_RENDERING, 0)) {
		command_buffer->rendering = false;
	}
}

void rtCmdClearColor(rt_command_buffer handle, rt_location location, f32 r, f32 g, f32 b, f32 a) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	struct rtsw_ir_clear_color* command;

	(void)location;
	if (!command_buffer || !command_buffer->recording || (!command_buffer->rendering && !command_buffer->rendering_continuation)) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdClearColor requires an active rendering scope");
		return;
	}

	command = rtsw_command_buffer_append(command_buffer, RTSW_COMMAND_CLEAR_COLOR, sizeof(*command));
	if (!command) {
		return;
	}

	command->r = r;
	command->g = g;
	command->b = b;
	command->a = a;
}

void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth) {
	rtsw_clear_error();
	struct rtsw_command_buffer* buffer = rtsw_command_buffer_from_handle(command_buffer);
	struct rtsw_ir_clear_depth* command;
	if (!buffer || !buffer->recording || (!buffer->rendering && !buffer->rendering_continuation)) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdClearDepth requires active rendering");
		return;
	}
	command = rtsw_command_buffer_append(buffer, RTSW_COMMAND_CLEAR_DEPTH, sizeof(*command));
	if (command) command->depth = depth;
}

void rtCmdClearStencil(rt_command_buffer command_buffer, usize stencil) {
	rtsw_clear_error();
	struct rtsw_command_buffer* buffer = rtsw_command_buffer_from_handle(command_buffer);
	struct rtsw_ir_clear_stencil* command;
	if (!buffer || !buffer->recording || (!buffer->rendering && !buffer->rendering_continuation)) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdClearStencil requires active rendering");
		return;
	}
	command = rtsw_command_buffer_append(buffer, RTSW_COMMAND_CLEAR_STENCIL, sizeof(*command));
	if (command) command->stencil = stencil;
}

void rtCmdClear(rt_command_buffer command_buffer, enum rt_clear_flag attachments) {
	rtsw_clear_error();
	struct rtsw_command_buffer* buffer = rtsw_command_buffer_from_handle(command_buffer);
	struct rtsw_ir_clear* command;

	if (!buffer || !buffer->recording || (!buffer->rendering && !buffer->rendering_continuation) || attachments == RT_CLEAR_NONE || (attachments & ~(RT_CLEAR_COLOR | RT_CLEAR_DEPTH | RT_CLEAR_STENCIL))) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdClear requires selected attachments inside rendering");
		return;
	}
	command = rtsw_command_buffer_append(buffer, RTSW_COMMAND_CLEAR, sizeof(*command));
	if (command) {
		command->attachments = attachments;
	}
}

void rtCmdSetViewport(rt_command_buffer handle, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	struct rtsw_ir_rectangle* command;

	if (!command_buffer || !command_buffer->recording) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdSetViewport requires a recording command buffer");
		return;
	}

	command = rtsw_command_buffer_append(command_buffer, RTSW_COMMAND_SET_VIEWPORT, sizeof(*command));
	if (!command) {
		return;
	}

	command->x = x;
	command->y = y;
	command->width = width;
	command->height = height;
	command->min_depth = min_depth;
	command->max_depth = max_depth;
}

void rtCmdSetScissor(rt_command_buffer handle, usize x, usize y, usize width, usize height) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	struct rtsw_ir_rectangle* command;

	if (!command_buffer || !command_buffer->recording) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdSetScissor requires a recording command buffer");
		return;
	}

	command = rtsw_command_buffer_append(command_buffer, RTSW_COMMAND_SET_SCISSOR, sizeof(*command));
	if (!command) {
		return;
	}

	command->x = x;
	command->y = y;
	command->width = width;
	command->height = height;
	command->min_depth = 0.0f;
	command->max_depth = 1.0f;
}

void rtCmdUseProgram(rt_command_buffer handle, rt_program program_handle) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	struct rtsw_program* program = rtsw_program_from_handle(program_handle);
	struct rtsw_ir_use_program* command;

	if (!command_buffer || !command_buffer->recording || (!command_buffer->rendering && !command_buffer->rendering_continuation) || !program || !program->finalized) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdUseProgram requires an active rendering scope and finalized program");
		return;
	}
	command = rtsw_command_buffer_append(command_buffer, RTSW_COMMAND_USE_PROGRAM, sizeof(*command));
	if (!command) {
		return;
	}
	command->program = program;
	rtsw_resource_retain(&program->base);
}

void rtCmdUniformData(rt_command_buffer command_buffer, rt_location location, const u08* data, usize size) {
	rtsw_clear_error();
	(void)command_buffer;
	(void)location;
	(void)data;
	(void)size;
	rtsw_throwf(RT_FEATURE_NOT_SUPPORTED, "rt-software uniform data is not implemented yet");
}

void rtCmdStorageData(rt_command_buffer command_buffer, rt_location location, const u08* data, usize size) {
	rtsw_clear_error();
	(void)command_buffer;
	(void)location;
	(void)data;
	(void)size;
	rtsw_throwf(RT_FEATURE_NOT_SUPPORTED, "rt-software storage data is not implemented yet");
}

void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range) {
	rtsw_clear_error();
	struct rtsw_command_buffer* commands = rtsw_command_buffer_from_handle(command_buffer);
	struct rt_location_t* uniform = location;
	struct rtsw_buffer* resource = rtsw_buffer_from_handle(buffer);
	struct rtsw_ir_bind_buffer* command;
	if (!commands || !commands->recording || !uniform || !uniform->program || !rtsw_buffer_valid_range(resource, range)) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdBindBuffer requires a recording command buffer, uniform location, and valid buffer range");
		return;
	}
	command = rtsw_command_buffer_append(commands, RTSW_COMMAND_BIND_BUFFER, sizeof(*command));
	if (!command) return;
	command->program = uniform->program;
	command->buffer = resource;
	command->range = range;
	command->symbol = uniform->symbol;
	rtsw_resource_retain(&uniform->program->base);
	rtsw_resource_retain(&resource->base);
}

void rtCmdVertexBuffer(rt_command_buffer handle, rt_location location, rt_buffer buffer_handle, rt_buffer_range range) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	struct rt_location_t* input = location;
	struct rtsw_buffer* buffer = rtsw_buffer_from_handle(buffer_handle);
	struct rtsw_ir_vertex_buffer* command;

	if (!command_buffer || !command_buffer->recording || (!command_buffer->rendering && !command_buffer->rendering_continuation) || !input ||
		!rtsw_buffer_valid_range(buffer, range)) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdVertexBuffer requires an active rendering scope, input location, and valid buffer range");
		return;
	}
	command = rtsw_command_buffer_append(command_buffer, RTSW_COMMAND_VERTEX_BUFFER, sizeof(*command));
	if (!command) {
		return;
	}
	command->buffer = buffer;
	command->range = range;
	command->input = input->address;
	rtsw_resource_retain(&buffer->base);
}

void rtCmdDraw(rt_command_buffer handle, usize vertex_count, usize first_vertex) {
	rtsw_clear_error();
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(handle);
	struct rtsw_ir_draw* command;

	if (!command_buffer || !command_buffer->recording || (!command_buffer->rendering && !command_buffer->rendering_continuation) || vertex_count == 0) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdDraw requires an active rendering scope and nonzero vertex count");
		return;
	}
	command = rtsw_command_buffer_append(command_buffer, RTSW_COMMAND_DRAW, sizeof(*command));
	if (!command) {
		return;
	}
	command->vertex_count = vertex_count;
	command->first_vertex = first_vertex;
}

void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, enum rt_index_format format) {
	rtsw_clear_error();
	struct rtsw_command_buffer* commands = rtsw_command_buffer_from_handle(command_buffer);
	struct rtsw_buffer* index_buffer = rtsw_buffer_from_handle(buffer);
	struct rtsw_ir_index_buffer* command;
	if (!commands || !commands->recording || (!commands->rendering && !commands->rendering_continuation) ||
		!rtsw_buffer_valid_range(index_buffer, range) || (format != RT_INDEX_U16 && format != RT_INDEX_U32)) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdIndexBuffer requires active rendering, a valid range, and a supported index format");
		return;
	}
	command = rtsw_command_buffer_append(commands, RTSW_COMMAND_INDEX_BUFFER, sizeof(*command));
	if (!command) return;
	command->buffer = index_buffer;
	command->range = range;
	command->format = format;
	rtsw_resource_retain(&index_buffer->base);
}

void rtCmdDrawInstanced(rt_command_buffer command_buffer, usize vertex_count, usize instance_count, usize first_vertex, usize first_instance) {
	rtsw_clear_error();
	struct rtsw_command_buffer* commands = rtsw_command_buffer_from_handle(command_buffer);
	struct rtsw_ir_draw_instanced* command;
	if (!commands || !commands->recording || (!commands->rendering && !commands->rendering_continuation) || !vertex_count || vertex_count % 3 || !instance_count) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdDrawInstanced requires active rendering, non-zero instances, and vertices divisible by three");
		return;
	}
	command = rtsw_command_buffer_append(commands, RTSW_COMMAND_DRAW_INSTANCED, sizeof(*command));
	if (!command) return;
	*command = (struct rtsw_ir_draw_instanced){ vertex_count, instance_count, first_vertex, first_instance };
}

void rtCmdDrawIndexed(rt_command_buffer command_buffer, usize index_count, usize first_index, usize vertex_offset) {
	rtsw_clear_error();
	struct rtsw_command_buffer* commands = rtsw_command_buffer_from_handle(command_buffer);
	struct rtsw_ir_draw_indexed* command;
	if (!commands || !commands->recording || (!commands->rendering && !commands->rendering_continuation) || !index_count || index_count % 3) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdDrawIndexed requires active rendering and an index count divisible by three");
		return;
	}
	command = rtsw_command_buffer_append(commands, RTSW_COMMAND_DRAW_INDEXED, sizeof(*command));
	if (!command) return;
	command->index_count = index_count;
	command->first_index = first_index;
	command->vertex_offset = vertex_offset;
}

void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, usize index_count, usize instance_count, usize first_index, usize vertex_offset, usize first_instance) {
	rtsw_clear_error();
	struct rtsw_command_buffer* commands = rtsw_command_buffer_from_handle(command_buffer);
	struct rtsw_ir_draw_indexed_instanced* command;
	if (!commands || !commands->recording || (!commands->rendering && !commands->rendering_continuation) || !index_count || index_count % 3 || !instance_count) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdDrawIndexedInstanced requires active rendering, non-zero instances, and indices divisible by three");
		return;
	}
	command = rtsw_command_buffer_append(commands, RTSW_COMMAND_DRAW_INDEXED_INSTANCED, sizeof(*command));
	if (!command) return;
	*command = (struct rtsw_ir_draw_indexed_instanced){ index_count, instance_count, first_index, vertex_offset, first_instance };
}

void rtCmdBufferBarrier(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, rt_access src, rt_access dst) {
	rtsw_clear_error();
	struct rtsw_command_buffer* commands = rtsw_command_buffer_from_handle(command_buffer);
	if (!commands || !commands->recording || !rtsw_buffer_valid_range(rtsw_buffer_from_handle(buffer), range) ||
		src.stage == RT_STAGE_NONE || dst.stage == RT_STAGE_NONE) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdBufferBarrier requires a recording command buffer, valid range, and stages");
	}
}

void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view) {
	rtsw_clear_error();
	(void)command_buffer;
	(void)location;
	(void)texture_view;
	rtsw_throwf(RT_FEATURE_NOT_SUPPORTED, "rt-software texture bindings are not implemented yet");
}

void rtCmdBindSampler(rt_command_buffer command_buffer, rt_location location, rt_sampler sampler) {
	rtsw_clear_error();
	(void)command_buffer;
	(void)location;
	(void)sampler;
	rtsw_throwf(RT_FEATURE_NOT_SUPPORTED, "rt-software sampler bindings are not implemented yet");
}

void rtCmdTextureBarrier(rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, rt_access src, rt_access dst) {
	rtsw_clear_error();
	struct rtsw_command_buffer* commands = rtsw_command_buffer_from_handle(command_buffer);
	if (!commands || !commands->recording || !rtsw_texture_validate_range(rtsw_texture_from_handle(texture), range) ||
		src.stage == RT_STAGE_NONE || dst.stage == RT_STAGE_NONE) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdTextureBarrier requires a recording command buffer, valid range, and stages");
	}
}
