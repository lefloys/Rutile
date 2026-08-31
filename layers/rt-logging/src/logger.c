#include "procs.h"

#include <stdarg.h>
#include <stdio.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

static rt_output rtlog_output = NULL;
static void* rtlog_output_user_data = NULL;

static void rtlog_default_output(const char* message, void* user_data) {
	fputs(message, stdout);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

void rtlog_set_output(rt_output output, void* user_data) {
	rtlog_output = output;
	rtlog_output_user_data = user_data;
}

void rtlog_printf(const char* format, ...) {
	char message[1024];
	va_list args;
	rt_output output = rtlog_output ? rtlog_output : rtlog_default_output;

	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);
	message[sizeof(message) - 1] = '\0';
	output(message, rtlog_output_user_data);
}
