#ifndef GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_BUBBLE_SORT_NETWORK_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_BUBBLE_SORT_NETWORK_HEADER_INCLUDED

// Generates a bubble sort sorting network of following form:
// (0,1) (1,2) (2,3) ... (N-2,N-1)
// ...
// (0,1) (1,2) (2,3)
// (0,1) (1,2)
// (0,1)

#include <gungnir/median_filter/impl/sorting/sorting_network_executor.hpp>

#include <cstddef>
#include <utility>

namespace gungnir::median_filter::impl::sorting {

template <std::size_t N>
struct BubbleSortNetwork {
private:
	template <std::size_t... PairIndices>
	static auto generate_pass_pairs(const std::index_sequence<PairIndices...>)
		-> CompareSwapGroup<CompareSwapPair<PairIndices, PairIndices + 1>...>
	{
		return {};
	}

	template <std::size_t... PassIndices>
	static auto generate_passes(const std::index_sequence<PassIndices...>)
		-> SortingNetworkTopology<
			decltype(generate_pass_pairs(
				std::make_index_sequence<sizeof...(PassIndices) - PassIndices - 1>()
			))...
		>
	{
		return {};
	}

public:
	using type = decltype(generate_passes(std::make_index_sequence<N>()));
};

} // namespace gungnir::median_filter::impl::sorting

#endif
