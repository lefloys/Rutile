#define RUTILE_IMPL
#include "cli.hpp"
#include "rt_ext_glfw.h"
#include "rt_ext_swapchain.h"
#include "rutile.h"

#include <GLFW/glfw3.h>
#include <rtsl/program.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

extern "C" const rtsl::ProgramBytes plasma_rtslp;

constexpr u32 PlasmaWidth = 480;
constexpr u32 PlasmaHeight = 270;

struct Vertex {
	f32 position[2];
	f32 uv[2];
};

rt_swapchain Swapchain = RT_NULL_HANDLE;
u32 FramebufferWidth = 1280;
u32 FramebufferHeight = 720;
f32 MouseX = 0.5f;
f32 MouseY = 0.5f;
bool MouseDown = false;

constexpr Vertex Vertices[] = {
	{ { -1, -1 }, { 0, 1 } },
	{ { 1, -1 }, { 1, 1 } },
	{ { 1, 1 }, { 1, 0 } },
	{ { -1, -1 }, { 0, 1 } },
	{ { 1, 1 }, { 1, 0 } },
	{ { -1, 1 }, { 0, 0 } },
};

constexpr rt_vertex_attribute Attributes[] = {
	{ "position", offsetof(Vertex, position), RT_RG32_SFLOAT },
	{ "uv", offsetof(Vertex, uv), RT_RG32_SFLOAT },
};

constexpr rt_vertex_layout Layout = { sizeof(Vertex), Attributes, 2 };

void framebuffer_resized(GLFWwindow*, int width, int height) {
	if (width <= 0 || height <= 0) {
		return;
	}
	FramebufferWidth = static_cast<u32>(width);
	FramebufferHeight = static_cast<u32>(height);
	rtSwapchainResize(Swapchain, static_cast<u32>(width), static_cast<u32>(height));
}

void cursor_moved(GLFWwindow*, double x, double y) {
	MouseX = std::clamp(static_cast<f32>(x) / static_cast<f32>(FramebufferWidth), 0.0f, 1.0f);
	MouseY = std::clamp(static_cast<f32>(y) / static_cast<f32>(FramebufferHeight), 0.0f, 1.0f);
}

void mouse_button(GLFWwindow*, int button, int action, int) {
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		MouseDown = action == GLFW_PRESS;
	}
}

std::uint8_t channel(f32 value) {
	return static_cast<std::uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
}

void update_plasma(std::vector<std::uint8_t>& pixels, f32 time) {
	const f32 aspect = static_cast<f32>(PlasmaWidth) / static_cast<f32>(PlasmaHeight);
	for (u32 y = 0; y < PlasmaHeight; ++y) {
		for (u32 x = 0; x < PlasmaWidth; ++x) {
			const f32 u = static_cast<f32>(x) / static_cast<f32>(PlasmaWidth - 1);
			const f32 v = static_cast<f32>(y) / static_cast<f32>(PlasmaHeight - 1);
			const f32 px = (u - 0.5f) * aspect;
			const f32 py = v - 0.5f;
			const f32 mouse_x = (MouseX - 0.5f) * aspect;
			const f32 mouse_y = MouseY - 0.5f;
			const f32 distance = std::sqrt((px - mouse_x) * (px - mouse_x) + (py - mouse_y) * (py - mouse_y));
			f32 energy = 0.5f;
			energy += std::sin(px * 13.0f + time * 1.1f) * 0.16f;
			energy += std::sin(py * 17.0f - time * 1.4f) * 0.16f;
			energy += std::sin((px + py) * 11.0f + time * 0.8f) * 0.14f;
			energy += std::cos(std::sqrt(px * px + py * py) * 25.0f - time * 2.2f) * 0.18f;
			const f32 mouse_energy = 0.22f - distance;
			if (mouse_energy > 0.0f) {
				energy += mouse_energy * (MouseDown ? 2.8f : 0.7f);
			}
			energy = std::clamp(energy, 0.0f, 1.0f);
			const f32 red = std::clamp((energy - 0.42f) * 2.2f, 0.02f, 1.0f);
			const f32 green = std::clamp(1.0f - std::abs(energy - 0.58f) * 2.4f, 0.03f, 1.0f);
			const f32 blue = std::clamp(1.15f - energy * 1.35f, 0.08f, 1.0f);
			const std::size_t offset = (static_cast<std::size_t>(y) * PlasmaWidth + x) * 4;
			pixels[offset + 0] = channel(red);
			pixels[offset + 1] = channel(green);
			pixels[offset + 2] = channel(blue);
			pixels[offset + 3] = 255;
		}
	}
}

