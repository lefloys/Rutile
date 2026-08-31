#include "buffer.h"
#include "context.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>

rt_buffer rtBufferCreate(void) {
	rtvk_begin_errorable_operation();
	return rtvk_buffer_to_handle(rtvk_buffer_create(rtvk_get_current_context()));
}

void rtBufferDestroy(rt_buffer buffer) {
	rtvk_buffer_destroy(rtvk_get_current_context(), rtvk_buffer_from_handle(buffer));
}

void rtBufferResize(rt_buffer buffer, enum rt_memory_type memory_type, usize size) {
	rtvk_begin_errorable_operation();
	rtvk_buffer_resize(rtvk_get_current_context(), rtvk_buffer_from_handle(buffer), memory_type, size);
}

void rtBufferRead(rt_buffer buffer, rt_buffer_range range, u08* data, usize data_size) {
	rtvk_begin_errorable_operation();
	rtvk_buffer_read(rtvk_get_current_context(), rtvk_buffer_from_handle(buffer), range, data, data_size);
}

u08* rtBufferMap(rt_buffer buffer, rt_buffer_range range) {
	rtvk_begin_errorable_operation();
	return rtvk_buffer_map(rtvk_get_current_context(), rtvk_buffer_from_handle(buffer), range);
}

void rtBufferUnmap(rt_buffer buffer) {
	rtvk_begin_errorable_operation();
	rtvk_buffer_unmap(rtvk_get_current_context(), rtvk_buffer_from_handle(buffer));
}

RTVK_DEFINE_RESOURCE_PRIVATE(buffer)

void rtvk_buffer_init(struct rtvk_context* ctx, struct rtvk_buffer* buffer) {
	rtvk_init_resource_base(ctx, RTVK_RESOURCE_BASE(buffer), buffer, rtvk_buffer_finalize_resource);
}

void rtvk_buffer_finish(struct rtvk_buffer* buffer) {
	rtvk_release_resource(buffer->active);
	while (buffer->next) {
		struct rtvk_buffer* node = buffer->next;
		buffer->next = node->next;
		node->next = NULL;
		rtvk_release_resource(node);
	}
	if (buffer->vk_buffer) {
		vmaDestroyBuffer(buffer->base.ctx->vma_allocator, buffer->vk_buffer, buffer->vma_allocation);
	}
}

struct rtvk_buffer* rtvk_buffer_node_create(struct rtvk_context* ctx, enum rt_memory_type memory_type, usize size) {
	struct rtvk_buffer* node = calloc(1, sizeof(*node));
	if (!node) {
		rtvk_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate %zu bytes for buffer metadata", sizeof(*node));
		return NULL;
	}
	VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	info.size = size ? size : 1;
	info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VmaAllocationCreateInfo allocation = { 0 };
	if (memory_type == RT_HOST_MEMORY) {
		allocation.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		allocation.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	} else {
		allocation.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	}
	VkResult result = vmaCreateBuffer(ctx->vma_allocator, &info, &allocation, &node->vk_buffer, &node->vma_allocation, NULL);
	if (result != VK_SUCCESS) {
		free(node);
		rtvk_throwf(rtvk_error_from_vk(result), "Vulkan call returned %s", rtvk_vk_result_name(result));
		return NULL;
	}
	rtvk_init_resource_base(ctx, RTVK_RESOURCE_BASE(node), node, rtvk_buffer_finalize_resource);
	rtvk_atomic_bool_store(&node->base.zombie, true);
	node->size = size;
	node->memory_type = memory_type;
	return node;
}

void rtvk_buffer_recycle_node(struct rtvk_buffer* buffer, struct rtvk_buffer* node) {
	node->next = buffer->next;
	buffer->next = node;
}

struct rtvk_buffer* rtvk_buffer_take_reusable_node(struct rtvk_buffer* buffer, enum rt_memory_type memory_type, usize size) {
	for (struct rtvk_buffer** link = &buffer->next; *link; link = &(*link)->next) {
		struct rtvk_buffer* node = *link;
		if (node->memory_type == memory_type && node->size == size && rtvk_atomic_load(&node->base.ref_count) == 1 && rtvk_atomic_load(&node->base.job_count) == 0) {
			*link = node->next;
			node->next = NULL;
			return node;
		}
	}
	return NULL;
}

