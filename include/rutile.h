#ifndef RUTILE_H
#define RUTILE_H

/*!
** @file rutile.h
** @brief Rutile public C API and dynamic loader.
**
** Applications load Rutile, initialize the features they will use, create
** resources, record commands, and submit completed command buffers. @ref
** rtLoad resolves the minimal core and every built-in extension together.
** This header defines the behavior visible to the application.
**
*/

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

#if !defined(RT_TYPES_ONLY)

#define RT_API static inline

#ifdef __cplusplus
extern "C" {
#endif
/*!
** @brief Load Rutile and an optional ordered layer list.
**
** Success makes the core and every built-in extension callable and makes
** @ref rtLoaded return true. The first layer name is applied closest to the
** application. Only one load may be active at a time.
**
** @param backend_name  Backend name (e.g. `"rt-vulkan"`).
** @param layer_names   Optional array of layer names. The first entry receives
**                      application calls first; later entries apply in array
**                      order. May be NULL when @p layer_count is 0.
** @param layer_count   Number of entries in @p layer_names.
**
** @return RT_SUCCESS when Rutile is ready for @ref rtInit.
** @return RT_NO_BACKEND when @p backend_name cannot be loaded.
** @return RT_IMPROPER_USAGE for invalid arguments, an unavailable layer, or
**         an existing active load.
** @return RT_EXTENSION_NOT_PRESENT when a required procedure is unavailable.
**
** On failure, an existing active load is unchanged; otherwise Rutile remains
** unloaded.
*/
enum rt_error rtLoad(const char* backend_name, const char* const* layer_names, usize layer_count);

/*!
** @brief Attempt to load Rutile without requiring a backend.
**
** This has the same successful state as @ref rtLoad. If @p backend_name is
** NULL or unavailable, it returns RT_SUCCESS and leaves Rutile unloaded.
**
** @param backend_name  Backend name, or NULL to load no backend.
** @param layer_names   Optional array of layer names (see @ref rtLoad).
** @param layer_count   Number of entries in @p layer_names.
**
** @return RT_SUCCESS when Rutile is loaded or no backend is available.
** @return RT_IMPROPER_USAGE for invalid arguments or an unavailable layer.
*/
enum rt_error rtLoadDevelopment(const char* backend_name, const char* const* layer_names, usize layer_count);

/*!
** @brief End the active Rutile load.
**
** After this call, @ref rtLoaded returns false and loaded procedures are not
** callable until another successful load. Calling this while unloaded has no
** effect.
*/
void rtUnload(void);

/*!
** @brief Report whether Rutile procedures are callable.
**
** @return true after a successful load and before @ref rtUnload; otherwise
**         false.
*/
bool rtLoaded(void);

/*!
** @brief Resolve a named procedure through the active load.
**
** @param name  Null-terminated procedure name.
** @return Callable procedure pointer, including active layers, or NULL when
**         the name is unavailable.
*/
rt_proc_t rtGetProc(const char* name);

/*!
** @brief Initialize the features the application will use.
**
** Call once after loading and before using initialized API. A feature is
** available only when its name is included here. The currently defined
** feature is @ref RT_FEATURE_PRESENTATION.
**
** @param features       Array of feature name strings, or NULL when
**                       @p feature_count is 0.
** @param feature_count  Number of entries in @p features.
**
** @error RT_FEATURE_NOT_SUPPORTED  A requested feature is unavailable.
** @error RT_ALREADY_INITIALIZED    Rutile is already initialized.
** @error RT_INITIALIZATION_FAILED  Initialization failed.
*/
RT_API void rtInit(const char* const* features, usize feature_count);

/*!
** @brief Return Rutile to its loaded, uninitialized state.
**
** Destroy application-created resources before calling. The active load
** remains available, so @ref rtInit may be called again.
*/
RT_API void rtExit(void);

/*!
** @brief Return the Rutile version provided by the active load.
**
** @return Packed major, minor, patch, and snapshot fields. Use the
**         RT_VERSION_GET_* macros to read the four 16-bit fields.
*/
RT_API u64 rtVersion(void);

/*!
** @brief Select where Rutile diagnostic messages are sent.
**
** Future diagnostic messages are passed to @p output with @p user_data.
** Passing NULL restores the default output destination.
**
** @param output     Callback to receive messages, or NULL to restore the
**                   default.
** @param user_data  Value passed unchanged to @p output.
*/
RT_API void rtSetOutput(rt_output output, void* user_data);

/*!
** @brief Return the calling thread's current error code.
**
** @return The current error code, or RT_SUCCESS if none.
*/
RT_API enum rt_error rtError(void);

/*!
** @brief Return the calling thread's current error message.
**
** @return Null-terminated text valid until that thread records or clears an
**         error.
*/
RT_API const char* rtErrorMessage(void);

/*!
** @brief Clear the calling thread's error state.
**
** Afterwards @ref rtError returns RT_SUCCESS and @ref rtErrorMessage returns
** an empty string until another error is reported on the same thread.
*/
RT_API void rtClearError(void);

/*!
** @brief Return the name selected by the active load.
**
** @return Null-terminated name valid until @ref rtUnload.
*/
RT_API const char* rtGetName(void);

/*===============================================================================================*/
/* Command buffer                                                                                */
/*===============================================================================================*/

/*!
** @brief Create a command buffer.
**
** The new command buffer contains no commands and is ready for a begin call.
**
** @return New command buffer, or NULL on failure.
*/
RT_API rt_command_buffer rtCommandBufferCreate(void);

/*!
** @brief Destroy a command buffer.
**
** The handle is invalid after this call.
**
** @param command_buffer  Command buffer to destroy.
*/
RT_API void rtCommandBufferDestroy(rt_command_buffer command_buffer);

/*!
** @brief Discard a command buffer's recorded commands.
**
** The command buffer becomes empty and may be begun again. It must not be
** recording when reset.
**
** @param command_buffer  Command buffer to reset.
*/
RT_API void rtCommandBufferReset(rt_command_buffer command_buffer);

/*!
** @brief Begin recording a command buffer.
**
** This begins a standalone command sequence. Commands appended before the
** matching @ref rtCommandBufferEnd execute in their recorded order.
**
** @param command_buffer  Command buffer to record into.
*/
RT_API void rtCommandBufferBegin(rt_command_buffer command_buffer);

/*!
** @brief Begin recording a reusable command sequence.
**
** After @ref rtCommandBufferEnd, another recording command buffer may execute
** this sequence with @ref rtCmdExecute outside a rendering scope.
**
** @param command_buffer  Command buffer to record into.
*/
RT_API void rtCommandBufferContinue(rt_command_buffer command_buffer);

/*!
** @brief Begin recording a reusable rendering command sequence.
**
** After @ref rtCommandBufferEnd, another recording command buffer may execute
** this sequence with @ref rtCmdExecute between @ref rtCmdBeginRendering and
** @ref rtCmdEndRendering. This call does not begin a rendering scope.
**
** @param command_buffer  Command buffer to record into.
*/
RT_API void rtCommandBufferContinueRendering(rt_command_buffer command_buffer);

/*!
** @brief Finish recording a command buffer.
**
** The command buffer becomes executable and may be submitted or referenced by
** @ref rtCmdExecute according to the begin mode used.
**
** @param command_buffer  Command buffer being recorded.
*/
RT_API void rtCommandBufferEnd(rt_command_buffer command_buffer);

/*!
** @brief Append an executable command buffer to the current recording.
**
** @p secondary must be ended. A rendering continuation is valid only inside
** a rendering scope; a regular continuation is valid only outside one.
**
** @param command_buffer  Command buffer being recorded.
** @param secondary       Ended command buffer to execute.
*/
RT_API void rtCmdExecute(rt_command_buffer command_buffer, rt_command_buffer secondary);

/*===============================================================================================*/
/* Rendering commands                                                                             */
/*===============================================================================================*/

/*!
** @brief Begin a rendering scope.
**
** Subsequent rendering commands target @p framebuffer until
** @ref rtCmdEndRendering.
**
** @param command_buffer  Command buffer being recorded.
** @param framebuffer     Framebuffer whose attachments will be rendered to.
*/
RT_API void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer);

