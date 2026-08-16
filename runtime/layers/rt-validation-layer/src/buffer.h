#ifndef RTVAL_BUFFER_H
#define RTVAL_BUFFER_H

#include "handles.h"

struct rtval_buffer {
	rt_buffer backend;
};

struct rtval_buffer* rtval_buffer_create(void);
void rtval_buffer_destroy(struct rtval_buffer* buffer);
void rtval_buffer_resize(struct rtval_buffer* buffer, enum rt_memory_type memory_type, usize size);
void rtval_buffer_read(struct rtval_buffer* buffer, rt_buffer_range range, u08* data, usize data_size);
u08* rtval_buffer_map(struct rtval_buffer* buffer, rt_buffer_range range);
void rtval_buffer_unmap(struct rtval_buffer* buffer);

#endif
