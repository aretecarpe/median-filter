# Gungnir Median Filter

Header-only C++17 library. Runs on both NVIDIA (CUDA) and AMD (ROCm/HIP) GPUs with zero source changes.

## Why median filtering is hard to make fast

Median filtering is deceptively simple to describe - replace each pixel with the median of its neighborhood, but it is one of the most optimization-resistant operations in image processing. Here is why:

**There is no closed-form median.** Unlike a box blur, where you accumulate a running sum and divide, the median has no algebraic shortcut. You must actually determine which value sits in the middle of the sorted order. For a 7x7 filter, that means ranking 49 values per pixel. At 4K resolution that is 8.3 million pixels, each requiring a 49-element sort. Over 400 million compare-and-swap operations per frame.

**It is fundamentally comparison-bound, not arithmetic-bound.** GPUs are designed to hide arithmetic latency through massive parallelism, but median filtering is dominated by data-dependent branching (comparisons and conditional swaps). This makes it resistant to the usual GPU trick of throwing more ALUs at the problem. Each comparison result determines which comparison happens next, creating serial dependency chains that stall pipelines.

**Memory access is the enemy.** Each output pixel reads from a neighborhood that overlaps with its neighbors. A naive implementation re-reads the same pixels dozens of times from global memory. The 7x7 case reads 49 values per pixel, but adjacent pixels share 42 of those 49 values. Without careful shared memory management, the kernel becomes memory-bandwidth-bound long before the compute units are saturated.

**The sorting problem scales quadratically.** Naive sorting algorithms (insertion sort, bubble sort) require O(n^2) comparisons. For a 7x7 window, that is 1,176 comparisons per pixel. Even efficient general-purpose sorts like merge sort achieve O(n log n) = ~275 comparisons per pixel, but they have irregular memory access patterns that do not vectorize well on GPUs. You need an algorithm whose structure is known at compile time so the compiler can keep everything in registers.

**You do not actually need a full sort.** This is the key insight most implementations miss. The median is a single order statistic. A full sort does far more work than necessary, placing all 49 elements in order when you only care about element 24. But exploiting this requires a way to determine at compile time which comparisons in a sorting network can actually influence the median position, a non-trivial static analysis problem.

## How this implementation solves it

### Sorting networks: branch-free, register-only sorting

Instead of runtime sorting algorithms with loops and branches, this library uses **sorting networks** a fixed sequences of compare-and-swap operations determined entirely at compile time. A sorting network for N elements is a static sequence of pairs `(i, j)` meaning "compare elements i and j, swap if out of order." The sequence is the same regardless of input data.

This is a perfect fit for GPU execution:

- **Branch-free.** Every thread in a warp executes the exact same instruction sequence. No warp divergence. No branch misprediction.
- **Register-resident.** The entire neighborhood (49 floats for a 7x7 filter = 196 bytes) fits in registers. The sorting network operates entirely on registers with zero memory traffic.
- **Compile-time structure.** The compiler sees the entire sequence of operations and can schedule them optimally, interleaving independent comparisons to hide latency.

Six network generation algorithms are provided:

| Algorithm | Description | Best for |
|---|---|---|
| `bitonic_merge_sort` | Bitonic merge sort network | Power-of-two sizes, GPU-friendly parallel structure |
| `bose_nelson_sort` | Bose-Nelson (1962) construction | Good general-purpose, any size |
| `batcher_odd_even_merge_sort` | Batcher's odd-even merge | Power-of-two sizes only |
| `insertion_sort` | Insertion sort network | Small N, simple structure |
| `bubble_sort` | Bubble sort network | Baseline comparison |
| `size_optimized_sort` | Hardcoded optimal networks for N=1..32, Bose-Nelson fallback | Minimum comparison count |

### Median selection networks: do less work, get the same answer

The biggest single optimization is the **median selection network**. Given a full sorting network topology, a compile-time backwards dependency analysis identifies which compare-and-swap operations can possibly influence the value at the median position (index N/2). Every operation that cannot influence the median is pruned.

The pruning algorithm:

1. Start with the set `{N/2}` (the median position)
2. Walk backwards through the sorting network
3. For each compare-and-swap pair `(a, b)`: if either `a` or `b` is in the needed set, keep the operation and add both `a` and `b` to the needed set
4. Discard all operations not marked as needed

This produces a **partial sorting network** that guarantees `array[N/2]` contains the true median, while performing significantly fewer comparisons:

