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
// File: test_irv_precision64.cpp
//***************************************************************************/
// Decoding of an irreversible codestream whose QCD marker segment makes the
// decoder use 64 bit codeblock buffers.  The QCD step size exponent is not
// part of the coded data once the codeblocks are coded, because K_max - the
// exponent plus the guard bits minus one - only sets where the decoded
// magnitudes start; raising the exponent of an existing codestream so that
// K_max is 31 selects the 64 bit path while leaving the reconstructed
// samples unchanged.  The test checks that both paths decode to the same
// samples.  The 64 bit path once wrote its float samples with an 8 byte
// stride, past the end of the subband line.
//***************************************************************************/

#include <vector>

#include "ojph_mem.h"
#include "ojph_file.h"
#include "ojph_codestream.h"
#include "ojph_params.h"
#include "gtest/gtest.h"

namespace {

  ///////////////////////////////////////////////////////////////////////////
  // Encodes an 8 bit, single component, irreversible image without
  // decompositions, so that its single subband spans several codeblocks
  ///////////////////////////////////////////////////////////////////////////
  void encode(ojph::ui32 width, ojph::ui32 height,
              const std::vector<ojph::si32>& samples, ojph::mem_outfile& out)
  {
    out.open();
    ojph::codestream codestream;

    ojph::param_siz siz = codestream.access_siz();
    siz.set_image_extent(ojph::point(width, height));
    siz.set_image_offset(ojph::point(0, 0));
    siz.set_tile_size(ojph::size(width, height));
    siz.set_tile_offset(ojph::point(0, 0));
    siz.set_num_components(1);
    siz.set_component(0, ojph::point(1, 1), 8, false);

    ojph::param_cod cod = codestream.access_cod();
    cod.set_num_decomposition(0);
    cod.set_block_dims(64, 64);
    cod.set_color_transform(false);
    cod.set_reversible(false);
    codestream.access_qcd().set_irrev_quant(0.01f);

    codestream.set_planar(false);
    codestream.write_headers(&out);

    ojph::ui32 next_comp;
    ojph::line_buf* cur_line = codestream.exchange(NULL, next_comp);
    for (ojph::ui32 y = 0; y < height; ++y)
    {
      for (ojph::ui32 x = 0; x < width; ++x)
        cur_line->i32[x] = samples[y * width + x];
      cur_line = codestream.exchange(cur_line, next_comp);
    }
    codestream.flush();
    codestream.close();
  }

  ///////////////////////////////////////////////////////////////////////////
  // Decodes a single component codestream into samples
  ///////////////////////////////////////////////////////////////////////////
  void decode(const ojph::ui8* data, size_t size, ojph::ui32 width,
              ojph::ui32 height, std::vector<ojph::si32>& samples)
  {
    samples.assign((size_t)width * height, 0);
    ojph::mem_infile in;
    in.open(data, size);
    ojph::codestream codestream;
    codestream.read_headers(&in);
    codestream.set_planar(false);
    codestream.create();
    for (ojph::ui32 y = 0; y < height; ++y)
    {
      ojph::ui32 comp_num;
      ojph::line_buf* line = codestream.pull(comp_num);
      for (ojph::ui32 x = 0; x < width; ++x)
        samples[y * width + x] = line->i32[x];
    }
    codestream.close();
  }

}

///////////////////////////////////////////////////////////////////////////////
// An irreversible subband decoded through 64 bit codeblock buffers
TEST(TestIrvPrecision64, SameSamplesAs32BitPath) {
  const ojph::ui32 width = 256, height = 16;
  std::vector<ojph::si32> samples((size_t)width * height);
  for (ojph::ui32 i = 0; i < width * height; ++i)
    samples[i] = (ojph::si32)((i * 37 + (i >> 5) * 11) & 0xFF);

  ojph::mem_outfile out;
  encode(width, height, samples, out);
  std::vector<ojph::ui8> cs(out.get_data(),
                            out.get_data() + out.get_used_size());

  std::vector<ojph::si32> ref;
  decode(cs.data(), cs.size(), width, height, ref);

  // find the QCD marker segment; it holds a single 16 bit step size
  size_t pos = 2; // skip SOC
  while (pos + 4 <= cs.size()
         && !(cs[pos] == 0xFF && cs[pos + 1] == 0x5C))
    pos += 2 + (size_t)((cs[pos + 2] << 8) | cs[pos + 3]);
  ASSERT_LE(pos + 7, cs.size());
  ASSERT_EQ((cs[pos + 2] << 8) | cs[pos + 3], 5);
  ojph::ui8 Sqcd = cs[pos + 4];
  ASSERT_EQ(Sqcd & 0x1F, 2);
  ojph::ui32 guard_bits = Sqcd >> 5u;
  ASSERT_GE(guard_bits, 1u);

  // set the exponent so that K_max = exponent + guard_bits - 1 = 31,
  // which makes the decoder employ 64 bit codeblock buffers
  ojph::ui32 eps = 32 - guard_bits;
  cs[pos + 5] = (ojph::ui8)((cs[pos + 5] & 0x07) | (eps << 3));

  std::vector<ojph::si32> dec;
  decode(cs.data(), cs.size(), width, height, dec);

  for (ojph::ui32 y = 0; y < height; ++y)
    for (ojph::ui32 x = 0; x < width; ++x)
      ASSERT_EQ(dec[y * width + x], ref[y * width + x])
        << "at (" << x << ", " << y << ")";
}

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
