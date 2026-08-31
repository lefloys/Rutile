#include "sampler.h"

#include "context.h"
#include "error.h"

rt_sampler rtSamplerCreate(void) {
	rtvk_begin_errorable_operation();
	struct rtvk_context* ctx = rtvk_get_current_context();
	struct rtvk_sampler* sampler = rtvk_sampler_create(ctx);
	if (sampler && !sampler->active) {
		rtvk_sampler_destroy(ctx, sampler);
		sampler = NULL;
	}
	return rtvk_sampler_to_handle(sampler);
}

void rtSamplerDestroy(rt_sampler sampler) {
	rtvk_sampler_destroy(rtvk_get_current_context(), rtvk_sampler_from_handle(sampler));
}

void rtSamplerSetFilter(rt_sampler handle, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter) {
	rtvk_begin_errorable_operation();
	struct rtvk_sampler* sampler = rtvk_sampler_from_handle(handle);
	if (!sampler || (mag_filter != RT_FILTER_NEAREST && mag_filter != RT_FILTER_LINEAR) || (min_filter != RT_FILTER_NEAREST && min_filter != RT_FILTER_LINEAR) || mip_filter < RT_MIP_FILTER_NONE || mip_filter > RT_MIP_FILTER_LINEAR) {
		rtvk_throwf(RT_IMPROPER_USAGE, "rtSamplerSetFilter received an invalid sampler or filter");
		return;
	}
	const enum rt_filter old_mag = sampler->mag_filter;
	const enum rt_filter old_min = sampler->min_filter;
	const enum rt_mip_filter old_mip = sampler->mip_filter;
	sampler->mag_filter = mag_filter;
	sampler->min_filter = min_filter;
	sampler->mip_filter = mip_filter;
	if (!rtvk_sampler_rebuild(sampler)) {
		sampler->mag_filter = old_mag;
		sampler->min_filter = old_min;
		sampler->mip_filter = old_mip;
	}
}

void rtSamplerSetAddress(rt_sampler handle, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w) {
	rtvk_begin_errorable_operation();
	struct rtvk_sampler* sampler = rtvk_sampler_from_handle(handle);
	if (!sampler || address_u < RT_ADDRESS_CLAMP || address_u > RT_ADDRESS_MIRROR || address_v < RT_ADDRESS_CLAMP || address_v > RT_ADDRESS_MIRROR || address_w < RT_ADDRESS_CLAMP || address_w > RT_ADDRESS_MIRROR) {
		rtvk_throwf(RT_IMPROPER_USAGE, "rtSamplerSetAddress received an invalid sampler or address mode");
		return;
	}
	const enum rt_address_mode old_u = sampler->address_u;
	const enum rt_address_mode old_v = sampler->address_v;
	const enum rt_address_mode old_w = sampler->address_w;
	sampler->address_u = address_u;
	sampler->address_v = address_v;
	sampler->address_w = address_w;
	if (!rtvk_sampler_rebuild(sampler)) {
		sampler->address_u = old_u;
		sampler->address_v = old_v;
		sampler->address_w = old_w;
	}
}

void rtSamplerSetAnisotropy(rt_sampler handle, usize max_anisotropy) {
	rtvk_begin_errorable_operation();
	struct rtvk_sampler* sampler = rtvk_sampler_from_handle(handle);
	if (!sampler || !max_anisotropy) {
		rtvk_throwf(RT_IMPROPER_USAGE, "rtSamplerSetAnisotropy requires a sampler and a nonzero maximum");
		return;
	}
	const usize old = sampler->max_anisotropy;
	sampler->max_anisotropy = max_anisotropy;
	if (!rtvk_sampler_rebuild(sampler)) sampler->max_anisotropy = old;
}

void rtSamplerSetLod(rt_sampler handle, f32 min_lod, f32 max_lod, f32 lod_bias) {
	rtvk_begin_errorable_operation();
	struct rtvk_sampler* sampler = rtvk_sampler_from_handle(handle);
	if (!sampler || min_lod > max_lod) {
		rtvk_throwf(RT_IMPROPER_USAGE, "rtSamplerSetLod requires a sampler and an ordered LOD range");
		return;
	}
	const f32 old_min = sampler->min_lod;
	const f32 old_max = sampler->max_lod;
	const f32 old_bias = sampler->lod_bias;
	sampler->min_lod = min_lod;
	sampler->max_lod = max_lod;
	sampler->lod_bias = lod_bias;
	if (!rtvk_sampler_rebuild(sampler)) {
		sampler->min_lod = old_min;
		sampler->max_lod = old_max;
		sampler->lod_bias = old_bias;
	}
}

RTVK_DEFINE_RESOURCE_PRIVATE(sampler)