/*!
** @brief Set the next clear value for one color attachment.
**
** This does not clear the attachment. @ref rtCmdClear consumes the currently
** selected value.
**
** @param command_buffer  Command buffer being recorded.
** @param location        Non-null fragment-output location obtained from the
**                        program.
** @param r               Red.
** @param g               Green.
** @param b               Blue.
** @param a               Alpha.
*/
RT_API void rtCmdClearColor(rt_command_buffer command_buffer, rt_location location, f32 r, f32 g, f32 b, f32 a);

/*!
** @brief Set the next depth clear value.
**
** This does not clear the depth attachment.
**
** @param command_buffer  Command buffer being recorded.
** @param depth           Depth clear value.
*/
RT_API void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth);

/*!
** @brief Set the next stencil clear value.
**
** This does not clear the stencil attachment.
**
** @param command_buffer  Command buffer being recorded.
** @param stencil         Stencil clear value.
*/
RT_API void rtCmdClearStencil(rt_command_buffer command_buffer, usize stencil);

/*!
** @brief Append a clear of selected framebuffer attachments.
**
** Combine @ref RT_CLEAR_COLOR, @ref RT_CLEAR_DEPTH, and
** @ref RT_CLEAR_STENCIL with bitwise OR. Each selected attachment uses its
** current clear value.
**
** @param command_buffer  Command buffer being recorded.
** @param attachments     Bitset of @ref rt_clear_flag values to clear.
*/
RT_API void rtCmdClear(rt_command_buffer command_buffer, enum rt_clear_flag attachments);

/*!
** @brief Set the viewport used by subsequent draws.
**
** @param command_buffer  Command buffer being recorded.
** @param x               Viewport left edge in framebuffer pixels.
** @param y               Viewport top edge in framebuffer pixels.
** @param width           Viewport width in framebuffer pixels.
** @param height          Viewport height in framebuffer pixels.
** @param min_depth       Minimum depth value.
** @param max_depth       Maximum depth value.
*/
RT_API void rtCmdSetViewport(rt_command_buffer command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth);

/*!
** @brief Set the scissor used by subsequent draws.
**
** @param command_buffer  Command buffer being recorded.
** @param x               Scissor left edge in framebuffer pixels.
** @param y               Scissor top edge in framebuffer pixels.
** @param width           Scissor width in framebuffer pixels.
** @param height          Scissor height in framebuffer pixels.
*/
RT_API void rtCmdSetScissor(rt_command_buffer command_buffer, usize x, usize y, usize width, usize height);

/*!
** @brief End the current rendering scope.
**
** Subsequent commands are outside rendering until another
** @ref rtCmdBeginRendering.
**
** @param command_buffer  Command buffer being recorded.
*/
RT_API void rtCmdEndRendering(rt_command_buffer command_buffer);

/*===============================================================================================*/
/* Command state                                                                                  */
/*===============================================================================================*/

/*!
** @brief Use a program for subsequent draw commands.
**
** @p program must be finalized. It remains current until another program is
** used or the recording ends.
**
** @param command_buffer  Command buffer being recorded.
** @param program         Finalized program to use.
*/
RT_API void rtCmdUseProgram(rt_command_buffer command_buffer, rt_program program);

/*!
** @brief Bind a buffer range for subsequent draws.
**
** @p location must be a uniform-resource location from the current program.
**
** @param command_buffer  Command buffer being recorded.
** @param location        Uniform-resource location obtained from the current program.
** @param buffer          Buffer to bind to the resource slot.
** @param range           Byte range visible through the resource slot.
*/
RT_API void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range);

/*!
** @brief Bind a texture view for subsequent draws.
**
** @p location must be a uniform-resource location from the current program.
**
** @param command_buffer  Command buffer being recorded.
** @param location        Uniform-resource location obtained from the current program.
** @param texture_view    Texture view to bind to the resource slot.
*/
RT_API void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view);

