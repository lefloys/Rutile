#ifndef RUTILE_H
#define RUTILE_H

/*!
** @file rutile.h
** @brief Rutile public C API and dynamic loader.
**
** Rutile is a graphics abstraction. This header provides:
**   - The loader (@ref rtLoad, @ref rtUnload, @ref rtLoaded, @ref rtGetProc).
**     It selects a backend and optional layers for the current process.
**   - The core API surface itself (resources, command buffers, queues,
**     synchronization).
**
** @section threading Threading
** Any function in this API may be called concurrently. Calls that change the
** same resource state still require caller synchronization unless the
** function's contract explicitly permits disjoint ranges. Submissions through
** one @ref rt_queue are ordered; submissions concurrently issued without an
** established caller order execute in the implementation's serialization
** order.
**
** @section timepoints Timepoints
** Work that touches the GPU returns an @ref rt_timepoint: an opaque completion
** signal that fires once the operation has completed on the device. Pass it
** to @ref rtTimepointWait to block the CPU until completion, to
** @ref rtTimepointReached to poll, to @ref rtCmdWait to make later
** command-buffer work wait for it, or to @ref rtQueueWait to make a queue's
** next submission wait for it. Rutile does not define a timepoint's storage or
** its relationship to queues. A zero-initialized timepoint (`{ 0 }`) is the
** canonical "already reached" sentinel and is always safe to wait on; it
** records no dependency.
**
** @section transfers Transfer synchronization
** A transfer operation that returns a timepoint reserves the state it updates
** until that timepoint is waited where the next access occurs. @ref rtBufferData
** reserves its whole buffer because it redefines that buffer's storage;
** sub-range buffer transfers reserve only their affected range, and texture
** transfers reserve their affected texel region. CPU access requires
** @ref rtTimepointWait; GPU access requires @ref rtCmdWait in the consuming
** command buffer or @ref rtQueueWait before its submission. Concurrent
** sub-range transfers are permitted only when their affected ranges or texel
** regions do not overlap. Rutile does not verify that contract outside an
** optional validation layer.
**
** @section errors Error model
** Aside from @ref rtLoad and @ref rtLoadDevelopment,
** core API functions do not return error codes. They report failure
** out-of-band by recording an error code and message on the current
** thread, retrievable via @ref rtError and @ref rtErrorMessage and cleared
** by @ref rtClearError.
**
*/

#include <stddef.h>
#include <stdint.h>

typedef uint8_t u08;

#ifndef __cplusplus
#if defined(_MSC_VER) && !defined(__clang__)
typedef u08 bool;
#define false 0
#define true 1
#else
#include <stdbool.h>
#endif
#endif

#define RT_NULL_HANDLE NULL

#ifdef __cplusplus
extern "C" {
#endif

#define RT_FEATURE_PRESENTATION "RT_FEATURE_PRESENTATION"

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

enum rt_buffer_mode { RT_BUFFER_MODE__RESERVED = 0x7fffffff };
#define RT_BUFFER_STATIC ((enum rt_buffer_mode)1)
#define RT_BUFFER_DYNAMIC ((enum rt_buffer_mode)2)

enum rt_buffer_usage { RT_BUFFER_USAGE__RESERVED = 0x7fffffff };
#define RT_BUFFER_USAGE_NONE ((enum rt_buffer_usage)0x00)
#define RT_BUFFER_USAGE_STAGING ((enum rt_buffer_usage)0x01)
#define RT_BUFFER_USAGE_VERTEX ((enum rt_buffer_usage)0x02)
#define RT_BUFFER_USAGE_INDEX ((enum rt_buffer_usage)0x04)
#define RT_BUFFER_USAGE_UNIFORM ((enum rt_buffer_usage)0x08)
#define RT_BUFFER_USAGE_STORAGE ((enum rt_buffer_usage)0x10)
#define RT_BUFFER_USAGE_TRANSFER_SRC ((enum rt_buffer_usage)0x20)
#define RT_BUFFER_USAGE_TRANSFER_DST ((enum rt_buffer_usage)0x40)

enum rt_texture_type { RT_TEXTURE_TYPE__RESERVED = 0x7fffffff };
#define RT_TEXTURE_UNKNOWN ((enum rt_texture_type)0)
#define RT_TEXTURE_1D ((enum rt_texture_type)1)
#define RT_TEXTURE_2D ((enum rt_texture_type)2)
#define RT_TEXTURE_3D ((enum rt_texture_type)3)
#define RT_TEXTURE_1D_ARRAY ((enum rt_texture_type)4)
#define RT_TEXTURE_2D_ARRAY ((enum rt_texture_type)5)

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

typedef struct rt_texture_t* rt_texture;
typedef struct rt_texture_view_t* rt_texture_view;
typedef struct rt_buffer_t* rt_buffer;
typedef struct rt_graphics_program_t* rt_graphics_program;
typedef struct rt_location_t* rt_location;
typedef struct rt_command_buffer_t* rt_command_buffer;
typedef struct rt_framebuffer_t* rt_framebuffer;
typedef struct rt_queue_t* rt_queue;

typedef struct rt_vertex_stream {
	usize stride;
	enum rt_vertex_rate rate;
} rt_vertex_stream;

typedef struct rt_vertex_attribute {
	const char* name;
	usize stream;
	usize offset;
	enum rt_format format;
} rt_vertex_attribute;

typedef struct rt_vertex_layout {
	const rt_vertex_stream* streams;
	const rt_vertex_attribute* attributes;
	usize stream_count;
	usize attribute_count;
} rt_vertex_layout;

typedef struct rt_timepoint {
	u64 value;
} rt_timepoint;

typedef struct rt_extent_3d {
	usize width;
	usize height;
	usize depth;
} rt_extent_3d;

typedef void* rt_proc_t;

typedef struct rt_proc_chain {
	rt_proc_t (*get_proc)(const struct rt_proc_chain* chain, const char* name);
} rt_proc_chain;

typedef const char* (*PFN_rtLayerGetName)(void);
typedef void (*PFN_rtLayerSetNext)(rt_proc_chain next);
typedef void (*PFN_rtOutput)(const char* message, void* user_data);

#ifdef __cplusplus
}
#endif


