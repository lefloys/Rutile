#include "handles.h"
#include "logger.h"
#include "procs.h"
#include "queue.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define RTVAL_DEFINE_NEXT(return_type, name, parameters, arguments) return_type (*rtval_next_##name) parameters = NULL;
RT_CORE_PROCEDURES(RTVAL_DEFINE_NEXT)
#undef RTVAL_DEFINE_NEXT

#define RTVAL_DEFINE_EXTENSION_NEXT(return_type, name, parameters, arguments) return_type (*rtval_next_##name) parameters = NULL;
RT_SWAPCHAIN_PROCEDURES(RTVAL_DEFINE_EXTENSION_NEXT)
RT_GLFW_SWAPCHAIN_PROCEDURES(RTVAL_DEFINE_EXTENSION_NEXT)
#undef RTVAL_DEFINE_EXTENSION_NEXT

/*===============================================================================================*/
/* central handle registry: open-addressing hashmap. Each slot owns a heap-allocated payload so   */
/* pointers returned by rtval_handle_payload stay valid across registry grows — entries that      */
/* nest into rtval_handle_create (eg. acquire wrapping a framebuffer) rely on this stability.     */
/*===============================================================================================*/

#define RTVAL_HANDLE_PAYLOAD_SIZE 64
#define RTVAL_HANDLE_EMPTY NULL
#define RTVAL_HANDLE_TOMBSTONE ((void*)~(uintptr_t)0)

typedef struct rtval_handle_slot {
	void* key;
	rtval_handle_type type;
	void* payload; /* heap-allocated so the pointer stays stable across registry grows */
} rtval_handle_slot;

static const char* rtval_handle_type_name(rtval_handle_type t) {
	switch (t) {
	case RTVAL_HANDLE_TYPE_BUFFER:
		return "buffer";
	case RTVAL_HANDLE_TYPE_TEXTURE:
		return "texture";
	case RTVAL_HANDLE_TYPE_TEXTURE_VIEW:
		return "texture_view";
	case RTVAL_HANDLE_TYPE_FRAMEBUFFER:
		return "framebuffer";
	case RTVAL_HANDLE_TYPE_PROGRAM:
		return "program";
	case RTVAL_HANDLE_TYPE_COMMAND_BUFFER:
		return "command_buffer";
	case RTVAL_HANDLE_TYPE_QUEUE:
		return "queue";
	case RTVAL_HANDLE_TYPE_SWAPCHAIN:
		return "swapchain";
	case RTVAL_HANDLE_TYPE_SAMPLER:
		return "sampler";
	case RTVAL_HANDLE_TYPE_LOCATION:
		return "location";
	default:
		return "unknown";
	}
}

static rtval_handle_slot* rtval_handle_slots = NULL;
static usize rtval_handle_capacity = 0; /* power of two */
static usize rtval_handle_count = 0;	/* live entries (excludes tombstones) */
static usize rtval_handle_used = 0;		/* live + tombstones */
static void* rtval_retired_keys = NULL;

static usize rtval_hash_pointer(void* h) {
	uintptr_t p = (uintptr_t)h;
	p ^= p >> 33;
	p *= (uintptr_t)0xff51afd7ed558ccdull;
	p ^= p >> 33;
	p *= (uintptr_t)0xc4ceb9fe1a85ec53ull;
	p ^= p >> 33;
	return (usize)p;
}

static rtval_handle_slot* rtval_find_handle_slot(void* key, rtval_handle_slot* slots, usize capacity) {
	usize mask = capacity - 1;
	usize i = rtval_hash_pointer(key) & mask;
	rtval_handle_slot* tombstone = NULL;
	for (usize probe = 0; probe < capacity; ++probe) {
		void* k = slots[i].key;
		if (k == RTVAL_HANDLE_EMPTY) {
			return tombstone ? tombstone : &slots[i];
		}
		if (k == RTVAL_HANDLE_TOMBSTONE) {
			if (!tombstone) {
				tombstone = &slots[i];
			}
		} else if (k == key) {
			return &slots[i];
		}
		i = (i + 1) & mask;
	}
	return tombstone;
}

