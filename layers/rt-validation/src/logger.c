#include "logger.h"
#include "procs.h"

#include <stdio.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

static rt_output rtval_output = NULL;
static void* rtval_output_user_data = NULL;

#if defined(_WIN32)
#define RTVAL_THREAD_LOCAL __declspec(thread)
#else
#define RTVAL_THREAD_LOCAL _Thread_local
#endif

static RTVAL_THREAD_LOCAL enum rt_error rtval_error = RT_SUCCESS;
static RTVAL_THREAD_LOCAL char rtval_error_message[1024];

static void rtval_default_output(const char* message, void* user_data) {
	fputs(message, stdout);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

void rtvalSetOutput(rt_output output, void* user_data) {
	rtval_output = output;
	rtval_output_user_data = user_data;
}

void rtval_vprintf(const char* format, va_list args) {
	char message[1024];
	rt_output output = rtval_output ? rtval_output : rtval_default_output;

	if (!format) {
		return;
	}

	vsnprintf(message, sizeof(message), format, args);
	message[sizeof(message) - 1] = '\0';
	output(message, rtval_output_user_data);
}

void rtval_printf(const char* format, ...) {
	va_list args;
	va_start(args, format);
	rtval_vprintf(format, args);
	va_end(args);
}

void rtval_fail(const char* message) {
	const char* text = message ? message : "validation failed";
	rtval_error = RT_IMPROPER_USAGE;
	snprintf(rtval_error_message, sizeof(rtval_error_message), "%s", text);
	rtval_error_message[sizeof(rtval_error_message) - 1] = '\0';
	rtval_printf("[validation] %s, dropping call\n", rtval_error_message);
}

enum rt_error rtval_local_error(void) { return rtval_error; }
const char* rtval_local_error_message(void) { return rtval_error_message; }
void rtval_clear_local_error(void) {
	rtval_error = RT_SUCCESS;
	rtval_error_message[0] = '\0';
}

bool rtval_report_error(const char* call_name) {
	enum rt_error error = rtval_next_rtError();
	if (error == RT_SUCCESS) {
		return true;
	}

	const char* message = rtval_next_rtErrorMessage();
	rtval_printf("[validation] %s failed: error=%d message=\"%s\"\n", call_name ? call_name : "<unknown>", (i32)error, message ? message : "");
	return false;
}

#undef RTVAL_THREAD_LOCAL
