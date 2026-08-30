#include "sync.h"

bool rtvk_mutex_init(struct rtvk_mutex* mutex) {
	return pthread_mutex_init(&mutex->native, NULL) == 0;
}

void rtvk_mutex_finish(struct rtvk_mutex* mutex) {
	pthread_mutex_destroy(&mutex->native);
}

void rtvk_mutex_lock(struct rtvk_mutex* mutex) {
	pthread_mutex_lock(&mutex->native);
}

void rtvk_mutex_unlock(struct rtvk_mutex* mutex) {
	pthread_mutex_unlock(&mutex->native);
}

bool rtvk_condition_init(struct rtvk_condition* condition) {
	return pthread_cond_init(&condition->native, NULL) == 0;
}

void rtvk_condition_finish(struct rtvk_condition* condition) {
	pthread_cond_destroy(&condition->native);
}

void rtvk_condition_wait(struct rtvk_condition* condition, struct rtvk_mutex* mutex) {
	pthread_cond_wait(&condition->native, &mutex->native);
}

void rtvk_condition_broadcast(struct rtvk_condition* condition) {
	pthread_cond_broadcast(&condition->native);
}
