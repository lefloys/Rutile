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

extern "C" const rt_example_program software_debug_rtslp;

struct Vertex { f32 position[2]; f32 color[3]; };
static const Vertex Vertices[] = {
	{{-0.75f, -0.70f}, {1.0f, 0.15f, 0.10f}},
	{{ 0.75f, -0.55f}, {0.10f, 1.0f, 0.20f}},
	{{ 0.00f,  0.80f}, {0.15f, 0.35f, 1.0f}},
};
static const rt_vertex_attribute Attributes[] = {{"position", offsetof(Vertex, position), RT_RG32_SFLOAT}, {"color", offsetof(Vertex, color), RT_RGB32_SFLOAT}};
static const rt_vertex_input Inputs[] = {{Attributes, 2, sizeof(Vertex), RT_VERTEX_RATE_VERTEX}};
static const rt_vertex_layout Layout = {Inputs, 1};
static const char* DebugLayers[] = {"rt-debug"};

int main(int argc, char* argv[]) {
	const ExampleOptions options = parse_cli(argc, argv);
	const auto layers = selected_layers(options, DebugLayers);
	if (rtLoad(options.backend.c_str(), layers.data(), layers.size()) != RT_SUCCESS) { std::cerr << "rtLoad failed\n"; return 1; }
	const char* features[] = {RT_FEATURE_PRESENTATION}; rtInit(features, 1); rtLoadSwapchain(); rtLoadGlfwSwapchain();
	rt_program program = rtProgramCreate(); rtProgramSource(program, "main", software_debug_rtslp.data, software_debug_rtslp.size); rtProgramSetLayout(program, &Layout); rtProgramFinalize(program);
	glfwInit(); glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); const std::string window_title = "Rutile rt-debug software - " + options.backend; GLFWwindow* window = glfwCreateWindow(960, 540, window_title.c_str(), nullptr, nullptr);
	rt_swapchain swapchain = rtSwapchainCreate(); rtSwapchainBindGLFW(swapchain, window); rt_queue queue = rtQueueCreate(RT_QUEUE_GRAPHICS);
	rt_buffer vertices = rtBufferCreate(); rtBufferResize(vertices, RT_DEVICE_MEMORY, sizeof(Vertices)); rt_location input = rtProgramInputLocation(program, Attributes, 2);
	rt_command_buffer draw = rtCommandBufferCreate(); rtCommandBufferBegin(draw); rtCmdBufferData(draw, vertices, {sizeof(Vertices), 0}, reinterpret_cast<const u08*>(Vertices)); rtCommandBufferEnd(draw); rtTimepointWait(rtQueueSubmit(queue, draw));
	rtCommandBufferReset(draw); rtCommandBufferContinueRendering(draw); rtCmdUseProgram(draw, program); rtCmdVertexBuffer(draw, input, vertices, {sizeof(Vertices), 0}); rtCmdDraw(draw, 3, 0); rtCommandBufferEnd(draw);
	rt_command_buffer primary = rtCommandBufferCreate();
	while (!glfwWindowShouldClose(window)) { glfwPollEvents(); rt_swapchain_acquire_result acquired = rtSwapchainAcquire(swapchain); if (!acquired.framebuffer) continue; rtQueueWait(queue, acquired.timepoint); int width, height; glfwGetFramebufferSize(window, &width, &height); rtCommandBufferReset(primary); rtCommandBufferBegin(primary); rtCmdBeginRendering(primary, acquired.framebuffer); rtCmdClearColor(primary, nullptr, .05f, .06f, .09f, 1); rtCmdClear(primary, RT_CLEAR_COLOR); rtCmdSetViewport(primary, 0, 0, (usize)width, (usize)height, 0, 1); rtCmdExecute(primary, draw); rtCmdEndRendering(primary); rtCommandBufferEnd(primary); rtSwapchainPresent(swapchain, rtQueueSubmit(queue, primary)); }
	rtTimepointWait(rtQueueFlush(queue)); rtCommandBufferDestroy(primary); rtCommandBufferDestroy(draw); rtBufferDestroy(vertices); rtQueueDestroy(queue); rtProgramDestroy(program); rtSwapchainDestroy(swapchain); rtExit(); rtUnload(); glfwDestroyWindow(window); glfwTerminate();
}
