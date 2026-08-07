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
// File: test_truncated_decode.cpp
// Author: Bill Wallace
// Date: 07 August 2026
//***************************************************************************/
//
// These tests decode codestreams that were physically truncated -- the tail
// of the file is simply missing, and the decoder cannot know how long the
// original was.  This is what a partially received codestream looks like to
// a streaming client.
//
// Not every truncation is detectable.  When the cut falls inside codeblock
// data the parser stops cleanly and reconstructs from what it has; when it
// falls inside a packet header the parser has to raise an error.  It is that
// second group that codestream::enable_resilience() governs:
//
//   - resilience off (the default): the truncation is reported by throwing,
//     so a caller that needs a complete image is told the codestream is
//     broken.  Importantly it throws rather than terminating the process.
//   - resilience on: the truncation is reported through OJPH_INFO and the
//     decoder returns the part of the image it was able to reconstruct.
//
// Everything is done in memory, so the tests need no external files and run
// the same way on all platforms.

#include <stdexcept>
#include <vector>

#include "ojph_arch.h"
#include "ojph_codestream.h"
#include "ojph_file.h"
#include "ojph_mem.h"
#include "ojph_message.h"
#include "ojph_params.h"
#include "gtest/gtest.h"

namespace {

////////////////////////////////////////////////////////////////////////////////
// The image is big enough, and detailed enough, that a reversible codestream
// spans many packets, so that cutting it short lands in tile data rather than
// in the main header.
static const ojph::ui32 IMAGE_WIDTH  = 256;
static const ojph::ui32 IMAGE_HEIGHT = 256;

// The number of truncation lengths tried; the codestream is cut at each
// 1/NUM_CUTS of its length.
static const ojph::ui32 NUM_CUTS = 16;

////////////////////////////////////////////////////////////////////////////////
//                          encode_test_codestream
////////////////////////////////////////////////////////////////////////////////
// Encodes a single component 8 bit reversible image to memory and returns the
// resulting codestream.  The encoding is lossless, so the bytes produced are
// the same on every platform.
static std::vector<ojph::ui8> encode_test_codestream()
{
  ojph::codestream cs;

  ojph::param_siz siz = cs.access_siz();
  siz.set_image_extent(ojph::point(IMAGE_WIDTH, IMAGE_HEIGHT));
  siz.set_num_components(1);
  siz.set_component(0, ojph::point(1, 1), 8, false);

  ojph::param_cod cod = cs.access_cod();
  cod.set_num_decomposition(5);
  cod.set_block_dims(64, 64);
  cod.set_reversible(true);

  ojph::mem_outfile out;
  out.open();
  cs.write_headers(&out);

  ojph::ui32 next_comp = 0;
  ojph::line_buf* line = cs.exchange(NULL, next_comp);
  for (ojph::ui32 y = 0; y < IMAGE_HEIGHT; ++y)
  {
    ojph::si32* dp = line->i32;
    for (ojph::ui32 x = 0; x < IMAGE_WIDTH; ++x)
      dp[x] = (ojph::si32)((x * 7 + y * 13 + ((x * y) >> 3)) & 0xFF);
    line = cs.exchange(line, next_comp);
  }
  cs.flush();

  std::vector<ojph::ui8> buf(out.get_data(),
                             out.get_data() + (size_t)out.tell());
  cs.close();
  return buf;
}

////////////////////////////////////////////////////////////////////////////////
//                                decode_lines
////////////////////////////////////////////////////////////////////////////////
// Decodes the codestream in buf and returns the number of lines pulled.  Any
// error raised by the library propagates to the caller.
static ojph::ui32 decode_lines(const std::vector<ojph::ui8>& buf,
                               bool resilient)
{
  ojph::mem_infile in;
  in.open(buf.data(), buf.size());

  ojph::codestream cs;
  if (resilient)
    cs.enable_resilience();
  cs.read_headers(&in);
  cs.create();

  ojph::param_siz siz = cs.access_siz();
  ojph::ui32 num_lines = 0;
  for (ojph::ui32 y = siz.get_recon_height(0); y > 0; --y)
  {
    ojph::ui32 comp_num = 0;
    cs.pull(comp_num);
    ++num_lines;
  }
  cs.close();
  return num_lines;
}

////////////////////////////////////////////////////////////////////////////////
//                             truncated_decode
////////////////////////////////////////////////////////////////////////////////
class truncated_decode : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // A truncated codestream is expected to be noisy; these tests are about
    // the return path, not about the text that is printed.
    ojph::set_message_level(ojph::OJPH_MSG_NO_MSG);
    full = encode_test_codestream();
    ASSERT_GT(full.size(), NUM_CUTS * 64u)
      << "the test codestream is too small to be meaningfully truncated";
  }

  void TearDown() override
  {
    ojph::set_message_level(ojph::OJPH_MSG_ALL_MSG);
  }

  // Returns the codestream cut down to cut/NUM_CUTS of its length.
  std::vector<ojph::ui8> truncate(ojph::ui32 cut) const
  {
    size_t len = full.size() * cut / NUM_CUTS;
    return std::vector<ojph::ui8>(
      full.begin(), full.begin() + static_cast<std::ptrdiff_t>(len));
  }

  std::vector<ojph::ui8> full;
};

