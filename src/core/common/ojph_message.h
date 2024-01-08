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
// File: ojph_message.h
// Author: Aous Naman
// Date: 29 August 2019
//***************************************************************************/

#ifndef OJPH_MESSAGE_H
#define OJPH_MESSAGE_H 

#include <cstring>
#include "ojph_arch.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  enum OJPH_MSG_LEVEL : int
  {
    NO_MSG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
  };

  ////////////////////////////////////////////////////////////////////////////
  class OJPH_EXPORT message_base {
  public:
      virtual void operator() (int warn_code, const char* file_name,
        int line_num, const char *fmt, ...) = 0;
  };

  ////////////////////////////////////////////////////////////////////////////
  class OJPH_EXPORT message_info : public message_base
  {
    public:
      virtual void operator() (int info_code, const char* file_name,
        int line_num, const char* fmt, ...);
  };

  ////////////////////////////////////////////////////////////////////////////
  OJPH_EXPORT
    void set_info_stream(FILE* s);
  OJPH_EXPORT
    void configure_info(message_info* info);
  OJPH_EXPORT
    message_info& get_info();

  ////////////////////////////////////////////////////////////////////////////
  class OJPH_EXPORT message_warning : public message_base
  {
    public:
      virtual void operator() (int warn_code, const char* file_name,
        int line_num, const char* fmt, ...);
  };

  ////////////////////////////////////////////////////////////////////////////
  OJPH_EXPORT
    void set_warning_stream(FILE* s);
  OJPH_EXPORT
    void configure_warning(message_warning* warn);
  OJPH_EXPORT
    message_warning& get_warning();

  ////////////////////////////////////////////////////////////////////////////
  class OJPH_EXPORT message_error : public message_base
  {
    public:
      virtual void operator() (int warn_code, const char* file_name,
        int line_num, const char *fmt, ...);
  };

  ////////////////////////////////////////////////////////////////////////////
  OJPH_EXPORT
  void set_error_stream(FILE *s);
  OJPH_EXPORT
  void configure_error(message_error* error);
  OJPH_EXPORT
  message_error& get_error();
}

//////////////////////////////////////////////////////////////////////////////
#if (defined OJPH_OS_WINDOWS)
  #define __OJPHFILE__ \
    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
  #define __OJPHFILE__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

//////////////////////////////////////////////////////////////////////////////
#define OJPH_INFO(t, ...) \
  { ojph::get_info()(t, __OJPHFILE__, __LINE__, __VA_ARGS__); }
//////////////////////////////////////////////////////////////////////////////
#define OJPH_WARN(t, ...) \
  { ojph::get_warning()(t, __OJPHFILE__, __LINE__, __VA_ARGS__); }
//////////////////////////////////////////////////////////////////////////////
#define OJPH_ERROR(t, ...) \
  { ojph::get_error()(t, __OJPHFILE__, __LINE__,__VA_ARGS__); }


#endif // !OJPH_MESSAGE_H
