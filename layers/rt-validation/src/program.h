#ifndef RTVAL_PROGRAM_H
#define RTVAL_PROGRAM_H

#include "handles.h"

struct rtval_program {
	rt_program backend;
};

struct rtval_program* rtval_program_create(void);
void rtval_program_destroy(struct rtval_program* program);
void rtval_program_layout(struct rtval_program* program, const rt_vertex_layout* layout);
void rtval_program_source(struct rtval_program* program, const char* entry_point, const u08* program_bytes, usize program_byte_size);
void rtval_program_raster_state(struct rtval_program* program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode);
void rtval_program_blend_state(struct rtval_program* program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op);
void rtval_program_finalize(struct rtval_program* program);
rt_location rtval_program_uniform_location(struct rtval_program* program, const char* name);
rt_location rtval_program_input_location(struct rtval_program* program, const rt_vertex_attribute* attributes, usize attribute_count);
rt_location rtval_program_output_location(struct rtval_program* program, const char* name);

#endif
