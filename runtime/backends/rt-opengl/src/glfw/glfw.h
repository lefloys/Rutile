#ifndef RTGL_GLFW_GLFW_H
#define RTGL_GLFW_GLFW_H

#include "platform/context.h"

typedef struct GLFWwindow GLFWwindow;

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

RTGL_EXTERN_C_ENTER

void rtgl_init_glfw_platform(void);
struct gl_surface* rtgl_create_glfw_surface(struct gl_context* context, GLFWwindow* window);

#if defined(_WIN32)
HWND glfwGetWin32Window(GLFWwindow* window);
#endif
void glfwGetFramebufferSize(GLFWwindow* window, int* width, int* height);

RTGL_EXTERN_C_EXIT
#endif /* RTGL_GLFW_GLFW_H */
