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

#include <iostream>

#include "dpnp_fptr.hpp"
#include "dpnp_iface.hpp"
#include "queue_sycl.hpp"

template <typename _KernelNameSpecialization>
class dpnp_arange_c_kernel;

template <typename _DataType>
void dpnp_arange_c(size_t start, size_t step, void* result1, size_t size)
{
    // parameter `size` used instead `stop` to avoid dependency on array length calculation algorithm
    // TODO: floating point (and negatives) types from `start` and `step`

    if (!size)
    {
        return;
    }

    cl::sycl::event event;

    _DataType* result = reinterpret_cast<_DataType*>(result1);

    cl::sycl::range<1> gws(size);
    auto kernel_parallel_for_func = [=](cl::sycl::id<1> global_id) {
        size_t i = global_id[0];

        result[i] = start + i * step;
    };

    auto kernel_func = [&](cl::sycl::handler& cgh) {
        cgh.parallel_for<class dpnp_arange_c_kernel<_DataType>>(gws, kernel_parallel_for_func);
    };

    event = DPNP_QUEUE.submit(kernel_func);

    event.wait();
}

template <typename _DataType>
void dpnp_diag_c(
    void* v_in, void* result1, const int k, size_t* shape, size_t* res_shape, const size_t ndim, const size_t res_ndim)
{
    // avoid warning unused variable
    (void)res_ndim;

    _DataType* v = reinterpret_cast<_DataType*>(v_in);
    _DataType* result = reinterpret_cast<_DataType*>(result1);

    size_t init0 = std::max(0, -k);
    size_t init1 = std::max(0, k);

    if (ndim == 1)
    {
        for (size_t i = 0; i < shape[0]; ++i)
        {
            size_t ind = (init0 + i) * res_shape[1] + init1 + i;
            result[ind] = v[i];
        }
    }
    else
    {
        for (size_t i = 0; i < res_shape[0]; ++i)
        {
            size_t ind = (init0 + i) * shape[1] + init1 + i;
            result[i] = v[ind];
        }
    }
    return;
}

template <typename _KernelNameSpecialization>
class dpnp_full_c_kernel;

template <typename _DataType>
void dpnp_full_c(void* array_in, void* result, const size_t size)
{
    dpnp_initval_c<_DataType>(result, array_in, size);
}

template <typename _DataType>
void dpnp_full_like_c(void* array_in, void* result, const size_t size)
{
    dpnp_full_c<_DataType>(array_in, result, size);
}

template <typename _KernelNameSpecialization>
class dpnp_identity_c_kernel;

template <typename _DataType>
void dpnp_identity_c(void* result1, const size_t n)
{
    if (n == 0)
    {
        return;
    }

    cl::sycl::event event;

    _DataType* result = reinterpret_cast<_DataType*>(result1);

    cl::sycl::range<2> gws(n, n);
    auto kernel_parallel_for_func = [=](cl::sycl::id<2> global_id) {
        size_t i = global_id[0];
        size_t j = global_id[1];
        result[i * n + j] = i == j;
    };

    auto kernel_func = [&](cl::sycl::handler& cgh) {
        cgh.parallel_for<class dpnp_identity_c_kernel<_DataType>>(gws, kernel_parallel_for_func);
    };

    event = DPNP_QUEUE.submit(kernel_func);

    event.wait();
}

template <typename _DataType>
void dpnp_ones_c(void* result, size_t size)
{
    _DataType* fill_value = reinterpret_cast<_DataType*>(dpnp_memory_alloc_c(sizeof(_DataType)));
    fill_value[0] = 1;

    dpnp_initval_c<_DataType>(result, fill_value, size);

    dpnp_memory_free_c(fill_value);
}

template <typename _DataType>
void dpnp_ones_like_c(void* result, size_t size)
{
    dpnp_ones_c<_DataType>(result, size);
}

