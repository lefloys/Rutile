#include "windows/glfw.hpp"

#include "context.hpp"
#include "error.hpp"
#include "resource/queue.hpp"
#include "resource/swapchain.hpp"

#include <cassert>
#include <new>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

using PFN_rtdx_glfwGetFramebufferSize = void (*)(GLFWwindow* window, int* width, int* height);
using PFN_rtdx_glfwGetWin32Window = HWND (*)(GLFWwindow* window);

struct rtdx_glfw_procs {
	PFN_rtdx_glfwGetFramebufferSize get_framebuffer_size = nullptr;
	PFN_rtdx_glfwGetWin32Window get_win32_window = nullptr;
	bool resolved = false;
};

static rtdx_glfw_procs rtdx_glfw;

struct rtdx_presentation {
	IDXGISwapChain3* dxgi_swapchain;
};

u32 rtdx_presentation_current_image_index(rtdx_presentation* presentation);
void rtdx_presentation_destroy(rtdx_context* ctx, rtdx_presentation* presentation);

static HMODULE rtdx_glfw_module() {
	HMODULE module = GetModuleHandleA("glfw3.dll");
	return module ? module : GetModuleHandleA(NULL);
}

static void* rtdx_glfw_symbol(HMODULE module, const char* name) {
	return module ? reinterpret_cast<void*>(GetProcAddress(module, name)) : nullptr;
}

static bool rtdx_glfw_resolve() {
	HMODULE module;

	if (rtdx_glfw.resolved) {
		return rtdx_glfw.get_framebuffer_size && rtdx_glfw.get_win32_window;
	}

	module = rtdx_glfw_module();
	rtdx_glfw.get_framebuffer_size = reinterpret_cast<PFN_rtdx_glfwGetFramebufferSize>(rtdx_glfw_symbol(module, "glfwGetFramebufferSize"));
	rtdx_glfw.get_win32_window = reinterpret_cast<PFN_rtdx_glfwGetWin32Window>(rtdx_glfw_symbol(module, "glfwGetWin32Window"));

	if (!rtdx_glfw.get_framebuffer_size || !rtdx_glfw.get_win32_window) {
		rtdx_throwf(RT_UNSUPPORTED_PLATFORM, "GLFW symbols are not exported by the executable or available from glfw3.dll");
		return false;
	}

	rtdx_glfw.resolved = true;
	return true;
}

RTDX_API bool rtInit_GLFW(void) {
	rtdx_glfw_resolve();
	return rtError() == RT_SUCCESS;
}

static HWND rtdx_glfw_get_hwnd(GLFWwindow* window) {
	assert(rtdx_glfw.get_win32_window);
	assert(window);
	return rtdx_glfw.get_win32_window(window);
}

static void rtdx_glfw_get_framebuffer_size(GLFWwindow* window, int* width, int* height) {
	assert(rtdx_glfw.get_framebuffer_size);
	assert(window);
	rtdx_glfw.get_framebuffer_size(window, width, height);
}

