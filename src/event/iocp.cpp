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

#include <fcntl.h>
#include <cstdlib>
#include <config.h>
#include <errno.h>
#include "os/os.h"


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
	_Mutex = new Mutex;
	_firstConnectionContext = NULL;
	_lastConnectionContext = NULL;
}

libhttppp::Event::~Event() {
#ifdef DEBUG_MUTEX
	_httpexception.Note("~Event", "Lock MainMutex");
#endif
	_Mutex->lock();
	delete   _Cpool;
	delete   _firstConnectionContext;
	delete   _Mutex;
	_lastConnectionContext = NULL;
}

libhttppp::Event* _EventIns = NULL;

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

	WSADATA wsaData;
	int nRet = 0;

	if ((nRet = WSAStartup(0x202, &wsaData)) != 0) {
		_httpexception.Critical("WSAStartup() failed: %d\n", nRet);
		SetConsoleCtrlHandler(CtrlHandler, FALSE);
		if (_hCleanupEvent[0] != WSA_INVALID_EVENT) {
			WSACloseEvent(_hCleanupEvent[0]);
			_hCleanupEvent[0] = WSA_INVALID_EVENT;
		}
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
		_EventIns = this;
		_EventRestartloop = true;
		_EventEndloop = true;
		WSAResetEvent(_hCleanupEvent[0]);

		_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (_IOCP == NULL) {
			_httpexception.Critical("CreateIoCompletionPort() failed to create I/O completion port:",
				GetLastError());
			throw _httpexception;
		}

		for (DWORD dwCPU = 0; dwCPU < threadcount; dwCPU++) {
			Thread  hThread;
			hThread.Create(WorkerThread, this);
//			g_ThreadHandles[dwCPU] = hThread.getHandle();
		}

		/*
		int n = epoll_wait(_epollFD, _Events, maxconnets, EPOLLWAIT);
		if (n < 0) {
			if (errno == EINTR) {
				continue;
			}
			else {
				_httpexception.Critical("epoll wait failure");
				throw _httpexception;
			}

		}
		for (int i = 0; i < n; i++) {
			ConnectionContext *curct = NULL;
			if (_Events[i].data.fd == srvssocket) {
				try {
#ifdef DEBUG_MUTEX
					_httpexception.Note("runeventloop", "Lock MainMutex");
#endif
					_Mutex->lock();
					curct = addConnection();
#ifdef DEBUG_MUTEX
					_httpexception.Note("runeventloop", "Unlock MainMutex");
#endif
					_Mutex->unlock();
#ifdef DEBUG_MUTEX
					_httpexception.Note("runeventloop", "Lock ConnectionMutex");
#endif
					curct->_Mutex->lock();
					ClientSocket *clientsocket = curct->_CurConnection->getClientSocket();
					int fd = _ServerSocket->acceptEvent(clientsocket);
					clientsocket->setnonblocking();
					if (fd > 0) {
						_setEvent.data.ptr = (void*)curct;
						_setEvent.events = EPOLLIN | EPOLLET;
						if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, fd, &_setEvent) == -1 && errno == EEXIST)
							epoll_ctl(_epollFD, EPOLL_CTL_MOD, fd, &_setEvent);
						ConnectEvent(curct->_CurConnection);
#ifdef DEBUG_MUTEX
						_httpexception.Note("runeventloop", "Unlock ConnectionMutex");
#endif
						curct->_Mutex->unlock();
					}
					else {
#ifdef DEBUG_MUTEX
						_httpexception.Note("runeventloop", "Unlock ConnectionMutex");
#endif
						curct->_Mutex->unlock();
#ifdef DEBUG_MUTEX
						_httpexception.Note("runeventloop", "Lock MainMutex");
#endif
						_Mutex->lock();
						delConnection(curct->_CurConnection);
#ifdef DEBUG_MUTEX
						_httpexception.Note("runeventloop", "Unlock MainMutex");
#endif
						_Mutex->unlock();
					}

				}
				catch (HTTPException &e) {
#ifdef DEBUG_MUTEX
					_httpexception.Note("runeventloop", "Lock MainMutex");
#endif
					_Mutex->lock();
#ifdef DEBUG_MUTEX
					_httpexception.Note("runeventloop", "Unlock ConnectionMutex");
#endif
					curct->_Mutex->unlock();
					delConnection(curct->_CurConnection);
#ifdef DEBUG_MUTEX
					_httpexception.Note("runeventloop", "Unlock MainMutex");
#endif
					_Mutex->unlock();
					if (e.isCritical())
						throw e;
				}
			}
			else {
				curct = (ConnectionContext*)_Events[i].data.ptr;
				if (_Events[i].events & EPOLLIN) {
					Thread(ReadEvent, curct);
				}
				else {
					CloseEvent(curct);
				}
			}
		}*/
		
	}
}

