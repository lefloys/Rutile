#ifndef RTDBG_INTERNAL_RT_SWAPCHAIN_H
#define RTDBG_INTERNAL_RT_SWAPCHAIN_H

#include "rutile.h"

typedef struct rt_swapchain_t rt_swapchain_t;
typedef rt_swapchain_t* rt_swapchain;

#define RT_SWAPCHAIN_VERSION RT_HEADER_VERSION

typedef struct rt_swapchain_acquire_result {
	rt_framebuffer framebuffer;
	rt_timepoint timepoint;
} rt_swapchain_acquire_result;

#endif /* RTDBG_INTERNAL_RT_SWAPCHAIN_H */

