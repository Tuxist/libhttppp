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
  #include "unix/socket.h"
  #include "unix/lock.h"
  #include "unix/thread.h"
  #include "unix/sysinfo.h"
  #include "unix/ctrlhandler.h"
  #include "unix/console.h"
#endif

#endif
