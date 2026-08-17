#ifndef RT_ATOMIC_H
#define RT_ATOMIC_H

#include "rutile.h"

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

#endif /* RT_ATOMIC_H */
