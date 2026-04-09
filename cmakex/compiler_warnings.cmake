# =============================================================================
# compiler_warnings.cmake — Per-compiler warning presets
# =============================================================================
#
# Provides:
#   set_project_warnings(<target> [WARNINGS_AS_ERRORS])
#
# Applies a curated set of compiler warnings to <target>. The correct warning
# set is chosen automatically based on the active compiler:
#
#   MSVC  — /W4 plus additional /w1xxxx diagnostics, /permissive-
#   Clang — -Wall -Wextra and many specific -W flags
#   GCC   — All Clang warnings plus GCC-specific extras
#   CUDA  — A reduced set suitable for device code
#
# Options:
#   WARNINGS_AS_ERRORS — Promote warnings to errors (/WX on MSVC, -Werror
#                        on GCC/Clang).
#
# Interface libraries receive warnings via INTERFACE; all other target types
# receive them via PRIVATE so they do not leak to consumers.
#
# Usage:
#   add_library(mylib src.cpp)
#   set_project_warnings(mylib WARNINGS_AS_ERRORS)
# =============================================================================
include_guard(GLOBAL)

function(set_project_warnings project_name)
	set(options WARNINGS_AS_ERRORS)
	cmake_parse_arguments(
		arg
		"${options}"
		""
		""
		${ARGN}
	)

	set(MSVC_WARNINGS
		/W4
		/w14242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
		/w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits'
		/w14263 # 'function': member function does not override any base class virtual member function
		/w14265 # 'classname': class has virtual functions, but destructor is not virtual
		/w14287 # 'operator': unsigned/negative constant mismatch
		/we4289 # 'variable': loop control variable is used outside the for-loop scope
		/w14296 # 'operator': expression is always 'boolean_value'
		/w14311 # 'variable': pointer truncation from 'type1' to 'type2'
		/w14545 # expression before comma evaluates to a function which is missing an argument list
		/w14546 # function call before comma missing argument list
		/w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
		/w14549 # 'operator': operator before comma has no effect
		/w14555 # expression has no effect; expected expression with side-effect
		/w14619 # pragma warning: there is no warning number 'number'
		/w14640 # thread un-safe static member initialization
		/w14826 # conversion from 'type1' to 'type2' is sign-extended
		/w14905 # wide string literal cast to 'LPSTR'
		/w14906 # string literal cast to 'LPWSTR'
		/w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
		/permissive-
	)

	set(CLANG_WARNINGS
		-Wall
		-Wextra
		-Wshadow
		-Wnon-virtual-dtor
		-Wold-style-cast
		-Wcast-align
		-Wunused
		-Woverloaded-virtual
		-Wpedantic
		-Wconversion
		-Wsign-conversion
		-Wnull-dereference
		-Wdouble-promotion
		-Wformat=2
		-Wimplicit-fallthrough
	)

	set(GCC_WARNINGS
		${CLANG_WARNINGS}
		-Wmisleading-indentation
		-Wduplicated-cond
		-Wduplicated-branches
		-Wlogical-op
		-Wuseless-cast
		-Wsuggest-override
	)

	set(CUDA_WARNINGS
		-Wall
		-Wextra
		-Wunused
		-Wconversion
		-Wshadow
	)

	if(MSVC)
		set(PROJECT_WARNINGS ${MSVC_WARNINGS})
	elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
		set(PROJECT_WARNINGS ${CLANG_WARNINGS})
	elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
		set(PROJECT_WARNINGS ${GCC_WARNINGS})
	else()
		message(FATAL_ERROR "Compiler not supported.")
	endif()

	if(CMAKE_CUDA_COMPILER)
		set(PROJECT_WARNINGS_CUDA "${CUDA_WARNINGS}")
	else()
		set(PROJECT_WARNINGS_CUDA "")
	endif()

	if(arg_WARNINGS_AS_ERRORS)
		if(MSVC)
			list(APPEND PROJECT_WARNINGS /WX)
		else()
			list(APPEND PROJECT_WARNINGS -Werror)
		endif()
	endif()

	get_target_property(TARGET_TYPE ${project_name} TYPE)
	if(${TARGET_TYPE} STREQUAL "INTERFACE_LIBRARY")
		target_compile_options(
			${project_name}
			INTERFACE
				$<$<COMPILE_LANGUAGE:CXX>:${PROJECT_WARNINGS}>
				$<$<COMPILE_LANGUAGE:CUDA>:${PROJECT_WARNINGS_CUDA}>
		)
	else()
		target_compile_options(
			${project_name}
			PRIVATE
				$<$<COMPILE_LANGUAGE:CXX>:${PROJECT_WARNINGS}>
				$<$<COMPILE_LANGUAGE:CUDA>:${PROJECT_WARNINGS_CUDA}>
		)
	endif()
endfunction()
