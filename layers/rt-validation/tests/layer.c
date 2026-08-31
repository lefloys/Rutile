#define RT_TYPES_ONLY
#include "rutile.h"
#undef RT_TYPES_ONLY
#include "rt_swapchain.h"

#include <stdio.h>
#include <string.h>

extern void rtLayerSetNext(rt_proc_chain next);
extern void rtInit(const char* const* features, usize feature_count);
extern void rtExit(void);
extern void rtSetOutput(rt_output output, void* user_data);
extern enum rt_error rtError(void);
extern const char* rtErrorMessage(void);
extern void rtClearError(void);
extern void rtCommandBufferReset(rt_command_buffer command_buffer);
extern rt_command_buffer rtCommandBufferCreate(void);
extern void rtCommandBufferBegin(rt_command_buffer command_buffer);
extern void rtSamplerSetLod(rt_sampler sampler, f32 min_lod, f32 max_lod, f32 lod_bias);
extern rt_sampler rtSamplerCreate(void);
extern void rtSamplerSetFilter(rt_sampler sampler, enum rt_filter mag, enum rt_filter min, enum rt_mip_filter mip);
extern rt_buffer rtBufferCreate(void);
extern void rtBufferDestroy(rt_buffer buffer);
extern void rtBufferResize(rt_buffer buffer, enum rt_memory_type memory_type, usize size);
extern u08* rtBufferMap(rt_buffer buffer, rt_buffer_range range);
extern void rtBufferUnmap(rt_buffer buffer);
extern rt_queue rtQueueCreate(enum rt_queue_capability capability);
extern void rtCmdDraw(rt_command_buffer command_buffer, usize vertex_count, usize first_vertex);
extern void rtCmdBufferBarrier(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, rt_access src, rt_access dst);
extern rt_program rtProgramCreate(void);
extern void rtProgramSetRasterState(rt_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode);
extern rt_location rtProgramOutputLocation(rt_program program, const char* name);
extern rt_texture rtTextureCreate(void);
extern void rtTextureResize(rt_texture texture, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count);
extern rt_swapchain rtSwapchainCreate(void);
extern rt_swapchain_acquire_result rtSwapchainAcquire(rt_swapchain swapchain);
extern void rtSwapchainResize(rt_swapchain swapchain, u32 width, u32 height);
extern void rtSwapchainDestroy(rt_swapchain swapchain);

static enum rt_error backend_error;
static unsigned backend_clear_count;
static char output[1024];
static unsigned sampler_filter_count;
static int sampler_backend;
static int command_buffer_backend;
static unsigned command_buffer_begin_count;
static bool command_buffer_begin_fails;
static int buffer_backend;
static u08 mapped_bytes[16];
static unsigned buffer_map_count;
static unsigned buffer_unmap_count;
static unsigned buffer_destroy_count;
static bool buffer_destroy_fails;
static unsigned buffer_barrier_count;
static int swapchain_backend;
static int swapchain_framebuffer_backend;
static unsigned swapchain_resize_count;
static unsigned swapchain_destroy_count;
static bool swapchain_return_null_framebuffer;
static unsigned exit_count;
static int program_backend;
static unsigned program_raster_state_count;
static unsigned program_output_location_count;
static int texture_backend;
static unsigned texture_resize_count;

