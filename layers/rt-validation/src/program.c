#include "program.h"
#include "logger.h"

#define RTVAL_DROP(message) rtval_fail(message)

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RT_API_PUBLIC rt_program rtProgramCreate(void) {
	return rtval_program_to_handle(rtval_program_create());
}

RT_API_PUBLIC void rtProgramDestroy(rt_program program) {
	rtval_program_destroy(rtval_program_from_handle(program));
}

RT_API_PUBLIC void rtProgramSetLayout(rt_program program, const rt_vertex_layout* layout) {
	rtval_program_layout(rtval_program_from_handle(program), layout);
}
RT_API_PUBLIC void rtProgramSource(rt_program program, const char* entry_point, const u08* program_bytes, usize program_byte_size) {
	rtval_program_source(rtval_program_from_handle(program), entry_point, program_bytes, program_byte_size);
}

RT_API_PUBLIC void rtProgramSetRasterState(rt_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) {
	rtval_program_raster_state(rtval_program_from_handle(program), cull_mode, front_face, fill_mode);
}

RT_API_PUBLIC void rtProgramSetBlendState(rt_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op) {
	rtval_program_blend_state(rtval_program_from_handle(program), enabled, src_color, dst_color, color_op, src_alpha, dst_alpha, alpha_op);
}

RT_API_PUBLIC void rtProgramFinalize(rt_program program) {
	rtval_program_finalize(rtval_program_from_handle(program));
}

RT_API_PUBLIC rt_location rtProgramUniformLocation(rt_program program, const char* name) {
	return rtval_program_uniform_location(rtval_program_from_handle(program), name);
}

RT_API_PUBLIC rt_location rtProgramInputLocation(rt_program program, const rt_vertex_attribute* attributes, usize attribute_count) {
	return rtval_program_input_location(rtval_program_from_handle(program), attributes, attribute_count);
}

