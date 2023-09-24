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

#include <stddef.h>
#include <sys/types.h>

#include <string>
#include <netplus/eventapi.h>

#include "config.h"

#include "httpdefinitions.h"

#pragma once

namespace libhttppp {

  class HttpHeader {
  public:
    class HeaderData {
    public:
      HeaderData &operator<<(const char *value);
      HeaderData &operator<<(size_t value);
      HeaderData &operator<<(int value);
    protected:
      HeaderData(const char *key);
      ~HeaderData();
    private:
      std::string  _Key;
      std::string  _Value;
      HeaderData   *_nextHeaderData;
      friend class HttpHeader;
    };
    
    
    HeaderData *getfirstHeaderData();
    HeaderData *nextHeaderData(HeaderData *pos);
    const char *getKey(HeaderData *pos);
    const char *getValue(HeaderData *pos);
    
    HeaderData *setData(const char *key);
    HeaderData *setData(const char* key,HeaderData *pos);
    
    const char *getData(const char *key,HeaderData **pos=nullptr);
    size_t      getDataSizet(const char *key,HeaderData **pos=nullptr);
    int         getDataInt(const char *key,HeaderData **pos=nullptr);
    void        deldata(const char *key);
    void        deldata(HeaderData *pos);
    
    size_t      getElements();
    size_t      getHeaderSize();
  protected:
    HttpHeader();
    ~HttpHeader();
    HeaderData *_firstHeaderData;
    HeaderData *_lastHeaderData;
  };
  
  class HttpResponse : public HttpHeader {
  public:
    HttpResponse();
    ~HttpResponse();

    void   setState(const char *httpstate);
    void   setContentType(const char *type);
    void   setContentLength(size_t len);
    void   setConnection(const char *type);
    void   setVersion(const char* version);

    size_t printHeader(std::string &buffer);

    /*server methods*/
    void   send(netplus::con *curconnection,const char* data);
    void   send(netplus::con *curconnection,const char* data, unsigned long datalen); //only use as server

    /*client method*/
    void   parse(const char *data,size_t len);
  private:
    std::string      _State;
    std::string      _Version;
    HeaderData      *_Connection;
    HeaderData      *_ContentType;
    HeaderData      *_ContentLength;
  };
  
 
  class HttpRequest : public HttpHeader{
  public:
    HttpRequest();
    ~HttpRequest();
    /*server methods*/
    void           parse(netplus::con *curconnection); //only use as server
    void           printHeader(std::string &buffer);
    int            getRequestType();
    const char    *getRequestURL();
    const char    *getRequest();
    size_t         getRequestLength();
    const char    *getRequestVersion();

    /*Client methods*/
    void           setRequestType(int req);
    void           setRequestURL(const char *url);
    void           setRequestVersion(const char *version);
    /*only for post Reuquesttype*/
    void           setRequestData(const char *data,size_t len);
    void           send(netplus::socket* src,netplus::socket* dest);
  private:
    std::string    _Request;
    int            _RequestType;
    std::string    _RequestURL;
    std::string    _RequestVersion;
    
    HttpHeader     *_HttpHeader;
    netplus::con   *_Connection;
  };
  
  class HttpForm {
  public:
    class MultipartFormData{
    public:
      class Content{
      public:
          const char *getKey();
          const char *getValue();
          Content    *nextContent();
      private:
         Content(const char *key,const char *value);
         ~Content();
         char    *_Key;
         char    *_Value;
         Content *_nextContent;
         friend class MultipartFormData;
      };
      class ContentDisposition{
      public:
        char *getDisposition();
        char *getName();
        char *getFilename();
        
        void  setDisposition(const char *disposition);
        void  setName(const char *name);
        void  setFilename(const char *filename);
      private:
        ContentDisposition();
        ~ContentDisposition();
        char *_Disposition;
        char *_Name;
        char *_Filename;
        friend class MultipartFormData;
      };
      ContentDisposition *getContentDisposition();
      
