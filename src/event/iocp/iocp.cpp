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
#include <errno.h>

#ifdef  Windows
#include <Windows.h>
#include <winsock2.h>
#include <mswsock.h>
#endif //  Windows


#include "config.h"
#include "os/os.h"
#include "../../threadpool.h"
#include "iocp.h"

#define READEVENT 0
#define SENDEVENT 1

libhttppp::IOCP::IOCP(ServerSocket *serversocket) {
	_ServerSocket = serversocket;
	_ServerSocket->setnonblocking();
	_ServerSocket->listenSocket();
	_IOCPCon = new IOCPConnection();
}

libhttppp::IOCP::~IOCP() {
	delete _IOCPCon;
}

const char * libhttppp::IOCP::getEventType() {
	return "IOCP";
}

void libhttppp::IOCP::initEventHandler() {
	HTTPException httpexception;
	_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_IOCP == NULL) {
		httpexception.Critical("CreateIoCompletionPort() failed to create I/O completion port:",GetLastError());
		throw httpexception;
	}

	if (WSA_INVALID_EVENT == (_hCleanupEvent[0] = WSACreateEvent())){
		httpexception.Critical("WSACreateEvent() failed:", WSAGetLastError());
		throw httpexception;
	}
	
	try {
		InitializeCriticalSection(&_CriticalSection);
	}
	catch (...) {
		HTTPException hexception;
		hexception.Critical("InitializeCriticalSection raised an exception.\n");
		if (_hCleanupEvent[0] != WSA_INVALID_EVENT) {
			WSACloseEvent(_hCleanupEvent[0]);
			_hCleanupEvent[0] = WSA_INVALID_EVENT;
		}
		throw httpexception;
	}

	_IOCP = CreateIoCompletionPort((HANDLE)_ServerSocket->getSocket(), _IOCP, (DWORD_PTR)_IOCPCon, 0);
	if (_IOCP == NULL) {
		httpexception.Critical("createiocompletionport() failed:", GetLastError());
		delete _IOCPCon;
		_IOCPCon = NULL;
		throw httpexception;
	}
 
	if (_IOCPCon == NULL) {
		httpexception.Critical("failed to update listen socket to iocp\n");
		throw httpexception;
	}
}

int libhttppp::IOCP::waitEventHandler() {
	HTTPException httpexception;
	int nRet = 0;
	DWORD bytes = 0;
	DWORD dwrecvnumbytes = 0;
	GUID acceptex_guid = WSAID_ACCEPTEX;

	// load the acceptex extension function from the provider for this socket
	nRet = WSAIoctl(
		_ServerSocket->getSocket(),
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&acceptex_guid,
		sizeof(acceptex_guid),
		&_IOCPCon->fnAcceptEx,
		sizeof(_IOCPCon->fnAcceptEx),
		&bytes,
		NULL,
		NULL
	);
 
	if (nRet == SOCKET_ERROR) {
		httpexception.Critical("failed to load acceptex: %d\n", WSAGetLastError());
		throw httpexception;
	}

	/*disable buffer in clientsocket*/
	try {
		ClientSocket *lsock = _IOCPCon->getClientSocket();
		lsock->disableBuffer();
	}catch (HTTPException &e) {
		if (e.isCritical()) {
			throw e;
		}
	}
	//
	// pay close attention to these parameters and buffer lengths
	//

	char buf[BLOCKSIZE];
	WSAOVERLAPPED *wsaover = new WSAOVERLAPPED;
	nRet = _IOCPCon->fnAcceptEx(_ServerSocket->getSocket(),
		_IOCPCon->getClientSocket()->getSocket(),
		(LPVOID)(buf),
		BLOCKSIZE - (2 * (sizeof(SOCKADDR_STORAGE) + 16)),
		sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
		&dwrecvnumbytes,
		(LPOVERLAPPED)wsaover);
 
	if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
		httpexception.Critical("acceptex() failed: %d\n", WSAGetLastError());
		throw httpexception;
	}
	
	WSAWaitForMultipleEvents(1, _hCleanupEvent, TRUE, WSA_INFINITE, FALSE);
	return nRet;
}

/*API Events*/

void libhttppp::IOCP::RequestEvent(Connection *curcon) {
	return;
};

void libhttppp::IOCP::ResponseEvent(Connection *curcon) {
	return;
};

void libhttppp::IOCP::ConnectEvent(Connection *curcon) {
	return;
};

