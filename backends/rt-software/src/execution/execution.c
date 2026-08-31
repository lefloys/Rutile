#include "execution.h"

#include "error.h"
#include "interpreter/execute.h"

#include <stdlib.h>
#include <string.h>

static void rtsw_execute_buffer_data(struct rtsw_ir_buffer_data* command) {
	memcpy(command->buffer->bytes + command->range.offset, command->data, command->range.size);
}

static void rtsw_execute_buffer_copy(struct rtsw_ir_buffer_copy* command) {
	memmove(
		command->dst->bytes + command->dst_range.offset,
		command->src->bytes + command->src_range.offset,
		command->src_range.size
	);
}

static void rtsw_execute_buffer_copy_to_texture(struct rtsw_ir_buffer_copy_to_texture* command) {
	rtsw_texture_write(command->dst, command->dst_range, command->src->bytes + command->src_range.offset);
}

static void rtsw_execute_texture_data(struct rtsw_ir_texture_data* command) {
	rtsw_texture_write(command->texture, command->range, command->data);
}

static void rtsw_execute_texture_copy(struct rtsw_ir_texture_copy* command) {
	usize byte_size = rtsw_texture_range_byte_size(command->src, command->src_range);
	u08* bytes = malloc(byte_size);
	if (!bytes) return;
	rtsw_texture_read(command->src, command->src_range, bytes);
	rtsw_texture_write(command->dst, command->dst_range, bytes);
	free(bytes);
}

static void rtsw_execute_texture_copy_to_buffer(struct rtsw_ir_texture_copy_to_buffer* command) {
	rtsw_texture_read(command->src, command->src_range, command->dst->bytes + command->dst_range.offset);
}

static u08 rtsw_execute_unorm8(f32 value) {
	if (value <= 0.0f) {
		return 0;
	}
	if (value >= 1.0f) {
		return 255;
	}
	return (u08)(value * 255.0f + 0.5f);
}

static f32 rtsw_execute_blend_factor(enum rt_blend_factor factor, f32 source, f32 destination, f32 source_alpha, f32 destination_alpha) {
	switch (factor) {
	case RT_BLEND_ZERO: return 0.0f;
	case RT_BLEND_ONE: return 1.0f;
	case RT_BLEND_SRC_COLOR: return source;
	case RT_BLEND_ONE_MINUS_SRC_COLOR: return 1.0f - source;
	case RT_BLEND_DST_COLOR: return destination;
	case RT_BLEND_ONE_MINUS_DST_COLOR: return 1.0f - destination;
	case RT_BLEND_SRC_ALPHA: return source_alpha;
	case RT_BLEND_ONE_MINUS_SRC_ALPHA: return 1.0f - source_alpha;
	case RT_BLEND_DST_ALPHA: return destination_alpha;
	case RT_BLEND_ONE_MINUS_DST_ALPHA: return 1.0f - destination_alpha;
	default: return 0.0f;
	}
}

static f32 rtsw_execute_blend_operation(enum rt_blend_op operation, f32 source, f32 destination) {
	switch (operation) {
	case RT_BLEND_OP_ADD: return source + destination;
	case RT_BLEND_OP_SUBTRACT: return source - destination;
	case RT_BLEND_OP_REVERSE_SUBTRACT: return destination - source;
	case RT_BLEND_OP_MIN: return source < destination ? source : destination;
	case RT_BLEND_OP_MAX: return source > destination ? source : destination;
	default: return 0.0f;
	}
}

static void rtsw_execute_clear_color(struct rtsw_framebuffer* framebuffer, const struct rtsw_ir_clear_color* command) {
	struct rtsw_texture_view* view;
	struct rtsw_texture* texture;
	usize pixel_count;
	usize pixel;
	u08 color[4];

	if (!framebuffer || !(view = framebuffer->color_views[0]) || !(texture = view->texture)) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdClearColor requires framebuffer color attachment zero");
		return;
	}
	if (texture->format != RT_RGBA8_UNORM) {
		rtsw_throwf(RT_FEATURE_NOT_SUPPORTED, "rt-software clear currently requires RT_RGBA8_UNORM color attachment");
		return;
	}

	color[0] = rtsw_execute_unorm8(command->r);
	color[1] = rtsw_execute_unorm8(command->g);
	color[2] = rtsw_execute_unorm8(command->b);
	color[3] = rtsw_execute_unorm8(command->a);
	pixel_count = texture->extent.width * texture->extent.height * texture->extent.depth;
	for (pixel = 0; pixel != pixel_count; ++pixel) {
		memcpy(texture->bytes + pixel * sizeof(color), color, sizeof(color));
	}
}

