#ifndef RTVK_COMMAND_BUFFER_H
#define RTVK_COMMAND_BUFFER_H

#include "buffer.h"
#include "config.h"
#include "framebuffer.h"
#include "graphics_program.h"
#include "resource.h"
#include "texture.h"

#include <volk.h>

/*===============================================================================================*/
/*                                                                                                */
/*===============================================================================================*/

RTVK_API rt_command_buffer rtCommandBufferCreate(void);
RTVK_API void rtCommandBufferDestroy(rt_command_buffer command_buffer);
RTVK_API void rtCmdReset(rt_command_buffer command_buffer);
RTVK_API void rtCmdBegin(rt_command_buffer command_buffer);
RTVK_API void rtCmdWait(rt_command_buffer command_buffer, rt_timepoint timepoint);
RTVK_API void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer);
RTVK_API void rtCmdClearColor(rt_command_buffer command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a);
RTVK_API void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth);
RTVK_API void rtCmdClearStencil(rt_command_buffer command_buffer, u32 stencil);
RTVK_API void rtCmdSetViewport(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth);
RTVK_API void rtCmdSetScissor(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height);
RTVK_API void rtCmdEndRendering(rt_command_buffer command_buffer);
RTVK_API void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program);
RTVK_API void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset, usize size);
RTVK_API void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view);
RTVK_API void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset);
RTVK_API void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, usize offset, enum rt_index_format format);
RTVK_API void rtCmdDraw(rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex);
RTVK_API void rtCmdDrawInstanced(rt_command_buffer command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
RTVK_API void rtCmdDrawIndexed(rt_command_buffer command_buffer, u32 index_count, u32 first_index, i32 vertex_offset);
RTVK_API void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance);
RTVK_API void rtCmdEnd(rt_command_buffer command_buffer);

/*===============================================================================================*/
/*                                                                                                */
/*===============================================================================================*/

typedef enum rtvk_command_opcode {
	RTVK_COMMAND_WAIT,
	RTVK_COMMAND_BEGIN_RENDERING,
	RTVK_COMMAND_CLEAR_COLOR,
	RTVK_COMMAND_CLEAR_DEPTH,
	RTVK_COMMAND_CLEAR_STENCIL,
	RTVK_COMMAND_SET_VIEWPORT,
	RTVK_COMMAND_SET_SCISSOR,
	RTVK_COMMAND_END_RENDERING,
	RTVK_COMMAND_USE_GRAPHICS_PROGRAM,
	RTVK_COMMAND_BIND_BUFFER,
	RTVK_COMMAND_BIND_TEXTURE,
	RTVK_COMMAND_VERTEX_BUFFER,
	RTVK_COMMAND_INDEX_BUFFER,
	RTVK_COMMAND_DRAW,
	RTVK_COMMAND_DRAW_INSTANCED,
	RTVK_COMMAND_DRAW_INDEXED,
	RTVK_COMMAND_DRAW_INDEXED_INSTANCED,
} rtvk_command_opcode;

struct rtvk_command_header {
	void* alignment;
	u08 opcode;
};

struct rtvk_ir_wait {
	rt_timepoint timepoint;
};

struct rtvk_ir_framebuffer {
	struct rtvk_framebuffer* framebuffer;
};

struct rtvk_ir_clear_color {
	u32 index;
	f32 r;
	f32 g;
	f32 b;
	f32 a;
};

struct rtvk_ir_clear_depth {
	f32 depth;
};

struct rtvk_ir_clear_stencil {
	u32 stencil;
};

struct rtvk_ir_viewport {
	u32 x;
	u32 y;
	u32 width;
	u32 height;
	f32 min_depth;
	f32 max_depth;
};

struct rtvk_ir_scissor {
	u32 x;
	u32 y;
	u32 width;
	u32 height;
};

struct rtvk_ir_program {
	struct rtvk_graphics_program* program;
};

struct rtvk_ir_buffer {
	rt_location location;
	struct rtvk_buffer* buffer;
	usize offset;
	usize size;
};

struct rtvk_ir_texture {
	rt_location location;
	struct rtvk_texture_view* view;
};

struct rtvk_ir_vertex_buffer {
	rt_location location;
	struct rtvk_buffer* buffer;
	usize offset;
};

struct rtvk_ir_index_buffer {
	struct rtvk_buffer* buffer;
	usize offset;
	enum rt_index_format format;
};

