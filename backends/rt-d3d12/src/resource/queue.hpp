#pragma once

#include "config.hpp"
#include "resource.hpp"
#include "sync.hpp"

#include <d3d12.h>
#include <mutex>

RTD3D12_API rt_queue_t* rtQueueCreate(rt::queue_capability capability);
RTD3D12_API void rtQueueDestroy(rt_queue_t* queue);
RTD3D12_API rt::timepoint rtQueueSubmit(rt_queue_t* queue, rt_command_buffer_t* command_buffer);
RTD3D12_API void rtQueueWait(rt_queue_t* queue, rt::timepoint timepoint);
RTD3D12_API rt::timepoint rtQueueFlush(rt_queue_t* queue);
RTD3D12_API void rtTimepointWait(rt::timepoint timepoint);
RTD3D12_API bool rtTimepointReached(rt::timepoint timepoint);

struct rt_command_buffer_t;

struct rtd3d12_submitted_batch {
	ID3D12CommandAllocator* d3d_allocator;
	ID3D12GraphicsCommandList* d3d_command_list;
	ID3D12DescriptorHeap* d3d_resource_heap;
	ID3D12DescriptorHeap* d3d_sampler_heap;
	rt_command_buffer_t* command_snapshot;
	rtd3d12_submitted_batch* next;
	u64 value;
};

struct rt_queue_t : rtd3d12_resource<rt_queue_t> {
	explicit rt_queue_t(rtd3d12_context* ctx) : rtd3d12_resource(ctx) {}
	~rt_queue_t();
	static rt_queue_t* create(rtd3d12_context* ctx, rt::queue_capability capability);
	void destroy();
	bool initialize(rt::queue_capability capability);

	ID3D12CommandQueue* d3d_queue{};
	ID3D12Fence* d3d_fence{};
	ID3D12CommandAllocator* upload_allocator{};
	ID3D12GraphicsCommandList* upload_command_list{};
	ID3D12Resource* upload_buffer{};
	std::mutex upload_lock;

	rt::timepoint wait_timepoints[8]{};
	rtd3d12_submitted_batch* submitted_head{};
	rtd3d12_submitted_batch* submitted_tail{};

	u64 fence_value{};
	u64 upload_fence_value{};
	u64 upload_buffer_size{};
	rt::queue_capability capability{};
	u32 wait_count{};
	u08 timepoint_id{};
};

rt_queue_t* rtd3d12_context_queue(rtd3d12_context* ctx, rt::queue_capability capability);
void rtd3d12_queue_wait(rtd3d12_context* ctx, rt_queue_t* queue, rt::timepoint timepoint);
rt::timepoint rtd3d12_queue_submit(rtd3d12_context* ctx, rt_queue_t* queue, rt_command_buffer_t* command_buffer);
rt::timepoint rtd3d12_queue_submit_locked(rtd3d12_context* ctx, rt_queue_t* queue, rt_command_buffer_t* command_buffer);
rt::timepoint rtd3d12_queue_flush(rtd3d12_context* ctx, rt_queue_t* queue);
rt::timepoint rtd3d12_queue_signal(rtd3d12_context* ctx, rt_queue_t* queue);
void rtd3d12_queue_wait_idle(rtd3d12_context* ctx, rt_queue_t* queue);
void rtd3d12_queue_collect(rtd3d12_context* ctx, rt_queue_t* queue);
void rtd3d12_queue_collect_locked(rtd3d12_context* ctx, rt_queue_t* queue);
bool rtd3d12_queue_acquire_upload_command(rtd3d12_context* ctx, rt_queue_t* queue);
void rtd3d12_wait_for_timepoint(rtd3d12_context* ctx, rt::timepoint timepoint);
bool rtd3d12_is_timepoint_reached(rtd3d12_context* ctx, rt::timepoint timepoint);
