#os dependencies
install(FILES os/os.h 
        DESTINATION include/httppp/os
        COMPONENT Devel
)

#os dependencies
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  list(APPEND libhttpSrcs "os/Windows/socket.cpp")
  list(APPEND libhttpSrcs "os/Windows/lock.cpp")
  list(APPEND libhttpSrcs "os/Windows/thread.cpp")
  list(APPEND libhttpSrcs "os/Windows/sysinfo.cpp")
ELSE()
  list(APPEND libhttpSrcs "os/Unix/socket.cpp")
  list(APPEND libhttpSrcs "os/Unix/lock.cpp")
  list(APPEND libhttpSrcs "os/Unix/thread.cpp")
  list(APPEND libhttpSrcs "os/Unix/sysinfo.cpp")
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
   list(APPEND libhttpSrcs "os/Windows/iocp.cpp")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   list(APPEND libhttpSrcs "os/Unix/Linux/epoll.cpp")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "BSD" OR 
       ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   list(APPEND libhttpSrcs "os/Unix/BSD/kqueue.cpp")
ELSE()
   list(APPEND libhttpSrcs "os/Unix/select.cpp")
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
install(FILES os/Windows/inttype.h 
              os/Windows/socket.h
              os/Windows/lock.h
              os/Windows/thread.h
			  os/Windows/sysinfo.h
			  os/Windows/iocp.h
        DESTINATION include/httppp/os/Windows
        COMPONENT Devel
)
ELSE()
install(FILES os/Unix/inttype.h 
              os/Unix/socket.h
              os/Unix/lock.h
              os/Unix/thread.h
              os/Unix/sysinfo.h
	IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
			  os/Unix/Linux/epoll.h
	ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "BSD" OR 
       ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	          os/Unix/BSD/kqueue.h
	ELSE()
	          os/Unix/select.h
	ENDIF()
        DESTINATION include/httppp/os/Unix
        COMPONENT Devel
)
ENDIF()