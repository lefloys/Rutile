#include "core.h"
#include "context.h"

#include <string.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTGL_API void rtInit(const char* const* features, usize feature_count) {
	rtgl_context_flags flags;

	rtgl_begin_errorable_operation();
	if (current_context) {
		rtgl_throwf(RT_ALREADY_INITIALIZED, "rtInit called while rt-opengl is already initialized");
		return;
	}

	if (feature_count && !features) {
		rtgl_throwf(RT_IMPROPER_USAGE, "rtInit feature_count is %zu but features is NULL", feature_count);
		return;
	}

	flags = (rtgl_context_flags){ 0 };
	for (usize i = 0; i < feature_count; i++) {
		const char* feature = features[i];
		if (!feature) {
			rtgl_throwf(RT_IMPROPER_USAGE, "rtInit feature at index %zu is NULL", i);
			return;
		}
		if (strcmp(feature, RT_FEATURE_PRESENTATION) == 0) {
			flags.presentation = true;
			continue;
		}
		rtgl_throwf(RT_UNSUPPORTED_FEATURE, "unsupported rtInit feature: %s", feature);
		return;
	}

	rtgl_printf("rutile: initializing backend rt-opengl\n");
	current_context = rtgl_create_context(flags);
}

RTGL_API void rtExit(void) {
	rtgl_context_destroy(current_context);
	current_context = NULL;
}

RTGL_API u64 rtVersion(void) {
	return RT_HEADER_VERSION;
}

RTGL_API const char* rtGetName(void) {
	return "rt-opengl";
}
