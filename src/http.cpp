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

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <cstring>

#include "config.h"

#include "http.h"

#include "exception.h"

#include <mbedtls/base64.h>

#include <netplus/connection.h>
#include <netplus/exception.h>
#include <assert.h>

libhttppp::HttpHeader::HttpHeader(){
  _firstHeaderData=nullptr;
  _lastHeaderData=nullptr;
}

libhttppp::HttpHeader::HeaderData& libhttppp::HttpHeader::HeaderData::operator<<(const char* value) {
    if (!value)
        return *this;
    _Value.append(value);
    return *this;
}

libhttppp::HttpHeader::HeaderData &libhttppp::HttpHeader::HeaderData::operator<<(size_t value){
  char buf[512];
  snprintf(buf,512,"%zu",value);
  *this<<buf;
  return *this;
}

libhttppp::HttpHeader::HeaderData &libhttppp::HttpHeader::HeaderData::operator<<(int value){
  char buf[512];
  snprintf(buf,512,"%d",value);
  *this<<buf;
  return *this;
}

libhttppp::HttpHeader::HeaderData* libhttppp::HttpHeader::getfirstHeaderData(){
  return _firstHeaderData;
}

libhttppp::HttpHeader::HeaderData* libhttppp::HttpHeader::nextHeaderData(HttpHeader::HeaderData* pos){
  return pos->_nextHeaderData;
}

const char* libhttppp::HttpHeader::getKey(HttpHeader::HeaderData* pos){
  return pos->_Key.c_str();
}

const char* libhttppp::HttpHeader::getValue(HttpHeader::HeaderData* pos){
  return pos->_Value.c_str();
}

libhttppp::HttpHeader::HeaderData *libhttppp::HttpHeader::getData(const char* key){
  HeaderData *curdat =_firstHeaderData;
  while(curdat){
    if(curdat->_Key==key){
        return curdat;
    }
    curdat=curdat->_nextHeaderData;
  }
  return nullptr;
}

const char* libhttppp::HttpHeader::getData(HttpHeader::HeaderData *pos){
  if(!pos){
      HTTPException httpexception;
      httpexception[HTTPException::Note] << "getData no valid pointer set !";
      assert(0);
      throw httpexception;
  }
  return pos->_Value.c_str();
}

size_t libhttppp::HttpHeader::getDataSizet(HttpHeader::HeaderData *pos){
    HTTPException httpexception;
    const char *val=getData(pos);
    size_t ret;
    sscanf(val,"%zu",&ret);
    return ret;
}

int libhttppp::HttpHeader::getDataInt(HttpHeader::HeaderData *pos){
    const char *val=getData(pos);
    return atoi(val);
}

libhttppp::HttpHeader::HeaderData *libhttppp::HttpHeader::setData(const char* key){
  if(!_firstHeaderData){
    _firstHeaderData=new HeaderData(key);
    _lastHeaderData=_firstHeaderData;
  }else{
    _lastHeaderData->_nextHeaderData=new HeaderData(key);
    _lastHeaderData=_lastHeaderData->_nextHeaderData;
  }
  return _lastHeaderData;
}

void libhttppp::HttpHeader::deldata(const char* key){
  for(HeaderData *curdat=_firstHeaderData; curdat; curdat=curdat->_nextHeaderData){
    if(curdat->_Key==key){
      deldata(curdat);  
    }
  }
}

void libhttppp::HttpHeader::deldata(libhttppp::HttpHeader::HeaderData* pos){
  HeaderData *prevdat=nullptr;
  for(HeaderData *curdat=_firstHeaderData; curdat; curdat=curdat->_nextHeaderData){
    if(curdat==pos){
      if(prevdat){
        prevdat->_nextHeaderData=curdat->_nextHeaderData;
        if(_lastHeaderData==curdat)
          _lastHeaderData=prevdat;
      }else{
        _firstHeaderData=curdat->_nextHeaderData;
        if(_lastHeaderData==curdat)
          _lastHeaderData=_firstHeaderData;
      }
      curdat->_nextHeaderData=nullptr;
      delete curdat;
      return;
    }
    prevdat=curdat;
  }    
}


size_t libhttppp::HttpHeader::getElements(){
  size_t elements=0;
  for(HeaderData *curdat=_firstHeaderData; curdat; curdat=curdat->_nextHeaderData){
    elements++;
  }
  return elements;
}

size_t libhttppp::HttpHeader::getHeaderSize(){
  size_t hsize=0;
  for(HeaderData *curdat=_firstHeaderData; curdat; curdat=curdat->_nextHeaderData){
    hsize+=curdat->_Key.length() + curdat->_Value.length();
  }
  return hsize;
}

void libhttppp::HttpHeader::clear(){
  while(_firstHeaderData){
    HeaderData *tmpdat=_firstHeaderData->_nextHeaderData;
    _firstHeaderData->_nextHeaderData=nullptr;
    delete _firstHeaderData;
    _firstHeaderData=tmpdat;
  }
  _lastHeaderData=nullptr;
}


libhttppp::HttpHeader::HeaderData::HeaderData(const char *key){
    HTTPException excep;
    if(!key){
        excep[HTTPException::Error] << "no headerdata key set can't do this";
        throw excep;
    }
    for(size_t i=0; i<strlen(key); ++i){
       _Key.push_back(tolower(key[i]));
    }
    _nextHeaderData=nullptr;
}

libhttppp::HttpHeader::HeaderData::~HeaderData(){
  delete _nextHeaderData;
}

libhttppp::HttpHeader::~HttpHeader(){
  delete _firstHeaderData;
}

libhttppp::HttpResponse::HttpResponse() : HttpHeader(){
  setState(HTTP200);
  setVersion(HTTPVERSION(1.1));
  _ContentType=nullptr;
  _ContentLength=nullptr;
  _Connection=setData("connection");
  *_Connection<<"keep-alive";
  _TransferEncoding=nullptr;
}

