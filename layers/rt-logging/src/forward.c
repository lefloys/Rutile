#include "procs.h"

/*
** The public procedure inventory is the layer's forwarding inventory too.
** Keep this deliberately mechanical: every procedure is resolved in state.c
** and exported here with the exact public signature.
*/

#define RTLOG_FORWARD_PROCEDURE(return_type, name, parameters, arguments) \
	RT_API_PUBLIC return_type name parameters {                              \
		rtlog_printf("[logging] " #name "()\\n");                        \
		return next_##name arguments;                                         \
	}

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4098)
#endif

#define rtSetOutput rtlog_forward_rtSetOutput
RT_CORE_PROCEDURES(RTLOG_FORWARD_PROCEDURE)
#undef rtSetOutput
RT_SWAPCHAIN_PROCEDURES(RTLOG_FORWARD_PROCEDURE)
RT_GLFW_SWAPCHAIN_PROCEDURES(RTLOG_FORWARD_PROCEDURE)

RT_API_PUBLIC void rtSetOutput(rt_output output, void* user_data) {
	rtlog_set_output(output, user_data);
	rtlog_forward_rtSetOutput(output, user_data);
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#undef RTLOG_FORWARD_PROCEDURE
