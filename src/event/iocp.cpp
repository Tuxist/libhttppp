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

libhttppp::Queue* _QueueIns=NULL;

libhttppp::Queue::Queue(ServerSocket *serversocket) : ConnectionPool(serversocket) {
	_ServerSocket = serversocket;
    _EventEndloop=true;
	_EventRestartloop = true;
	_IOCP=INVALID_HANDLE_VALUE;
	_QueueIns = this;
}

void libhttppp::Queue::runEventloop() {
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

				hThread = CreateThread(NULL, 0, WorkerThread, _IOCP, 0, &dwThreadId);
				if (hThread == NULL) {
					//myprintf("CreateThread() failed to create worker thread: %d\n",
					//	GetLastError());
					__leave;
				}
				_ThreadHandles[dwCPU] = hThread;
				hThread = INVALID_HANDLE_VALUE;
			}

			_ServerSocket->listenSocket();
			//if (!CreateListenSocket())
			//	__leave;

			//if (!CreateAcceptSocket(TRUE))
			//	__leave;

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

			//	if (g_sdListen != INVALID_SOCKET) {
			//		closesocket(g_sdListen);
			//		g_sdListen = INVALID_SOCKET;
			//	}

			//	if (g_pCtxtListenSocket) {
			//		while (!HasOverlappedIoCompleted((LPOVERLAPPED)&g_pCtxtListenSocket->pIOContext->Overlapped))
			//			Sleep(0);

			//		if (g_pCtxtListenSocket->pIOContext->SocketAccept != INVALID_SOCKET)
			//			closesocket(g_pCtxtListenSocket->pIOContext->SocketAccept);
			//		g_pCtxtListenSocket->pIOContext->SocketAccept = INVALID_SOCKET;

			//		//
			//		// We know there is only one overlapped I/O on the listening socket
			//		//
			//		if (g_pCtxtListenSocket->pIOContext)
			//			xfree(g_pCtxtListenSocket->pIOContext);

			//		if (g_pCtxtListenSocket)
			//			xfree(g_pCtxtListenSocket);
			//		g_pCtxtListenSocket = NULL;
			//	}

			//	CtxtListFree();

			//	if (g_hIOCP) {
			//		CloseHandle(g_hIOCP);
			//		g_hIOCP = NULL;
			//	}
			//} //finally

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
}