/*!
** @brief Bind a sampler for subsequent draws.
**
** @p location must be a uniform-resource location from the current program.
**
** @param command_buffer  Command buffer being recorded.
** @param location        Uniform-resource location obtained from the current program.
** @param sampler         Sampler to bind to the resource slot.
*/
RT_API void rtCmdBindSampler(rt_command_buffer command_buffer, rt_location location, rt_sampler sampler);

/*!
** @brief Set the vertex buffer for one input group.
**
** @p location identifies one input group in the current program. @p range
** begins at input element zero and supplies every attribute in that group.
**
** @param command_buffer  Command buffer being recorded.
** @param location        Input location obtained from the program.
** @param buffer          Buffer containing the input group's elements.
** @param range           Byte range whose offset is input element 0.
*/
RT_API void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range);

/*!
** @brief Set the index buffer for subsequent indexed draws.
**
** @p range begins at index element zero. The index buffer remains current
** until replaced or the recording ends.
**
** @param command_buffer  Command buffer being recorded.
** @param buffer          Buffer containing indices.
** @param range           Byte range whose offset is index 0.
** @param format          Index-element format.
*/
RT_API void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, enum rt_index_format format);

/*===============================================================================================*/
/* Drawing commands                                                                               */
/*===============================================================================================*/

/*!
** @brief Append a non-indexed draw.
**
** @param command_buffer  Command buffer being recorded.
** @param vertex_count    Number of vertices to draw.
** @param first_vertex    Index of the first vertex (added to vertex IDs).
*/
RT_API void rtCmdDraw(rt_command_buffer command_buffer, usize vertex_count, usize first_vertex);

/*!
** @brief Append an instanced non-indexed draw.
**
** @param command_buffer  Command buffer being recorded.
** @param vertex_count    Number of vertices per instance.
** @param instance_count  Number of instances to draw.
** @param first_vertex    Index of the first vertex.
** @param first_instance  Index of the first instance.
*/
RT_API void rtCmdDrawInstanced(rt_command_buffer command_buffer, usize vertex_count, usize instance_count, usize first_vertex, usize first_instance);

/*!
** @brief Append an indexed draw.
**
** @param command_buffer  Command buffer being recorded.
** @param index_count     Number of indices to draw.
** @param first_index     Index of the first index element.
** @param vertex_offset   Added to every vertex index.
*/
RT_API void rtCmdDrawIndexed(rt_command_buffer command_buffer, usize index_count, usize first_index, usize vertex_offset);

/*!
** @brief Append an indexed instanced draw.
**
** @param command_buffer  Command buffer being recorded.
** @param index_count     Number of indices per instance.
** @param instance_count  Number of instances to draw.
** @param first_index     Index of the first index element.
** @param vertex_offset   Added to every vertex index.
** @param first_instance  Index of the first instance.
*/
RT_API void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, usize index_count, usize instance_count, usize first_index, usize vertex_offset, usize first_instance);

/*===============================================================================================*/
/* Queue                                                                                         */
/*===============================================================================================*/

/*!
** @brief Create a queue with the requested capability.
**
** Submissions on the returned queue complete in submission order.
**
** @param capability  Required queue capability.
** @return New queue, or NULL when the capability is unavailable.
*/
RT_API rt_queue rtQueueCreate(enum rt_queue_capability capability);

/*!
** @brief Destroy a queue.
**
** The queue handle is invalid after this call. Timepoints returned before the
** call remain usable until @ref rtUnload.
**
** @param queue  Queue to destroy.
*/
RT_API void rtQueueDestroy(rt_queue queue);

/*!
** @brief Make a queue's next submission wait for a timepoint.
**
** The dependency is consumed by the next @ref rtQueueSubmit on @p queue. This
** call does not wait on the calling thread.
**
** @param queue      Queue whose next submission depends on @p timepoint.
** @param timepoint  Completion point required before that submission runs.
*/
RT_API void rtQueueWait(rt_queue queue, rt_timepoint timepoint);

/*!
** @brief Submit an executable command buffer.
**
** The returned timepoint is reached after every command in this submission
** completes. The command buffer remains executable and may be submitted again
** or reset.
**
** @param queue           Queue receiving the completed command buffer.
** @param command_buffer  Completed command buffer.
**
** @return Completion timepoint for this submission.
*/
RT_API rt_timepoint rtQueueSubmit(rt_queue queue, rt_command_buffer command_buffer);

/*!
** @brief Return a completion point for prior queue submissions.
**
** The returned timepoint is reached after all submissions made on @p queue
** before this call complete.
**
** @param queue  Queue to flush.
**
** @return Completion timepoint for the prior submissions.
*/
RT_API rt_timepoint rtQueueFlush(rt_queue queue);

/*!
** @brief Wait on the calling thread until a timepoint is reached.
**
** A zero-initialized or already-reached timepoint returns immediately.
**
** @param timepoint  Timepoint to wait for.
*/
RT_API void rtTimepointWait(rt_timepoint timepoint);

/*!
** @brief Check a timepoint without waiting.
**
** @param timepoint  Timepoint to query.
** @return true when reached. A zero-initialized timepoint is always reached.
*/
RT_API bool rtTimepointReached(rt_timepoint timepoint);

/*===============================================================================================*/
/* Framebuffer                                                                                   */
/*===============================================================================================*/

/*!
** @brief Create a framebuffer.
**
** The new framebuffer has no attachments.
**
** @return New framebuffer, or NULL on failure.
*/
RT_API rt_framebuffer rtFramebufferCreate(void);

/*!
** @brief Destroy a framebuffer.
**
** The framebuffer handle is invalid after this call. Attached texture views
** are not destroyed.
**
** @param framebuffer  Framebuffer to destroy.
*/
RT_API void rtFramebufferDestroy(rt_framebuffer framebuffer);

/*!
** @brief Return the color attachment at a location.
**
** @param framebuffer  Framebuffer to query.
** @param location     Non-null fragment-output location obtained from the
**                     program.
** @return Attached texture view, or NULL when the location is empty.
*/
RT_API rt_texture_view rtFramebufferColorView(rt_framebuffer framebuffer, rt_location location);

