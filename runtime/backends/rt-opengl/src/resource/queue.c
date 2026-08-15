#include "resource/queue.h"

#include "context.h"
#include "execution.h"
#include "resource/command_buffer.h"
#include "resource/framebuffer.h"
#include "resource/swapchain.h"
#include "resource/texture.h"

#include <stdlib.h>

#define RTGL_TIMEPOINT_QUEUE_SHIFT 56
#define RTGL_TIMEPOINT_VALUE_MASK  UINT64_C(0x00ffffffffffffff)

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/
rt_queue rtQueueCreate(enum rt_queue_capability capability) {
	return rtgl_queue_to_handle(rtgl_queue_create_virtual(rtgl_get_current_context(), capability));
}

void rtQueueDestroy(rt_queue queue) {
	struct rtgl_queue* internal = rtgl_queue_from_handle(queue);
	if (!internal) {
		return;
	}
	rt_mutex_lock(internal->submit_lock);
	rtgl_execution_lock(internal->base.ctx);
	internal->active = false;
	rtgl_execution_unlock(internal->base.ctx);
	rt_mutex_unlock(internal->submit_lock);
}

rt_timepoint rtQueueSubmit(rt_queue queue, rt_command_buffer command_buffer) {
	struct rtgl_queue* internal = rtgl_queue_from_handle(queue);
	if (!internal) {
		return (rt_timepoint){ 0 };
	}
	rt_mutex_lock(internal->submit_lock);
	rtgl_execution_lock(internal->base.ctx);
	const bool active = internal->active;
	rtgl_execution_unlock(internal->base.ctx);
	if (!active) {
		rt_mutex_unlock(internal->submit_lock);
		return (rt_timepoint){ 0 };
	}
	rt_timepoint submitted = rtgl_command_buffer_submit(internal->base.ctx, internal, rtgl_command_buffer_from_handle(command_buffer));
	rt_mutex_unlock(internal->submit_lock);
	return submitted;
}

void rtQueueWait(rt_queue queue, rt_timepoint timepoint) {
	struct rtgl_queue* internal = rtgl_queue_from_handle(queue);
	if (!internal) {
		return;
	}
	rt_mutex_lock(internal->submit_lock);
	internal->pending_wait = timepoint;
	rt_mutex_unlock(internal->submit_lock);
}

rt_timepoint rtQueueFlush(rt_queue queue) {
	struct rtgl_queue* internal = rtgl_queue_from_handle(queue);
	if (!internal) {
		return (rt_timepoint){ 0 };
	}
	rt_mutex_lock(internal->submit_lock);
	rtgl_execution_lock(internal->base.ctx);
	rt_timepoint timepoint = { ((u64)internal->identifier << RTGL_TIMEPOINT_QUEUE_SHIFT) | internal->submitted_value };
	rtgl_execution_unlock(internal->base.ctx);
	rt_mutex_unlock(internal->submit_lock);
	rtgl_timepoint_wait(internal->base.ctx, timepoint);
	return timepoint;
}

void rtTimepointWait(rt_timepoint timepoint) {
	rtgl_timepoint_wait(rtgl_get_current_context(), timepoint);
}

