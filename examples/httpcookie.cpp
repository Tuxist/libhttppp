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

#include <systempp/sysutils.h>

#include "htmlpp/html.h"

#include "../src/exception.h"

#include "http.h"
#include "httpd.h"


class CookieTest {
public:
    CookieTest(libhttppp::Connection *curcon,libhttppp::HttpRequest *curreq){
        _Curcon=curcon;
        _Curreq=curreq;
        
        _Curres.setState(HTTP200);
        _Curres.setVersion(HTTPVERSION(1.1));
        _Curres.setContentType("text/html"); 
        _HTMLDat  << "<!DOCTYPE HTML>"
        << " <html>"
        << "  <head>"
        << "    <title>CookieTest</title>"
        << "    <meta content=\"\">"
        << "    <meta charset=\"utf-8\">"
        << "    <style></style>"
        << "  </head>"
        << "<body>";
        setCookieForm();
        parseCookie();
        _HTMLDat  << "</body></html>";
        _Curres.send(_Curcon,_HTMLDat.c_str(),_HTMLDat.size());
    };
    
    const char *getData(const char *key){
        libhttppp::HttpForm curform;
        curform.parse(_Curreq);
        if(curform.getUrlcodedFormData()){
            for(libhttppp::HttpForm::UrlcodedFormData *cururlform=curform.getUrlcodedFormData(); cururlform; 
                cururlform=cururlform->nextUrlcodedFormData()){
                if(sys::ncompare(key,sys::getlen(key),cururlform->getKey(),sys::getlen(key))==0)
                    return cururlform->getValue();
                }
        }
        return NULL;
    }
    
    int getDataInt(const char *key){
        libhttppp::HttpForm curform;
        curform.parse(_Curreq);
        if(curform.getUrlcodedFormData()){
            for(libhttppp::HttpForm::UrlcodedFormData *cururlform=curform.getUrlcodedFormData(); cururlform; 
                cururlform=cururlform->nextUrlcodedFormData()){
                if(sys::ncompare(key,sys::getlen(key),cururlform->getKey(),
                    sys::getlen(cururlform->getKey()))==0){
                        char ktmp[255];
                        sys::scopy(cururlform->getValue(),cururlform->getValue()+
                                         sys::getlen(cururlform->getValue()),
                                         ktmp);
                        return sys::atoi(ktmp);
                    }
                }
        }
        return 0;
    }
    
    void setCookieForm(){
        _HTMLDat << "<div style=\"border: thin solid black\"><span>Set Httpcookie</span></br>"
        << "<form action=\"/\" method=\"post\">"
        << "Key:<br> <input type=\"text\" name=\"key\" value=\"key\"><br>"
        << "Value:<br> <input type=\"text\" name=\"value\" value=\"value\"><br>"
        << "Comment:<br> <input type=\"text\" name=\"comment\" value=\"\"><br>"
        << "Domain:<br> <input type=\"text\" name=\"domain\" value=\"\"><br>"
        << "Max-Age:<br> <input type=\"text\" name=\"max-age\" value=\"-1\"><br>"
        << "Path:<br> <input type=\"text\" name=\"path\" value=\"\"><br>"
        << "Secure:<br> <input type=\"text\" name=\"secure\" value=\"\"><br>"
        << "Version:<br> <input type=\"text\" name=\"version\" value=\"1\"><br>"
        << "<button type=\"submit\">Submit</button>"
        << "</form>";
        
        _HTMLDat << "</div></br>";
        _Cookie.setcookie(&_Curres,getData("key"),getData("value"),getData("comment"),getData("domain"),getDataInt("max-age"),getData("path"),false,getData("version"));
    };
    
    void parseCookie() {   
        _Cookie.parse(_Curreq);
        _HTMLDat  << "<div style=\"border: thin solid black\"><span>Httpcookie's</span></br>";
        for(libhttppp::HttpCookie::CookieData *curcookie=_Cookie.getfirstCookieData(); 
            curcookie; curcookie=curcookie->nextCookieData()){
            _HTMLDat  << "key: " << curcookie->getKey() << " ";
        _HTMLDat  << "value: " << curcookie->getValue() << "</br>";
            }
            _HTMLDat << "</div></br>";
    };
    
private:
    libhtmlpp::HtmlString  _HTMLDat;
    libhttppp::HttpCookie   _Cookie;
    libhttppp::HttpResponse _Curres;
    libhttppp::Connection  *_Curcon;
    libhttppp::HttpRequest *_Curreq;
};

class Controller : public libhttppp::Event {
public:
    Controller(sys::ServerSocket* serversocket) : Event(serversocket){
        
    };
    void RequestEvent(libhttppp::Connection *curcon){

        try{
            std::cout << "Parse Request" << std::endl;
            libhttppp::HttpRequest curreq;
            curreq.parse(curcon);
            std::cout << "Send answer" << std::endl;
            CookieTest(curcon,&curreq);
        }catch(libhttppp::HTTPException &e){
            std::cerr << e.what() << std::endl;
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
            std::cerr << e.what() << std::endl;
        }
    };
private:
};

int main(int argc, char** argv){
    HttpConD(argc,argv);
} 