/*!
** @brief Set the color attachment at a location.
**
** @param framebuffer  Framebuffer to update.
** @param view         Texture view to attach, or NULL to detach the current
**                     view at @p location.
** @param location     Non-null fragment-output location obtained from the
**                     program.
*/
RT_API void rtFramebufferSetColorView(rt_framebuffer framebuffer, rt_texture_view view, rt_location location);

/*!
** @brief Set the framebuffer's depth attachment.
**
** @param framebuffer  Framebuffer to update.
** @param view         Depth or depth-stencil view, or NULL to detach it.
*/
RT_API void rtFramebufferSetDepthView(rt_framebuffer framebuffer, rt_texture_view view);

/*!
** @brief Set the framebuffer's stencil attachment.
**
** One depth-stencil view may be set as both the depth and stencil attachment.
**
** @param framebuffer  Framebuffer to update.
** @param view         Stencil or depth-stencil view, or NULL to detach it.
*/
RT_API void rtFramebufferSetStencilView(rt_framebuffer framebuffer, rt_texture_view view);

/*===============================================================================================*/
/* Program                                                                              */
/*===============================================================================================*/

/*!
** @brief Create a program.
**
** The new program may be configured until @ref rtProgramFinalize.
**
** @return New program, or NULL on failure.
*/
RT_API rt_program rtProgramCreate(void);

/*!
** @brief Destroy a program.
**
** The program and locations queried from it are invalid after this call.
**
** @param program  Program to destroy.
*/
RT_API void rtProgramDestroy(rt_program program);

/*!
** @brief Set the vertex input layout used by a program.
**
** Each @ref rt_vertex_input describes one vertex buffer. Its attributes are
** interleaved using the input stride and are consumed at the input rate. The
** complete description is copied before this call returns. Passing NULL sets
** an empty layout.
**
** @param program  Program to configure.
** @param layout   Vertex layout description, or NULL.
*/
RT_API void rtProgramSetLayout(rt_program program, const rt_vertex_layout* layout);

/*!
** @brief Load the shader stages exposed by an RTSL Program binary.
**
** @p bytes contains one RTSL Program binary. Every shader stage exposed by
** the binary for @p entry_point is added to @p program. The entry-point name
** and @p byte_size source bytes are copied before this call returns.
**
** Calling this on a finalized program is invalid. Create a new program to
** change its source.
**
** @param program      Program to configure.
** @param entry_point  Null-terminated entry-point name to load.
** @param bytes        Pointer to an RTSL Program binary.
** @param byte_size    Size of @p bytes in bytes.
*/
RT_API void rtProgramSource(rt_program program, const char* entry_point, const u08* bytes, usize byte_size);

/*!
** @brief Set rasterization state for a program.
**
** The selected state applies to draws that use the finalized program.
**
** @param program     Program to configure.
** @param cull_mode   Which triangle faces are culled.
** @param front_face  Winding order considered front-facing.
** @param fill_mode   Triangle fill mode (solid or wireframe).
*/
RT_API void rtProgramSetRasterState(rt_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode);

/*!
** @brief Set color-blend state for a program.
**
** When @p enabled is false, fragment output replaces the current attachment
** value and the factor and operation arguments have no effect.
**
** When enabled, RGB uses @p src_color, @p dst_color, and @p color_op. Alpha
** uses @p src_alpha, @p dst_alpha, and @p alpha_op.
**
** @param program     Program to configure.
** @param enabled     Master enable for color blending.
** @param src_color   Source factor for the RGB channels.
** @param dst_color   Destination factor for the RGB channels.
** @param color_op    Combining operation for the RGB channels.
** @param src_alpha   Source factor for the alpha channel.
** @param dst_alpha   Destination factor for the alpha channel.
** @param alpha_op    Combining operation for the alpha channel.
*/
RT_API void rtProgramSetBlendState(rt_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op);

/*!
** @brief Finalize a program for use in draw commands.
**
** Success fixes the program's source, layout, raster state, and blend state.
** A finalized program may be queried for locations and used with
** @ref rtCmdUseProgram, but may no longer be configured.
**
** @param program  Program to finalize.
**
** @error RT_SHADER_COMPILATION_FAILED  A selected stage is invalid.
** @error RT_SHADER_LINK_FAILED         The selected stages are incompatible.
*/
RT_API void rtProgramFinalize(rt_program program);

/*!
** @brief Look up a named uniform-resource location.
**
** The program must be finalized. The returned location may be used with
** @ref rtCmdBindBuffer, @ref rtCmdBindTexture, or @ref rtCmdBindSampler
** according to the named resource's type.
**
** @param program  Finalized program to query.
** @param name     Null-terminated uniform-resource name.
** @return Location handle, or NULL if @p name is not a uniform resource.
*/
RT_API rt_location rtProgramUniformLocation(rt_program program, const char* name);

/*!
** @brief Look up a vertex-buffer input location.
**
** The program must be finalized. The returned location identifies the input
** whose attribute sequence exactly matches @p attributes.
**
** @param program  Finalized program to query.
** @param attributes      Attribute array from one @ref rt_vertex_input.
** @param attribute_count Number of attributes in @p attributes.
** @return Location handle, or NULL if no input has those attributes.
*/
RT_API rt_location rtProgramInputLocation(rt_program program, const rt_vertex_attribute* attributes, usize attribute_count);

/*!
** @brief Look up a named fragment-output location.
**
** The program must be finalized. Pass NULL for an unnamed single color
** output. A matching output returns a non-null location whose address is
** zero. Named output locations may be passed to
** @ref rtFramebufferSetColorView.
**
** @param program  Finalized program to query.
** @param name     Null for the single unnamed color output, or a
**                 null-terminated fragment-output field name.
** @return Location handle, or NULL if @p name is not a fragment output.
*/
RT_API rt_location rtProgramOutputLocation(rt_program program, const char* name);

/*===============================================================================================*/
/* Buffer                                                                                        */
/*===============================================================================================*/

