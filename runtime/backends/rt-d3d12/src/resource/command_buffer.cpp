#include "command_buffer.hpp"
#include "context.hpp"
#include "error.hpp"
#include "resource/buffer.hpp"
#include "resource/framebuffer.hpp"
#include "resource/graphics_program.hpp"
#include "resource/texture.hpp"

#include <stdlib.h>
#include <string.h>

static void rtdx_lower_texture_revision_copy(ID3D12GraphicsCommandList* command_list, rtdx_image_base* source, rtdx_image_base* target);

/*===============================================================================================*/
/*                                                                                                */
/*===============================================================================================*/

rt_command_buffer rtCommandBufferCreate(void) {
	return rtdx_command_buffer_to_handle(rtdx_command_buffer_create(rtdx_get_current_context()));
}

void rtCommandBufferDestroy(rt_command_buffer command_buffer) {
	rtdx_command_buffer_destroy(rtdx_get_current_context(), rtdx_command_buffer_from_handle(command_buffer));
}

void rtCommandBufferReset(rt_command_buffer command_buffer) {
	rtdx_command_buffer_reset(rtdx_command_buffer_from_handle(command_buffer));
}

void rtCommandBufferBegin(rt_command_buffer command_buffer) {
	rtdx_command_buffer_begin(rtdx_command_buffer_from_handle(command_buffer));
}

void rtCommandBufferContinue(rt_command_buffer command_buffer) { rtdx_command_buffer_continue(rtdx_command_buffer_from_handle(command_buffer), false); }
void rtCommandBufferContinueRendering(rt_command_buffer command_buffer) { rtdx_command_buffer_continue(rtdx_command_buffer_from_handle(command_buffer), true); }
void rtCommandBufferEnd(rt_command_buffer command_buffer) { rtdx_command_buffer_end(rtdx_command_buffer_from_handle(command_buffer)); }
void rtCmdExecute(rt_command_buffer command_buffer, rt_command_buffer secondary) { rtdx_command_buffer_execute(rtdx_command_buffer_from_handle(command_buffer), rtdx_command_buffer_from_handle(secondary)); }

void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer) {
	rtdx_command_buffer_begin_rendering(rtdx_command_buffer_from_handle(command_buffer), rtdx_framebuffer_from_handle(framebuffer));
}

void rtCmdClearColor(rt_command_buffer command_buffer, rt_location location, f32 r, f32 g, f32 b, f32 a) {
	rtdx_location* private_location = rtdx_location_from_handle(location);
	rtdx_command_buffer_clear_color(rtdx_command_buffer_from_handle(command_buffer), private_location ? private_location->slot : 0, r, g, b, a);
}

void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth) {
	rtdx_command_buffer_clear_depth(rtdx_command_buffer_from_handle(command_buffer), depth);
}

void rtCmdClearStencil(rt_command_buffer command_buffer, usize stencil) {
	rtdx_command_buffer_clear_stencil(rtdx_command_buffer_from_handle(command_buffer), (u32)stencil);
}

void rtCmdClear(rt_command_buffer command_buffer, enum rt_clear_flag attachments) {
	rtdx_command_buffer_clear(rtdx_command_buffer_from_handle(command_buffer), attachments);
}

void rtCmdSetViewport(rt_command_buffer command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth) {
	rtdx_command_buffer_set_viewport(rtdx_command_buffer_from_handle(command_buffer), x, y, width, height, min_depth, max_depth);
}

void rtCmdSetScissor(rt_command_buffer command_buffer, usize x, usize y, usize width, usize height) {
	rtdx_command_buffer_set_scissor(rtdx_command_buffer_from_handle(command_buffer), x, y, width, height);
}

void rtCmdEndRendering(rt_command_buffer command_buffer) {
	rtdx_command_buffer_end_rendering(rtdx_command_buffer_from_handle(command_buffer));
}

void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program) {
	rtdx_command_buffer_use_graphics_program(rtdx_command_buffer_from_handle(command_buffer), rtdx_graphics_program_from_handle(program));
}

void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range) {
	rtdx_command_buffer_bind_buffer(rtdx_command_buffer_from_handle(command_buffer), location, rtdx_buffer_from_handle(buffer), range.offset, range.size);
}

void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view) {
	rtdx_command_buffer_bind_texture(rtdx_command_buffer_from_handle(command_buffer), location, rtdx_texture_view_from_handle(texture_view));
}

void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range) {
	(void)range.size;
	rtdx_command_buffer_vertex_buffer(rtdx_command_buffer_from_handle(command_buffer), location, rtdx_buffer_from_handle(buffer), range.offset);
}

void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, enum rt_index_format format) {
	(void)range.size;
	rtdx_command_buffer_index_buffer(rtdx_command_buffer_from_handle(command_buffer), rtdx_buffer_from_handle(buffer), range.offset, format);
}

void rtCmdDraw(rt_command_buffer command_buffer, usize vertex_count, usize first_vertex) {
	rtdx_command_buffer_draw(rtdx_command_buffer_from_handle(command_buffer), vertex_count, first_vertex);
}

void rtCmdDrawInstanced(rt_command_buffer command_buffer, usize vertex_count, usize instance_count, usize first_vertex, usize first_instance) {
	rtdx_command_buffer_draw_instanced(rtdx_command_buffer_from_handle(command_buffer), vertex_count, instance_count, first_vertex, first_instance);
}

void rtCmdDrawIndexed(rt_command_buffer command_buffer, usize index_count, usize first_index, usize vertex_offset) {
	rtdx_command_buffer_draw_indexed(rtdx_command_buffer_from_handle(command_buffer), index_count, first_index, vertex_offset);
}

void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, usize index_count, usize instance_count, usize first_index, usize vertex_offset, usize first_instance) {
	rtdx_command_buffer_draw_indexed_instanced(rtdx_command_buffer_from_handle(command_buffer), index_count, instance_count, first_index, vertex_offset, first_instance);
}

void rtCmdBufferData(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, const u08* data) {
	rtdx_command_buffer_buffer_data(rtdx_command_buffer_from_handle(command_buffer), rtdx_buffer_from_handle(buffer), range, data);
}

void rtCmdBufferCopy(rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_buffer dst, rt_buffer_range dst_range) {
	rtdx_command_buffer_buffer_copy(rtdx_command_buffer_from_handle(command_buffer), rtdx_buffer_from_handle(src), src_range, rtdx_buffer_from_handle(dst), dst_range);
}

void rtCmdBufferCopyToTexture(rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_texture dst, rt_texture_range dst_range) {
	rtdx_command_buffer_buffer_copy_to_texture(rtdx_command_buffer_from_handle(command_buffer), rtdx_buffer_from_handle(src), src_range, rtdx_texture_from_handle(dst), dst_range);
}

void rtCmdBufferBarrier(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, rt_access src, rt_access dst) {
	rtdx_command_buffer_buffer_barrier(rtdx_command_buffer_from_handle(command_buffer), rtdx_buffer_from_handle(buffer), range, src, dst);
}

/*===============================================================================================*/
/*                                                                                                */
/*===============================================================================================*/

RTDX_DEFINE_RESOURCE_PRIVATE(command_buffer)

void rtdx_command_transition_buffer(ID3D12GraphicsCommandList* command_list, rtdx_buffer_storage* storage, D3D12_RESOURCE_STATES state) {
	if (!storage || !storage->d3d_resource || storage->state == state) {
		return;
	}
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = storage->d3d_resource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = storage->state;
	barrier.Transition.StateAfter = state;
	command_list->ResourceBarrier(1, &barrier);
	storage->state = state;
}

void rtdx_command_transition_image(ID3D12GraphicsCommandList* command_list, rtdx_image_base* image, D3D12_RESOURCE_STATES state) {
	if (!image || !image->d3d_resource) { return; }
	const usize count = rtdx_texture_subresource_count(image);
	for (usize layer = 0; layer < image->layer_count; ++layer) {
		for (usize mip = 0; mip < image->mip_count; ++mip) {
			D3D12_RESOURCE_STATES before = rtdx_texture_subresource_state(image, mip, layer);
			if (before == state) { continue; }
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = image->d3d_resource;
			barrier.Transition.Subresource = (UINT)(layer * image->mip_count + mip);
			barrier.Transition.StateBefore = before;
			barrier.Transition.StateAfter = state;
			command_list->ResourceBarrier(1, &barrier);
			rtdx_texture_set_subresource_state(image, mip, layer, state);
		}
	}
	if (!count) { image->state = state; }
}

void rtdx_command_transition_image_range(ID3D12GraphicsCommandList* command_list, rtdx_image_base* image, rt_texture_range range, D3D12_RESOURCE_STATES state) {
	if (!image || !image->d3d_resource) { return; }
	for (usize layer = 0; layer < range.layer_count; ++layer) {
		for (usize mip = 0; mip < range.mip_count; ++mip) {
			const usize actual_mip = range.base_mip + mip;
			const usize actual_layer = range.base_layer + layer;
			D3D12_RESOURCE_STATES before = rtdx_texture_subresource_state(image, actual_mip, actual_layer);
			if (before == state) { continue; }
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = image->d3d_resource;
			barrier.Transition.Subresource = (UINT)(actual_layer * image->mip_count + actual_mip);
			barrier.Transition.StateBefore = before;
			barrier.Transition.StateAfter = state;
			command_list->ResourceBarrier(1, &barrier);
			rtdx_texture_set_subresource_state(image, actual_mip, actual_layer, state);
		}
	}
}

void rtdx_command_buffer_init(rtdx_context* ctx, rtdx_command_buffer* command_buffer) {
	rtdx_init_resource_base(ctx, RTDX_RESOURCE_BASE(command_buffer), rtdx_resource_type::command_buffer);
	command_buffer->clear_depth_value = 1.0f;
}

void rtdx_command_buffer_finish(rtdx_context* ctx, rtdx_command_buffer* command_buffer) {
	rtdx_command_buffer_release_resources(command_buffer);
	free(command_buffer->ir_data);
	rtdx_finish_resource_base(ctx, RTDX_RESOURCE_BASE(command_buffer));
}

usize rtdx_command_record_size(rtdx_command_opcode opcode) {
	usize size = sizeof(rtdx_command_header);
	switch (opcode) {
	case rtdx_command_opcode::begin_rendering: size += sizeof(rtdx_ir_framebuffer); break;
	case rtdx_command_opcode::clear_color: size += sizeof(rtdx_ir_clear_color); break;
	case rtdx_command_opcode::clear_depth: size += sizeof(rtdx_ir_clear_depth); break;
	case rtdx_command_opcode::clear_stencil: size += sizeof(rtdx_ir_clear_stencil); break;
	case rtdx_command_opcode::set_viewport: size += sizeof(rtdx_ir_viewport); break;
	case rtdx_command_opcode::set_scissor: size += sizeof(rtdx_ir_scissor); break;
	case rtdx_command_opcode::end_rendering: break;
	case rtdx_command_opcode::use_graphics_program: size += sizeof(rtdx_ir_program); break;
	case rtdx_command_opcode::bind_buffer: size += sizeof(rtdx_ir_buffer); break;
	case rtdx_command_opcode::bind_texture: size += sizeof(rtdx_ir_texture); break;
	case rtdx_command_opcode::vertex_buffer: size += sizeof(rtdx_ir_vertex_buffer); break;
	case rtdx_command_opcode::index_buffer: size += sizeof(rtdx_ir_index_buffer); break;
	case rtdx_command_opcode::draw: size += sizeof(rtdx_ir_draw); break;
	case rtdx_command_opcode::draw_instanced: size += sizeof(rtdx_ir_draw_instanced); break;
	case rtdx_command_opcode::draw_indexed: size += sizeof(rtdx_ir_draw_indexed); break;
	case rtdx_command_opcode::draw_indexed_instanced: size += sizeof(rtdx_ir_draw_indexed_instanced); break;
	case rtdx_command_opcode::buffer_data: size += sizeof(rtdx_ir_buffer_data); break;
	case rtdx_command_opcode::buffer_copy: size += sizeof(rtdx_ir_buffer_copy); break;
	case rtdx_command_opcode::buffer_barrier: size += sizeof(rtdx_ir_buffer_barrier); break;
	case rtdx_command_opcode::buffer_copy_to_texture: size += sizeof(rtdx_ir_buffer_copy_to_texture); break;
	case rtdx_command_opcode::texture_copy: size += sizeof(rtdx_ir_texture_copy); break;
	case rtdx_command_opcode::texture_data: size += sizeof(rtdx_ir_texture_data); break;
	case rtdx_command_opcode::texture_copy_to_buffer: size += sizeof(rtdx_ir_texture_copy_to_buffer); break;
	case rtdx_command_opcode::texture_barrier: size += sizeof(rtdx_ir_texture_barrier); break;
	}
	return (size + alignof(void*) - 1) & ~(alignof(void*) - 1);
}

