#include "command_buffer.h"
#include "context.h"
#include "error.h"
#include "command_context.h"
#include "queue.h"
#include "texture.h"
#include <assert.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtvk_command_context* rtvk_command_buffer_lock_context(struct rtvk_command_buffer* command_buffer) {
	if (!command_buffer || !command_buffer->command_context_lock) {
		rtvk_throwf(RT_IMPROPER_USAGE, "command buffer requires a live context-owned buffer");
		return NULL;
	}
	rt_mutex_lock(command_buffer->command_context_lock);
	struct rtvk_command_context* command_context = command_buffer->command_context;
	if (!command_context) {
		rtvk_throwf(RT_IMPROPER_USAGE, "command buffer requires a live context-owned buffer");
		rt_mutex_unlock(command_buffer->command_context_lock);
		return NULL;
	}
	rtvk_retain_resource(command_context);
	rt_mutex_unlock(command_buffer->command_context_lock);
	rtvk_command_context_lock(command_context);
	if (command_context->destroyed) {
		rtvk_throwf(RT_IMPROPER_USAGE, "command buffer context has been destroyed");
		rtvk_command_buffer_unlock_context(command_context);
		return NULL;
	}
	return command_context;
}

void rtvk_command_buffer_unlock_context(struct rtvk_command_context* command_context) {
	rtvk_command_context_unlock(command_context);
	rtvk_release_resource(command_context);
}

void rtvk_command_buffer_detach_context(struct rtvk_command_buffer* command_buffer) {
	if (!command_buffer || !command_buffer->command_context_lock) {
		return;
	}
	rt_mutex_lock(command_buffer->command_context_lock);
	command_buffer->command_context = NULL;
	command_buffer->next_child = NULL;
	rt_mutex_unlock(command_buffer->command_context_lock);
}

void rtCommandBufferDestroy(rt_command_buffer command_buffer) {
	struct rtvk_command_buffer* child = rtvk_command_buffer_from_handle(command_buffer);
	if (!child) {
		return;
	}
	struct rtvk_command_context* command_context = rtvk_command_buffer_lock_context(child);
	if (command_context) {
		struct rtvk_command_buffer** link = &command_context->children;
		while (*link && *link != child) {
			link = &(*link)->next_child;
		}
		if (*link) {
			*link = child->next_child;
		}
		rtvk_command_buffer_discard(rtvk_get_current_context(), child);
		rtvk_command_buffer_detach_context(child);
		rtvk_command_buffer_unlock_context(command_context);
		rtvk_command_buffer_destroy(rtvk_get_current_context(), child);
		rtvk_release_resource(child);
		return;
	}
	rtvk_command_buffer_detach_context(child);
	rtvk_command_buffer_destroy(rtvk_get_current_context(), child);
}
void rtCmdReset(rt_command_buffer command_buffer) {
	struct rtvk_command_buffer* child = rtvk_command_buffer_from_handle(command_buffer);
	if (!child || !child->command_context) {
		rtvk_throwf(RT_IMPROPER_USAGE, "command buffer reset requires a live context-owned buffer");
		return;
	}
	struct rtvk_command_context* command_context = rtvk_command_buffer_lock_context(child);
	if (!command_context) {
		return;
	}
	if (child->recording || (child->executed && !command_context->submitted)) {
		rtvk_throwf(RT_IMPROPER_USAGE, "cannot reset a recording or executed-unsubmitted command buffer");
		rtvk_command_buffer_unlock_context(command_context);
		return;
	}
	rtvk_command_buffer_reset(rtvk_get_current_context(), child);
	rtvk_command_buffer_unlock_context(command_context);
}
void rtCmdBegin(rt_command_buffer command_buffer) {
	struct rtvk_command_buffer* child = rtvk_command_buffer_from_handle(command_buffer);
	struct rtvk_command_context* command_context = rtvk_command_buffer_lock_context(child);
	if (!command_context) {
		return;
	}
	if (!command_context->queue || !command_context->framebuffer || !command_context->rendering ||
		command_context->submitted || child->recording || child->executable) {
		rtvk_throwf(RT_IMPROPER_USAGE, "command buffer begin requires an initial child in an active framebuffer scope");
		rtvk_command_buffer_unlock_context(command_context);
		return;
	}
	rtvk_command_buffer_begin(rtvk_get_current_context(), child);
	rtvk_command_buffer_unlock_context(command_context);
}
void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program) {
	struct rtvk_command_buffer* child = rtvk_command_buffer_from_handle(command_buffer);
	struct rtvk_command_context* command_context = rtvk_command_buffer_lock_context(child);
	if (!command_context) {
		return;
	}
	rtvk_command_buffer_use_graphics_program(
		rtvk_get_current_context(),
		child,
		rtvk_graphics_program_from_handle(program)
	);
	rtvk_command_buffer_unlock_context(command_context);
}

void rtCmdUniformBuffer(rt_command_buffer command_buffer, rt_uniform_location location, rt_buffer buffer, u64 offset, u64 size) {
	struct rtvk_command_buffer* child = rtvk_command_buffer_from_handle(command_buffer);
	struct rtvk_command_context* command_context = rtvk_command_buffer_lock_context(child);
	if (!command_context) {
		return;
	}
	rtvk_command_buffer_uniform_buffer(
		rtvk_get_current_context(),
		child,
		rtvk_uniform_location_from_handle(location),
		rtvk_buffer_from_handle(buffer),
		offset,
		size
	);
	rtvk_command_buffer_unlock_context(command_context);
}

void rtCmdUniformTexture(rt_command_buffer command_buffer, rt_uniform_location location, rt_texture_view texture_view) {
	struct rtvk_command_buffer* child = rtvk_command_buffer_from_handle(command_buffer);
	struct rtvk_command_context* command_context = rtvk_command_buffer_lock_context(child);
	if (!command_context) {
		return;
	}
	rtvk_command_buffer_uniform_texture(
		rtvk_get_current_context(),
		child,
		rtvk_uniform_location_from_handle(location),
		rtvk_texture_view_from_handle(texture_view)
	);
	rtvk_command_buffer_unlock_context(command_context);
}

