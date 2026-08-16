#include "graphics_program.h"

#include "context.h"
#include "error.h"
#include "execution.h"

#include <stdlib.h>
#include <string.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/
rt_graphics_program rtGraphicsProgramCreate(void) {
	return rtgl_graphics_program_to_handle(rtgl_graphics_program_create(rtgl_get_current_context()));
}

void rtGraphicsProgramDestroy(rt_graphics_program program) {
	rtgl_graphics_program_destroy(
		rtgl_get_current_context(),
		rtgl_graphics_program_from_handle(program)
	);
}

void rtGraphicsProgramSetLayout(rt_graphics_program program, const rt_vertex_layout* layout) {
	rtgl_graphics_program_layout(
		rtgl_get_current_context(),
		rtgl_graphics_program_from_handle(program),
		layout
	);
}

void rtGraphicsProgramSetSource(rt_graphics_program program, const u08* data, usize size) {
	rtgl_graphics_program_source(
		rtgl_get_current_context(),
		rtgl_graphics_program_from_handle(program),
		data,
		size
	);
}

void rtGraphicsProgramSetRasterState(rt_graphics_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) {
	rtgl_graphics_program_raster_state(
		rtgl_get_current_context(),
		rtgl_graphics_program_from_handle(program),
		cull_mode,
		front_face,
		fill_mode
	);
}

void rtGraphicsProgramSetBlendState(rt_graphics_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op) {
	rtgl_graphics_program_blend_state(
		rtgl_get_current_context(),
		rtgl_graphics_program_from_handle(program),
		enabled,
		src_color,
		dst_color,
		color_op,
		src_alpha,
		dst_alpha,
		alpha_op
	);
}

void rtGraphicsProgramFinalize(rt_graphics_program program) {
	rtgl_graphics_program_finalize(
		rtgl_get_current_context(),
		rtgl_graphics_program_from_handle(program)
	);
}

rt_location rtGraphicsProgramUniformLocation(rt_graphics_program program, const char* name) {
	return (rt_location)rtgl_graphics_program_uniform_location(rtgl_get_current_context(), rtgl_graphics_program_from_handle(program), name);
}

rt_location rtGraphicsProgramInputLocation(rt_graphics_program program, const rt_vertex_attribute* attributes, usize attribute_count) {
	struct rtgl_graphics_program* internal = rtgl_graphics_program_from_handle(program);
	if (!internal || !attributes || !attribute_count || !internal->gl_program) {
		return NULL;
	}
	for (usize input_index = 0; input_index < internal->vertex_layout.input_count; input_index++) {
		const rt_vertex_input* input = &internal->vertex_layout.inputs[input_index];
		if (input->attribute_count != attribute_count) {
			continue;
		}
		bool match = true;
		for (usize attribute_index = 0; attribute_index < attribute_count; attribute_index++) {
			const rt_vertex_attribute* expected = &input->attributes[attribute_index];
			const rt_vertex_attribute* actual = &attributes[attribute_index];
			if (expected->offset != actual->offset || expected->format != actual->format || !expected->name || !actual->name || strcmp(expected->name, actual->name) != 0) {
				match = false;
				break;
			}
		}
		if (!match || internal->uniform_location_count == 16) {
			continue;
		}
		rtgl_uniform_location* location = &internal->uniform_locations[internal->uniform_location_count++];
		memset(location, 0, sizeof(*location));
		location->program = internal;
		location->binding = (u32)input_index;
		location->gl_location = -1;
		location->kind = RTGL_UNIFORM_LOCATION_VERTEX_STREAM;
		return (rt_location)location;
	}
	return NULL;
}

