#ifndef RTGL_PROGRAM_H
#define RTGL_PROGRAM_H

#include "glad/gl.h"
#include "resource.h"

#define RTGL_MAX_VERTEX_ATTRIBUTES 16
#define RTGL_LOCATION_ADDRESS_COUNT 256

RTGL_EXTERN_C_ENTER

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTGL_API rt_program rtProgramCreate(void);
RTGL_API void rtProgramDestroy(rt_program program);
RTGL_API void rtProgramSetLayout(rt_program program, const rt_vertex_layout* layout);
RTGL_API void rtProgramSource(rt_program program, const char* entry_point, const u08* data, usize size);
RTGL_API void rtProgramSetRasterState(rt_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode);
RTGL_API void rtProgramSetBlendState(rt_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op);
RTGL_API void rtProgramFinalize(rt_program program);
RTGL_API rt_location rtProgramUniformLocation(rt_program program, const char* name);
RTGL_API rt_location rtProgramInputLocation(rt_program program, const rt_vertex_attribute* attributes, usize attribute_count);
RTGL_API rt_location rtProgramOutputLocation(rt_program program, const char* name);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

typedef enum rtgl_location_kind {
	RTGL_LOCATION_MAPPING_UNIFORM_DATA,
	RTGL_LOCATION_MAPPING_STORAGE_DATA,
	RTGL_LOCATION_MAPPING_UNIFORM_BUFFER,
	RTGL_LOCATION_MAPPING_STORAGE_BUFFER,
	RTGL_LOCATION_MAPPING_STORAGE_TEXTURE_BUFFER,
	RTGL_LOCATION_MAPPING_TEXTURE,
	RTGL_LOCATION_MAPPING_VERTEX_STREAM,
	RTGL_LOCATION_MAPPING_OUTPUT,
} rtgl_location_kind;

struct rt_location_t {
	u08 address;
};

struct rtgl_program_mapping {
	char name[64];
	rtgl_location_kind kind;
	u32 binding;
	GLint gl_location;
	GLenum storage_texture_format;
	usize byte_offset;
	usize byte_size;
	usize block_size;
};

struct rtgl_program {
	struct rtgl_resource_base base;
	GLuint gl_program;
	char* entry_point;
	u08* source_bytes;
	u64 source_size;
	struct rt_location_t locations[RTGL_LOCATION_ADDRESS_COUNT];
	struct rtgl_program_mapping mappings[RTGL_LOCATION_ADDRESS_COUNT];
	bool location_occupied[RTGL_LOCATION_ADDRESS_COUNT];
	rt_vertex_layout vertex_layout;
	rt_vertex_input vertex_inputs[16];
	rt_vertex_attribute vertex_attributes[RTGL_MAX_VERTEX_ATTRIBUTES];
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
	bool tessellated;
	u32 patch_vertices;
};
RTGL_DECLARE_NEW_RESOURCE(program)

void rtgl_program_prepare(struct rtgl_context* ctx, struct rtgl_program* program);
void rtgl_program_layout(struct rtgl_context* ctx, struct rtgl_program* program, const rt_vertex_layout* layout);
void rtgl_program_source(struct rtgl_context* ctx, struct rtgl_program* program, const char* entry_point, const void* data, usize size);
void rtgl_program_raster_state(struct rtgl_context* ctx, struct rtgl_program* program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode);
void rtgl_program_blend_state(struct rtgl_context* ctx, struct rtgl_program* program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op);
void rtgl_program_finalize(struct rtgl_context* ctx, struct rtgl_program* program);
struct rt_location_t* rtgl_program_uniform_location(struct rtgl_context* ctx, struct rtgl_program* program, const char* name);
struct rt_location_t* rtgl_program_allocate_location(struct rtgl_program* program, bool zero_address);
struct rtgl_program_mapping* rtgl_program_mapping(struct rtgl_program* program, const struct rt_location_t* location);
struct rtgl_program* rtgl_location_program(const struct rt_location_t* location);
void rtgl_program_clear_locations(struct rtgl_program* program);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTGL_EXTERN_C_EXIT
#endif /* RTGL_PROGRAM_H */
