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

#include <fcntl.h>
#include <sys/sysinfo.h>
#include <cstdlib>
#include <config.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#define READEVENT 0
#define SENDEVENT 1

#include "../event.h"

libhttppp::Event::Event(ServerSocket *serversocket) {
    _ServerSocket=serversocket;
    _ServerSocket->setnonblocking();
    _ServerSocket->listenSocket();
    _EventEndloop =true;
    _Cpool= new ConnectionPool(_ServerSocket);
    _Events = new epoll_event[(_ServerSocket->getMaxconnections())];
}

libhttppp::Event::~Event() {
  delete   _Cpool;
  delete[] _Events;
}

libhttppp::Event* _EventIns=NULL;

void libhttppp::Event::CtrlHandler(int signum) {
    _EventIns->_EventEndloop=false;
}

void libhttppp::Event::runEventloop() {
//     int threadcount=1;/*get_nprocs()*2;*/
//     pthread_t threads[threadcount];
//     for(int i=0; i<threadcount; i++){
//     int thc = pthread_create( &threads[i], NULL, &WorkerThread,(void*) this);
//       if( thc != 0 ) {
//         _httpexception.Critical("can't create thread");
//         throw _httpexception;
//       }  
//     }
//     for(int i=0; i<threadcount; i++){
//         pthread_join(threads[i], NULL);
//  
    _setEvent = {0};
    for(int i=0; i<_ServerSocket->getMaxconnections(); i++)
        _Events[i].data.fd = -1;
    _epollFD = epoll_create1(0);

    if (_epollFD == -1) {
        _httpexception.Critical("can't create epoll");
        throw _httpexception;
    }

    _setEvent.events = EPOLLIN;
    _setEvent.data.fd = _ServerSocket->getSocket();
    
    if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, _ServerSocket->getSocket(), &_setEvent) < 0) {
        _httpexception.Critical("can't create epoll");
        throw _httpexception;
    }
    
    int srvssocket=_ServerSocket->getSocket();
    int maxconnets=_ServerSocket->getMaxconnections();
    while(_EventEndloop) {
        int n = epoll_wait(_epollFD,_Events,maxconnets, EPOLLWAIT);
        if(n<0){
            if(errno== EINTR){
                continue;
            }else{
                 _httpexception.Critical("epoll wait failure");
                 throw _httpexception;
            }
                
        }
        for(int i=0; i<n; i++) {
            Connection *curcon=NULL;
            if(_Events[i].data.fd == srvssocket) {
              try {
              /*will create warning debug mode that normally because the check already connection
               * with this socket if getconnection throw they will be create a new one
               */
                curcon=_Cpool->addConnection();
                ClientSocket *clientsocket=curcon->getClientSocket();
                int fd=_ServerSocket->acceptEvent(clientsocket);
                clientsocket->setnonblocking();
                if(fd>0) {
                  _setEvent.data.ptr = (void*) curcon;
                  _setEvent.events = EPOLLIN|EPOLLET;
                  if(epoll_ctl(_epollFD, EPOLL_CTL_ADD, fd, &_setEvent)==-1 && errno==EEXIST)
                    epoll_ctl(_epollFD, EPOLL_CTL_MOD,fd, &_setEvent);
                  ConnectEvent(curcon);
                } else {
                  _Cpool->delConnection(curcon);
                }
              } catch(HTTPException &e) {
                _Cpool->delConnection(curcon);
                if(e.isCritical())
                  throw e;
              }
            } else {
                curcon=(Connection*)_Events[i].data.ptr;
                if(_Events[i].events & EPOLLIN) {
                    ReadEvent(curcon);
                }else{
                    CloseEvent(curcon);
                }
            } 
        }
        _EventIns=this;
        signal(SIGINT, CtrlHandler);
    }
}

void libhttppp::Event::ReadEvent(libhttppp::Connection* curcon){
     try {
       char buf[BLOCKSIZE];
       int rcvsize=0;
       do{
         rcvsize=_ServerSocket->recvData(curcon->getClientSocket(),buf,BLOCKSIZE);
         if(rcvsize>0)
           curcon->addRecvQueue(buf,rcvsize);
       }while(rcvsize>0);
       RequestEvent(curcon);
       WriteEvent(curcon);
     } catch(HTTPException &e) {
       if(e.isCritical()) {
         throw e;
       }
       if(e.isError()){
          CloseEvent(curcon);
       }
     }   
}

void libhttppp::Event::WriteEvent(libhttppp::Connection* curcon){
  try {
    ssize_t sended=0;
    while(curcon->getSendData()){
      sended=_ServerSocket->sendData(curcon->getClientSocket(),
                                    (void*)curcon->getSendData()->getData(),
                                    curcon->getSendData()->getDataSize());
      if(sended>0)
        curcon->resizeSendQueue(sended);
    };
    ResponseEvent(curcon);
  } catch(HTTPException &e) {
    curcon->cleanSendData();
    CloseEvent(curcon);
  }
}

void libhttppp::Event::CloseEvent(libhttppp::Connection* curcon){
  DisconnectEvent(curcon);
  try {
    epoll_ctl(_epollFD, EPOLL_CTL_DEL, curcon->getClientSocket()->getSocket(), &_setEvent);
    _Cpool->delConnection(curcon);
    curcon=NULL;
    _httpexception.Note("Connection shutdown!");
  } catch(HTTPException &e) {
    _httpexception.Note("Can't do Connection shutdown!");
  }
}

void libhttppp::Event::RequestEvent(Connection *curcon) {
    return;
}

void libhttppp::Event::ResponseEvent(libhttppp::Connection *curcon) {
    return;
}

void libhttppp::Event::ConnectEvent(libhttppp::Connection *curcon) {
    return;
}

void libhttppp::Event::DisconnectEvent(Connection *curcon) {
    return;
}
