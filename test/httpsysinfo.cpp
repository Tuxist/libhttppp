
#include <http.h>
#include <httpd.h>
#include <iostream>
#include <sstream>
#include <fstream>
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
    std::string line;
    std::ifstream cpufile ("/proc/cpuinfo");
    HtmlTable cputable;
    if (cpufile.is_open()){
      while ( getline (cpufile,line) ){
          size_t delimter=line.find(":");
          if(delimter!=std::string::npos)
             cputable.createRow(line.substr(0,delimter).c_str(),line.substr(delimter+1,(line.size()-(delimter+1))).c_str());
      }
      cpufile.close();
    }
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

class Controller : public libhttppp::Queue {
public:
  Controller(libhttppp::ServerSocket* serversocket) : Queue(serversocket){
    
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
