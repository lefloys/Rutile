#pragma once

#include "rutile.hpp"

class rtd3d12_event {
public:
	rtd3d12_event(bool manual_reset, bool initial_state);
	~rtd3d12_event();

	rtd3d12_event(const rtd3d12_event&) = delete;
	rtd3d12_event& operator=(const rtd3d12_event&) = delete;

	explicit operator bool() const;
	void wait() const;
	uptr native_handle() const;

private:
	uptr handle = 0;
};
