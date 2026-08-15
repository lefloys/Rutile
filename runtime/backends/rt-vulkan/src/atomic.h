#ifndef RTVK_ATOMIC_H
#define RTVK_ATOMIC_H

#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

#ifdef __cplusplus
extern "C" {
#endif

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

void rtvk_atomic_bool_store(bool* value, bool next);
bool rtvk_atomic_bool_load(const bool* value);
bool rtvk_atomic_bool_exchange(bool* value, bool next);
void rtvk_atomic_store(u32* value, u32 next);
u32 rtvk_atomic_load(const u32* value);
u32 rtvk_atomic_inc(u32* value);
u32 rtvk_atomic_dec(u32* value);

#ifdef __cplusplus
}
#endif

#endif
