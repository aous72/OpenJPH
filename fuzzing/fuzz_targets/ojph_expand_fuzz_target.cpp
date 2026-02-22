//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2019, Aous Naman 
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
// File: ojph_expand_fuzz_target.cpp
// Author: Pierre-Anthony Lemieux
// Date: 17 February 2026
//***************************************************************************/

#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include <ojph_arch.h>
#include <ojph_file.h>
#include <ojph_params.h>
#include <ojph_mem.h>
#include <ojph_codestream.h>
#include <ojph_message.h>
#include <exception>
#include <iostream>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
  try
  {

    ojph::mem_infile infile;
    infile.open(reinterpret_cast<const ojph::ui8 *>(Data), Size);

    ojph::codestream cs;
    cs.read_headers(&infile);

    cs.create();

    if (cs.is_planar())
    {
      ojph::param_siz siz = cs.access_siz();
      for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
      {
        ojph::ui32 height = siz.get_recon_height(c);
        for (ojph::ui32 i = height; i > 0; --i)
        {
          ojph::ui32 comp_num;
          cs.pull(comp_num);
          assert(comp_num == c);
        }
      }
    }
    else
    {
      ojph::param_siz siz = cs.access_siz();
      ojph::ui32 height = siz.get_recon_height(0);
      for (ojph::ui32 i = 0; i < height; ++i)
      {
        for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
        {
          ojph::ui32 comp_num;
          cs.pull(comp_num);
          assert(comp_num == c);
        }
      }
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

#ifdef OJPH_FUZZ_TARGET_MAIN
int main(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    FILE *f = fopen(argv[i], "rb");
    if (!f) { perror(argv[i]); continue; }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    std::vector<uint8_t> buf(len);
    fread(buf.data(), 1, len, f);
    fclose(f);
    LLVMFuzzerTestOneInput(buf.data(), buf.size());
  }
  return 0;
}
#endif