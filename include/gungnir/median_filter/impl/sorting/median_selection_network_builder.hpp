#ifndef GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_MEDIAN_SELECTION_NETWORK_BUILDER_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_MEDIAN_SELECTION_NETWORK_BUILDER_HEADER_INCLUDED

// Compile-time median selection network builder.
//
// Given a full sorting network topology (as a type), this machinery:
// 1. Flattens the network into a linear sequence of compare-and-swap pairs
// 2. Performs backwards dependency analysis from the median position
// 3. Produces a compact array of only the pairs needed to correctly place
//    the median element
//
// The result is a sorting network that does strictly fewer comparisons than
// a full sort, while still guaranteeing that array[N/2] contains the true
// median after execution.

#include <gungnir/median_filter/impl/sorting/sorting_network_executor.hpp>

#include <cstddef>
#include <cstdint>

namespace gungnir::median_filter::impl::sorting {

template <typename T>
struct TotalCompareSwapPairs;

template <std::size_t A, std::size_t B>
struct TotalCompareSwapPairs<CompareSwapPair<A, B>> {
	static constexpr std::size_t value = 1;
};

template <typename... Ops>
struct TotalCompareSwapPairs<CompareSwapGroup<Ops...>> {
	static constexpr std::size_t value = (TotalCompareSwapPairs<Ops>::value + ... + 0);
};

template <typename... Groups>
struct TotalCompareSwapPairs<SortingNetworkTopology<Groups...>> {
	static constexpr std::size_t value = (TotalCompareSwapPairs<Groups>::value + ... + 0);
};

template <std::size_t MaxPairs>
struct FlatCompareSwapPairs {
	uint16_t first[MaxPairs > 0 ? MaxPairs : 1] = {};
	uint16_t second[MaxPairs > 0 ? MaxPairs : 1] = {};
	std::size_t count = 0;

	constexpr void add(uint16_t a, uint16_t b) {
		first[count] = a;
		second[count] = b;
		++count;
	}
};

template <typename Topology, std::size_t MaxPairs>
struct NetworkFlattener;

template <std::size_t A, std::size_t B, std::size_t MaxPairs>
struct NetworkFlattener<CompareSwapPair<A, B>, MaxPairs> {
	static constexpr void flatten(FlatCompareSwapPairs<MaxPairs>& out) {
		out.add(static_cast<uint16_t>(A), static_cast<uint16_t>(B));
	}
};

template <std::size_t MaxPairs, typename... Ops>
struct NetworkFlattener<CompareSwapGroup<Ops...>, MaxPairs> {
	static constexpr void flatten(FlatCompareSwapPairs<MaxPairs>& out) {
		(NetworkFlattener<Ops, MaxPairs>::flatten(out), ...);
	}
};

template <std::size_t MaxPairs, typename... Groups>
struct NetworkFlattener<SortingNetworkTopology<Groups...>, MaxPairs> {
	static constexpr void flatten(FlatCompareSwapPairs<MaxPairs>& out) {
		(NetworkFlattener<Groups, MaxPairs>::flatten(out), ...);
	}
};

template <std::size_t NumElements, typename TopologyType>
constexpr auto build_median_selection_pairs() {
	constexpr std::size_t total = TotalCompareSwapPairs<TopologyType>::value;

	// Step 1: Flatten the full sorting network
	FlatCompareSwapPairs<total> all{};
	NetworkFlattener<TopologyType, total>::flatten(all);

	// Step 2: Backwards dependency analysis from the median position
	constexpr std::size_t median_rank = NumElements / 2;
	bool needed[NumElements > 0 ? NumElements : 1] = {};
	needed[median_rank] = true;

	bool keep[total > 0 ? total : 1] = {};

	for (std::size_t i = all.count; i > 0; --i) {
		const uint16_t a = all.first[i - 1];
		const uint16_t b = all.second[i - 1];
		if (needed[a] || needed[b]) {
			keep[i - 1] = true;
			needed[a] = true;
			needed[b] = true;
		}
	}

	// Step 3: Collect only the kept pairs in execution order
	FlatCompareSwapPairs<total> result{};
	for (std::size_t i = 0; i < all.count; ++i) {
		if (keep[i]) {
			result.add(all.first[i], all.second[i]);
		}
	}

	return result;
}

} // namespace gungnir::median_filter::impl::sorting

#endif
