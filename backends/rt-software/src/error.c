#include "error.h"
#include "context.h"

#include <stdio.h>

struct rtsw_pre_context_state {
	struct rtsw_error_state error;
	rt_output output;
	void* output_user_data;
};

static _Thread_local struct rtsw_pre_context_state pre_context;

static struct rtsw_error_state* rtsw_current_error_state(void) {
	struct rtsw_context* context = rtsw_get_current_context();
	return context ? &context->error : &pre_context.error;
}

void rtsw_error_attach_context(struct rtsw_context* context) {
	if (!context) return;
	context->error = pre_context.error;
	context->output = pre_context.output;
	context->output_user_data = pre_context.output_user_data;
}

void rtSetOutput(rt_output output, void* user_data) {
	struct rtsw_context* context = rtsw_get_current_context();
	if (context) {
		context->output = output;
		context->output_user_data = user_data;
	} else {
		pre_context.output = output;
		pre_context.output_user_data = user_data;
	}
}

enum rt_error rtError(void) {
	return rtsw_current_error_state()->code;
}

const char* rtErrorMessage(void) {
	return rtsw_current_error_state()->message;
}

void rtClearError(void) {
	rtsw_clear_error();
}

void rtsw_clear_error(void) {
	struct rtsw_error_state* error = rtsw_current_error_state();
	error->code = RT_SUCCESS;
	error->message[0] = '\0';
}

void rtsw_vprintf(const char* format, va_list args) {
	char message[1024];
	vsnprintf(message, sizeof(message), format, args);
	struct rtsw_context* context = rtsw_get_current_context();
	if (context && context->output) {
		context->output(message, context->output_user_data);
	} else if (!context && pre_context.output) {
		pre_context.output(message, pre_context.output_user_data);
	} else {
		fputs(message, stderr);
	}
}

void rtsw_printf(const char* format, ...) {
	va_list args;
	va_start(args, format);
	rtsw_vprintf(format, args);
	va_end(args);
}

void rtsw_throwf(enum rt_error error, const char* format, ...) {
	va_list args;
	struct rtsw_error_state* state = rtsw_current_error_state();
	va_start(args, format);
	vsnprintf(state->message, sizeof(state->message), format, args);
	va_end(args);
	state->code = error;
}

enum rt_error rtsw_error(void) {
	return rtsw_current_error_state()->code;
}
