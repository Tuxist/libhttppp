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

#include <iostream>
#include <netplus/exception.h>

#include <htmlpp/html.h>

#include "../src/exception.h"

#include "http.h"
#include "httpd.h"


class Controller : public libhttppp::HttpEvent {
public:
    Controller(netplus::socket* serversocket) : HttpEvent(serversocket){
        
    };

    void RequestEvent(libhttppp::HttpRequest* curreq , const int tid,void *args){
            libhtmlpp::HtmlString formdat;
            try {
                Multiform(curreq, formdat);
                URlform(curreq, formdat);
                std::cout << "Send answer" << std::endl;
                sendResponse(curreq, formdat);
            }
            catch (libhttppp::HTTPException& e) {
                if (e.getErrorType() != libhttppp::HTTPException::Note) {
                    libhttppp::HttpResponse curres;
                    curres.setState(HTTP500);
                    curres.setVersion(HTTPVERSION(2.0));
                    curres.setContentLength(0);
                    curres.send(curreq, e.what(), strlen(e.what()));
                }
            }
    }
private:
    void Multiform(libhttppp::HttpRequest* curreq, libhtmlpp::HtmlString& condat) {
        libhttppp::HttpForm curform;
        curform.parse(curreq);

        if (curform.getBoundary()) {
            condat << "Boundary: " << curform.getBoundary() << "<br>";
            for (libhttppp::HttpForm::MultipartForm::Data* curformdat = curform.MultipartFormData.getFormData();
                 curformdat; curformdat = curformdat->nextData()) {
                condat << "Content-Disposition: <br>";
                for(libhttppp::HttpForm::MultipartForm::Data::ContentDisposition *curdispo=curformdat->getDisposition();
                    curdispo; curdispo=curdispo->nextContentDisposition()
                ){
                        condat << "Key: "
                               << curdispo->getKey()
                               << " Value: "
                               << curdispo->getValue();
                }
                condat << "<br>";
                if (!curformdat->Value.empty()){
                    condat.append(curformdat->Value.data(),curformdat->Value.size());
                }
                condat << "<br>";
            }
            condat << "\r\n<br></div>";
        }
    };

    void URlform(libhttppp::HttpRequest* curreq, libhtmlpp::HtmlString& condat) {
        libhttppp::HttpForm curform;
        curform.parse(curreq);

        for (libhttppp::HttpForm::UrlcodedForm::Data* cururlform = curform.UrlFormData.getFormData(); cururlform;
             cururlform = cururlform->nextData()) {
            condat << "<span>"
                   << "Key: " << cururlform->getKey()
                   << " Value: " << cururlform->getValue()
                   << "</span><br/>";
        }
    };

    void sendResponse(libhttppp::HttpRequest* curreq, libhtmlpp::HtmlString formdat) {
        libhttppp::HttpResponse curres;
        libhtmlpp::HtmlString condat;
        curres.setState(HTTP200);
        curres.setVersion(HTTPVERSION(1.1));
        curres.setContentType("text/html");
        condat << "<!DOCTYPE HTML>"
            << " <html>"
            << "  <head>"
            << "    <title>FormTest</title>"
            << "    <meta content=\"\">"
            << "    <meta charset=\"utf-8\">"
            << "    <style> </style>"
            << "  </head>"
            << "<body>"

            << "<div style=\"border: thin solid black\">"
            << "<h2>Get Request</h2>"
            << "<form action=\"/\" method=\"get\">"
            << "First name:<br> <input type=\"text\" name=\"firstname\" value=\"test\"><br>"
            << "Last name:<br>  <input type=\"text\" name=\"lastname\" value=\"test\"><br>"
            << "<button type=\"submit\">Submit</button>"
            << "</form>"
            << "</div><br/>"

            << "<div style=\"border: thin solid black\">"
            << "<h2>Post Request</h2>"
            << "<form action=\"/\" method=\"post\">"
            << "First name:<br> <input type=\"text\" name=\"firstname\" value=\"test\"><br>"
            << "Last name:<br> <input type=\"text\" name=\"lastname\" value=\"test\"><br>"
            << "<button type=\"submit\">Submit</button>"
            << "</form>"
            << "</div></br>"

            << "<div style=\"border: thin solid black\">"
            << "<h2>Post Multiform Request</h2>"
            << "<form action=\"/\" method=\"post\" enctype=\"multipart/form-data\" >"
            << "First name:<br> <input type=\"text\" name=\"firstname\" value=\"test\"><br>"
            << "Last name:<br> <input type=\"text\" name=\"lastname\" value=\"test\"><br>"
            << "<button type=\"submit\">Submit</button>"
            << "</form>"
            << "</div></br>"

            << "<div style=\"border: thin solid black\">"
            << "<h2>Post Multiform File upload</h2>"
            << "<form action=\"/\" method=\"post\" enctype=\"multipart/form-data\" >"
            << "File name:<br><input name=\"datei\" type=\"file\"><br>"
            << "<button type=\"submit\">Submit</button>"
            << "</form>"
            << "</div></br>"

            << "<div style=\"border: thin solid black\">"
            << "<h2>Post Multiform mutiple File upload</h2>"
            << "<form action=\"/\" method=\"post\" enctype=\"multipart/form-data\" >"
            << "File name:<br><input name=\"datei\" type=\"file\" multiple><br>"
            << "<button type=\"submit\">Submit</button>"
            << "</form>"
            << "</div></br>"

            << "<div style=\"border: thin solid black\">"
            << "<h2>Encoding Test</h2>"
            << "<form action=\"/\" method=\"post\">"
            << "First name:<br> <input type=\"text\" name=\"encoding\" value=\"&=\" readonly><br>"
            << "<button type=\"submit\">Submit</button>"
            << "</form>"
            << "</div></br>"
            << "<div style=\"border: thin solid black\">"
            << "<h2>Output</h2>";
        condat+=formdat;
        condat << "</div></body></html>";
        libhtmlpp::HtmlString html;
        libhtmlpp::print(condat.parse(),html);
        curres.send(curreq, html.c_str(), html.size());
    };
};


class HttpConD : public libhttppp::HttpD {
public:
  HttpConD(int argc, char** argv) : HttpD(argc,argv){
    libhttppp::HTTPException httpexception;
    try {
      Controller controller(getServerSocket());
      controller.runEventloop();
    }catch(netplus::NetException &e){
      std::cerr << e.what() << std::endl;
    }
  };
private:
};

int main(int argc, char** argv){
  HttpConD(argc,argv);
} 
