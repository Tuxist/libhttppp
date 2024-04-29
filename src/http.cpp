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


libhttppp::HttpHeader::HeaderData::HeaderData(const char *key){
    HTTPException excep;
    if(!key){
        excep[HTTPException::Error] << "no headerdata key set can't do this";
        throw excep;
    }
    _Key = key;
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
  curconnection->addSendQueue(header.c_str(), headersize);
  if(datalen>0)
    curconnection->addSendQueue(data,datalen);
  curconnection->sending(true);
}
#include <iostream>
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
          std::cerr << cur << std::endl;
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

libhttppp::HttpRequest::HttpRequest(){
  _RequestType=0;
}

void libhttppp::HttpRequest::parse(netplus::con* curconnection){
    HTTPException excep;
    if(!curconnection)
        return;

    try{
        netplus::con::condata *curdat=curconnection->getRecvFirst();
        if(curdat){
            netplus::con::condata *startblock;
            int startpos=0;
            
            if((startpos=curconnection->searchValue(curdat,&startblock,"GET",3))==0 && startblock==curdat){
                _RequestType=GETREQUEST;
            }else if((startpos=curconnection->searchValue(curdat,&startblock,"POST",4))==0 && startblock==curdat){
                _RequestType=POSTREQUEST;
            }else{
                excep[HTTPException::Warning] << "Requesttype not known cleanup";
                curconnection->cleanRecvData();
                throw excep;
            }
            netplus::con::condata *endblock;
            ssize_t endpos=curconnection->searchValue(startblock,&endblock,"\r\n\r\n",4);
            if(endpos==-1){
                excep[HTTPException::Error] << "can't find newline headerend";
                throw excep;
            }
            endpos+=4;  
            
            
            std::string header;
            curconnection->copyValue(startblock,startpos,endblock,endpos,header);
            
            bool found=false;
            int pos=0;

            for(size_t cpos=pos; cpos< header.length(); ++cpos){
                if(header[cpos]==' '){
                    pos=++cpos;
                    break;
                }
            }

            for(size_t cpos=pos; cpos<header.length(); ++cpos){
                if(header[cpos]==' ' && (cpos-pos)<255){
                    _Request = header.substr(pos,cpos-pos);
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

            for(size_t cpos=pos; cpos<header.length(); ++cpos){
                if(header[cpos]==' ' && (cpos-pos)<255){
                    _RequestVersion = header.substr(pos,cpos-pos);
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
            
            for(size_t pos=0; pos< header.length(); pos++){
                if(delimeter==0 && header[pos]==':'){
                    delimeter=pos; 
                }
                if(header[pos]=='\r'){
                    if(delimeter>lrow && delimeter!=0){
                        size_t keylen=delimeter-startkeypos;
                        if(keylen>0 && keylen <= header.length()){
                            std::string key;
                            key=header.substr(startkeypos,keylen);
                            for (size_t it = 0; it < keylen; ++it) {
                                key[it] = (char)tolower(key[it]);
                            }
                            size_t valuelen=(pos-delimeter)-2;
                            if(pos > 0 && valuelen <= header.length()){
                                std::string value;
                                size_t vstart=delimeter+2;
                                value=header.substr(vstart,valuelen);
                                for (size_t it = 0; it < valuelen; ++it) {
                                    value[it] = (char)tolower(value[it]);
                                }

                                *setData(key.c_str())<<value.c_str();
                            }
                        }
                    }
                    delimeter=0;
                    lrow=pos;
                    startkeypos=lrow+2;
                }
            }

            HttpHeader::HeaderData *contentlen=getData("content-length");

            if(_RequestType==POSTREQUEST){
                if((getDataInt(contentlen)+ header.length()) <= curconnection->getRecvLength()){
                    netplus::con::condata *edblock,*sdblock;
                    size_t edblocksize = getDataSizet(contentlen)+header.length(), sdblocksize = header.length();

                    for (sdblock = curconnection->getRecvFirst(); sdblock; sdblock = sdblock->nextcondata()) {
                        if (sdblocksize!=0 && sdblock->getDataLength() <= sdblocksize) {
                            sdblocksize -= sdblock->getDataLength();
                            continue;
                        }
                        break;
                    }

                    for(edblock=curconnection->getRecvFirst(); edblock; edblock=edblock->nextcondata()){
                        if (edblocksize !=0 && edblock->getDataLength() <= edblocksize) {
                            edblocksize -= edblock->getDataLength();
                            continue;
                        }
                        break;
                    }

                    _Request.clear();
                    curconnection->copyValue(sdblock, sdblocksize, edblock, edblocksize, _Request);
                    curconnection->resizeRecvQueue(getDataSizet(contentlen) + header.length());
                }else{
                    netplus::con::condata *edblock=curconnection->getRecvLast(),*sdblock=curconnection->getRecvFirst();
                    _Request.clear();
                    curconnection->copyValue(sdblock,header.length(), edblock, edblock->getDataLength(), _Request);
                    curconnection->resizeRecvQueue( + header.length());
                }
            } else {
                curconnection->resizeRecvQueue(header.length());
            }

            if (curconnection->getRecvLength()!=0) {
                excep[HTTPException::Note] << "RequestAgain";
            }

        }else{
            excep[HTTPException::Note] << "No Incoming data in queue";
            throw excep;
        }

    }catch(HTTPException &e){
        if (e.getErrorType() != libhttppp::HTTPException::Note) {
            curconnection->cleanRecvData();           
        }
        throw e;
    }
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

void libhttppp::HttpRequest::setRequestVersion(const char* version){
  if(version)
    _RequestVersion=version;
  else
    _RequestVersion.clear();
}


void libhttppp::HttpRequest::send(netplus::socket* src,netplus::socket* dest){
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
  _Boundary=nullptr;
  _Elements=0;
  _firstUrlcodedFormData=nullptr;
  _lastUrlcodedFormData=nullptr;
  _firstMultipartFormData=nullptr;
  _lastMultipartFormData=nullptr;
}

libhttppp::HttpForm::~HttpForm(){
  delete[] _Boundary;
  delete   _firstMultipartFormData;
  _lastMultipartFormData=nullptr;
}


void libhttppp::HttpForm::parse(libhttppp::HttpRequest* request){
  try{
      HttpHeader::HeaderData *ctype=request->getData("content-type");
      _ContentType = request->getData(ctype);
      if(_ContentType &&
          strncmp(_ContentType,"multipart/form-data",18)==0){
          _parseMulitpart(request);
      }
  }catch(...){}
  _parseUrlDecode(request);
}

const char *libhttppp::HttpForm::getContentType(){
  return _ContentType;;
}

inline int libhttppp::HttpForm::_ishex(int x){
  return  (x >= '0' && x <= '9')  ||
          (x >= 'a' && x <= 'f')  ||
          (x >= 'A' && x <= 'F');
}

ssize_t libhttppp::HttpForm::urlDecode(const char *urlin,size_t urlinsize,char **urlout){
    char *o;
    char *dec = new char[urlinsize+1];
    *urlout=dec;
    const char *end = urlin + urlinsize;
    int c;
    for (o = dec; urlin <= end; o++) {
        c = *urlin++;
        if (c == '+'){
            c = ' ';
        }else if (c == '%' && (!_ishex(*urlin++)|| !_ishex(*urlin++)	||
            !sscanf(urlin - 2, "%2x", &c))){
            return -1;
            }
            if (dec){
                *o = c;
            }
    }
    return o - dec;
}

const char *libhttppp::HttpForm::getBoundary(){
  return _Boundary;  
}

size_t libhttppp::HttpForm::getBoundarySize(){
  return _BoundarySize;
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
  if(_Boundary)
     delete[] _Boundary;
  _BoundarySize=(ctendpos-ctstartpos);
  _Boundary=new char[_BoundarySize+1];
  memcpy(_Boundary,contenttype+ctstartpos,(ctendpos-ctstartpos));
  _Boundary[(ctendpos-ctstartpos)]='\0';
}

void libhttppp::HttpForm::_parseMulitpart(libhttppp::HttpRequest* request){
    _parseBoundary(_ContentType);
    char *realboundary = new char[_BoundarySize+3];
    memcpy(realboundary+2,_Boundary,_BoundarySize+1);
    realboundary[0]='-';
    realboundary[1]='-';
    size_t realboundarylen=_BoundarySize+2;
    const char *req=request->getRequest();
    size_t reqsize=request->getRequestLength();
    size_t realboundarypos=0;
    unsigned int datalength = 0;
    const char *datastart=nullptr;
    size_t oldpos=0;
    size_t cr=0;
    while(cr < reqsize){
        //check if boundary
        if((char)tolower(req[cr++])==realboundary[realboundarypos++]){
            //check if boundary completed
            if((realboundarypos+1)==realboundarylen){
                //ceck if boundary before found set data end
                if(datastart!=nullptr){
                    //cut boundarylen
                    datalength=(cr-oldpos)-realboundarylen;
                    if(datalength>0)
                        _parseMultiSection(datastart,datalength);
                    datastart=nullptr;
                }
                //set cr one higher to cut boundary
                cr++;
                //cut first new line
                while(req[cr]=='\r' || req[cr]=='\n'){
                    cr++;
                }
                datastart=req+cr;
                //store pos dor datalength
                oldpos=cr;
                //check if boundary finished
                if(req[cr]=='-' && req[cr+1]=='-'){
                    //boundary finished
                    realboundarylen=0;
                    delete[] realboundary;
                    return;
                }
            }
        }else{
            //boundary reset if sign not correct
            realboundarypos=0;
        }
    }
    delete[] realboundary;
}

void libhttppp::HttpForm::_parseMultiSection(const char* section, size_t sectionsize){
  _Elements++;
  MultipartFormData *curmultipartformdata=addMultipartFormData();
  const char *windelimter="\r\n\r\n";
  size_t windelimtersize=strlen(windelimter);
  size_t fpos=0,fendpos=0,fcurpos=0;
  for(size_t dp=0; dp<sectionsize; dp++){
    if(windelimter[fcurpos]==section[dp]){
        if(fcurpos==0){
          fpos=dp;
        }
        fcurpos++;
      }else{
        fcurpos=0;
        fpos=0;
      }
      if(fcurpos==windelimtersize)
        break;
  }
  fendpos=fpos+windelimtersize;
  if(fpos==0 || fendpos==0)
    return;
   
  /*change stpartpos for data*/
  curmultipartformdata->_Data=section+fendpos;
  curmultipartformdata->_Datasize=sectionsize-(fendpos+1);
  
  /*Debug data in formdata*/
//   for(size_t cd=0; cd<curmultipartformdata->_Datasize; cd++){
//     printf("%c",curmultipartformdata->_Data[cd]);
//     printf(" -> testing loop: %zu \n",cd);
//   }
  
  /*content parameter parsing*/
  size_t lrow=0,delimeter=0,startkeypos=0;
  for(size_t pos=0; pos<fendpos; pos++){
    if(delimeter==0 && section[pos]==':'){
      delimeter=pos;
    }
    if(section[pos]=='\r' && delimeter!=0){
      if(delimeter>lrow){
        size_t keylen=delimeter-startkeypos;
        if(keylen>0 && keylen <=sectionsize){
          char *key=new char[keylen+1];
          memcpy(key,section+startkeypos,keylen);
          key[keylen]='\0';

          for (size_t it = 0; it < keylen; ++it) {
              key[it] = (char)tolower(key[it]);
          }

          size_t valuelen=((pos-delimeter)-2);
          if(pos > 0 && valuelen <= sectionsize){
            char *value=new char[valuelen+1];
            size_t vstart=delimeter+2;
            memcpy(value,section+vstart,valuelen);
            value[valuelen]='\0';
            curmultipartformdata->addContent(key,value);
            delete[] value;
          }
          delete[] key;
	}
      }
      delimeter=0;
      lrow=pos;
      startkeypos=lrow+2;
    }
  }
  //Debug content headerdata
//   for(libhttppp::HttpForm::MultipartFormData::Content *curcnt=curmultipartformdata->_firstContent; curcnt; curcnt=curcnt->nextContent()){
//       printf("%s -> %s\n",curcnt->getKey(),curcnt->getValue());
//       
//   }
  
  curmultipartformdata->_parseContentDisposition(
    curmultipartformdata->getContent("content-disposition")
  );
}

libhttppp::HttpForm::MultipartFormData  *libhttppp::HttpForm::addMultipartFormData(){
  if(!_firstMultipartFormData){
    _firstMultipartFormData= new MultipartFormData;
    _lastMultipartFormData=_firstMultipartFormData;
  }else{
    _lastMultipartFormData->_nextMultipartFormData=new MultipartFormData;
    _lastMultipartFormData=_lastMultipartFormData->_nextMultipartFormData;
  }
  return _lastMultipartFormData;
}

libhttppp::HttpForm::MultipartFormData  *libhttppp::HttpForm::getMultipartFormData(){
  return _firstMultipartFormData;
}

libhttppp::HttpForm::MultipartFormData::MultipartFormData(){
  _ContentDisposition=new ContentDisposition;
  _nextMultipartFormData=nullptr;
  _Data=nullptr;
  _firstContent=nullptr;
  _lastContent=nullptr;
}

libhttppp::HttpForm::MultipartFormData::~MultipartFormData(){
  delete _ContentDisposition;
  delete _nextMultipartFormData;
  delete _firstContent;
  _lastContent=nullptr;
}

const char *libhttppp::HttpForm::MultipartFormData::getData(){
  return _Data;  
}

size_t libhttppp::HttpForm::MultipartFormData::getDataSize(){
  return _Datasize;
}


libhttppp::HttpForm::MultipartFormData::ContentDisposition *libhttppp::HttpForm::MultipartFormData::getContentDisposition(){
  return _ContentDisposition;
}

void libhttppp::HttpForm::MultipartFormData::_parseContentDisposition(const char *disposition){
  size_t dislen=strlen(disposition);
  
  for(size_t dpos=0; dpos<dislen; dpos++){
    if(disposition[dpos]==';' || disposition[dpos]=='\r'){
      char *ctype = new char[dpos+1];
      memcpy(ctype,disposition,dpos);
      ctype[dpos]='\0';
      getContentDisposition()->setDisposition(ctype);
      delete[] ctype;
      break;
    }
  }
  
  const char *lownamedelimter="name=\"";
  const char* HIGHnamedelimter = "NAME=\"";
  ssize_t namedelimtersize=strlen(lownamedelimter);
  ssize_t fpos=-1,fendpos=0,fcurpos=0,fterm=0;
  for(size_t dp=0; dp<dislen; dp++){
    if(lownamedelimter[fcurpos]==disposition[dp] ||
        HIGHnamedelimter[fcurpos] == disposition[dp]){
        if(fcurpos==0){
          fpos=dp;
        }
        fcurpos++;
      }else{
        fcurpos=0;
        fpos=-1;
      }
      if(fcurpos==namedelimtersize)
        break;
  }
  if(fpos!=-1){
    fendpos=fpos+namedelimtersize;
    for(size_t dp=fendpos; dp<dislen; dp++){
      if(disposition[dp]=='\"'){
        fterm=dp;
        break;
      }
    }
  
    size_t namesize=(fterm-fendpos);
    char *name=new char[namesize+1];
    memcpy(name,disposition+fendpos,namesize);
    name[namesize]='\0';
    getContentDisposition()->setName(name);
    delete[] name;
  }
  
  const char *lowfilenamedelimter="filename=\"";
  const char* HIGHfilenamedelimter = "FILENAME=\"";
  ssize_t filenamedelimtersize=strlen(lowfilenamedelimter);
  ssize_t filepos=-1,fileendpos=0,filecurpos=0,fileterm=0;
  for(size_t dp=0; dp<dislen; dp++){
    if(lowfilenamedelimter[filecurpos]==disposition[dp] || 
        HIGHfilenamedelimter[filecurpos]==disposition[dp]){
        if(filecurpos==0){
          filepos=dp;
        }
        filecurpos++;
      }else{
        filecurpos=0;
        filepos=-1;
      }
      if(filecurpos==filenamedelimtersize)
        break;
  }
  if(filepos!=-1){
    fileendpos=filepos+filenamedelimtersize;
    for(size_t dp=fileendpos; dp<dislen; dp++){
      if(disposition[dp]=='\"'){
        fileterm=dp;
        break;
      }
    }
  
    size_t filenamesize=(fileterm-fileendpos);
    char *filename=new char[filenamesize+1];
    memcpy(filename,disposition+fileendpos,filenamesize);
    filename[filenamesize]='\0';
    getContentDisposition()->setFilename(filename);
    delete[] filename;
  }
}

void libhttppp::HttpForm::MultipartFormData::addContent(const char *key,const char *value){
  if(!_firstContent){
    _firstContent= new Content(key,value);
    _lastContent=_firstContent;
  }else{
    _lastContent->_nextContent=new Content(key,value);
    _lastContent=_lastContent->_nextContent;
  }
}

const char * libhttppp::HttpForm::MultipartFormData::getContent(const char* key){
  if(!key)
    return nullptr;
  for(Content *curcontent=_firstContent; curcontent; curcontent=curcontent->_nextContent){
    if(strcmp(curcontent->getKey(),key)==0){
      return curcontent->getValue();
    }
  }
  return nullptr;
}

const char  *libhttppp::HttpForm::MultipartFormData::getContentType(){
  return getContent("content-type");
}

libhttppp::HttpForm::MultipartFormData *libhttppp::HttpForm::MultipartFormData::nextMultipartFormData(){
   return _nextMultipartFormData; 
}

libhttppp::HttpForm::MultipartFormData::Content::Content(const char *key,const char *value){
  _nextContent=nullptr;
  if(!key || !value)
    return;
  
  _Key=new char[strlen(key)+1];
  memcpy(_Key,key,strlen(key));
  _Key[strlen(key)]='\0';
  
  _Value=new char[strlen(value)+1];
  memcpy(_Value,value,strlen(value));
  _Value[strlen(value)]='\0';
}

libhttppp::HttpForm::MultipartFormData::Content::~Content(){
  delete[] _Key;
  delete[] _Value; 
  delete _nextContent;
}

const char *libhttppp::HttpForm::MultipartFormData::Content::getKey(){
  return _Key;
}

const char *libhttppp::HttpForm::MultipartFormData::Content::getValue(){
  return _Value;
}

libhttppp::HttpForm::MultipartFormData::Content * libhttppp::HttpForm::MultipartFormData::Content::nextContent(){
  return _nextContent;
}

libhttppp::HttpForm::MultipartFormData::ContentDisposition::ContentDisposition(){
  _Disposition=nullptr;
  _Name=nullptr;
  _Filename=nullptr;
}

libhttppp::HttpForm::MultipartFormData::ContentDisposition::~ContentDisposition(){
  delete[] _Disposition;
  delete[] _Name;
  delete[] _Filename;  
}

char *libhttppp::HttpForm::MultipartFormData::ContentDisposition::getDisposition(){
  return _Disposition;    
}

char *libhttppp::HttpForm::MultipartFormData::ContentDisposition::getFilename(){
  return _Filename;  
}

char *libhttppp::HttpForm::MultipartFormData::ContentDisposition::getName(){
  return _Name;  
}

void libhttppp::HttpForm::MultipartFormData::ContentDisposition::setDisposition(const char* disposition){
  delete[] _Disposition;
  _Disposition=new char[strlen(disposition)+1];
  memcpy(_Disposition,disposition,strlen(disposition));
  _Disposition[strlen(disposition)]='\0';
}

void libhttppp::HttpForm::MultipartFormData::ContentDisposition::setName(const char* name){
  delete[] _Name;
  _Name=new char[strlen(name)+1];
  memcpy(_Name,name,strlen(name));
  _Name[strlen(name)]='\0';
}

void libhttppp::HttpForm::MultipartFormData::ContentDisposition::setFilename(const char* filename){
  delete[] _Filename;
  _Filename=new char[strlen(filename)+1];
  memcpy(_Filename,filename,strlen(filename));
  _Filename[strlen(filename)]='\0';
}

void libhttppp::HttpForm::_parseUrlDecode(libhttppp::HttpRequest* request){
  HTTPException httpexception;
  std::string formdat;
  if(request->getRequestType()==POSTREQUEST){
      size_t rsize=request->getRequestLength();
      formdat.append(request->getRequest(),rsize);
  }else if(request->getRequestType()==GETREQUEST){
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
         formdat.append(rurl+rdelimter,rurlsize-rdelimter);
      }
  }else{
    httpexception[HTTPException::Error] << "HttpForm unknown Requestype";
    throw httpexception;
  }
  size_t fdatstpos=0;
  size_t keyendpos=0;
  for(size_t fdatpos=0; fdatpos<=formdat.length(); fdatpos++){
    switch(formdat[fdatpos]){
        case '&': case '\0':{
          if(keyendpos >fdatstpos && keyendpos<fdatpos){
            std::string key;
            size_t vlstpos=keyendpos+1;
            std::string value;
            char *urldecdValue=nullptr;
            char *urldecdKey=nullptr;
            key=formdat.substr(fdatstpos,keyendpos-fdatstpos);
            if(vlstpos<=formdat.length()){
                value=formdat.substr(vlstpos,(fdatpos-vlstpos));
                urlDecode(value.c_str(),value.length(),&urldecdValue);
            }
            urlDecode(key.c_str(),key.length(),&urldecdKey);
            UrlcodedFormData *newenrty;
            newenrty=addUrlcodedFormData();
            newenrty->setKey(urldecdKey);
            newenrty->setValue(urldecdValue);

          }
          fdatstpos=fdatpos+1;
        };
        case '=':{
          keyendpos=fdatpos;  
        };
    }
  }
}

void libhttppp::HttpForm::UrlcodedFormData::setKey(const char* key){
   if(!key){
	HTTPException httpexception;
	httpexception[HTTPException::Error] << "no urldecoded key set";
	throw httpexception;
   }
   _Key=key;
}

void libhttppp::HttpForm::UrlcodedFormData::setValue(const char* value){
  if(!value){
      _Value.clear();
      return;
  }
  _Value=value; 
}

const char *libhttppp::HttpForm::UrlcodedFormData::getKey(){
  return _Key.c_str();
}

const char *libhttppp::HttpForm::UrlcodedFormData::getValue(){
  return _Value.c_str();
}

libhttppp::HttpForm::UrlcodedFormData  *libhttppp::HttpForm::UrlcodedFormData::nextUrlcodedFormData(){
  return _nextUrlcodedFormData;  
}

libhttppp::HttpForm::UrlcodedFormData::UrlcodedFormData(){
  _nextUrlcodedFormData=nullptr;
}

libhttppp::HttpForm::UrlcodedFormData::~UrlcodedFormData(){
  delete _nextUrlcodedFormData;
}

libhttppp::HttpForm::UrlcodedFormData  *libhttppp::HttpForm::addUrlcodedFormData(){
  if(!_firstUrlcodedFormData){
    _firstUrlcodedFormData= new UrlcodedFormData;
    _lastUrlcodedFormData=_firstUrlcodedFormData;
  }else{
    _lastUrlcodedFormData->_nextUrlcodedFormData=new UrlcodedFormData;
    _lastUrlcodedFormData=_lastUrlcodedFormData->_nextUrlcodedFormData;
  }
  return _lastUrlcodedFormData;
}

libhttppp::HttpForm::UrlcodedFormData  *libhttppp::HttpForm::getUrlcodedFormData(){
  return _firstUrlcodedFormData;
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

  std::string cdat;
  cdat = curreq->getData(hc);

  int delimeter=-1;
  int keyendpos=-1;
  int startpos=0;
  
  for (size_t cpos = 0; cpos < cdat.length(); cpos++) {
      if (cdat[cpos] == ' ') {
          ++startpos;
      }
      else if (cdat[cpos] == '=') {
          keyendpos = cpos;
      }else if (cdat[cpos] == ';'){
          delimeter = cpos;
      }else if (cpos == (cdat.length() - 1)) {
		  delimeter = cpos+1;
	  }
	  if (keyendpos != -1 && delimeter != -1) {
		  CookieData* curcookie = addCookieData();
          curcookie->_Key = cdat.substr(startpos, keyendpos-startpos);
		  curcookie->_Value = cdat.substr((keyendpos+1),delimeter-(keyendpos+1));
          keyendpos = -1;
          delimeter = -1;
          startpos = cpos+1;
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
