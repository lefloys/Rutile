#pragma once

/*!
** @file rutile.hpp
** @brief Lightweight C++ value and byte-range adapters for @ref rutile.h.
**
** These wrappers keep Rutile's native handles and asynchronous timepoints
** unchanged. They provide strongly typed enum values and adapt C++ byte spans
** to the pointer-and-size data-transfer procedures in the C API.
*/

#include <rutile.h>

#include <cstddef>
#include <span>

namespace rt {
/*! @brief C++ mirror of the supported native texture formats. */
enum class Format : u32 {
	Unknown = 0,
	Rg32Float = 14,
	Rgb32Float = 15,
	Rgba32Float = 16,
};

/*! @brief C++ mirror of the native buffer allocation modes. */
enum class BufferMode : u32 {
	Static = 1,
	Dynamic = 2,
};

/*! @brief C++ mirror of the native buffer usage flags. */
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

/*! @brief C++ mirror of the native graphics culling modes. */
enum class CullMode : u32 {
	None = 0,
	Front = 1,
	Back = 2,
};

/*! @brief C++ mirror of the native front-face winding values. */
enum class FrontFace : u32 {
	CounterClockwise = 0,
	Clockwise = 1,
};

/*! @brief C++ mirror of the native polygon fill modes. */
enum class FillMode : u32 {
	Solid = 0,
	Wireframe = 1,
};

/*! @brief C++ mirror of the native vertex-input rates. */
enum class VertexRate : u32 {
	Vertex = 0,
	Instance = 1,
};

/*! @brief Convert a C++ texture format to its @ref rt_format value. */
constexpr rt_format native(Format value) {
	return static_cast<rt_format>(value);
}

/*! @brief Convert a C++ buffer mode to its @ref rt_buffer_mode value. */
constexpr rt_buffer_mode native(BufferMode value) {
	return static_cast<rt_buffer_mode>(value);
}

/*! @brief Convert C++ buffer usage flags to @ref rt_buffer_usage. */
constexpr rt_buffer_usage native(BufferUsage value) {
	return static_cast<rt_buffer_usage>(value);
}

/*! @brief Convert a C++ culling mode to its @ref rt_cull_mode value. */
constexpr rt_cull_mode native(CullMode value) {
	return static_cast<rt_cull_mode>(value);
}

/*! @brief Convert a C++ front-face value to its @ref rt_front_face value. */
constexpr rt_front_face native(FrontFace value) {
	return static_cast<rt_front_face>(value);
}

/*! @brief Convert a C++ fill mode to its @ref rt_fill_mode value. */
constexpr rt_fill_mode native(FillMode value) {
	return static_cast<rt_fill_mode>(value);
}

/*! @brief Convert a C++ vertex-input rate to @ref rt_vertex_rate. */
constexpr rt_vertex_rate native(VertexRate value) {
	return static_cast<rt_vertex_rate>(value);
}

/*!
** @brief Allocate @p size bytes of buffer storage without uploading data.
**
** This is the null-data form of @ref rtBufferData. The returned timepoint
** signals when the allocation request has completed.
*/
inline rt_timepoint BufferData(rt_buffer buffer, BufferMode mode, BufferUsage usage, u64 size) {
	return rtBufferData(buffer, native(mode), native(usage), size, nullptr);
}

/*!
** @brief Allocate buffer storage and upload the bytes in @p data.
**
** @p data is read only for the duration of the underlying @ref rtBufferData
** call; the caller retains ownership of the span. The returned timepoint
** signals completion of the upload.
*/
inline rt_timepoint BufferData(rt_buffer buffer, BufferMode mode, BufferUsage usage, std::span<const std::byte> data) {
	return rtBufferData(buffer, native(mode), native(usage), data.size(), data.data());
}

/*!
** @brief Upload @p data to an existing buffer starting at @p offset.
**
** The span is borrowed for the underlying @ref rtBufferSubdata call. The
** returned timepoint signals completion of the transfer.
*/
inline rt_timepoint BufferSubdata(rt_buffer buffer, u64 offset, std::span<const std::byte> data) {
	return rtBufferSubdata(buffer, offset, data.size(), data.data());
}

/*!
** @brief Read bytes from @p buffer into the caller-owned mutable span.
**
** The span supplies both the destination pointer and byte count. This wrapper
** preserves @ref rtBufferRead's completion and error behavior.
*/
inline void BufferRead(rt_buffer buffer, u64 offset, std::span<std::byte> data) {
	rtBufferRead(buffer, offset, data.size(), data.data());
}

/*!
** @brief Set graphics-program source from a caller-owned byte span.
**
** The source bytes are borrowed for the underlying @ref rtGraphicsProgramSource
** call; the caller retains ownership of the span.
*/
inline void GraphicsProgramSource(rt_graphics_program program, std::span<const std::byte> data) {
	rtGraphicsProgramSource(program, data.data(), data.size());
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
static_assert(static_cast<u32>(rt::VertexRate::Instance) == RT_VERTEX_RATE_INSTANCE);
