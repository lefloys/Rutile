#include "artifact.h"
#include "error.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
	RTSW_ARTIFACT_HEADER_SIZE = 24,
	RTSW_ARTIFACT_DIRECTORY_ENTRY_SIZE = 24,
	RTSW_ARTIFACT_SECTION_COUNT = 9,
	RTSW_ARTIFACT_MAXIMUM_RECORD_COUNT = 1u << 24,
	RTSW_ARTIFACT_PROGRAM = 2,
	RTSW_ARTIFACT_SECTION_STRINGS = 1,
	RTSW_ARTIFACT_SECTION_TYPES = 3,
	RTSW_ARTIFACT_SECTION_SYMBOLS = 4,
	RTSW_ARTIFACT_SECTION_ENTRIES = 6,
	RTSW_ARTIFACT_SECTION_FUNCTIONS = 5,
	RTSW_ARTIFACT_SECTION_RESOURCES = 7,
	RTSW_ARTIFACT_SECTION_UNIFORMS = 8,
};

struct rtsw_artifact_reader {
	const u08* bytes;
	usize size;
	usize offset;
};

struct rtsw_artifact_section {
	u32 kind;
	u64 offset;
	u64 size;
};

static bool rtsw_artifact_read_u8(struct rtsw_artifact_reader* reader, u08* value) {
	if (reader->offset >= reader->size) {
		return false;
	}

	*value = reader->bytes[reader->offset++];
	return true;
}

static bool rtsw_artifact_read_u16(struct rtsw_artifact_reader* reader, u16* value) {
	u08 first;
	u08 second;

	if (!rtsw_artifact_read_u8(reader, &first) || !rtsw_artifact_read_u8(reader, &second)) {
		return false;
	}

	*value = (u16)first | (u16)((u16)second << 8);
	return true;
}

static bool rtsw_artifact_read_u32(struct rtsw_artifact_reader* reader, u32* value) {
	u32 result = 0;
	u32 shift;

	for (shift = 0; shift != 32; shift += 8) {
		u08 part;
		if (!rtsw_artifact_read_u8(reader, &part)) {
			return false;
		}
		result |= (u32)part << shift;
	}

	*value = result;
	return true;
}

static bool rtsw_artifact_read_u64(struct rtsw_artifact_reader* reader, u64* value) {
	u64 result = 0;
	u32 shift;

	for (shift = 0; shift != 64; shift += 8) {
		u08 part;
		if (!rtsw_artifact_read_u8(reader, &part)) {
			return false;
		}
		result |= (u64)part << shift;
	}

	*value = result;
	return true;
}

static bool rtsw_artifact_skip(struct rtsw_artifact_reader* reader, usize size) {
	if (size > reader->size - reader->offset) {
		return false;
	}

	reader->offset += size;
	return true;
}

static bool rtsw_artifact_read_count(struct rtsw_artifact_reader* reader, u32* value) {
	return rtsw_artifact_read_u32(reader, value) && *value <= RTSW_ARTIFACT_MAXIMUM_RECORD_COUNT;
}

static bool rtsw_artifact_read_vector(struct rtsw_artifact_reader* reader, usize element_size) {
	u32 count;

	if (!rtsw_artifact_read_count(reader, &count)) {
		return false;
	}

	if (element_size != 0 && count > (reader->size - reader->offset) / element_size) {
		return false;
	}

	return rtsw_artifact_skip(reader, (usize)count * element_size);
}

static const struct rtsw_artifact_section* rtsw_artifact_find_section(
	const struct rtsw_artifact_section* sections,
	u32 kind
) {
	u32 index;

	for (index = 0; index != RTSW_ARTIFACT_SECTION_COUNT; ++index) {
		if (sections[index].kind == kind) {
			return &sections[index];
		}
	}

	return NULL;
}

static bool rtsw_rtir_read_strings(struct rtsw_rtir_program* program, struct rtsw_artifact_reader* reader) {
	u32 index;

	if (!rtsw_artifact_read_count(reader, &program->string_count)) {
		return false;
	}

	if (program->string_count == UINT32_MAX) {
		return false;
	}

	program->strings = calloc((usize)program->string_count + 1, sizeof(*program->strings));
	if (!program->strings) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate RTIR string table");
		return false;
	}

	for (index = 1; index <= program->string_count; ++index) {
		u32 length;
		char* string;

		if (!rtsw_artifact_read_count(reader, &length) || length > reader->size - reader->offset) {
			return false;
		}

		string = malloc((usize)length + 1);
		if (!string) {
			rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate RTIR string");
			return false;
		}

		memcpy(string, reader->bytes + reader->offset, length);
		string[length] = '\0';
		program->strings[index] = string;
		reader->offset += length;
	}

	return reader->offset == reader->size;
}

