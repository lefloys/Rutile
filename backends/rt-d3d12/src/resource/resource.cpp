#include "resource.hpp"
#include "context.hpp"
#include "error.hpp"
#include "queue.hpp"

#include <cstdlib>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

void rtd3d12::report_resource_allocation_failure() {
	rtd3d12_fail(rt::error::out_of_host_memory, "failed to allocate graphics resource");
}

std::byte* rtd3d12::allocate_bytes(usize size) {
	auto* result = static_cast<std::byte*>(std::malloc(size));
	if (!result) {
		report_resource_allocation_failure();
	}
	return result;
}

std::byte* rtd3d12::resize_bytes(std::byte* allocation, usize size) {
	auto* result = static_cast<std::byte*>(std::realloc(allocation, size));
	if (!result) {
		report_resource_allocation_failure();
	}
	return result;
}

void rtd3d12::release_bytes(void* allocation) {
	std::free(allocation);
}

rt::timepoint rtd3d12_queue_timepoint(rt_queue_t* queue, u64 value) {
	if (!queue || !value) {
		return {};
	}
	return { (static_cast<u64>(queue->timepoint_id) << 56) | value };
}

rt_queue_t* rtd3d12_queue_from_timepoint(rtd3d12_context* ctx, rt::timepoint timepoint) {
	if (!ctx || !timepoint.value) {
		return nullptr;
	}
	u08 id = static_cast<u08>(timepoint.value >> 56);
	return ctx->timepoint_queues[id];
}
