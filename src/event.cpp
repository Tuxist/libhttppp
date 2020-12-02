/*******************************************************************************
Copyright (c) 2019, Jan Koester jan.koester@gmx.net
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

#include <config.h>

#include "connections.h"
#include "eventapi.h"
#include "threadpool.h"

#include "event.h"

libhttppp::Event::Event(libhttppp::ServerSocket* serversocket) : EVENT(serversocket){
    _Run=true;
	_Restart = true;
}

libhttppp::Event::~Event(){
}

libhttppp::EventApi::~EventApi(){
}

void libhttppp::Event::CTRLCloseEvent() {
	_Run = false;
}

void libhttppp::Event::CTRLBreakEvent() {
	_Restart = false;
}

void libhttppp::Event::runEventloop(){
    _Run=true;
    _Restart = true;
    while (_Restart) {
        ThreadPool thpool;
        SYSInfo sysinfo;
        size_t thrs = sysinfo.getNumberOfProcessors();
        initEventHandler();
        for (size_t i = 0; i < thrs; i++) {
            Thread *th = thpool.addThread();
            th->Create(WorkerThread, (void*)this);
        }
        
        for (Thread *curth = thpool.getfirstThread(); curth; curth = curth->nextThread()) {
            curth->Join();
        }
    }
}

void * libhttppp::Event::WorkerThread(void* wrkevent){
    Event *eventptr=(Event*)wrkevent;
    int lock=LockConnectionStatus::LOCKNOTREADY;
    while (eventptr->_Run) {
        int des=eventptr->waitEventHandler();
        for (int i = 0; i < des; ++i) {
            lock = eventptr->LockConnection(i);
            try{
                int state=eventptr->StatusEventHandler(i);
                switch(state){
                    case EventApi::EventHandlerStatus::EVCON:
                        if(lock==LockConnectionStatus::LOCKNOTREADY)
                        eventptr->ConnectEventHandler(i);
                        break;
                    case EventApi::EventHandlerStatus::EVIN:
                        if(lock==LockConnectionStatus::LOCKREADY)
                            eventptr->ReadEventHandler(i);
                        break;
                    case EventApi::EventHandlerStatus::EVOUT:
                        if(lock==LockConnectionStatus::LOCKREADY)
                            eventptr->WriteEventHandler(i);
                        break;
                }
            }catch(HTTPException &e){
                try{
                    if(lock==LockConnectionStatus::LOCKREADY)
                        eventptr->CloseEventHandler(i);
                }catch(...){}
            }
            eventptr->UnlockConnction(i);
        }
    }
    return NULL;
}
