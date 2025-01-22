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
// File: ojph_colour.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/

#include <cmath>
#include <climits>

#include "ojph_defs.h"
#include "ojph_arch.h"
#include "ojph_mem.h"
#include "ojph_colour.h"
#include "ojph_colour_local.h"

namespace ojph {

  // defined elsewhere
  class line_buf;

  namespace local {

    //////////////////////////////////////////////////////////////////////////
    void (*rev_convert)
      (const line_buf *src_line, const ui32 src_line_offset, 
       line_buf *dst_line, const ui32 dst_line_offset, 
       si64 shift, ui32 width) = NULL;

    //////////////////////////////////////////////////////////////////////////
    void (*rev_convert_nlt_type3)
      (const line_buf *src_line, const ui32 src_line_offset, 
       line_buf *dst_line, const ui32 dst_line_offset, 
       si64 shift, ui32 width) = NULL;

    //////////////////////////////////////////////////////////////////////////
    void (*cnvrt_si32_to_float_shftd)
      (const si32 *sp, float *dp, float mul, ui32 width) = NULL;

    //////////////////////////////////////////////////////////////////////////
    void (*cnvrt_si32_to_float)
      (const si32 *sp, float *dp, float mul, ui32 width) = NULL;
      
    //////////////////////////////////////////////////////////////////////////
    void (*cnvrt_float_to_si32_shftd)
      (const float *sp, si32 *dp, float mul, ui32 width) = NULL;

    //////////////////////////////////////////////////////////////////////////
    void (*cnvrt_float_to_si32)
      (const float *sp, si32 *dp, float mul, ui32 width) = NULL;

    //////////////////////////////////////////////////////////////////////////
    void (*irv_convert_to_float_nlt_type3) (
      const line_buf *src_line, ui32 src_line_offset,
      line_buf *dst_line, ui32 bit_depth, bool is_signed, ui32 width) = NULL;
      
    //////////////////////////////////////////////////////////////////////////
    void (*irv_convert_to_integer_nlt_type3) (
      const line_buf *src_line, line_buf *dst_line, ui32 dst_line_offset, 
      ui32 bit_depth, bool is_signed, ui32 width) = NULL;

    //////////////////////////////////////////////////////////////////////////
    void (*rct_forward)
      (const line_buf* r, const line_buf* g, const line_buf* b,
       line_buf* y, line_buf* cb, line_buf* cr, ui32 repeat) = NULL;

    //////////////////////////////////////////////////////////////////////////
    void (*rct_backward)
      (const line_buf* r, const line_buf* g, const line_buf* b,
       line_buf* y, line_buf* cb, line_buf* cr, ui32 repeat) = NULL;

    //////////////////////////////////////////////////////////////////////////
    void (*ict_forward)
      (const float *r, const float *g, const float *b,
       float *y, float *cb, float *cr, ui32 repeat) = NULL;

    //////////////////////////////////////////////////////////////////////////
    void (*ict_backward)
      (const float *y, const float *cb, const float *cr,
       float *r, float *g, float *b, ui32 repeat) = NULL;

    //////////////////////////////////////////////////////////////////////////
    static bool colour_transform_functions_initialized = false;

