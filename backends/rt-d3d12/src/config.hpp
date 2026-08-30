#pragma once

#if defined(_WIN32)
#define RTD3D12_API extern "C" __declspec(dllexport)
#else
#define RTD3D12_API extern "C" __attribute__((visibility("default")))
#endif

inline constexpr size_t RTD3D12_MAX_FRAMES_IN_FLIGHT = 3;
