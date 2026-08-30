#include "system/sync.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <new>
#include <vector>

struct rt_event {
	HANDLE native;
};

struct rt_thread {
	HANDLE native;
	unsigned id;
};

struct rt_thread_start {
	rt_thread_proc proc;
	void* data;
};

static_assert(sizeof(SRWLOCK) == sizeof(void*));
static_assert(alignof(SRWLOCK) <= alignof(rtgl_mutex));
static_assert(sizeof(CONDITION_VARIABLE) == sizeof(void*));
static_assert(alignof(CONDITION_VARIABLE) <= alignof(rtgl_condition));

static DWORD WINAPI rt_thread_entry(void* data) {
	rt_thread_start* start = static_cast<rt_thread_start*>(data);
	rt_thread_proc proc = start->proc;
	void* user_data = start->data;
	delete start;
	return proc(user_data);
}

extern "C" {

struct rt_event* rt_event_create(bool manual_reset, bool initial_state) {
	rt_event* event = new (std::nothrow) rt_event;
	if (!event) {
		return nullptr;
	}
	event->native = CreateEventW(nullptr, manual_reset, initial_state, nullptr);
	if (!event->native) {
		delete event;
		return nullptr;
	}
	return event;
}

void rt_event_destroy(struct rt_event* event) {
	if (!event) {
		return;
	}
	CloseHandle(event->native);
	delete event;
}

void rt_event_signal(struct rt_event* event) {
	SetEvent(event->native);
}

void rt_event_reset(struct rt_event* event) {
	ResetEvent(event->native);
}

void rt_event_wait(struct rt_event* event) {
	WaitForSingleObject(event->native, INFINITE);
}

u32 rt_event_wait_any(struct rt_event** events, u32 count) {
	if (!count) {
		return 0;
	}
	std::vector<HANDLE> handles(count);
	for (u32 index = 0; index < count; index++) {
		handles[index] = events[index]->native;
	}
	return (u32)(WaitForMultipleObjects(count, handles.data(), FALSE, INFINITE) - WAIT_OBJECT_0);
}

void rtgl_mutex_init(struct rtgl_mutex* mutex) {
	*mutex = {};
	InitializeSRWLock(reinterpret_cast<PSRWLOCK>(&mutex->critical_section.native));
}

void rtgl_mutex_finish(struct rtgl_mutex*) {
}

void rtgl_mutex_lock(struct rtgl_mutex* mutex) {
	AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&mutex->critical_section.native));
}

void rtgl_mutex_unlock(struct rtgl_mutex* mutex) {
	ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&mutex->critical_section.native));
}

void rtgl_condition_init(struct rtgl_condition* condition) {
	*condition = {};
	InitializeConditionVariable(reinterpret_cast<PCONDITION_VARIABLE>(&condition->native));
}

void rtgl_condition_finish(struct rtgl_condition*) {
}

void rtgl_condition_wait(struct rtgl_condition* condition, struct rtgl_mutex* mutex) {
	SleepConditionVariableSRW(
		reinterpret_cast<PCONDITION_VARIABLE>(&condition->native),
		reinterpret_cast<PSRWLOCK>(&mutex->critical_section.native),
		INFINITE,
		0
	);
}

void rtgl_condition_broadcast(struct rtgl_condition* condition) {
	WakeAllConditionVariable(reinterpret_cast<PCONDITION_VARIABLE>(&condition->native));
}

struct rt_thread* rt_thread_create(rt_thread_proc proc, void* data) {
	rt_thread* thread = new (std::nothrow) rt_thread;
	rt_thread_start* start = new (std::nothrow) rt_thread_start{ proc, data };
	if (!thread || !start) {
		delete start;
		delete thread;
		return nullptr;
	}
	DWORD id = 0;
	thread->native = CreateThread(nullptr, 0, rt_thread_entry, start, 0, &id);
	thread->id = id;
	if (!thread->native) {
		delete start;
		delete thread;
		return nullptr;
	}
	return thread;
}

void rt_thread_join(struct rt_thread* thread) {
	if (!thread) {
		return;
	}
	WaitForSingleObject(thread->native, INFINITE);
	CloseHandle(thread->native);
	delete thread;
}

unsigned rt_thread_id(struct rt_thread* thread) {
	return thread ? thread->id : 0;
}

unsigned rt_current_thread_id(void) {
	return GetCurrentThreadId();
}

}
