#include "program.hpp"
#include "context.hpp"
#include "error.hpp"

#include <cassert>
#include <cstddef>
#include <climits>
#include <cstring>
#include <dxcapi.h>
#include <iterator>
#include <optional>
#include <rtsl/hlsl.hpp>
#include <span>
#include <string>
#include <string_view>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

rt_program_t* rtProgramCreate() {
	return rtd3d12::create_resource<rt_program_t>(rtd3d12_get_current_context());
}

void rtProgramDestroy(rt_program_t* program) {
	if (program) {
		(program)->retire();
	}
}

void rtProgramSetLayout(rt_program_t* program, const rt::vertex_layout* layout) {
	program->layout(layout);
}

void rtProgramSource(rt_program_t* program, const char* entry_point, const u08* data, usize size) {
	program->source(entry_point, data, size);
}

void rtProgramSetRasterState(rt_program_t* program, rt::cull_mode cull_mode, rt::front_face front_face, rt::fill_mode fill_mode) {
	program->raster_state(cull_mode, front_face, fill_mode);
}

void rtProgramSetBlendState(rt_program_t* program, bool enabled, rt::blend_factor src_color, rt::blend_factor dst_color, rt::blend_op color_op, rt::blend_factor src_alpha, rt::blend_factor dst_alpha, rt::blend_op alpha_op) {
	program->blend_state(enabled, src_color, dst_color, color_op, src_alpha, dst_alpha, alpha_op);
}

void rtProgramFinalize(rt_program_t* program) {
	program->finalize();
}

rt::location* rtProgramUniformLocation(rt_program_t* program, const char* name) {
	return program->uniform_location(name);
}

rt::location* rtProgramInputLocation(rt_program_t* program, const rt::vertex_attribute* attributes, usize attribute_count) {
	return program->input_location(attributes, attribute_count);
}

rt::location* rtProgramOutputLocation(rt_program_t* program, const char* name) {
	return program->output_location(name);
}

