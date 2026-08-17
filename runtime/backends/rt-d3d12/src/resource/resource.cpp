#include "resource.hpp"
#include "context.hpp"
#include "buffer.hpp"
#include "command_buffer.hpp"
#include "error.hpp"
#include "framebuffer.hpp"
#include "graphics_program.hpp"
#include "queue.hpp"
#include "resource/swapchain.hpp"
#include "texture.hpp"

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

 rtdx_resource_base::rtdx_resource_base(rtdx_context* context, rtdx_resource_type resource_type)
	: type(resource_type), ctx(context), ref_count(1), job_count(0), zombie(false) {
}

rtdx_resource_base::~rtdx_resource_base() {
	type = rtdx_resource_type::unknown;
	ctx = nullptr;
	zombie.store(true, std::memory_order_relaxed);
}

void rtdx_resource_base::finish() {
}

void rtdx_resource_finalize(rtdx_resource_base* base) {
	if (!base || base->type == rtdx_resource_type::unknown) {
		return;
	}
	base->finish();
	delete base;
}

static void rtdx_resource_try_free(rtdx_resource_base* base) {
	if (rtdx_resource_ready_to_destroy(*base)) {
		rtdx_resource_finalize(base);
	}
}

void rtdx_resource_retain(rtdx_resource_base* base) {
	base->ref_count.fetch_add(1, std::memory_order_relaxed);
}

void rtdx_resource_release(rtdx_resource_base* base) {
	base->ref_count.fetch_sub(1, std::memory_order_relaxed);
	rtdx_resource_try_free(base);
}

void rtdx_resource_retire(rtdx_resource_base* base) {
	base->zombie.store(true, std::memory_order_relaxed);
	rtdx_resource_release(base);
}

bool rtdx_resource_ready_to_destroy(const rtdx_resource_base& base) {
	return base.zombie.load(std::memory_order_relaxed) &&
		   base.ref_count.load(std::memory_order_relaxed) == 0 &&
		   base.job_count.load(std::memory_order_relaxed) == 0;
}

rt_timepoint rtdx_queue_timepoint(rtdx_queue* queue, u64 value) {
	if (!queue || !value) {
		return {};
	}
	return { ((u64)queue->timepoint_id << 56) | value };
}

rtdx_queue* rtdx_queue_from_timepoint(rtdx_context* ctx, rt_timepoint timepoint) {
	if (!ctx || !timepoint.value) {
		return NULL;
	}
	u08 id = (u08)(timepoint.value >> 56);
	return ctx->timepoint_queues[id];
}
