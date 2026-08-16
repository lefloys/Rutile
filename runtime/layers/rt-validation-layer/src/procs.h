#ifndef RTVAL_PROCS_H
#define RTVAL_PROCS_H

#include "rutile.h"

#define RTVAL_DECLARE_NEXT(return_type, name, parameters, arguments) extern return_type (*rtval_next_##name) parameters;
RT_CORE_PROCEDURES(RTVAL_DECLARE_NEXT)
#undef RTVAL_DECLARE_NEXT

#define RTVAL_DECLARE_EXTENSION_NEXT(return_type, name, parameters, arguments) extern return_type (*rtval_next_##name) parameters;
RT_EXT_SWAPCHAIN_PROCEDURES(RTVAL_DECLARE_EXTENSION_NEXT)
RT_EXT_GLFW_PROCEDURES(RTVAL_DECLARE_EXTENSION_NEXT)
#undef RTVAL_DECLARE_EXTENSION_NEXT

void rtval_rtInit(const char* const* features, u32 feature_count);
void rtval_rtExit(void);
enum rt_error rtval_rtError(void);
const char* rtval_rtErrorMessage(void);
void rtval_rtClearError(void);
const char* rtval_rtGetName(void);
enum rt_format_usage rtval_rtQueryFormatCapabilities(enum rt_format format);
void rtval_rtSetOutput(rt_output output, void* user_data);

#endif
