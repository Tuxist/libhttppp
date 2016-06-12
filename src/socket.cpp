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

#include "socket.h"

#ifndef Windows
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <sys/fcntl.h>
#else
  #include "win32.h"
  #include <Windows.h>
#endif

#include <sys/types.h>
#include <algorithm>
#include <config.h>
#include <errno.h>


using namespace libhttppp;

ClientSocket::ClientSocket(){
  _Socket=0;
  _SSL=0;
}

ClientSocket::~ClientSocket(){
  shutdown(_Socket,
#ifndef Windows 
  SHUT_RDWR 
#else 
  SD_BOTH
#endif
  );
}

void ClientSocket::setnonblocking(){
#ifndef Windows
  fcntl(_Socket, F_SETFL, O_NONBLOCK);
#else
  u_long bmode=1;
  ioctlsocket(_Socket,FIONBIO,&bmode);
#endif 
}

#ifndef Windows
int ClientSocket::getSocket(){
#else
SOCKET ClientSocket::getSocket(){
#endif
  return _Socket;
}

#ifndef Windows
ServerSocket::ServerSocket(const char* uxsocket,int maxconnections){
  int optval = 1;
 _Maxconnections=maxconnections;
  _UXSocketAddr.sun_family = AF_UNIX;
  try {
    std::copy(uxsocket,uxsocket+strlen(uxsocket),_UXSocketAddr.sun_path);
  }catch(...){
     _httpexception.Cirtical("Can't create Server Socket");
     throw _httpexception;
  }

  if ((_Socket = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0){
    _httpexception.Cirtical("Can't create Server Socket");
    throw _httpexception;
  }

  setsockopt(_Socket,SOL_SOCKET,SO_REUSEADDR,&optval, sizeof(optval));
  
  if (bind(_Socket, (struct sockaddr *)&_UXSocketAddr, sizeof(struct sockaddr)) < 0){
    _httpexception.Cirtical("Can't create Server Socket");
    throw _httpexception;
  }

  fcntl(_Socket, F_SETFL, O_NONBLOCK);

  if(listen(_Socket, _Maxconnections) < 0){
    _httpexception.Cirtical("Can't create Server Socket");
    throw _httpexception;
  }
}
#endif

ServerSocket::ServerSocket(const char* addr, int port,int maxconnections){
  _Maxconnections=maxconnections;
  _SockAddr.sin_family = AF_INET;
  _SockAddr.sin_port = htons(port);
  if(addr==NULL)
    _SockAddr.sin_addr.s_addr = INADDR_ANY;
  else
    _SockAddr.sin_addr.s_addr = inet_addr(addr);
  if ((_Socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    _httpexception.Cirtical("Can't create Server Socket");
    throw _httpexception;
  }
#ifndef Windows
  int optval = 1;
  setsockopt(_Socket,SOL_SOCKET,SO_REUSEADDR,&optval, sizeof(optval));
#else
  BOOL bOptVal = TRUE;
  int bOptLen = sizeof (BOOL);
  setsockopt(_Socket,SOL_SOCKET,SO_REUSEADDR,(char *)&bOptVal, bOptLen);
#endif
  if (bind(_Socket, (struct sockaddr *)&_SockAddr, sizeof(struct sockaddr)) < 0){
    _httpexception.Cirtical("Can't create Server Socket");
    throw _httpexception;
  }
#ifndef Windows
  fcntl(_Socket, F_SETFL, O_NONBLOCK);
#else
  u_long bmode=1;
  ioctlsocket(_Socket,FIONBIO,&bmode);
#endif
  if(listen(_Socket, _Maxconnections) < 0){
    _httpexception.Cirtical("Can't create Server Socket");
    throw _httpexception;
  }
}

ServerSocket::~ServerSocket(){

}

#ifndef Windows
int ServerSocket::getSocket(){
  return _Socket;
}
#else
SOCKET ServerSocket::getSocket(){
  return _Socket;
}
#endif

int ServerSocket::getMaxconnections(){
  return _Maxconnections;
}

#ifndef Windows
int ServerSocket::acceptEvent(ClientSocket *clientsocket){
#else
SOCKET ServerSocket::acceptEvent(ClientSocket *clientsocket){
#endif
  clientsocket->_ClientAddrLen=sizeof(clientsocket);
#ifndef Windows
  int socket = accept(_Socket,(struct sockaddr *)&clientsocket->_ClientAddr, &clientsocket->_ClientAddrLen);
#else
  SOCKET socket = accept(_Socket,(struct sockaddr *)&clientsocket->_ClientAddr, &clientsocket->_ClientAddrLen);
#endif
  if(socket==-1){
    char errbuf[255];
    strerror_r(errno,errbuf,255);
    _httpexception.Error(errbuf);
  }
  clientsocket->_Socket=socket;
  return socket;
}

ssize_t ServerSocket::sendData(ClientSocket* socket, void* data, size_t size){
#ifndef Windows
   ssize_t rval=sendto(socket->getSocket(),data, size,0,&socket->_ClientAddr, socket->_ClientAddrLen);
#else
  int rval=sendto(socket->getSocket(),(const char*) data, (int)size,0,&socket->_ClientAddr, socket->_ClientAddrLen);
#endif
  if(rval==-1){
#ifdef Linux
    char errbuf[255];
    _httpexception.Error(strerror_r(errno,errbuf,255));
#else
    char errbuf[255];
    strerror_r(errno,errbuf,255);
    _httpexception.Error(errbuf);
#endif
    if(errno != EAGAIN)
      throw _httpexception;
  }
  return rval;
}

ssize_t ServerSocket::recvData(ClientSocket* socket, void* data, size_t size){
#ifndef Windows
  ssize_t recvsize=recvfrom(socket->getSocket(),data, size,0,
			    &socket->_ClientAddr, &socket->_ClientAddrLen);
#else
  ssize_t recvsize=recvfrom(socket->getSocket(), (char*)data,(int)size,0,
			    &socket->_ClientAddr, &socket->_ClientAddrLen);
#endif
  if(recvsize==-1){
#ifdef Linux 
    char errbuf[255];
    _httpexception.Error(strerror_r(errno,errbuf,255));
#else
    char errbuf[255];
    strerror_r(errno,errbuf,255);
    _httpexception.Error(errbuf);
#endif
    throw _httpexception;
  }
  return recvsize;
}
