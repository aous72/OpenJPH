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
// File: test_atk.cpp
//***************************************************************************/
// Tests of the arbitrary transformation kernel (ATK) marker segment of
// JPEG2000 Part 2.  The library reads ATK marker segments but does not write
// them, so the segments used here are assembled byte by byte and spliced into
// the main header of a codestream the library encoded, the way another
// encoder may have produced it.
//
// Eatk is the exponent of a reversible lifting step: the step right shifts a
// sample by it.  A sample is held in 32 or in 64 bits, so a value of 32 or
// more shifts by at least the width of the type, which is undefined
// behaviour; built with -fsanitize=shift -fno-sanitize-recover=shift these
// tests also check that no such shift is reached.
//***************************************************************************/

#include <vector>

#include "ojph_mem.h"
#include "ojph_message.h"
#include "ojph_file.h"
#include "ojph_codestream.h"
#include "ojph_params.h"
#include "gtest/gtest.h"

using namespace ojph;

// the JPEG2000 Part 2 marker segment that carries a transformation kernel;
// see JP2K_MARKER::ATK, and JP2K_MARKER::COD for the marker the kernel of a
// codestream is selected in
#define ATK_MARKER_HIGH  0xFF
#define ATK_MARKER_LOW   0x79
#define COD_MARKER_HIGH  0xFF
#define COD_MARKER_LOW   0x52

namespace {

  // one reversible lifting step of a kernel, as ATK carries it
  struct atk_step
  {
    ui8 Eatk;    // exponent, the step shifts right by it
    si16 Batk;   // additive residue
    si8 Aatk;    // lifting coefficient
  };

  ///////////////////////////////////////////////////////////////////////////
  // The samples of the test image, a pattern rather than a constant so that
  // a wavelet that does not invert exactly would be caught.
  ///////////////////////////////////////////////////////////////////////////
  std::vector<si32> test_samples(ui32 width, ui32 height)
  {
    std::vector<si32> s((size_t)width * height);
    for (ui32 i = 0; i < width * height; ++i)
      s[i] = (si32)((i * 37 + (i >> 3)) & 0xFF);
    return s;
  }

  ///////////////////////////////////////////////////////////////////////////
  // Encodes a one component reversible image into memory.
  ///////////////////////////////////////////////////////////////////////////
  void encode(mem_outfile& out, ui32 width, ui32 height,
              const std::vector<si32>& samples)
  {
    out.open();
    codestream cs;

    param_siz siz = cs.access_siz();
    siz.set_image_extent(point(width, height));
    siz.set_image_offset(point(0, 0));
    siz.set_tile_size(size(width, height));
    siz.set_tile_offset(point(0, 0));
    siz.set_num_components(1);
    siz.set_component(0, point(1, 1), 8, false);

    param_cod cod = cs.access_cod();
    cod.set_num_decomposition(2);
    cod.set_block_dims(64, 64);
    cod.set_color_transform(false);
    cod.set_reversible(true);

    cs.set_planar(false);
    cs.write_headers(&out);

    ui32 next_comp;
    line_buf* cur_line = cs.exchange(NULL, next_comp);
    for (ui32 y = 0; y < height; ++y)
    {
      for (ui32 x = 0; x < width; ++x)
        cur_line->i32[x] = samples[(size_t)y * width + x];
      cur_line = cs.exchange(cur_line, next_comp);
    }
    cs.flush();
    cs.close();
  }

  ///////////////////////////////////////////////////////////////////////////
  // Builds an ATK marker segment for a whole sample, reversible kernel whose
  // lifting coefficients are one byte each, and gives it the index kernel.
  ///////////////////////////////////////////////////////////////////////////
  std::vector<ui8> atk_marker(ui8 kernel, const std::vector<atk_step>& steps)
  {
    // Satk: constant boundary extension, reversible, whole sample, one byte
    // coefficients, and the index of this kernel
    const ui16 Satk = (ui16)(0x4000 | 0x1000 | 0x0800 | kernel);
    // Latk itself, Satk, Natk, and five bytes for every step
    const ui16 Latk = (ui16)(2 + 2 + 1 + 5 * steps.size());

    std::vector<ui8> m;
    m.push_back(ATK_MARKER_HIGH);
    m.push_back(ATK_MARKER_LOW);
    m.push_back((ui8)(Latk >> 8));
    m.push_back((ui8)(Latk & 0xFF));
    m.push_back((ui8)(Satk >> 8));
    m.push_back((ui8)(Satk & 0xFF));
    m.push_back((ui8)steps.size());
    for (size_t s = 0; s < steps.size(); ++s)
    {
      m.push_back(steps[s].Eatk);
      m.push_back((ui8)(((ui16)steps[s].Batk) >> 8));
      m.push_back((ui8)(((ui16)steps[s].Batk) & 0xFF));
      m.push_back(1);                       // LCatk, one coefficient
      m.push_back((ui8)steps[s].Aatk);
    }
    return m;
  }

