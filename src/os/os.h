#ifndef OS_H
#define OS_H

#include <config.h>

#ifdef Windows
  #include "Windows/socket.h"
  #include "Windows/inttype.h"
  #include "Windows/mutex.h"
#else
  #include "Unix/socket.h"
  #include "Unix/inttype.h"
  #include "Unix/mutex.h"
#endif

#endif
