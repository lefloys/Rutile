#pragma once

#include <rtsl/IR/IR.hpp>

#include <optional>
#include <string>

namespace rtd3d12::hlsl {

struct Translation {
	std::string source;
	std::string entry_point;
};

struct Error {
	std::string context;
	std::string message;
};

[[nodiscard]] std::optional<Translation> transpile(const rtsl::ir::Module& module,
	const rtsl::ir::EntryPoint& entry, Error& error);

} // namespace rtd3d12::hlsl