static void rtsw_execute_clear_depth(struct rtsw_framebuffer* framebuffer, f32 depth) {
	struct rtsw_texture* texture = framebuffer && framebuffer->depth_view ? framebuffer->depth_view->texture : NULL;
	usize pixel_count;
	if (!texture || texture->format != RT_D32_SFLOAT) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdClear requires an RT_D32_SFLOAT depth attachment");
		return;
	}
	pixel_count = texture->extent.width * texture->extent.height;
	for (usize pixel = 0; pixel < pixel_count; ++pixel) {
		memcpy(texture->bytes + pixel * sizeof(depth), &depth, sizeof(depth));
	}
}

static void rtsw_execute_clear_stencil(struct rtsw_framebuffer* framebuffer, usize stencil) {
	struct rtsw_texture* texture = framebuffer && framebuffer->stencil_view ? framebuffer->stencil_view->texture : NULL;
	if (!texture || texture->format != RT_S8_UINT) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdClear requires an RT_S8_UINT stencil attachment");
		return;
	}
	memset(texture->bytes, (u08)stencil, texture->byte_size);
}

static usize rtsw_execute_float_components(enum rt_format format) {
	switch (format) {
	case RT_R32_SFLOAT: return 1;
	case RT_RG32_SFLOAT: return 2;
	case RT_RGB32_SFLOAT: return 3;
	case RT_RGBA32_SFLOAT: return 4;
	default: return 0;
	}
}

static bool rtsw_execute_vertex_value(
	const struct rtsw_program* program,
	const struct rtsw_ir_vertex_buffer* buffers,
	usize buffer_count,
	usize vertex,
	usize instance,
	struct rtsw_rtir_value* value
) {
	const struct rtsw_rtir_type* type = rtsw_rtir_program_type(&program->module, program->vertex_input_type);
	usize component = 0;
	u32 member_index;
	if (!type || type->kind != 7) return false;
	memset(value, 0, sizeof(*value));
	value->type = type->id;
	for (member_index = 0; member_index != type->member_count; ++member_index) {
		const char* name = program->module.strings[type->members[member_index].name];
		usize input_index;
		bool found = false;
		for (input_index = 0; input_index != program->vertex_input_count; ++input_index) {
			const rt_vertex_input* input = &program->vertex_inputs[input_index];
			usize attribute_index;
			if (input_index >= buffer_count || !buffers[input_index].buffer) return false;
			for (attribute_index = 0; attribute_index != input->attribute_count; ++attribute_index) {
				const rt_vertex_attribute* attribute = &input->attributes[attribute_index];
				usize count;
				usize byte_offset;
				if (!attribute->name || strcmp(attribute->name, name) != 0) continue;
				count = rtsw_execute_float_components(attribute->format);
				byte_offset = buffers[input_index].range.offset + (input->rate == RT_VERTEX_RATE_INSTANCE ? instance : vertex) * input->stride + attribute->offset;
				if (!count || component + count > RTSW_RTIR_MAX_COMPONENTS || byte_offset > buffers[input_index].range.offset + buffers[input_index].range.size || count * sizeof(f32) > buffers[input_index].range.offset + buffers[input_index].range.size - byte_offset) return false;
				memcpy(value->components + component, buffers[input_index].buffer->bytes + byte_offset, count * sizeof(f32));
				component += count;
				found = true;
				break;
			}
			if (found) break;
		}
		if (!found) return false;
	}
	return true;
}

