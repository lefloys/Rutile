#ifndef RTSW_INTERPRETER_EXECUTE_H
#define RTSW_INTERPRETER_EXECUTE_H

#include "artifact.h"

#include <stdbool.h>

#define RTSW_RTIR_MAX_COMPONENTS 64

struct rtsw_rtir_value {
	u32 type;
	f32 components[RTSW_RTIR_MAX_COMPONENTS];
};

typedef bool (*rtsw_rtir_resource_load_proc)(void* user_data, u32 symbol, u32 type, struct rtsw_rtir_value* value);

struct rtsw_rtir_execution_context {
	void* user_data;
	rtsw_rtir_resource_load_proc resource_load;
};

bool rtsw_rtir_execute_function(
	const struct rtsw_rtir_program* program,
	u32 function,
	const struct rtsw_rtir_value* parameters,
	u32 parameter_count,
	struct rtsw_rtir_value* result
);
bool rtsw_rtir_execute_function_with_context(
	const struct rtsw_rtir_program* program,
	u32 function,
	const struct rtsw_rtir_execution_context* context,
	const struct rtsw_rtir_value* parameters,
	u32 parameter_count,
	struct rtsw_rtir_value* result
);
usize rtsw_rtir_value_component_count(const struct rtsw_rtir_program* program, u32 type);

#endif
