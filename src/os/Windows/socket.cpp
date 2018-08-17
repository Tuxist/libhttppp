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


#include <Windows.h>
#include <sys/types.h>
#include <algorithm>
#include <cstring>
#include <config.h>
#include <errno.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>

libhttppp::ClientSocket::ClientSocket(){
  _Socket= WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
  if (_Socket == INVALID_SOCKET) {
	  _httpexception.Error("WSASocket(sdSocket) failed: ",WSAGetLastError());
  }
  _SSL=NULL;
}

libhttppp::ClientSocket::~ClientSocket(){
  shutdown(_Socket,SD_BOTH);
  SSL_free(_SSL);
}

void libhttppp::ClientSocket::setnonblocking(){
  u_long bmode=1;
  ioctlsocket(_Socket,FIONBIO,&bmode);
}

void libhttppp::ClientSocket::disableBuffer(){
	int nzero = 0;
	int nRet = setsockopt(_Socket, SOL_SOCKET, SO_SNDBUF, (char *)&nzero, sizeof(nzero));
	if (nRet == SOCKET_ERROR) {
		_httpexception.Error("setsockopt(SNDBUF) failed: ", WSAGetLastError());
	}
}

SOCKET libhttppp::ClientSocket::getSocket(){
  return _Socket;
}

void libhttppp::ClientSocket::setSocket(SOCKET socket) {
   _Socket=socket;
}

libhttppp::ServerSocket::ServerSocket(const char* uxsocket,int maxconnections){
  _httpexception.Critical("ServerSocket","Unix Socket not soppurted by this OS");
  throw _httpexception;
}



libhttppp::ServerSocket::ServerSocket(SOCKET socket) {
	_Socket = socket;
	_Maxconnections = MAXDEFAULTCONN;
}

libhttppp::ServerSocket::ServerSocket(const char* addr, int port,int maxconnections){
  _Maxconnections=maxconnections;
  int iResult;
  WSADATA wsaData;
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
	  _httpexception.Critical("WSAStartup failed");
  }

  char port_buffer[6];
  snprintf(port_buffer,6, "%d", port);
  struct addrinfo *result, *rp;
  memset(&_SockAddr, 0, sizeof(struct addrinfo));
  _SockAddr.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  _SockAddr.ai_socktype = SOCK_STREAM; /* Datagram socket */
  _SockAddr.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
  _SockAddr.ai_protocol = 0;          /* Any protocol */
  _SockAddr.ai_canonname = NULL;
  _SockAddr.ai_addr = NULL;
  _SockAddr.ai_next = NULL;

  int s = getaddrinfo(addr, port_buffer, &_SockAddr, &result);
  if (s != 0) {
	  _httpexception.Critical("getaddrinfo failed ", gai_strerror(s));
	  throw _httpexception;
  }

  /* getaddrinfo() returns a list of address structures.
  Try each address until we successfully bind(2).
  If socket(2) (or bind(2)) fails, we (close the socket
  and) try the next address. */

  for (rp = result; rp != NULL; rp = rp->ai_next) {
	  _Socket = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
	  if (_Socket == -1)
		  continue;
	  BOOL bOptVal = TRUE;
	  int bOptLen = sizeof(BOOL);
	  setsockopt(_Socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&bOptVal, bOptLen);
	  if (bind(_Socket, rp->ai_addr, rp->ai_addrlen) == 0)
		  break;                  /* Success */
	  closesocket(_Socket);
  }

  if (rp == NULL) {               /* No address succeeded */
	  _httpexception.Critical("Could not bind\n");
	  throw _httpexception;
  }
  freeaddrinfo(result);
}

libhttppp::ServerSocket::~ServerSocket(){
}

void libhttppp::ServerSocket::setnonblocking(){
  u_long bmode=1;
  ioctlsocket(_Socket,FIONBIO,&bmode);
}

void libhttppp::ServerSocket::listenSocket(){
  if(listen(_Socket, _Maxconnections) < 0){
    _httpexception.Critical("Can't listen Server Socket", errno);
    throw _httpexception;
  }
}

SOCKET libhttppp::ServerSocket::getSocket(){
  return _Socket;
}

