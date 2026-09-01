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
// File: ojph_precinct.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#include <climits>
#include <cmath>
#include <cstdio>   // for the temporary multi-layer trace
#include <cstdlib>  // for the temporary multi-layer trace
#include <cstring>

#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream_local.h"
#include "ojph_precinct.h"
#include "ojph_subband.h"
#include "ojph_codeblock.h" // for coded_cb_header
#include "ojph_bitbuffer_write.h"
#include "ojph_bitbuffer_read.h"


namespace ojph {

  namespace local
  {

    //////////////////////////////////////////////////////////////////////////
    struct tag_tree
    {
      void init(ui8* buf, ui32 *lev_idx, ui32 num_levels, size s, int init_val)
      {
        for (ui32 i = 0; i <= num_levels; ++i) //on extra level
          levs[i] = buf + lev_idx[i];
        for (ui32 i = num_levels + 1; i < 16; ++i)
          levs[i] = (ui8*)INT_MAX; //make it crash on error
        width = s.w;
        height = s.h;
        for (ui32 i = 0; i < num_levels; ++i)
        {
          ui32 size = 1u << ((num_levels - 1 - i) << 1);
          memset(levs[i], init_val, size);
        }
        *levs[num_levels] = 0;
        this->num_levels = num_levels;
      }

      ui8* get(ui32 x, ui32 y, ui32 lev)
      {
        return levs[lev] + (x + y * ((width + (1 << lev) - 1) >> lev));
      }

      ui32 width, height, num_levels;
      ui8* levs[16]; // you cannot have this high number of levels
    };

