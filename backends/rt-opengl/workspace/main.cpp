#define RUTILE_IMPL
#include <rt_ext_glfw.h>
#include <rt_ext_swapchain.h>
#include <rutile.h>

#include <GLFW/glfw3.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

int main(int argc, char** argv) {
	u32 frame_limit = 0;
	if (argc == 3 && std::strcmp(argv[1], "--frames") == 0) {
		frame_limit = static_cast<u32>(std::strtoul(argv[2], nullptr, 10));
	}
	rtLoadDevelopment("rt-opengl", nullptr, 0);

	const char* features[] = { RT_FEATURE_PRESENTATION };
	rtInit(features, 1);
	rtLoad_RT_EXT_SWAPCHAIN();
	rtLoad_RT_EXT_GLFW();

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(640, 360, "rt-opengl workspace", nullptr, nullptr);

	rt_swapchain swapchain = rtSwapchainCreate();
	rtSwapchainBindGLFW(swapchain, window);
	rt_queue queue = rtQueueCreate(RT_QUEUE_GRAPHICS);

	rt_command_buffer command_buffer = rtCommandBufferCreate();
	u32 rendered_frames = 0;
	while (!glfwWindowShouldClose(window) && (!frame_limit || rendered_frames < frame_limit)) {
		glfwPollEvents();
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
		if (glfwWindowShouldClose(window)) {
			break;
		}

		rt_swapchain_acquire_result acquired = rtSwapchainAcquire(swapchain);
		rtQueueWait(queue, acquired.timepoint);
		rtCommandBufferReset(command_buffer);
		rtCommandBufferBegin(command_buffer);
		rtCmdBeginRendering(command_buffer, acquired.framebuffer);
		rtCmdClearColor(command_buffer, nullptr, 0.02f, 0.11f, 0.18f, 1.0f);
		rtCmdClear(command_buffer, RT_CLEAR_COLOR);
		rtCmdEndRendering(command_buffer);
		rtCommandBufferEnd(command_buffer);

		rt_timepoint cleared = rtQueueSubmit(queue, command_buffer);
		rtSwapchainPresent(swapchain, cleared);
		rtQueueFlush(queue);
		rendered_frames++;
	}
	rtQueueFlush(queue);

	std::cout << "rt-opengl-workspace: initialized " << rtGetName() << " and cleared the swapchain\n";

	rtCommandBufferDestroy(command_buffer);
	rtQueueDestroy(queue);
	rtSwapchainDestroy(swapchain);
	rtExit();
	rtUnload();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
