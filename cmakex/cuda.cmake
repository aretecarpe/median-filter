# =============================================================================
# cuda.cmake — CUDA target configuration utilities
# =============================================================================
#
# Provides:
#
#   setup_cuda(<target>
#       [STATIC_RUNTIME | SHARED_RUNTIME]
#       [STANDARD <std>]
#       [ARCHITECTURES <arch>...]
#       [PER_THREAD_STREAMS]
#   )
#       Configures a target for CUDA compilation. Sets the CUDA standard,
#       runtime linkage, GPU architectures, and optional per-thread default
#       stream. When no ARCHITECTURES are given the host GPU is auto-detected
#       via cuda_detect_architectures().
#
#       Options:
#           STATIC_RUNTIME    — Link the static CUDA runtime (cudart_static).
#           SHARED_RUNTIME    — Link the shared CUDA runtime (cudart).
#                               If neither is specified, defaults to shared
#                               with a warning.
#           PER_THREAD_STREAMS — Pass --default-stream=per-thread to nvcc.
#
#       Single-value arguments:
#           STANDARD     — CUDA language standard (default: 20, may be
#                          downgraded to 17 for older Clang/GCC + C++20).
#
#       Multi-value arguments:
#           ARCHITECTURES — Explicit list of SM architectures (e.g. "80" "86").
#
#   set_cuda_runtime(<target> USE_STATIC <bool>)
#       Low-level helper — links the appropriate CUDA runtime to <target>.
#
#   set_cuda_architectures(<out_var>)
#       Populates <out_var> with a semicolon-separated list of GPU
#       architectures (with "-real" suffix). Tries to auto-detect the
#       host GPU; falls back to a hard-coded list filtered by toolkit
#       version.
#
#   cuda_detect_architectures(<possible_archs_var> <out_var>)
#       Compiles and runs a small CUDA program to query the architectures
#       of locally installed GPUs. Falls back to the list passed in
#       <possible_archs_var> if detection fails.
#
# Usage:
#   enable_language(CUDA)
#   add_executable(my_app main.cu)
#   setup_cuda(my_app STATIC_RUNTIME ARCHITECTURES "86" PER_THREAD_STREAMS)
# =============================================================================
include_guard(GLOBAL)

function(setup_cuda TARGET)
	include(CMakeParseArguments)
	set(options STATIC_RUNTIME SHARED_RUNTIME PER_THREAD_STREAMS)
	set(
		oneValueArgs
		RUNTIME
		STANDARD
	)
	set(multiValueArgs ARCHITECTURES)
	cmake_parse_arguments(
		ARG
		"${options}"
		"${oneValueArgs}"
		"${multiValueArgs}"
		${ARGN}
	)

	if(NOT ARG_ARCHITECTURES)
		set(ARCHITECTURES)
		set_cuda_architectures(ARCHITECTURES)
	else()
		set(ARCHITECTURES "${ARG_ARCHITECTURES}")
	endif()

	if(ARG_STATIC_RUNTIME)
		set_cuda_runtime(${TARGET} USE_STATIC TRUE)
	elseif(ARG_SHARED_RUNTIME)
		set_cuda_runtime(${TARGET} USE_STATIC FALSE)
	else()
		message(WARNING "Defaulting cuda runtime to `shared`.")
		set_cuda_runtime(${TARGET} USE_STATIC FALSE)
	endif()

	# Older Clang (<18) and GCC (<13) fail to compile CUDA with C++20.
	# Fall back to C++17 for those combinations.
	if(
		CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
		CMAKE_CXX_COMPILER_VERSION VERSION_LESS "18.0" AND
		"${CMAKE_CXX_STANDARD}" STREQUAL "20"
	)
		set(STANDARD 17)
	elseif(
		CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
		CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0.0" AND
		"${CMAKE_CXX_STANDARD}" STREQUAL "20"
	)
		set(STANDARD 17)
	else()
		if(NOT ARG_STANDARD)
			set(STANDARD 20)
		else()
			set(STANDARD ${ARG_STANDARD})
		endif()
	endif()

	set_target_properties(
		${TARGET} PROPERTIES
		CUDA_STANDARD ${STANDARD}
		CUDA_STANDARD_REQUIRED ON
		CUDA_ARCHITECTURES "${ARCHITECTURES}"
		CUDA_RESOLVE_DEVICE_SYMBOLS ON
		CUDA_SEPARABLE_COMPILATION OFF
	)

	get_target_property(type ${TARGET} TYPE)
	if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
		message(STATUS "Enabling cuda device debugging")
		if(${type} MATCHES "^(INTERFACE)")
			target_compile_options(${TARGET} INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:-g -G>)
		else()
			target_compile_options(${TARGET} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-g -G>)
		endif()
	endif()

	if(ARG_PER_THREAD_STREAMS)
		if(${type} MATCHES "^(INTERFACE)")
			target_compile_options(
				${TARGET} INTERFACE
				$<$<COMPILE_LANGUAGE:CUDA>:--default-stream=per-thread>
			)
		else()
			target_compile_options(
				${TARGET} PRIVATE
				$<$<COMPILE_LANGUAGE:CUDA>:--default-stream=per-thread>
			)
		endif()
	endif()
