#include "next.h"
#include "trace.h"
#include "resource/rasterizer.h"
#include "debugger.h"
#include "texture_preview.h"

RT_API_PUBLIC u64 rtVersion(void) {
	rtdbg_trace_api("rtVersion");
	return rtdbg_procs.rtVersion();
}

RT_API_PUBLIC void rtSetOutput(rt_output output, void* user_data) {
	rtdbg_trace_api("rtSetOutput");
	rtdbg_procs.rtSetOutput(output, user_data);
}

RT_API_PUBLIC enum rt_error rtError(void) {
	rtdbg_trace_api("rtError");
	return rtdbg_procs.rtError();
}

RT_API_PUBLIC const char* rtErrorMessage(void) {
	rtdbg_trace_api("rtErrorMessage");
	return rtdbg_procs.rtErrorMessage();
}

RT_API_PUBLIC void rtClearError(void) {
	rtdbg_trace_api("rtClearError");
	rtdbg_procs.rtClearError();
}

RT_API_PUBLIC const char* rtGetName(void) {
	rtdbg_trace_api("rtGetName");
	return rtdbg_procs.rtGetName();
}

RT_API_PUBLIC void rtInit(const char* const* features, usize feature_count) {
	rtdbg_debugger_start();
	rtdbg_texture_preview_reset();
	rtdbg_trace_api("rtInit");
	rtdbg_procs.rtInit(features, feature_count);
}

RT_API_PUBLIC void rtExit(void) {
	rtdbg_rasterizer_reset();
	rtdbg_trace_api("rtExit");
	rtdbg_procs.rtExit();
	rtdbg_texture_preview_reset();
	rtdbg_trace_close();
}

