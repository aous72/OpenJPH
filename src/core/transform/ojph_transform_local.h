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
// File: ojph_transform_local.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_TRANSFORM_LOCAL_H
#define OJPH_TRANSFORM_LOCAL_H

#include "ojph_defs.h"

namespace ojph {
  struct line_buf;
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    struct LIFTING_FACTORS
    {
      static const float steps[8];
      static const float K;
      static const float K_inv;
    };

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                           Generic Functions
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Reversible functions
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_vert_wvlt_fwd_predict(const line_buf* src1,
                                       const line_buf* src2,
                                       line_buf *dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_vert_wvlt_fwd_update(const line_buf* src1,
                                      const line_buf* src2,
                                      line_buf *dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_horz_wvlt_fwd_tx(line_buf* src, line_buf *ldst,
                                  line_buf *hdst, ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_vert_wvlt_bwd_predict(const line_buf* src1,
                                       const line_buf* src2,
                                       line_buf *dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_vert_wvlt_bwd_update(const line_buf* src1,
                                      const line_buf* src2,
                                      line_buf *dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_horz_wvlt_bwd_tx(line_buf* dst, line_buf *lsrc,
                                  line_buf *hsrc, ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    // Irreversible functions
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void gen_irrev_vert_wvlt_step(const line_buf* src1, const line_buf* src2,
                                  line_buf *dst, int step_num, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void gen_irrev_vert_wvlt_K(const line_buf *src, line_buf *dst,
                               bool L_analysis_or_H_synthesis, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void gen_irrev_horz_wvlt_fwd_tx(line_buf* src, line_buf *ldst,
                                    line_buf *hdst, ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    void gen_irrev_horz_wvlt_bwd_tx(line_buf* src, line_buf *ldst,
                                    line_buf *hdst, ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                       SSE Functions (float)
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Irreversible functions
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void sse_irrev_vert_wvlt_step(const line_buf* src1, const line_buf* src2,
                                  line_buf *dst, int step_num, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void sse_irrev_vert_wvlt_K(const line_buf *src, line_buf *dst,
                               bool L_analysis_or_H_synthesis, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void sse_irrev_horz_wvlt_fwd_tx(line_buf* src, line_buf *ldst,
                                    line_buf *hdst, ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    void sse_irrev_horz_wvlt_bwd_tx(line_buf* src, line_buf *ldst,
                                    line_buf *hdst, ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                       SSE2 Functions (int)
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Reversible functions
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_vert_wvlt_fwd_predict(const line_buf* src1,
                                        const line_buf* src2,
                                        line_buf *dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_vert_wvlt_fwd_update(const line_buf* src1,
                                       const line_buf* src2,
                                       line_buf *dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_horz_wvlt_fwd_tx(line_buf* src, line_buf *ldst,
                                   line_buf *hdst, ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_vert_wvlt_bwd_predict(const line_buf* src1,
                                        const line_buf* src2,
                                        line_buf *dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_vert_wvlt_bwd_update(const line_buf* src1,
                                       const line_buf* src2,
                                       line_buf *dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_horz_wvlt_bwd_tx(line_buf* dst, line_buf *lsrc,
                                   line_buf *hsrc, ui32 width, bool even);


    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                       AVX Functions (float)
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Irreversible functions
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void avx_irrev_vert_wvlt_step(const line_buf* src1, const line_buf* src2,
                                  line_buf *dst, int step_num, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void avx_irrev_vert_wvlt_K(const line_buf *src, line_buf *dst,
                               bool L_analysis_or_H_synthesis, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void avx_irrev_horz_wvlt_fwd_tx(line_buf* src, line_buf *ldst,
                                    line_buf *hdst, ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    void avx_irrev_horz_wvlt_bwd_tx(line_buf* src, line_buf *ldst,
                                    line_buf *hdst, ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                       AVX2 Functions (int)
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Reversible functions
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_vert_wvlt_fwd_predict(const line_buf* src1,
                                        const line_buf* src2,
                                        line_buf *dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_vert_wvlt_fwd_update(const line_buf* src1,
                                       const line_buf* src2,
                                       line_buf *dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_horz_wvlt_fwd_tx(line_buf* src, line_buf *ldst,
                                   line_buf *hdst, ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_vert_wvlt_bwd_predict(const line_buf* src1,
                                        const line_buf* src2,
                                        line_buf *dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_vert_wvlt_bwd_update(const line_buf* src1,
                                       const line_buf* src2,
                                       line_buf *dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_horz_wvlt_bwd_tx(line_buf* dst, line_buf *lsrc,
                                   line_buf *hsrc, ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                          WASM Functions
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Reversible functions
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_vert_wvlt_fwd_predict(const line_buf *line_src1, 
                                        const line_buf *line_src2,
                                        line_buf *line_dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_vert_wvlt_fwd_update(const line_buf *line_src1, 
                                       const line_buf *line_src2,
                                       line_buf *line_dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_horz_wvlt_fwd_tx(line_buf *line_src, line_buf *line_ldst, 
                                   line_buf *line_hdst, ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_vert_wvlt_bwd_predict(const line_buf *line_src1, 
                                        const line_buf *line_src2,
                                        line_buf *line_dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_vert_wvlt_bwd_update(const line_buf *line_src1, 
                                       const line_buf *line_src2,
                                       line_buf *line_dst, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_horz_wvlt_bwd_tx(line_buf *line_dst, line_buf *line_lsrc, 
                                   line_buf *line_hsrc, ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    // Irreversible functions
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void wasm_irrev_vert_wvlt_step(const line_buf* line_src1, 
                                   const line_buf* line_src2,
                                   line_buf *line_dst, int step_num, 
                                   ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void wasm_irrev_vert_wvlt_K(const line_buf *line_src, line_buf *line_dst,
                                bool L_analysis_or_H_synthesis, ui32 repeat);

    //////////////////////////////////////////////////////////////////////////
    void wasm_irrev_horz_wvlt_fwd_tx(line_buf *line_src, line_buf *line_ldst, 
                                     line_buf *line_hdst, ui32 width, 
                                     bool even);

    //////////////////////////////////////////////////////////////////////////
    void wasm_irrev_horz_wvlt_bwd_tx(line_buf *line_src, line_buf *line_ldst, 
                                     line_buf *line_hdst, ui32 width, 
                                     bool even);
  }
}

#endif // !OJPH_TRANSFORM_LOCAL_H