template <typename _DataType_input, typename _DataType_output>
void dpnp_vander_c(const void* array1_in, void* result1, const size_t size_in, const size_t N, const int increasing)
{
    if ((array1_in == nullptr) || (result1 == nullptr))
        return;

    if (!size_in || !N)
        return;

    const _DataType_input* array_in = reinterpret_cast<const _DataType_input*>(array1_in);
    _DataType_output* result = reinterpret_cast<_DataType_output*>(result1);

    if (N == 1)
    {
        dpnp_ones_c<_DataType_output>(result, size_in);
        return;
    }

    if (increasing)
    {
        for (size_t i = 0; i < size_in; ++i)
        {
            result[i * N] = 1;
        }
        for (size_t i = 1; i < N; ++i)
        {
            for (size_t j = 0; j < size_in; ++j)
            {
                result[j * N + i] = result[j * N + i - 1] * array_in[j];
            }
        }
    }
    else
    {
        for (size_t i = 0; i < size_in; ++i)
        {
            result[i * N + N - 1] = 1;
        }
        for (size_t i = N - 2; i > 0; --i)
        {
            for (size_t j = 0; j < size_in; ++j)
            {
                result[j * N + i] = result[j * N + i + 1] * array_in[j];
            }
        }

        for (size_t i = 0; i < size_in; ++i)
        {
            result[i * N] = result[i * N + 1] * array_in[i];
        }
    }
}

template <typename _DataType, typename _ResultType>
class dpnp_trace_c_kernel;

template <typename _DataType, typename _ResultType>
void dpnp_trace_c(const void* array1_in, void* result1, const size_t* shape_, const size_t ndim)
{
    cl::sycl::event event;

    if ((array1_in == nullptr) || (result1 == nullptr))
    {
        return;
    }

    const _DataType* array_in = reinterpret_cast<const _DataType*>(array1_in);
    _ResultType* result = reinterpret_cast<_ResultType*>(result1);

    if (shape_ == nullptr)
    {
        return;
    }

    if (ndim == 0)
    {
        return;
    }

    size_t size = 1;
    for (size_t i = 0; i < ndim - 1; ++i)
    {
        size *= shape_[i];
    }

    if (size == 0)
    {
        return;
    }

    size_t* shape = reinterpret_cast<size_t*>(dpnp_memory_alloc_c(ndim * sizeof(size_t)));
    auto memcpy_event = DPNP_QUEUE.memcpy(shape, shape_, ndim * sizeof(size_t));

    cl::sycl::range<1> gws(size);
    auto kernel_parallel_for_func = [=](cl::sycl::id<1> global_id) {
        size_t i = global_id[0];
        result[i] = 0;
        for (size_t j = 0; j < shape[ndim - 1]; ++j)
        {
            result[i] += array_in[i * shape[ndim - 1] + j];
        }
    };

    auto kernel_func = [&](cl::sycl::handler& cgh) {
        cgh.depends_on({memcpy_event});
        cgh.parallel_for<class dpnp_trace_c_kernel<_DataType, _ResultType>>(gws, kernel_parallel_for_func);
    };

    event = DPNP_QUEUE.submit(kernel_func);

    event.wait();

    dpnp_memory_free_c(shape);
}

template <typename _DataType>
class dpnp_tri_c_kernel;

template <typename _DataType>
void dpnp_tri_c(void* result1, const size_t N, const size_t M, const int k)
{
    cl::sycl::event event;

    if (!result1 || !N || !M)
    {
        return;
    }

    _DataType* result = reinterpret_cast<_DataType*>(result1);

    size_t idx = N * M;
    cl::sycl::range<1> gws(idx);
    auto kernel_parallel_for_func = [=](cl::sycl::id<1> global_id) {
        size_t ind = global_id[0];
        size_t i = ind / M;
        size_t j = ind % M;

        int val = i + k + 1;
        size_t diag_idx_ = (val > 0) ? (size_t)val : 0;
        size_t diag_idx = (M < diag_idx_) ? M : diag_idx_;

        if (j < diag_idx)
        {
            result[ind] = 1;
        }
        else
        {
            result[ind] = 0;
        }
    };

    auto kernel_func = [&](cl::sycl::handler& cgh) {
        cgh.parallel_for<class dpnp_tri_c_kernel<_DataType>>(gws, kernel_parallel_for_func);
    };

    event = DPNP_QUEUE.submit(kernel_func);

    event.wait();
}

