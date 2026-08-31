#include "texture_preview.h"
#include "debugger.h"
#include "trace.h"

#include <stdlib.h>
#include <string.h>

struct rtdbg_texture_preview_source {
	rt_texture texture;
	enum rt_texture_type type;
	enum rt_format format;
	rt_extent_3d extent;
	usize mip_count;
	struct rtdbg_texture_preview_source* next;
};

static struct rtdbg_texture_preview_source* rtdbg_texture_preview_sources;

static struct rtdbg_texture_preview_source* rtdbg_texture_preview_find(rt_texture texture) {
	for (struct rtdbg_texture_preview_source* source = rtdbg_texture_preview_sources; source; source = source->next) if (source->texture == texture) return source;
	return NULL;
}

void rtdbg_texture_preview_resize(rt_texture texture, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count) {
	struct rtdbg_texture_preview_source* source = rtdbg_texture_preview_find(texture);
	if (!source) {
		source = calloc(1, sizeof(*source));
		if (!source) return;
		source->texture = texture;
		source->next = rtdbg_texture_preview_sources;
		rtdbg_texture_preview_sources = source;
	}
	source->type = type;
	source->format = format;
	source->extent = extent;
	source->mip_count = mip_count;
}

static usize rtdbg_texture_preview_texel_size(enum rt_format format) {
	switch (format) {
	case RT_R8_UNORM: return 1;
	case RT_RG8_UNORM: return 2;
	case RT_RGB8_UNORM: return 3;
	case RT_RGBA8_UNORM: return 4;
	default: return 0;
	}
}

void rtdbg_texture_preview_data(rt_texture texture, rt_texture_range range, const u08* data) {
	struct rtdbg_texture_preview_source* source = rtdbg_texture_preview_find(texture);
	if (!source || !data || source->type != RT_TEXTURE_2D || source->extent.depth != 1 || source->mip_count == 0 || range.aspects != RT_TEXTURE_ASPECT_COLOR || range.base_mip != 0 || range.mip_count != 1 || range.base_layer != 0 || range.layer_count != 1 || range.offset.width != 0 || range.offset.height != 0 || range.offset.depth != 0 || memcmp(&range.extent, &source->extent, sizeof(range.extent))) return;
	usize texel_size = rtdbg_texture_preview_texel_size(source->format);
	if (!texel_size || source->extent.width > SIZE_MAX / source->extent.height || source->extent.width * source->extent.height > SIZE_MAX / 4) return;
	usize texel_count = source->extent.width * source->extent.height;
	usize rgba_size = texel_count * 4;
	u08* rgba = malloc(rgba_size);
	if (!rgba) return;
	for (usize index = 0; index < texel_count; ++index) {
		const u08* input = data + index * texel_size;
		u08* output = rgba + index * 4;
		output[0] = input[0];
		output[1] = texel_size > 1 ? input[1] : input[0];
		output[2] = texel_size > 2 ? input[2] : input[0];
		output[3] = texel_size > 3 ? input[3] : 255;
	}
	rtdbg_debugger_texture_preview(rtdbg_trace_handle_id(texture), source->extent.width, source->extent.height, rgba, rgba_size);
	free(rgba);
}

void rtdbg_texture_preview_destroy(rt_texture texture) {
	struct rtdbg_texture_preview_source** link = &rtdbg_texture_preview_sources;
	while (*link && (*link)->texture != texture) link = &(*link)->next;
	if (*link) { struct rtdbg_texture_preview_source* source = *link; *link = source->next; free(source); }
	rtdbg_debugger_texture_preview_remove(rtdbg_trace_handle_id(texture));
}

void rtdbg_texture_preview_reset(void) {
	while (rtdbg_texture_preview_sources) {
		struct rtdbg_texture_preview_source* source = rtdbg_texture_preview_sources;
		rtdbg_texture_preview_sources = source->next;
		free(source);
	}
	rtdbg_debugger_texture_preview_reset();
}
