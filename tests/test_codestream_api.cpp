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
// File: test_codestream_api.cpp
// Author: Pierre-Anthony Lemieux
// Date: 11 May 2026
//***************************************************************************/

#include "gtest/gtest.h"

#include "ojph_mem.h"
#include "ojph_file.h"
#include "ojph_codestream.h"
#include "ojph_params.h"

// Compress a 128x128 image with 4 components (3x 8-bit unsigned, 1x 32-bit
// unsigned) using irreversible coding with qstep=0.0003.
//
// The exchange lines returned by codestream::exchange() are always si32-typed
// (LFT_32BIT | LFT_INTEGER). For irreversible paths the library internally
// converts those integers to float via irv_convert_to_float, so the caller
// always fills line->i32 regardless of reversibility.
TEST(CodestreamAPI, Compress128x128x4Ch_qstep0003)
{
  const ojph::ui32 width  = 128;
  const ojph::ui32 height = 128;

  ojph::codestream cs;

  // QCD: quantization base step size.
  cs.access_qcd().set_irrev_quant(0.0003f);

  // SIZ: 128x128, components 0-2 are 8-bit unsigned, component 3 is 32-bit
  // unsigned.
  ojph::param_siz siz = cs.access_siz();
  siz.set_image_extent(ojph::point(width, height));
  siz.set_num_components(5);
  for (ojph::ui32 c = 0; c < 3; ++c)
    siz.set_component(c, ojph::point(1, 1), 8, false);
  siz.set_component(3, ojph::point(1, 1), 32, true);
  ojph::param_nlt nlt = cs.access_nlt();
  nlt.set_nonlinear_transform(3, ojph::param_nlt::OJPH_NLT_BINARY_COMPLEMENT_NLT);
  siz.set_component(4, ojph::point(1, 1), 32, false);

  // COD: irreversible, no color transform (components differ in bit depth).
  ojph::param_cod cod = cs.access_cod();
  cod.set_reversible(false);
  cod.set_color_transform(false);

  // Planar mode: each component is pushed row-by-row in full before the next.
  cs.set_planar(true);

  ojph::mem_outfile outfile;
  outfile.open();
  cs.write_headers(&outfile);

  ojph::ui32 next_comp;
  ojph::line_buf *line = cs.exchange(nullptr, next_comp);

  // Push all rows for all 4 components (planar order: comp 0 fully, then 1,
  // then 2, then 3).
  for (ojph::ui32 r = 0; r < 4 * height; ++r)
  {
    ojph::si32 *dp = line->i32;
    if (next_comp < 3)
    {
      // 8-bit unsigned: values in [0, 255].
      for (ojph::ui32 x = 0; x < width; ++x)
        dp[x] = (ojph::si32)((r * 17u + x * 3u) & 0xFFu);
    }
    else
    {
      // 32-bit unsigned: keep values positive to stay within si32 range.
      for (ojph::ui32 x = 0; x < width; ++x)
        dp[x] = (ojph::si32)((r * 100000u + x * 1000u) & 0x7FFFFFFFu);
    }
    line = cs.exchange(line, next_comp);
  }

  cs.flush();
  cs.close();

  // Verify a non-empty J2C codestream was produced (SOC marker = 0xFF 0x4F).
  EXPECT_GT(outfile.get_used_size(), 0u);
  const ojph::ui8 *data = outfile.get_data();
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data[0], 0xFF);
  EXPECT_EQ(data[1], 0x4F);
}
