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
// File: ojph_transform.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_TRANSFORM_H
#define OJPH_TRANSFORM_H

#include "ojph_defs.h"

namespace ojph {
  struct line_buf;
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    void init_wavelet_transform_functions();

    /////////////////////////////////////////////////////////////////////////
    // Reversible functions
    /////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////
    extern void (*rev_vert_wvlt_fwd_predict)
      (const line_buf* src1, const line_buf* src2, line_buf *dst,
       ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    extern void (*rev_vert_wvlt_fwd_update)
      (const line_buf* src1, const line_buf* src2, line_buf *dst,
       ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    extern void (*rev_horz_wvlt_fwd_tx)
      (line_buf* src, line_buf *ldst, line_buf *hdst, ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    extern void (*rev_vert_wvlt_bwd_predict)
      (const line_buf* src1, const line_buf* src2, line_buf *dst,
       ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    extern void (*rev_vert_wvlt_bwd_update)
      (const line_buf* src1, const line_buf* src2, line_buf *dst,
       ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    extern void (*rev_horz_wvlt_bwd_tx)
      (line_buf* dst, line_buf *lsrc, line_buf *hsrc, ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    // Irreversible functions
    /////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////
    extern void (*irrev_vert_wvlt_step)
      (const line_buf* src1, const line_buf* src2, line_buf *dst,
       int step_num, ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    extern void (*irrev_vert_wvlt_K)
      (const line_buf *src, line_buf *dst, bool L_analysis_or_H_synthesis,
       ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    extern void (*irrev_horz_wvlt_fwd_tx)
      (line_buf* src, line_buf *ldst, line_buf *hdst, ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    extern void (*irrev_horz_wvlt_bwd_tx)
      (line_buf* src, line_buf *ldst, line_buf *hdst, ui32 width, bool even);

  }
}

#endif // !OJPH_TRANSFORM_H