static bool rtsw_execute_vertices(
	const struct rtsw_program* program,
	const struct rtsw_rtir_execution_context* context,
	const struct rtsw_ir_vertex_buffer* buffers,
	usize buffer_count,
	const struct rtsw_ir_draw* draw,
	usize instance,
	struct rtsw_rtir_value* outputs
) {
	usize index;
	for (index = 0; index != draw->vertex_count; ++index) {
		struct rtsw_rtir_value input;
		if (!rtsw_execute_vertex_value(program, buffers, buffer_count, draw->first_vertex + index, instance, &input) ||
			!rtsw_rtir_execute_function_with_context(&program->module, program->vertex_function, context, &input, 1, &outputs[index])) return false;
	}
	return true;
}

static f32 rtsw_execute_edge(f32 ax, f32 ay, f32 bx, f32 by, f32 px, f32 py) {
	return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

static void rtsw_execute_rasterize_unclipped_triangle(
	struct rtsw_framebuffer* framebuffer,
	const struct rtsw_program* program,
	const struct rtsw_rtir_execution_context* context,
	const struct rtsw_rtir_value* first,
	const struct rtsw_rtir_value* second,
	const struct rtsw_rtir_value* third,
	const struct rtsw_ir_rectangle* viewport,
	const struct rtsw_ir_rectangle* scissor
) {
	struct rtsw_texture_view* view = framebuffer->color_views[0];
	struct rtsw_texture* texture = view ? view->texture : NULL;
	struct rtsw_texture* depth_texture = framebuffer->depth_view ? framebuffer->depth_view->texture : NULL;
	f32 x0;
	f32 y0;
	f32 x1;
	f32 y1;
	f32 x2;
	f32 y2;
	f32 area;
	f32 ndc_area;
	usize viewport_x;
	usize viewport_y;
	usize viewport_width;
	usize viewport_height;
	usize scissor_x;
	usize scissor_y;
	usize scissor_width;
	usize scissor_height;
	usize y;
	if (!texture || texture->format != RT_RGBA8_UNORM) {
		rtsw_throwf(RT_FEATURE_NOT_SUPPORTED, "rt-software rasterization requires an RT_RGBA8_UNORM color attachment");
		return;
	}
	if (depth_texture && depth_texture->format != RT_D32_SFLOAT) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rt-software rasterization requires RT_D32_SFLOAT for a depth attachment");
		return;
	}
	if (first->components[3] == 0.0f || second->components[3] == 0.0f || third->components[3] == 0.0f) return;
	viewport_x = viewport->x > texture->extent.width ? texture->extent.width : viewport->x;
	viewport_y = viewport->y > texture->extent.height ? texture->extent.height : viewport->y;
	viewport_width = viewport->width > texture->extent.width - viewport_x ? texture->extent.width - viewport_x : viewport->width;
	viewport_height = viewport->height > texture->extent.height - viewport_y ? texture->extent.height - viewport_y : viewport->height;
	scissor_x = scissor->x > texture->extent.width ? texture->extent.width : scissor->x;
	scissor_y = scissor->y > texture->extent.height ? texture->extent.height : scissor->y;
	scissor_width = scissor->width > texture->extent.width - scissor_x ? texture->extent.width - scissor_x : scissor->width;
	scissor_height = scissor->height > texture->extent.height - scissor_y ? texture->extent.height - scissor_y : scissor->height;
	x0 = (f32)viewport_x + (first->components[0] / first->components[3] * 0.5f + 0.5f) * (f32)viewport_width;
	y0 = (f32)viewport_y + (1.0f - (first->components[1] / first->components[3] * 0.5f + 0.5f)) * (f32)viewport_height;
	x1 = (f32)viewport_x + (second->components[0] / second->components[3] * 0.5f + 0.5f) * (f32)viewport_width;
	y1 = (f32)viewport_y + (1.0f - (second->components[1] / second->components[3] * 0.5f + 0.5f)) * (f32)viewport_height;
	x2 = (f32)viewport_x + (third->components[0] / third->components[3] * 0.5f + 0.5f) * (f32)viewport_width;
	y2 = (f32)viewport_y + (1.0f - (third->components[1] / third->components[3] * 0.5f + 0.5f)) * (f32)viewport_height;
	area = rtsw_execute_edge(x0, y0, x1, y1, x2, y2);
	if (area == 0.0f) return;
	ndc_area = (second->components[0] / second->components[3] - first->components[0] / first->components[3]) *
		(third->components[1] / third->components[3] - first->components[1] / first->components[3]) -
		(second->components[1] / second->components[3] - first->components[1] / first->components[3]) *
		(third->components[0] / third->components[3] - first->components[0] / first->components[3]);
	if ((program->cull_mode == RT_CULL_FRONT && ((program->front_face == RT_FRONT_FACE_CCW) == (ndc_area > 0.0f))) ||
		(program->cull_mode == RT_CULL_BACK && ((program->front_face == RT_FRONT_FACE_CCW) != (ndc_area > 0.0f)))) return;
	for (y = viewport_y > scissor_y ? viewport_y : scissor_y;
		y < viewport_y + viewport_height && y < scissor_y + scissor_height; ++y) {
		usize x;
		for (x = viewport_x > scissor_x ? viewport_x : scissor_x;
			x < viewport_x + viewport_width && x < scissor_x + scissor_width; ++x) {
			f32 px = (f32)x + 0.5f;
			f32 py = (f32)y + 0.5f;
			f32 a = rtsw_execute_edge(x1, y1, x2, y2, px, py) / area;
			f32 b = rtsw_execute_edge(x2, y2, x0, y0, px, py) / area;
			f32 c = 1.0f - a - b;
			struct rtsw_rtir_value input = { 0 };
			struct rtsw_rtir_value color = { 0 };
			usize component;
			if (a < 0.0f || b < 0.0f || c < 0.0f ||
				(program->fill_mode == RT_FILL_WIREFRAME && a > 0.02f && b > 0.02f && c > 0.02f)) continue;
			input.type = first->type;
			{
				f32 inverse_w = a / first->components[3] + b / second->components[3] + c / third->components[3];
				if (inverse_w == 0.0f) continue;
	for (component = 0; component != RTSW_RTIR_MAX_COMPONENTS; ++component) {
					input.components[component] = (first->components[component] * a / first->components[3] +
						second->components[component] * b / second->components[3] +
						third->components[component] * c / third->components[3]) / inverse_w;
				}
			}
			if (!rtsw_rtir_execute_function_with_context(&program->module, program->fragment_function, context, &input, 1, &color)) {
				rtsw_throwf(RT_FEATURE_NOT_SUPPORTED, "rt-software could not execute RTIR fragment stage");
				return;
			}
			{
				u08* destination = texture->bytes + (y * texture->extent.width + x) * 4;
				f32 depth = viewport->min_depth + (input.components[2] / input.components[3]) * (viewport->max_depth - viewport->min_depth);
				f32 values[4] = { color.components[0], color.components[1], color.components[2], color.components[3] };
				if (depth_texture) {
					f32* destination_depth = (f32*)(depth_texture->bytes + (y * depth_texture->extent.width + x) * sizeof(f32));
					if (depth >= *destination_depth) continue;
					*destination_depth = depth;
				}
				if (program->blend_enabled) {
					for (component = 0; component != 4; ++component) {
						f32 dst = (f32)destination[component] / 255.0f;
						enum rt_blend_factor src_factor = component == 3 ? program->src_alpha_blend : program->src_color_blend;
						enum rt_blend_factor dst_factor = component == 3 ? program->dst_alpha_blend : program->dst_color_blend;
						enum rt_blend_op operation = component == 3 ? program->alpha_blend_op : program->color_blend_op;
						values[component] = rtsw_execute_blend_operation(
							operation,
							values[component] * rtsw_execute_blend_factor(src_factor, values[component], dst, values[3], (f32)destination[3] / 255.0f),
							dst * rtsw_execute_blend_factor(dst_factor, values[component], dst, values[3], (f32)destination[3] / 255.0f)
						);
					}
				}
				for (component = 0; component != 4; ++component) {
					destination[component] = rtsw_execute_unorm8(values[component]);
				}
			}
		}
	}
}