void libhttppp::IOCP::DisconnectEvent(Connection *curcon) {
	return;
};


//#define DEBUG_MUTEX


// 
// libhttppp::IOCPConnectionData::IOCPConnectionData(const char * data, size_t datasize, WSAOVERLAPPED * overlapped) 
// 	                                                                      : ConnectionData(data, datasize) {
// 	_Overlapped = overlapped;
// }
// 
// libhttppp::IOCPConnectionData::~IOCPConnectionData(){
// 	delete _Overlapped;
// }
// 
// libhttppp::IOCPConnection::IOCPConnection() : Connection(){
// 	_ReadDataFirst=NULL;
// 	_ReadDataLast=NULL;
// }
// 
// 
// libhttppp::IOCPConnection::~IOCPConnection(){
// 	delete _ReadDataFirst;
// 	delete _ReadDataLast;
// }
// 
// libhttppp::IOCPConnectionData *libhttppp::IOCPConnection::addRecvQueue(const char data[BLOCKSIZE],
//                                                                        size_t datasize, WSAOVERLAPPED * overlapped){
// 	if (!_ReadDataFirst) {
// 		_ReadDataFirst = new IOCPConnectionData(data,datasize, overlapped);
// 		_ReadDataLast = _ReadDataFirst;
// 	}
// 	else {
// 		_ReadDataLast->_nextConnectionData = new IOCPConnectionData(data,datasize, overlapped);
// 		_ReadDataLast = _ReadDataLast->_nextConnectionData;
// 	}
// 	_ReadDataSize += datasize;
// 	return _ReadDataLast;
// }
// libhttppp::ConnectionContext::ConnectionContext() {
// 	_Lock = new Lock;
// 	_CurConnection = NULL;
// }
// 
// libhttppp::ConnectionContext::~ConnectionContext() {
// 	delete _Lock;
// 	delete _CurConnection;
// }
// 
// 
// libhttppp::ConnectionContext * libhttppp::ConnectionContext::nextConnectionContext() {
// 	return _nextConnectionContext;
// }
// 
// libhttppp::WorkerContext::WorkerContext() {
// 	_CurIOCP = NULL;
// 	_CurThread = NULL;
// 	_nextWorkerContext = NULL;
// }
// 
// libhttppp::WorkerContext::~WorkerContext() {
// 	delete _nextWorkerContext;
// }
// 
// libhttppp::WorkerContext *libhttppp::IOCP::addWorkerContext() {
// 	if (_firstWorkerContext) {
// 		_lastWorkerContext->_nextWorkerContext = new WorkerContext;
// 		_lastWorkerContext = _lastWorkerContext->_nextWorkerContext;
// 	}
// 	else {
// 		_firstWorkerContext = new WorkerContext;
// 		_lastWorkerContext = _firstWorkerContext;
// 	}
// 	_lastWorkerContext->_CurIOCP = this;
// 	_lastWorkerContext->_CurThread = _WorkerPool->addThread();
// 	return _lastWorkerContext;
// }
// 
// libhttppp::WorkerContext *libhttppp::IOCP::delWorkerContext(
// 	libhttppp::WorkerContext *delwrkctx) {
// 	WorkerContext *prevwrk = NULL;
// 	for (WorkerContext *curwrk = _firstWorkerContext; curwrk; curwrk = curwrk->_nextWorkerContext) {
// 		if (curwrk == delwrkctx) {
// 			if (prevwrk) {
// 				prevwrk->_nextWorkerContext = curwrk->_nextWorkerContext;
// 			}
// 			if (curwrk == _firstWorkerContext) {
// 				_firstWorkerContext = curwrk->_nextWorkerContext;
// 			}
// 			if (curwrk == _lastWorkerContext) {
// 				_lastWorkerContext = prevwrk;
// 			}
// 			curwrk->_nextWorkerContext = NULL;
// 			delete curwrk;
// 		}
// 		prevwrk = curwrk;
// 	}
// 	if (prevwrk)
// 		return prevwrk->_nextWorkerContext;
// 	else
// 		return _firstWorkerContext;
// }
// 
// void libhttppp::IOCP::addConnectionContext(libhttppp::ConnectionContext **addcon) {
// 	_Lock->lock();
// 	HTTPException httpexception;
// 	if (!addcon)
// 		return;
// 	if (_firstConnectionContext) {
// 		libhttppp::ConnectionContext *prevcon = _lastConnectionContext;;
// 		prevcon->_nextConnectionContext = new ConnectionContext;
// 		_lastConnectionContext = prevcon->_nextConnectionContext;
// 	}
// 	else {
// 		_firstConnectionContext = new ConnectionContext;
// 		_lastConnectionContext = _firstConnectionContext;
// 	}
// 	_lastConnectionContext->_CurConnection = new IOCPConnection;
// 	*addcon = _lastConnectionContext;
// 	_Lock->unlock();
// }
// 
// 
// void libhttppp::IOCP::delConnectionContext(libhttppp::ConnectionContext *delctx,
// 	libhttppp::ConnectionContext **nextcxt) {
// 	_Lock->lock();
// 	HTTPException httpexception;
// 	ConnectionContext *prevcontext = NULL;
// 	for (ConnectionContext *curcontext = _firstConnectionContext; curcontext;
// 		curcontext = curcontext->nextConnectionContext()) {
// 		if (curcontext == delctx) {
// 			if (prevcontext) {
// 				prevcontext->_nextConnectionContext = curcontext->_nextConnectionContext;
// 			}
// 			if (_firstConnectionContext == curcontext) {
// 				if (_lastConnectionContext == _firstConnectionContext) {
// 					_firstConnectionContext = _firstConnectionContext->nextConnectionContext();
// 					_lastConnectionContext = _firstConnectionContext;
// 				}
// 				else {
// 					_firstConnectionContext = _firstConnectionContext->nextConnectionContext();
// 				}
// 			}
// 			if (_lastConnectionContext == delctx) {
// 				if (prevcontext) {
// 					_lastConnectionContext = prevcontext;
// 				}
// 				else {
// 					_lastConnectionContext = _firstConnectionContext;
// 				}
// 			}
// 			curcontext->_nextConnectionContext = NULL;
// 			delete curcontext;
// 			break;
// 		}
// 		prevcontext = curcontext;
// 	}
// 	if (nextcxt) {
// 		if (prevcontext && prevcontext->_nextConnectionContext) {
// 			*nextcxt = prevcontext->_nextConnectionContext;
// 		}
// 		else {
// 			*nextcxt = _firstConnectionContext;
// 		}
// 	}
// 	_Lock->unlock();
// }
// 
// libhttppp::IOCP::IOCP(ServerSocket *serversocket) {
// 	_ServerSocket = serversocket;
// 	_ServerSocket->setnonblocking();
// 	_ServerSocket->listenSocket();
// 	_EventEndloop = true;
// 	_WorkerPool = new ThreadPool;
// 	_Lock = new Lock;
// 	_firstConnectionContext = NULL;
// 	_lastConnectionContext = NULL;
// 	_firstWorkerContext = NULL;
// 	_lastWorkerContext = NULL;
// }
// 
// libhttppp::IOCP::~IOCP() {
// 	WSASetEvent(_hCleanupEvent[0]);
// 	delete   _firstConnectionContext;
// 	delete   _WorkerPool;
// 	delete   _firstWorkerContext;
// 	_lastWorkerContext = NULL;
// 	delete   _Lock;
// 	_lastConnectionContext = NULL;
// }
// 
// void libhttppp::IOCP::runEventloop() {