void rtCmdStorageBuffer(rt_command_buffer command_buffer, rt_uniform_location location, rt_buffer buffer, u64 offset, u64 size) {
	struct rtvk_command_buffer* child = rtvk_command_buffer_from_handle(command_buffer);
	struct rtvk_command_context* command_context = rtvk_command_buffer_lock_context(child);
	if (!command_context) {
		return;
	}
	rtvk_command_buffer_storage_buffer(rtvk_get_current_context(), child, rtvk_uniform_location_from_handle(location), rtvk_buffer_from_handle(buffer), offset, size);
	rtvk_command_buffer_unlock_context(command_context);
}

void rtCmdBindVertexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, u64 offset) {
	struct rtvk_command_buffer* child = rtvk_command_buffer_from_handle(command_buffer);
	struct rtvk_command_context* command_context = rtvk_command_buffer_lock_context(child);
	if (!command_context) {
		return;
	}
	rtvk_command_buffer_bind_vertex_buffer(
		rtvk_get_current_context(),
		child,
		rtvk_buffer_from_handle(buffer),
		offset
	);
	rtvk_command_buffer_unlock_context(command_context);
}

void rtCmdDraw(rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex) {
	struct rtvk_command_buffer* child = rtvk_command_buffer_from_handle(command_buffer);
	struct rtvk_command_context* command_context = rtvk_command_buffer_lock_context(child);
	if (!command_context) {
		return;
	}
	rtvk_command_buffer_draw(
		rtvk_get_current_context(),
		child,
		vertex_count,
		first_vertex
	);
	rtvk_command_context_unlock(command_context);
}

void rtCmdEnd(rt_command_buffer command_buffer) {
	struct rtvk_command_buffer* child = rtvk_command_buffer_from_handle(command_buffer);
	struct rtvk_command_context* command_context = rtvk_command_buffer_lock_context(child);
	if (!command_context) {
		return;
	}
	if (!child->recording) {
		rtvk_throwf(RT_IMPROPER_USAGE, "command buffer end requires a recording context-owned child");
		rtvk_command_buffer_unlock_context(command_context);
		return;
	}
	rtvk_command_buffer_end(rtvk_get_current_context(), child);
	if (!child->recording) {
		child->executable = rtvk_error() == RT_SUCCESS;
	}
	rtvk_command_buffer_unlock_context(command_context);
}

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTVK_DEFINE_RESOURCE_PRIVATE(command_buffer)

void rtvk_command_buffer_init(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer) {
	assert(ctx);
	rtvk_init_resource_base(ctx, RTVK_RESOURCE_BASE(command_buffer), RT_RESOURCE_COMMAND_BUFFER);
	command_buffer->command_context_lock = rt_mutex_create();
	if (!command_buffer->command_context_lock) {
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate command buffer context lock");
	}
	command_buffer->family_index = (u32)-1;
}
void rtvk_command_buffer_finish(struct rtvk_command_buffer* command_buffer) {
	struct rtvk_context* ctx = command_buffer->base.ctx;
	assert(ctx);
	struct rtvk_command_buffer* node = command_buffer->next;
	if (command_buffer->active) {
		rtvk_command_buffer_wait_pending(ctx, command_buffer->active);
		rtvk_release_resource(command_buffer->active);
	}
	command_buffer->active = NULL;
	while (node) {
		struct rtvk_command_buffer* next = node->next;
		node->next = NULL;
		rtvk_command_buffer_wait_pending(ctx, node);
		rtvk_release_resource(node);
		node = next;
	}
	command_buffer->next = NULL;
	if (command_buffer->vk_command_pool || command_buffer->vk_command_buffer) {
		rtvk_command_buffer_release_recorded_resources(command_buffer);
		rtvk_command_buffer_destroy_descriptor_pools(ctx, command_buffer);
		rtvk_command_buffer_destroy_vk_handles(ctx, command_buffer);
	}
	rt_mutex_destroy(command_buffer->command_context_lock);
	command_buffer->command_context_lock = NULL;
	rtvk_finish_resource_base(RTVK_RESOURCE_BASE(command_buffer));
}

void rtvk_command_buffer_release_recorded_resources(struct rtvk_command_buffer* command_buffer) {
	assert(command_buffer);
	if (command_buffer->vertex_buffer_node) {
		rtvk_release_resource(command_buffer->vertex_buffer_node);
	}
	command_buffer->vertex_buffer_node = NULL;
	if (command_buffer->vk_command_buffer && command_buffer->graphics_program) {
		rtvk_release_resource(command_buffer->graphics_program);
	}
	command_buffer->graphics_program = NULL;
	for (u32 i = 0; i < command_buffer->uniform_slot_count; i++) {
		rtvk_command_buffer_clear_uniform_slot(&command_buffer->uniform_slots[i]);
	}
	free(command_buffer->uniform_slots);
	command_buffer->uniform_slots = NULL;
	command_buffer->uniform_slot_count = 0;
	rtvk_command_buffer_reset_descriptor_pools(command_buffer->base.ctx, command_buffer);
	command_buffer->bound_descriptor_set = VK_NULL_HANDLE;
	command_buffer->uniforms_dirty = true;
}

void rtvk_command_buffer_clear_uniform_slot(rtvk_uniform_slot* slot) {
	assert(slot);
	if (slot->kind == RTVK_UNIFORM_SLOT_BUFFER || slot->kind == RTVK_UNIFORM_SLOT_STORAGE_BUFFER) {
		rtvk_release_resource(slot->buffer.node);
	}
	if (slot->kind == RTVK_UNIFORM_SLOT_TEXTURE) {
		if (slot->texture.view) {
			rtvk_release_resource(slot->texture.view);
		}
	}
	*slot = (rtvk_uniform_slot){ 0 };
}

