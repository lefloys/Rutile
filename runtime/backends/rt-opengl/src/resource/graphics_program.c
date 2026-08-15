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

void rtGraphicsProgramLayout(rt_graphics_program program, const rt_vertex_layout* layout) {
	rtgl_graphics_program_layout(
		rtgl_get_current_context(),
		rtgl_graphics_program_from_handle(program),
		layout
	);
}

void rtGraphicsProgramSource(rt_graphics_program program, const void* data, usize size) {
	rtgl_graphics_program_source(
		rtgl_get_current_context(),
		rtgl_graphics_program_from_handle(program),
		data,
		size
	);
}

void rtGraphicsProgramRasterState(rt_graphics_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) {
	rtgl_graphics_program_raster_state(
		rtgl_get_current_context(),
		rtgl_graphics_program_from_handle(program),
		cull_mode,
		front_face,
		fill_mode
	);
}

void rtGraphicsProgramBlendState(rt_graphics_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op) {
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

rt_location rtGraphicsProgramLocation(rt_graphics_program program, const char* name) {
	return (rt_location)rtgl_graphics_program_uniform_location(rtgl_get_current_context(), rtgl_graphics_program_from_handle(program), name);
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
	if (!layout || !layout->attributes || layout->attribute_count == 0) {
		internal->vertex_layout = (rt_vertex_layout){ 0 };
		return;
	}
	if (layout->attribute_count > RTGL_MAX_VERTEX_ATTRIBUTES || layout->stream_count > 16) {
		rtgl_throwf(RT_IMPROPER_USAGE, "too many vertex attributes");
		return;
	}
	memcpy(internal->vertex_attributes, layout->attributes, sizeof(layout->attributes[0]) * layout->attribute_count);
	memcpy(internal->vertex_streams, layout->streams, sizeof(layout->streams[0]) * layout->stream_count);
	internal->vertex_layout.streams = internal->vertex_streams;
	internal->vertex_layout.attributes = internal->vertex_attributes;
	internal->vertex_layout.stream_count = layout->stream_count;
	internal->vertex_layout.attribute_count = layout->attribute_count;
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
	for (u32 i = 0; i < internal->vertex_layout.attribute_count; i++) {
		const rt_vertex_attribute* attribute = &internal->vertex_attributes[i];
		if (strcmp(attribute->name, name) != 0 || internal->uniform_location_count == 16) {
			continue;
		}
		rtgl_uniform_location* location = &internal->uniform_locations[internal->uniform_location_count++];
		memset(location, 0, sizeof(*location));
		location->program = internal;
		strncpy(location->name, name, sizeof(location->name) - 1);
		location->binding = (u32)attribute->stream;
		location->gl_location = -1;
		location->kind = RTGL_UNIFORM_LOCATION_VERTEX_STREAM;
		return location;
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
