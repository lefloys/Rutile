#pragma once

#include "config.hpp"
#include "resource.hpp"

#include <d3d12.h>

struct rtdx_buffer;
struct rtdx_buffer_storage;
struct rtdx_framebuffer;
struct rtdx_graphics_program;
struct rtdx_image_base;
struct rtdx_texture;
struct rtdx_texture_view;

void rtdx_command_transition_buffer(ID3D12GraphicsCommandList* command_list, rtdx_buffer_storage* storage, D3D12_RESOURCE_STATES state);
void rtdx_command_transition_image(ID3D12GraphicsCommandList* command_list, rtdx_image_base* image, D3D12_RESOURCE_STATES state);
void rtdx_command_transition_image_range(ID3D12GraphicsCommandList* command_list, rtdx_image_base* image, rt_texture_range range, D3D12_RESOURCE_STATES state);

RTDX_API rt_command_buffer rtCommandBufferCreate(void);
RTDX_API void rtCommandBufferDestroy(rt_command_buffer command_buffer);
RTDX_API void rtCommandBufferReset(rt_command_buffer command_buffer);
RTDX_API void rtCommandBufferBegin(rt_command_buffer command_buffer);
RTDX_API void rtCommandBufferContinue(rt_command_buffer command_buffer);
RTDX_API void rtCommandBufferContinueRendering(rt_command_buffer command_buffer);
RTDX_API void rtCommandBufferEnd(rt_command_buffer command_buffer);
RTDX_API void rtCmdExecute(rt_command_buffer command_buffer, rt_command_buffer secondary);
RTDX_API void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer);
RTDX_API void rtCmdClearColor(rt_command_buffer command_buffer, rt_location location, f32 r, f32 g, f32 b, f32 a);
RTDX_API void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth);
RTDX_API void rtCmdClearStencil(rt_command_buffer command_buffer, usize stencil);
RTDX_API void rtCmdClear(rt_command_buffer command_buffer, enum rt_clear_flag attachments);
RTDX_API void rtCmdSetViewport(rt_command_buffer command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth);
RTDX_API void rtCmdSetScissor(rt_command_buffer command_buffer, usize x, usize y, usize width, usize height);
RTDX_API void rtCmdEndRendering(rt_command_buffer command_buffer);
RTDX_API void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program);
RTDX_API void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range);
RTDX_API void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view);
RTDX_API void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range);
RTDX_API void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, enum rt_index_format format);
RTDX_API void rtCmdDraw(rt_command_buffer command_buffer, usize vertex_count, usize first_vertex);
RTDX_API void rtCmdDrawInstanced(rt_command_buffer command_buffer, usize vertex_count, usize instance_count, usize first_vertex, usize first_instance);
RTDX_API void rtCmdDrawIndexed(rt_command_buffer command_buffer, usize index_count, usize first_index, usize vertex_offset);
RTDX_API void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, usize index_count, usize instance_count, usize first_index, usize vertex_offset, usize first_instance);
RTDX_API void rtCmdBufferData(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, const u08* data);
RTDX_API void rtCmdBufferCopy(rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_buffer dst, rt_buffer_range dst_range);
RTDX_API void rtCmdBufferCopyToTexture(rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_texture dst, rt_texture_range dst_range);
RTDX_API void rtCmdBufferBarrier(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, rt_access src, rt_access dst);

enum class rtdx_command_opcode : u08 {
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
	buffer_data,
	buffer_copy,
	buffer_copy_to_texture,
	buffer_barrier,
	texture_copy,
	texture_data,
	texture_copy_to_buffer,
	texture_barrier,
};