void libhttppp::HttpResponse::setState(const char* httpstate){
  HTTPException httpexception;
  if(httpstate==nullptr){
    httpexception[HTTPException::Error] << "http state not set don't do that !!!";
    throw httpexception;
  }
  size_t hlen=strlen(httpstate);
  if(hlen>254){
    httpexception[HTTPException::Error] << "http state with over 255 signs sorry your are drunk !";
    throw httpexception;
  }
  _State=httpstate;
}

void libhttppp::HttpResponse::setContentLength(size_t len){
  if(!_ContentLength)
      _ContentLength=setData("content-length");
  *_ContentLength<<len;
}

void libhttppp::HttpResponse::setContentType(const char* type){
  if(!_ContentType)
    _ContentType=setData("content-type");
  *_ContentType<<type;
}

void libhttppp::HttpResponse::setConnection(const char* type){
  if(!_Connection)
    _Connection=setData("connection");
  *_Connection<<type;
}

void libhttppp::HttpResponse::setVersion(const char* version){
  HTTPException excep;  
  if(version==nullptr)
    throw excep[HTTPException::Error] << "http version not set don't do that !!!";
  size_t vlen=strlen(version);
  if(vlen>254)
    throw excep[HTTPException::Error] << "http version with over 255 signs sorry your are drunk !";
  _Version=version;
}

const char * libhttppp::HttpResponse::getState(){
  return _State.c_str();
}

size_t libhttppp::HttpResponse::getContentLength(){
  return getDataSizet(_ContentLength);
}

const char * libhttppp::HttpResponse::getContentType(){
  return getData(_ContentType);
}

void libhttppp::HttpResponse::send(netplus::con* curconnection, const char* data){
  send(curconnection,data,strlen(data));
}

size_t libhttppp::HttpResponse::printHeader(std::string &buffer){
    buffer=_Version;
    buffer.append(" ");
    buffer.append(_State.c_str());
    buffer.append("\r\n");
    for(HeaderData *curdat=getfirstHeaderData(); curdat; curdat=nextHeaderData(curdat)){ 
        buffer.append(getKey(curdat));
        buffer.append(": ");
        if(getValue(curdat))
            buffer.append(getValue(curdat));
        buffer.append("\r\n");
    } 
    buffer.append("\r\n");
    return buffer.length();
}


void libhttppp::HttpResponse::send(netplus::con* curconnection,const char* data, int datalen){
  if(datalen>=0){
        setContentLength(datalen);
  }
  std::string header;
  size_t headersize = printHeader(header);
  std::copy(header.begin(),header.end(),std::inserter<std::vector<char>>(curconnection->SendData,
                                                                         curconnection->SendData.end()));
  if(datalen>0)
    std::copy(data,data+datalen,std::inserter<std::vector<char>>(curconnection->SendData,
                                                                         curconnection->SendData.end()));
  curconnection->sending(true);
}

size_t libhttppp::HttpResponse::parse(const char *data,size_t inlen){
  HTTPException excep;

  if(inlen <9){

      throw excep[HTTPException::Error] << "HttpResponse header too small aborting!";
  }

  size_t v=5;

  if(memcmp(data,"HTTP/",5)==0){
    while(v<inlen){
      if(data[v]==' ')
        break;
      ++v;
    }
  }else{
      throw excep[HTTPException::Error] << "HttpResponse no Version Tag found !";
  }

  std::copy(data,data+v,std::begin(_Version));

  size_t ve=++v;

  while(ve<inlen){
    if(data[ve]=='\r' || data[ve]=='\n')
       break;
    ++ve;
  };

  --ve;

  std::copy(data+v,data+ve,std::begin(_State));

  ++ve;

  size_t helen=inlen;

  bool hend=false;

  for(size_t cur=ve; cur<inlen; ++cur){
    switch(data[cur]){
      case '\r':
        break;
      case '\n':
        if(hend){
          helen=cur;
          goto HEADERENDFOUND;
        }
        hend=true;
        break;
      default:
        hend=false;
        break;
    }
  }

  throw excep[HTTPException::Error] << "HttpHeader end not found!";

HEADERENDFOUND:

  /*parse the http header fields*/
  size_t lrow=0,delimeter=0,startkeypos=0;

  for(size_t pos=ve; pos< helen; ++pos){
    if(delimeter==0 && data[pos]==':'){
      delimeter=pos;
    }
    if( data[pos]=='\r' || (data[pos-1]!='\r' && data[pos]=='\n') ){
      if(delimeter>lrow && delimeter!=0){
        size_t keylen=delimeter-startkeypos;
        if(keylen>0 && keylen <= helen){
          std::string key;
          key.resize(keylen);
          std::copy(data+startkeypos,data+delimeter,std::begin(key));
          for (size_t it = 0; it < keylen; ++it) {
            if(isalpha(key[it]))
              key[it] = (char)tolower(key[it]);
          }
          ++delimeter;
          while(data[delimeter]==' '){
            ++delimeter;
          };
          size_t valuelen=pos-delimeter;
          if(pos > 0 && valuelen <=helen){
            std::string value;
            value.resize(valuelen);
            std::copy(data+delimeter,data+pos,std::begin(value));
            for (size_t it = 0; it < valuelen; ++it) {
              if(isalpha(value[it]))
                value[it] = (char)tolower(value[it]);
            }
            *setData(key.c_str())<<value.c_str();
          }
        }
      }
      delimeter=0;
      lrow=pos;
      startkeypos=lrow+2;
    }else if(delimeter==0 && data[pos]==':'){
      delimeter=pos;
    }
  }

 _ContentLength=getData("content-length");
 _Connection=getData("connection");
 _ContentType=getData("content-type");
 _TransferEncoding=getData("transfer-encoding");

  return helen;
}

