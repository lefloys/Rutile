#include "resource/swapchain.h"
#include "context.h"
#include "error.h"
#include "resource/queue.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

const VkSurfaceFormatKHR rtvk_swapchain_format_preferences[] = {
	{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
	{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
	{ VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
	{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
};
const u32 rtvk_swapchain_format_preferences_count = (u32)(sizeof(rtvk_swapchain_format_preferences) / sizeof(rtvk_swapchain_format_preferences[0]));
const VkPresentModeKHR rtvk_swapchain_present_mode_preferences[] = {
	VK_PRESENT_MODE_MAILBOX_KHR,
	VK_PRESENT_MODE_IMMEDIATE_KHR,
	VK_PRESENT_MODE_FIFO_RELAXED_KHR,
};
const u32 rtvk_swapchain_present_mode_preferences_count = (u32)(sizeof(rtvk_swapchain_present_mode_preferences) / sizeof(rtvk_swapchain_present_mode_preferences[0]));

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

rt_swapchain rtSwapchainCreate(void) {
	return rtvk_swapchain_to_handle(rtvk_swapchain_create(rtvk_get_current_context()));
}

void rtSwapchainDestroy(rt_swapchain swapchain) {
	rtvk_swapchain_destroy(rtvk_get_current_context(), rtvk_swapchain_from_handle(swapchain));
}

void rtSwapchainResize(rt_swapchain swapchain, u32 width, u32 height) {
	rtvk_swapchain_resize(
		rtvk_get_current_context(),
		rtvk_swapchain_from_handle(swapchain),
		width,
		height
	);
}

rt_swapchain_acquire_result rtSwapchainAcquire(rt_swapchain swapchain) {
	return rtvk_swapchain_acquire(
		rtvk_get_current_context(),
		rtvk_swapchain_from_handle(swapchain)
	);
}

void rtSwapchainPresent(rt_swapchain swapchain, rt_timepoint rendered) {
	rtvk_swapchain_present(
		rtvk_get_current_context(),
		rtvk_swapchain_from_handle(swapchain),
		rendered
	);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTVK_DEFINE_RESOURCE_PRIVATE(swapchain)

void rtvk_swapchain_init(struct rtvk_context* ctx, struct rtvk_swapchain* swapchain) {
	rtvk_init_resource_base(ctx, RTVK_RESOURCE_BASE(swapchain), RT_RESOURCE_SWAPCHAIN);
	swapchain->frame_lock = rt_mutex_create();
	swapchain->frame_condition = rt_condition_create();
	if (!swapchain->frame_lock || !swapchain->frame_condition) {
		rtvk_throwf(RT_PLATFORM_FAILURE, "failed to create Vulkan swapchain synchronization");
	}
}

static void rtvk_swapchain_wait_frame(struct rtvk_context* ctx, struct rtvk_swapchain_frame* frame);


static struct rtvk_swapchain_surface* rtvk_swapchain_surface_create(struct rtvk_context* ctx, VkSurfaceKHR vk_surface) {
	struct rtvk_swapchain_surface* surface = RTVK_ALLOC_RESOURCE(struct rtvk_swapchain_surface);
	if (!surface) {
		return NULL;
	}
	rtvk_init_resource_base(ctx, RTVK_RESOURCE_BASE(surface), RT_RESOURCE_SWAPCHAIN_SURFACE);
	surface->vk_surface = vk_surface;
	return surface;
}

static struct rtvk_swapchain_generation* rtvk_swapchain_generation_create(struct rtvk_context* ctx) {
	struct rtvk_swapchain_generation* generation = RTVK_ALLOC_RESOURCE(struct rtvk_swapchain_generation);
	if (!generation) {
		return NULL;
	}
	rtvk_init_resource_base(ctx, RTVK_RESOURCE_BASE(generation), RT_RESOURCE_SWAPCHAIN_GENERATION);
	return generation;
}

static void rtvk_swapchain_generation_retire(struct rtvk_context* ctx, struct rtvk_swapchain_generation* generation) {
	if (!generation) {
		return;
	}
	if (generation->present_queue) {
		VkResult result = vkQueueWaitIdle(generation->present_queue->vk_queue);
		if (result != VK_SUCCESS) {
			rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
			return;
		}
	}

	for (u32 index = 0; index < generation->image_count; index++) {
		struct rtvk_swapchain_frame* frame = generation->frames ? generation->frames[index] : NULL;
		if (!frame) {
			continue;
		}
		rtvk_swapchain_wait_frame(ctx, frame);
		if (frame->framebuffer) {
			rtvk_framebuffer_set_color_view(ctx, frame->framebuffer, 0, NULL);
			rtvk_framebuffer_destroy(ctx, frame->framebuffer);
			frame->framebuffer = NULL;
		}
		if (frame->color_view) {
			rtvk_texture_view_destroy(ctx, frame->color_view);
			frame->color_view = NULL;
		}
		rtvk_resource_retire(RTVK_RESOURCE_BASE(frame));
		generation->frames[index] = NULL;
	}
	free(generation->frames);
	generation->frames = NULL;
	generation->image_count = 0;
	rtvk_resource_retire(RTVK_RESOURCE_BASE(generation));
}

void rtvk_swapchain_surface_finish(struct rtvk_swapchain_surface* surface) {
	struct rtvk_context* ctx = surface->base.ctx;
	if (surface->vk_surface) {
		vkDestroySurfaceKHR(ctx->vk_instance, surface->vk_surface, VK_ALLOCATOR);
		surface->vk_surface = VK_NULL_HANDLE;
	}
	rtvk_finish_resource_base(RTVK_RESOURCE_BASE(surface));
}

void rtvk_swapchain_generation_finish(struct rtvk_swapchain_generation* generation) {
	struct rtvk_context* ctx = generation->base.ctx;
	assert(!generation->frames);
	if (generation->vk_swapchain) {
		vkDestroySwapchainKHR(ctx->vk_device, generation->vk_swapchain, VK_ALLOCATOR);
		generation->vk_swapchain = VK_NULL_HANDLE;
	}
	if (generation->present_queue) {
		rtvk_release_resource(generation->present_queue);
	}
	if (generation->surface) {
		rtvk_release_resource(generation->surface);
	}
	rtvk_finish_resource_base(RTVK_RESOURCE_BASE(generation));
}

static void rtvk_swapchain_lock(struct rtvk_swapchain* swapchain) {
	rt_mutex_lock(swapchain->frame_lock);
}

static void rtvk_swapchain_unlock(struct rtvk_swapchain* swapchain) {
	rt_mutex_unlock(swapchain->frame_lock);
}

static void rtvk_swapchain_lock_unacquired(struct rtvk_swapchain* swapchain) {
	rtvk_swapchain_lock(swapchain);
	while (swapchain->frame_acquired) {
		rt_condition_wait(swapchain->frame_condition, swapchain->frame_lock);
	}
}

static void rtvk_swapchain_mark_unacquired_locked(struct rtvk_swapchain* swapchain) {
	swapchain->frame_acquired = false;
	if (swapchain->generation && swapchain->generation->image_count) {
		swapchain->generation->current_frame_index = (swapchain->generation->current_frame_index + 1) % swapchain->generation->image_count;
	}
	rt_condition_broadcast(swapchain->frame_condition);
}

static void rtvk_swapchain_mark_unacquired(struct rtvk_swapchain* swapchain) {
	rtvk_swapchain_lock(swapchain);
	rtvk_swapchain_mark_unacquired_locked(swapchain);
	rtvk_swapchain_unlock(swapchain);
}

static void rtvk_swapchain_release_acquired_image_locked(struct rtvk_context* ctx, struct rtvk_swapchain* swapchain, struct rtvk_swapchain_generation* generation, struct rtvk_swapchain_frame* frame) {
	VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &frame->image_available;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &generation->vk_swapchain;
	present_info.pImageIndices = &generation->current_image_index;
	present_info.pResults = NULL;
	(void)vkQueuePresentKHR(generation->present_queue->vk_queue, &present_info);
	rtvk_swapchain_mark_unacquired_locked(swapchain);
}

static void rtvk_swapchain_force_unacquired(struct rtvk_swapchain* swapchain) {
	rtvk_swapchain_lock(swapchain);
	swapchain->frame_acquired = false;
	rtvk_swapchain_unlock(swapchain);
}

static void rtvk_swapchain_finish_sync(struct rtvk_swapchain* swapchain) {
	rt_condition_destroy(swapchain->frame_condition);
	swapchain->frame_condition = NULL;
	rt_mutex_destroy(swapchain->frame_lock);
	swapchain->frame_lock = NULL;
}

static void rtvk_swapchain_destroy_present_command(struct rtvk_context* ctx, struct rtvk_swapchain_frame* frame) {
	vkDestroyCommandPool(ctx->vk_device, frame->present_command_pool, VK_ALLOCATOR);
	frame->present_command_pool = VK_NULL_HANDLE;
	frame->present_command_buffer = VK_NULL_HANDLE;
	frame->present_command_family_index = (u32)-1;
}

static void rtvk_swapchain_wait_frame(struct rtvk_context* ctx, struct rtvk_swapchain_frame* frame) {
	if (frame->acquire_wait.value) {
		rtvk_timepoint_wait(ctx, frame->acquire_wait);
		frame->acquire_wait = (rt_timepoint){ 0 };
	}

	if (frame->present_done.value) {
		rtvk_timepoint_wait(ctx, frame->present_done);
		frame->present_done = (rt_timepoint){ 0 };
	}
}

// Wait until all in-flight work touching any frame has settled. Called from
// the swapchain destroy path before we tear down frame semaphores/pools.
static void rtvk_swapchain_wait_all_frames(struct rtvk_context* ctx, struct rtvk_swapchain_generation* generation) {
	if (!generation || !generation->frames) {
		return;
	}
	for (u32 i = 0; i < generation->image_count; i++) {
		if (generation->frames[i]) {
			rtvk_swapchain_wait_frame(ctx, generation->frames[i]);
		}
	}
}

void rtvk_swapchain_frame_init(struct rtvk_context* ctx, struct rtvk_swapchain_frame* frame) {
	rtvk_init_resource_base(ctx, RTVK_RESOURCE_BASE(frame), RT_RESOURCE_SWAPCHAIN_FRAME);
	frame->base.vk_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	frame->base.vk_format = VK_FORMAT_UNDEFINED;
	frame->base.type = RT_TEXTURE_2D;
	frame->base.depth = 1;
	frame->base.mip_levels = 1;
}

// VkImage itself belongs to VkSwapchainKHR and is destroyed by that API.
void rtvk_swapchain_frame_finish(struct rtvk_swapchain_frame* frame) {
	struct rtvk_context* ctx = frame->base.base.ctx;
	if (frame->framebuffer) {
		rtvk_framebuffer_set_color_view(ctx, frame->framebuffer, 0, NULL);
	}
	if (frame->framebuffer) {
		rtvk_framebuffer_destroy(ctx, frame->framebuffer);
	}
	if (frame->color_view) {
		rtvk_texture_view_destroy(ctx, frame->color_view);
	}
	if (frame->image_available) {
		vkDestroySemaphore(ctx->vk_device, frame->image_available, VK_ALLOCATOR);
	}
	if (frame->present_ready) {
		vkDestroySemaphore(ctx->vk_device, frame->present_ready, VK_ALLOCATOR);
	}
	if (frame->present_command_pool) {
		vkDestroyCommandPool(ctx->vk_device, frame->present_command_pool, VK_ALLOCATOR);
	}
	frame->image_available = VK_NULL_HANDLE;
	frame->present_ready = VK_NULL_HANDLE;
	frame->present_command_pool = VK_NULL_HANDLE;
	frame->present_command_buffer = VK_NULL_HANDLE;
	frame->color_view = NULL;
	frame->framebuffer = NULL;
	frame->base.vk_image = VK_NULL_HANDLE;
	if (frame->generation) {
		rtvk_release_resource(frame->generation);
	}
	rtvk_finish_resource_base(RTVK_RESOURCE_BASE(frame));
}

void rtvk_swapchain_finish(struct rtvk_swapchain* swapchain) {
	struct rtvk_context* ctx = swapchain->base.ctx;
	rtvk_swapchain_force_unacquired(swapchain);
	if (swapchain->generation) {
		rtvk_swapchain_generation_retire(ctx, swapchain->generation);
		swapchain->generation = NULL;
	}
	if (swapchain->surface) {
		rtvk_resource_retire(RTVK_RESOURCE_BASE(swapchain->surface));
		swapchain->surface = NULL;
	}
	rtvk_swapchain_finish_sync(swapchain);
	rtvk_finish_resource_base(RTVK_RESOURCE_BASE(swapchain));
}

static void rtvk_swapchain_create_framebuffers(struct rtvk_context* ctx, struct rtvk_swapchain_generation* generation) {
	VkImage* images = NULL;
	u32 image_count = 0;

	VkResult result = vkGetSwapchainImagesKHR(ctx->vk_device, generation->vk_swapchain, &image_count, NULL);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "vkGetSwapchainImagesKHR failed while counting swapchain images");
		return;
	}

	images = calloc(image_count, sizeof(*images));
	RTVK_CHECK_ALLOC(images, (usize)image_count * sizeof(*images), "swapchain image list");
	if (rtvk_error() != RT_SUCCESS) {
		goto cleanup;
	}

	generation->frames = calloc(image_count, sizeof(*generation->frames));
	RTVK_CHECK_ALLOC(generation->frames, (usize)image_count * sizeof(*generation->frames), "swapchain frame list");
	if (rtvk_error() != RT_SUCCESS) {
		goto cleanup;
	}

	result = vkGetSwapchainImagesKHR(ctx->vk_device, generation->vk_swapchain, &image_count, images);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "vkGetSwapchainImagesKHR failed while fetching swapchain images");
		goto cleanup;
	}

	generation->image_count = image_count;
	for (u32 i = 0; i < image_count; i++) {
		struct rtvk_swapchain_frame* frame = calloc(1, sizeof(*frame));
		if (!frame) {
			rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate swapchain frame");
			goto cleanup;
		}
		rtvk_swapchain_frame_init(ctx, frame);
		frame->base.vk_image = images[i];
		frame->base.vk_format = generation->vk_format;
		frame->base.width = generation->extent.width;
		frame->base.height = generation->extent.height;
		frame->generation = generation;
		rtvk_retain_resource(generation);
		generation->frames[i] = frame;

		frame->color_view = rtvk_texture_view_create(ctx);
		if (!frame->color_view) {
			goto cleanup;
		}
		rtvk_texture_view_bind_image(ctx, frame->color_view, &frame->base);
		if (rtvk_error() != RT_SUCCESS) {
			goto cleanup;
		}
		frame->framebuffer = rtvk_framebuffer_create(ctx);
		if (!frame->framebuffer) {
			goto cleanup;
		}
		rtvk_framebuffer_set_color_view(ctx, frame->framebuffer, 0, frame->color_view);
		if (rtvk_error() != RT_SUCCESS) {
			goto cleanup;
		}
	}

cleanup:
	free(images);
}

static void rtvk_swapchain_create_frame_sync(struct rtvk_context* ctx, struct rtvk_swapchain_generation* generation) {
	// Frames must already be allocated by rtvk_swapchain_create_framebuffers.
	assert(generation->frames);

	VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	semaphore_info.pNext = NULL;
	semaphore_info.flags = 0;

	for (u32 i = 0; i < generation->image_count; i++) {
		struct rtvk_swapchain_frame* frame = generation->frames[i];
		frame->present_command_family_index = (u32)-1;

		VkResult result = vkCreateSemaphore(ctx->vk_device, &semaphore_info, VK_ALLOCATOR, &frame->image_available);
		if (result != VK_SUCCESS) {
			rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
			return;
		}

		result = vkCreateSemaphore(ctx->vk_device, &semaphore_info, VK_ALLOCATOR, &frame->present_ready);
		if (result != VK_SUCCESS) {
			rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
			return;
		}
	}
}

static VkSurfaceFormatKHR rtvk_choose_swapchain_format(VkSurfaceFormatKHR* formats, u32 format_count) {
	for (u32 p = 0; p < rtvk_swapchain_format_preferences_count; p++) {
		for (u32 i = 0; i < format_count; i++) {
			if (formats[i].format == rtvk_swapchain_format_preferences[p].format && formats[i].colorSpace == rtvk_swapchain_format_preferences[p].colorSpace) {
				return formats[i];
			}
		}
	}

	return format_count ? formats[0] : (VkSurfaceFormatKHR){ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
}

static VkPresentModeKHR rtvk_choose_swapchain_present_mode(VkPresentModeKHR* present_modes, u32 present_mode_count) {
	for (u32 p = 0; p < rtvk_swapchain_present_mode_preferences_count; p++) {
		for (u32 i = 0; i < present_mode_count; i++) {
			if (present_modes[i] == rtvk_swapchain_present_mode_preferences[p]) {
				return present_modes[i];
			}
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D rtvk_swapchain_choose_extent(VkSurfaceCapabilitiesKHR capabilities, u32 width, u32 height) {
	VkExtent2D extent;
	if (capabilities.currentExtent.width != 0xffffffffu) {
		return capabilities.currentExtent;
	}

	extent.width = width;
	extent.height = height;
	if (extent.width < capabilities.minImageExtent.width) {
		extent.width = capabilities.minImageExtent.width;
	}
	if (extent.width > capabilities.maxImageExtent.width) {
		extent.width = capabilities.maxImageExtent.width;
	}
	if (extent.height < capabilities.minImageExtent.height) {
		extent.height = capabilities.minImageExtent.height;
	}
	if (extent.height > capabilities.maxImageExtent.height) {
		extent.height = capabilities.maxImageExtent.height;
	}
	return extent;
}

static struct rtvk_swapchain_generation* rtvk_swapchain_generation_build(
	struct rtvk_context* ctx,
	struct rtvk_swapchain_surface* surface_owner,
	u32 width,
	u32 height,
	VkSwapchainKHR old_swapchain
) {
	VkSurfaceKHR surface = surface_owner->vk_surface;
	struct rtvk_swapchain_generation* generation;
	generation = rtvk_swapchain_generation_create(ctx);
	if (!generation) {
		return NULL;
	}
	generation->surface = surface_owner;
	rtvk_retain_resource(surface_owner);
	generation->present_queue = rtvk_queue_query_present(ctx, surface);
	if (!generation->present_queue) {
		if (rtvk_error() == RT_SUCCESS) {
			rtvk_throwf(RT_UNSUPPORTED_PLATFORM, NULL);
		}
		goto cleanup;
	}
	rtvk_retain_resource(generation->present_queue);

	VkSurfaceCapabilitiesKHR capabilities;
	VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->vk_physical_device, surface, &capabilities);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		goto cleanup;
	}

	u32 format_count = 0;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_physical_device, surface, &format_count, NULL);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		goto cleanup;
	}

	VkSurfaceFormatKHR* formats = calloc(format_count, sizeof(*formats));
	if (!formats) {
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %u Vulkan surface formats", format_count);
		goto cleanup;
	}
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_physical_device, surface, &format_count, formats);
	if (result != VK_SUCCESS) {
		free(formats);
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		goto cleanup;
	}

	u32 present_mode_count = 0;
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->vk_physical_device, surface, &present_mode_count, NULL);
	if (result != VK_SUCCESS) {
		free(formats);
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		goto cleanup;
	}
	if (present_mode_count == 0) {
		free(formats);
		rtvk_throwf(RT_INCOMPATIBLE_DRIVER, NULL);
		goto cleanup;
	}

	VkPresentModeKHR* present_modes = calloc(present_mode_count, sizeof(*present_modes));
	if (!present_modes) {
		free(formats);
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %u Vulkan present modes", present_mode_count);
		goto cleanup;
	}
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->vk_physical_device, surface, &present_mode_count, present_modes);
	if (result != VK_SUCCESS) {
		free(formats);
		free(present_modes);
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		goto cleanup;
	}

	VkSurfaceFormatKHR format = rtvk_choose_swapchain_format(formats, format_count);
	VkPresentModeKHR present_mode = rtvk_choose_swapchain_present_mode(present_modes, present_mode_count);
	free(formats);
	free(present_modes);

	u32 image_count = RTVK_MAX_FRAMES_IN_FLIGHT;
	if (image_count < capabilities.minImageCount) {
		image_count = capabilities.minImageCount;
	}
	if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
		image_count = capabilities.maxImageCount;
	}

	VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	if ((capabilities.supportedUsageFlags & image_usage) != image_usage) {
		rtvk_throwf(RT_UNSUPPORTED_FEATURE, "swapchain images do not support transfer source usage");
		goto cleanup;
	}

	VkSwapchainCreateInfoKHR swapchain_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchain_info.pNext = NULL;
	swapchain_info.flags = 0;
	swapchain_info.surface = surface;
	swapchain_info.minImageCount = image_count;
	swapchain_info.imageFormat = format.format;
	swapchain_info.imageColorSpace = format.colorSpace;
	swapchain_info.imageExtent = rtvk_swapchain_choose_extent(capabilities, width, height);
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = image_usage;

	struct rtvk_queue* graphics_queue = rtvk_queue_query(ctx, RT_QUEUE_GRAPHICS);
	u32 queue_family_indices[2];
	if (graphics_queue && graphics_queue->family_index != generation->present_queue->family_index) {
		queue_family_indices[0] = graphics_queue->family_index;
		queue_family_indices[1] = generation->present_queue->family_index;
		swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_info.queueFamilyIndexCount = 2;
		swapchain_info.pQueueFamilyIndices = queue_family_indices;
	} else {
		swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_info.queueFamilyIndexCount = 0;
		swapchain_info.pQueueFamilyIndices = NULL;
	}
	swapchain_info.preTransform = capabilities.currentTransform;
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.presentMode = present_mode;
	swapchain_info.clipped = VK_TRUE;
	swapchain_info.oldSwapchain = old_swapchain;

	result = vkCreateSwapchainKHR(ctx->vk_device, &swapchain_info, VK_ALLOCATOR, &generation->vk_swapchain);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		goto cleanup;
	}

	generation->vk_format = format.format;
	generation->extent = swapchain_info.imageExtent;
	rtvk_swapchain_create_framebuffers(ctx, generation);
	if (rtvk_error() != RT_SUCCESS) {
		goto cleanup;
	}
	rtvk_swapchain_create_frame_sync(ctx, generation);
	if (rtvk_error() != RT_SUCCESS) {
		goto cleanup;
	}
	return generation;