static f32 rtsw_execute_clip_distance(const struct rtsw_rtir_value* vertex, usize plane) {
	switch (plane) {
	case 0: return vertex->components[0] + vertex->components[3];
	case 1: return vertex->components[3] - vertex->components[0];
	case 2: return vertex->components[1] + vertex->components[3];
	case 3: return vertex->components[3] - vertex->components[1];
	case 4: return vertex->components[2];
	default: return vertex->components[3] - vertex->components[2];
	}
}

static struct rtsw_rtir_value rtsw_execute_clip_intersection(
	const struct rtsw_rtir_value* first,
	const struct rtsw_rtir_value* second,
	f32 first_distance,
	f32 second_distance
) {
	struct rtsw_rtir_value result = { 0 };
	f32 amount = first_distance / (first_distance - second_distance);
	result.type = first->type;
	for (usize component = 0; component != RTSW_RTIR_MAX_COMPONENTS; ++component) {
		result.components[component] = first->components[component] + amount * (second->components[component] - first->components[component]);
	}
	return result;
}

static void rtsw_execute_rasterize_triangle(
	struct rtsw_framebuffer* framebuffer,
	const struct rtsw_program* program,
	const struct rtsw_rtir_execution_context* context,
	const struct rtsw_rtir_value* first,
	const struct rtsw_rtir_value* second,
	const struct rtsw_rtir_value* third,
	const struct rtsw_ir_rectangle* viewport,
	const struct rtsw_ir_rectangle* scissor
) {
	struct rtsw_rtir_value vertices[16];
	struct rtsw_rtir_value clipped[16];
	usize vertex_count = 3;
	vertices[0] = *first;
	vertices[1] = *second;
	vertices[2] = *third;
	for (usize plane = 0; plane != 6 && vertex_count; ++plane) {
		usize clipped_count = 0;
		for (usize index = 0; index != vertex_count; ++index) {
			const struct rtsw_rtir_value* previous = &vertices[(index + vertex_count - 1) % vertex_count];
			const struct rtsw_rtir_value* current = &vertices[index];
			f32 previous_distance = rtsw_execute_clip_distance(previous, plane);
			f32 current_distance = rtsw_execute_clip_distance(current, plane);
			bool previous_inside = previous_distance >= 0.0f;
			bool current_inside = current_distance >= 0.0f;
			if (previous_inside != current_inside) {
				clipped[clipped_count++] = rtsw_execute_clip_intersection(previous, current, previous_distance, current_distance);
			}
			if (current_inside) clipped[clipped_count++] = *current;
		}
		memcpy(vertices, clipped, clipped_count * sizeof(*vertices));
		vertex_count = clipped_count;
	}
	for (usize index = 1; index + 1 < vertex_count; ++index) {
		rtsw_execute_rasterize_unclipped_triangle(framebuffer, program, context, &vertices[0], &vertices[index], &vertices[index + 1], viewport, scissor);
	}
}