struct rtvk_ir_draw {
	u32 count;
	u32 first;
};

struct rtvk_ir_draw_instanced {
	u32 count;
	u32 instances;
	u32 first;
	u32 first_instance;
};

struct rtvk_ir_draw_indexed {
	u32 count;
	u32 first;
	i32 vertex_offset;
};

struct rtvk_ir_draw_indexed_instanced {
	u32 count;
	u32 instances;
	u32 first;
	i32 vertex_offset;
	u32 first_instance;
};

struct rtvk_command_buffer {
	struct rtvk_resource_base base;
	u08* ir_data;
	usize ir_size;
	usize ir_capacity;
	bool recording;
	bool executable;
};
RTVK_DECLARE_NEW_RESOURCE(command_buffer)

struct rtvk_lowered_command_segment {
	VkCommandBuffer vk_command_buffer;
	VkDescriptorPool vk_descriptor_pool;
	rt_timepoint wait;
	usize command_count;
};

struct rtvk_lowered_resource_job {
	struct rtvk_resource_job base;
	struct rtvk_lowered_resource_job* next;
};

struct rtvk_lowered_command_buffer {
	struct rtvk_lowered_resource_job* resource_jobs;
	struct rtvk_lowered_command_segment* segments;
	VkCommandPool vk_command_pool;
	usize segment_count;
	usize segment_capacity;
	usize segment_index;
};

void rtvk_command_buffer_reset(struct rtvk_command_buffer* command_buffer);
void rtvk_command_buffer_begin(struct rtvk_command_buffer* command_buffer);
void rtvk_command_buffer_wait(struct rtvk_command_buffer* command_buffer, rt_timepoint timepoint);
void rtvk_command_buffer_begin_rendering(struct rtvk_command_buffer* command_buffer, struct rtvk_framebuffer* framebuffer);
void rtvk_command_buffer_clear_color(struct rtvk_command_buffer* command_buffer, u32 index, f32 r, f32 g, f32 b, f32 a);
void rtvk_command_buffer_clear_depth(struct rtvk_command_buffer* command_buffer, f32 depth);
void rtvk_command_buffer_clear_stencil(struct rtvk_command_buffer* command_buffer, u32 stencil);
void rtvk_command_buffer_set_viewport(struct rtvk_command_buffer* command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth);
void rtvk_command_buffer_set_scissor(struct rtvk_command_buffer* command_buffer, u32 x, u32 y, u32 width, u32 height);
void rtvk_command_buffer_end_rendering(struct rtvk_command_buffer* command_buffer);
void rtvk_command_buffer_use_graphics_program(struct rtvk_command_buffer* command_buffer, struct rtvk_graphics_program* program);
void rtvk_command_buffer_bind_buffer(struct rtvk_command_buffer* command_buffer, rt_location location, struct rtvk_buffer* buffer, usize offset, usize size);
void rtvk_command_buffer_bind_texture(struct rtvk_command_buffer* command_buffer, rt_location location, struct rtvk_texture_view* view);
void rtvk_command_buffer_vertex_buffer(struct rtvk_command_buffer* command_buffer, rt_location location, struct rtvk_buffer* buffer, usize offset);
void rtvk_command_buffer_index_buffer(struct rtvk_command_buffer* command_buffer, struct rtvk_buffer* buffer, usize offset, enum rt_index_format format);
void rtvk_command_buffer_draw(struct rtvk_command_buffer* command_buffer, u32 count, u32 first);
void rtvk_command_buffer_draw_instanced(struct rtvk_command_buffer* command_buffer, u32 count, u32 instances, u32 first, u32 first_instance);
void rtvk_command_buffer_draw_indexed(struct rtvk_command_buffer* command_buffer, u32 count, u32 first, i32 vertex_offset);
void rtvk_command_buffer_draw_indexed_instanced(struct rtvk_command_buffer* command_buffer, u32 count, u32 instances, u32 first, i32 vertex_offset, u32 first_instance);
void rtvk_command_buffer_end(struct rtvk_command_buffer* command_buffer);
void rtvk_command_buffer_release_resources(struct rtvk_command_buffer* command_buffer);

usize rtvk_command_record_size(rtvk_command_opcode opcode);
void rtvk_command_buffer_lower(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer, struct rtvk_lowered_command_buffer* lowered);
void rtvk_lowered_command_buffer_destroy(struct rtvk_context* ctx, struct rtvk_lowered_command_buffer* lowered);

#endif
