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
#include "ojph_params.h"
#include "../codestream/ojph_params_local.h"

#include "ojph_transform.h"
#include "ojph_transform_local.h"

#include <immintrin.h>

namespace ojph {
  namespace local {

    /////////////////////////////////////////////////////////////////////////
    void avx2_rev_vert_step(const lifting_step* s, const line_buf* sig, 
                            const line_buf* other, const line_buf* aug, 
                            ui32 repeat, bool synthesis)
    {
      const si32 a = s->rev.Aatk;
      const si32 b = s->rev.Batk;
      const si32 e = s->rev.Eatk;
      __m256i va = _mm256_set1_epi32(a);
      __m256i vb = _mm256_set1_epi32(b);

      si32* dst = aug->i32;
      const si32* src1 = sig->i32, * src2 = other->i32;
      // The general definition of the wavelet in Part 2 is slightly 
      // different to part 2, although they are mathematically equivalent
      // here, we identify the simpler form from Part 1 and employ them
      if (a == 1)
      { // 5/3 update and any case with a == 1
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i v = _mm256_add_epi32(vb, t);
            __m256i w = _mm256_srai_epi32(v, e);
            d = _mm256_sub_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
        else
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i v = _mm256_add_epi32(vb, t);
            __m256i w = _mm256_srai_epi32(v, e);
            d = _mm256_add_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
      }
      else if (a == -1 && b == 1 && e == 1)
      { // 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i w = _mm256_srai_epi32(t, e);
            d = _mm256_add_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
        else
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i w = _mm256_srai_epi32(t, e);
            d = _mm256_sub_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
      }
      else if (a == -1)
      { // any case with a == -1, which is not 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i v = _mm256_sub_epi32(vb, t);
            __m256i w = _mm256_srai_epi32(v, e);
            d = _mm256_sub_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
        else
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i v = _mm256_sub_epi32(vb, t);
            __m256i w = _mm256_srai_epi32(v, e);
            d = _mm256_add_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
      }
      else { // general case
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i u = _mm256_mullo_epi32(va, t);
            __m256i v = _mm256_add_epi32(vb, u);
            __m256i w = _mm256_srai_epi32(v, e);
            d = _mm256_sub_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
        else
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i u = _mm256_mullo_epi32(va, t);
            __m256i v = _mm256_add_epi32(vb, u);
            __m256i w = _mm256_srai_epi32(v, e);
            d = _mm256_add_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void avx2_rev_horz_ana(const param_atk* atk, const line_buf* ldst, 
                           const line_buf* hdst, const line_buf* src, 
                           ui32 width, bool even)
    {
      if (width > 1)
      {
        // combine both lsrc and hsrc into dst
        {
          float* dpl = ldst->f32;
          float* dph = hdst->f32;
          float* sp = src->f32;
          int w = (int)width;
          AVX_DEINTERLEAVE(dpl, dph, sp, w, even);
        }

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
          const si32 e = s->rev.Eatk;
          __m256i va = _mm256_set1_epi32(a);
          __m256i vb = _mm256_set1_epi32(b);

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
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_add_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_add_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i w = _mm256_srai_epi32(t, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i w = _mm256_srai_epi32(t, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_sub_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_sub_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
          }
          else {
            // general case
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i u = _mm256_mullo_epi32(va, t);
                __m256i v = _mm256_add_epi32(vb, u);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i u = _mm256_mullo_epi32(va, t);
                __m256i v = _mm256_add_epi32(vb, u);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
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
    void avx2_rev_horz_syn(const param_atk* atk, const line_buf* dst, 
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
          const si32 e = s->rev.Eatk;
          __m256i va = _mm256_set1_epi32(a);
          __m256i vb = _mm256_set1_epi32(b);

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
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_add_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_add_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i w = _mm256_srai_epi32(t, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i w = _mm256_srai_epi32(t, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_sub_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_sub_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
          }
          else {
            // general case
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i u = _mm256_mullo_epi32(va, t);
                __m256i v = _mm256_add_epi32(vb, u);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i u = _mm256_mullo_epi32(va, t);
                __m256i v = _mm256_add_epi32(vb, u);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
          }

          // swap buffers
          si32* t = aug; aug = oth; oth = t;
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
          dst->i32[0] = lsrc->i32[0];
        else
          dst->i32[0] = hsrc->i32[0] >> 1;
      }
    }

  } // !local
} // !ojph
