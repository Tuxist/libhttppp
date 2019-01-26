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

#define DEBUG_MUTEX

bool libhttppp::Event::_EventEndloop=true;
bool libhttppp::Event::_EventRestartloop=true;

libhttppp::Event::ConnectionContext::ConnectionContext() {
    _Lock=new Lock;
    _CurConnection=NULL;
}

libhttppp::Event::ConnectionContext::~ConnectionContext() {
    delete _Lock;
    delete _CurConnection;
}


libhttppp::Event::ConnectionContext * libhttppp::Event::ConnectionContext::nextConnectionContext() {
    return _nextConnectionContext;
}

void libhttppp::Event::addConnectionContext(libhttppp::Event::ConnectionContext **addcon) {
    _Lock->lock();
    HTTPException httpexception;
    if(!addcon)
        return;
    if(_firstConnectionContext) {
        ConnectionContext *prevcon=_lastConnectionContext;;
        prevcon->_nextConnectionContext=new ConnectionContext;
        _lastConnectionContext=prevcon->_nextConnectionContext;
    } else {
        _firstConnectionContext=new ConnectionContext;
        _lastConnectionContext=_firstConnectionContext;
    }
#ifndef EVENT_IOCP
    _lastConnectionContext->_CurConnection=new Connection;
#else
	_lastConnectionContext->_CurConnection = new Event::ConnectionContext::IOCPConnection;
#endif
    *addcon=_lastConnectionContext;
    _Lock->unlock();
}

void libhttppp::Event::delConnectionContext(libhttppp::Event::ConnectionContext *delctx,
        libhttppp::Event::ConnectionContext **nextcxt) {
    _Lock->lock();
    HTTPException httpexception;
    ConnectionContext *prevcontext=NULL;
    for(ConnectionContext *curcontext=_firstConnectionContext; curcontext;
            curcontext=curcontext->nextConnectionContext()) {
        if(curcontext==delctx) {
            if(prevcontext) {
                prevcontext->_nextConnectionContext=curcontext->_nextConnectionContext;
            }
            if(_firstConnectionContext==curcontext) {
                if(_lastConnectionContext==_firstConnectionContext) {
                    _firstConnectionContext=_firstConnectionContext->nextConnectionContext();
                    _lastConnectionContext=_firstConnectionContext;
                } else {
                    _firstConnectionContext=_firstConnectionContext->nextConnectionContext();
                }
            }
            if(_lastConnectionContext==delctx) {
                if(prevcontext) {
                    _lastConnectionContext=prevcontext;
                } else {
                    _lastConnectionContext=_firstConnectionContext;
                }
            }
            curcontext->_nextConnectionContext=NULL;
            delete curcontext;
            break;
        }
        prevcontext=curcontext;
    }
    if(nextcxt) {
        if(prevcontext && prevcontext->_nextConnectionContext) {
            *nextcxt= prevcontext->_nextConnectionContext;
        } else {
            *nextcxt=_firstConnectionContext;
        }
    }
    _Lock->unlock();
}

libhttppp::Event::WorkerContext::WorkerContext() {
    _CurEvent=NULL;
    _CurThread=NULL;
    _nextWorkerContext=NULL;
}

libhttppp::Event::WorkerContext::~WorkerContext() {
    delete _nextWorkerContext;
}

libhttppp::Event::WorkerContext *libhttppp::Event::addWorkerContext() {
    if(_firstWorkerContext) {
        _lastWorkerContext->_nextWorkerContext=new WorkerContext;
        _lastWorkerContext=_lastWorkerContext->_nextWorkerContext;
    } else {
        _firstWorkerContext=new WorkerContext;
        _lastWorkerContext = _firstWorkerContext;
    }
    _lastWorkerContext->_CurEvent=this;
    _lastWorkerContext->_CurThread=_WorkerPool->addThread();
    return _lastWorkerContext;
}

libhttppp::Event::WorkerContext *libhttppp::Event::delWorkerContext(
    libhttppp::Event::WorkerContext *delwrkctx) {
    WorkerContext *prevwrk=NULL;
    for(WorkerContext *curwrk=_firstWorkerContext; curwrk; curwrk=curwrk->_nextWorkerContext) {
        if(curwrk==delwrkctx) {
            if(prevwrk) {
                prevwrk->_nextWorkerContext=curwrk->_nextWorkerContext;
            }
            if(curwrk==_firstWorkerContext) {
                _firstWorkerContext=curwrk->_nextWorkerContext;
            }
            if(curwrk==_lastWorkerContext) {
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

