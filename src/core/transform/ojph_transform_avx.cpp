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
#include "ojph_transform.h"
#include "ojph_transform_local.h"

#include "ojph_params.h"
#include "../codestream/ojph_params_local.h"

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    void avx_irv_vert_ana_step(const lifting_step* s, const line_buf* sig, 
                               const line_buf* other, const line_buf* aug, 
                               ui32 repeat)
    {
      __m256 factor = _mm256_set1_ps(s->irv.Aatk);

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

    /////////////////////////////////////////////////////////////////////////
    void avx_irv_horz_ana(const param_atk* atk, const line_buf* ldst, 
                          const line_buf* hdst, const line_buf* src, 
                          ui32 width, bool even)
    {
      if (width > 1)
      {
        // split src into ldst and hdst
        if (even)
        {
          float* dph = hdst->f32;
          float* dpl = ldst->f32;
          float* sp = src->f32;
          int i = (int)width;
          for ( ; i > 8; i -= 16, sp += 16, dpl += 8, dph += 8)
          {
             __m256 a = _mm256_load_ps(sp);
             __m256 b = _mm256_load_ps(sp + 8);
             __m256 c = _mm256_permute2f128_ps(a, b, (2 << 4) | (0));
             __m256 d = _mm256_permute2f128_ps(a, b, (3 << 4) | (1));
             __m256 e = _mm256_shuffle_ps(c, d, _MM_SHUFFLE(2, 0, 2, 0));
             __m256 f = _mm256_shuffle_ps(c, d, _MM_SHUFFLE(3, 1, 3, 1));
             _mm256_store_ps(dpl, e);
             _mm256_store_ps(dph, f);
          }
          for (; i > 0; i -= 8, sp += 8, dpl += 4, dph += 4)
          {
            __m128 a = _mm_load_ps(sp);
            __m128 b = _mm_load_ps(sp + 4);
            __m128 c = _mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0));
            __m128 d = _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1));
            _mm_store_ps(dpl, c);
            _mm_store_ps(dph, d);
          }
        }
        else
        {
          float* dph = hdst->f32;
          float* dpl = ldst->f32;
          float* sp = src->f32;
          int i = (int)width;
          for ( ; i > 8; i -= 16, sp += 16, dpl += 8, dph += 8)
          {
            __m256 a = _mm256_load_ps(sp);
            __m256 b = _mm256_load_ps(sp + 8);
            __m256 c = _mm256_permute2f128_ps(a, b, (2 << 4) | (0));
            __m256 d = _mm256_permute2f128_ps(a, b, (3 << 4) | (1));
            __m256 e = _mm256_shuffle_ps(c, d, _MM_SHUFFLE(2, 0, 2, 0));
            __m256 f = _mm256_shuffle_ps(c, d, _MM_SHUFFLE(3, 1, 3, 1));
            _mm256_store_ps(dpl, f);
            _mm256_store_ps(dph, e);
          }
          for (; i > 0; i -= 8, sp += 8, dpl += 4, dph += 4)
          {
            __m128 a = _mm_load_ps(sp);
            __m128 b = _mm_load_ps(sp + 4);
            __m128 c = _mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0));
            __m128 d = _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1));
            _mm_store_ps(dpl, d);
            _mm_store_ps(dph, c);
          }
        }

        // the actual horizontal transform
        float* hp = hdst->f32, * lp = ldst->f32;
        ui32 l_width = (width + (even ? 1 : 0)) >> 1;  // low pass
        ui32 h_width = (width + (even ? 0 : 1)) >> 1;  // high pass
        ui32 num_steps = atk->get_num_steps();
        for (ui32 j = num_steps; j > 0; --j)
        {
          // first lifting step
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
          float* dp;
          int i;
          __m256 factor;

          factor = _mm256_set1_ps(K_inv);
          dp = lp;
          i = (int)l_width;
          for ( ; i > 0; i -= 8, dp += 8)
          {
            __m256 s = _mm256_load_ps(dp);
            _mm256_store_ps(dp, _mm256_mul_ps(factor, s));
          }

          factor = _mm256_set1_ps(K);
          dp = hp;
          i = (int)h_width;
          for ( ; i > 0; i -= 8, dp += 8)
          {
            __m256 s = _mm256_load_ps(dp);
            _mm256_store_ps(dp, _mm256_mul_ps(factor, s));
          }
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
    void avx_irv_vert_syn_step(const lifting_step* s, const line_buf* aug, 
                               const line_buf* sig, const line_buf* other, 
                               ui32 repeat)
    {
      __m256 factor = _mm256_set1_ps(s->irv.Aatk);

      float* dst = aug->f32;
      const float* src1 = sig->f32, * src2 = other->f32;
      int i = (int)repeat;
      for ( ; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
      {
        __m256 s1 = _mm256_load_ps(src1);
        __m256 s2 = _mm256_load_ps(src2);
        __m256 d  = _mm256_load_ps(dst);
        d = _mm256_sub_ps(d, _mm256_mul_ps(factor, _mm256_add_ps(s1, s2)));
        _mm256_store_ps(dst, d);
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
          float* dp;
          int i;
          __m256 factor;

          factor = _mm256_set1_ps(K);
          dp = aug;
          i = (int)aug_width;
          for ( ; i > 0; i -= 8, dp += 8)
          {
            __m256 s = _mm256_load_ps(dp);
            _mm256_store_ps(dp, _mm256_mul_ps(factor, s));
          }

          factor = _mm256_set1_ps(K_inv);
          dp = oth;
          i = (int)oth_width;
          for ( ; i > 0; i -= 8, dp += 8)
          {
            __m256 s = _mm256_load_ps(dp);
            _mm256_store_ps(dp, _mm256_mul_ps(factor, s));
          }
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
        if (even)
        {
          float* sph = hsrc->f32;
          float* spl = lsrc->f32;
          float* dp = dst->f32;
          int i = (int)width;
          for ( ; i > 8; i -= 16, dp += 16, spl += 8, sph += 8)
          {
            __m256 a = _mm256_load_ps(spl);
            __m256 b = _mm256_load_ps(sph);
            __m256 c = _mm256_unpacklo_ps(a, b);
            __m256 d = _mm256_unpackhi_ps(a, b);
            __m256 e = _mm256_permute2f128_ps(c, d, (2 << 4) | (0));
            __m256 f = _mm256_permute2f128_ps(c, d, (3 << 4) | (1));
            _mm256_store_ps(dp, e);
            _mm256_store_ps(dp + 8, f);
          }
          for (; i > 0; i -= 8, dp += 8, spl += 4, sph += 4)
          {
            __m128 a = _mm_load_ps(spl);
            __m128 b = _mm_load_ps(sph);
            __m128 c = _mm_unpacklo_ps(a, b);
            __m128 d = _mm_unpackhi_ps(a, b);
            _mm_store_ps(dp, c);
            _mm_store_ps(dp + 4, d);
          }
        }
        else
        {
          float* sph = hsrc->f32;
          float* spl = lsrc->f32;
          float* dp = dst->f32;
          int i = (int)width;
          for (; i > 8; i -= 16, dp += 16, spl += 8, sph += 8)
          { // i>=8 because we can exceed the aligned buffer by up to 7
            __m256 a = _mm256_load_ps(spl);
            __m256 b = _mm256_load_ps(sph);
            __m256 c = _mm256_unpacklo_ps(b, a);
            __m256 d = _mm256_unpackhi_ps(b, a);
            __m256 e = _mm256_permute2f128_ps(c, d, (2 << 4) | (0));
            __m256 f = _mm256_permute2f128_ps(c, d, (3 << 4) | (1));
            _mm256_store_ps(dp, e);
            _mm256_store_ps(dp + 8, f);
          }
          for (; i > 0; i -= 8, dp += 8, spl += 4, sph += 4)
          {
            __m128 a = _mm_load_ps(spl);
            __m128 b = _mm_load_ps(sph);
            __m128 c = _mm_unpacklo_ps(b, a);
            __m128 d = _mm_unpackhi_ps(b, a);
            _mm_store_ps(dp, c);
            _mm_store_ps(dp + 4, d);
          }
        }
      }
      else {
        if (even)
          dst->f32[0] = lsrc->f32[0];
        else
          dst->f32[0] = hsrc->f32[0] * 0.5f;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat)
    {
      __m256 factor = _mm256_set1_ps(K);
      float* dst = aug->f32;
      int i = (int)repeat;
      for ( ; i > 0; i -= 8, dst += 8 )
      {
        __m256 s = _mm256_load_ps(dst);
        _mm256_store_ps(dst, _mm256_mul_ps(factor, s));
      }
    }


  } // !local
} // !ojph
