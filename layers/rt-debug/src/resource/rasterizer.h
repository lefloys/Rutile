#ifndef RTDBG_RESOURCE_RASTERIZER_H
#define RTDBG_RESOURCE_RASTERIZER_H

#include "../rutile.h"

void rtdbg_rasterizer_reset(void);
void rtdbg_rasterizer_buffer_data(rt_buffer buffer, rt_buffer_range range, const u08* data);
void rtdbg_rasterizer_layout(const rt_vertex_layout* layout);
void rtdbg_rasterizer_vertex_buffer(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range);
void rtdbg_rasterizer_draw(rt_command_buffer command_buffer, usize vertex_count, usize first_vertex);
void rtdbg_rasterizer_begin(rt_command_buffer command_buffer);
void rtdbg_rasterizer_clear_color(rt_command_buffer command_buffer, f32 r, f32 g, f32 b, f32 a);
void rtdbg_rasterizer_viewport(rt_command_buffer command_buffer, usize width, usize height);
void rtdbg_rasterizer_execute(rt_command_buffer command_buffer, rt_command_buffer secondary);

#endif
