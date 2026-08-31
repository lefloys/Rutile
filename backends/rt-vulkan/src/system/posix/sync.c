#include "sync.h"

#include <pthread.h>

_Static_assert(sizeof(pthread_mutex_t) <= RTVK_MUTEX_STORAGE_SIZE, "RTVK mutex storage is too small");
_Static_assert(_Alignof(pthread_mutex_t) <= _Alignof(struct rtvk_mutex), "RTVK mutex storage is under-aligned");
_Static_assert(sizeof(pthread_cond_t) <= RTVK_CONDITION_STORAGE_SIZE, "RTVK condition storage is too small");
_Static_assert(_Alignof(pthread_cond_t) <= _Alignof(struct rtvk_condition), "RTVK condition storage is under-aligned");

bool rtvk_mutex_init(struct rtvk_mutex* mutex) {
	return pthread_mutex_init((pthread_mutex_t*)mutex->storage, NULL) == 0;
}

void rtvk_mutex_finish(struct rtvk_mutex* mutex) {
	pthread_mutex_destroy((pthread_mutex_t*)mutex->storage);
}

void rtvk_mutex_lock(struct rtvk_mutex* mutex) {
	pthread_mutex_lock((pthread_mutex_t*)mutex->storage);
}

void rtvk_mutex_unlock(struct rtvk_mutex* mutex) {
	pthread_mutex_unlock((pthread_mutex_t*)mutex->storage);
}

bool rtvk_condition_init(struct rtvk_condition* condition) {
	return pthread_cond_init((pthread_cond_t*)condition->storage, NULL) == 0;
}

void rtvk_condition_finish(struct rtvk_condition* condition) {
	pthread_cond_destroy((pthread_cond_t*)condition->storage);
}

void rtvk_condition_wait(struct rtvk_condition* condition, struct rtvk_mutex* mutex) {
	pthread_cond_wait((pthread_cond_t*)condition->storage, (pthread_mutex_t*)mutex->storage);
}

void rtvk_condition_broadcast(struct rtvk_condition* condition) {
	pthread_cond_broadcast((pthread_cond_t*)condition->storage);
}
