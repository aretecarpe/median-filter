#ifndef GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_BATCHER_ODD_EVEN_MERGE_SORT_NETWORK_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_BATCHER_ODD_EVEN_MERGE_SORT_NETWORK_HEADER_INCLUDED

// Generates a sorting network based on the construction scheme by Ken Batcher
// (odd-even merge sort). Only valid for power-of-two sizes.

#include <gungnir/median_filter/impl/sorting/sorting_network_executor.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace gungnir::median_filter::impl::sorting {

namespace detail {

constexpr bool is_power_of_two(const std::size_t n) {
	return (n > 0) && !(n & (n - 1));
}

template <typename T>
constexpr T constexpr_ceil_div(const T x, const T y) {
	return T{1} + ((x - T{1}) / y);
}

} // namespace detail

template <std::size_t N, typename = void>
struct BatcherOddEvenMergeSortNetwork;

template <std::size_t N>
struct BatcherOddEvenMergeSortNetwork<N, std::enable_if_t<detail::is_power_of_two(N)>> {
private:
	template <std::size_t I, std::size_t J, std::size_t R, typename = void>
	struct GenerateMerge {
		using type = CompareSwapGroup<CompareSwapPair<I, (I + R)>>;
	};

	template <std::size_t I, std::size_t J, std::size_t R>
	struct GenerateMerge<I, J, R, std::enable_if_t<(R * 2 <= J - I)>> {
		static constexpr std::size_t step = R * 2;

		template <std::size_t... Indices>
		static auto generate_pairs(std::index_sequence<Indices...>)
			-> CompareSwapGroup<CompareSwapPair<I + R + Indices * step, I + R + Indices * step + R>...>
		{
			return {};
		}

		using type = CompareSwapGroup<
			typename GenerateMerge<I, J, step>::type,
			typename GenerateMerge<I + R, J, step>::type,
			decltype(generate_pairs(
				std::make_index_sequence<detail::constexpr_ceil_div((J - R) - (I + R), step)>()
			))
		>;
	};

	template <std::size_t Lo, std::size_t Hi, typename = void>
	struct GenerateSort {
		using type = CompareSwapGroup<>;
	};

	template <std::size_t Lo, std::size_t Hi>
	struct GenerateSort<Lo, Hi, std::enable_if_t<(Hi - Lo >= 1)>> {
		static constexpr std::size_t Mid = Lo + (Hi - Lo) / 2;

		using type = CompareSwapGroup<
			typename GenerateSort<Lo, Mid>::type,
			typename GenerateSort<Mid + 1, Hi>::type,
			typename GenerateMerge<Lo, Hi, 1>::type
		>;
	};

public:
	using type = SortingNetworkTopology<typename GenerateSort<0, N - 1>::type>;
};

} // namespace gungnir::median_filter::impl::sorting

#endif
