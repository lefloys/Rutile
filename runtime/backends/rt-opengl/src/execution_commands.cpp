#include "execution_internal.hpp"

#include "glad/gl.h"
#include "glfw/glfw.h"
#include "resource/buffer.h"
#include "resource/framebuffer.h"
#include "resource/graphics_program.h"
#include "resource/queue.h"
#include "resource/resource.h"
#include "resource/swapchain.h"
#include "resource/texture.h"
#include "rtsl_spirv.h"

#include <string.h>

static GLenum rtgl_buffer_gl_usage(void) {
	return GL_DYNAMIC_DRAW;
}

void rtgl_execution_buffer_create(struct rtgl_context* ctx, struct rtgl_buffer_storage* storage) {
	rtgl_execution_submit_sync(ctx, [storage](struct rtgl_context*) {
		glCreateBuffers(1, &storage->gl_buffer);
		glCreateTextures(GL_TEXTURE_BUFFER, 1, &storage->gl_texture_buffer);
	});
}

void rtgl_execution_buffer_delete(struct rtgl_context* ctx, struct rtgl_buffer_storage* storage) {
	rtgl_execution_submit_sync(ctx, [storage](struct rtgl_context*) {
		if (storage->gl_buffer) {
			glDeleteBuffers(1, &storage->gl_buffer);
			storage->gl_buffer = 0;
		}
		if (storage->gl_texture_buffer) {
			glDeleteTextures(1, &storage->gl_texture_buffer);
			storage->gl_texture_buffer = 0;
		}
	});
}

void rtgl_execution_buffer_data(struct rtgl_context* ctx, struct rtgl_buffer_storage* storage, usize size, const u08* bytes) {
	rtgl_execution_submit_sync(ctx, [storage, size, bytes](struct rtgl_context*) {
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glNamedBufferData(storage->gl_buffer, (GLsizeiptr)size, bytes, rtgl_buffer_gl_usage());
	});
}

void rtgl_execution_buffer_subdata(struct rtgl_context* ctx, struct rtgl_buffer_storage* storage, u64 offset, u64 size, const u08* bytes) {
	rtgl_execution_submit_sync(ctx, [storage, offset, size, bytes](struct rtgl_context*) {
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glNamedBufferSubData(storage->gl_buffer, (GLintptr)offset, (GLsizeiptr)size, bytes);
	});
}

void rtgl_execution_buffer_read(struct rtgl_context* ctx, struct rtgl_buffer_storage* storage, u64 offset, u64 size, u08* bytes) {
	rtgl_execution_submit_sync(ctx, [storage, offset, size, bytes](struct rtgl_context*) {
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glGetNamedBufferSubData(storage->gl_buffer, (GLintptr)offset, (GLsizeiptr)size, bytes);
	});
}

void rtgl_execution_framebuffer_create(struct rtgl_context* ctx, struct rtgl_framebuffer* framebuffer) {
	rtgl_execution_submit_sync(ctx, [framebuffer](struct rtgl_context*) {
		glCreateFramebuffers(1, &framebuffer->gl_framebuffer);
	});
}

void rtgl_execution_framebuffer_delete(struct rtgl_context* ctx, struct rtgl_framebuffer* framebuffer) {
	rtgl_execution_submit_sync(ctx, [framebuffer](struct rtgl_context*) {
		if (framebuffer->gl_framebuffer) {
			glDeleteFramebuffers(1, &framebuffer->gl_framebuffer);
			framebuffer->gl_framebuffer = 0;
		}
	});
}

void rtgl_execution_framebuffer_attach_color(struct rtgl_context* ctx, struct rtgl_framebuffer* framebuffer, u32 slot, struct rtgl_texture_view* view) {
	rtgl_execution_submit_sync(ctx, [framebuffer, slot, view](struct rtgl_context*) {
		GLuint texture = view && view->image ? view->image->gl_texture : 0;
		GLenum draw_buffer = GL_COLOR_ATTACHMENT0;
		glNamedFramebufferTexture(framebuffer->gl_framebuffer, GL_COLOR_ATTACHMENT0 + slot, texture, 0);
		glNamedFramebufferDrawBuffers(framebuffer->gl_framebuffer, 1, &draw_buffer);
		if (texture) {
			GLenum status = glCheckNamedFramebufferStatus(framebuffer->gl_framebuffer, GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				rtgl_throwf(RT_INITIALIZATION_FAILED, "OpenGL framebuffer is incomplete: 0x%04x", status);
			}
		}
	});
}

