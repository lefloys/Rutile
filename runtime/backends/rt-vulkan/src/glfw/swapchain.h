#ifndef RTVK_GLFW_SWAPCHAIN_H
#define RTVK_GLFW_SWAPCHAIN_H

#include "config.h"
#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

typedef struct GLFWwindow GLFWwindow;

RTVK_API void rtSwapchainBindWindowGLFW(rt_swapchain swapchain, GLFWwindow* window);

struct rtvk_context;
struct rtvk_swapchain;

void rtvk_swapchain_bind_window_glfw(struct rtvk_context* ctx, struct rtvk_swapchain* swapchain, GLFWwindow* window);

#endif /* RTVK_GLFW_SWAPCHAIN_H */
