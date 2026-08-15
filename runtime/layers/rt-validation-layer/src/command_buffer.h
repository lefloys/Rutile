#ifndef RTVAL_COMMAND_BUFFER_H
#define RTVAL_COMMAND_BUFFER_H

#include "handles.h"

struct rtval_command_buffer {
	rt_command_buffer backend;
	bool recording;
	bool executable;
	bool rendering;
};

struct rtval_command_buffer* rtval_command_buffer_create(void);
void rtval_command_buffer_destroy(struct rtval_command_buffer* command_buffer);
void rtval_command_buffer_reset(struct rtval_command_buffer* command_buffer);
void rtval_command_buffer_begin(struct rtval_command_buffer* command_buffer);
void rtval_command_buffer_wait(struct rtval_command_buffer* command_buffer, rt_timepoint timepoint);
void rtval_command_buffer_begin_rendering(struct rtval_command_buffer* command_buffer, struct rtval_framebuffer* framebuffer);
void rtval_command_buffer_clear_color(struct rtval_command_buffer* command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a);
void rtval_command_buffer_clear_depth(struct rtval_command_buffer* command_buffer, f32 depth);
void rtval_command_buffer_clear_stencil(struct rtval_command_buffer* command_buffer, u32 stencil);
void rtval_command_buffer_set_viewport(struct rtval_command_buffer* command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth);
void rtval_command_buffer_set_scissor(struct rtval_command_buffer* command_buffer, u32 x, u32 y, u32 width, u32 height);
void rtval_command_buffer_end_rendering(struct rtval_command_buffer* command_buffer);
void rtval_command_buffer_use_graphics_program(struct rtval_command_buffer* command_buffer, struct rtval_graphics_program* program);
void rtval_command_buffer_bind_buffer(struct rtval_command_buffer* command_buffer, rt_location location, struct rtval_buffer* buffer, usize offset, usize size);
void rtval_command_buffer_bind_texture(struct rtval_command_buffer* command_buffer, rt_location location, struct rtval_texture_view* texture_view);
void rtval_command_buffer_vertex_buffer(struct rtval_command_buffer* command_buffer, rt_location location, struct rtval_buffer* buffer, usize offset);
void rtval_command_buffer_index_buffer(struct rtval_command_buffer* command_buffer, struct rtval_buffer* buffer, usize offset, enum rt_index_format format);
void rtval_command_buffer_draw(struct rtval_command_buffer* command_buffer, u32 vertex_count, u32 first_vertex);
void rtval_command_buffer_draw_instanced(struct rtval_command_buffer* command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
void rtval_command_buffer_draw_indexed(struct rtval_command_buffer* command_buffer, u32 index_count, u32 first_index, i32 vertex_offset);
void rtval_command_buffer_draw_indexed_instanced(struct rtval_command_buffer* command_buffer, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance);
void rtval_command_buffer_end(struct rtval_command_buffer* command_buffer);

#endif