rt_program_t* rtd3d12_location_program(const rt::location* location) {
	if (!location) return nullptr;
	const rt::location* locations = location - location->address;
	return reinterpret_cast<rt_program_t*>(reinterpret_cast<std::byte*>(const_cast<rt::location*>(locations)) - offsetof(rt_program_t, locations));
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

rt_program_t::rt_program_t(rtd3d12_context* context)
	: rtd3d12_resource(context),
	  d3d_root_signature(nullptr),
	  d3d_pipeline(nullptr),
	  vertex_layout{},
	  vertex_inputs{},
	  vertex_attributes{},
	  vertex_attribute_inputs{},
	  cull_mode(rt::cull_mode::none),
	  front_face(rt::front_face::counter_clockwise),
	  fill_mode(rt::fill_mode::solid),
	  blend_enabled(false),
	  src_color_blend(rt::blend_factor::one),
	  dst_color_blend(rt::blend_factor::zero),
	  color_blend_op(rt::blend_op::add),
	  src_alpha_blend(rt::blend_factor::one),
	  dst_alpha_blend(rt::blend_factor::zero),
	  alpha_blend_op(rt::blend_op::add),
	  d3d_pipeline_format(DXGI_FORMAT_UNKNOWN),
	  d3d_pipeline_depth_format(DXGI_FORMAT_UNKNOWN),
	  locations{},
	  input_mappings{},
	  output_mappings{},
	  descriptor_mappings{},
	  uniform_data_mappings{},
	  storage_data_mappings{},
	  location_allocated{},
	  location_count(0) {}

void rt_program_t::destroy_pipeline() {
	if (d3d_pipeline) {
		d3d_pipeline->Release();
		d3d_pipeline = nullptr;
	}
	d3d_pipeline_format = DXGI_FORMAT_UNKNOWN;
	d3d_pipeline_depth_format = DXGI_FORMAT_UNKNOWN;
}

void rt_program_t::destroy_root_signature() {
	destroy_pipeline();
	if (d3d_root_signature) {
		d3d_root_signature->Release();
		d3d_root_signature = nullptr;
	}
}

rt_program_t::~rt_program_t() {
	destroy_root_signature();
	entry_point.clear();
	rtsl_program.reset();
	shaders.clear();
}

void rt_program_t::clear_mappings() {
	std::memset(locations, 0, sizeof(locations));
	input_mappings.fill(std::nullopt);
	output_mappings.fill(std::nullopt);
	descriptor_mappings.fill(std::nullopt);
	uniform_data_mappings.fill(std::nullopt);
	storage_data_mappings.fill(std::nullopt);
	location_allocated.fill(false);
	location_count = 0;
}

rt::location* rt_program_t::allocate_location(bool zero_address) {
	if (location_count == std::size(locations)) {
		rtd3d12_fail(rt::error::shader_link_failed, "program exposes more than 256 locations");
		return nullptr;
	}
	for (u32 address = zero_address ? 0u : 1u; address < std::size(locations); ++address) {
		if (location_allocated[address]) {
			continue;
		}
		locations[address].address = static_cast<u08>(address);
		location_allocated[address] = true;
		++location_count;
		return &locations[address];
	}
	rtd3d12_fail(rt::error::shader_link_failed, zero_address ? "program location zero is already mapped" : "program has no free nonzero locations");
	return nullptr;
}

static D3D12_SHADER_VISIBILITY rtd3d12_shader_visibility(rtsl::StageMask stages) {
	if (stages == rtsl::StageMask::vertex) {
		return D3D12_SHADER_VISIBILITY_VERTEX;
	}
	if (stages == rtsl::StageMask::fragment) {
		return D3D12_SHADER_VISIBILITY_PIXEL;
	}
	return D3D12_SHADER_VISIBILITY_ALL;
}

static std::optional<u32> rtd3d12_type_byte_size(const rtsl::Program& program, rtsl::ir::Id type_id) {
	const rtsl::ir::Type* type = program.find_type(type_id);
	if (!type) {
		return std::nullopt;
	}
	switch (type->kind) {
	case rtsl::ir::TypeKind::boolean:
	case rtsl::ir::TypeKind::signed_integer:
	case rtsl::ir::TypeKind::unsigned_integer:
	case rtsl::ir::TypeKind::floating:
		return type->bit_width / 8u;
	case rtsl::ir::TypeKind::vector: {
		const std::optional<u32> element = rtd3d12_type_byte_size(program, type->element_type);
		return element ? std::optional<u32>{ *element * type->element_count } : std::nullopt;
	}
	case rtsl::ir::TypeKind::matrix: {
		const std::optional<u32> column = rtd3d12_type_byte_size(program, type->element_type);
		return column ? std::optional<u32>{ *column * type->element_count } : std::nullopt;
	}
	case rtsl::ir::TypeKind::structure: {
		u32 size = 0;
		for (rtsl::ir::Id member : type->members) {
			const std::optional<u32> member_size = rtd3d12_type_byte_size(program, member);
			if (!member_size) {
				return std::nullopt;
			}
			size += *member_size;
		}
		return size;
	}
	default:
		return std::nullopt;
	}
}

static std::optional<u32> rtd3d12_storage_buffer_stride(const rtsl::Program& program, const rtsl::Resource& resource) {
	const rtsl::ir::Type* block = program.find_type(resource.value_type);
	const rtsl::ir::Type* array = block && block->kind == rtsl::ir::TypeKind::structure && block->members.size() == 1
									  ? program.find_type(block->members.front())
									  : nullptr;
	if (!array || array->kind != rtsl::ir::TypeKind::runtime_array) {
		return std::nullopt;
	}
	return rtd3d12_type_byte_size(program, array->element_type);
}

static bool rtd3d12_program_create_root_signature(rtd3d12_context* ctx, rt_program_t* program) {
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
	std::vector<D3D12_ROOT_PARAMETER> parameters;
	ranges.reserve(program->rtsl_program->resources().size() * 2);
	parameters.reserve(program->rtsl_program->resources().size() * 2);
	program->clear_mappings();

	for (const rtsl::Resource& resource : program->rtsl_program->resources()) {
		rtd3d12_program_descriptor_mapping mapping = {};
		mapping.name = resource.name;
		mapping.binding = resource.descriptor.binding;
		const D3D12_SHADER_VISIBILITY visibility = rtd3d12_shader_visibility(resource.stages);

		if (resource.kind == rtsl::ResourceKind::uniform_buffer) {
			mapping.type = rtd3d12_descriptor_type::constant_buffer;
			mapping.root_parameter = static_cast<u32>(parameters.size());
			D3D12_DESCRIPTOR_RANGE range = {};
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			range.NumDescriptors = 1;
			range.BaseShaderRegister = resource.descriptor.binding;
			range.RegisterSpace = resource.descriptor.set;
			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			ranges.push_back(range);
			D3D12_ROOT_PARAMETER parameter = {};
			parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameter.DescriptorTable.NumDescriptorRanges = 1;
			parameter.DescriptorTable.pDescriptorRanges = &ranges.back();
			parameter.ShaderVisibility = visibility;
			parameters.push_back(parameter);
		} else if (resource.kind == rtsl::ResourceKind::sampled_texture || resource.kind == rtsl::ResourceKind::sampler) {
			mapping.type = rtd3d12_descriptor_type::texture;
			mapping.root_parameter = static_cast<u32>(parameters.size());
			for (const D3D12_DESCRIPTOR_RANGE_TYPE range_type : { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER }) {
				D3D12_DESCRIPTOR_RANGE range = {};
				range.RangeType = range_type;
				range.NumDescriptors = 1;
				range.BaseShaderRegister = resource.descriptor.binding;
				range.RegisterSpace = resource.descriptor.set;
				range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				ranges.push_back(range);

				D3D12_ROOT_PARAMETER parameter = {};
				parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				parameter.DescriptorTable.NumDescriptorRanges = 1;
				parameter.DescriptorTable.pDescriptorRanges = &ranges.back();
				parameter.ShaderVisibility = visibility;
				parameters.push_back(parameter);
			}
			mapping.sampler_root_parameter = mapping.root_parameter + 1;
		} else if (resource.kind == rtsl::ResourceKind::storage_buffer) {
			const std::optional<u32> stride = rtd3d12_storage_buffer_stride(*program->rtsl_program, resource);
			if (!stride || *stride == 0) {
				rtd3d12_fail(rt::error::unsupported_feature, "DirectX 12 cannot reflect RTSL storage buffer '{}'", resource.name.c_str());
				return false;
			}
			mapping.type = rtd3d12_descriptor_type::storage_buffer;
			mapping.storage_stride = *stride;
			mapping.root_parameter = static_cast<u32>(parameters.size());
			D3D12_DESCRIPTOR_RANGE range = {};
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			range.NumDescriptors = 1;
			range.BaseShaderRegister = resource.descriptor.binding;
			range.RegisterSpace = resource.descriptor.set;
			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			ranges.push_back(range);

			D3D12_ROOT_PARAMETER parameter = {};
			parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameter.DescriptorTable.NumDescriptorRanges = 1;
			parameter.DescriptorTable.pDescriptorRanges = &ranges.back();
			parameter.ShaderVisibility = visibility;
			parameters.push_back(parameter);
		} else {
			rtd3d12_fail(rt::error::unsupported_feature, "DirectX 12 does not support RTSL resource '{}' yet", resource.name.c_str());
			return false;
		}
		rt::location* location = program->allocate_location();
		if (!location) return false;
		program->descriptor_mappings[location->address] = std::move(mapping);
	}
	for (usize index = 0; index < program->vertex_layout.input_count; index++) {
		rt::location* location = program->allocate_location();
		if (!location) return false;
		program->input_mappings[location->address] = rtd3d12_program_input_mapping{.vertex_input = index};
	}
	const rtsl::EntryPoint* fragment = program->rtsl_program->entry(rtsl::Stage::fragment);
	if (fragment && fragment->output) {
		for (const rtsl::InterfaceElement& element : fragment->output->elements) {
			if (!element.location) {
				continue;
			}
			rt::location* location = program->allocate_location(element.name.empty());
			if (!location) return false;
			program->output_mappings[location->address] = rtd3d12_program_output_mapping{
				.name = element.name,
				.binding = *element.location,
			};
		}
	}

	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = static_cast<UINT>(parameters.size());
	desc.pParameters = parameters.data();
	desc.NumStaticSamplers = 0;
	desc.pStaticSamplers = nullptr;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* signature = nullptr;
	ID3DBlob* errors = nullptr;
	HRESULT result = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errors);
	if (FAILED(result)) {
		const char* message = errors ? static_cast<const char*>(errors->GetBufferPointer()) : "root signature serialization failed";
		rtd3d12_fail(rt::error::initialization_failed, "{}", message);
		if (errors) {
			errors->Release();
			errors = nullptr;
		}
		return false;
	}

	result = ctx->d3d_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&program->d3d_root_signature));
	if (signature) {
		signature->Release();
		signature = nullptr;
	}
	if (errors) {
		errors->Release();
		errors = nullptr;
	}
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateRootSignature failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}
	return true;
}

