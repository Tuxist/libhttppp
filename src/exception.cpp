/*******************************************************************************
 * Copyright (c) 2021, Jan Koester jan.koester@gmx.net
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

#include "exception.h"

libhttppp::HTTPException::HTTPException() : sys::SystemException(){
};

libhttppp::HTTPException::HTTPException(const HTTPException &exp) : sys::SystemException(exp){
}

libhttppp::HTTPException::~HTTPException(){
}

int libhttppp::HTTPException::getErrorType(){
    return curCType; 
}

const char* libhttppp::HTTPException::what(){
    return sys::SystemException::what();
}

libhttppp::HTTPException& libhttppp::HTTPException::asign(const char *src){
    sys::SystemException::asign(src);
    return *this;   
}

libhttppp::HTTPException& libhttppp::HTTPException::operator[](int errtype){
    curCType=errtype;
    return *this;
}

libhttppp::HTTPException& libhttppp::HTTPException::operator<<(const char *src){
    return asign(src);   
};

libhttppp::HTTPException& libhttppp::HTTPException::operator<<(int src){
    sys::SystemException::operator<<(src);
    return *this;
}

