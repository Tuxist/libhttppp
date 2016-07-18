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
#include <mutex>
#include <config.h>

#include "connections.h"

using namespace libhttppp;

const char* ConnectionData::getData(){
  return _Data;
}

size_t ConnectionData::getDataSize(){
  return _DataSize;
}

ConnectionData *ConnectionData::nextConnectionData(){
  return _nextConnectionData;
}

ConnectionData::ConnectionData(const char*data,size_t datasize){
  _nextConnectionData=NULL;
  std::copy(data,data+datasize,_Data);
  _DataSize=datasize;
}

ConnectionData::~ConnectionData(){
  if(_nextConnectionData)
    delete _nextConnectionData;
}

ClientSocket *Connection::getClientSocket(){
  return _ClientSocket;
}

Connection *Connection::nextConnection(){
  return _nextConnection; 
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
ConnectionData *Connection::addSendQueue(const char*data,size_t datasize){
  size_t written=0;
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
  _SendDataSize+=written;
  return _SendDataLast;
}

void Connection::cleanSendData(){
   if(_SendDataFirst)
     delete _SendDataFirst;
   _SendDataFirst=NULL;
   _SendDataLast=NULL;
   _SendDataSize=0;
}

ConnectionData *Connection::resizeSendQueue(size_t size){
  return _resizeQueue(&_SendDataFirst,&_SendDataLast,&_SendDataSize,size);
}

ConnectionData* Connection::getSendData(){
  return _SendDataFirst;
}

size_t Connection::getSendSize(){
  return _SendDataSize;
}


ConnectionData *Connection::addRecvQueue(const char data[BLOCKSIZE],size_t datasize){
  if(!_ReadDataFirst){
    _ReadDataFirst= new ConnectionData(data,datasize);
    _ReadDataLast=_ReadDataFirst;
  }else{
    _ReadDataLast->_nextConnectionData=new ConnectionData(data,datasize);
    _ReadDataLast=_ReadDataLast->_nextConnectionData;
  }
  std::copy(data,data+datasize,_ReadDataLast->_Data);
  _ReadDataLast->_DataSize=datasize;
  _ReadDataSize+=datasize;
  return _ReadDataLast;
}

void Connection::cleanRecvData(){
   delete _ReadDataFirst;
  _ReadDataFirst=NULL;
  _ReadDataLast=NULL;
  _ReadDataSize=0;
}


ConnectionData *Connection::resizeRecvQueue(size_t size){
  return _resizeQueue(&_ReadDataFirst,&_ReadDataLast,&_ReadDataSize,size);
}

ConnectionData *Connection::getRecvData(){
  return _ReadDataFirst;
}

size_t Connection::getRecvSize(){
  return _ReadDataSize;
}


ConnectionData *Connection::_resizeQueue(ConnectionData** firstdata, ConnectionData** lastdata,
					 size_t *qsize, size_t size){
  ConnectionData *firstdat=*firstdata;
  while(firstdat!=NULL && size!=0){
    size_t delsize=0;
    if(size>=firstdat->getDataSize()){
       delsize=firstdat->getDataSize();
       ConnectionData *deldat=firstdat;
       firstdat=firstdat->_nextConnectionData;
       if(deldat==*lastdata)
         *lastdata=firstdat;
       deldat->_nextConnectionData=NULL;
       delete deldat;
    }else{
       delsize=size;
       firstdat->_DataSize-=size;
    }
    size-=delsize;
    *qsize-=delsize;
  }
  *firstdata=firstdat;
  return firstdat;
}

int Connection::copyValue(ConnectionData* startblock, int startpos, 
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
      std::copy(curdat->_Data+startpos,curdat->_Data+(endpos-startpos),buf+copypos);
    }else if(curdat==startblock){
      std::copy(curdat->_Data+startpos,curdat->_Data+(curdat->getDataSize()-startpos),buf+copypos);
      copypos+=curdat->getDataSize()-startpos;
    }else if(curdat==endblock){
      std::copy(curdat->_Data,curdat->_Data+endpos,buf+copypos);
      copypos+=endpos;
    }else{
      std::copy(curdat->_Data,curdat->_Data+curdat->getDataSize(),buf+copypos);
      copypos+=curdat->getDataSize();
    }
    if(curdat==endblock)
      break;
  }
  buf[copysize]='\0';
  *buffer=buf;
  return copysize; //not include termination
}

