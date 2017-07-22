#include <iostream>
#include <sstream>
#include "../src/exception.h"

#include "http.h"
#include "httpd.h"

void sendResponse(libhttppp::Connection *curcon,libhttppp::HttpRequest *curreq) {
     libhttppp::HttpResponse curres;
     curres.setState(HTTP200);
     curres.setVersion(HTTPVERSION(1.1));
     curres.setContentType("text/html");
     std::stringstream condat;
     condat  << "<!DOCTYPE HTML>"
             << " <html>"
             << "  <head>"
             << "    <title>ConnectionTest</title>"
             << "    <meta content=\"\">"
             << "    <meta charset=\"utf-8\">"
             << "    <style></style>"
             << "  </head>"
             << "<body>";
     condat  << "</body></html>";
     libhttppp::HttpCookie cookie;
     cookie.setcookie(&curres,"test","test");
     cookie.setcookie(&curres,"test2","test2");
     std::string buffer=condat.str();
     curres.send(curcon,buffer.c_str(),buffer.length());
};

class Controller : public libhttppp::Queue {
public:
  Controller(libhttppp::ServerSocket* serversocket) : Queue(serversocket){
    
  };
  void RequestEvent(libhttppp::Connection *curcon){
   try{
     std::cerr << "Parse Request\n";
     libhttppp::HttpRequest curreq;
     curreq.parse(curcon);
     std::cerr << "Send answer\n";
     sendResponse(curcon,&curreq);
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
