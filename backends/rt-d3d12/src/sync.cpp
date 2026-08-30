#include "sync.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

rtd3d12_event::rtd3d12_event(bool manual_reset, bool initial_state)
	: handle(reinterpret_cast<uptr>(CreateEventA(nullptr, manual_reset ? TRUE : FALSE, initial_state ? TRUE : FALSE, nullptr))) {
}

rtd3d12_event::~rtd3d12_event() {
	if (handle) {
		CloseHandle(reinterpret_cast<HANDLE>(handle));
	}
}

rtd3d12_event::operator bool() const {
	return handle != 0;
}

void rtd3d12_event::wait() const {
	WaitForSingleObject(reinterpret_cast<HANDLE>(handle), INFINITE);
}

uptr rtd3d12_event::native_handle() const {
	return handle;
}
