#if !defined (ORG_FOSSIL_SCM_FSL_CONFIG_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_CONFIG_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

*/
#if defined(_MSC_VER) && !defined(FSL_AMALGAMATION_BUILD)
#  include "config-win32.h" /* manually generated */
#else
#  include "autoconfig.h" /* auto-generated */
#endif

#ifdef _WIN32
# if defined(BUILD_libfossil_static) || defined(FSL_AMALGAMATION_BUILD)
#  define FSL_EXPORT extern
# elif defined(BUILD_libfossil)
#  define FSL_EXPORT extern __declspec(dllexport)
# else
#  define FSL_EXPORT extern __declspec(dllimport)
# endif
#else
# define FSL_EXPORT extern
#endif

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
#  if defined(__cplusplus) && !defined(__STDC_FORMAT_MACROS)
/* inttypes.h needs this for the PRI* and SCN* macros in C++ mode. */
#    define __STDC_FORMAT_MACROS
#  else
#    error "This tree requires a standards-compliant C99-capable compiler."
#  endif
#endif

#include <stdint.h>
#include <inttypes.h>

#if !defined(FSL_AUX_SCHEMA)
#error "Expecting FSL_AUX_SCHEMA to be defined by the configuration bits."
#endif
#if !defined(FSL_LIBRARY_VERSION)
#error "Expecting FSL_LIBRARY_VERSION to be defined by the configuration bits."
#endif


/** @typedef some_int_type fsl_int_t

    fsl_int_t is a signed integer type used to denote "relative"
    ranges and lengths, or to tell a routine that it should try to
    figure out the length of some byte array itself (e.g.  by using
    fsl_strlen() on it). It is provided primarily for
    documentation/readability purposes, to avoid confusion with the
    widely varying integer semantics used by various APIs. This type
    is never used as a return type for functions which use "result code
    semantics." Those always use an unadorned integer type or some
    API-specific enum type.

    The library typedefs this to a 64-bit type if possible, else
    a 32-bit type.
*/
typedef int64_t fsl_int_t;
/**
    The unsigned counterpart of fsl_int_t.
 */
typedef uint64_t fsl_uint_t;
/** @def FSL_INT_T_PFMT

    Fossil's fsl_int_t equivalent of C99's PRIi32 and friends.
 */
#define FSL_INT_T_PFMT PRIi64
/** @def FSL_INT_T_SFMT

    Fossil's fsl_int_t equivalent of C99's SCNi32 and friends.
 */
#define FSL_INT_T_SFMT SCNi64
/** @def FSL_UINT_T_PFMT

    Fossil's fsl_uint_t equivalent of C99's PRIu32 and friends.
 */
#define FSL_UINT_T_PFMT PRIu64
/** @def FSL_UINT_T_SFMT

    Fossil's fsl_uint_t equivalent of C99's SCNu32 and friends.
 */
#define FSL_UINT_T_SFMT SCNu64

/** @def FSL_JULIAN_T_PFMT

    An output format specifier for Julian-format doubles.
 */
#define FSL_JULIAN_T_PFMT ".17g"

/** 
    fsl_size_t is an unsigned integer type used to denote absolute
    ranges and lengths. It is provided primarily for
    documentation/readability purposes, to avoid confusion with the
    widely varying integer semantics used by various APIs. While a
    32-bit type is legal, a 64-bit type is required for "unusually
    large" repos and for some metrics reporting even for mid-sized
    repos.
 */
typedef uint64_t fsl_size_t;

