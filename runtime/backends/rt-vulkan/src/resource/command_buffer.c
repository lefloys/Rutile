#include "command_buffer.h"
#include "context.h"
#include "error.h"
#include "queue.h"

#include <assert.h>
#include <intrin.h>
#include <stdlib.h>
#include <string.h>

/*===============================================================================================*/
/*                                                                                                */
/*===============================================================================================*/

rt_command_buffer rtCommandBufferCreate(void) {
	return rtvk_command_buffer_to_handle(rtvk_command_buffer_create(rtvk_get_current_context()));
}

void rtCommandBufferDestroy(rt_command_buffer command_buffer) {
	rtvk_command_buffer_destroy(rtvk_get_current_context(), rtvk_command_buffer_from_handle(command_buffer));
}

void rtCmdReset(rt_command_buffer command_buffer) {
	rtvk_command_buffer_reset(rtvk_command_buffer_from_handle(command_buffer));
}

void rtCmdBegin(rt_command_buffer command_buffer) {
	rtvk_command_buffer_begin(rtvk_command_buffer_from_handle(command_buffer));
}

void rtCmdWait(rt_command_buffer command_buffer, rt_timepoint timepoint) {
	rtvk_command_buffer_wait(
		rtvk_command_buffer_from_handle(command_buffer),
		timepoint
	);
}

void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer) {
	rtvk_command_buffer_begin_rendering(
		rtvk_command_buffer_from_handle(command_buffer),
		rtvk_framebuffer_from_handle(framebuffer)
	);
}

void rtCmdClearColor(rt_command_buffer command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a) {
	rtvk_command_buffer_clear_color(rtvk_command_buffer_from_handle(command_buffer), color_index, r, g, b, a);
}

void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth) {
	rtvk_command_buffer_clear_depth(rtvk_command_buffer_from_handle(command_buffer), depth);
}

void rtCmdClearStencil(rt_command_buffer command_buffer, u32 stencil) {
	rtvk_command_buffer_clear_stencil(rtvk_command_buffer_from_handle(command_buffer), stencil);
}

void rtCmdSetViewport(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth) {
	rtvk_command_buffer_set_viewport(rtvk_command_buffer_from_handle(command_buffer), x, y, width, height, min_depth, max_depth);
}

void rtCmdSetScissor(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height) {
	rtvk_command_buffer_set_scissor(rtvk_command_buffer_from_handle(command_buffer), x, y, width, height);
}

void rtCmdEndRendering(rt_command_buffer command_buffer) {
	rtvk_command_buffer_end_rendering(rtvk_command_buffer_from_handle(command_buffer));
}

void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program) {
	rtvk_command_buffer_use_graphics_program(
		rtvk_command_buffer_from_handle(command_buffer),
		rtvk_graphics_program_from_handle(program)
	);
}

void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset, usize size) {
	rtvk_command_buffer_bind_buffer(
		rtvk_command_buffer_from_handle(command_buffer),
		location,
		rtvk_buffer_from_handle(buffer),
		offset,
		size
	);
}

void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view) {
	rtvk_command_buffer_bind_texture(
		rtvk_command_buffer_from_handle(command_buffer),
		location,
		rtvk_texture_view_from_handle(texture_view)
	);
}

void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset) {
	rtvk_command_buffer_vertex_buffer(
		rtvk_command_buffer_from_handle(command_buffer),
		location,
		rtvk_buffer_from_handle(buffer),
		offset
	);
}

void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, usize offset, enum rt_index_format format) {
	rtvk_command_buffer_index_buffer(rtvk_command_buffer_from_handle(command_buffer), rtvk_buffer_from_handle(buffer), offset, format);
}

void rtCmdDraw(rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex) {
	rtvk_command_buffer_draw(rtvk_command_buffer_from_handle(command_buffer), vertex_count, first_vertex);
}

void rtCmdDrawInstanced(rt_command_buffer command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
	rtvk_command_buffer_draw_instanced(
		rtvk_command_buffer_from_handle(command_buffer),
		vertex_count,
		instance_count,
		first_vertex,
		first_instance
	);
}

void rtCmdDrawIndexed(rt_command_buffer command_buffer, u32 index_count, u32 first_index, i32 vertex_offset) {
	rtvk_command_buffer_draw_indexed(rtvk_command_buffer_from_handle(command_buffer), index_count, first_index, vertex_offset);
}

void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) {
	rtvk_command_buffer_draw_indexed_instanced(
		rtvk_command_buffer_from_handle(command_buffer),
		index_count,
		instance_count,
		first_index,
		vertex_offset,
		first_instance
	);
}

void rtCmdEnd(rt_command_buffer command_buffer) {
	rtvk_command_buffer_end(rtvk_command_buffer_from_handle(command_buffer));
}

/*===============================================================================================*/
/*                                                                                                */
/*===============================================================================================*/

