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
// File: test_plt.cpp
// Author: Zwaar Contrast
// Date: 30 September 2026
//***************************************************************************/

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <string>
#include <vector>

#include "ojph_mem.h"
#include "ojph_file.h"
#include "ojph_codestream.h"
#include "ojph_params.h"
#include "gtest/gtest.h"

///////////////////////////////////////////////////////////////////////////////
// PLT marker segment tests (T.800, A.7.3).
//
// The lengths are checked against the packets themselves: OpenJPH writes an
// empty packet as the byte 0x00, and a non-empty one starts with a 1 bit.
// The images mix empty and non-empty packets, so a wrong or misordered
// length lands on the wrong kind of byte.

namespace {

  struct plt_seg { ojph::ui32 Zplt; ojph::ui32 Lplt; };

  struct tile_part {
    ojph::ui32 Isot, TPsot, Psot;
    std::vector<plt_seg> segs;
    std::vector<ojph::ui32> lengths; // Iplt, concatenated in Zplt order
    std::vector<ojph::ui8> body;     // bytes after SOD
  };

  struct codestream_info {
    std::vector<ojph::ui32> Ptlm;
    std::vector<tile_part> parts;
  };

  ///////////////////////////////////////////////////////////////////////////
  std::vector<ojph::ui8> read_file(const char* filename)
  {
    std::vector<ojph::ui8> b;
    FILE* f = fopen(filename, "rb");
    if (f == NULL)
      return b;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    b.resize((size_t)len);
    size_t got = fread(b.data(), 1, (size_t)len, f);
    fclose(f);
    b.resize(got);
    return b;
  }

  ///////////////////////////////////////////////////////////////////////////
  ojph::ui32 be(const ojph::ui8* p, int n)
  {
    ojph::ui32 v = 0;
    for (int i = 0; i < n; ++i)
      v = (v << 8) | p[i];
    return v;
  }

  ///////////////////////////////////////////////////////////////////////////
  // Decodes Iplt values per Table A.36. Each segment must end on a completed
  // length, so segments are decoded independently of each other.
  void decode_iplt(const ojph::ui8* p, size_t n,
                   std::vector<ojph::ui32>& lengths)
  {
    ojph::ui32 v = 0;
    bool pending = false;
    for (size_t i = 0; i < n; ++i)
    {
      ASSERT_FALSE(pending && v > (0xFFFFFFFFu >> 7)) << "length overflow";
      v = (v << 7) | (p[i] & 0x7F);
      pending = (p[i] & 0x80) != 0;
      if (!pending) { lengths.push_back(v); v = 0; }
    }
    ASSERT_FALSE(pending) << "PLT segment ends inside a packet length";
  }

  ///////////////////////////////////////////////////////////////////////////
  void scan(const char* filename, codestream_info& info)
  {
    std::vector<ojph::ui8> b = read_file(filename);
    ASSERT_GT(b.size(), 4u);
    const ojph::ui8* p = b.data();
    size_t len = b.size();

    size_t pos = 2; // skip SOC
    while (pos + 4 <= len && be(p + pos, 2) != 0xFF90)
    {
      ojph::ui32 seg_len = be(p + pos + 2, 2);
      if (be(p + pos, 2) == 0xFF55) // TLM, Stlm 0x60 as written by OpenJPH
        for (size_t q = pos + 6; q + 6 <= pos + 2 + seg_len; q += 6)
          info.Ptlm.push_back(be(p + q + 2, 4));
      pos += seg_len + 2;
    }

    while (pos + 12 <= len && be(p + pos, 2) == 0xFF90)
    {
      tile_part t;
      t.Isot = be(p + pos + 4, 2);
      t.Psot = be(p + pos + 6, 4);
      t.TPsot = p[pos + 10];
      ASSERT_NE(t.Psot, 0u);
      ASSERT_LE(pos + t.Psot, len);

      size_t q = pos + 12;
      while (be(p + q, 2) != 0xFF93) // SOD
      {
        ASSERT_LT(q + 4, pos + t.Psot);
        ojph::ui32 seg_len = be(p + q + 2, 2);
        if (be(p + q, 2) == 0xFF58)
        {
          plt_seg s = { p[q + 4], seg_len };
          ASSERT_GE(s.Lplt, 4u);
          t.segs.push_back(s);
          ASSERT_NO_FATAL_FAILURE(
            decode_iplt(p + q + 5, seg_len - 3, t.lengths));
        }
        q += seg_len + 2;
      }
      q += 2;
      t.body.assign(p + q, p + pos + t.Psot);
      info.parts.push_back(t);
      pos += t.Psot;
    }
    ASSERT_EQ(be(p + pos, 2), 0xFFD9u); // EOC
  }

