# cython: language_level=3
# -*- coding: utf-8 -*-
# *****************************************************************************
# Copyright (c) 2016-2020, Intel Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
# *****************************************************************************

"""Module Backend (FFT part)

This module contains interface functions between C backend layer
and the rest of the library

"""


import dpnp
from dpnp.dpnp_algo cimport *
from dpnp.dpnp_utils cimport *


__all__ = [
    "dpnp_fft"
]

ctypedef void(*fptr_dpnp_fft_fft_t)(void *, void * , long * , long * , size_t, long, long, size_t)


cpdef dparray dpnp_fft(dparray input, size_t input_boundarie, size_t output_boundarie, long axis, size_t inverse):

    cdef dparray_shape_type input_shape = input.shape
    cdef dparray_shape_type output_shape = input_shape

    cdef long axis_norm = normalize_axis((axis,), input_shape.size())[0]
    output_shape[axis_norm] = output_boundarie

    # convert string type names (dparray.dtype) to C enum DPNPFuncType
    cdef DPNPFuncType param1_type = dpnp_dtype_to_DPNPFuncType(input.dtype)

    # get the FPTR data structure
    cdef DPNPFuncData kernel_data = get_dpnp_function_ptr(DPNP_FN_FFT_FFT, param1_type, param1_type)

    result_type = dpnp_DPNPFuncType_to_dtype( < size_t > kernel_data.return_type)
    # ceate result array with type given by FPTR data
    cdef dparray result = dparray(output_shape, dtype=result_type)

    cdef fptr_dpnp_fft_fft_t func = <fptr_dpnp_fft_fft_t > kernel_data.ptr
    # call FPTR function
    func(input.get_data(), result.get_data(), input_shape.data(),
         output_shape.data(), input_shape.size(), axis_norm, input_boundarie, inverse)

    return result
