#pragma once

#include "config.hpp"
#include "resource.hpp"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>
#include <rtsl/program.hpp>
#include <vector>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

inline constexpr u32 RTDX_MAX_VERTEX_ATTRIBUTES = 16;
inline constexpr u32 RTDX_MAX_VERTEX_STREAMS = 16;
inline constexpr u32 RTDX_MAX_SHADER_RESOURCE_NAME = 64;

struct rtdx_graphics_program;

RTDX_API rt_graphics_program rtGraphicsProgramCreate();
RTDX_API void rtGraphicsProgramDestroy(rt_graphics_program program);
RTDX_API void rtGraphicsProgramSetLayout(rt_graphics_program program, const rt_vertex_layout* layout);
RTDX_API void rtGraphicsProgramSetSource(rt_graphics_program program, const u08* data, usize size);
RTDX_API void rtGraphicsProgramSetRasterState(rt_graphics_program program, rt_cull_mode cull_mode, rt_front_face front_face, rt_fill_mode fill_mode);
RTDX_API void rtGraphicsProgramSetBlendState(rt_graphics_program program, bool enabled, rt_blend_factor src_color, rt_blend_factor dst_color, rt_blend_op color_op, rt_blend_factor src_alpha, rt_blend_factor dst_alpha, rt_blend_op alpha_op);
RTDX_API void rtGraphicsProgramFinalize(rt_graphics_program program);
RTDX_API rt_location rtGraphicsProgramUniformLocation(rt_graphics_program program, const char* name);
RTDX_API rt_location rtGraphicsProgramInputLocation(rt_graphics_program program, const rt_vertex_attribute* attributes, usize attribute_count);
RTDX_API rt_location rtGraphicsProgramOutputLocation(rt_graphics_program program, const char* name);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

enum class rtdx_location_kind {
	buffer,
	storage_buffer,
	texture,
	vertex_input,
};

struct rtdx_location {
	rtdx_graphics_program* program;
	char name[RTDX_MAX_SHADER_RESOURCE_NAME];
	rtdx_location_kind kind;
	u32 slot;
	u32 binding;
	u32 storage_stride;
	u32 root_parameter;
	u32 sampler_root_parameter;
	usize vertex_input;
};

inline rtdx_location* rtdx_location_from_handle(rt_location location) { return reinterpret_cast<rtdx_location*>(location); }
inline rt_location rtdx_location_to_handle(rtdx_location* location) { return reinterpret_cast<rt_location>(location); }

struct rtdx_graphics_program {
	rtdx_resource_base base;
	ID3D12RootSignature* d3d_root_signature;
	ID3D12PipelineState* d3d_pipeline;

	rt_vertex_layout vertex_layout;
	rt_vertex_input vertex_inputs[RTDX_MAX_VERTEX_STREAMS];
	rt_vertex_attribute vertex_attributes[RTDX_MAX_VERTEX_ATTRIBUTES];
	usize vertex_attribute_inputs[RTDX_MAX_VERTEX_ATTRIBUTES];
	rt_cull_mode cull_mode;
	rt_front_face front_face;
	rt_fill_mode fill_mode;
	bool blend_enabled;
	rt_blend_factor src_color_blend;
	rt_blend_factor dst_color_blend;
	rt_blend_op color_blend_op;
	rt_blend_factor src_alpha_blend;
	rt_blend_factor dst_alpha_blend;
	rt_blend_op alpha_blend_op;
	DXGI_FORMAT d3d_pipeline_format;
	DXGI_FORMAT d3d_pipeline_depth_format;

	std::vector<rtdx_location> locations;
	std::vector<unsigned char> program_source;
	std::unique_ptr<rtsl::Program> rtsl_program;
	std::vector<unsigned char> vertex_dxil;
	std::vector<unsigned char> fragment_dxil;
};
RTDX_DECLARE_NEW_RESOURCE(graphics_program)

bool rtdx_graphics_program_prepare(rtdx_context* ctx, rtdx_graphics_program* program, DXGI_FORMAT color_format, DXGI_FORMAT depth_format);
void rtdx_graphics_program_layout(rtdx_context* ctx, rtdx_graphics_program* program, const rt_vertex_layout* layout);
void rtdx_graphics_program_source(rtdx_context* ctx, rtdx_graphics_program* program, const void* data, usize size);
void rtdx_graphics_program_raster_state(rtdx_context* ctx, rtdx_graphics_program* program, rt_cull_mode cull_mode, rt_front_face front_face, rt_fill_mode fill_mode);
void rtdx_graphics_program_blend_state(rtdx_context* ctx, rtdx_graphics_program* program, bool enabled, rt_blend_factor src_color, rt_blend_factor dst_color, rt_blend_op color_op, rt_blend_factor src_alpha, rt_blend_factor dst_alpha, rt_blend_op alpha_op);
void rtdx_graphics_program_finalize(rtdx_context* ctx, rtdx_graphics_program* program);
rt_location rtdx_graphics_program_uniform_location(rtdx_context* ctx, rtdx_graphics_program* program, const char* name);
rt_location rtdx_graphics_program_input_location(rtdx_context* ctx, rtdx_graphics_program* program, const rt_vertex_attribute* attributes, usize attribute_count);
rt_location rtdx_graphics_program_output_location(rtdx_context* ctx, rtdx_graphics_program* program, const char* name);
