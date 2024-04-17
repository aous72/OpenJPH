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
// File: ojph_stream_expand.cpp
// Author: Aous Naman
// Date: 17 April 2024
//***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "ojph_message.h"
#include "ojph_socket.h"

#define BUFLEN 2048	//Max length of buffer
#define PORT   8080	//The port on which to listen for incoming data

int main()
{
  ojph::net::socket_manager smanager;
	
  // create a socket
  ojph::net::socket s =
  smanager.create_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(s.intern() < 0)
  {
    std::string err = smanager.get_last_error_message();
    OJPH_ERROR(0x02000001, "Could not create socket : %s\n", err.data());
  }

  // prepare the sockaddr_in structure
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(PORT);

  //Bind
  if( bind(s.intern(), (struct sockaddr *)&server, sizeof(server)) == -1)
  {
    std::string err = smanager.get_last_error_message();
    OJPH_ERROR(0x02000002, "Could not create socket : %s\n", err.data());
  }

  // keep listening for data
  while(1)
  {
    char buf[BUFLEN];
    memset(buf, 0, BUFLEN);

    // receive data -- this is a blocking call
    struct sockaddr_in si_other;
    socklen_t socklen = sizeof(si_other);    
    int recv_len = (int)recvfrom(
      s.intern(), buf, BUFLEN, 0, (struct sockaddr *) &si_other, &socklen);
    if (recv_len < 0)
    {
      std::string err = smanager.get_last_error_message();
      OJPH_ERROR(0x02000003, "Could not create socket : %s\n", err.data());
    }

    // print details of the client/peer and the data received
    char src_addr[1024];
    inet_ntop(AF_INET, &si_other.sin_addr, src_addr, sizeof(src_addr));
    printf("Received packet from %s:%d .. ", src_addr, ntohs(si_other.sin_port));
    printf("Data: %02x\n" , buf[0]);
  }

  s.close();
  return 0;
}

