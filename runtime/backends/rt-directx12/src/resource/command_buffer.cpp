#include "command_buffer.hpp"
#include "context.hpp"
#include "error.hpp"
#include "resource/buffer.hpp"
#include "resource/framebuffer.hpp"
#include "resource/graphics_program.hpp"
#include "resource/texture.hpp"

#include <stdlib.h>

/*===============================================================================================*/
/*                                                                                                */
/*===============================================================================================*/

rt_command_buffer rtCommandBufferCreate(void) {
	return rtdx_command_buffer_to_handle(rtdx_command_buffer_create(rtdx_get_current_context()));
}

void rtCommandBufferDestroy(rt_command_buffer command_buffer) {
	rtdx_command_buffer_destroy(rtdx_get_current_context(), rtdx_command_buffer_from_handle(command_buffer));
}

void rtCmdReset(rt_command_buffer command_buffer) {
	rtdx_command_buffer_reset(rtdx_command_buffer_from_handle(command_buffer));
}

void rtCmdBegin(rt_command_buffer command_buffer) {
	rtdx_command_buffer_begin(rtdx_command_buffer_from_handle(command_buffer));
}

void rtCmdWait(rt_command_buffer command_buffer, rt_timepoint timepoint) {
	rtdx_command_buffer_wait(rtdx_command_buffer_from_handle(command_buffer), timepoint);
}

void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer) {
	rtdx_command_buffer_begin_rendering(rtdx_command_buffer_from_handle(command_buffer), rtdx_framebuffer_from_handle(framebuffer));
}

void rtCmdClearColor(rt_command_buffer command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a) {
	rtdx_command_buffer_clear_color(rtdx_command_buffer_from_handle(command_buffer), color_index, r, g, b, a);
}

void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth) {
	rtdx_command_buffer_clear_depth(rtdx_command_buffer_from_handle(command_buffer), depth);
}

void rtCmdClearStencil(rt_command_buffer command_buffer, u32 stencil) {
	rtdx_command_buffer_clear_stencil(rtdx_command_buffer_from_handle(command_buffer), stencil);
}

void rtCmdSetViewport(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth) {
	rtdx_command_buffer_set_viewport(rtdx_command_buffer_from_handle(command_buffer), x, y, width, height, min_depth, max_depth);
}

void rtCmdSetScissor(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height) {
	rtdx_command_buffer_set_scissor(rtdx_command_buffer_from_handle(command_buffer), x, y, width, height);
}

void rtCmdEndRendering(rt_command_buffer command_buffer) {
	rtdx_command_buffer_end_rendering(rtdx_command_buffer_from_handle(command_buffer));
}

void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program) {
	rtdx_command_buffer_use_graphics_program(rtdx_command_buffer_from_handle(command_buffer), rtdx_graphics_program_from_handle(program));
}

void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset, usize size) {
	rtdx_command_buffer_bind_buffer(rtdx_command_buffer_from_handle(command_buffer), location, rtdx_buffer_from_handle(buffer), offset, size);
}

void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view) {
	rtdx_command_buffer_bind_texture(rtdx_command_buffer_from_handle(command_buffer), location, rtdx_texture_view_from_handle(texture_view));
}

void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset) {
	rtdx_command_buffer_vertex_buffer(rtdx_command_buffer_from_handle(command_buffer), location, rtdx_buffer_from_handle(buffer), offset);
}

void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, usize offset, enum rt_index_format format) {
	rtdx_command_buffer_index_buffer(rtdx_command_buffer_from_handle(command_buffer), rtdx_buffer_from_handle(buffer), offset, format);
}

void rtCmdDraw(rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex) {
	rtdx_command_buffer_draw(rtdx_command_buffer_from_handle(command_buffer), vertex_count, first_vertex);
}

void rtCmdDrawInstanced(rt_command_buffer command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
	rtdx_command_buffer_draw_instanced(rtdx_command_buffer_from_handle(command_buffer), vertex_count, instance_count, first_vertex, first_instance);
}

