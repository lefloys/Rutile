#define RUTILE_IMPL
#include "cli.hpp"
#include "embedded_program.hpp"
#include "rt_glfw_swapchain.h"
#include "rt_swapchain.h"
#include "rutile.h"

#include <GLFW/glfw3.h>
#include <cstddef>
#include <iostream>
#include <string>

extern "C" const rt_example_program triangle_rtslp;

struct Vertex {
	f32 position[3];
	f32 color[4];
};

static const Vertex Vertices[] = {
	{ { 0.0f, -0.6f, 0.0f }, { 1.0f, 0.2f, 0.2f, 1.0f } },
	{ { 0.6f, 0.5f, 0.0f }, { 0.2f, 1.0f, 0.3f, 1.0f } },
	{ { -0.6f, 0.5f, 0.0f }, { 0.3f, 0.4f, 1.0f, 1.0f } },
};

static const rt_vertex_attribute Attributes[] = {
	{ "position", offsetof(Vertex, position), RT_RGB32_SFLOAT },
	{ "color", offsetof(Vertex, color), RT_RGBA32_SFLOAT },
};
static const rt_vertex_input Inputs[] = {
	{ Attributes, 2, sizeof(Vertex), RT_VERTEX_RATE_VERTEX },
};
static const rt_vertex_layout Layout = { Inputs, 1 };

int main(int argc, char* argv[]) {
	const ExampleOptions options = parse_cli(argc, argv);
	const auto layers = selected_layers(options);
	if (rtLoad(options.backend.c_str(), layers.data(), layers.size()) != RT_SUCCESS) {
		std::cerr << "rtLoad failed\n";
		return 1;
	}
	const char* features[] = { RT_FEATURE_PRESENTATION };
	rtInit(features, 1);
	rtLoadSwapchain();
	rtLoadGlfwSwapchain();

	rt_program program = rtProgramCreate();
	rtProgramSource(program, "main", triangle_rtslp.data, triangle_rtslp.size);
	rtProgramSetLayout(program, &Layout);
	rtProgramFinalize(program);

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	const std::string window_title = "Rutile 01 Triangle - " + options.backend;
	GLFWwindow* window = glfwCreateWindow(960, 540, window_title.c_str(), nullptr, nullptr);

	rt_swapchain swapchain = rtSwapchainCreate();
	rtSwapchainBindGLFW(swapchain, window);

	rt_queue queue = rtQueueCreate(RT_QUEUE_GRAPHICS);
	rt_buffer vbo = rtBufferCreate();
	rtBufferResize(vbo, RT_DEVICE_MEMORY, sizeof(Vertices));

	rt_location vertex_location = rtProgramInputLocation(program, Attributes, 2);
	rt_command_buffer cmd = rtCommandBufferCreate();
	rtCommandBufferBegin(cmd);
	rtCmdBufferData(cmd, vbo, { sizeof(Vertices), 0 }, reinterpret_cast<const u08*>(Vertices));
	rtCommandBufferEnd(cmd);
	rtTimepointWait(rtQueueSubmit(queue, cmd));
	rtCommandBufferReset(cmd);
	rtCommandBufferContinueRendering(cmd);
	rtCmdUseProgram(cmd, program);
	rtCmdVertexBuffer(cmd, vertex_location, vbo, { sizeof(Vertices), 0 });
	rtCmdDraw(cmd, 3, 0);
	rtCommandBufferEnd(cmd);
	rt_command_buffer primary = rtCommandBufferCreate();
	u32 rendered_frames = 0;

	while (!glfwWindowShouldClose(window) && (!options.frames || rendered_frames < options.frames)) {
		glfwPollEvents();

		rt_swapchain_acquire_result acquired = rtSwapchainAcquire(swapchain);
		if (!acquired.framebuffer) {
			continue;
		}

		rtQueueWait(queue, acquired.timepoint);
		rtCommandBufferReset(primary);
		rtCommandBufferBegin(primary);
		rtCmdBeginRendering(primary, acquired.framebuffer);
		rtCmdClearColor(primary, nullptr, 0.08f, 0.09f, 0.12f, 1.0f);
		rtCmdClear(primary, RT_CLEAR_COLOR);
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		rtCmdSetViewport(primary, 0, 0, (usize)width, (usize)height, 0.0f, 1.0f);
		rtCmdSetScissor(primary, 0, 0, (usize)width, (usize)height);
		rtCmdExecute(primary, cmd);
		rtCmdEndRendering(primary);
		rtCommandBufferEnd(primary);

		rtSwapchainPresent(swapchain, rtQueueSubmit(queue, primary));
		rendered_frames++;
	}

	rtTimepointWait(rtQueueFlush(queue));
	rtCommandBufferDestroy(primary);
	rtCommandBufferDestroy(cmd);
	rtQueueDestroy(queue);
	rtProgramDestroy(program);
	rtBufferDestroy(vbo);
	rtSwapchainDestroy(swapchain);
	rtExit();
	rtUnload();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
