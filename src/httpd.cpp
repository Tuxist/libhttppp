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

#include <vector>
#include <fstream>
#include <iostream>

#include <netplus/socket.h>
#include <netplus/exception.h>

#include <cmdplus/cmdplus.h>

#include "httpd.h"

#define MAXDEFAULTCONN 1024

libhttppp::HttpD::HttpD(int argc, char** argv){
    HTTPDCmdController= &cmdplus::CmdController::getInstance();
    /*Register Parameters*/
    HTTPDCmdController->registerCmd("help", 'h', false, (const char*) nullptr, "Helpmenu");
    HTTPDCmdController->registerCmd("httpaddr",'a', true,(const char*) nullptr,"Address to listen");
    HTTPDCmdController->registerCmd("httpport", 'p', false, 8080, "Port to listen");
    HTTPDCmdController->registerCmd("maxconnections", 'm',false, MAXDEFAULTCONN, "Max connections that can connect");
    HTTPDCmdController->registerCmd("httpscert", 'c',false,(const char*) nullptr, "HTTPS Certfile");
    HTTPDCmdController->registerCmd("httpskey", 'k',false, (const char*) nullptr, "HTTPS Keyfile");
    /*Parse Parameters*/
    HTTPDCmdController->parseCmd(argc,argv);

    if (HTTPDCmdController->getCmdbyKey("help") && HTTPDCmdController->getCmdbyKey("help")->getFound()) {
        HTTPDCmdController->printHelp();
        throw _httpexception[HTTPException::Note] << "Help Menu printed";
    }

    if (!HTTPDCmdController->checkRequired()) {
        HTTPDCmdController->printHelp();
        _httpexception[HTTPException::Critical] << "cmd parser not enough arguments given";        throw _httpexception;
    }
      
    /*get port from console paramter*/
    int port = 0;
    bool portset=false;
    if(HTTPDCmdController->getCmdbyKey("httpport")){
        port = HTTPDCmdController->getCmdbyKey("httpport")->getValueInt();
        portset = HTTPDCmdController->getCmdbyKey("httpport")->getFound();
    }
    
    /*get httpaddress from console paramter*/
    const char *httpaddr = nullptr;
    if (HTTPDCmdController->getCmdbyKey("httpaddr"))
        httpaddr = HTTPDCmdController->getCmdbyKey("httpaddr")->getValue();
    
    /*get max connections from console paramter*/
    int maxconnections = 0;
    if (HTTPDCmdController->getCmdbyKey("maxconnections"))
        maxconnections = HTTPDCmdController->getCmdbyKey("maxconnections")->getValueInt();
    
    /*get httpaddress from console paramter*/
    const char *sslcertpath = nullptr;
    if (HTTPDCmdController->getCmdbyKey("httpscert"))
        sslcertpath = HTTPDCmdController->getCmdbyKey("httpscert")->getValue();
    
    /*get httpaddress from console paramter*/
    const char *sslkeypath = nullptr;
    if (HTTPDCmdController->getCmdbyKey("httpskey"))
        sslkeypath = HTTPDCmdController->getCmdbyKey("httpskey")->getValue();
    
    try {
        if (sslcertpath && sslkeypath) {

            std::vector<char> cert,key;

            std::ifstream keyfile(sslkeypath);

            if(keyfile.is_open()){
                while(keyfile.peek() != EOF){
                    key.push_back((char)keyfile.get());
                }
                keyfile.close();
            }

            key.push_back('\0');

            std::ifstream certfile(sslcertpath);

            if(certfile.is_open()){
                while(certfile.peek() != EOF){
                    cert.push_back((char)certfile.get());
                }
                certfile.close();
            }

            cert.push_back('\0');
            
            _ServerSocket = new netplus::ssl(httpaddr, port, maxconnections,-1,(const unsigned char*)cert.data(),
                                             cert.size(),(const unsigned char*)key.data(),key.size());
        }else{
            #ifndef Windows
            if (portset == true)
                _ServerSocket = new netplus::tcp(httpaddr, port, maxconnections,-1);
            else
                _ServerSocket = new netplus::tcp(httpaddr, maxconnections,-1);
            #else
            _ServerSocket = new netplus::tcp(httpaddr, port, maxconnections);
            #endif 
        }
    }catch (netplus::NetException &e) {
        HTTPException ee;
        ee[HTTPException::Critical] << e.what();
        throw ee;
    }
}

libhttppp::HttpD::HttpD(const char *httpaddr, int port,int maxconnections,const char *sslcertpath,const char *sslkeypath){

    try {
        if (sslcertpath && sslkeypath) {

            std::vector<char> cert,key;

            std::ifstream keyfile(sslkeypath);

            if(keyfile.is_open()){
                while(keyfile.peek() != EOF){
                    key.push_back((char)keyfile.get());
                }
                keyfile.close();
            }

            key.push_back('\0');

            std::ifstream certfile(sslcertpath);

            if(certfile.is_open()){
                while(certfile.peek() != EOF){
                    cert.push_back((char)certfile.get());
                }
                certfile.close();
            }

            cert.push_back('\0');

            _ServerSocket = new netplus::ssl(httpaddr, port, maxconnections,-1,(const unsigned char*)cert.data(),
                                             cert.size(),(const unsigned char*)key.data(),key.size());
        }else{
            #ifndef Windows
            if (port != -1)
                _ServerSocket = new netplus::tcp(httpaddr, port, maxconnections,-1);
            else
                _ServerSocket = new netplus::tcp(httpaddr, maxconnections,-1);
            #else
            _ServerSocket = new netplus::tcp(httpaddr, port, maxconnections);
            #endif
        }
    }catch (netplus::NetException &e) {
        HTTPException ee;
        ee[HTTPException::Critical] << e.what();
        throw ee;
    }
}


netplus::socket *libhttppp::HttpD::getServerSocket(){
  return _ServerSocket;
}

libhttppp::HttpD::~HttpD(){
  delete _ServerSocket;
}