DWORD WINAPI libhttppp::Event::WorkerThread(LPVOID WorkThreadContext) {
	HTTPException httpexepction;
	Event *curenv= (Event*)WorkThreadContext;
	HANDLE hIOCP = curenv->_IOCP;
	BOOL bSuccess = FALSE;
	int nRet = 0;
	LPWSAOVERLAPPED lpOverlapped = NULL;
	WSABUF buffRecv;
	WSABUF buffSend;
	DWORD dwRecvNumBytes = 0;
	DWORD dwSendNumBytes = 0;
	DWORD dwFlags = 0;
	DWORD dwIoSize = 0;
	HRESULT hRet;

	while (TRUE) {

		//
		// continually loop to service io completion packets
		//
		bSuccess = GetQueuedCompletionStatus(
			hIOCP,
			&dwIoSize,
			(PDWORD_PTR)&lpPerSocketContext,
			(LPOVERLAPPED *)&lpOverlapped,
			INFINITE
		);
		if (!bSuccess)
			httpexepction.Error("GetQueuedCompletionStatus() failed:",(const char*)GetLastError());

		if (curenv->_EventEndloop) {
			return(0);
		}

		lpIOContext = (PPER_IO_CONTEXT)lpOverlapped;

		//
		//We should never skip the loop and not post another AcceptEx if the current
		//completion packet is for previous AcceptEx
		//
		if (lpIOContext->IOOperation != ClientIoAccept) {
			if (!bSuccess || (bSuccess && (0 == dwIoSize))) {

				//
				// client connection dropped, continue to service remaining (and possibly 
				// new) client connections
				//
				CloseClient(lpPerSocketContext, FALSE);
				continue;
			}
		}

		//
		// determine what type of IO packet has completed by checking the PER_IO_CONTEXT 
		// associated with this socket.  This will determine what action to take.
		//
		switch (lpIOContext->IOOperation) {
		case ClientIoAccept:

			//
			// When the AcceptEx function returns, the socket sAcceptSocket is 
			// in the default state for a connected socket. The socket sAcceptSocket 
			// does not inherit the properties of the socket associated with 
			// sListenSocket parameter until SO_UPDATE_ACCEPT_CONTEXT is set on 
			// the socket. Use the setsockopt function to set the SO_UPDATE_ACCEPT_CONTEXT 
			// option, specifying sAcceptSocket as the socket handle and sListenSocket 
			// as the option value. 
			//

			SOCKET sock = curenv->_ServerSocket->getSocket();

			nRet = setsockopt(
				lpPerSocketContext->pIOContext->SocketAccept,
				SOL_SOCKET,
				SO_UPDATE_ACCEPT_CONTEXT,
				(char *)&sock,
				sizeof(sock)
			);

			if (nRet == SOCKET_ERROR) {

				//
				//just warn user here.
				//
				myprintf("setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed to update accept socket\n");
				WSASetEvent(g_hCleanupEvent[0]);
				return(0);
			}

			lpAcceptSocketContext = UpdateCompletionPort(
				lpPerSocketContext->pIOContext->SocketAccept,
				ClientIoAccept, TRUE);

			if (lpAcceptSocketContext == NULL) {

				//
				//just warn user here.
				//
				myprintf("failed to update accept socket to IOCP\n");
				WSASetEvent(g_hCleanupEvent[0]);
				return(0);
			}

			if (dwIoSize) {
				lpAcceptSocketContext->pIOContext->IOOperation = ClientIoWrite;
				lpAcceptSocketContext->pIOContext->nTotalBytes = dwIoSize;
				lpAcceptSocketContext->pIOContext->nSentBytes = 0;
				lpAcceptSocketContext->pIOContext->wsabuf.len = dwIoSize;
				hRet = StringCbCopyN(lpAcceptSocketContext->pIOContext->Buffer,
					MAX_BUFF_SIZE,
					lpPerSocketContext->pIOContext->Buffer,
					sizeof(lpPerSocketContext->pIOContext->Buffer)
				);
				lpAcceptSocketContext->pIOContext->wsabuf.buf = lpAcceptSocketContext->pIOContext->Buffer;

				nRet = WSASend(
					lpPerSocketContext->pIOContext->SocketAccept,
					&lpAcceptSocketContext->pIOContext->wsabuf, 1,
					&dwSendNumBytes,
					0,
					&(lpAcceptSocketContext->pIOContext->Overlapped), NULL);

				if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
					myprintf("WSASend() failed: %d\n", WSAGetLastError());
					CloseClient(lpAcceptSocketContext, FALSE);
				}
				else if (g_bVerbose) {
					myprintf("WorkerThread %d: Socket(%d) AcceptEx completed (%d bytes), Send posted\n",
						GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize);
				}
			}
			else {

				//
				// AcceptEx completes but doesn't read any data so we need to post
				// an outstanding overlapped read.
				//
				lpAcceptSocketContext->pIOContext->IOOperation = ClientIoRead;
				dwRecvNumBytes = 0;
				dwFlags = 0;
				buffRecv.buf = lpAcceptSocketContext->pIOContext->Buffer,
					buffRecv.len = MAX_BUFF_SIZE;
				nRet = WSARecv(
					lpAcceptSocketContext->Socket,
					&buffRecv, 1,
					&dwRecvNumBytes,
					&dwFlags,
					&lpAcceptSocketContext->pIOContext->Overlapped, NULL);
				if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
					myprintf("WSARecv() failed: %d\n", WSAGetLastError());
					CloseClient(lpAcceptSocketContext, FALSE);
				}
			}

			//
			//Time to post another outstanding AcceptEx
			//
			if (!CreateAcceptSocket(FALSE)) {
				myprintf("Please shut down and reboot the server.\n");
				WSASetEvent(curenv->_hCleanupEvent[0]);
				return(0);
			}
			break;
		} //switch
	} //while
	return(0);
}

