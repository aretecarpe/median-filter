# =============================================================================
# add_component.cmake — Create and register component library targets
# =============================================================================
#
# Provides:
#   add_library_component(<target_name>
#       [STATIC | SHARED | INTERFACE]
#       [STANDARD <std>]
#       [WARNINGS_AS_ERRORS]
#       [NAMESPACE <ns>]
#       [SOURCES <file>...]
#       [HEADERS <file>...]
#       [HEADER_BASE_DIRS <dir>...]
#       [DEPENDENCIES <target>...]
#       [PUBLIC_DEPENDENCIES <target>...]
#       [COMPONENT_DEPENDENCIES <component>...]
#       [FIND_DEPENDENCIES <pkg>...]
#   )
#
# Creates a library target and registers it as a named component in the
# project's component registry. The registry is consumed by install_project()
# to generate an umbrella config-file package that supports:
#
#   find_package(<project> REQUIRED COMPONENTS core math)
#   target_link_libraries(app PRIVATE <project>::core <project>::math)
#
# Component naming:
#   The component name is derived from the target name by stripping the
#   PROJECT_NAME prefix. For example, target "gungnir.exemplar.core" under
#   project "gungnir.exemplar" becomes component "core".
#
# Options:
#   STATIC             — Build as a static library (default).
#   SHARED             — Build as a shared library.
#   INTERFACE          — Header-only library (no compiled sources).
#   WARNINGS_AS_ERRORS — Promote compiler warnings to errors.
#
# Single-value arguments:
#   STANDARD  — C++ standard to require (default: 20).
#   NAMESPACE — Namespace prefix for the export set
#               (default: "${PROJECT_NAME}::").
#
# Multi-value arguments:
#   SOURCES               — Source files (.cpp, .cu, etc.).
#   HEADERS               — Public header files to install.
#   HEADER_BASE_DIRS      — Base directories for FILE_SET HEADERS
#                           (default: "include").
#   DEPENDENCIES          — Targets to link PRIVATE.
#   PUBLIC_DEPENDENCIES   — Targets to link PUBLIC / INTERFACE (propagated).
#   COMPONENT_DEPENDENCIES — Other component *names* (not target names) that
#                            this component depends on. These are recorded in
#                            the registry so that find_package() auto-resolves
#                            them when a consumer requests this component.
#                            The corresponding targets are also linked PUBLIC.
#   FIND_DEPENDENCIES     — External packages that consumers must find before
#                            using this component (e.g. "CUDAToolkit", "Boost").
#                            Emitted as find_dependency() calls in the umbrella
#                            config so that consumers don't need to manually
#                            find transitive dependencies.
#
# Example:
#   add_library_component(gungnir.exemplar.core
#       STATIC
#       STANDARD 20
#       WARNINGS_AS_ERRORS
#       SOURCES src/device_info.cpp
#       HEADERS include/gungnir/exemplar/core/device_info.hpp
#   )
#
#   add_library_component(gungnir.exemplar.math
#       STATIC
#       WARNINGS_AS_ERRORS
#       SOURCES src/transform.cpp
#       HEADERS include/gungnir/exemplar/math/transform.hpp
#       COMPONENT_DEPENDENCIES core
#   )
#
# After install_project() is called, consumers can do:
#   find_package(gungnir.exemplar REQUIRED COMPONENTS math)
#   # "core" is automatically pulled in as a dependency of "math".
#   target_link_libraries(app PRIVATE gungnir.exemplar::math)
# =============================================================================
include_guard(GLOBAL)

