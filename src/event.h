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
#include "socket.h"
#include <config.h>

#ifdef Windows
  #include <Windows.h>
#endif

#ifndef QUEUE_H
#define QUEUE_H

namespace libhttppp {
#ifdef MSVC
	class __declspec(dllexport)  Queue : public ConnectionPool {
#else
	class __attribute__ ((visibility ("default"))) Queue : public ConnectionPool {
#endif
	public:
		Queue(ServerSocket *serversocket);
		virtual ~Queue();

		/*API Events*/
		virtual void RequestEvent(Connection *curcon);
		virtual void ResponseEvent(libhttppp::Connection *curcon);
		virtual void ConnectEvent(libhttppp::Connection *curcon);
        virtual void DisconnectEvent(Connection *curcon);

		virtual void runEventloop();
        static  void exitEventLoop(int signum);
  private:
#ifdef Windows
	static DWORD WINAPI       _WorkerThread(LPVOID WorkThreadContext);
	DWORD              _g_dwThreadCount;
	HANDLE             _g_ThreadHandles[MAX_WORKER_THREAD];
	CRITICAL_SECTION   _g_CriticalSection;
	HANDLE             _g_hIOCP;
#endif
    HTTPException       _httpexception;
    ServerSocket       *_ServerSocket;
    bool                _Eventloop;
  };
}

#endif
