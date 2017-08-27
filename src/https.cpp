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

#include <openssl/x509.h>

#include "https.h"


libhttppp::HTTPS::HTTPS(){
  SSL_load_error_strings();	
  OpenSSL_add_ssl_algorithms();
}

libhttppp::HTTPS::~HTTPS(){
  EVP_cleanup();
}

void libhttppp::HTTPS::createContext(){
  const SSL_METHOD *method;
  method = SSLv23_server_method();
  _CTX = SSL_CTX_new(method);
  if (!_CTX) {
    perror("Unable to create SSL context");
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }
}

void libhttppp::HTTPS::setCert(const unsigned char* crt,size_t crtlen){
  _CERT = d2i_X509(NULL, &crt, crtlen);
  SSL_CTX_use_certificate(_CTX, _CERT);
}

void libhttppp::HTTPS::setKey(const unsigned char* key,size_t keylen){
  _PKey = d2i_RSAPrivateKey(NULL, &key, keylen);
  SSL_CTX_use_RSAPrivateKey(_CTX, _PKey);
}
