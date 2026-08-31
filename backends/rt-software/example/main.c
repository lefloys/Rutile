#include "rt_glfw_swapchain.h"
#include "rt_swapchain.h"
#include "rutile.h"

#include <GLFW/glfw3.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const u08 rtsw_demo_program[];
extern const usize rtsw_demo_program_size;

struct software_vertex {
	f32 position[3];
	f32 color[4];
};

static const struct software_vertex vertices[] = {
	{ { -0.9f, -0.9f, 0.0f }, { 0.95f, 0.15f, 0.20f, 1.0f } },
	{ {  0.9f, -0.9f, 0.0f }, { 0.10f, 0.80f, 0.95f, 1.0f } },
	{ {  0.9f,  0.9f, 0.0f }, { 0.30f, 0.95f, 0.40f, 1.0f } },
	{ { -0.9f, -0.9f, 0.0f }, { 0.95f, 0.15f, 0.20f, 1.0f } },
	{ {  0.9f,  0.9f, 0.0f }, { 0.30f, 0.95f, 0.40f, 1.0f } },
	{ { -0.9f,  0.9f, 0.0f }, { 0.45f, 0.20f, 0.95f, 1.0f } },
};

static const rt_vertex_attribute attributes[] = {
	{ "position", offsetof(struct software_vertex, position), RT_RGB32_SFLOAT },
	{ "color", offsetof(struct software_vertex, color), RT_RGBA32_SFLOAT },
};

static const rt_vertex_input input = { attributes, 2, sizeof(struct software_vertex), RT_VERTEX_RATE_VERTEX };
static const rt_vertex_layout layout = { &input, 1 };

static u32 parse_frame_count(int argc, char** argv) {
	for (int index = 1; index + 1 < argc; ++index) {
		if (strcmp(argv[index], "--frames") == 0) {
			return (u32)strtoul(argv[index + 1], NULL, 10);
		}
	}
	return 0;
}

static int fail(const char* operation) {
	fprintf(stderr, "%s: %s\n", operation, rtErrorMessage());
	return 1;
}

int main(int argc, char** argv) {
	const char* features[] = { RT_FEATURE_PRESENTATION };
	u32 frame_limit = parse_frame_count(argc, argv);
	u32 frame_count = 0;
	rt_program program;
	rt_location vertex_location;
	rt_queue queue;
	rt_buffer vertex_buffer;
	rt_command_buffer upload;
	rt_command_buffer draw;
	rt_command_buffer primary;
	rt_swapchain swapchain;
	GLFWwindow* window;

	if (rtLoad("rt-software", NULL, 0) != RT_SUCCESS) return 1;
	rtInit(features, 1);
	rtLoadSwapchain();
	rtLoadGlfwSwapchain();
	if (rtError() != RT_SUCCESS) return fail("rt-software initialization");
	if (!glfwInit()) return 1;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(960, 540, "Rutile software demo", NULL, NULL);
	if (!window) return 1;

	program = rtProgramCreate();
	rtProgramSource(program, "main", rtsw_demo_program, rtsw_demo_program_size);
	rtProgramSetLayout(program, &layout);
	rtProgramFinalize(program);
	vertex_location = rtProgramInputLocation(program, attributes, 2);
	if (rtError() != RT_SUCCESS || !vertex_location) return fail("software program");
	swapchain = rtSwapchainCreate();
	rtSwapchainBindGLFW(swapchain, window);
	queue = rtQueueCreate(RT_QUEUE_GRAPHICS);
	vertex_buffer = rtBufferCreate();
	rtBufferResize(vertex_buffer, RT_DEVICE_MEMORY, sizeof(vertices));
	upload = rtCommandBufferCreate();
	rtCommandBufferBegin(upload);
	rtCmdBufferData(upload, vertex_buffer, (rt_buffer_range){ sizeof(vertices), 0 }, (const u08*)vertices);
	rtCommandBufferEnd(upload);
	rtTimepointWait(rtQueueSubmit(queue, upload));
	draw = rtCommandBufferCreate();
	rtCommandBufferContinueRendering(draw);
	rtCmdUseProgram(draw, program);
	rtCmdVertexBuffer(draw, vertex_location, vertex_buffer, (rt_buffer_range){ sizeof(vertices), 0 });
	rtCmdDraw(draw, 6, 0);
	rtCommandBufferEnd(draw);
	primary = rtCommandBufferCreate();

	while (!glfwWindowShouldClose(window) && (!frame_limit || frame_count < frame_limit)) {
		rt_swapchain_acquire_result acquired;
		int width;
		int height;
		glfwPollEvents();
		acquired = rtSwapchainAcquire(swapchain);
		if (!acquired.framebuffer) continue;
		glfwGetFramebufferSize(window, &width, &height);
		rtCommandBufferReset(primary);
		rtCommandBufferBegin(primary);
		rtCmdBeginRendering(primary, acquired.framebuffer);
		rtCmdClearColor(primary, NULL, 0.035f, 0.040f, 0.075f, 1.0f);
		rtCmdClear(primary, RT_CLEAR_COLOR);
		rtCmdSetViewport(primary, 0, 0, (usize)width, (usize)height, 0.0f, 1.0f);
		rtCmdSetScissor(primary, 0, 0, (usize)width, (usize)height);
		rtCmdExecute(primary, draw);
		rtCmdEndRendering(primary);
		rtCommandBufferEnd(primary);
		rtSwapchainPresent(swapchain, rtQueueSubmit(queue, primary));
		if (rtError() != RT_SUCCESS) return fail("software frame");
		++frame_count;
	}

	rtTimepointWait(rtQueueFlush(queue));
	rtCommandBufferDestroy(primary);
	rtCommandBufferDestroy(draw);
	rtCommandBufferDestroy(upload);
	rtBufferDestroy(vertex_buffer);
	rtQueueDestroy(queue);
	rtSwapchainDestroy(swapchain);
	rtProgramDestroy(program);
	rtExit();
	rtUnload();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
