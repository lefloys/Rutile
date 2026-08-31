#ifndef RTVK_SYSTEM_SYNC_H
#define RTVK_SYSTEM_SYNC_H

#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

#define RTVK_MUTEX_STORAGE_SIZE 64
#define RTVK_CONDITION_STORAGE_SIZE 64

struct rtvk_mutex {
	union {
		uptr storage[RTVK_MUTEX_STORAGE_SIZE / sizeof(uptr)];
	};
};

struct rtvk_condition {
	union {
		uptr storage[RTVK_CONDITION_STORAGE_SIZE / sizeof(uptr)];
	};
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
