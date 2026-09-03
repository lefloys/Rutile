#include "handles.h"
#include "logger.h"
#include "procs.h"

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RT_API_PUBLIC void rtInit(const char* const* features, usize feature_count) {
	rtval_rtInit(features, feature_count);
}

RT_API_PUBLIC void rtExit(void) {
	rtval_rtExit();
}

RT_API_PUBLIC u64 rtVersion(void) {
	rtval_clear_local_error();
	return rtval_next_rtVersion();
}

RT_API_PUBLIC enum rt_error rtError(void) {
	return rtval_rtError();
}

RT_API_PUBLIC const char* rtErrorMessage(void) {
	return rtval_rtErrorMessage();
}

RT_API_PUBLIC void rtClearError(void) {
	rtval_rtClearError();
}

RT_API_PUBLIC const char* rtGetName(void) {
	rtval_clear_local_error();
	return rtval_rtGetName();
}

RT_API_PUBLIC void rtSetOutput(rt_output output, void* user_data) {
	rtval_rtSetOutput(output, user_data);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

void rtval_rtInit(const char* const* features, usize feature_count) {
	if (feature_count && !features) {
		rtval_fail("rtInit: feature array required when feature_count is non-zero");
		return;
	}
	for (usize i = 0; i < feature_count; i++) {
		if (!features[i] || !features[i][0]) {
			rtval_fail("rtInit: feature names must be non-empty");
			return;
		}
	}
	rtval_next_rtInit(features, feature_count);
	rtval_report_error("rtInit");
}

void rtval_rtExit(void) {
	bool leaked_handles = rtval_handle_report_leaks();
	rtval_next_rtExit();
	if (!rtval_report_error("rtExit")) {
		return;
	}
	rtval_handle_reset_registry();
	if (leaked_handles) {
		rtval_fail("rtExit: application-created resources must be destroyed before exit");
	}
}

enum rt_error rtval_rtError(void) {
	if (rtval_local_error() != RT_SUCCESS) {
		return rtval_local_error();
	}
	return rtval_next_rtError();
}

const char* rtval_rtErrorMessage(void) {
	if (rtval_local_error() != RT_SUCCESS) {
		return rtval_local_error_message();
	}
	return rtval_next_rtErrorMessage();
}

void rtval_rtClearError(void) {
	rtval_clear_local_error();
	rtval_next_rtClearError();
}

const char* rtval_rtGetName(void) {
	return rtval_next_rtGetName();
}

void rtval_rtSetOutput(rt_output output, void* user_data) {
	rtval_clear_local_error();
	rtvalSetOutput(output, user_data);
	rtval_next_rtSetOutput(output, user_data);
}
