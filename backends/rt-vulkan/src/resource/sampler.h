#ifndef RTVK_SAMPLER_H
#define RTVK_SAMPLER_H

#include "config.h"
#include "resource.h"

#include <volk.h>

RTVK_API rt_sampler rtSamplerCreate(void);
RTVK_API void rtSamplerDestroy(rt_sampler sampler);
RTVK_API void rtSamplerSetFilter(rt_sampler sampler, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter);
RTVK_API void rtSamplerSetAddress(rt_sampler sampler, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w);
RTVK_API void rtSamplerSetAnisotropy(rt_sampler sampler, usize max_anisotropy);
RTVK_API void rtSamplerSetLod(rt_sampler sampler, f32 min_lod, f32 max_lod, f32 lod_bias);

struct rtvk_sampler {
	struct rtvk_resource_base base;
	struct rtvk_sampler* active;
	struct rtvk_sampler* next;
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
RTVK_DECLARE_NEW_RESOURCE(sampler)

struct rtvk_sampler* rtvk_sampler_active_node(struct rtvk_sampler* sampler);
struct rtvk_sampler* rtvk_sampler_node_create(struct rtvk_context* ctx, const struct rtvk_sampler* settings);
bool rtvk_sampler_rebuild(struct rtvk_sampler* sampler);
void rtvk_sampler_collect_nodes(struct rtvk_sampler* sampler);
VkSamplerAddressMode rtvk_sampler_address_mode(enum rt_address_mode mode);

#endif
