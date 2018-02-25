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


#include "../event.h"

#define xmalloc(s) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(s))
#define xfree(p) HeapFree(GetProcessHeap(),0,(p))

libhttppp::Event* _QueueIns=NULL;

libhttppp::Event::Event(ServerSocket *serversocket) {
	_ServerSocket = serversocket;
    _EventEndloop=true;
	_EventRestartloop = true;
	_IOCP=INVALID_HANDLE_VALUE;
	_pCtxtListenSocket = NULL;
	_pCtxtList = NULL;
	_QueueIns = this;
}

void libhttppp::Event::runEventloop() {
	_QueueIns = this;
	SYSTEM_INFO systemInfo;
	DWORD dwThreadCount = 0;
	int nRet = 0;

	_ThreadHandles[0] = (HANDLE)WSA_INVALID_EVENT;

	for (int i = 0; i < MAX_WORKER_THREAD; i++) {
		_ThreadHandles[i] = INVALID_HANDLE_VALUE;
	}

	if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
		//myprintf("SetConsoleCtrlHandler() failed to install console handler: %d\n",
			//GetLastError());
		return;
	}

	GetSystemInfo(&systemInfo);
	dwThreadCount = systemInfo.dwNumberOfProcessors * 2;

	if (WSA_INVALID_EVENT == (_CleanupEvent[0] = WSACreateEvent()))
	{
		//myprintf("WSACreateEvent() failed: %d\n", WSAGetLastError());
		return;
	}

	__try {
		InitializeCriticalSection(&_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		//myprintf("InitializeCriticalSection raised an exception.\n");
		SetConsoleCtrlHandler(CtrlHandler, FALSE);
		if (_CleanupEvent[0] != WSA_INVALID_EVENT) {
			WSACloseEvent(_CleanupEvent[0]);
			_CleanupEvent[0] = WSA_INVALID_EVENT;
		}
		return;
	}
	while (_EventRestartloop) {
		_EventRestartloop = FALSE;
		_EventEndloop = FALSE;
		WSAResetEvent(_CleanupEvent[0]);

		__try {

			//
			// notice that we will create more worker threads (dwThreadCount) than 
			// the thread concurrency limit on the IOCP.
			//
			_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
			if (_IOCP == NULL) {
				//myprintf("CreateIoCompletionPort() failed to create I/O completion port: %d\n",
					//GetLastError());
				__leave;
			}

			for (DWORD dwCPU = 0; dwCPU < dwThreadCount; dwCPU++) {

				//
				// Create worker threads to service the overlapped I/O requests.  The decision
				// to create 2 worker threads per CPU in the system is a heuristic.  Also,
				// note that thread handles are closed right away, because we will not need them
				// and the worker threads will continue to execute.
				//
				HANDLE  hThread;
				DWORD   dwThreadId;

				hThread = CreateThread(NULL, 0, WorkerThread, this, 0, &dwThreadId);
				if (hThread == NULL) {
					//myprintf("CreateThread() failed to create worker thread: %d\n",
					//	GetLastError());
					__leave;
				}
				_ThreadHandles[dwCPU] = hThread;
				hThread = INVALID_HANDLE_VALUE;
			}

			_ServerSocket->listenSocket();

			if (!CreateAcceptSocket(TRUE))
				__leave;

			WSAWaitForMultipleEvents(1, _CleanupEvent, TRUE, WSA_INFINITE, FALSE);
		}

		__finally {

			_EventEndloop = TRUE;

			//
			// Cause worker threads to exit
			//
			if (_IOCP) {
				for (DWORD i = 0; i < dwThreadCount; i++)
					PostQueuedCompletionStatus(_IOCP, 0, 0, NULL);
			}

			//
			// Make sure worker threads exits.
			//
			if (WAIT_OBJECT_0 != WaitForMultipleObjects(dwThreadCount, _ThreadHandles, TRUE, 1000)) {
				/*myprintf("WaitForMultipleObjects() failed: %d\n", GetLastError());*/
			}
			else {
				for (DWORD i = 0; i < dwThreadCount; i++) {
					if (_ThreadHandles[i] != INVALID_HANDLE_VALUE)
						CloseHandle(_ThreadHandles[i]);
					_ThreadHandles[i] = INVALID_HANDLE_VALUE;
				}
			}

			if (_ServerSocket->getSocket() != INVALID_SOCKET) {
				closesocket(_ServerSocket->getSocket());
			}

			if (_pCtxtListenSocket) {
				while (!HasOverlappedIoCompleted((LPOVERLAPPED)&_pCtxtListenSocket->pIOContext->Overlapped))
					Sleep(0);

				if (_pCtxtListenSocket->pIOContext->SocketAccept != INVALID_SOCKET)
					closesocket(_pCtxtListenSocket->pIOContext->SocketAccept);
				_pCtxtListenSocket->pIOContext->SocketAccept = INVALID_SOCKET;

				//
				// We know there is only one overlapped I/O on the listening socket
				//
				if (_pCtxtListenSocket->pIOContext)
					xfree(_pCtxtListenSocket->pIOContext);

				if (_pCtxtListenSocket)
					xfree(_pCtxtListenSocket);
				_pCtxtListenSocket = NULL;
			}

			CtxtListFree();

			if (_IOCP) {
				CloseHandle(_IOCP);
				_IOCP = NULL;
			}
		}//finally

		if (_EventRestartloop) {
			//	//myprintf("\niocpserverex is restarting...\n");
			} else {
			//	myprintf("\niocpserverex is exiting...\n");
			}

		} //while (g_bRestart)

		DeleteCriticalSection(&_CriticalSection);
		if (_CleanupEvent[0] != WSA_INVALID_EVENT) {
			WSACloseEvent(_CleanupEvent[0]);
			_CleanupEvent[0] = WSA_INVALID_EVENT;
		}
		WSACleanup();
		SetConsoleCtrlHandler(CtrlHandler, FALSE);
}