void rtgl_execution_framebuffer_attach_depth(struct rtgl_context* ctx, struct rtgl_framebuffer* framebuffer, struct rtgl_texture_view* view) {
	rtgl_execution_submit_sync(ctx, [framebuffer, view](struct rtgl_context*) {
		GLuint texture = view && view->image ? view->image->gl_texture : 0;
		glNamedFramebufferTexture(framebuffer->gl_framebuffer, GL_DEPTH_ATTACHMENT, texture, 0);
		if (framebuffer->color_texture_count && texture) {
			GLenum status = glCheckNamedFramebufferStatus(framebuffer->gl_framebuffer, GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				rtgl_throwf(RT_INITIALIZATION_FAILED, "OpenGL framebuffer with depth is incomplete: 0x%04x", status);
			}
		}
	});
}

static GLuint rtgl_execution_compile_spirv_shader(GLenum stage, const u32* words, u64 word_count) {
	if (!words || word_count == 0) {
		return 0;
	}
	GLuint shader = glCreateShader(stage);
	glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, words, (GLsizei)(word_count * sizeof(u32)));
	glSpecializeShader(shader, "main", 0, NULL, NULL);
	GLint ok = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[2048] = { 0 };
		glGetShaderInfoLog(shader, sizeof(log), NULL, log);
		rtgl_throwf(RT_SHADER_COMPILATION_FAILED, "OpenGL SPIR-V specialization failed: %s", log);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

static rtgl_uniform_location_kind rtgl_uniform_location_kind_from_spirv(rtsl_spirv_resource_kind kind) {
	switch (kind) {
	case RTSL_SPIRV_UNIFORM_BUFFER:
		return RTGL_UNIFORM_LOCATION_UNIFORM_BUFFER;
	case RTSL_SPIRV_STORAGE_BUFFER:
		return RTGL_UNIFORM_LOCATION_STORAGE_BUFFER;
	case RTSL_SPIRV_SAMPLED_TEXTURE:
	case RTSL_SPIRV_SAMPLER:
		return RTGL_UNIFORM_LOCATION_TEXTURE;
	case RTSL_SPIRV_STORAGE_IMAGE:
		return RTGL_UNIFORM_LOCATION_TEXTURE;
	}
	return RTGL_UNIFORM_LOCATION_UNIFORM_BUFFER;
}

static void rtgl_graphics_program_reflect_spirv(struct rtgl_graphics_program* program, const rtsl_spirv_translation* translation) {
	program->uniform_location_count = 0;
	const u32 resource_count = rtsl_spirv_resource_count(translation);
	for (u32 i = 0; i < resource_count && program->uniform_location_count < (u32)(sizeof(program->uniform_locations) / sizeof(program->uniform_locations[0])); i++) {
		rtsl_spirv_resource_info resource = { 0 };
		if (!rtsl_spirv_resource(translation, i, &resource) || !resource.name || resource.descriptor_set != 0) {
			continue;
		}
		rtgl_uniform_location* location = &program->uniform_locations[program->uniform_location_count++];
		memset(location, 0, sizeof(*location));
		location->program = program;
		strncpy(location->name, resource.name, sizeof(location->name) - 1);
		location->binding = resource.binding;
		location->gl_location = -1;
		location->kind = rtgl_uniform_location_kind_from_spirv(resource.kind);
	}
}

