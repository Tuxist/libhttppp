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

#include <fcntl.h>
#include <sys/epoll.h>
#include <cstdlib>
#include <config.h>
#include <errno.h>

#define READEVENT 0
#define SENDEVENT 1

#include "../queue.h"

using namespace libhttppp;

Queue::Queue(ServerSocket *serversocket) : ConnectionPool(serversocket) {
  struct epoll_event *events;
  struct epoll_event  event = {0};
  events = new epoll_event[(serversocket->getMaxconnections()*sizeof(struct epoll_event))];
  for(int i=0; i<serversocket->getMaxconnections(); i++)
    events[i].data.fd = -1;
  int epollfd = epoll_create(serversocket->getMaxconnections());
  
  if (epollfd == -1){
    _httpexception.Cirtical("can't create epoll");
    throw _httpexception;
  }
  
  event.events = EPOLLIN | EPOLLET |EPOLLPRI;
  event.data.fd = serversocket->getSocket();
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, serversocket->getSocket(), &event) < 0){
    _httpexception.Cirtical("can't create epoll");
    throw _httpexception;
  }
  
  for(;;){
    int n = epoll_wait(epollfd, events, serversocket->getMaxconnections(), EPOLLWAIT);
    for(int i=0; i<n; i++) {
      Connection *curcon=NULL;
      if(events[i].data.fd == serversocket->getSocket()) {
        try{
          /*will create warning debug mode that normally because the check already connection
           * with this socket if getconnection throw they will be create a new one
           */
	  curcon=addConnection();
	  ClientSocket *clientsocket=curcon->getClientSocket();
          event.data.fd =_ServerSocket->acceptEvent(clientsocket);
	  clientsocket->setnonblocking();
          event.events = EPOLLIN |EPOLLRDHUP;
          
          if(epoll_ctl(epollfd, EPOLL_CTL_ADD, event.data.fd, &event)==-1 && errno==EEXIST)
            epoll_ctl(epollfd, EPOLL_CTL_MOD, events[i].data.fd, &event);
        }catch(HTTPException &e){
          if(e.isCritical())
            throw e;
        }
        continue;
      }else{
	curcon=getConnection(events[i].data.fd);
      }
      
      if(events[i].events & EPOLLRDHUP || events[i].events & EPOLLERR) {
          CloseConnection:
            try{
              delConnection(curcon);
              epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, &event);
	      _httpexception.Note("Connection shutdown!");
	      continue;
            }catch(HTTPException &e){
              _httpexception.Note("Can't do Connection shutdown!");
            }
          ;
      }
      
    
      if(events[i].events & EPOLLIN){
          try{
	    for(;;){
              char buf[BLOCKSIZE];
	      int rcvsize=0;
	      try{
                rcvsize=_ServerSocket->recvData(curcon->getClientSocket(),buf,BLOCKSIZE);
	      }catch(HTTPException &e){
		  break;
	      }
	      if(rcvsize==0)
		break;
              curcon->addRecvQueue(buf,rcvsize);   
	    }
            RequestEvent(curcon);
            if(curcon->getSendData()){
              event.events = EPOLLOUT | EPOLLET;
              epoll_ctl(epollfd, EPOLL_CTL_MOD, events[i].data.fd, &event);
	      _httpexception.Note("switch to send mode!");
            }
          }catch(HTTPException &e){
            if(e.isCritical()){
              throw e;
            }
            if(e.isError())
             goto CloseConnection;
          }
      }
      
      if(events[i].events & EPOLLOUT) {
        try{
          if(curcon && curcon->getSendData()){
	    ssize_t sended=0;
	    while(curcon->getSendData()){
              sended=_ServerSocket->sendData(curcon->getClientSocket(),
                                                    (void*)curcon->getSendData()->getData(),
                                                     curcon->getSendData()->getDataSize(),MSG_NOSIGNAL);
	      
	    
              if(sended==-1){
                _httpexception.Note("Sending Failed");
                if (errno == EAGAIN){
                  continue;
                }else{
		  curcon->cleanSendData();
                  goto CloseConnection;
	        }
              }else{
                curcon->resizeSendQueue(sended); 
              }
            }
          }else{
              event.events = EPOLLIN | EPOLLET;
              epoll_ctl(epollfd, EPOLL_CTL_MOD, events[i].data.fd, &event);
	      _httpexception.Note("switch to recv mode!");
          }
        }catch(HTTPException &e){
           goto CloseConnection;
        }
      }
    }
  }
  delete events;
    
}

Queue::~Queue(){

}