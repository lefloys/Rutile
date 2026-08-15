#include "procs.h"

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RT_API_PUBLIC rt_command_buffer rtCommandBufferCreate(void) { return rtlog_rtCommandBufferCreate(); }
RT_API_PUBLIC void rtCommandBufferDestroy(rt_command_buffer command_buffer) { rtlog_rtCommandBufferDestroy(command_buffer); }
RT_API_PUBLIC void rtCmdReset(rt_command_buffer command_buffer) { rtlog_rtCmdReset(command_buffer); }
RT_API_PUBLIC void rtCmdBegin(rt_command_buffer command_buffer) { rtlog_rtCmdBegin(command_buffer); }
RT_API_PUBLIC void rtCmdWait(rt_command_buffer command_buffer, rt_timepoint timepoint) { rtlog_rtCmdWait(command_buffer, timepoint); }
RT_API_PUBLIC void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer) { rtlog_rtCmdBeginRendering(command_buffer, framebuffer); }
RT_API_PUBLIC void rtCmdClearColor(rt_command_buffer command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a) { rtlog_rtCmdClearColor(command_buffer, color_index, r, g, b, a); }
RT_API_PUBLIC void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth) { rtlog_rtCmdClearDepth(command_buffer, depth); }
RT_API_PUBLIC void rtCmdClearStencil(rt_command_buffer command_buffer, u32 stencil) { rtlog_rtCmdClearStencil(command_buffer, stencil); }
RT_API_PUBLIC void rtCmdSetViewport(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth) { rtlog_rtCmdSetViewport(command_buffer, x, y, width, height, min_depth, max_depth); }
RT_API_PUBLIC void rtCmdSetScissor(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height) { rtlog_rtCmdSetScissor(command_buffer, x, y, width, height); }
RT_API_PUBLIC void rtCmdEndRendering(rt_command_buffer command_buffer) { rtlog_rtCmdEndRendering(command_buffer); }
RT_API_PUBLIC void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program) { rtlog_rtCmdUseGraphicsProgram(command_buffer, program); }
RT_API_PUBLIC void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset, usize size) { rtlog_rtCmdBindBuffer(command_buffer, location, buffer, offset, size); }
RT_API_PUBLIC void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view) { rtlog_rtCmdBindTexture(command_buffer, location, texture_view); }
RT_API_PUBLIC void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset) { rtlog_rtCmdVertexBuffer(command_buffer, location, buffer, offset); }
RT_API_PUBLIC void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, usize offset, enum rt_index_format format) { rtlog_rtCmdIndexBuffer(command_buffer, buffer, offset, format); }
RT_API_PUBLIC void rtCmdDraw(rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex) { rtlog_rtCmdDraw(command_buffer, vertex_count, first_vertex); }
RT_API_PUBLIC void rtCmdDrawInstanced(rt_command_buffer command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) { rtlog_rtCmdDrawInstanced(command_buffer, vertex_count, instance_count, first_vertex, first_instance); }
RT_API_PUBLIC void rtCmdDrawIndexed(rt_command_buffer command_buffer, u32 index_count, u32 first_index, i32 vertex_offset) { rtlog_rtCmdDrawIndexed(command_buffer, index_count, first_index, vertex_offset); }
RT_API_PUBLIC void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) { rtlog_rtCmdDrawIndexedInstanced(command_buffer, index_count, instance_count, first_index, vertex_offset, first_instance); }
RT_API_PUBLIC void rtCmdEnd(rt_command_buffer command_buffer) { rtlog_rtCmdEnd(command_buffer); }

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

rt_command_buffer rtlog_rtCommandBufferCreate(void) { return next_rtCommandBufferCreate(); }
void rtlog_rtCommandBufferDestroy(rt_command_buffer command_buffer) { next_rtCommandBufferDestroy(command_buffer); rtlog_error("rtCommandBufferDestroy"); }
void rtlog_rtCmdReset(rt_command_buffer command_buffer) { next_rtCmdReset(command_buffer); rtlog_error("rtCmdReset"); }
void rtlog_rtCmdBegin(rt_command_buffer command_buffer) { next_rtCmdBegin(command_buffer); rtlog_error("rtCmdBegin"); }
void rtlog_rtCmdWait(rt_command_buffer command_buffer, rt_timepoint timepoint) { next_rtCmdWait(command_buffer, timepoint); rtlog_error("rtCmdWait"); }
void rtlog_rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer) { next_rtCmdBeginRendering(command_buffer, framebuffer); rtlog_error("rtCmdBeginRendering"); }
void rtlog_rtCmdClearColor(rt_command_buffer command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a) { next_rtCmdClearColor(command_buffer, color_index, r, g, b, a); rtlog_error("rtCmdClearColor"); }
void rtlog_rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth) { next_rtCmdClearDepth(command_buffer, depth); rtlog_error("rtCmdClearDepth"); }
void rtlog_rtCmdClearStencil(rt_command_buffer command_buffer, u32 stencil) { next_rtCmdClearStencil(command_buffer, stencil); rtlog_error("rtCmdClearStencil"); }
void rtlog_rtCmdSetViewport(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth) { next_rtCmdSetViewport(command_buffer, x, y, width, height, min_depth, max_depth); rtlog_error("rtCmdSetViewport"); }
void rtlog_rtCmdSetScissor(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height) { next_rtCmdSetScissor(command_buffer, x, y, width, height); rtlog_error("rtCmdSetScissor"); }
void rtlog_rtCmdEndRendering(rt_command_buffer command_buffer) { next_rtCmdEndRendering(command_buffer); rtlog_error("rtCmdEndRendering"); }
void rtlog_rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program) { next_rtCmdUseGraphicsProgram(command_buffer, program); rtlog_error("rtCmdUseGraphicsProgram"); }
void rtlog_rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset, usize size) { next_rtCmdBindBuffer(command_buffer, location, buffer, offset, size); rtlog_error("rtCmdBindBuffer"); }
void rtlog_rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view) { next_rtCmdBindTexture(command_buffer, location, texture_view); rtlog_error("rtCmdBindTexture"); }
void rtlog_rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset) { next_rtCmdVertexBuffer(command_buffer, location, buffer, offset); rtlog_error("rtCmdVertexBuffer"); }
void rtlog_rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, usize offset, enum rt_index_format format) { next_rtCmdIndexBuffer(command_buffer, buffer, offset, format); rtlog_error("rtCmdIndexBuffer"); }
void rtlog_rtCmdDraw(rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex) { next_rtCmdDraw(command_buffer, vertex_count, first_vertex); rtlog_error("rtCmdDraw"); }
void rtlog_rtCmdDrawInstanced(rt_command_buffer command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) { next_rtCmdDrawInstanced(command_buffer, vertex_count, instance_count, first_vertex, first_instance); rtlog_error("rtCmdDrawInstanced"); }
void rtlog_rtCmdDrawIndexed(rt_command_buffer command_buffer, u32 index_count, u32 first_index, i32 vertex_offset) { next_rtCmdDrawIndexed(command_buffer, index_count, first_index, vertex_offset); rtlog_error("rtCmdDrawIndexed"); }
void rtlog_rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) { next_rtCmdDrawIndexedInstanced(command_buffer, index_count, instance_count, first_index, vertex_offset, first_instance); rtlog_error("rtCmdDrawIndexedInstanced"); }
void rtlog_rtCmdEnd(rt_command_buffer command_buffer) { next_rtCmdEnd(command_buffer); rtlog_error("rtCmdEnd"); }
