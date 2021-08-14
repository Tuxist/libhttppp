/*******************************************************************************
Copyright (c) 2021, Jan Koester jan.koester@gmx.net
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

#include "connections.h"
#include <iostream>
#include <assert.h>

#define SENDDATA "6a3a08f0-f84f-11eb-b83a-97319b38c7ae"

libhttppp::Connection testcon;

void addsenddata(int amount){
    for(int i = 0; i<amount; ++i){
        testcon.addSendQueue(SENDDATA,46);
    }
    const char *dat=testcon.getSendData()->getData();
    for(int i = 0; i<46; ++i){
        if(SENDDATA[i]!=dat[i])
            assert("data failed");
    }
    std::cout << "addsenddata ok!" << std::endl;
}

void resizeSendQueue(int size){
    testcon.resizeSendQueue(size);
    const char *dat=testcon.getSendData()->getData();
    int ii=4;
    for(int i = 0; i<42; ++i){
        if(SENDDATA[ii]!=dat[i++])
            assert("data failed");
    }
    std::cout << "resizeSendQueue ok!" << std::endl;
}

void resizeval(){
    addsenddata(1);
    resizeSendQueue(4);
    addsenddata(1);
    resizeSendQueue(4);
    std::cout<<testcon.getSendSize()<<std::endl;
};

int main(int argc,char *argv[]){
    resizeval();
}
