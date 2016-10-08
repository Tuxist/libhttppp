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

#include "http.h"
#include <algorithm>

#ifdef WIN32
#include "win32.h"
#endif

using namespace libhttppp;


HttpHeader::HttpHeader(){
  _firstHeaderData=NULL;
  _lastHeaderData=NULL;
}

HttpHeader::HeaderData* HttpHeader::getfirstHeaderData(){
  return _firstHeaderData;
}

HttpHeader::HeaderData* HttpHeader::nextHeaderData(HttpHeader::HeaderData* pos){
  return pos->_nextHeaderData;
}

const char* HttpHeader::getKey(HttpHeader::HeaderData* pos){
  return pos->_Key;
}

const char* HttpHeader::getValue(HttpHeader::HeaderData* pos){
  return pos->_Value;
}

const char* HttpHeader::getData(const char* key,HttpHeader::HeaderData **pos){
  for(HeaderData *curdat=_firstHeaderData; curdat; curdat=curdat->_nextHeaderData){
    if(strncmp(key,curdat->_Key,curdat->_Keylen)==0){
      if(pos!=NULL)
        *pos=curdat;
      return curdat->_Value;
    }
  }
  return NULL;
}

HttpHeader::HeaderData *HttpHeader::setData(const char* key, const char* value){
  for(HeaderData *curdat=_firstHeaderData; curdat; curdat=curdat->_nextHeaderData){
    if(strncmp(curdat->_Key,key,curdat->_Keylen)==0){
       return setData(key,value,curdat);
    }
  }
  if(!_firstHeaderData){
    _firstHeaderData=new HeaderData(key,value);
    _lastHeaderData=_firstHeaderData;
  }else{
    _lastHeaderData->_nextHeaderData=new HeaderData(key,value);
    _lastHeaderData=_lastHeaderData->_nextHeaderData;
  }
  return _lastHeaderData;
}

HttpHeader::HeaderData *HttpHeader::setData(const char* key, const char* value,
					    HttpHeader::HeaderData *pos){
  if(pos){
    delete[] pos->_Key;
    delete[] pos->_Value;
    pos->_Keylen=strlen(key);
    pos->_Valuelen=strlen(value);
    pos->_Key=new char[pos->_Keylen+1];
    pos->_Value=new char[pos->_Valuelen+1];
    std::copy(key,key+(pos->_Keylen+1),pos->_Key);
    std::copy(value,value+(pos->_Valuelen+1),pos->_Value);
    return pos;
  }else{
    setData(key,value);
  }
  return pos;
}

HttpHeader::HeaderData *HttpHeader::setData(const char* key, size_t value,
					    HttpHeader::HeaderData *pos){
  char buf[sizeof(size_t)+1];
  snprintf(buf, sizeof(size_t)+1, "%zu", value);
  return setData(key,buf,pos);
}

void HttpHeader::deldata(const char* key){
  HeaderData *prevdat=NULL;
  for(HeaderData *curdat=_firstHeaderData; curdat; curdat=curdat->_nextHeaderData){
    if(strncmp(curdat->_Key,key,curdat->_Keylen)==0){
      if(prevdat){
        prevdat->_nextHeaderData=curdat->_nextHeaderData;
        if(_lastHeaderData==curdat)
          _lastHeaderData=prevdat;
      }else{
        _firstHeaderData=curdat->_nextHeaderData;
        if(_lastHeaderData==curdat)
          _lastHeaderData=_firstHeaderData;
      }
      curdat->_nextHeaderData=NULL;
      delete curdat;
      return;
    }
    prevdat=curdat;
  }
}

size_t HttpHeader::getElements(){
  size_t elements=0;
  for(HeaderData *curdat=_firstHeaderData; curdat; curdat=curdat->_nextHeaderData){
    elements++;
  }
  return elements;
}

size_t HttpHeader::getHeaderSize(){
  size_t hsize=0;
  for(HeaderData *curdat=_firstHeaderData; curdat; curdat=curdat->_nextHeaderData){
    hsize+=curdat->_Keylen+curdat->_Valuelen;
  }
  return hsize;
}



HttpHeader::HeaderData::HeaderData(const char *key,const char*value){
  _nextHeaderData=NULL;
  _Keylen=strlen(key);
  _Valuelen=strlen(value);
  _Key=new char[_Keylen+1];
  _Value=new char[_Valuelen+1];
  std::copy(key,key+(_Keylen+1),_Key);
  std::copy(value,value+(_Valuelen+1),_Value);
}

HttpHeader::HeaderData::~HeaderData(){
  delete[] _Key;
  delete[] _Value;
  delete _nextHeaderData;
}

HttpHeader::~HttpHeader(){
  delete _firstHeaderData;
}

HttpResponse::HttpResponse(){
  setState(HTTP200);
  setVersion(HTTPVERSION("1.1"));
  _ContentType=setData("Content-Type","text/plain");
  _ContentLength=setData("Content-Length",(size_t)0);
  _Connection=setData("Connection","Keep-Alive");
}

void HttpResponse::setState(const char* httpstate){
  if(httpstate==NULL){
    _httpexception.Error("http state not set don't do that !!!");
    throw _httpexception;
  }
  size_t hlen=strlen(httpstate);
  if(hlen>254){
    _httpexception.Error("http state with over 255 signs sorry your are drunk !");
    throw _httpexception;
  }
  _Statelen=hlen;
  std::copy(httpstate,httpstate+hlen,_State);
  _State[hlen]='\0';
}

