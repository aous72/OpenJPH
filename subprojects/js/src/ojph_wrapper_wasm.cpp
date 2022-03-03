/****************************************************************************/
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
/****************************************************************************/
// This file is part of the OpenJPH software implementation.
// File: ojph_wrapper_wasm.cpp
// Author: Aous Naman
// Date: 01 March 2022
/****************************************************************************/

#include <exception>
#include <emscripten.h>
#include <wasm_simd128.h>

#include "ojph_arch.h"
#include "ojph_file.h"
#include "ojph_wrapper.h"
#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream.h"

//////////////////////////////////////////////////////////////////////////////
EMSCRIPTEN_KEEPALIVE
void simd_pull_j2c_buf8(j2k_struct *j2c)
{
  ojph::param_siz siz = j2c->codestream.access_siz();
  int width = siz.get_recon_width(0);
  int height = siz.get_recon_height(0);
  int num_comps = siz.get_num_components();
  
  int bit_depth = siz.get_bit_depth(0);
  int shift = bit_depth >= 8 ? bit_depth - 8 : 0;
  int half = shift > 0 ? (1<<(shift-1)) : 0;
  bool is_signed = siz.is_signed(0);
  half += is_signed ? (1 << (bit_depth - 1)) : 0;
  
  if (j2c->buffer == NULL)
    j2c->buffer = new unsigned char[width * height * 4];

  if (num_comps == 1) {
  
  }
  else {
    for (ojph::ui32 i = 0; i < height; ++i)
    {
      ojph::ui32 comp_num;
      signed int *sp0 = j2c->codestream.pull(comp_num)->i32;
      signed int *sp1 = j2c->codestream.pull(comp_num)->i32;
      signed int *sp2 = j2c->codestream.pull(comp_num)->i32;
      unsigned char *dp = j2c->buffer + i * width * 4;
      int tw = width;
      
      v128_t zero = wasm_i32x4_splat(0);
      v128_t vhalf = wasm_i32x4_splat(half);
      v128_t v255  = wasm_i32x4_splat(255);
      v128_t *vdp  = (v128_t *)dp;
      v128_t *vsp0 = (v128_t *)sp0;
      v128_t *vsp1 = (v128_t *)sp1;
      v128_t *vsp2 = (v128_t *)sp2;
      int repeat = tw >> 2;
      for (int x = repeat; x > 0; --x)
      {
        v128_t t0 = wasm_i32x4_add(wasm_v128_load(vsp0++), vhalf);
        v128_t t1 = wasm_i32x4_add(wasm_v128_load(vsp1++), vhalf);
        v128_t t2 = wasm_i32x4_add(wasm_v128_load(vsp2++), vhalf);
        
        t0 = wasm_i32x4_shr(t0, shift);
        t1 = wasm_i32x4_shr(t1, shift);
        t2 = wasm_i32x4_shr(t2, shift);
        
        v128_t u0 = wasm_i32x4_shuffle(t0, t1, 0, 4, 1, 5);
        v128_t u1 = wasm_i32x4_shuffle(t0, t1, 2, 6, 3, 7);
        v128_t u2 = wasm_i32x4_shuffle(t2, v255, 0, 4, 1, 5);
        v128_t u3 = wasm_i32x4_shuffle(t2, v255, 2, 6, 3, 7);
   
        v128_t v0 = wasm_i64x2_shuffle(u0, u2, 0, 2);
        v128_t v1 = wasm_i64x2_shuffle(u0, u2, 1, 3);
        v128_t v2 = wasm_i64x2_shuffle(u1, u3, 0, 2);
        v128_t v3 = wasm_i64x2_shuffle(u1, u3, 1, 3);
        
        v128_t w0 = wasm_u16x8_narrow_i32x4(v0, v1);
        v128_t w1 = wasm_u16x8_narrow_i32x4(v2, v3);
        v128_t z0 = wasm_u8x16_narrow_i16x8(w0, w1);
        
        wasm_v128_store(vdp++, z0);
      }
     
      dp = (unsigned char *)vdp;
      sp0 = (signed int *)vsp0;
      sp1 = (signed int *)vsp1;
      sp2 = (signed int *)vsp2;
      tw -= repeat << 2;
   
      for (int x = tw; x > 0; --x)
      {
        int val;
        val = (*sp0++ + half) >> shift;
        val = val >= 0 ? val : 0;
        val = val <= 255 ? val : 255;
        *dp++ = val;
        val = (*sp1++ + half) >> shift;
        val = val >= 0 ? val : 0;
        val = val <= 255 ? val : 255;
        *dp++ = val;
        val = (*sp2++ + half) >> shift;
        val = val >= 0 ? val : 0;
        val = val <= 255 ? val : 255;
        *dp++ = val;
        *dp++ = 255u;
      }
    }
  }
}