static enum rt_error fake_rtError(void) { return backend_error; }
static const char* fake_rtErrorMessage(void) { return "backend error"; }
static void fake_rtClearError(void) { backend_error = RT_SUCCESS; backend_clear_count++; }
static void fake_rtExit(void) { exit_count++; }
static void fake_rtSetOutput(rt_output callback, void* user_data) { (void)callback; (void)user_data; }
static rt_sampler fake_rtSamplerCreate(void) { return (rt_sampler)&sampler_backend; }
static void fake_rtSamplerSetFilter(rt_sampler sampler, enum rt_filter mag, enum rt_filter min, enum rt_mip_filter mip) { (void)sampler; (void)mag; (void)min; (void)mip; sampler_filter_count++; }
static rt_command_buffer fake_rtCommandBufferCreate(void) { return (rt_command_buffer)&command_buffer_backend; }
static void fake_rtCommandBufferBegin(rt_command_buffer command_buffer) {
	(void)command_buffer;
	command_buffer_begin_count++;
	if (command_buffer_begin_fails) {
		backend_error = RT_IMPROPER_USAGE;
	}
}
static rt_buffer fake_rtBufferCreate(void) { return (rt_buffer)&buffer_backend; }
static void fake_rtBufferDestroy(rt_buffer buffer) {
	(void)buffer;
	buffer_destroy_count++;
	if (buffer_destroy_fails) {
		backend_error = RT_IMPROPER_USAGE;
	}
}
static void fake_rtBufferResize(rt_buffer buffer, enum rt_memory_type memory_type, usize size) { (void)buffer; (void)memory_type; (void)size; }
static u08* fake_rtBufferMap(rt_buffer buffer, rt_buffer_range range) { (void)buffer; (void)range; buffer_map_count++; return mapped_bytes; }
static void fake_rtBufferUnmap(rt_buffer buffer) { (void)buffer; buffer_unmap_count++; }
static void fake_rtCmdBufferBarrier(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, rt_access src, rt_access dst) { (void)command_buffer; (void)buffer; (void)range; (void)src; (void)dst; buffer_barrier_count++; }
static rt_program fake_rtProgramCreate(void) { return (rt_program)&program_backend; }
static void fake_rtProgramSetRasterState(rt_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode) { (void)program; (void)cull_mode; (void)front_face; (void)fill_mode; program_raster_state_count++; }
static rt_location fake_rtProgramOutputLocation(rt_program program, const char* name) { (void)program; (void)name; program_output_location_count++; return NULL; }
static rt_texture fake_rtTextureCreate(void) { return (rt_texture)&texture_backend; }
static void fake_rtTextureResize(rt_texture texture, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count) { (void)texture; (void)type; (void)format; (void)extent; (void)mip_count; texture_resize_count++; }
static rt_swapchain fake_rtSwapchainCreate(void) { return (rt_swapchain)&swapchain_backend; }
static rt_swapchain_acquire_result fake_rtSwapchainAcquire(rt_swapchain swapchain) { (void)swapchain; return (rt_swapchain_acquire_result){ swapchain_return_null_framebuffer ? NULL : (rt_framebuffer)&swapchain_framebuffer_backend, { 1 } }; }
static void fake_rtSwapchainResize(rt_swapchain swapchain, u32 width, u32 height) { (void)swapchain; (void)width; (void)height; swapchain_resize_count++; }
static void fake_rtSwapchainDestroy(rt_swapchain swapchain) { (void)swapchain; swapchain_destroy_count++; }

static rt_proc_t fake_get_proc(const rt_proc_chain* chain, const char* name) {
	(void)chain;
	if (strcmp(name, "rtError") == 0) { return (rt_proc_t)fake_rtError; }
	if (strcmp(name, "rtErrorMessage") == 0) { return (rt_proc_t)fake_rtErrorMessage; }
	if (strcmp(name, "rtClearError") == 0) { return (rt_proc_t)fake_rtClearError; }
	if (strcmp(name, "rtExit") == 0) { return (rt_proc_t)fake_rtExit; }
	if (strcmp(name, "rtSetOutput") == 0) { return (rt_proc_t)fake_rtSetOutput; }
	if (strcmp(name, "rtSamplerCreate") == 0) { return (rt_proc_t)fake_rtSamplerCreate; }
	if (strcmp(name, "rtSamplerSetFilter") == 0) { return (rt_proc_t)fake_rtSamplerSetFilter; }
	if (strcmp(name, "rtCommandBufferCreate") == 0) { return (rt_proc_t)fake_rtCommandBufferCreate; }
	if (strcmp(name, "rtCommandBufferBegin") == 0) { return (rt_proc_t)fake_rtCommandBufferBegin; }
	if (strcmp(name, "rtBufferCreate") == 0) { return (rt_proc_t)fake_rtBufferCreate; }
	if (strcmp(name, "rtBufferDestroy") == 0) { return (rt_proc_t)fake_rtBufferDestroy; }
	if (strcmp(name, "rtBufferResize") == 0) { return (rt_proc_t)fake_rtBufferResize; }
	if (strcmp(name, "rtBufferMap") == 0) { return (rt_proc_t)fake_rtBufferMap; }
	if (strcmp(name, "rtBufferUnmap") == 0) { return (rt_proc_t)fake_rtBufferUnmap; }
	if (strcmp(name, "rtCmdBufferBarrier") == 0) { return (rt_proc_t)fake_rtCmdBufferBarrier; }
	if (strcmp(name, "rtProgramCreate") == 0) { return (rt_proc_t)fake_rtProgramCreate; }
	if (strcmp(name, "rtProgramSetRasterState") == 0) { return (rt_proc_t)fake_rtProgramSetRasterState; }
	if (strcmp(name, "rtProgramOutputLocation") == 0) { return (rt_proc_t)fake_rtProgramOutputLocation; }
	if (strcmp(name, "rtTextureCreate") == 0) { return (rt_proc_t)fake_rtTextureCreate; }
	if (strcmp(name, "rtTextureResize") == 0) { return (rt_proc_t)fake_rtTextureResize; }
	if (strcmp(name, "rtSwapchainCreate") == 0) { return (rt_proc_t)fake_rtSwapchainCreate; }
	if (strcmp(name, "rtSwapchainAcquire") == 0) { return (rt_proc_t)fake_rtSwapchainAcquire; }
	if (strcmp(name, "rtSwapchainResize") == 0) { return (rt_proc_t)fake_rtSwapchainResize; }
	if (strcmp(name, "rtSwapchainDestroy") == 0) { return (rt_proc_t)fake_rtSwapchainDestroy; }
	return NULL;
}

