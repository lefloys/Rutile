#include "core.h"
#include "error.h"
#include "buffer.h"
#include "command_buffer.h"
#include "framebuffer.h"
#include "queue.h"
#include "program.h"
#include "texture.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static int test_triangle_program(void) {
#ifdef RTSW_TRIANGLE_ARTIFACT
	struct vertex {
		f32 position[3];
		f32 color[4];
	};
	const struct vertex vertices[] = {
		{ { -0.8f, -0.8f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.8f, -0.8f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { 0.0f, 1.6f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
	};
	const u16 indices[] = { 0, 1, 2 };
	const rt_vertex_attribute attributes[] = {
		{ "position", offsetof(struct vertex, position), RT_RGB32_SFLOAT },
		{ "color", offsetof(struct vertex, color), RT_RGBA32_SFLOAT },
	};
	const rt_vertex_input input = { attributes, 2, sizeof(struct vertex), RT_VERTEX_RATE_VERTEX };
	const rt_vertex_layout layout = { &input, 1 };
	FILE* file = fopen(RTSW_TRIANGLE_ARTIFACT, "rb");
	long file_size;
	u08* bytes;
	rt_program program;
	rt_buffer vertex_buffer;
	rt_buffer index_buffer;
	rt_texture texture;
	rt_texture_view texture_view;
	rt_texture depth_texture;
	rt_texture_view depth_view;
	rt_framebuffer framebuffer;
	rt_command_buffer command_buffer;
	rt_command_buffer primary;
	rt_queue queue;
	rt_location location;
	u08 pixels[8 * 8 * 4] = { 0 };
	f32 depth_pixels[8 * 8] = { 0 };
	rt_texture_range range = { RT_TEXTURE_ASPECT_COLOR, 0, 1, 0, 1, { 8, 8, 1 }, { 0, 0, 0 } };
	if (!file) return 1;
	if (fseek(file, 0, SEEK_END) != 0 || (file_size = ftell(file)) <= 0 || fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return 1;
	}
	bytes = malloc((usize)file_size);
	if (!bytes || fread(bytes, 1, (usize)file_size, file) != (usize)file_size) {
		free(bytes);
		fclose(file);
		return 1;
	}
	fclose(file);
	program = rtProgramCreate();
	rtProgramSource(program, "main", bytes, (usize)file_size);
	rtProgramSetLayout(program, &layout);
	rtProgramFinalize(program);
	free(bytes);
	if (rtError() != RT_SUCCESS) {
		rtProgramDestroy(program);
		return 1;
	}
	location = rtProgramInputLocation(program, attributes, 2);
	vertex_buffer = rtBufferCreate();
	rtBufferResize(vertex_buffer, RT_DEVICE_MEMORY, sizeof(vertices));
	index_buffer = rtBufferCreate();
	rtBufferResize(index_buffer, RT_DEVICE_MEMORY, sizeof(indices));
	texture = rtTextureCreate();
	rtTextureResize(texture, RT_TEXTURE_2D, RT_RGBA8_UNORM, (rt_extent_3d){ 8, 8, 1 }, 1);
	texture_view = rtTextureViewCreate();
	rtTextureViewSetTexture(texture_view, texture);
	framebuffer = rtFramebufferCreate();
	rtFramebufferSetColorView(framebuffer, texture_view, RT_NULL_HANDLE);
	depth_texture = rtTextureCreate();
	rtTextureResize(depth_texture, RT_TEXTURE_2D, RT_D32_SFLOAT, (rt_extent_3d){ 8, 8, 1 }, 1);
	depth_view = rtTextureViewCreate();
	rtTextureViewSetTexture(depth_view, depth_texture);
	rtFramebufferSetDepthView(framebuffer, depth_view);
	queue = rtQueueCreate(RT_QUEUE_GRAPHICS);
	command_buffer = rtCommandBufferCreate();
	rtCommandBufferBegin(command_buffer);
	rtCmdBufferData(command_buffer, vertex_buffer, (rt_buffer_range){ sizeof(vertices), 0 }, (const u08*)vertices);
	rtCmdBufferData(command_buffer, index_buffer, (rt_buffer_range){ sizeof(indices), 0 }, (const u08*)indices);
	rtCommandBufferEnd(command_buffer);
	rtQueueSubmit(queue, command_buffer);
	rtCommandBufferReset(command_buffer);
	rtCommandBufferContinueRendering(command_buffer);
	rtCmdUseProgram(command_buffer, program);
	rtCmdVertexBuffer(command_buffer, location, vertex_buffer, (rt_buffer_range){ sizeof(vertices), 0 });
	rtCmdIndexBuffer(command_buffer, index_buffer, (rt_buffer_range){ sizeof(indices), 0 }, RT_INDEX_U16);
	rtCmdDrawIndexed(command_buffer, 3, 0, 0);
	rtCommandBufferEnd(command_buffer);
	primary = rtCommandBufferCreate();
	rtCommandBufferBegin(primary);
	rtCmdBeginRendering(primary, framebuffer);
	rtCmdClearColor(primary, RT_NULL_HANDLE, 0.0f, 0.0f, 0.0f, 1.0f);
	rtCmdClearDepth(primary, 1.0f);
	rtCmdClear(primary, RT_CLEAR_COLOR | RT_CLEAR_DEPTH);
	rtCmdSetViewport(primary, 2, 2, 4, 4, 0.0f, 1.0f);
	rtCmdSetScissor(primary, 3, 3, 2, 2);
	rtCmdExecute(primary, command_buffer);
	rtCmdEndRendering(primary);
	rtCommandBufferEnd(primary);
	rtQueueSubmit(queue, primary);
	rtTextureViewRead(texture_view, range, pixels, sizeof(pixels));
	rtTextureViewRead(depth_view, (rt_texture_range){ RT_TEXTURE_ASPECT_DEPTH, 0, 1, 0, 1, { 8, 8, 1 }, { 0, 0, 0 } }, (u08*)depth_pixels, sizeof(depth_pixels));
	rtCommandBufferDestroy(primary);
	rtCommandBufferDestroy(command_buffer);
	rtQueueDestroy(queue);
	rtFramebufferDestroy(framebuffer);
	rtTextureViewDestroy(depth_view);
	rtTextureDestroy(depth_texture);
	rtTextureViewDestroy(texture_view);
	rtTextureDestroy(texture);
	rtBufferDestroy(vertex_buffer);
	rtBufferDestroy(index_buffer);
	if (!location || rtError() != RT_SUCCESS ||
		(pixels[(4 * 8 + 4) * 4] == 0 && pixels[(4 * 8 + 4) * 4 + 1] == 0 && pixels[(4 * 8 + 4) * 4 + 2] == 0) ||
		pixels[(4 * 8 + 2) * 4] != 0 || pixels[(4 * 8 + 2) * 4 + 1] != 0 || pixels[(4 * 8 + 2) * 4 + 2] != 0 ||
		depth_pixels[4 * 8 + 4] != 0.0f || depth_pixels[4 * 8 + 2] != 1.0f) {
		rtProgramDestroy(program);
		return 1;
	}
	rtProgramDestroy(program);
#endif
	return 0;
}

static int test_linked_program_location(void) {
#ifdef RTSW_CUBE_ARTIFACT
	FILE* file = fopen(RTSW_CUBE_ARTIFACT, "rb");
	long file_size;
	u08* bytes;
	rt_program program;
	rt_location location;
	rt_buffer buffer;
	rt_command_buffer command_buffer;
	if (!file) return 1;
	if (fseek(file, 0, SEEK_END) != 0 || (file_size = ftell(file)) <= 0 || fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return 1;
	}
	bytes = malloc((usize)file_size);
	if (!bytes || fread(bytes, 1, (usize)file_size, file) != (usize)file_size) {
		free(bytes);
		fclose(file);
		return 1;
	}
	fclose(file);
	program = rtProgramCreate();
	rtProgramSource(program, "main", bytes, (usize)file_size);
	free(bytes);
	if (rtError() != RT_SUCCESS) {
		fprintf(stderr, "linked program source failed: %s\n", rtErrorMessage());
		rtProgramDestroy(program);
		return 1;
	}
	rtProgramFinalize(program);
	if (rtError() != RT_SUCCESS) {
		fprintf(stderr, "linked program finalization failed: %s\n", rtErrorMessage());
		rtProgramDestroy(program);
		return 1;
	}
	location = rtProgramUniformLocation(program, "scene");
	if (!location || rtError() != RT_SUCCESS || rtProgramUniformLocation(program, "missing") != NULL || rtError() != RT_SUCCESS) {
		fprintf(stderr, "linked program location lookup failed: %s\n", rtErrorMessage());
		rtProgramDestroy(program);
		return 1;
	}
	buffer = rtBufferCreate();
	rtBufferResize(buffer, RT_DEVICE_MEMORY, sizeof(f32) * 16);
	command_buffer = rtCommandBufferCreate();
	rtCommandBufferBegin(command_buffer);
	rtCmdBindBuffer(command_buffer, location, buffer, (rt_buffer_range){ sizeof(f32) * 16, 0 });
	rtCommandBufferEnd(command_buffer);
	rtBufferDestroy(buffer);
	rtProgramDestroy(program);
	rtCommandBufferReset(command_buffer);
	rtCommandBufferDestroy(command_buffer);
	if (rtError() != RT_SUCCESS) return 1;
#endif
	return 0;
}

int main(void) {
	const u08 input[] = { 1, 2, 3, 4 };
	u08 output[sizeof(input)] = { 0 };
	rt_buffer source;
	rt_buffer destination;
	rt_command_buffer command_buffer;
	rt_queue queue;
	rt_queue waiting_queue;
	rt_timepoint submitted;
	rt_texture texture;
	rt_texture_view texture_view;
	rt_texture depth_texture;
	rt_texture_view depth_view;
	rt_texture stencil_texture;
	rt_texture_view stencil_view;
	rt_framebuffer framebuffer;
	u08 pixels[16] = { 0 };
	f32 depth_pixels[4] = { 0 };
	u08 stencil_pixels[4] = { 0 };
	rt_texture_range texture_range = {
		RT_TEXTURE_ASPECT_COLOR,
		0,
		1,
		0,
		1,
		{ 2, 2, 1 },
		{ 0, 0, 0 },
	};

	rtInit(NULL, 0);
	if (rtError() != RT_SUCCESS) return 1;
	rtBufferResize(RT_NULL_HANDLE, RT_HOST_MEMORY, 1);
	if (rtError() != RT_IMPROPER_USAGE) return 1;
	if (rtVersion() != RT_HEADER_VERSION || rtError() != RT_IMPROPER_USAGE) return 1;
	source = rtBufferCreate();
	if (!source || rtError() != RT_SUCCESS) return 1;
	rtBufferDestroy(source);
	if (test_triangle_program() != 0) return 1;
	if (test_linked_program_location() != 0) return 1;

	source = rtBufferCreate();
	destination = rtBufferCreate();
	rtBufferResize(source, RT_DEVICE_MEMORY, sizeof(input));
	rtBufferResize(destination, RT_HOST_MEMORY, sizeof(output));

	command_buffer = rtCommandBufferCreate();
	rtCommandBufferBegin(command_buffer);
	rtCmdBufferData(command_buffer, source, (rt_buffer_range){ sizeof(input), 0 }, input);
	rtCmdBufferCopy(command_buffer, source, (rt_buffer_range){ sizeof(input), 0 }, destination, (rt_buffer_range){ sizeof(output), 0 });
	rtCommandBufferEnd(command_buffer);

	rtBufferDestroy(source);
	queue = rtQueueCreate(RT_QUEUE_GRAPHICS);
	submitted = rtQueueSubmit(queue, command_buffer);
	waiting_queue = rtQueueCreate(RT_QUEUE_TRANSFER);
	rtQueueWait(waiting_queue, submitted);
	rtQueueSubmit(waiting_queue, command_buffer);
	if (rtError() != RT_SUCCESS || !rtTimepointReached(submitted)) return 1;
	rtQueueDestroy(waiting_queue);
	rtBufferRead(destination, (rt_buffer_range){ sizeof(output), 0 }, output, sizeof(output));

	texture = rtTextureCreate();
	rtTextureResize(texture, RT_TEXTURE_2D, RT_RGBA8_UNORM, (rt_extent_3d){ 2, 2, 1 }, 1);
	texture_view = rtTextureViewCreate();
	rtTextureViewSetTexture(texture_view, texture);
	framebuffer = rtFramebufferCreate();
	rtFramebufferSetColorView(framebuffer, texture_view, RT_NULL_HANDLE);
	depth_texture = rtTextureCreate();
	rtTextureResize(depth_texture, RT_TEXTURE_2D, RT_D32_SFLOAT, (rt_extent_3d){ 2, 2, 1 }, 1);
	depth_view = rtTextureViewCreate();
	rtTextureViewSetTexture(depth_view, depth_texture);
	rtFramebufferSetDepthView(framebuffer, depth_view);
	stencil_texture = rtTextureCreate();
	rtTextureResize(stencil_texture, RT_TEXTURE_2D, RT_S8_UINT, (rt_extent_3d){ 2, 2, 1 }, 1);
	stencil_view = rtTextureViewCreate();
	rtTextureViewSetTexture(stencil_view, stencil_texture);
	rtFramebufferSetStencilView(framebuffer, stencil_view);
	rtCommandBufferReset(command_buffer);
	rtCommandBufferBegin(command_buffer);
	rtCmdBeginRendering(command_buffer, framebuffer);
	rtCmdClearColor(command_buffer, RT_NULL_HANDLE, 0.25f, 0.5f, 0.75f, 1.0f);
	rtCmdClearDepth(command_buffer, 0.25f);
	rtCmdClearStencil(command_buffer, 0xa5);
	rtCmdClear(command_buffer, RT_CLEAR_COLOR | RT_CLEAR_DEPTH | RT_CLEAR_STENCIL);
	rtCmdEndRendering(command_buffer);
	rtCommandBufferEnd(command_buffer);
	rtQueueSubmit(queue, command_buffer);
	rtTextureViewRead(texture_view, texture_range, pixels, sizeof(pixels));
	rtTextureViewRead(depth_view, (rt_texture_range){ RT_TEXTURE_ASPECT_DEPTH, 0, 1, 0, 1, { 2, 2, 1 }, { 0, 0, 0 } }, (u08*)depth_pixels, sizeof(depth_pixels));
	rtTextureViewRead(stencil_view, (rt_texture_range){ RT_TEXTURE_ASPECT_STENCIL, 0, 1, 0, 1, { 2, 2, 1 }, { 0, 0, 0 } }, stencil_pixels, sizeof(stencil_pixels));

	rtQueueDestroy(queue);
	rtCommandBufferDestroy(command_buffer);
	rtFramebufferDestroy(framebuffer);
	rtTextureViewDestroy(stencil_view);
	rtTextureDestroy(stencil_texture);
	rtTextureViewDestroy(depth_view);
	rtTextureDestroy(depth_texture);
	rtTextureViewDestroy(texture_view);
	rtTextureDestroy(texture);
	rtBufferDestroy(destination);
	rtExit();

	return memcmp(input, output, sizeof(input)) == 0 &&
		pixels[0] == 64 && pixels[1] == 128 && pixels[2] == 191 && pixels[3] == 255 &&
		depth_pixels[0] == 0.25f && depth_pixels[3] == 0.25f &&
		stencil_pixels[0] == 0xa5 && stencil_pixels[3] == 0xa5 &&
		pixels[12] == 64 && pixels[13] == 128 && pixels[14] == 191 && pixels[15] == 255 ? 0 : 1;
}
