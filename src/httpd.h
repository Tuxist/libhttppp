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

#include "event.h"
#include "exception.h"

#ifndef HTTPD_H
#define HTTPD_H

namespace libhttppp {
  class ServerSocket;
  class HTTPDCmd {
  public:
	  const char *getKey();
	  const char  getShortkey();
	  const char *getValue();
	  size_t      getValueSize_t();
	  int         getValueInt();
	  const char *getHelp();
	  bool        getFound();
	  bool        getRequired();
	  HTTPDCmd   *nextHTTPDCmd();
  private:
	  HTTPDCmd();
	  ~HTTPDCmd();
	  char         *_Key;
	  char          _SKey;
	  char         *_Value;
	  char         *_Help;
	  bool          _Found;
	  bool          _Required;
	  HTTPDCmd     *_nextHTTPDCmd;
	  friend class HTTPDCmdController;
  };

  class HTTPDCmdController {
  public:
	  HTTPDCmdController();
	  ~HTTPDCmdController();
	  void registerCmd(const char *key,char skey,bool required,const char *defaultvalue,const char *help);
	  void registerCmd(const char *key,char skey,bool required,size_t defaultvalue, const char *help);
	  void registerCmd(const char *key,char skey,bool required,int defaultvalue, const char *help);
	  void printHelp();
	  void parseCmd(int argc, char** argv);
	  bool checkRequired();
	  HTTPDCmd        *getHTTPDCmdbyKey(const char *key);
  private:
	  HTTPDCmd        *_firstHTTPDCmd;
	  HTTPDCmd        *_lastHTTPDCmd;
	  HTTPException   _httpexception;
  };

  class HttpD {
  public:
    HttpD(int argc, char** argv);
    ~HttpD();
    ServerSocket       *getServerSocket();
  protected:
    HTTPDCmdController *CmdController;
  private:
    ServerSocket   *_ServerSocket;
    HTTPException   _httpexception;
  };
};

#endif