static DXGI_FORMAT rtd3d12_vertex_format(rt::format format) {
	switch (format) {
	case rt::format::r32_sfloat:
		return DXGI_FORMAT_R32_FLOAT;
	case rt::format::rg32_sfloat:
		return DXGI_FORMAT_R32G32_FLOAT;
	case rt::format::rgb32_sfloat:
		return DXGI_FORMAT_R32G32B32_FLOAT;
	case rt::format::rgba32_sfloat:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case rt::format::r32_sint:
		return DXGI_FORMAT_R32_SINT;
	case rt::format::rg32_sint:
		return DXGI_FORMAT_R32G32_SINT;
	case rt::format::rgb32_sint:
		return DXGI_FORMAT_R32G32B32_SINT;
	case rt::format::rgba32_sint:
		return DXGI_FORMAT_R32G32B32A32_SINT;
	case rt::format::r32_uint:
		return DXGI_FORMAT_R32_UINT;
	case rt::format::rg32_uint:
		return DXGI_FORMAT_R32G32_UINT;
	case rt::format::rgb32_uint:
		return DXGI_FORMAT_R32G32B32_UINT;
	case rt::format::rgba32_uint:
		return DXGI_FORMAT_R32G32B32A32_UINT;
	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

static D3D12_BLEND rtd3d12_blend_factor(rt::blend_factor factor) {
	switch (factor) {
	case rt::blend_factor::zero:
		return D3D12_BLEND_ZERO;
	case rt::blend_factor::one:
		return D3D12_BLEND_ONE;
	case rt::blend_factor::source_color:
		return D3D12_BLEND_SRC_COLOR;
	case rt::blend_factor::one_minus_source_color:
		return D3D12_BLEND_INV_SRC_COLOR;
	case rt::blend_factor::destination_color:
		return D3D12_BLEND_DEST_COLOR;
	case rt::blend_factor::one_minus_destination_color:
		return D3D12_BLEND_INV_DEST_COLOR;
	case rt::blend_factor::source_alpha:
		return D3D12_BLEND_SRC_ALPHA;
	case rt::blend_factor::one_minus_source_alpha:
		return D3D12_BLEND_INV_SRC_ALPHA;
	case rt::blend_factor::destination_alpha:
		return D3D12_BLEND_DEST_ALPHA;
	case rt::blend_factor::one_minus_destination_alpha:
		return D3D12_BLEND_INV_DEST_ALPHA;
	default:
		return D3D12_BLEND_ZERO;
	}
}

static D3D12_BLEND_OP rtd3d12_blend_op(rt::blend_op operation) {
	switch (operation) {
	case rt::blend_op::add:
		return D3D12_BLEND_OP_ADD;
	case rt::blend_op::subtract:
		return D3D12_BLEND_OP_SUBTRACT;
	case rt::blend_op::reverse_subtract:
		return D3D12_BLEND_OP_REV_SUBTRACT;
	case rt::blend_op::minimum:
		return D3D12_BLEND_OP_MIN;
	case rt::blend_op::maximum:
		return D3D12_BLEND_OP_MAX;
	default:
		return D3D12_BLEND_OP_ADD;
	}
}

bool rt_program_t::prepare(
	DXGI_FORMAT color_format,
	DXGI_FORMAT depth_format
) {
	if (!d3d_root_signature) {
		rtd3d12_fail(rt::error::improper_usage, "program must be finalized before use");
		return false;
	}
	if (d3d_pipeline && d3d_pipeline_format == color_format && d3d_pipeline_depth_format == depth_format) {
		return true;
	}
	destroy_pipeline();

	std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
	elements.reserve(RTD3D12_MAX_VERTEX_ATTRIBUTES);
	const rtsl::EntryPoint* vertex = rtsl_program->entry(rtsl::Stage::vertex);
	if (vertex_layout.input_count && (!vertex || !vertex->input)) {
		rtd3d12_fail(rt::error::shader_link_failed, "RTSL vertex entry does not match the configured vertex layout");
		return false;
	}
	for (usize input_index = 0; input_index < vertex_layout.input_count; ++input_index) {
		const rt::vertex_input& input = vertex_layout.inputs[input_index];
		for (usize attribute_index = 0; attribute_index < input.attribute_count; ++attribute_index) {
			const rt::vertex_attribute& attribute = input.attributes[attribute_index];
			const DXGI_FORMAT format = rtd3d12_vertex_format(attribute.format);
			if (format == DXGI_FORMAT_UNKNOWN) {
				rtd3d12_fail(rt::error::unsupported_feature, "unsupported vertex attribute format");
				return false;
			}
			auto reflected = vertex->input->elements.end();
			for (auto candidate = vertex->input->elements.begin(); candidate != vertex->input->elements.end(); ++candidate) {
				if (candidate->name == attribute.name) {
					reflected = candidate;
					break;
				}
			}
			if (reflected == vertex->input->elements.end() || !reflected->location) {
				rtd3d12_fail(rt::error::shader_link_failed, "vertex attribute '{}' is not declared by the RTSL entry", attribute.name);
				return false;
			}
			elements.push_back(D3D12_INPUT_ELEMENT_DESC{
				.SemanticName = "TEXCOORD",
				.SemanticIndex = static_cast<UINT>(*reflected->location),
				.Format = format,
				.InputSlot = static_cast<UINT>(input_index),
				.AlignedByteOffset = static_cast<UINT>(attribute.offset),
				.InputSlotClass = input.rate == rt::vertex_rate::instance ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				.InstanceDataStepRate = input.rate == rt::vertex_rate::instance ? 1u : 0u,
			});
		}
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = d3d_root_signature;
	for (const rtd3d12_program_shader& shader : shaders) {
		const D3D12_SHADER_BYTECODE bytecode = { shader.bytecode.data(), shader.bytecode.size() };
		if (shader.stage == rtsl::Stage::vertex) {
			desc.VS = bytecode;
		} else if (shader.stage == rtsl::Stage::fragment) {
			desc.PS = bytecode;
		}
	}
	desc.BlendState.AlphaToCoverageEnable = FALSE;
	desc.BlendState.IndependentBlendEnable = FALSE;
	D3D12_RENDER_TARGET_BLEND_DESC& blend = desc.BlendState.RenderTarget[0];
	blend.BlendEnable = blend_enabled;
	blend.LogicOpEnable = FALSE;
	blend.SrcBlend = rtd3d12_blend_factor(src_color_blend);
	blend.DestBlend = rtd3d12_blend_factor(dst_color_blend);
	blend.BlendOp = rtd3d12_blend_op(color_blend_op);
	blend.SrcBlendAlpha = rtd3d12_blend_factor(src_alpha_blend);
	blend.DestBlendAlpha = rtd3d12_blend_factor(dst_alpha_blend);
	blend.BlendOpAlpha = rtd3d12_blend_op(alpha_blend_op);
	blend.LogicOp = D3D12_LOGIC_OP_NOOP;
	blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	desc.SampleMask = UINT_MAX;
	desc.RasterizerState.FillMode = fill_mode == rt::fill_mode::wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
	desc.RasterizerState.CullMode = cull_mode == rt::cull_mode::front ? D3D12_CULL_MODE_FRONT : cull_mode == rt::cull_mode::back ? D3D12_CULL_MODE_BACK
																																	 : D3D12_CULL_MODE_NONE;
	desc.RasterizerState.FrontCounterClockwise = front_face == rt::front_face::counter_clockwise;
	desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	desc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	desc.RasterizerState.DepthClipEnable = TRUE;
	desc.DepthStencilState.DepthEnable = depth_format != DXGI_FORMAT_UNKNOWN;
	desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	desc.DepthStencilState.StencilEnable = FALSE;
	desc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	desc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	desc.InputLayout = { elements.data(), static_cast<UINT>(elements.size()) };
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = color_format;
	desc.DSVFormat = depth_format;
	desc.SampleDesc.Count = 1;

	const HRESULT result = ctx->d3d_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&d3d_pipeline));
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateGraphicsPipelineState failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}

	d3d_pipeline_format = color_format;
	d3d_pipeline_depth_format = depth_format;
	return true;
}

