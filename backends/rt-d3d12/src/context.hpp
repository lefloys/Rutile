#pragma once

#include "rutile.hpp"

#include <d3d12.h>
#include <dxgi1_6.h>
#include "sync.hpp"

#include <chrono>
#include <mutex>

struct rtd3d12_context_flags {
	unsigned presentation : 1;
};

struct rt_queue_t;
struct rtd3d12_context {
	explicit rtd3d12_context(rtd3d12_context_flags context_flags);
	~rtd3d12_context();
	static rtd3d12_context* create(rtd3d12_context_flags flags);
	void initialize();
	bool create_factory();
	bool pick_adapter();
	bool create_device();
	void destroy_queues();
	void report_validation();
	void log_startup_time(std::chrono::steady_clock::time_point start);

	IDXGIFactory6* dxgi_factory{};
	IDXGIAdapter1* dxgi_adapter{};
	ID3D12Device* d3d_device{};
	ID3D12CommandQueue* d3d_graphics_queue{};
	ID3D12Fence* d3d_graphics_fence{};
	rtd3d12_event graphics_fence_event{ false, false };
	/* Owns the physical queue stream, its fence values, virtual-queue pending
	 * waits, and resource-state transitions emitted while command IR lowers. */
	std::mutex queue_lock;
	rt_queue_t* timepoint_queues[UINT8_MAX + 1]{};
	u08 next_queue_id{};
	rtd3d12_context_flags flags;
};

extern rtd3d12_context* current_context;

rtd3d12_context* rtd3d12_get_current_context();
