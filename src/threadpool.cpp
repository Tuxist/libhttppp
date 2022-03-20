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

#include <unistd.h>

#include "threadpool.h"

libhttppp::ThreadPool::ThreadPool(){
}

libhttppp::ThreadPool::~ThreadPool(){
    for(std::vector<std::thread*>::iterator thit=_Threads.begin(); thit!=_Threads.end(); ++thit){
        (*thit)->join();
    }    
}

void libhttppp::ThreadPool::addjob(void *func(void*),void *args){
   _Threads.push_back((new std::thread(func,args)));
}


int libhttppp::ThreadPool::getAmount(){
    return _Threads.size();
}

void libhttppp::ThreadPool::join(){
    for(;;){
        if(_Threads.empty()){
           break; 
        }
        for(std::vector<std::thread*>::iterator thit=_Threads.begin(); thit!=_Threads.end(); ++thit){
            if((*thit)->joinable()){
                (*thit)->join();
                _Threads.erase(thit);
            }
        }
        usleep(1000);
    }
}
