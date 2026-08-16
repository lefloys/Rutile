#include "graphics_program.h"
#include "logger.h"

#define RTVAL_DROP(message) rtval_printf("[validation] %s, dropping call\n", message)

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RT_API_PUBLIC rt_graphics_program rtGraphicsProgramCreate(void) {
	return rtval_graphics_program_to_handle(rtval_graphics_program_create());
}

RT_API_PUBLIC void rtGraphicsProgramDestroy(rt_graphics_program program) {
	rtval_graphics_program_destroy(rtval_graphics_program_from_handle(program));
}

RT_API_PUBLIC void rtGraphicsProgramSetLayout(rt_graphics_program program, const rt_vertex_layout* layout) {
	rtval_graphics_program_layout(rtval_graphics_program_from_handle(program), layout);
}
RT_API_PUBLIC void rtGraphicsProgramSetSource(rt_graphics_program program, const u08* data, usize size) {
	rtval_graphics_program_source(rtval_graphics_program_from_handle(program), data, size);
}

RT_API_PUBLIC void rtGraphicsProgramSetRasterState(rt_graphics_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) {
	rtval_graphics_program_raster_state(rtval_graphics_program_from_handle(program), cull_mode, front_face, fill_mode);
}

RT_API_PUBLIC void rtGraphicsProgramSetBlendState(rt_graphics_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op) {
	rtval_graphics_program_blend_state(rtval_graphics_program_from_handle(program), enabled, src_color, dst_color, color_op, src_alpha, dst_alpha, alpha_op);
}

RT_API_PUBLIC void rtGraphicsProgramFinalize(rt_graphics_program program) {
	rtval_graphics_program_finalize(rtval_graphics_program_from_handle(program));
}

RT_API_PUBLIC rt_location rtGraphicsProgramUniformLocation(rt_graphics_program program, const char* name) {
	return rtval_graphics_program_uniform_location(rtval_graphics_program_from_handle(program), name);
}

RT_API_PUBLIC rt_location rtGraphicsProgramInputLocation(rt_graphics_program program, const rt_vertex_attribute* attributes, usize attribute_count) {
	return rtval_graphics_program_input_location(rtval_graphics_program_from_handle(program), attributes, attribute_count);
}

