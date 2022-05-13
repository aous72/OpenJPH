//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2019, Aous Naman 
// Copyright (c) 2019, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2019, The University of New South Wales, Australia
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//***************************************************************************/
// This file is part of the OpenJPH software implementation.
// File: ojph_colour_sse.cpp
// Author: Aous Naman
// Date: 11 October 2019
//***************************************************************************/

#include <cmath>

#include "ojph_defs.h"
#include "ojph_arch.h"
#include "ojph_colour.h"
#include "ojph_colour_local.h"

#include <immintrin.h>

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    void sse_cnvrt_si32_to_float_shftd(const si32 *sp, float *dp, float mul,
                                       ui32 width)
    {
      __m128 shift = _mm_set1_ps(0.5f);
      __m128 m = _mm_set1_ps(mul);
      for (ui32 i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        __m128i t = _mm_castps_si128(_mm_loadu_ps((float*)sp));
        __m128 s = _mm_cvtepi32_ps(t);
        s = _mm_mul_ps(s, m);
        s = _mm_sub_ps(s, shift);
        _mm_store_ps(dp, s);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse_cnvrt_si32_to_float(const si32 *sp, float *dp, float mul,
                                 ui32 width)
    {
      __m128 m = _mm_set1_ps(mul);
      for (ui32 i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        __m128i t = _mm_castps_si128(_mm_loadu_ps((float*)sp));
        __m128 s = _mm_cvtepi32_ps(t);
        s = _mm_mul_ps(s, m);
        _mm_store_ps(dp, s);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse_cnvrt_float_to_si32_shftd(const float *sp, si32 *dp, float mul,
                                       ui32 width)
    {
      uint32_t rounding_mode = _MM_GET_ROUNDING_MODE();
      _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
      __m128 shift = _mm_set1_ps(0.5f);
      __m128 m = _mm_set1_ps(mul);
      for (ui32 i = (width + 3) >> 2; i > 0; --i, sp+=4)
      {
        __m128 t = _mm_load_ps(sp);
        __m128 s = _mm_add_ps(t, shift);
        s = _mm_mul_ps(s, m);
        // the following is a poorly designed code, but it is the only
        // code that I am aware of that compiles on VS 32 and 64 modes
        t = s;
        *dp++ = _mm_cvtss_si32(t); 
        t = _mm_shuffle_ps(s, s, 1);
        *dp++ = _mm_cvtss_si32(t); 
        t = _mm_shuffle_ps(s, s, 2);
        *dp++ = _mm_cvtss_si32(t); 
        t = _mm_shuffle_ps(s, s, 3);
        *dp++ = _mm_cvtss_si32(t);
      }
      _MM_SET_ROUNDING_MODE(rounding_mode);
    }

    //////////////////////////////////////////////////////////////////////////
    void sse_cnvrt_float_to_si32(const float *sp, si32 *dp, float mul,
                                 ui32 width)
    {
      uint32_t rounding_mode = _MM_GET_ROUNDING_MODE();
      _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
      __m128 m = _mm_set1_ps(mul);
      for (ui32 i = (width + 3) >> 2; i > 0; --i, sp+=4)
      {
        __m128 t = _mm_load_ps(sp);
        __m128 s = _mm_mul_ps(t, m);
        // the following is a poorly designed code, but it is the only
        // code that I am aware of that compiles on VS 32 and 64 modes
        t = s;
        *dp++ = _mm_cvtss_si32(t);
        t = _mm_shuffle_ps(s, s, 1);
        *dp++ = _mm_cvtss_si32(t);
        t = _mm_shuffle_ps(s, s, 2);
        *dp++ = _mm_cvtss_si32(t);
        t = _mm_shuffle_ps(s, s, 3);
        *dp++ = _mm_cvtss_si32(t);
      }
      _MM_SET_ROUNDING_MODE(rounding_mode);
    }

    //////////////////////////////////////////////////////////////////////////
    void sse_ict_forward(const float *r, const float *g, const float *b,
                         float *y, float *cb, float *cr, ui32 repeat)
    {
      __m128 alpha_rf = _mm_set1_ps(CT_CNST::ALPHA_RF);
      __m128 alpha_gf = _mm_set1_ps(CT_CNST::ALPHA_GF);
      __m128 alpha_bf = _mm_set1_ps(CT_CNST::ALPHA_BF);
      __m128 beta_cbf = _mm_set1_ps(CT_CNST::BETA_CbF);
      __m128 beta_crf = _mm_set1_ps(CT_CNST::BETA_CrF);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i)
      {
        __m128 mr = _mm_load_ps(r);
        __m128 mb = _mm_load_ps(b);
        __m128 my = _mm_mul_ps(alpha_rf, mr);
        my = _mm_add_ps(my, _mm_mul_ps(alpha_gf, _mm_load_ps(g)));
        my = _mm_add_ps(my, _mm_mul_ps(alpha_bf, mb));
        _mm_store_ps(y, my);
        _mm_store_ps(cb, _mm_mul_ps(beta_cbf, _mm_sub_ps(mb, my)));
        _mm_store_ps(cr, _mm_mul_ps(beta_crf, _mm_sub_ps(mr, my)));
        
        r += 4; g += 4; b += 4;
        y += 4; cb += 4; cr += 4;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse_ict_backward(const float *y, const float *cb, const float *cr,
                          float *r, float *g, float *b, ui32 repeat)
    {
      __m128 gamma_cr2g = _mm_set1_ps(CT_CNST::GAMMA_CR2G);
      __m128 gamma_cb2g = _mm_set1_ps(CT_CNST::GAMMA_CB2G);
      __m128 gamma_cr2r = _mm_set1_ps(CT_CNST::GAMMA_CR2R);
      __m128 gamma_cb2b = _mm_set1_ps(CT_CNST::GAMMA_CB2B);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i)
      {
        __m128 my = _mm_load_ps(y);
        __m128 mcr = _mm_load_ps(cr);
        __m128 mcb = _mm_load_ps(cb);
        __m128 mg = _mm_sub_ps(my, _mm_mul_ps(gamma_cr2g, mcr));
        _mm_store_ps(g, _mm_sub_ps(mg, _mm_mul_ps(gamma_cb2g, mcb)));
        _mm_store_ps(r, _mm_add_ps(my, _mm_mul_ps(gamma_cr2r, mcr)));
        _mm_store_ps(b, _mm_add_ps(my, _mm_mul_ps(gamma_cb2b, mcb)));

        y += 4; cb += 4; cr += 4;
        r += 4; g += 4; b += 4;
      }
    }
  }
}
