#ifndef RTVK_TEXTURE_H
#define RTVK_TEXTURE_H

#include "config.h"
#include "resource.h"

#include <vk_mem_alloc.h>
#include <volk.h>

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtvk_buffer;

RTVK_API rt_texture rtTextureCreate(void);
RTVK_API void rtTextureDestroy(rt_texture texture);
RTVK_API void rtTextureResize(rt_texture texture, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count);
RTVK_API rt_texture_view rtTextureViewCreate(void);
RTVK_API void rtTextureViewDestroy(rt_texture_view texture_view);
RTVK_API rt_extent_3d rtTextureViewExtent(rt_texture_view texture_view);
RTVK_API void rtTextureViewSetTexture(rt_texture_view texture_view, rt_texture texture);
RTVK_API void rtTextureViewSetFilter(rt_texture_view texture_view, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter);
RTVK_API void rtTextureViewSetAddress(rt_texture_view texture_view, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w);
RTVK_API void rtTextureViewSetAnisotropy(rt_texture_view texture_view, usize max_anisotropy);
RTVK_API void rtTextureViewSetLod(rt_texture_view texture_view, f32 min_lod, f32 max_lod, f32 lod_bias);
RTVK_API void rtTextureViewRead(rt_texture_view texture_view, rt_texture_range range, u08* data, usize data_size);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

struct rtvk_image_base {
	struct rtvk_resource_base base;
	VkImage vk_image;
	VkFormat vk_format;
	VkImageLayout vk_layout;
	enum rt_texture_type type;
	u32 width;
	u32 height;
	u32 depth;
	u32 mip_levels;
	bool presentable;
};

struct rtvk_texture {
	struct rtvk_image_base base;
	struct rtvk_texture* active;
	struct rtvk_texture* next;

	VmaAllocation vma_allocation;
};
RTVK_DECLARE_NEW_RESOURCE(texture)

struct rtvk_texture_write {
	struct rtvk_texture* source;
	struct rtvk_texture* target;
};

struct rtvk_texture_view {
	struct rtvk_resource_base base;
	struct rtvk_image_base* image;
	VkImageView vk_image_view;
	VkSampler vk_sampler;

	enum rt_filter mag_filter;
	enum rt_filter min_filter;
	enum rt_mip_filter mip_filter;
	enum rt_address_mode address_u;
	enum rt_address_mode address_v;
	enum rt_address_mode address_w;
	usize max_anisotropy;
	f32 min_lod;
	f32 max_lod;
	f32 lod_bias;
};
RTVK_DECLARE_NEW_RESOURCE(texture_view)

struct rtvk_texture_view* rtvk_texture_view_create_for_texture(struct rtvk_context* ctx, struct rtvk_texture* texture);
void rtvk_texture_view_bind(struct rtvk_context* ctx, struct rtvk_texture_view* view, struct rtvk_texture* texture);
void rtvk_texture_view_bind_image(struct rtvk_context* ctx, struct rtvk_texture_view* view, struct rtvk_image_base* image);
void rtvk_texture_resize(struct rtvk_context* ctx, struct rtvk_texture* texture, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count);
struct rtvk_texture* rtvk_texture_active_node(struct rtvk_texture* texture);
struct rtvk_texture* rtvk_texture_node_create(struct rtvk_context* ctx);
struct rtvk_texture* rtvk_texture_node_clone(struct rtvk_context* ctx, const struct rtvk_texture* source);
struct rtvk_texture_write rtvk_texture_write_begin(struct rtvk_context* ctx, struct rtvk_texture* texture);
void rtvk_texture_write_commit(struct rtvk_texture* texture, struct rtvk_texture_write* write);
void rtvk_texture_write_cancel(struct rtvk_texture* texture, struct rtvk_texture_write* write);

u32 rtvk_view_width(const struct rtvk_texture_view* view);
u32 rtvk_view_height(const struct rtvk_texture_view* view);
VkFormat rtvk_view_format(const struct rtvk_texture_view* view);
VkImageLayout rtvk_view_layout(const struct rtvk_texture_view* view);
void rtvk_image_transition_layout(VkCommandBuffer command_buffer, struct rtvk_image_base* image, VkImageLayout layout);

void rtvk_texture_view_filter(struct rtvk_texture_view* texture_view, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter);
void rtvk_texture_view_address(struct rtvk_texture_view* texture_view, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w);
void rtvk_texture_view_anisotropy(struct rtvk_texture_view* texture_view, usize max_anisotropy);
void rtvk_texture_view_lod(struct rtvk_texture_view* texture_view, f32 min_lod, f32 max_lod, f32 lod_bias);
VkImageAspectFlags rtvk_texture_format_aspect(VkFormat format);
void rtvk_texture_recycle_node(struct rtvk_texture* texture, struct rtvk_texture* node);
void rtvk_texture_collect_nodes(struct rtvk_texture* texture);

#endif