int Connection::searchValue(ConnectionData* startblock, ConnectionData** findblock, 
			    const char* keyword){
  return searchValue(startblock, findblock, keyword,strlen(keyword));
}

int Connection::searchValue(ConnectionData* startblock, ConnectionData** findblock, 
			    const char* keyword,size_t keylen){
  size_t fpos=0,fcurpos=0;
  for(ConnectionData *curdat=startblock; curdat; curdat=curdat->nextConnectionData()){
    for(size_t pos=0; pos<curdat->getDataSize(); pos++){
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

bool Connection::tryLock(){
  try{
    _Locked->try_lock();
    return true;
  }catch(HTTPException &e){
    return false;
  }
}

bool Connection::tryUnlock(){
  try{
    _Locked->unlock();
    return true;
  }catch(HTTPException &e){
    return false;
  }
}

Connection::Connection(){
  _ClientSocket=new ClientSocket;
  _nextConnection=NULL;
  _ReadDataFirst=NULL;
  _ReadDataLast=NULL;
  _ReadDataSize=0;
  _SendDataFirst=NULL;
  _SendDataLast=NULL;
  _SendDataSize=0;
}

Connection::~Connection(){
  delete _ClientSocket;
  delete _ReadDataFirst;
  delete _SendDataFirst;
  delete _nextConnection;
}


ConnectionPool::ConnectionPool(ServerSocket *socket){
  _firstConnection=NULL;
  _lastConnection=NULL;
  _ServerSocket=socket;
  if(!_ServerSocket){
    _httpexception.Cirtical("ServerSocket not set!");
    throw _httpexception;
  }
}

ConnectionPool::~ConnectionPool(){
    delete _firstConnection;
}

Connection* ConnectionPool::addConnection(){
  if(!_firstConnection){
    _firstConnection=new Connection;
    _lastConnection=_firstConnection;
  }else{
    _lastConnection->_nextConnection=new Connection;
    _lastConnection=_lastConnection->_nextConnection;
  }
  return _lastConnection;
}

#ifndef Windows
Connection* ConnectionPool::delConnection(int socket){
  return delConnection(getConnection(socket));
}
#else
Connection* ConnectionPool::delConnection(SOCKET socket){
  return delConnection(getConnection(socket));
}
#endif


Connection* ConnectionPool::delConnection(ClientSocket *clientsocket){
  return delConnection(getConnection(clientsocket));
}

Connection* ConnectionPool::delConnection(Connection *delcon){
  Connection *prevcon=NULL;
  for(Connection *curcon=_firstConnection; curcon; curcon=curcon->nextConnection()){
    if(curcon==delcon){
      if(prevcon){
        prevcon->_nextConnection=curcon->_nextConnection;
        if(_lastConnection==delcon)
          _lastConnection=prevcon;
      }else{
        _firstConnection=curcon->_nextConnection;
        if(_lastConnection==delcon)
          _lastConnection=_firstConnection;
      }
      curcon->_nextConnection=NULL;
      delete curcon;
      break;
    }
    prevcon=curcon;
  }
  if(prevcon && prevcon->_nextConnection)
    return prevcon->_nextConnection;
  else
    return _firstConnection;
}

Connection* ConnectionPool::getConnection(ClientSocket *clientsocket){
  for(Connection *curcon=_firstConnection; curcon; curcon=curcon->nextConnection()){
    if(curcon->getClientSocket()==clientsocket)
      return curcon;
  }
  return NULL;
}

#ifndef Windows
Connection* ConnectionPool::getConnection(int socket){
#else
Connection* ConnectionPool::getConnection(SOCKET socket){
#endif
  for(Connection *curcon=_firstConnection; curcon; curcon=curcon->nextConnection()){
    if(curcon->getClientSocket()->getSocket()==socket)
      return curcon;
  }
  return NULL; 
}