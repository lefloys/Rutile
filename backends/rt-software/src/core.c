#include "core.h"
#include "context.h"
#include "error.h"

#include <string.h>

static bool rtsw_feature_equals(const char* feature, const char* expected) {
	return feature && strcmp(feature, expected) == 0;
}

static bool rtsw_validate_init_features(const char* const* features, usize feature_count, rtsw_context_flags* flags) {
	if (feature_count && !features) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtInit feature_count is %zu but features is NULL", feature_count);
		return false;
	}

	*flags = (rtsw_context_flags){ 0 };
	for (usize i = 0; i < feature_count; ++i) {
		const char* feature = features[i];
		if (!feature) {
			rtsw_throwf(RT_IMPROPER_USAGE, "rtInit feature at index %zu is NULL", i);
			return false;
		}
		if (rtsw_feature_equals(feature, RT_FEATURE_PRESENTATION)) {
			flags->presentation = true;
			continue;
		}
		rtsw_throwf(RT_UNSUPPORTED_FEATURE, "unsupported rtInit feature: %s", feature);
		return false;
	}

	return true;
}

void rtInit(const char* const* features, usize feature_count) {
	rtsw_context_flags flags;
	rtsw_clear_error();
	if (current_context) {
		rtsw_throwf(RT_ALREADY_INITIALIZED, "rt-software is already initialized");
		return;
	}
	if (!rtsw_validate_init_features(features, feature_count, &flags)) {
		return;
	}

	current_context = rtsw_create_context(flags);
	if (current_context) {
		rtsw_error_attach_context(current_context);
	}
}

void rtExit(void) {
	rtsw_context_destroy(current_context);
	current_context = NULL;
}

u64 rtVersion(void) {
	return RT_HEADER_VERSION;
}

const char* rtGetName(void) {
	return "rt-software";
}
