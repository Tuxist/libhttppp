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

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <errno.h>
#include <err.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

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
    int threadcount=1;
    pthread_t threads[threadcount];
    for(int i=0; i<threadcount; i++){
        int thc = pthread_create( &threads[i], NULL, &WorkerThread,(void*) this);
        if( thc != 0 ) {
            _httpexception.Critical("can't create thread");
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
    int srvssocket=queue->_ServerSocket->getSocket();
    int maxconnets=queue->_ServerSocket->getMaxconnections();
    
    struct kevent setevent;
    struct kevent *events;
    events = new struct kevent[(maxconnets*sizeof(struct kevent))];
    int kq = kqueue();
    if (kq    == -1){
        queue->_httpexception.Critical("can't create kqueue");
        throw queue->_httpexception;
    }
    /* Initialize kevent structure. */
    EV_SET(&setevent,srvssocket, EVFILT_READ | EVFILT_WRITE , EV_ADD | EV_ONESHOT, 0,0,NULL);
    /* Attach event to the    kqueue.    */
    int ret = kevent(kq, &setevent, 1, events, maxconnets, NULL);
    if (ret == -1){
        queue->_httpexception.Critical("can't attach event kqueue");
        throw queue->_httpexception;
    }
    
    if (setevent.flags & EV_ERROR){
        queue->_httpexception.Critical("everror on event kqueue");
        throw queue->_httpexception;
    }
    
    
    while(queue->_EventEndloop) {
        printf("test\n");
        ret = kevent(kq, NULL, 0, events, maxconnets, NULL);
        printf("ret: %d \n",ret);
        for(int i=0; i<ret; i++) {
            printf("loop\n");
            Connection *curcon=NULL;
            if((int)events[i].ident == srvssocket) {
                try {
                    /*will create warning debug mode that normally because the check already connection
                     * with this socket if getconnection throw they will be create a new one
                     */
                    printf("connecting\n");
                    curcon=cpool.addConnection();
                    ClientSocket *clientsocket=curcon->getClientSocket();
                    int fd=queue->_ServerSocket->acceptEvent(clientsocket);
                    clientsocket->setnonblocking();
                    if(fd>0) {
                        events[i].udata=(void*)curcon;
                        queue->ConnectEvent(curcon);
                    } else {
                        cpool.delConnection(curcon);
                    }
                } catch(HTTPException &e) {
                    cpool.delConnection(curcon);
                    if(e.isCritical())
                        throw e;
                }
            } else {
                curcon=(Connection*)events[i].udata;
            }
            if(events[i].flags & EVFILT_READ) {
                try {
                    char buf[BLOCKSIZE];
                    int rcvsize=0;
                    rcvsize=queue->_ServerSocket->recvData(curcon->getClientSocket(),buf,BLOCKSIZE);
                    if(rcvsize>0) {
                        curcon->addRecvQueue(buf,rcvsize);
                        queue->RequestEvent(curcon);
                    }else{
                        continue;
                    }
                } catch(HTTPException &e) {
                    if(e.isCritical()) {
                        throw e;
                    }
                    if(e.isError()){
                        printf("socket: %d \n",curcon->getClientSocket()->getSocket());
                        goto CloseConnection;
                    }
                    
                }
            }else if(events[i].flags & EV_EOF) {
            CloseConnection:
                queue->DisconnectEvent(curcon);
                try {
                    cpool.delConnection(curcon);
                    curcon=NULL;
                    queue->_httpexception.Note("Connection shutdown!");
                    continue;
                } catch(HTTPException &e) {
                    queue->_httpexception.Note("Can't do Connection shutdown!");
                }
                ;
            }else if(events[i].flags & EVFILT_WRITE) {
                try {
                    if(curcon->getSendData()) {
                        ssize_t sended=0;
                        sended=queue->_ServerSocket->sendData(curcon->getClientSocket(),
                                                              (void*)curcon->getSendData()->getData(),
                                                              curcon->getSendData()->getDataSize());
                        
                        
                        if(sended>0){
                            curcon->resizeSendQueue(sended);
                            queue->ResponseEvent(curcon);
                        }
                    }
                } catch(HTTPException &e) {
                    curcon->cleanSendData();
                    goto CloseConnection;
                }
            }else{
                goto CloseConnection;
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