template <typename _DataType>
void dpnp_tril_c(void* array_in,
                 void* result1,
                 const int k,
                 size_t* shape,
                 size_t* res_shape,
                 const size_t ndim,
                 const size_t res_ndim)
{
    if ((array_in == nullptr) || (result1 == nullptr))
    {
        return;
    }

    _DataType* array_m = reinterpret_cast<_DataType*>(array_in);
    _DataType* result = reinterpret_cast<_DataType*>(result1);

    if ((shape == nullptr) || (res_shape == nullptr))
    {
        return;
    }

    if ((ndim == 0) || (res_ndim == 0))
    {
        return;
    }

    size_t res_size = 1;
    for (size_t i = 0; i < res_ndim; ++i)
    {
        res_size *= res_shape[i];
    }

    if (res_size == 0)
    {
        return;
    }

    if (ndim == 1)
    {
        for (size_t i = 0; i < res_size; ++i)
        {
            size_t n = res_size;
            size_t val = i;
            int ids[res_ndim];
            for (size_t j = 0; j < res_ndim; ++j)
            {
                n /= res_shape[j];
                size_t p = val / n;
                ids[j] = p;
                if (p != 0)
                {
                    val = val - p * n;
                }
            }

            int diag_idx_ = (ids[res_ndim - 2] + k > -1) ? (ids[res_ndim - 2] + k) : -1;
            int values = res_shape[res_ndim - 1];
            int diag_idx = (values < diag_idx_) ? values : diag_idx_;

            if (ids[res_ndim - 1] <= diag_idx)
            {
                result[i] = array_m[ids[res_ndim - 1]];
            }
            else
            {
                result[i] = 0;
            }
        }
    }
    else
    {
        for (size_t i = 0; i < res_size; ++i)
        {
            size_t n = res_size;
            size_t val = i;
            int ids[res_ndim];
            for (size_t j = 0; j < res_ndim; ++j)
            {
                n /= res_shape[j];
                size_t p = val / n;
                ids[j] = p;
                if (p != 0)
                {
                    val = val - p * n;
                }
            }

            int diag_idx_ = (ids[res_ndim - 2] + k > -1) ? (ids[res_ndim - 2] + k) : -1;
            int values = res_shape[res_ndim - 1];
            int diag_idx = (values < diag_idx_) ? values : diag_idx_;

            if (ids[res_ndim - 1] <= diag_idx)
            {
                result[i] = array_m[i];
            }
            else
            {
                result[i] = 0;
            }
        }
    }
    return;
}

