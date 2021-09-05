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
#include "exception.h"

#include "event.h"

bool libhttppp::Event::_Run=true;
bool libhttppp::Event::_Restart=false;

libhttppp::Event::Event(libhttppp::ServerSocket* serversocket) : EVENT(serversocket){
    libhttppp::CtrlHandler::initCtrlHandler();
}

libhttppp::Event::~Event(){
}

libhttppp::EventApi::~EventApi(){
}

void libhttppp::CtrlHandler::CTRLCloseEvent() {
    libhttppp::Event::_Run = false;
}

void libhttppp::CtrlHandler::CTRLBreakEvent() {
    libhttppp::Event::_Restart = true;
    libhttppp::Event::_Run=false;
    libhttppp::Event::_Run=true;
}

void libhttppp::CtrlHandler::CTRLTermEvent() {
    libhttppp::Event::_Run = false;
}

void libhttppp::CtrlHandler::SIGPIPEEvent() {
    return;
}

void libhttppp::Event::runEventloop(){
        CpuInfo cpuinfo;
        size_t thrs = cpuinfo.getCores();
        initEventHandler();
MAINWORKERLOOP:
        ThreadPool thpool;
        for (size_t i = 0; i < thrs; i++) {
            Thread *cthread=thpool.addThread();
            cthread->Create(WorkerThread, (void*)this);
        }
        for (Thread *curth = thpool.getfirstThread(); curth; curth = curth->nextThread()) {
            curth->Join();
        }
        if(libhttppp::Event::_Restart){
            libhttppp::Event::_Restart=false;
            goto MAINWORKERLOOP;
        }
}

void * libhttppp::Event::WorkerThread(void* wrkevent){
    Event *eventptr=((Event*)wrkevent);
    int lstate=LockState::NOLOCK;
    while (libhttppp::Event::_Run) {
        try {
            for (int i = 0; i < eventptr->waitEventHandler(); ++i) {
                if((lstate=eventptr->LockConnection(i))!=LockState::ERRLOCK){
                    try{
                        switch(eventptr->StatusEventHandler(i)){
                            case EventApi::EventHandlerStatus::EVNOTREADY:
                                eventptr->ConnectEventHandler(i);
                                break;
                            case EventApi::EventHandlerStatus::EVIN:
                                eventptr->ReadEventHandler(i);
                                break;
                            case EventApi::EventHandlerStatus::EVOUT:
                                eventptr->WriteEventHandler(i);
                                break;
                            default:
                                eventptr->CloseEventHandler(i);
                                break;
                        }
                        eventptr->UnlockConnection(i);
                    }catch(HTTPException &e){
                        switch(e.getErrorType()){
                            case HTTPException::Critical:
                                throw e;
                                break;
                            case HTTPException::Error:
                                try{
                                    eventptr->CloseEventHandler(i);
                                }catch(HTTPException &e){
                                    eventptr->UnlockConnection(i);
                                    throw e;
                                }
                                break;      
                        }
                    }
                }                        
            }
        }catch(HTTPException &e){
            switch(e.getErrorType()){
                case HTTPException::Critical:
                    throw e;
                    break;
                default:
                    Console con;
                    con << e.what() << con.endl;
            }
        }
    }
    return NULL;
}
