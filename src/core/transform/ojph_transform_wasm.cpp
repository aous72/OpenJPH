//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2021, Aous Naman 
// Copyright (c) 2021, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2021, The University of New South Wales, Australia
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
// File: ojph_transform_wasm.cpp
// Author: Aous Naman
// Date: 09 February 2021
//***************************************************************************/

#include <cstdio>
#include <wasm_simd128.h>

#include "ojph_defs.h"
#include "ojph_arch.h"
#include "ojph_mem.h"
#include "ojph_transform.h"
#include "ojph_transform_local.h"

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_vert_wvlt_fwd_predict(const line_buf* line_src1, 
                                        const line_buf* line_src2,
                                        line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;

      for (ui32 i = (repeat + 3) >> 2; i > 0; --i, dst+=4, src1+=4, src2+=4)
      {
        v128_t s1 = wasm_v128_load(src1);
        v128_t s2 = wasm_v128_load(src2);
        v128_t d = wasm_v128_load(dst);
        s1 = wasm_i32x4_shr(wasm_i32x4_add(s1, s2), 1);
        d = wasm_i32x4_sub(d, s1);
        wasm_v128_store(dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_vert_wvlt_fwd_update(const line_buf* line_src1, 
                                       const line_buf* line_src2,
                                       line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;

      v128_t offset = wasm_i32x4_splat(2);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i, dst+=4, src1+=4, src2+=4)
      {
        v128_t s1 = wasm_v128_load(src1);
        s1 = wasm_i32x4_add(s1, offset);
        v128_t s2 = wasm_v128_load(src2);
        s2 = wasm_i32x4_add(s2, s1);
        v128_t d = wasm_v128_load(dst);
        d = wasm_i32x4_add(d, wasm_i32x4_shr(s2, 2));
        wasm_v128_store(dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_horz_wvlt_fwd_tx(line_buf *line_src, line_buf *line_ldst, 
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
          v128_t s1 = wasm_v128_load(sp - 1);
          v128_t s2 = wasm_v128_load(sp + 1);
          v128_t d = wasm_v128_load(sp);
          s1 = wasm_i32x4_shr(wasm_i32x4_add(s1, s2), 1);
          v128_t d1 = wasm_i32x4_sub(d, s1);
          sp += 4;
          s1 = wasm_v128_load(sp - 1);
          s2 = wasm_v128_load(sp + 1);
          d = wasm_v128_load(sp);
          s1 = wasm_i32x4_shr(wasm_i32x4_add(s1, s2), 1);
          v128_t d2 = wasm_i32x4_sub(d, s1);
          sp += 4;
          d = wasm_i32x4_shuffle(d1, d2, 0, 2, 4, 6);
          wasm_v128_store(dph, d);
        }

        // extension
        hdst[-1] = hdst[0];
        hdst[H_width] = hdst[H_width-1];
        // update
        sp = src + (even ? 0 : 1);
        const si32* sph = hdst + (even ? 0 : 1);
        si32 *dpl = ldst;
        v128_t offset = wasm_i32x4_splat(2);
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, sp+=8, sph+=4, dpl+=4)
        {
          v128_t s1 = wasm_v128_load(sph - 1);
          s1 = wasm_i32x4_add(s1, offset);
          v128_t s2 = wasm_v128_load(sph);
          s2 = wasm_i32x4_add(s2, s1);
          v128_t d1 = wasm_v128_load(sp);
          v128_t d2 = wasm_v128_load(sp + 4);
          v128_t d = wasm_i32x4_shuffle(d1, d2, 0, 2, 4, 6);
          d = wasm_i32x4_add(d, wasm_i32x4_shr(s2, 2));
          wasm_v128_store(dpl, d);
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
    void wasm_rev_vert_wvlt_bwd_predict(const line_buf *line_src1, 
                                        const line_buf *line_src2,
                                        line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;
    
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i, dst+=4, src1+=4, src2+=4)
      {
        v128_t s1 = wasm_v128_load(src1);
        v128_t s2 = wasm_v128_load(src2);
        v128_t d = wasm_v128_load(dst);
        s1 = wasm_i32x4_shr(wasm_i32x4_add(s1, s2), 1);
        d = wasm_i32x4_add(d, s1);
        wasm_v128_store(dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_vert_wvlt_bwd_update(const line_buf *line_src1, 
                                       const line_buf *line_src2,
                                       line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;
    
      v128_t offset = wasm_i32x4_splat(2);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i, dst+=4, src1+=4, src2+=4)
      {
        v128_t s1 = wasm_v128_load(src1);
        s1 = wasm_i32x4_add(s1, offset);
        v128_t s2 = wasm_v128_load(src2);
        s2 = wasm_i32x4_add(s2, s1);
        v128_t d = wasm_v128_load(dst);
        d = wasm_i32x4_sub(d, wasm_i32x4_shr(s2, 2));
        wasm_v128_store(dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_horz_wvlt_bwd_tx(line_buf *line_dst, line_buf *line_lsrc, 
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
        v128_t offset = wasm_i32x4_splat(2);
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, sph+=4, spl+=4)
        {
          v128_t s1 = wasm_v128_load(sph - 1);
          s1 = wasm_i32x4_add(s1, offset);
          v128_t s2 = wasm_v128_load(sph);
          s2 = wasm_i32x4_add(s2, s1);
          v128_t d = wasm_v128_load(spl);
          d = wasm_i32x4_sub(d, wasm_i32x4_shr(s2, 2));
          wasm_v128_store(spl, d);
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
          v128_t s1 = wasm_v128_load(spl);
          v128_t s2 = wasm_v128_load(spl + 1);
          v128_t d = wasm_v128_load(sph);
          s2 = wasm_i32x4_shr(wasm_i32x4_add(s1, s2), 1);
          d = wasm_i32x4_add(d, s2);
          wasm_v128_store(dp, wasm_i32x4_shuffle(s1, d, 0, 4, 1, 5));
          wasm_v128_store(dp + 4, wasm_i32x4_shuffle(s1, d, 2, 6, 3, 7));
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
    
    //////////////////////////////////////////////////////////////////////////
    void wasm_irrev_vert_wvlt_step(const line_buf *line_src1, 
                                   const line_buf *line_src2,
                                   line_buf *line_dst, int step_num, 
                                   ui32 repeat)
    {
      float *dst = line_dst->f32;
      const float *src1 = line_src1->f32, *src2 = line_src2->f32;
    
      v128_t factor = wasm_f32x4_splat(LIFTING_FACTORS::steps[step_num]);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i, dst+=4, src1+=4, src2+=4)
      {
        v128_t s1 = wasm_v128_load(src1);
        v128_t s2 = wasm_v128_load(src2);
        v128_t d = wasm_v128_load(dst);
        d = wasm_f32x4_add(d, wasm_f32x4_mul(factor, wasm_f32x4_add(s1, s2)));
        wasm_v128_store(dst, d);
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void wasm_irrev_vert_wvlt_K(const line_buf *line_src, line_buf *line_dst,
                                bool L_analysis_or_H_synthesis, ui32 repeat)
    {
      float *dst = line_dst->f32;
      const float *src = line_src->f32;

      float f = LIFTING_FACTORS::K_inv;
      f = L_analysis_or_H_synthesis ? f : LIFTING_FACTORS::K;
      v128_t factor = wasm_f32x4_splat(f);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i, dst+=4, src+=4)
      {
        v128_t s = wasm_v128_load(src);
        wasm_v128_store(dst, wasm_f32x4_mul(factor, s));
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void wasm_irrev_horz_wvlt_fwd_tx(line_buf *line_src, line_buf *line_ldst, 
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
        v128_t factor = wasm_f32x4_splat(LIFTING_FACTORS::steps[0]);
        for (ui32 i = (H_width + 3) >> 2; i > 0; --i, dph+=4)
        { //this is doing twice the work it needs to do
          //it can be definitely written better
          v128_t s1 = wasm_v128_load(sp - 1);
          v128_t s2 = wasm_v128_load(sp + 1);
          v128_t d = wasm_v128_load(sp);
          s1 = wasm_f32x4_mul(factor, wasm_f32x4_add(s1, s2));
          v128_t d1 = wasm_f32x4_add(d, s1);
          sp += 4;
          s1 = wasm_v128_load(sp - 1);
          s2 = wasm_v128_load(sp + 1);
          d = wasm_v128_load(sp);
          s1 = wasm_f32x4_mul(factor, wasm_f32x4_add(s1, s2));
          v128_t d2 = wasm_f32x4_add(d, s1);
          sp += 4;
          d = wasm_i32x4_shuffle(d1, d2, 0, 2, 4, 6);
          wasm_v128_store(dph, d);
        }

        // extension
        hdst[-1] = hdst[0];
        hdst[H_width] = hdst[H_width-1];
        // update
        factor = wasm_f32x4_splat(LIFTING_FACTORS::steps[1]);
        sp = src + (even ? 0 : 1);
        const float* sph = hdst + (even ? 0 : 1);
        float *dpl = ldst;
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, sp+=8, sph+=4, dpl+=4)
        {
          v128_t s1 = wasm_v128_load(sph - 1);
          v128_t s2 = wasm_v128_load(sph);
          s1 = wasm_f32x4_mul(factor, wasm_f32x4_add(s1, s2));
          v128_t d1 = wasm_v128_load(sp);
          v128_t d2 = wasm_v128_load(sp + 4);
          v128_t d = wasm_i32x4_shuffle(d1, d2, 0, 2, 4, 6);
          d = wasm_f32x4_add(d, s1);
          wasm_v128_store(dpl, d);
        }

        //extension
        ldst[-1] = ldst[0];
        ldst[L_width] = ldst[L_width-1];
        //predict
        factor = wasm_f32x4_splat(LIFTING_FACTORS::steps[2]);
        const float* spl = ldst + (even ? 1 : 0);
        dph = hdst;
        for (ui32 i = (H_width + 3) >> 2; i > 0; --i, spl+=4, dph+=4)
        {
          v128_t s1 = wasm_v128_load(spl - 1);
          v128_t s2 = wasm_v128_load(spl);
          v128_t d = wasm_v128_load(dph);
          s1 = wasm_f32x4_mul(factor, wasm_f32x4_add(s1, s2));
          d = wasm_f32x4_add(d, s1);
          wasm_v128_store(dph, d);
        }

        // extension
        hdst[-1] = hdst[0];
        hdst[H_width] = hdst[H_width-1];
        // update
        factor = wasm_f32x4_splat(LIFTING_FACTORS::steps[3]);
        sph = hdst + (even ? 0 : 1);
        dpl = ldst;
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, sph+=4, dpl+=4)
        {
          v128_t s1 = wasm_v128_load(sph - 1);
          v128_t s2 = wasm_v128_load(sph);
          v128_t d = wasm_v128_load(dpl);
          s1 = wasm_f32x4_mul(factor, wasm_f32x4_add(s1, s2));
          d = wasm_f32x4_add(d, s1);
          wasm_v128_store(dpl, d);
        }

        //multipliers
        float *dp = ldst;
        factor = wasm_f32x4_splat(LIFTING_FACTORS::K_inv);
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, dp+=4)
        {
          v128_t d = wasm_v128_load(dp);
          wasm_v128_store(dp, wasm_f32x4_mul(factor, d));
        }
        dp = hdst;
        factor = wasm_f32x4_splat(LIFTING_FACTORS::K);
        for (int i = (H_width + 3) >> 2; i > 0; --i, dp+=4)
        {
          v128_t d = wasm_v128_load(dp);
          wasm_v128_store(dp, wasm_f32x4_mul(factor, d));
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
    void wasm_irrev_horz_wvlt_bwd_tx(line_buf *line_dst, line_buf *line_lsrc, 
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
        v128_t factor = wasm_f32x4_splat(LIFTING_FACTORS::K);
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, dp+=4)
        {
          v128_t d = wasm_v128_load(dp);
          wasm_v128_store(dp, wasm_f32x4_mul(factor, d));
        }
        dp = hsrc;
        factor = wasm_f32x4_splat(LIFTING_FACTORS::K_inv);
        for (ui32 i = (H_width + 3) >> 2; i > 0; --i, dp+=4)
        {
          v128_t d = wasm_v128_load(dp);
          wasm_v128_store(dp, wasm_f32x4_mul(factor, d));
        }

        //extension
        hsrc[-1] = hsrc[0];
        hsrc[H_width] = hsrc[H_width-1];
        //inverse update
        factor = wasm_f32x4_splat(LIFTING_FACTORS::steps[7]);
        const float *sph = hsrc + (even ? 0 : 1);
        float *dpl = lsrc;
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, dpl+=4, sph+=4)
        {
          v128_t s1 = wasm_v128_load(sph - 1);
          v128_t s2 = wasm_v128_load(sph);
          v128_t d = wasm_v128_load(dpl);
          s1 = wasm_f32x4_mul(factor, wasm_f32x4_add(s1, s2));
          d = wasm_f32x4_add(d, s1);
          wasm_v128_store(dpl, d);
        }

        //extension
        lsrc[-1] = lsrc[0];
        lsrc[L_width] = lsrc[L_width-1];
        //inverse perdict
        factor = wasm_f32x4_splat(LIFTING_FACTORS::steps[6]);
        const float *spl = lsrc + (even ? 0 : -1);
        float *dph = hsrc;
        for (ui32 i = (H_width + 3) >> 2; i > 0; --i, dph+=4, spl+=4)
        {
          v128_t s1 = wasm_v128_load(spl);
          v128_t s2 = wasm_v128_load(spl + 1);
          v128_t d = wasm_v128_load(dph);
          s1 = wasm_f32x4_mul(factor, wasm_f32x4_add(s1, s2));
          d = wasm_f32x4_add(d, s1);
          wasm_v128_store(dph, d);
        }

        //extension
        hsrc[-1] = hsrc[0];
        hsrc[H_width] = hsrc[H_width-1];
        //inverse update
        factor = wasm_f32x4_splat(LIFTING_FACTORS::steps[5]);
        sph = hsrc + (even ? 0 : 1);
        dpl = lsrc;
        for (ui32 i = (L_width + 3) >> 2; i > 0; --i, dpl+=4, sph+=4)
        {
          v128_t s1 = wasm_v128_load(sph - 1);
          v128_t s2 = wasm_v128_load(sph);
          v128_t d = wasm_v128_load(dpl);
          s1 = wasm_f32x4_mul(factor, wasm_f32x4_add(s1, s2));
          d = wasm_f32x4_add(d, s1);
          wasm_v128_store(dpl, d);
        }

        //extension
        lsrc[-1] = lsrc[0];
        lsrc[L_width] = lsrc[L_width-1];
        //inverse perdict and combine
        factor = wasm_f32x4_splat(LIFTING_FACTORS::steps[4]);
        dp = dst + (even ? 0 : -1);
        spl = lsrc + (even ? 0 : -1);
        sph = hsrc;
        ui32 width = L_width + (even ? 0 : 1);
        for (ui32 i = (width + 3) >> 2; i > 0; --i, spl+=4, sph+=4, dp+=8)
        {
          v128_t s1 = wasm_v128_load(spl);
          v128_t s2 = wasm_v128_load(spl + 1);
          v128_t d = wasm_v128_load(sph);
          s2 = wasm_f32x4_mul(factor, wasm_f32x4_add(s1, s2));
          d = wasm_f32x4_add(d, s2);
          wasm_v128_store(dp, wasm_i32x4_shuffle(s1, d, 0, 4, 1, 5));
          wasm_v128_store(dp + 4, wasm_i32x4_shuffle(s1, d, 2, 6, 3, 7));
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