template <typename _DataType>
void dpnp_triu_c(void* array_in,
                 void* result1,
                 const int k,
                 size_t* shape,
                 size_t* res_shape,
                 const size_t ndim,
                 const size_t res_ndim)
{
    if ((array_in == nullptr) || (result1 == nullptr))
    {
        return;
    }
    _DataType* array_m = reinterpret_cast<_DataType*>(array_in);
    _DataType* result = reinterpret_cast<_DataType*>(result1);

    if ((shape == nullptr) || (res_shape == nullptr))
    {
        return;
    }

    if ((ndim == 0) || (res_ndim == 0))
    {
        return;
    }

    size_t res_size = 1;
    for (size_t i = 0; i < res_ndim; ++i)
    {
        res_size *= res_shape[i];
    }

    if (res_size == 0)
    {
        return;
    }

    if (ndim == 1)
    {
        for (size_t i = 0; i < res_size; ++i)
        {
            size_t n = res_size;
            size_t val = i;
            int ids[res_ndim];
            for (size_t j = 0; j < res_ndim; ++j)
            {
                n /= res_shape[j];
                size_t p = val / n;
                ids[j] = p;
                if (p != 0)
                {
                    val = val - p * n;
                }
            }

            int diag_idx_ = (ids[res_ndim - 2] + k > -1) ? (ids[res_ndim - 2] + k) : -1;
            int values = res_shape[res_ndim - 1];
            int diag_idx = (values < diag_idx_) ? values : diag_idx_;

            if (ids[res_ndim - 1] >= diag_idx)
            {
                result[i] = array_m[ids[res_ndim - 1]];
            }
            else
            {
                result[i] = 0;
            }
        }
    }
    else
    {
        for (size_t i = 0; i < res_size; ++i)
        {
            size_t n = res_size;
            size_t val = i;
            int ids[res_ndim];
            for (size_t j = 0; j < res_ndim; ++j)
            {
                n /= res_shape[j];
                size_t p = val / n;
                ids[j] = p;
                if (p != 0)
                {
                    val = val - p * n;
                }
            }

            int diag_idx_ = (ids[res_ndim - 2] + k > -1) ? (ids[res_ndim - 2] + k) : -1;
            int values = res_shape[res_ndim - 1];
            int diag_idx = (values < diag_idx_) ? values : diag_idx_;

            if (ids[res_ndim - 1] >= diag_idx)
            {
                result[i] = array_m[i];
            }
            else
            {
                result[i] = 0;
            }
        }
    }
    return;
}

template <typename _DataType>
void dpnp_zeros_c(void* result, size_t size)
{
    _DataType* fill_value = reinterpret_cast<_DataType*>(dpnp_memory_alloc_c(sizeof(_DataType)));
    fill_value[0] = 0;

    dpnp_initval_c<_DataType>(result, fill_value, size);

    dpnp_memory_free_c(fill_value);
}

template <typename _DataType>
void dpnp_zeros_like_c(void* result, size_t size)
{
    dpnp_zeros_c<_DataType>(result, size);
}

