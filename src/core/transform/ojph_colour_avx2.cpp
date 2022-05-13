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
// File: ojph_colour_avx2.cpp
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
    void avx2_cnvrt_si32_to_si32_shftd(const si32 *sp, si32 *dp, int shift,
                                       ui32 width)
    {
      __m256i sh = _mm256_set1_epi32(shift);
      for (int i = (width + 7) >> 3; i > 0; --i, sp+=8, dp+=8)
      {
        __m256i s = _mm256_loadu_si256((__m256i*)sp);
        s = _mm256_add_epi32(s, sh);
        _mm256_storeu_si256((__m256i*)dp, s);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_rct_forward(const si32 *r, const si32 *g, const si32 *b,
                          si32 *y, si32 *cb, si32 *cr, ui32 repeat)
    {
      for (int i = (repeat + 7) >> 3; i > 0; --i)
      {
        __m256i mr = _mm256_load_si256((__m256i*)r);
        __m256i mg = _mm256_load_si256((__m256i*)g);
        __m256i mb = _mm256_load_si256((__m256i*)b);
        __m256i t = _mm256_add_epi32(mr, mb);
        t = _mm256_add_epi32(t, _mm256_slli_epi32(mg, 1));
        _mm256_store_si256((__m256i*)y, _mm256_srai_epi32(t, 2));
        t = _mm256_sub_epi32(mb, mg);
        _mm256_store_si256((__m256i*)cb, t);
        t = _mm256_sub_epi32(mr, mg);
        _mm256_store_si256((__m256i*)cr, t);

        r += 8; g += 8; b += 8;
        y += 8; cb += 8; cr += 8;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_rct_backward(const si32 *y, const si32 *cb, const si32 *cr,
                           si32 *r, si32 *g, si32 *b, ui32 repeat)
    {
      for (int i = (repeat + 7) >> 3; i > 0; --i)
      {
        __m256i my  = _mm256_load_si256((__m256i*)y);
        __m256i mcb = _mm256_load_si256((__m256i*)cb);
        __m256i mcr = _mm256_load_si256((__m256i*)cr);

        __m256i t = _mm256_add_epi32(mcb, mcr);
        t = _mm256_sub_epi32(my, _mm256_srai_epi32(t, 2));
        _mm256_store_si256((__m256i*)g, t);
        __m256i u = _mm256_add_epi32(mcb, t);
        _mm256_store_si256((__m256i*)b, u);
        u = _mm256_add_epi32(mcr, t);
        _mm256_store_si256((__m256i*)r, u);

        y += 8; cb += 8; cr += 8;
        r += 8; g += 8; b += 8;
      }
    }

  }
}
