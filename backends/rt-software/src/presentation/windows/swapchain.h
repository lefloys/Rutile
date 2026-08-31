#ifndef RTSW_WINDOWS_SWAPCHAIN_H
#define RTSW_WINDOWS_SWAPCHAIN_H

#include "resource/framebuffer.h"

struct GLFWwindow;

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtsw_swapchain {
	struct rtsw_resource_base base;
	void* window;
	struct rtsw_texture* color_texture;
	struct rtsw_texture_view* color_view;
	struct rtsw_framebuffer* framebuffer;
	void* presentation_dc;
	void* presentation_bitmap;
	void* presentation_old_bitmap;
	u08* presentation_pixels;
	void* previous_window_procedure;
	u32 width;
	u32 height;
	bool frame_acquired;
};

RTSW_API rt_swapchain rtSwapchainCreate(void);
RTSW_API void rtSwapchainDestroy(rt_swapchain swapchain);
RTSW_API void rtSwapchainResize(rt_swapchain swapchain, u32 width, u32 height);
RTSW_API rt_swapchain_acquire_result rtSwapchainAcquire(rt_swapchain swapchain);
RTSW_API void rtSwapchainPresent(rt_swapchain swapchain, rt_timepoint rendered);
RTSW_API void rtSwapchainBindGLFW(rt_swapchain swapchain, struct GLFWwindow* window);

RTSW_DECLARE_HANDLE(swapchain, rtsw_swapchain);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

#endif /* RTSW_WINDOWS_SWAPCHAIN_H */
