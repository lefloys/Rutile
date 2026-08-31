#include "next.h"
#include "trace.h"
#include "texture_preview.h"

RT_API_PUBLIC void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdBindTexture command-buffer #%llu location %p texture-view #%llu", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (void*)location, (unsigned long long)rtdbg_trace_handle_id(texture_view));
	rtdbg_procs.rtCmdBindTexture(command_buffer, location, texture_view);
}

RT_API_PUBLIC void rtTextureResize(rt_texture texture, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtTextureResize texture #%llu type %u format %u extent %llu x %llu x %llu mips %llu", (unsigned long long)rtdbg_trace_handle_id(texture), (unsigned)type, (unsigned)format, (unsigned long long)extent.width, (unsigned long long)extent.height, (unsigned long long)extent.depth, (unsigned long long)mip_count);
	rtdbg_trace_resource_detail(texture, "type %u; format %u; extent %llu x %llu x %llu; mip levels %llu", (unsigned)type, (unsigned)format, (unsigned long long)extent.width, (unsigned long long)extent.height, (unsigned long long)extent.depth, (unsigned long long)mip_count);
	rtdbg_texture_preview_resize(texture, type, format, extent, mip_count);
	rtdbg_procs.rtTextureResize(texture, type, format, extent, mip_count);
}

RT_API_PUBLIC void rtCmdTextureCopy(rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_texture dst, rt_texture_range dst_range) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdTextureCopy command-buffer #%llu texture #%llu -> texture #%llu", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (unsigned long long)rtdbg_trace_handle_id(src), (unsigned long long)rtdbg_trace_handle_id(dst));
	rtdbg_procs.rtCmdTextureCopy(command_buffer, src, src_range, dst, dst_range);
}

RT_API_PUBLIC void rtCmdTextureData(rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, const u08* data) {
	rtdbg_trace_api("rtCmdTextureData");
	rtdbg_texture_preview_data(texture, range, data);
	rtdbg_procs.rtCmdTextureData(command_buffer, texture, range, data);
}

RT_API_PUBLIC void rtCmdTextureCopyToBuffer(rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_buffer dst, rt_buffer_range dst_range) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdTextureCopyToBuffer command-buffer #%llu texture #%llu -> buffer #%llu offset %llu size %llu", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (unsigned long long)rtdbg_trace_handle_id(src), (unsigned long long)rtdbg_trace_handle_id(dst), (unsigned long long)dst_range.offset, (unsigned long long)dst_range.size);
	rtdbg_procs.rtCmdTextureCopyToBuffer(command_buffer, src, src_range, dst, dst_range);
}

RT_API_PUBLIC void rtCmdTextureBarrier(rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, rt_access src, rt_access dst) {
	rtdbg_trace_api("rtCmdTextureBarrier");
	rtdbg_procs.rtCmdTextureBarrier(command_buffer, texture, range, src, dst);
}

RT_API_PUBLIC rt_extent_3d rtTextureViewExtent(rt_texture_view texture_view) {
	rtdbg_trace_api("rtTextureViewExtent");
	return rtdbg_procs.rtTextureViewExtent(texture_view);
}

RT_API_PUBLIC void rtTextureViewSetTexture(rt_texture_view texture_view, rt_texture texture) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtTextureViewSetTexture texture-view #%llu texture #%llu", (unsigned long long)rtdbg_trace_handle_id(texture_view), (unsigned long long)rtdbg_trace_handle_id(texture));
	rtdbg_trace_resource_detail(texture_view, "texture #%llu", (unsigned long long)rtdbg_trace_handle_id(texture));
	rtdbg_procs.rtTextureViewSetTexture(texture_view, texture);
}

RT_API_PUBLIC void rtTextureViewRead(rt_texture_view texture_view, rt_texture_range range, u08 * data, usize data_size) {
	rtdbg_trace_api("rtTextureViewRead");
	rtdbg_procs.rtTextureViewRead(texture_view, range, data, data_size);
}

RT_API_PUBLIC rt_texture rtTextureCreate(void) {
	rt_texture handle = rtdbg_procs.rtTextureCreate();
	rtdbg_trace_resource_create("rtTextureCreate", "texture", handle);
	return handle;
}

RT_API_PUBLIC void rtTextureDestroy(rt_texture texture) {
	rtdbg_texture_preview_destroy(texture);
	rtdbg_trace_resource_destroy("rtTextureDestroy", "texture", texture);
	rtdbg_procs.rtTextureDestroy(texture);
}

RT_API_PUBLIC rt_texture_view rtTextureViewCreate(void) {
	rt_texture_view handle = rtdbg_procs.rtTextureViewCreate();
	rtdbg_trace_resource_create("rtTextureViewCreate", "texture-view", handle);
	return handle;
}

RT_API_PUBLIC void rtTextureViewDestroy(rt_texture_view texture_view) {
	rtdbg_trace_resource_destroy("rtTextureViewDestroy", "texture-view", texture_view);
	rtdbg_procs.rtTextureViewDestroy(texture_view);
}

