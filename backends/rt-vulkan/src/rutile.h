#ifndef RTVK_INTERNAL_RUTILE_H
#define RTVK_INTERNAL_RUTILE_H

/* Backend ABI declarations. This is deliberately not the public Rutile header. */

#include <stddef.h>
#include <stdint.h>

#if !defined(__cplusplus)
#include <stdbool.h>
#endif

#define RT_NULL_HANDLE NULL
#define RT_FEATURE_PRESENTATION "RT_FEATURE_PRESENTATION"

typedef uint8_t u08;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;
typedef uintptr_t uptr;
typedef int8_t i08;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;

enum rt_error {
	RT_SUCCESS = 0,
	RT_OUT_OF_HOST_MEMORY,
	RT_OUT_OF_DEVICE_MEMORY,
	RT_IMPROPER_USAGE,
	RT_PLATFORM_FAILURE,
	RT_DEVICE_LOST,
	RT_ALREADY_INITIALIZED,
	RT_UNSUPPORTED_PLATFORM,
	RT_NO_BACKEND,
	RT_UNSUPPORTED_FEATURE,
	RT_INITIALIZATION_FAILED,
	RT_LAYER_NOT_PRESENT,
	RT_EXTENSION_NOT_PRESENT,
	RT_INCOMPATIBLE_DRIVER,
	RT_SHADER_COMPILATION_FAILED,
	RT_SHADER_LINK_FAILED,
	RT_FEATURE_NOT_SUPPORTED,
};

enum rt_format {
	RT_FORMAT_UNKNOWN = 0,
	RT_R8_UNORM,
	RT_RG8_UNORM,
	RT_RGB8_UNORM,
	RT_RGBA8_UNORM,
	RT_R16_UNORM,
	RT_RG16_UNORM,
	RT_RGB16_UNORM,
	RT_RGBA16_UNORM,
	RT_R16_SFLOAT,
	RT_RG16_SFLOAT,
	RT_RGB16_SFLOAT,
	RT_RGBA16_SFLOAT,
	RT_R32_SFLOAT,
	RT_RG32_SFLOAT,
	RT_RGB32_SFLOAT,
	RT_RGBA32_SFLOAT,
	RT_R8_SINT,
	RT_RG8_SINT,
	RT_RGB8_SINT,
	RT_RGBA8_SINT,
	RT_R16_SINT,
	RT_RG16_SINT,
	RT_RGB16_SINT,
	RT_RGBA16_SINT,
	RT_R32_SINT,
	RT_RG32_SINT,
	RT_RGB32_SINT,
	RT_RGBA32_SINT,
	RT_R8_UINT,
	RT_RG8_UINT,
	RT_RGB8_UINT,
	RT_RGBA8_UINT,
	RT_R16_UINT,
	RT_RG16_UINT,
	RT_RGB16_UINT,
	RT_RGBA16_UINT,
	RT_R32_UINT,
	RT_RG32_UINT,
	RT_RGB32_UINT,
	RT_RGBA32_UINT,
	RT_D16_UNORM,
	RT_D32_SFLOAT,
	RT_S8_UINT,
	RT_D24_UNORM_S8_UINT,
	RT_D32_SFLOAT_S8_UINT,
};

enum rt_format_usage {
	RT_FORMAT_USAGE_NONE = 0x00,
	RT_FORMAT_USAGE_SAMPLED = 0x01,
	RT_FORMAT_USAGE_COLOR_ATTACHMENT = 0x02,
	RT_FORMAT_USAGE_DEPTH_ATTACHMENT = 0x04,
	RT_FORMAT_USAGE_STORAGE = 0x08,
	RT_FORMAT_USAGE_TRANSFER_SRC = 0x10,
	RT_FORMAT_USAGE_TRANSFER_DST = 0x20,
};

enum rt_memory_type {
	RT_HOST_MEMORY = 1,
	RT_DEVICE_MEMORY,
};

enum rt_clear_flag {
	RT_CLEAR_NONE = 0x00,
	RT_CLEAR_COLOR = 0x01,
	RT_CLEAR_DEPTH = 0x02,
	RT_CLEAR_STENCIL = 0x04,
};

enum rt_stage_flag {
	RT_STAGE_NONE = 0x00,
	RT_STAGE_TRANSFER = 0x01,
	RT_STAGE_VERTEX = 0x02,
	RT_STAGE_FRAGMENT = 0x04,
	RT_STAGE_COMPUTE = 0x08,
	RT_STAGE_COLOR_ATTACHMENT = 0x10,
	RT_STAGE_DEPTH_STENCIL_ATTACHMENT = 0x20,
	RT_STAGE_ALL = 0x3f,
};

enum rt_access_type {
	RT_ACCESS_NONE = 0,
	RT_ACCESS_READ,
	RT_ACCESS_WRITE,
};

enum rt_texture_type {
	RT_TEXTURE_UNKNOWN = 0,
	RT_TEXTURE_1D,
	RT_TEXTURE_2D,
	RT_TEXTURE_3D,
	RT_TEXTURE_1D_ARRAY,
	RT_TEXTURE_2D_ARRAY,
};