void libhttppp::Event::CloseEvent(libhttppp::Connection* curcon){
}

void libhttppp::Event::ReadEvent(libhttppp::Connection* curcon)
{
}

void libhttppp::Event::WriteEvent(libhttppp::Connection* curcon)
{
}


//
// Create a socket with all the socket options we need, namely disable buffering
// and set linger.
//
SOCKET libhttppp::Event::CreateSocket(void) {
	int nRet = 0;
	int nZero = 0;
	SOCKET sdSocket = INVALID_SOCKET;

	sdSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sdSocket == INVALID_SOCKET) {
		//myprintf("WSASocket(sdSocket) failed: %d\n", WSAGetLastError());
		return(sdSocket);
	}

	//
	// Disable send buffering on the socket.  Setting SO_SNDBUF
	// to 0 causes winsock to stop buffering sends and perform
	// sends directly from our buffers, thereby save one memory copy.
	//
	// However, this does prevent the socket from ever filling the
	// send pipeline. This can lead to packets being sent that are
	// not full (i.e. the overhead of the IP and TCP headers is 
	// great compared to the amount of data being carried).
	//
	// Disabling the send buffer has less serious repercussions 
	// than disabling the receive buffer.
	//
	nZero = 0;
	nRet = setsockopt(sdSocket, SOL_SOCKET, SO_SNDBUF, (char *)&nZero, sizeof(nZero));
	if (nRet == SOCKET_ERROR) {
		//myprintf("setsockopt(SNDBUF) failed: %d\n", WSAGetLastError());
		return(sdSocket);
	}

	//
	// Don't disable receive buffering. This will cause poor network
	// performance since if no receive is posted and no receive buffers,
	// the TCP stack will set the window size to zero and the peer will
	// no longer be allowed to send data.
	//

	// 
	// Do not set a linger value...especially don't set it to an abortive
	// close. If you set abortive close and there happens to be a bit of
	// data remaining to be transfered (or data that has not been 
	// acknowledged by the peer), the connection will be forcefully reset
	// and will lead to a loss of data (i.e. the peer won't get the last
	// bit of data). This is BAD. If you are worried about malicious
	// clients connecting and then not sending or receiving, the server
	// should maintain a timer on each connection. If after some point,
	// the server deems a connection is "stale" it can then set linger
	// to be abortive and close the connection.
	//

	/*
	LINGER lingerStruct;
	lingerStruct.l_onoff = 1;
	lingerStruct.l_linger = 0;
	nRet = setsockopt(sdSocket, SOL_SOCKET, SO_LINGER,
	(char *)&lingerStruct, sizeof(lingerStruct));
	if( nRet == SOCKET_ERROR ) {
	myprintf("setsockopt(SO_LINGER) failed: %d\n", WSAGetLastError());
	return(sdSocket);
	}
	*/

	return(sdSocket);
}

