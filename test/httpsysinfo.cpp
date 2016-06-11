
#include <http.h>
#include <httpd.h>
#include <iostream>
#include <sstream>
#include <exception.h>

#include "header_png.h"

#include <sys/utsname.h>

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
              << "<div><img src=\"images/header.png\"/></div>"
              << "<span>System Info:</span><br/>";   
    struct utsname usysinfo;
    uname(&usysinfo);
    HtmlTable htmltable;
    htmltable.createRow("Operating system:",usysinfo.sysname);
    htmltable.createRow("Release Version:",usysinfo.release);
    htmltable.createRow("Hardware:",usysinfo.machine);
    sysstream << htmltable.getTable()
              << "</body></html>";
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
  size_t _BufferSize;
};

class Controller{
public:
  Controller(libhttppp::Connection *curcon){
    try{
      libhttppp::HttpRequest curreq;
      curreq.parse(curcon);
      const char *cururl=curreq.getRequestURL();
      std::cerr << "Requesturl: " << cururl << "\n";
      libhttppp::HttpResponse curres;
      curres.setState(HTTP200);
      curres.setVersion(HTTPVERSION(1.1));
      if(strncmp(cururl,"/images/header.png",19)==0){
        curres.setContentType("image/png");
	curres.setContentLength(header_png_size);
        curres.send(curcon,(const char*)header_png,header_png_size);
      }else{
        curres.setContentType("text/html");
        IndexPage idx;
        const char *idxpg=idx.getIndexPage();
        curres.send(curcon,idxpg,idx.getIndexPageSize());
      }
    }catch(libhttppp::HTTPException &e){
      throw e;
    }
  }
};

void libhttppp::Queue::RequestEvent(libhttppp::Connection *curcon){
   try{
     Controller cntl(curcon);
   }catch(HTTPException &e){
     std::cerr << e.what() << "\n";
     throw e;
   }
}

class HttpConD : public libhttppp::HttpD {
public:
  HttpConD(int argc, char** argv) : HttpD(argc,argv){
    libhttppp::HTTPException httpexception;
    try {
      runDaemon();
    }catch(libhttppp::HTTPException &e){
      std::cerr << e.what() << "\n";
    }
  };
private:
};

int main(int argc, char** argv){
  HttpConD(argc,argv);
}
