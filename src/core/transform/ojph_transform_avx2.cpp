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
// File: ojph_transform_avx2.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/

#include <cstdio>

#include "ojph_defs.h"
#include "ojph_arch.h"
#include "ojph_mem.h"
#include "ojph_transform.h"
#include "ojph_transform_local.h"

#include <immintrin.h>

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_vert_wvlt_fwd_predict(const line_buf* line_src1,
                                        const line_buf* line_src2,
                                        line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;

      for (ui32 i = (repeat + 7) >> 3; i > 0; --i, dst+=8, src1+=8, src2+=8)
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
    void avx2_rev_vert_wvlt_fwd_update(const line_buf* line_src1,
                                       const line_buf* line_src2,
                                       line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;

      __m256i offset = _mm256_set1_epi32(2);
      for (ui32 i = (repeat + 7) >> 3; i > 0; --i, dst+=8, src1+=8, src2+=8)
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
    void avx2_rev_horz_wvlt_fwd_tx(line_buf* line_src, line_buf *line_ldst,
                                   line_buf *line_hdst,ui32 width, bool even)
    {
      if (width > 1)
      {
        si32 *src = line_src->i32;
        si32 *ldst = line_ldst->i32, *hdst = line_hdst->i32;

        const ui32 L_width = (width + (even ? 1 : 0)) >> 1;
        const ui32 H_width = (width + (even ? 0 : 1)) >> 1;

        // extension
        src[-1] = src[1];
        src[width] = src[width-2];
        // predict
        const si32* sp = src + (even ? 1 : 0);
        si32 *dph = hdst;
        const __m256i mask = _mm256_setr_epi32(0, 2, 4, 6, 1, 3, 5, 7);
        for (ui32 i = (H_width + 7) >> 3; i > 0; --i, dph+=8)
        { //this is doing twice the work it needs to do
          //it can be definitely written better
          __m256i s1 = _mm256_loadu_si256((__m256i*)(sp-1));
          __m256i s2 = _mm256_loadu_si256((__m256i*)(sp+1));
          __m256i d = _mm256_loadu_si256((__m256i*)sp);
          s1 = _mm256_srai_epi32(_mm256_add_epi32(s1, s2), 1);
          __m256i d1 = _mm256_sub_epi32(d, s1);
          sp += 8;
          s1 = _mm256_loadu_si256((__m256i*)(sp-1));
          s2 = _mm256_loadu_si256((__m256i*)(sp+1));
          d = _mm256_loadu_si256((__m256i*)sp);
          s1 = _mm256_srai_epi32(_mm256_add_epi32(s1, s2), 1);
          __m256i d2 = _mm256_sub_epi32(d, s1);
          sp += 8;
          d1 = _mm256_permutevar8x32_epi32(d1, mask);
          d2 = _mm256_permutevar8x32_epi32(d2, mask);
          d = _mm256_permute2x128_si256(d1, d2, (2 << 4) | 0);
          _mm256_store_si256((__m256i*)dph, d);
        }

        // extension
        hdst[-1] = hdst[0];
        hdst[H_width] = hdst[H_width-1];
        // update
        sp = src + (even ? 0 : 1);
        const si32* sph = hdst + (even ? 0 : 1);
        si32 *dpl = ldst;
        __m256i offset = _mm256_set1_epi32(2);
        for (ui32 i = (L_width + 7) >> 3; i > 0; --i, sp+=16, sph+=8, dpl+=8)
        {
          __m256i s1 = _mm256_loadu_si256((__m256i*)(sph-1));
          s1 = _mm256_add_epi32(s1, offset);
          __m256i s2 = _mm256_loadu_si256((__m256i*)sph);
          s2 = _mm256_add_epi32(s2, s1);
          __m256i d1 = _mm256_loadu_si256((__m256i*)sp);
          __m256i d2 = _mm256_loadu_si256((__m256i*)sp + 1);
          d1 = _mm256_permutevar8x32_epi32(d1, mask);
          d2 = _mm256_permutevar8x32_epi32(d2, mask);
          __m256i d = _mm256_permute2x128_si256(d1, d2, (2 << 4) | 0);
          d = _mm256_add_epi32(d, _mm256_srai_epi32(s2, 2));
          _mm256_store_si256((__m256i*)dpl, d);
        }
      }
      else
      {
        if (even)
          line_ldst->i32[0] = line_src->i32[0];
        else
          line_hdst->i32[0] = line_src->i32[0] << 1;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_vert_wvlt_bwd_predict(const line_buf* line_src1,
                                        const line_buf* line_src2,
                                        line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;
    
      for (ui32 i = (repeat + 7) >> 3; i > 0; --i, dst+=8, src1+=8, src2+=8)
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
    void avx2_rev_vert_wvlt_bwd_update(const line_buf* line_src1,
                                       const line_buf* line_src2,
                                       line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;
    
      __m256i offset = _mm256_set1_epi32(2);
      for (ui32 i = (repeat + 7) >> 3; i > 0; --i, dst+=8, src1+=8, src2+=8)
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
    void avx2_rev_horz_wvlt_bwd_tx(line_buf* line_dst, line_buf *line_lsrc,
                                   line_buf *line_hsrc, ui32 width, bool even)
    {
      if (width > 1)
      {
        si32 *lsrc = line_lsrc->i32, *hsrc = line_hsrc->i32;
        si32 *dst = line_dst->i32;

        const ui32 L_width = (width + (even ? 1 : 0)) >> 1;
        const ui32 H_width = (width + (even ? 0 : 1)) >> 1;

        // extension
        hsrc[-1] = hsrc[0];
        hsrc[H_width] = hsrc[H_width-1];
        //inverse update
        const si32 *sph = hsrc + (even ? 0 : 1);
        si32 *spl = lsrc;
        __m256i offset = _mm256_set1_epi32(2);
        for (ui32 i = (L_width + 7) >> 3; i > 0; --i, sph+=8, spl+=8)
        {
          __m256i s1 = _mm256_loadu_si256((__m256i*)(sph-1));
          s1 = _mm256_add_epi32(s1, offset);
          __m256i s2 = _mm256_loadu_si256((__m256i*)sph);
          s2 = _mm256_add_epi32(s2, s1);
          __m256i d = _mm256_load_si256((__m256i*)spl);
          d = _mm256_sub_epi32(d, _mm256_srai_epi32(s2, 2));
          _mm256_store_si256((__m256i*)spl, d);
        }

        // extension
        lsrc[-1] = lsrc[0];
        lsrc[L_width] = lsrc[L_width - 1];
        // inverse predict and combine
        si32 *dp = dst + (even ? 0 : -1);
        spl = lsrc + (even ? 0 : -1);
        sph = hsrc;
        ui32 width = L_width + (even ? 0 : 1);
        for (ui32 i = (width + 7) >> 3; i > 0; --i, sph+=8, spl+=8, dp+=16)
        {
          __m256i s1 = _mm256_loadu_si256((__m256i*)spl);
          __m256i s2 = _mm256_loadu_si256((__m256i*)(spl+1));
          __m256i d = _mm256_load_si256((__m256i*)sph);
          s2 = _mm256_srai_epi32(_mm256_add_epi32(s1, s2), 1);
          d = _mm256_add_epi32(d, s2);
          s2 = _mm256_unpackhi_epi32(s1, d);
          s1 = _mm256_unpacklo_epi32(s1, d);
          d = _mm256_permute2x128_si256(s1, s2, (2 << 4) | 0);
          _mm256_storeu_si256((__m256i*)dp, d);
          d = _mm256_permute2x128_si256(s1, s2, (3 << 4) | 1);
          _mm256_storeu_si256((__m256i*)dp + 1, d);
        }
      }
      else
      {
        if (even)
          line_dst->i32[0] = line_lsrc->i32[0];
        else
          line_dst->i32[0] = line_hsrc->i32[0] >> 1;
      }
    }
  }
}
