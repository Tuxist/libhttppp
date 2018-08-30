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
    _Events = new struct kevent[(_ServerSocket->getMaxconnections())];
    _Mutex = new Mutex;
    _firstConnectionContext=NULL;
    _lastConnectionContext=NULL;
}

libhttppp::Event::~Event() {
#ifdef DEBUG_MUTEX
   _httpexception.Note("~Event","Lock MainMutex");
#endif
  _Mutex->lock(); 
  delete   _Cpool; 
  delete[] _Events; 
  delete   _firstConnectionContext; 
  _lastConnectionContext=NULL; 
  delete   _Mutex;
}

libhttppp::Event* _EventIns=NULL;

void libhttppp::Event::CtrlHandler(int signum) {
    _EventIns->_EventEndloop=false;
}

void libhttppp::Event::runEventloop() {
    int srvssocket=_ServerSocket->getSocket();
    int maxconnets=_ServerSocket->getMaxconnections();
    int nev = 0;
    _setEvent = (struct kevent){0};
    _Kq = kqueue();
    EV_SET(&_setEvent, srvssocket, EVFILT_READ , EV_ADD || EV_CLEAR , 0, 0, NULL);
    if (kevent(_Kq, &_setEvent, 1, NULL, 0, NULL) == -1)
      _httpexception.Critical("runeventloop","can't create kqueue!");
    signal(SIGPIPE, SIG_IGN);

    while(_EventEndloop) {
	nev = kevent(_Kq, NULL, 0, _Events,maxconnets, NULL);
        if(nev<0){
            if(errno== EINTR){
                continue;
            }else{
                 _httpexception.Critical("epoll wait failure");
                 throw _httpexception;
            }
                
        }
        for(int i=0; i<nev; i++) {
            ConnectionContext *curct=NULL;
            if(_Events[i].ident == (uintptr_t)srvssocket) {
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
                curct->_EventCounter=i;
                if(fd>0) {
                  _setEvent.fflags=0;
                  _setEvent.filter=EVFILT_READ;
                  _setEvent.flags=EV_ADD;
		  _setEvent.udata=(void*) curct;
		  _setEvent.ident=(uintptr_t)fd;
                  EV_SET(&_setEvent, fd, EVFILT_READ, EV_ADD, 0, 0, (void*) curct);
                  if (kevent(_Kq, &_setEvent, 1, NULL, 0, NULL) == -1){
                    _httpexception.Error("runeventloop","can't accep't in  kqueue!");
                  }else{
                    ConnectEvent(curct->_CurConnection);
                  }
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
                }
                
              } catch(HTTPException &e) {
#ifdef DEBUG_MUTEX
                _httpexception.Note("runeventloop","Unlock ConnectionMutex");
#endif
                curct->_Mutex->unlock();
                delConnectionContext(curct->_CurConnection);
                if(e.isCritical())
                  throw e;
              }
            } else {
                curct=(ConnectionContext*)_Events[i].udata;
                if(_Events[i].filter == EVFILT_READ) {
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
  _EventCounter=-1;
}

libhttppp::Event::ConnectionContext::~ConnectionContext(){
  delete _Mutex;
  delete _nextConnectionContext;
}


libhttppp::Event::ConnectionContext * libhttppp::Event::ConnectionContext::nextConnectionContext(){
  return _nextConnectionContext;    
}


libhttppp::Event::ConnectionContext * libhttppp::Event::addConnectionContext(){
#ifdef DEBUG_MUTEX
  _httpexception.Note("addConnection","Lock MainMutex");
#endif
  _Mutex->lock();
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
#ifdef DEBUG_MUTEX
  _httpexception.Note("addConnection","unlock MainMutex");
#endif
  _Mutex->unlock();
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
#ifdef DEBUG_MUTEX
  ccon->_httpexception.Note("ReadEvent","lock ConnectionMutex");
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
  ccon->_httpexception.Note("ReadEvent","unlock ConnectionMutex");
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
  Connection *con=ccon->_CurConnection;
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
    return NULL;
  }
#ifdef DEBUG_MUTEX
  httpexception.Note("WriteEvent","unlock ConnectionMutex");
#endif
  if(ccon)
    ccon->_Mutex->unlock();
  return NULL;
}

void *libhttppp::Event::CloseEvent(void *curcon){
  ConnectionContext *ccon=(ConnectionContext*)curcon;
  Event *eventins=ccon->_CurEvent;
#ifdef DEBUG_MUTEX
  ccon->_httpexception.Note("CloseEvent","ConnectionMutex");
#endif
  ccon->_Mutex->lock();
  Connection *con=(Connection*)ccon->_CurConnection;  
  eventins->DisconnectEvent(con);
#ifdef DEBUG_MUTEX
  ccon->_httpexception.Note("CloseEvent","unlock ConnectionMutex");
#endif
  ccon->_Mutex->unlock();
  try {
    EV_SET(&eventins->_setEvent,con->getClientSocket()->getSocket(),
           eventins->_Events[ccon->_EventCounter].filter, 
           EV_DELETE, 0, 0, NULL);
    if (kevent(eventins->_Kq,&eventins->_setEvent, 1, NULL, 0, NULL) == -1)
      eventins->_httpexception.Error("Connection can't delete from kqueue");                
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