void libhttppp::HttpResponse::setTransferEncoding(const char* enc){
  if(!_TransferEncoding)
    _TransferEncoding=setData("transfer-encoding");
  *_TransferEncoding << enc;
}

const char * libhttppp::HttpResponse::getTransferEncoding(){
  return getData(_TransferEncoding);
}


libhttppp::HttpResponse::~HttpResponse() {

}

libhttppp::HttpRequest::HttpRequest() : HttpHeader(){
  _RequestType=0;
  _MaxUploadSize=DEFAULT_UPLOADSIZE;
  _firstHeaderData=nullptr;
  _lastHeaderData=nullptr;
};

libhttppp::HttpRequest::HttpRequest(netplus::eventapi *evapi) : netplus::con(evapi) {
  _RequestType=0;
  _MaxUploadSize=DEFAULT_UPLOADSIZE;
  _firstHeaderData=nullptr;
  _lastHeaderData=nullptr;
}

void libhttppp::HttpRequest::clear(){
  HttpHeader::clear();
  _RequestType=0;
  _Request.clear();
  _RequestURL.clear();
  _RequestVersion.clear();
}



size_t libhttppp::HttpRequest::parse(){
  HTTPException excep;

  std::vector<char> header;

  auto searchElement = [this](const char *word){
    size_t wsize=strlen(word);
    for(size_t i=0; i<RecvData.size(); ++i){
      for(size_t ii=0; ii<=wsize; ++ii){
        if(ii==wsize){
          return i-wsize;
        }
        if(RecvData[i]==word[ii]){
          ++i;
          continue;
        }
        break;
      }
    }
    return std::string::npos;
  };

  try{

    size_t startpos=0;

    if((startpos=searchElement("GET"))!=std::string::npos){
      _RequestType=GETREQUEST;
    }else if((startpos=searchElement("POST"))!=std::string::npos){
      _RequestType=POSTREQUEST;
    }else{
      excep[HTTPException::Warning] << "Requesttype not known cleanup";
      RecvData.clear();
      throw excep;
    }

    size_t endpos=searchElement("\r\n\r\n");
    if(endpos==std::string::npos){
      excep[HTTPException::Error] << "can't find newline headerend";
      throw excep;
    }

    std::copy(RecvData.begin()+startpos,RecvData.begin()+endpos,
              std::inserter<std::vector<char>>(header,header.begin()));

    endpos+=4;
    bool found=false;
    int pos=0;

    for(size_t cpos=pos; cpos< header.size(); ++cpos){
      if(header[cpos]==' '){
        pos=++cpos;
        break;
      }
    }

    for(size_t cpos=pos; cpos<header.size(); ++cpos){
      if(header[cpos]==' ' && (cpos-pos)<255){
        std::copy(header.begin()+pos,header.begin()+cpos,std::inserter<std::string>(_Request,_Request.begin()));
        _Request.push_back('\0');
        ++pos;
        break;
      }
    }

    if(!_Request.empty()){
      size_t last =_Request.rfind('?');
      if(last==std::string::npos)
        last=_Request.length();
      _RequestURL=_Request.substr(0,last);
    }

    for(size_t cpos=pos; cpos<header.size(); ++cpos){
      if(header[cpos]=='\n' && (cpos-pos)<255){
        std::copy(header.begin()+pos,header.begin()+cpos,std::inserter<std::string>(_RequestVersion,_RequestVersion.begin()));
        _RequestVersion.push_back('\0');
        ++pos;
        found=true;
        break;
      }
    }

    if(!found){
      excep[HTTPException::Error] << "can't parse http head";
      throw excep;
    }

    /*parse the http header fields*/
    size_t lrow=0,delimeter=0,startkeypos=0;

    for(size_t pos=0; pos< header.size(); pos++){
      if(delimeter==0 && header[pos]==':'){
        delimeter=pos;
      }
      if(header[pos]=='\r'){
        if(delimeter>lrow && delimeter!=0){
          size_t keylen=delimeter-startkeypos;
          if(keylen>0 && keylen <= header.size()){
            std::string key(header.begin()+startkeypos,header.begin()+delimeter);
            for (size_t it = 0; it < keylen; ++it) {
              key[it] = (char)tolower(key[it]);
            }
            key.push_back('\0');
            size_t valuelen=(pos-delimeter)-2;
            if(pos > 0 && valuelen <= header.size()){
              size_t vstart=delimeter+2;
              std::string value(header.begin()+vstart,header.begin()+pos);
              for (size_t it = 0; it < valuelen; ++it) {
                value[it] = (char)tolower(value[it]);
              }
              value.push_back('\0');
              *setData(key.c_str())<<value.c_str();
            }
          }
        }
        delimeter=0;
        lrow=pos;
        startkeypos=lrow+2;
      }
    }

    std::move(RecvData.begin()+endpos,RecvData.end(),RecvData.begin());
    RecvData.resize(RecvData.size()-endpos);

  }catch(netplus::NetException &e){
    if (e.getErrorType() != netplus::NetException::Note) {
      RecvData.clear();
    }
    excep[HTTPException::Error] << "netplus error: " << e.what();
    throw excep;
  }
  return header.size();
}

void libhttppp::HttpRequest::printHeader(std::string &buffer){
  if(_RequestType==GETREQUEST)
    buffer="GET ";
  else if(_RequestType==POSTREQUEST)
    buffer="POST ";

  buffer.append(_RequestURL);
  buffer.append(" ");
  buffer.append(_RequestVersion);
  buffer.append("\r\n");
  for(HeaderData *curdat=getfirstHeaderData(); curdat; curdat=nextHeaderData(curdat)){
        buffer.append(getKey(curdat));
        buffer.append(": ");
        if(getValue(curdat))
            buffer.append(getValue(curdat));
        buffer.append("\r\n");
  }
  buffer.append("\r\n");
}


