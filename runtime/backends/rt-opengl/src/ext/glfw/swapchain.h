#ifndef RTGL_EXT_GLFW_SWAPCHAIN_H
#define RTGL_EXT_GLFW_SWAPCHAIN_H

#include "config.h"
#include "glfw/glfw.h"
#include "resource/swapchain.h"
#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

RTGL_EXTERN_C_ENTER

RTGL_API bool rtInit_GLFW(void);
RTGL_API void rtSwapchainBindWindowGLFW(rt_swapchain swapchain, GLFWwindow* window);

RTGL_EXTERN_C_EXIT

void rtgl_swapchain_bind_window_glfw(struct rtgl_context* ctx, struct rtgl_swapchain* swapchain, GLFWwindow* window, u32 width, u32 height);

#endif /* RTGL_EXT_GLFW_SWAPCHAIN_H */