void rtSwapchainBindWindowGLFW(rt_swapchain swapchain, GLFWwindow* window) {
	if (!window) {
		rtdx_throwf(RT_IMPROPER_USAGE, "rtSwapchainBindWindowGLFW window is NULL");
		return;
	}

	int width = 0;
	int height = 0;
	rtdx_glfw_get_framebuffer_size(window, &width, &height);
	rtdx_presentation* presentation = new (std::nothrow) rtdx_presentation{};
	if (!presentation) {
		rtdx_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate Windows swapchain platform state");
		return;
	}

	rtdx_context* ctx = rtdx_get_current_context();
	rtdx_swapchain* target = rtdx_swapchain_from_handle(swapchain);
	if (!ctx || !target) {
		delete presentation;
		rtdx_throwf(RT_IMPROPER_USAGE, "swapchain and context must be valid");
		return;
	}

	DXGI_SWAP_CHAIN_DESC1 swapchain_info = {};
	swapchain_info.Width = (u32)width;
	swapchain_info.Height = (u32)height;
	swapchain_info.Format = target->dxgi_format;
	swapchain_info.Stereo = FALSE;
	swapchain_info.SampleDesc.Count = 1;
	swapchain_info.SampleDesc.Quality = 0;
	swapchain_info.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchain_info.BufferCount = RTDX_MAX_FRAMES_IN_FLIGHT;
	swapchain_info.Scaling = DXGI_SCALING_STRETCH;
	swapchain_info.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchain_info.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchain_info.Flags = ctx->allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	rtdx_queue* queue = rtdx_context_queue(ctx, RT_QUEUE_GRAPHICS);
	IDXGISwapChain1* swapchain1 = NULL;
	HRESULT result = ctx->dxgi_factory->CreateSwapChainForHwnd(queue->d3d_queue, rtdx_glfw_get_hwnd(window), &swapchain_info, NULL, NULL, &swapchain1);
	if (FAILED(result)) {
		delete presentation;
		rtdx_throwf(rtdx_error_from_hresult(result), "CreateSwapChainForHwnd failed: 0x%08x", (u32)result);
		return;
	}

	ctx->dxgi_factory->MakeWindowAssociation(rtdx_glfw_get_hwnd(window), DXGI_MWA_NO_ALT_ENTER);
	result = swapchain1->QueryInterface(IID_PPV_ARGS(&presentation->dxgi_swapchain));
	swapchain1->Release();
	if (FAILED(result)) {
		delete presentation;
		rtdx_throwf(rtdx_error_from_hresult(result), "IDXGISwapChain3 QueryInterface failed: 0x%08x", (u32)result);
		return;
	}

	target->presentation = presentation;
	target->width = (u32)width;
	target->height = (u32)height;
	target->current_image_index = rtdx_presentation_current_image_index(presentation);
	if (!rtdx_swapchain_create_framebuffers(ctx, target)) {
		rtdx_presentation_destroy(ctx, presentation);
		target->presentation = NULL;
	}
}

bool rtdx_presentation_resize(rtdx_context*, rtdx_presentation* presentation, u32 width, u32 height, DXGI_FORMAT format, bool allow_tearing) {
	if (!presentation || !presentation->dxgi_swapchain) {
		rtdx_throwf(RT_IMPROPER_USAGE, "Windows swapchain platform is unavailable");
		return false;
	}
	HRESULT result = presentation->dxgi_swapchain->ResizeBuffers(RTDX_MAX_FRAMES_IN_FLIGHT, width, height, format, allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);
	if (FAILED(result)) {
		rtdx_throwf(rtdx_error_from_hresult(result), "IDXGISwapChain3::ResizeBuffers failed: %s (0x%08x)", rtdx_hresult_name(result), (u32)result);
		return false;
	}
	return true;
}

bool rtdx_presentation_get_image(rtdx_context*, rtdx_presentation* presentation, u32 index, ID3D12Resource** image) {
	if (!presentation || !presentation->dxgi_swapchain || !image) {
		rtdx_throwf(RT_IMPROPER_USAGE, "Windows swapchain platform buffer request is invalid");
		return false;
	}
	HRESULT result = presentation->dxgi_swapchain->GetBuffer(index, IID_PPV_ARGS(image));
	if (FAILED(result)) {
		rtdx_throwf(rtdx_error_from_hresult(result), "IDXGISwapChain3::GetBuffer failed: %s (0x%08x)", rtdx_hresult_name(result), (u32)result);
		return false;
	}
	return true;
}

u32 rtdx_presentation_current_image_index(rtdx_presentation* presentation) {
	return presentation && presentation->dxgi_swapchain ? presentation->dxgi_swapchain->GetCurrentBackBufferIndex() : 0;
}

bool rtdx_presentation_present(rtdx_presentation* presentation, bool vsync, bool allow_tearing) {
	if (!presentation || !presentation->dxgi_swapchain) {
		rtdx_throwf(RT_IMPROPER_USAGE, "Windows swapchain platform is unavailable");
		return false;
	}
	const UINT sync_interval = vsync ? 1u : 0u;
	const UINT present_flags = !vsync && allow_tearing ? DXGI_PRESENT_ALLOW_TEARING : 0u;
	HRESULT result = presentation->dxgi_swapchain->Present(sync_interval, present_flags);
	if (FAILED(result)) {
		rtdx_throwf(rtdx_error_from_hresult(result), "IDXGISwapChain::Present failed: %s (0x%08x)", rtdx_hresult_name(result), (u32)result);
		return false;
	}
	return true;
}

void rtdx_presentation_destroy(rtdx_context*, rtdx_presentation* presentation) {
	if (!presentation) {
		return;
	}
	rtdx_release(presentation->dxgi_swapchain);
	delete presentation;
}
