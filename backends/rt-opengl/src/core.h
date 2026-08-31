#ifndef RTGL_CORE_H
#define RTGL_CORE_H

#include "config.h"
#include "error.h"
#include "rutile.h"

RTGL_EXTERN_C_ENTER

RTGL_API void rtInit(const char* const* features, usize feature_count);
RTGL_API void rtExit(void);
RTGL_API u64 rtVersion(void);
RTGL_API const char* rtGetName(void);

RTGL_EXTERN_C_EXIT

#endif /* RTGL_CORE_H */
