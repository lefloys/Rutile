#include "rt_ext_glfw.h"

PFN_rtInit_RT_EXT_GLFW rt_rtInit_RT_EXT_GLFW = NULL;
PFN_rtSwapchainBindWindowGLFW rt_rtSwapchainBindWindowGLFW = NULL;

static void rt_clear_glfw_extension(void) {
	rt_rtInit_RT_EXT_GLFW = NULL;
	rt_rtSwapchainBindWindowGLFW = NULL;
}

enum rt_error rtLoad_RT_EXT_GLFW(void) {
	rt_clear_glfw_extension();
	PFN_rtInit_RT_EXT_GLFW init = (PFN_rtInit_RT_EXT_GLFW)rtGetProc("rtInit_RT_EXT_GLFW");
	if (init && !init()) {
		return rtError() != RT_SUCCESS ? rtError() : RT_UNSUPPORTED_FEATURE;
	}
	rt_rtSwapchainBindWindowGLFW = (PFN_rtSwapchainBindWindowGLFW)rtGetProc("rtSwapchainBindWindowGLFW");
	if (!rt_rtSwapchainBindWindowGLFW) {
		rt_clear_glfw_extension();
		return RT_EXTENSION_NOT_PRESENT;
	}
	rt_rtInit_RT_EXT_GLFW = init;
	return RT_SUCCESS;
}

#if defined(_WIN32) && defined(_MSC_VER)
#pragma comment(linker, "/EXPORT:glfwCreateWindowSurface")
#pragma comment(linker, "/EXPORT:glfwGetFramebufferSize")
#pragma comment(linker, "/EXPORT:glfwGetError")
#pragma comment(linker, "/EXPORT:glfwVulkanSupported")
#pragma comment(linker, "/EXPORT:glfwGetWin32Window")
#endif
