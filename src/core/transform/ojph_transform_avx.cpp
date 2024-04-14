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
// File: ojph_transform_avx.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/

#include <cstdio>
#include <immintrin.h>

#include "ojph_defs.h"
#include "ojph_arch.h"
#include "ojph_mem.h"
#include "ojph_params.h"
#include "../codestream/ojph_params_local.h"

#include "ojph_transform.h"
#include "ojph_transform_local.h"

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    static inline void avx_multiply_const(float* p, float f, int width)
    {
      __m256 factor = _mm256_set1_ps(f);
      for (; width > 0; width -= 8, p += 8)
      {
        __m256 s = _mm256_load_ps(p);
        _mm256_store_ps(p, _mm256_mul_ps(factor, s));
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx_irv_vert_step(const lifting_step* s, const line_buf* sig, 
                           const line_buf* other, const line_buf* aug, 
                           ui32 repeat, bool synthesis)
    {
      float a = s->irv.Aatk;
      if (synthesis)
        a = -a;

      __m256 factor = _mm256_set1_ps(a);

      float* dst = aug->f32;
      const float* src1 = sig->f32, * src2 = other->f32;
      int i = (int)repeat;
      for ( ; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
      {
        __m256 s1 = _mm256_load_ps(src1);
        __m256 s2 = _mm256_load_ps(src2);
        __m256 d = _mm256_load_ps(dst);
        d = _mm256_add_ps(d, _mm256_mul_ps(factor, _mm256_add_ps(s1, s2)));
        _mm256_store_ps(dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat)
    {
      avx_multiply_const(aug->f32, K, (int)repeat);
    }

    /////////////////////////////////////////////////////////////////////////
    void avx_irv_horz_ana(const param_atk* atk, const line_buf* ldst, 
                          const line_buf* hdst, const line_buf* src, 
                          ui32 width, bool even)
    {
      if (width > 1)
      {
        // split src into ldst and hdst
        {
          float* dpl = ldst->f32;
          float* dph = hdst->f32;
          float* sp = src->f32;
          int w = (int)width;
          AVX_DEINTERLEAVE(dpl, dph, sp, w, even);
        }

        // the actual horizontal transform
        float* hp = hdst->f32, * lp = ldst->f32;
        ui32 l_width = (width + (even ? 1 : 0)) >> 1;  // low pass
        ui32 h_width = (width + (even ? 0 : 1)) >> 1;  // high pass
        ui32 num_steps = atk->get_num_steps();
        for (ui32 j = num_steps; j > 0; --j)
        {
          const lifting_step* s = atk->get_step(j - 1);
          const float a = s->irv.Aatk;

          // extension
          lp[-1] = lp[0];
          lp[l_width] = lp[l_width - 1];
          // lifting step
          const float* sp = lp;
          float* dp = hp;
          int i = (int)h_width;
          __m256 f = _mm256_set1_ps(a);
          if (even)
          {
            for (; i > 0; i -= 8, sp += 8, dp += 8)
            {
              __m256 m = _mm256_load_ps(sp);
              __m256 n = _mm256_loadu_ps(sp + 1);
              __m256 p = _mm256_load_ps(dp);
              p = _mm256_add_ps(p, _mm256_mul_ps(f, _mm256_add_ps(m, n)));
              _mm256_store_ps(dp, p);
            }
          }
          else
          {
            for (; i > 0; i -= 8, sp += 8, dp += 8)
            {
              __m256 m = _mm256_load_ps(sp);
              __m256 n = _mm256_loadu_ps(sp - 1);
              __m256 p = _mm256_load_ps(dp);
              p = _mm256_add_ps(p, _mm256_mul_ps(f, _mm256_add_ps(m, n)));
              _mm256_store_ps(dp, p);
            }
          }

          // swap buffers
          float* t = lp; lp = hp; hp = t;
          even = !even;
          ui32 w = l_width; l_width = h_width; h_width = w;
        }

        { // multiply by K or 1/K
          float K = atk->get_K();
          float K_inv = 1.0f / K;
          avx_multiply_const(lp, K_inv, (int)l_width);
          avx_multiply_const(hp, K, (int)h_width);
        }
      }
      else {
        if (even)
          ldst->f32[0] = src->f32[0];
        else
          hdst->f32[0] = src->f32[0] * 2.0f;
      }
    }
    
    //////////////////////////////////////////////////////////////////////////
    void avx_irv_horz_syn(const param_atk* atk, const line_buf* dst, 
                          const line_buf* lsrc, const line_buf* hsrc, 
                          ui32 width, bool even)
    {
      if (width > 1)
      {
        bool ev = even;
        float* oth = hsrc->f32, * aug = lsrc->f32;
        ui32 aug_width = (width + (even ? 1 : 0)) >> 1;  // low pass
        ui32 oth_width = (width + (even ? 0 : 1)) >> 1;  // high pass

        { // multiply by K or 1/K
          float K = atk->get_K();
          float K_inv = 1.0f / K;
          avx_multiply_const(aug, K, (int)aug_width);
          avx_multiply_const(oth, K_inv, (int)oth_width);
        }

        // the actual horizontal transform
        ui32 num_steps = atk->get_num_steps();
        for (ui32 j = 0; j < num_steps; ++j)
        {
          const lifting_step* s = atk->get_step(j);
          const float a = s->irv.Aatk;

          // extension
          oth[-1] = oth[0];
          oth[oth_width] = oth[oth_width - 1];
          // lifting step
          const float* sp = oth;
          float* dp = aug;
          int i = (int)aug_width;
          __m256 f = _mm256_set1_ps(a);
          if (ev)
          {
            for (; i > 0; i -= 8, sp += 8, dp += 8)
            {
              __m256 m = _mm256_load_ps(sp);
              __m256 n = _mm256_loadu_ps(sp - 1);
              __m256 p = _mm256_load_ps(dp);
              p = _mm256_sub_ps(p, _mm256_mul_ps(f, _mm256_add_ps(m, n)));
              _mm256_store_ps(dp, p);
            }
          }
          else
          {
            for (; i > 0; i -= 8, sp += 8, dp += 8)
            {
              __m256 m = _mm256_load_ps(sp);
              __m256 n = _mm256_loadu_ps(sp + 1);
              __m256 p = _mm256_load_ps(dp);
              p = _mm256_sub_ps(p, _mm256_mul_ps(f, _mm256_add_ps(m, n)));
              _mm256_store_ps(dp, p);
            }
          }

          // swap buffers
          float* t = aug; aug = oth; oth = t;
          ev = !ev;
          ui32 w = aug_width; aug_width = oth_width; oth_width = w;
        }

        // combine both lsrc and hsrc into dst
        {
          float* dp = dst->f32;
          float* spl = lsrc->f32;
          float* sph = hsrc->f32;
          int w = (int)width;
          AVX_INTERLEAVE(dp, spl, sph, w, even);
        }
      }
      else {
        if (even)
          dst->f32[0] = lsrc->f32[0];
        else
          dst->f32[0] = hsrc->f32[0] * 0.5f;
      }
    }

  } // !local
} // !ojph
