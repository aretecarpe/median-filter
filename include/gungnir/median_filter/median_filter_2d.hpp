#ifndef GUNGNIR_MEDIAN_FILTER_MEDIAN_FILTER_2D_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_MEDIAN_FILTER_2D_HEADER_INCLUDED

#include <gungnir/median_filter/impl/pitched_image.hpp>
#include <gungnir/median_filter/impl/kernel/median_filter_2d_kernel.hpp>

#include <cstddef>
#include <cstdint>

#include <gungnir/median_filter/impl/unified_gpu_runtime.hpp>

namespace gungnir::median_filter {

template <
	typename T,
	int32_t FilterSize,
	SortingAlgorithm Algorithm = SortingAlgorithm::bose_nelson_sort,
	bool UseMedianSelection = false,
	int32_t BlockSize = 16
>
inline auto median_filter_2d(
	const T *input,
	const uint32_t input_stride_in_bytes,
	T *output,
	const uint32_t output_stride_in_bytes,
	const uint32_t width,
	const uint32_t height,
	uniStream_t gpu_stream = 0
) -> void {
	static_assert(FilterSize % 2 == 1, "FilterSize only runs on odd sizes!");

	const dim3 block_size(BlockSize, BlockSize);
	const dim3 grid_size(
		impl::ceil_div(width, static_cast<uint32_t>(BlockSize)),
		impl::ceil_div(height, static_cast<uint32_t>(BlockSize))
	);

	constexpr auto calc_launch_param_smem = []() -> uint32_t {
		constexpr uint32_t filter_radius = FilterSize / 2;
		constexpr uint32_t block_x = BlockSize + 2  * filter_radius;
		constexpr uint32_t pitch_smem_size = impl::ceil_div(
			static_cast<uint32_t>(block_x * sizeof(T)),
			static_cast<uint32_t>(1)
		);
		return pitch_smem_size * (BlockSize + 2 * filter_radius);
	};

	impl::PitchedImage<const T> input_img(
		reinterpret_cast<const uint8_t*>(input),
		width,
		height,
		input_stride_in_bytes
	);

	impl::PitchedImage<T> output_img(
		reinterpret_cast<uint8_t*>(output),
		width,
		height,
		output_stride_in_bytes
	);
	impl::kernel::median_filter_2d_kernel<T, FilterSize, Algorithm, UseMedianSelection, BlockSize>
		<<<grid_size, block_size, calc_launch_param_smem(), gpu_stream>>>(
			input_img,
			output_img
		);
}

} // namespace gungnir::median_filter

#endif
