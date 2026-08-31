#ifndef RTSW_QUEUE_H
#define RTSW_QUEUE_H

#include "command_buffer.h"

struct rtsw_queue {
	struct rtsw_resource_base base;
	enum rt_queue_capability capability;
	u08 identifier;
	u64 timeline_value;
	rt_timepoint* wait_timepoints;
	usize wait_count;
	usize wait_capacity;
	struct rtsw_queue* next_context_queue;
};

RTSW_API rt_queue rtQueueCreate(enum rt_queue_capability capability);
RTSW_API void rtQueueDestroy(rt_queue queue);
RTSW_API void rtQueueWait(rt_queue queue, rt_timepoint timepoint);
RTSW_API rt_timepoint rtQueueSubmit(rt_queue queue, rt_command_buffer command_buffer);
RTSW_API rt_timepoint rtQueueFlush(rt_queue queue);
RTSW_API void rtTimepointWait(rt_timepoint timepoint);
RTSW_API bool rtTimepointReached(rt_timepoint timepoint);

RTSW_DECLARE_HANDLE(queue, rtsw_queue);

rt_timepoint rtsw_queue_timepoint(const struct rtsw_queue* queue, u64 value);
struct rtsw_queue* rtsw_queue_from_timepoint(struct rtsw_context* context, rt_timepoint timepoint);
bool rtsw_timepoint_reached(struct rtsw_context* context, rt_timepoint timepoint);

#endif
