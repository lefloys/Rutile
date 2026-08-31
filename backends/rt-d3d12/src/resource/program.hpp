#pragma once

#include "config.hpp"
#include "resource.hpp"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <array>
#include <cstddef>
#include <optional>
#include <rtsl/program.hpp>
#include <string>
#include <vector>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

inline constexpr u32 RTD3D12_MAX_VERTEX_ATTRIBUTES = 16;
inline constexpr u32 RTD3D12_MAX_VERTEX_STREAMS = 16;
inline constexpr u32 RTD3D12_MAX_SHADER_RESOURCE_NAME = 64;

struct rt_program_t;

RTD3D12_API rt_program_t* rtProgramCreate();
RTD3D12_API void rtProgramDestroy(rt_program_t* program);
RTD3D12_API void rtProgramSetLayout(rt_program_t* program, const rt::vertex_layout* layout);
RTD3D12_API void rtProgramSource(rt_program_t* program, const char* entry_point, const u08* data, usize size);
RTD3D12_API void rtProgramSetRasterState(rt_program_t* program, rt::cull_mode cull_mode, rt::front_face front_face, rt::fill_mode fill_mode);
RTD3D12_API void rtProgramSetBlendState(rt_program_t* program, bool enabled, rt::blend_factor src_color, rt::blend_factor dst_color, rt::blend_op color_op, rt::blend_factor src_alpha, rt::blend_factor dst_alpha, rt::blend_op alpha_op);
RTD3D12_API void rtProgramFinalize(rt_program_t* program);
RTD3D12_API rt::location* rtProgramUniformLocation(rt_program_t* program, const char* name);
RTD3D12_API rt::location* rtProgramInputLocation(rt_program_t* program, const rt::vertex_attribute* attributes, usize attribute_count);
RTD3D12_API rt::location* rtProgramOutputLocation(rt_program_t* program, const char* name);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

namespace rt {
	struct location {
		u08 address;
	};
}

enum class rtd3d12_descriptor_type {
	constant_buffer,
	storage_buffer,
	texture,
};

struct rtd3d12_program_input_mapping {
	usize vertex_input;
};

struct rtd3d12_program_output_mapping {
	std::string name;
	u32 binding;
};

struct rtd3d12_program_descriptor_mapping {
	std::string name;
	rtd3d12_descriptor_type type;
	u32 binding;
	u32 storage_stride;
	u32 root_parameter;
	u32 sampler_root_parameter;
};

struct rtd3d12_program_data_mapping {
	std::string name;
	u32 binding;
	u32 root_parameter;
	usize byte_offset;
	usize byte_size;
	usize block_size;
};

struct rtd3d12_program_shader {
	rtsl::Stage stage;
	std::vector<std::byte> bytecode;
};

struct rt_program_t : rtd3d12_resource<rt_program_t> {
	explicit rt_program_t(rtd3d12_context* ctx);
	~rt_program_t();
	bool prepare(DXGI_FORMAT color_format, DXGI_FORMAT depth_format);
	void layout(const rt::vertex_layout* layout);
	void source(const char* entry_point, const void* data, usize size);
	void raster_state(rt::cull_mode cull_mode, rt::front_face front_face, rt::fill_mode fill_mode);
	void blend_state(bool enabled, rt::blend_factor src_color, rt::blend_factor dst_color, rt::blend_op color_op, rt::blend_factor src_alpha, rt::blend_factor dst_alpha, rt::blend_op alpha_op);
	void finalize();
	rt::location* uniform_location(const char* name);
	rt::location* input_location(const rt::vertex_attribute* attributes, usize attribute_count);
	rt::location* output_location(const char* name);
	rt::location* allocate_location(bool zero_address = false);
	void clear_mappings();
	void destroy_pipeline();
	void destroy_root_signature();
	ID3D12RootSignature* d3d_root_signature;
	ID3D12PipelineState* d3d_pipeline;

	rt::vertex_layout vertex_layout;
	rt::vertex_input vertex_inputs[RTD3D12_MAX_VERTEX_STREAMS];
	rt::vertex_attribute vertex_attributes[RTD3D12_MAX_VERTEX_ATTRIBUTES];
	usize vertex_attribute_inputs[RTD3D12_MAX_VERTEX_ATTRIBUTES];
	rt::cull_mode cull_mode;
	rt::front_face front_face;
	rt::fill_mode fill_mode;
	bool blend_enabled;
	rt::blend_factor src_color_blend;
	rt::blend_factor dst_color_blend;
	rt::blend_op color_blend_op;
	rt::blend_factor src_alpha_blend;
	rt::blend_factor dst_alpha_blend;
	rt::blend_op alpha_blend_op;
	DXGI_FORMAT d3d_pipeline_format;
	DXGI_FORMAT d3d_pipeline_depth_format;

	rt::location locations[256];
	std::array<std::optional<rtd3d12_program_input_mapping>, 256> input_mappings;
	std::array<std::optional<rtd3d12_program_output_mapping>, 256> output_mappings;
	std::array<std::optional<rtd3d12_program_descriptor_mapping>, 256> descriptor_mappings;
	std::array<std::optional<rtd3d12_program_data_mapping>, 256> uniform_data_mappings;
	std::array<std::optional<rtd3d12_program_data_mapping>, 256> storage_data_mappings;
	std::array<bool, 256> location_allocated;
	u32 location_count;
	std::string entry_point;
	std::optional<rtsl::Program> rtsl_program;
	std::vector<rtd3d12_program_shader> shaders;
};

rt_program_t* rtd3d12_location_program(const rt::location* location);
