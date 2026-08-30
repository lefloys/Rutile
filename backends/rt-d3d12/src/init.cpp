#include "core.hpp"

#include "context.hpp"
#include "error.hpp"

#include <string.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

static bool rtd3d12_feature_equals(const char* feature, const char* expected) {
	return feature && strcmp(feature, expected) == 0;
}

static bool rtd3d12_validate_init_features(const char* const* features, u32 feature_count, rtd3d12_context_flags* flags) {
	if (feature_count && !features) {
		rtd3d12_fail(rt::error::improper_usage, "rtInit feature_count is {} but features is nullptr", feature_count);
		return false;
	}

	*flags = {};
	for (u32 i = 0; i < feature_count; i++) {
		const char* feature = features[i];
		if (!feature) {
			rtd3d12_fail(rt::error::improper_usage, "rtInit feature at index {} is nullptr", i);
			return false;
		}
		if (rtd3d12_feature_equals(feature, rt::feature_presentation)) {
			flags->presentation = true;
			continue;
		}

		rtd3d12_fail(rt::error::unsupported_feature, "unsupported rtInit feature: {}", feature);
		return false;
	}

	return true;
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

void rtInit(const char* const* features, u32 feature_count) {
	rtd3d12_context_flags flags;

	if (current_context) {
		rtd3d12_fail(rt::error::already_initialized, "rtInit called while rt-d3d12 is already initialized");
		return;
	}

	if (!rtd3d12_validate_init_features(features, feature_count, &flags)) {
		return;
	}

	rtd3d12_print("rutile: initializing backend rt-d3d12\n");
	current_context = rtd3d12_context::create(flags);
}

void rtExit(void) {
	delete current_context;
	current_context = nullptr;
}
