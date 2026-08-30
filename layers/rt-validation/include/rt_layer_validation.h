#ifndef RT_LAYER_VALIDATION_H
#define RT_LAYER_VALIDATION_H

#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

#if defined(_WIN32)
#define RT_API_PUBLIC __declspec(dllexport)
#else
#define RT_API_PUBLIC __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

RT_API_PUBLIC const char* rtLayerGetName(void);
RT_API_PUBLIC void rtLayerSetNext(rt_proc_chain next);

#ifdef __cplusplus
}
#endif
#endif