#if !defined(RT_BUILD_DLL)
#define RT_API_PUBLIC
#elif defined(_WIN32)
#define RT_API_PUBLIC __declspec(dllexport)
#else
#define RT_API_PUBLIC __attribute__((visibility("default")))
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
** @param name  Null-terminated entry-point name (e.g. `"rtBufferData"`, or
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
RT_API void rtInit(const char* const* features, u32 feature_count);

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
RT_API void rtSetOutput(PFN_rtOutput output, void* user_data);

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
** @brief Define a buffer's storage and optionally upload its contents.
**
** May be called repeatedly. Each call sets @p size, @p mode, and @p usage. If
** @p data is non-NULL, its bytes are uploaded; otherwise, the storage contents
** are undefined.
**
** Each call is an object-mutating operation. Do not perform any other CPU or
** GPU operation that accesses @p buffer until the returned timepoint has been
** synchronized at that access point: use @ref rtTimepointWait for CPU access,
** or @ref rtCmdWait / @ref rtQueueWait for GPU access.
**
** @param buffer  Buffer to configure.
** @param mode    Storage/update strategy hint (static vs dynamic).
** @param usage   Bitset of RT_BUFFER_USAGE_* flags describing how the buffer
**                will be used (vertex, index, uniform, storage, transfer).
** @param size    Storage size in bytes.
** @param data    Optional source bytes copied into the buffer, or NULL.
**
** @return Signal that fires when the upload has completed on the GPU.
*/
RT_API rt_timepoint rtBufferData(rt_buffer buffer, enum rt_buffer_mode mode, enum rt_buffer_usage usage, usize size, const void* data);

/*!
** @brief Upload bytes into an existing range of a buffer.
**
** Uploads @p size bytes from @p data beginning at @p offset. Concurrent calls
** may update disjoint ranges of one buffer. The caller must synchronize
** overlapping ranges.
**
** Do not access the updated range on the CPU or GPU until the returned
** timepoint has been waited at that access point: use @ref rtTimepointWait for
** CPU access, or @ref rtCmdWait / @ref rtQueueWait for GPU access.
**
** @param buffer  Destination buffer.
** @param offset  Byte offset into @p buffer.
** @param size    Number of bytes to upload.
** @param data    Source bytes copied into the buffer range.
**
** @return Signal that fires when the upload has completed on the GPU.
*/
RT_API rt_timepoint rtBufferSubdata(rt_buffer buffer, usize offset, usize size, const void* data);

/*!
** @brief Copy bytes out of a buffer into application memory.
**
** @param buffer  Source buffer.
** @param offset  Byte offset into @p buffer.
** @param size    Number of bytes to copy.
** @param data    Destination memory, at least @p size bytes.
*/
RT_API void rtBufferRead(rt_buffer buffer, usize offset, usize size, void* data);

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
** @brief Copy one entire mip level of a texture to another texture's mip.
**
** The returned timepoint gates all CPU and GPU access to the source and
** destination mip texels. Before CPU access to those texels, call
** @ref rtTimepointWait with that timepoint. Before GPU work accesses them,
** either record @ref rtCmdWait in its command buffer or call @ref rtQueueWait
** on its queue before submission.
**
** @param src_texture  Source texture.
** @param src_mip      Mip level of @p src_texture to read from.
** @param dst_texture  Destination texture.
** @param dst_mip      Mip level of @p dst_texture to write to.
**
** @return Signal that fires when the copy has completed on the GPU.
*/
RT_API rt_timepoint rtTextureCopy(rt_texture src_texture, u32 src_mip, rt_texture dst_texture, u32 dst_mip);

/*!
** @brief (Re)define a mip level of a texture and optionally upload its data.
**
** Configures @p texture to hold a @p type texture of the given @p format,
** with storage for @p width times @p height times @p depth texels at mip
** level @p mip. If @p data is non-NULL, the level is initialized from tightly
** packed source data in @p format; otherwise, its contents are undefined.
**
** Texture dimensionality (@p type) and format are properties of the texture
** as a whole; supplying different values across calls on the same texture
** redefines it.
**
** The returned timepoint gates all CPU and GPU access to the texture mip
** texels set by this call. Before CPU access to those texels, call
** @ref rtTimepointWait with that timepoint. Before GPU work accesses them,
** either record @ref rtCmdWait in its command buffer or call @ref rtQueueWait
** on its queue before submission.
**
** @param texture  Texture to (re)configure.
** @param type     Texture dimensionality (1D, 2D, 3D, 1D array, 2D array).
** @param mip      Mip level to define.
** @param width    Storage width in texels.
** @param height   Storage height in texels.
** @param depth    Storage depth in texels (or array length, for array types).
** @param format   Pixel format.
** @param data     Optional source texel data, or NULL.
**
** @return Signal that fires when the upload has completed on the GPU.
*/
RT_API rt_timepoint rtTextureData(rt_texture texture, enum rt_texture_type type, u32 mip, u32 width, u32 height, u32 depth, enum rt_format format, const void* data);

/*!
** @brief Copy a sub-region of one texture mip to a sub-region of another.
**
** Copies @p extent texels from (@p src_texture, @p src_mip) at
** @p src_offset into (@p dst_texture, @p dst_mip) at @p dst_offset.
** Both regions must fit within their respective mip extents.
**
** The returned timepoint gates all CPU and GPU access to the source and
** destination texel regions. Before CPU access to those texels, call
** @ref rtTimepointWait with that timepoint. Before GPU work accesses them,
** either record @ref rtCmdWait in its command buffer or call @ref rtQueueWait
** on its queue before submission.
**
** @return Signal that fires when the copy has completed on the GPU.
*/
RT_API rt_timepoint rtTextureSubcopy(rt_texture src_texture, u32 src_mip, rt_extent_3d src_offset, rt_texture dst_texture, u32 dst_mip, rt_extent_3d dst_offset, rt_extent_3d extent);

/*!
** @brief Upload tightly packed texel data into a sub-region of a texture mip.
**
** Writes @p extent texels from @p data into mip level @p mip of @p texture
** starting at @p offset. The region must fit within the level extents
** previously defined by @ref rtTextureData.
**
** The returned timepoint gates all CPU and GPU access to the updated texel
** region. Before CPU access to those texels, call @ref rtTimepointWait with
** that timepoint. Before GPU work accesses them, either record @ref rtCmdWait
** in its command buffer or call @ref rtQueueWait on its queue before
** submission.
**
** @return Signal that fires when the upload has completed on the GPU.
*/
RT_API rt_timepoint rtTextureSubdata(rt_texture texture, u32 mip, rt_extent_3d offset, rt_extent_3d extent, const void* data);

/*!
** @brief Create a texture view.
**
** @return New texture view handle, or NULL on failure.
*/
RT_API rt_texture_view rtTextureViewCreate(void);

/*!
** @brief Bind a texture to an existing texture view.
**
** @param texture_view  Texture view to update.
** @param texture       Texture to view through @p texture_view.
*/
RT_API void rtTextureViewBind(rt_texture_view texture_view, rt_texture texture);

/*!
** @brief Destroy a texture view.
**
** @param texture_view  Texture view to destroy.
*/
RT_API void rtTextureViewDestroy(rt_texture_view texture_view);

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
RT_API void rtTextureViewFilter(rt_texture_view texture_view, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter);

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
RT_API void rtTextureViewAddress(rt_texture_view texture_view, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w);

/*!
** @brief Set the maximum anisotropy used when sampling through a view.
**
** @param texture_view    Texture view to update.
** @param max_anisotropy  Maximum anisotropic sample count. 0 maps to the
**                        backend default (no anisotropy).
*/
RT_API void rtTextureViewAnisotropy(rt_texture_view texture_view, u32 max_anisotropy);

/*!
** @brief Set the LOD selection state used when sampling through a view.
**
** @param texture_view  Texture view to update.
** @param min_lod       Lower clamp on the computed mip level.
** @param max_lod       Upper clamp on the computed mip level.
** @param lod_bias      Bias added to the computed mip level before clamping.
*/
RT_API void rtTextureViewLod(rt_texture_view texture_view, f32 min_lod, f32 max_lod, f32 lod_bias);

/*!
** @brief Copy the contents seen through a texture view into a buffer.
**
** The returned timepoint gates all CPU and GPU access to the source view
** texels and destination buffer bytes. Before CPU access to that data, call
** @ref rtTimepointWait with that timepoint. Before GPU work accesses it,
** either record @ref rtCmdWait in its command buffer or call @ref rtQueueWait
** on its queue before submission.
**
** @param texture_view  Source texture view.
** @param buffer        Destination buffer, large enough for the view's texels.
**
** @return Signal that fires when the copy has completed on the GPU.
**
** @error RT_IMPROPER_USAGE  Destination buffer is too small.
*/
RT_API rt_timepoint rtTextureViewCopyToBuffer(rt_texture_view texture_view, rt_buffer buffer);

/*!
** @brief Return the texel extent visible through a texture view.
**
** Reports the width/height/depth of the texture (or selected mip) seen
** through @p texture_view, in texels.
**
** @param texture_view  Texture view to query.
** @return The view's extent.
*/
RT_API rt_extent_3d rtTextureViewExtent(rt_texture_view texture_view);

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
** @brief Return the texture view currently attached to a color slot.
**
** @param framebuffer  Framebuffer to query.
** @param slot         Color attachment slot index.
** @return The bound texture view, or NULL if @p slot is empty.
*/
RT_API rt_texture_view rtFramebufferColorView(rt_framebuffer framebuffer, u32 slot);

/*!
** @brief Attach (or detach) a texture view to a color slot of a framebuffer.
**
** @param framebuffer  Framebuffer to update.
** @param slot         Color attachment slot index.
** @param view         Texture view to attach, or NULL to clear the slot.
*/
RT_API void rtFramebufferSetColorView(rt_framebuffer framebuffer, u32 slot, rt_texture_view view);

/*!
** @brief Attach (or detach) the depth/stencil texture view of a framebuffer.
**
** @param framebuffer  Framebuffer to update.
** @param view         Texture view of a depth or depth-stencil format to
**                     attach, or NULL to remove the depth attachment.
*/
RT_API void rtFramebufferDepthView(rt_framebuffer framebuffer, rt_texture_view view);

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
** @p layout describes the vertex streams and their attributes. Each stream
** defines its stride and input rate. Each attribute identifies one stream,
** its byte offset in that stream, format, and name. The structure and its
** inner arrays are copied; the caller retains ownership.
**
** Pass NULL for @p layout if the program does not read vertex attributes
** (e.g. vertex IDs only).
**
** @param program  Graphics program to configure.
** @param layout   Vertex layout description, or NULL.
*/
RT_API void rtGraphicsProgramLayout(rt_graphics_program program, const rt_vertex_layout* layout);

/*!
** @brief Provide the shader code for a graphics program.
**
** @p data points to a compiled RTSL Program binary blob of @p size bytes.
** A single RTSL Program contains every shader entry point (vertex,
** fragment, etc.) the graphics program needs.
**
** Calling this on a finalized program is invalid. Create a new program to
** change its source.
**
** @param program  Graphics program to configure.
** @param data     Pointer to an RTSL Program binary.
** @param size     Size of @p data in bytes.
*/
RT_API void rtGraphicsProgramSource(rt_graphics_program program, const void* data, usize size);

/*!
** @brief Set rasterization state for a graphics program.
**
** @param program     Graphics program to configure.
** @param cull_mode   Which triangle faces are culled.
** @param front_face  Winding order considered front-facing.
** @param fill_mode   Triangle fill mode (solid or wireframe).
*/
RT_API void rtGraphicsProgramRasterState(rt_graphics_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode);

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
RT_API void rtGraphicsProgramBlendState(rt_graphics_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op);

/*!
** @brief Finalize a graphics program for use in draw commands.
**
** Locks in the current configuration (layout, source, raster/blend state)
** for draw commands. After this call, the configuration setters
** (@ref rtGraphicsProgramLayout, @ref rtGraphicsProgramSource,
** @ref rtGraphicsProgramRasterState, @ref rtGraphicsProgramBlendState) may
** not be called again.
**
** @ref rtGraphicsProgramLocation may only be called on a finalized
** program. A program must be finalized before being bound by
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
** @brief Look up a named shader input on a finalized graphics program.
**
** The returned location handle is owned by @p program and is invalidated
** when @p program is destroyed. A location identifies one typed program
** input: a vertex attribute, buffer resource, combined texture/sampler
** resource, or a future address input.
**
** @param program  Finalized graphics program to query.
** @param name     Null-terminated input name as it appears in the shader.
** @return Location handle, or NULL if no such input exists.
*/
RT_API rt_location rtGraphicsProgramLocation(rt_graphics_program program, const char* name);

/*!
** @brief Create a command buffer.
**
** A command buffer records Rutile intermediate representation. It is not tied
** to a queue. The active backend lowers its representation only when
** @ref rtQueueSubmit submits it.
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
RT_API void rtCmdReset(rt_command_buffer command_buffer);

/*!
** @brief Begin recording a command buffer.
**
** Every `rtCmd*` call below requires @p command_buffer to be recording.
**
** @param command_buffer  Command buffer to record into.
*/
RT_API void rtCmdBegin(rt_command_buffer command_buffer);

/*!
** @brief Make later recorded commands wait for a timepoint.
**
** Commands recorded after this operation execute only after @p timepoint is
** reached and observe the completed Rutile work represented by it. The wait is
** lowered at its recorded position during submission. Record it outside an
** active rendering scope. A zero-initialized timepoint adds no dependency.
**
** @param command_buffer  Command buffer being recorded.
** @param timepoint       Completion point required by later commands.
*/
RT_API void rtCmdWait(rt_command_buffer command_buffer, rt_timepoint timepoint);

/*!
** @brief Begin a rendering scope.
**
** @param command_buffer  Command buffer being recorded.
** @param framebuffer     Framebuffer whose attachments will be rendered to.
*/
RT_API void rtCmdBeginRendering(rt_command_buffer command_buffer, rt_framebuffer framebuffer);

/*!
** @brief Clear a color attachment in the current rendering scope.
**
** @param command_buffer  Command buffer being recorded.
** @param color_index     Color attachment slot to clear.
** @param r               Red.
** @param g               Green.
** @param b               Blue.
** @param a               Alpha.
*/
RT_API void rtCmdClearColor(rt_command_buffer command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a);

/*!
** @brief Clear the depth attachment in the current rendering scope.
**
** @param command_buffer  Command buffer being recorded.
** @param depth           Depth clear value.
*/
RT_API void rtCmdClearDepth(rt_command_buffer command_buffer, f32 depth);

/*!
** @brief Clear the stencil attachment in the current rendering scope.
**
** @param command_buffer  Command buffer being recorded.
** @param stencil         Stencil clear value.
*/
RT_API void rtCmdClearStencil(rt_command_buffer command_buffer, u32 stencil);

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
RT_API void rtCmdSetViewport(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth);

/*!
** @brief Set the scissor for the current rendering scope.
**
** @param command_buffer  Command buffer being recorded.
** @param x               Scissor left edge in framebuffer pixels.
** @param y               Scissor top edge in framebuffer pixels.
** @param width           Scissor width in framebuffer pixels.
** @param height          Scissor height in framebuffer pixels.
*/
RT_API void rtCmdSetScissor(rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height);

/*!
** @brief End the current rendering scope.
**
** @param command_buffer  Command buffer being recorded.
*/
RT_API void rtCmdEndRendering(rt_command_buffer command_buffer);

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
** @brief Bind a buffer range to a program location for subsequent draws.
**
** The location kind determines whether the range is a uniform or storage
** resource.
**
** @param command_buffer  Command buffer being recorded.
** @param location        Buffer-resource location obtained from the bound program.
** @param buffer          Buffer to read from.
** @param offset          Byte offset into @p buffer.
** @param size            Number of bytes visible through the binding.
*/
RT_API void rtCmdBindBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset, usize size);