static bool rtsw_rtir_skip_optional_u32(struct rtsw_artifact_reader* reader) {
	u08 present;
	return rtsw_artifact_read_u8(reader, &present) && present <= 1 &&
		(!present || rtsw_artifact_skip(reader, sizeof(u32)));
}

static bool rtsw_rtir_skip_optional_binding(struct rtsw_artifact_reader* reader) {
	u08 present;
	return rtsw_artifact_read_u8(reader, &present) && present <= 1 &&
		(!present || rtsw_artifact_skip(reader, sizeof(u32) * 2));
}

static bool rtsw_rtir_read_symbols(struct rtsw_rtir_program* program, struct rtsw_artifact_reader* reader) {
	u32 index;
	if (!rtsw_artifact_read_count(reader, &program->symbol_count)) return false;
	program->symbols = calloc(program->symbol_count, sizeof(*program->symbols));
	if (program->symbol_count && !program->symbols) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate RTIR symbol table");
		return false;
	}
	for (index = 0; index != program->symbol_count; ++index) {
		if (!rtsw_artifact_read_u32(reader, &program->symbols[index].id) || !program->symbols[index].id ||
			!rtsw_artifact_read_u32(reader, &program->symbols[index].name) || program->symbols[index].name > program->string_count) return false;
		if (program->version_minor >= 3) {
			u08 exported;
			if (!rtsw_artifact_read_u8(reader, &exported) || exported > 1) return false;
		}
	}
	return reader->offset == reader->size;
}

static bool rtsw_rtir_read_resources(struct rtsw_rtir_program* program, struct rtsw_artifact_reader* reader) {
	u32 index;
	if (!rtsw_artifact_read_count(reader, &program->resource_count)) return false;
	program->resources = calloc(program->resource_count, sizeof(*program->resources));
	if (program->resource_count && !program->resources) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate RTIR resource table");
		return false;
	}
	for (index = 0; index != program->resource_count; ++index) {
		u32 access;
		if (!rtsw_artifact_read_u32(reader, &program->resources[index].symbol) || !program->resources[index].symbol ||
			!rtsw_artifact_read_u32(reader, &program->resources[index].kind) || program->resources[index].kind > 5 ||
			!rtsw_artifact_read_u32(reader, &program->resources[index].type) ||
			!rtsw_artifact_read_u32(reader, &access) || access > 2 ||
			!rtsw_rtir_skip_optional_binding(reader) || !rtsw_artifact_read_vector(reader, sizeof(u32))) return false;
	}
	return reader->offset == reader->size;
}

static bool rtsw_rtir_read_uniforms(struct rtsw_rtir_program* program, struct rtsw_artifact_reader* reader) {
	u32 index;
	if (!rtsw_artifact_read_count(reader, &program->uniform_count)) return false;
	program->uniforms = calloc(program->uniform_count, sizeof(*program->uniforms));
	if (program->uniform_count && !program->uniforms) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate RTIR uniform table");
		return false;
	}
	for (index = 0; index != program->uniform_count; ++index) {
		if (!rtsw_artifact_read_u32(reader, &program->uniforms[index].symbol) || !program->uniforms[index].symbol ||
			!rtsw_artifact_read_u32(reader, &program->uniforms[index].type) || !rtsw_rtir_skip_optional_binding(reader) ||
			!rtsw_rtir_skip_optional_u32(reader) || !rtsw_rtir_skip_optional_u32(reader) || !rtsw_rtir_skip_optional_u32(reader)) return false;
	}
	return reader->offset == reader->size;
}

