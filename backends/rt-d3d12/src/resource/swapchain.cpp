#include "resource/swapchain.hpp"
#include "context.hpp"
#include "error.hpp"
#include "glfw.hpp"

#include <new>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

bool rtd3d12_presentation_resize(rtd3d12_context* ctx, rtd3d12_presentation* presentation, u32 width, u32 height, DXGI_FORMAT format);
bool rtd3d12_presentation_get_image(rtd3d12_context* ctx, rtd3d12_presentation* presentation, u32 index, ID3D12Resource** image);
u32 rtd3d12_presentation_current_image_index(rtd3d12_presentation* presentation);
bool rtd3d12_presentation_present(rtd3d12_presentation* presentation, bool vsync);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

rt_swapchain_t* rtSwapchainCreate(void) {
	rtd3d12_begin_errorable_operation();
	struct rt_swapchain_t* swapchain = rtd3d12::create_resource<rt_swapchain_t>(rtd3d12_get_current_context());
	if (swapchain) {
		rtd3d12_swapchain_init(rtd3d12_get_current_context(), swapchain);
	}
	return swapchain;
}

void rtSwapchainDestroy(rt_swapchain_t* swapchain) {
	if (swapchain) swapchain->retire();
}

void rtSwapchainResize(rt_swapchain_t* swapchain, u32 width, u32 height) {
	rtd3d12_begin_errorable_operation();
	rtd3d12_swapchain_resize(
		rtd3d12_get_current_context(),
		swapchain,
		width,
		height
	);
}

rt::swapchain_acquire_result rtSwapchainAcquire(rt_swapchain_t* swapchain) {
	rtd3d12_begin_errorable_operation();
	return rtd3d12_swapchain_acquire(
		rtd3d12_get_current_context(),
		swapchain
	);
}

void rtSwapchainPresent(rt_swapchain_t* swapchain, rt::timepoint rendered) {
	rtd3d12_begin_errorable_operation();
	rtd3d12_swapchain_present(rtd3d12_get_current_context(), swapchain, rendered);
}

void rtSwapchainBindGLFW(rt_swapchain_t* swapchain, GLFWwindow* window) {
	rtd3d12_begin_errorable_operation();
	if (!window) {
		rtd3d12_fail(rt::error::improper_usage, "rtSwapchainBindGLFW window is nullptr");
		return;
	}

	rtd3d12_context* ctx = rtd3d12_get_current_context();
	rt_swapchain_t* target = swapchain;
	if (!ctx || !target) {
		rtd3d12_fail(rt::error::improper_usage, "swapchain and context must be valid");
		return;
	}

	int width = 0;
	int height = 0;
	rtd3d12_glfw_get_framebuffer_size(window, &width, &height);

	rtd3d12_presentation* presentation = rtd3d12::allocate<rtd3d12_presentation>();
	if (!presentation) {
		return;
	}
	BOOL allow_tearing = FALSE;
	if (SUCCEEDED(ctx->dxgi_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(allow_tearing)))) {
		presentation->allow_tearing = allow_tearing == TRUE;
	}

	DXGI_SWAP_CHAIN_DESC1 swapchain_info = {};
	swapchain_info.Width = static_cast<u32>(width);
	swapchain_info.Height = static_cast<u32>(height);
	swapchain_info.Format = target->dxgi_format;
	swapchain_info.Stereo = FALSE;
	swapchain_info.SampleDesc.Count = 1;
	swapchain_info.SampleDesc.Quality = 0;
	swapchain_info.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchain_info.BufferCount = RTD3D12_MAX_FRAMES_IN_FLIGHT;
	swapchain_info.Scaling = DXGI_SCALING_STRETCH;
	swapchain_info.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchain_info.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchain_info.Flags = presentation->allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	rt_queue_t* queue = rtd3d12_context_queue(ctx, rt::queue_capability::graphics);
	IDXGISwapChain1* swapchain1 = nullptr;
	HWND hwnd = static_cast<HWND>(rtd3d12_glfw_get_native_window(window));
	HRESULT result = ctx->dxgi_factory->CreateSwapChainForHwnd(queue->d3d_queue, hwnd, &swapchain_info, nullptr, nullptr, &swapchain1);
	if (FAILED(result)) {
		delete presentation;
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateSwapChainForHwnd failed: 0x{:08x}", static_cast<u32>(result));
		return;
	}

	ctx->dxgi_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
	result = swapchain1->QueryInterface(IID_PPV_ARGS(&presentation->dxgi_swapchain));
	swapchain1->Release();
	if (FAILED(result)) {
		delete presentation;
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "IDXGISwapChain3 QueryInterface failed: 0x{:08x}", static_cast<u32>(result));
		return;
	}

	target->presentation = presentation;
	target->width = static_cast<u32>(width);
	target->height = static_cast<u32>(height);
	target->current_image_index = rtd3d12_presentation_current_image_index(presentation);
	if (!rtd3d12_swapchain_create_framebuffers(ctx, target)) {
		delete presentation;
		target->presentation = nullptr;
	}
}