int libhttppp::HttpRequest::getRequestType(){
  return _RequestType;
}

const char* libhttppp::HttpRequest::getRequestURL(){
  return _RequestURL.c_str();
}

const char* libhttppp::HttpRequest::getRequest(){
  return _Request.c_str();
}

size_t libhttppp::HttpRequest::getRequestLength() {
    return _Request.length();
}

const char * libhttppp::HttpRequest::getRequestVersion(){
  return _RequestVersion.c_str();
}

size_t libhttppp::HttpRequest::getContentLength(){
  HttpHeader::HeaderData *clen=getData("content-length");
  if(!clen)
    return 0;
  return getDataSizet(clen);
}

size_t libhttppp::HttpRequest::getMaxUploadSize(){
  return _MaxUploadSize;
}


bool libhttppp::HttpRequest::isMobile(){
  for(HttpHeader::HeaderData *curdat=_firstHeaderData; curdat; curdat=nextHeaderData(curdat)){
      if(strcmp(getKey(curdat),"user-agent")==0){
          if(strstr(getValue(curdat),"mobi"))
            return true;
          break;
      }
  }
  return false;
}

void libhttppp::HttpRequest::setRequestData(const char* data, size_t len){
  _Request.clear();
  _Request.resize(len);
  _Request.insert(0,data);
}

void libhttppp::HttpRequest::setMaxUploadSize(size_t upsize){
  _MaxUploadSize=upsize;
}


void libhttppp::HttpRequest::setRequestVersion(const char* version){
  if(version)
    _RequestVersion=version;
  else
    _RequestVersion.clear();
}


void libhttppp::HttpRequest::send(std::shared_ptr<netplus::socket> src,std::shared_ptr<netplus::socket> dest){
  std::string header;
  printHeader(header);

  try {
    size_t send=dest->sendData(src,(void*)header.c_str(),header.length());

    while(send<_Request.length()){
      send+=dest->sendData(src,(void*)_Request.substr(send,_Request.length()-send).c_str(),
                          _Request.length()-send);
    }
  }catch(netplus::NetException &e){
    HTTPException excep;
    excep[HTTPException::Error] << "RequestSend Socket Error: " << e.what();
    throw excep;
  }
}


void libhttppp::HttpRequest::setRequestType(int req){
  if(req == POSTREQUEST || req == GETREQUEST){
    _RequestType=req;
    return;
  }
   HTTPException excep;
   excep[HTTPException::Error] << "setRequestType: " << "Unknown Request will not set !";
   throw excep;
}

void libhttppp::HttpRequest::setRequestURL(const char* url){
  if(url)
    _RequestURL=url;
  else
    _RequestURL.clear();
}


libhttppp::HttpRequest::~HttpRequest(){
}

libhttppp::HttpForm::HttpForm(){
  _Elements=0;
}

libhttppp::HttpForm::~HttpForm(){
}

void libhttppp::HttpForm::parse(libhttppp::HttpRequest* request){
  try{
      if(request->getRequestType()==POSTREQUEST){
          std::vector<char> bodydat;
          HttpHeader::HeaderData *ctype=request->getData("content-type");

          if(request->RecvData.size()<= request->getContentLength()){
              std::copy(request->RecvData.begin(),request->RecvData.begin()+request->getContentLength(),
                        std::inserter<std::vector<char>>(bodydat,bodydat.begin()));


              if(ctype && strncmp(request->getData(ctype),"multipart/form-data",16)==0){
                  _parseBoundary(request->getData(ctype));
                  _parseMulitpart(bodydat);
              }
              if(ctype && strncmp(request->getData(ctype),"application/x-www-form-urlencoded",34)==0){
                  _parseUrlDecode(bodydat);
              }
          }
      }
      std::vector<char> urldat;
      const char *rurl=request->getRequest();
      size_t rurlsize=strlen(rurl);
      ssize_t rdelimter=-1;
      for(size_t cpos=0; cpos<rurlsize; cpos++){
        if(rurl[cpos]=='?'){
          rdelimter=cpos;
          rdelimter++;
          break;
        }
      }
      if(rdelimter!=-1){
         std::copy(rurl+rdelimter,rurl+rurlsize,std::inserter<std::vector<char>>(urldat,urldat.begin()));
         _parseUrlDecode(urldat);
      }
  }catch(...){}
}

const char *libhttppp::HttpForm::getContentType(){
  return _ContentType;
}

inline int libhttppp::HttpForm::_ishex(int x){
  return  (x >= '0' && x <= '9')  ||
          (x >= 'a' && x <= 'f')  ||
          (x >= 'A' && x <= 'F');
}

ssize_t libhttppp::HttpForm::urlDecode(const char *urlin,size_t urlinsize,std::vector<char> &out){
    char *o;
    const char *end = urlin + urlinsize;
    int c;
    while(urlin <= end) {
        c = *urlin++;
        if (c == '+'){
            c = ' ';
        }else if (c == '%' && (!_ishex(*urlin++)|| !_ishex(*urlin++)	|| !sscanf(urlin - 2, "%2x", &c))){
                return -1;
        }
        out.push_back(c);
    }
    return out.size();
}

const char *libhttppp::HttpForm::getBoundary(){
  return _Boundary.data();
}

size_t libhttppp::HttpForm::getBoundarySize(){
  return _Boundary.size();
}


