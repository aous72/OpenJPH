/****************************************************************************/
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
/****************************************************************************/
// This file is part of the OpenJPH software implementation.
// File: ojph_file.cpp
// Author: Aous Naman
// Date: 28 August 2019
/****************************************************************************/


#include <cassert>

#include "ojph_file.h"
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
  void j2c_outfile::open(const char *filename)
  {
    assert(fh == 0);
    fh = fopen(filename, "wb");
    if (fh == NULL)
      OJPH_ERROR(0x00060001, "failed to open %s for writing", filename);
  }

  ////////////////////////////////////////////////////////////////////////////
  size_t j2c_outfile::write(const void *ptr, size_t size)
  {
    assert(fh);
    return fwrite(ptr, 1, size, fh);
  }

  ////////////////////////////////////////////////////////////////////////////
  void j2c_outfile::flush()
  {
    assert(fh);
    fflush(fh);
  }

  ////////////////////////////////////////////////////////////////////////////
  void j2c_outfile::close()
  {
    assert(fh);
    fclose(fh);
    fh = NULL;
  }


  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void j2c_infile::open(const char *filename)
  {
    assert(fh == NULL);
    fh = fopen(filename, "rb");
    if (fh == NULL)
      OJPH_ERROR(0x00060002, "failed to open %s for reading", filename);
  }

  ////////////////////////////////////////////////////////////////////////////
  size_t j2c_infile::read(void *ptr, size_t size)
  {
    assert(fh);
    return fread(ptr, 1, size, fh);
  }

  ////////////////////////////////////////////////////////////////////////////
  int j2c_infile::seek(long int offset, enum seek origin)
  {
    assert(fh);
    return fseek(fh, offset, origin);
  }

  ////////////////////////////////////////////////////////////////////////////
  long j2c_infile::tell()
  {
    assert(fh);
    return ftell(fh);
  }

  ////////////////////////////////////////////////////////////////////////////
  void j2c_infile::close()
  {
    assert(fh);
    fclose(fh);
    fh = NULL;
  }


  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void mem_infile::open(const ui8* data, size_t size)
  {
    assert(this->data == NULL);
    cur_ptr = this->data = data;
    this->size = size;
  }

  ////////////////////////////////////////////////////////////////////////////
  size_t mem_infile::read(void *ptr, size_t size)
  {
    size_t num_bytes = ojph_min(size, data + this->size - cur_ptr);
    memcpy(ptr, cur_ptr, num_bytes);
    cur_ptr += num_bytes;
    return num_bytes;
  }

  ////////////////////////////////////////////////////////////////////////////
  int mem_infile::seek(long int offset, enum seek origin)
  {
    int result = -1;
    if (origin == OJPH_SEEK_SET)
    {
      if (offset >= 0 && offset <= size)
      {
        cur_ptr = data + offset;
        result = 0;
      }
    }
    else if (origin == OJPH_SEEK_CUR)
    {
      size_t bytes_off = cur_ptr - data + offset;
      if (bytes_off >= 0 && bytes_off <= size)
      {
        cur_ptr = data + bytes_off;
        result = 0;
      }
    }
    else if (origin == OJPH_SEEK_END)
    {
      if (offset <= 0 && size + offset >= 0)
      {
        cur_ptr = data + size + offset;
        result = 0;
      }
    }
    else
      assert(0);

    return result;
  }


  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void mem_outfile::open()
  {
    assert(this->data == NULL);
    cur_ptr = this->data = data;
    this->size = size;
  }

  size_t mem_outfile::write(const void *ptr, size_t size) 
  {
      // ensure buffer is big enough for write
      size_t current_size = this->get_size();
      this->data = (ui8*)realloc(this->data, current_size + size);
      this->cur_ptr = this->data + current_size;

      // copy bytes into buffer and adjust cur_ptr
      memcpy(cur_ptr, ptr, size);
      cur_ptr += size;
  }

}

