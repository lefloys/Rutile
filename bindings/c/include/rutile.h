#ifndef RUTILE_H
#define RUTILE_H

/*!
** @file rutile.h
** @brief Rutile public C API and dynamic loader.
**
** Rutile is a graphics abstraction. This header provides:
**   - The loader (@ref rtLoad, @ref rtUnload, @ref rtLoaded, @ref rtGetProc).
**     The loader resolves a backend DLL plus an optional ordered chain of
**     layer DLLs, then exposes the core API through inline wrappers.
**   - The core API surface itself (resources, command buffers, queues,
**     synchronization). All core entry points are dispatched through function
**     pointers populated by the loader.
**
** @section threading Threading
** Any function in this API may be called from any thread, concurrently with
** any other function. The only constraint is that the caller must not mutate
** the same resource from two threads at once. For example, two threads may
** record into two different command buffers in parallel, but two threads must
** not call @ref rtBufferData on the same buffer concurrently.
**
** @section timepoints Timepoints
** Work that touches the GPU returns an @ref rt_timepoint - an opaque signal
** that fires once the operation has completed on the device. Pass a timepoint
** to @ref rtTimepointWait to block the CPU until completion, to
** @ref rtTimepointReached to poll, or to @ref rtQueueWait to make a queue
** wait for it before its next submission. A zero-initialized timepoint
** (`{ NULL, 0 }`) is the canonical "already reached" sentinel and is always
** safe to wait on; it records no dependency.
**
** @section errors Error model
** Aside from the loader entry points (@ref rtLoad, @ref rtLoadDevelopment),
** core API functions do not return error codes. They report failure
** out-of-band by recording an error code and message on the current
** thread, retrievable via @ref rtError and @ref rtErrorMessage and cleared
** by @ref rtClearError.
**
** **Any core function may record @ref RT_IMPROPER_USAGE.** This is the
** contract layers rely on: a validation layer in the dispatch chain is
** entitled to inspect arguments and surface usage violations through the
** error state of any call, even when the backend itself would have
** silently proceeded. Backends are not required to detect usage violations
** on their own.
**
** Beyond that universal rule, each function documents the additional
** error codes it may produce in an `@error` block. Functions that have no
** `@error` block can only report RT_IMPROPER_USAGE (and only when a
** validation layer is active).
**
** Functions that return a handle indicate failure by returning NULL; the
** specific error code is recorded as described above.
**
** **Destroy functions never record an error.** They are infallible by
** contract - not even RT_IMPROPER_USAGE, and not even from a validation
** layer.
**
** @section build_macros Build macros
**   - `RT_BUILD_DLL`       - define when building Rutile itself as a DLL.
**   - `RUTILE_IMPL`        - define in exactly one translation unit to emit
**                            the loader implementation.
**                            and skip the core API declarations.
**                            and use the `rt_rt*` function pointers directly.
*/

#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdint.h>

#define RT_NULL_HANDLE NULL

#if !defined(RT_BUILD_DLL)
#define RT_EXPORT
#elif defined(_WIN32)
#define RT_EXPORT __declspec(dllexport)
#else
#define RT_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* rt_proc_t;

typedef struct rt_proc_chain {
	rt_proc_t (*get_proc)(const struct rt_proc_chain* chain, const char* name);
} rt_proc_chain;

typedef const char* (*PFN_rtLayerGetName)(void);
typedef void (*PFN_rtLayerSetNext)(rt_proc_chain next);


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

#define RT_FEATURE_PRESENTATION "RT_FEATURE_PRESENTATION"

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

enum rt_index_type { RT_INDEX_TYPE__RESERVED = 0x7fffffff };
#define RT_INDEX_U16 ((enum rt_index_type)1)
#define RT_INDEX_U32 ((enum rt_index_type)2)

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

typedef struct rt_vertex_attribute {
	const char* name;
	usize offset;
	enum rt_format format;
} rt_vertex_attribute;

typedef struct rt_vertex_layout {
	const rt_vertex_attribute* attributes;
	usize stride;
	usize attribute_count;
} rt_vertex_layout;

typedef struct rt_texture_t* rt_texture;
typedef struct rt_texture_view_t* rt_texture_view;
typedef struct rt_buffer_t* rt_buffer;
typedef struct rt_graphics_program_t* rt_graphics_program;
typedef struct rt_uniform_location_t* rt_uniform_location;
typedef struct rt_command_context_t* rt_command_context;
typedef struct rt_command_buffer_t* rt_command_buffer;
typedef struct rt_framebuffer_t* rt_framebuffer;
typedef struct rt_queue_t* rt_queue;

typedef struct rt_timepoint {
	rt_queue queue;
	u64 value;
} rt_timepoint;

typedef struct rt_extent_3d {
	usize width;
	usize height;
	usize depth;
} rt_extent_3d;

/*!
** @brief Load a backend and an optional ordered chain of layers.
**
** Opens the backend shared library, opens each layer shared library in order,
** wires them into a dispatch chain (calls flow through layer 0, layer 1, ...,
** finally reaching the backend), and resolves every required core entry point
** into the `rt_rt*` function pointers. After a successful call,
** @ref rtLoaded returns true and the static-inline wrappers are safe to use.
**
** At most one backend may be loaded at a time. Calling @ref rtLoad while a
** backend is already loaded implicitly unloads the previous one first.
**
** @param backend_name  Backend name (e.g. `"rutile-vulkan"`). The loader maps
**                      this to a platform-specific DLL/SO/dylib filename.
** @param layer_names   Optional array of layer names, applied in order. The
**                      first entry sits closest to the application; the last
**                      sits closest to the backend. May be NULL when
**                      @p layer_count is 0.
** @param layer_count   Number of entries in @p layer_names.
**
** @return RT_SUCCESS on success.
** @return RT_NO_BACKEND if the backend DLL could not be opened or did not
**         export the expected entry points.
** @return RT_IMPROPER_USAGE for invalid arguments or layer-loading failures.
** @return RT_EXTENSION_NOT_PRESENT if the backend is missing a required core
**         entry point.
**
** @note On failure the loader is left fully unloaded. Use @ref rtGetProc to
**       resolve optional/extension entry points after a successful load.
*/
enum rt_error rtLoad(const char* backend_name, const char* const* layer_names, usize layer_count);

