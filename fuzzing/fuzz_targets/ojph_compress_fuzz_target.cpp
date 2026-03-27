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
// File: ojph_compress_fuzz_target.cpp
// Fuzz target for the HTJ2K encoding (compression) path.
//***************************************************************************/

#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "ojph_mem.h"
#include "ojph_file.h"
#include "ojph_codestream.h"
#include "ojph_params.h"

// Input layout (4 control bytes + pixel data):
//   byte 0: [6:0] width-1  (1..128)
//   byte 1: [6:0] height-1 (1..128)
//   byte 2: [1:0] num_components-1 (1..4)
//           [3:2] bit_depth selector (8,10,12,16)
//           [4]   is_signed
//           [5]   reversible
//           [6]   color_transform
//   byte 3: [2:0] num_decompositions (0..5, clamped)
//           [3]   planar
//   bytes 4+: pixel data (each byte becomes one sample)

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
  if (Size < 5)
    return 0;

  ojph::ui32 width     = (Data[0] & 0x7F) + 1;
  ojph::ui32 height    = (Data[1] & 0x7F) + 1;
  ojph::ui32 num_comps = (Data[2] & 0x03) + 1;
  ojph::ui32 bit_depth = (ojph::ui32[]){8, 10, 12, 16}[(Data[2] >> 2) & 0x03];
  bool is_signed       = (Data[2] >> 4) & 1;
  bool reversible      = (Data[2] >> 5) & 1;
  bool color_transform = (Data[2] >> 6) & 1;
  ojph::ui32 num_decomps = Data[3] & 0x07;
  bool planar          = (Data[3] >> 3) & 1;

  if (num_decomps > 5) num_decomps = 5;
  if (num_comps < 3)   color_transform = false;
  if (color_transform) planar = false;

  const uint8_t *pixels = Data + 4;
  size_t pixels_len = Size - 4;
  size_t pix_idx = 0;

  try
  {
    ojph::codestream cs;

    ojph::param_siz siz = cs.access_siz();
    siz.set_image_extent(ojph::point(width, height));
    siz.set_num_components(num_comps);
    for (ojph::ui32 c = 0; c < num_comps; ++c)
      siz.set_component(c, ojph::point(1, 1), bit_depth, is_signed);

    ojph::param_cod cod = cs.access_cod();
    cod.set_num_decomposition(num_decomps);
    cod.set_color_transform(color_transform);
    cod.set_reversible(reversible);

    if (!reversible)
      cs.access_qcd().set_irrev_quant(0.0005f);

    cs.set_planar(planar);

    ojph::mem_outfile outfile;
    outfile.open();
    cs.write_headers(&outfile);

    // Total rows to push: planar processes each component fully,
    // interleaved processes one row from all components at a time.
    ojph::ui32 total_rows = num_comps * height;
    ojph::ui32 next_comp;
    ojph::line_buf *line = cs.exchange(NULL, next_comp);

    for (ojph::ui32 r = 0; r < total_rows; ++r)
    {
      ojph::si32 *dp = line->i32;
      for (ojph::ui32 x = 0; x < width; ++x)
      {
        // Use fuzz bytes as sample values, wrapping around as needed
        ojph::si32 val = (ojph::si32)pixels[pix_idx % pixels_len];
        pix_idx++;
        dp[x] = is_signed ? val - 128 : val;
      }
      line = cs.exchange(line, next_comp);
    }

    cs.flush();
    cs.close();
  }
  catch (const std::exception &)
  {
  }
  return 0;
}
