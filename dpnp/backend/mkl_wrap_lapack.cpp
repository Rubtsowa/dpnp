//*****************************************************************************
// Copyright (c) 2016-2020, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//*****************************************************************************

#include <algorithm>
#include <iostream>
#include <mkl_lapack_sycl.hpp>

#include <backend/backend_iface.hpp>
#include "queue_sycl.hpp"

template <typename _DataType>
void mkl_lapack_syevd_c(void* array_in, void* result1, size_t size)
{
    cl::sycl::event status;

    _DataType* array = reinterpret_cast<_DataType*>(array_in);
    _DataType* result = reinterpret_cast<_DataType*>(result1);

    const std::int64_t lda = std::max<size_t>(1UL, size);

    auto queue = DPNP_QUEUE;

    const std::int64_t scratchpad_size =
        mkl::lapack::syevd_scratchpad_size<_DataType>(queue, mkl::job::vec, mkl::uplo::upper, size, lda);
    // const std::int64_t scratchpad_size = 7; // size = 2 & novec & lower
    // const std::int64_t scratchpad_size = 34; // size = 2 & vec & upper

    _DataType* scratchpad = reinterpret_cast<_DataType*>(dpnp_memory_alloc_c(scratchpad_size * sizeof(_DataType)));

#if 1
    printf("mkl_lapack_syevd_c array ptr type %d \n", cl::sycl::get_pointer_type(array, queue.get_context()));
    printf("mkl_lapack_syevd_c result ptr type %d \n", cl::sycl::get_pointer_type(result, queue.get_context()));
    printf("mkl_lapack_syevd_c scratchpad ptr type %d \n", cl::sycl::get_pointer_type(scratchpad, queue.get_context()));
    printf("mkl_lapack_syevd_c array[0] %g \n", array[0]);
    printf("mkl_lapack_syevd_c result[0] %g \n", result[0]);
    std::cout << "mkl_lapack_syevd_c size = " << size << std::endl;
    std::cout << "mkl_lapack_syevd_c lda = " << lda << std::endl;
    std::cout << "mkl_lapack_syevd_c scratchpad_size = " << scratchpad_size << std::endl;
#endif

    try
    {
        queue.wait_and_throw();
        status = mkl::lapack::syevd(queue,            // queue
                                    mkl::job::vec,    // jobz
                                    mkl::uplo::upper, // uplo
                                    size,             // The order of the matrix A (0≤n)
                                    array,            // will be overwritten with eigenvectors
                                    lda,
                                    result,
                                    scratchpad,
                                    scratchpad_size);
    }
    catch (mkl::lapack::exception const& e)
    {
        // Handle LAPACK related exceptions happened during asynchronous call
        std::cout << "Unexpected exception caught during asynchronous LAPACK operation:\n"
                  << e.reason() << "\ninfo: " << e.info() << std::endl;
    }
    catch (cl::sycl::exception const& e)
    {
        std::cerr << "Caught synchronous SYCL exception during mkl_lapack_syevd_c():\n"
                  << e.what() << "\nOpenCL status: " << e.get_cl_code() << std::endl;
    }

    status.wait();

#if 1
    std::cout << "mkl_lapack_syevd_c res = " << result[0] << std::endl;
#endif

    dpnp_memory_free_c(scratchpad);
}

template void mkl_lapack_syevd_c<double>(void* array1_in, void* result1, size_t size);
template void mkl_lapack_syevd_c<float>(void* array1_in, void* result1, size_t size);