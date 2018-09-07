/*******************************************************************************
Copyright (c) 2018, Jan Koester jan.koester@gmx.net
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

#include "thread.h" 

libhttppp::Thread::Thread(){
  _ThreadId = -1;
  _nextThread=NULL;
}

libhttppp::Thread::~Thread(){
}

void libhttppp::Thread::Create(LPTHREAD_START_ROUTINE function, void* arguments) {
    HTTPException  httpexception;
	_Thread = CreateThread(NULL, 0, function, arguments, 0, &_ThreadId);
	if (_Thread == NULL) {
		httpexception.Critical("Can't create Thread!", GetLastError());
		throw httpexception;
	}
}

void libhttppp::Thread::Detach() {
    HTTPException   httpexception;
	httpexception.Note("Detach not support by this OS");
}

DWORD libhttppp::Thread::getThreadID() {
  return _ThreadId;
}

HANDLE libhttppp::Thread::getHandle() {
	return _Thread;
}

void libhttppp::Thread::Join(){
}


libhttppp::Thread *libhttppp::Thread::nextThread(){
    return _nextThread;
}