/*!
** @brief Load a backend and layers in best-effort mode for development.
**
** Same intent as @ref rtLoad but tolerates a missing backend for development
** tools. Explicitly requested layers are still required: a missing or
** invalid layer fails the load rather than silently changing the dispatch
** chain. Entry points that the backend does not provide are left as NULL
** function pointers rather than failing the load.
**
** Intended for iterative development where backends are being built out and
** not every entry point exists yet. Production code should call @ref rtLoad.
**
** @param backend_name  Backend name, or NULL to load no backend.
** @param layer_names   Optional array of layer names (see @ref rtLoad).
** @param layer_count   Number of entries in @p layer_names.
**
** @return RT_SUCCESS when the backend is absent or loads successfully;
**         an error when a requested layer cannot be loaded.
*/
enum rt_error rtLoadDevelopment(const char* backend_name, const char* const* layer_names, usize layer_count);

/*!
** @brief Unload the current backend and layers.
**
** Clears every dispatch function pointer back to NULL, tears down the
** dispatch chain, and closes every shared library that was opened by the
** loader. After this call @ref rtLoaded returns false and the static-inline
** core wrappers must not be called.
**
** Safe to call when nothing is loaded; in that case it is a no-op.
*/
void rtUnload(void);

/*!
** @brief Report whether a backend is currently loaded.
**
** @return true when the loader has a backend wired up and the core wrappers
**         may be called; false otherwise.
*/
bool rtLoaded(void);

/*!
** @brief Resolve a named entry point through the current dispatch chain.
**
** This is the loader's general lookup mechanism. It returns the function
** pointer for @p name as seen at the head of the dispatch chain - i.e. with
** every loaded layer's interception applied. The core wrappers use this
** internally; callers reach for it directly to load optional or
** extension-provided entry points.
**
** @param name  Null-terminated entry-point name (e.g. `"rtBufferData"`, or
**              the name of an extension function).
** @return Function pointer to the entry, or NULL if no loaded module
**         provides it.
*/
rt_proc_t rtGetProc(const char* name);

typedef void (*PFN_rtOutput)(const char* message, void* user_data);

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
** and an explanatory message naming the offending feature is recorded; query
** them via @ref rtError and @ref rtErrorMessage.
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
static inline void rtInit(const char* const* features, u32 feature_count);

/*!
** @brief Shut down Rutile.
**
** Releases all backend-owned resources that were created since @ref rtInit
** and returns the runtime to a pre-initialization state. The loader itself
** remains loaded; call @ref rtInit again to bring the runtime back up, or
** @ref rtUnload to also tear down the backend DLLs.
**
** All public handles (buffers, textures, programs, command buffers,
** framebuffers, queues, uniform locations, timepoints) acquired since the
** matching @ref rtInit are invalidated. Destroy your own resources before
** calling.
*/
static inline void rtExit(void);

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
static inline void rtSetOutput(PFN_rtOutput output, void* user_data);

/*!
** @brief Return the most recently recorded backend/runtime error code.
**
** Reports the current error state of the loaded backend and layers (not the
** loader itself, which surfaces its errors through @ref rtLoad's return
** value). Returns RT_SUCCESS when no error is recorded.
**
** @return The current error code, or RT_SUCCESS if none.
*/
static inline enum rt_error rtError(void);

/*!
** @brief Return a human-readable description of the current error.
**
** Pairs with @ref rtError. The returned pointer is owned by Rutile and
** remains valid until the next call that mutates error state (any further
** API call may invalidate it). The string may be empty when the code alone
** is sufficient.
**
** @return Null-terminated message, owned by Rutile.
*/
static inline const char* rtErrorMessage(void);

/*!
** @brief Clear the current backend/runtime error state.
**
** After this call @ref rtError returns RT_SUCCESS and @ref rtErrorMessage
** returns an empty string, until the next operation records a new error.
*/
static inline void rtClearError(void);

/*!
** @brief Return the loaded backend's self-reported name.
**
** Used by the loader to verify backend identity and useful to applications
** that want to log or branch on the active backend. The returned pointer is
** owned by the backend and remains valid until @ref rtUnload.
**
** @return Static null-terminated name string.
*/
static inline const char* rtGetName(void);

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
static inline enum rt_format_usage rtQueryFormatCapabilities(enum rt_format format);

/*!
** @brief Create a buffer handle with no backing storage.
**
** Allocates only the handle. Storage is defined by a subsequent call to
** @ref rtBufferData, which sets size, usage, and mode in one step.
**
** @return New buffer handle, or NULL on failure.
**
** @error RT_OUT_OF_HOST_MEMORY  Allocation of the buffer backing resource failed.
*/
static inline rt_buffer rtBufferCreate(void);

/*!
** @brief Destroy a buffer and release its storage.
**
** Safe to call on NULL. Any GPU work still referencing the buffer continues
** to run against the now-zombie resource; the storage is reclaimed once that
** work completes.
**
** @param buffer  Buffer to destroy.
*/
static inline void rtBufferDestroy(rt_buffer buffer);

/*!
** @brief (Re)define buffer storage and optionally upload initial contents.
**
** Replaces any previously defined storage on @p buffer with a fresh
** allocation of @p size bytes, configured for the given mode and usage
** flags, and (if @p data is non-NULL) uploads @p size bytes from @p data
** into it. If @p data is NULL the contents are zero-initialized.
**
** @param buffer  Buffer to (re)configure.
** @param mode    Storage/update strategy hint (static vs dynamic).
** @param usage   Bitset of RT_BUFFER_USAGE_* flags describing how the buffer
**                will be used (vertex, index, uniform, storage, transfer).
** @param size    New storage size in bytes.
** @param data    Optional source bytes copied into the buffer, or NULL to
**                zero-initialize.
**
** @return Signal that fires when the upload has completed on the GPU.
**
** @error RT_OUT_OF_HOST_MEMORY    Host staging allocation failed.
** @error RT_OUT_OF_DEVICE_MEMORY  Device allocation for the storage failed.
*/
static inline rt_timepoint rtBufferData(rt_buffer buffer, enum rt_buffer_mode mode, enum rt_buffer_usage usage, usize size, const void* data);

/*!
** @brief Upload bytes into an existing range of a buffer.
**
** Does not change the buffer's size, mode, or usage - only its contents in
** `[offset, offset + size)`. The range must lie fully within the storage
** previously defined by @ref rtBufferData.
**
** @param buffer  Destination buffer.
** @param offset  Byte offset into @p buffer.
** @param size    Number of bytes to upload.
** @param data    Source bytes copied into the buffer range.
**
** @return Signal that fires when the upload has completed on the GPU.
**
** @error RT_OUT_OF_DEVICE_MEMORY  Device-side staging allocation failed.
** @error RT_DEVICE_LOST           The device was lost while scheduling the
**                                 upload.
*/
static inline rt_timepoint rtBufferSubdata(rt_buffer buffer, usize offset, usize size, const void* data);

