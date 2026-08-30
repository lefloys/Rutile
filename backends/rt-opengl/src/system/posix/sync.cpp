#include "system/sync.h"

#include <pthread.h>
#include <time.h>

#include <functional>
#include <new>

struct rt_event {
	pthread_mutex_t mutex;
	pthread_cond_t condition;
	bool manual_reset;
	bool signaled;
};

struct rt_thread {
	pthread_t native;
	unsigned id;
};

struct rt_thread_start {
	rt_thread_proc proc;
	void* data;
};

static_assert(sizeof(pthread_mutex_t) <= RTGL_MUTEX_STORAGE_SIZE);
static_assert(alignof(pthread_mutex_t) <= alignof(uptr));
static_assert(sizeof(pthread_cond_t) <= RTGL_CONDITION_STORAGE_SIZE);
static_assert(alignof(pthread_cond_t) <= alignof(uptr));

static pthread_mutex_t* rtgl_mutex_native(struct rtgl_mutex* mutex) {
	return reinterpret_cast<pthread_mutex_t*>(mutex->storage);
}

static pthread_cond_t* rtgl_condition_native(struct rtgl_condition* condition) {
	return reinterpret_cast<pthread_cond_t*>(condition->storage);
}

static void* rt_thread_entry(void* data) {
	rt_thread_start* start = static_cast<rt_thread_start*>(data);
	rt_thread_proc proc = start->proc;
	void* user_data = start->data;
	delete start;
	return reinterpret_cast<void*>((uptr)proc(user_data));
}

static bool rt_event_try_consume(struct rt_event* event) {
	pthread_mutex_lock(&event->mutex);
	bool signaled = event->signaled;
	if (signaled && !event->manual_reset) {
		event->signaled = false;
	}
	pthread_mutex_unlock(&event->mutex);
	return signaled;
}

extern "C" {

struct rt_event* rt_event_create(bool manual_reset, bool initial_state) {
	rt_event* event = new (std::nothrow) rt_event;
	if (!event) {
		return nullptr;
	}
	pthread_mutex_init(&event->mutex, nullptr);
	pthread_cond_init(&event->condition, nullptr);
	event->manual_reset = manual_reset;
	event->signaled = initial_state;
	return event;
}

void rt_event_destroy(struct rt_event* event) {
	if (!event) {
		return;
	}
	pthread_cond_destroy(&event->condition);
	pthread_mutex_destroy(&event->mutex);
	delete event;
}

void rt_event_signal(struct rt_event* event) {
	pthread_mutex_lock(&event->mutex);
	event->signaled = true;
	if (event->manual_reset) {
		pthread_cond_broadcast(&event->condition);
	} else {
		pthread_cond_signal(&event->condition);
	}
	pthread_mutex_unlock(&event->mutex);
}

void rt_event_reset(struct rt_event* event) {
	pthread_mutex_lock(&event->mutex);
	event->signaled = false;
	pthread_mutex_unlock(&event->mutex);
}

void rt_event_wait(struct rt_event* event) {
	pthread_mutex_lock(&event->mutex);
	while (!event->signaled) {
		pthread_cond_wait(&event->condition, &event->mutex);
	}
	if (!event->manual_reset) {
		event->signaled = false;
	}
	pthread_mutex_unlock(&event->mutex);
}

u32 rt_event_wait_any(struct rt_event** events, u32 count) {
	const timespec delay = { 0, 1000000 };
	for (;;) {
		for (u32 index = 0; index < count; index++) {
			if (rt_event_try_consume(events[index])) {
				return index;
			}
		}
		nanosleep(&delay, nullptr);
	}
}

void rtgl_mutex_init(struct rtgl_mutex* mutex) {
	*mutex = {};
	pthread_mutex_init(rtgl_mutex_native(mutex), nullptr);
}

void rtgl_mutex_finish(struct rtgl_mutex* mutex) {
	pthread_mutex_destroy(rtgl_mutex_native(mutex));
}

void rtgl_mutex_lock(struct rtgl_mutex* mutex) {
	pthread_mutex_lock(rtgl_mutex_native(mutex));
}

void rtgl_mutex_unlock(struct rtgl_mutex* mutex) {
	pthread_mutex_unlock(rtgl_mutex_native(mutex));
}

void rtgl_condition_init(struct rtgl_condition* condition) {
	*condition = {};
	pthread_cond_init(rtgl_condition_native(condition), nullptr);
}

void rtgl_condition_finish(struct rtgl_condition* condition) {
	pthread_cond_destroy(rtgl_condition_native(condition));
}

void rtgl_condition_wait(struct rtgl_condition* condition, struct rtgl_mutex* mutex) {
	pthread_cond_wait(rtgl_condition_native(condition), rtgl_mutex_native(mutex));
}

void rtgl_condition_broadcast(struct rtgl_condition* condition) {
	pthread_cond_broadcast(rtgl_condition_native(condition));
}

struct rt_thread* rt_thread_create(rt_thread_proc proc, void* data) {
	rt_thread* thread = new (std::nothrow) rt_thread;
	rt_thread_start* start = new (std::nothrow) rt_thread_start{ proc, data };
	if (!thread || !start || pthread_create(&thread->native, nullptr, rt_thread_entry, start) != 0) {
		delete start;
		delete thread;
		return nullptr;
	}
	thread->id = (unsigned)std::hash<pthread_t>{}(thread->native);
	return thread;
}

void rt_thread_join(struct rt_thread* thread) {
	if (!thread) {
		return;
	}
	pthread_join(thread->native, nullptr);
	delete thread;
}

unsigned rt_thread_id(struct rt_thread* thread) {
	return thread ? thread->id : 0;
}

unsigned rt_current_thread_id(void) {
	return (unsigned)std::hash<pthread_t>{}(pthread_self());
}

}
