#ifndef RTSW_CORE_H
#define RTSW_CORE_H

#include "config.h"
#include "rutile.h"

RTSW_EXTERN_C_ENTER

RTSW_API void rtInit(const char* const* features, usize feature_count);
RTSW_API void rtExit(void);
RTSW_API u64 rtVersion(void);
RTSW_API const char* rtGetName(void);

RTSW_EXTERN_C_EXIT

#endif