static void rtval_grow_registry(void) {
	usize new_capacity = rtval_handle_capacity ? rtval_handle_capacity * 2 : 64;
	rtval_handle_slot* new_slots = calloc(new_capacity, sizeof(*new_slots));
	if (!new_slots) {
		return;
	}
	for (usize i = 0; i < rtval_handle_capacity; i++) {
		void* k = rtval_handle_slots[i].key;
		if (k == RTVAL_HANDLE_EMPTY || k == RTVAL_HANDLE_TOMBSTONE) {
			continue;
		}
		rtval_handle_slot* dst = rtval_find_handle_slot(k, new_slots, new_capacity);
		if (!dst) {
			free(new_slots);
			return;
		}
		*dst = rtval_handle_slots[i];
	}
	free(rtval_handle_slots);
	rtval_handle_slots = new_slots;
	rtval_handle_capacity = new_capacity;
	rtval_handle_used = rtval_handle_count;
}

void* rtval_handle_create(rtval_handle_type type) {
	if (!rtval_handle_capacity || rtval_handle_used * 2 >= rtval_handle_capacity) {
		rtval_grow_registry();
	}
	if (!rtval_handle_capacity) {
		return NULL;
	}
	/* Keep tokens allocated until rtExit so a stale handle cannot become live again. */
	void* key = malloc(sizeof(void*));
	if (!key) {
		return NULL;
	}

	rtval_handle_slot* slot = rtval_find_handle_slot(key, rtval_handle_slots, rtval_handle_capacity);
	if (!slot) {
		free(key);
		return NULL;
	}
	if (slot->key == RTVAL_HANDLE_EMPTY) {
		rtval_handle_used++;
	}
	void* payload = calloc(1, RTVAL_HANDLE_PAYLOAD_SIZE);
	if (!payload) {
		free(key);
		return NULL;
	}
	slot->key = key;
	slot->type = type;
	slot->payload = payload;
	rtval_handle_count++;
	return key;
}

static rtval_handle_slot* rtval_find_handle_slot_by_payload(void* payload, rtval_handle_slot* slots, usize capacity) {
	if (!payload || !slots || !capacity) {
		return NULL;
	}
	for (usize i = 0; i < capacity; i++) {
		if (slots[i].key == RTVAL_HANDLE_EMPTY || slots[i].key == RTVAL_HANDLE_TOMBSTONE) {
			continue;
		}
		if (slots[i].payload == payload) {
			return &slots[i];
		}
	}
	return NULL;
}

bool rtval_handle_report_leaks(void) {
	u32 counts[RTVAL_HANDLE_TYPE_COUNT] = { 0 };
	for (usize i = 0; i < rtval_handle_capacity; i++) {
		void* k = rtval_handle_slots[i].key;
		if (k == RTVAL_HANDLE_EMPTY || k == RTVAL_HANDLE_TOMBSTONE) {
			continue;
		}
		rtval_handle_type t = rtval_handle_slots[i].type;
		if (t == RTVAL_HANDLE_TYPE_LOCATION) {
			continue;
		}
		if ((u32)t < RTVAL_HANDLE_TYPE_COUNT) {
			counts[t]++;
		}
	}
	bool any = false;
	for (u32 t = 0; t < RTVAL_HANDLE_TYPE_COUNT; t++) {
		if (!counts[t]) {
			continue;
		}
		if (!any) {
			rtval_printf("[validation] leaked handles at shutdown:\n");
			any = true;
		}
		rtval_printf("[validation]   %s: %u\n", rtval_handle_type_name((rtval_handle_type)t), counts[t]);
	}
	if (!any) {
		return false;
	}
	for (usize i = 0; i < rtval_handle_capacity; i++) {
		void* k = rtval_handle_slots[i].key;
		if (k == RTVAL_HANDLE_EMPTY || k == RTVAL_HANDLE_TOMBSTONE) {
			continue;
		}
		if (rtval_handle_slots[i].type == RTVAL_HANDLE_TYPE_LOCATION) {
			continue;
		}
		rtval_printf("[validation]     live %s handle=%p\n", rtval_handle_type_name(rtval_handle_slots[i].type), k);
	}
	return true;
}

