#ifndef RTVK_POSIX_SYNC_H
#define RTVK_POSIX_SYNC_H

#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

#include <pthread.h>

struct rtvk_mutex {
	pthread_mutex_t native;
};

struct rtvk_condition {
	pthread_cond_t native;
};

bool rtvk_mutex_init(struct rtvk_mutex* mutex);
void rtvk_mutex_finish(struct rtvk_mutex* mutex);
void rtvk_mutex_lock(struct rtvk_mutex* mutex);
void rtvk_mutex_unlock(struct rtvk_mutex* mutex);

bool rtvk_condition_init(struct rtvk_condition* condition);
void rtvk_condition_finish(struct rtvk_condition* condition);
void rtvk_condition_wait(struct rtvk_condition* condition, struct rtvk_mutex* mutex);
void rtvk_condition_broadcast(struct rtvk_condition* condition);

#endif