cleanup:
	rtvk_swapchain_generation_retire(ctx, generation);
	return NULL;
}

void rtvk_swapchain_init_from_surface(struct rtvk_context* ctx, struct rtvk_swapchain* swapchain, VkSurfaceKHR surface, u32 width, u32 height) {
	struct rtvk_swapchain_surface* surface_owner = rtvk_swapchain_surface_create(ctx, surface);
	if (!surface_owner) {
		return;
	}
	swapchain->generation = rtvk_swapchain_generation_build(ctx, surface_owner, width, height, VK_NULL_HANDLE);
	if (!swapchain->generation) {
		rtvk_resource_retire(RTVK_RESOURCE_BASE(surface_owner));
		return;
	}
	swapchain->surface = surface_owner;
}

void rtvk_swapchain_prepare_present_command(
	struct rtvk_context* ctx,
	struct rtvk_swapchain_frame* frame,
	u32 family_index
) {
	if (frame->present_command_pool && frame->present_command_family_index != family_index) {
		rtvk_swapchain_destroy_present_command(ctx, frame);
	}
	if (frame->present_command_pool) {
		return;
	}

	VkCommandPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	pool_info.pNext = NULL;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = family_index;

	VkResult result = vkCreateCommandPool(ctx->vk_device, &pool_info, VK_ALLOCATOR, &frame->present_command_pool);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return;
	}

	VkCommandBufferAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocate_info.pNext = NULL;
	allocate_info.commandPool = frame->present_command_pool;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = 1;

	result = vkAllocateCommandBuffers(ctx->vk_device, &allocate_info, &frame->present_command_buffer);
	if (result != VK_SUCCESS) {
		rtvk_swapchain_destroy_present_command(ctx, frame);
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return;
	}

	frame->present_command_family_index = family_index;
}

