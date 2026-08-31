#ifndef RTDBG_INTERNAL_RUTILE_H
#define RTDBG_INTERNAL_RUTILE_H

#include <stddef.h>
#include <stdint.h>
#if !defined(__cplusplus)
#include <stdbool.h>
#endif

#define RT_NULL_HANDLE NULL

#ifdef __cplusplus
extern "C" {
#endif

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

/*!
** @brief Pack four 16-bit version fields into one @ref u64.
**
** The fields are stored in major, minor, patch, snapshot order from most to
** least significant. Values larger than 16 bits are truncated.
*/
#define RT_MAKE_VERSION(major, minor, patch, snapshot) \
	((((u64)(major) & 0xffffu) << 48u) | (((u64)(minor) & 0xffffu) << 32u) | (((u64)(patch) & 0xffffu) << 16u) | ((u64)(snapshot) & 0xffffu))

/*! @brief Read the major field from a packed version. */
#define RT_VERSION_GET_MAJOR(version) ((u16)(((u64)(version) >> 48u) & 0xffffu))
/*! @brief Read the minor field from a packed version. */
#define RT_VERSION_GET_MINOR(version) ((u16)(((u64)(version) >> 32u) & 0xffffu))
/*! @brief Read the patch field from a packed version. */
#define RT_VERSION_GET_PATCH(version) ((u16)(((u64)(version) >> 16u) & 0xffffu))
/*! @brief Read the snapshot field from a packed version. */
#define RT_VERSION_GET_SNAPSHOT(version) ((u16)((u64)(version) & 0xffffu))

/*! @brief Version of the API contract declared by this header. */
#define RT_HEADER_VERSION RT_MAKE_VERSION(0, 1, 0, 0)

/*!
** @brief Error reported to the calling thread.
**
** @ref rtLoad and @ref rtLoadDevelopment return an error directly. Other
** calls report errors through @ref rtError and @ref rtErrorMessage.
*/
enum rt_error {
	RT_SUCCESS = 0,
	RT_OUT_OF_HOST_MEMORY = 1,
	RT_OUT_OF_DEVICE_MEMORY = 2,
	RT_IMPROPER_USAGE = 3,
	RT_PLATFORM_FAILURE = 4,
	RT_DEVICE_LOST = 5,
	RT_ALREADY_INITIALIZED = 6,
	RT_UNSUPPORTED_PLATFORM = 7,
	RT_NO_BACKEND = 8,
	RT_UNSUPPORTED_FEATURE = 9,
	RT_INITIALIZATION_FAILED = 10,
	RT_LAYER_NOT_PRESENT = 11,
	RT_EXTENSION_NOT_PRESENT = 12,
	RT_INCOMPATIBLE_DRIVER = 13,
	RT_SHADER_COMPILATION_FAILED = 14,
	RT_SHADER_LINK_FAILED = 15,
	RT_FEATURE_NOT_SUPPORTED = 16,
};

enum rt_format {
	RT_FORMAT_UNKNOWN = 0,
	RT_R8_UNORM = 1,
	RT_RG8_UNORM = 2,
	RT_RGB8_UNORM = 3,
	RT_RGBA8_UNORM = 4,
	RT_R16_UNORM = 5,
	RT_RG16_UNORM = 6,
	RT_RGB16_UNORM = 7,
	RT_RGBA16_UNORM = 8,
	RT_R16_SFLOAT = 9,
	RT_RG16_SFLOAT = 10,
	RT_RGB16_SFLOAT = 11,
	RT_RGBA16_SFLOAT = 12,
	RT_R32_SFLOAT = 13,
	RT_RG32_SFLOAT = 14,
	RT_RGB32_SFLOAT = 15,
	RT_RGBA32_SFLOAT = 16,
	RT_R8_SINT = 17,
	RT_RG8_SINT = 18,
	RT_RGB8_SINT = 19,
	RT_RGBA8_SINT = 20,
	RT_R16_SINT = 21,
	RT_RG16_SINT = 22,
	RT_RGB16_SINT = 23,
	RT_RGBA16_SINT = 24,
	RT_R32_SINT = 25,
	RT_RG32_SINT = 26,
	RT_RGB32_SINT = 27,
	RT_RGBA32_SINT = 28,
	RT_R8_UINT = 29,
	RT_RG8_UINT = 30,
	RT_RGB8_UINT = 31,
	RT_RGBA8_UINT = 32,
	RT_R16_UINT = 33,
	RT_RG16_UINT = 34,
	RT_RGB16_UINT = 35,
	RT_RGBA16_UINT = 36,
	RT_R32_UINT = 37,
	RT_RG32_UINT = 38,
	RT_RGB32_UINT = 39,
	RT_RGBA32_UINT = 40,
	RT_D16_UNORM = 41,
	RT_D32_SFLOAT = 42,
	RT_S8_UINT = 43,
	RT_D24_UNORM_S8_UINT = 44,
	RT_D32_SFLOAT_S8_UINT = 45,
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

/*!
** @brief Buffer access class selected by @ref rtBufferResize.
**
** @ref RT_HOST_MEMORY permits direct access through @ref rtBufferMap.
** @ref RT_DEVICE_MEMORY permits access through recorded commands only.
*/
enum rt_memory_type {
	RT_HOST_MEMORY = 1,
	RT_DEVICE_MEMORY = 2,
};

/*! @brief Attachment classes selected by @ref rtCmdClear. */
enum rt_clear_flag {
	RT_CLEAR_NONE = 0x00,
	RT_CLEAR_COLOR = 0x01,
	RT_CLEAR_DEPTH = 0x02,
	RT_CLEAR_STENCIL = 0x04,
};

/*!
** @brief Stages selected by program source and resource barriers.
**
** Combine stage flags with bitwise OR.
*/
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

/*! @brief No pipeline stage. */

/*! @brief Buffer and texture uploads, copies, and readback transfers. */

/*! @brief Vertex-buffer fetch and vertex-shader resource accesses. */

/*! @brief Fragment-shader resource accesses. */

/*! @brief Compute-shader resource accesses. */

/*! @brief Color-attachment reads, writes, and clears in rendering commands. */

/*! @brief Depth/stencil-attachment reads, writes, and clears in rendering commands. */

/*! @brief Every Rutile pipeline stage. */

/*!
** @brief Access mode used by @ref rt_access in resource barriers.
*/
enum rt_access_type {
	RT_ACCESS_NONE = 0,
	RT_ACCESS_READ = 1,
	RT_ACCESS_WRITE = 2,
};

/*! @brief No resource access. */

/*! @brief Resource reads that consume previously written contents. */

/*! @brief Resource writes that produce contents for later accesses. */

enum rt_texture_type {
	RT_TEXTURE_UNKNOWN = 0,
	RT_TEXTURE_1D = 1,
	RT_TEXTURE_2D = 2,
	RT_TEXTURE_3D = 3,
	RT_TEXTURE_1D_ARRAY = 4,
	RT_TEXTURE_2D_ARRAY = 5,
};

enum rt_texture_aspect_flag {
	RT_TEXTURE_ASPECT_NONE = 0x00,
	RT_TEXTURE_ASPECT_COLOR = 0x01,
	RT_TEXTURE_ASPECT_DEPTH = 0x02,
	RT_TEXTURE_ASPECT_STENCIL = 0x04,
};

enum rt_filter {
	RT_FILTER_NEAREST = 1,
	RT_FILTER_LINEAR = 2,
};

enum rt_mip_filter {
	RT_MIP_FILTER_NONE = 0,
	RT_MIP_FILTER_NEAREST = 1,
	RT_MIP_FILTER_LINEAR = 2,
};

enum rt_address_mode {
	RT_ADDRESS_CLAMP = 1,
	RT_ADDRESS_REPEAT = 2,
	RT_ADDRESS_MIRROR = 3,
};

enum rt_queue_capability {
	RT_QUEUE_TRANSFER = 1,
	RT_QUEUE_COMPUTE = 2,
	RT_QUEUE_GRAPHICS = 3,
};

enum rt_index_format {
	RT_INDEX_U16 = 1,
	RT_INDEX_U32 = 2,
};

enum rt_vertex_rate {
	RT_VERTEX_RATE_VERTEX = 0,
	RT_VERTEX_RATE_INSTANCE = 1,
};

enum rt_cull_mode {
	RT_CULL_NONE = 0,
	RT_CULL_FRONT = 1,
	RT_CULL_BACK = 2,
};

enum rt_front_face {
	RT_FRONT_FACE_CCW = 0,
	RT_FRONT_FACE_CW = 1,
};

enum rt_fill_mode {
	RT_FILL_SOLID = 0,
	RT_FILL_WIREFRAME = 1,
};

enum rt_compare_op {
	RT_COMPARE_NEVER = 0,
	RT_COMPARE_LESS = 1,
	RT_COMPARE_EQUAL = 2,
	RT_COMPARE_LESS_EQUAL = 3,
	RT_COMPARE_GREATER = 4,
	RT_COMPARE_NOT_EQUAL = 5,
	RT_COMPARE_GREATER_EQUAL = 6,
	RT_COMPARE_ALWAYS = 7,
};

enum rt_blend_factor {
	RT_BLEND_ZERO = 0,
	RT_BLEND_ONE = 1,
	RT_BLEND_SRC_COLOR = 2,
	RT_BLEND_ONE_MINUS_SRC_COLOR = 3,
	RT_BLEND_DST_COLOR = 4,
	RT_BLEND_ONE_MINUS_DST_COLOR = 5,
	RT_BLEND_SRC_ALPHA = 6,
	RT_BLEND_ONE_MINUS_SRC_ALPHA = 7,
	RT_BLEND_DST_ALPHA = 8,
	RT_BLEND_ONE_MINUS_DST_ALPHA = 9,
};

enum rt_blend_op {
	RT_BLEND_OP_ADD = 0,
	RT_BLEND_OP_SUBTRACT = 1,
	RT_BLEND_OP_REVERSE_SUBTRACT = 2,
	RT_BLEND_OP_MIN = 3,
	RT_BLEND_OP_MAX = 4,
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

/*! @brief One shader vertex attribute within an @ref rt_vertex_input. */
typedef struct rt_vertex_attribute {
	const char* name;
	usize offset;
	enum rt_format format;
} rt_vertex_attribute;

/*!
** @brief One vertex-buffer input and the shader attributes it supplies.
**
** Attributes in @p attributes are interleaved in one buffer. Query this
** input's location with @ref rtProgramInputLocation, passing the
** same attribute array and count, then pass that location to
** @ref rtCmdVertexBuffer.
*/
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

/*!
** @brief Opaque completion signal returned by queue submission.
**
** A zero-initialized timepoint is already reached. Pass a timepoint to
** @ref rtTimepointWait to wait on the CPU, @ref rtTimepointReached to poll,
** or @ref rtQueueWait to make a queue's next submission depend on it.
*/
typedef struct rt_timepoint {
	u64 value;
} rt_timepoint;

typedef struct rt_extent_3d {
	usize width;
	usize height;
	usize depth;
} rt_extent_3d;

/*! @brief Contiguous byte range within a buffer. */
typedef struct rt_buffer_range {
	usize size;
	usize offset;
} rt_buffer_range;

/*!
** @brief Texel area selected by mip, array layer, and aspect.
**
** A range addresses @p extent texels beginning at @p offset in the selected
** mip levels. @p base_mip and @p mip_count select those levels; @p base_layer
** and @p layer_count select array layers. For a non-array texture, use base
** layer 0 and layer count 1. Use @p aspects to select color, depth, stencil,
** or a depth/stencil combination.
*/
typedef struct rt_texture_range {
	enum rt_texture_aspect_flag aspects;
	usize base_mip;
	usize mip_count;
	usize base_layer;
	usize layer_count;
	rt_extent_3d extent;
	rt_extent_3d offset;
} rt_texture_range;

/*!
** @brief One side of a resource dependency.
**
** @p stage selects the participating stages and @p type selects whether they
** read or write the resource.
*/
typedef struct rt_access {
	enum rt_stage_flag stage;
	enum rt_access_type type;
} rt_access;

typedef void* rt_proc_t;

typedef struct rt_proc_chain {
	rt_proc_t (*get_proc)(const struct rt_proc_chain* chain, const char* name);
} rt_proc_chain;

/*! @brief Callback receiving a Rutile diagnostic message. */
typedef void (*rt_output)(const char* message, void* user_data);

#ifdef __cplusplus
}
#endif


#endif /* RTDBG_INTERNAL_RUTILE_H */

