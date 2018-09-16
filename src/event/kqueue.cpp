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
#include "threadpool.h"

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
    _WorkerPool = new ThreadPool;
    _Mutex = new Mutex;
    _firstConnectionContext=NULL;
    _lastConnectionContext=NULL;
    _firstWorkerContext=NULL;
    _lastWorkerContext=NULL;
}

libhttppp::Event::~Event() {
    delete   _Cpool;
    delete[] _Events;
    delete   _firstConnectionContext;
    delete   _WorkerPool;
    delete   _firstWorkerContext;
    _lastWorkerContext=NULL;
    _lastConnectionContext=NULL;
    delete   _Mutex;
}

void libhttppp::Event::CtrlHandler(int signum) {
    _EventEndloop=false;
}

void libhttppp::Event::runEventloop() {
    int srvssocket=_ServerSocket->getSocket();
    int maxconnets=_ServerSocket->getMaxconnections();
    struct kevent setEvent=(struct kevent) {
        0
    };
    _Kq = kqueue();
    EV_SET(&setEvent, srvssocket, EVFILT_READ , EV_ADD || EV_CLEAR || EV_ONESHOT, 0, 0, NULL);
    if (kevent(_Kq, &setEvent, 1, NULL, 0, NULL) == -1)
        _httpexception.Critical("runeventloop","can't create kqueue!");
    signal(SIGPIPE, SIG_IGN);
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
    int srvssocket=wevent->_ServerSocket->getSocket();
    int maxconnets=wevent->_ServerSocket->getMaxconnections();
    SYSInfo sysinfo;
    wctx->_CurThread->setPid(sysinfo.getPid());
    struct kevent setEvent=(struct kevent) {
        0
    };
    int nev = 0;
    while(wevent->_EventEndloop) {
        nev = kevent(wevent->_Kq, NULL, 0, wevent->_Events,maxconnets, NULL);
        if(nev<0) {
            if(errno== EINTR) {
                continue;
            } else {
                httpexception.Critical("epoll wait failure");
                throw httpexception;
            }

        }
        for(int i=0; i<nev; i++) {
            ConnectionContext *curct=NULL;
            if(wevent->_Events[i].ident == (uintptr_t)srvssocket) {
                try {
                    /*will create warning debug mode that normally because the check already connection
                     * with this socket if getconnection throw they will be create a new one
                     */
                    wevent->addConnectionContext(&curct);
#ifdef DEBUG_MUTEX
                    httpexception.Note("runeventloop","Lock ConnectionMutex");
#endif
                    curct->_Mutex->lock();
                    ClientSocket *clientsocket=curct->_CurConnection->getClientSocket();
                    int fd=wevent->_ServerSocket->acceptEvent(clientsocket);
                    clientsocket->setnonblocking();
                    curct->_EventCounter=i;
                    if(fd>0) {
                        setEvent.fflags=0;
                        setEvent.filter=EVFILT_READ;
                        setEvent.flags=EV_ADD;
                        setEvent.udata=(void*) curct;
                        setEvent.ident=(uintptr_t)fd;
                        EV_SET(&setEvent, fd, EVFILT_READ, EV_ADD, 0, 0, (void*) curct);
                        if (kevent(wevent->_Kq, &setEvent, 1, NULL, 0, NULL) == -1) {
                            httpexception.Error("runeventloop","can't accep't in  kqueue!");
                        } else {
                            wevent->ConnectEvent(curct->_CurConnection);
                        }
#ifdef DEBUG_MUTEX
                        httpexception.Note("runeventloop","Unlock ConnectionMutex");
#endif
                        curct->_Mutex->unlock();
                    } else {
#ifdef DEBUG_MUTEX
                        httpexception.Note("runeventloop","Unlock ConnectionMutex");
#endif
                        curct->_Mutex->unlock();
                        wevent->delConnectionContext(curct,NULL);
                    }

                } catch(HTTPException &e) {
#ifdef DEBUG_MUTEX
                    httpexception.Note("runeventloop","Unlock ConnectionMutex");
#endif
                    curct->_Mutex->unlock();
                    wevent->delConnectionContext(curct,NULL);
                    if(e.isCritical())
                        throw e;
                }
            } else {
                curct=(ConnectionContext*)wevent->_Events[i].udata;
                switch(wevent->_Events[i].filter) {
                case EVFILT_READ: {
#ifdef DEBUG_MUTEX
                    httpexception.Note("ReadEvent","lock ConnectionMutex");
#endif
                    curct->_Mutex->lock();
                    ClientSocket *clientsocket=curct->_CurConnection->getClientSocket();
                    int fd=clientsocket->getSocket();
                    try {
                        char buf[BLOCKSIZE];
                        int rcvsize=wevent->_ServerSocket->recvData(clientsocket,buf,BLOCKSIZE);
                        switch(rcvsize) {
                        case -1: {
                            setEvent.filter=EVFILT_READ;
                            setEvent.flags=EV_RECEIPT;
                            EV_SET(&setEvent, fd, EVFILT_READ, EV_RECEIPT, 0, 0, (void*) curct);
                            if (kevent(wevent->_Kq, &setEvent, 1, NULL, 0, NULL) == -1) {
                                httpexception.Error("runeventloop","can't mod  kqueue!");
                            }
                        }
                        case 0: {
                            if(curct->_CurConnection->getSendSize()!=0) {
                                setEvent.filter=EVFILT_WRITE;
                                setEvent.flags=EV_RECEIPT;
                                EV_SET(&setEvent, fd, EVFILT_WRITE, EV_RECEIPT, 0, 0, (void*) curct);
                                if (kevent(wevent->_Kq, &setEvent, 1, NULL, 0, NULL) == -1) {
                                    httpexception.Error("runeventloop","can't mod  kqueue!");
                                } else {
                                    goto CLOSECONNECTION;
                                }
                            }
                        }
                        default: {
                            curct->_CurConnection->addRecvQueue(buf,rcvsize);
                            wevent->RequestEvent(curct->_CurConnection);
                            setEvent.filter=EVFILT_READ;
                            setEvent.flags=EV_RECEIPT;
                            EV_SET(&setEvent, fd, EVFILT_READ, EV_RECEIPT, 0, 0, (void*) curct);
                            if (kevent(wevent->_Kq, &setEvent, 1, NULL, 0, NULL) == -1) {
                                httpexception.Error("runeventloop","can't mod  kqueue!");
                            }
                        }
#ifdef DEBUG_MUTEX
                        httpexception.Note("ReadEvent","unlock ConnectionMutex");
#endif
                        curct->_Mutex->unlock();
                        }
                    } catch(HTTPException &e) {
#ifdef DEBUG_MUTEX
                        httpexception.Note("ReadEvent","unlock ConnectionMutex");
#endif
                        curct->_Mutex->unlock();
                        if(e.isCritical()) {
                            throw e;
                        }
                        if(e.isError()) {
                            curct->_CurConnection->cleanRecvData();
                            goto CLOSECONNECTION;
                        }
                    }
                }
                case EVFILT_WRITE: {
#ifdef DEBUG_MUTEX
                    httpexception.Note("WriteEvent","lock ConnectionMutex");
#endif
                    curct->_Mutex->lock();
                    Connection *con=curct->_CurConnection;
                    try {
                        ssize_t sended=0;
                        while(con->getSendData()) {
                            sended=wevent->_ServerSocket->sendData(con->getClientSocket(),
                                                                   (void*)con->getSendData()->getData(),
                                                                   con->getSendData()->getDataSize());
                            if(sended>0)
                                con->resizeSendQueue(sended);
                        }
                        wevent->ResponseEvent(con);
                    } catch(HTTPException &e) {
#ifdef DEBUG_MUTEX
                        httpexception.Note("WriteEvent","unlock ConnectionMutex");
#endif
                        curct->_Mutex->unlock();
                        goto CLOSECONNECTION;
                    }
#ifdef DEBUG_MUTEX
                    httpexception.Note("WriteEvent","unlock ConnectionMutex");
#endif
                    if(curct)
                        curct->_Mutex->unlock();
                }
                }
                if (wevent->_Events[i].flags & EV_ERROR) {
CLOSECONNECTION:
#ifdef DEBUG_MUTEX
                    httpexception.Note("CloseEvent","ConnectionMutex");
#endif
                    curct->_Mutex->lock();
                    Connection *con=(Connection*)curct->_CurConnection;
                    wevent->DisconnectEvent(con);
                    try {
                        EV_SET(&setEvent,con->getClientSocket()->getSocket(),
                               wevent->_Events[curct->_EventCounter].filter,
                               EV_DELETE, 0, 0, NULL);
                        if (kevent(wevent->_Kq,&setEvent, 1, NULL, 0, NULL) == -1)
                            httpexception.Error("Connection can't delete from kqueue");
#ifdef DEBUG_MUTEX
                        httpexception.Note("CloseEvent","unlock ConnectionMutex");
#endif
                        curct->_Mutex->unlock();
                        wevent->delConnectionContext(curct,NULL);
                        curct=NULL;
                        httpexception.Note("Connection shutdown!");
                    } catch(HTTPException &e) {
#ifdef DEBUG_MUTEX
                        httpexception.Note("CloseEvent","unlock ConnectionMutex");
#endif
                        if(curct)
                            curct->_Mutex->unlock();
                        httpexception.Note("Can't do Connection shutdown!");
                    }
                }
            }
        }
        signal(SIGINT, CtrlHandler);
    }
    return NULL;
}