bool rtd3d12_presentation_resize(rtd3d12_context*, rtd3d12_presentation* presentation, u32 width, u32 height, DXGI_FORMAT format) {
	if (!presentation || !presentation->dxgi_swapchain) {
		rtd3d12_fail(rt::error::improper_usage, "Direct3D 12 swapchain presentation is unavailable");
		return false;
	}

	HRESULT result = presentation->dxgi_swapchain->ResizeBuffers(
		RTD3D12_MAX_FRAMES_IN_FLIGHT,
		width,
		height,
		format,
		presentation->allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0
	);
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "IDXGISwapChain3::ResizeBuffers failed: {} (0x{:08x})", rtd3d12_hresult_name(result), static_cast<u32>(result));
		return false;
	}

	return true;
}

bool rtd3d12_presentation_get_image(rtd3d12_context*, rtd3d12_presentation* presentation, u32 index, ID3D12Resource** image) {
	if (!presentation || !presentation->dxgi_swapchain || !image) {
		rtd3d12_fail(rt::error::improper_usage, "Direct3D 12 swapchain presentation image request is invalid");
		return false;
	}

	HRESULT result = presentation->dxgi_swapchain->GetBuffer(index, IID_PPV_ARGS(image));
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "IDXGISwapChain3::GetBuffer failed: {} (0x{:08x})", rtd3d12_hresult_name(result), static_cast<u32>(result));
		return false;
	}

	return true;
}

u32 rtd3d12_presentation_current_image_index(rtd3d12_presentation* presentation) {
	return presentation && presentation->dxgi_swapchain ? presentation->dxgi_swapchain->GetCurrentBackBufferIndex() : 0;
}

bool rtd3d12_presentation_present(rtd3d12_presentation* presentation, bool vsync) {
	if (!presentation || !presentation->dxgi_swapchain) {
		rtd3d12_fail(rt::error::improper_usage, "Direct3D 12 swapchain presentation is unavailable");
		return false;
	}

	const UINT sync_interval = vsync ? 1u : 0u;
	const UINT present_flags = !vsync && presentation->allow_tearing ? DXGI_PRESENT_ALLOW_TEARING : 0u;
	HRESULT result = presentation->dxgi_swapchain->Present(sync_interval, present_flags);
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "IDXGISwapChain::Present failed: {} (0x{:08x})", rtd3d12_hresult_name(result), static_cast<u32>(result));
		return false;
	}

	return true;
}