BOOL libhttppp::Event::CreateAcceptSocket(BOOL fUpdateIOCP) {

	int nRet = 0;
	DWORD dwRecvNumBytes = 0;
	DWORD bytes = 0;

	//
	// GUID to Microsoft specific extensions
	//
	GUID acceptex_guid = WSAID_ACCEPTEX;

	//
	//The context for listening socket uses the SockAccept member to store the
	//socket for client connection. 
	//
	if (fUpdateIOCP) {
		_pCtxtListenSocket = UpdateCompletionPort(_ServerSocket->getSocket(), ClientIoAccept, FALSE);
		if (_pCtxtListenSocket == NULL) {
			//myprintf("failed to update listen socket to IOCP\n");
			return(FALSE);
		}

		// Load the AcceptEx extension function from the provider for this socket
		nRet = WSAIoctl(
			_ServerSocket->getSocket(),
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&acceptex_guid,
			sizeof(acceptex_guid),
			&_pCtxtListenSocket->fnAcceptEx,
			sizeof(_pCtxtListenSocket->fnAcceptEx),
			&bytes,
			NULL,
			NULL
		);
		if (nRet == SOCKET_ERROR)
		{
			////myprintf("failed to load AcceptEx: %d\n", WSAGetLastError());
			return (FALSE);
		}
	}

	_pCtxtListenSocket->pIOContext->SocketAccept = CreateSocket();
	if (_pCtxtListenSocket->pIOContext->SocketAccept == INVALID_SOCKET) {
		//myprintf("failed to create new accept socket\n");
		return(FALSE);
	}

	//
	// pay close attention to these parameters and buffer lengths
	//
	nRet = _pCtxtListenSocket->fnAcceptEx(_ServerSocket->getSocket(), _pCtxtListenSocket->pIOContext->SocketAccept,
		(LPVOID)(_pCtxtListenSocket->pIOContext->Buffer),
		BLOCKSIZE - (2 * (sizeof(SOCKADDR_STORAGE) + 16)),
		sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
		&dwRecvNumBytes,
		(LPOVERLAPPED) &(_pCtxtListenSocket->pIOContext->Overlapped));
	if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
		//myprintf("AcceptEx() failed: %d\n", WSAGetLastError());
		return(FALSE);
	}

	return(TRUE);
}

//
//  Allocate a context structures for the socket and add the socket to the IOCP.  
//  Additionally, add the context structure to the global list of context structures.
//
libhttppp::Event::PPER_SOCKET_CONTEXT libhttppp::Event::UpdateCompletionPort(SOCKET sd, IO_OPERATION ClientIo,
	                                                                         BOOL bAddToList) {

	PPER_SOCKET_CONTEXT lpPerSocketContext;

	lpPerSocketContext = CtxtAllocate(sd, ClientIo);
	if (lpPerSocketContext == NULL)
		return(NULL);

	_IOCP = CreateIoCompletionPort((HANDLE)sd, _IOCP, (DWORD_PTR)lpPerSocketContext, 0);
	if (_IOCP == NULL) {
		//myprintf("CreateIoCompletionPort() failed: %d\n", GetLastError());
		if (lpPerSocketContext->pIOContext)
			xfree(lpPerSocketContext->pIOContext);
		xfree(lpPerSocketContext);
		return(NULL);
	}

	//
	//The listening socket context (bAddToList is FALSE) is not added to the list.
	//All other socket contexts are added to the list.
	//
	if (bAddToList) 
		CtxtListAddTo(lpPerSocketContext);
	return(lpPerSocketContext);
}

