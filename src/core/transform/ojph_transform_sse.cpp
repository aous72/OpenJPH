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
// File: ojph_transform_sse.cpp
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
    void sse_irrev_vert_wvlt_step(const line_buf* line_src1,
                                  const line_buf* line_src2,
                                  line_buf *line_dst,
                                  int step_num, ui32 repeat)
    {
      float *dst = line_dst->f32;
      const float *src1 = line_src1->f32, *src2 = line_src2->f32;

      __m128 factor = _mm_set1_ps(LIFTING_FACTORS::steps[step_num]);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i, dst+=4, src1+=4, src2+=4)
      {
        __m128 s1 = _mm_load_ps(src1);
        __m128 s2 = _mm_load_ps(src2);
        __m128 d = _mm_load_ps(dst);
        d = _mm_add_ps(d, _mm_mul_ps(factor, _mm_add_ps(s1, s2)));
        _mm_store_ps(dst, d);
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void sse_irrev_vert_wvlt_K(const line_buf* line_src, line_buf* line_dst,
                               bool L_analysis_or_H_synthesis, ui32 repeat)
    {
      float *dst = line_dst->f32;
      const float *src = line_src->f32;

      float f = LIFTING_FACTORS::K_inv;
      f = L_analysis_or_H_synthesis ? f : LIFTING_FACTORS::K;
      __m128 factor = _mm_set1_ps(f);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i, dst+=4, src+=4)
      {
        __m128 s = _mm_load_ps(src);
        _mm_store_ps(dst, _mm_mul_ps(factor, s));
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void sse_irrev_horz_wvlt_fwd_tx(line_buf* line_src, line_buf *line_ldst,
                                    line_buf *line_hdst, ui32 width,
                                    bool even)
    {
      if (width > 1)
      {
        float *src = line_src->f32;
        float *ldst = line_ldst->f32, *hdst = line_hdst->f32;

        const ui32 L_width = (width + (even ? 1 : 0)) >> 1;
        const ui32 H_width = (width + (even ? 0 : 1)) >> 1;

        //extension
        src[-1] = src[1];
        src[width] = src[width-2];
        // predict
        const float* sp = src + (even ? 1 : 0);
        float *dph = hdst;
        __m128 factor = _mm_set1_ps(LIFTING_FACTORS::steps[0]);
        for (ui32 i = (H_width + 3) >> 2; i > 0; --i, dph+=4)
        { //this is doing twice the work it needs to do
          //it can be definitely written better
          __m128 s1 = _mm_loadu_ps(sp - 1);
          __m128 s2 = _mm_loadu_ps(sp + 1);
          __m128 d = _mm_loadu_ps(sp);
          s1 = _mm_mul_ps(factor, _mm_add_ps(s1, s2));
          __m128 d1 = _mm_add_ps(d, s1);
          sp += 4;
          s1 = _mm_loadu_ps(sp - 1);
          s2 = _mm_loadu_ps(sp + 1);
          d = _mm_loadu_ps(sp);
          s1 = _mm_mul_ps(factor, _mm_add_ps(s1, s2));
          __m128 d2 = _mm_add_ps(d, s1);
          sp += 4;
          d = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(2, 0, 2, 0));
          _mm_store_ps(dph, d);
        }

        // extension
        hdst[-1] = hdst[0];
        hdst[H_width] = hdst[H_width-1];
        // update
        factor = _mm_set1_ps(LIFTING_FACTORS::steps[1]);
        sp = src + (even ? 0 : 1);
        const float* sph = hdst + (even ? 0 : 1);
        float *dpl = ldst;
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, sp+=8, sph+=4, dpl+=4)
        {
          __m128 s1 = _mm_loadu_ps(sph - 1);
          __m128 s2 = _mm_loadu_ps(sph);
          s1 = _mm_mul_ps(factor, _mm_add_ps(s1, s2));
          __m128 d1 = _mm_loadu_ps(sp);
          __m128 d2 = _mm_loadu_ps(sp + 4);
          __m128 d = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(2, 0, 2, 0));
          d = _mm_add_ps(d, s1);
          _mm_store_ps(dpl, d);
        }

        //extension
        ldst[-1] = ldst[0];
        ldst[L_width] = ldst[L_width-1];
        //predict
        factor = _mm_set1_ps(LIFTING_FACTORS::steps[2]);
        const float* spl = ldst + (even ? 1 : 0);
        dph = hdst;
        for (ui32 i = (H_width + 3) >> 2; i > 0; --i, spl+=4, dph+=4)
        {
          __m128 s1 = _mm_loadu_ps(spl - 1);
          __m128 s2 = _mm_loadu_ps(spl);
          __m128 d = _mm_loadu_ps(dph);
          s1 = _mm_mul_ps(factor, _mm_add_ps(s1, s2));
          d = _mm_add_ps(d, s1);
          _mm_store_ps(dph, d);
        }

        // extension
        hdst[-1] = hdst[0];
        hdst[H_width] = hdst[H_width-1];
        // update
        factor = _mm_set1_ps(LIFTING_FACTORS::steps[3]);
        sph = hdst + (even ? 0 : 1);
        dpl = ldst;
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, sph+=4, dpl+=4)
        {
          __m128 s1 = _mm_loadu_ps(sph - 1);
          __m128 s2 = _mm_loadu_ps(sph);
          __m128 d = _mm_loadu_ps(dpl);
          s1 = _mm_mul_ps(factor, _mm_add_ps(s1, s2));
          d = _mm_add_ps(d, s1);
          _mm_store_ps(dpl, d);
        }

        //multipliers
        float *dp = ldst;
        factor = _mm_set1_ps(LIFTING_FACTORS::K_inv);
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, dp+=4)
        {
          __m128 d = _mm_load_ps(dp);
          _mm_store_ps(dp, _mm_mul_ps(factor, d));
        }
        dp = hdst;
        factor = _mm_set1_ps(LIFTING_FACTORS::K);
        for (int i = (H_width + 3) >> 2; i > 0; --i, dp+=4)
        {
          __m128 d = _mm_load_ps(dp);
          _mm_store_ps(dp, _mm_mul_ps(factor, d));
        }
      }
      else
      {
        if (even)
          line_ldst->f32[0] = line_src->f32[0];
        else
          line_hdst->f32[0] = line_src->f32[0] + line_src->f32[0];
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void sse_irrev_horz_wvlt_bwd_tx(line_buf* line_dst, line_buf *line_lsrc,
                                    line_buf *line_hsrc, ui32 width,
                                    bool even)
    {
      if (width > 1)
      {
        float *lsrc = line_lsrc->f32, *hsrc = line_hsrc->f32;
        float *dst = line_dst->f32;

        const ui32 L_width = (width + (even ? 1 : 0)) >> 1;
        const ui32 H_width = (width + (even ? 0 : 1)) >> 1;

        //multipliers
        float *dp = lsrc;
        __m128 factor = _mm_set1_ps(LIFTING_FACTORS::K);
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, dp+=4)
        {
          __m128 d = _mm_load_ps(dp);
          _mm_store_ps(dp, _mm_mul_ps(factor, d));
        }
        dp = hsrc;
        factor = _mm_set1_ps(LIFTING_FACTORS::K_inv);
        for (ui32 i = (H_width + 3) >> 2; i > 0; --i, dp+=4)
        {
          __m128 d = _mm_load_ps(dp);
          _mm_store_ps(dp, _mm_mul_ps(factor, d));
        }

        //extension
        hsrc[-1] = hsrc[0];
        hsrc[H_width] = hsrc[H_width-1];
        //inverse update
        factor = _mm_set1_ps(LIFTING_FACTORS::steps[7]);
        const float *sph = hsrc + (even ? 0 : 1);
        float *dpl = lsrc;
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, dpl+=4, sph+=4)
        {
          __m128 s1 = _mm_loadu_ps(sph - 1);
          __m128 s2 = _mm_loadu_ps(sph);
          __m128 d = _mm_loadu_ps(dpl);
          s1 = _mm_mul_ps(factor, _mm_add_ps(s1, s2));
          d = _mm_add_ps(d, s1);
          _mm_store_ps(dpl, d);
        }

        //extension
        lsrc[-1] = lsrc[0];
        lsrc[L_width] = lsrc[L_width-1];
        //inverse perdict
        factor = _mm_set1_ps(LIFTING_FACTORS::steps[6]);
        const float *spl = lsrc + (even ? 0 : -1);
        float *dph = hsrc;
        for (ui32 i = (H_width + 3) >> 2; i > 0; --i, dph+=4, spl+=4)
        {
          __m128 s1 = _mm_loadu_ps(spl);
          __m128 s2 = _mm_loadu_ps(spl + 1);
          __m128 d = _mm_loadu_ps(dph);
          s1 = _mm_mul_ps(factor, _mm_add_ps(s1, s2));
          d = _mm_add_ps(d, s1);
          _mm_store_ps(dph, d);
        }

        //extension
        hsrc[-1] = hsrc[0];
        hsrc[H_width] = hsrc[H_width-1];
        //inverse update
        factor = _mm_set1_ps(LIFTING_FACTORS::steps[5]);
        sph = hsrc + (even ? 0 : 1);
        dpl = lsrc;
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, dpl+=4, sph+=4)
        {
          __m128 s1 = _mm_loadu_ps(sph - 1);
          __m128 s2 = _mm_loadu_ps(sph);
          __m128 d = _mm_loadu_ps(dpl);
          s1 = _mm_mul_ps(factor, _mm_add_ps(s1, s2));
          d = _mm_add_ps(d, s1);
          _mm_store_ps(dpl, d);
        }

        //extension
        lsrc[-1] = lsrc[0];
        lsrc[L_width] = lsrc[L_width-1];
        //inverse perdict and combine
        factor = _mm_set1_ps(LIFTING_FACTORS::steps[4]);
        dp = dst + (even ? 0 : -1);
        spl = lsrc + (even ? 0 : -1);
        sph = hsrc;
        ui32 width = L_width + (even ? 0 : 1);
        for (ui32 i = (width + 3) >> 2; i > 0; --i, spl+=4, sph+=4, dp+=8)
        {
          __m128 s1 = _mm_loadu_ps(spl);
          __m128 s2 = _mm_loadu_ps(spl + 1);
          __m128 d = _mm_load_ps(sph);
          s2 = _mm_mul_ps(factor, _mm_add_ps(s1, s2));
          d = _mm_add_ps(d, s2);
          _mm_storeu_ps(dp, _mm_unpacklo_ps(s1, d));
          _mm_storeu_ps(dp + 4, _mm_unpackhi_ps(s1, d));
        }
      }
      else
      {
        if (even)
          line_dst->f32[0] = line_lsrc->f32[0];
        else
          line_dst->f32[0] = line_hsrc->f32[0] * 0.5f;
      }
    }
  }
}
