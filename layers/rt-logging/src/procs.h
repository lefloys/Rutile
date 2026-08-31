#ifndef RTLOG_PROCS_H
#define RTLOG_PROCS_H

#include "rutile.h"

#if defined(_WIN32)
#define RT_API_PUBLIC __declspec(dllexport)
#else
#define RT_API_PUBLIC __attribute__((visibility("default")))
#endif

void rtlog_printf(const char* format, ...);
void rtlog_error(const char* name);

#define RTLOG_DECLARE_NEXT(return_type, name, parameters, arguments) extern return_type (*next_##name) parameters;
RT_CORE_PROCEDURES(RTLOG_DECLARE_NEXT)
#undef RTLOG_DECLARE_NEXT

#define RTLOG_DECLARE_EXTENSION_NEXT(return_type, name, parameters, arguments) extern return_type (*next_##name) parameters;
RT_SWAPCHAIN_PROCEDURES(RTLOG_DECLARE_EXTENSION_NEXT)
RT_GLFW_SWAPCHAIN_PROCEDURES(RTLOG_DECLARE_EXTENSION_NEXT)
#undef RTLOG_DECLARE_EXTENSION_NEXT

#define RTLOG_DECLARE_PROCEDURE(return_type, name, parameters, arguments) return_type rtlog_##name parameters;
RT_CORE_PROCEDURES(RTLOG_DECLARE_PROCEDURE)
#undef RTLOG_DECLARE_PROCEDURE

#define RTLOG_DECLARE_EXTENSION_PROCEDURE(return_type, name, parameters, arguments) return_type rtlog_##name parameters;
RT_SWAPCHAIN_PROCEDURES(RTLOG_DECLARE_EXTENSION_PROCEDURE)
RT_GLFW_SWAPCHAIN_PROCEDURES(RTLOG_DECLARE_EXTENSION_PROCEDURE)
#undef RTLOG_DECLARE_EXTENSION_PROCEDURE

#endif