RT_API_PUBLIC rt_location rtProgramOutputLocation(rt_program program, const char* name) {
	return rtval_program_output_location(rtval_program_from_handle(program), name);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtval_program* rtval_program_create(void) {
	rt_program backend = rtval_next_rtProgramCreate();
	if (!backend) {
		rtval_report_error("rtProgramCreate");
		return NULL;
	}
	struct rtval_program* handle = rtval_handle_create(RTVAL_HANDLE_TYPE_PROGRAM);
	if (!handle) {
		rtval_next_rtProgramDestroy(backend);
		return NULL;
	}
	struct rtval_program* state = RTVAL_PAYLOAD(handle, struct rtval_program);
	state->backend = backend;
	rtval_report_error("rtProgramCreate");
	return handle;
}

void rtval_program_destroy(struct rtval_program* program) {
	if (!program) {
		RTVAL_DROP("rtProgramDestroy: invalid handle");
		return;
	}
	struct rtval_program* state = RTVAL_PAYLOAD(program, struct rtval_program);
	if (!state) {
		RTVAL_DROP("rtProgramDestroy: null handle");
		return;
	}
	rtval_next_rtProgramDestroy(state->backend);
	if (rtval_report_error("rtProgramDestroy")) {
		rtval_handle_destroy(program);
	}
}

void rtval_program_layout(struct rtval_program* program, const rt_vertex_layout* layout) {
	struct rtval_program* state = RTVAL_PAYLOAD(program, struct rtval_program);
	if (!state) {
		RTVAL_DROP("rtProgramSetLayout: null handle");
		return;
	}
	if (layout && layout->input_count && !layout->inputs) {
		RTVAL_DROP("rtProgramSetLayout: missing inputs");
		return;
	}

	rtval_next_rtProgramSetLayout(state->backend, layout);
	rtval_report_error("rtProgramSetLayout");
}

void rtval_program_source(struct rtval_program* program, const char* entry_point, const u08* program_bytes, usize program_byte_size) {
	struct rtval_program* state = RTVAL_PAYLOAD(program, struct rtval_program);
	if (!state) {
		RTVAL_DROP("rtProgramSource: null handle");
		return;
	}
	if (!program_bytes || program_byte_size == 0) {
		RTVAL_DROP("rtProgramSource: empty linked RTSLP program bytes");
		return;
	}
	if (!entry_point || !entry_point[0]) {
		RTVAL_DROP("rtProgramSource: empty entry point");
		return;
	}
	rtval_next_rtProgramSource(state->backend, entry_point, program_bytes, program_byte_size);
	rtval_report_error("rtProgramSource");
}

void rtval_program_raster_state(struct rtval_program* program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) {
	struct rtval_program* state = RTVAL_PAYLOAD(program, struct rtval_program);
	if (!state) {
		RTVAL_DROP("rtProgramRasterState: null handle");
		return;
	}
	if (cull_mode < RT_CULL_NONE || cull_mode > RT_CULL_BACK || front_face < RT_FRONT_FACE_CCW || front_face > RT_FRONT_FACE_CW || fill_mode < RT_FILL_SOLID || fill_mode > RT_FILL_WIREFRAME) {
		RTVAL_DROP("rtProgramSetRasterState: valid raster-state enums required");
		return;
	}

	rtval_next_rtProgramSetRasterState(state->backend, cull_mode, front_face, fill_mode);
	rtval_report_error("rtProgramSetRasterState");
}

void rtval_program_blend_state(
	struct rtval_program* program,
	bool enabled,
	enum rt_blend_factor src_color,
	enum rt_blend_factor dst_color,
	enum rt_blend_op color_op,
	enum rt_blend_factor src_alpha,
	enum rt_blend_factor dst_alpha,
	enum rt_blend_op alpha_op
) {
	struct rtval_program* state = RTVAL_PAYLOAD(program, struct rtval_program);
	if (!state) {
		RTVAL_DROP("rtProgramBlendState: null handle");
		return;
	}
	if (src_color < RT_BLEND_ZERO || src_color > RT_BLEND_ONE_MINUS_DST_ALPHA || dst_color < RT_BLEND_ZERO || dst_color > RT_BLEND_ONE_MINUS_DST_ALPHA || src_alpha < RT_BLEND_ZERO || src_alpha > RT_BLEND_ONE_MINUS_DST_ALPHA || dst_alpha < RT_BLEND_ZERO || dst_alpha > RT_BLEND_ONE_MINUS_DST_ALPHA || color_op < RT_BLEND_OP_ADD || color_op > RT_BLEND_OP_MAX || alpha_op < RT_BLEND_OP_ADD || alpha_op > RT_BLEND_OP_MAX) {
		RTVAL_DROP("rtProgramSetBlendState: valid blend-state enums required");
		return;
	}

	rtval_next_rtProgramSetBlendState(state->backend, enabled, src_color, dst_color, color_op, src_alpha, dst_alpha, alpha_op);
	rtval_report_error("rtProgramSetBlendState");
}

void rtval_program_finalize(struct rtval_program* program) {
	struct rtval_program* state = RTVAL_PAYLOAD(program, struct rtval_program);
	if (!state) {
		RTVAL_DROP("rtProgramFinalize: null handle");
		return;
	}

	rtval_next_rtProgramFinalize(state->backend);
	rtval_report_error("rtProgramFinalize");
}

rt_location rtval_program_uniform_location(struct rtval_program* program, const char* name) {
	struct rtval_program* state = RTVAL_PAYLOAD(program, struct rtval_program);
	if (!state) {
		RTVAL_DROP("rtProgramUniformLocation: null handle");
		return RT_NULL_HANDLE;
	}
	if (!name) {
		RTVAL_DROP("rtProgramUniformLocation: NULL name");
		return RT_NULL_HANDLE;
	}

	rt_location location = rtval_next_rtProgramUniformLocation(state->backend, name);
	rtval_report_error("rtProgramUniformLocation");
	return location;
}

rt_location rtval_program_input_location(struct rtval_program* program, const rt_vertex_attribute* attributes, usize attribute_count) {
	struct rtval_program* state = RTVAL_PAYLOAD(program, struct rtval_program);
	if (!state || !attributes || !attribute_count) {
		RTVAL_DROP("rtProgramInputLocation: program and attributes required");
		return RT_NULL_HANDLE;
	}
	rt_location location = rtval_next_rtProgramInputLocation(state->backend, attributes, attribute_count);
	rtval_report_error("rtProgramInputLocation");
	return location;
}

rt_location rtval_program_output_location(struct rtval_program* program, const char* name) {
	struct rtval_program* state = RTVAL_PAYLOAD(program, struct rtval_program);
	if (!state) {
		RTVAL_DROP("rtProgramOutputLocation: valid program required");
		return RT_NULL_HANDLE;
	}
	rt_location location = rtval_next_rtProgramOutputLocation(state->backend, name);
	rtval_report_error("rtProgramOutputLocation");
	return location;
}

#undef RTVAL_DROP
