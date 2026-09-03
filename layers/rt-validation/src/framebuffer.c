#include "framebuffer.h"
#include "logger.h"
#include "program.h"
#include "texture_view.h"

#define RTVAL_DROP(message) rtval_fail(message)

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RT_API_PUBLIC rt_framebuffer rtFramebufferCreate(void) {
	return rtval_framebuffer_to_handle(rtval_framebuffer_create());
}

RT_API_PUBLIC void rtFramebufferDestroy(rt_framebuffer framebuffer) {
	rtval_framebuffer_destroy(rtval_framebuffer_from_handle(framebuffer));
}

RT_API_PUBLIC rt_texture_view rtFramebufferColorView(rt_framebuffer framebuffer, rt_location location) {
	return rtval_framebuffer_color_view(rtval_framebuffer_from_handle(framebuffer), location);
}

RT_API_PUBLIC void rtFramebufferSetColorView(rt_framebuffer framebuffer, rt_texture_view view, rt_location location) {
	rtval_framebuffer_set_color_view(
		rtval_framebuffer_from_handle(framebuffer),
		rtval_texture_view_from_handle(view),
		location
	);
}

RT_API_PUBLIC void rtFramebufferSetDepthView(rt_framebuffer framebuffer, rt_texture_view view) {
	rtval_framebuffer_set_depth_view(
		rtval_framebuffer_from_handle(framebuffer),
		rtval_texture_view_from_handle(view)
	);
}

