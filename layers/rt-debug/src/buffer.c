#include "next.h"
#include "trace.h"
#include "resource/rasterizer.h"

RT_API_PUBLIC void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdBindBuffer command-buffer #%llu location %p buffer #%llu offset %llu size %llu", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (void*)location, (unsigned long long)rtdbg_trace_handle_id(buffer), (unsigned long long)range.offset, (unsigned long long)range.size);
	rtdbg_procs.rtCmdBindBuffer(command_buffer, location, buffer, range);
}

RT_API_PUBLIC void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range) {
	rtdbg_rasterizer_vertex_buffer(command_buffer, buffer, range);
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdVertexBuffer command-buffer #%llu location %p buffer #%llu offset %llu size %llu", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (void*)location, (unsigned long long)rtdbg_trace_handle_id(buffer), (unsigned long long)range.offset, (unsigned long long)range.size);
	rtdbg_procs.rtCmdVertexBuffer(command_buffer, location, buffer, range);
}

RT_API_PUBLIC void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, enum rt_index_format format) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdIndexBuffer command-buffer #%llu buffer #%llu offset %llu size %llu format %u", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (unsigned long long)rtdbg_trace_handle_id(buffer), (unsigned long long)range.offset, (unsigned long long)range.size, (unsigned)format);
	rtdbg_procs.rtCmdIndexBuffer(command_buffer, buffer, range, format);
}

RT_API_PUBLIC void rtBufferResize(rt_buffer buffer, enum rt_memory_type memory_type, usize size) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtBufferResize buffer #%llu memory %u bytes %llu", (unsigned long long)rtdbg_trace_handle_id(buffer), (unsigned)memory_type, (unsigned long long)size);
	rtdbg_trace_resource_detail(buffer, "memory type %u; size %llu bytes", (unsigned)memory_type, (unsigned long long)size);
	rtdbg_procs.rtBufferResize(buffer, memory_type, size);
}

RT_API_PUBLIC void rtBufferRead(rt_buffer buffer, rt_buffer_range range, u08 * data, usize data_size) {
	rtdbg_trace_api("rtBufferRead");
	rtdbg_procs.rtBufferRead(buffer, range, data, data_size);
}

RT_API_PUBLIC u08* rtBufferMap(rt_buffer buffer, rt_buffer_range range) {
	rtdbg_trace_api("rtBufferMap");
	return rtdbg_procs.rtBufferMap(buffer, range);
}

RT_API_PUBLIC void rtBufferUnmap(rt_buffer buffer) {
	rtdbg_trace_api("rtBufferUnmap");
	rtdbg_procs.rtBufferUnmap(buffer);
}

RT_API_PUBLIC void rtCmdBufferData(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, const u08* data) {
	rtdbg_trace_api("rtCmdBufferData");
	rtdbg_rasterizer_buffer_data(buffer, range, data);
	rtdbg_procs.rtCmdBufferData(command_buffer, buffer, range, data);
}

RT_API_PUBLIC void rtCmdBufferCopy(rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_buffer dst, rt_buffer_range dst_range) {
	rtdbg_trace_event(RTDBG_TRACE_COMMAND, "rtCmdBufferCopy command-buffer #%llu buffer #%llu offset %llu size %llu -> buffer #%llu offset %llu size %llu", (unsigned long long)rtdbg_trace_handle_id(command_buffer), (unsigned long long)rtdbg_trace_handle_id(src), (unsigned long long)src_range.offset, (unsigned long long)src_range.size, (unsigned long long)rtdbg_trace_handle_id(dst), (unsigned long long)dst_range.offset, (unsigned long long)dst_range.size);
	rtdbg_procs.rtCmdBufferCopy(command_buffer, src, src_range, dst, dst_range);
}

RT_API_PUBLIC void rtCmdBufferCopyToTexture(rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_texture dst, rt_texture_range dst_range) {
	rtdbg_trace_api("rtCmdBufferCopyToTexture");
	rtdbg_procs.rtCmdBufferCopyToTexture(command_buffer, src, src_range, dst, dst_range);
}

RT_API_PUBLIC void rtCmdBufferBarrier(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, rt_access src, rt_access dst) {
	rtdbg_trace_api("rtCmdBufferBarrier");
	rtdbg_procs.rtCmdBufferBarrier(command_buffer, buffer, range, src, dst);
}

RT_API_PUBLIC rt_buffer rtBufferCreate(void) {
	rt_buffer handle = rtdbg_procs.rtBufferCreate();
	rtdbg_trace_resource_create("rtBufferCreate", "buffer", handle);
	return handle;
}

RT_API_PUBLIC void rtBufferDestroy(rt_buffer buffer) {
	rtdbg_trace_resource_destroy("rtBufferDestroy", "buffer", buffer);
	rtdbg_procs.rtBufferDestroy(buffer);
}