rtvk_uniform_slot* rtvk_command_buffer_uniform_slot(struct rtvk_command_buffer* command_buffer, u32 index) {
	if (index < command_buffer->uniform_slot_count) {
		return &command_buffer->uniform_slots[index];
	}

	u32 count = command_buffer->uniform_slot_count ? command_buffer->uniform_slot_count : 8;
	while (count <= index) {
		count *= 2;
	}
	void* slots = realloc(command_buffer->uniform_slots, sizeof(command_buffer->uniform_slots[0]) * count);
	if (!slots) {
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate command buffer uniform slots");
		return NULL;
	}

	command_buffer->uniform_slots = slots;
	memset(&command_buffer->uniform_slots[command_buffer->uniform_slot_count], 0, sizeof(command_buffer->uniform_slots[0]) * (count - command_buffer->uniform_slot_count));
	command_buffer->uniform_slot_count = count;
	return &command_buffer->uniform_slots[index];
}

void rtvk_command_buffer_reset_descriptor_pools(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer) {
	assert(ctx);
	assert(command_buffer);
	command_buffer->current_descriptor_pool = command_buffer->descriptor_pools;
	for (rtvk_descriptor_pool_node* pool = command_buffer->descriptor_pools; pool; pool = pool->next) {
		if (!pool->vk_pool) {
			continue;
		}
		VkResult result = vkResetDescriptorPool(ctx->vk_device, pool->vk_pool, 0);
		if (result != VK_SUCCESS) {
			rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
			continue;
		}
		pool->allocated_sets = 0;
	}
}

void rtvk_command_buffer_destroy_descriptor_pools(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer) {
	assert(ctx);
	assert(command_buffer);
	rtvk_descriptor_pool_node* pool = command_buffer->descriptor_pools;
	while (pool) {
		rtvk_descriptor_pool_node* next = pool->next;
		if (pool->vk_pool) {
			vkDestroyDescriptorPool(ctx->vk_device, pool->vk_pool, VK_ALLOCATOR);
		}
		free(pool);
		pool = next;
	}
	command_buffer->descriptor_pools = NULL;
	command_buffer->current_descriptor_pool = NULL;
}

void rtvk_command_buffer_destroy_vk_handles(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer) {
	assert(ctx);
	assert(command_buffer);
	if (command_buffer->vk_command_buffer) {
		vkFreeCommandBuffers(ctx->vk_device, command_buffer->vk_command_pool, 1, &command_buffer->vk_command_buffer);
	}
	command_buffer->vk_command_buffer = VK_NULL_HANDLE;
	if (command_buffer->owns_command_pool && command_buffer->vk_command_pool) {
		vkDestroyCommandPool(ctx->vk_device, command_buffer->vk_command_pool, VK_ALLOCATOR);
	}
	command_buffer->vk_command_pool = VK_NULL_HANDLE;
	command_buffer->bound_descriptor_set = VK_NULL_HANDLE;
	command_buffer->uniforms_dirty = true;
	command_buffer->family_index = (u32)-1;
}
void rtvk_command_buffer_wait_pending(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer) {
	assert(ctx);
	assert(command_buffer);
	if (!command_buffer->pending_timepoint.queue || command_buffer->pending_timepoint.value == 0) {
		return;
	}

	struct rtvk_timepoint timepoint = command_buffer->pending_timepoint;
	rtvk_timepoint_wait(ctx, timepoint);
	command_buffer->pending_timepoint.queue = NULL;
	command_buffer->pending_timepoint.value = 0;
}
struct rtvk_command_buffer* rtvk_command_buffer_node_create(struct rtvk_context* ctx, u32 family_index, bool secondary, VkCommandPool shared_pool) {
	struct rtvk_command_buffer* node = calloc(1, sizeof(*node));
	if (!node) {
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate command buffer node");
		return NULL;
	}

	VkResult result;
	if (shared_pool) {
		node->vk_command_pool = shared_pool;
		node->owns_command_pool = false;
	} else {
		VkCommandPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		pool_info.pNext = NULL;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = family_index;
		result = vkCreateCommandPool(ctx->vk_device, &pool_info, VK_ALLOCATOR, &node->vk_command_pool);
		if (result != VK_SUCCESS) {
			free(node);
			rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
			return NULL;
		}
		node->owns_command_pool = true;
	}

	VkCommandBufferAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocate_info.pNext = NULL;
	allocate_info.commandPool = node->vk_command_pool;
	allocate_info.level = secondary ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = 1;

	result = vkAllocateCommandBuffers(ctx->vk_device, &allocate_info, &node->vk_command_buffer);
	if (result != VK_SUCCESS) {
		if (node->owns_command_pool) {
			vkDestroyCommandPool(ctx->vk_device, node->vk_command_pool, VK_ALLOCATOR);
		}
		free(node);
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return NULL;
	}

	rtvk_init_resource_base(ctx, RTVK_RESOURCE_BASE(node), RT_RESOURCE_COMMAND_BUFFER);
	rtvk_atomic_bool_store(&node->base.zombie, true);
	node->family_index = family_index;
	node->secondary = secondary;
	return node;
}

static void rtvk_command_buffer_recycle_node(struct rtvk_command_buffer* command_buffer, struct rtvk_command_buffer* node) {
	if (!node) {
		return;
	}
	node->next = command_buffer->next;
	command_buffer->next = node;
}

static struct rtvk_command_buffer* rtvk_command_buffer_take_reusable_node(struct rtvk_command_buffer* command_buffer, u32 family_index) {
	struct rtvk_command_buffer** link = &command_buffer->next;
	while (*link) {
		struct rtvk_command_buffer* node = *link;
		if (node->family_index == family_index && rtvk_atomic_load(&node->base.ref_count) == 1) {
			*link = node->next;
			node->next = NULL;
			return node;
		}
		link = &node->next;
	}
	return NULL;
}
void rtvk_command_buffer_prepare(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer) {
	rtvk_command_buffer_recycle_node(command_buffer, command_buffer->active);
	command_buffer->active = NULL;

	struct rtvk_command_context* command_context = command_buffer->command_context;
	struct rtvk_command_buffer* node = rtvk_command_buffer_take_reusable_node(command_buffer, command_context->queue->family_index);
	if (!node) {
		rtvk_queue_collect_to_value(ctx, command_context->queue, rtvk_queue_completed_value(ctx, command_context->queue));
		node = rtvk_command_buffer_take_reusable_node(command_buffer, command_context->queue->family_index);
	}
	if (!node) {
		node = rtvk_command_buffer_node_create(ctx, command_context->queue->family_index, command_buffer->secondary, command_context->vk_command_pool);
	}
	if (!node) {
		return;
	}

	rtvk_command_buffer_release_recorded_resources(node);
	command_buffer->active = node;
}

