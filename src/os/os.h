#ifndef OS_H
#define OS_H

#include <config.h>

#ifdef Windows
  #include "windows/socket.h"
  #include "windows/lock.h"
  #include "windows/thread.h"
  #include "windows/sysinfo.h"
  #include "windows/ctrlhandler.h"
#else
  #include "unix/ctrlhandler.h"
#endif

#endif
