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
#include "ojph_params.h"
#include "../codestream/ojph_params_local.h"

#include "ojph_transform.h"
#include "ojph_transform_local.h"

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    void wasm_deinterleave(float* dpl, float* dph, float* sp, 
                           int width, bool even)
    {
      if (even)
        for (; width > 0; width -= 8, sp += 8, dpl += 4, dph += 4)
        {
          v128_t a = wasm_v128_load(sp);
          v128_t b = wasm_v128_load(sp + 4);
          v128_t c = wasm_i32x4_shuffle(a, b, 0, 2, 4 + 0, 4 + 2);
          v128_t d = wasm_i32x4_shuffle(a, b, 1, 3, 4 + 1, 4 + 3);
          // v128_t c = _mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0));
          // v128_t d = _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1));
          wasm_v128_store(dpl, c);
          wasm_v128_store(dph, d);
        }
      else
        for (; width > 0; width -= 8, sp += 8, dpl += 4, dph += 4)
        {
          v128_t a = wasm_v128_load(sp);
          v128_t b = wasm_v128_load(sp + 4);
          v128_t c = wasm_i32x4_shuffle(a, b, 0, 2, 4 + 0, 4 + 2);
          v128_t d = wasm_i32x4_shuffle(a, b, 1, 3, 4 + 1, 4 + 3);
          // v128_t c = _mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0));
          // v128_t d = _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1));
          wasm_v128_store(dpl, d);
          wasm_v128_store(dph, c);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_interleave(float* dp, float* spl, float* sph, 
                         int width, bool even)
    {
      if (even)
        for (; width > 0; width -= 8, dp += 8, spl += 4, sph += 4)
        {
          v128_t a = wasm_v128_load(spl);
          v128_t b = wasm_v128_load(sph);
          v128_t c = wasm_i32x4_shuffle(a, b, 0, 4 + 0, 1, 4 + 1);
          v128_t d = wasm_i32x4_shuffle(a, b, 2, 4 + 2, 3, 4 + 3);
          // v128_t c = _mm_unpacklo_ps(a, b);
          // v128_t d = _mm_unpackhi_ps(a, b);
          wasm_v128_store(dp, c);
          wasm_v128_store(dp + 4, d);
        }
      else
        for (; width > 0; width -= 8, dp += 8, spl += 4, sph += 4)
        {
          v128_t a = wasm_v128_load(spl);
          v128_t b = wasm_v128_load(sph);
          v128_t c = wasm_i32x4_shuffle(b, a, 0, 4 + 0, 1, 4 + 1);
          v128_t d = wasm_i32x4_shuffle(b, a, 2, 4 + 2, 3, 4 + 3);
          // v128_t c = _mm_unpacklo_ps(b, a);
          // v128_t d = _mm_unpackhi_ps(b, a);
          wasm_v128_store(dp, c);
          wasm_v128_store(dp + 4, d);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline void wasm_multiply_const(float* p, float f, int width)
    {
      v128_t factor = wasm_f32x4_splat(f);
      for (; width > 0; width -= 4, p += 4)
      {
        v128_t s = wasm_v128_load(p);
        wasm_v128_store(p, wasm_f32x4_mul(factor, s));
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_irv_vert_step(const lifting_step* s, const line_buf* sig, 
                            const line_buf* other, const line_buf* aug, 
                            ui32 repeat, bool synthesis)
    {
      float a = s->irv.Aatk;
      if (synthesis)
        a = -a;

      v128_t factor = wasm_f32x4_splat(a);

      float* dst = aug->f32;
      const float* src1 = sig->f32, * src2 = other->f32;
      int i = (int)repeat;
      for ( ; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
      {
        v128_t s1 = wasm_v128_load(src1);
        v128_t s2 = wasm_v128_load(src2);
        v128_t d  = wasm_v128_load(dst);
        d = wasm_f32x4_add(d, wasm_f32x4_mul(factor, wasm_f32x4_add(s1, s2)));
        wasm_v128_store(dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat)
    {
      wasm_multiply_const(aug->f32, K, (int)repeat);
    }

    /////////////////////////////////////////////////////////////////////////
    void wasm_irv_horz_ana(const param_atk* atk, const line_buf* ldst, 
                           const line_buf* hdst, const line_buf* src, 
                           ui32 width, bool even)
    {
      if (width > 1)
      {
        // split src into ldst and hdst
        wasm_deinterleave(ldst->f32, hdst->f32, src->f32, (int)width, even);

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
          v128_t f = wasm_f32x4_splat(a);
          if (even)
          {
            for (; i > 0; i -= 4, sp += 4, dp += 4)
            {
              v128_t m = wasm_v128_load(sp);
              v128_t n = wasm_v128_load(sp + 1);
              v128_t p = wasm_v128_load(dp);
              p = wasm_f32x4_add(p, wasm_f32x4_mul(f, wasm_f32x4_add(m, n)));
              wasm_v128_store(dp, p);
            }
          }
          else
          {
            for (; i > 0; i -= 4, sp += 4, dp += 4)
            {
              v128_t m = wasm_v128_load(sp);
              v128_t n = wasm_v128_load(sp - 1);
              v128_t p = wasm_v128_load(dp);
              p = wasm_f32x4_add(p, wasm_f32x4_mul(f, wasm_f32x4_add(m, n)));
              wasm_v128_store(dp, p);
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
          wasm_multiply_const(lp, K_inv, (int)l_width);
          wasm_multiply_const(hp, K, (int)h_width);
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
    void wasm_irv_horz_syn(const param_atk* atk, const line_buf* dst, 
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
          wasm_multiply_const(aug, K, (int)aug_width);
          wasm_multiply_const(oth, K_inv, (int)oth_width);
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
          v128_t f = wasm_f32x4_splat(a);
          if (ev)
          {
            for ( ; i > 0; i -= 4, sp += 4, dp += 4)
            {
              v128_t m = wasm_v128_load(sp);
              v128_t n = wasm_v128_load(sp - 1);
              v128_t p = wasm_v128_load(dp);
              p = wasm_f32x4_sub(p, wasm_f32x4_mul(f, wasm_f32x4_add(m, n)));
              wasm_v128_store(dp, p);
            }
          }
          else
          {
            for ( ; i > 0; i -= 4, sp += 4, dp += 4)
            {
              v128_t m = wasm_v128_load(sp);
              v128_t n = wasm_v128_load(sp + 1);
              v128_t p = wasm_v128_load(dp);
              p = wasm_f32x4_sub(p, wasm_f32x4_mul(f, wasm_f32x4_add(m, n)));
              wasm_v128_store(dp, p);
            }
          }

          // swap buffers
          float* t = aug; aug = oth; oth = t;
          ev = !ev;
          ui32 w = aug_width; aug_width = oth_width; oth_width = w;
        }

        // combine both lsrc and hsrc into dst
        wasm_interleave(dst->f32, lsrc->f32, hsrc->f32, (int)width, even);
      }
      else {
        if (even)
          dst->f32[0] = lsrc->f32[0];
        else
          dst->f32[0] = hsrc->f32[0] * 0.5f;
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void wasm_rev_vert_step(const lifting_step* s, const line_buf* sig, 
                            const line_buf* other, const line_buf* aug, 
                            ui32 repeat, bool synthesis)
    {
      const si32 a = s->rev.Aatk;
      const si32 b = s->rev.Batk;
      const ui32 e = s->rev.Eatk;
      v128_t va = wasm_i32x4_splat(a);
      v128_t vb = wasm_i32x4_splat(b);

      si32* dst = aug->i32;
      const si32* src1 = sig->i32, * src2 = other->i32;
      // The general definition of the wavelet in Part 2 is slightly 
      // different to part 2, although they are mathematically equivalent
      // here, we identify the simpler form from Part 1 and employ them
      if (a == 1)
      { // 5/3 update and any case with a == 1
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = wasm_v128_load((v128_t*)src1);
            v128_t s2 = wasm_v128_load((v128_t*)src2);
            v128_t d = wasm_v128_load((v128_t*)dst);
            v128_t t = wasm_i32x4_add(s1, s2);
            v128_t v = wasm_i32x4_add(vb, t);
            v128_t w = wasm_i32x4_shr(v, e);
            d = wasm_i32x4_sub(d, w);
            wasm_v128_store((v128_t*)dst, d);
          }
        else
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = wasm_v128_load((v128_t*)src1);
            v128_t s2 = wasm_v128_load((v128_t*)src2);
            v128_t d = wasm_v128_load((v128_t*)dst);
            v128_t t = wasm_i32x4_add(s1, s2);
            v128_t v = wasm_i32x4_add(vb, t);
            v128_t w = wasm_i32x4_shr(v, e);
            d = wasm_i32x4_add(d, w);
            wasm_v128_store((v128_t*)dst, d);
          }
      }
      else if (a == -1 && b == 1 && e == 1)
      { // 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = wasm_v128_load((v128_t*)src1);
            v128_t s2 = wasm_v128_load((v128_t*)src2);
            v128_t d = wasm_v128_load((v128_t*)dst);
            v128_t t = wasm_i32x4_add(s1, s2);
            v128_t w = wasm_i32x4_shr(t, e);
            d = wasm_i32x4_add(d, w);
            wasm_v128_store((v128_t*)dst, d);
          }
        else
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = wasm_v128_load((v128_t*)src1);
            v128_t s2 = wasm_v128_load((v128_t*)src2);
            v128_t d = wasm_v128_load((v128_t*)dst);
            v128_t t = wasm_i32x4_add(s1, s2);
            v128_t w = wasm_i32x4_shr(t, e);
            d = wasm_i32x4_sub(d, w);
            wasm_v128_store((v128_t*)dst, d);
          }
      }
      else if (a == -1)
      { // any case with a == -1, which is not 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = wasm_v128_load((v128_t*)src1);
            v128_t s2 = wasm_v128_load((v128_t*)src2);
            v128_t d = wasm_v128_load((v128_t*)dst);
            v128_t t = wasm_i32x4_add(s1, s2);
            v128_t v = wasm_i32x4_sub(vb, t);
            v128_t w = wasm_i32x4_shr(v, e);
            d = wasm_i32x4_sub(d, w);
            wasm_v128_store((v128_t*)dst, d);
          }
        else
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = wasm_v128_load((v128_t*)src1);
            v128_t s2 = wasm_v128_load((v128_t*)src2);
            v128_t d = wasm_v128_load((v128_t*)dst);
            v128_t t = wasm_i32x4_add(s1, s2);
            v128_t v = wasm_i32x4_sub(vb, t);
            v128_t w = wasm_i32x4_shr(v, e);
            d = wasm_i32x4_add(d, w);
            wasm_v128_store((v128_t*)dst, d);
          }
      }
      else 
      { // general case
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = wasm_v128_load((v128_t*)src1);
            v128_t s2 = wasm_v128_load((v128_t*)src2);
            v128_t d = wasm_v128_load((v128_t*)dst);
            v128_t t = wasm_i32x4_add(s1, s2);
            v128_t u = wasm_i32x4_mul(va, t);
            v128_t v = wasm_i32x4_add(vb, u);
            v128_t w = wasm_i32x4_shr(v, e);
            d = wasm_i32x4_sub(d, w);
            wasm_v128_store((v128_t*)dst, d);
          }
        else
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = wasm_v128_load((v128_t*)src1);
            v128_t s2 = wasm_v128_load((v128_t*)src2);
            v128_t d = wasm_v128_load((v128_t*)dst);
            v128_t t = wasm_i32x4_add(s1, s2);
            v128_t u = wasm_i32x4_mul(va, t);
            v128_t v = wasm_i32x4_add(vb, u);
            v128_t w = wasm_i32x4_shr(v, e);
            d = wasm_i32x4_add(d, w);
            wasm_v128_store((v128_t*)dst, d);
          }
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void wasm_rev_horz_ana(const param_atk* atk, const line_buf* ldst, 
                           const line_buf* hdst, const line_buf* src, 
                           ui32 width, bool even)
    {
      if (width > 1)
      {
        // combine both lsrc and hsrc into dst
        wasm_deinterleave(ldst->f32, hdst->f32, src->f32, (int)width, even);          

        si32* hp = hdst->i32, * lp = ldst->i32;
        ui32 l_width = (width + (even ? 1 : 0)) >> 1;  // low pass
        ui32 h_width = (width + (even ? 0 : 1)) >> 1;  // high pass
        ui32 num_steps = atk->get_num_steps();
        for (ui32 j = num_steps; j > 0; --j)
        {
          // first lifting step
          const lifting_step* s = atk->get_step(j - 1);
          const si32 a = s->rev.Aatk;
          const si32 b = s->rev.Batk;
          const ui32 e = s->rev.Eatk;
          v128_t va = wasm_i32x4_splat(a);
          v128_t vb = wasm_i32x4_splat(b);

          // extension
          lp[-1] = lp[0];
          lp[l_width] = lp[l_width - 1];
          // lifting step
          const si32* sp = lp;
          si32* dp = hp;
          if (a == 1)
          { // 5/3 update and any case with a == 1
            int i = (int)h_width;
            if (even)
            {
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp + 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t v = wasm_i32x4_add(vb, t);
                v128_t w = wasm_i32x4_shr(v, e);
                d = wasm_i32x4_add(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp - 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t v = wasm_i32x4_add(vb, t);
                v128_t w = wasm_i32x4_shr(v, e);
                d = wasm_i32x4_add(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp + 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t w = wasm_i32x4_shr(t, e);
                d = wasm_i32x4_sub(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp - 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t w = wasm_i32x4_shr(t, e);
                d = wasm_i32x4_sub(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp + 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t v = wasm_i32x4_sub(vb, t);
                v128_t w = wasm_i32x4_shr(v, e);
                d = wasm_i32x4_add(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp - 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t v = wasm_i32x4_sub(vb, t);
                v128_t w = wasm_i32x4_shr(v, e);
                d = wasm_i32x4_add(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
          }
          else 
          { // general case
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp + 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t u = wasm_i32x4_mul(va, t);
                v128_t v = wasm_i32x4_add(vb, u);
                v128_t w = wasm_i32x4_shr(v, e);
                d = wasm_i32x4_add(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp - 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t u = wasm_i32x4_mul(va, t);                
                v128_t v = wasm_i32x4_add(vb, u);
                v128_t w = wasm_i32x4_shr(v, e);
                d = wasm_i32x4_add(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
          }

          // swap buffers
          si32* t = lp; lp = hp; hp = t;
          even = !even;
          ui32 w = l_width; l_width = h_width; h_width = w;
        }
      }
      else {
        if (even)
          ldst->i32[0] = src->i32[0];
        else
          hdst->i32[0] = src->i32[0] << 1;
      }
    }
    
    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_horz_syn(const param_atk* atk, const line_buf* dst, 
                           const line_buf* lsrc, const line_buf* hsrc, 
                           ui32 width, bool even)
    {
      if (width > 1)
      {
        bool ev = even;
        si32* oth = hsrc->i32, * aug = lsrc->i32;
        ui32 aug_width = (width + (even ? 1 : 0)) >> 1;  // low pass
        ui32 oth_width = (width + (even ? 0 : 1)) >> 1;  // high pass
        ui32 num_steps = atk->get_num_steps();
        for (ui32 j = 0; j < num_steps; ++j)
        {
          const lifting_step* s = atk->get_step(j);
          const si32 a = s->rev.Aatk;
          const si32 b = s->rev.Batk;
          const ui32 e = s->rev.Eatk;
          v128_t va = wasm_i32x4_splat(a);
          v128_t vb = wasm_i32x4_splat(b);

          // extension
          oth[-1] = oth[0];
          oth[oth_width] = oth[oth_width - 1];
          // lifting step
          const si32* sp = oth;
          si32* dp = aug;
          if (a == 1)
          { // 5/3 update and any case with a == 1
            int i = (int)aug_width;
            if (ev)
            {
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp - 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t v = wasm_i32x4_add(vb, t);
                v128_t w = wasm_i32x4_shr(v, e);
                d = wasm_i32x4_sub(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp + 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t v = wasm_i32x4_add(vb, t);
                v128_t w = wasm_i32x4_shr(v, e);
                d = wasm_i32x4_sub(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp - 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t w = wasm_i32x4_shr(t, e);
                d = wasm_i32x4_add(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp + 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t w = wasm_i32x4_shr(t, e);
                d = wasm_i32x4_add(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp - 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t v = wasm_i32x4_sub(vb, t);
                v128_t w = wasm_i32x4_shr(v, e);
                d = wasm_i32x4_sub(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp + 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t v = wasm_i32x4_sub(vb, t);
                v128_t w = wasm_i32x4_shr(v, e);
                d = wasm_i32x4_sub(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
          }
          else 
          { // general case
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp - 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t u = wasm_i32x4_mul(va, t);
                v128_t v = wasm_i32x4_add(vb, u);
                v128_t w = wasm_i32x4_shr(v, e);
                d = wasm_i32x4_sub(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = wasm_v128_load((v128_t*)sp);
                v128_t s2 = wasm_v128_load((v128_t*)(sp + 1));
                v128_t d = wasm_v128_load((v128_t*)dp);
                v128_t t = wasm_i32x4_add(s1, s2);
                v128_t u = wasm_i32x4_mul(va, t);                
                v128_t v = wasm_i32x4_add(vb, u);
                v128_t w = wasm_i32x4_shr(v, e);
                d = wasm_i32x4_sub(d, w);
                wasm_v128_store((v128_t*)dp, d);
              }
          }

          // swap buffers
          si32* t = aug; aug = oth; oth = t;
          ev = !ev;
          ui32 w = aug_width; aug_width = oth_width; oth_width = w;
        }

        // combine both lsrc and hsrc into dst
        wasm_interleave(dst->f32, lsrc->f32, hsrc->f32, (int)width, even);
      }
      else {
        if (even)
          dst->i32[0] = lsrc->i32[0];
        else
          dst->i32[0] = hsrc->i32[0] >> 1;
      }
    }
    
  } // !local
} // !ojph
