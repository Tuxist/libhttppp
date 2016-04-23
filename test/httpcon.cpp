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
             << "<body>"
             << "<p>Requested-Url: " << curreq->getRequestURL() << "</p>"
// 	     << "<p> Host: " << curreq->getData("Host") << "</p>"
//              << "<p>User-Agent: " << curreq->getData("User-Agent") << "</p>"
// 	     << "<p>Accept-Language: "<< curreq->getData("Accept-Language") << "</p>"
// 	     << "<p>Accept-Encoding: "<< curreq->getData("Accept-Encoding") << "</p>"
	     << "</body></html>";
     std::string buffer=condat.str();
     curres.send(curcon,buffer.c_str(),buffer.length());
};

void libhttppp::Queue::RequestEvent(libhttppp::Connection *curcon){
   try{
     std::cerr << "Parse Request\n";
     libhttppp::HttpRequest curreq;
     curreq.parse(curcon);
     std::cerr << "Send answer\n";
     sendResponse(curcon,&curreq);
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