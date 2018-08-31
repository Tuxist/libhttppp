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
#include "os/os.h"

#define READEVENT 0
#define SENDEVENT 1

//#define DEBUG_MUTEX

#include "../event.h"

libhttppp::Event::Event(ServerSocket *serversocket) {
    _ServerSocket=serversocket;
    _ServerSocket->setnonblocking();
    _ServerSocket->listenSocket();
    _EventEndloop =true;
    _Cpool= new ConnectionPool(_ServerSocket);
    _Events = new epoll_event[(_ServerSocket->getMaxconnections())];
    _Mutex = new Mutex;
    _firstConnectionContext=NULL;
    _lastConnectionContext=NULL;
}

libhttppp::Event::~Event() {
  delete   _Cpool;
  delete[] _Events;
  delete   _firstConnectionContext;
  delete   _Mutex;
  _lastConnectionContext=NULL;
}

libhttppp::Event* _EventIns=NULL;

void libhttppp::Event::CtrlHandler(int signum) {
    _EventIns->_EventEndloop=false;
}

void libhttppp::Event::runEventloop() {
    _setEvent = (struct epoll_event){0};
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
    signal(SIGPIPE, SIG_IGN);
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
            ConnectionContext *curct=NULL;
            if(_Events[i].data.fd == srvssocket) {
              try {
              /*will create warning debug mode that normally because the check already connection
               * with this socket if getconnection throw they will be create a new one
               */
                curct=addConnectionContext();

#ifdef DEBUG_MUTEX
                _httpexception.Note("runeventloop","Lock ConnectionMutex");
#endif
                curct->_Mutex->lock();
                ClientSocket *clientsocket=curct->_CurConnection->getClientSocket();
                int fd=_ServerSocket->acceptEvent(clientsocket);
                clientsocket->setnonblocking();
                if(fd>0) {
                  _setEvent.data.ptr = (void*) curct;
                  _setEvent.events = EPOLLIN|EPOLLET;
                  if(epoll_ctl(_epollFD, EPOLL_CTL_ADD, fd, &_setEvent)==-1 && errno==EEXIST)
                    epoll_ctl(_epollFD, EPOLL_CTL_MOD,fd, &_setEvent);
                  ConnectEvent(curct->_CurConnection);
#ifdef DEBUG_MUTEX
                _httpexception.Note("runeventloop","Unlock ConnectionMutex");
#endif
                  curct->_Mutex->unlock();
                } else {
#ifdef DEBUG_MUTEX
                _httpexception.Note("runeventloop","Unlock ConnectionMutex");
#endif
                   curct->_Mutex->unlock();
                  delConnectionContext(curct->_CurConnection);
                  _Mutex->unlock();
                }
                
              } catch(HTTPException &e) {
#ifdef DEBUG_MUTEX
                _httpexception.Note("runeventloop","Unlock ConnectionMutex");
#endif
                curct->_Mutex->unlock();
                delConnectionContext(curct->_CurConnection);
                _Mutex->unlock();
                if(e.isCritical())
                  throw e;
              }
            } else {
                curct=(ConnectionContext*)_Events[i].data.ptr;
                if(_Events[i].events & EPOLLIN) {
					Thread curthread;
					curthread.Create(ReadEvent,curct);
                    curthread.Detach();
                }else{
                    CloseEvent(curct);
                }
            } 
        }
        _EventIns=this;
        signal(SIGINT, CtrlHandler);
    }
}

libhttppp::Event::ConnectionContext::ConnectionContext(){
  _CurConnection=NULL;
  _CurCPool=NULL;
  _CurEvent=NULL;
  _Mutex=new Mutex;
  _nextConnectionContext=NULL;    
}

libhttppp::Event::ConnectionContext::~ConnectionContext(){
  delete _Mutex;
  delete _nextConnectionContext;
}


libhttppp::Event::ConnectionContext * libhttppp::Event::ConnectionContext::nextConnectionContext(){
  return _nextConnectionContext;    
}


