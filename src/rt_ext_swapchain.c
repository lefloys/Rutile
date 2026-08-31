#include "rt_swapchain.h"

#include <stdio.h>

#define RT_DEFINE_SWAPCHAIN_DISPATCH(return_type, name, parameters, arguments) \
	PFN_##name rt_##name = NULL;
RT_SWAPCHAIN_PROCEDURES(RT_DEFINE_SWAPCHAIN_DISPATCH)
#undef RT_DEFINE_SWAPCHAIN_DISPATCH

static void rtl_clear_swapchain_extension(void) {
#define RT_CLEAR_SWAPCHAIN_PROCEDURE(return_type, name, parameters, arguments) rt_##name = NULL;
	RT_SWAPCHAIN_PROCEDURES(RT_CLEAR_SWAPCHAIN_PROCEDURE)
#undef RT_CLEAR_SWAPCHAIN_PROCEDURE
}

void rtLoadSwapchain(void) {
	rtl_clear_swapchain_extension();
#define RT_RESOLVE_SWAPCHAIN_PROCEDURE(return_type, name, parameters, arguments) \
	do { \
		rt_##name = (PFN_##name)rtGetProc(#name); \
		if (!rt_##name) { \
			fprintf(stderr, "rtLoadSwapchain missing required procedure: %s\n", #name); \
			rtl_clear_swapchain_extension(); \
			return; \
		} \
	} while (0);
	RT_SWAPCHAIN_PROCEDURES(RT_RESOLVE_SWAPCHAIN_PROCEDURE)
#undef RT_RESOLVE_SWAPCHAIN_PROCEDURE
}