function(add_library_component NAME)
	set(options STATIC SHARED INTERFACE WARNINGS_AS_ERRORS)
	set(oneValueArgs STANDARD NAMESPACE)
	set(multiValueArgs
		SOURCES HEADERS HEADER_BASE_DIRS
		DEPENDENCIES PUBLIC_DEPENDENCIES COMPONENT_DEPENDENCIES
		FIND_DEPENDENCIES
	)
	cmake_parse_arguments(
		ARG
		"${options}"
		"${oneValueArgs}"
		"${multiValueArgs}"
		${ARGN}
	)

	# -- Derive component name ------------------------------------------------
	string(REPLACE "${PROJECT_NAME}." "" COMPONENT_NAME "${NAME}")

	if(NOT ARG_NAMESPACE)
		set(ARG_NAMESPACE "${PROJECT_NAME}::")
	endif()

	# -- Register in global component list ------------------------------------
	set_property(GLOBAL APPEND PROPERTY _GUNGNIR_COMPONENT_NAMES "${COMPONENT_NAME}")
	set_property(GLOBAL PROPERTY _GUNGNIR_COMPONENT_${COMPONENT_NAME}_TARGET "${NAME}")

	if(ARG_COMPONENT_DEPENDENCIES)
		set_property(GLOBAL PROPERTY
			_GUNGNIR_COMPONENT_${COMPONENT_NAME}_DEPS
			"${ARG_COMPONENT_DEPENDENCIES}"
		)
	endif()

	if(ARG_FIND_DEPENDENCIES)
		set_property(GLOBAL APPEND PROPERTY
			_GUNGNIR_FIND_DEPENDENCIES
			${ARG_FIND_DEPENDENCIES}
		)
	endif()

	# -- Determine library type -----------------------------------------------
	if(ARG_INTERFACE)
		set(LIB_TYPE INTERFACE)
	elseif(ARG_SHARED)
		set(LIB_TYPE SHARED)
	else()
		set(LIB_TYPE STATIC)
	endif()

	add_library(${NAME} ${LIB_TYPE})

	# -- C++ standard ---------------------------------------------------------
	if(NOT ARG_STANDARD)
		set(ARG_STANDARD 20)
	endif()

	if(NOT LIB_TYPE STREQUAL "INTERFACE")
		set(VISIBILITY PRIVATE)
		set(PUBLIC_VISIBILITY PUBLIC)
		target_compile_features(${NAME} PUBLIC cxx_std_${ARG_STANDARD})
	else()
		set(VISIBILITY INTERFACE)
		set(PUBLIC_VISIBILITY INTERFACE)
		target_compile_features(${NAME} INTERFACE cxx_std_${ARG_STANDARD})
	endif()

	# -- Sources --------------------------------------------------------------
	if(ARG_SOURCES)
		if(LIB_TYPE STREQUAL "INTERFACE")
			message(FATAL_ERROR
				"add_library_component(${NAME}): INTERFACE libraries cannot have SOURCES."
			)
		endif()
		target_sources(${NAME} PRIVATE ${ARG_SOURCES})
	endif()

	# -- Public headers (FILE_SET) --------------------------------------------
	if(ARG_HEADERS)
		if(NOT ARG_HEADER_BASE_DIRS)
			set(ARG_HEADER_BASE_DIRS "include")
		endif()
		target_sources(
			${NAME}
			${PUBLIC_VISIBILITY}
			FILE_SET HEADERS
			BASE_DIRS ${ARG_HEADER_BASE_DIRS}
			FILES ${ARG_HEADERS}
		)
	endif()

	# -- Dependencies ---------------------------------------------------------
	if(ARG_DEPENDENCIES)
		target_link_libraries(${NAME} ${VISIBILITY} ${ARG_DEPENDENCIES})
	endif()

	if(ARG_PUBLIC_DEPENDENCIES)
		target_link_libraries(${NAME} ${PUBLIC_VISIBILITY} ${ARG_PUBLIC_DEPENDENCIES})
	endif()

	# Link component dependencies (by full target name) as PUBLIC.
	if(ARG_COMPONENT_DEPENDENCIES)
		foreach(_dep IN LISTS ARG_COMPONENT_DEPENDENCIES)
			set(_dep_target "${PROJECT_NAME}.${_dep}")
			if(TARGET "${_dep_target}")
				target_link_libraries(${NAME} ${PUBLIC_VISIBILITY} ${_dep_target})
			else()
				message(FATAL_ERROR
					"add_library_component(${NAME}): Component dependency "
					"'${_dep}' (target '${_dep_target}') does not exist. "
					"Ensure it is added before this component."
				)
			endif()
		endforeach()
	endif()

	# -- Warnings -------------------------------------------------------------
	if(ARG_WARNINGS_AS_ERRORS)
		set_project_warnings(${NAME} WARNINGS_AS_ERRORS)
	else()
		set_project_warnings(${NAME})
	endif()

	# -- Install (component-aware) --------------------------------------------
	include(GNUInstallDirs)

	set(_PACKAGE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

	# Set the export name to the short component name so that the namespaced
	# target becomes <namespace><component> (e.g. gungnir.exemplar::core).
	set_target_properties(${NAME} PROPERTIES EXPORT_NAME "${COMPONENT_NAME}")

	# Install the target itself (headers, archive, shared lib, etc.)
	install(
		TARGETS ${NAME}
		COMPONENT ${NAME}
		EXPORT ${NAME}-export
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		FILE_SET HEADERS
	)

	# Install the export set into the shared umbrella cmake directory.
	install(
		EXPORT ${NAME}-export
		DESTINATION "${_PACKAGE_INSTALL_DIR}"
		NAMESPACE "${ARG_NAMESPACE}"
		FILE "${PROJECT_NAME}-${COMPONENT_NAME}-targets.cmake"
		COMPONENT ${NAME}
	)
endfunction()