/*!
** @brief Create a buffer.
**
** The new buffer has size zero until @ref rtBufferResize.
**
** @return New buffer, or NULL on failure.
*/
RT_API rt_buffer rtBufferCreate(void);

/*!
** @brief Destroy a buffer.
**
** The buffer handle is invalid after this call.
**
** @param buffer Buffer to destroy.
*/
RT_API void rtBufferDestroy(rt_buffer buffer);

/*!
** @brief Set a buffer's size and access class.
**
** The buffer contains @p size bytes after this call. Existing contents are
** not preserved.
**
** @param buffer       Buffer to resize.
** @param memory_type  Direct or command-only access class.
** @param size         New size in bytes.
*/
RT_API void rtBufferResize(rt_buffer buffer, enum rt_memory_type memory_type, usize size);

/*!
** @brief Read a buffer range.
**
** On return, @p data contains @p range.size bytes from @p buffer.
**
** @param buffer  Source buffer.
** @param range   Byte range to copy.
** @param data       Destination with room for the selected range.
** @param data_size  Available destination size in bytes.
*/
RT_API void rtBufferRead(rt_buffer buffer, rt_buffer_range range, u08* data, usize data_size);

/*!
** @brief Directly access a host-memory buffer range.
**
** Only @ref RT_HOST_MEMORY buffers may be mapped. The returned pointer covers
** exactly @p range and remains valid until @ref rtBufferUnmap.
**
** @param buffer  Host-memory buffer to map.
** @param range   Byte range to map.
** @return Pointer to the first mapped byte, or NULL on failure.
*/
RT_API u08* rtBufferMap(rt_buffer buffer, rt_buffer_range range);

/*!
** @brief End direct access to a mapped buffer.
**
** @param buffer  Buffer previously passed to @ref rtBufferMap.
*/
RT_API void rtBufferUnmap(rt_buffer buffer);

/*!
** @brief Append a buffer-range upload.
**
** Exactly @p range.size bytes are copied from @p data before this call
** returns. Executing the command writes those bytes without resizing the
** buffer.
**
** @param command_buffer  Command buffer being recorded.
** @param buffer          Destination buffer.
** @param range           Byte range to upload.
** @param data            Source bytes.
*/
RT_API void rtCmdBufferData(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, const u08* data);

/*!
** @brief Append a buffer-to-buffer copy.
**
** Executing the command copies @p src_range.size bytes. Source and
** destination ranges must have the same size.
**
** @param command_buffer  Command buffer being recorded.
** @param src             Source buffer.
** @param src_range       Source byte range.
** @param dst             Destination buffer.
** @param dst_range       Destination byte range.
*/
RT_API void rtCmdBufferCopy(rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_buffer dst, rt_buffer_range dst_range);

/*!
** @brief Append a buffer-to-texture copy.
**
** @param command_buffer  Command buffer being recorded.
** @param src             Source buffer.
** @param src_range       Source byte range.
** @param dst             Destination texture.
** @param dst_range       Destination subresource and texel range.
*/
RT_API void rtCmdBufferCopyToTexture(rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_texture dst, rt_texture_range dst_range);

/*!
** @brief Append a dependency between buffer accesses.
**
** Matching @p src accesses recorded before the barrier complete before
** matching @p dst accesses recorded after it begin. Writes made by the source
** accesses are visible to the destination accesses for @p range.
**
** @param command_buffer  Command buffer being recorded.
** @param buffer          Buffer whose range is synchronized.
** @param range           Contiguous byte range to synchronize.
** @param src             Source stage and access type before this barrier.
** @param dst             Destination stage and access type after this barrier.
*/
RT_API void rtCmdBufferBarrier(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, rt_access src, rt_access dst);

/*===============================================================================================*/
/* Texture                                                                                       */
/*===============================================================================================*/

/*!
** @brief Create a texture.
**
** The new texture has no texel extent until @ref rtTextureResize.
**
** @return New texture, or NULL on failure.
*/
RT_API rt_texture rtTextureCreate(void);

/*!
** @brief Destroy a texture.
**
** The texture handle is invalid after this call.
**
** @param texture  Texture to destroy.
*/
RT_API void rtTextureDestroy(rt_texture texture);

/*!
** @brief Set a texture's type, format, extent, and mip count.
**
** Existing texel contents are not preserved.
**
** @param texture    Texture to resize.
** @param type       Texture dimensionality.
** @param format     Pixel format.
** @param extent     Base-level extent in texels.
** @param mip_count  Number of mip levels.
*/
RT_API void rtTextureResize(rt_texture texture, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count);

/*!
** @brief Append a texture-to-texture copy.
**
** Source and destination aspects, mip counts, layer counts, and extents must
** match. Executing the command copies the complete selected range.
**
** @param command_buffer  Command buffer being recorded.
** @param src             Source texture.
** @param src_range       Source subresource and texel range.
** @param dst             Destination texture.
** @param dst_range       Destination subresource and texel range.
*/
RT_API void rtCmdTextureCopy(rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_texture dst, rt_texture_range dst_range);

/*!
** @brief Append a texture-range upload.
**
** The tightly packed source texels are copied before this call returns.
** Executing the command writes the selected range without resizing the
** texture.
**
** @param command_buffer  Command buffer being recorded.
** @param texture         Destination texture.
** @param range           Destination subresource and texel range.
** @param data            Tightly packed source texels.
*/
RT_API void rtCmdTextureData(rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, const u08* data);

/*!
** @brief Append a texture-to-buffer copy.
**
** @param command_buffer  Command buffer being recorded.
** @param src             Source texture.
** @param src_range       Source subresource and texel range.
** @param dst             Destination buffer.
** @param dst_range       Destination byte range.
*/
RT_API void rtCmdTextureCopyToBuffer(rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_buffer dst, rt_buffer_range dst_range);

