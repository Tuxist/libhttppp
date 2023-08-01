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

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>

#include <http.h>
#include <httpd.h>
#include <exception.h>

#include <netplus/eventapi.h>


#include "htmlpp/exception.h"
#include "htmlpp/html.h"

#include "header_png.h"
#include "favicon_ico.h"

#ifndef Windows
#include <sys/utsname.h>
#include <sys/resource.h>
#endif // !Windows

class HtmlTable{
public:
    HtmlTable(const char *id=NULL){
        _firstRow=nullptr;
        _lastRow=nullptr;
        _Id=id;
    }
    
    ~HtmlTable(){
        delete _firstRow;
    }
    
    class Row {
    public:
        Row &operator<<(const char *value){
            _Data+=value;
            return *this;
        };
        
        Row &operator<<(int value){
            char buf[255];
            snprintf(buf,255,"%d",value);
            return *this << buf;
        };
        
    private:
        std::string  _Data;
        int          _Size;
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
    
    void getTable(std::string &table){
        _Table.clear();
        if(_Id!=NULL)
            _Table << "<table id=\"" << _Id << "\">";
        else
            _Table << "<table>";
        for(Row *curow=_firstRow; curow; curow=curow->_nextRow){
            _Table << "<tr>";
            _Table << curow->_Data;
            _Table << "</tr>";
        }
        _Table << "</table>";
        table+=_Table.str();
    }

private:
    std::stringstream     _Table;
    const char           *_Id;
    Row                  *_firstRow;
    Row                  *_lastRow;
};

class HtmlContent{
    void generateHeader(){
        
    }
};


class IndexPage{
public:
    IndexPage(){
        _Index << "<!DOCTYPE html><html lang=\"en\" ><head><title>sysinfo</title> </head><body style=\"color:rgb(239, 240, 241); background:rgb(79, 83, 88);\">"
        << "<div id=\"mainbar\" style=\"border-radius: 38px; background:rgb(35, 38, 41); width:1280px; margin:0px auto;\">"
        << "<div id=\"headerimage\"><img alt=\"headerimage\" src=\"images/header.png\"></div>"
        << "<div id=\"sysinfo\" style=\"padding:40px 20px;\"><span>System Info:</span><br>";
        TimeInfo();
        KernelInfo();
        _Index << "</div></div></body></html>";
    }
    
    void TimeInfo() {
        std::time_t t = std::time(0);
        std::tm* mytime = std::localtime(&t);
        _Index << "<h2>Date & Time:</h2>";
        _Index << "<span>Date:" << (unsigned long)mytime->tm_mday << "."
                                << (unsigned long)mytime->tm_mon << "."
                                << (unsigned long)mytime->tm_year << " </span>"
                                << "<span>Time:" << (unsigned long)mytime->tm_hour << ":"
                                << (unsigned long)mytime->tm_min << ":"
                                << (unsigned long)mytime->tm_sec << "</span>";
    }
    
    void KernelInfo(){
        #ifndef Windows
        struct utsname usysinfo;
        uname(&usysinfo);
        HtmlTable htmltable;
        htmltable.createRow() << "<td>Operating system</td><td>" << usysinfo.sysname <<"</td>";
        htmltable.createRow() << "<td>Release Version</td><td>" << usysinfo.release <<"</td>";
        htmltable.createRow() << "<td>Hardware</td><td>" << usysinfo.machine <<"</td>";
        _Index << "<h2>KernelInfo:</h2>";
        std::string table;
        htmltable.getTable(table);
        _Index << table;
        #endif
    }
    
    void getIndexPage(std::string &index){
        index=_Index.str();
    }
    
private:
    std::stringstream  _Index;
};

class Controller : public netplus::event {
public:
    Controller(netplus::socket* serversocket) : event(serversocket){
        
    };
    
    void IndexController(netplus::con *curcon){
        try{
            libhttppp::HttpRequest curreq;
            curreq.parse(curcon);
            const char *cururl=curreq.getRequestURL();
            libhttppp::HttpResponse curres;
            curres.setState(HTTP200);
            curres.setVersion(HTTPVERSION(2.0));
            std::cout << cururl << std::endl;
            if(strncmp(cururl,"/",strlen(cururl))==0){
                curres.setContentType("text/html");
                IndexPage idx;
                std::string html;
                libhtmlpp::HtmlPage index;
                idx.getIndexPage(html);
                index.loadString(html);
                index.printHtml(html);
                curres.send(curcon,html.c_str(),html.length());
            }else if(strncmp(cururl,"/images/header.png",16)==0){
                curres.setContentType("image/png");
                curres.setContentLength(header_png_size);
                curres.send(curcon,(const char*)header_png,header_png_size);
            }else if(strncmp(cururl,"/favicon.ico ",12)==0){
                curres.setContentType("image/ico");
                curres.setContentLength(favicon_ico_size);
                curres.send(curcon,(const char*)favicon_ico,favicon_ico_size);
            }else{
                curres.setState(HTTP404);
                curres.send(curcon,NULL,0);
            }
        }catch(libhttppp::HTTPException &e){
            std::cerr << e.what() << std::endl;
        }
    }
    
    void RequestEvent(netplus::con *curcon){
        try{
            IndexController(curcon);
        }catch(libhttppp::HTTPException &e){
            std::cerr << e.what() <<std::endl;
        }
    }
private:
    
};



class HttpConD : public libhttppp::HttpD {
public:
    HttpConD(int argc, char** argv) : HttpD(argc,argv){
        Controller controller(getServerSocket());
        controller.runEventloop();
    };
};

int main(int argc, char** argv){
    try{
        HttpConD(argc,argv);
        return 0;
    }catch(libhttppp::HTTPException &e){
        std::cerr << e.what() <<std::endl;
        if(e.getErrorType()==libhttppp::HTTPException::Note 
            || libhttppp::HTTPException::Warning)
            return 0;
        else
            return -1;
    };
}
