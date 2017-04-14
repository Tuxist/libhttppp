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
#include <cstring>

#ifdef Windows
  #define strtok_r strtok_s
#endif


libhttppp::HttpHeader::HttpHeader(){
  _firstHeaderData=NULL;
  _lastHeaderData=NULL;
}

libhttppp::HttpHeader::HeaderData* libhttppp::HttpHeader::getfirstHeaderData(){
  return _firstHeaderData;
}

libhttppp::HttpHeader::HeaderData* libhttppp::HttpHeader::nextHeaderData(HttpHeader::HeaderData* pos){
  return pos->_nextHeaderData;
}

const char* libhttppp::HttpHeader::getKey(HttpHeader::HeaderData* pos){
  return pos->_Key;
}

const char* libhttppp::HttpHeader::getValue(HttpHeader::HeaderData* pos){
  return pos->_Value;
}

const char* libhttppp::HttpHeader::getData(const char* key,HttpHeader::HeaderData **pos){
  for(HeaderData *curdat=_firstHeaderData; curdat; curdat=curdat->_nextHeaderData){
    if(strncmp(key,curdat->_Key,curdat->_Keylen)==0){
      if(pos!=NULL)
        *pos=curdat;
      return curdat->_Value;
    }
  }
  return NULL;
}

size_t libhttppp::HttpHeader::getDataSizet(const char *key,HttpHeader::HeaderData **pos){
  size_t len = 0;
  const char *buf=getData(key,pos);
  if(buf){
    if (1 == sscanf(buf, "%zu", &len))
      return len;
  }
  return 0;
}

int libhttppp::HttpHeader::getDataInt(const char *key,HttpHeader::HeaderData **pos){
  int len = 0;
  const char *buf=getData(key,pos);
  if(buf){
    if (1 == sscanf(buf, "%d", &len))
      return len;
  }
  return 0;
}