void rtvk_command_buffer_discard(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer) {
	if (!command_buffer) {
		return;
	}
	rtvk_command_buffer_wait_pending(ctx, command_buffer);
	if (command_buffer->active) {
		rtvk_command_buffer_wait_pending(ctx, command_buffer->active);
		if (command_buffer->recording) {
			vkResetCommandBuffer(command_buffer->active->vk_command_buffer, 0);
		}
		rtvk_release_resource(command_buffer->active);
	}
	command_buffer->active = NULL;
	while (command_buffer->next) {
		struct rtvk_command_buffer* node = command_buffer->next;
		command_buffer->next = node->next;
		node->next = NULL;
		rtvk_command_buffer_wait_pending(ctx, node);
		rtvk_release_resource(node);
	}
	rtvk_command_buffer_release_recorded_resources(command_buffer);
	command_buffer->recording = false;
	command_buffer->executable = false;
	command_buffer->executed = false;
}

void rtvk_command_buffer_begin(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer) {
	struct rtvk_command_context* command_context = command_buffer ? command_buffer->command_context : NULL;
	bool reserved_declaration_boundary = false;
	if (command_buffer && command_buffer->secondary) {
		if (!command_context || !command_context->rendering || !command_context->framebuffer) {
			rtvk_throwf(RT_IMPROPER_USAGE, "secondary command buffer requires a context framebuffer");
			return;
		}
		if (!rtvk_atomic_bool_load(&command_context->draw_packet_begun)) {
			rtvk_atomic_bool_store(&command_context->draw_packet_begun, true);
			reserved_declaration_boundary = true;
		}
	}
	rtvk_command_buffer_prepare(ctx, command_buffer);
	if (rtvk_error() != RT_SUCCESS) {
		if (reserved_declaration_boundary) {
			rtvk_atomic_bool_store(&command_context->draw_packet_begun, false);
		}
		return;
	}

	struct rtvk_command_buffer* node = command_buffer->active;
	vkResetCommandBuffer(node->vk_command_buffer, 0);

	VkCommandBufferInheritanceRenderingInfo rendering_inheritance = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO };
	VkCommandBufferInheritanceInfo inheritance = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
	VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	begin_info.pNext = NULL;
	begin_info.flags = command_buffer->secondary ? VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begin_info.pInheritanceInfo = NULL;
	if (command_buffer->secondary) {
		struct rtvk_command_context* command_context = command_buffer->command_context;
		if (!command_context || command_context->color_view_count == 0) {
			rtvk_throwf(RT_IMPROPER_USAGE, "secondary command buffer requires a context framebuffer");
			if (reserved_declaration_boundary) {
				rtvk_atomic_bool_store(&command_context->draw_packet_begun, false);
			}
			return;
		}
		VkFormat color_formats[RTVK_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS];
		for (u32 i = 0; i < command_context->color_view_count; i++) {
			struct rtvk_texture_view* color = command_context->color_views[i];
			if (!color || !color->image) {
				rtvk_throwf(RT_IMPROPER_USAGE, "secondary command buffer requires complete context color attachments");
				if (reserved_declaration_boundary) {
					rtvk_atomic_bool_store(&command_context->draw_packet_begun, false);
				}
				return;
			}
			color_formats[i] = color->image->vk_format;
		}
		rendering_inheritance.viewMask = 0;
		rendering_inheritance.colorAttachmentCount = command_context->color_view_count;
		rendering_inheritance.pColorAttachmentFormats = color_formats;
		rendering_inheritance.depthAttachmentFormat = command_context->depth_view ? command_context->depth_view->image->vk_format : VK_FORMAT_UNDEFINED;
		rendering_inheritance.stencilAttachmentFormat = command_context->stencil_view ? command_context->stencil_view->image->vk_format :
			(command_context->depth_view && (rtvk_texture_format_aspect(rtvk_view_format(command_context->depth_view)) & VK_IMAGE_ASPECT_STENCIL_BIT) ? command_context->depth_view->image->vk_format : VK_FORMAT_UNDEFINED);
		rendering_inheritance.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		inheritance.pNext = &rendering_inheritance;
		begin_info.pInheritanceInfo = &inheritance;
	}

	VkResult result = vkBeginCommandBuffer(node->vk_command_buffer, &begin_info);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		if (reserved_declaration_boundary) {
			rtvk_atomic_bool_store(&command_context->draw_packet_begun, false);
		}
		return;
	}

	command_buffer->recording = true;
	command_buffer->graphics_program = NULL;
	command_buffer->vertex_buffer = NULL;
	if (command_buffer->secondary) {
		struct rtvk_command_context* command_context = command_buffer->command_context;
		VkViewport viewport = { 0 };
		viewport.x = (f32)command_context->viewport_x;
		viewport.y = (f32)(command_context->viewport_y + command_context->viewport_height);
		viewport.width = (f32)command_context->viewport_width;
		viewport.height = -(f32)command_context->viewport_height;
		viewport.minDepth = command_context->min_depth;
		viewport.maxDepth = command_context->max_depth;
		vkCmdSetViewport(node->vk_command_buffer, 0, 1, &viewport);

		VkRect2D scissor = { 0 };
		scissor.offset.x = (int32_t)command_context->scissor_x;
		scissor.offset.y = (int32_t)command_context->scissor_y;
		scissor.extent.width = command_context->scissor_width;
		scissor.extent.height = command_context->scissor_height;
		vkCmdSetScissor(node->vk_command_buffer, 0, 1, &scissor);
	}
	node->bound_descriptor_set = VK_NULL_HANDLE;
	node->uniforms_dirty = true;
}
void rtvk_command_buffer_reset(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer) {
	if (!command_buffer) {
		return;
	}
	if (command_buffer->recording) {
		rtvk_throwf(RT_IMPROPER_USAGE, "cannot reset a recording command buffer");
		return;
	}
	if (command_buffer->active) {
		rtvk_command_buffer_wait_pending(ctx, command_buffer->active);
		rtvk_command_buffer_release_recorded_resources(command_buffer->active);
		vkResetCommandBuffer(command_buffer->active->vk_command_buffer, 0);
	}
	command_buffer->graphics_program = NULL;
	command_buffer->vertex_buffer = NULL;
	command_buffer->recording = false;
	command_buffer->executable = false;
	command_buffer->executed = false;
}
void rtvk_command_buffer_end(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer) {
	struct rtvk_command_buffer* node = command_buffer ? command_buffer->active : NULL;
	if (!node || !command_buffer->recording) {
		rtvk_throwf(RT_IMPROPER_USAGE, "end command buffer requires a recording command buffer");
		return;
	}
	VkResult result = vkEndCommandBuffer(node->vk_command_buffer);
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return;
	}
	command_buffer->recording = false;
}

