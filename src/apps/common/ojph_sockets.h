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
// File: ojph_socket.h
// Author: Aous Naman
// Date: 17 April 2024
//***************************************************************************/

#ifndef OJPH_SOCKET_H
#define OJPH_SOCKET_H

#include <string>
#include "ojph_arch.h"

#ifdef OJPH_OS_WINDOWS
	#include <winsock2.h>
	#include <WS2tcpip.h>

  typedef SOCKET ojph_socket;
  #define OJPH_INVALID_SOCKET (INVALID_SOCKET)
  #define OJPH_EWOULDBLOCK (WSAEWOULDBLOCK)
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <errno.h>
  #include <fcntl.h>

  typedef int ojph_socket;
  #define OJPH_INVALID_SOCKET (-1)
  #define OJPH_EWOULDBLOCK (EWOULDBLOCK)
#endif

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

    //************************************************************************/
    /** @brief A small wrapper for socket that only abstract Winsock2
     * 
     *  This is a small wrapper that only abstracts the difference between 
     *  Windows and Linux/MacOS socket implementations.
     *  It does not not do much other than carry int for Linux/OS and 
     *  SOCKET for Windows, which is unsigned int/int64.
     */
    class socket {
    public:
      socket() { s = OJPH_INVALID_SOCKET; }
      socket(ojph_socket s);
      void close();
      bool set_blocking_mode(bool block);

      ojph_socket intern() { return s; }

    private:
      ojph_socket s;  //!<int for Linux/MacOS and SOCKET for Windows>
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    ///////////////////////////////////////////////////////////////////////////

    //************************************************************************/
    /** @brief A small wrapper for some Winsock2 functionality
     * 
     *  This is useful for windows, as it initializes and destroys 
     *  WinSock2 library.
     *  It keeps a count of how many times the constructor is called, 
     *  reducing the count whenever the destructor is called.  When the 
     *  count reaches zero, the library is destroyed -- Windows only.
     * 
     *  It also allows the creation of a socket, access to the last error 
     *  in a portable way, and the translation of an error into a text 
     *  message.
     */
    class socket_manager {
    public:
      socket_manager();
      ~socket_manager();

      socket create_socket(int domain, int type, int protocol);
      int get_last_error();
      std::string get_error_message(int errnum);
      std::string get_last_error_message();
      ui32 get_addr(const sockaddr_in& addr);
    };

  } // !net namespace
} // !ojph namespace



#endif // !OJPH_SOCKET_H