#ifndef GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_NETWORK_EXECUTOR_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_IMPL_SORTING_NETWORK_EXECUTOR_HEADER_INCLUDED

#include <gungnir/median_filter/impl/unified_gpu_runtime.hpp>

#include <cstddef>

namespace gungnir::median_filter::impl::sorting {

template <std::size_t A, std::size_t B>
struct CompareSwapPair {};

template <typename... Ops>
struct CompareSwapGroup {};

template <typename... Groups>
struct SortingNetworkTopology {};

template <typename It, typename CompareOp, std::size_t A, std::size_t B>
__device__ __host__ constexpr auto execute(
	It data,
	CompareOp &&op,
	CompareSwapPair<A, B>
) noexcept -> void {
	op(data[A], data[B]);
}

template <typename It, typename CompareOp, typename... Ops>
__device__ __host__ constexpr auto execute(
	It data,
	CompareOp &&op,
	CompareSwapGroup<Ops...>
) noexcept -> void {
	(execute(data, op, Ops{}), ...);
}

template <typename It, typename CompareOp, typename... Groups>
__device__ __host__ constexpr auto execute(
	It data,
	CompareOp &&op,
	SortingNetworkTopology<Groups...>
) noexcept -> void {
	(execute(data, op, Groups{}), ...);
}

} // namespace gungnir::median_filter::impl::sorting

#endif