static bool rtsw_execute_draw(struct rtsw_framebuffer* framebuffer, const struct rtsw_program* program,
	const struct rtsw_rtir_execution_context* context,
	const struct rtsw_ir_vertex_buffer* buffers, usize buffer_count, const struct rtsw_ir_draw* draw,
	usize instance, const struct rtsw_ir_rectangle* viewport, const struct rtsw_ir_rectangle* scissor) {
	struct rtsw_rtir_value* vertices;
	usize index;
	if (draw->vertex_count % 3 != 0) return false;
	vertices = calloc(draw->vertex_count, sizeof(*vertices));
	if (!vertices) return false;
	if (!rtsw_execute_vertices(program, context, buffers, buffer_count, draw, instance, vertices)) {
		free(vertices);
		return false;
	}
	for (index = 0; index != draw->vertex_count; index += 3)
		rtsw_execute_rasterize_triangle(framebuffer, program, context, &vertices[index], &vertices[index + 1], &vertices[index + 2], viewport, scissor);
	free(vertices);
	return true;
}

static bool rtsw_execute_draw_indexed(struct rtsw_framebuffer* framebuffer, const struct rtsw_program* program,
	const struct rtsw_rtir_execution_context* context,
	const struct rtsw_ir_vertex_buffer* buffers, usize buffer_count, const struct rtsw_ir_index_buffer* index_buffer,
	const struct rtsw_ir_draw_indexed* draw, usize instance, const struct rtsw_ir_rectangle* viewport, const struct rtsw_ir_rectangle* scissor) {
	usize index_size = index_buffer->format == RT_INDEX_U16 ? sizeof(u16) : sizeof(u32);
	struct rtsw_rtir_value* vertices;
	if (!index_buffer->buffer || draw->first_index > index_buffer->range.size / index_size ||
		draw->index_count > index_buffer->range.size / index_size - draw->first_index) return false;
	vertices = calloc(draw->index_count, sizeof(*vertices));
	if (!vertices) return false;
	for (usize index = 0; index < draw->index_count; ++index) {
		usize offset = index_buffer->range.offset + (draw->first_index + index) * index_size;
		usize vertex;
		if (index_buffer->format == RT_INDEX_U16) {
			u16 value;
			memcpy(&value, index_buffer->buffer->bytes + offset, sizeof(value));
			vertex = value;
		} else {
			u32 value;
			memcpy(&value, index_buffer->buffer->bytes + offset, sizeof(value));
			vertex = value;
		}
		if (vertex > SIZE_MAX - draw->vertex_offset || !rtsw_execute_vertex_value(program, buffers, buffer_count,
			vertex + draw->vertex_offset, instance, &vertices[index]) || !rtsw_rtir_execute_function_with_context(&program->module,
			program->vertex_function, context, &vertices[index], 1, &vertices[index])) {
			free(vertices);
			return false;
		}
	}
	for (usize index = 0; index < draw->index_count; index += 3)
		rtsw_execute_rasterize_triangle(framebuffer, program, context, &vertices[index], &vertices[index + 1], &vertices[index + 2], viewport, scissor);
	free(vertices);
	return true;
}

