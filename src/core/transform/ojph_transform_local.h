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
    union lifting_step;

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                           Generic Functions
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Irreversible functions
    //////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////
    void gen_irv_vert_step(const lifting_step* s, const line_buf* sig, 
                           const line_buf* other, const line_buf* aug, 
                           ui32 repeat, bool synthesis);

    /////////////////////////////////////////////////////////////////////////
    void gen_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void gen_irv_horz_ana(const param_atk* atk, const line_buf* ldst, 
                          const line_buf* hdst, const line_buf* src, 
                          ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void gen_irv_horz_syn(const param_atk *atk, const line_buf* dst, 
                          const line_buf *lsrc, const line_buf *hsrc, 
                          ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    // Reversible functions
    //////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////
    void gen_rev_vert_step(const lifting_step* s, const line_buf* sig, 
                           const line_buf* other, const line_buf* aug, 
                           ui32 repeat, bool synthesis);

    /////////////////////////////////////////////////////////////////////////
    void gen_rev_horz_ana(const param_atk* atk, const line_buf* ldst, 
                          const line_buf* hdst, const line_buf* src, 
                          ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void gen_rev_horz_syn(const param_atk* atk, const line_buf* dst, 
                          const line_buf* lsrc, const line_buf* hsrc, 
                          ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //                       SSE Functions (float)
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Supporting macros
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    #define SSE_DEINTERLEAVE(dpl, dph, sp, width, even)                      \
    {                                                                        \
      if (even)                                                              \
        for (; width > 0; width -= 8, sp += 8, dpl += 4, dph += 4)           \
        {                                                                    \
          __m128 a = _mm_load_ps(sp);                                        \
          __m128 b = _mm_load_ps(sp + 4);                                    \
          __m128 c = _mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0));          \
          __m128 d = _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1));          \
          _mm_store_ps(dpl, c);                                              \
          _mm_store_ps(dph, d);                                              \
        }                                                                    \
      else                                                                   \
        for (; width > 0; width -= 8, sp += 8, dpl += 4, dph += 4)           \
        {                                                                    \
          __m128 a = _mm_load_ps(sp);                                        \
          __m128 b = _mm_load_ps(sp + 4);                                    \
          __m128 c = _mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0));          \
          __m128 d = _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1));          \
          _mm_store_ps(dpl, d);                                              \
          _mm_store_ps(dph, c);                                              \
        }                                                                    \
    }                                                                        

    //////////////////////////////////////////////////////////////////////////
    #define SSE_INTERLEAVE(dp, spl, sph, width, even)                        \
    {                                                                        \
      if (even)                                                              \
        for (; width > 0; width -= 8, dp += 8, spl += 4, sph += 4)           \
        {                                                                    \
          __m128 a = _mm_load_ps(spl);                                       \
          __m128 b = _mm_load_ps(sph);                                       \
          __m128 c = _mm_unpacklo_ps(a, b);                                  \
          __m128 d = _mm_unpackhi_ps(a, b);                                  \
          _mm_store_ps(dp, c);                                               \
          _mm_store_ps(dp + 4, d);                                           \
        }                                                                    \
      else                                                                   \
        for (; width > 0; width -= 8, dp += 8, spl += 4, sph += 4)           \
        {                                                                    \
          __m128 a = _mm_load_ps(spl);                                       \
          __m128 b = _mm_load_ps(sph);                                       \
          __m128 c = _mm_unpacklo_ps(b, a);                                  \
          __m128 d = _mm_unpackhi_ps(b, a);                                  \
          _mm_store_ps(dp, c);                                               \
          _mm_store_ps(dp + 4, d);                                           \
        }                                                                    \
    }

    //////////////////////////////////////////////////////////////////////////
    // Irreversible functions
    //////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////
    void sse_irv_vert_step(const lifting_step* s, const line_buf* sig, 
                           const line_buf* other, const line_buf* aug, 
                           ui32 repeat, bool synthesis);

    /////////////////////////////////////////////////////////////////////////
    void sse_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void sse_irv_horz_ana(const param_atk* atk, const line_buf* ldst,
                          const line_buf* hdst, const line_buf* src, 
                          ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void sse_irv_horz_syn(const param_atk *atk, const line_buf* dst,
                          const line_buf *lsrc, const line_buf *hsrc, 
                          ui32 width, bool even);

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
    void sse2_rev_vert_step(const lifting_step* s, const line_buf* sig, 
                            const line_buf* other, const line_buf* aug, 
                            ui32 repeat, bool synthesis);

    /////////////////////////////////////////////////////////////////////////
    void sse2_rev_horz_ana(const param_atk* atk, const line_buf* ldst,
                           const line_buf* hdst, const line_buf* src, 
                           ui32 width, bool even);

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
    // Supporting macros
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    #define AVX_DEINTERLEAVE(dpl, dph, sp, width, even)                      \
    {                                                                        \
      if (even)                                                              \
      {                                                                      \
        for (; width > 0; width -= 16, sp += 16, dpl += 8, dph += 8)         \
        {                                                                    \
          __m256 a = _mm256_load_ps(sp);                                     \
          __m256 b = _mm256_load_ps(sp + 8);                                 \
          __m256 c = _mm256_permute2f128_ps(a, b, (2 << 4) | (0));           \
          __m256 d = _mm256_permute2f128_ps(a, b, (3 << 4) | (1));           \
          __m256 e = _mm256_shuffle_ps(c, d, _MM_SHUFFLE(2, 0, 2, 0));       \
          __m256 f = _mm256_shuffle_ps(c, d, _MM_SHUFFLE(3, 1, 3, 1));       \
          _mm256_store_ps(dpl, e);                                           \
          _mm256_store_ps(dph, f);                                           \
        }                                                                    \
      }                                                                      \
      else                                                                   \
      {                                                                      \
        for (; width > 0; width -= 16, sp += 16, dpl += 8, dph += 8)         \
        {                                                                    \
          __m256 a = _mm256_load_ps(sp);                                     \
          __m256 b = _mm256_load_ps(sp + 8);                                 \
          __m256 c = _mm256_permute2f128_ps(a, b, (2 << 4) | (0));           \
          __m256 d = _mm256_permute2f128_ps(a, b, (3 << 4) | (1));           \
          __m256 e = _mm256_shuffle_ps(c, d, _MM_SHUFFLE(2, 0, 2, 0));       \
          __m256 f = _mm256_shuffle_ps(c, d, _MM_SHUFFLE(3, 1, 3, 1));       \
          _mm256_store_ps(dpl, f);                                           \
          _mm256_store_ps(dph, e);                                           \
        }                                                                    \
      }                                                                      \
    }

    //////////////////////////////////////////////////////////////////////////
    #define AVX_INTERLEAVE(dp, spl, sph, width, even)                        \
    {                                                                        \
      if (even)                                                              \
      {                                                                      \
        for (; width > 0; width -= 16, dp += 16, spl += 8, sph += 8)         \
        {                                                                    \
          __m256 a = _mm256_load_ps(spl);                                    \
          __m256 b = _mm256_load_ps(sph);                                    \
          __m256 c = _mm256_unpacklo_ps(a, b);                               \
          __m256 d = _mm256_unpackhi_ps(a, b);                               \
          __m256 e = _mm256_permute2f128_ps(c, d, (2 << 4) | (0));           \
          __m256 f = _mm256_permute2f128_ps(c, d, (3 << 4) | (1));           \
          _mm256_store_ps(dp, e);                                            \
          _mm256_store_ps(dp + 8, f);                                        \
        }                                                                    \
      }                                                                      \
      else                                                                   \
      {                                                                      \
        for (; width > 0; width -= 16, dp += 16, spl += 8, sph += 8)         \
        {                                                                    \
          __m256 a = _mm256_load_ps(spl);                                    \
          __m256 b = _mm256_load_ps(sph);                                    \
          __m256 c = _mm256_unpacklo_ps(b, a);                               \
          __m256 d = _mm256_unpackhi_ps(b, a);                               \
          __m256 e = _mm256_permute2f128_ps(c, d, (2 << 4) | (0));           \
          __m256 f = _mm256_permute2f128_ps(c, d, (3 << 4) | (1));           \
          _mm256_store_ps(dp, e);                                            \
          _mm256_store_ps(dp + 8, f);                                        \
        }                                                                    \
      }                                                                      \
    }

    //////////////////////////////////////////////////////////////////////////
    // Irreversible functions
    //////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////
    void avx_irv_vert_step(const lifting_step* s, const line_buf* sig, 
                           const line_buf* other, const line_buf* aug, 
                           ui32 repeat, bool synthesis);

    /////////////////////////////////////////////////////////////////////////
    void avx_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void avx_irv_horz_ana(const param_atk* atk, const line_buf* ldst,
                          const line_buf* hdst, const line_buf* src, 
                          ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void avx_irv_horz_syn(const param_atk *atk, const line_buf* dst,
                          const line_buf *lsrc, const line_buf *hsrc, 
                          ui32 width, bool even);

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
    void avx2_rev_vert_step(const lifting_step* s, const line_buf* sig, 
                            const line_buf* other, const line_buf* aug, 
                            ui32 repeat, bool synthesis);

    /////////////////////////////////////////////////////////////////////////
    void avx2_rev_horz_ana(const param_atk* atk, const line_buf* ldst,
                           const line_buf* hdst, const line_buf* src, 
                           ui32 width, bool even);

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
    void avx512_irv_vert_step(const lifting_step* s, const line_buf* sig, 
                              const line_buf* other, const line_buf* aug, 
                              ui32 repeat, bool synthesis);

    /////////////////////////////////////////////////////////////////////////
    void avx512_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void avx512_irv_horz_ana(const param_atk* atk, const line_buf* ldst,
                             const line_buf* hdst, const line_buf* src, 
                             ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void avx512_irv_horz_syn(const param_atk *atk, const line_buf* dst,
                             const line_buf *lsrc, const line_buf *hsrc, 
                             ui32 width, bool even);


    //////////////////////////////////////////////////////////////////////////
    // Reversible functions
    //////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////
    void avx512_rev_vert_step(const lifting_step* s, const line_buf* sig,
                              const line_buf* other, const line_buf* aug, 
                              ui32 repeat, bool synthesis);

    /////////////////////////////////////////////////////////////////////////
    void avx512_rev_horz_ana(const param_atk* atk, const line_buf* ldst,
                             const line_buf* hdst, const line_buf* src, 
                             ui32 width, bool even);

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
    // Irreversible functions
    //////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////
    void wasm_irv_vert_step(const lifting_step* s, const line_buf* sig, 
                            const line_buf* other, const line_buf* aug, 
                            ui32 repeat, bool synthesis);

    /////////////////////////////////////////////////////////////////////////
    void wasm_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat);

    /////////////////////////////////////////////////////////////////////////
    void wasm_irv_horz_ana(const param_atk* atk, const line_buf* ldst,
                           const line_buf* hdst, const line_buf* src, 
                           ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void wasm_irv_horz_syn(const param_atk *atk, const line_buf* dst,
                           const line_buf *lsrc, const line_buf *hsrc, 
                           ui32 width, bool even);

    //////////////////////////////////////////////////////////////////////////
    // Reversible functions
    //////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////
    void wasm_rev_vert_step(const lifting_step* s, const line_buf* sig,
                            const line_buf* other, const line_buf* aug, 
                            ui32 repeat, bool synthesis);

    /////////////////////////////////////////////////////////////////////////
    void wasm_rev_horz_ana(const param_atk* atk, const line_buf* ldst,
                           const line_buf* hdst, const line_buf* src, 
                           ui32 width, bool even);

    /////////////////////////////////////////////////////////////////////////
    void wasm_rev_horz_syn(const param_atk* atk, const line_buf* dst,
                           const line_buf* lsrc, const line_buf* hsrc, 
                           ui32 width, bool even);
  }
}

#endif // !OJPH_TRANSFORM_LOCAL_H
