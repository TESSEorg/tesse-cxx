#ifndef TTG_DEVICE_CUBLAS_HELPER_H
#define TTG_DEVICE_CUBLAS_HELPER_H

#include "ttg/config.h"

#ifdef TTG_HAVE_CUDART
#include <cublas_v2.h>

namespace ttg::detail {

/// \brief Returns the cuBLAS context used by this thread
const cublasHandle_t& cublas_get_handle();

/// associates the given CUDA stream with the cuBLAS context used by this thread
void cublas_set_kernel_stream(cudaStream_t stream);

} // namespace ttg::detail
#endif // TTG_HAVE_CUDART


#ifdef TTG_HAVE_HIPBLAS
#include <hipblas/hipblas.h>

namespace ttg::detail {

const hipblasHandle_t& hipblas_get_handle();

/// associates the given HIP stream with the hipblas context used by this thread
void hipblas_set_kernel_stream(hipStream_t stream);

} // namespace ttg::detail
#endif // TTG_HAVE_HIPBLAS


#endif // TTG_DEVICE_CUBLAS_HELPER_H