usize rtvk_command_payload_size(rtvk_command_opcode opcode) {
	switch (opcode) {
	case RTVK_COMMAND_WAIT:
		return sizeof(struct rtvk_ir_wait);
	case RTVK_COMMAND_BEGIN_RENDERING:
		return sizeof(struct rtvk_ir_framebuffer);
	case RTVK_COMMAND_CLEAR_COLOR:
		return sizeof(struct rtvk_ir_clear_color);
	case RTVK_COMMAND_CLEAR_DEPTH:
		return sizeof(struct rtvk_ir_clear_depth);
	case RTVK_COMMAND_CLEAR_STENCIL:
		return sizeof(struct rtvk_ir_clear_stencil);
	case RTVK_COMMAND_SET_VIEWPORT:
		return sizeof(struct rtvk_ir_viewport);
	case RTVK_COMMAND_SET_SCISSOR:
		return sizeof(struct rtvk_ir_scissor);
	case RTVK_COMMAND_END_RENDERING:
		return 0;
	case RTVK_COMMAND_USE_GRAPHICS_PROGRAM:
		return sizeof(struct rtvk_ir_program);
	case RTVK_COMMAND_BIND_BUFFER:
		return sizeof(struct rtvk_ir_buffer);
	case RTVK_COMMAND_BIND_TEXTURE:
		return sizeof(struct rtvk_ir_texture);
	case RTVK_COMMAND_VERTEX_BUFFER:
		return sizeof(struct rtvk_ir_vertex_buffer);
	case RTVK_COMMAND_INDEX_BUFFER:
		return sizeof(struct rtvk_ir_index_buffer);
	case RTVK_COMMAND_DRAW:
		return sizeof(struct rtvk_ir_draw);
	case RTVK_COMMAND_DRAW_INSTANCED:
		return sizeof(struct rtvk_ir_draw_instanced);
	case RTVK_COMMAND_DRAW_INDEXED:
		return sizeof(struct rtvk_ir_draw_indexed);
	case RTVK_COMMAND_DRAW_INDEXED_INSTANCED:
		return sizeof(struct rtvk_ir_draw_indexed_instanced);
	default:
		return 0;
	}
}

usize rtvk_command_record_size(rtvk_command_opcode opcode) {
	const usize alignment = _Alignof(void*);
	const usize size = sizeof(struct rtvk_command_header) + rtvk_command_payload_size(opcode);
	return (size + alignment - 1) & ~(alignment - 1);
}

void* rtvk_command_append(struct rtvk_command_buffer* command_buffer, rtvk_command_opcode opcode) {
	const usize size = rtvk_command_record_size(opcode);
	if (command_buffer->ir_capacity - command_buffer->ir_size < size) {
		usize required_size = command_buffer->ir_size + size;
		unsigned long most_significant_bit = 0;
		_BitScanReverse64(&most_significant_bit, required_size - 1);
		usize capacity = (usize)1 << (most_significant_bit + 1);
		if (capacity < 4096) {
			capacity = 4096;
		}
		void* data = realloc(command_buffer->ir_data, capacity);
		if (!data) {
			rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for command IR", capacity);
			return NULL;
		}
		command_buffer->ir_data = data;
		command_buffer->ir_capacity = capacity;
	}

	struct rtvk_command_header* header = (struct rtvk_command_header*)(command_buffer->ir_data + command_buffer->ir_size);
	header->opcode = (u08)opcode;
	command_buffer->ir_size += size;
	return header + 1;
}

void rtvk_command_buffer_release_resources(struct rtvk_command_buffer* command_buffer) {
	for (usize offset = 0; offset < command_buffer->ir_size; (void)0) {
		struct rtvk_command_header* header = (struct rtvk_command_header*)(command_buffer->ir_data + offset);
		void* payload = header + 1;
		switch ((rtvk_command_opcode)header->opcode) {
		case RTVK_COMMAND_BEGIN_RENDERING:
			rtvk_release_resource(((struct rtvk_ir_framebuffer*)payload)->framebuffer);
			break;
		case RTVK_COMMAND_USE_GRAPHICS_PROGRAM:
			rtvk_release_resource(((struct rtvk_ir_program*)payload)->program);
			break;
		case RTVK_COMMAND_BIND_BUFFER:
			rtvk_release_resource(((struct rtvk_ir_buffer*)payload)->buffer);
			break;
		case RTVK_COMMAND_BIND_TEXTURE:
			rtvk_release_resource(((struct rtvk_ir_texture*)payload)->view);
			break;
		case RTVK_COMMAND_VERTEX_BUFFER:
			rtvk_release_resource(((struct rtvk_ir_vertex_buffer*)payload)->buffer);
			break;
		case RTVK_COMMAND_INDEX_BUFFER:
			rtvk_release_resource(((struct rtvk_ir_index_buffer*)payload)->buffer);
			break;
		default:
			break;
		}
		offset += rtvk_command_record_size((rtvk_command_opcode)header->opcode);
	}

	free(command_buffer->ir_data);
	command_buffer->ir_data = NULL;
	command_buffer->ir_size = 0;
	command_buffer->ir_capacity = 0;
}

void rtvk_command_buffer_reset(struct rtvk_command_buffer* command_buffer) {
	rtvk_command_buffer_release_resources(command_buffer);
	command_buffer->recording = false;
	command_buffer->executable = false;
}

void rtvk_command_buffer_begin(struct rtvk_command_buffer* command_buffer) {
	command_buffer->recording = true;
}

void rtvk_command_buffer_wait(struct rtvk_command_buffer* command_buffer, rt_timepoint timepoint) {
	struct rtvk_ir_wait* command = rtvk_command_append(command_buffer, RTVK_COMMAND_WAIT);
	if (!command) { return; }
	command->timepoint = timepoint;
}

void rtvk_command_buffer_begin_rendering(struct rtvk_command_buffer* command_buffer, struct rtvk_framebuffer* framebuffer) {
	struct rtvk_ir_framebuffer* command = rtvk_command_append(command_buffer, RTVK_COMMAND_BEGIN_RENDERING);
	if (!command) { return; }
	command->framebuffer = framebuffer;
	rtvk_retain_resource(command->framebuffer);
}