/** @def FSL_SIZE_T_PFMT

    Fossil's fsl_size_t equivalent of C99's PRIu32 and friends.

    ACHTUNG: when passing arguments of this type of fsl_appendf(), or
    any function which uses it for formatting purposes, it is very
    important if if you pass _literal integers_ OR enum values, that
    they be cast to fsl_size_t, or the va_list handling might extract
    the wrong number of bytes from the argument list, leading to
    really weird side-effects via what is effectively memory
    corruption.

    That warning applies primarily to the following typedefs and their
    format specifiers: fsl_size_t, fsl_int_t, fsl_uint_t, fsl_id_t.

    The warning does not apply to strongly-typed arguments,
    e.g. variables of the proper type, so long as the format specifier
    string matches the argument type.

    For example:

    @code
    fsl_size_t sz = 3;
    fsl_fprintf( stdout, "%"FSL_SIZE_T_PFMT" %"FSL_SIZE_T_PFMT\n",
                 sz, // OK!
                 3 // BAD! See below...
                 );
    @endcode

    The "fix" is to cast the literal 3 to a fsl_size_t resp. the type
    appropriate for the format specifier. That ensures that there is
    no (or much less ;) confusion when va_arg() extracts arguments
    from the variadic array.

    Reminders to self:

    @code
    int i = 0;
    f_out(("#%d: %"FSL_ID_T_PFMT" %"FSL_ID_T_PFMT" %"FSL_ID_T_PFMT"\n",
            ++i, 1, 2, 3));
    f_out(("#%d: %"FSL_SIZE_T_PFMT" %"FSL_ID_T_PFMT" %"FSL_SIZE_T_PFMT"\n",
           ++i, (fsl_size_t)1, (fsl_id_t)2, (fsl_size_t)3));
    // This one is the (generally) problematic case:
    f_out(("#%d: %"FSL_SIZE_T_PFMT" %"FSL_ID_T_PFMT" %"FSL_SIZE_T_PFMT"\n",
           ++i, 1, 2, 3));
    @endcode

    The above was Tested with gcc, clang, tcc on a 32-bit linux
    platform (it has not been problematic on 64-bit builds!). The
    above problem was reproduced on all compiler combinations i
    tried. Current code (20130824) seems to be behaving well as long
    as callers always cast to help variadic arg handling DTRT.
 */
#define FSL_SIZE_T_PFMT FSL_UINT_T_PFMT

/** @def FSL_SIZE_T_SFMT

    Fossil's fsl_int_t equivalent of C99's SCNu32 and friends.
 */
#define FSL_SIZE_T_SFMT FSL_UINT_T_SFMT

/**
    fsl_id_t is a signed integer type used to store database record
    IDs. It is provided primarily for documentation/readability purposes,
    to avoid confusion with the widely varying integer semantics used
    by various APIs.

    This type "could" be 32-bit (instead of 64) because the
    oldest/largest Fossil repo (the TCL tree, with 15 years of
    history) currently (August 2013) has only 131k RIDs. HOWEVER,
    changing this type can have side-effects vis-a-vis va_arg() deep
    in the fsl_appendf() implementation if FSL_ID_T_PFMT is not 100%
    correct for this typedef. After changing this, _make sure_ to do a
    full clean rebuild and test thoroughly because changing a sizeof
    can produce weird side-effects (effectively memory corruption) on
    unclean rebuilds.
 */
typedef int32_t fsl_id_t;

/** @def FSL_ID_T_PFMT

    Fossil's fsl_id_t equivalent of C99's PRIi32 and friends.

    ACHTUNG: see FSL_SIZE_T_PFMT for important details.
 */
#define FSL_ID_T_PFMT PRIi32

/** @def FSL_ID_T_SFMT

    Fossil's fsl_id_t equivalent of C99's SCNi32 and friends.
 */
#define FSL_ID_T_SFMT SCNi32

/**
    The type used to represent type values. Unless noted otherwise,
    the general convention is Unix Epoch. That said, Fossil internally
    uses Julian Date for times, so this typedef is clearly the result
    of over-specification/over-thinking the problem. THAT said,
    application-level code more commonly works with Unix timestamps,
    so... here it is. Over-specified, perhaps, but not 100%
    unjustifiable.
 */
typedef int64_t fsl_time_t;

/** @def FSL_TIME_T_PFMT

    Fossil's fsl_time_t equivalent of C99's PRIi32 and friends.
 */
#define FSL_TIME_T_PFMT PRIi64

/** @def FSL_TIME_T_SFMT

    Fossil's fsl_time_t equivalent of C99's SCNi32 and friends.
 */
#define FSL_TIME_T_SFMT SCNi64

/**
   If true, the fsl_timer_xxx() family of functions might do something useful,
   otherwise they do not.
 */
#define FSL_CONFIG_ENABLE_TIMER 1


#endif
/* ORG_FOSSIL_SCM_FSL_CONFIG_H_INCLUDED */