BOOL WINAPI libhttppp::Event::CtrlHandler(DWORD dwEvent) {
	switch (dwEvent) {
	case CTRL_BREAK_EVENT:
		_EventIns->_EventRestartloop = true;
	case CTRL_C_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
	case CTRL_CLOSE_EVENT:
		_EventIns->_EventEndloop = true;
		WSASetEvent(_EventIns->_hCleanupEvent[0]);
		break;

	default:
		//
		// unknown type--better pass it on.
		//

		return(FALSE);
	}
	return(TRUE);
}

libhttppp::Event::ConnectionContext::ConnectionContext() {
	_CurConnection = NULL;
	_CurCPool = NULL;
	_CurEvent = NULL;
	_Mutex = new Mutex;
	_nextConnectionContext = NULL;
}

libhttppp::Event::ConnectionContext::~ConnectionContext() {
	delete _Mutex;
	delete _nextConnectionContext;
}


libhttppp::Event::ConnectionContext * libhttppp::Event::ConnectionContext::nextConnectionContext() {
	return _nextConnectionContext;
}


libhttppp::Event::ConnectionContext * libhttppp::Event::addConnection() {
	if (!_firstConnectionContext) {
		_firstConnectionContext = new ConnectionContext();
#ifdef DEBUG_MUTEX
		_httpexception.Note("addConnection", "Lock ConnectionMutex");
#endif
		_firstConnectionContext->_Mutex->lock();
		_lastConnectionContext = _firstConnectionContext;
#ifdef DEBUG_MUTEX
		_httpexception.Note("addConnection", "Unlock ConnectionMutex");
#endif
		_firstConnectionContext->_Mutex->unlock();
	}
	else {
#ifdef DEBUG_MUTEX
		_httpexception.Note("addConnection", "Lock ConnectionMutex");
#endif
		ConnectionContext *prevcon = _lastConnectionContext;
		prevcon->_Mutex->lock();
		_lastConnectionContext->_nextConnectionContext = new ConnectionContext();
		_lastConnectionContext = _lastConnectionContext->_nextConnectionContext;
#ifdef DEBUG_MUTEX
		_httpexception.Note("addConnection", "Unlock ConnectionMutex");
#endif
		prevcon->_Mutex->unlock();
	}
	_lastConnectionContext->_CurConnection = _Cpool->addConnection();
	_lastConnectionContext->_CurCPool = _Cpool;
	_lastConnectionContext->_CurEvent = this;

	return _lastConnectionContext;
}

