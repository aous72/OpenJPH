//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2022, Aous Naman 
// Copyright (c) 2022, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2022, The University of New South Wales, Australia
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
// File: ojph_codestream_avx2.cpp
// Author: Aous Naman
// Date: 15 May 2022
//***************************************************************************/

#include <immintrin.h>
#include "ojph_defs.h"

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    ui32 avx2_find_max_val(ui32* address)
    {
      __m128i x0 = _mm_loadu_si128((__m128i*)address);
      __m128i x1 = _mm_loadu_si128((__m128i*)address + 1);
      x0 = _mm_or_si128(x0, x1);
      x1 = _mm_shuffle_epi32(x0, 0xEE);   // x1 = x0[2,3,2,3]
      x0 = _mm_or_si128(x0, x1);
      x1 = _mm_shuffle_epi32(x0, 0x55);   // x1 = x0[1,1,1,1]
      x0 = _mm_or_si128(x0, x1);
      ui32 t = (ui32)_mm_extract_epi32(x0, 0);
      return t;
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max, 
                           float delta_inv, ui32 count, ui32* max_val)
    {
      ojph_unused(delta_inv);

      // convert to sign and magnitude and keep max_val      
      ui32 shift = 31 - K_max;
      __m256i m0 = _mm256_set1_epi32((int)0x80000000);
      __m256i tmax = _mm256_loadu_si256((__m256i*)max_val);
      __m256i *p = (__m256i*)sp;
      for (ui32 i = 0; i < count; i += 8, p += 1, dp += 8)
      {
        __m256i v = _mm256_loadu_si256(p);
        __m256i sign = _mm256_and_si256(v, m0);
        __m256i val = _mm256_abs_epi32(v);
        val = _mm256_slli_epi32(val, (int)shift);
        tmax = _mm256_or_si256(tmax, val);
        val = _mm256_or_si256(val, sign);
        _mm256_storeu_si256((__m256i*)dp, val);
      }
      _mm256_storeu_si256((__m256i*)max_val, tmax);
    }
                           
    //////////////////////////////////////////////////////////////////////////
    void avx2_irv_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                           float delta_inv, ui32 count, ui32* max_val)
    {
      ojph_unused(K_max);

      //quantize and convert to sign and magnitude and keep max_val
      __m256 d = _mm256_set1_ps(delta_inv);
      __m256i m0 = _mm256_set1_epi32((int)0x80000000);
      __m256i tmax = _mm256_loadu_si256((__m256i*)max_val);
      float *p = (float*)sp;
      
      for (ui32 i = 0; i < count; i += 8, p += 8, dp += 8)
      {
        __m256 vf = _mm256_loadu_ps(p);
        vf = _mm256_mul_ps(vf, d);                // multiply
        __m256i val = _mm256_cvtps_epi32(vf);     // convert to int
        __m256i sign = _mm256_and_si256(val, m0); // get sign
        val = _mm256_abs_epi32(val);
        tmax = _mm256_or_si256(tmax, val);
        val = _mm256_or_si256(val, sign);
        _mm256_storeu_si256((__m256i*)dp, val);
      }
      _mm256_storeu_si256((__m256i*)max_val, tmax);
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max, 
                             float delta, ui32 count)
    {
      ojph_unused(delta);
      ui32 shift = 31 - K_max;
      __m256i m1 = _mm256_set1_epi32(0x7FFFFFFF);
      si32 *p = (si32*)dp;
      for (ui32 i = 0; i < count; i += 8, sp += 8, p += 8)
      {
          __m256i v = _mm256_load_si256((__m256i*)sp);
          __m256i val = _mm256_and_si256(v, m1);
          val = _mm256_srli_epi32(val, (int)shift);
          val = _mm256_sign_epi32(val, v);
          _mm256_storeu_si256((__m256i*)p, val);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_irv_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max, 
                             float delta, ui32 count)
    {
      ojph_unused(K_max);
      __m256i m1 = _mm256_set1_epi32(0x7FFFFFFF);
      __m256 d = _mm256_set1_ps(delta);
      float *p = (float*)dp;
      for (ui32 i = 0; i < count; i += 8, sp += 8, p += 8)
      {
        __m256i v = _mm256_load_si256((__m256i*)sp);
        __m256i vali = _mm256_and_si256(v, m1);
        __m256  valf = _mm256_cvtepi32_ps(vali);
        valf = _mm256_mul_ps(valf, d);
        __m256i sign = _mm256_andnot_si256(m1, v);
        valf = _mm256_or_ps(valf, _mm256_castsi256_ps(sign));
        _mm256_storeu_ps(p, valf);
      }
    }
  }
}