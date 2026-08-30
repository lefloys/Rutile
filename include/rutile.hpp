#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

using u08 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using usize = std::size_t;
using uptr = std::uintptr_t;
using i08 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using f32 = float;
using f64 = double;

struct rt_command_buffer_t;
struct rt_queue_t;
struct rt_framebuffer_t;
struct rt_program_t;
struct rt_buffer_t;
struct rt_texture_t;
struct rt_texture_view_t;
struct rt_sampler_t;
struct rt_swapchain_t;
struct rt_location_t;
struct GLFWwindow;

namespace rt {

inline constexpr auto null_handle = nullptr;
inline constexpr char feature_presentation[] = "RT_FEATURE_PRESENTATION";

constexpr u64 make_version(u16 major, u16 minor, u16 patch, u16 snapshot) noexcept {
	return (static_cast<u64>(major) << 48u) | (static_cast<u64>(minor) << 32u) | (static_cast<u64>(patch) << 16u) | static_cast<u64>(snapshot);
}

constexpr u16 version_major(u64 version) noexcept { return static_cast<u16>(version >> 48u); }
constexpr u16 version_minor(u64 version) noexcept { return static_cast<u16>(version >> 32u); }
constexpr u16 version_patch(u64 version) noexcept { return static_cast<u16>(version >> 16u); }
constexpr u16 version_snapshot(u64 version) noexcept { return static_cast<u16>(version); }
inline constexpr u64 header_version = make_version(0, 1, 0, 0);

enum class error : i32 {
	success = 0,
	out_of_host_memory = 1,
	out_of_device_memory = 2,
	improper_usage = 3,
	platform_failure = 4,
	device_lost = 5,
	already_initialized = 6,
	unsupported_platform = 7,
	no_backend = 8,
	unsupported_feature = 9,
	initialization_failed = 10,
	layer_not_present = 11,
	extension_not_present = 12,
	incompatible_driver = 13,
	shader_compilation_failed = 14,
	shader_link_failed = 15,
	feature_not_supported = 16,
};

enum class format : i32 {
	unknown = 0,
	r8_unorm = 1,
	rg8_unorm = 2,
	rgb8_unorm = 3,
	rgba8_unorm = 4,
	r16_unorm = 5,
	rg16_unorm = 6,
	rgb16_unorm = 7,
	rgba16_unorm = 8,
	r16_sfloat = 9,
	rg16_sfloat = 10,
	rgb16_sfloat = 11,
	rgba16_sfloat = 12,
	r32_sfloat = 13,
	rg32_sfloat = 14,
	rgb32_sfloat = 15,
	rgba32_sfloat = 16,
	r8_sint = 17,
	rg8_sint = 18,
	rgb8_sint = 19,
	rgba8_sint = 20,
	r16_sint = 21,
	rg16_sint = 22,
	rgb16_sint = 23,
	rgba16_sint = 24,
	r32_sint = 25,
	rg32_sint = 26,
	rgb32_sint = 27,
	rgba32_sint = 28,
	r8_uint = 29,
	rg8_uint = 30,
	rgb8_uint = 31,
	rgba8_uint = 32,
	r16_uint = 33,
	rg16_uint = 34,
	rgb16_uint = 35,
	rgba16_uint = 36,
	r32_uint = 37,
	rg32_uint = 38,
	rgb32_uint = 39,
	rgba32_uint = 40,
	d16_unorm = 41,
	d32_sfloat = 42,
	s8_uint = 43,
	d24_unorm_s8_uint = 44,
	d32_sfloat_s8_uint = 45,
};

enum class format_usage : u32 {
	none = 0x00,
	sampled = 0x01,
	color_attachment = 0x02,
	depth_attachment = 0x04,
	storage = 0x08,
	transfer_source = 0x10,
	transfer_destination = 0x20,
};

enum class memory_type : i32 {
	host = 1,
	device = 2,
};

enum class clear : u32 {
	none = 0x00,
	color = 0x01,
	depth = 0x02,
	stencil = 0x04,
};

enum class stage : u32 {
	none = 0x00,
	transfer = 0x01,
	vertex = 0x02,
	fragment = 0x04,
	compute = 0x08,
	color_attachment = 0x10,
	depth_stencil_attachment = 0x20,
	all = 0x3f,
};

enum class access_type : i32 {
	none = 0,
	read = 1,
	write = 2,
};

enum class texture_type : i32 {
	unknown = 0,
	texture_1d = 1,
	texture_2d = 2,
	texture_3d = 3,
	texture_1d_array = 4,
	texture_2d_array = 5,
};

enum class texture_aspect : u32 {
	none = 0x00,
	color = 0x01,
	depth = 0x02,
	stencil = 0x04,
};

enum class filter : i32 {
	nearest = 1,
	linear = 2,
};

enum class mip_filter : i32 {
	none = 0,
	nearest = 1,
	linear = 2,
};

enum class address_mode : i32 {
	clamp = 1,
	repeat = 2,
	mirror = 3,
};

enum class queue_capability : i32 {
	transfer = 1,
	compute = 2,
	graphics = 3,
};

enum class index_format : i32 {
	u16 = 1,
	u32 = 2,
};

enum class vertex_rate : i32 {
	vertex = 0,
	instance = 1,
};

enum class cull_mode : i32 {
	none = 0,
	front = 1,
	back = 2,
};

enum class front_face : i32 {
	counter_clockwise = 0,
	clockwise = 1,
};

enum class fill_mode : i32 {
	solid = 0,
	wireframe = 1,
};

enum class compare_op : i32 {
	never = 0,
	less = 1,
	equal = 2,
	less_equal = 3,
	greater = 4,
	not_equal = 5,
	greater_equal = 6,
	always = 7,
};

enum class blend_factor : i32 {
	zero = 0,
	one = 1,
	source_color = 2,
	one_minus_source_color = 3,
	destination_color = 4,
	one_minus_destination_color = 5,
	source_alpha = 6,
	one_minus_source_alpha = 7,
	destination_alpha = 8,
	one_minus_destination_alpha = 9,
};

enum class blend_op : i32 {
	add = 0,
	subtract = 1,
	reverse_subtract = 2,
	minimum = 3,
	maximum = 4,
};

template <typename T>
struct is_bitmask_enum : std::false_type {};

template <>
struct is_bitmask_enum<format_usage> : std::true_type {};
template <>
struct is_bitmask_enum<clear> : std::true_type {};
template <>
struct is_bitmask_enum<stage> : std::true_type {};
template <>
struct is_bitmask_enum<texture_aspect> : std::true_type {};

template <typename T>
concept bitmask_enum = std::is_enum_v<T> && is_bitmask_enum<T>::value;

template <typename T>
	requires bitmask_enum<T>
constexpr T operator|(T lhs, T rhs) noexcept {
	using value_type = std::underlying_type_t<T>;
	return static_cast<T>(static_cast<value_type>(lhs) | static_cast<value_type>(rhs));
}

template <typename T>
	requires bitmask_enum<T>
constexpr T operator&(T lhs, T rhs) noexcept {
	using value_type = std::underlying_type_t<T>;
	return static_cast<T>(static_cast<value_type>(lhs) & static_cast<value_type>(rhs));
}

template <typename T>
	requires bitmask_enum<T>
constexpr T operator^(T lhs, T rhs) noexcept {
	using value_type = std::underlying_type_t<T>;
	return static_cast<T>(static_cast<value_type>(lhs) ^ static_cast<value_type>(rhs));
}

template <typename T>
	requires bitmask_enum<T>
constexpr T operator~(T value) noexcept {
	using value_type = std::underlying_type_t<T>;
	return static_cast<T>(~static_cast<value_type>(value));
}

template <typename T>
	requires bitmask_enum<T>
constexpr T& operator|=(T& lhs, T rhs) noexcept {
	return lhs = lhs | rhs;
}

template <typename T>
	requires bitmask_enum<T>
constexpr T& operator&=(T& lhs, T rhs) noexcept {
	return lhs = lhs & rhs;
}

template <typename T>
	requires bitmask_enum<T>
constexpr T& operator^=(T& lhs, T rhs) noexcept {
	return lhs = lhs ^ rhs;
}

template <typename T>
	requires bitmask_enum<T>
constexpr bool any(T value) noexcept {
	return static_cast<std::underlying_type_t<T>>(value) != 0;
}

struct vertex_attribute {
	const char* name;
	usize offset;
	format format;
};

struct vertex_input {
	const vertex_attribute* attributes;
	usize attribute_count;
	usize stride;
	vertex_rate rate;
};

struct vertex_layout {
	const vertex_input* inputs;
	usize input_count;
};

struct timepoint {
	u64 value;
};

struct extent_3d {
	usize width;
	usize height;
	usize depth;
};

struct buffer_range {
	usize size;
	usize offset;
};

struct texture_range {
	texture_aspect aspects;
	usize base_mip;
	usize mip_count;
	usize base_layer;
	usize layer_count;
	extent_3d extent;
	extent_3d offset;
};

struct access {
	stage pipeline_stage;
	access_type type;
};

struct swapchain_acquire_result {
	rt_framebuffer_t* framebuffer;
	timepoint timepoint;
};

using output = void (*)(const char* message, void* user_data);
using proc = void*;

struct proc_chain {
	proc (*get_proc)(const proc_chain* chain, const char* name);
};

} // namespace rt