bool rtTimepointReached(rt_timepoint timepoint) {
	return rtgl_timepoint_reached(rtgl_get_current_context(), timepoint);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/
RTGL_DEFINE_RESOURCE_PRIVATE(queue)

void rtgl_queue_init(struct rtgl_context* ctx, struct rtgl_queue* queue) {
	rtgl_init_resource_base(ctx, RTGL_RESOURCE_BASE(queue), RTGL_RESOURCE_QUEUE);
	queue->completion_condition = rt_condition_create();
	queue->submit_lock = rt_mutex_create();
	if (!queue->completion_condition) {
		rtgl_throwf(RT_OUT_OF_HOST_MEMORY, "failed to create OpenGL queue completion condition");
	}
	if (!queue->submit_lock) {
		rtgl_throwf(RT_OUT_OF_HOST_MEMORY, "failed to create OpenGL queue submission lock");
	}
	queue->capability = RT_QUEUE_GRAPHICS;
	queue->active = true;
}

void rtgl_queue_finish(struct rtgl_queue* queue) {
	if (queue->completion_condition) {
		rt_condition_destroy(queue->completion_condition);
	}
	if (queue->submit_lock) {
		rt_mutex_destroy(queue->submit_lock);
	}
	queue->completion_condition = NULL;
	queue->submit_lock = NULL;
	rtgl_finish_resource_base(RTGL_RESOURCE_BASE(queue));
}

struct rtgl_queue* rtgl_queue_create_virtual(struct rtgl_context* ctx, enum rt_queue_capability capability) {
	if (!ctx) {
		return NULL;
	}
	rtgl_execution_lock(ctx);
	if (ctx->queue_count == 256) {
		rtgl_execution_unlock(ctx);
		return NULL;
	}
	if (ctx->queue_count == ctx->queue_capacity) {
		u32 capacity = ctx->queue_capacity ? ctx->queue_capacity * 2 : 4;
		struct rtgl_queue** queues = (struct rtgl_queue**)realloc(ctx->queues, sizeof(*queues) * capacity);
		RTGL_CHECK_ALLOC(queues, sizeof(*queues) * capacity, "OpenGL virtual queue registry");
		if (!queues) {
			rtgl_execution_unlock(ctx);
			return NULL;
		}
		ctx->queues = queues;
		ctx->queue_capacity = capacity;
	}
	struct rtgl_queue* queue = rtgl_queue_create(ctx);
	if (queue) {
		queue->identifier = (u08)ctx->queue_count;
		queue->capability = capability;
		ctx->queues[ctx->queue_count++] = queue;
	}
	rtgl_execution_unlock(ctx);
	return queue;
}

struct rtgl_queue* rtgl_queue_from_timepoint(struct rtgl_context* ctx, rt_timepoint timepoint) {
	if (!ctx || timepoint.value == 0) {
		return NULL;
	}
	const u64 identifier = timepoint.value >> RTGL_TIMEPOINT_QUEUE_SHIFT;
	rtgl_execution_lock(ctx);
	struct rtgl_queue* queue = identifier < ctx->queue_count ? ctx->queues[identifier] : NULL;
	rtgl_execution_unlock(ctx);
	return queue;
}

rt_timepoint rtgl_queue_signal(struct rtgl_queue* queue) {
	rtgl_execution_lock(queue->base.ctx);
	const u64 value = ++queue->submitted_value;
	rtgl_execution_unlock(queue->base.ctx);
	return (rt_timepoint){ ((u64)queue->identifier << RTGL_TIMEPOINT_QUEUE_SHIFT) | value };
}

u64 rtgl_timepoint_queue_value(rt_timepoint timepoint) {
	return timepoint.value & RTGL_TIMEPOINT_VALUE_MASK;
}

void rtgl_queue_complete(struct rtgl_queue* queue, u64 value) {
	struct rtgl_context* ctx = queue->base.ctx;
	rtgl_execution_lock(ctx);
	if (queue->completed_value < value) {
		queue->completed_value = value;
		rt_condition_broadcast(queue->completion_condition);
	}
	rtgl_execution_unlock(ctx);
}

rt_timepoint rtgl_queue_present(struct rtgl_queue* queue, struct rtgl_swapchain* swapchain, struct rtgl_framebuffer* framebuffer) {
	return rtgl_execution_present(queue->base.ctx, queue, swapchain, framebuffer);
}

void rtgl_timepoint_wait(struct rtgl_context* ctx, rt_timepoint timepoint) {
	struct rtgl_queue* queue = rtgl_queue_from_timepoint(ctx, timepoint);
	const u64 value = rtgl_timepoint_queue_value(timepoint);
	if (!queue || value == 0) {
		return;
	}
	rtgl_execution_lock(ctx);
	while (queue->completed_value < value) {
		rt_condition_wait(queue->completion_condition, ctx->execution.work_lock);
	}
	rtgl_execution_unlock(ctx);
}

bool rtgl_timepoint_reached(struct rtgl_context* ctx, rt_timepoint timepoint) {
	struct rtgl_queue* queue = rtgl_queue_from_timepoint(ctx, timepoint);
	const u64 value = rtgl_timepoint_queue_value(timepoint);
	if (!queue || value == 0) {
		return true;
	}
	rtgl_execution_lock(ctx);
	const bool reached = queue->completed_value >= value;
	rtgl_execution_unlock(ctx);
	return reached;
}
