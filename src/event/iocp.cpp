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

libhttppp::Queue::Queue(ServerSocket *serversocket) : ConnectionPool(serversocket) {
	_ServerSocket = serversocket;
    _Eventloop=true;
	_g_dwThreadCount = 0;
	_g_hIOCP = INVALID_HANDLE_VALUE;
}

void libhttppp::Queue::runEventloop() {
  SYSTEM_INFO sysinfo;

  for (int i = 0; i < MAX_WORKER_THREAD; i++) {
	  _g_ThreadHandles[i] = INVALID_HANDLE_VALUE;
  }

  GetSystemInfo(&sysinfo);
  _g_dwThreadCount = sysinfo.dwNumberOfProcessors * 2;
  __try{
	  InitializeCriticalSection(&_g_CriticalSection);
  }
  __except(EXCEPTION_EXECUTE_HANDLER){
	//  myprintf("InitializeCriticalSection raised exception.\n");
	  return;
  }

  while (_Eventloop) {
	__try {
		 _g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		 if (_g_hIOCP == NULL) {
			//  myprintf("CreateIoCompletionPort() failed to create I/O completion port: %d\n",
			//	  GetLastError());
			__leave;
		 }

		  for (DWORD dwCPU = 0; dwCPU < _g_dwThreadCount; dwCPU++) {
			HANDLE hThread = INVALID_HANDLE_VALUE;
			DWORD dwThreadId = 0;
			hThread = CreateThread(NULL, 0,_WorkerThread, _g_hIOCP, 0, &dwThreadId);
			if (hThread == NULL) {
			//	  myprintf("CreateThread() failed to create worker thread: %d\n",
			//		  GetLastError());
			  __leave;
			}
			_g_ThreadHandles[dwCPU] = hThread;
			hThread = INVALID_HANDLE_VALUE;
		  }

		  while (TRUE) {

			//  //
			//  // Loop forever accepting connections from clients until console shuts down.
			//  //
			//  sdAccept = WSAAccept(g_sdListen, NULL, NULL, NULL, 0);
			//  if (sdAccept == SOCKET_ERROR) {

			//	  //
			//	  // If user hits Ctrl+C or Ctrl+Brk or console window is closed, the control
			//	  // handler will close the g_sdListen socket. The above WSAAccept call will 
			//	  // fail and we thus break out the loop,
			//	  //
			//	  myprintf("WSAAccept() failed: %d\n", WSAGetLastError());
			//	  __leave;
			//  }

			//  //
			//  // we add the just returned socket descriptor to the IOCP along with its
			//  // associated key data.  Also the global list of context structures
			//  // (the key data) gets added to a global list.
			//  //
			//  lpPerSocketContext = UpdateCompletionPort(sdAccept, ClientIoRead, TRUE);
			//  if (lpPerSocketContext == NULL)
			//	  __leave;

			//  //
			//  // if a CTRL-C was pressed "after" WSAAccept returns, the CTRL-C handler
			//  // will have set this flag and we can break out of the loop here before
			//  // we go ahead and post another read (but after we have added it to the 
			//  // list of sockets to close).
			//  //
			//  if (g_bEndServer)
			//	  break;

			//  //
			//  // post initial receive on this socket
			//  //
			//  nRet = WSARecv(sdAccept, &(lpPerSocketContext->pIOContext->wsabuf),
			//	  1, &dwRecvNumBytes, &dwFlags,
			//	  &(lpPerSocketContext->pIOContext->Overlapped), NULL);
			//  if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
			//	  myprintf("WSARecv() Failed: %d\n", WSAGetLastError());
			//	  CloseClient(lpPerSocketContext, FALSE);
			//  }
		 } //while
	  }

	__finally {

		 //g_bEndServer = TRUE;

		 // //
		 // // Cause worker threads to exit
		 // //
		 if (_g_hIOCP) {
			for (DWORD i = 0; i < _g_dwThreadCount; i++)
			  PostQueuedCompletionStatus(_g_hIOCP, 0, 0, NULL);
		 }

		  //
		  //Make sure worker threads exits.
		  //
		 if (WAIT_OBJECT_0 != WaitForMultipleObjects(_g_dwThreadCount, _g_ThreadHandles, TRUE, 1000)) {
			 //  myprintf("WaitForMultipleObjects() failed: %d\n", GetLastError());
		 } else {
			for (DWORD i = 0; i < _g_dwThreadCount; i++) {
			  if (_g_ThreadHandles[i] != INVALID_HANDLE_VALUE) CloseHandle(_g_ThreadHandles[i]);
				  _g_ThreadHandles[i] = INVALID_HANDLE_VALUE;
			}
		 }

		 // CtxtListFree();

		 // if (g_hIOCP) {
			//  CloseHandle(g_hIOCP);
			//  g_hIOCP = NULL;
		 // }

		 // if (g_sdListen != INVALID_SOCKET) {
			//  closesocket(g_sdListen);
			//  g_sdListen = INVALID_SOCKET;
		 // }

		 // if (sdAccept != INVALID_SOCKET) {
			//  closesocket(sdAccept);
			//  sdAccept = INVALID_SOCKET;
		 // }

	  } //finally
  }
}

