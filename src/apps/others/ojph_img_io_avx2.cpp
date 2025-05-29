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
// File: ojph_img_io_avx2.cpp
// Author: Aous Naman
// Date: 23 May 2022
//***************************************************************************/

#include "ojph_arch.h"
#if defined(OJPH_ARCH_I386) || defined(OJPH_ARCH_X86_64)

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
  void avx2_cvrt_32b1c_to_8ub1c(const line_buf *ln0, const line_buf *ln1, 
                                const line_buf *ln2, void *dp, 
                                ui32 bit_depth, ui32 count)
  {
    ojph_unused(ln1);
    ojph_unused(ln2);
    
    __m256i max_val_vec = _mm256_set1_epi32((1 << bit_depth) - 1);
    __m256i zero = _mm256_setzero_si256();
    __m256i mask = _mm256_set_epi64x(0x0F0B07030E0A0602, 0x0D0905010C080400, 
                                     0x0F0B07030E0A0602, 0x0D0905010C080400);
    const si32 *sp = ln0->i32;
    ui8* p = (ui8 *)dp;

    // 32 bytes or entries in each loop
    for ( ; count >= 32; count -= 32, sp += 32, p += 32) 
    {
      __m256i a, t, u, v0, v1;
      a = _mm256_load_si256((__m256i*)sp);
      a = _mm256_max_epi32(a, zero);
      t = _mm256_min_epi32(a, max_val_vec);

      a = _mm256_load_si256((__m256i*)sp + 1);
      a = _mm256_max_epi32(a, zero);
      a = _mm256_min_epi32(a, max_val_vec);
      a = _mm256_slli_epi32(a, 16);
      t = _mm256_or_si256(t, a);

      a = _mm256_load_si256((__m256i*)sp + 2);
      a = _mm256_max_epi32(a, zero);
      u = _mm256_min_epi32(a, max_val_vec);

      a = _mm256_load_si256((__m256i*)sp + 3);
      a = _mm256_max_epi32(a, zero);
      a = _mm256_min_epi32(a, max_val_vec);
      a = _mm256_slli_epi32(a, 16);
      u = _mm256_or_si256(u, a);

      v0 = _mm256_permute2x128_si256(t, u, 0x20);
      v1 = _mm256_permute2x128_si256(t, u, 0x31);
      v1 = _mm256_slli_epi32(v1, 8);
      v0 = _mm256_or_si256(v0, v1);
      
      v0 = _mm256_shuffle_epi8(v0, mask);
      _mm256_storeu_si256((__m256i*)p, v0);
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
  void avx2_cvrt_32b3c_to_8ub3c(const line_buf *ln0, const line_buf *ln1, 
                                const line_buf *ln2, void *dp, 
                                ui32 bit_depth, ui32 count)
  {
    int max_val = (1 << bit_depth) - 1;
    __m256i max_val_vec = _mm256_set1_epi32(max_val);
    __m256i zero = _mm256_setzero_si256();
    __m256i m0 = _mm256_set_epi64x((si64)0xFFFFFFFF0E0D0C0A,
                                   (si64)0x0908060504020100,
                                   (si64)0xFFFFFFFF0E0D0C0A,
                                   (si64)0x0908060504020100);

    // 32 entries in each loop
    const __m256i* sp0 = (__m256i*)ln0->i32;
    const __m256i* sp1 = (__m256i*)ln1->i32;
    const __m256i* sp2 = (__m256i*)ln2->i32;
    ui8* p = (ui8*)dp;
    for ( ; count >= 32; count -= 32, sp0 += 4, sp1 += 4, sp2 += 4, p += 96)
    {
      __m256i a, t, u, v, w;

      a = _mm256_load_si256(sp0);
      a = _mm256_max_epi32(a, zero);
      t = _mm256_min_epi32(a, max_val_vec);

      a = _mm256_load_si256(sp1);
      a = _mm256_max_epi32(a, zero);
      a = _mm256_min_epi32(a, max_val_vec);
      a = _mm256_slli_epi32(a, 8);
      t = _mm256_or_si256(t, a);

      a = _mm256_load_si256(sp2);
      a = _mm256_max_epi32(a, zero);
      a = _mm256_min_epi32(a, max_val_vec);
      a = _mm256_slli_epi32(a, 16);
      t = _mm256_or_si256(t, a);
      t = _mm256_shuffle_epi8(t, m0);


      a = _mm256_load_si256(sp0 + 1);
      a = _mm256_max_epi32(a, zero);
      u = _mm256_min_epi32(a, max_val_vec);

      a = _mm256_load_si256(sp1 + 1);
      a = _mm256_max_epi32(a, zero);
      a = _mm256_min_epi32(a, max_val_vec);
      a = _mm256_slli_epi32(a, 8);
      u = _mm256_or_si256(u, a);

      a = _mm256_load_si256(sp2 + 1);
      a = _mm256_max_epi32(a, zero);
      a = _mm256_min_epi32(a, max_val_vec);
      a = _mm256_slli_epi32(a, 16);
      u = _mm256_or_si256(u, a);
      u = _mm256_shuffle_epi8(u, m0);


      a = _mm256_load_si256(sp0 + 2);
      a = _mm256_max_epi32(a, zero);
      v = _mm256_min_epi32(a, max_val_vec);

      a = _mm256_load_si256(sp1 + 2);
      a = _mm256_max_epi32(a, zero);
      a = _mm256_min_epi32(a, max_val_vec);
      a = _mm256_slli_epi32(a, 8);
      v = _mm256_or_si256(v, a);

      a = _mm256_load_si256(sp2 + 2);
      a = _mm256_max_epi32(a, zero);
      a = _mm256_min_epi32(a, max_val_vec);
      a = _mm256_slli_epi32(a, 16);
      v = _mm256_or_si256(v, a);
      v = _mm256_shuffle_epi8(v, m0);


      a = _mm256_load_si256(sp0 + 3);
      a = _mm256_max_epi32(a, zero);
      w = _mm256_min_epi32(a, max_val_vec);

      a = _mm256_load_si256(sp1 + 3);
      a = _mm256_max_epi32(a, zero);
      a = _mm256_min_epi32(a, max_val_vec);
      a = _mm256_slli_epi32(a, 8);
      w = _mm256_or_si256(w, a);

      a = _mm256_load_si256(sp2 + 3);
      a = _mm256_max_epi32(a, zero);
      a = _mm256_min_epi32(a, max_val_vec);
      a = _mm256_slli_epi32(a, 16);
      w = _mm256_or_si256(w, a);
      w = _mm256_shuffle_epi8(w, m0);

      _mm_storeu_si128((__m128i*)(p     ), _mm256_castsi256_si128(t));
      _mm_storeu_si128((__m128i*)(p + 12), _mm256_extracti128_si256(t,1));
      _mm_storeu_si128((__m128i*)(p + 24), _mm256_castsi256_si128(u));
      _mm_storeu_si128((__m128i*)(p + 36), _mm256_extracti128_si256(u,1));
      _mm_storeu_si128((__m128i*)(p + 48), _mm256_castsi256_si128(v));
      _mm_storeu_si128((__m128i*)(p + 60), _mm256_extracti128_si256(v,1));
      _mm_storeu_si128((__m128i*)(p + 72), _mm256_castsi256_si128(w));
#ifdef OJPH_ARCH_X86_64      
      *((si64*)(p + 84)) = _mm256_extract_epi64(w, 2);
#elif (defined OJPH_ARCH_I386)
      *((si32*)(p + 84)) = _mm256_extract_epi32(w, 4);
      *((si32*)(p + 88)) = _mm256_extract_epi32(w, 5);
#else
      #error Error unsupport compiler
#endif
      *((si32*)(p + 92)) = _mm256_extract_epi32(w, 6);

      // this is an alterative slower implementation
      //__m256i tx, ux, vx, wx;
      //tx = _mm256_permute2x128_si256(t, v, 0x20);
      //ux = _mm256_permute2x128_si256(t, v, 0x31);
      //vx = _mm256_permute2x128_si256(u, w, 0x20);
      //wx = _mm256_permute2x128_si256(u, w, 0x31);

      //tx = _mm256_or_si256(tx, _mm256_bslli_epi128(ux, 12));
      //ux = _mm256_or_si256(_mm256_bsrli_epi128(ux, 4), 
      //                     _mm256_bslli_epi128(vx, 8));
      //vx = _mm256_or_si256(_mm256_bsrli_epi128(vx, 8), 
      //                     _mm256_bslli_epi128(wx, 4));

      //a = _mm256_permute2x128_si256(tx, ux, 0x20);
      //_mm256_storeu_si256(p, a);
      //a = _mm256_permute2x128_si256(vx, tx, 0x30);
      //_mm256_storeu_si256(p + 1, a);
      //a = _mm256_permute2x128_si256(ux, vx, 0x31);
      //_mm256_storeu_si256(p + 2, a);
    }

    const si32* ssp0 = (si32*)sp0;
    const si32* ssp1 = (si32*)sp1;
    const si32* ssp2 = (si32*)sp2;
    for ( ; count > 0; --count)
    {
      int val;
      val = *ssp0++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui8) val;
      val = *ssp1++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui8) val;
      val = *ssp2++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui8) val;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  void avx2_cvrt_32b1c_to_16ub1c_le(const line_buf *ln0, const line_buf *ln1, 
                                    const line_buf *ln2, void *dp, 
                                    ui32 bit_depth, ui32 count)
  {
    ojph_unused(ln1);
    ojph_unused(ln2);

    __m256i max_val_vec = _mm256_set1_epi32((1 << bit_depth) - 1);
    __m256i zero = _mm256_setzero_si256();
    __m256i mask = _mm256_set_epi64x(0x0F0E0B0A07060302, 0x0D0C090805040100,
                                     0x0F0E0B0A07060302, 0x0D0C090805040100);
    const si32 *sp = ln0->i32;
    ui16* p = (ui16 *)dp;

    // 16 entries in each loop
    for ( ; count >= 16; count -= 16, sp += 16, p += 16) 
    {
      __m256i a, t;
      a = _mm256_load_si256((__m256i*)sp);
      a = _mm256_max_epi32(a, zero);
      t = _mm256_min_epi32(a, max_val_vec);

      a = _mm256_load_si256((__m256i*)sp + 1);
      a = _mm256_max_epi32(a, zero);
      a = _mm256_min_epi32(a, max_val_vec);
      a = _mm256_slli_epi32(a, 16);
      t = _mm256_or_si256(t, a);

      t = _mm256_shuffle_epi8(t, mask);
      t = _mm256_permute4x64_epi64(t, 0xD8);
      _mm256_storeu_si256((__m256i*)p, t);
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
  void avx2_cvrt_32b1c_to_16ub1c_be(const line_buf *ln0, const line_buf *ln1, 
                                    const line_buf *ln2, void *dp, 
                                    ui32 bit_depth, ui32 count)
  {
    ojph_unused(ln1);
    ojph_unused(ln2);

    __m256i max_val_vec = _mm256_set1_epi32((1 << bit_depth) - 1);
    __m256i zero = _mm256_setzero_si256();
    __m256i mask = _mm256_set_epi64x(0x0E0F0A0B06070203, 0x0C0D080904050001,
                                     0x0E0F0A0B06070203, 0x0C0D080904050001);
    const si32 *sp = ln0->i32;
    ui16* p = (ui16 *)dp;

    // 16 entries in each loop
    for ( ; count >= 16; count -= 16, sp += 16, p += 16) 
    {
      __m256i a, t;
      a = _mm256_load_si256((__m256i*)sp);
      a = _mm256_max_epi32(a, zero);
      t = _mm256_min_epi32(a, max_val_vec);

      a = _mm256_load_si256((__m256i*)sp + 1);
      a = _mm256_max_epi32(a, zero);
      a = _mm256_min_epi32(a, max_val_vec);
      a = _mm256_slli_epi32(a, 16);
      t = _mm256_or_si256(t, a);

      t = _mm256_shuffle_epi8(t, mask);
      t = _mm256_permute4x64_epi64(t, 0xD8);
      _mm256_storeu_si256((__m256i*)p, t);
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
}

#endif
