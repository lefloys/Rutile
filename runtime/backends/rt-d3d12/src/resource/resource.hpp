#pragma once

#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

#include <atomic>
#include <cstddef>
#include <new>
#include <type_traits>

struct rtdx_context;
struct rtdx_queue;

enum class rtdx_resource_type {
	unknown,
	buffer,
	command_buffer,
	framebuffer,
	graphics_program,
	queue,
	swapchain,
	texture,
	texture_view,
};

struct rtdx_resource_base {
	rtdx_resource_type type;
	rtdx_context* ctx;
	std::atomic<u32> ref_count;
	std::atomic<u32> job_count;
	std::atomic<bool> zombie;

	rtdx_resource_base(rtdx_context* context, rtdx_resource_type resource_type);
	virtual ~rtdx_resource_base();
	virtual void finish();
};

void rtdx_resource_retain(rtdx_resource_base* base);
void rtdx_resource_release(rtdx_resource_base* base);
void rtdx_resource_retire(rtdx_resource_base* base);
void rtdx_resource_finalize(rtdx_resource_base* base);
bool rtdx_resource_ready_to_destroy(const rtdx_resource_base& base);
rt_timepoint rtdx_queue_timepoint(rtdx_queue* queue, u64 value);
rtdx_queue* rtdx_queue_from_timepoint(rtdx_context* ctx, rt_timepoint timepoint);

template <typename T>
inline T* rtdx_new_resource(rtdx_context* ctx) {
	T* result = new (std::nothrow) T(ctx);
	if (!result) {
		rtdx_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for graphics resource", sizeof(T));
	}
	return result;
}

template <typename T>
inline void rtdx_delete_resource(T* resource) {
	delete resource;
}

#define RTDX_ALLOC_RESOURCE(type, context) (new (std::nothrow) type(context))
#define RTDX_FREE_RESOURCE(resource) (delete (resource))
template <typename T>
inline rtdx_resource_base* rtdx_resource_base_ptr(T* resource) {
	static_assert(std::is_base_of_v<rtdx_resource_base, T>, "resource must derive from rtdx_resource_base");
	return static_cast<rtdx_resource_base*>(resource);
}

#define RTDX_RESOURCE_BASE(resource) (rtdx_resource_base_ptr(resource))
#define rtdx_retain_resource(resource) rtdx_resource_retain(RTDX_RESOURCE_BASE(resource))
#define rtdx_release_resource(resource) rtdx_resource_release(RTDX_RESOURCE_BASE(resource))

#define RTDX_DECLARE_NEW_RESOURCE(type)                                                                            \
	inline rtdx_##type* rtdx_##type##_from_handle(rt_##type type) { return reinterpret_cast<rtdx_##type*>(type); } \
	inline rt_##type rtdx_##type##_to_handle(rtdx_##type* type) { return reinterpret_cast<rt_##type>(type); }      \
	rtdx_##type* rtdx_##type##_create(rtdx_context* ctx);                                                          \
	void rtdx_##type##_destroy(rtdx_context* ctx, rtdx_##type* type);

#define RTDX_DEFINE_RESOURCE_PRIVATE(type)                         \
	rtdx_##type* rtdx_##type##_create(rtdx_context* ctx) {         \
		return rtdx_new_resource<rtdx_##type>(ctx);               \
	}                                                              \
	void rtdx_##type##_destroy(rtdx_context*, rtdx_##type* type) { \
		if (!type) {                                               \
			return;                                                \
		}                                                          \
		rtdx_resource_retire(RTDX_RESOURCE_BASE(type));            \
	}
