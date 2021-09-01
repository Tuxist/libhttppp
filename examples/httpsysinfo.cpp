/*******************************************************************************
 * Copyright (c) 2014, Jan Koester jan.koester@gmx.net
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 * Neither the name of the <organization> nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************/

#include <http.h>
#include <httpd.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <exception.h>

#include "utils.h"
#include "header_png.h"
#include "favicon_ico.h"
#ifndef Windows
#include <sys/utsname.h>
#endif // !Windows

std::string &operator<<(std::string &data,const char *value){
//     data.assign(value,value+libhttppp::getlen(value));
    data+=value;
    return data;
};

class HtmlTable{
public:
    HtmlTable(const char *id=NULL){
        _firstRow=nullptr;
        _lastRow=nullptr;
        _Id=id;
    }
    
    ~HtmlTable(){
        
    }
    
    class Row {
    public:
        Row &operator<<(const char *value){
            _Data+=value;
            return *this;
        };
        
        Row &operator<<(int value){
            char buf[255];
            snprintf(buf,255, "%d", value); 
            return *this << buf;
        };
        
    private:
        std::string  _Data;
        int   _Size;
        Row(){
            _Size=0;
            _nextRow=nullptr;
        };
        ~Row(){
            delete _nextRow;
        };
        Row *_nextRow;
        friend class HtmlTable;
    };
    
    
    
    Row &createRow(){
        if(_firstRow!=nullptr){
            _lastRow->_nextRow=new Row;
            _lastRow=_lastRow->_nextRow;
        }else{
            _firstRow=new Row;
            _lastRow=_firstRow;
        }
        return *_lastRow;
    }
    
    const char *getTable(){
        _Buffer.clear();
        if(_Id!=NULL)
            _Buffer << "<table id=\"" << _Id << "\">";
        else
            _Buffer << "<table>";
        for(Row *curow=_firstRow; curow; curow=curow->_nextRow){
            _Buffer << "<tr>";
            _Buffer+=curow->_Data;
            _Buffer << "</tr>";
        }
        _Buffer << "</table>";
        _Buffer.push_back('\0');
        return _Buffer.data();
    }
private:
    std::string         _Buffer;
    const char        *_Id;
    Row               *_firstRow;
    Row               *_lastRow;
};

class HtmlContent{
    void generateHeader(){
        
    }
};


class IndexPage{
public:
    IndexPage(){
        _Buffer=nullptr;
        _Index << "<!DOCTYPE html><body style=\"color:rgb(239, 240, 241); background:rgb(79, 83, 88);\">"
        << "<div id=\"mainbar\" style=\"border-radius: 38px; background:rgb(35, 38, 41); width:1280px; margin:0px auto;\">"
        << "<div id=\"headerimage\"><img src=\"images/header.png\"/></div>"
        << "<div id=\"sysinfo\" style=\"padding:40px 20px;\"><span>System Info:</span><br/>"; 
        KernelInfo();
        CPUInfo();
        SysInfo();
        FsInfo();
        _Index << "</div></div></body></html>";
    }
    
    void KernelInfo(){
        #ifndef Windows
        struct utsname usysinfo;
        uname(&usysinfo);
        HtmlTable htmltable;
        htmltable.createRow() << "<td>Operating system</td><td>" << usysinfo.sysname <<"</td>";
        htmltable.createRow() << "<td>Release Version</td><td>" << usysinfo.release <<"</td>";
        htmltable.createRow() << "<td>Hardware</td><td>" << usysinfo.machine <<"</td>";
        _Index << "<h2>KernelInfo:</h2>" << htmltable.getTable();
        #endif
    }
    
    void CPUInfo(){
        libhttppp::CpuInfo cpuinfo;
        _Index << "<h2>CPUInfo:</h2>";
        HtmlTable cputable;
        cputable.createRow() << "<td>Cores</td><td>" << cpuinfo.getCores()<<"</td>";
        cputable.createRow() << "<td>Running on Thread</td><td>"<< cpuinfo.getActualThread()<<"</td>";
        cputable.createRow() << "<td>Threads</td><td>" << cpuinfo.getThreads()<<"</td>";
        _Index << cputable.getTable();
    }
    
