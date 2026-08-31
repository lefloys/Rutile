#ifndef RTSW_PROGRAM_H
#define RTSW_PROGRAM_H

#include "interpreter/artifact.h"
#include "resource.h"

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTSW_API rt_program rtProgramCreate(void);
RTSW_API void rtProgramDestroy(rt_program program);
RTSW_API void rtProgramSetLayout(rt_program program, const rt_vertex_layout* layout);
RTSW_API void rtProgramSource(rt_program program, const char* entry_point, const u08* bytes, usize byte_size);
RTSW_API void rtProgramSetRasterState(rt_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode);
RTSW_API void rtProgramSetBlendState(rt_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op);
RTSW_API void rtProgramFinalize(rt_program program);
RTSW_API rt_location rtProgramUniformLocation(rt_program program, const char* name);
RTSW_API rt_location rtProgramInputLocation(rt_program program, const rt_vertex_attribute* attributes, usize attribute_count);
RTSW_API rt_location rtProgramOutputLocation(rt_program program, const char* name);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rt_location_t {
	struct rtsw_program* program;
	u32 address;
	u32 symbol;
};

struct rtsw_program {
	struct rtsw_resource_base base;
	struct rtsw_rtir_program module;
	char* entry_point;
	u32 vertex_function;
	u32 fragment_function;
	u32 vertex_input_type;
	u32 fragment_input_type;
	rt_vertex_input* vertex_inputs;
	rt_vertex_attribute* vertex_attributes;
	struct rt_location_t* input_locations;
	struct rt_location_t* uniform_locations;
	usize vertex_input_count;
	usize vertex_attribute_count;
	usize uniform_location_count;
	enum rt_cull_mode cull_mode;
	enum rt_front_face front_face;
	enum rt_fill_mode fill_mode;
	bool blend_enabled;
	enum rt_blend_factor src_color_blend;
	enum rt_blend_factor dst_color_blend;
	enum rt_blend_op color_blend_op;
	enum rt_blend_factor src_alpha_blend;
	enum rt_blend_factor dst_alpha_blend;
	enum rt_blend_op alpha_blend_op;
	bool finalized;
};

RTSW_DECLARE_HANDLE(program, rtsw_program);
void rtsw_program_init(struct rtsw_context* ctx, struct rtsw_program* program);
void rtsw_program_finalize_resource(void* resource);
struct rtsw_program* rtsw_program_create(struct rtsw_context* ctx);
void rtsw_program_destroy(struct rtsw_program* program);

#endif