void rtvk_command_buffer_clear_color(struct rtvk_command_buffer* command_buffer, u32 index, f32 r, f32 g, f32 b, f32 a) {
	struct rtvk_ir_clear_color* command = rtvk_command_append(command_buffer, RTVK_COMMAND_CLEAR_COLOR);
	if (!command) { return; }
	*command = (struct rtvk_ir_clear_color){ index, r, g, b, a };
}

void rtvk_command_buffer_clear_depth(struct rtvk_command_buffer* command_buffer, f32 depth) {
	struct rtvk_ir_clear_depth* command = rtvk_command_append(command_buffer, RTVK_COMMAND_CLEAR_DEPTH);
	if (!command) { return; }
	command->depth = depth;
}

void rtvk_command_buffer_clear_stencil(struct rtvk_command_buffer* command_buffer, u32 stencil) {
	struct rtvk_ir_clear_stencil* command = rtvk_command_append(command_buffer, RTVK_COMMAND_CLEAR_STENCIL);
	if (!command) { return; }
	command->stencil = stencil;
}

void rtvk_command_buffer_set_viewport(struct rtvk_command_buffer* command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth) {
	struct rtvk_ir_viewport* command = rtvk_command_append(command_buffer, RTVK_COMMAND_SET_VIEWPORT);
	if (!command) { return; }
	*command = (struct rtvk_ir_viewport){ x, y, width, height, min_depth, max_depth };
}

void rtvk_command_buffer_set_scissor(struct rtvk_command_buffer* command_buffer, u32 x, u32 y, u32 width, u32 height) {
	struct rtvk_ir_scissor* command = rtvk_command_append(command_buffer, RTVK_COMMAND_SET_SCISSOR);
	if (!command) { return; }
	*command = (struct rtvk_ir_scissor){ x, y, width, height };
}

void rtvk_command_buffer_end_rendering(struct rtvk_command_buffer* command_buffer) {
	struct rtvk_command_header* command = rtvk_command_append(command_buffer, RTVK_COMMAND_END_RENDERING);
	if (!command) { return; }
}

void rtvk_command_buffer_use_graphics_program(struct rtvk_command_buffer* command_buffer, struct rtvk_graphics_program* program) {
	struct rtvk_ir_program* command = rtvk_command_append(command_buffer, RTVK_COMMAND_USE_GRAPHICS_PROGRAM);
	if (!command) { return; }
	command->program = program;
	rtvk_retain_resource(command->program);
}

void rtvk_command_buffer_bind_buffer(struct rtvk_command_buffer* command_buffer, rt_location location, struct rtvk_buffer* buffer, usize offset, usize size) {
	struct rtvk_ir_buffer* command = rtvk_command_append(command_buffer, RTVK_COMMAND_BIND_BUFFER);
	if (!command) { return; }
	*command = (struct rtvk_ir_buffer){ location, buffer, offset, size };
	rtvk_retain_resource(command->buffer);
}

void rtvk_command_buffer_bind_texture(struct rtvk_command_buffer* command_buffer, rt_location location, struct rtvk_texture_view* view) {
	struct rtvk_ir_texture* command = rtvk_command_append(command_buffer, RTVK_COMMAND_BIND_TEXTURE);
	if (!command) { return; }
	*command = (struct rtvk_ir_texture){ location, view };
	rtvk_retain_resource(command->view);
}

void rtvk_command_buffer_vertex_buffer(struct rtvk_command_buffer* command_buffer, rt_location location, struct rtvk_buffer* buffer, usize offset) {
	struct rtvk_ir_vertex_buffer* command = rtvk_command_append(command_buffer, RTVK_COMMAND_VERTEX_BUFFER);
	if (!command) { return; }
	*command = (struct rtvk_ir_vertex_buffer){ location, buffer, offset };
	rtvk_retain_resource(command->buffer);
}

void rtvk_command_buffer_index_buffer(struct rtvk_command_buffer* command_buffer, struct rtvk_buffer* buffer, usize offset, enum rt_index_format format) {
	struct rtvk_ir_index_buffer* command = rtvk_command_append(command_buffer, RTVK_COMMAND_INDEX_BUFFER);
	if (!command) { return; }
	*command = (struct rtvk_ir_index_buffer){ buffer, offset, format };
	rtvk_retain_resource(command->buffer);
}

void rtvk_command_buffer_draw(struct rtvk_command_buffer* command_buffer, u32 count, u32 first) {
	struct rtvk_ir_draw* command = rtvk_command_append(command_buffer, RTVK_COMMAND_DRAW);
	if (!command) { return; }
	*command = (struct rtvk_ir_draw){ count, first };
}

void rtvk_command_buffer_draw_instanced(struct rtvk_command_buffer* command_buffer, u32 count, u32 instances, u32 first, u32 first_instance) {
	struct rtvk_ir_draw_instanced* command = rtvk_command_append(command_buffer, RTVK_COMMAND_DRAW_INSTANCED);
	if (!command) { return; }
	*command = (struct rtvk_ir_draw_instanced){ count, instances, first, first_instance };
}

void rtvk_command_buffer_draw_indexed(struct rtvk_command_buffer* command_buffer, u32 count, u32 first, i32 vertex_offset) {
	struct rtvk_ir_draw_indexed* command = rtvk_command_append(command_buffer, RTVK_COMMAND_DRAW_INDEXED);
	if (!command) { return; }
	*command = (struct rtvk_ir_draw_indexed){ count, first, vertex_offset };
}

