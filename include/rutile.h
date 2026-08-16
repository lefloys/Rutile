#ifndef RUTILE_H
#define RUTILE_H

/*!
** @file rutile.h
** @brief Rutile public C API and dynamic loader.
**
** Rutile is a graphics abstraction with a dynamic loader and a core API for
** resources, command recording, submission, and presentation. Each public
** type and operation documents its own contract below.
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
** @brief Error reported by a Rutile operation.
**
** Except for @ref rtLoad and @ref rtLoadDevelopment, Rutile reports errors on
** the calling thread. Query the current error with @ref rtError and its text
** with @ref rtErrorMessage; clear it with @ref rtClearError.
*/
enum rt_error { RT_ERROR__RESERVED = 0x7fffffff };
#define RT_SUCCESS ((enum rt_error)0)
#define RT_OUT_OF_HOST_MEMORY ((enum rt_error)1)
#define RT_OUT_OF_DEVICE_MEMORY ((enum rt_error)2)
#define RT_IMPROPER_USAGE ((enum rt_error)3)
#define RT_PLATFORM_FAILURE ((enum rt_error)4)
#define RT_DEVICE_LOST ((enum rt_error)5)
#define RT_ALREADY_INITIALIZED ((enum rt_error)6)
#define RT_UNSUPPORTED_PLATFORM ((enum rt_error)7)
#define RT_NO_BACKEND ((enum rt_error)8)
#define RT_UNSUPPORTED_FEATURE ((enum rt_error)9)
#define RT_INITIALIZATION_FAILED ((enum rt_error)10)
#define RT_LAYER_NOT_PRESENT ((enum rt_error)11)
#define RT_EXTENSION_NOT_PRESENT ((enum rt_error)12)
#define RT_INCOMPATIBLE_DRIVER ((enum rt_error)13)
#define RT_SHADER_COMPILATION_FAILED ((enum rt_error)14)
#define RT_SHADER_LINK_FAILED ((enum rt_error)15)
#define RT_FEATURE_NOT_SUPPORTED ((enum rt_error)16)

enum rt_format { RT_FORMAT__RESERVED = 0x7fffffff };
#define RT_FORMAT_UNKNOWN ((enum rt_format)0)
#define RT_R8_UNORM ((enum rt_format)1)
#define RT_RG8_UNORM ((enum rt_format)2)
#define RT_RGB8_UNORM ((enum rt_format)3)
#define RT_RGBA8_UNORM ((enum rt_format)4)
#define RT_R16_UNORM ((enum rt_format)5)
#define RT_RG16_UNORM ((enum rt_format)6)
#define RT_RGB16_UNORM ((enum rt_format)7)
#define RT_RGBA16_UNORM ((enum rt_format)8)
#define RT_R16_SFLOAT ((enum rt_format)9)
#define RT_RG16_SFLOAT ((enum rt_format)10)
#define RT_RGB16_SFLOAT ((enum rt_format)11)
#define RT_RGBA16_SFLOAT ((enum rt_format)12)
#define RT_R32_SFLOAT ((enum rt_format)13)
#define RT_RG32_SFLOAT ((enum rt_format)14)
#define RT_RGB32_SFLOAT ((enum rt_format)15)
#define RT_RGBA32_SFLOAT ((enum rt_format)16)
#define RT_R8_SINT ((enum rt_format)17)
#define RT_RG8_SINT ((enum rt_format)18)
#define RT_RGB8_SINT ((enum rt_format)19)
#define RT_RGBA8_SINT ((enum rt_format)20)
#define RT_R16_SINT ((enum rt_format)21)
#define RT_RG16_SINT ((enum rt_format)22)
#define RT_RGB16_SINT ((enum rt_format)23)
#define RT_RGBA16_SINT ((enum rt_format)24)
#define RT_R32_SINT ((enum rt_format)25)
#define RT_RG32_SINT ((enum rt_format)26)
#define RT_RGB32_SINT ((enum rt_format)27)
#define RT_RGBA32_SINT ((enum rt_format)28)
#define RT_R8_UINT ((enum rt_format)29)
#define RT_RG8_UINT ((enum rt_format)30)
#define RT_RGB8_UINT ((enum rt_format)31)
#define RT_RGBA8_UINT ((enum rt_format)32)
#define RT_R16_UINT ((enum rt_format)33)
#define RT_RG16_UINT ((enum rt_format)34)
#define RT_RGB16_UINT ((enum rt_format)35)
#define RT_RGBA16_UINT ((enum rt_format)36)
#define RT_R32_UINT ((enum rt_format)37)
#define RT_RG32_UINT ((enum rt_format)38)
#define RT_RGB32_UINT ((enum rt_format)39)
#define RT_RGBA32_UINT ((enum rt_format)40)
#define RT_D16_UNORM ((enum rt_format)41)
#define RT_D32_SFLOAT ((enum rt_format)42)
#define RT_S8_UINT ((enum rt_format)43)
#define RT_D24_UNORM_S8_UINT ((enum rt_format)44)
#define RT_D32_SFLOAT_S8_UINT ((enum rt_format)45)

enum rt_format_usage { RT_FORMAT_USAGE__RESERVED = 0x7fffffff };
#define RT_FORMAT_USAGE_NONE ((enum rt_format_usage)0x00)
#define RT_FORMAT_USAGE_SAMPLED ((enum rt_format_usage)0x01)
#define RT_FORMAT_USAGE_COLOR_ATTACHMENT ((enum rt_format_usage)0x02)
#define RT_FORMAT_USAGE_DEPTH_ATTACHMENT ((enum rt_format_usage)0x04)
#define RT_FORMAT_USAGE_STORAGE ((enum rt_format_usage)0x08)
#define RT_FORMAT_USAGE_TRANSFER_SRC ((enum rt_format_usage)0x10)
#define RT_FORMAT_USAGE_TRANSFER_DST ((enum rt_format_usage)0x20)

/*!
** @brief Memory class selected when resizing a buffer.
**
** @ref RT_HOST_MEMORY permits @ref rtBufferMap. @ref RT_DEVICE_MEMORY does
** not permit mapping; use recorded buffer commands to transfer its bytes.
*/
enum rt_memory_type { RT_MEMORY_TYPE__RESERVED = 0x7fffffff };
#define RT_HOST_MEMORY ((enum rt_memory_type)1)
#define RT_DEVICE_MEMORY ((enum rt_memory_type)2)

/*! @brief Attachment classes selected by @ref rtCmdClear. */
enum rt_clear_flag { RT_CLEAR__RESERVED = 0x7fffffff };
#define RT_CLEAR_NONE    ((enum rt_clear_flag)0x00)
#define RT_CLEAR_COLOR   ((enum rt_clear_flag)0x01)
#define RT_CLEAR_DEPTH   ((enum rt_clear_flag)0x02)
#define RT_CLEAR_STENCIL ((enum rt_clear_flag)0x04)

/*!
** @brief Pipeline stages used by @ref rt_access in resource barriers.
**
** A stage flag names the part of a recorded operation that accesses a
** resource. Combine flags with bitwise OR when one access covers more than
** one stage.
*/
enum rt_stage_flag { RT_STAGE__RESERVED = 0x7fffffff };

/*! @brief No pipeline stage. */
#define RT_STAGE_NONE ((enum rt_stage_flag)0x00)

/*! @brief Buffer and texture uploads, copies, and readback transfers. */
#define RT_STAGE_TRANSFER ((enum rt_stage_flag)0x01)

/*! @brief Vertex-buffer fetch and vertex-shader resource accesses. */
#define RT_STAGE_VERTEX ((enum rt_stage_flag)0x02)

/*! @brief Fragment-shader resource accesses. */
#define RT_STAGE_FRAGMENT ((enum rt_stage_flag)0x04)

/*! @brief Compute-shader resource accesses. */
#define RT_STAGE_COMPUTE ((enum rt_stage_flag)0x08)

/*! @brief Color-attachment reads, writes, and clears in rendering commands. */
#define RT_STAGE_COLOR_ATTACHMENT ((enum rt_stage_flag)0x10)

/*! @brief Depth/stencil-attachment reads, writes, and clears in rendering commands. */
#define RT_STAGE_DEPTH_STENCIL_ATTACHMENT ((enum rt_stage_flag)0x20)

/*! @brief Every Rutile pipeline stage. */
#define RT_STAGE_ALL ((enum rt_stage_flag)0x3f)