void* rtdx_command_append(rtdx_command_buffer* command_buffer, rtdx_command_opcode opcode) {
	if (!command_buffer || !command_buffer->recording) {
		return NULL;
	}
	usize size = rtdx_command_record_size(opcode);
	if (command_buffer->ir_capacity - command_buffer->ir_size < size) {
		usize required = command_buffer->ir_size + size;
		usize capacity = 1;
		while (capacity < required) { capacity <<= 1; }
		void* data = realloc(command_buffer->ir_data, capacity);
		if (!data) { rtdx_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for command IR", capacity); return NULL; }
		command_buffer->ir_data = static_cast<u08*>(data);
		command_buffer->ir_capacity = capacity;
	}
	rtdx_command_header* command = reinterpret_cast<rtdx_command_header*>(command_buffer->ir_data + command_buffer->ir_size);
	command->opcode = opcode;
	command_buffer->ir_size += size;
	return command + 1;
}

void rtdx_command_buffer_release_resources(rtdx_command_buffer* command_buffer) {
	for (usize offset = 0; offset < command_buffer->ir_size; (void)0) {
		rtdx_command_header* header = reinterpret_cast<rtdx_command_header*>(command_buffer->ir_data + offset);
		void* payload = header + 1;
		switch (header->opcode) {
		case rtdx_command_opcode::begin_rendering: {
			rtdx_ir_framebuffer* command = static_cast<rtdx_ir_framebuffer*>(payload);
			rtdx_framebuffer* framebuffer = command->framebuffer;
			if (framebuffer) { rtdx_resource_release(RTDX_RESOURCE_BASE(framebuffer)); }
			for (usize i = 0; i < command->color_count; ++i) { if (command->color_copy_sources[i]) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->color_copy_sources[i])); } if (command->color_images[i]) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->color_images[i])); } rtdx_release(&command->color_rtv_heaps[i]); }
			if (command->depth_copy_source) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->depth_copy_source)); }
			if (command->depth_image) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->depth_image)); }
			rtdx_release(&command->depth_dsv_heap);
			if (command->stencil_copy_source && command->stencil_copy_source != command->depth_copy_source) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->stencil_copy_source)); }
			if (command->stencil_image && command->stencil_image != command->depth_image) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->stencil_image)); }
			rtdx_release(&command->stencil_dsv_heap);
			break;
		}
		case rtdx_command_opcode::use_graphics_program: {
			rtdx_graphics_program* program = static_cast<rtdx_ir_program*>(payload)->program;
			if (program) { rtdx_resource_release(RTDX_RESOURCE_BASE(program)); }
			break;
		}
		case rtdx_command_opcode::bind_buffer: {
			rtdx_ir_buffer* command = static_cast<rtdx_ir_buffer*>(payload);
			rtdx_buffer_storage_release(command->storage);
			rtdx_location* location = rtdx_location_from_handle(command->location);
			if (location) { rtdx_resource_release(RTDX_RESOURCE_BASE(location->program)); }
			break;
		}
		case rtdx_command_opcode::bind_texture: {
			rtdx_ir_texture* command = static_cast<rtdx_ir_texture*>(payload);
			rtdx_texture_view* texture_view = command->texture_view;
			if (texture_view) { rtdx_resource_release(RTDX_RESOURCE_BASE(texture_view)); }
			if (command->image) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->image)); }
			rtdx_release(&command->sampler_heap);
			rtdx_location* location = rtdx_location_from_handle(command->location);
			if (location) { rtdx_resource_release(RTDX_RESOURCE_BASE(location->program)); }
			break;
		}
		case rtdx_command_opcode::vertex_buffer: {
			rtdx_ir_vertex_buffer* command = static_cast<rtdx_ir_vertex_buffer*>(payload);
			rtdx_buffer_storage_release(command->storage);
			rtdx_location* location = rtdx_location_from_handle(command->location);
			if (location) { rtdx_resource_release(RTDX_RESOURCE_BASE(location->program)); }
			break;
		}
		case rtdx_command_opcode::index_buffer: {
			rtdx_buffer_storage_release(static_cast<rtdx_ir_index_buffer*>(payload)->storage);
			break;
		}
		case rtdx_command_opcode::buffer_data: {
			rtdx_ir_buffer_data* command = static_cast<rtdx_ir_buffer_data*>(payload);
			rtdx_buffer_storage_release(command->copy_source);
			rtdx_buffer_storage_release(command->target);
			rtdx_release(&command->upload);
			break;
		}
		case rtdx_command_opcode::buffer_copy: {
			rtdx_ir_buffer_copy* command = static_cast<rtdx_ir_buffer_copy*>(payload);
			rtdx_buffer_storage_release(command->source);
			rtdx_buffer_storage_release(command->target_copy_source);
			rtdx_buffer_storage_release(command->target);
			break;
		}
		case rtdx_command_opcode::buffer_barrier:
			rtdx_buffer_storage_release(static_cast<rtdx_ir_buffer_barrier*>(payload)->storage);
			break;
		case rtdx_command_opcode::buffer_copy_to_texture: {
			rtdx_ir_buffer_copy_to_texture* command = static_cast<rtdx_ir_buffer_copy_to_texture*>(payload);
			rtdx_buffer_storage_release(command->source); if (command->source_texture) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->source_texture)); } if (command->target_copy_source) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->target_copy_source)); } if (command->target) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->target)); } rtdx_release(&command->upload);
			break;
		}
		case rtdx_command_opcode::texture_copy: {
			rtdx_ir_texture_copy* command = static_cast<rtdx_ir_texture_copy*>(payload);
			if (command->source) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->source)); } if (command->target_copy_source) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->target_copy_source)); } if (command->target) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->target)); }
			break;
		}
		case rtdx_command_opcode::texture_data: {
			rtdx_ir_texture_data* command = static_cast<rtdx_ir_texture_data*>(payload);
			if (command->copy_source) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->copy_source)); } if (command->target) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->target)); } free(command->data); rtdx_release(&command->upload);
			break;
		}
		case rtdx_command_opcode::texture_copy_to_buffer: {
			rtdx_ir_texture_copy_to_buffer* command = static_cast<rtdx_ir_texture_copy_to_buffer*>(payload);
			if (command->source) { rtdx_resource_release(RTDX_RESOURCE_BASE(command->source)); } rtdx_buffer_storage_release(command->target_copy_source); rtdx_buffer_storage_release(command->target); rtdx_release(&command->staging);
			break;
		}
		case rtdx_command_opcode::texture_barrier:
			if (static_cast<rtdx_ir_texture_barrier*>(payload)->image) { rtdx_resource_release(RTDX_RESOURCE_BASE(static_cast<rtdx_ir_texture_barrier*>(payload)->image)); }
			break;
		default:
			break;
		}
		offset += rtdx_command_record_size(header->opcode);
	}
	command_buffer->ir_size = 0;
}

void rtdx_command_buffer_reset(rtdx_command_buffer* command_buffer) {
	if (!command_buffer) { return; }
	rtdx_command_buffer_release_resources(command_buffer);
	command_buffer->recording = false;
	command_buffer->executable = false;
	command_buffer->continuation = false;
	command_buffer->rendering_continuation = false;
	command_buffer->rendering = false;
	command_buffer->active_framebuffer = NULL;
}

static void rtdx_command_buffer_begin_mode(rtdx_command_buffer* command_buffer, bool continuation, bool rendering_continuation) {
	if (!command_buffer) { return; }
	rtdx_command_buffer_release_resources(command_buffer);
	command_buffer->recording = true;
	command_buffer->executable = false;
	command_buffer->continuation = continuation;
	command_buffer->rendering_continuation = rendering_continuation;
	command_buffer->rendering = false;
}

void rtdx_command_buffer_begin(rtdx_command_buffer* command_buffer) { rtdx_command_buffer_begin_mode(command_buffer, false, false); }
void rtdx_command_buffer_continue(rtdx_command_buffer* command_buffer, bool rendering) { rtdx_command_buffer_begin_mode(command_buffer, true, rendering); }

static void rtdx_command_buffer_retain_payload(rtdx_command_opcode opcode, void* payload) {
	switch (opcode) {
	case rtdx_command_opcode::begin_rendering: {
		rtdx_ir_framebuffer* command = static_cast<rtdx_ir_framebuffer*>(payload);
		rtdx_framebuffer* framebuffer = command->framebuffer;
		if (framebuffer) { rtdx_resource_retain(RTDX_RESOURCE_BASE(framebuffer)); }
		for (usize i = 0; i < command->color_count; ++i) { if (command->color_copy_sources[i]) { rtdx_resource_retain(RTDX_RESOURCE_BASE(command->color_copy_sources[i])); } if (command->color_images[i]) { rtdx_resource_retain(RTDX_RESOURCE_BASE(command->color_images[i])); } if (command->color_rtv_heaps[i]) { command->color_rtv_heaps[i]->AddRef(); } }
		if (command->depth_copy_source) { rtdx_resource_retain(RTDX_RESOURCE_BASE(command->depth_copy_source)); }
		if (command->depth_image) { rtdx_resource_retain(RTDX_RESOURCE_BASE(command->depth_image)); }
		if (command->depth_dsv_heap) { command->depth_dsv_heap->AddRef(); }
		if (command->stencil_copy_source && command->stencil_copy_source != command->depth_copy_source) { rtdx_resource_retain(RTDX_RESOURCE_BASE(command->stencil_copy_source)); }
		if (command->stencil_image && command->stencil_image != command->depth_image) { rtdx_resource_retain(RTDX_RESOURCE_BASE(command->stencil_image)); }
		if (command->stencil_dsv_heap) { command->stencil_dsv_heap->AddRef(); }
		break;
	}
	case rtdx_command_opcode::use_graphics_program: {
		rtdx_graphics_program* program = static_cast<rtdx_ir_program*>(payload)->program;
		if (program) { rtdx_resource_retain(RTDX_RESOURCE_BASE(program)); }
		break;
	}
	case rtdx_command_opcode::bind_buffer: {
		rtdx_ir_buffer* command = static_cast<rtdx_ir_buffer*>(payload);
		rtdx_buffer_storage_retain(command->storage);
		rtdx_location* location = rtdx_location_from_handle(command->location);
		if (location) { rtdx_resource_retain(RTDX_RESOURCE_BASE(location->program)); }
		break;
	}
	case rtdx_command_opcode::bind_texture: {
		rtdx_ir_texture* command = static_cast<rtdx_ir_texture*>(payload);
		if (command->texture_view) { rtdx_resource_retain(RTDX_RESOURCE_BASE(command->texture_view)); }
		if (command->image) { rtdx_resource_retain(RTDX_RESOURCE_BASE(command->image)); }
		if (command->sampler_heap) { command->sampler_heap->AddRef(); }
		rtdx_location* location = rtdx_location_from_handle(command->location);
		if (location) { rtdx_resource_retain(RTDX_RESOURCE_BASE(location->program)); }
		break;
	}
	case rtdx_command_opcode::vertex_buffer: {
		rtdx_ir_vertex_buffer* command = static_cast<rtdx_ir_vertex_buffer*>(payload);
		rtdx_buffer_storage_retain(command->storage);
		rtdx_location* location = rtdx_location_from_handle(command->location);
		if (location) { rtdx_resource_retain(RTDX_RESOURCE_BASE(location->program)); }
		break;
	}
	case rtdx_command_opcode::index_buffer:
		rtdx_buffer_storage_retain(static_cast<rtdx_ir_index_buffer*>(payload)->storage);
		break;
	case rtdx_command_opcode::buffer_data: {
		rtdx_ir_buffer_data* command = static_cast<rtdx_ir_buffer_data*>(payload);
		rtdx_buffer_storage_retain(command->copy_source);
		rtdx_buffer_storage_retain(command->target);
		if (command->upload) { command->upload->AddRef(); }
		break;
	}
	case rtdx_command_opcode::buffer_copy: {
		rtdx_ir_buffer_copy* command = static_cast<rtdx_ir_buffer_copy*>(payload);
		rtdx_buffer_storage_retain(command->source);
		rtdx_buffer_storage_retain(command->target_copy_source);
		rtdx_buffer_storage_retain(command->target);
		break;
	}
	case rtdx_command_opcode::buffer_barrier:
		rtdx_buffer_storage_retain(static_cast<rtdx_ir_buffer_barrier*>(payload)->storage);
		break;
	case rtdx_command_opcode::buffer_copy_to_texture: { rtdx_ir_buffer_copy_to_texture* c = static_cast<rtdx_ir_buffer_copy_to_texture*>(payload); rtdx_buffer_storage_retain(c->source); if (c->source_texture) { rtdx_resource_retain(RTDX_RESOURCE_BASE(c->source_texture)); } if (c->target_copy_source) { rtdx_resource_retain(RTDX_RESOURCE_BASE(c->target_copy_source)); } if (c->target) { rtdx_resource_retain(RTDX_RESOURCE_BASE(c->target)); } if (c->upload) { c->upload->AddRef(); } break; }
	case rtdx_command_opcode::texture_copy: { rtdx_ir_texture_copy* c = static_cast<rtdx_ir_texture_copy*>(payload); if (c->source) { rtdx_resource_retain(RTDX_RESOURCE_BASE(c->source)); } if (c->target_copy_source) { rtdx_resource_retain(RTDX_RESOURCE_BASE(c->target_copy_source)); } if (c->target) { rtdx_resource_retain(RTDX_RESOURCE_BASE(c->target)); } break; }
	case rtdx_command_opcode::texture_data: { rtdx_ir_texture_data* c = static_cast<rtdx_ir_texture_data*>(payload); if (c->copy_source) { rtdx_resource_retain(RTDX_RESOURCE_BASE(c->copy_source)); } if (c->target) { rtdx_resource_retain(RTDX_RESOURCE_BASE(c->target)); } if (c->upload) { c->upload->AddRef(); } if (c->data_size) { u08* copy = static_cast<u08*>(malloc(c->data_size)); if (!copy) { rtdx_throwf(RT_OUT_OF_HOST_MEMORY, "failed to clone texture upload bytes"); c->data_size = 0; c->data = NULL; } else { memcpy(copy, c->data, c->data_size); c->data = copy; } } break; }
	case rtdx_command_opcode::texture_copy_to_buffer: { rtdx_ir_texture_copy_to_buffer* c = static_cast<rtdx_ir_texture_copy_to_buffer*>(payload); if (c->source) { rtdx_resource_retain(RTDX_RESOURCE_BASE(c->source)); } rtdx_buffer_storage_retain(c->target_copy_source); rtdx_buffer_storage_retain(c->target); if (c->staging) { c->staging->AddRef(); } break; }
	case rtdx_command_opcode::texture_barrier: { rtdx_image_base* image = static_cast<rtdx_ir_texture_barrier*>(payload)->image; if (image) { rtdx_resource_retain(RTDX_RESOURCE_BASE(image)); } break; }
	default:
		break;
	}
}

