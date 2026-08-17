#include "atomic.h"

#if defined(_MSC_VER)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>

u32 rt_atomic_load(const u32* value) {
	return (u32)InterlockedCompareExchange((volatile LONG*)value, 0, 0);
}

u32 rt_atomic_inc(u32* value) {
	return (u32)InterlockedIncrement((volatile LONG*)value);
}

u32 rt_atomic_dec(u32* value) {
	return (u32)InterlockedDecrement((volatile LONG*)value);
}

bool rt_atomic_bool_load(const bool* value) {
	return __iso_volatile_load8((const volatile char*)value) != 0;
}

void rt_atomic_bool_store(bool* value, bool next) {
	__iso_volatile_store8((volatile char*)value, (char)next);
}

#else

u32 rt_atomic_load(const u32* value) {
	return __atomic_load_n(value, __ATOMIC_RELAXED);
}

u32 rt_atomic_inc(u32* value) {
	return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED);
}

u32 rt_atomic_dec(u32* value) {
	return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED);
}

bool rt_atomic_bool_load(const bool* value) {
	return __atomic_load_n(value, __ATOMIC_RELAXED);
}

void rt_atomic_bool_store(bool* value, bool next) {
	__atomic_store_n(value, next, __ATOMIC_RELAXED);
}

#endif