rtd3d12_presentation::~rtd3d12_presentation() {
	if (dxgi_swapchain) {
		dxgi_swapchain->Release();
		dxgi_swapchain = nullptr;
	}
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

void rtd3d12_swapchain_init(struct rtd3d12_context* ctx, struct rt_swapchain_t* swapchain) {
	(void)ctx;
	swapchain->dxgi_format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapchain->vsync = false;
}

static void rtd3d12_swapchain_lock(struct rt_swapchain_t* swapchain) {
	swapchain->frame_lock.lock();
}

static void rtd3d12_swapchain_unlock(struct rt_swapchain_t* swapchain) {
	swapchain->frame_lock.unlock();
}

static void rtd3d12_swapchain_lock_unacquired(struct rt_swapchain_t* swapchain) {
	rtd3d12_swapchain_lock(swapchain);
	while (swapchain->frame_acquired) {
		std::unique_lock<std::mutex> lock(swapchain->frame_lock, std::adopt_lock);
		swapchain->frame_condition.wait(lock);
		lock.release();
	}
}

static void rtd3d12_swapchain_mark_unacquired(struct rt_swapchain_t* swapchain) {
	rtd3d12_swapchain_lock(swapchain);
	swapchain->frame_acquired = false;
	swapchain->frame_condition.notify_all();
	rtd3d12_swapchain_unlock(swapchain);
}

void rtd3d12_swapchain_wait_frame(struct rtd3d12_context* ctx, struct rtd3d12_swapchain_frame* frame) {
	if (!frame) {
		return;
	}
	if (frame->present_timepoint.value) {
		rtd3d12_wait_for_timepoint(ctx, frame->present_timepoint);
		frame->present_timepoint = {};
	}
}

static void rtd3d12_swapchain_destroy_framebuffers(struct rtd3d12_context* ctx, struct rt_swapchain_t* swapchain) {
	for (u32 i = 0; i < RTD3D12_MAX_FRAMES_IN_FLIGHT; i++) {
		struct rtd3d12_swapchain_frame* frame = &swapchain->frames[i];
		if (frame->framebuffer) {
			frame->framebuffer->retire();
			frame->framebuffer = nullptr;
		}
		// The texture/view wrappers may still be retained by a user command buffer that recorded
		// into the previous back buffer. ResizeBuffers requires zero outstanding references to the
		// swapchain's ID3D12Resource objects, so drop the back-buffer pointer directly here — the
		// wrappers' ref-counted cleanup will then find a nullptr resource and skip the release.
		if (frame->color_view) {
			if (frame->color_view->image) {
				frame->color_view->image->d3d_resource = nullptr;
			}
			frame->color_view->retire();
			frame->color_view = nullptr;
		}
		if (frame->texture) {
			if (frame->texture->active) {
				if (frame->texture->active->d3d_resource) {
					frame->texture->active->d3d_resource->Release();
					frame->texture->active->d3d_resource = nullptr;
				}
			}
			frame->texture->retire();
			frame->texture = nullptr;
		}
	}
}

bool rtd3d12_swapchain_resize(struct rtd3d12_context* ctx, struct rt_swapchain_t* swapchain, u32 width, u32 height) {
	if (!swapchain || !swapchain->presentation) {
		rtd3d12_fail(rt::error::improper_usage, "swapchain resize requires a valid swapchain");
		return false;
	}
	if (width == 0 || height == 0) {
		return false;
	}
	if (width == swapchain->width && height == swapchain->height) {
		return true;
	}

	rtd3d12_swapchain_lock_unacquired(swapchain);
	for (u32 i = 0; i < RTD3D12_MAX_FRAMES_IN_FLIGHT; i++) {
		rtd3d12_swapchain_wait_frame(ctx, &swapchain->frames[i]);
	}

	rtd3d12_swapchain_destroy_framebuffers(ctx, swapchain);
	if (swapchain->rtv_heap) {
		swapchain->rtv_heap->Release();
		swapchain->rtv_heap = nullptr;
	}

	if (!rtd3d12_presentation_resize(ctx, swapchain->presentation, width, height, swapchain->dxgi_format)) {
		rtd3d12_swapchain_unlock(swapchain);
		return false;
	}

	swapchain->width = width;
	swapchain->height = height;
	swapchain->current_image_index = rtd3d12_presentation_current_image_index(swapchain->presentation);
	bool ok = rtd3d12_swapchain_create_framebuffers(ctx, swapchain);
	rtd3d12_swapchain_unlock(swapchain);
	return ok;
}

void rtd3d12_swapchain_set_vsync(struct rtd3d12_context* ctx, struct rt_swapchain_t* swapchain, bool enabled) {
	(void)ctx;
	if (!swapchain) {
		return;
	}
	swapchain->vsync = enabled;
}

rt_swapchain_t::~rt_swapchain_t() {
	rtd3d12_swapchain_lock(this);
	frame_acquired = false;
	for (u32 i = 0; i < RTD3D12_MAX_FRAMES_IN_FLIGHT; ++i) {
		rtd3d12_swapchain_wait_frame(ctx, &frames[i]);
	}
	rtd3d12_swapchain_destroy_framebuffers(ctx, this);
	if (rtv_heap) {
		rtv_heap->Release();
		rtv_heap = nullptr;
	}
	delete presentation;
	presentation = nullptr;
	rtd3d12_swapchain_unlock(this);
}

bool rtd3d12_swapchain_create_framebuffers(struct rtd3d12_context* ctx, struct rt_swapchain_t* swapchain) {
	D3D12_DESCRIPTOR_HEAP_DESC heap_info = {};
	heap_info.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heap_info.NumDescriptors = RTD3D12_MAX_FRAMES_IN_FLIGHT;
	heap_info.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heap_info.NodeMask = 0;

	HRESULT result = ctx->d3d_device->CreateDescriptorHeap(&heap_info, IID_PPV_ARGS(&swapchain->rtv_heap));
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateDescriptorHeap(RTV) failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}

	swapchain->rtv_descriptor_size = ctx->d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapchain->rtv_heap->GetCPUDescriptorHandleForHeapStart();
	for (u32 i = 0; i < RTD3D12_MAX_FRAMES_IN_FLIGHT; i++) {
		struct rtd3d12_swapchain_frame* frame = &swapchain->frames[i];
		ID3D12Resource* image = nullptr;
		if (!rtd3d12_presentation_get_image(ctx, swapchain->presentation, i, &image)) {
			return false;
		}

		D3D12_RENDER_TARGET_VIEW_DESC rtv_info = {};
		rtv_info.Format = swapchain->dxgi_format;
		rtv_info.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtv_info.Texture2D.MipSlice = 0;
		rtv_info.Texture2D.PlaneSlice = 0;

		ctx->d3d_device->CreateRenderTargetView(image, &rtv_info, rtv);
		frame->texture = rtd3d12_texture_create_for_swapchain_image(ctx, image, rtv_info.Format, swapchain->width, swapchain->height);
		image->Release();
		if (!frame->texture) {
			rtd3d12_swapchain_destroy_framebuffers(ctx, swapchain);
			return false;
		}

		frame->color_view = rtd3d12_texture_view_create_for_swapchain(ctx, frame->texture, rtv);
		if (!frame->color_view) {
			rtd3d12_swapchain_destroy_framebuffers(ctx, swapchain);
			return false;
		}

		frame->framebuffer = rtd3d12::create_resource<rt_framebuffer_t>(ctx);
		if (!frame->framebuffer) {
			rtd3d12_swapchain_destroy_framebuffers(ctx, swapchain);
			return false;
		}

		rtd3d12_framebuffer_set_color_view(ctx, frame->framebuffer, 0, frame->color_view);
		if (rtError() != rt::error::success) {
			rtd3d12_swapchain_destroy_framebuffers(ctx, swapchain);
			return false;
		}

		rtv.ptr += swapchain->rtv_descriptor_size;
	}

	return true;
}