    //////////////////////////////////////////////////////////////////////////
    void init_colour_transform_functions()
    {
      if (colour_transform_functions_initialized)
        return;

#if !defined(OJPH_ENABLE_WASM_SIMD) || !defined(OJPH_EMSCRIPTEN)

      rev_convert = gen_rev_convert;
      rev_convert_nlt_type3 = gen_rev_convert_nlt_type3;
      cnvrt_si32_to_float_shftd = gen_cnvrt_si32_to_float_shftd;
      cnvrt_si32_to_float = gen_cnvrt_si32_to_float;
      cnvrt_float_to_si32_shftd = gen_cnvrt_float_to_si32_shftd;
      cnvrt_float_to_si32 = gen_cnvrt_float_to_si32;
      irv_convert_to_float_nlt_type3 = gen_irv_convert_to_float_nlt_type3;
      irv_convert_to_integer_nlt_type3 = gen_irv_convert_to_integer_nlt_type3;
      rct_forward = gen_rct_forward;
      rct_backward = gen_rct_backward;
      ict_forward = gen_ict_forward;
      ict_backward = gen_ict_backward;

  #ifndef OJPH_DISABLE_SIMD

    #if (defined(OJPH_ARCH_X86_64) || defined(OJPH_ARCH_I386))

      #ifndef OJPH_DISABLE_SSE
        if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_SSE)
        {
          cnvrt_si32_to_float_shftd = sse_cnvrt_si32_to_float_shftd;
          cnvrt_si32_to_float = sse_cnvrt_si32_to_float;
          cnvrt_float_to_si32_shftd = sse_cnvrt_float_to_si32_shftd;
          cnvrt_float_to_si32 = sse_cnvrt_float_to_si32;
          ict_forward = sse_ict_forward;
          ict_backward = sse_ict_backward;
        }
      #endif // !OJPH_DISABLE_SSE

      #ifndef OJPH_DISABLE_SSE2
        if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_SSE2)
        {
          rev_convert = sse2_rev_convert;
          rev_convert_nlt_type3 = sse2_rev_convert_nlt_type3;
          cnvrt_float_to_si32_shftd = sse2_cnvrt_float_to_si32_shftd;
          cnvrt_float_to_si32 = sse2_cnvrt_float_to_si32;
          rct_forward = sse2_rct_forward;
          rct_backward = sse2_rct_backward;
        }
      #endif // !OJPH_DISABLE_SSE2

      #ifndef OJPH_DISABLE_AVX
        if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_AVX)
        {
          cnvrt_si32_to_float_shftd = avx_cnvrt_si32_to_float_shftd;
          cnvrt_si32_to_float = avx_cnvrt_si32_to_float;
          cnvrt_float_to_si32_shftd = avx_cnvrt_float_to_si32_shftd;
          cnvrt_float_to_si32 = avx_cnvrt_float_to_si32;
          ict_forward = avx_ict_forward;
          ict_backward = avx_ict_backward;
        }
      #endif // !OJPH_DISABLE_AVX

      #ifndef OJPH_DISABLE_AVX2
        if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_AVX2)
        {
          rev_convert = avx2_rev_convert;
          rev_convert_nlt_type3 = avx2_rev_convert_nlt_type3;
          rct_forward = avx2_rct_forward;
          rct_backward = avx2_rct_backward;
        }
      #endif // !OJPH_DISABLE_AVX2

    #elif defined(OJPH_ARCH_ARM)
    
    #endif // !(defined(OJPH_ARCH_X86_64) || defined(OJPH_ARCH_I386))

  #endif // !OJPH_DISABLE_SIMD

#else // OJPH_ENABLE_WASM_SIMD

      rev_convert = wasm_rev_convert;
      rev_convert_nlt_type3 = wasm_rev_convert_nlt_type3;
      cnvrt_si32_to_float_shftd = wasm_cnvrt_si32_to_float_shftd;
      cnvrt_si32_to_float = wasm_cnvrt_si32_to_float;
      cnvrt_float_to_si32_shftd = wasm_cnvrt_float_to_si32_shftd;
      cnvrt_float_to_si32 = wasm_cnvrt_float_to_si32;
      rct_forward = wasm_rct_forward;
      rct_backward = wasm_rct_backward;
      ict_forward = wasm_ict_forward;
      ict_backward = wasm_ict_backward;

