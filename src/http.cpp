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
#include "base64.h"
#include "utils.h"

libhttppp::HttpHeader::HttpHeader(){
  _firstHeaderData=nullptr;
  _lastHeaderData=nullptr;
}

libhttppp::HttpHeader::HeaderData &libhttppp::HttpHeader::HeaderData::operator<<(const char* value){
    if(!value)
        return *this;
    size_t nsize=getlen(value)+_Valuelen;
    char *buf=new char [nsize+1];
    if(_Valuelen>0)
        scopy(_Value,_Value+_Valuelen,buf);
    scopy(value,value+getlen(value),buf+_Valuelen);
    _Valuelen=nsize;
    delete[] _Value;
    buf[nsize]='\0';
    _Value=buf;
    return *this;
}

libhttppp::HttpHeader::HeaderData &libhttppp::HttpHeader::HeaderData::operator<<(size_t value){
  char buf[255];
  ultoa(value,buf);
  *this<<buf;
  return *this;
}

libhttppp::HttpHeader::HeaderData &libhttppp::HttpHeader::HeaderData::operator<<(int value){
  char buf[255];
  itoa(value,buf);
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
  return pos->_Key;
}

const char* libhttppp::HttpHeader::getValue(HttpHeader::HeaderData* pos){
  return pos->_Value;
}

const char* libhttppp::HttpHeader::getData(const char* key,HttpHeader::HeaderData **pos){
  for(HeaderData *curdat=_firstHeaderData; curdat; curdat=curdat->_nextHeaderData){
    if(libhttppp::ncompare(key,libhttppp::getlen(key),
                            curdat->_Key,curdat->_Keylen)==0){
      if(pos!=NULL)
        *pos=curdat;
      return curdat->_Value;
    }
  }
  return NULL;
}

size_t libhttppp::HttpHeader::getDataSizet(const char *key,HttpHeader::HeaderData **pos){
  const char *val=getData(key,pos);
  if(getlen(val)<255)
      return 0;
  char buf[255];
  scopy(val,val+getlen(val),buf);
  return atoi(buf);
}

