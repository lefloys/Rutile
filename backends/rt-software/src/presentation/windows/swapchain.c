#include "swapchain.h"

#include "context.h"
#include "error.h"
#include "glfw.h"
#include "resource/texture.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

static const char rtsw_swapchain_window_property[] = "RutileSoftwareSwapchain";

static void rtsw_swapchain_blit(struct rtsw_swapchain* swapchain, HDC destination, int width, int height) {
	if (swapchain->presentation_dc && width > 0 && height > 0) {
		StretchBlt(destination, 0, 0, width, height, (HDC)swapchain->presentation_dc,
			0, 0, (int)swapchain->width, (int)swapchain->height, SRCCOPY);
	}
}

static LRESULT CALLBACK rtsw_swapchain_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	struct rtsw_swapchain* swapchain = GetPropA(window, rtsw_swapchain_window_property);
	WNDPROC previous = swapchain ? (WNDPROC)swapchain->previous_window_procedure : DefWindowProcA;

	if (!swapchain) {
		return DefWindowProcA(window, message, wparam, lparam);
	}
	if (message == WM_ERASEBKGND) {
		return 1;
	}
	if (message == WM_PAINT) {
		PAINTSTRUCT paint;
		HDC destination = BeginPaint(window, &paint);
		rtsw_swapchain_blit(swapchain, destination, paint.rcPaint.right - paint.rcPaint.left, paint.rcPaint.bottom - paint.rcPaint.top);
		EndPaint(window, &paint);
		return 0;
	}
	return CallWindowProcA(previous, window, message, wparam, lparam);
}

static void rtsw_swapchain_release_presentation(struct rtsw_swapchain* swapchain) {
	if (swapchain->presentation_dc && swapchain->presentation_old_bitmap) {
		SelectObject((HDC)swapchain->presentation_dc, (HGDIOBJ)swapchain->presentation_old_bitmap);
	}
	if (swapchain->presentation_bitmap) {
		DeleteObject((HGDIOBJ)swapchain->presentation_bitmap);
	}
	if (swapchain->presentation_dc) {
		DeleteDC((HDC)swapchain->presentation_dc);
	}
	swapchain->presentation_dc = NULL;
	swapchain->presentation_bitmap = NULL;
	swapchain->presentation_old_bitmap = NULL;
	swapchain->presentation_pixels = NULL;
}

static bool rtsw_swapchain_create_presentation(struct rtsw_swapchain* swapchain, u32 width, u32 height) {
	BITMAPINFO bitmap_info = { 0 };
	HDC window_dc;

	bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
	bitmap_info.bmiHeader.biWidth = (LONG)width;
	bitmap_info.bmiHeader.biHeight = -(LONG)height;
	bitmap_info.bmiHeader.biPlanes = 1;
	bitmap_info.bmiHeader.biBitCount = 32;
	bitmap_info.bmiHeader.biCompression = BI_RGB;

	window_dc = GetDC((HWND)swapchain->window);
	if (!window_dc) {
		rtsw_throwf(RT_PLATFORM_FAILURE, "GetDC failed while creating a swapchain bitmap");
		return false;
	}
	swapchain->presentation_dc = CreateCompatibleDC(window_dc);
	swapchain->presentation_bitmap = CreateDIBSection(window_dc, &bitmap_info, DIB_RGB_COLORS, (void**)&swapchain->presentation_pixels, NULL, 0);
	ReleaseDC((HWND)swapchain->window, window_dc);
	if (!swapchain->presentation_dc || !swapchain->presentation_bitmap || !swapchain->presentation_pixels) {
		rtsw_swapchain_release_presentation(swapchain);
		rtsw_throwf(RT_OUT_OF_HOST_MEMORY, "failed to create a CPU presentation bitmap");
		return false;
	}
	swapchain->presentation_old_bitmap = SelectObject((HDC)swapchain->presentation_dc, (HGDIOBJ)swapchain->presentation_bitmap);
	return true;
}

static void rtsw_swapchain_release_images(struct rtsw_swapchain* swapchain) {
	rtsw_swapchain_release_presentation(swapchain);
	if (swapchain->framebuffer) {
		rtFramebufferDestroy(rtsw_framebuffer_to_handle(swapchain->framebuffer));
		swapchain->framebuffer = NULL;
	}
	if (swapchain->color_view) {
		rtTextureViewDestroy(rtsw_texture_view_to_handle(swapchain->color_view));
		swapchain->color_view = NULL;
	}
	if (swapchain->color_texture) {
		rtTextureDestroy(rtsw_texture_to_handle(swapchain->color_texture));
		swapchain->color_texture = NULL;
	}
	swapchain->width = 0;
	swapchain->height = 0;
}

