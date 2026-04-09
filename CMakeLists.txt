cmake_minimum_required(VERSION 3.25)

project(
	gungnir.median_filter
	DESCRIPTION "Blazingly fast implementation of median filter in CUDA/HIP C++"
	LANGUAGES CXX
	VERSION 0.1.0
)

include(cmakex/cmake.cmake)

default_build_type(Debug)

set(
	GPU_BACKEND "CUDA" CACHE STRING
	"GPU backend to use. Values: { CUDA, HIP }."
)
set_property(CACHE GPU_BACKEND PROPERTY STRINGS "CUDA" "HIP")

option(
	${PROJECT_NAME}_BUILD_TESTS
	"Enable building tests and test infrastructure. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
	${PROJECT_IS_TOP_LEVEL}
)

option(
	${PROJECT_NAME}_BUILD_EXAMPLES
	"Enable building examples. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
	${PROJECT_IS_TOP_LEVEL}
)

add_library(${PROJECT_NAME} INTERFACE)
add_library(gungnir::median_filter ALIAS ${PROJECT_NAME})

if(GPU_BACKEND STREQUAL "HIP")
	enable_language(HIP)
	setup_hip(
		${PROJECT_NAME}
		STANDARD ${CMAKE_CXX_STANDARD}
		ARCHITECTURES "gfx942"
	)
else()
	setup_cuda(
		${PROJECT_NAME}
		STATIC_RUNTIME
		STANDARD ${CMAKE_CXX_STANDARD}
		ARCHITECTURES "87"
		PER_THREAD_STREAMS
	)
endif()

target_sources(
	${PROJECT_NAME}
	PUBLIC
		FILE_SET HEADERS
		BASE_DIRS
			${PROJECT_SOURCE_DIR}/include
		FILES
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/median_filter_2d.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/sorting_network_sorter.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/median_selection_network_sorter.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/impl/unified_gpu_runtime.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/impl/ceiling_division.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/impl/bounds_check.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/impl/pitched_image.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/impl/kernel/median_filter_2d_kernel.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/impl/sorting/compare_and_swap.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/impl/sorting/sorting_network_executor.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/impl/sorting/compile_time_grid_loop.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/impl/sorting/bitonic_merge_sort_network.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/impl/sorting/bose_nelson_sort_network.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/impl/sorting/batcher_odd_even_merge_sort_network.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/impl/sorting/insertion_sort_network.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/impl/sorting/bubble_sort_network.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/impl/sorting/size_optimized_sort_network.hpp
			${PROJECT_SOURCE_DIR}/include/gungnir/median_filter/impl/sorting/median_selection_network_builder.hpp
)

if(${PROJECT_NAME}_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()
