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


#include <cstdlib>
#include <cstring>

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
                                 int bit_depth, int count)
  {
    ojph_unused(ln1);
    ojph_unused(ln2);

    // 32 bytes or entries in each loop
    // int loops = count & ~31;
    // count &= 31;

    int max_val = (1 << bit_depth) - 1;
    const si32 *sp = ln0->i32;
    ui8* p = (ui8 *)dp;
    for (ui32 i = count; i > 0; --i)
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
                                int bit_depth, int count)
  {
    int max_val = (1<<bit_depth) - 1;
    const si32 *sp0 = ln0->i32;
    const si32 *sp1 = ln1->i32;
    const si32 *sp2 = ln2->i32;
    ui8* p = (ui8 *)dp;
    for (ui32 i = count; i > 0; --i)
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
  void avx2_cvrt_32b1c_to_16ub1c_le(const line_buf *ln0, const line_buf *ln1, 
                                    const line_buf *ln2, void *dp, 
                                    int bit_depth, int count)
  {
    ojph_unused(ln1);
    ojph_unused(ln2);

    // 16 entries in each loop
    // int loops = count & ~15;
    // count &= 15;

    int max_val = (1<<bit_depth) - 1;
    const si32 *sp = ln0->i32;
    ui16* p = (ui16*)dp;
    for (ui32 i = count; i > 0; --i)
    {
      int val = *sp++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui16) val;
    }    
  }

  /////////////////////////////////////////////////////////////////////////////
  void avx2_cvrt_32b3c_to_16ub3c_le(const line_buf *ln0, const line_buf *ln1, 
                                    const line_buf *ln2, void *dp, 
                                    int bit_depth, int count)
  {
    int max_val = (1<<bit_depth) - 1;
    const si32 *sp0 = ln0->i32;
    const si32 *sp1 = ln1->i32;
    const si32 *sp2 = ln2->i32;
    ui16* p = (ui16*)dp;
    for (ui32 i = count; i > 0; --i)
    {
      int val;
      val = *sp0++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui16) val;
      val = *sp1++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui16) val;
      val = *sp2++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui16) val;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  void avx2_cvrt_32b1c_to_16ub1c_be(const line_buf *ln0, const line_buf *ln1, 
                                    const line_buf *ln2, void *dp, 
                                    int bit_depth, int count)
  {
    ojph_unused(ln1);
    ojph_unused(ln2);

    // 16 entries in each loop
    // int loops = count & ~15;
    // count &= 15;

    int max_val = (1<<bit_depth) - 1;
    const si32 *sp = ln0->i32;
    ui16* p = (ui16*)dp;
    for (ui32 i = count; i > 0; --i)
    {
      int val = *sp++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = be2le((ui16) val);
    }    
  }

  /////////////////////////////////////////////////////////////////////////////
  void avx2_cvrt_32b3c_to_16ub3c_be(const line_buf *ln0, const line_buf *ln1, 
                                    const line_buf *ln2, void *dp, 
                                    int bit_depth, int count)
  {
    int max_val = (1<<bit_depth) - 1;
    const si32 *sp0 = ln0->i32;
    const si32 *sp1 = ln1->i32;
    const si32 *sp2 = ln2->i32;
    ui16* p = (ui16*)dp;
    for (ui32 i = count; i > 0; --i)
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
