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
// File: test_placeholder_passes.cpp
//***************************************************************************/
// A codeblock that signals placeholder passes has its missing MSBs
// increased by one for every three placeholder passes, after the missing
// MSBs have been checked against Kmax.  The codestream below is an 8x8,
// single component, signed 32 bit reversible image, which is decoded on the
// 64 bit path.  Its only codeblock signals 163 coding passes (54 groups of
// placeholder passes), so the decoder sees 84 missing MSBs, more than the
// 62 bits the 64 bit block decoder can represent.  Such a codeblock must be
// rejected, rather than decoded with a negative least significant
// bitplane.
//***************************************************************************/

#include <stdexcept>

#include "ojph_mem.h"
#include "ojph_file.h"
#include "ojph_codestream.h"
#include "ojph_params.h"
#include "gtest/gtest.h"

namespace {

  const ojph::ui8 codestream_data[] = {
    0xFF, 0x4F, 0xFF, 0x51, 0x00, 0x29, 0x40, 0x00, 0x00, 0x00, 0x00, 0x08,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x9F, 0x01, 0x01, 0xFF, 0x50, 0x00,
    0x08, 0x00, 0x02, 0x00, 0x00, 0x00, 0x14, 0xFF, 0x52, 0x00, 0x0C, 0x00,
    0x02, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x40, 0x01, 0xFF, 0x5C, 0x00,
    0x04, 0x20, 0xF8, 0xFF, 0x90, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x4E, 0x00, 0x01, 0xFF, 0x93, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0x7F, 0xB8,
    0x06, 0xE0, 0x64, 0x9E, 0x2C, 0x84, 0x52, 0x12, 0x34, 0x49, 0x87, 0xED,
    0x33, 0xFD, 0xB9, 0x0D, 0x51, 0xA4, 0xB8, 0x38, 0xBB, 0xF3, 0x1C, 0xDD,
    0x4C, 0x6B, 0x33, 0xCE, 0x37, 0x16, 0x25, 0x20, 0x84, 0x02, 0xF3, 0x54,
    0x71, 0xC0, 0x3F, 0xC4, 0x05, 0x22, 0xAC, 0x12, 0xFA, 0x19, 0x59, 0xF4,
    0xB5, 0x57, 0x8B, 0x92, 0x40, 0x28, 0xB3, 0x31, 0x01, 0xFF, 0xD9,
  };

  const ojph::ui32 width = 8, height = 8;

  ///////////////////////////////////////////////////////////////////////////
  // Decodes the codestream above, checking every decoded sample with
  // check_sample, which is called with the sample value.
  ///////////////////////////////////////////////////////////////////////////
  template<typename F>
  void decode(bool resilient, F check_sample)
  {
    ojph::mem_infile in;
    in.open(codestream_data, sizeof(codestream_data));

    ojph::codestream codestream;
    if (resilient)
      codestream.enable_resilience();
    codestream.read_headers(&in);
    ASSERT_EQ(codestream.access_siz().get_bit_depth(0), 32u);
    codestream.create();

    for (ojph::ui32 y = 0; y < height; ++y)
    {
      ojph::ui32 comp_num;
      ojph::line_buf* line = codestream.pull(comp_num);
      ASSERT_EQ(comp_num, 0u);
      for (ojph::ui32 x = 0; x < width; ++x)
        check_sample(line->i32[x]);
    }
    codestream.close();
  }

}

///////////////////////////////////////////////////////////////////////////////
// Without resilience, the codeblock is reported as an error
TEST(TestPlaceholderPasses, MissingMsbsTooLarge64) {
  EXPECT_THROW(decode(false, [](ojph::si32) {}), std::exception);
}

///////////////////////////////////////////////////////////////////////////////
// With resilience, the codeblock is dropped, and its samples are zero
TEST(TestPlaceholderPasses, MissingMsbsTooLarge64Resilient) {
  decode(true, [](ojph::si32 v) { ASSERT_EQ(v, 0); });
}

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
