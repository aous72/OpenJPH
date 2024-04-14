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
#include "ojph_base.h"
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

  ////////////////////////////////////////////////////////////////////////////
  enum OJPH_TILEPART_DIVISIONS: ui32 {
    OJPH_TILEPART_NO_DIVISIONS = 0x0, // no divisions to tile parts
    OJPH_TILEPART_RESOLUTIONS  = 0x1,
    OJPH_TILEPART_COMPONENTS   = 0x2,
    OJPH_TILEPART_LAYERS       = 0x4, // these are meaningless with HTJ2K
  };

  namespace local {

    //defined here
    struct param_siz;
    struct param_cod;
    struct param_qcd;
    struct param_qcc;
    struct param_cap;
    struct param_sot;
    struct param_tlm;
    struct param_dfs;
    struct param_atk;

    //////////////////////////////////////////////////////////////////////////
    enum JP2K_MARKER : ui16
    {
      SOC = 0xFF4F, //start of codestream (required)
      CAP = 0xFF50, //extended capability
      SIZ = 0xFF51, //image and tile size (required)
      COD = 0xFF52, //coding style default (required)
      COC = 0xFF53, //coding style component
      TLM = 0xFF55, //tile-part lengths
      PRF = 0xFF56, //profile
      PLM = 0xFF57, //packet length, main header
      PLT = 0xFF58, //packet length, tile-part header
      CPF = 0xFF59, //corresponding profile values
      QCD = 0xFF5C, //qunatization default (required)
      QCC = 0xFF5D, //quantization component
      RGN = 0xFF5E, //region of interest
      POC = 0xFF5F, //progression order change
      PPM = 0xFF60, //packed packet headers, main header
      PPT = 0xFF61, //packed packet headers, tile-part header
      CRG = 0xFF63, //component registration
      COM = 0xFF64, //comment
      DFS = 0xFF72, //downsampling factor styles
      ADS = 0xFF73, //arbitrary decomposition styles
      ATK = 0xFF79, //arbitrary transformation kernels
      SOT = 0xFF90, //start of tile-part
      SOP = 0xFF91, //start of packet
      EPH = 0xFF92, //end of packet
      SOD = 0xFF93, //start of data
      EOC = 0xFFD9, //end of codestream (required)
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

      void set_num_components(ui32 num_comps)
      {
        Csiz = (ui16)num_comps;
        if (Csiz > old_Csiz)
        {
          if (cptr != store)
            delete[] cptr;
          cptr = new siz_comp_info[num_comps];
          old_Csiz = Csiz;
        }
        memset(cptr, 0, sizeof(local::siz_comp_info) * num_comps);
      }

      void set_comp_info(ui32 comp_num, const point& downsampling,
                         ui32 bit_depth, bool is_signed)
      {
        assert(comp_num < Csiz);
        assert(downsampling.x != 0 && downsampling.y != 0);
        cptr[comp_num].SSiz = (ui8)(bit_depth - 1 + (is_signed ? 0x80 : 0));
        cptr[comp_num].XRsiz = (ui8)downsampling.x;
        cptr[comp_num].YRsiz = (ui8)downsampling.y;
      }

      void check_validity(const param_cod& cod)
      {
        this->cod = &cod;

        if (XTsiz == 0 && YTsiz == 0)
        { XTsiz = Xsiz + XOsiz; YTsiz = Ysiz + YOsiz; }
        if (Xsiz == 0 || Ysiz == 0 || XTsiz == 0 || YTsiz == 0)
          OJPH_ERROR(0x00040001, 
            "You cannot set image extent nor tile size to zero");
        if (XTOsiz > XOsiz || YTOsiz > YOsiz)
          OJPH_ERROR(0x00040002,
            "tile offset has to be smaller than image offset");
        if (XTsiz + XTOsiz <= XOsiz || YTsiz + YTOsiz <= YOsiz)
          OJPH_ERROR(0x00040003,
            "the top left tile must intersect with the image");
      }

      ui16 get_num_components() const { return Csiz; }
      ui32 get_bit_depth(ui32 comp_num) const
      {
        assert(comp_num < Csiz);
        return (cptr[comp_num].SSiz & 0x7F) + 1u;
      }
      bool is_signed(ui32 comp_num) const
      {
        assert(comp_num < Csiz);
        return (cptr[comp_num].SSiz & 0x80) != 0;
      }
      point get_downsampling(ui32 comp_num) const
      {
        assert(comp_num < Csiz);
        return point(cptr[comp_num].XRsiz, cptr[comp_num].YRsiz);
      }

      bool write(outfile_base *file);
      void read(infile_base *file);

      void link(const param_cod* cod)
      { this->cod = cod; }

      void link(const param_dfs* dfs)
      { this->dfs = dfs; }

      void set_skipped_resolutions(ui32 skipped_resolutions)
      { this->skipped_resolutions = skipped_resolutions; }
      
      ui32 get_width(ui32 comp_num) const
      {
        assert(comp_num < get_num_components());
        ui32 ds = (ui32)cptr[comp_num].XRsiz;
        ui32 t = ojph_div_ceil(Xsiz, ds) - ojph_div_ceil(XOsiz, ds);
        return t;
      }
      ui32 get_height(ui32 comp_num) const
      {
        assert(comp_num < get_num_components());
        ui32 ds = (ui32)cptr[comp_num].YRsiz;
        ui32 t = ojph_div_ceil(Ysiz, ds) - ojph_div_ceil(YOsiz, ds);
        return t;
      }

      point get_recon_downsampling(ui32 comp_num) const;
      point get_recon_size(ui32 comp_num) const;
      ui32 get_recon_width(ui32 comp_num) const
      { return get_recon_size(comp_num).x; }
      ui32 get_recon_height(ui32 comp_num) const
      { return get_recon_size(comp_num).y; }

      bool is_ws_kern_support_needed() { return ws_kern_support_needed; }
      bool is_dfs_support_needed() { return dfs_support_needed; }

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
      ui32 skipped_resolutions;
      int old_Csiz;
      siz_comp_info store[4];
      bool ws_kern_support_needed;
      bool dfs_support_needed;
      const param_cod* cod;
      const param_dfs* dfs;
      param_siz(const param_siz&) = delete; //prevent copy constructor
      param_siz& operator=(const param_siz&) = delete; //prevent copy
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

      size get_log_block_dims() const
      { return size(block_width + 2, block_height + 2); }
      size get_block_dims() const 
      { size t = get_log_block_dims(); return size(1 << t.w, 1 << t.h); }
      size get_log_precinct_size(ui32 res_num) const
      {
        assert(res_num <= num_decomp);
        size ps(precinct_size[res_num] & 0xF, precinct_size[res_num] >> 4);
        return ps;
      }
    };

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
      // serves for both COD and COC markers

      friend ::ojph::param_cod;
      ////////////////////////////////////////
      enum BLOCK_CODING_STYLES {
        VERT_CAUSAL_MODE = 0x8,
        HT_MODE = 0x40
      };
      ////////////////////////////////////////
      enum cod_type : ui8 {
        UNDEFINED = 0,
        COD_MAIN  = 1,
        COC_MAIN  = 2,
        COD_TILE  = 3,
        COC_TILE  = 4
      };
      ////////////////////////////////////////
      enum dwt_type : ui8 {
        DWT_IRV97 = 0,
        DWT_REV53 = 1,
      };

    public: // COD_MAIN and COC_MAIN common functions
      ////////////////////////////////////////
      param_cod()
      {
        memset(this, 0, sizeof(param_cod));
        SPcod.block_style = HT_MODE;
        SGCod.prog_order = OJPH_PO_RPCL;
        SGCod.num_layers = 1;
        SGCod.mc_trans = 0;
        SPcod.num_decomp = 5;
        SPcod.block_width = 4; //64
        SPcod.block_height = 4; //64
        next = NULL;
      }

      ////////////////////////////////////////
      void set_reversible(bool reversible)
      {
        assert(type == UNDEFINED || type == COD_MAIN);
        type = COD_MAIN;
        SPcod.wavelet_trans = reversible ? DWT_REV53 : DWT_IRV97;
      }

      ////////////////////////////////////////
      void employ_color_transform(ui8 val)
      {
        assert(val == 0 || val == 1);
        assert(type == UNDEFINED || type == COD_MAIN);
        type = COD_MAIN;
        SGCod.mc_trans = val;
      }

      ////////////////////////////////////////
      void check_validity(const param_siz& siz)
      {
        assert(type == UNDEFINED || type == COD_MAIN);
        type = COD_MAIN;

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
          for (ui32 i = 1; i < 3; ++i)
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
          ui32 num_comps = siz.get_num_components();
          for (ui32 i = 0; i < num_comps; ++i)
          {
            point r = siz.get_downsampling(i);
            if (r.x & (r.x - 1) || r.y & (r.y - 1))
              OJPH_ERROR(0x00040013, "For RPCL and PCRL progression orders,"
                "component downsampling factors have to be powers of 2");
          }
        }
      }

      ////////////////////////////////////////
      ui8 get_num_decompositions() const
      {
        if (type == COD_MAIN)
          return SPcod.num_decomp;
        else if (type == COC_MAIN)
        {
          if (is_dfs_defined())
            return parent->get_num_decompositions();
          else
            return SPcod.num_decomp;
        }
        else {
          assert(0);
          return 0; // just in case
        }
      }

      ////////////////////////////////////////
      size get_block_dims() const
      { return SPcod.get_block_dims(); }

      ////////////////////////////////////////
      size get_log_block_dims() const
      { return SPcod.get_log_block_dims(); }

      ////////////////////////////////////////
      ui8 get_wavelet_kern() const
      { return SPcod.wavelet_trans; }

      ////////////////////////////////////////
      bool is_employing_color_transform() const
      { return (SGCod.mc_trans == 1); }

      ////////////////////////////////////////
      size get_precinct_size(ui32 res_num) const
      {
        size t = get_log_precinct_size(res_num);
        return size(1 << t.w, 1 << t.h);
      }

      ////////////////////////////////////////
      size get_log_precinct_size(ui32 res_num) const
      { 
        if (Scod & 1)
          return SPcod.get_log_precinct_size(res_num);
        else
          return size(15, 15);
      }

      ////////////////////////////////////////
      bool packets_may_use_sop() const
      { 
        if (parent)
          return (parent->Scod & 2) == 2; 
        else
          return (Scod & 2) == 2;
      }

      ////////////////////////////////////////
      bool packets_use_eph() const
      { 
        if (parent)
          return (parent->Scod & 4) == 4;
        else
          return (Scod & 4) == 4;
      }

      ////////////////////////////////////////
      bool write(outfile_base *file);

      ////////////////////////////////////////
      void read(infile_base *file, cod_type type);

      ////////////////////////////////////////
      void read(infile_base* file, cod_type type, ui32 num_comps, 
                param_cod* cod);

      ////////////////////////////////////////
      void update_atk(const param_atk* atk);

      ////////////////////////////////////////
      void link_cod(const param_cod* coc)
      { this->next = coc; }

      ////////////////////////////////////////
      const param_cod* get_cod(ui32 comp_num) const
      {
        const param_cod* result = this->next;
        while (result != NULL && result->get_comp_num() != comp_num)
          result = result->next;
        if (result)
          return result;
        else
          return this;
      }

      ////////////////////////////////////////
      const param_atk* access_atk() const { return atk; }

    public: // COC_MAIN only functions
      ////////////////////////////////////////
      bool is_dfs_defined() const 
      { return (SPcod.num_decomp & 0x80) != 0; }

      ////////////////////////////////////////
      ui16 get_dfs_index() const  // cannot be more than 15
      { return SPcod.num_decomp & 0xF; }

      ////////////////////////////////////////
      ui32 get_comp_num() const
      { assert(type == COC_MAIN); return comp_num; }

    private: // Common variables
      cod_type type;        // The type of this cod structure
      ui16 Lcod;            // serves as Lcod and Scod
      ui8 Scod;             // serves as Scod and Scoc
      cod_SGcod SGCod;      // Used in COD and copied to COC
      cod_SPcod SPcod;      // serves as SPcod and SPcoc
      const param_cod* next;// to chain coc parameters to cod

    private: // COC only variables
      param_cod* parent;    // parent COD structure
      ui16 comp_num;        // component index of this COC structure
      const param_atk* atk; // useful when SPcod.wavelet_trans > 1
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
      { 
        Lqcd = 0;
        Sqcd = 0;
        memset(u16_SPqcd, 0, sizeof(u16_SPqcd));
        num_subbands = 0;
        base_delta = -1.0f;
      }

      void set_delta(float delta) { base_delta = delta; }

      void check_validity(const param_siz& siz, const param_cod& cod)
      {
        ui32 num_decomps = cod.get_num_decompositions();
        num_subbands = 1 + 3 * num_decomps;
        if (cod.get_wavelet_kern() == param_cod::DWT_REV53)
        {
          ui32 bit_depth = 0;
          for (ui32 i = 0; i < siz.get_num_components(); ++i)
            bit_depth = ojph_max(bit_depth, siz.get_bit_depth(i));
          set_rev_quant(num_decomps, bit_depth,
            cod.is_employing_color_transform());
        }
        else if (cod.get_wavelet_kern() == param_cod::DWT_IRV97)
        {
          if (base_delta == -1.0f) {
            ui32 bit_depth = 0;
            for (ui32 i = 0; i < siz.get_num_components(); ++i)
              bit_depth =
              ojph_max(bit_depth, siz.get_bit_depth(i) + siz.is_signed(i));
            base_delta = 1.0f / (float)(1 << bit_depth);
          }
          set_irrev_quant(num_decomps);
        }
        else
          assert(0);
      }
      ui32 get_num_guard_bits() const;
      ui32 get_MAGBp() const;
      ui32 get_Kmax(const param_dfs* dfs, ui32 num_decompositions,
                    ui32 resolution, ui32 subband) const;
      float irrev_get_delta(const param_dfs* dfs,
                            ui32 num_decompositions,
                            ui32 resolution, ui32 subband) const;

      bool write(outfile_base *file);
      void read(infile_base *file);

    protected:
      void set_rev_quant(ui32 num_decomps, ui32 bit_depth, 
                         bool is_employing_color_transform);
      void set_irrev_quant(ui32 num_decomps);

    protected:
      ui16 Lqcd;
      ui8 Sqcd;
      union
      {
        ui8 u8_SPqcd[97];
        ui16 u16_SPqcd[97];
      };
      ui32 num_subbands;        // number of subbands
      float base_delta;         // base quantization step size -- all other
                                // step sizes are derived from it.
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    ///////////////////////////////////////////////////////////////////////////
    struct param_qcc : public param_qcd
    {
    public:
      param_qcc() : param_qcd()
      { comp_idx = 0; }

      ui16 get_comp_num() { return comp_idx; }
      void read(infile_base *file, ui32 num_comps);

    protected:
        ui16 comp_idx;
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
        if (cod.get_wavelet_kern() == param_cod::DWT_REV53)
          Ccap[0] &= 0xFFDF;
        else
          Ccap[0] |= 0x0020;
        Ccap[0] &= 0xFFE0;
        ui32 Bp = 0;
        ui32 B = qcd.get_MAGBp();
        if (B <= 8)
          Bp = 0;
        else if (B < 28)
          Bp = B - 8;
        else if (B < 48)
          Bp = 13 + (B >> 2);
        else
          Bp = 31;
        Ccap[0] = (ui16)(Ccap[0] | (ui16)Bp);
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
      void init(ui32 payload_length = 0, ui16 tile_idx = 0,
                ui8 tile_part_index = 0, ui8 num_tile_parts = 0)
      {
        Lsot = 10;
        Psot = payload_length + 12; //total = payload + SOT marker
        Isot = tile_idx;
        TPsot = tile_part_index;
        TNsot = num_tile_parts;
      }

      bool write(outfile_base *file, ui32 payload_len);
      bool write(outfile_base *file, ui32 payload_len, ui8 TPsot, ui8 TNsot);
      bool read(infile_base *file, bool resilient);

      ui16 get_tile_index() const { return Isot; }
      ui32 get_payload_length() const { return Psot > 0 ? Psot - 12 : 0; }
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
      void init(ui32 num_pairs, Ttlm_Ptlm_pair* store);

      void set_next_pair(ui16 Ttlm, ui32 Ptlm);
      bool write(outfile_base *file);

    private:
      ui16 Ltlm;
      ui8 Ztlm;
      ui8 Stlm;
      Ttlm_Ptlm_pair* pairs;
      ui32 num_pairs;
      ui32 next_pair_index;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    ///////////////////////////////////////////////////////////////////////////
    struct param_dfs
    {
    public:
      enum dfs_dwt_type : ui8 {
        NO_DWT    = 0,  // no wavelet transform
        BIDIR_DWT = 1,  // bidirectional DWT (this the conventional DWT)
        HORZ_DWT  = 2,  // horizontal only DWT transform
        VERT_DWT  = 3,  // vertical only DWT transform
      };

    public: // member functions
      param_dfs() { memset(this, 0, sizeof(param_dfs)); }
      ~param_dfs() { if (next) delete next; }
      void init() { memset(this, 0, sizeof(param_dfs)); }
      bool read(infile_base *file);
      bool exists() const { return Ldfs != 0; }

      // get_dfs return a dfs structure Sdfs == index, or NULL if not found
      const param_dfs* get_dfs(int index) const;
      // decomp_level is the decomposition level, starting from 1 for highest
      // resolution to num_decomps for the coarsest resolution
      dfs_dwt_type get_dwt_type(ui32 decomp_level) const;
      ui32 get_subband_idx(ui32 num_decompositions, ui32 resolution,
                           ui32 subband) const;
      point get_res_downsamp(ui32 skipped_resolutions) const;

    private: // member variables
      ui16 Ldfs;       // length of the segment marker
      ui16 Sdfs;       // index of this DFS marker segment
      ui8 Ids;         // number of elements in Ddfs, 2 bits per sub-level
      ui8 Ddfs[8];     // a string defining number of decomposition sub-levels
                       // 8 bytes should be enough for 32 levels
      param_dfs* next; // used for linking other dfs segments
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    ///////////////////////////////////////////////////////////////////////////
    // data structures used by param_atk

    union lifting_step {
      struct irv_data {
        // si8 Oatk;     // only for arbitrary filter
        // ui8 LCatk;    // number of lifting coefficients in a step
        float Aatk;      // lifting coefficient
      };

      struct rev_data {
        // si8 Oatk;     // only for arbitrary filter, offset of filter
        ui8 Eatk;        // only for reversible, epsilon, the power of 2
        si16 Batk;       // only for reversible, beta, the additive residue
        // ui8 LCatk;    // number of lifting coefficients in a step
        si16 Aatk;       // lifting coefficient
      };

      irv_data irv;
      rev_data rev;
    };

    struct param_atk
    {
      // Limitations:
      // Arbitrary filters (ARB) are not supported
      // Up to 6 steps are supported -- more than 6 are not supported
      // Only one coefficient per step -- first order filter
      // Only even-indexed subsequence in first reconstruction step,
      //   m_init = 0 is supported

    public: // member functions
      param_atk() { init(); }
      ~param_atk() {
        if (next && alloced_next) {
          delete next;
          next = NULL;
        }
        if (d != NULL && d != d_store) {
          delete[] d;
          init(false);
        }
      }
      bool read(infile_base *file);
      bool read_coefficient(infile_base *file, float &K);
      bool read_coefficient(infile_base *file, si16 &K);
      void init(bool clear_all = true) { 
        if (clear_all)
          memset(this, 0, sizeof(param_atk));
        d = d_store; max_steps = sizeof(d_store) / sizeof(lifting_step);
      }
      void init_irv97();
      void init_rev53();
      void link(param_atk* next) 
      { assert(this->next == NULL); this->next = next; alloced_next = false; }

      ui8 get_index() const { return (ui8)(Satk & 0xFF); }
      int get_coeff_type() const { return (Satk >> 8) & 0x7; }
      bool is_whole_sample() const { return (Satk & 0x800) != 0; }
      bool is_reversible() const { return (Satk & 0x1000) != 0; }
      bool is_m_init0() const { return (Satk & 0x2000) == 0; }
      bool is_using_ws_extension() const { return (Satk & 0x4000) != 0; }
      const param_atk* get_atk(int index) const;
      const lifting_step* get_step(ui32 s) const 
      { assert(s < Natk); return d + s; }
      ui32 get_num_steps() const { return Natk; }
      float get_K() const { return Katk; }

    private: // member variables
      ui16 Latk;         // structure length
      ui16 Satk;         // carries a variety of information
      float Katk;        // only for irreversible scaling factor K
      ui8 Natk;          // number of lifting steps
      lifting_step* d;   // pointer to data, initialized to d_store
      int max_steps;     // maximum number of steps without memory allocation
      lifting_step d_store[6];   // lifting step coefficient
      param_atk* next;   // used for chaining if more than one atk segment
                         // exist in the codestream
      bool alloced_next; // true if next was allocated, not just set to an
                         // existing object
    };
  } // !local namespace
} // !ojph namespace

#endif // !OJPH_PARAMS_LOCAL_H