/*!
** @brief Bind a texture view to a program location for subsequent draws.
**
** @p texture_view provides the combined texture and sampler input.
**
** @param command_buffer  Command buffer being recorded.
** @param location        Texture-resource location obtained from the bound program.
** @param texture_view    Texture view to sample.
*/
RT_API void rtCmdBindTexture(rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view);

/*!
** @brief Set a vertex attribute's source buffer.
**
** A vertex-buffer command binds the stream selected by @p location. Every
** attribute in that stream uses the specified buffer and offset.
**
** @param command_buffer  Command buffer being recorded.
** @param location        Vertex-attribute location obtained from the program.
** @param buffer          Buffer containing the selected stream's elements.
** @param offset          Byte offset into @p buffer where stream element 0 begins.
*/
RT_API void rtCmdVertexBuffer(rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset);

/*!
** @brief Set the index buffer for subsequent indexed draws.
**
** @param command_buffer  Command buffer being recorded.
** @param buffer          Buffer containing indices.
** @param offset          Byte offset into @p buffer where index 0 begins.
** @param format          Index-element format.
*/
RT_API void rtCmdIndexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, usize offset, enum rt_index_format format);

/*!
** @brief Record a non-indexed draw.
**
** @param command_buffer  Command buffer being recorded.
** @param vertex_count    Number of vertices to draw.
** @param first_vertex    Index of the first vertex (added to vertex IDs).
*/
RT_API void rtCmdDraw(rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex);

