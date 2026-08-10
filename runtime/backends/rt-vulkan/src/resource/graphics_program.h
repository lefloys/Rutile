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

RTVK_API void rtGraphicsProgramLayout(rt_graphics_program program, const rt_vertex_layout* layout);
RTVK_API void rtGraphicsProgramSource(rt_graphics_program program, u64 size, const void* data);
RTVK_API void rtGraphicsProgramRasterState(rt_graphics_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode);
RTVK_API void rtGraphicsProgramBlendState(rt_graphics_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op);
RTVK_API void rtGraphicsProgramFinalize(rt_graphics_program program);
RTVK_API void rtGraphicsProgramReset(rt_graphics_program program);
RTVK_API rt_uniform_location rtGraphicsProgramUniformLocation(rt_graphics_program program, const char* name);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

typedef enum rtvk_uniform_location_kind {
	RTVK_UNIFORM_LOCATION_BUFFER,
	RTVK_UNIFORM_LOCATION_STORAGE_BUFFER,
	RTVK_UNIFORM_LOCATION_TEXTURE,
} rtvk_uniform_location_kind;

struct rtvk_uniform_location {
	struct rtvk_graphics_program* program;
	char name[RTVK_MAX_SHADER_UNIFORM_NAME];
	VkShaderStageFlags stages;
	rtvk_uniform_location_kind kind;
	u32 binding;
	u32 index;
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
	rt_vertex_attribute vertex_attributes[RTVK_MAX_VERTEX_ATTRIBUTES];
	struct rtvk_uniform_location* uniform_locations;
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
	u32 uniform_location_count;
	u32 uniform_location_capacity;
};
RTVK_DECLARE_NEW_RESOURCE(graphics_program)

static inline struct rtvk_uniform_location* rtvk_uniform_location_from_handle(rt_uniform_location location) { return (struct rtvk_uniform_location*)location; }
static inline rt_uniform_location rtvk_uniform_location_to_handle(struct rtvk_uniform_location* location) { return (rt_uniform_location)location; }

VkPipeline rtvk_graphics_program_prepare(struct rtvk_context* ctx, struct rtvk_graphics_program* program, const VkFormat* color_formats, u32 color_format_count, VkFormat depth_format, VkFormat stencil_format, VkSampleCountFlagBits rasterization_samples);
void rtvk_graphics_program_layout(struct rtvk_context* ctx, struct rtvk_graphics_program* program, const rt_vertex_layout* layout);
void rtvk_graphics_program_source(struct rtvk_context* ctx, struct rtvk_graphics_program* program, u64 size, const void* data);
void rtvk_graphics_program_raster_state(struct rtvk_context* ctx, struct rtvk_graphics_program* program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode);
void rtvk_graphics_program_blend_state(struct rtvk_context* ctx, struct rtvk_graphics_program* program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op);
void rtvk_graphics_program_finalize(struct rtvk_context* ctx, struct rtvk_graphics_program* program);
void rtvk_graphics_program_reset(struct rtvk_context* ctx, struct rtvk_graphics_program* program);
struct rtvk_uniform_location* rtvk_graphics_program_uniform_location(struct rtvk_context* ctx, struct rtvk_graphics_program* program, const char* name);
void rtvk_graphics_program_clear_uniform_locations(struct rtvk_graphics_program* program);
void rtvk_graphics_program_destroy_pipeline(struct rtvk_context* ctx, struct rtvk_graphics_program* program);
void rtvk_graphics_program_destroy_pipeline_layout(struct rtvk_context* ctx, struct rtvk_graphics_program* program);

#endif
