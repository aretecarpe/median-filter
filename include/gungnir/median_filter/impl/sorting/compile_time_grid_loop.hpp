#ifndef GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_COMPILE_TIME_GRID_LOOP_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_COMPILE_TIME_GRID_LOOP_HEADER_INCLUDED

#include <gungnir/median_filter/impl/unified_gpu_runtime.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

namespace gungnir::median_filter::impl::sorting {

template <int32_t X, int32_t Y>
struct CompileTimeGridCoord {
	static constexpr int32_t x = X;
	static constexpr int32_t y = Y;
};

template <int32_t Rows, int32_t Cols, typename F, std::size_t... Is>
__device__ __host__ constexpr auto compile_time_grid_loop_impl(
	F &&f, std::index_sequence<Is...>
) -> void {
	(f(CompileTimeGridCoord<Is % Cols, Is / Cols>{}), ...);
}

template <int32_t Rows, int32_t Cols, typename F>
__device__ __host__ constexpr auto compile_time_grid_for_each(F &&f) -> void {
	static_assert(Rows >= 0 && Cols >= 0, "Dimensions must be non-negative!");

	if constexpr (Rows * Cols > 0) {
		compile_time_grid_loop_impl<Rows, Cols>(
			std::forward<F>(f),
			std::make_index_sequence<Rows * Cols>{}
		);
	}
}

} // namespace gungnir::median_filter::impl::sorting

#endif
