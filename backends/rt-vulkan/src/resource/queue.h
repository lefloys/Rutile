#ifndef RTVK_QUEUE_H
#define RTVK_QUEUE_H

#include "command_buffer.h"
#include "config.h"
#include "resource.h"
#include "sync.h"

#include <vk_mem_alloc.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTVK_API rt_queue rtQueueCreate(enum rt_queue_capability capability);
RTVK_API void rtQueueDestroy(rt_queue queue);
RTVK_API rt_timepoint rtQueueSubmit(rt_queue queue, rt_command_buffer command_buffer);
RTVK_API void rtQueueWait(rt_queue queue, rt_timepoint timepoint);
RTVK_API rt_timepoint rtQueueFlush(rt_queue queue);
RTVK_API void rtTimepointWait(rt_timepoint timepoint);
RTVK_API bool rtTimepointReached(rt_timepoint timepoint);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtvk_submitted_batch {
	struct rtvk_lowered_command_buffer* lowered_command_buffer;
	struct rtvk_submitted_batch* next;
	rt_timepoint* wait_timepoints;
	VkSemaphore* binary_waits;
	VkSemaphore* binary_signals;
	usize wait_count;
	usize binary_wait_count;
	usize binary_wait_capacity;
	usize binary_signal_count;
	usize binary_signal_capacity;
	u64 first_value;
	u64 value;
};

struct rtvk_retired_upload_resource {
	struct rtvk_retired_upload_resource* next;
	rt_timepoint timepoint;
	VkCommandPool command_pool;
	VkBuffer staging_buffer;
	VmaAllocation staging_allocation;
};

struct rtvk_queue {
	struct rtvk_resource_base base;

	VkQueue vk_queue;
	VkQueueFlags capabilities;
	struct rtvk_mutex lock;
	VkSemaphore vk_timeline;
	VkCommandPool copy_command_pool;
	VkCommandBuffer copy_command_buffer;
	rt_timepoint copy_command_timepoint;
	VkCommandPool upload_command_pool;
	VkCommandBuffer upload_command_buffer;
	VkBuffer upload_staging_buffer;
	VmaAllocation upload_staging_allocation;
	u64 upload_staging_size;
	rt_timepoint upload_command_timepoint;

	struct rtvk_submitted_batch* submitted_head;
	struct rtvk_submitted_batch* submitted_tail;
	struct rtvk_submitted_batch* pending_head;
	struct rtvk_submitted_batch* pending_tail;
	struct rtvk_retired_upload_resource* retired_uploads;
	u64 timeline_value;
	u64 submitted_value;
	u64 completed_value;
	enum rt_queue_capability capability;
	u32 family_index;
	u32 queue_index;
	u08 timepoint_id;
};

struct rtvk_virtual_queue {
	rt_timepoint* wait_timepoints;
	struct rtvk_queue* native_queue;
	struct rtvk_mutex lock;
	usize wait_capacity;
	usize wait_count;
};

RTVK_DECLARE_HANDLE(queue, rtvk_virtual_queue)
rt_timepoint rtvk_virtual_queue_submit(struct rtvk_context* ctx, struct rtvk_virtual_queue* queue, struct rtvk_command_buffer* command_buffer);
rt_timepoint rtvk_virtual_queue_flush(struct rtvk_context* ctx, struct rtvk_virtual_queue* queue);
void rtvk_virtual_queue_wait(struct rtvk_context* ctx, struct rtvk_virtual_queue* queue, rt_timepoint timepoint);
void rtvk_timepoint_wait_public(struct rtvk_context* ctx, rt_timepoint timepoint);
bool rtvk_timepoint_reached_public(struct rtvk_context* ctx, rt_timepoint timepoint);

struct rtvk_queue* rtvk_queue_create(struct rtvk_context* ctx, VkQueue vk_queue, VkQueueFlags capabilities, enum rt_queue_capability capability, u32 family_index, u32 queue_index);
void rtvk_queue_destroy(struct rtvk_context* ctx, struct rtvk_queue* queue);
void rtvk_queue_init(struct rtvk_context* ctx, struct rtvk_queue* queue, VkQueue vk_queue, VkQueueFlags capabilities, enum rt_queue_capability capability, u32 family_index, u32 queue_index);
void rtvk_queue_finish(struct rtvk_queue* queue);
struct rtvk_queue* rtvk_queue_query(struct rtvk_context* ctx, enum rt_queue_capability capability);
struct rtvk_lowered_command_buffer* rtvk_queue_create_lowered_command_buffer(struct rtvk_context* ctx, struct rtvk_queue* queue);
struct rtvk_virtual_queue* rtvk_virtual_queue_create(struct rtvk_context* ctx, enum rt_queue_capability capability);
void rtvk_virtual_queue_destroy(struct rtvk_virtual_queue* queue);
struct rtvk_queue* rtvk_queue_query_present(struct rtvk_context* ctx, VkSurfaceKHR surface);
VkPipelineStageFlags rtvk_queue_wait_stage(struct rtvk_queue* queue);
u64 rtvk_queue_completed_value(struct rtvk_context* ctx, struct rtvk_queue* queue);
void rtvk_queue_collect_to_value(struct rtvk_context* ctx, struct rtvk_queue* queue, u64 completed_value);
void rtvk_queue_lock_pair(struct rtvk_queue* first, struct rtvk_queue* second);
void rtvk_queue_unlock_pair(struct rtvk_queue* first, struct rtvk_queue* second);
rt_timepoint rtvk_queue_submit(struct rtvk_context* ctx, struct rtvk_queue* queue, struct rtvk_command_buffer* command_buffer, rt_timepoint** wait_timepoints, usize* wait_count, usize* wait_capacity);
rt_timepoint rtvk_queue_submit_lowered(struct rtvk_context* ctx, struct rtvk_queue* queue, struct rtvk_lowered_command_buffer* lowered, rt_timepoint* wait_timepoints, usize wait_count);
rt_timepoint rtvk_queue_flush(struct rtvk_context* ctx, struct rtvk_queue* queue);
rt_timepoint rtvk_queue_flush_locked(struct rtvk_context* ctx, struct rtvk_queue* queue);
rt_timepoint rtvk_queue_submit_immediate(struct rtvk_context* ctx, struct rtvk_queue* queue, VkCommandBuffer command_buffer);
rt_timepoint rtvk_queue_submit_immediate_locked(struct rtvk_context* ctx, struct rtvk_queue* queue, VkCommandBuffer command_buffer);
rt_timepoint rtvk_queue_signal(struct rtvk_context* ctx, struct rtvk_queue* queue);
rt_timepoint rtvk_queue_wait_binary(struct rtvk_context* ctx, struct rtvk_queue* queue, VkSemaphore semaphore);
bool rtvk_queue_signal_binary_on_next_flush(struct rtvk_queue* queue, VkSemaphore semaphore);
rt_timepoint rtvk_queue_signal_binary_after_timepoint(struct rtvk_queue* queue, u64 wait_value, VkSemaphore semaphore);
void rtvk_queue_retire_upload_resources(struct rtvk_context* ctx, struct rtvk_queue* queue, bool command, bool staging);
void rtvk_timepoint_wait(struct rtvk_context* ctx, rt_timepoint timepoint);
bool rtvk_timepoint_reached(struct rtvk_context* ctx, rt_timepoint timepoint);

#endif
