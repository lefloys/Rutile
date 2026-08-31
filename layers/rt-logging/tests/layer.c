#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

#include <stdio.h>
#include <string.h>

extern void rtLayerSetNext(rt_proc_chain next);
extern void rtSetOutput(rt_output output, void* user_data);
extern u64 rtVersion(void);

static rt_output forwarded_output;
static void* forwarded_user_data;
static char output[1024];

static void fake_rtSetOutput(rt_output callback, void* user_data) {
	forwarded_output = callback;
	forwarded_user_data = user_data;
}
static u64 fake_rtVersion(void) { return 42; }
static rt_proc_t fake_get_proc(const rt_proc_chain* chain, const char* name) {
	(void)chain;
	if (strcmp(name, "rtSetOutput") == 0) { return (rt_proc_t)fake_rtSetOutput; }
	if (strcmp(name, "rtVersion") == 0) { return (rt_proc_t)fake_rtVersion; }
	return NULL;
}
static void capture(const char* message, void* user_data) {
	(void)user_data;
	snprintf(output, sizeof(output), "%s", message ? message : "");
}

int main(void) {
	rt_proc_chain chain = { fake_get_proc };
	rtLayerSetNext(chain);
	rtSetOutput(capture, output);
	if (forwarded_output != capture || forwarded_user_data != output) {
		return 1;
	}
	if (rtVersion() != 42 || strstr(output, "[logging] rtVersion()") == NULL) {
		return 2;
	}
	return 0;
}