DWORD WINAPI libhttppp::Queue::_WorkerThread(LPVOID WorkThreadContext) {
	//HANDLE hIOCP = (HANDLE)WorkThreadContext;
	//BOOL bSuccess = FALSE;
	//int nRet = 0;
	//LPWSAOVERLAPPED lpOverlapped = NULL;
	//PPER_SOCKET_CONTEXT lpPerSocketContext = NULL;
	//PPER_IO_CONTEXT lpIOContext = NULL;
	//WSABUF buffRecv;
	//WSABUF buffSend;
	//DWORD dwRecvNumBytes = 0;
	//DWORD dwSendNumBytes = 0;
	//DWORD dwFlags = 0;
	//DWORD dwIoSize = 0;

	//while (TRUE) {

	//	//
	//	// continually loop to service io completion packets
	//	//
	//	bSuccess = GetQueuedCompletionStatus(hIOCP, &dwIoSize,
	//		(PDWORD_PTR)&lpPerSocketContext,
	//		(LPOVERLAPPED *)&lpOverlapped,
	//		INFINITE);
	//	if (!bSuccess)
	//		myprintf("GetQueuedCompletionStatus() failed: %d\n", GetLastError());

	//	if (lpPerSocketContext == NULL) {

	//		//
	//		// CTRL-C handler used PostQueuedCompletionStatus to post an I/O packet with
	//		// a NULL CompletionKey (or if we get one for any reason).  It is time to exit.
	//		//
	//		return(0);
	//	}

	//	if (g_bEndServer) {

	//		//
	//		// main thread will do all cleanup needed - see finally block
	//		//
	//		return(0);
	//	}

	//	if (!bSuccess || (bSuccess && (dwIoSize == 0))) {

	//		//
	//		// client connection dropped, continue to service remaining (and possibly 
	//		// new) client connections
	//		//
	//		CloseClient(lpPerSocketContext, FALSE);
	//		continue;
	//	}

	//	//
	//	// determine what type of IO packet has completed by checking the PER_IO_CONTEXT 
	//	// associated with this socket.  This will determine what action to take.
	//	//
	//	lpIOContext = (PPER_IO_CONTEXT)lpOverlapped;
	//	switch (lpIOContext->IOOperation) {
	//	case ClientIoRead:

	//		//
	//		// a read operation has completed, post a write operation to echo the
	//		// data back to the client using the same data buffer.
	//		//
	//		lpIOContext->IOOperation = ClientIoWrite;
	//		lpIOContext->nTotalBytes = dwIoSize;
	//		lpIOContext->nSentBytes = 0;
	//		lpIOContext->wsabuf.len = dwIoSize;
	//		dwFlags = 0;
	//		nRet = WSASend(lpPerSocketContext->Socket, &lpIOContext->wsabuf, 1,
	//			&dwSendNumBytes, dwFlags, &(lpIOContext->Overlapped), NULL);
	//		if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
	//			myprintf("WSASend() failed: %d\n", WSAGetLastError());
	//			CloseClient(lpPerSocketContext, FALSE);
	//		}
	//		else if (g_bVerbose) {
	//			myprintf("WorkerThread %d: Socket(%d) Recv completed (%d bytes), Send posted\n",
	//				GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize);
	//		}
	//		break;

	//	case ClientIoWrite:

	//		//
	//		// a write operation has completed, determine if all the data intended to be
	//		// sent actually was sent.
	//		//
	//		lpIOContext->IOOperation = ClientIoWrite;
	//		lpIOContext->nSentBytes += dwIoSize;
	//		dwFlags = 0;
	//		if (lpIOContext->nSentBytes < lpIOContext->nTotalBytes) {

	//			//
	//			// the previous write operation didn't send all the data,
	//			// post another send to complete the operation
	//			//
	//			buffSend.buf = lpIOContext->Buffer + lpIOContext->nSentBytes;
	//			buffSend.len = lpIOContext->nTotalBytes - lpIOContext->nSentBytes;
	//			nRet = WSASend(lpPerSocketContext->Socket, &buffSend, 1,
	//				&dwSendNumBytes, dwFlags, &(lpIOContext->Overlapped), NULL);
	//			if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
	//				myprintf("WSASend() failed: %d\n", WSAGetLastError());
	//				CloseClient(lpPerSocketContext, FALSE);
	//			}
	//			else if (g_bVerbose) {
	//				myprintf("WorkerThread %d: Socket(%d) Send partially completed (%d bytes), Recv posted\n",
	//					GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize);
	//			}
	//		}
	//		else {

	//			//
	//			// previous write operation completed for this socket, post another recv
	//			//
	//			lpIOContext->IOOperation = ClientIoRead;
	//			dwRecvNumBytes = 0;
	//			dwFlags = 0;
	//			buffRecv.buf = lpIOContext->Buffer,
	//				buffRecv.len = MAX_BUFF_SIZE;
	//			nRet = WSARecv(lpPerSocketContext->Socket, &buffRecv, 1,
	//				&dwRecvNumBytes, &dwFlags, &lpIOContext->Overlapped, NULL);
	//			if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
	//				myprintf("WSARecv() failed: %d\n", WSAGetLastError());
	//				CloseClient(lpPerSocketContext, FALSE);
	//			}
	//			else if (g_bVerbose) {
	//				myprintf("WorkerThread %d: Socket(%d) Send completed (%d bytes), Recv posted\n",
	//					GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize);
	//			}
	//		}
	//		break;

	//	} //switch
	//} //while
	return(0);
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