rtdx_command_buffer* rtdx_command_buffer_snapshot_create(const rtdx_command_buffer* command_buffer) {
	if (!command_buffer || !command_buffer->ir_size) {
		return NULL;
	}
	rtdx_command_buffer* snapshot = static_cast<rtdx_command_buffer*>(calloc(1, sizeof(*snapshot)));
	if (!snapshot) {
		rtdx_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate submitted command snapshot");
		return NULL;
	}
	snapshot->ir_data = static_cast<u08*>(malloc(command_buffer->ir_size));
	if (!snapshot->ir_data) {
		free(snapshot);
		rtdx_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate submitted command IR");
		return NULL;
	}
	snapshot->ir_size = command_buffer->ir_size;
	snapshot->ir_capacity = command_buffer->ir_size;
	memcpy(snapshot->ir_data, command_buffer->ir_data, snapshot->ir_size);
	for (usize offset = 0; offset < snapshot->ir_size;) {
		rtdx_command_header* header = reinterpret_cast<rtdx_command_header*>(snapshot->ir_data + offset);
		rtdx_command_buffer_retain_payload(header->opcode, header + 1);
		offset += rtdx_command_record_size(header->opcode);
	}
	return snapshot;
}

void rtdx_command_buffer_snapshot_destroy(rtdx_command_buffer* snapshot) {
	if (!snapshot) {
		return;
	}
	rtdx_command_buffer_release_resources(snapshot);
	free(snapshot->ir_data);
	free(snapshot);
}

void rtdx_command_buffer_execute(rtdx_command_buffer* command_buffer, rtdx_command_buffer* secondary) {
	if (!command_buffer || !secondary || command_buffer == secondary || !command_buffer->recording || !secondary->executable || !secondary->continuation || (secondary->rendering_continuation && !command_buffer->rendering)) {
		return;
	}
	for (usize offset = 0; offset < secondary->ir_size;) {
		rtdx_command_header* source = reinterpret_cast<rtdx_command_header*>(secondary->ir_data + offset);
		const usize record_size = rtdx_command_record_size(source->opcode);
		void* destination = rtdx_command_append(command_buffer, source->opcode);
		if (!destination) { return; }
		memcpy(destination, source + 1, record_size - sizeof(*source));
		rtdx_command_buffer_retain_payload(source->opcode, destination);
		offset += record_size;
	}
}
void rtdx_command_buffer_begin_rendering(rtdx_command_buffer* command_buffer, rtdx_framebuffer* framebuffer) {
	if (!command_buffer || !command_buffer->recording || command_buffer->continuation || command_buffer->rendering || !framebuffer || !rtdx_framebuffer_valid(framebuffer)) { return; }
	rtdx_ir_framebuffer* command = static_cast<rtdx_ir_framebuffer*>(rtdx_command_append(command_buffer, rtdx_command_opcode::begin_rendering));
	if (!command) { return; }
	*command = {};
	command->framebuffer = framebuffer;
	command->color_count = framebuffer->color_texture_count;
	rtdx_resource_retain(RTDX_RESOURCE_BASE(framebuffer));
	for (usize i = 0; i < command->color_count; ++i) {
		rtdx_texture_view* view = framebuffer->color_views[i];
		if (!rtdx_texture_view_refresh(rtdx_get_current_context(), view)) { return; }
		rtdx_texture_write write = rtdx_texture_write_begin(rtdx_get_current_context(), view->texture);
		if (!write.target || !rtdx_texture_view_refresh(rtdx_get_current_context(), view)) { return; }
		command->color_copy_sources[i] = write.source;
		command->color_images[i] = write.target;
		command->color_rtvs[i] = view->rtv;
		command->color_rtv_heaps[i] = view->d3d_rtv_heap;
		if (command->color_rtv_heaps[i]) { command->color_rtv_heaps[i]->AddRef(); }
		if (write.source) { rtdx_resource_retain(RTDX_RESOURCE_BASE(write.source)); }
		rtdx_resource_retain(RTDX_RESOURCE_BASE(write.target));
	}
	rtdx_texture_view* depth_view = framebuffer->depth_view;
	rtdx_texture_view* stencil_view = framebuffer->stencil_view;
	if (depth_view) {
		if (!rtdx_texture_view_refresh(rtdx_get_current_context(), depth_view)) { return; }
		rtdx_texture_write write = rtdx_texture_write_begin(rtdx_get_current_context(), depth_view->texture);
		if (!write.target || !rtdx_texture_view_refresh(rtdx_get_current_context(), depth_view)) { return; }
		command->depth_copy_source = write.source;
		command->depth_image = write.target;
		command->depth_dsv = depth_view->dsv;
		command->depth_dsv_heap = depth_view->d3d_dsv_heap;
		if (command->depth_dsv_heap) { command->depth_dsv_heap->AddRef(); }
		if (write.source) { rtdx_resource_retain(RTDX_RESOURCE_BASE(write.source)); }
		rtdx_resource_retain(RTDX_RESOURCE_BASE(write.target));
	}
	if (stencil_view) {
		rtdx_texture_write write = {};
		if (depth_view && stencil_view->texture == depth_view->texture) {
			write.source = command->depth_copy_source;
			write.target = command->depth_image;
		} else {
			if (!rtdx_texture_view_refresh(rtdx_get_current_context(), stencil_view)) { return; }
			write = rtdx_texture_write_begin(rtdx_get_current_context(), stencil_view->texture);
			if (!write.target) { return; }
		}
		if (!rtdx_texture_view_refresh(rtdx_get_current_context(), stencil_view)) { return; }
		command->stencil_copy_source = write.source;
		command->stencil_image = write.target;
		command->stencil_dsv = stencil_view->dsv;
		command->stencil_dsv_heap = stencil_view->d3d_dsv_heap;
		if (command->stencil_dsv_heap) { command->stencil_dsv_heap->AddRef(); }
		if (write.source && write.source != command->depth_copy_source) { rtdx_resource_retain(RTDX_RESOURCE_BASE(write.source)); }
		if (write.target && write.target != command->depth_image) { rtdx_resource_retain(RTDX_RESOURCE_BASE(write.target)); }
	}
	command_buffer->active_framebuffer = framebuffer;
	command_buffer->rendering = true;
}
void rtdx_command_buffer_clear_color(rtdx_command_buffer* command_buffer, u32 index, f32 r, f32 g, f32 b, f32 a) { if (!command_buffer || !command_buffer->recording || !command_buffer->rendering || index >= 8) { return; } command_buffer->clear_colors[index][0] = r; command_buffer->clear_colors[index][1] = g; command_buffer->clear_colors[index][2] = b; command_buffer->clear_colors[index][3] = a; }
void rtdx_command_buffer_clear_depth(rtdx_command_buffer* command_buffer, f32 depth) { if (command_buffer && command_buffer->recording && command_buffer->rendering) { command_buffer->clear_depth_value = depth; } }
void rtdx_command_buffer_clear_stencil(rtdx_command_buffer* command_buffer, u32 stencil) { if (command_buffer && command_buffer->recording && command_buffer->rendering) { command_buffer->clear_stencil_value = stencil; } }
void rtdx_command_buffer_clear(rtdx_command_buffer* command_buffer, rt_clear_flag attachments) { if (!command_buffer || !command_buffer->recording || !command_buffer->rendering) { return; } if (attachments & RT_CLEAR_COLOR) { const u32 count = command_buffer->active_framebuffer ? command_buffer->active_framebuffer->color_texture_count : 0; for (u32 index = 0; index < count; ++index) { rtdx_ir_clear_color* c = static_cast<rtdx_ir_clear_color*>(rtdx_command_append(command_buffer, rtdx_command_opcode::clear_color)); if (c) { *c = { index, command_buffer->clear_colors[index][0], command_buffer->clear_colors[index][1], command_buffer->clear_colors[index][2], command_buffer->clear_colors[index][3] }; } } } if (attachments & RT_CLEAR_DEPTH) { rtdx_ir_clear_depth* c = static_cast<rtdx_ir_clear_depth*>(rtdx_command_append(command_buffer, rtdx_command_opcode::clear_depth)); if (c) { c->depth = command_buffer->clear_depth_value; } } if (attachments & RT_CLEAR_STENCIL) { rtdx_ir_clear_stencil* c = static_cast<rtdx_ir_clear_stencil*>(rtdx_command_append(command_buffer, rtdx_command_opcode::clear_stencil)); if (c) { c->stencil = command_buffer->clear_stencil_value; } } }
void rtdx_command_buffer_set_viewport(rtdx_command_buffer* command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth) { rtdx_ir_viewport* command = static_cast<rtdx_ir_viewport*>(rtdx_command_append(command_buffer, rtdx_command_opcode::set_viewport)); if (!command) { return; } *command = { x, y, width, height, min_depth, max_depth }; }
void rtdx_command_buffer_set_scissor(rtdx_command_buffer* command_buffer, usize x, usize y, usize width, usize height) { rtdx_ir_scissor* command = static_cast<rtdx_ir_scissor*>(rtdx_command_append(command_buffer, rtdx_command_opcode::set_scissor)); if (!command) { return; } *command = { x, y, width, height }; }
void rtdx_command_buffer_end_rendering(rtdx_command_buffer* command_buffer) { if (!command_buffer || command_buffer->continuation || !command_buffer->rendering) { return; } rtdx_command_append(command_buffer, rtdx_command_opcode::end_rendering); command_buffer->rendering = false; command_buffer->active_framebuffer = NULL; }
void rtdx_command_buffer_use_graphics_program(rtdx_command_buffer* command_buffer, rtdx_graphics_program* program) { rtdx_ir_program* command = static_cast<rtdx_ir_program*>(rtdx_command_append(command_buffer, rtdx_command_opcode::use_graphics_program)); if (!command) { return; } command->program = program; if (program) { rtdx_resource_retain(RTDX_RESOURCE_BASE(program)); } }
void rtdx_command_buffer_bind_buffer(rtdx_command_buffer* command_buffer, rt_location location, rtdx_buffer* buffer, usize offset, usize size) { rtdx_ir_buffer* command = static_cast<rtdx_ir_buffer*>(rtdx_command_append(command_buffer, rtdx_command_opcode::bind_buffer)); if (!command) { return; } rtdx_buffer_storage* storage = buffer ? buffer->storage : NULL; *command = { location, storage, offset, size }; rtdx_buffer_storage_retain(storage); rtdx_location* private_location = rtdx_location_from_handle(location); if (private_location) { rtdx_resource_retain(RTDX_RESOURCE_BASE(private_location->program)); } }
void rtdx_command_buffer_bind_texture(rtdx_command_buffer* command_buffer, rt_location location, rtdx_texture_view* texture_view) { if (!command_buffer || !command_buffer->recording || !texture_view || !rtdx_texture_view_refresh(rtdx_get_current_context(), texture_view) || !rtdx_texture_view_prepare_sampler(rtdx_get_current_context(), texture_view)) { return; } rtdx_image_base* image = texture_view->image; rtdx_ir_texture* command = static_cast<rtdx_ir_texture*>(rtdx_command_append(command_buffer, rtdx_command_opcode::bind_texture)); if (!command) { return; } *command = { location, texture_view, image, texture_view->d3d_sampler_heap, texture_view->sampler_cpu }; rtdx_resource_retain(RTDX_RESOURCE_BASE(texture_view)); if (image) { rtdx_resource_retain(RTDX_RESOURCE_BASE(image)); } if (command->sampler_heap) { command->sampler_heap->AddRef(); } rtdx_location* private_location = rtdx_location_from_handle(location); if (private_location) { rtdx_resource_retain(RTDX_RESOURCE_BASE(private_location->program)); } }
void rtdx_command_buffer_vertex_buffer(rtdx_command_buffer* command_buffer, rt_location location, rtdx_buffer* buffer, usize offset) { rtdx_ir_vertex_buffer* command = static_cast<rtdx_ir_vertex_buffer*>(rtdx_command_append(command_buffer, rtdx_command_opcode::vertex_buffer)); if (!command) { return; } rtdx_buffer_storage* storage = buffer ? buffer->storage : NULL; *command = { location, storage, offset }; rtdx_buffer_storage_retain(storage); rtdx_location* private_location = rtdx_location_from_handle(location); if (private_location) { rtdx_resource_retain(RTDX_RESOURCE_BASE(private_location->program)); } }
void rtdx_command_buffer_index_buffer(rtdx_command_buffer* command_buffer, rtdx_buffer* buffer, usize offset, rt_index_format format) { rtdx_ir_index_buffer* command = static_cast<rtdx_ir_index_buffer*>(rtdx_command_append(command_buffer, rtdx_command_opcode::index_buffer)); if (!command) { return; } rtdx_buffer_storage* storage = buffer ? buffer->storage : NULL; *command = { storage, offset, format }; rtdx_buffer_storage_retain(storage); }
void rtdx_command_buffer_draw(rtdx_command_buffer* command_buffer, usize count, usize first) { rtdx_ir_draw* command = static_cast<rtdx_ir_draw*>(rtdx_command_append(command_buffer, rtdx_command_opcode::draw)); if (!command) { return; } *command = { count, first }; }
void rtdx_command_buffer_draw_instanced(rtdx_command_buffer* command_buffer, usize count, usize instances, usize first, usize first_instance) { rtdx_ir_draw_instanced* command = static_cast<rtdx_ir_draw_instanced*>(rtdx_command_append(command_buffer, rtdx_command_opcode::draw_instanced)); if (!command) { return; } *command = { count, instances, first, first_instance }; }
void rtdx_command_buffer_draw_indexed(rtdx_command_buffer* command_buffer, usize count, usize first, usize vertex_offset) { rtdx_ir_draw_indexed* command = static_cast<rtdx_ir_draw_indexed*>(rtdx_command_append(command_buffer, rtdx_command_opcode::draw_indexed)); if (!command) { return; } *command = { count, first, vertex_offset }; }
void rtdx_command_buffer_draw_indexed_instanced(rtdx_command_buffer* command_buffer, usize count, usize instances, usize first, usize vertex_offset, usize first_instance) { rtdx_ir_draw_indexed_instanced* command = static_cast<rtdx_ir_draw_indexed_instanced*>(rtdx_command_append(command_buffer, rtdx_command_opcode::draw_indexed_instanced)); if (!command) { return; } *command = { count, instances, first, vertex_offset, first_instance }; }
void rtdx_command_buffer_end(rtdx_command_buffer* command_buffer) { if (!command_buffer || !command_buffer->recording || command_buffer->rendering) { return; } command_buffer->recording = false; command_buffer->executable = true; }

