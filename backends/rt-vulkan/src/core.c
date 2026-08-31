#include "core.h"
#include "context.h"

#include <stdio.h>
#include <string.h>

static bool rtvk_validate_init_features(const char* const* features, usize feature_count, rtvk_context_flags* flags);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

void rtInit(const char* const* features, usize feature_count) {
	rtvk_context_flags flags;

	rtvk_begin_errorable_operation();
	if (current_context) {
		rtvk_throwf(RT_ALREADY_INITIALIZED, "rtInit called while rt-vulkan is already initialized");
		return;
	}

	if (!rtvk_validate_init_features(features, feature_count, &flags)) {
		return;
	}

	rtvk_printf("rutile: initializing backend rt-vulkan\n");
	current_context = rtvk_create_context(flags);
}
void rtExit(void) {
	rtvk_context_destroy(current_context);
	current_context = NULL;
}
u64 rtVersion(void) { return RT_HEADER_VERSION; }
void rtSettingSet(const char* name, const char* value) {
	(void)name;
	(void)value;
}
const char* rtGetName(void) { return "rt-vulkan"; }

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

VkFormat rtvk_format_to_vk(enum rt_format format) {
	switch (format) {
	case RT_R8_UNORM:
		return VK_FORMAT_R8_UNORM;
	case RT_RG8_UNORM:
		return VK_FORMAT_R8G8_UNORM;
	case RT_RGB8_UNORM:
		return VK_FORMAT_R8G8B8_UNORM;
	case RT_RGBA8_UNORM:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case RT_R16_UNORM:
		return VK_FORMAT_R16_UNORM;
	case RT_RG16_UNORM:
		return VK_FORMAT_R16G16_UNORM;
	case RT_RGB16_UNORM:
		return VK_FORMAT_R16G16B16_UNORM;
	case RT_RGBA16_UNORM:
		return VK_FORMAT_R16G16B16A16_UNORM;
	case RT_R16_SFLOAT:
		return VK_FORMAT_R16_SFLOAT;
	case RT_RG16_SFLOAT:
		return VK_FORMAT_R16G16_SFLOAT;
	case RT_RGB16_SFLOAT:
		return VK_FORMAT_R16G16B16_SFLOAT;
	case RT_RGBA16_SFLOAT:
		return VK_FORMAT_R16G16B16A16_SFLOAT;
	case RT_R32_SFLOAT:
		return VK_FORMAT_R32_SFLOAT;
	case RT_RG32_SFLOAT:
		return VK_FORMAT_R32G32_SFLOAT;
	case RT_RGB32_SFLOAT:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case RT_RGBA32_SFLOAT:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case RT_R8_SINT:
		return VK_FORMAT_R8_SINT;
	case RT_RG8_SINT:
		return VK_FORMAT_R8G8_SINT;
	case RT_RGB8_SINT:
		return VK_FORMAT_R8G8B8_SINT;
	case RT_RGBA8_SINT:
		return VK_FORMAT_R8G8B8A8_SINT;
	case RT_R16_SINT:
		return VK_FORMAT_R16_SINT;
	case RT_RG16_SINT:
		return VK_FORMAT_R16G16_SINT;
	case RT_RGB16_SINT:
		return VK_FORMAT_R16G16B16_SINT;
	case RT_RGBA16_SINT:
		return VK_FORMAT_R16G16B16A16_SINT;
	case RT_R32_SINT:
		return VK_FORMAT_R32_SINT;
	case RT_RG32_SINT:
		return VK_FORMAT_R32G32_SINT;
	case RT_RGB32_SINT:
		return VK_FORMAT_R32G32B32_SINT;
	case RT_RGBA32_SINT:
		return VK_FORMAT_R32G32B32A32_SINT;
	case RT_R8_UINT:
		return VK_FORMAT_R8_UINT;
	case RT_RG8_UINT:
		return VK_FORMAT_R8G8_UINT;
	case RT_RGB8_UINT:
		return VK_FORMAT_R8G8B8_UINT;
	case RT_RGBA8_UINT:
		return VK_FORMAT_R8G8B8A8_UINT;
	case RT_R16_UINT:
		return VK_FORMAT_R16_UINT;
	case RT_RG16_UINT:
		return VK_FORMAT_R16G16_UINT;
	case RT_RGB16_UINT:
		return VK_FORMAT_R16G16B16_UINT;
	case RT_RGBA16_UINT:
		return VK_FORMAT_R16G16B16A16_UINT;
	case RT_R32_UINT:
		return VK_FORMAT_R32_UINT;
	case RT_RG32_UINT:
		return VK_FORMAT_R32G32_UINT;
	case RT_RGB32_UINT:
		return VK_FORMAT_R32G32B32_UINT;
	case RT_RGBA32_UINT:
		return VK_FORMAT_R32G32B32A32_UINT;
	case RT_D16_UNORM:
		return VK_FORMAT_D16_UNORM;
	case RT_D32_SFLOAT:
		return VK_FORMAT_D32_SFLOAT;
	case RT_S8_UINT:
		return VK_FORMAT_S8_UINT;
	case RT_D24_UNORM_S8_UINT:
		return VK_FORMAT_D24_UNORM_S8_UINT;
	case RT_D32_SFLOAT_S8_UINT:
		return VK_FORMAT_D32_SFLOAT_S8_UINT;
	default:
		return VK_FORMAT_UNDEFINED;
	}
}
static bool rtvk_feature_equals(const char* feature, const char* expected) {
	return feature && strcmp(feature, expected) == 0;
}
static bool rtvk_validate_init_features(const char* const* features, usize feature_count, rtvk_context_flags* flags) {
	if (feature_count && !features) {
		rtvk_throwf(RT_IMPROPER_USAGE, "rtInit feature_count is %zu but features is NULL", feature_count);
		return false;
	}

	*flags = (rtvk_context_flags){ 0 };
	for (usize i = 0; i < feature_count; i++) {
		const char* feature = features[i];
		if (!feature) {
			rtvk_throwf(RT_IMPROPER_USAGE, "rtInit feature at index %zu is NULL", i);
			return false;
		}
		if (rtvk_feature_equals(feature, RT_FEATURE_PRESENTATION)) {
			flags->presentation = true;
			continue;
		}
		rtvk_throwf(RT_UNSUPPORTED_FEATURE, "unsupported rtInit feature: %s", feature);
		return false;
	}

	return true;
}
