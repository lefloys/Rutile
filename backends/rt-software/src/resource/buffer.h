#ifndef RTSW_BUFFER_H
#define RTSW_BUFFER_H

#include "resource.h"

struct rtsw_buffer {
	struct rtsw_resource_base base;
	u08* bytes;
	usize size;
	enum rt_memory_type memory_type;
	bool mapped;
};

RTSW_API rt_buffer rtBufferCreate(void);
RTSW_API void rtBufferDestroy(rt_buffer buffer);
RTSW_API void rtBufferResize(rt_buffer buffer, enum rt_memory_type memory_type, usize size);
RTSW_API void rtBufferRead(rt_buffer buffer, rt_buffer_range range, u08* data, usize data_size);
RTSW_API u08* rtBufferMap(rt_buffer buffer, rt_buffer_range range);
RTSW_API void rtBufferUnmap(rt_buffer buffer);

RTSW_DECLARE_HANDLE(buffer, rtsw_buffer);

#endif
