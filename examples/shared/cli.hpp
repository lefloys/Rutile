#pragma once

#include <CLI/CLI.hpp>

#include <cstdint>
#include <cstdlib>

#include <string>

struct ExampleOptions {
	std::uint32_t frames;
	std::string backend;
};

inline ExampleOptions parse_cli(int argc, char* argv[]) {
	CLI::App cli;
	ExampleOptions options;
	cli.add_option("--frames", options.frames, "Exit after rendering this many frames")->default_val(0);
	cli.add_option("--backend", options.backend, "What backend to use for the example")->default_val("rt-vulkan");
	try {
		cli.parse(argc, argv);
	} catch (const CLI::ParseError& error) {
		std::exit(cli.exit(error));
	}
	return options;
}
