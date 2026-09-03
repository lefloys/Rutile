#define RUTILE_IMPL
#include "cli.hpp"
#include "embedded_program.hpp"
#include "rt_glfw_swapchain.h"
#include "rt_swapchain.h"
#include "rutile.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

extern "C" const rt_example_program temperature_rtslp;

constexpr usize Width = 256;
constexpr usize Height = 144;

struct Point {
	f32 position[2];
	f32 uv[2];
};

struct Brush {
	u32 x;
	u32 y;
	u32 radius;
	u32 active;
	f32 strength;
};
static_assert(sizeof(Brush) == sizeof(u32) * 4 + sizeof(f32));

constexpr Point Points[] = {
	{ { -1.0f, -1.0f }, { 0.0f, 1.0f } },
	{ { 1.0f, -1.0f }, { 1.0f, 1.0f } },
	{ { 1.0f, 1.0f }, { 1.0f, 0.0f } },
	{ { -1.0f, -1.0f }, { 0.0f, 1.0f } },
	{ { 1.0f, 1.0f }, { 1.0f, 0.0f } },
	{ { -1.0f, 1.0f }, { 0.0f, 0.0f } },
};

constexpr rt_vertex_attribute Attributes[] = {
	{ "position", offsetof(Point, position), RT_RG32_SFLOAT },
	{ "uv", offsetof(Point, uv), RT_RG32_SFLOAT },
};
constexpr rt_vertex_input Inputs[] = {
	{ Attributes, 2, sizeof(Point), RT_VERTEX_RATE_VERTEX },
};
constexpr rt_vertex_layout Layout = { Inputs, 1 };
constexpr rt_texture_range TemperatureRange = {
	RT_TEXTURE_ASPECT_COLOR,
	0, 1,
	0, 1,
	{ Width, Height, 1 },
	{ 0, 0, 0 },
};
constexpr const char* Features[] = { RT_FEATURE_PRESENTATION };

rt_swapchain Swapchain = RT_NULL_HANDLE;
u32 FramebufferWidth = 960;
u32 FramebufferHeight = 540;
f32 MouseX = 0.5f;
f32 MouseY = 0.5f;
bool LeftMouseDown = false;
bool RightMouseDown = false;

void framebuffer_resized(GLFWwindow*, int width, int height) {
	if (width <= 0 || height <= 0) {
		return;
	}
	FramebufferWidth = static_cast<u32>(width);
	FramebufferHeight = static_cast<u32>(height);
	rtSwapchainResize(Swapchain, FramebufferWidth, FramebufferHeight);
}

void cursor_moved(GLFWwindow*, double x, double y) {
	MouseX = std::clamp(static_cast<f32>(x) / static_cast<f32>(FramebufferWidth), 0.0f, 1.0f);
	MouseY = std::clamp(static_cast<f32>(y) / static_cast<f32>(FramebufferHeight), 0.0f, 1.0f);
}

void mouse_button(GLFWwindow*, int button, int action, int) {
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		LeftMouseDown = action == GLFW_PRESS;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		RightMouseDown = action == GLFW_PRESS;
	}
}

bool report_error(const char* operation) {
	if (rtError() == RT_SUCCESS) {
		return true;
	}
	std::cerr << operation << ": " << rtErrorMessage() << '\n';
	rtClearError();
	return false;
}

