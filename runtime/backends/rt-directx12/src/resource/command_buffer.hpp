#pragma once

#include "config.hpp"
#include "resource.hpp"

#include <d3d12.h>

struct rtdx_buffer;
struct rtdx_buffer_storage;
struct rtdx_framebuffer;
struct rtdx_graphics_program;
struct rtdx_image_base;
struct rtdx_texture_view;

void rtdx_command_transition_buffer(ID3D12GraphicsCommandList* command_list, rtdx_buffer_storage* storage, D3D12_RESOURCE_STATES state);
void rtdx_command_transition_image(ID3D12GraphicsCommandList* command_list, rtdx_image_base* image, D3D12_RESOURCE_STATES state);

RTDX_API rt_command_buffer rtCommandBufferCreate(void);
RTDX_API void rtCommandBufferDestroy(rt_command_buffer command_buffer);
RTDX_API void rtCmdReset(rt_command_buffer command_buffer);
RTDX_API void rtCmdBegin(rt_command_buffer command_buffer);
RTDX_API void rtCmdWait(rt_command_buffer command_buffer, rt_timepoint timepoint);
RTDX_API void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer);
RTDX_API void rtCmdClearColor(rt_command_buffer command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a);
RTDX_API void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth);
RTDX_API void rtCmdClearStencil(rt_command_buffer command_buffer, u32 stencil);
RTDX_API void rtCmdSetViewport(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth);
RTDX_API void rtCmdSetScissor(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height);
RTDX_API void rtCmdEndRendering(rt_command_buffer command_buffer);
RTDX_API void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program);
RTDX_API void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset, usize size);
RTDX_API void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view);
RTDX_API void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset);
RTDX_API void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, usize offset, rt_index_format format);
RTDX_API void rtCmdDraw(rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex);
RTDX_API void rtCmdDrawInstanced(rt_command_buffer command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
RTDX_API void rtCmdDrawIndexed(rt_command_buffer command_buffer, u32 index_count, u32 first_index, i32 vertex_offset);
RTDX_API void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance);
RTDX_API void rtCmdEnd(rt_command_buffer command_buffer);

enum class rtdx_command_opcode : u08 {
	wait,
	begin_rendering,
	clear_color,
	clear_depth,
	clear_stencil,
	set_viewport,
	set_scissor,
	end_rendering,
	use_graphics_program,
	bind_buffer,
	bind_texture,
	vertex_buffer,
	index_buffer,
	draw,
	draw_instanced,
	draw_indexed,
	draw_indexed_instanced,
};

struct rtdx_command_header { void* alignment; rtdx_command_opcode opcode; };
struct rtdx_ir_wait { rt_timepoint timepoint; };
struct rtdx_ir_framebuffer { rtdx_framebuffer* framebuffer; };
struct rtdx_ir_clear_color { u32 index; f32 r; f32 g; f32 b; f32 a; };
struct rtdx_ir_clear_depth { f32 depth; };
struct rtdx_ir_clear_stencil { u32 stencil; };
struct rtdx_ir_viewport { u32 x; u32 y; u32 width; u32 height; f32 min_depth; f32 max_depth; };
struct rtdx_ir_scissor { u32 x; u32 y; u32 width; u32 height; };
struct rtdx_ir_program { rtdx_graphics_program* program; };
struct rtdx_ir_buffer { rt_location location; rtdx_buffer* buffer; usize offset; usize size; };
struct rtdx_ir_texture { rt_location location; rtdx_texture_view* texture_view; };
struct rtdx_ir_vertex_buffer { rt_location location; rtdx_buffer* buffer; usize offset; };
struct rtdx_ir_index_buffer { rtdx_buffer* buffer; usize offset; rt_index_format format; };
struct rtdx_ir_draw { u32 count; u32 first; };
struct rtdx_ir_draw_instanced { u32 count; u32 instances; u32 first; u32 first_instance; };
struct rtdx_ir_draw_indexed { u32 count; u32 first; i32 vertex_offset; };
struct rtdx_ir_draw_indexed_instanced { u32 count; u32 instances; u32 first; i32 vertex_offset; u32 first_instance; };

struct rtdx_command_buffer {
	rtdx_resource_base base;
	u08* ir_data;
	usize ir_size;
	usize ir_capacity;
	bool recording;
	bool executable;
};
RTDX_DECLARE_NEW_RESOURCE(command_buffer)

struct rtdx_command_lower_state {
	rtdx_framebuffer* framebuffer;
	rtdx_graphics_program* program;
	ID3D12DescriptorHeap* resource_heap;
	ID3D12DescriptorHeap* sampler_heap;
	UINT resource_step;
	UINT sampler_step;
	UINT resource_index;
	UINT sampler_index;
};

