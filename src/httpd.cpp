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
#include <cstring>
#include <cstdlib>

#include "os/os.h"
#include "utils.h"
#include "httpd.h"

#define KTKEY 0
#define KTSKEY 1

libhttppp::HTTPDCmd::HTTPDCmd() {
	_Key = NULL;
	_SKey = '\0';
	_Value = NULL;
	_Help = NULL;
	_Found = false;
	_Required = false;
	_nextHTTPDCmd = NULL;
}

const char *libhttppp::HTTPDCmd::getKey() {
	return _Key;
}

const char libhttppp::HTTPDCmd::getShortkey() {
	return _SKey;
}

const char *libhttppp::HTTPDCmd::getValue() {
	return _Value;
}

size_t libhttppp::HTTPDCmd::getValueSize_t() {
	return atoi(_Value);
}

int libhttppp::HTTPDCmd::getValueInt() {
	return atoi(_Value);
}

const char *libhttppp::HTTPDCmd::getHelp() {
	return _Help;
}

bool libhttppp::HTTPDCmd::getFound() {
	return _Found;
}

bool libhttppp::HTTPDCmd::getRequired() {
	return _Required;
}

libhttppp::HTTPDCmd *libhttppp::HTTPDCmd::nextHTTPDCmd() {
	return _nextHTTPDCmd;
}

libhttppp::HTTPDCmd::~HTTPDCmd() {
	delete[] _Key;
	delete[] _Value;
	delete[] _Help;
	delete _nextHTTPDCmd;
}

libhttppp::HTTPDCmdController::HTTPDCmdController() {
	_firstHTTPDCmd = NULL;
	_lastHTTPDCmd = NULL;
}

void libhttppp::HTTPDCmdController::registerCmd(const char *key, const char skey,bool required, const char *defaultvalue, const char *help) {
	if (!key || !skey || !help) {
		_httpexception.Critical("cmd parser key,skey or help not set!");
		throw _httpexception;
	}
	/*if key exist overwriting options*/
	for (HTTPDCmd *curhttpdcmd = _firstHTTPDCmd; curhttpdcmd; curhttpdcmd=curhttpdcmd->nextHTTPDCmd()) {
		if (strncmp(curhttpdcmd->getKey(), key, strlen(curhttpdcmd->getKey())) == 0) {
			/*set new shortkey*/
			curhttpdcmd->_SKey = skey;
			/*set reqirement flag*/
			curhttpdcmd->_Required = required;
			/*set new value*/
			delete[] curhttpdcmd->_Value;
			curhttpdcmd->_Value = new char[strlen(defaultvalue)+1];
			scopy(defaultvalue, defaultvalue+strlen(defaultvalue),curhttpdcmd->_Value);
			curhttpdcmd->_Value[strlen(defaultvalue)] = '\0';
			/*set new help*/
			delete[] curhttpdcmd->_Help;
			curhttpdcmd->_Help = new char[strlen(help) + 1];
			scopy(help, help + strlen(help), curhttpdcmd->_Help);
			curhttpdcmd->_Help[strlen(help)] = '\0';
			return;
		}
	}
	/*create new key value store*/
	if (!_firstHTTPDCmd) {
		_firstHTTPDCmd = new HTTPDCmd;
		_lastHTTPDCmd = _firstHTTPDCmd;
	}
	else {
		_lastHTTPDCmd->_nextHTTPDCmd = new HTTPDCmd;
		_lastHTTPDCmd = _lastHTTPDCmd->_nextHTTPDCmd;
	}
	/*set new key*/
	_lastHTTPDCmd->_Key = new char[strlen(key) + 1];
	scopy(key,key+strlen(key),_lastHTTPDCmd->_Key);
	_lastHTTPDCmd->_Key[strlen(key)] = '\0';
	/*set new shortkey*/
	_lastHTTPDCmd->_SKey = skey;
	/*set reqirement flag*/
	_lastHTTPDCmd->_Required = required;
	/*set new value*/
	if (defaultvalue) {
		_lastHTTPDCmd->_Value = new char[strlen(defaultvalue) + 1];
		scopy(defaultvalue, defaultvalue + strlen(defaultvalue), _lastHTTPDCmd->_Value);
		_lastHTTPDCmd->_Value[strlen(defaultvalue)] = '\0';
	}
	/*set new help*/
	_lastHTTPDCmd->_Help = new char[strlen(help) + 1];
	scopy(help, help + strlen(help), _lastHTTPDCmd->_Help);
	_lastHTTPDCmd->_Help[strlen(help)] = '\0';

}

