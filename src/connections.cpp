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

#include <assert.h>

#include <config.h>

#include "connections.h"
#include "utils.h"
#include "eventapi.h"

const char* libhttppp::ConnectionData::getData(){
  return _Data;
}

size_t libhttppp::ConnectionData::getDataSize(){
  return _DataSize;
}

libhttppp::ConnectionData *libhttppp::ConnectionData::nextConnectionData(){
  return _nextConnectionData;
}

libhttppp::ConnectionData::ConnectionData(const char*data,size_t datasize){
  _Data = new char[BLOCKSIZE];
  scopy(data,data+datasize,_Data);
  _DataSize=datasize;
  _nextConnectionData=nullptr;
}

libhttppp::ConnectionData::~ConnectionData() {
    delete[] _Data;
    delete _nextConnectionData;
}

libsystempp::ClientSocket *libhttppp::Connection::getClientSocket(){
  return _ClientSocket;
}

/** \brief a method to add Data to Sendqueue
  * \param data an const char* to add to sendqueue
  * \param datasize an size_t to set datasize
  * \return the last ConnectionData Block from Sendqueue
  * 
  * This method does unbelievably useful things.  
  * And returns exceptionally the new connection data block.
  * Use it everyday with good health.
  */

libhttppp::ConnectionData *libhttppp::Connection::addSendQueue(const char*data,size_t datasize){
    HTTPException excep;
    size_t written=0;
    if(datasize==0)
        return nullptr;
    for(size_t cursize=datasize; cursize>0; cursize=datasize-written){
        if(cursize>BLOCKSIZE){
            cursize=BLOCKSIZE;
        }
        if(!_SendDataFirst){
            _SendDataFirst= new ConnectionData(data+written,cursize);
            _SendDataLast=_SendDataFirst;
        }else{
            _SendDataLast->_nextConnectionData=new ConnectionData(data+written,cursize);
            _SendDataLast=_SendDataLast->_nextConnectionData;
        }
        written+=cursize;
    }
#ifdef DEBUG
    Console con;
    con << "Written:" << written << " Datasize: " << datasize <<con.endl;
#endif
    if(datasize!=written)
        throw excep[HTTPException::Critical] << "something goes wrong in addsendque !";
    _SendDataSize+=datasize;
    _EventApi->sendReady(this,true);
    return _SendDataLast;
}

void libhttppp::Connection::cleanSendData(){
   delete _SendDataFirst;
   _SendDataFirst=nullptr;
   _SendDataLast=nullptr;
   _SendDataSize=0;
}

libhttppp::ConnectionData *libhttppp::Connection::resizeSendQueue(size_t size){
    try{
        return _resizeQueue(&_SendDataFirst,&_SendDataLast,&_SendDataSize,size);
    }catch(HTTPException &e){
        throw e; 
    }
}

libhttppp::ConnectionData* libhttppp::Connection::getSendData(){
  return _SendDataFirst;
}

size_t libhttppp::Connection::getSendSize(){
  return _SendDataSize;
}

libhttppp::ConnectionData *libhttppp::Connection::addRecvQueue(const char data[BLOCKSIZE],size_t datasize){
    if(datasize<=0){
        HTTPException httpexception;
        httpexception[HTTPException::Error] << "addRecvQueue wrong datasize";
        throw httpexception;
    }
    if(!_ReadDataFirst){
        _ReadDataFirst= new ConnectionData(data,datasize);
        _ReadDataLast=_ReadDataFirst;
    }else{
        _ReadDataLast->_nextConnectionData=new ConnectionData(data,datasize);
        _ReadDataLast=_ReadDataLast->_nextConnectionData;
    }
    _ReadDataSize+=datasize;
    return _ReadDataLast;
}

void libhttppp::Connection::cleanRecvData(){
   delete _ReadDataFirst;
  _ReadDataFirst=nullptr;
  _ReadDataLast=nullptr;
  _ReadDataSize=0;
}


libhttppp::ConnectionData *libhttppp::Connection::resizeRecvQueue(size_t size){
    try{
        return _resizeQueue(&_ReadDataFirst,&_ReadDataLast,&_ReadDataSize,size);
    }catch(HTTPException &e){
        throw e; 
    }
}

libhttppp::ConnectionData *libhttppp::Connection::getRecvData(){
  return _ReadDataFirst;
}

size_t libhttppp::Connection::getRecvSize(){
  return _ReadDataSize;
}