int libhttppp::HttpHeader::getDataInt(const char *key,HttpHeader::HeaderData **pos){
  const char *val=getData(key,pos);
  if(getlen(val)<255)
      return 0;
  char buf[255];
  scopy(val,val+getlen(val),buf);
  return atoi(buf);
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

libhttppp::HttpHeader::HeaderData *libhttppp::HttpHeader::setData(const char* key,libhttppp::HttpHeader::HeaderData *pos){
  if(!key){
    _httpexception[HTTPException::Error] << "no headerdata key set can't do this";
    throw _httpexception;
  }
  if(pos){
    delete[] pos->_Key;
    delete[] pos->_Value;
    pos->_Keylen=getlen(key);
    pos->_Key=new char[pos->_Keylen+1];
    scopy(key,key+(pos->_Keylen+1),pos->_Key);
    pos->_Value=nullptr;
    pos->_Valuelen=0;
    return pos;
  }else{
    return setData(key);
  }
}

void libhttppp::HttpHeader::deldata(const char* key){
  for(HeaderData *curdat=_firstHeaderData; curdat; curdat=curdat->_nextHeaderData){
    if(libhttppp::ncompare(curdat->_Key,curdat->_Keylen,key,
        libhttppp::getlen(key))==0){
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
    hsize+=curdat->_Keylen+curdat->_Valuelen;
  }
  return hsize;
}


libhttppp::HttpHeader::HeaderData::HeaderData(const char *key){
    HTTPException excep;
    if(!key){
        excep[HTTPException::Error] << "no headerdata key set can't do this";
        throw excep;
    }
    _nextHeaderData=NULL;
    _Keylen=getlen(key);
    _Key=new char[_Keylen+1];
    scopy(key,key+(_Keylen+1),_Key);
    _Valuelen=0;
    _Value=nullptr;
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
  setVersion(HTTPVERSION("2.0"));
  _ContentType=NULL;
  _ContentLength=NULL;
  _Connection=setData("Connection");
  *_Connection<<"keep-alive";
}

void libhttppp::HttpResponse::setState(const char* httpstate){
  if(httpstate==NULL){
    _httpexception[HTTPException::Error] << "http state not set don't do that !!!";
    throw _httpexception;
  }
  size_t hlen=getlen(httpstate);
  if(hlen>254){
    _httpexception[HTTPException::Error] << "http state with over 255 signs sorry your are drunk !";
    throw _httpexception;
  }
  _Statelen=hlen;
  scopy(httpstate,httpstate+hlen,_State);
  _State[hlen]='\0';
}

void libhttppp::HttpResponse::setContentLength(size_t len){
  _ContentLength=setData("content-length",_ContentLength);
  *_ContentLength<<len;
}

void libhttppp::HttpResponse::setContentType(const char* type){
  _ContentType=setData("content-type",_ContentType);
  *_ContentType<<type;
}

void libhttppp::HttpResponse::setConnection(const char* type){
  _ContentLength=setData("connection",_ContentType);
  *_ContentLength<<type;
}

void libhttppp::HttpResponse::setVersion(const char* version){
  HTTPException excep;  
  if(version==NULL)
    throw excep[HTTPException::Error] << "http version not set don't do that !!!";
  size_t vlen=getlen(version);
  if(vlen>254)
    throw excep[HTTPException::Error] << "http version with over 255 signs sorry your are drunk !";
  _VersionLen=vlen;
  scopy(version,version+vlen,_Version);
  _Version[vlen]='\0';
}

void libhttppp::HttpResponse::parse(ClientConnection *curconnection){
}

void libhttppp::HttpResponse::send(Connection* curconnection, const char* data){
  send(curconnection,data,getlen(data));
}

size_t libhttppp::HttpResponse::printHeader(char **buffer){
    *buffer=new char[getlen(_Version)+1];
    scopy(_Version,_Version+(getlen(_Version)+1),*buffer);
    append(buffer," ");
    append(buffer,_State);
    append(buffer,"\r\n");
    for(HeaderData *curdat=getfirstHeaderData(); curdat; curdat=nextHeaderData(curdat)){ 
        append(buffer,getKey(curdat));
        append(buffer,": ");
        if(getValue(curdat))
            append(buffer,getValue(curdat));
        append(buffer,"\r\n");
    } 
    append(buffer,"\r\n");
    return getlen((*buffer));
}


void libhttppp::HttpResponse::send(Connection* curconnection,const char* data, unsigned long datalen){
  if(datalen>=0){
        setContentLength(datalen);
  }
  char *header;
  size_t headersize = printHeader(&header);
  curconnection->addSendQueue(header,headersize);
  delete[] header;
  if(datalen>=0)
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
    HTTPException excep;
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
                excep[HTTPException::Warning] << "Requesttype not known cleanup";
                curconnection->cleanRecvData();
                throw excep;
            }
            ConnectionData *endblock;
            ssize_t endpos=curconnection->searchValue(startblock,&endblock,"\r\n\r\n",4);
            if(endpos==-1){
                excep[HTTPException::Error] << "can't find newline headerend";
                throw excep;
            }
            endpos+=4;  
            
            
            char *header;
            size_t headersize=curconnection->copyValue(startblock,startpos,endblock,endpos,&header);
            
            bool found=false;
            int pos;

            for(pos=0; pos<headersize; ++pos){
                if(header[pos]==' '){
                    ++pos;
                    break;
                }
            }

            for(; pos<headersize; ++pos){
                if(header[pos]==' ' && (headersize-pos)<255){
                    scopy(header,header+pos,_RequestURL);
                    ++pos;
                    break;
                }
            }

            for(; pos<headersize; ++pos){
                if(header[pos]==' ' && (headersize-pos)<255){
                    scopy(header,header+pos,_Version);
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
            
            for(size_t pos=0; pos<headersize; pos++){
                if(delimeter==0 && header[pos]==':'){
                    delimeter=pos; 
                }
                if(header[pos]=='\r'){
                    if(delimeter>lrow && delimeter!=0){
                        size_t keylen=delimeter-startkeypos;
                        if(keylen>0 && keylen <=headersize){
                            char *key=new char[keylen+1];
                            scopy(header+startkeypos,header+(startkeypos+keylen),key);
                            key[keylen]='\0';
                            size_t valuelen=(pos-delimeter)-2;
                            if(pos > 0 && valuelen <= headersize){
                                char *value=new char[valuelen+1];
                                size_t vstart=delimeter+2;
                                scopy(header+vstart,header+(vstart+valuelen),value);
                                value[valuelen]='\0';
                                *setData(key)<<value;
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
            
            curconnection->resizeRecvQueue(headersize);
            
            if(_RequestType==POSTREQUEST){
                size_t csize=getDataSizet("content-length");
                size_t rsize=curconnection->getRecvSize()-headersize;
                if(csize<=rsize){
                    size_t dlocksize=curconnection->getRecvSize();
                    ConnectionData *dblock=NULL;
                    size_t cdlocksize=0;
                    for(dblock=curconnection->getRecvData(); dblock; dblock=dblock->nextConnectionData()){
                        dlocksize-=dblock->getDataSize();
                        cdlocksize+=dblock->getDataSize();
                        if(csize>=cdlocksize){
                            break;
                        }
                    }
                    size_t rcsize=curconnection->copyValue(curconnection->getRecvData(),0,dblock,dlocksize,&_Request);
                    curconnection->resizeRecvQueue(rcsize);
                    _RequestSize=rcsize;
                }else{
                    excep[HTTPException::Note] << "Request incomplete";
                    throw excep;
                }
            }
        }else{
            excep[HTTPException::Note] << "No Incoming data in queue";
            throw excep;
        }
    }catch(HTTPException &e){
        curconnection->cleanRecvData();
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
  _firstUrlcodedFormData=NULL;
  _lastUrlcodedFormData=NULL;
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
         libhttppp::ncompare(request->getData("Content-Type"),12,"multipart/form-data",18)==0){
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
        }else if (c == '%' && (!_ishex(*urlin++)|| !_ishex(*urlin++)	/*||*/
            /*!sscanf(urlin - 2, "%2x", &c)*/)){
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
  const char *boundary="boundary=\0";
  size_t bdpos=0;
  for(size_t cpos=0; cpos<getlen(contenttype); cpos++){
    if(bdpos==(getlen(boundary)-1)){
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
  for(size_t cpos=ctstartpos; cpos<getlen(contenttype); cpos++){
      if(contenttype[cpos]==';'){
          ctendpos=cpos;
      }
  }
  if(ctendpos==0)
    ctendpos=getlen(contenttype);
  /*cut boundary=*/
  ctstartpos+=getlen(boundary);
  if(_Boundary)
    delete[] _Boundary;
  _BoundarySize=(ctendpos-ctstartpos);
  _Boundary=new char[_BoundarySize+1];
  scopy(contenttype+ctstartpos,contenttype+ctendpos,_Boundary);
  _Boundary[(ctendpos-ctstartpos)]='\0';
}

void libhttppp::HttpForm::_parseMulitpart(libhttppp::HttpRequest* request){
  _parseBoundary(request->getData("Content-Type"));
  char *realboundary = new char[_BoundarySize+3];
  scopy(realboundary+2,realboundary+(_BoundarySize+1),_Boundary);
  realboundary[0]='-';
  realboundary[1]='-';
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
  size_t windelimtersize=getlen(windelimter);
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
          scopy(section+startkeypos,section+(startkeypos+keylen),key);
          key[keylen]='\0';
          size_t valuelen=((pos-delimeter)-2);
          if(pos > 0 && valuelen <= sectionsize){
            char *value=new char[valuelen+1];
            size_t vstart=delimeter+2;
            scopy(section+vstart,section+(vstart+valuelen),value);
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
  size_t dislen=getlen(disposition);
  
  for(size_t dpos=0; dpos<dislen; dpos++){
    if(disposition[dpos]==';' || disposition[dpos]=='\r'){
      char *ctype = new char[dpos+1];
      scopy(disposition,disposition+dpos,ctype);
      ctype[dpos]='\0';
      getContentDisposition()->setDisposition(ctype);
      delete[] ctype;
      break;
    }
  }
  
  const char *namedelimter="name=\"";
  ssize_t namedelimtersize=getlen(namedelimter);
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
    scopy(disposition+fendpos,disposition+(fendpos+namesize),name);
    name[namesize]='\0';
    getContentDisposition()->setName(name);
    delete[] name;
  }
  
  const char *filenamedelimter="filename=\"";
  ssize_t filenamedelimtersize=getlen(filenamedelimter);
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
    scopy(disposition+fileendpos,disposition+(fileendpos+filenamesize),filename);
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
    if(libhttppp::ncompare(curcontent->getKey(),
        libhttppp::getlen(curcontent->getKey()),key,getlen(key))==0){
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
  
  _Key=new char[getlen(key)+1];
  scopy(key,key+getlen(key),_Key);
  _Key[getlen(key)]='\0';
  
  _Value=new char[getlen(value)+1];
  scopy(value,value+getlen(value),_Value);
  _Value[getlen(value)]='\0';
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
  _Disposition=new char[getlen(disposition)+1];
  scopy(disposition,disposition+getlen(disposition),_Disposition);
  _Disposition[getlen(disposition)]='\0';
}

void libhttppp::HttpForm::MultipartFormData::ContentDisposition::setName(const char* name){
  delete[] _Name;
  _Name=new char[getlen(name)+1];
  scopy(name,name+getlen(name),_Name);
  _Name[getlen(name)]='\0';
}

void libhttppp::HttpForm::MultipartFormData::ContentDisposition::setFilename(const char* filename){
  delete[] _Filename;
  _Filename=new char[getlen(filename)+1];
  scopy(filename,filename+getlen(filename),_Filename);
  _Filename[getlen(filename)]='\0';
}

void libhttppp::HttpForm::_parseUrlDecode(libhttppp::HttpRequest* request){
  char *formdat=nullptr;
  if(request->getRequestType()==POSTREQUEST){
      size_t rsize=request->getRequestSize();
      formdat = new char[rsize+1];
      scopy(request->getRequest(),request->getRequest()+rsize,formdat);
      formdat[rsize]='\0';
  }else if(request->getRequestType()==GETREQUEST){
      const char *rurl=request->getRequestURL();
      size_t rurlsize=getlen(rurl);
      ssize_t rdelimter=-1;
      for(size_t cpos=0; cpos<rurlsize; cpos++){
        if(rurl[cpos]=='?'){
          rdelimter=cpos;
          rdelimter++;
          break;
        }
      }
      if(rdelimter==-1){
        return;
      }
      size_t rsize=rurlsize-rdelimter;
      formdat = new char[rsize+1];
      scopy(rurl+rdelimter,rurl+(rdelimter+rsize),formdat);
      formdat[rsize]='\0';
  }else{
    _httpexception[HTTPException::Error] << "HttpForm unknown Requestype";
    throw _httpexception;
  }
  size_t fdatstpos=0;
  size_t keyendpos=0;
  for(size_t fdatpos=0; fdatpos<getlen(formdat)+1; fdatpos++){
    switch(formdat[fdatpos]){
        case '&': case '\0':{
          if(keyendpos >fdatstpos && keyendpos<fdatpos){
            char *key=new char[(keyendpos-fdatstpos)+1];
            size_t vlstpos=keyendpos+1;
            char *value=nullptr;
            char *urldecdValue=nullptr;
            char *urldecdKey=nullptr;
            scopy(formdat+fdatstpos,formdat+keyendpos,key);
            if(formdat+vlstpos){
                value=new char[(fdatpos-vlstpos)+1];
                scopy(formdat+vlstpos,formdat+fdatpos,value);
                key[(keyendpos-fdatstpos)]='\0';
                value[(fdatpos-vlstpos)]='\0';
                urlDecode(value,getlen(value),&urldecdValue);
            }            
            urlDecode(key,getlen(key),&urldecdKey);
            UrlcodedFormData *newenrty;
            newenrty=addUrlcodedFormData();
            newenrty->setKey(urldecdKey);
            newenrty->setValue(urldecdValue);
            delete[] key;
            delete[] urldecdKey;
            delete[] value;
            delete[] urldecdValue;
          }
          fdatstpos=fdatpos+1;
        };
        case '=':{
          keyendpos=fdatpos;  
        };
    }
  }
  delete[] formdat;
}

void libhttppp::HttpForm::UrlcodedFormData::setKey(const char* key){
  if(_Key)
    delete[] _Key;
  _Key=new char [getlen(key)+1];
  scopy(key,key+getlen(key),_Key);
  _Key[getlen(key)]='\0';  
}

void libhttppp::HttpForm::UrlcodedFormData::setValue(const char* value){
  if(_Value)
    delete[] _Value;
  if(!value){
      _Value=nullptr;
      return;
  }
  _Value=new char [getlen(value)+1];
  scopy(value,value+getlen(value),_Value);
  _Value[getlen(value)]='\0'; 
}

const char *libhttppp::HttpForm::UrlcodedFormData::getKey(){
  return _Key;
}

const char *libhttppp::HttpForm::UrlcodedFormData::getValue(){
  return _Value;
}

libhttppp::HttpForm::UrlcodedFormData  *libhttppp::HttpForm::UrlcodedFormData::nextUrlcodedFormData(){
  return _nextUrlcodedFormData;  
}

libhttppp::HttpForm::UrlcodedFormData::UrlcodedFormData(){
  _Key=NULL;
  _Value=NULL;
  _nextUrlcodedFormData=NULL;
}

libhttppp::HttpForm::UrlcodedFormData::~UrlcodedFormData(){
  delete[] _Key;
  delete[] _Value;
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
  _Key=NULL;
  _Value=NULL;
  _nextCookieData=NULL;
}

libhttppp::HttpCookie::CookieData::~CookieData(){
  delete[] _Key;
  delete[] _Value;
  delete   _nextCookieData;
}

libhttppp::HttpCookie::CookieData * libhttppp::HttpCookie::CookieData::nextCookieData(){
  return _nextCookieData;
}

const char * libhttppp::HttpCookie::CookieData::getKey(){
  return _Key;
}

const char * libhttppp::HttpCookie::CookieData::getValue(){
  return _Value;
}



libhttppp::HttpCookie::HttpCookie(){
  _firstCookieData=NULL;
  _lastCookieData=NULL;
}

libhttppp::HttpCookie::~HttpCookie(){
  delete _firstCookieData;
}


void libhttppp::HttpCookie::setcookie(HttpResponse *curresp,
                                      const char* key, const char* value,
                                      const char *comment,const char *domain,  
                                      int maxage, const char* path,
                                      bool secure,const char *version){
    if(!key || !value){
        _httpexception[HTTPException::Note] << "no key or value set in cookie!";
        return;
    }
    HttpHeader::HeaderData *dat=curresp->setData("Set-Cookie");
    *dat << key << "=" << value;
    if(comment){
        *dat << "; Comment="; 
        *dat << comment;
    }
    if(domain)
        *dat << "; Domain=" << domain;
    if(maxage>=0)
        *dat << "; Max-Age=" << maxage;
    if(path)
        *dat << "; Path=" << path;
    if(secure)
        *dat << "; Secure";
    if(version)
        *dat << "; Version=" << version;
    
}


void libhttppp::HttpCookie::parse(libhttppp::HttpRequest* curreq){
  const char *cdat=curreq->getData("Cookie");
  if(!cdat)
    return;
  
  size_t delimeter=0;
  size_t keyendpos=0;
  size_t startpos=0;
  
  for(size_t cpos=0; cpos < getlen(cdat)+1; cpos++){
    if(cdat[cpos]=='='){
      keyendpos=cpos;  
    }
    if(cdat[cpos]==';' || cdat[cpos]=='\0'){
      delimeter=cpos;  
    }
    if(keyendpos!=0 && delimeter!=0){
      CookieData *curcookie=addCookieData();
        
      curcookie->_Key=new char[(keyendpos-startpos)+1];
      scopy(cdat+startpos,cdat+keyendpos,curcookie->_Key);
      curcookie->_Key[(keyendpos-startpos)]='\0';
      
      size_t valuestartpos=keyendpos+1;
      curcookie->_Value=new char[(delimeter-valuestartpos)+1];
      scopy(cdat+valuestartpos,cdat+delimeter,curcookie->_Value);
      curcookie->_Value[(delimeter-valuestartpos)]='\0';
      
      startpos=delimeter+2;
      keyendpos=0;
      delimeter=0;
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
  _Username=NULL;
  _Password=NULL;
  _Realm=NULL;
}

libhttppp::HttpAuth::~HttpAuth(){
  delete[] _Username;
  delete[] _Password;
  delete[] _Realm;
}


void libhttppp::HttpAuth::parse(libhttppp::HttpRequest* curreq){
  const char *authstr=curreq->getData("Authorization");
  if(!authstr)
    return;
  if(libhttppp::ncompare(authstr,libhttppp::getlen(authstr),"Basic",5)==0)
    _Authtype=BASICAUTH;
  else if(libhttppp::ncompare(authstr,libhttppp::getlen(authstr),"Digest",6)==0)
    _Authtype=DIGESTAUTH;
  switch(_Authtype){
    case BASICAUTH:{
      size_t base64strsize=getlen(authstr+6);
      char *base64str=new char[base64strsize+1];
      scopy(authstr+6,authstr+(getlen(authstr)+1),base64str);
      Base64 base64;
      char *clearstr=new char[base64.Decodelen(base64str)];
      size_t cleargetlen=base64.Decode(clearstr,base64str);
      delete[] base64str;
      for(size_t clearpos=0; clearpos<cleargetlen; clearpos++){
         if(clearstr[clearpos]==':'){
            char *usernm=new char[clearpos+1];
            scopy(clearstr,clearstr+clearpos,usernm);
            usernm[clearpos]='\0';
            setUsername(usernm);
            setPassword(clearstr+(clearpos+1));
            delete[] usernm;
         }
      }
      delete[] clearstr;
    };
    case DIGESTAUTH:{
      for(size_t authstrpos=7; authstrpos<getlen(authstr)+1; authstrpos++){
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
  HttpHeader::HeaderData *dat=curresp->setData("WWW-Authenticate",nullptr);
  switch(_Authtype){
    case BASICAUTH:{
      if(_Realm){
        *dat << "Basic realm=\"" << _Realm << "\"";
      }else{
        *dat << "Basic";
      }
    };
    case DIGESTAUTH:{
      *dat << "Digest";
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
    size_t realmsize=getlen(realm);
    _Realm=new char [realmsize+1];
    scopy(realm,realm+realmsize,_Realm);
    _Realm[realmsize]='\0';
  }else{
    _Realm=NULL;  
  }
}

void libhttppp::HttpAuth::setUsername(const char* username){
  if(_Username)
    delete[] _Username;
  size_t usernamelen=getlen(username);
  _Username=new char[usernamelen+1];
  scopy(username,username+(usernamelen+1),_Username);
}

void libhttppp::HttpAuth::setPassword(const char* password){
  if(_Password)
    delete[] _Password;
  size_t passwordlen=getlen(password);
  _Password=new char[passwordlen+1];
  scopy(password,password+(passwordlen+1),_Password);
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
