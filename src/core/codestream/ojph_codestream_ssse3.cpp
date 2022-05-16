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
// File: ojph_codestream_ssse3.cpp
// Author: Aous Naman
// Date: 15 May 2022
//***************************************************************************/

#include <immintrin.h>
#include "ojph_defs.h"

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    void ssse3_rev_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max, 
                            float delta_inv, ui32 count, ui32* max_val)
    {
      ojph_unused(delta_inv);

      // convert to sign and magnitude and keep max_val      
      ui32 shift = 31 - K_max;
      __m128i m0 = _mm_set1_epi32((int)0x80000000);
      __m128i tmax = _mm_loadu_si128((__m128i*)max_val);
      __m128i *p = (__m128i*)sp;
      for (ui32 i = 0; i < count; i += 4, p += 1, dp += 4)
      {
        __m128i v = _mm_loadu_si128(p);
        __m128i sign = _mm_and_si128(v, m0);
        __m128i val = _mm_sign_epi32(v, v);
        val = _mm_slli_epi32(val, (int)shift);
        tmax = _mm_or_si128(tmax, val);
        val = _mm_or_si128(val, sign);
        _mm_storeu_si128((__m128i*)dp, val);
      }
      _mm_storeu_si128((__m128i*)max_val, tmax);
    }

    //////////////////////////////////////////////////////////////////////////
    void ssse3_rev_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max, 
                              float delta, ui32 count)
    {
      ojph_unused(delta);
      ui32 shift = 31 - K_max;
      __m128i m1 = _mm_set1_epi32(0x7FFFFFFF);
      si32 *p = (si32*)dp;
      for (ui32 i = 0; i < count; i += 4, sp += 4, p += 4)
      {
          __m128i v = _mm_load_si128((__m128i*)sp);
          __m128i val = _mm_and_si128(v, m1);
          val = _mm_srli_epi32(val, (int)shift);
          val = _mm_sign_epi32(val, v);
          _mm_storeu_si128((__m128i*)p, val);
      }
    }
  }
}