//#define DEBUG
libhttppp::ConnectionData *libhttppp::Connection::_resizeQueue(ConnectionData** firstdata, ConnectionData** lastdata,
                                                               size_t *qsize, size_t size){
    HTTPException httpexception;
    if(!*firstdata){
        throw httpexception[HTTPException::Error] << "_resizeQueue wrong datasize or ConnectionData";
    }
    #ifdef DEBUG
    size_t delsize=0,presize=*qsize;
    #endif
    (*qsize)-=size;
    while(size>0 && size>=(*firstdata)->getDataSize()){
        #ifdef DEBUG
        delsize+=(*firstdata)->getDataSize();;
        #endif
        size-=(*firstdata)->getDataSize();
        ConnectionData *newdat=(*firstdata)->_nextConnectionData;
        (*firstdata)->_nextConnectionData=NULL;
        if(*firstdata==*lastdata)
            (*lastdata)=nullptr; 
        delete (*firstdata);
        *firstdata=newdat;
    }
    
    if(size>0){
        #ifdef DEBUG
        delsize+=size;
        Console con;
        #endif
        for(size_t i=0; i<((*firstdata)->getDataSize()-size); ++i){
            (*firstdata)->_Data[i]=(*firstdata)->_Data[size+i];
        }
        (*firstdata)->_DataSize-=size;
        *firstdata=(*firstdata);
    }
    #ifdef DEBUG
    Console con;
    con  << " delsize: "              << delsize
    << " Calculated Blocksize: " << (presize-delsize) << con.endl;
    if((presize-delsize)!=*qsize)
        throw httpexception[HTTPException::Critical] << "_resizeQueue: Calculated wrong size";
    #endif
    return *firstdata;
                                                               }
                                                               
int libhttppp::Connection::copyValue(ConnectionData* startblock, int startpos, 
                          ConnectionData* endblock, int endpos, char** buffer){
  size_t copysize=0,copypos=0;
  for(ConnectionData *curdat=startblock; curdat; curdat=curdat->nextConnectionData()){
    if(curdat==endblock){
      copysize+=endpos;
      break;
    }
    copysize+=curdat->getDataSize();
  }
  copysize-=startpos;
  char *buf;
  buf = new char[(copysize+1)]; //one more for termination
  for(ConnectionData *curdat=startblock; curdat; curdat=curdat->nextConnectionData()){
    if(curdat==startblock && curdat==endblock){
      scopy(curdat->_Data+startpos,curdat->_Data+(endpos-startpos),buf+copypos);
    }else if(curdat==startblock){
      scopy(curdat->_Data+startpos,curdat->_Data+(curdat->getDataSize()-startpos),buf+copypos);
      copypos+=curdat->getDataSize()-startpos;
    }else if(curdat==endblock){
      scopy(curdat->_Data,curdat->_Data+endpos,buf+copypos);
      copypos+=endpos;
    }else{
      scopy(curdat->_Data,curdat->_Data+curdat->getDataSize(),buf+copypos);
      copypos+=curdat->getDataSize();
    }
    if(curdat==endblock)
      break;
  }
  buf[copysize]='\0';
  *buffer=buf;
  return copysize; //not include termination
}

int libhttppp::Connection::searchValue(ConnectionData* startblock, ConnectionData** findblock, 
                                       const char* keyword){
    return searchValue(startblock, findblock, keyword,getlen(keyword));
}
                                       
int libhttppp::Connection::searchValue(ConnectionData* startblock, ConnectionData** findblock, 
                                       const char* keyword,size_t keylen){
    size_t fpos=0,fcurpos=0;
    for(ConnectionData *curdat=startblock; curdat; curdat=curdat->nextConnectionData()){
        for(size_t pos=0; pos<curdat->getDataSize(); ++pos){
            if(keyword[fcurpos]==curdat->_Data[pos]){
                if(fcurpos==0){
                    fpos=pos;
                    *findblock=curdat;
                }
                fcurpos++;
            }else{
                fcurpos=0;
                fpos=0;
                *findblock=NULL;
            }
            if(fcurpos==keylen)
                return fpos;
        }
    }
    return -1;
}

libhttppp::Connection::Connection(libsystempp::ServerSocket *servsock,EventApi *event){
  _ClientSocket=new libsystempp::ClientSocket();
  _ServerSocket = servsock;
  _ReadDataFirst=nullptr;
  _ReadDataLast=nullptr;
  _ReadDataSize=0;
  _SendDataFirst=nullptr;
  _SendDataLast=nullptr;
  _SendDataSize=0;
  _EventApi=event;
  ConnectionPtr=nullptr;
}

libhttppp::Connection::~Connection(){
    delete _ClientSocket;
    delete _ReadDataFirst;
    delete _SendDataFirst;
}
