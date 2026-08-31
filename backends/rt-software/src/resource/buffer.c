#include "buffer.h"

#include "context.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>

static void rtsw_buffer_finish(struct rtsw_buffer* buffer) {
	free(buffer->bytes);
	buffer->bytes = NULL;
	buffer->size = 0;
}

static void rtsw_buffer_finalize_resource(void* value) {
	struct rtsw_buffer* buffer = value;
	rtsw_buffer_finish(buffer);
	free(buffer);
}

RTSW_DEFINE_HANDLE(buffer, rtsw_buffer)

rt_buffer rtBufferCreate(void) {
	struct rtsw_context* ctx = rtsw_get_current_context();
	struct rtsw_buffer* buffer;
	rtsw_clear_error();

	if (!ctx) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtBufferCreate called before rtInit");
		return RT_NULL_HANDLE;
	}

	buffer = RTSW_ALLOC_RESOURCE(struct rtsw_buffer);
	if (!buffer) {
		return RT_NULL_HANDLE;
	}
	rtsw_init_resource_base(ctx, &buffer->base, buffer, rtsw_buffer_finalize_resource);
	return rtsw_buffer_to_handle(buffer);
}

void rtBufferDestroy(rt_buffer handle) {
	struct rtsw_buffer* buffer = rtsw_buffer_from_handle(handle);
	if (buffer) {
		rtsw_resource_retire(&buffer->base);
	}
}

void rtBufferResize(rt_buffer handle, enum rt_memory_type memory_type, usize size) {
	struct rtsw_buffer* buffer = rtsw_buffer_from_handle(handle);
	u08* bytes;
	rtsw_clear_error();

	if (!buffer || buffer->mapped) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtBufferResize requires an unmapped buffer");
		return;
	}
	if (memory_type != RT_HOST_MEMORY && memory_type != RT_DEVICE_MEMORY) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtBufferResize received an invalid memory type");
		return;
	}

	bytes = size ? realloc(buffer->bytes, size) : NULL;
	if (size && !bytes) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for buffer", size);
		return;
	}
	if (size > buffer->size) {
		memset(bytes + buffer->size, 0, size - buffer->size);
	}
	if (!size) {
		free(buffer->bytes);
	}
	buffer->bytes = bytes;
	buffer->size = size;
	buffer->memory_type = memory_type;
}

static bool rtsw_buffer_validate_range(struct rtsw_buffer* buffer, rt_buffer_range range) {
	return buffer && range.offset <= buffer->size && range.size <= buffer->size - range.offset;
}

void rtBufferRead(rt_buffer handle, rt_buffer_range range, u08* data, usize data_size) {
	struct rtsw_buffer* buffer = rtsw_buffer_from_handle(handle);
	rtsw_clear_error();
	if (!rtsw_buffer_validate_range(buffer, range) || !data || data_size != range.size) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtBufferRead received an invalid range");
		return;
	}
	memcpy(data, buffer->bytes + range.offset, range.size);
}

u08* rtBufferMap(rt_buffer handle, rt_buffer_range range) {
	struct rtsw_buffer* buffer = rtsw_buffer_from_handle(handle);
	rtsw_clear_error();
	if (!rtsw_buffer_validate_range(buffer, range) || buffer->memory_type != RT_HOST_MEMORY || buffer->mapped) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtBufferMap requires an unmapped host buffer and valid range");
		return NULL;
	}
	buffer->mapped = true;
	return buffer->bytes + range.offset;
}

void rtBufferUnmap(rt_buffer handle) {
	struct rtsw_buffer* buffer = rtsw_buffer_from_handle(handle);
	rtsw_clear_error();
	if (!buffer || !buffer->mapped) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtBufferUnmap requires a mapped buffer");
		return;
	}
	buffer->mapped = false;
}
