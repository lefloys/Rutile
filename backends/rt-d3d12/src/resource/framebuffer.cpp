#include "framebuffer.hpp"
#include "context.hpp"
#include "error.hpp"
#include "resource/program.hpp"

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

rt_framebuffer_t* rtFramebufferCreate(void) {
	rtd3d12_begin_errorable_operation();
	return rtd3d12::create_resource<rt_framebuffer_t>(rtd3d12_get_current_context());
}

void rtFramebufferDestroy(rt_framebuffer_t* framebuffer) {
	if (framebuffer) framebuffer->retire();
}

rt_texture_view_t* rtFramebufferColorView(rt_framebuffer_t* framebuffer, rt::location* location) {
	rtd3d12_begin_errorable_operation();
	rt_program_t* program = rtd3d12_location_program(location);
	const rtd3d12_program_output_mapping* mapping = program && location && program->output_mappings[location->address]
		? &*program->output_mappings[location->address] : nullptr;
	struct rt_texture_view_t* view = rtd3d12_framebuffer_color_view(framebuffer, mapping ? mapping->binding : 0);
	return view;
}

void rtFramebufferSetColorView(rt_framebuffer_t* framebuffer, rt_texture_view_t* view, rt::location* location) {
	rtd3d12_begin_errorable_operation();
	rt_program_t* program = rtd3d12_location_program(location);
	const rtd3d12_program_output_mapping* mapping = program && location && program->output_mappings[location->address]
		? &*program->output_mappings[location->address] : nullptr;
	rtd3d12_framebuffer_set_color_view(
		rtd3d12_get_current_context(),
		framebuffer,
		mapping ? mapping->binding : 0,
		view
	);
}

void rtFramebufferSetDepthView(rt_framebuffer_t* framebuffer, rt_texture_view_t* view) {
	rtd3d12_begin_errorable_operation();
	rtd3d12_framebuffer_set_depth_view(
		rtd3d12_get_current_context(),
		framebuffer,
		view
	);
}

