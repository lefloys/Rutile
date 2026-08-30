#ifndef RTVK_WINDOWS_SYNC_H
#define RTVK_WINDOWS_SYNC_H

#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

struct rtvk_mutex {
	SRWLOCK native;
};

struct rtvk_condition {
	CONDITION_VARIABLE native;
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
