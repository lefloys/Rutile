#include "hlsl.hpp"

#include <rtsl/IR/Verifier.hpp>

#include <algorithm>
#include <bit>
#include <charconv>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_map>

namespace rtd3d12::hlsl {
namespace {

std::string hlslIdentifier(std::string_view spelling);

std::string typeName(const rtsl::ir::Module& module, rtsl::ir::TypeId id, Error& error) {
	const rtsl::ir::Type* type = module.findType(id);
	if (!type) { error = {"type", "references an unknown RTIR type"}; return {}; }
	switch (type->kind) {
	case rtsl::ir::TypeKind::type_void: return "void";
	case rtsl::ir::TypeKind::type_boolean: return "bool";
	case rtsl::ir::TypeKind::type_signed_integer: return "int";
	case rtsl::ir::TypeKind::type_unsigned_integer: return "uint";
	case rtsl::ir::TypeKind::type_floating: return "float";
	case rtsl::ir::TypeKind::type_vector: {
		std::string element = typeName(module, type->element_type, error);
		return element.empty() ? std::string{} : element + std::to_string(type->element_count);
	}
	case rtsl::ir::TypeKind::type_matrix: {
		const rtsl::ir::Type* column = module.findType(type->element_type);
		if (!column || column->kind != rtsl::ir::TypeKind::type_vector) { error = {"type", "matrix column is not a vector"}; return {}; }
		std::string element = typeName(module, column->element_type, error);
		return element.empty() ? std::string{} : element + std::to_string(column->element_count) + "x" + std::to_string(type->element_count);
	}
	case rtsl::ir::TypeKind::type_structure: {
		std::string name = hlslIdentifier(module.strings.get(type->name));
		if (name.empty()) { error = {"type", "anonymous structure cannot be emitted to HLSL"}; return {}; }
		return name;
	}
	case rtsl::ir::TypeKind::type_patch: return "rtsl_patch";
	default: error = {"type", "RTIR type is not supported by the D3D12 HLSL translator"}; return {};
	}
}

std::string hlslIdentifier(std::string_view spelling) {
	std::string result;
	result.reserve(spelling.size() + 1);
	for (const char character : spelling) {
		const unsigned char value = static_cast<unsigned char>(character);
		result.push_back(std::isalnum(value) || character == '_' ? character : '_');
	}
	if (result.empty() || std::isdigit(static_cast<unsigned char>(result.front()))) result.insert(result.begin(), '_');
	return result;
}

std::string symbolName(const rtsl::ir::Module& module, rtsl::ir::SymbolId symbol) {
	const auto* value = module.findSymbol(symbol);
	return value ? hlslIdentifier(module.strings.get(value->fully_qualified_name)) : std::string{};
}

std::string valueName(rtsl::ir::ValueId id) { return "v" + std::to_string(id.value()); }

class Emitter {
public:
	Emitter(const rtsl::ir::Module& module, Error& error) : module(module), error(error) {}