static void rtsw_swapchain_finalize_resource(void* value) {
	struct rtsw_swapchain* swapchain = value;
	rtsw_swapchain_release_images(swapchain);
	free(swapchain);
}

static bool rtsw_swapchain_create_images(struct rtsw_swapchain* swapchain, u32 width, u32 height) {
	rt_texture texture;
	rt_texture_view view;
	rt_framebuffer framebuffer;

	texture = rtTextureCreate();
	if (!texture) {
		return false;
	}
	rtTextureResize(texture, RT_TEXTURE_2D, RT_RGBA8_UNORM, (rt_extent_3d){ width, height, 1 }, 1);
	if (rtsw_error() != RT_SUCCESS) {
		rtTextureDestroy(texture);
		return false;
	}

	view = rtTextureViewCreate();
	if (!view) {
		rtTextureDestroy(texture);
		return false;
	}
	rtTextureViewSetTexture(view, texture);
	if (rtsw_error() != RT_SUCCESS) {
		rtTextureViewDestroy(view);
		rtTextureDestroy(texture);
		return false;
	}

	framebuffer = rtFramebufferCreate();
	if (!framebuffer) {
		rtTextureViewDestroy(view);
		rtTextureDestroy(texture);
		return false;
	}
	rtFramebufferSetColorView(framebuffer, view, NULL);
	if (rtsw_error() != RT_SUCCESS) {
		rtFramebufferDestroy(framebuffer);
		rtTextureViewDestroy(view);
		rtTextureDestroy(texture);
		return false;
	}
	if (!rtsw_swapchain_create_presentation(swapchain, width, height)) {
		rtFramebufferDestroy(framebuffer);
		rtTextureViewDestroy(view);
		rtTextureDestroy(texture);
		return false;
	}

	swapchain->color_texture = rtsw_texture_from_handle(texture);
	swapchain->color_view = rtsw_texture_view_from_handle(view);
	swapchain->framebuffer = rtsw_framebuffer_from_handle(framebuffer);
	swapchain->width = width;
	swapchain->height = height;
	return true;
}

static bool rtsw_swapchain_resize(struct rtsw_swapchain* swapchain, u32 width, u32 height) {
	if (!swapchain || !swapchain->window || !width || !height || swapchain->frame_acquired) {
		rtsw_throwf(RT_IMPROPER_USAGE, "swapchain resize requires a bound, unacquired swapchain and a non-zero extent");
		return false;
	}
	if (swapchain->width == width && swapchain->height == height) {
		return true;
	}
	rtsw_swapchain_release_images(swapchain);
	return rtsw_swapchain_create_images(swapchain, width, height);
}

RTSW_DEFINE_HANDLE(swapchain, rtsw_swapchain)

rt_swapchain rtSwapchainCreate(void) {
	struct rtsw_context* context = rtsw_get_current_context();
	struct rtsw_swapchain* swapchain;
	rtsw_clear_error();

	if (!context || !context->flags.presentation) {
		rtsw_throwf(RT_FEATURE_NOT_SUPPORTED, "rtSwapchainCreate requires RT_FEATURE_PRESENTATION at rtInit");
		return NULL;
	}
	swapchain = RTSW_ALLOC_RESOURCE(struct rtsw_swapchain);
	if (!swapchain) {
		return NULL;
	}
	rtsw_init_resource_base(context, &swapchain->base, swapchain, rtsw_swapchain_finalize_resource);
	return rtsw_swapchain_to_handle(swapchain);
}

void rtSwapchainDestroy(rt_swapchain handle) {
	struct rtsw_swapchain* swapchain = rtsw_swapchain_from_handle(handle);
	rtsw_clear_error();
	if (!swapchain) {
		return;
	}
	if (swapchain->frame_acquired) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtSwapchainDestroy called while a frame is acquired");
		return;
	}
	if (swapchain->window && swapchain->previous_window_procedure) {
		SetWindowLongPtrA((HWND)swapchain->window, GWLP_WNDPROC, (LONG_PTR)swapchain->previous_window_procedure);
		RemovePropA((HWND)swapchain->window, rtsw_swapchain_window_property);
	}
	rtsw_resource_retire(&swapchain->base);
}

void rtSwapchainResize(rt_swapchain handle, u32 width, u32 height) {
	rtsw_clear_error();
	rtsw_swapchain_resize(rtsw_swapchain_from_handle(handle), width, height);
}

