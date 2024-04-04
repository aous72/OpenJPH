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
#include <new>

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
    void resolution::pre_alloc(codestream* codestream, const rect& res_rect,
                               const rect& recon_res_rect, 
                               ui32 comp_num, ui32 res_num)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();
      const param_cod* cdp = codestream->get_cod(comp_num);
      ui32 t = cdp->get_num_decompositions()
             - codestream->get_skipped_res_for_recon();
      bool skipped_res_for_recon = res_num > t;

      const param_atk* atk = cdp->access_atk();
      param_dfs::dfs_dwt_type downsampling_style = param_dfs::BIDIR_DWT;
      if (cdp->is_dfs_defined()) {
        const param_dfs* dfs = codestream->access_dfs();
        if (dfs == NULL) {
          OJPH_ERROR(0x00070001, "There is a problem with codestream "
            "marker segments. COD/COC specifies the use of a DFS marker "
            "but there are no DFS markers within the main codestream "
            "headers");
        }
        else {
          ui16 dfs_idx = cdp->get_dfs_index();
          dfs = dfs->get_dfs(dfs_idx);
          if (dfs == NULL) {
            OJPH_ERROR(0x00070002, "There is a problem with codestream "
              "marker segments. COD/COC specifies the use of a DFS marker "
              "with index %d, but there are no such marker within the "
              "main codestream headers", dfs_idx);
          }
          ui32 num_decomps = cdp->get_num_decompositions();
          downsampling_style = dfs->get_dwt_type(num_decomps - res_num + 1);
        }
      }

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
          skipped_res_for_recon ? recon_res_rect : next_res_rect, 
          comp_num, res_num - 1);
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
          subband::pre_alloc(codestream, band_rect, comp_num, res_num);
        }
      }
      else
        subband::pre_alloc(codestream, res_rect, comp_num, res_num);

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
        ui32 num_steps = atk->get_num_steps();
        allocator->pre_alloc_obj<line_buf>(num_steps + 2);
        allocator->pre_alloc_obj<lifting_buf>(num_steps + 2);

        ui32 width = res_rect.siz.w + 1;
        for (ui32 i = 0; i < num_steps; ++i)
          allocator->pre_alloc_data<si32>(width, 1);
        allocator->pre_alloc_data<si32>(width, 1);
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
      const param_cod* cdp = codestream->get_cod(comp_num);
      ui32 t, num_decomps = cdp->get_num_decompositions();
      t = num_decomps - codestream->get_skipped_res_for_recon();
      skipped_res_for_recon = res_num > t;
      t = num_decomps - codestream->get_skipped_res_for_read();
      skipped_res_for_read = res_num > t;

      this->comp_downsamp = comp_downsamp;
      this->parent_comp = parent_tile_comp;
      this->parent_res = parent_res;
      this->res_rect = res_rect;
      this->comp_num = comp_num;
      this->res_num = res_num;
      this->num_bytes = 0;
      this->atk = cdp->access_atk();
      this->downsampling_style = param_dfs::BIDIR_DWT;
      if (cdp->is_dfs_defined()) {
        const param_dfs* dfs = codestream->access_dfs();
        if (dfs == NULL) {
          OJPH_ERROR(0x00070011, "There is a problem with codestream "
              "marker segments. COD/COC specifies the use of a DFS marker "
              "but there are no DFS markers within the main codestream "
            "headers");
        }
        else {
          ui16 dfs_idx = cdp->get_dfs_index();
          dfs = dfs->get_dfs(dfs_idx);
          if (dfs == NULL) {
            OJPH_ERROR(0x00070012, "There is a problem with codestream "
              "marker segments. COD/COC specifies the use of a DFS marker "
              "with index %d, but there are no such marker within the "
              "main codestream headers", dfs_idx);
          }
          ui32 num_decomps = cdp->get_num_decompositions();
          this->downsampling_style = 
            dfs->get_dwt_type(num_decomps - res_num + 1);
        }
      }

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
        this->atk = cdp->access_atk();
        this->reversible = atk->is_reversible();
        this->num_steps = atk->get_num_steps();
        // create line buffers and lifting_bufs
        lines = allocator->post_alloc_obj<line_buf>(num_steps + 2);
        ssp = allocator->post_alloc_obj<lifting_buf>(num_steps + 2);
        sig = ssp + num_steps;
        aug = ssp + num_steps + 1;

        // initiate lifting_bufs
        for (ui32 i = 0; i < num_steps; ++i) {
          new (ssp + i) lifting_buf;
          ssp[i].line = lines + i;
        };
        new (sig) lifting_buf;
        sig->line = lines + num_steps;
        new (aug) lifting_buf;
        aug->line = lines + num_steps + 1;

        // initiate storage of line_buf
        ui32 width = res_rect.siz.w + 1;
        for (ui32 i = 0; i < num_steps; ++i)
          ssp[i].line->wrap(
            allocator->post_alloc_data<si32>(width, 1), width, 1);
        sig->line->wrap(allocator->post_alloc_data<si32>(width, 1), width, 1);
        aug->line->wrap(allocator->post_alloc_data<si32>(width, 1), width, 1);

        cur_line = 0;
        rows_to_produce = res_rect.siz.h;
        vert_even = (res_rect.org.y & 1) == 0;
        horz_even = (res_rect.org.x & 1) == 0;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    line_buf* resolution::get_line()
    { 
      if (vert_even)
      {
        ++cur_line;
        sig->active = true;
        return sig->line;
      }
      else
      {
        ++cur_line;
        aug->active = true;
        return aug->line;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void resolution::push_line()
    {
      if (res_num == 0)
      {
        assert(num_bands == 1 && child_res == NULL);
        bands[0].exchange_buf(vert_even ? sig->line : aug->line);
        bands[0].push_line();
        return;
      }

      ui32 width = res_rect.siz.w;
      if (width == 0)
        return;
      if (reversible)
      {
        if (res_rect.siz.h > 1)
        {
          if (!vert_even && cur_line < res_rect.siz.h) {
            vert_even = !vert_even;
            return;
          }

          do
          {
            //vertical transform
            for (ui32 i = 0; i < num_steps; ++i)
            {
              if (aug->active && (sig->active || ssp[i].active))
              {
                line_buf* dp = aug->line;
                line_buf* sp1 = sig->active ? sig->line : ssp[i].line;
                line_buf* sp2 = ssp[i].active ? ssp[i].line : sig->line;
                const lifting_step* s = atk->get_step(i);
                rev_vert_ana_step(s, sp1, sp2, dp, width);
              }
              lifting_buf t = *aug; *aug = ssp[i]; ssp[i] = *sig; *sig = t;
            }

            if (aug->active) {
              rev_horz_ana(atk, bands[2].get_line(),
                bands[3].get_line(), aug->line, width, horz_even);
              bands[2].push_line();
              bands[3].push_line();
              aug->active = false;
              --rows_to_produce;
            }
            if (sig->active) {
              rev_horz_ana(atk, child_res->get_line(),
                bands[1].get_line(), sig->line, width, horz_even);
              bands[1].push_line();
              child_res->push_line();
              sig->active = false;
              --rows_to_produce;
            };
            vert_even = !vert_even;
          } while (cur_line >= res_rect.siz.h && rows_to_produce > 0);
        }
        else
        {
          if (vert_even) {
            // horizontal transform
            rev_horz_ana(atk, child_res->get_line(),
              bands[1].get_line(), sig->line, width, horz_even);
            bands[1].push_line();
            child_res->push_line();
          }
          else
          {
            // vertical transform
            si32* sp = aug->line->i32;
            for (ui32 i = width; i > 0; --i)
              *sp++ <<= 1;
            // horizontal transform
            rev_horz_ana(atk, bands[2].get_line(),
              bands[3].get_line(), aug->line, width, horz_even);
            bands[2].push_line();
            bands[3].push_line();
          }
        }
      }
      else
      {
        if (res_rect.siz.h > 1)
        {
          if (!vert_even && cur_line < res_rect.siz.h) {
            vert_even = !vert_even;
            return;
          }

          do
          {
            //vertical transform
            for (ui32 i = 0; i < num_steps; ++i)
            {
              if (aug->active && (sig->active || ssp[i].active))
              {
                line_buf* dp = aug->line;
                line_buf* sp1 = sig->active ? sig->line : ssp[i].line;
                line_buf* sp2 = ssp[i].active ? ssp[i].line : sig->line;
                const lifting_step* s = atk->get_step(i);
                irv_vert_ana_step(s, sp1, sp2, dp, width);
              }
              lifting_buf t = *aug; *aug = ssp[i]; ssp[i] = *sig; *sig = t;
            }

            if (aug->active) {
              const float K = atk->get_K();
              irv_vert_times_K(K, aug->line, width);

              irv_horz_ana(atk, bands[2].get_line(),
                bands[3].get_line(), aug->line, width, horz_even);
              bands[2].push_line();
              bands[3].push_line();
              aug->active = false;
              --rows_to_produce;
            }
            if (sig->active) {
              const float K_inv = 1.0f / atk->get_K();
              irv_vert_times_K(K_inv, sig->line, width);

              irv_horz_ana(atk, child_res->get_line(),
                bands[1].get_line(), sig->line, width, horz_even);
              bands[1].push_line();
              child_res->push_line();
              sig->active = false;
              --rows_to_produce;
            };
            vert_even = !vert_even;
          } while (cur_line >= res_rect.siz.h && rows_to_produce > 0);
        }
        else
        {
          if (vert_even) {
            // horizontal transform
            irv_horz_ana(atk, child_res->get_line(),
              bands[1].get_line(), sig->line, width, horz_even);
            bands[1].push_line();
            child_res->push_line();
          }
          else
          {
            // vertical transform
            float* sp = aug->line->f32;
            for (ui32 i = width; i > 0; --i)
              *sp++ *= 2.0f;
            // horizontal transform
            irv_horz_ana(atk, bands[2].get_line(),
              bands[3].get_line(), aug->line, width, horz_even);
            bands[2].push_line();
            bands[3].push_line();
          }
        }
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
        return NULL;
      if (reversible)
      {
        if (res_rect.siz.h > 1)
        {
          if (sig->active) {
            sig->active = false;
            return sig->line;
          };
          for (;;)
          {
            //horizontal transform
            if (cur_line < res_rect.siz.h)
            {
              if (vert_even) { // even
                rev_horz_syn(atk, aug->line,
                  child_res->pull_line(), bands[1].pull_line(),
                  width, horz_even);
                aug->active = true;
                vert_even = !vert_even;
                ++cur_line;
                continue;
              }
              else {
                rev_horz_syn(atk, sig->line,
                  bands[2].pull_line(), bands[3].pull_line(),
                  width, horz_even);
                sig->active = true;
                vert_even = !vert_even;
                ++cur_line;
              }
            }

            //vertical transform
            for (ui32 i = 0; i < num_steps; ++i)
            {
              if (aug->active && (sig->active || ssp[i].active))
              {
                line_buf* dp = aug->line;
                line_buf* sp1 = sig->active ? sig->line : ssp[i].line;
                line_buf* sp2 = ssp[i].active ? ssp[i].line : sig->line;
                const lifting_step* s = atk->get_step(num_steps - i - 1);
                rev_vert_syn_step(s, dp, sp1, sp2, width);
              }
              lifting_buf t = *aug; *aug = ssp[i]; ssp[i] = *sig; *sig = t;
            }

            if (aug->active) {
              aug->active = false;
              return aug->line;
            }
            if (sig->active) {
              sig->active = false;
              return sig->line;
            };
          }
        }
        else
        {
          if (vert_even)
            rev_horz_syn(atk, aug->line, child_res->pull_line(),
              bands[1].pull_line(), width, horz_even);
          else
          {
            rev_horz_syn(atk, aug->line, bands[2].pull_line(),
              bands[3].pull_line(), width, horz_even);
            si32* sp = aug->line->i32;
            for (ui32 i = width; i > 0; --i)
              *sp++ >>= 1;
          }
          return aug->line;
        }
      }
      else
      {
        if (res_rect.siz.h > 1)
        {
          if (sig->active) {
            sig->active = false;
            return sig->line;
          };
          for (;;)
          {
            //horizontal transform
            if (cur_line < res_rect.siz.h)
            {
              if (vert_even) { // even
                irv_horz_syn(atk, aug->line,
                  child_res->pull_line(), bands[1].pull_line(),
                  width, horz_even);
                aug->active = true;
                vert_even = !vert_even;
                ++cur_line;

                const float K = atk->get_K();
                irv_vert_times_K(K, aug->line, width);

                continue;
              }
              else {
                irv_horz_syn(atk, sig->line,
                  bands[2].pull_line(), bands[3].pull_line(),
                  width, horz_even);
                sig->active = true;
                vert_even = !vert_even;
                ++cur_line;

                const float K_inv = 1.0f / atk->get_K();
                irv_vert_times_K(K_inv, sig->line, width);
              }
            }

            //vertical transform
            for (ui32 i = 0; i < num_steps; ++i)
            {
              if (aug->active && (sig->active || ssp[i].active))
              {
                line_buf* dp = aug->line;
                line_buf* sp1 = sig->active ? sig->line : ssp[i].line;
                line_buf* sp2 = ssp[i].active ? ssp[i].line : sig->line;
                const lifting_step* s = atk->get_step(num_steps - i - 1);
                irv_vert_syn_step(s, dp, sp1, sp2, width);
              }
              lifting_buf t = *aug; *aug = ssp[i]; ssp[i] = *sig; *sig = t;
            }

            if (aug->active) {
              aug->active = false;
              return aug->line;
            }
            if (sig->active) {
              sig->active = false;
              return sig->line;
            };
          }
        }
        else
        {
          if (vert_even)
            irv_horz_syn(atk, aug->line, child_res->pull_line(),
              bands[1].pull_line(), width, horz_even);
          else
          {
            irv_horz_syn(atk, aug->line, bands[2].pull_line(),
              bands[3].pull_line(), width, horz_even);
            float *sp = aug->line->f32;
            for (ui32 i = width; i > 0; --i)
              *sp++ *= 0.5f;
          }
          return aug->line;
        }
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