void rtvk_command_buffer_draw_indexed_instanced(struct rtvk_command_buffer* command_buffer, u32 count, u32 instances, u32 first, i32 vertex_offset, u32 first_instance) {
	struct rtvk_ir_draw_indexed_instanced* command = rtvk_command_append(command_buffer, RTVK_COMMAND_DRAW_INDEXED_INSTANCED);
	if (!command) { return; }
	*command = (struct rtvk_ir_draw_indexed_instanced){ count, instances, first, vertex_offset, first_instance };
}

void rtvk_command_buffer_end(struct rtvk_command_buffer* command_buffer) {
	command_buffer->recording = false;
	command_buffer->executable = rtvk_error() == RT_SUCCESS;
}

/*===============================================================================================*/
/*                                                                                                */
/*===============================================================================================*/

RTVK_DEFINE_RESOURCE_PRIVATE(command_buffer)

void rtvk_command_buffer_init(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer) {
	rtvk_init_resource_base(ctx, RTVK_RESOURCE_BASE(command_buffer), RT_RESOURCE_COMMAND_BUFFER);
}

void rtvk_command_buffer_finish(struct rtvk_command_buffer* command_buffer) {
	rtvk_command_buffer_release_resources(command_buffer);
	rtvk_finish_resource_base(RTVK_RESOURCE_BASE(command_buffer));
}

/*===============================================================================================*/
/*                                                                                                */
/*===============================================================================================*/

struct rtvk_lower_state {
	struct rtvk_buffer** bound_buffers;
	struct rtvk_texture_view** bound_textures;
	struct rtvk_buffer** vertex_buffers;
	VkDeviceSize* vertex_offsets;
	struct rtvk_framebuffer* framebuffer;
	struct rtvk_graphics_program* program;
	struct rtvk_buffer* index_buffer;
	usize* bound_offsets;
	usize* bound_sizes;
	usize binding_capacity;
	usize vertex_buffer_capacity;
	usize index_offset;
	u32 render_width;
	u32 render_height;
	VkViewport viewport;
	VkRect2D scissor;
	enum rt_index_format index_format;
	bool rendering;
	bool descriptors_dirty;
	bool scissor_set;
	bool viewport_set;
};

void rtvk_lower_reserve_bindings(struct rtvk_lower_state* state, usize count) {
	if (state->binding_capacity >= count) {
		return;
	}

	struct rtvk_buffer** buffers = calloc(count, sizeof(*buffers));
	struct rtvk_texture_view** textures = calloc(count, sizeof(*textures));
	usize* offsets = calloc(count, sizeof(*offsets));
	usize* sizes = calloc(count, sizeof(*sizes));
	if (!buffers || !textures || !offsets || !sizes) {
		free(buffers);
		free(textures);
		free(offsets);
		free(sizes);
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for descriptor binding state", count * (sizeof(*buffers) + sizeof(*textures) + sizeof(*offsets) + sizeof(*sizes)));
		return;
	}

	memcpy(buffers, state->bound_buffers, sizeof(*buffers) * state->binding_capacity);
	memcpy(textures, state->bound_textures, sizeof(*textures) * state->binding_capacity);
	memcpy(offsets, state->bound_offsets, sizeof(*offsets) * state->binding_capacity);
	memcpy(sizes, state->bound_sizes, sizeof(*sizes) * state->binding_capacity);
	free(state->bound_buffers);
	free(state->bound_textures);
	free(state->bound_offsets);
	free(state->bound_sizes);
	state->bound_buffers = buffers;
	state->bound_textures = textures;
	state->bound_offsets = offsets;
	state->bound_sizes = sizes;
	state->binding_capacity = count;
}

void rtvk_lower_reserve_vertex_buffers(struct rtvk_lower_state* state, usize count) {
	if (state->vertex_buffer_capacity >= count) {
		return;
	}

	struct rtvk_buffer** buffers = calloc(count, sizeof(*buffers));
	VkDeviceSize* offsets = calloc(count, sizeof(*offsets));
	if (!buffers || !offsets) {
		free(buffers);
		free(offsets);
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for vertex binding state", count * (sizeof(*buffers) + sizeof(*offsets)));
		return;
	}

	memcpy(buffers, state->vertex_buffers, sizeof(*buffers) * state->vertex_buffer_capacity);
	memcpy(offsets, state->vertex_offsets, sizeof(*offsets) * state->vertex_buffer_capacity);
	free(state->vertex_buffers);
	free(state->vertex_offsets);
	state->vertex_buffers = buffers;
	state->vertex_offsets = offsets;
	state->vertex_buffer_capacity = count;
}

void rtvk_lower_state_finish(struct rtvk_lower_state* state) {
	free(state->bound_buffers);
	free(state->bound_textures);
	free(state->bound_offsets);
	free(state->bound_sizes);
	free(state->vertex_buffers);
	free(state->vertex_offsets);
}

void rtvk_lowered_command_buffer_add_resource_job(struct rtvk_lowered_command_buffer* lowered, struct rtvk_resource_base* resource) {
	if (!resource) {
		return;
	}

	for (struct rtvk_lowered_resource_job* job = lowered->resource_jobs; job; job = job->next) {
		if (job->base.resource == resource) {
			return;
		}
	}

	struct rtvk_lowered_resource_job* job = calloc(1, sizeof(*job));
	if (!job) {
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for Vulkan resource job", sizeof(*job));
		return;
	}

	job->base.resource = resource;
	rtvk_resource_job_begin(resource);
	job->next = lowered->resource_jobs;
	lowered->resource_jobs = job;
}