void rt_program_t::layout(const rt::vertex_layout* layout) {
	if (!layout || !layout->inputs || layout->input_count == 0) {
		vertex_layout = {};
		destroy_pipeline();
		return;
	}
	if (layout->input_count > RTD3D12_MAX_VERTEX_STREAMS) {
		rtd3d12_fail(rt::error::improper_usage, "too many vertex inputs");
		return;
	}
	usize attribute_count = 0;
	for (usize input_index = 0; input_index < layout->input_count; ++input_index) {
		const rt::vertex_input& input = layout->inputs[input_index];
		if (!input.attributes || input.attribute_count == 0 || attribute_count + input.attribute_count > RTD3D12_MAX_VERTEX_ATTRIBUTES) {
			rtd3d12_fail(rt::error::improper_usage, "invalid vertex input attributes");
			return;
		}
		std::memcpy(vertex_attributes + attribute_count, input.attributes, sizeof(input.attributes[0]) * input.attribute_count);
		vertex_inputs[input_index] = input;
		vertex_inputs[input_index].attributes = vertex_attributes + attribute_count;
		attribute_count += input.attribute_count;
	}
	vertex_layout.inputs = vertex_inputs;
	vertex_layout.input_count = layout->input_count;
	destroy_pipeline();
}

void rt_program_t::source(const char* entry_point, const void* data, usize size) {
	if (!entry_point || !entry_point[0]) {
		rtd3d12_fail(rt::error::improper_usage, "program entry point is empty");
		return;
	}
	if (!data || size == 0) {
		rtd3d12_fail(rt::error::improper_usage, "program source data is empty");
		return;
	}
	auto loaded = rtsl::load_program(std::span<const std::byte>{ static_cast<const std::byte*>(data), size });
	if (!loaded) {
		rtd3d12_fail(rt::error::shader_link_failed, "invalid RTSLP source: {}", loaded.error().message.c_str());
		return;
	}
	destroy_root_signature();
	this->entry_point = entry_point;
	clear_mappings();
	rtsl_program.emplace(std::move(*loaded));
	shaders.clear();
}

