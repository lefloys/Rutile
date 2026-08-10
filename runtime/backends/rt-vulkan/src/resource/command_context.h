#ifndef RTVK_COMMAND_CONTEXT_H
#define RTVK_COMMAND_CONTEXT_H

#include "command_buffer.h"
#include "framebuffer.h"
#include "queue.h"
#include "sync.h"

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTVK_API rt_command_context rtCommandContextCreate(void);
RTVK_API void rtCommandContextDestroy(rt_command_context command_context);
RTVK_API void rtCommandContextBind(rt_command_context command_context, rt_queue queue);
RTVK_API rt_command_buffer rtCommandContextAllocate(rt_command_context command_context);
RTVK_API void rtCommandContextBindFramebuffer(rt_command_context command_context, rt_framebuffer framebuffer);
RTVK_API void rtCommandContextClearColor(rt_command_context command_context, u32 color_index, f32 r, f32 g, f32 b, f32 a);
RTVK_API void rtCommandContextClearDepth(rt_command_context command_context, f32 depth);
RTVK_API void rtCommandContextClearStencil(rt_command_context command_context, u32 stencil);
RTVK_API void rtCommandContextSetViewport(rt_command_context command_context, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth);
RTVK_API void rtCommandContextSetScissor(rt_command_context command_context, u32 x, u32 y, u32 width, u32 height);
RTVK_API void rtCommandContextExecute(rt_command_context command_context, rt_command_buffer command_buffer);
RTVK_API void rtCommandContextEndRendering(rt_command_context command_context);
RTVK_API rt_timepoint rtCommandContextSubmit(rt_command_context command_context);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtvk_command_context {
	struct rtvk_resource_base base;
	struct rtvk_command_buffer* primary;
	struct rtvk_command_buffer* children;
	struct rtvk_queue* queue;
	struct rtvk_framebuffer* framebuffer;
	struct rtvk_texture_view* color_views[RTVK_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS];
	struct rtvk_texture_view* depth_view;
	struct rtvk_texture_view* stencil_view;

	VkCommandPool vk_command_pool;
	struct rt_mutex* command_pool_lock;
	VkClearValue color_clears[RTVK_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS];
	VkClearValue depth_stencil_clear;

	u32 viewport_x;
	u32 viewport_y;
	u32 viewport_width;
	u32 viewport_height;
	u32 scissor_x;
	u32 scissor_y;
	u32 scissor_width;
	u32 scissor_height;
	f32 min_depth;
	f32 max_depth;
	u32 color_view_count;
	u32 render_width;
	u32 render_height;

	bool clear_colors[RTVK_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS];
	bool clear_depth;
	bool clear_stencil;
	bool draw_packet_begun;
	bool rendering;
	bool rendering_open;
	bool submitted;
	bool destroyed;

	struct rtvk_timepoint submitted_timepoint;
};

RTVK_DECLARE_NEW_RESOURCE(command_context)

static inline void rtvk_command_context_lock(struct rtvk_command_context* command_context) {
	if (command_context && command_context->command_pool_lock) {
		rt_mutex_lock(command_context->command_pool_lock);
	}
}

static inline void rtvk_command_context_unlock(struct rtvk_command_context* command_context) {
	if (command_context && command_context->command_pool_lock) {
		rt_mutex_unlock(command_context->command_pool_lock);
	}
}

void rtvk_command_context_bind(struct rtvk_context* ctx, struct rtvk_command_context* command_context, struct rtvk_queue* queue);
struct rtvk_command_buffer* rtvk_command_context_allocate(struct rtvk_context* ctx, struct rtvk_command_context* command_context);
void rtvk_command_context_bind_framebuffer(struct rtvk_context* ctx, struct rtvk_command_context* command_context, struct rtvk_framebuffer* framebuffer);
void rtvk_command_context_clear_color(struct rtvk_command_context* command_context, u32 color_index, f32 r, f32 g, f32 b, f32 a);
void rtvk_command_context_clear_depth(struct rtvk_command_context* command_context, f32 depth);
void rtvk_command_context_clear_stencil(struct rtvk_command_context* command_context, u32 stencil);
void rtvk_command_context_set_viewport(struct rtvk_command_context* command_context, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth);
void rtvk_command_context_set_scissor(struct rtvk_command_context* command_context, u32 x, u32 y, u32 width, u32 height);
void rtvk_command_context_execute(struct rtvk_context* ctx, struct rtvk_command_context* command_context, struct rtvk_command_buffer* command_buffer);
void rtvk_command_context_end_rendering(struct rtvk_command_context* command_context);
struct rtvk_timepoint rtvk_command_context_submit(struct rtvk_context* ctx, struct rtvk_command_context* command_context);
bool rtvk_command_context_child_recording(const struct rtvk_command_context* command_context);

#endif