/*!
** @brief Append a dependency between texture accesses.
**
** Matching @p src accesses recorded before the barrier complete before
** matching @p dst accesses recorded after it begin. Writes made by the source
** accesses are visible to the destination accesses for @p range.
**
** @param command_buffer  Command buffer being recorded.
** @param texture         Texture whose range is synchronized.
** @param range           Subresource and texel range to synchronize.
** @param src             Source stage and access type.
** @param dst             Destination stage and access type.
*/
RT_API void rtCmdTextureBarrier(rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, rt_access src, rt_access dst);

/*===============================================================================================*/
/* Texture view                                                                                  */
/*===============================================================================================*/

/*!
** @brief Create a texture view.
**
** The new view has no texture until @ref rtTextureViewSetTexture.
**
** @return New texture view, or NULL on failure.
*/
RT_API rt_texture_view rtTextureViewCreate(void);

/*!
** @brief Destroy a texture view.
**
** The view handle is invalid after this call. Its texture is not destroyed.
**
** @param texture_view  Texture view to destroy.
*/
RT_API void rtTextureViewDestroy(rt_texture_view texture_view);

/*!
** @brief Return the texel extent visible through a texture view.
**
** @param texture_view  Texture view to query.
** @return The view's extent in texels.
*/
RT_API rt_extent_3d rtTextureViewExtent(rt_texture_view texture_view);

/*!
** @brief Set the texture used by a texture view.
**
** Subsequent uses of @p texture_view refer to @p texture. Setting a new
** texture replaces the previous one.
**
** @param texture_view  Texture view to update.
** @param texture       Texture to view through @p texture_view.
*/
RT_API void rtTextureViewSetTexture(rt_texture_view texture_view, rt_texture texture);

/*!
** @brief Read texels through a texture view.
**
** On return, @p data contains the tightly packed texels selected by @p range.
**
** @param texture_view  Texture view to read.
** @param range         Texture range to read.
** @param data          Destination for the selected texels.
** @param data_size     Available destination size in bytes.
*/
RT_API void rtTextureViewRead(rt_texture_view texture_view, rt_texture_range range, u08* data, usize data_size);

/*===============================================================================================*/
/* Sampler                                                                                       */
/*===============================================================================================*/

/*!
** @brief Create a sampler.
**
** @return New sampler, or NULL on failure.
*/
RT_API rt_sampler rtSamplerCreate(void);

/*!
** @brief Destroy a sampler.
**
** The sampler handle is invalid after this call.
**
** @param sampler  Sampler to destroy.
*/
RT_API void rtSamplerDestroy(rt_sampler sampler);

/*!
** @brief Set a sampler's texture filtering.
**
** @param sampler     Sampler to update.
** @param mag_filter  Filter used when the footprint covers less than one
**                    texel.
** @param min_filter  Filter used when the footprint covers more than one
**                    texel.
** @param mip_filter  Filter applied between mip levels, or
**                    RT_MIP_FILTER_NONE to disable mip sampling.
*/
RT_API void rtSamplerSetFilter(rt_sampler sampler, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter);

/*!
** @brief Set a sampler's per-axis address modes.
**
** @param sampler    Sampler to update.
** @param address_u  Address mode for the U axis.
** @param address_v  Address mode for the V axis.
** @param address_w  Address mode for the W axis.
*/
RT_API void rtSamplerSetAddress(rt_sampler sampler, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w);

/*!
** @brief Set a sampler's maximum anisotropy.
**
** A value of zero disables anisotropic filtering.
**
** @param sampler         Sampler to update.
** @param max_anisotropy  Maximum anisotropic sample count.
*/
RT_API void rtSamplerSetAnisotropy(rt_sampler sampler, usize max_anisotropy);

/*!
** @brief Set a sampler's LOD selection.
**
** The bias is applied before the selected level is clamped to the inclusive
** range from @p min_lod to @p max_lod.
**
** @param sampler   Sampler to update.
** @param min_lod   Lower clamp on the computed mip level.
** @param max_lod   Upper clamp on the computed mip level.
** @param lod_bias  Bias added before clamping.
*/
RT_API void rtSamplerSetLod(rt_sampler sampler, f32 min_lod, f32 max_lod, f32 lod_bias);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

#endif /* !RT_TYPES_ONLY */

#define RT_CORE_PROCEDURES(X)                                                                      \
	X(void, rtInit, (const char* const* features, usize feature_count), (features, feature_count)) \
	X(void, rtExit, (void), ())                                                                    \
	X(u64, rtVersion, (void), ())                                                                  \
	X(void, rtSetOutput, (rt_output output, void* user_data), (output, user_data))                 \
	X(enum rt_error, rtError, (void), ())                                                          \
	X(const char*, rtErrorMessage, (void), ())                                                     \
	X(void, rtClearError, (void), ())                                                              \
	X(const char*, rtGetName, (void), ())
/* RT_CORE_PROCEDURES */

