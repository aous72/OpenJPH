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
// File: ojph_img_io_sse41.cpp
// Author: Aous Naman
// Date: 23 May 2022
//***************************************************************************/

#include "ojph_arch.h"
#if defined(OJPH_ARCH_I386) \
  || defined(OJPH_ARCH_X86_64) \
  || defined(OJPH_ENABLE_WASM_SIMD)

#include <cstdlib>
#include <cstring>
#include <immintrin.h>

#include "ojph_file.h"
#include "ojph_img_io.h"
#include "ojph_mem.h"
#include "ojph_message.h"

namespace ojph {

  /////////////////////////////////////////////////////////////////////////////
  static
  ui16 be2le(const ui16 v)
  {
    return (ui16)((v<<8) | (v>>8));
  }

  /////////////////////////////////////////////////////////////////////////////
  void sse41_cvrt_32b1c_to_8ub1c(const line_buf *ln0, const line_buf *ln1, 
                                 const line_buf *ln2, void *dp, 
                                 ui32 bit_depth, ui32 count)
  {
    ojph_unused(ln1);
    ojph_unused(ln2);
    
    __m128i max_val_vec = _mm_set1_epi32((1 << bit_depth) - 1);
    __m128i zero = _mm_setzero_si128();
    __m128i mask = _mm_set_epi64x(0x0F0B07030E0A0602, 0x0D0905010C080400);
    const si32 *sp = ln0->i32;
    ui8* p = (ui8 *)dp;

    // 16 bytes or entries in each loop
    for ( ; count >= 16; count -= 16, sp += 16, p += 16) 
    {
      __m128i a, t;
      a = _mm_load_si128((__m128i*)sp);
      a = _mm_max_epi32(a, zero);
      t = _mm_min_epi32(a, max_val_vec);

      a = _mm_load_si128((__m128i*)sp + 1);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 8);
      t = _mm_or_si128(t, a);

      a = _mm_load_si128((__m128i*)sp + 2);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 16);
      t = _mm_or_si128(t, a);

