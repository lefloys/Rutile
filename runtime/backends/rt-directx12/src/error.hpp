#pragma once

#include "config.hpp"
#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

#include <dxgi.h>
#include <stdarg.h>

RTDX_API void rtSetOutput(PFN_rtOutput output, void* user_data);
RTDX_API rt_error rtError();
RTDX_API const char* rtErrorMessage();
RTDX_API void rtClearError();

void rtdx_printf(const char* format, ...);
void rtdx_vprintf(const char* format, va_list args);
void rtdx_throwf(rt_error error, const char* format, ...);
rt_error rtdx_error_from_hresult(HRESULT result);
const char* rtdx_hresult_name(HRESULT result);