/*!
** @brief Copy bytes out of a buffer into application memory.
**
** Blocking readback. The call returns only after the requested bytes have
** been retrieved from @p buffer and written to @p data; any pending GPU work
** that produces those bytes is waited on internally.
**
** @param buffer  Source buffer.
** @param offset  Byte offset into @p buffer.
** @param size    Number of bytes to copy.
** @param data    Destination memory, at least @p size bytes.
**
** @error RT_OUT_OF_DEVICE_MEMORY  Device-side staging allocation failed.
** @error RT_DEVICE_LOST           The device was lost while waiting for the
**                                 contents to become readable.
*/
static inline void rtBufferRead(rt_buffer buffer, usize offset, usize size, void* data);

/*!
** @brief Create a texture handle with no backing storage.
**
** Allocates only the handle. Storage (dimensionality, extents, format, mip
** contents) is defined by a subsequent call to @ref rtTextureData.
**
** @return New texture handle, or NULL on failure.
**
** @error RT_OUT_OF_HOST_MEMORY  Allocation of the buffer backing resource failed.
*/
static inline rt_texture rtTextureCreate(void);

/*!
** @brief Destroy a texture and release its storage.
**
** Safe to call on NULL. Any GPU work still referencing the texture continues
** to run against the now-zombie resource; storage is reclaimed once that
** work completes. Texture views that referenced this texture are
** invalidated.
**
** @param texture  Texture to destroy.
*/
static inline void rtTextureDestroy(rt_texture texture);

/*!
** @brief Create an unbound texture view with default sampler state.
**
** A texture view bundles a texture reference with sampler state (filtering,
** addressing, anisotropy, LOD). The view is created unbound; call
** @ref rtTextureViewBind to associate it with a texture.
**
** @return New texture view handle, or NULL on failure.
**
** @error RT_OUT_OF_HOST_MEMORY  Allocation of the buffer backing resource failed.
*/
static inline rt_texture_view rtTextureViewCreate(void);

/*!
** @brief Bind a texture to an existing texture view.
**
** Re-points @p texture_view at @p texture. The view's sampler state is
** preserved across the rebind. The view becomes invalid if @p texture is
** later destroyed.
**
** @param texture_view  Texture view to update.
** @param texture       Texture to view through @p texture_view.
*/
static inline void rtTextureViewBind(rt_texture_view texture_view, rt_texture texture);

/*!
** @brief Destroy a texture view.
**
** Does not affect the underlying texture. Safe to call on NULL.
**
** @param texture_view  Texture view to destroy.
*/
static inline void rtTextureViewDestroy(rt_texture_view texture_view);

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
static inline void rtTextureViewFilter(rt_texture_view texture_view, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter);

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
static inline void rtTextureViewAddress(rt_texture_view texture_view, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w);

/*!
** @brief Set the maximum anisotropy used when sampling through a view.
**
** @param texture_view    Texture view to update.
** @param max_anisotropy  Maximum anisotropic sample count. 0 maps to the
**                        backend default (no anisotropy).
*/
static inline void rtTextureViewAnisotropy(rt_texture_view texture_view, u32 max_anisotropy);

/*!
** @brief Set the LOD selection state used when sampling through a view.
**
** @param texture_view  Texture view to update.
** @param min_lod       Lower clamp on the computed mip level.
** @param max_lod       Upper clamp on the computed mip level.
** @param lod_bias      Bias added to the computed mip level before clamping.
*/
static inline void rtTextureViewLod(rt_texture_view texture_view, f32 min_lod, f32 max_lod, f32 lod_bias);

/*!
** @brief Copy one entire mip level of a texture to another texture's mip.
**
** Copies the full extent of (@p src_texture, @p src_mip) into
** (@p dst_texture, @p dst_mip). The two mip levels must have matching
** extents and compatible formats. Both textures must already have storage
** defined.
**
** @param src_texture  Source texture.
** @param src_mip      Mip level of @p src_texture to read from.
** @param dst_texture  Destination texture.
** @param dst_mip      Mip level of @p dst_texture to write to.
**
** @return Signal that fires when the copy has completed on the GPU.
**
** @error RT_DEVICE_LOST  The device was lost while scheduling the copy.
*/
static inline rt_timepoint rtTextureCopy(rt_texture src_texture, u32 src_mip, rt_texture dst_texture, u32 dst_mip);

/*!
** @brief (Re)define a mip level of a texture and optionally upload its data.
**
** Configures @p texture to hold a @p type texture of the given @p format,
** with storage for the mip level @p mip sized @p width ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â @p height ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â
** @p depth texels. If @p data is non-NULL, the level is initialized from
** tightly packed source data in @p format; if NULL, the level is
** zero-initialized (matching @ref rtBufferData).
**
** Texture dimensionality (@p type) and format are properties of the texture
** as a whole; supplying different values across calls on the same texture
** redefines it.
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
**
** @error RT_OUT_OF_HOST_MEMORY    Host staging allocation failed.
** @error RT_OUT_OF_DEVICE_MEMORY  Device allocation for the storage failed.
*/
static inline rt_timepoint rtTextureData(rt_texture texture, enum rt_texture_type type, u32 mip, u32 width, u32 height, u32 depth, enum rt_format format, const void* data);

/*!
** @brief Copy a sub-region of one texture mip to a sub-region of another.
**
** Copies a @p width ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â @p height ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â @p depth block of texels from
** (@p src_texture, @p src_mip) at offset (@p src_x, @p src_y, @p src_z)
** into (@p dst_texture, @p dst_mip) at offset (@p dst_x, @p dst_y,
** @p dst_z). Both source and destination regions must fit within their
** respective mip extents.
**
** @return Signal that fires when the copy has completed on the GPU.
**
** @error RT_DEVICE_LOST  The device was lost while scheduling the copy.
*/
static inline rt_timepoint rtTextureSubcopy(rt_texture src_texture, u32 src_mip, u32 src_x, u32 src_y, u32 src_z, rt_texture dst_texture, u32 dst_mip, u32 dst_x, u32 dst_y, u32 dst_z, u32 width, u32 height, u32 depth);

/*!
** @brief Upload tightly packed texel data into a sub-region of a texture mip.
**
** Writes a @p width ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â @p height ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â @p depth block of texels from @p data
** into mip level @p mip of @p texture starting at offset (@p offset_x,
** @p offset_y, @p offset_z). The region must fit within the level extents
** previously defined by @ref rtTextureData.
**
** @return Signal that fires when the upload has completed on the GPU.
**
** @error RT_OUT_OF_DEVICE_MEMORY  Device-side staging allocation failed.
** @error RT_DEVICE_LOST           The device was lost while scheduling the
**                                 upload.
*/
static inline rt_timepoint rtTextureSubdata(rt_texture texture, u32 mip, u32 offset_x, u32 offset_y, u32 offset_z, u32 width, u32 height, u32 depth, const void* data);

