#include <gungnir/median_filter/median_filter_2d.hpp>
#include <gungnir/tiffxx/tiffxx.hpp>

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

#include <gungnir/median_filter/impl/unified_gpu_runtime.hpp>

auto compare_images_raw(
	const float *img1,
	const float *img2,
	std::size_t num_elements
) -> void;

namespace kernel {

template <typename From, typename To>
__global__ void convert(const From *from, To *to, std::size_t pixels) {
	const auto idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx >= pixels)
		return;
	to[idx] = static_cast<To>(from[idx]);
}

} // namespace kernel

namespace medfilt = gungnir::median_filter;
namespace tiffxx = gungnir::tiffxx;

auto main(int argc, char **argv) -> int {
	if (argc != 4) {
		std::cerr << "Usage: " << argv[0] << " <input.tif> <output.tif> <truth_image>\n";
		return 1;
	}

	const auto input_path = argv[1];
	const auto output_path = argv[2];
	const auto truth_path = argv[3];

	// Read 16-bit TIFF
	auto [info, data_u16] = tiffxx::read<uint16_t>(input_path);
	const auto width = info.width;
	const auto height = info.height;
	const auto pixels = info.pixel_count();

	std::cout << "Read " << width << "x" << height
	          << " (" << info.bits_per_sample << "bps, "
	          << info.samples_per_pixel << "spp)\n";

	// Convert uint16 -> float on GPU
	uint16_t *d_u16 = nullptr;
	float *d_input = nullptr;
	float *d_output = nullptr;

	UNI_CHECK(uniMalloc(&d_u16, pixels * sizeof(uint16_t)));
	UNI_CHECK(uniMalloc(&d_input, pixels * sizeof(float)));
	UNI_CHECK(uniMalloc(&d_output, pixels * sizeof(float)));

	UNI_CHECK(uniMemcpy(d_u16, data_u16.data(), pixels * sizeof(uint16_t), uniMemcpyHostToDevice));

	constexpr std::size_t block = 256;
	const auto grid = static_cast<int>((pixels + block - 1) / block);
	kernel::convert<uint16_t, float><<<grid, block>>>(d_u16, d_input, pixels);
	UNI_CHECK(uniDeviceSynchronize());

	UNI_CHECK(uniFree(d_u16));

	constexpr int32_t filter_size = 7;
	constexpr auto algo = medfilt::SortingAlgorithm::bitonic_merge_sort;

	std::cout << "Full sort (bitonic)\n";
	for (std::size_t i = 0; i < 10; ++i) {
		auto start = std::chrono::high_resolution_clock::now();
		medfilt::median_filter_2d<float, filter_size, algo, false>(
			d_input,
			static_cast<uint32_t>(width * sizeof(float)),
			d_output,
			static_cast<uint32_t>(width * sizeof(float)),
			width,
			height
		);
		UNI_CHECK(uniDeviceSynchronize());
		auto stop = std::chrono::high_resolution_clock::now();
		auto us = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
		std::cout << "  " << us << "us\n";
	}

	// Copy result back to host as float
	std::vector<float> h_output(pixels);
	UNI_CHECK(uniMemcpy(h_output.data(), d_output, pixels * sizeof(float), uniMemcpyDeviceToHost));
	UNI_CHECK(uniDeviceSynchronize());

	UNI_CHECK(uniFree(d_input));
	UNI_CHECK(uniFree(d_output));

	// Write float TIFF
	tiffxx::write<float>(output_path, width, height, info.samples_per_pixel,
	                      h_output.data(), h_output.size());

	std::cout << "Wrote " << output_path << "\n";

	auto [truth_info, truth_data] = tiffxx::read<float>(truth_path);
	compare_images_raw(
		h_output.data(),
		truth_data.data(),
		h_output.size()
	);

	return 0;
}

auto compare_images_raw(
	const float* img1,
	const float* img2,
	size_t num_elements
) -> void {
if (num_elements == 0) {
        std::cerr << "      -> Error: Number of elements is zero.\n";
        return;
    }

    if (img1 == nullptr || img2 == nullptr) {
        std::cerr << "      -> Error: Null pointer provided.\n";
        return;
    }

    double sum_sq_diff = 0.0;
    float max_diff = 0.0f;

    for (size_t i = 0; i < num_elements; ++i) {
        // Calculate the absolute difference for the current pixel
        float diff = std::abs(img1[i] - img2[i]);

        // Update the maximum difference found so far
        if (diff > max_diff) {
            max_diff = diff;
        }

        // Accumulate the squared difference
        // Cast to double before multiplying to prevent overflow/precision loss
        sum_sq_diff += static_cast<double>(diff) * static_cast<double>(diff);
    }

    // Calculate the mean
    double mse = sum_sq_diff / static_cast<double>(num_elements);

    // Print out the results formatting exactly like the Python script
    std::cout << "      -> Comparison Results:\n";

    // Set cout to always show 4 decimal places
    std::cout << std::fixed << std::setprecision(4);

    std::cout << "         - Mean Squared Error (MSE): " << mse << "\n";
    std::cout << "         - Max Absolute Difference:  " << max_diff << "\n\n";

    // Optional: Reset cout to its default state if you print other things later
    std::cout << std::defaultfloat;
}
