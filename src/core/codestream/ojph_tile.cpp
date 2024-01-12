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
// File: ojph_tile.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#include <climits>
#include <cmath>

#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream_local.h"
#include "ojph_tile.h"
#include "ojph_tile_comp.h"

#include "../transform/ojph_colour.h"

namespace ojph {

  namespace local
  {

    //////////////////////////////////////////////////////////////////////////
    void tile::pre_alloc(codestream *codestream, const rect& tile_rect,
                         const rect& recon_tile_rect, ui32& num_tileparts)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();

      //allocate tiles_comp
      const param_siz *szp = codestream->get_siz();
      ui32 num_comps = szp->get_num_components();
      allocator->pre_alloc_obj<tile_comp>(num_comps);
      allocator->pre_alloc_obj<rect>(num_comps); //for comp_rects
      allocator->pre_alloc_obj<rect>(num_comps); //for recon_comp_rects
      allocator->pre_alloc_obj<ui32>(num_comps); //for line_offsets
      allocator->pre_alloc_obj<ui32>(num_comps); //for num_bits
      allocator->pre_alloc_obj<bool>(num_comps); //for is_signed
      allocator->pre_alloc_obj<ui32>(num_comps); //for cur_line

      ui32 tilepart_div = codestream->get_tilepart_div();
      num_tileparts = 1; //for num_rc_bytes
      // this code is not ideal, since the number of decompositions can be 
      // different for different components
      if (tilepart_div & OJPH_TILEPART_COMPONENTS)
        num_tileparts *= num_comps;
      if (tilepart_div & OJPH_TILEPART_RESOLUTIONS)
        num_tileparts *= codestream->get_cod()->get_num_decompositions() + 1;
      if (num_tileparts > 255)
        OJPH_ERROR(0x000300D1, "Trying to create %d tileparts; a tile cannot "
          "have more than 255 tile parts.", num_tileparts);

      ui32 tx0 = tile_rect.org.x;
      ui32 ty0 = tile_rect.org.y;
      ui32 tx1 = tile_rect.org.x + tile_rect.siz.w;
      ui32 ty1 = tile_rect.org.y + tile_rect.siz.h;
      ui32 recon_tx0 = recon_tile_rect.org.x;
      ui32 recon_ty0 = recon_tile_rect.org.y;
      ui32 recon_tx1 = recon_tile_rect.org.x + recon_tile_rect.siz.w;
      ui32 recon_ty1 = recon_tile_rect.org.y + recon_tile_rect.siz.h;

      ui32 width = 0;
      for (ui32 i = 0; i < num_comps; ++i)
      {
        point downsamp = szp->get_downsampling(i);

        ui32 tcx0 = ojph_div_ceil(tx0, downsamp.x);
        ui32 tcy0 = ojph_div_ceil(ty0, downsamp.y);
        ui32 tcx1 = ojph_div_ceil(tx1, downsamp.x);
        ui32 tcy1 = ojph_div_ceil(ty1, downsamp.y);
        ui32 recon_tcx0 = ojph_div_ceil(recon_tx0, downsamp.x);
        ui32 recon_tcy0 = ojph_div_ceil(recon_ty0, downsamp.y);
        ui32 recon_tcx1 = ojph_div_ceil(recon_tx1, downsamp.x);
        ui32 recon_tcy1 = ojph_div_ceil(recon_ty1, downsamp.y);

        rect comp_rect;
        comp_rect.org.x = tcx0;
        comp_rect.org.y = tcy0;
        comp_rect.siz.w = tcx1 - tcx0;
        comp_rect.siz.h = tcy1 - tcy0;

        rect recon_comp_rect;
        recon_comp_rect.org.x = recon_tcx0;
        recon_comp_rect.org.y = recon_tcy0;
        recon_comp_rect.siz.w = recon_tcx1 - recon_tcx0;
        recon_comp_rect.siz.h = recon_tcy1 - recon_tcy0;

        tile_comp::pre_alloc(codestream, comp_rect, recon_comp_rect);
        width = ojph_max(width, recon_comp_rect.siz.w);
      }