void rtCmdDrawIndexed(rt_command_buffer command_buffer, u32 index_count, u32 first_index, i32 vertex_offset) {
	rtdx_command_buffer_draw_indexed(rtdx_command_buffer_from_handle(command_buffer), index_count, first_index, vertex_offset);
}

void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) {
	rtdx_command_buffer_draw_indexed_instanced(rtdx_command_buffer_from_handle(command_buffer), index_count, instance_count, first_index, vertex_offset, first_instance);
}

void rtCmdEnd(rt_command_buffer command_buffer) {
	rtdx_command_buffer_end(rtdx_command_buffer_from_handle(command_buffer));
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
	if (!image || !image->d3d_resource || image->state == state) {
		return;
	}
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = image->d3d_resource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = image->state;
	barrier.Transition.StateAfter = state;
	command_list->ResourceBarrier(1, &barrier);
	image->state = state;
}

void rtdx_command_buffer_init(rtdx_context* ctx, rtdx_command_buffer* command_buffer) {
	rtdx_init_resource_base(ctx, RTDX_RESOURCE_BASE(command_buffer), rtdx_resource_type::command_buffer);
}

void rtdx_command_buffer_finish(rtdx_context* ctx, rtdx_command_buffer* command_buffer) {
	rtdx_command_buffer_release_resources(command_buffer);
	free(command_buffer->ir_data);
	rtdx_finish_resource_base(ctx, RTDX_RESOURCE_BASE(command_buffer));
}

usize rtdx_command_record_size(rtdx_command_opcode opcode) {
	usize size = sizeof(rtdx_command_header);
	switch (opcode) {
	case rtdx_command_opcode::wait: size += sizeof(rtdx_ir_wait); break;
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
	}
	return (size + alignof(void*) - 1) & ~(alignof(void*) - 1);
}

void* rtdx_command_append(rtdx_command_buffer* command_buffer, rtdx_command_opcode opcode) {
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
			rtdx_framebuffer* framebuffer = static_cast<rtdx_ir_framebuffer*>(payload)->framebuffer;
			if (framebuffer) { rtdx_resource_release(RTDX_RESOURCE_BASE(framebuffer)); }
			break;
		}
		case rtdx_command_opcode::use_graphics_program: {
			rtdx_graphics_program* program = static_cast<rtdx_ir_program*>(payload)->program;
			if (program) { rtdx_resource_release(RTDX_RESOURCE_BASE(program)); }
			break;
		}
		case rtdx_command_opcode::bind_buffer: {
			rtdx_ir_buffer* command = static_cast<rtdx_ir_buffer*>(payload);
			rtdx_buffer* buffer = command->buffer;
			if (buffer) { rtdx_resource_release(RTDX_RESOURCE_BASE(buffer)); }
			rtdx_location* location = rtdx_location_from_handle(command->location);
			if (location) { rtdx_resource_release(RTDX_RESOURCE_BASE(location->program)); }
			break;
		}
		case rtdx_command_opcode::bind_texture: {
			rtdx_ir_texture* command = static_cast<rtdx_ir_texture*>(payload);
			rtdx_texture_view* texture_view = command->texture_view;
			if (texture_view) { rtdx_resource_release(RTDX_RESOURCE_BASE(texture_view)); }
			rtdx_location* location = rtdx_location_from_handle(command->location);
			if (location) { rtdx_resource_release(RTDX_RESOURCE_BASE(location->program)); }
			break;
		}
		case rtdx_command_opcode::vertex_buffer: {
			rtdx_ir_vertex_buffer* command = static_cast<rtdx_ir_vertex_buffer*>(payload);
			rtdx_buffer* buffer = command->buffer;
			if (buffer) { rtdx_resource_release(RTDX_RESOURCE_BASE(buffer)); }
			rtdx_location* location = rtdx_location_from_handle(command->location);
			if (location) { rtdx_resource_release(RTDX_RESOURCE_BASE(location->program)); }
			break;
		}
		case rtdx_command_opcode::index_buffer: {
			rtdx_buffer* buffer = static_cast<rtdx_ir_index_buffer*>(payload)->buffer;
			if (buffer) { rtdx_resource_release(RTDX_RESOURCE_BASE(buffer)); }
			break;
		}
		default:
			break;
		}
		offset += rtdx_command_record_size(header->opcode);
	}
	command_buffer->ir_size = 0;
}

