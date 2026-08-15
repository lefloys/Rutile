#include "rt_ext_glfw.h"

#include <stdio.h>

rt_proc_t rtl_backend_proc(const char* name);

#define RT_DEFINE_GLFW_DISPATCH(return_type, name, parameters, arguments) \
	PFN_##name rt_##name = NULL;
RT_EXT_GLFW_PROCEDURES(RT_DEFINE_GLFW_DISPATCH)
#undef RT_DEFINE_GLFW_DISPATCH

static void rtl_clear_glfw_extension(void) {
#define RT_CLEAR_GLFW_PROCEDURE(return_type, name, parameters, arguments) rt_##name = NULL;
	RT_EXT_GLFW_PROCEDURES(RT_CLEAR_GLFW_PROCEDURE)
#undef RT_CLEAR_GLFW_PROCEDURE
}

void rtLoad_RT_EXT_GLFW(void) {
	typedef bool (*rtl_glfw_initialize_proc)(void);
	rtl_glfw_initialize_proc initialize;

	rtl_clear_glfw_extension();
	initialize = (rtl_glfw_initialize_proc)rtl_backend_proc("rtInit_GLFW");
	if (initialize && !initialize()) {
		enum rt_error error = rt_rtError ? rt_rtError() : RT_SUCCESS;
		const char* message = rt_rtErrorMessage ? rt_rtErrorMessage() : NULL;
		if (error == RT_SUCCESS) {
			fprintf(stderr, "rtLoad_RT_EXT_GLFW backend initialization failed without recording an error\n");
		} else {
			fprintf(stderr, "%s\n", message && message[0] ? message : "rtLoad_RT_EXT_GLFW backend initialization failed");
		}
		return;
	}

#define RT_RESOLVE_GLFW_PROCEDURE(return_type, name, parameters, arguments) \
	rt_##name = (PFN_##name)rtGetProc(#name); \
	if (!rt_##name) { \
		fprintf(stderr, "rtLoad_RT_EXT_GLFW missing required procedure: %s\n", #name); \
		rtl_clear_glfw_extension(); \
		return; \
	}
	RT_EXT_GLFW_PROCEDURES(RT_RESOLVE_GLFW_PROCEDURE)
#undef RT_RESOLVE_GLFW_PROCEDURE
}

#if defined(_WIN32) && defined(_MSC_VER)
#pragma comment(linker, "/EXPORT:glfwCreateWindowSurface")
#pragma comment(linker, "/EXPORT:glfwGetFramebufferSize")
#pragma comment(linker, "/EXPORT:glfwGetError")
#pragma comment(linker, "/EXPORT:glfwVulkanSupported")
#pragma comment(linker, "/EXPORT:glfwGetWin32Window")
#endif