extern "C" {

rt::error rtLoad(const char* backend_name, const char* const* layer_names, usize layer_count);
rt::error rtLoadDevelopment(const char* backend_name, const char* const* layer_names, usize layer_count);
void rtUnload();
bool rtLoaded();
rt::proc rtGetProc(const char* name);

#if !defined(RT_TYPES_ONLY)

#if !defined(RT_API)
#define RT_API static inline
#endif

#define RT_CORE_PROCEDURES(X)                                                                     \
	X(void, rtInit, (const char* const* features, usize feature_count), (features, feature_count)) \
	X(void, rtExit, (), ())                                                                         \
	X(u64, rtVersion, (), ())                                                                       \
	X(void, rtSetOutput, (rt::output output, void* user_data), (output, user_data))                 \
	X(rt::error, rtError, (), ())                                                                   \
	X(const char*, rtErrorMessage, (), ())                                                          \
	X(void, rtClearError, (), ())                                                                   \
	X(const char*, rtGetName, (), ())

#define RT_COMMAND_BUFFER_EXTENSION_PROCEDURES(X)                                                                                                                                                                                                  \
	X(rt_command_buffer_t*, rtCommandBufferCreate, (), ())                                                                                                                                                                                           \
	X(void, rtCommandBufferDestroy, (rt_command_buffer_t* command_buffer), (command_buffer))                                                                                                                                                         \
	X(void, rtCommandBufferReset, (rt_command_buffer_t* command_buffer), (command_buffer))                                                                                                                                                           \
	X(void, rtCommandBufferBegin, (rt_command_buffer_t* command_buffer), (command_buffer))                                                                                                                                                           \
	X(void, rtCommandBufferContinue, (rt_command_buffer_t* command_buffer), (command_buffer))                                                                                                                                                        \
	X(void, rtCommandBufferContinueRendering, (rt_command_buffer_t* command_buffer), (command_buffer))                                                                                                                                               \
	X(void, rtCommandBufferEnd, (rt_command_buffer_t* command_buffer), (command_buffer))                                                                                                                                                             \
	X(void, rtCmdExecute, (rt_command_buffer_t* command_buffer, rt_command_buffer_t* secondary), (command_buffer, secondary))                                                                                                                       \
	X(void, rtCmdBeginRendering, (rt_command_buffer_t* command_buffer, rt_framebuffer_t* framebuffer), (command_buffer, framebuffer))                                                                                                               \
	X(void, rtCmdClearColor, (rt_command_buffer_t* command_buffer, rt_location_t* location, f32 r, f32 g, f32 b, f32 a), (command_buffer, location, r, g, b, a))                                                                                    \
	X(void, rtCmdClearDepth, (rt_command_buffer_t* command_buffer, f32 depth), (command_buffer, depth))                                                                                                                                              \
	X(void, rtCmdClearStencil, (rt_command_buffer_t* command_buffer, usize stencil), (command_buffer, stencil))                                                                                                                                      \
	X(void, rtCmdClear, (rt_command_buffer_t* command_buffer, rt::clear attachments), (command_buffer, attachments))                                                                                                                                \
	X(void, rtCmdSetViewport, (rt_command_buffer_t* command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth), (command_buffer, x, y, width, height, min_depth, max_depth))                                         \
	X(void, rtCmdSetScissor, (rt_command_buffer_t* command_buffer, usize x, usize y, usize width, usize height), (command_buffer, x, y, width, height))                                                                                              \
	X(void, rtCmdEndRendering, (rt_command_buffer_t* command_buffer), (command_buffer))                                                                                                                                                              \
	X(void, rtCmdDraw, (rt_command_buffer_t* command_buffer, usize vertex_count, usize first_vertex), (command_buffer, vertex_count, first_vertex))                                                                                                  \
	X(void, rtCmdDrawInstanced, (rt_command_buffer_t* command_buffer, usize vertex_count, usize instance_count, usize first_vertex, usize first_instance), (command_buffer, vertex_count, instance_count, first_vertex, first_instance))             \
	X(void, rtCmdDrawIndexed, (rt_command_buffer_t* command_buffer, usize index_count, usize first_index, usize vertex_offset), (command_buffer, index_count, first_index, vertex_offset))                                                           \
	X(void, rtCmdDrawIndexedInstanced, (rt_command_buffer_t* command_buffer, usize index_count, usize instance_count, usize first_index, usize vertex_offset, usize first_instance), (command_buffer, index_count, instance_count, first_index, vertex_offset, first_instance))

#define RT_QUEUE_EXTENSION_PROCEDURES(X)                                                                                                  \
	X(rt_queue_t*, rtQueueCreate, (rt::queue_capability capability), (capability))                                                          \
	X(void, rtQueueDestroy, (rt_queue_t* queue), (queue))                                                                                   \
	X(void, rtQueueWait, (rt_queue_t* queue, rt::timepoint timepoint), (queue, timepoint))                                                  \
	X(rt::timepoint, rtQueueSubmit, (rt_queue_t* queue, rt_command_buffer_t* command_buffer), (queue, command_buffer))                      \
	X(rt::timepoint, rtQueueFlush, (rt_queue_t* queue), (queue))                                                                            \
	X(void, rtTimepointWait, (rt::timepoint timepoint), (timepoint))                                                                        \
	X(bool, rtTimepointReached, (rt::timepoint timepoint), (timepoint))

#define RT_FRAMEBUFFER_EXTENSION_PROCEDURES(X)                                                                                                                        \
	X(rt_framebuffer_t*, rtFramebufferCreate, (), ())                                                                                                                  \
	X(void, rtFramebufferDestroy, (rt_framebuffer_t* framebuffer), (framebuffer))                                                                                       \
	X(rt_texture_view_t*, rtFramebufferColorView, (rt_framebuffer_t* framebuffer, rt_location_t* location), (framebuffer, location))                                    \
	X(void, rtFramebufferSetColorView, (rt_framebuffer_t* framebuffer, rt_texture_view_t* view, rt_location_t* location), (framebuffer, view, location))                 \
	X(void, rtFramebufferSetDepthView, (rt_framebuffer_t* framebuffer, rt_texture_view_t* view), (framebuffer, view))                                                    \
	X(void, rtFramebufferSetStencilView, (rt_framebuffer_t* framebuffer, rt_texture_view_t* view), (framebuffer, view))

#define RT_PROGRAM_EXTENSION_PROCEDURES(X)                                                                                                                                                                                                                                                                                    \
	X(void, rtCmdUseProgram, (rt_command_buffer_t* command_buffer, rt_program_t* program), (command_buffer, program))                                                                                                                                                                                                           \
	X(rt_program_t*, rtProgramCreate, (), ())                                                                                                                                                                                                                                                                                    \
	X(void, rtProgramDestroy, (rt_program_t* program), (program))                                                                                                                                                                                                                                                                \
	X(void, rtProgramSetLayout, (rt_program_t* program, const rt::vertex_layout* layout), (program, layout))                                                                                                                                                                                                                      \
	X(void, rtProgramSource, (rt_program_t* program, const char* entry_point, const u08* bytes, usize byte_size), (program, entry_point, bytes, byte_size))                                                                                                                                                                      \
	X(void, rtProgramSetRasterState, (rt_program_t* program, rt::cull_mode cull_mode, rt::front_face front_face, rt::fill_mode fill_mode), (program, cull_mode, front_face, fill_mode))                                                                                                                                          \
	X(void, rtProgramSetBlendState, (rt_program_t* program, bool enabled, rt::blend_factor src_color, rt::blend_factor dst_color, rt::blend_op color_op, rt::blend_factor src_alpha, rt::blend_factor dst_alpha, rt::blend_op alpha_op), (program, enabled, src_color, dst_color, color_op, src_alpha, dst_alpha, alpha_op)) \
	X(void, rtProgramFinalize, (rt_program_t* program), (program))                                                                                                                                                                                                                                                                \
	X(rt_location_t*, rtProgramUniformLocation, (rt_program_t* program, const char* name), (program, name))                                                                                                                                                                                                                       \
	X(rt_location_t*, rtProgramInputLocation, (rt_program_t* program, const rt::vertex_attribute* attributes, usize attribute_count), (program, attributes, attribute_count))                                                                                                                                                    \
	X(rt_location_t*, rtProgramOutputLocation, (rt_program_t* program, const char* name), (program, name))

#define RT_BUFFER_EXTENSION_PROCEDURES(X)                                                                                                                                                                                                          \
	X(void, rtCmdBindBuffer, (rt_command_buffer_t* command_buffer, rt_location_t* location, rt_buffer_t* buffer, rt::buffer_range range), (command_buffer, location, buffer, range))                                                                  \
	X(void, rtCmdVertexBuffer, (rt_command_buffer_t* command_buffer, rt_location_t* location, rt_buffer_t* buffer, rt::buffer_range range), (command_buffer, location, buffer, range))                                                                \
	X(void, rtCmdIndexBuffer, (rt_command_buffer_t* command_buffer, rt_buffer_t* buffer, rt::buffer_range range, rt::index_format format), (command_buffer, buffer, range, format))                                                                   \
	X(rt_buffer_t*, rtBufferCreate, (), ())                                                                                                                                                                                                           \
	X(void, rtBufferDestroy, (rt_buffer_t* buffer), (buffer))                                                                                                                                                                                         \
	X(void, rtBufferResize, (rt_buffer_t* buffer, rt::memory_type memory_type, usize size), (buffer, memory_type, size))                                                                                                                              \
	X(void, rtBufferRead, (rt_buffer_t* buffer, rt::buffer_range range, u08* data, usize data_size), (buffer, range, data, data_size))                                                                                                                \
	X(u08*, rtBufferMap, (rt_buffer_t* buffer, rt::buffer_range range), (buffer, range))                                                                                                                                                               \
	X(void, rtBufferUnmap, (rt_buffer_t* buffer), (buffer))                                                                                                                                                                                           \
	X(void, rtCmdBufferData, (rt_command_buffer_t* command_buffer, rt_buffer_t* buffer, rt::buffer_range range, const u08* data), (command_buffer, buffer, range, data))                                                                              \
	X(void, rtCmdBufferCopy, (rt_command_buffer_t* command_buffer, rt_buffer_t* src, rt::buffer_range src_range, rt_buffer_t* dst, rt::buffer_range dst_range), (command_buffer, src, src_range, dst, dst_range))                                     \
	X(void, rtCmdBufferCopyToTexture, (rt_command_buffer_t* command_buffer, rt_buffer_t* src, rt::buffer_range src_range, rt_texture_t* dst, rt::texture_range dst_range), (command_buffer, src, src_range, dst, dst_range))                          \
	X(void, rtCmdBufferBarrier, (rt_command_buffer_t* command_buffer, rt_buffer_t* buffer, rt::buffer_range range, rt::access src, rt::access dst), (command_buffer, buffer, range, src, dst))

#define RT_EXT_TEXTURE_PROCEDURES(X)                                                                                                                                                                                                               \
	X(void, rtCmdBindTexture, (rt_command_buffer_t* command_buffer, rt_location_t* location, rt_texture_view_t* texture_view), (command_buffer, location, texture_view))                                                                              \
	X(void, rtCmdBindSampler, (rt_command_buffer_t* command_buffer, rt_location_t* location, rt_sampler_t* sampler), (command_buffer, location, sampler))                                                                                             \
	X(rt_texture_t*, rtTextureCreate, (), ())                                                                                                                                                                                                         \
	X(void, rtTextureDestroy, (rt_texture_t* texture), (texture))                                                                                                                                                                                     \
	X(void, rtTextureResize, (rt_texture_t* texture, rt::texture_type type, rt::format format, rt::extent_3d extent, usize mip_count), (texture, type, format, extent, mip_count))                                                                    \
	X(void, rtCmdTextureCopy, (rt_command_buffer_t* command_buffer, rt_texture_t* src, rt::texture_range src_range, rt_texture_t* dst, rt::texture_range dst_range), (command_buffer, src, src_range, dst, dst_range))                                \
	X(void, rtCmdTextureData, (rt_command_buffer_t* command_buffer, rt_texture_t* texture, rt::texture_range range, const u08* data), (command_buffer, texture, range, data))                                                                         \
	X(void, rtCmdTextureCopyToBuffer, (rt_command_buffer_t* command_buffer, rt_texture_t* src, rt::texture_range src_range, rt_buffer_t* dst, rt::buffer_range dst_range), (command_buffer, src, src_range, dst, dst_range))                          \
	X(void, rtCmdTextureBarrier, (rt_command_buffer_t* command_buffer, rt_texture_t* texture, rt::texture_range range, rt::access src, rt::access dst), (command_buffer, texture, range, src, dst))                                                   \
	X(rt_texture_view_t*, rtTextureViewCreate, (), ())                                                                                                                                                                                               \
	X(void, rtTextureViewDestroy, (rt_texture_view_t* texture_view), (texture_view))                                                                                                                                                                  \
	X(rt::extent_3d, rtTextureViewExtent, (rt_texture_view_t* texture_view), (texture_view))                                                                                                                                                           \
	X(void, rtTextureViewSetTexture, (rt_texture_view_t* texture_view, rt_texture_t* texture), (texture_view, texture))                                                                                                                               \
	X(void, rtTextureViewRead, (rt_texture_view_t* texture_view, rt::texture_range range, u08* data, usize data_size), (texture_view, range, data, data_size))                                                                                         \
	X(rt_sampler_t*, rtSamplerCreate, (), ())                                                                                                                                                                                                         \
	X(void, rtSamplerDestroy, (rt_sampler_t* sampler), (sampler))                                                                                                                                                                                     \
	X(void, rtSamplerSetFilter, (rt_sampler_t* sampler, rt::filter mag_filter, rt::filter min_filter, rt::mip_filter mip_filter), (sampler, mag_filter, min_filter, mip_filter))                                                                     \
	X(void, rtSamplerSetAddress, (rt_sampler_t* sampler, rt::address_mode address_u, rt::address_mode address_v, rt::address_mode address_w), (sampler, address_u, address_v, address_w))                                                            \
	X(void, rtSamplerSetAnisotropy, (rt_sampler_t* sampler, usize max_anisotropy), (sampler, max_anisotropy))                                                                                                                                          \
	X(void, rtSamplerSetLod, (rt_sampler_t* sampler, f32 min_lod, f32 max_lod, f32 lod_bias), (sampler, min_lod, max_lod, lod_bias))

#define RT_CORE_EXTENSION_PROCEDURES(X)       \
	RT_COMMAND_BUFFER_EXTENSION_PROCEDURES(X) \
	RT_QUEUE_EXTENSION_PROCEDURES(X)          \
	RT_FRAMEBUFFER_EXTENSION_PROCEDURES(X)    \
	RT_PROGRAM_EXTENSION_PROCEDURES(X)        \
	RT_BUFFER_EXTENSION_PROCEDURES(X)         \
	RT_EXT_TEXTURE_PROCEDURES(X)

#define RT_PROCEDURES(X)  \
	RT_CORE_PROCEDURES(X) \
	RT_CORE_EXTENSION_PROCEDURES(X)

#define RT_DECLARE_PROCEDURE(return_type, name, parameters, arguments) \
	using PFN_##name = return_type (*) parameters;                       \
	extern PFN_##name rt_##name;                                        \
	RT_API return_type name parameters { return rt_##name arguments; }

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4098)
#endif
RT_PROCEDURES(RT_DECLARE_PROCEDURE)
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#undef RT_DECLARE_PROCEDURE

void rtLoad_RT_EXT_SWAPCHAIN();
void rtLoad_RT_EXT_GLFW();

#define RT_EXT_SWAPCHAIN_PROCEDURES(X)                                                                                               \
	X(rt_swapchain_t*, rtSwapchainCreate, (), ())                                                                                     \
	X(void, rtSwapchainDestroy, (rt_swapchain_t* swapchain), (swapchain))                                                             \
	X(void, rtSwapchainResize, (rt_swapchain_t* swapchain, u32 width, u32 height), (swapchain, width, height))                         \
	X(rt::swapchain_acquire_result, rtSwapchainAcquire, (rt_swapchain_t* swapchain), (swapchain))                                     \
	X(void, rtSwapchainPresent, (rt_swapchain_t* swapchain, rt::timepoint rendered), (swapchain, rendered))

#define RT_EXT_GLFW_PROCEDURES(X) \
	X(void, rtSwapchainBindGLFW, (rt_swapchain_t* swapchain, GLFWwindow* window), (swapchain, window))

#define RT_DECLARE_EXTENSION_PROCEDURE(return_type, name, parameters, arguments) \
	using PFN_##name = return_type (*) parameters;                                 \
	extern PFN_##name rt_##name;                                                   \
	RT_API return_type name parameters { return rt_##name arguments; }

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4098)
#endif
RT_EXT_SWAPCHAIN_PROCEDURES(RT_DECLARE_EXTENSION_PROCEDURE)
RT_EXT_GLFW_PROCEDURES(RT_DECLARE_EXTENSION_PROCEDURE)
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif // !RT_TYPES_ONLY

} // extern "C"
