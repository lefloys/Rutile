#ifndef RTVK_GRAPHICS_PROGRAM_H
#define RTVK_GRAPHICS_PROGRAM_H

#include "config.h"
#include "resource.h"

#include <volk.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTVK_API rt_graphics_program rtGraphicsProgramCreate(void);
RTVK_API void rtGraphicsProgramDestroy(rt_graphics_program program);

RTVK_API void rtGraphicsProgramSetLayout(rt_graphics_program program, const rt_vertex_layout* layout);
RTVK_API void rtGraphicsProgramSetSource(rt_graphics_program program, const u08* data, usize size);
RTVK_API void rtGraphicsProgramSetRasterState(rt_graphics_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode);
RTVK_API void rtGraphicsProgramSetBlendState(rt_graphics_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op);
RTVK_API void rtGraphicsProgramFinalize(rt_graphics_program program);
RTVK_API rt_location rtGraphicsProgramUniformLocation(rt_graphics_program program, const char* name);
RTVK_API rt_location rtGraphicsProgramInputLocation(rt_graphics_program program, const rt_vertex_attribute* attributes, usize attribute_count);
RTVK_API rt_location rtGraphicsProgramOutputLocation(rt_graphics_program program, const char* name);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

typedef enum rtvk_location_kind {
	RTVK_LOCATION_VERTEX,
	RTVK_LOCATION_OUTPUT,
	RTVK_LOCATION_BUFFER,
	RTVK_LOCATION_STORAGE_BUFFER,
	RTVK_LOCATION_TEXTURE,
} rtvk_location_kind;

struct rt_location_t {
	struct rtvk_graphics_program* program;
	char name[RTVK_MAX_SHADER_UNIFORM_NAME];
	VkShaderStageFlags stages;
	rtvk_location_kind kind;
	u32 binding;
	u32 index;
	u32 shader_location;
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

struct rtvk_graphics_program {
	struct rtvk_resource_base base;
	VkShaderModule vk_vertex_shader;
	VkShaderModule vk_fragment_shader;
	VkDescriptorSetLayout vk_descriptor_set_layout;
	VkPipelineLayout vk_pipeline_layout;
	struct rtvk_graphics_pipeline_variant* pipeline_variants;

	char* program_source;

	rt_vertex_layout vertex_layout;
	rt_vertex_input vertex_inputs[RTVK_MAX_VERTEX_ATTRIBUTES];
	rt_vertex_attribute vertex_attributes[RTVK_MAX_VERTEX_ATTRIBUTES];
	const rt_vertex_attribute* vertex_attribute_sources[RTVK_MAX_VERTEX_ATTRIBUTES];
	rt_location locations;
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
	u32 location_count;
	u32 location_capacity;
};
RTVK_DECLARE_NEW_RESOURCE(graphics_program)

VkPipeline rtvk_graphics_program_prepare(struct rtvk_context* ctx, struct rtvk_graphics_program* program, const VkFormat* color_formats, u32 color_format_count, VkFormat depth_format, VkFormat stencil_format, VkSampleCountFlagBits rasterization_samples);
void rtvk_graphics_program_layout(struct rtvk_context* ctx, struct rtvk_graphics_program* program, const rt_vertex_layout* layout);
void rtvk_graphics_program_source(struct rtvk_context* ctx, struct rtvk_graphics_program* program, const u08* data, usize size);
void rtvk_graphics_program_raster_state(struct rtvk_context* ctx, struct rtvk_graphics_program* program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode);
void rtvk_graphics_program_blend_state(struct rtvk_context* ctx, struct rtvk_graphics_program* program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op);
void rtvk_graphics_program_finalize(struct rtvk_context* ctx, struct rtvk_graphics_program* program);
rt_location rtvk_graphics_program_uniform_location(struct rtvk_context* ctx, struct rtvk_graphics_program* program, const char* name);
rt_location rtvk_graphics_program_input_location(struct rtvk_context* ctx, struct rtvk_graphics_program* program, const rt_vertex_attribute* attributes, usize attribute_count);
rt_location rtvk_graphics_program_output_location(struct rtvk_context* ctx, struct rtvk_graphics_program* program, const char* name);
rt_location rtvk_graphics_program_find_location(struct rtvk_graphics_program* program, const char* name);
void rtvk_graphics_program_clear_locations(struct rtvk_graphics_program* program);
void rtvk_graphics_program_destroy_pipeline(struct rtvk_context* ctx, struct rtvk_graphics_program* program);
void rtvk_graphics_program_destroy_pipeline_layout(struct rtvk_context* ctx, struct rtvk_graphics_program* program);

#endif