      //allocate lines
      if (codestream->get_cod()->is_employing_color_transform())
      {
        allocator->pre_alloc_obj<line_buf>(3);
        for (int i = 0; i < 3; ++i)
          allocator->pre_alloc_data<si32>(width, 0);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void tile::finalize_alloc(codestream *codestream, const rect& tile_rect,
                              const rect& recon_tile_rect, ui32 tile_idx, 
                              ui32 offset, ui32 &num_tileparts)
    {
      //this->parent = codestream;
      mem_fixed_allocator* allocator = codestream->get_allocator();

      sot.init(0, (ui16)tile_idx, 0, 1);
      prog_order = codestream->access_cod().get_progression_order();

      //allocate tiles_comp
      const param_siz *szp = codestream->get_siz();

      this->num_bytes = 0;
      num_comps = szp->get_num_components();
      skipped_res_for_read = codestream->get_skipped_res_for_read();
      comps = allocator->post_alloc_obj<tile_comp>(num_comps);
      comp_rects = allocator->post_alloc_obj<rect>(num_comps);
      recon_comp_rects = allocator->post_alloc_obj<rect>(num_comps);
      line_offsets = allocator->post_alloc_obj<ui32>(num_comps);
      num_bits = allocator->post_alloc_obj<ui32>(num_comps);
      is_signed = allocator->post_alloc_obj<bool>(num_comps);
      cur_line = allocator->post_alloc_obj<ui32>(num_comps);

      profile = codestream->get_profile();
      tilepart_div = codestream->get_tilepart_div();
      need_tlm = codestream->is_tlm_needed();
      num_tileparts = 1;
      // this code is not ideal, since the number of decompositions can be 
      // different for different components
      if (tilepart_div & OJPH_TILEPART_COMPONENTS)
        num_tileparts *= num_comps;
      if (tilepart_div & OJPH_TILEPART_RESOLUTIONS)
        num_tileparts *= codestream->get_cod()->get_num_decompositions() + 1;

      this->resilient = codestream->is_resilient();
      this->tile_rect = tile_rect;
      this->recon_tile_rect = recon_tile_rect;

      ui32 tx0 = tile_rect.org.x;
      ui32 ty0 = tile_rect.org.y;
      ui32 tx1 = tile_rect.org.x + tile_rect.siz.w;
      ui32 ty1 = tile_rect.org.y + tile_rect.siz.h;
      ui32 recon_tx0 = recon_tile_rect.org.x;
      ui32 recon_ty0 = recon_tile_rect.org.y;
      ui32 recon_tx1 = recon_tile_rect.org.x + recon_tile_rect.siz.w;
      ui32 recon_ty1 = recon_tile_rect.org.y + recon_tile_rect.siz.h;

      ui32 width = 0;
      for (ui32 i = 0; i < num_comps; ++i)
      {
        point downsamp = szp->get_downsampling(i);

        ui32 tcx0 = ojph_div_ceil(tx0, downsamp.x);
        ui32 tcy0 = ojph_div_ceil(ty0, downsamp.y);
        ui32 tcx1 = ojph_div_ceil(tx1, downsamp.x);
        ui32 tcy1 = ojph_div_ceil(ty1, downsamp.y);
        ui32 recon_tcx0 = ojph_div_ceil(recon_tx0, downsamp.x);
        ui32 recon_tcy0 = ojph_div_ceil(recon_ty0, downsamp.y);
        ui32 recon_tcx1 = ojph_div_ceil(recon_tx1, downsamp.x);
        ui32 recon_tcy1 = ojph_div_ceil(recon_ty1, downsamp.y);

        line_offsets[i] = 
          recon_tcx0 - ojph_div_ceil(recon_tx0 - offset, downsamp.x);
        comp_rects[i].org.x = tcx0;
        comp_rects[i].org.y = tcy0;
        comp_rects[i].siz.w = tcx1 - tcx0;
        comp_rects[i].siz.h = tcy1 - tcy0;
        recon_comp_rects[i].org.x = recon_tcx0;
        recon_comp_rects[i].org.y = recon_tcy0;
        recon_comp_rects[i].siz.w = recon_tcx1 - recon_tcx0;
        recon_comp_rects[i].siz.h = recon_tcy1 - recon_tcy0;

        comps[i].finalize_alloc(codestream, this, i, comp_rects[i], 
          recon_comp_rects[i]);
        width = ojph_max(width, recon_comp_rects[i].siz.w);

        num_bits[i] = szp->get_bit_depth(i);
        is_signed[i] = szp->is_signed(i);
        cur_line[i] = 0;
      }

      //allocate lines
      const param_cod* cdp = codestream->get_cod();
      this->reversible = cdp->is_reversible();
      this->employ_color_transform = cdp->is_employing_color_transform();
      if (this->employ_color_transform)
      {
        num_lines = 3;
        lines = allocator->post_alloc_obj<line_buf>(num_lines);
        for (int i = 0; i < 3; ++i)
          lines[i].wrap(
            allocator->post_alloc_data<si32>(width,0),width,0);
      }
      else
      {
        lines = NULL;
        num_lines = 0;
      }
      next_tile_part = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    bool tile::push(line_buf *line, ui32 comp_num)
    {
      assert(comp_num < num_comps);
      if (cur_line[comp_num] >= comp_rects[comp_num].siz.h)
        return false;
      cur_line[comp_num]++;

      //converts to signed representation
      //employs color transform if there is a need
      if (!employ_color_transform || comp_num >= 3)
      {
        assert(comp_num < num_comps);
        ui32 comp_width = comp_rects[comp_num].siz.w;
        line_buf *tc = comps[comp_num].get_line();
        if (reversible)
        {
          int shift = 1 << (num_bits[comp_num] - 1);
          const si32 *sp = line->i32 + line_offsets[comp_num];
          si32* dp = tc->i32;
          if (is_signed[comp_num])
            memcpy(dp, sp, comp_width * sizeof(si32));
          else
            cnvrt_si32_to_si32_shftd(sp, dp, -shift, comp_width);
        }
        else
        {
          float mul = 1.0f / (float)(1<<num_bits[comp_num]);
          const si32 *sp = line->i32 + line_offsets[comp_num];
          float *dp = tc->f32;
          if (is_signed[comp_num])
            cnvrt_si32_to_float(sp, dp, mul, comp_width);
          else
            cnvrt_si32_to_float_shftd(sp, dp, mul, comp_width);
        }
        comps[comp_num].push_line();
      }
      else
      {
        ui32 comp_width = comp_rects[comp_num].siz.w;
        if (reversible)
        {
          int shift = 1 << (num_bits[comp_num] - 1);
          const si32 *sp = line->i32 + line_offsets[comp_num];
          si32 *dp = lines[comp_num].i32;
          if (is_signed[comp_num])
            memcpy(dp, sp, comp_width * sizeof(si32));
          else
            cnvrt_si32_to_si32_shftd(sp, dp, -shift, comp_width);
          if (comp_num == 2)
          { // reversible color transform
            rct_forward(lines[0].i32, lines[1].i32, lines[2].i32,
                        comps[0].get_line()->i32,
                        comps[1].get_line()->i32,
                        comps[2].get_line()->i32, comp_width);
                        comps[0].push_line();
                        comps[1].push_line();
                        comps[2].push_line();
          }
        }
        else
        {
          float mul = 1.0f / (float)(1<<num_bits[comp_num]);
          const si32 *sp = line->i32 + line_offsets[comp_num];
          float *dp = lines[comp_num].f32;
          if (is_signed[comp_num])
            cnvrt_si32_to_float(sp, dp, mul, comp_width);
          else
            cnvrt_si32_to_float_shftd(sp, dp, mul, comp_width);
          if (comp_num == 2)
          { // irreversible color transform
            ict_forward(lines[0].f32, lines[1].f32, lines[2].f32,
                        comps[0].get_line()->f32,
                        comps[1].get_line()->f32,
                        comps[2].get_line()->f32, comp_width);
                        comps[0].push_line();
                        comps[1].push_line();
                        comps[2].push_line();
          }
        }
      }

      return true;
    }

    //////////////////////////////////////////////////////////////////////////
    bool tile::pull(line_buf* tgt_line, ui32 comp_num)
    {
      assert(comp_num < num_comps);
      if (cur_line[comp_num] >= recon_comp_rects[comp_num].siz.h)
        return false;

      cur_line[comp_num]++;

      if (!employ_color_transform || num_comps == 1)
      {
        line_buf *src_line = comps[comp_num].pull_line();
        ui32 comp_width = recon_comp_rects[comp_num].siz.w;
        if (reversible)
        {
          int shift = 1 << (num_bits[comp_num] - 1);
          const si32 *sp = src_line->i32;
          si32* dp = tgt_line->i32 + line_offsets[comp_num];
          if (is_signed[comp_num])
            memcpy(dp, sp, comp_width * sizeof(si32));
          else
            cnvrt_si32_to_si32_shftd(sp, dp, +shift, comp_width);
        }
        else
        {
          float mul = (float)(1 << num_bits[comp_num]);
          const float *sp = src_line->f32;
          si32 *dp = tgt_line->i32 + line_offsets[comp_num];
          if (is_signed[comp_num])
            cnvrt_float_to_si32(sp, dp, mul, comp_width);
          else
            cnvrt_float_to_si32_shftd(sp, dp, mul, comp_width);
        }
      }
      else
      {
        assert(num_comps >= 3);
        ui32 comp_width = recon_comp_rects[comp_num].siz.w;
        if (comp_num == 0)
        {
          if (reversible)
            rct_backward(comps[0].pull_line()->i32, comps[1].pull_line()->i32,
              comps[2].pull_line()->i32, lines[0].i32, lines[1].i32,
              lines[2].i32, comp_width);
          else
            ict_backward(comps[0].pull_line()->f32, comps[1].pull_line()->f32,
              comps[2].pull_line()->f32, lines[0].f32, lines[1].f32,
              lines[2].f32, comp_width);
        }
        if (reversible)
        {
          int shift = 1 << (num_bits[comp_num] - 1);
          const si32 *sp;
          if (comp_num < 3)
            sp = lines[comp_num].i32;
          else
            sp = comps[comp_num].pull_line()->i32;
          si32* dp = tgt_line->i32 + line_offsets[comp_num];
          if (is_signed[comp_num])
            memcpy(dp, sp, comp_width * sizeof(si32));
          else
            cnvrt_si32_to_si32_shftd(sp, dp, +shift, comp_width);
        }
        else
        {
          float mul = (float)(1 << num_bits[comp_num]);
          const float *sp;
          if (comp_num < 3)
            sp = lines[comp_num].f32;
          else
            sp = comps[comp_num].pull_line()->f32;
          si32 *dp = tgt_line->i32 + line_offsets[comp_num];
          if (is_signed[comp_num])
            cnvrt_float_to_si32(sp, dp, mul, comp_width);
          else
            cnvrt_float_to_si32_shftd(sp, dp, mul, comp_width);
        }
      }

      return true;
    }


    //////////////////////////////////////////////////////////////////////////
    void tile::prepare_for_flush()
    {
      this->num_bytes = 0;
      //prepare precinct headers
      for (ui32 c = 0; c < num_comps; ++c)
        num_bytes += comps[c].prepare_precincts();
    }

    //////////////////////////////////////////////////////////////////////////
    void tile::fill_tlm(param_tlm *tlm)
    {
      if (tilepart_div == OJPH_TILEPART_NO_DIVISIONS) {
        tlm->set_next_pair(sot.get_tile_index(), this->num_bytes);
      }
      else if (tilepart_div == OJPH_TILEPART_RESOLUTIONS)
      { 
        assert(prog_order != OJPH_PO_PCRL && prog_order != OJPH_PO_CPRL);
        ui32 max_decs = 0;
        for (ui32 c = 0; c < num_comps; ++c)
          max_decs = ojph_max(max_decs, comps[c].get_num_decompositions());
        for (ui32 r = 0; r <= max_decs; ++r) 
        {
          ui32 bytes = 0;
          for (ui32 c = 0; c < num_comps; ++c)
            bytes += comps[c].get_num_bytes(r);
          tlm->set_next_pair(sot.get_tile_index(), bytes);
        }
      }      
      else if (tilepart_div == OJPH_TILEPART_COMPONENTS)
      {
        if (prog_order == OJPH_PO_LRCP || prog_order == OJPH_PO_RLCP)
        { 
          ui32 max_decs = 0;
          for (ui32 c = 0; c < num_comps; ++c)
            max_decs = ojph_max(max_decs, comps[c].get_num_decompositions());
          for (ui32 r = 0; r <= max_decs; ++r) 
            for (ui32 c = 0; c < num_comps; ++c)
              if (r <= comps[c].get_num_decompositions())
                tlm->set_next_pair(sot.get_tile_index(), 
                                   comps[c].get_num_bytes(r));
        }
        else if (prog_order == OJPH_PO_CPRL)
          for (ui32 c = 0; c < num_comps; ++c)
            tlm->set_next_pair(sot.get_tile_index(), comps[c].get_num_bytes());
        else 
          assert(0); // should not be here
      }
      else 
      {
        assert(prog_order == OJPH_PO_LRCP || prog_order == OJPH_PO_RLCP);
        ui32 max_decs = 0;
        for (ui32 c = 0; c < num_comps; ++c)
          max_decs = ojph_max(max_decs, comps[c].get_num_decompositions());
        for (ui32 r = 0; r <= max_decs; ++r) 
          for (ui32 c = 0; c < num_comps; ++c)
            if (r <= comps[c].get_num_decompositions())
              tlm->set_next_pair(sot.get_tile_index(), 
                                 comps[c].get_num_bytes(r));
      }
    }


    //////////////////////////////////////////////////////////////////////////
    void tile::flush(outfile_base *file)
    {
      ui32 max_decompositions = 0;
      for (ui32 c = 0; c < num_comps; ++c)
        max_decompositions = ojph_max(max_decompositions,
          comps[c].get_num_decompositions());

      if (tilepart_div == OJPH_TILEPART_NO_DIVISIONS)
      {
        //write tile header
        if (!sot.write(file, this->num_bytes))
          OJPH_ERROR(0x00030081, "Error writing to file");

        //write start of data
        ui16 t = swap_byte(JP2K_MARKER::SOD);
        if (!file->write(&t, 2))
          OJPH_ERROR(0x00030082, "Error writing to file");
      }


      //sequence the writing of precincts according to progression order
      if (prog_order == OJPH_PO_LRCP || prog_order == OJPH_PO_RLCP)
      {
        if (tilepart_div == OJPH_TILEPART_NO_DIVISIONS)
        {
          for (ui32 r = 0; r <= max_decompositions; ++r)
            for (ui32 c = 0; c < num_comps; ++c)
              comps[c].write_precincts(r, file);
        }
        else if (tilepart_div == OJPH_TILEPART_RESOLUTIONS) 
        {
          for (ui32 r = 0; r <= max_decompositions; ++r) 
          {
            ui32 bytes = 0;
            for (ui32 c = 0; c < num_comps; ++c)
              bytes += comps[c].get_num_bytes(r);

            //write tile header
            if (!sot.write(file, bytes, (ui8)r, (ui8)(max_decompositions + 1)))
              OJPH_ERROR(0x00030083, "Error writing to file");

            //write start of data
            ui16 t = swap_byte(JP2K_MARKER::SOD);
            if (!file->write(&t, 2))
              OJPH_ERROR(0x00030084, "Error writing to file");
            
            //write precincts
            for (ui32 c = 0; c < num_comps; ++c)
              comps[c].write_precincts(r, file);              
          }
        }
        else 
        {
          ui32 num_tileparts = num_comps * (max_decompositions + 1);
          for (ui32 r = 0; r <= max_decompositions; ++r)
            for (ui32 c = 0; c < num_comps; ++c)
              if (r <= comps[c].get_num_decompositions()) {
                //write tile header
                if (!sot.write(file, comps[c].get_num_bytes(r), 
                               (ui8)(c + r * num_comps), (ui8)num_tileparts))
                  OJPH_ERROR(0x00030085, "Error writing to file");
                //write start of data
                ui16 t = swap_byte(JP2K_MARKER::SOD);
                if (!file->write(&t, 2))
                  OJPH_ERROR(0x00030086, "Error writing to file");                
                comps[c].write_precincts(r, file);
              }
        }
      }
      else if (prog_order == OJPH_PO_RPCL)
      {
        for (ui32 r = 0; r <= max_decompositions; ++r)
        {
          if (tilepart_div == OJPH_TILEPART_RESOLUTIONS)
          {
            ui32 bytes = 0;
            for (ui32 c = 0; c < num_comps; ++c)
              bytes += comps[c].get_num_bytes(r);
            //write tile header
            if (!sot.write(file, bytes, (ui8)r, (ui8)(max_decompositions + 1)))
              OJPH_ERROR(0x00030087, "Error writing to file");

            //write start of data
            ui16 t = swap_byte(JP2K_MARKER::SOD);
            if (!file->write(&t, 2))
              OJPH_ERROR(0x00030088, "Error writing to file");
          }
          while (true)
          {
            bool found = false;
            ui32 comp_num = 0;
            point smallest(INT_MAX, INT_MAX), cur;
            for (ui32 c = 0; c < num_comps; ++c)
            {
              if (!comps[c].get_top_left_precinct(r, cur))
                continue;
              else
                found = true;

              if (cur.y < smallest.y)
              { smallest = cur; comp_num = c; }
              else if (cur.y == smallest.y && cur.x < smallest.x)
              { smallest = cur; comp_num = c; }
            }
            if (found == true)
              comps[comp_num].write_one_precinct(r, file);
            else
              break;
          }
        }
      }
      else if (prog_order == OJPH_PO_PCRL)
      {
        while (true)
        {
          bool found = false;
          ui32 comp_num = 0;
          ui32 res_num = 0;
          point smallest(INT_MAX, INT_MAX), cur;
          for (ui32 c = 0; c < num_comps; ++c)
          {
            for (ui32 r = 0; r <= comps[c].get_num_decompositions(); ++r)
            {
              if (!comps[c].get_top_left_precinct(r, cur))
                continue;
              else
                found = true;

              if (cur.y < smallest.y)
              { smallest = cur; comp_num = c; res_num = r; }
              else if (cur.y == smallest.y && cur.x < smallest.x)
              { smallest = cur; comp_num = c; res_num = r; }
              else if (cur.y == smallest.y && cur.x == smallest.x &&
                       c < comp_num)
              { smallest = cur; comp_num = c; res_num = r; }
              else if (cur.y == smallest.y && cur.x == smallest.x &&
                       c == comp_num && r < res_num)
              { smallest = cur; comp_num = c; res_num = r; }
            }
          }
          if (found == true)
            comps[comp_num].write_one_precinct(res_num, file);
          else
            break;
        }
      }
      else if (prog_order == OJPH_PO_CPRL)
      {
        for (ui32 c = 0; c < num_comps; ++c)
        {
          if (tilepart_div == OJPH_TILEPART_COMPONENTS)
          {
            ui32 bytes = comps[c].get_num_bytes();
            //write tile header
            if (!sot.write(file, bytes, (ui8)c, (ui8)num_comps))
              OJPH_ERROR(0x0003008A, "Error writing to file");

            //write start of data
            ui16 t = swap_byte(JP2K_MARKER::SOD);
            if (!file->write(&t, 2))
              OJPH_ERROR(0x0003008B, "Error writing to file");
          }

          while (true)
          {
            bool found = false;
            ui32 res_num = 0;
            point smallest(INT_MAX, INT_MAX), cur;
            for (ui32 r = 0; r <= max_decompositions; ++r)
            {
              if (!comps[c].get_top_left_precinct(r, cur)) //res exist?
                continue;
              else
                found = true;

              if (cur.y < smallest.y)
              { smallest = cur; res_num = r; }
              else if (cur.y == smallest.y && cur.x < smallest.x)
              { smallest = cur; res_num = r; }
            }
            if (found == true)
              comps[c].write_one_precinct(res_num, file);
            else
              break;
          }
        }
      }
      else
        assert(0);

    }

    //////////////////////////////////////////////////////////////////////////
    void tile::parse_tile_header(const param_sot &sot, infile_base *file,
                                 const ui64& tile_start_location)
    {
      if (sot.get_tile_part_index() != next_tile_part)
      {
        if (resilient)
          OJPH_INFO(0x00030091, "wrong tile part index")
        else
          OJPH_ERROR(0x00030091, "wrong tile part index")
      }
      ++next_tile_part;

      //tile_end_location used on failure
      ui64 tile_end_location = tile_start_location + sot.get_payload_length();

      ui32 data_left = sot.get_payload_length(); //bytes left to parse
      data_left -= (ui32)((ui64)file->tell() - tile_start_location);

      if (data_left == 0)
        return;

      ui32 max_decompositions = 0;
      for (ui32 c = 0; c < num_comps; ++c)
        max_decompositions = ojph_max(max_decompositions,
          comps[c].get_num_decompositions());

      try
      {
        //sequence the reading of precincts according to progression order
        if (prog_order == OJPH_PO_LRCP || prog_order == OJPH_PO_RLCP)
        {
          max_decompositions -= skipped_res_for_read;
          for (ui32 r = 0; r <= max_decompositions; ++r)
            for (ui32 c = 0; c < num_comps; ++c)
              if (data_left > 0)
                comps[c].parse_precincts(r, data_left, file);
        }
        else if (prog_order == OJPH_PO_RPCL)
        {
          max_decompositions -= skipped_res_for_read;
          for (ui32 r = 0; r <= max_decompositions; ++r)
          {
            while (true)
            {
              bool found = false;
              ui32 comp_num = 0;
              point smallest(INT_MAX, INT_MAX), cur;
              for (ui32 c = 0; c < num_comps; ++c)
              {
                if (!comps[c].get_top_left_precinct(r, cur))
                  continue;
                else
                  found = true;

                if (cur.y < smallest.y)
                { smallest = cur; comp_num = c; }
                else if (cur.y == smallest.y && cur.x < smallest.x)
                { smallest = cur; comp_num = c; }
              }
              if (found == true && data_left > 0)
                comps[comp_num].parse_one_precinct(r, data_left, file);
              else
                break;
            }
          }
        }
        else if (prog_order == OJPH_PO_PCRL)
        {
          while (true)
          {
            bool found = false;
            ui32 comp_num = 0;
            ui32 res_num = 0;
            point smallest(INT_MAX, INT_MAX), cur;
            for (ui32 c = 0; c < num_comps; ++c)
            {
              for (ui32 r = 0; r <= comps[c].get_num_decompositions(); ++r)
              {
                if (!comps[c].get_top_left_precinct(r, cur))
                  continue;
                else
                  found = true;

                if (cur.y < smallest.y)
                { smallest = cur; comp_num = c; res_num = r; }
                else if (cur.y == smallest.y && cur.x < smallest.x)
                { smallest = cur; comp_num = c; res_num = r; }
                else if (cur.y == smallest.y && cur.x == smallest.x &&
                         c < comp_num)
                { smallest = cur; comp_num = c; res_num = r; }
                else if (cur.y == smallest.y && cur.x == smallest.x &&
                         c == comp_num && r < res_num)
                { smallest = cur; comp_num = c; res_num = r; }
              }
            }
            if (found == true && data_left > 0)
              comps[comp_num].parse_one_precinct(res_num, data_left, file);
            else
              break;
          }
        }
        else if (prog_order == OJPH_PO_CPRL)
        {
          for (ui32 c = 0; c < num_comps; ++c)
          {
            while (true)
            {
              bool found = false;
              ui32 res_num = 0;
              point smallest(INT_MAX, INT_MAX), cur;
              for (ui32 r = 0; r <= max_decompositions; ++r)
              {
                if (!comps[c].get_top_left_precinct(r, cur)) //res exist?
                  continue;
                else
                  found = true;

                if (cur.y < smallest.y)
                { smallest = cur; res_num = r; }
                else if (cur.y == smallest.y && cur.x < smallest.x)
                { smallest = cur; res_num = r; }
              }
              if (found == true && data_left > 0)
                comps[c].parse_one_precinct(res_num, data_left, file);
              else
                break;
            }
          }
        }
        else
          assert(0);

      }
      catch (const char *error)
      {
        if (resilient)
          OJPH_INFO(0x00030092, "%s", error)
        else
          OJPH_ERROR(0x00030092, "%s", error)
      }
      file->seek((si64)tile_end_location, infile_base::OJPH_SEEK_SET);
    }

  }
}