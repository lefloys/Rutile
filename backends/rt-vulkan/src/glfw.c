#include "glfw.h"
#include "context.h"
#include "error.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <assert.h>

/*
 * rt-vulkan deliberately does NOT link GLFW. The host application brings its
 * own GLFW (statically linked into the executable, or loaded as glfw3.dll),
 * and we resolve the GLFW entry points we need from whatever module the host
 * already has in its address space. This avoids the "two static GLFWs in the
 * same process" trap where each module has its own uninitialised copy of
 * GLFW's global state.
 */

typedef struct GLFWwindow GLFWwindow;

typedef VkResult (*PFN_glfwCreateWindowSurface)(VkInstance, GLFWwindow*, const VkAllocationCallbacks*, VkSurfaceKHR*);
typedef void (*PFN_glfwGetFramebufferSize)(GLFWwindow*, int*, int*);
typedef int (*PFN_glfwGetError)(const char**);
typedef int (*PFN_glfwVulkanSupported)(void);

struct rtvk_glfw_procs {
	int resolved;
	int ok;
	const char* failure_reason;
	PFN_glfwCreateWindowSurface create_window_surface;
	PFN_glfwGetFramebufferSize get_framebuffer_size;
	PFN_glfwGetError get_error;
	PFN_glfwVulkanSupported vulkan_supported;
};

static struct rtvk_glfw_procs g_glfw;

#if defined(_WIN32)
static void* rtvk_glfw_find_proc(const char* name) {
	HMODULE executable = GetModuleHandleA(NULL);
	if (executable) {
		FARPROC proc = GetProcAddress(executable, name);
		if (proc) {
			return (void*)proc;
		}
	}
	HMODULE glfw_dll = GetModuleHandleA("glfw3.dll");
	if (glfw_dll) {
		return (void*)GetProcAddress(glfw_dll, name);
	}
	return NULL;
}
#else
static void* rtvk_glfw_find_proc(const char* name) {
	/* RTLD_DEFAULT searches every library already loaded by the process. */
	return dlsym(RTLD_DEFAULT, name);
}
#endif

static void rtvk_glfw_resolve(void) {
	if (g_glfw.resolved) {
		return;
	}
	g_glfw.resolved = 1;

	g_glfw.create_window_surface = (PFN_glfwCreateWindowSurface)rtvk_glfw_find_proc("glfwCreateWindowSurface");
	g_glfw.get_framebuffer_size = (PFN_glfwGetFramebufferSize)rtvk_glfw_find_proc("glfwGetFramebufferSize");
	g_glfw.get_error = (PFN_glfwGetError)rtvk_glfw_find_proc("glfwGetError");
	g_glfw.vulkan_supported = (PFN_glfwVulkanSupported)rtvk_glfw_find_proc("glfwVulkanSupported");

	if (!g_glfw.create_window_surface) {
		g_glfw.failure_reason = "glfwCreateWindowSurface not found in any loaded module - is GLFW linked into the host executable?";
		return;
	}
	if (!g_glfw.get_framebuffer_size) {
		g_glfw.failure_reason = "glfwGetFramebufferSize not found in any loaded module";
		return;
	}
	g_glfw.ok = 1;
}

void rtvk_init_glfw_platform(void) {
	rtvk_glfw_resolve();
	if (!g_glfw.ok) {
		rtvk_throwf(
			RT_UNSUPPORTED_PLATFORM,
			"rt-vulkan could not locate GLFW in this process: %s",
			g_glfw.failure_reason ? g_glfw.failure_reason : "unknown reason"
		);
	}
}

bool rtInit_GLFW(void) {
	rtvk_init_glfw_platform();
	return rtvk_error() == RT_SUCCESS;
}

VkSurfaceKHR rtvk_create_glfw_surface(struct rtvk_context* ctx, GLFWwindow* window) {
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkResult result = g_glfw.create_window_surface(ctx->vk_instance, window, VK_ALLOCATOR, &surface);
	if (result != VK_SUCCESS) {
		const char* glfw_msg = NULL;
		int glfw_code = 0;
		if (g_glfw.get_error) {
			glfw_code = g_glfw.get_error(&glfw_msg);
		}
		rtvk_throwf(
			rtvk_error_from_vk(result),
			"glfwCreateWindowSurface failed: %s (GLFW %d: %s)",
			rtvk_vk_result_name(result),
			glfw_code,
			glfw_msg ? glfw_msg : "no GLFW error message"
		);
		return VK_NULL_HANDLE;
	}
	return surface;
}

void rtvk_glfw_get_framebuffer_size(GLFWwindow* window, int* width, int* height) {
	assert(g_glfw.get_framebuffer_size);
	assert(window);
	g_glfw.get_framebuffer_size(window, width, height);
}

void rtvk_glfw_get_error(int* code, const char** message) {
	assert(code);
	assert(message);
	*code = 0;
	*message = NULL;
	if (g_glfw.get_error) {
		*code = g_glfw.get_error(message);
	}
}
