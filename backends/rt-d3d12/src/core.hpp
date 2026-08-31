#pragma once

#include "config.hpp"
#include "error.hpp"
#include "resource/buffer.hpp"
#include "resource/command_buffer.hpp"
#include "resource/program.hpp"
#include "resource/queue.hpp"
#include "resource/sampler.hpp"
#include "resource/swapchain.hpp"
#include "resource/texture.hpp"
#include "rutile.hpp"

RTD3D12_API void rtInit(const char* const* features, usize feature_count);
RTD3D12_API void rtExit();
RTD3D12_API u64 rtVersion();
RTD3D12_API void rtSettingSet(const char* name, const char* value);
RTD3D12_API const char* rtGetName();
