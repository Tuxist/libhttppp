/*******************************************************************************
Copyright (c) 2014, Jan Koester jan.koester@gmx.net
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include <config.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>

#include "https.h"
#include "exception.h"

#ifndef SOCKET_H
#define SOCKET_H

namespace libhttppp {
class ClientSocket {
public:
    ClientSocket();
    ~ClientSocket();
    void              setnonblocking();
    SOCKET            getSocket();
    void              setSocket(SOCKET socket);
private:
    SOCKET           _Socket;
    SSL             *_SSL;
    struct sockaddr  _ClientAddr;
    socklen_t        _ClientAddrLen;
    friend class ServerSocket;
};

class ServerSocket : public HTTPS {
public:
    ServerSocket(const char *uxsocket,int maxconnections);
    ServerSocket(SOCKET socket);
    SOCKET        acceptEvent(ClientSocket *clientsocket);
    SOCKET        getSocket();
    ServerSocket(const char *addr,int port,int maxconnections);
    ~ServerSocket();

    int           getMaxconnections();
    void          setnonblocking();
    void          listenSocket();
    ssize_t       sendData(ClientSocket *socket,void *data,size_t size);
    ssize_t       sendData(ClientSocket *socket,void *data,size_t size,int flags);
    ssize_t       recvData(ClientSocket *socket,void *data,size_t size);
    ssize_t       recvData(ClientSocket *socket,void *data,size_t size,int flags);
private:
    struct addrinfo _SockAddr;
    SOCKET          _Socket;
    int             _Port;
    int             _Maxconnections;
    HTTPException   _httpexception;
};
};
#endif
