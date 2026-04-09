#ifndef GUNGNIR_MEDIAN_FILTER_IMPL_CEILING_DIVISION_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_IMPL_CEILING_DIVISION_HEADER_INCLUDED

#include <gungnir/median_filter/impl/unified_gpu_runtime.hpp>

namespace gungnir::median_filter::impl {

template <typename T>
__forceinline__ __device__ __host__ constexpr T ceil_div(const T x, const T y) {
	return x == 0 ? 0 : 1 + ((x - 1) / y);
}

} // namespace gungnir::median_filter::impl

#endif
