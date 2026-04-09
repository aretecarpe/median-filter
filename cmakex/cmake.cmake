# =============================================================================
# cmake.cmake — Module aggregator
# =============================================================================
#
# Includes all project-level cmake utility modules. Include this file once from
# the top-level CMakeLists.txt to make every helper function available.
#
# Modules loaded:
#   build_type.cmake        — default_build_type()
#   compiler_warnings.cmake — set_project_warnings()
#   cuda.cmake              — setup_cuda(), set_cuda_runtime(),
#                             set_cuda_architectures(), cuda_detect_architectures()
#   install_target.cmake    — install_target()
#   add_component.cmake     — add_library_component()
#   install_project.cmake   — install_project()
#
# Usage:
#   include(cmake/cmake.cmake)
# =============================================================================
include_guard(GLOBAL)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(${CMAKE_CURRENT_LIST_DIR}/build_type.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/compiler_warnings.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cuda.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/hip.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/install_target.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/add_component.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/install_project.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)
