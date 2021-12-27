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
// File: ojph_params.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/

#define _USE_MATH_DEFINES
#include <cmath>

#include "ojph_base.h"
#include "ojph_file.h"
#include "ojph_params.h"

#include "ojph_params_local.h"
#include "ojph_message.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void param_siz::set_image_extent(point dims)
  {
    state->Xsiz = dims.x;
    state->Ysiz = dims.y;
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_siz::set_tile_size(size s)
  {
    state->XTsiz = s.w;
    state->YTsiz = s.h;
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_siz::set_image_offset(point offset)
  { // WARNING need to check if these are valid
    state->XOsiz = offset.x;
    state->YOsiz = offset.y;
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_siz::set_tile_offset(point offset)
  { // WARNING need to check if these are valid
    state->XTOsiz = offset.x;
    state->YTOsiz = offset.y;
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_siz::set_num_components(ui32 num_comps)
  {
    state->set_num_components(num_comps);
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_siz::set_component(ui32 comp_num, const point& downsampling,
                                ui32 bit_depth, bool is_signed)
  {
    state->set_comp_info(comp_num, downsampling, bit_depth, is_signed);
  }

  ////////////////////////////////////////////////////////////////////////////
  point param_siz::get_image_extent() const
  {
    return point(state->Xsiz, state->Ysiz);
  }

  ////////////////////////////////////////////////////////////////////////////
  point param_siz::get_image_offset() const
  {
    return point(state->XOsiz, state->YOsiz);
  }

  ////////////////////////////////////////////////////////////////////////////
  size param_siz::get_tile_size() const
  {
    return size(state->XTsiz, state->YTsiz);
  }

  ////////////////////////////////////////////////////////////////////////////
  point param_siz::get_tile_offset() const
  {
    return point(state->XTOsiz, state->YTOsiz);
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 param_siz::get_num_components() const
  {
    return state->Csiz;
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 param_siz::get_bit_depth(ui32 comp_num) const
  {
    return state->get_bit_depth(comp_num);
  }

  ////////////////////////////////////////////////////////////////////////////
  bool param_siz::is_signed(ui32 comp_num) const
  {
    return state->is_signed(comp_num);
  }

  ////////////////////////////////////////////////////////////////////////////
  point param_siz::get_downsampling(ui32 comp_num) const
  {
    return state->get_downsampling(comp_num);
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 param_siz::get_recon_width(ui32 comp_num) const
  {
    return state->get_recon_width(comp_num);
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 param_siz::get_recon_height(ui32 comp_num) const
  {
    return state->get_recon_height(comp_num);
  }

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void param_cod::set_num_decomposition(ui32 num_decompositions)
  {
    if (num_decompositions > 32)
      OJPH_ERROR(0x00050001,
        "maximum number of decompositions cannot exceed 32");
    state->SPcod.num_decomp = (ui8)num_decompositions;
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_cod::set_block_dims(ui32 width, ui32 height)
  {
    ui32 log_width = 31 - count_leading_zeros(width);
    ui32 log_height = 31 - count_leading_zeros(height);
    if (width == 0 || width != (1u << log_width)
      || height == 0 || height != (1u << log_height)
      || log_width < 2 || log_height < 2
      || log_width + log_height > 12)
      OJPH_ERROR(0x00050011, "incorrect code block dimensions");
    state->SPcod.block_width = (ui8)(log_width - 2);
    state->SPcod.block_height = (ui8)(log_height - 2);
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_cod::set_precinct_size(int num_levels, size* precinct_size)
  {
    if (num_levels == 0 || precinct_size == NULL)
      state->Scod &= 0xFE;
    else
    {
      state->Scod |= 1;
      for (int i = 0; i <= state->SPcod.num_decomp; ++i)
      {
        size t = precinct_size[i < num_levels ? i : num_levels - 1];

        ui32 PPx = 31 - count_leading_zeros(t.w);
        ui32 PPy = 31 - count_leading_zeros(t.h);
        if (t.w == 0 || t.h == 0)
          OJPH_ERROR(0x00050021, "precinct width or height cannot be 0");
        if (t.w != (1u<<PPx) || t.h != (1u<<PPy))
          OJPH_ERROR(0x00050022,
            "precinct width and height should be a power of 2");
        if (PPx > 15 || PPy > 15)
          OJPH_ERROR(0x00050023, "precinct size is too large");
        if (i > 0 && (PPx == 0 || PPy == 0))
          OJPH_ERROR(0x00050024, "precinct size is too small");
        state->SPcod.precinct_size[i] = (ui8)(PPx | (PPy << 4));
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_cod::set_progression_order(const char *name)
  {
    int prog_order = 0;
    size_t len = strlen(name);
    if (len == 4)
    {
      if (strncmp(name, OJPH_PO_STRING_LRCP, 4) == 0)
        prog_order = OJPH_PO_LRCP;
      else if (strncmp(name, OJPH_PO_STRING_RLCP, 4) == 0)
        prog_order = OJPH_PO_RLCP;
      else if (strncmp(name, OJPH_PO_STRING_RPCL, 4) == 0)
        prog_order = OJPH_PO_RPCL;
      else if (strncmp(name, OJPH_PO_STRING_PCRL, 4) == 0)
        prog_order = OJPH_PO_PCRL;
      else if (strncmp(name, OJPH_PO_STRING_CPRL, 4) == 0)
        prog_order = OJPH_PO_CPRL;
      else
        OJPH_ERROR(0x00050031, "unknown progression order");
    }
    else
      OJPH_ERROR(0x00050032, "improper progression order");


    state->SGCod.prog_order = (ui8)prog_order;
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_cod::set_color_transform(bool color_transform)
  {
    state->employ_color_transform(color_transform ? 1 : 0);
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_cod::set_reversible(bool reversible)
  {
    state->set_reversible(reversible);
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 param_cod::get_num_decompositions() const
  {
    return state->get_num_decompositions();
  }

  ////////////////////////////////////////////////////////////////////////////
  size param_cod::get_block_dims() const
  {
    return state->get_block_dims();
  }

  ////////////////////////////////////////////////////////////////////////////
  size param_cod::get_log_block_dims() const
  {
    return state->get_log_block_dims();
  }

  ////////////////////////////////////////////////////////////////////////////
  bool param_cod::is_reversible() const
  {
    return state->is_reversible();
  }

  ////////////////////////////////////////////////////////////////////////////
  size param_cod::get_precinct_size(ui32 level_num) const
  {
    return state->get_precinct_size(level_num);
  }

  ////////////////////////////////////////////////////////////////////////////
  size param_cod::get_log_precinct_size(ui32 level_num) const
  {
    return state->get_log_precinct_size(level_num);
  }

  ////////////////////////////////////////////////////////////////////////////
  int param_cod::get_progression_order() const
  {
    return state->SGCod.prog_order;
  }

  ////////////////////////////////////////////////////////////////////////////
  const char* param_cod::get_progression_order_as_string() const
  {
    if (state->SGCod.prog_order == OJPH_PO_LRCP)
      return OJPH_PO_STRING_LRCP;
    else if (state->SGCod.prog_order == OJPH_PO_RLCP)
      return OJPH_PO_STRING_RLCP;
    else if (state->SGCod.prog_order == OJPH_PO_RPCL)
      return OJPH_PO_STRING_RPCL;
    else if (state->SGCod.prog_order == OJPH_PO_PCRL)
      return OJPH_PO_STRING_PCRL;
    else if (state->SGCod.prog_order == OJPH_PO_CPRL)
      return OJPH_PO_STRING_CPRL;
    else
      assert(0);
    return "";
  }

  ////////////////////////////////////////////////////////////////////////////
  int param_cod::get_num_layers() const
  {
    return state->SGCod.num_layers;
  }

  ////////////////////////////////////////////////////////////////////////////
  bool param_cod::is_using_color_transform() const
  {
    return state->is_employing_color_transform();
  }

  ////////////////////////////////////////////////////////////////////////////
  bool param_cod::packets_may_use_sop() const
  {
    return state->packets_may_use_sop();
  }

  ////////////////////////////////////////////////////////////////////////////
  bool param_cod::packets_use_eph() const
  {
    return state->packets_use_eph();
  }

  ////////////////////////////////////////////////////////////////////////////
  bool param_cod::get_block_vertical_causality() const
  {
    return (state->SPcod.block_style & local::param_cod::VERT_CAUSAL_MODE)!=0;
  }


  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////
  void param_qcd::set_irrev_quant(float delta)
  {
    state->set_delta(delta);
  }

  //////////////////////////////////////////////////////////////////////////
  //
  //
  //                                LOCAL
  //
  //
  //////////////////////////////////////////////////////////////////////////

  namespace local {

    //////////////////////////////////////////////////////////////////////////
    static inline
    ui16 swap_byte(ui16 t)
    {
      return (ui16)((t << 8) | (t >> 8));
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    ui32 swap_byte(ui32 t)
    {
      ui32 u = swap_byte((ui16)(t & 0xFFFFu));
      u <<= 16;
      u |= swap_byte((ui16)(t >> 16));
      return u;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //static
    class sqrt_energy_gains
    {
    public:
      static float get_gain_l(ui32 num_decomp, bool reversible)
      { return reversible ? gain_5x3_l[num_decomp] : gain_9x7_l[num_decomp]; }
      static float get_gain_h(ui32 num_decomp, bool reversible)
      { return reversible ? gain_5x3_h[num_decomp] : gain_9x7_h[num_decomp]; }

    private:
      static const float gain_9x7_l[34];
      static const float gain_9x7_h[34];
      static const float gain_5x3_l[34];
      static const float gain_5x3_h[34];
    };

    //////////////////////////////////////////////////////////////////////////
    const float sqrt_energy_gains::gain_9x7_l[34] = { 1.0000e+00f,
      1.4021e+00f, 2.0304e+00f, 2.9012e+00f, 4.1153e+00f, 5.8245e+00f,
      8.2388e+00f, 1.1652e+01f, 1.6479e+01f, 2.3304e+01f, 3.2957e+01f,
      4.6609e+01f, 6.5915e+01f, 9.3217e+01f, 1.3183e+02f, 1.8643e+02f,
      2.6366e+02f, 3.7287e+02f, 5.2732e+02f, 7.4574e+02f, 1.0546e+03f,
      1.4915e+03f, 2.1093e+03f, 2.9830e+03f, 4.2185e+03f, 5.9659e+03f,
      8.4371e+03f, 1.1932e+04f, 1.6874e+04f, 2.3864e+04f, 3.3748e+04f,
      4.7727e+04f, 6.7496e+04f, 9.5454e+04f };
    const float sqrt_energy_gains::gain_9x7_h[34] = { 1.4425e+00f,
      1.9669e+00f, 2.8839e+00f, 4.1475e+00f, 5.8946e+00f, 8.3472e+00f,
      1.1809e+01f, 1.6701e+01f, 2.3620e+01f, 3.3403e+01f, 4.7240e+01f,
      6.6807e+01f, 9.4479e+01f, 1.3361e+02f, 1.8896e+02f, 2.6723e+02f,
      3.7792e+02f, 5.3446e+02f, 7.5583e+02f, 1.0689e+03f, 1.5117e+03f,
      2.1378e+03f, 3.0233e+03f, 4.2756e+03f, 6.0467e+03f, 8.5513e+03f,
      1.2093e+04f, 1.7103e+04f, 2.4187e+04f, 3.4205e+04f, 4.8373e+04f,
      6.8410e+04f, 9.6747e+04f, 1.3682e+05f };
    const float sqrt_energy_gains::gain_5x3_l[34] = { 1.0000e+00f,
      1.2247e+00f, 1.3229e+00f, 1.5411e+00f, 1.7139e+00f, 1.9605e+00f,
      2.2044e+00f, 2.5047e+00f, 2.8277e+00f, 3.2049e+00f, 3.6238e+00f,
      4.1033e+00f, 4.6423e+00f, 5.2548e+00f, 5.9462e+00f, 6.7299e+00f,
      7.6159e+00f, 8.6193e+00f, 9.7544e+00f, 1.1039e+01f, 1.2493e+01f,
      1.4139e+01f, 1.6001e+01f, 1.8108e+01f, 2.0493e+01f, 2.3192e+01f,
      2.6246e+01f, 2.9702e+01f, 3.3614e+01f, 3.8041e+01f, 4.3051e+01f,
      4.8721e+01f, 5.5138e+01f, 6.2399e+01f };
    const float sqrt_energy_gains::gain_5x3_h[34] = { 1.0458e+00f,
      1.3975e+00f, 1.4389e+00f, 1.7287e+00f, 1.8880e+00f, 2.1841e+00f,
      2.4392e+00f, 2.7830e+00f, 3.1341e+00f, 3.5576e+00f, 4.0188e+00f,
      4.5532e+00f, 5.1494e+00f, 5.8301e+00f, 6.5963e+00f, 7.4663e+00f,
      8.4489e+00f, 9.5623e+00f, 1.0821e+01f, 1.2247e+01f, 1.3860e+01f,
      1.5685e+01f, 1.7751e+01f, 2.0089e+01f, 2.2735e+01f, 2.5729e+01f,
      2.9117e+01f, 3.2952e+01f, 3.7292e+01f, 4.2203e+01f, 4.7761e+01f,
      5.4051e+01f, 6.1170e+01f, 6.9226e+01f };

    //////////////////////////////////////////////////////////////////////////
    //static
    class bibo_gains
    {
    public:
      static float get_bibo_gain_l(ui32 num_decomp, bool reversible)
      { return reversible ? gain_5x3_l[num_decomp] : gain_9x7_l[num_decomp]; }
      static float get_bibo_gain_h(ui32 num_decomp, bool reversible)
      { return reversible ? gain_5x3_h[num_decomp] : gain_9x7_h[num_decomp]; }

    private:
      static const float gain_9x7_l[34];
      static const float gain_9x7_h[34];
      static const float gain_5x3_l[34];
      static const float gain_5x3_h[34];
    };

    //////////////////////////////////////////////////////////////////////////
    const float bibo_gains::gain_9x7_l[34] = { 1.0000e+00f, 1.3803e+00f,
      1.3328e+00f, 1.3067e+00f, 1.3028e+00f, 1.3001e+00f, 1.2993e+00f,
      1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f,
      1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f,
      1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f,
      1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f,
      1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f,
      1.2992e+00f, 1.2992e+00f };
    const float bibo_gains::gain_9x7_h[34] = { 1.2976e+00f, 1.3126e+00f,
      1.2757e+00f, 1.2352e+00f, 1.2312e+00f, 1.2285e+00f, 1.2280e+00f,
      1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f,
      1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f,
      1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f,
      1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f,
      1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f,
      1.2278e+00f, 1.2278e+00f };
    const float bibo_gains::gain_5x3_l[34] = { 1.0000e+00f, 1.5000e+00f,
      1.6250e+00f, 1.6875e+00f, 1.6963e+00f, 1.7067e+00f, 1.7116e+00f,
      1.7129e+00f, 1.7141e+00f, 1.7145e+00f, 1.7151e+00f, 1.7152e+00f,
      1.7155e+00f, 1.7155e+00f, 1.7156e+00f, 1.7156e+00f, 1.7156e+00f,
      1.7156e+00f, 1.7156e+00f, 1.7156e+00f, 1.7156e+00f, 1.7156e+00f,
      1.7156e+00f, 1.7156e+00f, 1.7156e+00f, 1.7156e+00f, 1.7156e+00f,
      1.7156e+00f, 1.7156e+00f, 1.7156e+00f, 1.7156e+00f, 1.7156e+00f,
      1.7156e+00f, 1.7156e+00f };
    const float bibo_gains::gain_5x3_h[34] = { 2.0000e+00f, 2.5000e+00f,
      2.7500e+00f, 2.8047e+00f, 2.8198e+00f, 2.8410e+00f, 2.8558e+00f,
      2.8601e+00f, 2.8628e+00f, 2.8656e+00f, 2.8662e+00f, 2.8667e+00f,
      2.8669e+00f, 2.8670e+00f, 2.8671e+00f, 2.8671e+00f, 2.8671e+00f,
      2.8671e+00f, 2.8671e+00f, 2.8671e+00f, 2.8671e+00f, 2.8671e+00f,
      2.8671e+00f, 2.8671e+00f, 2.8671e+00f, 2.8671e+00f, 2.8671e+00f,
      2.8671e+00f, 2.8671e+00f, 2.8671e+00f, 2.8671e+00f, 2.8671e+00f,
      2.8671e+00f, 2.8671e+00f };


    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    bool param_siz::write(outfile_base *file)
    {
      //marker size excluding header
      Lsiz = (ui16)(38 + 3 * Csiz);

      ui8 buf[4];
      bool result = true;

      *(ui16*)buf = JP2K_MARKER::SIZ;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Lsiz);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Rsiz);
      result &= file->write(&buf, 2) == 2;
      *(ui32*)buf = swap_byte(Xsiz);
      result &= file->write(&buf, 4) == 4;
      *(ui32*)buf = swap_byte(Ysiz);
      result &= file->write(&buf, 4) == 4;
      *(ui32*)buf = swap_byte(XOsiz);
      result &= file->write(&buf, 4) == 4;
      *(ui32*)buf = swap_byte(YOsiz);
      result &= file->write(&buf, 4) == 4;
      *(ui32*)buf = swap_byte(XTsiz);
      result &= file->write(&buf, 4) == 4;
      *(ui32*)buf = swap_byte(YTsiz);
      result &= file->write(&buf, 4) == 4;
      *(ui32*)buf = swap_byte(XTOsiz);
      result &= file->write(&buf, 4) == 4;
      *(ui32*)buf = swap_byte(YTOsiz);
      result &= file->write(&buf, 4) == 4;
      *(ui16*)buf = swap_byte(Csiz);
      result &= file->write(&buf, 2) == 2;
      for (int c = 0; c < Csiz; ++c)
      {
        buf[0] = cptr[c].SSiz;
        buf[1] = cptr[c].XRsiz;
        buf[2] = cptr[c].YRsiz;
        result &= file->write(&buf, 3) == 3;
      }

      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    void param_siz::read(infile_base *file)
    {
      if (file->read(&Lsiz, 2) != 2)
        OJPH_ERROR(0x00050041, "error reading SIZ marker");
      Lsiz = swap_byte(Lsiz);
      int num_comps = (Lsiz - 38) / 3;
      if (Lsiz != 38 + 3 * num_comps)
        OJPH_ERROR(0x00050042, "error in SIZ marker length");
      if (file->read(&Rsiz, 2) != 2)
        OJPH_ERROR(0x00050043, "error reading SIZ marker");
      Rsiz = swap_byte(Rsiz);
      if ((Rsiz & 0x4000) == 0)
        OJPH_ERROR(0x00050044, "Rsiz bit 14 not set (this is not a JPH file)");
      if (Rsiz & 0xBFFF)
        OJPH_WARN(0x00050001, "Rsiz in SIZ has unimplemented fields");
      if (file->read(&Xsiz, 4) != 4)
        OJPH_ERROR(0x00050045, "error reading SIZ marker");
      Xsiz = swap_byte(Xsiz);
      if (file->read(&Ysiz, 4) != 4)
        OJPH_ERROR(0x00050046, "error reading SIZ marker");
      Ysiz = swap_byte(Ysiz);
      if (file->read(&XOsiz, 4) != 4)
        OJPH_ERROR(0x00050047, "error reading SIZ marker");
      XOsiz = swap_byte(XOsiz);
      if (file->read(&YOsiz, 4) != 4)
        OJPH_ERROR(0x00050048, "error reading SIZ marker");
      YOsiz = swap_byte(YOsiz);
      if (file->read(&XTsiz, 4) != 4)
        OJPH_ERROR(0x00050049, "error reading SIZ marker");
      XTsiz = swap_byte(XTsiz);
      if (file->read(&YTsiz, 4) != 4)
        OJPH_ERROR(0x0005004A, "error reading SIZ marker");
      YTsiz = swap_byte(YTsiz);
      if (file->read(&XTOsiz, 4) != 4)
        OJPH_ERROR(0x0005004B, "error reading SIZ marker");
      XTOsiz = swap_byte(XTOsiz);
      if (file->read(&YTOsiz, 4) != 4)
        OJPH_ERROR(0x0005004C, "error reading SIZ marker");
      YTOsiz = swap_byte(YTOsiz);
      if (file->read(&Csiz, 2) != 2)
        OJPH_ERROR(0x0005004D, "error reading SIZ marker");
      Csiz = swap_byte(Csiz);
      if (Csiz != num_comps)
        OJPH_ERROR(0x0005004E, "Csiz does not match the SIZ marker size");
      if (Csiz > old_Csiz)
      {
        if (cptr != store)
          delete[] cptr;
        cptr = new siz_comp_info[num_comps];
        old_Csiz = Csiz;
      }
      for (int c = 0; c < Csiz; ++c)
      {
        if (file->read(&cptr[c].SSiz, 1) != 1)
          OJPH_ERROR(0x00050051, "error reading SIZ marker");
        if (file->read(&cptr[c].XRsiz, 1) != 1)
          OJPH_ERROR(0x00050052, "error reading SIZ marker");
        if (file->read(&cptr[c].YRsiz, 1) != 1)
          OJPH_ERROR(0x00050053, "error reading SIZ marker");
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
    bool param_cap::write(outfile_base *file)
    {
      //marker size excluding header
      Lcap = 8;

      char buf[4];
      bool result = true;

      *(ui16*)buf = JP2K_MARKER::CAP;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Lcap);
      result &= file->write(&buf, 2) == 2;
      *(ui32*)buf = swap_byte(Pcap);
      result &= file->write(&buf, 4) == 4;

      *(ui16*)buf = swap_byte(Ccap[0]);
      result &= file->write(&buf, 2) == 2;

      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    void param_cap::read(infile_base *file)
    {
      if (file->read(&Lcap, 2) != 2)
        OJPH_ERROR(0x00050061, "error reading CAP marker");
      Lcap = swap_byte(Lcap);
      if (file->read(&Pcap, 4) != 4)
        OJPH_ERROR(0x00050062, "error reading CAP marker");
      Pcap = swap_byte(Pcap);
      ui32 count = population_count(Pcap);
      if (Pcap & 0xFFFDFFFF)
        OJPH_ERROR(0x00050063,
          "error Pcap in CAP has options that are not supported");
      if ((Pcap & 0x00020000) == 0)
        OJPH_ERROR(0x00050064,
          "error Pcap should have its 15th MSB set, Pcap^15. "
          " This is not a JPH file");
      for (ui32 i = 0; i < count; ++i)
        if (file->read(Ccap+i, 2) != 2)
          OJPH_ERROR(0x00050065, "error reading CAP marker");
      if (Lcap != 6 + 2 * count)
        OJPH_ERROR(0x00050066, "error in CAP marker length");
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    bool param_cod::write(outfile_base *file)
    {
      //marker size excluding header
      Lcod = 12;
      Lcod = (ui16)(Lcod + (Scod & 1 ? 1 + SPcod.num_decomp : 0));

      ui8 buf[4];
      bool result = true;

      *(ui16*)buf = JP2K_MARKER::COD;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Lcod);
      result &= file->write(&buf, 2) == 2;
      *(ui8*)buf = Scod;
      result &= file->write(&buf, 1) == 1;
      *(ui8*)buf = SGCod.prog_order;
      result &= file->write(&buf, 1) == 1;
      *(ui16*)buf = swap_byte(SGCod.num_layers);
      result &= file->write(&buf, 2) == 2;
      *(ui8*)buf = SGCod.mc_trans;
      result &= file->write(&buf, 1) == 1;
      buf[0] = SPcod.num_decomp;
      buf[1] = SPcod.block_width;
      buf[2] = SPcod.block_height;
      buf[3] = SPcod.block_style;
      result &= file->write(&buf, 4) == 4;
      *(ui8*)buf = SPcod.wavelet_trans;
      result &= file->write(&buf, 1) == 1;
      if (Scod & 1)
        for (int i = 0; i <= SPcod.num_decomp; ++i)
        {
          *(ui8*)buf = SPcod.precinct_size[i];
          result &= file->write(&buf, 1) == 1;
        }

      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    void param_cod::read(infile_base *file)
    {
      if (file->read(&Lcod, 2) != 2)
        OJPH_ERROR(0x00050071, "error reading COD marker");
      Lcod = swap_byte(Lcod);
      if (file->read(&Scod, 1) != 1)
        OJPH_ERROR(0x00050072, "error reading COD marker");
      if (file->read(&SGCod.prog_order, 1) != 1)
        OJPH_ERROR(0x00050073, "error reading COD marker");
      if (file->read(&SGCod.num_layers, 2) != 2)
        OJPH_ERROR(0x00050074, "error reading COD marker");
      if (file->read(&SGCod.mc_trans, 1) != 1)
        OJPH_ERROR(0x00050075, "error reading COD marker");
      if (file->read(&SPcod.num_decomp, 1) != 1)
        OJPH_ERROR(0x00050076, "error reading COD marker");
      if (file->read(&SPcod.block_width, 1) != 1)
        OJPH_ERROR(0x00050077, "error reading COD marker");
      if (file->read(&SPcod.block_height, 1) != 1)
        OJPH_ERROR(0x00050078, "error reading COD marker");
      if (file->read(&SPcod.block_style, 1) != 1)
        OJPH_ERROR(0x00050079, "error reading COD marker");
      if (file->read(&SPcod.wavelet_trans, 1) != 1)
        OJPH_ERROR(0x0005007A, "error reading COD marker");
      if (Scod & 1)
        for (int i = 0; i <= SPcod.num_decomp; ++i)
          if (file->read(&SPcod.precinct_size[i], 1) != 1)
            OJPH_ERROR(0x0005007B, "error reading COD marker");
      if (Lcod != 12 + ((Scod & 1) ? 1 + SPcod.num_decomp : 0))
        OJPH_ERROR(0x0005007C, "error in COD marker length");
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void param_qcd::set_rev_quant(ui32 bit_depth,
                                  bool is_employing_color_transform)
    {
      int guard_bits = 1;
      Sqcd = (ui8)(guard_bits << 5); //one guard bit, and no quantization
      ui32 B = bit_depth;
      B += is_employing_color_transform ? 1 : 0; //1 bit for RCT
      int s = 0;
      float bibo_l = bibo_gains::get_bibo_gain_l(num_decomps, true);
      //we leave some leeway for numerical error by multiplying by 1.1f
      ui32 X = (ui32) ceil(log(bibo_l * bibo_l * 1.1f) / M_LN2);
      u8_SPqcd[s++] = (ui8)((B + X) << 3);
      for (ui32 d = num_decomps; d > 0; --d)
      {
        float bibo_l = bibo_gains::get_bibo_gain_l(d, true);
        float bibo_h = bibo_gains::get_bibo_gain_h(d - 1, true);
        X = (ui32) ceil(log(bibo_h * bibo_l * 1.1f) / M_LN2);
        u8_SPqcd[s++] = (ui8)((B + X) << 3);
        u8_SPqcd[s++] = (ui8)((B + X) << 3);
        X = (ui32) ceil(log(bibo_h * bibo_h * 1.1f) / M_LN2);
        u8_SPqcd[s++] = (ui8)((B + X) << 3);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void param_qcd::set_irrev_quant()
    {
      int guard_bits = 1;
      Sqcd = (ui8)((guard_bits<<5)|0x2);//one guard bit, scalar quantization
      int s = 0;
      float gain_l = sqrt_energy_gains::get_gain_l(num_decomps, false);
      float delta_b = base_delta / (gain_l * gain_l);
      int exp = 0, mantissa;
      while (delta_b < 1.0f)
      { exp++; delta_b *= 2.0f; }
      //with rounding, there is a risk of becoming equal to 1<<12
      // but that should not happen in reality
      mantissa = (int)round(delta_b * (float)(1<<11)) - (1<<11);
      mantissa = mantissa < (1<<11) ? mantissa : 0x7FF;
      u16_SPqcd[s++] = (ui16)((exp << 11) | mantissa);
      for (ui32 d = num_decomps; d > 0; --d)
      {
        float gain_l = sqrt_energy_gains::get_gain_l(d, false);
        float gain_h = sqrt_energy_gains::get_gain_h(d - 1, false);

        delta_b = base_delta / (gain_l * gain_h);

        int exp = 0, mantissa;
        while (delta_b < 1.0f)
        { exp++; delta_b *= 2.0f; }
        mantissa = (int)round(delta_b * (float)(1<<11)) - (1<<11);
        mantissa = mantissa < (1<<11) ? mantissa : 0x7FF;
        u16_SPqcd[s++] = (ui16)((exp << 11) | mantissa);
        u16_SPqcd[s++] = (ui16)((exp << 11) | mantissa);

        delta_b = base_delta / (gain_h * gain_h);

        exp = 0;
        while (delta_b < 1)
        { exp++; delta_b *= 2.0f; }
        mantissa = (int)round(delta_b * (float)(1<<11)) - (1<<11);
        mantissa = mantissa < (1<<11) ? mantissa : 0x7FF;
        u16_SPqcd[s++] = (ui16)((exp << 11) | mantissa);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 param_qcd::get_MAGBp() const
    { //this can be written better, but it is only executed once
      ui32 B = 0;
      int irrev = Sqcd & 0x1F;
      if (irrev == 0) //reversible
        for (ui32 i = 0; i < 3 * num_decomps + 1; ++i)
          B = ojph_max(B, (u8_SPqcd[i] >> 3) + get_num_guard_bits() - 1u);
      else if (irrev == 2) //scalar expounded
        for (ui32 i = 0; i < 3 * num_decomps + 1; ++i)
        {
          ui32 nb = num_decomps - (i ? (i - 1) / 3 : 0); //decompsition level
          B = ojph_max(B, (u16_SPqcd[i] >> 11) + get_num_guard_bits() - nb);
        }
      else
        assert(0);

      return B;
    }

    //////////////////////////////////////////////////////////////////////////
    float param_qcd::irrev_get_delta(ui32 resolution, ui32 subband) const
    {
      assert((resolution == 0 && subband == 0) ||
             (resolution <= num_decomps && subband > 0 && subband<4));
      assert((Sqcd & 0x1F) == 2);
      float arr[] = { 1.0f, 2.0f, 2.0f, 4.0f };

      ui32 idx = resolution == 0 ? 0 : (resolution - 1) * 3 + subband;
      int eps = u16_SPqcd[idx] >> 11;
      float mantissa;
      mantissa = (float)((u16_SPqcd[idx] & 0x7FF) | 0x800) * arr[subband];
      mantissa /= (float)(1 << 11);
      mantissa /= (float)(1u << eps);
      return mantissa;
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 param_qcd::get_num_guard_bits() const
    {
      return (Sqcd >> 5);
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 param_qcd::get_Kmax(ui32 resolution, ui32 subband) const
    {
      assert((resolution == 0 && subband == 0) ||
             (resolution <= num_decomps && subband > 0 && subband<4));
      ui32 num_bits = get_num_guard_bits();
      ui32 idx = resolution == 0 ? 0 : (resolution - 1) * 3 + subband;
      int irrev = Sqcd & 0x1F;
      if (irrev == 0) //reversible; this is (10.22) from the J2K book
      {
        num_bits += u8_SPqcd[idx] >> 3;
        num_bits = num_bits == 0 ? 0 : num_bits - 1;
      }
      else if (irrev == 1)
        assert(0);
      else if (irrev == 2) //scalar expounded
        num_bits += (u16_SPqcd[idx] >> 11) - 1;
      else
        assert(0);

      return num_bits;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_qcd::write(outfile_base *file)
    {
      int irrev = Sqcd & 0x1F;
      ui32 num_subbands = 1 + 3 * num_decomps;

      //marker size excluding header
      Lqcd = 3;
      if (irrev == 0)
        Lqcd = (ui16)(Lqcd + num_subbands);
      else if (irrev == 2)
        Lqcd = (ui16)(Lqcd + 2 * num_subbands);
      else
        assert(0);

      char buf[4];
      bool result = true;

      *(ui16*)buf = JP2K_MARKER::QCD;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Lqcd);
      result &= file->write(&buf, 2) == 2;
      *(ui8*)buf = Sqcd;
      result &= file->write(&buf, 1) == 1;

      if (irrev == 0)
        for (ui32 i = 0; i < num_subbands; ++i)
        {
          *(ui8*)buf = u8_SPqcd[i];
          result &= file->write(&buf, 1) == 1;
        }
      else if (irrev == 2)
        for (ui32 i = 0; i < num_subbands; ++i)
        {
          *(ui16*)buf = swap_byte(u16_SPqcd[i]);
          result &= file->write(&buf, 2) == 2;
        }
      else
        assert(0);


      return result;
    }
    //////////////////////////////////////////////////////////////////////////
    void param_qcd::read(infile_base *file)
    {
      if (file->read(&Lqcd, 2) != 2)
        OJPH_ERROR(0x00050081, "error reading QCD marker");
      Lqcd = swap_byte(Lqcd);
      if (file->read(&Sqcd, 1) != 1)
        OJPH_ERROR(0x00050082, "error reading QCD marker");
      if ((Sqcd & 0x1F) == 0)
      {
        num_decomps = (Lqcd - 4) / 3;
        if (Lqcd != 4 + 3 * num_decomps)
          OJPH_ERROR(0x00050083, "wrong Lqcd value in QCD marker");
        for (ui32 i = 0; i < 1 + 3 * num_decomps; ++i)
          if (file->read(&u8_SPqcd[i], 1) != 1)
            OJPH_ERROR(0x00050084, "error reading QCD marker");
      }
      else if ((Sqcd & 0x1F) == 1)
      {
        num_decomps = 0;
        OJPH_ERROR(0x00050089, 
          "Scalar derived quantization is not supported yet in QCD marker");
        if (Lqcd != 5)
          OJPH_ERROR(0x00050085, "wrong Lqcd value in QCD marker");
      }
      else if ((Sqcd & 0x1F) == 2)
      {
        num_decomps = (Lqcd - 5) / 6;
        if (Lqcd != 5 + 6 * num_decomps)
          OJPH_ERROR(0x00050086, "wrong Lqcd value in QCD marker");
        for (ui32 i = 0; i < 1 + 3 * num_decomps; ++i)
        {
          if (file->read(&u16_SPqcd[i], 2) != 2)
            OJPH_ERROR(0x00050087, "error reading QCD marker");
          u16_SPqcd[i] = swap_byte(u16_SPqcd[i]);
        }
      }
      else
        OJPH_ERROR(0x00050088, "wrong Sqcd value in QCD marker");
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void param_qcc::read(infile_base *file, ui32 num_comps)
    {
      if (file->read(&Lqcd, 2) != 2)
        OJPH_ERROR(0x000500A1, "error reading QCC marker");
      Lqcd = swap_byte(Lqcd);
      if (num_comps < 257)
      {
        ui8 v;
        if (file->read(&v, 1) != 1)
          OJPH_ERROR(0x000500A2, "error reading QCC marker");
        comp_idx = v;
      }
      else
      {
        if (file->read(&comp_idx, 2) != 2)
          OJPH_ERROR(0x000500A3, "error reading QCC marker");
        comp_idx = swap_byte(comp_idx);
      }
      if (file->read(&Sqcd, 1) != 1)
        OJPH_ERROR(0x000500A4, "error reading QCC marker");
      if ((Sqcd & 0x1F) == 0)
      {
        ui32 offset = num_comps < 257 ? 5 : 6;
        num_decomps = (Lqcd - offset) / 3;
        if (Lqcd != offset + 3 * num_decomps)
          OJPH_ERROR(0x000500A5, "wrong Lqcd value in QCC marker");
        for (ui32 i = 0; i < 1 + 3 * num_decomps; ++i)
          if (file->read(&u8_SPqcd[i], 1) != 1)
            OJPH_ERROR(0x000500A6, "error reading QCC marker");
      }
      else if ((Sqcd & 0x1F) == 1)
      {
        ui32 offset = num_comps < 257 ? 6 : 7;
        num_decomps = 0;
        OJPH_ERROR(0x000500AB, 
          "Scalar derived quantization is not supported yet in QCC marker");
        if (Lqcd != offset)
          OJPH_ERROR(0x000500A7, "wrong Lqcc value in QCC marker");
      }
      else if ((Sqcd & 0x1F) == 2)
      {
        ui32 offset = num_comps < 257 ? 6 : 7;
        num_decomps = (Lqcd - offset) / 6;
        if (Lqcd != offset + 6 * num_decomps)
          OJPH_ERROR(0x000500A8, "wrong Lqcc value in QCC marker");
        for (ui32 i = 0; i < 1 + 3 * num_decomps; ++i)
        {
          if (file->read(&u16_SPqcd[i], 2) != 2)
            OJPH_ERROR(0x000500A9, "error reading QCC marker");
          u16_SPqcd[i] = swap_byte(u16_SPqcd[i]);
        }
      }
      else
        OJPH_ERROR(0x000500AA, "wrong Sqcc value in QCC marker");
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    bool param_sot::write(outfile_base *file, ui32 payload_len)
    {
      char buf[4];
      bool result = true;

      this->Psot = payload_len + 14; //inc. SOT marker, field & SOD

      *(ui16*)buf = JP2K_MARKER::SOT;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Lsot);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Isot);
      result &= file->write(&buf, 2) == 2;
      *(ui32*)buf = swap_byte(Psot);
      result &= file->write(&buf, 4) == 4;
      *(ui8*)buf = TPsot;
      result &= file->write(&buf, 1) == 1;
      *(ui8*)buf = TNsot;
      result &= file->write(&buf, 1) == 1;

      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_sot::write(outfile_base *file, ui32 payload_len,
                          ui8 TPsot, ui8 TNsot)
    {
      char buf[4];
      bool result = true;

      *(ui16*)buf = JP2K_MARKER::SOT;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Lsot);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Isot);
      result &= file->write(&buf, 2) == 2;
      *(ui32*)buf = swap_byte(payload_len + 14);
      result &= file->write(&buf, 4) == 4;
      *(ui8*)buf = TPsot;
      result &= file->write(&buf, 1) == 1;
      *(ui8*)buf = TNsot;
      result &= file->write(&buf, 1) == 1;

      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_sot::read(infile_base *file, bool resilient)
    {
      if (resilient)
      {
        if (file->read(&Lsot, 2) != 2)
        {
          OJPH_INFO(0x00050091, "error reading SOT marker");
          Lsot = 0; Isot = 0; Psot = 0; TPsot = 0; TNsot = 0; 
          return false;
        }
        Lsot = swap_byte(Lsot);
        if (Lsot != 10)
        {
          OJPH_INFO(0x00050092, "error in SOT length");
          Lsot = 0; Isot = 0; Psot = 0; TPsot = 0; TNsot = 0;
          return false;
        }
        if (file->read(&Isot, 2) != 2)
        {
          OJPH_INFO(0x00050093, "error reading tile index");
          Lsot = 0; Isot = 0; Psot = 0; TPsot = 0; TNsot = 0;
          return false;
        }
        Isot = swap_byte(Isot);
        if (Isot == 0xFFFF)
        {
          OJPH_INFO(0x00050094, "tile index in SOT marker cannot be 0xFFFF");
          Lsot = 0; Isot = 0; Psot = 0; TPsot = 0; TNsot = 0;
          return false;
        }
        if (file->read(&Psot, 4) != 4)
        {
          OJPH_INFO(0x00050095, "error reading SOT marker");
          Lsot = 0; Isot = 0; Psot = 0; TPsot = 0; TNsot = 0;
          return false;
        }
        Psot = swap_byte(Psot);
        if (file->read(&TPsot, 1) != 1)
        {
          OJPH_INFO(0x00050096, "error reading SOT marker");
          Lsot = 0; Isot = 0; Psot = 0; TPsot = 0; TNsot = 0;
          return false;
        }
        if (file->read(&TNsot, 1) != 1)
        {
          OJPH_INFO(0x00050097, "error reading SOT marker");
          Lsot = 0; Isot = 0; Psot = 0; TPsot = 0; TNsot = 0;
          return false;
        }
      }
      else
      {
        if (file->read(&Lsot, 2) != 2)
          OJPH_ERROR(0x00050091, "error reading SOT marker");
        Lsot = swap_byte(Lsot);
        if (Lsot != 10)
          OJPH_ERROR(0x00050092, "error in SOT length");
        if (file->read(&Isot, 2) != 2)
          OJPH_ERROR(0x00050093, "error reading SOT tile index");
        Isot = swap_byte(Isot);
        if (Isot == 0xFFFF)
          OJPH_ERROR(0x00050094, "tile index in SOT marker cannot be 0xFFFF");
        if (file->read(&Psot, 4) != 4)
          OJPH_ERROR(0x00050095, "error reading SOT marker");
        Psot = swap_byte(Psot);
        if (file->read(&TPsot, 1) != 1)
          OJPH_ERROR(0x00050096, "error reading SOT marker");
        if (file->read(&TNsot, 1) != 1)
          OJPH_ERROR(0x00050097, "error reading SOT marker");
      }
      return true;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void param_tlm::init(ui32 num_pairs, Ttlm_Ptlm_pair *store)
    {
      this->num_pairs = num_pairs;
      pairs = (Ttlm_Ptlm_pair*)store;
      Ltlm = (ui16)(4 + 6 * num_pairs);
      Ztlm = 0;
      Stlm = 0x60;
    }

    //////////////////////////////////////////////////////////////////////////
    void param_tlm::set_next_pair(ui16 Ttlm, ui32 Ptlm)
    {
      assert(next_pair_index < num_pairs);
      pairs[next_pair_index].Ttlm = Ttlm;
      pairs[next_pair_index].Ptlm = Ptlm + 14;
      ++next_pair_index;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_tlm::write(outfile_base *file)
    {
      assert(next_pair_index == num_pairs);
      char buf[4];
      bool result = true;

      *(ui16*)buf = JP2K_MARKER::TLM;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Ltlm);
      result &= file->write(&buf, 2) == 2;
      result &= file->write(&Ztlm, 1) == 1;
      result &= file->write(&Stlm, 1) == 1;
      for (ui32 i = 0; i < num_pairs; ++i)
      {
        *(ui16*)buf = swap_byte(pairs[i].Ttlm);
        result &= file->write(&buf, 2) == 2;
        *(ui32*)buf = swap_byte(pairs[i].Ptlm);
        result &= file->write(&buf, 4) == 4;
      }
      return result;
    }

  }

}
