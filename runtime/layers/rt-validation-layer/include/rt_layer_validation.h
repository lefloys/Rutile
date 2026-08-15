#ifndef RT_LAYER_VALIDATION_H
#define RT_LAYER_VALIDATION_H

#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

#ifdef __cplusplus
extern "C" {
#endif

RT_API_PUBLIC const char* rtLayerGetName(void);
RT_API_PUBLIC void rtLayerSetNext(rt_proc_chain next);

#ifdef __cplusplus
}
#endif
#endif
