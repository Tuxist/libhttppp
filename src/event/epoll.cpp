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
#include <sys/epoll.h>
#include <sys/sysinfo.h>
#include <cstdlib>
#include <config.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#define READEVENT 0
#define SENDEVENT 1

#include "../event.h"

libhttppp::Queue::Queue(ServerSocket *serversocket) {
    _ServerSocket=serversocket;
    _ServerSocket->setnonblocking();
    _ServerSocket->listenSocket();
    _EventEndloop =true;
}

libhttppp::Queue::~Queue() {

}

libhttppp::Queue* _QueueIns=NULL;

void libhttppp::Queue::CtrlHandler(int signum) {
    _QueueIns->_EventEndloop=false;
}

void libhttppp::Queue::runEventloop() {
    int threadcount=get_nprocs()*2;
    pthread_t threads[threadcount];
    for(int i=0; i<threadcount; i++){
    int thc = pthread_create( &threads[i], NULL, &WorkerThread,(void*) this);
      if( thc != 0 ) {
        _httpexception.Cirtical("can't create thread");
        throw _httpexception;
      }  
    }
    for(int i=0; i<threadcount; i++){
        pthread_join(threads[i], NULL);
    }
}
    
void *libhttppp::Queue::WorkerThread(void *instance){
    Queue *queue=(Queue *)instance;
    ConnectionPool cpool(queue->_ServerSocket);
    struct epoll_event *events;
    struct epoll_event  event = {0};
    events = new epoll_event[(queue->_ServerSocket->getMaxconnections()*sizeof(struct epoll_event))];
    for(int i=0; i<queue->_ServerSocket->getMaxconnections(); i++)
        events[i].data.fd = -1;
    int epollfd = epoll_create(queue->_ServerSocket->getMaxconnections());

    if (epollfd == -1) {
        queue->_httpexception.Cirtical("can't create epoll");
        throw queue->_httpexception;
    }

    event.events = EPOLLIN | EPOLLOUT | EPOLLONESHOT;
    event.data.fd = queue->_ServerSocket->getSocket();
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, queue->_ServerSocket->getSocket(), &event) < 0) {
        queue->_httpexception.Cirtical("can't create epoll");
        throw queue->_httpexception;
    }

    while(queue->_EventEndloop) {
        int n = epoll_wait(epollfd, events, queue->_ServerSocket->getMaxconnections(), EPOLLWAIT);
        for(int i=0; i<n; i++) {
            Connection *curcon=NULL;
            if(events[i].data.fd == queue->_ServerSocket->getSocket()) {
                try {
                    /*will create warning debug mode that normally because the check already connection
                     * with this socket if getconnection throw they will be create a new one
                     */
                    curcon=cpool.addConnection();
                    ClientSocket *clientsocket=curcon->getClientSocket();
                    int fd=queue->_ServerSocket->acceptEvent(clientsocket);
                    if(fd>0) {
                        event.data.fd = fd;
                        clientsocket->setnonblocking();
                        event.events = EPOLLIN |EPOLLOUT |EPOLLRDHUP;
                        if(epoll_ctl(epollfd, EPOLL_CTL_ADD, event.data.fd, &event)==-1 && errno==EEXIST)
                            epoll_ctl(epollfd, EPOLL_CTL_MOD, events[i].data.fd, &event);
                        queue->ConnectEvent(curcon);
                    } else {
                        cpool.delConnection(curcon);
                    }
                } catch(HTTPException &e) {
                    cpool.delConnection(curcon);
                    if(e.isCritical())
                        throw e;
                }
                continue;
            } else {
                curcon=cpool.getConnection(events[i].data.fd);
            }

            if(events[i].events & EPOLLRDHUP || events[i].events & EPOLLERR) {
CloseConnection:
                queue->DisconnectEvent(curcon);
                try {
                    cpool.delConnection(curcon);
                    curcon=NULL;
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, &event);
                    queue->_httpexception.Note("Connection shutdown!");
                    continue;
                } catch(HTTPException &e) {
                    queue->_httpexception.Note("Can't do Connection shutdown!");
                }
                ;
            }


            if(events[i].events & EPOLLIN) {
                try {
                    char buf[BLOCKSIZE];
                    int rcvsize=0;
                    try {
                        rcvsize=queue->_ServerSocket->recvData(curcon->getClientSocket(),buf,BLOCKSIZE);
                        if(rcvsize>0) {
                            curcon->addRecvQueue(buf,rcvsize);
                        }
                    } catch(HTTPException &e) {

                    }
                    queue->RequestEvent(curcon);
                    event.events = EPOLLIN | EPOLLOUT |EPOLLRDHUP;
                    epoll_ctl(epollfd, EPOLL_CTL_MOD, events[i].data.fd, &event);
                } catch(HTTPException &e) {
                    if(e.isCritical()) {
                        throw e;
                    }
                    if(e.isError())
                        goto CloseConnection;
                }
            }

            if(events[i].events & EPOLLOUT) {
                try {
                    if(curcon && curcon->getSendData()) {
                        ssize_t sended=0;

                        sended=queue->_ServerSocket->sendData(curcon->getClientSocket(),
                                                       (void*)curcon->getSendData()->getData(),
                                                       curcon->getSendData()->getDataSize(),MSG_NOSIGNAL);


                        if(sended==-1) {
                            queue->_httpexception.Note("Sending Failed");
                            if (errno == EAGAIN || EWOULDBLOCK) {
                                continue;
                            } else {
                                curcon->cleanSendData();
                                goto CloseConnection;
                            }
                        } else {
                            curcon->resizeSendQueue(sended);
                        }
                    } else {
                        event.events = EPOLLIN |EPOLLRDHUP;
                        epoll_ctl(epollfd, EPOLL_CTL_MOD, events[i].data.fd, &event);
                    }
                    queue->ResponseEvent(curcon);
                } catch(HTTPException &e) {
                    goto CloseConnection;
                }
            }
        }
        _QueueIns=queue;
        signal(SIGINT, CtrlHandler);
    }
    delete[] events;
    return NULL;
}

void libhttppp::Queue::RequestEvent(Connection *curcon) {
    return;
}

void libhttppp::Queue::ResponseEvent(libhttppp::Connection *curcon) {
    return;
};

void libhttppp::Queue::ConnectEvent(libhttppp::Connection *curcon) {
    return;
};

void libhttppp::Queue::DisconnectEvent(Connection *curcon) {
    return;
}