static bool rtsw_rtir_read_types(struct rtsw_rtir_program* program, struct rtsw_artifact_reader* reader) {
	u32 type_index;

	if (!rtsw_artifact_read_count(reader, &program->type_count)) {
		return false;
	}
	program->types = calloc(program->type_count, sizeof(*program->types));
	if (program->type_count != 0 && !program->types) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate RTIR type table");
		return false;
	}

	for (type_index = 0; type_index != program->type_count; ++type_index) {
		struct rtsw_rtir_type* type = &program->types[type_index];
		u32 address_space;
		u32 parameter_count;
		u32 member_index;
		if (!rtsw_artifact_read_u32(reader, &type->id) || !type->id ||
			!rtsw_artifact_read_u32(reader, &type->kind) || type->kind > 17 ||
			!rtsw_artifact_read_u32(reader, &type->bit_width) ||
			!rtsw_artifact_read_u32(reader, &type->element_type) ||
			!rtsw_artifact_read_u32(reader, &type->element_count) ||
			!rtsw_artifact_read_u32(reader, &address_space) || address_space > 6 ||
			!rtsw_artifact_read_count(reader, &parameter_count) ||
			!rtsw_artifact_skip(reader, (usize)parameter_count * sizeof(u32)) ||
			!rtsw_artifact_read_count(reader, &type->member_count)) {
			return false;
		}
		type->members = calloc(type->member_count, sizeof(*type->members));
		if (type->member_count != 0 && !type->members) {
			rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate RTIR structure members");
			return false;
		}
		for (member_index = 0; member_index != type->member_count; ++member_index) {
			struct rtsw_rtir_member* member = &type->members[member_index];
			if (!rtsw_artifact_read_u32(reader, &member->name) || member->name > program->string_count ||
				!rtsw_artifact_read_u32(reader, &member->type) ||
				!rtsw_rtir_skip_optional_u32(reader) || !rtsw_rtir_skip_optional_u32(reader)) {
				return false;
			}
		}
		if (!rtsw_artifact_skip(reader, sizeof(u32))) {
			return false;
		}
	}
	return reader->offset == reader->size;
}

static bool rtsw_rtir_skip_contracts(struct rtsw_artifact_reader* reader) {
	u32 count;
	u32 index;

	if (!rtsw_artifact_read_count(reader, &count)) {
		return false;
	}

	for (index = 0; index != count; ++index) {
		if (!rtsw_artifact_skip(reader, sizeof(u32)) ||
			!rtsw_artifact_read_vector(reader, sizeof(u32)) ||
			!rtsw_artifact_skip(reader, sizeof(u32))) {
			return false;
		}
	}

	return true;
}

static bool rtsw_rtir_skip_entry_attributes(struct rtsw_artifact_reader* reader) {
	u32 count;
	u32 index;
	if (!rtsw_artifact_read_count(reader, &count)) return false;
	for (index = 0; index != count; ++index) {
		if (!rtsw_artifact_skip(reader, sizeof(u32)) || !rtsw_artifact_read_vector(reader, sizeof(u32))) return false;
	}
	return true;
}

static bool rtsw_rtir_read_entries(struct rtsw_rtir_program* program, struct rtsw_artifact_reader* reader) {
	u32 index;

	if (!rtsw_artifact_read_count(reader, &program->entry_count)) {
		return false;
	}

	program->entries = calloc(program->entry_count, sizeof(*program->entries));
	if (program->entry_count != 0 && !program->entries) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate RTIR entry table");
		return false;
	}

	for (index = 0; index != program->entry_count; ++index) {
		u32 symbol;
		u08 configuration;

		if (!rtsw_artifact_read_u32(reader, &symbol) ||
			!rtsw_artifact_read_u32(reader, &program->entries[index].function) ||
			!rtsw_artifact_read_u32(reader, &program->entries[index].source_name) ||
			!rtsw_artifact_read_u32(reader, (u32*)&program->entries[index].stage) ||
			!rtsw_artifact_read_u8(reader, &configuration)) {
			return false;
		}

		if (program->entries[index].source_name > program->string_count ||
			program->entries[index].stage > RTSW_RTIR_STAGE_COMPUTE) {
			return false;
		}

		switch (configuration) {
		case 0:
			break;
		case 1:
			if (!rtsw_artifact_skip(reader, sizeof(u32))) return false;
			break;
		case 2:
			if (!rtsw_artifact_skip(reader, sizeof(u32) * 3)) return false;
			break;
		case 3:
			if (!rtsw_artifact_skip(reader, sizeof(u32) * 4)) return false;
			break;
		case 4:
			if (!rtsw_artifact_skip(reader, sizeof(u32) * 3)) return false;
			break;
		default:
			return false;
		}

		if (program->version_minor >= 4 && !rtsw_rtir_skip_entry_attributes(reader)) {
			return false;
		}
		if (!rtsw_rtir_skip_contracts(reader)) {
			return false;
		}
	}

	return reader->offset == reader->size;
}

static bool rtsw_rtir_read_boolean(struct rtsw_artifact_reader* reader) {
	u08 value;
	return rtsw_artifact_read_u8(reader, &value) && value <= 1;
}

