#include "procs.h"

/*
** The public procedure inventory is the layer's forwarding inventory too.
** Keep this deliberately mechanical: every procedure is resolved in state.c
** and exported here with the exact public signature.
*/

#define RTLOG_FORWARD_PROCEDURE(return_type, name, parameters, arguments) \
	RT_API_PUBLIC return_type name parameters {                              \
		rtlog_printf(#name "()\\n");                                       \
		return next_##name arguments;                                         \
	}

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4098)
#endif

RT_CORE_PROCEDURES(RTLOG_FORWARD_PROCEDURE)
RT_SWAPCHAIN_PROCEDURES(RTLOG_FORWARD_PROCEDURE)
RT_GLFW_SWAPCHAIN_PROCEDURES(RTLOG_FORWARD_PROCEDURE)

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#undef RTLOG_FORWARD_PROCEDURE
