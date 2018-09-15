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

#include "event.h"
#include "threadpool.h"

bool libhttppp::Event::_EventEndloop=true;

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

void libhttppp::Event::addConnectionContext(libhttppp::Event::ConnectionContext **addcon){
if(!_firstConnectionContext){
#ifdef DEBUG_MUTEX
    _httpexception.Note("delConnection","Lock MainMutex");
#endif
    _Mutex->lock();
    _firstConnectionContext=new ConnectionContext();
    _lastConnectionContext=_firstConnectionContext;
#ifdef DEBUG_MUTEX
    _httpexception.Note("delConnection","unlock MainMutex");
#endif
    _Mutex->unlock();
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
 #ifdef DEBUG_MUTEX
    _httpexception.Note("addConnection","Lock ConnectionMutex");
#endif
  _lastConnectionContext->_Mutex->lock(); 
  _lastConnectionContext->_CurConnection=_Cpool->addConnection();
  _lastConnectionContext->_CurCPool=_Cpool;
  _lastConnectionContext->_CurEvent=this;
  *addcon=_lastConnectionContext;
#ifdef DEBUG_MUTEX
    _httpexception.Note("addConnection","Unlock ConnectionMutex");
#endif
  _lastConnectionContext->_Mutex->unlock();
}

void libhttppp::Event::delConnectionContext(libhttppp::Event::ConnectionContext *delctx,
                                            libhttppp::Event::ConnectionContext **nextcxt){
  ConnectionContext *prevcontext=NULL;
#ifdef DEBUG_MUTEX
  _httpexception.Note("delConnection","Lock MainMutex");
#endif
  _Mutex->lock();
  for(ConnectionContext *curcontext=_firstConnectionContext; curcontext; 
      curcontext=curcontext->nextConnectionContext()){
    if(curcontext==delctx){
#ifdef DEBUG_MUTEX
      _httpexception.Note("delConnection","Lock ConnectionMutex");
#endif
      curcontext->_Mutex->lock();
      _Cpool->delConnection(curcontext->_CurConnection);
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
        if(_lastConnectionContext==delctx)
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
  if(nextcxt){
    if(prevcontext && prevcontext->_nextConnectionContext){
      *nextcxt= prevcontext->_nextConnectionContext;
    }else{
      *nextcxt=_firstConnectionContext;
    }
  }
}

libhttppp::Event::WorkerContext::WorkerContext(){
    _CurEvent=NULL;
    _CurThread=NULL;
    _nextWorkerContext=NULL;
}

libhttppp::Event::WorkerContext::~WorkerContext(){
    delete _nextWorkerContext;
}

libhttppp::Event::WorkerContext *libhttppp::Event::addWorkerContext(){
    if(_firstWorkerContext){
        _lastWorkerContext->_nextWorkerContext=new WorkerContext;
        _lastWorkerContext=_lastWorkerContext->_nextWorkerContext;
    }else{
        _firstWorkerContext=new WorkerContext;
        _lastWorkerContext = _firstWorkerContext;
    }
    _lastWorkerContext->_CurEvent=this;
    _lastWorkerContext->_CurThread=_WorkerPool->addThread();
    return _lastWorkerContext;
}

libhttppp::Event::WorkerContext *libhttppp::Event::delWorkerContext(
    libhttppp::Event::WorkerContext *delwrkctx){
    WorkerContext *prevwrk=NULL;
    for(WorkerContext *curwrk=_firstWorkerContext; curwrk; curwrk=curwrk->_nextWorkerContext){
        if(curwrk==delwrkctx){
            if(prevwrk){
                prevwrk->_nextWorkerContext=curwrk->_nextWorkerContext;
            }
            if(curwrk==_firstWorkerContext){
              _firstWorkerContext=curwrk->_nextWorkerContext;  
            }
            if(curwrk==_lastWorkerContext){
              _lastWorkerContext=prevwrk;
            }
            curwrk->_nextWorkerContext=NULL;
            delete curwrk;
        }
        prevwrk=curwrk;
    }
    if(prevwrk)
        return prevwrk->_nextWorkerContext;
    else
        return _firstWorkerContext;
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
