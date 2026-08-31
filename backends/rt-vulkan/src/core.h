#ifndef RTVK_CORE_H
#define RTVK_CORE_H

#include "config.h"
#include "error.h"
#include "resource/buffer.h"
#include "resource/command_buffer.h"
#include "resource/program.h"
#include "resource/queue.h"
#include "resource/swapchain.h"
#include "resource/texture.h"
#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTVK_API void rtInit(const char* const* features, usize feature_count);
RTVK_API void rtExit(void);
RTVK_API u64 rtVersion(void);
RTVK_API void rtSettingSet(const char* name, const char* value);
RTVK_API const char* rtGetName(void);
VkFormat rtvk_format_to_vk(enum rt_format format);

#endif /* RTVK_CORE_H */
