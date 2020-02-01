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
// File: ojph_params_local.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_PARAMS_LOCAL_H
#define OJPH_PARAMS_LOCAL_H

#include <cstring>
#include <cassert>

#include "ojph_defs.h"
#include "ojph_arch.h"
#include "ojph_message.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  class outfile_base;
  class infile_base;

  ////////////////////////////////////////////////////////////////////////////
  enum PROGRESSION_ORDER : si32
  {
    OJPH_PO_LRCP = 0,
    OJPH_PO_RLCP = 1,
    OJPH_PO_RPCL = 2,
    OJPH_PO_PCRL = 3,
    OJPH_PO_CPRL = 4
  };

  ////////////////////////////////////////////////////////////////////////////
  const char OJPH_PO_STRING_LRCP[] = "LRCP";
  const char OJPH_PO_STRING_RLCP[] = "RLCP";
  const char OJPH_PO_STRING_RPCL[] = "RPCL";
  const char OJPH_PO_STRING_PCRL[] = "PCRL";
  const char OJPH_PO_STRING_CPRL[] = "CPRL";

  ////////////////////////////////////////////////////////////////////////////
  enum OJPH_PROFILE_NUM : si32
  {
    OJPH_PN_UNDEFINED = 0,
    OJPH_PN_PROFILE0 = 1,
    OJPH_PN_PROFILE1 = 2,
    OJPH_PN_CINEMA2K = 3,
    OJPH_PN_CINEMA4K = 4,
    OJPH_PN_CINEMAS2K = 5,
    OJPH_PN_CINEMAS4K = 6,
    OJPH_PN_BROADCAST = 7,
    OJPH_PN_IMF = 8
  };

  ////////////////////////////////////////////////////////////////////////////
  const char OJPH_PN_STRING_PROFILE0[] = "PROFILE0";
  const char OJPH_PN_STRING_PROFILE1[] = "PROFILE1";
  const char OJPH_PN_STRING_CINEMA2K[] = "CINEMA2K";
  const char OJPH_PN_STRING_CINEMA4K[] = "CINEMA4K";
  const char OJPH_PN_STRING_CINEMAS2K[] = "CINEMAS2K";
  const char OJPH_PN_STRING_CINEMAS4K[] = "CINEMAS4K";
  const char OJPH_PN_STRING_BROADCAST[] = "BROADCAST";
  const char OJPH_PN_STRING_IMF[] = "IMF";

  namespace local {

    //////////////////////////////////////////////////////////////////////////
    enum JP2K_MARKER : ui16
    {
      SOC = 0xFF4F, //start of codestream (required)
      CAP = 0xFF50, //extended capability
      SIZ = 0xFF51, //image and tile size (required)
      COD = 0xFF52, //coding style default (required)
      TLM = 0xFF55, //tile-part lengths
      PRF = 0xFF56, //profile
      PLM = 0xFF57, //packet length, main header
      PLT = 0xFF58, //packet length, tile-part header
      CPF = 0xFF59, //corresponding profile values
      QCD = 0xFF5C, //qunatization default (required)
      COM = 0xFF64, //comment
      SOT = 0xFF90, //start of tile-part
      SOP = 0xFF91, //start of packet
      EPH = 0xFF92, //end of packet
      SOD = 0xFF93, //start of data
      EOC = 0xFFD9, //end of codestream (required)

      COC = 0xFF53, //coding style component
      QCC = 0xFF5D, //quantization component
      RGN = 0xFF5E, //region of interest
      POC = 0xFF5F, //progression order change
      PPM = 0xFF60, //packed packet headers, main header
      PPT = 0xFF61, //packed packet headers, tile-part header
      CRG = 0xFF63, //component registration
    };

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////
    struct siz_comp_info
    {
      ui8 SSiz;
      ui8 XRsiz;
      ui8 YRsiz;
    };

    //////////////////////////////////////////////////////////////////////////
    struct param_siz
    {
      friend ::ojph::param_siz;

    public:
      param_siz()
      {
        memset(this, 0, sizeof(param_siz));
        cptr = store;
        old_Csiz = 4;
        Rsiz = 0x4000; //for jph, bit 14 of Rsiz is 1
      }

      ~param_siz()
      {
        if (cptr != store) delete[] cptr;
      }

      void set_num_components(int num_comps)
      {
        Csiz = num_comps;
        if (Csiz > old_Csiz)
        {
          if (cptr != store)
            delete[] cptr;
          cptr = new siz_comp_info[num_comps];
          old_Csiz = Csiz;
        }
        memset(cptr, 0, sizeof(local::siz_comp_info) * num_comps);
      }

      void set_comp_info(si32 comp_num, const point& downsampling,
                         si32 bit_depth, bool is_signed)
      {
        assert(comp_num < Csiz);
        assert(downsampling.x != 0 && downsampling.y != 0);
        cptr[comp_num].SSiz = bit_depth - 1 + (is_signed ? 0x80 : 0);
        cptr[comp_num].XRsiz = downsampling.x;
        cptr[comp_num].YRsiz = downsampling.y;
      }

      void check_validity()
      {
        if (XTsiz == 0 && YTsiz == 0)
        { XTsiz = Xsiz - XOsiz; YTsiz = Ysiz - YOsiz; }
        if (Xsiz <= 0 || Ysiz <= 0 || XTsiz <= 0 || YTsiz <= 0 ||
            XOsiz < 0 || YOsiz < 0 || XTOsiz < 0 || YTOsiz < 0)
          OJPH_ERROR(0x00040001,
            "image extent and offset, and tile size and offset cannot be "
            "negative");
        if (XTOsiz > XOsiz || YTOsiz > XOsiz)
          OJPH_ERROR(0x00040002,
            "tile offset has to be smaller than image offset");
        if (XTsiz + XTOsiz <= XOsiz || YTsiz + YTOsiz <= YOsiz)
          OJPH_ERROR(0x00040003,
            "the top left tile must intersect with the image");
      }

      ui16 get_num_components() const { return Csiz; }
      si32 get_bit_depth(si32 comp_num) const
      {
        assert(comp_num < Csiz);
        return (cptr[comp_num].SSiz & 0x7F) + 1;
      }
      bool is_signed(si32 comp_num) const
      {
        assert(comp_num < Csiz);
        return (cptr[comp_num].SSiz & 0x80) != 0;
      }
      point get_downsampling(si32 comp_num) const
      {
        assert(comp_num < Csiz);
        return point(cptr[comp_num].XRsiz, cptr[comp_num].YRsiz);
      }

      bool write(outfile_base *file);
      void read(infile_base *file);

    private:
      ui16 Lsiz;
      ui16 Rsiz;
      ui32 Xsiz;
      ui32 Ysiz;
      ui32 XOsiz;
      ui32 YOsiz;
      ui32 XTsiz;
      ui32 YTsiz;
      ui32 XTOsiz;
      ui32 YTOsiz;
      ui16 Csiz;
      siz_comp_info* cptr;

    private:
      int old_Csiz;
      siz_comp_info store[4];
      param_siz(const param_siz&); //prevent copy constructor
      param_siz& operator=(const param_siz&); //prevent copy
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    ///////////////////////////////////////////////////////////////////////////
    struct cod_SPcod
    {
      ui8 num_decomp;
      ui8 block_width;
      ui8 block_height;
      ui8 block_style;
      ui8 wavelet_trans;
      ui8 precinct_size[33]; //num_decomp is in [0,32]
    };

    ///////////////////////////////////////////////////////////////////////////
    typedef cod_SPcod cod_SPcoc;

    ///////////////////////////////////////////////////////////////////////////
    struct cod_SGcod
    {
      ui8 prog_order;
      ui16 num_layers;
      ui8 mc_trans;
    };

    ///////////////////////////////////////////////////////////////////////////
    struct param_cod
    {
      friend ::ojph::param_cod;
    public:
      param_cod()
      {
        memset(this, 0, sizeof(param_cod));
        SPcod.block_style = 0x40;
        SGCod.prog_order = 2;
        SGCod.num_layers = 1;
        SGCod.mc_trans = 0;
        SPcod.num_decomp = 5;
        SPcod.block_width = 4; //64
        SPcod.block_height = 4; //64
        set_reversible(false);
      }

      void set_reversible(bool reversible)
      {
        SPcod.wavelet_trans = reversible ? 1 : 0;
      }

      void employ_color_transform(ui8 val)
      {
        assert(val == 0 || val == 1);
        SGCod.mc_trans = val;
      }

      void check_validity(const param_siz& siz)
      {
        //check that colour transform and match number of components and
        // downsampling
        int num_comps = siz.get_num_components();
        if (SGCod.mc_trans == 1 && num_comps < 3)
          OJPH_ERROR(0x00040011,
            "color transform can only be employed when the image has 3 or "
            "more color components");

        if (SGCod.mc_trans == 1)
        {
          bool test = false;
          point p = siz.get_downsampling(0);
          for (int i = 1; i < 3; ++i)
          {
            point p1 = siz.get_downsampling(i);
            test = test || (p.x != p1.x || p.y != p1.y);
          }
          if (test)
            OJPH_ERROR(0x00040012,
              "when color transform is used, the first 3 colour "
              "components must have the same downsampling.");
        }

        //check the progression order matches downsampling
        if (SGCod.prog_order == 2 || SGCod.prog_order == 3)
        {
          int num_comps = siz.get_num_components();
          for (int i = 0; i < num_comps; ++i)
          {
            point r = siz.get_downsampling(i);
            if (r.x & (r.x - 1) || r.y & (r.y - 1))
              OJPH_ERROR(0x00040013, "For RPCL and PCRL progression orders,"
                "component downsampling factors have to be powers of 2");
          }
        }
      }

      ui8 get_num_decompositions() const
      { return SPcod.num_decomp; }
      bool is_reversible() const
      { return (SPcod.wavelet_trans == 1); }
      bool is_employing_color_transform() const
      { return (SGCod.mc_trans == 1); }

      bool write(outfile_base *file);
      void read(infile_base *file);

    private:
      ui16 Lcod;
      ui8 Scod;
      cod_SGcod SGCod;
      cod_SPcod SPcod;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    ///////////////////////////////////////////////////////////////////////////
    struct param_qcd
    {
      friend ::ojph::param_qcd;
    public:
      param_qcd()
      { memset(this, 0, sizeof(param_qcd)); base_delta = -1.0f; }

      void set_delta(float delta) { base_delta = delta; }
      void set_rev_quant(int bit_depth, bool is_employing_color_transform);
      void set_irrev_quant();

      void check_validity(const param_siz& siz, const param_cod& cod)
      {
        num_decomps = cod.get_num_decompositions();
        if (cod.is_reversible())
        {
          int bit_depth = 0;
          for (int i = 0; i < siz.get_num_components(); ++i)
            bit_depth = ojph_max(bit_depth, siz.get_bit_depth(i));
          set_rev_quant(bit_depth, cod.is_employing_color_transform());
        }
        else
        {
          if (base_delta == -1.0f)
            base_delta = 1.0f /
              (1 << (siz.get_bit_depth(0) + siz.is_signed(0)));
          set_irrev_quant();
         }
      }

      int get_num_guard_bits() const;
      int get_MAGBp() const;
      int get_Kmax(int resolution, int subband) const;
      int rev_get_num_bits(int resolution, int subband) const;
      float irrev_get_delta(int resolution, int subband) const;

      bool write(outfile_base *file);
      void read(infile_base *file);

    private:
      ui16 Lqcd;
      ui8 Sqcd;
      union
      {
        ui8 u8_SPqcd[97];
        ui16 u16_SPqcd[97];
      };
      int num_decomps;
      float base_delta;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    ///////////////////////////////////////////////////////////////////////////
    struct param_cap
    {
    public:
      param_cap()
      {
        memset(this, 0, sizeof(param_cap));
        Lcap = 8;
        Pcap = 0x00020000; //for jph, Pcap^15 must be set, the 15th MSB
      }

      void check_validity(const param_cod& cod, const param_qcd& qcd)
      {
        if (cod.is_reversible())
          Ccap[0] &= 0xFFDF;
        else
          Ccap[0] |= 0x0020;
        Ccap[0] &= 0xFFE0;
        int Bp = 0;
        int B = qcd.get_MAGBp();
        if (B <= 8)
          Bp = 0;
        else if (B < 28)
          Bp = B - 8;
        else if (B < 48)
          Bp = 13 + (B >> 2);
        else
          Bp = 31;
        Ccap[0] |= Bp;
      }

      bool write(outfile_base *file);
      void read(infile_base *file);

    private:
      ui16 Lcap;
      ui32 Pcap;
      ui16 Ccap[32]; //a maximum of 32
    };


    ///////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    ///////////////////////////////////////////////////////////////////////////
    struct param_sot
    {
    public:
      void init(ui32 tile_length = 0, ui16 tile_idx = 0,
                ui8 tile_part_index = 0, ui8 num_tile_parts = 0)
      {
        Lsot = 10;
        Psot = tile_length > 0 ? tile_length + 14 : 0;
        Isot = tile_idx;
        TPsot = tile_part_index;
        TNsot = num_tile_parts;
      }

      bool write(outfile_base *file, ui32 payload_len);
      bool write(outfile_base *file, ui32 payload_len, ui8 TPsot, ui8 TNsot);
      void read(infile_base *file);
      void append(ui32 additional_length)
      {
        Psot = get_length() + additional_length;
      }

      ui16 get_tile_index() const { return Isot; }
      ui32 get_length() const { return Psot > 0 ? Psot - 14 : 0; }
      ui8  get_tile_part_index() const { return TPsot; }
      ui8  get_num_tile_parts() const { return TNsot; }

    private:
      ui16 Lsot;
      ui16 Isot;
      ui32 Psot;
      ui8 TPsot;
      ui8 TNsot;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    ///////////////////////////////////////////////////////////////////////////
    struct param_tlm
    {
      struct Ttlm_Ptlm_pair
      {
        ui16 Ttlm;
        ui32 Ptlm;
      };

    public:
      param_tlm() { pairs = NULL; num_pairs = 0; next_pair_index = 0; };
      void init(int num_pairs, Ttlm_Ptlm_pair* store);

      void set_next_pair(ui16 Ttlm, ui32 Ptlm);
      bool write(outfile_base *file);

    private:
      ui16 Ltlm;
      ui8 Ztlm;
      ui8 Stlm;
      Ttlm_Ptlm_pair* pairs;
      int num_pairs;
      int next_pair_index;
      
    };
  }
}

#endif // !OJPH_PARAMS_LOCAL_H