#define RT_COMMAND_BUFFER_EXTENSION_PROCEDURES(X)                                                                                                                                                                                     \
	X(rt_command_buffer, rtCommandBufferCreate, (void), ())                                                                                                                                                                           \
	X(void, rtCommandBufferDestroy, (rt_command_buffer command_buffer), (command_buffer))                                                                                                                                             \
	X(void, rtCommandBufferReset, (rt_command_buffer command_buffer), (command_buffer))                                                                                                                                               \
	X(void, rtCommandBufferBegin, (rt_command_buffer command_buffer), (command_buffer))                                                                                                                                               \
	X(void, rtCommandBufferContinue, (rt_command_buffer command_buffer), (command_buffer))                                                                                                                                            \
	X(void, rtCommandBufferContinueRendering, (rt_command_buffer command_buffer), (command_buffer))                                                                                                                                   \
	X(void, rtCommandBufferEnd, (rt_command_buffer command_buffer), (command_buffer))                                                                                                                                                 \
	X(void, rtCmdExecute, (rt_command_buffer command_buffer, rt_command_buffer secondary), (command_buffer, secondary))                                                                                                               \
	X(void, rtCmdBeginRendering, (rt_command_buffer command_buffer, rt_framebuffer framebuffer), (command_buffer, framebuffer))                                                                                                       \
	X(void, rtCmdClearColor, (rt_command_buffer command_buffer, rt_location location, f32 r, f32 g, f32 b, f32 a), (command_buffer, location, r, g, b, a))                                                                            \
	X(void, rtCmdClearDepth, (rt_command_buffer command_buffer, f32 depth), (command_buffer, depth))                                                                                                                                  \
	X(void, rtCmdClearStencil, (rt_command_buffer command_buffer, usize stencil), (command_buffer, stencil))                                                                                                                          \
	X(void, rtCmdClear, (rt_command_buffer command_buffer, enum rt_clear_flag attachments), (command_buffer, attachments))                                                                                                            \
	X(void, rtCmdSetViewport, (rt_command_buffer command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth), (command_buffer, x, y, width, height, min_depth, max_depth))                             \
	X(void, rtCmdSetScissor, (rt_command_buffer command_buffer, usize x, usize y, usize width, usize height), (command_buffer, x, y, width, height))                                                                                  \
	X(void, rtCmdEndRendering, (rt_command_buffer command_buffer), (command_buffer))                                                                                                                                                  \
	X(void, rtCmdDraw, (rt_command_buffer command_buffer, usize vertex_count, usize first_vertex), (command_buffer, vertex_count, first_vertex))                                                                                      \
	X(void, rtCmdDrawInstanced, (rt_command_buffer command_buffer, usize vertex_count, usize instance_count, usize first_vertex, usize first_instance), (command_buffer, vertex_count, instance_count, first_vertex, first_instance)) \
	X(void, rtCmdDrawIndexed, (rt_command_buffer command_buffer, usize index_count, usize first_index, usize vertex_offset), (command_buffer, index_count, first_index, vertex_offset))                                               \
	X(void, rtCmdDrawIndexedInstanced, (rt_command_buffer command_buffer, usize index_count, usize instance_count, usize first_index, usize vertex_offset, usize first_instance), (command_buffer, index_count, instance_count, first_index, vertex_offset, first_instance))
/* RT_COMMAND_BUFFER_EXTENSION_PROCEDURES */

#define RT_QUEUE_EXTENSION_PROCEDURES(X)                                                                        \
	X(rt_queue, rtQueueCreate, (enum rt_queue_capability capability), (capability))                             \
	X(void, rtQueueDestroy, (rt_queue queue), (queue))                                                          \
	X(void, rtQueueWait, (rt_queue queue, rt_timepoint timepoint), (queue, timepoint))                          \
	X(rt_timepoint, rtQueueSubmit, (rt_queue queue, rt_command_buffer command_buffer), (queue, command_buffer)) \
	X(rt_timepoint, rtQueueFlush, (rt_queue queue), (queue))                                                    \
	X(void, rtTimepointWait, (rt_timepoint timepoint), (timepoint))                                             \
	X(bool, rtTimepointReached, (rt_timepoint timepoint), (timepoint))
/* RT_QUEUE_EXTENSION_PROCEDURES */

#define RT_FRAMEBUFFER_EXTENSION_PROCEDURES(X)                                                                                                  \
	X(rt_framebuffer, rtFramebufferCreate, (void), ())                                                                                          \
	X(void, rtFramebufferDestroy, (rt_framebuffer framebuffer), (framebuffer))                                                                  \
	X(rt_texture_view, rtFramebufferColorView, (rt_framebuffer framebuffer, rt_location location), (framebuffer, location))                     \
	X(void, rtFramebufferSetColorView, (rt_framebuffer framebuffer, rt_texture_view view, rt_location location), (framebuffer, view, location)) \
	X(void, rtFramebufferSetDepthView, (rt_framebuffer framebuffer, rt_texture_view view), (framebuffer, view))                                 \
	X(void, rtFramebufferSetStencilView, (rt_framebuffer framebuffer, rt_texture_view view), (framebuffer, view))
/* RT_FRAMEBUFFER_EXTENSION_PROCEDURES */

#define RT_PROGRAM_EXTENSION_PROCEDURES(X)                                                                                                                                                                                                                                                                                                        \
	X(void, rtCmdUseProgram, (rt_command_buffer command_buffer, rt_program program), (command_buffer, program))                                                                                                                                                                                                                                   \
	X(rt_program, rtProgramCreate, (void), ())                                                                                                                                                                                                                                                                                                    \
	X(void, rtProgramDestroy, (rt_program program), (program))                                                                                                                                                                                                                                                                                    \
	X(void, rtProgramSetLayout, (rt_program program, const rt_vertex_layout* layout), (program, layout))                                                                                                                                                                                                                                          \
	X(void, rtProgramSource, (rt_program program, const char* entry_point, const u08* bytes, usize byte_size), (program, entry_point, bytes, byte_size))                                                                                                                                                                                          \
	X(void, rtProgramSetRasterState, (rt_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode), (program, cull_mode, front_face, fill_mode))                                                                                                                                                  \
	X(void, rtProgramSetBlendState, (rt_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op), (program, enabled, src_color, dst_color, color_op, src_alpha, dst_alpha, alpha_op)) \
	X(void, rtProgramFinalize, (rt_program program), (program))                                                                                                                                                                                                                                                                                   \
	X(rt_location, rtProgramUniformLocation, (rt_program program, const char* name), (program, name))                                                                                                                                                                                                                                             \
	X(rt_location, rtProgramInputLocation, (rt_program program, const rt_vertex_attribute* attributes, usize attribute_count), (program, attributes, attribute_count))                                                                                                                                                                            \
	X(rt_location, rtProgramOutputLocation, (rt_program program, const char* name), (program, name))
/* RT_PROGRAM_EXTENSION_PROCEDURES */

