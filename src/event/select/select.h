/*******************************************************************************
Copyright (c) 2021, Jan Koester jan.koester@gmx.net
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

#define EVENT_SELECT

#ifndef SELECT_H
#define SELECT_H

namespace libhttppp {

    class ConLock {
    private:
        ConLock();
        ~ConLock();
        int      _Des;
        Thread  *_Thread;
        Lock     _Lock;
        ConLock *_nextConLock;
        friend class SELECT;
    };
    
    class SELECT : public EventApi{
    public:
        SELECT(ServerSocket* serversocket);
        ~SELECT();
        
        /*Lock mechanism*/
        void initLockPool(ThreadPool *pool);
        void destroyLockPool();
        bool LockConnection(Thread *cth,int des);
        void UnlockConnection(Thread *cth,int des);
               
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
        void                      _setSelectEvents(Connection *curcon,int events);
        int                       _selectFD;
        ConLock                  *_ConLock;
        struct epoll_event       *_Events;
        ServerSocket             *_ServerSocket;
    };
};

#endif
