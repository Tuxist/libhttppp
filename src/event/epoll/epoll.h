/*******************************************************************************
Copyright (c) 2019, Jan Koester jan.koester@gmx.net
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

#include "../../eventapi.h"
#include "../../os/os.h"
#include <sys/epoll.h>

#define EVENT_EPOLL

#ifndef EPOLL_H
#define EPOLL_H

namespace libhttppp {
   
    class EPOLL : public EventApi{
    public:
        EPOLL(ServerSocket* serversocket);
        ~EPOLL();
        
        /*Lock mechanism*/
        void initLockPool(ThreadPool *pool);
        void destroyLockPool();
        int  LockConnection(int des);
        void UnlockConnection(int des);
               
        /*event handler function*/
        void       initEventHandler();
        int        waitEventHandler();
        void       ConnectEventHandler(int des);
        int        StatusEventHandler(int des);
        void       ReadEventHandler(int des);
        void       WriteEventHandler(int des);
        void       CloseEventHandler(int des);
        const char *getEventType();

        /*HTTP API Events*/
        void RequestEvent(Connection *curcon);
        void ResponseEvent(Connection *curcon);
        void ConnectEvent(Connection *curcon);
        void DisconnectEvent(Connection *curcon);
        
        /*Connection Ready to send Data*/
        void sendReady(Connection *curcon,bool ready);
        
    private:
        void                      _setEpollEvents(Connection *curcon,int events);
        int                       _epollFD;
        Lock                      _ConLock;
        struct epoll_event       *_Events;
        ServerSocket             *_ServerSocket;
    };
};

#endif