#define RT_BUFFER_EXTENSION_PROCEDURES(X)                                                                                                                                                                         \
	X(void, rtCmdBindBuffer, (rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range), (command_buffer, location, buffer, range))                                        \
	X(void, rtCmdVertexBuffer, (rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range), (command_buffer, location, buffer, range))                                      \
	X(void, rtCmdIndexBuffer, (rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, enum rt_index_format format), (command_buffer, buffer, range, format))                                  \
	X(rt_buffer, rtBufferCreate, (void), ())                                                                                                                                                                      \
	X(void, rtBufferDestroy, (rt_buffer buffer), (buffer))                                                                                                                                                        \
	X(void, rtBufferResize, (rt_buffer buffer, enum rt_memory_type memory_type, usize size), (buffer, memory_type, size))                                                                                         \
	X(void, rtBufferRead, (rt_buffer buffer, rt_buffer_range range, u08 * data, usize data_size), (buffer, range, data, data_size))                                                                               \
	X(u08*, rtBufferMap, (rt_buffer buffer, rt_buffer_range range), (buffer, range))                                                                                                                              \
	X(void, rtBufferUnmap, (rt_buffer buffer), (buffer))                                                                                                                                                          \
	X(void, rtCmdBufferData, (rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, const u08* data), (command_buffer, buffer, range, data))                                                 \
	X(void, rtCmdBufferCopy, (rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_buffer dst, rt_buffer_range dst_range), (command_buffer, src, src_range, dst, dst_range))            \
	X(void, rtCmdBufferCopyToTexture, (rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_texture dst, rt_texture_range dst_range), (command_buffer, src, src_range, dst, dst_range)) \
	X(void, rtCmdBufferBarrier, (rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, rt_access src, rt_access dst), (command_buffer, buffer, range, src, dst))
/* RT_BUFFER_EXTENSION_PROCEDURES */

#define RT_EXT_TEXTURE_PROCEDURES(X)                                                                                                                                                                              \
	X(void, rtCmdBindTexture, (rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view), (command_buffer, location, texture_view))                                                   \
	X(void, rtCmdBindSampler, (rt_command_buffer command_buffer, rt_location location, rt_sampler sampler), (command_buffer, location, sampler))                                                                  \
	X(rt_texture, rtTextureCreate, (void), ())                                                                                                                                                                    \
	X(void, rtTextureDestroy, (rt_texture texture), (texture))                                                                                                                                                    \
	X(void, rtTextureResize, (rt_texture texture, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count), (texture, type, format, extent, mip_count))                            \
	X(void, rtCmdTextureCopy, (rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_texture dst, rt_texture_range dst_range), (command_buffer, src, src_range, dst, dst_range))       \
	X(void, rtCmdTextureData, (rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, const u08* data), (command_buffer, texture, range, data))                                            \
	X(void, rtCmdTextureCopyToBuffer, (rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_buffer dst, rt_buffer_range dst_range), (command_buffer, src, src_range, dst, dst_range)) \
	X(void, rtCmdTextureBarrier, (rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, rt_access src, rt_access dst), (command_buffer, texture, range, src, dst))                        \
	X(rt_texture_view, rtTextureViewCreate, (void), ())                                                                                                                                                           \
	X(void, rtTextureViewDestroy, (rt_texture_view texture_view), (texture_view))                                                                                                                                 \
	X(rt_extent_3d, rtTextureViewExtent, (rt_texture_view texture_view), (texture_view))                                                                                                                          \
	X(void, rtTextureViewSetTexture, (rt_texture_view texture_view, rt_texture texture), (texture_view, texture))                                                                                                 \
	X(void, rtTextureViewRead, (rt_texture_view texture_view, rt_texture_range range, u08 * data, usize data_size), (texture_view, range, data, data_size))                                                       \
	X(rt_sampler, rtSamplerCreate, (void), ())                                                                                                                                                                    \
	X(void, rtSamplerDestroy, (rt_sampler sampler), (sampler))                                                                                                                                                    \
	X(void, rtSamplerSetFilter, (rt_sampler sampler, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter), (sampler, mag_filter, min_filter, mip_filter))                         \
	X(void, rtSamplerSetAddress, (rt_sampler sampler, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w), (sampler, address_u, address_v, address_w))                \
	X(void, rtSamplerSetAnisotropy, (rt_sampler sampler, usize max_anisotropy), (sampler, max_anisotropy))                                                                                                        \
	X(void, rtSamplerSetLod, (rt_sampler sampler, f32 min_lod, f32 max_lod, f32 lod_bias), (sampler, min_lod, max_lod, lod_bias))
/* RT_EXT_TEXTURE_PROCEDURES */

#define RT_CORE_EXTENSION_PROCEDURES(X)       \
	RT_COMMAND_BUFFER_EXTENSION_PROCEDURES(X) \
	RT_QUEUE_EXTENSION_PROCEDURES(X)          \
	RT_FRAMEBUFFER_EXTENSION_PROCEDURES(X)    \
	RT_PROGRAM_EXTENSION_PROCEDURES(X)        \
	RT_BUFFER_EXTENSION_PROCEDURES(X)         \
	RT_EXT_TEXTURE_PROCEDURES(X)
/* RT_CORE_EXTENSION_PROCEDURES */

#define RT_PROCEDURES(X)  \
	RT_CORE_PROCEDURES(X) \
	RT_CORE_EXTENSION_PROCEDURES(X)
/* RT_PROCEDURES */

#if !defined(RT_TYPES_ONLY)

#define RT_DECLARE_PROCEDURE(return_type, name, parameters, arguments) \
	typedef return_type(*PFN_##name) parameters;                       \
	extern PFN_##name rt_##name;                                       \
	RT_API return_type name parameters { return rt_##name arguments; }
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4098) // warning C4098: 'function': 'void' function returning a value
#endif
RT_PROCEDURES(RT_DECLARE_PROCEDURE)
#if defined(_MSC_VER)
#pragma warning(pop) // warning C4098: 'function': 'void' function returning a value
#endif
#undef RT_DECLARE_PROCEDURE

#define RT_DECLARE_EXTENSION_PROCEDURE(return_type, name, parameters, arguments) \
	typedef return_type(*PFN_##name) parameters;                                 \
	extern PFN_##name rt_##name;                                                 \
	RT_API return_type name parameters { return rt_##name arguments; }

#ifdef __cplusplus
}
#endif

#endif /* !RT_TYPES_ONLY */

#include "rt_ext_glfw.h"
#include "rt_ext_swapchain.h"

#endif /* RUTILE_H */