Brush current_brush(f32 delta_seconds) {
	return {
		static_cast<u32>(MouseX * static_cast<f32>(Width - 1)),
		static_cast<u32>(MouseY * static_cast<f32>(Height - 1)),
		8,
		(LeftMouseDown || RightMouseDown) ? 1u : 0u,
		(LeftMouseDown ? 1.0f : -1.0f) * 0.1f * delta_seconds,
	};
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

	rt_program compute_program = rtProgramCreate();
	rtProgramSource(compute_program, "temperature", temperature_rtslp.data, temperature_rtslp.size);
	rtProgramFinalize(compute_program);
	if (!report_error("rtProgramFinalize")) {
		return 1;
	}
	rt_program display_program = rtProgramCreate();
	rtProgramSource(display_program, "display", temperature_rtslp.data, temperature_rtslp.size);
	rtProgramSetLayout(display_program, &Layout);
	rtProgramFinalize(display_program);
	if (!report_error("display rtProgramFinalize")) {
		return 1;
	}

	if (!glfwInit()) {
		std::cerr << "glfwInit failed\n";
		return 1;
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	const std::string title = "Rutile 05 Temperature Simulation - " + options.backend;
	GLFWwindow* window = glfwCreateWindow(static_cast<int>(FramebufferWidth), static_cast<int>(FramebufferHeight), title.c_str(), nullptr, nullptr);
	if (!window) {
		std::cerr << "glfwCreateWindow failed\n";
		glfwTerminate();
		return 1;
	}
	glfwSetFramebufferSizeCallback(window, framebuffer_resized);
	glfwSetCursorPosCallback(window, cursor_moved);
	glfwSetMouseButtonCallback(window, mouse_button);

	rt_swapchain swapchain = rtSwapchainCreate();
	rtSwapchainBindGLFW(swapchain, window);
	Swapchain = swapchain;

	rt_texture textures[2] = { rtTextureCreate(), rtTextureCreate() };
	rt_texture_view texture_views[2] = { rtTextureViewCreate(), rtTextureViewCreate() };
	for (usize index = 0; index < 2; ++index) {
		rtTextureResize(textures[index], RT_TEXTURE_2D, RT_R32_SFLOAT, { Width, Height, 1 }, 1);
		rtTextureViewSetTexture(texture_views[index], textures[index]);
	}

	rt_buffer point_buffer = rtBufferCreate();
	rtBufferResize(point_buffer, RT_DEVICE_MEMORY, sizeof(Points));
	const rt_location input_location = rtProgramUniformLocation(compute_program, "temperature_in");
	const rt_location output_location = rtProgramUniformLocation(compute_program, "temperature_out");
	const rt_location brush_location = rtProgramUniformLocation(compute_program, "brush");
	const rt_location display_location = rtProgramUniformLocation(display_program, "temperature_out");
	const rt_location point_location = rtProgramInputLocation(display_program, Attributes, 2);
	rt_queue queue = rtQueueCreate(RT_QUEUE_GRAPHICS);
	rt_command_buffer setup = rtCommandBufferCreate();
	std::vector<f32> initial_temperature(Width * Height, 0.5f);
	rtCommandBufferBegin(setup);
	rtCmdBufferData(setup, point_buffer, { sizeof(Points), 0 }, reinterpret_cast<const u08*>(Points));
	for (usize index = 0; index < 2; ++index) {
		rtCmdTextureData(setup, textures[index], TemperatureRange, reinterpret_cast<const u08*>(initial_temperature.data()));
	}
	rtCommandBufferEnd(setup);
	rtTimepointWait(rtQueueSubmit(queue, setup));
	if (!report_error("temperature initialization")) {
		return 1;
	}

	rt_command_buffer draw = rtCommandBufferCreate();
	rtCommandBufferContinueRendering(draw);
	rtCmdUseProgram(draw, display_program);
	rtCmdVertexBuffer(draw, point_location, point_buffer, { sizeof(Points), 0 });
	rtCmdDraw(draw, 6, 0);
	rtCommandBufferEnd(draw);

	rt_command_buffer primary = rtCommandBufferCreate();
	std::array<rt_access, 2> last_access = { rt_access{ RT_STAGE_TRANSFER, RT_ACCESS_WRITE }, rt_access{ RT_STAGE_TRANSFER, RT_ACCESS_WRITE } };
	usize read_index = 0;
	u32 rendered_frames = 0;
	bool succeeded = true;
	double previous_time = glfwGetTime();
	while (!glfwWindowShouldClose(window) && (!options.frames || rendered_frames < options.frames)) {
		const auto frame_start = std::chrono::steady_clock::now();
		glfwPollEvents();
		const double current_time = glfwGetTime();
		const f32 delta_seconds = static_cast<f32>(current_time - previous_time);
		previous_time = current_time;
		rt_swapchain_acquire_result acquired = rtSwapchainAcquire(swapchain);
		if (!acquired.framebuffer) {
			continue;
		}
		const usize write_index = 1 - read_index;
		const Brush brush = current_brush(delta_seconds);
		rtQueueWait(queue, acquired.timepoint);
		rtCommandBufferReset(primary);
		rtCommandBufferBegin(primary);
		rtCmdTextureBarrier(primary, textures[read_index], TemperatureRange, last_access[read_index], { RT_STAGE_COMPUTE, RT_ACCESS_READ });
		rtCmdTextureBarrier(primary, textures[write_index], TemperatureRange, last_access[write_index], { RT_STAGE_COMPUTE, RT_ACCESS_WRITE });
		rtCmdUseProgram(primary, compute_program);
		rtCmdBindTexture(primary, input_location, texture_views[read_index]);
		rtCmdBindTexture(primary, output_location, texture_views[write_index]);
		rtCmdUniformData(primary, brush_location, reinterpret_cast<const u08*>(&brush), sizeof(brush));
		rtCmdDispatch(primary, (Width + 15) / 16, (Height + 15) / 16, 1);
		last_access[read_index] = { RT_STAGE_COMPUTE, RT_ACCESS_READ };
		last_access[write_index] = { RT_STAGE_COMPUTE, RT_ACCESS_WRITE };
		rtCmdTextureBarrier(primary, textures[write_index], TemperatureRange, last_access[write_index], { RT_STAGE_FRAGMENT, RT_ACCESS_READ });
		last_access[write_index] = { RT_STAGE_FRAGMENT, RT_ACCESS_READ };


		rtCmdBeginRendering(primary, acquired.framebuffer);
		rtCmdClearColor(primary, nullptr, 0.02f, 0.03f, 0.05f, 1.0f);
		rtCmdClear(primary, RT_CLEAR_COLOR);
		rtCmdSetViewport(primary, 0, 0, FramebufferWidth, FramebufferHeight, 0.0f, 1.0f);
		rtCmdSetScissor(primary, 0, 0, FramebufferWidth, FramebufferHeight);
		rtCmdBindTexture(primary, display_location, texture_views[write_index]);
		rtCmdExecute(primary, draw);
		rtCmdEndRendering(primary);
		rtCommandBufferEnd(primary);


		const rt_timepoint rendered = rtQueueSubmit(queue, primary);
		if (!rendered.value || !report_error("rtQueueSubmit")) {
			succeeded = false;
			break;
		}
		rtSwapchainPresent(swapchain, rendered);
		if (!report_error("rtSwapchainPresent")) {
			succeeded = false;
			break;
		}
		read_index = write_index;
		++rendered_frames;
		std::this_thread::sleep_until(frame_start + std::chrono::milliseconds(16));
	}

	rtTimepointWait(rtQueueFlush(queue));
	rtCommandBufferDestroy(primary);
	rtCommandBufferDestroy(draw);
	rtCommandBufferDestroy(setup);
	rtQueueDestroy(queue);
	rtBufferDestroy(point_buffer);
	for (usize index = 0; index < 2; ++index) {
		rtTextureViewDestroy(texture_views[index]);
		rtTextureDestroy(textures[index]);
	}
	rtSwapchainDestroy(swapchain);
	glfwDestroyWindow(window);
	glfwTerminate();
	rtProgramDestroy(display_program);
	rtProgramDestroy(compute_program);
	rtExit();
	rtUnload();
	return succeeded ? 0 : 1;
}