libhttppp::Event::ConnectionContext * libhttppp::Event::addConnectionContext(){
  if(!_firstConnectionContext){      
    _firstConnectionContext=new ConnectionContext();
#ifdef DEBUG_MUTEX
    _httpexception.Note("addConnection","Lock ConnectionMutex");
#endif
    _firstConnectionContext->_Mutex->lock();
    _lastConnectionContext=_firstConnectionContext;
#ifdef DEBUG_MUTEX
    _httpexception.Note("addConnection","Unlock ConnectionMutex");
#endif
    _firstConnectionContext->_Mutex->unlock();
  }else{
#ifdef DEBUG_MUTEX
    _httpexception.Note("addConnection","Lock ConnectionMutex");
#endif
    ConnectionContext *prevcon=_lastConnectionContext;
    prevcon->_Mutex->lock();
    _lastConnectionContext->_nextConnectionContext=new ConnectionContext();
    _lastConnectionContext=_lastConnectionContext->_nextConnectionContext;
#ifdef DEBUG_MUTEX
    _httpexception.Note("addConnection","Unlock ConnectionMutex");
#endif
    prevcon->_Mutex->unlock();
  }
  _lastConnectionContext->_CurConnection=_Cpool->addConnection();
  _lastConnectionContext->_CurCPool=_Cpool;
  _lastConnectionContext->_CurEvent=this;
  
  return _lastConnectionContext;
}

libhttppp::Event::ConnectionContext * libhttppp::Event::delConnectionContext(libhttppp::Connection* delcon){
  ConnectionContext *prevcontext=NULL;
#ifdef DEBUG_MUTEX
  _httpexception.Note("delConnection","Lock MainMutex");
#endif
  _Mutex->lock();
  for(ConnectionContext *curcontext=_firstConnectionContext; curcontext; 
      curcontext=curcontext->nextConnectionContext()){
    if(curcontext->_CurConnection==delcon){
#ifdef DEBUG_MUTEX
      _httpexception.Note("delConnection","Lock ConnectionMutex");
#endif
      curcontext->_Mutex->lock();
      _Cpool->delConnection(delcon);
      if(prevcontext){
#ifdef DEBUG_MUTEX
        _httpexception.Note("delConnection","Lock prevConnectionMutex");
#endif
        prevcontext->_Mutex->lock();
        prevcontext->_nextConnectionContext=curcontext->_nextConnectionContext;
        if(_lastConnectionContext==curcontext){
          _lastConnectionContext=prevcontext;
        }
#ifdef DEBUG_MUTEX
        _httpexception.Note("delConnection","unlock prevConnectionMutex");
#endif
        prevcontext->_Mutex->unlock();
      }else{
#ifdef DEBUG_MUTEX
        _httpexception.Note("delConnection","lock firstConnectionMutex");
#endif
        _firstConnectionContext->_Mutex->lock();
#ifdef DEBUG_MUTEX
        _httpexception.Note("delConnection","lock lastConnectionMutex");
#endif
        _lastConnectionContext->_Mutex->lock();
        _firstConnectionContext=curcontext->_nextConnectionContext;
        if(_lastConnectionContext->_CurConnection==delcon)
          _lastConnectionContext=_firstConnectionContext;
        if(_firstConnectionContext){
#ifdef DEBUG_MUTEX
        _httpexception.Note("delConnection","unlock firstConnectionMutex");
#endif
        _firstConnectionContext->_Mutex->unlock();
        }
        if(_lastConnectionContext){
#ifdef DEBUG_MUTEX
     _httpexception.Note("delConnection","unlock lastConnectionMutex");
#endif
        _lastConnectionContext->_Mutex->unlock();
        }
      }
#ifdef DEBUG_MUTEX
      _httpexception.Note("delConnection","Unlock ConnectionMutex");
#endif
      curcontext->_Mutex->unlock();
      curcontext->_nextConnectionContext=NULL;
      delete curcontext;
      break;
    }
    prevcontext=curcontext;
  }
#ifdef DEBUG_MUTEX
  _httpexception.Note("delConnection","unlock MainMutex");
#endif
  _Mutex->unlock();
  if(prevcontext && prevcontext->_nextConnectionContext){
    return prevcontext->_nextConnectionContext;
  }else{
    ConnectionContext *fcontext=_firstConnectionContext;
    return fcontext;
  }
}



