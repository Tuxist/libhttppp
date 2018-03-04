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
            /*next entry*/
            ConnectionContext      *_nextConnectionContext;
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
		typedef enum _IO_OPERATION {
			ClientIoAccept,
			ClientIoRead,
			ClientIoWrite
		} IO_OPERATION, *PIO_OPERATION;

		//
		// data to be associated for every I/O operation on a socket
		//
		typedef struct _PER_IO_CONTEXT {
			WSAOVERLAPPED               Overlapped;
			char                        Buffer[BLOCKSIZE];
			WSABUF                      wsabuf;
			int                         nTotalBytes;
			int                         nSentBytes;
			IO_OPERATION                IOOperation;
			SOCKET                      SocketAccept;

			struct _PER_IO_CONTEXT      *pIOContextForward;
		} PER_IO_CONTEXT, *PPER_IO_CONTEXT;

		//
		// For AcceptEx, the IOCP key is the PER_SOCKET_CONTEXT for the listening socket,
		// so we need to another field SocketAccept in PER_IO_CONTEXT. When the outstanding
		// AcceptEx completes, this field is our connection socket handle.
		//

		//
		// data to be associated with every socket added to the IOCP
		//
		typedef struct _PER_SOCKET_CONTEXT {
            Connection                 *CurConnection;
            
			LPFN_ACCEPTEX               fnAcceptEx;
            
			//
			//linked list for all outstanding i/o on the socket
			//
			PPER_IO_CONTEXT             pIOContext;
			struct _PER_SOCKET_CONTEXT  *pCtxtBack;
			struct _PER_SOCKET_CONTEXT  *pCtxtForward;
		} PER_SOCKET_CONTEXT, *PPER_SOCKET_CONTEXT;

		static BOOL WINAPI CtrlHandler(DWORD dwEvent);
		static DWORD WINAPI WorkerThread(LPVOID WorkThreadContext);
		BOOL CreateAcceptSocket(BOOL fUpdateIOCP);
		PPER_SOCKET_CONTEXT UpdateCompletionPort(SOCKET sd, IO_OPERATION ClientIo,
			                                     BOOL bAddToList);
		VOID CloseClient(PPER_SOCKET_CONTEXT lpPerSocketContext, BOOL bGraceful);
		PPER_SOCKET_CONTEXT CtxtAllocate(SOCKET sd, IO_OPERATION ClientIO);
		VOID CtxtListAddTo(PPER_SOCKET_CONTEXT lpPerSocketContext);
		VOID CtxtListDeleteFrom(PPER_SOCKET_CONTEXT lpPerSocketContext);
		VOID CtxtListFree();
		SOCKET CreateSocket(void);
#else
        static  void  CtrlHandler(int signum);
#endif
  private:
#ifdef EVENT_EPOLL
  int                 _epollFD;
  struct epoll_event *_Events;
  struct epoll_event  _setEvent;    
#endif
#ifdef EVENT_IOCP
	PPER_SOCKET_CONTEXT _pCtxtListenSocket;
	PPER_SOCKET_CONTEXT _pCtxtList;
	HANDLE              _ThreadHandles[MAX_WORKER_THREAD];
	HANDLE              _IOCP;
	WSAEVENT            _CleanupEvent[1];
	CRITICAL_SECTION    _CriticalSection;
#endif
    
    /*Connection Context helper*/
    ConnectionContext *_firstConnectionContext;
    ConnectionContext *_lastConnectionContext;
    
    HTTPException       _httpexception;
    ServerSocket       *_ServerSocket;
	bool                _EventEndloop;
	bool                _EventRestartloop;
    ConnectionPool     *_Cpool;
  };
}

#endif