/*!
** @brief Access mode used by @ref rt_access in resource barriers.
*/
enum rt_access_type { RT_ACCESS__RESERVED = 0x7fffffff };

/*! @brief No resource access. */
#define RT_ACCESS_NONE ((enum rt_access_type)0)

/*! @brief Resource reads that consume previously written contents. */
#define RT_ACCESS_READ ((enum rt_access_type)1)

/*! @brief Resource writes that produce contents for later accesses. */
#define RT_ACCESS_WRITE ((enum rt_access_type)2)

enum rt_texture_type { RT_TEXTURE_TYPE__RESERVED = 0x7fffffff };
#define RT_TEXTURE_UNKNOWN ((enum rt_texture_type)0)
#define RT_TEXTURE_1D ((enum rt_texture_type)1)
#define RT_TEXTURE_2D ((enum rt_texture_type)2)
#define RT_TEXTURE_3D ((enum rt_texture_type)3)
#define RT_TEXTURE_1D_ARRAY ((enum rt_texture_type)4)
#define RT_TEXTURE_2D_ARRAY ((enum rt_texture_type)5)

enum rt_texture_aspect_flag { RT_TEXTURE_ASPECT__RESERVED = 0x7fffffff };
#define RT_TEXTURE_ASPECT_NONE ((enum rt_texture_aspect_flag)0x00)
#define RT_TEXTURE_ASPECT_COLOR ((enum rt_texture_aspect_flag)0x01)
#define RT_TEXTURE_ASPECT_DEPTH ((enum rt_texture_aspect_flag)0x02)
#define RT_TEXTURE_ASPECT_STENCIL ((enum rt_texture_aspect_flag)0x04)

enum rt_filter { RT_FILTER__RESERVED = 0x7fffffff };
#define RT_FILTER_NEAREST ((enum rt_filter)1)
#define RT_FILTER_LINEAR ((enum rt_filter)2)

enum rt_mip_filter { RT_MIP_FILTER__RESERVED = 0x7fffffff };
#define RT_MIP_FILTER_NONE ((enum rt_mip_filter)0)
#define RT_MIP_FILTER_NEAREST ((enum rt_mip_filter)1)
#define RT_MIP_FILTER_LINEAR ((enum rt_mip_filter)2)

enum rt_address_mode { RT_ADDRESS_MODE__RESERVED = 0x7fffffff };
#define RT_ADDRESS_CLAMP ((enum rt_address_mode)1)
#define RT_ADDRESS_REPEAT ((enum rt_address_mode)2)
#define RT_ADDRESS_MIRROR ((enum rt_address_mode)3)

enum rt_queue_capability { RT_QUEUE_CAPABILITY__RESERVED = 0x7fffffff };
#define RT_QUEUE_TRANSFER ((enum rt_queue_capability)1)
#define RT_QUEUE_COMPUTE ((enum rt_queue_capability)2)
#define RT_QUEUE_GRAPHICS ((enum rt_queue_capability)3)

enum rt_index_format { RT_INDEX_FORMAT__RESERVED = 0x7fffffff };
#define RT_INDEX_U16 ((enum rt_index_format)1)
#define RT_INDEX_U32 ((enum rt_index_format)2)

enum rt_vertex_rate { RT_VERTEX_RATE__RESERVED = 0x7fffffff };
#define RT_VERTEX_RATE_VERTEX ((enum rt_vertex_rate)0)
#define RT_VERTEX_RATE_INSTANCE ((enum rt_vertex_rate)1)

enum rt_cull_mode { RT_CULL_MODE__RESERVED = 0x7fffffff };
#define RT_CULL_NONE ((enum rt_cull_mode)0)
#define RT_CULL_FRONT ((enum rt_cull_mode)1)
#define RT_CULL_BACK ((enum rt_cull_mode)2)

enum rt_front_face { RT_FRONT_FACE__RESERVED = 0x7fffffff };
#define RT_FRONT_FACE_CCW ((enum rt_front_face)0)
#define RT_FRONT_FACE_CW ((enum rt_front_face)1)

enum rt_fill_mode { RT_FILL_MODE__RESERVED = 0x7fffffff };
#define RT_FILL_SOLID ((enum rt_fill_mode)0)
#define RT_FILL_WIREFRAME ((enum rt_fill_mode)1)

enum rt_compare_op { RT_COMPARE_OP__RESERVED = 0x7fffffff };
#define RT_COMPARE_NEVER ((enum rt_compare_op)0)
#define RT_COMPARE_LESS ((enum rt_compare_op)1)
#define RT_COMPARE_EQUAL ((enum rt_compare_op)2)
#define RT_COMPARE_LESS_EQUAL ((enum rt_compare_op)3)
#define RT_COMPARE_GREATER ((enum rt_compare_op)4)
#define RT_COMPARE_NOT_EQUAL ((enum rt_compare_op)5)
#define RT_COMPARE_GREATER_EQUAL ((enum rt_compare_op)6)
#define RT_COMPARE_ALWAYS ((enum rt_compare_op)7)

enum rt_blend_factor { RT_BLEND_FACTOR__RESERVED = 0x7fffffff };
#define RT_BLEND_ZERO ((enum rt_blend_factor)0)
#define RT_BLEND_ONE ((enum rt_blend_factor)1)
#define RT_BLEND_SRC_COLOR ((enum rt_blend_factor)2)
#define RT_BLEND_ONE_MINUS_SRC_COLOR ((enum rt_blend_factor)3)
#define RT_BLEND_DST_COLOR ((enum rt_blend_factor)4)
#define RT_BLEND_ONE_MINUS_DST_COLOR ((enum rt_blend_factor)5)
#define RT_BLEND_SRC_ALPHA ((enum rt_blend_factor)6)
#define RT_BLEND_ONE_MINUS_SRC_ALPHA ((enum rt_blend_factor)7)
#define RT_BLEND_DST_ALPHA ((enum rt_blend_factor)8)
#define RT_BLEND_ONE_MINUS_DST_ALPHA ((enum rt_blend_factor)9)

enum rt_blend_op { RT_BLEND_OP__RESERVED = 0x7fffffff };
#define RT_BLEND_OP_ADD ((enum rt_blend_op)0)
#define RT_BLEND_OP_SUBTRACT ((enum rt_blend_op)1)
#define RT_BLEND_OP_REVERSE_SUBTRACT ((enum rt_blend_op)2)
#define RT_BLEND_OP_MIN ((enum rt_blend_op)3)
#define RT_BLEND_OP_MAX ((enum rt_blend_op)4)

typedef struct rt_command_buffer_t* rt_command_buffer;
typedef struct rt_queue_t* rt_queue;
typedef struct rt_framebuffer_t* rt_framebuffer;
typedef struct rt_graphics_program_t* rt_graphics_program;
typedef struct rt_buffer_t* rt_buffer;
typedef struct rt_texture_t* rt_texture;
typedef struct rt_texture_view_t* rt_texture_view;

typedef struct rt_location_t* rt_location;

/*! @brief The default fragment-output location, at color attachment zero. */
#define RT_LOCATION_ZERO ((rt_location)0)

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
** input's location with @ref rtGraphicsProgramInputLocation, passing the
** same attribute array and count, then bind that location with
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
** @brief Pipeline stage and access mode used by a resource barrier.
**
** @p stage is a bitset of RT_STAGE_* flags. @p type states whether the stage
** reads or writes the selected resource range. A barrier from
** `{ RT_STAGE_COMPUTE, RT_ACCESS_WRITE }` to
** `{ RT_STAGE_VERTEX, RT_ACCESS_READ }` makes compute writes available to
** vertex reads.
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
** @brief Load a backend and an optional ordered chain of layers.
**
** Makes @p backend_name available to Rutile. Layers in @p layer_names apply
** in array order, with the first layer closest to the application.
** After a successful call, @ref rtLoaded returns true and Rutile core API
** calls are available.
**
** At most one backend may be loaded at a time. Calling @ref rtLoad while a
** backend is already loaded is an error and fails; call @ref rtUnload first.
**
** @param backend_name  Backend name (e.g. `"rt-vulkan"`).
** @param layer_names   Optional array of layer names, applied in order. The
**                      first entry sits closest to the application; the last
**                      sits closest to the backend. May be NULL when
**                      @p layer_count is 0.
** @param layer_count   Number of entries in @p layer_names.
**
** @return RT_SUCCESS on success.
** @return RT_NO_BACKEND if the requested backend is unavailable.
** @return RT_IMPROPER_USAGE for invalid arguments, layer-loading failures, or
**         a call made while a backend is already loaded.
** @return RT_EXTENSION_NOT_PRESENT if the backend does not provide Rutile's
**         required core API.
**
** @note On failure before a backend is loaded, Rutile remains unloaded. A
**       failed second call leaves the already loaded backend in place. Use
**       @ref rtGetProc to resolve optional or extension entry points after a
**       successful load.
*/
enum rt_error rtLoad(const char* backend_name, const char* const* layer_names, usize layer_count);

