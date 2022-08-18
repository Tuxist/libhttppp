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

#include <string.h>

#include <systempp/sysconsole.h>
#include <systempp/sysutils.h>
#include <systempp/syseventapi.h>

#include "htmlpp/html.h"

#include "../src/exception.h"

#include "http.h"
#include "httpd.h"

class Controller : public sys::event {
public:
    Controller(sys::ServerSocket* serversocket) : event(serversocket){
        
    };
    
    void RequestEvent(sys::con *curcon){
        try{
            sys::cout << "Parse Request\n" << sys::endl;
            libhttppp::HttpRequest curreq;
            curreq.parse(curcon);
            const char *cururl=curreq.getRequestURL();
            if(sys::ncompare(cururl,strlen(cururl),"/",1)==0){
                libhttppp::HttpResponse curres;
                curres.setState(HTTP200);
                curres.setVersion(HTTPVERSION(1.1));
                curres.setContentType("text/html");
                libhtmlpp::HtmlString condat;
                condat  << "<!DOCTYPE HTML>"
                << " <html>"
                << "  <head>"
                << "    <title>AuthTest</title>"
                << "    <meta content=\"\">"
                << "    <meta charset=\"utf-8\">"
                << "    <style></style>"
                << "  </head>"
                << "<body><ul>"
                << "<li><a href=\"/httpbasicauth\"> Basicauth </<a></li>"
                << "<li><a href=\"/httpdigestauth\"> Digestauth </<a></li>";
                condat  << "</ul></body></html>";
                curres.send(curcon,condat.c_str(),condat.size());
            }else if(sys::ncompare(cururl,strlen(cururl),"/httpbasicauth",13)==0 ||
                sys::ncompare(cururl,strlen(cururl),"/httpdigestauth",14)==0){
                libhttppp::HttpAuth httpauth;
                httpauth.parse(&curreq);
                const char *username=httpauth.getUsername();
                const char *password=httpauth.getPassword();
                if(username && password){
                    libhttppp::HttpResponse curres;
                    curres.setState(HTTP200);
                    curres.setVersion(HTTPVERSION(1.1));
                    curres.setContentType(nullptr);
                    curres.send(curcon,nullptr,0);
                    return;
                }else{
                    libhttppp::HttpResponse curres;
                    curres.setState(HTTP401);
                    curres.setVersion(HTTPVERSION(1.1));
                    curres.setContentType(nullptr);
                    if(sys::ncompare(cururl,strlen(cururl),"/httpbasicauth",13)==0){
                        httpauth.setAuthType(BASICAUTH);
                    }else if(sys::ncompare(cururl,strlen(cururl),"/httpdigestauth",14)==0){
                        httpauth.setAuthType(DIGESTAUTH);
                    }
                    httpauth.setRealm("httpauthtest");
                    httpauth.setAuth(&curres);
                    curres.send(curcon,nullptr,0);
                    return;
                }
            }else{
                libhttppp::HttpResponse curres;
                curres.setState(HTTP404);
                curres.setVersion(HTTPVERSION(1.1));
                curres.setContentType("text/html");
                curres.send(curcon,nullptr,0);
            }                
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

