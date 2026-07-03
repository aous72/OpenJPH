//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) Pierre-Anthony Lemieux
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
// File: test_mixed_coc.cpp
// Author: Pierre-Anthony Lemieux
// Date: 29 June 2026
//***************************************************************************/

#include <cstring>
#include <vector>

#include "ojph_mem.h"
#include "ojph_file.h"
#include "ojph_codestream.h"
#include "ojph_params.h"
#include "gtest/gtest.h"

///////////////////////////////////////////////////////////////////////////////
// Test encoding a 4-component image where 3 components use the irreversible
// (9/7) wavelet and 1 component uses the reversible (5/3) wavelet.
// This exercises COC and QCC marker generation for mixed coding modes.
// Verifies that encoding succeeds, headers can be read back, and the
// per-component coding settings are preserved.
TEST(TestMixedCOC, FourCompMixedReversibility) {
  const ojph::ui32 width = 64;
  const ojph::ui32 height = 64;
  const ojph::ui32 num_comps = 4;
  const ojph::ui32 bit_depth = 8;
  const ojph::ui32 num_decomps = 5;

  // samples fed into component 3 during encoding, kept around so they can
  // be compared against the decoded output further down
  std::vector<ojph::si32> comp3_orig(width * height);

  // encode
  {
    ojph::codestream codestream;

    ojph::param_siz siz = codestream.access_siz();
    siz.set_image_extent(ojph::point(width, height));
    siz.set_num_components(num_comps);
    for (ojph::ui32 c = 0; c < num_comps; ++c)
      siz.set_component(c, ojph::point(1, 1), bit_depth, false);
    siz.set_image_offset(ojph::point(0, 0));
    siz.set_tile_size(ojph::size(width, height));
    siz.set_tile_offset(ojph::point(0, 0));

    ojph::param_cod cod = codestream.access_cod();
    cod.set_num_decomposition(num_decomps);
    cod.set_block_dims(64, 64);
    cod.set_color_transform(false);
    cod.set_reversible(false);

    cod.set_reversible(3, true);

    codestream.access_qcd().set_irrev_quant(0.01f);
    codestream.set_planar(true);

    ojph::j2c_outfile j2c_file;
    j2c_file.open("mixed_rev_irrev_4comp.j2c");
    codestream.write_headers(&j2c_file);

    ojph::ui32 next_comp;
    ojph::line_buf* cur_line = codestream.exchange(NULL, next_comp);

    for (ojph::ui32 c = 0; c < num_comps; ++c)
    {
      for (ojph::ui32 y = 0; y < height; ++y)
      {
        ASSERT_EQ(next_comp, c);
        if (c == 3)
        {
          // non-constant pattern so a roundtrip mismatch would be caught
          for (ojph::ui32 x = 0; x < width; ++x)
          {
            ojph::si32 val = (ojph::si32)((x + y * width) % 256);
            cur_line->i32[x] = val;
            comp3_orig[y * width + x] = val;
          }
        }
        else if (cur_line->flags & ojph::line_buf::LFT_INTEGER)
          memset(cur_line->i32, 0,
                 sizeof(ojph::si32) * cur_line->size);
        else
          memset(cur_line->f32, 0,
                 sizeof(float) * cur_line->size);
        cur_line = codestream.exchange(cur_line, next_comp);
      }
    }

    codestream.flush();
    codestream.close();
  }

  // read back headers and verify per-component settings
  {
    ojph::codestream codestream;
    ojph::j2c_infile j2c_file;
    j2c_file.open("mixed_rev_irrev_4comp.j2c");
    codestream.read_headers(&j2c_file);

    ojph::param_siz siz = codestream.access_siz();
    EXPECT_EQ(siz.get_num_components(), num_comps);
    for (ojph::ui32 c = 0; c < num_comps; ++c)
      EXPECT_EQ(siz.get_bit_depth(c), bit_depth);

    ojph::param_cod cod = codestream.access_cod();
    EXPECT_FALSE(cod.is_reversible());

    for (ojph::ui32 c = 0; c < 3; ++c) {
      EXPECT_FALSE(cod.is_reversible(c))
        << "Component " << c << " should be irreversible";
    }

    EXPECT_TRUE(cod.is_reversible(3))
      << "Component 3 should be reversible";

    // decode all components and verify component 3 (reversible) samples
    // are identical to what was encoded
    codestream.restrict_input_resolution(0, 0);
    codestream.set_planar(true);
    codestream.create();

    for (ojph::ui32 c = 0; c < num_comps; ++c)
    {
      ojph::ui32 comp_height = siz.get_recon_height(c);
      ojph::ui32 comp_width = siz.get_recon_width(c);
      for (ojph::ui32 y = 0; y < comp_height; ++y)
      {
        ojph::ui32 comp_num;
        ojph::line_buf* line = codestream.pull(comp_num);
        ASSERT_EQ(comp_num, c);
        if (c == 3)
        {
          ASSERT_EQ(comp_width, width);
          for (ojph::ui32 x = 0; x < width; ++x)
            EXPECT_EQ(line->i32[x], comp3_orig[y * width + x])
              << "Component 3 sample mismatch at (" << x << ", " << y << ")";
        }
      }
    }

    codestream.close();
  }
}

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
