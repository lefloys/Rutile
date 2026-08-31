#ifndef RTSW_TEXTURE_H
#define RTSW_TEXTURE_H

#include "resource.h"

struct rtsw_texture {
	struct rtsw_resource_base base;
	enum rt_texture_type type;
	enum rt_format format;
	rt_extent_3d extent;
	usize mip_count;
	u08* bytes;
	usize byte_size;
};

struct rtsw_texture_view {
	struct rtsw_resource_base base;
	struct rtsw_texture* texture;
};

RTSW_API rt_texture rtTextureCreate(void);
RTSW_API void rtTextureDestroy(rt_texture texture);
RTSW_API void rtTextureResize(rt_texture texture, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count);
RTSW_API rt_texture_view rtTextureViewCreate(void);
RTSW_API void rtTextureViewDestroy(rt_texture_view texture_view);
RTSW_API rt_extent_3d rtTextureViewExtent(rt_texture_view texture_view);
RTSW_API void rtTextureViewSetTexture(rt_texture_view texture_view, rt_texture texture);
RTSW_API void rtTextureViewRead(rt_texture_view texture_view, rt_texture_range range, u08* data, usize data_size);

RTSW_DECLARE_HANDLE(texture, rtsw_texture);
RTSW_DECLARE_HANDLE(texture_view, rtsw_texture_view);

usize rtsw_format_texel_size(enum rt_format format);
bool rtsw_texture_validate_range(const struct rtsw_texture* texture, rt_texture_range range);
usize rtsw_texture_range_byte_size(const struct rtsw_texture* texture, rt_texture_range range);
void rtsw_texture_read(const struct rtsw_texture* texture, rt_texture_range range, u08* data);
void rtsw_texture_write(struct rtsw_texture* texture, rt_texture_range range, const u08* data);

#endif
