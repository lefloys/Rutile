#include "queue.h"
#include "command_buffer.h"
#include "logger.h"

#define RTVAL_DROP(message) rtval_printf("[validation] %s, dropping call\n", message)

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RT_API_PUBLIC rt_queue rtQueueCreate(enum rt_queue_capability capability) {
	return rtval_queue_to_handle(rtval_queue_create(capability));
}

RT_API_PUBLIC void rtQueueDestroy(rt_queue queue) {
	rtval_queue_destroy(rtval_queue_from_handle(queue));
}

RT_API_PUBLIC void rtQueueWait(rt_queue queue, rt_timepoint timepoint) {
	rtval_queue_wait(rtval_queue_from_handle(queue), timepoint);
}

RT_API_PUBLIC rt_timepoint rtQueueSubmit(rt_queue queue, rt_command_buffer command_buffer) {
	return rtval_queue_submit(
		rtval_queue_from_handle(queue),
		rtval_command_buffer_from_handle(command_buffer)
	);
}

RT_API_PUBLIC rt_timepoint rtQueueFlush(rt_queue queue) {
	return rtval_queue_flush(rtval_queue_from_handle(queue));
}

RT_API_PUBLIC void rtTimepointWait(rt_timepoint timepoint) {
	rtval_timepoint_wait_public(timepoint);
}

RT_API_PUBLIC bool rtTimepointReached(rt_timepoint timepoint) {
	return rtval_timepoint_reached_public(timepoint);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtval_queue* rtval_queue_create(enum rt_queue_capability capability) {
	rt_queue backend = rtval_next_rtQueueCreate(capability);
	if (!backend) {
		rtval_report_error("rtQueueCreate");
		return NULL;
	}
	struct rtval_queue* queue = rtval_handle_create(RTVAL_HANDLE_TYPE_QUEUE);
	if (!queue) {
		rtval_next_rtQueueDestroy(backend);
		return NULL;
	}
	RTVAL_PAYLOAD(queue, struct rtval_queue)->backend = backend;
	rtval_report_error("rtQueueCreate");
	return queue;
}

void rtval_queue_destroy(struct rtval_queue* queue) {
	struct rtval_queue* state = RTVAL_PAYLOAD(queue, struct rtval_queue);
	if (!state) {
		RTVAL_DROP("rtQueueDestroy: invalid queue");
		return;
	}
	rtval_next_rtQueueDestroy(state->backend);
	rtval_handle_destroy(queue);
	rtval_report_error("rtQueueDestroy");
}

void rtval_queue_wait(struct rtval_queue* queue, rt_timepoint timepoint) {
	struct rtval_queue* state = RTVAL_PAYLOAD(queue, struct rtval_queue);
	if (!state) {
		RTVAL_DROP("rtQueueWait: invalid queue");
		return;
	}
	rtval_next_rtQueueWait(state->backend, timepoint);
	rtval_report_error("rtQueueWait");
}

rt_timepoint rtval_queue_submit(struct rtval_queue* queue, struct rtval_command_buffer* command_buffer) {
	struct rtval_queue* queue_state = RTVAL_PAYLOAD(queue, struct rtval_queue);
	struct rtval_command_buffer* command_buffer_state = RTVAL_PAYLOAD(command_buffer, struct rtval_command_buffer);
	if (!queue_state || !command_buffer_state) {
		RTVAL_DROP("rtQueueSubmit: valid queue and command buffer required");
		return (rt_timepoint){ 0 };
	}
	if (!command_buffer_state->executable || command_buffer_state->recording || command_buffer_state->rendering) {
		RTVAL_DROP("rtQueueSubmit: completed command buffer required");
		return (rt_timepoint){ 0 };
	}
	rt_timepoint timepoint = rtval_next_rtQueueSubmit(queue_state->backend, command_buffer_state->backend);
	rtval_report_error("rtQueueSubmit");
	return timepoint;
}

rt_timepoint rtval_queue_flush(struct rtval_queue* queue) {
	struct rtval_queue* state = RTVAL_PAYLOAD(queue, struct rtval_queue);
	if (!state) {
		RTVAL_DROP("rtQueueFlush: invalid queue");
		return (rt_timepoint){ 0 };
	}
	rt_timepoint timepoint = rtval_next_rtQueueFlush(state->backend);
	rtval_report_error("rtQueueFlush");
	return timepoint;
}

void rtval_timepoint_wait_public(rt_timepoint timepoint) {
	rtval_next_rtTimepointWait(timepoint);
	rtval_report_error("rtTimepointWait");
}

bool rtval_timepoint_reached_public(rt_timepoint timepoint) {
	bool reached = rtval_next_rtTimepointReached(timepoint);
	rtval_report_error("rtTimepointReached");
	return reached;
}

#undef RTVAL_DROP