void rtdx_command_buffer_reset(rtdx_command_buffer* command_buffer);
void rtdx_command_buffer_release_resources(rtdx_command_buffer* command_buffer);
void rtdx_command_buffer_begin(rtdx_command_buffer* command_buffer);
void rtdx_command_buffer_wait(rtdx_command_buffer* command_buffer, rt_timepoint timepoint);
void rtdx_command_buffer_begin_rendering(rtdx_command_buffer* command_buffer, rtdx_framebuffer* framebuffer);
void rtdx_command_buffer_clear_color(rtdx_command_buffer* command_buffer, u32 index, f32 r, f32 g, f32 b, f32 a);
void rtdx_command_buffer_clear_depth(rtdx_command_buffer* command_buffer, f32 depth);
void rtdx_command_buffer_clear_stencil(rtdx_command_buffer* command_buffer, u32 stencil);
void rtdx_command_buffer_set_viewport(rtdx_command_buffer* command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth);
void rtdx_command_buffer_set_scissor(rtdx_command_buffer* command_buffer, u32 x, u32 y, u32 width, u32 height);
void rtdx_command_buffer_end_rendering(rtdx_command_buffer* command_buffer);
void rtdx_command_buffer_use_graphics_program(rtdx_command_buffer* command_buffer, rtdx_graphics_program* program);
void rtdx_command_buffer_bind_buffer(rtdx_command_buffer* command_buffer, rt_location location, rtdx_buffer* buffer, usize offset, usize size);
void rtdx_command_buffer_bind_texture(rtdx_command_buffer* command_buffer, rt_location location, rtdx_texture_view* texture_view);
void rtdx_command_buffer_vertex_buffer(rtdx_command_buffer* command_buffer, rt_location location, rtdx_buffer* buffer, usize offset);
void rtdx_command_buffer_index_buffer(rtdx_command_buffer* command_buffer, rtdx_buffer* buffer, usize offset, rt_index_format format);
void rtdx_command_buffer_draw(rtdx_command_buffer* command_buffer, u32 count, u32 first);
void rtdx_command_buffer_draw_instanced(rtdx_command_buffer* command_buffer, u32 count, u32 instances, u32 first, u32 first_instance);
void rtdx_command_buffer_draw_indexed(rtdx_command_buffer* command_buffer, u32 count, u32 first, i32 vertex_offset);
void rtdx_command_buffer_draw_indexed_instanced(rtdx_command_buffer* command_buffer, u32 count, u32 instances, u32 first, i32 vertex_offset, u32 first_instance);
void rtdx_command_buffer_end(rtdx_command_buffer* command_buffer);
void rtdx_lower_begin_rendering(rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_framebuffer* command);
void rtdx_lower_clear_color(rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_clear_color* command);
void rtdx_lower_clear_depth(rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_clear_depth* command);
void rtdx_lower_clear_stencil(rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_clear_stencil* command);
void rtdx_lower_set_viewport(ID3D12GraphicsCommandList* command_list, const rtdx_ir_viewport* command);
void rtdx_lower_set_scissor(ID3D12GraphicsCommandList* command_list, const rtdx_ir_scissor* command);
void rtdx_lower_use_graphics_program(rtdx_context* ctx, rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_program* command);
void rtdx_lower_bind_buffer(rtdx_context* ctx, rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_buffer* command);
void rtdx_lower_bind_texture(rtdx_context* ctx, rtdx_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtdx_ir_texture* command);
void rtdx_lower_vertex_buffer(ID3D12GraphicsCommandList* command_list, const rtdx_ir_vertex_buffer* command);
void rtdx_lower_index_buffer(ID3D12GraphicsCommandList* command_list, const rtdx_ir_index_buffer* command);
void rtdx_lower_draw(ID3D12GraphicsCommandList* command_list, const rtdx_ir_draw* command);
void rtdx_lower_draw_instanced(ID3D12GraphicsCommandList* command_list, const rtdx_ir_draw_instanced* command);
void rtdx_lower_draw_indexed(ID3D12GraphicsCommandList* command_list, const rtdx_ir_draw_indexed* command);
void rtdx_lower_draw_indexed_instanced(ID3D12GraphicsCommandList* command_list, const rtdx_ir_draw_indexed_instanced* command);
void rtdx_command_buffer_lower_segment(rtdx_context* ctx, rtdx_command_buffer* command_buffer, usize begin, usize end, ID3D12GraphicsCommandList* command_list, ID3D12DescriptorHeap** resource_heap, ID3D12DescriptorHeap** sampler_heap);
usize rtdx_command_record_size(rtdx_command_opcode opcode);