/*!
** @brief Record an instanced non-indexed draw.
**
** @param command_buffer  Command buffer being recorded.
** @param vertex_count    Number of vertices per instance.
** @param instance_count  Number of instances to draw.
** @param first_vertex    Index of the first vertex.
** @param first_instance  Index of the first instance.
*/
RT_API void rtCmdDrawInstanced(rt_command_buffer command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);

/*!
** @brief Record an indexed draw.
**
** @param command_buffer  Command buffer being recorded.
** @param index_count     Number of indices to draw.
** @param first_index     Index of the first index element.
** @param vertex_offset   Added to every vertex index.
*/
RT_API void rtCmdDrawIndexed(rt_command_buffer command_buffer, u32 index_count, u32 first_index, i32 vertex_offset);

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
RT_API void rtCmdDrawIndexedInstanced(rt_command_buffer command_buffer, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance);

/*!
** @brief Finish recording a command buffer.
**
** @param command_buffer  Command buffer being recorded.
*/
RT_API void rtCmdEnd(rt_command_buffer command_buffer);

/*!
** @brief Create a virtual queue with the requested capability.
**
** A queue is an ordered virtual submission stream. Rutile may map distinct
** virtual queues to the same device queue or to different device queues.
** Only the order of submissions through this queue is defined. Rutile makes
** no promise that two virtual queues run concurrently or use different device
** queues.
**
** @param capability  Required queue capability.
** @return New virtual queue, or NULL when no compatible device queue exists.
*/
RT_API rt_queue rtQueueCreate(enum rt_queue_capability capability);

