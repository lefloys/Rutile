#include "buffer.h"
#include "logger.h"

#define RTVAL_DROP(message) rtval_printf("[validation] %s, dropping call\n", message)
#define RTVAL_RESOLVE(handle, call_name, ret)                                  \
	struct rtval_buffer* state = RTVAL_PAYLOAD((handle), struct rtval_buffer); \
	if (!state) {                                                              \
		RTVAL_DROP(call_name ": invalid handle");                              \
		return ret;                                                            \
	}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RT_API_PUBLIC rt_buffer rtBufferCreate(void) {
	return rtval_buffer_to_handle(rtval_buffer_create());
}

RT_API_PUBLIC void rtBufferDestroy(rt_buffer buffer) {
	rtval_buffer_destroy(rtval_buffer_from_handle(buffer));
}

RT_API_PUBLIC void rtBufferResize(rt_buffer buffer, enum rt_memory_type memory_type, usize size) {
	rtval_buffer_resize(rtval_buffer_from_handle(buffer), memory_type, size);
}

RT_API_PUBLIC void rtBufferRead(rt_buffer buffer, rt_buffer_range range, u08* data, usize data_size) {
	rtval_buffer_read(rtval_buffer_from_handle(buffer), range, data, data_size);
}

RT_API_PUBLIC u08* rtBufferMap(rt_buffer buffer, rt_buffer_range range) {
	return rtval_buffer_map(rtval_buffer_from_handle(buffer), range);
}

RT_API_PUBLIC void rtBufferUnmap(rt_buffer buffer) {
	rtval_buffer_unmap(rtval_buffer_from_handle(buffer));
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtval_buffer* rtval_buffer_create(void) {
	rt_buffer backend = rtval_next_rtBufferCreate();
	if (!backend) {
		rtval_report_error("rtBufferCreate");
		return NULL;
	}
	struct rtval_buffer* handle = rtval_handle_create(RTVAL_HANDLE_TYPE_BUFFER);
	if (!handle) {
		rtval_next_rtBufferDestroy(backend);
		return NULL;
	}
	struct rtval_buffer* state = RTVAL_PAYLOAD(handle, struct rtval_buffer);
	state->backend = backend;
	rtval_report_error("rtBufferCreate");
	return handle;
}

void rtval_buffer_destroy(struct rtval_buffer* buffer) {
	if (!buffer) {
		return;
	}
	struct rtval_buffer* state = RTVAL_PAYLOAD(buffer, struct rtval_buffer);
	if (!state) {
		RTVAL_DROP("rtBufferDestroy: invalid handle");
		return;
	}
	rtval_next_rtBufferDestroy(state->backend);
	rtval_handle_destroy(buffer);
}

void rtval_buffer_resize(struct rtval_buffer* buffer, enum rt_memory_type memory_type, usize size) {
	RTVAL_RESOLVE(buffer, "rtBufferResize", );
	if (memory_type != RT_HOST_MEMORY && memory_type != RT_DEVICE_MEMORY) {
		RTVAL_DROP("rtBufferResize: valid memory type required");
		return;
	}
	rtval_next_rtBufferResize(state->backend, memory_type, size);
	rtval_report_error("rtBufferResize");
}

void rtval_buffer_read(struct rtval_buffer* buffer, rt_buffer_range range, u08* data, usize data_size) {
	RTVAL_RESOLVE(buffer, "rtBufferRead", );
	if ((range.size && !data) || data_size < range.size) {
		RTVAL_DROP("rtBufferRead: destination must hold the requested range");
		return;
	}
	rtval_next_rtBufferRead(state->backend, range, data, data_size);
	rtval_report_error("rtBufferRead");
}

u08* rtval_buffer_map(struct rtval_buffer* buffer, rt_buffer_range range) {
	RTVAL_RESOLVE(buffer, "rtBufferMap", NULL);
	u08* data = rtval_next_rtBufferMap(state->backend, range);
	rtval_report_error("rtBufferMap");
	return data;
}

void rtval_buffer_unmap(struct rtval_buffer* buffer) {
	RTVAL_RESOLVE(buffer, "rtBufferUnmap", );
	rtval_next_rtBufferUnmap(state->backend);
	rtval_report_error("rtBufferUnmap");
}

#undef RTVAL_RESOLVE
#undef RTVAL_DROP
