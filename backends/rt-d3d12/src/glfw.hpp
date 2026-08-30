#pragma once

#include "config.hpp"

#include "rutile.hpp"

struct GLFWwindow;

RTD3D12_API bool rtInit_GLFW(void);

void rtd3d12_glfw_get_framebuffer_size(GLFWwindow* window, int* width, int* height);
void* rtd3d12_glfw_get_native_window(GLFWwindow* window);