/*!
** @brief Copy the contents seen through a texture view into a buffer.
**
** Writes the view's texels into @p buffer as tightly packed bytes (no row
** padding) in the source texture's format. The buffer must have storage
** large enough to hold the result.
**
** @param texture_view  Source texture view.
** @param buffer        Destination buffer.
**
** @return Signal that fires when the copy has completed on the GPU.
**
** @error RT_DEVICE_LOST  The device was lost while scheduling the copy.
*/
static inline rt_timepoint rtTextureViewCopyToBuffer(rt_texture_view texture_view, rt_buffer buffer);

/*!
** @brief Return the texel extent visible through a texture view.
**
** Reports the width/height/depth of the texture (or selected mip) seen
** through @p texture_view, in texels.
**
** @param texture_view  Texture view to query.
** @return The view's extent, or `{0, 0, 0}` if the view is invalid or its
**         backing texture has been destroyed.
*/
static inline rt_extent_3d rtTextureViewExtent(rt_texture_view texture_view);

/*!
** @brief Create an empty framebuffer with no attachments.
**
** Color slots and the depth slot are populated via
** @ref rtFramebufferSetColorView and @ref rtFramebufferDepthView.
**
** @return New framebuffer handle, or NULL on failure.
**
** @error RT_OUT_OF_HOST_MEMORY  Allocation of the buffer backing resource failed.
*/
static inline rt_framebuffer rtFramebufferCreate(void);

/*!
** @brief Destroy a framebuffer.
**
** Does not destroy the texture views that were attached to it. Safe to call
** on NULL.
**
** @param framebuffer  Framebuffer to destroy.
*/
static inline void rtFramebufferDestroy(rt_framebuffer framebuffer);

/*!
** @brief Return the texture view currently attached to a color slot.
**
** @param framebuffer  Framebuffer to query.
** @param slot         Color attachment slot index.
** @return The bound texture view, or NULL if @p slot is empty.
*/
static inline rt_texture_view rtFramebufferColorView(rt_framebuffer framebuffer, u32 slot);

/*!
** @brief Attach (or detach) a texture view to a color slot of a framebuffer.
**
** @param framebuffer  Framebuffer to update.
** @param slot         Color attachment slot index.
** @param view         Texture view to attach, or NULL to clear the slot.
*/
static inline void rtFramebufferSetColorView(rt_framebuffer framebuffer, u32 slot, rt_texture_view view);

/*!
** @brief Attach (or detach) the depth/stencil texture view of a framebuffer.
**
** @param framebuffer  Framebuffer to update.
** @param view         Texture view of a depth or depth-stencil format to
**                     attach, or NULL to remove the depth attachment.
*/
static inline void rtFramebufferDepthView(rt_framebuffer framebuffer, rt_texture_view view);

/*!
** @brief Create an unconfigured graphics program.
**
** Allocates only the handle. The program is built up by setting its vertex
** layout, shader source, raster state, and blend state, then sealed with
** @ref rtGraphicsProgramFinalize before it can be used in a draw call.
**
** @return New graphics program handle, or NULL on failure.
**
** @error RT_OUT_OF_HOST_MEMORY  Allocation of the buffer backing resource failed.
*/
static inline rt_graphics_program rtGraphicsProgramCreate(void);

/*!
** @brief Destroy a graphics program.
**
** Any uniform locations previously obtained from this program are
** invalidated. Safe to call on NULL.
**
** @param program  Graphics program to destroy.
*/
static inline void rtGraphicsProgramDestroy(rt_graphics_program program);

/*!
** @brief Set the vertex input layout used by a graphics program.
**
** @p layout describes the stride of a single vertex and the per-attribute
** offsets/formats/names that the vertex shader will consume. The structure
** and its inner arrays are copied; the caller retains ownership.
**
** Pass NULL for @p layout if the program does not read vertex attributes
** (e.g. vertex IDs only).
**
** @param program  Graphics program to configure.
** @param layout   Vertex layout description, or NULL.
*/
static inline void rtGraphicsProgramLayout(rt_graphics_program program, const rt_vertex_layout* layout);

/*!
** @brief Provide the shader code for a graphics program.
**
** @p data points to a compiled RTSL Program binary blob of @p size bytes.
** A single RTSL Program contains every shader entry point (vertex,
** fragment, etc.) the graphics program needs.
**
** Calling this on a finalized program is invalid; reset it first with
** @ref rtGraphicsProgramReset.
**
** @param program  Graphics program to configure.
** @param size     Size of @p data in bytes.
** @param data     Pointer to an RTSL Program binary.
*/
static inline void rtGraphicsProgramSource(rt_graphics_program program, usize size, const void* data);

/*!
** @brief Set rasterization state for a graphics program.
**
** @param program     Graphics program to configure.
** @param cull_mode   Which triangle faces are culled.
** @param front_face  Winding order considered front-facing.
** @param fill_mode   Triangle fill mode (solid or wireframe).
*/
static inline void rtGraphicsProgramRasterState(rt_graphics_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode);

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
static inline void rtGraphicsProgramBlendState(rt_graphics_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op);

/*!
** @brief Finalize a graphics program for use in draw commands.
**
** Locks in the current configuration (layout, source, raster/blend state)
** and performs any backend-side compilation/linking needed to make the
** program drawable. After this call, the configuration setters
** (@ref rtGraphicsProgramLayout, @ref rtGraphicsProgramSource,
** @ref rtGraphicsProgramRasterState, @ref rtGraphicsProgramBlendState) may
** not be called again until the program is reset with
** @ref rtGraphicsProgramReset.
**
** @ref rtGraphicsProgramUniformLocation may only be called on a finalized
** program. A program must be finalized before being bound by
** @ref rtCmdUseGraphicsProgram.
**
** Errors during compilation/linking are reported through
** @ref rtError / @ref rtErrorMessage (typically RT_SHADER_COMPILATION_FAILED
** or RT_SHADER_LINK_FAILED).
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
static inline void rtGraphicsProgramFinalize(rt_graphics_program program);

/*!
** @brief Return a finalized graphics program to a configurable state.
**
** Mirror of @ref rtGraphicsProgramFinalize. Discards the backend pipeline
** built by the previous finalize and re-opens the configuration setters.
** All uniform locations previously obtained from the program are
** invalidated.
**
** No-op when the program has not been finalized.
**
** @param program  Graphics program to reset.
*/
static inline void rtGraphicsProgramReset(rt_graphics_program program);