#endif // !OJPH_ENABLE_WASM_SIMD

      colour_transform_functions_initialized = true;
    }

    //////////////////////////////////////////////////////////////////////////
    const float CT_CNST::ALPHA_RF = 0.299f;
    const float CT_CNST::ALPHA_GF = 0.587f;
    const float CT_CNST::ALPHA_BF = 0.114f;
    const float CT_CNST::BETA_CbF = float(0.5/(1-double(CT_CNST::ALPHA_BF)));
    const float CT_CNST::BETA_CrF = float(0.5/(1-double(CT_CNST::ALPHA_RF)));
    const float CT_CNST::GAMMA_CB2G =
      float(2.0*double(ALPHA_BF)*(1.0-double(ALPHA_BF))/double(ALPHA_GF));
    const float CT_CNST::GAMMA_CR2G =
      float(2.0*double(ALPHA_RF)*(1.0-double(ALPHA_RF))/double(ALPHA_GF));
    const float CT_CNST::GAMMA_CB2B = float(2.0 * (1.0 - double(ALPHA_BF)));
    const float CT_CNST::GAMMA_CR2R = float(2.0 * (1.0 - double(ALPHA_RF)));

    //////////////////////////////////////////////////////////////////////////

#if !defined(OJPH_ENABLE_WASM_SIMD) || !defined(OJPH_EMSCRIPTEN)

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_convert(
      const line_buf *src_line, const ui32 src_line_offset, 
      line_buf *dst_line, const ui32 dst_line_offset, 
      si64 shift, ui32 width)
    {
      if (src_line->flags & line_buf::LFT_32BIT)
      { 
        if (dst_line->flags & line_buf::LFT_32BIT)
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si32 *dp = dst_line->i32 + dst_line_offset;
          si32 s = (si32)shift;
          for (ui32 i = width; i > 0; --i)
            *dp++ = *sp++ + s;
        }
        else 
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si64 *dp = dst_line->i64 + dst_line_offset;
          for (ui32 i = width; i > 0; --i)
            *dp++ = *sp++ + shift;
        }
      }
      else 
      {
        assert(src_line->flags & line_buf::LFT_64BIT);
        assert(dst_line->flags & line_buf::LFT_32BIT);
        const si64 *sp = src_line->i64 + src_line_offset;
        si32 *dp = dst_line->i32 + dst_line_offset;
        for (ui32 i = width; i > 0; --i)
          *dp++ = (si32)(*sp++ + shift);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_convert_nlt_type3(
      const line_buf *src_line, const ui32 src_line_offset, 
      line_buf *dst_line, const ui32 dst_line_offset, 
      si64 shift, ui32 width)
    {
      if (src_line->flags & line_buf::LFT_32BIT)
      { 
        if (dst_line->flags & line_buf::LFT_32BIT)
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si32 *dp = dst_line->i32 + dst_line_offset;
          si32 s = (si32)shift;
          for (ui32 i = width; i > 0; --i) {
            const si32 v = *sp++;
            *dp++ = v >= 0 ? v : (- v - s);
          }
        }
        else 
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si64 *dp = dst_line->i64 + dst_line_offset;
          for (ui32 i = width; i > 0; --i) {
            const si64 v = *sp++;
            *dp++ = v >= 0 ? v : (- v - shift);
          }
        }
      }
      else 
      {
        assert(src_line->flags & line_buf::LFT_64BIT);
        assert(dst_line->flags & line_buf::LFT_32BIT);
        const si64 *sp = src_line->i64 + src_line_offset;
        si32 *dp = dst_line->i32 + dst_line_offset;
        for (ui32 i = width; i > 0; --i) {
          const si64 v = *sp++;
          *dp++ = (si32)(v >= 0 ? v : (- v - shift));
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_cnvrt_si32_to_float_shftd(const si32 *sp, float *dp, float mul,
                                       ui32 width)
    {
      for (ui32 i = width; i > 0; --i)
        *dp++ = (float)*sp++ * mul - 0.5f;
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_cnvrt_si32_to_float(const si32 *sp, float *dp, float mul,
                                 ui32 width)
    {
      for (ui32 i = width; i > 0; --i)
        *dp++ = (float)*sp++ * mul;
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_cnvrt_float_to_si32_shftd(const float *sp, si32 *dp, float mul,
                                       ui32 width)
    {
      for (ui32 i = width; i > 0; --i)
        *dp++ = ojph_round((*sp++ + 0.5f) * mul);
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_cnvrt_float_to_si32(const float *sp, si32 *dp, float mul,
                                 ui32 width)
    {
      for (ui32 i = width; i > 0; --i)
        *dp++ = ojph_round(*sp++ * mul);
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_irv_convert_to_float_nlt_type3(const line_buf *src_line, 
      ui32 src_line_offset, line_buf *dst_line, 
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      assert((src_line->flags & line_buf::LFT_32BIT) &&
             (src_line->flags & line_buf::LFT_INTEGER) &&
             (dst_line->flags & line_buf::LFT_32BIT) &&
             (dst_line->flags & line_buf::LFT_INTEGER) == 0);

      float mul = (float)(1.0 / 65536.0 / 65536.0);

      const si32* sp = src_line->i32 + src_line_offset;
      float* dp = dst_line->f32;
      ui32 shift = 32 - bit_depth;
      if (is_signed)
      {
        si32 bias = (si32)((ui32)INT_MIN + 1);
        for (ui32 i = width; i > 0; --i) {
          si32 v = *sp++ << shift;
          v = (v >= 0) ? v : (- v - bias);
          *dp++ = (float)v * mul;
        }
      }
      else
      {
        for (ui32 i = width; i > 0; --i) {
          si32 v = *sp++ << shift;
          *dp++ = (float)v * mul - 0.5f;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_irv_convert_to_integer_nlt_type3(const line_buf *src_line, 
      line_buf *dst_line, ui32 dst_line_offset,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      assert((src_line->flags & line_buf::LFT_32BIT) &&
             (src_line->flags & line_buf::LFT_INTEGER) == 0 &&
             (dst_line->flags & line_buf::LFT_32BIT) &&
             (dst_line->flags & line_buf::LFT_INTEGER));
      
      const float* sp = src_line->f32;
      si32* dp = dst_line->i32 + dst_line_offset;
      if (bit_depth <= 30) 
      {
        // We are leaving two bit overhead -- here, we are assuming that after
        // multiplications, the resulting number can still be represented
        // using 32 bit integer
        float mul = (float)(1 << bit_depth);
        const si32 upper_limit = INT_MAX >> (32 - bit_depth);
        const si32 lower_limit = INT_MIN >> (32 - bit_depth);

        if (is_signed)
        {
          const si32 bias = (1 << (bit_depth - 1)) + 1;
          for (ui32 i = width; i > 0; --i) {
            si32 v = ojph_round(*sp++ * mul);
            v = ojph_max(v, lower_limit);
            v = ojph_min(v, upper_limit);
            v = (v >= 0) ? v : (- v - bias);
            *dp++ = v;
          }
        }
        else
        {
          const si32 half = (1 << (bit_depth - 1));
          for (ui32 i = width; i > 0; --i) {
            si32 v = ojph_round(*sp++ * mul);
            v = ojph_max(v, lower_limit);
            v = ojph_min(v, upper_limit);
            *dp++ = v + half;
          }
        }
      }
      else
      {
        // There is the possibility that converting to integer will
        // exceed the dynamic range of 32bit integer; therefore, we need
        // to use 64 bit.  One may think, why not limit the floats to the
        // range of [-0.5f, 0.5f)? 
        // Notice the half closed range -- we need a value just below 0.5f.
        // While getting this number is possible, after multiplication, the
        // resulting number will not be exactly the maximum that the integer 
        // can achieve.  All this is academic, because here are talking
        // about a number which has all the exponent bits set, meaning 
        // it is either infinity, -infinity, qNan or sNan.
        float mul = (float)(1ull << bit_depth);
        const si64 upper_limit = (si64)LLONG_MAX >> (64 - bit_depth);
        const si64 lower_limit = (si64)LLONG_MIN >> (64 - bit_depth);

        if (is_signed)
        {
          const si32 bias = (1 << (bit_depth - 1)) + 1;
          for (ui32 i = width; i > 0; --i) {
            si64 t = ojph_round64(*sp++ * mul);
            t = ojph_max(t, lower_limit);
            t = ojph_min(t, upper_limit);
            si32 v = (si32)t;
            v = (v >= 0) ? v : (- v - bias);
            *dp++ = v;
          }
        }
        else
        {
          const si32 half = (1 << (bit_depth - 1));
          for (ui32 i = width; i > 0; --i) {
            si64 t = ojph_round64(*sp++ * mul);
            t = ojph_max(t, lower_limit);
            t = ojph_min(t, upper_limit);
            si32 v = (si32)t;
            *dp++ = v + half;
          }
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_rct_forward(
      const line_buf *r, const line_buf *g, const line_buf *b,
      line_buf *y, line_buf *cb, line_buf *cr, ui32 repeat)
    {
      assert((y->flags  & line_buf::LFT_INTEGER) &&
             (cb->flags & line_buf::LFT_INTEGER) && 
             (cr->flags & line_buf::LFT_INTEGER) &&
             (r->flags  & line_buf::LFT_INTEGER) &&
             (g->flags  & line_buf::LFT_INTEGER) && 
             (b->flags  & line_buf::LFT_INTEGER));
      
      if  (y->flags & line_buf::LFT_32BIT)
      {
        assert((y->flags  & line_buf::LFT_32BIT) &&
               (cb->flags & line_buf::LFT_32BIT) && 
               (cr->flags & line_buf::LFT_32BIT) &&
               (r->flags  & line_buf::LFT_32BIT) &&
               (g->flags  & line_buf::LFT_32BIT) && 
               (b->flags  & line_buf::LFT_32BIT));        
        const si32 *rp = r->i32, * gp = g->i32, * bp = b->i32;
        si32 *yp = y->i32, * cbp = cb->i32, * crp = cr->i32;
        for (ui32 i = repeat; i > 0; --i)
        {
          si32 rr = *rp++, gg = *gp++, bb = *bp++;
          *yp++ = (rr + (gg << 1) + bb) >> 2;
          *cbp++ = (bb - gg);
          *crp++ = (rr - gg);
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
        for (ui32 i = repeat; i > 0; --i)
        {
          si64 rr = *rp++, gg = *gp++, bb = *bp++;
          *yp++ = (rr + (gg << 1) + bb) >> 2;
          *cbp++ = (bb - gg);
          *crp++ = (rr - gg);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_rct_backward(
      const line_buf *y, const line_buf *cb, const line_buf *cr,
      line_buf *r, line_buf *g, line_buf *b, ui32 repeat)
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
        for (ui32 i = repeat; i > 0; --i)
        {
          si32 yy = *yp++, cbb = *cbp++, crr = *crp++;
          si32 gg = yy - ((cbb + crr) >> 2);
          *rp++ = crr + gg;
          *gp++ = gg;
          *bp++ = cbb + gg;
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
        for (ui32 i = repeat; i > 0; --i)
        {
          si64 yy = *yp++, cbb = *cbp++, crr = *crp++;
          si64 gg = yy - ((cbb + crr) >> 2);
          *rp++ = (si32)(crr + gg);
          *gp++ = (si32)gg;
          *bp++ = (si32)(cbb + gg);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_ict_forward(const float *r, const float *g, const float *b,
                         float *y, float *cb, float *cr, ui32 repeat)
    {
      for (ui32 i = repeat; i > 0; --i)
      {
        *y = CT_CNST::ALPHA_RF * *r
           + CT_CNST::ALPHA_GF * *g++
           + CT_CNST::ALPHA_BF * *b;
        *cb++ = CT_CNST::BETA_CbF * (*b++ - *y);
        *cr++ = CT_CNST::BETA_CrF * (*r++ - *y++);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_ict_backward(const float *y, const float *cb, const float *cr,
                          float *r, float *g, float *b, ui32 repeat)
    {
      for (ui32 i = repeat; i > 0; --i)
      {
        *g++ = *y - CT_CNST::GAMMA_CR2G * *cr - CT_CNST::GAMMA_CB2G * *cb;
        *r++ = *y + CT_CNST::GAMMA_CR2R * *cr++;
        *b++ = *y++ + CT_CNST::GAMMA_CB2B * *cb++;
      }
    }

#endif // !OJPH_ENABLE_WASM_SIMD

  }
}