void rt_program_t::raster_state(
	rt::cull_mode cull_mode,
	rt::front_face front_face,
	rt::fill_mode fill_mode
) {
	this->cull_mode = cull_mode;
	this->front_face = front_face;
	this->fill_mode = fill_mode;
	destroy_pipeline();
}

void rt_program_t::blend_state(
	bool enabled,
	rt::blend_factor src_color,
	rt::blend_factor dst_color,
	rt::blend_op color_op,
	rt::blend_factor src_alpha,
	rt::blend_factor dst_alpha,
	rt::blend_op alpha_op
) {
	blend_enabled = enabled;
	src_color_blend = src_color;
	dst_color_blend = dst_color;
	color_blend_op = color_op;
	src_alpha_blend = src_alpha;
	dst_alpha_blend = dst_alpha;
	alpha_blend_op = alpha_op;
	destroy_pipeline();
}

namespace rtd3d12_program_detail {

bool compile_hlsl(std::string_view source, std::string_view entry_point, const wchar_t* profile, std::vector<std::byte>& bytecode) {
	IDxcCompiler3* compiler = nullptr;
	IDxcResult* result = nullptr;
	IDxcBlobUtf8* errors = nullptr;
	IDxcBlob* object = nullptr;

	HRESULT status = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
	if (SUCCEEDED(status)) {
		const DxcBuffer buffer = {
			.Ptr = source.data(),
			.Size = source.size(),
			.Encoding = DXC_CP_UTF8,
		};
		const std::wstring wide_entry_point(entry_point.begin(), entry_point.end());
		const wchar_t* arguments[] = { L"-E", wide_entry_point.c_str(), L"-T", profile, L"-HV", L"2021", L"-Ges" };
		status = compiler->Compile(&buffer, arguments, static_cast<UINT32>(std::size(arguments)), nullptr, IID_PPV_ARGS(&result));
	}
	HRESULT compile_status = status;
	if (SUCCEEDED(status)) {
		result->GetStatus(&compile_status);
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
	}
	if (FAILED(status) || FAILED(compile_status)) {
		const char* message = errors && errors->GetStringLength() ? errors->GetStringPointer() : "DXC failed without diagnostics";
		rtd3d12_fail(rt::error::shader_link_failed, "{}", message);
		if (object) {
			object->Release();
			object = nullptr;
		}
		if (errors) {
			errors->Release();
			errors = nullptr;
		}
		if (result) {
			result->Release();
			result = nullptr;
		}
		if (compiler) {
			compiler->Release();
			compiler = nullptr;
		}
		return false;
	}
	status = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&object), nullptr);
	if (FAILED(status) || !object) {
		rtd3d12_fail(rt::error::shader_link_failed, "DXC did not produce shader bytecode");
		if (errors) {
			errors->Release();
			errors = nullptr;
		}
		if (result) {
			result->Release();
			result = nullptr;
		}
		if (compiler) {
			compiler->Release();
			compiler = nullptr;
		}
		return false;
	}
	const auto* begin = static_cast<const std::byte*>(object->GetBufferPointer());
	bytecode.assign(begin, begin + object->GetBufferSize());
	if (object) {
		object->Release();
		object = nullptr;
	}
	if (errors) {
		errors->Release();
		errors = nullptr;
	}
	if (result) {
		result->Release();
		result = nullptr;
	}
	if (compiler) {
		compiler->Release();
		compiler = nullptr;
	}
	return true;
}

}