////////////////////////////////////////////////////////////////////////////////
// A complete codestream decodes the same way whether or not resilience is
// enabled; enabling it must not change the handling of good codestreams.
TEST_F(truncated_decode, complete_codestream_decodes_in_both_modes)
{
  EXPECT_EQ(decode_lines(full, false), IMAGE_HEIGHT);
  EXPECT_EQ(decode_lines(full, true), IMAGE_HEIGHT);
}

////////////////////////////////////////////////////////////////////////////////
// With resilience enabled, no truncation length may raise an error, and the
// decoder must still produce a full frame from whatever it received.
TEST_F(truncated_decode, resilient_mode_decodes_every_truncation_length)
{
  for (ojph::ui32 cut = 1; cut < NUM_CUTS; ++cut)
  {
    std::vector<ojph::ui8> part = truncate(cut);
    ojph::ui32 num_lines = 0;
    ASSERT_NO_THROW(num_lines = decode_lines(part, true))
      << "truncated to " << part.size() << " of " << full.size() << " bytes";
    EXPECT_EQ(num_lines, IMAGE_HEIGHT)
      << "truncated to " << part.size() << " of " << full.size() << " bytes";
  }
}

////////////////////////////////////////////////////////////////////////////////
// Without resilience, a truncation that the parser detects is reported by
// throwing -- not by terminating the process, and not by silently returning
// an incomplete image.  Truncations that fall inside codeblock data are not
// detectable and are expected to decode without an error in either mode.
TEST_F(truncated_decode, non_resilient_mode_throws_on_detected_truncation)
{
  ojph::ui32 num_detected = 0;
  for (ojph::ui32 cut = 1; cut < NUM_CUTS; ++cut)
  {
    std::vector<ojph::ui8> part = truncate(cut);
    bool detected = false;
    try {
      decode_lines(part, false);
    }
    catch (const std::runtime_error&) {
      detected = true;
    }
    catch (...) {
      FAIL() << "the library must report errors as a std::runtime_error; "
        << "truncated to " << part.size() << " of " << full.size() << " bytes";
    }
    if (detected)
    {
      ++num_detected;
      // The same input, with resilience enabled, must decode instead of
      // throwing; it is the flag, and only the flag, that decides.
      EXPECT_EQ(decode_lines(part, true), IMAGE_HEIGHT)
        << "truncated to " << part.size() << " of " << full.size() << " bytes";
    }
  }

  EXPECT_GT(num_detected, 0u)
    << "no truncation of the test codestream was detected by the parser, so "
    << "this test is no longer testing anything";
}

} // anonymous namespace
