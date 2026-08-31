#include "execute.h"

#include <stdlib.h>
#include <string.h>

enum {
	RTSW_RTIR_CONSTANT_FLOATING = 3,
	RTSW_RTIR_CONSTANT_BOOLEAN = 1,
	RTSW_RTIR_CONSTANT_INTEGER = 2,
	RTSW_RTIR_ACCESS = 8,
	RTSW_RTIR_ADD = 9,
	RTSW_RTIR_SUBTRACT = 10,
	RTSW_RTIR_MULTIPLY = 11,
	RTSW_RTIR_DIVIDE = 12,
	RTSW_RTIR_REMAINDER = 13,
	RTSW_RTIR_NEGATE = 14,
	RTSW_RTIR_COMPARE_EQUAL = 15,
	RTSW_RTIR_COMPARE_NOT_EQUAL = 16,
	RTSW_RTIR_COMPARE_LESS = 17,
	RTSW_RTIR_COMPARE_LESS_EQUAL = 18,
	RTSW_RTIR_COMPARE_GREATER = 19,
	RTSW_RTIR_COMPARE_GREATER_EQUAL = 20,
	RTSW_RTIR_RESOURCE_LOAD = 34,
	RTSW_RTIR_CONSTRUCT = 26,
	RTSW_RTIR_CALL = 29,
	RTSW_RTIR_RETURN_VALUE = 4,
	RTSW_RTIR_MAX_VALUES = 1024,
	RTSW_RTIR_MAX_OPERANDS = 16,
};

struct rtsw_execute_reader {
	const u08* bytes;
	usize size;
	usize offset;
};

struct rtsw_execute_value {
	u32 id;
	struct rtsw_rtir_value value;
};

static bool rtsw_execute_u8(struct rtsw_execute_reader* reader, u08* value) {
	if (reader->offset == reader->size) return false;
	*value = reader->bytes[reader->offset++];
	return true;
}

static bool rtsw_execute_u32(struct rtsw_execute_reader* reader, u32* value) {
	u32 shift;
	*value = 0;
	for (shift = 0; shift != 32; shift += 8) {
		u08 byte;
		if (!rtsw_execute_u8(reader, &byte)) return false;
		*value |= (u32)byte << shift;
	}
	return true;
}

static bool rtsw_execute_skip(struct rtsw_execute_reader* reader, usize size) {
	if (size > reader->size - reader->offset) return false;
	reader->offset += size;
	return true;
}

static bool rtsw_execute_vector(struct rtsw_execute_reader* reader, u32* values, u32 capacity, u32* count) {
	u32 index;
	if (!rtsw_execute_u32(reader, count) || *count > capacity) return false;
	for (index = 0; index != *count; ++index)
		if (!rtsw_execute_u32(reader, &values[index])) return false;
	return true;
}

static usize rtsw_execute_component_count(const struct rtsw_rtir_program* program, u32 type) {
	const struct rtsw_rtir_type* descriptor = rtsw_rtir_program_type(program, type);
	usize count = 0;
	u32 index;
	if (!descriptor) return 0;
	if (descriptor->kind == 5 || descriptor->kind == 9) return descriptor->element_count * rtsw_execute_component_count(program, descriptor->element_type);
	if (descriptor->kind == 6) return descriptor->element_count * rtsw_execute_component_count(program, descriptor->element_type);
	if (descriptor->kind != 7) return 1;
	for (index = 0; index != descriptor->member_count; ++index)
		count += rtsw_execute_component_count(program, descriptor->members[index].type);
	return count;
}

static struct rtsw_rtir_value* rtsw_execute_find_value(struct rtsw_execute_value* values, u32 count, u32 id) {
	u32 index;
	for (index = 0; index != count; ++index)
		if (values[index].id == id) return &values[index].value;
	return NULL;
}

