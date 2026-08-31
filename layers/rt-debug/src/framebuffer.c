#include "next.h"
#include "trace.h"

RT_API_PUBLIC rt_texture_view rtFramebufferColorView(rt_framebuffer framebuffer, rt_location location) {
	rtdbg_trace_api("rtFramebufferColorView");
	return rtdbg_procs.rtFramebufferColorView(framebuffer, location);
}

RT_API_PUBLIC void rtFramebufferSetColorView(rt_framebuffer framebuffer, rt_texture_view view, rt_location location) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtFramebufferSetColorView framebuffer #%llu texture-view #%llu location %p", (unsigned long long)rtdbg_trace_handle_id(framebuffer), (unsigned long long)rtdbg_trace_handle_id(view), (void*)location);
	rtdbg_trace_resource_detail(framebuffer, "color view #%llu at location %p", (unsigned long long)rtdbg_trace_handle_id(view), (void*)location);
	rtdbg_procs.rtFramebufferSetColorView(framebuffer, view, location);
}

RT_API_PUBLIC void rtFramebufferSetDepthView(rt_framebuffer framebuffer, rt_texture_view view) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtFramebufferSetDepthView framebuffer #%llu texture-view #%llu", (unsigned long long)rtdbg_trace_handle_id(framebuffer), (unsigned long long)rtdbg_trace_handle_id(view));
	rtdbg_trace_resource_detail(framebuffer, "depth view #%llu", (unsigned long long)rtdbg_trace_handle_id(view));
	rtdbg_procs.rtFramebufferSetDepthView(framebuffer, view);
}

RT_API_PUBLIC void rtFramebufferSetStencilView(rt_framebuffer framebuffer, rt_texture_view view) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtFramebufferSetStencilView framebuffer #%llu texture-view #%llu", (unsigned long long)rtdbg_trace_handle_id(framebuffer), (unsigned long long)rtdbg_trace_handle_id(view));
	rtdbg_trace_resource_detail(framebuffer, "stencil view #%llu", (unsigned long long)rtdbg_trace_handle_id(view));
	rtdbg_procs.rtFramebufferSetStencilView(framebuffer, view);
}

RT_API_PUBLIC rt_framebuffer rtFramebufferCreate(void) {
	rt_framebuffer handle = rtdbg_procs.rtFramebufferCreate();
	rtdbg_trace_resource_create("rtFramebufferCreate", "framebuffer", handle);
	return handle;
}

RT_API_PUBLIC void rtFramebufferDestroy(rt_framebuffer framebuffer) {
	rtdbg_trace_resource_destroy("rtFramebufferDestroy", "framebuffer", framebuffer);
	rtdbg_procs.rtFramebufferDestroy(framebuffer);
}

