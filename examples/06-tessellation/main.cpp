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

extern "C" const rt_example_program bezier_rtslp;
extern "C" const rt_example_program control_points_rtslp;

struct ControlPoint {
	f32 position[2];
};

constexpr ControlPoint ControlPoints[] = {
	{ { -0.9f, -0.7f } },
	{ { -0.5f, 0.9f } },
	{ { 0.5f, -0.9f } },
	{ { 0.9f, 0.7f } },
};

constexpr ControlPoint MarkerVertices[] = {
	{ { -0.925f, -0.725f } }, { { -0.875f, -0.725f } }, { { -0.875f, -0.675f } }, { { -0.925f, -0.725f } }, { { -0.875f, -0.675f } }, { { -0.925f, -0.675f } },
	{ { -0.525f, 0.875f } }, { { -0.475f, 0.875f } }, { { -0.475f, 0.925f } }, { { -0.525f, 0.875f } }, { { -0.475f, 0.925f } }, { { -0.525f, 0.925f } },
	{ { 0.475f, -0.925f } }, { { 0.525f, -0.925f } }, { { 0.525f, -0.875f } }, { { 0.475f, -0.925f } }, { { 0.525f, -0.875f } }, { { 0.475f, -0.875f } },
	{ { 0.875f, 0.675f } }, { { 0.925f, 0.675f } }, { { 0.925f, 0.725f } }, { { 0.875f, 0.675f } }, { { 0.925f, 0.725f } }, { { 0.875f, 0.725f } },
};

constexpr rt_vertex_attribute ControlPointAttributes[] = {
	{ "position", offsetof(ControlPoint, position), RT_RG32_SFLOAT },
};
constexpr rt_vertex_input ControlPointInputs[] = {
	{ ControlPointAttributes, 1, sizeof(ControlPoint), RT_VERTEX_RATE_VERTEX },
};
constexpr rt_vertex_layout ControlPointLayout = { ControlPointInputs, 1 };
constexpr const char* Features[] = { RT_FEATURE_PRESENTATION };

static bool report_error(const char* operation) {
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
	rtProgramSource(program, "main", bezier_rtslp.data, bezier_rtslp.size);
	rtProgramSetLayout(program, &ControlPointLayout);
	rtProgramFinalize(program);
	if (!report_error("rtProgramFinalize")) return 1;
	rt_program markers_program = rtProgramCreate();
	rtProgramSource(markers_program, "main", control_points_rtslp.data, control_points_rtslp.size);
	rtProgramSetLayout(markers_program, &ControlPointLayout);
	rtProgramFinalize(markers_program);
	if (!report_error("marker rtProgramFinalize")) return 1;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	const std::string window_title = "Rutile 06 Tessellation - " + options.backend;
	GLFWwindow* window = glfwCreateWindow(960, 540, window_title.c_str(), nullptr, nullptr);

	rt_swapchain swapchain = rtSwapchainCreate();
	rtSwapchainBindGLFW(swapchain, window);
	rt_queue queue = rtQueueCreate(RT_QUEUE_GRAPHICS);

	rt_buffer control_points = rtBufferCreate();
	rtBufferResize(control_points, RT_DEVICE_MEMORY, sizeof(ControlPoints));
	rt_buffer marker_vertices = rtBufferCreate();
	rtBufferResize(marker_vertices, RT_DEVICE_MEMORY, sizeof(MarkerVertices));

	const rt_location control_point_location = rtProgramInputLocation(program, ControlPointAttributes, 1);
	const rt_location marker_location = rtProgramInputLocation(markers_program, ControlPointAttributes, 1);
	rt_command_buffer draw = rtCommandBufferCreate();
	rtCommandBufferBegin(draw);
	rtCmdBufferData(draw, control_points, { sizeof(ControlPoints), 0 }, reinterpret_cast<const u08*>(ControlPoints));
	rtCmdBufferData(draw, marker_vertices, { sizeof(MarkerVertices), 0 }, reinterpret_cast<const u08*>(MarkerVertices));
	rtCommandBufferEnd(draw);
	rtTimepointWait(rtQueueSubmit(queue, draw));
	if (!report_error("control-point upload")) return 1;
	rtCommandBufferReset(draw);
	rtCommandBufferContinueRendering(draw);
	rtCmdUseProgram(draw, program);
	rtCmdVertexBuffer(draw, control_point_location, control_points, { sizeof(ControlPoints), 0 });
	rtCmdDraw(draw, 4, 0);
	rtCmdUseProgram(draw, markers_program);
	rtCmdVertexBuffer(draw, marker_location, marker_vertices, { sizeof(MarkerVertices), 0 });
	rtCmdDraw(draw, sizeof(MarkerVertices) / sizeof(MarkerVertices[0]), 0);
	rtCommandBufferEnd(draw);

	rt_command_buffer primary = rtCommandBufferCreate();
	u32 rendered_frames = 0;
	while (!glfwWindowShouldClose(window) && (!options.frames || rendered_frames < options.frames)) {
		glfwPollEvents();

		rt_swapchain_acquire_result acquired = rtSwapchainAcquire(swapchain);
		if (!acquired.framebuffer) continue;

		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		rtQueueWait(queue, acquired.timepoint);
		rtCommandBufferReset(primary);
		rtCommandBufferBegin(primary);
		rtCmdBeginRendering(primary, acquired.framebuffer);
		rtCmdClearColor(primary, nullptr, 0.03f, 0.04f, 0.08f, 1.0f);
		rtCmdClear(primary, RT_CLEAR_COLOR);
		rtCmdSetViewport(primary, 0, 0, (usize)width, (usize)height, 0.0f, 1.0f);
		rtCmdSetScissor(primary, 0, 0, (usize)width, (usize)height);
		rtCmdExecute(primary, draw);
		rtCmdEndRendering(primary);
		rtCommandBufferEnd(primary);
		const rt_timepoint rendered = rtQueueSubmit(queue, primary);
		if (!rendered.value || !report_error("rtQueueSubmit")) break;
		rtSwapchainPresent(swapchain, rendered);
		if (!report_error("rtSwapchainPresent")) break;
		rendered_frames++;
	}

	rtTimepointWait(rtQueueFlush(queue));
	rtCommandBufferDestroy(primary);
	rtCommandBufferDestroy(draw);
	rtBufferDestroy(marker_vertices);
	rtBufferDestroy(control_points);
	rtQueueDestroy(queue);
	rtSwapchainDestroy(swapchain);
	glfwDestroyWindow(window);
	glfwTerminate();
	rtProgramDestroy(program);
	rtProgramDestroy(markers_program);
	rtExit();
	rtUnload();
	return 0;
}
