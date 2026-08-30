#pragma once

#include "rutile.hpp"
#include "error.hpp"

#include <atomic>
#include <concepts>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

struct rtd3d12_context;
struct rt_queue_t;

template <typename Derived>
struct rtd3d12_resource {
	rtd3d12_context* ctx;
	std::atomic<u32> ref_count;
	std::atomic<bool> zombie;

	explicit rtd3d12_resource(rtd3d12_context* context)
		: ctx(context), ref_count(1), zombie(false) {}

	void retain() {
		ref_count.fetch_add(1, std::memory_order_relaxed);
	}

	void release() {
		ref_count.fetch_sub(1, std::memory_order_relaxed);
		if (zombie.load(std::memory_order_relaxed) && ref_count.load(std::memory_order_relaxed) == 0) {
			delete static_cast<Derived*>(this);
		}
	}

	void retire() {
		zombie.store(true, std::memory_order_relaxed);
		release();
	}
};

rt::timepoint rtd3d12_queue_timepoint(rt_queue_t* queue, u64 value);
rt_queue_t* rtd3d12_queue_from_timepoint(rtd3d12_context* ctx, rt::timepoint timepoint);

namespace rtd3d12 {

void report_resource_allocation_failure();
std::byte* allocate_bytes(usize size);
std::byte* resize_bytes(std::byte* allocation, usize size);
void release_bytes(void* allocation);

template <typename T, typename... Args>
T* allocate(Args&&... args) {
	T* result = new (std::nothrow) T(std::forward<Args>(args)...);
	if (!result) {
		report_resource_allocation_failure();
	}
	return result;
}

template <typename T>
T* allocate_array(usize count) {
	T* result = new (std::nothrow) T[count]{};
	if (!result) {
		report_resource_allocation_failure();
	}
	return result;
}

template <typename T>
concept resource_type = requires(T& resource) {
	resource.retain();
	resource.release();
	resource.retire();
};

template <resource_type T>
T* create_resource(rtd3d12_context* ctx) {
	return allocate<T>(ctx);
}

} // namespace rtd3d12
