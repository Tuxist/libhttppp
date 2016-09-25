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
#include <algorithm>
#include "httpd.h"

using namespace libhttppp;

HttpD::HttpD(int argc, char** argv){
  int port=0;
  _MaxConnections=MAXDEFAULTCONN;
  _Queue=NULL;
  char *httpaddr=NULL,*rootpath=NULL;
  for(int args=1; args<argc; args++){
    if(strncmp(argv[args],"--httpaddr=",11)==0){
      httpaddr=argv[args]+11;
    }else if(strncmp(argv[args],"--httpport=",11)==0){
      port=atoi(argv[args]+11);
    }else if(strncmp(argv[args],"--rootpath=",11)==0){
      rootpath=argv[args]+11;
      _RootPathLen=strlen(rootpath);
      if(_RootPathLen<=PATHSIZE)
        std::copy(rootpath,rootpath+_RootPathLen,_RootPath);
    }else if(strncmp(argv[args],"--maxconnections=",17)==0){
      _MaxConnections=atoi(argv[args]+17);
    }else if(strncmp(argv[args],"--help",6) || strncmp(argv[args],"-h",2)){
      _Help();
    }
  }
  if(!httpaddr || !rootpath){
    _httpexception.Cirtical("not enough arguments given");
    _Help();
    throw _httpexception;
  }
  if(port!=0)
    _ServerSocket= new ServerSocket(httpaddr,port,_MaxConnections);
#ifndef WIN32
  else
    _ServerSocket= new ServerSocket(httpaddr,_MaxConnections);
#endif
}

void HttpD::_Help(){
        printf("%s%s%s","--httpaddr=0.0.0.0        Address to listen\n"
                       ,"--httpport=80             Port to listen\n" 
                       ,"--rootpath=/tmp           Directory for file content\n\n"
	      );
}

void HttpD::runDaemon(){
  _ServerSocket->setnonblocking();
  _ServerSocket->listenSocket();
  _Queue = new Queue(_ServerSocket);
}

HttpD::~HttpD(){
  delete _ServerSocket;
  delete _Queue;
}