/*!
** @brief Load a backend and layers in best-effort mode for development.
**
** Same intent as @ref rtLoad, except an unavailable backend is not an error.
** When no backend is available, @ref rtLoaded returns false. When a backend
** is available, every requested layer must be available as well.
**
** @param backend_name  Backend name, or NULL to load no backend.
** @param layer_names   Optional array of layer names (see @ref rtLoad).
** @param layer_count   Number of entries in @p layer_names.
**
** @return RT_SUCCESS, including when no backend is available.
** @return RT_IMPROPER_USAGE for invalid arguments or an unavailable requested
**         layer.
*/
enum rt_error rtLoadDevelopment(const char* backend_name, const char* const* layer_names, usize layer_count);

/*!
** @brief Unload the current backend and layers.
**
** Makes the current backend and layers unavailable. After this call
** @ref rtLoaded returns false and no Rutile core API call may be made until
** a backend is loaded again.
**
** Safe to call when nothing is loaded; in that case it is a no-op.
*/
void rtUnload(void);

/*!
** @brief Report whether a backend is currently loaded.
**
** @return true when Rutile core API calls are available; false otherwise.
*/
bool rtLoaded(void);

/*!
** @brief Resolve a named optional or extension entry point.
**
** Use this to access optional or extension-provided API after a successful
** load. Active layers apply to the returned entry point.
**
** @param name  Null-terminated entry-point name (e.g. `"rtCmdBufferData"`, or
**              the name of an extension function).
** @return Function pointer to the entry, or NULL if it is unavailable.
*/
rt_proc_t rtGetProc(const char* name);

/*!
** @brief Initialize Rutile with an explicit set of features.
**
** Brings up the loaded backend and any layers, requesting the listed
** features. Features are distinct from extensions: a feature is a contract
** the application announces ahead of time ("I will use presentation"), and
** the backend opts in to the corresponding support during initialization.
** Extensions, in contrast, may be queried and used at any time.
**
** A feature must be enabled here even when the backend would otherwise
** support it - the backend will only set up the matching support for
** features that were explicitly requested.
**
** Currently defined features:
**   - @ref RT_FEATURE_PRESENTATION
**
** If any requested feature is not supported by the loaded backend,
** initialization fails. The error code is set to RT_FEATURE_NOT_SUPPORTED
** and an explanatory message naming the offending feature is recorded.
**
** @param features       Array of feature name strings, or NULL when
**                       @p feature_count is 0.
** @param feature_count  Number of entries in @p features.
**
** @error RT_FEATURE_NOT_SUPPORTED   A requested feature is not supported by
**                                   the loaded backend.
** @error RT_ALREADY_INITIALIZED     rtInit has already been called without
**                                   an intervening rtExit.
** @error RT_INITIALIZATION_FAILED   The backend reported a generic
**                                   initialization failure.
** @error RT_OUT_OF_HOST_MEMORY      Host allocation failed during init.
** @error RT_OUT_OF_DEVICE_MEMORY    Device allocation failed during init.
*/
RT_API void rtInit(const char* const* features, usize feature_count);

/*!
** @brief Shut down Rutile.
**
** Returns the runtime to a pre-initialization state. The loader itself remains
** loaded; call @ref rtInit again to bring the runtime back up, or
** @ref rtUnload to make the current backend and layers unavailable. Rutile
** does not own or track caller-created handles; destroy resources before
** calling.
*/
RT_API void rtExit(void);

/*!
** @brief Install a callback that receives diagnostic output from Rutile.
**
** Routes log/debug messages produced by the backend and active layers
** through @p output instead of the default destination. Pass NULL for
** @p output to restore the default behavior.
**
** @param output     Callback to receive messages, or NULL to restore the
**                   default.
** @param user_data  Opaque pointer forwarded to every invocation of
**                   @p output. Rutile never dereferences or owns it.
**
** @note This entry point is provisional and likely to be reworked. Treat its
**       exact behavior (threading, message format, when it fires) as
**       unstable.
*/
RT_API void rtSetOutput(rt_output output, void* user_data);

/*!
** @brief Return the most recently recorded Rutile error code.
**
** Concurrent calls are safe. This reports only the error state for the
** calling thread; errors recorded by other threads are not visible here.
**
** @return The current error code, or RT_SUCCESS if none.
*/
RT_API enum rt_error rtError(void);

/*!
** @brief Return the message associated with the current error.
**
** Concurrent calls are safe. This reports only the calling thread's current
** error message. The returned pointer is owned by Rutile and remains valid
** until the next API call on that thread that can record an error.
**
** @return Null-terminated message, owned by Rutile.
*/
RT_API const char* rtErrorMessage(void);

/*!
** @brief Clear the current Rutile error state.
**
** Concurrent calls are safe. This clears only the calling thread's error
** state. Afterwards @ref rtError returns RT_SUCCESS and @ref rtErrorMessage
** returns an empty string until a later operation on that thread records an
** error.
*/
RT_API void rtClearError(void);

/*!
** @brief Return the loaded backend's name.
**
** May be called after a successful backend load, including
** @ref rtLoadDevelopment, before @ref rtInit. The returned string remains
** valid until @ref rtUnload.
**
** @return Null-terminated backend name string.
*/
RT_API const char* rtGetName(void);

/*!
** @brief Query which usages are supported for a given pixel format.
**
** Tells the application what it may legally do with @p format on the loaded
** backend: sample from it, attach it as color/depth, use it as transfer
** source/destination, bind it as storage, and so on. Use this before
** committing texture format choices that depend on the runtime device.
**
** @param format  Format to query.
** @return Bitset of @ref rt_format_usage flags. Returns RT_FORMAT_USAGE_NONE
**         for formats the backend does not support at all (including
**         unrecognized values).
*/
RT_API enum rt_format_usage rtQueryFormatCapabilities(enum rt_format format);

/*===============================================================================================*/
/* Command buffer                                                                                */
/*===============================================================================================*/

/*!
** @brief Create a command buffer.
**
** @return New command buffer handle, or NULL on failure.
*/
RT_API rt_command_buffer rtCommandBufferCreate(void);

/*!
** @brief Destroy a command buffer.
**
** @param command_buffer  Command buffer to destroy.
*/
RT_API void rtCommandBufferDestroy(rt_command_buffer command_buffer);

/*!
** @brief Discard a command buffer's recorded commands.
**
** A completed command buffer may be submitted again. Reset it before recording
** different commands. Resetting a recording buffer is invalid.
**
** @param command_buffer  Command buffer to reset.
*/
RT_API void rtCommandBufferReset(rt_command_buffer command_buffer);

/*!
** @brief Begin recording a command buffer.
**
** Every `rtCmd*` call below requires @p command_buffer to be recording.
**
** @param command_buffer  Command buffer to record into.
*/
RT_API void rtCommandBufferBegin(rt_command_buffer command_buffer);

/*!
** @brief Begin recording a command buffer for execution by another command buffer.
**
** The recorded commands do not begin a rendering scope. Record it with
** @ref rtCmdExecute from another command buffer after ending it with
** @ref rtCommandBufferEnd.
**
** @param command_buffer  Command buffer to record into.
*/
RT_API void rtCommandBufferContinue(rt_command_buffer command_buffer);

/*!
** @brief Begin recording a command buffer for execution inside a rendering scope.
**
** Use this for draw and rendering-state commands that another command buffer
** will execute between @ref rtCmdBeginRendering and @ref rtCmdEndRendering.
** It does not select a framebuffer or begin rendering itself. End recording
** with @ref rtCommandBufferEnd before recording it with @ref rtCmdExecute.
**
** @param command_buffer  Command buffer to record into.
*/
RT_API void rtCommandBufferContinueRendering(rt_command_buffer command_buffer);

/*!
** @brief Finish recording a command buffer.
**
** @param command_buffer  Command buffer being recorded.
*/
RT_API void rtCommandBufferEnd(rt_command_buffer command_buffer);

