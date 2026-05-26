//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2019, Aous Naman
// Copyright (c) 2019, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2019, The University of New South Wales, Australia
// Copyright (c) 2026, Osamu Watanabe
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
// File: ojph_colour_avx512.cpp
// Date: 21 May 2026
//***************************************************************************/

#include "ojph_arch.h"
#if defined(OJPH_ARCH_X86_64)

#include <climits>
#include <cmath>

#include "ojph_defs.h"
#include "ojph_mem.h"
#include "ojph_colour.h"

#include <immintrin.h>

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    void avx512_rev_convert(const line_buf *src_line,
                            const ui32 src_line_offset,
                            line_buf *dst_line,
                            const ui32 dst_line_offset,
                            si64 shift, ui32 width)
    {
      if (src_line->flags & line_buf::LFT_32BIT)
      {
        if (dst_line->flags & line_buf::LFT_32BIT)
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si32 *dp = dst_line->i32 + dst_line_offset;
          __m512i sh = _mm512_set1_epi32((si32)shift);
          for (int i = (width + 15) >> 4; i > 0; --i, sp += 16, dp += 16)
          {
            __m512i s = _mm512_loadu_si512((__m512i*)sp);
            s = _mm512_add_epi32(s, sh);
            _mm512_storeu_si512((__m512i*)dp, s);
          }
        }
        else
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si64 *dp = dst_line->i64 + dst_line_offset;
          __m512i sh = _mm512_set1_epi64(shift);
          for (int i = (width + 15) >> 4; i > 0; --i, sp += 16, dp += 16)
          {
            __m512i s = _mm512_loadu_si512((__m512i*)sp);

            __m512i t = _mm512_cvtepi32_epi64(
              _mm512_castsi512_si256(s));
            t = _mm512_add_epi64(t, sh);
            _mm512_storeu_si512((__m512i*)dp, t);

            t = _mm512_cvtepi32_epi64(
              _mm512_extracti64x4_epi64(s, 1));
            t = _mm512_add_epi64(t, sh);
            _mm512_storeu_si512((__m512i*)(dp + 8), t);
          }
        }
      }
      else
      {
        assert(src_line->flags | line_buf::LFT_64BIT);
        assert(dst_line->flags | line_buf::LFT_32BIT);
        const si64 *sp = src_line->i64 + src_line_offset;
        si32 *dp = dst_line->i32 + dst_line_offset;
        __m512i sh = _mm512_set1_epi64(shift);
        for (int i = (width + 15) >> 4; i > 0; --i, sp += 16, dp += 16)
        {
          __m512i s0 = _mm512_loadu_si512((__m512i*)sp);
          s0 = _mm512_add_epi64(s0, sh);
          __m512i s1 = _mm512_loadu_si512((__m512i*)(sp + 8));
          s1 = _mm512_add_epi64(s1, sh);

          __m256i lo = _mm512_cvtepi64_epi32(s0);
          __m256i hi = _mm512_cvtepi64_epi32(s1);
          __m512i t = _mm512_castsi256_si512(lo);
          t = _mm512_inserti64x4(t, hi, 1);
          _mm512_storeu_si512((__m512i*)dp, t);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx512_rev_convert_nlt_type3(const line_buf *src_line,
                                      const ui32 src_line_offset,
                                      line_buf *dst_line,
                                      const ui32 dst_line_offset,
                                      si64 shift, ui32 width)
    {
      if (src_line->flags & line_buf::LFT_32BIT)
      {
        if (dst_line->flags & line_buf::LFT_32BIT)
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si32 *dp = dst_line->i32 + dst_line_offset;
          __m512i sh = _mm512_set1_epi32((si32)(-shift));
          __m512i zero = _mm512_setzero_si512();
          for (int i = (width + 15) >> 4; i > 0; --i, sp += 16, dp += 16)
          {
            __m512i s = _mm512_loadu_si512((__m512i*)sp);
            __mmask16 neg = _mm512_cmpgt_epi32_mask(zero, s);
            __m512i v_m_sh = _mm512_sub_epi32(sh, s);
            __m512i r = _mm512_mask_mov_epi32(s, neg, v_m_sh);
            _mm512_storeu_si512((__m512i*)dp, r);
          }
        }
        else
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si64 *dp = dst_line->i64 + dst_line_offset;
          __m512i sh = _mm512_set1_epi64(-shift);
          __m512i zero = _mm512_setzero_si512();
          for (int i = (width + 15) >> 4; i > 0; --i, sp += 16, dp += 16)
          {
            __m512i s = _mm512_loadu_si512((__m512i*)sp);

            __m512i t = _mm512_cvtepi32_epi64(
              _mm512_castsi512_si256(s));
            __mmask8 neg = _mm512_cmpgt_epi64_mask(zero, t);
            __m512i v_m_sh = _mm512_sub_epi64(sh, t);
            t = _mm512_mask_mov_epi64(t, neg, v_m_sh);
            _mm512_storeu_si512((__m512i*)dp, t);

            t = _mm512_cvtepi32_epi64(
              _mm512_extracti64x4_epi64(s, 1));
            neg = _mm512_cmpgt_epi64_mask(zero, t);
            v_m_sh = _mm512_sub_epi64(sh, t);
            t = _mm512_mask_mov_epi64(t, neg, v_m_sh);
            _mm512_storeu_si512((__m512i*)(dp + 8), t);
          }
        }
      }
      else
      {
        assert(src_line->flags | line_buf::LFT_64BIT);
        assert(dst_line->flags | line_buf::LFT_32BIT);
        const si64 *sp = src_line->i64 + src_line_offset;
        si32 *dp = dst_line->i32 + dst_line_offset;
        __m512i sh = _mm512_set1_epi64(-shift);
        __m512i zero = _mm512_setzero_si512();
        for (int i = (width + 15) >> 4; i > 0; --i, sp += 16, dp += 16)
        {
          __m512i s0 = _mm512_loadu_si512((__m512i*)sp);
          __mmask8 neg = _mm512_cmpgt_epi64_mask(zero, s0);
          __m512i v = _mm512_sub_epi64(sh, s0);
          s0 = _mm512_mask_mov_epi64(s0, neg, v);

          __m512i s1 = _mm512_loadu_si512((__m512i*)(sp + 8));
          neg = _mm512_cmpgt_epi64_mask(zero, s1);
          v = _mm512_sub_epi64(sh, s1);
          s1 = _mm512_mask_mov_epi64(s1, neg, v);

          __m256i lo = _mm512_cvtepi64_epi32(s0);
          __m256i hi = _mm512_cvtepi64_epi32(s1);
          __m512i t = _mm512_castsi256_si512(lo);
          t = _mm512_inserti64x4(t, hi, 1);
          _mm512_storeu_si512((__m512i*)dp, t);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    template<bool NLT_TYPE3>
    static inline
    void local_avx512_irv_convert_to_integer(const line_buf *src_line,
      line_buf *dst_line, ui32 dst_line_offset,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      assert((src_line->flags & line_buf::LFT_32BIT) &&
             (src_line->flags & line_buf::LFT_INTEGER) == 0 &&
             (dst_line->flags & line_buf::LFT_32BIT) &&
             (dst_line->flags & line_buf::LFT_INTEGER));

      assert(bit_depth <= 32);
      const float* sp = src_line->f32;
      si32* dp = dst_line->i32 + dst_line_offset;
      si32 neg_limit = (si32)INT_MIN >> (32 - bit_depth);
      __m512 mul = _mm512_set1_ps((float)(1ull << bit_depth));
      __m512 fl_up_lim = _mm512_set1_ps(-(float)neg_limit);
      __m512 fl_low_lim = _mm512_set1_ps((float)neg_limit);
      __m512i s32_up_lim = _mm512_set1_epi32(INT_MAX >> (32 - bit_depth));
      __m512i s32_low_lim = _mm512_set1_epi32(INT_MIN >> (32 - bit_depth));

      if (is_signed)
      {
        __m512i zero = _mm512_setzero_si512();
        __m512i bias =
          _mm512_set1_epi32(-(si32)((1ULL << (bit_depth - 1)) + 1));
        for (int i = (int)width; i > 0; i -= 16, sp += 16, dp += 16) {
          __m512 t = _mm512_loadu_ps(sp);
          t = _mm512_mul_ps(t, mul);
          __m512i u = _mm512_cvtps_epi32(t);
          __mmask16 ge = _mm512_cmp_ps_mask(t, fl_low_lim, _CMP_NLT_UQ);
          u = _mm512_mask_mov_epi32(s32_low_lim, ge, u);
          __mmask16 lt = _mm512_cmp_ps_mask(t, fl_up_lim, _CMP_NGE_UQ);
          u = _mm512_mask_mov_epi32(s32_up_lim, lt, u);
          if (NLT_TYPE3)
          {
            __mmask16 neg = _mm512_cmpgt_epi32_mask(zero, u);
            __m512i nv = _mm512_sub_epi32(bias, u);
            u = _mm512_mask_mov_epi32(u, neg, nv);
          }
          _mm512_storeu_si512((__m512i*)dp, u);
        }
      }
      else
      {
        __m512i half = _mm512_set1_epi32((si32)(1ULL << (bit_depth - 1)));
        for (int i = (int)width; i > 0; i -= 16, sp += 16, dp += 16) {
          __m512 t = _mm512_loadu_ps(sp);
          t = _mm512_mul_ps(t, mul);
          __m512i u = _mm512_cvtps_epi32(t);
          __mmask16 ge = _mm512_cmp_ps_mask(t, fl_low_lim, _CMP_NLT_UQ);
          u = _mm512_mask_mov_epi32(s32_low_lim, ge, u);
          __mmask16 lt = _mm512_cmp_ps_mask(t, fl_up_lim, _CMP_NGE_UQ);
          u = _mm512_mask_mov_epi32(s32_up_lim, lt, u);
          u = _mm512_add_epi32(u, half);
          _mm512_storeu_si512((__m512i*)dp, u);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx512_irv_convert_to_integer(const line_buf *src_line,
      line_buf *dst_line, ui32 dst_line_offset,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_avx512_irv_convert_to_integer<false>(src_line, dst_line,
        dst_line_offset, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    void avx512_irv_convert_to_integer_nlt_type3(const line_buf *src_line,
      line_buf *dst_line, ui32 dst_line_offset,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_avx512_irv_convert_to_integer<true>(src_line, dst_line,
        dst_line_offset, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    template<bool NLT_TYPE3>
    static inline
    void local_avx512_irv_convert_to_float(const line_buf *src_line,
      ui32 src_line_offset, line_buf *dst_line,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      assert((src_line->flags & line_buf::LFT_32BIT) &&
             (src_line->flags & line_buf::LFT_INTEGER) &&
             (dst_line->flags & line_buf::LFT_32BIT) &&
             (dst_line->flags & line_buf::LFT_INTEGER) == 0);

      assert(bit_depth <= 32);
      __m512 mul = _mm512_set1_ps((float)(1.0 / (double)(1ULL << bit_depth)));

      const si32* sp = src_line->i32 + src_line_offset;
      float* dp = dst_line->f32;
      if (is_signed)
      {
        __m512i zero = _mm512_setzero_si512();
        __m512i bias =
          _mm512_set1_epi32(-(si32)((1ULL << (bit_depth - 1)) + 1));
        for (int i = (int)width; i > 0; i -= 16, sp += 16, dp += 16) {
          __m512i t = _mm512_loadu_si512((__m512i*)sp);
          if (NLT_TYPE3)
          {
            __mmask16 neg = _mm512_cmpgt_epi32_mask(zero, t);
            __m512i nv = _mm512_sub_epi32(bias, t);
            t = _mm512_mask_mov_epi32(t, neg, nv);
          }
          __m512 v = _mm512_cvtepi32_ps(t);
          v = _mm512_mul_ps(v, mul);
          _mm512_storeu_ps(dp, v);
        }
      }
      else
      {
        __m512i half = _mm512_set1_epi32((si32)(1ULL << (bit_depth - 1)));
        for (int i = (int)width; i > 0; i -= 16, sp += 16, dp += 16) {
          __m512i t = _mm512_loadu_si512((__m512i*)sp);
          t = _mm512_sub_epi32(t, half);
          __m512 v = _mm512_cvtepi32_ps(t);
          v = _mm512_mul_ps(v, mul);
          _mm512_storeu_ps(dp, v);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx512_irv_convert_to_float(const line_buf *src_line,
      ui32 src_line_offset, line_buf *dst_line,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_avx512_irv_convert_to_float<false>(src_line, src_line_offset,
        dst_line, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    void avx512_irv_convert_to_float_nlt_type3(const line_buf *src_line,
      ui32 src_line_offset, line_buf *dst_line,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_avx512_irv_convert_to_float<true>(src_line, src_line_offset,
        dst_line, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    void avx512_rct_forward(const line_buf *r,
                            const line_buf *g,
                            const line_buf *b,
                            line_buf *y, line_buf *cb, line_buf *cr,
                            ui32 repeat)
    {
      assert((y->flags  & line_buf::LFT_INTEGER) &&
             (cb->flags & line_buf::LFT_INTEGER) &&
             (cr->flags & line_buf::LFT_INTEGER) &&
             (r->flags  & line_buf::LFT_INTEGER) &&
             (g->flags  & line_buf::LFT_INTEGER) &&
             (b->flags  & line_buf::LFT_INTEGER));

      if (y->flags & line_buf::LFT_32BIT)
      {
        assert((y->flags  & line_buf::LFT_32BIT) &&
               (cb->flags & line_buf::LFT_32BIT) &&
               (cr->flags & line_buf::LFT_32BIT) &&
               (r->flags  & line_buf::LFT_32BIT) &&
               (g->flags  & line_buf::LFT_32BIT) &&
               (b->flags  & line_buf::LFT_32BIT));
        const si32 *rp = r->i32, *gp = g->i32, *bp = b->i32;
        si32 *yp = y->i32, *cbp = cb->i32, *crp = cr->i32;
        for (int i = (repeat + 15) >> 4; i > 0; --i)
        {
          __m512i mr = _mm512_load_si512((__m512i*)rp);
          __m512i mg = _mm512_load_si512((__m512i*)gp);
          __m512i mb = _mm512_load_si512((__m512i*)bp);
          __m512i t = _mm512_add_epi32(mr, mb);
          t = _mm512_add_epi32(t, _mm512_slli_epi32(mg, 1));
          _mm512_store_si512((__m512i*)yp, _mm512_srai_epi32(t, 2));
          t = _mm512_sub_epi32(mb, mg);
          _mm512_store_si512((__m512i*)cbp, t);
          t = _mm512_sub_epi32(mr, mg);
          _mm512_store_si512((__m512i*)crp, t);

          rp += 16; gp += 16; bp += 16;
          yp += 16; cbp += 16; crp += 16;
        }
      }
      else
      {
        assert((y->flags  & line_buf::LFT_64BIT) &&
               (cb->flags & line_buf::LFT_64BIT) &&
               (cr->flags & line_buf::LFT_64BIT) &&
               (r->flags  & line_buf::LFT_32BIT) &&
               (g->flags  & line_buf::LFT_32BIT) &&
               (b->flags  & line_buf::LFT_32BIT));
        const si32 *rp = r->i32, *gp = g->i32, *bp = b->i32;
        si64 *yp = y->i64, *cbp = cb->i64, *crp = cr->i64;
        for (int i = (repeat + 15) >> 4; i > 0; --i)
        {
          __m512i mr32 = _mm512_load_si512((__m512i*)rp);
          __m512i mg32 = _mm512_load_si512((__m512i*)gp);
          __m512i mb32 = _mm512_load_si512((__m512i*)bp);
          __m512i mr, mg, mb, t;

          mr = _mm512_cvtepi32_epi64(_mm512_castsi512_si256(mr32));
          mg = _mm512_cvtepi32_epi64(_mm512_castsi512_si256(mg32));
          mb = _mm512_cvtepi32_epi64(_mm512_castsi512_si256(mb32));

          t = _mm512_add_epi64(mr, mb);
          t = _mm512_add_epi64(t, _mm512_slli_epi64(mg, 1));
          _mm512_store_si512((__m512i*)yp, _mm512_srai_epi64(t, 2));
          t = _mm512_sub_epi64(mb, mg);
          _mm512_store_si512((__m512i*)cbp, t);
          t = _mm512_sub_epi64(mr, mg);
          _mm512_store_si512((__m512i*)crp, t);

          yp += 8; cbp += 8; crp += 8;

          mr = _mm512_cvtepi32_epi64(
            _mm512_extracti64x4_epi64(mr32, 1));
          mg = _mm512_cvtepi32_epi64(
            _mm512_extracti64x4_epi64(mg32, 1));
          mb = _mm512_cvtepi32_epi64(
            _mm512_extracti64x4_epi64(mb32, 1));

          t = _mm512_add_epi64(mr, mb);
          t = _mm512_add_epi64(t, _mm512_slli_epi64(mg, 1));
          _mm512_store_si512((__m512i*)yp, _mm512_srai_epi64(t, 2));
          t = _mm512_sub_epi64(mb, mg);
          _mm512_store_si512((__m512i*)cbp, t);
          t = _mm512_sub_epi64(mr, mg);
          _mm512_store_si512((__m512i*)crp, t);

          rp += 16; gp += 16; bp += 16;
          yp += 8; cbp += 8; crp += 8;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx512_rct_backward(const line_buf *y,
                             const line_buf *cb,
                             const line_buf *cr,
                             line_buf *r, line_buf *g, line_buf *b,
                             ui32 repeat)
    {
      assert((y->flags  & line_buf::LFT_INTEGER) &&
             (cb->flags & line_buf::LFT_INTEGER) &&
             (cr->flags & line_buf::LFT_INTEGER) &&
             (r->flags  & line_buf::LFT_INTEGER) &&
             (g->flags  & line_buf::LFT_INTEGER) &&
             (b->flags  & line_buf::LFT_INTEGER));

      if (y->flags & line_buf::LFT_32BIT)
      {
        assert((y->flags  & line_buf::LFT_32BIT) &&
               (cb->flags & line_buf::LFT_32BIT) &&
               (cr->flags & line_buf::LFT_32BIT) &&
               (r->flags  & line_buf::LFT_32BIT) &&
               (g->flags  & line_buf::LFT_32BIT) &&
               (b->flags  & line_buf::LFT_32BIT));
        const si32 *yp = y->i32, *cbp = cb->i32, *crp = cr->i32;
        si32 *rp = r->i32, *gp = g->i32, *bp = b->i32;
        for (int i = (repeat + 15) >> 4; i > 0; --i)
        {
          __m512i my  = _mm512_load_si512((__m512i*)yp);
          __m512i mcb = _mm512_load_si512((__m512i*)cbp);
          __m512i mcr = _mm512_load_si512((__m512i*)crp);

          __m512i t = _mm512_add_epi32(mcb, mcr);
          t = _mm512_sub_epi32(my, _mm512_srai_epi32(t, 2));
          _mm512_store_si512((__m512i*)gp, t);
          __m512i u = _mm512_add_epi32(mcb, t);
          _mm512_store_si512((__m512i*)bp, u);
          u = _mm512_add_epi32(mcr, t);
          _mm512_store_si512((__m512i*)rp, u);

          yp += 16; cbp += 16; crp += 16;
          rp += 16; gp += 16; bp += 16;
        }
      }
      else
      {
        assert((y->flags  & line_buf::LFT_64BIT) &&
               (cb->flags & line_buf::LFT_64BIT) &&
               (cr->flags & line_buf::LFT_64BIT) &&
               (r->flags  & line_buf::LFT_32BIT) &&
               (g->flags  & line_buf::LFT_32BIT) &&
               (b->flags  & line_buf::LFT_32BIT));
        const si64 *yp = y->i64, *cbp = cb->i64, *crp = cr->i64;
        si32 *rp = r->i32, *gp = g->i32, *bp = b->i32;
        for (int i = (repeat + 15) >> 4; i > 0; --i)
        {
          __m512i my, mcb, mcr, tg, tb, tr;

          my  = _mm512_load_si512((__m512i*)yp);
          mcb = _mm512_load_si512((__m512i*)cbp);
          mcr = _mm512_load_si512((__m512i*)crp);

          tg = _mm512_add_epi64(mcb, mcr);
          tg = _mm512_sub_epi64(my, _mm512_srai_epi64(tg, 2));
          tb = _mm512_add_epi64(mcb, tg);
          tr = _mm512_add_epi64(mcr, tg);

          __m256i r_lo = _mm512_cvtepi64_epi32(tr);
          __m256i g_lo = _mm512_cvtepi64_epi32(tg);
          __m256i b_lo = _mm512_cvtepi64_epi32(tb);

          yp += 8; cbp += 8; crp += 8;

          my  = _mm512_load_si512((__m512i*)yp);
          mcb = _mm512_load_si512((__m512i*)cbp);
          mcr = _mm512_load_si512((__m512i*)crp);

          tg = _mm512_add_epi64(mcb, mcr);
          tg = _mm512_sub_epi64(my, _mm512_srai_epi64(tg, 2));
          tb = _mm512_add_epi64(mcb, tg);
          tr = _mm512_add_epi64(mcr, tg);

          __m256i r_hi = _mm512_cvtepi64_epi32(tr);
          __m256i g_hi = _mm512_cvtepi64_epi32(tg);
          __m256i b_hi = _mm512_cvtepi64_epi32(tb);

          __m512i mr = _mm512_castsi256_si512(r_lo);
          mr = _mm512_inserti64x4(mr, r_hi, 1);
          _mm512_store_si512((__m512i*)rp, mr);

          __m512i mg = _mm512_castsi256_si512(g_lo);
          mg = _mm512_inserti64x4(mg, g_hi, 1);
          _mm512_store_si512((__m512i*)gp, mg);

          __m512i mb = _mm512_castsi256_si512(b_lo);
          mb = _mm512_inserti64x4(mb, b_hi, 1);
          _mm512_store_si512((__m512i*)bp, mb);

          yp += 8; cbp += 8; crp += 8;
          rp += 16; gp += 16; bp += 16;
        }
      }
    }

  }
}

#endif