void libhttppp::HttpForm::_parseBoundary(const char* contenttype){
  size_t ctstartpos=0;
  size_t ctendpos=0;
  const char* boundary="boundary=";
  size_t bdpos=0;
  for(size_t cpos=0; cpos<strlen(contenttype); cpos++){
    if(bdpos==(strlen(boundary)-1)){
      break;
    }else if(contenttype[cpos]==boundary[bdpos]){
      if(ctstartpos==0)
        ctstartpos=cpos;
      bdpos++;
    }else{
      bdpos=0;
      ctstartpos=0;
    }
  }
  for(size_t cpos=ctstartpos; cpos<strlen(contenttype); cpos++){
      if(contenttype[cpos]==';'){
          ctendpos=cpos;
      }
  }
  if(ctendpos==0)
    ctendpos=strlen(contenttype);
  /*cut boundary=*/
  ctstartpos+=strlen(boundary);
  std::copy(contenttype+ctstartpos,contenttype+ctendpos,
            std::inserter<std::vector<char>>(_Boundary,_Boundary.begin()));
  _Boundary.push_back('\0');
}

void libhttppp::HttpForm::_parseMulitpart(const std::vector<char> &data){
    std::vector<char> realboundary;
    realboundary.resize(_Boundary.size()+2);
    std::copy(_Boundary.begin(),_Boundary.end(),realboundary.begin()+2);
    realboundary[0]='-';
    realboundary[1]='-';

    std::vector<char> req;

    std::copy(data.begin(),data.end(),
              std::inserter<std::vector<char>>(req,req.begin()));

    size_t realboundarypos=0;
    size_t oldpos=std::string::npos;

    for(size_t cr=0; cr < req.size(); cr++){
        //check if boundary
        if((char)tolower(req[cr])==realboundary[realboundarypos++]){
            //check if boundary completed
            if(realboundarypos==realboundary.size()-1){
                //ceck if boundary before found set data end
                if(oldpos!=std::string::npos)
                   _parseMultiSection(req,oldpos,cr-realboundary.size());
                oldpos=cr;
                //check if boundary finished
                if(req[cr]=='-' && req[cr+1]=='-'){
                    //boundary finished
                    realboundary.clear();
                    return;
                }
            }
        }else{
            //boundary reset if sign not correct
            realboundarypos=0;
        }
    }
}

void libhttppp::HttpForm::_parseMultiSection(std::vector<char> &data,size_t start, size_t end){

  auto searchElement = [](size_t starts,size_t ends, const char *word,std::vector<char> &sdata){
    size_t wsize=strlen(word);
    for(size_t i=starts; i< ends; ++i){
      for(size_t ii=0; ii<=wsize; ++ii){
        if(ii==wsize){
          return i;
        }
        if( tolower(sdata[i])==tolower(word[ii]) ){
          ++i;
          continue;
        }
        break;
      }
    }
    return std::string::npos;
  };

  size_t findel=searchElement(start,end,"\r\n\r\n",data);

  if(findel==std::string::npos)
    return;

  size_t ctl=std::string::npos,ctlt=std::string::npos;

  if(MultipartFormData._firstData){
     MultipartFormData._lastData->_nextData =new MultipartForm::Data;
     MultipartFormData._lastData=MultipartFormData._lastData->_nextData;
  }else{
      MultipartFormData._firstData=new MultipartForm::Data;
      MultipartFormData._lastData=MultipartFormData._firstData;
  }

  for(size_t scl=++start; scl<findel; ++scl){

      if((data[scl]!= ' ' && data[scl]!= '\r' && data[scl]!= '\n') && ctl==std::string::npos){
          ctl=scl;
      }else if(data[scl]==':' && (ctl & ctlt) !=std::string::npos){
          ctlt=scl;
      }

      if(data[scl]=='\r' && ctl!=std::string::npos && ctlt!=std::string::npos){
              MultipartForm::Data::Content content;
              std::copy(data.begin()+ctl,data.begin()+ctlt,
                            std::inserter<std::vector<char>>(content._Key,content._Key.begin()));

              content._Key.push_back('\0');

              for(size_t tl=0; tl<content._Key.size(); ++tl){
                content._Key[tl]=tolower(content._Key[tl]);
              }

              while(data[++ctlt]==' ');

              std::copy(data.begin()+ctlt,data.begin()+scl,
                            std::inserter<std::vector<char>>(content._Value,content._Value.begin()));

              content._Value.push_back('\0');

              MultipartFormData._lastData->addContent(content);

              ctl=std::string::npos;
              ctlt=std::string::npos;
      }
  }

  MultipartForm::Data::Content *cdispo=nullptr;

  for(MultipartForm::Data::Content *curcontent=MultipartFormData._lastData->_firstContent; curcontent; curcontent=curcontent->nextContent()){
     if( strcmp(curcontent->getKey(),"content-disposition") ==0 ) {
        cdispo=curcontent;
     }
  }

  if(cdispo){

      size_t ifdp= searchElement(0,cdispo->_Value.size(),"form-data",cdispo->_Value);

      size_t scss=std::string::npos,scse=std::string::npos;

      size_t scvts=std::string::npos;

      size_t scvss=std::string::npos,scvse=std::string::npos;

      for(size_t sc=ifdp; sc<cdispo->_Value.size(); ++sc){
        if(scss==std::string::npos){
          if(cdispo->_Value[sc]!=';' && cdispo->_Value[sc]!=' '){
            scss=sc;
          }
        }else{
          if( (cdispo->_Value[sc]==' ' || cdispo->_Value[sc]=='=' || sc+1==cdispo->_Value.size()) && scse==std::string::npos){
            scse=sc;
          }

          if( scse!=std::string::npos){
            for(size_t scv=scss; scv<cdispo->_Value.size(); ++scv){
              if(cdispo->_Value[scv]=='=' && scvts==std::string::npos){
                scvts=scv;
              }else if(scvts!=std::string::npos){
                if(cdispo->_Value[scv]=='"'){
                  if(scvss==std::string::npos) {
                    scvss=++scv;
                  }else if(scvse==std::string::npos){
                    scvse=scv;
                    scv++;
                    sc=scv;
                  }
                }
              }
            }
          }
        }

        if(scss !=std::string::npos && scse !=std::string::npos){
          MultipartForm::Data::ContentDisposition dispo;
          std::copy(cdispo->_Value.begin()+scss,cdispo->_Value.begin()+scse,
                    std::inserter<std::vector<char>>(dispo._Key,dispo._Key.begin()));

          dispo._Key.push_back('\0');
          if(scvss!=std::string::npos && scvse!=std::string::npos){

            std::copy(cdispo->_Value.begin()+scvss,cdispo->_Value.begin()+scvse,
                      std::inserter<std::vector<char>>(dispo._Value,dispo._Value.begin()));

            dispo._Value.push_back('\0');

            scvss=std::string::npos;
            scvse=std::string::npos;
            scvts=std::string::npos;
          }

          scss=std::string::npos;
          scse=std::string::npos;
          MultipartFormData._lastData->addDisposition(dispo);
        }

      }
      if(MultipartFormData._lastData)
          std::copy(data.begin()+findel,data.begin()+end,std::inserter<std::vector<char>>(MultipartFormData._lastData->Value,
                                                                                          MultipartFormData._lastData->Value.begin()));
  }

}

