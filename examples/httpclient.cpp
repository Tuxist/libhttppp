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
#include <signal.h>

#include <netplus/socket.h>
#include <netplus/exception.h>

#include "http.h"
#include "exception.h"

#include <cstring>

template<typename T = size_t>
T Hex2Int(const char* const hexstr, bool* overflow=nullptr)
{
    if (!hexstr)
        return false;
    if (overflow)
        *overflow = false;

    auto between = [](char val, char c1, char c2) { return val >= c1 && val <= c2; };
    size_t len = strlen(hexstr);
    T result = 0;

    for (size_t i = 0, offset = sizeof(T) << 3; i < len && (int)offset > 0; i++)
    {
        if (between(hexstr[i], '0', '9'))
            result = result << 4 ^ (hexstr[i] - '0');
        else if (between(tolower(hexstr[i]), 'a', 'f'))
            result = result << 4 ^ (tolower(hexstr[i]) - ('a' - 10)); // Remove the decimal part;
        offset -= 4;
    }
    if (((len + ((len % 2) != 0)) << 2) > (sizeof(T) << 3) && overflow)
        *overflow = true;
    return result;
}

size_t readchunk(const char *data,size_t datasize,size_t &pos){
  size_t start=pos;
  while( (pos < datasize || pos < 512)&& data[pos++]!='\r');
  char value[512];

  if(pos-start > 512){
      return 0;
  }

  memcpy(value,data+start,pos-start);

  ++pos;

  return Hex2Int(value,nullptr);
}

int main(int argc, char** argv){
  signal(SIGPIPE, SIG_IGN);
  netplus::socket *cltsock=nullptr;
  try{
    netplus::tcp srvsock(argv[1],atoi(argv[2]),1,0);
    cltsock=srvsock.connect();

    try{
      libhttppp::HttpRequest req;
      req.setRequestType(GETREQUEST);
      req.setRequestURL(argv[3]);
      req.setRequestVersion(HTTPVERSION(1.1));
      *req.setData("connection") << "keep-alive";
      *req.setData("host") << argv[1] << ":" << argv[2];
      *req.setData("accept") << "text/html";
      *req.setData("user-agent") << "libhttppp/1.0 (Alpha Version 0.1)";
      req.send(cltsock,&srvsock);
    }catch(libhttppp::HTTPException &e){
      std::cerr << e.what() << std::endl;
      return -1;
    }

    char data[16384];
    size_t recv=cltsock->recvData(&srvsock,data,16384);

    std::string html;
    libhttppp::HttpResponse res;
    size_t len=recv,chunklen=0,hsize=0;

    int rlen=0;

    try {
      hsize=res.parse(data,len);
      if(strcmp(res.getTransferEncoding(),"chunked")==0){
          chunklen=readchunk(data,recv,--hsize);
      }else{
         rlen=res.getContentLength();
         html.resize(rlen);
      }
    }catch(libhttppp::HTTPException &e){
      std::cerr << e.what() << std::endl;
    };

    if(chunklen==0){
        do{
            html.append(data+hsize,recv-hsize);
            rlen-=recv-hsize;
            if(rlen>0){
              recv=cltsock->recvData(&srvsock,data,16384);
              hsize=0;
            }
        }while(rlen>0);
    }else{
        size_t cpos=hsize;
        do{
            html.append(data+cpos,chunklen);
            cpos+=chunklen;
            if(recv<chunklen){
              recv=cltsock->recvData(&srvsock,data,16384);
              cpos=0;
            }
        }while((chunklen=readchunk(data,recv,cpos))!=0);
    }
    delete cltsock;
    if(!html.empty())
      std::cout << html << std::endl;
    return 0;
  }catch(netplus::NetException &exp){
    delete cltsock;
    std::cerr << exp.what() <<std::endl;
    return -1;
  }
}
