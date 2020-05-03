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

#include "ojph_defs.h"
#include "ojph_arch.h"
#include "ojph_colour.h"
#include "ojph_colour_local.h"

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    void (*cnvrt_si32_to_si32_shftd)
      (const si32 *sp, si32 *dp, int shift, int width)
      = gen_cnvrt_si32_to_si32_shftd;

    ////////////////////////////////////////////////////////////////////////////
    void (*cnvrt_si32_to_float_shftd)
      (const si32 *sp, float *dp, float mul, int width)
      = gen_cnvrt_si32_to_float_shftd;

    ////////////////////////////////////////////////////////////////////////////
    void (*cnvrt_si32_to_float)
      (const si32 *sp, float *dp, float mul, int width)
      = gen_cnvrt_si32_to_float;

    ////////////////////////////////////////////////////////////////////////////
    void (*cnvrt_float_to_si32_shftd)
      (const float *sp, si32 *dp, float mul, int width)
      = gen_cnvrt_float_to_si32_shftd;

    ////////////////////////////////////////////////////////////////////////////
    void (*cnvrt_float_to_si32)
      (const float *sp, si32 *dp, float mul, int width)
      = gen_cnvrt_float_to_si32;

    ////////////////////////////////////////////////////////////////////////////
    void (*rct_forward)
      (const si32 *r, const si32 *g, const si32 *b,
       si32 *y, si32 *cb, si32 *cr, int repeat)
      = gen_rct_forward;

    ////////////////////////////////////////////////////////////////////////////
    void (*rct_backward)
      (const si32 *y, const si32 *cb, const si32 *cr,
       si32 *r, si32 *g, si32 *b, int repeat)
      = gen_rct_backward;

    ////////////////////////////////////////////////////////////////////////////
    void (*ict_forward)
      (const float *r, const float *g, const float *b,
       float *y, float *cb, float *cr, int repeat)
      = gen_ict_forward;

    ////////////////////////////////////////////////////////////////////////////
    void (*ict_backward)
      (const float *y, const float *cb, const float *cr,
       float *r, float *g, float *b, int repeat)
      = gen_ict_backward;

    ////////////////////////////////////////////////////////////////////////////
    static bool colour_transform_functions_initialized = false;

    //////////////////////////////////////////////////////////////////////////
    void init_colour_transform_functions()
    {
      if (colour_transform_functions_initialized)
        return;

      cnvrt_si32_to_si32_shftd = gen_cnvrt_si32_to_si32_shftd;
      cnvrt_si32_to_float_shftd = gen_cnvrt_si32_to_float_shftd;
      cnvrt_si32_to_float = gen_cnvrt_si32_to_float;
      cnvrt_float_to_si32_shftd = gen_cnvrt_float_to_si32_shftd;
      cnvrt_float_to_si32 = gen_cnvrt_float_to_si32;
      rct_forward = gen_rct_forward;
      rct_backward = gen_rct_backward;
      ict_forward = gen_ict_forward;
      ict_backward = gen_ict_backward;

#ifndef OJPH_DISABLE_INTEL_SIMD
      int level = cpu_ext_level();

      if (level >= 2)
      {
        cnvrt_si32_to_float_shftd = sse_cnvrt_si32_to_float_shftd;
        cnvrt_si32_to_float = sse_cnvrt_si32_to_float;
        cnvrt_float_to_si32_shftd = sse_cnvrt_float_to_si32_shftd;
        cnvrt_float_to_si32 = sse_cnvrt_float_to_si32;
        ict_forward = sse_ict_forward;
        ict_backward = sse_ict_backward;
      }

      if (level >= 3)
      {
        cnvrt_float_to_si32_shftd = sse2_cnvrt_float_to_si32_shftd;
        cnvrt_float_to_si32 = sse2_cnvrt_float_to_si32;
        cnvrt_si32_to_si32_shftd = sse2_cnvrt_si32_to_si32_shftd;
        rct_forward = sse2_rct_forward;
        rct_backward = sse2_rct_backward;
      }

      if (level >= 7)
      {
        cnvrt_si32_to_float_shftd = avx_cnvrt_si32_to_float_shftd;
        cnvrt_si32_to_float = avx_cnvrt_si32_to_float;
        cnvrt_float_to_si32_shftd = avx_cnvrt_float_to_si32_shftd;
        cnvrt_float_to_si32 = avx_cnvrt_float_to_si32;
        ict_forward = avx_ict_forward;
        ict_backward = avx_ict_backward;
      }

      if (level >= 8)
      {
        cnvrt_si32_to_si32_shftd = avx2_cnvrt_si32_to_si32_shftd;
        rct_forward = avx2_rct_forward;
        rct_backward = avx2_rct_backward;
      }
#endif

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
    void gen_cnvrt_si32_to_si32_shftd(const si32 *sp, si32 *dp, int shift,
                                      int width)
    {
      for (int i = width; i > 0; --i)
        *dp++ = *sp++ + shift;
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_cnvrt_si32_to_float_shftd(const si32 *sp, float *dp, float mul,
                                       int width)
    {
      for (int i = width; i > 0; --i)
        *dp++ = (float)*sp++ * mul - 0.5f;
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_cnvrt_si32_to_float(const si32 *sp, float *dp, float mul,
                                 int width)
    {
      for (int i = width; i > 0; --i)
        *dp++ = (float)*sp++ * mul;
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_cnvrt_float_to_si32_shftd(const float *sp, si32 *dp, float mul,
                                       int width)
    {
      for (int i = width; i > 0; --i)
        *dp++ = ojph_round((*sp++ + 0.5f) * mul);
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_cnvrt_float_to_si32(const float *sp, si32 *dp, float mul,
                                 int width)
    {
      for (int i = width; i > 0; --i)
        *dp++ = ojph_round(*sp++ * mul);
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_rct_forward(const si32 *r, const si32 *g, const si32 *b,
                         si32 *y, si32 *cb, si32 *cr, int repeat)
    {
      for (int i = repeat; i > 0; --i)
      {
        *y++ = (*r + (*g << 1) + *b) >> 2;
        *cb++ = (*b++ - *g);
        *cr++ = (*r++ - *g++);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_rct_backward(const si32 *y, const si32 *cb, const si32 *cr,
                          si32 *r, si32 *g, si32 *b, int repeat)
    {
      for (int i = repeat; i > 0; --i)
      {
        *g = *y++ - ((*cb + *cr)>>2);
        *b++ = *cb++ + *g;
        *r++ = *cr++ + *g++;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_ict_forward(const float *r, const float *g, const float *b,
                         float *y, float *cb, float *cr, int repeat)
    {
      for (int i = repeat; i > 0; --i)
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
                          float *r, float *g, float *b, int repeat)
    {
      for (int i = repeat; i > 0; --i)
      {
        *g++ = *y - CT_CNST::GAMMA_CR2G * *cr - CT_CNST::GAMMA_CB2G * *cb;
        *r++ = *y + CT_CNST::GAMMA_CR2R * *cr++;
        *b++ = *y++ + CT_CNST::GAMMA_CB2B * *cb++;
      }
    }

  }
}