VkSamplerAddressMode rtvk_sampler_address_mode(enum rt_address_mode mode) {
	switch (mode) {
	case RT_ADDRESS_CLAMP: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case RT_ADDRESS_MIRROR: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case RT_ADDRESS_REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
}

struct rtvk_sampler* rtvk_sampler_node_create(struct rtvk_context* ctx, const struct rtvk_sampler* settings) {
	struct rtvk_sampler* node = RTVK_ALLOC_RESOURCE(struct rtvk_sampler);
	if (!node) return NULL;
	rtvk_init_resource_base(ctx, RTVK_RESOURCE_BASE(node), node, rtvk_sampler_finalize_resource);
	node->mag_filter = settings->mag_filter;
	node->min_filter = settings->min_filter;
	node->mip_filter = settings->mip_filter;
	node->address_u = settings->address_u;
	node->address_v = settings->address_v;
	node->address_w = settings->address_w;
	node->max_anisotropy = settings->max_anisotropy;
	node->min_lod = settings->min_lod;
	node->max_lod = settings->max_lod;
	node->lod_bias = settings->lod_bias;

	VkPhysicalDeviceFeatures features = { 0 };
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceFeatures(ctx->vk_physical_device, &features);
	vkGetPhysicalDeviceProperties(ctx->vk_physical_device, &properties);
	f32 anisotropy = (f32)node->max_anisotropy;
	if (anisotropy > properties.limits.maxSamplerAnisotropy) anisotropy = properties.limits.maxSamplerAnisotropy;
	VkSamplerCreateInfo info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	info.magFilter = node->mag_filter == RT_FILTER_NEAREST ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	info.minFilter = node->min_filter == RT_FILTER_NEAREST ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	info.mipmapMode = node->mip_filter == RT_MIP_FILTER_LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
	info.addressModeU = rtvk_sampler_address_mode(node->address_u);
	info.addressModeV = rtvk_sampler_address_mode(node->address_v);
	info.addressModeW = rtvk_sampler_address_mode(node->address_w);
	info.mipLodBias = node->lod_bias;
	info.anisotropyEnable = features.samplerAnisotropy && anisotropy > 1.0f;
	info.maxAnisotropy = info.anisotropyEnable ? anisotropy : 1.0f;
	info.minLod = node->min_lod;
	info.maxLod = node->max_lod;
	info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	const VkResult result = vkCreateSampler(ctx->vk_device, &info, VK_ALLOCATOR, &node->vk_sampler);
	if (result != VK_SUCCESS) {
		rtvk_resource_retire(RTVK_RESOURCE_BASE(node));
		rtvk_throwf(rtvk_error_from_vk(result), "vkCreateSampler failed: %s", rtvk_vk_result_name(result));
		return NULL;
	}
	return node;
}

void rtvk_sampler_init(struct rtvk_context* ctx, struct rtvk_sampler* sampler) {
	rtvk_init_resource_base(ctx, RTVK_RESOURCE_BASE(sampler), sampler, rtvk_sampler_finalize_resource);
	sampler->mag_filter = RT_FILTER_LINEAR;
	sampler->min_filter = RT_FILTER_LINEAR;
	sampler->mip_filter = RT_MIP_FILTER_NONE;
	sampler->address_u = RT_ADDRESS_REPEAT;
	sampler->address_v = RT_ADDRESS_REPEAT;
	sampler->address_w = RT_ADDRESS_REPEAT;
	sampler->max_anisotropy = 1;
	sampler->active = rtvk_sampler_node_create(ctx, sampler);
}

void rtvk_sampler_finish(struct rtvk_sampler* sampler) {
	struct rtvk_context* ctx = sampler->base.ctx;
	if (sampler->vk_sampler) vkDestroySampler(ctx->vk_device, sampler->vk_sampler, VK_ALLOCATOR);
	sampler->vk_sampler = VK_NULL_HANDLE;
	if (sampler->active) {
		rtvk_resource_retire(RTVK_RESOURCE_BASE(sampler->active));
		sampler->active = NULL;
	}
	while (sampler->next) {
		struct rtvk_sampler* node = sampler->next;
		sampler->next = node->next;
		node->next = NULL;
		rtvk_resource_retire(RTVK_RESOURCE_BASE(node));
	}
}

bool rtvk_sampler_rebuild(struct rtvk_sampler* sampler) {
	struct rtvk_sampler* node = rtvk_sampler_node_create(sampler->base.ctx, sampler);
	if (!node) return false;
	if (sampler->active) {
		sampler->active->next = sampler->next;
		sampler->next = sampler->active;
	}
	sampler->active = node;
	rtvk_sampler_collect_nodes(sampler);
	return true;
}

void rtvk_sampler_collect_nodes(struct rtvk_sampler* sampler) {
	struct rtvk_sampler** link = &sampler->next;
	while (*link) {
		struct rtvk_sampler* node = *link;
		if (rtvk_atomic_load(&node->base.ref_count) == 1) {
			*link = node->next;
			node->next = NULL;
			rtvk_resource_retire(RTVK_RESOURCE_BASE(node));
			continue;
		}
		link = &node->next;
	}
}

struct rtvk_sampler* rtvk_sampler_active_node(struct rtvk_sampler* sampler) {
	return sampler ? sampler->active : NULL;
}
