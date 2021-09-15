# libhttppp

### Status Report:

## Finished:
- HttpForm
- HttpMultiform
- Httpcookie
- Httpbasicauth
- Https
- Multithreading
- epoll (linux support)


## On Work
- Move sys to seperate libsystempp
- Remove Glibc and std dependecy
- Httpclientsupport

## Todo:
- kqueue (Mac OS BSD support)
- select (other os)
- iocp (Windows support)
- Httpdigestauth
- Httpntlmauth
- Httpcompression (gzip)



## Download:
https://github.com/Tuxist/libhttppp


## Building Library
cd <libpath> <br/>
mkdir build <br/>
cd build <br/>
cmake ../ <br/>
make <br/>
make install <br/>
<br/>
Note to build examples you need libhtml: <br/>
https://github.com/Tuxist/libhtmlpp