/*!
** @brief Look up a named shader uniform on a finalized graphics program.
**
** The returned location handle is owned by @p program and is invalidated
** when the program is reset (@ref rtGraphicsProgramReset) or destroyed
** (@ref rtGraphicsProgramDestroy). Locations are used with the
** `rtCmdUniform*` family to bind resources during command-buffer recording.
**
** @param program  Finalized graphics program to query.
** @param name     Null-terminated uniform name as it appears in the shader.
** @return Uniform location handle, or NULL if no such uniform exists.
*/
static inline rt_uniform_location rtGraphicsProgramUniformLocation(rt_graphics_program program, const char* name);

/*!
** @brief Create a single-fire command context.
**
** Allocates an unbound, single-fire context. Bind a queue with
** @ref rtCommandContextBind before declaring its rendering scope or
** allocating child buffers. Command-context functions are mandatory core
** entry points; until the sequential backend and layer work implements them,
** strict @ref rtLoad rejects implementations that do not export them.
**
** @return New command context handle, or NULL on failure.
**
** @error RT_OUT_OF_HOST_MEMORY  Allocation of the buffer backing resource failed.
*/
static inline rt_command_context rtCommandContextCreate(void);

/*!
** @brief Destroy a command context and invalidate all of its child buffers.
**
** Every remaining child handle becomes invalid immediately. Submitted work
** remains valid; the backend defers native reclamation until it is safe.
** Safe to call on NULL.
**
** @param command_context  Context to destroy.
*/
static inline void rtCommandContextDestroy(rt_command_context command_context);

/*!
** @brief Bind a queue to a command context.
**
** Rebinding always discards all context work and every child buffer's
** contents, including when @p queue is unchanged. Child handles remain owned
** by the context but return to their Initial state and must be recorded again.
**
** @param command_context  Context to bind.
** @param queue            Queue for this context lifetime.
*/
static inline void rtCommandContextBind(rt_command_context command_context, rt_queue queue);

/*!
** @brief Allocate a resettable child command buffer.
**
** Allocation requires the context's current queue binding and is allowed
** before or after its framebuffer scope is declared, but never after
** submission. The context owns the returned handle; destroying the context
** invalidates it.
**
** @param command_context  Queue-bound context that owns the child.
** @return A new child command buffer, or NULL on failure.
*/
static inline rt_command_buffer rtCommandContextAllocate(rt_command_context command_context);

/*!
** @brief Destroy one child command buffer.
**
** It must not have been executed by its owning context before that context
** submits. Submitted native work is reclaimed safely.
** Safe to call on NULL.
**
** @param command_buffer  Child command buffer to destroy.
*/
static inline void rtCommandBufferDestroy(rt_command_buffer command_buffer);

/*!
** @brief Discard one child buffer's recorded contents.
**
** Preserves its owning context. It must not have been executed by an
** unsubmitted owning context.
** Call @ref rtCmdReset before recording an executable child again.
**
** @param command_buffer  Child command buffer to reset.
*/
static inline void rtCmdReset(rt_command_buffer command_buffer);

/*!
** @brief Begin recording a child command buffer.
**
** Starts recording an Initial child buffer. Call @ref rtCmdReset before
** recording an Executable buffer again. The owning context must be live and
** queue-bound with an active framebuffer scope. The current public graphics
** command set records draw work for that scope's fixed framebuffer
** compatibility. A future generic-work command set may use the reserved
** pre-framebuffer context position without adding a user-selected buffer mode.
**
** Every `rtCmd*` call below requires the target command buffer to be in the
** recording state.
**
** @param command_buffer  Command buffer to record into.
*/
static inline void rtCmdBegin(rt_command_buffer command_buffer);

/*!
** @brief Bind a graphics program for subsequent draw commands.
**
** The program must be finalized (@ref rtGraphicsProgramFinalize). The
** binding stays in effect until the next @ref rtCmdUseGraphicsProgram or
** until the command buffer ends.
** The command buffer must be recording as a draw packet while its owning
** context has an active framebuffer scope.
**
** @param command_buffer  Command buffer being recorded.
** @param program         Finalized graphics program to bind.
*/
static inline void rtCmdUseGraphicsProgram(rt_command_buffer command_buffer, rt_graphics_program program);

/*!
** @brief Bind a buffer range to a uniform location for subsequent draws.
**
** Records into the command buffer that the shader uniform at @p location
** reads from `[offset, offset + size)` of @p buffer. @p buffer must have
** been defined with RT_BUFFER_USAGE_UNIFORM in its usage flags, and
** @p location must belong to the currently bound graphics program.
** The command buffer must be recording as a draw packet while its owning
** context has an active framebuffer scope.
**
** @param command_buffer  Command buffer being recorded.
** @param location        Uniform location obtained from the bound program.
** @param buffer          Buffer to read from.
** @param offset          Byte offset into @p buffer.
** @param size            Number of bytes visible through the binding.
*/
static inline void rtCmdUniformBuffer(rt_command_buffer command_buffer, rt_uniform_location location, rt_buffer buffer, usize offset, usize size);

/*!
** @brief Bind a sampled texture view to a uniform location for subsequent draws.
**
** Records into the command buffer that the shader sampler at @p location
** samples through @p texture_view. @p location must belong to the
** currently bound graphics program.
** The command buffer must be recording as a draw packet while its owning
** context has an active framebuffer scope.
**
** @param command_buffer  Command buffer being recorded.
** @param location        Uniform location obtained from the bound program.
** @param texture_view    Texture view to sample.
*/
static inline void rtCmdUniformTexture(rt_command_buffer command_buffer, rt_uniform_location location, rt_texture_view texture_view);

/*!
** @brief Bind a storage-buffer range to a uniform location for following draws.
**
** The shader uniform at @p location reads and writes
** [offset, offset + size) of @p buffer. @p location must belong to the
** currently bound graphics program. The command buffer must be recording as
** a draw packet while its owning context has an active framebuffer scope.
**
** @param command_buffer  Command buffer being recorded.
** @param location        Uniform location obtained from the bound program.
** @param buffer          Buffer to bind.
** @param offset          Byte offset into @p buffer.
** @param size            Number of bytes visible through the binding.
*/
static inline void rtCmdStorageBuffer(rt_command_buffer command_buffer, rt_uniform_location location, rt_buffer buffer, usize offset, usize size);

