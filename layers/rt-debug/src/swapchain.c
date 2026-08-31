#include "next.h"
#include "trace.h"

RT_API_PUBLIC void rtSwapchainResize(rt_swapchain swapchain, u32 width, u32 height) {
	rtdbg_trace_api("rtSwapchainResize");
	rtdbg_procs.rtSwapchainResize(swapchain, width, height);
}

RT_API_PUBLIC rt_swapchain_acquire_result rtSwapchainAcquire(rt_swapchain swapchain) {
	rtdbg_trace_api("rtSwapchainAcquire");
	return rtdbg_procs.rtSwapchainAcquire(swapchain);
}

RT_API_PUBLIC void rtSwapchainPresent(rt_swapchain swapchain, rt_timepoint rendered) {
	rtdbg_trace_api("rtSwapchainPresent");
	rtdbg_procs.rtSwapchainPresent(swapchain, rendered);
}

RT_API_PUBLIC void rtSwapchainBindGLFW(rt_swapchain swapchain, struct GLFWwindow * window) {
	rtdbg_trace_api("rtSwapchainBindGLFW");
	rtdbg_procs.rtSwapchainBindGLFW(swapchain, window);
}

RT_API_PUBLIC rt_swapchain rtSwapchainCreate(void) {
	rt_swapchain handle = rtdbg_procs.rtSwapchainCreate();
	rtdbg_trace_resource_create("rtSwapchainCreate", "swapchain", handle);
	return handle;
}

RT_API_PUBLIC void rtSwapchainDestroy(rt_swapchain swapchain) {
	rtdbg_trace_resource_destroy("rtSwapchainDestroy", "swapchain", swapchain);
	rtdbg_procs.rtSwapchainDestroy(swapchain);
}