  ///////////////////////////////////////////////////////////////////////////
  // The lengths must tile the tile-part body exactly, each packet starting
  // with 0x00 when it is empty, and with a 1 bit otherwise. Returns how many
  // packets of each kind were seen, so callers can check both occurred.
  void expect_lengths_match_packets(const tile_part& t,
                                    ojph::ui32& num_empty,
                                    ojph::ui32& num_coded)
  {
    size_t off = 0;
    for (size_t i = 0; i < t.lengths.size(); ++i)
    {
      ASSERT_LT(off, t.body.size()) << "packet " << i;
      ojph::ui8 first = t.body[off];
      if (t.lengths[i] == 1) {
        ASSERT_EQ(first, 0x00) << "packet " << i;
        ++num_empty;
      }
      else {
        ASSERT_NE(first & 0x80, 0) << "packet " << i;
        ++num_coded;
      }
      off += t.lengths[i];
    }
    ASSERT_EQ(off, t.body.size());
  }

  ///////////////////////////////////////////////////////////////////////////
  ojph::ui32 iplt_bytes(ojph::ui32 len)
  {
    ojph::ui32 n = 1;
    while (len >>= 7)
      ++n;
    return n;
  }

  ///////////////////////////////////////////////////////////////////////////
  // Zplt restarts at 0 in every tile-part header (A.7.3), and every
  // segment but the last is filled until the next length would not fit.
  void expect_segments_well_formed(const tile_part& t)
  {
    ASSERT_FALSE(t.segs.empty());
    size_t next = 0;
    for (size_t k = 0; k < t.segs.size(); ++k)
    {
      ASSERT_EQ(t.segs[k].Zplt, (ojph::ui32)k);
      ojph::ui32 seg_bytes = t.segs[k].Lplt - 3, used = 0;
      while (used < seg_bytes)
        used += iplt_bytes(t.lengths[next++]);
      ASSERT_EQ(used, seg_bytes);
      if (k + 1 < t.segs.size())
        ASSERT_GT(seg_bytes + iplt_bytes(t.lengths[next]), 65535u - 3)
          << "segment " << k;
    }
    ASSERT_EQ(next, t.lengths.size());
  }

