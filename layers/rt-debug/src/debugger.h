#ifndef RTDBG_DEBUGGER_H
#define RTDBG_DEBUGGER_H

#include "rutile.h"

void rtdbg_debugger_start(void);
void rtdbg_debugger_record(uint64_t sequence, uint32_t kind, const char* text);
void rtdbg_debugger_resource_create(uint64_t resource_id, const char* type);
void rtdbg_debugger_resource_destroy(uint64_t resource_id);
void rtdbg_debugger_resource_detail(uint64_t resource_id, const char* text);
void rtdbg_debugger_resource_reset(void);
void rtdbg_debugger_point(void);
void rtdbg_debugger_texture_preview(uint64_t texture_id, usize width, usize height, const u08* rgba, usize byte_count);
void rtdbg_debugger_texture_preview_remove(uint64_t texture_id);
void rtdbg_debugger_texture_preview_reset(void);

#endif
