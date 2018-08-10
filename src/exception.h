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
  class Mutex;
  class HTTPException {
  public:
    
    HTTPException();
    ~HTTPException();
    
    virtual bool isNote(){
      if(CType==TNote)
        return true;    
      return false;
    }
    
    virtual bool isWarning(){
      if(CType==TWarning)
        return true;    
      return false;
    }
    
    virtual bool isError(){
      if(CType==TError)
        return true;    
      return false;
    }
    
    virtual bool isCritical(){
      if(CType==TCritical)
        return true;    
      return false;
    }
    
    virtual const char* Note(const char *desc,const char *msg=NULL){
      return ErrorTemplate(TNote,_Buffer,"HTTP Note: %s %s\r\n", desc,msg);
    }

    virtual const char* Note(const char *desc,size_t msg){
      return ErrorTemplate(TNote,_Buffer,"HTTP Note: %zu\r\n",desc, msg);
    }
    
    virtual const char* Warning(const char *desc,const char *msg=NULL){
      return ErrorTemplate(TWarning,_Buffer,"HTTP Warning: %s %s\r\n",desc, msg);
    }
    
    virtual const char* Error(const char *desc,const char *msg = NULL){
      return ErrorTemplate(TError,_Buffer,"HTTP Error: %s %s \r\n",desc, msg);
    }
  
    virtual const char* Critical(const char *desc,const char *msg = NULL){
      return ErrorTemplate(TCritical,_Buffer,"HTTP Cirtical: %s %s \r\n",desc, msg);
    }
  
    virtual const char* Critical(const char *desc, int msg) {
      return ErrorTemplate(TCritical, _Buffer, "HTTP Cirtical: %s %d \r\n", desc, msg);
    }

    virtual const char* what() const throw(){
      return _Buffer;
    }
  protected:
    template <typename Arg1,typename Arg2, typename Arg3>
    const char *ErrorTemplate(int type,char *buffer,Arg1 printstyle,Arg2 description, Arg3 message){
      CType=type;
//      std::fill(buffer,buffer+MSGLEN,NULL);
      snprintf(buffer,MSGLEN,printstyle,description,message);
      flockfile(stdout);
#ifdef DEBUG
      printf("%s\n",buffer);
#endif
      funlockfile(stdout);
      return buffer;
    }
    
    char _Buffer[MSGLEN];
    enum Type {TNote,TWarning,TError,TCritical};
    int CType;
    Mutex *_Mutex;
  };
}
#endif
