# =============================================================================
# install_target.cmake — Install rules for executables and standalone targets
# =============================================================================
#
# Provides:
#   install_target(<target_name>)
#
# Generates install rules for a single target. Behaviour depends on the
# target type:
#
#   EXECUTABLE
#       Installed to CMAKE_INSTALL_BINDIR (typically bin/).
#
# For component libraries, prefer add_library_component() which handles both
# creation and installation with full component registry integration.
# install_target() can still install standalone (non-component) libraries —
# they get their own independent config-file package.
#
# Usage:
#   add_executable(gungnir.exemplar main.cpp)
#   install_target(gungnir.exemplar)   # → bin/gungnir.exemplar
# =============================================================================
include_guard(GLOBAL)

function(install_target NAME)
	if(NOT TARGET "${NAME}")
		message(FATAL_ERROR "TARGET `${NAME}` does not exist.")
	endif()

	if(NOT ARGN STREQUAL "")
		message(
			FATAL_ERROR
			"install_target(<NAME>) does not accept extra arguments: ${ARGN} !"
		)
	endif()

	string(REPLACE "." ";" NAME_PARTS "${NAME}")
	set(TARGET_NAME "${NAME}")
	set(INSTALL_COMPONENT_NAME "${NAME}")
	set(EXPORT_NAME "${NAME}")
	set(PACKAGE_NAME "${NAME}")
	list(GET NAME_PARTS -1 COMPONENT_NAME)

	get_target_property(TARGET_TYPE "${TARGET_NAME}" TYPE)

	include(GNUInstallDirs)

	# --- Executables: simple bin/ install ------------------------------------
	if(TARGET_TYPE STREQUAL "EXECUTABLE")
		install(
			TARGETS "${TARGET_NAME}"
			COMPONENT "${INSTALL_COMPONENT_NAME}"
			RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		)
		return()
	endif()

	# --- Standalone libraries: full install with own config package -----------
	install(
		TARGETS "${TARGET_NAME}"
		COMPONENT "${INSTALL_COMPONENT_NAME}"
		EXPORT "${EXPORT_NAME}"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		FILE_SET HEADERS
	)

	set_target_properties(
		"${TARGET_NAME}"
		PROPERTIES EXPORT_NAME "${COMPONENT_NAME}"
	)

	string(TOUPPER "${NAME}" PROJECT_PREFIX)
	string(REPLACE "." "_" PROJECT_PREFIX "${PROJECT_PREFIX}")

	option(
		${PROJECT_PREFIX}_INSTALL_CONFIG_FILE_PACKAGE
		"Install CMake config-file package for find_package() support. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
		${PROJECT_IS_TOP_LEVEL}
	)

	set(INSTALL_CONFIG_PACKAGE ON)

	if(DEFINED GUNGNIR_INSTALL_CONFIG_FILE_PACKAGES)
		if(NOT "${INSTALL_COMPONENT_NAME}" IN_LIST GUNGNIR_INSTALL_CONFIG_FILE_PACKAGES)
			set(INSTALL_CONFIG_PACKAGE OFF)
		endif()
	endif()

	if(DEFINED ${PROJECT_PREFIX}_INSTALL_CONFIG_FILE_PACKAGE)
		set(INSTALL_CONFIG_PACKAGE ${${PROJECT_PREFIX}_INSTALL_CONFIG_FILE_PACKAGE})
	endif()

	if(INSTALL_CONFIG_PACKAGE)
		message(
			DEBUG
			"install_target: Installing a config package for `${NAME}`."
		)

		include(CMakePackageConfigHelpers)

		find_file(
			CONFIG_FILE_TEMPLATE
			NAMES "Config.cmake.in"
			PATHS
				"${CMAKE_CURRENT_SOURCE_DIR}"
				"${PROJECT_SOURCE_DIR}/cmake/template/"
			NO_DEFAULT_PATH
			NO_CACHE
			REQUIRED
		)
		set(
			CONFIG_PACKAGE_FILE
			"${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-config.cmake"
		)
		set(PACKAGE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PACKAGE_NAME}")
		configure_package_config_file(
			"${CONFIG_FILE_TEMPLATE}"
			"${CONFIG_PACKAGE_FILE}"
			INSTALL_DESTINATION "${PACKAGE_INSTALL_DIR}"
			PATH_VARS PROJECT_NAME PROJECT_VERSION
		)
		set(CONFIG_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-config-version.cmake")
		write_basic_package_version_file(
			"${CONFIG_VERSION_FILE}"
			VERSION "${PROJECT_VERSION}"
			COMPATIBILITY ExactVersion
		)

		install(
			FILES "${CONFIG_PACKAGE_FILE}" "${CONFIG_VERSION_FILE}"
			DESTINATION "${PACKAGE_INSTALL_DIR}"
			COMPONENT "${INSTALL_COMPONENT_NAME}"
		)

		set(CONFIG_TARGETS_FILE "${PACKAGE_NAME}-target.cmake")
		install(
			EXPORT "${EXPORT_NAME}"
			DESTINATION "${PACKAGE_INSTALL_DIR}"
			NAMESPACE gungnir::
			FILE "${CONFIG_TARGETS_FILE}"
			COMPONENT "${INSTALL_COMPONENT_NAME}"
		)
	else()
		message(
			DEBUG
			"install_target: Not installing a config package for `${NAME}`."
		)
	endif()
endfunction()
