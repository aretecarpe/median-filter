#ifndef GUNGNIR_MEDIAN_FILTER_IMPL_PITCHED_IMAGE_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_IMPL_PITCHED_IMAGE_HEADER_INCLUDED

#include <gungnir/median_filter/impl/unified_gpu_runtime.hpp>

#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace gungnir::median_filter::impl {

template <typename T>
class PitchedImage {
private:
	using pointer_value_type = typename std::
		conditional<std::is_const<T>::value, const uint8_t, uint8_t>::type;

	pointer_value_type *image_ptr_;
	uint32_t image_width_;
	uint32_t image_height_;
	uint32_t pitch_in_bytes_;

public:
	explicit __device__ __host__ constexpr PitchedImage(
		pointer_value_type *image_ptr,
		const uint32_t image_width,
		const uint32_t image_height,
		const uint32_t pitch_in_bytes
	) noexcept :
		image_ptr_(image_ptr),
		image_width_(image_width),
		image_height_(image_height),
		pitch_in_bytes_(pitch_in_bytes)
	{ }

	[[nodiscard]] __device__ __host__ constexpr auto width() const noexcept
		-> uint32_t {
		return image_width_;
	}

	[[nodiscard]] __device__ __host__ constexpr auto height() const noexcept
		-> uint32_t {
		return image_height_;
	}

	[[nodiscard]] __device__ __host__ constexpr auto pitch_in_bytes()
		const noexcept -> uint32_t {
		return pitch_in_bytes_;
	}

	[[nodiscard]] __forceinline__ __device__ __host__ constexpr auto
		get(const uint32_t x, const uint32_t y) const -> const T& {
		auto *row_start = reinterpret_cast<const T*>(
			image_ptr_ + (static_cast<std::size_t>(y) * pitch_in_bytes_)
		);
		return row_start[x];
	}

	__forceinline__ __device__ __host__ constexpr auto
		set(const T value, const uint32_t x, const uint32_t y) const -> void {
		auto *row_start = reinterpret_cast<T*>(
			image_ptr_ + (static_cast<std::size_t>(y) * pitch_in_bytes_)
		);
		row_start[x] = value;
	}
};

} // namespace gungnir::median_filter::impl

#endif
