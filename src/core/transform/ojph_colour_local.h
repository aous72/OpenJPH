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
// File: ojph_colour_local.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_COLOR_LOCAL_H
#define OJPH_COLOR_LOCAL_H

namespace ojph {
  namespace local {

    struct CT_CNST
    {
      // for irreversible color transform
      static const float ALPHA_RF;
      static const float ALPHA_GF;
      static const float ALPHA_BF;
      static const float BETA_CbF;
      static const float BETA_CrF;
      static const float GAMMA_CR2R;
      static const float GAMMA_CB2B;
      static const float GAMMA_CR2G;
      static const float GAMMA_CB2G;
    };

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                           Generic Functions
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_convert(
      const line_buf *src_line, const ui32 src_line_offset, 
      line_buf *dst_line, const ui32 dst_line_offset, 
      si64 shift, ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_convert_nlt_type3(
      const line_buf *src_line, const ui32 src_line_offset, 
      line_buf *dst_line, const ui32 dst_line_offset, 
      si64 shift, ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void gen_cnvrt_si32_to_float_shftd(const si32 *sp, float *dp, float mul,
                                       ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void gen_cnvrt_si32_to_float(const si32 *sp, float *dp, float mul,
                                 ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void gen_cnvrt_float_to_si32_shftd(const float *sp, si32 *dp, float mul,
                                       ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void gen_cnvrt_float_to_si32(const float *sp, si32 *dp, float mul,
                                 ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void gen_rct_forward(
      const line_buf *r, const line_buf *g, const line_buf *b,
      line_buf *y, line_buf *cb, line_buf *cr, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void gen_rct_backward(
      const line_buf *y, const line_buf *cb, const line_buf *cr,
      line_buf *r, line_buf *g, line_buf *b, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void gen_ict_forward(const float *r, const float *g, const float *b,
                         float *y, float *cb, float *cr, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void gen_ict_backward(const float *y, const float *cb, const float *cr,
                          float *r, float *g, float *b, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                       SSE Functions (float)
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void sse_cnvrt_si32_to_float_shftd(const si32 *sp, float *dp, float mul,
                                       ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void sse_cnvrt_si32_to_float(const si32 *sp, float *dp, float mul,
                                 ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void sse_cnvrt_float_to_si32_shftd(const float *sp, si32 *dp, float mul,
                                       ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void sse_cnvrt_float_to_si32(const float *sp, si32 *dp, float mul,
                                 ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void sse_ict_forward(const float *r, const float *g, const float *b,
                         float *y, float *cb, float *cr, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void sse_ict_backward(const float *y, const float *cb, const float *cr,
                          float *r, float *g, float *b, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                      SSE2 Functions (float)
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void sse2_cnvrt_float_to_si32_shftd(const float *sp, si32 *dp, float mul,
                                        ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void sse2_cnvrt_float_to_si32(const float *sp, si32 *dp, float mul,
                                  ui32 width);

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                       SSE2 Functions (integer)
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_convert(
      const line_buf *src_line, const ui32 src_line_offset, 
      line_buf *dst_line, const ui32 dst_line_offset, 
      si64 shift, ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_convert_nlt_type3(
      const line_buf *src_line, const ui32 src_line_offset, 
      line_buf *dst_line, const ui32 dst_line_offset, 
      si64 shift, ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void sse2_rct_forward(
      const line_buf *r, const line_buf *g, const line_buf *b,
      line_buf *y, line_buf *cb, line_buf *cr, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void sse2_rct_backward(
      const line_buf *y, const line_buf *cb, const line_buf *cr,
      line_buf *r, line_buf *g, line_buf *b, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                       AVX Functions (float)
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void avx_cnvrt_si32_to_float_shftd(const si32 *sp, float *dp, float mul,
                                       ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void avx_cnvrt_si32_to_float(const si32 *sp, float *dp, float mul,
                                 ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void avx_cnvrt_float_to_si32_shftd(const float *sp, si32 *dp, float mul,
                                       ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void avx_cnvrt_float_to_si32(const float *sp, si32 *dp, float mul,
                                 ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void avx_ict_forward(const float *r, const float *g, const float *b,
                         float *y, float *cb, float *cr, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void avx_ict_backward(const float *y, const float *cb, const float *cr,
                          float *r, float *g, float *b, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                       AVX2 Functions (integer)
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_convert(
      const line_buf *src_line, const ui32 src_line_offset, 
      line_buf *dst_line, const ui32 dst_line_offset, 
      si64 shift, ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_convert_nlt_type3(
      const line_buf *src_line, const ui32 src_line_offset, 
      line_buf *dst_line, const ui32 dst_line_offset, 
      si64 shift, ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void avx2_rct_forward(
      const line_buf *r, const line_buf *g, const line_buf *b,
      line_buf *y, line_buf *cb, line_buf *cr, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void avx2_rct_backward(
      const line_buf *y, const line_buf *cb, const line_buf *cr,
      line_buf *r, line_buf *g, line_buf *b, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                              WASM Functions 
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void wasm_cnvrt_si32_to_float_shftd(const si32 *sp, float *dp, float mul,
                                        ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void wasm_cnvrt_si32_to_float(const si32 *sp, float *dp, float mul,
                                  ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void wasm_cnvrt_float_to_si32_shftd(const float *sp, si32 *dp, float mul,
                                        ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void wasm_cnvrt_float_to_si32(const float *sp, si32 *dp, float mul,
                                  ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_convert(
      const line_buf *src_line, const ui32 src_line_offset, 
      line_buf *dst_line, const ui32 dst_line_offset, 
      si64 shift, ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_convert_nlt_type3(
      const line_buf *src_line, const ui32 src_line_offset, 
      line_buf *dst_line, const ui32 dst_line_offset, 
      si64 shift, ui32 width);

    //////////////////////////////////////////////////////////////////////////
    void wasm_rct_forward(
      const line_buf *r, const line_buf *g, const line_buf *b,
      line_buf *y, line_buf *cb, line_buf *cr, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void wasm_rct_backward(
      const line_buf *y, const line_buf *cb, const line_buf *cr,
      line_buf *r, line_buf *g, line_buf *b, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void wasm_ict_forward(const float *r, const float *g, const float *b,
                          float *y, float *cb, float *cr, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void wasm_ict_backward(const float *y, const float *cb, const float *cr,
                           float *r, float *g, float *b, ui32 repeat);

  }
}



#endif // !OJPH_COLOR_LOCAL_H