void rtvk_swapchain_submit_present_transition(struct rtvk_context* ctx, struct rtvk_swapchain_generation* generation, struct rtvk_swapchain_frame* frame, rt_timepoint rendered) {
	struct rtvk_swapchain_frame* current = generation->frames[generation->current_image_index];
	struct rtvk_queue* rendered_queue = rtvk_timepoint_queue(ctx, rendered);
	u64 rendered_value = rtvk_timepoint_value(rendered);
	if (!current || !rendered_queue) {
		return;
	}
	rtvk_swapchain_prepare_present_command(ctx, frame, rendered_queue->family_index);
	if (rtvk_error() != RT_SUCCESS) {
		return;
	}

	VkResult result = vkResetCommandPool(ctx->vk_device, frame->present_command_pool, 0);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return;
	}

	VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	begin_info.pNext = NULL;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begin_info.pInheritanceInfo = NULL;

	result = vkBeginCommandBuffer(frame->present_command_buffer, &begin_info);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return;
	}

	rtvk_image_transition_layout(frame->present_command_buffer, &current->base, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	result = vkEndCommandBuffer(frame->present_command_buffer);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return;
	}

	if (rendered_value > rendered_queue->submitted_value) {
		rtvk_queue_flush(ctx, rendered_queue);
	}

	u64 signal_value = rendered_queue->timeline_value + 1;
	u64 signal_values[2] = { 0, signal_value };
	VkSemaphore signal_semaphores[2] = { frame->present_ready, rendered_queue->vk_timeline };

	VkTimelineSemaphoreSubmitInfo timeline_info = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
	timeline_info.pNext = NULL;
	timeline_info.waitSemaphoreValueCount = 1;
	timeline_info.pWaitSemaphoreValues = &rendered_value;
	timeline_info.signalSemaphoreValueCount = 2;
	timeline_info.pSignalSemaphoreValues = signal_values;

	VkPipelineStageFlags wait_stage = rtvk_queue_wait_stage(rendered_queue);
	VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submit_info.pNext = &timeline_info;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &rendered_queue->vk_timeline;
	submit_info.pWaitDstStageMask = &wait_stage;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &frame->present_command_buffer;
	submit_info.signalSemaphoreCount = 2;
	submit_info.pSignalSemaphores = signal_semaphores;

	result = vkQueueSubmit(rendered_queue->vk_queue, 1, &submit_info, VK_NULL_HANDLE);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return;
	}

	struct rtvk_texture_view* color_view = current->color_view;
	if (color_view && color_view->image) {
		// Post-present, the swapchain image is in PRESENT_SRC_KHR. Sync the
		// backing frame's layout so the next transition uses the correct
		// oldLayout.
		color_view->image->vk_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}
	rendered_queue->timeline_value = signal_value;
	rendered_queue->submitted_value = signal_value;
	frame->present_done = rtvk_timepoint_make(rendered_queue, signal_value);
}

