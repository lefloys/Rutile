#include "program.h"

#include "context.h"
#include "error.h"
#include "execution/execution.h"

#include <stdlib.h>
#include <string.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/
rt_program rtProgramCreate(void) {
	return rtgl_program_to_handle(rtgl_program_create(rtgl_get_current_context()));
}

void rtProgramDestroy(rt_program program) {
	rtgl_program_destroy(
		rtgl_get_current_context(),
		rtgl_program_from_handle(program)
	);
}

void rtProgramSetLayout(rt_program program, const rt_vertex_layout* layout) {
	rtgl_program_layout(
		rtgl_get_current_context(),
		rtgl_program_from_handle(program),
		layout
	);
}

void rtProgramSource(rt_program program, const char* entry_point, const u08* data, usize size) {
	rtgl_program_source(
		rtgl_get_current_context(),
		rtgl_program_from_handle(program),
		entry_point,
		data,
		size
	);
}

void rtProgramSetRasterState(rt_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) {
	rtgl_program_raster_state(
		rtgl_get_current_context(),
		rtgl_program_from_handle(program),
		cull_mode,
		front_face,
		fill_mode
	);
}

void rtProgramSetBlendState(rt_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op) {
	rtgl_program_blend_state(
		rtgl_get_current_context(),
		rtgl_program_from_handle(program),
		enabled,
		src_color,
		dst_color,
		color_op,
		src_alpha,
		dst_alpha,
		alpha_op
	);
}

void rtProgramFinalize(rt_program program) {
	rtgl_program_finalize(
		rtgl_get_current_context(),
		rtgl_program_from_handle(program)
	);
}

rt_location rtProgramUniformLocation(rt_program program, const char* name) {
	struct rtgl_context* ctx = rtgl_get_current_context();
	return rtgl_program_uniform_location(ctx, rtgl_program_from_handle(program), name);
}

rt_location rtProgramInputLocation(rt_program program, const rt_vertex_attribute* attributes, usize attribute_count) {
	struct rtgl_program* internal = rtgl_program_from_handle(program);
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
		if (!match) {
			continue;
		}
		for (u32 address = 1; address < RTGL_LOCATION_ADDRESS_COUNT; address++) {
			struct rt_location_t* location = &internal->locations[address];
			if (internal->location_occupied[address] && location->kind == RTGL_LOCATION_MAPPING_VERTEX_STREAM && location->binding == input_index) {
				return location;
			}
		}
		struct rt_location_t* location = rtgl_program_allocate_location(internal, false);
		if (!location) {
			continue;
		}
		location->program = internal;
		location->binding = (u32)input_index;
		location->gl_location = -1;
		location->kind = RTGL_LOCATION_MAPPING_VERTEX_STREAM;
		return location;
	}
	return NULL;
}

rt_location rtProgramOutputLocation(rt_program program, const char* name) {
	struct rtgl_program* internal = rtgl_program_from_handle(program);
	if (!internal || !internal->gl_program) {
		return NULL;
	}
	for (u32 address = 0; address < RTGL_LOCATION_ADDRESS_COUNT; address++) {
		struct rt_location_t* location = &internal->locations[address];
		if (internal->location_occupied[address] && location->kind == RTGL_LOCATION_MAPPING_OUTPUT && ((name && strcmp(location->name, name) == 0) || (!name && !location->name[0]))) {
			return location;
		}
	}
	return NULL;
}

struct rt_location_t* rtgl_program_allocate_location(struct rtgl_program* program, bool zero_address) {
	const u32 first = zero_address ? 0 : 1;
	const u32 end = zero_address ? 1 : RTGL_LOCATION_ADDRESS_COUNT;
	for (u32 address = first; address < end; address++) {
		if (!program->location_occupied[address]) {
			program->location_occupied[address] = true;
			struct rt_location_t* location = &program->locations[address];
			memset(location, 0, sizeof(*location));
			location->address = (u08)address;
			return location;
		}
	}
	return NULL;
}

