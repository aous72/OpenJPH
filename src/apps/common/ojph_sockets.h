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
  *  It does not not do much other than define a local member variable
  *  of type int for Linux/OS and type SOCKET for Windows, which is 
  *  unsigned int/int64.
  */
class socket {
public:
  /**
    *  @brief default constructor
    */
  socket() { s = OJPH_INVALID_SOCKET; }

  /**
    *  @brief a copy constructor
    */
  socket(const ojph_socket& s);

  /**
    *  @brief Abstracts socket closing function
    */
  void close();

  /**
    *  @brief Sets the blocking mode
    * 
    *  @param  block sets to true to operate in blocking mode
    *  @return returns true when the operation succeeds
    */
  bool set_blocking_mode(bool block);

  /**
    *  @brief provides access to the internal socket handle
    * 
    *  @return returns the internal socket handle
    */
  ojph_socket intern() { return s; }

private:
  ojph_socket s;  //!<int for Linux/MacOS and SOCKET for Windows
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
  /**
    *  @brief default constructor
    * 
    *  This function initializes the Winsock2 stack in windows; it 
    *  also increments the static member that keeps count of how many
    *  times this object is used.
    */
  socket_manager();

  /**
    *  @brief default constructor
    * 
    *  This function cleans up the Winsock2 stack in windows when
    *  the static member that keeps count of how many times this object 
    *  is used reaches zero.
    *
    */
  ~socket_manager();

  /**
    *  @brief Abstructs socket creation
    *
    *  This function takes the same parameters as the conventional 
    *  socket() function
    *
    *  @param domain the same as in conventional socket() function
    *  @param type the same as in conventional socket() function
    *  @param protocol the same as in conventional socket() function
    *  @return returns an abstraction of socket
    * 
    */
  socket create_socket(int domain, int type, int protocol);

  /**
    *  @brief Abstructs get last error or errno
    *
    *  This function abstracts Windows GetLastError or Linux errno
    * 
    *  @return returns a number representing the error
    *
    */
  int get_last_error();

  /**
    *  @brief Abstructs obtaining a textual message for an errnum
    *
    *  This function abstracts obtaining a textual message for an errnum
    *
    *  @param errnum the error number
    *  @return a string holding a textual message for the error number
    *
    */
  std::string get_error_message(int errnum);

  /**
    *  @brief Abstructs obtaining a textual message for GetLastError/errno
    *
    *  This function combines get_error_message() and get_last_error().
    *  This function effectively calls get_last_error() and uses the 
    *  returned error number to obtain a string by calling 
    *  get_error_message(errnum).
    *
    *  @return a string holding a textual message for the error number
    *
    */
  std::string get_last_error_message();

  /**
    *  @brief Abstractly obtains the 32-bit IPv4 address integer
    *
    *  This function obtains a 32-bit integer that represents the IPv4 
    *  address in abstrct way (working both in Windows and Linux).
    *  This is really an independent function, but it is convenient to 
    *  put it here.
    *
    *  @return returns an integer holding IPv4 address
    *
    */
  static ui32 get_addr(const sockaddr_in& addr);

private:
  static int ojph_socket_manager_counter;
};

} // !net namespace
} // !ojph namespace



#endif // !OJPH_SOCKET_H