RT_API_PUBLIC rt_location rtGraphicsProgramOutputLocation(rt_graphics_program program, const char* name) {
	return rtval_graphics_program_output_location(rtval_graphics_program_from_handle(program), name);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtval_graphics_program* rtval_graphics_program_create(void) {
	rt_graphics_program backend = rtval_next_rtGraphicsProgramCreate();
	if (!backend) {
		rtval_report_error("rtGraphicsProgramCreate");
		return NULL;
	}
	struct rtval_graphics_program* handle = rtval_handle_create(RTVAL_HANDLE_TYPE_GRAPHICS_PROGRAM);
	if (!handle) {
		rtval_next_rtGraphicsProgramDestroy(backend);
		return NULL;
	}
	struct rtval_graphics_program* state = RTVAL_PAYLOAD(handle, struct rtval_graphics_program);
	state->backend = backend;
	rtval_report_error("rtGraphicsProgramCreate");
	return handle;
}

void rtval_graphics_program_destroy(struct rtval_graphics_program* program) {
	if (!program) {
		return;
	}
	struct rtval_graphics_program* state = RTVAL_PAYLOAD(program, struct rtval_graphics_program);
	if (!state) {
		RTVAL_DROP("rtGraphicsProgramDestroy: null handle");
		return;
	}
	rtval_next_rtGraphicsProgramDestroy(state->backend);
	rtval_handle_destroy(program);
}

void rtval_graphics_program_layout(struct rtval_graphics_program* program, const rt_vertex_layout* layout) {
	struct rtval_graphics_program* state = RTVAL_PAYLOAD(program, struct rtval_graphics_program);
	if (!state) {
		RTVAL_DROP("rtGraphicsProgramSetLayout: null handle");
		return;
	}
	if (layout && layout->input_count && !layout->inputs) {
		RTVAL_DROP("rtGraphicsProgramSetLayout: missing inputs");
		return;
	}

	rtval_next_rtGraphicsProgramSetLayout(state->backend, layout);
	rtval_report_error("rtGraphicsProgramSetLayout");
}

void rtval_graphics_program_source(struct rtval_graphics_program* program, const u08* data, usize size) {
	struct rtval_graphics_program* state = RTVAL_PAYLOAD(program, struct rtval_graphics_program);
	if (!state) {
		RTVAL_DROP("rtGraphicsProgramSetSource: null handle");
		return;
	}
	if (!data || size == 0) {
		RTVAL_DROP("rtGraphicsProgramSetSource: empty source data");
		return;
	}
	rtval_next_rtGraphicsProgramSetSource(state->backend, data, size);
	rtval_report_error("rtGraphicsProgramSetSource");
}

void rtval_graphics_program_raster_state(struct rtval_graphics_program* program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) {
	struct rtval_graphics_program* state = RTVAL_PAYLOAD(program, struct rtval_graphics_program);
	if (!state) {
		RTVAL_DROP("rtGraphicsProgramRasterState: null handle");
		return;
	}

	rtval_next_rtGraphicsProgramSetRasterState(state->backend, cull_mode, front_face, fill_mode);
	rtval_report_error("rtGraphicsProgramSetRasterState");
}

void rtval_graphics_program_blend_state(
	struct rtval_graphics_program* program,
	bool enabled,
	enum rt_blend_factor src_color,
	enum rt_blend_factor dst_color,
	enum rt_blend_op color_op,
	enum rt_blend_factor src_alpha,
	enum rt_blend_factor dst_alpha,
	enum rt_blend_op alpha_op
) {
	struct rtval_graphics_program* state = RTVAL_PAYLOAD(program, struct rtval_graphics_program);
	if (!state) {
		RTVAL_DROP("rtGraphicsProgramBlendState: null handle");
		return;
	}

	rtval_next_rtGraphicsProgramSetBlendState(state->backend, enabled, src_color, dst_color, color_op, src_alpha, dst_alpha, alpha_op);
	rtval_report_error("rtGraphicsProgramSetBlendState");
}

void rtval_graphics_program_finalize(struct rtval_graphics_program* program) {
	struct rtval_graphics_program* state = RTVAL_PAYLOAD(program, struct rtval_graphics_program);
	if (!state) {
		RTVAL_DROP("rtGraphicsProgramFinalize: null handle");
		return;
	}

	rtval_next_rtGraphicsProgramFinalize(state->backend);
	rtval_report_error("rtGraphicsProgramFinalize");
}

rt_location rtval_graphics_program_uniform_location(struct rtval_graphics_program* program, const char* name) {
	struct rtval_graphics_program* state = RTVAL_PAYLOAD(program, struct rtval_graphics_program);
	if (!state) {
		RTVAL_DROP("rtGraphicsProgramUniformLocation: null handle");
		return RT_NULL_HANDLE;
	}
	if (!name) {
		RTVAL_DROP("rtGraphicsProgramUniformLocation: NULL name");
		return RT_NULL_HANDLE;
	}

	rt_location location = rtval_next_rtGraphicsProgramUniformLocation(state->backend, name);
	rtval_report_error("rtGraphicsProgramUniformLocation");
	return location;
}

rt_location rtval_graphics_program_input_location(struct rtval_graphics_program* program, const rt_vertex_attribute* attributes, usize attribute_count) {
	struct rtval_graphics_program* state = RTVAL_PAYLOAD(program, struct rtval_graphics_program);
	if (!state || !attributes || !attribute_count) {
		RTVAL_DROP("rtGraphicsProgramInputLocation: program and attributes required");
		return RT_NULL_HANDLE;
	}
	rt_location location = rtval_next_rtGraphicsProgramInputLocation(state->backend, attributes, attribute_count);
	rtval_report_error("rtGraphicsProgramInputLocation");
	return location;
}

rt_location rtval_graphics_program_output_location(struct rtval_graphics_program* program, const char* name) {
	struct rtval_graphics_program* state = RTVAL_PAYLOAD(program, struct rtval_graphics_program);
	if (!state || !name) {
		RTVAL_DROP("rtGraphicsProgramOutputLocation: program and name required");
		return RT_NULL_HANDLE;
	}
	rt_location location = rtval_next_rtGraphicsProgramOutputLocation(state->backend, name);
	rtval_report_error("rtGraphicsProgramOutputLocation");
	return location;
}

#undef RTVAL_DROP
