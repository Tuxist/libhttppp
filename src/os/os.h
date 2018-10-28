#ifndef OS_H
#define OS_H

#include <config.h>

#ifdef Windows
  #include "Windows/socket.h"
  #include "Windows/inttype.h"
  #include "Windows/lock.h"
  #include "Windows/thread.h"
  #include "Windows/sysinfo.h"
#else
  #include "Unix/socket.h"
  #include "Unix/inttype.h"
  #include "Unix/lock.h"
  #include "Unix/thread.h"
  #include "Unix/sysinfo.h"
#endif

#endif