| Algorithm (N=49) | Full sort comparisons | Median-only comparisons | Reduction |
|---|---|---|---|
| Bitonic merge sort | 672 | ~400 | ~40% |
| Bose-Nelson | 480 | ~290 | ~40% |
| Size-optimized | 344 | ~210 | ~39% |

All of this analysis happens at compile time via C++ template metaprogramming. The runtime cost is zero.

### Shared memory tiling with halo regions

The kernel uses a tiled shared memory strategy:

1. Each thread block loads a tile of the input image into shared memory, including **halo pixels** (the border region needed for filter neighborhoods at tile edges)
2. The load is cooperative, threads collectively load the entire tile including halos, using compile-time-unrolled nested loops
3. After a single `__syncthreads()`, every thread can read its full neighborhood from shared memory with no global memory traffic
4. For a 16x16 block with a 7x7 filter, the shared memory tile is 22x22, each global memory value is loaded exactly once but read up to 49 times from shared memory

### Compile-time loop unrolling

All nested loops over the filter neighborhood are unrolled at compile time using `compile_time_grid_for_each`, a constexpr utility that generates a flat sequence of operations from a 2D grid via `std::index_sequence`. The compiler sees no loops, just a straight-line sequence of loads and compare-and-swaps. This eliminates loop overhead and allows maximum instruction-level parallelism.

### Branchless min/max comparisons

The compare-and-swap operation in the kernel uses hardware `min`/`max` instructions instead of branching:

```cpp
sorter(local_pixels, [](auto &a, auto &b) {
    const auto temp = a;
    a = min(a, b);
    b = max(temp, b);
});
```

This compiles to native `v_min_f32` / `v_max_f32` (AMD) or `fmin` / `fmax` (NVIDIA) instructions. Two instructions per comparison, no branches, no predication overhead.

## Performance

| GPU | Image | Filter | Time |
|---|---|---|---|
| AMD MI300X | 3840x2160 (4K) | 7x7 | **174 us** |

## Requirements

- CMake >= 3.25
- C++17 or later
- One of:
  - NVIDIA: CUDA Toolkit (nvcc)
  - AMD: ROCm (hipcc)

## Building

### AMD (ROCm / HIP)

```bash
cmake -B build \
    -DGPU_BACKEND=HIP \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

To target a specific architecture (default is `gfx942` for MI300X):

```bash
cmake -B build \
    -DGPU_BACKEND=HIP \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_HIP_ARCHITECTURES="gfx90a;gfx942"

cmake --build build
```

Common AMD architecture targets:
| GPU | Architecture |
|---|---|
| MI300X / MI300A | `gfx942` |
| MI250X / MI210 | `gfx90a` |
| MI100 | `gfx908` |
| RX 7900 XTX | `gfx1100` |

### NVIDIA (CUDA)

```bash
cmake -B build \
    -DGPU_BACKEND=CUDA \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

To target a specific architecture (default is SM 87):

```bash
cmake -B build \
    -DGPU_BACKEND=CUDA \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CUDA_ARCHITECTURES="80;86;89;90"

cmake --build build
```

Common NVIDIA architecture targets:
| GPU | SM |
|---|---|
| A100 | `80` |
| RTX 3090 | `86` |
| RTX 4090 | `89` |
| H100 | `90` |

### Build options

| Option | Default | Description |
|---|---|---|
| `GPU_BACKEND` | `CUDA` | GPU backend: `CUDA` or `HIP` |
| `gungnir.median_filter_BUILD_EXAMPLES` | `ON` (top-level) | Build example executable |
| `gungnir.median_filter_BUILD_TESTS` | `ON` (top-level) | Build tests |

## Usage

### Integration

This is a header-only library. Add it to your project via CMake:

```cmake
find_package(gungnir.median_filter 0.1.0 REQUIRED)
target_link_libraries(your_target PRIVATE gungnir::median_filter)
```

Or add as a subdirectory:

```cmake
add_subdirectory(median-filter)
target_link_libraries(your_target PRIVATE gungnir::median_filter)
```

### API

Single function call:

