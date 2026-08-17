#pragma once

#include "config.hpp"

#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

struct GLFWwindow;

RTDX_API bool rtInit_GLFW(void);
RTDX_API void rtSwapchainBindWindowGLFW(rt_swapchain swapchain, GLFWwindow* window);
