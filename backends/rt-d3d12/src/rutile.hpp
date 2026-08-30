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

namespace rt {

inline constexpr auto null_handle = nullptr;
inline constexpr char feature_presentation[] = "RT_FEATURE_PRESENTATION";

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

struct location;

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

} // namespace rt
