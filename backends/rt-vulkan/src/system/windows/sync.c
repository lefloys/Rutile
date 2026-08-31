#include "sync.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

_Static_assert(sizeof(SRWLOCK) <= RTVK_MUTEX_STORAGE_SIZE, "RTVK mutex storage is too small");
_Static_assert(_Alignof(SRWLOCK) <= _Alignof(struct rtvk_mutex), "RTVK mutex storage is under-aligned");
_Static_assert(sizeof(CONDITION_VARIABLE) <= RTVK_CONDITION_STORAGE_SIZE, "RTVK condition storage is too small");
_Static_assert(_Alignof(CONDITION_VARIABLE) <= _Alignof(struct rtvk_condition), "RTVK condition storage is under-aligned");

bool rtvk_mutex_init(struct rtvk_mutex* mutex) {
	InitializeSRWLock((PSRWLOCK)mutex->storage);
	return true;
}

void rtvk_mutex_finish(struct rtvk_mutex* mutex) {
	(void)mutex;
}

void rtvk_mutex_lock(struct rtvk_mutex* mutex) {
	AcquireSRWLockExclusive((PSRWLOCK)mutex->storage);
}

void rtvk_mutex_unlock(struct rtvk_mutex* mutex) {
	ReleaseSRWLockExclusive((PSRWLOCK)mutex->storage);
}

bool rtvk_condition_init(struct rtvk_condition* condition) {
	InitializeConditionVariable((PCONDITION_VARIABLE)condition->storage);
	return true;
}

void rtvk_condition_finish(struct rtvk_condition* condition) {
	(void)condition;
}

void rtvk_condition_wait(struct rtvk_condition* condition, struct rtvk_mutex* mutex) {
	SleepConditionVariableSRW((PCONDITION_VARIABLE)condition->storage, (PSRWLOCK)mutex->storage, INFINITE, 0);
}

void rtvk_condition_broadcast(struct rtvk_condition* condition) {
	WakeAllConditionVariable((PCONDITION_VARIABLE)condition->storage);
}