void rt_program_t::finalize() {
	if (!rtsl_program) {
		rtd3d12_fail(rt::error::shader_link_failed, "program finalize requires an RTSLP source set via rtProgramSource");
		return;
	}
	static constexpr struct {
		rtsl::Stage stage;
		const wchar_t* profile;
	} stage_configs[] = {
		{ rtsl::Stage::vertex, L"vs_6_0" },
		{ rtsl::Stage::fragment, L"ps_6_0" },
	};

	std::vector<rtd3d12_program_shader> shaders;
	for (const auto& config : stage_configs) {
		const rtsl::EntryPoint* entry = rtsl_program->entry(config.stage);
		if (!entry || entry->name != entry_point) {
			continue;
		}
		auto translation = rtsl::hlsl::transpile(*rtsl_program, config.stage);
		if (!translation) {
			const rtsl::hlsl::Error& error = translation.error();
			rtd3d12_fail(rt::error::shader_link_failed, "RTSL to HLSL failed in {}: {}", error.context.c_str(), error.message.c_str());
			return;
		}
		rtd3d12_program_shader shader = { .stage = config.stage };
		if (!rtd3d12_program_detail::compile_hlsl(translation->source, translation->entry_point, config.profile, shader.bytecode)) {
			return;
		}
		shaders.push_back(std::move(shader));
	}
	if (shaders.empty()) {
		rtd3d12_fail(rt::error::shader_link_failed, "RTSL program does not expose entry point '{}' in a D3D12-supported stage", entry_point.c_str());
		return;
	}

	destroy_root_signature();
	this->shaders = std::move(shaders);
	if (!rtd3d12_program_create_root_signature(ctx, this)) {
		this->shaders.clear();
	}
}