/*!
** @brief Destroy a virtual queue.
**
** Destroying a queue prevents future submissions through it. Timepoints
** returned by earlier submissions remain usable until @ref rtUnload.
**
** @param queue  Virtual queue to destroy.
*/
RT_API void rtQueueDestroy(rt_queue queue);

/*!
** @brief Make a queue's next submission wait for a timepoint.
**
** The wait applies to the next submission through @p queue and is consumed by
** that submission. It is a device-side dependency and does not block the CPU.
** For a dependency at a particular point within a command buffer, use
** @ref rtCmdWait instead.
**
** @param queue      Queue whose next submission depends on @p timepoint.
** @param timepoint  Completion point required before that submission runs.
*/
RT_API void rtQueueWait(rt_queue queue, rt_timepoint timepoint);

/*!
** @brief Submit a completed command buffer to a queue.
**
** Submission lowers the buffer's recorded Rutile intermediate representation
** to backend commands. A completed command buffer may be submitted again or
** reset and recorded with different commands.
**
** The returned timepoint signals once this submission has completed on the
** GPU. Its implementation is private to the active backend.
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
** @brief Flush backend-deferred work from a virtual queue.
**
** Forces work previously submitted through @p queue to be handed to the
** device when the active backend defers submission. A backend that submits
** immediately returns a timepoint for work already handed to the device.
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
/*                                                                                               */
/*===============================================================================================*/