void rtvk_lowered_command_buffer_release_resource_jobs(struct rtvk_lowered_command_buffer* lowered) {
	while (lowered->resource_jobs) {
		struct rtvk_lowered_resource_job* job = lowered->resource_jobs;
		lowered->resource_jobs = job->next;
		rtvk_resource_job_end(job->base.resource);
		free(job);
	}
}

void rtvk_lowered_command_buffer_end_segment(struct rtvk_lowered_command_segment* segment) {
	VkResult result = vkEndCommandBuffer(segment->vk_command_buffer);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return;
	}
}

struct rtvk_lowered_command_segment* rtvk_lowered_command_buffer_current_segment(struct rtvk_lowered_command_buffer* lowered) {
	return &lowered->segments[lowered->segment_index];
}

void rtvk_lower_begin_rendering(VkCommandBuffer command_buffer, struct rtvk_framebuffer* framebuffer) {
	VkRenderingAttachmentInfo colors[RTVK_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS] = { 0 };
	VkExtent3D extent = { 0 };
	for (usize index = 0; index < framebuffer->color_texture_count; index++) {
		struct rtvk_texture_view* view = framebuffer->color_views[index];
		rtvk_image_transition_layout(command_buffer, view->image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		colors[index].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colors[index].imageView = view->vk_image_view;
		colors[index].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colors[index].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colors[index].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		if (index == 0) {
			extent.width = view->image->width;
			extent.height = view->image->height;
			extent.depth = view->image->depth;
		}
	}

	VkRenderingAttachmentInfo depth = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	if (framebuffer->depth_view) {
		rtvk_image_transition_layout(command_buffer, framebuffer->depth_view->image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		depth.imageView = framebuffer->depth_view->vk_image_view;
		depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	}

	VkRenderingAttachmentInfo stencil = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	if (framebuffer->stencil_view) {
		rtvk_image_transition_layout(command_buffer, framebuffer->stencil_view->image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		stencil.imageView = framebuffer->stencil_view->vk_image_view;
		stencil.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		stencil.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		stencil.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	}

	VkRenderingInfo info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
	info.renderArea.extent.width = extent.width;
	info.renderArea.extent.height = extent.height;
	info.layerCount = 1;
	info.colorAttachmentCount = framebuffer->color_texture_count;
	info.pColorAttachments = colors;
	info.pDepthAttachment = framebuffer->depth_view ? &depth : NULL;
	info.pStencilAttachment = framebuffer->stencil_view ? &stencil : NULL;
	vkCmdBeginRendering(command_buffer, &info);
}

void rtvk_lower_bind_descriptors(struct rtvk_context* ctx, struct rtvk_lowered_command_segment* segment, struct rtvk_lower_state* state) {
	if (!state->descriptors_dirty) {
		return;
	}
	if (!state->program || !state->program->vk_descriptor_set_layout) {
		state->descriptors_dirty = false;
		return;
	}
	rtvk_lower_reserve_bindings(state, state->program->location_count);
	if (rtvk_error() != RT_SUCCESS) {
		return;
	}

	VkDescriptorSetLayout layout = state->program->vk_descriptor_set_layout;
	VkDescriptorSetAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocate_info.descriptorPool = segment->vk_descriptor_pool;
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts = &layout;
	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	VkResult result = vkAllocateDescriptorSets(ctx->vk_device, &allocate_info, &descriptor_set);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return;
	}

	u32 descriptor_count = 0;
	for (u32 location_index = 0; location_index < state->program->location_count; location_index++) {
		if (state->program->locations[location_index].kind != RTVK_LOCATION_VERTEX) {
			descriptor_count++;
		}
	}

	VkDescriptorBufferInfo* buffer_infos = calloc(descriptor_count, sizeof(*buffer_infos));
	VkDescriptorImageInfo* image_infos = calloc(descriptor_count, sizeof(*image_infos));
	VkWriteDescriptorSet* writes = calloc(descriptor_count, sizeof(*writes));
	if (!buffer_infos || !image_infos || !writes) {
		free(buffer_infos);
		free(image_infos);
		free(writes);
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for descriptor writes", (usize)descriptor_count * (sizeof(*buffer_infos) + sizeof(*image_infos) + sizeof(*writes)));
		return;
	}

	u32 descriptor_index = 0;
	for (u32 location_index = 0; location_index < state->program->location_count; location_index++) {
		rt_location location = &state->program->locations[location_index];
		if (location->kind == RTVK_LOCATION_VERTEX) {
			continue;
		}
		writes[descriptor_index] = (VkWriteDescriptorSet){ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		writes[descriptor_index].dstSet = descriptor_set;
		writes[descriptor_index].dstBinding = location->binding;
		writes[descriptor_index].descriptorCount = 1;
		if (location->kind == RTVK_LOCATION_TEXTURE) {
			struct rtvk_texture_view* view = location->index < state->binding_capacity ? state->bound_textures[location->index] : NULL;
			image_infos[descriptor_index].sampler = view->vk_sampler;
			image_infos[descriptor_index].imageView = view->vk_image_view;
			image_infos[descriptor_index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			writes[descriptor_index].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[descriptor_index].pImageInfo = &image_infos[descriptor_index];
		} else {
			struct rtvk_buffer* buffer = location->index < state->binding_capacity ? state->bound_buffers[location->index] : NULL;
			struct rtvk_buffer* active = buffer->active;
			buffer_infos[descriptor_index].buffer = active->vk_buffer;
			buffer_infos[descriptor_index].offset = state->bound_offsets[location->index];
			buffer_infos[descriptor_index].range = state->bound_sizes[location->index];
			writes[descriptor_index].descriptorType = location->kind == RTVK_LOCATION_STORAGE_BUFFER ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writes[descriptor_index].pBufferInfo = &buffer_infos[descriptor_index];
		}
		descriptor_index++;
	}

	vkUpdateDescriptorSets(ctx->vk_device, descriptor_count, writes, 0, NULL);
	vkCmdBindDescriptorSets(segment->vk_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->program->vk_pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
	free(buffer_infos);
	free(image_infos);
	free(writes);
	state->descriptors_dirty = false;
}

void rtvk_lower_bind_program(struct rtvk_context* ctx, struct rtvk_lowered_command_segment* segment, struct rtvk_lower_state* state) {
	VkFormat formats[RTVK_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS];
	for (usize index = 0; index < state->framebuffer->color_texture_count; index++) {
		formats[index] = rtvk_view_format(state->framebuffer->color_views[index]);
	}

	VkPipeline pipeline = rtvk_graphics_program_prepare(
		ctx,
		state->program,
		formats,
		state->framebuffer->color_texture_count,
		rtvk_view_format(state->framebuffer->depth_view),
		VK_FORMAT_UNDEFINED,
		VK_SAMPLE_COUNT_1_BIT
	);
	if (pipeline) {
		vkCmdBindPipeline(segment->vk_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	}
}

void rtvk_lower_begin_rendering_command(struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_framebuffer* command) {
	struct rtvk_lowered_command_segment* segment = rtvk_lowered_command_buffer_current_segment(lowered);
	state->framebuffer = command->framebuffer;
	rtvk_lowered_command_buffer_add_resource_job(lowered, RTVK_RESOURCE_BASE(state->framebuffer));
	rtvk_lower_begin_rendering(segment->vk_command_buffer, state->framebuffer);
	state->render_width = state->framebuffer->color_views[0]->image->width;
	state->render_height = state->framebuffer->color_views[0]->image->height;
	state->rendering = true;
	segment->command_count++;
}

void rtvk_lower_clear_color(struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_clear_color* command) {
	struct rtvk_lowered_command_segment* segment = rtvk_lowered_command_buffer_current_segment(lowered);
	VkClearAttachment attachment = { VK_IMAGE_ASPECT_COLOR_BIT, command->index, { .color = {{ command->r, command->g, command->b, command->a }} } };
	VkClearRect rect = { { { 0, 0 }, { state->render_width, state->render_height } }, 0, 1 };
	vkCmdClearAttachments(segment->vk_command_buffer, 1, &attachment, 1, &rect);
	segment->command_count++;
}

void rtvk_lower_clear_depth(struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_clear_depth* command) {
	struct rtvk_lowered_command_segment* segment = rtvk_lowered_command_buffer_current_segment(lowered);
	VkClearAttachment attachment = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, { .depthStencil = { command->depth, 0 } } };
	VkClearRect rect = { { { 0, 0 }, { state->render_width, state->render_height } }, 0, 1 };
	vkCmdClearAttachments(segment->vk_command_buffer, 1, &attachment, 1, &rect);
	segment->command_count++;
}

void rtvk_lower_clear_stencil(struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_clear_stencil* command) {
	struct rtvk_lowered_command_segment* segment = rtvk_lowered_command_buffer_current_segment(lowered);
	VkClearAttachment attachment = { VK_IMAGE_ASPECT_STENCIL_BIT, 0, { .depthStencil = { 1.0f, command->stencil } } };
	VkClearRect rect = { { { 0, 0 }, { state->render_width, state->render_height } }, 0, 1 };
	vkCmdClearAttachments(segment->vk_command_buffer, 1, &attachment, 1, &rect);
	segment->command_count++;
}

void rtvk_lower_set_viewport(struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_viewport* command) {
	struct rtvk_lowered_command_segment* segment = rtvk_lowered_command_buffer_current_segment(lowered);
	state->viewport = (VkViewport){ (f32)command->x, (f32)(command->y + command->height), (f32)command->width, -(f32)command->height, command->min_depth, command->max_depth };
	state->viewport_set = true;
	vkCmdSetViewport(segment->vk_command_buffer, 0, 1, &state->viewport);
	segment->command_count++;
}

void rtvk_lower_set_scissor(struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_scissor* command) {
	struct rtvk_lowered_command_segment* segment = rtvk_lowered_command_buffer_current_segment(lowered);
	state->scissor = (VkRect2D){ { (i32)command->x, (i32)command->y }, { command->width, command->height } };
	state->scissor_set = true;
	vkCmdSetScissor(segment->vk_command_buffer, 0, 1, &state->scissor);
	segment->command_count++;
}

void rtvk_lower_end_rendering(struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state) {
	struct rtvk_lowered_command_segment* segment = rtvk_lowered_command_buffer_current_segment(lowered);
	vkCmdEndRendering(segment->vk_command_buffer);
	state->rendering = false;
	segment->command_count++;
}

void rtvk_lower_use_graphics_program(struct rtvk_context* ctx, struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_program* command) {
	state->program = command->program;
	state->descriptors_dirty = true;
	rtvk_lowered_command_buffer_add_resource_job(lowered, RTVK_RESOURCE_BASE(state->program));
	rtvk_lower_bind_program(ctx, rtvk_lowered_command_buffer_current_segment(lowered), state);
	rtvk_lowered_command_buffer_current_segment(lowered)->command_count++;
}

void rtvk_lower_bind_buffer(struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_buffer* command) {
	if (!command->location) {
		return;
	}
	rtvk_lower_reserve_bindings(state, command->location->index + 1);
	if (rtvk_error() != RT_SUCCESS) {
		return;
	}
	state->bound_buffers[command->location->index] = command->buffer;
	state->bound_textures[command->location->index] = NULL;
	state->bound_offsets[command->location->index] = command->offset;
	state->bound_sizes[command->location->index] = command->size;
	state->descriptors_dirty = true;
	rtvk_lowered_command_buffer_add_resource_job(lowered, RTVK_RESOURCE_BASE(command->buffer));
}

void rtvk_lower_bind_texture(struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_texture* command) {
	if (!command->location) {
		return;
	}
	rtvk_lower_reserve_bindings(state, command->location->index + 1);
	if (rtvk_error() != RT_SUCCESS) {
		return;
	}
	state->bound_textures[command->location->index] = command->view;
	state->bound_buffers[command->location->index] = NULL;
	state->descriptors_dirty = true;
	rtvk_lowered_command_buffer_add_resource_job(lowered, RTVK_RESOURCE_BASE(command->view));
}

void rtvk_lower_vertex_buffer(struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_vertex_buffer* command) {
	struct rtvk_buffer* buffer = command->buffer ? command->buffer->active : NULL;
	if (!command->location || !buffer) {
		return;
	}
	rtvk_lower_reserve_vertex_buffers(state, command->location->binding + 1);
	if (rtvk_error() != RT_SUCCESS) {
		return;
	}
	state->vertex_buffers[command->location->binding] = buffer;
	state->vertex_offsets[command->location->binding] = command->offset;
	rtvk_lowered_command_buffer_add_resource_job(lowered, RTVK_RESOURCE_BASE(command->buffer));
	struct rtvk_lowered_command_segment* segment = rtvk_lowered_command_buffer_current_segment(lowered);
	vkCmdBindVertexBuffers(segment->vk_command_buffer, command->location->binding, 1, &buffer->vk_buffer, &state->vertex_offsets[command->location->binding]);
	segment->command_count++;
}

void rtvk_lower_index_buffer(struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_index_buffer* command) {
	struct rtvk_buffer* buffer = command->buffer ? command->buffer->active : NULL;
	if (!buffer) {
		return;
	}
	state->index_buffer = buffer;
	state->index_offset = command->offset;
	state->index_format = command->format;
	rtvk_lowered_command_buffer_add_resource_job(lowered, RTVK_RESOURCE_BASE(command->buffer));
	struct rtvk_lowered_command_segment* segment = rtvk_lowered_command_buffer_current_segment(lowered);
	VkIndexType type = command->format == RT_INDEX_U16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
	vkCmdBindIndexBuffer(segment->vk_command_buffer, buffer->vk_buffer, command->offset, type);
	segment->command_count++;
}

void rtvk_lower_draw(struct rtvk_context* ctx, struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_draw* command) {
	struct rtvk_lowered_command_segment* segment = rtvk_lowered_command_buffer_current_segment(lowered);
	rtvk_lower_bind_descriptors(ctx, segment, state);
	if (rtvk_error() != RT_SUCCESS) {
		return;
	}
	vkCmdDraw(segment->vk_command_buffer, command->count, 1, command->first, 0);
	segment->command_count++;
}

void rtvk_lower_draw_instanced(struct rtvk_context* ctx, struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_draw_instanced* command) {
	struct rtvk_lowered_command_segment* segment = rtvk_lowered_command_buffer_current_segment(lowered);
	rtvk_lower_bind_descriptors(ctx, segment, state);
	if (rtvk_error() != RT_SUCCESS) {
		return;
	}
	vkCmdDraw(segment->vk_command_buffer, command->count, command->instances, command->first, command->first_instance);
	segment->command_count++;
}

void rtvk_lower_draw_indexed(struct rtvk_context* ctx, struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_draw_indexed* command) {
	struct rtvk_lowered_command_segment* segment = rtvk_lowered_command_buffer_current_segment(lowered);
	rtvk_lower_bind_descriptors(ctx, segment, state);
	if (rtvk_error() != RT_SUCCESS) {
		return;
	}
	vkCmdDrawIndexed(segment->vk_command_buffer, command->count, 1, command->first, command->vertex_offset, 0);
	segment->command_count++;
}

void rtvk_lower_draw_indexed_instanced(struct rtvk_context* ctx, struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_draw_indexed_instanced* command) {
	struct rtvk_lowered_command_segment* segment = rtvk_lowered_command_buffer_current_segment(lowered);
	rtvk_lower_bind_descriptors(ctx, segment, state);
	if (rtvk_error() != RT_SUCCESS) {
		return;
	}
	vkCmdDrawIndexed(segment->vk_command_buffer, command->count, command->instances, command->first, command->vertex_offset, command->first_instance);
	segment->command_count++;
}

void rtvk_lower_resume_segment(struct rtvk_context* ctx, struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state) {
	struct rtvk_lowered_command_segment* segment = rtvk_lowered_command_buffer_current_segment(lowered);
	VkMemoryBarrier barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
	barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	vkCmdPipelineBarrier(
		segment->vk_command_buffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0,
		1,
		&barrier,
		0,
		NULL,
		0,
		NULL
	);
	segment->command_count++;

	if (state->program) {
		rtvk_lower_bind_program(ctx, segment, state);
		segment->command_count++;
	}
	if (state->viewport_set) {
		vkCmdSetViewport(segment->vk_command_buffer, 0, 1, &state->viewport);
		segment->command_count++;
	}
	if (state->scissor_set) {
		vkCmdSetScissor(segment->vk_command_buffer, 0, 1, &state->scissor);
		segment->command_count++;
	}
	for (usize index = 0; index < state->vertex_buffer_capacity; index++) {
		struct rtvk_buffer* buffer = state->vertex_buffers[index];
		if (buffer) {
			vkCmdBindVertexBuffers(segment->vk_command_buffer, (u32)index, 1, &buffer->vk_buffer, &state->vertex_offsets[index]);
			segment->command_count++;
		}
	}
	if (state->index_buffer) {
		VkIndexType type = state->index_format == RT_INDEX_U16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
		vkCmdBindIndexBuffer(segment->vk_command_buffer, state->index_buffer->vk_buffer, state->index_offset, type);
		segment->command_count++;
	}
	state->descriptors_dirty = true;
}

void rtvk_lower_wait(struct rtvk_context* ctx, struct rtvk_lowered_command_buffer* lowered, struct rtvk_lower_state* state, const struct rtvk_ir_wait* command) {
	struct rtvk_lowered_command_segment* segment = rtvk_lowered_command_buffer_current_segment(lowered);
	rtvk_lowered_command_buffer_end_segment(segment);
	if (rtvk_error() != RT_SUCCESS) {
		return;
	}
	lowered->segment_index++;
	rtvk_lowered_command_buffer_current_segment(lowered)->wait = command->timepoint;
	rtvk_lower_resume_segment(ctx, lowered, state);
}

void rtvk_command_buffer_lower(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer, struct rtvk_lowered_command_buffer* lowered) {
	struct rtvk_lower_state state = { 0 };
	for (usize offset = 0; offset < command_buffer->ir_size; (void)0) {
		struct rtvk_command_header* header = (struct rtvk_command_header*)(command_buffer->ir_data + offset);
		void* payload = header + 1;
		switch ((rtvk_command_opcode)header->opcode) {
		case RTVK_COMMAND_WAIT:
			rtvk_lower_wait(ctx, lowered, &state, payload);
			break;
		case RTVK_COMMAND_BEGIN_RENDERING:
			rtvk_lower_begin_rendering_command(lowered, &state, payload);
			break;
		case RTVK_COMMAND_CLEAR_COLOR:
			rtvk_lower_clear_color(lowered, &state, payload);
			break;
		case RTVK_COMMAND_CLEAR_DEPTH:
			rtvk_lower_clear_depth(lowered, &state, payload);
			break;
		case RTVK_COMMAND_CLEAR_STENCIL:
			rtvk_lower_clear_stencil(lowered, &state, payload);
			break;
		case RTVK_COMMAND_SET_VIEWPORT:
			rtvk_lower_set_viewport(lowered, &state, payload);
			break;
		case RTVK_COMMAND_SET_SCISSOR:
			rtvk_lower_set_scissor(lowered, &state, payload);
			break;
		case RTVK_COMMAND_END_RENDERING:
			rtvk_lower_end_rendering(lowered, &state);
			break;
		case RTVK_COMMAND_USE_GRAPHICS_PROGRAM:
			rtvk_lower_use_graphics_program(ctx, lowered, &state, payload);
			break;
		case RTVK_COMMAND_BIND_BUFFER:
			rtvk_lower_bind_buffer(lowered, &state, payload);
			break;
		case RTVK_COMMAND_BIND_TEXTURE:
			rtvk_lower_bind_texture(lowered, &state, payload);
			break;
		case RTVK_COMMAND_VERTEX_BUFFER:
			rtvk_lower_vertex_buffer(lowered, &state, payload);
			break;
		case RTVK_COMMAND_INDEX_BUFFER:
			rtvk_lower_index_buffer(lowered, &state, payload);
			break;
		case RTVK_COMMAND_DRAW:
			rtvk_lower_draw(ctx, lowered, &state, payload);
			break;
		case RTVK_COMMAND_DRAW_INSTANCED:
			rtvk_lower_draw_instanced(ctx, lowered, &state, payload);
			break;
		case RTVK_COMMAND_DRAW_INDEXED:
			rtvk_lower_draw_indexed(ctx, lowered, &state, payload);
			break;
		case RTVK_COMMAND_DRAW_INDEXED_INSTANCED:
			rtvk_lower_draw_indexed_instanced(ctx, lowered, &state, payload);
			break;
		}
		if (rtvk_error() != RT_SUCCESS) {
			rtvk_lower_state_finish(&state);
			return;
		}
		offset += rtvk_command_record_size((rtvk_command_opcode)header->opcode);
	}

	rtvk_lowered_command_buffer_end_segment(rtvk_lowered_command_buffer_current_segment(lowered));
	if (rtvk_error() != RT_SUCCESS) {
		rtvk_lower_state_finish(&state);
		return;
	}
	rtvk_lower_state_finish(&state);
}

void rtvk_lowered_command_buffer_destroy(struct rtvk_context* ctx, struct rtvk_lowered_command_buffer* lowered) {
	if (!lowered) {
		return;
	}
	for (usize index = 0; index < lowered->segment_count; index++) {
		vkDestroyDescriptorPool(ctx->vk_device, lowered->segments[index].vk_descriptor_pool, VK_ALLOCATOR);
	}
	vkDestroyCommandPool(ctx->vk_device, lowered->vk_command_pool, VK_ALLOCATOR);
	rtvk_lowered_command_buffer_release_resource_jobs(lowered);
	free(lowered->segments);
	free(lowered);
}