libhttppp::Event::ConnectionContext * libhttppp::Event::delConnection(libhttppp::Connection* delcon) {
	ConnectionContext *prevcontext = NULL;
#ifdef DEBUG_MUTEX
	_httpexception.Note("delConnection", "Lock MainMutex");
#endif
	_Mutex->lock();
	for (ConnectionContext *curcontext = _firstConnectionContext; curcontext;
		curcontext = curcontext->nextConnectionContext()) {
		if (curcontext->_CurConnection == delcon) {
#ifdef DEBUG_MUTEX
			_httpexception.Note("delConnection", "Lock ConnectionMutex");
#endif
			curcontext->_Mutex->lock();
			_Cpool->delConnection(delcon);
			if (prevcontext) {
#ifdef DEBUG_MUTEX
				_httpexception.Note("delConnection", "Lock prevConnectionMutex");
#endif
				prevcontext->_Mutex->lock();
				prevcontext->_nextConnectionContext = curcontext->_nextConnectionContext;
				if (_lastConnectionContext == curcontext) {
					_lastConnectionContext = prevcontext;
				}
#ifdef DEBUG_MUTEX
				_httpexception.Note("delConnection", "unlock prevConnectionMutex");
#endif
				prevcontext->_Mutex->unlock();
			}
			else {
#ifdef DEBUG_MUTEX
				_httpexception.Note("delConnection", "lock firstConnectionMutex");
#endif
				_firstConnectionContext->_Mutex->lock();
#ifdef DEBUG_MUTEX
				_httpexception.Note("delConnection", "lock lastConnectionMutex");
#endif
				_lastConnectionContext->_Mutex->lock();
				_firstConnectionContext = curcontext->_nextConnectionContext;
				if (_lastConnectionContext->_CurConnection == delcon)
					_lastConnectionContext = _firstConnectionContext;
				if (_firstConnectionContext) {
#ifdef DEBUG_MUTEX
					_httpexception.Note("delConnection", "unlock firstConnectionMutex");
#endif
					_firstConnectionContext->_Mutex->unlock();
				}
				if (_lastConnectionContext) {
#ifdef DEBUG_MUTEX
					_httpexception.Note("delConnection", "unlock lastConnectionMutex");
#endif
					_lastConnectionContext->_Mutex->unlock();
				}
			}
			curcontext->_nextConnectionContext = NULL;
			delete curcontext;
			break;
		}
		prevcontext = curcontext;
	}
#ifdef DEBUG_MUTEX
	_httpexception.Note("delConnection", "unlock MainMutex");
#endif
	_Mutex->unlock();
	if (prevcontext && prevcontext->_nextConnectionContext) {
		return prevcontext->_nextConnectionContext;
	}
	else {
		ConnectionContext *fcontext = _firstConnectionContext;
		return fcontext;
	}
}



