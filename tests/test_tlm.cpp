//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) Zwaar Contrast
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
// File: test_tlm.cpp
// Author: Zwaar Contrast
// Date: 08 September 2026
//***************************************************************************/

#include <cstdio>
#include <cstring>
#include <vector>

#include "ojph_mem.h"
#include "ojph_file.h"
#include "ojph_codestream.h"
#include "ojph_params.h"
#include "gtest/gtest.h"

///////////////////////////////////////////////////////////////////////////////
// TLM marker segment tests.
//
// A TLM segment can index at most (65535 - 4) / 6 = 10921 tile-parts, because
// Ltlm is 16 bits. Beyond that the entries continue in further segments with
// increasing Ztlm, which is 8 bits and so allows 256 of them. These tests
// check that the entries are split only when they have to be, that the split
// is well formed, and that the result still decodes.

namespace {

  const ojph::ui32 MAX_PAIRS_PER_SEG = (65535 - 4) / 6;

  struct tlm_seg { ojph::ui8 Ztlm; ojph::ui8 Stlm; ojph::ui32 count; };
  struct tlm_entry { ojph::ui32 Ttlm; ojph::ui32 Ptlm; };
  struct tile_part { size_t offset; ojph::ui32 Isot; ojph::ui32 length; };

  ///////////////////////////////////////////////////////////////////////////
  // Collect the TLM segments and their entries, and walk the SOT markers by
  // Psot, so what the index claims can be checked against what is really
  // there.
  void scan(const char* filename, std::vector<tlm_seg>& segs,
            std::vector<tlm_entry>& entries,
            std::vector<tile_part>& parts, size_t& header_end)
  {
    FILE* f = fopen(filename, "rb");
    ASSERT_NE(f, (FILE*)NULL);
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<ojph::ui8> b((size_t)len);
    size_t got = fread(b.data(), 1, (size_t)len, f);
    fclose(f);
    ASSERT_EQ(got, (size_t)len);

    const ojph::ui8* p = b.data();
    size_t pos = 2; // skip SOC
    header_end = 0;
    while (pos + 4 <= (size_t)len)
    {
      ojph::ui16 marker = (ojph::ui16)((p[pos] << 8) | p[pos + 1]);
      if (marker == 0xFF90) { header_end = pos; break; } // SOT
      ojph::ui16 seg_len = (ojph::ui16)((p[pos + 2] << 8) | p[pos + 3]);
      if (marker == 0xFF55) // TLM
      {
        tlm_seg s;
        s.Ztlm = p[pos + 4];
        s.Stlm = p[pos + 5];
        ojph::ui32 st = (s.Stlm >> 4) & 3;
        ojph::ui32 sp = (s.Stlm >> 6) & 1;
        ojph::ui32 entry_size = st + (sp ? 4u : 2u);
        s.count = (ojph::ui32)(seg_len - 4) / entry_size;
        segs.push_back(s);

        size_t q = pos + 6; // past marker, Ltlm, Ztlm, Stlm
        for (ojph::ui32 i = 0; i < s.count; ++i)
        {
          tlm_entry e;
          e.Ttlm = 0;
          for (ojph::ui32 k = 0; k < st; ++k)
            e.Ttlm = (e.Ttlm << 8) | p[q + k];
          q += st;
          e.Ptlm = 0;
          for (ojph::ui32 k = 0; k < (sp ? 4u : 2u); ++k)
            e.Ptlm = (e.Ptlm << 8) | p[q + k];
          q += sp ? 4 : 2;
          entries.push_back(e);
        }
      }
      pos += (size_t)seg_len + 2;
    }

    pos = header_end;
    while (pos + 12 <= (size_t)len &&
           ((p[pos] << 8) | p[pos + 1]) == 0xFF90)
    {
      ojph::ui32 Isot = (ojph::ui32)((p[pos + 4] << 8) | p[pos + 5]);
      ojph::ui32 Psot = ((ojph::ui32)p[pos + 6] << 24)
                      | ((ojph::ui32)p[pos + 7] << 16)
                      | ((ojph::ui32)p[pos + 8] << 8)
                      | (ojph::ui32)p[pos + 9];
      if (Psot == 0)
      { // the last tile-part may run to EOC
        size_t end = (size_t)len;
        if (end >= 2 && ((p[end - 2] << 8) | p[end - 1]) == 0xFFD9)
          end -= 2;
        tile_part t = { pos, Isot, (ojph::ui32)(end - pos) };
        parts.push_back(t);
        break;
      }
      tile_part t = { pos, Isot, Psot };
      parts.push_back(t);
      pos += Psot;
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  // The entries must describe the tile-parts that are really in the file: the
  // same count, in the same order, with lengths that add up to each
  // tile-part's offset. Counting entries alone would not catch a wrong Ptlm.
  void expect_entries_describe_tileparts(const std::vector<tlm_entry>& entries,
                                         const std::vector<tile_part>& parts,
                                         size_t header_end)
  {
    ASSERT_EQ(entries.size(), parts.size());
    size_t running = header_end;
    for (size_t i = 0; i < entries.size(); ++i)
    {
      ASSERT_EQ(entries[i].Ttlm, parts[i].Isot) << "entry " << i;
      ASSERT_EQ(entries[i].Ptlm, parts[i].length) << "entry " << i;
      ASSERT_EQ(running, parts[i].offset) << "entry " << i;
      running += entries[i].Ptlm;
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  // Encode a single-component image tiled at tile_size, with one tile-part
  // per resolution, and a TLM marker.
  void encode(const char* filename, ojph::ui32 width, ojph::ui32 height,
              ojph::ui32 tile_size, ojph::ui32 num_decomps)
  {
    ojph::codestream codestream;

    ojph::param_siz siz = codestream.access_siz();
    siz.set_image_extent(ojph::point(width, height));
    siz.set_num_components(1);
    siz.set_component(0, ojph::point(1, 1), 8, false);
    siz.set_image_offset(ojph::point(0, 0));
    siz.set_tile_size(ojph::size(tile_size, tile_size));
    siz.set_tile_offset(ojph::point(0, 0));

    ojph::param_cod cod = codestream.access_cod();
    cod.set_num_decomposition(num_decomps);
    cod.set_block_dims(64, 64);
    cod.set_color_transform(false);
    cod.set_reversible(true);
    cod.set_progression_order("RPCL");

    codestream.set_tilepart_divisions(true, false);
    codestream.request_tlm_marker(true);
    codestream.set_planar(false);

    ojph::j2c_outfile j2c_file;
    j2c_file.open(filename);
    codestream.write_headers(&j2c_file);

    ojph::ui32 next_comp;
    ojph::line_buf* cur_line = codestream.exchange(NULL, next_comp);
    for (ojph::ui32 y = 0; y < height; ++y)
    {
      for (ojph::ui32 x = 0; x < width; ++x)
        cur_line->i32[x] = (ojph::si32)((x + y) & 0xFF);
      cur_line = codestream.exchange(cur_line, next_comp);
    }
    codestream.flush();
    codestream.close();
  }

  ///////////////////////////////////////////////////////////////////////////
  void decode(const char* filename)
  {
    ojph::codestream codestream;
    ojph::j2c_infile j2c_file;
    j2c_file.open(filename);
    codestream.read_headers(&j2c_file);
    codestream.create();
    ojph::ui32 comp_num;
    ojph::param_siz siz = codestream.access_siz();
    ojph::ui32 height = (ojph::ui32)siz.get_image_extent().y;
    for (ojph::ui32 y = 0; y < height; ++y)
      codestream.pull(comp_num);
    codestream.close();
  }
}

///////////////////////////////////////////////////////////////////////////////
// Below the per-segment limit nothing is split: one TLM segment, Ztlm 0.
TEST(TestTLM, SingleSegmentBelowLimit) {
  const char* filename = "tlm_single_segment.j2c";
  // 32x32 tiles over 512x512 is 256 tiles, 2 tile-parts each = 512
  encode(filename, 512, 512, 32, 1);

  std::vector<tlm_seg> segs;
  std::vector<tlm_entry> entries;
  std::vector<tile_part> parts;
  size_t header_end = 0;
  ASSERT_NO_FATAL_FAILURE(scan(filename, segs, entries, parts, header_end));

  EXPECT_EQ(parts.size(), 512u);
  ASSERT_EQ(segs.size(), 1u);
  EXPECT_EQ(segs[0].Ztlm, 0);
  EXPECT_EQ(segs[0].count, (ojph::ui32)parts.size());
  ASSERT_NO_FATAL_FAILURE(
    expect_entries_describe_tileparts(entries, parts, header_end));

  decode(filename);
  remove(filename);
}

///////////////////////////////////////////////////////////////////////////////
// Past the per-segment limit the entries continue in further segments. Every
// segment but the last is full, Ztlm increases from zero, and the entries
// still describe the tile-parts that are really in the codestream.
TEST(TestTLM, MultipleSegmentsAboveLimit) {
  const char* filename = "tlm_multiple_segments.j2c";
  // 8x8 tiles over 512x512 is 4096 tiles, 3 tile-parts each = 12288, which
  // needs two segments
  encode(filename, 512, 512, 8, 2);

  std::vector<tlm_seg> segs;
  std::vector<tlm_entry> entries;
  std::vector<tile_part> parts;
  size_t header_end = 0;
  ASSERT_NO_FATAL_FAILURE(scan(filename, segs, entries, parts, header_end));

  EXPECT_EQ(parts.size(), 12288u);
  ASSERT_EQ(segs.size(), 2u);

  ojph::ui32 total = 0;
  for (size_t i = 0; i < segs.size(); ++i)
  {
    EXPECT_EQ(segs[i].Ztlm, (ojph::ui8)i);
    EXPECT_EQ(segs[i].Stlm, 0x60);
    if (i + 1 < segs.size())
      EXPECT_EQ(segs[i].count, MAX_PAIRS_PER_SEG);
    total += segs[i].count;
  }
  EXPECT_EQ(total, (ojph::ui32)parts.size());
  ASSERT_NO_FATAL_FAILURE(
    expect_entries_describe_tileparts(entries, parts, header_end));

  decode(filename);
  remove(filename);
}

///////////////////////////////////////////////////////////////////////////////
// The boundary itself. A codestream with exactly MAX_PAIRS_PER_SEG tile-parts
// is what unpatched OpenJPH accepts, and it must still be written as one
// segment.
TEST(TestTLM, ExactlyOneFullSegment) {
  const char* filename = "tlm_exactly_full.j2c";
  // 8x8 tiles over 536x1304 is 67x163 = 10921 tiles, one tile-part each
  encode(filename, 536, 1304, 8, 0);

  std::vector<tlm_seg> segs;
  std::vector<tlm_entry> entries;
  std::vector<tile_part> parts;
  size_t header_end = 0;
  ASSERT_NO_FATAL_FAILURE(scan(filename, segs, entries, parts, header_end));

  EXPECT_EQ(parts.size(), MAX_PAIRS_PER_SEG);
  ASSERT_EQ(segs.size(), 1u);
  EXPECT_EQ(segs[0].Ztlm, 0);
  EXPECT_EQ(segs[0].count, MAX_PAIRS_PER_SEG);
  ASSERT_NO_FATAL_FAILURE(
    expect_entries_describe_tileparts(entries, parts, header_end));

  decode(filename);
  remove(filename);
}

///////////////////////////////////////////////////////////////////////////////
// One tile-part past the boundary is the first codestream that needs a second
// segment, and that segment carries exactly one entry.
TEST(TestTLM, OneEntryPastFullSegment) {
  const char* filename = "tlm_one_past_full.j2c";
  // 8x8 tiles over 688x1016 is 86x127 = 10922 tiles, one tile-part each
  encode(filename, 688, 1016, 8, 0);

  std::vector<tlm_seg> segs;
  std::vector<tlm_entry> entries;
  std::vector<tile_part> parts;
  size_t header_end = 0;
  ASSERT_NO_FATAL_FAILURE(scan(filename, segs, entries, parts, header_end));

  EXPECT_EQ(parts.size(), MAX_PAIRS_PER_SEG + 1);
  ASSERT_EQ(segs.size(), 2u);
  EXPECT_EQ(segs[0].Ztlm, 0);
  EXPECT_EQ(segs[0].count, MAX_PAIRS_PER_SEG);
  EXPECT_EQ(segs[1].Ztlm, 1);
  EXPECT_EQ(segs[1].count, 1u);
  ASSERT_NO_FATAL_FAILURE(
    expect_entries_describe_tileparts(entries, parts, header_end));

  decode(filename);
  remove(filename);
}
