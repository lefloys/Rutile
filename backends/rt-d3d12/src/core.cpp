#include "core.hpp"

#include <cstdio>
#include <cstring>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

u64 rtVersion() { return RTD3D12_HEADER_VERSION; }

void rtSettingSet(const char* name, const char* value) {
	(void)name;
	(void)value;
}

const char* rtGetName(void) { return "rt-d3d12"; }
