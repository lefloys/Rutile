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
	explicit rtdx_context(rtdx_context_flags context_flags);
	~rtdx_context();
	void initialize();

	IDXGIFactory6* dxgi_factory;
	IDXGIAdapter1* dxgi_adapter;
	ID3D12Device* d3d_device;
	ID3D12CommandQueue* d3d_graphics_queue;
	ID3D12Fence* d3d_graphics_fence;
	rt_event* graphics_fence_event;
	/* Owns the physical queue stream, its fence values, virtual-queue pending
	 * waits, and resource-state transitions emitted while command IR lowers. */
	rt_mutex* queue_lock;
	rtdx_queue* timepoint_queues[UINT8_MAX + 1];
	u08 next_queue_id;
	rtdx_context_flags flags;
};

extern rtdx_context* current_context;

rtdx_context* rtdx_get_current_context();
rtdx_context* rtdx_create_context(rtdx_context_flags flags);
void rtdx_context_report_validation(rtdx_context* ctx);

struct rtdx_physical_queue_scope {
	rtdx_context& context;

	explicit rtdx_physical_queue_scope(rtdx_context& value) : context(value) {
		rt_mutex_lock(context.queue_lock);
	}

	~rtdx_physical_queue_scope() {
		rt_mutex_unlock(context.queue_lock);
	}
};

template <typename T>
inline void rtdx_release(T*& object) {
	if (object) {
		object->Release();
		object = nullptr;
	}
}
