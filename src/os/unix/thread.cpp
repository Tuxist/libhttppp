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

#include <errno.h>

#include "thread.h"
#include <string.h>

libhttppp::Thread::Thread(){
  _Pid=-1;
  _nextThread=NULL;
}

libhttppp::Thread::~Thread(){
  delete _nextThread;
}

void libhttppp::Thread::Create(void *function(void*), void *arguments) {
  HTTPException httpexception;
  int rth = pthread_create(&_Thread, NULL, function, arguments);
  if (rth != 0) {
#ifdef __GLIBCXX__
    char errbuf[255];
    httpexception.Error("Thread Create",strerror_r(errno, errbuf, 255));
#else
    char errbuf[255];
    strerror_r(errno, errbuf, 255);
    httpexception.Error("Thread Create",errbuf);
#endif
    throw httpexception;
  }
}

void libhttppp::Thread::Detach(){
    pthread_detach(_Thread);
}

int libhttppp::Thread::getThreadID() {
    HTTPException httpexception;
	httpexception.Note("ThreadID not support by this OS");
	return -1;
}

int libhttppp::Thread::getPid(){
  return _Pid;  
}

void libhttppp::Thread::setPid(int pid){
  _Pid=pid;
}

void libhttppp::Thread::Join(){
  if(pthread_join(_Thread,&_Retval)==0){
    return;  
  }else{
    HTTPException httpexception;    
#ifdef __GLIBCXX__
    char errbuf[255];
    httpexception.Error("Can't join Thread",strerror_r(errno, errbuf, 255));
#else
    char errbuf[255];
    strerror_r(errno, errbuf, 255);
    httpexception.Error("Can't join Thread",errbuf);
#endif  
  }
}

libhttppp::Thread *libhttppp::Thread::nextThread(){
    return _nextThread;
}