enum rt_texture_aspect_flag {
	RT_TEXTURE_ASPECT_NONE = 0x00,
	RT_TEXTURE_ASPECT_COLOR = 0x01,
	RT_TEXTURE_ASPECT_DEPTH = 0x02,
	RT_TEXTURE_ASPECT_STENCIL = 0x04,
};

enum rt_filter {
	RT_FILTER_NEAREST = 1,
	RT_FILTER_LINEAR,
};

enum rt_mip_filter {
	RT_MIP_FILTER_NONE = 0,
	RT_MIP_FILTER_NEAREST,
	RT_MIP_FILTER_LINEAR,
};

enum rt_address_mode {
	RT_ADDRESS_CLAMP = 1,
	RT_ADDRESS_REPEAT,
	RT_ADDRESS_MIRROR,
};

enum rt_queue_capability {
	RT_QUEUE_TRANSFER = 1,
	RT_QUEUE_COMPUTE,
	RT_QUEUE_GRAPHICS,
};

enum rt_index_format {
	RT_INDEX_U16 = 1,
	RT_INDEX_U32,
};

enum rt_vertex_rate {
	RT_VERTEX_RATE_VERTEX = 0,
	RT_VERTEX_RATE_INSTANCE,
};

enum rt_cull_mode {
	RT_CULL_NONE = 0,
	RT_CULL_FRONT,
	RT_CULL_BACK,
};

enum rt_front_face {
	RT_FRONT_FACE_CCW = 0,
	RT_FRONT_FACE_CW,
};

enum rt_fill_mode {
	RT_FILL_SOLID = 0,
	RT_FILL_WIREFRAME,
};

enum rt_compare_op {
	RT_COMPARE_NEVER = 0,
	RT_COMPARE_LESS,
	RT_COMPARE_EQUAL,
	RT_COMPARE_LESS_EQUAL,
	RT_COMPARE_GREATER,
	RT_COMPARE_NOT_EQUAL,
	RT_COMPARE_GREATER_EQUAL,
	RT_COMPARE_ALWAYS,
};

enum rt_blend_factor {
	RT_BLEND_ZERO = 0,
	RT_BLEND_ONE,
	RT_BLEND_SRC_COLOR,
	RT_BLEND_ONE_MINUS_SRC_COLOR,
	RT_BLEND_DST_COLOR,
	RT_BLEND_ONE_MINUS_DST_COLOR,
	RT_BLEND_SRC_ALPHA,
	RT_BLEND_ONE_MINUS_SRC_ALPHA,
	RT_BLEND_DST_ALPHA,
	RT_BLEND_ONE_MINUS_DST_ALPHA,
};

enum rt_blend_op {
	RT_BLEND_OP_ADD = 0,
	RT_BLEND_OP_SUBTRACT,
	RT_BLEND_OP_REVERSE_SUBTRACT,
	RT_BLEND_OP_MIN,
	RT_BLEND_OP_MAX,
};

typedef struct rt_command_buffer_t* rt_command_buffer;
typedef struct rt_queue_t* rt_queue;
typedef struct rt_framebuffer_t* rt_framebuffer;
typedef struct rt_program_t* rt_program;
typedef struct rt_buffer_t* rt_buffer;
typedef struct rt_texture_t* rt_texture;
typedef struct rt_texture_view_t* rt_texture_view;
typedef struct rt_sampler_t* rt_sampler;
typedef struct rt_location_t* rt_location;
typedef struct rt_swapchain_t* rt_swapchain;

typedef struct rt_vertex_attribute {
	const char* name;
	usize offset;
	enum rt_format format;
} rt_vertex_attribute;

typedef struct rt_vertex_input {
	const rt_vertex_attribute* attributes;
	usize attribute_count;
	usize stride;
	enum rt_vertex_rate rate;
} rt_vertex_input;

typedef struct rt_vertex_layout {
	const rt_vertex_input* inputs;
	usize input_count;
} rt_vertex_layout;

typedef struct rt_timepoint {
	u64 value;
} rt_timepoint;

typedef struct rt_extent_3d {
	usize width;
	usize height;
	usize depth;
} rt_extent_3d;

typedef struct rt_buffer_range {
	usize size;
	usize offset;
} rt_buffer_range;

typedef struct rt_texture_range {
	enum rt_texture_aspect_flag aspects;
	usize base_mip;
	usize mip_count;
	usize base_layer;
	usize layer_count;
	rt_extent_3d extent;
	rt_extent_3d offset;
} rt_texture_range;

typedef struct rt_access {
	enum rt_stage_flag stage;
	enum rt_access_type type;
} rt_access;

typedef struct rt_swapchain_acquire_result {
	rt_framebuffer framebuffer;
	rt_timepoint timepoint;
} rt_swapchain_acquire_result;

typedef void (*rt_output)(const char* message, void* user_data);

#endif /* RTVK_INTERNAL_RUTILE_H */
