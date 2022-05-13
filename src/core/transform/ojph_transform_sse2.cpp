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
// File: ojph_transform_sse2.cpp
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
    void sse2_rev_vert_wvlt_fwd_predict(const line_buf* line_src1,
                                        const line_buf* line_src2,
                                        line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;

      for (ui32 i = (repeat + 3) >> 2; i > 0; --i, dst+=4, src1+=4, src2+=4)
      {
        __m128i s1 = _mm_load_si128((__m128i*)src1);
        __m128i s2 = _mm_load_si128((__m128i*)src2);
        __m128i d = _mm_load_si128((__m128i*)dst);
        s1 = _mm_srai_epi32(_mm_add_epi32(s1, s2), 1);
        d = _mm_sub_epi32(d, s1);
        _mm_store_si128((__m128i*)dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_vert_wvlt_fwd_update(const line_buf* line_src1,
                                       const line_buf* line_src2,
                                       line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;
    
      __m128i offset = _mm_set1_epi32(2);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i, dst+=4, src1+=4, src2+=4)
      {
        __m128i s1 = _mm_load_si128((__m128i*)src1);
        s1 = _mm_add_epi32(s1, offset);
        __m128i s2 = _mm_load_si128((__m128i*)src2);
        s2 = _mm_add_epi32(s2, s1);
        __m128i d = _mm_load_si128((__m128i*)dst);
        d = _mm_add_epi32(d, _mm_srai_epi32(s2, 2));
        _mm_store_si128((__m128i*)dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_horz_wvlt_fwd_tx(line_buf *line_src, line_buf *line_ldst,
                                   line_buf *line_hdst, ui32 width, bool even)
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
        for (ui32 i = (H_width + 3) >> 2; i > 0; --i, dph+=4)
        { //this is doing twice the work it needs to do
          //it can be definitely written better
          __m128i s1 = _mm_loadu_si128((__m128i*)(sp-1));
          __m128i s2 = _mm_loadu_si128((__m128i*)(sp+1));
          __m128i d = _mm_loadu_si128((__m128i*)sp);
          s1 = _mm_srai_epi32(_mm_add_epi32(s1, s2), 1);
          __m128i d1 = _mm_sub_epi32(d, s1);
          sp += 4;
          s1 = _mm_loadu_si128((__m128i*)(sp-1));
          s2 = _mm_loadu_si128((__m128i*)(sp+1));
          d = _mm_loadu_si128((__m128i*)sp);
          s1 = _mm_srai_epi32(_mm_add_epi32(s1, s2), 1);
          __m128i d2 = _mm_sub_epi32(d, s1);
          sp += 4;
          d = _mm_castps_si128(_mm_shuffle_ps(
              _mm_castsi128_ps(d1), _mm_castsi128_ps(d2), 0x88));
          _mm_store_si128((__m128i*)dph, d);
        }

        // extension
        hdst[-1] = hdst[0];
        hdst[H_width] = hdst[H_width-1];
        // update
        sp = src + (even ? 0 : 1);
        const si32* sph = hdst + (even ? 0 : 1);
        si32 *dpl = ldst;
        __m128i offset = _mm_set1_epi32(2);
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, sp+=8, sph+=4, dpl+=4)
        {
          __m128i s1 = _mm_loadu_si128((__m128i*)(sph-1));
          s1 = _mm_add_epi32(s1, offset);
          __m128i s2 = _mm_loadu_si128((__m128i*)sph);
          s2 = _mm_add_epi32(s2, s1);
          __m128i d1 = _mm_loadu_si128((__m128i*)sp);
          __m128i d2 = _mm_loadu_si128((__m128i*)sp + 1);
          __m128i d = _mm_castps_si128(_mm_shuffle_ps(
              _mm_castsi128_ps(d1), _mm_castsi128_ps(d2), 0x88));
          d = _mm_add_epi32(d, _mm_srai_epi32(s2, 2));
          _mm_store_si128((__m128i*)dpl, d);
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
    void sse2_rev_vert_wvlt_bwd_predict(const line_buf* line_src1,
                                        const line_buf* line_src2,
                                        line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;
    
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i, dst+=4, src1+=4, src2+=4)
      {
        __m128i s1 = _mm_load_si128((__m128i*)src1);
        __m128i s2 = _mm_load_si128((__m128i*)src2);
        __m128i d = _mm_load_si128((__m128i*)dst);
        s1 = _mm_srai_epi32(_mm_add_epi32(s1, s2), 1);
        d = _mm_add_epi32(d, s1);
        _mm_store_si128((__m128i*)dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_vert_wvlt_bwd_update(const line_buf* line_src1,
                                       const line_buf* line_src2,
                                       line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;
    
      __m128i offset = _mm_set1_epi32(2);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i, dst+=4, src1+=4, src2+=4)
      {
        __m128i s1 = _mm_load_si128((__m128i*)src1);
        s1 = _mm_add_epi32(s1, offset);
        __m128i s2 = _mm_load_si128((__m128i*)src2);
        s2 = _mm_add_epi32(s2, s1);
        __m128i d = _mm_load_si128((__m128i*)dst);
        d = _mm_sub_epi32(d, _mm_srai_epi32(s2, 2));
        _mm_store_si128((__m128i*)dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_horz_wvlt_bwd_tx(line_buf *line_dst, line_buf *line_lsrc,
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
        __m128i offset = _mm_set1_epi32(2);
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, sph+=4, spl+=4)
        {
          __m128i s1 = _mm_loadu_si128((__m128i*)(sph-1));
          s1 = _mm_add_epi32(s1, offset);
          __m128i s2 = _mm_loadu_si128((__m128i*)sph);
          s2 = _mm_add_epi32(s2, s1);
          __m128i d = _mm_load_si128((__m128i*)spl);
          d = _mm_sub_epi32(d, _mm_srai_epi32(s2, 2));
          _mm_store_si128((__m128i*)spl, d);
        }

        // extension
        lsrc[-1] = lsrc[0];
        lsrc[L_width] = lsrc[L_width - 1];
        // inverse predict and combine
        si32 *dp = dst + (even ? 0 : -1);
        spl = lsrc + (even ? 0 : -1);
        sph = hsrc;
        ui32 width = L_width + (even ? 0 : 1);
        for (ui32 i = (width + 3) >> 2; i > 0; --i, sph+=4, spl+=4, dp+=8)
        {
          __m128i s1 = _mm_loadu_si128((__m128i*)spl);
          __m128i s2 = _mm_loadu_si128((__m128i*)(spl+1));
          __m128i d = _mm_load_si128((__m128i*)sph);
          s2 = _mm_srai_epi32(_mm_add_epi32(s1, s2), 1);
          d = _mm_add_epi32(d, s2);
          _mm_storeu_si128((__m128i*)dp, _mm_unpacklo_epi32(s1, d));
          _mm_storeu_si128((__m128i*)dp + 1, _mm_unpackhi_epi32(s1, d));
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
