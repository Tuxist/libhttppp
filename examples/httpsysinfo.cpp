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
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>

#include <http.h>
#include <httpd.h>
#include <exception.h>

#include <netplus/eventapi.h>


#include "htmlpp/exception.h"
#include "htmlpp/html.h"

#include "header_png.h"
#include "favicon_ico.h"

#ifndef Windows
#include <sys/utsname.h>
#include <sys/resource.h>
#endif // !Windows

#define INDEXPAGE "<!DOCTYPE html><html lang=\"en\" ><head><title>sysinfo</title></head><body style=\"color:rgb(239, 240, 241); background:rgb(79, 83, 88);\">\
<div id=\"mainbar\" style=\"border-radius: 38px; background:rgb(35, 38, 41); width:1280px; margin:0px auto;\">\
<div id=\"headerimage\"><img alt=\"headerimage\" src=\"images/header.png\"></div>\
<div id=\"sysinfo\" style=\"padding:40px 20px;\"><span class=\"sysheader\">System Info:</span><br>\
</div></div></body></html>"

libhtmlpp::HtmlPage     SysPage;
libhtmlpp::HtmlElement *RootNode;

class Sysinfo{
public:
    void TimeInfo(libhtmlpp::HtmlElement &index) {
        time_t t = time(0);
        tm* mytime = localtime(&t);
        libhtmlpp::HtmlString html;
        html << "<div>"
             << "<span>Date & Time:</span><br/>"
             << "<span>Date:" << mytime->tm_mday << "."
                              << mytime->tm_mon << "."
                              << mytime->tm_year << " </span>"
                              << "<span>Time:" << mytime->tm_hour << ":"
                              << mytime->tm_min << ":"
                              << mytime->tm_sec << "</span>"
            << "</div>";
        libhtmlpp::HtmlElement *sysdiv=nullptr;
        sysdiv=index.getElementbyID("sysinfo");
        if(sysdiv)
            sysdiv->appendChild(html.parse());
    }
    
    void KernelInfo(libhtmlpp::HtmlElement &index){
        #ifndef Windows
        struct utsname usysinfo;
        uname(&usysinfo);
        /*create htmltable widget*/
        libhtmlpp::HtmlTable htmltable;

        /*create table rows*/
        htmltable << libhtmlpp::HtmlTable::Row() << "Operating system:" << usysinfo.sysname;
        htmltable << libhtmlpp::HtmlTable::Row() << "Release Version :" << usysinfo.release;
        htmltable << libhtmlpp::HtmlTable::Row() << "Hardware        :" << usysinfo.machine;

        libhtmlpp::HtmlElement table;
        /*convert htmltable to dom HtmlElement*/
        htmltable.insert(&table);
        /*set css class for the table*/
        table.setAttribute("class","kinfo");

        /*HtmlString Accepts Html Raw input*/
        libhtmlpp::HtmlString html;
        html << "<div><span>KernelInfo:</span> <br/> </div> ";
        /*convert Htmlstring to Html Dom element*/
        libhtmlpp::HtmlElement *div=html.parse();

        /*append table to dom element div*/
        div->appendChild(&table);

        /* search HtmlElement with id sysinfo */
        libhtmlpp::HtmlElement *sysdiv=index.getElementbyID("sysinfo");
        if(sysdiv)
            /*append table to dom element sysdiv*/
            sysdiv->appendChild(div);

        #endif
    }

};

class Controller : public libhttppp::HttpEvent {
public:
    Controller(netplus::socket* serversocket) : HttpEvent(serversocket){
        
    };
    
    void IndexController(libhttppp::HttpRequest *curreq){
        try{
            const char *cururl=curreq->getRequestURL();
            libhttppp::HttpResponse curres;
            curres.setState(HTTP200);
            curres.setVersion(HTTPVERSION(2.0));
            std::cout << cururl << std::endl;
            if(strncmp(cururl,"/",strlen(cururl))==0){
                curres.setContentType("text/html");
                libhtmlpp::HtmlString html;
                libhtmlpp::HtmlElement index;
                index=RootNode;
                Sysinfo sys;
                sys.TimeInfo(index);
                sys.KernelInfo(index);
                libhtmlpp::print(&index,html);
                curres.send(curreq,html.c_str(),html.size());
            }else if(strncmp(cururl,"/images/header.png",18)==0){
                curres.setContentType("image/png");
                curres.send(curreq,(const char*)header_png,header_png_size);
            }else if(strncmp(cururl,"/favicon.ico ",12)==0){
                curres.setContentType("image/ico");
                curres.send(curreq,(const char*)favicon_ico,favicon_ico_size);
            }else{
                curres.setState(HTTP404);
                curres.send(curreq,nullptr,0);
            }
        }catch(libhttppp::HTTPException &e){
            std::cerr << e.what() << std::endl;
        }
    }
    /*virtual method from event will call for every incomming Request*/
    void RequestEvent(libhttppp::HttpRequest* curreq, const int tid,void *args){
        try{
            /*self implemented controller see above*/
            IndexController(curreq);
        }catch(libhttppp::HTTPException &e){
            std::cerr << e.what() <<std::endl;
        }
    }
private:

};



class HttpConD : public libhttppp::HttpD {
public:
    HttpConD(int argc, char** argv) : HttpD(argc,argv){
        Controller controller(getServerSocket());
        controller.runEventloop();
    };
};

int main(int argc, char** argv){
    try{
        /*loads Macro INDEXPAGE into class HtmlPage for parsing and
         *return the first element from indexpage normaly <!DOCTYPE html>
         */
        RootNode=SysPage.loadString(INDEXPAGE);
        /*parse cmd for port , bind address and etc ...
         *after that mainloop will started so you
         * to send interuppt signal if you want running the code afterwards
         */
        HttpConD(argc,argv);
        return 0;
    }catch(libhttppp::HTTPException &e){
        std::cerr << e.what() <<std::endl;
        if(e.getErrorType()==libhttppp::HTTPException::Note 
            || libhttppp::HTTPException::Warning)
            return 0;
        else
            return -1;
    };
}
