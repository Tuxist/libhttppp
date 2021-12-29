# libhttppp

### Status Report:

## Finished:
- HttpForm
- HttpMultiform
- Httpcookie
- Httpbasicauth
- Multithreading
- epoll (linux support)
- Move sys to seperate libsystempp
- Remove Glibc and std dependecy

## On Work
- Httpclientsupport
- Https seperate own lib

## Todo:
- create new class for ssl support
- Httpdigestauth
- Httpntlmauth
- Httpcompression (gzip)

## Download:
https://github.com/Tuxist/libhttppp

## Requirements
- libsystempp
- kernel 4.19 (lowest that i have testing)
- gcc/clang
- cmake

# Optional
- libhtmlpp (for examples)
- doxygen (for docu not much at the moment)

## Building Library
cd <libpath> <br/>
mkdir build <br/>
cd build <br/>
cmake ../ <br/>
make <br/>
make install <br/>
<br/>
Note to build examples you need libhtmlpp: <br/>
https://github.com/Tuxist/libhtmlpp
