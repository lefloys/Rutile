#include "queue.h"

#include "context.h"
#include "error.h"
#include "execution.h"

#include <stdlib.h>

#define RTSW_TIMEPOINT_QUEUE_SHIFT 56
#define RTSW_TIMEPOINT_VALUE_MASK UINT64_C(0x00ffffffffffffff)

static void rtsw_queue_finish(struct rtsw_queue* queue) {
	struct rtsw_context* context = queue->base.ctx;
	struct rtsw_queue** previous = context ? &context->queues : NULL;
	while (previous && *previous && *previous != queue) previous = &(*previous)->next_context_queue;
	if (previous && *previous == queue) *previous = queue->next_context_queue;
	free(queue->wait_timepoints);
}

static void rtsw_queue_finalize_resource(void* value) {
	struct rtsw_queue* queue = value;
	rtsw_queue_finish(queue);
	free(queue);
}

RTSW_DEFINE_HANDLE(queue, rtsw_queue)

rt_timepoint rtsw_queue_timepoint(const struct rtsw_queue* queue, u64 value) {
	if (!queue || !value) return (rt_timepoint){ 0 };
	return (rt_timepoint){ ((u64)queue->identifier << RTSW_TIMEPOINT_QUEUE_SHIFT) | value };
}

struct rtsw_queue* rtsw_queue_from_timepoint(struct rtsw_context* context, rt_timepoint timepoint) {
	u08 identifier;
	struct rtsw_queue* queue;
	if (!context || !timepoint.value) return NULL;
	identifier = (u08)(timepoint.value >> RTSW_TIMEPOINT_QUEUE_SHIFT);
	for (queue = context->queues; queue; queue = queue->next_context_queue) {
		if (queue->identifier == identifier) return queue;
	}
	return NULL;
}

bool rtsw_timepoint_reached(struct rtsw_context* context, rt_timepoint timepoint) {
	struct rtsw_queue* queue;
	u64 value;
	if (!timepoint.value) return true;
	queue = rtsw_queue_from_timepoint(context, timepoint);
	value = timepoint.value & RTSW_TIMEPOINT_VALUE_MASK;
	return queue && value && value <= queue->timeline_value;
}

static bool rtsw_queue_append_wait(struct rtsw_queue* queue, rt_timepoint timepoint) {
	if (queue->wait_count == queue->wait_capacity) {
		usize capacity = queue->wait_capacity ? queue->wait_capacity * 2 : 8;
		rt_timepoint* timepoints = realloc(queue->wait_timepoints, capacity * sizeof(*timepoints));
		if (!timepoints) {
			rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to retain rt-software queue dependency");
			return false;
		}
		queue->wait_timepoints = timepoints;
		queue->wait_capacity = capacity;
	}
	queue->wait_timepoints[queue->wait_count++] = timepoint;
	return true;
}

rt_queue rtQueueCreate(enum rt_queue_capability capability) {
	struct rtsw_context* ctx = rtsw_get_current_context();
	struct rtsw_queue* queue;
	rtsw_clear_error();
	if (!ctx) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtQueueCreate called before rtInit");
		return RT_NULL_HANDLE;
	}
	if (capability != RT_QUEUE_TRANSFER && capability != RT_QUEUE_COMPUTE && capability != RT_QUEUE_GRAPHICS) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtQueueCreate received an invalid capability");
		return RT_NULL_HANDLE;
	}
	if (ctx->next_queue_identifier == UINT8_MAX) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "rt-software has no queue timepoint identifiers remaining");
		return RT_NULL_HANDLE;
	}
	queue = RTSW_ALLOC_RESOURCE(struct rtsw_queue);
	if (!queue) return RT_NULL_HANDLE;
	rtsw_init_resource_base(ctx, &queue->base, queue, rtsw_queue_finalize_resource);
	queue->capability = capability;
	queue->identifier = ++ctx->next_queue_identifier;
	queue->next_context_queue = ctx->queues;
	ctx->queues = queue;
	return rtsw_queue_to_handle(queue);
}

void rtQueueDestroy(rt_queue handle) {
	struct rtsw_queue* queue = rtsw_queue_from_handle(handle);
	if (queue) rtsw_resource_retire(&queue->base);
}

void rtQueueWait(rt_queue handle, rt_timepoint timepoint) {
	struct rtsw_queue* queue = rtsw_queue_from_handle(handle);
	rtsw_clear_error();
	if (!queue || !rtsw_timepoint_reached(rtsw_get_current_context(), timepoint)) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtQueueWait received an invalid or unreached timepoint");
		return;
	}
	if (timepoint.value) (void)rtsw_queue_append_wait(queue, timepoint);
}

rt_timepoint rtQueueSubmit(rt_queue queue_handle, rt_command_buffer command_buffer_handle) {
	struct rtsw_queue* queue = rtsw_queue_from_handle(queue_handle);
	struct rtsw_command_buffer* command_buffer = rtsw_command_buffer_from_handle(command_buffer_handle);
	rtsw_clear_error();
	if (!queue || !command_buffer || !command_buffer->executable) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtQueueSubmit requires an executable command buffer");
		return (rt_timepoint){ 0 };
	}
	for (usize index = 0; index < queue->wait_count; ++index) {
		if (!rtsw_timepoint_reached(rtsw_get_current_context(), queue->wait_timepoints[index])) {
			rtsw_throwf(RT_IMPROPER_USAGE, "rtQueueSubmit received an unresolved queue dependency");
			return (rt_timepoint){ 0 };
		}
	}
	queue->wait_count = 0;
	rtsw_execute_command_buffer(command_buffer);
	return rtsw_queue_timepoint(queue, ++queue->timeline_value);
}

rt_timepoint rtQueueFlush(rt_queue handle) {
	struct rtsw_queue* queue = rtsw_queue_from_handle(handle);
	return rtsw_queue_timepoint(queue, queue ? queue->timeline_value : 0);
}

void rtTimepointWait(rt_timepoint timepoint) {
	rtsw_clear_error();
	if (!rtsw_timepoint_reached(rtsw_get_current_context(), timepoint)) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtTimepointWait received an invalid or unreached timepoint");
	}
}

bool rtTimepointReached(rt_timepoint timepoint) {
	return rtsw_timepoint_reached(rtsw_get_current_context(), timepoint);
}
