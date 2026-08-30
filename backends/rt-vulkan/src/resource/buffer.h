#ifndef RTVK_BUFFER_H
#define RTVK_BUFFER_H

#include "config.h"
#include "resource.h"

#include <vk_mem_alloc.h>
#include <volk.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTVK_API rt_buffer rtBufferCreate(void);
RTVK_API void rtBufferDestroy(rt_buffer buffer);
RTVK_API void rtBufferResize(rt_buffer buffer, enum rt_memory_type memory_type, usize size);
RTVK_API void rtBufferRead(rt_buffer buffer, rt_buffer_range range, u08* data, usize data_size);
RTVK_API u08* rtBufferMap(rt_buffer buffer, rt_buffer_range range);
RTVK_API void rtBufferUnmap(rt_buffer buffer);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtvk_buffer {
	struct rtvk_resource_base base;

	struct rtvk_buffer* active;
	struct rtvk_buffer* next;
	VkBuffer vk_buffer;
	VmaAllocation vma_allocation;
	usize size;
	enum rt_memory_type memory_type;
};

struct rtvk_buffer_write {
	struct rtvk_buffer* source;
	struct rtvk_buffer* target;
};
RTVK_DECLARE_NEW_RESOURCE(buffer)

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

void rtvk_buffer_resize(struct rtvk_context* ctx, struct rtvk_buffer* buffer, enum rt_memory_type memory_type, usize size);
void rtvk_buffer_read(struct rtvk_context* ctx, struct rtvk_buffer* buffer, rt_buffer_range range, u08* data, usize data_size);
u08* rtvk_buffer_map(struct rtvk_context* ctx, struct rtvk_buffer* buffer, rt_buffer_range range);
void rtvk_buffer_unmap(struct rtvk_context* ctx, struct rtvk_buffer* buffer);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtvk_buffer* rtvk_buffer_node_create(struct rtvk_context* ctx, enum rt_memory_type memory_type, usize size);
struct rtvk_buffer* rtvk_buffer_active_node(struct rtvk_buffer* buffer);
void rtvk_buffer_recycle_node(struct rtvk_buffer* buffer, struct rtvk_buffer* node);
struct rtvk_buffer* rtvk_buffer_take_reusable_node(struct rtvk_buffer* buffer, enum rt_memory_type memory_type, usize size);
struct rtvk_buffer_write rtvk_buffer_write_begin(struct rtvk_context* ctx, struct rtvk_buffer* buffer);
void rtvk_buffer_write_commit(struct rtvk_buffer* buffer, struct rtvk_buffer_write* write);
void rtvk_buffer_write_cancel(struct rtvk_buffer* buffer, struct rtvk_buffer_write* write);

#endif
