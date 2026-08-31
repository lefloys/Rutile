#include "artifact.h"
#include "execute.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool identity_scene(void* user_data, u32 symbol, u32 type, struct rtsw_rtir_value* value) {
	(void)symbol;
	if (user_data) ++*(usize*)user_data;
	value->type = type;
	memset(value->components, 0, sizeof(value->components));
	value->components[0] = 1.0f;
	value->components[5] = 1.0f;
	value->components[10] = 1.0f;
	value->components[15] = 1.0f;
	return true;
}

static int test_cube_vertex(void) {
#ifdef RTSW_CUBE_ARTIFACT
	FILE* file = fopen(RTSW_CUBE_ARTIFACT, "rb");
	long file_size;
	u08* bytes;
	struct rtsw_rtir_program program;
	const struct rtsw_rtir_entry* vertex;
	struct rtsw_rtir_value point = { 0 };
	struct rtsw_rtir_value result = { 0 };
	usize load_count = 0;
	const struct rtsw_rtir_execution_context context = { &load_count, identity_scene };
	if (!file || fseek(file, 0, SEEK_END) != 0 || (file_size = ftell(file)) <= 0 || fseek(file, 0, SEEK_SET) != 0) {
		if (file) fclose(file);
		return 1;
	}
	bytes = malloc((usize)file_size);
	if (!bytes || fread(bytes, 1, (usize)file_size, file) != (usize)file_size) {
		free(bytes);
		fclose(file);
		return 1;
	}
	fclose(file);
	if (!rtsw_rtir_program_read(&program, bytes, (usize)file_size)) {
		free(bytes);
		return 1;
	}
	free(bytes);
	vertex = rtsw_rtir_program_entry(&program, "main", RTSW_RTIR_STAGE_VERTEX);
	if (vertex) point.type = rtsw_rtir_program_function_parameter_type(&program, vertex->function, 0);
	point.components[0] = 1.0f;
	point.components[1] = 2.0f;
	point.components[2] = 3.0f;
	point.components[3] = 0.25f;
	point.components[4] = 0.5f;
	point.components[5] = 0.75f;
	point.components[6] = 0.0f;
	point.components[7] = 1.0f;
	point.components[8] = 0.0f;
	if (!vertex || !point.type || !rtsw_rtir_execute_function_with_context(&program, vertex->function, &context, &point, 1, &result) ||
		result.components[0] != 1.0f || result.components[1] != 2.0f || result.components[2] != 3.0f || result.components[3] != 1.0f ||
		result.components[4] != 0.25f || result.components[5] != 0.5f || result.components[6] != 0.75f) {
		fprintf(stderr, "cube vertex execution failed: vertex=%p input=%u output=%u loads=%zu values=%g %g %g %g\n", (void*)vertex, point.type, result.type, load_count, result.components[0], result.components[1], result.components[2], result.components[3]);
		rtsw_rtir_program_finish(&program);
		return 1;
	}
	rtsw_rtir_program_finish(&program);
#endif
	return 0;
}

int main(void) {
	FILE* file = fopen(RTSW_TRIANGLE_ARTIFACT, "rb");
	long file_size;
	u08* bytes;
	struct rtsw_rtir_program program;
	const struct rtsw_rtir_entry* vertex;
	const struct rtsw_rtir_entry* fragment;
	struct rtsw_rtir_value point = { 0 };
	struct rtsw_rtir_value vertex_value = { 0 };
	u32 type_index;

	if (!file || fseek(file, 0, SEEK_END) != 0 || (file_size = ftell(file)) <= 0 || fseek(file, 0, SEEK_SET) != 0) {
		if (file) fclose(file);
		return 1;
	}
	bytes = malloc((usize)file_size);
	if (!bytes || fread(bytes, 1, (usize)file_size, file) != (usize)file_size) {
		free(bytes);
		fclose(file);
		return 1;
	}
	fclose(file);
	if (!rtsw_rtir_program_read(&program, bytes, (usize)file_size)) {
		fprintf(stderr, "read failed\n");
		free(bytes);
		return 1;
	}
	free(bytes);
	vertex = rtsw_rtir_program_entry(&program, "main", RTSW_RTIR_STAGE_VERTEX);
	fragment = rtsw_rtir_program_entry(&program, "main", RTSW_RTIR_STAGE_FRAGMENT);
	for (type_index = 0; type_index != program.type_count; ++type_index) {
		const struct rtsw_rtir_type* type = &program.types[type_index];
		if (type->kind == 7 && type->member_count == 2 &&
			strcmp(program.strings[type->members[0].name], "position") == 0 &&
			strcmp(program.strings[type->members[1].name], "color") == 0) {
			point.type = type->id;
			break;
		}
	}
	point.components[0] = 1.0f;
	point.components[1] = 2.0f;
	point.components[2] = 3.0f;
	point.components[3] = 0.25f;
	point.components[4] = 0.5f;
	point.components[5] = 0.75f;
	point.components[6] = 1.0f;
	if (!vertex || !point.type || !rtsw_rtir_execute_function(&program, vertex->function, &point, 1, &vertex_value) ||
		vertex_value.components[0] != 1.0f || vertex_value.components[1] != 2.0f || vertex_value.components[2] != 3.0f || vertex_value.components[3] != 1.0f ||
		vertex_value.components[4] != 0.25f || vertex_value.components[5] != 0.5f || vertex_value.components[6] != 0.75f || vertex_value.components[7] != 1.0f) {
		fprintf(stderr, "vertex=%p point=%u result=%u values=%g %g %g %g %g %g %g %g\n", (void*)vertex, point.type, vertex_value.type, vertex_value.components[0], vertex_value.components[1], vertex_value.components[2], vertex_value.components[3], vertex_value.components[4], vertex_value.components[5], vertex_value.components[6], vertex_value.components[7]);
		rtsw_rtir_program_finish(&program);
		return 1;
	}
	{
		struct rtsw_rtir_value color = { 0 };
		if (!fragment || !rtsw_rtir_execute_function(&program, fragment->function, &vertex_value, 1, &color) ||
			color.components[0] != 0.25f || color.components[1] != 0.5f || color.components[2] != 0.75f || color.components[3] != 1.0f) {
			rtsw_rtir_program_finish(&program);
			return 1;
		}
	}
	rtsw_rtir_program_finish(&program);
	return test_cube_vertex();
}