      const char         *getData();
      size_t              getDataSize();
      
      void                addContent(const char *key,const char *value);
      const char         *getContent(const char *key);
      const char         *getContentType();
      
      MultipartFormData  *nextMultipartFormData();
    private:
      MultipartFormData();
      ~MultipartFormData();
      
      Content            *_firstContent;
      Content            *_lastContent;
      
      void                _parseContentDisposition(const char *disposition);
      ContentDisposition *_ContentDisposition;
      
      const char         *_Data;
      size_t              _Datasize;
      MultipartFormData  *_nextMultipartFormData;
      friend class HttpForm;
    };
    
    class UrlcodedFormData{
    public:
      const char        *getKey();
      const char        *getValue();
      
      void               setKey(const char *key);
      void               setValue(const char *value);
      UrlcodedFormData  *nextUrlcodedFormData();
    private:
      UrlcodedFormData();
      ~UrlcodedFormData();
      std::string        _Key;
      std::string        _Value;
      UrlcodedFormData  *_nextUrlcodedFormData;
      friend class HttpForm;
    };
    
    HttpForm();
    ~HttpForm();
    void                parse(HttpRequest *request);
    const char         *getContentType();
    /*urldecoded form*/
    ssize_t             urlDecode(const char *urlin,size_t urlinsize,char **urlout);
    UrlcodedFormData   *addUrlcodedFormData();
    UrlcodedFormData   *getUrlcodedFormData();
    
    /*multiform*/
    const char         *getBoundary();
    size_t              getBoundarySize();
    
    MultipartFormData  *getMultipartFormData();
    MultipartFormData  *addMultipartFormData();
  private:
    /*urldecoded*/
    void               _parseUrlDecode(HttpRequest *request);
    UrlcodedFormData  *_firstUrlcodedFormData;
    UrlcodedFormData  *_lastUrlcodedFormData;
    /*multiform*/
    void               _parseMulitpart(HttpRequest *request);
    void               _parseMultiSection(const char *section,size_t sectionsize);
    void               _parseBoundary(const char *contenttype);
    char              *_Boundary;
    size_t             _BoundarySize;
    MultipartFormData *_firstMultipartFormData;
    MultipartFormData *_lastMultipartFormData;
    
    /*both methods*/
    inline int         _ishex(int x);
    size_t             _Elements;
    const char*        _ContentType;
  };

  class HttpCookie {
  public:
    class CookieData {
    public:
      CookieData *nextCookieData();
      const char *getKey();
      const char *getValue();
    private:
      CookieData();
      ~CookieData();
      std::string _Key;
      std::string _Value;
      CookieData *_nextCookieData;

      friend class HttpCookie;
    };
    HttpCookie();
    ~HttpCookie();
    void parse(HttpRequest *curreq);
    void setcookie(HttpResponse *curresp,
                   const char *key,const char *value,
                   const char *comment=nullptr,const char *domain=nullptr, 
                   int maxage=-1,const char *path=nullptr,
                   bool secure=false,const char *version="1",const char *samesite=nullptr);
    CookieData    *getfirstCookieData();
    CookieData    *getlastCookieData();
    CookieData    *addCookieData();
  private:
    CookieData     *_firstCookieData;
    CookieData     *_lastCookieData;
  };

#define BASICAUTH  0
#define DIGESTAUTH 1
#define NTLMAUTH   2
  
  class HttpAuth {
  public:
    HttpAuth();
    ~HttpAuth();
    void        parse(HttpRequest *curreq);
    void        setAuth(HttpResponse *curresp);
    
    void        setAuthType(int authtype);
    void        setRealm(const char *realm);
    void        setUsername(const char *username);
    void        setPassword(const char *password);
    
    const char *getUsername();
    const char *getPassword();
    int         getAuthType();
    const char *getAuthRequest();
    
  private:
    int         _Authtype;
    char       *_Username;
    char       *_Password;
    char       *_Realm;
    char       *_Nonce;
    
  };
};
