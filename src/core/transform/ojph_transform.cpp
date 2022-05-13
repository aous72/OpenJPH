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
// File: ojph_transform.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/

#include <cstdio>

#include "ojph_arch.h"
#include "ojph_mem.h"
#include "ojph_transform.h"
#include "ojph_transform_local.h"

namespace ojph {
  struct line_buf;

  namespace local {

    /////////////////////////////////////////////////////////////////////////
    // Reversible functions
    /////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////
    void (*rev_vert_wvlt_fwd_predict)
      (const line_buf* src1, const line_buf* src2, line_buf *dst,
       ui32 repeat) = NULL;

    /////////////////////////////////////////////////////////////////////////
    void (*rev_vert_wvlt_fwd_update)
      (const line_buf* src1, const line_buf* src2, line_buf *dst,
       ui32 repeat) = NULL;

    /////////////////////////////////////////////////////////////////////////
    void (*rev_horz_wvlt_fwd_tx)
      (line_buf* src, line_buf *ldst, line_buf *hdst, ui32 width, bool even)
      = NULL;

    /////////////////////////////////////////////////////////////////////////
    void (*rev_vert_wvlt_bwd_predict)
      (const line_buf* src1, const line_buf* src2, line_buf *dst,
       ui32 repeat) = NULL;

    /////////////////////////////////////////////////////////////////////////
    void (*rev_vert_wvlt_bwd_update)
      (const line_buf* src1, const line_buf* src2, line_buf *dst,
       ui32 repeat) = NULL;

    /////////////////////////////////////////////////////////////////////////
    void (*rev_horz_wvlt_bwd_tx)
      (line_buf* dst, line_buf *lsrc, line_buf *hsrc, ui32 width, bool even)
      = NULL;

    /////////////////////////////////////////////////////////////////////////
    // Irreversible functions
    /////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////
    void (*irrev_vert_wvlt_step)
      (const line_buf* src1, const line_buf* src2, line_buf *dst,
       int step_num, ui32 repeat) = NULL;

    /////////////////////////////////////////////////////////////////////////
    void (*irrev_vert_wvlt_K)
      (const line_buf *src, line_buf *dst, bool L_analysis_or_H_synthesis,
       ui32 repeat) = NULL;

    /////////////////////////////////////////////////////////////////////////
    void (*irrev_horz_wvlt_fwd_tx)
      (line_buf* src, line_buf *ldst, line_buf *hdst, ui32 width, bool even)
      = NULL;

    /////////////////////////////////////////////////////////////////////////
    void (*irrev_horz_wvlt_bwd_tx)
      (line_buf* src, line_buf *ldst, line_buf *hdst, ui32 width, bool even)
      = NULL;

    ////////////////////////////////////////////////////////////////////////////
    static bool wavelet_transform_functions_initialized = false;

