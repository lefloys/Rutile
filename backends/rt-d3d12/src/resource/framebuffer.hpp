#pragma once

#include "texture.hpp"

inline constexpr u32 RTD3D12_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS = 8;

struct rt_framebuffer_t : rtd3d12_resource<rt_framebuffer_t> {
	explicit rt_framebuffer_t(rtd3d12_context* ctx) : rtd3d12_resource(ctx) {}
	~rt_framebuffer_t();
	rt_texture_view_t* color_views[RTD3D12_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS]{};
	rt_texture_view_t* depth_view{};
	rt_texture_view_t* stencil_view{};
	u32 color_texture_count{};
};
RTD3D12_API rt_framebuffer_t* rtFramebufferCreate();
RTD3D12_API void rtFramebufferDestroy(rt_framebuffer_t* framebuffer);
RTD3D12_API rt_texture_view_t* rtFramebufferColorView(rt_framebuffer_t* framebuffer, rt::location* location);
RTD3D12_API void rtFramebufferSetColorView(rt_framebuffer_t* framebuffer, rt_texture_view_t* view, rt::location* location);
RTD3D12_API void rtFramebufferSetDepthView(rt_framebuffer_t* framebuffer, rt_texture_view_t* view);
RTD3D12_API void rtFramebufferSetStencilView(rt_framebuffer_t* framebuffer, rt_texture_view_t* view);

bool rtd3d12_texture_view_valid(rt_texture_view_t* view);
void rtd3d12_framebuffer_set_color_view(rtd3d12_context* ctx, rt_framebuffer_t* framebuffer, u32 slot, rt_texture_view_t* view);
void rtd3d12_framebuffer_set_depth_view(rtd3d12_context* ctx, rt_framebuffer_t* framebuffer, rt_texture_view_t* view);
void rtd3d12_framebuffer_set_stencil_view(rtd3d12_context* ctx, rt_framebuffer_t* framebuffer, rt_texture_view_t* view);
rt_texture_view_t* rtd3d12_framebuffer_color_view(rt_framebuffer_t* framebuffer, u32 slot);
bool rtd3d12_framebuffer_valid(rt_framebuffer_t* framebuffer);
