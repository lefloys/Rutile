#include "procs.h"

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RT_API_PUBLIC rt_queue rtQueueCreate(enum rt_queue_capability capability) { return rtlog_rtQueueCreate(capability); }
RT_API_PUBLIC void rtQueueDestroy(rt_queue queue) { rtlog_rtQueueDestroy(queue); }
RT_API_PUBLIC rt_timepoint rtQueueSubmit(rt_queue queue, rt_command_buffer command_buffer) { return rtlog_rtQueueSubmit(queue, command_buffer); }
RT_API_PUBLIC void rtQueueWait(rt_queue queue, rt_timepoint timepoint) { rtlog_rtQueueWait(queue, timepoint); }
RT_API_PUBLIC rt_timepoint rtQueueFlush(rt_queue queue) { return rtlog_rtQueueFlush(queue); }
RT_API_PUBLIC void rtTimepointWait(rt_timepoint timepoint) { rtlog_rtTimepointWait(timepoint); }
RT_API_PUBLIC bool rtTimepointReached(rt_timepoint timepoint) { return rtlog_rtTimepointReached(timepoint); }

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

rt_queue rtlog_rtQueueCreate(enum rt_queue_capability capability) {
	u64 start_ns = rtlog_now_ns();

	rtlog_printf("rtQueueCreate(capability=%d)\n", (i32)capability);
	rt_queue result = next_rtQueueCreate(capability);
	rtlog_printf("rtQueueCreate -> %s [%s]\n", rtlog_pointer(result), rtlog_elapsed(start_ns));
	rtlog_error("rtQueueCreate");
	return result;
}

void rtlog_rtQueueDestroy(rt_queue queue) {
	next_rtQueueDestroy(queue);
	rtlog_error("rtQueueDestroy");
}

rt_timepoint rtlog_rtQueueSubmit(rt_queue queue, rt_command_buffer command_buffer) {
	u64 start_ns = rtlog_now_ns();
	rtlog_printf("rtQueueSubmit(queue=%s, command_buffer=%s)\n", rtlog_pointer(queue), rtlog_pointer(command_buffer));
	rt_timepoint result = next_rtQueueSubmit(queue, command_buffer);
	rtlog_printf("rtQueueSubmit -> %s [%s]\n", rtlog_timepoint(result), rtlog_elapsed(start_ns));
	rtlog_error("rtQueueSubmit");
	return result;
}

void rtlog_rtQueueWait(rt_queue queue, rt_timepoint timepoint) {
	u64 start_ns = rtlog_now_ns();
	rtlog_printf("rtQueueWait(queue=%s, timepoint=%s)\n", rtlog_pointer(queue), rtlog_timepoint(timepoint));
	next_rtQueueWait(queue, timepoint);
	rtlog_printf("rtQueueWait completed in %s\n", rtlog_elapsed(start_ns));
	rtlog_error("rtQueueWait");
}

rt_timepoint rtlog_rtQueueFlush(rt_queue queue) {
	u64 start_ns = rtlog_now_ns();

	rtlog_printf("rtQueueFlush(queue=%s)\n", rtlog_pointer(queue));
	rt_timepoint result = next_rtQueueFlush(queue);
	rtlog_printf("rtQueueFlush -> %s [%s]\n", rtlog_timepoint(result), rtlog_elapsed(start_ns));
	rtlog_error("rtQueueFlush");
	return result;
}

void rtlog_rtTimepointWait(rt_timepoint timepoint) {
	u64 start_ns = rtlog_now_ns();
	rtlog_printf("rtTimepointWait(timepoint=%s)\n", rtlog_timepoint(timepoint));
	next_rtTimepointWait(timepoint);
	rtlog_printf("rtTimepointWait completed in %s\n", rtlog_elapsed(start_ns));
	rtlog_error("rtTimepointWait");
}

bool rtlog_rtTimepointReached(rt_timepoint timepoint) {
	u64 start_ns = rtlog_now_ns();

	rtlog_printf("rtTimepointReached(timepoint=%s)\n", rtlog_timepoint(timepoint));
	bool result = next_rtTimepointReached(timepoint);
	rtlog_printf("rtTimepointReached -> %s [%s]\n", result ? "true" : "false", rtlog_elapsed(start_ns));
	rtlog_error("rtTimepointReached");
	return result;
}
