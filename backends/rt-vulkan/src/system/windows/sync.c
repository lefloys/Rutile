#include "sync.h"

bool rtvk_mutex_init(struct rtvk_mutex* mutex) {
	InitializeSRWLock(&mutex->native);
	return true;
}

void rtvk_mutex_finish(struct rtvk_mutex* mutex) {
	(void)mutex;
}

void rtvk_mutex_lock(struct rtvk_mutex* mutex) {
	AcquireSRWLockExclusive(&mutex->native);
}

void rtvk_mutex_unlock(struct rtvk_mutex* mutex) {
	ReleaseSRWLockExclusive(&mutex->native);
}

bool rtvk_condition_init(struct rtvk_condition* condition) {
	InitializeConditionVariable(&condition->native);
	return true;
}

void rtvk_condition_finish(struct rtvk_condition* condition) {
	(void)condition;
}

void rtvk_condition_wait(struct rtvk_condition* condition, struct rtvk_mutex* mutex) {
	SleepConditionVariableSRW(&condition->native, &mutex->native, INFINITE, 0);
}

void rtvk_condition_broadcast(struct rtvk_condition* condition) {
	WakeAllConditionVariable(&condition->native);
}
