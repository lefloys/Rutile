#define RUTILE_IMPL
#include "rt_ext_glfw.h"
#include "rt_ext_swapchain.h"
#include "rutile.h"

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
	rtSwapchainBindWindowGLFW(swapchain, window);
	rt_queue queue = rtQueueCreate(RT_QUEUE_GRAPHICS);

	u32 upload[] = { 1, 2, 3, 4 };
	u32 update[] = { 20, 30 };
	u32 expected[] = { 1, 20, 30, 4 };
	u32 download[sizeof(expected) / sizeof(expected[0])] = {};

	rt_buffer buffer = rtBufferCreate();
	rtBufferData(buffer, RT_BUFFER_DYNAMIC, (rt_buffer_usage)(RT_BUFFER_USAGE_STAGING | RT_BUFFER_USAGE_TRANSFER_SRC | RT_BUFFER_USAGE_TRANSFER_DST), sizeof(upload), upload);
	rtBufferSubdata(buffer, sizeof(u32), sizeof(update), update);
	rtBufferRead(buffer, 0, sizeof(download), download);

	if (std::memcmp(download, expected, sizeof(expected)) != 0) {
		std::cerr << "rt-opengl-workspace: buffer readback mismatch: {" << download[0] << ", " << download[1] << ", " << download[2] << ", " << download[3] << "}\n";
		rtBufferDestroy(buffer);
		rtSwapchainDestroy(swapchain);
		rtExit();
		rtUnload();
		glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}

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
		rtCmdReset(command_buffer);
		rtCmdBegin(command_buffer);
		rtCmdWait(command_buffer, acquired.timepoint);
		rtCmdBeginRendering(command_buffer, acquired.framebuffer);
		rtCmdClearColor(command_buffer, 0, 0.02f, 0.11f, 0.18f, 1.0f);
		rtCmdEndRendering(command_buffer);
		rtCmdEnd(command_buffer);

		rt_timepoint cleared = rtQueueSubmit(queue, command_buffer);
		rtSwapchainPresent(swapchain, cleared);
		rtQueueFlush(queue);
		rendered_frames++;
	}
	rtQueueFlush(queue);

	std::cout << "rt-opengl-workspace: initialized " << rtGetName() << ", verified buffer upload/readback, and cleared the swapchain\n";

	rtCommandBufferDestroy(command_buffer);
	rtQueueDestroy(queue);
	rtBufferDestroy(buffer);
	rtSwapchainDestroy(swapchain);
	rtExit();
	rtUnload();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
