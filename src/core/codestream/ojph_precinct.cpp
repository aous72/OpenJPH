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

        // A parent's value is a lower bound for its children.
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
          // Value is at least the threshold; nothing more is known yet.
          result = *v;
          return true;
        }

        lower_bound = *v;
      }

      result = lower_bound;
      return true;
    }

    //////////////////////////////////////////////////////////////////////////
    void precinct::parse(int tag_tree_size, ui32* lev_idx,
                         mem_elastic_allocator *elastic,
                         ui32 &data_left, infile_base *file, bool skipped,
                         ui32 num_layers)
    {
      assert(data_left > 0);
      static int parse_call = 0;
      ++parse_call;
      if (ojph_debug_layers())
        fprintf(stderr, "=== parse #%d num_layers=%u data_left=%u\n",
          parse_call, num_layers, data_left);

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

        // Four tables per subband, laid out consecutively in the scratch space.
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

        // Per-codeblock inclusion state also spans layers.
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

        // Only codeblocks contributing to this packet have bytes in it, so the
        // flag is cleared for everyone before the header is read. Their
        // pass_length values are left alone: they still describe the last set
        // each codeblock contributed, which is what the decoder needs.
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
            if (ojph_debug_layers())
              fprintf(stderr, "    non_empty_bit=%u (s=%d)\n", bit, s);
            if (bit == 0) //empty packet
            { zero_length_packet = true; break; }
          }

          //
          ui32 band_width = bands[s].num_blocks.w;
          ui32 width = cb_idxs[s].siz.w;
          ui32 height = cb_idxs[s].siz.h;
          for (ui32 y = 0; y < height; ++y)
          {
            coded_cb_header *cp = bands[s].coded_cbs;
            cp += cb_idxs[s].org.x + (y + cb_idxs[s].org.y) * band_width;
            for (ui32 x = 0; x < width; ++x, ++cp)
            {
              //process inclusion
              if (cp->included)
              {
                // Already contributing: one bit says whether this layer adds
                // anything.
                ui32 bit;
                if (bb_read_bit(&bb, bit) == false)
                { data_left = 0; throw "error reading from file p1"; }
                if (ojph_debug_layers())
                  fprintf(stderr, "    L%u s%d cb(%u,%u) again-inc=%u\n",
                    layer, s, x, y, bit);
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
                if (ojph_debug_layers())
                  fprintf(stderr, "    L%u s%d cb(%u,%u) first_layer=%u\n",
                    layer, s, x, y, first_layer);
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
                cp->missing_msbs = mmsbs;
                cp->included = 1;
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

              // Parse pass lengths
              // When number of passes is one, one length.
              // When number of passes is two or three, two lengths.
              // When number of passes > 3, we have place holder passes;
              // In this case, subtract multiples of 3 from the number of
              // passes; for example, if we have 10 passes, we subtract 9,
              // producing 1 pass.

              // 1 => 1, 2 => 2, 3 => 3, 4 => 1, 5 => 2, 6 => 3
              ui32 num_phld_passes = (num_passes - 1) / 3;
              cp->missing_msbs += num_phld_passes;

              num_phld_passes *= 3;
              cp->num_passes = num_passes - num_phld_passes;
              cp->pass_length[0] = cp->pass_length[1] = 0;

              // Lblock grows monotonically over the layers that include this
              // codeblock, so it lives with the codeblock rather than being
              // reset per packet.
              int Lblock = 3 + cp->Lblock_m3;
              bit = 1;
              while (bit)
              {
                // add any extra bits here
                if (bb_read_bit(&bb, bit) == false)
                { data_left = 0; throw "error reading from file p8"; }
                Lblock += bit;
              }
              cp->Lblock_m3 = (ui8)(Lblock - 3);

              int bits = Lblock + 31 -
                (int)count_leading_zeros(num_phld_passes + 1);
              if (bb_read_bits(&bb, bits, bit) == false)
              { data_left = 0; throw "error reading from file p9"; }

              if (ojph_debug_layers())
                fprintf(stderr, "L%u s%d cb(%u,%u) inc=%u np=%u phld=%u "
                  "Lblock=%d bits=%d len0=%u\n", layer, s, x, y, cp->included,
                  num_passes, num_phld_passes, Lblock, bits, bit);

              if (bit < 2)
                throw "The cleanup segment of an HT codeblock cannot contain "
                  "less than 2 bytes";
              if (bit >= 65535)
                throw "The cleanup segment of an HT codeblock must contain "
                  "less than 65535 bytes";
              cp->pass_length[0] = bit;

              if (cp->num_passes > 1)
              {
                //bits = Lblock + 31 - count_leading_zeros(cp->num_passes - 1);
                // The following is simpler than the above, I think?
                bits = Lblock + (cp->num_passes > 2 ? 1 : 0);
                if (bb_read_bits(&bb, bits, bit) == false)
                { data_left = 0; throw "error reading from file p10"; }
                if (bit >= 2047)
                  throw "The refinement segment (SigProp and MagRep passes) of "
                    "an HT codeblock must contain less than 2047 bytes";
                cp->pass_length[1] = bit;
                if (ojph_debug_layers())
                  fprintf(stderr, "        len1=%u (bits=%d)\n", bit, bits);
              }

              cp->in_layer = 1;
            }
          }
        }

        if (!non_empty_bit_read)
        { // all subbands are empty, so the zero length bit was never read
          ui32 bit = 0;
          bb_read_bit(&bb, bit);
          //assert(bit == 0);
        }
        if (ojph_debug_layers())
          fprintf(stderr, "    header done, bytes_left=%u\n", bb.bytes_left);
        bb_terminate(&bb, uses_eph);
        if (ojph_debug_layers())
          fprintf(stderr, "    after terminate, bytes_left=%u\n", bb.bytes_left);

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
                // Codeblocks that did not contribute to this packet have no
                // bytes in it; their pass_length still refers to an earlier
                // packet and must not be read again.
                if (cp->in_layer == 0)
                  continue;

                ui32 num_bytes = cp->pass_length[0] + cp->pass_length[1];
                if (data_left)
                {
                  if (num_bytes)
                  {
                    if (skipped)
                    { //no need to read
                      si64 cur_loc = file->tell();
                      ui32 t = ojph_min(num_bytes, bb.bytes_left);
                      file->seek(t, infile_base::OJPH_SEEK_CUR);
                      ui32 bytes_read = (ui32)(file->tell() - cur_loc);
                      cp->pass_length[0] = cp->pass_length[1] = 0;
                      bb.bytes_left -= bytes_read;
                      assert(bytes_read == t || bb.bytes_left == 0);
                    }
                    else
                    {
                      // A later layer carrying a new HT set supersedes the
                      // earlier one: get_buffer hands back a fresh buffer, so
                      // the codeblock keeps the last set it was given, which is
                      // the highest quality one.
                      if (!bb_read_chunk(&bb, num_bytes, cp->next_coded,
                            elastic))
                      {
                        //no need to decode a broken codeblock
                        cp->pass_length[0] = cp->pass_length[1] = 0;
                        data_left = 0;
                      }
                    }
                  }
                }
                else
                  cp->pass_length[0] = cp->pass_length[1] = 0;
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