void libhttppp::HTTPDCmdController::registerCmd(const char *key, const char skey, bool required, size_t defaultvalue, const char *help) {
	char buf[255];
	snprintf(buf, sizeof(buf), "%zu", defaultvalue);
	registerCmd(key,skey,required,buf,help);
}

void libhttppp::HTTPDCmdController::registerCmd(const char *key, const char skey, bool required, int defaultvalue, const char *help) {
	char buf[255];
	snprintf(buf, sizeof(buf), "%d", defaultvalue);
	registerCmd(key, skey, required, buf, help);
}

void libhttppp::HTTPDCmdController::parseCmd(int argc, char** argv){
	for (int args = 1; args < argc; args++) {
		int keytype = -1;
		if (argv[args][0]=='-' && argv[args][1] == '-') {
			keytype = KTKEY;
		}else if (argv[args][0] == '-'){
			keytype = KTSKEY;
		}else {
			break;
		}

		size_t kendpos = strlen(argv[args]);
		for (size_t cmdpos = 0; cmdpos < strlen(argv[args])+1; cmdpos++) {	
			switch (argv[args][cmdpos]) {
			  case '=': {
				  kendpos = cmdpos;
			  };
			}
		}

		char *key = NULL;
		char skey = '0';
		if (keytype == KTKEY) {
			key = new char[kendpos-1];
			scopy(argv[args] +2, argv[args] +kendpos, key);
			key[kendpos - 2] = '\0';
		} else if (keytype == KTSKEY){
			skey = argv[args][1];
		}

		for (HTTPDCmd *curhttpdcmd = _firstHTTPDCmd; curhttpdcmd; curhttpdcmd = curhttpdcmd->nextHTTPDCmd()) {
			if (keytype == KTKEY) {
				if (strncmp(curhttpdcmd->getKey(), key, strlen(curhttpdcmd->getKey())) == 0) {
					curhttpdcmd->_Found = true;
					int valuesize = (strlen(argv[args]) - (kendpos+1));
					if (valuesize > 0) {
						delete[] curhttpdcmd->_Value;
						curhttpdcmd->_Value = new char[valuesize+1];
						scopy(argv[args]+(kendpos+1), argv[args] + strlen(argv[args]),curhttpdcmd->_Value);
						curhttpdcmd->_Value[valuesize] = '\0';
					}
				}
			} else if (keytype == KTSKEY) {
				if (curhttpdcmd->getShortkey()== skey) {
					curhttpdcmd->_Found = true;
					int valuesize = (strlen(argv[args]) - (kendpos + 1));
					if (valuesize > 0) {
						delete[] curhttpdcmd->_Value;
						curhttpdcmd->_Value = new char[valuesize + 1];
						scopy(argv[args] + (kendpos + 1), argv[args] + strlen(argv[args]), curhttpdcmd->_Value);
						curhttpdcmd->_Value[valuesize] = '\0';
					}
				}
			}
		}

		delete[] key;
	}
}

bool libhttppp::HTTPDCmdController::checkRequired() {
	for (HTTPDCmd *curhttpdcmd = _firstHTTPDCmd; curhttpdcmd; curhttpdcmd = curhttpdcmd->nextHTTPDCmd()) {
		if (curhttpdcmd->getRequired() && !curhttpdcmd->_Found) {
			return false;
		}
	}
	return true;
}

void libhttppp::HTTPDCmdController::printHelp() {
	for (HTTPDCmd *curhttpdcmd = _firstHTTPDCmd; curhttpdcmd; curhttpdcmd = curhttpdcmd->nextHTTPDCmd()) {
		printf("--%s -%c %s\n",curhttpdcmd->getKey(),curhttpdcmd->getShortkey(),curhttpdcmd->getHelp());
	}
}

