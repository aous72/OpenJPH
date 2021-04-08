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
// This file is part of the testing routines for the OpenJPH software 
// implementation.
// File: psnr_pae.cpp
// Author: Aous Naman
// Date: 18 March 2021
//***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include "../common/ojph_img_io.h"
#include "../common/ojph_mem.h"

using namespace ojph;

enum : ui32 {
  UNDEFINED = 0,
  FORMAT444 = 1,
  FORMAT422 = 2,
  FORMAT420 = 3,
};

struct img_info {
  img_info() { 
    num_comps = 0; 
    comp[0] = comp[1] = comp[2] = 0;
  }
  ~img_info() {
    for (ui32 i = 0; i < num_comps; ++i)
      if (comp[i]) delete[] comp[i];
  }
  
  void init(ui32 num_comps, ui32 width, ui32 height, ui32 format)
  {
  }
  
  ui32 num_comps;
  ui32 width, height;
  point downsampling[3];
  si32 *comp[3];
};

//     void open(const char* filename);
//     void finalize_alloc();
//     virtual ui32 read(const line_buf* line, ui32 comp_num);
//     void close() { if(fh) { fclose(fh); fh = NULL; } fname = NULL; }
//     void set_plannar(bool planar) { this->planar = planar; }


void load_ppm(const char *filename, img_info& img)
{
  ppm_in ppm;
  ppm.open(filename);

  //img.init(, );
  line_buf line;


}

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  
  
  
  
  
  return 0;
}