static bool rtsw_rtir_skip_terminator(struct rtsw_artifact_reader* reader) {
	u32 kind;
	u32 successors;
	u32 index;

	if (!rtsw_artifact_read_u32(reader, &kind) || kind > 6 ||
		!rtsw_artifact_read_vector(reader, sizeof(u32)) ||
		!rtsw_artifact_read_count(reader, &successors)) {
		return false;
	}

	for (index = 0; index != successors; ++index) {
		if (!rtsw_artifact_skip(reader, sizeof(u32)) ||
			!rtsw_artifact_read_vector(reader, sizeof(u32))) {
			return false;
		}
	}

	return rtsw_artifact_read_vector(reader, sizeof(u32));
}

static bool rtsw_rtir_read_functions(struct rtsw_rtir_program* program, struct rtsw_artifact_reader* reader) {
	u32 function_index;

	if (!rtsw_artifact_read_count(reader, &program->function_count)) {
		return false;
	}

	program->function_ids = calloc(program->function_count, sizeof(*program->function_ids));
	program->function_offsets = calloc(program->function_count, sizeof(*program->function_offsets));
	program->function_sizes = calloc(program->function_count, sizeof(*program->function_sizes));
	if (program->function_count != 0 && (!program->function_ids || !program->function_offsets || !program->function_sizes)) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate RTIR function table");
		return false;
	}

	for (function_index = 0; function_index != program->function_count; ++function_index) {
		u32 symbol;
		u32 return_type;
		u32 parameter_count;
		u32 block_count;
		u32 parameter_index;
		u32 block_index;

		program->function_offsets[function_index] = reader->offset;
		if (!rtsw_artifact_read_u32(reader, &program->function_ids[function_index]) ||
			!program->function_ids[function_index] ||
			!rtsw_artifact_read_u32(reader, &symbol) ||
			!rtsw_artifact_read_u32(reader, &return_type) ||
			!rtsw_rtir_read_boolean(reader) || !rtsw_rtir_read_boolean(reader) ||
			!rtsw_artifact_read_count(reader, &parameter_count)) {
			return false;
		}

		for (parameter_index = 0; parameter_index != parameter_count; ++parameter_index) {
			if (!rtsw_artifact_skip(reader, sizeof(u32) * 3)) {
				return false;
			}
		}

		if (!rtsw_artifact_read_count(reader, &block_count)) {
			return false;
		}

		for (block_index = 0; block_index != block_count; ++block_index) {
			u32 argument_count;
			u32 instruction_count;
			u32 argument_index;
			u32 instruction_index;
			u08 terminator_present;
			u32 merge_kind;

			if (!rtsw_artifact_skip(reader, sizeof(u32)) ||
				!rtsw_artifact_read_count(reader, &argument_count)) {
				return false;
			}
			for (argument_index = 0; argument_index != argument_count; ++argument_index) {
				if (!rtsw_artifact_skip(reader, sizeof(u32) * 2)) {
					return false;
				}
			}

			if (!rtsw_artifact_read_count(reader, &instruction_count)) {
				return false;
			}
			for (instruction_index = 0; instruction_index != instruction_count; ++instruction_index) {
				u32 opcode;
				if (!rtsw_artifact_read_u32(reader, &opcode) || opcode > 39 ||
					!rtsw_artifact_skip(reader, sizeof(u32) * 3) ||
					!rtsw_artifact_read_vector(reader, sizeof(u32)) ||
					!rtsw_artifact_read_vector(reader, sizeof(u32))) {
					return false;
				}
			}

			if (!rtsw_artifact_read_u8(reader, &terminator_present) || terminator_present > 1 ||
				(terminator_present && !rtsw_rtir_skip_terminator(reader)) ||
				!rtsw_artifact_read_u32(reader, &merge_kind) || merge_kind > 2 ||
				!rtsw_artifact_skip(reader, sizeof(u32) * 2)) {
				return false;
			}
		}
		program->function_sizes[function_index] = reader->offset - program->function_offsets[function_index];
	}

	return reader->offset == reader->size;
}