  ///////////////////////////////////////////////////////////////////////////
  // Returns a copy of a codestream that carries the ATK marker segment in
  // its main header and whose COD selects the kernel the segment defines.
  ///////////////////////////////////////////////////////////////////////////
  std::vector<ui8> with_atk(const mem_outfile& encoded, ui8 kernel,
                            const std::vector<atk_step>& steps)
  {
    const ui8* p = encoded.get_data();
    std::vector<ui8> data(p, p + encoded.get_used_size());

    size_t cod = data.size();
    for (size_t i = 0; i + 1 < data.size(); ++i)
      if (data[i] == COD_MARKER_HIGH && data[i + 1] == COD_MARKER_LOW)
      {
        cod = i;
        break;
      }
    EXPECT_LT(cod, data.size()) << "the encoder wrote no COD marker segment";

    // SPcod-wavelet_trans, the last byte of a COD that has no precincts
    data[cod + 13] = kernel;

    const std::vector<ui8> m = atk_marker(kernel, steps);
    data.insert(data.begin() + (long)cod, m.begin(), m.end());
    return data;
  }

  ///////////////////////////////////////////////////////////////////////////
  // The 5/3 kernel of Part 1, written as an ATK segment
  ///////////////////////////////////////////////////////////////////////////
  std::vector<atk_step> rev53_steps()
  {
    std::vector<atk_step> steps(2);
    steps[0].Eatk = 2; steps[0].Batk = 2; steps[0].Aatk = 1;
    steps[1].Eatk = 1; steps[1].Batk = 1; steps[1].Aatk = -1;
    return steps;
  }

  ///////////////////////////////////////////////////////////////////////////
  // A kernel that is not 5/3, so that the lifting steps take the general
  // paths of the inverse wavelet, with the exponent of its first step set to
  // Eatk
  ///////////////////////////////////////////////////////////////////////////
  std::vector<atk_step> steps_with_exponent(ui8 Eatk)
  {
    std::vector<atk_step> steps = rev53_steps();
    steps[1].Eatk = Eatk;
    return steps;
  }

}

///////////////////////////////////////////////////////////////////////////////
// An exponent of 32 or more would shift a 32 bit sample by at least its own
// width, so the codestream has to be rejected
TEST(AtkTest, LiftingStepExponentOfAtLeast32IsRejected) {
  const ui32 width = 64, height = 32;
  mem_outfile out;
  encode(out, width, height, test_samples(width, height));

  const ui8 exponents[] = { 32, 63, 64, 255 };
  for (size_t i = 0; i < sizeof(exponents) / sizeof(exponents[0]); ++i)
  {
    std::vector<ui8> data =
      with_atk(out, 2, steps_with_exponent(exponents[i]));
    mem_infile in;
    in.open(&data[0], data.size());
    codestream cs;
    EXPECT_THROW(cs.read_headers(&in), std::runtime_error)
      << "Eatk of " << (int)exponents[i] << " was accepted";
  }
}

///////////////////////////////////////////////////////////////////////////////
// The largest exponent a 32 bit sample can be shifted by is still accepted
TEST(AtkTest, LiftingStepExponentOf31IsAccepted) {
  const ui32 width = 64, height = 32;
  mem_outfile out;
  encode(out, width, height, test_samples(width, height));

  std::vector<ui8> data = with_atk(out, 2, steps_with_exponent(31));
  mem_infile in;
  in.open(&data[0], data.size());
  codestream cs;
  ASSERT_NO_THROW(cs.read_headers(&in));
  cs.set_planar(false);
  cs.create();
  for (ui32 y = 0; y < height; ++y)
  {
    ui32 comp_num;
    ASSERT_NO_THROW(cs.pull(comp_num));
  }
  cs.close();
}

///////////////////////////////////////////////////////////////////////////////
// A codestream that selects the 5/3 kernel through an ATK marker segment
// decodes to the samples that were encoded with the 5/3 kernel of Part 1
TEST(AtkTest, Rev53AsAnAtkSegmentKeepsSamples) {
  const ui32 width = 64, height = 32;
  const std::vector<si32> samples = test_samples(width, height);
  mem_outfile out;
  encode(out, width, height, samples);

  std::vector<ui8> data = with_atk(out, 2, rev53_steps());
  mem_infile in;
  in.open(&data[0], data.size());
  codestream cs;
  ASSERT_NO_THROW(cs.read_headers(&in));
  cs.set_planar(false);
  cs.create();
  for (ui32 y = 0; y < height; ++y)
  {
    ui32 comp_num;
    line_buf* line = cs.pull(comp_num);
    for (ui32 x = 0; x < width; ++x)
      ASSERT_EQ(line->i32[x], samples[(size_t)y * width + x])
        << "at (" << x << ", " << y << ")";
  }
  cs.close();
}

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