bool rtvk_swapchain_resize(struct rtvk_context* ctx, struct rtvk_swapchain* swapchain, u32 width, u32 height) {
	if (!swapchain || !swapchain->surface || !swapchain->generation) {
		rtvk_throwf(RT_IMPROPER_USAGE, "swapchain resize requires a valid swapchain");
		return false;
	}
	if (width == 0 || height == 0) {
		return false;
	}

	rtvk_swapchain_lock_unacquired(swapchain);
	struct rtvk_swapchain_generation* old_generation = swapchain->generation;
	if (width == old_generation->extent.width && height == old_generation->extent.height) {
		rtvk_swapchain_unlock(swapchain);
		return true;
	}

	struct rtvk_swapchain_generation* new_generation = rtvk_swapchain_generation_build(
		ctx,
		swapchain->surface,
		width,
		height,
		old_generation->vk_swapchain
	);
	if (new_generation) {
		swapchain->generation = new_generation;
		rtvk_swapchain_generation_retire(ctx, old_generation);
	}
	rtvk_swapchain_unlock(swapchain);
	return new_generation != NULL;
}

rt_swapchain_acquire_result rtvk_swapchain_acquire(struct rtvk_context* ctx, struct rtvk_swapchain* swapchain) {
	rt_swapchain_acquire_result acquire = { 0 };
	if (!swapchain) {
		rtvk_throwf(RT_IMPROPER_USAGE, "swapchain acquire requires a valid swapchain");
		return acquire;
	}
	rtvk_swapchain_lock_unacquired(swapchain);
	struct rtvk_swapchain_generation* generation = swapchain->generation;
	if (!generation || !generation->frames || generation->image_count == 0) {
		rtvk_swapchain_unlock(swapchain);
		rtvk_throwf(RT_IMPROPER_USAGE, "swapchain has no sync frames");
		return acquire;
	}
	struct rtvk_swapchain_frame* acquire_frame = generation->frames[generation->current_frame_index];
	rtvk_swapchain_wait_frame(ctx, acquire_frame);

	/* Comment ** finite timeout: the spec forbids UINT64_MAX when forward progress
	 * cannot be guaranteed (VUID-vkAcquireNextImageKHR-surface-07783). One second is
	 * long enough that a healthy frame pipeline never trips it, short enough that a
	 * deadlock surfaces as VK_TIMEOUT instead of hanging the app. */
	VkResult result = vkAcquireNextImageKHR(ctx->vk_device, generation->vk_swapchain, 1000000000ull, acquire_frame->image_available, VK_NULL_HANDLE, &generation->current_image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		/* Window resizing can invalidate the surface between the GLFW resize
		 * callback and this acquire. This is a transient no-frame condition,
		 * not an application error; the pending resize will rebuild the
		 * swapchain and the render loop can retry on its next iteration. */
		rtvk_swapchain_unlock(swapchain);
		return acquire;
	}
	if (result == VK_TIMEOUT || result == VK_NOT_READY) {
		/* Occlusion and interactive resizing may temporarily make forward
		 * progress unavailable. Match the out-of-date path and skip a frame. */
		rtvk_swapchain_unlock(swapchain);
		return acquire;
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		rtvk_throwf(rtvk_error_from_vk(result), "swapchain acquire failed: vkAcquireNextImageKHR returned %s", rtvk_vk_result_name(result));
		rtvk_swapchain_unlock(swapchain);
		return acquire;
	}

	swapchain->frame_acquired = true;
	acquire_frame->acquire_wait = rtvk_queue_wait_binary(ctx, generation->present_queue, acquire_frame->image_available);
	if (rtvk_error() != RT_SUCCESS) {
		acquire_frame->acquire_wait = (rt_timepoint){ 0 };
		rtvk_swapchain_release_acquired_image_locked(ctx, swapchain, generation, acquire_frame);
		rtvk_swapchain_unlock(swapchain);
		return acquire;
	}
	if (acquire_frame->acquire_wait.value == 0) {
		rtvk_throwf(RT_PLATFORM_FAILURE, "swapchain acquire failed: acquire wait is invalid");
		acquire_frame->acquire_wait = (rt_timepoint){ 0 };
		rtvk_swapchain_release_acquired_image_locked(ctx, swapchain, generation, acquire_frame);
		rtvk_swapchain_unlock(swapchain);
		return acquire;
	}

	acquire.timepoint = acquire_frame->acquire_wait;
	acquire.framebuffer = rtvk_framebuffer_to_handle(generation->frames[generation->current_image_index]->framebuffer);
	if (!acquire.timepoint.value || !acquire.framebuffer) {
		rtvk_throwf(RT_PLATFORM_FAILURE, "swapchain acquire failed: null timepoint");
		rtvk_swapchain_release_acquired_image_locked(ctx, swapchain, generation, acquire_frame);
		rtvk_swapchain_unlock(swapchain);
		return (rt_swapchain_acquire_result){ 0 };
	}
	rtvk_swapchain_unlock(swapchain);
	return acquire;
}