bool rtsw_rtir_program_read(struct rtsw_rtir_program* program, const u08* bytes, usize byte_size) {
	static const u08 magic[] = { 'R', 'T', 'S', 'L', 'R', 'T', 'I', 'R' };
	struct rtsw_artifact_reader reader;
	struct rtsw_artifact_section sections[RTSW_ARTIFACT_SECTION_COUNT] = { 0 };
	u16 major;
	u16 minor;
	u32 endian;
	u08 kind;
	u08 reserved[3];
	u32 section_count;
	u32 index;
	const struct rtsw_artifact_section* strings;
	const struct rtsw_artifact_section* types;
	const struct rtsw_artifact_section* symbols;
	const struct rtsw_artifact_section* entries;
	const struct rtsw_artifact_section* functions;
	const struct rtsw_artifact_section* resources;
	const struct rtsw_artifact_section* uniforms;

	memset(program, 0, sizeof(*program));
	if (!bytes || byte_size < RTSW_ARTIFACT_HEADER_SIZE) {
		return false;
	}

	reader = (struct rtsw_artifact_reader){ bytes, byte_size, 0 };
	if (!rtsw_artifact_skip(&reader, sizeof(magic)) || memcmp(bytes, magic, sizeof(magic)) != 0 ||
		!rtsw_artifact_read_u16(&reader, &major) || !rtsw_artifact_read_u16(&reader, &minor) ||
		!rtsw_artifact_read_u32(&reader, &endian) || !rtsw_artifact_read_u8(&reader, &kind) ||
		!rtsw_artifact_read_u8(&reader, &reserved[0]) || !rtsw_artifact_read_u8(&reader, &reserved[1]) ||
		!rtsw_artifact_read_u8(&reader, &reserved[2]) || !rtsw_artifact_read_u32(&reader, &section_count)) {
		return false;
	}

	if (major != 1 || minor > 4 || endian != 0x01020304 || kind != RTSW_ARTIFACT_PROGRAM ||
		reserved[0] != 0 || reserved[1] != 0 || reserved[2] != 0 ||
		section_count != RTSW_ARTIFACT_SECTION_COUNT ||
		byte_size < RTSW_ARTIFACT_HEADER_SIZE + RTSW_ARTIFACT_DIRECTORY_ENTRY_SIZE * section_count) {
		return false;
	}
	program->version_minor = minor;

	for (index = 0; index != section_count; ++index) {
		u32 reserved_directory;
		struct rtsw_artifact_section* section = &sections[index];

		if (!rtsw_artifact_read_u32(&reader, &section->kind) ||
			!rtsw_artifact_read_u32(&reader, &reserved_directory) ||
			!rtsw_artifact_read_u64(&reader, &section->offset) ||
			!rtsw_artifact_read_u64(&reader, &section->size) ||
			section->kind == 0 || section->kind > RTSW_ARTIFACT_SECTION_COUNT ||
			reserved_directory != 0 || section->offset > byte_size || section->size > byte_size - section->offset) {
			return false;
		}
	}

	strings = rtsw_artifact_find_section(sections, RTSW_ARTIFACT_SECTION_STRINGS);
	types = rtsw_artifact_find_section(sections, RTSW_ARTIFACT_SECTION_TYPES);
	symbols = rtsw_artifact_find_section(sections, RTSW_ARTIFACT_SECTION_SYMBOLS);
	entries = rtsw_artifact_find_section(sections, RTSW_ARTIFACT_SECTION_ENTRIES);
	functions = rtsw_artifact_find_section(sections, RTSW_ARTIFACT_SECTION_FUNCTIONS);
	resources = rtsw_artifact_find_section(sections, RTSW_ARTIFACT_SECTION_RESOURCES);
	uniforms = rtsw_artifact_find_section(sections, RTSW_ARTIFACT_SECTION_UNIFORMS);
	if (!strings || !types || !symbols || !entries || !functions || !resources || !uniforms) {
		return false;
	}

	reader = (struct rtsw_artifact_reader){ bytes + strings->offset, (usize)strings->size, 0 };
	if (!rtsw_rtir_read_strings(program, &reader)) {
		goto failure;
	}
	reader = (struct rtsw_artifact_reader){ bytes + types->offset, (usize)types->size, 0 };
	if (!rtsw_rtir_read_types(program, &reader)) {
		goto failure;
	}
	reader = (struct rtsw_artifact_reader){ bytes + symbols->offset, (usize)symbols->size, 0 };
	if (!rtsw_rtir_read_symbols(program, &reader)) goto failure;
	reader = (struct rtsw_artifact_reader){ bytes + resources->offset, (usize)resources->size, 0 };
	if (!rtsw_rtir_read_resources(program, &reader)) goto failure;
	reader = (struct rtsw_artifact_reader){ bytes + uniforms->offset, (usize)uniforms->size, 0 };
	if (!rtsw_rtir_read_uniforms(program, &reader)) goto failure;

	reader = (struct rtsw_artifact_reader){ bytes + entries->offset, (usize)entries->size, 0 };
	if (!rtsw_rtir_read_entries(program, &reader)) {
		goto failure;
	}

	program->function_bytes = malloc((usize)functions->size);
	if (functions->size != 0 && !program->function_bytes) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to retain RTIR function section");
		goto failure;
	}
	memcpy(program->function_bytes, bytes + functions->offset, (usize)functions->size);
	program->function_byte_size = (usize)functions->size;
	reader = (struct rtsw_artifact_reader){ program->function_bytes, program->function_byte_size, 0 };
	if (!rtsw_rtir_read_functions(program, &reader)) {
		goto failure;
	}

	program->bytes = malloc(byte_size);
	if (!program->bytes) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to retain RTIR program bytes");
		goto failure;
	}

	memcpy(program->bytes, bytes, byte_size);
	program->byte_size = byte_size;
	return true;

