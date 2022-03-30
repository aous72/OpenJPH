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
// File: ojph_file.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_FILE_H
#define OJPH_FILE_H

#include <cstdlib>
#include <cstdio>

#include "ojph_arch.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
#ifdef OJPH_OS_WINDOWS
  int inline ojph_fseek(FILE* stream, si64 offset, int origin)
  {
    return _fseeki64(stream, offset, origin);
  }

  si64 inline ojph_ftell(FILE* stream)
  {
    return _ftelli64(stream);
  }
#else
  int inline ojph_fseek(FILE* stream, si64 offset, int origin)
  {
    return fseeko(stream, offset, origin);
  }

  si64 inline ojph_ftell(FILE* stream)
  {
    return ftello(stream);
  }
#endif


  ////////////////////////////////////////////////////////////////////////////
  class outfile_base
  {
  public:

    virtual ~outfile_base() {}

    virtual size_t write(const void *ptr, size_t size) = 0;
    virtual si64 tell() { return 0; }
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
    virtual si64 tell();
    OJPH_EXPORT
    virtual void flush();
    OJPH_EXPORT
    virtual void close();

  private:
    FILE *fh;
  };

  //*************************************************************************/
  /**  @brief mem_outfile stores encoded j2k codestreams in memory
   *
   *  This code was first developed by Chris Hafey https://github.com/chafey
   *  I took the code and integrated with OpenJPH, with some modifications.
   *
   *  This class serves as a memory-based file storage.
   *  For example, generated j2k codestream is stored in memory
   *  instead of a conventional file. The memory buffer associated with
   *  this class grows with the addition of new data.
   *
   *  memory data can be accessed using get_data()
   */
  class mem_outfile : public outfile_base
  {
  public:
    /**  A constructor */
    OJPH_EXPORT
    mem_outfile();
    /**  A destructor */
    OJPH_EXPORT
    ~mem_outfile();

    /**  Call this function to open a memory file.
	 *
     *  This function creates a memory buffer to be used for storing
     *  the generated j2k codestream.
     *
     *  @param initial_size is the initial memory buffer size.
     *         The default value is 2^16.
     */
    OJPH_EXPORT
    void open(size_t initial_size = 65536);

    /**  Call this function to write data to the memory file.
	 *
     *  This function adds new data to the memory file.  The memory buffer
     *  of the file grows as needed.
     *
     *  @param ptr is the address of the new data.
     *  @param size the number of bytes in the new data.
     */
    OJPH_EXPORT
    virtual size_t write(const void *ptr, size_t size);

    /** Call this function to know the file size (i.e., number of bytes used
     *  to store the file).
     *
     *  @return the file size.
     */
    OJPH_EXPORT
    virtual si64 tell() { return cur_ptr - buf; }

    /** Call this function to close the file and deallocate memory
	 *
     *  The object can be used again after calling close
     */
    OJPH_EXPORT
    virtual void close();

    /** Call this function to access memory file data.
	 *
     *  It is not recommended to store the returned value because buffer
     *  storage address can change between write calls.
     *
     *  @return a constant pointer to the data.
     */
    OJPH_EXPORT
    const ui8* get_data() { return buf; }

    /** Call this function to access memory file data (for const objects)
	 *
     *  This is similar to the above function, except that it can be used
     *  with constant objects.
     *
     *  @return a constant pointer to the data.
     */
    OJPH_EXPORT
    const ui8* get_data() const { return buf; }

  private:
    bool is_open;
    size_t buf_size;
    ui8 *buf;
    ui8 *cur_ptr;
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
    virtual int seek(si64 offset, enum infile_base::seek origin) = 0;
    virtual si64 tell() = 0;
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
    virtual int seek(si64 offset, enum infile_base::seek origin);
    OJPH_EXPORT
    virtual si64 tell();
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
    virtual int seek(si64 offset, enum infile_base::seek origin);
    OJPH_EXPORT
    virtual si64 tell() { return cur_ptr - data; }
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
