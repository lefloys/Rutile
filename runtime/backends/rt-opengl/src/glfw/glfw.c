#include "glfw/glfw.h"

#include "error.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// @GPT FIXED: why are you defining the procs multiple times
typedef struct rtgl_glfw_procs {
	HWND (*glfwGetWin32Window)(GLFWwindow* window);
	void (*glfwGetFramebufferSize)(GLFWwindow* window, int* width, int* height);
} rtgl_glfw_procs;
// @GPT FIXED: what?
static rtgl_glfw_procs glfw_procs;
// @GPT FIXED: what is this
struct gl_surface* rtgl_create_wgl_surface(struct gl_context* context, HWND window);

static HMODULE rtgl_glfw_module(void) {
	HMODULE module = GetModuleHandleA("glfw3.dll");
	return module ? module : GetModuleHandleA(NULL);
}
// @GPT FIXED:what the fuck but you have a macro ??
static void* rtgl_glfw_load_proc(HMODULE module, const char* name) {
	return module ? (void*)GetProcAddress(module, name) : NULL;
}

#define RTGL_GLFW_LOAD_PROC(cast, name)                                                             \
	do {                                                                                            \
		glfw_procs.name = cast rtgl_glfw_load_proc(module, #name);                                  \
		if (!glfw_procs.name) {                                                                     \
			rtgl_throwf(RT_UNSUPPORTED_PLATFORM, "%s not found in executable or loaded glfw3.dll", #name); \
			return;                                                                                 \
		}                                                                                           \
	} while (0)

void rtgl_init_glfw_platform(void) {
	HMODULE module;
	// @GPT FIXED: why are you checking if EVERYTHING exists ?? just add a flag loaded in this case
	if (glfw_procs.glfwGetWin32Window && glfw_procs.glfwGetFramebufferSize) {
		return;
	}
	// @GPT FIXED: why the fuck do you need to redefine the functions
	module = rtgl_glfw_module();
	RTGL_GLFW_LOAD_PROC((HWND (*)(GLFWwindow*)), glfwGetWin32Window);
	RTGL_GLFW_LOAD_PROC((void (*)(GLFWwindow*, int*, int*)), glfwGetFramebufferSize);
}

#undef RTGL_GLFW_LOAD_PROC

HWND glfwGetWin32Window(GLFWwindow* window) {
	// @GPT FIXED: why assert they are loaded. and like you are allowed to pass fucking NULL to glfwGetWin32Window
	if (!glfw_procs.glfwGetWin32Window || !window) {
		return NULL;
	}
	return glfw_procs.glfwGetWin32Window(window);
}

struct gl_surface* rtgl_create_glfw_surface(struct gl_context* context, GLFWwindow* window) {
	// @GPT FIXED: why is this abstracted to the level where you can do it from a glfwwindow. why not just call that one function outside...
	return rtgl_create_wgl_surface(context, glfwGetWin32Window(window));
}

void glfwGetFramebufferSize(GLFWwindow* window, int* width, int* height) {
	// @GPT FIXED: why the asserts
	if (!glfw_procs.glfwGetFramebufferSize || !window || !width || !height) {
		return;
	}
	glfw_procs.glfwGetFramebufferSize(window, width, height);
}