rt_location rtGraphicsProgramOutputLocation(rt_graphics_program program, const char* name) {
	(void)program;
	(void)name;
	return NULL;
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTGL_DEFINE_RESOURCE_PRIVATE(graphics_program)

void rtgl_graphics_program_init(struct rtgl_context* ctx, struct rtgl_graphics_program* program) {
	rtgl_init_resource_base(ctx, RTGL_RESOURCE_BASE(program), RTGL_RESOURCE_GRAPHICS_PROGRAM);
	program->cull_mode = RT_CULL_NONE;
	program->front_face = RT_FRONT_FACE_CCW;
	program->fill_mode = RT_FILL_SOLID;
	program->blend_enabled = false;
	program->src_color_blend = RT_BLEND_ONE;
	program->dst_color_blend = RT_BLEND_ZERO;
	program->color_blend_op = RT_BLEND_OP_ADD;
	program->src_alpha_blend = RT_BLEND_ONE;
	program->dst_alpha_blend = RT_BLEND_ZERO;
	program->alpha_blend_op = RT_BLEND_OP_ADD;
}

void rtgl_graphics_program_layout(struct rtgl_context* ctx, struct rtgl_graphics_program* internal, const rt_vertex_layout* layout) {
	if (!layout || !layout->inputs || layout->input_count == 0) {
		internal->vertex_layout = (rt_vertex_layout){ 0 };
		return;
	}
	if (layout->input_count > 16) {
		rtgl_throwf(RT_IMPROPER_USAGE, "too many vertex attributes");
		return;
	}
	usize attribute_count = 0;
	for (usize input_index = 0; input_index < layout->input_count; input_index++) {
		const rt_vertex_input* source = &layout->inputs[input_index];
		if (!source->attributes || !source->attribute_count || attribute_count + source->attribute_count > RTGL_MAX_VERTEX_ATTRIBUTES) {
			rtgl_throwf(RT_IMPROPER_USAGE, "invalid vertex input layout");
			return;
		}
		internal->vertex_inputs[input_index] = *source;
		memcpy(&internal->vertex_attributes[attribute_count], source->attributes, sizeof(source->attributes[0]) * source->attribute_count);
		internal->vertex_inputs[input_index].attributes = &internal->vertex_attributes[attribute_count];
		attribute_count += source->attribute_count;
	}
	internal->vertex_layout.inputs = internal->vertex_inputs;
	internal->vertex_layout.input_count = layout->input_count;
}

void rtgl_graphics_program_source(struct rtgl_context* ctx, struct rtgl_graphics_program* internal, const void* data, usize size) {
	free(internal->source_bytes);
	internal->source_bytes = NULL;
	internal->source_size = 0;
	if (!data || size == 0) {
		return;
	}
	internal->source_bytes = (u08*)malloc((usize)size);
	RTGL_CHECK_ALLOC(internal->source_bytes, (usize)size, "OpenGL RTSL program source");
	if (!internal->source_bytes) {
		return;
	}
	memcpy(internal->source_bytes, data, (usize)size);
	internal->source_size = size;
}

void rtgl_graphics_program_raster_state(struct rtgl_context* ctx, struct rtgl_graphics_program* internal, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) {
	internal->cull_mode = cull_mode;
	internal->front_face = front_face;
	internal->fill_mode = fill_mode;
}

void rtgl_graphics_program_blend_state(struct rtgl_context* ctx, struct rtgl_graphics_program* internal, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op) {
	internal->blend_enabled = enabled;
	internal->src_color_blend = src_color;
	internal->dst_color_blend = dst_color;
	internal->color_blend_op = color_op;
	internal->src_alpha_blend = src_alpha;
	internal->dst_alpha_blend = dst_alpha;
	internal->alpha_blend_op = alpha_op;
}

rtgl_uniform_location* rtgl_graphics_program_uniform_location(struct rtgl_context* ctx, struct rtgl_graphics_program* internal, const char* name) {
	(void)ctx;
	if (!internal || !name || !name[0]) {
		return NULL;
	}
	if (!internal->gl_program) {
		rtgl_throwf(RT_IMPROPER_USAGE, "graphics program must be finalized before querying uniforms");
		return NULL;
	}
	for (u32 i = 0; i < internal->uniform_location_count; i++) {
		if (strcmp(internal->uniform_locations[i].name, name) == 0) {
			return &internal->uniform_locations[i];
		}
	}
	return NULL;
}

void rtgl_graphics_program_finish(struct rtgl_graphics_program* program) {
	if (program->gl_program) {
		rtgl_execution_graphics_program_destroy(program->base.ctx, program);
	}
	free(program->source_bytes);
	program->source_bytes = NULL;
	program->source_size = 0;
	rtgl_finish_resource_base(RTGL_RESOURCE_BASE(program));
}

void rtgl_graphics_program_prepare(struct rtgl_context* ctx, struct rtgl_graphics_program* program) {
	(void)ctx;
	if (!program || !program->gl_program) {
		rtgl_throwf(RT_IMPROPER_USAGE, "OpenGL graphics program must be finalized before use");
	}
}

void rtgl_graphics_program_finalize(struct rtgl_context* ctx, struct rtgl_graphics_program* program) {
	if (!program || program->gl_program) {
		return;
	}
	rtgl_execution_graphics_program_finalize(ctx, program);
}
