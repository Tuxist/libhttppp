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
#include <cstdlib>
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

libhttppp::ConntectionPtr::ConntectionPtr(){
}

libhttppp::EPOLL::EPOLL(libhttppp::ServerSocket* serversocket) {
    HTTPException httpexception;
    _ServerSocket=serversocket;

}

libhttppp::EPOLL::~EPOLL(){
}

int libhttppp::EPOLL::LockConnection(int des){
    if(!_Events[des].data.ptr)
        return LockConnectionStatus::LOCKNOTREADY;
    if(((ConntectionPtr*)_Events[des].data.ptr)->_ConnectionLock.trylock())
        return LockConnectionStatus::LOCKREADY;
    return LockConnectionStatus::LOCKFAILED;
}

void libhttppp::EPOLL::UnlockConnction(int des)
{
    if(_Events[des].data.ptr)
        ((ConntectionPtr*)_Events[des].data.ptr)->_ConnectionLock.unlock();
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
        _Events[i].data.ptr = NULL;
}

int libhttppp::EPOLL::waitEventHandler(){
    int n = epoll_wait(_epollFD,_Events,_ServerSocket->getMaxconnections(), EPOLLWAIT);
    if(n<0) {
        HTTPException httpexception;
        if(errno== EINTR) {
            httpexception.Warning("initEventHandler:","EINTR");
            return n;
        } else {
            httpexception.Critical("initEventHandler:","epoll wait failure");
            throw httpexception;
        }
    }
    return n;
}

int libhttppp::EPOLL::StatusEventHandler(int des){
    if(!_Events[des].data.ptr)
        return EventHandlerStatus::EVCON;
    else if(((ConntectionPtr*)_Events[des].data.ptr)->_Connection->getSendData())
        return EventHandlerStatus::EVOUT;
    return EventHandlerStatus::EVIN;
}

void libhttppp::EPOLL::ConnectEventHandler(int des){
    HTTPException httpexception;
    struct epoll_event setevent{0};
    Connection* curct = new Connection;
    try {
        /*will create warning debug mode that normally because the check already connection
         * with this socket if getconnection throw they will be create a new one
         */
        curct->getClientSocket()->setnonblocking();
        if(_ServerSocket->acceptEvent(curct->getClientSocket())>0) {
            setevent.events = EPOLLIN | EPOLLOUT;
            setevent.data.ptr = new ConntectionPtr;
            ((ConntectionPtr*)setevent.data.ptr)->_Connection=curct;
            int err=0;
            if((err=epoll_ctl(_epollFD, EPOLL_CTL_ADD, curct->getClientSocket()->Socket, &setevent))==-1){
                httpexception.Error("ConnectEventHandler: ",strerror(errno));
                goto CONNRESET;
            }
            ConnectEvent(curct);
            return;
        }else{
            httpexception.Error("ConnectEventHandler:","Connect Event invalid fildescriptor");
            goto CONNRESET;
        }
    }catch(HTTPException &e) {
        if(e.isCritical())
            goto CONNRESET;
    }
CONNRESET:
    delete curct;
    throw httpexception;
}

void libhttppp::EPOLL::ReadEventHandler(int des){
    HTTPException httpexception;
    Connection *curct=((ConntectionPtr*)_Events[des].data.ptr)->_Connection;
    try {
        char buf[BLOCKSIZE];
        int rcvsize=_ServerSocket->recvData(curct->getClientSocket(),buf,BLOCKSIZE);
        if(rcvsize > 0){
            std::cout << buf << std::endl;
            curct->addRecvQueue(buf,rcvsize);
            RequestEvent(curct);
        }else if(errno!=EAGAIN){
            httpexception.Error("Recv Failed",strerror(errno));
            throw httpexception;
        }
    } catch(HTTPException &e) {
        throw e;
    }
}

void libhttppp::EPOLL::WriteEventHandler(int des){
    HTTPException httpexception;
    Connection *curct=((ConntectionPtr*)_Events[des].data.ptr)->_Connection;
    try {
        ssize_t sended=_ServerSocket->sendData(curct->getClientSocket(),
                                       (void*)curct->getSendData()->getData(),
                                       curct->getSendData()->getDataSize());
        if(sended > 0){
            curct->resizeSendQueue(sended);
            ResponseEvent(curct);
            return;
        }else if(errno!=EAGAIN){
            throw httpexception.Error("WriteEventHandler:",strerror(errno));
        }
    } catch(HTTPException &e) {
        throw e;
    }
}

void libhttppp::EPOLL::CloseEventHandler(int des){
    HTTPException httpexception;   
    struct epoll_event setevent{0};
    Connection *curct=((ConntectionPtr*)_Events[des].data.ptr)->_Connection;
    DisconnectEvent(curct);
    try {
        int ect=epoll_ctl(_epollFD, EPOLL_CTL_DEL, curct->getClientSocket()->Socket,&setevent);
        if(ect<0) {
            httpexception.Error("CloseEventHandler:","can't delete Connection from epoll");
            throw httpexception;
        }
        delete curct;
        delete (ConntectionPtr*)_Events[des].data.ptr;
        _Events[des].data.ptr=NULL;
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