/*!
** @brief Record execution of an executable command buffer.
**
** @p secondary must have been ended after @ref rtCommandBufferContinue or
** @ref rtCommandBufferContinueRendering. A rendering continuation may be
** executed only while @p command_buffer is inside a rendering scope.
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
** @param command_buffer  Command buffer being recorded.
** @param framebuffer     Framebuffer whose attachments will be rendered to.
*/
RT_API void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer);

/*!
** @brief Set the color used for a color-attachment clear.
**
** @param command_buffer  Command buffer being recorded.
** @param location        Color attachment location, or @ref RT_LOCATION_ZERO
**                        for color attachment zero.
** @param r               Red.
** @param g               Green.
** @param b               Blue.
** @param a               Alpha.
*/
RT_API void rtCmdClearColor(rt_command_buffer command_buffer, rt_location location, f32 r, f32 g, f32 b, f32 a);

/*!
** @brief Set the depth value used for a depth-attachment clear.
**
** @param command_buffer  Command buffer being recorded.
** @param depth           Depth clear value.
*/
RT_API void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth);

/*!
** @brief Set the stencil value used for a stencil-attachment clear.
**
** @param command_buffer  Command buffer being recorded.
** @param stencil         Stencil clear value.
*/
RT_API void rtCmdClearStencil(rt_command_buffer command_buffer, usize stencil);

/*!
** @brief Clear selected attachments in the current rendering scope.
**
** Combine @ref RT_CLEAR_COLOR, @ref RT_CLEAR_DEPTH, and
** @ref RT_CLEAR_STENCIL with bitwise OR. Each selected attachment uses the
** value most recently set with @ref rtCmdClearColor, @ref rtCmdClearDepth,
** or @ref rtCmdClearStencil.
**
** @param command_buffer  Command buffer being recorded.
** @param attachments     Bitset of @ref rt_clear_flag values to clear.
*/
RT_API void rtCmdClear(rt_command_buffer command_buffer, enum rt_clear_flag attachments);

/*!
** @brief Set the viewport for the current rendering scope.
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
** @brief Set the scissor for the current rendering scope.
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
** @param command_buffer  Command buffer being recorded.
*/
RT_API void rtCmdEndRendering(rt_command_buffer command_buffer);

/*===============================================================================================*/
/* Command state                                                                                  */
/*===============================================================================================*/

/*!
** @brief Bind a graphics program for subsequent draw commands.
**
** The program must be finalized (@ref rtGraphicsProgramFinalize). The
** binding stays in effect until the next @ref rtCmdUseGraphicsProgram or
** until the command buffer ends.
**
** @param command_buffer  Command buffer being recorded.
** @param program         Finalized graphics program to bind.
*/
RT_API void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program);

/*!
** @brief Bind a buffer range to a program uniform-resource slot for subsequent draws.
**
** The slot may represent a uniform buffer or storage buffer. Vertex inputs
** are bound separately with @ref rtCmdVertexBuffer.
**
** @param command_buffer  Command buffer being recorded.
** @param location        Uniform-resource location obtained from the bound program.
** @param buffer          Buffer to bind to the resource slot.
** @param range           Byte range visible through the resource slot.
*/
RT_API void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range);

/*!
** @brief Bind a texture view to a program uniform-resource slot for subsequent draws.
**
** @p texture_view provides the combined texture and sampler resource.
**
** @param command_buffer  Command buffer being recorded.
** @param location        Uniform-resource location obtained from the bound program.
** @param texture_view    Texture view to bind to the resource slot.
*/
RT_API void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view);

/*!
** @brief Bind a vertex-buffer input group.
**
** @p location identifies one vertex input declared in the graphics-program
** layout. The buffer supplies every attribute in that input group.
**
** @param command_buffer  Command buffer being recorded.
** @param location        Input location obtained from the graphics program.
** @param buffer          Buffer containing the input group's elements.
** @param range           Byte range whose offset is input element 0.
*/
RT_API void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range);

/*!
** @brief Set the index buffer for subsequent indexed draws.
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
** @brief Record a non-indexed draw.
**
** @param command_buffer  Command buffer being recorded.
** @param vertex_count    Number of vertices to draw.
** @param first_vertex    Index of the first vertex (added to vertex IDs).
*/
RT_API void rtCmdDraw(rt_command_buffer command_buffer, usize vertex_count, usize first_vertex);

/*!
** @brief Record an instanced non-indexed draw.
**
** @param command_buffer  Command buffer being recorded.
** @param vertex_count    Number of vertices per instance.
** @param instance_count  Number of instances to draw.
** @param first_vertex    Index of the first vertex.
** @param first_instance  Index of the first instance.
*/
RT_API void rtCmdDrawInstanced(rt_command_buffer command_buffer, usize vertex_count, usize instance_count, usize first_vertex, usize first_instance);

/*!
** @brief Record an indexed draw.
**
** @param command_buffer  Command buffer being recorded.
** @param index_count     Number of indices to draw.
** @param first_index     Index of the first index element.
** @param vertex_offset   Added to every vertex index.
*/
RT_API void rtCmdDrawIndexed(rt_command_buffer command_buffer, usize index_count, usize first_index, usize vertex_offset);

/*!
** @brief Record an indexed instanced draw.
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
** Submissions through one queue execute in submission order.
**
** @param capability  Required queue capability.
** @return New queue, or NULL when the requested capability is unavailable.
*/
RT_API rt_queue rtQueueCreate(enum rt_queue_capability capability);

/*!
** @brief Destroy a queue.
**
** Destroying a queue prevents future submissions through it. Timepoints
** returned by earlier submissions remain usable until @ref rtUnload.
**
** @param queue  Queue to destroy.
*/
RT_API void rtQueueDestroy(rt_queue queue);

/*!
** @brief Make a queue's next submission wait for a timepoint.
**
** The wait applies to the next submission through @p queue and is consumed by
** that submission and does not block the CPU.
** Use @ref rtCmdBufferBarrier or @ref rtCmdTextureBarrier for a resource
** visibility dependency at a particular point within one command buffer.
**
** @param queue      Queue whose next submission depends on @p timepoint.
** @param timepoint  Completion point required before that submission runs.
*/
RT_API void rtQueueWait(rt_queue queue, rt_timepoint timepoint);

/*!
** @brief Submit a completed command buffer to a queue.
**
** A completed command buffer may be submitted again or reset and recorded
** with different commands.
**
** The returned timepoint signals once this submission has completed.
**
** @param queue           Queue receiving the completed command buffer.
** @param command_buffer  Completed command buffer.
**
** @return Signal that fires when this submission has completed on the GPU.
**
** @error RT_DEVICE_LOST           The device was lost while queuing the
**                                 submission.
** @error RT_OUT_OF_DEVICE_MEMORY  Device allocation failed while queuing
**                                 the submission.
*/
RT_API rt_timepoint rtQueueSubmit(rt_queue queue, rt_command_buffer command_buffer);

/*!
** @brief Flush submitted work from a queue.
**
** Makes work previously submitted through @p queue available for execution.
**
** @param queue  Queue to flush.
**
** @return Signal that fires when all flushed work has completed on the GPU.
**
** @error RT_DEVICE_LOST           The device was lost while flushing.
** @error RT_OUT_OF_DEVICE_MEMORY  Device allocation failed during the
**                                 flush.
*/
RT_API rt_timepoint rtQueueFlush(rt_queue queue);

/*!
** @brief Block the CPU until a timepoint is reached.
**
** Returns once the GPU has signaled @p timepoint. A zero-initialized or
** already-reached timepoint returns immediately.
**
** @param timepoint  Timepoint to wait for.
*/
RT_API void rtTimepointWait(rt_timepoint timepoint);

/*!
** @brief Non-blocking check of whether a timepoint has been reached.
**
** Polls the current state of @p timepoint. A zero-initialized timepoint is
** always considered reached.
**
** @param timepoint  Timepoint to query.
** @return true if the timepoint has been signaled (or is the always-reached
**         sentinel); false otherwise.
*/
RT_API bool rtTimepointReached(rt_timepoint timepoint);

/*===============================================================================================*/
/* Framebuffer                                                                                   */
/*===============================================================================================*/

/*!
** @brief Create a framebuffer.
**
** @return New framebuffer handle, or NULL on failure.
*/
RT_API rt_framebuffer rtFramebufferCreate(void);

/*!
** @brief Destroy a framebuffer.
**
** @param framebuffer  Framebuffer to destroy.
*/
RT_API void rtFramebufferDestroy(rt_framebuffer framebuffer);