RT_API_PUBLIC void rtFramebufferSetStencilView(rt_framebuffer framebuffer, rt_texture_view view) {
	rtval_framebuffer_set_stencil_view(
		rtval_framebuffer_from_handle(framebuffer),
		rtval_texture_view_from_handle(view)
	);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtval_framebuffer* rtval_framebuffer_create(void) {
	rt_framebuffer backend = rtval_next_rtFramebufferCreate();
	if (!backend) {
		rtval_report_error("rtFramebufferCreate");
		return NULL;
	}
	struct rtval_framebuffer* handle = rtval_handle_create(RTVAL_HANDLE_TYPE_FRAMEBUFFER);
	if (!handle) {
		rtval_next_rtFramebufferDestroy(backend);
		return NULL;
	}
	struct rtval_framebuffer* state = RTVAL_PAYLOAD(handle, struct rtval_framebuffer);
	state->backend = backend;
	state->owns_backend = true;
	rtval_report_error("rtFramebufferCreate");
	return handle;
}

struct rtval_framebuffer* rtval_framebuffer_wrap(rt_framebuffer backend) {
	if (!backend) {
		return NULL;
	}
	struct rtval_framebuffer* handle = rtval_handle_create(RTVAL_HANDLE_TYPE_FRAMEBUFFER);
	if (!handle) {
		return NULL;
	}
	struct rtval_framebuffer* state = RTVAL_PAYLOAD(handle, struct rtval_framebuffer);
	state->backend = backend;
	state->owns_backend = false;
	rtval_report_error("rtFramebufferWrap");
	return handle;
}

void rtval_framebuffer_destroy(struct rtval_framebuffer* framebuffer) {
	if (!framebuffer) {
		RTVAL_DROP("rtFramebufferDestroy: NULL handle");
		return;
	}
	struct rtval_framebuffer* framebuffer_state = RTVAL_PAYLOAD(framebuffer, struct rtval_framebuffer);
	if (!framebuffer_state) {
		RTVAL_DROP("rtFramebufferDestroy: invalid handle");
		return;
	}
	if (framebuffer_state->owns_backend) {
		rtval_next_rtFramebufferDestroy(framebuffer_state->backend);
		if (!rtval_report_error("rtFramebufferDestroy")) {
			return;
		}
	}
	rtval_handle_destroy(framebuffer);
}

rt_texture_view rtval_framebuffer_color_view(struct rtval_framebuffer* framebuffer, rt_location location) {
	if (!framebuffer) {
		RTVAL_DROP("rtFramebufferColorView: NULL handle");
		return RT_NULL_HANDLE;
	}
	struct rtval_framebuffer* framebuffer_state = RTVAL_PAYLOAD(framebuffer, struct rtval_framebuffer);
	if (!framebuffer_state) {
		RTVAL_DROP("rtFramebufferColorView: invalid handle");
		return RT_NULL_HANDLE;
	}
	rt_location backend_location = RT_NULL_HANDLE;
	if (!rtval_location_unwrap(location, RTVAL_LOCATION_OUTPUT, true, &backend_location, "rtFramebufferColorView: live output location required")) {
		return RT_NULL_HANDLE;
	}
	rt_texture_view result = rtval_next_rtFramebufferColorView(framebuffer_state->backend, backend_location);
	rtval_report_error("rtFramebufferColorView");
	if (!result) {
		return RT_NULL_HANDLE;
	}
	rt_texture_view view = rtval_handle_find_by_backend(RTVAL_HANDLE_TYPE_TEXTURE_VIEW, result);
	if (!view) {
		RTVAL_DROP("rtFramebufferColorView: attachment is not a validation texture view");
	}
	return view;
}

void rtval_framebuffer_set_color_view(struct rtval_framebuffer* framebuffer, struct rtval_texture_view* view, rt_location location) {
	if (!framebuffer) {
		RTVAL_DROP("rtFramebufferSetColorView: NULL handle");
		return;
	}
	struct rtval_framebuffer* framebuffer_state = RTVAL_PAYLOAD(framebuffer, struct rtval_framebuffer);
	if (!framebuffer_state) {
		RTVAL_DROP("rtFramebufferSetColorView: invalid handle");
		return;
	}
	rt_texture_view view_backend = RT_NULL_HANDLE;
	if (view) {
		struct rtval_texture_view* view_state = RTVAL_PAYLOAD(view, struct rtval_texture_view);
		if (!view_state) {
			RTVAL_DROP("rtFramebufferSetColorView: invalid view handle");
			return;
		}
		view_backend = view_state->backend;
	}
	rt_location backend_location = RT_NULL_HANDLE;
	if (!rtval_location_unwrap(location, RTVAL_LOCATION_OUTPUT, true, &backend_location, "rtFramebufferSetColorView: live output location required")) {
		return;
	}
	rtval_next_rtFramebufferSetColorView(framebuffer_state->backend, view_backend, backend_location);
	rtval_report_error("rtFramebufferSetColorView");
}

void rtval_framebuffer_set_depth_view(struct rtval_framebuffer* framebuffer, struct rtval_texture_view* view) {
	if (!framebuffer) {
		RTVAL_DROP("rtFramebufferSetDepthView: NULL handle");
		return;
	}
	struct rtval_framebuffer* framebuffer_state = RTVAL_PAYLOAD(framebuffer, struct rtval_framebuffer);
	if (!framebuffer_state) {
		RTVAL_DROP("rtFramebufferSetDepthView: invalid handle");
		return;
	}
	rt_texture_view view_backend = RT_NULL_HANDLE;
	if (view) {
		struct rtval_texture_view* view_state = RTVAL_PAYLOAD(view, struct rtval_texture_view);
		if (!view_state) {
			RTVAL_DROP("rtFramebufferSetDepthView: invalid view handle");
			return;
		}
		view_backend = view_state->backend;
	}
	rtval_next_rtFramebufferSetDepthView(framebuffer_state->backend, view_backend);
	rtval_report_error("rtFramebufferSetDepthView");
}

void rtval_framebuffer_set_stencil_view(struct rtval_framebuffer* framebuffer, struct rtval_texture_view* view) {
	struct rtval_framebuffer* framebuffer_state = RTVAL_PAYLOAD(framebuffer, struct rtval_framebuffer);
	if (!framebuffer_state) {
		RTVAL_DROP("rtFramebufferSetStencilView: invalid handle");
		return;
	}
	rt_texture_view view_backend = RT_NULL_HANDLE;
	if (view) {
		struct rtval_texture_view* view_state = RTVAL_PAYLOAD(view, struct rtval_texture_view);
		if (!view_state) {
			RTVAL_DROP("rtFramebufferSetStencilView: invalid view handle");
			return;
		}
		view_backend = view_state->backend;
	}
	rtval_next_rtFramebufferSetStencilView(framebuffer_state->backend, view_backend);
	rtval_report_error("rtFramebufferSetStencilView");
}

#undef RTVAL_DROP
