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
#include <sys/epoll.h>

#include "os/os.h"
#include "threadpool.h"

#define READEVENT 0
#define SENDEVENT 1

#define DEBUG_MUTEX

#include "../event.h"

void libhttppp::IOCP::addConnectionContext(libhttppp::ConnectionContext **addcon) {
	_Lock->lock();
	HTTPException httpexception;
	if (!addcon)
		return;
	if (_firstConnectionContext) {
		libhttppp::ConnectionContext *prevcon = _lastConnectionContext;;
		prevcon->_nextConnectionContext = new ConnectionContext;
		_lastConnectionContext = prevcon->_nextConnectionContext;
	}
	else {
		_firstConnectionContext = new ConnectionContext;
		_lastConnectionContext = _firstConnectionContext;
	}
	_lastConnectionContext->_CurConnection = new Event::ConnectionContext::IOCPConnection;
	*addcon = _lastConnectionContext;
	_Lock->unlock();
}


libhttppp::Event::Event(ServerSocket *serversocket) {
    _ServerSocket=serversocket;
    _ServerSocket->setnonblocking();
    _ServerSocket->listenSocket();
    _EventEndloop =true;
    _WorkerPool = new ThreadPool;
    _Lock = new Lock;
    _firstConnectionContext=NULL;
    _lastConnectionContext=NULL;
    _firstWorkerContext=NULL;
    _lastWorkerContext=NULL;
}

libhttppp::Event::~Event() {
    delete   _firstConnectionContext;
    delete   _WorkerPool;
    delete   _Lock;
    delete   _firstWorkerContext;
    _lastWorkerContext=NULL;
    _lastConnectionContext=NULL;
}

void libhttppp::Event::CtrlHandler(int signum) {
    if(_EventEndloop!=false)
      _EventEndloop=false;
}

void libhttppp::Event::runEventloop() {
    HTTPException httpexception;
    struct epoll_event setevent= (struct epoll_event) {
        0
    };

    _epollFD = epoll_create1(0);

    if (_epollFD == -1) {
        httpexception.Critical("can't create epoll");
        throw httpexception;
    }

    setevent.events = EPOLLIN|EPOLLET;
    setevent.data.fd = _ServerSocket->getSocket();

    if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, _ServerSocket->getSocket(), &setevent) < 0) {
        httpexception.Critical("can't create epoll");
        throw httpexception;
    }
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, CtrlHandler);
    SYSInfo sysinfo;
    size_t thrs = sysinfo.getNumberOfProcessors();
    for(size_t i=0; i<thrs; i++) {
        WorkerContext *curwrkctx=addWorkerContext();
        curwrkctx->_CurThread->Create(WorkerThread,curwrkctx);
    }
    for(WorkerContext *curth=_firstWorkerContext; curth; curth=curth->_nextWorkerContext) {
        curth->_CurThread->Join();
    }
}

