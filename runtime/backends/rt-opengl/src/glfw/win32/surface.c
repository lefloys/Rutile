#include "glfw/glfw.h"

#include "error.h"

#include <assert.h>

struct gl_surface* rtgl_create_wgl_surface(struct gl_context* context, HWND window);

struct gl_surface* rtgl_create_glfw_surface(struct gl_context* context, GLFWwindow* window) {
	HWND hwnd;

	assert(context);
	assert(window);

	hwnd = glfwGetWin32Window(window);
	if (!hwnd) {
		rtgl_throwf(RT_PLATFORM_FAILURE, "glfwGetWin32Window returned NULL");
		return NULL;
	}

	return rtgl_create_wgl_surface(context, hwnd);
}
