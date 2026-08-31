#ifndef RTDBG_TEXTURE_PREVIEW_H
#define RTDBG_TEXTURE_PREVIEW_H

#include "rutile.h"

void rtdbg_texture_preview_resize(rt_texture texture, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count);
void rtdbg_texture_preview_data(rt_texture texture, rt_texture_range range, const u08* data);
void rtdbg_texture_preview_destroy(rt_texture texture);
void rtdbg_texture_preview_reset(void);

#endif
