#include "context.h"
#include "error.h"

#include <stdlib.h>

struct rtsw_context* current_context;

struct rtsw_context* rtsw_get_current_context(void) {
	return current_context;
}

struct rtsw_context* rtsw_create_context(rtsw_context_flags flags) {
	struct rtsw_context* ctx = calloc(1, sizeof(*ctx));
	if (!ctx) {
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate rt-software context");
		return NULL;
	}

	ctx->flags = flags;
	return ctx;
}

void rtsw_context_destroy(struct rtsw_context* ctx) {
	if (!ctx) {
		return;
	}
	free(ctx);
}