static VkAccessFlags rtvk_command_buffer_layout_access(VkImageLayout layout) {
	switch (layout) {
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: /**********/
		return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: /**/
		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: /**************/
		return VK_ACCESS_TRANSFER_READ_BIT;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: /**************/
		return VK_ACCESS_TRANSFER_WRITE_BIT;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: /**********/
		return VK_ACCESS_SHADER_READ_BIT;
	case VK_IMAGE_LAYOUT_GENERAL: /***************************/
		return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	default:
		return 0;
	}
}

static VkPipelineStageFlags rtvk_command_buffer_layout_stage(VkImageLayout layout) {
	switch (layout) {
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: /**********/
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: /**/
		return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: /**************/
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: /**************/
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: /***********/
		return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	case VK_IMAGE_LAYOUT_GENERAL: /***************************/
		return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	default: /************************************************/
		return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
}

void rtvk_command_buffer_transition_texture(struct rtvk_command_buffer* command_buffer, struct rtvk_texture_view* view, VkImageLayout layout, VkAccessFlags dst_access, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage) {
	if (!command_buffer || !command_buffer->vk_command_buffer) {
		rtvk_throwf(RT_IMPROPER_USAGE, "texture transition requires a valid command buffer");
		return;
	}
	if (!view || !view->image) {
		rtvk_throwf(RT_IMPROPER_USAGE, "texture transition requires a valid texture view");
		return;
	}

	struct rtvk_image_base* image = view->image;
	VkImageLayout* vk_layout = &image->vk_layout;
	if (!image->vk_image) {
		rtvk_throwf(RT_IMPROPER_USAGE, "texture transition requires an allocated Vulkan image");
		return;
	}

	if (*vk_layout == layout) {
		return;
	}

	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.pNext = NULL;
	VkImageLayout old_layout = *vk_layout;
	barrier.srcAccessMask = rtvk_command_buffer_layout_access(old_layout);
	VkPipelineStageFlags actual_src_stage = rtvk_command_buffer_layout_stage(old_layout);
	if (actual_src_stage == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
		barrier.srcAccessMask = 0;
		actual_src_stage = src_stage;
	}
	barrier.dstAccessMask = dst_access;
	barrier.oldLayout = old_layout;
	barrier.newLayout = layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image->vk_image;
	barrier.subresourceRange.aspectMask = rtvk_texture_format_aspect(image->vk_format);
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = image->mip_levels ? image->mip_levels : 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(command_buffer->vk_command_buffer, actual_src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
	*vk_layout = layout;
}

void rtvk_command_buffer_use_graphics_program(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer, struct rtvk_graphics_program* program) {
	struct rtvk_command_buffer* node = command_buffer ? command_buffer->active : NULL;
	struct rtvk_command_context* command_context = command_buffer ? command_buffer->command_context : NULL;
	struct rtvk_texture_view* color_view = command_context && command_context->color_view_count ? command_context->color_views[0] : NULL;

	if (!node || !command_buffer->recording || !command_buffer->command_context->rendering || !program || !color_view) {
		rtvk_throwf(RT_IMPROPER_USAGE, "graphics program requires a recording draw packet and context color attachment");
		return;
	}

	VkFormat color_formats[RTVK_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS];
	for (u32 i = 0; i < command_context->color_view_count; i++) {
		if (!command_context->color_views[i]) {
			rtvk_throwf(RT_IMPROPER_USAGE, "graphics program requires complete context color attachments");
			return;
		}
		color_formats[i] = rtvk_view_format(command_context->color_views[i]);
	}
	VkPipeline pipeline = rtvk_graphics_program_prepare(
		ctx,
		program,
		color_formats,
		command_context->color_view_count,
		rtvk_view_format(command_context->depth_view),
		command_context->stencil_view ? rtvk_view_format(command_context->stencil_view) :
			(command_context->depth_view && (rtvk_texture_format_aspect(rtvk_view_format(command_context->depth_view)) & VK_IMAGE_ASPECT_STENCIL_BIT) ? rtvk_view_format(command_context->depth_view) : VK_FORMAT_UNDEFINED),
		VK_SAMPLE_COUNT_1_BIT
	);
	if (!pipeline) {
		return;
	}

	vkCmdBindPipeline(node->vk_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	if (command_buffer->graphics_program != program) {
		node->bound_descriptor_set = VK_NULL_HANDLE;
		node->uniforms_dirty = true;
	}
	if (node->graphics_program != program) {
		rtvk_retain_resource(program);
		rtvk_release_resource(node->graphics_program);
		node->graphics_program = program;
	}
	command_buffer->graphics_program = program;
}

void rtvk_command_buffer_uniform_buffer(
	struct rtvk_context* ctx,
	struct rtvk_command_buffer* command_buffer,
	struct rtvk_uniform_location* location,
	struct rtvk_buffer* buffer,
	u64 offset,
	u64 size
) {
	struct rtvk_command_buffer* node = command_buffer ? command_buffer->active : NULL;
	struct rtvk_buffer* buffer_node = buffer ? buffer->active : NULL;

	if (!node || !command_buffer->recording || !command_buffer->command_context || !command_buffer->command_context->rendering) {
		rtvk_throwf(RT_IMPROPER_USAGE, "setting a uniform buffer requires active rendering");
		return;
	}
	if (!location) {
		rtvk_throwf(RT_IMPROPER_USAGE, "uniform location is NULL");
		return;
	}
	if (location->kind != RTVK_UNIFORM_LOCATION_BUFFER) {
		rtvk_throwf(RT_IMPROPER_USAGE, "uniform location does not accept a buffer");
		return;
	}
	if (!command_buffer->graphics_program || command_buffer->graphics_program != location->program) {
		rtvk_throwf(RT_IMPROPER_USAGE, "uniform location does not belong to the active graphics program");
		return;
	}
	if (!buffer_node || !buffer_node->vk_buffer) {
		rtvk_throwf(RT_IMPROPER_USAGE, "uniform buffer has no storage");
		return;
	}
	if (!(buffer_node->usage & RT_BUFFER_USAGE_UNIFORM) || (buffer_node->usage & RT_BUFFER_USAGE_STAGING)) {
		rtvk_throwf(RT_IMPROPER_USAGE, "buffer usage is not compatible with uniform binding");
		return;
	}
	if (size == 0) {
		rtvk_throwf(RT_IMPROPER_USAGE, "uniform buffer binding size is zero");
		return;
	}
	if (offset > buffer_node->size || size > buffer_node->size - offset) {
		rtvk_throwf(RT_IMPROPER_USAGE, "uniform buffer range is out of bounds");
		return;
	}

	rtvk_uniform_slot* slot = rtvk_command_buffer_uniform_slot(node, location->index);
	if (!slot) {
		return;
	}
	if (slot->kind == RTVK_UNIFORM_SLOT_BUFFER &&
		slot->buffer.node == buffer_node &&
		slot->buffer.offset == offset &&
		slot->buffer.size == size) {
		return;
	}
	rtvk_command_buffer_clear_uniform_slot(slot);
	rtvk_retain_resource(buffer_node);
	slot->kind = RTVK_UNIFORM_SLOT_BUFFER;
	slot->buffer.node = buffer_node;
	slot->buffer.offset = offset;
	slot->buffer.size = size;
	node->bound_descriptor_set = VK_NULL_HANDLE;
	node->uniforms_dirty = true;
}

void rtvk_command_buffer_storage_buffer(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer, struct rtvk_uniform_location* location, struct rtvk_buffer* buffer, u64 offset, u64 size) {
	(void)ctx;
	struct rtvk_command_buffer* node = command_buffer ? command_buffer->active : NULL;
	struct rtvk_buffer* buffer_node = buffer ? buffer->active : NULL;
	if (!node || !command_buffer->recording || !command_buffer->command_context || !command_buffer->command_context->rendering) {
		rtvk_throwf(RT_IMPROPER_USAGE, "setting a storage buffer requires active rendering");
		return;
	}
	if (!command_buffer->graphics_program) {
		rtvk_throwf(RT_IMPROPER_USAGE, "setting a storage buffer requires an active graphics program");
		return;
	}
	if (!location || location->kind != RTVK_UNIFORM_LOCATION_STORAGE_BUFFER || location->program != command_buffer->graphics_program) {
		rtvk_throwf(RT_IMPROPER_USAGE, "storage buffer location does not belong to the active graphics program");
		return;
	}
	if (!buffer_node || !buffer_node->vk_buffer) {
		rtvk_throwf(RT_IMPROPER_USAGE, "storage buffer has no storage");
		return;
	}
	if (!(buffer_node->usage & RT_BUFFER_USAGE_STORAGE) || (buffer_node->usage & RT_BUFFER_USAGE_STAGING)) {
		rtvk_throwf(RT_IMPROPER_USAGE, "buffer usage is not compatible with storage binding");
		return;
	}
	if (size == 0 || offset > buffer_node->size || size > buffer_node->size - offset) {
		rtvk_throwf(RT_IMPROPER_USAGE, "storage buffer range is invalid");
		return;
	}
	rtvk_uniform_slot* slot = rtvk_command_buffer_uniform_slot(node, location->index);
	if (!slot) {
		return;
	}
	if (slot->kind == RTVK_UNIFORM_SLOT_STORAGE_BUFFER && slot->buffer.node == buffer_node && slot->buffer.offset == offset && slot->buffer.size == size) {
		return;
	}
	rtvk_command_buffer_clear_uniform_slot(slot);
	rtvk_retain_resource(buffer_node);
	slot->kind = RTVK_UNIFORM_SLOT_STORAGE_BUFFER;
	slot->buffer.node = buffer_node;
	slot->buffer.offset = offset;
	slot->buffer.size = size;
	node->bound_descriptor_set = VK_NULL_HANDLE;
	node->uniforms_dirty = true;
}

void rtvk_command_buffer_uniform_texture(
	struct rtvk_context* ctx,
	struct rtvk_command_buffer* command_buffer,
	struct rtvk_uniform_location* location,
	struct rtvk_texture_view* texture_view
) {
	struct rtvk_command_buffer* node = command_buffer ? command_buffer->active : NULL;
	if (!node || !command_buffer->recording || !command_buffer->command_context || !command_buffer->command_context->rendering) {
		rtvk_throwf(RT_IMPROPER_USAGE, "setting a uniform texture requires active rendering");
		return;
	}
	if (!location) {
		rtvk_throwf(RT_IMPROPER_USAGE, "uniform location is NULL");
		return;
	}
	if (location->kind != RTVK_UNIFORM_LOCATION_TEXTURE) {
		rtvk_throwf(RT_IMPROPER_USAGE, "uniform location does not accept a texture");
		return;
	}
	if (!command_buffer->graphics_program || command_buffer->graphics_program != location->program) {
		rtvk_throwf(RT_IMPROPER_USAGE, "uniform location does not belong to the active graphics program");
		return;
	}
	if (!texture_view || !texture_view->image || !texture_view->vk_image_view) {
		rtvk_throwf(RT_IMPROPER_USAGE, "uniform texture view is NULL");
		return;
	}
	if (rtvk_view_layout(texture_view) != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		rtvk_throwf(RT_IMPROPER_USAGE, "uniform texture must be shader-readable before draw-packet recording");
		return;
	}
	rtvk_uniform_slot* slot = rtvk_command_buffer_uniform_slot(node, location->index);
	if (!slot) {
		return;
	}
	if (slot->kind == RTVK_UNIFORM_SLOT_TEXTURE && slot->texture.view == texture_view) {
		return;
	}
	rtvk_command_buffer_clear_uniform_slot(slot);
	rtvk_retain_resource(texture_view);
	slot->kind = RTVK_UNIFORM_SLOT_TEXTURE;
	slot->texture.view = texture_view;
	node->bound_descriptor_set = VK_NULL_HANDLE;
	node->uniforms_dirty = true;
}

void rtvk_command_buffer_bind_vertex_buffer(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer, struct rtvk_buffer* buffer, u64 offset) {
	struct rtvk_command_buffer* command_buffer_node = command_buffer ? command_buffer->active : NULL;
	struct rtvk_buffer* node = buffer ? buffer->active : NULL;

	if (!command_buffer_node || !command_buffer->recording || !command_buffer->command_context || !command_buffer->command_context->rendering || !node || !node->vk_buffer) {
		rtvk_throwf(RT_IMPROPER_USAGE, "vertex buffer binding requires a recording draw packet and allocated buffer");
		return;
	}
	if (!(node->usage & RT_BUFFER_USAGE_VERTEX) || (node->usage & RT_BUFFER_USAGE_STAGING)) {
		rtvk_throwf(RT_IMPROPER_USAGE, "buffer usage is not compatible with vertex binding");
		return;
	}
	if (offset > node->size) {
		rtvk_throwf(RT_IMPROPER_USAGE, "vertex buffer offset is out of range");
		return;
	}

	VkDeviceSize vk_offset = (VkDeviceSize)offset;
	vkCmdBindVertexBuffers(command_buffer_node->vk_command_buffer, 0, 1, &node->vk_buffer, &vk_offset);
	rtvk_retain_resource(node);
	if (command_buffer_node->vertex_buffer_node) {
		rtvk_release_resource(command_buffer_node->vertex_buffer_node);
	}
	command_buffer_node->vertex_buffer_node = node;
	command_buffer->vertex_buffer = buffer;
}

static rtvk_descriptor_pool_node* rtvk_command_buffer_create_descriptor_pool(
	struct rtvk_context* ctx,
	struct rtvk_command_buffer* command_buffer,
	u32 min_sets,
	u32 descriptors_per_type
) {
	u32 max_sets = RTVK_INITIAL_DESCRIPTOR_SETS_PER_POOL;
	if (command_buffer->current_descriptor_pool && command_buffer->current_descriptor_pool->max_sets >= max_sets) {
		max_sets = command_buffer->current_descriptor_pool->max_sets * 2;
	}
	if (max_sets < min_sets) {
		max_sets = min_sets;
	}
	if (descriptors_per_type == 0) {
		descriptors_per_type = 1;
	}

	VkDescriptorPoolSize pool_sizes[4] = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, max_sets * descriptors_per_type },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, max_sets * descriptors_per_type },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, max_sets * descriptors_per_type },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, max_sets * descriptors_per_type },
	};

	VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	pool_info.pNext = NULL;
	pool_info.flags = 0;
	pool_info.maxSets = max_sets;
	pool_info.poolSizeCount = 4;
	pool_info.pPoolSizes = pool_sizes;

	rtvk_descriptor_pool_node* pool = calloc(1, sizeof(*pool));
	if (!pool) {
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate descriptor pool metadata");
		return NULL;
	}

	VkResult result = vkCreateDescriptorPool(ctx->vk_device, &pool_info, VK_ALLOCATOR, &pool->vk_pool);
	if (result != VK_SUCCESS) {
		free(pool);
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return NULL;
	}

	pool->max_sets = max_sets;
	pool->descriptors_per_type = descriptors_per_type;
	pool->next = command_buffer->descriptor_pools;
	command_buffer->descriptor_pools = pool;
	command_buffer->current_descriptor_pool = pool;
	return pool;
}

