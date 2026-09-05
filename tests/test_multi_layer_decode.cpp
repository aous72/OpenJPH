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
// File: test_multi_layer_decode.cpp
// Author: Aous Naman
// Date: 05 September 2026
//***************************************************************************/
//
// These tests decode codestreams that carry more than one quality layer.
//
// The encoder writes one quality layer only, so the multi-layer codestreams
// have to be built here.  They are built by re-layering a codestream the
// encoder produced, using a transformation that adds layers without moving a
// single byte of coded data:
//
//   A packet whose header begins with a 0 bit is an empty packet (T.800
//   B.10.3).  It is one byte long, it includes no code-block, and there is
//   nothing else in it.  So a one-layer codestream can be turned into an
//   N-layer codestream by writing N into the number-of-layers field of the
//   COD marker segment and inserting an empty packet everywhere the extra
//   layers call for one.  All coded data stays in layer 0 and the added
//   layers are empty, so the image the codestream represents is unchanged
//   and the decoded samples must come out bit-identical.
//
// The added layers are also built in a second form, in which their packets
// carry a header that includes none of the code-blocks rather than being
// empty; see make_excluding_packet().  That one makes the decoder walk a
// packet header in a layer other than the first, which the empty packet does
// not.  Both forms contribute nothing, so both must decode identically.
//
// Where the empty packets go depends on the progression order, and that is
// the part worth testing.  In LRCP the layer is the outermost loop, so the
// added layers are one block of empty packets after all the real ones.  In
// RLCP the layer sits inside the resolution loop, so a block of empty
// packets follows each resolution.  In RPCL, PCRL and CPRL the layer is the
// innermost loop, so a single empty packet follows every real packet.
//
// Building the new tile requires knowing where each packet of the original
// ends.  Rather than parse packet headers, the packets are collected from a
// second encoding of the same image that asks for tile-part divisions at
// resolution and component level: with one precinct per resolution that puts
// exactly one packet in each tile part, and tile-part boundaries are given by
// the Psot fields.  The collected packets are then re-assembled in the order
// each progression calls for.  Every test first re-assembles the one-layer
// case and requires it to be byte-identical to what the encoder wrote, which
// checks both the collected packets and the packet ordering used here.
//
// Everything is done in memory, so the tests need no external files.

#include <algorithm>
#include <cstddef>
#include <cstring>
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
// The image is small, but it has enough resolutions and components that the
// five progression orders do not all produce the same packet sequence, and
// enough detail that every code-block carries data.
static const ojph::ui32 IMAGE_WIDTH        = 128;
static const ojph::ui32 IMAGE_HEIGHT       = 128;
static const ojph::ui32 NUM_COMPONENTS     = 3;
static const ojph::ui32 NUM_DECOMPOSITIONS = 3;
static const ojph::ui32 NUM_RESOLUTIONS    = NUM_DECOMPOSITIONS + 1;
static const ojph::ui32 BLOCK_DIM          = 32;
static const ojph::ui32 PACKETS_PER_LAYER  =
  NUM_RESOLUTIONS * NUM_COMPONENTS;

// Precincts are left at their default, which is the largest the standard
// allows, so every resolution of every component holds exactly one precinct.
// The re-layering below relies on that: it lets a packet be named by its
// resolution and component alone, and it makes the number of packets in a
// layer simply the number of resolutions times the number of components.

// The empty packet: a single byte whose first bit, the zero length bit of
// the packet header, is 0 (T.800 B.10.3).
static const ojph::ui8 EMPTY_PACKET = 0x00;

// How the packets of the added layers are written.
enum added_layer_kind
{
  // one empty packet, which has no header past its leading 0 bit
  EMPTY_PACKET_LAYER,
  // a packet that has a header, but whose header includes no code-block
  EXCLUDING_PACKET_LAYER
};

static const char* const PROGRESSION_ORDERS[] =
  { "LRCP", "RLCP", "RPCL", "PCRL", "CPRL" };
static const ojph::ui32 NUM_PROGRESSION_ORDERS = 5;

// marker codes used while walking the codestream
static const ojph::ui32 MARKER_COD = 0xFF52;
static const ojph::ui32 MARKER_SOT = 0xFF90;
static const ojph::ui32 MARKER_SOD = 0xFF93;
static const ojph::ui32 MARKER_EOC = 0xFFD9;