void rtvk_swapchain_present(struct rtvk_context* ctx, struct rtvk_swapchain* swapchain, rt_timepoint rendered) {
	if (!swapchain || !swapchain->generation || !swapchain->generation->frames || swapchain->generation->image_count == 0 || !swapchain->frame_acquired) {
		rtvk_throwf(RT_IMPROPER_USAGE, "swapchain present requires an acquired frame");
		return;
	}
	struct rtvk_swapchain_generation* generation = swapchain->generation;
	struct rtvk_swapchain_frame* frame = generation->frames[generation->current_frame_index];
	if (!rtvk_timepoint_queue(ctx, rendered) || rendered.value == 0) {
		rtvk_throwf(RT_IMPROPER_USAGE, "swapchain present requires a render timepoint");
		rtvk_swapchain_mark_unacquired(swapchain);
		return;
	}
	rtvk_swapchain_submit_present_transition(ctx, generation, frame, rendered);
	if (rtvk_error() != RT_SUCCESS) {
		rtvk_swapchain_mark_unacquired(swapchain);
		return;
	}

	VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	present_info.pNext = NULL;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &frame->present_ready;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &generation->vk_swapchain;
	present_info.pImageIndices = &generation->current_image_index;
	present_info.pResults = NULL;

	VkResult result = vkQueuePresentKHR(generation->present_queue->vk_queue, &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		rtvk_swapchain_mark_unacquired(swapchain);
		return;
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		rtvk_swapchain_mark_unacquired(swapchain);
		return;
	}

	rtvk_swapchain_mark_unacquired(swapchain);
}