// 
// 	while (_EventRestartloop) {
// 		_EventRestartloop = true;
// 		_EventEndloop = true;
// 		WSAResetEvent(_hCleanupEvent[0]);
// 
// 		_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
// 		if (_IOCP == NULL) {
// 			httpexception.Critical("CreateIoCompletionPort() failed to create I/O completion port:",
// 				GetLastError());
// 			throw httpexception;
// 		}
// 
// 		SYSInfo sysinfo;
// 		size_t thrs = sysinfo.getNumberOfProcessors();
// 		for (size_t i = 0; i < thrs; i++) {
// 			WorkerContext *curwrkctx = addWorkerContext();
// 			curwrkctx->_CurThread->Create(WorkerThread, curwrkctx);
// 		}
// 
// 		int nRet = 0;
// 		DWORD dwRecvNumBytes = 0;
// 		DWORD bytes = 0;
// 
// 		GUID acceptex_guid = WSAID_ACCEPTEX;
// 
// 		ConnectionContext *curcxt = NULL;
// 		addConnectionContext(&curcxt);
// 
// 
// 		_IOCP = CreateIoCompletionPort(srvssocket, _IOCP, (DWORD_PTR)curcxt, 0);
// 		if (_IOCP == NULL) {
// 			httpexception.Critical("CreateIoCompletionPort() failed: %d\n", GetLastError());
// 			delConnectionContext(curcxt, NULL);
// 			throw httpexception;
// 		}
// 
// 		if (curcxt == NULL) {
// 			httpexception.Critical("failed to update listen socket to IOCP\n");
// 			throw httpexception;
// 		}
// 
// 		// Load the AcceptEx extension function from the provider for this socket
// 		nRet = WSAIoctl(
// 			_ServerSocket->getSocket(),
// 			SIO_GET_EXTENSION_FUNCTION_POINTER,
// 			&acceptex_guid,
// 			sizeof(acceptex_guid),
// 			&curcxt->fnAcceptEx,
// 			sizeof(curcxt->fnAcceptEx),
// 			&bytes,
// 			NULL,
// 			NULL
// 		);
// 
// 		if (nRet == SOCKET_ERROR) {
// 			httpexception.Critical("failed to load AcceptEx: %d\n", WSAGetLastError());
// 			throw httpexception;
// 		}
// 
// 		/*Disable Buffer in Clientsocket*/
// 		try {
// 			ClientSocket *lsock = curcxt->_CurConnection->getClientSocket();
// 			lsock->disableBuffer();
// 		}
// 		catch (HTTPException &e) {
// 			if (e.isCritical()) {
// 				throw e;
// 			}
// 		}
// 
// 		//
// 		// pay close attention to these parameters and buffer lengths
// 		//
// 
// 		char buf[BLOCKSIZE];
// 		WSAOVERLAPPED *wsaover = new WSAOVERLAPPED;
// 		nRet = curcxt->fnAcceptEx(_ServerSocket->getSocket(),
// 			curcxt->_CurConnection->getClientSocket()->getSocket(),
// 			(LPVOID)(buf),
// 			BLOCKSIZE - (2 * (sizeof(SOCKADDR_STORAGE) + 16)),
// 			sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
// 			&dwRecvNumBytes,
// 			(LPOVERLAPPED)wsaover);
// 
// 		if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
// 			httpexception.Critical("AcceptEx() failed: %d\n", WSAGetLastError());
// 			throw httpexception;
// 		}
// 
// 		IOCPConnectionData *cdat;
// 		cdat = curcxt->_CurConnection->addRecvQueue(buf, dwRecvNumBytes, wsaover);
// 
// 		WSAWaitForMultipleEvents(1, _hCleanupEvent, TRUE, WSA_INFINITE, FALSE);
// 	}
// 
// 	DeleteCriticalSection(&_CriticalSection);
// 
// 	if (_hCleanupEvent[0] != WSA_INVALID_EVENT) {
// 		WSACloseEvent(_hCleanupEvent[0]);
// 		_hCleanupEvent[0] = WSA_INVALID_EVENT;
// 	}
// 
// 	WSACleanup();
// 	
// }
// 
// DWORD WINAPI libhttppp::IOCP::WorkerThread(LPVOID WorkThreadContext) {
// 	HTTPException httpexepction;
// 	WorkerContext *workerctx = (WorkerContext*)WorkThreadContext;
// 	IOCP *curiocp = workerctx->_CurIOCP;
// 	ConnectionContext *curcxt = NULL;
// 	HANDLE hIOCP = curiocp->_IOCP;
// 	BOOL bSuccess = FALSE;
// 	DWORD dwIoSize = 0;
// 	LPWSAOVERLAPPED lpOverlapped = NULL;
// 	HTTPException httpexception;
// 	httpexception.Note("Worker starting");
// 	for (;;) {
// 		bSuccess = GetQueuedCompletionStatus(
// 			hIOCP,
// 			&dwIoSize,
// 			(PDWORD_PTR)&curcxt,
// 			(LPOVERLAPPED *)&lpOverlapped,
// 			INFINITE
// 		);
// 		if (!bSuccess)
// 			httpexception.Error("GetQueuedCompletionStatus() failed: ", GetLastError());
// 
// 		if (curcxt == NULL) {
// 			return(0);
// 		}
// 
// 		if (curiocp->_EventEndloop) {
// 			return(0);
// 		}
// 		std::cout << "test\n";
// 		std::cout << curcxt->_CurConnection->getRecvData()->getData();
// 	}
// 	return(0);
// }
//
