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
    struct param_atk;

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

    /////////////////////////////////////////////////////////////////////////
    void gen_rev_vert_ana_step(const lifting_step* s, const line_buf* sig, 
                               const line_buf* other, const line_buf* aug, 
                               ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void gen_rev_horz_ana(const param_atk* atk, const line_buf* ldst, 
                          const line_buf* hdst, const line_buf* src, 
                          ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void gen_rev_vert_syn_step(const lifting_step* s, const line_buf* aug, 
                               const line_buf* sig, const line_buf* other, 
                               ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void gen_rev_horz_syn(const param_atk* atk, const line_buf* dst, 
                          const line_buf* lsrc, const line_buf* hsrc, 
                          ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    // Irreversible functions
    //////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////
    void gen_irv_vert_ana_step(const lifting_step* s, const line_buf* sig, 
                               const line_buf* other, const line_buf* aug, 
                               ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void gen_irv_horz_ana(const param_atk* atk, const line_buf* ldst, 
                          const line_buf* hdst, const line_buf* src, 
                          ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void gen_irv_vert_syn_step(const lifting_step* s, const line_buf* aug,
                               const line_buf* sig, const line_buf* other,
                               ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void gen_irv_horz_syn(const param_atk *atk, const line_buf* dst, 
                          const line_buf *lsrc, const line_buf *hsrc, 
                          ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void gen_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat);

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

    /////////////////////////////////////////////////////////////////////////
    void sse_irv_vert_ana_step(const lifting_step* s, const line_buf* sig, 
                               const line_buf* other, const line_buf* aug, 
                               ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void sse_irv_horz_ana(const param_atk* atk, const line_buf* ldst,
                          const line_buf* hdst, const line_buf* src, 
                          ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void sse_irv_vert_syn_step(const lifting_step* s, const line_buf* aug,
                               const line_buf* sig, const line_buf* other,
                               ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void sse_irv_horz_syn(const param_atk *atk, const line_buf* dst,
                          const line_buf *lsrc, const line_buf *hsrc, 
                          ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void sse_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat);

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

    /////////////////////////////////////////////////////////////////////////
    void sse2_rev_vert_ana_step(const lifting_step* s, const line_buf* sig, 
                                const line_buf* other, const line_buf* aug, 
                                ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void sse2_rev_horz_ana(const param_atk* atk, const line_buf* ldst,
                           const line_buf* hdst, const line_buf* src, 
                           ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void sse2_rev_vert_syn_step(const lifting_step* s, const line_buf* aug,
                                const line_buf* sig, const line_buf* other, 
                                ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void sse2_rev_horz_syn(const param_atk* atk, const line_buf* dst,
                           const line_buf* lsrc, const line_buf* hsrc, 
                           ui32 width, bool even);


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

    /////////////////////////////////////////////////////////////////////////
    void avx_irv_vert_ana_step(const lifting_step* s, const line_buf* sig, 
                               const line_buf* other, const line_buf* aug, 
                               ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void avx_irv_horz_ana(const param_atk* atk, const line_buf* ldst,
                          const line_buf* hdst, const line_buf* src, 
                          ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void avx_irv_vert_syn_step(const lifting_step* s, const line_buf* aug,
                               const line_buf* sig, const line_buf* other,
                               ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void avx_irv_horz_syn(const param_atk *atk, const line_buf* dst,
                          const line_buf *lsrc, const line_buf *hsrc, 
                          ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void avx_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat);

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

    /////////////////////////////////////////////////////////////////////////
    void avx2_rev_vert_ana_step(const lifting_step* s, const line_buf* sig, 
                                const line_buf* other, const line_buf* aug, 
                                ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void avx2_rev_horz_ana(const param_atk* atk, const line_buf* ldst,
                           const line_buf* hdst, const line_buf* src, 
                           ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void avx2_rev_vert_syn_step(const lifting_step* s, const line_buf* aug,
                                const line_buf* sig, const line_buf* other, 
                                ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void avx2_rev_horz_syn(const param_atk* atk, const line_buf* dst,
                           const line_buf* lsrc, const line_buf* hsrc, 
                           ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                        AVX512 Functions
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Irreversible functions
    //////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////
    void avx512_irv_vert_ana_step(const lifting_step* s, const line_buf* sig, 
                                  const line_buf* other, const line_buf* aug, 
                                  ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void avx512_irv_horz_ana(const param_atk* atk, const line_buf* ldst,
                             const line_buf* hdst, const line_buf* src, 
                             ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void avx512_irv_vert_syn_step(const lifting_step* s, const line_buf* aug,
                                  const line_buf* sig, const line_buf* other,
                                  ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void avx512_irv_horz_syn(const param_atk *atk, const line_buf* dst,
                             const line_buf *lsrc, const line_buf *hsrc, 
                             ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void avx512_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat);


    //////////////////////////////////////////////////////////////////////////
    // Reversible functions
    //////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////
    void avx512_rev_vert_ana_step(const lifting_step* s, const line_buf* sig,
                                  const line_buf* other, const line_buf* aug, 
                                  ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void avx512_rev_horz_ana(const param_atk* atk, const line_buf* ldst,
                             const line_buf* hdst, const line_buf* src, 
                             ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void avx512_rev_vert_syn_step(const lifting_step* s, const line_buf* aug,
                                  const line_buf* sig, const line_buf* other, 
                                  ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void avx512_rev_horz_syn(const param_atk* atk, const line_buf* dst,
                             const line_buf* lsrc, const line_buf* hsrc, 
                             ui32 width, bool even);

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

    /////////////////////////////////////////////////////////////////////////
    void wasm_rev_vert_ana_step(const lifting_step* s, const line_buf* sig,
                                const line_buf* other, const line_buf* aug, 
                                ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void wasm_rev_horz_ana(const param_atk* atk, const line_buf* ldst,
                           const line_buf* hdst, const line_buf* src, 
                           ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void wasm_rev_vert_syn_step(const lifting_step* s, const line_buf* aug,
                                const line_buf* sig, const line_buf* other, 
                                ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void wasm_rev_horz_syn(const param_atk* atk, const line_buf* dst,
                           const line_buf* lsrc, const line_buf* hsrc, 
                           ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    // Irreversible functions
    //////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////
    void wasm_irv_vert_ana_step(const lifting_step* s, const line_buf* sig, 
                                const line_buf* other, const line_buf* aug, 
                                ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void wasm_irv_horz_ana(const param_atk* atk, const line_buf* ldst,
                           const line_buf* hdst, const line_buf* src, 
                           ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void wasm_irv_vert_syn_step(const lifting_step* s, const line_buf* aug,
                                const line_buf* sig, const line_buf* other,
                                ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void wasm_irv_horz_syn(const param_atk *atk, const line_buf* dst,
                           const line_buf *lsrc, const line_buf *hsrc, 
                           ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void wasm_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat);
  }
}

#endif // !OJPH_TRANSFORM_LOCAL_H