#endif /* !RT_TYPES_ONLY */

#define RT_CORE_PROCEDURES(X) \
	X( void                 , rtClearError                 , ( void                                                                                                                                                                                                                            ) , (                                                                                         ) ) \
	X( enum rt_error        , rtError                      , ( void                                                                                                                                                                                                                            ) , (                                                                                         ) ) \
	X( const char*          , rtErrorMessage               , ( void                                                                                                                                                                                                                            ) , (                                                                                         ) ) \
	X( void                 , rtInit                       , ( const char* const* features, u32 feature_count                                                                                                                                                                                  ) , ( features, feature_count                                                                 ) ) \
	X( void                 , rtExit                       , ( void                                                                                                                                                                                                                            ) , (                                                                                         ) ) \
	X( void                 , rtSetOutput                  , ( PFN_rtOutput output, void* user_data                                                                                                                                                                                            ) , ( output, user_data                                                                       ) ) \
	X( const char*          , rtGetName                    , ( void                                                                                                                                                                                                                            ) , (                                                                                         ) ) \
	X( enum rt_format_usage , rtQueryFormatCapabilities    , ( enum rt_format format                                                                                                                                                                                                           ) , ( format                                                                                  ) ) \
	X( rt_buffer            , rtBufferCreate               , ( void                                                                                                                                                                                                                            ) , (                                                                                         ) ) \
	X( void                 , rtBufferDestroy              , ( rt_buffer buffer                                                                                                                                                                                                                ) , ( buffer                                                                                  ) ) \
	X( rt_timepoint         , rtBufferData                 , ( rt_buffer buffer, enum rt_buffer_mode mode, enum rt_buffer_usage usage, usize size, const void* data                                                                                                                            ) , ( buffer, mode, usage, size, data                                                         ) ) \
	X( rt_timepoint         , rtBufferSubdata              , ( rt_buffer buffer, usize offset, usize size, const void* data                                                                                                                                                                    ) , ( buffer, offset, size, data                                                              ) ) \
	X( void                 , rtBufferRead                 , ( rt_buffer buffer, usize offset, usize size, void* data                                                                                                                                                                          ) , ( buffer, offset, size, data                                                              ) ) \
	X( rt_texture           , rtTextureCreate              , ( void                                                                                                                                                                                                                            ) , (                                                                                         ) ) \
	X( void                 , rtTextureDestroy             , ( rt_texture texture                                                                                                                                                                                                              ) , ( texture                                                                                 ) ) \
	X( rt_timepoint         , rtTextureCopy                , ( rt_texture src_texture, u32 src_mip, rt_texture dst_texture, u32 dst_mip                                                                                                                                                        ) , ( src_texture, src_mip, dst_texture, dst_mip                                              ) ) \
	X( rt_timepoint         , rtTextureData                , ( rt_texture texture, enum rt_texture_type type, u32 mip, u32 width, u32 height, u32 depth, enum rt_format format, const void* data                                                                                               ) , ( texture, type, mip, width, height, depth, format, data                                  ) ) \
	X( rt_timepoint         , rtTextureSubcopy             , ( rt_texture src_texture, u32 src_mip, rt_extent_3d src_offset, rt_texture dst_texture, u32 dst_mip, rt_extent_3d dst_offset, rt_extent_3d extent                                                                                 ) , ( src_texture, src_mip, src_offset, dst_texture, dst_mip, dst_offset, extent              ) ) \
	X( rt_timepoint         , rtTextureSubdata             , ( rt_texture texture, u32 mip, rt_extent_3d offset, rt_extent_3d extent, const void* data                                                                                                                                         ) , ( texture, mip, offset, extent, data                                                      ) ) \
	X( rt_texture_view      , rtTextureViewCreate          , ( void                                                                                                                                                                                                                            ) , (                                                                                         ) ) \
	X( void                 , rtTextureViewBind            , ( rt_texture_view texture_view, rt_texture texture                                                                                                                                                                                ) , ( texture_view, texture                                                                   ) ) \
	X( void                 , rtTextureViewDestroy         , ( rt_texture_view texture_view                                                                                                                                                                                                    ) , ( texture_view                                                                            ) ) \
	X( void                 , rtTextureViewFilter          , ( rt_texture_view texture_view, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter                                                                                                               ) , ( texture_view, mag_filter, min_filter, mip_filter                                        ) ) \
	X( void                 , rtTextureViewAddress         , ( rt_texture_view texture_view, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w                                                                                                    ) , ( texture_view, address_u, address_v, address_w                                           ) ) \
	X( void                 , rtTextureViewAnisotropy      , ( rt_texture_view texture_view, u32 max_anisotropy                                                                                                                                                                                ) , ( texture_view, max_anisotropy                                                            ) ) \
	X( void                 , rtTextureViewLod             , ( rt_texture_view texture_view, f32 min_lod, f32 max_lod, f32 lod_bias                                                                                                                                                            ) , ( texture_view, min_lod, max_lod, lod_bias                                                ) ) \
	X( rt_timepoint         , rtTextureViewCopyToBuffer    , ( rt_texture_view texture_view, rt_buffer buffer                                                                                                                                                                                  ) , ( texture_view, buffer                                                                    ) ) \
	X( rt_extent_3d         , rtTextureViewExtent          , ( rt_texture_view texture_view                                                                                                                                                                                                    ) , ( texture_view                                                                            ) ) \
	X( rt_framebuffer       , rtFramebufferCreate          , ( void                                                                                                                                                                                                                            ) , (                                                                                         ) ) \
	X( void                 , rtFramebufferDestroy         , ( rt_framebuffer framebuffer                                                                                                                                                                                                      ) , ( framebuffer                                                                             ) ) \
	X( rt_texture_view      , rtFramebufferColorView       , ( rt_framebuffer framebuffer, u32 slot                                                                                                                                                                                            ) , ( framebuffer, slot                                                                       ) ) \
	X( void                 , rtFramebufferSetColorView    , ( rt_framebuffer framebuffer, u32 slot, rt_texture_view view                                                                                                                                                                      ) , ( framebuffer, slot, view                                                                 ) ) \
	X( void                 , rtFramebufferDepthView       , ( rt_framebuffer framebuffer, rt_texture_view view                                                                                                                                                                                ) , ( framebuffer, view                                                                       ) ) \
	X( rt_graphics_program  , rtGraphicsProgramCreate      , ( void                                                                                                                                                                                                                            ) , (                                                                                         ) ) \
	X( void                 , rtGraphicsProgramDestroy     , ( rt_graphics_program program                                                                                                                                                                                                     ) , ( program                                                                                 ) ) \
	X( void                 , rtGraphicsProgramLayout      , ( rt_graphics_program program, const rt_vertex_layout* layout                                                                                                                                                                     ) , ( program, layout                                                                         ) ) \
	X( void                 , rtGraphicsProgramSource      , ( rt_graphics_program program, const void* data, usize size                                                                                                                                                                       ) , ( program, data, size                                                                     ) ) \
	X( void                 , rtGraphicsProgramRasterState , ( rt_graphics_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode                                                                                                            ) , ( program, cull_mode, front_face, fill_mode                                               ) ) \
	X( void                 , rtGraphicsProgramBlendState  , ( rt_graphics_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op ) , ( program, enabled, src_color, dst_color, color_op, src_alpha, dst_alpha, alpha_op        ) ) \
	X( void                 , rtGraphicsProgramFinalize    , ( rt_graphics_program program                                                                                                                                                                                                     ) , ( program                                                                                 ) ) \
	X( rt_location          , rtGraphicsProgramLocation    , ( rt_graphics_program program, const char* name                                                                                                                                                                                   ) , ( program, name                                                                           ) ) \
	X( rt_command_buffer    , rtCommandBufferCreate        , ( void                                                                                                                                                                                                                            ) , (                                                                                         ) ) \
	X( void                 , rtCommandBufferDestroy       , ( rt_command_buffer command_buffer                                                                                                                                                                                                ) , ( command_buffer                                                                          ) ) \
	X( void                 , rtCmdReset                   , ( rt_command_buffer command_buffer                                                                                                                                                                                                ) , ( command_buffer                                                                          ) ) \
	X( void                 , rtCmdBegin                   , ( rt_command_buffer command_buffer                                                                                                                                                                                                ) , ( command_buffer                                                                          ) ) \
	X( void                 , rtCmdWait                    , ( rt_command_buffer command_buffer, rt_timepoint timepoint                                                                                                                                                                        ) , ( command_buffer, timepoint                                                               ) ) \
	X( void                 , rtCmdBeginRendering          , ( rt_command_buffer command_buffer, rt_framebuffer framebuffer                                                                                                                                                                    ) , ( command_buffer, framebuffer                                                             ) ) \
	X( void                 , rtCmdClearColor              , ( rt_command_buffer command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a                                                                                                                                                   ) , ( command_buffer, color_index, r, g, b, a                                                 ) ) \
	X( void                 , rtCmdClearDepth              , ( rt_command_buffer command_buffer, f32 depth                                                                                                                                                                                     ) , ( command_buffer, depth                                                                   ) ) \
	X( void                 , rtCmdClearStencil            , ( rt_command_buffer command_buffer, u32 stencil                                                                                                                                                                                   ) , ( command_buffer, stencil                                                                 ) ) \
	X( void                 , rtCmdSetViewport             , ( rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth                                                                                                                             ) , ( command_buffer, x, y, width, height, min_depth, max_depth                               ) ) \
	X( void                 , rtCmdSetScissor              , ( rt_command_buffer command_buffer, u32 x, u32 y, u32 width, u32 height                                                                                                                                                           ) , ( command_buffer, x, y, width, height                                                     ) ) \
	X( void                 , rtCmdEndRendering            , ( rt_command_buffer command_buffer                                                                                                                                                                                                ) , ( command_buffer                                                                          ) ) \
	X( void                 , rtCmdUseGraphicsProgram      , ( rt_command_buffer command_buffer, rt_graphics_program program                                                                                                                                                                   ) , ( command_buffer, program                                                                 ) ) \
	X( void                 , rtCmdBindBuffer              , ( rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset, usize size                                                                                                                              ) , ( command_buffer, location, buffer, offset, size                                          ) ) \
	X( void                 , rtCmdBindTexture             , ( rt_command_buffer command_buffer, rt_location location, rt_texture_view texture_view                                                                                                                                            ) , ( command_buffer, location, texture_view                                                  ) ) \
	X( void                 , rtCmdVertexBuffer            , ( rt_command_buffer command_buffer, rt_location location, rt_buffer buffer, usize offset                                                                                                                                          ) , ( command_buffer, location, buffer, offset                                                ) ) \
	X( void                 , rtCmdIndexBuffer             , ( rt_command_buffer command_buffer, rt_buffer buffer, usize offset, enum rt_index_format format                                                                                                                                   ) , ( command_buffer, buffer, offset, format                                                  ) ) \
	X( void                 , rtCmdDraw                    , ( rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex                                                                                                                                                            ) , ( command_buffer, vertex_count, first_vertex                                              ) ) \
	X( void                 , rtCmdDrawInstanced           , ( rt_command_buffer command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance                                                                                                                    ) , ( command_buffer, vertex_count, instance_count, first_vertex, first_instance              ) ) \
	X( void                 , rtCmdDrawIndexed             , ( rt_command_buffer command_buffer, u32 index_count, u32 first_index, i32 vertex_offset                                                                                                                                           ) , ( command_buffer, index_count, first_index, vertex_offset                                 ) ) \
	X( void                 , rtCmdDrawIndexedInstanced    , ( rt_command_buffer command_buffer, u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance                                                                                                   ) , ( command_buffer, index_count, instance_count, first_index, vertex_offset, first_instance ) ) \
	X( void                 , rtCmdEnd                     , ( rt_command_buffer command_buffer                                                                                                                                                                                                ) , ( command_buffer                                                                          ) ) \
	X( rt_queue             , rtQueueCreate                , ( enum rt_queue_capability capability                                                                                                                                                                                             ) , ( capability                                                                              ) ) \
	X( void                 , rtQueueDestroy               , ( rt_queue queue                                                                                                                                                                                                                  ) , ( queue                                                                                   ) ) \
	X( void                 , rtQueueWait                  , ( rt_queue queue, rt_timepoint timepoint                                                                                                                                                                                          ) , ( queue, timepoint                                                                        ) ) \
	X( rt_timepoint         , rtQueueSubmit                , ( rt_queue queue, rt_command_buffer command_buffer                                                                                                                                                                                ) , ( queue, command_buffer                                                                   ) ) \
	X( rt_timepoint         , rtQueueFlush                 , ( rt_queue queue                                                                                                                                                                                                                  ) , ( queue                                                                                   ) ) \
	X( void                 , rtTimepointWait              , ( rt_timepoint timepoint                                                                                                                                                                                                          ) , ( timepoint                                                                               ) ) \
	X( bool                 , rtTimepointReached           , ( rt_timepoint timepoint                                                                                                                                                                                                          ) , ( timepoint                                                                               ) )
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

#endif /* RUTILE_H */