void func_map_init_arraycreation(func_map_t& fmap)
{
    fmap[DPNPFuncName::DPNP_FN_ARANGE][eft_INT][eft_INT] = {eft_INT, (void*)dpnp_arange_c<int>};
    fmap[DPNPFuncName::DPNP_FN_ARANGE][eft_LNG][eft_LNG] = {eft_LNG, (void*)dpnp_arange_c<long>};
    fmap[DPNPFuncName::DPNP_FN_ARANGE][eft_FLT][eft_FLT] = {eft_FLT, (void*)dpnp_arange_c<float>};
    fmap[DPNPFuncName::DPNP_FN_ARANGE][eft_DBL][eft_DBL] = {eft_DBL, (void*)dpnp_arange_c<double>};

    fmap[DPNPFuncName::DPNP_FN_DIAG][eft_INT][eft_INT] = {eft_INT, (void*)dpnp_diag_c<int>};
    fmap[DPNPFuncName::DPNP_FN_DIAG][eft_LNG][eft_LNG] = {eft_LNG, (void*)dpnp_diag_c<long>};
    fmap[DPNPFuncName::DPNP_FN_DIAG][eft_FLT][eft_FLT] = {eft_FLT, (void*)dpnp_diag_c<float>};
    fmap[DPNPFuncName::DPNP_FN_DIAG][eft_DBL][eft_DBL] = {eft_DBL, (void*)dpnp_diag_c<double>};

    fmap[DPNPFuncName::DPNP_FN_FULL][eft_INT][eft_INT] = {eft_INT, (void*)dpnp_full_c<int>};
    fmap[DPNPFuncName::DPNP_FN_FULL][eft_LNG][eft_LNG] = {eft_LNG, (void*)dpnp_full_c<long>};
    fmap[DPNPFuncName::DPNP_FN_FULL][eft_FLT][eft_FLT] = {eft_FLT, (void*)dpnp_full_c<float>};
    fmap[DPNPFuncName::DPNP_FN_FULL][eft_DBL][eft_DBL] = {eft_DBL, (void*)dpnp_full_c<double>};
    fmap[DPNPFuncName::DPNP_FN_FULL][eft_BLN][eft_BLN] = {eft_BLN, (void*)dpnp_full_c<bool>};
    fmap[DPNPFuncName::DPNP_FN_FULL][eft_C128][eft_C128] = {eft_C128, (void*)dpnp_full_c<std::complex<double>>};

    fmap[DPNPFuncName::DPNP_FN_FULL_LIKE][eft_INT][eft_INT] = {eft_INT, (void*)dpnp_full_like_c<int>};
    fmap[DPNPFuncName::DPNP_FN_FULL_LIKE][eft_LNG][eft_LNG] = {eft_LNG, (void*)dpnp_full_like_c<long>};
    fmap[DPNPFuncName::DPNP_FN_FULL_LIKE][eft_FLT][eft_FLT] = {eft_FLT, (void*)dpnp_full_like_c<float>};
    fmap[DPNPFuncName::DPNP_FN_FULL_LIKE][eft_DBL][eft_DBL] = {eft_DBL, (void*)dpnp_full_like_c<double>};
    fmap[DPNPFuncName::DPNP_FN_FULL_LIKE][eft_BLN][eft_BLN] = {eft_BLN, (void*)dpnp_full_like_c<bool>};
    fmap[DPNPFuncName::DPNP_FN_FULL_LIKE][eft_C128][eft_C128] = {eft_C128,
                                                                 (void*)dpnp_full_like_c<std::complex<double>>};

    fmap[DPNPFuncName::DPNP_FN_IDENTITY][eft_INT][eft_INT] = {eft_INT, (void*)dpnp_identity_c<int>};
    fmap[DPNPFuncName::DPNP_FN_IDENTITY][eft_LNG][eft_LNG] = {eft_LNG, (void*)dpnp_identity_c<long>};
    fmap[DPNPFuncName::DPNP_FN_IDENTITY][eft_FLT][eft_FLT] = {eft_FLT, (void*)dpnp_identity_c<float>};
    fmap[DPNPFuncName::DPNP_FN_IDENTITY][eft_DBL][eft_DBL] = {eft_DBL, (void*)dpnp_identity_c<double>};
    fmap[DPNPFuncName::DPNP_FN_IDENTITY][eft_BLN][eft_BLN] = {eft_BLN, (void*)dpnp_identity_c<bool>};
    fmap[DPNPFuncName::DPNP_FN_IDENTITY][eft_C128][eft_C128] = {eft_C128, (void*)dpnp_identity_c<std::complex<double>>};

    fmap[DPNPFuncName::DPNP_FN_ONES][eft_INT][eft_INT] = {eft_INT, (void*)dpnp_ones_c<int>};
    fmap[DPNPFuncName::DPNP_FN_ONES][eft_LNG][eft_LNG] = {eft_LNG, (void*)dpnp_ones_c<long>};
    fmap[DPNPFuncName::DPNP_FN_ONES][eft_FLT][eft_FLT] = {eft_FLT, (void*)dpnp_ones_c<float>};
    fmap[DPNPFuncName::DPNP_FN_ONES][eft_DBL][eft_DBL] = {eft_DBL, (void*)dpnp_ones_c<double>};
    fmap[DPNPFuncName::DPNP_FN_ONES][eft_BLN][eft_BLN] = {eft_BLN, (void*)dpnp_ones_c<bool>};
    fmap[DPNPFuncName::DPNP_FN_ONES][eft_C128][eft_C128] = {eft_C128, (void*)dpnp_ones_c<std::complex<double>>};

    fmap[DPNPFuncName::DPNP_FN_ONES_LIKE][eft_INT][eft_INT] = {eft_INT, (void*)dpnp_ones_like_c<int>};
    fmap[DPNPFuncName::DPNP_FN_ONES_LIKE][eft_LNG][eft_LNG] = {eft_LNG, (void*)dpnp_ones_like_c<long>};
    fmap[DPNPFuncName::DPNP_FN_ONES_LIKE][eft_FLT][eft_FLT] = {eft_FLT, (void*)dpnp_ones_like_c<float>};
    fmap[DPNPFuncName::DPNP_FN_ONES_LIKE][eft_DBL][eft_DBL] = {eft_DBL, (void*)dpnp_ones_like_c<double>};
    fmap[DPNPFuncName::DPNP_FN_ONES_LIKE][eft_BLN][eft_BLN] = {eft_BLN, (void*)dpnp_ones_like_c<bool>};
    fmap[DPNPFuncName::DPNP_FN_ONES_LIKE][eft_C128][eft_C128] = {eft_C128,
                                                                 (void*)dpnp_ones_like_c<std::complex<double>>};

    fmap[DPNPFuncName::DPNP_FN_VANDER][eft_INT][eft_INT] = {eft_LNG, (void*)dpnp_vander_c<int, long>};
    fmap[DPNPFuncName::DPNP_FN_VANDER][eft_LNG][eft_LNG] = {eft_LNG, (void*)dpnp_vander_c<long, long>};
    fmap[DPNPFuncName::DPNP_FN_VANDER][eft_FLT][eft_FLT] = {eft_DBL, (void*)dpnp_vander_c<float, double>};
    fmap[DPNPFuncName::DPNP_FN_VANDER][eft_DBL][eft_DBL] = {eft_DBL, (void*)dpnp_vander_c<double, double>};
    fmap[DPNPFuncName::DPNP_FN_VANDER][eft_BLN][eft_BLN] = {eft_LNG, (void*)dpnp_vander_c<bool, long>};
    fmap[DPNPFuncName::DPNP_FN_VANDER][eft_C128][eft_C128] = {
        eft_C128, (void*)dpnp_vander_c<std::complex<double>, std::complex<double>>};

    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_INT][eft_INT] = {eft_INT, (void*)dpnp_trace_c<int, int>};
    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_LNG][eft_INT] = {eft_INT, (void*)dpnp_trace_c<long, int>};
    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_FLT][eft_INT] = {eft_INT, (void*)dpnp_trace_c<float, int>};
    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_DBL][eft_INT] = {eft_INT, (void*)dpnp_trace_c<double, int>};
    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_INT][eft_LNG] = {eft_LNG, (void*)dpnp_trace_c<int, long>};
    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_LNG][eft_LNG] = {eft_LNG, (void*)dpnp_trace_c<long, long>};
    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_FLT][eft_LNG] = {eft_LNG, (void*)dpnp_trace_c<float, long>};
    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_DBL][eft_LNG] = {eft_LNG, (void*)dpnp_trace_c<double, long>};
    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_INT][eft_FLT] = {eft_FLT, (void*)dpnp_trace_c<int, float>};
    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_LNG][eft_FLT] = {eft_FLT, (void*)dpnp_trace_c<long, float>};
    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_FLT][eft_FLT] = {eft_FLT, (void*)dpnp_trace_c<float, float>};
    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_DBL][eft_FLT] = {eft_FLT, (void*)dpnp_trace_c<double, float>};
    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_INT][eft_DBL] = {eft_DBL, (void*)dpnp_trace_c<int, double>};
    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_LNG][eft_DBL] = {eft_DBL, (void*)dpnp_trace_c<long, double>};
    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_FLT][eft_DBL] = {eft_DBL, (void*)dpnp_trace_c<float, double>};
    fmap[DPNPFuncName::DPNP_FN_TRACE][eft_DBL][eft_DBL] = {eft_DBL, (void*)dpnp_trace_c<double, double>};

    fmap[DPNPFuncName::DPNP_FN_TRI][eft_INT][eft_INT] = {eft_INT, (void*)dpnp_tri_c<int>};
    fmap[DPNPFuncName::DPNP_FN_TRI][eft_LNG][eft_LNG] = {eft_LNG, (void*)dpnp_tri_c<long>};
    fmap[DPNPFuncName::DPNP_FN_TRI][eft_FLT][eft_FLT] = {eft_FLT, (void*)dpnp_tri_c<float>};
    fmap[DPNPFuncName::DPNP_FN_TRI][eft_DBL][eft_DBL] = {eft_DBL, (void*)dpnp_tri_c<double>};

    fmap[DPNPFuncName::DPNP_FN_TRIL][eft_INT][eft_INT] = {eft_INT, (void*)dpnp_tril_c<int>};
    fmap[DPNPFuncName::DPNP_FN_TRIL][eft_LNG][eft_LNG] = {eft_LNG, (void*)dpnp_tril_c<long>};
    fmap[DPNPFuncName::DPNP_FN_TRIL][eft_FLT][eft_FLT] = {eft_FLT, (void*)dpnp_tril_c<float>};
    fmap[DPNPFuncName::DPNP_FN_TRIL][eft_DBL][eft_DBL] = {eft_DBL, (void*)dpnp_tril_c<double>};

    fmap[DPNPFuncName::DPNP_FN_TRIU][eft_INT][eft_INT] = {eft_INT, (void*)dpnp_triu_c<int>};
    fmap[DPNPFuncName::DPNP_FN_TRIU][eft_LNG][eft_LNG] = {eft_LNG, (void*)dpnp_triu_c<long>};
    fmap[DPNPFuncName::DPNP_FN_TRIU][eft_FLT][eft_FLT] = {eft_FLT, (void*)dpnp_triu_c<float>};
    fmap[DPNPFuncName::DPNP_FN_TRIU][eft_DBL][eft_DBL] = {eft_DBL, (void*)dpnp_triu_c<double>};

    fmap[DPNPFuncName::DPNP_FN_ZEROS][eft_INT][eft_INT] = {eft_INT, (void*)dpnp_zeros_c<int>};
    fmap[DPNPFuncName::DPNP_FN_ZEROS][eft_LNG][eft_LNG] = {eft_LNG, (void*)dpnp_zeros_c<long>};
    fmap[DPNPFuncName::DPNP_FN_ZEROS][eft_FLT][eft_FLT] = {eft_FLT, (void*)dpnp_zeros_c<float>};
    fmap[DPNPFuncName::DPNP_FN_ZEROS][eft_DBL][eft_DBL] = {eft_DBL, (void*)dpnp_zeros_c<double>};
    fmap[DPNPFuncName::DPNP_FN_ZEROS][eft_BLN][eft_BLN] = {eft_BLN, (void*)dpnp_zeros_c<bool>};
    fmap[DPNPFuncName::DPNP_FN_ZEROS][eft_C128][eft_C128] = {eft_C128, (void*)dpnp_zeros_c<std::complex<double>>};

    fmap[DPNPFuncName::DPNP_FN_ZEROS_LIKE][eft_INT][eft_INT] = {eft_INT, (void*)dpnp_zeros_like_c<int>};
    fmap[DPNPFuncName::DPNP_FN_ZEROS_LIKE][eft_LNG][eft_LNG] = {eft_LNG, (void*)dpnp_zeros_like_c<long>};
    fmap[DPNPFuncName::DPNP_FN_ZEROS_LIKE][eft_FLT][eft_FLT] = {eft_FLT, (void*)dpnp_zeros_like_c<float>};
    fmap[DPNPFuncName::DPNP_FN_ZEROS_LIKE][eft_DBL][eft_DBL] = {eft_DBL, (void*)dpnp_zeros_like_c<double>};
    fmap[DPNPFuncName::DPNP_FN_ZEROS_LIKE][eft_BLN][eft_BLN] = {eft_BLN, (void*)dpnp_zeros_like_c<bool>};
    fmap[DPNPFuncName::DPNP_FN_ZEROS_LIKE][eft_C128][eft_C128] = {eft_C128,
                                                                  (void*)dpnp_zeros_like_c<std::complex<double>>};

    return;
}
