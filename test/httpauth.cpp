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

#include <iostream>
#include <sstream>
#include "../src/exception.h"

#include "http.h"
#include "httpd.h"

class Controller : public libhttppp::Queue {
public:
  Controller(libhttppp::ServerSocket* serversocket) : Queue(serversocket){
    
  };
  void RequestEvent(libhttppp::Connection *curcon){
   try{
     std::cerr << "Parse Request\n";
     libhttppp::HttpRequest curreq;
     curreq.parse(curcon);
     const char *cururl=curreq.getRequestURL();
     if(strncmp(cururl,"/",strlen(cururl))==0){
       libhttppp::HttpResponse curres;
       curres.setState(HTTP200);
       curres.setVersion(HTTPVERSION(1.1));
       curres.setContentType("text/html");
       std::stringstream condat;
       condat  << "<!DOCTYPE HTML>"
               << " <html>"
               << "  <head>"
               << "    <title>AuthTest</title>"
               << "    <meta content=\"\">"
               << "    <meta charset=\"utf-8\">"
               << "    <style></style>"
               << "  </head>"
               << "<body>"
               << "<a href=\"/httpbasicauth\"> Basicauth </<a>"
               << "<a href=\"/httpdigestauth\"> Digestauth </<a>";
       condat  << "</body></html>";
       std::string buffer=condat.str();
       curres.send(curcon,buffer.c_str(),buffer.length());
     }else if(strncmp(cururl,"/httpbasicauth",strlen(cururl))==0 ||
              strncmp(cururl,"/httpdigestauth",strlen(cururl))==0){
       libhttppp::HttpAuth httpauth;
       httpauth.parse(&curreq);
       const char *username=httpauth.getUsername();
       const char *password=httpauth.getPassword();
       if(username && password){
           
          return;
       }else{
         libhttppp::HttpResponse curres;
         curres.setState(HTTP401);
         curres.setVersion(HTTPVERSION(1.1));
         curres.setContentType(NULL);
         if(strncmp(cururl,"/httpbasicauth",strlen(cururl))==0){
           httpauth.setAuthType(BASICAUTH);
         }else if(strncmp(cururl,"/httpdigestauth",strlen(cururl))==0){
           httpauth.setAuthType(DIGESTAUTH);
         }
         httpauth.setRealm("httpauthtest");
         httpauth.setAuth(&curres);
         curres.send(curcon,NULL,0);
         return;
       }
     }else{
       libhttppp::HttpResponse curres;
       curres.setState(HTTP404);
       curres.setVersion(HTTPVERSION(1.1));
       curres.setContentType("text/html");
       curres.send(curcon,NULL,0);
     }

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