libhttppp::HttpForm::MultipartForm::Data * libhttppp::HttpForm::MultipartForm::Data::nextData(){
  return _nextData;
}

libhttppp::HttpForm::MultipartForm::Data::Content *libhttppp::HttpForm::MultipartForm::Data::Content::nextContent(){
  return _nextContent;
}

libhttppp::HttpForm::MultipartForm::Data * libhttppp::HttpForm::MultipartForm::getFormData(){
  return _firstData;
}

libhttppp::HttpForm::MultipartForm::Data::ContentDisposition * libhttppp::HttpForm::MultipartForm::Data::ContentDisposition::nextContentDisposition(){
  return _nextContentDisposition;
}


libhttppp::HttpForm::MultipartForm::MultipartForm(){
  _firstData=nullptr;
  _lastData=nullptr;
}

libhttppp::HttpForm::MultipartForm::~MultipartForm(){
  Data *next=_firstData,*curel=nullptr;
  while(next){
        curel=next->_nextData;
        next->_nextData=nullptr;
        delete next;
        next=curel;
  }
  _lastData=nullptr;
}

libhttppp::HttpForm::MultipartForm::Data::Data(){
  _firstDisposition=nullptr;
  _lastDisposition=nullptr;
  _firstContent=nullptr;
  _lastContent=nullptr;
  _nextData=nullptr;
}

libhttppp::HttpForm::MultipartForm::Data::~Data(){
  Content *nextContent=_firstContent,*curelc=nullptr;
  while(nextContent){
        curelc=nextContent->_nextContent;
        nextContent->_nextContent=nullptr;
        delete nextContent;
        nextContent=curelc;
  }
  _lastContent=nullptr;

  ContentDisposition *nextdis=_firstDisposition,*cureld=nullptr;
  while(nextdis){
        cureld=nextdis->_nextContentDisposition;
        nextdis->_nextContentDisposition=nullptr;
        delete nextdis;
        nextdis=cureld;
  }
  _lastDisposition=nullptr;

}

libhttppp::HttpForm::MultipartForm::Data::Content::Content(){
  _nextContent=nullptr;
}

libhttppp::HttpForm::MultipartForm::Data::Content::~Content(){
}

void libhttppp::HttpForm::MultipartForm::Data::Content::setKey(const char* key){
  _Key.clear();
  std::copy(key,key+(strlen(key)+1),std::inserter<std::vector<char>>(_Key,_Key.begin()));
}

void libhttppp::HttpForm::MultipartForm::Data::Content::setValue(const char* value){
  _Value.clear();
  std::copy(value,value+(strlen(value)+1),std::inserter<std::vector<char>>(_Value,_Value.begin()));
}

const char * libhttppp::HttpForm::MultipartForm::Data::Content::getKey(){
  return _Key.data();
}

const char * libhttppp::HttpForm::MultipartForm::Data::Content::getValue(){
  return _Value.data();
}


const char *libhttppp::HttpForm::MultipartForm::Data::ContentDisposition::getKey(){
  return _Key.data();
}

const char *libhttppp::HttpForm::MultipartForm::Data::ContentDisposition::getValue(){
  return _Value.data();
}

void libhttppp::HttpForm::MultipartForm::Data::ContentDisposition::setKey(const char* key){
  _Key.clear();
  std::copy(key,key+(strlen(key)+1),std::inserter<std::vector<char>>(_Key,_Key.begin()));
}

void libhttppp::HttpForm::MultipartForm::Data::ContentDisposition::setValue(const char* value){
  _Value.clear();
  std::copy(value,value+(strlen(value)+1),std::inserter<std::vector<char>>(_Value,_Value.begin()));
}


libhttppp::HttpForm::MultipartForm::Data::ContentDisposition::ContentDisposition(){
  _nextContentDisposition=nullptr;
}

libhttppp::HttpForm::MultipartForm::Data::ContentDisposition::~ContentDisposition(){
}

libhttppp::HttpForm::MultipartForm::Data::ContentDisposition* libhttppp::HttpForm::MultipartForm::Data::getDisposition(){
  return _firstDisposition;
}

void libhttppp::HttpForm::MultipartForm::Data::addDisposition(ContentDisposition disposition){
  if(_firstDisposition){
      _lastDisposition->_nextContentDisposition=new ContentDisposition(disposition);
      _lastDisposition=_lastDisposition->_nextContentDisposition;
  }else{
      _firstDisposition=new ContentDisposition(disposition);
      _lastDisposition=_firstDisposition;
  }
}

libhttppp::HttpForm::MultipartForm::Data::Content* libhttppp::HttpForm::MultipartForm::Data::getContent(){
  return _firstContent;
}

