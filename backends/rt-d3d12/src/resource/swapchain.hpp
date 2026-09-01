#pragma once

#include "config.hpp"
#include "resource/framebuffer.hpp"
#include "resource/queue.hpp"
#include "resource/texture.hpp"
#include "sync.hpp"

#include <dxgi1_6.h>
#include <condition_variable>
#include <mutex>

struct GLFWwindow;

RTD3D12_API rt_swapchain_t* rtSwapchainCreate();
RTD3D12_API void rtSwapchainDestroy(rt_swapchain_t* swapchain);
RTD3D12_API void rtSwapchainResize(rt_swapchain_t* swapchain, u32 width, u32 height);
RTD3D12_API void rtSwapchainSetVsync(rt_swapchain_t* swapchain, bool enabled);
RTD3D12_API rt::swapchain_acquire_result rtSwapchainAcquire(rt_swapchain_t* swapchain);
RTD3D12_API void rtSwapchainPresent(rt_swapchain_t* swapchain, rt::timepoint rendered);
RTD3D12_API void rtSwapchainBindGLFW(rt_swapchain_t* swapchain, GLFWwindow* window);

struct rtd3d12_swapchain_frame {
	rt_texture_t* texture;
	rt_texture_view_t* color_view;
	rt_framebuffer_t* framebuffer;

	rt::timepoint present_timepoint;
};

struct rtd3d12_presentation {
	~rtd3d12_presentation();
	IDXGISwapChain3* dxgi_swapchain{};
	bool allow_tearing{};
};

struct rt_swapchain_t : rtd3d12_resource<rt_swapchain_t> {
	explicit rt_swapchain_t(rtd3d12_context* ctx) : rtd3d12_resource(ctx) {}
	~rt_swapchain_t();

	rtd3d12_presentation* presentation{};
	rtd3d12_swapchain_frame frames[RTD3D12_MAX_FRAMES_IN_FLIGHT]{};
	ID3D12DescriptorHeap* rtv_heap{};
	usize width{};
	usize height{};
	usize current_image_index{};
	DXGI_FORMAT dxgi_format{};
	DXGI_FORMAT rtv_format{};
	u32 rtv_descriptor_size{};
	bool frame_acquired{};
	bool vsync{};
	std::mutex frame_lock;
	std::condition_variable frame_condition;
};

void rtd3d12_swapchain_init(rtd3d12_context* ctx, rt_swapchain_t* swapchain);
void rtd3d12_swapchain_attach_presentation(rtd3d12_context* ctx, rt_swapchain_t* swapchain, rtd3d12_presentation* presentation, u32 width, u32 height);
bool rtd3d12_swapchain_create_framebuffers(rtd3d12_context* ctx, rt_swapchain_t* swapchain);
bool rtd3d12_swapchain_resize(rtd3d12_context* ctx, rt_swapchain_t* swapchain, u32 width, u32 height);
rt::swapchain_acquire_result rtd3d12_swapchain_acquire(rtd3d12_context* ctx, rt_swapchain_t* swapchain);
void rtd3d12_swapchain_present(rtd3d12_context* ctx, rt_swapchain_t* swapchain, rt::timepoint rendered);
void rtd3d12_swapchain_wait_frame(rtd3d12_context* ctx, rtd3d12_swapchain_frame* frame);