failure:
	rtsw_rtir_program_finish(program);
	return false;
}

void rtsw_rtir_program_finish(struct rtsw_rtir_program* program) {
	u32 index;

	if (!program) {
		return;
	}

	for (index = 1; index <= program->string_count; ++index) {
		free(program->strings[index]);
	}
	for (index = 0; index != program->type_count; ++index) {
		free(program->types[index].members);
	}

	free(program->entries);
	free(program->uniforms);
	free(program->resources);
	free(program->symbols);
	free(program->function_bytes);
	free(program->function_ids);
	free(program->function_offsets);
	free(program->function_sizes);
	free(program->strings);
	free(program->types);
	free(program->bytes);
	memset(program, 0, sizeof(*program));
}

const struct rtsw_rtir_entry* rtsw_rtir_program_entry(const struct rtsw_rtir_program* program, const char* source_name, enum rtsw_rtir_stage stage) {
	u32 index;

	if (!program || !source_name) {
		return NULL;
	}

	for (index = 0; index != program->entry_count; ++index) {
		const struct rtsw_rtir_entry* entry = &program->entries[index];
		if (entry->stage == stage && strcmp(program->strings[entry->source_name], source_name) == 0) {
			return entry;
		}
	}

	return NULL;
}

bool rtsw_rtir_program_has_function(const struct rtsw_rtir_program* program, u32 function) {
	u32 index;

	if (!program || !function) {
		return false;
	}

	for (index = 0; index != program->function_count; ++index) {
		if (program->function_ids[index] == function) {
			return true;
		}
	}

	return false;
}

bool rtsw_rtir_program_function_bytes(const struct rtsw_rtir_program* program, u32 function, const u08** bytes, usize* byte_size) {
	u32 index;

	if (!program || !bytes || !byte_size || !function) {
		return false;
	}

	for (index = 0; index != program->function_count; ++index) {
		if (program->function_ids[index] == function) {
			*bytes = program->function_bytes + program->function_offsets[index];
			*byte_size = program->function_sizes[index];
			return true;
		}
	}

	return false;
}

const struct rtsw_rtir_type* rtsw_rtir_program_type(const struct rtsw_rtir_program* program, u32 type) {
	u32 index;

	if (!program || !type) {
		return NULL;
	}
	for (index = 0; index != program->type_count; ++index) {
		if (program->types[index].id == type) {
			return &program->types[index];
		}
	}
	return NULL;
}

u32 rtsw_rtir_program_function_parameter_type(const struct rtsw_rtir_program* program, u32 function, u32 parameter_index) {
	const u08* bytes;
	usize byte_size;
	struct rtsw_artifact_reader reader;
	u32 parameter_count;
	u32 index;
	u32 ignored;

	if (!rtsw_rtir_program_function_bytes(program, function, &bytes, &byte_size)) {
		return 0;
	}
	reader = (struct rtsw_artifact_reader){ bytes, byte_size, 0 };
	if (!rtsw_artifact_skip(&reader, sizeof(u32) * 3) ||
		!rtsw_artifact_skip(&reader, sizeof(u08) * 2) ||
		!rtsw_artifact_read_count(&reader, &parameter_count) || parameter_index >= parameter_count) {
		return 0;
	}
	for (index = 0; index != parameter_count; ++index) {
		u32 type;
		if (!rtsw_artifact_read_u32(&reader, &ignored) || !rtsw_artifact_read_u32(&reader, &type) ||
			!rtsw_artifact_read_u32(&reader, &ignored)) {
			return 0;
		}
		if (index == parameter_index) {
			return type;
		}
	}
	return 0;
}