void libhttppp::HttpForm::MultipartForm::Data::addContent(Content content){
  if(_firstContent){
      _lastContent->_nextContent=new Content(content);
      _lastContent=_lastContent->_nextContent;
  }else{
      _firstContent=new Content(content);
      _lastContent=_firstContent;
  }
}


void libhttppp::HttpForm::_parseUrlDecode(const std::vector<char> &data){
  HTTPException httpexception;

  size_t fdatstpos=0;
  size_t keyendpos=0;
  for(size_t fdatpos=0; fdatpos<=data.size(); fdatpos++){
    if(data[fdatpos] == '&' || fdatpos==data.size()){
      if(keyendpos >fdatstpos && keyendpos<fdatpos){
        std::vector<char> key,ukey;;
        size_t vlstpos=keyendpos+1;
        std::vector<char> value,uvalue;
        char *urldecdKey=nullptr;
        std::copy(data.begin()+fdatstpos,data.begin()+keyendpos,std::inserter<std::vector<char>>(key,key.begin()));
        key.push_back('\0');
        if(vlstpos<=data.size()){
          std::copy(data.begin()+vlstpos,data.begin()+fdatpos,std::inserter<std::vector<char>>(value,value.begin()));
          value.push_back('\0');
          urlDecode(value.data(),value.size(),uvalue);
        }
        urlDecode(key.data(),key.size(),ukey);
        UrlcodedForm::Data urldat(ukey.data(),uvalue.data());
        UrlFormData.addFormData(urldat);
      }
      fdatstpos=fdatpos+1;
    }else if( data[fdatpos] == '=' ){
      keyendpos=fdatpos;
    };
  }
}

void libhttppp::HttpForm::UrlcodedForm::Data::setKey(const char* key){
   if(!key){
	HTTPException httpexception;
	httpexception[HTTPException::Error] << "no urldecoded key set";
	throw httpexception;
   }
   _Key=key;
}

void libhttppp::HttpForm::UrlcodedForm::Data::setValue(const char* value){
  if(!value){
      _Value.clear();
      return;
  }
  _Value=value; 
}

const char *libhttppp::HttpForm::UrlcodedForm::Data::getKey(){
  return _Key.c_str();
}

const char *libhttppp::HttpForm::UrlcodedForm::Data::getValue(){
  return _Value.c_str();
}

libhttppp::HttpForm::UrlcodedForm::Data  *libhttppp::HttpForm::UrlcodedForm::Data::nextData(){
  return _next;
}

libhttppp::HttpForm::UrlcodedForm::Data::Data(Data& fdat){
  _Key=fdat._Key;
  _Value=fdat._Value;
  _next = nullptr;
}

libhttppp::HttpForm::UrlcodedForm::Data::Data(const char *key,const char *value){
  _Key=key;
  _Value=value;
  _next=nullptr;
}

libhttppp::HttpForm::UrlcodedForm::Data::~Data(){
}

libhttppp::HttpForm::UrlcodedForm::UrlcodedForm(){
  _firstData=nullptr;
  _lastData=nullptr;
}

libhttppp::HttpForm::UrlcodedForm::~UrlcodedForm(){
  Data *next=_firstData,*curel=nullptr;
  while(next){
        curel=next->_next;
        next->_next=nullptr;
        delete next;
        next=curel;
  }
  _lastData=nullptr;
}


void libhttppp::HttpForm::UrlcodedForm::addFormData(Data &formdat){
  if(!_firstData){
    _firstData= new UrlcodedForm::Data(formdat);
    _lastData=_firstData;
  }else{
    _lastData->_next=new UrlcodedForm::Data(formdat);
    _lastData=_lastData->_next;
  }
}

libhttppp::HttpForm::UrlcodedForm::Data  *libhttppp::HttpForm::UrlcodedForm::getFormData(){
  return _firstData;
}

libhttppp::HttpCookie::CookieData::CookieData(){
  _nextCookieData=nullptr;
}

libhttppp::HttpCookie::CookieData::~CookieData(){
  delete   _nextCookieData;
}

libhttppp::HttpCookie::CookieData * libhttppp::HttpCookie::CookieData::nextCookieData(){
  return _nextCookieData;
}

const char * libhttppp::HttpCookie::CookieData::getKey(){
  return _Key.c_str();
}

const char * libhttppp::HttpCookie::CookieData::getValue(){
  return _Value.c_str();
}



libhttppp::HttpCookie::HttpCookie(){
  _firstCookieData=nullptr;
  _lastCookieData=nullptr;
}

libhttppp::HttpCookie::~HttpCookie(){
  delete _firstCookieData;
}


void libhttppp::HttpCookie::setcookie(HttpResponse *curresp,
                                      const char* key, const char* value,
                                      const char *comment,const char *domain,  
                                      int maxage, const char* path,
                                      bool secure,const char *version,const char *samesite){
    HTTPException httpexception;
    if(!key || !value){
        httpexception[HTTPException::Note] << "no key or value set in cookie!";
        throw httpexception;
    }
    HttpHeader::HeaderData *dat=curresp->setData("set-cookie");
    *dat << key << "=" << value;
    if(comment){
        *dat << "; comment="; 
        *dat << comment;
    }
    if(domain)
        *dat << "; domain=" << domain;
    if(maxage>=0)
        *dat << "; max-age=" << maxage;
    if(path)
        *dat << "; path=" << path;
    if(samesite)
        *dat << "; sameSite=" << samesite;
    if(secure)
        *dat << "; secure";
    if(version)
        *dat << "; version=" << version;
    
}


