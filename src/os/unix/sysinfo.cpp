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

#include "sysinfo.h"
#include <fstream>
#include <unistd.h>
#include <iostream>
#include <utils.h>
#include <cstring>

libhttppp::CpuInfo::CpuInfo(){
#if defined(ARCH_x86_64) || defined(ARCH_x86)
    asm volatile("cpuid":"=a" (Eax),
        "=b" (Ebx),
        "=c" (Ecx),
        "=d" (Edx)
        : "0" (Eax), "2" (Ecx)
    : );
#else
    Eax=sysconf(_SC_NPROCESSORS_ONLN);
    Ebx=0;
#endif
}

libhttppp::CpuInfo::~CpuInfo(){
}


int libhttppp::CpuInfo::getCores(){
    return Eax;
}

int libhttppp::CpuInfo::getThreads(){
    return Ebx;
}

int libhttppp::CpuInfo::getActualThread(){
    return Edx;
}


int libhttppp::CpuInfo::getPid(){
    return getpid();
}

libhttppp::SysInfo::SysInfo(){
    sysinfo(&_Sysinfo);
}

uint libhttppp::SysInfo::getTotalRam(){
    return _Sysinfo.totalram;
}

uint libhttppp::SysInfo::getBufferRam(){
    return _Sysinfo.bufferram;
}

uint libhttppp::SysInfo::getFreeRam(){
    return _Sysinfo.freeram;
}

libhttppp::MountPoint::MountPoint(){
    _nextMountPoint=nullptr;
}

libhttppp::MountPoint::~MountPoint(){
}

libhttppp::FsInfo::FsInfo(){
    _firstMountPoint=NULL;
    _lastMountPoint=NULL;
    std::fstream mountinfo;
    mountinfo.open("/proc/self/mountinfo",std::fstream::in);
    char buffer[1024];
    if(mountinfo.is_open()){
        NEXTFSTABLINE:
        if(mountinfo.getline(buffer,1024)){
            int entrypos=0,entry=0;
            bool ne=false;
            char split[10][FSINFOMAXLEN];
            for(int i=0; i<1024; ++i){
                switch(buffer[i]){
                    case ' ':{
                        if(!ne){
                            split[entry][entrypos]='\0';
                            ++entry;
                            entrypos=0;
                        }
                        ne=true;
                    }break;
                    case '\0':{
                        MountPoint *curm=addMountpoint();
                        scopy(split[9],split[9]+FSINFOMAXLEN,curm->_Device);
                        goto NEXTFSTABLINE;
                    }break;
                    default:{
                        split[entry][entrypos]=buffer[i];
                        ++entrypos;
                        ne=false;
                    }break;
                }
            }
        }
    }else{
        HTTPException httpexception;
        httpexception.Error("FsInfo","Could not open Fstab");
    }
}

libhttppp::FsInfo::~FsInfo()
{
}

libhttppp::MountPoint * libhttppp::FsInfo::addMountpoint(){
    if(_firstMountPoint){
        _lastMountPoint->_nextMountPoint=new MountPoint();
        _lastMountPoint=_lastMountPoint->_nextMountPoint;
    }else{
        _firstMountPoint=new MountPoint();
        _lastMountPoint=_firstMountPoint;
    }
    return _lastMountPoint;
}

libhttppp::MountPoint * libhttppp::FsInfo::getFirstDevice()
{
    return _firstMountPoint;
}

libhttppp::MountPoint * libhttppp::MountPoint::nextMountPoint(){
    return _nextMountPoint;
}

const char * libhttppp::MountPoint::getDevice(){
    return _Device;
}


