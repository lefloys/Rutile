#include "sync.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <process.h>
#include <stdlib.h>
#include <windows.h>

struct rt_event {
	HANDLE handle;
};

struct rt_mutex {
	CRITICAL_SECTION critical_section;
};

struct rt_condition {
	CONDITION_VARIABLE condition;
};

struct rt_thread {
	HANDLE handle;
	unsigned id;
};

struct rt_thread_start {
	rt_thread_proc proc;
	void* data;
};

static unsigned __stdcall rt_thread_entry(void* data) {
	struct rt_thread_start* start = (struct rt_thread_start*)data;
	rt_thread_proc proc = start->proc;
	void* proc_data = start->data;
	free(start);
	return proc(proc_data);
}

struct rt_event* rt_event_create(bool manual_reset, bool initial_state) {
	struct rt_event* event = calloc(1, sizeof(*event));
	if (!event) {
		return NULL;
	}
	event->handle = CreateEventA(NULL, manual_reset ? TRUE : FALSE, initial_state ? TRUE : FALSE, NULL);
	if (!event->handle) {
		free(event);
		return NULL;
	}
	return event;
}

void rt_event_destroy(struct rt_event* event) {
	if (!event) {
		return;
	}
	CloseHandle(event->handle);
	free(event);
}

void rt_event_signal(struct rt_event* event) {
	SetEvent(event->handle);
}

void rt_event_reset(struct rt_event* event) {
	ResetEvent(event->handle);
}

void rt_event_wait(struct rt_event* event) {
	WaitForSingleObject(event->handle, INFINITE);
}

u32 rt_event_wait_any(struct rt_event** events, u32 count) {
	HANDLE handles[16];
	DWORD result;

	if (count > 16) {
		return count;
	}
	for (u32 i = 0; i < count; ++i) {
		handles[i] = events[i]->handle;
	}
	result = WaitForMultipleObjects(count, handles, FALSE, INFINITE);
	if (result < WAIT_OBJECT_0 || result >= WAIT_OBJECT_0 + count) {
		return count;
	}
	return (u32)(result - WAIT_OBJECT_0);
}

void* rt_event_native_handle(struct rt_event* event) {
	return event ? event->handle : NULL;
}

void rt_native_wait_handle_wait(void* handle) {
	if (handle) {
		WaitForSingleObject((HANDLE)handle, INFINITE);
	}
}

void rt_native_wait_handle_close(void* handle) {
	if (handle) {
		CloseHandle((HANDLE)handle);
	}
}

struct rt_mutex* rt_mutex_create(void) {
	struct rt_mutex* mutex = calloc(1, sizeof(*mutex));
	if (!mutex) {
		return NULL;
	}
	InitializeCriticalSection(&mutex->critical_section);
	return mutex;
}

usize rt_mutex_allocation_size(void) {
	return sizeof(struct rt_mutex);
}

void rt_mutex_destroy(struct rt_mutex* mutex) {
	if (!mutex) {
		return;
	}
	DeleteCriticalSection(&mutex->critical_section);
	free(mutex);
}

void rt_mutex_lock(struct rt_mutex* mutex) {
	EnterCriticalSection(&mutex->critical_section);
}

void rt_mutex_unlock(struct rt_mutex* mutex) {
	LeaveCriticalSection(&mutex->critical_section);
}

struct rt_condition* rt_condition_create(void) {
	struct rt_condition* condition = calloc(1, sizeof(*condition));
	if (!condition) {
		return NULL;
	}
	InitializeConditionVariable(&condition->condition);
	return condition;
}

void rt_condition_destroy(struct rt_condition* condition) {
	free(condition);
}

void rt_condition_wait(struct rt_condition* condition, struct rt_mutex* mutex) {
	SleepConditionVariableCS(&condition->condition, &mutex->critical_section, INFINITE);
}

void rt_condition_broadcast(struct rt_condition* condition) {
	WakeAllConditionVariable(&condition->condition);
}

struct rt_thread* rt_thread_create(rt_thread_proc proc, void* data) {
	struct rt_thread* thread = calloc(1, sizeof(*thread));
	struct rt_thread_start* start = calloc(1, sizeof(*start));
	if (!thread || !start) {
		free(start);
		free(thread);
		return NULL;
	}
	start->proc = proc;
	start->data = data;
	thread->handle = (HANDLE)_beginthreadex(NULL, 0, rt_thread_entry, start, 0, &thread->id);
	if (!thread->handle) {
		free(start);
		free(thread);
		return NULL;
	}
	return thread;
}

void rt_thread_join(struct rt_thread* thread) {
	if (!thread) {
		return;
	}
	WaitForSingleObject(thread->handle, INFINITE);
	CloseHandle(thread->handle);
	free(thread);
}

unsigned rt_thread_id(struct rt_thread* thread) {
	return thread ? thread->id : 0;
}

unsigned rt_current_thread_id(void) {
	return GetCurrentThreadId();
}