static bool rtgl_execution_graphics_program_finalize_spirv(struct rtgl_context* exec_ctx, struct rtgl_graphics_program* program) {
	(void)exec_ctx;
	if (!program->source_bytes || program->source_size == 0) {
		return false;
	}
	char message[2048] = { 0 };
	rtsl_spirv_translation* translation = NULL;
	rtsl_spirv_status status = rtsl_spirv_translate(program->source_size, program->source_bytes, &translation, message, sizeof(message));
	if (status != RTSL_SPIRV_SUCCESS) {
		rtgl_throwf(RT_SHADER_COMPILATION_FAILED, "OpenGL RTSL to SPIR-V failed: %s", message);
		return true;
	}

	u64 vertex_word_count = 0;
	u64 fragment_word_count = 0;
	const u32* vertex_words = rtsl_spirv_stage_words(translation, RTSL_SPIRV_VERTEX, &vertex_word_count);
	const u32* fragment_words = rtsl_spirv_stage_words(translation, RTSL_SPIRV_FRAGMENT, &fragment_word_count);
	GLuint vertex = rtgl_execution_compile_spirv_shader(GL_VERTEX_SHADER, vertex_words, vertex_word_count);
	GLuint fragment = vertex ? rtgl_execution_compile_spirv_shader(GL_FRAGMENT_SHADER, fragment_words, fragment_word_count) : 0;
	if (!vertex || !fragment) {
		if (vertex) {
			glDeleteShader(vertex);
		}
		rtsl_spirv_translation_destroy(translation);
		return true;
	}

	GLuint gl_program = glCreateProgram();
	glAttachShader(gl_program, vertex);
	glAttachShader(gl_program, fragment);
	glLinkProgram(gl_program);
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	GLint ok = GL_FALSE;
	glGetProgramiv(gl_program, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[2048] = { 0 };
		glGetProgramInfoLog(gl_program, sizeof(log), NULL, log);
		rtgl_throwf(RT_SHADER_LINK_FAILED, "OpenGL SPIR-V shader link failed: %s", log);
		glDeleteProgram(gl_program);
		rtsl_spirv_translation_destroy(translation);
		return true;
	}

	if (program->gl_program) {
		glDeleteProgram(program->gl_program);
	}
	program->gl_program = gl_program;
	rtgl_graphics_program_reflect_spirv(program, translation);
	rtsl_spirv_translation_destroy(translation);
	return true;
}

void rtgl_execution_graphics_program_finalize(struct rtgl_context* ctx, struct rtgl_graphics_program* program) {
	rtgl_retain_resource(program);
	rtgl_execution_submit_sync(ctx, [program](struct rtgl_context* exec_ctx) {
		const bool finalized = rtgl_execution_graphics_program_finalize_spirv(exec_ctx, program);
		if (finalized && program->gl_program) {
			for (u32 i = 0; i < program->uniform_location_count; i++) {
				rtgl_uniform_location* location = &program->uniform_locations[i];
				if (location->kind == RTGL_UNIFORM_LOCATION_TEXTURE) {
					location->gl_location = glGetUniformLocation(program->gl_program, location->name);
					if (location->gl_location >= 0) {
						glProgramUniform1i(program->gl_program, location->gl_location, (GLint)location->binding);
					}
				} else if (location->kind == RTGL_UNIFORM_LOCATION_STORAGE_BUFFER) {
					const GLuint block = glGetProgramResourceIndex(program->gl_program, GL_SHADER_STORAGE_BLOCK, location->name);
					if (block != GL_INVALID_INDEX) {
						glShaderStorageBlockBinding(program->gl_program, block, location->binding);
					}
				} else {
					const GLuint block = glGetUniformBlockIndex(program->gl_program, location->name);
					if (block != GL_INVALID_INDEX) {
						glUniformBlockBinding(program->gl_program, block, location->binding);
					}
				}
			}
		}
		rtgl_resource_release(RTGL_RESOURCE_BASE(program));
	});
}
void rtgl_execution_graphics_program_destroy(struct rtgl_context* ctx, struct rtgl_graphics_program* program) {
	rtgl_execution_submit_sync(ctx, [program](struct rtgl_context*) {
		if (program->gl_program) {
			glDeleteProgram(program->gl_program);
			program->gl_program = 0;
		}
	});
}

void rtgl_execution_texture_create(struct rtgl_context* ctx, struct rtgl_image_base* image) {
	rtgl_execution_submit_sync(ctx, [image](struct rtgl_context*) {
		glCreateTextures(image->gl_target, 1, &image->gl_texture);
	});
}