void rtFramebufferSetStencilView(rt_framebuffer_t* framebuffer, rt_texture_view_t* view) {
	rtd3d12_begin_errorable_operation();
	rtd3d12_framebuffer_set_stencil_view(
		rtd3d12_get_current_context(),
		framebuffer,
		view
	);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

void rtd3d12_framebuffer_init(struct rtd3d12_context* ctx, struct rt_framebuffer_t* framebuffer) {
}

rt_framebuffer_t::~rt_framebuffer_t() {
	for (u32 i = 0; i < color_texture_count; i++) {
		if (color_views[i]) {
			color_views[i]->release();
		}
		color_views[i] = nullptr;
	}
	color_texture_count = 0;
	if (depth_view) {
		depth_view->release();
	}
	if (stencil_view) {
		stencil_view->release();
	}
	depth_view = nullptr;
	stencil_view = nullptr;
}

bool rtd3d12_texture_view_valid(struct rt_texture_view_t* view) {
	return view && rtd3d12_texture_view_refresh(view->ctx, view) && view->image && view->image->d3d_resource;
}

void rtd3d12_framebuffer_set_color_view(struct rtd3d12_context* ctx, struct rt_framebuffer_t* framebuffer, u32 slot, struct rt_texture_view_t* view) {
	if (slot >= RTD3D12_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS) {
		rtd3d12_fail(rt::error::unsupported_feature, "framebuffer requested color attachment {}, max is {}", slot, RTD3D12_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS);
		return;
	}

	if (view && !rtd3d12_texture_view_valid(view)) {
		rtd3d12_fail(rt::error::improper_usage, "framebuffer color texture view is invalid");
		return;
	}
	if (view && !view->rtv.ptr) {
		rtd3d12_fail(rt::error::improper_usage, "framebuffer color texture view has no render target view");
		return;
	}
	if (view && view->image && rtd3d12_texture_format_is_depth(view->image->dxgi_format)) {
		rtd3d12_fail(rt::error::improper_usage, "framebuffer color texture view format has no color aspect");
		return;
	}

	if (framebuffer->color_views[slot] != view) {
		if (view) {
			(view)->retain();
		}
		if (framebuffer->color_views[slot]) {
			(framebuffer->color_views[slot])->release();
		}
		framebuffer->color_views[slot] = view;
	}
	if (view && slot >= framebuffer->color_texture_count) {
		framebuffer->color_texture_count = slot + 1;
	}
	while (framebuffer->color_texture_count && !framebuffer->color_views[framebuffer->color_texture_count - 1]) {
		framebuffer->color_texture_count--;
	}
}

struct rt_texture_view_t* rtd3d12_framebuffer_color_view(struct rt_framebuffer_t* framebuffer, u32 slot) {
	if (!framebuffer) {
		rtd3d12_fail(rt::error::improper_usage, "framebuffer is nullptr");
		return nullptr;
	}
	if (slot >= RTD3D12_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS) {
		rtd3d12_fail(rt::error::unsupported_feature, "framebuffer requested color attachment {}, max is {}", slot, RTD3D12_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS);
		return nullptr;
	}

	if (slot >= framebuffer->color_texture_count) {
		return nullptr;
	}
	return framebuffer->color_views[slot];
}

void rtd3d12_framebuffer_set_depth_view(struct rtd3d12_context* ctx, struct rt_framebuffer_t* framebuffer, struct rt_texture_view_t* view) {
	if (!framebuffer) {
		rtd3d12_fail(rt::error::improper_usage, "framebuffer is nullptr");
		return;
	}
	if (view && !rtd3d12_texture_view_valid(view)) {
		rtd3d12_fail(rt::error::improper_usage, "framebuffer depth texture view is invalid");
		return;
	}
	if (view && !view->dsv.ptr) {
		rtd3d12_fail(rt::error::improper_usage, "framebuffer depth texture view has no depth stencil view");
		return;
	}
	if (view && view->image && !rtd3d12_texture_format_is_depth(view->image->dxgi_format)) {
		rtd3d12_fail(rt::error::improper_usage, "framebuffer depth texture view format has no depth aspect");
		return;
	}
	if (framebuffer->depth_view != view) {
		if (view) {
			(view)->retain();
		}
		if (framebuffer->depth_view) {
			(framebuffer->depth_view)->release();
		}
		framebuffer->depth_view = view;
	}
}

void rtd3d12_framebuffer_set_stencil_view(struct rtd3d12_context* ctx, struct rt_framebuffer_t* framebuffer, struct rt_texture_view_t* view) {
	if (!framebuffer) {
		rtd3d12_fail(rt::error::improper_usage, "framebuffer is nullptr");
		return;
	}
	if (view && (!rtd3d12_texture_view_valid(view) || !view->dsv.ptr || (view->image->dxgi_format != DXGI_FORMAT_D24_UNORM_S8_UINT && view->image->dxgi_format != DXGI_FORMAT_D32_FLOAT_S8X24_UINT))) {
		rtd3d12_fail(rt::error::improper_usage, "framebuffer stencil texture view is invalid");
		return;
	}
	if (framebuffer->stencil_view != view) {
		if (view) {
			(view)->retain();
		}
		if (framebuffer->stencil_view) {
			(framebuffer->stencil_view)->release();
		}
		framebuffer->stencil_view = view;
	}
}

bool rtd3d12_framebuffer_valid(struct rt_framebuffer_t* framebuffer) {
	for (u32 i = 0; i < framebuffer->color_texture_count; i++) {
		if (!rtd3d12_texture_view_valid(framebuffer->color_views[i])) {
			return false;
		}
	}
	if (framebuffer->depth_view && !rtd3d12_texture_view_valid(framebuffer->depth_view)) {
		return false;
	}
	if (framebuffer->stencil_view && !rtd3d12_texture_view_valid(framebuffer->stencil_view)) {
		return false;
	}
	return true;
}
