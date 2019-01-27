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

#include <Windows.h>
#include <mswsock.h>
#include <Strsafe.h>
#include <config.h>

#include "../../connections.h"
#include "os/os.h"

#define EVENT_IOCP

#ifndef IOCP_H
#define IOCP_H

namespace libhttppp {
	class IOCP;
	class IOCPConnectionData : protected ConnectionData {
	protected:
		IOCPConnectionData(const char*data, size_t datasize, WSAOVERLAPPED *overlapped);
		~IOCPConnectionData();
	private:
		WSAOVERLAPPED         *_Overlapped;
		IOCPConnectionData    *_nextConnectionData;
		friend class IOCPConnection;
	};

	class IOCPConnection : public Connection {
	public:
		IOCPConnection();
		~IOCPConnection();
		IOCPConnectionData *addRecvQueue(const char data[BLOCKSIZE], size_t datasize, WSAOVERLAPPED *overlapped);
	private:
		/*Incomming Data*/
		IOCPConnectionData *_ReadDataFirst;
		IOCPConnectionData *_ReadDataLast;
	};

	class WorkerContext {
	private:
		WorkerContext();
		~WorkerContext();

		/*Linking to IOCP Events*/
		IOCP                  *_CurIOCP;
		Thread                *_CurThread;
		WorkerContext         *_nextWorkerContext;
		friend class IOCP;
	};

	class ConnectionContext {
	public:
		ConnectionContext     *nextConnectionContext();
	private:
		ConnectionContext();
		~ConnectionContext();
		/*Acceptex for iocp*/
		LPFN_ACCEPTEX           fnAcceptEx;
		/*Indefier Connection*/
		IOCPConnection         *_CurConnection;
		/*current Mutex*/
		Lock                   *_Lock;
		/*next entry*/
		ConnectionContext      *_nextConnectionContext;
		friend class IOCP;
	};

	class IOCP {
	public:
		void runEventloop();
	protected:
		IOCP(ServerSocket *serversocket);
		~IOCP();
		static BOOL WINAPI CtrlHandler(DWORD dwEvent);
		static DWORD WINAPI WorkerThread(LPVOID WorkThreadContext);
		static bool _EventEndloop;
		static bool _EventRestartloop;
	private:

		void addConnectionContext(ConnectionContext **addcon);
		void delConnectionContext(ConnectionContext *delctx, ConnectionContext **nextcxt);

		WorkerContext *addWorkerContext();
		WorkerContext *delWorkerContext(WorkerContext *delwrkctx);

		HANDLE              _IOCP;
		WSAEVENT            _hCleanupEvent[1];
		CRITICAL_SECTION    _CriticalSection;


		/*Connection Context helper*/
		ConnectionContext *_firstConnectionContext;
		ConnectionContext *_lastConnectionContext;

		/*Threadpools*/
		ThreadPool              *_WorkerPool;
		WorkerContext           *_firstWorkerContext;
		WorkerContext           *_lastWorkerContext;
		Lock                    *_Lock;

		ServerSocket            *_ServerSocket;
	};
};

#endif