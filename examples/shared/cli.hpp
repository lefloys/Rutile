#pragma once

#include <CLI/CLI.hpp>

#include <cstdint>
#include <cstdlib>

#include <string>
#include <span>
#include <vector>

struct ExampleOptions {
	std::uint32_t frames;
	std::string backend;
	std::vector<std::string> layers;
};

inline ExampleOptions parse_cli(int argc, char* argv[]) {
	CLI::App cli;
	ExampleOptions options;
	cli.add_option("--frames", options.frames, "Exit after rendering this many frames")->default_val(0);
	cli.add_option("--backend", options.backend, "What backend to use for the example")->default_val("rt-vulkan");
	cli.add_option("-l,--layer", options.layers, "Runtime layers to load")->expected(-1);
	try {
		cli.parse(argc, argv);
	} catch (const CLI::ParseError& error) {
		std::exit(cli.exit(error));
	}
	return options;
}

inline std::vector<const char*> selected_layers(const ExampleOptions& options,
	std::span<const char* const> built_in_layers = {}) {
	std::vector<const char*> result;
	result.reserve(built_in_layers.size() + options.layers.size());
	result.insert(result.end(), built_in_layers.begin(), built_in_layers.end());
	for (const std::string& layer : options.layers) result.push_back(layer.c_str());
	return result;
}
