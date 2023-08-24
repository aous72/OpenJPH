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
// File: ojph_codeblock_fun.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#include <climits>
#include <cmath>

#include "ojph_file.h"
#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream.h"
#include "ojph_codestream_local.h"
#include "ojph_codeblock_fun.h"

#include "../transform/ojph_colour.h"
#include "../transform/ojph_transform.h"
#include "../coding/ojph_block_decoder.h"
#include "../coding/ojph_block_encoder.h"

namespace ojph {

  namespace local
  {

    //////////////////////////////////////////////////////////////////////////
    void gen_mem_clear(void* addr, size_t count);
    void sse_mem_clear(void* addr, size_t count);
    void avx_mem_clear(void* addr, size_t count);
    void wasm_mem_clear(void* addr, size_t count);

    //////////////////////////////////////////////////////////////////////////
    ui32 gen_find_max_val(ui32* address);
    ui32 sse2_find_max_val(ui32* address);
    ui32 avx2_find_max_val(ui32* address);
    ui32 wasm_find_max_val(ui32* address);

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                           float delta_inv, ui32 count, ui32* max_val);
    void sse2_rev_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                           float delta_inv, ui32 count, ui32* max_val);
    void avx2_rev_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                           float delta_inv, ui32 count, ui32* max_val);
    void gen_irv_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                           float delta_inv, ui32 count, ui32* max_val);
    void sse2_irv_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                           float delta_inv, ui32 count, ui32* max_val);
    void avx2_irv_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                           float delta_inv, ui32 count, ui32* max_val);
    void wasm_rev_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                           float delta_inv, ui32 count, ui32* max_val);
    void wasm_irv_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                           float delta_inv, ui32 count, ui32* max_val);

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                             float delta, ui32 count);
    void sse2_rev_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                             float delta, ui32 count);
    void avx2_rev_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                             float delta, ui32 count);
    void gen_irv_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                             float delta, ui32 count);
    void sse2_irv_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                             float delta, ui32 count);
    void avx2_irv_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                             float delta, ui32 count);
    void wasm_rev_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                             float delta, ui32 count);
    void wasm_irv_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                             float delta, ui32 count);


    void codeblock_fun::init(bool reversible) {

#if !defined(OJPH_ENABLE_WASM_SIMD) || !defined(OJPH_EMSCRIPTEN)

      // Default path, no acceleration.  We may change this later
      decode_cb = ojph_decode_codeblock;
      find_max_val = gen_find_max_val;
      mem_clear = gen_mem_clear;
      if (reversible) {
        tx_to_cb = gen_rev_tx_to_cb;
        tx_from_cb = gen_rev_tx_from_cb;
      }
      else
      {
        tx_to_cb = gen_irv_tx_to_cb;
        tx_from_cb = gen_irv_tx_from_cb;
      }
      encode_cb = ojph_encode_codeblock;

#ifndef OJPH_DISABLE_INTEL_SIMD

      // Accelerated functions for INTEL/AMD CPUs
      if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_SSE)
        mem_clear = sse_mem_clear;

      if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_SSE2) {
        find_max_val = sse2_find_max_val;
        if (reversible) {
          tx_to_cb = sse2_rev_tx_to_cb;
          tx_from_cb = sse2_rev_tx_from_cb;
        }
        else {
          tx_to_cb = sse2_irv_tx_to_cb;
          tx_from_cb = sse2_irv_tx_from_cb;
        }
      }

      if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_SSSE3)
        decode_cb = ojph_decode_codeblock_ssse3;


      if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_AVX)
        mem_clear = avx_mem_clear;

      if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_AVX2) {
        find_max_val = avx2_find_max_val;
        if (reversible) {
          tx_to_cb = avx2_rev_tx_to_cb;
          tx_from_cb = avx2_rev_tx_from_cb;
        }
        else {
          tx_to_cb = avx2_irv_tx_to_cb;
          tx_from_cb = avx2_irv_tx_from_cb;
        }
      }

#ifdef OJPH_ENABLE_INTEL_AVX512
      if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_AVX512)
        encode_cb = ojph_encode_codeblock_avx512;
#endif // !OJPH_ENABLE_INTEL_AVX512

#endif // !OJPH_DISABLE_INTEL_SIMD

#else // OJPH_ENABLE_WASM_SIMD

      // Accelerated functions for WASM SIMD.
      decode_cb = ojph_decode_codeblock_wasm;
      find_max_val = wasm_find_max_val;
      mem_clear = wasm_mem_clear;
      if (reversible) {
        tx_to_cb = wasm_rev_tx_to_cb;
        tx_from_cb = wasm_rev_tx_from_cb;
      }
      else {
        tx_to_cb = wasm_irv_tx_to_cb;
        tx_from_cb = wasm_irv_tx_from_cb;
      }
      encode_cb = ojph_encode_codeblock;

#endif // !OJPH_ENABLE_WASM_SIMD

    }
  }  // local
}  // ojph
