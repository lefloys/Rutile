#ifndef RTGL_SYSTEM_SYNC_H
#define RTGL_SYSTEM_SYNC_H

#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

#ifdef __cplusplus
extern "C" {
#endif

struct rt_event;
struct rt_thread;

#define RTGL_MUTEX_STORAGE_SIZE 64
#define RTGL_CONDITION_STORAGE_SIZE 64

struct rtgl_critical_section {
	void* unused;
	void* native;
};

struct rtgl_mutex {
	union {
		struct rtgl_critical_section critical_section;
		uptr storage[RTGL_MUTEX_STORAGE_SIZE / sizeof(uptr)];
	};
};

struct rtgl_condition {
	union {
		void* native;
		uptr storage[RTGL_CONDITION_STORAGE_SIZE / sizeof(uptr)];
	};
};

typedef unsigned (*rt_thread_proc)(void* data);

struct rt_event* rt_event_create(bool manual_reset, bool initial_state);
void rt_event_destroy(struct rt_event* event);
void rt_event_signal(struct rt_event* event);
void rt_event_reset(struct rt_event* event);
void rt_event_wait(struct rt_event* event);
u32 rt_event_wait_any(struct rt_event** events, u32 count);

void rtgl_mutex_init(struct rtgl_mutex* mutex);
void rtgl_mutex_finish(struct rtgl_mutex* mutex);
void rtgl_mutex_lock(struct rtgl_mutex* mutex);
void rtgl_mutex_unlock(struct rtgl_mutex* mutex);

void rtgl_condition_init(struct rtgl_condition* condition);
void rtgl_condition_finish(struct rtgl_condition* condition);
void rtgl_condition_wait(struct rtgl_condition* condition, struct rtgl_mutex* mutex);
void rtgl_condition_broadcast(struct rtgl_condition* condition);

struct rt_thread* rt_thread_create(rt_thread_proc proc, void* data);
void rt_thread_join(struct rt_thread* thread);
unsigned rt_thread_id(struct rt_thread* thread);
unsigned rt_current_thread_id(void);

#ifdef __cplusplus
}
#endif

#endif