/*!
** @brief Bind a vertex buffer for subsequent draw commands.
**
** The buffer's layout is interpreted using the vertex layout set on the
** currently bound graphics program.
** The command buffer must be recording as a draw packet while its owning
** context has an active framebuffer scope.
**
** @param command_buffer  Command buffer being recorded.
** @param buffer          Buffer to read vertex data from.
** @param offset          Byte offset into @p buffer where vertex 0 begins.
*/
static inline void rtCmdBindVertexBuffer(rt_command_buffer command_buffer, rt_buffer buffer, usize offset);

/*!
** @brief Record a non-indexed draw.
**
** Draws @p vertex_count vertices starting at vertex @p first_vertex using
** the currently bound graphics program, vertex buffer, and uniform
** bindings. Child buffers inherit the context rendering compatibility.
** The command buffer must be recording as a draw packet while its owning
** context has an active framebuffer scope.
**
** @param command_buffer  Command buffer being recorded.
** @param vertex_count    Number of vertices to draw.
** @param first_vertex    Index of the first vertex (added to vertex IDs).
*/
static inline void rtCmdDraw(rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex);

/*!
** @brief Finish recording a child command buffer.
**
** Transitions the command buffer from recording to executable. The owning
** context may execute it only in the context position in which it began.
**
** @param command_buffer  Command buffer being recorded.
*/
static inline void rtCmdEnd(rt_command_buffer command_buffer);

/*!
** @brief Declare the context's one framebuffer scope.
**
** The logical scope opens on @p framebuffer. It fixes compatibility for all
** child draw packets begun while the scope is active. The pre-framebuffer
** position is reserved for a future generic-work command set; a context has
** at most one framebuffer scope.
**
** @param command_context  Context declaring the framebuffer scope.
** @param framebuffer      Framebuffer whose attachments will be rendered to.
*/
static inline void rtCommandContextBindFramebuffer(rt_command_context command_context, rt_framebuffer framebuffer);

/*!
** @brief Declare a color attachment clear for the context framebuffer scope.
**
** This is a load declaration, not a command recorded within a child packet.
** Call it after @ref rtCommandContextBindFramebuffer and before any child
** begins.
**
** @param command_context  Context with the active framebuffer scope.
** @param color_index      Color attachment slot to clear.
** @param r                Red.
** @param g                Green.
** @param b                Blue.
** @param a                Alpha.
*/
static inline void rtCommandContextClearColor(rt_command_context command_context, u32 color_index, f32 r, f32 g, f32 b, f32 a);

/*!
** @brief Declare a depth clear for the context framebuffer scope.
**
** This is a load declaration. Call it after
** @ref rtCommandContextBindFramebuffer and before any child begins.
**
** @param command_context  Context with the active framebuffer scope.
** @param depth            Depth clear value (typically in [0, 1]).
*/
static inline void rtCommandContextClearDepth(rt_command_context command_context, f32 depth);

/*!
** @brief Declare a stencil clear for the context framebuffer scope.
**
** This is a load declaration. Call it after
** @ref rtCommandContextBindFramebuffer and before any child begins.
**
** @param command_context  Context with the active framebuffer scope.
** @param stencil          Stencil clear value.
*/
static inline void rtCommandContextClearStencil(rt_command_context command_context, u32 stencil);

/*!
** @brief Declare the viewport used by every draw packet in the active scope.
**
** Call after @ref rtCommandContextBindFramebuffer and before any child begins.
** The viewport is context state because child packets must
** remain portable across backend secondary-list state rules.
**
** @param command_context  Context declaring the viewport.
** @param x                Viewport left edge in framebuffer pixels.
** @param y                Viewport top edge in framebuffer pixels.
** @param width            Viewport width in framebuffer pixels.
** @param height           Viewport height in framebuffer pixels.
** @param min_depth        Minimum depth value.
** @param max_depth        Maximum depth value.
*/
static inline void rtCommandContextSetViewport(rt_command_context command_context, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth);

/*!
** @brief Declare the scissor used by every draw packet in the active scope.
**
** Call after @ref rtCommandContextBindFramebuffer and before any child begins.
** A child command buffer cannot set a scissor.
**
** @param command_context  Context declaring the scissor.
** @param x                Scissor left edge in framebuffer pixels.
** @param y                Scissor top edge in framebuffer pixels.
** @param width            Scissor width in framebuffer pixels.
** @param height           Scissor height in framebuffer pixels.
*/
static inline void rtCommandContextSetScissor(rt_command_context command_context, u32 x, u32 y, u32 width, u32 height);

/*!
** @brief Append an ended child packet to the context's ordered submission.
**
** The current public graphics command set executes a draw packet in the active
** framebuffer scope with that scope's fixed compatibility. The pre-framebuffer
** position is reserved for a future generic-work command set. The context
** must be queue-bound and unsubmitted.
**
** @param command_context  Context receiving the child packet.
** @param command_buffer   Ended child buffer owned by the context.
*/
static inline void rtCommandContextExecute(rt_command_context command_context, rt_command_buffer command_buffer);

/*!
** @brief End the context's current rendering scope.
**
** Closes the framebuffer scope declared by the most recent
** @ref rtCommandContextBindFramebuffer. A context has one rendering scope.
** Only child buffers still recording prevent this call; executable children
** may remain available for execution until the scope closes.
**
** @param command_context  Context whose rendering scope will close.
*/
static inline void rtCommandContextEndRendering(rt_command_context command_context);

/*!
** @brief Get a queue offering the requested capability.
**
** Returns a borrowed handle to a backend-provided queue with the requested
** capability tier (transfer, compute, graphics). Queue handles are not
** owned by the caller and remain valid until @ref rtExit.
**
** If no queue with the requested capability exists, the function returns
** NULL without recording an error.
**
** @param capability  Required queue capability.
** @return Queue handle, or NULL when no matching queue is available.
*/
static inline rt_queue rtQueueQuery(enum rt_queue_capability capability);

/*!
** @brief Insert a GPU-side wait for a timepoint on a queue.
**
** Inserts a dependency on @p queue: every subsequent submission on
** @p queue will wait for @p timepoint to be signaled before it runs. The
** wait is enforced by the device, not by blocking the CPU. The dependency
** is persistent - subsequent submissions all observe it until the
** timepoint is reached.
**
** Waiting on a zero-initialized/already-reached timepoint records no
** dependency.
**
** @param queue      Queue to insert the wait on.
** @param timepoint  Timepoint that must be reached before subsequent
**                   submissions on @p queue may run.
*/
static inline void rtQueueWait(rt_queue queue, rt_timepoint timepoint);