static ID3D12Resource* rtdx_command_buffer_upload(rtdx_context* ctx, const u08* data, usize size) {
	if (!data || !size) { return NULL; }
	D3D12_HEAP_PROPERTIES heap = {};
	heap.Type = D3D12_HEAP_TYPE_UPLOAD;
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ID3D12Resource* upload = NULL;
	HRESULT result = ctx->d3d_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&upload));
	if (FAILED(result)) { rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommittedResource(command upload) failed: 0x%08x", (u32)result); return NULL; }
	void* mapped = NULL;
	result = upload->Map(0, NULL, &mapped);
	if (FAILED(result)) { rtdx_release(&upload); rtdx_throwf(rtdx_error_from_hresult(result), "ID3D12Resource::Map(command upload) failed: 0x%08x", (u32)result); return NULL; }
	memcpy(mapped, data, size);
	upload->Unmap(0, NULL);
	return upload;
}

void rtdx_command_buffer_buffer_data(rtdx_command_buffer* command_buffer, rtdx_buffer* buffer, rt_buffer_range range, const u08* data) {
	if (!command_buffer || !command_buffer->recording || !buffer || !buffer->storage || !data || !range.size || range.offset > buffer->storage->size || range.size > buffer->storage->size - range.offset) { return; }
	rtdx_buffer_write write = rtdx_buffer_write_begin(rtdx_get_current_context(), buffer);
	if (!write.target) { return; }
	ID3D12Resource* upload = rtdx_command_buffer_upload(rtdx_get_current_context(), data, range.size);
	if (!upload) { return; }
	rtdx_ir_buffer_data* command = static_cast<rtdx_ir_buffer_data*>(rtdx_command_append(command_buffer, rtdx_command_opcode::buffer_data));
	if (!command) { rtdx_release(&upload); return; }
	*command = { write.source, write.target, range, upload };
	rtdx_buffer_storage_retain(write.source);
	rtdx_buffer_storage_retain(write.target);
	if (write.target->shadow_data) { memcpy(static_cast<u08*>(write.target->shadow_data) + range.offset, data, range.size); }
	rtdx_buffer_storage_mark_shadow_valid(write.target, range);
	rtdx_buffer_storage_invalidate_texture_source(write.target, range);
}

void rtdx_command_buffer_buffer_copy(rtdx_command_buffer* command_buffer, rtdx_buffer* src, rt_buffer_range src_range, rtdx_buffer* dst, rt_buffer_range dst_range) {
	if (!command_buffer || !command_buffer->recording || !src || !dst || !src->storage || !dst->storage || !src_range.size || src_range.size != dst_range.size || src_range.offset > src->storage->size || src_range.size > src->storage->size - src_range.offset || dst_range.offset > dst->storage->size || dst_range.size > dst->storage->size - dst_range.offset) { return; }
	rtdx_buffer_write write = rtdx_buffer_write_begin(rtdx_get_current_context(), dst);
	if (!write.target) { return; }
	rtdx_ir_buffer_copy* command = static_cast<rtdx_ir_buffer_copy*>(rtdx_command_append(command_buffer, rtdx_command_opcode::buffer_copy));
	if (!command) { return; }
	*command = { src->storage, src_range, write.source, write.target, dst_range };
	rtdx_buffer_storage_retain(command->source);
	rtdx_buffer_storage_retain(command->target_copy_source);
	rtdx_buffer_storage_retain(command->target);
	if (command->target->shadow_data && command->source->shadow_data && rtdx_buffer_storage_shadow_range_valid(command->source, src_range)) { memcpy(static_cast<u08*>(command->target->shadow_data) + dst_range.offset, static_cast<u08*>(command->source->shadow_data) + src_range.offset, src_range.size); rtdx_buffer_storage_mark_shadow_valid(command->target, dst_range); } else { rtdx_buffer_storage_mark_shadow_invalid(command->target, dst_range); }
	rtdx_buffer_storage_invalidate_texture_source(command->target, dst_range);
}

void rtdx_command_buffer_buffer_barrier(rtdx_command_buffer* command_buffer, rtdx_buffer* buffer, rt_buffer_range range, rt_access src, rt_access dst) {
	if (!command_buffer || !command_buffer->recording || !buffer || !buffer->storage || range.offset > buffer->storage->size || range.size > buffer->storage->size - range.offset) { return; }
	rtdx_ir_buffer_barrier* command = static_cast<rtdx_ir_buffer_barrier*>(rtdx_command_append(command_buffer, rtdx_command_opcode::buffer_barrier));
	if (!command) { return; }
	*command = { buffer->storage, src, dst };
	rtdx_buffer_storage_retain(command->storage);
}

static bool rtdx_texture_range_valid(rtdx_image_base* image, rt_texture_range range) {
	if (!image || !image->d3d_resource || !range.mip_count || !range.layer_count || !range.extent.width || !range.extent.height || !range.extent.depth || range.base_mip >= image->mip_count || range.mip_count > image->mip_count - range.base_mip) { return false; }
	const enum rt_texture_aspect_flag available = rtdx_texture_format_is_depth(image->dxgi_format)
		? (image->dxgi_format == DXGI_FORMAT_D24_UNORM_S8_UINT || image->dxgi_format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT
			? (enum rt_texture_aspect_flag)(RT_TEXTURE_ASPECT_DEPTH | RT_TEXTURE_ASPECT_STENCIL)
			: RT_TEXTURE_ASPECT_DEPTH)
		: RT_TEXTURE_ASPECT_COLOR;
	if (!range.aspects || (range.aspects & ~available)) { return false; }
	const usize layers = (image->type == RT_TEXTURE_1D_ARRAY || image->type == RT_TEXTURE_2D_ARRAY) ? image->layer_count : 1;
	if (range.base_layer >= layers || range.layer_count > layers - range.base_layer) { return false; }
	if (image->type == RT_TEXTURE_1D || image->type == RT_TEXTURE_1D_ARRAY) { if (range.offset.height || range.extent.height != 1 || range.offset.depth || range.extent.depth != 1) { return false; } }
	if (image->type != RT_TEXTURE_3D && (range.offset.depth || range.extent.depth != 1)) { return false; }
	for (usize mip = 0; mip < range.mip_count; ++mip) {
		const usize actual_mip = range.base_mip + mip;
		const usize width = image->width >> actual_mip ? image->width >> actual_mip : 1;
		const usize height = image->type == RT_TEXTURE_1D || image->type == RT_TEXTURE_1D_ARRAY ? 1 : (image->height >> actual_mip ? image->height >> actual_mip : 1);
		const usize depth = image->type == RT_TEXTURE_3D ? (image->depth >> actual_mip ? image->depth >> actual_mip : 1) : 1;
		if (range.offset.width > width || range.extent.width > width - range.offset.width || range.offset.height > height || range.extent.height > height - range.offset.height || range.offset.depth > depth || range.extent.depth > depth - range.offset.depth) { return false; }
	}
	return true;
}

static bool rtdx_texture_range_copy_supported(const rtdx_image_base* image, rt_texture_range range) {
	/* D3D12 copy locations address a complete subresource. Depth/stencil formats
	 * therefore transfer the complete packed depth-stencil texel, never one
	 * aspect while silently copying the other. */
	if (!image || (image->dxgi_format != DXGI_FORMAT_D24_UNORM_S8_UINT && image->dxgi_format != DXGI_FORMAT_D32_FLOAT_S8X24_UINT)) {
		return true;
	}
	return range.aspects == (enum rt_texture_aspect_flag)(RT_TEXTURE_ASPECT_DEPTH | RT_TEXTURE_ASPECT_STENCIL);
}

static usize rtdx_command_texture_range_bytes(const rtdx_image_base* image, rt_texture_range range);

void rtdx_command_buffer_buffer_copy_to_texture(rtdx_command_buffer* cb, rtdx_buffer* src, rt_buffer_range src_range, rtdx_texture* dst, rt_texture_range dst_range) {
	rtdx_image_base* original = dst ? dst->active : NULL;
	if (!cb || !cb->recording || !src || !src->storage || !rtdx_texture_range_valid(original, dst_range) || !rtdx_texture_range_copy_supported(original, dst_range) || src_range.offset > src->storage->size || src_range.size > src->storage->size - src_range.offset) { return; }
	rtdx_texture_write write = rtdx_texture_write_begin(rtdx_get_current_context(), dst); if (!write.target) { return; }
	rtdx_image_base* source_texture = NULL; rt_texture_range source_texture_range = {};
	for (const rtdx_buffer_texture_source& source : src->storage->texture_sources) {
		if (source.destination_range.offset == src_range.offset && source.destination_range.size == src_range.size) { source_texture = source.image; source_texture_range = source.source_range; break; }
	}
	rtdx_ir_buffer_copy_to_texture* c = static_cast<rtdx_ir_buffer_copy_to_texture*>(rtdx_command_append(cb, rtdx_command_opcode::buffer_copy_to_texture)); if (!c) { return; }
	*c = { src->storage, src_range, source_texture, source_texture_range, write.source, write.target, dst_range, NULL }; rtdx_buffer_storage_retain(c->source); if (source_texture) { rtdx_resource_retain(RTDX_RESOURCE_BASE(source_texture)); } if (write.source) { rtdx_resource_retain(RTDX_RESOURCE_BASE(write.source)); } rtdx_resource_retain(RTDX_RESOURCE_BASE(write.target));
}