//
//  Close down a connection with a client.  This involves closing the socket (when 
//  initiated as a result of a CTRL-C the socket closure is not graceful).  Additionally, 
//  any context data associated with that socket is free'd.
//
VOID libhttppp::Event::CloseClient(PPER_SOCKET_CONTEXT lpPerSocketContext, BOOL bGraceful) {

	__try
	{
		EnterCriticalSection(&_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		//myprintf("EnterCriticalSection raised an exception.\n");
		return;
	}

	if (lpPerSocketContext) {
		if (!bGraceful) {

			//
			// force the subsequent closesocket to be abortative.
			//
			LINGER  lingerStruct;

			lingerStruct.l_onoff = 1;
			lingerStruct.l_linger = 0;
			setsockopt(lpPerSocketContext->CurConnection->getClientSocket()->getSocket(), SOL_SOCKET, SO_LINGER,
				(char *)&lingerStruct, sizeof(lingerStruct));
		}
		if (lpPerSocketContext->pIOContext->SocketAccept != INVALID_SOCKET) {
			closesocket(lpPerSocketContext->pIOContext->SocketAccept);
			lpPerSocketContext->pIOContext->SocketAccept = INVALID_SOCKET;
		};

		closesocket(lpPerSocketContext->CurConnection->getClientSocket()->getSocket());
		lpPerSocketContext->CurConnection->getClientSocket()->setSocket(INVALID_SOCKET);
		CtxtListDeleteFrom(lpPerSocketContext);
		lpPerSocketContext = NULL;
	}
	else {
		//myprintf("CloseClient: lpPerSocketContext is NULL\n");
	}

	LeaveCriticalSection(&_CriticalSection);

	return;
}

//
// Allocate a socket context for the new connection.  
//
libhttppp::Event::PPER_SOCKET_CONTEXT libhttppp::Event::CtxtAllocate(SOCKET sd, IO_OPERATION ClientIO) {

	PPER_SOCKET_CONTEXT lpPerSocketContext;

	__try
	{
		EnterCriticalSection(&_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		//myprintf("EnterCriticalSection raised an exception.\n");
		return NULL;
	}

	lpPerSocketContext = (PPER_SOCKET_CONTEXT)xmalloc(sizeof(PER_SOCKET_CONTEXT));
	if (lpPerSocketContext) {
		lpPerSocketContext->pIOContext = (PPER_IO_CONTEXT)xmalloc(sizeof(PER_IO_CONTEXT));
		if (lpPerSocketContext->pIOContext) {
			lpPerSocketContext->CurConnection->getClientSocket()->setSocket(sd);
			lpPerSocketContext->pCtxtBack = NULL;
			lpPerSocketContext->pCtxtForward = NULL;

			lpPerSocketContext->pIOContext->Overlapped.Internal = 0;
			lpPerSocketContext->pIOContext->Overlapped.InternalHigh = 0;
			lpPerSocketContext->pIOContext->Overlapped.Offset = 0;
			lpPerSocketContext->pIOContext->Overlapped.OffsetHigh = 0;
			lpPerSocketContext->pIOContext->Overlapped.hEvent = NULL;
			lpPerSocketContext->pIOContext->IOOperation = ClientIO;
			lpPerSocketContext->pIOContext->pIOContextForward = NULL;
			lpPerSocketContext->pIOContext->nTotalBytes = 0;
			lpPerSocketContext->pIOContext->nSentBytes = 0;
			lpPerSocketContext->pIOContext->wsabuf.buf = lpPerSocketContext->pIOContext->Buffer;
			lpPerSocketContext->pIOContext->wsabuf.len = sizeof(lpPerSocketContext->pIOContext->Buffer);
			lpPerSocketContext->pIOContext->SocketAccept = INVALID_SOCKET;

			ZeroMemory(lpPerSocketContext->pIOContext->wsabuf.buf, lpPerSocketContext->pIOContext->wsabuf.len);
		}
		else {
			xfree(lpPerSocketContext);
			//myprintf("HeapAlloc() PER_IO_CONTEXT failed: %d\n", GetLastError());
		}

	}
	else {
		//myprintf("HeapAlloc() PER_SOCKET_CONTEXT failed: %d\n", GetLastError());
		return(NULL);
	}

	LeaveCriticalSection(&_CriticalSection);

	return(lpPerSocketContext);
}

//
//  Add a client connection context structure to the global list of context structures.
//
VOID libhttppp::Event::CtxtListAddTo(PPER_SOCKET_CONTEXT lpPerSocketContext) {

	PPER_SOCKET_CONTEXT pTemp;

	__try
	{
		EnterCriticalSection(&_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		//myprintf("EnterCriticalSection raised an exception.\n");
		return;
	}

	if (_pCtxtList == NULL) {

		//
		// add the first node to the linked list
		//
		lpPerSocketContext->pCtxtBack = NULL;
		lpPerSocketContext->pCtxtForward = NULL;
		_pCtxtList = lpPerSocketContext;
	}
	else {

		//
		// add node to head of list
		//
		pTemp = _pCtxtList;

		_pCtxtList = lpPerSocketContext;
		lpPerSocketContext->pCtxtBack = pTemp;
		lpPerSocketContext->pCtxtForward = NULL;

		pTemp->pCtxtForward = lpPerSocketContext;
	}

	LeaveCriticalSection(&_CriticalSection);

	return;
}

//
//  Remove a client context structure from the global list of context structures.
//
VOID libhttppp::Event::CtxtListDeleteFrom(PPER_SOCKET_CONTEXT lpPerSocketContext) {

	PPER_SOCKET_CONTEXT pBack;
	PPER_SOCKET_CONTEXT pForward;
	PPER_IO_CONTEXT     pNextIO = NULL;
	PPER_IO_CONTEXT     pTempIO = NULL;

	__try
	{
		EnterCriticalSection(&_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		//myprintf("EnterCriticalSection raised an exception.\n");
		return;
	}

	if (lpPerSocketContext) {
		pBack = lpPerSocketContext->pCtxtBack;
		pForward = lpPerSocketContext->pCtxtForward;

		if (pBack == NULL && pForward == NULL) {

			//
			// This is the only node in the list to delete
			//
			_pCtxtList = NULL;
		}
		else if (pBack == NULL && pForward != NULL) {

			//
			// This is the start node in the list to delete
			//
			pForward->pCtxtBack = NULL;
			_pCtxtList = pForward;
		}
		else if (pBack != NULL && pForward == NULL) {

			//
			// This is the end node in the list to delete
			//
			pBack->pCtxtForward = NULL;
		}
		else if (pBack && pForward) {

			//
			// Neither start node nor end node in the list
			//
			pBack->pCtxtForward = pForward;
			pForward->pCtxtBack = pBack;
		}

		//
		// Free all i/o context structures per socket
		//
		pTempIO = (PPER_IO_CONTEXT)(lpPerSocketContext->pIOContext);
		do {
			pNextIO = (PPER_IO_CONTEXT)(pTempIO->pIOContextForward);
			if (pTempIO) {

				//
				//The overlapped structure is safe to free when only the posted i/o has
				//completed. Here we only need to test those posted but not yet received 
				//by PQCS in the shutdown process.
				//
				if (_EventEndloop)
					while (!HasOverlappedIoCompleted((LPOVERLAPPED)pTempIO)) Sleep(0);
				xfree(pTempIO);
				pTempIO = NULL;
			}
			pTempIO = pNextIO;
		} while (pNextIO);

		xfree(lpPerSocketContext);
		lpPerSocketContext = NULL;
	}
	else {
		//myprintf("CtxtListDeleteFrom: lpPerSocketContext is NULL\n");
	}

	LeaveCriticalSection(&_CriticalSection);

	return;
}

//
//  Free all context structure in the global list of context structures.
//
VOID libhttppp::Event::CtxtListFree() {
	PPER_SOCKET_CONTEXT pTemp1, pTemp2;

	__try
	{
		EnterCriticalSection(&_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		//myprintf("EnterCriticalSection raised an exception.\n");
		return;
	}

	pTemp1 = _pCtxtList;
	while (pTemp1) {
		pTemp2 = pTemp1->pCtxtBack;
		CloseClient(pTemp1, FALSE);
		pTemp1 = pTemp2;
	}

	LeaveCriticalSection(&_CriticalSection);

	return;
}

DWORD WINAPI libhttppp::Event::WorkerThread(LPVOID WorkThreadContext) {
	Event  *event = (Event*)WorkThreadContext;
	BOOL bSuccess = FALSE;
	int nRet = 0;
	LPWSAOVERLAPPED lpOverlapped = NULL;
	PPER_SOCKET_CONTEXT lpPerSocketContext = NULL;
	PPER_SOCKET_CONTEXT lpAcceptSocketContext = NULL;
	PPER_IO_CONTEXT lpIOContext = NULL;
	ConnectionPool cpool(event->_ServerSocket);
	Connection *curcon = NULL;
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
			event->_IOCP,
			&dwIoSize,
			(PDWORD_PTR)&lpPerSocketContext,
			(LPOVERLAPPED *)&lpOverlapped,
			INFINITE
		);
		if (!bSuccess) {
			//_httpexception.Cirtical("GetQueuedCompletionStatus() failed", GetLastError());
			//throw _httpexception;
			;
		}
		if (lpPerSocketContext == NULL) {

			//
			// CTRL-C handler used PostQueuedCompletionStatus to post an I/O packet with
			// a NULL CompletionKey (or if we get one for any reason).  It is time to exit.
			//
			return(0);
		}

		if (event->_EventEndloop) {

			//
			// main thread will do all cleanup needed - see finally block
			//
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
				event->CloseClient(lpPerSocketContext, FALSE);
				continue;
			}
		}

		//
		// determine what type of IO packet has completed by checking the PER_IO_CONTEXT 
		// associated with this socket.  This will determine what action to take.
		//
		switch (lpIOContext->IOOperation) {
		case ClientIoAccept: {

			//
			// When the AcceptEx function returns, the socket sAcceptSocket is 
			// in the default state for a connected socket. The socket sAcceptSocket 
			// does not inherit the properties of the socket associated with 
			// sListenSocket parameter until SO_UPDATE_ACCEPT_CONTEXT is set on 
			// the socket. Use the setsockopt function to set the SO_UPDATE_ACCEPT_CONTEXT 
			// option, specifying sAcceptSocket as the socket handle and sListenSocket 
			// as the option value. 
			//
			SOCKET sock = event->_ServerSocket->getSocket();

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
				//printf("setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed to update accept socket\n");
				WSASetEvent(event->_CleanupEvent[0]);
				return(0);
			}

			lpAcceptSocketContext = event->UpdateCompletionPort(
				lpPerSocketContext->pIOContext->SocketAccept,
				ClientIoAccept, TRUE);

			if (lpAcceptSocketContext == NULL) {

				//
				//just warn user here.
				//
				//printf("failed to update accept socket to IOCP\n");
				WSASetEvent(event->_CleanupEvent[0]);
				return(0);
			}

			//
			// Add connection to connection handler
			//
			curcon = cpool.addConnection();
            lpPerSocketContext->CurConnection=curcon;
			ClientSocket *clientsocket = curcon->getClientSocket();
			clientsocket->setSocket(lpAcceptSocketContext->CurConnection->getClientSocket()->getSocket());
			event->ConnectEvent(curcon);
			printf("aceppt connection on port: %ld\n", clientsocket->getSocket());

			if (dwIoSize) {
				lpAcceptSocketContext->pIOContext->IOOperation = ClientIoWrite;
				lpAcceptSocketContext->pIOContext->nTotalBytes = dwIoSize;
				lpAcceptSocketContext->pIOContext->nSentBytes = 0;
				hRet = StringCbCopyN(lpAcceptSocketContext->pIOContext->Buffer,
					BLOCKSIZE,
					lpPerSocketContext->pIOContext->Buffer,
					sizeof(lpPerSocketContext->pIOContext->Buffer)
				);

				curcon->addRecvQueue(lpAcceptSocketContext->pIOContext->Buffer, dwIoSize);
				event->RequestEvent(curcon);

				if (curcon->getSendData()) {
					lpAcceptSocketContext->pIOContext->wsabuf.len = curcon->getSendData()->getDataSize();
					lpAcceptSocketContext->pIOContext->wsabuf.buf = (char*)curcon->getSendData()->getData();
					nRet = WSASend(
						lpPerSocketContext->pIOContext->SocketAccept,
						&lpAcceptSocketContext->pIOContext->wsabuf, 1,
						&dwSendNumBytes,
						0,
						&(lpAcceptSocketContext->pIOContext->Overlapped), NULL);
					curcon->resizeSendQueue(dwSendNumBytes);
				}
				if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
					//printf("WSASend() failed: %d\n", WSAGetLastError());
					event->CloseClient(lpAcceptSocketContext, FALSE);
					cpool.delConnection(curcon);
					curcon = NULL;
				}
			}

			//
			//Time to post another outstanding AcceptEx
			//
			if (!event->CreateAcceptSocket(FALSE)) {
				//myprintf("Please shut down and reboot the server.\n");
				WSASetEvent(event->_CleanupEvent[0]);
				return(0);
			}
			break;
		}


		case ClientIoRead: {

			//
			// a read operation has completed, post a write operation to echo the
			// data back to the client using the same data buffer.
			//
			lpIOContext->IOOperation = ClientIoWrite;
			lpIOContext->nTotalBytes = dwIoSize;
			lpIOContext->nSentBytes = 0;
			lpIOContext->wsabuf.len = dwIoSize;
			dwFlags = 0;
            
			curcon = lpPerSocketContext->CurConnection;
			if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
				//myprintf("WSASend() failed: %d\n", WSAGetLastError());
				event->CloseClient(lpPerSocketContext, FALSE);
				event->DisconnectEvent(curcon);
				cpool.delConnection(curcon);
				break;
			}
			if(curcon)
				event->RequestEvent(curcon);
			printf("read\n");
			break;
		}

		case ClientIoWrite: {

			//
			// a write operation has completed, determine if all the data intended to be
			// sent actually was sent.
			//
			lpIOContext->IOOperation = ClientIoWrite;
			lpIOContext->nSentBytes += dwIoSize;
			dwFlags = 0;
			curcon = lpPerSocketContext->CurConnection;
			if (!curcon)
				break;
			if (curcon->getSendData() && curcon->getSendData()->getDataSize()<0) {

				//
				// the previous write operation didn't send all the data,
				// post another send to complete the operation
				//
				buffSend.buf = (char*)curcon->getSendData()->getData();
				buffSend.len = curcon->getSendData()->getDataSize();
				nRet = WSASend(
					lpPerSocketContext->CurConnection->getClientSocket()->getSocket(),
					&buffSend, 1, &dwSendNumBytes,
					dwFlags,
					&(lpIOContext->Overlapped), NULL);
				if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
					//myprintf("WSASend() failed: %d\n", WSAGetLastError());
					event->CloseClient(lpPerSocketContext, FALSE);
					event->DisconnectEvent(curcon);
					cpool.delConnection(lpPerSocketContext->CurConnection);
				} else {
					curcon->resizeSendQueue(dwSendNumBytes);
				}
			}
			break;
		}
		} //switch
	} //while
	return(0);
}


BOOL WINAPI libhttppp::Event::CtrlHandler(DWORD dwEvent) {
	switch (dwEvent) {
	case CTRL_BREAK_EVENT:
		_QueueIns->_EventRestartloop = TRUE;
	case CTRL_C_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
	case CTRL_CLOSE_EVENT:
		_QueueIns->_EventEndloop = TRUE;
		WSASetEvent(_QueueIns->_CleanupEvent[0]);
		break;
	default:
		//
		// unknown type--better pass it on.
		//

		return(FALSE);
	}
	return(TRUE);
}

libhttppp::Event::~Event(){

}

void libhttppp::Event::RequestEvent(Connection *curcon) {
  return;
}

void libhttppp::Event::ResponseEvent(libhttppp::Connection *curcon){
  return;    
};
    
void libhttppp::Event::ConnectEvent(libhttppp::Connection *curcon){
  return;    
};

void libhttppp::Event::DisconnectEvent(libhttppp::Connection* curcon){
  return;
}

