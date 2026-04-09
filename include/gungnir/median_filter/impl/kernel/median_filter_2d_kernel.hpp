#ifndef GUNGNIR_MEDIAN_FILTER_IMPL_KERNEL_MEDIAN_FILTER_2D_KERNEL_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_IMPL_KERNEL_MEDIAN_FILTER_2D_KERNEL_HEADER_INCLUDED

#include <gungnir/median_filter/impl/ceiling_division.hpp>
#include <gungnir/median_filter/impl/bounds_check.hpp>
#include <gungnir/median_filter/impl/pitched_image.hpp>
#include <gungnir/median_filter/impl/sorting/compile_time_grid_loop.hpp>
#include <gungnir/median_filter/sorting_network_sorter.hpp>
#include <gungnir/median_filter/median_selection_network_sorter.hpp>

#include <cstdint>
#include <type_traits>

#include <gungnir/median_filter/impl/unified_gpu_runtime.hpp>

namespace gungnir::median_filter::impl::kernel {

template <typename T, int32_t FilterSize, SortingAlgorithm Algorithm, bool UseMedianSelection, int32_t BlockSize>
__global__ void median_filter_2d_kernel(
	const PitchedImage<const T> input,
	PitchedImage<T> output
) {
	constexpr int32_t filter_elements = FilterSize * FilterSize;
	constexpr int32_t filter_radius = FilterSize / 2;

	const int32_t lx = threadIdx.x;
	const int32_t ly = threadIdx.y;
	const int32_t bx = blockIdx.x;
	const int32_t by = blockIdx.y;

	extern __shared__ uint8_t smem[];
	constexpr int32_t block_x = BlockSize + 2 * filter_radius;
	constexpr int32_t block_y = BlockSize + 2 * filter_radius;
	constexpr int32_t row_pitch = impl::ceil_div(
		static_cast<uint32_t>(block_x * sizeof(T)),
		static_cast<uint32_t>(1)
	);

	constexpr auto load_operations_y = ceil_div(block_y, BlockSize);
	constexpr auto load_operations_x = ceil_div(block_x, BlockSize);
	const auto origin_x = BlockSize * bx - filter_radius;
	const auto origin_y = BlockSize * by - filter_radius;

	sorting::compile_time_grid_for_each<load_operations_y, load_operations_x>(
		[&](const auto mult) {
			const int32_t idxx = lx + mult.x * BlockSize;
			const int32_t idxy = ly + mult.y * BlockSize;
			const int32_t pixel_idxx = origin_x + idxx;
			const int32_t pixel_idxy = origin_y + idxy;

			T pixel_value{};
			if (
				is_within_bounds(
					pixel_idxx,
					pixel_idxy,
					static_cast<int32_t>(input.width()),
					static_cast<int32_t>(input.height())
				)
			) {
				pixel_value = input.get(pixel_idxx, pixel_idxy);
			}

			if (is_within_bounds(idxx, idxy, block_x, block_y)) {
				auto *row_start = reinterpret_cast<T*>(
					smem + (static_cast<ptrdiff_t>(idxy) * row_pitch)
				);
				row_start[idxx] = pixel_value;
			}
		}
	);

	__syncthreads();

	T filtered_value;
	{
		T local_pixels[filter_elements];
		T *local_pixels_ptr = local_pixels;

		sorting::compile_time_grid_for_each<FilterSize, FilterSize>([&](const auto idx) {
			const int32_t dy = idx.y - filter_radius;
			const int32_t dx = idx.x - filter_radius;

			const int32_t idxx = lx + filter_radius + dx;
			const int32_t idxy = ly + filter_radius + dy;

			auto *row_start = reinterpret_cast<const T*>(
				smem + (static_cast<ptrdiff_t>(idxy) * row_pitch)
			);
			*(local_pixels_ptr++) = row_start[idxx];
		});

		using SorterType = std::conditional_t<
			UseMedianSelection,
			MedianSelectionNetworkSorter<filter_elements, Algorithm>,
			SortingNetworkSorter<filter_elements, Algorithm>
		>;
		constexpr SorterType sorter;
		sorter(local_pixels, [](auto &a, auto &b) {
			const auto temp = a;
			a = min(a, b);
			b = max(temp, b);
		});

		filtered_value = *(local_pixels + filter_elements / 2);
	}

	const int32_t gidxx = lx + BlockSize * bx;
	const int32_t gidxy = ly + BlockSize * by;
	if (is_within_bounds(
		gidxx,
		gidxy,
		static_cast<int32_t>(input.width()),
		static_cast<int32_t>(input.height())
	)) {
		output.set(filtered_value, gidxx, gidxy);
	}
}

} // namespace gungnir::median_filter::impl::kernel

#endif
