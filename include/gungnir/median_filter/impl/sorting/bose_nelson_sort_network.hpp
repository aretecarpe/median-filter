#ifndef GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_BOSE_NELSON_SORT_NETWORK_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_BOSE_NELSON_SORT_NETWORK_HEADER_INCLUDED

// Generates a sorting network based on the algorithm proposed
// by Bose and Nelson in "A Sorting Problem" (1962).

#include <gungnir/median_filter/impl/sorting/sorting_network_executor.hpp>

#include <cstddef>
#include <type_traits>

namespace gungnir::median_filter::impl::sorting {

template <std::size_t N>
struct BoseNelsonSortNetwork {
private:
	template <std::size_t I, std::size_t J, std::size_t X, std::size_t Y, typename = void>
	struct GenerateMerge {
		static constexpr std::size_t L = X / 2;
		static constexpr std::size_t M = (X & 1 ? Y : Y + 1) / 2;

		using type = CompareSwapGroup<
			typename GenerateMerge<I, J, L, M>::type,
			typename GenerateMerge<I + L, J + M, X - L, Y - M>::type,
			typename GenerateMerge<I + L, J, X - L, M>::type
		>;
	};

	template <std::size_t I, std::size_t J, std::size_t X, std::size_t Y>
	struct GenerateMerge<I, J, X, Y, std::enable_if_t<(X == 1 && Y == 1)>> {
		using type = CompareSwapGroup<CompareSwapPair<I - 1, J - 1>>;
	};

	template <std::size_t I, std::size_t J, std::size_t X, std::size_t Y>
	struct GenerateMerge<I, J, X, Y, std::enable_if_t<(X == 1 && Y == 2)>> {
		using type = CompareSwapGroup<
			CompareSwapPair<I - 1, J>,
			CompareSwapPair<I - 1, J - 1>
		>;
	};

	template <std::size_t I, std::size_t J, std::size_t X, std::size_t Y>
	struct GenerateMerge<I, J, X, Y, std::enable_if_t<(X == 2 && Y == 1)>> {
		using type = CompareSwapGroup<
			CompareSwapPair<I - 1, J - 1>,
			CompareSwapPair<I, J - 1>
		>;
	};

	template <std::size_t Lo, std::size_t Count, typename = void>
	struct GenerateSort {
		using type = CompareSwapGroup<>;
	};

	template <std::size_t Lo, std::size_t Count>
	struct GenerateSort<Lo, Count, std::enable_if_t<(Count > 1)>> {
		static constexpr std::size_t M = Count / 2;

		template <bool Enable, typename = void>
		struct LeftHalf {
			using type = CompareSwapGroup<>;
		};

		template <bool Enable>
		struct LeftHalf<Enable, std::enable_if_t<Enable>> {
			using type = typename GenerateSort<Lo, M>::type;
		};

		template <bool Enable, typename = void>
		struct RightHalf {
			using type = CompareSwapGroup<>;
		};

		template <bool Enable>
		struct RightHalf<Enable, std::enable_if_t<Enable>> {
			using type = typename GenerateSort<Lo + M, Count - M>::type;
		};

		using type = CompareSwapGroup<
			typename LeftHalf<(M > 1)>::type,
			typename RightHalf<(Count - M > 1)>::type,
			typename GenerateMerge<Lo, Lo + M, M, Count - M>::type
		>;
	};

public:
	using type = SortingNetworkTopology<typename GenerateSort<1, N>::type>;
};

} // namespace gungnir::median_filter::impl::sorting

#endif
