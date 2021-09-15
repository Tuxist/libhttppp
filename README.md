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
cd <libpath>
mkdir build
cd build
cmake ../
make
make install

Note to build examples you need libhtml:
https://github.com/Tuxist/libhtmlpp
