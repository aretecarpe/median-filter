#ifndef GUNGNIR_MEDIAN_FILTER_IMPL_BOUNDS_CHECK_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_IMPL_BOUNDS_CHECK_HEADER_INCLUDED

#include <gungnir/median_filter/impl/unified_gpu_runtime.hpp>

namespace gungnir::median_filter::impl {

template <typename T>
__forceinline__ __device__ __host__ constexpr bool is_within_bounds(
	const T x,
	const T y,
	const T width,
	const T height
) {
	return (x >= 0) & (y >= 0) & (x < width) & (y < height);
}

} // namespace gungnir::median_filter::impl

#endif
