/*******************************************************************************
Copyright (c) 2014, Jan Koester jan.koester@gmx.net
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

#include <http.h>
#include <httpd.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <exception.h>

#include "header_png.h"
#include "favicon_ico.h"
#ifndef Windows
#include <sys/utsname.h>
#endif // !Windows


class HtmlTable{
public:
  HtmlTable(const char *id=NULL){
     if(id!=NULL)
       tdat << "<table id=\"" << id << "\">";
     else
       tdat << "<table>";
  }

  ~HtmlTable(){
  }

  void createRow(const char* key,const char *value){
    tdat << "<tr><td>" << key << "</td><td>" << value <<"</td></tr>";
  }

  void createRow(const char* key,int value){
    tdat << "<tr><td>" << key << "</td><td>" << value <<"</td></tr>";
  }
  
  const char *getTable(){
    tdat << "</table>";
    buffer=tdat.str();
    return buffer.c_str();
  }

private:
  std::string       buffer;
  std::stringstream tdat;
  ssize_t pos;
};

class HtmlContent{
  void generateHeader(){
    
  }
};


class IndexPage{
public:
  IndexPage(){
    sysstream << "<!DOCTYPE html><body>"
              << "<div id=\"mainbar\" style=\"width:1280px; margin:0px auto;\">"
              << "<div id=\"headerimage\"><img src=\"images/header.png\"/></div>"
              << "<div id=\"sysinfo\"><span>System Info:</span><br/>"; 
    KernelInfo();
    CPUInfo();
    sysstream << "</div></div></body></html>";
              
  }
  
  void KernelInfo(){
#ifndef Windows
      struct utsname usysinfo;
      uname(&usysinfo);
      HtmlTable htmltable;
      htmltable.createRow("Operating system ",usysinfo.sysname);
      htmltable.createRow("Release Version  ",usysinfo.release);
      htmltable.createRow("Hardware ",usysinfo.machine);
      sysstream << "<h2>KernelInfo:</h2>" << htmltable.getTable();
#endif
  }
  
  void CPUInfo(){
#ifndef Windows
    sysstream << "<h2>CPUInfo:</h2>";
    HtmlTable cputable;
    int cpus = sysconf(_SC_NPROCESSORS_ONLN);
    cputable.createRow("Cores ",cpus);
    sysstream << cputable.getTable();
#endif      
  }
  
  const char *getIndexPage(){
    _Buffer=sysstream.str();
    return _Buffer.c_str();
  }
  
  size_t getIndexPageSize(){
    return _Buffer.size();
  }
  
private:
  std::string       _Buffer;
  std::stringstream sysstream;
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
      if(strncmp(cururl,"/",strlen(cururl))==0){
        curres.setContentType("text/html");
        IndexPage idx;
        const char *idxpg=idx.getIndexPage();
        curres.send(curcon,idxpg,idx.getIndexPageSize());
      }else if(strncmp(cururl,"/images/header.png",strlen(cururl))==0){
        curres.setContentType("image/png");
	curres.setContentLength(header_png_size);
        curres.send(curcon,(const char*)header_png,header_png_size);
      }else if(strncmp(cururl,"/favicon.ico ",strlen(cururl))==0){
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
  HttpConD(argc,argv);
}
