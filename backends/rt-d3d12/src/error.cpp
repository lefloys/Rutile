#include "error.hpp"

#include <cstdio>
#include <string>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

static thread_local rt::error rtd3d12_error = rt::error::success;
static thread_local std::string rtd3d12_error_text;

static thread_local rt::output rtd3d12_output = nullptr;
static thread_local void* rtd3d12_output_user_data = nullptr;

static const char* rtd3d12_hresult_fallback(HRESULT result) {
	switch (result) {
	case E_OUTOFMEMORY:
		return "E_OUTOFMEMORY";
	case DXGI_ERROR_DEVICE_REMOVED:
		return "DXGI_ERROR_DEVICE_REMOVED";
	case DXGI_ERROR_DEVICE_RESET:
		return "DXGI_ERROR_DEVICE_RESET";
	case DXGI_ERROR_DEVICE_HUNG:
		return "DXGI_ERROR_DEVICE_HUNG";
	case DXGI_ERROR_UNSUPPORTED:
		return "DXGI_ERROR_UNSUPPORTED";
	case DXGI_ERROR_INVALID_CALL:
		return "DXGI_ERROR_INVALID_CALL";
	default:
		return "HRESULT";
	}
}

static void rtd3d12_default_output(const char* message, void* user_data) {
	fputs(message, stdout);
	fflush(stdout);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

void rtSetOutput(rt::output output, void* user_data) {
	rtd3d12_output = output;
	rtd3d12_output_user_data = user_data;
}

void rtd3d12_write(std::string_view message) {
	rt::output output = rtd3d12_output ? rtd3d12_output : rtd3d12_default_output;
	const std::string text(message);
	output(text.c_str(), rtd3d12_output_user_data);
}

void rtd3d12_set_error(rt::error error, std::string message) {
	rtd3d12_error = error;
	rtd3d12_error_text = std::move(message);
}

rt::error rtd3d12_error_from_hresult(HRESULT result) {
	switch (result) {
	case E_OUTOFMEMORY:
		return rt::error::out_of_host_memory;
	case DXGI_ERROR_DEVICE_REMOVED:
	case DXGI_ERROR_DEVICE_RESET:
	case DXGI_ERROR_DEVICE_HUNG:
		return rt::error::device_lost;
	case DXGI_ERROR_UNSUPPORTED:
		return rt::error::unsupported_platform;
	default:
		return rt::error::initialization_failed;
	}
}

std::string rtd3d12_hresult_name(HRESULT result) {
	const char* name = rtd3d12_hresult_fallback(result);
	if (std::string_view(name) != "HRESULT") {
		return name;
	}
	return std::format("HRESULT(0x{:08x})", static_cast<u32>(result));
}

rt::error rtError(void) {
	return rtd3d12_error;
}

const char* rtErrorMessage(void) {
	return rtd3d12_error_text.c_str();
}

void rtClearError(void) {
	rtd3d12_error = rt::error::success;
	rtd3d12_error_text.clear();
}
