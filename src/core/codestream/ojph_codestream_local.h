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
// File: ojph_codestream_local.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_CODESTREAM_LOCAL_H
#define OJPH_CODESTREAM_LOCAL_H

#include "ojph_defs.h"
#include "ojph_file.h"
#include "ojph_params_local.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //defined elsewhere
  struct line_buf;
  class mem_fixed_allocator;
  class mem_elastic_allocator;
  class codestream;
  struct coded_lists;

  namespace local {

    //////////////////////////////////////////////////////////////////////////
    //defined here
    class tile;
    class tile_comp;
    class resolution;
    struct precinct;
    class subband;
    class codeblock;
    struct coded_cb_header;

    //////////////////////////////////////////////////////////////////////////
    class codestream
    {
      friend ::ojph::codestream;
      
    public:
      codestream();
      ~codestream();

      void pre_alloc();
      void finalize_alloc();

      ojph::param_siz access_siz()            //return externally wrapped siz
      { return ojph::param_siz(&siz); }
      const param_siz* get_siz() //return internal siz
      { return &siz; }
      ojph::param_cod access_cod()            //return externally wrapped cod
      { return ojph::param_cod(&cod); }
      const param_cod* get_cod() //return internal code
      { return &cod; }
      param_qcd* access_qcd(ui32 comp_num)
      { 
        if (used_qcc_fields > 0)
          for (int v = 0; v < used_qcc_fields; ++v)
            if (qcc[v].get_comp_num() == comp_num)
              return qcc + v;
        return &qcd; 
      }
      mem_fixed_allocator* get_allocator() { return allocator; }
      mem_elastic_allocator* get_elastic_alloc() { return elastic_alloc; }
      outfile_base* get_file() { return outfile; }

      line_buf* exchange(line_buf* line, ui32& next_component);
      void write_headers(outfile_base *file);
      void enable_resilience();
      bool is_resilient() { return resilient; }
      void read_headers(infile_base *file);
      void restrict_input_resolution(ui32 skipped_res_for_data,
        ui32 skipped_res_for_recon);
      void read();
      void set_planar(int planar);
      void set_profile(const char *s);
      line_buf* pull(ui32 &comp_num);
      void flush();
      void close();

      bool is_planar() const { return planar != 0; }
      si32 get_profile() const { return profile; };

      void check_imf_validity();
      void check_broadcast_validity();

      ui8* get_precinct_scratch() { return precinct_scratch; }
      ui32 get_skipped_res_for_recon()
      { return skipped_res_for_recon; }
      ui32 get_skipped_res_for_read()
      { return skipped_res_for_read; }

    private:
      ui32 precinct_scratch_needed_bytes;
      ui8* precinct_scratch;

    private:
      ui32 cur_line;
      ui32 cur_comp;
      ui32 cur_tile_row;
      bool resilient;
      ui32 skipped_res_for_read, skipped_res_for_recon;

    private:
      size num_tiles;
      tile *tiles;
      line_buf* lines;
      ui32 num_comps;
      size *comp_size;       //stores full resolution no. of lines and width
      size *recon_comp_size; //stores number of lines and width of each comp
      bool employ_color_transform;
      int planar;
      int profile;

    private:
      param_siz siz;
      param_cod cod;
      param_cap cap;
      param_qcd qcd;
      param_tlm tlm;

    private: // this is to handle qcc
      int used_qcc_fields;
      param_qcc qcc_store[4], *qcc; // we allocate 4, 
                                    // if not enough, we allocate more

    private:
      mem_fixed_allocator *allocator;
      mem_elastic_allocator *elastic_alloc;
      outfile_base *outfile;
      infile_base *infile;
    };

    //////////////////////////////////////////////////////////////////////////
    class tile
    {
    public:
      static void pre_alloc(codestream *codestream, const rect& tile_rect,
                            const rect& recon_tile_rect);
      void finalize_alloc(codestream *codestream, const rect& tile_rect,
                          const rect& recon_tile_rect, 
                          ui32 tile_idx, ui32 offset);

      bool push(line_buf *line, ui32 comp_num);
      void prepare_for_flush();
      void fill_tlm(param_tlm* tlm);
      void flush(outfile_base *file);
      void parse_tile_header(const param_sot& sot, infile_base *file,
                             const ui64& tile_start_location);
      bool pull(line_buf *, ui32 comp_num);
      rect get_tile_rect() { return tile_rect; }

    private:
      //codestream *parent;
      rect tile_rect, recon_tile_rect;
      ui32 num_comps;
      tile_comp *comps;
      ui32 num_lines;
      line_buf* lines;
      bool reversible, employ_color_transform, resilient;
      rect *comp_rects, *recon_comp_rects;
      ui32 *line_offsets;
      ui32 skipped_res_for_read;

      ui32 *num_bits;
      bool *is_signed;
      ui32 *cur_line;
      int prog_order;

    private:
      param_sot sot;
      int next_tile_part;

    private:
      int profile;
      ui32 *num_comp_bytes; //this for use with TLM
    };

    //////////////////////////////////////////////////////////////////////////
    class tile_comp
    {
    public:
      static void pre_alloc(codestream *codestream, const rect& comp_rect,
                            const rect& recon_comp_rect);
      void finalize_alloc(codestream *codestream, tile *parent,
                          ui32 comp_num, const rect& comp_rect,
                          const rect& recon_comp_rect);

      ui32 get_num_resolutions() { return num_decomps + 1; }
      ui32 get_num_decompositions() { return num_decomps; }
      tile* get_tile() { return parent_tile; }
      line_buf* get_line();
      void push_line();
      line_buf* pull_line();

      ui32 prepare_precincts();
      void write_precincts(ui32 res_num, outfile_base *file);
      bool get_top_left_precinct(ui32 res_num, point &top_left);
      void write_one_precinct(ui32 res_num, outfile_base *file);
      void parse_precincts(ui32 res_num, ui32& data_left, infile_base *file);
      void parse_one_precinct(ui32 res_num, ui32& data_left, 
                              infile_base *file);

    private:
      tile *parent_tile;
      resolution *res;
      rect comp_rect;
      ojph::point comp_downsamp;
      ui32 num_decomps;
      ui32 comp_num;
    };

    //////////////////////////////////////////////////////////////////////////
    class resolution
    {
    public:

    public:
      static void pre_alloc(codestream *codestream, const rect& res_rect,
                            const rect& recon_res_rect, ui32 res_num);
      void finalize_alloc(codestream *codestream, const rect& res_rect,
                          const rect& recon_res_rect, ui32 comp_num,
                          ui32 res_num, point comp_downsamp,
                          tile_comp *parent_tile_comp,
                          resolution *parent_res);

      line_buf* get_line() { return lines + 0; }
      void push_line();
      line_buf* pull_line();
      rect get_rect() { return res_rect; }
      ui32 get_comp_num() { return comp_num; }

      ui32 prepare_precinct();
      void write_precincts(outfile_base *file);
      bool get_top_left_precinct(point &top_left);
      void write_one_precinct(outfile_base *file);
      resolution *next_resolution() { return child_res; }
      void parse_all_precincts(ui32& data_left, infile_base *file);
      void parse_one_precinct(ui32& data_left, infile_base *file);

    private:
      bool reversible, skipped_res_for_read, skipped_res_for_recon;
      ui32 num_lines;
      ui32 num_bands, res_num;
      ui32 comp_num;
      point comp_downsamp;
      rect res_rect;
      line_buf *lines;
      subband *bands;
      tile_comp *parent_comp;
      resolution *parent_res, *child_res;
      //precincts stuff
      precinct *precincts;
      size num_precincts;
      size log_PP;
      ui32 max_num_levels;
      int tag_tree_size;
      ui32 level_index[20]; //more than enough
      point cur_precinct_loc; //used for progressing spatial modes (2, 3, 4)
      //wavelet machinery
      ui32 cur_line;
      bool vert_even, horz_even;
      mem_elastic_allocator *elastic;
    };

    //////////////////////////////////////////////////////////////////////////
    struct precinct
    {
      precinct() {
        scratch = NULL; bands = NULL; coded = NULL;
        num_bands = 0; may_use_sop = uses_eph = false;
      }
      ui32 prepare_precinct(int tag_tree_size, ui32* lev_idx,
                            mem_elastic_allocator *elastic);
      void write(outfile_base *file);
      void parse(int tag_tree_size, ui32* lev_idx,
                 mem_elastic_allocator *elastic,
                 ui32& data_left, infile_base *file, bool skipped);

      ui8 *scratch;
      point img_point;   //the precinct projected to full resolution
      rect cb_idxs[4]; //indices of codeblocks
      subband *bands;  //the subbands
      coded_lists* coded;
      ui32 num_bands;
      bool may_use_sop, uses_eph;
    };

    //////////////////////////////////////////////////////////////////////////
    class subband
    {
      friend struct precinct;
    public:
      static void pre_alloc(codestream *codestream, const rect& band_rect,
                            ui32 res_num);
      void finalize_alloc(codestream *codestream, const rect& band_rect,
                          resolution* res, ui32 res_num, ui32 subband_num);

      void exchange_buf(line_buf* l);
      line_buf* get_line() { return lines; }
      void push_line();

      void get_cb_indices(const size& num_precincts, precinct *precincts);
      float get_delta() { return delta; }

      line_buf* pull_line();

    private:
      ui32 res_num, band_num;
      bool reversible;
      bool empty;
      rect band_rect;
      line_buf *lines;
      resolution* parent;
      codeblock* blocks;
      size num_blocks;
      size log_PP;
      ui32 xcb_prime, ycb_prime;
      ui32 cur_cb_row;
      int cur_line;
      int cur_cb_height;
      float delta, delta_inv;
      ui32 K_max;
      coded_cb_header *coded_cbs;
      mem_elastic_allocator *elastic;
    };

    //////////////////////////////////////////////////////////////////////////
    class codeblock
    {
      friend struct precinct;
    public:
      static void pre_alloc(codestream *codestream, const size& nominal);
      void finalize_alloc(codestream *codestream, subband* parent,
                          const size& nominal, const size& cb_size,
                          coded_cb_header* coded_cb,
                          ui32 K_max, int tbx0);
      void push(line_buf *line);
      void encode(mem_elastic_allocator *elastic);
      void recreate(const size& cb_size, coded_cb_header* coded_cb);

      void decode();
      void pull_line(line_buf *line);

    private:
      ui32* buf;
      size nominal_size;
      size cb_size;
      ui32 stride;
      ui32 buf_size;
      subband* parent;
      int line_offset;
      ui32 cur_line;
      float delta, delta_inv;
      ui32 K_max;
      bool reversible;
      bool resilient;
      bool stripe_causal;
      bool zero_block; // true when the decoded block is all zero
      ui32 max_val[8]; // supports up to 256 bits
      coded_cb_header* coded_cb;

    private:
      // define function signature simple memory clearing
      typedef void (*mem_clear_fun)(void* addr, size_t count);
      // a pointer to the max value finding function
      mem_clear_fun mem_clear;
      static void gen_mem_clear(void* addr, size_t count);

      // define function signature for max value finding
      typedef ui32 (*find_max_val_fun)(ui32* addr);
      // a pointer to the max value finding function
      find_max_val_fun find_max_val;
      static ui32 gen_find_max_val(ui32* addr) { return addr[0]; }

      // define line transfer function signature from subbands to codeblocks
      typedef void (*tx_to_cb_fun)(const void *sp, ui32 *dp, ui32 K_max,
                                   float delta_inv, ui32 count, ui32* max_val);
      // a pointer to function transferring samples from subbands to codeblocks
      tx_to_cb_fun tx_to_cb;
      static void gen_rev_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                                   float delta_inv, ui32 count, ui32* max_val);
      static void gen_irv_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                                   float delta_inv, ui32 count, ui32* max_val);

      // define line transfer function signature from codeblock to subband
      typedef void (*tx_from_cb_fun)(const ui32 *sp, void *dp, ui32 K_max,
                                     float delta, ui32 count);
      // a pointer to function transferring samples from codeblocks to subbands
      tx_from_cb_fun tx_from_cb;
      static void gen_rev_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                                     float delta, ui32 count);
      static void gen_irv_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                                     float delta, ui32 count);

      // define the block decoder function signature
      typedef bool (*cb_decoder_fun)(ui8* coded_data, ui32* decoded_data,
        ui32 missing_msbs, ui32 num_passes, ui32 lengths1, ui32 lengths2,
        ui32 width, ui32 height, ui32 stride, bool stripe_causal); 
      // a pointer to the decoder function
      static cb_decoder_fun decode_cb;
    };

    //////////////////////////////////////////////////////////////////////////
    struct coded_cb_header
    {
      ui32 pass_length[2];
      ui32 num_passes;
      ui32 Kmax;
      ui32 missing_msbs;
      coded_lists *next_coded;

      static const int prefix_buf_size = 8;
      static const int suffix_buf_size = 16;
    };

    //////////////////////////////////////////////////////////////////////////
    void sse_mem_clear(void* addr, size_t count);
    void avx_mem_clear(void* addr, size_t count);
    void wasm_mem_clear(void* addr, size_t count);

    //////////////////////////////////////////////////////////////////////////
    ui32 sse2_find_max_val(ui32* address);
    ui32 avx2_find_max_val(ui32* address);
    ui32 wasm_find_max_val(ui32* address);

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                           float delta_inv, ui32 count, ui32* max_val);
    void avx2_rev_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                           float delta_inv, ui32 count, ui32* max_val);
    void sse2_irv_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                           float delta_inv, ui32 count, ui32* max_val);
    void avx2_irv_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                           float delta_inv, ui32 count, ui32* max_val);
    void wasm_rev_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                           float delta_inv, ui32 count, ui32* max_val);
    void wasm_irv_tx_to_cb(const void *sp, ui32 *dp, ui32 K_max,
                           float delta_inv, ui32 count, ui32* max_val);

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                             float delta, ui32 count);
    void avx2_rev_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                             float delta, ui32 count);
    void sse2_irv_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                             float delta, ui32 count);
    void avx2_irv_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                             float delta, ui32 count);
    void wasm_rev_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                             float delta, ui32 count);
    void wasm_irv_tx_from_cb(const ui32 *sp, void *dp, ui32 K_max,
                             float delta, ui32 count);

  }
}


#endif // !OJPH_CODESTREAM_LOCAL_H
