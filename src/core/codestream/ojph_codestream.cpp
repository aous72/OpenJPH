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
// File: ojph_codestream.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#include <climits>
#include <cmath>

#include "ojph_file.h"
#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream.h"
#include "ojph_codestream_local.h"

#include "../transform/ojph_colour.h"
#include "../transform/ojph_transform.h"
#include "../coding/ojph_block_decoder.h"
#include "../coding/ojph_block_encoder.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  codestream::~codestream()
  {
    if (state) delete state;
  }

  ////////////////////////////////////////////////////////////////////////////
  codestream::codestream()
  {
    state = new local::codestream;
  }

  ////////////////////////////////////////////////////////////////////////////
  param_siz codestream::access_siz()
  {
    return param_siz(&state->siz);
  }

  ////////////////////////////////////////////////////////////////////////////
  param_cod codestream::access_cod()
  {
    return param_cod(&state->cod);
  }

  ////////////////////////////////////////////////////////////////////////////
  param_qcd codestream::access_qcd()
  {
    return param_qcd(&state->qcd);
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::set_planar(bool planar)
  {
    state->set_planar(planar ? 1 : 0);
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::set_profile(const char *s)
  {
    state->set_profile(s);
  }

  ////////////////////////////////////////////////////////////////////////////
  bool codestream::is_planar() const
  {
    return state->is_planar();
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::write_headers(outfile_base *file)
  {
    state->write_headers(file);
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::enable_resilience()
  {
    state->enable_resilience();
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::read_headers(infile_base *file)
  {
    state->read_headers(file);
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::restrict_input_resolution(ui32 skipped_res_for_read,
                                             ui32 skipped_res_for_recon)
  {
    state->restrict_input_resolution(skipped_res_for_read,
      skipped_res_for_recon);
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::create()
  {
    state->read();
  }

  ////////////////////////////////////////////////////////////////////////////
  line_buf* codestream::pull(ui32 &comp_num)
  {
    return state->pull(comp_num);
  }


  ////////////////////////////////////////////////////////////////////////////
  void codestream::flush()
  {
    state->flush();
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::close()
  {
    state->close();
  }

  ////////////////////////////////////////////////////////////////////////////
  line_buf* codestream::exchange(line_buf* line, ui32& next_component)
  {
    return state->exchange(line, next_component);
  }



  //////////////////////////////////////////////////////////////////////////
  //
  //
  //                                LOCAL
  //
  //
  //////////////////////////////////////////////////////////////////////////

  namespace local
  {

    /////////////////////////////////////////////////////////////////////////
    static inline
    ui16 swap_byte(ui16 t)
    {
      return (ui16)((t << 8) | (t >> 8));
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////
    codestream::codestream()
    : precinct_scratch(NULL), allocator(NULL), elastic_alloc(NULL)
    {
      tiles = NULL;
      lines = NULL;
      comp_size = NULL;
      recon_comp_size = NULL;
      allocator = NULL;
      outfile = NULL;
      infile = NULL;

      num_comps = 0;
      employ_color_transform = false;
      planar = -1;
      profile = OJPH_PROFILE_NUM::OJPH_PN_UNDEFINED;

      cur_comp = 0;
      cur_line = 0;
      cur_tile_row = 0;
      resilient = false;
      skipped_res_for_read = skipped_res_for_recon = 0;

      precinct_scratch_needed_bytes = 0;

      used_qcc_fields = 0;
      qcc = qcc_store;

      allocator = new mem_fixed_allocator;
      elastic_alloc = new mem_elastic_allocator(1048576); //1 megabyte

      init_colour_transform_functions();
      init_wavelet_transform_functions();
    }

    ////////////////////////////////////////////////////////////////////////////
    codestream::~codestream()
    {
      if (qcc_store != qcc)
        delete[] qcc;
      if (allocator)
        delete allocator;
      if (elastic_alloc)
        delete elastic_alloc;
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::pre_alloc()
    {
      ojph::param_siz sz = access_siz();
      num_tiles.w = sz.get_image_extent().x - sz.get_tile_offset().x;
      num_tiles.w = ojph_div_ceil(num_tiles.w, sz.get_tile_size().w);
      num_tiles.h = sz.get_image_extent().y - sz.get_tile_offset().y;
      num_tiles.h = ojph_div_ceil(num_tiles.h, sz.get_tile_size().h);
      if (num_tiles.area() > 65535)
        OJPH_ERROR(0x00030011, "number of tiles cannot exceed 65535");

      //allocate tiles
      allocator->pre_alloc_obj<tile>(num_tiles.area());

      point index;
      rect tile_rect, recon_tile_rect;
      ui32 ds = 1 << skipped_res_for_recon;
      for (index.y = 0; index.y < num_tiles.h; ++index.y)
      {
        ui32 y0 = sz.get_tile_offset().y
                + index.y * sz.get_tile_size().h;
        ui32 y1 = y0 + sz.get_tile_size().h; //end of tile

        tile_rect.org.y = ojph_max(y0, sz.get_image_offset().y);
        tile_rect.siz.h = 
          ojph_min(y1, sz.get_image_extent().y) - tile_rect.org.y;

        recon_tile_rect.org.y = ojph_max(ojph_div_ceil(y0, ds), 
          ojph_div_ceil(sz.get_image_offset().y, ds));
        recon_tile_rect.siz.h = ojph_min(ojph_div_ceil(y1, ds),
          ojph_div_ceil(sz.get_image_extent().y, ds))
          - recon_tile_rect.org.y;

        for (index.x = 0; index.x < num_tiles.w; ++index.x)
        {
          ui32 x0 = sz.get_tile_offset().x
                  + index.x * sz.get_tile_size().w;
          ui32 x1 = x0 + sz.get_tile_size().w;

          tile_rect.org.x = ojph_max(x0, sz.get_image_offset().x);
          tile_rect.siz.w = 
            ojph_min(x1, sz.get_image_extent().x) - tile_rect.org.x;

          recon_tile_rect.org.x = ojph_max(ojph_div_ceil(x0, ds),
            ojph_div_ceil(sz.get_image_offset().x, ds));
          recon_tile_rect.siz.w = ojph_min(ojph_div_ceil(x1, ds),
            ojph_div_ceil(sz.get_image_extent().x, ds))
            - recon_tile_rect.org.x;

          tile::pre_alloc(this, tile_rect, recon_tile_rect);
        }
      }

      //allocate lines
      //These lines are used by codestream to exchange data with external
      // world
      ui32 num_comps = sz.get_num_components();
      allocator->pre_alloc_obj<line_buf>(num_comps);
      allocator->pre_alloc_obj<size>(num_comps); //for *comp_size
      allocator->pre_alloc_obj<size>(num_comps); //for *recon_comp_size
      for (ui32 i = 0; i < num_comps; ++i)
        allocator->pre_alloc_data<si32>(siz.get_recon_width(i), 0);

      //allocate tlm
      if (outfile != NULL)
      {
        if (profile == OJPH_PN_IMF || profile == OJPH_PN_BROADCAST)
        {
          ui32 num_pairs = (ui32)num_tiles.area() * num_comps;
          allocator->pre_alloc_obj<param_tlm::Ttlm_Ptlm_pair>(num_pairs);
        }
        else
        {
          ui32 num_pairs = (ui32)num_tiles.area();
          allocator->pre_alloc_obj<param_tlm::Ttlm_Ptlm_pair>(num_pairs);
        }
      }

      //precinct scratch buffer
      ui32 num_decomps = cod.get_num_decompositions();
      size log_cb = cod.get_log_block_dims();

      size ratio;
      for (ui32 r = 0; r <= num_decomps; ++r)
      {
        size log_PP = cod.get_log_precinct_size(r);
        log_PP.w -= (r ? 1 : 0);
        log_PP.h -= (r ? 1 : 0);
        ratio.w = ojph_max(ratio.w, log_PP.w - ojph_min(log_cb.w, log_PP.w));
        ratio.h = ojph_max(ratio.h, log_PP.h - ojph_min(log_cb.h, log_PP.h));
      }
      ui32 max_ratio = ojph_max(ratio.w, ratio.h);
      max_ratio = 1 << max_ratio;
      // assuming that we have a hierarchy of n levels.
      // This needs 4/3 times the area, rounded up
      // (rounding up leaves one extra entry).
      // This exta entry is necessary
      // We need 4 such tables. These tables store
      // 1. missing msbs and 2. their flags, 
      // 3. number of layers and 4. their flags
      precinct_scratch_needed_bytes = 
        4 * ((max_ratio * max_ratio * 4 + 2) / 3);

      allocator->pre_alloc_obj<ui8>(precinct_scratch_needed_bytes);
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::finalize_alloc()
    {
      allocator->alloc();

      //precinct scratch buffer
      precinct_scratch = 
        allocator->post_alloc_obj<ui8>(precinct_scratch_needed_bytes);

      //get tiles
      tiles = this->allocator->post_alloc_obj<tile>(num_tiles.area());

      point index;
      rect tile_rect, recon_tile_rect;
      ojph::param_siz sz = access_siz();
      ui32 ds = 1 << skipped_res_for_recon;
      for (index.y = 0; index.y < num_tiles.h; ++index.y)
      {
        ui32 y0 = sz.get_tile_offset().y
                + index.y * sz.get_tile_size().h;
        ui32 y1 = y0 + sz.get_tile_size().h; //end of tile

        tile_rect.org.y = ojph_max(y0, sz.get_image_offset().y);
        tile_rect.siz.h = 
          ojph_min(y1, sz.get_image_extent().y) - tile_rect.org.y;

        recon_tile_rect.org.y = ojph_max(ojph_div_ceil(y0, ds), 
          ojph_div_ceil(sz.get_image_offset().y, ds));
        recon_tile_rect.siz.h = ojph_min(ojph_div_ceil(y1, ds),
          ojph_div_ceil(sz.get_image_extent().y, ds))
          - recon_tile_rect.org.y;

        ui32 offset = 0;
        for (index.x = 0; index.x < num_tiles.w; ++index.x)
        {
          ui32 x0 = sz.get_tile_offset().x
                  + index.x * sz.get_tile_size().w;
          ui32 x1 = x0 + sz.get_tile_size().w;

          tile_rect.org.x = ojph_max(x0, sz.get_image_offset().x);
          tile_rect.siz.w = 
            ojph_min(x1, sz.get_image_extent().x) - tile_rect.org.x;

          recon_tile_rect.org.x = ojph_max(ojph_div_ceil(x0, ds),
            ojph_div_ceil(sz.get_image_offset().x, ds));
          recon_tile_rect.siz.w = ojph_min(ojph_div_ceil(x1, ds),
            ojph_div_ceil(sz.get_image_extent().x, ds))
            - recon_tile_rect.org.x;

          ui32 idx = index.y * num_tiles.w + index.x;
          tiles[idx].finalize_alloc(this, tile_rect, recon_tile_rect, 
                                    idx, offset);
          offset += recon_tile_rect.siz.w;
        }
      }

      //allocate lines
      //These lines are used by codestream to exchange data with external
      // world
      this->num_comps = sz.get_num_components();
      lines = allocator->post_alloc_obj<line_buf>(this->num_comps);
      comp_size = allocator->post_alloc_obj<size>(this->num_comps);
      recon_comp_size = allocator->post_alloc_obj<size>(this->num_comps);
      employ_color_transform = cod.is_employing_color_transform();
      for (ui32 i = 0; i < this->num_comps; ++i)
      {
        comp_size[i].w = siz.get_width(i);
        comp_size[i].h = siz.get_height(i);
        ui32 cw = siz.get_recon_width(i);
        recon_comp_size[i].w = cw;
        recon_comp_size[i].h = siz.get_recon_height(i);
        lines[i].wrap(allocator->post_alloc_data<si32>(cw, 0), cw, 0);        
      }

      cur_comp = 0;
      cur_line = 0;

      //allocate tlm
      if (outfile != NULL)
      {
        if (profile == OJPH_PN_IMF || profile == OJPH_PN_BROADCAST)
        {
          ui32 num_pairs = (ui32)num_tiles.area() * this->num_comps;
          tlm.init(num_pairs,
            allocator->post_alloc_obj<param_tlm::Ttlm_Ptlm_pair>(num_pairs));
        }
        else
        {
          ui32 num_pairs = (ui32)num_tiles.area();
          tlm.init(num_pairs,
            allocator->post_alloc_obj<param_tlm::Ttlm_Ptlm_pair>(num_pairs));
        }
      }
    }


    //////////////////////////////////////////////////////////////////////////
    void codestream::check_imf_validity()
    {
      //two possibilities lossy single tile or lossless
      //For the following code, we use the least strict profile
      ojph::param_siz sz(&siz);
      ojph::param_cod cd(&cod);
      bool reversible = cd.is_reversible();
      bool imf2k = !reversible, imf4k = !reversible, imf8k = !reversible;
      bool imf2kls = reversible, imf4kls = reversible, imf8kls = reversible;

      if (reversible)
      {
        point ext = sz.get_image_extent();
        if (ext.x <= 2048 && ext.y <= 1556)
          imf2kls &= true;
        if (ext.x <= 4096 && ext.y <= 3112)
          imf4kls &= true;
        if (ext.x <= 8192 && ext.y <= 6224)
          imf8kls &= true;

        if (!imf2kls && !imf4kls && !imf8kls)
          OJPH_ERROR(0x000300C1,
            "Image dimensions do not meet any of the lossless IMF profiles");
      }
      else
      {
        point ext = sz.get_image_extent();
        if (ext.x <= 2048 && ext.y <= 1556)
          imf2k &= true;
        if (ext.x <= 4096 && ext.y <= 3112)
          imf4k &= true;
        if (ext.x <= 8192 && ext.y <= 6224)
          imf8k &= true;

        if (!imf2k && !imf4k && !imf8k)
          OJPH_ERROR(0x000300C2,
            "Image dimensions do not meet any of the lossy IMF profiles");
      }


      if (sz.get_image_offset().x != 0 || sz.get_image_offset().y != 0)
        OJPH_ERROR(0x000300C3,
          "For IMF profile, image offset (XOsiz, YOsiz) has to be 0.");
      if (sz.get_tile_offset().x != 0 || sz.get_tile_offset().y != 0)
        OJPH_ERROR(0x000300C4,
          "For IMF profile, tile offset (XTOsiz, YTOsiz) has to be 0.");
      if (sz.get_num_components() > 3)
        OJPH_ERROR(0x000300C5,
          "For IMF profile, the number of components has to be less "
          " or equal to 3");
      bool test_ds1 = true, test_ds2 = true;
      for (ojph::ui32 i = 0; i < sz.get_num_components(); ++i)
      {
        point downsamping = sz.get_downsampling(i);
        test_ds1 &= downsamping.y == 1;
        test_ds2 &= downsamping.y == 1;

        test_ds1 &= downsamping.x == 1;
        if (i == 1 || i == 2)
          test_ds2 &= downsamping.x == 2;
        else
          test_ds2 &= downsamping.x == 1;
      }
      if (!test_ds1 && !test_ds2)
        OJPH_ERROR(0x000300C6,
          "For IMF profile, either no component downsampling is used,"
          " or the x-dimension of the 2nd and 3rd components is downsampled"
          " by 2.");

      bool test_bd = true;
      for (ojph::ui32 i = 0; i < sz.get_num_components(); ++i)
      {
        ui32 bit_depth = sz.get_bit_depth(i);
        bool is_signed = sz.is_signed(i);
        test_bd &= bit_depth >= 8 && bit_depth <= 16 && is_signed == false;
      }
      if (!test_bd)
        OJPH_ERROR(0x000300C7,
          "For IMF profile, compnent bit_depth has to be between"
          " 8 and 16 bits inclusively, and the samples must be unsigned");

      if (cd.get_log_block_dims().w != 5 || cd.get_log_block_dims().h != 5)
        OJPH_ERROR(0x000300C8,
          "For IMF profile, codeblock dimensions are restricted."
          " Use \"-block_size {32,32}\" at the commandline");

      ui32 num_decomps = cd.get_num_decompositions();
      bool test_pz = cd.get_log_precinct_size(0).w == 7
                  && cd.get_log_precinct_size(0).h == 7;
      for (ui32 i = 1; i <= num_decomps; ++i)
        test_pz = cd.get_log_precinct_size(i).w == 8
               && cd.get_log_precinct_size(i).h == 8;
      if (!test_pz)
        OJPH_ERROR(0x000300C9,
          "For IMF profile, precinct sizes are restricted."
          " Use \"-precincts {128,128},{256,256}\" at the commandline");

      if (cd.get_progression_order() != OJPH_PO_CPRL)
        OJPH_ERROR(0x000300CA,
          "For IMF profile, the CPRL progression order must be used."
          " Use \"-prog_order CPRL\".");

      imf2k &= num_decomps <= 5;
      imf2kls &= num_decomps <= 5;
      imf4k &= num_decomps <= 6;
      imf4kls &= num_decomps <= 6;
      imf8k &= num_decomps <= 7;
      imf8kls &= num_decomps <= 7;

      if (num_decomps == 0 ||
        (!imf2k && !imf4k && !imf8k && !imf2kls && !imf4kls && !imf8kls))
        OJPH_ERROR(0x000300CB,
          "Number of decompositions does not match the IMF profile"
          " dictated by wavelet reversibility and image dimensions.");

      ui32 tiles_w = sz.get_image_extent().x;
      tiles_w = ojph_div_ceil(tiles_w, sz.get_tile_size().w);
      ui32 tiles_h = sz.get_image_extent().y;
      tiles_h = ojph_div_ceil(tiles_h, sz.get_tile_size().h);
      ui32 total_tiles = tiles_w * tiles_h;

      if (total_tiles > 1)
      {
        if (!reversible)
          OJPH_ERROR(0x000300CC,
            "Lossy IMF profile must have one tile.");

        size tt = sz.get_tile_size();
        imf2kls &= (tt.w == 1024 && tt.h == 1024);
        imf2kls &= (tt.w >= 1024 && num_decomps <= 4)
                || (tt.w >= 2048 && num_decomps <= 5);
        imf4kls &= (tt.w == 1024 && tt.h == 1024)
                || (tt.w == 2048 && tt.h == 2048);
        imf4kls &= (tt.w >= 1024 && num_decomps <= 4)
                || (tt.w >= 2048 && num_decomps <= 5)
                || (tt.w >= 4096 && num_decomps <= 6);
        imf8kls &= (tt.w == 1024 && tt.h == 1024)
                || (tt.w == 2048 && tt.h == 2048)
                || (tt.w == 4096 && tt.h == 4096);
        imf8kls &= (tt.w >= 1024 && num_decomps <= 4)
                || (tt.w >= 2048 && num_decomps <= 5)
                || (tt.w >= 4096 && num_decomps <= 6)
                || (tt.w >= 8192 && num_decomps <= 7);
        if (!imf2kls && !imf4kls && !imf8kls)
          OJPH_ERROR(0x000300CD,
            "Number of decompositions does not match the IMF profile"
            " dictated by wavelet reversibility and image dimensions and"
            " tiles.");
     }

    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::check_broadcast_validity()
    {
      ojph::param_siz sz(&siz);
      ojph::param_cod cd(&cod);

      if (sz.get_image_offset().x != 0 || sz.get_image_offset().y != 0)
        OJPH_ERROR(0x000300B1,
          "For broadcast profile, image offset (XOsiz, YOsiz) has to be 0.");
      if (sz.get_tile_offset().x != 0 || sz.get_tile_offset().y != 0)
        OJPH_ERROR(0x000300B2,
          "For broadcast profile, tile offset (XTOsiz, YTOsiz) has to be 0.");
      if (sz.get_num_components() > 4)
        OJPH_ERROR(0x000300B3,
          "For broadcast profile, the number of components has to be less "
          " or equal to 4");
      bool test_ds1 = true, test_ds2 = true;
      for (ojph::ui32 i = 0; i < sz.get_num_components(); ++i)
      {
        point downsamping = sz.get_downsampling(i);
        test_ds1 &= downsamping.y == 1;
        test_ds2 &= downsamping.y == 1;

        test_ds1 &= downsamping.x == 1;
        if (i == 1 || i == 2)
          test_ds2 &= downsamping.x == 2;
        else
          test_ds2 &= downsamping.x == 1;
      }
      if (!test_ds1 && !test_ds2)
        OJPH_ERROR(0x000300B4,
          "For broadcast profile, either no component downsampling is used,"
          " or the x-dimension of the 2nd and 3rd components is downsampled"
          " by 2.");

      bool test_bd = true;
      for (ojph::ui32 i = 0; i < sz.get_num_components(); ++i)
      {
        ui32 bit_depth = sz.get_bit_depth(i);
        bool is_signed = sz.is_signed(i);
        test_bd &= bit_depth >= 8 && bit_depth <= 12 && is_signed == false;
      }
      if (!test_bd)
        OJPH_ERROR(0x000300B5,
          "For broadcast profile, compnent bit_depth has to be between"
          " 8 and 12 bits inclusively, and the samples must be unsigned");

      ui32 num_decomps = cd.get_num_decompositions();
      if (num_decomps == 0 || num_decomps > 5)
        OJPH_ERROR(0x000300B6,
          "For broadcast profile, number of decompositions has to be between"
          "1 and 5 inclusively.");

      if (cd.get_log_block_dims().w < 5 || cd.get_log_block_dims().w > 7)
        OJPH_ERROR(0x000300B7,
          "For broadcast profile, codeblock dimensions are restricted such"
          " that codeblock width has to be either 32, 64, or 128.");

      if (cd.get_log_block_dims().h < 5 || cd.get_log_block_dims().h > 7)
        OJPH_ERROR(0x000300B8,
          "For broadcast profile, codeblock dimensions are restricted such"
          " that codeblock height has to be either 32, 64, or 128.");

      bool test_pz = cd.get_log_precinct_size(0).w == 7
                  && cd.get_log_precinct_size(0).h == 7;
      for (ui32 i = 1; i <= num_decomps; ++i)
        test_pz = cd.get_log_precinct_size(i).w == 8
               && cd.get_log_precinct_size(i).h == 8;
      if (!test_pz)
        OJPH_ERROR(0x000300B9,
          "For broadcast profile, precinct sizes are restricted."
          " Use \"-precincts {128,128},{256,256}\" at the commandline");

      if (cd.get_progression_order() != OJPH_PO_CPRL)
        OJPH_ERROR(0x000300BA,
          "For broadcast profile, the CPRL progression order must be used."
          " Use \"-prog_order CPRL\".");

      ui32 tiles_w = sz.get_image_extent().x;
      tiles_w = ojph_div_ceil(tiles_w, sz.get_tile_size().w);
      ui32 tiles_h = sz.get_image_extent().y;
      tiles_h = ojph_div_ceil(tiles_h, sz.get_tile_size().h);
      ui32 total_tiles = tiles_w * tiles_h;

      if (total_tiles != 1 && total_tiles != 4)
        OJPH_ERROR(0x000300BB,
          "The broadcast profile can only have 1 or 4 tiles");
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::write_headers(outfile_base *file)
    {
      //finalize
      siz.check_validity();
      cod.check_validity(siz);
      qcd.check_validity(siz, cod);
      cap.check_validity(cod, qcd);
      if (profile == OJPH_PN_IMF)
        check_imf_validity();
      else if (profile == OJPH_PN_BROADCAST)
        check_broadcast_validity();

      if (planar == -1) //not initialized
        planar = cod.is_employing_color_transform() ? 1 : 0;
      else if (planar == 0) //interleaved is chosen
      {
      }
      else if (planar == 1) //plannar is chosen
      {
        if (cod.is_employing_color_transform() == true)
          OJPH_ERROR(0x00030021,
            "the planar interface option cannot be used when colour "
            "transform is employed");
      }
      else
        assert(0);

      assert(this->outfile == NULL);
      this->outfile = file;
      this->pre_alloc();
      this->finalize_alloc();

      ui16 t = swap_byte(JP2K_MARKER::SOC);
      if (file->write(&t, 2) != 2)
        OJPH_ERROR(0x00030022, "Error writing to file");

      if (!siz.write(file))
        OJPH_ERROR(0x00030023, "Error writing to file");

      if (!cap.write(file))
        OJPH_ERROR(0x00030024, "Error writing to file");

      if (!cod.write(file))
        OJPH_ERROR(0x00030025, "Error writing to file");

      if (!qcd.write(file))
        OJPH_ERROR(0x00030026, "Error writing to file");

      char buf[] = "      OpenJPH Ver "
        OJPH_INT_TO_STRING(OPENJPH_VERSION_MAJOR) "."
        OJPH_INT_TO_STRING(OPENJPH_VERSION_MINOR) "."
        OJPH_INT_TO_STRING(OPENJPH_VERSION_PATCH) ".";
      size_t len = strlen(buf);
      *(ui16*)buf = swap_byte(JP2K_MARKER::COM);
      *(ui16*)(buf + 2) = swap_byte((ui16)(len - 2));
      //1 for General use (IS 8859-15:1999 (Latin) values)
      *(ui16*)(buf + 4) = swap_byte((ui16)(1)); 
      if (file->write(buf, len) != len)
        OJPH_ERROR(0x00030027, "Error writing to file");
    }

    //////////////////////////////////////////////////////////////////////////
    static
    int find_marker(infile_base *f, const ui16* char_list, int list_len)
    {
      //returns the marker index in char_list, or -1
      while (!f->eof())
      {
        ui8 new_char;
        size_t num_bytes = f->read(&new_char, 1);
        if (num_bytes != 1)
            return -1;
        if (new_char == 0xFF)
        {
          size_t num_bytes = f->read(&new_char, 1);

          if (num_bytes != 1)
              return -1;

          for (int i = 0; i < list_len; ++i)
            if (new_char == (char_list[i] & 0xFF))
              return i;
        }
      }
      return -1;
    }

    //////////////////////////////////////////////////////////////////////////
    static
    int skip_marker(infile_base *file, const char *marker,
                     const char *msg, int msg_level, bool resilient)
    {
      ojph_unused(marker);
      ui16 com_len;
      if (file->read(&com_len, 2) != 2)
      {
        if (resilient)
          return -1;
        else
          OJPH_ERROR(0x00030041, "error reading marker");
      }
      com_len = swap_byte(com_len);
      file->seek(com_len - 2, infile_base::OJPH_SEEK_CUR);
      if (msg != NULL && msg_level != OJPH_MSG_LEVEL::NO_MSG)
      {
        if (msg_level == OJPH_MSG_LEVEL::INFO)
        {
          OJPH_INFO(0x00030001, "%s\n", msg);
        }
        else if (msg_level == OJPH_MSG_LEVEL::WARN)
        {
          OJPH_WARN(0x00030001, "%s\n", msg);
        }
        else if (msg_level == OJPH_MSG_LEVEL::ERROR)
        {
          OJPH_ERROR(0x00030001, "%s\n", msg);
        }
        else
          assert(0);
      }
      return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::read_headers(infile_base *file)
    {
      ui16 marker_list[17] = { SOC, SIZ, CAP, PRF, CPF, COD, COC, QCD, QCC,
        RGN, POC, PPM, TLM, PLM, CRG, COM, SOT };
      find_marker(file, marker_list, 1); //find SOC
      find_marker(file, marker_list + 1, 1); //find SIZ
      siz.read(file);
      int marker_idx = 0;
      int received_markers = 0; //check that COD, & QCD received
      while (true)
      {
        marker_idx = find_marker(file, marker_list + 2, 15);
        if (marker_idx == 0)
          cap.read(file);
        else if (marker_idx == 1)
          //Skipping PRF marker segment; this should not cause any issues
          skip_marker(file, "PRF", NULL, OJPH_MSG_LEVEL::NO_MSG, false);
        else if (marker_idx == 2)
          //Skipping CPF marker segment; this should not cause any issues
          skip_marker(file, "CPF", NULL, OJPH_MSG_LEVEL::NO_MSG, false);
        else if (marker_idx == 3)
        { cod.read(file); received_markers |= 1; }
        else if (marker_idx == 4)
          skip_marker(file, "COC", "COC is not supported yet",
            OJPH_MSG_LEVEL::WARN, false);
        else if (marker_idx == 5)
        { qcd.read(file); received_markers |= 2; }
        else if (marker_idx == 6)
          {
            ui32 num_comps = siz.get_num_components();
            if (qcc == qcc_store && 
                num_comps * sizeof(param_qcc) > sizeof(qcc_store))
            {
              qcc = new param_qcc[num_comps];
            }
            qcc[used_qcc_fields++].read(file, num_comps);
          }
        else if (marker_idx == 7)
          skip_marker(file, "RGN", "RGN is not supported yet",
            OJPH_MSG_LEVEL::WARN, false);
        else if (marker_idx == 8)
          skip_marker(file, "POC", "POC is not supported yet",
            OJPH_MSG_LEVEL::WARN, false);
        else if (marker_idx == 9)
          skip_marker(file, "PPM", "PPM is not supported yet",
            OJPH_MSG_LEVEL::WARN, false);
        else if (marker_idx == 10)
          //Skipping TLM marker segment; this should not cause any issues
          skip_marker(file, "TLM", NULL, OJPH_MSG_LEVEL::NO_MSG, false);
        else if (marker_idx == 11)
          //Skipping PLM marker segment; this should not cause any issues
          skip_marker(file, "PLM", NULL, OJPH_MSG_LEVEL::NO_MSG, false);
        else if (marker_idx == 12)
          //Skipping CRG marker segment;
          skip_marker(file, "CRG", "CRG has been ignored; CRG is related to"
            " where the Cb and Cr colour components are co-sited or located"
            " with respect to the Y' luma component. Perhaps, it is better"
            " to get the indivdual components and assemble the samples"
            " according to your needs",
            OJPH_MSG_LEVEL::INFO, false);
        else if (marker_idx == 13)
          skip_marker(file, "COM", NULL, OJPH_MSG_LEVEL::NO_MSG, false);
        else if (marker_idx == 14)
          break;
        else
          OJPH_ERROR(0x00030051, "File ended before finding a tile segment");
      }

      if (received_markers != 3)
        OJPH_ERROR(0x00030052, "markers error, COD and QCD are required");

      this->infile = file;
      planar = cod.is_employing_color_transform() ? 0 : 1;
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::restrict_input_resolution(ui32 skipped_res_for_read,
      ui32 skipped_res_for_recon)
    {
      if (skipped_res_for_read < skipped_res_for_recon)
        OJPH_ERROR(0x000300A1,
          "skipped_resolution for data %d must be equal or smaller than "
          " skipped_resolution for reconstruction %d\n", 
          skipped_res_for_read, skipped_res_for_recon);
      if (skipped_res_for_read > cod.get_num_decompositions())
        OJPH_ERROR(0x000300A2,
          "skipped_resolution for data %d must be smaller than "
          " the number of decomposition levels %d\n",
          skipped_res_for_read, cod.get_num_decompositions());

      this->skipped_res_for_read = skipped_res_for_read;
      this->skipped_res_for_recon = skipped_res_for_recon;
      siz.set_skipped_resolutions(skipped_res_for_recon);
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::enable_resilience()
    {
      if (infile != NULL)
        OJPH_ERROR(0x000300A3, "Codestream resilience must be enabled before"
          " reading file headers.\n");
      this->resilient = true;
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::read()
    {
      this->pre_alloc();
      this->finalize_alloc();

      while (true)
      {
        param_sot sot;
        if (sot.read(infile, resilient))
        {
          ui64 tile_start_location = (ui64)infile->tell();

          if (sot.get_tile_index() > (int)num_tiles.area())
          {
            if (resilient)
              OJPH_INFO(0x00030061, "wrong tile index")
            else
              OJPH_ERROR(0x00030061, "wrong tile index")
          }

          if (sot.get_tile_part_index())
          { //tile part
            if (sot.get_num_tile_parts() &&
              sot.get_tile_part_index() >= sot.get_num_tile_parts())
            {
              if (resilient)
                OJPH_INFO(0x00030062,
                  "error in tile part number, should be smaller than total"
                  " number of tile parts")
              else
                OJPH_ERROR(0x00030062,
                  "error in tile part number, should be smaller than total"
                  " number of tile parts")
            }

            bool sod_found = false;
            ui16 other_tile_part_markers[6] = { SOT, POC, PPT, PLT, COM, SOD };
            while (true)
            {
              int marker_idx = 0;
              int result = 0;
              marker_idx = find_marker(infile, other_tile_part_markers + 1, 5);
              if (marker_idx == 0)
                result = skip_marker(infile, "POC",
                  "POC in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 1)
                result = skip_marker(infile, "PPT",
                  "PPT in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 2)
                //Skipping PLT marker segment;this should not cause any issues
                result = skip_marker(infile, "PLT", NULL,
                  OJPH_MSG_LEVEL::NO_MSG, resilient);
              else if (marker_idx == 3)
                result = skip_marker(infile, "COM", NULL,
                  OJPH_MSG_LEVEL::NO_MSG, resilient);
              else if (marker_idx == 4)
              {
                sod_found = true;
                break;
              }

              if (marker_idx == -1) //marker not found
              {
                if (resilient)
                  OJPH_INFO(0x00030063,
                    "File terminated early before start of data is found"
                    " for tile indexed %d and tile part %d",
                    sot.get_tile_index(), sot.get_tile_part_index())
                else
                  OJPH_ERROR(0x00030063,
                    "File terminated early before start of data is found"
                    " for tile indexed %d and tile part %d",
                    sot.get_tile_index(), sot.get_tile_part_index())
                break;
              }
              if (result == -1) //file terminated during marker seg. skipping
              {
                if (resilient)
                  OJPH_INFO(0x00030064,
                    "File terminated during marker segment skipping")
                else
                  OJPH_ERROR(0x00030064,
                    "File terminated during marker segment skipping")
                break;
              }
            }
            if (sod_found)
              tiles[sot.get_tile_index()].parse_tile_header(sot, infile,
                tile_start_location);
          }
          else
          { //first tile part
            bool sod_found = false;
            ui16 first_tile_part_markers[11] = { SOT, COD, COC, QCD, QCC, RGN,
              POC, PPT, PLT, COM, SOD };
            while (true)
            {
              int marker_idx = 0;
              int result = 0;
              marker_idx = find_marker(infile, first_tile_part_markers+1, 10);
              if (marker_idx == 0)
                result = skip_marker(infile, "COD",
                  "COD in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 1)
                result = skip_marker(infile, "COC",
                  "COC in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 2)
                result = skip_marker(infile, "QCD",
                  "QCD in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 3)
                result = skip_marker(infile, "QCC",
                  "QCC in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 4)
                result = skip_marker(infile, "RGN",
                  "RGN in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 5)
                result = skip_marker(infile, "POC",
                  "POC in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 6)
                result = skip_marker(infile, "PPT",
                  "PPT in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 7)
                //Skipping PLT marker segment;this should not cause any issues
                result = skip_marker(infile, "PLT", NULL,
                  OJPH_MSG_LEVEL::NO_MSG, resilient);
              else if (marker_idx == 8)
                result = skip_marker(infile, "COM", NULL,
                  OJPH_MSG_LEVEL::NO_MSG, resilient);
              else if (marker_idx == 9)
              {
                sod_found = true;
                break;
              }

              if (marker_idx == -1) //marker not found
              {
                if (resilient)
                  OJPH_INFO(0x00030065,
                    "File terminated early before start of data is found"
                    " for tile indexed %d and tile part %d",
                    sot.get_tile_index(), sot.get_tile_part_index())
                else
                  OJPH_ERROR(0x00030065,
                    "File terminated early before start of data is found"
                    " for tile indexed %d and tile part %d",
                    sot.get_tile_index(), sot.get_tile_part_index())
                break;
              }
              if (result == -1) //file terminated during marker seg. skipping
              {
                if (resilient)
                  OJPH_INFO(0x00030066,
                    "File terminated during marker segment skipping")
                else
                  OJPH_ERROR(0x00030066,
                    "File terminated during marker segment skipping")
                break;
              }
            }
            if (sod_found)
              tiles[sot.get_tile_index()].parse_tile_header(sot, infile,
                tile_start_location);
          }
        }

        // check the next marker; either SOT or EOC,
        // if something is broken, just an end of file
        ui16 next_markers[2] = { SOT, EOC };
        int marker_idx = find_marker(infile, next_markers, 2);
        if (marker_idx == -1)
        {
          OJPH_INFO(0x00030067, "File terminated early");
          break;
        }
        else if (marker_idx == 0)
          ;
        else if (marker_idx == 1)
          break;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::set_planar(int planar)
    {
      this->planar = planar;
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::set_profile(const char *s)
    {
      size_t len = strlen(s);
      if (len == 9 && strncmp(s, OJPH_PN_STRING_BROADCAST, 9) == 0)
        profile = OJPH_PN_BROADCAST;
      else if (len == 3 && strncmp(s, OJPH_PN_STRING_IMF, 3) == 0)
        profile = OJPH_PN_IMF;
      else
        OJPH_ERROR(0x000300A1, "unkownn or unsupported profile");
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::flush()
    {
      si32 repeat = (si32)num_tiles.area();
      for (si32 i = 0; i < repeat; ++i)
        tiles[i].prepare_for_flush();
      if (profile == OJPH_PN_BROADCAST || profile == OJPH_PN_IMF)
      { //write tlm
        for (si32 i = 0; i < repeat; ++i)
          tiles[i].fill_tlm(&tlm);
        tlm.write(outfile);
      }
      for (si32 i = 0; i < repeat; ++i)
        tiles[i].flush(outfile);
      ui16 t = swap_byte(JP2K_MARKER::EOC);
      if (!outfile->write(&t, 2))
        OJPH_ERROR(0x00030071, "Error writing to file");
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::close()
    {
      if (infile)
        infile->close();
      if (outfile)
        outfile->close();
    }

    //////////////////////////////////////////////////////////////////////////
    line_buf* codestream::exchange(line_buf *line, ui32 &next_component)
    {
      if (line)
      {
        bool success = false;
        while (!success)
        {
          success = true;
          for (ui32 i = 0; i < num_tiles.w; ++i)
          {
            ui32 idx = i + cur_tile_row * num_tiles.w;
            if ((success &= tiles[idx].push(line, cur_comp)) == false)
              break;
          }
          cur_tile_row += success == false ? 1 : 0;
          if (cur_tile_row >= num_tiles.h)
            cur_tile_row = 0;
        }

        if (planar) //process one component at a time
        {
          if (++cur_line >= comp_size[cur_comp].h)
          {
            cur_line = 0;
            cur_tile_row = 0;
            if (++cur_comp >= num_comps)
            {
              next_component = 0;
              return NULL;
            }
          }
        }
        else //process all component for a line
        {
          if (++cur_comp >= num_comps)
          {
            cur_comp = 0;
            if (++cur_line >= comp_size[cur_comp].h)
            {
              next_component = 0;
              return NULL;
            }
          }
        }
      }

      next_component = cur_comp;
      return this->lines + cur_comp;
    }

    //////////////////////////////////////////////////////////////////////////
    line_buf* codestream::pull(ui32 &comp_num)
    {
      bool success = false;
      while (!success)
      {
        success = true;
        for (ui32 i = 0; i < num_tiles.w; ++i)
        {
          ui32 idx = i + cur_tile_row * num_tiles.w;
          if ((success &= tiles[idx].pull(lines + cur_comp, cur_comp)) == false)
            break;
        }
        cur_tile_row += success == false ? 1 : 0;
        if (cur_tile_row >= num_tiles.h)
          cur_tile_row = 0;
      }
      comp_num = cur_comp;

      if (planar) //process one component at a time
      {
        if (++cur_line >= recon_comp_size[cur_comp].h)
        {
          cur_line = 0;
          cur_tile_row = 0;
          if (cur_comp++ >= num_comps)
          {
            comp_num = 0;
            return NULL;
          }
        }
      }
      else //process all component for a line
      {
        if (++cur_comp >= num_comps)
        {
          cur_comp = 0;
          if (cur_line++ >= recon_comp_size[cur_comp].h)
          {
            comp_num = 0;
            return NULL;
          }
        }
      }

      return lines + comp_num;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void tile::pre_alloc(codestream *codestream, const rect& tile_rect,
                         const rect& recon_tile_rect)
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

      int profile = codestream->get_profile();
      if (profile == OJPH_PN_IMF || profile == OJPH_PN_BROADCAST )
        allocator->pre_alloc_obj<ui32>(num_comps);  //for num_comp_bytes
      else
        allocator->pre_alloc_obj<ui32>(1);

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
                              const rect& recon_tile_rect, 
                              ui32 tile_idx, ui32 offset)
    {
      //this->parent = codestream;
      mem_fixed_allocator* allocator = codestream->get_allocator();

      sot.init(0, (ui16)tile_idx, 0, 1);
      prog_order = codestream->access_cod().get_progression_order();

      //allocate tiles_comp
      const param_siz *szp = codestream->get_siz();

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
      if (profile == OJPH_PN_IMF || profile == OJPH_PN_BROADCAST )
        num_comp_bytes = allocator->post_alloc_obj<ui32>(num_comps);
      else
        num_comp_bytes = allocator->post_alloc_obj<ui32>(1);

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
      //prepare precinct headers
      if (profile == OJPH_PN_IMF || profile == OJPH_PN_BROADCAST)
        for (ui32 c = 0; c < num_comps; ++c)
          num_comp_bytes[c] = comps[c].prepare_precincts();
      else
      {
        num_comp_bytes[0] = 0;
        for (ui32 c = 0; c < num_comps; ++c)
          num_comp_bytes[0] += comps[c].prepare_precincts();
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void tile::fill_tlm(param_tlm *tlm)
    {
      if (profile == OJPH_PN_IMF || profile == OJPH_PN_BROADCAST)
      {
        for (ui32 c = 0; c < num_comps; ++c)
          tlm->set_next_pair(sot.get_tile_index(), num_comp_bytes[c]);
      }
      else
        tlm->set_next_pair(sot.get_tile_index(), num_comp_bytes[0]);
    }


    //////////////////////////////////////////////////////////////////////////
    void tile::flush(outfile_base *file)
    {
      ui32 max_decompositions = 0;
      for (ui32 c = 0; c < num_comps; ++c)
        max_decompositions = ojph_max(max_decompositions,
          comps[c].get_num_decompositions());

      if (profile != OJPH_PN_IMF && profile != OJPH_PN_BROADCAST)
      {
        //write tile header
        if (!sot.write(file, num_comp_bytes[0]))
          OJPH_ERROR(0x00030081, "Error writing to file");

        //write start of data
        ui16 t = swap_byte(JP2K_MARKER::SOD);
        if (!file->write(&t, 2))
          OJPH_ERROR(0x00030082, "Error writing to file");
      }


      //sequence the writing of precincts according to progression order
      if (prog_order == OJPH_PO_LRCP || prog_order == OJPH_PO_RLCP)
      {
        for (ui32 r = 0; r <= max_decompositions; ++r)
          for (ui32 c = 0; c < num_comps; ++c)
            comps[c].write_precincts(r, file);
      }
      else if (prog_order == OJPH_PO_RPCL)
      {
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
          if (profile == OJPH_PN_IMF || profile == OJPH_PN_BROADCAST)
          {
            //write tile header
            if (!sot.write(file, num_comp_bytes[c], (ui8)c, (ui8)num_comps))
              OJPH_ERROR(0x00030083, "Error writing to file");

            //write start of data
            ui16 t = swap_byte(JP2K_MARKER::SOD);
            if (!file->write(&t, 2))
              OJPH_ERROR(0x00030084, "Error writing to file");
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

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void tile_comp::pre_alloc(codestream *codestream, const rect& comp_rect,
                              const rect& recon_comp_rect)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();

      //allocate a resolution
      ui32 num_decomps = codestream->access_cod().get_num_decompositions();
      allocator->pre_alloc_obj<resolution>(1);

      resolution::pre_alloc(codestream, comp_rect, recon_comp_rect, 
                            num_decomps);
    }

    //////////////////////////////////////////////////////////////////////////
    void tile_comp::finalize_alloc(codestream *codestream, tile *parent,
                                  ui32 comp_num, const rect& comp_rect,
                                  const rect& recon_comp_rect)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();

      //allocate a resolution
      num_decomps = codestream->get_cod()->get_num_decompositions();

      comp_downsamp = codestream->get_siz()->get_downsampling(comp_num);
      this->comp_rect = comp_rect;
      this->parent_tile = parent;

      this->comp_num = comp_num;
      res = allocator->post_alloc_obj<resolution>(1);
      res->finalize_alloc(codestream, comp_rect, recon_comp_rect, comp_num,
                          num_decomps, comp_downsamp, this, NULL);
    }

    //////////////////////////////////////////////////////////////////////////
    line_buf* tile_comp::get_line()
    {
      return res->get_line();
    }

    //////////////////////////////////////////////////////////////////////////
    void tile_comp::push_line()
    {
      res->push_line();
    }

    //////////////////////////////////////////////////////////////////////////
    line_buf* tile_comp::pull_line()
    {
      return res->pull_line();
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 tile_comp::prepare_precincts()
    {
      return res->prepare_precinct();
    }

    //////////////////////////////////////////////////////////////////////////
    void tile_comp::write_precincts(ui32 res_num, outfile_base *file)
    {
      assert(res_num <= num_decomps);
      res_num = num_decomps - res_num; //how many levels to go down
      resolution *r = res;
      while (res_num > 0 && r != NULL)
      {
        r = r->next_resolution();
        --res_num;
      }
      if (r) //resolution does not exist if r is NULL
        r->write_precincts(file);
    }

    //////////////////////////////////////////////////////////////////////////
    bool tile_comp::get_top_left_precinct(ui32 res_num, point &top_left)
    {
      assert(res_num <= num_decomps);
      res_num = num_decomps - res_num;
      resolution *r = res;
      while (res_num > 0 && r != NULL)
      {
        r = r->next_resolution();
        --res_num;
      }
      if (r) //resolution does not exist if r is NULL
        return r->get_top_left_precinct(top_left);
      else
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    void tile_comp::write_one_precinct(ui32 res_num, outfile_base *file)
    {
      assert(res_num <= num_decomps);
      res_num = num_decomps - res_num;
      resolution *r = res;
      while (res_num > 0 && r != NULL)
      {
        r = r->next_resolution();
        --res_num;
      }
      if (r) //resolution does not exist if r is NULL
        r->write_one_precinct(file);
    }

    //////////////////////////////////////////////////////////////////////////
    void tile_comp::parse_precincts(ui32 res_num, ui32& data_left,
                                    infile_base *file)
    {
      assert(res_num <= num_decomps);
      res_num = num_decomps - res_num; //how many levels to go down
      resolution *r = res;
      while (res_num > 0 && r != NULL)
      {
        r = r->next_resolution();
        --res_num;
      }
      if (r) //resolution does not exist if r is NULL
        r->parse_all_precincts(data_left, file);
    }


    //////////////////////////////////////////////////////////////////////////
    void tile_comp::parse_one_precinct(ui32 res_num, ui32& data_left,
                                       infile_base *file)
    {
      assert(res_num <= num_decomps);
      res_num = num_decomps - res_num;
      resolution *r = res;
      while (res_num > 0 && r != NULL)
      {
        r = r->next_resolution();
        --res_num;
      }
      if (r) //resolution does not exist if r is NULL
        r->parse_one_precinct(data_left, file);
    }



    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    static void rotate_buffers(line_buf *line1, line_buf* line2,
                               line_buf *line3, line_buf* line4)
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
    static void rotate_buffers(line_buf *line1, line_buf* line2,
                               line_buf *line3, line_buf* line4,
                               line_buf *line5, line_buf* line6)
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
    void resolution::pre_alloc(codestream *codestream, const rect &res_rect, 
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
          ui32 tbx0 = (trx0 - (i&1) + 1) >> 1;
          ui32 tbx1 = (trx1 - (i&1) + 1) >> 1;
          ui32 tby0 = (try0 - (i>>1) + 1) >> 1;
          ui32 tby1 = (try1 - (i>>1) + 1) >> 1;

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
        num_precincts.w = (trx1 + (1<<log_PP.w) - 1) >> log_PP.w;
        num_precincts.w -= trx0 >> log_PP.w;
        num_precincts.h = (try1 + (1<<log_PP.h) - 1) >> log_PP.h;
        num_precincts.h -= try0 >> log_PP.h;
        allocator->pre_alloc_obj<precinct>(num_precincts.area());
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
    void resolution::finalize_alloc(codestream *codestream, 
                                    const rect& res_rect, 
                                    const rect& recon_res_rect,
                                    ui32 comp_num, ui32 res_num,
                                    point comp_downsamp,
                                    tile_comp *parent_tile_comp,
                                    resolution *parent_res)
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
          ui32 tbx0 = (trx0 - (i&1) + 1) >> 1;
          ui32 tbx1 = (trx1 - (i&1) + 1) >> 1;
          ui32 tby0 = (try0 - (i>>1) + 1) >> 1;
          ui32 tby1 = (try1 - (i>>1) + 1) >> 1;

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
        num_precincts.w = (trx1 + (1<<log_PP.w) - 1) >> log_PP.w;
        num_precincts.w -= trx0 >> log_PP.w;
        num_precincts.h = (try1 + (1<<log_PP.h) - 1) >> log_PP.h;
        num_precincts.h -= try0 >> log_PP.h;
        precincts = allocator->post_alloc_obj<precinct>(num_precincts.area());
        ui64 num = num_precincts.area();
        for (ui64 i = 0; i < num; ++i)
          precincts[i] = precinct();
      }
      // precincts will be initialized in full shortly

      ui32 x_lower_bound = (trx0 >> log_PP.w) << log_PP.w;
      ui32 y_lower_bound = (try0 >> log_PP.h) << log_PP.h;

      point proj_factor;
      proj_factor.x = comp_downsamp.x * (1<<(num_decomps - res_num));
      proj_factor.y = comp_downsamp.y * (1<<(num_decomps - res_num));
      precinct *pp = precincts;

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
      log_PP.w -= (res_num?1:0);
      log_PP.h -= (res_num?1:0);
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
                                       cur_line > 1 ? lines + 2:lines,
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
              si32 *sp = lines[0].i32;
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

        rotate_buffers(lines, lines+1, lines+2, lines+3);

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
              irrev_vert_wvlt_K(lines + 2, lines + 5,
                                false, width);
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
              //push to H
              irrev_horz_wvlt_fwd_tx(lines, bands[2].get_line(),
                bands[3].get_line(), width, horz_even);
              bands[2].push_line();
              bands[3].push_line();
            }
          }
        }

        rotate_buffers(lines, lines+1, lines+2, lines+3, lines+4, lines+5);

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
            rotate_buffers(lines, lines+1, lines+2, lines+3);
            ++cur_line;
          }
          while (cur_line < 3);
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
              si32 *sp = lines[0].i32;
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
                cur_line < res_rect.siz.h     ? lines : lines + 2,
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
            rotate_buffers(lines,lines+1,lines+2,lines+3,lines+4,lines+5);
            ++cur_line;
          }
          while (cur_line < 5);
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
              float *sp = lines[0].f32;
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
      ui32 used_bytes = 0;
      if (res_num != 0)
         used_bytes = child_res->prepare_precinct();

      si32 repeat = (si32)num_precincts.area();
      for (si32 i = 0; i < repeat; ++i)
        used_bytes += precincts[i].prepare_precinct(tag_tree_size,
                                                    level_index, elastic);

      return used_bytes;
    }

    //////////////////////////////////////////////////////////////////////////
    void resolution::write_precincts(outfile_base *file)
    {
      precinct *p = precincts;
      for (si32 i = 0; i < (si32)num_precincts.area(); ++i)
        p[i].write(file);
    }

    //////////////////////////////////////////////////////////////////////////
    bool resolution::get_top_left_precinct(point &top_left)
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
    void resolution::write_one_precinct(outfile_base *file)
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
    void resolution::parse_all_precincts(ui32& data_left, infile_base *file)
    {
      precinct *p = precincts;
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
    void resolution::parse_one_precinct(ui32& data_left, infile_base *file)
    {
      ui32 idx = cur_precinct_loc.x + cur_precinct_loc.y * num_precincts.w;
      assert(idx < num_precincts.area());

      if (data_left == 0)
        return;
      precinct *p = precincts + idx;
      p->parse(tag_tree_size, level_index, elastic, data_left, file,
        skipped_res_for_read);
      if (++cur_precinct_loc.x >= num_precincts.w)
      {
        cur_precinct_loc.x = 0;
        ++cur_precinct_loc.y;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    struct bit_write_buf
    {
      static const int needed;

      bit_write_buf() { ccl = NULL; avail_bits = 0; tmp = 0; }
      coded_lists* ccl;
      int avail_bits;
      ui64 tmp;
    };

    //////////////////////////////////////////////////////////////////////////
    const int bit_write_buf::needed = 512;

    //////////////////////////////////////////////////////////////////////////
    static inline
    void bb_expand_buf(bit_write_buf *bbp, mem_elastic_allocator *elastic,
                       coded_lists*& cur_coded_list)
    {
      assert(cur_coded_list == NULL);
      elastic->get_buffer(bit_write_buf::needed, cur_coded_list);
      bbp->ccl = cur_coded_list;
      bbp->tmp = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void bb_init(bit_write_buf *bbp, mem_elastic_allocator *elastic,
                 coded_lists*& cur_coded_list)
    {
      bb_expand_buf(bbp, elastic, cur_coded_list);
      bbp->avail_bits = 8;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void bb_put_bit(bit_write_buf *bbp, ui32 bit,
                    mem_elastic_allocator *elastic,
                    coded_lists*& cur_coded_list, ui32& ph_bytes)
    {
      --bbp->avail_bits;
      bbp->tmp |= (bit & 1) << bbp->avail_bits;
      if (bbp->avail_bits <= 0)
      {
        bbp->avail_bits = 8 - (bbp->tmp != 0xFF ? 0 : 1);
        bbp->ccl->buf[bbp->ccl->buf_size - bbp->ccl->avail_size] =
          (ui8)(bbp->tmp & 0xFF);
        bbp->tmp = 0;
        --bbp->ccl->avail_size;
        if (bbp->ccl->avail_size == 0)
        {
          bb_expand_buf(bbp, elastic, cur_coded_list->next_list);
          cur_coded_list = cur_coded_list->next_list;
          ph_bytes += bit_write_buf::needed;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void bb_put_bits(bit_write_buf *bbp, ui32 data, int num_bits,
                     mem_elastic_allocator *elastic,
                     coded_lists*& cur_coded_list, ui32& ph_bytes)
    {
//      assert(num_bits <= 32);
      for (int i = num_bits - 1; i >= 0; --i)
        bb_put_bit(bbp, data >> i, elastic, cur_coded_list, ph_bytes);
//      while (num_bits) {
//        int tx_bits = num_bits < bbp->avail_bits ? num_bits : bbp->avail_bits;
//        bbp->tmp |= (data >> (num_bits - tx_bits)) & ((1 << tx_bits) - 1);
//        bbp->avail_bits -= tx_bits;
//        if (bbp->avail_bits <= 0)
//        {
//          bbp->avail_bits = 8 - (bbp->tmp != 0xFF ? 0 : 1);
//          bbp->buf[bbp->buf_size - bbp->avail_size] = (ui8)(bbp->tmp & 0xFF);
//          bbp->tmp = 0;
//          --bbp->avail_size;
//          if (bbp->avail_size == 0)
//          {
//            bb_expand_buf(bbp, elastic, cur_coded_list->next_list);
//            cur_coded_list = cur_coded_list->next_list;
//            ph_bytes += bit_buffer::needed;
//          }
//        }
//      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void bb_terminate(bit_write_buf *bbp)
    {
      if (bbp->avail_bits < 8) //bits have been written
      {
        ui8 val = (ui8)(bbp->tmp & 0xFF);
        bbp->ccl->buf[bbp->ccl->buf_size - bbp->ccl->avail_size] = val;
        --bbp->ccl->avail_size;
      }
    }

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
      int sst = num_bands == 3 ? 1 : 0;
      int send = num_bands == 3 ? 4 : 1;
      int num_skipped_subbands = 0;
      for (int s = sst; s < send; ++s)
      {
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
                bb_put_bits(&bb, 1, num_zeros + 1,
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

      return coded ? cb_bytes + ph_bytes : 1;
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
        int sst = num_bands == 3 ? 1 : 0;
        int send = num_bands == 3 ? 4 : 1;
        for (int s = sst; s < send; ++s)
        {
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
    struct bit_read_buf
    {
      infile_base *file;
      ui32 tmp;
      int avail_bits;
      bool unstuff;
      ui32 bytes_left;
    };

    //////////////////////////////////////////////////////////////////////////
    static inline
    void bb_init(bit_read_buf *bbp, ui32 bytes_left, infile_base* file)
    {
      bbp->avail_bits = 0;
      bbp->file = file;
      bbp->bytes_left = bytes_left;
      bbp->tmp = 0;
      bbp->unstuff = false;
    }

    /////////////////////////////////////////////////////////////////////////////
    static inline
    bool bb_read(bit_read_buf *bbp)
    {
      if (bbp->bytes_left > 0)
      {
        ui32 t = 0;
        if (bbp->file->read(&t, 1) != 1)
          throw "error reading from file";
        bbp->tmp = t;
        bbp->avail_bits = 8 - bbp->unstuff;
        bbp->unstuff = (t == 0xFF);
        --bbp->bytes_left;
        return true;
      }
      else
      {
        bbp->tmp = 0;
        bbp->avail_bits = 8 - bbp->unstuff;
        bbp->unstuff = false;
        return false;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    bool bb_read_bit(bit_read_buf *bbp, ui32& bit)
    {
      bool result = true;
      if (bbp->avail_bits == 0)
        result = bb_read(bbp);
      bit = (bbp->tmp >> --bbp->avail_bits) & 1;
      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    bool bb_read_bits(bit_read_buf *bbp, int num_bits, ui32& bits)
    {
      assert(num_bits <= 32);

      bits = 0;
      bool result = true;
      while (num_bits) {
        if (bbp->avail_bits == 0)
          result = bb_read(bbp);
        int tx_bits = ojph_min(bbp->avail_bits, num_bits);
        bits <<= tx_bits;
        bbp->avail_bits -= tx_bits;
        num_bits -= tx_bits;
        bits |= (bbp->tmp >> bbp->avail_bits) & ((1 << tx_bits) - 1);
      }
      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    bool bb_read_chunk(bit_read_buf *bbp, ui32 num_bytes,
                       coded_lists*& cur_coded_list,
                       mem_elastic_allocator *elastic)
    {
      assert(bbp->avail_bits == 0 && bbp->unstuff == false);
      ui32 bytes = ojph_min(num_bytes, bbp->bytes_left);
      elastic->get_buffer(bytes + coded_cb_header::prefix_buf_size
        + coded_cb_header::suffix_buf_size, cur_coded_list);
      ui32 bytes_read = (ui32)bbp->file->read(
        cur_coded_list->buf + coded_cb_header::prefix_buf_size, bytes);
      if (num_bytes > bytes_read)
        memset(cur_coded_list->buf + coded_cb_header::prefix_buf_size + bytes, 
          0, num_bytes - bytes_read);
      bbp->bytes_left -= bytes_read;
      return bytes_read == bytes;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void bb_skip_eph(bit_read_buf *bbp)
    {
      if (bbp->bytes_left >= 2)
      {
        ui8 marker[2];
        if (bbp->file->read(marker, 2) != 2)
          throw "error reading from file";
        bbp->bytes_left -= 2;
        if ((int)marker[0] != (EPH >> 8) || (int)marker[1] != (EPH & 0xFF))
          throw "should find EPH, but found something else";
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    bool bb_terminate(bit_read_buf *bbp, bool uses_eph)
    {
      bool result = true;
      if (bbp->unstuff)
        result = bb_read(bbp);
      assert(bbp->unstuff == false);
      if (uses_eph)
        bb_skip_eph(bbp);
      bbp->tmp = 0;
      bbp->avail_bits = 0;
      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    bool bb_skip_sop(bit_read_buf *bbp)
    {
      if (bbp->bytes_left >= 2)
      {
        ui8 marker[2];
        if (bbp->file->read(marker, 2) != 2)
          throw "error reading from file";
        if ((int)marker[0] == (SOP >> 8) && (int)marker[1] == (SOP & 0xFF))
        {
          bbp->bytes_left -= 2;
          if (bbp->bytes_left >= 4)
          {
            ui16 com_len;
            if (bbp->file->read(&com_len, 2) != 2)
              throw "error reading from file";
            com_len = swap_byte(com_len);
            if (com_len != 4)
              throw "something is wrong with SOP length";
            int result = 
              bbp->file->seek(com_len - 2, infile_base::OJPH_SEEK_CUR);
            if (result != 0)
              throw "error seeking file";
            bbp->bytes_left -= com_len;
          }
          else
            throw "precinct truncated early";
          return true;
        }
        else
        {
          //put the bytes back
          if (bbp->file->seek(-2, infile_base::OJPH_SEEK_CUR) != 0)
            throw "error seeking file";
          return false;
        }
      }

      return false;
    }

    //////////////////////////////////////////////////////////////////////////
    void precinct::parse(int tag_tree_size, ui32* lev_idx,
                         mem_elastic_allocator *elastic,
                         ui32 &data_left, infile_base *file,
                         bool skipped)
    {
      assert(data_left > 0);
      bit_read_buf bb;
      bb_init(&bb, data_left, file);
      if (may_use_sop)
        bb_skip_sop(&bb);

      int sst = num_bands == 3 ? 1 : 0;
      int send = num_bands == 3 ? 4 : 1;
      bool empty_packet = true;
      for (int s = sst; s < send; ++s)
      {
        if (cb_idxs[s].siz.w == 0 || cb_idxs[s].siz.h == 0)
          continue;

        if (empty_packet) //one bit to check if the packet is empty
        {
          ui32 bit;
          bb_read_bit(&bb, bit);
          if (bit == 0) //empty packet
          { bb_terminate(&bb, uses_eph); data_left = bb.bytes_left; return; }
          empty_packet = false;
        }

        ui32 num_levels = 1 +
          ojph_max(log2ceil(cb_idxs[s].siz.w), log2ceil(cb_idxs[s].siz.h));

        //create quad trees for inclusion and missing msbs
        tag_tree inc_tag, inc_tag_flags, mmsb_tag, mmsb_tag_flags;
        inc_tag.init(scratch, lev_idx, num_levels, cb_idxs[s].siz, 0);
        *inc_tag.get(0, 0, num_levels) = 0;
        inc_tag_flags.init(scratch + tag_tree_size, lev_idx, num_levels,
          cb_idxs[s].siz, 0);
        *inc_tag_flags.get(0, 0, num_levels) = 0;
        mmsb_tag.init(scratch + (tag_tree_size<<1), lev_idx, num_levels,
          cb_idxs[s].siz, 0);
        *mmsb_tag.get(0, 0, num_levels) = 0;
        mmsb_tag_flags.init(scratch + (tag_tree_size<<1) + tag_tree_size,
          lev_idx, num_levels, cb_idxs[s].siz, 0);
        *mmsb_tag_flags.get(0, 0, num_levels) = 0;

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
            bool empty_cb = false;
            for (ui32 cl = num_levels; cl > 0; --cl)
            {
              ui32 cur_lev = cl - 1;
              empty_cb = *inc_tag.get(x>>cur_lev, y>>cur_lev, cur_lev) == 1;
              if (empty_cb)
                break;
              //check received
              if (*inc_tag_flags.get(x>>cur_lev, y>>cur_lev, cur_lev) == 0)
              {
                ui32 bit;
                if (bb_read_bit(&bb, bit) == false)
                { data_left = 0; throw "error reading from file p1"; }
                empty_cb = (bit == 0);
                *inc_tag.get(x>>cur_lev, y>>cur_lev, cur_lev) = (ui8)(1 - bit);
                *inc_tag_flags.get(x>>cur_lev, y>>cur_lev, cur_lev) = 1;
              }
              if (empty_cb)
                break;
            }

            if (empty_cb)
              continue;

            //process missing msbs
            ui32 mmsbs = 0;
            for (ui32 levp1 = num_levels; levp1 > 0; --levp1)
            {
              ui32 cur_lev = levp1 - 1;
              mmsbs = *mmsb_tag.get(x>>levp1, y>>levp1, levp1);
              //check received
              if (*mmsb_tag_flags.get(x>>cur_lev, y>>cur_lev, cur_lev) == 0)
              {
                ui32 bit = 0;
                while (bit == 0)
                {
                  if (bb_read_bit(&bb, bit) == false)
                  { data_left = 0; throw "error reading from file p2"; }
                  mmsbs += 1 - bit;
                }
                *mmsb_tag.get(x>>cur_lev, y>>cur_lev, cur_lev) = (ui8)mmsbs;
                *mmsb_tag_flags.get(x>>cur_lev, y>>cur_lev, cur_lev) = 1;
              }
            }

            if (mmsbs > cp->Kmax)
              throw "error in parsing a tile header; "
              "missing msbs are larger or equal to Kmax. The most likely "
              "cause is a corruption in the bitstream.";
            cp->missing_msbs = mmsbs;

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
            cp->num_passes = num_passes;

            //parse pass lengths
            //for one pass, one length, but for 2 or 3 passes, two lengths
            int extra_bit = cp->num_passes > 2 ? 1 : 0;
            int bits1 = 3;
            bit = 1;
            while (bit)
            {
              if (bb_read_bit(&bb, bit) == false)
              { data_left = 0; throw "error reading from file p8"; }
              bits1 += bit;
            }

            if (bb_read_bits(&bb, bits1, bit) == false)
            { data_left = 0; throw "error reading from file p9"; }
            cp->pass_length[0] = bit;
            if (num_passes > 1)
            {
              if (bb_read_bits(&bb, bits1 + extra_bit, bit) == false)
              { data_left = 0; throw "error reading from file p10"; }
              cp->pass_length[1] = bit;
            }
          }
        }
      }
      bb_terminate(&bb, uses_eph);
      //read codeblock data
      for (int s = sst; s < send; ++s)
      {
        ui32 band_width = bands[s].num_blocks.w;
        ui32 width = cb_idxs[s].siz.w;
        ui32 height = cb_idxs[s].siz.h;
        for (ui32 y = 0; y < height; ++y)
        {
          coded_cb_header *cp = bands[s].coded_cbs;
          cp += cb_idxs[s].org.x + (y + cb_idxs[s].org.y) * band_width;
          for (ui32 x = 0; x < width; ++x, ++cp)
          {
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
                  if (!bb_read_chunk(&bb, num_bytes, cp->next_coded, elastic))
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
      data_left = bb.bytes_left;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void subband::pre_alloc(codestream *codestream, const rect &band_rect,
                            ui32 res_num)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();

      bool empty = ((band_rect.siz.w == 0) || (band_rect.siz.h == 0));
      if (empty)
        return;

      const param_cod* cdp = codestream->get_cod();
      size log_cb = cdp->get_log_block_dims();
      size log_PP = cdp->get_log_precinct_size(res_num);

      ui32 xcb_prime = ojph_min(log_cb.w, log_PP.w - (res_num?1:0));
      ui32 ycb_prime = ojph_min(log_cb.h, log_PP.h - (res_num?1:0));

      size nominal(1 << xcb_prime, 1 << ycb_prime);

      ui32 tbx0 = band_rect.org.x;
      ui32 tby0 = band_rect.org.y;
      ui32 tbx1 = band_rect.org.x + band_rect.siz.w;
      ui32 tby1 = band_rect.org.y + band_rect.siz.h;

      size num_blocks;
      num_blocks.w = (tbx1 + (1 << xcb_prime) - 1) >> xcb_prime;
      num_blocks.w -= tbx0 >> xcb_prime;
      num_blocks.h = (tby1 + (1 << ycb_prime) - 1) >> ycb_prime;
      num_blocks.h -= tby0 >> ycb_prime;

      allocator->pre_alloc_obj<codeblock>(num_blocks.w);
      //allocate codeblock headers
      allocator->pre_alloc_obj<coded_cb_header>(num_blocks.area());

      for (ui32 i = 0; i < num_blocks.w; ++i)
        codeblock::pre_alloc(codestream, nominal);

      //allocate lines
      allocator->pre_alloc_obj<line_buf>(1);
      //allocate line_buf
      ui32 width = band_rect.siz.w + 1;
      allocator->pre_alloc_data<si32>(width, 1);
    }

    //////////////////////////////////////////////////////////////////////////
    void subband::finalize_alloc(codestream *codestream,
                                 const rect &band_rect,
                                 resolution* res, ui32 res_num,
                                 ui32 subband_num)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();
      elastic = codestream->get_elastic_alloc();

      this->res_num = res_num;
      this->band_num = subband_num;
      this->band_rect = band_rect;
      this->parent = res;

      const param_cod* cdp = codestream->get_cod();
      this->reversible = cdp->is_reversible();
      size log_cb = cdp->get_log_block_dims();
      log_PP = cdp->get_log_precinct_size(res_num);

      xcb_prime = ojph_min(log_cb.w, log_PP.w - (res_num?1:0));
      ycb_prime = ojph_min(log_cb.h, log_PP.h - (res_num?1:0));

      size nominal(1 << xcb_prime, 1 << ycb_prime);

      cur_cb_row = 0;
      cur_line = 0;
      cur_cb_height = 0;
      param_qcd *qcd = codestream->access_qcd(parent->get_comp_num());
      this->K_max = qcd->get_Kmax(this->res_num, band_num);
      if (!reversible)
      {
        float d = qcd->irrev_get_delta(res_num, subband_num);
        d /= (float)(1u << (31 - this->K_max));
        delta = d;
        delta_inv = (1.0f/d);
      }

      this->empty = ((band_rect.siz.w == 0) || (band_rect.siz.h == 0));
      if (this->empty)
        return;

      ui32 tbx0 = band_rect.org.x;
      ui32 tby0 = band_rect.org.y;
      ui32 tbx1 = band_rect.org.x + band_rect.siz.w;
      ui32 tby1 = band_rect.org.y + band_rect.siz.h;

      num_blocks = size();
      num_blocks.w = (tbx1 + (1 << xcb_prime) - 1) >> xcb_prime;
      num_blocks.w -= tbx0 >> xcb_prime;
      num_blocks.h = (tby1 + (1 << ycb_prime) - 1) >> ycb_prime;
      num_blocks.h -= tby0 >> ycb_prime;

      blocks = allocator->post_alloc_obj<codeblock>(num_blocks.w);
      //allocate codeblock headers
      coded_cb_header *cp = coded_cbs =
        allocator->post_alloc_obj<coded_cb_header>(num_blocks.area());
      memset(coded_cbs, 0, sizeof(coded_cb_header) * num_blocks.area());
      for (int i = (int)num_blocks.area(); i > 0; --i, ++cp)
        cp->Kmax = K_max;

      ui32 x_lower_bound = (tbx0 >> xcb_prime) << xcb_prime;
      ui32 y_lower_bound = (tby0 >> ycb_prime) << ycb_prime;

      size cb_size;
      cb_size.h = ojph_min(tby1, y_lower_bound + nominal.h) - tby0;
      cur_cb_height = (si32)cb_size.h;
      int line_offset = 0;
      for (ui32 i = 0; i < num_blocks.w; ++i)
      {
        ui32 cbx0 = ojph_max(tbx0, x_lower_bound + i * nominal.w);
        ui32 cbx1 = ojph_min(tbx1, x_lower_bound + (i + 1) * nominal.w);
        cb_size.w = cbx1 - cbx0;
        blocks[i].finalize_alloc(codestream, this, nominal, cb_size,
                                  coded_cbs + i, K_max, line_offset);
        line_offset += cb_size.w;
      }

      //allocate lines
      lines = allocator->post_alloc_obj<line_buf>(1);
      //allocate line_buf
      ui32 width = band_rect.siz.w + 1;
      lines->wrap(allocator->post_alloc_data<si32>(width,1),width,1);
    }

    //////////////////////////////////////////////////////////////////////////
    void subband::get_cb_indices(const size& num_precincts,
                                 precinct *precincts)
    {
      if (empty)
        return;

      rect res_rect = parent->get_rect();
      ui32 trx0 = res_rect.org.x;
      ui32 try0 = res_rect.org.y;
      ui32 trx1 = res_rect.org.x + res_rect.siz.w;
      ui32 try1 = res_rect.org.y + res_rect.siz.h;

      ui32 pc_lft = (res_rect.org.x >> log_PP.w) << log_PP.w;
      ui32 pc_top = (res_rect.org.y >> log_PP.h) << log_PP.h;

      ui32 pcx0, pcx1, pcy0, pcy1, shift = (band_num != 0 ? 1 : 0);
      ui32 yb, xb, coly = 0, colx = 0;
      for (ui32 y = 0; y < num_precincts.h; ++y)
      {
        pcy0 = ojph_max(try0, pc_top + (y << log_PP.h));
        pcy1 = ojph_min(try1, pc_top + ((y + 1) << log_PP.h));
        pcy0 = (pcy0 - (band_num >> 1) + (1<<shift) - 1) >> shift;
        pcy1 = (pcy1 - (band_num >> 1) + (1<<shift) - 1) >> shift;

        precinct *p = precincts + y * num_precincts.w;
        yb = ((pcy1 + (1<<ycb_prime) - 1) >> ycb_prime);
        yb -= (pcy0 >> ycb_prime);
        colx = 0;

        for (ui32 x = 0; x < num_precincts.w; ++x, ++p)
        {
          pcx0 = ojph_max(trx0, pc_lft + (x << log_PP.w));
          pcx1 = ojph_min(trx1, pc_lft + ((x + 1) << log_PP.w));
          pcx0 = (pcx0 - (band_num & 1) + (1<<shift) - 1) >> shift;
          pcx1 = (pcx1 - (band_num & 1) + (1<<shift) - 1) >> shift;

          rect *bp = p->cb_idxs + band_num;
          xb = ((pcx1 + (1<<xcb_prime) - 1) >> xcb_prime);
          xb -= (pcx0 >> xcb_prime);

          bp->org.x = colx;
          bp->org.y = coly;
          bp->siz.w = xb;
          bp->siz.h = yb;

          colx += xb;
        }
        coly += yb;
      }
      assert(colx == num_blocks.w && coly == num_blocks.h);
    }

    //////////////////////////////////////////////////////////////////////////
    void subband::exchange_buf(line_buf *l)
    {
      if (empty)
        return;

      assert(l->pre_size == lines[0].pre_size && l->size == lines[0].size);
      si32* t = lines[0].i32;
      lines[0].i32 = l->i32;
      l->i32 = t;
    }

    //////////////////////////////////////////////////////////////////////////
    void subband::push_line()
    {
      if (empty)
        return;

      //push to codeblocks
      for (ui32 i = 0; i < num_blocks.w; ++i)
        blocks[i].push(lines + 0);
      if (++cur_line >= cur_cb_height)
      {
        for (ui32 i = 0; i < num_blocks.w; ++i)
          blocks[i].encode(elastic);

        if (++cur_cb_row < num_blocks.h)
        {
          cur_line = 0;

          ui32 tbx0 = band_rect.org.x;
          ui32 tby0 = band_rect.org.y;
          ui32 tbx1 = band_rect.org.x + band_rect.siz.w;
          ui32 tby1 = band_rect.org.y + band_rect.siz.h;
          size nominal(1 << xcb_prime, 1 << ycb_prime);

          ui32 x_lower_bound = (tbx0 >> xcb_prime) << xcb_prime;
          ui32 y_lower_bound = (tby0 >> ycb_prime) << ycb_prime;
          ui32 cby0 = y_lower_bound + cur_cb_row * nominal.h;
          ui32 cby1 = ojph_min(tby1, cby0 + nominal.h);

          size cb_size;
          cb_size.h = cby1 - ojph_max(tby0, cby0);
          cur_cb_height = (int)cb_size.h;
          for (ui32 i = 0; i < num_blocks.w; ++i)
          {
            ui32 cbx0 = ojph_max(tbx0, x_lower_bound + i * nominal.w);
            ui32 cbx1 = ojph_min(tbx1, x_lower_bound + (i + 1) * nominal.w);
            cb_size.w = cbx1 - cbx0;
            blocks[i].recreate(cb_size,
                               coded_cbs + i + cur_cb_row * num_blocks.w);
          }
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    line_buf *subband::pull_line()
    {
      if (empty)
        return lines;

      //pull from codeblocks
      if (--cur_line <= 0)
      {
        if (cur_cb_row < num_blocks.h)
        {
          ui32 tbx0 = band_rect.org.x;
          ui32 tby0 = band_rect.org.y;
          ui32 tbx1 = band_rect.org.x + band_rect.siz.w;
          ui32 tby1 = band_rect.org.y + band_rect.siz.h;
          size nominal(1 << xcb_prime, 1 << ycb_prime);

          ui32 x_lower_bound = (tbx0 >> xcb_prime) << xcb_prime;
          ui32 y_lower_bound = (tby0 >> ycb_prime) << ycb_prime;
          ui32 cby0 = ojph_max(tby0, y_lower_bound + cur_cb_row * nominal.h);
          ui32 cby1 = ojph_min(tby1, y_lower_bound+(cur_cb_row+1)*nominal.h);

          size cb_size;
          cb_size.h = cby1 - cby0;
          cur_line = cur_cb_height = (int)cb_size.h;
          for (ui32 i = 0; i < num_blocks.w; ++i)
          {
            ui32 cbx0 = ojph_max(tbx0, x_lower_bound + i * nominal.w);
            ui32 cbx1 = ojph_min(tbx1, x_lower_bound + (i + 1) * nominal.w);
            cb_size.w = cbx1 - cbx0;
            blocks[i].recreate(cb_size,
                               coded_cbs + i + cur_cb_row * num_blocks.w);
            blocks[i].decode();
          }
          ++cur_cb_row;
        }
      }

      assert(cur_line >= 0);

      //pull from codeblocks
      for (ui32 i = 0; i < num_blocks.w; ++i)
        blocks[i].pull_line(lines + 0);

      return lines;
    }


    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    codeblock::cb_decoder_fun codeblock::decode_cb = NULL;

    //////////////////////////////////////////////////////////////////////////
    void codeblock::pre_alloc(codestream *codestream,
                              const size& nominal)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();
      
      ui32 stride = (nominal.w + 7) & ~7U; // a multiple of 8
      allocator->pre_alloc_data<ui32>(nominal.h * stride, 0);
    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::finalize_alloc(codestream *codestream,
                                   subband *parent, const size& nominal,
                                   const size& cb_size,
                                   coded_cb_header* coded_cb,
                                   ui32 K_max, int line_offset)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();

      this->stride = (nominal.w + 7) & ~7U; // a multiple of 8
      this->buf_size = this->stride * nominal.h;
      this->buf = allocator->post_alloc_data<ui32>(this->buf_size, 0);

      this->nominal_size = nominal;
      this->cb_size = cb_size;
      this->parent = parent;
      this->line_offset = line_offset;
      this->cur_line = 0;
      this->delta = parent->get_delta();
      this->delta_inv = 1.0f / this->delta;
      this->K_max = K_max;
      for (int i = 0; i < 8; ++i)
        this->max_val[i] = 0;
      ojph::param_cod cod = codestream->access_cod();
      this->reversible = cod.is_reversible();
      this->resilient = codestream->is_resilient();
      this->stripe_causal = cod.get_block_vertical_causality();
      this->zero_block = false;
      this->coded_cb = coded_cb;

#if !defined(OJPH_ENABLE_WASM_SIMD) || !defined(OJPH_EMSCRIPTEN)

      decode_cb = ojph_decode_codeblock;
      find_max_val = codeblock::gen_find_max_val;
      mem_clear = codeblock::gen_mem_clear;
      if (reversible) {
        tx_to_cb = codeblock::gen_rev_tx_to_cb;
        tx_from_cb = codeblock::gen_rev_tx_from_cb;
      }
      else
      {
        tx_to_cb = codeblock::gen_irv_tx_to_cb;
        tx_from_cb = codeblock::gen_irv_tx_from_cb;
      }

#ifndef OJPH_DISABLE_INTEL_SIMD

      if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_SSE)
        mem_clear = sse_mem_clear;

      if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_SSE2) {
        find_max_val = sse2_find_max_val;
        if (reversible) {
          tx_to_cb = sse2_rev_tx_to_cb;
          tx_from_cb = sse2_rev_tx_from_cb;
        }
        else {
          tx_to_cb = sse2_irv_tx_to_cb;
          tx_from_cb = sse2_irv_tx_from_cb;
        }
      }

      if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_SSSE3)
        decode_cb = ojph_decode_codeblock_ssse3;


      if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_AVX)
        mem_clear = avx_mem_clear;

      if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_AVX2) {
        find_max_val = avx2_find_max_val;
        if (reversible) {
          tx_to_cb = avx2_rev_tx_to_cb;
          tx_from_cb = avx2_rev_tx_from_cb;
        }
        else {
          tx_to_cb = avx2_irv_tx_to_cb;
          tx_from_cb = avx2_irv_tx_from_cb;
        }
      }

#endif // !OJPH_DISABLE_INTEL_SIMD

#else // OJPH_ENABLE_WASM_SIMD

      decode_cb = ojph_decode_codeblock_wasm;
      find_max_val = wasm_find_max_val;
      mem_clear = wasm_mem_clear;
      if (reversible) {
        tx_to_cb = wasm_rev_tx_to_cb;
        tx_from_cb = wasm_rev_tx_from_cb;
      }
      else {
        tx_to_cb = wasm_irv_tx_to_cb;
        tx_from_cb = wasm_irv_tx_from_cb;
      }

#endif // !OJPH_ENABLE_WASM_SIMD

    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::push(line_buf *line)
    {
      // convert to sign and magnitude and keep max_val
      const si32 *sp = line->i32 + line_offset;
      ui32 *dp = buf + cur_line * stride;
      tx_to_cb(sp, dp, K_max, delta_inv, cb_size.w, max_val);
      ++cur_line;
    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::encode(mem_elastic_allocator *elastic)
    {
      ui32 mv = find_max_val(max_val);
      if (mv >= 1u<<(31 - K_max))
      {
        coded_cb->missing_msbs = K_max - 1;
        assert(coded_cb->missing_msbs > 0);
        assert(coded_cb->missing_msbs < K_max);
        coded_cb->num_passes = 1;
        
        ojph_encode_codeblock(buf, K_max-1, 1,
          cb_size.w, cb_size.h, stride, coded_cb->pass_length,
          elastic, coded_cb->next_coded);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::recreate(const size &cb_size, coded_cb_header* coded_cb)
    {
      assert(cb_size.h * stride <= buf_size && cb_size.w <= stride);
      this->cb_size = cb_size;
      this->coded_cb = coded_cb;
      this->cur_line = 0;
      for (int i = 0; i < 8; ++i)
        this->max_val[i] = 0;
      this->zero_block = false;
    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::decode()
    {
      if (coded_cb->pass_length[0] > 0 && coded_cb->num_passes > 0 &&
          coded_cb->next_coded != NULL)
      {
        bool result = decode_cb(
            coded_cb->next_coded->buf + coded_cb_header::prefix_buf_size,
            buf, coded_cb->missing_msbs, coded_cb->num_passes,
            coded_cb->pass_length[0], coded_cb->pass_length[1],
            cb_size.w, cb_size.h, stride, stripe_causal);

        if (result == false)
          {
            if (resilient == true)
              zero_block = true;
            else
              OJPH_ERROR(0x000300A1, "Error decoding a codeblock\n");
          }
      }
      else
        zero_block = true;
    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::gen_mem_clear(void* addr, size_t count)
    {
      ui32* p = (ui32*)addr;
      for (size_t i = 0; i < count; i += 4, p += 1)
        *p = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::gen_rev_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max, 
                                     float delta_inv, ui32 count, 
                                     ui32* max_val)
    {
      ojph_unused(delta_inv);
      ui32 shift = 31 - K_max;
      // convert to sign and magnitude and keep max_val
      ui32 tmax = *max_val;
      si32 *p = (si32*)sp;
      for (ui32 i = count; i > 0; --i)
      {
        si32 v = *p++;
        ui32 sign = v >= 0 ? 0 : 0x80000000;
        ui32 val = (ui32)(v >= 0 ? v : -v);
        val <<= shift;
        *dp++ = sign | val;
        tmax |= val; // it is more efficient to use or than max
      }
      *max_val = tmax;
    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::gen_irv_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                                     float delta_inv, ui32 count, 
                                     ui32* max_val)
    {
      ojph_unused(K_max);
      //quantize and convert to sign and magnitude and keep max_val
      ui32 tmax = *max_val;
      float *p = (float*)sp;
      for (ui32 i = count; i > 0; --i)
      {
        float v = *p++;
        si32 t = ojph_trunc(v * delta_inv);
        ui32 sign = t >= 0 ? 0 : 0x80000000;
        ui32 val = (ui32)(t >= 0 ? t : -t);
        *dp++ = sign | val;
        tmax |= val; // it is more efficient to use or than max
      }
      *max_val = tmax;
    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::gen_rev_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                                       float delta, ui32 count)
    {
      ojph_unused(delta);
      ui32 shift = 31 - K_max;
      //convert to sign and magnitude
      si32 *p = (si32*)dp;
      for (ui32 i = count; i > 0; --i)
      {
        ui32 v = *sp++;
        si32 val = (v & 0x7FFFFFFF) >> shift;
        *p++ = (v & 0x80000000) ? -val : val;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::gen_irv_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                                       float delta, ui32 count)
    {
      ojph_unused(K_max);
      //convert to sign and magnitude
      float *p = (float*)dp;
      for (ui32 i = count; i > 0; --i)
      {
        ui32 v = *sp++;
        float val = (float)(v & 0x7FFFFFFF) * delta;
        *p++ = (v & 0x80000000) ? -val : val;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::pull_line(line_buf *line)
    {
      si32 *dp = line->i32 + line_offset;
      if (!zero_block)
      {
        //convert to sign and magnitude
        const ui32 *sp = buf + cur_line * stride;
        tx_from_cb(sp, dp, K_max, delta, cb_size.w);
      }
      else
        mem_clear(dp, cb_size.w * sizeof(*dp));
      ++cur_line;
      assert(cur_line <= cb_size.h);
    }

  }
}