/*Workers*/
void *libhttppp::Event::ReadEvent(void *curcon) {
	ConnectionContext *ccon = (ConnectionContext*)curcon;
	Event *eventins = ccon->_CurEvent;
#ifdef DEBUG_MUTEX
	ccon->_httpexception.Note("ReadEvent", "lock ConnectionMutex");
#endif  
	ccon->_Mutex->lock();
	Connection *con = (Connection*)ccon->_CurConnection;
	try {
		char buf[BLOCKSIZE];
		int rcvsize = 0;
		do {
			rcvsize = eventins->_ServerSocket->recvData(con->getClientSocket(), buf, BLOCKSIZE);
			if (rcvsize > 0)
				con->addRecvQueue(buf, rcvsize);
		} while (rcvsize > 0);
		eventins->RequestEvent(con);
#ifdef DEBUG_MUTEX
		ccon->_httpexception.Note("ReadEvent", "unlock ConnectionMutex");
#endif 
		ccon->_Mutex->unlock();
		WriteEvent(ccon);
	}
	catch (HTTPException &e) {
#ifdef DEBUG_MUTEX
		ccon->_httpexception.Note("ReadEvent", "unlock ConnectionMutex");
#endif 
		ccon->_Mutex->unlock();
		if (e.isCritical()) {
			throw e;
		}
		if (e.isError()) {
			con->cleanRecvData();
			CloseEvent(ccon);
		}
	}
	return NULL;
}

void *libhttppp::Event::WriteEvent(void* curcon) {
	ConnectionContext *ccon = (ConnectionContext*)curcon;
	Event *eventins = ccon->_CurEvent;
#ifdef DEBUG_MUTEX
	ccon->_httpexception.Note("WriteEvent", "lock ConnectionMutex");
#endif
	ccon->_Mutex->lock();
	Connection *con = (Connection*)ccon->_CurConnection;
	try {
		ssize_t sended = 0;
		while (con->getSendData()) {
			sended = eventins->_ServerSocket->sendData(con->getClientSocket(),
				(void*)con->getSendData()->getData(),
				con->getSendData()->getDataSize());
			if (sended > 0)
				con->resizeSendQueue(sended);
			eventins->ResponseEvent(con);
		}
	}
	catch (HTTPException &e) {
		CloseEvent(ccon);
	}
#ifdef DEBUG_MUTEX
	ccon->_httpexception.Note("WriteEvent", "unlock ConnectionMutex");
#endif
	ccon->_Mutex->unlock();
	return NULL;
}

void *libhttppp::Event::CloseEvent(void *curcon) {
	ConnectionContext *ccon = (ConnectionContext*)curcon;
	Event *eventins = ccon->_CurEvent;
#ifdef DEBUG_MUTEX
	ccon->_httpexception.Note("CloseEvent", "ConnectionMutex");
#endif
	ccon->_Mutex->lock();
	Connection *con = (Connection*)ccon->_CurConnection;
	eventins->DisconnectEvent(con);
#ifdef DEBUG_MUTEX
	ccon->_httpexception.Note("CloseEvent", "unlock ConnectionMutex");
#endif
	ccon->_Mutex->unlock();
	try {
//		epoll_ctl(eventins->_epollFD, EPOLL_CTL_DEL, con->getClientSocket()->getSocket(), &eventins->_setEvent);
		eventins->delConnection(con);
		curcon = NULL;
		eventins->_httpexception.Note("Connection shutdown!");
	}
	catch (HTTPException &e) {
		eventins->_httpexception.Note("Can't do Connection shutdown!");
	}
	return NULL;
}

/*Event Handlers*/
void libhttppp::Event::RequestEvent(libhttppp::Connection *curcon) {
	return;
}

void libhttppp::Event::ResponseEvent(libhttppp::Connection *curcon) {
	return;
}

void libhttppp::Event::ConnectEvent(libhttppp::Connection *curcon) {
	return;
}

void libhttppp::Event::DisconnectEvent(libhttppp::Connection *curcon) {
	return;
}