/*!
** @brief Return the texture view attached to a fragment-output location.
**
** @param framebuffer  Framebuffer to query.
** @param location     Fragment-output location obtained from the graphics program,
**                     or @ref RT_LOCATION_ZERO for color attachment zero.
** @return The bound texture view, or NULL if @p location is empty.
*/
RT_API rt_texture_view rtFramebufferColorView(rt_framebuffer framebuffer, rt_location location);

/*!
** @brief Attach (or detach) a texture view at a fragment-output location.
**
** @param framebuffer  Framebuffer to update.
** @param view         Texture view to attach, or NULL to clear @p location.
** @param location     Fragment-output location obtained from the graphics program,
**                     or @ref RT_LOCATION_ZERO for color attachment zero.
*/
RT_API void rtFramebufferSetColorView(rt_framebuffer framebuffer, rt_texture_view view, rt_location location);

/*!
** @brief Attach (or detach) the depth texture view of a framebuffer.
**
** @param framebuffer  Framebuffer to update.
** @param view         Texture view of a depth or depth-stencil format to
**                     attach, or NULL to remove the depth attachment.
*/
RT_API void rtFramebufferSetDepthView(rt_framebuffer framebuffer, rt_texture_view view);

/*!
** @brief Attach (or detach) the stencil texture view of a framebuffer.
**
** @p view must have a stencil or depth-stencil format. When one depth-stencil
** texture supplies both attachments, bind the same view with
** @ref rtFramebufferSetDepthView and this function.
**
** @param framebuffer  Framebuffer to update.
** @param view         Texture view to attach, or NULL to remove the stencil
**                     attachment.
*/
RT_API void rtFramebufferSetStencilView(rt_framebuffer framebuffer, rt_texture_view view);

/*===============================================================================================*/
/* Graphics program                                                                              */
/*===============================================================================================*/

/*!
** @brief Create a graphics program.
**
** @return New graphics program handle, or NULL on failure.
*/
RT_API rt_graphics_program rtGraphicsProgramCreate(void);

/*!
** @brief Destroy a graphics program.
**
** @param program  Graphics program to destroy.
*/
RT_API void rtGraphicsProgramDestroy(rt_graphics_program program);

/*!
** @brief Set the vertex input layout used by a graphics program.
**
** @p layout describes vertex-buffer inputs and their attributes. Each input
** defines its attribute array, stride, and rate. Each attribute names one
** shader input and defines its byte offset and format. Attributes in one
** input are interleaved in one buffer; define multiple inputs to use multiple
** buffers. After finalization, query an input with
** @ref rtGraphicsProgramInputLocation and bind it with
** @ref rtCmdVertexBuffer. The structure and its inner arrays are copied; the
** caller retains ownership.
**
** Pass NULL for @p layout if the program does not read vertex attributes
** (e.g. vertex IDs only).
**
** @param program  Graphics program to configure.
** @param layout   Vertex layout description, or NULL.
*/
RT_API void rtGraphicsProgramSetLayout(rt_graphics_program program, const rt_vertex_layout* layout);

/*!
** @brief Provide the shader code for a graphics program.
**
** @p data points to a compiled RTSL Program binary blob of @p size bytes.
**
** Calling this on a finalized program is invalid. Create a new program to
** change its source.
**
** @param program  Graphics program to configure.
** @param data     Pointer to an RTSL Program binary.
** @param size     Size of @p data in bytes.
*/
RT_API void rtGraphicsProgramSetSource(rt_graphics_program program, const u08* data, usize size);

/*!
** @brief Set rasterization state for a graphics program.
**
** @param program     Graphics program to configure.
** @param cull_mode   Which triangle faces are culled.
** @param front_face  Winding order considered front-facing.
** @param fill_mode   Triangle fill mode (solid or wireframe).
*/
RT_API void rtGraphicsProgramSetRasterState(rt_graphics_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode);

/*!
** @brief Set color-blend state for a graphics program.
**
** When @p enabled is false, blending is disabled and the remaining
** factor/op arguments are ignored - fragment shader output is written
** through unmodified.
**
** When @p enabled is true, the output color is blended as
** `color_op(src_color * src, dst_color * dst)` and the output alpha as
** `alpha_op(src_alpha * src, dst_alpha * dst)`.
**
** @param program     Graphics program to configure.
** @param enabled     Master enable for color blending.
** @param src_color   Source factor for the RGB channels.
** @param dst_color   Destination factor for the RGB channels.
** @param color_op    Combining operation for the RGB channels.
** @param src_alpha   Source factor for the alpha channel.
** @param dst_alpha   Destination factor for the alpha channel.
** @param alpha_op    Combining operation for the alpha channel.
*/
RT_API void rtGraphicsProgramSetBlendState(rt_graphics_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op);

/*!
** @brief Finalize a graphics program for use in draw commands.
**
** Locks in the current configuration (layout, source, raster/blend state)
** for draw commands. After this call, the configuration setters
** (@ref rtGraphicsProgramSetLayout, @ref rtGraphicsProgramSetSource,
** @ref rtGraphicsProgramSetRasterState,
** @ref rtGraphicsProgramSetBlendState) may
** not be called again.
**
** The location queries may only be called on a finalized program. A program
** must be finalized before being bound by
** @ref rtCmdUseGraphicsProgram.
**
** @param program  Graphics program to finalize.
**
** @error RT_SHADER_COMPILATION_FAILED  A shader stage failed to compile.
** @error RT_SHADER_LINK_FAILED         The compiled stages failed to link
**                                      into a pipeline.
** @error RT_OUT_OF_HOST_MEMORY         Host allocation failed during
**                                      finalize.
** @error RT_OUT_OF_DEVICE_MEMORY       Device allocation failed during
**                                      finalize.
*/
RT_API void rtGraphicsProgramFinalize(rt_graphics_program program);

/*!
** @brief Look up a named uniform-resource location.
**
** Use the returned location with @ref rtCmdBindBuffer for a uniform or
** storage buffer, or with @ref rtCmdBindTexture for a combined
** texture/sampler resource.
**
** @param program  Finalized graphics program to query.
** @param name     Null-terminated uniform-resource name.
** @return Location handle, or NULL if @p name is not a uniform resource.
*/
RT_API rt_location rtGraphicsProgramUniformLocation(rt_graphics_program program, const char* name);

/*!
** @brief Look up a vertex-buffer input location.
**
** An input location represents the @ref rt_vertex_input whose attributes
** exactly match @p attributes. Use it with @ref rtCmdVertexBuffer to bind
** the buffer that supplies every attribute in that input.
**
** @param program  Finalized graphics program to query.
** @param attributes      Attribute array from one @ref rt_vertex_input.
** @param attribute_count Number of attributes in @p attributes.
** @return Location handle, or NULL if no input has those attributes.
*/
RT_API rt_location rtGraphicsProgramInputLocation(rt_graphics_program program, const rt_vertex_attribute* attributes, usize attribute_count);

/*!
** @brief Look up a named fragment-output location.
**
** Use the returned location with @ref rtFramebufferSetColorView. When the
** fragment shader returns one color value rather than a struct, its output is
** always color location 0; pass NULL for @p name to query that location.
** When the fragment return is a struct, query each named field.
**
** @param program  Finalized graphics program to query.
** @param name     Null for the single unnamed color output, or a
**                 null-terminated fragment-output field name.
** @return Location handle, or NULL if @p name is not a fragment output.
*/
RT_API rt_location rtGraphicsProgramOutputLocation(rt_graphics_program program, const char* name);

/*===============================================================================================*/
/* Buffer                                                                                        */
/*===============================================================================================*/

/*!
** @brief Create a buffer.
**
** @return New buffer handle, or NULL on failure.
*/
RT_API rt_buffer rtBufferCreate(void);

/*!
** @brief Destroy a buffer.
**
** @param buffer Buffer to destroy.
*/
RT_API void rtBufferDestroy(rt_buffer buffer);

/*!
** @brief Resize a buffer's storage.
**
** Resizes @p buffer to @p size bytes with @p memory_type. Its contents after
** resizing are undefined. Upload bytes with @ref rtCmdBufferData.
**
** Concurrent resizes of one buffer require caller synchronization.
**
** @param buffer       Buffer to resize.
** @param memory_type  Memory type for the resized storage.
** @param size         New logical storage size in bytes.
*/
RT_API void rtBufferResize(rt_buffer buffer, enum rt_memory_type memory_type, usize size);