/*!
** @brief Submit a completed command context.
**
** Hands the context's ordered submission to its bound queue. The context must
** have no active rendering scope and may be submitted only once per binding.
** A context may be empty or contain its one completed framebuffer scope.
** Ended child buffers retain their ownership;
** after this context is rebound, record each child again before executing it.
**
** The returned timepoint is associated with the context's queue and signals
** once this submission has completed on the GPU.
**
** @param command_context  Bound context with a completed ordered submission.
**
** @return Signal that fires when this submission has completed on the GPU.
**
** @error RT_DEVICE_LOST           The device was lost while queuing the
**                                 submission.
** @error RT_OUT_OF_DEVICE_MEMORY  Device allocation failed while queuing
**                                 the submission.
*/
static inline rt_timepoint rtCommandContextSubmit(rt_command_context command_context);

/*!
** @brief Flush any pending submissions on a queue to the GPU.
**
** @ref rtCommandContextSubmit is allowed to defer the actual handoff to the device;
** this call forces every pending submission on @p queue to be sent. Use it
** at the end of a frame, or before any CPU-side wait that requires the
** GPU to be making progress.
**
** @param queue  Queue to flush.
**
** @return Signal that fires when all flushed work has completed on the GPU.
**
** @error RT_DEVICE_LOST           The device was lost while flushing.
** @error RT_OUT_OF_DEVICE_MEMORY  Device allocation failed during the
**                                 flush.
*/
static inline rt_timepoint rtQueueFlush(rt_queue queue);

/*!
** @brief Block the CPU until a timepoint is reached.
**
** Returns once the GPU has signaled @p timepoint. The queue stored inside
** @p timepoint defines the synchronization domain. A zero-initialized or
** already-reached timepoint returns immediately.
**
** @param timepoint  Timepoint to wait for.
*/
static inline void rtTimepointWait(rt_timepoint timepoint);

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
static inline bool rtTimepointReached(rt_timepoint timepoint);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