  ///////////////////////////////////////////////////////////////////////////
  // Every tile-part: well formed segments, lengths that describe its packets,
  // and a Ptlm that matches Psot.
  void expect_valid_plt(const codestream_info& info, ojph::ui32& num_empty,
                        ojph::ui32& num_coded)
  {
    ASSERT_EQ(info.Ptlm.size(), info.parts.size());
    for (size_t i = 0; i < info.parts.size(); ++i)
    {
      SCOPED_TRACE("tile-part " + std::to_string(i));
      const tile_part& t = info.parts[i];
      ASSERT_NO_FATAL_FAILURE(expect_segments_well_formed(t));
      ASSERT_NO_FATAL_FAILURE(
        expect_lengths_match_packets(t, num_empty, num_coded));
      EXPECT_EQ(info.Ptlm[i], t.Psot);
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  struct encode_params {
    const char* prog_order = "RPCL";
    bool at_res = false, at_comp = false; // tile-part divisions
    ojph::ui32 width = 256, height = 256; // excluding the image offset
    ojph::ui32 offset = 0;                // image offset, x and y
    ojph::ui32 tile_size = 128, num_decomps = 2, precinct = 32;
    bool subsampled = false; // components 1 and 2 at half size, as 4:2:0
    bool coc = false;        // component 1 has one decomposition less
    bool plt = true;
    bool all_zero = false;   // every packet empty
  };

  ojph::ui32 div_ceil(ojph::ui32 a, ojph::ui32 b) { return (a + b - 1) / b; }

  ojph::ui32 comp_decomps(const encode_params& e, ojph::ui32 c)
  { return e.coc && c == 1 ? e.num_decomps - 1 : e.num_decomps; }

  ojph::ui32 comp_down(const encode_params& e, ojph::ui32 c)
  { return e.subsampled && c != 0 ? 2 : 1; }

  ///////////////////////////////////////////////////////////////////////////
  // Precincts of component c at resolution r of the tile spanning x0..x1,
  // y0..y1 on the reference grid, per T.800 equations B-12, B-14 and B-16.
  ojph::ui32 num_precincts(const encode_params& e, ojph::ui32 c,
                           ojph::ui32 r, ojph::ui32 x0, ojph::ui32 x1,
                           ojph::ui32 y0, ojph::ui32 y1)
  {
    ojph::ui32 d = comp_down(e, c);
    ojph::ui32 scale = d << (comp_decomps(e, c) - r);
    ojph::ui32 rx0 = div_ceil(x0, scale), rx1 = div_ceil(x1, scale);
    ojph::ui32 ry0 = div_ceil(y0, scale), ry1 = div_ceil(y1, scale);
    if (rx1 == rx0 || ry1 == ry0)
      return 0;
    ojph::ui32 p = e.precinct;
    return (div_ceil(rx1, p) - rx0 / p) * (div_ceil(ry1, p) - ry0 / p);
  }

  ///////////////////////////////////////////////////////////////////////////
  // Packets in each tile-part of the codestream, tiles in raster order
  // (B-7, B-8), and tile-parts grouped as OpenJPH divides them.
  std::vector<ojph::ui32> expected_packets(const encode_params& e)
  {
    std::vector<ojph::ui32> parts;
    ojph::ui32 x_end = e.offset + e.width, y_end = e.offset + e.height;
    for (ojph::ui32 ty = 0; ty * e.tile_size < y_end; ++ty)
      for (ojph::ui32 tx = 0; tx * e.tile_size < x_end; ++tx)
      {
        ojph::ui32 x0 = std::max(tx * e.tile_size, e.offset);
        ojph::ui32 x1 = std::min((tx + 1) * e.tile_size, x_end);
        ojph::ui32 y0 = std::max(ty * e.tile_size, e.offset);
        ojph::ui32 y1 = std::min((ty + 1) * e.tile_size, y_end);
        std::vector<ojph::ui32> n(3 * (e.num_decomps + 1), 0); // [r][c]
        for (ojph::ui32 c = 0; c < 3; ++c)
          for (ojph::ui32 r = 0; r <= comp_decomps(e, c); ++r)
            n[r * 3 + c] = num_precincts(e, c, r, x0, x1, y0, y1);

        bool cprl = strcmp(e.prog_order, "CPRL") == 0;
        if (!e.at_res && !e.at_comp)
          parts.push_back(std::accumulate(n.begin(), n.end(), 0u));
        else if (cprl) // one tile-part per component
          for (ojph::ui32 c = 0; c < 3; ++c)
          {
            ojph::ui32 sum = 0;
            for (ojph::ui32 r = 0; r <= e.num_decomps; ++r)
              sum += n[r * 3 + c];
            parts.push_back(sum);
          }
        else if (!e.at_comp) // one tile-part per resolution
          for (ojph::ui32 r = 0; r <= e.num_decomps; ++r)
            parts.push_back(n[r * 3] + n[r * 3 + 1] + n[r * 3 + 2]);
        else // one per resolution and component, if it has that resolution
          for (ojph::ui32 r = 0; r <= e.num_decomps; ++r)
            for (ojph::ui32 c = 0; c < 3; ++c)
              if (r <= comp_decomps(e, c))
                parts.push_back(n[r * 3 + c]);
      }
    return parts;
  }

  ///////////////////////////////////////////////////////////////////////////
  // Component 0 is all zero after the level shift, giving empty packets;
  // 1 and 2 are noise, giving non-empty packets at every resolution.
  ojph::si32 sample(ojph::ui32 c, ojph::ui32 x, ojph::ui32 y)
  {
    if (c == 0)
      return 128;
    ojph::ui32 h = (x * 73856093u) ^ (y * 19349663u) ^ (c * 83492791u);
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return (ojph::si32)(h & 0xFF);
  }

  ///////////////////////////////////////////////////////////////////////////
  void encode(const char* filename, const encode_params& e)
  {
    ojph::codestream codestream;

    ojph::param_siz siz = codestream.access_siz();
    siz.set_image_extent(ojph::point(e.offset + e.width,
                                     e.offset + e.height));
    siz.set_num_components(3);
    for (ojph::ui32 c = 0; c < 3; ++c)
      siz.set_component(c, ojph::point(comp_down(e, c), comp_down(e, c)),
                        8, false);
    siz.set_image_offset(ojph::point(e.offset, e.offset));
    siz.set_tile_size(ojph::size(e.tile_size, e.tile_size));
    siz.set_tile_offset(ojph::point(0, 0));

    ojph::param_cod cod = codestream.access_cod();
    cod.set_num_decomposition(e.num_decomps);
    cod.set_block_dims(64, 64);
    ojph::size precinct(e.precinct, e.precinct);
    cod.set_precinct_size(1, &precinct);
    if (e.coc) // a COC starts from defaults, not from the COD
    {
      cod.set_num_decomposition(1, e.num_decomps - 1);
      cod.set_precinct_size(1, 1, &precinct);
      cod.set_block_dims(1, 64, 64);
      cod.set_reversible(1, true);
    }
    cod.set_color_transform(false);
    cod.set_reversible(true);
    cod.set_progression_order(e.prog_order);

    codestream.set_tilepart_divisions(e.at_res, e.at_comp);
    codestream.request_tlm_marker(true);
    codestream.request_plt_marker(e.plt);
    codestream.set_planar(e.subsampled); // required for subsampling

    ojph::j2c_outfile j2c_file;
    j2c_file.open(filename);
    codestream.write_headers(&j2c_file);

    ojph::ui32 total_lines = 0, cur_row[3] = { 0, 0, 0 };
    for (ojph::ui32 c = 0; c < 3; ++c)
      total_lines += siz.get_recon_height(c);
    ojph::ui32 c;
    ojph::line_buf* line = codestream.exchange(NULL, c);
    for (ojph::ui32 i = 0; i < total_lines; ++i)
    {
      ojph::ui32 y = cur_row[c]++;
      for (ojph::ui32 x = 0; x < siz.get_recon_width(c); ++x)
        line->i32[x] = e.all_zero ? 128 : sample(c, x, y);
      line = codestream.exchange(line, c);
    }
    codestream.flush();
    codestream.close();
  }

  ///////////////////////////////////////////////////////////////////////////
  // One plane of samples per component, rows in order.
  typedef std::vector<std::vector<ojph::si32> > planes;

  ///////////////////////////////////////////////////////////////////////////
  planes decode(const char* filename)
  {
    planes samples(3);
    ojph::codestream codestream;
    ojph::j2c_infile j2c_file;
    j2c_file.open(filename);
    codestream.read_headers(&j2c_file);
    codestream.create();
    ojph::param_siz siz = codestream.access_siz();
    ojph::ui32 total_lines = 0;
    for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
      total_lines += siz.get_recon_height(c);
    for (ojph::ui32 i = 0; i < total_lines; ++i)
    {
      ojph::ui32 c;
      ojph::line_buf* line = codestream.pull(c);
      samples[c].insert(samples[c].end(), line->i32,
                        line->i32 + siz.get_recon_width(c));
    }
    codestream.close();
    return samples;
  }

  ///////////////////////////////////////////////////////////////////////////
  // What encode() pushed, as decode() returns it.
  planes source(const encode_params& e)
  {
    planes samples(3);
    for (ojph::ui32 c = 0; c < 3; ++c)
    {
      ojph::ui32 d = comp_down(e, c);
      ojph::ui32 w = div_ceil(e.offset + e.width, d) - div_ceil(e.offset, d);
      ojph::ui32 h = div_ceil(e.offset + e.height, d) - div_ceil(e.offset, d);
      for (ojph::ui32 y = 0; y < h; ++y)
        for (ojph::ui32 x = 0; x < w; ++x)
          samples[c].push_back(e.all_zero ? 128 : sample(c, x, y));
    }
    return samples;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Every progression order and tile-part division, on geometries where the
// components' precincts fall at different places: the lengths describe the
// packets, the packets match an encode without PLT (which has none), and
// both decode losslessly.
enum geometry { UNIFORM, COC, YUV420, UNEVEN };

struct plt_case {
  const char* prog_order;
  bool at_res, at_comp;
  geometry geom;
};

class TestPLTOrders : public ::testing::TestWithParam<plt_case> {};

TEST_P(TestPLTOrders, LengthsDescribePackets) {
  const plt_case& pc = GetParam();
  const char* geom_names[] = { "uniform", "coc", "yuv420", "uneven" };
  std::string name = std::string("plt_") + pc.prog_order
    + (pc.at_res ? "_R" : "") + (pc.at_comp ? "_C" : "") + "_"
    + geom_names[pc.geom];
  std::string with = name + ".j2c", without = name + "_ref.j2c";

  encode_params e;
  e.prog_order = pc.prog_order;
  e.at_res = pc.at_res;
  e.at_comp = pc.at_comp;
  e.coc = pc.geom == COC;
  e.subsampled = pc.geom == YUV420 || pc.geom == UNEVEN;
  if (pc.geom == UNEVEN) // 3x2 tiles, cut short on the right and bottom
  {
    e.offset = 5;
    e.width = 261;
    e.height = 195;
    e.tile_size = 100;
  }
  encode(with.c_str(), e);
  e.plt = false;
  encode(without.c_str(), e);

  codestream_info info, ref;
  ASSERT_NO_FATAL_FAILURE(scan(with.c_str(), info));
  ASSERT_NO_FATAL_FAILURE(scan(without.c_str(), ref));

  std::vector<ojph::ui32> packets = expected_packets(e);
  ASSERT_EQ(info.parts.size(), packets.size());
  ASSERT_EQ(info.parts.size(), ref.parts.size());

  ojph::ui32 num_empty = 0, num_coded = 0;
  ASSERT_NO_FATAL_FAILURE(expect_valid_plt(info, num_empty, num_coded));
  for (size_t i = 0; i < info.parts.size(); ++i)
  {
    SCOPED_TRACE("tile-part " + std::to_string(i));
    const tile_part& t = info.parts[i];
    EXPECT_EQ(t.lengths.size(), packets[i]);
    EXPECT_TRUE(ref.parts[i].segs.empty());
    EXPECT_EQ(t.body, ref.parts[i].body);
    EXPECT_EQ(t.Isot, ref.parts[i].Isot);
    EXPECT_EQ(t.TPsot, ref.parts[i].TPsot);
  }
  EXPECT_GT(num_empty, 0u);
  EXPECT_GT(num_coded, 0u);

  EXPECT_TRUE(decode(with.c_str()) == source(e));
  EXPECT_TRUE(decode(without.c_str()) == source(e));
  remove(with.c_str());
  remove(without.c_str());
}

std::vector<plt_case> all_cases()
{
  const plt_case orders[] = {
    { "LRCP", false, false, UNIFORM }, { "LRCP", true, false, UNIFORM },
    { "LRCP", true, true, UNIFORM },   { "RLCP", false, false, UNIFORM },
    { "RLCP", true, false, UNIFORM },  { "RLCP", true, true, UNIFORM },
    { "RPCL", false, false, UNIFORM }, { "RPCL", true, false, UNIFORM },
    { "PCRL", false, false, UNIFORM }, { "CPRL", false, false, UNIFORM },
    { "CPRL", false, true, UNIFORM } };
  std::vector<plt_case> cases;
  for (int g = UNIFORM; g <= UNEVEN; ++g)
    for (plt_case c : orders)
    {
      c.geom = (geometry)g;
      // OpenJPH numbers these tile-parts with gaps when components have
      // different numbers of decompositions, and cannot read them back
      if (c.geom == COC && c.at_comp && c.prog_order[1] != 'P')
        continue;
      cases.push_back(c);
    }
  return cases;
}

INSTANTIATE_TEST_SUITE_P(AllOrders, TestPLTOrders,
                         ::testing::ValuesIn(all_cases()));

///////////////////////////////////////////////////////////////////////////////
// PLT is off unless requested, and the request can be read back.
TEST(TestPLT, RequestIsReported) {
  ojph::codestream codestream;
  EXPECT_FALSE(codestream.is_plt_requested());
  codestream.request_plt_marker(true);
  EXPECT_TRUE(codestream.is_plt_requested());
  codestream.request_plt_marker(false);
  EXPECT_FALSE(codestream.is_plt_requested());
}

///////////////////////////////////////////////////////////////////////////////
// The expected counts from the spec's equations match counts worked out by
// hand: 1, 4 and 16 precincts per component at resolutions 0, 1 and 2.
TEST(TestPLT, ExpectedPacketsMatchHandCounts) {
  encode_params e;
  EXPECT_EQ(expected_packets(e), std::vector<ojph::ui32>(4, 63));
  e.at_res = true;
  std::vector<ojph::ui32> r = expected_packets(e);
  ASSERT_EQ(r.size(), 12u);
  EXPECT_EQ(std::vector<ojph::ui32>(r.begin(), r.begin() + 3),
            (std::vector<ojph::ui32>{ 3, 12, 48 }));
  e.at_comp = true;
  r = expected_packets(e);
  ASSERT_EQ(r.size(), 36u);
  EXPECT_EQ(std::vector<ojph::ui32>(r.begin(), r.begin() + 9),
            (std::vector<ojph::ui32>{ 1, 1, 1, 4, 4, 4, 16, 16, 16 }));
  e.prog_order = "CPRL";
  e.at_res = false;
  r = expected_packets(e);
  ASSERT_EQ(r.size(), 12u);
  EXPECT_EQ(std::vector<ojph::ui32>(r.begin(), r.begin() + 3),
            (std::vector<ojph::ui32>{ 21, 21, 21 }));
}

///////////////////////////////////////////////////////////////////////////////
// Lengths of 128 bytes and more take several Iplt bytes, most significant
// group first (Table A.36). One precinct per resolution makes the noise
// packets long enough to need three bytes.
TEST(TestPLT, MultiByteLengths) {
  const char* filename = "plt_multibyte.j2c";
  encode_params e;
  e.tile_size = 256;
  e.num_decomps = 1;
  e.precinct = 32768;
  encode(filename, e);

  codestream_info info;
  ASSERT_NO_FATAL_FAILURE(scan(filename, info));
  ASSERT_EQ(info.parts.size(), 1u);
  const tile_part& t = info.parts[0];
  ASSERT_EQ(t.lengths.size(), 6u);

  ojph::ui32 num_empty = 0, num_coded = 0;
  ASSERT_NO_FATAL_FAILURE(expect_valid_plt(info, num_empty, num_coded));
  EXPECT_GE(*std::max_element(t.lengths.begin(), t.lengths.end()),
            1u << 14);
  EXPECT_EQ(num_empty, 2u);
  EXPECT_EQ(num_coded, 4u);
  remove(filename);
}

///////////////////////////////////////////////////////////////////////////////
// More than 65532 bytes of Iplt need several segments. Lengths of 1 + 2 + 2
// bytes per position do not divide 65532, so the first segment must stop
// short of full rather than break a length in two.
TEST(TestPLT, SplitsAcrossSegments) {
  const char* filename = "plt_split.j2c";
  // 2048x2048, 1 decomposition, 16x16 precincts: 16384 + 4096 precincts,
  // 3 packets each
  encode_params e;
  e.at_res = true;
  e.width = e.height = e.tile_size = 2048;
  e.num_decomps = 1;
  e.precinct = 16;
  encode(filename, e);

  codestream_info info;
  ASSERT_NO_FATAL_FAILURE(scan(filename, info));
  ASSERT_EQ(info.parts.size(), 2u);
  ojph::ui32 num_empty = 0, num_coded = 0;
  ASSERT_NO_FATAL_FAILURE(expect_valid_plt(info, num_empty, num_coded));
  EXPECT_EQ(info.parts[0].lengths.size(), 3u * 4096u);
  EXPECT_EQ(info.parts[1].lengths.size(), 3u * 16384u);
  ASSERT_GE(info.parts[1].segs.size(), 2u);

  // the 1 + 2 + 2 bytes the check below relies on
  for (size_t i = 0; i < info.parts[1].lengths.size(); ++i)
  {
    ojph::ui32 len = info.parts[1].lengths[i];
    ASSERT_TRUE(i % 3 == 0 ? len == 1 : len >= 128 && len < 16384)
      << "packet " << i << " has length " << len;
  }
  EXPECT_LT(info.parts[1].segs[0].Lplt, 65535u);
  remove(filename);
}

///////////////////////////////////////////////////////////////////////////////
// When every length takes one byte, the first segment is filled exactly, to
// the largest Lplt allowed, 65535.
TEST(TestPLT, FullSegment) {
  const char* filename = "plt_full_segment.j2c";
  // 4096x2048, 1 decomposition, 16x16 precincts: 32768 + 8192 precincts,
  // 3 empty packets each
  encode_params e;
  e.width = e.tile_size = 4096;
  e.height = 2048;
  e.num_decomps = 1;
  e.precinct = 16;
  e.all_zero = true;
  encode(filename, e);

  codestream_info info;
  ASSERT_NO_FATAL_FAILURE(scan(filename, info));
  ASSERT_EQ(info.parts.size(), 1u);
  const tile_part& t = info.parts[0];
  ojph::ui32 num_empty = 0, num_coded = 0;
  ASSERT_NO_FATAL_FAILURE(expect_valid_plt(info, num_empty, num_coded));
  EXPECT_EQ(num_empty, 3u * 40960u);
  EXPECT_EQ(num_coded, 0u);
  ASSERT_EQ(t.segs.size(), 2u);
  EXPECT_EQ(t.segs[0].Lplt, 65535u);
  EXPECT_EQ(t.segs[1].Lplt, 3u * 40960u - 65532u + 3u);
  remove(filename);
}