static bool rtsw_execute_construct(
	const struct rtsw_rtir_program* program,
	struct rtsw_rtir_value* result,
	struct rtsw_execute_value* values,
	u32 value_count,
	const u32* operands,
	u32 operand_count
) {
	u32 operand_index;
	usize component = 0;
	for (operand_index = 0; operand_index != operand_count; ++operand_index) {
		struct rtsw_rtir_value* operand = rtsw_execute_find_value(values, value_count, operands[operand_index]);
		usize count;
		if (!operand || (count = rtsw_execute_component_count(program, operand->type)) == 0 || component + count > RTSW_RTIR_MAX_COMPONENTS) return false;
		memcpy(result->components + component, operand->components, count * sizeof(f32));
		component += count;
	}
	return component == rtsw_execute_component_count(program, result->type);
}

static bool rtsw_execute_access(
	const struct rtsw_rtir_program* program,
	struct rtsw_rtir_value* result,
	struct rtsw_execute_value* values,
	u32 value_count,
	const u32* operands,
	u32 operand_count,
	const u32* immediates,
	u32 immediate_count
) {
	struct rtsw_rtir_value* base;
	const struct rtsw_rtir_type* descriptor;
	usize component = 0;
	u32 index;
	if (operand_count != 1 || immediate_count != 1 || !(base = rtsw_execute_find_value(values, value_count, operands[0])) ||
		!(descriptor = rtsw_rtir_program_type(program, base->type))) return false;
	if (descriptor->kind == 5) {
		const char* swizzle;
		usize output_count = rtsw_execute_component_count(program, result->type);
		usize component;
		if (immediates[0] >= program->string_count || !(swizzle = program->strings[immediates[0]]) ||
			strlen(swizzle) != output_count || output_count > descriptor->element_count) return false;
		for (component = 0; component != output_count; ++component) {
			switch (swizzle[component]) {
			case 'x': case 'r': result->components[component] = base->components[0]; break;
			case 'y': case 'g': if (descriptor->element_count < 2) return false; result->components[component] = base->components[1]; break;
			case 'z': case 'b': if (descriptor->element_count < 3) return false; result->components[component] = base->components[2]; break;
			case 'w': case 'a': if (descriptor->element_count < 4) return false; result->components[component] = base->components[3]; break;
			default: return false;
			}
		}
		return true;
	}
	if (descriptor->kind != 7) return false;
	for (index = 0; index != descriptor->member_count; ++index) {
		usize count = rtsw_execute_component_count(program, descriptor->members[index].type);
		if (descriptor->members[index].name == immediates[0]) {
			if (count == 0 || count > RTSW_RTIR_MAX_COMPONENTS || result->type != descriptor->members[index].type) return false;
			memcpy(result->components, base->components + component, count * sizeof(f32));
			return true;
		}
		component += count;
	}
	return false;
}

static bool rtsw_execute_unary(
	const struct rtsw_rtir_program* program,
	struct rtsw_rtir_value* result,
	struct rtsw_execute_value* values,
	u32 value_count,
	u32 opcode,
	const u32* operands,
	u32 operand_count
) {
	struct rtsw_rtir_value* operand;
	usize component_count;
	usize component;
	if (operand_count != 1 || !(operand = rtsw_execute_find_value(values, value_count, operands[0])) ||
		(component_count = rtsw_execute_component_count(program, result->type)) == 0 ||
		component_count > RTSW_RTIR_MAX_COMPONENTS) return false;
	for (component = 0; component != component_count; ++component) {
		if (opcode == RTSW_RTIR_NEGATE) result->components[component] = -operand->components[component];
		else return false;
	}
	return true;
}

