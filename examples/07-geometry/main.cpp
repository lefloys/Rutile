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

extern "C" const rt_example_program geometry_rtslp;

struct Point {
	f32 position[2];
	f32 color[4];
};

constexpr Point Points[] = {
	{ { -0.78f, -0.72f }, { 1.0f, 0.28f, 0.20f, 1.0f } },
	{ {  0.78f, -0.72f }, { 0.20f, 0.82f, 1.0f, 1.0f } },
	{ {  0.00f,  0.78f }, { 0.96f, 0.76f, 0.18f, 1.0f } },
};

constexpr rt_vertex_attribute Attributes[] = {
	{ "position", offsetof(Point, position), RT_RG32_SFLOAT },
	{ "color", offsetof(Point, color), RT_RGBA32_SFLOAT },
};
constexpr rt_vertex_input Inputs[] = {
	{ Attributes, 2, sizeof(Point), RT_VERTEX_RATE_VERTEX },
};
constexpr rt_vertex_layout Layout = { Inputs, 1 };
constexpr const char* Features[] = { RT_FEATURE_PRESENTATION };

bool report_error(const char* operation) {
	if (rtError() == RT_SUCCESS) return true;
	std::cerr << operation << ": " << rtErrorMessage() << '\n';
	rtClearError();
	return false;
}

int main(int argc, char* argv[]) {
	const ExampleOptions options = parse_cli(argc, argv);
	const auto layers = selected_layers(options);
	if (rtLoad(options.backend.c_str(), layers.data(), layers.size()) != RT_SUCCESS) {
		std::cerr << "rtLoad failed\n";
		return 1;
	}
	rtInit(Features, 1);
	rtLoadSwapchain();
	rtLoadGlfwSwapchain();

	rt_program program = rtProgramCreate();
	rtProgramSource(program, "main", geometry_rtslp.data, geometry_rtslp.size);
	rtProgramSetLayout(program, &Layout);
	rtProgramFinalize(program);
	if (!report_error("rtProgramFinalize")) return 1;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	const std::string title = "Rutile 07 Geometry - " + options.backend;
	GLFWwindow* window = glfwCreateWindow(960, 540, title.c_str(), nullptr, nullptr);
	rt_swapchain swapchain = rtSwapchainCreate();
	rtSwapchainBindGLFW(swapchain, window);
	rt_queue queue = rtQueueCreate(RT_QUEUE_GRAPHICS);
	rt_buffer points = rtBufferCreate();
	rtBufferResize(points, RT_DEVICE_MEMORY, sizeof(Points));

	const rt_location location = rtProgramInputLocation(program, Attributes, 2);
	rt_command_buffer draw = rtCommandBufferCreate();
	rtCommandBufferBegin(draw);
	rtCmdBufferData(draw, points, { sizeof(Points), 0 }, reinterpret_cast<const u08*>(Points));
	rtCommandBufferEnd(draw);
	rtTimepointWait(rtQueueSubmit(queue, draw));
	if (!report_error("vertex upload")) return 1;
	rtCommandBufferReset(draw);
	rtCommandBufferContinueRendering(draw);
	rtCmdUseProgram(draw, program);
	rtCmdVertexBuffer(draw, location, points, { sizeof(Points), 0 });
	rtCmdDraw(draw, 3, 0);
	rtCommandBufferEnd(draw);

	rt_command_buffer primary = rtCommandBufferCreate();
	u32 frames{};
	while (!glfwWindowShouldClose(window) && (!options.frames || frames < options.frames)) {
		glfwPollEvents();
		rt_swapchain_acquire_result acquired = rtSwapchainAcquire(swapchain);
		if (!acquired.framebuffer) continue;
		int width{}, height{};
		glfwGetFramebufferSize(window, &width, &height);
		rtQueueWait(queue, acquired.timepoint);
		rtCommandBufferReset(primary);
		rtCommandBufferBegin(primary);
		rtCmdBeginRendering(primary, acquired.framebuffer);
		rtCmdClearColor(primary, nullptr, 0.03f, 0.04f, 0.08f, 1.0f);
		rtCmdClear(primary, RT_CLEAR_COLOR);
		rtCmdSetViewport(primary, 0, 0, static_cast<usize>(width), static_cast<usize>(height), 0.0f, 1.0f);
		rtCmdSetScissor(primary, 0, 0, static_cast<usize>(width), static_cast<usize>(height));
		rtCmdExecute(primary, draw);
		rtCmdEndRendering(primary);
		rtCommandBufferEnd(primary);
		const rt_timepoint rendered = rtQueueSubmit(queue, primary);
		if (!rendered.value || !report_error("rtQueueSubmit")) break;
		rtSwapchainPresent(swapchain, rendered);
		if (!report_error("rtSwapchainPresent")) break;
		++frames;
	}

	rtTimepointWait(rtQueueFlush(queue));
	rtCommandBufferDestroy(primary);
	rtCommandBufferDestroy(draw);
	rtBufferDestroy(points);
	rtQueueDestroy(queue);
	rtSwapchainDestroy(swapchain);
	glfwDestroyWindow(window);
	glfwTerminate();
	rtProgramDestroy(program);
	rtExit();
	rtUnload();
	return 0;
}