    void SysInfo(){
        libhttppp::SysInfo sysinfo;
        _Index << "<h2>SysInfo:</h2>";
        HtmlTable cputable;
        cputable.createRow() << "<td>Total Ram</td><td>" << sysinfo.getTotalRam()<<"</td>";
        cputable.createRow() << "<td>Free Ram</td><td>" << sysinfo.getFreeRam()<<"</td>";
        cputable.createRow() << "<td>Buffered Ram</td><td>" <<sysinfo.getBufferRam()<<"</td>";
        _Index << cputable.getTable();   
    }
    
    void FsInfo(){
        libhttppp::FsInfo fsinfo;
        _Index << "<h2>FsInfo:</h2>";
        HtmlTable fstable;
        for(libhttppp::MountPoint *curm=fsinfo.getFirstDevice(); curm; curm=curm->nextMountPoint()){
            fstable.createRow() << "<td>Device</td><td>" << curm->getDevice() <<"</td>";
        }
        _Index << fstable.getTable();
    }
    
    const char *getIndexPage(){
        delete[] _Buffer;
        _Buffer = new char[_Index.size()+1];
        std::copy(_Index.cbegin(),_Index.cend(),_Buffer);
        _Buffer[_Index.size()]='\0';
        return _Index.data();
    }
    
    size_t getIndexPageSize(){
        return _Index.size();
    }
    
private:
    char              *_Buffer;
    char              *_BufferSize; 
    std::string        _Index;
};

class Controller : public libhttppp::Event {
public:
    Controller(libhttppp::ServerSocket* serversocket) : Event(serversocket){
        
    };
    
    void IndexController(libhttppp::Connection *curcon){
        try{
            libhttppp::HttpRequest curreq;
            curreq.parse(curcon);
            const char *cururl=curreq.getRequestURL();
            std::cerr << "Requesturl: " << cururl << "\n";
            libhttppp::HttpResponse curres;
            curres.setState(HTTP200);
            curres.setVersion(HTTPVERSION(1.1));
            if(strncmp(cururl,"/", libhttppp::getlen(cururl))==0){
                curres.setContentType("text/html");
                IndexPage idx;
                curres.send(curcon,idx.getIndexPage(),idx.getIndexPageSize());
            }else if(strncmp(cururl,"/images/header.png", libhttppp::getlen(cururl))==0){
                curres.setContentType("image/png");
                curres.setContentLength(header_png_size);
                curres.send(curcon,(const char*)header_png,header_png_size);
            }else if(strncmp(cururl,"/favicon.ico ",libhttppp::getlen(cururl))==0){
                curres.setContentType("image/ico");
                curres.setContentLength(favicon_ico_size);
                curres.send(curcon,(const char*)favicon_ico,favicon_ico_size);
            }else{
                curres.setState(HTTP404);
                curres.send(curcon,NULL,0);
            }
        }catch(libhttppp::HTTPException &e){
            throw e;
        }
    }
    
    void RequestEvent(libhttppp::Connection *curcon){
        try{
            IndexController(curcon);
        }catch(libhttppp::HTTPException &e){
            std::cerr << e.what() << "\n";
            throw e;
        }
    }
private:
    
};



class HttpConD : public libhttppp::HttpD {
public:
    HttpConD(int argc, char** argv) : HttpD(argc,argv){
        libhttppp::HTTPException httpexception;
        try {
            Controller controller(getServerSocket());
            controller.runEventloop();
        }catch(libhttppp::HTTPException &e){
            std::cerr << e.what() << "\n";
        }
    };
private:
};

int main(int argc, char** argv){
    try{
        HttpConD(argc,argv);
    }catch(...){};
}
