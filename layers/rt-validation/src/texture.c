#include "texture.h"
#include "logger.h"
#include "texture_view.h"

#define RTVAL_DROP(message) rtval_printf("[validation] %s, dropping call\n", message)

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RT_API_PUBLIC rt_texture rtTextureCreate(void) {
	return rtval_texture_to_handle(rtval_texture_create());
}

RT_API_PUBLIC void rtTextureDestroy(rt_texture texture) {
	rtval_texture_destroy(rtval_texture_from_handle(texture));
}

RT_API_PUBLIC void rtTextureResize(rt_texture texture, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count) {
	rtval_texture_resize(rtval_texture_from_handle(texture), type, format, extent, mip_count);
}

RT_API_PUBLIC rt_texture_view rtTextureViewCreate(void) {
	return rtval_texture_view_to_handle(rtval_texture_view_create());
}

RT_API_PUBLIC void rtTextureViewSetTexture(rt_texture_view texture_view, rt_texture texture) {
	rtval_texture_view_set_texture(rtval_texture_view_from_handle(texture_view), rtval_texture_from_handle(texture));
}

RT_API_PUBLIC void rtTextureViewDestroy(rt_texture_view texture_view) {
	rtval_texture_view_destroy(rtval_texture_view_from_handle(texture_view));
}

RT_API_PUBLIC rt_extent_3d rtTextureViewExtent(rt_texture_view texture_view) {
	return rtval_texture_view_extent(rtval_texture_view_from_handle(texture_view));
}

RT_API_PUBLIC void rtTextureViewRead(rt_texture_view texture_view, rt_texture_range range, u08* data, usize data_size) {
	rtval_texture_view_read(rtval_texture_view_from_handle(texture_view), range, data, data_size);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtval_texture* rtval_texture_create(void) {
	rt_texture backend = rtval_next_rtTextureCreate();
	if (!backend) {
		rtval_report_error("rtTextureCreate");
		return NULL;
	}
	struct rtval_texture* handle = rtval_handle_create(RTVAL_HANDLE_TYPE_TEXTURE);
	if (!handle) {
		rtval_next_rtTextureDestroy(backend);
		return NULL;
	}
	struct rtval_texture* state = RTVAL_PAYLOAD(handle, struct rtval_texture);
	state->backend = backend;
	rtval_report_error("rtTextureCreate");
	return handle;
}

void rtval_texture_destroy(struct rtval_texture* texture) {
	if (!texture) {
		return;
	}
	struct rtval_texture* state = RTVAL_PAYLOAD(texture, struct rtval_texture);
	if (!state) {
		RTVAL_DROP("rtTextureDestroy: invalid handle");
		return;
	}
	rtval_next_rtTextureDestroy(state->backend);
	rtval_handle_destroy(texture);
}

void rtval_texture_resize(struct rtval_texture* texture, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count) {
	struct rtval_texture* state = RTVAL_PAYLOAD(texture, struct rtval_texture);
	if (!state || !mip_count || !extent.width || !extent.height || !extent.depth) {
		RTVAL_DROP("rtTextureResize: texture, non-zero extent, and mip count required");
		return;
	}
	rtval_next_rtTextureResize(state->backend, type, format, extent, mip_count);
	rtval_report_error("rtTextureResize");
}

struct rtval_texture_view* rtval_texture_view_create(void) {
	rt_texture_view backend = rtval_next_rtTextureViewCreate();
	if (!backend) {
		rtval_report_error("rtTextureViewCreate");
		return NULL;
	}
	struct rtval_texture_view* handle = rtval_handle_create(RTVAL_HANDLE_TYPE_TEXTURE_VIEW);
	if (!handle) {
		rtval_next_rtTextureViewDestroy(backend);
		return NULL;
	}
	struct rtval_texture_view* state = RTVAL_PAYLOAD(handle, struct rtval_texture_view);
	state->backend = backend;
	rtval_report_error("rtTextureViewCreate");
	return handle;
}

void rtval_texture_view_set_texture(struct rtval_texture_view* view, struct rtval_texture* texture) {
	struct rtval_texture_view* view_state = RTVAL_PAYLOAD(view, struct rtval_texture_view);
	struct rtval_texture* tex_state = RTVAL_PAYLOAD(texture, struct rtval_texture);
	if (!view_state || !tex_state) {
		RTVAL_DROP("rtTextureViewSetTexture: invalid handle");
		return;
	}
	rtval_next_rtTextureViewSetTexture(view_state->backend, tex_state->backend);
	rtval_report_error("rtTextureViewSetTexture");
}

void rtval_texture_view_destroy(struct rtval_texture_view* view) {
	if (!view) {
		return;
	}
	struct rtval_texture_view* state = RTVAL_PAYLOAD(view, struct rtval_texture_view);
	if (!state) {
		RTVAL_DROP("rtTextureViewDestroy: invalid handle");
		return;
	}
	rtval_next_rtTextureViewDestroy(state->backend);
	rtval_handle_destroy(view);
}

rt_extent_3d rtval_texture_view_extent(struct rtval_texture_view* view) {
	rt_extent_3d empty = { 0, 0, 0 };
	struct rtval_texture_view* state = RTVAL_PAYLOAD(view, struct rtval_texture_view);
	if (!state) {
		RTVAL_DROP("rtTextureViewExtent: invalid handle");
		return empty;
	}
	rt_extent_3d extent = rtval_next_rtTextureViewExtent(state->backend);
	rtval_report_error("rtTextureViewExtent");
	return extent;
}

void rtval_texture_view_read(struct rtval_texture_view* view, rt_texture_range range, u08* data, usize data_size) {
	struct rtval_texture_view* state = RTVAL_PAYLOAD(view, struct rtval_texture_view);
	if (!state || !data) {
		RTVAL_DROP("rtTextureViewRead: view and destination required");
		return;
	}
	rtval_next_rtTextureViewRead(state->backend, range, data, data_size);
	rtval_report_error("rtTextureViewRead");
}

#undef RTVAL_DROP
