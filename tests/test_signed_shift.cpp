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
// File: test_signed_shift.cpp
//***************************************************************************/
// Round trips of signed images whose samples are negative, through code
// paths that double a sample.  Left-shifting a negative value is undefined
// behaviour before C++20, so these paths must multiply instead.  The tests
// check that the round trips are exact; built with
// -fsanitize=shift -fno-sanitize-recover=shift they also check that no
// negative value is shifted left.
//
//   * The reversible colour transform (RCT) computes (R + 2G + B) >> 2.
//     The 32 bit path is taken for a bit depth that fits a 32 bit line, and
//     the 64 bit path for a bit depth that does not.
//   * The reversible horizontal analysis of a line that is one sample wide
//     and starts at an odd position doubles that sample.
//***************************************************************************/

#include <vector>

#include "ojph_mem.h"
#include "ojph_file.h"
#include "ojph_codestream.h"
#include "ojph_params.h"
#include "gtest/gtest.h"

namespace {

  ///////////////////////////////////////////////////////////////////////////
  // Encodes a signed, reversible image held in samples (one vector per
  // component) into memory, decodes it, and checks that the decoded samples
  // equal the encoded ones.  The image starts at (x_off, 0) so that its
  // first column can be odd.
  ///////////////////////////////////////////////////////////////////////////
  void round_trip(ojph::ui32 width, ojph::ui32 height, ojph::ui32 num_comps,
                  ojph::ui32 bit_depth, ojph::ui32 x_off,
                  bool color_transform, ojph::ui32 num_decomps,
                  const std::vector<std::vector<ojph::si32> >& samples)
  {
    ojph::mem_outfile out;
    out.open();
    {
      ojph::codestream codestream;

      ojph::param_siz siz = codestream.access_siz();
      siz.set_image_extent(ojph::point(x_off + width, height));
      siz.set_image_offset(ojph::point(x_off, 0));
      siz.set_tile_size(ojph::size(x_off + width, height));
      siz.set_tile_offset(ojph::point(0, 0));
      siz.set_num_components(num_comps);
      for (ojph::ui32 c = 0; c < num_comps; ++c)
        siz.set_component(c, ojph::point(1, 1), bit_depth, true);

      ojph::param_cod cod = codestream.access_cod();
      cod.set_num_decomposition(num_decomps);
      cod.set_block_dims(64, 64);
      cod.set_color_transform(color_transform);
      cod.set_reversible(true);

      codestream.set_planar(false);
      codestream.write_headers(&out);

      ojph::ui32 next_comp;
      ojph::line_buf* cur_line = codestream.exchange(NULL, next_comp);
      for (ojph::ui32 y = 0; y < height; ++y)
        for (ojph::ui32 c = 0; c < num_comps; ++c)
        {
          ASSERT_EQ(next_comp, c);
          for (ojph::ui32 x = 0; x < width; ++x)
            cur_line->i32[x] = samples[c][y * width + x];
          cur_line = codestream.exchange(cur_line, next_comp);
        }
      codestream.flush();
      codestream.close();
    }

    ojph::mem_infile in;
    in.open(out.get_data(), out.get_used_size());
    {
      ojph::codestream codestream;
      codestream.read_headers(&in);
      codestream.set_planar(false);
      codestream.create();

      for (ojph::ui32 y = 0; y < height; ++y)
        for (ojph::ui32 c = 0; c < num_comps; ++c)
        {
          ojph::ui32 comp_num;
          ojph::line_buf* line = codestream.pull(comp_num);
          ASSERT_EQ(comp_num, c);
          for (ojph::ui32 x = 0; x < width; ++x)
            ASSERT_EQ(line->i32[x], samples[c][y * width + x])
              << "component " << c << " at (" << x << ", " << y << ")";
        }
      codestream.close();
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  // Three components of negative samples; the green component is the most
  // negative so that 2G is negative whatever R and B are.
  ///////////////////////////////////////////////////////////////////////////
  std::vector<std::vector<ojph::si32> >
  negative_rgb(ojph::ui32 width, ojph::ui32 height, ojph::si32 scale)
  {
    std::vector<std::vector<ojph::si32> > s(3,
      std::vector<ojph::si32>((size_t)width * height));
    for (ojph::ui32 i = 0; i < width * height; ++i)
    {
      s[0][i] = -scale * (ojph::si32)(i % 7 + 1);
      s[1][i] = -scale * (ojph::si32)(i % 5 + 8);
      s[2][i] = -scale * (ojph::si32)(i % 3 + 1);
    }
    return s;
  }

}

///////////////////////////////////////////////////////////////////////////////
// RCT on 32 bit lines, as used for bit depths up to about 30 bits
TEST(TestSignedShift, RctNegative32) {
  const ojph::ui32 width = 64, height = 16;
  round_trip(width, height, 3, 16, 0, true, 5,
             negative_rgb(width, height, 1000));
}

///////////////////////////////////////////////////////////////////////////////
// RCT on 64 bit lines, which the library selects for large bit depths
TEST(TestSignedShift, RctNegative64) {
  const ojph::ui32 width = 64, height = 16;
  round_trip(width, height, 3, 32, 0, true, 5,
             negative_rgb(width, height, 100000000));
}

///////////////////////////////////////////////////////////////////////////////
// A one sample wide image at an odd horizontal position, so that the single
// sample is a high-pass sample of the horizontal analysis
TEST(TestSignedShift, OddSingleColumnNegative) {
  const ojph::ui32 width = 1, height = 16;
  std::vector<std::vector<ojph::si32> > s(1,
    std::vector<ojph::si32>(height));
  for (ojph::ui32 y = 0; y < height; ++y)
    s[0][y] = -1000 - (ojph::si32)y;
  round_trip(width, height, 1, 16, 1, false, 1, s);
}

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
