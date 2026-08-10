#ifndef RT_EXT_GLFW_H
#define RT_EXT_GLFW_H

/*
 * RT_EXT_GLFW extension package.
 */

#include "rt_ext_swapchain.h"
#include "rutile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*PFN_rtInit_RT_EXT_GLFW)(void);
typedef void (*PFN_rtSwapchainBindWindowGLFW)(rt_swapchain swapchain, struct GLFWwindow* window);

extern PFN_rtInit_RT_EXT_GLFW rt_rtInit_RT_EXT_GLFW;
extern PFN_rtSwapchainBindWindowGLFW rt_rtSwapchainBindWindowGLFW;
enum rt_error rtLoad_RT_EXT_GLFW(void);

static inline void rtSwapchainBindWindowGLFW(rt_swapchain swapchain, struct GLFWwindow* window) { rt_rtSwapchainBindWindowGLFW(swapchain, window); }

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* RT_EXT_GLFW_H */