void rtvk_buffer_resize(struct rtvk_context* ctx, struct rtvk_buffer* buffer, enum rt_memory_type memory_type, usize size) {
	if (!buffer) {
		return;
	}
	struct rtvk_buffer* node = rtvk_buffer_take_reusable_node(buffer, memory_type, size);
	if (!node) {
		node = rtvk_buffer_node_create(ctx, memory_type, size);
	}
	if (!node) {
		return;
	}
	if (buffer->active) {
		rtvk_buffer_recycle_node(buffer, buffer->active);
	}
	buffer->active = node;
}

struct rtvk_buffer* rtvk_buffer_active_node(struct rtvk_buffer* buffer) {
	if (!buffer) {
		return NULL;
	}
	return buffer->active;
}

struct rtvk_buffer_write rtvk_buffer_write_begin(struct rtvk_context* ctx, struct rtvk_buffer* buffer) {
	struct rtvk_buffer_write write = { 0 };
	if (!buffer) {
		return write;
	}

	write.target = buffer->active;
	if (!write.target || (rtvk_atomic_load(&write.target->base.ref_count) == 1 && rtvk_atomic_load(&write.target->base.job_count) == 0)) {
		return write;
	}

	write.source = write.target;
	write.target = rtvk_buffer_take_reusable_node(buffer, write.source->memory_type, write.source->size);
	if (!write.target) {
		write.target = rtvk_buffer_node_create(ctx, write.source->memory_type, write.source->size);
	}
	if (!write.target) {
		write.source = NULL;
	}
	return write;
}

void rtvk_buffer_write_commit(struct rtvk_buffer* buffer, struct rtvk_buffer_write* write) {
	if (!buffer || !write || !write->source || !write->target) {
		return;
	}

	rtvk_buffer_recycle_node(buffer, write->source);
	buffer->active = write->target;
	write->source = NULL;
	write->target = NULL;
}

void rtvk_buffer_write_cancel(struct rtvk_buffer* buffer, struct rtvk_buffer_write* write) {
	if (!buffer || !write || !write->source || !write->target) {
		return;
	}

	rtvk_buffer_recycle_node(buffer, write->target);
	write->source = NULL;
	write->target = NULL;
}

void rtvk_buffer_read(struct rtvk_context* ctx, struct rtvk_buffer* buffer, rt_buffer_range range, u08* data, usize data_size) {
	if (!buffer || !buffer->active || !data || data_size < range.size || range.offset > buffer->active->size || range.size > buffer->active->size - range.offset) {
		return;
	}
	if (buffer->active->memory_type != RT_HOST_MEMORY) {
		return;
	}
	VmaAllocationInfo allocation;
	vmaGetAllocationInfo(ctx->vma_allocator, buffer->active->vma_allocation, &allocation);
	vmaInvalidateAllocation(ctx->vma_allocator, buffer->active->vma_allocation, range.offset, range.size);
	memcpy(data, (u08*)allocation.pMappedData + range.offset, range.size);
}

u08* rtvk_buffer_map(struct rtvk_context* ctx, struct rtvk_buffer* buffer, rt_buffer_range range) {
	(void)ctx;
	if (!buffer || !buffer->active || buffer->active->memory_type != RT_HOST_MEMORY || range.offset > buffer->active->size || range.size > buffer->active->size - range.offset) {
		return NULL;
	}
	VmaAllocationInfo allocation;
	vmaGetAllocationInfo(buffer->base.ctx->vma_allocator, buffer->active->vma_allocation, &allocation);
	return (u08*)allocation.pMappedData + range.offset;
}

void rtvk_buffer_unmap(struct rtvk_context* ctx, struct rtvk_buffer* buffer) {
	(void)ctx;
	if (buffer && buffer->active && buffer->active->memory_type == RT_HOST_MEMORY) {
		vmaFlushAllocation(buffer->base.ctx->vma_allocator, buffer->active->vma_allocation, 0, buffer->active->size);
	}
}