	bool emit(std::ostringstream& out, const rtsl::ir::EntryPoint& entry) {
		for (const auto& type : module.types) if (type.kind == rtsl::ir::TypeKind::type_structure && !emitStruct(out, type)) return false;
		std::uint32_t next_uniform_binding = 0;
		for (const auto& resource : module.resources) if (resource.binding && resource.binding->set == 0) next_uniform_binding = (std::max)(next_uniform_binding, resource.binding->binding + 1);
		for (const auto& resource : module.resources) {
			const std::string name = symbolName(module, resource.symbol);
			if (name.empty()) return fail("resource", "resource has no symbol name");
			const auto binding = resource.binding.value_or(rtsl::ir::Binding{});
			switch (resource.kind) {
			case rtsl::ir::ResourceKind::resource_uniform_buffer: out << "ConstantBuffer<float4> " << name << " : register(b" << binding.binding << ", space" << binding.set << ");\n"; break;
			case rtsl::ir::ResourceKind::resource_storage_buffer: out << "StructuredBuffer<float4> " << name << " : register(t" << binding.binding << ", space" << binding.set << ");\n"; break;
			case rtsl::ir::ResourceKind::resource_sampled_texture: out << "Texture2D<float4> " << name << " : register(t" << binding.binding << ", space" << binding.set << ");\n"; break;
			case rtsl::ir::ResourceKind::resource_sampler: out << "SamplerState " << name << " : register(s" << binding.binding << ", space" << binding.set << ");\n"; break;
			default: return fail("resource", "resource kind is not supported by the D3D12 HLSL translator");
			}
		}
		for (const auto& uniform : module.uniforms) {
			const std::string name = symbolName(module, uniform.symbol);
			const std::string type = typeName(module, uniform.type, error);
			if (name.empty() || type.empty()) return fail("uniform", "uniform has no emittable name or type");
			const auto binding = uniform.binding.value_or(rtsl::ir::Binding{.set = 0, .binding = next_uniform_binding++});
			out << "cbuffer cb_" << name << " : register(b" << binding.binding << ", space" << binding.set << ") { " << type << " " << name << "; };\n";
		}
		for (const auto& function : module.functions) {
			const bool is_entry = std::ranges::any_of(module.entry_points,
				[&](const rtsl::ir::EntryPoint& candidate) { return candidate.function == function.id; });
			if (!function.declaration && !is_entry && !emitFunction(out, function)) return false;
		}
		const auto* function = module.findFunction(entry.function);
		if (!function) return fail("entry", "entry point references an unknown function");
		out << "\n";
		if (entry.stage == rtsl::ir::Stage::stage_tessellation_control)
			return emitTessellationControl(out, entry, *function);
		if (entry.stage == rtsl::ir::Stage::stage_tessellation_evaluation)
			return emitTessellationEvaluation(out, entry, *function);
		if (entry.stage == rtsl::ir::Stage::stage_geometry)
			return emitGeometry(out, entry, *function);
		if (entry.stage != rtsl::ir::Stage::stage_vertex && entry.stage != rtsl::ir::Stage::stage_fragment)
			return fail("entry", "D3D12 backend does not support this entry stage");
		const std::string entry_return_type = typeName(module, function->return_type, error);
		if (entry_return_type.empty()) return false;
		const rtsl::ir::Type* return_type = module.findType(function->return_type);
		out << entry_return_type << " main(";
		for (std::size_t i = 0; i < function->parameters.size(); ++i) {
			if (i) out << ", ";
			const std::string parameter_type = typeName(module, function->parameters[i].type, error);
			if (parameter_type.empty()) return false;
			out << parameter_type << " " << valueName(function->parameters[i].value);
			const rtsl::ir::Type* type = module.findType(function->parameters[i].type);
			if (!type || type->kind != rtsl::ir::TypeKind::type_structure) out << " : TEXCOORD" << i;
		}
		out << ")";
		if (!return_type || return_type->kind != rtsl::ir::TypeKind::type_structure) {
			if (entry.stage == rtsl::ir::Stage::stage_vertex) out << " : SV_Position";
			else out << " : SV_Target0";
		}
		out << " { return " << symbolName(module, function->symbol) << "(";
		for (std::size_t i = 0; i < function->parameters.size(); ++i) { if (i) out << ", "; out << valueName(function->parameters[i].value); }
		out << "); }\n";
		return !error.message.size();
	}

private:
	bool fail(std::string context, std::string message) { error = {std::move(context), std::move(message)}; return false; }
	const rtsl::ir::Type* patchElement(const rtsl::ir::Function& function, std::size_t parameter = 0) {
		if (function.parameters.size() <= parameter) { fail("entry", "entry point has no patch parameter"); return nullptr; }
		const auto* patch = module.findType(function.parameters[parameter].type);
		if (patch && patch->kind == rtsl::ir::TypeKind::type_pointer) patch = module.findType(patch->element_type);
		if (!patch || patch->kind != rtsl::ir::TypeKind::type_patch) { fail("entry", "entry parameter is not an RTIR patch type"); return nullptr; }
		const auto* element = module.findType(patch->element_type);
		if (!element) fail("entry", "patch element type is unknown");
		return element;
	}
	std::optional<std::uint32_t> controlPointCount(const rtsl::ir::EntryPoint& entry) {
		for (const auto& attribute : entry.attributes) {
			if (module.strings.get(attribute.name) != "invocations" || attribute.tokens.size() != 1) continue;
			const std::string_view token = module.strings.get(attribute.tokens.front());
			std::uint32_t value{};
			const auto [end, result] = std::from_chars(token.data(), token.data() + token.size(), value);
			if (result == std::errc{} && end == token.data() + token.size() && value != 0) return value;
			fail("entry", "@invocations must contain one non-zero unsigned numeric token");
			return std::nullopt;
		}
		fail("entry", "tessellation-control entry requires @invocations : N");
		return std::nullopt;
	}
	const rtsl::ir::TessellationEvaluationConfiguration* tessellationEvaluation(const rtsl::ir::EntryPoint& entry) {
		for (const auto& candidate : module.entry_points) {
			if (candidate.stage != rtsl::ir::Stage::stage_tessellation_evaluation || module.strings.get(candidate.source_name) != module.strings.get(entry.source_name)) continue;
			return std::get_if<rtsl::ir::TessellationEvaluationConfiguration>(&candidate.configuration);
		}
		fail("entry", "tessellation-control entry has no matching tessellation-evaluation entry");
		return nullptr;
	}
	std::optional<std::uint32_t> matchingControlPointCount(const rtsl::ir::EntryPoint& entry) {
		for (const auto& candidate : module.entry_points)
			if (candidate.stage == rtsl::ir::Stage::stage_tessellation_control && module.strings.get(candidate.source_name) == module.strings.get(entry.source_name))
				return controlPointCount(candidate);
		fail("entry", "tessellation-evaluation entry has no matching tessellation-control entry");
		return std::nullopt;
	}
	static std::string tessellationDomain(const rtsl::ir::TessellationEvaluationConfiguration& configuration) {
		switch (configuration.domain) {
		case rtsl::ir::TessellationDomain::tessellation_domain_triangles: return "tri";
		case rtsl::ir::TessellationDomain::tessellation_domain_quads: return "quad";
		case rtsl::ir::TessellationDomain::tessellation_domain_isolines: return "isoline";
		}
		return {};
	}
	static std::string tessellationPartitioning(const rtsl::ir::TessellationEvaluationConfiguration& configuration) {
		switch (configuration.spacing) {
		case rtsl::ir::TessellationSpacing::tessellation_spacing_equal: return "integer";
		case rtsl::ir::TessellationSpacing::tessellation_spacing_fractional_even: return "fractional_even";
		case rtsl::ir::TessellationSpacing::tessellation_spacing_fractional_odd: return "fractional_odd";
		}
		return {};
	}
	static std::string tessellationOutputTopology(const rtsl::ir::TessellationEvaluationConfiguration& configuration) {
		if (configuration.domain == rtsl::ir::TessellationDomain::tessellation_domain_isolines) return "line";
		return configuration.winding == rtsl::ir::Winding::winding_clockwise ? "triangle_cw" : "triangle_ccw";
	}
	bool emitTessellationFactorType(std::ostringstream& out, const rtsl::ir::TessellationEvaluationConfiguration& configuration) {
		out << "struct rtsl_tessellation_factors {\n";
		switch (configuration.domain) {
		case rtsl::ir::TessellationDomain::tessellation_domain_triangles: out << "  float outer[3] : SV_TessFactor;\n  float inner[1] : SV_InsideTessFactor;\n"; break;
		case rtsl::ir::TessellationDomain::tessellation_domain_quads: out << "  float outer[4] : SV_TessFactor;\n  float inner[2] : SV_InsideTessFactor;\n"; break;
		case rtsl::ir::TessellationDomain::tessellation_domain_isolines: out << "  float outer[2] : SV_TessFactor;\n"; break;
		default: return fail("entry", "unknown tessellation domain");
		}
		out << "};\n";
		return true;
	}
	bool emitTessellationControl(std::ostringstream& out, const rtsl::ir::EntryPoint& entry, const rtsl::ir::Function& function) {
		const auto count = controlPointCount(entry); if (!count) return false;
		const auto* configuration = tessellationEvaluation(entry); if (!configuration) return false;
		const auto* element = patchElement(function); if (!element) return false;
		const std::string element_name = typeName(module, element->id, error); if (element_name.empty()) return false;
		if (!emitTessellationFactorType(out, *configuration)) return false;
		const std::string input = valueName(function.parameters[0].value);
		const std::string patch_function = "rtsl_patch_constants_" + std::to_string(entry.function.value());
		out << "rtsl_tessellation_factors " << patch_function << "(InputPatch<" << element_name << ", " << *count << "> " << input << ") {\n";
		entry_patch_parameter = function.parameters[0].value; entry_patch_name = input; entry_current_index = "0"; entry_factor_name = "result"; suppress_outer_stores = false; suppress_returns = true;
		out << "  rtsl_tessellation_factors result = (rtsl_tessellation_factors)0;\n";
		if (!emitEntryFunctionBody(out, function, 1)) return false;
		out << "  return result;\n}\n";
		out << "[domain(\"" << tessellationDomain(*configuration) << "\")]\n[partitioning(\"" << tessellationPartitioning(*configuration) << "\")]\n[outputtopology(\"" << tessellationOutputTopology(*configuration) << "\")]\n[outputcontrolpoints(" << *count << ")]\n[patchconstantfunc(\"" << patch_function << "\")]\n";
		out << element_name << " main(InputPatch<" << element_name << ", " << *count << "> " << input << ", uint rtsl_control_point : SV_OutputControlPointID) {\n";
		entry_patch_parameter = function.parameters[0].value; entry_patch_name = input; entry_current_index = "rtsl_control_point"; entry_factor_name = "rtsl_ignored_outer"; suppress_outer_stores = true; suppress_returns = false;
		if (!emitEntryFunctionBody(out, function, 1)) return false;
		out << "}\n";
		clearEntryLowering();
		return true;
	}
	bool emitTessellationEvaluation(std::ostringstream& out, const rtsl::ir::EntryPoint& entry, const rtsl::ir::Function& function) {
		const auto count = matchingControlPointCount(entry); if (!count) return false;
		const auto* configuration = std::get_if<rtsl::ir::TessellationEvaluationConfiguration>(&entry.configuration);
		if (!configuration) return fail("entry", "tessellation-evaluation configuration is missing");
		const auto* element = patchElement(function); if (!element) return false;
		const std::string element_name = typeName(module, element->id, error); if (element_name.empty()) return false;
		if (!emitTessellationFactorType(out, *configuration)) return false;
		const std::string input = valueName(function.parameters[0].value);
		out << "[domain(\"" << tessellationDomain(*configuration) << "\")]\n" << element_name << " main(rtsl_tessellation_factors rtsl_factors, float2 rtsl_domain_location : SV_DomainLocation, OutputPatch<" << element_name << ", " << *count << "> " << input << ") {\n";
		entry_patch_parameter = function.parameters[0].value; entry_patch_name = input; entry_coordinate_name = configuration->domain == rtsl::ir::TessellationDomain::tessellation_domain_isolines ? "rtsl_domain_location.x" : "rtsl_domain_location";
		suppress_returns = false;
		if (!emitEntryFunctionBody(out, function, 1)) return false;
		out << "}\n";
		clearEntryLowering();
		return true;
	}
	bool emitGeometry(std::ostringstream& out, const rtsl::ir::EntryPoint& entry, const rtsl::ir::Function& function) {
		const auto* configuration = std::get_if<rtsl::ir::GeometryConfiguration>(&entry.configuration);
		if (!configuration || configuration->input != rtsl::ir::PrimitiveTopology::primitive_triangles || configuration->output != rtsl::ir::PrimitiveTopology::primitive_triangle_strip || configuration->maximum_vertices == 0)
			return fail("entry", "D3D12 geometry lowering requires triangle input, triangle-strip output, and a non-zero maximum_vertices RTIR configuration");
		if (function.parameters.size() != 1) return fail("entry", "geometry entry must have one primitive parameter");
		const auto* primitive = module.findType(function.parameters[0].type);
		if (!primitive || primitive->kind != rtsl::ir::TypeKind::type_primitive) return fail("entry", "geometry entry parameter is not an RTIR primitive type");
		const std::string element = typeName(module, primitive->element_type, error); if (element.empty()) return false;
		const std::string input = valueName(function.parameters[0].value);
		out << "[maxvertexcount(" << configuration->maximum_vertices << ")]\nvoid main(triangle " << element << " " << input << "[3], inout TriangleStream<" << element << "> rtsl_stream) {\n";
		entry_geometry_stream = "rtsl_stream"; suppress_returns = true;
		if (!emitEntryFunctionBody(out, function, 1)) return false;
		out << "}\n";
		clearEntryLowering();
		return true;
	}
	bool emitEntryFunctionBody(std::ostringstream& out, const rtsl::ir::Function& function, int indent) {
		std::unordered_map<std::uint32_t, rtsl::ir::TypeId> value_types;
		for (const auto& parameter : function.parameters) value_types.emplace(parameter.value.value(), parameter.type);
		for (const auto& block : function.blocks) for (const auto& argument : block.arguments) value_types.emplace(argument.value.value(), argument.type);
		for (const auto& block : function.blocks) for (const auto& instruction : block.instructions) if (instruction.result) value_types[instruction.result.value()] = instruction.type;
		std::unordered_map<std::uint32_t, const rtsl::ir::Block*> blocks;
		for (const auto& block : function.blocks) blocks.emplace(block.id.value(), &block);
		return !function.blocks.empty() && emitBlock(out, function.blocks.front(), blocks, value_types, indent);
	}
	void clearEntryLowering() { entry_patch_parameter = {}; entry_patch_name.clear(); entry_current_index.clear(); entry_coordinate_name.clear(); entry_factor_name.clear(); entry_geometry_stream.clear(); outer_accesses.clear(); suppress_outer_stores = false; suppress_returns = false; }
	bool emitStruct(std::ostringstream& out, const rtsl::ir::Type& type) {
		const std::string name(module.strings.get(type.name)); if (name.empty()) return fail("type", "anonymous structure cannot be emitted to HLSL");
		out << "struct " << name << " {\n";
		std::uint32_t texcoord = 0;
		for (const auto& member : type.members) {
			const std::string member_type = typeName(module, member.type, error);
			const std::string member_name = hlslIdentifier(module.strings.get(member.name));
			if (member_type.empty()) return false;
			out << "  " << member_type << " " << member_name;
			if (member_name == "position" && member_type == "float4") out << " : SV_Position";
			else out << " : TEXCOORD" << texcoord++;
			out << ";\n";
		}
		out << "};\n"; return true;
	}
	bool emitFunction(std::ostringstream& out, const rtsl::ir::Function& function) {
		const std::string name = symbolName(module, function.symbol); if (name.empty()) return fail("function", "function has no symbol name");
		const std::string result_type = typeName(module, function.return_type, error); if (result_type.empty()) return false;
		out << result_type << " " << name << "(";
		for (std::size_t i = 0; i < function.parameters.size(); ++i) { if (i) out << ", "; const std::string type = typeName(module, function.parameters[i].type, error); if (type.empty()) return false; out << type << " " << valueName(function.parameters[i].value); }
		out << ") {\n";
		std::unordered_map<std::uint32_t, rtsl::ir::TypeId> value_types;
		for (const auto& parameter : function.parameters) value_types.emplace(parameter.value.value(), parameter.type);
		for (const auto& block : function.blocks) for (const auto& argument : block.arguments) value_types.emplace(argument.value.value(), argument.type);
		for (const auto& block : function.blocks) for (const auto& instruction : block.instructions) if (instruction.result) value_types[instruction.result.value()] = instruction.type;
		std::unordered_map<std::uint32_t, const rtsl::ir::Block*> blocks;
		for (const auto& block : function.blocks) blocks.emplace(block.id.value(), &block);
		if (function.blocks.empty() || !emitBlock(out, function.blocks.front(), blocks, value_types, 1)) return false;
		out << "}\n"; return true;
	}
	bool emitBlock(std::ostringstream& out, const rtsl::ir::Block& block, const std::unordered_map<std::uint32_t, const rtsl::ir::Block*>& blocks, const std::unordered_map<std::uint32_t, rtsl::ir::TypeId>& value_types, int indent) {
		const std::string pad(static_cast<std::size_t>(indent) * 2, ' ');
		for (const auto& instruction : block.instructions) if (!emitInstruction(out, instruction, value_types, pad)) return false;
		if (!block.terminator) return fail("function", "function has no terminator");
		const auto& term = *block.terminator;
		if (term.kind == rtsl::ir::TerminatorKind::terminator_return) { if (!suppress_returns) out << pad << "return;\n"; return true; }
		if (term.kind == rtsl::ir::TerminatorKind::terminator_return_value && term.operands.size() == 1) { if (!suppress_returns) out << pad << "return " << valueName(term.operands[0]) << ";\n"; return true; }
		if (term.kind != rtsl::ir::TerminatorKind::terminator_conditional_branch || term.operands.size() != 1 || term.successors.size() != 2 || block.merge.kind != rtsl::ir::MergeKind::merge_selection) return fail("terminator", "RTIR control flow is not supported by the D3D12 HLSL translator");
		auto then_it = blocks.find(term.successors[0].block.value()), else_it = blocks.find(term.successors[1].block.value()), merge_it = blocks.find(block.merge.merge_block.value());
		if (then_it == blocks.end() || else_it == blocks.end() || merge_it == blocks.end()) return fail("terminator", "selection references an unknown block");
		const auto& merge = *merge_it->second;
		for (const auto& argument : merge.arguments) { const std::string type = typeName(module, argument.type, error); if (type.empty()) return false; out << pad << type << " " << valueName(argument.value) << ";\n"; }
		out << pad << "if (" << valueName(term.operands[0]) << ") {\n";
		if (!emitSelectionArm(out, *then_it->second, merge, blocks, value_types, indent + 1)) return false;
		out << pad << "} else {\n";
		if (!emitSelectionArm(out, *else_it->second, merge, blocks, value_types, indent + 1)) return false;
		out << pad << "}\n";
		return emitBlock(out, merge, blocks, value_types, indent);
	}
	bool emitSelectionArm(std::ostringstream& out, const rtsl::ir::Block& arm, const rtsl::ir::Block& merge, const std::unordered_map<std::uint32_t, const rtsl::ir::Block*>& blocks, const std::unordered_map<std::uint32_t, rtsl::ir::TypeId>& value_types, int indent) {
		const std::string pad(static_cast<std::size_t>(indent) * 2, ' ');
		for (const auto& instruction : arm.instructions) if (!emitInstruction(out, instruction, value_types, pad)) return false;
		if (!arm.terminator) return fail("terminator", "selection arm has no terminator");
		const auto& term = *arm.terminator;
		if (term.kind == rtsl::ir::TerminatorKind::terminator_branch && term.successors.size() == 1 && term.successors[0].block == merge.id) {
			if (term.successors[0].arguments.size() != merge.arguments.size()) return fail("terminator", "selection merge arguments are malformed");
			for (std::size_t index = 0; index < merge.arguments.size(); ++index) out << pad << valueName(merge.arguments[index].value) << " = " << valueName(term.successors[0].arguments[index]) << ";\n";
			return true;
		}
		return fail("terminator", "nested or non-structured selection arm is not supported by the D3D12 HLSL translator");
	}
	bool emitInstruction(std::ostringstream& out, const rtsl::ir::Instruction& ins, const std::unordered_map<std::uint32_t, rtsl::ir::TypeId>& value_types, std::string_view pad) {
		const bool resultless = ins.opcode == rtsl::ir::Opcode::opcode_store || ins.opcode == rtsl::ir::Opcode::opcode_emit || ins.opcode == rtsl::ir::Opcode::opcode_end_primitive;
		const std::string type = resultless ? std::string{} : typeName(module, ins.type, error);
		if (type.empty() && ins.opcode != rtsl::ir::Opcode::opcode_store && ins.opcode != rtsl::ir::Opcode::opcode_emit && ins.opcode != rtsl::ir::Opcode::opcode_end_primitive) return false;
		auto binary = [&](const char* op) { if (ins.operands.size() != 2) return fail("instruction", "binary instruction has invalid operand count"); out << pad << type << " " << valueName(ins.result) << " = " << valueName(ins.operands[0]) << " " << op << " " << valueName(ins.operands[1]) << ";\n"; return true; };
		switch (ins.opcode) {
		case rtsl::ir::Opcode::opcode_constant_boolean: out << pad << type << " " << valueName(ins.result) << " = " << (ins.immediates.empty() || !ins.immediates[0] ? "false" : "true") << ";\n"; return true;
		case rtsl::ir::Opcode::opcode_constant_integer: if (ins.immediates.empty()) return fail("instruction", "integer constant has no literal"); out << pad << type << " " << valueName(ins.result) << " = " << ins.immediates[0] << ";\n"; return true;
		case rtsl::ir::Opcode::opcode_constant_floating: {
			if (ins.immediates.empty()) return fail("instruction", "floating constant has no literal");
			std::ostringstream literal;
			literal << std::setprecision(std::numeric_limits<float>::max_digits10) << std::showpoint << std::bit_cast<float>(ins.immediates[0]);
			out << pad << type << " " << valueName(ins.result) << " = " << literal.str() << "f;\n";
			return true;
		}
		case rtsl::ir::Opcode::opcode_constant_composite:
		case rtsl::ir::Opcode::opcode_construct: {
			const rtsl::ir::Type* constructed = module.findType(ins.type);
			if (!constructed) return fail("instruction", "construction references an unknown type");
			if (constructed->kind == rtsl::ir::TypeKind::type_structure) {
				if (constructed->members.size() != ins.operands.size()) return fail("instruction", "structure construction has the wrong operand count");
				out << pad << type << " " << valueName(ins.result) << ";\n";
				for (std::size_t i = 0; i < constructed->members.size(); ++i)
					out << pad << valueName(ins.result) << "." << hlslIdentifier(module.strings.get(constructed->members[i].name)) << " = " << valueName(ins.operands[i]) << ";\n";
				return true;
			}
			out << pad << type << " " << valueName(ins.result) << " = " << type << "(";
			for (std::size_t i = 0; i < ins.operands.size(); ++i) { if (i) out << ", "; out << valueName(ins.operands[i]); }
			out << ");\n";
			return true;
		}
		case rtsl::ir::Opcode::opcode_load: {
			const auto* loaded = module.findType(ins.type);
			if (loaded && loaded->kind == rtsl::ir::TypeKind::type_patch && !entry_patch_name.empty()) return true;
			return fail("instruction", "D3D12 HLSL translator only supports loading the entry patch");
		}
		case rtsl::ir::Opcode::opcode_add: return binary("+"); case rtsl::ir::Opcode::opcode_subtract: return binary("-"); case rtsl::ir::Opcode::opcode_multiply: return binary("*"); case rtsl::ir::Opcode::opcode_divide: return binary("/");
		case rtsl::ir::Opcode::opcode_remainder: return binary("%");
		case rtsl::ir::Opcode::opcode_compare_equal: return binary("==");
		case rtsl::ir::Opcode::opcode_compare_not_equal: return binary("!=");
		case rtsl::ir::Opcode::opcode_compare_less: return binary("<");
		case rtsl::ir::Opcode::opcode_compare_less_equal: return binary("<=");
		case rtsl::ir::Opcode::opcode_compare_greater: return binary(">");
		case rtsl::ir::Opcode::opcode_compare_greater_equal: return binary(">=");
		case rtsl::ir::Opcode::opcode_logical_and: return binary("&&");
		case rtsl::ir::Opcode::opcode_logical_or: return binary("||");
		case rtsl::ir::Opcode::opcode_logical_not: if (ins.operands.size() != 1) return fail("instruction", "logical not has invalid operand count"); out << pad << type << " " << valueName(ins.result) << " = !" << valueName(ins.operands[0]) << ";\n"; return true;
		case rtsl::ir::Opcode::opcode_negate: if (ins.operands.size() != 1) return fail("instruction", "negate has invalid operand count"); out << pad << type << " " << valueName(ins.result) << " = -" << valueName(ins.operands[0]) << ";\n"; return true;
		case rtsl::ir::Opcode::opcode_extract: if (ins.operands.size() != 1 || ins.immediates.empty()) return fail("instruction", "extract is malformed"); out << pad << type << " " << valueName(ins.result) << " = " << valueName(ins.operands[0]) << "[" << ins.immediates[0] << "];\n"; return true;
		case rtsl::ir::Opcode::opcode_access: {
			if (ins.operands.empty()) return fail("instruction", "access has no base operand");
			auto found = value_types.find(ins.operands[0].value()); if (found == value_types.end()) return fail("instruction", "access base type is unavailable");
			const rtsl::ir::Type* base = module.findType(found->second); if (!base) return fail("instruction", "access base type is unknown");
			if (base->kind == rtsl::ir::TypeKind::type_pointer) base = module.findType(base->element_type);
			if (base && base->kind == rtsl::ir::TypeKind::type_patch && ins.immediates.size() == 1) {
				const std::string_view member = module.strings.get(static_cast<rtsl::ir::StringId>(ins.immediates[0]));
				if (member == "current" && !entry_current_index.empty()) {
					out << pad << type << " " << valueName(ins.result) << " = " << entry_patch_name << "[" << entry_current_index << "];\n";
					return true;
				}
				if (member == "coordinate" && !entry_coordinate_name.empty()) {
					out << pad << type << " " << valueName(ins.result) << " = " << entry_coordinate_name << ";\n";
					return true;
				}
				if (member == "outer" && ins.operands.size() == 2 && !entry_factor_name.empty()) {
					outer_accesses.emplace(ins.result.value(), entry_factor_name + ".outer[" + valueName(ins.operands[1]) + "]");
					return true;
				}
				return fail("instruction", "unsupported patch member access for this D3D12 stage: " + std::string(member));
			}
			out << pad << type << " " << valueName(ins.result) << " = " << valueName(ins.operands[0]);
			if (base->kind == rtsl::ir::TypeKind::type_structure && !ins.immediates.empty()) {
				const auto member = static_cast<rtsl::ir::StringId>(ins.immediates[0]);
				out << "." << hlslIdentifier(module.strings.get(member));
			} else if (ins.immediates.empty() && ins.operands.size() == 2) out << "[" << valueName(ins.operands[1]) << "]";
			else return fail("instruction", "access form is unsupported");
			out << ";\n"; return true;
		}
		case rtsl::ir::Opcode::opcode_store: {
			if (ins.operands.size() != 2) return fail("instruction", "store has invalid operand count");
			const auto outer = outer_accesses.find(ins.operands[0].value());
			if (outer != outer_accesses.end()) {
				if (!suppress_outer_stores) out << pad << outer->second << " = " << valueName(ins.operands[1]) << ";\n";
				return true;
			}
			return fail("instruction", "D3D12 HLSL translator only supports stores to indexed patch outer levels");
		}
		case rtsl::ir::Opcode::opcode_emit:
			if (ins.operands.size() != 1 || entry_geometry_stream.empty()) return fail("instruction", "emit is only supported by a geometry entry");
			out << pad << entry_geometry_stream << ".Append(" << valueName(ins.operands[0]) << ");\n"; return true;
		case rtsl::ir::Opcode::opcode_end_primitive:
			if (!ins.operands.empty() || entry_geometry_stream.empty()) return fail("instruction", "end_primitive is only supported by a geometry entry");
			out << pad << entry_geometry_stream << ".RestartStrip();\n"; return true;
		case rtsl::ir::Opcode::opcode_resource_load: { if (ins.immediates.size() != 1) return fail("instruction", "resource load is malformed"); const std::string name = symbolName(module, rtsl::ir::SymbolId{ins.immediates[0]}); if (name.empty()) return fail("instruction", "resource load references an unknown symbol"); out << pad << type << " " << valueName(ins.result) << " = " << name << ";\n"; return true; }
		case rtsl::ir::Opcode::opcode_call: { const auto* callee = module.findFunction(ins.callee); if (!callee) return fail("instruction", "call references unknown function"); out << pad << type << " " << valueName(ins.result) << " = " << symbolName(module, callee->symbol) << "("; for(std::size_t i=0;i<ins.operands.size();++i) { if(i) out << ", "; out << valueName(ins.operands[i]); } out << ");\n"; return true; }
		default: return fail("instruction", "RTIR opcode is not supported by the D3D12 HLSL translator");
		}
	}
	const rtsl::ir::Module& module; Error& error;
	rtsl::ir::ValueId entry_patch_parameter{};
	std::string entry_patch_name;
	std::string entry_current_index;
	std::string entry_coordinate_name;
	std::string entry_factor_name;
	std::string entry_geometry_stream;
	std::unordered_map<std::uint32_t, std::string> outer_accesses;
	bool suppress_outer_stores{};
	bool suppress_returns{};
};
}

std::optional<Translation> transpile(const rtsl::ir::Module& module, const rtsl::ir::EntryPoint& entry, Error& error) {
	const auto verification = rtsl::ir::verify(module);
	if (!verification.valid()) { const auto& issue = verification.issues().front(); error = {issue.context, issue.message}; return std::nullopt; }
	std::ostringstream source; Emitter emitter(module, error);
	if (!emitter.emit(source, entry)) return std::nullopt;
	return Translation{source.str(), "main"};
}

} // namespace rtd3d12::hlsl
