#ifndef OS_H
#define OS_H

#include "config.h"

#ifdef Windows
  #include "windows/socket.h"
  #include "windows/inttype.h"
  #include "windows/lock.h"
  #include "windows/thread.h"
  #include "windows/sysinfo.h"
#else
  #include "unix/socket.h"
  #include "unix/inttype.h"
  #include "unix/lock.h"
  #include "unix/thread.h"
  #include "unix/sysinfo.h"
#endif

#endif
