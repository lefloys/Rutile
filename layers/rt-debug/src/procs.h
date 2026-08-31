#ifndef RTDBG_PROCS_H
#define RTDBG_PROCS_H

#include "rutile.h"
#include "rt_swapchain.h"
#include "rt_glfw_swapchain.h"

#if defined(_WIN32)
#define RT_API_PUBLIC __declspec(dllexport)
#else
#define RT_API_PUBLIC __attribute__((visibility("default")))
#endif

#endif
