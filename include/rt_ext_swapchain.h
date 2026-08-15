#ifndef RT_EXT_SWAPCHAIN_H
#define RT_EXT_SWAPCHAIN_H

/*!
** @file rt_ext_swapchain.h
** @brief Rutile's presentation-swapchain extension.
**
** This extension supplies the procedures that connect a rendered framebuffer
** to a platform presentation target. Resolve its procedures with
** @ref rtLoad_RT_EXT_SWAPCHAIN after loading a backend. Before creating or
** using presentation resources, initialize Rutile with
** @ref RT_FEATURE_PRESENTATION.
**
** A swapchain owns its presentation images and the framebuffers returned by
** @ref rtSwapchainAcquire. The acquired framebuffer is borrowed: use it for
** the current frame only, then return it to the swapchain with
** @ref rtSwapchainPresent before acquiring again or destroying the swapchain.
*/

typedef struct rt_swapchain_t rt_swapchain_t;
typedef rt_swapchain_t* rt_swapchain;

#include "rutile.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*!
** @brief The presentation target acquired for one frame.
**
** @ref framebuffer is borrowed from the swapchain and is valid until the
** matching @ref rtSwapchainPresent. @ref timepoint signals when rendering may
** begin to use that framebuffer; make the rendering queue wait on it before
** submitting work that targets the framebuffer.
**
** A NULL @ref framebuffer means that no frame is currently available or that
** acquisition failed. After clearing an earlier error, inspect @ref rtError
** to distinguish a recorded failure from a transient no-frame result.
*/
typedef struct rt_swapchain_acquire_result {
	rt_framebuffer framebuffer;
	rt_timepoint timepoint;
} rt_swapchain_acquire_result;

#if !defined(RT_TYPES_ONLY)

/*!
** @brief Resolve presentation procedures through the loaded dispatch chain.
**
** Call after @ref rtLoad has established the backend dispatch chain, and
** before any swapchain wrapper. This loader does not require @ref rtInit. A
** failed call clears every extension dispatch pointer;
** no swapchain wrapper may be called until a later successful load.
**
** Failure is reported through Rutile's thread-local error state. A failed
** load clears every extension dispatch pointer.
*/
void rtLoad_RT_EXT_SWAPCHAIN(void);

/*!
** @brief Create an unbound presentation swapchain.
**
** Call only after @ref rtInit has enabled @ref RT_FEATURE_PRESENTATION. Bind
** the returned swapchain to a native window through a compatible platform
** extension before acquiring images.
**
** @return A new swapchain, or NULL when creation fails.
** @note Failure is reported through Rutile's thread-local error state; a
**       validation layer may also record RT_IMPROPER_USAGE.
*/
RT_API rt_swapchain rtSwapchainCreate(void);

/*!
** @brief Destroy a presentation swapchain.
**
** @param swapchain Swapchain to destroy.
*/
RT_API void rtSwapchainDestroy(rt_swapchain swapchain);

/*!
** @brief Recreate the swapchain images for a new non-zero pixel extent.
**
** Call after the target window changes size and before acquiring the next
** image at that extent. Do not resize while a framebuffer from this swapchain
** is acquired; present that framebuffer first.
**
** @param swapchain Swapchain whose images are recreated.
** @param width     Target width in pixels; must be non-zero.
** @param height    Target height in pixels; must be non-zero.
** @note Failures are reported through Rutile's thread-local error state; a
**       validation layer may also record RT_IMPROPER_USAGE.
*/
RT_API void rtSwapchainResize(rt_swapchain swapchain, u32 width, u32 height);

/*!
** @brief Acquire the next framebuffer for rendering.
**
** Acquire only once per swapchain frame. The returned framebuffer remains
** owned by @p swapchain and must be presented with @ref rtSwapchainPresent
** before another acquisition. Wait for the returned timepoint before
** submitting rendering that writes to the framebuffer.
**
** @param swapchain Swapchain to acquire from.
** @return A borrowed framebuffer and its readiness timepoint. A NULL
**         framebuffer can mean that no frame is currently available or that
**         acquisition failed. After clearing any earlier error, inspect
**         @ref rtError to distinguish a recorded failure from a transient
**         no-frame result.
** @note A validation layer may record RT_IMPROPER_USAGE for invalid call
**       ordering or arguments.
*/
RT_API rt_swapchain_acquire_result rtSwapchainAcquire(rt_swapchain swapchain);

/*!
** @brief Present the framebuffer acquired for the current frame.
**
** @p rendered must identify the work that finished rendering the framebuffer
** returned by the preceding @ref rtSwapchainAcquire. This releases the
** borrowed framebuffer back to @p swapchain; do not use it after this call.
**
** @param swapchain Swapchain from which the current framebuffer was acquired.
** @param rendered  Completion signal for rendering submitted against that
**                  framebuffer.
** @note Failures are reported through Rutile's thread-local error state; a
**       validation layer may also record RT_IMPROPER_USAGE.
*/
RT_API void rtSwapchainPresent(rt_swapchain swapchain, rt_timepoint rendered);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

#endif /* !RT_TYPES_ONLY */

#define RT_EXT_SWAPCHAIN_PROCEDURES(X) \
	X( rt_swapchain                , rtSwapchainCreate  , ( void                                          ) , (                          ) ) \
	X( void                        , rtSwapchainDestroy , ( rt_swapchain swapchain                        ) , ( swapchain                ) ) \
	X( void                        , rtSwapchainResize  , ( rt_swapchain swapchain, u32 width, u32 height ) , ( swapchain, width, height ) ) \
	X( rt_swapchain_acquire_result , rtSwapchainAcquire , ( rt_swapchain swapchain                        ) , ( swapchain                ) ) \
	X( void                        , rtSwapchainPresent , ( rt_swapchain swapchain, rt_timepoint rendered ) , ( swapchain, rendered      ) )
/* RT_EXT_SWAPCHAIN_PROCEDURES */

#if !defined(RT_TYPES_ONLY)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4098)
#endif
RT_EXT_SWAPCHAIN_PROCEDURES(RT_DECLARE_EXTENSION_PROCEDURE)
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif /* !RT_TYPES_ONLY */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RT_EXT_SWAPCHAIN_H */