int main(int argc, char** argv) {
	const ExampleOptions options = parse_cli(argc, argv);
	if (rtLoad(options.backend.c_str(), nullptr, 0) != RT_SUCCESS) {
		std::fprintf(stderr, "rtLoad failed\n");
		return 1;
	}
	const char* features[] = { RT_FEATURE_PRESENTATION };
	rtInit(features, 1);
	rtLoad_RT_EXT_SWAPCHAIN();
	rtLoad_RT_EXT_GLFW();

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Rutile 03 Plasma Lab - drag to excite", nullptr, nullptr);
	glfwSetFramebufferSizeCallback(window, framebuffer_resized);
	glfwSetCursorPosCallback(window, cursor_moved);
	glfwSetMouseButtonCallback(window, mouse_button);

	rt_swapchain swapchain = rtSwapchainCreate();
	rtSwapchainBindWindowGLFW(swapchain, window);
	Swapchain = swapchain;
	rt_queue queue = rtQueueQuery(RT_QUEUE_GRAPHICS);

	rt_buffer vertex_buffer = rtBufferCreate();
	rtBufferData(vertex_buffer, RT_BUFFER_STATIC, RT_BUFFER_USAGE_VERTEX, sizeof(Vertices), Vertices);
	std::vector<std::uint8_t> pixels(static_cast<std::size_t>(PlasmaWidth) * PlasmaHeight * 4);
	update_plasma(pixels, 0.0f);
	rt_texture texture = rtTextureCreate();
	rtTextureData(texture, RT_TEXTURE_2D, 0, PlasmaWidth, PlasmaHeight, 1, RT_RGBA8_UNORM, pixels.data());
	rt_texture_view texture_view = rtTextureViewCreate();
	rtTextureViewBind(texture_view, texture);
	rtTextureViewFilter(texture_view, RT_FILTER_LINEAR, RT_FILTER_LINEAR, RT_MIP_FILTER_NONE);
	rtTextureViewAddress(texture_view, RT_ADDRESS_CLAMP, RT_ADDRESS_CLAMP, RT_ADDRESS_CLAMP);

	rt_graphics_program program = rtGraphicsProgramCreate();
	rtGraphicsProgramSource(program, plasma_rtslp.size, plasma_rtslp.data);
	rtGraphicsProgramLayout(program, &Layout);
	rtGraphicsProgramFinalize(program);
	rt_uniform_location plasma_location = rtGraphicsProgramUniformLocation(program, "plasma");
	rt_command_context command_context = rtCommandContextCreate();
	rtCommandContextBind(command_context, queue);
	rt_command_buffer command_buffer = rtCommandContextAllocate(command_context);
	const auto start = std::chrono::steady_clock::now();
	u32 rendered_frames = 0;

	while (!glfwWindowShouldClose(window) && (!options.frames || rendered_frames < options.frames)) {
		glfwPollEvents();
		const f32 time = std::chrono::duration<f32>(std::chrono::steady_clock::now() - start).count();
		update_plasma(pixels, time);
		rtTextureSubdata(texture, 0, 0, 0, 0, PlasmaWidth, PlasmaHeight, 1, pixels.data());
		rt_swapchain_acquire_result acquired = rtSwapchainAcquire(swapchain);
		if (!acquired.framebuffer) {
			continue;
		}
		rtQueueWait(queue, acquired.timepoint);
		rtCommandContextBind(command_context, queue);
		rtCommandContextBindFramebuffer(command_context, acquired.framebuffer);
		rtCommandContextClearColor(command_context, 0, 0.0f, 0.0f, 0.0f, 1.0f);
		rtCmdBegin(command_buffer);
		rtCmdUseGraphicsProgram(command_buffer, program);
		rtCmdUniformTexture(command_buffer, plasma_location, texture_view);
		rtCmdBindVertexBuffer(command_buffer, vertex_buffer, 0);
		rtCmdDraw(command_buffer, 6, 0);
		rtCmdEnd(command_buffer);
		rtCommandContextExecute(command_context, command_buffer);
		rtCommandContextEndRendering(command_context);
		rtSwapchainPresent(swapchain, rtCommandContextSubmit(command_context));
		rendered_frames++;
	}

	rtTimepointWait(rtQueueFlush(queue));
	rtCommandBufferDestroy(command_buffer);
	rtCommandContextDestroy(command_context);
	rtGraphicsProgramDestroy(program);
	rtTextureViewDestroy(texture_view);
	rtTextureDestroy(texture);
	rtBufferDestroy(vertex_buffer);
	rtSwapchainDestroy(swapchain);
	glfwDestroyWindow(window);
	glfwTerminate();
	rtExit();
	rtUnload();
	return 0;
}