      a = _mm_load_si128((__m128i*)sp + 3);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 24);
      t = _mm_or_si128(t, a);

      t = _mm_shuffle_epi8(t, mask);
      _mm_storeu_si128((__m128i*)p, t);
    }

    int max_val = (1 << bit_depth) - 1;
    for ( ; count > 0; --count)
    {
      int val = *sp++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui8)val;
    }    
  }

  /////////////////////////////////////////////////////////////////////////////
  void sse41_cvrt_32b3c_to_8ub3c(const line_buf *ln0, const line_buf *ln1, 
                                 const line_buf *ln2, void *dp, 
                                 ui32 bit_depth, ui32 count)
  {
    const si32 *sp0 = ln0->i32;
    const si32 *sp1 = ln1->i32;
    const si32 *sp2 = ln2->i32;
    ui8* p = (ui8 *)dp;

    __m128i max_val_vec = _mm_set1_epi32((1 << bit_depth) - 1);
    __m128i zero = _mm_setzero_si128();
    __m128i m0 = _mm_set_epi64x((si64)0xFFFFFFFF0E0D0C0A, 
                                (si64)0x0908060504020100);

    // 16 entries in each loop
    for ( ; count >= 16; count -= 16, sp0 += 16, sp1 += 16, sp2 += 16, p += 48) 
    {
      __m128i a, t, u, v, w;
      a = _mm_load_si128((__m128i*)sp0);
      a = _mm_max_epi32(a, zero);
      t = _mm_min_epi32(a, max_val_vec);

      a = _mm_load_si128((__m128i*)sp1);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 8);
      t = _mm_or_si128(t, a);

      a = _mm_load_si128((__m128i*)sp2);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 16);
      t = _mm_or_si128(t, a);
      t = _mm_shuffle_epi8(t, m0);

      a = _mm_load_si128((__m128i*)sp0 + 1);
      a = _mm_max_epi32(a, zero);
      u = _mm_min_epi32(a, max_val_vec);

      a = _mm_load_si128((__m128i*)sp1 + 1);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 8);
      u = _mm_or_si128(u, a);

      a = _mm_load_si128((__m128i*)sp2 + 1);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 16);
      u = _mm_or_si128(u, a);
      u = _mm_shuffle_epi8(u, m0);

      a = _mm_load_si128((__m128i*)sp0 + 2);
      a = _mm_max_epi32(a, zero);
      v = _mm_min_epi32(a, max_val_vec);

      a = _mm_load_si128((__m128i*)sp1 + 2);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 8);
      v = _mm_or_si128(v, a);

      a = _mm_load_si128((__m128i*)sp2 + 2);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 16);
      v = _mm_or_si128(v, a);
      v = _mm_shuffle_epi8(v, m0);

      a = _mm_load_si128((__m128i*)sp0 + 3);
      a = _mm_max_epi32(a, zero);
      w = _mm_min_epi32(a, max_val_vec);

      a = _mm_load_si128((__m128i*)sp1 + 3);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 8);
      w = _mm_or_si128(w, a);

      a = _mm_load_si128((__m128i*)sp2 + 3);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 16);
      w = _mm_or_si128(w, a);
      w = _mm_shuffle_epi8(w, m0);

      t = _mm_or_si128(t, _mm_bslli_si128(u, 12));
      u = _mm_or_si128(_mm_bsrli_si128(u, 4), _mm_bslli_si128(v, 8));
      v = _mm_or_si128(_mm_bsrli_si128(v, 8), _mm_bslli_si128(w, 4));

      _mm_storeu_si128((__m128i*)p + 0, t);
      _mm_storeu_si128((__m128i*)p + 1, u);
      _mm_storeu_si128((__m128i*)p + 2, v);
    }

    int max_val = (1<<bit_depth) - 1;
    for ( ; count > 0; --count)
    {
      int val;
      val = *sp0++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui8) val;
      val = *sp1++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui8) val;
      val = *sp2++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui8) val;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  void sse41_cvrt_32b1c_to_16ub1c_le(const line_buf *ln0, const line_buf *ln1, 
                                     const line_buf *ln2, void *dp, 
                                     ui32 bit_depth, ui32 count)
  {
    ojph_unused(ln1);
    ojph_unused(ln2);

    __m128i max_val_vec = _mm_set1_epi32((1 << bit_depth) - 1);
    __m128i zero = _mm_setzero_si128();
    __m128i mask = _mm_set_epi64x(0x0F0E0B0A07060302, 0x0D0C090805040100);
    const si32 *sp = ln0->i32;
    ui16* p = (ui16 *)dp;

    // 8 entries in each loop
    for ( ; count >= 8; count -= 8, sp += 8, p += 8) 
    {
      __m128i a, t;
      a = _mm_load_si128((__m128i*)sp);
      a = _mm_max_epi32(a, zero);
      t = _mm_min_epi32(a, max_val_vec);

      a = _mm_load_si128((__m128i*)sp + 1);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 16);
      t = _mm_or_si128(t, a);

      t = _mm_shuffle_epi8(t, mask);
      _mm_storeu_si128((__m128i*)p, t);
    }

    int max_val = (1<<bit_depth) - 1;
    for ( ; count > 0; --count)
    {
      int val = *sp++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui16) val;
    }      
  }

  /////////////////////////////////////////////////////////////////////////////
  void sse41_cvrt_32b3c_to_16ub3c_le(const line_buf *ln0, const line_buf *ln1, 
                                     const line_buf *ln2, void *dp, 
                                     ui32 bit_depth, ui32 count)
  {
    const si32 *sp0 = ln0->i32;
    const si32 *sp1 = ln1->i32;
    const si32 *sp2 = ln2->i32;
    ui16* p = (ui16*)dp;

    __m128i max_val_vec = _mm_set1_epi32((1 << bit_depth) - 1);
    __m128i zero = _mm_setzero_si128();

    __m128i m0 = _mm_set_epi64x((si64)0x0B0A0908FFFF0706, 
                                (si64)0x0504FFFF03020100);
    __m128i m1 = _mm_set_epi64x((si64)0xFFFFFFFF0504FFFF, 
                                (si64)0xFFFF0100FFFFFFFF);
    __m128i m2 = _mm_set_epi64x((si64)0xFFFFFFFFFFFFFFFF, 
                                (si64)0xFFFF0F0E0D0CFFFF);
    __m128i m3 = _mm_set_epi64x((si64)0x0706FFFFFFFF0302, 
                                (si64)0x0D0CFFFFFFFF0908);
    __m128i m4 = _mm_set_epi64x((si64)0xFFFF03020100FFFF, 
                                (si64)0xFFFFFFFFFFFFFFFF);
    __m128i m5 = _mm_set_epi64x((si64)0xFFFFFFFF0F0EFFFF, 
                                (si64)0xFFFF0B0AFFFFFFFF);
    __m128i m6 = _mm_set_epi64x((si64)0x0F0E0D0CFFFF0B0A, 
                                (si64)0x0908FFFF07060504);

    // 24 entries in each loop
    for ( ; count >= 8; count -= 8, sp0 += 8, sp1 += 8, sp2 += 8, p += 24) 
    {
      __m128i a, b, t, u, v;
      a = _mm_load_si128((__m128i*)sp0);
      a = _mm_max_epi32(a, zero);
      t = _mm_min_epi32(a, max_val_vec);

      a = _mm_load_si128((__m128i*)sp1);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 16);
      t = _mm_or_si128(t, a);

      a = _mm_load_si128((__m128i*)sp2);
      a = _mm_max_epi32(a, zero);
      u = _mm_min_epi32(a, max_val_vec);

      a = _mm_load_si128((__m128i*)sp0 + 1);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 16);
      u = _mm_or_si128(u, a);

      a = _mm_load_si128((__m128i*)sp1 + 1);
      a = _mm_max_epi32(a, zero);
      v = _mm_min_epi32(a, max_val_vec);

      a = _mm_load_si128((__m128i*)sp2 + 1);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 16);
      v = _mm_or_si128(v, a);

      a = _mm_shuffle_epi8(t, m0);
      b = _mm_shuffle_epi8(u, m1);
      a = _mm_or_si128(a, b);
      _mm_storeu_si128((__m128i*)p, a);

      a = _mm_shuffle_epi8(t, m2);
      b = _mm_shuffle_epi8(u, m3);
      a = _mm_or_si128(a, b);
      b = _mm_shuffle_epi8(v, m4);
      a = _mm_or_si128(a, b);
      _mm_storeu_si128((__m128i*)p + 1, a);

      a = _mm_shuffle_epi8(u, m5);
      b = _mm_shuffle_epi8(v, m6);
      a = _mm_or_si128(a, b);
      _mm_storeu_si128((__m128i*)p + 2, a);
    }

    int max_val = (1<<bit_depth) - 1;
    for ( ; count > 0; --count)
    {
      int val;
      val = *sp0++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = be2le((ui16) val);
      val = *sp1++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = be2le((ui16) val);
      val = *sp2++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui16) val;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  void sse41_cvrt_32b1c_to_16ub1c_be(const line_buf *ln0, const line_buf *ln1, 
                                     const line_buf *ln2, void *dp, 
                                     ui32 bit_depth, ui32 count)
  {
    ojph_unused(ln1);
    ojph_unused(ln2);

    __m128i max_val_vec = _mm_set1_epi32((1 << bit_depth) - 1);
    __m128i zero = _mm_setzero_si128();
    __m128i mask = _mm_set_epi64x(0x0E0F0A0B06070203, 0x0C0D080904050001);
    const si32 *sp = ln0->i32;
    ui16* p = (ui16 *)dp;

    // 8 entries in each loop
    for ( ; count >= 8; count -= 8, sp += 8, p += 8) 
    {
      __m128i a, t;
      a = _mm_load_si128((__m128i*)sp);
      a = _mm_max_epi32(a, zero);
      t = _mm_min_epi32(a, max_val_vec);

      a = _mm_load_si128((__m128i*)sp + 1);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 16);
      t = _mm_or_si128(t, a);

      t = _mm_shuffle_epi8(t, mask);
      _mm_storeu_si128((__m128i*)p, t);
    }

    int max_val = (1<<bit_depth) - 1;
    for ( ; count > 0; --count)
    {
      int val = *sp++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = be2le((ui16) val);
    }    
  }

  /////////////////////////////////////////////////////////////////////////////
  void sse41_cvrt_32b3c_to_16ub3c_be(const line_buf *ln0, const line_buf *ln1, 
                                     const line_buf *ln2, void *dp, 
                                     ui32 bit_depth, ui32 count)
  {
    const si32 *sp0 = ln0->i32;
    const si32 *sp1 = ln1->i32;
    const si32 *sp2 = ln2->i32;
    ui16* p = (ui16*)dp;

    __m128i max_val_vec = _mm_set1_epi32((1 << bit_depth) - 1);
    __m128i zero = _mm_setzero_si128();

    __m128i m0 = _mm_set_epi64x((si64)0x0A0B0809FFFF0607, 
                                (si64)0x0405FFFF02030001);
    __m128i m1 = _mm_set_epi64x((si64)0xFFFFFFFF0405FFFF, 
                                (si64)0xFFFF0001FFFFFFFF);
    __m128i m2 = _mm_set_epi64x((si64)0xFFFFFFFFFFFFFFFF, 
                                (si64)0xFFFF0E0F0C0DFFFF);
    __m128i m3 = _mm_set_epi64x((si64)0x0607FFFFFFFF0203, 
                                (si64)0x0C0DFFFFFFFF0809);
    __m128i m4 = _mm_set_epi64x((si64)0xFFFF02030001FFFF, 
                                (si64)0xFFFFFFFFFFFFFFFF);
    __m128i m5 = _mm_set_epi64x((si64)0xFFFFFFFF0E0FFFFF, 
                                (si64)0xFFFF0A0BFFFFFFFF);
    __m128i m6 = _mm_set_epi64x((si64)0x0E0F0C0DFFFF0A0B, 
                                (si64)0x0809FFFF06070405);

    // 24 entries in each loop
    for ( ; count >= 8; count -= 8, sp0 += 8, sp1 += 8, sp2 += 8, p += 24) 
    {
      __m128i a, b, t, u, v;
      a = _mm_load_si128((__m128i*)sp0);
      a = _mm_max_epi32(a, zero);
      t = _mm_min_epi32(a, max_val_vec);

      a = _mm_load_si128((__m128i*)sp1);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 16);
      t = _mm_or_si128(t, a);

      a = _mm_load_si128((__m128i*)sp2);
      a = _mm_max_epi32(a, zero);
      u = _mm_min_epi32(a, max_val_vec);

      a = _mm_load_si128((__m128i*)sp0 + 1);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 16);
      u = _mm_or_si128(u, a);

      a = _mm_load_si128((__m128i*)sp1 + 1);
      a = _mm_max_epi32(a, zero);
      v = _mm_min_epi32(a, max_val_vec);

      a = _mm_load_si128((__m128i*)sp2 + 1);
      a = _mm_max_epi32(a, zero);
      a = _mm_min_epi32(a, max_val_vec);
      a = _mm_slli_epi32(a, 16);
      v = _mm_or_si128(v, a);

      a = _mm_shuffle_epi8(t, m0);
      b = _mm_shuffle_epi8(u, m1);
      a = _mm_or_si128(a, b);
      _mm_storeu_si128((__m128i*)p, a);

      a = _mm_shuffle_epi8(t, m2);
      b = _mm_shuffle_epi8(u, m3);
      a = _mm_or_si128(a, b);
      b = _mm_shuffle_epi8(v, m4);
      a = _mm_or_si128(a, b);
      _mm_storeu_si128((__m128i*)p + 1, a);

      a = _mm_shuffle_epi8(u, m5);
      b = _mm_shuffle_epi8(v, m6);
      a = _mm_or_si128(a, b);
      _mm_storeu_si128((__m128i*)p + 2, a);
    }

    int max_val = (1<<bit_depth) - 1;
    for ( ; count > 0; --count)
    {
      int val;
      val = *sp0++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = be2le((ui16) val);
      val = *sp1++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = be2le((ui16) val);
      val = *sp2++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = be2le((ui16) val);
    }
  }
}

#endif
