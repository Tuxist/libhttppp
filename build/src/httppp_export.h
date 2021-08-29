
#ifndef HTTPPP_EXPORT_H
#define HTTPPP_EXPORT_H

#ifdef HTTPPP_STATIC_DEFINE
#  define HTTPPP_EXPORT
#  define HTTPPP_NO_EXPORT
#else
#  ifndef HTTPPP_EXPORT
#    ifdef httppp_EXPORTS
        /* We are building this library */
#      define HTTPPP_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define HTTPPP_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef HTTPPP_NO_EXPORT
#    define HTTPPP_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef HTTPPP_DEPRECATED
#  define HTTPPP_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef HTTPPP_DEPRECATED_EXPORT
#  define HTTPPP_DEPRECATED_EXPORT HTTPPP_EXPORT HTTPPP_DEPRECATED
#endif

#ifndef HTTPPP_DEPRECATED_NO_EXPORT
#  define HTTPPP_DEPRECATED_NO_EXPORT HTTPPP_NO_EXPORT HTTPPP_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef HTTPPP_NO_DEPRECATED
#    define HTTPPP_NO_DEPRECATED
#  endif
#endif

#endif /* HTTPPP_EXPORT_H */
