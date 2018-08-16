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

#include "exception.h"
#include "connections.h"
#include "os/os.h"
#include <config.h>

#ifdef Windows
  #include <Windows.h>
  #include <mswsock.h>
  #include <Strsafe.h>
#endif

#ifdef EVENT_EPOLL
  #include <sys/epoll.h>
#endif

#ifndef EVENT_H
#define EVENT_H

namespace libhttppp {
#ifdef MSVC
	class __declspec(dllexport)  Event {
#else
	class __attribute__ ((visibility ("default"))) Event {
#endif
	public:
		Event(ServerSocket *serversocket);
		virtual ~Event();

        /*Worker Events*/
        class ConnectionContext {
        public:
            ConnectionContext     *nextConnectionContext();
        private:
            ConnectionContext();
            ~ConnectionContext();
            /*Indefier Connection*/
            Connection             *_CurConnection;
            /*Linking to CurrentConnectionpoll*/
            ConnectionPool         *_CurCPool;
            /*Linking to Events*/
            Event                  *_CurEvent;
            /*Thread Monitor*/
            Mutex                  *_Mutex;
            /*next entry*/
            ConnectionContext      *_nextConnectionContext;
            /*exception handler*/
            HTTPException           _httpexception;
            friend class Event;
        };
        
        ConnectionContext *addConnection();
        ConnectionContext *delConnection(Connection *delcon);
        
        static void *ReadEvent(void *curcon);
        static void *WriteEvent(void *curcon);
        static void *CloseEvent(void *curcon);
        
	/*API Events*/
	virtual void RequestEvent(Connection *curcon);
	virtual void ResponseEvent(libhttppp::Connection *curcon);
	virtual void ConnectEvent(libhttppp::Connection *curcon);
    virtual void DisconnectEvent(Connection *curcon);
	/*Run Mainloop*/
	virtual void runEventloop();

#ifdef EVENT_IOCP
	static DWORD WINAPI WorkerThread(LPVOID WorkThreadContext);
#endif

#ifdef Windows
		static BOOL WINAPI CtrlHandler(DWORD dwEvent);
#else
        static  void  CtrlHandler(int signum);
#endif

  private:
#ifdef EVENT_EPOLL
	int                 _epollFD;
	struct epoll_event *_Events;
	struct epoll_event  _setEvent;    
#elif EVENT_IOCP
	HANDLE              _IOCP;
	WSAEVENT            _hCleanupEvent[1];
	CRITICAL_SECTION    _CriticalSection;
#endif
    
    /*Connection Context helper*/
    ConnectionContext *_firstConnectionContext;
    ConnectionContext *_lastConnectionContext;
    
    /*Thread Monitor*/
    Mutex              *_Mutex;
    
    HTTPException       _httpexception;
    ServerSocket       *_ServerSocket;
    bool                _EventEndloop;
    bool                _EventRestartloop;
    ConnectionPool     *_Cpool;
  };
}

#endif
