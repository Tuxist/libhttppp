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

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <errno.h>
#include <err.h>

#include "../event.h"

libhttppp::Queue::Queue(ServerSocket *socket) : ConnectionPool(socket) {
  struct kevent change;
  struct kevent event;
  pid_t pid;
  int kq, nev,i,fd;

  struct kevent evSet;
  struct kevent *evList= new struct kevent[socket->getMaxconnections()];
  if ((kq = kqueue()) == -1){
    _httpexception.Cirtical("Can't create Kqueue!!!");
    throw _httpexception;
  }
  EV_SET(&change, 1, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, 2000, 0);
  for(;;){
    nev = kevent(kq, NULL, 0, evList, socket->getMaxconnections(), NULL);
    if (nev < 1){
      _httpexception.Cirtical("Kqueue crashed");
      throw _httpexception;
    }
    for (i=0; i<nev; i++) {
      if (evList[i].flags & EV_EOF) {
        CloseConnection:
        try{
          EV_SET(&evSet, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
          fd = evList[i].ident;
          if (kevent(kq, &evSet, 1, NULL, 0, NULL) == -1){
            _httpexception.Cirtical("Can't delete Client Connection that shoulnd't happend");
            throw _httpexception;
          }
          delConnection(fd);
          continue;
         }catch(HTTPException &e){
           throw e;
         }
         continue;
         ;
      }else if (evList[i].ident == socket->getSocket()) {
          try{
            _httpexception.Note("Acccept Connection!");
            ClientSocket *clientsocket = new ClientSocket;
            fd =_ServerSocket->acceptEvent(clientsocket);
            Connection *curcon=addConnection(clientsocket);
            EV_SET(&evSet, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
            if(kevent(kq, &evSet, 1, NULL, 0, NULL)==-1)
             delConnection(curcon);
          }catch(HTTPException &e){
            continue;
          }
        }else if(evList[i].flags & EVFILT_WRITE) {
          try{
            Connection *curcon=getConnection(fd);
            if(curcon->getSendData()){
              _httpexception.Note("Sending");
              ssize_t sended=_ServerSocket->sendData(curcon->getClientSocket(),(void*)curcon->getSendData()->getData(),curcon->getSendData()->getDataSize());
              if(sended==-1){
                if (errno == EAGAIN || errno==EWOULDBLOCK)
                  continue;
                else
                  goto CloseConnection;
              }
              curcon->resizeSendQueue(sended);
            }else{
              EV_SET(&evSet, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
            }
           }catch(HTTPException &e){
             goto CloseConnection;
           }
        }else if(evList[i].flags & EVFILT_READ) {
          try{
            Connection *curcon=getConnection(fd);
            _httpexception.Note("Reciving");
            char buf[BLOCKSIZE];
            int rcvsize=_ServerSocket->recvData(curcon->getClientSocket(),buf,BLOCKSIZE);
            if(rcvsize==-1){
               if (errno == EAGAIN || errno == EWOULDBLOCK)
                 continue;
               else
                 goto CloseConnection;
            }
            curcon->addRecvQueue(buf,rcvsize);
            RequestEvent(curcon);
            if(curcon->getSendData()){
              EV_SET(&evSet, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
            }
          }catch(HTTPException &e){
            if(e.isError())
             goto CloseConnection;
            else if(e.isCritical())
              std::terminate();
          }
        }
     }
  }
  delete evList;
}

libhttppp::Queue::~Queue(){

}
