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
// File: ojph_file.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


/** @file ojph_file.cpp
 *  @brief contains implementations of classes related to file operations
 */

#include <cassert>
#include <cstddef>

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
  si64 j2c_outfile::tell()
  {
    assert(fh);
    return ojph_ftell(fh);
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

  //*************************************************************************/
  // mem_outfile
  //*************************************************************************/

  /**  */
  mem_outfile::mem_outfile()
  {
    is_open = false;
    buf_size = 0;
    buf = cur_ptr = NULL;
  }

  /**  */
  mem_outfile::~mem_outfile()
  {
    close();
  }

  /**  */
  void mem_outfile::open(size_t initial_size /* = 65536 */)
  {
    assert(this->is_open == false);
    assert(this->buf_size == 0);
    assert(this->buf == NULL);
    assert(this->cur_ptr == NULL);

    // do initial buffer allocation
    this->is_open = true;
    this->buf_size = initial_size;
    if (initial_size)
      this->buf = (ui8*)malloc(this->buf_size);
    this->cur_ptr = this->buf;
  }

  /**  */
  void mem_outfile::close() {
    if (buf)
      free(buf);
    is_open = false;
    buf_size = 0;
    buf = cur_ptr = NULL;
  }

  /** The function starts with a buffer size of 65536.  Then, whenever the
   *  need arises, this buffer is expanded by a factor approx 1.5x
   */
  size_t mem_outfile::write(const void *ptr, size_t size)
  {
    assert(this->is_open);
    assert(this->buf_size);
    assert(this->buf);
    assert(this->cur_ptr);

    // expand buffer if needed to make sure it has room for this write
    si64 used_size = tell(); //current used size
    size_t new_used_size = (size_t)used_size + size; //needed size
    if (new_used_size > this->buf_size) //only expand when there is need
    {
      size_t new_buf_size = this->buf_size;
      while (new_used_size > new_buf_size)
        new_buf_size += new_buf_size >> 1; //expand by ~1.5x

      this->buf = (ui8*)realloc(this->buf, new_buf_size);
      this->buf_size = new_buf_size;
      this->cur_ptr = this->buf + used_size;
    }

    // copy bytes into buffer and adjust cur_ptr
    memcpy(this->cur_ptr, ptr, size);
    cur_ptr += size;

    return size;
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
  int j2c_infile::seek(si64 offset, enum infile_base::seek origin)
  {
    assert(fh);
    return ojph_fseek(fh, offset, origin);
  }

  ////////////////////////////////////////////////////////////////////////////
  si64 j2c_infile::tell()
  {
    assert(fh);
    return ojph_ftell(fh);
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
    std::ptrdiff_t bytes_left = (data + this->size) - cur_ptr;
    if (bytes_left > 0)
    {
      size_t bytes_to_read = ojph_min(size, (size_t)bytes_left);
      memcpy(ptr, cur_ptr, bytes_to_read);
      cur_ptr += bytes_to_read;
      return bytes_to_read;
    }
    else
      return 0;
  }

  ////////////////////////////////////////////////////////////////////////////
  int mem_infile::seek(si64 offset, enum infile_base::seek origin)
  {
    int result = -1;
    if (origin == OJPH_SEEK_SET)
    {
      if (offset >= 0 && (size_t)offset <= size)
      {
        cur_ptr = data + offset;
        result = 0;
      }
    }
    else if (origin == OJPH_SEEK_CUR)
    {
      std::ptrdiff_t bytes_off = cur_ptr - data; bytes_off += offset;
      if (bytes_off >= 0 && (size_t)bytes_off <= size)
      {
        cur_ptr = data + bytes_off;
        result = 0;
      }
    }
    else if (origin == OJPH_SEEK_END)
    {
      if (offset <= 0 && (std::ptrdiff_t)size + offset >= 0)
      {
        cur_ptr = data + size + offset;
        result = 0;
      }
    }
    else
      assert(0);

    return result;
  }


}
