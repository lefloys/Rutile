#include "texture.h"

#include "context.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>

usize rtsw_format_texel_size(enum rt_format format) {
	switch (format) {
	case RT_R8_UNORM: return 1;
	case RT_RG8_UNORM: return 2;
	case RT_RGB8_UNORM: return 3;
	case RT_RGBA8_UNORM: return 4;
	case RT_R32_SFLOAT: return 4;
	case RT_RG32_SFLOAT: return 8;
	case RT_RGB32_SFLOAT: return 12;
	case RT_RGBA32_SFLOAT: return 16;
	case RT_D32_SFLOAT: return 4;
	case RT_S8_UINT: return 1;
	default: return 0;
	}
}

static void rtsw_texture_finish(struct rtsw_texture* texture) {
	free(texture->bytes);
	texture->bytes = NULL;
	texture->byte_size = 0;
}

static void rtsw_texture_finalize_resource(void* value) {
	struct rtsw_texture* texture = value;
	rtsw_texture_finish(texture);
	free(texture);
}

RTSW_DEFINE_HANDLE(texture, rtsw_texture)

rt_texture rtTextureCreate(void) {
	struct rtsw_context* ctx = rtsw_get_current_context();
	struct rtsw_texture* texture;
	rtsw_clear_error();

	if (!ctx) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtTextureCreate called before rtInit");
		return RT_NULL_HANDLE;
	}

	texture = RTSW_ALLOC_RESOURCE(struct rtsw_texture);
	if (!texture) {
		return RT_NULL_HANDLE;
	}
	rtsw_init_resource_base(ctx, &texture->base, texture, rtsw_texture_finalize_resource);
	return rtsw_texture_to_handle(texture);
}

void rtTextureDestroy(rt_texture handle) {
	struct rtsw_texture* texture = rtsw_texture_from_handle(handle);
	if (texture) {
		rtsw_resource_retire(&texture->base);
	}
}

void rtTextureResize(rt_texture handle, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count) {
	struct rtsw_texture* texture = rtsw_texture_from_handle(handle);
	usize texel_size = rtsw_format_texel_size(format);
	usize byte_size;
	u08* bytes;
	rtsw_clear_error();

	if (!texture || type != RT_TEXTURE_2D || !extent.width || !extent.height || extent.depth != 1 || mip_count != 1 || !texel_size) {
		rtsw_throwf(RT_FEATURE_NOT_SUPPORTED, "rt-software supports only single-mip 2D textures with implemented formats");
		return;
	}
	if (extent.width > SIZE_MAX / extent.height || extent.width * extent.height > SIZE_MAX / texel_size) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "texture extent is too large");
		return;
	}

	byte_size = extent.width * extent.height * texel_size;
	bytes = calloc(1, byte_size);
	if (!bytes) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for texture", byte_size);
		return;
	}
	rtsw_texture_finish(texture);
	texture->type = type;
	texture->format = format;
	texture->extent = extent;
	texture->mip_count = mip_count;
	texture->bytes = bytes;
	texture->byte_size = byte_size;
}

static void rtsw_texture_view_finish(struct rtsw_texture_view* texture_view) {
	if (texture_view->texture) {
		rtsw_resource_release(&texture_view->texture->base);
		texture_view->texture = NULL;
	}
}

static void rtsw_texture_view_finalize_resource(void* value) {
	struct rtsw_texture_view* texture_view = value;
	rtsw_texture_view_finish(texture_view);
	free(texture_view);
}

RTSW_DEFINE_HANDLE(texture_view, rtsw_texture_view)

rt_texture_view rtTextureViewCreate(void) {
	struct rtsw_context* ctx = rtsw_get_current_context();
	struct rtsw_texture_view* texture_view;
	rtsw_clear_error();

	if (!ctx) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtTextureViewCreate called before rtInit");
		return RT_NULL_HANDLE;
	}

	texture_view = RTSW_ALLOC_RESOURCE(struct rtsw_texture_view);
	if (!texture_view) {
		return RT_NULL_HANDLE;
	}
	rtsw_init_resource_base(ctx, &texture_view->base, texture_view, rtsw_texture_view_finalize_resource);
	return rtsw_texture_view_to_handle(texture_view);
}