static void capture(const char* message, void* user_data) {
	(void)user_data;
	snprintf(output, sizeof(output), "%s", message ? message : "");
}

int main(void) {
	rt_proc_chain chain = { fake_get_proc };
	rtLayerSetNext(chain);
	rtSetOutput(capture, NULL);
	rtInit(NULL, 1);
	if (rtError() != RT_IMPROPER_USAGE || strstr(rtErrorMessage(), "feature array") == NULL) {
		return 1;
	}
	rtClearError();
	rtCommandBufferReset(NULL);
	if (rtError() != RT_IMPROPER_USAGE || strstr(rtErrorMessage(), "rtCommandBufferReset") == NULL || strstr(output, "dropping call") == NULL) {
		return 2;
	}
	rtClearError();
	if (rtError() != RT_SUCCESS || backend_clear_count != 2) {
		return 3;
	}
	rtSamplerSetLod(NULL, 1.0f, 0.0f, 0.0f);
	if (rtError() != RT_IMPROPER_USAGE || strstr(rtErrorMessage(), "rtSamplerSetLod") == NULL) {
		return 4;
	}
	rtClearError();
	rt_sampler sampler = rtSamplerCreate();
	rtSamplerSetFilter(sampler, (enum rt_filter)99, RT_FILTER_LINEAR, RT_MIP_FILTER_LINEAR);
	if (rtError() != RT_IMPROPER_USAGE || sampler_filter_count != 0) {
		return 5;
	}
	rtClearError();
	rtSamplerSetFilter(sampler, RT_FILTER_NEAREST, RT_FILTER_LINEAR, RT_MIP_FILTER_LINEAR);
	if (rtError() != RT_SUCCESS || sampler_filter_count != 1) {
		return 6;
	}
	backend_error = RT_IMPROPER_USAGE;
	rtSamplerSetFilter(sampler, RT_FILTER_NEAREST, RT_FILTER_LINEAR, RT_MIP_FILTER_LINEAR);
	if (strstr(output, "rtSamplerSetFilter failed") == NULL) {
		return 7;
	}
	rtClearError();
	rt_command_buffer command_buffer = rtCommandBufferCreate();
	command_buffer_begin_fails = true;
	rtCommandBufferBegin(command_buffer);
	if (rtError() != RT_IMPROPER_USAGE || command_buffer_begin_count != 1) {
		return 8;
	}
	rtClearError();
	command_buffer_begin_fails = false;
	rtCommandBufferBegin(command_buffer);
	if (rtError() != RT_SUCCESS || command_buffer_begin_count != 2) {
		return 9;
	}
	rt_buffer buffer = rtBufferCreate();
	rtBufferResize(buffer, RT_HOST_MEMORY, sizeof(mapped_bytes));
	if (rtBufferMap(buffer, (rt_buffer_range){ 0, 4 }) != mapped_bytes || buffer_map_count != 1) {
		return 10;
	}
	rtBufferMap(buffer, (rt_buffer_range){ 0, 4 });
	if (rtError() != RT_IMPROPER_USAGE || buffer_map_count != 1) {
		return 11;
	}
	rtClearError();
	rtBufferUnmap(buffer);
	if (rtError() != RT_SUCCESS || buffer_unmap_count != 1) {
		return 12;
	}
	rtQueueCreate((enum rt_queue_capability)99);
	if (rtError() != RT_IMPROPER_USAGE) {
		return 13;
	}
	rtClearError();
	rtCmdDraw(command_buffer, 3, 0);
	if (rtError() != RT_IMPROPER_USAGE) {
		return 14;
	}
	rtClearError();
	buffer_destroy_fails = true;
	rtBufferDestroy(buffer);
	if (rtError() != RT_IMPROPER_USAGE || buffer_destroy_count != 1) {
		return 15;
	}
	rtClearError();
	rtBufferResize(buffer, RT_HOST_MEMORY, 4);
	if (rtError() != RT_SUCCESS) {
		return 16;
	}
	buffer_destroy_fails = false;
	rtBufferDestroy(buffer);
	if (rtError() != RT_SUCCESS || buffer_destroy_count != 2) {
		return 17;
	}
	rt_buffer replacement = rtBufferCreate();
	rtBufferResize(buffer, RT_HOST_MEMORY, 4);
	if (rtError() != RT_IMPROPER_USAGE || buffer_destroy_count != 2) {
		return 18;
	}
	rtClearError();
	rtBufferResize(replacement, RT_HOST_MEMORY, 4);
	if (rtError() != RT_SUCCESS) {
		return 19;
	}
	rtCmdBufferBarrier(command_buffer, replacement, (rt_buffer_range){ 0, 4 }, (rt_access){ (enum rt_stage_flag)0x80, RT_ACCESS_WRITE }, (rt_access){ RT_STAGE_TRANSFER, RT_ACCESS_READ });
	if (rtError() != RT_IMPROPER_USAGE || buffer_barrier_count != 0) {
		return 17;
	}
	rtClearError();
	rtCmdBufferBarrier(command_buffer, replacement, (rt_buffer_range){ 0, 4 }, (rt_access){ RT_STAGE_TRANSFER, RT_ACCESS_WRITE }, (rt_access){ RT_STAGE_FRAGMENT, RT_ACCESS_READ });
	if (rtError() != RT_SUCCESS || buffer_barrier_count != 1) {
		return 18;
	}
	rt_program program = rtProgramCreate();
	rtProgramSetRasterState(program, (enum rt_cull_mode)99, RT_FRONT_FACE_CCW, RT_FILL_SOLID);
	if (rtError() != RT_IMPROPER_USAGE || program_raster_state_count != 0) {
		return 19;
	}
	rtClearError();
	rtProgramSetRasterState(program, RT_CULL_BACK, RT_FRONT_FACE_CCW, RT_FILL_SOLID);
	if (rtError() != RT_SUCCESS || program_raster_state_count != 1) {
		return 20;
	}
	if (rtProgramOutputLocation(program, NULL) != NULL || rtError() != RT_SUCCESS || program_output_location_count != 1) {
		return 21;
	}
	rt_texture texture = rtTextureCreate();
	rtTextureResize(texture, RT_TEXTURE_UNKNOWN, RT_RGBA8_UNORM, (rt_extent_3d){ 1, 1, 1 }, 1);
	if (rtError() != RT_IMPROPER_USAGE || texture_resize_count != 0) {
		return 22;
	}
	rtClearError();
	rtTextureResize(texture, RT_TEXTURE_2D, RT_RGBA8_UNORM, (rt_extent_3d){ 1, 1, 1 }, 1);
	if (rtError() != RT_SUCCESS || texture_resize_count != 1) {
		return 23;
	}
	rt_swapchain swapchain = rtSwapchainCreate();
	swapchain_return_null_framebuffer = true;
	rt_swapchain_acquire_result unavailable = rtSwapchainAcquire(swapchain);
	if (unavailable.framebuffer != NULL || rtError() != RT_SUCCESS) {
		return 24;
	}
	swapchain_return_null_framebuffer = false;
	rt_swapchain_acquire_result acquired = rtSwapchainAcquire(swapchain);
	if (!acquired.framebuffer || acquired.timepoint.value != 1) {
		return 25;
	}
	rtSwapchainResize(swapchain, 1, 1);
	if (rtError() != RT_IMPROPER_USAGE || swapchain_resize_count != 0) {
		return 26;
	}
	rtClearError();
	rtSwapchainDestroy(swapchain);
	if (rtError() != RT_IMPROPER_USAGE || swapchain_destroy_count != 0) {
		return 27;
	}
	rtClearError();
	rtExit();
	if (rtError() != RT_IMPROPER_USAGE || exit_count != 1 || strstr(rtErrorMessage(), "resources must be destroyed") == NULL) {
		return 28;
	}
	return 0;
}
