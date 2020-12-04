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

libhttppp::IOData::IOData() {
	_databuff = { NULL };
	_overlapped = { NULL };
	_operationType = 0;
	_connection = NULL;
}

libhttppp::ConnectionPtr::ConnectionPtr() {
	_Connection = new Connection;
	_forwardPtr = NULL;
	_backPtr = NULL;
}

libhttppp::ConnectionPtr::~ConnectionPtr() {
	delete _Connection;
	delete _forwardPtr;
}

libhttppp::IOCP::IOCP(ServerSocket* serversocket) {
	HTTPException httpexception;
	try {
		InitializeCriticalSection(&_CriticalSection);
		_hCleanupEvent[0] = 0;
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
	_ServerSocket = serversocket;
	_IOCP = NULL;
	_firstConnectionPtr = NULL;
}

libhttppp::IOCP::~IOCP() {
	delete _firstConnectionPtr;
}

const char* libhttppp::IOCP::getEventType() {
	return "IOCP";
}

void libhttppp::IOCP::initEventHandler() {
	HTTPException httpexception;
	try {
		_ServerSocket->listenSocket();
	}
	catch (HTTPException& e) {
		throw e;
	}
	_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_IOCP == NULL) {
		httpexception.Critical("CreateIoCompletionPort() failed to create I/O completion port:",
			GetLastError());
		throw httpexception;
	}

	if (WSA_INVALID_EVENT == (_hCleanupEvent[0] = WSACreateEvent())) {
		httpexception.Critical("WSACreateEvent() failed:", WSAGetLastError());
		throw httpexception;
	}

	if (_IOCP == NULL) {
		httpexception.Critical("createiocompletionport() failed:", GetLastError());
		throw httpexception;
	}
}

void libhttppp::IOCP::initWorker() {
}

int libhttppp::IOCP::waitEventHandler() {
	return 0;
}

void libhttppp::IOCP::ConnectEventHandler(int des) {
	GUID acceptex_guid = WSAID_ACCEPTEX;
	HTTPException httpexception;
	char* buf = new char[BLOCKSIZE];
	DWORD dwrecvnumbytes = 0;
	DWORD bytes = 0;
	LPFN_ACCEPTEX lpfnAcceptEx;
	ConnectionPtr conptr;
	WSAOVERLAPPED wsaover = { NULL };

	int nRet = WSAIoctl(
		_ServerSocket->getSocket(),
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&acceptex_guid,
		sizeof(acceptex_guid),
		&lpfnAcceptEx,
		sizeof(lpfnAcceptEx),
		&bytes,
		NULL,
		NULL
	);

	nRet = lpfnAcceptEx(_ServerSocket->getSocket(),
		conptr._Connection->getClientSocket()->Socket,
		(LPVOID)(buf),
		BLOCKSIZE - (2 * (sizeof(SOCKADDR_STORAGE) + 16)),
		sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
		&dwrecvnumbytes,
		(LPOVERLAPPED)&wsaover);


	if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
		httpexception.Critical("acceptex() failed: %d\n", WSAGetLastError());
		throw httpexception;
	}

	if (nRet == SOCKET_ERROR) {
		httpexception.Critical("failed to load acceptex: %d\n", WSAGetLastError());
		throw httpexception;
	}

	WSAWaitForMultipleEvents(1, _hCleanupEvent, TRUE, WSA_INFINITE, FALSE);

	int asock = _ServerSocket->acceptEvent(conptr._Connection->getClientSocket());
	if (SOCKET_ERROR == asock) {
		std::cerr << "Accept Socket Error: " << GetLastError() << std::endl;
	}
	else {
		std::cerr << "Connected" << std::endl;
	}

	try {
		ClientSocket* lsock = conptr._Connection->getClientSocket();
		lsock->disableBuffer();
	}
	catch (HTTPException& e) {
		if (e.isCritical()) {
			throw e;
		}
	}

	_IOCP = CreateIoCompletionPort((HANDLE)asock, _IOCP, (DWORD_PTR)&conptr, 0);

	delete[] buf;
}

int libhttppp::IOCP::StatusEventHandler(int des) {
	return 1;
}

void libhttppp::IOCP::ReadEventHandler(int des) {
	DWORD recvd = 0;
	ConnectionPtr perHandlePtr;
	IOData iodata;
	//_ServerSocket->recvWSAData(_ConnectionPtr[des]._Connection->getClientSocket(), & (iodata._databuff), BLOCKSIZE, 0, & recvd, & (iodata._overlapped), NULL);
	
}

void libhttppp::IOCP::WriteEventHandler(int des) {

}

void libhttppp::IOCP::CloseEventHandler(int des) {

}


bool libhttppp::IOCP::LockConnection(int des) {
	//return _ConnectionLock.
	return 0;
}

void libhttppp::IOCP::UnlockConnction(int des)
{
}

/*API Events*/

void libhttppp::IOCP::RequestEvent(Connection* curcon) {
	return;
};

void libhttppp::IOCP::ResponseEvent(Connection* curcon) {
	return;
};

void libhttppp::IOCP::ConnectEvent(Connection* curcon) {
	return;
};

void libhttppp::IOCP::DisconnectEvent(Connection* curcon) {
	return;
};