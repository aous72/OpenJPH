/****************************************************************************/
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
/****************************************************************************/
// This file is part of the OpenJPH software implementation.
// File: ojph_transform_avx2.cpp
// Author: Aous Naman
// Date: 28 August 2019
/****************************************************************************/

#include <cstdio>

#include "ojph_transform.h"
#include "ojph_transform_local.h"

#ifdef OJPH_COMPILER_MSVC
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_vert_wvlt_fwd_predict(const si32* src1, const si32* src2,
                                        si32 *dst, int repeat)
    {
      for (int i = (repeat + 7) >> 3; i > 0; --i, dst+=8, src1+=8, src2+=8)
      {
        __m256i s1 = _mm256_load_si256((__m256i*)src1);
        __m256i s2 = _mm256_load_si256((__m256i*)src2);
        __m256i d = _mm256_load_si256((__m256i*)dst);
        s1 = _mm256_srai_epi32(_mm256_add_epi32(s1, s2), 1);
        d = _mm256_sub_epi32(d, s1);
        _mm256_store_si256((__m256i*)dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_vert_wvlt_fwd_update(const si32* src1, const si32* src2,
                                       si32 *dst, int repeat)
    {
      __m256i offset = _mm256_set1_epi32(2);
      for (int i = (repeat + 7) >> 3; i > 0; --i, dst+=8, src1+=8, src2+=8)
      {
        __m256i s1 = _mm256_load_si256((__m256i*)src1);
        s1 = _mm256_add_epi32(s1, offset);
        __m256i s2 = _mm256_load_si256((__m256i*)src2);
        s2 = _mm256_add_epi32(s2, s1);
        __m256i d = _mm256_load_si256((__m256i*)dst);
        d = _mm256_add_epi32(d, _mm256_srai_epi32(s2, 2));
        _mm256_store_si256((__m256i*)dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_horz_wvlt_fwd_tx(si32* src, si32 *ldst, si32 *hdst,
                                   int width, bool even)
    {
      if (width > 1)
      {
        const int L_width = (width + (even ? 1 : 0)) >> 1;
        const int H_width = (width + (even ? 0 : 1)) >> 1;

        // extension
        src[-1] = src[1];
        src[width] = src[width-2];
        // predict
        const si32* sp = src + (even ? 1 : 0);
        si32 *dph = hdst;
        for (int i = H_width; i > 0; --i, sp+=2)
          *dph++ = sp[0] - ((sp[-1] + sp[1]) >> 1);

        // extension
        hdst[-1] = hdst[0];
        hdst[H_width] = hdst[H_width-1];
        // update
        sp = src + (even ? 0 : 1);
        const si32* sph = hdst + (even ? 0 : 1);
        si32 *dpl = ldst;
        for (int i = L_width; i > 0; --i, sp+=2, sph++)
          *dpl++ = *sp + ((2 + sph[-1] + sph[0]) >> 2);
      }
      else
      {
        if (even)
          ldst[0] = src[0];
        else
          hdst[0] = src[0] << 1;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_vert_wvlt_bwd_predict(const si32* src1, const si32* src2,
                                        si32 *dst, int repeat)
    {
      for (int i = (repeat + 7) >> 3; i > 0; --i, dst+=8, src1+=8, src2+=8)
      {
        __m256i s1 = _mm256_load_si256((__m256i*)src1);
        __m256i s2 = _mm256_load_si256((__m256i*)src2);
        __m256i d = _mm256_load_si256((__m256i*)dst);
        s1 = _mm256_srai_epi32(_mm256_add_epi32(s1, s2), 1);
        d = _mm256_add_epi32(d, s1);
        _mm256_store_si256((__m256i*)dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_vert_wvlt_bwd_update(const si32* src1, const si32* src2,
                                       si32 *dst, int repeat)
    {
      __m256i offset = _mm256_set1_epi32(2);
      for (int i = (repeat + 7) >> 3; i > 0; --i, dst+=8, src1+=8, src2+=8)
      {
        __m256i s1 = _mm256_load_si256((__m256i*)src1);
        s1 = _mm256_add_epi32(s1, offset);
        __m256i s2 = _mm256_load_si256((__m256i*)src2);
        s2 = _mm256_add_epi32(s2, s1);
        __m256i d = _mm256_load_si256((__m256i*)dst);
        d = _mm256_sub_epi32(d, _mm256_srai_epi32(s2, 2));
        _mm256_store_si256((__m256i*)dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_horz_wvlt_bwd_tx(si32* dst, si32 *lsrc, si32 *hsrc,
                                   int width, bool even)
    {
      if (width > 1)
      {
        const int L_width = (width + (even ? 1 : 0)) >> 1;
        const int H_width = (width + (even ? 0 : 1)) >> 1;

        // extension
        hsrc[-1] = hsrc[0];
        hsrc[H_width] = hsrc[H_width-1];
        //inverse update
        const si32 *sph = hsrc + (even ? 0 : 1);
        si32 *spl = lsrc;
        for (int i = L_width; i > 0; --i, sph++, spl++)
          *spl -= ((2 + sph[-1] + sph[0]) >> 2);

        // extension
        lsrc[-1] = lsrc[0];
        lsrc[L_width] = lsrc[L_width - 1];
        // inverse predict and combine
        si32 *dp = dst + (even ? 0 : -1);
        spl = lsrc + (even ? 0 : -1);
        sph = hsrc;
        for (int i = L_width + (even ? 0 : 1); i > 0; --i, spl++, sph++)
        {
          *dp++ = *spl;
          *dp++ = *sph + ((spl[0] + spl[1]) >> 1);
        }
      }
      else
      {
        if (even)
          dst[0] = lsrc[0];
        else
          dst[0] = hsrc[0] >> 1;
      }
    }
  }
}