/*!
** @brief Copy bytes out of a buffer into application memory.
**
** @param buffer  Source buffer.
** @param range   Byte range to copy.
** @param data       Destination bytes.
** @param data_size  Size of @p data in bytes.
*/
RT_API void rtBufferRead(rt_buffer buffer, rt_buffer_range range, u08* data, usize data_size);

/*!
** @brief Map a host-memory buffer range into application memory.
**
** Only buffers resized with @ref RT_HOST_MEMORY can be mapped. The returned
** bytes remain mapped until @ref rtBufferUnmap is called for @p buffer.
**
** @param buffer  Host-memory buffer to map.
** @param range   Byte range to map.
** @return Pointer to the first mapped byte, or NULL on failure.
*/
RT_API u08* rtBufferMap(rt_buffer buffer, rt_buffer_range range);

/*!
** @brief End a buffer mapping.
**
** @param buffer  Buffer previously passed to @ref rtBufferMap.
*/
RT_API void rtBufferUnmap(rt_buffer buffer);

/*!
** @brief Record an upload into an existing buffer range.
**
** The command copies @p range.size bytes from @p data. It does not resize the
** buffer. Recording disjoint ranges concurrently is permitted; overlapping
** ranges require caller synchronization.
**
** Record @ref rtCmdBufferBarrier before a later command reads the updated
** range.
**
** @param command_buffer  Command buffer being recorded.
** @param buffer          Destination buffer.
** @param range           Byte range to upload.
** @param data            Source bytes.
*/
RT_API void rtCmdBufferData(rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, const u08* data);

/*!
** @brief Record a copy between buffer ranges.
**
** Record @ref rtCmdBufferBarrier before a later command reads @p dst_range.
**
** @param command_buffer  Command buffer being recorded.
** @param src             Source buffer.
** @param src_range       Source byte range.
** @param dst             Destination buffer.
** @param dst_range       Destination byte range.
*/
RT_API void rtCmdBufferCopy(rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_buffer dst, rt_buffer_range dst_range);

/*!
** @brief Record a copy from a buffer range into a texture range.
**
** @param command_buffer  Command buffer being recorded.
** @param src             Source buffer.
** @param src_range       Source byte range.
** @param dst             Destination texture.
** @param dst_range       Destination subresource and texel range.
*/
RT_API void rtCmdBufferCopyToTexture(rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_texture dst, rt_texture_range dst_range);

/*!
** @brief Make prior buffer accesses visible to later buffer accesses.
**
** Establishes a dependency for @p range of @p buffer. Operations before this
** command matching @p src must complete, and their writes must be visible,
** before later operations matching @p dst access that range. For example,
** `{ RT_STAGE_COMPUTE, RT_ACCESS_WRITE }` to
** `{ RT_STAGE_VERTEX, RT_ACCESS_READ }` makes compute writes available to
** vertex reads.
**
** A barrier describes a caller-selected dependency; it does not choose an
** order for overlapping writes that the caller has not synchronized.
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
** @return New texture handle, or NULL on failure.
*/
RT_API rt_texture rtTextureCreate(void);

/*!
** @brief Destroy a texture.
**
** @param texture  Texture to destroy.
*/
RT_API void rtTextureDestroy(rt_texture texture);

/*!
** @brief Resize a texture's storage.
**
** Resizes @p texture to the specified type, format, extent, and mip count.
** Its contents after resizing are undefined. Upload texels with
** @ref rtCmdTextureData.
**
** Concurrent resizes of one texture require caller synchronization.
**
** @param texture    Texture to resize.
** @param type       Texture dimensionality.
** @param format     Pixel format.
** @param extent     Base-level extent in texels.
** @param mip_count  Number of mip levels.
*/
RT_API void rtTextureResize(rt_texture texture, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count);

/*!
** @brief Record a copy between texture ranges.
**
** Copies @p src_range into @p dst_range. Their aspects, mip counts, layer
** counts, and texel extents must match. Record @ref rtCmdTextureBarrier
** before later commands access either range.
**
** @param command_buffer  Command buffer being recorded.
** @param src             Source texture.
** @param src_range       Source subresource and texel range.
** @param dst             Destination texture.
** @param dst_range       Destination subresource and texel range.
*/
RT_API void rtCmdTextureCopy(rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_texture dst, rt_texture_range dst_range);

/*!
** @brief Record an upload into an existing texture range.
**
** Writes @p range texels from @p data into @p texture. The region must fit
** within storage previously defined by @ref rtTextureResize.
**
** Record @ref rtCmdTextureBarrier before later commands access the updated
** texel region.
**
** @param command_buffer  Command buffer being recorded.
** @param texture         Destination texture.
** @param range           Destination subresource and texel range.
** @param data            Tightly packed source texels.
*/
RT_API void rtCmdTextureData(rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, const u08* data);

/*!
** @brief Record a copy from a texture range into a buffer range.
**
** @param command_buffer  Command buffer being recorded.
** @param src             Source texture.
** @param src_range       Source subresource and texel range.
** @param dst             Destination buffer.
** @param dst_range       Destination byte range.
*/
RT_API void rtCmdTextureCopyToBuffer(rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_buffer dst, rt_buffer_range dst_range);

/*!
** @brief Make prior texture accesses visible to later texture accesses.
**
** Establishes a dependency for @p range of @p texture. Operations before
** this command matching @p src complete and make their writes visible before
** later operations matching @p dst access that range. Use
** @ref rtCmdBufferBarrier for a buffer range.
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
** @return New texture view handle, or NULL on failure.
*/
RT_API rt_texture_view rtTextureViewCreate(void);

/*!
** @brief Destroy a texture view.
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
** @brief Bind a texture to an existing texture view.
**
** @param texture_view  Texture view to update.
** @param texture       Texture to view through @p texture_view.
*/
RT_API void rtTextureViewSetTexture(rt_texture_view texture_view, rt_texture texture);

/*!
** @brief Set the filtering used when sampling through a texture view.
**
** @param texture_view  Texture view to update.
** @param mag_filter    Filter used when the footprint covers less than one
**                      texel (magnification).
** @param min_filter    Filter used when the footprint covers more than one
**                      texel (minification).
** @param mip_filter    Filter applied between mip levels, or
**                      RT_MIP_FILTER_NONE to disable mip sampling.
*/
RT_API void rtTextureViewSetFilter(rt_texture_view texture_view, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter);

/*!
** @brief Set the per-axis address modes used when sampling through a view.
**
** Controls how out-of-range texture coordinates are resolved on each axis
** (clamp, repeat, mirror).
**
** @param texture_view  Texture view to update.
** @param address_u     Address mode for the U (X) axis.
** @param address_v     Address mode for the V (Y) axis.
** @param address_w     Address mode for the W (Z) axis. Ignored for 1D/2D
**                      textures.
*/
RT_API void rtTextureViewSetAddress(rt_texture_view texture_view, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w);

/*!
** @brief Set the maximum anisotropy used when sampling through a view.
**
** @param texture_view    Texture view to update.
** @param max_anisotropy  Maximum anisotropic sample count. 0 disables
**                        anisotropic filtering.
*/
RT_API void rtTextureViewSetAnisotropy(rt_texture_view texture_view, usize max_anisotropy);

/*!
** @brief Set the LOD selection state used when sampling through a view.
**
** @param texture_view  Texture view to update.
** @param min_lod       Lower clamp on the computed mip level.
** @param max_lod       Upper clamp on the computed mip level.
** @param lod_bias      Bias added to the computed mip level before clamping.
*/
RT_API void rtTextureViewSetLod(rt_texture_view texture_view, f32 min_lod, f32 max_lod, f32 lod_bias);

/*!
** @brief Read texels from a texture view into application memory.
**
** @param texture_view  Texture view to read.
** @param range         Texture range to read.
** @param data          Destination bytes.
** @param data_size     Size of @p data in bytes.
*/
RT_API void rtTextureViewRead(rt_texture_view texture_view, rt_texture_range range, u08* data, usize data_size);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

#endif /* !RT_TYPES_ONLY */

