#ifndef RTSW_COMMAND_BUFFER_H
#define RTSW_COMMAND_BUFFER_H

#include "buffer.h"
#include "framebuffer.h"
#include "program.h"
#include "texture.h"

enum rtsw_command_opcode {
	RTSW_COMMAND_BUFFER_DATA,
	RTSW_COMMAND_BUFFER_COPY,
	RTSW_COMMAND_BUFFER_COPY_TO_TEXTURE,
	RTSW_COMMAND_TEXTURE_DATA,
	RTSW_COMMAND_TEXTURE_COPY,
	RTSW_COMMAND_TEXTURE_COPY_TO_BUFFER,
	RTSW_COMMAND_BEGIN_RENDERING,
	RTSW_COMMAND_END_RENDERING,
	RTSW_COMMAND_CLEAR_COLOR,
	RTSW_COMMAND_CLEAR_DEPTH,
	RTSW_COMMAND_CLEAR_STENCIL,
	RTSW_COMMAND_CLEAR,
	RTSW_COMMAND_SET_VIEWPORT,
	RTSW_COMMAND_SET_SCISSOR,
	RTSW_COMMAND_EXECUTE,
	RTSW_COMMAND_USE_PROGRAM,
	RTSW_COMMAND_BIND_BUFFER,
	RTSW_COMMAND_VERTEX_BUFFER,
	RTSW_COMMAND_INDEX_BUFFER,
	RTSW_COMMAND_DRAW,
	RTSW_COMMAND_DRAW_INSTANCED,
	RTSW_COMMAND_DRAW_INDEXED,
	RTSW_COMMAND_DRAW_INDEXED_INSTANCED,
};

struct rtsw_command_header {
	void* alignment;
	u08 opcode;
};

struct rtsw_ir_buffer_data {
	struct rtsw_buffer* buffer;
	rt_buffer_range range;
	u08* data;
};

struct rtsw_ir_buffer_copy {
	struct rtsw_buffer* src;
	struct rtsw_buffer* dst;
	rt_buffer_range src_range;
	rt_buffer_range dst_range;
};

struct rtsw_ir_texture_data {
	struct rtsw_texture* texture;
	rt_texture_range range;
	u08* data;
	usize data_size;
};

struct rtsw_ir_buffer_copy_to_texture {
	struct rtsw_buffer* src;
	struct rtsw_texture* dst;
	rt_buffer_range src_range;
	rt_texture_range dst_range;
};

struct rtsw_ir_texture_copy {
	struct rtsw_texture* src;
	struct rtsw_texture* dst;
	rt_texture_range src_range;
	rt_texture_range dst_range;
};

struct rtsw_ir_texture_copy_to_buffer {
	struct rtsw_texture* src;
	struct rtsw_buffer* dst;
	rt_texture_range src_range;
	rt_buffer_range dst_range;
};

struct rtsw_ir_begin_rendering {
	struct rtsw_framebuffer* framebuffer;
};

struct rtsw_ir_clear_color {
	f32 r;
	f32 g;
	f32 b;
	f32 a;
};

struct rtsw_ir_clear_depth { f32 depth; };
struct rtsw_ir_clear_stencil { usize stencil; };

struct rtsw_ir_clear {
	enum rt_clear_flag attachments;
};

struct rtsw_ir_rectangle {
	usize x;
	usize y;
	usize width;
	usize height;
	f32 min_depth;
	f32 max_depth;
};

struct rtsw_ir_use_program {
	struct rtsw_program* program;
};

struct rtsw_ir_bind_buffer {
	struct rtsw_program* program;
	struct rtsw_buffer* buffer;
	rt_buffer_range range;
	u32 symbol;
};

struct rtsw_ir_execute {
	struct rtsw_command_buffer* secondary;
};

struct rtsw_ir_vertex_buffer {
	struct rtsw_buffer* buffer;
	rt_buffer_range range;
	u32 input;
};

struct rtsw_ir_draw {
	usize vertex_count;
	usize first_vertex;
};

struct rtsw_ir_index_buffer {
	struct rtsw_buffer* buffer;
	rt_buffer_range range;
	enum rt_index_format format;
};

struct rtsw_ir_draw_indexed {
	usize index_count;
	usize first_index;
	usize vertex_offset;
};

