#include "framebuffer.h"

#include "context.h"
#include "error.h"

#include <stdlib.h>

static void rtsw_framebuffer_release_view(struct rtsw_texture_view** destination) {
	if (*destination) {
		rtsw_resource_release(&(*destination)->base);
		*destination = NULL;
	}
}

static void rtsw_framebuffer_finish(struct rtsw_framebuffer* framebuffer) {
	for (u32 i = 0; i < RTSW_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS; ++i) {
		rtsw_framebuffer_release_view(&framebuffer->color_views[i]);
	}
	rtsw_framebuffer_release_view(&framebuffer->depth_view);
	rtsw_framebuffer_release_view(&framebuffer->stencil_view);
}

static void rtsw_framebuffer_finalize_resource(void* value) {
	struct rtsw_framebuffer* framebuffer = value;
	rtsw_framebuffer_finish(framebuffer);
	free(framebuffer);
}

RTSW_DEFINE_HANDLE(framebuffer, rtsw_framebuffer)

rt_framebuffer rtFramebufferCreate(void) {
	struct rtsw_context* ctx = rtsw_get_current_context();
	struct rtsw_framebuffer* framebuffer;
	rtsw_clear_error();
	if (!ctx) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtFramebufferCreate called before rtInit");
		return RT_NULL_HANDLE;
	}
	framebuffer = RTSW_ALLOC_RESOURCE(struct rtsw_framebuffer);
	if (!framebuffer) return RT_NULL_HANDLE;
	rtsw_init_resource_base(ctx, &framebuffer->base, framebuffer, rtsw_framebuffer_finalize_resource);
	return rtsw_framebuffer_to_handle(framebuffer);
}

void rtFramebufferDestroy(rt_framebuffer handle) {
	struct rtsw_framebuffer* framebuffer = rtsw_framebuffer_from_handle(handle);
	if (framebuffer) rtsw_resource_retire(&framebuffer->base);
}

rt_texture_view rtFramebufferColorView(rt_framebuffer handle, rt_location location) {
	struct rtsw_framebuffer* framebuffer = rtsw_framebuffer_from_handle(handle);
	(void)location;
	return framebuffer ? rtsw_texture_view_to_handle(framebuffer->color_views[0]) : RT_NULL_HANDLE;
}

void rtsw_framebuffer_set_color_view(struct rtsw_framebuffer* framebuffer, u32 slot, struct rtsw_texture_view* view) {
	if (slot >= RTSW_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS) return;
	if (view) rtsw_resource_retain(&view->base);
	rtsw_framebuffer_release_view(&framebuffer->color_views[slot]);
	framebuffer->color_views[slot] = view;
	if (view && framebuffer->color_texture_count <= slot) framebuffer->color_texture_count = slot + 1;
}

void rtsw_framebuffer_set_depth_view(struct rtsw_framebuffer* framebuffer, struct rtsw_texture_view* view) {
	if (view) rtsw_resource_retain(&view->base);
	rtsw_framebuffer_release_view(&framebuffer->depth_view);
	framebuffer->depth_view = view;
}

void rtsw_framebuffer_set_stencil_view(struct rtsw_framebuffer* framebuffer, struct rtsw_texture_view* view) {
	if (view) rtsw_resource_retain(&view->base);
	rtsw_framebuffer_release_view(&framebuffer->stencil_view);
	framebuffer->stencil_view = view;
}

void rtFramebufferSetColorView(rt_framebuffer handle, rt_texture_view view_handle, rt_location location) {
	struct rtsw_framebuffer* framebuffer = rtsw_framebuffer_from_handle(handle);
	rtsw_clear_error();
	(void)location;
	if (!framebuffer) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtFramebufferSetColorView received a NULL framebuffer");
		return;
	}
	rtsw_framebuffer_set_color_view(framebuffer, 0, rtsw_texture_view_from_handle(view_handle));
}

void rtFramebufferSetDepthView(rt_framebuffer handle, rt_texture_view view_handle) {
	struct rtsw_framebuffer* framebuffer = rtsw_framebuffer_from_handle(handle);
	rtsw_clear_error();
	if (!framebuffer) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtFramebufferSetDepthView received a NULL framebuffer");
		return;
	}
	rtsw_framebuffer_set_depth_view(framebuffer, rtsw_texture_view_from_handle(view_handle));
}

void rtFramebufferSetStencilView(rt_framebuffer handle, rt_texture_view view_handle) {
	struct rtsw_framebuffer* framebuffer = rtsw_framebuffer_from_handle(handle);
	rtsw_clear_error();
	if (!framebuffer) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtFramebufferSetStencilView received a NULL framebuffer");
		return;
	}
	rtsw_framebuffer_set_stencil_view(framebuffer, rtsw_texture_view_from_handle(view_handle));
}
