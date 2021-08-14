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

#include <connections.h>
#include <utils.h>

void testSendQueue(){
    libhttppp::Connection con;
    char buffer[]="test1 test2 test3 test4";
    con.addSendQueue(buffer,libhttppp::getlen(buffer));
    con.resizeSendQueue(6);
    char *buffer2;
    con.copyValue(con.getSendData(),0,con.getSendData(),con.getSendSize(),&buffer2);
    std::cout << "Before Resize: " << buffer << " After Resize: " << buffer2  << std::endl;
    delete[] buffer2;
    con.cleanSendData();
    
    con.addSendQueue("\0",BLOCK_SIZE*2);
    con.addSendQueue(buffer,libhttppp::getlen(buffer));
    libhttppp::ConnectionData *searchdat;
    int start = con.searchValue(con.getSendData(),&searchdat,"test3",5);
    con.copyValue(searchdat,start,con.getSendData(),con.getSendSize(),&buffer2);
    std::cout << "Searched Data:" << buffer2 << std::endl;
    delete[] buffer2;    
}

void testRecvQueue(){
    libhttppp::Connection con;
    char buffer[]="test1 test2 test3 test4";
    con.addRecvQueue(buffer,libhttppp::getlen(buffer));
    con.resizeRecvQueue(6);
    char *buffer2;
    con.copyValue(con.getRecvData(),0,con.getRecvData(),con.getRecvSize(),&buffer2);
    std::cout << "Before Resize: " << buffer << " After Resize: " << buffer2  << std::endl;
    delete[] buffer2;
    con.cleanRecvData();
    
    con.addRecvQueue("\0",BLOCK_SIZE*2);
    con.addRecvQueue(buffer,libhttppp::getlen(buffer));
    libhttppp::ConnectionData *searchdat;
    int start = con.searchValue(con.getRecvData(),&searchdat,"test3",5);
    con.copyValue(searchdat,start,con.getRecvData(),con.getRecvSize(),&buffer2);
    std::cout << "Searched Data:" << buffer2 << std::endl;
    delete[] buffer2;
}

int main(int argc, char** argv){
    testRecvQueue();
    testSendQueue();
}