void rtdx_command_buffer_texture_copy(rtdx_command_buffer* cb, rtdx_texture* src, rt_texture_range src_range, rtdx_texture* dst, rt_texture_range dst_range) {
	rtdx_image_base* source = src ? src->active : NULL; rtdx_image_base* original = dst ? dst->active : NULL;
	if (!cb || !cb->recording || !rtdx_texture_range_valid(source, src_range) || !rtdx_texture_range_valid(original, dst_range) || !rtdx_texture_range_copy_supported(source, src_range) || !rtdx_texture_range_copy_supported(original, dst_range) || src_range.mip_count != dst_range.mip_count || src_range.layer_count != dst_range.layer_count || src_range.extent.width != dst_range.extent.width || src_range.extent.height != dst_range.extent.height || src_range.extent.depth != dst_range.extent.depth) { return; }
	rtdx_texture_write write = rtdx_texture_write_begin(rtdx_get_current_context(), dst); if (!write.target) { return; }
	rtdx_ir_texture_copy* c = static_cast<rtdx_ir_texture_copy*>(rtdx_command_append(cb, rtdx_command_opcode::texture_copy)); if (!c) { return; }
	*c = { source, src_range, write.source, write.target, dst_range }; rtdx_resource_retain(RTDX_RESOURCE_BASE(source)); if (write.source) { rtdx_resource_retain(RTDX_RESOURCE_BASE(write.source)); } rtdx_resource_retain(RTDX_RESOURCE_BASE(write.target));
}

void rtdx_command_buffer_texture_data(rtdx_command_buffer* cb, rtdx_texture* texture, rt_texture_range range, const u08* data) {
	rtdx_image_base* original = texture ? texture->active : NULL;
	if (!cb || !cb->recording || !data || !rtdx_texture_range_valid(original, range) || !rtdx_texture_range_copy_supported(original, range)) { return; }
	rtdx_texture_write write = rtdx_texture_write_begin(rtdx_get_current_context(), texture); if (!write.target) { return; }
	rtdx_image_base* target = write.target;
	u32 bpp = target->dxgi_format == DXGI_FORMAT_R8G8B8A8_UNORM || target->dxgi_format == DXGI_FORMAT_B8G8R8A8_UNORM || target->dxgi_format == DXGI_FORMAT_D32_FLOAT || target->dxgi_format == DXGI_FORMAT_D24_UNORM_S8_UINT ? 4 : target->dxgi_format == DXGI_FORMAT_D16_UNORM ? 2 : target->dxgi_format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT ? 8 : 0;
	usize size = range.extent.width * range.extent.height * range.extent.depth * range.mip_count * range.layer_count * bpp;
	if (!bpp || !size) { rtdx_throwf(RT_UNSUPPORTED_FEATURE, "D3D12 recorded texture upload format is unsupported"); return; }
	u08* copy = static_cast<u08*>(malloc(size)); if (!copy) { rtdx_throwf(RT_OUT_OF_HOST_MEMORY, "failed to copy texture upload bytes"); return; } memcpy(copy, data, size);
	rtdx_ir_texture_data* c = static_cast<rtdx_ir_texture_data*>(rtdx_command_append(cb, rtdx_command_opcode::texture_data)); if (!c) { free(copy); return; }
	*c = { write.source, target, range, copy, size, NULL }; if (write.source) { rtdx_resource_retain(RTDX_RESOURCE_BASE(write.source)); } rtdx_resource_retain(RTDX_RESOURCE_BASE(target));
}

void rtdx_command_buffer_texture_copy_to_buffer(rtdx_command_buffer* cb, rtdx_texture* src, rt_texture_range src_range, rtdx_buffer* dst, rt_buffer_range dst_range) {
	rtdx_image_base* source = src ? src->active : NULL;
	const usize packed_size = source ? rtdx_command_texture_range_bytes(source, src_range) : 0;
	if (!cb || !cb->recording || !dst || !dst->storage || !rtdx_texture_range_valid(source, src_range) || !rtdx_texture_range_copy_supported(source, src_range) || !packed_size || dst_range.offset > dst->storage->size || dst_range.size < packed_size || packed_size > dst->storage->size - dst_range.offset) { return; }
	rt_buffer_range packed_dst_range = { packed_size, dst_range.offset };
	rtdx_buffer_write write = rtdx_buffer_write_begin(rtdx_get_current_context(), dst); if (!write.target) { return; }
	rtdx_ir_texture_copy_to_buffer* c = static_cast<rtdx_ir_texture_copy_to_buffer*>(rtdx_command_append(cb, rtdx_command_opcode::texture_copy_to_buffer)); if (!c) { return; }
	*c = { source, src_range, write.source, write.target, packed_dst_range, NULL }; rtdx_resource_retain(RTDX_RESOURCE_BASE(source)); rtdx_buffer_storage_retain(write.source); rtdx_buffer_storage_retain(c->target);
	rtdx_buffer_storage_mark_shadow_invalid(write.target, packed_dst_range);
	rtdx_buffer_storage_set_texture_source(write.target, source, src_range, packed_dst_range);
}

void rtdx_command_buffer_texture_barrier(rtdx_command_buffer* cb, rtdx_texture* texture, rt_texture_range range, rt_access src, rt_access dst) {
	rtdx_image_base* image = texture ? texture->active : NULL;
	if (!cb || !cb->recording || !rtdx_texture_range_valid(image, range)) { return; }
	rtdx_ir_texture_barrier* c = static_cast<rtdx_ir_texture_barrier*>(rtdx_command_append(cb, rtdx_command_opcode::texture_barrier)); if (!c) { return; }
	*c = { image, range, src, dst }; rtdx_resource_retain(RTDX_RESOURCE_BASE(image));
}

