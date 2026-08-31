#include "program.h"

#include "context.h"
#include "error.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static char* rtsw_program_duplicate_string(const char* source) {
	usize size = strlen(source) + 1;
	char* copy = malloc(size);

	if (copy) {
		memcpy(copy, source, size);
	}

	return copy;
}

static void rtsw_program_finish_layout(struct rtsw_program* program) {
	free(program->input_locations);
	free(program->vertex_attributes);
	free(program->vertex_inputs);
	program->input_locations = NULL;
	program->vertex_attributes = NULL;
	program->vertex_inputs = NULL;
	program->vertex_input_count = 0;
	program->vertex_attribute_count = 0;
}

static void rtsw_program_finish_uniform_locations(struct rtsw_program* program) {
	free(program->uniform_locations);
	program->uniform_locations = NULL;
	program->uniform_location_count = 0;
}

static const char* rtsw_program_symbol_name(const struct rtsw_program* program, u32 symbol) {
	u32 index;
	for (index = 0; index != program->module.symbol_count; ++index) {
		const struct rtsw_rtir_symbol* candidate = &program->module.symbols[index];
		if (candidate->id == symbol && candidate->name < program->module.string_count) {
			return program->module.strings[candidate->name];
		}
	}
	return NULL;
}

static bool rtsw_program_symbol_matches(const char* symbol_name, const char* name) {
	const char* unqualified_name;
	if (strcmp(symbol_name, name) == 0) return true;
	unqualified_name = strrchr(symbol_name, ':');
	return unqualified_name && unqualified_name > symbol_name && unqualified_name[-1] == ':' && strcmp(unqualified_name + 1, name) == 0;
}

static bool rtsw_program_has_uniform_location(const struct rtsw_program* program, u32 symbol) {
	usize index;
	for (index = 0; index != program->uniform_location_count; ++index) {
		if (program->uniform_locations[index].symbol == symbol) return true;
	}
	return false;
}

static bool rtsw_program_build_uniform_locations(struct rtsw_program* program) {
	struct rt_location_t* locations;
	usize capacity = program->module.resource_count + program->module.uniform_count;
	usize index;

	rtsw_program_finish_uniform_locations(program);
	if (capacity == 0) return true;
	locations = calloc(capacity, sizeof(*locations));
	if (!locations) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate program uniform locations");
		return false;
	}
	program->uniform_locations = locations;
	for (index = 0; index != program->module.resource_count; ++index) {
		u32 symbol = program->module.resources[index].symbol;
		if (!rtsw_program_symbol_name(program, symbol) || rtsw_program_has_uniform_location(program, symbol)) continue;
		locations[program->uniform_location_count++] = (struct rt_location_t){ program, (u32)program->uniform_location_count, symbol };
	}
	for (index = 0; index != program->module.uniform_count; ++index) {
		u32 symbol = program->module.uniforms[index].symbol;
		if (!rtsw_program_symbol_name(program, symbol) || rtsw_program_has_uniform_location(program, symbol)) continue;
		locations[program->uniform_location_count++] = (struct rt_location_t){ program, (u32)program->uniform_location_count, symbol };
	}
	return true;
}

static bool rtsw_program_copy_layout(struct rtsw_program* program, const rt_vertex_layout* layout) {
	rt_vertex_input* inputs = NULL;
	rt_vertex_attribute* attributes = NULL;
	struct rt_location_t* locations = NULL;
	usize attribute_count = 0;
	usize input_index;
	usize attribute_offset = 0;

	if (!layout) {
		rtsw_program_finish_layout(program);
		return true;
	}
	for (input_index = 0; input_index != layout->input_count; ++input_index) {
		if (!layout->inputs || !layout->inputs[input_index].attributes ||
			layout->inputs[input_index].attribute_count > SIZE_MAX - attribute_count) {
			rtsw_throwf(RT_IMPROPER_USAGE, "rtProgramSetLayout received invalid vertex input attributes");
			return false;
		}
		attribute_count += layout->inputs[input_index].attribute_count;
	}
	if (layout->input_count != 0) {
		inputs = calloc(layout->input_count, sizeof(*inputs));
		locations = calloc(layout->input_count, sizeof(*locations));
	}
	if (attribute_count != 0) {
		attributes = calloc(attribute_count, sizeof(*attributes));
	}
	if ((layout->input_count != 0 && (!inputs || !locations)) || (attribute_count != 0 && !attributes)) {
		free(locations);
		free(attributes);
		free(inputs);
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to copy program vertex layout");
		return false;
	}
	for (input_index = 0; input_index != layout->input_count; ++input_index) {
		const rt_vertex_input* source = &layout->inputs[input_index];
		inputs[input_index] = *source;
		inputs[input_index].attributes = attributes + attribute_offset;
		memcpy(attributes + attribute_offset, source->attributes, source->attribute_count * sizeof(*attributes));
		locations[input_index].program = program;
		locations[input_index].address = (u32)input_index;
		attribute_offset += source->attribute_count;
	}
	rtsw_program_finish_layout(program);
	program->vertex_inputs = inputs;
	program->vertex_attributes = attributes;
	program->input_locations = locations;
	program->vertex_input_count = layout->input_count;
	program->vertex_attribute_count = attribute_count;
	return true;
}