void rtSwapchainBindGLFW(rt_swapchain handle, struct GLFWwindow* window) {
	struct rtsw_swapchain* swapchain = rtsw_swapchain_from_handle(handle);
	int width;
	int height;
	void* native_window;
	rtsw_clear_error();

	if (!swapchain || swapchain->window || swapchain->frame_acquired || !window) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtSwapchainBindGLFW requires an unbound, unacquired swapchain and a GLFW window");
		return;
	}
	if (!rtsw_glfw_framebuffer_size(window, &width, &height) || width <= 0 || height <= 0) {
		if (rtsw_error() == RT_SUCCESS) {
			rtsw_throwf(RT_IMPROPER_USAGE, "GLFW window framebuffer extent must be positive");
		}
		return;
	}
	native_window = rtsw_glfw_win32_window(window);
	if (!native_window) {
		if (rtsw_error() == RT_SUCCESS) {
			rtsw_throwf(RT_PLATFORM_FAILURE, "glfwGetWin32Window returned NULL");
		}
		return;
	}
	swapchain->window = native_window;
	if (!rtsw_swapchain_create_images(swapchain, (u32)width, (u32)height)) {
		swapchain->window = NULL;
		return;
	}
	SetPropA((HWND)swapchain->window, rtsw_swapchain_window_property, swapchain);
	SetLastError(0);
	swapchain->previous_window_procedure = (void*)SetWindowLongPtrA(
		(HWND)swapchain->window, GWLP_WNDPROC, (LONG_PTR)rtsw_swapchain_window_procedure
	);
	if (!swapchain->previous_window_procedure && GetLastError()) {
		RemovePropA((HWND)swapchain->window, rtsw_swapchain_window_property);
		rtsw_swapchain_release_images(swapchain);
		swapchain->window = NULL;
		rtsw_throwf(RT_PLATFORM_FAILURE, "SetWindowLongPtrA failed while binding the swapchain window");
	}
}

rt_swapchain_acquire_result rtSwapchainAcquire(rt_swapchain handle) {
	struct rtsw_swapchain* swapchain = rtsw_swapchain_from_handle(handle);
	RECT client;
	u32 width;
	u32 height;
	rtsw_clear_error();

	if (!swapchain || !swapchain->window || swapchain->frame_acquired) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtSwapchainAcquire requires a bound swapchain without an acquired frame");
		return (rt_swapchain_acquire_result){ 0 };
	}
	if (!GetClientRect((HWND)swapchain->window, &client)) {
		rtsw_throwf(RT_PLATFORM_FAILURE, "GetClientRect failed while acquiring a swapchain frame");
		return (rt_swapchain_acquire_result){ 0 };
	}
	width = (u32)(client.right - client.left);
	height = (u32)(client.bottom - client.top);
	if (!width || !height) {
		return (rt_swapchain_acquire_result){ 0 };
	}
	if (!rtsw_swapchain_resize(swapchain, width, height)) {
		return (rt_swapchain_acquire_result){ 0 };
	}
	swapchain->frame_acquired = true;
	return (rt_swapchain_acquire_result){ rtsw_framebuffer_to_handle(swapchain->framebuffer), { 0 } };
}

void rtSwapchainPresent(rt_swapchain handle, rt_timepoint rendered) {
	struct rtsw_swapchain* swapchain = rtsw_swapchain_from_handle(handle);
	struct rtsw_texture* texture;
	HDC device_context;
	usize pixel_count;
	rtsw_clear_error();

	if (!swapchain || !swapchain->frame_acquired || !swapchain->window || !swapchain->color_texture || !rendered.value) {
		rtsw_throwf(RT_IMPROPER_USAGE, "rtSwapchainPresent requires an acquired frame and a render timepoint");
		return;
	}
	texture = swapchain->color_texture;
	pixel_count = (usize)swapchain->width * swapchain->height;
	for (usize pixel = 0; pixel < pixel_count; ++pixel) {
		const u08* source = texture->bytes + pixel * 4;
		u08* destination = swapchain->presentation_pixels + pixel * 4;
		destination[0] = source[2];
		destination[1] = source[1];
		destination[2] = source[0];
		destination[3] = 0xff;
	}

	device_context = GetDC((HWND)swapchain->window);
	if (!device_context) {
		rtsw_throwf(RT_PLATFORM_FAILURE, "GetDC failed while presenting a swapchain frame");
		swapchain->frame_acquired = false;
		return;
	}
	rtsw_swapchain_blit(swapchain, device_context, (int)swapchain->width, (int)swapchain->height);
	ReleaseDC((HWND)swapchain->window, device_context);
	swapchain->frame_acquired = false;
}