struct rtsw_ir_draw_instanced {
	usize vertex_count;
	usize instance_count;
	usize first_vertex;
	usize first_instance;
};

struct rtsw_ir_draw_indexed_instanced {
	usize index_count;
	usize instance_count;
	usize first_index;
	usize vertex_offset;
	usize first_instance;
};

struct rtsw_command_buffer {
	struct rtsw_resource_base base;
	u08* ir_data;
	usize ir_size;
	usize ir_capacity;
	bool recording;
	bool executable;
	bool rendering;
	bool rendering_continuation;
};

RTSW_API rt_command_buffer rtCommandBufferCreate(void);
RTSW_API void rtCommandBufferDestroy(rt_command_buffer command_buffer);
RTSW_API void rtCommandBufferReset(rt_command_buffer command_buffer);
RTSW_API void rtCommandBufferBegin(rt_command_buffer command_buffer);
RTSW_API void rtCommandBufferContinue(rt_command_buffer command_buffer);
RTSW_API void rtCommandBufferContinueRendering(rt_command_buffer command_buffer);
RTSW_API void rtCommandBufferEnd(rt_command_buffer command_buffer);
RTSW_API void rtCmdExecute(rt_command_buffer command_buffer, rt_command_buffer secondary);
RTSW_API void rtCmdBufferData(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, const u08* data);
RTSW_API void rtCmdBufferCopy(rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_buffer dst, rt_buffer_range dst_range);
RTSW_API void rtCmdBufferCopyToTexture(rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_texture dst, rt_texture_range dst_range);
RTSW_API void rtCmdTextureData(rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, const u08* data);
RTSW_API void rtCmdTextureCopy(rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_texture dst, rt_texture_range dst_range);
RTSW_API void rtCmdTextureCopyToBuffer(rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_buffer dst, rt_buffer_range dst_range);
RTSW_API void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer);
RTSW_API void rtCmdEndRendering(rt_command_buffer command_buffer);
RTSW_API void rtCmdClearColor(rt_command_buffer command_buffer, rt_location location, f32 r, f32 g, f32 b, f32 a);
RTSW_API void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth);
RTSW_API void rtCmdClearStencil(rt_command_buffer command_buffer, usize stencil);
RTSW_API void rtCmdClear(rt_command_buffer command_buffer, enum rt_clear_flag attachments);
RTSW_API void rtCmdSetViewport(rt_command_buffer command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth);
RTSW_API void rtCmdSetScissor(rt_command_buffer command_buffer, usize x, usize y, usize width, usize height);
RTSW_API void rtCmdUseProgram(rt_command_buffer command_buffer, rt_program program);
RTSW_API void rtCmdUniformData(rt_command_buffer command_buffer, rt_location location, const u08* data, usize size);
RTSW_API void rtCmdStorageData(rt_command_buffer command_buffer, rt_location location, const u08* data, usize size);
RTSW_API void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range);
RTSW_API void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range);
RTSW_API void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, enum rt_index_format format);
RTSW_API void rtCmdDraw(rt_command_buffer command_buffer, usize vertex_count, usize first_vertex);
RTSW_API void rtCmdDrawInstanced(rt_command_buffer command_buffer, usize vertex_count, usize instance_count, usize first_vertex, usize first_instance);
RTSW_API void rtCmdDrawIndexed(rt_command_buffer command_buffer, usize index_count, usize first_index, usize vertex_offset);
RTSW_API void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, usize index_count, usize instance_count, usize first_index, usize vertex_offset, usize first_instance);
RTSW_API void rtCmdBufferBarrier(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, rt_access src, rt_access dst);
RTSW_API void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view);
RTSW_API void rtCmdBindSampler(rt_command_buffer command_buffer, rt_location location, rt_sampler sampler);
RTSW_API void rtCmdTextureBarrier(rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, rt_access src, rt_access dst);

RTSW_DECLARE_HANDLE(command_buffer, rtsw_command_buffer);

void* rtsw_command_buffer_append(struct rtsw_command_buffer* command_buffer, enum rtsw_command_opcode opcode, usize payload_size);
usize rtsw_command_record_size(enum rtsw_command_opcode opcode);
void rtsw_command_buffer_release_resources(struct rtsw_command_buffer* command_buffer);

#endif
