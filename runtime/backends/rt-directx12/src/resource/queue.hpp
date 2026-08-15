#pragma once

#include "config.hpp"
#include "resource.hpp"
#include "sync.h"

#include <d3d12.h>

RTDX_API rt_queue rtQueueCreate(rt_queue_capability capability);
RTDX_API void rtQueueDestroy(rt_queue queue);
RTDX_API rt_timepoint rtQueueSubmit(rt_queue queue, rt_command_buffer command_buffer);
RTDX_API void rtQueueWait(rt_queue queue, rt_timepoint timepoint);
RTDX_API rt_timepoint rtQueueFlush(rt_queue queue);
RTDX_API void rtTimepointWait(rt_timepoint timepoint);
RTDX_API bool rtTimepointReached(rt_timepoint timepoint);

struct rtdx_command_buffer;

struct rtdx_submitted_batch {
	ID3D12CommandAllocator* d3d_allocator;
	ID3D12GraphicsCommandList* d3d_command_list;
	ID3D12DescriptorHeap* d3d_resource_heap;
	ID3D12DescriptorHeap* d3d_sampler_heap;
	rtdx_command_buffer* command_buffer;
	rtdx_submitted_batch* next;
	u64 value;
};

struct rtdx_queue {
	rtdx_resource_base base;

	ID3D12CommandQueue* d3d_queue;
	ID3D12Fence* d3d_fence;
	ID3D12CommandAllocator* upload_allocator;
	ID3D12GraphicsCommandList* upload_command_list;
	ID3D12Resource* upload_buffer;
	rt_event* fence_event;
	rt_mutex* upload_lock;

	rt_timepoint wait_timepoints[8];
	rtdx_submitted_batch* submitted_head;
	rtdx_submitted_batch* submitted_tail;

	u64 fence_value;
	u64 upload_fence_value;
	u64 upload_buffer_size;
	rt_queue_capability capability;
	u32 wait_count;
	u08 timepoint_id;
};

struct rtdx_queue_upload_scope {
	rtdx_queue* queue;

	explicit rtdx_queue_upload_scope(rtdx_queue* value) : queue(value) {
		rt_mutex_lock(queue->upload_lock);
	}

	~rtdx_queue_upload_scope() {
		rt_mutex_unlock(queue->upload_lock);
	}
};

inline rtdx_queue* rtdx_queue_from_handle(rt_queue queue) { return reinterpret_cast<rtdx_queue*>(queue); }
inline rt_queue rtdx_queue_to_handle(rtdx_queue* queue) { return reinterpret_cast<rt_queue>(queue); }

rtdx_queue* rtdx_queue_create(rtdx_context* ctx, rt_queue_capability capability);
void rtdx_queue_destroy(rtdx_context* ctx, rtdx_queue* queue);
bool rtdx_queue_init(rtdx_context* ctx, rtdx_queue* queue, rt_queue_capability capability);
void rtdx_queue_finish(rtdx_context* ctx, rtdx_queue* queue);
rtdx_queue* rtdx_context_queue(rtdx_context* ctx, rt_queue_capability capability);
void rtdx_queue_wait(rtdx_context* ctx, rtdx_queue* queue, rt_timepoint timepoint);
rt_timepoint rtdx_queue_submit(rtdx_context* ctx, rtdx_queue* queue, rtdx_command_buffer* command_buffer);
rt_timepoint rtdx_queue_submit_locked(rtdx_context* ctx, rtdx_queue* queue, rtdx_command_buffer* command_buffer);
rt_timepoint rtdx_queue_flush(rtdx_context* ctx, rtdx_queue* queue);
rt_timepoint rtdx_queue_signal(rtdx_context* ctx, rtdx_queue* queue);
void rtdx_queue_wait_idle(rtdx_context* ctx, rtdx_queue* queue);
void rtdx_queue_collect(rtdx_context* ctx, rtdx_queue* queue);
void rtdx_queue_collect_locked(rtdx_context* ctx, rtdx_queue* queue);
bool rtdx_queue_acquire_upload_command(rtdx_context* ctx, rtdx_queue* queue);
void rtdx_wait_for_timepoint(rtdx_context* ctx, rt_timepoint timepoint);
bool rtdx_is_timepoint_reached(rtdx_context* ctx, rt_timepoint timepoint);
