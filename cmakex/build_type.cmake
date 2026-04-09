# =============================================================================
# build_type.cmake — Default build type for single-configuration generators
# =============================================================================
#
# Provides:
#   default_build_type(<type>)
#
# Sets CMAKE_BUILD_TYPE to <type> when the user has not explicitly chosen one
# and the generator is single-configuration (i.e. Makefiles, Ninja).
# Multi-configuration generators (Visual Studio, Xcode, Ninja Multi-Config)
# are left untouched because they use CMAKE_CONFIGURATION_TYPES instead.
#
# Parameters:
#   DEFAULT_BUILD_TYPE — The fallback build type (e.g. "Release", "Debug").
#
# Usage:
#   default_build_type("Release")
# =============================================================================
include_guard(GLOBAL)

function(default_build_type DEFAULT_BUILD_TYPE)
	if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
		message(VERBOSE "Setting CMAKE_BUILD_TYPE to `${DEFAULT_BUILD_TYPE}`.")
		set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Build type." FORCE)
		set_property(
			CACHE CMAKE_BUILD_TYPE PROPERTY
			STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
		)
	endif()
endfunction()
