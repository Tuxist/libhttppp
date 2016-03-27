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

#include "socket.h"
#include "queue.h"
#include "httpdefinitions.h"
#include "exception.h"

#ifndef HTTP_H
#define HTTP_H

namespace libhttppp {

  class HttpHeader {
  public:
    class HeaderData {
    private:
      HeaderData(const char *key,const char*value);
      ~HeaderData();
      char       *_Key;
      size_t      _Keylen;
      char       *_Value;
      size_t      _Valuelen;
      HeaderData *_nextHeaderData;
      friend class HttpHeader;
    };
    void        setVersion(const char *version);
    HeaderData *setData(const char *key,const char *value);
    HeaderData *setData(const char *key,const char *value,HeaderData *pos);
    HeaderData *setData(const char* key, size_t value,HeaderData *pos=NULL);
    
    const char *getData(const char *key,HeaderData **pos=NULL);
    void        deldata(const char *key);
    
    const char *getHeader();
    size_t      getHeaderSize();
  protected:
    HttpHeader();
    ~HttpHeader();
    HeaderData *_firstHeaderData;
    HeaderData *_lastHeaderData;
    char       *_Header;
    size_t      _HeaderSize;
    char        _Headline[512];
    size_t      _HeadlineLen;
    char        _Version[255];

    size_t      _VersionLen;
    size_t      _HeaderDataSize;
    size_t      _Elements;
  };
  
  class HttpResponse : public HttpHeader {
  public:
    HttpResponse();
    ~HttpResponse();
    void setState(const char *httpstate);
    void setContentType(const char *type);
    void setContentLength(size_t len);
    void setConnection(const char *type);
    void send(Connection *curconnection,const char* data);
    void send(Connection *curconnection,const char* data, size_t datalen); //only use as server
    void parse(ClientConnection *curconnection); //only use as client
  private:
    char          _State[255];
    size_t        _Statelen;
    HeaderData   *_Connection;
    HeaderData   *_ContentType;
    HeaderData   *_ContentLength;
    HTTPException _httpexception;
  };
  
 
  class HttpRequest : public HttpHeader{
  public:
    HttpRequest();
    ~HttpRequest();
    void parse(Connection *curconnection); //only use as server
    void send(ClientConnection *curconnection); //only use as client
    
    const char    *getRequestURL();
    char          *_Buffer;
  private:
    char          *_Request;
    size_t         _Requestsize;
    int            _RequestType;
    
    char           _RequestURL[255];
    
    HttpHeader    *_HttpHeader;
    Connection    *_Connection;
    HTTPException  _httpexception;
  };
  
  class HttpForm {
  public:
    HttpForm(HttpRequest *request);
    const char *getValue(const char *key);
    void        addPair(const char *key,const char *value);
    void        clear();
  private:
    class FormData {
    private:
      FormData();
      ~FormData();
      char     *key;
      char     *value;
      FormData *nextFormData;
    };
  };

  class HttpCookie {
  private:
    class CookieData {
    private:
      CookieData();
      ~CookieData();
      char       *key;
      char       *value;
      CookieData *nextCookieData;
    };
    HttpHeader::HeaderData *_CookiePos;
  };

  
};

#endif