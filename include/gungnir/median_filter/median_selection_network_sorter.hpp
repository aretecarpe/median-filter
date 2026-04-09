#ifndef GUNGNIR_MEDIAN_FILTER_MEDIAN_SELECTION_NETWORK_SORTER_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_MEDIAN_SELECTION_NETWORK_SORTER_HEADER_INCLUDED

// A partial sorting network that only guarantees the median element
// (at position N/2) is placed correctly. Uses strictly fewer compare-and-swap
// operations than a full sort, while producing the same median value.
//
// The pruning is done entirely at compile time: the full sorting network is
// flattened, backwards dependency analysis identifies which compare-and-swap
// operations can influence the median position, and a new type-level topology
// is constructed from only those operations.
//
// Usage:
//   constexpr MedianSelectionNetworkSorter<49, SortingAlgorithm::bose_nelson_sort> sorter;
//   sorter(data, compare_op);
//   median = data[49 / 2];  // guaranteed correct

#include <gungnir/median_filter/sorting_network_sorter.hpp>
#include <gungnir/median_filter/impl/sorting/median_selection_network_builder.hpp>
#include <gungnir/median_filter/impl/unified_gpu_runtime.hpp>

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

namespace gungnir::median_filter {

template <std::size_t N, SortingAlgorithm Algorithm = SortingAlgorithm::bose_nelson_sort>
struct MedianSelectionNetworkSorter {
	static_assert(
		N > 0,
		"Size instantiated with MedianSelectionNetworkSorter must be greater than 0!"
	);

	using full_topology =
		typename impl::sorting::NetworkTopologySelector<N, Algorithm>::type;

	static constexpr std::size_t full_network_comparisons =
		impl::sorting::TotalCompareSwapPairs<full_topology>::value;

	static constexpr auto pruned_pairs =
		impl::sorting::build_median_selection_pairs<N, full_topology>();

	static constexpr std::size_t median_selection_comparisons =
		pruned_pairs.count;

	template <
		typename T,
		typename Compare = impl::sorting::CompareAndSwap<
			std::remove_pointer_t<T>,
			std::less<std::remove_pointer_t<T>>>
	>
	__device__ __host__ constexpr auto operator()(
		T begin, Compare comp = {}
	) const noexcept -> void {
		if constexpr (N > 1) {
			for (std::size_t i = 0; i < pruned_pairs.count; ++i) {
				comp(begin[pruned_pairs.first[i]], begin[pruned_pairs.second[i]]);
			}
		}
	}
};

} // namespace gungnir::median_filter

#endif
