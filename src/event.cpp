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

#include <systempp/sysinfo.h>

#include "config.h"

#include "connections.h"
#include "eventapi.h"
#include "threadpool.h"
#include "exception.h"

#include "event.h"
#include <signal.h>

bool libhttppp::Event::_Run=true;
bool libhttppp::Event::_Restart=false;

libhttppp::Event::Event(sys::ServerSocket* serversocket) : EVENT(serversocket){
//     libhttppp::CtrlHandler::initCtrlHandler();
    _firstLock=nullptr;
}

libhttppp::Event::~Event(){
}

libhttppp::EventApi::~EventApi(){
}

bool libhttppp::Event::LockConnection(int des) {
    LockedConnection *it;
    _EventLock.lock();
    for(it = _firstLock; it; it=it->_nextLockedConnection){
        if(it->_Descriptor==des){
            if(it->_ConectionLock.try_lock()){
                _EventLock.unlock();
                return true;
            }
            _EventLock.unlock();
            return false;
        }
    }
    
    if(!it){
        it=new LockedConnection;
        it->_prevLockedConnection=nullptr;
        _firstLock=it;
        
    }else{
        it->_nextLockedConnection=new LockedConnection;
        it->_prevLockedConnection=it;
        it=it->_nextLockedConnection;
    }
    it->_Descriptor=des;
    it->_ConectionLock.lock();
    it->_nextLockedConnection=nullptr;
    _EventLock.unlock();
    return true;
}

void libhttppp::Event::UnlockConnection(int des){
    HTTPException excep;
    LockedConnection *it;
    _EventLock.lock();
    for(it = _firstLock; it; it=it->_nextLockedConnection){
        if(it->_Descriptor==des){
            it->_ConectionLock.unlock();
            it->_prevLockedConnection=it->_nextLockedConnection;
            it->_nextLockedConnection=nullptr;
            delete it;
            _EventLock.unlock();
            return;
        }
    }
    excep[HTTPException::Critical]<<"UnlockConnection:" << des << "Lock not found";
    _EventLock.unlock();
    throw excep;
}

void libhttppp::Event::runEventloop(){
        sys::CpuInfo cpuinfo;
        signal(SIGPIPE, SIG_IGN);
        size_t thrs = 48;
        initEventHandler();
MAINWORKERLOOP:
        ThreadPool thpool;
        for (size_t i = 0; i < thrs; i++) {
            try{
                thpool.addjob(WorkerThread, (void*)this);
            }catch(HTTPException &e){
                throw e;
            }
        }
        
        thpool.join();
        
        if(libhttppp::Event::_Restart){
            libhttppp::Event::_Restart=false;
            goto MAINWORKERLOOP;
        }
}

void * libhttppp::Event::WorkerThread(void* wrkevent){
    Event *eventptr=((Event*)wrkevent);
    HTTPException excep;
    while (libhttppp::Event::_Run) {
        try {
            for (int i = 0; i < eventptr->waitEventHandler(); ++i) {
                if(eventptr->LockConnection(i)){                
                    try{
                        if(!eventptr->isConnected(i)){
                            eventptr->ConnectEventHandler(i);
                        }
                        switch(eventptr->StatusEventHandler(i)){
                            case EventHandlerStatus::EVIN:
                                eventptr->ReadEventHandler(i);
                                break;
                            case EventHandlerStatus::EVOUT:
                                eventptr->WriteEventHandler(i);
                                break;
                            default:
                                excep[HTTPException::Error] << "no action try to close";
                                throw excep;
                        }
                        eventptr->UnlockConnection(i);
                    }catch(HTTPException &e){
                        eventptr->CloseEventHandler(i);
                        if(e.getErrorType()==HTTPException::Critical){
                            eventptr->UnlockConnection(i);
                            throw e;
                        }
                        std::cerr << e.what() << std::endl;                        
                        eventptr->UnlockConnection(i);
                    }
                }
            }
        }catch(HTTPException &e){
            switch(e.getErrorType()){
                case HTTPException::Critical:
                    throw e;
                    break;
                default:
                    std::cerr<< e.what() << std::endl; 
            }
        }
    }
    return nullptr;
}
