#ifndef RTVAL_TEXTURE_H
#define RTVAL_TEXTURE_H

#include "handles.h"

struct rtval_texture {
	rt_texture backend;
};

struct rtval_texture* rtval_texture_create(void);
void rtval_texture_destroy(struct rtval_texture* texture);
void rtval_texture_resize(struct rtval_texture* texture, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count);

struct rtval_texture_view* rtval_texture_view_create(void);
void rtval_texture_view_set_texture(struct rtval_texture_view* view, struct rtval_texture* texture);
void rtval_texture_view_destroy(struct rtval_texture_view* view);
rt_extent_3d rtval_texture_view_extent(struct rtval_texture_view* view);
void rtval_texture_view_read(struct rtval_texture_view* view, rt_texture_range range, u08* data, usize data_size);

#endif
