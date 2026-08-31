#ifndef RTSW_WINDOWS_GLFW_H
#define RTSW_WINDOWS_GLFW_H

#include "config.h"

#include <stdbool.h>

struct GLFWwindow;

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTSW_API bool rtInit_GLFW(void);

bool rtsw_glfw_initialize(void);
bool rtsw_glfw_framebuffer_size(struct GLFWwindow* window, int* width, int* height);
void* rtsw_glfw_win32_window(struct GLFWwindow* window);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

#endif /* RTSW_WINDOWS_GLFW_H */