rt_program rtProgramCreate(void) {
	rtsw_clear_error();
	return rtsw_program_to_handle(rtsw_program_create(rtsw_get_current_context()));
}

void rtProgramDestroy(rt_program program) {
	rtsw_program_destroy(rtsw_program_from_handle(program));
}

void rtProgramSetLayout(rt_program program, const rt_vertex_layout* layout) {
	rtsw_clear_error();
	struct rtsw_program* software_program = rtsw_program_from_handle(program);
	if (!software_program || software_program->finalized) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtProgramSetLayout requires a configurable program");
		return;
	}
	if (!rtsw_program_copy_layout(software_program, layout)) {
		return;
	}
}

void rtProgramSource(rt_program program, const char* entry_point, const u08* bytes, usize byte_size) {
	rtsw_clear_error();
	struct rtsw_program* software_program = rtsw_program_from_handle(program);
	struct rtsw_rtir_program module;
	char* entry_point_copy;
	if (!software_program || software_program->finalized || !entry_point || !entry_point[0]) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtProgramSource requires a configurable program and entry point");
		return;
	}
	if (!rtsw_rtir_program_read(&module, bytes, byte_size)) {
		if (rtsw_error() == RT_SUCCESS) rtsw_throwf(RT_SHADER_LINK_FAILED, "program source is not a linked RTSLP artifact");
		return;
	}
	entry_point_copy = rtsw_program_duplicate_string(entry_point);
	if (!entry_point_copy) {
		rtsw_rtir_program_finish(&module);
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to retain program entry point");
		return;
	}
	free(software_program->entry_point);
	rtsw_rtir_program_finish(&software_program->module);
	software_program->module = module;
	software_program->entry_point = entry_point_copy;
}

void rtProgramSetRasterState(rt_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) {
	rtsw_clear_error();
	struct rtsw_program* software_program = rtsw_program_from_handle(program);
	if (!software_program || software_program->finalized) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtProgramSetRasterState requires a configurable program");
		return;
	}
	software_program->cull_mode = cull_mode;
	software_program->front_face = front_face;
	software_program->fill_mode = fill_mode;
}

void rtProgramSetBlendState(rt_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op) {
	rtsw_clear_error();
	struct rtsw_program* software_program = rtsw_program_from_handle(program);
	if (!software_program || software_program->finalized) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtProgramSetBlendState requires a configurable program");
		return;
	}
	software_program->blend_enabled = enabled;
	software_program->src_color_blend = src_color;
	software_program->dst_color_blend = dst_color;
	software_program->color_blend_op = color_op;
	software_program->src_alpha_blend = src_alpha;
	software_program->dst_alpha_blend = dst_alpha;
	software_program->alpha_blend_op = alpha_op;
}

void rtProgramFinalize(rt_program program) {
	rtsw_clear_error();
	struct rtsw_program* software_program = rtsw_program_from_handle(program);
	const struct rtsw_rtir_entry* vertex_entry;
	const struct rtsw_rtir_entry* fragment_entry;

	if (!software_program || software_program->finalized || !software_program->entry_point) {
		rtsw_throwf(RT_SHADER_LINK_FAILED, "program finalize requires vertex and fragment RTIR entry points with the selected source name");
		return;
	}
	vertex_entry = rtsw_rtir_program_entry(&software_program->module, software_program->entry_point, RTSW_RTIR_STAGE_VERTEX);
	fragment_entry = rtsw_rtir_program_entry(&software_program->module, software_program->entry_point, RTSW_RTIR_STAGE_FRAGMENT);
	if (!vertex_entry || !fragment_entry ||
		!rtsw_rtir_program_has_function(&software_program->module, vertex_entry->function) ||
		!rtsw_rtir_program_has_function(&software_program->module, fragment_entry->function)) {
		rtsw_throwf(RT_SHADER_LINK_FAILED, "program RTIR entries reference unavailable functions");
		return;
	}
	software_program->vertex_function = vertex_entry->function;
	software_program->fragment_function = fragment_entry->function;
	software_program->vertex_input_type = rtsw_rtir_program_function_parameter_type(&software_program->module, vertex_entry->function, 0);
	software_program->fragment_input_type = rtsw_rtir_program_function_parameter_type(&software_program->module, fragment_entry->function, 0);
	if (!software_program->vertex_input_type || !software_program->fragment_input_type) {
		rtsw_throwf(RT_SHADER_LINK_FAILED, "rt-software graphics entries require one typed parameter per stage");
		return;
	}
	if (!rtsw_program_build_uniform_locations(software_program)) {
		return;
	}
	software_program->finalized = true;
}