DWORD WINAPI libhttppp::Queue::WorkerThread(LPVOID WorkThreadContext) {

	HANDLE hIOCP = (HANDLE)WorkThreadContext;
	BOOL bSuccess = FALSE;
	int nRet = 0;
	LPWSAOVERLAPPED lpOverlapped = NULL;
	PPER_SOCKET_CONTEXT lpPerSocketContext = NULL;
	PPER_SOCKET_CONTEXT lpAcceptSocketContext = NULL;
	PPER_IO_CONTEXT lpIOContext = NULL;
	WSABUF buffRecv;
	WSABUF buffSend;
	DWORD dwRecvNumBytes = 0;
	DWORD dwSendNumBytes = 0;
	DWORD dwFlags = 0;
	DWORD dwIoSize = 0;
	HRESULT hRet;

	while (TRUE) {

		////
		//// continually loop to service io completion packets
		////
		//bSuccess = GetQueuedCompletionStatus(
		//	hIOCP,
		//	&dwIoSize,
		//	(PDWORD_PTR)&lpPerSocketContext,
		//	(LPOVERLAPPED *)&lpOverlapped,
		//	INFINITE
		//);
		//if (!bSuccess)
		//	myprintf("GetQueuedCompletionStatus() failed: %d\n", GetLastError());

		//if (lpPerSocketContext == NULL) {

		//	//
		//	// CTRL-C handler used PostQueuedCompletionStatus to post an I/O packet with
		//	// a NULL CompletionKey (or if we get one for any reason).  It is time to exit.
		//	//
		//	return(0);
		//}

		//if (g_bEndServer) {

		//	//
		//	// main thread will do all cleanup needed - see finally block
		//	//
		//	return(0);
		//}

		//lpIOContext = (PPER_IO_CONTEXT)lpOverlapped;

		////
		////We should never skip the loop and not post another AcceptEx if the current
		////completion packet is for previous AcceptEx
		////
		//if (lpIOContext->IOOperation != ClientIoAccept) {
		//	if (!bSuccess || (bSuccess && (0 == dwIoSize))) {

		//		//
		//		// client connection dropped, continue to service remaining (and possibly 
		//		// new) client connections
		//		//
		//		CloseClient(lpPerSocketContext, FALSE);
		//		continue;
		//	}
		//}

		////
		//// determine what type of IO packet has completed by checking the PER_IO_CONTEXT 
		//// associated with this socket.  This will determine what action to take.
		////
		//switch (lpIOContext->IOOperation) {
		//case ClientIoAccept:

		//	//
		//	// When the AcceptEx function returns, the socket sAcceptSocket is 
		//	// in the default state for a connected socket. The socket sAcceptSocket 
		//	// does not inherit the properties of the socket associated with 
		//	// sListenSocket parameter until SO_UPDATE_ACCEPT_CONTEXT is set on 
		//	// the socket. Use the setsockopt function to set the SO_UPDATE_ACCEPT_CONTEXT 
		//	// option, specifying sAcceptSocket as the socket handle and sListenSocket 
		//	// as the option value. 
		//	//
		//	nRet = setsockopt(
		//		lpPerSocketContext->pIOContext->SocketAccept,
		//		SOL_SOCKET,
		//		SO_UPDATE_ACCEPT_CONTEXT,
		//		(char *)&g_sdListen,
		//		sizeof(g_sdListen)
		//	);

		//	if (nRet == SOCKET_ERROR) {

		//		//
		//		//just warn user here.
		//		//
		//		myprintf("setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed to update accept socket\n");
		//		WSASetEvent(g_hCleanupEvent[0]);
		//		return(0);
		//	}

		//	lpAcceptSocketContext = UpdateCompletionPort(
		//		lpPerSocketContext->pIOContext->SocketAccept,
		//		ClientIoAccept, TRUE);

		//	if (lpAcceptSocketContext == NULL) {

		//		//
		//		//just warn user here.
		//		//
		//		myprintf("failed to update accept socket to IOCP\n");
		//		WSASetEvent(g_hCleanupEvent[0]);
		//		return(0);
		//	}

		//	if (dwIoSize) {
		//		lpAcceptSocketContext->pIOContext->IOOperation = ClientIoWrite;
		//		lpAcceptSocketContext->pIOContext->nTotalBytes = dwIoSize;
		//		lpAcceptSocketContext->pIOContext->nSentBytes = 0;
		//		lpAcceptSocketContext->pIOContext->wsabuf.len = dwIoSize;
		//		hRet = StringCbCopyN(lpAcceptSocketContext->pIOContext->Buffer,
		//			MAX_BUFF_SIZE,
		//			lpPerSocketContext->pIOContext->Buffer,
		//			sizeof(lpPerSocketContext->pIOContext->Buffer)
		//		);
		//		lpAcceptSocketContext->pIOContext->wsabuf.buf = lpAcceptSocketContext->pIOContext->Buffer;

		//		nRet = WSASend(
		//			lpPerSocketContext->pIOContext->SocketAccept,
		//			&lpAcceptSocketContext->pIOContext->wsabuf, 1,
		//			&dwSendNumBytes,
		//			0,
		//			&(lpAcceptSocketContext->pIOContext->Overlapped), NULL);

		//		if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
		//			myprintf("WSASend() failed: %d\n", WSAGetLastError());
		//			CloseClient(lpAcceptSocketContext, FALSE);
		//		}
		//		else if (g_bVerbose) {
		//			myprintf("WorkerThread %d: Socket(%d) AcceptEx completed (%d bytes), Send posted\n",
		//				GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize);
		//		}
		//	}
		//	else {

		//		//
		//		// AcceptEx completes but doesn't read any data so we need to post
		//		// an outstanding overlapped read.
		//		//
		//		lpAcceptSocketContext->pIOContext->IOOperation = ClientIoRead;
		//		dwRecvNumBytes = 0;
		//		dwFlags = 0;
		//		buffRecv.buf = lpAcceptSocketContext->pIOContext->Buffer,
		//			buffRecv.len = MAX_BUFF_SIZE;
		//		nRet = WSARecv(
		//			lpAcceptSocketContext->Socket,
		//			&buffRecv, 1,
		//			&dwRecvNumBytes,
		//			&dwFlags,
		//			&lpAcceptSocketContext->pIOContext->Overlapped, NULL);
		//		if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
		//			myprintf("WSARecv() failed: %d\n", WSAGetLastError());
		//			CloseClient(lpAcceptSocketContext, FALSE);
		//		}
		//	}

		//	//
		//	//Time to post another outstanding AcceptEx
		//	//
		//	if (!CreateAcceptSocket(FALSE)) {
		//		myprintf("Please shut down and reboot the server.\n");
		//		WSASetEvent(g_hCleanupEvent[0]);
		//		return(0);
		//	}
		//	break;


		//case ClientIoRead:

		//	//
		//	// a read operation has completed, post a write operation to echo the
		//	// data back to the client using the same data buffer.
		//	//
		//	lpIOContext->IOOperation = ClientIoWrite;
		//	lpIOContext->nTotalBytes = dwIoSize;
		//	lpIOContext->nSentBytes = 0;
		//	lpIOContext->wsabuf.len = dwIoSize;
		//	dwFlags = 0;
		//	nRet = WSASend(
		//		lpPerSocketContext->Socket,
		//		&lpIOContext->wsabuf, 1, &dwSendNumBytes,
		//		dwFlags,
		//		&(lpIOContext->Overlapped), NULL);
		//	if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
		//		myprintf("WSASend() failed: %d\n", WSAGetLastError());
		//		CloseClient(lpPerSocketContext, FALSE);
		//	}
		//	else if (g_bVerbose) {
		//		myprintf("WorkerThread %d: Socket(%d) Recv completed (%d bytes), Send posted\n",
		//			GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize);
		//	}
		//	break;

		//case ClientIoWrite:

		//	//
		//	// a write operation has completed, determine if all the data intended to be
		//	// sent actually was sent.
		//	//
		//	lpIOContext->IOOperation = ClientIoWrite;
		//	lpIOContext->nSentBytes += dwIoSize;
		//	dwFlags = 0;
		//	if (lpIOContext->nSentBytes < lpIOContext->nTotalBytes) {

		//		//
		//		// the previous write operation didn't send all the data,
		//		// post another send to complete the operation
		//		//
		//		buffSend.buf = lpIOContext->Buffer + lpIOContext->nSentBytes;
		//		buffSend.len = lpIOContext->nTotalBytes - lpIOContext->nSentBytes;
		//		nRet = WSASend(
		//			lpPerSocketContext->Socket,
		//			&buffSend, 1, &dwSendNumBytes,
		//			dwFlags,
		//			&(lpIOContext->Overlapped), NULL);
		//		if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
		//			myprintf("WSASend() failed: %d\n", WSAGetLastError());
		//			CloseClient(lpPerSocketContext, FALSE);
		//		}
		//		else if (g_bVerbose) {
		//			myprintf("WorkerThread %d: Socket(%d) Send partially completed (%d bytes), Recv posted\n",
		//				GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize);
		//		}
		//	}
		//	else {

		//		//
		//		// previous write operation completed for this socket, post another recv
		//		//
		//		lpIOContext->IOOperation = ClientIoRead;
		//		dwRecvNumBytes = 0;
		//		dwFlags = 0;
		//		buffRecv.buf = lpIOContext->Buffer,
		//			buffRecv.len = MAX_BUFF_SIZE;
		//		nRet = WSARecv(
		//			lpPerSocketContext->Socket,
		//			&buffRecv, 1, &dwRecvNumBytes,
		//			&dwFlags,
		//			&lpIOContext->Overlapped, NULL);
		//		if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
		//			myprintf("WSARecv() failed: %d\n", WSAGetLastError());
		//			CloseClient(lpPerSocketContext, FALSE);
		//		}
		//		else if (g_bVerbose) {
		//			myprintf("WorkerThread %d: Socket(%d) Send completed (%d bytes), Recv posted\n",
		//				GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize);
		//		}
		//	}
		//	break;

		//} //switch
	} //while
	return(0);
}


BOOL WINAPI libhttppp::Queue::CtrlHandler(DWORD dwEvent) {
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

libhttppp::Queue::~Queue(){

}

void libhttppp::Queue::RequestEvent(Connection *curcon) {
  return;
}

void libhttppp::Queue::ResponseEvent(libhttppp::Connection *curcon){
  return;    
};
    
void libhttppp::Queue::ConnectEvent(libhttppp::Connection *curcon){
  return;    
};

void libhttppp::Queue::DisconnectEvent(libhttppp::Connection* curcon){
  return;
}