rt::location* rt_program_t::uniform_location(const char* name) {
	if (!name) {
		rtd3d12_fail(rt::error::improper_usage, "uniform location name is nullptr");
		return nullptr;
	}
	if (!d3d_root_signature) {
		rtd3d12_fail(rt::error::improper_usage, "program must be finalized before querying uniforms");
		return nullptr;
	}
	for (u32 index = 0; index < std::size(locations); ++index) {
		if (!location_allocated[index]) continue;
		if ((uniform_data_mappings[index] && uniform_data_mappings[index]->name == name) ||
			(storage_data_mappings[index] && storage_data_mappings[index]->name == name) ||
			(descriptor_mappings[index] && descriptor_mappings[index]->name == name)) return &locations[index];
	}
	return nullptr;
}

rt::location* rt_program_t::input_location(const rt::vertex_attribute* attributes, usize attribute_count) {
	if (!attributes || !attribute_count) {
		rtd3d12_fail(rt::error::improper_usage, "vertex input attributes are invalid");
		return nullptr;
	}
	if (!d3d_root_signature) {
		rtd3d12_fail(rt::error::improper_usage, "program must be finalized before querying vertex inputs");
		return nullptr;
	}
	for (u32 index = 0; index < std::size(locations); ++index) {
		if (!location_allocated[index] || !input_mappings[index] || input_mappings[index]->vertex_input >= vertex_layout.input_count) continue;
		const rt::vertex_input& input = vertex_layout.inputs[input_mappings[index]->vertex_input];
		if (input.attribute_count != attribute_count) {
			continue;
		}
		bool matches = true;
		for (usize index = 0; index < attribute_count; ++index) {
			const rt::vertex_attribute& expected = input.attributes[index];
			const rt::vertex_attribute& supplied = attributes[index];
			if (expected.offset != supplied.offset || expected.format != supplied.format || std::strcmp(expected.name, supplied.name) != 0) {
				matches = false;
				break;
			}
		}
		if (matches) {
			return &locations[index];
		}
	}
	return nullptr;
}

rt::location* rt_program_t::output_location(const char* name) {
	if (!d3d_root_signature) {
		rtd3d12_fail(rt::error::improper_usage, "program must be finalized before querying fragment outputs");
		return nullptr;
	}
	for (u32 index = 0; index < std::size(locations); ++index) {
		if (!location_allocated[index] || !output_mappings[index]) continue;
		const rtd3d12_program_output_mapping& mapping = *output_mappings[index];
		if ((name && mapping.name == name) || (!name && mapping.name.empty())) return &locations[index];
	}
	return nullptr;
}