void rtdx_command_buffer_reset(rtdx_command_buffer* command_buffer) { rtdx_command_buffer_release_resources(command_buffer); command_buffer->recording = false; command_buffer->executable = false; }
void rtdx_command_buffer_begin(rtdx_command_buffer* command_buffer) { command_buffer->recording = true; }
void rtdx_command_buffer_wait(rtdx_command_buffer* command_buffer, rt_timepoint timepoint) { rtdx_ir_wait* command = static_cast<rtdx_ir_wait*>(rtdx_command_append(command_buffer, rtdx_command_opcode::wait)); if (!command) { return; } command->timepoint = timepoint; }
void rtdx_command_buffer_begin_rendering(rtdx_command_buffer* command_buffer, rtdx_framebuffer* framebuffer) { rtdx_ir_framebuffer* command = static_cast<rtdx_ir_framebuffer*>(rtdx_command_append(command_buffer, rtdx_command_opcode::begin_rendering)); if (!command) { return; } command->framebuffer = framebuffer; if (framebuffer) { rtdx_resource_retain(RTDX_RESOURCE_BASE(framebuffer)); } }
void rtdx_command_buffer_clear_color(rtdx_command_buffer* command_buffer, u32 index, f32 r, f32 g, f32 b, f32 a) { rtdx_ir_clear_color* command = static_cast<rtdx_ir_clear_color*>(rtdx_command_append(command_buffer, rtdx_command_opcode::clear_color)); if (!command) { return; } *command = { index, r, g, b, a }; }
void rtdx_command_buffer_clear_depth(rtdx_command_buffer* command_buffer, f32 depth) { rtdx_ir_clear_depth* command = static_cast<rtdx_ir_clear_depth*>(rtdx_command_append(command_buffer, rtdx_command_opcode::clear_depth)); if (!command) { return; } command->depth = depth; }
void rtdx_command_buffer_clear_stencil(rtdx_command_buffer* command_buffer, u32 stencil) { rtdx_ir_clear_stencil* command = static_cast<rtdx_ir_clear_stencil*>(rtdx_command_append(command_buffer, rtdx_command_opcode::clear_stencil)); if (!command) { return; } command->stencil = stencil; }
void rtdx_command_buffer_set_viewport(rtdx_command_buffer* command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth) { rtdx_ir_viewport* command = static_cast<rtdx_ir_viewport*>(rtdx_command_append(command_buffer, rtdx_command_opcode::set_viewport)); if (!command) { return; } *command = { x, y, width, height, min_depth, max_depth }; }
void rtdx_command_buffer_set_scissor(rtdx_command_buffer* command_buffer, u32 x, u32 y, u32 width, u32 height) { rtdx_ir_scissor* command = static_cast<rtdx_ir_scissor*>(rtdx_command_append(command_buffer, rtdx_command_opcode::set_scissor)); if (!command) { return; } *command = { x, y, width, height }; }
void rtdx_command_buffer_end_rendering(rtdx_command_buffer* command_buffer) { rtdx_command_append(command_buffer, rtdx_command_opcode::end_rendering); }
void rtdx_command_buffer_use_graphics_program(rtdx_command_buffer* command_buffer, rtdx_graphics_program* program) { rtdx_ir_program* command = static_cast<rtdx_ir_program*>(rtdx_command_append(command_buffer, rtdx_command_opcode::use_graphics_program)); if (!command) { return; } command->program = program; if (program) { rtdx_resource_retain(RTDX_RESOURCE_BASE(program)); } }
void rtdx_command_buffer_bind_buffer(rtdx_command_buffer* command_buffer, rt_location location, rtdx_buffer* buffer, usize offset, usize size) { rtdx_ir_buffer* command = static_cast<rtdx_ir_buffer*>(rtdx_command_append(command_buffer, rtdx_command_opcode::bind_buffer)); if (!command) { return; } *command = { location, buffer, offset, size }; if (buffer) { rtdx_resource_retain(RTDX_RESOURCE_BASE(buffer)); } rtdx_location* private_location = rtdx_location_from_handle(location); if (private_location) { rtdx_resource_retain(RTDX_RESOURCE_BASE(private_location->program)); } }
void rtdx_command_buffer_bind_texture(rtdx_command_buffer* command_buffer, rt_location location, rtdx_texture_view* texture_view) { rtdx_ir_texture* command = static_cast<rtdx_ir_texture*>(rtdx_command_append(command_buffer, rtdx_command_opcode::bind_texture)); if (!command) { return; } *command = { location, texture_view }; if (texture_view) { rtdx_resource_retain(RTDX_RESOURCE_BASE(texture_view)); } rtdx_location* private_location = rtdx_location_from_handle(location); if (private_location) { rtdx_resource_retain(RTDX_RESOURCE_BASE(private_location->program)); } }
void rtdx_command_buffer_vertex_buffer(rtdx_command_buffer* command_buffer, rt_location location, rtdx_buffer* buffer, usize offset) { rtdx_ir_vertex_buffer* command = static_cast<rtdx_ir_vertex_buffer*>(rtdx_command_append(command_buffer, rtdx_command_opcode::vertex_buffer)); if (!command) { return; } *command = { location, buffer, offset }; if (buffer) { rtdx_resource_retain(RTDX_RESOURCE_BASE(buffer)); } rtdx_location* private_location = rtdx_location_from_handle(location); if (private_location) { rtdx_resource_retain(RTDX_RESOURCE_BASE(private_location->program)); } }
void rtdx_command_buffer_index_buffer(rtdx_command_buffer* command_buffer, rtdx_buffer* buffer, usize offset, rt_index_format format) { rtdx_ir_index_buffer* command = static_cast<rtdx_ir_index_buffer*>(rtdx_command_append(command_buffer, rtdx_command_opcode::index_buffer)); if (!command) { return; } *command = { buffer, offset, format }; if (buffer) { rtdx_resource_retain(RTDX_RESOURCE_BASE(buffer)); } }
void rtdx_command_buffer_draw(rtdx_command_buffer* command_buffer, u32 count, u32 first) { rtdx_ir_draw* command = static_cast<rtdx_ir_draw*>(rtdx_command_append(command_buffer, rtdx_command_opcode::draw)); if (!command) { return; } *command = { count, first }; }
void rtdx_command_buffer_draw_instanced(rtdx_command_buffer* command_buffer, u32 count, u32 instances, u32 first, u32 first_instance) { rtdx_ir_draw_instanced* command = static_cast<rtdx_ir_draw_instanced*>(rtdx_command_append(command_buffer, rtdx_command_opcode::draw_instanced)); if (!command) { return; } *command = { count, instances, first, first_instance }; }
void rtdx_command_buffer_draw_indexed(rtdx_command_buffer* command_buffer, u32 count, u32 first, i32 vertex_offset) { rtdx_ir_draw_indexed* command = static_cast<rtdx_ir_draw_indexed*>(rtdx_command_append(command_buffer, rtdx_command_opcode::draw_indexed)); if (!command) { return; } *command = { count, first, vertex_offset }; }
void rtdx_command_buffer_draw_indexed_instanced(rtdx_command_buffer* command_buffer, u32 count, u32 instances, u32 first, i32 vertex_offset, u32 first_instance) { rtdx_ir_draw_indexed_instanced* command = static_cast<rtdx_ir_draw_indexed_instanced*>(rtdx_command_append(command_buffer, rtdx_command_opcode::draw_indexed_instanced)); if (!command) { return; } *command = { count, instances, first, vertex_offset, first_instance }; }
void rtdx_command_buffer_end(rtdx_command_buffer* command_buffer) { command_buffer->recording = false; command_buffer->executable = true; }

