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

#include <netplus/socket.h>
#include <netplus/eventapi.h>

#include "http.h"
#include "exception.h"

#pragma once

namespace cmdplus {
    class CmdController;    
}

namespace libhttppp {

    class HttpEvent : public netplus::event{
    public:
        HttpEvent(netplus::socket *ssock);
        void CreateConnetion(netplus::con **curon);
        void deleteConnetion(netplus::con *curon);

        virtual void RequestEvent(HttpRequest *curreq,const int tid,void *args);
        virtual void ResponseEvent(HttpRequest *curreq,const int tid,void *args);
        virtual void ConnectEvent(HttpRequest *curreq,const int tid,void *args);
        virtual void DisconnectEvent(HttpRequest *curreq,const int tid,void *args);

    private:
        void RequestEvent(netplus::con *curcon,const int tid,void *args);
        void ResponseEvent(netplus::con *curcon,const int tid,void *args);
        void ConnectEvent(netplus::con *curcon,const int tid,void *args);
        void DisconnectEvent(netplus::con *curcon,const int tid,void *args);

    };

    class HttpD {
    public:
        HttpD(int argc, char** argv);
        HttpD(const char *httpaddr, int port,int maxconnections,const char *sslcertpath,const char *sslkeypath);
        ~HttpD();
        netplus::socket            *getServerSocket();
    protected:
        void                        FileServer();
        cmdplus::CmdController     *HTTPDCmdController;
    private:
        bool                        _fileServer;
        netplus::socket            *_ServerSocket;
        HTTPException               _httpexception;
    };
};
