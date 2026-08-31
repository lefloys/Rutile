#ifndef RTVAL_LOGGER_H
#define RTVAL_LOGGER_H

#include "types.h"

#include <stdarg.h>

void rtvalSetOutput(rt_output output, void* user_data);
void rtval_printf(const char* format, ...);
void rtval_vprintf(const char* format, va_list args);
void rtval_fail(const char* message);
enum rt_error rtval_local_error(void);
const char* rtval_local_error_message(void);
void rtval_clear_local_error(void);
bool rtval_report_error(const char* call_name);

#endif
