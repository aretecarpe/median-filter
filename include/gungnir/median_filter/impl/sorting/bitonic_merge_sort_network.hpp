#ifndef GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_BITONIC_MERGE_SORT_NETWORK_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_BITONIC_MERGE_SORT_NETWORK_HEADER_INCLUDED

#include <gungnir/median_filter/impl/sorting/sorting_network_executor.hpp>

#include <cstddef>
#include <type_traits>

namespace gungnir::median_filter::impl::sorting {

namespace detail {

constexpr auto next_power_of_two(std::size_t n) noexcept -> std::size_t {
	std::size_t r = 1;
	while (r < n) { r <<= 1; }
	return r;
}

} // namespace detail

template <std::size_t Lo, std::size_t Hi, bool Descending>
struct BitonicMergeSubnetwork;

template <std::size_t Lo, std::size_t Hi, bool Descending>
struct BitonicMergeSortNetwork;

template <std::size_t Lo, std::size_t Hi, bool Descending, typename = void>
struct BitonicMergeSubnetworkImpl {
	using type = SortingNetworkTopology<>;
};

template <std::size_t Lo, std::size_t Hi, bool Descending>
struct BitonicMergeSubnetworkImpl<
	Lo,
	Hi,
	Descending,
	std::enable_if_t<(Hi - Lo >= 1)>
> {
	static constexpr std::size_t count = Hi - Lo + 1;
	static constexpr std::size_t stride = detail::next_power_of_two(count) / 2;

	template <typename Seq>
	struct MakeCompareSwapGroup;
	template <std::size_t... I>
	struct MakeCompareSwapGroup<std::index_sequence<I...>> {
		using type = CompareSwapGroup<std::conditional_t<
			Descending,
			CompareSwapPair<Lo + I + stride, Lo + I>,
			CompareSwapPair<Lo + I, Lo + I + stride>>...
		>;
	};

	using type = SortingNetworkTopology<
		typename MakeCompareSwapGroup<std::make_index_sequence<count - stride>>::type,
		typename BitonicMergeSubnetwork<Lo, Lo + stride - 1, Descending>::type,
		typename BitonicMergeSubnetwork<Lo + stride, Hi, Descending>::type
	>;
};

template <std::size_t Lo, std::size_t Hi, bool Descending>
struct BitonicMergeSubnetwork {
	using type = typename BitonicMergeSubnetworkImpl<Lo, Hi, Descending>::type;
};

template <std::size_t Lo, std::size_t Hi, bool Descending, typename = void>
struct BitonicMergeSortNetworkImpl {
	using type = SortingNetworkTopology<>;
};

template <std::size_t Lo, std::size_t Hi, bool Descending>
struct BitonicMergeSortNetworkImpl<
	Lo,
	Hi,
	Descending,
	std::enable_if_t<(Hi - Lo >= 1)>
> {
	static constexpr std::size_t mid = Lo + (Hi - Lo + 1) / 2;

	using type = SortingNetworkTopology<
		typename BitonicMergeSortNetwork<Lo, mid - 1, !Descending>::type,
		typename BitonicMergeSortNetwork<mid, Hi, Descending>::type,
		typename BitonicMergeSubnetwork<Lo, Hi, Descending>::type
	>;
};

template <std::size_t Lo, std::size_t Hi, bool Descending>
struct BitonicMergeSortNetwork {
	using type = typename BitonicMergeSortNetworkImpl<Lo, Hi, Descending>::type;
};

} // namespace gungnir::median_filter::impl::sorting

#endif
