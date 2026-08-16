#pragma once

#include "config.hpp"
#include "error.hpp"
#include "resource/buffer.hpp"
#include "resource/command_buffer.hpp"
#include "resource/graphics_program.hpp"
#include "resource/queue.hpp"
#include "resource/swapchain.hpp"
#include "resource/texture.hpp"
#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

RTDX_API void rtInit(const char* const* features, u32 feature_count);
RTDX_API void rtExit();
RTDX_API void rtSettingSet(const char* name, const char* value);
RTDX_API const char* rtGetName();
RTDX_API rt_format_usage rtQueryFormatCapabilities(rt_format format);