/*Workers*/
void *libhttppp::Event::ReadEvent(void *curcon){
  ConnectionContext *ccon=(ConnectionContext*)curcon;
  Event *eventins=ccon->_CurEvent;
  HTTPException httpexception;
#ifdef DEBUG_MUTEX
  httpexception.Note("ReadEvent","lock ConnectionMutex");
#endif  
  ccon->_Mutex->lock();
  Connection *con=(Connection*)ccon->_CurConnection;
  try {
    char buf[BLOCKSIZE];
    int rcvsize=0;
    do{
      rcvsize=eventins->_ServerSocket->recvData(con->getClientSocket(),buf,BLOCKSIZE);
      if(rcvsize>0)
        con->addRecvQueue(buf,rcvsize);
    }while(rcvsize>0);
    eventins->RequestEvent(con);
#ifdef DEBUG_MUTEX
  httpexception.Note("ReadEvent","unlock ConnectionMutex");
#endif 
    ccon->_Mutex->unlock(); 
    WriteEvent(ccon);
  } catch(HTTPException &e) {
#ifdef DEBUG_MUTEX
      ccon->_httpexception.Note("ReadEvent","unlock ConnectionMutex");
#endif 
      ccon->_Mutex->unlock();
       if(e.isCritical()) {
         throw e;
       }
       if(e.isError()){
          con->cleanRecvData();
          CloseEvent(ccon);
          return NULL;
       }
  }
  return NULL;
}

void *libhttppp::Event::WriteEvent(void* curcon){
  ConnectionContext *ccon=(ConnectionContext*)curcon;
  Event *eventins=ccon->_CurEvent;
  HTTPException httpexception;
#ifdef DEBUG_MUTEX
  httpexception.Note("WriteEvent","lock ConnectionMutex");
#endif
  ccon->_Mutex->lock();
  Connection *con=(Connection*)ccon->_CurConnection;
  try {
    ssize_t sended=0;
    while(con->getSendData()){
      sended=eventins->_ServerSocket->sendData(con->getClientSocket(),
                                    (void*)con->getSendData()->getData(),
                                    con->getSendData()->getDataSize());
      if(sended>0)
        con->resizeSendQueue(sended);
    }
    eventins->ResponseEvent(con);
  } catch(HTTPException &e) {
    CloseEvent(ccon);
  }
#ifdef DEBUG_MUTEX
  httpexception.Note("WriteEvent","unlock ConnectionMutex");
#endif
  ccon->_Mutex->unlock();
  return NULL;
}

void *libhttppp::Event::CloseEvent(void *curcon){
  ConnectionContext *ccon=(ConnectionContext*)curcon;
  Event *eventins=ccon->_CurEvent;
  HTTPException httpexception;
#ifdef DEBUG_MUTEX
  httpexception.Note("CloseEvent","ConnectionMutex");
#endif
  ccon->_Mutex->lock();
  Connection *con=(Connection*)ccon->_CurConnection;  
  eventins->DisconnectEvent(con);
  try {
    int ect=epoll_ctl(eventins->_epollFD, EPOLL_CTL_DEL, con->getClientSocket()->getSocket(), &eventins->_setEvent);
    if(ect==-1)
      httpexception.Note("CloseEvent","can't delete Connection from epoll");
#ifdef DEBUG_MUTEX
    httpexception.Note("CloseEvent","unlock ConnectionMutex");
#endif
    ccon->_Mutex->unlock();
    eventins->delConnectionContext(con);
    curcon=NULL;
    eventins->_httpexception.Note("Connection shutdown!");
  } catch(HTTPException &e) {
    eventins->_httpexception.Note("Can't do Connection shutdown!");
  }
  return NULL;
}

/*Event Handlers*/
void libhttppp::Event::RequestEvent(libhttppp::Connection *curcon) {
    return;
}

void libhttppp::Event::ResponseEvent(libhttppp::Connection *curcon) {
    return;
}

void libhttppp::Event::ConnectEvent(libhttppp::Connection *curcon) {
    return;
}

void libhttppp::Event::DisconnectEvent(libhttppp::Connection *curcon) {
    return;
}
