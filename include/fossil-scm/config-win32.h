#if !defined(_ORG_FOSSIL_SCM_FSL_AUTO_CONFIG_H_INCLUDED_)
#define _ORG_FOSSIL_SCM_FSL_AUTO_CONFIG_H_INCLUDED_ 1
#define FSL_AUX_SCHEMA "2015-01-24"
#define FSL_CONTENT_SCHEMA "2"
#define FSL_PACKAGE_NAME "libfossil"
#define FSL_LIBRARY_VERSION "0.0.1-alphabeta"
/* Tweak the following for your system... */
#define HAVE_GETADDRINFO 1
#define HAVE_INET_NTOP 1
#if !defined(_WIN32)
#define HAVE_DLFCN_H 1
#define HAVE_DLOPEN 1
#define HAVE_LIBDL 1
#define HAVE_LIBLTDL 0
#define HAVE_LSTAT 1
#define HAVE_LTDL_H 0
#define HAVE_LT_DLOPEN 0
#define HAVE_OPENDIR 1
#define HAVE_PIPE 1
#define HAVE_STAT 1
#ifndef _BSD_SOURCE
#define _BSD_SOURCE 1
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif
#else
#define HAVE_DLFCN_H 0
#define HAVE_DLOPEN 0
#define HAVE_LIBDL 0
#define HAVE_LIBLTDL 0
#define HAVE_LSTAT 0
#define HAVE_LTDL_H 0
#define HAVE_LT_DLOPEN 0
#define HAVE_OPENDIR 1
#define HAVE_PIPE 0
#define HAVE_STAT 0
#endif
/*_WIN32*/


#if defined(_MSC_VER)
#define FSL_PLATFORM_OS "windows"
#define FSL_PLATFORM_PLATFORM "windows"
#define FSL_PLATFORM_PATH_SEPARATOR ";"
#define FSL_CKOUTDB_NAME "./_FOSSIL_"
//define a __func__ compatibility macro
#if _MSC_VER < 1500	//(vc9.0; dev studio 2008)
//sorry; cant do much better than nothing at all on those earlier ones
#define __func__ "(func)"
#else
#define __func__ __FUNCTION__
#endif
//for the time being at least, don't complain about there being secure crt alternatives:
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
//for the time being at least, don't complain about using POSIX names instead of ISO C++:
#pragma warning ( disable : 4996 )
//for the time being at least, suppresss some int conversion warnings
#pragma warning ( disable : 4244 )		//'fsl_size_t' to 'int'; this masks other problems that should be fixed
#pragma warning ( disable : 4761 )		//'integral size mismatch in argument'; more size_t problems
#pragma warning ( disable : 4267 )		//'size_t' to 'int'; crops up especially in 64-bit builds
//these were extracted from fossil's unistd.h
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#include <io.h>
#elif defined(__MINGW32__)
#define FSL_PLATFORM_OS "mingw"
#define FSL_PLATFORM_PLATFORM "windows"
#define FSL_PLATFORM_PATH_SEPARATOR ";"
#define FSL_CKOUTDB_NAME "./.fslckout"
#elif defined(__CYGWIN__)
#define FSL_PLATFORM_OS "cygwin"
#define FSL_PLATFORM_PLATFORM "unix"
#define FSL_PLATFORM_PATH_SEPARATOR ":"
#define FSL_CKOUTDB_NAME "./_FOSSIL_"
#else
#define FSL_PLATFORM_OS "unknown"
#define FSL_PLATFORM_PLATFORM "unix"
#define FSL_PLATFORM_PATH_SEPARATOR ":"
#define FSL_CKOUTDB_NAME "./.fslckout"
#endif


#endif
/* _ORG_FOSSIL_SCM_FSL_AUTO_CONFIG_H_INCLUDED_ */