static bool rtsw_execute_binary(
	const struct rtsw_rtir_program* program,
	struct rtsw_rtir_value* result,
	struct rtsw_execute_value* values,
	u32 value_count,
	u32 opcode,
	const u32* operands,
	u32 operand_count
) {
	struct rtsw_rtir_value* left;
	struct rtsw_rtir_value* right;
	usize left_count;
	usize right_count;
	usize result_count;
	usize component;
	if (operand_count != 2 || !(left = rtsw_execute_find_value(values, value_count, operands[0])) ||
		!(right = rtsw_execute_find_value(values, value_count, operands[1])) ||
		!(left_count = rtsw_execute_component_count(program, left->type)) ||
		!(right_count = rtsw_execute_component_count(program, right->type)) ||
		!(result_count = rtsw_execute_component_count(program, result->type)) || result_count > RTSW_RTIR_MAX_COMPONENTS) return false;
	if (opcode == RTSW_RTIR_MULTIPLY && left_count == 16 && right_count == 4 && result_count == 4) {
		for (component = 0; component != 4; ++component) {
			result->components[component] = left->components[component] * right->components[0] + left->components[4 + component] * right->components[1] +
				left->components[8 + component] * right->components[2] + left->components[12 + component] * right->components[3];
		}
		return true;
	}
	if ((left_count != 1 && left_count != result_count) || (right_count != 1 && right_count != result_count)) return false;
	for (component = 0; component != result_count; ++component) {
		f32 left_value = left->components[left_count == 1 ? 0 : component];
		f32 right_value = right->components[right_count == 1 ? 0 : component];
		switch (opcode) {
		case RTSW_RTIR_ADD: result->components[component] = left_value + right_value; break;
		case RTSW_RTIR_SUBTRACT: result->components[component] = left_value - right_value; break;
		case RTSW_RTIR_MULTIPLY: result->components[component] = left_value * right_value; break;
		case RTSW_RTIR_DIVIDE: if (right_value == 0.0f) return false; result->components[component] = left_value / right_value; break;
		case RTSW_RTIR_REMAINDER: if (right_value == 0.0f) return false; result->components[component] = (f32)((i64)left_value % (i64)right_value); break;
		case RTSW_RTIR_COMPARE_EQUAL: result->components[component] = left_value == right_value; break;
		case RTSW_RTIR_COMPARE_NOT_EQUAL: result->components[component] = left_value != right_value; break;
		case RTSW_RTIR_COMPARE_LESS: result->components[component] = left_value < right_value; break;
		case RTSW_RTIR_COMPARE_LESS_EQUAL: result->components[component] = left_value <= right_value; break;
		case RTSW_RTIR_COMPARE_GREATER: result->components[component] = left_value > right_value; break;
		case RTSW_RTIR_COMPARE_GREATER_EQUAL: result->components[component] = left_value >= right_value; break;
		default: return false;
		}
	}
	return true;
}

