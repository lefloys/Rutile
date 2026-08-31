#pragma once

#include <cstdint>

#if defined(_WIN32)
#define RTD3D12_API extern "C" __declspec(dllexport)
#else
#define RTD3D12_API extern "C" __attribute__((visibility("default")))
#endif

inline constexpr size_t RTD3D12_MAX_FRAMES_IN_FLIGHT = 3;
inline constexpr std::uint64_t RTD3D12_HEADER_VERSION = (std::uint64_t{0} << 48u) | (std::uint64_t{1} << 32u);