    //////////////////////////////////////////////////////////////////////////
    void init_wavelet_transform_functions()
    {
      if (wavelet_transform_functions_initialized)
        return;

#if !defined(OJPH_ENABLE_WASM_SIMD) || !defined(OJPH_EMSCRIPTEN)

      rev_vert_wvlt_fwd_predict = gen_rev_vert_wvlt_fwd_predict;
      rev_vert_wvlt_fwd_update  = gen_rev_vert_wvlt_fwd_update;
      rev_horz_wvlt_fwd_tx      = gen_rev_horz_wvlt_fwd_tx;
      rev_vert_wvlt_bwd_predict = gen_rev_vert_wvlt_bwd_predict;
      rev_vert_wvlt_bwd_update  = gen_rev_vert_wvlt_bwd_update;
      rev_horz_wvlt_bwd_tx      = gen_rev_horz_wvlt_bwd_tx;
      irrev_vert_wvlt_step      = gen_irrev_vert_wvlt_step;
      irrev_vert_wvlt_K         = gen_irrev_vert_wvlt_K;
      irrev_horz_wvlt_fwd_tx    = gen_irrev_horz_wvlt_fwd_tx;
      irrev_horz_wvlt_bwd_tx    = gen_irrev_horz_wvlt_bwd_tx;

#ifndef OJPH_DISABLE_INTEL_SIMD
      int level = get_cpu_ext_level();

      if (level >= X86_CPU_EXT_LEVEL_SSE)
      {
        irrev_vert_wvlt_step    = sse_irrev_vert_wvlt_step;
        irrev_vert_wvlt_K       = sse_irrev_vert_wvlt_K;
        irrev_horz_wvlt_fwd_tx  = sse_irrev_horz_wvlt_fwd_tx;
        irrev_horz_wvlt_bwd_tx  = sse_irrev_horz_wvlt_bwd_tx;
      }

      if (level >= X86_CPU_EXT_LEVEL_SSE2)
      {
        rev_vert_wvlt_fwd_predict = sse2_rev_vert_wvlt_fwd_predict;
        rev_vert_wvlt_fwd_update  = sse2_rev_vert_wvlt_fwd_update;
        rev_horz_wvlt_fwd_tx      = sse2_rev_horz_wvlt_fwd_tx;
        rev_vert_wvlt_bwd_predict = sse2_rev_vert_wvlt_bwd_predict;
        rev_vert_wvlt_bwd_update  = sse2_rev_vert_wvlt_bwd_update;
        rev_horz_wvlt_bwd_tx      = sse2_rev_horz_wvlt_bwd_tx;
      }

      if (level >= X86_CPU_EXT_LEVEL_AVX)
      {
        irrev_vert_wvlt_step   = avx_irrev_vert_wvlt_step;
        irrev_vert_wvlt_K      = avx_irrev_vert_wvlt_K;
        irrev_horz_wvlt_fwd_tx = avx_irrev_horz_wvlt_fwd_tx;
        irrev_horz_wvlt_bwd_tx = avx_irrev_horz_wvlt_bwd_tx;
      }

      if (level >= X86_CPU_EXT_LEVEL_AVX2)
      {
        rev_vert_wvlt_fwd_predict = avx2_rev_vert_wvlt_fwd_predict;
        rev_vert_wvlt_fwd_update  = avx2_rev_vert_wvlt_fwd_update;
        rev_horz_wvlt_fwd_tx      = avx2_rev_horz_wvlt_fwd_tx;
        rev_vert_wvlt_bwd_predict = avx2_rev_vert_wvlt_bwd_predict;
        rev_vert_wvlt_bwd_update  = avx2_rev_vert_wvlt_bwd_update;
        rev_horz_wvlt_bwd_tx      = avx2_rev_horz_wvlt_bwd_tx;
      }
#endif // !OJPH_DISABLE_INTEL_SIMD

#else // OJPH_ENABLE_WASM_SIMD
      rev_vert_wvlt_fwd_predict = wasm_rev_vert_wvlt_fwd_predict;
      rev_vert_wvlt_fwd_update  = wasm_rev_vert_wvlt_fwd_update;
      rev_horz_wvlt_fwd_tx      = wasm_rev_horz_wvlt_fwd_tx;
      rev_vert_wvlt_bwd_predict = wasm_rev_vert_wvlt_bwd_predict;
      rev_vert_wvlt_bwd_update  = wasm_rev_vert_wvlt_bwd_update;
      rev_horz_wvlt_bwd_tx      = wasm_rev_horz_wvlt_bwd_tx;
      irrev_vert_wvlt_step      = wasm_irrev_vert_wvlt_step;
      irrev_vert_wvlt_K         = wasm_irrev_vert_wvlt_K;
      irrev_horz_wvlt_fwd_tx    = wasm_irrev_horz_wvlt_fwd_tx;
      irrev_horz_wvlt_bwd_tx    = wasm_irrev_horz_wvlt_bwd_tx;
#endif // !OJPH_ENABLE_WASM_SIMD

      wavelet_transform_functions_initialized = true;
    }
    
    //////////////////////////////////////////////////////////////////////////
    const float LIFTING_FACTORS::steps[8] =
    {
      -1.586134342059924f, -0.052980118572961f, +0.882911075530934f,
      +0.443506852043971f,
      +1.586134342059924f, +0.052980118572961f, -0.882911075530934f,
      -0.443506852043971f
    };
    const float LIFTING_FACTORS::K = 1.230174104914001f;
    const float LIFTING_FACTORS::K_inv  = (float)(1.0 / 1.230174104914001);

