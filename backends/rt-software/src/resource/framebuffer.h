#ifndef RTSW_FRAMEBUFFER_H
#define RTSW_FRAMEBUFFER_H

#include "texture.h"

struct rtsw_framebuffer {
	struct rtsw_resource_base base;
	struct rtsw_texture_view* color_views[RTSW_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS];
	struct rtsw_texture_view* depth_view;
	struct rtsw_texture_view* stencil_view;
	u32 color_texture_count;
};

RTSW_API rt_framebuffer rtFramebufferCreate(void);
RTSW_API void rtFramebufferDestroy(rt_framebuffer framebuffer);
RTSW_API rt_texture_view rtFramebufferColorView(rt_framebuffer framebuffer, rt_location location);
RTSW_API void rtFramebufferSetColorView(rt_framebuffer framebuffer, rt_texture_view view, rt_location location);
RTSW_API void rtFramebufferSetDepthView(rt_framebuffer framebuffer, rt_texture_view view);
RTSW_API void rtFramebufferSetStencilView(rt_framebuffer framebuffer, rt_texture_view view);

RTSW_DECLARE_HANDLE(framebuffer, rtsw_framebuffer);

void rtsw_framebuffer_set_color_view(struct rtsw_framebuffer* framebuffer, u32 slot, struct rtsw_texture_view* view);
void rtsw_framebuffer_set_depth_view(struct rtsw_framebuffer* framebuffer, struct rtsw_texture_view* view);
void rtsw_framebuffer_set_stencil_view(struct rtsw_framebuffer* framebuffer, struct rtsw_texture_view* view);

#endif
