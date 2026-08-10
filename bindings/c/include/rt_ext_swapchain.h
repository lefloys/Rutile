#ifndef RT_EXT_SWAPCHAIN_H
#define RT_EXT_SWAPCHAIN_H

/*!
** @file rt_ext_swapchain.h
** @brief Rutile's presentation extension.
**
** Load this extension with @ref rtLoad_RT_EXT_SWAPCHAIN after loading a
** backend and before using any swapchain procedure.
*/

#include "rutile.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct rt_swapchain_t rt_swapchain_t;
typedef rt_swapchain_t* rt_swapchain;

typedef struct rt_swapchain_acquire_result {
	rt_framebuffer framebuffer;
	rt_timepoint timepoint;
} rt_swapchain_acquire_result;

/*!
** @brief Resolve the presentation procedures exposed by the loaded backend.
** @return RT_SUCCESS, or RT_EXTENSION_NOT_PRESENT when the backend does not
**         implement presentation.
*/
enum rt_error rtLoad_RT_EXT_SWAPCHAIN(void);

/*! @brief Create an unbound presentation swapchain. */
static inline rt_swapchain rtSwapchainCreate(void);
/*! @brief Destroy a swapchain and its presentation resources. */
static inline void rtSwapchainDestroy(rt_swapchain swapchain);
/*! @brief Resize swapchain images to the requested pixel extent. */
static inline void rtSwapchainResize(rt_swapchain swapchain, u32 width, u32 height);
/*! @brief Acquire the next presentation image and its framebuffer. */
static inline rt_swapchain_acquire_result rtSwapchainAcquire(rt_swapchain swapchain);
/*! @brief Present work completed by @p rendered. */
static inline void rtSwapchainPresent(rt_swapchain swapchain, rt_timepoint rendered);

/* One inventory drives the dispatch types and shared loader storage. */
#define RT_SWAPCHAIN_PROCEDURES(X) \
	X(rt_swapchain,                rtSwapchainCreate,  (void)) \
	X(void,                        rtSwapchainDestroy, (rt_swapchain swapchain)) \
	X(void,                        rtSwapchainResize,  (rt_swapchain swapchain, u32 width, u32 height)) \
	X(rt_swapchain_acquire_result, rtSwapchainAcquire, (rt_swapchain swapchain)) \
	X(void,                        rtSwapchainPresent, (rt_swapchain swapchain, rt_timepoint rendered))

#define RT_DECLARE_SWAPCHAIN_PROCEDURE(return_type, name, parameters) \
	typedef return_type (*PFN_##name) parameters; \
	extern PFN_##name rt_##name;
RT_SWAPCHAIN_PROCEDURES(RT_DECLARE_SWAPCHAIN_PROCEDURE)
#undef RT_DECLARE_SWAPCHAIN_PROCEDURE

static inline rt_swapchain rtSwapchainCreate(void) { return rt_rtSwapchainCreate(); }
static inline void rtSwapchainDestroy(rt_swapchain swapchain) { rt_rtSwapchainDestroy(swapchain); }
static inline void rtSwapchainResize(rt_swapchain swapchain, u32 width, u32 height) { rt_rtSwapchainResize(swapchain, width, height); }
static inline rt_swapchain_acquire_result rtSwapchainAcquire(rt_swapchain swapchain) { return rt_rtSwapchainAcquire(swapchain); }
static inline void rtSwapchainPresent(rt_swapchain swapchain, rt_timepoint rendered) { rt_rtSwapchainPresent(swapchain, rendered); }

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RT_EXT_SWAPCHAIN_H */