struct rtsw_execution_state {
	struct rtsw_buffer_binding {
		struct rtsw_program* program;
		struct rtsw_buffer* buffer;
		rt_buffer_range range;
		u32 symbol;
	} *buffer_bindings;
	usize buffer_binding_count;
	struct rtsw_framebuffer* framebuffer;
	struct rtsw_program* program;
	struct rtsw_ir_vertex_buffer* vertex_buffers;
	usize vertex_buffer_count;
	struct rtsw_ir_index_buffer index_buffer;
	struct rtsw_ir_rectangle viewport;
	struct rtsw_ir_rectangle scissor;
	struct rtsw_ir_clear_color clear_color;
	f32 clear_depth;
	usize clear_stencil;
};

static bool rtsw_execute_resource_load(void* user_data, u32 symbol, u32 type, struct rtsw_rtir_value* value) {
	struct rtsw_execution_state* state = user_data;
	usize component_count;
	usize index;
	if (!state->program || !(component_count = rtsw_rtir_value_component_count(&state->program->module, type)) ||
		component_count > RTSW_RTIR_MAX_COMPONENTS) return false;
	for (index = state->buffer_binding_count; index != 0; --index) {
		const struct rtsw_buffer_binding* binding = &state->buffer_bindings[index - 1];
		if (binding->program == state->program && binding->symbol == symbol &&
			binding->range.size >= component_count * sizeof(f32)) {
			value->type = type;
			memcpy(value->components, binding->buffer->bytes + binding->range.offset, component_count * sizeof(f32));
			return true;
		}
	}
	return false;
}

static bool rtsw_execute_bind_buffer(struct rtsw_execution_state* state, const struct rtsw_ir_bind_buffer* command) {
	usize index;
	for (index = 0; index != state->buffer_binding_count; ++index) {
		struct rtsw_buffer_binding* binding = &state->buffer_bindings[index];
		if (binding->program == command->program && binding->symbol == command->symbol) {
			*binding = (struct rtsw_buffer_binding){ command->program, command->buffer, command->range, command->symbol };
			return true;
		}
	}
	{
		struct rtsw_buffer_binding* bindings = realloc(state->buffer_bindings, (state->buffer_binding_count + 1) * sizeof(*bindings));
		if (!bindings) return false;
		state->buffer_bindings = bindings;
		state->buffer_bindings[state->buffer_binding_count++] = (struct rtsw_buffer_binding){ command->program, command->buffer, command->range, command->symbol };
	}
	return true;
}

