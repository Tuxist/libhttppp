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
#include <cstdlib>
#include <config.h>
#include <errno.h>

#include "os/os.h"
#include "threadpool.h"

#define READEVENT 0
#define SENDEVENT 1

//#define DEBUG_MUTEX

#include "../event.h"

libhttppp::Event::Event(ServerSocket *serversocket) {
	_ServerSocket = serversocket;
	_ServerSocket->setnonblocking();
	_ServerSocket->listenSocket();
	_EventEndloop = true;
	_Cpool = new ConnectionPool(_ServerSocket);
    _WorkerPool = new ThreadPool;
	_Mutex = new Mutex;
	_firstConnectionContext = NULL;
	_lastConnectionContext = NULL;
    _firstWorkerContext=NULL;
    _lastWorkerContext=NULL;
}

libhttppp::Event::~Event() {
	WSASetEvent(_hCleanupEvent[0]);
	delete   _Cpool;
	delete   _firstConnectionContext;
    delete   _WorkerPool;
    delete   _firstWorkerContext;
  _lastWorkerContext=NULL;
	delete   _Mutex;
	_lastConnectionContext = NULL;
}



void libhttppp::Event::runEventloop() {
	int srvssocket = _ServerSocket->getSocket();
	int maxconnets = _ServerSocket->getMaxconnections();

	SYSInfo sysinfo;
	DWORD threadcount = sysinfo.getNumberOfProcessors() * 2;

	if (WSA_INVALID_EVENT == (_hCleanupEvent[0] = WSACreateEvent()))
	{
		_httpexception.Critical("WSACreateEvent() failed:", WSAGetLastError());
		throw _httpexception;
	}

	try{
		InitializeCriticalSection(&_CriticalSection);
	}catch (...){
		HTTPException hexception;
		hexception.Critical("InitializeCriticalSection raised an exception.\n");
		SetConsoleCtrlHandler(CtrlHandler, FALSE);
		if (_hCleanupEvent[0] != WSA_INVALID_EVENT) {
			WSACloseEvent(_hCleanupEvent[0]);
			_hCleanupEvent[0] = WSA_INVALID_EVENT;
		}
		throw _httpexception;
	}

	while (_EventRestartloop) {
		_EventRestartloop = true;
		_EventEndloop = true;
		WSAResetEvent(_hCleanupEvent[0]);

		_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (_IOCP == NULL) {
			_httpexception.Critical("CreateIoCompletionPort() failed to create I/O completion port:",
				GetLastError());
			throw _httpexception;
		}

		SYSInfo sysinfo;
		size_t thrs = sysinfo.getNumberOfProcessors();
		for (size_t i = 0; i < thrs; i++) {
			WorkerContext *curwrkctx = addWorkerContext();
			curwrkctx->_CurThread->Create(WorkerThread, curwrkctx);
		}

		int nRet = 0;
		DWORD dwRecvNumBytes = 0;
		DWORD bytes = 0;

#ifdef Windows
		GUID acceptex_guid = WSAID_ACCEPTEX;
#endif

		ConnectionContext *curcxt = NULL;
		addConnectionContext(&curcxt);


		_IOCP = CreateIoCompletionPort((HANDLE)_ServerSocket->getSocket(), _IOCP, (DWORD_PTR)curcxt, 0);
		if (_IOCP == NULL) {
			_httpexception.Critical("CreateIoCompletionPort() failed: %d\n", GetLastError());
			delConnectionContext(curcxt,NULL);
			throw _httpexception;
		}

		if (curcxt == NULL) {
			_httpexception.Critical("failed to update listen socket to IOCP\n");
			throw _httpexception;
		}

		// Load the AcceptEx extension function from the provider for this socket
		nRet = WSAIoctl(
			_ServerSocket->getSocket(),
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&acceptex_guid,
			sizeof(acceptex_guid),
			&curcxt->fnAcceptEx,
			sizeof(curcxt->fnAcceptEx),
			&bytes,
			NULL,
			NULL
		);

		if (nRet == SOCKET_ERROR){
			_httpexception.Critical("failed to load AcceptEx: %d\n", WSAGetLastError());
			throw _httpexception;
		}

		/*Disable Buffer in Clientsocket*/
		try {
			ClientSocket *lsock = curcxt->_CurConnection->getClientSocket();
			lsock->disableBuffer();
		}catch(HTTPException &e){
			if (e.isCritical()) {
				throw e;
			}
		}

		//
		// pay close attention to these parameters and buffer lengths
		//

		char buf[BLOCKSIZE];
		nRet = curcxt->fnAcceptEx(_ServerSocket->getSocket(), 
			curcxt->_CurConnection->getClientSocket()->getSocket(),
			(LPVOID)(buf),
			BLOCKSIZE - (2 * (sizeof(SOCKADDR_STORAGE) + 16)),
			sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
			&dwRecvNumBytes,
			(LPOVERLAPPED) &(curcxt->Overlapped)); //buggy needs impletend in event.h

		if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
			_httpexception.Critical("AcceptEx() failed: %d\n", WSAGetLastError());
			throw _httpexception;
		}

		ConnectionData *cdat = curcxt->_CurConnection->addRecvQueue(buf, dwRecvNumBytes);

		WSAWaitForMultipleEvents(1,_hCleanupEvent, TRUE, WSA_INFINITE, FALSE);
	}

	DeleteCriticalSection(&_CriticalSection);

	if (_hCleanupEvent[0] != WSA_INVALID_EVENT) {
		WSACloseEvent(_hCleanupEvent[0]);
		_hCleanupEvent[0] = WSA_INVALID_EVENT;
	}

	WSACleanup();
	SetConsoleCtrlHandler(CtrlHandler, FALSE);
}

DWORD WINAPI libhttppp::Event::WorkerThread(LPVOID WorkThreadContext) {
	HTTPException httpexepction;
	Event *curenv= (Event*)WorkThreadContext;
	ConnectionContext *curcxt = NULL;
	HANDLE hIOCP = curenv->_IOCP;
	HTTPException httpexception;
	httpexception.Note("Worker starting");
	BOOL bSuccess = FALSE;
	DWORD dwIoSize = 0;
	LPWSAOVERLAPPED lpOverlapped = NULL;
	for(;;) {
		bSuccess = GetQueuedCompletionStatus(
			hIOCP,
			&dwIoSize,
			(PDWORD_PTR)&curcxt,
			(LPOVERLAPPED *)&lpOverlapped,
			INFINITE
		);
		if (!bSuccess)
			httpexception.Error("GetQueuedCompletionStatus() failed: ",GetLastError());

		if (curcxt == NULL) {
			return(0);
		}

		if (curenv->_EventEndloop) {
			return(0);
		}
		std::cout << "test\n";
		std::cout << curcxt->_CurConnection->getRecvData()->getData();
	}
	return(0);
}

BOOL WINAPI libhttppp::Event::CtrlHandler(DWORD dwEvent) {
	switch (dwEvent) {
	case CTRL_BREAK_EVENT:
		_EventRestartloop = true;
	case CTRL_C_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
	case CTRL_CLOSE_EVENT:
		_EventEndloop = true;
		break;

	default:
		//
		// unknown type--better pass it on.
		//

		return(FALSE);
	}
	return(TRUE);
}