void rtdx_lower_begin_rendering(rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_framebuffer* command) {
	state->framebuffer = command->framebuffer;
	state->color_count = command->color_count;
	memcpy(state->color_images, command->color_images, sizeof(state->color_images));
	memcpy(state->color_rtvs, command->color_rtvs, sizeof(state->color_rtvs));
	state->depth_image = command->depth_image;
	state->depth_dsv = command->depth_dsv;
	state->stencil_image = command->stencil_image;
	state->stencil_dsv = command->stencil_dsv;
	if (!state->framebuffer || (!state->color_count && !state->depth_image && !state->stencil_image)) {
		return;
	}
	for (usize i = 0; i < state->color_count; ++i) {
		rtdx_lower_texture_revision_copy(command_list, command->color_copy_sources[i], state->color_images[i]);
		rtdx_command_transition_image(command_list, state->color_images[i], D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
	rtdx_lower_texture_revision_copy(command_list, command->depth_copy_source, state->depth_image);
	if (state->depth_image) { rtdx_command_transition_image(command_list, state->depth_image, D3D12_RESOURCE_STATE_DEPTH_WRITE); }
	rtdx_lower_texture_revision_copy(command_list, command->stencil_copy_source, state->stencil_image);
	if (state->stencil_image && state->stencil_image != state->depth_image) { rtdx_command_transition_image(command_list, state->stencil_image, D3D12_RESOURCE_STATE_DEPTH_WRITE); }
	/* D3D12 exposes one DSV slot. A combined depth-stencil view is intentionally
	 * represented by the same view in both Rutile attachment slots. */
	D3D12_CPU_DESCRIPTOR_HANDLE* dsv = state->depth_image ? &state->depth_dsv : (state->stencil_image ? &state->stencil_dsv : NULL);
	command_list->OMSetRenderTargets((UINT)state->color_count, state->color_count ? state->color_rtvs : NULL, FALSE, dsv);
	rtdx_image_base* extent_image = state->color_count ? state->color_images[0] : (state->depth_image ? state->depth_image : state->stencil_image);
	D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (f32)extent_image->width, (f32)extent_image->height, 0.0f, 1.0f };
	D3D12_RECT scissor = { 0, 0, (LONG)extent_image->width, (LONG)extent_image->height };
	command_list->RSSetViewports(1, &viewport);
	command_list->RSSetScissorRects(1, &scissor);
}

void rtdx_lower_clear_color(rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_clear_color* command) {
	f32 color[] = { command->r, command->g, command->b, command->a };
	if (command->index < state->color_count) { command_list->ClearRenderTargetView(state->color_rtvs[command->index], color, 0, NULL); }
}

void rtdx_lower_clear_depth(rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_clear_depth* command) {
	if (state->depth_image) {
		command_list->ClearDepthStencilView(state->depth_dsv, D3D12_CLEAR_FLAG_DEPTH, command->depth, 0, 0, NULL);
	}
}

void rtdx_lower_clear_stencil(rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_clear_stencil* command) {
	if (state->stencil_image) {
		command_list->ClearDepthStencilView(state->stencil_dsv, D3D12_CLEAR_FLAG_STENCIL, 0.0f, command->stencil, 0, NULL);
	}
}

void rtdx_lower_set_viewport(ID3D12GraphicsCommandList* command_list, const rtdx_ir_viewport* command) {
	D3D12_VIEWPORT viewport = {
		(f32)command->x,
		(f32)command->y,
		(f32)command->width,
		(f32)command->height,
		command->min_depth,
		command->max_depth,
	};
	command_list->RSSetViewports(1, &viewport);
}

void rtdx_lower_set_scissor(ID3D12GraphicsCommandList* command_list, const rtdx_ir_scissor* command) {
	D3D12_RECT scissor = {
		(LONG)command->x,
		(LONG)command->y,
		(LONG)(command->x + command->width),
		(LONG)(command->y + command->height),
	};
	command_list->RSSetScissorRects(1, &scissor);
}

static void rtdx_lower_remember_buffer_binding(rtdx_command_lower_state* state, const rtdx_ir_buffer* command) {
	for (usize index = 0; index < state->pending_buffer_count; ++index) {
		if (state->pending_buffers[index].location == command->location) { state->pending_buffers[index] = *command; return; }
	}
	if (state->pending_buffer_count < sizeof(state->pending_buffers) / sizeof(state->pending_buffers[0])) {
		state->pending_buffers[state->pending_buffer_count++] = *command;
	}
}

static void rtdx_lower_remember_texture_binding(rtdx_command_lower_state* state, const rtdx_ir_texture* command) {
	for (usize index = 0; index < state->pending_texture_count; ++index) {
		if (state->pending_textures[index].location == command->location) { state->pending_textures[index] = *command; return; }
	}
	if (state->pending_texture_count < sizeof(state->pending_textures) / sizeof(state->pending_textures[0])) {
		state->pending_textures[state->pending_texture_count++] = *command;
	}
}

void rtdx_lower_use_graphics_program(rtdx_context* ctx, rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_program* command) {
	state->program = command->program;
	if (!state->program || !state->framebuffer || !state->color_count || !rtdx_graphics_program_prepare(ctx, state->program, state->color_images[0]->dxgi_format, state->depth_image ? state->depth_image->dxgi_format : DXGI_FORMAT_UNKNOWN)) {
		return;
	}

	command_list->SetGraphicsRootSignature(state->program->d3d_root_signature);
	command_list->SetPipelineState(state->program->d3d_pipeline);
	command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	for (usize index = 0; index < state->pending_buffer_count; ++index) {
		rtdx_location* location = rtdx_location_from_handle(state->pending_buffers[index].location);
		if (location && location->program == state->program) {
			rtdx_lower_bind_buffer(ctx, state, command_list, &state->pending_buffers[index]);
		}
	}
	for (usize index = 0; index < state->pending_texture_count; ++index) {
		rtdx_location* location = rtdx_location_from_handle(state->pending_textures[index].location);
		if (location && location->program == state->program) {
			rtdx_lower_bind_texture(ctx, state, command_list, &state->pending_textures[index]);
		}
	}
}

void rtdx_lower_bind_buffer(rtdx_context* ctx, rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_buffer* command) {
	rtdx_location* location = rtdx_location_from_handle(command->location);
	if (!command->storage || !location || command->offset > command->storage->size || !command->size || command->size > command->storage->size - command->offset) {
		return;
	}
	rtdx_lower_remember_buffer_binding(state, command);
	if (location->program != state->program) {
		return;
	}
	if (!state->resource_heap || !command->storage->d3d_resource) {
		return;
	}
	D3D12_CPU_DESCRIPTOR_HANDLE cpu = state->resource_heap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpu = state->resource_heap->GetGPUDescriptorHandleForHeapStart();
	cpu.ptr += state->resource_index * state->resource_step;
	gpu.ptr += state->resource_index++ * state->resource_step;
	if (location->kind == rtdx_location_kind::buffer) {
		/* A D3D12 CBV uses a 256-byte-aligned address and rounded byte size.
		 * The public range is logical, but it must fit in the physical backing
		 * after D3D12's rounding. */
		const u64 cbv_size = (u64)(command->size + 255u) & ~UINT64_C(255);
		const u64 allocation_size = command->storage->d3d_resource->GetDesc().Width;
		if (command->offset % 256u || cbv_size > UINT_MAX || command->offset > allocation_size || cbv_size > allocation_size - command->offset) {
			rtdx_throwf(RT_IMPROPER_USAGE, "uniform buffer range is not representable as a D3D12 constant-buffer view");
			return;
		}
		rtdx_command_transition_buffer(command_list, command->storage, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {
			command->storage->d3d_resource->GetGPUVirtualAddress() + command->offset,
			(UINT)cbv_size,
		};
		ctx->d3d_device->CreateConstantBufferView(&desc, cpu);
		command_list->SetGraphicsRootDescriptorTable(location->root_parameter, gpu);
	} else if (location->kind == rtdx_location_kind::storage_buffer && location->storage_stride && command->offset % location->storage_stride == 0 && command->size % location->storage_stride == 0) {
		rtdx_command_transition_buffer(command_list, command->storage, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.Buffer.FirstElement = command->offset / location->storage_stride;
		desc.Buffer.NumElements = command->size / location->storage_stride;
		desc.Buffer.StructureByteStride = location->storage_stride;
		ctx->d3d_device->CreateShaderResourceView(command->storage->d3d_resource, &desc, cpu);
		command_list->SetGraphicsRootDescriptorTable(location->root_parameter, gpu);
	}
}

void rtdx_lower_bind_texture(rtdx_context* ctx, rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_texture* command) {
	rtdx_location* location = rtdx_location_from_handle(command->location);
	rtdx_texture_view* texture_view = command->texture_view;
	rtdx_image_base* image = command->image;
	if (!location || location->kind != rtdx_location_kind::texture || !texture_view || !image || !image->d3d_resource || !command->sampler_heap) {
		return;
	}
	rtdx_lower_remember_texture_binding(state, command);
	if (location->program != state->program || !state->resource_heap || !state->sampler_heap) {
		return;
	}
	D3D12_CPU_DESCRIPTOR_HANDLE resource_cpu = state->resource_heap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE resource_gpu = state->resource_heap->GetGPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE sampler_cpu = state->sampler_heap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE sampler_gpu = state->sampler_heap->GetGPUDescriptorHandleForHeapStart();
	resource_cpu.ptr += state->resource_index * state->resource_step;
	resource_gpu.ptr += state->resource_index++ * state->resource_step;
	sampler_cpu.ptr += state->sampler_index * state->sampler_step;
	sampler_gpu.ptr += state->sampler_index++ * state->sampler_step;
	rtdx_command_transition_image(command_list, image, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
	srv.Format = image->dxgi_format;
	if (rtdx_texture_format_is_depth(image->dxgi_format)) {
		switch (image->dxgi_format) {
		case DXGI_FORMAT_D16_UNORM: srv.Format = DXGI_FORMAT_R16_UNORM; break;
		case DXGI_FORMAT_D32_FLOAT: srv.Format = DXGI_FORMAT_R32_FLOAT; break;
		case DXGI_FORMAT_D24_UNORM_S8_UINT: srv.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; break;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: srv.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS; break;
		default: break;
		}
	}
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	switch (image->type) {
	case RT_TEXTURE_1D: srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D; srv.Texture1D.MipLevels = (UINT)image->mip_count; break;
	case RT_TEXTURE_1D_ARRAY: srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY; srv.Texture1DArray.ArraySize = (UINT)image->layer_count; srv.Texture1DArray.MipLevels = (UINT)image->mip_count; break;
	case RT_TEXTURE_2D: srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; srv.Texture2D.MipLevels = (UINT)image->mip_count; break;
	case RT_TEXTURE_2D_ARRAY: srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY; srv.Texture2DArray.ArraySize = (UINT)image->layer_count; srv.Texture2DArray.MipLevels = (UINT)image->mip_count; break;
	case RT_TEXTURE_3D: srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D; srv.Texture3D.MipLevels = (UINT)image->mip_count; break;
	default: return;
	}
	ctx->d3d_device->CreateShaderResourceView(image->d3d_resource, &srv, resource_cpu);
	ctx->d3d_device->CopyDescriptorsSimple(1, sampler_cpu, command->sampler_cpu, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	command_list->SetGraphicsRootDescriptorTable(location->root_parameter, resource_gpu);
	command_list->SetGraphicsRootDescriptorTable(location->sampler_root_parameter, sampler_gpu);
}

void rtdx_lower_vertex_buffer(ID3D12GraphicsCommandList* command_list, const rtdx_ir_vertex_buffer* command) {
	rtdx_location* location = rtdx_location_from_handle(command->location);
	if (!command->storage || !location || location->kind != rtdx_location_kind::vertex_input || location->vertex_input >= location->program->vertex_layout.input_count) {
		return;
	}

	rtdx_command_transition_buffer(command_list, command->storage, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	D3D12_VERTEX_BUFFER_VIEW view = command->storage->vertex_view;
	view.BufferLocation += command->offset;
	view.SizeInBytes -= static_cast<UINT>(command->offset);
	view.StrideInBytes = static_cast<UINT>(location->program->vertex_layout.inputs[location->vertex_input].stride);
	command_list->IASetVertexBuffers(static_cast<UINT>(location->vertex_input), 1, &view);
}

void rtdx_lower_index_buffer(ID3D12GraphicsCommandList* command_list, const rtdx_ir_index_buffer* command) {
	if (!command->storage) {
		return;
	}

	rtdx_command_transition_buffer(command_list, command->storage, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	D3D12_INDEX_BUFFER_VIEW view = {
		command->storage->d3d_resource->GetGPUVirtualAddress() + command->offset,
		(UINT)(command->storage->size - command->offset),
		command->format == RT_INDEX_U16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT,
	};
	command_list->IASetIndexBuffer(&view);
}

void rtdx_lower_draw(ID3D12GraphicsCommandList* command_list, const rtdx_ir_draw* command) {
	command_list->DrawInstanced(command->count, 1, command->first, 0);
}

void rtdx_lower_draw_instanced(ID3D12GraphicsCommandList* command_list, const rtdx_ir_draw_instanced* command) {
	command_list->DrawInstanced(command->count, command->instances, command->first, command->first_instance);
}

void rtdx_lower_draw_indexed(ID3D12GraphicsCommandList* command_list, const rtdx_ir_draw_indexed* command) {
	command_list->DrawIndexedInstanced(command->count, 1, command->first, command->vertex_offset, 0);
}

void rtdx_lower_draw_indexed_instanced(ID3D12GraphicsCommandList* command_list, const rtdx_ir_draw_indexed_instanced* command) {
	command_list->DrawIndexedInstanced(command->count, command->instances, command->first, command->vertex_offset, command->first_instance);
}

void rtdx_lower_buffer_data(ID3D12GraphicsCommandList* command_list, const rtdx_ir_buffer_data* command) {
	if (!command || !command->target || !command->target->d3d_resource || !command->upload) { return; }
	if (command->copy_source && command->copy_source != command->target) {
		rtdx_command_transition_buffer(command_list, command->copy_source, D3D12_RESOURCE_STATE_COPY_SOURCE);
		rtdx_command_transition_buffer(command_list, command->target, D3D12_RESOURCE_STATE_COPY_DEST);
		command_list->CopyBufferRegion(command->target->d3d_resource, 0, command->copy_source->d3d_resource, 0, command->target->size);
	}
	rtdx_command_transition_buffer(command_list, command->target, D3D12_RESOURCE_STATE_COPY_DEST);
	command_list->CopyBufferRegion(command->target->d3d_resource, command->range.offset, command->upload, 0, command->range.size);
}

void rtdx_lower_buffer_copy(ID3D12GraphicsCommandList* command_list, const rtdx_ir_buffer_copy* command) {
	if (!command || !command->source || !command->target || !command->source->d3d_resource || !command->target->d3d_resource) { return; }
	if (command->target_copy_source && command->target_copy_source != command->target) {
		rtdx_command_transition_buffer(command_list, command->target_copy_source, D3D12_RESOURCE_STATE_COPY_SOURCE);
		rtdx_command_transition_buffer(command_list, command->target, D3D12_RESOURCE_STATE_COPY_DEST);
		command_list->CopyBufferRegion(command->target->d3d_resource, 0, command->target_copy_source->d3d_resource, 0, command->target->size);
	}
	rtdx_command_transition_buffer(command_list, command->source, D3D12_RESOURCE_STATE_COPY_SOURCE);
	rtdx_command_transition_buffer(command_list, command->target, D3D12_RESOURCE_STATE_COPY_DEST);
	command_list->CopyBufferRegion(command->target->d3d_resource, command->dst_range.offset, command->source->d3d_resource, command->src_range.offset, command->src_range.size);
}

static u32 rtdx_command_texture_bpp(DXGI_FORMAT format) {
	return (format == DXGI_FORMAT_R8G8B8A8_UNORM || format == DXGI_FORMAT_B8G8R8A8_UNORM || format == DXGI_FORMAT_D32_FLOAT || format == DXGI_FORMAT_D24_UNORM_S8_UINT) ? 4 : format == DXGI_FORMAT_D16_UNORM ? 2 : format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT ? 8 : 0;
}

static UINT rtdx_command_texture_subresource(const rtdx_image_base* image, usize mip, usize layer) {
	return (UINT)(layer * image->mip_count + mip);
}

static usize rtdx_command_texture_range_bytes(const rtdx_image_base* image, rt_texture_range range) {
	const u32 bpp = image ? rtdx_command_texture_bpp(image->dxgi_format) : 0;
	return bpp ? range.extent.width * range.extent.height * range.extent.depth * range.mip_count * range.layer_count * bpp : 0;
}

static D3D12_PLACED_SUBRESOURCE_FOOTPRINT rtdx_command_texture_footprint(rtdx_image_base* image, rt_texture_range range, u64 offset) {
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT result = {};
	const u32 bpp = rtdx_command_texture_bpp(image->dxgi_format);
	result.Offset = offset;
	result.Footprint.Format = image->d3d_resource->GetDesc().Format;
	result.Footprint.Width = (UINT)range.extent.width;
	result.Footprint.Height = (UINT)range.extent.height;
	result.Footprint.Depth = (UINT)range.extent.depth;
	result.Footprint.RowPitch = (UINT)((range.extent.width * bpp + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1));
	return result;
}

static void rtdx_lower_texture_revision_copy(ID3D12GraphicsCommandList* command_list, rtdx_image_base* source, rtdx_image_base* target) {
	if (!source || !target || source == target || !source->d3d_resource || !target->d3d_resource) { return; }
	rtdx_command_transition_image(command_list, source, D3D12_RESOURCE_STATE_COPY_SOURCE);
	rtdx_command_transition_image(command_list, target, D3D12_RESOURCE_STATE_COPY_DEST);
	command_list->CopyResource(target->d3d_resource, source->d3d_resource);
}

static D3D12_RESOURCE_STATES rtdx_access_state(rt_access access) {
	if (access.type == RT_ACCESS_WRITE) {
		if (access.stage & RT_STAGE_COLOR_ATTACHMENT) { return D3D12_RESOURCE_STATE_RENDER_TARGET; }
		if (access.stage & RT_STAGE_DEPTH_STENCIL_ATTACHMENT) { return D3D12_RESOURCE_STATE_DEPTH_WRITE; }
		if (access.stage & RT_STAGE_TRANSFER) { return D3D12_RESOURCE_STATE_COPY_DEST; }
		return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}
	if (access.stage & RT_STAGE_TRANSFER) { return D3D12_RESOURCE_STATE_COPY_SOURCE; }
	if (access.stage & RT_STAGE_DEPTH_STENCIL_ATTACHMENT) { return D3D12_RESOURCE_STATE_DEPTH_READ; }
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
	if (access.stage & RT_STAGE_FRAGMENT) { state = (D3D12_RESOURCE_STATES)(state | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE); }
	if (access.stage & (RT_STAGE_VERTEX | RT_STAGE_COMPUTE)) { state = (D3D12_RESOURCE_STATES)(state | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE); }
	return state == D3D12_RESOURCE_STATE_COMMON ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE : state;
}

static void rtdx_lower_uav_barrier(ID3D12GraphicsCommandList* command_list, ID3D12Resource* resource) {
	if (!resource) { return; }
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = resource;
	command_list->ResourceBarrier(1, &barrier);
}

static void rtdx_lower_buffer_barrier(ID3D12GraphicsCommandList* command_list, const rtdx_ir_buffer_barrier* command) {
	if (!command || !command->storage) { return; }
	/* A transition carries layout/access state. A UAV barrier additionally makes
	 * preceding unordered writes visible when the source access says write. */
	if (command->src.type == RT_ACCESS_WRITE && rtdx_access_state(command->src) == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
		rtdx_lower_uav_barrier(command_list, command->storage->d3d_resource);
	}
	rtdx_command_transition_buffer(command_list, command->storage, rtdx_access_state(command->dst));
}

static void rtdx_lower_texture_barrier(ID3D12GraphicsCommandList* command_list, const rtdx_ir_texture_barrier* command) {
	if (!command || !command->image) { return; }
	if (command->src.type == RT_ACCESS_WRITE && rtdx_access_state(command->src) == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
		rtdx_lower_uav_barrier(command_list, command->image->d3d_resource);
	}
	rtdx_command_transition_image_range(command_list, command->image, command->range, rtdx_access_state(command->dst));
}

void rtdx_lower_texture_copy(ID3D12GraphicsCommandList* command_list, const rtdx_ir_texture_copy* command) {
	if (!command || !command->source || !command->target || command->source->dxgi_format != command->target->dxgi_format) { return; }
	rtdx_lower_texture_revision_copy(command_list, command->target_copy_source, command->target);
	rtdx_command_transition_image_range(command_list, command->source, command->src_range, D3D12_RESOURCE_STATE_COPY_SOURCE);
	rtdx_command_transition_image_range(command_list, command->target, command->dst_range, D3D12_RESOURCE_STATE_COPY_DEST);
	const usize mip_count = command->src_range.mip_count < command->dst_range.mip_count ? command->src_range.mip_count : command->dst_range.mip_count;
	const usize layer_count = command->src_range.layer_count < command->dst_range.layer_count ? command->src_range.layer_count : command->dst_range.layer_count;
	for (usize mip = 0; mip < mip_count; ++mip) for (usize layer = 0; layer < layer_count; ++layer) {
		D3D12_TEXTURE_COPY_LOCATION src = {}; src.pResource = command->source->d3d_resource; src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX; src.SubresourceIndex = rtdx_command_texture_subresource(command->source, command->src_range.base_mip + mip, command->src_range.base_layer + layer);
		D3D12_TEXTURE_COPY_LOCATION dst = {}; dst.pResource = command->target->d3d_resource; dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX; dst.SubresourceIndex = rtdx_command_texture_subresource(command->target, command->dst_range.base_mip + mip, command->dst_range.base_layer + layer);
		D3D12_BOX box = { (UINT)command->src_range.offset.width, (UINT)command->src_range.offset.height, (UINT)command->src_range.offset.depth, (UINT)(command->src_range.offset.width + command->src_range.extent.width), (UINT)(command->src_range.offset.height + command->src_range.extent.height), (UINT)(command->src_range.offset.depth + command->src_range.extent.depth) };
		command_list->CopyTextureRegion(&dst, (UINT)command->dst_range.offset.width, (UINT)command->dst_range.offset.height, (UINT)command->dst_range.offset.depth, &src, &box);
	}
}

void rtdx_lower_texture_data(rtdx_context* ctx, ID3D12GraphicsCommandList* command_list, rtdx_ir_texture_data* command) {
	if (!command || !command->target || !command->data || command->upload) { return; }
	rtdx_lower_texture_revision_copy(command_list, command->copy_source, command->target);
	u32 bpp = rtdx_command_texture_bpp(command->target->dxgi_format); if (!bpp) { return; }
	const usize region_bytes = command->range.extent.width * command->range.extent.height * command->range.extent.depth * bpp;
	const usize region_count = command->range.mip_count * command->range.layer_count;
	if (!region_bytes || command->data_size < region_bytes * region_count) { rtdx_throwf(RT_IMPROPER_USAGE, "texture upload bytes are too small"); return; }
	const u64 row_pitch = (command->range.extent.width * bpp + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
	const u64 region_size = row_pitch * command->range.extent.height * command->range.extent.depth;
	const u64 total = region_size * region_count;
	D3D12_HEAP_PROPERTIES heap = {}; heap.Type = D3D12_HEAP_TYPE_UPLOAD;
	D3D12_RESOURCE_DESC buffer = {}; buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; buffer.Width = total; buffer.Height = 1; buffer.DepthOrArraySize = 1; buffer.MipLevels = 1; buffer.SampleDesc.Count = 1; buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	HRESULT result = ctx->d3d_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&command->upload)); if (FAILED(result)) { rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommittedResource(texture command upload) failed: 0x%08x", (u32)result); return; }
	void* mapped = NULL; result = command->upload->Map(0, NULL, &mapped); if (FAILED(result)) { rtdx_throwf(rtdx_error_from_hresult(result), "Map(texture command upload) failed: 0x%08x", (u32)result); return; }
	for (usize region = 0; region < region_count; ++region) for (usize z = 0; z < command->range.extent.depth; ++z) for (usize y = 0; y < command->range.extent.height; ++y) {
		memcpy(static_cast<u08*>(mapped) + region * region_size + (z * command->range.extent.height + y) * row_pitch, command->data + region * region_bytes + (z * command->range.extent.height + y) * command->range.extent.width * bpp, command->range.extent.width * bpp);
	}
	command->upload->Unmap(0, NULL);
	rtdx_command_transition_image_range(command_list, command->target, command->range, D3D12_RESOURCE_STATE_COPY_DEST);
	usize region = 0;
	for (usize mip = 0; mip < command->range.mip_count; ++mip) for (usize layer = 0; layer < command->range.layer_count; ++layer, ++region) {
		D3D12_TEXTURE_COPY_LOCATION src = {}; src.pResource = command->upload; src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; src.PlacedFootprint = rtdx_command_texture_footprint(command->target, command->range, region * region_size);
		D3D12_TEXTURE_COPY_LOCATION dst = {}; dst.pResource = command->target->d3d_resource; dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX; dst.SubresourceIndex = rtdx_command_texture_subresource(command->target, command->range.base_mip + mip, command->range.base_layer + layer);
		command_list->CopyTextureRegion(&dst, (UINT)command->range.offset.width, (UINT)command->range.offset.height, (UINT)command->range.offset.depth, &src, NULL);
	}
}

void rtdx_lower_buffer_copy_to_texture(rtdx_context* ctx, ID3D12GraphicsCommandList* command_list, rtdx_ir_buffer_copy_to_texture* command) {
	if (!command || !command->source || !command->source->d3d_resource || !command->target || command->upload) { return; }
	rtdx_lower_texture_revision_copy(command_list, command->target_copy_source, command->target);
	/* A texture-to-buffer command immediately followed by this command refers to
	 * the just-produced physical bytes. Preserve that chain as a direct image
	 * copy instead of reading the buffer's upload shadow. */
	if (command->source_texture) {
		if (command->source_texture->dxgi_format != command->target->dxgi_format || command->source_texture_range.mip_count != command->dst_range.mip_count || command->source_texture_range.layer_count != command->dst_range.layer_count || command->source_texture_range.extent.width != command->dst_range.extent.width || command->source_texture_range.extent.height != command->dst_range.extent.height || command->source_texture_range.extent.depth != command->dst_range.extent.depth) { rtdx_throwf(RT_IMPROPER_USAGE, "texture-buffer-texture transfer ranges or formats are incompatible"); return; }
		rtdx_command_transition_image_range(command_list, command->source_texture, command->source_texture_range, D3D12_RESOURCE_STATE_COPY_SOURCE);
		rtdx_command_transition_image_range(command_list, command->target, command->dst_range, D3D12_RESOURCE_STATE_COPY_DEST);
		for (usize mip = 0; mip < command->dst_range.mip_count; ++mip) for (usize layer = 0; layer < command->dst_range.layer_count; ++layer) {
			D3D12_TEXTURE_COPY_LOCATION src = {}; src.pResource = command->source_texture->d3d_resource; src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX; src.SubresourceIndex = rtdx_command_texture_subresource(command->source_texture, command->source_texture_range.base_mip + mip, command->source_texture_range.base_layer + layer);
			D3D12_TEXTURE_COPY_LOCATION dst = {}; dst.pResource = command->target->d3d_resource; dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX; dst.SubresourceIndex = rtdx_command_texture_subresource(command->target, command->dst_range.base_mip + mip, command->dst_range.base_layer + layer);
			D3D12_BOX box = { (UINT)command->source_texture_range.offset.width, (UINT)command->source_texture_range.offset.height, (UINT)command->source_texture_range.offset.depth, (UINT)(command->source_texture_range.offset.width + command->source_texture_range.extent.width), (UINT)(command->source_texture_range.offset.height + command->source_texture_range.extent.height), (UINT)(command->source_texture_range.offset.depth + command->source_texture_range.extent.depth) };
			command_list->CopyTextureRegion(&dst, (UINT)command->dst_range.offset.width, (UINT)command->dst_range.offset.height, (UINT)command->dst_range.offset.depth, &src, &box);
		}
		return;
	}
	u32 bpp = rtdx_command_texture_bpp(command->target->dxgi_format); const usize packed_size = rtdx_command_texture_range_bytes(command->target, command->dst_range); if (!bpp || command->src_range.size < packed_size) { rtdx_throwf(RT_IMPROPER_USAGE, "buffer source range is too small for texture copy"); return; }
	const usize region_count = command->dst_range.mip_count * command->dst_range.layer_count;
	const usize region_bytes = packed_size / region_count;
	const u64 row_pitch = (command->dst_range.extent.width * bpp + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
	const u64 region_size = row_pitch * command->dst_range.extent.height * command->dst_range.extent.depth;
	const u64 total = region_size * region_count;
	D3D12_HEAP_PROPERTIES heap = {}; heap.Type = D3D12_HEAP_TYPE_UPLOAD; D3D12_RESOURCE_DESC buffer = {}; buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; buffer.Width = total; buffer.Height = 1; buffer.DepthOrArraySize = 1; buffer.MipLevels = 1; buffer.SampleDesc.Count = 1; buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	HRESULT result = ctx->d3d_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&command->upload)); if (FAILED(result)) { rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommittedResource(buffer texture upload) failed: 0x%08x", (u32)result); return; }
	void* mapped = NULL; result = command->upload->Map(0, NULL, &mapped); if (FAILED(result)) { rtdx_throwf(rtdx_error_from_hresult(result), "Map(buffer texture upload) failed: 0x%08x", (u32)result); return; }
	const u08* source = command->source->shadow_data && rtdx_buffer_storage_shadow_range_valid(command->source, command->src_range) ? static_cast<const u08*>(command->source->shadow_data) + command->src_range.offset : NULL;
	void* mapped_source = NULL;
	if (!source) {
		D3D12_RANGE read_range = { (SIZE_T)command->src_range.offset, (SIZE_T)(command->src_range.offset + command->src_range.size) };
		result = command->source->d3d_resource->Map(0, &read_range, &mapped_source);
		if (FAILED(result)) { command->upload->Unmap(0, NULL); rtdx_throwf(rtdx_error_from_hresult(result), "Map(host buffer texture source) failed: 0x%08x", (u32)result); return; }
		source = static_cast<const u08*>(mapped_source) + command->src_range.offset;
	}
	for (usize region = 0; region < region_count; ++region) for (usize z = 0; z < command->dst_range.extent.depth; ++z) for (usize y = 0; y < command->dst_range.extent.height; ++y) { memcpy(static_cast<u08*>(mapped) + region * region_size + (z * command->dst_range.extent.height + y) * row_pitch, source + region * region_bytes + (z * command->dst_range.extent.height + y) * command->dst_range.extent.width * bpp, command->dst_range.extent.width * bpp); }
	command->upload->Unmap(0, NULL);
	if (mapped_source) { D3D12_RANGE write_range = { 0, 0 }; command->source->d3d_resource->Unmap(0, &write_range); }
	rtdx_command_transition_image_range(command_list, command->target, command->dst_range, D3D12_RESOURCE_STATE_COPY_DEST);
	usize region = 0;
	for (usize mip = 0; mip < command->dst_range.mip_count; ++mip) for (usize layer = 0; layer < command->dst_range.layer_count; ++layer, ++region) { D3D12_TEXTURE_COPY_LOCATION src = {}; src.pResource = command->upload; src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; src.PlacedFootprint = rtdx_command_texture_footprint(command->target, command->dst_range, region * region_size); D3D12_TEXTURE_COPY_LOCATION dst = {}; dst.pResource = command->target->d3d_resource; dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX; dst.SubresourceIndex = rtdx_command_texture_subresource(command->target, command->dst_range.base_mip + mip, command->dst_range.base_layer + layer); command_list->CopyTextureRegion(&dst, (UINT)command->dst_range.offset.width, (UINT)command->dst_range.offset.height, (UINT)command->dst_range.offset.depth, &src, NULL); }
}

void rtdx_lower_texture_copy_to_buffer(rtdx_context* ctx, ID3D12GraphicsCommandList* command_list, rtdx_ir_texture_copy_to_buffer* command) {
	if (!command || !command->source || !command->target || !command->source->d3d_resource || !command->target->d3d_resource) { return; }
	const usize packed_size = rtdx_command_texture_range_bytes(command->source, command->src_range);
	if (!packed_size || command->dst_range.size < packed_size) { rtdx_throwf(RT_IMPROPER_USAGE, "texture copy destination buffer range is too small"); return; }
	if (command->target_copy_source && command->target_copy_source != command->target) {
		rtdx_command_transition_buffer(command_list, command->target_copy_source, D3D12_RESOURCE_STATE_COPY_SOURCE);
		rtdx_command_transition_buffer(command_list, command->target, D3D12_RESOURCE_STATE_COPY_DEST);
		command_list->CopyBufferRegion(command->target->d3d_resource, 0, command->target_copy_source->d3d_resource, 0, command->target->size);
	}
	const D3D12_RESOURCE_DESC texture_desc = command->source->d3d_resource->GetDesc();
	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints;
	usize staging_size = 0;
	for (usize mip = 0; mip < command->src_range.mip_count; ++mip) for (usize layer = 0; layer < command->src_range.layer_count; ++layer) {
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {}; u32 rows = 0; u64 row_size = 0; u64 total_size = 0;
		const UINT subresource = rtdx_command_texture_subresource(command->source, command->src_range.base_mip + mip, command->src_range.base_layer + layer);
		ctx->d3d_device->GetCopyableFootprints(&texture_desc, subresource, 1, staging_size, &footprint, &rows, &row_size, &total_size);
		footprints.push_back(footprint); staging_size = (usize)(footprint.Offset + total_size);
	}
	if (!command->staging) {
		D3D12_HEAP_PROPERTIES heap = {}; heap.Type = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_DESC desc = {}; desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; desc.Width = staging_size; desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1; desc.SampleDesc.Count = 1; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		HRESULT result = ctx->d3d_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&command->staging));
		if (FAILED(result)) { rtdx_throwf(rtdx_error_from_hresult(result), "CreateCommittedResource(texture buffer staging) failed: 0x%08x", (u32)result); return; }
	}
	D3D12_RESOURCE_BARRIER staging_barrier = {}; staging_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; staging_barrier.Transition.pResource = command->staging; staging_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES; staging_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON; staging_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST; command_list->ResourceBarrier(1, &staging_barrier);
	rtdx_command_transition_image_range(command_list, command->source, command->src_range, D3D12_RESOURCE_STATE_COPY_SOURCE);
	rtdx_command_transition_buffer(command_list, command->target, D3D12_RESOURCE_STATE_COPY_DEST);
	usize packed_offset = command->dst_range.offset;
	usize index = 0;
	const u32 bpp = rtdx_command_texture_bpp(command->source->dxgi_format);
	for (usize mip = 0; mip < command->src_range.mip_count; ++mip) for (usize layer = 0; layer < command->src_range.layer_count; ++layer, ++index) {
		const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint = footprints[index];
		const UINT subresource = rtdx_command_texture_subresource(command->source, command->src_range.base_mip + mip, command->src_range.base_layer + layer);
		D3D12_TEXTURE_COPY_LOCATION src = {}; src.pResource = command->source->d3d_resource; src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX; src.SubresourceIndex = subresource;
		D3D12_TEXTURE_COPY_LOCATION dst = {}; dst.pResource = command->staging; dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; dst.PlacedFootprint = footprint;
		D3D12_BOX box = { (UINT)command->src_range.offset.width, (UINT)command->src_range.offset.height, (UINT)command->src_range.offset.depth, (UINT)(command->src_range.offset.width + command->src_range.extent.width), (UINT)(command->src_range.offset.height + command->src_range.extent.height), (UINT)(command->src_range.offset.depth + command->src_range.extent.depth) };
		command_list->CopyTextureRegion(&dst, 0, 0, 0, &src, &box);
	}
	staging_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST; staging_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE; command_list->ResourceBarrier(1, &staging_barrier);
	index = 0;
	for (usize mip = 0; mip < command->src_range.mip_count; ++mip) for (usize layer = 0; layer < command->src_range.layer_count; ++layer, ++index) {
		const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint = footprints[index];
		const usize row_bytes = command->src_range.extent.width * bpp;
		for (usize z = 0; z < command->src_range.extent.depth; ++z) for (usize y = 0; y < command->src_range.extent.height; ++y) {
			command_list->CopyBufferRegion(command->target->d3d_resource, packed_offset, command->staging, footprint.Offset + (z * command->src_range.extent.height + y) * footprint.Footprint.RowPitch, row_bytes);
			packed_offset += row_bytes;
		}
	}
	staging_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE; staging_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON; command_list->ResourceBarrier(1, &staging_barrier);
}

void rtdx_command_buffer_lower(rtdx_context* ctx, rtdx_command_buffer* command_buffer, ID3D12GraphicsCommandList* command_list, ID3D12DescriptorHeap** resource_heap, ID3D12DescriptorHeap** sampler_heap) {
	const usize begin = 0;
	const usize end = command_buffer ? command_buffer->ir_size : 0;
	usize resource_bind_count = 0;
	usize sampler_bind_count = 0;
	usize program_count = 0;
	for (usize offset = begin; offset < end; (void)0) {
		rtdx_command_header* header = reinterpret_cast<rtdx_command_header*>(command_buffer->ir_data + offset);
		if (header->opcode == rtdx_command_opcode::bind_buffer || header->opcode == rtdx_command_opcode::bind_texture) {
			resource_bind_count++;
		}
		if (header->opcode == rtdx_command_opcode::bind_texture) {
			sampler_bind_count++;
		}
		if (header->opcode == rtdx_command_opcode::use_graphics_program) {
			program_count++;
		}
		offset += rtdx_command_record_size(header->opcode);
	}
	/* A root-signature selection invalidates descriptor-table arguments. Keep
	 * enough descriptors for every retained resource binding to be replayed at
	 * each recorded program selection. */
	if (program_count == SIZE_MAX ||
		(resource_bind_count && resource_bind_count > UINT_MAX / (program_count + 1)) ||
		(sampler_bind_count && sampler_bind_count > UINT_MAX / (program_count + 1))) {
		rtdx_throwf(RT_IMPROPER_USAGE, "command buffer requires too many D3D12 descriptors");
		return;
	}
	const usize resource_count = resource_bind_count * (program_count + 1);
	const usize sampler_count = sampler_bind_count * (program_count + 1);

	if (resource_count) {
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = static_cast<UINT>(resource_count);
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		HRESULT result = ctx->d3d_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(resource_heap));
		if (FAILED(result)) {
			rtdx_throwf(rtdx_error_from_hresult(result), "CreateDescriptorHeap(resource) failed: 0x%08x", (u32)result);
			return;
		}
	}
	if (sampler_count) {
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		desc.NumDescriptors = static_cast<UINT>(sampler_count);
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		HRESULT result = ctx->d3d_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(sampler_heap));
		if (FAILED(result)) {
			rtdx_throwf(rtdx_error_from_hresult(result), "CreateDescriptorHeap(sampler) failed: 0x%08x", (u32)result);
			return;
		}
	}
	if (*resource_heap) {
		ID3D12DescriptorHeap* heaps[] = { *resource_heap, *sampler_heap };
		command_list->SetDescriptorHeaps(*sampler_heap ? 2u : 1u, heaps);
	}

	rtdx_command_lower_state state = {};
	state.resource_heap = *resource_heap;
	state.sampler_heap = *sampler_heap;
	state.resource_step = state.resource_heap ? ctx->d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) : 0;
	state.sampler_step = state.sampler_heap ? ctx->d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) : 0;
	for (usize offset = begin; offset < end;) {
		rtdx_command_header* header = reinterpret_cast<rtdx_command_header*>(command_buffer->ir_data + offset);
		void* payload = header + 1;
		switch (header->opcode) {
		case rtdx_command_opcode::begin_rendering:
			rtdx_lower_begin_rendering(&state, command_list, static_cast<rtdx_ir_framebuffer*>(payload));
			break;
		case rtdx_command_opcode::clear_color:
			rtdx_lower_clear_color(&state, command_list, static_cast<rtdx_ir_clear_color*>(payload));
			break;
		case rtdx_command_opcode::clear_depth:
			rtdx_lower_clear_depth(&state, command_list, static_cast<rtdx_ir_clear_depth*>(payload));
			break;
		case rtdx_command_opcode::clear_stencil:
			rtdx_lower_clear_stencil(&state, command_list, static_cast<rtdx_ir_clear_stencil*>(payload));
			break;
		case rtdx_command_opcode::set_viewport:
			rtdx_lower_set_viewport(command_list, static_cast<rtdx_ir_viewport*>(payload));
			break;
		case rtdx_command_opcode::set_scissor:
			rtdx_lower_set_scissor(command_list, static_cast<rtdx_ir_scissor*>(payload));
			break;
		case rtdx_command_opcode::use_graphics_program:
			rtdx_lower_use_graphics_program(ctx, &state, command_list, static_cast<rtdx_ir_program*>(payload));
			break;
		case rtdx_command_opcode::bind_buffer:
			rtdx_lower_bind_buffer(ctx, &state, command_list, static_cast<rtdx_ir_buffer*>(payload));
			break;
		case rtdx_command_opcode::bind_texture:
			rtdx_lower_bind_texture(ctx, &state, command_list, static_cast<rtdx_ir_texture*>(payload));
			break;
		case rtdx_command_opcode::vertex_buffer:
			rtdx_lower_vertex_buffer(command_list, static_cast<rtdx_ir_vertex_buffer*>(payload));
			break;
		case rtdx_command_opcode::index_buffer:
			rtdx_lower_index_buffer(command_list, static_cast<rtdx_ir_index_buffer*>(payload));
			break;
		case rtdx_command_opcode::draw:
			rtdx_lower_draw(command_list, static_cast<rtdx_ir_draw*>(payload));
			break;
		case rtdx_command_opcode::draw_instanced:
			rtdx_lower_draw_instanced(command_list, static_cast<rtdx_ir_draw_instanced*>(payload));
			break;
		case rtdx_command_opcode::draw_indexed:
			rtdx_lower_draw_indexed(command_list, static_cast<rtdx_ir_draw_indexed*>(payload));
			break;
		case rtdx_command_opcode::draw_indexed_instanced:
			rtdx_lower_draw_indexed_instanced(command_list, static_cast<rtdx_ir_draw_indexed_instanced*>(payload));
			break;
		case rtdx_command_opcode::buffer_data:
			rtdx_lower_buffer_data(command_list, static_cast<rtdx_ir_buffer_data*>(payload));
			break;
		case rtdx_command_opcode::buffer_copy:
			rtdx_lower_buffer_copy(command_list, static_cast<rtdx_ir_buffer_copy*>(payload));
			break;
		case rtdx_command_opcode::buffer_copy_to_texture:
			rtdx_lower_buffer_copy_to_texture(ctx, command_list, static_cast<rtdx_ir_buffer_copy_to_texture*>(payload));
			break;
		case rtdx_command_opcode::buffer_barrier:
			rtdx_lower_buffer_barrier(command_list, static_cast<rtdx_ir_buffer_barrier*>(payload));
			break;
		case rtdx_command_opcode::texture_copy:
			rtdx_lower_texture_copy(command_list, static_cast<rtdx_ir_texture_copy*>(payload));
			break;
		case rtdx_command_opcode::texture_data:
			rtdx_lower_texture_data(ctx, command_list, static_cast<rtdx_ir_texture_data*>(payload));
			break;
		case rtdx_command_opcode::texture_copy_to_buffer:
			rtdx_lower_texture_copy_to_buffer(ctx, command_list, static_cast<rtdx_ir_texture_copy_to_buffer*>(payload));
			break;
		case rtdx_command_opcode::texture_barrier:
			rtdx_lower_texture_barrier(command_list, static_cast<rtdx_ir_texture_barrier*>(payload));
			break;
		default:
			break;
		}
		offset += rtdx_command_record_size(header->opcode);
	}
}