endfunction()

function(set_cuda_runtime target)
	cmake_parse_arguments(ARG "" "USE_STATIC" "" ${ARGN})

	get_target_property(type ${target} TYPE)
	if(type STREQUAL "INTERFACE_LIBRARY")
		set(mode INTERFACE)
	else()
		set(mode PRIVATE)
	endif()

	if(${ARG_USE_STATIC})
		set_target_properties(${target} PROPERTIES CUDA_RUNTIME_LIBRARY Static)
		find_package(CUDAToolkit REQUIRED COMPONENTS cudart_static cuda_driver)
		target_link_libraries(
			${target} ${mode}
			$<TARGET_NAME_IF_EXISTS:CUDA::cudart_static> $<TARGET_NAME_IF_EXISTS:CUDA::cuda_driver>
		)
	else()
		set_target_properties(${target} PROPERTIES CUDA_RUNTIME_LIBRARY Shared)
		find_package(CUDAToolkit REQUIRED COMPONENTS cudart cuda_driver)
		target_link_libraries(
			${target} ${mode}
			$<TARGET_NAME_IF_EXISTS:CUDA::cudart> $<TARGET_NAME_IF_EXISTS:CUDA::cuda_driver>
		)
	endif()
endfunction()

function(set_cuda_architectures result)
	set(supported_archs "70" "75" "80" "86" "87" "90" "100" "120")
	if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA" AND CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 11.1.0)
		list(REMOVE_ITEM supported_archs "86")
	endif()
	if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA" AND CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 11.8.0)
		list(REMOVE_ITEM supported_archs "90")
	endif()
	if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA" AND CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 12.8.0)
		list(REMOVE_ITEM supported_archs "100")
		list(REMOVE_ITEM supported_archs "120")
	endif()

	cuda_detect_architectures(supported_archs CMAKE_CUDA_ARCHITECTURES)
	list(TRANSFORM CMAKE_CUDA_ARCHITECTURES APPEND "-real")

	get_property(cached_value GLOBAL PROPERTY cuda_architectures)
	if(NOT cached_value)
		set_property(GLOBAL PROPERTY cuda_architectures "${CMAKE_CUDA_ARCHITECTURES}")
	endif()
	if(NOT cached_value STREQUAL CMAKE_CUDA_ARCHITECTURES)
		string(REPLACE ";" "\n " _cuda_architectures_pretty "${CMAKE_CUDA_ARCHITECTURES}")
		message(STATUS "Project ${PROJECT_NAME} is building for CUDA architectures:\n ${_cuda_architectures_pretty}")
	endif()

	set(${result} "${CMAKE_CUDA_ARCHITECTURES}" PARENT_SCOPE)
	set(CMAKE_CUDA_FLAGS ${CMAKE_CUDA_FLAGS} PARENT_SCOPE)
endfunction()

function(cuda_detect_architectures possible_archs_var gpu_archs)
	set(CMAKE_CUDA_ARCHITECTURES OFF)
	set(__gpu_archs ${${possible_archs_var}})

	set(eval_file ${PROJECT_BINARY_DIR}/eval_gpu_archs.cu)
	set(eval_exe ${PROJECT_BINARY_DIR}/eval_gpu_archs)
	set(error_file ${PROJECT_BINARY_DIR}/eval_gpu_archs.stderr.log)

	if(NOT DEFINED CMAKE_CUDA_COMPILER)
		message(FATAL_ERROR "No CUDA compiler specified, unable to determine machine's GPUs.")
	endif()

	if(NOT EXISTS "${eval_exe}")
		file(WRITE ${eval_file}
			"
#include <cstdio>
#include <set>
#include <string>
using namespace std;
int main(int argc, char **argv) {
	set<string> archs;
	int devices;
	if((cudaGetDeviceCount(&devices) == cudaSuccess) && (devices >0)) {
		for (int dev=0;dev<devices;++dev) {
			char buff[32];
			cudaDeviceProp prop;
			if(cudaGetDeviceProperties(&prop, dev) != cudaSuccess) continue;
			sprintf(buff, \"%d%d\", prop.major, prop.minor);
			archs.insert(buff);
		}
	}
	if(archs.empty()) {
		printf(\"${__gpu_archs}\");
	} else {
		bool first = true;
		for (const auto &arch : archs) {
			printf(first? \"%s\" : \";%s\", arch.c_str());
			first = false;
		}
	}
	printf(\"\\n\");
	return 0;
}
		")
		execute_process(
			COMMAND ${CMAKE_CUDA_COMPILER} -std=c++11 -o "${eval_exe}" "${eval_file}"
			ERROR_FILE "${error_file}"
		)
	endif()

	if(EXISTS "${eval_exe}")
		execute_process(
			COMMAND "${eval_exe}" OUTPUT_VARIABLE __gpu_archs
			OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_FILE "${error_file}"
		)
		message(STATUS "Using auto detection of gpu-archs: ${__gpu_archs}")
	else()
		message(STATUS "Failed auto detection of gpu-archs: ${__gpu_archs}")
	endif()
	set(${gpu_archs} ${__gpu_archs} PARENT_SCOPE)
endfunction()
