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
      param_qcd access_qcd()
      { return qcd; }
      mem_fixed_allocator* get_allocator() { return allocator; }
      mem_elastic_allocator* get_elastic_alloc() { return elastic_alloc; }
      outfile_base* get_file() { return outfile; }

      line_buf* exchange(line_buf* line, int& next_component);
      void write_headers(outfile_base *file);
      void enable_resilience();
      bool is_resilient() { return resilient; }
      void read_headers(infile_base *file);
      void restrict_input_resolution(int skipped_res_for_data,
        int skipped_res_for_recon);
      void read();
      void set_planar(int planar);
      void set_profile(const char *s);
      line_buf* pull(int &comp_num);
      void flush();
      void close();

      bool is_planar() const { return planar != 0; }
      si32 get_profile() const { return profile; };

      void check_imf_validity();
      void check_boardcast_validity();

      ui8* get_precinct_scratch() { return precinct_scratch; }
      int get_skipped_res_for_recon()
      { return skipped_res_for_recon; }
      int get_skipped_res_for_read()
      { return skipped_res_for_read; }

    private:
      int precinct_scratch_needed_bytes;
      ui8* precinct_scratch;

    private:
      int cur_line;
      int cur_comp;
      int cur_tile_row;
      bool resilient;
      int skipped_res_for_read, skipped_res_for_recon;

    private:
      size num_tiles;
      tile *tiles;
      line_buf* line;
      int num_comps;
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
                          int tile_idx, int offset);

      bool push(line_buf *line, int comp_num);
      void prepare_for_flush();
      void fill_tlm(param_tlm* tlm);
      void flush(outfile_base *file);
      void parse_tile_header(const param_sot& sot, infile_base *file,
                             const ui64& tile_start_location);
      bool pull(line_buf *, int comp_num);

    private:
      codestream *parent;
      rect tile_rect, recon_tile_rect;
      int num_comps;
      tile_comp *comps;
      int num_lines;
      line_buf* lines;
      bool reversible, employ_color_transform, resilient;
      rect *comp_rects, *recon_comp_rects;
      int *line_offsets;
      int skipped_res_for_read;

      int *num_bits;
      bool *is_signed;
      int *cur_line;
      int prog_order;

    private:
      param_sot sot;
      int next_tile_part;

    private:
      int profile;
      int *num_comp_bytes; //this for use with TLM
    };

    //////////////////////////////////////////////////////////////////////////
    class tile_comp
    {
    public:
      static void pre_alloc(codestream *codestream, const rect& comp_rect,
                            const rect& recon_comp_rect);
      void finalize_alloc(codestream *codestream, tile *parent,
                          int comp_num, const rect& comp_rect,
                          const rect& recon_comp_rect);

      int get_num_resolutions() { return num_decomps + 1; }
      int get_num_decompositions() { return num_decomps; }
      line_buf* get_line();
      void push_line();
      line_buf* pull_line();

      ui32 prepare_precincts();
      void write_precincts(int res_num, outfile_base *file);
      bool get_top_left_precinct(int res_num, point &top_left);
      void write_one_precinct(int res_num, outfile_base *file);
      void parse_precincts(int res_num, ui32& data_left, infile_base *file);
      void parse_one_precinct(int res_num, ui32& data_left, infile_base *file);

    private:
      tile *parent_tile;
      resolution *res;
      rect comp_rect;
      ojph::point comp_downsamp;
      int num_decomps;
      int comp_num;
    };

    //////////////////////////////////////////////////////////////////////////
    class resolution
    {
    public:

    public:
      static void pre_alloc(codestream *codestream, const rect& res_rect,
                            const rect& recon_res_rect, int res_num);
      void finalize_alloc(codestream *codestream, const rect& res_rect,
                          const rect& recon_res_rect,
                          int res_num, point comp_downsamp,
                          tile_comp *parent_tile,
                          resolution *parent_res);

      line_buf* get_line() { return lines + 0; }
      void push_line();
      line_buf* pull_line();
      rect get_rect() { return res_rect; }

      ui32 prepare_precinct();
      void write_precincts(outfile_base *file);
      bool get_top_left_precinct(point &top_left);
      void write_one_precinct(outfile_base *file);
      resolution *next_resolution() { return child_res; }
      void parse_all_precincts(ui32& data_left, infile_base *file);
      void parse_one_precinct(ui32& data_left, infile_base *file);

    private:
      bool reversible, skipped_res_for_read, skipped_res_for_recon;
      int num_lines, num_bands, res_num;
      point comp_downsamp;
      rect res_rect;
      line_buf *lines;
      subband *bands;
      tile_comp *parent;
      resolution *parent_res, *child_res;
      //precincts stuff
      precinct *precincts;
      size num_precincts;
      size log_PP;
      int max_num_levels;
      int tag_tree_size;
      si32 level_index[20]; //more than enough
      point cur_precinct_loc; //used for progressing spatial modes (2, 3, 4)
      //wavelet machinery
      int cur_line, available_lines;
      bool vert_even, horz_even;
      mem_elastic_allocator *elastic;
    };

    //////////////////////////////////////////////////////////////////////////
    struct precinct
    {
      ui32 prepare_precinct(int tag_tree_size, si32* lev_idx,
                            mem_elastic_allocator *elastic);
      void write(outfile_base *file);
      void parse(int tag_tree_size, si32* lev_idx,
                 mem_elastic_allocator *elastic,
                 ui32& data_left, infile_base *file, bool skipped);

      ui8 *scratch;
      point img_point;   //the precinct projected to full resolution
      rect cb_idxs[4]; //indices of codeblocks
      subband *bands;  //the subbands
      coded_lists* coded;
      int num_bands;
      bool special_x, special_y;
      bool may_use_sop, uses_eph;
    };

    //////////////////////////////////////////////////////////////////////////
    class subband
    {
      friend struct precinct;
    public:
      static void pre_alloc(codestream *codestream, const rect& band_rect,
                            int res_num);
      void finalize_alloc(codestream *codestream, const rect& band_rect,
                          resolution* res, int res_num, int subband_num);

      void exchange_buf(line_buf* l);
      line_buf* get_line() { return lines; }
      void push_line();

      void get_cb_indices(const size& num_precincts, precinct *precincts);

      line_buf* pull_line();

    private:
      int res_num, band_num;
      bool reversible;
      rect band_rect;
      line_buf *lines;
      resolution* parent;
      codeblock* blocks;
      size num_blocks;
      size log_PP;
      int xcb_prime, ycb_prime;
      int cur_cb_row;
      int cur_line;
      int cur_cb_height;
      float delta, delta_inv;
      int K_max;
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
                          int K_max, int tbx0);
      void push(line_buf *line);
      void encode(mem_elastic_allocator *elastic);
      void recreate(const size& cb_size, coded_cb_header* coded_cb);

      void decode();
      void pull_line(line_buf *line);

    private:
      si32* buf;
      size nominal_size;
      size cb_size;
      subband* parent;
      int line_offset;
      int cur_line;
      int K_max;
      int max_val;
      coded_cb_header* coded_cb;
    };

    //////////////////////////////////////////////////////////////////////////
    struct coded_cb_header
    {
      int pass_length[2];
      int num_passes;
      int Kmax;
      int missing_msbs;
      coded_lists *next_coded;

      static const int prefix_buf_size = 8;
      static const int suffix_buf_size = 8;
    };

  }
}


#endif // !OJPH_CODESTREAM_LOCAL_H