#define RT_CORE_PROCEDURES(X) \
	X( void                 , rtInit                             , ( const char* const* features, usize feature_count                                                                                                                                                                                  ) , ( features, feature_count                                                                 ) ) \
	X( void                 , rtExit                             , ( void                                                                                                                                                                                                                              ) , (                                                                                         ) ) \
	X( void                 , rtSetOutput                        , ( rt_output output, void* user_data                                                                                                                                                                                                 ) , ( output, user_data                                                                       ) ) \
	X( enum rt_error        , rtError                            , ( void                                                                                                                                                                                                                              ) , (                                                                                         ) ) \
	X( const char*          , rtErrorMessage                     , ( void                                                                                                                                                                                                                              ) , (                                                                                         ) ) \
	X( void                 , rtClearError                       , ( void                                                                                                                                                                                                                              ) , (                                                                                         ) ) \
	X( const char*          , rtGetName                          , ( void                                                                                                                                                                                                                              ) , (                                                                                         ) ) \
	X( enum rt_format_usage , rtQueryFormatCapabilities          , ( enum rt_format format                                                                                                                                                                                                             ) , ( format                                                                                  ) ) \
	X( rt_command_buffer    , rtCommandBufferCreate              , ( void                                                                                                                                                                                                                              ) , (                                                                                         ) ) \
	X( void                 , rtCommandBufferDestroy             , ( rt_command_buffer command_buffer                                                                                                                                                                                                  ) , ( command_buffer                                                                          ) ) \
	X( void                 , rtCommandBufferReset               , ( rt_command_buffer command_buffer                                                                                                                                                                                                  ) , ( command_buffer                                                                          ) ) \
	X( void                 , rtCommandBufferBegin               , ( rt_command_buffer command_buffer                                                                                                                                                                                                  ) , ( command_buffer                                                                          ) ) \
	X( void                 , rtCommandBufferContinue            , ( rt_command_buffer command_buffer                                                                                                                                                                                                  ) , ( command_buffer                                                                          ) ) \
	X( void                 , rtCommandBufferContinueRendering   , ( rt_command_buffer command_buffer                                                                                                                                                                                                  ) , ( command_buffer                                                                          ) ) \
	X( void                 , rtCommandBufferEnd                 , ( rt_command_buffer command_buffer                                                                                                                                                                                                  ) , ( command_buffer                                                                          ) ) \
	X( void                 , rtCmdExecute                       , ( rt_command_buffer command_buffer, rt_command_buffer secondary                                                                                                                                                                     ) , ( command_buffer, secondary                                                               ) ) \
	X( void                 , rtCmdBeginRendering                , ( rt_command_buffer command_buffer, rt_framebuffer framebuffer                                                                                                                                                                      ) , ( command_buffer, framebuffer                                                             ) ) \
	X( void                 , rtCmdClearColor                    , ( rt_command_buffer command_buffer, rt_location location, f32 r, f32 g, f32 b, f32 a                                                                                                                                                ) , ( command_buffer, location, r, g, b, a                                                    ) ) \
	X( void                 , rtCmdClearDepth                    , ( rt_command_buffer command_buffer, f32 depth                                                                                                                                                                                       ) , ( command_buffer, depth                                                                   ) ) \
	X( void                 , rtCmdClearStencil                  , ( rt_command_buffer command_buffer, usize stencil                                                                                                                                                                                   ) , ( command_buffer, stencil                                                                 ) ) \
	X( void                 , rtCmdClear                         , ( rt_command_buffer command_buffer, enum rt_clear_flag attachments                                                                                                                                                                  ) , ( command_buffer, attachments                                                             ) ) \
	X( void                 , rtCmdSetViewport                   , ( rt_command_buffer command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth                                                                                                                       ) , ( command_buffer, x, y, width, height, min_depth, max_depth                               ) ) \
	X( void                 , rtCmdSetScissor                    , ( rt_command_buffer command_buffer, usize x, usize y, usize width, usize height                                                                                                                                                     ) , ( command_buffer, x, y, width, height                                                     ) ) \
	X( void                 , rtCmdEndRendering                  , ( rt_command_buffer command_buffer                                                                                                                                                                                                  ) , ( command_buffer                                                                          ) ) \
	X( void                 , rtCmdUseGraphicsProgram            , ( rt_command_buffer command_buffer, rt_graphics_program program                                                                                                                                                                     ) , ( command_buffer, program                                                                 ) ) \
	X( void                 , rtCmdBindBuffer                    , ( rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range                                                                                                                                   ) , ( command_buffer, location, buffer, range                                                 ) ) \
	X( void                 , rtCmdBindTexture                   , ( rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view                                                                                                                                              ) , ( command_buffer, location, texture_view                                                  ) ) \
	X( void                 , rtCmdVertexBuffer                  , ( rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, rt_buffer_range range                                                                                                                                   ) , ( command_buffer, location, buffer, range                                                 ) ) \
	X( void                 , rtCmdIndexBuffer                   , ( rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, enum rt_index_format format                                                                                                                            ) , ( command_buffer, buffer, range, format                                                   ) ) \
	X( void                 , rtCmdDraw                          , ( rt_command_buffer command_buffer, usize vertex_count, usize first_vertex                                                                                                                                                          ) , ( command_buffer, vertex_count, first_vertex                                              ) ) \
	X( void                 , rtCmdDrawInstanced                 , ( rt_command_buffer command_buffer, usize vertex_count, usize instance_count, usize first_vertex, usize first_instance                                                                                                              ) , ( command_buffer, vertex_count, instance_count, first_vertex, first_instance              ) ) \
	X( void                 , rtCmdDrawIndexed                   , ( rt_command_buffer command_buffer, usize index_count, usize first_index, usize vertex_offset                                                                                                                                       ) , ( command_buffer, index_count, first_index, vertex_offset                                 ) ) \
	X( void                 , rtCmdDrawIndexedInstanced          , ( rt_command_buffer command_buffer, usize index_count, usize instance_count, usize first_index, usize vertex_offset, usize first_instance                                                                                           ) , ( command_buffer, index_count, instance_count, first_index, vertex_offset, first_instance ) ) \
	X( rt_queue             , rtQueueCreate                      , ( enum rt_queue_capability capability                                                                                                                                                                                               ) , ( capability                                                                              ) ) \
	X( void                 , rtQueueDestroy                     , ( rt_queue queue                                                                                                                                                                                                                    ) , ( queue                                                                                   ) ) \
	X( void                 , rtQueueWait                        , ( rt_queue queue, rt_timepoint timepoint                                                                                                                                                                                            ) , ( queue, timepoint                                                                        ) ) \
	X( rt_timepoint         , rtQueueSubmit                      , ( rt_queue queue, rt_command_buffer command_buffer                                                                                                                                                                                  ) , ( queue, command_buffer                                                                   ) ) \
	X( rt_timepoint         , rtQueueFlush                       , ( rt_queue queue                                                                                                                                                                                                                    ) , ( queue                                                                                   ) ) \
	X( void                 , rtTimepointWait                    , ( rt_timepoint timepoint                                                                                                                                                                                                            ) , ( timepoint                                                                               ) ) \
	X( bool                 , rtTimepointReached                 , ( rt_timepoint timepoint                                                                                                                                                                                                            ) , ( timepoint                                                                               ) ) \
	X( rt_framebuffer       , rtFramebufferCreate                , ( void                                                                                                                                                                                                                              ) , (                                                                                         ) ) \
	X( void                 , rtFramebufferDestroy               , ( rt_framebuffer framebuffer                                                                                                                                                                                                        ) , ( framebuffer                                                                             ) ) \
	X( rt_texture_view      , rtFramebufferColorView             , ( rt_framebuffer framebuffer, rt_location location                                                                                                                                                                                  ) , ( framebuffer, location                                                                   ) ) \
	X( void                 , rtFramebufferSetColorView          , ( rt_framebuffer framebuffer, rt_texture_view view, rt_location location                                                                                                                                                            ) , ( framebuffer, view, location                                                             ) ) \
	X( void                 , rtFramebufferSetDepthView          , ( rt_framebuffer framebuffer, rt_texture_view view                                                                                                                                                                                  ) , ( framebuffer, view                                                                       ) ) \
	X( void                 , rtFramebufferSetStencilView        , ( rt_framebuffer framebuffer, rt_texture_view view                                                                                                                                                                                  ) , ( framebuffer, view                                                                       ) ) \
	X( rt_graphics_program  , rtGraphicsProgramCreate            , ( void                                                                                                                                                                                                                              ) , (                                                                                         ) ) \
	X( void                 , rtGraphicsProgramDestroy           , ( rt_graphics_program program                                                                                                                                                                                                       ) , ( program                                                                                 ) ) \
	X( void                 , rtGraphicsProgramSetLayout         , ( rt_graphics_program program, const rt_vertex_layout* layout                                                                                                                                                                       ) , ( program, layout                                                                         ) ) \
	X( void                 , rtGraphicsProgramSetSource         , ( rt_graphics_program program, const u08* data, usize size                                                                                                                                                                          ) , ( program, data, size                                                                     ) ) \
	X( void                 , rtGraphicsProgramSetRasterState    , ( rt_graphics_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode                                                                                                              ) , ( program, cull_mode, front_face, fill_mode                                               ) ) \
	X( void                 , rtGraphicsProgramSetBlendState     , ( rt_graphics_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op   ) , ( program, enabled, src_color, dst_color, color_op, src_alpha, dst_alpha, alpha_op        ) ) \
	X( void                 , rtGraphicsProgramFinalize          , ( rt_graphics_program program                                                                                                                                                                                                       ) , ( program                                                                                 ) ) \
	X( rt_location          , rtGraphicsProgramUniformLocation   , ( rt_graphics_program program, const char* name                                                                                                                                                                                     ) , ( program, name                                                                           ) ) \
	X( rt_location          , rtGraphicsProgramInputLocation     , ( rt_graphics_program program, const rt_vertex_attribute* attributes, usize attribute_count                                                                                                                                          ) , ( program, attributes, attribute_count                                                    ) ) \
	X( rt_location          , rtGraphicsProgramOutputLocation    , ( rt_graphics_program program, const char* name                                                                                                                                                                                     ) , ( program, name                                                                           ) ) \
	X( rt_buffer            , rtBufferCreate                     , ( void                                                                                                                                                                                                                              ) , (                                                                                         ) ) \
	X( void                 , rtBufferDestroy                    , ( rt_buffer buffer                                                                                                                                                                                                                  ) , ( buffer                                                                                  ) ) \
	X( void                 , rtBufferResize                     , ( rt_buffer buffer, enum rt_memory_type memory_type, usize size                                                                                                                                                                     ) , ( buffer, memory_type, size                                                               ) ) \
	X( void                 , rtBufferRead                       , ( rt_buffer buffer, rt_buffer_range range, u08* data, usize data_size                                                                                                                                                               ) , ( buffer, range, data, data_size                                                          ) ) \
	X( u08*                 , rtBufferMap                        , ( rt_buffer buffer, rt_buffer_range range                                                                                                                                                                                           ) , ( buffer, range                                                                           ) ) \
	X( void                 , rtBufferUnmap                      , ( rt_buffer buffer                                                                                                                                                                                                                  ) , ( buffer                                                                                  ) ) \
	X( void                 , rtCmdBufferData                    , ( rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, const u08* data                                                                                                                                        ) , ( command_buffer, buffer, range, data                                                     ) ) \
	X( void                 , rtCmdBufferCopy                    , ( rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_buffer dst, rt_buffer_range dst_range                                                                                                              ) , ( command_buffer, src, src_range, dst, dst_range                                          ) ) \
	X( void                 , rtCmdBufferCopyToTexture           , ( rt_command_buffer command_buffer, rt_buffer src, rt_buffer_range src_range, rt_texture dst, rt_texture_range dst_range                                                                                                            ) , ( command_buffer, src, src_range, dst, dst_range                                          ) ) \
	X( void                 , rtCmdBufferBarrier                 , ( rt_command_buffer command_buffer, rt_buffer buffer, rt_buffer_range range, rt_access src, rt_access dst                                                                                                                           ) , ( command_buffer, buffer, range, src, dst                                                 ) ) \
	X( rt_texture           , rtTextureCreate                    , ( void                                                                                                                                                                                                                              ) , (                                                                                         ) ) \
	X( void                 , rtTextureDestroy                   , ( rt_texture texture                                                                                                                                                                                                                ) , ( texture                                                                                 ) ) \
	X( void                 , rtTextureResize                    , ( rt_texture texture, enum rt_texture_type type, enum rt_format format, rt_extent_3d extent, usize mip_count                                                                                                                        ) , ( texture, type, format, extent, mip_count                                                ) ) \
	X( void                 , rtCmdTextureCopy                   , ( rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_texture dst, rt_texture_range dst_range                                                                                                          ) , ( command_buffer, src, src_range, dst, dst_range                                          ) ) \
	X( void                 , rtCmdTextureData                   , ( rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, const u08* data                                                                                                                                     ) , ( command_buffer, texture, range, data                                                    ) ) \
	X( void                 , rtCmdTextureCopyToBuffer           , ( rt_command_buffer command_buffer, rt_texture src, rt_texture_range src_range, rt_buffer dst, rt_buffer_range dst_range                                                                                                            ) , ( command_buffer, src, src_range, dst, dst_range                                          ) ) \
	X( void                 , rtCmdTextureBarrier                , ( rt_command_buffer command_buffer, rt_texture texture, rt_texture_range range, rt_access src, rt_access dst                                                                                                                        ) , ( command_buffer, texture, range, src, dst                                                ) ) \
	X( rt_texture_view      , rtTextureViewCreate                , ( void                                                                                                                                                                                                                              ) , (                                                                                         ) ) \
	X( void                 , rtTextureViewDestroy               , ( rt_texture_view texture_view                                                                                                                                                                                                      ) , ( texture_view                                                                            ) ) \
	X( rt_extent_3d         , rtTextureViewExtent                , ( rt_texture_view texture_view                                                                                                                                                                                                      ) , ( texture_view                                                                            ) ) \
	X( void                 , rtTextureViewSetTexture            , ( rt_texture_view texture_view, rt_texture texture                                                                                                                                                                                  ) , ( texture_view, texture                                                                   ) ) \
	X( void                 , rtTextureViewSetFilter             , ( rt_texture_view texture_view, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter                                                                                                                 ) , ( texture_view, mag_filter, min_filter, mip_filter                                        ) ) \
	X( void                 , rtTextureViewSetAddress            , ( rt_texture_view texture_view, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w                                                                                                      ) , ( texture_view, address_u, address_v, address_w                                           ) ) \
	X( void                 , rtTextureViewSetAnisotropy         , ( rt_texture_view texture_view, usize max_anisotropy                                                                                                                                                                                ) , ( texture_view, max_anisotropy                                                            ) ) \
	X( void                 , rtTextureViewSetLod                , ( rt_texture_view texture_view, f32 min_lod, f32 max_lod, f32 lod_bias                                                                                                                                                              ) , ( texture_view, min_lod, max_lod, lod_bias                                                ) ) \
	X( void                 , rtTextureViewRead                  , ( rt_texture_view texture_view, rt_texture_range range, u08* data, usize data_size                                                                                                                                                  ) , ( texture_view, range, data, data_size                                                    ) )
