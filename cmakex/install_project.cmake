# =============================================================================
# install_project.cmake — Umbrella config-file package for component projects
# =============================================================================
#
# Provides:
#   install_project([NAMESPACE <ns>] [COMPATIBILITY <compat>])
#
# Call once from the top-level CMakeLists.txt *after* all add_library_component()
# calls. Generates and installs an umbrella config-file package that supports
# find_package(${PROJECT_NAME} COMPONENTS ...).
#
# This function reads the component registry populated by add_library_component()
# (stored in global properties) and produces:
#
#   <libdir>/cmake/${PROJECT_NAME}/
#     ${PROJECT_NAME}-config.cmake          — umbrella config with COMPONENTS
#     ${PROJECT_NAME}-config-version.cmake  — version compatibility check
#     ${PROJECT_NAME}-<comp>-targets.cmake  — per-component export sets
#                                             (installed by add_library_component)
#
# Single-value arguments:
#   NAMESPACE     — Namespace prefix for exported targets (default: "${PROJECT_NAME}::").
#   COMPATIBILITY — Version compatibility mode passed to
#                   write_basic_package_version_file(). One of:
#                     AnyNewerVersion | SameMajorVersion |
#                     SameMinorVersion | ExactVersion
#                   Default: SameMajorVersion.
#
# Usage:
#   # At the bottom of the top-level CMakeLists.txt, after all components:
#   install_project(
#       NAMESPACE "gungnir.exemplar::"
#       COMPATIBILITY SameMajorVersion
#   )
#
# Consumer:
#   find_package(gungnir.exemplar REQUIRED COMPONENTS core math)
#   target_link_libraries(app PRIVATE gungnir.exemplar::core)
# =============================================================================
include_guard(GLOBAL)

function(install_project)
	cmake_parse_arguments(ARG "" "NAMESPACE;COMPATIBILITY" "" ${ARGN})

	if(NOT ARG_NAMESPACE)
		set(ARG_NAMESPACE "${PROJECT_NAME}::")
	endif()

	if(NOT ARG_COMPATIBILITY)
		set(ARG_COMPATIBILITY SameMajorVersion)
	endif()

	include(GNUInstallDirs)
	include(CMakePackageConfigHelpers)

	# -- Read the component registry ------------------------------------------
	get_property(_component_names GLOBAL PROPERTY _GUNGNIR_COMPONENT_NAMES)
	if(NOT _component_names)
		message(WARNING
			"install_project(): No components registered. "
			"Call add_library_component() before install_project()."
		)
		return()
	endif()

	# Build the find_dependency block for external package dependencies.
	get_property(_find_deps GLOBAL PROPERTY _GUNGNIR_FIND_DEPENDENCIES)
	set(_find_dep_lines "")
	if(_find_deps)
		list(REMOVE_DUPLICATES _find_deps)
		foreach(_pkg IN LISTS _find_deps)
			string(APPEND _find_dep_lines "find_dependency(${_pkg})\n")
		endforeach()
	endif()

	# Build the component dependency block for the config template.
	# Each component's dependencies become a set() call in the generated config.
	set(_dependency_lines "")
	foreach(_comp IN LISTS _component_names)
		get_property(_deps GLOBAL PROPERTY _GUNGNIR_COMPONENT_${_comp}_DEPS)
		if(_deps)
			string(REPLACE ";" " " _deps_spaced "${_deps}")
			string(APPEND _dependency_lines
				"set(_${PROJECT_NAME}_${_comp}_dependencies ${_deps_spaced})\n"
			)
		endif()
	endforeach()

	# -- Variables consumed by ProjectConfig.cmake.in -------------------------
	# Convert list to space-separated for the template.
	string(REPLACE ";" " " _GUNGNIR_COMPONENT_NAMES "${_component_names}")
	set(_GUNGNIR_FIND_DEPENDENCY_BLOCK "${_find_dep_lines}")
	set(_GUNGNIR_DEPENDENCY_BLOCK "${_dependency_lines}")
	set(_GUNGNIR_NAMESPACE "${ARG_NAMESPACE}")

	# -- Locate the template --------------------------------------------------
	find_file(
		_PROJECT_CONFIG_TEMPLATE
		NAMES "ProjectConfig.cmake.in"
		PATHS "${PROJECT_SOURCE_DIR}/cmake/template/"
		NO_DEFAULT_PATH
		NO_CACHE
		REQUIRED
	)

	# -- Generate config and version files ------------------------------------
	set(_PACKAGE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

	set(_config_file "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake")
	configure_package_config_file(
		"${_PROJECT_CONFIG_TEMPLATE}"
		"${_config_file}"
		INSTALL_DESTINATION "${_PACKAGE_INSTALL_DIR}"
		PATH_VARS PROJECT_NAME PROJECT_VERSION
	)

	set(_version_file "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake")
	write_basic_package_version_file(
		"${_version_file}"
		VERSION "${PROJECT_VERSION}"
		COMPATIBILITY ${ARG_COMPATIBILITY}
	)

	# -- Install --------------------------------------------------------------
	install(
		FILES "${_config_file}" "${_version_file}"
		DESTINATION "${_PACKAGE_INSTALL_DIR}"
	)
endfunction()
