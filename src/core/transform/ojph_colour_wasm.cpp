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
// File: ojph_colour_wasm.cpp
// Author: Aous Naman
// Date: 9 February 2021
//***************************************************************************/

#include <cmath>
#include <wasm_simd128.h>

#include "ojph_defs.h"
#include "ojph_colour.h"
#include "ojph_colour_local.h"

namespace ojph {
  namespace local {
    
    //////////////////////////////////////////////////////////////////////////
    void wasm_cnvrt_si32_to_float_shftd(const si32 *sp, float *dp, float mul,
                                        ui32 width)
    {
      v128_t shift = wasm_f32x4_splat(0.5f);
      v128_t m = wasm_f32x4_splat(mul);
      for (ui32 i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        v128_t t = wasm_v128_load(sp);
        v128_t s = wasm_f32x4_convert_i32x4(t);
        s = wasm_f32x4_mul(s, m);
        s = wasm_f32x4_sub(s, shift);
        wasm_v128_store(dp, s);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_cnvrt_si32_to_float(const si32 *sp, float *dp, float mul,
                                  ui32 width)
    {
      v128_t m = wasm_f32x4_splat(mul);
      for (ui32 i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        v128_t t = wasm_v128_load(sp);
        v128_t s = wasm_f32x4_convert_i32x4(t);
        s = wasm_f32x4_mul(s, m);
        wasm_v128_store(dp, s);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_cnvrt_float_to_si32_shftd(const float *sp, si32 *dp, float mul,
                                        ui32 width)
    {
      // rounding mode is always set to _MM_ROUND_NEAREST
      v128_t shift = wasm_f32x4_splat(0.5f);
      v128_t m = wasm_f32x4_splat(mul);
      for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        v128_t t = wasm_v128_load(sp);
        v128_t s = wasm_f32x4_add(t, shift);
        s = wasm_f32x4_mul(s, m);
        s = wasm_f32x4_add(s, shift); // + 0.5 and followed by floor next
        wasm_v128_store(dp, wasm_i32x4_trunc_sat_f32x4(s));
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_cnvrt_float_to_si32(const float *sp, si32 *dp, float mul,
                                  ui32 width)
    {
      // rounding mode is always set to _MM_ROUND_NEAREST
      v128_t shift = wasm_f32x4_splat(0.5f);
      v128_t m = wasm_f32x4_splat(mul);
      for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        v128_t t = wasm_v128_load(sp);
        v128_t s = wasm_f32x4_mul(t, m);
        s = wasm_f32x4_add(s, shift); // + 0.5 and followed by floor next
        wasm_v128_store(dp, wasm_i32x4_trunc_sat_f32x4(s));
      }
    }


    //////////////////////////////////////////////////////////////////////////
    void wasm_cnvrt_si32_to_si32_shftd(const si32 *sp, si32 *dp, int shift,
                                       ui32 width)
    {
      v128_t sh = wasm_i32x4_splat(shift);
      for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        v128_t s = wasm_v128_load(sp);
        s = wasm_i32x4_add(s, sh);
        wasm_v128_store(dp, s);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_rct_forward(const si32 *r, const si32 *g, const si32 *b,
                          si32 *y, si32 *cb, si32 *cr, ui32 repeat)
    {
      for (int i = (repeat + 3) >> 2; i > 0; --i)
      {
        v128_t mr = wasm_v128_load(r);
        v128_t mg = wasm_v128_load(g);
        v128_t mb = wasm_v128_load(b);
        v128_t t = wasm_i32x4_add(mr, mb);
        t = wasm_i32x4_add(t, wasm_i32x4_shl(mg, 1));
        wasm_v128_store(y, wasm_i32x4_shr(t, 2));
        t = wasm_i32x4_sub(mb, mg);
        wasm_v128_store(cb, t);
        t = wasm_i32x4_sub(mr, mg);
        wasm_v128_store(cr, t);

        r += 4; g += 4; b += 4;
        y += 4; cb += 4; cr += 4;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_rct_backward(const si32 *y, const si32 *cb, const si32 *cr,
                           si32 *r, si32 *g, si32 *b, ui32 repeat)
    {
      for (int i = (repeat + 3) >> 2; i > 0; --i)
      {
        v128_t my  = wasm_v128_load(y);
        v128_t mcb = wasm_v128_load(cb);
        v128_t mcr = wasm_v128_load(cr);

        v128_t t = wasm_i32x4_add(mcb, mcr);
        t = wasm_i32x4_sub(my, wasm_i32x4_shr(t, 2));
        wasm_v128_store(g, t);
        v128_t u = wasm_i32x4_add(mcb, t);
        wasm_v128_store(b, u);
        u = wasm_i32x4_add(mcr, t);
        wasm_v128_store(r, u);

        y += 4; cb += 4; cr += 4;
        r += 4; g += 4; b += 4;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_ict_forward(const float *r, const float *g, const float *b,
                          float *y, float *cb, float *cr, ui32 repeat)
    {
      v128_t alpha_rf = wasm_f32x4_splat(CT_CNST::ALPHA_RF);
      v128_t alpha_gf = wasm_f32x4_splat(CT_CNST::ALPHA_GF);
      v128_t alpha_bf = wasm_f32x4_splat(CT_CNST::ALPHA_BF);
      v128_t beta_cbf = wasm_f32x4_splat(CT_CNST::BETA_CbF);
      v128_t beta_crf = wasm_f32x4_splat(CT_CNST::BETA_CrF);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i)
      {
        v128_t mr = wasm_v128_load(r);
        v128_t mb = wasm_v128_load(b);
        v128_t my = wasm_f32x4_mul(alpha_rf, mr);
        my = wasm_f32x4_add(my, wasm_f32x4_mul(alpha_gf, wasm_v128_load(g)));
        my = wasm_f32x4_add(my, wasm_f32x4_mul(alpha_bf, mb));
        wasm_v128_store(y, my);
        wasm_v128_store(cb, wasm_f32x4_mul(beta_cbf, wasm_f32x4_sub(mb, my)));
        wasm_v128_store(cr, wasm_f32x4_mul(beta_crf, wasm_f32x4_sub(mr, my)));
        
        r += 4; g += 4; b += 4;
        y += 4; cb += 4; cr += 4;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_ict_backward(const float *y, const float *cb, const float *cr,
                           float *r, float *g, float *b, ui32 repeat)
    {
      v128_t gamma_cr2g = wasm_f32x4_splat(CT_CNST::GAMMA_CR2G);
      v128_t gamma_cb2g = wasm_f32x4_splat(CT_CNST::GAMMA_CB2G);
      v128_t gamma_cr2r = wasm_f32x4_splat(CT_CNST::GAMMA_CR2R);
      v128_t gamma_cb2b = wasm_f32x4_splat(CT_CNST::GAMMA_CB2B);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i)
      {
        v128_t my = wasm_v128_load(y);
        v128_t mcr = wasm_v128_load(cr);
        v128_t mcb = wasm_v128_load(cb);
        v128_t mg = wasm_f32x4_sub(my, wasm_f32x4_mul(gamma_cr2g, mcr));
        wasm_v128_store(g, wasm_f32x4_sub(mg, wasm_f32x4_mul(gamma_cb2g, mcb)));
        wasm_v128_store(r, wasm_f32x4_add(my, wasm_f32x4_mul(gamma_cr2r, mcr)));
        wasm_v128_store(b, wasm_f32x4_add(my, wasm_f32x4_mul(gamma_cb2b, mcb)));

        y += 4; cb += 4; cr += 4;
        r += 4; g += 4; b += 4;
      }
    }

  }
}
