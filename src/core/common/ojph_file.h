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
// File: ojph_file.h
// Author: Aous Naman
// Date: 28 August 2019
/****************************************************************************/


#ifndef OJPH_FILE_H
#define OJPH_FILE_H

#include <cstdlib>
#include <cstdio>

#include "ojph_arch.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  class outfile_base
  {
  public:

    virtual ~outfile_base() {}

    virtual size_t write(const void *ptr, size_t size) = 0;
    virtual void flush() {}
    virtual void close() {}
  };

  ////////////////////////////////////////////////////////////////////////////
  class j2c_outfile : public outfile_base
  {
  public:
    OJPH_EXPORT
    j2c_outfile() { fh = 0; }
    OJPH_EXPORT
    ~j2c_outfile() { if (fh) fclose(fh); }

    OJPH_EXPORT
    void open(const char *filename);
    OJPH_EXPORT
    virtual size_t write(const void *ptr, size_t size);
    OJPH_EXPORT
    virtual void flush();
    OJPH_EXPORT
    virtual void close();

  private:
    FILE *fh;

  };


  ////////////////////////////////////////////////////////////////////////////
  class mem_outfile : public outfile_base
  {
  public:
    OJPH_EXPORT
    mem_outfile() {data = NULL; cur_ptr = NULL; size =0; }
    OJPH_EXPORT
    ~mem_outfile() { delete data; data = NULL; cur_ptr =NULL; size = 0; }

    OJPH_EXPORT
    void open();
    OJPH_EXPORT
    virtual size_t write(const void *ptr, size_t size);
    OJPH_EXPORT
    virtual void flush() {}
    OJPH_EXPORT
    virtual void close() {}

    OJPH_EXPORT
    ui8* get_data() {return data;}
    size_t get_size() const {return cur_ptr - data;}

  private:
    ui8 *data, *cur_ptr;
    size_t size;
  };

  ////////////////////////////////////////////////////////////////////////////
  class infile_base
  {
  public:
    enum seek : int {
      OJPH_SEEK_SET = SEEK_SET,
      OJPH_SEEK_CUR = SEEK_CUR,
      OJPH_SEEK_END = SEEK_END
    };

    virtual ~infile_base() {}

    //read reads size bytes, returns the number of bytes read
    virtual size_t read(void *ptr, size_t size) = 0;
    //seek returns 0 on success
    virtual int seek(long int offset, enum seek origin) = 0;
    virtual long tell() = 0;
    virtual bool eof() = 0;
    virtual void close() {}
  };

  ////////////////////////////////////////////////////////////////////////////
  class j2c_infile : public infile_base
  {
  public:
    OJPH_EXPORT
    j2c_infile() { fh = 0; }
    OJPH_EXPORT
    ~j2c_infile() { if (fh) fclose(fh); }

    OJPH_EXPORT
    void open(const char *filename);

    //read reads size bytes, returns the number of bytes read
    OJPH_EXPORT
    virtual size_t read(void *ptr, size_t size);
    //seek returns 0 on success
    OJPH_EXPORT
    virtual int seek(long int offset, enum seek origin);
    OJPH_EXPORT
    virtual long tell();
    OJPH_EXPORT
    virtual bool eof() { return feof(fh) != 0; }
    OJPH_EXPORT
    virtual void close();

  private:
    FILE *fh;

  };

  ////////////////////////////////////////////////////////////////////////////
  class mem_infile : public infile_base
  {
  public:
    OJPH_EXPORT
    mem_infile() { close(); }
    OJPH_EXPORT
    ~mem_infile() { }

    OJPH_EXPORT
    void open(const ui8* data, size_t size);

    //read reads size bytes, returns the number of bytes read
    OJPH_EXPORT
    virtual size_t read(void *ptr, size_t size);
    //seek returns 0 on success
    OJPH_EXPORT
    virtual int seek(long int offset, enum seek origin);
    OJPH_EXPORT
    virtual long tell() { return cur_ptr - data; }
    OJPH_EXPORT
    virtual bool eof() { return cur_ptr >= data + size; }
    OJPH_EXPORT
    virtual void close() { data = cur_ptr = NULL; size = 0; }

  private:
    const ui8 *data, *cur_ptr;
    size_t size;

  };


}

#endif // !OJPH_FILE_H