static void rtvk_command_buffer_allocate_descriptor_set(
	struct rtvk_context* ctx,
	struct rtvk_command_buffer* command_buffer,
	struct rtvk_graphics_program* program,
	VkDescriptorSet* descriptor_set
) {
	u32 descriptors_per_type = program->uniform_location_count ? program->uniform_location_count : 1;
	rtvk_descriptor_pool_node* pool = command_buffer->current_descriptor_pool;
	if (!pool || pool->allocated_sets >= pool->max_sets ||
		pool->descriptors_per_type < descriptors_per_type) {
		pool = rtvk_command_buffer_create_descriptor_pool(ctx, command_buffer, 1, descriptors_per_type);
		if (!pool) {
			return;
		}
	}

	VkDescriptorSetAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocate_info.pNext = NULL;
	allocate_info.descriptorPool = pool->vk_pool;
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts = &program->vk_descriptor_set_layout;

	VkResult result = vkAllocateDescriptorSets(ctx->vk_device, &allocate_info, descriptor_set);
	if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
		pool = rtvk_command_buffer_create_descriptor_pool(ctx, command_buffer, 1, descriptors_per_type);
		if (!pool) {
			return;
		}
		allocate_info.descriptorPool = pool->vk_pool;
		result = vkAllocateDescriptorSets(ctx->vk_device, &allocate_info, descriptor_set);
	}
	if (result != VK_SUCCESS) {
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return;
	}

	pool->allocated_sets++;
}

