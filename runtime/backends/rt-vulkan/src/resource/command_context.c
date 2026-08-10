#include "command_context.h"
#include "context.h"
#include "error.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

static void rtvk_command_context_reset_declarations(struct rtvk_command_context* command_context);
static void rtvk_command_context_wait_submitted(struct rtvk_context* ctx, struct rtvk_command_context* command_context);
static void rtvk_command_context_discard(struct rtvk_context* ctx, struct rtvk_command_context* command_context);
static void rtvk_command_context_retire_children(struct rtvk_context* ctx, struct rtvk_command_context* command_context);
static void rtvk_command_context_destroy_pool(struct rtvk_context* ctx, struct rtvk_command_context* command_context);
static bool rtvk_command_context_create_pool(struct rtvk_context* ctx, struct rtvk_command_context* command_context);
static bool rtvk_command_context_begin_primary(struct rtvk_context* ctx, struct rtvk_command_context* command_context);
static bool rtvk_command_context_open_rendering(struct rtvk_context* ctx, struct rtvk_command_context* command_context);
static struct rtvk_texture_view* rtvk_command_context_stencil_view(struct rtvk_command_context* command_context);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

rt_command_context rtCommandContextCreate(void) {
	return rtvk_command_context_to_handle(rtvk_command_context_create(rtvk_get_current_context()));
}

void rtCommandContextDestroy(rt_command_context command_context) {
	rtvk_command_context_destroy(rtvk_get_current_context(), rtvk_command_context_from_handle(command_context));
}

void rtCommandContextBind(rt_command_context command_context, rt_queue queue) {
	rtvk_command_context_bind(
		rtvk_get_current_context(),
		rtvk_command_context_from_handle(command_context),
		rtvk_queue_from_handle(queue)
	);
}

rt_command_buffer rtCommandContextAllocate(rt_command_context command_context) {
	return rtvk_command_buffer_to_handle(rtvk_command_context_allocate(
		rtvk_get_current_context(),
		rtvk_command_context_from_handle(command_context)
	));
}

void rtCommandContextBindFramebuffer(rt_command_context command_context, rt_framebuffer framebuffer) {
	rtvk_command_context_bind_framebuffer(
		rtvk_get_current_context(),
		rtvk_command_context_from_handle(command_context),
		rtvk_framebuffer_from_handle(framebuffer)
	);
}

void rtCommandContextClearColor(rt_command_context command_context, u32 color_index, f32 r, f32 g, f32 b, f32 a) {
	rtvk_command_context_clear_color(rtvk_command_context_from_handle(command_context), color_index, r, g, b, a);
}

void rtCommandContextClearDepth(rt_command_context command_context, f32 depth) {
	rtvk_command_context_clear_depth(rtvk_command_context_from_handle(command_context), depth);
}

void rtCommandContextClearStencil(rt_command_context command_context, u32 stencil) {
	rtvk_command_context_clear_stencil(rtvk_command_context_from_handle(command_context), stencil);
}

void rtCommandContextSetViewport(rt_command_context command_context, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth) {
	rtvk_command_context_set_viewport(rtvk_command_context_from_handle(command_context), x, y, width, height, min_depth, max_depth);
}

void rtCommandContextSetScissor(rt_command_context command_context, u32 x, u32 y, u32 width, u32 height) {
	rtvk_command_context_set_scissor(rtvk_command_context_from_handle(command_context), x, y, width, height);
}

void rtCommandContextExecute(rt_command_context command_context, rt_command_buffer command_buffer) {
	rtvk_command_context_execute(
		rtvk_get_current_context(),
		rtvk_command_context_from_handle(command_context),
		rtvk_command_buffer_from_handle(command_buffer)
	);
}

void rtCommandContextEndRendering(rt_command_context command_context) {
	rtvk_command_context_end_rendering(rtvk_command_context_from_handle(command_context));
}

