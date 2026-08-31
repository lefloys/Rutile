#ifndef RTSW_CONTEXT_H
#define RTSW_CONTEXT_H

#include "rutile.h"

typedef struct rtsw_context_flags {
	unsigned presentation : 1;
} rtsw_context_flags;

struct rtsw_error_state {
	enum rt_error code;
	char message[1024];
};

struct rtsw_queue;

struct rtsw_context {
	rtsw_context_flags flags;
	struct rtsw_error_state error;
	rt_output output;
	void* output_user_data;
	struct rtsw_queue* queues;
	u08 next_queue_identifier;
};

extern struct rtsw_context* current_context;

struct rtsw_context* rtsw_get_current_context(void);
struct rtsw_context* rtsw_create_context(rtsw_context_flags flags);
void rtsw_context_destroy(struct rtsw_context* ctx);

#endif
