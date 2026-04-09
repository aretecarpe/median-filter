# =============================================================================
# hip.cmake — HIP target configuration utilities
# =============================================================================
#
# Provides:
#
#   setup_hip(<target>
#       [STANDARD <std>]
#       [ARCHITECTURES <arch>...]
#   )
#       Configures a target for HIP compilation. Sets the HIP standard,
#       GPU architectures, and links the HIP runtime.
#
#       Single-value arguments:
#           STANDARD     — HIP/C++ language standard (default: 20).
#
#       Multi-value arguments:
#           ARCHITECTURES — Explicit list of GFX architectures
#                           (e.g. "gfx906" "gfx1030").
#
# Usage:
#   enable_language(HIP)
#   add_executable(my_app main.cu)
#   set_source_files_properties(main.cu PROPERTIES LANGUAGE HIP)
#   setup_hip(my_app ARCHITECTURES "gfx1030")
# =============================================================================
include_guard(GLOBAL)

function(setup_hip TARGET)
	include(CMakeParseArguments)
	set(options "")
	set(oneValueArgs STANDARD)
	set(multiValueArgs ARCHITECTURES)
	cmake_parse_arguments(
		ARG
		"${options}"
		"${oneValueArgs}"
		"${multiValueArgs}"
		${ARGN}
	)

	if(NOT ARG_STANDARD)
		set(STANDARD 20)
	else()
		set(STANDARD ${ARG_STANDARD})
	endif()

	if(NOT ARG_ARCHITECTURES)
		set(ARCHITECTURES "gfx942")
		message(STATUS "No HIP architectures specified, defaulting to: ${ARCHITECTURES}")
	else()
		set(ARCHITECTURES "${ARG_ARCHITECTURES}")
	endif()

	set_target_properties(
		${TARGET} PROPERTIES
		HIP_STANDARD ${STANDARD}
		HIP_STANDARD_REQUIRED ON
		HIP_ARCHITECTURES "${ARCHITECTURES}"
	)

	get_target_property(type ${TARGET} TYPE)
	if(${type} MATCHES "^(INTERFACE)")
		set(mode INTERFACE)
	else()
		set(mode PRIVATE)
	endif()

	target_compile_definitions(${TARGET} ${mode} GUNGNIR_USE_HIP)

	# Find and link HIP runtime
	find_package(hip REQUIRED)
	target_link_libraries(${TARGET} ${mode} hip::device)

	if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
		message(STATUS "Enabling HIP device debugging")
		target_compile_options(${TARGET} ${mode} $<$<COMPILE_LANGUAGE:HIP>:-g -O0>)
	endif()

	string(REPLACE ";" "\n " _hip_architectures_pretty "${ARCHITECTURES}")
	message(STATUS "Project ${PROJECT_NAME} is building for HIP architectures:\n ${_hip_architectures_pretty}")
endfunction()