void rtdx_lower_begin_rendering(rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_framebuffer* command) {
	state->framebuffer = command->framebuffer;
	if (!state->framebuffer) {
		return;
	}

	rtdx_texture_view* color = state->framebuffer->color_views[0];
	rtdx_command_transition_image(command_list, color->image, D3D12_RESOURCE_STATE_RENDER_TARGET);
	D3D12_CPU_DESCRIPTOR_HANDLE* depth = state->framebuffer->depth_view ? &state->framebuffer->depth_view->dsv : NULL;
	if (state->framebuffer->depth_view) {
		rtdx_command_transition_image(command_list, state->framebuffer->depth_view->image, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}
	command_list->OMSetRenderTargets(1, &color->rtv, FALSE, depth);
}

void rtdx_lower_clear_color(rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_clear_color* command) {
	f32 color[] = { command->r, command->g, command->b, command->a };
	command_list->ClearRenderTargetView(state->framebuffer->color_views[command->index]->rtv, color, 0, NULL);
}

void rtdx_lower_clear_depth(rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_clear_depth* command) {
	if (state->framebuffer->depth_view) {
		command_list->ClearDepthStencilView(state->framebuffer->depth_view->dsv, D3D12_CLEAR_FLAG_DEPTH, command->depth, 0, 0, NULL);
	}
}

void rtdx_lower_clear_stencil(rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_clear_stencil* command) {
	if (state->framebuffer->depth_view) {
		command_list->ClearDepthStencilView(state->framebuffer->depth_view->dsv, D3D12_CLEAR_FLAG_STENCIL, 0.0f, command->stencil, 0, NULL);
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

void rtdx_lower_use_graphics_program(rtdx_context* ctx, rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_program* command) {
	state->program = command->program;
	if (!state->program || !state->framebuffer || !rtdx_graphics_program_prepare(ctx, state->program, state->framebuffer->color_views[0]->image->dxgi_format, state->framebuffer->depth_view ? state->framebuffer->depth_view->image->dxgi_format : DXGI_FORMAT_UNKNOWN)) {
		return;
	}

	command_list->SetGraphicsRootSignature(state->program->d3d_root_signature);
	command_list->SetPipelineState(state->program->d3d_pipeline);
	command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void rtdx_lower_bind_buffer(rtdx_context* ctx, rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_buffer* command) {
	rtdx_location* location = rtdx_location_from_handle(command->location);
	D3D12_CPU_DESCRIPTOR_HANDLE cpu = state->resource_heap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpu = state->resource_heap->GetGPUDescriptorHandleForHeapStart();
	cpu.ptr += state->resource_index * state->resource_step;
	gpu.ptr += state->resource_index++ * state->resource_step;
	if (!command->buffer || !command->buffer->storage || !location || command->offset > command->buffer->storage->size || !command->size || command->size > command->buffer->storage->size - command->offset) {
		return;
	}

	if (location->kind == rtdx_location_kind::buffer) {
		rtdx_command_transition_buffer(command_list, command->buffer->storage, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {
			command->buffer->storage->d3d_resource->GetGPUVirtualAddress() + command->offset,
			(UINT)((command->size + 255) & ~255),
		};
		ctx->d3d_device->CreateConstantBufferView(&desc, cpu);
		command_list->SetGraphicsRootDescriptorTable(location->root_parameter, gpu);
	} else if (location->kind == rtdx_location_kind::storage_buffer && location->storage_stride && command->offset % location->storage_stride == 0 && command->size % location->storage_stride == 0) {
		rtdx_command_transition_buffer(command_list, command->buffer->storage, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.Buffer.FirstElement = command->offset / location->storage_stride;
		desc.Buffer.NumElements = command->size / location->storage_stride;
		desc.Buffer.StructureByteStride = location->storage_stride;
		ctx->d3d_device->CreateShaderResourceView(command->buffer->storage->d3d_resource, &desc, cpu);
		command_list->SetGraphicsRootDescriptorTable(location->root_parameter, gpu);
	}
}

void rtdx_lower_bind_texture(rtdx_context* ctx, rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_texture* command) {
	rtdx_location* location = rtdx_location_from_handle(command->location);
	rtdx_texture_view* texture_view = command->texture_view;
	D3D12_CPU_DESCRIPTOR_HANDLE resource_cpu = state->resource_heap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE resource_gpu = state->resource_heap->GetGPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE sampler_cpu = state->sampler_heap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE sampler_gpu = state->sampler_heap->GetGPUDescriptorHandleForHeapStart();
	resource_cpu.ptr += state->resource_index * state->resource_step;
	resource_gpu.ptr += state->resource_index++ * state->resource_step;
	sampler_cpu.ptr += state->sampler_index * state->sampler_step;
	sampler_gpu.ptr += state->sampler_index++ * state->sampler_step;
	if (!location || location->kind != rtdx_location_kind::texture || !texture_view || !texture_view->d3d_srv_heap || !rtdx_texture_view_prepare_sampler(ctx, texture_view)) {
		return;
	}

	rtdx_command_transition_image(command_list, texture_view->image, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	ctx->d3d_device->CopyDescriptorsSimple(1, resource_cpu, texture_view->srv_cpu, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	ctx->d3d_device->CopyDescriptorsSimple(1, sampler_cpu, texture_view->sampler_cpu, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	command_list->SetGraphicsRootDescriptorTable(location->root_parameter, resource_gpu);
	command_list->SetGraphicsRootDescriptorTable(location->sampler_root_parameter, sampler_gpu);
}

void rtdx_lower_vertex_buffer(ID3D12GraphicsCommandList* command_list, const rtdx_ir_vertex_buffer* command) {
	rtdx_location* location = rtdx_location_from_handle(command->location);
	if (!command->buffer || !command->buffer->storage || !location || location->kind != rtdx_location_kind::vertex_stream || location->vertex_stream >= location->program->vertex_layout.stream_count) {
		return;
	}

	rtdx_command_transition_buffer(command_list, command->buffer->storage, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	D3D12_VERTEX_BUFFER_VIEW view = command->buffer->storage->vertex_view;
	view.BufferLocation += command->offset;
	view.SizeInBytes -= static_cast<UINT>(command->offset);
	view.StrideInBytes = static_cast<UINT>(location->program->vertex_layout.streams[location->vertex_stream].stride);
	command_list->IASetVertexBuffers(static_cast<UINT>(location->vertex_stream), 1, &view);
}

void rtdx_lower_index_buffer(ID3D12GraphicsCommandList* command_list, const rtdx_ir_index_buffer* command) {
	if (!command->buffer || !command->buffer->storage) {
		return;
	}

	rtdx_command_transition_buffer(command_list, command->buffer->storage, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	D3D12_INDEX_BUFFER_VIEW view = {
		command->buffer->storage->d3d_resource->GetGPUVirtualAddress() + command->offset,
		(UINT)(command->buffer->storage->size - command->offset),
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

void rtdx_command_buffer_lower_segment(rtdx_context* ctx, rtdx_command_buffer* command_buffer, usize begin, usize end, ID3D12GraphicsCommandList* command_list, ID3D12DescriptorHeap** resource_heap, ID3D12DescriptorHeap** sampler_heap) {
	usize resource_count = 0;
	usize sampler_count = 0;
	for (usize offset = begin; offset < end; (void)0) {
		rtdx_command_header* header = reinterpret_cast<rtdx_command_header*>(command_buffer->ir_data + offset);
		if (header->opcode == rtdx_command_opcode::bind_buffer || header->opcode == rtdx_command_opcode::bind_texture) {
			resource_count++;
		}
		if (header->opcode == rtdx_command_opcode::bind_texture) {
			sampler_count++;
		}
		offset += rtdx_command_record_size(header->opcode);
	}

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
		default:
			break;
		}
		offset += rtdx_command_record_size(header->opcode);
	}
}
