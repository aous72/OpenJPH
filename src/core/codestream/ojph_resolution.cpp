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
// File: ojph_resolution.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#include <climits>
#include <cmath>

#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream_local.h"
#include "ojph_resolution.h"
#include "ojph_tile_comp.h"
#include "ojph_tile.h"
#include "ojph_subband.h"
#include "ojph_precinct.h"

#include "../transform/ojph_transform.h"

namespace ojph {

  namespace local
  {

    //////////////////////////////////////////////////////////////////////////
    static void rotate_buffers(line_buf* line1, line_buf* line2,
                               line_buf* line3, line_buf* line4)
    {
      assert(line1->size == line2->size &&
             line1->pre_size == line2->pre_size &&
             line1->size == line3->size &&
             line1->pre_size == line3->pre_size &&
             line1->size == line4->size &&
             line1->pre_size == line4->pre_size);
      si32* p = line4->i32;
      line4->i32 = line3->i32;
      line3->i32 = line2->i32;
      line2->i32 = line1->i32;
      line1->i32 = p;
    }

    //////////////////////////////////////////////////////////////////////////
    static void rotate_buffers(line_buf* line1, line_buf* line2,
                               line_buf* line3, line_buf* line4,
                               line_buf* line5, line_buf* line6)
    {
      assert(line1->size == line2->size &&
             line1->pre_size == line2->pre_size &&
             line1->size == line3->size &&
             line1->pre_size == line3->pre_size &&
             line1->size == line4->size &&
             line1->pre_size == line4->pre_size &&
             line1->size == line5->size &&
             line1->pre_size == line5->pre_size &&
             line1->size == line6->size &&
             line1->pre_size == line6->pre_size);
      si32* p = line6->i32;
      line6->i32 = line5->i32;
      line5->i32 = line4->i32;
      line4->i32 = line3->i32;
      line3->i32 = line2->i32;
      line2->i32 = line1->i32;
      line1->i32 = p;
    }

    //////////////////////////////////////////////////////////////////////////
    void resolution::pre_alloc(codestream* codestream, const rect& res_rect,
                               const rect& recon_res_rect, ui32 res_num)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();
      const param_cod* cdp = codestream->get_cod();
      ui32 t = codestream->get_cod()->get_num_decompositions()
             - codestream->get_skipped_res_for_recon();
      bool skipped_res_for_recon = res_num > t;

      //create next resolution
      if (res_num > 0)
      {
        //allocate a resolution
        allocator->pre_alloc_obj<resolution>(1);
        ui32 trx0 = ojph_div_ceil(res_rect.org.x, 2);
        ui32 try0 = ojph_div_ceil(res_rect.org.y, 2);
        ui32 trx1 = ojph_div_ceil(res_rect.org.x + res_rect.siz.w, 2);
        ui32 try1 = ojph_div_ceil(res_rect.org.y + res_rect.siz.h, 2);
        rect next_res_rect;
        next_res_rect.org.x = trx0;
        next_res_rect.org.y = try0;
        next_res_rect.siz.w = trx1 - trx0;
        next_res_rect.siz.h = try1 - try0;

        resolution::pre_alloc(codestream, next_res_rect,
          skipped_res_for_recon ? recon_res_rect : next_res_rect, res_num - 1);
      }

      //allocate subbands
      ui32 trx0 = res_rect.org.x;
      ui32 try0 = res_rect.org.y;
      ui32 trx1 = res_rect.org.x + res_rect.siz.w;
      ui32 try1 = res_rect.org.y + res_rect.siz.h;
      allocator->pre_alloc_obj<subband>(4);
      if (res_num > 0)
      {
        for (ui32 i = 1; i < 4; ++i)
        {
          ui32 tbx0 = (trx0 - (i & 1) + 1) >> 1;
          ui32 tbx1 = (trx1 - (i & 1) + 1) >> 1;
          ui32 tby0 = (try0 - (i >> 1) + 1) >> 1;
          ui32 tby1 = (try1 - (i >> 1) + 1) >> 1;

          rect band_rect;
          band_rect.org.x = tbx0;
          band_rect.org.y = tby0;
          band_rect.siz.w = tbx1 - tbx0;
          band_rect.siz.h = tby1 - tby0;
          subband::pre_alloc(codestream, band_rect, res_num);
        }
      }
      else
        subband::pre_alloc(codestream, res_rect, res_num);