libhttppp::HttpHeader::HeaderData *libhttppp::HttpHeader::setData(const char* key, const char* value){
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

libhttppp::HttpHeader::HeaderData *libhttppp::HttpHeader::setData(const char* key, const char* value,
								  libhttppp::HttpHeader::HeaderData *pos){
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

libhttppp::HttpHeader::HeaderData *libhttppp::HttpHeader::setData(const char* key, size_t value,
								  libhttppp::HttpHeader::HeaderData *pos){
  char buf[255];
  snprintf(buf, sizeof(buf), "%zu", value);
  return setData(key,buf,pos);
}

void libhttppp::HttpHeader::deldata(const char* key){
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
    hsize+=curdat->_Keylen+curdat->_Valuelen;
  }
  return hsize;
}



libhttppp::HttpHeader::HeaderData::HeaderData(const char *key,const char*value){
  _nextHeaderData=NULL;
  _Keylen=strlen(key);
  _Valuelen=strlen(value);
  _Key=new char[_Keylen+1];
  _Value=new char[_Valuelen+1];
  std::copy(key,key+(_Keylen+1),_Key);
  std::copy(value,value+(_Valuelen+1),_Value);
}

libhttppp::HttpHeader::HeaderData::~HeaderData(){
  delete[] _Key;
  delete[] _Value;
  delete _nextHeaderData;
}

libhttppp::HttpHeader::~HttpHeader(){
  delete _firstHeaderData;
}

libhttppp::HttpResponse::HttpResponse(){
  setState(HTTP200);
  setVersion(HTTPVERSION("1.1"));
  _ContentType=setData("Content-Type","text/plain");
  _ContentLength=setData("Content-Length",(size_t)0);
  _Connection=setData("Connection","Keep-Alive");
}

void libhttppp::HttpResponse::setState(const char* httpstate){
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

void libhttppp::HttpResponse::setContentLength(size_t len){
  setData("Content-Length",len,_ContentLength);
}

void libhttppp::HttpResponse::setContentType(const char* type){
  setData("Content-Type",type,_ContentType);
}

void libhttppp::HttpResponse::setConnection(const char* type){
  setData("Connection",type,_ContentType);
}

void libhttppp::HttpResponse::setVersion(const char* version){
  if(version==NULL)
    throw "http version not set don't do that !!!";
  size_t vlen=strlen(version);
  if(vlen>254)
    throw "http version with over 255 signs sorry your are drunk !";
  _VersionLen=vlen;
  std::copy(version,version+vlen,_Version);
  _Version[vlen]='\0';
}

void libhttppp::HttpResponse::parse(ClientConnection *curconnection){
}

void libhttppp::HttpResponse::send(Connection* curconnection, const char* data){
  send(curconnection,data,strlen(data));
}

size_t libhttppp::HttpResponse::printHeader(char **buffer){
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


void libhttppp::HttpResponse::send(Connection* curconnection,const char* data, size_t datalen){
  setData("Connection","keep-alive");
  setData("Content-Length",datalen);
  char *header;
  size_t headersize = printHeader(&header);
  curconnection->addSendQueue(header,headersize);
  delete[] header;
  if(datalen!=0)
    curconnection->addSendQueue(data,datalen);
}

libhttppp::HttpResponse::~HttpResponse(){
}

libhttppp::HttpRequest::HttpRequest(){
  _Request=NULL;
  _RequestType=0;
  _RequestSize=0;
}

void libhttppp::HttpRequest::parse(Connection* curconnection){
  try{
    ConnectionData *curdat=curconnection->getRecvData();
    
    if(curdat){
      ConnectionData *startblock;
      int startpos=0;
      
      if((startpos=curconnection->searchValue(curdat,&startblock,"GET",3))==0 && startblock==curdat){
        _RequestType=GETREQUEST;
      }else if((startpos=curconnection->searchValue(curdat,&startblock,"POST",4))==0 && startblock==curdat){
        _RequestType=POSTREQUEST;
      }else{
        _httpexception.Warning("Requesttype not known cleanup");
        curconnection->cleanRecvData();
        throw _httpexception;
      }
      ConnectionData *endblock;
      ssize_t endpos=curconnection->searchValue(startblock,&endblock,"\r\n\r\n",4);
      if(endpos==-1){
	 _httpexception.Error("can't find newline headerend");
         throw _httpexception;
      }
      endpos+=4;  
      
          
      char *header;
      size_t headersize=curconnection->copyValue(startblock,startpos,endblock,endpos,&header);
      if(sscanf(header,"%*s %s[255] %s[255]",_RequestURL,_Version)==-1){
	 _httpexception.Error("can't parse http head");
         throw _httpexception;
      }
      
      /*parse the http header fields*/
      size_t lrow=0,delimeter=0,startkeypos=0;
      
      for(size_t pos=0; pos<headersize; pos++){
	  if(delimeter==0 && header[pos]==':'){
	    delimeter=pos; 
	  }
	  if(header[pos]=='\r'){
	    if(delimeter>lrow && delimeter!=0){
	      size_t keylen=delimeter-startkeypos;
	      if(keylen>0 && keylen <=headersize){
		char *key=new char[keylen+1];
		std::copy(header+startkeypos,header+(startkeypos+keylen),key);
		key[keylen]='\0';
		size_t valuelen=(pos-delimeter)-2;
		if(pos > 0 && valuelen <= headersize){
		  char *value=new char[valuelen+1];
                  size_t vstart=delimeter+2;
		  std::copy(header+vstart,header+(vstart+valuelen),value);
		  value[valuelen]='\0';
 		  setData(key,value);
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

      delete[] header;
      
      if(_RequestType==POSTREQUEST){
        size_t csize=getDataSizet("Content-Length");
        size_t rsize=curconnection->getRecvSize()-headersize;
        if(csize<=rsize){
          curconnection->resizeRecvQueue(headersize);
          size_t dlocksize=curconnection->getRecvSize();
          ConnectionData *dblock=NULL;
          size_t cdlocksize=0;
          for(dblock=curconnection->getRecvData(); dblock; dblock=dblock->nextConnectionData()){
            dlocksize-=dblock->getDataSize();
            cdlocksize+=dblock->getDataSize();
            if(csize<cdlocksize){
                 break;
            }
          }
          size_t rcsize=curconnection->copyValue(curconnection->getRecvData(),0,dblock,dlocksize,&_Request);
          curconnection->resizeRecvQueue(rcsize);
	  _RequestSize=rcsize;
        }else{
          _httpexception.Note("Request incomplete");
          throw _httpexception;
        }
      }else{
        curconnection->resizeRecvQueue(headersize);  
      }
    }else{
      _httpexception.Note("No Incoming data in queue");
      throw _httpexception;
    }
  }catch(HTTPException &e){
    throw e;
  }
}

void libhttppp::HttpRequest::send(ClientConnection *curconnection){

}

int libhttppp::HttpRequest::getRequestType(){
  return _RequestType;
}

const char* libhttppp::HttpRequest::getRequestURL(){
  return _RequestURL;
}

const char* libhttppp::HttpRequest::getRequest(){
  return _Request;  
}

size_t libhttppp::HttpRequest::getRequestSize(){
  return _RequestSize;  
}

libhttppp::HttpRequest::~HttpRequest(){
  delete[] _Request;
}

libhttppp::HttpForm::HttpForm(){
  _Boundary=NULL;
  _Elements=0;
  _firstMultipartFormData=NULL;
  _lastMultipartFormData=NULL;
}

libhttppp::HttpForm::~HttpForm(){
  delete[] _Boundary;
  delete   _firstMultipartFormData;
  _lastMultipartFormData=NULL;
}


void libhttppp::HttpForm::parse(libhttppp::HttpRequest* request){
  int rtype = request->getRequestType();
  _ContentType = request->getData("Content-Type");
  switch(rtype){
    case GETREQUEST:{
      _parseUrlDecode(request);
      break;
    }
    case POSTREQUEST:{
      if(request->getData("Content-Type") && 
         strncmp(request->getData("Content-Type"),"multipart/form-data",18)==0){
         _parseMulitpart(request);
      }else{
         _parseUrlDecode(request);
      }
      break;
    }
  }
}

const char *libhttppp::HttpForm::getContentType(){
  return _ContentType;;
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
  const char *boundary="boundary=\0";
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
  std::copy(contenttype+ctstartpos,contenttype+ctendpos,_Boundary);
  _Boundary[(ctendpos-ctstartpos)]='\0';
}

void libhttppp::HttpForm::_parseMulitpart(libhttppp::HttpRequest* request){
  _parseBoundary(request->getData("Content-Type"));
  char *realboundary;
  realboundary =new char[_BoundarySize+3];
  snprintf(realboundary,_BoundarySize+3,"--%s",_Boundary);
  size_t realboundarylen=_BoundarySize+2;
  const char *req=request->getRequest();
  size_t reqsize=request->getRequestSize();
  size_t realboundarypos=0;
  unsigned int datalength = 0;
  const char *datastart=0;
  size_t oldpos=0; 
  for(size_t cr=0; cr < reqsize; cr++){
    //check if boundary
    if(req[cr]==realboundary[realboundarypos]){
      //check if boundary completed
      if((realboundarypos+1)==realboundarylen){
	//ceck if boundary before found set data end
	if(datastart!=NULL){
        //cut boundarylen
	  datalength=(cr-oldpos)-realboundarylen;
	  if(datalength>0)
	    _parseMultiSection(datastart,datalength);
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
      }else{
        //boundary not completed test next sign
	realboundarypos++;
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
          std::copy(section+startkeypos,section+(startkeypos+keylen),key);
          key[keylen]='\0';
          size_t valuelen=((pos-delimeter)-2);
          if(pos > 0 && valuelen <= sectionsize){
            char *value=new char[valuelen+1];
            size_t vstart=delimeter+2;
            std::copy(section+vstart,section+(vstart+valuelen),value);
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
    curmultipartformdata->getContent("Content-Disposition")
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
  _nextMultipartFormData=NULL;
  _Data=NULL;
  _firstContent=NULL;
  _lastContent=NULL;
}

libhttppp::HttpForm::MultipartFormData::~MultipartFormData(){
  delete _ContentDisposition;
  delete _nextMultipartFormData;
  delete _firstContent;
  _lastContent=NULL;
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
      std::copy(disposition,disposition+dpos,ctype);
      ctype[dpos]='\0';
      getContentDisposition()->setDisposition(ctype);
      delete[] ctype;
      break;
    }
  }
  
  const char *namedelimter="name=\"";
  ssize_t namedelimtersize=strlen(namedelimter);
  ssize_t fpos=-1,fendpos=0,fcurpos=0,fterm=0;
  for(size_t dp=0; dp<dislen; dp++){
    if(namedelimter[fcurpos]==disposition[dp]){
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
    std::copy(disposition+fendpos,disposition+(fendpos+namesize),name);
    name[namesize]='\0';
    getContentDisposition()->setName(name);
    delete[] name;
  }
  
  const char *filenamedelimter="filename=\"";
  ssize_t filenamedelimtersize=strlen(filenamedelimter);
  ssize_t filepos=-1,fileendpos=0,filecurpos=0,fileterm=0;
  for(size_t dp=0; dp<dislen; dp++){
    if(filenamedelimter[filecurpos]==disposition[dp]){
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
    std::copy(disposition+fileendpos,disposition+(fileendpos+filenamesize),filename);
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
    return NULL;
  for(Content *curcontent=_firstContent; curcontent; curcontent=curcontent->_nextContent){
    if(strncmp(curcontent->getKey(),key,strlen(key))==0){
      return curcontent->getValue();
    }
  }
  return NULL;
}

const char  *libhttppp::HttpForm::MultipartFormData::getContentType(){
  return getContent("Content-Type");
}

libhttppp::HttpForm::MultipartFormData *libhttppp::HttpForm::MultipartFormData::nextMultipartFormData(){
   return _nextMultipartFormData; 
}

libhttppp::HttpForm::MultipartFormData::Content::Content(const char *key,const char *value){
  _nextContent=NULL;
  if(!key || !value)
    return;
  
  _Key=new char[strlen(key)+1];
  std::copy(key,key+strlen(key),_Key);
  _Key[strlen(key)]='\0';
  
  _Value=new char[strlen(value)+1];
  std::copy(value,value+strlen(value),_Value);
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
  _Disposition=NULL;
  _Name=NULL;
  _Filename=NULL;
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
  std::copy(disposition,disposition+strlen(disposition),_Disposition);
  _Disposition[strlen(disposition)]='\0';
}

void libhttppp::HttpForm::MultipartFormData::ContentDisposition::setName(const char* name){
  delete[] _Name;
  _Name=new char[strlen(name)+1];
  std::copy(name,name+strlen(name),_Name);
  _Name[strlen(name)]='\0';
}

void libhttppp::HttpForm::MultipartFormData::ContentDisposition::setFilename(const char* filename){
  delete[] _Filename;
  _Filename=new char[strlen(filename)+1];
  std::copy(filename,filename+strlen(filename),_Filename);
  _Filename[strlen(filename)]='\0';
}

void libhttppp::HttpForm::_parseUrlDecode(libhttppp::HttpRequest* request){
  char *formdat=NULL;
  if(request->getRequestType()==POSTREQUEST){
      size_t rsize=request->getRequestSize();
      formdat = new char[rsize+1];
      std::copy(request->getRequest(),request->getRequest()+rsize,formdat);
      formdat[rsize]='\0';
  }else if(request->getRequestType()==GETREQUEST){
      const char *rurl=request->getRequestURL();
      size_t rurlsize=strlen(rurl);
      ssize_t rdelimter=-1;
      for(size_t cpos=0; cpos<rurlsize; cpos++){
        if(rurl[cpos]=='?'){
          rdelimter=cpos;
          rdelimter++;
          break;
        }
      }
      if(rdelimter==-1){
        _httpexception.Note("Get Request include no data");
        throw _httpexception;
      }
      size_t rsize=rurlsize-rdelimter;
      formdat = new char[rsize+1];
      std::copy(rurl+rdelimter,rurl+(rdelimter+rsize),formdat);
      formdat[rsize]='\0';
  }else{
    _httpexception.Error("HttpForm unknown Requestype");
    throw _httpexception;
  }

  char *subptr = NULL,*subptr2=NULL;
  for(char *substr = strtok_r(formdat,"&",&subptr); substr; substr=strtok_r(NULL,"&",&subptr)){
    char *key = strtok_r(substr,"=",&subptr2);
    char *value = strtok_r(NULL,"=",&subptr2);
    printf("%s -> %s\n",key,value);
  }
  
  printf("formdat: %s \n",formdat);
  delete[] formdat;
}


libhttppp::HttpCookie::CookieData::CookieData(){
}

libhttppp::HttpCookie::CookieData::~CookieData(){
}

void libhttppp::HttpCookie::parse(libhttppp::HttpRequest* curreq){
}