static bool rtsw_rtir_execute(struct rtsw_execute_reader* reader, const struct rtsw_rtir_program* program, const struct rtsw_rtir_execution_context* context, const struct rtsw_rtir_value* parameters, u32 parameter_count, struct rtsw_rtir_value* result) {
	struct rtsw_execute_value values[RTSW_RTIR_MAX_VALUES];
	struct rtsw_rtir_value* return_value;
	u32 value_count = 0;
	u32 function_id;
	u32 ignored;
	u08 boolean;
	u32 encoded_parameter_count;
	u32 block_count;
	u32 index;
	if (!rtsw_execute_u32(reader, &function_id) || !rtsw_execute_skip(reader, sizeof(u32) * 2) ||
		!rtsw_execute_u8(reader, &boolean) || boolean > 1 || !rtsw_execute_u8(reader, &boolean) || boolean > 1 ||
		!rtsw_execute_u32(reader, &encoded_parameter_count) || encoded_parameter_count != parameter_count) return false;
	for (index = 0; index != encoded_parameter_count; ++index) {
		u32 value;
		if (!rtsw_execute_u32(reader, &value) || !rtsw_execute_u32(reader, &ignored) || !rtsw_execute_skip(reader, sizeof(u32)) || value_count == RTSW_RTIR_MAX_VALUES) return false;
		values[value_count++] = (struct rtsw_execute_value){ value, parameters[index] };
	}
	if (!rtsw_execute_u32(reader, &block_count) || block_count != 1) return false;
	if (!rtsw_execute_skip(reader, sizeof(u32)) || !rtsw_execute_u32(reader, &ignored) || ignored != 0 || !rtsw_execute_u32(reader, &ignored)) return false;
	for (u32 instruction_index = 0; instruction_index != ignored; ++instruction_index) {
		u32 opcode, result_id, type, callee, operands[RTSW_RTIR_MAX_OPERANDS], immediates[RTSW_RTIR_MAX_OPERANDS], operand_count, immediate_count;
		struct rtsw_rtir_value value = { 0 };
		if (!rtsw_execute_u32(reader, &opcode) || !rtsw_execute_u32(reader, &result_id) || !rtsw_execute_u32(reader, &type) || !rtsw_execute_u32(reader, &callee) || !rtsw_execute_vector(reader, operands, RTSW_RTIR_MAX_OPERANDS, &operand_count) || !rtsw_execute_vector(reader, immediates, RTSW_RTIR_MAX_OPERANDS, &immediate_count) || value_count == RTSW_RTIR_MAX_VALUES) return false;
		value.type = type;
		if (opcode == RTSW_RTIR_CONSTANT_BOOLEAN) {
			if (immediate_count != 1 || immediates[0] > 1) return false;
			value.components[0] = (f32)immediates[0];
		} else if (opcode == RTSW_RTIR_CONSTANT_INTEGER) {
			if (immediate_count != 2) return false;
			value.components[0] = (f32)((u64)immediates[0] | (u64)immediates[1] << 32);
		} else if (opcode == RTSW_RTIR_CONSTANT_FLOATING) {
			if (immediate_count != 1) return false;
			memcpy(value.components, immediates, sizeof(f32));
		} else if (opcode == RTSW_RTIR_CONSTRUCT) {
			if (!rtsw_execute_construct(program, &value, values, value_count, operands, operand_count)) return false;
		} else if (opcode == RTSW_RTIR_ACCESS) {
			if (!rtsw_execute_access(program, &value, values, value_count, operands, operand_count, immediates, immediate_count)) return false;
		} else if (opcode == RTSW_RTIR_NEGATE) {
			if (!rtsw_execute_unary(program, &value, values, value_count, opcode, operands, operand_count)) return false;
		} else if (opcode >= RTSW_RTIR_ADD && opcode <= RTSW_RTIR_COMPARE_GREATER_EQUAL) {
			if (!rtsw_execute_binary(program, &value, values, value_count, opcode, operands, operand_count)) return false;
		} else if (opcode == RTSW_RTIR_RESOURCE_LOAD) {
			if (!context || !context->resource_load || immediate_count != 1 || operand_count != 0 ||
				!context->resource_load(context->user_data, immediates[0], type, &value)) return false;
		} else if (opcode == RTSW_RTIR_CALL) {
			struct rtsw_rtir_value arguments[RTSW_RTIR_MAX_OPERANDS];
			for (u32 operand_index = 0; operand_index != operand_count; ++operand_index) {
				struct rtsw_rtir_value* argument = rtsw_execute_find_value(values, value_count, operands[operand_index]);
				if (!argument) return false;
				arguments[operand_index] = *argument;
			}
			if (!rtsw_rtir_execute_function_with_context(program, callee, context, arguments, operand_count, &value)) return false;
		} else return false;
		if (result_id) values[value_count++] = (struct rtsw_execute_value){ result_id, value };
	}
	if (!rtsw_execute_u8(reader, &boolean) || boolean != 1 || !rtsw_execute_u32(reader, &ignored) || ignored != RTSW_RTIR_RETURN_VALUE || !rtsw_execute_vector(reader, &function_id, 1, &index) || index != 1) return false;
	return_value = rtsw_execute_find_value(values, value_count, function_id);
	if (!return_value) return false;
	*result = *return_value;
	return true;
}

bool rtsw_rtir_execute_function_with_context(const struct rtsw_rtir_program* program, u32 function, const struct rtsw_rtir_execution_context* context, const struct rtsw_rtir_value* parameters, u32 parameter_count, struct rtsw_rtir_value* result) {
	const u08* bytes;
	usize byte_size;
	struct rtsw_execute_reader reader;
	if (!program || !result || !rtsw_rtir_program_function_bytes(program, function, &bytes, &byte_size)) return false;
	reader = (struct rtsw_execute_reader){ bytes, byte_size, 0 };
	return rtsw_rtir_execute(&reader, program, context, parameters, parameter_count, result);
}

bool rtsw_rtir_execute_function(const struct rtsw_rtir_program* program, u32 function, const struct rtsw_rtir_value* parameters, u32 parameter_count, struct rtsw_rtir_value* result) {
	return rtsw_rtir_execute_function_with_context(program, function, NULL, parameters, parameter_count, result);
}

usize rtsw_rtir_value_component_count(const struct rtsw_rtir_program* program, u32 type) {
	return rtsw_execute_component_count(program, type);
}
