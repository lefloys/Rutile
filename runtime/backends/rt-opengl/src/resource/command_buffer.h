#ifndef RTGL_COMMAND_BUFFER_H
#define RTGL_COMMAND_BUFFER_H

#include "buffer.h"
#include "framebuffer.h"
#include "graphics_program.h"
#include "resource.h"
#include "texture.h"

RTGL_EXTERN_C_ENTER

RTGL_API rt_command_buffer rtCommandBufferCreate(void);
RTGL_API void rtCommandBufferDestroy(rt_command_buffer command_buffer);
RTGL_API void rtCmdReset(rt_command_buffer command_buffer);
RTGL_API void rtCmdBegin(rt_command_buffer command_buffer);
RTGL_API void rtCmdWait(rt_command_buffer command_buffer, rt_timepoint timepoint);
RTGL_API void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer);
RTGL_API void rtCmdClearColor(rt_command_buffer command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a);
RTGL_API void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth);
RTGL_API void rtCmdClearStencil(rt_command_buffer command_buffer, u32 stencil);
RTGL_API void rtCmdSetViewport(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth);
RTGL_API void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program);
RTGL_API void rtCmdSetScissor(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height);
RTGL_API void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset, usize size);
RTGL_API void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view);
RTGL_API void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset);
RTGL_API void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, usize offset, enum rt_index_format format);
RTGL_API void rtCmdDraw(rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex);
RTGL_API void rtCmdDrawInstanced(rt_command_buffer command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
RTGL_API void rtCmdDrawIndexed(rt_command_buffer command_buffer, u32 index_count, u32 first_index, i32 vertex_offset);
RTGL_API void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance);
RTGL_API void rtCmdEndRendering(rt_command_buffer command_buffer);
RTGL_API void rtCmdEnd(rt_command_buffer command_buffer);

RTGL_EXTERN_C_EXIT

typedef enum rtgl_recorded_command_kind {
	RTGL_RECORDED_COMMAND_BEGIN_RENDERING,
	RTGL_RECORDED_COMMAND_WAIT,
	RTGL_RECORDED_COMMAND_CLEAR_COLOR,
	RTGL_RECORDED_COMMAND_CLEAR_DEPTH,
	RTGL_RECORDED_COMMAND_CLEAR_STENCIL,
	RTGL_RECORDED_COMMAND_SET_VIEWPORT,
	RTGL_RECORDED_COMMAND_USE_GRAPHICS_PROGRAM,
	RTGL_RECORDED_COMMAND_SET_SCISSOR,
	RTGL_RECORDED_COMMAND_BIND_BUFFER,
	RTGL_RECORDED_COMMAND_BIND_TEXTURE,
	RTGL_RECORDED_COMMAND_VERTEX_BUFFER,
	RTGL_RECORDED_COMMAND_INDEX_BUFFER,
	RTGL_RECORDED_COMMAND_DRAW,
	RTGL_RECORDED_COMMAND_DRAW_INSTANCED,
	RTGL_RECORDED_COMMAND_DRAW_INDEXED,
	RTGL_RECORDED_COMMAND_DRAW_INDEXED_INSTANCED,
	RTGL_RECORDED_COMMAND_END_RENDERING,
} rtgl_recorded_command_kind;
typedef struct rtgl_recorded_command {
	rtgl_recorded_command_kind kind;
	u32 size;
	union {
		rt_timepoint wait;
		struct {
			struct rtgl_framebuffer* framebuffer;
			struct rtgl_texture_view* color_view;
			struct rtgl_texture_view* depth_view;
		} begin_rendering;
		struct {
			u32 color_index;
			f32 values[4];
		} clear_color;
		f32 clear_depth;
		u32 clear_stencil;
		struct {
			u32 x;
			u32 y;
			u32 width;
			u32 height;
			f32 min_depth;
			f32 max_depth;
		} set_viewport;
		struct {
			struct rtgl_graphics_program* program;
		} use_graphics_program;
		struct {
			u32 x;
			u32 y;
			u32 width;
			u32 height;
		} set_scissor;
		struct {
			struct rt_location_t* location;
			struct rtgl_graphics_program* location_program;
			struct rtgl_buffer* buffer;
			u64 offset;
			u64 size;
		} bind_buffer;
		struct {
			struct rt_location_t* location;
			struct rtgl_graphics_program* location_program;
			struct rtgl_texture_view* texture_view;
		} bind_texture;
		struct {
			struct rt_location_t* location;
			struct rtgl_graphics_program* location_program;
			struct rtgl_buffer* buffer;
			u64 offset;
		} vertex_buffer;
		struct {
			struct rtgl_buffer* buffer;
			u64 offset;
			enum rt_index_format format;
		} index_buffer;
		struct {
			u32 vertex_count;
			u32 first_vertex;
		} draw;
		struct {
			u32 vertex_count;
			u32 instance_count;
			u32 first_vertex;
			u32 first_instance;
		} draw_instanced;
		struct {
			u32 index_count;
			u32 first_index;
			i32 vertex_offset;
		} draw_indexed;
		struct {
			u32 index_count;
			u32 instance_count;
			u32 first_index;
			i32 vertex_offset;
			u32 first_instance;
		} draw_indexed_instanced;
	} data;
} rtgl_recorded_command;

struct rtgl_command_buffer {
	struct rtgl_resource_base base;
	rtgl_recorded_command* commands;
	u32 command_count;
	u32 command_capacity;
	bool recording;
	bool executable;
};

RTGL_EXTERN_C_ENTER
RTGL_DECLARE_NEW_RESOURCE(command_buffer)
rt_timepoint rtgl_command_buffer_submit(struct rtgl_context* ctx, struct rtgl_queue* queue, struct rtgl_command_buffer* command_buffer);
void rtgl_command_buffer_execute(struct rtgl_context* ctx, struct rtgl_command_buffer* command_buffer, struct rtgl_queue* queue, u64 complete_value);

RTGL_EXTERN_C_EXIT

#endif /* RTGL_COMMAND_BUFFER_H */