void rtgl_execution_texture_delete(struct rtgl_context* ctx, struct rtgl_image_base* image) {
	rtgl_execution_submit_sync(ctx, [image](struct rtgl_context*) {
		if (image->gl_texture) {
			glDeleteTextures(1, &image->gl_texture);
			image->gl_texture = 0;
		}
	});
}

void rtgl_execution_framebuffer_attach_stencil(struct rtgl_context* ctx, struct rtgl_framebuffer* framebuffer, struct rtgl_texture_view* view) {
	rtgl_execution_submit_sync(ctx, [framebuffer, view](struct rtgl_context*) {
		GLuint texture = view && view->image ? view->image->gl_texture : 0;
		glNamedFramebufferTexture(framebuffer->gl_framebuffer, GL_STENCIL_ATTACHMENT, texture, 0);
		if (framebuffer->color_texture_count && texture) {
			GLenum status = glCheckNamedFramebufferStatus(framebuffer->gl_framebuffer, GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				rtgl_throwf(RT_INITIALIZATION_FAILED, "OpenGL framebuffer with stencil is incomplete: 0x%04x", status);
			}
		}
	});
}

void rtgl_execution_texture_view_delete_sampler(struct rtgl_context* ctx, struct rtgl_texture_view* view) {
	rtgl_execution_submit_sync(ctx, [view](struct rtgl_context*) {
		if (view->gl_sampler) {
			glDeleteSamplers(1, &view->gl_sampler);
			view->gl_sampler = 0;
		}
	});
}

