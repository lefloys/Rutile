#ifndef RTGL_SYSTEM_ATOMIC_H
#define RTGL_SYSTEM_ATOMIC_H

#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

#ifdef __cplusplus
extern "C" {
#endif

u32 rt_atomic_load(const u32* value);
u32 rt_atomic_inc(u32* value);
u32 rt_atomic_dec(u32* value);
bool rt_atomic_bool_load(const bool* value);
void rt_atomic_bool_store(bool* value, bool next);

#ifdef __cplusplus
}
#endif

#endif