static void rtvk_command_buffer_bind_uniform_buffers(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer) {
	struct rtvk_command_buffer* node = command_buffer->active;
	struct rtvk_graphics_program* program = command_buffer->graphics_program;
	if (!program || program->uniform_location_count == 0) {
		return;
	}
	if (!program->vk_descriptor_set_layout || !program->vk_pipeline_layout) {
		rtvk_throwf(RT_IMPROPER_USAGE, "graphics program uniform layout is not ready");
		return;
	}
	if (!node->uniforms_dirty && node->bound_descriptor_set) {
		return;
	}
	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	rtvk_command_buffer_allocate_descriptor_set(ctx, node, program, &descriptor_set);
	if (rtvk_error() != RT_SUCCESS) {
		return;
	}

	u32 location_count = program->uniform_location_count;
	VkDescriptorBufferInfo stack_buffer_infos[16];
	VkDescriptorImageInfo stack_image_infos[16];
	VkWriteDescriptorSet stack_writes[16];
	VkDescriptorBufferInfo* buffer_infos = location_count <= 16 ? stack_buffer_infos : calloc(location_count, sizeof(*buffer_infos));
	VkDescriptorImageInfo* image_infos = location_count <= 16 ? stack_image_infos : calloc(location_count, sizeof(*image_infos));
	VkWriteDescriptorSet* writes = location_count <= 16 ? stack_writes : calloc(location_count, sizeof(*writes));
	if (location_count <= 16) {
		memset(buffer_infos, 0, sizeof(stack_buffer_infos));
		memset(image_infos, 0, sizeof(stack_image_infos));
		memset(writes, 0, sizeof(stack_writes));
	}
	if (!buffer_infos || !image_infos || !writes) {
		if (buffer_infos != stack_buffer_infos) {
			free(buffer_infos);
		}
		if (image_infos != stack_image_infos) {
			free(image_infos);
		}
		if (writes != stack_writes) {
			free(writes);
		}
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate descriptor writes");
		return;
	}

	for (u32 i = 0; i < program->uniform_location_count; i++) {
		struct rtvk_uniform_location* location = &program->uniform_locations[i];
		writes[i] = (VkWriteDescriptorSet){ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		writes[i].pNext = NULL;
		writes[i].dstSet = descriptor_set;
		writes[i].dstBinding = location->binding;
		writes[i].dstArrayElement = 0;
		writes[i].descriptorCount = 1;
		writes[i].pTexelBufferView = NULL;

		if (location->kind == RTVK_UNIFORM_LOCATION_BUFFER || location->kind == RTVK_UNIFORM_LOCATION_STORAGE_BUFFER) {
			rtvk_uniform_slot* slot = location->index < node->uniform_slot_count ? &node->uniform_slots[location->index] : NULL;
			const bool wants_storage = location->kind == RTVK_UNIFORM_LOCATION_STORAGE_BUFFER;
			if (!slot || slot->kind != (wants_storage ? RTVK_UNIFORM_SLOT_STORAGE_BUFFER : RTVK_UNIFORM_SLOT_BUFFER) || !slot->buffer.node) {
				rtvk_throwf(RT_IMPROPER_USAGE, "%s %s is not bound", wants_storage ? "storage buffer" : "uniform buffer", location->name);
				if (buffer_infos != stack_buffer_infos) {
					free(buffer_infos);
				}
				if (image_infos != stack_image_infos) {
					free(image_infos);
				}
				if (writes != stack_writes) {
					free(writes);
				}
				return;
			}

			buffer_infos[i].buffer = slot->buffer.node->vk_buffer;
			buffer_infos[i].offset = slot->buffer.offset;
			buffer_infos[i].range = slot->buffer.size;
			writes[i].descriptorType = wants_storage ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writes[i].pImageInfo = NULL;
			writes[i].pBufferInfo = &buffer_infos[i];
		} else {
			rtvk_uniform_slot* slot = location->index < node->uniform_slot_count ? &node->uniform_slots[location->index] : NULL;
			if (!slot || slot->kind != RTVK_UNIFORM_SLOT_TEXTURE || !slot->texture.view) {
				rtvk_throwf(RT_IMPROPER_USAGE, "uniform texture %s is not bound", location->name);
				if (buffer_infos != stack_buffer_infos) {
					free(buffer_infos);
				}
				if (image_infos != stack_image_infos) {
					free(image_infos);
				}
				if (writes != stack_writes) {
					free(writes);
				}
				return;
			}

			image_infos[i].sampler = slot->texture.view->vk_sampler;
			image_infos[i].imageView = slot->texture.view->vk_image_view;
			image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[i].pImageInfo = &image_infos[i];
			writes[i].pBufferInfo = NULL;
		}
	}

	vkUpdateDescriptorSets(ctx->vk_device, program->uniform_location_count, writes, 0, NULL);
	vkCmdBindDescriptorSets(node->vk_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, program->vk_pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
	node->bound_descriptor_set = descriptor_set;
	node->uniforms_dirty = false;
	if (buffer_infos != stack_buffer_infos) {
		free(buffer_infos);
	}
	if (image_infos != stack_image_infos) {
		free(image_infos);
	}
	if (writes != stack_writes) {
		free(writes);
	}
}

void rtvk_command_buffer_draw(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer, u32 vertex_count, u32 first_vertex) {
	if (!command_buffer || !command_buffer->active || !command_buffer->recording || !command_buffer->command_context ||
		!command_buffer->command_context->rendering || !command_buffer->graphics_program) {
		rtvk_throwf(RT_IMPROPER_USAGE, "draw requires a recording draw packet with a graphics program");
		return;
	}
	rtvk_command_buffer_bind_uniform_buffers(ctx, command_buffer);
	if (rtvk_error() != RT_SUCCESS) {
		return;
	}
	vkCmdDraw(command_buffer->active->vk_command_buffer, vertex_count, 1, first_vertex, 0);
}
