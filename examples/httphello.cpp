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

#include <string.h>

#include <systempp/sysconsole.h>
#include <systempp/syseventapi.h>

#include "http.h"
#include "httpd.h"

class Controller : public sys::net::event {
public:
    Controller(sys::net::socket* serversocket) : event(serversocket){
        
    };
    void RequestEvent(sys::net::con *curcon){
        try{
            libhttppp::HttpResponse curres;
            const char *hello="<!DOCTYPE html><html><head><title>hello</title></head><body>Hello World</body></html>";
            curres.setContentType("text/html");
            curres.send(curcon,hello,strlen(hello));
        }catch(libhttppp::HTTPException &e){
            sys::cerr << e.what() << sys::endl;
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
      sys::cout << e.what() << sys::endl;
    }
  };
private:
};

int main(int argc, char** argv){
  HttpConD(argc,argv);
}