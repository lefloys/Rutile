#ifndef RTSW_CONFIG_H
#define RTSW_CONFIG_H

#if defined(_WIN32)
#define RTSW_API __declspec(dllexport)
#else
#define RTSW_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
#define RTSW_EXTERN_C_ENTER extern "C" {
#define RTSW_EXTERN_C_EXIT }
#else
#define RTSW_EXTERN_C_ENTER
#define RTSW_EXTERN_C_EXIT
#endif

#define RTSW_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS 8

#endif
