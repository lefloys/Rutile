#ifndef RTVAL_COMMAND_BUFFER_H
#define RTVAL_COMMAND_BUFFER_H

#include "handles.h"

struct rtval_command_buffer {
	rt_command_buffer backend;
	bool recording;
	bool executable;
	bool rendering;
	bool continuation;
	bool continuation_rendering;
};

struct rtval_command_buffer* rtval_command_buffer_create(void);
void rtval_command_buffer_destroy(struct rtval_command_buffer* command_buffer);
void rtval_command_buffer_reset(struct rtval_command_buffer* command_buffer);
void rtval_command_buffer_begin(struct rtval_command_buffer* command_buffer);
void rtval_command_buffer_continue(struct rtval_command_buffer* command_buffer, bool rendering);
void rtval_command_buffer_execute(struct rtval_command_buffer* command_buffer, struct rtval_command_buffer* secondary);
void rtval_command_buffer_begin_rendering(struct rtval_command_buffer* command_buffer, struct rtval_framebuffer* framebuffer);
void rtval_command_buffer_clear_color(struct rtval_command_buffer* command_buffer, rt_location location, f32 r, f32 g, f32 b, f32 a);
void rtval_command_buffer_clear_depth(struct rtval_command_buffer* command_buffer, f32 depth);
void rtval_command_buffer_clear_stencil(struct rtval_command_buffer* command_buffer, usize stencil);
void rtval_command_buffer_clear(struct rtval_command_buffer* command_buffer, enum rt_clear_flag attachments);
void rtval_command_buffer_set_viewport(struct rtval_command_buffer* command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth);
void rtval_command_buffer_set_scissor(struct rtval_command_buffer* command_buffer, usize x, usize y, usize width, usize height);
void rtval_command_buffer_end_rendering(struct rtval_command_buffer* command_buffer);
void rtval_command_buffer_use_program(struct rtval_command_buffer* command_buffer, struct rtval_program* program);
void rtval_command_buffer_bind_buffer(struct rtval_command_buffer* command_buffer, rt_location location, struct rtval_buffer* buffer, rt_buffer_range range);
void rtval_command_buffer_bind_texture(struct rtval_command_buffer* command_buffer, rt_location location, struct rtval_texture_view* texture_view);
void rtval_command_buffer_vertex_buffer(struct rtval_command_buffer* command_buffer, rt_location location, struct rtval_buffer* buffer, rt_buffer_range range);
void rtval_command_buffer_index_buffer(struct rtval_command_buffer* command_buffer, struct rtval_buffer* buffer, rt_buffer_range range, enum rt_index_format format);
void rtval_command_buffer_draw(struct rtval_command_buffer* command_buffer, usize vertex_count, usize first_vertex);
void rtval_command_buffer_draw_instanced(struct rtval_command_buffer* command_buffer, usize vertex_count, usize instance_count, usize first_vertex, usize first_instance);
void rtval_command_buffer_draw_indexed(struct rtval_command_buffer* command_buffer, usize index_count, usize first_index, usize vertex_offset);
void rtval_command_buffer_draw_indexed_instanced(struct rtval_command_buffer* command_buffer, usize index_count, usize instance_count, usize first_index, usize vertex_offset, usize first_instance);
void rtval_command_buffer_end(struct rtval_command_buffer* command_buffer);

#endif
