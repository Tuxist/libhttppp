#ifndef OS_H
#define OS_H

#include <config.h>

#ifdef Windows
  #include "Windows/socket.h"
  #include "Windows/inttype.h"
  #include "Windows/lock.h"
  #include "Windows/thread.h"
  #include "Windows/sysinfo.h"
  #include "Windows/iocp.h"
#else
  #include "Unix/socket.h"
  #include "Unix/inttype.h"
  #include "Unix/lock.h"
  #include "Unix/thread.h"
  #include "Unix/sysinfo.h"
#ifdef Linux
  #include "Unix/linux/epoll.h"
#elif BSD
  #include "Unix/bsd/kqueue.h"
#elif Darwin
  #include "Unix/bsd/kqueue.h"
#endif

#endif

#endif
