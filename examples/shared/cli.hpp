#pragma once

#include <CLI/CLI.hpp>

#include <cstdint>
#include <cstdlib>

struct ExampleOptions {
	std::uint32_t frames;
};

inline ExampleOptions parse_cli(int argc, char** argv) {
	CLI::App cli;
	ExampleOptions options;
	cli.add_option("--frames", options.frames, "Exit after rendering this many frames")->default_val(0);
	try {
		cli.parse(argc, argv);
	} catch (const CLI::ParseError& error) {
		std::exit(cli.exit(error));
	}
	return options;
}
