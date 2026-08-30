#ifndef RTGL_GLFW_H
#define RTGL_GLFW_H

#include "platform/context.h"

typedef struct GLFWwindow GLFWwindow;

RTGL_EXTERN_C_ENTER

RTGL_API bool rtInit_GLFW(void);

void rtgl_init_glfw_platform(void);
struct gl_surface* rtgl_create_glfw_surface(struct gl_context* context, GLFWwindow* window);
void rtgl_glfw_get_framebuffer_size(GLFWwindow* window, int* width, int* height);

RTGL_EXTERN_C_EXIT
#endif /* RTGL_GLFW_H */
