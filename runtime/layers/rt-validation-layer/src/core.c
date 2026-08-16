#include "handles.h"
#include "logger.h"
#include "procs.h"

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RT_API_PUBLIC void rtInit(const char* const* features, u32 feature_count) { rtval_rtInit(features, feature_count); }
RT_API_PUBLIC void rtExit(void) { rtval_rtExit(); }
RT_API_PUBLIC enum rt_error rtError(void) { return rtval_rtError(); }
RT_API_PUBLIC const char* rtErrorMessage(void) { return rtval_rtErrorMessage(); }
RT_API_PUBLIC void rtClearError(void) { rtval_rtClearError(); }
RT_API_PUBLIC const char* rtGetName(void) { return rtval_rtGetName(); }
RT_API_PUBLIC enum rt_format_usage rtQueryFormatCapabilities(enum rt_format format) { return rtval_rtQueryFormatCapabilities(format); }
RT_API_PUBLIC void rtSetOutput(rt_output output, void* user_data) { rtval_rtSetOutput(output, user_data); }

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

void rtval_rtInit(const char* const* features, u32 feature_count) {
	rtval_next_rtInit(features, feature_count);
	rtval_report_error("rtInit");
}

void rtval_rtExit(void) {
	rtval_handle_report_leaks();
	rtval_handle_reset_registry();
	rtval_next_rtExit();
	rtval_report_error("rtExit");
}

enum rt_error rtval_rtError(void) {
	return rtval_next_rtError();
}

const char* rtval_rtErrorMessage(void) {
	return rtval_next_rtErrorMessage();
}

void rtval_rtClearError(void) {
	rtval_next_rtClearError();
}

const char* rtval_rtGetName(void) {
	return rtval_next_rtGetName();
}

enum rt_format_usage rtval_rtQueryFormatCapabilities(enum rt_format format) {
	enum rt_format_usage usage = rtval_next_rtQueryFormatCapabilities(format);
	rtval_report_error("rtQueryFormatCapabilities");
	return usage;
}

void rtval_rtSetOutput(rt_output output, void* user_data) {
	rtvalSetOutput(output, user_data);
	rtval_next_rtSetOutput(output, user_data);
}
