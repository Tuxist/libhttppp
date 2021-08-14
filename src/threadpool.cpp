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

#include "threadpool.h"

libhttppp::ThreadPool::ThreadPool(){
    _firstThread=NULL;
    _lastThread=NULL;
    _Amount=0;
}

libhttppp::ThreadPool::~ThreadPool(){
    delete _firstThread;
    _lastThread=NULL;
}

libhttppp::Thread *libhttppp::ThreadPool::addThread(){
    ++_Amount;
    if(!_firstThread){
        _firstThread= new Thread;
        _lastThread=_firstThread;
    }else{
        _lastThread->_nextThread=new Thread;
        _lastThread=_lastThread->_nextThread;
    }
    return _lastThread;  
}

libhttppp::Thread *libhttppp::ThreadPool::delThread(libhttppp::Thread *delthread){
    Thread *prevthr=NULL;
    for(Thread *curthr=_firstThread; curthr; curthr=curthr->nextThread()){
        if(curthr==delthread){
            if(prevthr){
                prevthr->_nextThread=curthr->_nextThread;
            }
            if(curthr==_firstThread){
              _firstThread=curthr->nextThread();  
            }
            if(curthr==_lastThread){
              _lastThread=prevthr;
            }
            curthr->_nextThread=NULL;
            --_Amount;
            delete curthr;
        }
        prevthr=curthr;
    }
    if(prevthr)
        return prevthr->nextThread();
    else
        return _firstThread;
}

libhttppp::Thread * libhttppp::ThreadPool::getfirstThread(){
    return _firstThread;
}

libhttppp::Thread * libhttppp::ThreadPool::getlastThread(){
    return _lastThread;
}

int libhttppp::ThreadPool::getAmount(){
    return _Amount;
}

