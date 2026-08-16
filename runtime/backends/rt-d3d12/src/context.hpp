#pragma once

#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

#include <d3d12.h>
#include <dxgi1_6.h>

#include "sync.h"

struct rtdx_context_flags {
	unsigned presentation : 1;
};

struct rtdx_queue;

struct rtdx_context {
	IDXGIFactory6* dxgi_factory;
	IDXGIAdapter1* dxgi_adapter;
	ID3D12Device* d3d_device;
	ID3D12CommandQueue* d3d_graphics_queue;
	ID3D12Fence* d3d_graphics_fence;
	rt_event* graphics_fence_event;
	/* Owns the physical queue stream, its fence values, virtual-queue pending
	 * waits, and resource-state transitions emitted while command IR lowers. */
	rt_mutex* queue_lock;
	rtdx_queue** queues;
	rtdx_queue* timepoint_queues[UINT8_MAX + 1];
	u64 next_fence_value;
	u32 queue_count;
	u08 next_queue_id;
	rtdx_context_flags flags;
	bool allow_tearing;
	bool shutting_down;
};

extern rtdx_context* current_context;

rtdx_context* rtdx_get_current_context();
rtdx_context* rtdx_create_context(rtdx_context_flags flags);
void rtdx_context_init(rtdx_context* ctx);
void rtdx_context_finish(rtdx_context* ctx);
void rtdx_context_destroy(rtdx_context* ctx);
void rtdx_context_report_validation(rtdx_context* ctx);

struct rtdx_physical_queue_scope {
	rtdx_context* ctx;

	explicit rtdx_physical_queue_scope(rtdx_context* context) : ctx(context) {
		rt_mutex_lock(ctx->queue_lock);
	}

	~rtdx_physical_queue_scope() {
		rt_mutex_unlock(ctx->queue_lock);
	}
};

template <typename T>
inline void rtdx_release(T** object) {
	if (*object) {
		(*object)->Release();
		*object = nullptr;
	}
}
