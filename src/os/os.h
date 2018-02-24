#ifndef OS_H
#define OS_H

#ifdef Windows
  #include "Windows/socket.h"
#else
  #include "Unix/socket.h"
#endif

#endif
