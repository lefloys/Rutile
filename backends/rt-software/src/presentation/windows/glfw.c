#include "glfw.h"

#include "error.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

typedef void (*rtsw_glfw_get_framebuffer_size_proc)(struct GLFWwindow* window, int* width, int* height);
typedef void* (*rtsw_glfw_get_win32_window_proc)(struct GLFWwindow* window);

struct rtsw_glfw_procedures {
	bool resolved;
	rtsw_glfw_get_framebuffer_size_proc get_framebuffer_size;
	rtsw_glfw_get_win32_window_proc get_win32_window;
};

static struct rtsw_glfw_procedures rtsw_glfw;

static void* rtsw_glfw_find_procedure(const char* name) {
	HMODULE executable = GetModuleHandleA(NULL);
	FARPROC procedure = executable ? GetProcAddress(executable, name) : NULL;

	if (!procedure) {
		HMODULE library = GetModuleHandleA("glfw3.dll");
		procedure = library ? GetProcAddress(library, name) : NULL;
	}
	return (void*)procedure;
}

bool rtsw_glfw_initialize(void) {
	if (!rtsw_glfw.resolved) {
		rtsw_glfw.resolved = true;
		rtsw_glfw.get_framebuffer_size = (rtsw_glfw_get_framebuffer_size_proc)rtsw_glfw_find_procedure("glfwGetFramebufferSize");
		rtsw_glfw.get_win32_window = (rtsw_glfw_get_win32_window_proc)rtsw_glfw_find_procedure("glfwGetWin32Window");
	}

	if (!rtsw_glfw.get_framebuffer_size || !rtsw_glfw.get_win32_window) {
		rtsw_throwf(
			RT_UNSUPPORTED_PLATFORM,
			"rt-software could not locate glfwGetFramebufferSize and glfwGetWin32Window in the host process"
		);
		return false;
	}
	return true;
}

bool rtInit_GLFW(void) {
	rtsw_clear_error();
	return rtsw_glfw_initialize();
}

bool rtsw_glfw_framebuffer_size(struct GLFWwindow* window, int* width, int* height) {
	if (!window || !width || !height || !rtsw_glfw_initialize()) {
		return false;
	}
	rtsw_glfw.get_framebuffer_size(window, width, height);
	return true;
}

void* rtsw_glfw_win32_window(struct GLFWwindow* window) {
	if (!window || !rtsw_glfw_initialize()) {
		return NULL;
	}
	return rtsw_glfw.get_win32_window(window);
}
