#include "resource.h"
#include "error.h"

#include <assert.h>
#include <stdlib.h>

void* rtsw_alloc_resource(usize size) {
	void* resource = calloc(1, size);
	if (!resource) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for graphics resource", size);
	}
	return resource;
}

void rtsw_init_resource_base(struct rtsw_context* ctx, struct rtsw_resource_base* base, void* resource, rtsw_resource_destroy_proc destroy) {
	assert(base);
	assert(resource);
	assert(destroy);
	base->ctx = ctx;
	base->resource = resource;
	base->destroy = destroy;
	base->ref_count = 1;
}

static void rtsw_resource_try_free(struct rtsw_resource_base* base) {
	if (base->zombie && base->ref_count == 0 && base->job_count == 0 && !base->finalizing) {
		base->finalizing = true;
		base->destroy(base->resource);
	}
}

void rtsw_resource_retain(struct rtsw_resource_base* base) {
	assert(base);
	++base->ref_count;
}

void rtsw_resource_release(struct rtsw_resource_base* base) {
	assert(base);
	assert(base->ref_count > 0);
	--base->ref_count;
	rtsw_resource_try_free(base);
}

void rtsw_resource_job_begin(struct rtsw_resource_base* base) {
	assert(base);
	++base->job_count;
}

void rtsw_resource_job_end(struct rtsw_resource_base* base) {
	assert(base);
	assert(base->job_count > 0);
	--base->job_count;
	rtsw_resource_try_free(base);
}

void rtsw_resource_retire(struct rtsw_resource_base* base) {
	assert(base);
	base->zombie = true;
	rtsw_resource_release(base);
}
