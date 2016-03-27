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
 
#ifndef HTTPDEFINITIONS_H
#define HTTPDEFINITIONS_H

//define REQUEST TYPE
#define GETREQUEST  1
#define POSTREQUEST 2

//define http version

#define HTTPVERSION(V) ("HTTP/"#V)

//#define HTTPCODE(C,N) ("HTTP/1.0 " #C " " #N "\r\n")
#define HTTPSTATECODE(C,N) (#C " " #N)

/*HTTP 1xx states*/
#define HTTP100 HTTPSTATECODE(100,Continue)
#define HTTP101 HTTPSTATECODE(101,Switching Protocols)
#define HTTP102 HTTPSTATECODE(102,Processing)
#define HTTP118 HTTPSTATECODE(118,Connection timed out)

/*HTTP 2xx states*/
#define HTTP200 HTTPSTATECODE(200,OK)
#define HTTP201 HTTPSTATECODE(201,Created)
#define HTTP202 HTTPSTATECODE(202,Accepted)
#define HTTP203 HTTPSTATECODE(203,Non-Authoritative Information)
#define HTTP204 HTTPSTATECODE(204,No Content)
#define HTTP205 HTTPSTATECODE(205,Reset Content)
#define HTTP206 HTTPSTATECODE(206,Partial Content)
#define HTTP207 HTTPSTATECODE(207,Multi-Status)

/*HTTP 3xx states*/
#define HTTP300 HTTPSTATECODE(300,Multiple Choices)
#define HTTP301 HTTPSTATECODE(301,Moved Permanently)
#define HTTP302 HTTPSTATECODE(302,Found)
#define HTTP303 HTTPSTATECODE(303,See Other)
#define HTTP304 HTTPSTATECODE(304,Not Modified)
#define HTTP305 HTTPSTATECODE(305,Use Proxy)
#define HTTP307 HTTPSTATECODE(307,Temporary Redirect)

/*HTTP 4xx states*/
#define HTTP400 HTTPSTATECODE(400,Bad Request)
#define HTTP401 HTTPSTATECODE(401,Unauthorized)
#define HTTP403 HTTPSTATECODE(403,Forbidden)
#define HTTP404 HTTPSTATECODE(404,Not Found)
#define HTTP405 HTTPSTATECODE(405,Method Not Allowed)
#define HTTP406 HTTPSTATECODE(406,Not Acceptable)
#define HTTP407 HTTPSTATECODE(407,Proxy Authentication Required)
#define HTTP408 HTTPSTATECODE(408,Request Time-out)
#define HTTP409 HTTPSTATECODE(409,Conflict)
#define HTTP410 HTTPSTATECODE(410,Gone)
#define HTTP411 HTTPSTATECODE(411,Length Required)
#define HTTP412 HTTPSTATECODE(412,Precondition Failed)
#define HTTP413 HTTPSTATECODE(413,Request Entity Too Large)
#define HTTP414 HTTPSTATECODE(414,Request-URI Too Long)
#define HTTP415 HTTPSTATECODE(415,Unsupported Media Type)
#define HTTP416 HTTPSTATECODE(416,Requested range not satisfiable)
#define HTTP417 HTTPSTATECODE(417,Expectation Failed)
#define HTTP418 HTTPSTATECODE(418,"I'm a teapot")
#define HTTP421 HTTPSTATECODE(421,There are too many connections from your internet address)
#define HTTP422 HTTPSTATECODE(422,Unprocessable Entity)
#define HTTP423 HTTPSTATECODE(423,Locked)
#define HTTP424 HTTPSTATECODE(424,Failed Dependency)
#define HTTP425 HTTPSTATECODE(425,Unordered Collection)
#define HTTP426 HTTPSTATECODE(426,Upgrade Required)

/*HTTP 5xx states*/
#define HTTP500 HTTPSTATECODE(500,Internal Server Error)
#define HTTP501 HTTPSTATECODE(501,Not Implemented)
#define HTTP502 HTTPSTATECODE(502,Bad Gateway)
#define HTTP503 HTTPSTATECODE(503,Service Unavailable)
#define HTTP504 HTTPSTATECODE(504,Gateway Time-out)
#define HTTP505 HTTPSTATECODE(505,HTTP Version not supported)
#define HTTP506 HTTPSTATECODE(506,Variant Also Negotiates)
#define HTTP507 HTTPSTATECODE(507,Insufficient Storage)
#define HTTP509 HTTPSTATECODE(509,Bandwidth Limit Exceeded)
#define HTTP510 HTTPSTATECODE(510,Not Extended)

#endif