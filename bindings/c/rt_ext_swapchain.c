#include "rt_ext_swapchain.h"

PFN_rtSwapchainCreate rt_rtSwapchainCreate = NULL;
PFN_rtSwapchainDestroy rt_rtSwapchainDestroy = NULL;
PFN_rtSwapchainResize rt_rtSwapchainResize = NULL;
PFN_rtSwapchainAcquire rt_rtSwapchainAcquire = NULL;
PFN_rtSwapchainPresent rt_rtSwapchainPresent = NULL;

#define RT_SWAPCHAIN_PROCEDURES(X) \
	X(rtSwapchainCreate) \
	X(rtSwapchainDestroy) \
	X(rtSwapchainResize) \
	X(rtSwapchainAcquire) \
	X(rtSwapchainPresent)

static void rt_clear_swapchain_extension(void) {
#define RT_CLEAR_SWAPCHAIN_PROCEDURE(name) rt_##name = NULL;
	RT_SWAPCHAIN_PROCEDURES(RT_CLEAR_SWAPCHAIN_PROCEDURE)
#undef RT_CLEAR_SWAPCHAIN_PROCEDURE
}

enum rt_error rtLoad_RT_EXT_SWAPCHAIN(void) {
	rt_clear_swapchain_extension();
#define RT_RESOLVE_SWAPCHAIN_PROCEDURE(name) \
	do { \
		rt_proc_t proc = rtGetProc(#name); \
		if (!proc) { rt_clear_swapchain_extension(); return RT_EXTENSION_NOT_PRESENT; } \
		rt_##name = (PFN_##name)proc; \
	} while (0);
	RT_SWAPCHAIN_PROCEDURES(RT_RESOLVE_SWAPCHAIN_PROCEDURE)
#undef RT_RESOLVE_SWAPCHAIN_PROCEDURE
	return RT_SUCCESS;
}
