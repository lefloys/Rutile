#include "procs.h"

#define RTLOG_DEFINE_NEXT(return_type, name, parameters, arguments) return_type (*next_##name) parameters = NULL;
RT_PROCEDURES(RTLOG_DEFINE_NEXT)
#undef RTLOG_DEFINE_NEXT

#define RTLOG_DEFINE_EXTENSION_NEXT(return_type, name, parameters, arguments) return_type (*next_##name) parameters = NULL;
RT_EXT_SWAPCHAIN_PROCEDURES(RTLOG_DEFINE_EXTENSION_NEXT)
RT_EXT_GLFW_PROCEDURES(RTLOG_DEFINE_EXTENSION_NEXT)
#undef RTLOG_DEFINE_EXTENSION_NEXT

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RT_API_PUBLIC void rtLayerSetNext(rt_proc_chain next) {
#define RTLOG_RESOLVE_NEXT(return_type, name, parameters, arguments) next_##name = (return_type (*) parameters)next.get_proc(&next, #name);
	RT_PROCEDURES(RTLOG_RESOLVE_NEXT)
#undef RTLOG_RESOLVE_NEXT

#define RTLOG_RESOLVE_EXTENSION_NEXT(return_type, name, parameters, arguments) next_##name = (return_type (*) parameters)next.get_proc(&next, #name);
	RT_EXT_SWAPCHAIN_PROCEDURES(RTLOG_RESOLVE_EXTENSION_NEXT)
	RT_EXT_GLFW_PROCEDURES(RTLOG_RESOLVE_EXTENSION_NEXT)
#undef RTLOG_RESOLVE_EXTENSION_NEXT
}
