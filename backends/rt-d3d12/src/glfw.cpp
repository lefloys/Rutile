#include "glfw.hpp"

#include "error.hpp"

#include <cassert>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

using PFN_rtd3d12_glfwGetFramebufferSize = void (*)(GLFWwindow* window, int* width, int* height);
using PFN_rtd3d12_glfwGetWin32Window = HWND (*)(GLFWwindow* window);

struct rtd3d12_glfw_procs {
	PFN_rtd3d12_glfwGetFramebufferSize get_framebuffer_size = nullptr;
	PFN_rtd3d12_glfwGetWin32Window get_win32_window = nullptr;
	bool resolved = false;
};

static rtd3d12_glfw_procs rtd3d12_glfw;

static HMODULE rtd3d12_glfw_module() {
	HMODULE module = GetModuleHandleA("glfw3.dll");
	return module ? module : GetModuleHandleA(nullptr);
}

static void* rtd3d12_glfw_symbol(HMODULE module, const char* name) {
	return module ? reinterpret_cast<void*>(GetProcAddress(module, name)) : nullptr;
}

static bool rtd3d12_glfw_resolve() {
	HMODULE module;

	if (rtd3d12_glfw.resolved) {
		return rtd3d12_glfw.get_framebuffer_size && rtd3d12_glfw.get_win32_window;
	}

	module = rtd3d12_glfw_module();
	rtd3d12_glfw.get_framebuffer_size = reinterpret_cast<PFN_rtd3d12_glfwGetFramebufferSize>(rtd3d12_glfw_symbol(module, "glfwGetFramebufferSize"));
	rtd3d12_glfw.get_win32_window = reinterpret_cast<PFN_rtd3d12_glfwGetWin32Window>(rtd3d12_glfw_symbol(module, "glfwGetWin32Window"));

	if (!rtd3d12_glfw.get_framebuffer_size || !rtd3d12_glfw.get_win32_window) {
		rtd3d12_fail(rt::error::unsupported_platform, "GLFW symbols are not exported by the executable or available from glfw3.dll");
		return false;
	}

	rtd3d12_glfw.resolved = true;
	return true;
}

RTD3D12_API bool rtInit_GLFW(void) {
	rtd3d12_glfw_resolve();
	return rtError() == rt::error::success;
}

void* rtd3d12_glfw_get_native_window(GLFWwindow* window) {
	assert(rtd3d12_glfw.get_win32_window);
	assert(window);
	return rtd3d12_glfw.get_win32_window(window);
}

void rtd3d12_glfw_get_framebuffer_size(GLFWwindow* window, int* width, int* height) {
	assert(rtd3d12_glfw.get_framebuffer_size);
	assert(window);
	rtd3d12_glfw.get_framebuffer_size(window, width, height);
}