    //////////////////////////////////////////////////////////////////////////

#if !defined(OJPH_ENABLE_WASM_SIMD) || !defined(OJPH_EMSCRIPTEN)

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_vert_wvlt_fwd_predict(const line_buf* line_src1,
                                       const line_buf* line_src2,
                                       line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;
      for (ui32 i = repeat; i > 0; --i)
        *dst++ -= (*src1++ + *src2++) >> 1;
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_vert_wvlt_fwd_update(const line_buf* line_src1,
                                      const line_buf* line_src2,
                                      line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;
      for (ui32 i = repeat; i > 0; --i)
        *dst++ += (*src1++ + *src2++ + 2) >> 2;
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_horz_wvlt_fwd_tx(line_buf *line_src, line_buf *line_ldst,
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
        for (ui32 i = H_width; i > 0; --i, sp+=2)
          *dph++ = sp[0] - ((sp[-1] + sp[1]) >> 1);

        // extension
        hdst[-1] = hdst[0];
        hdst[H_width] = hdst[H_width-1];
        // update
        sp = src + (even ? 0 : 1);
        const si32* sph = hdst + (even ? 0 : 1);
        si32 *dpl = ldst;
        for (ui32 i = L_width; i > 0; --i, sp+=2, sph++)
          *dpl++ = *sp + ((2 + sph[-1] + sph[0]) >> 2);
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
    void gen_rev_vert_wvlt_bwd_predict(const line_buf* line_src1,
                                       const line_buf* line_src2,
                                       line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;
      for (ui32 i = repeat; i > 0; --i)
        *dst++ += (*src1++ + *src2++) >> 1;
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_vert_wvlt_bwd_update(const line_buf* line_src1,
                                      const line_buf* line_src2,
                                      line_buf *line_dst, ui32 repeat)
    {
      si32 *dst = line_dst->i32;
      const si32 *src1 = line_src1->i32, *src2 = line_src2->i32;
      for (ui32 i = repeat; i > 0; --i)
        *dst++ -= (2 + *src1++ + *src2++) >> 2;
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_horz_wvlt_bwd_tx(line_buf* line_dst, line_buf *line_lsrc,
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
        for (ui32 i = L_width; i > 0; --i, sph++, spl++)
          *spl -= ((2 + sph[-1] + sph[0]) >> 2);

        // extension
        lsrc[-1] = lsrc[0];
        lsrc[L_width] = lsrc[L_width - 1];
        // inverse predict and combine
        si32 *dp = dst + (even ? 0 : -1);
        spl = lsrc + (even ? 0 : -1);
        sph = hsrc;
        for (ui32 i = L_width + (even ? 0 : 1); i > 0; --i, spl++, sph++)
        {
          *dp++ = *spl;
          *dp++ = *sph + ((spl[0] + spl[1]) >> 1);
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
    void gen_irrev_vert_wvlt_step(const line_buf* line_src1,
                                  const line_buf* line_src2,
                                  line_buf *line_dst,
                                  int step_num, ui32 repeat)
    {
      float *dst = line_dst->f32;
      const float *src1 = line_src1->f32, *src2 = line_src2->f32;
      float factor = LIFTING_FACTORS::steps[step_num];
      for (ui32 i = repeat; i > 0; --i)
        *dst++ += factor * (*src1++ + *src2++);
    }

    /////////////////////////////////////////////////////////////////////////
    void gen_irrev_vert_wvlt_K(const line_buf* line_src,
                               line_buf* line_dst,
                               bool L_analysis_or_H_synthesis, ui32 repeat)
    {
      float *dst = line_dst->f32;
      const float *src = line_src->f32;
      float factor = LIFTING_FACTORS::K_inv;
      factor = L_analysis_or_H_synthesis ? factor : LIFTING_FACTORS::K;
      for (ui32 i = repeat; i > 0; --i)
        *dst++ = *src++ * factor;
    }


    /////////////////////////////////////////////////////////////////////////
    void gen_irrev_horz_wvlt_fwd_tx(line_buf* line_src,
                                    line_buf *line_ldst,
                                    line_buf *line_hdst,
                                    ui32 width, bool even)
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
        float factor = LIFTING_FACTORS::steps[0];
        const float* sp = src + (even ? 1 : 0);
        float *dph = hdst;
        for (ui32 i = H_width; i > 0; --i, sp+=2)
          *dph++ = sp[0] + factor * (sp[-1] + sp[1]);

        // extension
        hdst[-1] = hdst[0];
        hdst[H_width] = hdst[H_width-1];
        // update
        factor = LIFTING_FACTORS::steps[1];
        sp = src + (even ? 0 : 1);
        const float* sph = hdst + (even ? 0 : 1);
        float *dpl = ldst;
        for (ui32 i = L_width; i > 0; --i, sp+=2, sph++)
          *dpl++ = sp[0] + factor * (sph[-1] + sph[0]);

        //extension
        ldst[-1] = ldst[0];
        ldst[L_width] = ldst[L_width-1];
        //predict
        factor = LIFTING_FACTORS::steps[2];
        const float* spl = ldst + (even ? 1 : 0);
        dph = hdst;
        for (ui32 i = H_width; i > 0; --i, spl++)
          *dph++ += factor * (spl[-1] + spl[0]);

        // extension
        hdst[-1] = hdst[0];
        hdst[H_width] = hdst[H_width-1];
        // update
        factor = LIFTING_FACTORS::steps[3];
        sph = hdst + (even ? 0 : 1);
        dpl = ldst;
        for (ui32 i = L_width; i > 0; --i, sph++)
          *dpl++ += factor * (sph[-1] + sph[0]);

        //multipliers
        float *dp = ldst;
        for (ui32 i = L_width; i > 0; --i, dp++)
          *dp *= LIFTING_FACTORS::K_inv;
        dp = hdst;
        for (ui32 i = H_width; i > 0; --i, dp++)
          *dp *= LIFTING_FACTORS::K;
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
    void gen_irrev_horz_wvlt_bwd_tx(line_buf* line_dst, line_buf *line_lsrc,
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
        for (ui32 i = L_width; i > 0; --i, dp++)
          *dp *= LIFTING_FACTORS::K;
        dp = hsrc;
        for (ui32 i = H_width; i > 0; --i, dp++)
          *dp *= LIFTING_FACTORS::K_inv;

        //extension
        hsrc[-1] = hsrc[0];
        hsrc[H_width] = hsrc[H_width-1];
        //inverse update
        float factor = LIFTING_FACTORS::steps[7];
        const float *sph = hsrc + (even ? 0 : 1);
        float *dpl = lsrc;
        for (ui32 i = L_width; i > 0; --i, dpl++, sph++)
          *dpl += factor * (sph[-1] + sph[0]);

        //extension
        lsrc[-1] = lsrc[0];
        lsrc[L_width] = lsrc[L_width-1];
        //inverse perdict
        factor = LIFTING_FACTORS::steps[6];
        const float *spl = lsrc + (even ? 0 : -1);
        float *dph = hsrc;
        for (ui32 i = H_width; i > 0; --i, dph++, spl++)
          *dph += factor * (spl[0] + spl[1]);

        //extension
        hsrc[-1] = hsrc[0];
        hsrc[H_width] = hsrc[H_width-1];
        //inverse update
        factor = LIFTING_FACTORS::steps[5];
        sph = hsrc + (even ? 0 : 1);
        dpl = lsrc;
        for (ui32 i = L_width; i > 0; --i, dpl++, sph++)
          *dpl += factor * (sph[-1] + sph[0]);

        //extension
        lsrc[-1] = lsrc[0];
        lsrc[L_width] = lsrc[L_width-1];
        //inverse perdict and combine
        factor = LIFTING_FACTORS::steps[4];
        dp = dst + (even ? 0 : -1);
        spl = lsrc + (even ? 0 : -1);
        sph = hsrc;
        for (ui32 i = L_width+(even?0:1); i > 0; --i, spl++, sph++)
        {
          *dp++ = *spl;
          *dp++ = *sph + factor * (spl[0] + spl[1]);
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

#endif // !OJPH_ENABLE_WASM_SIMD

  }
}