void rtgl_program_clear_locations(struct rtgl_program* program) {
	memset(program->locations, 0, sizeof(program->locations));
	memset(program->location_occupied, 0, sizeof(program->location_occupied));
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTGL_DEFINE_RESOURCE_PRIVATE(program)

void rtgl_program_init(struct rtgl_context* ctx, struct rtgl_program* program) {
	rtgl_init_resource_base(ctx, RTGL_RESOURCE_BASE(program), RTGL_RESOURCE_PROGRAM);
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

void rtgl_program_layout(struct rtgl_context* ctx, struct rtgl_program* internal, const rt_vertex_layout* layout) {
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

void rtgl_program_source(struct rtgl_context* ctx, struct rtgl_program* internal, const char* entry_point, const void* data, usize size) {
	if (!entry_point || !entry_point[0]) {
		rtgl_throwf(RT_IMPROPER_USAGE, "program entry point is empty");
		return;
	}
	if (!data || size == 0) {
		rtgl_throwf(RT_IMPROPER_USAGE, "program source data is empty");
		return;
	}
	const usize entry_point_size = strlen(entry_point) + 1;
	char* new_entry_point = (char*)malloc(entry_point_size);
	RTGL_CHECK_ALLOC(new_entry_point, entry_point_size, "OpenGL program entry point");
	if (!new_entry_point) {
		return;
	}
	u08* new_source = (u08*)malloc((usize)size);
	RTGL_CHECK_ALLOC(new_source, (usize)size, "OpenGL RTSL program source");
	if (!new_source) {
		free(new_entry_point);
		return;
	}
	memcpy(new_entry_point, entry_point, entry_point_size);
	memcpy(new_source, data, (usize)size);
	free(internal->entry_point);
	free(internal->source_bytes);
	internal->entry_point = new_entry_point;
	internal->source_bytes = new_source;
	internal->source_size = size;
}

void rtgl_program_raster_state(struct rtgl_context* ctx, struct rtgl_program* internal, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) {
	internal->cull_mode = cull_mode;
	internal->front_face = front_face;
	internal->fill_mode = fill_mode;
}

void rtgl_program_blend_state(struct rtgl_context* ctx, struct rtgl_program* internal, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op) {
	internal->blend_enabled = enabled;
	internal->src_color_blend = src_color;
	internal->dst_color_blend = dst_color;
	internal->color_blend_op = color_op;
	internal->src_alpha_blend = src_alpha;
	internal->dst_alpha_blend = dst_alpha;
	internal->alpha_blend_op = alpha_op;
}

struct rt_location_t* rtgl_program_uniform_location(struct rtgl_context* ctx, struct rtgl_program* internal, const char* name) {
	(void)ctx;
	if (!internal || !name || !name[0]) {
		return NULL;
	}
	if (!internal->gl_program) {
		rtgl_throwf(RT_IMPROPER_USAGE, "program must be finalized before querying uniforms");
		return NULL;
	}
	for (u32 address = 1; address < RTGL_LOCATION_ADDRESS_COUNT; address++) {
		if (internal->location_occupied[address] && strcmp(internal->locations[address].name, name) == 0) {
			return &internal->locations[address];
		}
	}
	return NULL;
}

void rtgl_program_finish(struct rtgl_program* program) {
	rtgl_program_clear_locations(program);
	if (program->gl_program) {
		rtgl_execution_program_destroy(program->base.ctx, program);
	}
	free(program->entry_point);
	program->entry_point = NULL;
	free(program->source_bytes);
	program->source_bytes = NULL;
	program->source_size = 0;
	rtgl_finish_resource_base(RTGL_RESOURCE_BASE(program));
}

void rtgl_program_prepare(struct rtgl_context* ctx, struct rtgl_program* program) {
	(void)ctx;
	if (!program || !program->gl_program) {
		rtgl_throwf(RT_IMPROPER_USAGE, "OpenGL program must be finalized before use");
	}
}

void rtgl_program_finalize(struct rtgl_context* ctx, struct rtgl_program* program) {
	if (!program || program->gl_program) {
		return;
	}
	rtgl_execution_program_finalize(ctx, program);
}
