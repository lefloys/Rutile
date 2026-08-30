#pragma once

#include "config.hpp"
#include "rutile.hpp"

#include <dxgi.h>
#include <format>
#include <string>
#include <string_view>
#include <utility>

RTD3D12_API void rtSetOutput(rt::output output, void* user_data);
RTD3D12_API rt::error rtError();
RTD3D12_API const char* rtErrorMessage();
RTD3D12_API void rtClearError();

void rtd3d12_write(std::string_view message);
void rtd3d12_set_error(rt::error error, std::string message);

template <typename... Args>
void rtd3d12_print(std::format_string<Args...> format, Args&&... args) {
	rtd3d12_write(std::format(format, std::forward<Args>(args)...));
}

template <typename... Args>
void rtd3d12_fail(rt::error error, std::format_string<Args...> format, Args&&... args) {
	rtd3d12_set_error(error, std::format(format, std::forward<Args>(args)...));
}

rt::error rtd3d12_error_from_hresult(HRESULT result);
std::string rtd3d12_hresult_name(HRESULT result);
