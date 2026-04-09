#ifndef GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_COMPARE_AND_SWAP_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_COMPARE_AND_SWAP_HEADER_INCLUDED

#include <gungnir/median_filter/impl/unified_gpu_runtime.hpp>

#include <type_traits>
#include <utility>

namespace gungnir::median_filter::impl::sorting {

template <typename T, typename Comparator>
struct CompareAndSwap {
	__device__ __host__ constexpr auto operator()(T &a, T &b) const noexcept(
		std::is_nothrow_swappable_v<T> &&
		std::is_nothrow_default_constructible_v<Comparator> &&
		noexcept(Comparator{}(b, a))
	) -> void {
		if (Comparator{}(b, a)) {
			using std::swap;
			swap(a, b);
		}
	}
};

} // namespace gungnir::median_filter::impl::sorting

#endif