void* rtval_handle_payload(void* key) {
	if (!key || key == RTVAL_HANDLE_TOMBSTONE || !rtval_handle_capacity || !rtval_handle_slots) {
		return NULL;
	}
	rtval_handle_slot* slot = rtval_find_handle_slot(key, rtval_handle_slots, rtval_handle_capacity);
	if (!slot) {
		return NULL;
	}
	if (slot->key != key) {
		return NULL;
	}
	return slot->payload;
}

void* rtval_handle_find_by_backend(rtval_handle_type type, void* backend) {
	if (!backend) {
		return NULL;
	}
	for (usize i = 0; i < rtval_handle_capacity; i++) {
		rtval_handle_slot* slot = &rtval_handle_slots[i];
		if (slot->key == RTVAL_HANDLE_EMPTY || slot->key == RTVAL_HANDLE_TOMBSTONE || slot->type != type) {
			continue;
		}
		if (*(void**)slot->payload == backend) {
			return slot->key;
		}
	}
	return NULL;
}

bool rtval_handle_is_live(void* key) {
	return rtval_handle_payload(key) != NULL;
}

void rtval_handle_destroy(void* key) {
	if (!key || key == RTVAL_HANDLE_TOMBSTONE || !rtval_handle_capacity || !rtval_handle_slots) {
		return;
	}
	rtval_handle_slot* slot = rtval_find_handle_slot(key, rtval_handle_slots, rtval_handle_capacity);
	if (!slot || (slot->key != key && slot->payload != key)) {
		slot = rtval_find_handle_slot_by_payload(key, rtval_handle_slots, rtval_handle_capacity);
		if (!slot) {
			return;
		}
	}
	if (slot->key) {
		*(void**)slot->key = rtval_retired_keys;
		rtval_retired_keys = slot->key;
		free(slot->payload);
		slot->payload = NULL;
		slot->key = RTVAL_HANDLE_TOMBSTONE;
		rtval_handle_count--;
	}
}

void rtval_handle_reset_registry(void) {
	for (usize i = 0; i < rtval_handle_capacity; i++) {
		void* k = rtval_handle_slots ? rtval_handle_slots[i].key : NULL;
		if (k == RTVAL_HANDLE_EMPTY || k == RTVAL_HANDLE_TOMBSTONE) {
			continue;
		}
		free(k);
		free(rtval_handle_slots[i].payload);
	}
	free(rtval_handle_slots);
	while (rtval_retired_keys) {
		void* key = rtval_retired_keys;
		rtval_retired_keys = *(void**)key;
		free(key);
	}
	rtval_handle_slots = NULL;
	rtval_handle_capacity = 0;
	rtval_handle_count = 0;
	rtval_handle_used = 0;
}

#undef RTVAL_HANDLE_TOMBSTONE
#undef RTVAL_HANDLE_EMPTY
#undef RTVAL_HANDLE_PAYLOAD_SIZE

/*===============================================================================================*/
/* timepoints are opaque backend values and need no validation-layer translation                  */
/*===============================================================================================*/

rt_timepoint rtval_timepoint_wrap(rt_timepoint backend_tp) {
	return backend_tp;
}

rt_timepoint rtval_timepoint_unwrap(rt_timepoint public_tp) {
	return public_tp;
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RT_API_PUBLIC void rtLayerSetNext(rt_proc_chain next) {
#define RTVAL_RESOLVE_NEXT(return_type, name, parameters, arguments) rtval_next_##name = (return_type (*) parameters)next.get_proc(&next, #name);
	RT_CORE_PROCEDURES(RTVAL_RESOLVE_NEXT)
#undef RTVAL_RESOLVE_NEXT

#define RTVAL_RESOLVE_EXTENSION_NEXT(return_type, name, parameters, arguments) rtval_next_##name = (return_type (*) parameters)next.get_proc(&next, #name);
	RT_SWAPCHAIN_PROCEDURES(RTVAL_RESOLVE_EXTENSION_NEXT)
	RT_GLFW_SWAPCHAIN_PROCEDURES(RTVAL_RESOLVE_EXTENSION_NEXT)
#undef RTVAL_RESOLVE_EXTENSION_NEXT
}
