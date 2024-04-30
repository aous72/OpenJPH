//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2024, Aous Naman
// Copyright (c) 2024, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2024, The University of New South Wales, Australia
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
// File: ojph_socket.cpp
// Author: Aous Naman
// Date: 17 April 2024
//***************************************************************************/

#include <cassert>
#include <string.h>
#include "ojph_message.h"
#include "ojph_sockets.h"

namespace ojph
{
  namespace net 
  {

    ///////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    socket::socket(const ojph_socket& s)
    {
      this->s = s;
    }

    ///////////////////////////////////////////////////////////////////////////
    void socket::close()
    {

      if (s != OJPH_INVALID_SOCKET)
      {
      #ifdef OJPH_OS_WINDOWS
        ::closesocket(s);
      #else
        ::close(s);
      #endif
        s = OJPH_INVALID_SOCKET;      
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    bool socket::set_blocking_mode(bool block)
    {
    #ifdef OJPH_OS_WINDOWS
      u_long mode = block ? 0 : 1;
      return ioctlsocket(s, FIONBIO, &mode) == 0;
    #else
      int flags = fcntl(s, F_GETFL);
      if (flags == -1) // error
        return false;
      if (block)
        flags &= ~O_NONBLOCK;
      else
        flags |= O_NONBLOCK;
      return fcntl(s, F_SETFL, flags) != -1;
    #endif  
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    int socket_manager::ojph_socket_manager_counter = 0;

    ///////////////////////////////////////////////////////////////////////////
    socket_manager::socket_manager()
    {
      if (ojph_socket_manager_counter == 0)
      {
    #ifdef OJPH_OS_WINDOWS
      WSADATA wsa;
      if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
      {
        std::string err = get_last_error_message();
        OJPH_ERROR(0x00080001, "Could not create socket : %s\n", err.data());
      }
    #endif
      }
      ++ojph_socket_manager_counter;
    }

    ///////////////////////////////////////////////////////////////////////////
    socket_manager::~socket_manager()
    {
      assert(ojph_socket_manager_counter >= 1);
      --ojph_socket_manager_counter;
      if (ojph_socket_manager_counter == 0)
      {
      #ifdef _MSC_VER  
      	WSACleanup();
      #endif
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    socket socket_manager::create_socket(int domain, int type, int protocol)
    {
      socket s(::socket(domain, type, protocol));
      return s;
    }

    ///////////////////////////////////////////////////////////////////////////
    int socket_manager::get_last_error()
    {
    #ifdef OJPH_OS_WINDOWS
      return WSAGetLastError();
    #else
      return errno;
    #endif
    }

    ///////////////////////////////////////////////////////////////////////////
    std::string socket_manager::get_error_message(int errnum)
    {
      if( errnum == 0 )
        return std::string("");
      const int max_buf_size = 1024;
      char buf[max_buf_size]; 
      char *v = buf;
    #ifdef OJPH_OS_WINDOWS
      size_t size = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM
                                  | FORMAT_MESSAGE_IGNORE_INSERTS,
                                  NULL, errnum, 
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                                  buf, max_buf_size, NULL);
      buf[max_buf_size - 1] = 0;
    #elif (defined OJPH_OS_APPLE) || \
      ((_POSIX_C_SOURCE >= 200112L) && !_GNU_SOURCE)
      // it is not clear if the returned value is in buf or in v
      int t = strerror_r(errnum, (char*)buf, max_buf_size);
      if (t != 0)
        OJPH_ERROR(0x00080002, "Error retrieving a text message for "
          "socket error number %d\n", errnum);
      buf[max_buf_size - 1] = 0;
    #else
      v = strerror_r(errnum, (char*)buf, max_buf_size);
    #endif
      std::string str;
      str = v;    
      return str;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::string socket_manager::get_last_error_message()
    {
      int errnum = get_last_error();
      return get_error_message(errnum);
    }

    ///////////////////////////////////////////////////////////////////////////
    ui32 socket_manager::get_addr(const sockaddr_in& addr)
    {
    #ifdef OJPH_OS_WINDOWS
      return addr.sin_addr.S_un.S_addr;
    #else
      return addr.sin_addr.s_addr;
    #endif
    }

  } // !net namespace 
} // !ojph namespace 
