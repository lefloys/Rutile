#define RT_TYPES_ONLY
#include "../../../include/rutile.h"
#undef RT_TYPES_ONLY

#if defined(_WIN32)
#define RT_API_PUBLIC __declspec(dllexport)
#else
#define RT_API_PUBLIC __attribute__((visibility("default")))
#endif
