#pragma once

#include <windows.h>

struct GLFWwindow;

void rtdx_init_glfw_platform();
HWND rtdx_glfw_get_hwnd(GLFWwindow* window);
void rtdx_glfw_get_framebuffer_size(GLFWwindow* window, int* width, int* height);
