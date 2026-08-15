#ifndef RTGL_CORE_H
#define RTGL_CORE_H

#include "config.h"
#include "error.h"
#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

RTGL_EXTERN_C_ENTER

RTGL_API void rtInit(const char* const* features, u32 feature_count);
RTGL_API void rtExit(void);
RTGL_API void rtSettingSet(const char* name, const char* value);
RTGL_API const char* rtGetName(void);
RTGL_API enum rt_format_usage rtQueryFormatCapabilities(enum rt_format format);

RTGL_EXTERN_C_EXIT

#endif /* RTGL_CORE_H */