rt_location rtProgramUniformLocation(rt_program program, const char* name) {
	rtsw_clear_error();
	struct rtsw_program* software_program = rtsw_program_from_handle(program);
	usize index;
	if (!software_program || !name) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtProgramUniformLocation requires a program and name");
		return NULL;
	}
	if (!software_program->finalized) {
		rtsw_throwf(RT_IMPROPER_USAGE, "program must be finalized before querying locations");
		return NULL;
	}
	for (index = 0; index != software_program->uniform_location_count; ++index) {
		struct rt_location_t* location = &software_program->uniform_locations[index];
		const char* location_name = rtsw_program_symbol_name(software_program, location->symbol);
		if (location_name && rtsw_program_symbol_matches(location_name, name)) return location;
	}
	return NULL;
}

rt_location rtProgramInputLocation(rt_program program, const rt_vertex_attribute* attributes, usize attribute_count) {
	rtsw_clear_error();
	struct rtsw_program* software_program = rtsw_program_from_handle(program);
	usize input_index;
	if (!software_program || !software_program->finalized || !attributes || attribute_count == 0) {
		return NULL;
	}
	for (input_index = 0; input_index != software_program->vertex_input_count; ++input_index) {
		const rt_vertex_input* input = &software_program->vertex_inputs[input_index];
		usize attribute_index;
		if (input->attribute_count != attribute_count) {
			continue;
		}
		for (attribute_index = 0; attribute_index != attribute_count; ++attribute_index) {
			const rt_vertex_attribute* expected = &input->attributes[attribute_index];
			const rt_vertex_attribute* actual = &attributes[attribute_index];
			if (expected->offset != actual->offset || expected->format != actual->format ||
				!expected->name || !actual->name || strcmp(expected->name, actual->name) != 0) {
				break;
			}
		}
		if (attribute_index == attribute_count) {
			return &software_program->input_locations[input_index];
		}
	}
	return NULL;
}

rt_location rtProgramOutputLocation(rt_program program, const char* name) {
	rtsw_clear_error();
	(void)program;
	(void)name;
	rtsw_throwf(RT_FEATURE_NOT_SUPPORTED, "rt-software output locations are not interpreted yet");
	return NULL;
}

RTSW_DEFINE_HANDLE(program, rtsw_program)

void rtsw_program_init(struct rtsw_context* ctx, struct rtsw_program* program) {
	rtsw_init_resource_base(ctx, RTSW_RESOURCE_BASE(program), program, rtsw_program_finalize_resource);
	program->cull_mode = RT_CULL_NONE;
	program->front_face = RT_FRONT_FACE_CCW;
	program->fill_mode = RT_FILL_SOLID;
	program->src_color_blend = RT_BLEND_ONE;
	program->dst_color_blend = RT_BLEND_ZERO;
	program->color_blend_op = RT_BLEND_OP_ADD;
	program->src_alpha_blend = RT_BLEND_ONE;
	program->dst_alpha_blend = RT_BLEND_ZERO;
	program->alpha_blend_op = RT_BLEND_OP_ADD;
}

struct rtsw_program* rtsw_program_create(struct rtsw_context* ctx) {
	struct rtsw_program* program;
	if (!ctx) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtProgramCreate requires an rt-software context");
		return NULL;
	}
	program = RTSW_ALLOC_RESOURCE(struct rtsw_program);
	if (program) {
		rtsw_program_init(ctx, program);
	}
	return program;
}

void rtsw_program_destroy(struct rtsw_program* program) {
	if (program) {
		rtsw_resource_retire(RTSW_RESOURCE_BASE(program));
	}
}

void rtsw_program_finalize_resource(void* resource) {
	struct rtsw_program* program = resource;
	rtsw_rtir_program_finish(&program->module);
	rtsw_program_finish_layout(program);
	rtsw_program_finish_uniform_locations(program);
	free(program->entry_point);
	free(program);
}
