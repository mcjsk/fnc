#if !defined(_NET_FOSSIL_SCM_FSL_AMALGAMATION_CONFIG_H_INCLUDED_)
#define _NET_FOSSIL_SCM_FSL_AMALGAMATION_CONFIG_H_INCLUDED_ 1
#define FSL_AUX_SCHEMA "2015-01-24"
#define FSL_CONTENT_SCHEMA "2"
#define FSL_PACKAGE_NAME "libfossil"
#define FSL_LIBRARY_VERSION "0.0.1-alphabeta"
/* Tweak the following for your system... */
#if !defined(HAVE_COMPRESS)
#  define HAVE_COMPRESS 1
#endif
#if !defined(HAVE_DLFCN_H)
#  define HAVE_DLFCN_H 0
#endif
#if !defined(HAVE_DLOPEN)
#  define HAVE_DLOPEN 0
#endif
#if !defined(HAVE_GETADDRINFO)
#  define HAVE_GETADDRINFO 0
#endif
#if !defined(HAVE_INET_NTOP)
#  define HAVE_INET_NTOP 0
#endif
#if !defined(HAVE_INTTYPES_H)
#  define HAVE_INTTYPES_H 0
#endif
#if !defined(HAVE_LIBDL)
#  define HAVE_LIBDL 0
#endif
#if !defined(HAVE_LIBLTDL)
#  define HAVE_LIBLTDL 0
#endif
#if !defined(_WIN32)
#if !defined(HAVE_LSTAT)
#  define HAVE_LSTAT 1
#endif
#if !defined(HAVE_LTDL_H)
#  define HAVE_LTDL_H 0
#endif
#if !defined(HAVE_LT_DLOPEN)
#  define HAVE_LT_DLOPEN 0
#endif
#if !defined(HAVE_OPENDIR)
#  define HAVE_OPENDIR 1
#endif
#if !defined(HAVE_PIPE)
#  define HAVE_PIPE 1
#endif
#if !defined(HAVE_STAT)
#  define HAVE_STAT 1
#endif
#if !defined(HAVE_STDINT_H)
#  define HAVE_STDINT_H 0
#endif
#if !defined(_DEFAULT_SOURCE)
#  define _DEFAULT_SOURCE 1
#endif
#if !defined(_XOPEN_SOURCE)
#  define _XOPEN_SOURCE 500
#endif
#else
#if !defined(HAVE_LSTAT)
#  define HAVE_LSTAT 0
#endif
#if !defined(HAVE_LTDL_H)
#  define HAVE_LTDL_H 0
#endif
#if !defined(HAVE_LT_DLOPEN)
#  define HAVE_LT_DLOPEN 0
#endif
#if !defined(HAVE_OPENDIR)
#  define HAVE_OPENDIR 1
#endif
#if !defined(HAVE_PIPE)
#  define HAVE_PIPE 0
#endif
#if !defined(HAVE_STAT)
#  define HAVE_STAT 0
#endif
#if !defined(HAVE_STDINT_H)
#  define HAVE_STDINT_H 0
#endif
#endif
/* _WIN32 */


#define FSL_LIB_VERSION_HASH "d9c08847711d398ae048addf0d44b7a320f8bfe1"
#define FSL_LIB_VERSION_TIMESTAMP "2021-08-23 09:52:29.255 UTC"
#define FSL_LIB_CONFIG_TIME "2021-08-23 13:15 GMT"
#if defined(_MSC_VER)
#define FSL_PLATFORM_OS "windows"
#define FSL_PLATFORM_IS_WINDOWS 1
#define FSL_PLATFORM_IS_UNIX 0
#define FSL_PLATFORM_PLATFORM "windows"
#define FSL_PLATFORM_PATH_SEPARATOR ";"
#define FSL_CHECKOUTDB_NAME "./_FOSSIL_"
/* define a __func__ compatibility macro */
#if _MSC_VER < 1500    /* (vc9.0; dev studio 2008) */
/* sorry; cant do much better than nothing at all on those earlier ones */
#define __func__ "(func)"
#else
#define __func__ __FUNCTION__
#endif
/* for the time being at least, don't complain about there being secure crt alternatives: */
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
/* for the time being at least, don't complain about using POSIX names instead of ISO C++: */
#pragma warning ( disable : 4996 )
/* for the time being at least, suppresss some int conversion warnings */
#pragma warning ( disable : 4244 )     /*'fsl_size_t' to 'int'; this masks other problems that should be fixed*/
#pragma warning ( disable : 4761 )     /*'integral size mismatch in argument'; more size_t problems*/
#pragma warning ( disable : 4267 )     /*'size_t' to 'int'; crops up especially in 64-bit builds*/
/* these were extracted from fossil's unistd.h */
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#include <io.h>
#elif defined(__MINGW32__)
#define FSL_PLATFORM_OS "mingw"
#define FSL_PLATFORM_IS_WINDOWS 1
#define FSL_PLATFORM_IS_UNIX 0
#define FSL_PLATFORM_PLATFORM "windows"
#define FSL_PLATFORM_PATH_SEPARATOR ";"
#define FSL_CHECKOUTDB_NAME "./.fslckout"
#elif defined(__CYGWIN__)
#define FSL_PLATFORM_OS "cygwin"
#define FSL_PLATFORM_IS_WINDOWS 0
#define FSL_PLATFORM_IS_UNIX 1
#define FSL_PLATFORM_PLATFORM "unix"
#define FSL_PLATFORM_PATH_SEPARATOR ":"
#define FSL_CHECKOUTDB_NAME "./_FOSSIL_"
#else
#define FSL_PLATFORM_OS "unknown"
#define FSL_PLATFORM_IS_WINDOWS 0
#define FSL_PLATFORM_IS_UNIX 1
#define FSL_PLATFORM_PLATFORM "unix"
#define FSL_PLATFORM_PATH_SEPARATOR ":"
#define FSL_CHECKOUTDB_NAME "./.fslckout"
#endif


#endif
/* _NET_FOSSIL_SCM_FSL_AMALGAMATION_CONFIG_H_INCLUDED_ */

