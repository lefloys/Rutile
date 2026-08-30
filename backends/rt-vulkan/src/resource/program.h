#ifndef RTVK_PROGRAM_H
#define RTVK_PROGRAM_H

#include "config.h"
#include "resource.h"

#include <volk.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTVK_API rt_program rtProgramCreate(void);
RTVK_API void rtProgramDestroy(rt_program program);

RTVK_API void rtProgramSetLayout(rt_program program, const rt_vertex_layout* layout);
RTVK_API void rtProgramSource(rt_program program, const char* entry_point, const u08* data, usize size);
RTVK_API void rtProgramSetRasterState(rt_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode);
RTVK_API void rtProgramSetBlendState(rt_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op);
RTVK_API void rtProgramFinalize(rt_program program);
RTVK_API rt_location rtProgramUniformLocation(rt_program program, const char* name);
RTVK_API rt_location rtProgramInputLocation(rt_program program, const rt_vertex_attribute* attributes, usize attribute_count);
RTVK_API rt_location rtProgramOutputLocation(rt_program program, const char* name);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

typedef enum rtvk_program_location_kind {
	RTVK_LOCATION_VERTEX,
	RTVK_LOCATION_OUTPUT,
	RTVK_LOCATION_BUFFER,
	RTVK_LOCATION_STORAGE_BUFFER,
	RTVK_LOCATION_TEXTURE,
} rtvk_program_location_kind;

struct rt_location_t {
	u08 address;
	struct rtvk_program* program;
	char name[RTVK_MAX_SHADER_UNIFORM_NAME];
	VkShaderStageFlags stages;
	rtvk_program_location_kind kind;
	u32 binding;
	u32 shader_location;
};

struct rtvk_program_shader {
	VkShaderModule vk_shader;
	VkShaderStageFlagBits stage;
	char entry_point[RTVK_MAX_SHADER_UNIFORM_NAME];
};

struct rtvk_graphics_pipeline_variant {
	struct rtvk_graphics_pipeline_variant* next;
	VkPipeline vk_pipeline;
	VkFormat color_formats[RTVK_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS];
	VkFormat depth_format;
	VkFormat stencil_format;
	VkSampleCountFlagBits rasterization_samples;
	u32 color_format_count;
};

struct rtvk_program {
	struct rtvk_resource_base base;
	struct rtvk_program_shader* shaders;
	VkDescriptorSetLayout vk_descriptor_set_layout;
	VkPipelineLayout vk_pipeline_layout;
	struct rtvk_graphics_pipeline_variant* pipeline_variants;

	char* entry_point;
	char* program_source;

	rt_vertex_layout vertex_layout;
	rt_vertex_input vertex_inputs[RTVK_MAX_VERTEX_ATTRIBUTES];
	rt_vertex_attribute vertex_attributes[RTVK_MAX_VERTEX_ATTRIBUTES];
	const rt_vertex_attribute* vertex_attribute_sources[RTVK_MAX_VERTEX_ATTRIBUTES];
	struct rt_location_t locations[256];
	bool location_occupied[256];
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

	u64 program_source_size;
	usize vertex_attribute_count;
	u32 shader_count;
	u32 shader_capacity;
	u32 location_count;
};
RTVK_DECLARE_NEW_RESOURCE(program)

VkPipeline rtvk_program_prepare(struct rtvk_context* ctx, struct rtvk_program* program, const VkFormat* color_formats, u32 color_format_count, VkFormat depth_format, VkFormat stencil_format, VkSampleCountFlagBits rasterization_samples);
void rtvk_program_layout(struct rtvk_context* ctx, struct rtvk_program* program, const rt_vertex_layout* layout);
void rtvk_program_source(struct rtvk_context* ctx, struct rtvk_program* program, const char* entry_point, const u08* data, usize size);
void rtvk_program_raster_state(struct rtvk_context* ctx, struct rtvk_program* program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode);
void rtvk_program_blend_state(struct rtvk_context* ctx, struct rtvk_program* program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op);
void rtvk_program_finalize(struct rtvk_context* ctx, struct rtvk_program* program);
rt_location rtvk_program_uniform_location(struct rtvk_program* program, const char* name);
rt_location rtvk_program_input_location(struct rtvk_program* program, const rt_vertex_attribute* attributes, usize attribute_count);
rt_location rtvk_program_output_location(struct rtvk_program* program, const char* name);
struct rt_location_t* rtvk_program_find_location(struct rtvk_program* program, const char* name);
void rtvk_program_clear_locations(struct rtvk_program* program);
void rtvk_program_destroy_pipeline(struct rtvk_context* ctx, struct rtvk_program* program);
void rtvk_program_destroy_pipeline_layout(struct rtvk_context* ctx, struct rtvk_program* program);

#endif
