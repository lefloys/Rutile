#ifndef RTVK_COMMAND_BUFFER_H
#define RTVK_COMMAND_BUFFER_H

#include "buffer.h"
#include "config.h"
#include "graphics_program.h"
#include "resource.h"
#include "sync.h"
#include "texture.h"

#include <volk.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTVK_API void rtCommandBufferDestroy(rt_command_buffer command_buffer);
RTVK_API void rtCmdReset(rt_command_buffer command_buffer);
RTVK_API void rtCmdBegin(rt_command_buffer command_buffer);

RTVK_API void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program);
RTVK_API void rtCmdUniformBuffer(rt_command_buffer command_buffer, rt_uniform_location location, rt_buffer buffer, u64 offset, u64 size);
RTVK_API void rtCmdUniformTexture(rt_command_buffer command_buffer, rt_uniform_location location, rt_texture_view texture_view);
RTVK_API void rtCmdStorageBuffer(rt_command_buffer command_buffer, rt_uniform_location location, rt_buffer buffer, u64 offset, u64 size);
RTVK_API void rtCmdBindVertexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, u64 offset);
RTVK_API void rtCmdDraw(rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex);
RTVK_API void rtCmdEnd(rt_command_buffer command_buffer);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

typedef enum rtvk_uniform_slot_kind {
	RTVK_UNIFORM_SLOT_EMPTY,
	RTVK_UNIFORM_SLOT_BUFFER,
	RTVK_UNIFORM_SLOT_TEXTURE,
	RTVK_UNIFORM_SLOT_STORAGE_BUFFER,
} rtvk_uniform_slot_kind;

typedef struct rtvk_uniform_slot {
	rtvk_uniform_slot_kind kind;
	union {
		struct {
			struct rtvk_buffer* node;
			u64 offset;
			u64 size;
		} buffer;
		struct {
			struct rtvk_texture_view* view;
		} texture;
	};
} rtvk_uniform_slot;

typedef struct rtvk_descriptor_pool_node {
	struct rtvk_descriptor_pool_node* next;
	VkDescriptorPool vk_pool;
	u32 max_sets;
	u32 allocated_sets;
	u32 descriptors_per_type;
} rtvk_descriptor_pool_node;

struct rtvk_command_context;

struct rtvk_command_buffer {
	struct rtvk_resource_base base;
	struct rtvk_command_buffer* active;
	struct rtvk_command_buffer* next;
	struct rtvk_command_buffer* next_child;

	VkCommandPool vk_command_pool;
	VkCommandBuffer vk_command_buffer;
	VkDescriptorSet bound_descriptor_set;
	rtvk_descriptor_pool_node* descriptor_pools;
	rtvk_descriptor_pool_node* current_descriptor_pool;

	struct rtvk_command_context* command_context;
	struct rt_mutex* command_context_lock;
	struct rtvk_graphics_program* graphics_program;
	struct rtvk_buffer* vertex_buffer;
	struct rtvk_buffer* vertex_buffer_node;
	rtvk_uniform_slot* uniform_slots;

	struct rtvk_timepoint pending_timepoint;

	u32 uniform_slot_count;
	u32 family_index;
	bool recording;
	bool executable;
	bool executed;
	bool owns_command_pool;
	bool secondary;
	bool uniforms_dirty;
};
RTVK_DECLARE_NEW_RESOURCE(command_buffer)

struct rtvk_command_buffer* rtvk_command_buffer_node_create(struct rtvk_context* ctx, u32 family_index, bool secondary, VkCommandPool shared_pool);
void rtvk_command_buffer_destroy_vk_handles(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer);

void rtvk_command_buffer_begin(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer);
void rtvk_command_buffer_reset(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer);
void rtvk_command_buffer_end(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer);
void rtvk_command_buffer_discard(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer);

void rtvk_command_buffer_bind_vertex_buffer(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer, struct rtvk_buffer* buffer, u64 offset);
void rtvk_command_buffer_storage_buffer(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer, struct rtvk_uniform_location* location, struct rtvk_buffer* buffer, u64 offset, u64 size);
void rtvk_command_buffer_uniform_buffer(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer, struct rtvk_uniform_location* location, struct rtvk_buffer* buffer, u64 offset, u64 size);
void rtvk_command_buffer_uniform_texture(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer, struct rtvk_uniform_location* location, struct rtvk_texture_view* texture_view);
void rtvk_command_buffer_use_graphics_program(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer, struct rtvk_graphics_program* program);
void rtvk_command_buffer_draw(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer, u32 vertex_count, u32 first_vertex);
void rtvk_command_buffer_transition_texture(struct rtvk_command_buffer* command_buffer, struct rtvk_texture_view* view, VkImageLayout layout, VkAccessFlags dst_access, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);
void rtvk_command_buffer_wait_pending(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer);
struct rtvk_command_context* rtvk_command_buffer_lock_context(struct rtvk_command_buffer* command_buffer);
void rtvk_command_buffer_unlock_context(struct rtvk_command_context* command_context);
void rtvk_command_buffer_detach_context(struct rtvk_command_buffer* command_buffer);

void rtvk_command_buffer_clear_uniform_slot(rtvk_uniform_slot* slot);
void rtvk_command_buffer_destroy_descriptor_pools(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer);
void rtvk_command_buffer_prepare(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer);
void rtvk_command_buffer_release_recorded_resources(struct rtvk_command_buffer* command_buffer);
void rtvk_command_buffer_reset_descriptor_pools(struct rtvk_context* ctx, struct rtvk_command_buffer* command_buffer);
rtvk_uniform_slot* rtvk_command_buffer_uniform_slot(struct rtvk_command_buffer* command_buffer, u32 index);

#endif