rt_timepoint rtCommandContextSubmit(rt_command_context command_context) {
	return rtvk_timepoint_to_public(rtvk_command_context_submit(
		rtvk_get_current_context(),
		rtvk_command_context_from_handle(command_context)
	));
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtvk_command_context* rtvk_command_context_create(struct rtvk_context* ctx) {
	struct rtvk_command_context* command_context = RTVK_ALLOC_RESOURCE(struct rtvk_command_context);
	if (!command_context) {
		return NULL;
	}
	rtvk_command_context_init(ctx, command_context);
	if (!command_context->command_pool_lock) {
		rtvk_finish_resource_base(RTVK_RESOURCE_BASE(command_context));
		free(command_context);
		return NULL;
	}
	return command_context;
}

void rtvk_command_context_destroy(struct rtvk_context* ctx, struct rtvk_command_context* command_context) {
	(void)ctx;
	if (!command_context) {
		return;
	}
	rtvk_command_context_lock(command_context);
	if (!command_context->destroyed) {
		command_context->destroyed = true;
		rtvk_command_context_discard(command_context->base.ctx, command_context);
		rtvk_command_context_retire_children(command_context->base.ctx, command_context);
	}
	rtvk_command_context_unlock(command_context);
	rtvk_resource_retire(RTVK_RESOURCE_BASE(command_context));
}

void rtvk_command_context_init(struct rtvk_context* ctx, struct rtvk_command_context* command_context) {
	rtvk_init_resource_base(ctx, RTVK_RESOURCE_BASE(command_context), RT_RESOURCE_COMMAND_CONTEXT);
	command_context->command_pool_lock = rt_mutex_create();
	if (!command_context->command_pool_lock) {
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate command context pool lock");
	}
}

void rtvk_command_context_finish(struct rtvk_command_context* command_context) {
	struct rtvk_context* ctx = command_context->base.ctx;
	assert(ctx);
	rtvk_command_context_lock(command_context);
	rtvk_command_context_discard(ctx, command_context);
	rtvk_command_context_retire_children(ctx, command_context);
	if (command_context->primary) {
		command_context->primary->command_context = NULL;
		rtvk_command_buffer_destroy(ctx, command_context->primary);
	}
	rtvk_command_context_destroy_pool(ctx, command_context);
	rtvk_command_context_unlock(command_context);
	rt_mutex_destroy(command_context->command_pool_lock);
	command_context->command_pool_lock = NULL;
	rtvk_finish_resource_base(RTVK_RESOURCE_BASE(command_context));
}

void rtvk_command_context_bind(struct rtvk_context* ctx, struct rtvk_command_context* context, struct rtvk_queue* queue) {
	rtvk_command_context_lock(context);
	if (!context || context->destroyed) {
		rtvk_throwf(RT_IMPROPER_USAGE, "command context bind requires a live context");
		rtvk_command_context_unlock(context);
		return;
	}
	if (!context || !queue) {
		rtvk_throwf(RT_IMPROPER_USAGE, "command context bind requires valid context and queue");
		rtvk_command_context_unlock(context);
		return;
	}

	rtvk_command_context_discard(ctx, context);
	rtvk_command_context_destroy_pool(ctx, context);
	context->queue = queue;
	if (!rtvk_command_context_create_pool(ctx, context)) {
		context->queue = NULL;
		rtvk_command_context_unlock(context);
		return;
	}
	if (!rtvk_command_context_begin_primary(ctx, context)) {
		rtvk_command_buffer_discard(ctx, context->primary);
		rtvk_command_context_destroy_pool(ctx, context);
		context->queue = NULL;
	}
	rtvk_command_context_unlock(context);
}

struct rtvk_command_buffer* rtvk_command_context_allocate(struct rtvk_context* ctx, struct rtvk_command_context* context) {
	rtvk_command_context_lock(context);
	if (!context || context->destroyed || !context->queue || context->submitted) {
		rtvk_throwf(RT_IMPROPER_USAGE, "command buffer allocation requires a queue-bound, unsubmitted context");
		rtvk_command_context_unlock(context);
		return NULL;
	}

	struct rtvk_command_buffer* command_buffer = rtvk_command_buffer_create(ctx);
	if (!command_buffer) {
		rtvk_command_context_unlock(context);
		return NULL;
	}
	if (!command_buffer->command_context_lock) {
		rtvk_command_buffer_destroy(ctx, command_buffer);
		rtvk_command_context_unlock(context);
		return NULL;
	}
	command_buffer->command_context = context;
	command_buffer->secondary = true;
	rtvk_retain_resource(command_buffer);
	command_buffer->next_child = context->children;
	context->children = command_buffer;
	rtvk_command_context_unlock(context);
	return command_buffer;
}

void rtvk_command_context_bind_framebuffer(struct rtvk_context* ctx, struct rtvk_command_context* context, struct rtvk_framebuffer* framebuffer_node) {
	rtvk_command_context_lock(context);
	if (!context || context->destroyed || !context->queue || context->queue->capability != RT_QUEUE_GRAPHICS || context->submitted || context->rendering || !framebuffer_node) {
		rtvk_throwf(RT_IMPROPER_USAGE, "framebuffer bind requires a queue-bound context with no previous framebuffer scope");
		rtvk_command_context_unlock(context);
		return;
	}
	for (u32 i = 0; i < framebuffer_node->color_texture_count; i++) {
		if (!framebuffer_node->color_views[i] ||
			!(rtvk_texture_format_aspect(rtvk_view_format(framebuffer_node->color_views[i])) & VK_IMAGE_ASPECT_COLOR_BIT)) {
			rtvk_throwf(RT_IMPROPER_USAGE, "framebuffer has an incomplete color attachment sequence");
			return;
		}
	}
	if (framebuffer_node->depth_view &&
		!(rtvk_texture_format_aspect(rtvk_view_format(framebuffer_node->depth_view)) & VK_IMAGE_ASPECT_DEPTH_BIT)) {
		rtvk_throwf(RT_IMPROPER_USAGE, "framebuffer depth attachment must have a depth aspect");
		return;
	}
	if (framebuffer_node->stencil_view &&
		!(rtvk_texture_format_aspect(rtvk_view_format(framebuffer_node->stencil_view)) & VK_IMAGE_ASPECT_STENCIL_BIT)) {
		rtvk_throwf(RT_IMPROPER_USAGE, "framebuffer stencil attachment must have a stencil aspect");
		return;
	}
	rtvk_retain_resource(framebuffer_node);
	context->framebuffer = framebuffer_node;
	for (u32 i = 0; i < framebuffer_node->color_texture_count; i++) {
		context->color_views[i] = framebuffer_node->color_views[i];
		rtvk_retain_resource(context->color_views[i]);
	}
	context->color_view_count = framebuffer_node->color_texture_count;
	context->depth_view = framebuffer_node->depth_view;
	if (context->depth_view) {
		rtvk_retain_resource(context->depth_view);
	}
	context->stencil_view = framebuffer_node->stencil_view;
	if (context->stencil_view) {
		rtvk_retain_resource(context->stencil_view);
	}
	context->rendering = true;

	struct rtvk_texture_view* color_view = context->color_views[0];
	context->viewport_x = 0;
	context->viewport_y = 0;
	context->viewport_width = rtvk_view_width(color_view);
	context->viewport_height = rtvk_view_height(color_view);
	context->min_depth = 0.0f;
	context->max_depth = 1.0f;
	context->scissor_x = 0;
	context->scissor_y = 0;
	context->scissor_width = context->viewport_width;
	context->scissor_height = context->viewport_height;
}

void rtvk_command_context_clear_color(struct rtvk_command_context* context, u32 color_index, f32 r, f32 g, f32 b, f32 a) {
	if (!context || !context->rendering || rtvk_atomic_bool_load(&context->draw_packet_begun) || color_index >= context->color_view_count) {
		rtvk_throwf(RT_IMPROPER_USAGE, "color clear requires an unstarted framebuffer scope and valid attachment");
		return;
	}
	context->color_clears[color_index].color.float32[0] = r;
	context->color_clears[color_index].color.float32[1] = g;
	context->color_clears[color_index].color.float32[2] = b;
	context->color_clears[color_index].color.float32[3] = a;
	context->clear_colors[color_index] = true;
}

void rtvk_command_context_clear_depth(struct rtvk_command_context* context, f32 depth) {
	if (!context || !context->rendering || rtvk_atomic_bool_load(&context->draw_packet_begun) || !context->depth_view) {
		rtvk_throwf(RT_IMPROPER_USAGE, "depth clear requires an unstarted framebuffer scope with depth attachment");
		return;
	}
	context->depth_stencil_clear.depthStencil.depth = depth;
	context->clear_depth = true;
}

void rtvk_command_context_clear_stencil(struct rtvk_command_context* context, u32 stencil) {
	if (!context || !context->rendering || rtvk_atomic_bool_load(&context->draw_packet_begun) || !rtvk_command_context_stencil_view(context)) {
		rtvk_throwf(RT_IMPROPER_USAGE, "stencil clear requires an unstarted framebuffer scope with stencil attachment");
		return;
	}
	context->depth_stencil_clear.depthStencil.stencil = stencil;
	context->clear_stencil = true;
}

void rtvk_command_context_set_viewport(struct rtvk_command_context* context, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth) {
	if (!context || !context->rendering || rtvk_atomic_bool_load(&context->draw_packet_begun) || width == 0 || height == 0 || min_depth > max_depth) {
		rtvk_throwf(RT_IMPROPER_USAGE, "viewport requires an unstarted framebuffer scope and valid extent/depth range");
		return;
	}
	context->viewport_x = x;
	context->viewport_y = y;
	context->viewport_width = width;
	context->viewport_height = height;
	context->min_depth = min_depth;
	context->max_depth = max_depth;
}

void rtvk_command_context_set_scissor(struct rtvk_command_context* context, u32 x, u32 y, u32 width, u32 height) {
	if (!context || !context->rendering || rtvk_atomic_bool_load(&context->draw_packet_begun) || width == 0 || height == 0) {
		rtvk_throwf(RT_IMPROPER_USAGE, "scissor requires an unstarted framebuffer scope and valid extent");
		return;
	}
	context->scissor_x = x;
	context->scissor_y = y;
	context->scissor_width = width;
	context->scissor_height = height;
}

void rtvk_command_context_execute(struct rtvk_context* ctx, struct rtvk_command_context* context, struct rtvk_command_buffer* child) {
	if (!context || !child || child->command_context != context || !context->queue || !context->rendering || context->submitted ||
		child->recording || !child->executable || !child->active) {
		rtvk_throwf(RT_IMPROPER_USAGE, "command context execute requires an ended child in its active framebuffer scope");
		return;
	}
	if (!context->rendering_open && !rtvk_command_context_open_rendering(ctx, context)) {
		return;
	}

	VkCommandBuffer secondary = child->active->vk_command_buffer;
	vkCmdExecuteCommands(context->primary->active->vk_command_buffer, 1, &secondary);
	child->executed = true;
}

void rtvk_command_context_end_rendering(struct rtvk_command_context* context) {
	if (!context || !context->rendering || context->submitted || rtvk_command_context_child_recording(context)) {
		rtvk_throwf(RT_IMPROPER_USAGE, "ending rendering requires an active context scope with no recording child");
		return;
	}
	if (context->rendering_open) {
		vkCmdEndRendering(context->primary->active->vk_command_buffer);
		context->rendering_open = false;
	}
	context->rendering = false;
}

struct rtvk_timepoint rtvk_command_context_submit(struct rtvk_context* ctx, struct rtvk_command_context* context) {
	if (!context || !context->queue || !context->primary || !context->primary->recording || context->rendering || context->submitted ||
		rtvk_command_context_child_recording(context)) {
		rtvk_throwf(RT_IMPROPER_USAGE, "command context submit requires an ended, one-shot context with no recording child");
		return (struct rtvk_timepoint){ 0 };
	}

	rtvk_command_buffer_end(ctx, context->primary);
	if (rtvk_error() != RT_SUCCESS) {
		return (struct rtvk_timepoint){ 0 };
	}
	struct rtvk_timepoint timepoint = rtvk_queue_submit(ctx, context->queue, context->primary);
	if (rtvk_error() != RT_SUCCESS) {
		return (struct rtvk_timepoint){ 0 };
	}
	for (struct rtvk_command_buffer* child = context->children; child; child = child->next_child) {
		if (child->executed && child->active) {
			child->active->pending_timepoint = timepoint;
		}
	}
	context->submitted = true;
	context->submitted_timepoint = timepoint;
	return timepoint;
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

static void rtvk_command_context_reset_declarations(struct rtvk_command_context* command_context) {
	for (u32 i = 0; i < command_context->color_view_count; i++) {
		rtvk_release_resource(command_context->color_views[i]);
	}
	command_context->color_view_count = 0;
	if (command_context->depth_view) {
		rtvk_release_resource(command_context->depth_view);
	}
	if (command_context->stencil_view) {
		rtvk_release_resource(command_context->stencil_view);
	}
	command_context->framebuffer = NULL;
	memset(command_context->color_clears, 0, sizeof(command_context->color_clears));
	memset(command_context->clear_colors, 0, sizeof(command_context->clear_colors));
	command_context->depth_stencil_clear = (VkClearValue){ 0 };
	command_context->clear_depth = false;
	command_context->clear_stencil = false;
	rtvk_atomic_bool_store(&command_context->draw_packet_begun, false);
	command_context->rendering = false;
	command_context->rendering_open = false;
}

static void rtvk_command_context_wait_submitted(struct rtvk_context* ctx, struct rtvk_command_context* command_context) {
	if (command_context->submitted_timepoint.queue) {
		rtvk_timepoint_wait(ctx, command_context->submitted_timepoint);
		command_context->submitted_timepoint = (struct rtvk_timepoint){ 0 };
	}
}

static void rtvk_command_context_discard(struct rtvk_context* ctx, struct rtvk_command_context* command_context) {
	rtvk_command_context_wait_submitted(ctx, command_context);
	for (struct rtvk_command_buffer* child = command_context->children; child; child = child->next_child) {
		rtvk_command_buffer_discard(ctx, child);
	}
	if (command_context->primary) {
		rtvk_command_buffer_discard(ctx, command_context->primary);
	}
	if (command_context->framebuffer) {
		rtvk_release_resource(command_context->framebuffer);
	}
	rtvk_command_context_reset_declarations(command_context);
	command_context->submitted = false;
}

static void rtvk_command_context_destroy_pool(struct rtvk_context* ctx, struct rtvk_command_context* command_context) {
	if (command_context->vk_command_pool) {
		vkDestroyCommandPool(ctx->vk_device, command_context->vk_command_pool, VK_ALLOCATOR);
		command_context->vk_command_pool = VK_NULL_HANDLE;
	}
}

static bool rtvk_command_context_create_pool(struct rtvk_context* ctx, struct rtvk_command_context* command_context) {
	VkCommandPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	pool_info.pNext = NULL;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = command_context->queue->family_index;

	VkResult result = vkCreateCommandPool(ctx->vk_device, &pool_info, VK_ALLOCATOR, &command_context->vk_command_pool);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return false;
	}
	return true;
}

bool rtvk_command_context_child_recording(const struct rtvk_command_context* command_context) {
	for (const struct rtvk_command_buffer* child = command_context->children; child; child = child->next_child) {
		if (child->recording) {
			return true;
		}
	}
	return false;
}

static bool rtvk_command_context_begin_primary(struct rtvk_context* ctx, struct rtvk_command_context* command_context) {
	if (!command_context->primary) {
		command_context->primary = rtvk_command_buffer_create(ctx);
		if (!command_context->primary) {
			return false;
		}
		command_context->primary->command_context = command_context;
		command_context->primary->secondary = false;
	}
	rtvk_command_buffer_begin(ctx, command_context->primary);
	return rtvk_error() == RT_SUCCESS;
}

static bool rtvk_command_context_open_rendering(struct rtvk_context* ctx, struct rtvk_command_context* command_context) {
	assert(command_context);
	assert(command_context->primary);
	assert(command_context->color_view_count);

	struct rtvk_command_buffer* primary = command_context->primary;
	if (!primary->active || !primary->recording || command_context->color_view_count == 0) {
		rtvk_throwf(RT_IMPROPER_USAGE, "command context requires a recording primary and color attachment");
		return false;
	}

	VkRenderingAttachmentInfo color_attachments[RTVK_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS];
	for (u32 i = 0; i < command_context->color_view_count; i++) {
		struct rtvk_texture_view* color_view = command_context->color_views[i];
		if (!color_view || !color_view->image || !color_view->vk_image_view) {
			rtvk_throwf(RT_IMPROPER_USAGE, "framebuffer color attachment %u is invalid", i);
			return false;
		}
		rtvk_command_buffer_transition_texture(
			primary->active,
			color_view,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		);
		if (rtvk_error() != RT_SUCCESS) {
			return false;
		}
		color_attachments[i] = (VkRenderingAttachmentInfo){ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		color_attachments[i].pNext = NULL;
		color_attachments[i].imageView = color_view->vk_image_view;
		color_attachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachments[i].resolveMode = VK_RESOLVE_MODE_NONE;
		color_attachments[i].resolveImageView = VK_NULL_HANDLE;
		color_attachments[i].resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachments[i].loadOp = command_context->clear_colors[i] ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		color_attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachments[i].clearValue = command_context->color_clears[i];
	}

	struct rtvk_texture_view* depth_view = command_context->depth_view;
	struct rtvk_texture_view* stencil_view = rtvk_command_context_stencil_view(command_context);
	VkRenderingAttachmentInfo depth_attachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	if (depth_view) {
		if (!depth_view->image || !depth_view->vk_image_view) {
			rtvk_throwf(RT_IMPROPER_USAGE, "framebuffer depth attachment is invalid");
			return false;
		}
		rtvk_command_buffer_transition_texture(
			primary->active,
			depth_view,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
		);
		if (rtvk_error() != RT_SUCCESS) {
			return false;
		}
		depth_attachment.pNext = NULL;
		depth_attachment.imageView = depth_view->vk_image_view;
		depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depth_attachment.resolveMode = VK_RESOLVE_MODE_NONE;
		depth_attachment.resolveImageView = VK_NULL_HANDLE;
		depth_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.loadOp = command_context->clear_depth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depth_attachment.clearValue = command_context->depth_stencil_clear;
	}
	VkRenderingAttachmentInfo stencil_attachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	if (stencil_view) {
		if (!stencil_view->image || !stencil_view->vk_image_view) {
			rtvk_throwf(RT_IMPROPER_USAGE, "framebuffer stencil attachment is invalid");
			return false;
		}
		if (stencil_view != depth_view) {
			rtvk_command_buffer_transition_texture(
				primary->active,
				stencil_view,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
			);
			if (rtvk_error() != RT_SUCCESS) {
				return false;
			}
		}
		stencil_attachment.pNext = NULL;
		stencil_attachment.imageView = stencil_view->vk_image_view;
		stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		stencil_attachment.resolveMode = VK_RESOLVE_MODE_NONE;
		stencil_attachment.resolveImageView = VK_NULL_HANDLE;
		stencil_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		stencil_attachment.loadOp = command_context->clear_stencil ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		stencil_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		stencil_attachment.clearValue = command_context->depth_stencil_clear;
	}

	struct rtvk_texture_view* color_view = command_context->color_views[0];
	VkRenderingInfo rendering_info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
	rendering_info.pNext = NULL;
	rendering_info.flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;
	rendering_info.renderArea.offset.x = 0;
	rendering_info.renderArea.offset.y = 0;
	rendering_info.renderArea.extent.width = rtvk_view_width(color_view);
	rendering_info.renderArea.extent.height = rtvk_view_height(color_view);
	rendering_info.layerCount = 1;
	rendering_info.viewMask = 0;
	rendering_info.colorAttachmentCount = command_context->color_view_count;
	rendering_info.pColorAttachments = color_attachments;
	rendering_info.pDepthAttachment = depth_view ? &depth_attachment : NULL;
	rendering_info.pStencilAttachment = stencil_view ? &stencil_attachment : NULL;
	vkCmdBeginRendering(primary->active->vk_command_buffer, &rendering_info);

	command_context->rendering_open = true;
	return true;
}

static struct rtvk_texture_view* rtvk_command_context_stencil_view(struct rtvk_command_context* command_context) {
	if (command_context->stencil_view) {
		return command_context->stencil_view;
	}
	if (command_context->depth_view && (rtvk_texture_format_aspect(rtvk_view_format(command_context->depth_view)) & VK_IMAGE_ASPECT_STENCIL_BIT)) {
		return command_context->depth_view;
	}
	return NULL;
}