```cpp
#include <gungnir/median_filter/median_filter_2d.hpp>

namespace medfilt = gungnir::median_filter;

// Apply a 7x7 median filter using bitonic merge sort
medfilt::median_filter_2d<
    float,       // pixel type
    7,           // filter size (must be odd)
    medfilt::SortingAlgorithm::bitonic_merge_sort,  // sorting algorithm
    false,       // true = use median selection (fewer comparisons)
    16           // thread block size (default 16x16)
>(
    d_input,                                          // input device pointer
    static_cast<uint32_t>(width * sizeof(float)),     // input row stride in bytes
    d_output,                                         // output device pointer
    static_cast<uint32_t>(width * sizeof(float)),     // output row stride in bytes
    width,                                            // image width in pixels
    height,                                           // image height in pixels
    gpu_stream                                        // GPU stream (default 0)
);
```

### Template parameters

| Parameter | Type | Description |
|---|---|---|
| `T` | typename | Pixel type (`float`, `double`, `uint8_t`, etc.) |
| `FilterSize` | `int32_t` | Filter window size. Must be odd (3, 5, 7, 9, ...) |
| `Algorithm` | `SortingAlgorithm` | Sorting network algorithm (default: `bose_nelson_sort`) |
| `UseMedianSelection` | `bool` | Use pruned median-only network (default: `false`) |
| `BlockSize` | `int32_t` | Thread block dimension (default: `16`, meaning 16x16 blocks) |

### Choosing an algorithm

For most cases, use `bitonic_merge_sort` or `size_optimized_sort`:

```cpp
// Fastest for general use fewest comparisons for N <= 32
medfilt::median_filter_2d<float, 5, medfilt::SortingAlgorithm::size_optimized_sort>(
    d_in, stride, d_out, stride, w, h);

// Good all-around choice, works with any filter size
medfilt::median_filter_2d<float, 7, medfilt::SortingAlgorithm::bitonic_merge_sort>(
    d_in, stride, d_out, stride, w, h);

// Enable median selection for ~40% fewer comparisons (same result)
medfilt::median_filter_2d<float, 7, medfilt::SortingAlgorithm::bitonic_merge_sort, true>(
    d_in, stride, d_out, stride, w, h);
```

### Pitched memory support

The stride parameters support pitched (row-padded) GPU allocations:

```cpp
// Using pitched allocation
size_t pitch;
float *d_input;
uniMallocPitch(&d_input, &pitch, width * sizeof(float), height);

medfilt::median_filter_2d<float, 5>(
    d_input, static_cast<uint32_t>(pitch),
    d_output, static_cast<uint32_t>(pitch),
    width, height);
```

### Unified runtime API

The library provides a vendor-neutral GPU runtime API through `unified_gpu_runtime.hpp`:

```cpp
#include <gungnir/median_filter/impl/unified_gpu_runtime.hpp>

// Works on both CUDA and HIP
uniStream_t stream;
float *d_ptr;
UNI_CHECK(uniMalloc(&d_ptr, size));
UNI_CHECK(uniMemcpy(d_ptr, h_ptr, size, uniMemcpyHostToDevice));
UNI_CHECK(uniDeviceSynchronize());
UNI_CHECK(uniFree(d_ptr));
```

The `UNI_CHECK` macro validates return codes and aborts with file, line, and error string on failure.

## Project structure

```
include/gungnir/median_filter/
    median_filter_2d.hpp                          User-facing API
    sorting_network_sorter.hpp                    Full sorting network sorter
    median_selection_network_sorter.hpp           Pruned median-only sorter
    impl/
        unified_gpu_runtime.hpp                   CUDA/HIP abstraction + UNI_CHECK
        ceiling_division.hpp                      ceil_div utility
        pitched_image.hpp                         Pitched GPU memory wrapper
        bounds_check.hpp                          Branchless bounds checking
        kernel/
            median_filter_2d_kernel.hpp           GPU kernel implementation
        sorting/
            sorting_network_executor.hpp          Type-level network execution engine
            compile_time_grid_loop.hpp            Constexpr 2D loop unrolling
            compare_and_swap.hpp                  Generic compare-and-swap functor
            bitonic_merge_sort_network.hpp         Bitonic merge sort topology
            bose_nelson_sort_network.hpp           Bose-Nelson topology
            batcher_odd_even_merge_sort_network.hpp Batcher's odd-even merge topology
            insertion_sort_network.hpp             Insertion sort topology
            bubble_sort_network.hpp                Bubble sort topology
            size_optimized_sort_network.hpp        Hardcoded optimal networks (N<=32)
            median_selection_network_builder.hpp   Compile-time network pruning
```

## License

See [LICENSE](LICENSE) for details.
