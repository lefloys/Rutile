#ifndef RTSW_ARTIFACT_H
#define RTSW_ARTIFACT_H

#include "rutile.h"

#include <stdbool.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

enum rtsw_rtir_stage {
	RTSW_RTIR_STAGE_VERTEX,
	RTSW_RTIR_STAGE_TESSELLATION_CONTROL,
	RTSW_RTIR_STAGE_TESSELLATION_EVALUATION,
	RTSW_RTIR_STAGE_GEOMETRY,
	RTSW_RTIR_STAGE_FRAGMENT,
	RTSW_RTIR_STAGE_COMPUTE,
};

struct rtsw_rtir_entry {
	u32 function;
	u32 source_name;
	enum rtsw_rtir_stage stage;
};

struct rtsw_rtir_member {
	u32 name;
	u32 type;
};

struct rtsw_rtir_symbol {
	u32 id;
	u32 name;
};

struct rtsw_rtir_resource {
	u32 symbol;
	u32 type;
	u32 kind;
};

struct rtsw_rtir_uniform {
	u32 symbol;
	u32 type;
};

struct rtsw_rtir_type {
	u32 id;
	u32 kind;
	u32 bit_width;
	u32 element_type;
	u32 element_count;
	struct rtsw_rtir_member* members;
	u32 member_count;
};

struct rtsw_rtir_program {
	u16 version_minor;
	u08* bytes;
	usize byte_size;
	char** strings;
	u32 string_count;
	struct rtsw_rtir_symbol* symbols;
	u32 symbol_count;
	struct rtsw_rtir_resource* resources;
	u32 resource_count;
	struct rtsw_rtir_uniform* uniforms;
	u32 uniform_count;
	struct rtsw_rtir_type* types;
	u32 type_count;
	struct rtsw_rtir_entry* entries;
	u32 entry_count;
	u08* function_bytes;
	usize function_byte_size;
	u32* function_ids;
	usize* function_offsets;
	usize* function_sizes;
	u32 function_count;
};

bool rtsw_rtir_program_read(
	struct rtsw_rtir_program* program,
	const u08* bytes,
	usize byte_size
);
void rtsw_rtir_program_finish(struct rtsw_rtir_program* program);
const struct rtsw_rtir_entry* rtsw_rtir_program_entry(
	const struct rtsw_rtir_program* program,
	const char* source_name,
	enum rtsw_rtir_stage stage
);
bool rtsw_rtir_program_has_function(const struct rtsw_rtir_program* program, u32 function);
bool rtsw_rtir_program_function_bytes(
	const struct rtsw_rtir_program* program,
	u32 function,
	const u08** bytes,
	usize* byte_size
);
const struct rtsw_rtir_type* rtsw_rtir_program_type(const struct rtsw_rtir_program* program, u32 type);
u32 rtsw_rtir_program_function_parameter_type(
	const struct rtsw_rtir_program* program,
	u32 function,
	u32 parameter_index
);

#endif