    //////////////////////////////////////////////////////////////////////////
    static inline ui32 log2ceil(ui32 x)
    {
      ui32 t = 31 - count_leading_zeros(x);
      return t + (x & (x - 1) ? 1 : 0);
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 precinct::prepare_precinct(int tag_tree_size, ui32* lev_idx,
                                    mem_elastic_allocator* elastic)
    {
      bit_write_buf bb;
      coded_lists *cur_coded_list = NULL;
      ui32 cb_bytes = 0; //cb_bytes;
      ui32 ph_bytes = 0; //precinct header size
      int num_skipped_subbands = 0;
      for (int s = 0; s < 4; ++s)
      {
        if (bands[s].empty)
          continue;

        if (cb_idxs[s].siz.w == 0 || cb_idxs[s].siz.h == 0)
          continue;

        ui32 num_levels = 1 +
          ojph_max(log2ceil(cb_idxs[s].siz.w), log2ceil(cb_idxs[s].siz.h));

        //create quad trees for inclusion and missing msbs
        tag_tree inc_tag, inc_tag_flags, mmsb_tag, mmsb_tag_flags;
        inc_tag.init(scratch, lev_idx, num_levels, cb_idxs[s].siz, 255);
        inc_tag_flags.init(scratch + tag_tree_size,
          lev_idx, num_levels, cb_idxs[s].siz, 0);
        mmsb_tag.init(scratch + (tag_tree_size<<1),
          lev_idx, num_levels, cb_idxs[s].siz, 255);
        mmsb_tag_flags.init(scratch + (tag_tree_size<<1) + tag_tree_size,
          lev_idx, num_levels, cb_idxs[s].siz, 0);
        ui32 band_width = bands[s].num_blocks.w;
        coded_cb_header *cp = bands[s].coded_cbs;
        cp += cb_idxs[s].org.x + cb_idxs[s].org.y * band_width;
        for (ui32 y = 0; y < cb_idxs[s].siz.h; ++y)
        {
          for (ui32 x = 0; x < cb_idxs[s].siz.w; ++x)
          {
            coded_cb_header *p = cp + x;
            *inc_tag.get(x, y, 0) = (p->next_coded == NULL); //1 if true
            *mmsb_tag.get(x, y, 0) = (ui8)p->missing_msbs;
          }
          cp += band_width;
        }
        for (ui32 lev = 1; lev < num_levels; ++lev)
        {
          ui32 height = (cb_idxs[s].siz.h + (1<<lev) - 1) >> lev;
          ui32 width = (cb_idxs[s].siz.w + (1<<lev) - 1) >> lev;
          for (ui32 y = 0; y < height; ++y)
          {
            for (ui32 x = 0; x < width; ++x)
            {
              ui8 t1, t2;
              t1 = ojph_min(*inc_tag.get(x<<1, y<<1, lev-1),
                            *inc_tag.get((x<<1) + 1, y<<1, lev-1));
              t2 = ojph_min(*inc_tag.get(x<<1, (y<<1) + 1, lev-1),
                            *inc_tag.get((x<<1) + 1, (y<<1) + 1, lev-1));
              *inc_tag.get(x, y, lev) = ojph_min(t1, t2);
              *inc_tag_flags.get(x, y, lev) = 0;
              t1 = ojph_min(*mmsb_tag.get(x<<1, y<<1, lev-1),
                            *mmsb_tag.get((x<<1) + 1, y<<1, lev-1));
              t2 = ojph_min(*mmsb_tag.get(x<<1, (y<<1) + 1, lev-1),
                            *mmsb_tag.get((x<<1) + 1, (y<<1) + 1, lev-1));
              *mmsb_tag.get(x, y, lev) = ojph_min(t1, t2);
              *mmsb_tag_flags.get(x, y, lev) = 0;
            }
          }
        }
        *inc_tag.get(0,0,num_levels) = 0;
        *inc_tag_flags.get(0,0,num_levels) = 0;
        *mmsb_tag.get(0,0,num_levels) = 0;
        *mmsb_tag_flags.get(0,0,num_levels) = 0;
        if (*inc_tag.get(0, 0, num_levels-1) != 0) //empty subband
        {
          if (coded) //non empty precinct, tag tree top is 0
            bb_put_bits(&bb, 0, 1, elastic, cur_coded_list, ph_bytes);
          else
            ++num_skipped_subbands;
          continue;
        }
        //now we are in a position to code
        if (coded == NULL)
        {
          bb_init(&bb, elastic, cur_coded_list);
          coded = cur_coded_list;
          //store non empty packet
          bb_put_bit(&bb, 1, elastic, cur_coded_list, ph_bytes);

          // if the first one or two subbands are empty (has codeblocks but
          // no data in them), we need to code them here.
          bb_put_bits(&bb, 0, num_skipped_subbands, elastic, cur_coded_list,
                      ph_bytes);
          num_skipped_subbands = 0; //this line is not needed
        }

        ui32 width = cb_idxs[s].siz.w;
        ui32 height = cb_idxs[s].siz.h;
        for (ui32 y = 0; y < height; ++y)
        {
          cp = bands[s].coded_cbs;
          cp += cb_idxs[s].org.x + (y + cb_idxs[s].org.y) * band_width;
          for (ui32 x = 0; x < width; ++x, ++cp)
          {
            //inclusion bits
            for (ui32 cur_lev = num_levels; cur_lev > 0; --cur_lev)
            {
              ui32 levm1 = cur_lev - 1;
              //check sent
              if (*inc_tag_flags.get(x>>levm1, y>>levm1, levm1) == 0)
              {
                ui32 skipped = *inc_tag.get(x>>levm1, y>>levm1, levm1);
                skipped -= *inc_tag.get(x>>cur_lev, y>>cur_lev, cur_lev);
                assert(skipped <= 1); // for HTJ2K, this should 0 or 1
                bb_put_bits(&bb, 1 - skipped, 1,
                  elastic, cur_coded_list, ph_bytes);
                *inc_tag_flags.get(x>>levm1, y>>levm1, levm1) = 1;
              }
              if (*inc_tag.get(x>>levm1, y>>levm1, levm1) > 0)
                break;
            }

            if (cp->num_passes == 0) //empty codeblock
              continue;

            //missing msbs
            for (ui32 cur_lev = num_levels; cur_lev > 0; --cur_lev)
            {
              ui32 levm1 = cur_lev - 1;
              //check sent
              if (*mmsb_tag_flags.get(x>>levm1, y>>levm1, levm1) == 0)
              {
                int num_zeros = *mmsb_tag.get(x>>levm1, y>>levm1, levm1);
                num_zeros -= *mmsb_tag.get(x>>cur_lev, y>>cur_lev, cur_lev);
                bb_put_zeros(&bb, num_zeros,
                  elastic, cur_coded_list, ph_bytes);
                bb_put_bits(&bb, 1, 1,
                  elastic, cur_coded_list, ph_bytes);
                *mmsb_tag_flags.get(x>>levm1, y>>levm1, levm1) = 1;
              }
            }

            //number of coding passes
            switch (cp->num_passes)
            {
              case 3:
                bb_put_bits(&bb, 12, 4, elastic, cur_coded_list, ph_bytes);
                break;
              case 2:
                bb_put_bits(&bb, 2, 2, elastic, cur_coded_list, ph_bytes);
                break;
              case 1:
                bb_put_bits(&bb, 0, 1, elastic, cur_coded_list, ph_bytes);
                break;
              default:
                assert(0);
            }

            //pass lengths
            //either one, two, or three passes, but only one or two lengths
            int bits1 = 32 - (int)count_leading_zeros(cp->pass_length[0]);
            int extra_bit = cp->num_passes > 2 ? 1 : 0; //for 2nd length
            int bits2 = 0;
            if (cp->num_passes > 1)
              bits2 = 32 - (int)count_leading_zeros(cp->pass_length[1]);
            int bits = ojph_max(bits1, bits2 - extra_bit) - 3;
            bits = ojph_max(bits, 0);
            bb_put_bits(&bb, 0xFFFFFFFEu, bits+1,
              elastic, cur_coded_list, ph_bytes);

            bb_put_bits(&bb, cp->pass_length[0], bits+3,
              elastic, cur_coded_list, ph_bytes);
            if (cp->num_passes > 1)
              bb_put_bits(&bb, cp->pass_length[1], bits+3+extra_bit,
                elastic, cur_coded_list, ph_bytes);

            cb_bytes += cp->pass_length[0] + cp->pass_length[1];
          }
        }
      }

      if (coded)
      {
        bb_terminate(&bb);
        ph_bytes += cur_coded_list->buf_size - cur_coded_list->avail_size;
      }

      return coded ? cb_bytes + ph_bytes : 1; // 1 for empty packet
    }

    //////////////////////////////////////////////////////////////////////////
    void precinct::write(outfile_base *file)
    {
      if (coded)
      {
        //write packet header
        coded_lists *ccl = coded;
        while (ccl)
        {
          file->write(ccl->buf, ccl->buf_size - ccl->avail_size);
          ccl = ccl->next_list;
        }

        //write codeblocks
        for (int s = 0; s < 4; ++s)
        {
          if (bands[s].empty)
            continue;

          ui32 band_width = bands[s].num_blocks.w;
          ui32 width = cb_idxs[s].siz.w;
          ui32 height = cb_idxs[s].siz.h;
          for (ui32 y = 0; y < height; ++y)
          {
            coded_cb_header *cp = bands[s].coded_cbs;
            cp += cb_idxs[s].org.x + (y + cb_idxs[s].org.y) * band_width;
            for (ui32 x = 0; x < width; ++x, ++cp)
            {
              coded_lists *ccl = cp->next_coded;
              while (ccl)
              {
                file->write(ccl->buf, ccl->buf_size - ccl->avail_size);
                ccl = ccl->next_list;
              }
            }
          }
        }
      }
      else
      {
        //empty packet
        char buf = 0x00;
        file->write(&buf, 1);
      }
    }


    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Set OJPH_DEBUG_LAYERS to trace multi-layer packet parsing. Temporary.
    static bool ojph_debug_layers()
    {
      static int state = -1;
      if (state < 0)
        state = getenv("OJPH_DEBUG_LAYERS") != NULL ? 1 : 0;
      return state == 1;
    }

    //////////////////////////////////////////////////////////////////////////
    // Decodes one tag tree leaf up to a threshold, keeping partially decoded
    // state so decoding can continue in a later quality layer.
    //
    // A node's value is a lower bound that grows as 0 bits are read; a 1 bit
    // fixes the value, which the flags tree records. When the value reaches the
    // threshold before being fixed, decoding stops: the caller only learns the
    // value is at least the threshold, and a later layer with a higher
    // threshold resumes from here.
    //
    // With one quality layer the threshold is always 1 and this reduces to
    // reading one bit per level, which is what the single-layer path did.
    static bool tag_tree_decode(bit_read_buf* bb, tag_tree& val, tag_tree& known,
                                ui32 x, ui32 y, ui32 num_levels, ui32 threshold,
                                ui32& result)
    {
      ui32 lower_bound = 0;
      for (ui32 levp1 = num_levels; levp1 > 0; --levp1)
      {
        ui32 cur_lev = levp1 - 1;
        ui8* v = val.get(x >> cur_lev, y >> cur_lev, cur_lev);
        ui8* k = known.get(x >> cur_lev, y >> cur_lev, cur_lev);

        if (*v < lower_bound)
          *v = (ui8)lower_bound;

        while (*k == 0 && *v < threshold)
        {
          ui32 bit;
          if (bb_read_bit(bb, bit) == false)
            return false;
          if (bit)
            *k = 1;
          else
            *v = (ui8)(*v + 1);
        }

        if (*k == 0)
        {
          result = *v;
          return true;
        }

        lower_bound = *v;
      }

      result = lower_bound;
      return true;
    }

    //////////////////////////////////////////////////////////////////////////
    // Reads bytes into a caller supplied buffer with the same accounting
    // bb_read_chunk does. Needed because the segments of one HT set can arrive
    // in different packets and have to end up contiguous.
    static bool read_bytes_into(bit_read_buf* bbp, ui8* dst, ui32 num_bytes)
    {
      ui32 bytes = ojph_min(num_bytes, bbp->bytes_left);
      ui32 bytes_read = (ui32)bbp->file->read(dst, bytes);
      if (num_bytes > bytes_read)
        memset(dst + bytes_read, 0, num_bytes - bytes_read);
      bbp->bytes_left -= bytes_read;
      return bytes_read == bytes;
    }

    //////////////////////////////////////////////////////////////////////////
    // Consumes bytes of an HT set that a later set supersedes.
    static void skip_bytes(bit_read_buf* bbp, infile_base* file, ui32 num_bytes)
    {
      ui32 bytes = ojph_min(num_bytes, bbp->bytes_left);
      si64 start = file->tell();
      file->seek((si64)bytes, infile_base::OJPH_SEEK_CUR);
      bbp->bytes_left -= (ui32)(file->tell() - start);
    }

    //////////////////////////////////////////////////////////////////////////
    void precinct::parse(int tag_tree_size, ui32* lev_idx,
                         mem_elastic_allocator *elastic,
                         ui32 &data_left, infile_base *file, bool skipped,
                         ui32 num_layers)
    {
      assert(data_left > 0);

      // Inclusion and missing-MSB tag trees must survive from one quality layer
      // to the next, so they are built once here rather than per packet, and
      // each subband gets its own storage. This is why all of a precinct's
      // packets are parsed in a single call: the progression orders that reach
      // here place quality layers innermost, so those packets are consecutive.
      tag_tree inc_tag[4], inc_tag_flags[4], mmsb_tag[4], mmsb_tag_flags[4];
      ui32 num_levels[4] = { 0, 0, 0, 0 };

      for (int s = 0; s < 4; ++s)
      {
        if (bands[s].empty || cb_idxs[s].siz.w == 0 || cb_idxs[s].siz.h == 0)
          continue;

        num_levels[s] = 1 +
          ojph_max(log2ceil(cb_idxs[s].siz.w), log2ceil(cb_idxs[s].siz.h));

        ui8* base = scratch + (size_t)s * 4 * (size_t)tag_tree_size;
        inc_tag[s].init(base, lev_idx, num_levels[s], cb_idxs[s].siz, 0);
        *inc_tag[s].get(0, 0, num_levels[s]) = 0;
        inc_tag_flags[s].init(base + tag_tree_size, lev_idx, num_levels[s],
          cb_idxs[s].siz, 0);
        *inc_tag_flags[s].get(0, 0, num_levels[s]) = 0;
        mmsb_tag[s].init(base + (tag_tree_size << 1), lev_idx, num_levels[s],
          cb_idxs[s].siz, 0);
        *mmsb_tag[s].get(0, 0, num_levels[s]) = 0;
        mmsb_tag_flags[s].init(base + (tag_tree_size << 1) + tag_tree_size,
          lev_idx, num_levels[s], cb_idxs[s].siz, 0);
        *mmsb_tag_flags[s].get(0, 0, num_levels[s]) = 0;

        ui32 band_width = bands[s].num_blocks.w;
        for (ui32 y = 0; y < cb_idxs[s].siz.h; ++y)
        {
          coded_cb_header* cp = bands[s].coded_cbs;
          cp += cb_idxs[s].org.x + (y + cb_idxs[s].org.y) * band_width;
          for (ui32 x = 0; x < cb_idxs[s].siz.w; ++x, ++cp)
          {
            cp->included = 0;
            cp->in_layer = 0;
            cp->Lblock_m3 = 0;
            cp->has_cleanup = 0;
            cp->starts_new_set = 0;
            cp->set_passes = 0;
            cp->placeholder_passes = 0;
            cp->pass_index = 0;
            cp->sets_seen = 0;
            cp->zero_bitplanes = 0;
            cp->decoded_set_skip = 0;
            cp->pkt_pre_skip = 0;
            cp->pkt_cleanup = 0;
            cp->pkt_refine = 0;
          }
        }
      }

      for (ui32 layer = 0; layer < num_layers && data_left > 0; ++layer)
      {
        if (ojph_debug_layers())
          fprintf(stderr, ">>> layer %u data_left=%u\n", layer, data_left);

        bit_read_buf bb;
        bb_init(&bb, data_left, file);
        if (may_use_sop)
          bb_skip_sop(&bb);

        // Only codeblocks contributing to this packet have bytes in it.
        for (int s = 0; s < 4; ++s)
        {
          if (bands[s].empty || cb_idxs[s].siz.w == 0 || cb_idxs[s].siz.h == 0)
            continue;
          ui32 band_width = bands[s].num_blocks.w;
          for (ui32 y = 0; y < cb_idxs[s].siz.h; ++y)
          {
            coded_cb_header* cp = bands[s].coded_cbs;
            cp += cb_idxs[s].org.x + (y + cb_idxs[s].org.y) * band_width;
            for (ui32 x = 0; x < cb_idxs[s].siz.w; ++x, ++cp)
              cp->in_layer = 0;
          }
        }

        bool non_empty_bit_read = false;
        bool zero_length_packet = false;

        for (int s = 0; s < 4; ++s)
        {
          if (bands[s].empty)
            continue;

          if (cb_idxs[s].siz.w == 0 || cb_idxs[s].siz.h == 0)
            continue;

          if (!non_empty_bit_read) //one bit to check if the packet is empty
          {
            ui32 bit;
            bb_read_bit(&bb, bit);
            non_empty_bit_read = true;
            if (bit == 0) //empty packet
            { zero_length_packet = true; break; }
          }

          ui32 band_width = bands[s].num_blocks.w;
          ui32 width = cb_idxs[s].siz.w;
          ui32 height = cb_idxs[s].siz.h;
          for (ui32 y = 0; y < height; ++y)
          {
            coded_cb_header *cp = bands[s].coded_cbs;
            cp += cb_idxs[s].org.x + (y + cb_idxs[s].org.y) * band_width;
            for (ui32 x = 0; x < width; ++x, ++cp)
            {
              bool first_contribution = false;

              //process inclusion
              if (cp->included)
              {
                // Already contributing: one bit says whether this layer adds
                // anything.
                ui32 bit;
                if (bb_read_bit(&bb, bit) == false)
                { data_left = 0; throw "error reading from file p1"; }
                if (bit == 0)
                  continue;
              }
              else
              {
                // Not yet contributing: the tag tree codes the layer of first
                // inclusion, decoded up to this layer's threshold.
                ui32 first_layer = 0;
                if (tag_tree_decode(&bb, inc_tag[s], inc_tag_flags[s], x, y,
                      num_levels[s], layer + 1, first_layer) == false)
                { data_left = 0; throw "error reading from file p1"; }
                if (first_layer > layer)
                  continue;

                //process missing msbs
                ui32 mmsbs = 0;
                for (ui32 levp1 = num_levels[s]; levp1 > 0; --levp1)
                {
                  ui32 cur_lev = levp1 - 1;
                  mmsbs = *mmsb_tag[s].get(x>>levp1, y>>levp1, levp1);
                  //check received
                  if (*mmsb_tag_flags[s].get(x>>cur_lev, y>>cur_lev, cur_lev) == 0)
                  {
                    ui32 bit = 0;
                    while (bit == 0)
                    {
                      if (bb_read_bit(&bb, bit) == false)
                      { data_left = 0; throw "error reading from file p2"; }
                      mmsbs += 1 - bit;
                    }
                    *mmsb_tag[s].get(x>>cur_lev, y>>cur_lev, cur_lev) = (ui8)mmsbs;
                    *mmsb_tag_flags[s].get(x>>cur_lev, y>>cur_lev, cur_lev) = 1;
                  }
                }

                if (mmsbs > cp->Kmax)
                  throw "error in parsing a tile header; "
                  "missing msbs are larger or equal to Kmax. The most likely "
                  "cause is a corruption in the bitstream.";
                cp->zero_bitplanes = mmsbs;
                cp->included = 1;
                first_contribution = true;
              }

              //get number of passes
              ui32 bit, num_passes = 1;
              if (bb_read_bit(&bb, bit) == false)
              { data_left = 0; throw "error reading from file p3"; }
              if (bit)
              {
                num_passes = 2;
                if (bb_read_bit(&bb, bit) == false)
                { data_left = 0; throw "error reading from file p4"; }
                if (bit)
                {
                  if (bb_read_bits(&bb, 2, bit) == false)
                  { data_left = 0; throw "error reading from file p5";  }
                  num_passes = 3 + bit;
                  if (bit == 3)
                  {
                    if (bb_read_bits(&bb, 5, bit) == false)
                    { data_left = 0; throw "error reading from file p6"; }
                    num_passes = 6 + bit;
                    if (bit == 31)
                    {
                      if (bb_read_bits(&bb, 7, bit) == false)
                      { data_left = 0; throw "error reading from file p7"; }
                      num_passes = 37 + bit;
                    }
                  }
                }
              }

              // Lblock grows monotonically over the layers that include this
              // codeblock, so it lives with the codeblock.
              int Lblock = 3 + cp->Lblock_m3;
              bit = 1;
              while (bit)
              {
                if (bb_read_bit(&bb, bit) == false)
                { data_left = 0; throw "error reading from file p8"; }
                Lblock += bit;
              }
              cp->Lblock_m3 = (ui8)(Lblock - 3);

              // A codeblock's placeholder passes come in groups of three, all
              // ahead of its first HT set, and share a codeword segment with
              // the first HT cleanup pass (T.814 B.1, B.2). A first
              // contribution therefore carries 3*P0 + 1..3 passes, which is
              // what fixes P0.
              if (first_contribution)
                cp->placeholder_passes = ((num_passes - 1) / 3) * 3;

              // T.814 B.2: segment boundaries lie at pass indices
              //   T = union over k of (3*P0 + ceil(3k/2)),
              // so past the placeholder run a cleanup segment covers one pass
              // and the refinement segment after it covers up to two. Each
              // segment carries its own length, so walk them one at a time.
              ui32 remaining = num_passes;
              cp->pkt_pre_skip = 0;
              cp->pkt_cleanup = 0;
              cp->pkt_refine = 0;
              cp->starts_new_set = 0;

              while (remaining > 0)
              {
                bool is_cleanup;
                ui32 seg_passes;

                if (cp->pass_index <= cp->placeholder_passes)
                {
                  // The placeholder passes and the first cleanup pass form one
                  // codeword segment.
                  is_cleanup = true;
                  seg_passes = cp->placeholder_passes + 1 - cp->pass_index;
                }
                else
                {
                  ui32 rel = cp->pass_index - cp->placeholder_passes;
                  if (rel % 3 == 0)      { is_cleanup = true;  seg_passes = 1; }
                  else if (rel % 3 == 1) { is_cleanup = false; seg_passes = 2; }
                  else                   { is_cleanup = false; seg_passes = 1; }
                }

                if (seg_passes > remaining)
                  seg_passes = remaining;

                int bits = Lblock + 31 - (int)count_leading_zeros(seg_passes);
                if (bb_read_bits(&bb, bits, bit) == false)
                { data_left = 0; throw "error reading from file p9"; }
                ui32 seg_len = bit;

                if (ojph_debug_layers())
                  fprintf(stderr, "    L%u s%d cb(%u,%u) np=%u p0=%u idx=%u "
                    "%s passes=%u Lblock=%d bits=%d len=%u\n", layer, s, x, y,
                    num_passes, cp->placeholder_passes, cp->pass_index,
                    is_cleanup ? "cleanup" : "refine ", seg_passes, Lblock,
                    bits, seg_len);

                if (is_cleanup)
                {
                  ++cp->sets_seen;
                  // A zero length cleanup segment means the HT set contributes
                  // nothing; such sets exist to skip magnitude bit-planes
                  // (T.814 B.3), and the refinement segment of the same set is
                  // empty too.
                  if (seg_len > 0)
                  {
                    // A new HT set supersedes anything gathered before it.
                    cp->pkt_pre_skip += cp->pkt_cleanup + cp->pkt_refine;
                    cp->pkt_cleanup = seg_len;
                    cp->pkt_refine = 0;
                    cp->starts_new_set = 1;
                    cp->set_passes = (ui8)seg_passes;
                    // T.814 B.3: S_skip counts the HT sets ahead of the one
                    // being decoded, so it has to be taken now. Empty sets
                    // arriving later must not inflate it.
                    cp->decoded_set_skip = cp->sets_seen - 1;
                  }
                }
                else if (seg_len > 0)
                {
                  if (cp->starts_new_set || cp->has_cleanup)
                  {
                    cp->pkt_refine += seg_len;
                    cp->set_passes = (ui8)(cp->set_passes + seg_passes);
                  }
                  else
                    cp->pkt_pre_skip += seg_len;
                }

                cp->pass_index += seg_passes;
                remaining -= seg_passes;
              }

              if (cp->pkt_cleanup || cp->pkt_refine || cp->pkt_pre_skip)
                cp->in_layer = 1;
            }
          }
        }

        if (!non_empty_bit_read)
        { // all subbands are empty, so the zero length bit was never read
          ui32 bit = 0;
          bb_read_bit(&bb, bit);
        }
        bb_terminate(&bb, uses_eph);

        //read codeblock data
        if (!zero_length_packet)
        {
          for (int s = 0; s < 4; ++s)
          {
            if (bands[s].empty)
              continue;

            ui32 band_width = bands[s].num_blocks.w;
            ui32 width = cb_idxs[s].siz.w;
            ui32 height = cb_idxs[s].siz.h;
            for (ui32 y = 0; y < height; ++y)
            {
              coded_cb_header *cp = bands[s].coded_cbs;
              cp += cb_idxs[s].org.x + (y + cb_idxs[s].org.y) * band_width;
              for (ui32 x = 0; x < width; ++x, ++cp)
              {
                if (cp->in_layer == 0 || data_left == 0)
                  continue;

                if (skipped)
                { //no need to read
                  skip_bytes(&bb, file,
                    cp->pkt_pre_skip + cp->pkt_cleanup + cp->pkt_refine);
                  cp->pass_length[0] = cp->pass_length[1] = 0;
                  continue;
                }

                // Bytes of HT sets that a later set replaces are consumed and
                // dropped.
                if (cp->pkt_pre_skip)
                  skip_bytes(&bb, file, cp->pkt_pre_skip);

                if (cp->starts_new_set)
                {
                  ui32 total = cp->pkt_cleanup + cp->pkt_refine;
                  if (!bb_read_chunk(&bb, total, cp->next_coded, elastic))
                  {
                    cp->pass_length[0] = cp->pass_length[1] = 0;
                    data_left = 0;
                    continue;
                  }
                  cp->pass_length[0] = cp->pkt_cleanup;
                  cp->pass_length[1] = cp->pkt_refine;
                  cp->has_cleanup = 1;
                }
                else if (cp->pkt_refine)
                {
                  // T.814 B.3: an HT segment's bytes are the concatenation of
                  // its codeword segments, so a refinement segment arriving
                  // after its cleanup segment is appended rather than replacing
                  // it.
                  ui32 held = cp->pass_length[0] + cp->pass_length[1];
                  coded_lists* previous = cp->next_coded;
                  elastic->get_buffer(held + cp->pkt_refine
                    + coded_cb_header::prefix_buf_size
                    + coded_cb_header::suffix_buf_size, cp->next_coded);
                  if (held && previous)
                    memcpy(cp->next_coded->buf
                      + coded_cb_header::prefix_buf_size,
                      previous->buf + coded_cb_header::prefix_buf_size, held);
                  if (!read_bytes_into(&bb, cp->next_coded->buf
                        + coded_cb_header::prefix_buf_size + held,
                        cp->pkt_refine))
                  {
                    cp->pass_length[0] = cp->pass_length[1] = 0;
                    data_left = 0;
                    continue;
                  }
                  cp->pass_length[1] += cp->pkt_refine;
                }

                // T.814 B.3: Z_blk, the number of coding passes the decoder
                // processes, is 1 when the cleanup segment is the only
                // non-empty segment of the set.
                cp->num_passes = cp->pass_length[1] == 0
                  ? 1u : (ui32)cp->set_passes;

                // T.814 B.3: S_blk = P + P0 + S_skip, where S_skip counts the
                // HT sets ahead of the one being decoded.
                cp->missing_msbs = cp->zero_bitplanes
                  + cp->placeholder_passes / 3
                  + cp->decoded_set_skip;

                if (ojph_debug_layers())
                  fprintf(stderr, "    -> cb(%u,%u) len0=%u len1=%u Zblk=%u "
                    "P=%u P0=%u Sskip=%u Sblk=%u\n", x, y, cp->pass_length[0],
                    cp->pass_length[1], cp->num_passes, cp->zero_bitplanes,
                    cp->placeholder_passes / 3, cp->decoded_set_skip,
                    cp->missing_msbs);
              }
            }
          }
        }
        data_left = bb.bytes_left;
        if (ojph_debug_layers())
          fprintf(stderr, "<<< layer %u done zero_pkt=%d data_left=%u\n",
            layer, (int)zero_length_packet, data_left);
      }
    }

  }
}