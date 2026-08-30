#include "core.hpp"

#include <cstdio>
#include <cstring>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

void rtSettingSet(const char* name, const char* value) {
	(void)name;
	(void)value;
}

const char* rtGetName(void) { return "rt-d3d12"; }

rt::format_usage rtQueryFormatCapabilities(rt::format format) {
	return rt::format_usage::none;
}