      //prealloc precincts
      size log_PP = cdp->get_log_precinct_size(res_num);
      size num_precincts;
      if (trx0 != trx1 && try0 != try1)
      {
        num_precincts.w = (trx1 + (1 << log_PP.w) - 1) >> log_PP.w;
        num_precincts.w -= trx0 >> log_PP.w;
        num_precincts.h = (try1 + (1 << log_PP.h) - 1) >> log_PP.h;
        num_precincts.h -= try0 >> log_PP.h;
        allocator->pre_alloc_obj<precinct>((size_t)num_precincts.area());
      }

      //allocate lines
      if (skipped_res_for_recon == false)
      {
        bool reversible = cdp->is_reversible();
        ui32 num_lines = reversible ? 4 : 6;
        allocator->pre_alloc_obj<line_buf>(num_lines);

        ui32 width = res_rect.siz.w + 1;
        for (ui32 i = 0; i < num_lines; ++i)
          allocator->pre_alloc_data<si32>(width, 1);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void resolution::finalize_alloc(codestream* codestream,
                                    const rect& res_rect,
                                    const rect& recon_res_rect,
                                    ui32 comp_num, ui32 res_num,
                                    point comp_downsamp,
                                    tile_comp* parent_tile_comp,
                                    resolution* parent_res)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();
      elastic = codestream->get_elastic_alloc();
      ui32 t, num_decomps = codestream->get_cod()->get_num_decompositions();
      t = num_decomps - codestream->get_skipped_res_for_recon();
      skipped_res_for_recon = res_num > t;
      t = num_decomps - codestream->get_skipped_res_for_read();
      skipped_res_for_read = res_num > t;
      const param_cod* cdp = codestream->get_cod();

      this->comp_downsamp = comp_downsamp;
      this->parent_comp = parent_tile_comp;
      this->parent_res = parent_res;
      this->res_rect = res_rect;
      this->comp_num = comp_num;
      this->res_num = res_num;
      this->num_bytes = 0;
      //finalize next resolution
      if (res_num > 0)
      {
        //allocate a resolution
        child_res = allocator->post_alloc_obj<resolution>(1);
        ui32 trx0 = ojph_div_ceil(res_rect.org.x, 2);
        ui32 try0 = ojph_div_ceil(res_rect.org.y, 2);
        ui32 trx1 = ojph_div_ceil(res_rect.org.x + res_rect.siz.w, 2);
        ui32 try1 = ojph_div_ceil(res_rect.org.y + res_rect.siz.h, 2);
        rect next_res_rect;
        next_res_rect.org.x = trx0;
        next_res_rect.org.y = try0;
        next_res_rect.siz.w = trx1 - trx0;
        next_res_rect.siz.h = try1 - try0;

        child_res->finalize_alloc(codestream, next_res_rect,
          skipped_res_for_recon ? recon_res_rect : next_res_rect, comp_num,
          res_num - 1, comp_downsamp, parent_tile_comp, this);
      }
      else
        child_res = NULL;

      //allocate subbands
      ui32 trx0 = res_rect.org.x;
      ui32 try0 = res_rect.org.y;
      ui32 trx1 = res_rect.org.x + res_rect.siz.w;
      ui32 try1 = res_rect.org.y + res_rect.siz.h;
      bands = allocator->post_alloc_obj<subband>(4);
      if (res_num > 0)
      {
        this->num_bands = 3;
        for (ui32 i = 1; i < 4; ++i)
        {
          ui32 tbx0 = (trx0 - (i & 1) + 1) >> 1;
          ui32 tbx1 = (trx1 - (i & 1) + 1) >> 1;
          ui32 tby0 = (try0 - (i >> 1) + 1) >> 1;
          ui32 tby1 = (try1 - (i >> 1) + 1) >> 1;

          rect band_rect;
          band_rect.org.x = tbx0;
          band_rect.org.y = tby0;
          band_rect.siz.w = tbx1 - tbx0;
          band_rect.siz.h = tby1 - tby0;
          bands[i].finalize_alloc(codestream, band_rect, this, res_num, i);
        }
      }
      else {
        this->num_bands = 1;
        bands[0].finalize_alloc(codestream, res_rect, this, res_num, 0);
      }

      //finalize precincts
      log_PP = cdp->get_log_precinct_size(res_num);
      num_precincts = size();
      precincts = NULL;
      if (trx0 != trx1 && try0 != try1)
      {
        num_precincts.w = (trx1 + (1 << log_PP.w) - 1) >> log_PP.w;
        num_precincts.w -= trx0 >> log_PP.w;
        num_precincts.h = (try1 + (1 << log_PP.h) - 1) >> log_PP.h;
        num_precincts.h -= try0 >> log_PP.h;
        precincts = 
          allocator->post_alloc_obj<precinct>((size_t)num_precincts.area());
        ui64 num = num_precincts.area();
        for (ui64 i = 0; i < num; ++i)
          precincts[i] = precinct();
      }
      // precincts will be initialized in full shortly

      ui32 x_lower_bound = (trx0 >> log_PP.w) << log_PP.w;
      ui32 y_lower_bound = (try0 >> log_PP.h) << log_PP.h;

      point proj_factor;
      proj_factor.x = comp_downsamp.x * (1 << (num_decomps - res_num));
      proj_factor.y = comp_downsamp.y * (1 << (num_decomps - res_num));
      precinct* pp = precincts;

      point tile_top_left = parent_tile_comp->get_tile()->get_tile_rect().org;
      for (ui32 y = 0; y < num_precincts.h; ++y)
      {
        ui32 ppy0 = y_lower_bound + (y << log_PP.h);
        for (ui32 x = 0; x < num_precincts.w; ++x, ++pp)
        {
          ui32 ppx0 = x_lower_bound + (x << log_PP.w);
          point t(proj_factor.x * ppx0, proj_factor.y * ppy0);
          t.x = t.x > tile_top_left.x ? t.x : tile_top_left.x;
          t.y = t.y > tile_top_left.y ? t.y : tile_top_left.y;
          pp->img_point = t;
          pp->num_bands = num_bands;
          pp->bands = bands;
          pp->may_use_sop = cdp->packets_may_use_sop();
          pp->uses_eph = cdp->packets_use_eph();
          pp->scratch = codestream->get_precinct_scratch();
          pp->coded = NULL;
        }
      }
      if (num_bands == 1)
        bands[0].get_cb_indices(num_precincts, precincts);
      else
        for (int i = 1; i < 4; ++i)
          bands[i].get_cb_indices(num_precincts, precincts);

      size log_cb = cdp->get_log_block_dims();
      log_PP.w -= (res_num ? 1 : 0);
      log_PP.h -= (res_num ? 1 : 0);
      size ratio;
      ratio.w = log_PP.w - ojph_min(log_cb.w, log_PP.w);
      ratio.h = log_PP.h - ojph_min(log_cb.h, log_PP.h);
      max_num_levels = ojph_max(ratio.w, ratio.h);
      ui32 val = 1u << (max_num_levels << 1);
      tag_tree_size = (int)((val * 4 + 2) / 3);
      ++max_num_levels;
      level_index[0] = 0;
      for (ui32 i = 1; i <= max_num_levels; ++i, val >>= 2)
        level_index[i] = level_index[i - 1] + val;
      cur_precinct_loc = point(0, 0);

      //allocate lines
      if (skipped_res_for_recon == false)
      {
        this->reversible = cdp->is_reversible();
        this->num_lines = this->reversible ? 4 : 6;
        lines = allocator->post_alloc_obj<line_buf>(num_lines);

        ui32 width = res_rect.siz.w + 1;
        for (ui32 i = 0; i < num_lines; ++i)
          lines[i].wrap(allocator->post_alloc_data<si32>(width, 1), width, 1);
        cur_line = 0;
        vert_even = (res_rect.org.y & 1) == 0;
        horz_even = (res_rect.org.x & 1) == 0;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void resolution::push_line()
    {
      if (res_num == 0)
      {
        assert(num_bands == 1 && child_res == NULL);
        bands[0].exchange_buf(lines + 0);//line at location 0
        bands[0].push_line();
        return;
      }

      ui32 width = res_rect.siz.w;
      if (width == 0)
        return;
      if (reversible)
      {
        //vertical transform
        assert(num_lines >= 4);
        if (vert_even)
        {
          rev_vert_wvlt_fwd_predict(lines,
                                    cur_line > 1 ? lines + 2 : lines,
                                    lines + 1, width);
          rev_vert_wvlt_fwd_update(lines + 1,
                                   cur_line > 2 ? lines + 3 : lines + 1,
                                   lines + 2, width);

          // push to horizontal transform lines[2](L) and lines[1] (H)
          if (cur_line >= 1)
          {
            rev_horz_wvlt_fwd_tx(lines + 1, bands[2].get_line(),
              bands[3].get_line(), width, horz_even);
            bands[2].push_line();
            bands[3].push_line();
          }
          if (cur_line >= 2)
          {
            rev_horz_wvlt_fwd_tx(lines + 2, child_res->get_line(),
              bands[1].get_line(), width, horz_even);
            bands[1].push_line();
            child_res->push_line();
          }
        }

        if (cur_line >= res_rect.siz.h - 1)
        { //finished, so we need to process any lines left
          if (cur_line)
          {
            if (vert_even)
            {
              rev_vert_wvlt_fwd_update(lines + 1, lines + 1,
                                       lines, width);
              //push lines[0] to L
              rev_horz_wvlt_fwd_tx(lines, child_res->get_line(),
                bands[1].get_line(), width, horz_even);
              bands[1].push_line();
              child_res->push_line();
            }
            else
            {
              rev_vert_wvlt_fwd_predict(lines + 1, lines + 1,
                                        lines, width);
              rev_vert_wvlt_fwd_update(lines,
                                       cur_line > 1 ? lines + 2 : lines,
                                       lines + 1, width);

              // push to horizontal transform lines[1](L) and line[0] (H)
              //line[0] to H
              rev_horz_wvlt_fwd_tx(lines, bands[2].get_line(),
                bands[3].get_line(), width, horz_even);
              bands[2].push_line();
              bands[3].push_line();
              //line[1] to L
              rev_horz_wvlt_fwd_tx(lines + 1, child_res->get_line(),
                bands[1].get_line(), width, horz_even);
              bands[1].push_line();
              child_res->push_line();
            }
          }
          else
          { //only one line
            if (vert_even)
            {
              //push to L
              rev_horz_wvlt_fwd_tx(lines, child_res->get_line(),
                bands[1].get_line(), width, horz_even);
              bands[1].push_line();
              child_res->push_line();
            }
            else
            {
              si32* sp = lines[0].i32;
              for (ui32 i = width; i > 0; --i)
                *sp++ <<= 1;
              //push to H
              rev_horz_wvlt_fwd_tx(lines, bands[2].get_line(),
                bands[3].get_line(), width, horz_even);
              bands[2].push_line();
              bands[3].push_line();
            }
          }
        }

        rotate_buffers(lines, lines + 1, lines + 2, lines + 3);

        ++cur_line;
        vert_even = !vert_even;
      }
      else
      {
        //vertical transform
        assert(num_lines >= 6);
        if (vert_even)
        {
          irrev_vert_wvlt_step(lines + 0,
                               cur_line > 1 ? lines + 2 : lines,
                               lines + 1, 0, width);
          irrev_vert_wvlt_step(lines + 1,
                               cur_line > 2 ? lines + 3 : lines + 1,
                               lines + 2, 1, width);
          irrev_vert_wvlt_step(lines + 2,
                               cur_line > 3 ? lines + 4 : lines + 2,
                               lines + 3, 2, width);
          irrev_vert_wvlt_step(lines + 3,
                               cur_line > 4 ? lines + 5 : lines + 3,
                               lines + 4, 3, width);

          // push to horizontal transform lines[4](L) and lines[3] (H)
          if (cur_line >= 3)
          {
            irrev_vert_wvlt_K(lines + 3, lines + 5,
                              false, width);
            irrev_horz_wvlt_fwd_tx(lines + 5, bands[2].get_line(),
              bands[3].get_line(), width, horz_even);
            bands[2].push_line();
            bands[3].push_line();
          }
          if (cur_line >= 4)
          {
            irrev_vert_wvlt_K(lines + 4, lines + 5,
                              true, width);
            irrev_horz_wvlt_fwd_tx(lines + 5, child_res->get_line(),
              bands[1].get_line(), width, horz_even);
            bands[1].push_line();
            child_res->push_line();
          }
        }

        if (cur_line >= res_rect.siz.h - 1)
        { //finished, so we need to process any left line
          if (cur_line)
          {
            if (vert_even)
            {
              irrev_vert_wvlt_step(lines + 1, lines + 1,
                                   lines, 1, width);
              irrev_vert_wvlt_step(lines,
                                   cur_line > 1 ? lines + 2 : lines,
                                   lines + 1, 2, width);
              irrev_vert_wvlt_step(lines + 1,
                                   cur_line > 2 ? lines + 3 : lines + 1,
                                   lines + 2, 3, width);
              irrev_vert_wvlt_step(lines + 1, lines + 1,
                                   lines, 3, width);
              //push lines[2] to L, lines[1] to H, and lines[0] to L
              if (cur_line >= 2)
              {
                irrev_vert_wvlt_K(lines + 2, lines + 5,
                                  true, width);
                irrev_horz_wvlt_fwd_tx(lines + 5,
                  child_res->get_line(), bands[1].get_line(),
                  width, horz_even);
                bands[1].push_line();
                child_res->push_line();
              }
              irrev_vert_wvlt_K(lines + 1, lines + 5,
                                false, width);
              irrev_horz_wvlt_fwd_tx(lines + 5, bands[2].get_line(),
                bands[3].get_line(), width, horz_even);
              bands[2].push_line();
              bands[3].push_line();
              irrev_vert_wvlt_K(lines, lines + 5,
                                true, width);
              irrev_horz_wvlt_fwd_tx(lines + 5, child_res->get_line(),
                bands[1].get_line(), width, horz_even);
              bands[1].push_line();
              child_res->push_line();
            }
            else
            {
              irrev_vert_wvlt_step(lines + 1, lines + 1,
                                   lines, 0, width);
              irrev_vert_wvlt_step(lines,
                                   cur_line > 1 ? lines + 2 : lines,
                                   lines + 1, 1, width);
              irrev_vert_wvlt_step(lines + 1,
                                   cur_line > 2 ? lines + 3 : lines + 1,
                                   lines + 2, 2, width);
              irrev_vert_wvlt_step(lines + 2,
                                   cur_line > 3 ? lines + 4 : lines + 2,
                                   lines + 3, 3, width);

              irrev_vert_wvlt_step(lines + 1, lines + 1,
                                   lines, 2, width);
              irrev_vert_wvlt_step(lines,
                                   cur_line > 1 ? lines + 2 : lines,
                                   lines + 1, 3, width);

              //push lines[3] L, lines[2] H, lines[1] L, and lines[0] H
              if (cur_line >= 3)
              {
                irrev_vert_wvlt_K(lines + 3, lines + 5,
                                  true, width);
                irrev_horz_wvlt_fwd_tx(lines + 5,
                  child_res->get_line(), bands[1].get_line(),
                  width, horz_even);
                bands[1].push_line();
                child_res->push_line();
              }
              if (cur_line >= 2)
                irrev_vert_wvlt_K(lines + 2, lines + 5, false, width);
              else
                irrev_vert_wvlt_K(lines, lines + 5, false, width);
              irrev_horz_wvlt_fwd_tx(lines + 5, bands[2].get_line(),
                bands[3].get_line(), width, horz_even);
              bands[2].push_line();
              bands[3].push_line();
              irrev_vert_wvlt_K(lines + 1, lines + 5,
                                true, width);
              irrev_horz_wvlt_fwd_tx(lines + 5, child_res->get_line(),
                bands[1].get_line(), width, horz_even);
              bands[1].push_line();
              child_res->push_line();
              irrev_vert_wvlt_K(lines, lines + 5,
                                false, width);
              irrev_horz_wvlt_fwd_tx(lines + 5, bands[2].get_line(),
                bands[3].get_line(), width, horz_even);
              bands[2].push_line();
              bands[3].push_line();
            }
          }
          else
          { //only one line
            if (vert_even)
            {
              //push to L
              irrev_horz_wvlt_fwd_tx(lines, child_res->get_line(),
                bands[1].get_line(), width, horz_even);
              bands[1].push_line();
              child_res->push_line();
            }
            else
            {
              float* sp = lines[0].f32;
              for (ui32 i = width; i > 0; --i)
                *sp++ *= 2.0f;
              //push to H
              irrev_horz_wvlt_fwd_tx(lines, bands[2].get_line(),
                bands[3].get_line(), width, horz_even);
              bands[2].push_line();
              bands[3].push_line();
            }
          }
        }

        rotate_buffers(lines, lines + 1, lines + 2, lines + 3, lines + 4, 
                       lines + 5);

        ++cur_line;
        vert_even = !vert_even;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    line_buf* resolution::pull_line()
    {
      if (res_num == 0)
      {
        assert(num_bands == 1 && child_res == NULL);
        return bands[0].pull_line();
      }

      if (skipped_res_for_recon == true)
        return child_res->pull_line();

      ui32 width = res_rect.siz.w;
      if (width == 0)
        return lines;
      if (reversible)
      {
        assert(num_lines >= 4);
        if (res_rect.siz.h > 1)
        {
          do
          {
            //horizontal transform
            if (cur_line < res_rect.siz.h)
            {
              if (vert_even)
                rev_horz_wvlt_bwd_tx(lines,
                  child_res->pull_line(), bands[1].pull_line(),
                  width, horz_even);
              else
                rev_horz_wvlt_bwd_tx(lines,
                  bands[2].pull_line(), bands[3].pull_line(),
                  width, horz_even);
            }

            //vertical transform
            if (!vert_even)
            {
              rev_vert_wvlt_bwd_update(
                cur_line > 1 ? lines + 2 : lines,
                cur_line < res_rect.siz.h ? lines : lines + 2,
                lines + 1, width);
              rev_vert_wvlt_bwd_predict(
                cur_line > 2 ? lines + 3 : lines + 1,
                cur_line < res_rect.siz.h + 1 ? lines + 1 : lines + 3,
                lines + 2, width);
            }

            vert_even = !vert_even;
            rotate_buffers(lines, lines + 1, lines + 2, lines + 3);
            ++cur_line;
          } while (cur_line < 3);
          memcpy(lines[0].i32, lines[3].i32, res_rect.siz.w * sizeof(si32));
          return lines;
        }
        else if (res_rect.siz.h == 1)
        {
          if (vert_even)
          {
            rev_horz_wvlt_bwd_tx(lines, child_res->pull_line(),
              bands[1].pull_line(), width, horz_even);
          }
          else
          {
            rev_horz_wvlt_bwd_tx(lines, bands[2].pull_line(),
              bands[3].pull_line(), width, horz_even);
            if (width)
            {
              si32* sp = lines[0].i32;
              for (ui32 i = width; i > 0; --i)
                *sp++ >>= 1;
            }
          }
          return lines;
        }
        else
          return lines;
      }
      else
      {
        assert(num_lines >= 6);
        if (res_rect.siz.h > 1)
        {
          do
          {
            //horizontal transform
            if (cur_line < res_rect.siz.h)
            {
              if (vert_even)
              {
                irrev_horz_wvlt_bwd_tx(lines,
                  child_res->pull_line(), bands[1].pull_line(),
                  width, horz_even);
                irrev_vert_wvlt_K(lines, lines, false, width);
              }
              else
              {
                irrev_horz_wvlt_bwd_tx(lines,
                  bands[2].pull_line(), bands[3].pull_line(),
                  width, horz_even);
                irrev_vert_wvlt_K(lines, lines, true, width);
              }
            }

            //vertical transform
            if (!vert_even)
            {
              irrev_vert_wvlt_step(
                cur_line > 1 ? lines + 2 : lines,
                cur_line < res_rect.siz.h ? lines : lines + 2,
                lines + 1, 7, width);
              irrev_vert_wvlt_step(
                cur_line > 2 ? lines + 3 : lines + 1,
                cur_line < res_rect.siz.h + 1 ? lines + 1 : lines + 3,
                lines + 2, 6, width);
              irrev_vert_wvlt_step(
                cur_line > 3 ? lines + 4 : lines + 2,
                cur_line < res_rect.siz.h + 2 ? lines + 2 : lines + 4,
                lines + 3, 5, width);
              irrev_vert_wvlt_step(
                cur_line > 4 ? lines + 5 : lines + 3,
                cur_line < res_rect.siz.h + 3 ? lines + 3 : lines + 5,
                lines + 4, 4, width);
            }

            vert_even = !vert_even;
            rotate_buffers(lines, lines + 1, lines + 2, lines + 3, lines + 4, 
                           lines + 5);
            ++cur_line;
          } while (cur_line < 5);
          memcpy(lines[0].f32, lines[5].f32, res_rect.siz.w * sizeof(float));
          return lines;
        }
        else if (res_rect.siz.h == 1)
        {
          if (vert_even)
          {
            irrev_horz_wvlt_bwd_tx(lines, child_res->pull_line(),
              bands[1].pull_line(), width, horz_even);
          }
          else
          {
            irrev_horz_wvlt_bwd_tx(lines, bands[2].pull_line(),
              bands[3].pull_line(), width, horz_even);
            if (width)
            {
              float* sp = lines[0].f32;
              for (ui32 i = width; i > 0; --i)
                *sp++ *= 0.5f;
            }
          }
          return lines;
        }
        else
          return lines;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 resolution::prepare_precinct()
    {
      ui32 lower_resolutions_bytes = 0;
      if (res_num != 0)
        lower_resolutions_bytes = child_res->prepare_precinct();

      this->num_bytes = 0;
      si32 repeat = (si32)num_precincts.area();
      for (si32 i = 0; i < repeat; ++i)
        this->num_bytes += precincts[i].prepare_precinct(tag_tree_size,
          level_index, elastic);
      return this->num_bytes + lower_resolutions_bytes;
    }

    //////////////////////////////////////////////////////////////////////////
    void resolution::write_precincts(outfile_base* file)
    {
      precinct* p = precincts;
      for (si32 i = 0; i < (si32)num_precincts.area(); ++i)
        p[i].write(file);
    }

    //////////////////////////////////////////////////////////////////////////
    bool resolution::get_top_left_precinct(point& top_left)
    {
      ui32 idx = cur_precinct_loc.x + cur_precinct_loc.y * num_precincts.w;
      if (idx < num_precincts.area())
      {
        top_left = precincts[idx].img_point;
        return true;
      }
      return false;
    }

    //////////////////////////////////////////////////////////////////////////
    void resolution::write_one_precinct(outfile_base* file)
    {
      ui32 idx = cur_precinct_loc.x + cur_precinct_loc.y * num_precincts.w;
      assert(idx < num_precincts.area());
      precincts[idx].write(file);

      if (++cur_precinct_loc.x >= num_precincts.w)
      {
        cur_precinct_loc.x = 0;
        ++cur_precinct_loc.y;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void resolution::parse_all_precincts(ui32& data_left, infile_base* file)
    {
      precinct* p = precincts;
      ui32 idx = cur_precinct_loc.x + cur_precinct_loc.y * num_precincts.w;
      for (ui32 i = idx; i < num_precincts.area(); ++i)
      {
        if (data_left == 0)
          break;
        p[i].parse(tag_tree_size, level_index, elastic, data_left, file,
          skipped_res_for_read);
        if (++cur_precinct_loc.x >= num_precincts.w)
        {
          cur_precinct_loc.x = 0;
          ++cur_precinct_loc.y;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void resolution::parse_one_precinct(ui32& data_left, infile_base* file)
    {
      ui32 idx = cur_precinct_loc.x + cur_precinct_loc.y * num_precincts.w;
      assert(idx < num_precincts.area());

      if (data_left == 0)
        return;
      precinct* p = precincts + idx;
      p->parse(tag_tree_size, level_index, elastic, data_left, file,
        skipped_res_for_read);
      if (++cur_precinct_loc.x >= num_precincts.w)
      {
        cur_precinct_loc.x = 0;
        ++cur_precinct_loc.y;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 resolution::get_num_bytes(ui32 resolution_num) const
    {
      if (this->res_num == resolution_num)
        return get_num_bytes();
      else {
        if (child_res)
          return child_res->get_num_bytes(resolution_num);
        else
          return 0;
      }

    }
  }
}