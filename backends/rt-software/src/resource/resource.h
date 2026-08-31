#ifndef RTSW_RESOURCE_H
#define RTSW_RESOURCE_H

#include "config.h"
#include "rutile.h"

#include <stdbool.h>

struct rtsw_context;

typedef void (*rtsw_resource_destroy_proc)(void* resource);

struct rtsw_resource_base {
	struct rtsw_context* ctx;
	void* resource;
	rtsw_resource_destroy_proc destroy;
	u32 ref_count;
	u32 job_count;
	bool zombie;
	bool finalizing;
};

void* rtsw_alloc_resource(usize size);
void rtsw_init_resource_base(struct rtsw_context* ctx, struct rtsw_resource_base* base, void* resource, rtsw_resource_destroy_proc destroy);
void rtsw_resource_retain(struct rtsw_resource_base* base);
void rtsw_resource_release(struct rtsw_resource_base* base);
void rtsw_resource_job_begin(struct rtsw_resource_base* base);
void rtsw_resource_job_end(struct rtsw_resource_base* base);
void rtsw_resource_retire(struct rtsw_resource_base* base);

#define RTSW_ALLOC_RESOURCE(type) (type*)rtsw_alloc_resource(sizeof(type))
#define RTSW_RESOURCE_BASE(resource) ((struct rtsw_resource_base*)&(resource)->base)
#define rtsw_retain_resource(resource) rtsw_resource_retain(RTSW_RESOURCE_BASE(resource))
#define rtsw_release_resource(resource) rtsw_resource_release(RTSW_RESOURCE_BASE(resource))

#define RTSW_DECLARE_HANDLE(handle, type) \
	struct type* rtsw_##handle##_from_handle(rt_##handle handle); \
	rt_##handle rtsw_##handle##_to_handle(struct type* handle)

#define RTSW_DEFINE_HANDLE(handle, type) \
	struct type* rtsw_##handle##_from_handle(rt_##handle handle) { return (struct type*)handle; } \
	rt_##handle rtsw_##handle##_to_handle(struct type* handle) { return (rt_##handle)handle; }

#endif