/* RT_CORE_PROCEDURES */

#if !defined(RT_TYPES_ONLY)

#define RT_DECLARE_CORE_PROCEDURE(return_type, name, parameters, arguments) \
	typedef return_type(*PFN_##name) parameters;                            \
	extern PFN_##name rt_##name;                                            \
	RT_API return_type name parameters { return rt_##name arguments; }
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4098) // warning C4098: 'function': 'void' function returning a value
#endif
RT_CORE_PROCEDURES(RT_DECLARE_CORE_PROCEDURE)
#if defined(_MSC_VER)
#pragma warning(pop) // warning C4098: 'function': 'void' function returning a value
#endif
#undef RT_DECLARE_CORE_PROCEDURE

#define RT_DECLARE_EXTENSION_PROCEDURE(return_type, name, parameters, arguments) \
	typedef return_type(*PFN_##name) parameters;                                 \
	extern PFN_##name rt_##name;                                                 \
	RT_API return_type name parameters { return rt_##name arguments; }

#ifdef __cplusplus
}
#endif

#endif /* !RT_TYPES_ONLY */

#include "rt_ext_swapchain.h"
#include "rt_ext_glfw.h"

/*!
** @section implementation_hints Implementation hints
**
** These notes are non-normative. They describe the intended shape of a
** Rutile implementation, not additional application requirements.
**
** An implementation can retain a Rutile-owned intermediate representation for
** a command buffer and translate it only when a queue submits the buffer. A
** queue can multiplex public queues onto fewer native execution queues while
** preserving each public queue's submission order.
**
** An implementation can retain source bytes for recorded uploads, stage them
** for a transfer, and maintain resource revisions so submitted readers retain
** the contents selected when their commands were recorded.
*/

#endif /* RUTILE_H */
