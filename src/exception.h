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

#include <config.h>
#include <cstdio>

/*
* Error Statas
* Note: Notice only for Information
* Warning: Could be ending in an error
* Error: Error happend possible closed Client Connection
* Critical: Some happend that will shutdown your Apllication
*/

#ifndef EXCEPTION_H
#define EXCEPTION_H

#ifdef DEBUG

#endif

#define MSGLEN 255

namespace libhttppp {
  class HTTPException {
  public:
    
    HTTPException(){
      _Note=false;
      _Warning=false;
      _Error=false;
      _Critical=false;
    }
    
    virtual bool isNote(){
      return _Note;
    }
    
    virtual bool isWarning(){
      return _Warning;
    }
    
    virtual bool isError(){
      return _Error;
    }
    
    virtual bool isCritical(){
      return _Critical;
    }
    
    virtual const char* Note(const char *msg){
      return ErrorTemplate(&_Note,_Buffer,"HTTP Note: %s\r\n",msg);
    }

    virtual const char* Note(size_t msg){
      return ErrorTemplate(&_Note,_Buffer,"HTTP Note: %zu\r\n",msg);
    }
    
    virtual const char* Warning(const char *msg){
      return ErrorTemplate(&_Note,_Buffer,"HTTP Warning: %s\r\n",msg);
    }
  
    virtual const char* Error(const char *msg){
      return ErrorTemplate(&_Note,_Buffer,"HTTP Error: %s\r\n",msg);
    }
  
    virtual const char* Cirtical(const char *msg){
      return ErrorTemplate(&_Note,_Buffer,"HTTP Cirtical: %s\r\n",msg);
    }
  
    virtual const char* what() const throw(){
      return _Buffer;
    }
  protected:
    template <typename Arg1,typename Arg2>
    const char *ErrorTemplate(bool *type,char *buffer,Arg1 printstyle,Arg2 message){
      *type=true;
//      std::fill(buffer,buffer+MSGLEN,NULL);
      snprintf(buffer,MSGLEN,printstyle,message);
#ifdef DEBUG
      printf("%s",buffer);
#endif
      return buffer;
    }
    char _Buffer[MSGLEN];
    bool _Note;
    bool _Warning;
    bool _Error;
    bool _Critical;
  };
}
#endif