////////////////////////////////////////////////////////////////////////////////
//                             byte level helpers
////////////////////////////////////////////////////////////////////////////////
static ojph::ui32 get_u16(const std::vector<ojph::ui8>& buf, size_t pos)
{
  return ((ojph::ui32)buf[pos] << 8) | (ojph::ui32)buf[pos + 1];
}

////////////////////////////////////////////////////////////////////////////////
static ojph::ui32 get_u32(const std::vector<ojph::ui8>& buf, size_t pos)
{
  return ((ojph::ui32)buf[pos    ] << 24) | ((ojph::ui32)buf[pos + 1] << 16)
       | ((ojph::ui32)buf[pos + 2] <<  8) | ((ojph::ui32)buf[pos + 3]      );
}

////////////////////////////////////////////////////////////////////////////////
static void put_u16(std::vector<ojph::ui8>& buf, size_t pos, ojph::ui32 val)
{
  buf[pos    ] = (ojph::ui8)((val >> 8) & 0xFF);
  buf[pos + 1] = (ojph::ui8)( val       & 0xFF);
}

////////////////////////////////////////////////////////////////////////////////
static void put_u32(std::vector<ojph::ui8>& buf, size_t pos, ojph::ui32 val)
{
  buf[pos    ] = (ojph::ui8)((val >> 24) & 0xFF);
  buf[pos + 1] = (ojph::ui8)((val >> 16) & 0xFF);
  buf[pos + 2] = (ojph::ui8)((val >>  8) & 0xFF);
  buf[pos + 3] = (ojph::ui8)( val        & 0xFF);
}