struct rtdx_command_header { void* alignment; rtdx_command_opcode opcode; };
struct rtdx_ir_framebuffer {
	rtdx_framebuffer* framebuffer;
	usize color_count;
	rtdx_image_base* color_copy_sources[8];
	rtdx_image_base* color_images[8];
	D3D12_CPU_DESCRIPTOR_HANDLE color_rtvs[8];
	ID3D12DescriptorHeap* color_rtv_heaps[8];
	rtdx_image_base* depth_copy_source;
	rtdx_image_base* depth_image;
	D3D12_CPU_DESCRIPTOR_HANDLE depth_dsv;
	ID3D12DescriptorHeap* depth_dsv_heap;
	rtdx_image_base* stencil_copy_source;
	rtdx_image_base* stencil_image;
	D3D12_CPU_DESCRIPTOR_HANDLE stencil_dsv;
	ID3D12DescriptorHeap* stencil_dsv_heap;
};
struct rtdx_ir_clear_color { u32 index; f32 r; f32 g; f32 b; f32 a; };
struct rtdx_ir_clear_depth { f32 depth; };
struct rtdx_ir_clear_stencil { u32 stencil; };
struct rtdx_ir_viewport { usize x; usize y; usize width; usize height; f32 min_depth; f32 max_depth; };
struct rtdx_ir_scissor { usize x; usize y; usize width; usize height; };
struct rtdx_ir_program { rtdx_graphics_program* program; };
struct rtdx_ir_buffer { rt_location location; rtdx_buffer_storage* storage; usize offset; usize size; };
struct rtdx_ir_texture { rt_location location; rtdx_texture_view* texture_view; rtdx_image_base* image; ID3D12DescriptorHeap* sampler_heap; D3D12_CPU_DESCRIPTOR_HANDLE sampler_cpu; };
struct rtdx_ir_vertex_buffer { rt_location location; rtdx_buffer_storage* storage; usize offset; };
struct rtdx_ir_index_buffer { rtdx_buffer_storage* storage; usize offset; rt_index_format format; };
struct rtdx_ir_draw { usize count; usize first; };
struct rtdx_ir_draw_instanced { usize count; usize instances; usize first; usize first_instance; };
struct rtdx_ir_draw_indexed { usize count; usize first; usize vertex_offset; };
struct rtdx_ir_draw_indexed_instanced { usize count; usize instances; usize first; usize vertex_offset; usize first_instance; };
struct rtdx_ir_buffer_data { rtdx_buffer_storage* copy_source; rtdx_buffer_storage* target; rt_buffer_range range; ID3D12Resource* upload; };
struct rtdx_ir_buffer_copy { rtdx_buffer_storage* source; rt_buffer_range src_range; rtdx_buffer_storage* target_copy_source; rtdx_buffer_storage* target; rt_buffer_range dst_range; };
struct rtdx_ir_buffer_barrier { rtdx_buffer_storage* storage; rt_access src; rt_access dst; };
struct rtdx_ir_buffer_copy_to_texture { rtdx_buffer_storage* source; rt_buffer_range src_range; rtdx_image_base* source_texture; rt_texture_range source_texture_range; rtdx_image_base* target_copy_source; rtdx_image_base* target; rt_texture_range dst_range; ID3D12Resource* upload; };
struct rtdx_ir_texture_copy { rtdx_image_base* source; rt_texture_range src_range; rtdx_image_base* target_copy_source; rtdx_image_base* target; rt_texture_range dst_range; };
struct rtdx_ir_texture_data { rtdx_image_base* copy_source; rtdx_image_base* target; rt_texture_range range; u08* data; usize data_size; ID3D12Resource* upload; };
struct rtdx_ir_texture_copy_to_buffer { rtdx_image_base* source; rt_texture_range src_range; rtdx_buffer_storage* target_copy_source; rtdx_buffer_storage* target; rt_buffer_range dst_range; ID3D12Resource* staging; };
struct rtdx_ir_texture_barrier { rtdx_image_base* image; rt_texture_range range; rt_access src; rt_access dst; };

struct rtdx_command_buffer {
	rtdx_resource_base base;
	u08* ir_data;
	usize ir_size;
	usize ir_capacity;
	bool recording;
	bool executable;
	bool continuation;
	bool rendering_continuation;
	bool rendering;
	u32 in_flight;
	rtdx_framebuffer* active_framebuffer;
	f32 clear_colors[8][4];
	f32 clear_depth_value;
	u32 clear_stencil_value;
};
RTDX_DECLARE_NEW_RESOURCE(command_buffer)

struct rtdx_command_lower_state {
	rtdx_framebuffer* framebuffer;
	usize color_count;
	rtdx_image_base* color_images[8];
	D3D12_CPU_DESCRIPTOR_HANDLE color_rtvs[8];
	rtdx_image_base* depth_image;
	D3D12_CPU_DESCRIPTOR_HANDLE depth_dsv;
	rtdx_image_base* stencil_image;
	D3D12_CPU_DESCRIPTOR_HANDLE stencil_dsv;
	rtdx_graphics_program* program;
	ID3D12DescriptorHeap* resource_heap;
	ID3D12DescriptorHeap* sampler_heap;
	UINT resource_step;
	UINT sampler_step;
	UINT resource_index;
	UINT sampler_index;
	rtdx_ir_buffer pending_buffers[32];
	usize pending_buffer_count;
	rtdx_ir_texture pending_textures[32];
	usize pending_texture_count;
};

