#pragma once

#include "config.hpp"
#include "resource.hpp"

#include <d3d12.h>
#include <vector>

struct rt_buffer_t;
struct rt_framebuffer_t;
struct rt_program_t;
struct rtd3d12_image_base;
struct rt_texture_t;
struct rt_texture_view_t;

RTD3D12_API rt_command_buffer_t* rtCommandBufferCreate(void);
RTD3D12_API void rtCommandBufferDestroy(rt_command_buffer_t* command_buffer);
RTD3D12_API void rtCommandBufferReset(rt_command_buffer_t* command_buffer);
RTD3D12_API void rtCommandBufferBegin(rt_command_buffer_t* command_buffer);
RTD3D12_API void rtCommandBufferContinue(rt_command_buffer_t* command_buffer);
RTD3D12_API void rtCommandBufferContinueRendering(rt_command_buffer_t* command_buffer);
RTD3D12_API void rtCommandBufferEnd(rt_command_buffer_t* command_buffer);

RTD3D12_API void rtCmdExecute(rt_command_buffer_t* command_buffer, rt_command_buffer_t* secondary);
RTD3D12_API void rtCmdBeginRendering(rt_command_buffer_t* command_buffer, rt_framebuffer_t* framebuffer);
RTD3D12_API void rtCmdClearColor(rt_command_buffer_t* command_buffer, rt::location* location, f32 r, f32 g, f32 b, f32 a);
RTD3D12_API void rtCmdClearDepth(rt_command_buffer_t* command_buffer, f32 depth);
RTD3D12_API void rtCmdClearStencil(rt_command_buffer_t* command_buffer, usize stencil);
RTD3D12_API void rtCmdClear(rt_command_buffer_t* command_buffer, rt::clear attachments);
RTD3D12_API void rtCmdSetViewport(rt_command_buffer_t* command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth);
RTD3D12_API void rtCmdSetScissor(rt_command_buffer_t* command_buffer, usize x, usize y, usize width, usize height);
RTD3D12_API void rtCmdEndRendering(rt_command_buffer_t* command_buffer);
RTD3D12_API void rtCmdUseProgram(rt_command_buffer_t* command_buffer, rt_program_t* program);
RTD3D12_API void rtCmdUniformData(rt_command_buffer_t* command_buffer, rt::location* location, const u08* data, usize size);
RTD3D12_API void rtCmdStorageData(rt_command_buffer_t* command_buffer, rt::location* location, const u08* data, usize size);
RTD3D12_API void rtCmdBindBuffer(rt_command_buffer_t* command_buffer, rt::location* location, rt_buffer_t* buffer, rt::buffer_range range);
RTD3D12_API void rtCmdBindTexture(rt_command_buffer_t* command_buffer, rt::location* location, rt_texture_view_t* texture_view);
RTD3D12_API void rtCmdVertexBuffer(rt_command_buffer_t* command_buffer, rt::location* location, rt_buffer_t* buffer, rt::buffer_range range);
RTD3D12_API void rtCmdIndexBuffer(rt_command_buffer_t* command_buffer, rt_buffer_t* buffer, rt::buffer_range range, rt::index_format format);
RTD3D12_API void rtCmdDraw(rt_command_buffer_t* command_buffer, usize vertex_count, usize first_vertex);
RTD3D12_API void rtCmdDrawInstanced(rt_command_buffer_t* command_buffer, usize vertex_count, usize instance_count, usize first_vertex, usize first_instance);
RTD3D12_API void rtCmdDrawIndexed(rt_command_buffer_t* command_buffer, usize index_count, usize first_index, usize vertex_offset);
RTD3D12_API void rtCmdDrawIndexedInstanced(rt_command_buffer_t* command_buffer, usize index_count, usize instance_count, usize first_index, usize vertex_offset, usize first_instance);
RTD3D12_API void rtCmdBufferData(rt_command_buffer_t* command_buffer, rt_buffer_t* buffer, rt::buffer_range range, const u08* data);
RTD3D12_API void rtCmdBufferCopy(rt_command_buffer_t* command_buffer, rt_buffer_t* src, rt::buffer_range src_range, rt_buffer_t* dst, rt::buffer_range dst_range);
RTD3D12_API void rtCmdBufferCopyToTexture(rt_command_buffer_t* command_buffer, rt_buffer_t* src, rt::buffer_range src_range, rt_texture_t* dst, rt::texture_range dst_range);
RTD3D12_API void rtCmdBufferBarrier(rt_command_buffer_t* command_buffer, rt_buffer_t* buffer, rt::buffer_range range, rt::access src, rt::access dst);

