#include "program.h"
#include "context.h"
#include "error.h"
#include "rtsl_spirv.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

rt_program rtProgramCreate(void) {
	return rtvk_program_to_handle(rtvk_program_create(rtvk_get_current_context()));
}

void rtProgramDestroy(rt_program program) {
	rtvk_program_destroy(
		rtvk_get_current_context(),
		rtvk_program_from_handle(program)
	);
}

void rtProgramSetLayout(rt_program program, const rt_vertex_layout* layout) {
	rtvk_program_layout(
		rtvk_get_current_context(),
		rtvk_program_from_handle(program),
		layout
	);
}

void rtProgramSource(rt_program program, const char* entry_point, const u08* data, usize size) {
	rtvk_program_source(
		rtvk_get_current_context(),
		rtvk_program_from_handle(program),
		entry_point,
		data,
		size
	);
}

void rtProgramSetRasterState(rt_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) {
	rtvk_program_raster_state(rtvk_get_current_context(), rtvk_program_from_handle(program), cull_mode, front_face, fill_mode);
}

void rtProgramSetBlendState(rt_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op) {
	rtvk_program_blend_state(
		rtvk_get_current_context(),
		rtvk_program_from_handle(program),
		enabled,
		src_color,
		dst_color,
		color_op,
		src_alpha,
		dst_alpha,
		alpha_op
	);
}

void rtProgramFinalize(rt_program program) {
	rtvk_program_finalize(
		rtvk_get_current_context(),
		rtvk_program_from_handle(program)
	);
}

rt_location rtProgramUniformLocation(rt_program program, const char* name) {
	return rtvk_program_uniform_location(rtvk_program_from_handle(program), name);
}

rt_location rtProgramInputLocation(rt_program program, const rt_vertex_attribute* attributes, usize attribute_count) {
	return rtvk_program_input_location(rtvk_program_from_handle(program), attributes, attribute_count);
}

rt_location rtProgramOutputLocation(rt_program program, const char* name) {
	return rtvk_program_output_location(rtvk_program_from_handle(program), name);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTVK_DEFINE_RESOURCE_PRIVATE(program)

void rtvk_program_init(struct rtvk_context* ctx, struct rtvk_program* program) {
	rtvk_init_resource_base(ctx, RTVK_RESOURCE_BASE(program), program, rtvk_program_finalize_resource);
	program->cull_mode = RT_CULL_NONE;
	program->front_face = RT_FRONT_FACE_CCW;
	program->fill_mode = RT_FILL_SOLID;
	program->blend_enabled = false;
	program->src_color_blend = RT_BLEND_ONE;
	program->dst_color_blend = RT_BLEND_ZERO;
	program->color_blend_op = RT_BLEND_OP_ADD;
	program->src_alpha_blend = RT_BLEND_ONE;
	program->dst_alpha_blend = RT_BLEND_ZERO;
	program->alpha_blend_op = RT_BLEND_OP_ADD;
}

void rtvk_program_clear_locations(struct rtvk_program* program) {
	memset(program->locations, 0, sizeof(program->locations));
	memset(program->location_occupied, 0, sizeof(program->location_occupied));
	program->location_count = 0;
}

static void rtvk_program_clear_shaders(struct rtvk_context* ctx, struct rtvk_program* program) {
	for (u32 index = 0; index < program->shader_count; index++) {
		if (program->shaders[index].vk_shader) {
			vkDestroyShaderModule(ctx->vk_device, program->shaders[index].vk_shader, VK_ALLOCATOR);
		}
	}
	free(program->shaders);
	program->shaders = NULL;
	program->shader_count = 0;
	program->shader_capacity = 0;
}

void rtvk_program_destroy_pipeline(struct rtvk_context* ctx, struct rtvk_program* program) {
	struct rtvk_graphics_pipeline_variant* variant = program->pipeline_variants;
	while (variant) {
		struct rtvk_graphics_pipeline_variant* next = variant->next;
		vkDestroyPipeline(ctx->vk_device, variant->vk_pipeline, VK_ALLOCATOR);
		free(variant);
		variant = next;
	}
	program->pipeline_variants = NULL;
}

void rtvk_program_destroy_pipeline_layout(struct rtvk_context* ctx, struct rtvk_program* program) {
	rtvk_program_destroy_pipeline(ctx, program);
	if (program->vk_pipeline_layout) {
		vkDestroyPipelineLayout(ctx->vk_device, program->vk_pipeline_layout, VK_ALLOCATOR);
		program->vk_pipeline_layout = VK_NULL_HANDLE;
	}
	if (program->vk_descriptor_set_layout) {
		vkDestroyDescriptorSetLayout(ctx->vk_device, program->vk_descriptor_set_layout, VK_ALLOCATOR);
		program->vk_descriptor_set_layout = VK_NULL_HANDLE;
	}
}

enum rtvk_spirv_opcode {
	RTVK_SPIRV_OPCODE_NAME = 5,
	RTVK_SPIRV_OPCODE_DECORATE = 71,
};

enum rtvk_spirv_decoration {
	RTVK_SPIRV_DECORATION_LOCATION = 30,
};

static VkFormat rtvk_vertex_format(enum rt_format format) {
	switch (format) {
	case RT_R32_SFLOAT:
		return VK_FORMAT_R32_SFLOAT;
	case RT_RG32_SFLOAT:
		return VK_FORMAT_R32G32_SFLOAT;
	case RT_RGB32_SFLOAT:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case RT_RGBA32_SFLOAT:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case RT_R32_SINT:
		return VK_FORMAT_R32_SINT;
	case RT_RG32_SINT:
		return VK_FORMAT_R32G32_SINT;
	case RT_RGB32_SINT:
		return VK_FORMAT_R32G32B32_SINT;
	case RT_RGBA32_SINT:
		return VK_FORMAT_R32G32B32A32_SINT;
	case RT_R32_UINT:
		return VK_FORMAT_R32_UINT;
	case RT_RG32_UINT:
		return VK_FORMAT_R32G32_UINT;
	case RT_RGB32_UINT:
		return VK_FORMAT_R32G32B32_UINT;
	case RT_RGBA32_UINT:
		return VK_FORMAT_R32G32B32A32_UINT;
	default:
		return VK_FORMAT_UNDEFINED;
	}
}

void rtvk_program_finish(struct rtvk_program* program) {
	struct rtvk_context* ctx = program->base.ctx;
	rtvk_program_destroy_pipeline_layout(ctx, program);
	rtvk_program_clear_shaders(ctx, program);
	free(program->entry_point);
	program->entry_point = NULL;
	free(program->program_source);
	program->program_source = NULL;
	program->program_source_size = 0;
	rtvk_program_clear_locations(program);
}

static VkDescriptorType rtvk_program_descriptor_type(rtvk_program_location_kind kind) {
	switch (kind) {
	case RTVK_LOCATION_BUFFER:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case RTVK_LOCATION_STORAGE_BUFFER:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case RTVK_LOCATION_TEXTURE:
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	case RTVK_LOCATION_VERTEX:
	case RTVK_LOCATION_OUTPUT:
		break;
	}
	return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

static void rtvk_program_create_descriptor_set_layout(struct rtvk_context* ctx, struct rtvk_program* program) {
	u32 descriptor_count = 0;
	for (u32 index = 0; index < program->location_count; index++) {
		if (program->locations[index].kind != RTVK_LOCATION_VERTEX && program->locations[index].kind != RTVK_LOCATION_OUTPUT) {
			descriptor_count++;
		}
	}
	if (descriptor_count == 0) {
		return;
	}

	VkDescriptorSetLayoutBinding* bindings = calloc(descriptor_count, sizeof(*bindings));
	if (!bindings) {
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for descriptor set layout bindings", (usize)descriptor_count * sizeof(*bindings));
		return;
	}

	u32 binding_index = 0;
	for (u32 index = 0; index < program->location_count; index++) {
		struct rt_location_t* location = &program->locations[index];
		if (location->kind == RTVK_LOCATION_VERTEX || location->kind == RTVK_LOCATION_OUTPUT) {
			continue;
		}
		bindings[binding_index].binding = location->binding;
		bindings[binding_index].descriptorType = rtvk_program_descriptor_type(location->kind);
		bindings[binding_index].descriptorCount = 1;
		bindings[binding_index].stageFlags = location->stages;
		bindings[binding_index].pImmutableSamplers = NULL;
		binding_index++;
	}

	VkDescriptorSetLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layout_info.pNext = NULL;
	layout_info.flags = 0;
	layout_info.bindingCount = descriptor_count;
	layout_info.pBindings = bindings;

	VkResult result = vkCreateDescriptorSetLayout(ctx->vk_device, &layout_info, VK_ALLOCATOR, &program->vk_descriptor_set_layout);
	free(bindings);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return;
	}
}

static void rtvk_program_create_pipeline_layout(struct rtvk_context* ctx, struct rtvk_program* program) {
	VkPipelineLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

	if (!program->vk_descriptor_set_layout) {
		rtvk_program_create_descriptor_set_layout(ctx, program);
		if (rtvk_error() != RT_SUCCESS) {
			return;
		}
	}

	layout_info.pNext = NULL;
	layout_info.flags = 0;
	layout_info.setLayoutCount = program->vk_descriptor_set_layout ? 1 : 0;
	layout_info.pSetLayouts = program->vk_descriptor_set_layout ? &program->vk_descriptor_set_layout : NULL;
	layout_info.pushConstantRangeCount = 0;
	layout_info.pPushConstantRanges = NULL;

	VkResult result = vkCreatePipelineLayout(ctx->vk_device, &layout_info, VK_ALLOCATOR, &program->vk_pipeline_layout);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return;
	}
}