void libhttppp::HttpCookie::parse(libhttppp::HttpRequest* curreq){
  HttpHeader::HeaderData *hc = curreq->getData("cookie");

  if(!hc)
    return;

  std::vector<char> cdat;
  std::copy(curreq->getData(hc),curreq->getData(hc)+strlen(curreq->getData(hc)),
            std::inserter<std::vector<char>>(cdat,cdat.begin()));

  int delimeter=-1;
  int keyendpos=-1;
  int startpos=0;
  
  for (size_t cpos = 0; cpos < cdat.size(); cpos++) {
      if (cdat[cpos] == ' ') {
          ++startpos;
      }
      else if (cdat[cpos] == '=') {
          keyendpos = cpos;
      }else if (cdat[cpos] == ';'){
          delimeter = cpos;
      }else if (cpos == (cdat.size() - 1)) {
		  delimeter = cpos+1;
	  }
	  if (keyendpos != -1 && delimeter != -1) {
		  CookieData* curcookie = addCookieData();
          for(size_t i=startpos; i<keyendpos; ++i){
              curcookie->_Key.push_back(tolower(cdat[i]));
          }

          for(size_t i=++keyendpos; i<delimeter; ++i){
            curcookie->_Value.push_back(cdat[i]);
          }
          
          keyendpos = -1;
          delimeter = -1;
          startpos = ++cpos;
	  }
  }
}

libhttppp::HttpCookie::CookieData *libhttppp::HttpCookie::getfirstCookieData(){
  return _firstCookieData;
}

libhttppp::HttpCookie::CookieData *libhttppp::HttpCookie::getlastCookieData(){
  return _lastCookieData;
}


libhttppp::HttpCookie::CookieData  *libhttppp::HttpCookie::addCookieData(){
  if(!_firstCookieData){
    _firstCookieData= new CookieData;
    _lastCookieData=_firstCookieData;
  }else{
    _lastCookieData->_nextCookieData=new CookieData;
    _lastCookieData=_lastCookieData->_nextCookieData;
  }
  return _lastCookieData;
}

libhttppp::HttpAuth::HttpAuth(){
  _Authtype=BASICAUTH;
  _Username=nullptr;
  _Password=nullptr;
  _Realm=nullptr;
}

libhttppp::HttpAuth::~HttpAuth(){
  delete[] _Username;
  delete[] _Password;
  delete[] _Realm;
}


void libhttppp::HttpAuth::parse(libhttppp::HttpRequest* curreq){
  HttpHeader::HeaderData *hau=curreq->getData("authorization");
  if(!hau)
    return;
  const char *authstr=curreq->getData(hau);
  if(!authstr)
    return;
  if(strcmp(authstr,"basic")==0)
    _Authtype=BASICAUTH;
  else if(strcmp(authstr,"digest")==0)
    _Authtype=DIGESTAUTH;
  switch(_Authtype){
    case BASICAUTH:{
      size_t base64strsize=strlen(authstr+6);
      unsigned char *base64str=new char unsigned[base64strsize+1];
      memcpy(base64str,authstr+6,strlen(authstr)-5);
      char *clearstr= new char[(base64strsize/4)*3];
      size_t cleargetlen;
      mbedtls_base64_decode((unsigned char*)clearstr,((base64strsize/4)*3),&cleargetlen,base64str,base64strsize);
      delete[] base64str;
      for(size_t clearpos=0; clearpos<cleargetlen; clearpos++){
         if(clearstr[clearpos]==':'){
            char *usernm=new char[clearpos+1];
            memcpy(usernm,clearstr+clearpos,strlen(clearstr)-clearpos);
            usernm[clearpos]='\0';
            setUsername(usernm);
            setPassword(clearstr+(clearpos+1));
            delete[] usernm;
         }
      }
      delete[] clearstr;
    };
    case DIGESTAUTH:{
      for(size_t authstrpos=7; authstrpos<strlen(authstr)+1; authstrpos++){
        switch(authstr[authstrpos]){
          case '\0':{
        
            break;    
          }
          case ' ':{
          
          };
        }
      }
    };
  }
}

void libhttppp::HttpAuth::setAuth(libhttppp::HttpResponse* curresp){
  HttpHeader::HeaderData *dat=curresp->setData("www-authenticate");
  switch(_Authtype){
    case BASICAUTH:{
      if(_Realm){
        *dat << "basic realm=\"" << _Realm << "\"";
      }else{
        *dat << "basic";
      }
    };
    case DIGESTAUTH:{
      *dat << "digest";
      if(_Realm){
        *dat << " realm=\"" << _Realm <<"\"";
      }  
      char nonce[16]={'0'};
      for(size_t noncepos=0; noncepos<=16; noncepos++){
        char random[]={1,2,3,4,5,6,7,8,9,'a','b','c','d','e','f','\0'};
        for(size_t rdpos=0; random[rdpos]!='\0'; rdpos++){ 
           nonce[noncepos]=random[rdpos];
        }
      }
      *dat << "nonce=\"" << nonce << "\"";
    };
  }
}

void libhttppp::HttpAuth::setAuthType(int authtype){
  _Authtype=authtype;
}

void libhttppp::HttpAuth::setRealm(const char* realm){
  if(_Realm)
    delete[] _Realm;
  if(realm){
    size_t realmsize=strlen(realm);
    _Realm=new char [realmsize+1];
    memcpy(_Realm,realm,realmsize);
    _Realm[realmsize]='\0';
  }else{
    _Realm=nullptr;  
  }
}

void libhttppp::HttpAuth::setUsername(const char* username){
  if(_Username)
    delete[] _Username;
  size_t usernamelen=strlen(username);
  _Username=new char[usernamelen+1];
  memcpy(_Username,username,usernamelen+1);
}

void libhttppp::HttpAuth::setPassword(const char* password){
  if(_Password)
    delete[] _Password;
  size_t passwordlen=strlen(password);
  _Password=new char[passwordlen+1];
  memcpy(_Password,password,passwordlen+1);
}


const char *libhttppp::HttpAuth::getUsername(){
  return _Username;
}

const char *libhttppp::HttpAuth::getPassword(){
  return _Password;
}

int libhttppp::HttpAuth::getAuthType(){
  return _Authtype;
}
