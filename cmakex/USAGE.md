# Usage Guide

Complete reference for the gungnir.exemplar cmake modules, consumer integration patterns, and instructions for extending the project with your own components.

## Table of contents

- [Consuming the library](#consuming-the-library)
- [CMake module reference](#cmake-module-reference)
  - [cmake.cmake](#cmakecmake)
  - [add_component.cmake](#add_componentcmake)
  - [install_project.cmake](#install_projectcmake)
  - [install_target.cmake](#install_targetcmake)
  - [compiler_warnings.cmake](#compiler_warningscmake)
  - [build_type.cmake](#build_typecmake)
  - [cuda.cmake](#cudacmake)
- [Adding a new component](#adding-a-new-component)
- [Adding an executable](#adding-an-executable)
- [Config-file package internals](#config-file-package-internals)
- [Adapting for your own project](#adapting-for-your-own-project)

---

## Consuming the library

### find_package with COMPONENTS

Request only the components you need. Transitive dependencies are resolved automatically.

```cmake
cmake_minimum_required(VERSION 3.25)
project(my_app LANGUAGES CXX)

# Requesting "math" automatically pulls in "core".
find_package(gungnir.exemplar REQUIRED COMPONENTS math)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE gungnir.exemplar::math)
```

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/gungnir/install
cmake --build build
```

### find_package without COMPONENTS (load all)

When no components are listed, every available component is loaded.

```cmake
find_package(gungnir.exemplar REQUIRED)

# ${gungnir.exemplar_LIBS} contains all imported targets.
target_link_libraries(my_app PRIVATE ${gungnir.exemplar_LIBS})
```

### Variables set by find_package

| Variable | Value |
|----------|-------|
| `gungnir.exemplar_FOUND` | `TRUE` if the package and all requested components were found |
| `gungnir.exemplar_VERSION` | Package version string (e.g. `0.1.0`) |
| `gungnir.exemplar_COMPONENTS` | List of component names that were loaded |
| `gungnir.exemplar_LIBS` | List of imported targets (e.g. `gungnir.exemplar::core;gungnir.exemplar::math`) |
| `gungnir.exemplar_<comp>_FOUND` | `TRUE` / `FALSE` for each requested component |

### Using as a subdirectory

You can also embed this project directly via `add_subdirectory()`:

```cmake
add_subdirectory(third_party/gungnir.exemplar)

# Link against targets directly (no find_package needed).
target_link_libraries(my_app PRIVATE gungnir.exemplar.core gungnir.exemplar.math)
```

When included as a subdirectory, `PROJECT_IS_TOP_LEVEL` is `FALSE`, so tests and examples are disabled by default.

---

## CMake module reference

All modules are loaded by including `cmake/cmake.cmake` from the top-level `CMakeLists.txt`. Each module uses `include_guard(GLOBAL)` so it is safe to include multiple times.

### cmake.cmake

Module aggregator. Include once to make all functions available.

```cmake
include(cmake/cmake.cmake)
```

Loads: `build_type.cmake`, `compiler_warnings.cmake`, `cuda.cmake`, `install_target.cmake`, `add_component.cmake`, `install_project.cmake`.

Also sets `CMAKE_EXPORT_COMPILE_COMMANDS ON` for tooling support.

---

### add_component.cmake

#### `add_library_component()`

Creates a library target, registers it in the component registry, applies warnings, and generates install rules. This is the primary function for defining library components.

```
add_library_component(<target_name>
    [STATIC | SHARED | INTERFACE]
    [STANDARD <std>]
    [WARNINGS_AS_ERRORS]
    [NAMESPACE <ns>]
    [SOURCES <file>...]
    [HEADERS <file>...]
    [HEADER_BASE_DIRS <dir>...]
    [DEPENDENCIES <target>...]
    [PUBLIC_DEPENDENCIES <target>...]
    [COMPONENT_DEPENDENCIES <component>...]
    [FIND_DEPENDENCIES <pkg>...]
)
```

**Component naming:** The component name is derived by stripping the `PROJECT_NAME` prefix from the target name. `gungnir.exemplar.core` under project `gungnir.exemplar` becomes component `core`.

**Options:**

| Option | Description |
|--------|-------------|
| `STATIC` | Build as a static library (default when none specified) |
| `SHARED` | Build as a shared library |
| `INTERFACE` | Header-only library (no compiled sources) |
| `WARNINGS_AS_ERRORS` | Promote compiler warnings to errors |

**Single-value arguments:**

| Argument | Default | Description |
|----------|---------|-------------|
| `STANDARD` | `20` | C++ standard to require |
| `NAMESPACE` | `${PROJECT_NAME}::` | Namespace prefix for the export set |

**Multi-value arguments:**

| Argument | Description |
|----------|-------------|
| `SOURCES` | Source files (`.cpp`, `.cu`, etc.) |
| `HEADERS` | Public header files to install |
| `HEADER_BASE_DIRS` | Base directories for `FILE_SET HEADERS` (default: `include`) |
| `DEPENDENCIES` | Targets to link `PRIVATE` |
| `PUBLIC_DEPENDENCIES` | Targets to link `PUBLIC` (propagated to consumers) |
| `COMPONENT_DEPENDENCIES` | Other component **names** (not full target names) that this component depends on. Recorded in the registry for automatic `find_package()` resolution. Also linked `PUBLIC` at build time. |
| `FIND_DEPENDENCIES` | External packages consumers must find (e.g. `CUDAToolkit`). Emitted as `find_dependency()` calls in the generated config. |

**Example — static library with CUDA dependency:**

```cmake
add_library_component(gungnir.exemplar.core
    STATIC
    STANDARD 20
    WARNINGS_AS_ERRORS
    SOURCES
        src/device_info.cpp
    HEADERS
        include/gungnir/exemplar/core/device_info.hpp
    HEADER_BASE_DIRS
        include
    FIND_DEPENDENCIES
        CUDAToolkit
)
```

**Example — component with inter-component dependency:**

```cmake
add_library_component(gungnir.exemplar.math
    STATIC
    WARNINGS_AS_ERRORS
    SOURCES
        src/transform.cu
    HEADERS
        include/gungnir/exemplar/math/transform.hpp
    HEADER_BASE_DIRS
        include
    COMPONENT_DEPENDENCIES
        core
)
```

**Example — header-only (INTERFACE) library:**

```cmake
add_library_component(gungnir.exemplar.types
    INTERFACE
    STANDARD 20
    HEADERS
        include/gungnir/exemplar/types/vec3.hpp
        include/gungnir/exemplar/types/mat4.hpp
    HEADER_BASE_DIRS
        include
)
```

---

### install_project.cmake

#### `install_project()`

Generates and installs the umbrella config-file package. Call **once** from the top-level `CMakeLists.txt`, **after** all `add_library_component()` calls.

```
install_project(
    [NAMESPACE <ns>]
    [COMPATIBILITY <compat>]
)
```

| Argument | Default | Description |
|----------|---------|-------------|
| `NAMESPACE` | `${PROJECT_NAME}::` | Namespace prefix for exported targets |
| `COMPATIBILITY` | `SameMajorVersion` | Version compatibility mode: `AnyNewerVersion`, `SameMajorVersion`, `SameMinorVersion`, or `ExactVersion` |

**Produces:**

```
<libdir>/cmake/<project>/
    <project>-config.cmake
    <project>-config-version.cmake
```

The per-component target files (`<project>-<comp>-targets.cmake`) are installed by `add_library_component()`.

**Example:**

```cmake
install_project(
    NAMESPACE "gungnir.exemplar::"
    COMPATIBILITY SameMajorVersion
)
```

---

### install_target.cmake

#### `install_target()`

Installs a single target. Use this for **executables** and **standalone libraries** that are not part of the component system.

```
install_target(<target_name>)
```

Behavior depends on target type:

| Target type | Install destination | Config package |
|-------------|-------------------|----------------|
| `EXECUTABLE` | `bin/` | No |
| `STATIC_LIBRARY` / `SHARED_LIBRARY` | `lib/`, headers, own config package | Yes (standalone) |
| `INTERFACE_LIBRARY` | Headers, own config package | Yes (standalone) |

For component libraries, use `add_library_component()` instead — it handles installation with the umbrella config.

**Example:**

```cmake
add_executable(gungnir.exemplar main.cu)
install_target(gungnir.exemplar)
```

---

### compiler_warnings.cmake

#### `set_project_warnings()`

Applies a curated set of compiler warnings to a target.

```
set_project_warnings(<target> [WARNINGS_AS_ERRORS])
```

Warning sets by compiler:

| Compiler | Warnings |
|----------|----------|
| **GCC** | `-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast -Wsuggest-override` |
| **Clang** | Same as GCC minus the GCC-specific flags |
| **MSVC** | `/W4` plus 20 specific `/w1xxxx` diagnostics, `/permissive-` |
| **CUDA** | `-Wall -Wextra -Wunused -Wconversion -Wshadow` |

`WARNINGS_AS_ERRORS` adds `-Werror` (GCC/Clang) or `/WX` (MSVC).

Interface libraries receive warnings via `INTERFACE`; all other types via `PRIVATE`.

---

### build_type.cmake

#### `default_build_type()`

Sets a default `CMAKE_BUILD_TYPE` for single-configuration generators (Makefiles, Ninja) when none was specified. Multi-config generators (Visual Studio, Xcode) are unaffected.

```
default_build_type(<type>)
```

**Example:**

```cmake
default_build_type("Release")
```

---

### cuda.cmake

#### `setup_cuda()`

One-call CUDA configuration for a target.

```
setup_cuda(<target>
    [STATIC_RUNTIME | SHARED_RUNTIME]
    [STANDARD <std>]
    [ARCHITECTURES <arch>...]
    [PER_THREAD_STREAMS]
)
```

| Argument | Description |
|----------|-------------|
| `STATIC_RUNTIME` | Link `cudart_static` |
| `SHARED_RUNTIME` | Link `cudart` (shared). Default if neither is specified. |
| `STANDARD` | CUDA language standard (default: `20`; auto-downgrades to `17` for older Clang/GCC) |
| `ARCHITECTURES` | Explicit SM architectures (e.g. `"80" "86"`). Auto-detected if omitted. |
| `PER_THREAD_STREAMS` | Pass `--default-stream=per-thread` to nvcc |

Sets target properties: `CUDA_STANDARD`, `CUDA_STANDARD_REQUIRED`, `CUDA_ARCHITECTURES`, `CUDA_RESOLVE_DEVICE_SYMBOLS`, `CUDA_SEPARABLE_COMPILATION`.

In `Debug` builds, adds `-g -G` for device debugging.

**Example:**

```cmake
enable_language(CUDA)
add_executable(my_app main.cu)
setup_cuda(my_app STATIC_RUNTIME ARCHITECTURES "86" "90" PER_THREAD_STREAMS)
```

#### `set_cuda_runtime()`

Low-level helper to link the CUDA runtime. Prefer `setup_cuda()`.

```
set_cuda_runtime(<target> USE_STATIC <bool>)
```

#### `set_cuda_architectures()`

Populates an output variable with detected GPU architectures (with `-real` suffix).

```
set_cuda_architectures(<out_var>)
```

Falls back to a built-in list filtered by CUDA toolkit version if auto-detection fails. Supported architectures: `70`, `75`, `80`, `86`, `87`, `90`, `100`, `120`.

#### `cuda_detect_architectures()`

Compiles and runs a small CUDA program to query locally installed GPU architectures. Called internally by `set_cuda_architectures()`.

```
cuda_detect_architectures(<possible_archs_var> <out_var>)
```

---

## Adding a new component

Follow this pattern to add a new component library (e.g. `io`):

### 1. Create the directory structure

```
src/gungnir/exemplar/io/
├── CMakeLists.txt
├── include/gungnir/exemplar/io/
│   └── reader.hpp
└── src/
    └── reader.cpp
```

### 2. Write the CMakeLists.txt

```cmake
add_library_component(gungnir.exemplar.io
    STATIC
    STANDARD 20
    WARNINGS_AS_ERRORS
    SOURCES
        src/reader.cpp
    HEADERS
        include/gungnir/exemplar/io/reader.hpp
    HEADER_BASE_DIRS
        include
    COMPONENT_DEPENDENCIES
        core
    FIND_DEPENDENCIES
        Threads
)
```

### 3. Register in the top-level CMakeLists.txt

Add the subdirectory **before** any targets that depend on it, and **before** `install_project()`:

```cmake
# Component libraries (order matters — dependencies first)
add_subdirectory(src/gungnir/exemplar/core)
add_subdirectory(src/gungnir/exemplar/io)     # NEW
add_subdirectory(src/gungnir/exemplar/math)

# ... executable, tests, etc. ...

# Must come after all add_library_component() calls
install_project(
    NAMESPACE "gungnir.exemplar::"
    COMPATIBILITY SameMajorVersion
)
```

### 4. Done

After rebuild and install, consumers can use it:

```cmake
find_package(gungnir.exemplar REQUIRED COMPONENTS io)
target_link_libraries(app PRIVATE gungnir.exemplar::io)
# "core" is pulled in automatically via COMPONENT_DEPENDENCIES.
```

---

## Adding an executable

Executables do not participate in the component system. Use `install_target()` directly:

```cmake
set(TARGET_NAME gungnir.exemplar)
add_executable(${TARGET_NAME})

target_sources(${TARGET_NAME} PRIVATE scratch.cu)

target_link_libraries(${TARGET_NAME}
    PRIVATE
    gungnir.exemplar.core
    gungnir.exemplar.math
)

setup_cuda(${TARGET_NAME} STATIC_RUNTIME ARCHITECTURES "87")
set_project_warnings(${TARGET_NAME} WARNINGS_AS_ERRORS)

# Installs to bin/
install_target(${TARGET_NAME})
```

---

## Config-file package internals

Understanding how the generated config works helps when debugging consumer issues.

### File layout after install

```
lib/cmake/gungnir.exemplar/
├── gungnir.exemplar-config.cmake               # Umbrella — dispatches COMPONENTS
├── gungnir.exemplar-config-version.cmake       # Version check
├── gungnir.exemplar-core-targets.cmake         # Imported target: gungnir.exemplar::core
├── gungnir.exemplar-core-targets-release.cmake # Per-config properties (lib path, etc.)
├── gungnir.exemplar-math-targets.cmake         # Imported target: gungnir.exemplar::math
└── gungnir.exemplar-math-targets-release.cmake
```

### What the umbrella config does

When a consumer calls `find_package(gungnir.exemplar REQUIRED COMPONENTS math)`:

1. **External dependencies** are found via `find_dependency()` (e.g. `CUDAToolkit`).
2. **Component dependencies** are resolved transitively. Requesting `math` discovers that `core` is required.
3. Components are **topologically sorted** so `core` is loaded before `math`.
4. Each component's **targets file** is included, creating the imported targets.
5. **Convenience variables** are set: `_COMPONENTS`, `_LIBS`, `_<comp>_FOUND`.
6. `check_required_components()` validates that everything requested was found.

### Templates

| Template | Used by | Purpose |
|----------|---------|---------|
| `cmake/template/ProjectConfig.cmake.in` | `install_project()` | Umbrella config with COMPONENTS dispatch |
| `cmake/template/Config.cmake.in` | `install_target()` | Simple config for standalone libraries |

---

## Adapting for your own project

To use this as a starting point for your own project:

### 1. Rename the project

In the top-level `CMakeLists.txt`, change the project name:

```cmake
project(
    myorg.myproject
    DESCRIPTION "My project."
    LANGUAGES CXX
    VERSION 1.0.0
)
```

### 2. Set the namespace

In the `install_project()` call:

```cmake
install_project(
    NAMESPACE "myorg.myproject::"
    COMPATIBILITY SameMajorVersion
)
```

### 3. Update directory structure

Rename `src/gungnir/exemplar/` to match your project naming, and update `include/` paths accordingly.

### 4. Replace example components

Delete the `core` and `math` examples and add your own components using `add_library_component()`.

### 5. Non-CUDA projects

If you don't need CUDA, remove:
- `enable_language(CUDA)` calls
- `setup_cuda()` calls
- `cmake/cuda.cmake` from `cmake/cmake.cmake`
- `FIND_DEPENDENCIES CUDAToolkit`

The rest of the cmake infrastructure (components, warnings, install) works for pure C++ projects.