int libhttppp::ServerSocket::getMaxconnections(){
  return _Maxconnections;
}

SOCKET libhttppp::ServerSocket::acceptEvent(ClientSocket *clientsocket){
  clientsocket->_ClientAddrLen=sizeof(clientsocket);
  SOCKET socket = accept(_Socket,(struct sockaddr *)&clientsocket->_ClientAddr, &clientsocket->_ClientAddrLen);
  if(socket==-1){
    char errbuf[255];
    strerror_r(errno, errbuf, 255);
    _httpexception.Error("Can't accept on  Socket",errbuf);
  }
  clientsocket->_Socket=socket;
  if(isSSLTrue()){
     clientsocket->_SSL = SSL_new(_CTX);
     SSL_set_fd(clientsocket->_SSL, socket);
     if (SSL_accept(clientsocket->_SSL) <= 0) {
       ERR_print_errors_fp(stderr);
       return -1;
     }
  }
  return socket;
}

ssize_t libhttppp::ServerSocket::sendData(ClientSocket* socket, void* data, size_t size){
  return sendData(socket,data,size,0);
}

ssize_t libhttppp::ServerSocket::sendData(ClientSocket* socket, void* data, size_t size,int flags){
  int rval=0;
  if(isSSLTrue() && socket->_SSL){
    rval=SSL_write(socket->_SSL,data,size);
  }else{
    rval=sendto(socket->getSocket(),(const char*) data, (int)size,flags,&socket->_ClientAddr, socket->_ClientAddrLen);
  }

  if(rval==-1){
    char errbuf[255];
    strerror_r(errno,errbuf,255);
    _httpexception.Error("Socket sendata:",errbuf);
    if(errno != EAGAIN || errno !=EWOULDBLOCK)
      throw _httpexception;
  }
  return rval;
}

ssize_t libhttppp::ServerSocket::sendWSAData(ClientSocket *socket, WSABUF *data, DWORD size,DWORD flags,
	                                         LPDWORD numberofbytessend, LPWSAOVERLAPPED lpOverlapped,
	                                         LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {
  int	rval = WSASendTo(socket->getSocket(), data, size, numberofbytessend, flags, &socket->_ClientAddr, 
	                     socket->_ClientAddrLen,lpOverlapped, lpCompletionRoutine);
  if (rval == -1) {
	char errbuf[255];
	strerror_r(errno, errbuf, 255);
	_httpexception.Error("Socket sendata:", errbuf);
	if (errno != EAGAIN || errno != EWOULDBLOCK)
		throw _httpexception;
  }
  return rval;
}

ssize_t libhttppp::ServerSocket::recvData(ClientSocket* socket, void* data, size_t size){
   return recvData(socket,data,size,0);
}

ssize_t libhttppp::ServerSocket::recvData(ClientSocket* socket, void* data, size_t size,int flags){
  ssize_t recvsize=0;
  if(isSSLTrue() && socket->_SSL){
    recvsize=SSL_read(socket->_SSL,data,size);
  }else{
    recvsize=recvfrom(socket->getSocket(), (char*)data,(int)size,flags,
                              &socket->_ClientAddr, &socket->_ClientAddrLen);
  }
  if(recvsize==-1){
    char errbuf[255];
    strerror_r(errno,errbuf,255);
    _httpexception.Error("Socket recvata:",errbuf);
    if(errno != EAGAIN || errno !=EWOULDBLOCK){
      throw _httpexception;
    }
  }
  return recvsize;
}

ssize_t libhttppp::ServerSocket::recvWSAData(ClientSocket *socket, WSABUF *data, DWORD size, LPDWORD flags,
	LPDWORD numberofbytessend, LPWSAOVERLAPPED lpOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {
	ssize_t recvsize = WSARecvFrom(socket->getSocket(), data, size, numberofbytessend, flags, &socket->_ClientAddr,
		(LPINT)socket->_ClientAddrLen, lpOverlapped, lpCompletionRoutine);
	if (recvsize == -1) {
		char errbuf[255];
		strerror_r(errno, errbuf, 255);
		_httpexception.Error("Socket sendata:", errbuf);
		if (errno != EAGAIN || errno != EWOULDBLOCK)
			throw _httpexception;
	}
	return recvsize;
}