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

#include <vector>
#include <string>

#include <netplus/socket.h>
#include <netplus/connection.h>
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

      HeaderData &operator=(const char *value);
      HeaderData &operator=(size_t value);
      HeaderData &operator=(int value);

      void clear();
    private:
      HeaderData(const char *key);
      ~HeaderData();
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
    
    HeaderData *getData(const char *key);

    const char *getData(HeaderData *pos);
    size_t      getDataSizet(HeaderData *pos);
    int         getDataInt(HeaderData *pos);
    void        deldata(const char *key);
    void        deldata(HeaderData *pos);
    
    size_t      getElements();
    size_t      getHeaderSize();

    void        clear();
  protected:
    HttpHeader();
    virtual ~HttpHeader();
    HeaderData *_firstHeaderData;
    HeaderData *_lastHeaderData;
  };
  
  class HttpResponse : public HttpHeader,public netplus::con {
  public:
    HttpResponse();
    ~HttpResponse();

    /*server methods*/
    void   setState(const char *httpstate);
    void   setContentType(const char *type);
    void   setContentLength(size_t len);
    void   setConnection(const char *type);
    void   setVersion(const char* version);
    void   setTransferEncoding(const char *enc);

    /*client methods*/
    const char *getState();
    const char *getContentType();
    size_t      getContentLength();
    const char *getConnection();
    const char *getVersion();
    const char *getTransferEncoding();

    size_t printHeader(std::vector<char> &buffer);

    /*server methods*/
    void send(netplus::con *curconnection,const char* data);
    void send(netplus::con *curconnection,const char* data, int datalen); //only use as server

    /*client method*/
    size_t   parse(const char *in,size_t inlen);
  private:
    std::string      _State;
    std::string      _Version;
    HeaderData      *_TransferEncoding;
    HeaderData      *_Connection;
    HeaderData      *_ContentType;
    HeaderData      *_ContentLength;
  };
  
 
  class HttpRequest : public HttpHeader, public netplus::con{
  public:
    HttpRequest();
    HttpRequest(netplus::eventapi *evapi);
    ~HttpRequest();

    void clear();

    /*server methods*/

    size_t            parse(); //only use as server
    void              printHeader(std::string &buffer);
    int               getRequestType();
    const char       *getRequestURL();
    const char       *getRequest();
    size_t            getRequestLength();
    const char       *getRequestVersion();
    size_t            getContentLength();
    size_t            getMaxUploadSize();

    /*mobilphone switch*/
    bool isMobile();

    /*Client methods*/
    void              setRequestType(int req);
    void              setRequestURL(const char *url);
    void              setRequestVersion(const char *version);
    /*only for post Reuquesttype*/
    void              setRequestData(const char *data,size_t len);
    void              setMaxUploadSize(size_t upsize);

    void              send(netplus::socket *src,netplus::socket *dest);

  private:
    std::string       _Request;
    int               _RequestType;
    std::string       _RequestURL;
    std::string       _RequestVersion;
    size_t            _MaxUploadSize;
    friend class HttpForm;
    friend class HttpEvent;
  };
  
  class HttpForm {
  public:
    class MultipartForm{
    public:
      class Data{
        public:
        class Content {
        public:
          const char *getKey();
          const char *getValue();

          void setKey(const char *key);
          void setValue(const char *value);

          Content();
          ~Content();

          Content *nextContent();

        private:
          Content            *_nextContent;
          std::vector<char>   _Key;
          std::vector<char>   _Value;
          friend class HttpForm;
          friend class MultipartFormData;
        };

        class ContentDisposition{
        public:
          const char *getKey();
          const char *getValue();

          void setKey(const char *key);
          void setValue(const char *value);

          ContentDisposition();
          ~ContentDisposition();

          ContentDisposition *nextContentDisposition();

        private:
          ContentDisposition *_nextContentDisposition;
          std::vector<char>   _Key;
          std::vector<char>   _Value;
          friend class HttpForm;
          friend class MultipartFormData;
        };

        ContentDisposition *getDisposition();
        void                addDisposition(ContentDisposition disposition);

        Content            *getContent();
        void                addContent(Content content);

        std::vector<char>   Value;
        Data               *nextData();
      private:
        Data();
        ~Data();
        Content*             _firstContent;
        Content*             _lastContent;

        ContentDisposition *_firstDisposition;
        ContentDisposition *_lastDisposition;
        Data               *_nextData;
        friend class HttpForm;
        friend class MultipartFormData;
      };
      
      Data               *getFormData();

      MultipartForm();
      ~MultipartForm();
    private:
      Data               *_firstData;
      Data               *_lastData;

      void                _parseContentDisposition(const char *disposition);

      friend class HttpForm;
    };
    
    class UrlcodedForm{
    public:
      class Data {
        public:
          Data(Data &fdat);
          Data(const char *key,const char *value);
          ~Data();
          const char   *getKey();
          const char   *getValue();
      
          void          setKey(const char *key);
          void          setValue(const char *value);
          Data         *nextData();
        private:
          std::string        _Key;
          std::string        _Value;
          Data              *_next;
          friend class HttpForm;
      };

      UrlcodedForm();
      ~UrlcodedForm();

      void  addFormData(Data &formdat);
      Data *getFormData();
    private:
      Data *_firstData;
      Data *_lastData;
      friend class HttpForm;
    };
    
    HttpForm();
    ~HttpForm();
    void                parse(HttpRequest *request);
    const char         *getContentType();
    /*urldecoded form*/
    long                urlDecode(const std::vector<char> in,std::vector<char> &out);
    UrlcodedForm        UrlFormData;
    /*multiform*/
    const char         *getBoundary();
    size_t              getBoundarySize();
    MultipartForm       MultipartFormData;
  private:
    /*urldecoded*/
    void               _parseUrlDecode(const netplus::condata<char> &data,size_t csize);

    /*multiform*/
    void               _parseMulitpart(const netplus::condata<char> &data);
    void               _parseMultiSection(netplus::condata<char> &data,size_t start, size_t end);
    void               _parseBoundary(const char *contenttype);
    std::vector<char>  _Boundary;
    
    /*both methods*/
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