////////////////////////////////////////////////////////////////////////////////
//                              find_cod_segment
////////////////////////////////////////////////////////////////////////////////
// Returns the offset of the COD marker in the main header.  Every marker
// segment of the main header carries a two byte length right after the
// marker, so the header can be walked without knowing any of the segments.
static size_t find_cod_segment(const std::vector<ojph::ui8>& buf)
{
  size_t pos = 2;                             // just past SOC
  while (get_u16(buf, pos) != MARKER_SOT)
  {
    if (get_u16(buf, pos) == MARKER_COD)
      return pos;
    pos += 2 + get_u16(buf, pos + 2);
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//                                  tile_part
////////////////////////////////////////////////////////////////////////////////
// One tile part of the codestream: where its SOT marker is, and the range of
// bytes holding its packets.
struct tile_part
{
  size_t sot_pos;                             // offset of the SOT marker
  size_t body_pos;                            // first byte after SOD
  size_t body_end;                            // one past the last packet byte
};

////////////////////////////////////////////////////////////////////////////////
//                                     off
////////////////////////////////////////////////////////////////////////////////
// The offsets above are held as size_t, while an iterator advances by a signed
// difference_type.  Make that conversion explicit here rather than repeat it at
// every call site.
static std::ptrdiff_t off(size_t offset)
{
  return static_cast<std::ptrdiff_t>(offset);
}

////////////////////////////////////////////////////////////////////////////////
//                               scan_tile_parts
////////////////////////////////////////////////////////////////////////////////
// Lists the tile parts of a single tile codestream.  A tile part runs from
// its SOT marker for Psot bytes; its packets start right after the SOD marker
// that ends the tile-part header.
static std::vector<tile_part> scan_tile_parts(const std::vector<ojph::ui8>& buf)
{
  std::vector<tile_part> parts;
  size_t pos = 2;                             // just past SOC
  while (get_u16(buf, pos) != MARKER_SOT)
    pos += 2 + get_u16(buf, pos + 2);

  while (pos < buf.size() && get_u16(buf, pos) == MARKER_SOT)
  {
    tile_part part;
    part.sot_pos = pos;
    part.body_end = pos + get_u32(buf, pos + 6);   // Psot

    size_t hdr = pos + 2 + get_u16(buf, pos + 2);  // past the SOT segment
    while (get_u16(buf, hdr) != MARKER_SOD)
      hdr += 2 + get_u16(buf, hdr + 2);
    part.body_pos = hdr + 2;

    parts.push_back(part);
    pos = part.body_end;
  }
  return parts;
}

////////////////////////////////////////////////////////////////////////////////
//                           encode_test_codestream
////////////////////////////////////////////////////////////////////////////////
// Encodes a fixed synthetic image losslessly with the requested progression
// order, and returns the codestream.  When tile_parts is true the tile is
// split into one tile part per resolution and component.
static std::vector<ojph::ui8> encode_test_codestream(const char* prog_order,
                                                     bool tile_parts)
{
  ojph::codestream cs;

  ojph::param_siz siz = cs.access_siz();
  siz.set_image_extent(ojph::point(IMAGE_WIDTH, IMAGE_HEIGHT));
  siz.set_num_components(NUM_COMPONENTS);
  for (ojph::ui32 c = 0; c < NUM_COMPONENTS; ++c)
    siz.set_component(c, ojph::point(1, 1), 8, false);
  siz.set_image_offset(ojph::point(0, 0));
  siz.set_tile_size(ojph::size(IMAGE_WIDTH, IMAGE_HEIGHT));
  siz.set_tile_offset(ojph::point(0, 0));

  ojph::param_cod cod = cs.access_cod();
  cod.set_num_decomposition(NUM_DECOMPOSITIONS);
  cod.set_block_dims(BLOCK_DIM, BLOCK_DIM);
  cod.set_color_transform(false);
  cod.set_reversible(true);
  cod.set_progression_order(prog_order);

  if (tile_parts)
    cs.set_tilepart_divisions(true, true);
  cs.set_planar(true);

  ojph::mem_outfile out;
  out.open();
  cs.write_headers(&out);

  ojph::ui32 next_comp = 0;
  ojph::line_buf* line = cs.exchange(NULL, next_comp);
  for (ojph::ui32 c = 0; c < NUM_COMPONENTS; ++c)
    for (ojph::ui32 y = 0; y < IMAGE_HEIGHT; ++y)
    {
      ojph::si32* dp = line->i32;
      for (ojph::ui32 x = 0; x < IMAGE_WIDTH; ++x)
        dp[x] = (ojph::si32)
          ((x * 7 + y * 13 + c * 29 + ((x * y) >> 3)) & 0xFF);
      line = cs.exchange(line, next_comp);
    }
  cs.flush();

  std::vector<ojph::ui8> buf(out.get_data(),
                             out.get_data() + (size_t)out.tell());
  cs.close();
  return buf;
}

////////////////////////////////////////////////////////////////////////////////
//                              collect_packets
////////////////////////////////////////////////////////////////////////////////
// Returns the coded bytes of each packet of the test image, indexed by
// res * NUM_COMPONENTS + comp.
//
// The packets are read out of an LRCP encoding that was asked for tile-part
// divisions at both resolution and component level.  For LRCP that gives one
// tile part per resolution and component, in resolution major order (see
// tile::flush()), and because every resolution holds a single precinct each
// of those tile parts holds exactly one packet.  A packet's bytes depend only
// on the code-blocks of its own precinct, never on the order the packets are
// written in, so the same bytes serve every progression order; each test
// checks that by re-assembling the one-layer codestream and comparing it with
// what the encoder wrote.
static std::vector<std::vector<ojph::ui8> > collect_packets()
{
  std::vector<ojph::ui8> buf = encode_test_codestream("LRCP", true);
  std::vector<tile_part> parts = scan_tile_parts(buf);

  std::vector<std::vector<ojph::ui8> > packets;
  if (parts.size() != PACKETS_PER_LAYER)
    return packets;                           // reported by the caller

  for (size_t i = 0; i < parts.size(); ++i)
    packets.push_back(
      std::vector<ojph::ui8>(buf.begin() + off(parts[i].body_pos),
                             buf.begin() + off(parts[i].body_end)));
  return packets;
}

////////////////////////////////////////////////////////////////////////////////
//                         num_code_blocks_in_precinct
////////////////////////////////////////////////////////////////////////////////
// The number of code-blocks held by the single precinct of the given
// resolution.  Resolution 0 carries the LL band on its own, and every other
// resolution carries HL, LH and HH; a band of bw by bh samples, starting at
// the origin, is covered by ceil(bw / BLOCK_DIM) * ceil(bh / BLOCK_DIM)
// code-blocks (T.800 B.7).
static ojph::ui32 num_code_blocks_in_precinct(ojph::ui32 res)
{
  ojph::ui32 shift = (res == 0) ? NUM_DECOMPOSITIONS
                                : NUM_DECOMPOSITIONS - res + 1;
  ojph::ui32 bw = (IMAGE_WIDTH  + (1u << shift) - 1) >> shift;
  ojph::ui32 bh = (IMAGE_HEIGHT + (1u << shift) - 1) >> shift;
  ojph::ui32 num_blocks = ((bw + BLOCK_DIM - 1) / BLOCK_DIM)
                        * ((bh + BLOCK_DIM - 1) / BLOCK_DIM);
  return num_blocks * ((res == 0) ? 1u : 3u);
}

////////////////////////////////////////////////////////////////////////////////
//                            make_excluding_packet
////////////////////////////////////////////////////////////////////////////////
// A packet that carries a header but includes none of its code-blocks; the
// counterpart of the empty packet for a decoder that has to walk a packet
// header in a layer other than the first.
//
// Its header is a 1 bit, saying the packet is not empty, followed by the
// inclusion signalling of every code-block of the precinct, and then nothing
// else, since no code-block contributes.  Every code-block of this image is
// included in layer 0 -- the image has detail everywhere, so no code-block
// is all zero -- and for a code-block that a previous layer already included
// the inclusion signalling is a single bit, 0 meaning it does not contribute
// to this layer (T.800 B.10.4).  So the header is one 1 bit and one 0 bit
// per code-block, and because those bits are all 0 their order does not
// matter, only how many there are.  The header is then padded to a byte with
// 0 bits; no byte of it can reach 0xFF, so no bit stuffing arises.
static std::vector<ojph::ui8> make_excluding_packet(ojph::ui32 res)
{
  ojph::ui32 num_bits = 1 + num_code_blocks_in_precinct(res);
  std::vector<ojph::ui8> packet((num_bits + 7) / 8, 0);
  packet[0] = 0x80;                           // the zero length bit, set
  return packet;
}

////////////////////////////////////////////////////////////////////////////////
//                                  packet_ref
////////////////////////////////////////////////////////////////////////////////
// Names one packet of the tile.  The precinct index is not needed because
// there is only one precinct per resolution here.
struct packet_ref
{
  packet_ref(ojph::ui32 r, ojph::ui32 c, ojph::ui32 l)
  : res(r), comp(c), layer(l) {}
  ojph::ui32 res;
  ojph::ui32 comp;
  ojph::ui32 layer;
};

////////////////////////////////////////////////////////////////////////////////
//                               packet_sequence
////////////////////////////////////////////////////////////////////////////////
// The order the packets of the tile appear in, for the given progression
// order and number of layers; T.800 B.12.
//
// The position loops of RPCL, PCRL and CPRL are written out here as a loop
// over resolutions, which is what they come to when every resolution holds
// one precinct: all precincts then project to the top left corner of the
// tile, so the position loop visits them in the order the loops nested
// inside it impose.  That is component major, resolution minor for PCRL and
// CPRL, and resolution major, component minor for RPCL.
static std::vector<packet_ref> packet_sequence(const char* prog_order,
                                               ojph::ui32 num_layers)
{
  std::vector<packet_ref> seq;
  ojph::ui32 r, c, l;

  if (strncmp(prog_order, "LRCP", 4) == 0)
  {
    for (l = 0; l < num_layers; ++l)
      for (r = 0; r < NUM_RESOLUTIONS; ++r)
        for (c = 0; c < NUM_COMPONENTS; ++c)
          seq.push_back(packet_ref(r, c, l));
  }
  else if (strncmp(prog_order, "RLCP", 4) == 0)
  {
    for (r = 0; r < NUM_RESOLUTIONS; ++r)
      for (l = 0; l < num_layers; ++l)
        for (c = 0; c < NUM_COMPONENTS; ++c)
          seq.push_back(packet_ref(r, c, l));
  }
  else if (strncmp(prog_order, "RPCL", 4) == 0)
  {
    for (r = 0; r < NUM_RESOLUTIONS; ++r)
      for (c = 0; c < NUM_COMPONENTS; ++c)          // position, then comp
        for (l = 0; l < num_layers; ++l)
          seq.push_back(packet_ref(r, c, l));
  }
  else if (strncmp(prog_order, "PCRL", 4) == 0
        || strncmp(prog_order, "CPRL", 4) == 0)
  {
    for (c = 0; c < NUM_COMPONENTS; ++c)            // position and comp
      for (r = 0; r < NUM_RESOLUTIONS; ++r)
        for (l = 0; l < num_layers; ++l)
          seq.push_back(packet_ref(r, c, l));
  }
  return seq;
}

////////////////////////////////////////////////////////////////////////////////
//                                assemble_tile
////////////////////////////////////////////////////////////////////////////////
// Lays the packets out in the given order.  Layer 0 carries all of the coded
// data, so a packet of any other layer contributes nothing and is written in
// whichever of the two forms the caller asked for.
static std::vector<ojph::ui8> assemble_tile(
  const std::vector<std::vector<ojph::ui8> >& packets,
  const std::vector<packet_ref>& seq,
  added_layer_kind kind)
{
  std::vector<ojph::ui8> body;
  for (size_t i = 0; i < seq.size(); ++i)
  {
    if (seq[i].layer != 0)
    {
      if (kind == EMPTY_PACKET_LAYER)
        body.push_back(EMPTY_PACKET);
      else
      {
        std::vector<ojph::ui8> p = make_excluding_packet(seq[i].res);
        body.insert(body.end(), p.begin(), p.end());
      }
    }
    else
    {
      const std::vector<ojph::ui8>& p =
        packets[seq[i].res * NUM_COMPONENTS + seq[i].comp];
      body.insert(body.end(), p.begin(), p.end());
    }
  }
  return body;
}

////////////////////////////////////////////////////////////////////////////////
//                              relayer_codestream
////////////////////////////////////////////////////////////////////////////////
// Builds an equivalent codestream that declares num_layers quality layers.
//
// base is a one-layer, one-tile, one-tile-part codestream; body is the new
// sequence of packets for its tile.  Everything up to and including the SOD
// marker is kept as it stands, apart from two fields: the number of layers
// in the COD marker segment, and Psot, which has to follow the length of the
// tile part.
static std::vector<ojph::ui8> relayer_codestream(
  const std::vector<ojph::ui8>& base,
  const std::vector<ojph::ui8>& body,
  ojph::ui32 num_layers)
{
  std::vector<tile_part> parts = scan_tile_parts(base);
  const tile_part& part = parts[0];

  std::vector<ojph::ui8> out(base.begin(), base.begin() + off(part.body_pos));
  out.insert(out.end(), body.begin(), body.end());
  out.push_back((ojph::ui8)(MARKER_EOC >> 8));
  out.push_back((ojph::ui8)(MARKER_EOC & 0xFF));

  // SGcod in the COD segment holds the progression order in one byte and the
  // number of layers in the two that follow it (T.800 A.6.1); the segment
  // starts with the marker and the two byte Lcod, then Scod.
  size_t cod_pos = find_cod_segment(out);
  put_u16(out, cod_pos + 6, num_layers);

  // Psot counts from the SOT marker to the end of the tile part.
  put_u32(out, part.sot_pos + 6,
          (ojph::ui32)(part.body_pos - part.sot_pos + body.size()));
  return out;
}

////////////////////////////////////////////////////////////////////////////////
//                                decode_image
////////////////////////////////////////////////////////////////////////////////
// Decodes a codestream held in memory and returns every sample of every
// component, one component after the other.
static std::vector<ojph::si32> decode_image(const std::vector<ojph::ui8>& buf)
{
  ojph::mem_infile in;
  in.open(buf.data(), buf.size());

  ojph::codestream cs;
  cs.read_headers(&in);
  cs.set_planar(true);
  cs.create();

  ojph::param_siz siz = cs.access_siz();
  std::vector<ojph::si32> samples;
  for (ojph::ui32 c = 0; c < NUM_COMPONENTS; ++c)
  {
    ojph::ui32 height = siz.get_recon_height(c);
    ojph::ui32 width = siz.get_recon_width(c);
    for (ojph::ui32 y = 0; y < height; ++y)
    {
      ojph::ui32 comp_num = 0;
      ojph::line_buf* line = cs.pull(comp_num);
      samples.insert(samples.end(), line->i32, line->i32 + width);
    }
  }
  cs.close();
  return samples;
}

////////////////////////////////////////////////////////////////////////////////
//                             multi_layer_decode
////////////////////////////////////////////////////////////////////////////////
class multi_layer_decode : public ::testing::Test
{
protected:
  void SetUp() override
  {
    packets = collect_packets();
    ASSERT_EQ(packets.size(), PACKETS_PER_LAYER)
      << "the tile-part divided encoding did not produce one tile part per "
      << "resolution and component, so the packets cannot be collected";
  }

  // Re-assembles the one-layer codestream from the collected packets and
  // requires it to be what the encoder wrote, then returns both it and the
  // codestream the same packets make when spread over num_layers layers.
  void build(const char* prog_order, ojph::ui32 num_layers,
             added_layer_kind kind,
             std::vector<ojph::ui8>& one_layer,
             std::vector<ojph::ui8>& many_layers)
  {
    one_layer = encode_test_codestream(prog_order, false);
    std::vector<tile_part> parts = scan_tile_parts(one_layer);
    ASSERT_EQ(parts.size(), 1u) << "expected a single tile part";

    std::vector<ojph::ui8> body =
      assemble_tile(packets, packet_sequence(prog_order, 1), kind);
    ASSERT_EQ(body.size(), parts[0].body_end - parts[0].body_pos)
      << "the re-assembled tile is not the length the encoder wrote";
    ASSERT_TRUE(std::equal(body.begin(), body.end(),
                           one_layer.begin() + off(parts[0].body_pos)))
      << "the re-assembled tile differs from the one the encoder wrote, so "
      << "either the collected packets or the packet order used here is "
      << "wrong";

    body = assemble_tile(packets, packet_sequence(prog_order, num_layers),
                         kind);
    many_layers = relayer_codestream(one_layer, body, num_layers);
  }

  std::vector<std::vector<ojph::ui8> > packets;
};

////////////////////////////////////////////////////////////////////////////////
// The re-layered codestream must really carry the layers it claims: the
// number of layers has to read back from the COD marker segment, and the
// codestream has to have grown by exactly one byte for every empty packet
// that was added.
TEST_F(multi_layer_decode, added_layers_are_present_in_the_codestream)
{
  for (ojph::ui32 i = 0; i < NUM_PROGRESSION_ORDERS; ++i)
    for (ojph::ui32 num_layers = 2; num_layers <= 3; ++num_layers)
    {
      SCOPED_TRACE(::testing::Message()
        << PROGRESSION_ORDERS[i] << ", " << num_layers << " layers");
      std::vector<ojph::ui8> one_layer, many_layers;
      ASSERT_NO_FATAL_FAILURE(build(PROGRESSION_ORDERS[i], num_layers,
                                    EMPTY_PACKET_LAYER, one_layer,
                                    many_layers));

      EXPECT_EQ(many_layers.size(),
                one_layer.size() + PACKETS_PER_LAYER * (num_layers - 1));

      ojph::mem_infile in;
      in.open(many_layers.data(), many_layers.size());
      ojph::codestream cs;
      cs.read_headers(&in);
      EXPECT_EQ((ojph::ui32)cs.access_cod().get_num_layers(), num_layers);
      cs.close();
    }
}

////////////////////////////////////////////////////////////////////////////////
// A codestream whose added layers contribute nothing holds the same image as
// the one-layer codestream it was built from, so the two have to decode to
// the same samples.  This is checked for both forms an added layer can take,
// for two and three layers, and for every progression order; LRCP and RLCP
// are the orders that place the layer outside the precinct loop, and RPCL,
// PCRL and CPRL are the ones that place it innermost.
TEST_F(multi_layer_decode, added_layers_decode_identically)
{
  const added_layer_kind kinds[] =
    { EMPTY_PACKET_LAYER, EXCLUDING_PACKET_LAYER };
  const char* const kind_names[] = { "empty packets", "excluding packets" };

  for (ojph::ui32 k = 0; k < 2; ++k)
    for (ojph::ui32 i = 0; i < NUM_PROGRESSION_ORDERS; ++i)
      for (ojph::ui32 num_layers = 2; num_layers <= 3; ++num_layers)
      {
        SCOPED_TRACE(::testing::Message()
          << PROGRESSION_ORDERS[i] << ", " << num_layers << " layers, "
          << kind_names[k]);
        std::vector<ojph::ui8> one_layer, many_layers;
        ASSERT_NO_FATAL_FAILURE(build(PROGRESSION_ORDERS[i], num_layers,
                                      kinds[k], one_layer, many_layers));

        std::vector<ojph::si32> expected = decode_image(one_layer);
        std::vector<ojph::si32> actual = decode_image(many_layers);
        ASSERT_EQ(expected.size(),
                  (size_t)NUM_COMPONENTS * IMAGE_WIDTH * IMAGE_HEIGHT);
        EXPECT_EQ(actual, expected);
      }
}

} // anonymous namespace