libhttppp::HTTPDCmd *libhttppp::HTTPDCmdController::getHTTPDCmdbyKey(const char *key) {
	for (HTTPDCmd *curhttpdcmd = _firstHTTPDCmd; curhttpdcmd; curhttpdcmd = curhttpdcmd->nextHTTPDCmd()) {
		if (strncmp(curhttpdcmd->getKey(), key, strlen(curhttpdcmd->getKey())) == 0) {
			return curhttpdcmd;
		}
	}
	return NULL;
}

libhttppp::HTTPDCmdController::~HTTPDCmdController() {
	delete _firstHTTPDCmd;
	_lastHTTPDCmd = NULL;
}


libhttppp::HttpD::HttpD(int argc, char** argv){
    CmdController= new HTTPDCmdController;
	/*Register Parameters*/
	CmdController->registerCmd("help", 'h', false, (const char*) NULL, "Helpmenu");
    CmdController->registerCmd("httpaddr",'a', true,(const char*) NULL,"Address to listen");
    CmdController->registerCmd("httpport", 'p', false, 8080, "Port to listen");
    CmdController->registerCmd("maxconnections", 'm',false, MAXDEFAULTCONN, "Max connections that can connect");
    CmdController->registerCmd("httpscert", 'c',false,(const char*) NULL, "HTTPS Certfile");
    CmdController->registerCmd("httpskey", 'k',false, (const char*) NULL, "HTTPS Keyfile");
  /*Parse Parameters*/
    CmdController->parseCmd(argc,argv);
	if (!CmdController->checkRequired()) {
		CmdController->printHelp();
		_httpexception.Critical("cmd parser not enough arguments given");
		throw _httpexception;
	}

	if (CmdController->getHTTPDCmdbyKey("help") && CmdController->getHTTPDCmdbyKey("help")->getFound()) {
		CmdController->printHelp();
		return;
	}

	/*get port from console paramter*/
	int port = 0;
    bool portset=false;
	if(CmdController->getHTTPDCmdbyKey("httpport")){
	    port = CmdController->getHTTPDCmdbyKey("httpport")->getValueInt();
        portset = CmdController->getHTTPDCmdbyKey("httpport")->getFound();
    }

	/*get httpaddress from console paramter*/
	const char *httpaddr = NULL;
	if (CmdController->getHTTPDCmdbyKey("httpaddr"))
		httpaddr = CmdController->getHTTPDCmdbyKey("httpaddr")->getValue();

	/*get max connections from console paramter*/
	int maxconnections = 0;
	if (CmdController->getHTTPDCmdbyKey("maxconnections"))
		maxconnections = CmdController->getHTTPDCmdbyKey("maxconnections")->getValueInt();

	/*get httpaddress from console paramter*/
	const char *sslcertpath = NULL;
	if (CmdController->getHTTPDCmdbyKey("httpscert"))
		sslcertpath = CmdController->getHTTPDCmdbyKey("httpscert")->getValue();

	/*get httpaddress from console paramter*/
	const char *sslkeypath = NULL;
	if (CmdController->getHTTPDCmdbyKey("httpskey"))
		sslkeypath = CmdController->getHTTPDCmdbyKey("httpskey")->getValue();

  try {
#ifndef Windows
	  if (portset == true)
		  _ServerSocket = new ServerSocket(httpaddr, port, maxconnections);
	  else
		  _ServerSocket = new ServerSocket(httpaddr, maxconnections);
#else
      _ServerSocket = new ServerSocket(httpaddr, port, maxconnections);
#endif

      if (!_ServerSocket) {
		  throw _httpexception;
	  }
	  
	  if (sslcertpath && sslkeypath) {
		  printf("%s : %s", sslcertpath, sslkeypath);
		  _ServerSocket->createContext();
		  _ServerSocket->loadCertfile(sslcertpath);
		  _ServerSocket->loadKeyfile(sslkeypath);
	  }
  }catch (HTTPException &e) {

  }
}

libhttppp::ServerSocket *libhttppp::HttpD::getServerSocket(){
  return _ServerSocket;
}

libhttppp::HttpD::~HttpD(){
  delete _ServerSocket;
  delete CmdController;
}
