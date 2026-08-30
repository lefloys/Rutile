#ifndef RT_EXT_GLFW_H
#define RT_EXT_GLFW_H

/*!
** @file rt_ext_glfw.h
** @brief GLFW binding for Rutile presentation swapchains.
**
** Load this extension and the swapchain extension before binding a GLFW window
** to a swapchain.
*/

#include "rt_ext_swapchain.h"
#include "rutile.h"

/*! @brief Version of the extension contract declared by this header. */
#define RT_EXT_GLFW_VERSION RT_HEADER_VERSION

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if !defined(RT_TYPES_ONLY)

/*!
** @brief Make the GLFW binding procedure callable.
**
** Call after @ref rtLoad and before @ref rtSwapchainBindGLFW. Failure is
** reported through @ref rtError; the binding procedure remains unavailable.
*/
void rtLoad_RT_EXT_GLFW(void);

/*!
** @brief Bind an unbound swapchain to an existing GLFW window.
**
** @ref RT_FEATURE_PRESENTATION must be enabled and both presentation
** extensions must be loaded. The swapchain must be unbound and have no
** acquired frame. Resize it whenever the window framebuffer extent changes.
**
** @param swapchain  Unbound swapchain.
** @param window     Non-NULL GLFW window.
*/
RT_API void rtSwapchainBindGLFW(rt_swapchain swapchain, struct GLFWwindow* window);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

#endif /* !RT_TYPES_ONLY */

#define RT_EXT_GLFW_PROCEDURES(X) \
	X(void, rtSwapchainBindGLFW, (rt_swapchain swapchain, struct GLFWwindow * window), (swapchain, window))
/* RT_EXT_GLFW_PROCEDURES */

#if !defined(RT_TYPES_ONLY)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4098)
#endif
RT_EXT_GLFW_PROCEDURES(RT_DECLARE_EXTENSION_PROCEDURE)
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif /* !RT_TYPES_ONLY */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RT_EXT_GLFW_H */
