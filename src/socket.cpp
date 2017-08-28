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
  #include <Windows.h>
#endif

#include <sys/types.h>
#include <algorithm>
#include <cstring>
#include <config.h>
#include <errno.h>

libhttppp::ClientSocket::ClientSocket(){
  _Socket=0;
  _SSL=NULL;
}

libhttppp::ClientSocket::~ClientSocket(){
  shutdown(_Socket,
#ifndef Windows 
  SHUT_RDWR 
#else 
  SD_BOTH
#endif
  );
  SSL_free(_SSL);
}

void libhttppp::ClientSocket::setnonblocking(){
#ifndef Windows
  fcntl(_Socket, F_SETFL, O_NONBLOCK);
#else
  u_long bmode=1;
  ioctlsocket(_Socket,FIONBIO,&bmode);
#endif 
}

#ifndef Windows
int libhttppp::ClientSocket::getSocket(){
#else
SOCKET libhttppp::ClientSocket::getSocket(){
#endif
  return _Socket;
}

#ifndef Windows
libhttppp::ServerSocket::ServerSocket(const char* uxsocket,int maxconnections){
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
}
#endif

libhttppp::ServerSocket::ServerSocket(const char* addr, int port,int maxconnections){
  _Maxconnections=maxconnections;
#ifdef Windows
  int iResult;
  WSADATA wsaData;
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
	  _httpexception.Cirtical("WSAStartup failed");
  }
#endif
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
  setsockopt(_Socket,SOL_SOCKET,SO_REUSEADDR,(const char *)&bOptVal, bOptLen);
#endif
  if (bind(_Socket, (struct sockaddr *)&_SockAddr, sizeof(struct sockaddr)) < 0){
    _httpexception.Cirtical("Can't bind Server Socket");
    throw _httpexception;
  }
}

libhttppp::ServerSocket::~ServerSocket(){

}

void libhttppp::ServerSocket::setnonblocking(){
#ifndef Windows
  fcntl(_Socket, F_SETFL, O_NONBLOCK);
#else
  u_long bmode=1;
  ioctlsocket(_Socket,FIONBIO,&bmode);
#endif
}

void libhttppp::ServerSocket::listenSocket(){
  if(listen(_Socket, _Maxconnections) < 0){
    _httpexception.Cirtical("Can't create Server Socket");
    throw _httpexception;
  }
}

#ifndef Windows
int libhttppp::ServerSocket::getSocket(){
  return _Socket;
}
#else
SOCKET libhttppp::ServerSocket::getSocket(){
  return _Socket;
}
#endif

int libhttppp::ServerSocket::getMaxconnections(){
  return _Maxconnections;
}

#ifndef Windows
int libhttppp::ServerSocket::acceptEvent(ClientSocket *clientsocket){
#else
SOCKET libhttppp::ServerSocket::acceptEvent(ClientSocket *clientsocket){
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
  if(isSSLTrue()){
     clientsocket->_SSL = SSL_new(_CTX);
     SSL_set_fd(clientsocket->_SSL, socket);
     if (SSL_accept(clientsocket->_SSL) <= 0) {
       ERR_print_errors_fp(stderr);
     }
  }
  return socket;
}

ssize_t libhttppp::ServerSocket::sendData(ClientSocket* socket, void* data, size_t size){
  return sendData(socket,data,size,0);
}

ssize_t libhttppp::ServerSocket::sendData(ClientSocket* socket, void* data, size_t size,int flags){
#ifndef Windows
   ssize_t rval=sendto(socket->getSocket(),data, size,flags,&socket->_ClientAddr, socket->_ClientAddrLen);
#else
  int rval=sendto(socket->getSocket(),(const char*) data, (int)size,flags,&socket->_ClientAddr, socket->_ClientAddrLen);
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

ssize_t libhttppp::ServerSocket::recvData(ClientSocket* socket, void* data, size_t size){
   return recvData(socket,data,size,0);
}

ssize_t libhttppp::ServerSocket::recvData(ClientSocket* socket, void* data, size_t size,int flags){
#ifndef Windows
  ssize_t recvsize=recvfrom(socket->getSocket(),data, size,flags,
			    &socket->_ClientAddr, &socket->_ClientAddrLen);
#else
  ssize_t recvsize=recvfrom(socket->getSocket(), (char*)data,(int)size,flags,
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