rt::swapchain_acquire_result rtd3d12_swapchain_acquire(struct rtd3d12_context* ctx, struct rt_swapchain_t* swapchain) {
	rt::swapchain_acquire_result empty = { rt::null_handle, { 0 } };
	if (!swapchain || !swapchain->presentation) {
		rtd3d12_fail(rt::error::improper_usage, "swapchain acquire requires a valid swapchain");
		return empty;
	}

	rtd3d12_swapchain_lock_unacquired(swapchain);
	swapchain->current_image_index = rtd3d12_presentation_current_image_index(swapchain->presentation);
	/* The caller establishes the dependency before submitting work for this
	 * image. Do not CPU-wait here: that would discard the frame completion
	 * signal and prevent normal per-frame command-buffer reuse. */
	rt::timepoint available = swapchain->frames[swapchain->current_image_index].present_timepoint;
	struct rtd3d12_swapchain_frame* frame = &swapchain->frames[swapchain->current_image_index];
	if (!frame->framebuffer) {
		rtd3d12_fail(rt::error::initialization_failed, "swapchain framebuffer {} is unavailable", swapchain->current_image_index);
		rtd3d12_swapchain_unlock(swapchain);
		return empty;
	}
	rt::swapchain_acquire_result acquire = {
		frame->framebuffer,
		available,
	};
	swapchain->frame_acquired = true;
	rtd3d12_swapchain_unlock(swapchain);
	return acquire;
}

void rtd3d12_swapchain_present(struct rtd3d12_context* ctx, struct rt_swapchain_t* swapchain, rt::timepoint rendered) {
	struct rtd3d12_swapchain_frame* frame = &swapchain->frames[swapchain->current_image_index];
	if (!rendered.value) {
		rtd3d12_fail(rt::error::improper_usage, "swapchain present requires a render timepoint");
		rtd3d12_swapchain_mark_unacquired(swapchain);
		return;
	}

	rt_queue_t* queue = rtd3d12_queue_from_timepoint(ctx, rendered);
	if (!queue || !queue->d3d_queue || !queue->d3d_fence) {
		rtd3d12_fail(rt::error::improper_usage, "swapchain present uses a timepoint from an unknown queue");
		rtd3d12_swapchain_mark_unacquired(swapchain);
		return;
	}

	if (!rtd3d12_presentation_present(swapchain->presentation, swapchain->vsync)) {
		rtd3d12_swapchain_mark_unacquired(swapchain);
		return;
	}

	rt::timepoint present_done = rtd3d12_queue_signal(ctx, queue);
	if (rtError() != rt::error::success) {
		rtd3d12_swapchain_mark_unacquired(swapchain);
		return;
	}
	frame->present_timepoint = present_done;
	rtd3d12_swapchain_mark_unacquired(swapchain);
}