void rtd3d12_command_transition_buffer(ID3D12GraphicsCommandList* command_list, rt_buffer_t* buffer, D3D12_RESOURCE_STATES state);
void rtd3d12_command_transition_image(ID3D12GraphicsCommandList* command_list, rtd3d12_image_base* image, D3D12_RESOURCE_STATES state);
void rtd3d12_command_transition_image_range(ID3D12GraphicsCommandList* command_list, rtd3d12_image_base* image, rt::texture_range range, D3D12_RESOURCE_STATES state);

enum class rtd3d12_command_opcode : u08 {
	begin_rendering,
	clear_color,
	clear_depth,
	clear_stencil,
	set_viewport,
	set_scissor,
	end_rendering,
	use_program,
	uniform_data,
	storage_data,
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

struct rtd3d12_command_header {
	void* alignment;
	rtd3d12_command_opcode opcode;
};
struct rtd3d12_ir_framebuffer {
	rt_framebuffer_t* framebuffer;
};
struct rtd3d12_ir_clear_color {
	u32 index;
	f32 r;
	f32 g;
	f32 b;
	f32 a;
};
struct rtd3d12_ir_clear_depth {
	f32 depth;
};
struct rtd3d12_ir_clear_stencil {
	u32 stencil;
};
struct rtd3d12_ir_viewport {
	usize x;
	usize y;
	usize width;
	usize height;
	f32 min_depth;
	f32 max_depth;
};
struct rtd3d12_ir_scissor {
	usize x;
	usize y;
	usize width;
	usize height;
};
struct rtd3d12_ir_program {
	rt_program_t* program;
};
struct rtd3d12_ir_program_data {
	u08 address;
	std::byte* bytes;
	usize size;
	ID3D12Resource* resource;
	ID3D12Resource* upload;
};
struct rtd3d12_ir_buffer {
	u08 address;
	rt_buffer_t* buffer;
	usize offset;
	usize size;
};
struct rtd3d12_ir_texture {
	u08 address;
	rt_texture_view_t* texture_view;
	rtd3d12_image_base* image;
	ID3D12DescriptorHeap* sampler_heap;
	D3D12_CPU_DESCRIPTOR_HANDLE sampler_cpu;
};
struct rtd3d12_ir_vertex_buffer {
	u08 address;
	rt_buffer_t* buffer;
	usize offset;
};
struct rtd3d12_ir_index_buffer {
	rt_buffer_t* buffer;
	usize offset;
	rt::index_format format;
};
struct rtd3d12_ir_draw {
	usize count;
	usize first;
};
struct rtd3d12_ir_draw_instanced {
	usize count;
	usize instances;
	usize first;
	usize first_instance;
};
struct rtd3d12_ir_draw_indexed {
	usize count;
	usize first;
	usize vertex_offset;
};
struct rtd3d12_ir_draw_indexed_instanced {
	usize count;
	usize instances;
	usize first;
	usize vertex_offset;
	usize first_instance;
};
struct rtd3d12_ir_buffer_data {
	rt_buffer_t* copy_source;
	rt_buffer_t* target;
	rt::buffer_range range;
	ID3D12Resource* upload;
};
struct rtd3d12_ir_buffer_copy {
	rt_buffer_t* source;
	rt::buffer_range src_range;
	rt_buffer_t* target_copy_source;
	rt_buffer_t* target;
	rt::buffer_range dst_range;
};
struct rtd3d12_ir_buffer_barrier {
	rt_buffer_t* buffer;
	rt::access src;
	rt::access dst;
};
struct rtd3d12_ir_buffer_copy_to_texture {
	rt_buffer_t* source;
	rt::buffer_range src_range;
	rtd3d12_image_base* target_copy_source;
	rtd3d12_image_base* target;
	rt::texture_range dst_range;
	ID3D12Resource* staging;
};
struct rtd3d12_ir_texture_copy {
	rtd3d12_image_base* source;
	rt::texture_range src_range;
	rtd3d12_image_base* target_copy_source;
	rtd3d12_image_base* target;
	rt::texture_range dst_range;
};
struct rtd3d12_ir_texture_data {
	rtd3d12_image_base* copy_source;
	rtd3d12_image_base* target;
	rt::texture_range range;
	u08* data;
	usize data_size;
	ID3D12Resource* upload;
};
struct rtd3d12_ir_texture_copy_to_buffer {
	rtd3d12_image_base* source;
	rt::texture_range src_range;
	rt_buffer_t* target_copy_source;
	rt_buffer_t* target;
	rt::buffer_range dst_range;
	ID3D12Resource* staging;
};
struct rtd3d12_ir_texture_barrier {
	rtd3d12_image_base* image;
	rt::texture_range range;
	rt::access src;
	rt::access dst;
};

struct rt_command_buffer_t : rtd3d12_resource<rt_command_buffer_t> {
	explicit rt_command_buffer_t(rtd3d12_context* ctx) : rtd3d12_resource(ctx) {}
	~rt_command_buffer_t();
	void uniform_data(rt::location* location, const u08* data, usize size);
	void storage_data(rt::location* location, const u08* data, usize size);
	u08* ir_data{};
	usize ir_size{};
	usize ir_capacity{};
	bool recording{};
	bool executable{};
	bool continuation{};
	bool rendering_continuation{};
	bool rendering{};
	u32 in_flight{};
	rt_framebuffer_t* active_framebuffer{};
	f32 clear_colors[8][4]{};
	f32 clear_depth_value{};
	u32 clear_stencil_value{};
};
struct rtd3d12_command_lower_state {
	rt_framebuffer_t* framebuffer;
	usize color_count;
	rtd3d12_image_base* color_images[8];
	D3D12_CPU_DESCRIPTOR_HANDLE color_rtvs[8];
	rtd3d12_image_base* depth_image;
	D3D12_CPU_DESCRIPTOR_HANDLE depth_dsv;
	rtd3d12_image_base* stencil_image;
	D3D12_CPU_DESCRIPTOR_HANDLE stencil_dsv;
	rt_program_t* program;
	ID3D12DescriptorHeap* resource_heap;
	ID3D12DescriptorHeap* sampler_heap;
	UINT resource_step;
	UINT sampler_step;
	UINT resource_index;
	UINT sampler_index;
	rtd3d12_ir_buffer pending_buffers[32];
	usize pending_buffer_count;
	rtd3d12_ir_texture pending_textures[32];
	usize pending_texture_count;
	std::vector<std::byte> uniform_blocks[256];
	std::vector<std::byte> storage_blocks[256];
	ID3D12Resource* uniform_resources[256];
	ID3D12Resource* storage_resources[256];
};

void rtd3d12_command_buffer_reset(rt_command_buffer_t* command_buffer);
void rtd3d12_command_buffer_release_resources(rt_command_buffer_t* command_buffer);
rt_command_buffer_t* rtd3d12_command_buffer_snapshot_create(const rt_command_buffer_t* command_buffer);
void rtd3d12_command_buffer_begin(rt_command_buffer_t* command_buffer);
void rtd3d12_command_buffer_continue(rt_command_buffer_t* command_buffer, bool rendering);
void rtd3d12_command_buffer_execute(rt_command_buffer_t* command_buffer, rt_command_buffer_t* secondary);
void rtd3d12_command_buffer_begin_rendering(rt_command_buffer_t* command_buffer, rt_framebuffer_t* framebuffer);
void rtd3d12_command_buffer_clear_color(rt_command_buffer_t* command_buffer, u32 index, f32 r, f32 g, f32 b, f32 a);
void rtd3d12_command_buffer_clear_depth(rt_command_buffer_t* command_buffer, f32 depth);
void rtd3d12_command_buffer_clear_stencil(rt_command_buffer_t* command_buffer, u32 stencil);
void rtd3d12_command_buffer_clear(rt_command_buffer_t* command_buffer, rt::clear attachments);
void rtd3d12_command_buffer_set_viewport(rt_command_buffer_t* command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth);
void rtd3d12_command_buffer_set_scissor(rt_command_buffer_t* command_buffer, usize x, usize y, usize width, usize height);
void rtd3d12_command_buffer_end_rendering(rt_command_buffer_t* command_buffer);
void rtd3d12_command_buffer_use_program(rt_command_buffer_t* command_buffer, rt_program_t* program);
void rtd3d12_command_buffer_bind_buffer(rt_command_buffer_t* command_buffer, rt::location* location, rt_buffer_t* buffer, usize offset, usize size);
void rtd3d12_command_buffer_bind_texture(rt_command_buffer_t* command_buffer, rt::location* location, rt_texture_view_t* texture_view);
void rtd3d12_command_buffer_vertex_buffer(rt_command_buffer_t* command_buffer, rt::location* location, rt_buffer_t* buffer, usize offset);
void rtd3d12_command_buffer_index_buffer(rt_command_buffer_t* command_buffer, rt_buffer_t* buffer, usize offset, rt::index_format format);
void rtd3d12_command_buffer_draw(rt_command_buffer_t* command_buffer, usize count, usize first);
void rtd3d12_command_buffer_draw_instanced(rt_command_buffer_t* command_buffer, usize count, usize instances, usize first, usize first_instance);
void rtd3d12_command_buffer_draw_indexed(rt_command_buffer_t* command_buffer, usize count, usize first, usize vertex_offset);
void rtd3d12_command_buffer_draw_indexed_instanced(rt_command_buffer_t* command_buffer, usize count, usize instances, usize first, usize vertex_offset, usize first_instance);
void rtd3d12_command_buffer_end(rt_command_buffer_t* command_buffer);
void rtd3d12_command_buffer_buffer_data(rt_command_buffer_t* command_buffer, rt_buffer_t* buffer, rt::buffer_range range, const u08* data);
void rtd3d12_command_buffer_buffer_copy(rt_command_buffer_t* command_buffer, rt_buffer_t* src, rt::buffer_range src_range, rt_buffer_t* dst, rt::buffer_range dst_range);
void rtd3d12_command_buffer_buffer_barrier(rt_command_buffer_t* command_buffer, rt_buffer_t* buffer, rt::buffer_range range, rt::access src, rt::access dst);
void rtd3d12_command_buffer_buffer_copy_to_texture(rt_command_buffer_t* command_buffer, rt_buffer_t* src, rt::buffer_range src_range, rt_texture_t* dst, rt::texture_range dst_range);
void rtd3d12_command_buffer_texture_copy(rt_command_buffer_t* command_buffer, rt_texture_t* src, rt::texture_range src_range, rt_texture_t* dst, rt::texture_range dst_range);
void rtd3d12_command_buffer_texture_data(rt_command_buffer_t* command_buffer, rt_texture_t* texture, rt::texture_range range, const u08* data);
void rtd3d12_command_buffer_texture_copy_to_buffer(rt_command_buffer_t* command_buffer, rt_texture_t* src, rt::texture_range src_range, rt_buffer_t* dst, rt::buffer_range dst_range);
void rtd3d12_command_buffer_texture_barrier(rt_command_buffer_t* command_buffer, rt_texture_t* texture, rt::texture_range range, rt::access src, rt::access dst);
void rtd3d12_lower_begin_rendering(rtd3d12_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_framebuffer* command);
void rtd3d12_lower_clear_color(rtd3d12_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_clear_color* command);
void rtd3d12_lower_clear_depth(rtd3d12_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_clear_depth* command);
void rtd3d12_lower_clear_stencil(rtd3d12_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_clear_stencil* command);
void rtd3d12_lower_end_rendering(rtd3d12_command_lower_state* state, ID3D12GraphicsCommandList* command_list);
void rtd3d12_lower_set_viewport(ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_viewport* command);
void rtd3d12_lower_set_scissor(ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_scissor* command);
void rtd3d12_lower_use_program(rtd3d12_context* ctx, rtd3d12_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_program* command);
void rtd3d12_lower_bind_buffer(rtd3d12_context* ctx, rtd3d12_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_buffer* command);
void rtd3d12_lower_bind_texture(rtd3d12_context* ctx, rtd3d12_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_texture* command);
void rtd3d12_lower_vertex_buffer(rtd3d12_command_lower_state* state, ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_vertex_buffer* command);
void rtd3d12_lower_index_buffer(ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_index_buffer* command);
void rtd3d12_lower_draw(ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_draw* command);
void rtd3d12_lower_draw_instanced(ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_draw_instanced* command);
void rtd3d12_lower_draw_indexed(ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_draw_indexed* command);
void rtd3d12_lower_draw_indexed_instanced(ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_draw_indexed_instanced* command);
void rtd3d12_lower_buffer_data(ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_buffer_data* command);
void rtd3d12_lower_buffer_copy(ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_buffer_copy* command);
void rtd3d12_lower_buffer_copy_to_texture(rtd3d12_context* ctx, ID3D12GraphicsCommandList* command_list, rtd3d12_ir_buffer_copy_to_texture* command);
void rtd3d12_lower_texture_copy(ID3D12GraphicsCommandList* command_list, const rtd3d12_ir_texture_copy* command);
void rtd3d12_lower_texture_data(rtd3d12_context* ctx, ID3D12GraphicsCommandList* command_list, rtd3d12_ir_texture_data* command);
void rtd3d12_lower_texture_copy_to_buffer(rtd3d12_context* ctx, ID3D12GraphicsCommandList* command_list, rtd3d12_ir_texture_copy_to_buffer* command);
void rtd3d12_command_buffer_lower(rtd3d12_context* ctx, rt_command_buffer_t* command_buffer, ID3D12GraphicsCommandList* command_list, ID3D12DescriptorHeap** resource_heap, ID3D12DescriptorHeap** sampler_heap);
usize rtd3d12_command_record_size(rtd3d12_command_opcode opcode);
