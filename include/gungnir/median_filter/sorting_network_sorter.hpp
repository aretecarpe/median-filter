#ifndef GUNGNIR_MEDIAN_FILTER_SORTING_NETWORK_SORTER_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_SORTING_NETWORK_SORTER_HEADER_INCLUDED

#include <gungnir/median_filter/impl/sorting/compare_and_swap.hpp>
#include <gungnir/median_filter/impl/sorting/sorting_network_executor.hpp>
#include <gungnir/median_filter/impl/sorting/bitonic_merge_sort_network.hpp>
#include <gungnir/median_filter/impl/sorting/bose_nelson_sort_network.hpp>
#include <gungnir/median_filter/impl/sorting/batcher_odd_even_merge_sort_network.hpp>
#include <gungnir/median_filter/impl/sorting/insertion_sort_network.hpp>
#include <gungnir/median_filter/impl/sorting/bubble_sort_network.hpp>
#include <gungnir/median_filter/impl/sorting/size_optimized_sort_network.hpp>
#include <gungnir/median_filter/impl/unified_gpu_runtime.hpp>

#include <functional>
#include <type_traits>

namespace gungnir::median_filter {

enum class SortingAlgorithm {
	bitonic_merge_sort,
	bose_nelson_sort,
	batcher_odd_even_merge_sort,
	insertion_sort,
	bubble_sort,
	size_optimized_sort
};

namespace impl::sorting {

template <std::size_t N, SortingAlgorithm Algorithm>
struct NetworkTopologySelector;

template <std::size_t N>
struct NetworkTopologySelector<N, SortingAlgorithm::bitonic_merge_sort> {
	using type = typename BitonicMergeSortNetwork<0, N - 1, false>::type;
};

template <std::size_t N>
struct NetworkTopologySelector<N, SortingAlgorithm::bose_nelson_sort> {
	using type = typename BoseNelsonSortNetwork<N>::type;
};

template <std::size_t N>
struct NetworkTopologySelector<N, SortingAlgorithm::batcher_odd_even_merge_sort> {
	static_assert(
		detail::is_power_of_two(N),
		"Batcher odd-even merge sort requires power-of-two sizes!"
	);
	using type = typename BatcherOddEvenMergeSortNetwork<N>::type;
};

template <std::size_t N>
struct NetworkTopologySelector<N, SortingAlgorithm::insertion_sort> {
	using type = typename InsertionSortNetwork<N>::type;
};

template <std::size_t N>
struct NetworkTopologySelector<N, SortingAlgorithm::bubble_sort> {
	using type = typename BubbleSortNetwork<N>::type;
};

template <std::size_t N>
struct NetworkTopologySelector<N, SortingAlgorithm::size_optimized_sort> {
	using type = typename SizeOptimizedSortNetwork<N>::type;
};

} // namespace impl::sorting

template <std::size_t N, SortingAlgorithm Algorithm = SortingAlgorithm::bose_nelson_sort>
struct SortingNetworkSorter {
	static_assert(
		N > 0,
		"Size instantiated with SortingNetworkSorter must be greater than 0!"
	);

	using topology = typename impl::sorting::NetworkTopologySelector<N, Algorithm>::type;

	template <
		typename T,
		typename Compare = impl::sorting::CompareAndSwap<
			std::remove_pointer_t<T>,
			std::less<std::remove_pointer_t<T>>>
	>
	__device__ __host__ constexpr auto operator()(T begin, Compare comp = {}) const noexcept -> void {
		if constexpr (N > 1) { impl::sorting::execute(begin, comp, topology{}); }
	}
};

} // namespace gungnir::median_filter

#endif
