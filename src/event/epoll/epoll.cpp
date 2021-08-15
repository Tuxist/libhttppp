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

#include <iostream>

#include <fcntl.h>
#include <sys/sysinfo.h>
#include <config.h>
#include <errno.h>
#include <signal.h>

#include "os/os.h"
#include "../../exception.h"
#include "../../connections.h"

#define READEVENT 0
#define SENDEVENT 1

#define DEBUG_MUTEX

#include "epoll.h"

libhttppp::ConLock::ConLock()
{
    _Des=-1;
    _nextConLock=nullptr;
}

libhttppp::ConLock::~ConLock()
{
    delete _nextConLock;
}

libhttppp::EPOLL::EPOLL(libhttppp::ServerSocket* serversocket) {
    HTTPException httpexception;
    _ServerSocket=serversocket;
}

libhttppp::EPOLL::~EPOLL(){
}

void libhttppp::EPOLL::initLockPool(int locks){
    _ConLock=nullptr;
    for(int i=0; i <locks; ++i){
        if(!_ConLock){
            _ConLock=new ConLock();
        }else{
            _ConLock->_nextConLock = new ConLock();
            _ConLock = _ConLock->_nextConLock;
        }
    }   
}

void libhttppp::EPOLL::destroyLockPool(){
    delete _ConLock;
}


bool libhttppp::EPOLL::LockConnection(int des) {
    ConLock *curlock=_ConLock;
    while(curlock){
        if(curlock->_Des==des)
            return false;
        curlock=curlock->_nextConLock;
    }
    for(ConLock *frlock=_ConLock; frlock; frlock=frlock->_nextConLock){
        if(frlock->_Des==(-1)){
            if(frlock->_Lock.trylock()){
                frlock->_Des=des;
                return true;
            }
        }
    }
    return false;
}

void libhttppp::EPOLL::UnlockConnection(int des){
    for(ConLock *curlock=_ConLock; curlock; curlock=curlock->_nextConLock){
        if(curlock->_Des==des){
            _ConLock->_Des=-1;
            _ConLock->_Lock.unlock();
        }
    }
}

const char * libhttppp::EPOLL::getEventType(){
    return "EPOLL";
}

void libhttppp::EPOLL::initEventHandler(){
    HTTPException httpexception;
    try{
        _ServerSocket->setnonblocking();
        _ServerSocket->listenSocket();
    }catch(HTTPException &e){
        throw e;
    }
    
    struct epoll_event setevent= (struct epoll_event) {
        0
    };

    _epollFD = epoll_create1(0);

    if (_epollFD == -1) {
        httpexception.Critical("initEventHandler:","can't create epoll");
        throw httpexception;
    }

    setevent.events = EPOLLIN|EPOLLOUT;

    if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, _ServerSocket->getSocket(), &setevent) < 0) {
        httpexception.Critical("initEventHandler:","can't create epoll");
        throw httpexception;
    }
    
    _Events = new epoll_event[(_ServerSocket->getMaxconnections())];
    for(int i=0; i<_ServerSocket->getMaxconnections(); ++i)
        _Events[i].data.ptr = nullptr;
}

int libhttppp::EPOLL::waitEventHandler(){
EVENTPOLLWAIT:
    int n = epoll_wait(_epollFD,_Events,_ServerSocket->getMaxconnections(), EPOLLWAIT);
    if(n<0) {
        HTTPException httpexception;
        if(errno== EINTR) {
            httpexception.Warning("initEventHandler:","EINTR");
            goto EVENTPOLLWAIT;
        } else {
            httpexception.Critical("initEventHandler:","epoll wait failure");
            throw httpexception;
        }
    }
    return n;
}

int libhttppp::EPOLL::StatusEventHandler(int des){
    HTTPException httpexception;
    if(!_Events[des].data.ptr){
        return EventHandlerStatus::EVNOTREADY;
    }else if(((Connection*)_Events[des].data.ptr)->getSendData()){
        return EventHandlerStatus::EVOUT;
    }
    return EventHandlerStatus::EVIN;
}