void rtTextureViewDestroy(rt_texture_view handle) {
	struct rtsw_texture_view* texture_view = rtsw_texture_view_from_handle(handle);
	if (texture_view) {
		rtsw_resource_retire(&texture_view->base);
	}
}

rt_extent_3d rtTextureViewExtent(rt_texture_view handle) {
	struct rtsw_texture_view* texture_view = rtsw_texture_view_from_handle(handle);
	return texture_view && texture_view->texture ? texture_view->texture->extent : (rt_extent_3d){ 0 };
}

void rtTextureViewSetTexture(rt_texture_view handle, rt_texture texture_handle) {
	struct rtsw_texture_view* texture_view = rtsw_texture_view_from_handle(handle);
	struct rtsw_texture* texture = rtsw_texture_from_handle(texture_handle);
	rtsw_clear_error();

	if (!texture_view) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtTextureViewSetTexture received a NULL view");
		return;
	}
	if (texture) {
		rtsw_resource_retain(&texture->base);
	}
	rtsw_texture_view_finish(texture_view);
	texture_view->texture = texture;
}

bool rtsw_texture_validate_range(const struct rtsw_texture* texture, rt_texture_range range) {
	enum rt_texture_aspect_flag aspect;
	if (!texture) return false;
	aspect = texture->format == RT_D32_SFLOAT ? RT_TEXTURE_ASPECT_DEPTH :
		texture->format == RT_S8_UINT ? RT_TEXTURE_ASPECT_STENCIL : RT_TEXTURE_ASPECT_COLOR;
	return texture &&
		range.aspects == aspect &&
		range.base_mip == 0 &&
		range.mip_count == 1 &&
		range.base_layer == 0 &&
		range.layer_count == 1 &&
		range.offset.depth == 0 &&
		range.extent.depth == 1 &&
		range.offset.width <= texture->extent.width &&
		range.offset.height <= texture->extent.height &&
		range.extent.width <= texture->extent.width - range.offset.width &&
		range.extent.height <= texture->extent.height - range.offset.height;
}

usize rtsw_texture_range_byte_size(const struct rtsw_texture* texture, rt_texture_range range) {
	return range.extent.width * range.extent.height * rtsw_format_texel_size(texture->format);
}

void rtsw_texture_read(const struct rtsw_texture* texture, rt_texture_range range, u08* data) {
	usize texel_size = rtsw_format_texel_size(texture->format);
	usize source_stride = texture->extent.width * texel_size;
	usize row_size = range.extent.width * texel_size;
	for (usize y = 0; y < range.extent.height; ++y) {
		const u08* source = texture->bytes + (range.offset.height + y) * source_stride + range.offset.width * texel_size;
		memcpy(data + y * row_size, source, row_size);
	}
}

void rtsw_texture_write(struct rtsw_texture* texture, rt_texture_range range, const u08* data) {
	usize texel_size = rtsw_format_texel_size(texture->format);
	usize destination_stride = texture->extent.width * texel_size;
	usize row_size = range.extent.width * texel_size;
	for (usize y = 0; y < range.extent.height; ++y) {
		u08* destination = texture->bytes + (range.offset.height + y) * destination_stride + range.offset.width * texel_size;
		memcpy(destination, data + y * row_size, row_size);
	}
}

void rtTextureViewRead(rt_texture_view handle, rt_texture_range range, u08* data, usize data_size) {
	struct rtsw_texture_view* texture_view = rtsw_texture_view_from_handle(handle);
	struct rtsw_texture* texture = texture_view ? texture_view->texture : NULL;
	rtsw_clear_error();

	if (!rtsw_texture_validate_range(texture, range) || !data || data_size != rtsw_texture_range_byte_size(texture, range)) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtTextureViewRead received an invalid range");
		return;
	}
	rtsw_texture_read(texture, range, data);
}