static bool rtsw_execute_command_stream(struct rtsw_command_buffer* command_buffer, struct rtsw_execution_state* state) {
	usize offset = 0;
	while (offset < command_buffer->ir_size) {
		struct rtsw_command_header* header = (struct rtsw_command_header*)(command_buffer->ir_data + offset);
		void* payload = header + 1;
		switch ((enum rtsw_command_opcode)header->opcode) {
		case RTSW_COMMAND_BUFFER_DATA:
			rtsw_execute_buffer_data(payload);
			break;
		case RTSW_COMMAND_BUFFER_COPY:
			rtsw_execute_buffer_copy(payload);
			break;
		case RTSW_COMMAND_BUFFER_COPY_TO_TEXTURE:
			rtsw_execute_buffer_copy_to_texture(payload);
			break;
		case RTSW_COMMAND_TEXTURE_DATA:
			rtsw_execute_texture_data(payload);
			break;
		case RTSW_COMMAND_TEXTURE_COPY:
			rtsw_execute_texture_copy(payload);
			break;
		case RTSW_COMMAND_TEXTURE_COPY_TO_BUFFER:
			rtsw_execute_texture_copy_to_buffer(payload);
			break;
		case RTSW_COMMAND_BEGIN_RENDERING:
			state->framebuffer = ((struct rtsw_ir_begin_rendering*)payload)->framebuffer;
			break;
		case RTSW_COMMAND_END_RENDERING:
			state->framebuffer = NULL;
			break;
		case RTSW_COMMAND_CLEAR_COLOR:
			state->clear_color = *(struct rtsw_ir_clear_color*)payload;
			break;
		case RTSW_COMMAND_CLEAR_DEPTH:
			state->clear_depth = ((struct rtsw_ir_clear_depth*)payload)->depth;
			break;
		case RTSW_COMMAND_CLEAR_STENCIL:
			state->clear_stencil = ((struct rtsw_ir_clear_stencil*)payload)->stencil;
			break;
		case RTSW_COMMAND_CLEAR:
			if (((struct rtsw_ir_clear*)payload)->attachments & RT_CLEAR_COLOR) {
				rtsw_execute_clear_color(state->framebuffer, &state->clear_color);
			}
			if (((struct rtsw_ir_clear*)payload)->attachments & RT_CLEAR_DEPTH) {
				rtsw_execute_clear_depth(state->framebuffer, state->clear_depth);
			}
			if (((struct rtsw_ir_clear*)payload)->attachments & RT_CLEAR_STENCIL) {
				rtsw_execute_clear_stencil(state->framebuffer, state->clear_stencil);
			}
			break;
		case RTSW_COMMAND_SET_VIEWPORT:
			state->viewport = *(struct rtsw_ir_rectangle*)payload;
			break;
		case RTSW_COMMAND_SET_SCISSOR:
			state->scissor = *(struct rtsw_ir_rectangle*)payload;
			break;
		case RTSW_COMMAND_EXECUTE:
			if (!rtsw_execute_command_stream(((struct rtsw_ir_execute*)payload)->secondary, state)) {
				return false;
			}
			break;
		case RTSW_COMMAND_USE_PROGRAM:
			state->program = ((struct rtsw_ir_use_program*)payload)->program;
			break;
		case RTSW_COMMAND_BIND_BUFFER:
			if (!rtsw_execute_bind_buffer(state, payload)) {
				rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate rt-software buffer binding state");
				return false;
			}
			break;
		case RTSW_COMMAND_VERTEX_BUFFER:
			if (((struct rtsw_ir_vertex_buffer*)payload)->input >= state->vertex_buffer_count) {
				usize required = (usize)((struct rtsw_ir_vertex_buffer*)payload)->input + 1;
				struct rtsw_ir_vertex_buffer* buffers = realloc(state->vertex_buffers, required * sizeof(*buffers));
				if (!buffers) {
					rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate rt-software vertex binding state");
					return false;
				}
				memset(buffers + state->vertex_buffer_count, 0, (required - state->vertex_buffer_count) * sizeof(*buffers));
				state->vertex_buffers = buffers;
				state->vertex_buffer_count = required;
			}
			state->vertex_buffers[((struct rtsw_ir_vertex_buffer*)payload)->input] = *(struct rtsw_ir_vertex_buffer*)payload;
			break;
		case RTSW_COMMAND_INDEX_BUFFER:
			state->index_buffer = *(struct rtsw_ir_index_buffer*)payload;
			break;
		case RTSW_COMMAND_DRAW:
			{
				const struct rtsw_rtir_execution_context context = { state, rtsw_execute_resource_load };
			if (!state->framebuffer || !state->program) {
				rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdDraw requires a framebuffer and program at execution time");
				return false;
			}
			if (state->program->vertex_input_count > state->vertex_buffer_count) {
				rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdDraw is missing one or more program vertex-buffer bindings");
				return false;
			}
			if (!rtsw_execute_draw(state->framebuffer, state->program, &context, state->vertex_buffers, state->vertex_buffer_count, payload, 0, &state->viewport, &state->scissor)) {
				rtsw_throwf(RT_FEATURE_NOT_SUPPORTED, "rt-software could not fetch or execute the requested RTIR vertices");
				return false;
			}
			break;
			}
		case RTSW_COMMAND_DRAW_INSTANCED: {
			struct rtsw_ir_draw_instanced* draw = payload;
			struct rtsw_ir_draw one = { draw->vertex_count, draw->first_vertex };
			const struct rtsw_rtir_execution_context context = { state, rtsw_execute_resource_load };
			if (!state->framebuffer || !state->program || state->program->vertex_input_count > state->vertex_buffer_count) {
				rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdDrawInstanced has incomplete CPU rendering state");
				return false;
			}
			for (usize instance = 0; instance < draw->instance_count; ++instance) {
				if (!rtsw_execute_draw(state->framebuffer, state->program, &context, state->vertex_buffers, state->vertex_buffer_count, &one,
					draw->first_instance + instance, &state->viewport, &state->scissor)) return false;
			}
			break;
		}
		case RTSW_COMMAND_DRAW_INDEXED: {
			const struct rtsw_rtir_execution_context context = { state, rtsw_execute_resource_load };
			if (!state->framebuffer || !state->program || !state->index_buffer.buffer || state->program->vertex_input_count > state->vertex_buffer_count ||
				!rtsw_execute_draw_indexed(state->framebuffer, state->program, &context, state->vertex_buffers, state->vertex_buffer_count,
					&state->index_buffer, payload, 0, &state->viewport, &state->scissor)) {
				rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdDrawIndexed has incomplete or invalid CPU rendering state");
				return false;
			}
			break;
		}
		case RTSW_COMMAND_DRAW_INDEXED_INSTANCED: {
			struct rtsw_ir_draw_indexed_instanced* draw = payload;
			struct rtsw_ir_draw_indexed one = { draw->index_count, draw->first_index, draw->vertex_offset };
			const struct rtsw_rtir_execution_context context = { state, rtsw_execute_resource_load };
			if (!state->framebuffer || !state->program || !state->index_buffer.buffer || state->program->vertex_input_count > state->vertex_buffer_count) {
				rtsw_throwf(RT_IMPROPER_USAGE, "rtCmdDrawIndexedInstanced has incomplete CPU rendering state");
				return false;
			}
			for (usize instance = 0; instance < draw->instance_count; ++instance) {
				if (!rtsw_execute_draw_indexed(state->framebuffer, state->program, &context, state->vertex_buffers, state->vertex_buffer_count,
					&state->index_buffer, &one, draw->first_instance + instance, &state->viewport, &state->scissor)) return false;
			}
			break;
		}
		default:
			break;
		}
		offset += sizeof(*header) + rtsw_command_record_size((enum rtsw_command_opcode)header->opcode);
	}
	return true;
}

void rtsw_execute_command_buffer(struct rtsw_command_buffer* command_buffer) {
	struct rtsw_execution_state state = {
		.viewport = { 0, 0, (usize)-1, (usize)-1, 0.0f, 1.0f },
		.scissor = { 0, 0, (usize)-1, (usize)-1, 0.0f, 1.0f },
	};

	(void)rtsw_execute_command_stream(command_buffer, &state);
	free(state.buffer_bindings);
	free(state.vertex_buffers);
}
