#ifndef RT_EXT_GLFW_H
#define RT_EXT_GLFW_H

/*!
** @file rt_ext_glfw.h
** @brief GLFW window binding for Rutile presentation swapchains.
**
** This extension binds an @ref rt_swapchain to an existing GLFW window.
** Resolve it after loading a Rutile backend. Before creating or binding
** presentation resources, initialize Rutile with @ref RT_FEATURE_PRESENTATION
** and resolve the companion swapchain extension.
*/

#include "rt_ext_swapchain.h"
#include "rutile.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if !defined(RT_TYPES_ONLY)

/*!
** @brief Resolve GLFW presentation procedures through the loaded dispatch chain.
**
** Call after @ref rtLoad has established the backend dispatch chain. This
** loader neither requires @ref rtInit nor a prior
** @ref rtLoad_RT_EXT_SWAPCHAIN call. A failed load clears the GLFW extension
** dispatch pointer; do not call @ref rtSwapchainBindWindowGLFW until a later
** successful load.
**
** Failure is reported through Rutile's thread-local error state. A failed
** load clears the GLFW extension dispatch pointer.
*/
void rtLoad_RT_EXT_GLFW(void);

/*!
** @brief Bind an unbound swapchain to an existing GLFW window.
**
** Call after @ref rtInit has enabled @ref RT_FEATURE_PRESENTATION and both
** presentation extension loaders have succeeded, and before
** @ref rtSwapchainAcquire. Resize the swapchain when the GLFW framebuffer
** extent changes.
**
** @param swapchain Unbound presentation swapchain to associate with @p window.
** @param window    Existing non-NULL GLFW window.
** @note Failures are reported through Rutile's thread-local error state; a
**       validation layer may also record RT_IMPROPER_USAGE.
*/
RT_API void rtSwapchainBindWindowGLFW(rt_swapchain swapchain, struct GLFWwindow* window);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

#endif /* !RT_TYPES_ONLY */

#define RT_EXT_GLFW_PROCEDURES(X) \
	X( void                        , rtSwapchainBindWindowGLFW , ( rt_swapchain swapchain, struct GLFWwindow* window ) , ( swapchain, window                          ) )
/* RT_EXT_GLFW_PROCEDURES */

#if !defined(RT_TYPES_ONLY)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4098)
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