static void rtvk_program_viewport_state(VkViewport* viewport, VkRect2D* scissor, VkPipelineViewportStateCreateInfo* viewport_info) {
	viewport->x = 0.0f;
	viewport->y = 0.0f;
	viewport->width = 1.0f;
	viewport->height = 1.0f;
	viewport->minDepth = 0.0f;
	viewport->maxDepth = 1.0f;

	scissor->offset.x = 0;
	scissor->offset.y = 0;
	scissor->extent.width = 1;
	scissor->extent.height = 1;

	*viewport_info = (VkPipelineViewportStateCreateInfo){ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewport_info->viewportCount = 1;
	viewport_info->pViewports = viewport;
	viewport_info->scissorCount = 1;
	viewport_info->pScissors = scissor;
}

static void rtvk_program_color_blend_state(struct rtvk_program* program, VkPipelineColorBlendAttachmentState* attachments, u32 attachment_count, VkPipelineColorBlendStateCreateInfo* color_blend_info) {
	static const VkBlendFactor blend_factors[] = {
		VK_BLEND_FACTOR_ZERO,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_FACTOR_SRC_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		VK_BLEND_FACTOR_DST_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
		VK_BLEND_FACTOR_SRC_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		VK_BLEND_FACTOR_DST_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
	};
	static const VkBlendOp blend_ops[] = {
		VK_BLEND_OP_ADD,
		VK_BLEND_OP_SUBTRACT,
		VK_BLEND_OP_REVERSE_SUBTRACT,
		VK_BLEND_OP_MIN,
		VK_BLEND_OP_MAX,
	};
	for (u32 i = 0; i < attachment_count; i++) {
		attachments[i].blendEnable = program->blend_enabled ? VK_TRUE : VK_FALSE;
		attachments[i].srcColorBlendFactor = blend_factors[program->src_color_blend];
		attachments[i].dstColorBlendFactor = blend_factors[program->dst_color_blend];
		attachments[i].colorBlendOp = blend_ops[program->color_blend_op];
		attachments[i].srcAlphaBlendFactor = blend_factors[program->src_alpha_blend];
		attachments[i].dstAlphaBlendFactor = blend_factors[program->dst_alpha_blend];
		attachments[i].alphaBlendOp = blend_ops[program->alpha_blend_op];
		attachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}

	*color_blend_info = (VkPipelineColorBlendStateCreateInfo){ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	color_blend_info->logicOpEnable = VK_FALSE;
	color_blend_info->logicOp = VK_LOGIC_OP_COPY;
	color_blend_info->attachmentCount = attachment_count;
	color_blend_info->pAttachments = attachments;
}

static VkPipeline rtvk_program_create_pipeline(struct rtvk_context* ctx, struct rtvk_program* program, const VkFormat* color_formats, u32 color_format_count, VkFormat depth_format, VkFormat stencil_format, VkSampleCountFlagBits rasterization_samples) {
	VkPipelineShaderStageCreateInfo* stages = calloc(program->shader_count, sizeof(*stages));
	if (!stages) {
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for Vulkan shader stages", sizeof(*stages) * program->shader_count);
		return VK_NULL_HANDLE;
	}
	for (u32 index = 0; index < program->shader_count; index++) {
		stages[index] = (VkPipelineShaderStageCreateInfo){ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stages[index].stage = program->shaders[index].stage;
		stages[index].module = program->shaders[index].vk_shader;
		stages[index].pName = program->shaders[index].entry_point;
	}

	VkVertexInputBindingDescription bindings[RTVK_MAX_VERTEX_ATTRIBUTES] = { 0 };
	VkVertexInputAttributeDescription attributes[RTVK_MAX_VERTEX_ATTRIBUTES] = { 0 };
	VkPipelineVertexInputStateCreateInfo vertex_input_info = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	if (program->vertex_attribute_count) {
		for (u32 i = 0; i < program->vertex_layout.input_count; i++) {
			bindings[i].binding = i;
			bindings[i].stride = (u32)program->vertex_inputs[i].stride;
			bindings[i].inputRate = program->vertex_inputs[i].rate == RT_VERTEX_RATE_INSTANCE
										? VK_VERTEX_INPUT_RATE_INSTANCE
										: VK_VERTEX_INPUT_RATE_VERTEX;
		}

		for (u32 i = 0; i < program->vertex_attribute_count; i++) {
			const rt_vertex_attribute* attribute = &program->vertex_attributes[i];
			struct rt_location_t* location = rtvk_program_find_location(program, attribute->name);
			if (!location || location->kind != RTVK_LOCATION_VERTEX) {
				free(stages);
				rtvk_throwf(RT_SHADER_LINK_FAILED, "vertex attribute %s has no Vulkan location", attribute->name);
				return VK_NULL_HANDLE;
			}
			attributes[i].location = location->shader_location;
			attributes[i].binding = location->binding;
			attributes[i].format = rtvk_vertex_format(attribute->format);
			attributes[i].offset = attribute->offset;
			if (attributes[i].format == VK_FORMAT_UNDEFINED) {
				free(stages);
				rtvk_throwf(RT_UNSUPPORTED_FEATURE, "unsupported vertex attribute format");
				return VK_NULL_HANDLE;
			}
		}

		vertex_input_info.vertexBindingDescriptionCount = (u32)program->vertex_layout.input_count;
		vertex_input_info.pVertexBindingDescriptions = bindings;
		vertex_input_info.vertexAttributeDescriptionCount = (u32)program->vertex_attribute_count;
		vertex_input_info.pVertexAttributeDescriptions = attributes;
	} else {
		vertex_input_info.vertexBindingDescriptionCount = 0;
		vertex_input_info.pVertexBindingDescriptions = NULL;
		vertex_input_info.vertexAttributeDescriptionCount = 0;
		vertex_input_info.pVertexAttributeDescriptions = NULL;
	}

	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_info.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = { 0 };
	VkRect2D scissor = { 0 };
	VkPipelineViewportStateCreateInfo viewport_info;
	rtvk_program_viewport_state(&viewport, &scissor, &viewport_info);

	VkPipelineRasterizationStateCreateInfo raster_info = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	raster_info.pNext = NULL;
	raster_info.flags = 0;
	raster_info.depthClampEnable = VK_FALSE;
	raster_info.rasterizerDiscardEnable = VK_FALSE;
	static const VkPolygonMode fill_modes[] = { VK_POLYGON_MODE_FILL, VK_POLYGON_MODE_LINE };
	static const VkCullModeFlags cull_modes[] = { VK_CULL_MODE_NONE, VK_CULL_MODE_FRONT_BIT, VK_CULL_MODE_BACK_BIT };
	static const VkFrontFace front_faces[] = { VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FRONT_FACE_CLOCKWISE };
	raster_info.polygonMode = fill_modes[program->fill_mode];
	raster_info.cullMode = cull_modes[program->cull_mode];
	raster_info.frontFace = front_faces[program->front_face];
	raster_info.depthBiasEnable = VK_FALSE;
	raster_info.depthBiasConstantFactor = 0.0f;
	raster_info.depthBiasClamp = 0.0f;
	raster_info.depthBiasSlopeFactor = 0.0f;
	raster_info.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample_info = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisample_info.pNext = NULL;
	multisample_info.flags = 0;
	multisample_info.rasterizationSamples = rasterization_samples;
	multisample_info.sampleShadingEnable = VK_FALSE;
	multisample_info.minSampleShading = 1.0f;
	multisample_info.pSampleMask = NULL;
	multisample_info.alphaToCoverageEnable = VK_FALSE;
	multisample_info.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState color_blend_attachments[RTVK_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS] = { 0 };
	VkPipelineColorBlendStateCreateInfo color_blend_info;
	rtvk_program_color_blend_state(program, color_blend_attachments, color_format_count, &color_blend_info);

	VkPipelineDepthStencilStateCreateInfo depth_info = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depth_info.pNext = NULL;
	depth_info.flags = 0;
	depth_info.depthTestEnable = depth_format != VK_FORMAT_UNDEFINED;
	depth_info.depthWriteEnable = depth_format != VK_FORMAT_UNDEFINED;
	depth_info.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_info.depthBoundsTestEnable = VK_FALSE;
	depth_info.stencilTestEnable = VK_FALSE;
	depth_info.front = (VkStencilOpState){ 0 };
	depth_info.back = (VkStencilOpState){ 0 };
	depth_info.minDepthBounds = 0.0f;
	depth_info.maxDepthBounds = 1.0f;

	VkDynamicState dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	VkPipelineDynamicStateCreateInfo dynamic_info = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamic_info.pNext = NULL;
	dynamic_info.flags = 0;
	dynamic_info.dynamicStateCount = (u32)(sizeof(dynamic_states) / sizeof(dynamic_states[0]));
	dynamic_info.pDynamicStates = dynamic_states;

	VkPipelineRenderingCreateInfo rendering_info = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	rendering_info.pNext = NULL;
	rendering_info.viewMask = 0;
	rendering_info.colorAttachmentCount = color_format_count;
	rendering_info.pColorAttachmentFormats = color_formats;
	rendering_info.depthAttachmentFormat = depth_format;
	rendering_info.stencilAttachmentFormat = stencil_format;

	VkGraphicsPipelineCreateInfo pipeline_info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipeline_info.pNext = &rendering_info;
	pipeline_info.flags = 0;
	pipeline_info.stageCount = program->shader_count;
	pipeline_info.pStages = stages;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly_info;
	pipeline_info.pTessellationState = NULL;
	pipeline_info.pViewportState = &viewport_info;
	pipeline_info.pRasterizationState = &raster_info;
	pipeline_info.pMultisampleState = &multisample_info;
	pipeline_info.pDepthStencilState = &depth_info;
	pipeline_info.pColorBlendState = &color_blend_info;
	pipeline_info.pDynamicState = &dynamic_info;
	pipeline_info.layout = program->vk_pipeline_layout;
	pipeline_info.renderPass = VK_NULL_HANDLE;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_info.basePipelineIndex = -1;

	VkPipeline pipeline = VK_NULL_HANDLE;
	VkResult result = vkCreateGraphicsPipelines(ctx->vk_device, VK_NULL_HANDLE, 1, &pipeline_info, VK_ALLOCATOR, &pipeline);
	free(stages);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return VK_NULL_HANDLE;
	}
	return pipeline;
}

VkPipeline rtvk_program_prepare(struct rtvk_context* ctx, struct rtvk_program* program, const VkFormat* color_formats, u32 color_format_count, VkFormat depth_format, VkFormat stencil_format, VkSampleCountFlagBits rasterization_samples) {
	if (!program || !program->vk_pipeline_layout || !color_formats || color_format_count == 0 || color_format_count > RTVK_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS) {
		rtvk_throwf(RT_IMPROPER_USAGE, "program must be finalized before use");
		return VK_NULL_HANDLE;
	}

	for (struct rtvk_graphics_pipeline_variant* variant = program->pipeline_variants; variant; variant = variant->next) {
		if (variant->color_format_count == color_format_count && variant->depth_format == depth_format &&
			variant->stencil_format == stencil_format && variant->rasterization_samples == rasterization_samples &&
			memcmp(variant->color_formats, color_formats, sizeof(*color_formats) * color_format_count) == 0) {
			return variant->vk_pipeline;
		}
	}

	VkPipeline pipeline = rtvk_program_create_pipeline(ctx, program, color_formats, color_format_count, depth_format, stencil_format, rasterization_samples);
	if (!pipeline) {
		return VK_NULL_HANDLE;
	}
	struct rtvk_graphics_pipeline_variant* variant = calloc(1, sizeof(*variant));
	if (!variant) {
		vkDestroyPipeline(ctx->vk_device, pipeline, VK_ALLOCATOR);
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for graphics pipeline variant", sizeof(*variant));
		return VK_NULL_HANDLE;
	}
	variant->vk_pipeline = pipeline;
	memcpy(variant->color_formats, color_formats, sizeof(*color_formats) * color_format_count);
	variant->depth_format = depth_format;
	variant->stencil_format = stencil_format;
	variant->rasterization_samples = rasterization_samples;
	variant->color_format_count = color_format_count;
	variant->next = program->pipeline_variants;
	program->pipeline_variants = variant;
	return pipeline;
}

void rtvk_program_layout(struct rtvk_context* ctx, struct rtvk_program* program, const rt_vertex_layout* layout) {
	assert(ctx);
	assert(program);
	if (!layout || !layout->inputs || layout->input_count == 0) {
		program->vertex_layout = (rt_vertex_layout){ 0 };
		program->vertex_attribute_count = 0;
		rtvk_program_destroy_pipeline_layout(ctx, program);
		rtvk_program_clear_shaders(ctx, program);
		rtvk_program_clear_locations(program);
		return;
	}
	if (layout->input_count > RTVK_MAX_VERTEX_ATTRIBUTES) {
		rtvk_throwf(RT_IMPROPER_USAGE, "too many vertex inputs");
		return;
	}

	usize attribute_count = 0;
	for (usize input_index = 0; input_index < layout->input_count; input_index++) {
		const rt_vertex_input* input = &layout->inputs[input_index];
		if (!input->attributes || input->attribute_count == 0 || input->stride == 0) {
			rtvk_throwf(RT_IMPROPER_USAGE, "vertex input must have attributes and a nonzero stride");
			return;
		}
		if (input->attribute_count > RTVK_MAX_VERTEX_ATTRIBUTES - attribute_count) {
			rtvk_throwf(RT_IMPROPER_USAGE, "too many vertex attributes");
			return;
		}
		attribute_count += input->attribute_count;
	}

	usize attribute_offset = 0;
	for (usize input_index = 0; input_index < layout->input_count; input_index++) {
		const rt_vertex_input* input = &layout->inputs[input_index];
		program->vertex_inputs[input_index] = *input;
		program->vertex_attribute_sources[input_index] = input->attributes;
		program->vertex_inputs[input_index].attributes = &program->vertex_attributes[attribute_offset];
		memcpy(&program->vertex_attributes[attribute_offset], input->attributes, sizeof(*input->attributes) * input->attribute_count);
		attribute_offset += input->attribute_count;
	}
	program->vertex_layout.inputs = program->vertex_inputs;
	program->vertex_layout.input_count = layout->input_count;
	program->vertex_attribute_count = attribute_count;
	rtvk_program_destroy_pipeline_layout(ctx, program);
}

void rtvk_program_source(struct rtvk_context* ctx, struct rtvk_program* program, const char* entry_point, const u08* data, usize size) {
	assert(program);
	if (!entry_point || !entry_point[0]) {
		rtvk_throwf(RT_IMPROPER_USAGE, "program entry point is empty");
		return;
	}
	if (!data || size == 0) {
		rtvk_throwf(RT_IMPROPER_USAGE, "program source data is empty");
		return;
	}
	const size_t entry_point_size = strlen(entry_point) + 1;
	char* new_entry_point = malloc(entry_point_size);
	if (!new_entry_point) {
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for program entry-point storage", entry_point_size);
		return;
	}
	char* new_source = malloc((size_t)size);
	if (!new_source) {
		free(new_entry_point);
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for shader source storage", size);
		return;
	}
	memcpy(new_entry_point, entry_point, entry_point_size);
	memcpy(new_source, data, (size_t)size);

	rtvk_program_destroy_pipeline_layout(ctx, program);
	rtvk_program_clear_shaders(ctx, program);
	free(program->entry_point);
	free(program->program_source);
	program->entry_point = NULL;
	program->program_source = NULL;
	program->program_source_size = 0;
	rtvk_program_clear_locations(program);
	program->entry_point = new_entry_point;
	program->program_source = new_source;
	program->program_source_size = size;
}

void rtvk_program_raster_state(struct rtvk_context* ctx, struct rtvk_program* program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) {
	assert(ctx);
	assert(program);

	program->cull_mode = cull_mode;
	program->front_face = front_face;
	program->fill_mode = fill_mode;
	rtvk_program_destroy_pipeline(ctx, program);
}

void rtvk_program_blend_state(struct rtvk_context* ctx, struct rtvk_program* program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op) {
	assert(ctx);
	assert(program);

	program->blend_enabled = enabled;
	program->src_color_blend = src_color;
	program->dst_color_blend = dst_color;
	program->color_blend_op = color_op;
	program->src_alpha_blend = src_alpha;
	program->dst_alpha_blend = dst_alpha;
	program->alpha_blend_op = alpha_op;
	rtvk_program_destroy_pipeline(ctx, program);
}

struct rt_location_t* rtvk_program_find_location(struct rtvk_program* program, const char* name) {
	assert(program);
	assert(name);
	for (u32 i = 0; i < program->location_count; i++) {
		if (program->location_occupied[i] && strcmp(program->locations[i].name, name) == 0) {
			return &program->locations[i];
		}
	}
	return NULL;
}

static bool rtvk_program_reserve_locations(struct rtvk_program* program, u32 count) {
	assert(program);
	if (count > 256) {
		rtvk_throwf(RT_SHADER_LINK_FAILED, "program exposes more than 256 locations");
		return false;
	}
	return true;
}

static bool rtvk_program_build_locations(
	struct rtvk_program* program,
	const rtsl_spirv_translation* translation
) {
	assert(program);
	assert(translation);

	rtvk_program_clear_locations(program);
	const u32 resource_count = rtsl_spirv_resource_count(translation);
	const u32 vertex_attribute_count = (u32)program->vertex_attribute_count;
	u64 vertex_word_count = 0;
	const u32* vertex_words = rtsl_spirv_stage_words(translation, RTSL_SPIRV_VERTEX, &vertex_word_count);
	if (!vertex_words || vertex_word_count < 5) {
		rtvk_throwf(RT_SHADER_LINK_FAILED, "RTSL vertex shader is missing SPIR-V instructions");
		return false;
	}
	u64 fragment_word_count = 0;
	const u32* fragment_words = rtsl_spirv_stage_words(translation, RTSL_SPIRV_FRAGMENT, &fragment_word_count);
	if (!fragment_words || fragment_word_count < 5) {
		rtvk_throwf(RT_SHADER_LINK_FAILED, "RTSL fragment shader is missing SPIR-V instructions");
		return false;
	}

	u32 fragment_output_count = 0;
	for (u64 word_index = 5; word_index < fragment_word_count; (void)0) {
		const u32 instruction = fragment_words[word_index];
		const u32 instruction_word_count = instruction >> 16;
		const u32 opcode = instruction & 0xffff;
		if (instruction_word_count == 0 || word_index + instruction_word_count > fragment_word_count) {
			rtvk_throwf(RT_SHADER_LINK_FAILED, "RTSL fragment shader contains an invalid SPIR-V instruction");
			return false;
		}
		if (opcode == RTVK_SPIRV_OPCODE_NAME && instruction_word_count >= 3) {
			const char* name = (const char*)&fragment_words[word_index + 2];
			if (strncmp(name, "out_", 4) == 0) {
				fragment_output_count++;
			}
		}
		word_index += instruction_word_count;
	}
	if (!rtvk_program_reserve_locations(program, resource_count + vertex_attribute_count + fragment_output_count)) {
		return false;
	}

	usize input_index = 0;
	usize input_attribute_end = program->vertex_layout.input_count ? program->vertex_inputs[0].attribute_count : 0;
	for (u32 attribute_index = 0; attribute_index < vertex_attribute_count; attribute_index++) {
		while (attribute_index >= input_attribute_end && input_index + 1 < program->vertex_layout.input_count) {
			input_index++;
			input_attribute_end += program->vertex_inputs[input_index].attribute_count;
		}
		const rt_vertex_attribute* attribute = &program->vertex_attributes[attribute_index];
		if (!attribute->name || strlen(attribute->name) >= RTVK_MAX_SHADER_UNIFORM_NAME) {
			rtvk_throwf(RT_SHADER_LINK_FAILED, "vertex attribute name is missing or exceeds the Vulkan backend limit");
			return false;
		}
		for (u32 location_index = 0; location_index < program->location_count; location_index++) {
			if (program->locations[location_index].kind == RTVK_LOCATION_VERTEX && strcmp(program->locations[location_index].name, attribute->name) == 0) {
				rtvk_throwf(RT_SHADER_LINK_FAILED, "vertex attribute %s is declared more than once", attribute->name);
				return false;
			}
		}

		u32 shader_id = 0;
		for (u64 word_index = 5; word_index < vertex_word_count; (void)0) {
			const u32 instruction = vertex_words[word_index];
			const u32 instruction_word_count = instruction >> 16;
			const u32 opcode = instruction & 0xffff;
			if (instruction_word_count == 0 || word_index + instruction_word_count > vertex_word_count) {
				rtvk_throwf(RT_SHADER_LINK_FAILED, "RTSL vertex shader contains an invalid SPIR-V instruction");
				return false;
			}
			if (opcode == RTVK_SPIRV_OPCODE_NAME && instruction_word_count >= 3) {
				const char* name = (const char*)&vertex_words[word_index + 2];
				if (strncmp(name, "in_", 3) == 0 && strcmp(name + 3, attribute->name) == 0) {
					shader_id = vertex_words[word_index + 1];
					break;
				}
			}
			word_index += instruction_word_count;
		}
		if (shader_id == 0) {
			rtvk_throwf(RT_SHADER_LINK_FAILED, "vertex attribute %s is not declared by the RTSL vertex shader", attribute->name);
			return false;
		}

		u32 shader_location = 0;
		bool shader_location_found = false;
		for (u64 word_index = 5; word_index < vertex_word_count; (void)0) {
			const u32 instruction = vertex_words[word_index];
			const u32 instruction_word_count = instruction >> 16;
			const u32 opcode = instruction & 0xffff;
			if (instruction_word_count == 0 || word_index + instruction_word_count > vertex_word_count) {
				rtvk_throwf(RT_SHADER_LINK_FAILED, "RTSL vertex shader contains an invalid SPIR-V instruction");
				return false;
			}
			if (opcode == RTVK_SPIRV_OPCODE_DECORATE && instruction_word_count >= 4 && vertex_words[word_index + 1] == shader_id && vertex_words[word_index + 2] == RTVK_SPIRV_DECORATION_LOCATION) {
				shader_location = vertex_words[word_index + 3];
				shader_location_found = true;
				break;
			}
			word_index += instruction_word_count;
		}
		if (!shader_location_found) {
			rtvk_throwf(RT_SHADER_LINK_FAILED, "vertex attribute %s has no RTSL vertex location", attribute->name);
			return false;
		}

		struct rt_location_t* location = &program->locations[program->location_count];
		location->address = (u08)input_index;
		location->program = program;
		memcpy(location->name, attribute->name, strlen(attribute->name) + 1);
		location->stages = VK_SHADER_STAGE_VERTEX_BIT;
		location->kind = RTVK_LOCATION_VERTEX;
		location->binding = (u32)input_index;
		location->shader_location = shader_location;
		program->location_occupied[program->location_count++] = true;
	}

	for (u64 word_index = 5; word_index < fragment_word_count; (void)0) {
		const u32 instruction = fragment_words[word_index];
		const u32 instruction_word_count = instruction >> 16;
		const u32 opcode = instruction & 0xffff;
		if (instruction_word_count == 0 || word_index + instruction_word_count > fragment_word_count) {
			rtvk_throwf(RT_SHADER_LINK_FAILED, "RTSL fragment shader contains an invalid SPIR-V instruction");
			return false;
		}
		if (opcode != RTVK_SPIRV_OPCODE_NAME || instruction_word_count < 3) {
			word_index += instruction_word_count;
			continue;
		}

		const char* shader_name = (const char*)&fragment_words[word_index + 2];
		if (strncmp(shader_name, "out_", 4) != 0) {
			word_index += instruction_word_count;
			continue;
		}
		const char* name = shader_name + 4;
		if (strlen(name) >= RTVK_MAX_SHADER_UNIFORM_NAME) {
			rtvk_throwf(RT_SHADER_LINK_FAILED, "fragment output %s is invalid or conflicts with another program location", name);
			return false;
		}

		u32 shader_location = 0;
		bool shader_location_found = false;
		for (u64 decoration_index = 5; decoration_index < fragment_word_count; (void)0) {
			const u32 decoration = fragment_words[decoration_index];
			const u32 decoration_word_count = decoration >> 16;
			const u32 decoration_opcode = decoration & 0xffff;
			if (decoration_word_count == 0 || decoration_index + decoration_word_count > fragment_word_count) {
				rtvk_throwf(RT_SHADER_LINK_FAILED, "RTSL fragment shader contains an invalid SPIR-V instruction");
				return false;
			}
			if (decoration_opcode == RTVK_SPIRV_OPCODE_DECORATE && decoration_word_count >= 4 && fragment_words[decoration_index + 1] == fragment_words[word_index + 1] && fragment_words[decoration_index + 2] == RTVK_SPIRV_DECORATION_LOCATION) {
				shader_location = fragment_words[decoration_index + 3];
				shader_location_found = true;
				break;
			}
			decoration_index += decoration_word_count;
		}
		if (!shader_location_found) {
			rtvk_throwf(RT_SHADER_LINK_FAILED, "fragment output %s has no RTSL fragment location", name);
			return false;
		}
		for (u32 existing_index = 0; existing_index < program->location_count; existing_index++) {
			struct rt_location_t* existing = &program->locations[existing_index];
			if (existing->kind == RTVK_LOCATION_OUTPUT && ((name[0] && strcmp(existing->name, name) == 0) || existing->binding == shader_location)) {
				rtvk_throwf(RT_SHADER_LINK_FAILED, "fragment output %s is invalid or conflicts with another program location", name);
				return false;
			}
		}

		if (shader_location > UINT8_MAX) {
			rtvk_throwf(RT_SHADER_LINK_FAILED, "fragment output %s location exceeds the rt_location address range", name);
			return false;
		}
		struct rt_location_t* location = &program->locations[program->location_count];
		location->address = (u08)shader_location;
		location->program = program;
		memcpy(location->name, name, strlen(name) + 1);
		location->stages = VK_SHADER_STAGE_FRAGMENT_BIT;
		location->kind = RTVK_LOCATION_OUTPUT;
		location->binding = shader_location;
		location->shader_location = shader_location;
		program->location_occupied[program->location_count++] = true;
		word_index += instruction_word_count;
	}

	for (u32 i = 0; i < resource_count; i++) {
		rtsl_spirv_resource_info resource;
		if (!rtsl_spirv_resource(translation, i, &resource)) {
			rtvk_throwf(RT_SHADER_LINK_FAILED, "RTSL resource reflection failed at index %u", i);
			return false;
		}
		if (resource.descriptor_set != 0) {
			rtvk_throwf(RT_UNSUPPORTED_FEATURE, "RTSL resource %s uses unsupported descriptor set %u", resource.name, resource.descriptor_set);
			return false;
		}
		if (!resource.name || strlen(resource.name) >= RTVK_MAX_SHADER_UNIFORM_NAME) {
			rtvk_throwf(RT_SHADER_LINK_FAILED, "RTSL resource name is missing or exceeds the Vulkan backend limit");
			return false;
		}

		VkShaderStageFlags stages = 0;
		if (resource.stages & RTSL_SPIRV_STAGE_VERTEX) {
			stages |= VK_SHADER_STAGE_VERTEX_BIT;
		}
		if (resource.stages & RTSL_SPIRV_STAGE_FRAGMENT) {
			stages |= VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		if (!stages) {
			continue;
		}

		rtvk_program_location_kind kind;
		switch (resource.kind) {
		case RTSL_SPIRV_UNIFORM_BUFFER:
			kind = RTVK_LOCATION_BUFFER;
			break;
		case RTSL_SPIRV_STORAGE_BUFFER:
			kind = RTVK_LOCATION_STORAGE_BUFFER;
			break;
		case RTSL_SPIRV_SAMPLED_TEXTURE:
			kind = RTVK_LOCATION_TEXTURE;
			break;
		case RTSL_SPIRV_SAMPLER:
		case RTSL_SPIRV_STORAGE_IMAGE:
		default:
			rtvk_throwf(RT_UNSUPPORTED_FEATURE, "RTSL resource %s has an unsupported graphics resource kind", resource.name);
			return false;
		}

		struct rt_location_t* location = NULL;
		for (u32 existing_index = 0; existing_index < program->location_count; existing_index++) {
			struct rt_location_t* existing = &program->locations[existing_index];
			if (existing->kind == RTVK_LOCATION_VERTEX || existing->kind == RTVK_LOCATION_OUTPUT) {
				continue;
			}
			if (existing->binding != resource.binding) {
				continue;
			}
			if (existing->kind != kind) {
				rtvk_throwf(RT_SHADER_LINK_FAILED, "RTSL resources %s and %s use binding %u with different descriptor kinds", existing->name, resource.name, resource.binding);
				return false;
			}
			if (strcmp(existing->name, resource.name) != 0) {
				rtvk_throwf(RT_SHADER_LINK_FAILED, "RTSL resources %s and %s both use binding %u", existing->name, resource.name, resource.binding);
				return false;
			}
			location = existing;
			break;
		}

		if (location) {
			location->stages |= stages;
			continue;
		}

		if (resource.binding > UINT8_MAX) {
			rtvk_throwf(RT_SHADER_LINK_FAILED, "RTSL resource %s address exceeds the rt_location range", resource.name);
			return false;
		}
		location = &program->locations[program->location_count];
		location->address = (u08)resource.binding;
		location->program = program;
		memcpy(location->name, resource.name, strlen(resource.name) + 1);
		location->stages = stages;
		location->kind = kind;
		location->binding = resource.binding;
		program->location_occupied[program->location_count++] = true;
	}
	return true;
}

static bool rtvk_program_create_shader(
	struct rtvk_context* ctx,
	const u32* words,
	u64 word_count,
	VkShaderModule* shader
) {
	if (!words || word_count == 0) {
		rtvk_throwf(RT_SHADER_COMPILATION_FAILED, "RTSL transpiler returned an empty SPIR-V module");
		return false;
	}

	VkShaderModuleCreateInfo shader_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	shader_info.codeSize = (size_t)word_count * sizeof(*words);
	shader_info.pCode = words;
	const VkResult result = vkCreateShaderModule(ctx->vk_device, &shader_info, VK_ALLOCATOR, shader);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "vkCreateShaderModule failed: %s", rtvk_vk_result_name(result));
		return false;
	}
	return true;
}

static bool rtvk_program_add_shader(
	struct rtvk_context* ctx,
	struct rtvk_program* program,
	const rtsl_spirv_translation* translation,
	rtsl_spirv_stage translation_stage,
	VkShaderStageFlagBits vk_stage
) {
	const char* entry_point = rtsl_spirv_stage_entry_point(translation, translation_stage);
	if (!entry_point || strlen(entry_point) >= RTVK_MAX_SHADER_UNIFORM_NAME) {
		rtvk_throwf(RT_SHADER_LINK_FAILED, "RTSL translation returned an invalid native entry-point name");
		return false;
	}

	if (program->shader_count == program->shader_capacity) {
		u32 capacity = program->shader_capacity ? program->shader_capacity * 2 : 4;
		void* shaders = realloc(program->shaders, sizeof(program->shaders[0]) * capacity);
		if (!shaders) {
			rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for program shaders", sizeof(program->shaders[0]) * capacity);
			return false;
		}
		program->shaders = shaders;
		program->shader_capacity = capacity;
	}

	struct rtvk_program_shader* shader = &program->shaders[program->shader_count];
	*shader = (struct rtvk_program_shader){ 0 };
	shader->stage = vk_stage;
	memcpy(shader->entry_point, entry_point, strlen(entry_point) + 1);

	u64 word_count = 0;
	const u32* words = rtsl_spirv_stage_words(translation, translation_stage, &word_count);
	if (!rtvk_program_create_shader(ctx, words, word_count, &shader->vk_shader)) {
		return false;
	}
	program->shader_count++;
	return true;
}

void rtvk_program_finalize(struct rtvk_context* ctx, struct rtvk_program* program) {
	assert(ctx);
	assert(program);

	if (!program->program_source || program->program_source_size == 0) {
		rtvk_throwf(RT_SHADER_LINK_FAILED, "program finalize requires an RTSLP source set via rtProgramSource");
		return;
	}
	if (program->vk_pipeline_layout || program->shader_count) {
		rtvk_throwf(RT_IMPROPER_USAGE, "program is already finalized; reset it before finalizing again");
		return;
	}

	char translation_message[1024] = { 0 };
	rtsl_spirv_translation* translation = NULL;
	const rtsl_spirv_status status = rtsl_spirv_translate(
		program->program_source_size,
		program->program_source,
		&translation,
		translation_message,
		sizeof(translation_message)
	);
	if (status != RTSL_SPIRV_SUCCESS) {
		const enum rt_error error = status == RTSL_SPIRV_OUT_OF_MEMORY
										? RT_OUT_OF_HOST_MEMORY
									: status == RTSL_SPIRV_INVALID_PROGRAM
										? RT_SHADER_LINK_FAILED
										: RT_SHADER_COMPILATION_FAILED;
		rtvk_throwf(error, "RTSL translation failed: %s", translation_message);
		return;
	}
	bool complete = false;
	static const struct {
		rtsl_spirv_stage translation_stage;
		VkShaderStageFlagBits vk_stage;
	} translated_stages[] = {
		{ RTSL_SPIRV_VERTEX, VK_SHADER_STAGE_VERTEX_BIT },
		{ RTSL_SPIRV_FRAGMENT, VK_SHADER_STAGE_FRAGMENT_BIT },
	};
	for (u32 index = 0; index < (u32)(sizeof(translated_stages) / sizeof(translated_stages[0])); index++) {
		if (!rtvk_program_add_shader(ctx, program, translation, translated_stages[index].translation_stage, translated_stages[index].vk_stage)) {
			goto cleanup;
		}
	}

	if (!rtvk_program_build_locations(program, translation)) {
		goto cleanup;
	}
	rtvk_program_create_pipeline_layout(ctx, program);
	if (!program->vk_pipeline_layout) {
		goto cleanup;
	}
	complete = true;

cleanup:
	rtsl_spirv_translation_destroy(translation);
	if (!complete) {
		rtvk_program_destroy_pipeline_layout(ctx, program);
		rtvk_program_clear_shaders(ctx, program);
		rtvk_program_clear_locations(program);
	}
}

rt_location rtvk_program_uniform_location(struct rtvk_program* program, const char* name) {
	if (!program || !name) {
		rtvk_throwf(RT_IMPROPER_USAGE, "program and uniform name must be valid");
		return NULL;
	}
	if (!program->vk_pipeline_layout) {
		rtvk_throwf(RT_IMPROPER_USAGE, "program must be finalized before querying locations");
		return NULL;
	}
	for (u32 index = 0; index < program->location_count; index++) {
		struct rt_location_t* location = &program->locations[index];
		if (location->kind != RTVK_LOCATION_VERTEX && location->kind != RTVK_LOCATION_OUTPUT && strcmp(location->name, name) == 0) {
			return location;
		}
	}
	return NULL;
}

rt_location rtvk_program_input_location(struct rtvk_program* program, const rt_vertex_attribute* attributes, usize attribute_count) {
	if (!program || !attributes || attribute_count == 0 || !program->vk_pipeline_layout) {
		return NULL;
	}
	for (usize input_index = 0; input_index < program->vertex_layout.input_count; input_index++) {
		if (program->vertex_attribute_sources[input_index] != attributes || program->vertex_inputs[input_index].attribute_count != attribute_count) {
			continue;
		}
		for (u32 location_index = 0; location_index < program->location_count; location_index++) {
			struct rt_location_t* location = &program->locations[location_index];
			if (location->kind == RTVK_LOCATION_VERTEX && location->binding == input_index) {
				return location;
			}
		}
	}
	return NULL;
}

rt_location rtvk_program_output_location(struct rtvk_program* program, const char* name) {
	if (!program || !program->vk_pipeline_layout) {
		return NULL;
	}
	for (u32 index = 0; index < program->location_count; index++) {
		struct rt_location_t* location = &program->locations[index];
		if (location->kind == RTVK_LOCATION_OUTPUT && ((name && strcmp(location->name, name) == 0) || (!name && !location->name[0]))) {
			return location;
		}
	}
	return NULL;
}
