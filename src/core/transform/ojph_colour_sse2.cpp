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
// File: ojph_colour_sse2.cpp
// Author: Aous Naman
// Date: 11 October 2019
//***************************************************************************/

#include <cmath>

#include "ojph_defs.h"
#include "ojph_arch.h"
#include "ojph_colour.h"

#include <immintrin.h>

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    void sse2_cnvrt_float_to_si32_shftd(const float *sp, si32 *dp, float mul,
                                       ui32 width)
    {
      uint32_t rounding_mode = _MM_GET_ROUNDING_MODE();
      _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
      __m128 shift = _mm_set1_ps(0.5f);
      __m128 m = _mm_set1_ps(mul);
      for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        __m128 t = _mm_loadu_ps(sp);
        __m128 s = _mm_add_ps(t, shift);
        s = _mm_mul_ps(s, m);
        _mm_storeu_si128((__m128i*)dp, _mm_cvtps_epi32(s));
      }
      _MM_SET_ROUNDING_MODE(rounding_mode);
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_cnvrt_float_to_si32(const float *sp, si32 *dp, float mul,
                                 ui32 width)
    {
      uint32_t rounding_mode = _MM_GET_ROUNDING_MODE();
      _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
      __m128 m = _mm_set1_ps(mul);
      for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        __m128 t = _mm_loadu_ps(sp);
        __m128 s = _mm_mul_ps(t, m);
        _mm_storeu_si128((__m128i*)dp, _mm_cvtps_epi32(s));
      }
      _MM_SET_ROUNDING_MODE(rounding_mode);
    }


    //////////////////////////////////////////////////////////////////////////
    void sse2_cnvrt_si32_to_si32_shftd(const si32 *sp, si32 *dp, int shift,
                                       ui32 width)
    {
      __m128i sh = _mm_set1_epi32(shift);
      for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        __m128i s = _mm_loadu_si128((__m128i*)sp);
        s = _mm_add_epi32(s, sh);
        _mm_storeu_si128((__m128i*)dp, s);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_rct_forward(const si32 *r, const si32 *g, const si32 *b,
                          si32 *y, si32 *cb, si32 *cr, ui32 repeat)
    {
      for (int i = (repeat + 3) >> 2; i > 0; --i)
      {
        __m128i mr = _mm_load_si128((__m128i*)r);
        __m128i mg = _mm_load_si128((__m128i*)g);
        __m128i mb = _mm_load_si128((__m128i*)b);
        __m128i t = _mm_add_epi32(mr, mb);
        t = _mm_add_epi32(t, _mm_slli_epi32(mg, 1));
        _mm_store_si128((__m128i*)y, _mm_srai_epi32(t, 2));
        t = _mm_sub_epi32(mb, mg);
        _mm_store_si128((__m128i*)cb, t);
        t = _mm_sub_epi32(mr, mg);
        _mm_store_si128((__m128i*)cr, t);

        r += 4; g += 4; b += 4;
        y += 4; cb += 4; cr += 4;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_rct_backward(const si32 *y, const si32 *cb, const si32 *cr,
                           si32 *r, si32 *g, si32 *b, ui32 repeat)
    {
      for (int i = (repeat + 3) >> 2; i > 0; --i)
      {
        __m128i my  = _mm_load_si128((__m128i*)y);
        __m128i mcb = _mm_load_si128((__m128i*)cb);
        __m128i mcr = _mm_load_si128((__m128i*)cr);

        __m128i t = _mm_add_epi32(mcb, mcr);
        t = _mm_sub_epi32(my, _mm_srai_epi32(t, 2));
        _mm_store_si128((__m128i*)g, t);
        __m128i u = _mm_add_epi32(mcb, t);
        _mm_store_si128((__m128i*)b, u);
        u = _mm_add_epi32(mcr, t);
        _mm_store_si128((__m128i*)r, u);

        y += 4; cb += 4; cr += 4;
        r += 4; g += 4; b += 4;
      }
    }

  }
}