void rtdx_command_buffer_reset(rtdx_command_buffer* command_buffer);
void rtdx_command_buffer_release_resources(rtdx_command_buffer* command_buffer);
rtdx_command_buffer* rtdx_command_buffer_snapshot_create(const rtdx_command_buffer* command_buffer);
void rtdx_command_buffer_snapshot_destroy(rtdx_command_buffer* snapshot);
void rtdx_command_buffer_begin(rtdx_command_buffer* command_buffer);
void rtdx_command_buffer_continue(rtdx_command_buffer* command_buffer, bool rendering);
void rtdx_command_buffer_execute(rtdx_command_buffer* command_buffer, rtdx_command_buffer* secondary);
void rtdx_command_buffer_begin_rendering(rtdx_command_buffer* command_buffer, rtdx_framebuffer* framebuffer);
void rtdx_command_buffer_clear_color(rtdx_command_buffer* command_buffer, u32 index, f32 r, f32 g, f32 b, f32 a);
void rtdx_command_buffer_clear_depth(rtdx_command_buffer* command_buffer, f32 depth);
void rtdx_command_buffer_clear_stencil(rtdx_command_buffer* command_buffer, u32 stencil);
void rtdx_command_buffer_clear(rtdx_command_buffer* command_buffer, rt_clear_flag attachments);
void rtdx_command_buffer_set_viewport(rtdx_command_buffer* command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth);
void rtdx_command_buffer_set_scissor(rtdx_command_buffer* command_buffer, usize x, usize y, usize width, usize height);
void rtdx_command_buffer_end_rendering(rtdx_command_buffer* command_buffer);
void rtdx_command_buffer_use_graphics_program(rtdx_command_buffer* command_buffer, rtdx_graphics_program* program);
void rtdx_command_buffer_bind_buffer(rtdx_command_buffer* command_buffer, rt_location location, rtdx_buffer* buffer, usize offset, usize size);
void rtdx_command_buffer_bind_texture(rtdx_command_buffer* command_buffer, rt_location location, rtdx_texture_view* texture_view);
void rtdx_command_buffer_vertex_buffer(rtdx_command_buffer* command_buffer, rt_location location, rtdx_buffer* buffer, usize offset);
void rtdx_command_buffer_index_buffer(rtdx_command_buffer* command_buffer, rtdx_buffer* buffer, usize offset, rt_index_format format);
void rtdx_command_buffer_draw(rtdx_command_buffer* command_buffer, usize count, usize first);
void rtdx_command_buffer_draw_instanced(rtdx_command_buffer* command_buffer, usize count, usize instances, usize first, usize first_instance);
void rtdx_command_buffer_draw_indexed(rtdx_command_buffer* command_buffer, usize count, usize first, usize vertex_offset);
void rtdx_command_buffer_draw_indexed_instanced(rtdx_command_buffer* command_buffer, usize count, usize instances, usize first, usize vertex_offset, usize first_instance);
void rtdx_command_buffer_end(rtdx_command_buffer* command_buffer);
void rtdx_command_buffer_buffer_data(rtdx_command_buffer* command_buffer, rtdx_buffer* buffer, rt_buffer_range range, const u08* data);
void rtdx_command_buffer_buffer_copy(rtdx_command_buffer* command_buffer, rtdx_buffer* src, rt_buffer_range src_range, rtdx_buffer* dst, rt_buffer_range dst_range);
void rtdx_command_buffer_buffer_barrier(rtdx_command_buffer* command_buffer, rtdx_buffer* buffer, rt_buffer_range range, rt_access src, rt_access dst);
void rtdx_command_buffer_buffer_copy_to_texture(rtdx_command_buffer* command_buffer, rtdx_buffer* src, rt_buffer_range src_range, rtdx_texture* dst, rt_texture_range dst_range);
void rtdx_command_buffer_texture_copy(rtdx_command_buffer* command_buffer, rtdx_texture* src, rt_texture_range src_range, rtdx_texture* dst, rt_texture_range dst_range);
void rtdx_command_buffer_texture_data(rtdx_command_buffer* command_buffer, rtdx_texture* texture, rt_texture_range range, const u08* data);
void rtdx_command_buffer_texture_copy_to_buffer(rtdx_command_buffer* command_buffer, rtdx_texture* src, rt_texture_range src_range, rtdx_buffer* dst, rt_buffer_range dst_range);
void rtdx_command_buffer_texture_barrier(rtdx_command_buffer* command_buffer, rtdx_texture* texture, rt_texture_range range, rt_access src, rt_access dst);
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
void rtdx_lower_buffer_data(ID3D12GraphicsCommandList* command_list, const rtdx_ir_buffer_data* command);
void rtdx_lower_buffer_copy(ID3D12GraphicsCommandList* command_list, const rtdx_ir_buffer_copy* command);
void rtdx_lower_buffer_copy_to_texture(rtdx_context* ctx, ID3D12GraphicsCommandList* command_list, rtdx_ir_buffer_copy_to_texture* command);
void rtdx_lower_texture_copy(ID3D12GraphicsCommandList* command_list, const rtdx_ir_texture_copy* command);
void rtdx_lower_texture_data(rtdx_context* ctx, ID3D12GraphicsCommandList* command_list, rtdx_ir_texture_data* command);
void rtdx_lower_texture_copy_to_buffer(rtdx_context* ctx, ID3D12GraphicsCommandList* command_list, rtdx_ir_texture_copy_to_buffer* command);
void rtdx_command_buffer_lower(rtdx_context* ctx, rtdx_command_buffer* command_buffer, ID3D12GraphicsCommandList* command_list, ID3D12DescriptorHeap** resource_heap, ID3D12DescriptorHeap** sampler_heap);
usize rtdx_command_record_size(rtdx_command_opcode opcode);