void HttpResponse::setContentLength(size_t len){
  setData("Content-Length",len,_ContentLength);
}

void HttpResponse::setContentType(const char* type){
  setData("Content-Type",type,_ContentType);
}

void HttpResponse::setConnection(const char* type){
  setData("Connection",type,_ContentType);
}

void HttpResponse::setVersion(const char* version){
  if(version==NULL)
    throw "http version not set don't do that !!!";
  size_t vlen=strlen(version);
  if(vlen>254)
    throw "http version with over 255 signs sorry your are drunk !";
  _VersionLen=vlen;
  std::copy(version,version+vlen,_Version);
  _Version[vlen]='\0';
}

void HttpResponse::parse(ClientConnection *curconnection){
}

void HttpResponse::send(Connection* curconnection, const char* data){
  send(curconnection,data,strlen(data));
}

size_t HttpResponse::printHeader(char **buffer){
  char *header=NULL;
  size_t headersize=((getHeaderSize()+_VersionLen+_Statelen)+((4*getElements())+5)); 
  header=new char[(headersize+1)];
  size_t pos=snprintf(header,headersize,"%s %s\r\n",_Version,_State);
  for(HeaderData *curdat=getfirstHeaderData(); curdat; curdat=nextHeaderData(curdat)){ 
    pos+=(snprintf(header+pos,(headersize-pos), 
           "%s: %s\r\n",getKey(curdat),getValue(curdat)));
  } 
  pos+=snprintf(header+pos,headersize+1,"\r\n");
  *buffer=header;
  return headersize;
}


void HttpResponse::send(Connection* curconnection,const char* data, size_t datalen){
  setData("Connection","keep-alive");
  setData("Content-Length",datalen);
  char *header;
  size_t headersize = printHeader(&header);
  curconnection->addSendQueue(header,headersize);
  delete[] header;
  if(datalen!=0)
    curconnection->addSendQueue(data,datalen);
}

HttpResponse::~HttpResponse(){
}

HttpRequest::HttpRequest(){
  _Request=NULL;
}

void HttpRequest::parse(Connection* curconnection){
  try{
    ConnectionData *curdat=curconnection->getRecvData();
    if(curdat){
      ConnectionData *startblock;
      int startpos=0;
      if((startpos=curconnection->searchValue(curdat,&startblock,"GET",3))!=-1){
        _RequestType=GETREQUEST;
      }else if((startpos=curconnection->searchValue(curdat,&startblock,"POST",4))!=-1){
        _RequestType=POSTREQUEST;
      }else{
        _httpexception.Warning("Requesttype not known cleanup");
        curconnection->cleanRecvData();
        throw _httpexception;
      }
      ConnectionData *endblock;
      int endpos=curconnection->searchValue(startblock,&endblock,"\r\n\r\n",4);
      if(endpos==-1){
	int endpos=curconnection->searchValue(startblock,&endblock,"\n\n",2);
	if(endpos==-1){
          _httpexception.Note("Request not complete termination not found");
          throw _httpexception;
	}
      }
          
      char *buffer;
      size_t buffersize=curconnection->copyValue(startblock,startpos,endblock,endpos+1,&buffer);
      curconnection->resizeRecvQueue(buffersize);
      if(sscanf(buffer,"%*s %s[255] %s[255]",_RequestURL,_Version)==-1){
	 _httpexception.Error("can't parse http head");
         throw _httpexception;
      }
      size_t lrow=0;
      size_t delimeter=0;
      for(size_t pos=0; pos<buffersize; pos++){
	  if(delimeter==0 && buffer[pos]==':'){
	    delimeter=pos; 
	  }
	  if(buffer[pos]=='\r'){
	    if(delimeter>lrow){
	      size_t keylen=delimeter-lrow;
	      if(keylen>0 && keylen <=buffersize){
		char *key=NULL;
		key=new char[keylen+1];
		std::copy(buffer+lrow,buffer+(lrow+keylen),key);
		key[keylen]='\0';
		size_t valuelen=pos-delimeter;
		if(pos > 0 && valuelen <= buffersize){
		  char *value=NULL;
		  value=new char[valuelen+1];
		  std::copy(buffer+delimeter,buffer+(delimeter+valuelen),value);
		  value[valuelen]='\0';
 		  setData(key+2,value+2);
		  delete[] value;
		}
		delete[] key;
	      }
	    }
	    delimeter=0;
	    lrow=pos;
	  }
      }
      delete[] buffer;
    }else{
      _httpexception.Note("No Incoming data in queue");
      throw _httpexception;
    }
  }catch(HTTPException &e){
    throw e;
  }
}

void HttpRequest::send(ClientConnection *curconnection){

}

int HttpRequest::getRequestType(){
  return _RequestType;
}

const char* HttpRequest::getRequestURL(){
  return _RequestURL;
}

HttpRequest::~HttpRequest(){
  if(_Request)
    delete[] _Request;
}

HttpForm::HttpForm(HttpRequest* request){
  _Request=request;
  int rtype = _Request->getRequestType();
  switch(rtype){
    case GETREQUEST:{
      printf("GETREQUEST");
    }
    case POSTREQUEST:{
      printf("GETREQUEST");
    }
  }
}
