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
// File: ojph_codestream.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_CODESTREAM_H
#define OJPH_CODESTREAM_H

#include <cstdlib>

#include "ojph_defs.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //local prototyping
  namespace local {
    class codestream;
  };

  ////////////////////////////////////////////////////////////////////////////
  //defined elsewhere
  class param_siz;
  class param_cod;
  class param_qcd;
  class mem_fixed_allocator;
  struct point;
  struct line_buf;
  class outfile_base;
  class infile_base;

  ////////////////////////////////////////////////////////////////////////////
  class codestream
  {
  public:
    OJPH_EXPORT
    codestream();
    OJPH_EXPORT
    ~codestream();

    OJPH_EXPORT
    void set_planar(bool planar);
    OJPH_EXPORT
    void set_profile(const char* s);

    OJPH_EXPORT
    void write_headers(outfile_base *file);
    OJPH_EXPORT
    line_buf* exchange(line_buf* line, ui32& next_component);
    OJPH_EXPORT
    void flush();

    OJPH_EXPORT
    void enable_resilience();             // before read_headers
    OJPH_EXPORT
    void read_headers(infile_base *file); // before resolution restrictions
    OJPH_EXPORT
    void restrict_input_resolution(ui32 skipped_res_for_data,
      ui32 skipped_res_for_recon);         // before create
    OJPH_EXPORT
    void create(); 
    OJPH_EXPORT
    line_buf* pull(ui32 &comp_num);

    OJPH_EXPORT
    void close();

    OJPH_EXPORT
    param_siz access_siz();
    OJPH_EXPORT
    param_cod access_cod();
    OJPH_EXPORT
    param_qcd access_qcd();
    OJPH_EXPORT
    bool is_planar() const;

  private:
    local::codestream* state;
  };

}

#endif // !OJPH_CODESTREAM_H
