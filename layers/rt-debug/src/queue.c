#include "next.h"
#include "trace.h"

RT_API_PUBLIC void rtQueueWait(rt_queue queue, rt_timepoint timepoint) {
	rtdbg_trace_api("rtQueueWait");
	rtdbg_procs.rtQueueWait(queue, timepoint);
}

RT_API_PUBLIC rt_timepoint rtQueueFlush(rt_queue queue) {
	rtdbg_trace_api("rtQueueFlush");
	return rtdbg_procs.rtQueueFlush(queue);
}

RT_API_PUBLIC void rtTimepointWait(rt_timepoint timepoint) {
	rtdbg_trace_api("rtTimepointWait");
	rtdbg_procs.rtTimepointWait(timepoint);
}

RT_API_PUBLIC bool rtTimepointReached(rt_timepoint timepoint) {
	rtdbg_trace_api("rtTimepointReached");
	return rtdbg_procs.rtTimepointReached(timepoint);
}

RT_API_PUBLIC rt_queue rtQueueCreate(enum rt_queue_capability capability) {
	rt_queue handle = rtdbg_procs.rtQueueCreate(capability);
	rtdbg_trace_resource_create("rtQueueCreate", "queue", handle);
	rtdbg_trace_resource_detail(handle, "capability %u", (unsigned)capability);
	return handle;
}

RT_API_PUBLIC void rtQueueDestroy(rt_queue queue) {
	rtdbg_trace_resource_destroy("rtQueueDestroy", "queue", queue);
	rtdbg_procs.rtQueueDestroy(queue);
}

RT_API_PUBLIC rt_timepoint rtQueueSubmit(rt_queue queue, rt_command_buffer command_buffer) {
	rt_timepoint timepoint = rtdbg_procs.rtQueueSubmit(queue, command_buffer);
	rtdbg_trace_queue_submit(queue, command_buffer, timepoint.value);
	return timepoint;
}

