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
#include <signal.h>

#include <netplus/socket.h>
#include <netplus/exception.h>

#include "http.h"
#include "exception.h"

int main(int argc, char** argv){
    signal(SIGPIPE, SIG_IGN);
    try{
        netplus::tcp srvsock(argv[1],atoi(argv[2]),1,0);
        netplus::socket *cltsock=srvsock.connect();

        libhttppp::HttpRequest req;
        try{
            req.setRequestType(GETREQUEST);
            req.setRequestURL(argv[3]);
            req.setRequestVersion(HTTPVERSION(1.1));
            *req.setData("connection") << "keep-alive";
            *req.setData("host") << argv[1] << ":" << argv[2];
            *req.setData("Accept") << "text/html";
            *req.setData("user-agent") << "libhttppp/1.0 (Alpha Version 0.1)";
            req.send(cltsock,&srvsock);
        }catch(libhttppp::HTTPException &e){
            std::cerr << e.what() << std::endl;
            return -1;
        }

        char data[16384];
        int len=cltsock->recvData(&srvsock,data,16384);

        std::string html;
        try {
          libhttppp::HttpResponse res;
          size_t hsize=res.parse(data,len);
          size_t amount = len-hsize;

          if(amount>0)
            html.assign(data+hsize,amount);

          size_t rlen;

          try{
            rlen=res.getContentLength();
          }catch(...) {
            rlen=amount;
          }

          while(amount < rlen){
            try{
                size_t recv=cltsock->recvData(&srvsock,data,16384);
                amount+=recv;
                html.append(data,recv);
            }catch(netplus::NetException &e){
                std::cerr << e.what() << std::endl;
            }
          }
        }catch(libhttppp::HTTPException &e){
            std::cerr << e.what() << std::endl;
        };
        if(!html.empty())
          std::cout << html << std::endl;
        delete cltsock;
    }catch(netplus::NetException &exp){
        std::cerr << exp.what() <<std::endl;
        return -1;
    }
}
