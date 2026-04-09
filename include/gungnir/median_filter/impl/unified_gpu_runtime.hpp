#ifndef GUNGNIR_MEDIAN_FILTER_IMPL_UNIFIED_GPU_RUNTIME_HEADER_INCLUDED
#define GUNGNIR_MEDIAN_FILTER_IMPL_UNIFIED_GPU_RUNTIME_HEADER_INCLUDED

#ifdef GUNGNIR_USE_HIP

#include <hip/hip_runtime.h>

// HIP does not define __forceinline__ as a keyword; map it to the compiler attr.
#ifndef __forceinline__
#define __forceinline__ __inline__ __attribute__((always_inline))
#endif

// Stream and error types
using uniStream_t = hipStream_t;
using uniError_t  = hipError_t;
#define uniSuccess hipSuccess

// Memory management
#define uniMalloc              hipMalloc
#define uniFree                hipFree
#define uniMemcpy              hipMemcpy
#define uniMemcpyHostToDevice  hipMemcpyHostToDevice
#define uniMemcpyDeviceToHost  hipMemcpyDeviceToHost

// Synchronization
#define uniDeviceSynchronize   hipDeviceSynchronize

// Device query
#define uniGetDeviceCount      hipGetDeviceCount
#define uniGetDeviceProperties hipGetDeviceProperties
using uniDeviceProp = hipDeviceProp_t;

#else

#include <cuda_runtime.h>

// Stream and error types
using uniStream_t = cudaStream_t;
using uniError_t  = cudaError_t;
#define uniSuccess cudaSuccess

// Memory management
#define uniMalloc              cudaMalloc
#define uniFree                cudaFree
#define uniMemcpy              cudaMemcpy
#define uniMemcpyHostToDevice  cudaMemcpyHostToDevice
#define uniMemcpyDeviceToHost  cudaMemcpyDeviceToHost

// Synchronization
#define uniDeviceSynchronize   cudaDeviceSynchronize

// Device query
#define uniGetDeviceCount      cudaGetDeviceCount
#define uniGetDeviceProperties cudaGetDeviceProperties
using uniDeviceProp = cudaDeviceProp;

#endif // GUNGNIR_USE_HIP

#include <cstdio>
#include <cstdlib>

#ifdef GUNGNIR_USE_HIP
#define UNI_CHECK(call)                                                        \
	do {                                                                       \
		hipError_t err_ = (call);                                              \
		if (err_ != hipSuccess) {                                              \
			std::fprintf(stderr, "%s:%d: %s returned %s\n",                    \
				__FILE__, __LINE__, #call, hipGetErrorString(err_));            \
			std::abort();                                                      \
		}                                                                      \
	} while (0)
#else
#define UNI_CHECK(call)                                                        \
	do {                                                                       \
		cudaError_t err_ = (call);                                             \
		if (err_ != cudaSuccess) {                                             \
			std::fprintf(stderr, "%s:%d: %s returned %s\n",                    \
				__FILE__, __LINE__, #call, cudaGetErrorString(err_));           \
			std::abort();                                                      \
		}                                                                      \
	} while (0)
#endif

#endif