void libhttppp::EPOLL::ConnectEventHandler(int des) {
    HTTPException httpexception;
    struct epoll_event setevent { 0 };
    Connection* curct = new Connection;
    try {
        /*will create warning debug mode that normally because the check already connection
         * with this socket if getconnection throw they will be create a new one
         */
        curct->getClientSocket()->setnonblocking();
        if (_ServerSocket->acceptEvent(curct->getClientSocket()) > 0) {
            setevent.events = EPOLLIN | EPOLLOUT;
            setevent.data.ptr = curct;
            int err = 0;
            if ((err = epoll_ctl(_epollFD, EPOLL_CTL_ADD, curct->getClientSocket()->Socket, &setevent)) == -1) {
#ifdef __GLIBCXX__
                char errbuf[255];
                httpexception.Error("ConnectEventHandler: ",strerror_r(errno, errbuf, 255));
#else
                char errbuf[255];
                strerror_r(errno, errbuf, 255);
                httpexception.Error("ConnectEventHandler: ",errbuf);
#endif
                throw httpexception;
            }
            ConnectEvent(curct);
        }else {
            httpexception.Error("ConnectEventHandler:", "Connect Event invalid fildescriptor");
            throw httpexception;
        }
    }catch (HTTPException& e) {
        delete curct;
        setevent.data.ptr=nullptr;
        throw e;
    }
    
}

void libhttppp::EPOLL::ReadEventHandler(int des){
    HTTPException httpexception;
    Connection *curct=((Connection*)_Events[des].data.ptr);
    try {
        char buf[BLOCKSIZE];
        int rcvsize=_ServerSocket->recvData(curct->getClientSocket(),&buf,BLOCKSIZE);
        rcvsize ? curct->addRecvQueue(buf,rcvsize) : curct->addRecvQueue(buf,BLOCKSIZE);
        RequestEvent(curct);
    } catch(HTTPException &e) {
        throw e;
    }
}

void libhttppp::EPOLL::WriteEventHandler(int des){
    HTTPException httpexception;
    Connection *curct=((Connection*)_Events[des].data.ptr);
    try {
        ssize_t sended=_ServerSocket->sendData(curct->getClientSocket(),
                                       (void*)curct->getSendData()->getData(),
                                       curct->getSendData()->getDataSize());
        sended ? curct->resizeSendQueue(sended) : curct->resizeSendQueue(
                                                   curct->getSendData()->getDataSize());
        ResponseEvent(curct);
    } catch(HTTPException &e) {
        curct->cleanSendData();
        throw e;
    }
}

void libhttppp::EPOLL::CloseEventHandler(int des){
    HTTPException httpexception;
    struct epoll_event setevent { 0 };
    try {
        Connection *curct=((Connection*)_Events[des].data.ptr);
        if(!curct)
            return;
        int ect=epoll_ctl(_epollFD, EPOLL_CTL_DEL, curct->getClientSocket()->Socket, &setevent);
        if(ect==-1) {
            httpexception.Error("CloseEvent","can't delete Connection from epoll");
            throw httpexception;
        }
        DisconnectEvent(curct);
        delete (Connection*)_Events[des].data.ptr;
        _Events[des].data.ptr=nullptr;
        httpexception.Note("CloseEventHandler:","Connection shutdown!");
    } catch(HTTPException &e) {
        httpexception.Note("CloseEventHandler:","Can't do Connection shutdown!");
    }
}


/*API Events*/

void libhttppp::EPOLL::RequestEvent(Connection *curcon){
    return;
};

void libhttppp::EPOLL::ResponseEvent(Connection *curcon){
    return;
};

void libhttppp::EPOLL::ConnectEvent(Connection *curcon){
    return;
};

void libhttppp::EPOLL::DisconnectEvent(Connection *curcon){
    return;
};
