#pragma once

#include <rutile.h>

#include <cstddef>
#include <span>

namespace rt {
	enum class Format : u32 {
		Unknown = 0,
		Rg32Float = 14,
		Rgb32Float = 15,
		Rgba32Float = 16,
	};

	enum class BufferMode : u32 {
		Static = 1,
		Dynamic = 2,
	};

	enum class BufferUsage : u32 {
		None = 0x00,
		Staging = 0x01,
		Vertex = 0x02,
		Index = 0x04,
		Uniform = 0x08,
		Storage = 0x10,
		TransferSrc = 0x20,
		TransferDst = 0x40,
	};

	enum class CullMode : u32 {
		None = 0,
		Front = 1,
		Back = 2,
	};

	enum class FrontFace : u32 {
		CounterClockwise = 0,
		Clockwise = 1,
	};

	enum class FillMode : u32 {
		Solid = 0,
		Wireframe = 1,
	};

	constexpr rt_format native(Format value) {
		return static_cast<rt_format>(value);
	}

	constexpr rt_buffer_mode native(BufferMode value) {
		return static_cast<rt_buffer_mode>(value);
	}

	constexpr rt_buffer_usage native(BufferUsage value) {
		return static_cast<rt_buffer_usage>(value);
	}

	constexpr rt_cull_mode native(CullMode value) {
		return static_cast<rt_cull_mode>(value);
	}

	constexpr rt_front_face native(FrontFace value) {
		return static_cast<rt_front_face>(value);
	}

	constexpr rt_fill_mode native(FillMode value) {
		return static_cast<rt_fill_mode>(value);
	}

	inline rt_timepoint BufferData(rt_buffer buffer, BufferMode mode, BufferUsage usage, u64 size) {
		return rtBufferData(buffer, native(mode), native(usage), size, nullptr);
	}

	inline rt_timepoint BufferData(rt_buffer buffer, BufferMode mode, BufferUsage usage, std::span<const std::byte> data) {
		return rtBufferData(buffer, native(mode), native(usage), data.size(), data.data());
	}

	inline rt_timepoint BufferSubdata(rt_buffer buffer, u64 offset, std::span<const std::byte> data) {
		return rtBufferSubdata(buffer, offset, data.size(), data.data());
	}

	inline void BufferRead(rt_buffer buffer, u64 offset, std::span<std::byte> data) {
		rtBufferRead(buffer, offset, data.size(), data.data());
	}

	inline void GraphicsProgramSource(rt_graphics_program program, std::span<const std::byte> data) {
		rtGraphicsProgramSource(program, data.size(), data.data());
	}
} // namespace rt

static_assert(static_cast<u32>(rt::Format::Unknown) == RT_FORMAT_UNKNOWN);
static_assert(static_cast<u32>(rt::Format::Rg32Float) == RT_RG32_SFLOAT);
static_assert(static_cast<u32>(rt::Format::Rgb32Float) == RT_RGB32_SFLOAT);
static_assert(static_cast<u32>(rt::Format::Rgba32Float) == RT_RGBA32_SFLOAT);
static_assert(static_cast<u32>(rt::BufferMode::Static) == RT_BUFFER_STATIC);
static_assert(static_cast<u32>(rt::BufferMode::Dynamic) == RT_BUFFER_DYNAMIC);
static_assert(static_cast<u32>(rt::BufferUsage::Storage) == RT_BUFFER_USAGE_STORAGE);
static_assert(static_cast<u32>(rt::CullMode::Back) == RT_CULL_BACK);
static_assert(static_cast<u32>(rt::FrontFace::Clockwise) == RT_FRONT_FACE_CW);
static_assert(static_cast<u32>(rt::FillMode::Wireframe) == RT_FILL_WIREFRAME);