void rtgl_execution_texture_data(struct rtgl_context* ctx, struct rtgl_image_base* image, const void* data) {
	rtgl_execution_submit_sync(ctx, [image, data](struct rtgl_context*) {
		switch (image->type) {
		case RT_TEXTURE_1D:
			glTextureStorage1D(image->gl_texture, (GLsizei)image->mip_levels, image->gl_internal_format, (GLsizei)image->width);
			break;
		case RT_TEXTURE_1D_ARRAY:
			glTextureStorage2D(image->gl_texture, (GLsizei)image->mip_levels, image->gl_internal_format, (GLsizei)image->width, (GLsizei)image->depth);
			break;
		case RT_TEXTURE_2D:
			glTextureStorage2D(image->gl_texture, (GLsizei)image->mip_levels, image->gl_internal_format, (GLsizei)image->width, (GLsizei)image->height);
			break;
		case RT_TEXTURE_2D_ARRAY:
		case RT_TEXTURE_3D:
			glTextureStorage3D(image->gl_texture, (GLsizei)image->mip_levels, image->gl_internal_format, (GLsizei)image->width, (GLsizei)image->height, (GLsizei)image->depth);
			break;
		default:
			return;
		}
		glTextureParameteri(image->gl_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(image->gl_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(image->gl_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(image->gl_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (data) {
			/* Initial uploads use the complete base level. */
			const GLenum format = rtgl_texture_upload_format(image->format);
			const GLenum type = rtgl_texture_upload_type(image->format);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			switch (image->type) {
			case RT_TEXTURE_1D: glTextureSubImage1D(image->gl_texture, 0, 0, (GLsizei)image->width, format, type, data); break;
			case RT_TEXTURE_1D_ARRAY: glTextureSubImage2D(image->gl_texture, 0, 0, 0, (GLsizei)image->width, (GLsizei)image->depth, format, type, data); break;
			case RT_TEXTURE_2D: glTextureSubImage2D(image->gl_texture, 0, 0, 0, (GLsizei)image->width, (GLsizei)image->height, format, type, data); break;
			case RT_TEXTURE_2D_ARRAY:
			case RT_TEXTURE_3D: glTextureSubImage3D(image->gl_texture, 0, 0, 0, 0, (GLsizei)image->width, (GLsizei)image->height, (GLsizei)image->depth, format, type, data); break;
			default: break;
			}
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		}
	});
}

static rt_texture_range rtgl_execution_texture_mip_range(rt_texture_range range, usize level) {
	rt_texture_range mip = range;
	mip.base_mip += level;
	mip.mip_count = 1;
	mip.offset.width >>= level;
	mip.offset.height >>= level;
	mip.offset.depth >>= level;
	mip.extent.width >>= level;
	mip.extent.height >>= level;
	mip.extent.depth >>= level;
	if (!mip.extent.width) mip.extent.width = 1;
	if (!mip.extent.height) mip.extent.height = 1;
	if (!mip.extent.depth) mip.extent.depth = 1;
	return mip;
}

static usize rtgl_execution_texture_range_bytes(const struct rtgl_image_base* image, rt_texture_range range) {
	const usize bytes = rtgl_texture_format_aspect_size(image->format, range.aspects);
	const usize layers = image->type == RT_TEXTURE_1D_ARRAY || image->type == RT_TEXTURE_2D_ARRAY ? range.layer_count : 1;
	return range.extent.width * range.extent.height * range.extent.depth * layers * bytes;
}

void rtgl_execution_texture_subdata(struct rtgl_context* ctx, struct rtgl_image_base* image, rt_texture_range range, const void* data) {
	rtgl_execution_submit_sync(ctx, [image, range, data](struct rtgl_context*) {
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		const u08* bytes = (const u08*)data;
		usize offset = 0;
		for (usize level = 0; level < range.mip_count; level++) {
			const rt_texture_range mip = rtgl_execution_texture_mip_range(range, level);
			const void* mip_data = bytes + offset;
			const GLenum format = rtgl_texture_upload_format_aspect(image->format, mip.aspects);
			const GLenum type = rtgl_texture_upload_type_aspect(image->format, mip.aspects);
			switch (image->type) {
		case RT_TEXTURE_1D:
			glTextureSubImage1D(image->gl_texture, (GLint)mip.base_mip, (GLint)mip.offset.width, (GLsizei)mip.extent.width, format, type, mip_data);
			break;
		case RT_TEXTURE_1D_ARRAY:
			glTextureSubImage2D(image->gl_texture, (GLint)mip.base_mip, (GLint)mip.offset.width, (GLint)mip.base_layer, (GLsizei)mip.extent.width, (GLsizei)mip.layer_count, format, type, mip_data);
			break;
		case RT_TEXTURE_2D:
			glTextureSubImage2D(image->gl_texture, (GLint)mip.base_mip, (GLint)mip.offset.width, (GLint)mip.offset.height, (GLsizei)mip.extent.width, (GLsizei)mip.extent.height, format, type, mip_data);
			break;
		case RT_TEXTURE_2D_ARRAY:
			glTextureSubImage3D(image->gl_texture, (GLint)mip.base_mip, (GLint)mip.offset.width, (GLint)mip.offset.height, (GLint)mip.base_layer, (GLsizei)mip.extent.width, (GLsizei)mip.extent.height, (GLsizei)mip.layer_count, format, type, mip_data);
			break;
		case RT_TEXTURE_3D:
			glTextureSubImage3D(image->gl_texture, (GLint)mip.base_mip, (GLint)mip.offset.width, (GLint)mip.offset.height, (GLint)mip.offset.depth, (GLsizei)mip.extent.width, (GLsizei)mip.extent.height, (GLsizei)mip.extent.depth, format, type, mip_data);
			break;
		default: break;
		}
			offset += rtgl_execution_texture_range_bytes(image, mip);
		}
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	});
}

void rtgl_execution_texture_read(struct rtgl_context* ctx, struct rtgl_image_base* image, rt_texture_range range, u08* data, usize data_size) {
	rtgl_execution_submit_sync(ctx, [image, range, data, data_size](struct rtgl_context*) {
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		usize offset = 0;
		for (usize level = 0; level < range.mip_count; level++) {
			const rt_texture_range mip = rtgl_execution_texture_mip_range(range, level);
			GLint y = (GLint)mip.offset.height;
			GLint z = (GLint)mip.offset.depth;
			GLsizei width = (GLsizei)mip.extent.width;
			GLsizei height = (GLsizei)mip.extent.height;
			GLsizei depth = (GLsizei)mip.extent.depth;
			if (image->type == RT_TEXTURE_1D_ARRAY) { y = (GLint)mip.base_layer; height = (GLsizei)mip.layer_count; depth = 1; }
			if (image->type == RT_TEXTURE_2D_ARRAY) { z = (GLint)mip.base_layer; depth = (GLsizei)mip.layer_count; }
			if (image->type == RT_TEXTURE_1D) { height = 1; depth = 1; }
			if (image->type == RT_TEXTURE_2D) { depth = 1; }
			const usize mip_bytes = rtgl_execution_texture_range_bytes(image, mip);
			glGetTextureSubImage(image->gl_texture, (GLint)mip.base_mip, (GLint)mip.offset.width, y, z, width, height, depth, rtgl_texture_upload_format_aspect(image->format, mip.aspects), rtgl_texture_upload_type_aspect(image->format, mip.aspects), (GLsizei)(data_size - offset), data + offset);
			offset += mip_bytes;
		}
	});
}

struct gl_surface* rtgl_execution_glfw_surface_create(struct rtgl_context* ctx, struct GLFWwindow* window) {
	struct gl_surface* surface = NULL;
	rtgl_execution_submit_sync(ctx, [window, &surface](struct rtgl_context* exec_ctx) {
		surface = rtgl_create_glfw_surface(exec_ctx->execution.gl_context, window);
	});
	return surface;
}

void rtgl_execution_surface_destroy(struct rtgl_context* ctx, struct gl_surface* surface) {
	rtgl_execution_submit_sync(ctx, [surface](struct rtgl_context*) {
		if (surface) {
			rtgl_destroy_glsurface(surface);
		}
	});
}

static void rtgl_execution_present_now(struct rtgl_context* ctx, struct rtgl_queue* queue, struct rtgl_swapchain* swapchain, struct rtgl_framebuffer* framebuffer, u64 value) {
	struct rtgl_texture_view* view = framebuffer->color_views[0];
	u32 width = view && view->image ? view->image->width : 0;
	u32 height = view && view->image ? view->image->height : 0;

	rtgl_make_glcontext_current(ctx->execution.gl_context, swapchain->surface);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_FRAMEBUFFER_SRGB);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer->gl_framebuffer);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	glDrawBuffer(GL_BACK);
	glBlitFramebuffer(0, 0, (GLint)width, (GLint)height, 0, 0, (GLint)width, (GLint)height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	rtgl_swap_glsurface_buffers(swapchain->surface);
	rtgl_make_glcontext_current(ctx->execution.gl_context, NULL);

	rtgl_execution_lock(ctx);
	rtgl_execution_queue_complete_locked(queue, value);
	rtgl_execution_unlock(ctx);
	rtgl_resource_release(RTGL_RESOURCE_BASE(framebuffer));
	rtgl_resource_release(RTGL_RESOURCE_BASE(swapchain));
}

rt_timepoint rtgl_execution_present(struct rtgl_context* ctx, struct rtgl_queue* queue, struct rtgl_swapchain* swapchain, struct rtgl_framebuffer* framebuffer) {
	rt_timepoint done = rtgl_queue_signal(queue);

	rtgl_retain_resource(swapchain);
	rtgl_retain_resource(framebuffer);
	if (!rtgl_execution_submit_async(ctx, [queue, swapchain, framebuffer, value = rtgl_timepoint_queue_value(done)](struct rtgl_context* exec_ctx) {
			rtgl_execution_present_now(exec_ctx, queue, swapchain, framebuffer, value);
		})) {
		rt_timepoint failed = { ((u64)queue->identifier << 56) | queue->submitted_value };
		rtgl_release_resource(framebuffer);
		rtgl_release_resource(swapchain);
		return failed;
	}
	return done;
}

void rtgl_execution_queue_complete(struct rtgl_context* ctx, struct rtgl_queue* queue, u64 value) {
	rtgl_execution_submit_sync(ctx, [queue, value](struct rtgl_context* exec_ctx) {
		rtgl_execution_lock(exec_ctx);
		rtgl_execution_queue_complete_locked(queue, value);
		rtgl_execution_unlock(exec_ctx);
	});
}