#define RT_CORE_PROCEDURES(X) \
	X(rt_buffer           , rtBufferCreate                  , (void)                                                                                                                                                                                                                           ) \
	X(rt_timepoint        , rtBufferData                    , (rt_buffer buffer, enum rt_buffer_mode mode, enum rt_buffer_usage usage, u64 size, const void* data)                                                                                                                             ) \
	X(void                , rtBufferDestroy                 , (rt_buffer buffer)                                                                                                                                                                                                               ) \
	X(void                , rtBufferRead                    , (rt_buffer buffer, u64 offset, u64 size, void* data)                                                                                                                                                                             ) \
	X(rt_timepoint        , rtBufferSubdata                 , (rt_buffer buffer, u64 offset, u64 size, const void* data)                                                                                                                                                                       ) \
	X(void                , rtClearError                    , (void)                                                                                                                                                                                                                           ) \
	X(void                , rtCmdBegin                      , (rt_command_buffer command_buffer)                                                                                                                                                                                               ) \
	X(void                , rtCmdBindVertexBuffer           , (rt_command_buffer command_buffer, rt_buffer buffer, u64 offset)                                                                                                                                                                 ) \
	X(void                , rtCmdDraw                       , (rt_command_buffer command_buffer, u32 vertex_count, u32 first_vertex)                                                                                                                                                           ) \
	X(void                , rtCmdEnd                        , (rt_command_buffer command_buffer)                                                                                                                                                                                               ) \
	X(void                , rtCmdReset                      , (rt_command_buffer command_buffer)                                                                                                                                                                                               ) \
	X(void                , rtCmdStorageBuffer              , (rt_command_buffer command_buffer, rt_uniform_location location, rt_buffer buffer, u64 offset, u64 size)                                                                                                                         ) \
	X(void                , rtCmdUniformBuffer              , (rt_command_buffer command_buffer, rt_uniform_location location, rt_buffer buffer, u64 offset, u64 size)                                                                                                                         ) \
	X(void                , rtCmdUniformTexture             , (rt_command_buffer command_buffer, rt_uniform_location location, rt_texture_view texture_view)                                                                                                                                   ) \
	X(void                , rtCmdUseGraphicsProgram         , (rt_command_buffer command_buffer, rt_graphics_program program)                                                                                                                                                                  ) \
	X(void                , rtCommandBufferDestroy          , (rt_command_buffer command_buffer)                                                                                                                                                                                               ) \
	X(rt_command_buffer   , rtCommandContextAllocate        , (rt_command_context command_context)                                                                                                                                                                                             ) \
	X(void                , rtCommandContextBind            , (rt_command_context command_context, rt_queue queue)                                                                                                                                                                             ) \
	X(void                , rtCommandContextBindFramebuffer , (rt_command_context command_context, rt_framebuffer framebuffer)                                                                                                                                                                 ) \
	X(void                , rtCommandContextClearColor      , (rt_command_context command_context, u32 color_index, f32 r, f32 g, f32 b, f32 a)                                                                                                                                                ) \
	X(void                , rtCommandContextClearDepth      , (rt_command_context command_context, f32 depth)                                                                                                                                                                                  ) \
	X(void                , rtCommandContextClearStencil    , (rt_command_context command_context, u32 stencil)                                                                                                                                                                                ) \
	X(rt_command_context  , rtCommandContextCreate          , (void)                                                                                                                                                                                                                           ) \
	X(void                , rtCommandContextDestroy         , (rt_command_context command_context)                                                                                                                                                                                             ) \
	X(void                , rtCommandContextEndRendering    , (rt_command_context command_context)                                                                                                                                                                                             ) \
	X(void                , rtCommandContextExecute         , (rt_command_context command_context, rt_command_buffer command_buffer)                                                                                                                                                           ) \
	X(void                , rtCommandContextSetScissor      , (rt_command_context command_context, u32 x, u32 y, u32 width, u32 height)                                                                                                                                                        ) \
	X(void                , rtCommandContextSetViewport     , (rt_command_context command_context, u32 x, u32 y, u32 width, u32 height, f32 min_depth, f32 max_depth)                                                                                                                          ) \
	X(rt_timepoint        , rtCommandContextSubmit          , (rt_command_context command_context)                                                                                                                                                                                             ) \
	X(enum rt_error       , rtError                         , (void)                                                                                                                                                                                                                           ) \
	X(const char*         , rtErrorMessage                  , (void)                                                                                                                                                                                                                           ) \
	X(void                , rtExit                          , (void)                                                                                                                                                                                                                           ) \
	X(rt_texture_view     , rtFramebufferColorView          , (rt_framebuffer framebuffer, u32 slot)                                                                                                                                                                                           ) \
	X(rt_framebuffer      , rtFramebufferCreate             , (void)                                                                                                                                                                                                                           ) \
	X(void                , rtFramebufferDepthView          , (rt_framebuffer framebuffer, rt_texture_view view)                                                                                                                                                                               ) \
	X(void                , rtFramebufferDestroy            , (rt_framebuffer framebuffer)                                                                                                                                                                                                     ) \
	X(void                , rtFramebufferSetColorView       , (rt_framebuffer framebuffer, u32 slot, rt_texture_view view)                                                                                                                                                                     ) \
	X(const char*         , rtGetName                       , (void)                                                                                                                                                                                                                           ) \
	X(void                , rtGraphicsProgramBlendState     , (rt_graphics_program program, bool enabled, enum rt_blend_factor src_color, enum rt_blend_factor dst_color, enum rt_blend_op color_op, enum rt_blend_factor src_alpha, enum rt_blend_factor dst_alpha, enum rt_blend_op alpha_op)) \
	X(rt_graphics_program , rtGraphicsProgramCreate         , (void)                                                                                                                                                                                                                           ) \
	X(void                , rtGraphicsProgramDestroy        , (rt_graphics_program program)                                                                                                                                                                                                    ) \
	X(void                , rtGraphicsProgramFinalize       , (rt_graphics_program program)                                                                                                                                                                                                    ) \
	X(void                , rtGraphicsProgramLayout         , (rt_graphics_program program, const rt_vertex_layout* layout)                                                                                                                                                                    ) \
	X(void                , rtGraphicsProgramRasterState    , (rt_graphics_program program, enum rt_cull_mode cull_mode, enum rt_front_face front_face, enum rt_fill_mode fill_mode)                                                                                                           ) \
	X(void                , rtGraphicsProgramReset          , (rt_graphics_program program)                                                                                                                                                                                                    ) \
	X(void                , rtGraphicsProgramSource         , (rt_graphics_program program, u64 size, const void* data)                                                                                                                                                                        ) \
	X(rt_uniform_location , rtGraphicsProgramUniformLocation, (rt_graphics_program program, const char* name)                                                                                                                                                                                  ) \
	X(void                , rtInit                          , (const char* const* features, u32 feature_count)                                                                                                                                                                                 ) \
	X(void                , rtOutput                        , (const char* message, void* user_data)                                                                                                                                                                                           ) \
	X(enum rt_format_usage, rtQueryFormatCapabilities       , (enum rt_format format)                                                                                                                                                                                                          ) \
	X(rt_timepoint        , rtQueueFlush                    , (rt_queue queue)                                                                                                                                                                                                                 ) \
	X(rt_queue            , rtQueueQuery                    , (enum rt_queue_capability capability)                                                                                                                                                                                            ) \
	X(void                , rtQueueWait                     , (rt_queue queue, rt_timepoint timepoint)                                                                                                                                                                                         ) \
	X(void                , rtSetOutput                     , (PFN_rtOutput output, void* user_data)                                                                                                                                                                                           ) \
	X(rt_timepoint        , rtTextureCopy                   , (rt_texture src_texture, u32 src_mip, rt_texture dst_texture, u32 dst_mip)                                                                                                                                                       ) \
	X(rt_texture          , rtTextureCreate                 , (void)                                                                                                                                                                                                                           ) \
	X(rt_timepoint        , rtTextureData                   , (rt_texture texture, enum rt_texture_type type, u32 mip, u32 width, u32 height, u32 depth, enum rt_format format, const void* data)                                                                                              ) \
	X(void                , rtTextureDestroy                , (rt_texture texture)                                                                                                                                                                                                             ) \
	X(rt_timepoint        , rtTextureSubcopy                , (rt_texture src_texture, u32 src_mip, u32 src_x, u32 src_y, u32 src_z, rt_texture dst_texture, u32 dst_mip, u32 dst_x, u32 dst_y, u32 dst_z, u32 width, u32 height, u32 depth)                                                   ) \
	X(rt_timepoint        , rtTextureSubdata                , (rt_texture texture, u32 mip, u32 offset_x, u32 offset_y, u32 offset_z, u32 width, u32 height, u32 depth, const void* data)                                                                                                      ) \
	X(void                , rtTextureViewAddress            , (rt_texture_view texture_view, enum rt_address_mode address_u, enum rt_address_mode address_v, enum rt_address_mode address_w)                                                                                                   ) \
	X(void                , rtTextureViewAnisotropy         , (rt_texture_view texture_view, u32 max_anisotropy)                                                                                                                                                                               ) \
	X(void                , rtTextureViewBind               , (rt_texture_view texture_view, rt_texture texture)                                                                                                                                                                               ) \
	X(rt_timepoint        , rtTextureViewCopyToBuffer       , (rt_texture_view texture_view, rt_buffer buffer)                                                                                                                                                                                 ) \
	X(rt_texture_view     , rtTextureViewCreate             , (void)                                                                                                                                                                                                                           ) \
	X(void                , rtTextureViewDestroy            , (rt_texture_view texture_view)                                                                                                                                                                                                   ) \
	X(rt_extent_3d        , rtTextureViewExtent             , (rt_texture_view texture_view)                                                                                                                                                                                                   ) \
	X(void                , rtTextureViewFilter             , (rt_texture_view texture_view, enum rt_filter mag_filter, enum rt_filter min_filter, enum rt_mip_filter mip_filter)                                                                                                              ) \
	X(void                , rtTextureViewLod                , (rt_texture_view texture_view, f32 min_lod, f32 max_lod, f32 lod_bias)                                                                                                                                                           ) \
	X(bool                , rtTimepointReached              , (rt_timepoint timepoint)                                                                                                                                                                                                         ) \
	X(void                , rtTimepointWait                 , (rt_timepoint timepoint)                                                                                                                                                                                                         )
/* RT_CORE_PROCEDURES */

#define RT_DECLARE_CORE_PROCEDURE(return_type, name, parameters) typedef return_type (*PFN_##name) parameters; extern PFN_##name rt_##name;
RT_CORE_PROCEDURES(RT_DECLARE_CORE_PROCEDURE)
#undef RT_DECLARE_CORE_PROCEDURE

#define RT_DEFINE_CORE_PROCEDURE(return_type, name, parameters) static inline return_type name parameters { return rt_##name arguments; }
RT_CORE_PROCEDURES(RT_DEFINE_CORE_PROCEDURE)
#undef RT_DEFINE_CORE_PROCEDURE

#ifdef __cplusplus
}
#endif

#endif /* RUTILE_H */