void *libhttppp::Event::WorkerThread(void *wrkevent) {
    HTTPException httpexception;
    WorkerContext *wctx=(WorkerContext*)wrkevent;
    Event *wevent=wctx->_CurEvent;
    SYSInfo sysinfo;
    wctx->_CurThread->setPid(sysinfo.getPid());
    int srvssocket=wevent->_ServerSocket->getSocket();
    int maxconnets=wevent->_ServerSocket->getMaxconnections();
    struct epoll_event setevent= (struct epoll_event) {
        0
    };
    struct epoll_event *events = new epoll_event[(wevent->_ServerSocket->getMaxconnections())];
    for(int i=0; i<maxconnets; i++)
        events[i].data.fd = -1;
    while(Event::_EventEndloop) {
        int n = epoll_wait(wevent->_epollFD,events,maxconnets, EPOLLWAIT);
        if(n<0) {
            if(errno== EINTR) {
                continue;
            } else {
                httpexception.Critical("epoll wait failure");
                throw httpexception;
            }

        }
        for(int i=0; i<n; i++) {
            ConnectionContext *curct=NULL;
            if(events[i].data.fd == srvssocket) {
                try {
                    /*will create warning debug mode that normally because the check already connection
                     * with this socket if getconnection throw they will be create a new one
                     */
                    wevent->addConnectionContext(&curct);
                    ClientSocket *clientsocket=curct->_CurConnection->getClientSocket();
                    int fd=wevent->_ServerSocket->acceptEvent(clientsocket);
                    clientsocket->setnonblocking();
                    if(fd>0) {
                        setevent.data.ptr = (void*) curct;
                        setevent.events = EPOLLIN|EPOLLRDHUP;
                        if(epoll_ctl(wevent->_epollFD, EPOLL_CTL_ADD, fd, &setevent)==-1 && errno==EEXIST){
                            epoll_ctl(wevent->_epollFD, EPOLL_CTL_MOD,fd, &setevent);
                        }
                        wevent->ConnectEvent(curct->_CurConnection);
                    } else {
                        wevent->delConnectionContext(curct,NULL);
                    }

                } catch(HTTPException &e) {
                    wevent->delConnectionContext(curct,NULL);
                    if(e.isCritical())
                        throw e;
                }
            } else {
                while(wevent->_Lock->isLocked());
                curct=(ConnectionContext*)events->data.ptr;
                switch(events[i].events) {
                case(EPOLLIN): {
                    ClientSocket *clientsocket=curct->_CurConnection->getClientSocket();
                    int fd=clientsocket->getSocket();
                    try {
                        char buf[BLOCKSIZE];
                        int rcvsize=0;
                        rcvsize=wevent->_ServerSocket->recvData(clientsocket,buf,BLOCKSIZE);
                        switch(rcvsize) {
                        case -1: {
                            setevent.events = EPOLLIN|EPOLLRDHUP;
                            epoll_ctl(wevent->_epollFD, EPOLL_CTL_MOD, fd, &setevent);
                        }
                        case 0: {
                            if(curct->_CurConnection->getSendSize()<0) {
                                setevent.events = EPOLLOUT |EPOLLRDHUP;
                                epoll_ctl(wevent->_epollFD, EPOLL_CTL_MOD, fd, &setevent);;
                            } else {
                                goto ClOSECONNECTION;
                            }
                        }
                        default: {
                            curct->_CurConnection->addRecvQueue(buf,rcvsize);
                            wevent->RequestEvent(curct->_CurConnection);
                            setevent.events = EPOLLIN|EPOLLRDHUP;
                            epoll_ctl(wevent->_epollFD, EPOLL_CTL_MOD, fd, &setevent);
                        }
                        }
                    } catch(HTTPException &e) {
                        if(e.isCritical()) {
                            throw e;
                        } else if(e.isError()) {
                            curct->_CurConnection->cleanRecvData();
                        }
                    }
                }
                case (EPOLLOUT): {
                    ClientSocket *clientsocket=curct->_CurConnection->getClientSocket();
                    int fd=clientsocket->getSocket();
                    try {
                        ssize_t sended=0;
                        if(curct->_CurConnection->getSendData()) {
                            sended=wevent->_ServerSocket->sendData(clientsocket,
                                                                   (void*)curct->_CurConnection->getSendData()->getData(),
                                                                   curct->_CurConnection->getSendData()->getDataSize());
                            if(sended>0) {
                                curct->_CurConnection->resizeSendQueue(sended);
                                wevent->ResponseEvent(curct->_CurConnection);
                            }
                            setevent.events = EPOLLOUT|EPOLLRDHUP;
                            epoll_ctl(wevent->_epollFD, EPOLL_CTL_MOD, fd, &setevent);
                        } else {
                            setevent.events = EPOLLIN|EPOLLRDHUP;
                            epoll_ctl(wevent->_epollFD, EPOLL_CTL_MOD, fd, &setevent);
                        }
                    } catch(HTTPException &e) {
                        goto ClOSECONNECTION;
                    }
                }
                case EPOLLERR|EPOLLHUP: {
ClOSECONNECTION:
                    ClientSocket *clientsocket=curct->_CurConnection->getClientSocket();
                    int fd=clientsocket->getSocket();
                    wevent->DisconnectEvent(curct->_CurConnection);
                    try {
                        int ect=epoll_ctl(wevent->_epollFD, EPOLL_CTL_DEL, fd, &setevent);
                        if(ect==-1) {
                            httpexception.Error("CloseEvent","can't delete Connection from epoll");
                            throw httpexception;
                        }
                        wevent->delConnectionContext(curct,NULL);
                        curct=NULL;
                        httpexception.Note("Connection shutdown!");
                        continue;
                    } catch(HTTPException &e) {
                        httpexception.Note("Can't do Connection shutdown!");
                    }
                    
                }
                }
                
            }
        }
    }
    delete[] events;
    return NULL;
}



