#if !defined(WANDERINGHORSE_NET_CWAL_S2_AMALGAMATION_H_INCLUDED)
#define WANDERINGHORSE_NET_CWAL_S2_AMALGAMATION_H_INCLUDED
#if !defined(S2_AMALGAMATION_BUILD)
#  define S2_AMALGAMATION_BUILD
#endif
#if !defined(_WIN32)
#  if ! defined(_XOPEN_SOURCE)
   /** on Linux, required for usleep(). */
#    define _XOPEN_SOURCE 500
#  endif
#  ifndef _XOPEN_SOURCE_EXTENDED
#    define _XOPEN_SOURCE_EXTENDED
#  endif
#  ifndef _BSD_SOURCE
#    define _BSD_SOURCE
#  endif
#  ifndef _DEFAULT_SOURCE
#    define _DEFAULT_SOURCE
#  endif
#endif
/* start of file /home/stephan/fossil/cwal/cwal_amalgamation.h */
#if !defined(WANDERINGHORSE_NET_CWAL_AMALGAMATION_H_INCLUDED)
#define WANDERINGHORSE_NET_CWAL_AMALGAMATION_H_INCLUDED
#if !defined(CWAL_VERSION_STRING)
#  define CWAL_VERSION_STRING "cwal 9364c7f5e7516e518902d282fc9fee887405b752 2021-07-24 10:57:44 built 2021-07-24 11:03"
#endif
#if defined(__cplusplus) && !defined(__STDC_FORMAT_MACROS) /* required for PRIi32 and friends.*/
#  define __STDC_FORMAT_MACROS
#endif
#if !defined(CWAL_CPPFLAGS)
#  define CWAL_CPPFLAGS "-I/home/stephan/include -DCWAL_OBASE_ISA_HASH=0  -I. -I/home/stephan/fossil/cwal/include -I/home/stephan/include -DDEBUG=1"
#endif
#if !defined(CWAL_CFLAGS)
#  define CWAL_CFLAGS "-Werror -Wall -Wextra -Wsign-compare -fPIC -std=c99 -g -Wpedantic  -O0"
#endif
#if !defined(CWAL_CXXFLAGS)
#  define CWAL_CXXFLAGS "-g -O2 -fPIC -O0"
#endif
/* start of file include/wh/cwal/cwal_config.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=4 et sw=2 tw=80: */
#if !defined(WANDERINGHORSE_NET_CWAL_CONFIG_H_INCLUDED)
#define WANDERINGHORSE_NET_CWAL_CONFIG_H_INCLUDED 1

#if defined(HAVE_CONFIG_H)
#  include "config.h"
#endif

#if !defined(CWAL_VERSION_STRING)
#define CWAL_VERSION_STRING "?CWAL_VERSION_STRING?"
#endif

#if !defined(CWAL_CPPFLAGS)
#define CWAL_CPPFLAGS "?CWAL_CPPFLAGS?"
#endif

#if !defined(CWAL_CFLAGS)
#define CWAL_CFLAGS "?CWAL_CFLAGS?"
#endif

#if !defined(CWAL_CXXFLAGS)
#define CWAL_CXXFLAGS "?CWAL_CXXFLAGS?"
#endif

#if !defined(CWAL_SWITCH_FALL_THROUGH)
#  if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ >= 7)
/*
  #define CWAL_USING_GCC

  gcc v7+ treats implicit 'switch' fallthrough as a warning
  (i.e. error because we always build with -Wall -Werror -Wextra
  -pedantic). Because now it's apparently considered modern to warn
  for using perfectly valid features of the language. Holy cow, guys,
  what the hell were you thinking!?!?!?

  Similarly braindead, clang #defines __GNUC__.

  So now we need this ugliness throughout the source tree:

  #if defined(CWAL_USING_GCC)
  __attribute__ ((fallthrough));
  #endif

  It turns out that one can write "fall through", case sensitive (or
  not, depending on the warning level), as a _standalone C-style
  comment_ to (possibly) achieve the same result (depending on the
  -Wimplicit-fallthrough=N warning level, which can be set high enough
  to disable that workaround or change its case-sensitivity).

  Facepalm! FacePalm!! FACEPALM!!!

  PS: i wanted to strip comments from one large piece of generated
  code to reduce its distribution size, but then gcc fails to compile
  it because of these goddamned "fall through" comments. gcc devs, i
  hate you for this.
*/
#    define CWAL_SWITCH_FALL_THROUGH __attribute__ ((fallthrough))
#  else
#    define CWAL_SWITCH_FALL_THROUGH
#  endif
#endif
/* /CWAL_SWITCH_FALL_THROUGH

   TODO: add support for the C++ attributes for doing this.
*/

#if defined(__arm__) || defined(__thumb__) \
    || defined(__TARGET_ARCH_ARM) \
    || defined(__TARGET_ARCH_THUMB) \
    || defined(_ARM) \
    || defined(_M_ARM) \
    || defined(_M_ARMT)
/* adapted from http://sourceforge.net/p/predef/wiki/Architectures/ */
#define CWAL_PLATFORM_ARM 1
#endif

#if defined(__cplusplus) && !defined(__STDC_FORMAT_MACROS)
/* inttypes.h needs this for the PRI* and SCN* macros in C++ mode. */
#  define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h> /* C99 PRIuXX macros and fixed-size
                         integers. */
/*#define CWAL_VOID_PTR_IS_BIG 1*/

#if defined(CWAL_SIZE_T_BITS)
# error "CWAL_SIZE_T_BITS must not be defined before including this file! Edit this file instead!"
#endif

/**
   A workaround for late-2015 gcc versions adding __func__ warnings
   to -pedantic mode.
*/
#if !defined(__cplusplus) && !defined(__func__)
#  if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
#    define __func__ "__func__"
#  endif
#endif


/** @def CWAL_ENABLE_JSON_PARSER

   If CWAL_ENABLE_JSON_PARSER is set to 0 then the cwal_json_parse()
   family of functions get disabled (they still exist, but will return
   errors). It has no effect on the JSON output routines.

   The reason for this is only one of licensing: the (3rd-party)
   parser uses a BSD-style license with a "do no evil" clause, and
   that clause prohibits the code from being used in certain package
   repositories.

   Note that setting this macro might not be enough to please package
   maintainers - they may also require physical removal of the code
   blocked off by this macro. It can all be found in cwal_json.c.
*/
#if !defined(CWAL_ENABLE_JSON_PARSER)
#define CWAL_ENABLE_JSON_PARSER 1
#endif

/** @def CWAL_DISABLE_FLOATING_POINT

    "The plan is" to allow compiling cwal with no support for doubles, but
    that is not yet in place.
*/
#if !defined(CWAL_DISABLE_FLOATING_POINT)
#define CWAL_DISABLE_FLOATING_POINT 0
#endif

/** @def CWAL_VOID_PTR_IS_BIG

    ONLY define this to a true value if you know that

    (sizeof(cwal_int_t) <= sizeof(void*))

    If that is the case, cwal does not need to dynamically allocate
    integers. ALL CLIENTS must use the same value for this macro.

    This value is (or was?) also used to
    optimize/modify/make-more-portable a couple other internal bits.

    FIXME: figure out exactly how this plays together with
    CWAL_INT_T_BITS and document what combinations are portable across
    c89/c99 and 32/64 bits.
    */
#if !defined(CWAL_VOID_PTR_IS_BIG)

    /* Largely taken from http://predef.sourceforge.net/prearch.html

    See also: http://poshlib.hookatooka.com/poshlib/trac.cgi/browser/posh.h
    */
#  if defined(_WIN64) || defined(__LP64__)/*gcc*/                       \
    || defined(_M_X64) || defined(__amd64__) || defined(__amd64)        \
    ||  defined(__x86_64__) || defined(__x86_64)                        \
    || defined(__ia64__) || defined(__ia64) || defined(_IA64) || defined(__IA64__) \
    || defined(_M_IA64)                                                 \
    || defined(__sparc_v9__) || defined(__sparcv9) || defined(_ADDR64)  \
    || defined(__64BIT__)
#    define CWAL_VOID_PTR_IS_BIG 1
#  else
#    define CWAL_VOID_PTR_IS_BIG 0
#  endif
#endif

/** @def CWAL_SIZE_T_BITS

    CWAL_SIZE_T_BITS defines the number of bits used by cwal's primary
    cwal_size_t. This is used so that cwal can guaranty and enforce
    certain number ranges.

    This value must be one of (16,32,64), though values lower than 32
    may or may not work well with any given component of the library
    (e.g. string interning can use relatively much compared to the
    rest of the lib).
*/

/** @def CWAL_INT_T_BITS
    
   CWAL_INT_T_BITS is the cwal_int_t counterpart of
   CWAL_SIZE_T_BITS. cwal_int_t is independent of cwal_size_t, and may
   have a different size.

*/

#if 0
/**
   For testing purposes, or if a client wants to override this
   globally...

   Reminder to self: a value of 64 does not work on ODroid ARM/Linux.
   Problems are mostly related to hash seeds and bitmasks being too
   large for unsigned long on that platform.
*/
#  define CWAL_SIZE_T_BITS 16
#endif

#if !defined(CWAL_SIZE_T_BITS)
/**
  CWAL_SIZE_T_BITS "really should" not be higher than the pointer size
  (in bits), since it would be impossible to allocate anywhere near
  that number of items and this value is largely used as a length- and
  reference counter. It is NOT anticipated that cwal will be used in
  any environments where an unsigned 32-bit limit could ever be
  reached for the things it uses cwal_size_t for (discounting
  artifical/malicious inflation of reference counts and such).

  A value of 16 is perfectly reasonable for small use cases. It
  doesn't save _much_ memory, but it does save some.
*/
#  if CWAL_VOID_PTR_IS_BIG
#    define CWAL_SIZE_T_BITS 64
#  else
#    define CWAL_SIZE_T_BITS 32
#  endif
#endif

#if !defined(CWAL_INT_T_BITS)
/**
  The ONLY reason we fall back to 32 bits here is because C89 lacks a
  portable printf format string for the equivalent of PRIi64
  :/. Other than that 64-bits will (should!) work find on 32-bit
  platforms as long as CWAL_VOID_PTR_IS_BIG is false. In C99 mode
  64-bit int compiles fine on 32-bit (because the result of PRIi64 is
  well-defined there).
*/
#  if CWAL_VOID_PTR_IS_BIG || (defined(__STDC_VERSION__) && (__STDC_VERSION__>=199901L))
#    define CWAL_INT_T_BITS CWAL_SIZE_T_BITS
#  else
#    define CWAL_INT_T_BITS CWAL_SIZE_T_BITS
#  endif
#endif


/** @def CWAL_SIZE_T_PFMT

    Is is a printf-style specifier, minus the '%' prefix, for
    use with cwal_size_t arguments. It can be used like this:

    @code
    cwal_size_t x = 42;
    printf("The value of x is %"CWAL_SIZE_T_PFMT".", x );
    @endcode

    Using this constant ensures that the printf-style commands
    work when cwal_size_t is of varying sizes.

    @see CWAL_SIZE_T_SFMT
*/

/** @def CWAL_SIZE_T_SFMT

CWAL_SIZE_T_SFMT is the scanf counterpart of CWAL_SIZE_T_PFMT.

@see CWAL_SIZE_T_PFMT
@see CWAL_SIZE_T_SFMT
*/

/** @def CWAL_SIZE_T_PFMTX

CWAL_SIZE_T_PFMTX is the upper-case hexidecimal counterpart of
CWAL_SIZE_T_PFMT.

@see CWAL_SIZE_T_PFMT
*/

/** @def CWAL_SIZE_T_PFMTx

CWAL_SIZE_T_PFMTX is the lower-case hexidecimal counterpart of
CWAL_SIZE_T_PFMT.

@see CWAL_SIZE_T_PFMT
*/


/** @def CWAL_SIZE_T_PFMTo

CWAL_SIZE_T_PFMTo is the octal counterpart of CWAL_SIZE_T_PFMT.

@see CWAL_INT_T_SFMT
*/

    
/** @def CWAL_SIZE_T_SFMTX

CWAL_SIZE_T_SFMTX is the hexidecimal counterpart to CWAL_SIZE_T_SFMT.

@see CWAL_SIZE_T_PFMT
@see CWAL_SIZE_T_SFMT
*/

/** @def CWAL_INT_T_PFMT

CWAL_INT_T_PFMT is the cwal_int_t counterpart of CWAL_SIZE_T_PFMT.

@see CWAL_SIZE_T_PFMT
@see CWAL_INT_T_SFMT
*/

/** @def CWAL_INT_T_SFMT

CWAL_INT_T_SFMT is the scanf counterpart of CWAL_INT_T_PFMT.

@see CWAL_INT_T_PFMT
*/

/** @def CWAL_INT_T_PFMTX

CWAL_INT_T_PFMTX is the upper-case hexidecimal counterpart of
CWAL_INT_T_PFMT.

@see CWAL_INT_T_SFMT
*/

/** @def CWAL_INT_T_PFMTx

CWAL_INT_T_PFMTX is the lower-case hexidecimal counterpart of
CWAL_INT_T_PFMT.

@see CWAL_INT_T_SFMT
*/

/** @def CWAL_INT_T_PFMTo

CWAL_INT_T_PFMTo is the octal counterpart of CWAL_INT_T_PFMT.

@see CWAL_INT_T_SFMT
*/

    
/** @def CWAL_INT_T_SFMTX

CWAL_INT_T_SFMTX is the hexidecimal counterpart to CWAL_INT_T_SFMT.

@see CWAL_INT_T_PFMT
@see CWAL_INT_T_SFMT
*/


/** @def CWAL_INT_T_MIN

CWAL_INT_T_MIN is the minimum value of the data type cwal_int_t.

@see CWAL_INT_T_MAX
*/

/** @def CWAL_INT_T_MAX

CWAL_INT_T_MAX is the maximum value of the data type cwal_int_t.

@see CWAL_INT_T_MAX
*/



/** typedef some_unsigned_int_type_which_is_CWAL_SIZE_T_BITS_long cwal_size_t

cwal_size_t is a configurable unsigned integer type specifying the
ranges used by this library. Its exact type depends on the value of
CWAL_SIZE_T_BITS: it will be uintXX_t, where XX is the value of
CWAL_SIZE_T_BITS (16, 32, or 64).

We use a fixed-size numeric type, instead of relying on a standard
type with an unspecified size (e.g. size_t) to help avoid nasty
surprises when porting to machines with different size_t
sizes.

For cwal's intended purposes uint16_t is "almost certainly" fine, but
those who are concerned about 64kb limitations on certain contexts
might want to set this to uint32_t.

*/

/** typedef some_unsigned_int_type cwal_midsize_t

A cwal_size_t counterpart which is intended to be capped at 32 bits
and used in contexts for which 64 bits is simply a massive waste (e.g.
arrays, strings, and hashtables).
*/

/** @typedef some_signed_integer cwal_int_t

    This is the type of integer value used by the library for its
    "script-visible" integers.
*/

/** @typedef some_unsigned_integer cwal_refcount_t

    This is the type of integer value used by the library to keep
    track of both reference counts and their embedded flags (which
    reduce the useful range of the reference count).
*/

/** @def CWAL_REFCOUNT_T_BITS

    @internal

    MUST be equal to (sizeof(cwal_refcount_t) * 8), but must be a
    constant usable by the preprocessor.
*/


/* Set up CWAL_SIZE_T... */
#if CWAL_SIZE_T_BITS == 16
#  define CWAL_SIZE_T_PFMT PRIu16
#  define CWAL_SIZE_T_PFMTx PRIx16
#  define CWAL_SIZE_T_PFMTX PRIX16
#  define CWAL_SIZE_T_PFMTo PRIo16
#  define CWAL_SIZE_T_SFMT SCNu16
#  define CWAL_SIZE_T_SFMTX SCNx16
#  define CWAL_SIZE_T_MAX 65535U
    typedef uint16_t cwal_size_t;
    typedef uint32_t cwal_refcount_t /* yes, 32, because we store flags in cwal_value refcounts */;
#  define CWAL_REFCOUNT_T_BITS 32
#elif CWAL_SIZE_T_BITS == 32
#  define CWAL_SIZE_T_PFMT PRIu32
#  define CWAL_SIZE_T_PFMTx PRIx32
#  define CWAL_SIZE_T_PFMTX PRIX32
#  define CWAL_SIZE_T_PFMTo PRIo32
#  define CWAL_SIZE_T_SFMT SCNu32
#  define CWAL_SIZE_T_SFMTX SCNx32
#  define CWAL_SIZE_T_MAX 4294967295U
    typedef uint32_t cwal_size_t;
    typedef uint32_t cwal_refcount_t;
#  define CWAL_REFCOUNT_T_BITS 32
#elif CWAL_SIZE_T_BITS == 64
#  define CWAL_SIZE_T_PFMT PRIu64
#  define CWAL_SIZE_T_PFMTx PRIx64
#  define CWAL_SIZE_T_PFMTX PRIX64
#  define CWAL_SIZE_T_PFMTo PRIo64
#  define CWAL_SIZE_T_SFMT SCNu64
#  define CWAL_SIZE_T_SFMTX SCNx64
#  define CWAL_SIZE_T_MAX 18446744073709551615U
    typedef uint64_t cwal_size_t;
    typedef uint64_t cwal_refcount_t /*32 bits "should be" fine, but using
                                       64 doesn't (because of padding) actually change
                                       the sizeof of cwal_value */;
#  define CWAL_REFCOUNT_T_BITS 64
#else
#  error "CWAL_SIZE_T_BITS must be one of: 16, 32, 64"
#endif

/* Set up CWAL_MIDSIZE_... */
#if CWAL_SIZE_T_BITS == 16
    typedef uint16_t cwal_midsize_t;
#  define CWAL_MIDSIZE_T_PFMT CWAL_SIZE_T_PFMT
#  define CWAL_MIDSIZE_T_PFMTx CWAL_SIZE_T_PFMTx
#  define CWAL_MIDSIZE_T_PFMTX CWAL_SIZE_T_PFMTX
#  define CWAL_MIDSIZE_T_PFMTo CWAL_SIZE_T_PFMTo
#  define CWAL_MIDSIZE_T_SFMT CWAL_SIZE_T_SFMT
#  define CWAL_MIDSIZE_T_SFMTX CWAL_SIZE_T_SFMTX
#  define CWAL_MIDSIZE_T_MAX CWAL_SIZE_T_MAX
#else
    typedef uint32_t cwal_midsize_t;
#  define CWAL_MIDSIZE_T_PFMT PRIu32
#  define CWAL_MIDSIZE_T_PFMTx PRIx32
#  define CWAL_MIDSIZE_T_PFMTX PRIX32
#  define CWAL_MIDSIZE_T_PFMTo PRIo32
#  define CWAL_MIDSIZE_T_SFMT SCNu32
#  define CWAL_MIDSIZE_T_SFMTX SCNx32
#  define CWAL_MIDSIZE_T_MAX 4294967295U
#endif

/* Set up CWAL_INT_... */
#if CWAL_INT_T_BITS == 16
#  define CWAL_INT_T_PFMT PRIi16
#  define CWAL_INT_T_PFMTx PRIx16
#  define CWAL_INT_T_PFMTX PRIX16
#  define CWAL_INT_T_PFMTo PRIo16
#  define CWAL_INT_T_SFMT SCNi16
#  define CWAL_INT_T_SFMTX SCNX16
#  define CWAL_INT_T_MAX 32767
    typedef int16_t cwal_int_t;
    typedef uint16_t cwal_uint_t;
#elif CWAL_INT_T_BITS == 32
#  define CWAL_INT_T_PFMT PRIi32
#  define CWAL_INT_T_PFMTx PRIx32
#  define CWAL_INT_T_PFMTX PRIX32
#  define CWAL_INT_T_PFMTo PRIo32
#  define CWAL_INT_T_SFMT SCNi32
#  define CWAL_INT_T_SFMTX SCNx32
#  define CWAL_INT_T_MAX 2147483647
    typedef int32_t cwal_int_t;
    typedef uint32_t cwal_uint_t;
#elif CWAL_INT_T_BITS == 64
#  define CWAL_INT_T_PFMT PRIi64
#  define CWAL_INT_T_PFMTx PRIx64
#  define CWAL_INT_T_PFMTX PRIX64
#  define CWAL_INT_T_PFMTo PRIo64
#  define CWAL_INT_T_SFMT SCNi64
#  define CWAL_INT_T_SFMTX SCNx64
#  define CWAL_INT_T_MAX 9223372036854775807
    typedef int64_t cwal_int_t;
    typedef uint64_t cwal_uint_t;
#else
#  error "CWAL_INT_T_BITS must be one of: 16, 32, 64"
#endif
#define CWAL_INT_T_MIN ((-CWAL_INT_T_MAX)-1)
/*
   Reminder: the definition ((-CWAL_INT_T_MAX)-1) was gleaned from the
   clang headers, but AFAIK C does not actually define what happens
   for under/overflow for _signed_ types. Trying to use the literal
   value in 64-bit mode gives me a compile error on gcc ("constant
   value is only signed in C99", or some such).
*/

/** @typedef some_unsigned_int_type cwal_hash_t

   Hash value type used by the library. It must be an unsigned integer
   type.
*/
#if 16 == CWAL_INT_T_BITS
typedef uint32_t cwal_hash_t /* need 32-bit for some hash seeds */;
#elif 32 == CWAL_INT_T_BITS
typedef uint32_t cwal_hash_t;
#elif 64 == CWAL_INT_T_BITS
typedef uint64_t cwal_hash_t;
#endif

/** @typedef double_or_long_double cwal_double_t

    This is the type of double value used by the library.  It is only
    lightly tested with long double, and when using long double the
    memory requirements for such values goes up (of course).

    Note that by default cwal uses C-API defaults for
    numeric precision. To use a custom precision throughout
    the library, one needs to define the macros CWAL_DOUBLE_T_SFMT
    and/or CWAL_DOUBLE_T_PFMT macros to include their desired
    precision, and must build BOTH cwal AND the client using
    these same values. For example:

    @code
    #define CWAL_DOUBLE_T_PFMT ".8Lf"
    #define HAVE_LONG_DOUBLE
    @endcode
*/
#if CWAL_DISABLE_FLOATING_POINT
   /* No doubles support: use integers instead so we don't have to
      block out portions of the API.
   */
   typedef cwal_int_t cwal_double_t;
#  define CWAL_DOUBLE_T_SFMT CWAL_INT_T_SFMT
#  define CWAL_DOUBLE_T_PFMT CWAL_INT_T_PFMT
#else
#  if defined(HAVE_LONG_DOUBLE)
     typedef long double cwal_double_t;
#  ifndef CWAL_DOUBLE_T_SFMT
#    define CWAL_DOUBLE_T_SFMT "Lf"
#  endif
#  ifndef CWAL_DOUBLE_T_PFMT
#    define CWAL_DOUBLE_T_PFMT "Lf"
#  endif
#  else
     typedef double cwal_double_t;
#  ifndef CWAL_DOUBLE_T_SFMT
#    define CWAL_DOUBLE_T_SFMT "f"
#endif
#  ifndef CWAL_DOUBLE_T_PFMT
#    define CWAL_DOUBLE_T_PFMT "f"
#endif
#  endif
#endif

#if !defined(CWAL_ENABLE_BUILTIN_LEN1_ASCII_STRINGS)
/**
   If true when cwal is compiled (as opposed to later on, when
   including this header via client code), cwal includes all 128
   length-1 ASCII strings in its list of static builtin values. This
   list includes a length-1 cwal_string instance (builtin/constant)
   for all ASCII values (0 to 127, inclusive). This has a static
   memory cost of ~7.5k on a 64-bit build but has the potential to
   save scads of allocations, especially if client code consciously
   takes advantage of it (mine does!). Here's an out-take from the
   cwal metrics dump after running the s2 unit test suit (20181128):

   @code
   Length-1 ASCII string optimization savings...
      strings:   2681 allocation(s), 150136 bytes.
      x-strings: 1 allocation(s), 56 bytes.
      z-strings: 1 allocation(s), 56 bytes.
   Total savings: 2683, allocation(s), 150248 bytes.
   @endcode
*/
#  define CWAL_ENABLE_BUILTIN_LEN1_ASCII_STRINGS 1
#endif

#if !defined(CWAL_ENABLE_TRACE)
/**
   Setting CWAL_ENABLE_TRACE to a true value enables
   cwal-internal tracing. Profiling shows it to be very
   expensive, and its level of detail is too great for most
   users to be able to do anything with, so it is recommended
   that it be left off unless needed.
 */
#  define CWAL_ENABLE_TRACE 1
#endif


#if !defined(CWAL_OBASE_ISA_HASH)
/** @def CWAL_OBASE_ISA_HASH

  If CWAL_OBASE_ISA_HASH is true then cwal_obase will use a hashtable,
  instead of a sorted list, for its property management. That works
  the same as the "legacy" mode (sorted doubly-linked property lists)
  except that (1) it's computationally faster, (2) requires more
  memory, and (3) has type-strict property keys. e.g. in legacy mode
  the property keys "1" (string) and 1 (integer) are equivalent, but
  they are distinct keys in hashtable mode.

  Because this flag changes the sizeof() of the cwal container types,
  it is critical that all compilation units (including loadable
  modules and any downstream client code) use the same value! The
  easiest(?) way to ensure that that happens when using the cwal
  amalgamation build (the preferred approach) from a client tree is to
  define HAVE_CONFIG_H for the whole build, which will cause this file
  to #include "config.h", where this macro can be set to a default
  value.
*/
#  define CWAL_OBASE_ISA_HASH 0
#endif

#endif
/* WANDERINGHORSE_NET_CWAL_CONFIG_H_INCLUDED */
/* end of file include/wh/cwal/cwal_config.h */
/* start of file include/wh/cwal/cwal.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
#if !defined(WANDERINGHORSE_NET_CWAL_H_INCLUDED)
#define WANDERINGHORSE_NET_CWAL_H_INCLUDED 1

#if defined(HAVE_CONFIG_H) || defined(HAVE_AUTOCONFIG_H)
#  include "config.h"
#endif
#include <stdarg.h> /* va_list */
#include <stdio.h> /* FILE decl */
#include <stdbool.h>

/**
   @page page_cwal cwal API

   cwal (pronounced "sea wall") is the Scriping Engine Without A
   Language (sewal, shortened to cwal): an object-oriented C99 API
   providing _part_ of a scripting engine, namely the engine and not
   the scripting. The intention is that custom scripting
   languages/mini-languages can be written on top of this basis, which
   takes care of the core type system, de/allocation of values,
   tracking references, and other low-level work which is largely
   independent of any specific language syntax. Its design does impose
   some conventions/requirements on host languages, but nothing too
   onerous, it is hoped. Alternately, it can be used as sort of
   garbage collection system for clients willing to use its type
   system. Another potential use might be a data-binding mechanism
   between, e.g. database drivers and client code, acting as a
   type-normalization layer between the two (so the client code can be
   written without much knowledge of the underlying driver(s)).

   cwal's API is stable in the sense that much client-side code relies
   on it which we don't want broken, but will always be beta in the
   sense that it's open to experimentation and change. It is very rare
   (nowadays) that it is modified in ways which "break stuff."  As of
   mid-2014 we have a good deal of add-on code, in particular with the
   s2 scripting engine, which we would like to keep running, so
   massive changes are unlikely. Though s2 is co-developed with cwal,
   the core cwal engine is intentionally kept free of
   scriping-language-specific artifacts.

   Project home page: https://fossil.wanderinghorse.net/r/cwal

   Author: Stephan Beal (https://www.wanderinghorse.net/home/stephan/)

   License: Dual Public Domain/MIT

   The full license text is in the main header file (cwal.h or
   cwal_amalgamation.h, depending on the distribution used): search
   the file for the word LICENSE. The optional JSON parser, from a 3rd
   party, has its own license, equivalent to the MIT license.

   Examples of how to use the library are scattered throughout the API
   documentation, in the test.c file in the source repo, in the wiki
   on the project home page, and (in a big way) in the s2 subdirectory
   of the main source tree.

   Main properties of cwal:

   - cwal does NOT provide a scripting language. It is an engine which
   "could" be used as a basis for one. It can also be used as simple
   form of garbage collector for client apps willing to live with its
   scoping mechanism.

   - Provides a type system similar (not identical) to that of
   ECMAScript. Values are opaque handles with some polymorphic
   behaviours depending on their logical type. Provides support for
   binding client-specified "native" values, such that they can
   participate in the normal lifetime tracking and destruction process
   and can be converted from their Value handles with 100%
   type-safety.

   - Uses a reference-counting/scope-ownership hybrid for value
   sharing and garbage collection. Its destruction mechanism behaves
   sanely when faced with cycles provided the rules of the API are
   followed (they're pretty easy).

   - Destruction (finalizer calls) is gauranteed and happens more or
   less like it does in C++, with the addition that refcounting can be
   used to extend the lifetime of a value outside of the scope where
   it is created. While refcounting in conjunction with container can
   make destruction order somewhat difficult to predict, it is still
   deterministic in that finalizers are guaranteed to be called, so
   long as the API is properly used.

   - The wrapping of client-provided native types uses a type-safety
   mechanism to ensure that clients are getting the type of pointer
   they expect when fetching the (void*) from its cwal value type
   counterpart. Such types participate as first-class Values and may
   contain properties (meaning they can participate in graphs).

   - Provides optional automatic "internalizing" of new string values,
   and all strings which share the same length and byte content are
   automatically shared via the reference-counting mechanism. When the
   final reference is released the string is un-internalized
   automatically. This causes a couple corner cases internally but can
   drastrically reduce allocations in scripts which make heavy use of
   identifier strings.

   - Highly optimized to reduce calls to malloc() and free(), and
   optionally makes use of memory recycling. The recycling limits can
   be set on a per-data-type basis. Practice has shown recycling to
   be tremendously effective at reducing malloc calls by over 90%
   in typical script code (a 98% reduction is not uncommon!).

   - Optionally provides tracing (via callbacks) so that clients
   can see what it is doing on the inside.

   - "Relatively clean" code is a design goal, as is having relatively
   good documentation.
*/

/**
   @page page_cwal_gc cwal Garbage Collection

   This section describes how cwal manages the memory of Values
   created by the client.
   
   cwal's main concern, above all, is memory management. This includes
   at least the following aspects:

   - Allocation and deallocation of raw memory. All memory allocation
   in the context of cwal is handled via a cwal_engine instance (a
   context type of which an application has one per "scripting
   engine"), and clients may optionally specify their own allocator
   when initializing a cwal_engine instance. cwal internally manages a
   number of optional recycling bins, broken down by data type, where
   it stores freed Values (and other internals) for re-use. Once the
   recycle bins have been populated a bit, it has to allocate memory
   far less often (how often depends on usage and recycler
   configuration).

   - Tracking the lifetimes of components which clients cannot
   reasonably track themselves. This refers specifically to Values,
   since their inter-relationships (via hierachies and key/value
   properties) can easily lead to unpredictable patterns which would
   be unrealistically burdensome for client code to try to properly
   manage.

   cwal's garbage collection mechanism has two basic components:
   reference counts and scopes.

   cwal uses convential refcount semantics. The reference count of a
   Value is increased when code "expresses an interest" in the Value,
   e.g. it is inserted into a container or an explicit call to
   cwal_value_ref() is called.  Reference counts are decreased as
   Values are removed from containers (e.g. Objects and Arrays) or
   cwal_value_unref() is called. When the refcount goes to 0, the
   Value is typically cleaned up (recursively for containers), but the
   engine also offers an operation which says "reduce the refcount but
   do not destroy the object if it reaches 0," which is amazingly
   useful in lifetime management of opaque values.

   Scopes act as a "root" for a collection of values, such that
   cleaning up a scope will clean up all values under that scope
   (regardless of their reference count). cwal uses C++-like scoping
   rules: a stack of scopes, only the youngest of which is active at
   any given time.

   Reference counting is straightforward and easy to implement but is
   conventionally problematic when it comes to managing graphs of
   Values (i.e. cyclic data structures). When a cwal scope is closed,
   it destructs any values it owns by iteratively dereferencing
   them. This process removes the values from the scope in such a way
   that graphs get weeded out incrementally (one level at a time) and
   (more or less) cleanly. Consider this pseudo-script-code:

   var obj = new Object();
   obj.set(obj, obj); // (key, value)

   (Admitedly unusual, but cwal's value system supports this.)

   We now have an object with 3 references: 1 held by the identifier
   mapping (i.e. the declared variable), one held by the property
   key, and one held by the property value. (Possibly others,
   depending on the scripting implementation.)

   Now we do:

   unset obj

   That removes the identifier and its reference. That leaves us with
   a refcount of 2 and no handle back to the object. The object must
   be kept alive because there are references (the key/value refering
   to the object itself). That value will remain an orphan until its
   owning scope is cleaned, at which point the scope's cleaning
   process will weed out the cycles and finalize the object when the
   final reference is removed. (cwal_scope_vacuum() can weed that out
   and clean it up, though.)

   There are of course wrinkles in that equation. For example, the
   finalization process for an object must recursively climb down
   properties, and in the above case doing so will trigger
   finalization of an object while it is traversing itself. cwal's
   cleanup mechanism temporarily delays freeing of Values' memory
   during scope cleanup, queuing them up for later destruction. The
   end effect is that Values get cleaned up but the memory remains
   valid until scope cleanup has finished. This makes it safe (if
   psychologically a bit unsettling) to traverse "destroyed" objects
   during finalization (it is a harmless no-op). Once scope cleanup is
   complete, all queued-up destroyed values are then flushed to the
   deallocator (or the recycling bin, if it has space).

   Summary: in cwal it is possible to orphan graphs in such a way that
   the client has no reference to them but circular references prevent
   cleanup. Such values will be freed when their parent scope is
   cleaned. There is also a "sweep" mechanism to trigger cleanups of
   values with no references and a more intensive "vacuum" operation
   which can also weed out unreachable cyclic structures. (Note that
   cwal does not use a "mark-and-sweep" approach: all memory it
   manages is carefully accounted for in internal bookkeeping, and
   there is no guesswork about which memory might or might not be
   "marked" for cleanup.)

   Speaking of parent scopes... cwal complements the reference
   counting with the concept of "owning scope." Every value belongs to
   exactly one scope, and this scope is always the highest-level
   (oldest) scope which has ever referenced the value. When a value is
   created, it belongs to the scope which is active at the
   time. Values can be rescoped, however, moving up in the stack (into
   an older scope, never a newer scope). When a value is inserted into
   a container, the value is scoped into the container's owning
   scope. When the container is added to another, the container is
   recursively re-scoped (if necessary) to match the scope of its
   parent container. This ensures that values from a higher-level
   (older) scope are always valid through the lifetime of lower-level
   (newer) scopes. It also allows a scope to pass a value to its
   parent scope. When a value is re-scoped, it is removed from its
   owning scope's management list and added to the new scope's
   list. These list are linked lists and use O(1) algos for the list
   management (except for cleanup, which is effectively O(N) on the
   number of values being cleaned up).

   As of this writing, in Dec. 2019, cwal has been in moderately heavy
   use for more than 5 years, first in the now-deceased "th1ish"
   scripting engine and subsequently in the "s2" engine, which has
   demonstrated, beyond the shadow of a doubt, the engine's ability to
   handle modest scripting-language needs. s2's range of features
   extend far, far beyond any initial hopes i had for cwal, and
   provide me with a useful tool with which to script my other
   software or do certain day-to-day tasks such as generate static
   pages for my website from s2-enabled template files (kind of like a
   poor man's PHP) and power a number of my JSON-based CGI
   applications (backend services for various web apps).
*/
#if defined(__cplusplus)
extern "C" {
#endif

/* Forward declarations. Most of these types are opaque to client
   code. */
typedef struct cwal_scope cwal_scope;
typedef struct cwal_engine_vtab cwal_engine_vtab;
typedef struct cwal_engine cwal_engine;
typedef struct cwal_value cwal_value;
typedef struct cwal_array cwal_array;
typedef struct cwal_object cwal_object;
typedef struct cwal_string cwal_string;
typedef struct cwal_kvp cwal_kvp;
typedef struct cwal_native cwal_native;
typedef struct cwal_buffer cwal_buffer;
typedef struct cwal_exception cwal_exception;
typedef struct cwal_hash cwal_hash;
typedef struct cwal_weak_ref cwal_weak_ref;
typedef struct cwal_callback_hook cwal_callback_hook;
typedef struct cwal_callback_args cwal_callback_args;
typedef struct cwal_tuple cwal_tuple;
typedef struct cwal_error cwal_error;

/**
   Typedef for flags fields which are limited to 16 bits.
*/
typedef uint16_t cwal_flags16_t;
/**
   Typedef for flags fields which are limited to 32 bits.
*/
typedef uint32_t cwal_flags32_t;

/**
   A callback type for pre-call callback hooks. If a hook is
   installed via cwal_callback_hook_set(), its "pre" callback is
   called before any "script-bound" function is called.

   The first parameter is the one which will be passed to the
   callback and post-callback hook if this hook succeeds.

   The state parameter (2nd argument) is the one passed to
   cwal_callback_hook_set(). Its interpretation is
   implementation-defined.

   The callback must return 0 on success. On error, the non-0
   result code (from the CWAL_RC_xxx set) is returned from the
   cwal API which triggered the hook (cwal_function_call() and
   friends). On success, the callback is called and then
   (regardless of callback success!) the "post" callback is
   called.

   The intention of the callback hook mechanism is to give script
   engines a places to do pre-call setup such as installing local
   variables (e.g. similar to JavaScript's "this", "arguments", and
   "arguments.callee"). It can also be used for instrumentation
   purposes (logging) or to implement multi-casting of client-side
   pre-call hook mechanism to multiple listeners.

   Note that the hook mechanism is "global" - it affects all
   cwal_function_call_in_scope() calls, which means all
   script-triggered callbacks and any other place a
   cwal_function_call() (or similar) is used. It _is_ possible
   (using cwal_function_state_get()) for a client to determine
   whether the callback exists in script code or native code,
   which means that hooks can act dependently of that if they need
   to. e.g. there is generally no need to inject "this" and
   "arguments" symbols into native-level callbacks, whereas it is
   generally useful to do so for script-side callbacks. Native
   callbacks have access to the same information via the argv
   object, so they don't need scope-level copies of those values.
   (That said, native callbacks which call back into script code
   might need such variables in place!)

   @see cwal_callback_hook_post_f()
   @see cwal_callback_hook
   @see cwal_callback_hook_set()
   @see cwal_callback_f
*/
typedef int (*cwal_callback_hook_pre_f)(cwal_callback_args const * argv, void * state);

/**
   The "post" counterpart of cwal_callback_hook_pre_f(), this hook is
   called by the cwal core after calling a cwal_function via and of
   the cwal_function_call() family of functions.

   The state parameter (2nd argument) is the one passed to
   cwal_callback_hook_set(). Its interpretation is
   implementation-defined.

   The fRc (3rd) parameter is the return code of the callback, in case
   it interests the hook (e.g. error logging). If this value is not 0
   then the rv parameter will be NULL.

   The rv (4rd) parameter is the result value of the function. The
   engine will re-scope rv, if necessary, brief milliseconds after
   this hook returns. rv MAY be NULL, indicating that the callback did
   not set a value (which typically equates to the undefined value
   (cwal_value_undefined()) downstream or may be because an exception
   was thrown or a non-exception error code was returned).

   If the "pre" hook is called and returns 0, the API guarantees
   that the post-hook is called. Conversely, if the pre hook
   returns non-0, the post hook is never called.

   If this callback returns non-0, that will be the result returned
   from the cwal_function_call()-like function which triggered it.

   @see cwal_callback_hook_pre_f()
   @see cwal_callback_hook
   @see cwal_callback_hook_set()
*/
typedef int (*cwal_callback_hook_post_f)(cwal_callback_args const * argv,
                                         void * state,
                                         int fRc, cwal_value * rv);

/**
   Holds state information for a set of cwal_engine callback
   hooks.

   @see cwal_callback_hook_pre_f()
   @see cwal_callback_hook_post_f()
   @see cwal_callback_hook_set()
*/
struct cwal_callback_hook {
  /**
     Implementation-dependent state pointer which gets passed
     as the 2nd argument to this->pre() and this->post().
  */
  void * state;
  /**
     The pre-callback hook. May be NULL.
  */
  cwal_callback_hook_pre_f pre;
  /**
     The post-callback hook. May be NULL.
  */
  cwal_callback_hook_post_f post;
};

/**
   An initialized-with-defaults instance of cwal_callback_hook,
   intended for const-copy intialization.
*/
#define cwal_callback_hook_empty_m {NULL,NULL,NULL}

/**
   An initialized-with-defaults instance of cwal_callback_hook,
   intended for copy intialization.
*/
extern const cwal_callback_hook cwal_callback_hook_empty;


/**
   The set of result codes used by most cwal API routines.  Not all of
   these are errors, per se, and may have context-dependent
   interpretations.

   Client code MUST NOT rely on any of these entries having a
   particular value EXCEPT for CWAL_RC_OK, which is guaranteed to
   be 0. All other entries are guaranteed to NOT be 0, but their
   exact values may change from time to time. The values are
   guaranteed to stay within a standard enum range of signed
   integer bits, but no other guarantees are made (e.g. whether
   the values are positive or negative, 2 digits or 5, are
   unspecified and may change).
*/
enum cwal_rc_e {
/**
   The canonical "not an error" code.
*/
CWAL_RC_OK = 0,
/**
   Generic "don't have anything better" error code.
*/
CWAL_RC_ERROR = 1,
/**
   Out-of-memory.
*/
CWAL_RC_OOM = 2,
/**
   Signifies that the cwal engine may be in an unspecific state
   and must not be used further.
*/
CWAL_RC_FATAL = 3,
/**
   Signals that the returning operation wants one of its callers
   to implement "continue" semantics.
*/
CWAL_RC_CONTINUE = 101,
/**
   Signals that the returning operation wants one of its callers
   to implement "break" semantics.
*/
CWAL_RC_BREAK = 102,
/**
   Signals that the returning operation wants one of its callers
   to implement "return" semantics.
*/
CWAL_RC_RETURN = 103,

/**
   Indicates that the interpreter should stop running
   the current script immediately.
*/
CWAL_RC_EXIT = 104,

/**
   Indicates that the interpreter "threw an exception", which
   "should" be reflected by passing a cwal_exception value down
   the call stack via one of the cwal_exception_set() family of
   functions. Callers "should" treat this return value as fatal,
   immediately passing it back to their callers (if possible),
   until a call is reached in the stack which handles this return
   type (e.g. the conventional "catch" handler), at which point
   the propagation should stop.
*/
CWAL_RC_EXCEPTION = 105,

/**
   Indicates that the interpreter triggered an assertion. Whether
   these are handled as outright fatal errors or exceptions (or some
   other mechanism) is up to the interpreter. It is also sometimes
   used in non-debug builds to flag cwal_engine::fatalCode when an
   assert() would have triggered in a debug build.
*/
CWAL_RC_ASSERT = 106,

/**
   Indicates that some argument value is incorrect or a precondition
   is not met.
*/
CWAL_RC_MISUSE = 201,
/**
   Indicates that a resource being searched for was not found.
*/
CWAL_RC_NOT_FOUND = 301,
/**
   Indicates that a resource being searched for or replaced already
   exists (whether or not this is an error is context-dependent).
*/
CWAL_RC_ALREADY_EXISTS = 302,
/**
   A more specific form of CWAL_RC_MISUSE, this indicates that some
   value (argument or data a routine depends on) is "out of range."
*/
CWAL_RC_RANGE = 303,
/**
   Indicates that some value is not of the required type (or family of
   types).
*/
CWAL_RC_TYPE = 304,
/**
   Indicates an unsupported/not-yet-implemented operation was
   requested.
*/
CWAL_RC_UNSUPPORTED = 305,

/**
   Indicates that access to some resource was denied.
*/
CWAL_RC_ACCESS = 306,

/**
   Indicates that an operation has failed because the value in
   question is a container which is being visited, and the operation
   in question is illegal for being-visited containers.

   Historically we have used CWAL_RC_ACCESS to note that iteration or
   modification is not allowed because it would be recursive. That
   code has proven to be ambiguous in some contexts, so this code was
   added specifically for that case.
*/
CWAL_RC_IS_VISITING = 307,

/**
   Indicates that an operation was requested which is disallowed
   because the resources is currently iterating over a list component
   which would be negatively affected. An example would be resizing an
   array while it's being sorted, or sorting twice concurrently.
*/
CWAL_RC_IS_VISITING_LIST = 308,

/**
   Indicates a CWAL_CONTAINER_DISALLOW_NEW_PROPERTIES constraint
   violation.

   A special case of CWAL_RC_ACCESS.
*/
CWAL_RC_DISALLOW_NEW_PROPERTIES = 309,

/**
   Indicates a CWAL_CONTAINER_DISALLOW_PROP_SET constraint violation.

   A special case of CWAL_RC_ACCESS.
*/
CWAL_RC_DISALLOW_PROP_SET = 310,

/**
   Indicates a CWAL_CONTAINER_DISALLOW_PROTOTYPE_SET constraint
   violation.

   A special case of CWAL_RC_ACCESS.
*/
CWAL_RC_DISALLOW_PROTOTYPE_SET = 311,

/**
   Indicates a CWAL_VAR_F_CONST constraint violation.

   A special case of CWAL_RC_ACCESS.
*/
CWAL_RC_CONST_VIOLATION = 312,

/**
   Indicates that a value is locked against the requested operation
   for some reason. e.g. an array may not be traversed while a sort()
   is underway.

   A special case of CWAL_RC_ACCESS.
*/
CWAL_RC_LOCKED = 313,

/**
   Indicates that visitation of a value would step into a cycle
   collision. Whether or not this is an error is context-dependent.
*/
CWAL_RC_CYCLES_DETECTED = 401,
/**
   Used by value cleanup to help ensure that values with cycles do not
   get freed multiple times. Returned only by cwal_value_unref() and
   friends and only when a value encounters a reference to itself
   somewhere in the destruction process. In that context this value is
   a flag, not an error, but it is also used in assert()s to ensure
   that pre- and post-conditions involving cycle traversal during
   destruction are held.
*/
CWAL_RC_DESTRUCTION_RUNNING = 402,

/**
   Returned by cwal_value_unref() when it really finalizes a value.
*/
CWAL_RC_FINALIZED = 403,

/**
   Returned by cwal_value_unref() when it really the value it is given
   still has active references after unref returns.
*/
CWAL_RC_HAS_REFERENCES = 404,

/**
   Reserved for future use.
*/
CWAL_RC_INTERRUPTED = 501,
/**
   Reserved for future use.
*/
CWAL_RC_CANCELLED = 502,
/**
   Indicates an i/o error of some sort.
*/
CWAL_RC_IO = 601,
/**
   Intended for use by routines which normally assert() a
   particular condition but do not do so when built in non-debug
   mode.
*/
CWAL_RC_CANNOT_HAPPEN = 666,

CWAL_RC_JSON_INVALID_CHAR = 700,
CWAL_RC_JSON_INVALID_KEYWORD,
CWAL_RC_JSON_INVALID_ESCAPE_SEQUENCE,
CWAL_RC_JSON_INVALID_UNICODE_SEQUENCE,
CWAL_RC_JSON_INVALID_NUMBER,
CWAL_RC_JSON_NESTING_DEPTH_REACHED,
CWAL_RC_JSON_UNBALANCED_COLLECTION,
CWAL_RC_JSON_EXPECTED_KEY,
CWAL_RC_JSON_EXPECTED_COLON,
    
/**
   The CWAL_SCR_xxx family of values are intended for use by
   concrete scripting implementations based on cwal.

   2020-02-10: in practice, the only one of the SCR values which sees
   any use is CWAL_SCR_SYNTAX. The rest are, it turns out, essentially
   special cases of conditions which are covered just as well by the
   more generic CWAL_RC_xxx values. s2 also uses CWAL_SCR_DIV_BY_ZERO,
   but that could just as easily be covered by CWAL_RC_RANGE.
*/
CWAL_SCR_readme = 2000,

/**
   Indicates that the provided token "could not be consumed" by
   the given handler, but that there is otherwise no known error.
*/
CWAL_SCR_CANNOT_CONSUME,

/**
   Indicates that an invalid operation was performed on a value, or
   that the type(s) required for a given operation are incorrect.

   More concrete case of CWAL_RC_TYPE or CWAL_RC_UNSUPPORTED.
*/
CWAL_SCR_INVALID_OP,

/**
   Special case of CWAL_RC_NOT_FOUND, indicates that an
   identifier string could not be found in the scope
   path.
*/
CWAL_SCR_UNKNOWN_IDENTIFIER,

/**
   Indicates a (failed) attempt to call() a non-Function value.
   More concrete case of CWAL_RC_TYPE.
*/
CWAL_SCR_CALL_OF_NON_FUNCTION,

/**
   More concrete case of CWAL_SCR_SYNTAX.
*/
CWAL_SCR_MISMATCHED_BRACE,

/**
   More concrete case of CWAL_SCR_SYNTAX.
*/
CWAL_SCR_MISSING_SEPARATOR,
/**
   More concrete case of CWAL_SCR_SYNTAX.
*/
CWAL_SCR_UNEXPECTED_TOKEN,

/**
   Indicates division or modulus by 0 would have been attempted.
*/
CWAL_SCR_DIV_BY_ZERO,
/**
   Indicates a generic syntax error.
*/
CWAL_SCR_SYNTAX,
/**
   Indicates that an unexpected EOF was encountered (e.g. while
   reading a string literal).
*/
CWAL_SCR_UNEXPECTED_EOF,
/**
   Indicates EOF was encountered. Whether or not this is an error
   is context-dependent, and CWAL_SCR_UNEXPECTED_EOF is intended
   for the error case.
*/
CWAL_SCR_EOF,
/**
   More concrete case of CWAL_RC_RANGE. 
*/
CWAL_SCR_TOO_MANY_ARGUMENTS,
/**
   More concrete case of CWAL_SCR_SYNTAX.
*/
CWAL_SCR_EXPECTING_IDENTIFIER,

/**
   The "legal" result code starting point for adding client-specific
   RC values for use with cwal. In other words, do not use codes with
   values lower than this one in cwal-bound client-side code, e.g. as
   return codes from cwal_callback_f() implementations, as doing so
   risks "semantic collisions" with cwal-internal/cwal-conventional
   handling of certain codes. Examples: CWAL_RC_OOM is used solely to
   propagate out-of-memory (OOM) conditions and CWAL_RC_EXCEPTION
   signifies that a script-side exception has been thrown (which
   typically requires different handling than other C-level errors).
*/
CWAL_RC_CLIENT_BEGIN = 3000
};

/** Convenience typedef. */
typedef enum cwal_rc_e cwal_rc;

/**
   Compile-time limits not covered by configuration macros.
*/
enum cwal_e_options_e {
/**
   Max number of arguments cwal_function_callf() and (variadic)
   friends. Remember that each one takes up sizeof(cwal_value*) in
   stack space.
*/
CWAL_OPT_MAX_FUNC_CALL_ARGS = 32
};


/**
   A collection of values which control what tracing messages get
   emitted by a cwal_engine.

   By an unfortunate fluke of mis-design, entries which are
   themselves not group masks (groups are named xxxx_MASK) cannot
   be effecitvely mixed together via bitmasking. The end effect is
   that only the MASK, NONE, or ALL entries can be
   usefully/predictibly applied. i'll see about fixing that.
*/
enum cwal_trace_flags_e {

CWAL_TRACE_NONE = 0,
CWAL_TRACE_GROUP_MASK = 0x7F000000,

CWAL_TRACE_MEM_MASK = 0x01000000,
CWAL_TRACE_MEM_MALLOC = CWAL_TRACE_MEM_MASK | (1 << 0),
CWAL_TRACE_MEM_REALLOC = CWAL_TRACE_MEM_MASK | (1 << 1),
CWAL_TRACE_MEM_FREE = CWAL_TRACE_MEM_MASK | (1 << 2),
CWAL_TRACE_MEM_TO_RECYCLER = CWAL_TRACE_MEM_MASK | (1 << 3),
CWAL_TRACE_MEM_FROM_RECYCLER = CWAL_TRACE_MEM_MASK | (1 << 4),
CWAL_TRACE_MEM_TO_GC_QUEUE = CWAL_TRACE_MEM_MASK | (1 << 5),

CWAL_TRACE_VALUE_MASK = 0x02000000,
CWAL_TRACE_VALUE_CREATED = CWAL_TRACE_VALUE_MASK | (1 << 0),
CWAL_TRACE_VALUE_SCOPED = CWAL_TRACE_VALUE_MASK | (1 << 1),
CWAL_TRACE_VALUE_UNSCOPED = CWAL_TRACE_VALUE_MASK | (1 << 2),
CWAL_TRACE_VALUE_CLEAN_START = CWAL_TRACE_VALUE_MASK | (1 << 3),
CWAL_TRACE_VALUE_CLEAN_END = CWAL_TRACE_VALUE_MASK | (1 << 4),
CWAL_TRACE_VALUE_CYCLE = CWAL_TRACE_VALUE_MASK | (1 << 5),
CWAL_TRACE_VALUE_INTERNED = CWAL_TRACE_VALUE_MASK | (1 << 6),
CWAL_TRACE_VALUE_UNINTERNED = CWAL_TRACE_VALUE_MASK | (1 << 7),
CWAL_TRACE_VALUE_VISIT_START = CWAL_TRACE_VALUE_MASK | (1 << 8),
CWAL_TRACE_VALUE_VISIT_END = CWAL_TRACE_VALUE_MASK | (1 << 9),
CWAL_TRACE_VALUE_REFCOUNT = CWAL_TRACE_VALUE_MASK | (1 << 10),

CWAL_TRACE_SCOPE_MASK = 0X04000000,
CWAL_TRACE_SCOPE_PUSHED = CWAL_TRACE_SCOPE_MASK | (1 << 0),
CWAL_TRACE_SCOPE_CLEAN_START = CWAL_TRACE_SCOPE_MASK | (1 << 1),
CWAL_TRACE_SCOPE_CLEAN_END = CWAL_TRACE_SCOPE_MASK | (1 << 2),
CWAL_TRACE_SCOPE_SWEEP_START = CWAL_TRACE_SCOPE_MASK | (1 << 3),
CWAL_TRACE_SCOPE_SWEEP_END = CWAL_TRACE_SCOPE_MASK | (1 << 4),

CWAL_TRACE_ENGINE_MASK =0X08000000,
CWAL_TRACE_ENGINE_STARTUP = CWAL_TRACE_ENGINE_MASK | (1 << 1),
CWAL_TRACE_ENGINE_SHUTDOWN_START = CWAL_TRACE_ENGINE_MASK | (1 << 2),
CWAL_TRACE_ENGINE_SHUTDOWN_END = CWAL_TRACE_ENGINE_MASK | (1 << 3),

CWAL_TRACE_FYI_MASK = 0x10000000,
CWAL_TRACE_MESSAGE = CWAL_TRACE_FYI_MASK | (1<<1),

CWAL_TRACE_ERROR_MASK = 0x20000000,
CWAL_TRACE_ERROR = CWAL_TRACE_ERROR_MASK | (1<<0),
    
/**
   Contains all cwal_trace_flags_e values except CWAL_TRACE_NONE.
*/
CWAL_TRACE_ALL = 0x7FFFFFFF/*1..31*/

};
/**
   Convenience typedef.
*/
typedef enum cwal_trace_flags_e cwal_trace_flags_e;

    
#if CWAL_ENABLE_TRACE
typedef struct cwal_trace_state cwal_trace_state;
/**
   State which gets passed to a cwal_engine_tracer_f() callback when
   cwal_engine tracing is enabled.
*/
struct cwal_trace_state {
  cwal_trace_flags_e event;
  int32_t mask;
  cwal_rc code_NYI;
  cwal_engine const * e;
  cwal_value const * value;
  cwal_scope const * scope;
  void const * memory;
  cwal_size_t memorySize;
  char const * msg;
  cwal_size_t msgLen;
  char const * cFile;
  char const * cFunc;
  int cLine;
};
#else
typedef char cwal_trace_state;
#endif

#if CWAL_ENABLE_TRACE
#  define cwal_trace_state_empty_m {                    \
    CWAL_TRACE_NONE/*event*/,                           \
    0/*mask*/, CWAL_RC_OK/*code*/,                    \
    0/*engine*/,0/*scope*/,0/*value*/,                \
    0/*mem*/,0/*memorySize*/,0/*msg*/,0/*msgLen*/,    \
    0/*cFile*/,0/*cFunc*/,0/*cLine*/               \
  }
#else
#  define cwal_trace_state_empty_m 0
#endif
extern const cwal_trace_state cwal_trace_state_empty;
/**
   Converts the given cwal_rc value to "some string", or returns an
   unspecified string if rc is not a cwal_rc value. The returned bytes
   are always the same for a given value, and static, and are thus
   guaranteed to survive at least until main() returns or exit() is
   called.

   If passed a non-cwal_rc value then it will delegate the call to
   one of its registered fallbacks.

   @see cwal_rc_cstr_fallback()
*/
char const * cwal_rc_cstr(int rc);

/**
   Functionally identical to cwal_rc_cstr() except that it returns 0
   if rc is not a code known by this library or one of its registered
   rc-to-string fallbacks.

   @see cwal_rc_cstr_fallback().
*/
char const * cwal_rc_cstr2(int rc);

/**
   Callback signature for a cwal_rc_cstr() fallback. This is intended
   to allow downstream API extensions to plug in their own rc codes
   into cwal_rc_cstr(), such that certain generic/high-level
   algorithms do not need to specifically know about which set of
   result codes they might be dealing with (e.g. cwal vs s2).

   Implementations must accept a result code integer. If it is one of
   that API's designated codes then the function must return a pointer
   to static/immutable memory which contains a human-readable name for
   that code (by convention the string form of its enum value), in the
   same style that cwal_rc_cstr() does. If it does not recognize the
   code then it must return NULL so that the next fallback (if any)
   can be tried.

   @see cwal_rc_cstr_fallback()
*/
typedef char const * (*cwal_rc_cstr_f)(int);

/**
   Installs a fallback handler for cwal_rc_str() and cwal_rc_cstr2(),
   such that if those routines cannot answer a request then they will
   try one of the registered fallbacks.

   CAVEATS:

   - The list of callbacks has a hard-coded maximum size and exceeding
   it will trigger an assert() (in debug builds) or an abort() (in
   non-debug). It is not expected that more than one, maybe two, such
   fallbacks will ever be useful/necessary in a given binary.

   - This function is not thread-safe. It is intended to be called
   once, maybe twice, during an app's main() and then never touched
   again.

   - Fallbacks are called in the _reverse_ order of which they are
   registered.
 */
void cwal_rc_cstr_fallback(cwal_rc_cstr_f);

/**
   Type IDs used by cwal. They correspond roughly to
   JavaScript/JSON-compatible types, plus some extensions.

   These are primarily in the public API to allow O(1) client-side
   dispatching based on cwal_value types, as opposed to using
   O(N) if/else if/else.

   Note that the integer values assigned here are not guaranteed to
   stay stable: they are intended only to assist human-level debug
   work in the cwal code.
*/
enum cwal_type_id {
/**
   GCC likes to make enums unsigned at times, which breaks
   strict comparison of integers with enums. Soooo...
*/
CWAL_TYPE_FORCE_SIGNED_ENUM = -1,
/**
   The special "undefined" value constant.

   Its value must be 0 for internal reasons.
*/
CWAL_TYPE_UNDEF = 0,
/**
   The special "null" value constant.
*/
CWAL_TYPE_NULL = 1,
/**
   The bool value type.
*/
CWAL_TYPE_BOOL = 2,
/**
   The integer value type, represented in this library
   by cwal_int_t.
*/
CWAL_TYPE_INTEGER = 3,
/**
   The double value type, represented in this library
   by cwal_double_t.
*/
CWAL_TYPE_DOUBLE = 4,
/** The immutable string type. This library stores strings
    as immutable UTF8.
*/
CWAL_TYPE_STRING = 5,
/** The "Array" type. */
CWAL_TYPE_ARRAY = 6,
/** The "Object" type. */
CWAL_TYPE_OBJECT = 7,
/** The "Function" type. */
CWAL_TYPE_FUNCTION = 8,
/** A handle to a generic "error" or "exception" type.
 */
CWAL_TYPE_EXCEPTION = 9,
/** A handle to a client-defined "native" handle. */
CWAL_TYPE_NATIVE = 10,
/**
   The "buffer" type, representing a generic memory buffer.
*/
CWAL_TYPE_BUFFER = 11,

/**
   Represents a hashtable type (cwal_hash), which is similar to
   OBJECT but guarantees a faster property store.
*/
CWAL_TYPE_HASH = 12,

/**
   A pseudo-type-id used internaly, and does not see use in the
   public API (it might at some future point).
*/
CWAL_TYPE_SCOPE = 13,

/**
   KVP (Key/Value Pair) is a pseudo-type-id used internally, and
   does not see use in the public API.
*/
CWAL_TYPE_KVP = 14,

/**
   Used _almost_ only internally. The only public API use
   for this entry is with cwal_engine_recycle_max() and friends.
*/
CWAL_TYPE_WEAK_REF = 15,
    
/**
   Used only internally during the initialization of "external
   strings." After initializations these take the type
   CWAL_TYPE_STRING. The only public API use for this entry is
   with cwal_engine_recycle_max() and friends.
*/
CWAL_TYPE_XSTRING = 16,

/**
   Used only internally during the initialization of "z-strings."
   After initializations these take the type CWAL_TYPE_STRING.
   The only public API use for this entry is with
   cwal_engine_recycle_max() and friends.
*/
CWAL_TYPE_ZSTRING = 17,

/**
   A type which is true in a boolean context but never compares
   equivalent to any other value, including true. Used as sentries or
   guaranteed-unique property keys. They optionally wrap a single
   arbitrary value, like a single-child container.
*/
CWAL_TYPE_UNIQUE = 18,

/**
   An Array variant optimized for/restricted to a fixed size
   and with different comparison semantics.
*/
CWAL_TYPE_TUPLE = 19,

/**
   Only used internally for metrics tracking of memory stored in
   cwal_list::list.
*/
CWAL_TYPE_LISTMEM = 20,

/** CWAL_TYPE_end is internally used as an iteration sentinel and MUST
    be the last entry in this enum. */
CWAL_TYPE_end

};
/**
   Convenience typedef.
*/
typedef enum cwal_type_id cwal_type_id;

/**
   Convenience typedef.
*/
typedef struct cwal_function cwal_function;

/** @struct cwal_callback_args

    A type holding arguments generated from "script" code which
    call()s a Function value. Instances of this type are created only
    by the cwal_function_call() family of functions, never by client
    code.  The populated state is, via cwal_function_call() and
    friends, passed to the cwal_callback_f() implementation which is
    wrapped by the call()'d Function.
*/
struct cwal_callback_args{
  /**
     The engine object making the call.
  */
  cwal_engine * engine;
  /**
     The scope in which the function is called.
  */
  cwal_scope * scope;
  /**
     The "this" value for this call.
  */
  cwal_value * self;
  /**
     In certain call contexts (namely interceptors), this gets set
     to the container in which the callee property was found. It may
     be self or some protototype of self, and may be of a distinctly
     different type.
  */
  cwal_value * propertyHolder;
  /**
     The function being called.
  */
  cwal_function * callee;
  /**
     State associated with the function by native client code.
     This is set via cwal_new_function() or equivalent.

     @see cwal_args_state()
     @see cwal_function_state_get()
  */
  void * state;

  /**
     A client-provided "tag" which can be used to determine if
     this->state is of the type the client expects. This allows us
     to do type-safe conversions from (void*) to (T*). This value is
     set via the cwal_new_function() family of APIs.

     In practice this value is simply the pointer to an arbitrary
     file-static data structure. e.g.:

     @code
     // value is irrelevant - we only use the pointer
     static const my_type_id = 1;
     @endcode

     Then we internally use the address of my_type_id as the
     stateTypeID when binding native data to a function instance via
     cwal_new_function().

     @see cwal_args_state()
     @see cwal_function_state_get()
  */
  void const * stateTypeID;

  /**
     Number of arguments.
  */
  uint16_t argc;

  /**
     Array of arguments argc items long.
  */
  cwal_value * const * argv;
};
#define cwal_callback_args_empty_m                          \
  {0/*engine*/,0/*scope*/,0/*self*/,0/*propertyHolder*/,    \
   0/*callee*/,0/*state*/,NULL/*stateTypeID*/,           \
   0/*argc*/,0/*argv*/                                   \
  }
extern const cwal_callback_args cwal_callback_args_empty;

/**
   Callback function interface for cwal "script" functions. args
   contains various state information related to the call.  The
   callback returns a value to the framework by assigning *rv to it
   (assigning it to NULL is equivalent to assigning it to
   cwal_value_undefined()). Implementations can rely on rv being
   non-NULL but must not rely on any previous contents of *rv. In
   practice, callbacks are passed a pointer to an initially-NULL
   value, and callback implementations will, on success, set *rv to
   the result value.

   Callbacks must return 0 on success, CWAL_RC_EXCEPTION if they set
   the cwal exception state, or (preferably) one of the other relevant
   CWAL_RC values on a genuine error. Practice strongly suggests that
   implementations should assign a new value to *rv only if they
   "succeed" (for a client-dependent definition of "succeed"), and
   "really shouldn't" assign it a new value if they "fail" (again,
   where "fail" sometimes has as client-specific meaning).

   This interface is the heart of client-side cwal bindings, and any
   non-trivial binding will likely have many functions of this type.
   
   ACHTUNG: it is critical that implementations return CWAL_RC_xxx
   values, as the framework relies on several specific values to
   report information to the framework and to scripting engines
   built on it. e.g. CWAL_RC_RETURN, CWAL_RC_OOM, CWAL_RC_BREAK,
   and CWAL_RC_EXCEPTION are often treated specially. If clients
   return non-cwal result codes from this function, cwal may get
   confused and downstream behaviour is undefined.
*/
typedef int (*cwal_callback_f)( cwal_callback_args const * args, cwal_value ** rv );

/**
   Framework-wide interface for finalizer functions for memory managed
   by/within a cwal_engine instance. Generally speaking it must
   semantically behave like free(3), but if the implementor knows what
   s/he's doing these can also be used for "cleanup" (as opposed to
   free()ing).

   The memory to free/clean up is passed as the second argument. First
   argument is the cwal_engine instance which is (at least ostensibly)
   managing that memory, and some implementations permit this to be
   NULL.

   For semantic compatibility with free(), implementations must
   accept NULL as the 2nd argument and must "do nothing" if
   passed a NULL second argument.
*/
typedef void (*cwal_finalizer_f)( cwal_engine * e, void * m );

/**
   A cwal_finalizer_f() implementation which requires that s be a
   (FILE*). If s is not NULL and not one of (stdin, stdout, stderr)
   then this routine fclose()s it, otherwise it has no side
   effects. This implementation ignores the e parameter. If s is NULL
   this is a harmless no-op. Results are undefined if s is not NULL
   and is not a valid opened (FILE*).
*/
void cwal_finalizer_f_fclose( cwal_engine * e, void * s );

/**
   Generic list type.

   It is up to the APIs using this type to manage the "count" member
   and use cwal_list_reserve() to manage the "alloced" member.
   
   @see cwal_list_reserve()
   @see cwal_list_append()
*/
struct cwal_list {
  /**
     Array of entries. It contains this->alloced
     entries, this->count of which are "valid"
     (in use).
  */
  void ** list;
  /**
     Number of "used" entries in the list.

     Reminder to self: we could reasonably use a 32-bit size type
     even in 64-bit builds, and that would save 8 bytes per array.
     It would require many changes to list-related API signatures
     which take cwal_size_t (which may be larger than 32-bits).
  */
  cwal_midsize_t count;
  /**
     Number of slots allocated in this->list. Use
     cwal_list_reserve() to modify this. Doing so
     might move the this->list pointer but the values
     it points to will stay stable.
  */
  cwal_midsize_t alloced;

  /**
     An internal consistency/misuse marker to let us know that this
     list is currently undergoing iteration/visitation and must
     therefore not be modified. Do not use this from client-level
     code.
  */
  bool isVisiting;
};
typedef struct cwal_list cwal_list;
/**
   Empty-initialized cwal_list object.
*/
#define cwal_list_empty_m { NULL, 0, 0, false }
/**
   Empty-initialized cwal_list object.
*/
extern const cwal_list cwal_list_empty;

/**
   A helper class for holding arbitrary state with an optional
   associated finalizer. The interpretation of the state and the
   finalizer's requirements are context-specific.
*/
struct cwal_state {
  /**
     The raw data. Its interpretation is of course very
     context-specific. The typeID field can be used to "tag"
     this value with type info so that clients can ensure that
     they do not mis-cast this pointer.
  */
  void * data;
  /**
     An arbitrary "tag" value which clients can use to indicate
     that this->data is of a specific type. In practice this is
     normally set to the address of some internal structure or
     value which is not exposed via public APIs.
  */
  void const * typeID;
  /**
     Cleanup function for this->data. It may be NULL if
     this->data has no cleanup requirements or is owned by
     someone else.
  */
  cwal_finalizer_f finalize;
};
/** Convenience typedef. */
typedef struct cwal_state cwal_state;
/**
   Empty-initialized cwal_state object.
*/
#define cwal_state_empty_m { NULL, NULL, NULL }
/**
   Empty-initialized cwal_state object.
*/
extern const cwal_state cwal_state_empty;

/**
   A generic output interface intended (primarily) to be used via
   cwal_engine_vtab via cwal_output(). Script-side code which
   generates "console-style" output intended for the user should use
   cwal_output() to put it there.  An implementation of this interface
   is then responsible for sending the output somewhere.

   Must return 0 on success or an error code (preferably from cwal_rc)
   on error. Because an output mechanism can modify the output, there
   is not direct 1-to-1 mapping of input and output lengths, and thus
   it returns neither of those.

   The state parameter's meaning is implementation-specific. e.g.
   cwal_output_f_FILE() requires it to be an opened-for-writing
   (FILE*). It is up to the caller to provide a state value which is
   appropriate for the given cwal_output_f() implementation.
*/
typedef int (*cwal_output_f)( void * state, void const * src, cwal_size_t n );

/**
   Library-wide interface for allocating, reallocating, freeing memory.
   It must semantically behave like realloc(3) with the minor clarification
   that the free() operation (size==0) it must return NULL instead of
   "some value suitable for passing to free()."

   The state argument (typically) comes from the state member of the
   cwal_engine_vtab which holds one of these functions. The (mem,size)
   parameters are as for realloc(3). In summary:

   If (mem==NULL) then it must semantically behave like malloc(3).

   If (size==0) then it must sematically behave like free(3).
   
   If (mem!=NULL) and (size!=0) then it must semantically behave like
   realloc(3).
*/
typedef void * (*cwal_engine_realloc_f)( void * state, void * mem, cwal_size_t size );

/**
   A callback for use in low-level tracing of cwal-internal
   activity. It is only used when tracing is enabled via the
   CWAL_ENABLE_TRACE configuration macro.

   2020-02-10: the cwal_engine tracing features are extremely
   low-level and produce absolute tons of output. They are intended
   solely to assist cwal's developer in debugging (in particular
   during early development of the library). It has, as of this
   writing, been 5+ years since tracing has been enabled in a cwal
   build, and there are no guarantees that the traced data have been
   kept entirely relevant vis-a-vis changes in the engine since then.
   i.e. do not use tracing unless you are trying to decode the cwal
   internals.
*/
typedef void (*cwal_engine_tracer_f)( void * state, cwal_trace_state const * event );
/**
   The combined state for a cwal_engine tracer. It is only used when
   tracing is enabled via the CWAL_ENABLE_TRACE configuration macro.
*/
struct cwal_engine_tracer{
  /** Callback for the tracer. */
  cwal_engine_tracer_f trace;
  /** A finalizer for this->state. If not NULL, it gets called
      during finalization of its associated cwal_engine instance. */
  void (*close)( void * state );
  /** Optional client-specific state for tracing. Gets passed,
      without interpretation, to the callback. */
  void * state;
};
typedef struct cwal_engine_tracer cwal_engine_tracer;
#define cwal_engine_tracer_empty_m { 0, 0, 0 }
extern const cwal_engine_tracer cwal_engine_tracer_empty;
extern const cwal_engine_tracer cwal_engine_tracer_FILE;
void cwal_engine_tracer_f_FILE( void * filePtr, cwal_trace_state const * event );
void cwal_engine_tracer_close_FILE( void * filePtr );

/**
   Part of the cwal_engine_vtab interface, this
   defines the API for a memory allocator used
   by the cwal_engine API.
*/
struct cwal_allocator{
  /**
     The memory management function. cwal_engine
     uses this exclusively for all de/re/allocations.
  */
  cwal_engine_realloc_f realloc;
  /**
     State for the allocator. Its requirements/interpretation
     depend on the concrete realloc implementation.
  */
  cwal_state state;
};
/** Convenience typedef. */
typedef struct cwal_allocator cwal_allocator;
/** Empty-initialized cwal_allocator object. */
#define cwal_allocator_empty_m { 0, cwal_state_empty_m }
/** Empty-initialized cwal_allocator object. */
extern const cwal_allocator cwal_allocator_empty;
/** cwal_allocator object configured to use realloc(3). */
extern const cwal_allocator cwal_allocator_std;
    
/**
   Part of the cwal_engine_vtab interface, this defines a generic
   interface for outputing "stuff" (presumably script-generated
   text). The intention is that script-side output should all go
   through a common channel, to provide the client an easy to to
   intercept/redirect it, or to add layers like output buffer
   stacks (this particular output interface originates from such
   an implementation in TH1).
*/
struct cwal_outputer{
  cwal_output_f output;
  /**
     Intended to flush the output channel, if needed.  If not
     needed, this member may be NULL, in which case it is
     ignored, or it may simply return 0.

     It is passed this.state.data as its argument.
  */
  int (*flush)( void * state );
  cwal_state state;
};
typedef struct cwal_outputer cwal_outputer;
/** Empty-initialized cwal_outputer object. */
#define cwal_outputer_empty_m { 0, NULL, cwal_state_empty_m }
/** Empty-initialized cwal_outputer object. */
extern const cwal_outputer cwal_outputer_empty;
/**
   cwal_outputer object set up to use cwal_output_f_FILE. After
   copying this value, set it the copy's state.data to a (FILE*) to
   redirect it. If its file handle needs to be closed during cleanup,
   the state.finalize member should be set to a function which will
   close the file handle (e.g. cwal_finalizer_f_fclose()).
*/
extern const cwal_outputer cwal_outputer_FILE;

/**
   Typedef for a predicate function which tells a cwal_engine
   whether or not a given string is "internable" or not. Clients
   may provide an implementation of this via
   cwal_engine_vtab::internable. If interning is enabled, when a new
   string is created, this function will be called and passed:

   - The state pointer set in cwal_engine_vtab::internable::state.

   - The string which is about to be created as a cwal_string.

   - The length of that string.

   This function is only called for non-empty strings. Thus len is
   always greater than 0, str is never NULL, and never starts with
   a NUL byte. Client implementations need not concern themselves
   with NULL str or a len of 0.

   Once a given series of bytes have been interned, this function
   will not be called again for that same series of bytes as long
   as there is at least one live interned reference to an
   equivalent string.
*/
typedef bool (*cwal_cstr_internable_predicate_f)( void * state, char const * str, cwal_size_t len );

/**
   The default "is this string internable?" predicate. The state parameter is ignored.

   The default impl uses only a basic length cutoff point to
   determine "internalizableness."

   @see cwal_cstr_internable_predicate_f()
*/
bool cwal_cstr_internable_predicate_f_default( void * state, char const * str, cwal_size_t len );

/**
   Configuration used to optionally cap the memory allocations made
   via a cwal_engine instance (i.e. via cwal_malloc() and
   cwal_realloc()). In client apps, these are set up via the
   cwal_engine_vtab::memcap member of the vtab used to initialize a
   cwal_engine.

   It is important that these config values not be modified after
   calling cwal_engine_init(), or undefined behaviour will ensue.

   Some memory-capping features require knowing exactly how much
   memory the vtab's realloc function has doles out and freed over
   time, and the only way to do that is to over-allocate all
   allocations and store their sizes in that memory. Over-allocation
   is only enabled when an option which requires it is enabled.

   The overhead imposed by over-allocation _is_ counted against any
   byte totals configured via this type. It also, when using
   CWAL_SIZE_T_BITS=64, restricts allocation sizes to a 32-bit range
   (as an optimization/compensation for 32-bit builds).


   Client-reported memory totals (via cwal_engine_adjust_client_mem())
   are not counted by this mechanism. In practice, client-reported
   memory is allocated via cwal_malloc() or cwal_realloc(), and those
   track/honor the configured caps.
*/
struct cwal_memcap_config {
  /**
     Caps the cumulative total memory allocated by the engine.

     Rejects (re)allocations which would take the absolute allocated
     byte total (including the memcap tracking overhead) over this
     value. Such a condition is not recoverable, as the total only
     increases (never decreases) over time.

     Set to 0 to disable.

     Requires (forces enabling of) over-allocation so that
     re-allocations can be counted consistently.
  */
  uint64_t maxTotalMem;

  /**
     Caps the cumulative total number of memory allocations made by
     the engine.

     Rejects _new_ allocations which would take the current total
     allocation over this value. This condition is unrecoverable, as
     the total only increases (never decreases) over time.

     Set to 0 to disable.
  */
  uint64_t maxTotalAllocCount;

  /**
     Caps the concurrent total memory usage.

     Rejects (re)allocations which would take the current byte total
     (including the memcap tracking overhead) over this value.

     Set to 0 to disable.

     Requires (forces enabling of) over-allocation.
  */
  cwal_size_t maxConcurrentMem;

  /**
     Caps the concurrent total memory allocation count.

     Rejects _new_ allocations (not reallocs) which would take
     the current total allocation count over this value.

     Set to 0 to disable.
  */
  cwal_size_t maxConcurrentAllocCount;

  /**
     Caps the size of any single allocation.

     If a single allocation request is larger than this, the
     allocator signals an OOM error instead of allocating.

     Set to 0 to disable.

     Design note: this is explicitly uint32_t, not cwal_size_t, so
     that 32-bit builds with CWAL_SIZE_T_BITS=64 do not need to
     over-allocate by 8 bytes.
  */
  uint32_t maxSingleAllocSize;

  /**
     If true (non-0) then during cwal_engine_init() memory
     allocation size tracking (and its required over-allocation) is
     enabled regardless of which other limits are enabled.

     When over-allocation is enabled, apps can generally expect a
     small reduction in malloc counts and a slight increase in
     peak/total memory usage from scripts.

     When recycling is _disabled_, enabling this option can be
     notably more memory-costly.
  */
  char forceAllocSizeTracking;
};
typedef struct cwal_memcap_config cwal_memcap_config;
/**
   Initialized-with-defaults cwal_memcap_config instance, intended for
   const-copy initialization.
*/
#define cwal_memcap_config_empty_m {                            \
    0/*maxTotalMem*/, 0/*maxTotalAllocCount*/,                  \
    0/*maxConcurrentMem*/, 0/*maxConcurrentAllocCount*/,      \
    /*maxSingleAllocSize*/((CWAL_SIZE_T_BITS==16)             \
                           ? 0x7FFF/*32k*/                    \
                           : (cwal_size_t)(1 << 24/*16MB*/)), \
    0/*forceAllocSizeTracking*/                               \
  }
/**
   Cleanly initialized cwal_memcap_config instance intended for
   non-const copy initialization.
*/
extern const cwal_memcap_config cwal_memcap_config_empty;

/**
   A piece of the infrastructure for hooking into cwal_scope
   push/pop operations performed on a cwal_engine.

   If cwal_engine_vtab::hook::scope_push is not NULL, it gets called
   during cwal_scope_push(), _after_ cwal has set up the scope as its
   current scope. Thus, during this callback, cwal_scope_current_get()
   will return the being-pushed scope. If this function returns non-0,
   pushing of the scope will fail, cwal will immediately pop the scope
   _without_ calling the scope_pop hook, and the result of this
   callback will be returned to the caller of cwal_scope_push(). If
   this hook succeeds (returns 0) then cwal will call the
   cwal_engine_vtab::hook::scope_pop callback when the scope is
   popped.

   This hook gets passed the just-pushed scope and any state set in
   cwal_engine_vtab::hook::scope_state.

   It is strictly illegal for this routine to call cwal_scope_push(),
   cwal_scope_pop(), or any other routine which modifies the scope
   stack.

   One interesting (but solvable) problem: cwal necessarily pushes a
   scope during initialization, before the init hook can be triggered
   and before the engine can be used with cwal_malloc(). That is
   likely to pose a problem for client code. This means that the
   client may, depending on where this hook get connected, get one
   more pop call than push calls. Any memory the client wants to
   allocate before cwal_engine_init() can set up the engine must be
   done directly via the engine's vtab::allocator member, rather than
   via cwal_malloc(), because cwal_malloc()'s behaviour is undefined
   before the engine has been initialized. In the case of s2 the
   problem more or less resolves itself: when s2 starts up it pops
   cwal's installed top scope so that it can push its own scope (it's
   always done this, even before this change). The timing of that
   allows for popping all of the scopes, setting up the push/pop
   hooks, and then pushing its own top-most scope, which then goes
   through the push hook.

   Added 20181123.

   @see cwal_scope_hook_pop_f()
   @see cwal_engine_vtab
*/
typedef int (*cwal_scope_hook_push_f)( cwal_scope * s, void * clientState );

/** 
    If cwal_engine_vtab::hook::scope_pop is not NULL, it gets called
    during cwal_scope_pop(), _before_ cwal has removed the scope from
    the stack.

    This hook gets passed the being-popped scope and any state set in
    cwal_engine_vtab::hook::scope_state.

    It is strictly illegal for this routine to call cwal_scope_push(),
    cwal_scope_pop(), or any other routine which modifies the scope
    stack.

    Reminder to self: should we pass any being-propagated result value
    (via cwal_scope_pop2()), if any, to this routine? i don't think we
    need to, but that's something to keep in mind as a possibility.

    Added 20181123.

    @see cwal_scope_hook_push_f()
    @see cwal_engine_vtab
*/
typedef void (*cwal_scope_hook_pop_f)( cwal_scope const *, void * clientState );

/**
   The "virtual table" of cwal_engine instances, providing the
   functionality which clients can override with their own
   implementations.

   Multiple cwal_engine instances may share if vtab instance if
   and only if:

   - The state member (if used) may legally share the same values
   across engines AND state::finalize() does not destroy
   state::state (if it does, each engine will try to clean it up).

   - The vtab does not make values from one engine visible to
   another. This will lead to Undefined Behaviour.

   - The app is single-threaded OR...

   - all access to cwal_engine_vtab is otherwise serialized via
   a client-side mutex OR...
       
   - the vtab instance is otherwise "immune" the threading effects
   (e.g. because its underlying APIs do the locking).

   Mutex locking is not a feature planned for the cwal API.
*/
struct cwal_engine_vtab {
  /*

    Potential TODOs:

    void (*shutdown)( cwal_engine_vtab * self );

    shutdown() would be called when an engine is cleaned up (after
    it has finished cleaning up), instead of state.finalize(), and
    would be responsible for cleaning up allocator.state and
    outputer.state, if needed.
  */

  /**
     The memory allocator. All memory allocated in the context
     of a given cwal_engine is (re)allocated/freed through it's
     vtab's allocator member.
  */
  cwal_allocator allocator;

  /**
     The handler which receives all data passed to
     cwal_output().
  */
  cwal_outputer outputer;

  /**
     Handles cwal_engine tracing events (if tracing is enabled).
  */
  cwal_engine_tracer tracer;

  /**
     A place to store client-defined state. The engine places
     no value on this, other than to (optionally) clean it up
     when the engine is finalized.

     The finalize() method in the state member is called when an
     engine using this object shuts down. Because it happens
     right after the engine is destroyed, it is passed a NULL
     engine argument. Thus it is called like:

     vtab->state.finalize( NULL, vtab->state.data );

     This of course means that a single vtab cannot differentiate
     between multiple engines for the shutdown phase, and we
     might have to add reference counting to the vtab in order to
     account for this (currently it would need to be somewhere in
     state.data).
  */
  cwal_state state;

  /**
     A place to add client-side hooks into engine
     post-initialization, and possibly for other events at some
     point (if we can find a use for it).
  */
  struct {
    /**
       May be used to add post-init code to cwal_engine
       instances. If this member is not 0 then it is called
       right after cwal_engine_init() is finished, before it
       returns. If this function returns non-0 then
       initialization fails, the engine is cleaned up, and the
       return value is passed back to the caller of
       cwal_engine_init().

       Note that the vtab parameter is guaranteed to be the
       vtab which initialized e. e->vtab==vtab is guaranteed
       to be true, but client code "should really" use the
       passed-in vtab pointer instead of relying on the
       private/internal e->vtab member (its name/placement may
       change).

       This is only called one time per initialization of an engine,
       so the client may (if needed) clean up the init_state member
       (i.e. vtab->hook->init_state).
    */
    int (*on_init)( cwal_engine * e, cwal_engine_vtab * vtab );
    /**
       Arbitrary state passed to on_init().
    */
    void * init_state;

    /**
       Gets triggered when cwal_scope_push() and friends are
       called. This callback may be NULL. Clients which need to
       keep their own scope-level state in sync with cwal's scope
       levels should create a pair of scope push/pop hook routines
       to manage that state.

       See cwal_scope_hook_push_f() for the docs.
    */
    cwal_scope_hook_push_f scope_push;

    /**
       Gets triggered when cwal_scope_pop() and friends
       are called.

       See cwal_scope_hook_pop_f() for the docs.
    */
    cwal_scope_hook_pop_f scope_pop;

    /**
       scope_state is passed as-is to scope_push() and
       scope_pop() every time they are called.

       The cwal engine does not manage or own this state. It is up
       to the client to manage it, if needed. In almost(?) every
       conceivable case in which this state is used, the client
       will have to (because of allocator availability) set it
       AFTER cwal has initialized, which means after cwal has
       pushed its first scope.
    */
    void * scope_state;
  } hook;

  /**
     Holds state for determining whether a given string is
     internable or not. It is not expected that such state will need
     to be finalized separately, thus this member holds no finalizer
     function.
  */
  struct {
    /**
       The is-internable predicate. If NULL, interning is
       disabled regardless of any other considerations
       (e.g. the CWAL_FEATURE_INTERN_STRINGS flag).

       @see cwal_cstr_internable_predicate_f()
    */
    cwal_cstr_internable_predicate_f is_internable;
    /**
       State to be passed as the first argument to is_internable().
    */
    void * state;
  } interning;

  /**
     Memory capping configuration.
  */
  cwal_memcap_config memcap;
};
/**
   Empty-initialized cwal_engine_vtab object.
*/
#define cwal_engine_vtab_empty_m {                                      \
  cwal_allocator_empty_m,                                             \
  cwal_outputer_empty_m,                                            \
  cwal_engine_tracer_empty_m,                                       \
  cwal_state_empty_m,                                               \
  {/*hook*/                                                           \
    NULL/*on_init()*/,0/*init_state*/,                                \
    NULL/*scope_push*/,NULL/*scope_pop*/,                           \
    NULL/*scope_state*/},                                           \
  {/*interning*/ cwal_cstr_internable_predicate_f_default, NULL}, \
  cwal_memcap_config_empty_m \
}

/**
   Empty-initialized cwal_engine_vtab object.
*/
extern const cwal_engine_vtab cwal_engine_vtab_empty;

/**
   A cwal_realloc_f() implementation which uses the standard C
   memory allocators.
*/
void * cwal_realloc_f_std( void * state, void * m, cwal_size_t n );

/**
   A cwal_output_f() implementation which requires state to be
   a valid (FILE*) opened in write mode. It sends all output
   to that file and returns n on success. If state is NULL then
   this is a harmless no-op.
*/
int cwal_output_f_FILE( void * state, void const * src, cwal_size_t n );

/**
   A cwal_output_f() implementation which requires that state be a valid
   (cwal_engine*). This outputer passes all output to cwal_output() using
   the given cwal_engine instance.
*/
int cwal_output_f_cwal_engine( void * state, void const * src, cwal_size_t n );

/**
   A cwal_outputer::flush() implementation whichr equires that f
   be a (FILE*). For symmetry with cwal_output_f_FILE, if !f then
   stdout is assumed.
*/
int cwal_output_flush_f_FILE( void * f );

/**
   A state type for use with cwal_output_f_buffer().

   @see cwal_output_f_buffer()
*/
struct cwal_output_buffer_state {
  cwal_engine * e;
  cwal_buffer * b;
};

/**
   Convenience typedef.
*/
typedef struct cwal_output_buffer_state cwal_output_buffer_state;

/**
   Empty-initialized cwal_output_buffer_state instance.
*/
extern const cwal_output_buffer_state cwal_output_buffer_state_empty;

/**
   A cwal_output_f() implementation which requires state to be a
   valid (cwal_output_buffer_state*), that state->e points to a
   valid (cwal_engine*), and that state->b points to a valid
   (cwal_buffer*). It sends all output to state->b, expanding the
   buffer as necessary, and returns 0 on success.  Results are
   undefined if state is not a cwal_output_buffer_state.
*/
int cwal_output_f_buffer( void * state, void const * src, cwal_size_t n );

/**
   A cwal_output_f() implementation which requires state to be a
   (cwal_outputer*). This routine simply redirects all (src,n) input
   to the cwal_outputer::output() method. Results are undefined if
   state is not a cwal_outputer.
*/
int cwal_output_f_cwal_outputer( void * state, void const * src, cwal_size_t n );

/**
   A cwal_finalizer_f() which requires that m be a
   (cwal_output_buffer_state*). This function calls
   cwal_buffer_reserve(e, state->buffer, 0) to free up the
   buffer's memory, then zeroes out state's contents.

   In theory this can be used together with cwal_output_f_buffer()
   and cwal_outputer to provide buffering of all
   cwal_output()-generated output, but there's a chicken-egg
   scenario there, in that the outputer "should" be set up before
   the engine is intialized. In this case it has to be modified
   after the engine is intialized because the engine is part of
   the outputer's state.
*/
void cwal_output_buffer_finalizer( cwal_engine * e, void * bufState );

/**
   A cwal_engine_vtab instance which can be bitwise copied to inialize
   a "basic" vtab instance for use with cwal_engine_init(). It uses
   cwal_allocator_std for its memory, cwal_outputer_FILE for output,
   and cwal_finalizer_f_fclose() as its output file finalizer.
   To enable output, the client must simply assign an
   opened (FILE*) handle to the vtab's outputer.state.state.
   To remove the finalizer and take over responsibility
   for closing the stream, set outputer.state.finalize
   to 0.
*/
extern const cwal_engine_vtab cwal_engine_vtab_basic;

/**
   Allocates n bytes of memory in the context of e.

   The returned memory is "associated with" (but strictly owned by) e
   and is owned (or shared with) the caller, who must eventually pass
   it to cwal_free() or cwal_realloc(), passing the same engine
   instance as used for the allocation.

   It is NEVER legal to share malloc/free/realloc memory across
   engine instances, even if they use the same allocator, because
   doing so can lead to "missing" entries in one engine or the
   other and mis-traversal of Value graphs.

   It is NEVER legal to call this before the given engine
   has been initialized via cwal_engine_init().
*/
void * cwal_malloc( cwal_engine * e, cwal_size_t n );

/**
   Works similarly to cwal_malloc(), but first tries to pull a memory
   chunk from the chunk recycler, looking for a chunk of size n or
   some "reasonable" factor larger (unspecified, but less than
   2*n). If it cannot find one, it returns the result of passing (e,n)
   to cwal_malloc().

   When clients are done with the memory, they "should" pass it to
   cwal_free2(), passing it the same value for n (which may recycle
   it), but they "may" alternately pass it to cwal_free() (which will
   not recycle it). They "must" do one or the other.

   It is NEVER legal to call this before the given engine
   has been initialized via cwal_engine_init().

   Minor caveat: when recycling a larger chunk, the client doesn't
   know how much larger than n it is. Whether or not cwal really knows
   that depends on whether over-allocating memory capping is enabled
   or not. If so, it is able to notice the difference when the memory
   is passed to cwal_free2() and will recover those "slack" bytes. If
   not, those slack bytes will be "lost" if/when they land back in the
   chunk recycler, in that the recycler will not know about the slack
   bytes at the end. The effect _could_ be, depending on usage, that
   such a block gets smaller on each recycling trip, but in practice
   that has never been witnessed to be a problem (and even if it
   was/is, _any_ re-use of the memory is a win, so it's not "really" a
   problem).
*/
void * cwal_malloc2( cwal_engine * e, cwal_size_t n );

/**
   Frees memory allocated via cwal_malloc() or cwal_realloc().

   e MUST be a valid, initialized cwal_engine instance.

   If !m then this is a no-op.

   @see cwal_free2().
*/
void cwal_free( cwal_engine * e, void * m );

/**
   This alternate form of cwal_free() will put mem in the recycling
   bin, if possible, else free it immediately. sizeOfMem must be the
   size of the memory block. If mem or sizeOfMem it is 0 then this
   function behaves like cwal_free()).

   Recycled memory goes into a pool used internally for various forms
   of allocations, e.g. buffers and arrays.
*/
void cwal_free2( cwal_engine * e, void * mem, cwal_size_t sizeOfMem );

/**
   Works as described for cwal_realloc_f(). See cwal_malloc() for
   important notes.
*/
void * cwal_realloc( cwal_engine * e, void * m, cwal_size_t n );

/** Convenience typedef. */
typedef struct cwal_exception_info cwal_exception_info;

/**
   NOT YET USED.
       
   Holds error state information for a cwal_engine
   instance.
*/
struct cwal_exception_info {
  /**
     Current error code.
  */
  cwal_rc code;
  /**
     Length (in bytes) of cMsg.
  */
  cwal_size_t msgLen;
  /**
     Pointer to string memory not owned by the engine but which
     must be guaranteed to live "long enough."  If zMsg is set
     then this must point to zMsg's.  This is primarily a malloc
     optimization, to allow us to point to strings we know are
     static without having to strdup() them or risk accidentally
     free()ing them.
  */
  char const * cMsg;
  /**
     Dynamically-allocated memory which is owned by the containing
     engine and might be freed/invalidated on the next call
     into the engine API.
  */
  char * zMsg;
  /**
     Error value associated with the error. This would
     presumably be some sort of language-specific error type,
     or maybe a cwal_string form of cMsg.
  */
  cwal_value * value;

  /* TODO?: stack trace info, if tracing is on. */
};

/**
   Empty-initialized cwal_exception_info object.
*/
#define cwal_exception_info_empty_m {           \
    CWAL_RC_OK /*code*/,                        \
    0U /*msgLen*/,                            \
    0 /*cMsg*/,                               \
    0 /*zMsg*/,                               \
    0 /*value*/                               \
  }
/**
   Empty-initialized cwal_exception_info object.
*/
extern const cwal_exception_info cwal_exception_info_empty;


/** @internal

    Internal part of the cwal_ptr_table construct.  Each
    cwal_ptr_table is made up of 0 or more cwal_ptr_page
    instances. Each slot in a page is analog to a hash code, and
    hash code collisions are resolved by creating a new page (as
    opposed to linking the individual colliding items into a list
    as a hashtable would do).
*/
struct cwal_ptr_page {
  /** List of pointers, with a length specified by the containing
      cwal_ptr_table::hashSize.
  */
  void ** list;
  /**
     Number of live entries in this page.
  */
  uint16_t entryCount;
  /**
     Link to the next entry in a linked list.
  */
  struct cwal_ptr_page * next;
};
/** Convenience typedef. */
typedef struct cwal_ptr_page cwal_ptr_page;

/** @internal

    A "key-only" hashtable, the intention being, being able to
    quickly answer the question "do we know about this pointer
    already?" It is used for tracking interned strings and weak
    references. It was originally conceived to help track cycles
    during traversal, but it is not used for that purpose.
*/
struct cwal_ptr_table{
  /**
     The number of (void*) entries in each page.
  */
  uint16_t hashSize;
  /**
     A "span" value for our strange hash function. Ideally this
     value should be the least common sizeof() shared by all values
     in the table, and it degrades somewhat when using mixed-size
     values (which most cwal_values actually are, internally, as a
     side-effect of malloc() reduction optimizations).  For tables
     where the sizeof() is the same for all members this type should
     provide near-ideal access speed and a fair memory cost if
     hashSize can be predicted (which it most likely cannot).
  */
  uint16_t step;
  /**
     Where we keep track of pages in the table.
  */
  struct {
    /**
       First page in the list.
    */
    cwal_ptr_page * head;
    /**
       Last page in the list. We keep this pointer only to
       speed up a small handful of operations.
    */
    cwal_ptr_page * tail;
  } pg;
  /**
     Internal allocation marker.
  */
  void const * allocStamp;
};
typedef struct cwal_ptr_table cwal_ptr_table;
/**
   Empty-initialized cwal_ptr_table, for use in in-struct
   initialization.
*/
#define cwal_ptr_table_empty_m {                \
    0/*hashSize*/,                              \
    0/*step*/,                                \
    {/*pg*/ NULL/*head*/, NULL/*tail*/},        \
    NULL/*allocStamp*/                        \
  }
/**
   Empty-initialized cwal_ptr_table, for use in copy
   initialization.
*/
extern const cwal_ptr_table cwal_ptr_table_empty;

/**
   Holds the state for a cwal scope. Scopes provide one layer of
   the cwal memory model, and are modeled more or less off of
   their C++ counterparts.

   All allocation of new values in a cwal_engine context happen
   within an active scope, and the engine tracks a stack of scopes
   which behave more or less as scopes do in C++. When a scope is
   popped from the stack it is cleaned up and unreferences any
   values it currently owns (those allocated by it and not since
   taken over by another scope). Unreferencing might or might not
   destroy the values, depending on factors such as reference
   counts from cycles in the value graph or from other scopes. If
   values remain after cleaning up, it cleans up again and again
   until all values are gone (this is how it resolves cycles).

   When values are manipulated the engine (tries to) keep(s) them
   in the lowest-level (oldest) scope from which they are ever
   referenced. This ensures that the values can survive
   destruction of their originating scope, while also ensuring
   that a destructing scope can clean up values which have _not_
   been taken over by another scope. This "can" (under specific
   usage patterns) potentially lead to some values being
   "orphaned" in a lower-level scope for an undue amount of time
   (unused but still owned by the scope), and the
   cwal_scope_sweep() API is intended to help alleviate that
   problem.
*/
struct cwal_scope {
  /**
     The engine which created and manages this scope.
  */
  cwal_engine * e;

  /**
     Parent scope. All scopes except the top-most have a parent.
  */
  cwal_scope * parent;

  /**
     Stores this object's key/value properties (its local
     variables). It may be an Object (cwal_object) or Hash (cwal_hash)
     Value. Which one gets created depends on the combination of
     compile-time CWAL_OBASE_ISA_HASH setting and the runtime
     cwal_engine-level CWAL_FEATURE_SCOPE_STORAGE_HASH flag.
  */
  cwal_value * props;

  /**
     Internal memory allocation stamp. Client code must never touch
     this.
  */
  void const * allocStamp;

  /**
     Values allocated while this scope is the top of the stack, or
     rescoped here after allocation, are all placed here and unref'd
     when the scope is cleaned up. We split it into multiple lists
     to simplify and improve the performance of certain operations
     (while slightly complicating others ;).

     Client code MUST NOT touch any of the fields in this
     sub-struct.  They are intricate bits of the internal memory
     management and will break if they are modified by client code.

     TODO(?): refactor this into an array of lists, like
     cwal_engine::recycler. We can then refine it easily by adding
     extra lists for specific types or groups of types.
  */
  struct {
    /**
       Head of the "PODs" (Plain old Data) list. This includes
       all non-containers.
    */
    cwal_value * headPod;
    /**
       Head of the "Objects" list. This includes all container
       types. (This distinction largely has to do with cycles.)
    */
    cwal_value * headObj;

    /**
       Holds items which just came into being and have a refcount
       of 0, or have been placed back into a probationary state
       with refcount 0. This potentially gives us a
       faster/safer/easier sweep() operation.
    */
    cwal_value * r0;

    /**
       Values marked with the flag CWAL_F_IS_VACUUM_SAFE are
       managed in this list and treated basically like named vars
       for purposes of vacuuming. The intention is to provide a
       place where clients can put non-script-visible values which
       are safe from sweep/vacuum operations, but otherwise have
       normal lifetimes. Making a value vacuum-proof does not make
       it sweep-proof.
    */
    cwal_value * headSafe;
  } mine;

  /**
     The depth level this scope was created at. This is used in
     figuring out whether a value needs to be migrated to a
     lower-numbered (a.k.a. "higher") scope for memory
     management reasons.

     Scope numbers start at 1, with 0 being reserved for
     "invalid scope.".
  */
  cwal_size_t level;

  /**
     Internal flags.
  */
  uint32_t flags;
};
/**
   Empty-initialized cwal_scope object.
*/
#define cwal_scope_empty_m {                                        \
    NULL/*engine*/,                                                 \
    NULL/*parent*/,                                               \
    NULL/*props*/,                                                \
    NULL/*allocStamp*/,                                           \
    {/*mine*/ 0/*headPod*/,0/*headObj*/,0/*r0*/, 0/*headSafe*/},    \
    0U/*level*/,                                                  \
    0U/*flags*/                                                 \
  }

/**
   Empty-initialized cwal_scope_api object.
*/
extern const cwal_scope cwal_scope_empty;

/**
   Used to store "recyclable memory" - that which has been finalized
   but not yet free()d. Each instance is responsible for holding
   memory of a single type. Most instances manage Value (cwal_value)
   memory, but specialized instances handle recycling of other types
   (cwal_kvp and String-type values, as those need special handling
   due to their allocation mechanism (which needs to be reconsidered
   for refactoring)).
*/
struct cwal_recycler {
  /**
     Client-interpreted "ID" for this instance. It is used
     internally for sanity checking.
  */
  int id;
  /**
     Current length of this->list.
  */
  cwal_size_t count;
  /**
     Preferred maximum length for this list. Algorithms which
     insert in this->list should honor this value and reject
     insertion if it would be exceeded.
  */
  cwal_size_t maxLength;
  /**
     Underlying list. The exact type of entry is
     context-dependent (e.g. cwal_value or cwal_kvp pointers).
  */
  void * list;
  /**
     Each time a request is made to fetch a recycled value and we
     have one to serve the request, this counter gets incremented.
  */
  cwal_size_t hits;
  /**
     Each time a request is made to fetch a recycled value and we do
     not have one to serve the request, this counter gets
     incremented.

     Note this count includes requests which cannot possibly
     succeed, e.g. the initial allocation of any Value.  In the
     general case (barring allocation errors or falling back to the
     chunk recycler), this number will correspond directly to the
     number of allocations made for the type(s) stored in this
     recycler.
  */
  cwal_size_t misses;
};
/**  Convenience typedef. */
typedef struct cwal_recycler cwal_recycler;
/** Default-initialized cwal_recycler object. */

#define cwal_recycler_empty_m {-1/*id*/, 0U/*count*/, 128U/*maxLength*/,NULL/*list*/,0/*hits*/,0/*misses*/}

/** Default-initialized cwal_recycler object. */
extern const cwal_recycler cwal_recycler_empty;
/**
   Configurable bits for cwal_memchunk.
   
   @see cwal_engine_memchunk_config()
*/
struct cwal_memchunk_config {
  /**
     Maximum number of entries to allow (0 means disable). This is
     also the initial capacity of this->pool, so DO NOT set it to
     something obscenely huge. Some related algos are linear, so do
     not set it to something unreasonably large.

     MUST currently be set up before the recycler is used (it is
     allocated the first time something tries to store memory in
     it), as the related at-runtime resizing code is untested.
  */
  cwal_size_t maxChunkCount;

  /**
     The largest single chunk size to recycle (those larger than
     this will be freed immediately instead of recycled). Set to
     (cwal_size_t)-1 for (effectively) no limit or 0 to disable. If
     if there is any semantic dispute between maxTotalSize and
     maxChunkSize, maxTotalSize wins.
  */
  cwal_size_t maxChunkSize;

  /**
     The maximum total chunk size. The recycler will not store
     more than this. Set to (cwal_size_t)-1 for no (effective)
     limit and 0 to disable chunk recycling.
  */
  cwal_size_t maxTotalSize;

  /**
     If true (non-0) then cwal will try to use the
     memchunk allocator for allocating arbitrary new cwal_value
     instances _if_ it cannot find one in the value-type-specific
     recycler. That has the following properties and implications:

     - Requires an exact-size match.

     - Based on s2 tests, lowers the hit/miss ratio in the
     chunk lookup notably: dropping from ~5% to ~20% misses.

     - it's a micro-optimization: <1% total allocation reduction in
     s2's test suite.

     - This adds a O(N) component to Value allocations when the
     type-specific recycler is empty.
  */
  char useForValues;
};
typedef struct cwal_memchunk_config cwal_memchunk_config;

/**
   Convenience typedef.
*/
typedef struct cwal_memchunk_overlay cwal_memchunk_overlay;
/** @internal

    Internal utility for recycling chunks of memory. It can only be
    used with chunks having a size >= sizeof(cwal_memchunk_overlay).
*/
struct cwal_memchunk_overlay {
  /** The size of this chunk, in bytes. */
  cwal_size_t size;
  /**
     Next chunk in this linked list.
  */
  cwal_memchunk_overlay * next;
};
/**
   Initialized-with-defaults cwal_memchunk_config struct, used
   for const copy initialization. The values defined here are
   the defaults for the cwal framework.
*/
#if 16 == CWAL_SIZE_T_BITS
#define cwal_memchunk_config_empty_m {          \
    25/*maxChunkCount*/,                        \
    1024 * 32/*maxChunkSize*/,                \
    1024 * 63/*maxTotalSize*/,                \
    1/*useForValues*/                         \
  }
#else
#define cwal_memchunk_config_empty_m {          \
    25/*maxChunkCount*/,                        \
    1024 * 32/*maxChunkSize*/,                \
    1024 * 64/*maxTotalSize*/,                \
    1/*useForValues*/                         \
  }
#endif
/**
   Intended for use with copy construction. Is guaranteed
   to hold the same bits as cwal_memchunk_config_empty_m.
*/
extern const cwal_memchunk_config cwal_memchunk_config_empty;

/**
   A helper type for recycling "chunks" of memory (for use with
   arrays, buffers, hash tables, etc.).
*/
struct cwal_memchunk_recycler {
  /**
     The head of the active (awaiting recyling) chunks. Holds
     this->headCount entries.
  */
  cwal_memchunk_overlay * head;

  /**
     The number of entries in this->head.
  */
  cwal_size_t headCount;

  /**
     The total value of the size member of all entries of
     this->head. i.e. the amount of memory currently awaiting reuse.
     This does not include the memory held by this->pool, which is
     (this->capacity * sizeof(void*)) bytes.
  */
  cwal_size_t currentTotal;

  /**
     Various internal metrics.
  */
  struct {
    cwal_size_t totalChunksServed;
    cwal_size_t totalBytesServed;
    cwal_size_t peakChunkCount;
    cwal_size_t peakTotalSize;
    cwal_size_t smallestChunkSize;
    cwal_size_t largestChunkSize;
    cwal_size_t requests;
    cwal_size_t searchComparisons;
    cwal_size_t searchMisses;
    cwal_size_t runningAverageSize;
    cwal_size_t runningAverageResponseSize;
  } metrics;
    
  cwal_memchunk_config config;
};
/** Convenience typedef. */
typedef struct cwal_memchunk_recycler cwal_memchunk_recycler;
/**
   An empty-initialized const cwal_memchunk_recycler.
*/
#define cwal_memchunk_recycler_empty_m {            \
    0/*head*/, 0/*headCount*/, 0/*currentTotal*/,   \
    {/*metrics*/                                    \
      0/*totalChunksServed*/,                       \
      0/*totalBytesServed*/,                      \
      0/*peakChunkCount*/,                        \
      0/*peakTotalSize*/,                         \
      0/*smallestChunkSize*/,                     \
      0/*largestChunkSize*/,                      \
      0/*requests*/,                              \
      0/*searchComparisons*/,                     \
      0/*searchMisses*/,                          \
      0/*runningAverageSize*/,                    \
      0/*runningAverageResponseSize*/,            \
    },                                          \
    cwal_memchunk_config_empty_m                  \
  }

/**
   A generic buffer class used throughout the cwal API, most often for
   buffering arbitrary streams and creating dynamic strings.

   For historical reasons (and because a retrofit would be awkward, in
   terms of new APIs, and relatively expensive in terms of new costs),
   cwal_buffer has two distinct uses:

   - "Value Buffers" are created using cwal_new_buffer() and their
   "buffer part" (cwal_value_get_buffer()) is owned by the Value which
   wraps it.

   - "Plain" buffers are created not associated with a Value instance
   and are used as demonstrated below. They are, in practice, always
   created on the stack or embedded in another struct.

   Note that the memory managed by this class does not partake in any
   cwal-level lifetime management. It is up to the client to use it
   properly, such that the memory is freed when no longer needed.

   They can be used like this:

   @code
   cwal_buffer b = cwal_buffer_empty
   // ALWAYS initialize this way, else risk Undefined Bahaviour!
   // For in-struct initialization, use cwal_buffer_empty_m.
   ;
   int rc = cwal_buffer_reserve( e, &buf, 100 );
   if( 0 != rc ) {
   ... allocation error ...
   assert(!buf.mem); // just to demonstrate
   }else{
   ... use buf.mem ...
   ... then free it up ...
   cwal_buffer_reserve( e, &buf, 0 );
   }
   @endcode

   To take over ownership of a buffer's memory:

   @code
   void * mem = b.mem;
   // mem is b.capacity bytes long, but only b.used
   // bytes of it has been "used" by the API.
   b = cwal_buffer_empty;
   @endcode

   The memory now belongs to the caller and must eventually be
   cwal_free()'d. Even better, if you remember the buffer's original
   capacity after taking over ownership, you can use cwal_free2(),
   passing it the memory and the block's size (the buffer's former
   capacity), which may allow cwal to recycle the memory better.
*/
struct cwal_buffer
{
  /**
     The number of bytes allocated for this object.
     Use cwal_buffer_reserve() to change its value.
  */
  cwal_size_t capacity;
  /**
     The number of bytes "used" by this object's mem member. It must
     be <= capacity.
  */
  cwal_size_t used;

  /**
     The memory allocated for and owned by this buffer.
     Use cwal_buffer_reserve() to change its size or
     free it. To take over ownership, do:

     @code
     void * myptr = buf.mem;
     buf = cwal_buffer_empty;
     @endcode

     (You might also need to store buf.used and buf.capacity,
     depending on what you want to do with the memory.)
       
     When doing so, the memory must eventually be passed to
     cwal_free() to deallocate it.
  */
  unsigned char * mem;

  /**
     For internal use by cwal to differentiate between
     container-style buffers and non-container style without
     requiring a complete overhaul of the buffer API to make
     Container-type representations of them.

     Clients MUST NOT modify this. It is only non-NULL for buffers
     created via cwal_new_buffer() resp. cwal_new_buffer_value(),
     and its non-NULL value has a very specific meaning. To
     reiterate: clients MUST NOT modify this.

     20181126: i'm not certain that we still need this. We can(?)
     ensure that a buffer is-a Value by doing the conversion from a
     buffer to a value, then back again, and see if we get the same
     result. If not, it wasn't a Value. If we do, it's _probably_(?
     possibly?) a Value.
  */
  void * self;
};

/**
   An empty-initialized cwal_buffer object.

   ALWAYS initialize embedded-in-struct cwal_buffers by copying
   this object!
*/
#define cwal_buffer_empty_m {0/*capacity*/,0/*used*/,NULL/*mem*/, 0/*self*/}

/**
   An empty-initialized cwal_buffer object. ALWAYS initialize
   stack-allocated cwal_buffers by copying this object!
*/
extern const cwal_buffer cwal_buffer_empty;

/**
   A typedef used by cwal_engine_type_name_proxy() to allow
   clients to hook their own type names into
   cwal_value_type_name().

   Its semantics are as follows:

   v is a valid, non-NULL value. If the implementation can map
   that value to a type name it must return that type name string
   and set *len (if len is not NULL) to the length of that string.
   The returned bytes must be guaranteed to be static/permanent in
   nature (they may be dynamically allocated but must outlive any
   values associated with the name).

   If it cannot map a name to the value then it must return NULL,
   in which case cwal_value_type_name() will fall back to its
   default implementation.

   Example implementation:

   @code
   static char const * my_type_name_proxy( cwal_value const * v,
   cwal_size_t * len ){
   cwal_value const * tn = cwal_prop_get(v, "__typename", 10);
   return tn ? cwal_value_get_cstr(tn, len) : NULL;
   }
   @endcode

   @see cwal_engine_type_name_proxy()
*/
typedef char const * (*cwal_value_type_name_proxy_f)( cwal_value const * v, cwal_size_t * len );

/**
   A generic error code/message combination. Intended for reporting
   non-exception errors, possibly propagating them on their way to
   becoming exceptions. The core library does not need this, but it
   has proven to be a useful abstraction in client-side code, so was
   ported into the main library API.

   @see cwal_error_set()
   @see cwal_error_get()
   @see cwal_error_reset()
   @see cwal_error_clear()
*/
struct cwal_error {
  /**
     Error code, preferably a CWAL_RC_xxx value.
  */
  int code;
  /**
     Line-number of error, if relevant. 1-based, so use 0
     as a sentinel value.
  */
  int line;
  /**
     Column position of error, if relevant. 0-based.
  */
  int col;
  /**
     The error message content.
  */
  cwal_buffer msg;

  /**
     Holds a script name associated with this error (if any). We use a
     buffer instead of a string because it might be re-set fairly
     often, and we can re-use the memory.
  */
  cwal_buffer script;
};

/**
   Empty-initialized cwal_error structure, intended for const-copy
   initialization.
*/
#define cwal_error_empty_m {0, 0, 0, cwal_buffer_empty_m, cwal_buffer_empty_m}

/**
   Empty-initialized cwal_error structure, intended for copy
   initialization.
*/
extern const cwal_error cwal_error_empty;


/**
   The core manager type for the cwal API. Each "engine" instance
   manages a stack of scopes and (indirectly) the memory associated
   with Values created during the life of a Scope.
*/
struct cwal_engine {
  cwal_engine_vtab * vtab;
  /**
     Internal memory allocation marker.
  */
  void const * allocStamp;
  /**
     A handle to the top scope. Used mainly for internal
     convenience and sanity checking of the scope stack
     handling.
  */
  cwal_scope * top;
  /**
     Scope stack. Manipulated via cwal_scope_push() and
     cwal_scope_pop().

     TODO: rename this to currentScope someday.
  */
  cwal_scope * current;
    
  /**
     When a scope is cleaned, if deferred freeing is not active
     then this pointer is set to some opaque value known only by
     the currently-being-cleaned scope before it starts cleaning
     up. As long as this is set, freeing and recycling of
     containers is deferred until cleanup returns to the
     being-freed scope, at which point this value is cleared and
     the gc list is flushed, all of its entries being submitted
     for recycling (or freeing, if recycling is disabled or
     full).

     This mechanism acts as a safety net when traversing cycles
     where one of the traversed values was freed along the way. The
     lowest-level scope from which destruction is initiated
     (normally also the bottom-most scope, but i would like to
     consider having scopes as first-class values) is the "fence"
     for this operation because destruction theoretically cannot
     happen for values in higher scopes during cleanup of a lower
     scope. i.e. when destructing scopes from anywhere but the top
     of the stack the initial scope in the destruction loop is the
     one which will queue up any to-be-freed containers for
     recycling, and it will flush the gc list. Because values form
     linked lists, we use those to form the chain of deferred
     destructions, so this operation costs us no additional memory
     (it just delays deallocation/recycling a bit) and is O(1)
     (flushing the queue is O(N)).

     Note that types which cannot participate in graphs are not
     queued - they are (normally) immediately recycled or
     cwal_free()d when their refcount drops to 0 (or is reduced
     when it is already 0, as is the case for "temporary" values
     which never get a reference).
  */
  void const * gcInitiator;

  /**
     Internal flags. See the API-internal CWAL_FLAGS enum for
     the gory details.
  */
  uint32_t flags;

  /**
     A flag which will someday be used to flag assertion-level
     failures in non-debug builds, such that the engine, if a
     condition arises which is normally fatal/assert()ed, then it
     will refuse to do anything further except (reluctantly)
     cwal_engine_destroy().
  */
  int fatalCode;

  /**
     May hold non-exception error state, potentially on behalf of
     client code.
  */
  cwal_error err;

  /**
     List of list managers for (cwal_value*) of types which we can
     recycle. We can recycle memory for these types:

     integer, double, array, object, hash, native, buffer, function,
     exception, x-string/z-string (in the same list), cwal_kvp,
     cwal_scope, cwal_weak_ref, unique, cwal_tuple.

     The lists are in an order determined by the internal
     function cwal_recycler_index(). Other types cannot be
     recycled as efficiently (e.g. cwal_string use a separate
     mechanism) or because they are never allocated (null, bool,
     undefined and any built-in shared value instances).

     Reminder: the default recycle bin sizes do not reflect any
     allocation size, simply the number of objects we don't free
     immediately, so there is little harm is setting them
     relatively high (e.g. 100 or 1000). Because cwal_value and
     cwal_kvp objects form a linked list, a given recycle bin
     may grow arbitrarly large without requiring extra memory to
     do so (we just link the values in each recycle bin, and
     those values have already been allocated). This all happens in
     O(1) time by simply making each new entry the head of the
     list (and removing them in that order as well).

     Maintenance reminder: the indexes of this list which actually
     get used depend on how the engine sets up the bins. It combines
     like-sized types into the same bins. Thus this list has to be
     larger than strictly necessary for platforms where it cannot
     combine types. On x86/64, it needs 8 slots as of this writing.
  */
  cwal_recycler recycler[15];

  /**
     reString is a special-case recycler for cwal_string values.
     String values are recycled based on their size. i.e. we
     won't recycle a 36-byte string's memory to serve a 10-byte
     string allocation request.

     As an optimization, we (optionally) pad string allocations
     to a multiple of some small number of bytes (e.g. 4 or 8),
     as this lets us recycle more efficiently (up to 36% more
     string recycling in some quick tests).

     See the API-internal CwalConsts::StringPadSize for details.
  */
  cwal_recycler reString;

  /**
     A generic memory chunk recycler.
  */
  cwal_memchunk_recycler reChunk;

  /**
     A place for client code to associated a data pointer and
     finalizer with the engine. It is cleaned up early in the engine
     finalization process.
  */
  cwal_state client;

  /**
     A value-to-type-name proxy, manipulated via
     cwal_engine_type_name_proxy().
  */
  cwal_value_type_name_proxy_f type_name_proxy;

  /**
     Where we store internalized strings.

     Maintenance note: this is-a cwal_ptr_table but uses its own
     hashing/searching/insertion/removal API. Do NOT use the
     equivalent cwal_ptr_table ops on this instance.  See the
     internal cwal_interned_search(), cwal_interned_insert(),
     and cwal_interned_remove() for details.
  */
  cwal_ptr_table interned;

  /**
     Memory for which we have a weak reference is annotated by
     simply inserting its address into this table. When the
     memory is cleaned up, if it has an entry here, we
     invalidate any cwal_weak_refs which point to it.
  */
  cwal_ptr_table weakp;

  /**
     cwal_weak_ref instances are stored here, grouped by
     underlying memory type to speed up the invalidate-ref
     operation. Weak refs to buit-in constants are handled
     specially (to avoid allocating new instances and because
     they can never be invalidated). Some slots of this array
     (those of constant types, e.g. null/undefined/bool) are
     unused, but we keep their slots in this array because it
     greatly simplifies our usage of this array.

     The (void*) memory pointed to by weak references is held in
     weakr[CWAL_TYPE_WEAK_REF], since that slot is otherwise
     unused. We "could" use the NULL/BOOL/UNDEF slots for
     similar purposes.
  */
  cwal_weak_ref * weakr[CWAL_TYPE_end];

  /**
     The top-most scope. This is an optimization to avoid an
     allocation.
  */
  cwal_scope topScope;

  /**
     If built with CWAL_ENABLE_TRACE to 0 then this is a no-op
     dummy placeholder, else it holds information regarding
     engine tracing. This state continually gets overwritten if
     tracing is active, and sent to the client via
     this->api->tracer.
  */
  cwal_trace_state trace;

  /**
     Buffer for internal string conversions and whatnot. This
     buffer is volatile and its contents may be re-allocated or
     modified by any calls into the public API. Internal APIs
     need to be careful not to stomp the buffer out from under
     higher-scope public APIs and internal calls.
  */
  cwal_buffer buffer;

  /**
     Where Function-type call() hooks are stored.
  */
  cwal_callback_hook cbHook;

  /**
     _Strictly_ interal bits for managing various specific values or
     categories of values.
  */
  struct {
    /**
       Holds any current pending exception, taking care to
       propagate it up the stack when scopes pop.
    */
    cwal_value * exception;

    /**
       A slot for a single propagating value which will
       automatically be pushed up the stack as scopes
       pop. Intended for keywords which propagate via 'return'
       semantics or error reporting, so that they have a place to
       keep their result (if any).
    */
    cwal_value * propagating;
        
    /**
       Where clients may store their customized base prototypes
       for each cwal_type_id. The indexes in this array correspond
       directly to cwal_type_id values, but (A) that is an
       implementation detail, and (B) some slots are not used (but
       we use this structure as a convenience to save cycles in
       type-to-index conversions and to keep those prototypes
       vacuum-safe).
    */
    cwal_array * prototypes;

    /**
       A place to store values which are being destroyed during
       the traversal of cycles. Used for delayed freeing of
       cwal_value memory during scope cleanup. See gcInitiator
       for more details.
    */
    cwal_value * gcList;

    /**
       Internal optimization for cwal_hash_resize() and
       cwal_hash_take_props() to allow us to transfer a kvp
       directly from the source to the target. This gets
       transfered (or not) from a container in the calling op to
       the target hashtable during the cwal_hash_insert_v(). If,
       after calling cwal_hash_insert_v(), it's not 0, it's up to
       the caller to manage its memory and reset this to 0.
    */
    cwal_kvp * hashXfer;
  } values;

  /**
     Holds metrics related to honoring memory capping. Note that
     recycled memory only counts once for purposes of these metrics.
     i.e. recycling a chunk of memory counts as a single allocation
     (the initial one), not a separate allocation each time the
     chunk is recycled.
  */
  struct {
    /**
       Caps concurrent cwal-allocated memory to this ammount.
    */
    cwal_size_t currentMem;
    /**
       Caps the concurrent cwal allocation count to this number
       (or thereabouts - reallocations are kind of a grey area).
    */
    cwal_size_t currentAllocCount;
    /**
       Caps the peak amount of cwal-allocated memory to
       this amount.
    */
    cwal_size_t peakMem;
    /**
       Records the peak concurrent allocation count (or
       thereabouts - reallocations are kind of a grey area). This
       is not a memory capping constraint.
    */
    cwal_size_t peakAllocCount;
    /**
       Caps the total cwal allocation count to this number (or
       thereabouts - reallocations are kind of a grey area).
    */
    uint64_t totalAllocCount;
    /**
       Caps the total amount of cwal-allocated memory to this
       amount.
    */
    uint64_t totalMem;
  } memcap;

  /**
     A place for storing metrics.
  */
  struct {
    /**
       Each time a request is made to allocate a Value, the
       value type's entry in this array is increments. Does
       not apply to certain optimized-away situations like
       empty strings, bools/null/undef, and the constant
       numeric values.
    */
    cwal_size_t requested[CWAL_TYPE_end];
    /**
       Each time we have to reach into the allocator to
       allocate an engine resource, its type's entry in this
       array is incremented. This values will always be less
       than or equal to the same offered in the 'requested'
       member.
    */
    cwal_size_t allocated[CWAL_TYPE_end];

    /**
       The number of allocated bytes for each type is totaled
       here.  We can't simply use (allocated*sizeof) for some
       types (e.g. strings, arrays, buffers, and hashtables),
       and this value requires some fiddling with in certain
       areas to ensure it gets all memory for some types
       (namely arrays and buffers, whose sizes change with
       time). In any case, it is only a close approximation
       because reallocs play havoc with our counting in some
       cases.
    */
    cwal_size_t bytes[CWAL_TYPE_end];

    /**
       clientMemTotal keeps a running total of memory usage
       declared by the client.

       @see cwal_engine_adjust_client_mem()
    */
    cwal_size_t clientMemTotal;

    /**
       Holds the current amount of memory declared by the client.

       @see cwal_engine_adjust_client_mem()
    */
    cwal_size_t clientMemCurrent;

    /**
       Each time an attempt to recycle a recyclable Value
       type (or cwal_kvp) succeeds, this is incremented.
    */
    cwal_size_t valuesRecycled;

    /**
       Each time we look in the value recyclers for a value and do
       not find one, this is incremented.
    */
    cwal_size_t valuesRecycleMisses;

    /**
       Counts the number of blocks for which
       this->recoveredSlackBytes applies.
    */
    cwal_size_t recoveredSlackCount;
    /**
       When over-allocating to account for memory capping,
       the chunk recycler can recover more "slack" bytes.
       Those are counted here.
    */
    cwal_size_t recoveredSlackBytes;

    /**
       The highest-ever refcount on any Value.
    */
    cwal_refcount_t highestRefcount;

    /**
       The data type of the value for which the highestRefcount
       metric was measured.
    */
    cwal_type_id highestRefcountType;

    /**
       Records the number of allocations saved by the using the
       built-in list of length-1 ASCII strings to serve
       cwal_new_string(), cwal_new_xstring(), and
       cwal_new_zstring() allocations.

       Indexes: 0: string, 1: x-string, 2: z-string
    */
    cwal_size_t len1StringsSaved[3];
  } metrics;
};
/** @def cwal_engine_empty_m
    Empty initialized const cwal_engine struct.
*/
#define cwal_engine_empty_m {                                           \
  NULL /* vtab */,                                                    \
  NULL /* allocStamp */,                                            \
  NULL /* top */,                                                   \
  NULL /* current */,                                               \
  NULL /* gcInitiator */,                                           \
  0U /* flags */,                                                   \
  0 /* fatalCode */,                                                \
  cwal_error_empty_m,                                               \
  {/* recycler */                                                     \
    cwal_recycler_empty_m, cwal_recycler_empty_m,                     \
    cwal_recycler_empty_m, cwal_recycler_empty_m,                   \
    cwal_recycler_empty_m, cwal_recycler_empty_m,                   \
    cwal_recycler_empty_m, cwal_recycler_empty_m,                   \
    cwal_recycler_empty_m, cwal_recycler_empty_m,                   \
    cwal_recycler_empty_m, cwal_recycler_empty_m,                   \
    cwal_recycler_empty_m, cwal_recycler_empty_m,                   \
    cwal_recycler_empty_m                                           \
  },                                                              \
  {/*reString*/ CWAL_TYPE_STRING, 0, 40, NULL, 0, 0 }, \
  /* reChunk*/ cwal_memchunk_recycler_empty_m,                        \
  cwal_state_empty_m /* client */,                                    \
  NULL/*type_name_proxy*/,                                            \
  cwal_ptr_table_empty_m/*interned*/,                                 \
  cwal_ptr_table_empty_m/*weakp*/,                                    \
  {/*weakr*/                                                            \
    NULL,NULL,NULL,NULL,NULL,                                           \
    NULL,NULL,NULL,NULL,NULL,                                         \
    NULL,NULL,NULL,NULL,NULL,                                         \
    NULL,NULL,NULL,NULL,NULL,                                         \
    NULL                                                              \
  },                                                                \
  cwal_scope_empty_m/*topScope*/,                                     \
  cwal_trace_state_empty_m/*trace*/,                                  \
  cwal_buffer_empty_m/*buffer*/,                                      \
  cwal_callback_hook_empty_m/*cbHook*/,                               \
 {/*values*/                                                           \
   NULL/* exception */,                                                 \
   NULL/* propagating */,                                             \
   NULL/* prototypes */,                                              \
   NULL/* gcList */,                                                  \
   NULL /* hashXfer */                                                \
 },                                                                 \
 {/*memcap*/ \
   0/*currentMem*/, 0/*currentAllocCount*/,                             \
   0/*peakMem*/, 0/*peakAllocCount*/,                                 \
   0/*totalAllocCount*/, 0/*totalMem*/                                \
 },                                                                 \
 {/*metrics*/ \
   /*requested[]*/{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,0},      \
   /*allocated[]*/{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,0},    \
   /*bytes[]*/    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,0},    \
   0/*clientMemTotal*/, 0/*clientMemCurrent*/,                        \
   0/*valuesRecycled*/, 0/*valuesRecycleMisses*/,                     \
   0/*recoveredSlackCount*/, 0/*recoveredSlackBytes*/,                \
   0/*highestRefcount*/, CWAL_TYPE_UNDEF/*highestRefcountType*/,      \
   {0,0,0}/*len1StringsSaved*/                                          \
 }                                                                      \
}/*end cwal_engine_empty_m*/

/**
   Empty-initialized cwal_engine object. When initializing
   stack-allocated cwal_engine instances, always copy this instance
   over them to set up any default state. For in-struct
   initialization, use cwal_engine_empty_m.
*/
extern const cwal_engine cwal_engine_empty;

/**
   Initializes a cwal_engine instance. vtab must not be NULL and must
   be populated in accordance with the cwal_engine_vtab
   documentation. e must either be a pointer to a NULL-initialized
   pointer or a pointer to a pre-allocated instance (possibly from
   the stack or embedded in another struct) with a clean state
   (e.g. by copying cwal_engine_empty or cwal_engine_empty_m over
   it).

   vtab MUST outlive e, and in practice it can normally be static.

   On success 0 is returned and *e points to the engine instance.

   On success e is initialized with a top scope, so clients need not
   use cwal_scope_push() before using the cwal_new_xxx() family
   of factory functions to allocate values.
    
   On error non-0 is returned and *e is NOT cleaned up so that
   any error state in the object can be collected by the client
   before freeing it. If *e is NULL on returning and argument
   validation succeeds, then allocation of the instance failed
   and CWAL_RC_OOM will be returned. If vtab.on_init() returns
   non-0, this function will return that code, but vtab.on_init()
   is only called if the priliminary initialization succeeds
   (it can only fail on allocation errors).

   In short: the caller must, regardless of whether or not this
   function succeeds, if *e is not NULL, eventually pass *e to
   cwal_engine_destroy(). If this function allocated it, that
   function will free it.

   Errors include:

   CWAL_RC_MISUSE: one of the arguments is NULL

   CWAL_RC_OOM: memory allocation failed

   Any other error code should be considered a bug in this code.


   Potential TODO: require the client to push the first scope,
   giving him a time slot between initialization and the first
   scope to set configuration options which might affect the first
   scope (currently we have none built in, and the vtab's on_init()
   hook can be used to make changes for the time being).
*/
int cwal_engine_init( cwal_engine ** e, cwal_engine_vtab * vtab );

/**
   Frees all resources owned by e. If e was allocated dyanamically by
   cwal_engine_init() (or equivalent) then this function
   deallocates it, otherwise e is a left in an empty state after this
   call (suitable for re-using with cwal_engine_init()).

   As a special case, if e has no vtab then it is assumed that
   cwal_engine_init() hhas not yet been called and this function
   returns 0 and has no side effects.
*/
int cwal_engine_destroy( cwal_engine * e );

/**
   Installs f as a proxy for cwal_value_type_name(). See
   cwal_value_type_name_proxy_f for full details. f may be NULL
   but e may not. e may only have one type name proxy installed at
   a time, and this replaces any existing one.

   Returns the existing proxy (possibly NULL), so that the client
   may swap it in and out, but in practice the client sets this up
   only once in the initialization phase.

   @see cwal_value_type_name()
   @see cwal_value_type_name2()
*/
cwal_value_type_name_proxy_f
cwal_engine_type_name_proxy( cwal_engine * e,
                             cwal_value_type_name_proxy_f f );

/**
   Pushes a new scope onto e's stack, making it the current scope.
   If s is not NULL and (*s) is NULL then *s is assigned to a
   newly-allocated/recycled scope on success.
       
   If s is not NULL and (*s) is not NULL when this function is
   called then it must be a cleanly-initialized scope value, and
   this function will use it instead of allocating/recycling
   one. For example:

   @code
   cwal_scope sub_ = cwal_scope_empty;
   cwal_scope * sub = &sub_;
   int rc = cwal_scope_push(e, &sub);
   if(rc) { ...error... do NOT pop the scope ...}
   else {
   ...do your work, then...
   cwal_scope_pop(sub);
   }
   @endcode

   (An easy way to determine, at the end of the function, whether
   'sub' was pushed is to check if sub->parent is 0 (in which case it
   was not pushed).)

   Using scopes this way can be considered a malloc-count
   optimization over simply passing NULL as the 's'
   parameter. Initializing scopes this way does not change how
   they are popped: cwal_scope_pop() is still the correct way to
   clean up the scope. Client who want to ensure that downstream
   code has not corrupted the stack can check if
   cwal_scope_current_get()==theirScopePointer before popping the
   stack (and should fail loudly if they do not match).

   Returns 0 on success. On error:

   CWAL_RC_MISUSE: e is NULL

   CWAL_RC_OOM: memory allocation error. This error historically could
   only happen if the user passes a NULL or a _pointer to NULL_ as the
   second parameter, otherwise this function allocates no memory and
   cannot fail if e and s are valid. However...

   As of 20181123, if the client application has a scope push hook
   (cwal_engine_vtab::hook:scope_push) installed then that hook gets
   called during this process (unless cwal needs to allocate the scope
   but cannot do so), in which case the result code from that hook is
   returned. It's conceivable that it may return a CWAL_RC_OOM, but
   any other error result seems unlikely in practice.

   When passed a client-supplied scope, this function has no error
   conditions as long as e is valid, with the caveat that a
   scope push hook (mentioned above) might fail.

   Reminder to self: practice suggests that, because scopes are
   effectively always stack-allocated, this really should take a
   pointer, not pointer-to-pointer 2nd argument. We'll add a second
   function for that case.

   @see cwal_scope_push2()
   @see cwal_scope_pop()
   @see cwal_scope_pop2()
*/
int cwal_scope_push( cwal_engine * e, cwal_scope ** s );

/**
   This is a convenience form of cwal_scope_push() which requires that
   s be non-NULL and be a freshly-initialized instance by having
   cwal_scope_empty copied over it to initialize its state. In
   practice, cwal_scope instances are always stack-allocated in a
   local function, and this variant simplifies the usage of
   such instances.

   Returns CWAL_RC_MISUSE if e or s are NULL or if s appears to
   contain any non-default state, else it returns as for
   cwal_scope_push().

   This routine is effectively simplifies this:

   @code
   cwal_scope _s = cwal_scope_empty;
   cwal_scope * s = &_s;
   int rc = cwal_scope_push( e, &s );
   @endcode

   to this:

   @code
   cwal_scope s = cwal_scope_empty;
   int rc = cwal_scope_push2( e, &s );
   @endcode

   @see cwal_scope_push()
   @see cwal_scope_pop()
   @see cwal_scope_pop2()
*/
int cwal_scope_push2( cwal_engine * e, cwal_scope * s );

/**
   Pops the current scope from the stack.

   Returns 0 on success. Returns CWAL_RC_MISUSE if !e and
   CWAL_RC_RANGE if e has no current scope (use cwal_scope_push()
   first)).
       
   This "really shouldn't" ever fail, and a failure is likely either
   the result of a cwal-internal bug or serious offenses against the
   memory management rules.

   @see cwal_scope_pop2()
*/
int cwal_scope_pop( cwal_engine * e );

/**
   A variant of cwal_scope_pop() which rescopes the given value (if
   not NULL) to the parent of the to-pop scope (if needed).

   This is intended to simplify propagating result values up the scope
   stack.

   Returns 0 on success, else:

   - CWAL_RC_MISUSE if !e

   - CWAL_RC_RANGE if resultVal is not NULL and e's top-most scope is
   the current scope (because no value may outlive the top scope).

   Returns 0 on success, else (except as mentioned above) as
   documented for cwal_scope_pop().

   Note that the reference count of the 2nd argument is not modified
   by this routine. Whether or not it needs/has a reference is
   entirely up to the surrounding code.

   Example:

   @code
   int rc;
   cwal_value * v;
   cwal_scope scope = cwal_scope_empty;
   rc = cwal_scope_push2( e, &scope );
   if(rc) return rc;
   assert(scope.parent); // just to demonstrate
   v = ...;
   cwal_value_ref(v);
   ... other stuff ...
   rc = cwal_scope_pop2( e, v );
   assert(!scope.parent); // just to demonstrate
   assert(cwal_value_refcount(v) || cwal_value_is_builtin(v));
   // Eventually we need to relinquish that ref. In the case
   // of propagating values, we normaly want:
   cwal_value_unhand(v);
   // So that we can pass the value back up to the caller with
   // a predictable, yet possibly zero, refcount.
   // If we instead want to discard the value altogether then
   // we need: cwal_value_unref(v);
   @endcode

   @see cwal_scope_pop()
   @see cwal_scope_push()
   @see cwal_scope_push2()
   @see cwal_value_unhand()
*/
int cwal_scope_pop2( cwal_engine * e, cwal_value * resultVal );

/**
   "Might partially clean up" the given scope, as follows...

   For each value owned by scope which has a reference count of
   _exactly_ 0, it is unref'd. Values with a refcount of 0 are
   considered "probationary" values, subject to summary cleanup.
   Once a value has ever had a reference added to it, it moves out
   of probationary status and cannot be affected by sweep
   operations unless they are once again re-probated (which can
   happen in one of several special cases). Thus this could be
   called after (or periodically during) loops which create lots
   of anonymous/throw-away values.

   It is only safe to call this if the client has explicitly
   referenced all values from the current scope which he is still
   holding a pointer to (and expects that pointer to be valid after
   this call). See cwal_new_VALUE() for a description of how
   references are acquired.

   Returns the number of unref's triggered by the sweep, or 0
   if any arguments are invalid. Note that it does not count
   recursively-removed values in the return code because
   those cleanups happen at a different level.

   Performance is effectively O(N+M) when no cycles are
   introduced, where N=total number of probationary values and M
   is the cleaning costs of those values cleaned. Cycles
   theoretically cannot happen in a probationary object because
   that would necessarily cause a refcount increase of the value
   partaking in the cycle, which would move the value out of
   probationary state.

   Note that if two calls are made to this function without having
   allocated/transfered new values from/to s, the second (and
   subsequent) will be no-ops because only probationary values are
   affected in any way.

   There are certain abstract operation chains where calling this
   will almost certainly be fatal to the app. For example,
   consider this pseudocode:

   @code
   myFunction( 1+2, 7*8, somethingWhichSweeps );
   @endcode

   The sweep activated in the 3rd argument could (depending on how the
   arguments are collected) destroy the temporaries before they get
   passed on to the function. Thus it is important that evaluation
   engines hold refs to all values they're working with.

   Earlier in the docs we mentioned a special corner case where a
   value can re-enter probationary state. This happens when moving
   values up scopes while containers in lower (newer) scopes holding
   references to them get cleaned up. Example code taken from th1ish:

   @code
   assert 17 === false || scope {
   var obj = object {a:17}
   obj.a // implicit scope result value
   }
   @endcode

   Normally obj.a would be cleaned up by obj at scope's end, but the
   'scope' operator supports implicit returns and thus needs to pass
   it unmolested up the scope chain. In this case, that moves (former)
   obj.a into the parent scope with a refcount of 0, moving it back
   into probationary state.

   @see cwal_engine_sweep()
   @see cwal_engine_vacuum()
*/
cwal_size_t cwal_scope_sweep( cwal_scope * s );

/**
   Returns the cwal_engine which owns s, or NULL if s is NULL.
*/
cwal_engine * cwal_scope_engine(cwal_scope const * s);

/**
   Calls cwal_scope_sweep() on e's current scope, returning the
   result of that call. Returns 0 if !e or if e has no current
   scope.

   @see cwal_scope_sweep()
   @see cwal_engine_vacuum()
*/
cwal_size_t cwal_engine_sweep( cwal_engine * e );
    
/**
   If allScopes is false, this behaves like cwal_engine_sweep(),
   otherwise it sweeps each scope, as per cwal_scope_sweep(),
   starting at the current scope and working upwards in the
   stack. Returns the total of all resulting cwal_scope_sweep()
   calls.

   This is inherently a dangerous operation as it can sweep up values
   in older scopes which are being used by the current scope if those
   values do not have a reference somewhere. Potential culprits here
   include:

   - Temporaries created while evaluating function arguments,
   which then get passed (without an explicit ref) to a function
   which triggers the recursive sweep. If the arguments get
   a reference, they are not problematic here.

   - Propagating result values, because they are not tracked directly
   by cwal, are not exempt from sweep-up. Initially, cwal kept track
   of a single result value, but it turned out (in th1ish) to be much
   easier to do from client code (the script interpreter). Maybe we
   can revisit that design decision someday. (Someday: see
   cwal_propagating_get() and cwal_propagating_set().)

   - Propagating exceptions have a reference, so are immune to
   sweep-up.
*/
cwal_size_t cwal_engine_sweep2( cwal_engine * e, char allScopes );


/**
   This function cleans up all values owned by the current scope
   by determining whether or not they refer to, or are referred to
   by, scope-level properties (variables).

   The mechanism is relatively simple:

   1) Push a new scope onto the stack with the same parent as e's
   current scope, but fake its level to (s->level-1) so that it
   looks like an older scope. We'll call the current scope s1 and
   this new scope s2.

   2) upscope s1's object properties and any values in s1 marked
   as vacuum-proof into s2. Because s2 looks like an older scope,
   this will transfer any values in s1 which are variables,
   vacuum-proof, or reachable via either of those, leaving any
   orphaned values in s1 but not in s2.

   3) clean all values remaining in s1.

   4) re-set s2's parent to be s1 and fake s2's level to
   (s1->level+1) so that s2 looks like a newer scope.

   5) upscope (again) the object properties and vacuum-proofed values,
   this time from s2 to s1. Because of step 4, s1 now looks like a
   higher/older scope to the rescope process, which will move the
   variables, and values referenced by them, back into s1. Note that
   we cannot simply move the value lists from s2 to s1 because we need
   to ensure that the value->scope pointers all point to where they
   need to, and the underlying engine does that for us if we just copy
   (well, move) the values back again.

   6) Clean up scope s2, as if it had been popped from the stack.

   The end result is that after this call (on success), only variabes,
   vacuum-proofed values, and values reachable via either variables or
   vacuum-proofed values, will be in the scope, all other (presumably
   script-unreachable) values having been cleaned up.

   This operation requires no allocation, just traversal of values to
   tag them with their new scope. It is, computationally, speaking,
   difficult to predict the performance. For current th1ish/s2 uses it
   is quite fast enough to run very often (after every expression
   evaluation), but very complex graphs will slow it down a bit. For
   most purposes (no graphs or only few simple ones) it can be
   considered linear on the number of values owned by the scope.

   ACHTUNG: this invalidates any and all of the following pointers:

   - Values owned by this scope but which are not reachable from
   either scope-level variables or a vacuum-proof value. They are
   destroyed.

   Returns 0 on success. On success, if sweepCount is not 0 then
   it is set to the number of values removed from the scope by
   this operaiton. If sweepCount is 0, this operation is a few
   ticks faster because it does not have to do any extra counting.

   Any error other than argument validation (CWAL_RC_MISUSE) indicates
   either an allocation problem or unexpected bits were found while
   fiddling around, either of which must be treated as fatal to the
   cwal_engine. In this case, the current scope will be cleaned up in
   its entirety (but not popped from the scope stack) because we
   simply have no other sane recovery strategy where all known values
   have some reasonable lifetime. (Sidebar: that has never happened
   in practice.)

   The only possible errors are invalid arguments or corruption
   detected during the operation. In debug builds it will assert() if
   it detects anything wrong.

   To make specific Values immune to vacuuming, use
   cwal_value_make_vacuum_proof().

   Design note: sweepCount is a signed int because initial tests
   in th1ish have added values to the scope (possibly internals
   not visible from script code), leading to an overall negative
   sweep count. i believe this to be either a th1ish-side usage
   error or bug, however: sweepCount should always be 0 or
   positive on success, and this code assert()s that that is so.

   @see cwal_value_make_vacuum_proof()
   @see cwal_engine_sweep()
*/
int cwal_engine_vacuum( cwal_engine * e, int * sweepCount );

/**
   DANGEROUS! DO NOT USE!

   This works identically to cwal_engine_vacuum() but applies to the
   given scope, as opposed to the current (though it may be the
   current scope). Whether or not this operation is "safe" on any
   scope other than the current one very much depends on how
   client-side code manages its own cwal_value instances. DO NOT call
   this function unless you know for absolute certain that doing so is
   legal vis-a-vis the app's cwal_value management. When uncertain,
   don't call it.

   @see cwal_engine_vacuum().
*/
int cwal_scope_vacuum( cwal_scope * s, int * sweepCount );

/**
   Sets the given pointers as client state in e. What that means is:

   - e applies no meaning to the state but will, at cleanup time,
   call dtor() (early on in the engine shutdown process) to clean
   up the state if dtor is not 0.

   - There can be only one piece of client state installed at a time,
   and this function fails with CWAL_RC_ACCESS if state
   is already set (to avoid having to answer questions about its
   lifetime).

   - cwal_engine_client_state_get() can be used, passed the same
   (e, typeId) values used here, to fetch the pointer later on.
   Calls to that function with other (e, typeId) combinations will
   return 0.

   The typeId can be an arbitrary pointer value, but must outlive e.
   It is typically the address of some static const value associated
   with state's concrete data type.

   Returns 0 on success. Errors include:

   - CWAL_RC_MISUSE if e or state are 0 (typeId and dtor may be 0)

   - CWAL_RC_ACCESS if state has already been sete on e.
*/
int cwal_engine_client_state_set( cwal_engine * e,
                                  void * state, void const * typeId,
                                  cwal_finalizer_f dtor);

/**
   If cwal_engine_client_state_set() was passed e and typeId at some
   point, and it has not be re-set since then, then this returns the
   state pointer, otherwise it returns 0.
*/
void * cwal_engine_client_state_get( cwal_engine * e, void const * typeId );


/**
   On success, *s is assigned to the current scope and 0 is returned.
   On error *s is not modified and one of the following are returned:

   CWAL_RC_MISUSE: one of the arguments is NULL.

   CWAL_RC_RANGE: e currently has no scope.
*/
int cwal_scope_current( cwal_engine * e, cwal_scope ** s );

/**
   Simplified form for cwal_scope_current() which returns the
   current scope, or 0 if !e or if there are no scopes.
*/
cwal_scope * cwal_scope_current_get( cwal_engine * e );

/**
   Interpreter-level flags for "variables." Maintenance reminder: keep
   these flags at 16 bits or less (see cwal_kvp::flags).
*/
enum cwal_var_flags {
/**
   The no-flags/default value.
*/
CWAL_VAR_F_NONE = 0,
/**
   Indicates that the variable/property should be "const."
   Property-setting routines must refuse to re-set a property which
   has this flag. cwal_props_clear() (and similar) must ignore this
   flag and clear the property.
*/
CWAL_VAR_F_CONST = 0x0001,

/**
   Indicates that property iteration operations on types capable of
   holding key/value pairs should not expose properties with this flag
   via iteration-like routines, e.g. cwal_props_visit_kvp() and
   friends. Such values will be found normally if searched for by
   their key.
*/
CWAL_VAR_F_HIDDEN = 0x0002,

/**
   Reserved. Unused.
*/
CWAL_VAR_F_PROP_GETTER = 0x0010,

/**
   Reserved. Unused.
*/
CWAL_VAR_F_PROP_SETTER = 0x0020,

/**
   Reserved. Unused.
*/
CWAL_VAR_F_PROP_INTERCEPTOR = CWAL_VAR_F_PROP_GETTER | CWAL_VAR_F_PROP_SETTER,

/**
   Indicates that any existing flags of the property should be
   kept as-is. For newly-created properties this is applied as if
   it were CWAL_VAR_F_NONE.

   Maintenance reminder: must currently be 16 bits.
*/
CWAL_VAR_F_PRESERVE = 0xFFFF

};

/**
   Flags for use exclusively with container-type values,
   cwal_container_flags_set(), and cwal_container_flags_get().

   These are limited to 16 bits.

   @see cwal_container_flags_set()
   @see cwal_container_flags_get()
*/
enum cwal_container_flags {
/**
   Tells the engine that setting object properties OR hash entries on
   this container is not allowed, and should trigger a
   CWAL_RC_DISALLOW_PROP_SET error.

   The restriction on hash entries is arguably a bug, but s2 currently
   relies on it (201912). FIXME: separate that case into a separate
   flag and have s2 accommodate that.

   Note that this restriction does not apply to array/tuple indexes,
   but it probably should (noting that tuples do not have container
   flags!). That behaviour may change in the future or another flag
   may be added to cover that case (for arrays, at least). In
   particular, it would be useful for tuples which are used as
   object/hash keys, to ensure that they keep a stable sort order.
*/
CWAL_CONTAINER_DISALLOW_PROP_SET = 0x0001,

/**
   Tells the engine that setting NEW properties on this container is
   not allowed, and should trigger a CWAL_RC_DISALLOW_NEW_PROPERTIES
   error.

   Note that this restriction does not apply to array indexes, but it
   probably should. That behaviour may change in the future.
*/
CWAL_CONTAINER_DISALLOW_NEW_PROPERTIES = 0x0002,

/**
   Specifies that cwal_value_prototype_set() should fail with code
   CWAL_RC_DISALLOW_PROTOTYPE_SET for a value with this flag.
*/
CWAL_CONTAINER_DISALLOW_PROTOTYPE_SET = 0x0004,

CWAL_CONTAINER_RESERVED1 = 0x0008 /* DISALLOW_LIST_SET? arrays? */,
CWAL_CONTAINER_RESERVED2 = 0x0010 /* DISALLOW_HASH_INSERT? */,

/**
   EXPERIMENTAL and likely to never enter public service. A flag for
   Functions indicating that they are to be treated as property
   interceptors via certain getter/setter APIs.
*/
CWAL_CONTAINER_INTERCEPTOR = 0x0020,
/**
   EXPERIMENTAL and likely to never enter public service. Transient
   flag to avoid recursion.
*/
CWAL_CONTAINER_INTERCEPTOR_RUNNING = 0x0040

};

/**
   Returns s's property storage object, instantiating it if
   necessary. If s is NULL, or on allocation error, it returns
   NULL. The returned Value will be of "some container type,"
   currently Object or Hashes, depending on whether the
   CWAL_FEATURE_SCOPE_STORAGE_HASH flag is active when the properties
   are initialized.

   s holds a reference to this container and the API offers no way to
   take ownership away from s, other than prying it from s's cold,
   dead fingers (which we'll leave as an exercise to the reader (tip:
   reference and rescope the properties before popping s)).

   It is strongly recommended that client code NOT expose this value
   via script-side features. The temptation to do so is strong, but
   all of the potential side-effects of doing so are not yet fully
   understood, and there are caveats vis-a-vis vacuum-safety.
*/
cwal_value * cwal_scope_properties( cwal_scope * s );

/**
   Returns the parent scope of s, NULL if !s or s has
   no parent.
*/
cwal_scope * cwal_scope_parent( cwal_scope * s );

/**
   Returns the top scope in s's stack, NULL if !s.
*/
cwal_scope * cwal_scope_top( cwal_scope * s );
    
/**
   Searches s and optionally its parents for the given key.  If
   maxDepth is greater than 0 then only up to that many scope
   levels is searched. If maxDepth is less than 0 then any number
   of parent levels can be searched. A maxDepth of 0 means to
   search only s.

   If foundIn is not NULL, it is assigned the scope in which
   the property is found.
       
   Returns NULL if !s, !k, or no entry is found, else returns the
   searched-for value.

   See cwal_prop_get_kvp_v() for import notes about the
   lookup/property key comparisons.

   @see cwal_prop_get_kvp_v()
   @see cwal_scope_search()
*/
cwal_value * cwal_scope_search_v( cwal_scope * s, int maxDepth,
                                  cwal_value const * k,
                                  cwal_scope ** foundIn );

/**
   Similar to cwal_prop_get_kvp_v(), but searches through a scope
   (optionally recursively). upToDepth is interpreted as described
   for cwal_scope_search_v(). If a match is found, the underlying
   key-value pair is returned and foundIn (if not 0) is assigned to
   the scope in which the match was found.

   Returns 0 if no mach is found, !s, or !key.

   This routine explicitly does not search for properties via
   prototypes of the scope's property storage. i.e. only "declared"
   variables can be found this way, not properties inherited by
   the underlying property storage object/hash.

   ACHTUNG: the returned object is owned by an object (or hash) which
   is owned either by the scope the key is found in or an older one,
   and it may be invalidated on any modification of that object
   (i.e. any changing of properties in that scope). i.e. don't hold on
   to this, just grab its flags or whatever and let go of it.

   @see cwal_scope_search_kvp_v()
   @see cwal_scope_search()
*/
cwal_kvp * cwal_scope_search_kvp_v( cwal_scope * s,
                                    int upToDepth,
                                    cwal_value const * key,
                                    cwal_scope ** foundIn );

/**
   The C-string counterpart of cwal_scope_search_kvp_v(). The first
   keyLen bytes of key are used as the search key.

   See cwal_prop_get() for details about the lookup key
   comparison.

   @see cwal_scope_search_kvp_v()
   @see cwal_scope_search()
*/
cwal_kvp * cwal_scope_search_kvp( cwal_scope * s,
                                  int upToDepth,
                                  char const * key,
                                  cwal_midsize_t keyLen,
                                  cwal_scope ** foundIn );


/**
   Functionally equivalent to cwal_scope_search_v() except that it
   takes a C-string key and can only match String-typed keys. The
   first keyLen bytes of key are used as the search key.

   Returns as described for cwal_scope_search_v(),
   and also returns NULL if (!key).

   See cwal_prop_get() for details about the lookup key
   comparison.

   @see cwal_scope_search_kvp_v()
   @see cwal_scope_search_kvp()
*/
cwal_value * cwal_scope_search( cwal_scope * s,
                                int maxDepth,
                                char const * key,
                                cwal_midsize_t keyLen,
                                cwal_scope ** foundIn );

/**
   Identical to cwal_scope_chain_set_with_flags_v(), passing
   CWAL_VAR_F_PRESERVE as the final parameter.
*/
int cwal_scope_chain_set_v( cwal_scope * s, int upToDepth,
                            cwal_value * key, cwal_value * val );

/**
   Sets a property in s or one of its parent scopes. If upToDepth
   is 0 then the property will be set in s, else s and up to
   upToDepth parents will be searched for the key (e.g. a value of
   1 means to check this scope and its parent, but no higher). If upToDepth
   is negative it means "arbitrarily high up in the stack." If
   it is found then it is set in the scope it was found in, else
   it is set in s.

   To unset a key, pass a val of NULL.

   Returns 0 on success. Its error codes and the flags are the same as
   for cwal_prop_set_with_flags_v(), with one exception: if scopes are
   configured to use hashes for property storage then this routine
   will (as of 20191211) return CWAL_RC_IS_VISITING_LIST if the
   property storage hash in which the property would be set is
   currently being iterated over or is otherwise temporarily locked
   against modification.
*/
int cwal_scope_chain_set_with_flags_v( cwal_scope * s, int upToDepth,
                                       cwal_value * k, cwal_value * v,
                                       uint16_t flags );

/**
   Identical to cwal_scope_chain_set_with_flags(), passing
   CWAL_VAR_F_PRESERVE as the final parameter.
*/
int cwal_scope_chain_set( cwal_scope * s, int upToDepth,
                          char const * k, cwal_midsize_t keyLen,
                          cwal_value * v );

/**
   The C-string form of cwal_scope_chain_set_v(), except that only the
   first keyLen bytes of k are considered as the search key.

   See cwal_prop_get() for details about the lookup key
   comparison.
*/
int cwal_scope_chain_set_with_flags( cwal_scope * s, int upToDepth,
                                     char const * k, cwal_midsize_t keyLen,
                                     cwal_value * v, uint16_t flags );

/**
   Copies properties from src to dest, retaining any flags set for
   those properties. If src is-a hashtable, its hash entries are
   used, otherwise if src is a container, those properties are used.

   Returns 0 on success, CWAL_RC_MISUSE if either argument is null,
   CWAL_RC_TYPE if dest is not properties-capable. Returns CWAL_RC_OOM
   if allocation of any resource fails while copying properties. If a
   client has made any properties of the scope "const"
   (CWAL_VAR_F_CONST) then this function will fail with
   CWAL_RC_CONST_VIOLATION if an attempt is made to import a symbol
   with the same key.
*/
int cwal_scope_import_props( cwal_scope * dest, cwal_value * src );

/**
   "Declares" a variable in the given scope. Declaring is almost
   identical to setting (cwal_scope_chain_set() and friends) but fails
   with CWAL_RC_ALREADY_EXISTS if the given entry is already declared
   (or set) in s. In addition, if v==NULL then cwal_value_undefined()
   is used as the default.

   If s is NULL then e's current scope is used. If e is 0 then s's
   engine is used. If both are NULL, CWAL_RC_MISUSE is returned.
       
   Returns CWAL_RC_MISUSE if key is NULL or empty (or otherwise
   starts with a NUL byte).

   Returns CWAL_RC_OOM on allocation error.

   Returns (as of 20191211) CWAL_RC_IS_VISITING if s's property
   storage *object* is currently being iterated over or is otherwise
   locked, and CWAL_RC_IS_VISITING_LIST if s's propert storage *hash*
   is in that state.
*/
int cwal_var_decl_v( cwal_engine * e, cwal_scope * s, cwal_value * key, cwal_value * v,
                     uint16_t flags );
/**
   Functionally identical to cwal_var_decl_v(), but takes a
   C-style string (key) which must be keyLen bytes long.
*/
int cwal_var_decl( cwal_engine * e, cwal_scope * s, char const * key,
                   cwal_midsize_t keyLen, cwal_value * v,
                   uint16_t flags );

/**
   cwal_value_unref() is THE function clients must use for
   destroying values allocated via this framework. It decrements
   the reference count of a cwal_value, cleaning up if needed.

   Whether or not clients must (or should) call this function
   depends on how the values are used. Newly-created values have a
   reference count of 0. This reference count is increased when
   the value is added to a container (as a key _or_ value) or the
   client calls cwal_value_ref(). If a client calls
   cwal_value_ref() then he is obligated to call this function OR
   allow the scope to clean up the value when the scope is
   popped. If a client creates a new value, does not explicitly
   reference, but adds it to a container (implicitly referencing
   it) then he must _NOT_ call cwal_value_unref() (doing so will
   leave a dangling pointer in the container).

   Caveat: unref'ing a STRING value without having explicitly
   referenced it (cwal_value_ref()) can potentially be dangerous
   if string interning is enabled and more than one (shared)
   reference to that string is alive. This does not apply to
   other value types.

   It is never _necessary_ to call this function: if it is not
   called, then the value's owning scope will clean it up whenever
   the scope is cleaned up OR (for newly-allocated values with no
   refcount) during a sweep or vacuum operation (see
   cwal_engine_sweep() and cwal_engine_vacuum()).

   The safest guideline for client usage, unless they really know
   what they're doing and how to use/abuse this properly:

   - DO NOT EVER call this function UNLESS one of the following
   applies:

   A) You have called cwal_value_ref() on that value.

   B) You created the value using cwal_new_TYPE() (or one of the
   various convenience variants) AND it is NOT a CWAL_TYPE_STRING
   value (x-strings and z-strings ARE safe here).

   All other uses, unless the caller is intricately familiar with
   cwal's memory management, "might" be dangerous.

   String interning (if enabled) leaves open a corner case where it is
   not safe to call this on a string (CWAL_TYPE_STRING) value unless
   every instance of that string has been explicitly
   cwal_value_ref()'d string or the client otherwise is very, very
   (almost inconceivably VERY) much certain about its ownership. Note
   that string interning does not apply to x-strings and z-strings, so
   they are "safe" in this context. This does not mean string
   interning is unsafe in general (in normal uses cases it works just
   fine), just that it opens up a corner case involving shared strings
   which does not apply to other types. Specifically: if clients
   ref/unref interned strings 100% symmetrically (meaning always ref
   and always unref), there will will/should be no problems. That is
   not feasible to do in some client code, however, in particular
   where temporary values are involved (which is where interning
   causes the the headaches).

   Clients MUST treat this function as if it destroys v; it has
   semantically the same role as free(3) and v must not be used by
   the client after this function is called.  Likewise, the return
   values may, for essentially all purposes, be ignored by the
   client, but this function returns a value to describe what it
   actually does, the semantics of which are somewhat different
   from the rest of the framework (i.e. non-0 is not necessarily
   an error):

   CWAL_RC_MISUSE if !v.

   CWAL_RC_OK if this function does nothing but that's not an
   error (e.g. if passed a handle to one of the built-in constant
   values).

   CWAL_RC_DESTRUCTION_RUNNING if v is currently being destroyed. This
   result should ONLY be returned while destructing a graph in which v
   has cycles. Client code should never see this unless they are doing
   manual cleanup of container values in the destructors of their own
   custom cwal_native implementations.

   CWAL_RC_HAS_REFERENCES if v was not destroyed because it still has
   pending references.
       
   CWAL_RC_FINALIZED if this function actually finalizes the value
   (refcount drops to (or was) zero).
    
   Implementation notes:

   - This function might recycle v's memory for the next allocation of
   the same value type. Some types are not recycled or are recycled
   differently (namely strings because their allocation mechanism
   limits how well we can recycle them). The interning mechanism,
   however, (if enabled) ensures that we don't need to alloc/free
   strings in many common usage patterns.

   - Note that built-in/constant values do not actually participate in
   reference-counting (see cwal_value_is_builtin()) but, for reasons
   of consistency, should be treated as if they do, and should be
   passed to this function just like any other values (it is a
   harmless no-op).

   @see cwal_value_ref()
   @see cwal_value_unhand()
   @see cwal_refunref()
*/
int cwal_value_unref( cwal_value * v );

/**
   A very close relative of cwal_value_unref(), this
   variant behaves slightly differently:

   - If v is NULL or has no refcount, it is left
   untouched. Builtin values are implicitly covered by this
   condition.

   - If v has a positive refcount, its refcount is reduced by 1.
   If the refcount is still above 0, there are no further
   side-effects. If the refcount drops to 0, v is _reprobated_ in
   its current owning scope. That means that it gets moved into
   the list of values which which can be swept up by
   cwal_engine_sweep() and friends. It will not, however, be
   immediately destroyed by reaching a refcount of 0.

   Returns v with one exception (which will never happen in perfectly
   well-behaved code): if v is not NULL and this function can
   determine that v is no longer valid (e.g. it's officially been
   destroyed, is awaiting destruction, or is sitting in a recycling
   bin) then it will assert() and then return NULL. (The assert() is a
   no-op in non-debug builds.) If it ever returns NULL when passed
   non-NULL then something quite fatal has happened and the app should
   treat it as a cwal-fatal error. That will only happen if the value
   is involved in serious reference mismanagement. Continuing to use
   such a corrupted Value from client code leads to Undefined
   Behaviour.

   The intended use of this function is to "let go" of a value with a
   clean conscience when propagating it, without outright destroying
   it or having to be unduly uncertain about whether it will survive
   the currently-evaluating expression. This allows them to take
   advantage of scope lifetimes and sweep intervals (both of which
   they control) to more closely manage the lifetimes of values which
   are potentially temporary but will possibly be used as result
   values (meaning that they need to survive a bit longer, despite
   having no clear owner-via-reference-count).

   Example:

   @code
   int myfunc( cwal_engine * e, cwal_value ** result ){
   int rc;
   cwal_value * v = 0;
   v = cwal_new_string_value(e, "hi, world...", 12);
   if(!v) return CWAL_RC_OOM;
   cwal_value_ref(v);
   rc = ... do useful work ...;
   if(rc){ // error!
   // probably destroys x (if it's not gained another ref via
   // whatever work we did;
   cwal_value_unref(v);
   }else{
   // remove our reference to v without the possibility of
   // destroying it, so that it has a predictable refcount
   // when the caller gets it...
   *result = cwal_value_unhand(v);
   }
   return rc;
   }
   @endcode

   API change: before 2018-11-24 this function returned void.

   @see cwal_value_ref()
   @see cwal_value_unref()
   @see cwal_refunref()
*/
cwal_value * cwal_value_unhand( cwal_value * v );

#if 0
/**
   A convenience alias for cwal_value_ref().
*/
#define cwal_ref(V) cwal_value_ref(V)
/*int cwal_unref( cwal_value * v );*/

/**
   A convenience alias for cwal_value_unref().
*/
#define cwal_unref(V) cwal_value_unref(V)
/*int cwal_unref( cwal_value * v );*/

/**
   A convenience alias for cwal_value_unhand()
*/
#define cwal_unhand(V) cwal_value_unhand(V)
/*cwal_value * cwal_value_unhand( cwal_value * v );*/
#endif

/**
   Increments v's reference count by 1 unless v is a built-in, in
   which case this is a harmless no-op.

   Returns 0 on success. The error conditions include:

   - CWAL_RC_MISUSE: v is NULL. This is harmless and okay, and lots of
   real client-side code passes a pointer to this routine without
   checking whether it's NULL or not.

   - CWAL_RC_MISUSE or an assert(): v has no scope, which indicates an
   internal error or memory mis-use/corruption (e.g. trying to ref a
   value which is currently undergoing destruction). An assert() will
   be triggered or CWAL_RC_MISUSE will be returned.

   - CWAL_RC_RANGE: incrementing would overflow past compile-time
   boundaries. This limit is, for all but the most intentionally
   malicious of purpsoses, unreachable. If this indeed ever happens
   then an assert() is triggered (in debug builds) and the associated
   cwal_engine is internally flagged as being "fatally dead", which
   will cause some several other functions to fail with this same
   result.

   In practice, the result value is ignored, as cwal will assert()
   here if it detects any serious lifetime mismanagement problems and
   there is no generic recovery strategy from what ammounts to memory
   corruption.

   Note that some built-in/constant values do not actually participate
   in reference-counting but, for reasons of consistency, should be
   treated as if they do, and may be passed to this function just like
   any other values. (See cwal_value_is_builtin() for the list of
   shared/constant values.)

   Claiming a reference point never requires a new allocation.

   Calling this function obligates the client to eventually either
   call cwal_value_unref() or cwal_value_unhand() to release the
   reference OR be content to wait until the value's owning scope
   (eventually) cleans up (at which point this value is freed
   regardless of its reference count).

   @see cwal_value_unref()
   @see cwal_value_unhand()
*/
int cwal_value_ref( cwal_value * v );

/**
   This is a obscure lifetime/cleanup related hack which can sometimes
   be used to clean up temporaries (values with a refcount of 0)
   without affecting non-temps (those with a positive refcount, or
   built-in values). Specifically, it is for cleaning up values which
   _might_ live in a higher scope but are temporaries. In some
   contexts (during cleanup of lower scopes), such values must be
   "kept around" by the engine, reinstated as temporaries (in their
   owning scope) instead of cleaning them up, in order to be able to
   handle value propagation through stack popping.

   Do not use this function without understanding exactly what it is
   for and when it is safe to use. It is often NOT safe to use, but
   only the higher-level environment can know for sure.

   This is functionally equivalent to:

   @code
   cwal_value_ref(v);
   cwal_value_unref(v);
   @endcode

   but is conserably more efficient, in that it avoids any
   intermediate movement of values those routines have to do on
   temporaries. This is (like those functions) a no-op on
   built-in/constant values (and clients should treat those just like
   any other values, so they're safe to pass here).

   The return result should be ignored by clients - it is only
   intended for use by cwal-level test code to ensure its proper
   functioning. ITS RETURN SEMANTICS ARE NOT PART OF THE PUBLIC API,
   but for the curious: it returns 0 (false) if this function has no
   side effects, true (non-0) if v is immediately destroyed by this
   call. **HOWEVER**: this info is of informational purposes only and
   should be ignored outside of test code. It MUST NOT to be used for
   any sort of application logic (e.g. "if not destroyed, unref it,"
   because that type of logic leads to all sorts of pain and suffering
   in cwal). This function MAY change to a void return at some point,
   so don't get used to actually checking the return value.

   Explaining when and why a client would want to do this would
   require that i refine the two and half years of experience which
   went into creating (well, stumbling upon) this (trivial) hack,
   and i'm finding that difficult to do. In short...

   When dealing with values created from sources other than the local
   function, it is, more often than not, not generically safe to unref
   them. In some particular cases (specific to the client application)
   it becomes, as a side effect of the other Value lifetime-related
   machinery, possible to solve the problem of "kill the value or
   not?" by simply adding a reference, then removing the
   reference. The side effect is that temporary values (those with a
   refcount of 0) will be destroyed by the call to cwal_value_unref(),
   but it has no lasting effect on non-temporaries. This routine is
   more efficient, however, in that it avoids the moving around of
   temparies into and out of the this-is-a-temp list (values of
   refcount 0 are internally kept in a separate list to facilitate
   sweeping and re-temp'ing of values in some cases after their
   refcount was formerly positive). While such movement of values is
   O(1), this routine offers a notably faster O(1) and has the same
   effect.

   cwal_value_unhand() is a close cousin of this routine.

   @see cwal_value_unhand()
*/
bool cwal_refunref( cwal_value * v );

/**
   Sets v as the (single) specially-propagating result value for
   e. This is only to be used by keywords which toss a value up
   the call stack, and use non-0 result codes to do so, but are not
   necessarily errors. e.g. return, break, exit.

   If v is not 0, a reference is added to v, held by e.

   If v is 0, any propagating value is removed from the
   propagating value slot.

   Regardless of whether or not v is 0, if e has a prior
   propagating value, it gets unref'd after referencing v (see
   cwal_value_unref()), possibly destroying it immediately.

   Returns v.

   @see cwal_propagating_get()
   @see cwal_propagating_take()
*/
cwal_value * cwal_propagating_set( cwal_engine * e, cwal_value * v );

/**
   Returns the currently specially-propagating value from e, if
   any. Ownership of the value is not modified by this call, and e
   still holds a reference to it (if the value is is not 0).

   The intention is that this value will be set for an as-yet
   unhandled RETURN or BREAK statements, as well as for an EXIT or
   FATAL (which necessarily can't be handled until the app level,
   or it loses its functionality).

   In practice, this routine is generally only used to check whether a
   propagating value has been set up, and cwal_propagating_take() is
   used for taking over ownership of that value.

   @see cwal_propagating_take()
   @see cwal_propagating_set()
*/
cwal_value * cwal_propagating_get( cwal_engine * e );

/**
   Effectively the same as calling cwal_propagating_get() followed by
   cwal_propagating_set(se,0), except that this keeps the pending
   value alive even if its refcount drops to 0. Returns the result of
   the first call. Note that the returned value may very well be a
   temporary awaiting a reference before the next sweep-up.

   @see cwal_propagating_get()
   @see cwal_propagating_set()
*/
cwal_value * cwal_propagating_take( cwal_engine * e );

/**
   This sets the given value to be e's one and only "exception"
   value.  This value is given special treatment in terms of
   lifetime - it wanders up the call stack as scopes are popped,
   until the client calls this again with x==NULL.

   x may be NULL, in which case any pending exception is cleared
   and its handle gets unreferenced (see cwal_value_unref()) and
   CWAL_RC_OK is returned. Otherwise...

   If x is not NULL then this function returns CWAL_RC_EXCEPTION
   on success(!!!) or a different non-0 cwal_rc value on
   error. The justification for this is so that it can be called
   in as the return expression for a callback which is throwing
   the exception. The exception value gets a reference held by the
   engine, and any pending exception is cwal_value_unhand()ed.

   If x is NULL then 0 indicates success.

   Interpretation of the exception value is client-dependent,
   and cwal's only special handling of it is ensuring that
   it survives the ride up a popping scope stack.

   While the exception value may be of any cwal Value type, the
   cwal_exception class is specifically intended for this purpose.

   Typical usage:

   @code
   // assuming we want to keep an existing exception:
   cwal_value * exc = cwal_exception_get(e);
   cwal_value_ref(exc);
   cwal_exception_set(e, 0);
   cwal_value_unhand(exc);

   // Or, if we do not need to keep the old value and we KNOW that
   // nobody is holding a pointer to the exception without also
   // having added a reference, then simply:
   cwal_exception_set(e, 0);
   @endcode

   @see cwal_exception_get()
*/
int cwal_exception_set( cwal_engine * e, cwal_value * x );

/**
   Convenience form of cwal_exception_set() which uses
   cwal_new_stringfv() to create an exception message string for
   the new cwal_exception value (which can be fetched using
   cwal_exception_get()). See cwal_printf() for the supported
   formatting options.

   code is the exception's error code. fmt and following arguments
   are the formatted error message.

   If !*fmt then this call creates an exception value with no
   message part.

   As a special case, certain code values will skip the creation of an
   exception and simply return that code. Currently CWAL_RC_OOM is the
   only such case.
*/
int cwal_exception_setfv(cwal_engine * e, int code, char const * fmt, va_list args);

/**
   Identical to cwal_exception_setfv() but takes its arguments in ellipsis form.
*/
int cwal_exception_setf(cwal_engine * e, int code, char const * fmt, ...);

/**
   If e has a current exception value, it is returned to the
   caller and (possibly) transfered into the calling scope (for
   lifetime/ownership purposes). If not, NULL is returned.

   Note that the lifetime of the exception value is managed internally
   by the engine to ensure that it survives as scopes are popped. If
   the client wants to stop this from happening for a given exception
   value, he should use cwal_exception_set() to set the current
   exception state to 0 (and use cwal_value_ref() to get a reference,
   if needed). That will, if the exception has a reference, keep the
   (previous) current exception rooted in its current scope, from
   which it will wander only if it is later referenced by/via an older
   scope.
*/
cwal_value * cwal_exception_get( cwal_engine * e );
    
/**
   NOT IMPLEMENTED.
    
   Frees any message-related memory owned by err (or shared with it,
   in the case of err->value).

   Returns 0 on success, or CWAL_RC_MISUSE if either paramter is 0.

   After calling this, err contains an empty state and must eventually
   be deallocated using whatever mechanism complements its allocation
   (e.g. do nothing more for stack-allocated objects or those embedded
   in another struct).
*/
/*int cwal_exception_info_clear( cwal_engine * e, cwal_exception_info * err );*/

/* NOT IMPLEMENTED. */
/*int cwal_engine_err_set( cwal_engine * e, cwal_exception_info * err );*/

/**
   Returns a pointer to e's current output handler. For purposes of
   swapping them in and out, the returned value should be
   bitwise-copied for later swapping in via
   cwal_engine_outputer_set().  Do not store its pointer, as the
   result's address is stable for a given cwal_engine instance, but
   its contents may be swapped in and out (if done so carefully).

   This will never return NULL unless e is NULL.

   Remember that cwal_outputer::output may legally be NULL, so don't
   just assume that the returned handler can actually output anything.

   @see cwal_engine_outputer_set()
*/
cwal_outputer const * cwal_engine_outputer_get( cwal_engine const * e );

/**
   If tgt is not NULL, this function bitwise-copies e's current output
   handler to *tgt. Bit-wise copies the replacement, making it the
   new output handler.

   @see cwal_engine_outputer_get()
*/
void cwal_engine_outputer_set( cwal_engine * e,
                               cwal_outputer const * replacement,
                               cwal_outputer * tgt );

/**
   Sends (src,n) through the engine-specified output mechanism
   (specified via its vtab). See cwal_output_f() for the
   semantics. Returns 0 on success:

   CWAL_RC_MISUSE: e or src are 0.

   Any other error code is propagated from the output routine.

   This function is a no-op if n==0.

   TODO? consider making (0==src, 0==n) a heuristic for signaling
   a desire to flush the output.
*/
int cwal_output( cwal_engine * e, void const * src, cwal_size_t n );

/**
   If e's vtab is set up to be able to flush its output channel,
   this calls that function and returns its result. Returns
   CWAL_RC_MISUSE if !e or e is not initialized. Returns 0 on
   success or if there is nothing to do.
*/
int cwal_output_flush( cwal_engine * e );

/**
   printf()-like variant of cwal_output(). See cwal_printf.h for
   the format specifiers (they're pretty much standard, plus some
   extensions inherited from sqlite).
*/
int cwal_outputf( cwal_engine * e, char const * fmt, ... );

/**
   va_list variant of cwal_outputf().
*/
int cwal_outputfv( cwal_engine * e, char const * fmt, va_list args );

/**
   The cwal_new_VALUE() function does not really exist - it is
   here for documentation purposes to consolidate the common
   documentation for the large family of cwal_new_xxx() routines.
   These routines typically come in some variation of these
   three forms:

   1) cwal_value * cwal_new_SOMETHING();
   2) cwal_value * cwal_new_SOMETHING(cwal_engine*);
   3) cwal_SOMETHING * cwal_new_SOMETHING(cwal_engine*, ...);

   The first form is only for types which do not allocate memory,
   meaning types with a known set of constant values (boolean,
   undefined, null).

   The second form is only for types which need no initialization
   parameters, e.g. Objects and Arrays.

   The third form is used by types which require more information for
   their initialization. Most such types represent immutable data,
   with values which cannot be changed for the lifetime of the
   cwal_value handle.

   Ownership of the new returned value is initially held by the
   scope which is active during creationg. A newly-created value
   has a reference count of 0 (not 1, though it was in versions
   prior to 20130522). A value with a refcount of 0 is considered
   a "probationary" value, and has a special status in the
   scope-sweep operations. In short, a sweep operation will free
   up _all_ values with refcount 0 in the scope. If clients need
   to ensure a specific lifetime, they must provide the value with
   a reference. This can happen in one of several ways:

   - Insert the value into a container. e.g. set it as an Object
   key or value, or insert it into an Array.

   - Call cwal_value_ref() to increase the refcount by 1.

   If a Value is ever referenced, perhaps indirectly, from an older
   scope, it is automatically moved into that scope for
   ownership/cleanup purposes. This ensures that Values live as long
   as the last scope which references them, or until they are
   otherwise cleaned up.

   Semantically speaking this function Returns NULL on allocation
   error, but the non-allocating factories never actually allocate
   (and so cannot fail). Nonetheless, all Values returned by
   variations of this function must be treated as if they are an
   allocated Value (this consistency is encouraged to avoid
   clients special-casing code due to a cwal-internal
   implementation detail).

   General rules for the cwal_new_XXX() family of functions (all of which
   point the reader to this function) are:

   - Those which (might) allocate memory take a cwal_engine value as
   their first argument. Non-allocating factories SHOULD be treated as
   if they allocate, e.g. by assuming that they participate in the
   normal reference-counting and de/allocation mechanism (which they
   don't, actually). Note that cwal_new_string() and friends can
   return a re-used pointer for an interned string, and it is
   criticial that the client call cwal_value_ref() to increase the
   reference count by 1 to avoid any problems vis-a-vis string
   interning, and each must be followed by a call to
   cwal_value_unref(). It is worth noting that string interning has
   historically caused much Grief with regards to reference counts,
   but it behaves properly if (and only if) all string-allocating
   client code properly refs/unrefs each returned string (even if it's
   a re-used/interened string). Similarly, cwal_new_string() and
   friends may (depending on build-time options) return
   static/shared/constant memory for length-1 ASCII strings, but those
   should also be treated as if they were made up of
   dynamically-allocated memory.

   - To re-iterate: certain built-in/constant values neither allocate
   nor participate in reference-counting/scope-tracking, but that is
   an internal implementation detail, and clients should treat all
   values as equivalent for memory-management purposes except for
   noted for specific APIs.
       
   - All newly-allocated values initially have a reference count of
   0. Clients must call cwal_value_ref() to claim a reference point,
   and adding values to containers also manipulates their reference
   count. Except for strings, client code may call cwal_value_unref()
   to unreference a newly-created (not yet ref'd) value, possibly
   cleaning it up (depending on other references to the value). For
   strings, due to string interning, unref'ing is extremely unsafe
   unless the client code called cwal_value_ref() on that instance.

   Additional notes and bits of wisdom earned via long-time use of
   this library...

   When inserting new values into containers, the safe/proper thing to
   do is to first reference the value, then perform the insertion,
   then, regardless of whether the insertion worked, unref the
   value. For example:

   @code
   int rc;
   cwal_value * v = cwal_new_integer(myEngine, 42);
   if(!v) return CWAL_RC_OOM;
   cwal_value_ref(v); // always obtain a reference
   rc = cwal_array_append(myArray, v);
   cwal_value_unref(v); // unref after the insertion
   if(rc) return rc;
   // On insertion success, the container now has the only reference
   // to v, and v is still alive. On error, v was cleaned up by
   // our call to cwal_value_unref().
   v = 0;
   @endcode

   If the container insertion succeeds, that container will have
   obtained a reference to the Value. If insertion fails, it will not.
   Either way, our call to cwal_value_unref() removes our local
   reference to the Value, making it _semantically_ illegal for us to
   refer to the that Value again.

   Code like the following will "normally" work but has some
   non-obvious pitfalls which may bite the client mightily somewhere
   down the road:

   @code
   // Expanding on the example above...
   // DO NOT DO THIS... DO NOT DO THIS... DO NOT DO THIS...
   v = cwal_new_integer(myEngine, 42);
   // v has a refcount of 0
   rc = cwal_array_append(myArray, v);
   // On success ^^^^ v has a refcount of 1, on error 0.
   if(rc) cwal_value_unref(v); // cleans up v
   v = 0;
   @endcode

   That would actually work fine (most of the time) for most types,
   but that approach breaks down in conjunction with cwal's
   string-interning feature (and _potential_ future interning/re-use
   features for other Value types (e.g. integers)). In the case of
   interned/reused values, the cwal_new_XXX() call will return an
   existing value but not increase its reference count, meaning that
   any unref in the above block is strictly illegal (because it
   doesn't obtain its own reference via cwal_value_ref()). In
   practice, misuse like the above may not trigger a problem until
   much later (especially when recycling is enabled, as that can
   temporarily hide this type of misuse), and will eventually trigger
   an assert() in cwal.

   Likewise, do not do the following:

   @code
   // Expanding on the example above...
   // DO NOT DO THIS... DO NOT DO THIS... DO NOT DO THIS...
   rc = cwal_array_append(myArray, cwal_new_integer(myEngine, 42));
   @endcode

   The problem there is that if the container insertion fails, we've
   "leaked" the newly-created Value and we have no way of referencing
   that value ever again. That "leaked" value will be cleaned up when
   the current cwal scope (see cwal_scope_push()) is popped. Likewise,
   the "leaked" value, because it has no reference count, would be
   cleaned up by a call to cwal_engine_sweep() or cwal_engine_vacuum()
   if that call were made within the context of the current cwal
   scope. So the value is not "really" leaked, in the classic sense of
   a memory leak, but it is effectively leaked until cwal gets around
   to cleaning it up (which, depending on client-specific usage, might
   not be until the cwal_engine instance is finalized).
*/
void cwal_new_VALUE(cwal_engine * e, ...);

/**
   Creates a new cwal_value from the given boolean value.

   Note that there are only two boolean values and they are singletons
   - the engine never allocates memory for booleans.

   @see cwal_new_VALUE()
*/
cwal_value * cwal_new_bool( int v );

    
/**
   Semantically the same as cwal_new_bool(), but for doubles.

   cwal's numeric-type Values (doubles and integers) are immutable -
   they may never be modified after construction.

   The engine reserves the right to compile certain numeric Values as
   built-in constants. Such values do not require any dynamic memory
   but behave just like normal, dynamically-allocated Values except
   that:

   - All refcount operations on such values are no-ops.

   - All flag-setting operations (e.g. vacuum-proofing) on such values
   are no-ops.

   - All such values are immune to garbage-collection.

   Nonetheless, client code must behave exactly as if such values were
   "normal" values, and all cwal APIs support this (by ignoring, where
   appropriate, operations on built-in values). e.g. client code must
   ref/unref these Values as if they were normal Values, even though
   those operations are no-ops for built-in constants.

   Which, if any, numeric Values are built-in constants may differ in
   any given build of this library. Client code must never attempt to
   apply different logic to built-in values, as that logic may or may
   not apply to any given build.

   @see cwal_new_VALUE()
   @see cwal_new_bool()
*/
cwal_value * cwal_new_double( cwal_engine * e, cwal_double_t v );

/**
   Semantically the same as cwal_new_double(), but for integers.

   @see cwal_new_VALUE()
   @see cwal_new_double()
*/
cwal_value * cwal_new_integer( cwal_engine * e, cwal_int_t v );

/**
   Returns a new "unique" value. Unique values are unusual in that:

   - Their identity is their value.

   - They may optionally wrap a single other value. They acquire a
   reference to that value and unref it up when the Unique is cleaned
   up. The value gets upscoped, if necessary, as the Unique gets
   upscoped (but the reverse is not true: the binding is not
   two-way!).

   - They always resolve to true in a boolean context (i.e. via
   cwal_value_get_bool()).

   - They never compare as equivalent to any value other than
   themselves (not even cwal_value_true(), even though it evaluates to
   true in a boolean context).

   They are intended to be used as opaque sentry value or possibly as
   a sort of enum entry substitute.

   Returns 0 if !e or on OOM, else it returns a new value instance
   with a refcount of 0.

   Sidebar: unique values are not containers, per se, and cannot hold
   any per-instance properties. Nonetheless, unlike most
   non-containers, they may participate in cycles. Also, unlike most
   other types, Uniques have no higher-level class representation
   (i.e. there is no Unique-type counterpart of cwal_object or
   cwal_string).

   @see cwal_unique_wrapped_set()
   @see cwal_unique_wrapped_get()
*/
cwal_value * cwal_new_unique( cwal_engine * e, cwal_value * wrapped );

/**
   If uniqueVal is of type CWAL_TYPE_UNIQUE (was created using
   cwal_new_unique()) and it has a wrapped value, that value is
   returned, otherwise 0 is returned. Ownership of the returned
   value is not modified.

   @see cwal_unique_wrapped_set()
   @see cwal_new_unique()
*/
cwal_value * cwal_unique_wrapped_get( cwal_value const * uniqueVal );

/**
   If uniqueVal is of type CWAL_TYPE_UNIQUE then this function sets
   uniqueVal's wrapped value to w. Any prior wrapped value is unref'd
   and may be cleaned up immediately.

   Returns 0 on success, CWAL_RC_TYPE if uniqueVal is not a
   Unique-type value, CWAL_RC_CYCLES_DETECTED if w==uniqueVal. (Note
   that it does not catch nested cycles-to-self, as those are legal
   (and would take arbitrarily long to find if they weren't).)

   w may be 0, but if the client wants to keep any current wrapped
   value alive, he needs to ensure he's got a reference point for it
   before passing 0 here, as this routine will destroy it if its
   reference count drops to 0 during this call.

   This is a no-op, returning 0, if w is the same value uniqueVal
   already wraps.

   @see cwal_unique_wrapped_get()
   @see cwal_new_unique()

*/
int cwal_unique_wrapped_set( cwal_value * uniqueVal, cwal_value * w );


/**
   Returns the special "null" singleton value.

   See cwal_new_VALUE() for notes regarding the returned value's
   memory.

   @see cwal_new_VALUE()
*/
cwal_value * cwal_value_null(void);

/**
   Returns the special "undefined" singleton value.
   
   See cwal_new_VALUE() for notes regarding the returned value's
   memory.

   @see cwal_new_VALUE()
*/
cwal_value * cwal_value_undefined(void);

/**
   Equivalent to cwal_new_bool(0).

   @see cwal_new_VALUE()
   @see cwal_new_bool()
*/
cwal_value * cwal_value_false(void);

/**
   Equivalent to cwal_new_bool(1).

   @see cwal_new_VALUE()
   @see cwal_new_bool()
*/
cwal_value * cwal_value_true(void);

/**
   Converts the given value to a boolean, using JavaScript semantics depending
   on the concrete type of val:

   undef or null: false
   
   boolean: same
   
   integer, double: 0 or 0.0 == false, else true
   
   object, array, hash, function, unique: true

   tuple: the empty tuple is false, all others are true.

   string: length-0 string is false, else true.

   Returns 0 on success and assigns *v (if v is not NULL) to either 0 or 1.
   On error (val is NULL) then v is not modified.

   In practice this is never used by clients - see
   cwal_value_get_bool().
*/
int cwal_value_fetch_bool( cwal_value const * val, char * v );

/**
   Simplified form of cwal_value_fetch_bool() which returns 0 if val
   is "false" and 1 if val is "truthy". Returns 0 if val is NULL.
*/
bool cwal_value_get_bool( cwal_value const * val );

/**
   Similar to cwal_value_fetch_bool(), but fetches an integer value.

   The conversion, if any, depends on the concrete type of val:

   NULL, null, undefined: *v is set to 0 and 0 is returned.
   
   string, object, array: *v is set to 0 and
   CWAL_RC_TYPE is returned. The error may normally be safely
   ignored, but it is provided for those wanted to know whether a direct
   conversion was possible.

   integer: *v is set to the int value and 0 is returned.
   
   double: *v is set to the value truncated to int and 0 is returned.

   In practice this is never used by clients - see
   cwal_value_get_integer().
*/
int cwal_value_fetch_integer( cwal_value const * val, cwal_int_t * v );

/**
   Simplified form of cwal_value_fetch_integer(). Returns 0 if val
   is NULL.
*/
cwal_int_t cwal_value_get_integer( cwal_value const * val );

/**
   The same conversions and return values as
   cwal_value_fetch_integer(), except that the roles of int/double are
   swapped.

   In practice this is never used by clients - see
   cwal_value_get_double().
*/
int cwal_value_fetch_double( cwal_value const * val, cwal_double_t * v );

/**
   Simplified form of cwal_value_fetch_double(). Returns 0.0 if val
   is NULL.
*/
cwal_double_t cwal_value_get_double( cwal_value const * val );

/**
   Equivalent to cwal_string_value( cwal_new_string(e,str,len) ).

   @see cwal_new_string()
   @see cwal_new_VALUE()
*/
cwal_value * cwal_new_string_value(cwal_engine * e, char const * str,
                                   cwal_midsize_t len);
/**
   Returns a pointer to the NULL-terminated string bytes of str.
   The bytes are owned by string and will be invalided when it
   is cleaned up.

   If str is NULL then NULL is returned. If the string has a length
   of 0 then "" is returned.

   @see cwal_string_length_bytes()
   @see cwal_value_get_string()
   @see cwal_string_cstr2()
*/
char const * cwal_string_cstr(cwal_string const *v);

/**
   Equivalent to cwal_string_cstr(), but if the 2nd argument is not
   NULL, *len is set to the string's length, in bytes.

   @see cwal_string_cstr()
*/
char const * cwal_string_cstr2(cwal_string const *v, cwal_midsize_t * len);

/**
   Case-folds a UTF-8 C-string, placing the result in a new
   cwal_string instance.

   cstr must point to at least cstrLen bytes of valid UTF-8 text. This
   function converts the case of each character to upper or lower (as
   specified by the 5th parameter).

   If the 5th parameter is true (non-0), it performs up-casing, else
   it performs lower-casing. It supports all 1-to-1 case conversions
   and none of the 1-to-N/special-case conversion (such characters are
   left as-is).

   On success *rv is set to a new String value and 0 is returned. On
   error, returns:

   - CWAL_RC_MISUSE if !e, !cstr, or !rv.

   - CWAL_RC_RANGE if folding results in invalid UTF-8 characters.

   - CWAL_RC_OOM on allocation error or if the 3rd parameter exceeds
   cwal's string-length limits. In such cases, *rv may be set to 0 (or
   may be unmodified).

   @see cwal_string_case_fold()
*/
int cwal_utf8_case_fold( cwal_engine * e, char const * cstr,
                         cwal_midsize_t cstrLen,
                         cwal_value **rv,
                         bool doUpper );

/**
   Works just like cwal_utf8_case_fold() with the following
   differences:

   - Case-folded output is appended to buf, which may cause buf->mem
   to get reallocated (and thus any prior pointer to it
   invalidated). To send the output to the start of a re-used buffer,
   rather than appending at the end, set buf->used=0 before calling
   this.

   - If len is 0, this is a no-op: buf is not modified, not even to
   add a NUL byte. (Note that the buffer APIs almost always ensure tha
   buffers get a NUL byte added.)

   - On success, buf->mem gets is NUL-terminated unless len is 0, in
   which case the memory is not modified. On error, no guarantees are
   made as to its NUL-termination.

   - This updates buf->used as it goes. After completion, the "string"
   part of buf is (unless buf->mem previously held non-string data)
   the first buf->used bytes of buf->mem. (Note that that range does
   not include the NUL terminator, but almost all buffer APIs add one
   at buf->mem[buf->used].)

   Returns CWAL_RC_MISUSE if !e, !cstr, !buf, or if cstr is part of
   buf's current memory range. Otherwise it returns as documented for
   cwal_utf8_case_fold().
*/
int cwal_utf8_case_fold_to_buffer( cwal_engine * e, char const * cstr,
                                   cwal_midsize_t len, cwal_buffer *buf,
                                   bool doUpper );


/**
   Searches for the given "needle" in the given "haystack".

   The 1st and 2nd parameters delimit the area to search. The 4th and
   5th specify the thing to look for.

   The 3rd parameter specifies an optional UTF-8 character (not byte!)
   offset within the haystack to start the search. If the offset is
   negative, it is counted as the number of characters from the end of
   the haystack, but does not change the search direction (because
   counting UTF-8 lengths backwards sounds really hard ;).

   Returns a negative value if:

   - No match is found (or cannot be found because, e.g. the needle
   is larger than the haystack) or the haystack appears to not be
   UTF-8.

   - Either string is NULL or either length parameter is 0.

   If a match is found, its position is returned, but its
   interpretation depends on the value of the 6th parameter:

   If returnAsByteOffset is true then the returned value is the byte
   offset of the match, else it is the UTF-8 character offset of the
   match.

   All inputs are assumed to be valid UTF-8 text.
*/
cwal_int_t cwal_utf8_indexof( char const * haystack, cwal_size_t hayByteLen,
                              cwal_int_t charOffset,
                              char const * needle, cwal_size_t needleByteLen,
                              char returnAsByteOffset );

/**
   Identical to cwal_utf8_case_fold() except that it takes its input
   in the form of a cwal_string.

   As a special case, if str is an empty string (length of 0),
   *rv is set to its value part. i.e. the output will be the
   input, but in its alternate type pointer.

   On 20180515 a (cwal_engine*) parameter was added because this
   function otherwise fails when used on built-in static strings
   (length-1 ASCII might, depending on build options, be compiled
   in). We don't internally special-case that because special-casing
   is ugly and because that corner case is a compile-time option which
   cwal_utf.c doesn't know about.

   @see cwal_utf8_case_fold()
*/
int cwal_string_case_fold( cwal_engine * e,
                           cwal_string const * str,
                           cwal_value **rv,
                           bool doUpper );

/**
   This creates a new cwal string Value which copies the first n bytes
   of str. The new string will be NUL-terminated after n bytes,
   regardless of whether the input string is.

   ACHTUNG: prior to 20171005, n=0 meant that this function should use
   cwal_strlen() to determine str's length. That is no longer the
   case, as it caused too much special-case code client-side. n=0 now
   means an empty string (i.e. copying zero bytes from the source).

   If str is NULL or n is 0, this function still returns non-NULL
   value representing the empty string. (The empty string is a
   library-internal constant, shared across all invocations.) This
   function may return shared/constant values for certain strings
   (e.g. the empty string and _possibly_ length-1 ASCII strings). Such
   strings do not actually partake in the lifetime management system
   (e.g. their refcount is always 0) but should, from a client's
   perspective, be treated as if they are normal Values
   (e.g. adding/removing references, even though such is a no-op on
   built-in Values). See cwal_new_double() for more details about
   built-in Values.

   If len is larger than (2^(CWAL_SIZE_T_BITS-3)) then this function
   returns NULL: cwal internally reserves 3 of the size's bits for
   internal information, limiting string lengths to that number of
   bits. (Design note: it's either that or increase the
   sizeof(cwal_string) by another (padded) increment, increasing
   memory costs for all strings.) This length limit is approximately
   8KiB on 16-bit builds (meaning CWAL_SIZE_T_BITS is 16), 512MiB on
   32-bit, and rediculously high on 64-bit. The library reserves the
   right to strip a further bit or two (thereby making 16-bit builds
   unfeasible, but... so what?).

   Returns NULL on allocation error or a range limit violation (see
   above).
   
   See cwal_new_VALUE() for important information about the
   returned memory.

   ACHTUNG: see cwal_value_unref() for important notes involving
   string interning.

   In practice, it's rare for clients to use this function: normally
   cwal_new_string_value() is a more convenient choice.

   @see cwal_string_value()
   @see cwal_new_string_value()
   @see cwal_string_cstr()
   @see cwal_string_cstr2()
*/
cwal_string * cwal_new_string(cwal_engine * e, char const * str,
                              cwal_midsize_t len);
/**
   printf-like form of cwal_new_string(). See cwal_printf() for
   the formatting specifiers.
*/
cwal_string * cwal_new_stringf(cwal_engine * e, char const * fmt, ...);
/**
   printf-like form of cwal_new_string(). See cwal_printfv() for
   the formatting specifiers.
*/
cwal_string * cwal_new_stringfv(cwal_engine * e, char const * fmt, va_list args);

/**
   Creates a new handle for an "x-string" (as in "external"). This
   is different from cwal_new_string() in that it does not copy
   str's bytes. The client must guaranty that len bytes of str are
   valid for at least as long as the returned value is used. i.e.
   this is safe to use on static strings or on buffers which the
   client can guaranty outlive the returned string.

   ACHTUNG: prior to 20171121 this function treated len=0 as a hint to
   use cwal_strlen() to determine the string's length.  It now
   requires len to be the byte length of the input string.

   Returns NULL on error.

   The returned string cannot be differentiated from a non-external
   string using the public API, with the minor exception that calling
   cwal_string_cstr() on the returned string will return the same
   C-string pointer passed to this function unless the string matches
   one of the built-in constant strings.

   Be aware that...

   - See cwal_new_string() for size limitation notes.

   - X-strings might not be NUL-terminated, so routines which
   blindly display strings until the NUL might be surprised by the
   results of doing so with cwal_string_cstr(anXString).

   - Strings shorter than sizeof(char *) are not going to get any
   memory benefits compared to non-X-strings. Use normal strings
   for those.

   - While technically this API allows strings to be non-NUL
   terminated, in practice many C APIs which get bound to scripting
   engines (not just cwal) do not take a length parameter and expect
   their inputs to be NUL-terminated. Thus it is HIGHLY RECOMMENDED
   that clients add a NUL terminator (but don't count the NUL in the
   string's length).

   - X-strings do not partake in string internalization, because doing
   so would potentially invalidate lifetime guarantees. Their "empty
   shells" (all but the external string pointer) participate in
   recycling.

   - X-strings DO partake in the length-0-string optimization, so
   cwal_new_string(e,"",0) and cwal_new_xstring(e,"",0) will
   return the same value (but that's an implementation detail
   clients should not make code-level decisions based on).
       
*/
cwal_string * cwal_new_xstring(cwal_engine * e, char const * str,
                               cwal_midsize_t len);

/**
   Equivalent to passing the return value of
   cwal_new_xstring(e,str,len) to cwal_string_value().
*/
cwal_value * cwal_new_xstring_value(cwal_engine * e, char const * str,
                                    cwal_midsize_t len);


/**
   A "z-string" is closely related to an "x-string" (see
   cwal_new_xstring()) in that the caller allocates the string, but
   (different from x-strings), the caller gives its memory over to a
   new cwal_string value. This can avoid extra copies in some cases,
   e.g. by using cwal_buffer_to_zstring() to construct a string using
   a buffer, then transfering its memory to a Z-string.

   The caller transfers ownership of str to this function, regardless
   of success or failure, and should treat it as if it were
   _immediately_ freed, using the cwal_string APIs to access it
   further.

   Note that transfer of str is only legal if str was allocated by
   the same underlying allocator as the rest of the library
   (i.e. cwal_free(str) must be legal or Undefined Behaviour may
   ensue).

   On success the ownership of str is transfered to the returned
   cwal_string value and it will be freed (via cwal_free()) when the
   cwal_string is freed or even possibly by this function. Thus it is
   critical that clients treat the str memory as invalid after calling
   this, and (to repeat) only use the cwal_string APIs to get its
   string value.

   To simplify usage, if allocation of the new cwal_string fails,
   this function _still_ takes over ownership of the given string
   memory and frees it before returning NULL from this
   call. (Design note: if we did not do this, error checking would
   become more complicated and the caller would have to decide to
   add extra checks or leak.)
       
   ACHTUNG:

   - Prior to 20171121 this function treated len=0 as a hint to use
   cwal_strlen() to determine the string's length. It now requires
   len to be the byte length of the input string.

   - See cwal_new_string() for size limitation notes.

   - z-strings do not participate in string interning, but their
   "empty shells" (and the client-supplied string bytes)
   participate in recycling of some form or another.

   - While technically this API allows strings to be non-NUL
   terminated, in practice many C APIs which get bound to scripting
   engines (not just cwal) do not take a length parameter and expect
   their inputs to be NUL-terminated. Thus it is HIGHLY RECOMMENDED
   that clients add a NUL terminator (but don't count it in the
   string's length).

   - str MUST have been allocated using the same allocator as
   cwal_malloc(e,...)  uses or results are undefined. e.g. memory from
   a cwal_buffer would be safe but memory which can from strdup(),
   malloc(), or similar "might" not be.

   - str's contents MUST NOT be modified after calling this. Doing so
   can lead to very unpredictable behaviour in code using the string
   (e.g. hashing of keys will break). The underlying laws of physics
   cwal is based on assume that string bytes are always immutable.
       
   The term "z-string" refers to a coding convention seen in some
   source tree (not this one) where pointers to strings for which the
   client owns the memory are named with a "z" prefix, e.g. zMyString.

   @see cwal_new_zstring_value()
   @see cwal_new_string()
   @see cwal_new_xstring()
*/
cwal_string * cwal_new_zstring(cwal_engine * e, char * str,
                               cwal_midsize_t len);

/**
   Equivalent to passing the result value of cwal_new_zstring(e,str,len) to
   cwal_string_value().
*/
cwal_value * cwal_new_zstring_value(cwal_engine * e, char * str,
                                    cwal_midsize_t len);

    
    
/**
   Creates a new cwal_string value by concatenating two string
   values. Returns NULL if either argument is NULL or if
   allocation of the new string fails.
*/
cwal_value * cwal_string_concat( cwal_string const * s1, cwal_string const * s2 );

/**
   An enum holding bitmasks for toggleable cwal_engine features.
   See cwal_engine_feature_flags().
*/
enum cwal_engine_features_e {
/** For internal use. All feature flags must have all their bits
    in this range. */
CWAL_FEATURE_MASK = 0xFF00,
/**
   Used in cwal_engine::flag to specify that auto-interning should
   be enabled.

   Reminder to self: these must currently reside in the high byte.
   Need to check/consolidate how the internal flags (low byte)
   are being used.

   ACHTUNG: see cwal_value_unref() for notes involving string
   interning. In short, DO NOT use string interning unless your app
   _always_ refs/unrefs all values (in particular, string values) OR
   doesn't unref any (leaving it to scope-level cleanup -
   _hypothetically_ that's safe, too). Failing to do so can lead to
   corrupting cwal state when interned copies of strings get more
   unrefs than refs (and yet the same number of refs as calls to
   cwal_new_string() and friends).

   When in doubt, leave interning disabled!
*/
CWAL_FEATURE_INTERN_STRINGS = 0x0100,

/**
   Used in cwal_engine::flags to specify that the engine should
   zero out string memory before freeing it.
*/
CWAL_FEATURE_ZERO_STRINGS_AT_CLEANUP = 0x0200,

/**
   Possibly (as yet undecided) deprecated in favor of
   CWAL_OBASE_ISA_HASH. With that build-time option enabled, this flag
   becomes largely obsolete.

   Used in cwal_engine::flags to tell the engine that its scopes
   should create hash tables for property storage, instead of
   Objects. This theoretically costs more memory, but if recycling is
   on then the difference is negligible (in the s2 unit test
   suite). If recycling is off, the memory cost is higher. However,
   hashes are much faster at property lookups when a scope contains
   many variables. Historically, objects have suited this purpose just
   fine.

   Changing this flag at runtime only affects future property storage
   creation, and does not affect scopes which have already allocated
   their scope properties (which they do lazily, only when needed).

   When the library is compiled with the CWAL_OBASE_ISA_HASH
   configuration option then scopes will use a hash either way, but
   the actual concrete type of the storage will differ: it will be
   cwal_object without this flag and cwal_hash with it. Whether that's
   really a feature or a bug is as yet unknown, but the latter enables
   a capability not available to the former: scope-level properties
   which are either exposed to or hidden from the scope lookup
   mechanism, depending on whether they're stored as cwal_hash entries
   or not.
*/
CWAL_FEATURE_SCOPE_STORAGE_HASH = 0x0400
};

/**
   Sets the current set of feature flags and returns the old flags.
   Pass a negative flags value to have it return the current flags
   without setting them. flags is interpreted as a bitmask of
   cwal_engine_features_e values. This may be called during engine
   initialization (via cwal_engine_vtab::hook::on_init()).

   If !e or tracing is disabled at built-time, returns -1.

   Example:

   @code
   // Get current flags:
   uint32_t const flags = cwal_engine_feature_flags(e,-1);
   // Disable string-interning:
   cwal_engine_feature_flags(e, flags & ~CWAL_FEATURE_INTERN_STRINGS );
   @endcode

   Calling this might have side effects other than setting
   the flags. Namely:

   If CWAL_FEATURE_INTERN_STRINGS is disabled by this call (and
   was enabled before it) then the memory used for tracking
   interned strings is released. The strings are left intact and
   unaffected, but future strings with those same values will be
   created anew instead of sharing the interned values. Note that
   interning may be enabled or disabled at any time without any
   adverse effects vis-a-vis string ownership, reference counting,
   etc.

   ACHTUNG:

   One fine debugging session in early 2016 it was discovered that
   string interning (via the CWAL_FEATURE_INTERN_STRINGS flag) has a
   property which makes it dangerous to use if client code uses string
   values without acquiring explicit references to them (e.g. they are
   "temp strings"). It can happen that 2 such strings share an
   interned instance concurrently and one of them is passed to
   cwal_refunref(). That will nuke the shared instance (because it has
   no refcount) but leave one of the 2 client code locations holding a
   stale pointer to it (pointing to memory which might live in the
   recycling bin or might have been deallocated or reallocated for
   another purpose). The only solution for this is to explicitly take
   references everywhere, and release them when done. That's not
   always practical (or fun), however, and the workaround in such
   cases is to disable string interning. For completeness, here is an
   example scenario:

   1) a function uses cwal_new_string[_value]() to create a local temp
   string and does not grab a reference to it (because it's often
   (seemingly) not strictly necessary to). This function calls
   another, which calls another, which...

   2) some downstream call, allocates the same string, which string
   interning, as it's supposed to, doesn't allocate, but returns
   from the interning table (which does _not_ modify its refcount
   because that would make interned strings live forever).

   3) that downstream code, being modern and safe, adds a reference
   when it gets the string and unrefs it when done.

   4) Blamo! When the function from step (1) tries to unref (or
   cwal_refunref()) the string, it will be invoking undefined
   behaviour because the state of the stale pointer is
   indeterminate. By that time, it might have been free()'d, it might
   live in cwal's recycling bin, or it might have been reallocated for
   a different purpose altogether.

   The morale of the story is: always explicitly add/remove references
   and your code will be safe. Failing to do so can lead to
   difficult-to-track disasters. Optionally, disable/do not use string
   interning, and "most" reasonable uses of temp/local strings are
   fine and kosher (provided one does not use cwal_engine_sweep() or
   cwal_engine_vacuum(), which will clean up those temps.

   All that being said: as of 20160111, cwal_new_string() will not
   re-use an interned string which has a refcount. Instead it will go
   through the normal allocation process (which might pull from the
   recycler).
*/
uint32_t cwal_engine_feature_flags( cwal_engine * e, int32_t flags);
    
/**
   Returns the Value associated with s, or NULL if !s.

   @see cwal_new_VALUE()
*/
cwal_value * cwal_string_value(cwal_string const * s);

/**
   Returns the length of str in bytes, or 0 if !str. This is an
   O(1) operation.
*/
cwal_midsize_t cwal_string_length_bytes( cwal_string const * str );

/**
   Returns the length of the first n bytes of str in UTF8
   characters, or 0 if !str. Results are undefined if str is not
   legal UTF8. This is an O(N) operation.

   Note that an embedded NUL byte before (str+n) is counted as a
   byte!
*/
cwal_midsize_t cwal_strlen_utf8( char const * str, cwal_midsize_t n );

/**
   Functionally equivalent to strlen(3) except that if !str it
   returns 0 instead of crashing, and it returns cwal_size_t,
   which very well may not be the same size as size_t.
*/
cwal_midsize_t cwal_strlen( char const * str );

/**
   Equivalent to:

   cwal_strlen_utf8(cwal_string_cstr(str),cwal_string_length_bytes(str))

   Unless str is known to be an ASCII string, in which case it is an
   O(1) operation.

   Returns 0 if !str.
*/
cwal_midsize_t cwal_string_length_utf8( cwal_string const * str );

/**
   If str is composed solely of ASCII characters (in the range
   (0,127), this returns true, else false. While normally of little
   significance, some common algorithms can be sped up notably if
   their input is guaranteed to have only 1 byte per character.

   This immutable flag is set when a string is created.

   Returns 0 if str is NULL or if it is the built-in empty string
   value. Most algorithms avoid all work if the length is zero, so its
   ASCII-ness in that case is irrelevant (and debatable).
*/
bool cwal_string_is_ascii( cwal_string const * str );

/**
   Returns the upper-cased form of the given utf8 character,
   or ch if it doesn't know what to do.

   The mappings cover all the one-to-one mappings defined by
   Unicode:

   https://www.unicode.org/faq/casemap_charprop.html
   ftp://ftp.unicode.org/Public/3.0-Update/UnicodeData-3.0.0.html
   ftp://ftp.unicode.org/Public/UCD/latest/ucd/UnicodeData.txt

   None of the "special cases" are covered.

   @see cwal_utf8_char_tolower()
*/
int cwal_utf8_char_toupper( int ch );

/**
   Returns the lower-cased form of the given utf8 character,
   or ch if it doesn't know what to do.

   @see cwal_utf8_char_toupper()
*/
int cwal_utf8_char_tolower( int ch );
    
/**
   Reads a single UTF-8 character from an input string and returns the
   unicode value.

   zBegin is the start of the string. zTerm points to the logical
   EOF (one-after-the-end). zBegin _must_ be less than zTerm.

   It writes a pointer to the next unread byte back into *pzNext.
   When looping, that value should be the next position passed to
   this function (see the example below).
       
   Notes On Invalid UTF-8:
       
   - This routine never allows a 7-bit character (0x00 through
   0x7f) to be encoded as a multi-byte character.  Any multi-byte
   character that attempts to encode a value between 0x00 and 0x7f
   is rendered as 0xfffd.
       
   - This routine never allows a UTF16 surrogate value to be
   encoded.  If a multi-byte character attempts to encode a value
   between 0xd800 and 0xe000 then it is rendered as 0xfffd.
       
   - Bytes in the range of 0x80 through 0xbf which occur as the first
   byte of a character are interpreted as single-byte characters and
   rendered as themselves even though they are technically invalid
   characters. (That can be considered a bug in the context of cwal!)
       
   - This routine accepts an infinite number of different UTF8
   encodings for unicode values 0x80 and greater.  It does not change
   over-length encodings to 0xfffd as some systems recommend. (That
   can be considered a bug in the context of cwal!)

   - An embedded NUL byte before zTerm is counted as a length-1
   character.

   Credits: the implementation and most of the docs were stolen from
   the public domain sqlite3 source tree.

   Example usage:

   @code
   char unsigned const * pos = inputString;
   char unsigned const * end = pos + inputStringLength;
   char unsigned const * next = 0;
   unsigned int ch;
   for( ; (pos < end)
   && (ch=cwal_utf8_read_char(pos, end, &next));
   pos = next ){
   // do something with ch...
   // but note that 0 is technically a legal value
   // for ch for generic purposes (but not for most practical
   // purposes). The byte-length of the character is (next-pos).
   }
   @endcode

   If zBegin>=zTerm, it returns 0 and *pzNext is set to zEnd.  Note
   that that result is indistinguishable from a NUL-terminated,
   zero-length string (i.e. containing only 1 byte: a NUL), but it's
   as good as we can get for such inputs. The moral is: check the
   input range before starting and do whatever is right for your use
   case.

   @see cwal_utf8_read_char1()
*/
unsigned int cwal_utf8_read_char( const unsigned char *zBegin,
                                  const unsigned char *zTerm,
                                  const unsigned char **pzNext);

/**
   A variant of cwal_utf8_read_char() which assumes that the input
   string is NUL-terminated UTF8. It returns the unicode value of the
   next UTF8 character and assigns *zIn to the first byte following
   that character. For more information about the return value, see
   cwal_utf8_read_char().

   @see cwal_utf8_read_char()
*/
unsigned int cwal_utf8_read_char1(const unsigned char **zIn);

/**
   Given UTF8 character value c, this calculates its length, in bytes,
   writes that many bytes to output, and returns that length. If the
   calculated size is >length then -1 is returned (which cannot happen
   if length>=4). If !output then only the UTF8 length of c is
   calculated and returned (and the length argument is ignored).

   Returns -1 if c is not a valid UTF8 character. The most bytes it
   will ever write to *output is four, so an output buffer of four
   bytes is sufficient for all encoding cases.

   Note that it does not NUL-terminate the output unless the character
   is incidentally a NUL byte. That is the only case in which it
   NUL-terminates the output.

   FIXME: return 1 if !c: a NUL byte has a length of 1. Make sure s2
   callers accommodate this first. Doh, we can't do that if (output)
   is optional, or the caller would not know when to stop!

   FIXME: remove the length param and require at least 4 bytes.

   FIXME?: return -1 for the various "invalid" characters: 0xfffd
   (?undocumented return value of cwal_utf8_read_char()?).

   @see cwal_utf8_char_at()
*/
int cwal_utf8_char_to_cstr(unsigned int c, unsigned char *output, cwal_size_t length);

/**
   Searches the UTF8-encoded string starting at b for the index'th
   UTF8 character. The string in the half-open range [b,e) must be
   well-formed UTF8. If found, 0 is returned and *unicode (if unicode
   is not NULL) is assigned to the code point. If index is out of
   range or invalid UTF8 is traversed, CWAL_RC_RANGE is returned and
   unicode is not modified.

   @see cwal_utf8_char_to_cstr()
*/
int cwal_utf8_char_at( unsigned char const * b,
                       unsigned char const * e,
                       cwal_size_t index,
                       unsigned int * unicode );

#if 0
/* do not use - fundamantally broken interface. */
/**
   A helper to loop over each UTF-8 character of an input string.

   The first two arguments define the current character and the
   one-after-the-end address (pos+strlen(pos)). It is initially
   passed the start of the string, but each iteration should pass
   (previousStart + return value of previous call). See below for
   an example.

   This routine only examines the first byte of *pos, so its
   results depend on pos pointing to the start of a UTF-8
   character boundary.

   On success:

   - If unicode is not 0 then *unicode will holds the UTF-8 encoded
   value of the character pointed to by pos. This increases the cost
   of the calculation notably. Note that this value differs from its
   code point!

   - The _byte_ length of the consumed character is returned. It tries
   to return 0 for invalid characters, but there are plenty of invalid
   UTF-8 characters which it does not currently treat as invalid (for
   fear of breaking existing code), though this behaviour may change
   someday.

   - For the second and subsequent iterations, the first parameter
   must be incremented by the return value of the previous call to
   this function.

   If pos>=end or the character read from pos extends past the
   end, 0 is returned and *unicode is not modified. e.g. if
   pos==(end-1) and it detects a two-byte character then it
   returns 0 rather than setting up the user to dereference
   any bytes past the end.

   It treats embedded NUL bytes (before the end position) as
   length-1 bytes.

   Example intended usage:

   @code
   char const * str = "hi world, in UTF-8";
   char const * pos = str;
   char const * const end = str + cwal_strlen(str); // BYTE length!
   unsigned int unicode = 0;
   int len = 0;
   for( ; (len = cwal_utf8_char_next(pos, end, &unicode))>0;
   pos += len){
   ... use bytes in the range [pos,pos+len) ...
   ... optionally use the unicode value ...
   assert(len>=1 && len<=4); // a NUL byte has a len of 1!
   }
   @endcode

   @see cwal_utf8_read_char()
   @see cwal_utf8_read_char1()
*/
int cwal_utf8_char_next( char const * pos, char const * end,
                         unsigned int * unicode);
#endif
    
/**
   Equivalent to cwal_value_unref( e, cwal_string_value(v) ),
   where e is s's owning cwal_engine.
*/
int cwal_string_unref(cwal_string * s);

/**
   If cwal_value_is_string(val) then this function assigns *str to the
   contents of the string. str may be NULL, in which case this function
   functions like cwal_value_is_string() but returns 0 on success.

   Returns 0 if val is-a string, else non-0, in which case *str is not
   modified.

   The bytes are owned by the given value and may be invalidated in any of
   the following ways:

   - The value is cleaned up or freed.

   - An array or object containing the value peforms a re-allocation
   (it shrinks or grows).

   And thus the bytes should be consumed before any further operations
   on val or any container which holds it.

   Note that this routine does not convert non-String values to their
   string representations. (Adding that ability would add more
   overhead to every cwal_value instance.)

   In practice this is never used by clients - see
   cwal_value_get_string().
*/
int cwal_value_fetch_string( cwal_value const * val, cwal_string ** dest );

/**
   Simplified form of cwal_value_fetch_string(). Returns NULL if val
   is-not-a string value.
*/
cwal_string * cwal_value_get_string( cwal_value const * val );

/**
   Convenience function which returns the string bytes of the
   given value if it is-a string or a buffer, otherwise it returns
   NULL. Note that this does no conversion of other types to
   strings, and returns NULL for them.

   The second argument, if not NULL, is set to the length of the
   string or buffer, in bytes (not UTF8 characters). For buffers, this
   value corresponds to their "used" property. If the 2nd argument is
   NULL, it is ignored.

   Using this for buffer values "might" (depending on the contents of
   the buffer and the intended use of the returned bytes) lead to
   undefined behaviour if the returned string is expected to contain
   valid string data (buffers can contain anything).

   The returned bytes are owned by the underlying value. In the case
   of strings their address and contents remain constant for the life
   of the string value. For buffers the contents and address may
   change at any time, so it is illegal to use the returned bytes if
   there is any chance that the buffer which owns/owned them has been
   modified _in any way_ since calling this.

   As a special case, a completely empty buffer value, with no
   buffered memory, will return 0 here and len (if not NULL) will be
   set to 0. An empty string, on the other hand, will return ""
   and set len (if not NULL) to 0.
*/
char const * cwal_value_get_cstr( cwal_value const * val, cwal_size_t * len );


/**
   Allocates a new "array" value and transfers ownership of it to the
   caller. It must eventually be destroyed, by the caller or its
   owning container, by passing it to cwal_value_unref().

   Returns NULL on allocation error.

   Post-conditions: cwal_value_is_array(value) will return true.

   @see cwal_new_object_value()
   @see cwal_new_VALUE()
*/
cwal_value * cwal_new_array_value( cwal_engine * e );

/**
   Convenience form of cwal_new_array_value() which returns its
   result as an array handle.

   Postconditions: cwal_array_value(result) is the value which would
   have been returned had the client called cwal_new_array_value()
   instead of this function.
*/
cwal_array * cwal_new_array(cwal_engine *e);

/**
   Equivalent to cwal_value_unref( cwal_array_value(v) ).
*/
int cwal_array_unref(cwal_array *a);

/**
   Identical to cwal_value_fetch_object(), but works on array values.

   In practice this is never used by clients - see
   cwal_value_get_array().

   @see cwal_value_get_array()
   @see cwal_value_array_part()
*/
int cwal_value_fetch_array( cwal_value const * val, cwal_array ** ar);

/**
   Simplified form of cwal_value_fetch_array(). Returns NULL if val
   is-not-a array value.
*/
cwal_array * cwal_value_get_array( cwal_value const * v );

/**
   The inverse of cwal_value_get_array().
*/
cwal_value * cwal_array_value(cwal_array const * s);

/**
   Sets the given index of the given array to the given value
   (which may be NULL).

   If ar already has an item at that index then it is cleaned up and
   freed before inserting the new item.

   ar is expanded, if needed, to be able to hold at least (ndx+1)
   items, and any new entries created by that expansion are empty
   (NULL values).

   On success, 0 is returned and ownership of v is transfered to ar.
  
   On error ownership of v is NOT modified, and the caller may still
   need to clean it up. For example, the following code will introduce
   a leak if this function fails:

   Fails with CWAL_RC_LOCKED if the array is currently locked against
   modification (e.g. while the list is being sorted).

   @code
   cwal_array_append( myArray, cwal_new_integer(42) );
   @endcode

   Because the value created by cwal_new_integer() has no owner
   and is not cleaned up. The "more correct" way to do this is:

   @code
   cwal_value * v = cwal_new_integer(42);
   int rc = cwal_array_append( myArray, v );
   if( 0 != rc ) {
   cwal_value_unref( v );
   ... handle error ...
   }
   @endcode

*/
int cwal_array_set( cwal_array * const ar, cwal_midsize_t ndx, cwal_value * const v );

/**
   Ensures that ar has allocated space for at least the given
   number of entries. This never shrinks the array and never
   changes its logical size, but may pre-allocate space in the
   array for storing new (as-yet-unassigned) values.

   Returns 0 on success, or non-zero on error:

   - If ar is NULL: CWAL_RC_MISUSE

   - If allocation fails: CWAL_RC_OOM
*/
int cwal_array_reserve( cwal_array * ar, cwal_midsize_t size );

/**
   Sets the length of the given array to n, allocating space if
   needed (as for cwal_array_reserve()), and unreferencing
   truncated objects. New entries will have NULL values.

   It does not free the underlying array storage but may free
   objects removed from the array via shrinking. i.e. this is not
   guaranteed to free all memory associated with ar's storage.

   Fails (as of 20191211) with CWAL_RC_IS_VISITING_LIST if called
   while traversing over the elements using one of the various
   list-traversal/sort APIs. i.e. the length may not be modified
   while a sort or list iteration is underway. (It "could" be made to
   work for iteration, but resizing while sorting would be
   catastrophic.)

   Fails with CWAL_RC_LOCKED if the list is currently locked (e.g.
   being sorted).
*/
int cwal_array_length_set( cwal_array * ar, cwal_midsize_t n );

/**
   Simplified form of cwal_array_length_fetch() which returns 0 if ar
   is NULL.
*/
cwal_midsize_t cwal_array_length_get( cwal_array const * ar );

/**
   If ar is not NULL, sets *v (if v is not NULL) to the length of the array
   and returns 0. Returns CWAL_RC_MISUSE if ar is NULL.
*/
int cwal_array_length_fetch( cwal_array const * ar, cwal_midsize_t* v );

/**
   Simplified form of cwal_array_value_fetch() which returns NULL if
   ar is NULL, pos is out of bounds or if ar has no element at that
   position.
*/
cwal_value * cwal_array_get( cwal_array const * ar, cwal_midsize_t pos );

/**
   "Takes" the given index entry out of the array and transfers
   ownership to the caller. Its refcount IS decremented by this,
   but using cwal_value_unhand() instead of cwal_value_unref(),
   so it won't be destroyed as a result of this call but may
   once again be a temporary.

   Achtung: if the returned value is reprobated by this function
   (it only does that if it holds the only reference) then as long
   as it has a refcount of 0, a call to cwal_scope_sweep() _will_
   destroy it out from under the client. So take a reference (or
   pass it to a container) if needed.
*/
cwal_value * cwal_array_take( cwal_array * ar, cwal_size_t pos );

/**
   If ar is-a array and is at least (pos+1) entries long then *v
   (if v is not NULL) is assigned to the value at that position
   (which may be NULL).

   Ownership of the *v return value is unchanged by this
   call. (The containing array may share ownership of the value
   with other containers.)

   If pos is out of range, non-0 is returned and *v is not
   modified.

   If v is NULL then this function returns 0 if pos is in bounds,
   but does not otherwise return a value to the caller.

   In practice this is never used by clients - see
   cwal_array_get().
*/
int cwal_array_value_fetch( cwal_array const * ar, cwal_size_t pos, cwal_value ** v );

/**
   Searches for a value in an Array.

   If ar contains v or a value which compares equivalent to v
   using cwal_value_compare(), it returns 0 and sets *index to the
   index the value is found at if index is not NULL.  Returns
   CWAL_RC_NOT_FOUND if no entry is found, CWAL_RC_MISUSE if ar is
   NULL. v may be NULL, in which case it searches for the first
   index in the array with no value in it (it never calls
   cwal_value_compare() in that case).

   If strictComparison is true (non-0), values are only compared if
   they have the same type. e.g. an integer will never match a double
   if this flag is true, but they may match if it is false. If v is
   NULL, this flag has no effect, as only NULL will compare equivalent
   to NULL.

   The cwal_value_compare() (if any) is done with v as the
   left-hand argument and the array's entry on the right.
*/
int cwal_array_index_of( cwal_array const * ar, cwal_value const * v,
                         cwal_size_t * index, char strictComparison );

/**
   Appends the given value to the given array. On error, ownership
   of v is not modified. Ownership of ar is never changed by this
   function. v may be NULL.

   This is functionally equivalent to
   cwal_array_set(ar,cwal_array_length_get(ar),v), but this
   implementation has slightly different array-preallocation policy
   (it grows more eagerly).
   
   Returns 0 on success, non-zero on error. Error cases include:

   - ar is NULL: CWAL_RC_MISUSE

   - Array cannot be expanded to hold enough elements: CWAL_RC_OOM.

   - Appending would cause a numeric overlow in the array's size:
   CWAL_RC_RANGE.  (However, you'll get an CWAL_RC_OOM long before
   that happens!)

   - CWAL_RC_LOCKED if the array is currently locked against
   modification (e.g. while the list is being sorted).

   On error ownership of v is NOT modified, and the caller may still
   need to clean it up. See cwal_array_set() for the details.
*/
int cwal_array_append( cwal_array * const ar, cwal_value * const v );

/**
   The opposite of cwal_array_append(), this prepends a value
   (which may be NULL) to the start of the array.

   This is a relatively expensive operations, as existing entries
   in the array all have to be moved to the right.

   Returns 0 on success, non-0 on error (see cwal_array_append()).
*/
int cwal_array_prepend( cwal_array * const ar, cwal_value * const v );

/**
   "Shifts" the first item from the given array and assigns
   it to *rv (if rv is not NULL).

   Returns 0 on success, CWAL_RC_MISUSE if !ar, and CWAL_RC_RANGE
   if ar is empty. On error, *rv is not modified.

   The array's reference to *rv is removed but if rv is not NULL,
   then then *rv is not immediately destroyed if its refcount goes
   to zero. Instead, it is re-probated in its owning scope. If rv
   is NULL then the shifted value may be reaped immediately
   (before this function returns). i.e. the effect is as if it has
   been cwal_value_unhand()'d, as opposed to cwal_value_unref()'d.
*/
int cwal_array_shift( cwal_array * ar, cwal_value **rv );

/**
   Copies a number of elements from ar into another array.  If
   !*dest then this function creates a new array and, on success,
   updates *dest to point to that array. On error *dest's
   ownership is not modified.

   Copies (at most) 'count' elements starting at the given
   offset. If 0==count then it copies until the end of the array.

   If count is too large for the array, count is trimmed to fit
   within bounds.

   If ar is empty or offset/count are out of range, it still
   creates a new array on success, to simplify/unify client-side
   usage.
       
   Returns 0 on success, CWAL_RC_MISUSE if !ar, !dest, or
   (ar==*dest). Returns CWAL_RC_OOM for any number of potential malloc
   errors. Returns (as of 20191212) CWAL_RC_LOCKED if either ar or
   a non-NULL *dest are currently locked.
*/
int cwal_array_copy_range( cwal_array * ar, cwal_size_t offset,
                           cwal_size_t count,
                           cwal_array **dest );
        
    
/**
   Clears the contents of the array, optionally releasing the
   underlying list as well (if freeList is true). If the list is
   not released it is available for re-use on subsequent
   insertions. If freeProps is true then key/value properties are
   also removed from ar, else they are kept intact.
*/
void cwal_array_clear( cwal_array * ar, char freeList, char freeProps );
    
/**
   A callback type for "value visitors," intended for use
   with cwal_array_visit() and friends.

   The 1st parameter is the value being visited (it MAY BE NULL for
   containers which may hold NULL values, e.g. arrays or tuples). The
   2nd is the state parameter passed to the visiting function
   (e.g. cwal_array_visit()).

   Implementations MUST NOT unref v unless they explicitly ref it
   first. The container which calls this callback holds at least
   one reference to each value passe here.

   20191211: since when do we pass on NULLs in cwal_array_visit()?
*/
typedef int (*cwal_value_visitor_f)( cwal_value * v, void * state );
    
/**
   A callback type for "array visitors," for use with cwal_array_visit2().

   The first 3 parameters specify the array, value, and index of that
   value in the array. The value may be NULL.
       
   The state parameter is that passed to cwal_array_visit2() (or
   equivalent).

   Implementations MUST NOT unref v unless the also ref it first, but
   may indirectly do so by re-assigning that entry in a.

   Note that changing a list's size while visiting is not allowed, and
   attemping to do so will trigger an error.
*/
typedef int (*cwal_array_visitor_f)( cwal_array * a, cwal_value * v, cwal_size_t index, void * state );

/**
   For each entry in the given array, f(theValue,state) is called.  If
   it returns non-0, looping stops and that value is returned to the
   caller.
       
   When traversing containers (this applies to Objects as well), they
   have a flag set which marks them as "being visited" for the
   moment. Prior to 20191211, concurrent visits were never allowed,
   but they now are, with some restrictions. Some APIs temporarily
   lock lists against traversal, which triggers the result code
   CWAL_RC_LOCKED from other APIs (such as this one).

   Returns 0 on success (note that having no entries is not an
   error).

   Minor achtung: entries in the array which have no Value in them are
   passed to the visitor as NULL. Script-side visitors may need to
   check for that and pass on cwal_value_undefined() or
   cwal_value_null() in its place (or skip them altogether). They are
   passed to the visitor so that the number of visits matches the
   indexes of the array, which simplifies some script code compared to
   this routine skipping NULL entries. (BTW, it _did_ skip over NULLs
   until 20160225, at which point the older, arguable behaviour was
   noticed and changed. It was likely an artifact of keeping th1ish
   happy.)
*/
int cwal_array_visit( cwal_array * const a, cwal_value_visitor_f const f, void * const state );

/**
   An alternative form of cwal_array_visit() which takes a
   different kind of callback. See cwal_array_visit() and
   cwal_array_visitor_f() for the semantics.
*/
int cwal_array_visit2( cwal_array * const a, cwal_array_visitor_f const f, void * const state );

/**
   Runs a qsort(2) on ar, using the given comparison function. The
   values passed to the comp routine will be a pointer to either
   (cwal_value const *) or NULL (empty array elements are NULL
   unless the client populates them with something else).

   Returns 0 on success, and the error conditions are quite
   limited:
       
   This function will fail with CWAL_RC_MISUSE if either argument
   is NULL.

   Fails with CWAL_RC_IS_VISITING_LIST if called while traversing over the
   elements using one of the various traversal APIs. (This was result
   code CWAL_RC_IS_VISITING prior to 20191211.)

   Fails with CWAL_RC_LOCKED if the list is currently locked.

   @see cwal_compare_value_void()
   @see cwal_compare_value_reverse_void()
   @see cwal_array_reverse()
*/
int cwal_array_sort( cwal_array * const ar, int(*comp)(void const *, void const *) );

/**
   Reverses the order of all elements in the array. Returns 0 on success,
   non-zero on error:

   - CWAL_RC_MISUSE if !ar.

   While this is technically legal during traversal of a list, the
   results may cause undue confusion.

   @see cwal_array_sort()
*/
int cwal_array_reverse( cwal_array * ar );
    
/**
   A comparison function for use with cwal_array_sort() which
   requires that lhs and rhs be either NULL or valid cwal_value
   pointers. It simply casts the arguments and returns
   the result of passing them to cwal_value_compare().

   @see cwal_array_sort()
*/
int cwal_compare_value_void( void const * lhs, void const * rhs );

/**
   A comparison function for use with cwal_array_sort() which
   requires that lhs and rhs be either NULL or valid cwal_value
   pointers. It simply casts the arguments, calls
   cwal_value_compare(), and returns that result, negated if it is
   not 0.
*/
int cwal_compare_value_reverse_void( void const * lhs, void const * rhs );
    
/**
   A cwal_value comparison function intended for use with
   cwal_array_sort_stateful(). Implementations must compare the given
   lhs/rhs values and return an integer using memcmp() semantics.

   The lhs and rhs value are the left/right values to compare (either
   or both _MAY_ be NULL). These arguments "should" be const but for
   the intended uses of this callback it would be impossible for the
   client to ensure that the constness is not (by necessity) cast
   away. Results are undefined if any sort-relevant state of lhs or
   rhs is modified during the sorting process.

   The state argument is provided by the caller of
   cwal_array_sort_stateful().

   Implementations must set *errCode to 0 on success and non-0
   (preferably a CWAL_RC_xxx value) on error. This can be used to
   propagate non-exception errors back through the sorting process
   (e.g.  script-engine syntax errors or interrupt handling errors).
   The cwal API guarantees that errCode will not be NULL if this
   callback is called from within the cwal API.
*/
typedef int (*cwal_value_stateful_compare_f)( cwal_value * lhs, cwal_value * rhs,
                                              void * state, int * errCode );

/**
   An array sort variant which allows the client to provide a
   stateful comparison operation. The intended use for this is in
   providing script-side callback functions for the sorting, where
   cmp would be a native wrapper around a cwal_function (the state
   param) and would call that function to perform the comparison.

   The exact semantics of the state parameter depend entirely on
   the cmp implementation - this function simply passes the state
   on to the comparison function.

   Returns 0 on success, or CWAL_RC_MISUSE if either !ar or !cmp.  If
   any sort-internal call to cmp() sets its final parameter (error
   code pointer) to a non-0 value, sorting is aborted and that error
   code is returned. This function returns 0 without side-effects if
   the length of the given array (see cwal_array_length_get()) is 0 or
   1.

   Fails with CWAL_RC_IS_VISITING_LIST if called while traversing over
   the elements using one of the various traversal APIs: it is illegal
   to sort an array while it is being iterated over. Likewise, it is
   illegal to iterate over an array from a comparison function while
   sorting is underway (because the results would be highly
   unpredictable). (This was code CWAL_RC_IS_VISITING prior to
   20191211.)

   Fails with CWAL_RC_LOCKED if the list is currently locked.

   If the sorting process does not return an error but the underlying
   cwal engine has, after sorting is complete, an exception awaiting
   propagation, CWAL_RC_EXCEPTION is returned. This is largely
   historical behaviour, from before the time when
   cwal_value_stateful_compare_f was capable of returning error codes.
   It may, in some unusual cases, be necessary for the client to
   ensure that no exception is propagating before calling this (by
   calling cwal_exception_set() with a NULL exception). In practice,
   sorting cannot be triggered during exception propagation, so this
   is really a non-issue unless the client is doing some truly odd
   error handling.
*/
int cwal_array_sort_stateful( cwal_array * const ar,
                              cwal_value_stateful_compare_f cmp,
                              void * state );

/**
   A wrapper around cwal_array_sort_stateful() which calls the
   given comparison function to perform the sorting comparisons.
   The function must accept two (cwal_value*) arguments, compare
   them using whatever heuristic it prefers, and "return" (to
   script-space) an integer value with memcmp() semantics.  Note
   that cwal_new_integer() does not allocate for the values (-1,
   0, 1), so implementations should use those specific values for
   their returns.

   The self parameter specifies the "this" object for the function
   call. It may be NULL, in which case cwal_function_value(cmp)
   is used.

   Fails with CWAL_RC_IS_VISITING_LIST if called while traversing over
   the elements using one of the various traversal APIs. (This was
   result code CWAL_RC_IS_VISITING prior to 20191211.)

   Fails with CWAL_RC_LOCKED if the list is currently locked.
*/
int cwal_array_sort_func( cwal_array * ar, cwal_value * self, cwal_function * cmp );
    
    
/**
   Identical to cwal_new_array_value() except that it creates
   an Object.

   @see cwal_new_VALUE()
*/
cwal_value * cwal_new_object_value( cwal_engine * e );

/**
   Identical to cwal_new_object_value() except that it returns
   the object handle which can converted back to its value
   handle using cwal_value_get_object().
*/
cwal_object * cwal_new_object(cwal_engine *e);

/**
   Equivalent to cwal_value_unref( e, cwal_object_value(v) ).
*/
int cwal_object_unref(cwal_object *v);

/**
   If cwal_value_is_object(val) then this function assigns *obj to the underlying
   object value and returns 0, otherwise non-0 is returned and *obj is not modified.

   obj may be NULL, in which case this function works like cwal_value_is_object()
   but with inverse return value semantics (0==success) (and it's a few
   CPU cycles slower).

   The *obj pointer is owned by val, and will be invalidated when val
   is cleaned up.

   Achtung: for best results, ALWAYS pass a pointer to NULL as the
   second argument, e.g.:

   @code
   cwal_object * obj = NULL;
   int rc = cwal_value_fetch_object( val, &obj );

   // Or, more simply:
   obj = cwal_value_get_object( val );
   @endcode

   In practice this is never used by clients - see
   cwal_value_get_object().

   @see cwal_value_get_object()
*/
int cwal_value_fetch_object( cwal_value const * val, cwal_object ** ar);

/**
   Simplified form of cwal_value_fetch_object(). Returns NULL if val
   is-not-a object value.
*/
cwal_object * cwal_value_get_object( cwal_value const * v );

/**
   The Object form of cwal_string_value(). See that function
   for full details.
*/
cwal_value * cwal_object_value(cwal_object const * s);

/**
   Fetches a property from a child (or [great-]*grand-child) object.

   obj is the object to search.

   path is a delimited string, where the delimiter is the given
   separator character.

   This function searches for the given path, starting at the given object
   and traversing its properties as the path specifies. If a given part of the
   path is not found, then this function fails with CWAL_RC_NOT_FOUND.

   If it finds the given path, it returns the value by assiging *tgt
   to it.  If tgt is NULL then this function has no side-effects but
   will return 0 if the given path is found within the object, so it can be used
   to test for existence without fetching it.
    
   Returns 0 if it finds an entry, CWAL_RC_NOT_FOUND if it finds
   no item, and any other non-zero error code on a "real" error. Errors include:

   - obj or path are NULL: CWAL_RC_MISUSE.
    
   - separator is 0, or path is an empty string or contains only
   separator characters: CWAL_RC_RANGE.

   - There is an upper limit on how long a single path component may
   be (some "reasonable" internal size), and CWAL_RC_RANGE is
   returned if that length is violated.

    
   Limitations:

   - It has no way to fetch data from arrays this way. i could
   imagine, e.g. a path of "subobj.subArray.0" for
   subobj.subArray[0], or "0.3.1" for [0][3][1]. But i'm too
   lazy/tired to add this.

   Example usage:
    

   Assume we have a JSON structure which abstractly looks like:

   @code
   {"subobj":{"subsubobj":{"myValue":[1,2,3]}}}
   @endcode

   Out goal is to get the value of myValue. We can do that with:

   @code
   cwal_value * v = NULL;
   int rc = cwal_prop_fetch_sub( object, &v, "subobj.subsubobj.myValue", '.' );
   @endcode

   Note that because keys in JSON may legally contain a '.', the
   separator must be specified by the caller. e.g. the path
   "subobj/subsubobj/myValue" with separator='/' is equivalent the
   path "subobj.subsubobj.myValue" with separator='.'. The value of 0
   is not legal as a separator character because we cannot
   distinguish that use from the real end-of-string without requiring
   the caller to also pass in the length of the string.
   
   Multiple successive separators in the list are collapsed into a
   single separator for parsing purposes. e.g. the path "a...b...c"
   (separator='.') is equivalent to "a.b.c".

   TODO: change last parameter to an int to support non-ASCII
   separators. s2's string.split() code is close to what we need here.

   @see cwal_prop_get_sub()
   @see cwal_prop_get_sub2()
*/
int cwal_prop_fetch_sub( cwal_value * obj, cwal_value ** tgt, char const * path,
                         char separator );

/**
   Similar to cwal_prop_fetch_sub(), but derives the path separator
   character from the first byte of the path argument. e.g. the
   following arg equivalent:

   @code
   cwal_prop_fetch_sub( obj, &tgt, "foo.bar.baz", '.' );
   cwal_prop_fetch_sub2( obj, &tgt, ".foo.bar.baz" );
   @endcode
*/
int cwal_prop_fetch_sub2( cwal_value * obj, cwal_value ** tgt, char const * path );

/**
   Convenience form of cwal_prop_fetch_sub() which returns NULL if the given
   item is not found.
*/
cwal_value * cwal_prop_get_sub( cwal_value * obj, char const * path, char sep );

/**
   Convenience form of cwal_prop_fetch_sub2() which returns NULL if the given
   item is not found.
*/
cwal_value * cwal_prop_get_sub2( cwal_value * obj, char const * path );

    
/**
   Returns v's reference count, or 0 if !v.
*/
cwal_refcount_t cwal_value_refcount( cwal_value const * v );

/**
   Typedef for generic visitor functions for traversing Objects.  The
   first argument holds they key/value pair and the second holds any
   state passed to cwal_props_visit_kvp() (and friends).

   If it returns non-0 the visit loop stops and that code is returned
   to the caller.

   Implementations MUST NOT unref the key/value parts of kvp. They are
   owned by (or shared with) the kvp object.

   TODO (20160205): consider making kvp non-const, as clients could
   hypothetically safely use cwal_kvp_value_set() on them during
   visitation.
*/
typedef int (*cwal_kvp_visitor_f)( cwal_kvp const * kvp, void * state );

/**
   Returns the key associated with the given key/value pair,
   or NULL if !kvp. The memory is owned by the object which contains
   the key/value pair, and may be invalidated by any modifications
   to that object.
*/
cwal_value * cwal_kvp_key( cwal_kvp const * kvp );

/**
   Returns the value associated with the given key/value pair,
   or NULL if !kvp. The memory is owned by the object which contains
   the key/value pair, and may be invalidated by any modifications
   to that object.
*/
cwal_value * cwal_kvp_value( cwal_kvp const * kvp );

/**
   Re-assigns the given kvp's value part.

   On success this adds a ref point to v and unrefs the old value,
   which may destroy the old value immediately.

   Returns 0 on success, non-0:

   - CWAL_RC_MISUSE if either argument is NULL. Values may not
   be unset by passing a NULL 2nd argument.

   Because keys are used in hashing and sorting, they may not be
   modified, so there is no cwal_kvp_key_set() routine.

   Achtung: this does not honor any CWAL_VAR_F_CONST flag set on kvp,
   but cwal_value_kvp_set2() does.

   ACHTUNG: because this routine is not guaranteed to be able to know
   which scope is correct for v, clients should, on success,
   cwal_value_rescope() v to the scope which owns the container which
   kvp came from (which will move it up only if needed). Do not
   rescope it if this fails or it might end up being stranded in a
   higher-up scope.

   @see cwal_value_kvp_set2()
*/
int cwal_kvp_value_set( cwal_kvp * const kvp, cwal_value * const v );

/**
   Similar to cwal_kvp_value_set() but honors CWAL_VAR_F_xxx flags set
   on kvp, such that it can return additional error codes:

   - CWAL_RC_CONST_VIOLATION if the CWAL_VAR_F_CONST flag is set.
   As a special case (to keep th1ish working :/), if the const flag
   is set but kvp's value == v (as in, the same C pointer, not cwal_value
   equivalence), 0 is returned.

   Unfortuntately, as this routine does not know which container kvp
   belongs to, it cannot enforce
   CWAL_CONTAINER_DISALLOW_NEW_PROPERTIES and
   CWAL_CONTAINER_DISALLOW_PROP_SET. Likewise, it cannot ensure that v
   is in the scope it needs to be in, so the caller should, for
   safety's sake, rescope v to the underlying container's scope (see
   cwal_value_rescope()).

   @see cwal_value_kvp_set()
*/
int cwal_kvp_value_set2( cwal_kvp * const kvp, cwal_value * const v );

/**
   Returns kvp's flags, or 0 if !kvp. Flags are typically
   set via cwal_var_decl() and friends.

   @see cwal_var_flags
*/
cwal_flags16_t cwal_kvp_flags( cwal_kvp const * kvp );

/**
   Sets kvp's flags to the given flags and returns the old
   flags. Only do this if you are absolutely certain of what
   you are doing and the side-effects it might have.

   As a special case, if flags is CWAL_VAR_F_PRESERVE then
   the old value is retained.

   @see cwal_var_flags
*/
cwal_flags16_t cwal_kvp_flags_set( cwal_kvp * kvp, cwal_flags16_t flags );


/**
   Clears all properties (set via the cwal_prop_set() family of
   functions) from the given container value. Returns 0 on
   success, or:

   - CWAL_RC_MISUSE if either c is NULL or its associated
   engine cannot be found (indicative of an internal error).

   - If c is not a type capable of holding properties, CWAL_RC_TYPE is
   returned.

   - CWAL_RC_DISALLOW_PROP_SET if the CWAL_CONTAINER_DISALLOW_PROP_SET
   flag is set on the value (via cwal_container_flags_set()).
*/
int cwal_props_clear( cwal_value * c );

#if 0
/* This one needs better definition... */
    
/**
   If v is of a type which can contain mutable state
   (e.g. properties or children of any sort, e.g. Array, Object,
   etc.) then all of that mutable state is cleaned up. For other
   types this is a no-op.

   This does not clear the prototype value, but clears everything
   else.

   Note that this effectively destroys Buffers, Natives, and
   Functions.

   Returns 0 on success. Errors include:

   - CWAL_RC_MISUSE: if c is NULL or its engine cannot be found.

   - CWAL_RC_TYPE: if c is not a type capable of holding
   properties.

*/
int cwal_value_clear_mutable_state( cwal_value * c );
#endif

/**
   For each property in container value c which does not have the
   CWAL_VAR_F_HIDDEN flag, f(property,state) is called. If it returns
   non-0 looping stops and that value is returned.

   Returns CWAL_RC_MISUSE if !o or !f, and 0 on success.

   Prior to 20200118, this routine returned CWAL_RC_CYCLES_DETECTED if
   concurrent iteration would have been triggered, but that is, as of
   20191211, permitted by the API, so that condition was removed from
   this function.
*/
int cwal_props_visit_kvp( cwal_value * c, cwal_kvp_visitor_f f, void * state );

/**
   Convenience wrapper around cwal_props_visit_kvp() which visits only the
   keys. f() is passed the (cwal_value*) for each property key.
*/
int cwal_props_visit_keys( cwal_value * c, cwal_value_visitor_f f, void * state );

/**
   Convenience variant of cwal_props_visit_kvp() which visits only the
   values. f() is passed the (cwal_value*) for each property value.
*/
int cwal_props_visit_values( cwal_value * o, cwal_value_visitor_f f, void * state );

/**
   If c is a property container type and its object-level properties
   may safely be iterated over without danger of recursion then this
   function returns true (non-0), else false (0).

   As of 20191211, objects and lists may iterate multiple times
   concurrently so long as no "locking" operation which calls back
   into client-side code is underway. As of this writing, there are no
   such operations, so this function always returns true for any
   values of types which are capable of holding properties.

   @see cwal_value_is_iterating_list()
   @see cwal_value_may_iterate_list()
*/
bool cwal_value_may_iterate( cwal_value const * const c );

/**
   If c is a value type with a list (array, tuple, hashtable (kinda))
   and that list is currently being iterated over, or is otherwise
   locked, this function returns true (non-0), else false (0).

   Certain list operations, e.g. resizing, sorting, or hash table
   manipulation, are disallowed when a list is being iterated over.
   (Note that a hashtable is a list, of sorts.)

   @see cwal_value_may_iterate()
   @see cwal_value_may_iterate_list()
   @see cwal_value_is_iterating_list()
*/
bool cwal_value_is_iterating_props( cwal_value const * const c );

/**
   If c is a value type with a list (array, tuple, hashtable (kinda))
   and that list is currently being iterated over, or is otherwise
   locked, this function returns true (non-0), else false (0).

   Certain list operations, e.g. resizing, sorting, or hash table
   manipulation, are disallowed when a list is being iterated over.
   (Note that a hashtable is a list, of sorts.)

   @see cwal_value_may_iterate()
   @see cwal_value_may_iterate_list()
*/
bool cwal_value_is_iterating_list( cwal_value const * const c );

/**
   If c is a value type with a list (array, tuple, hashtable (kinda))
   and that list is currently capable of being iterated over, this
   function returns true (non-0), else false (0).

   Certain list operations, e.g. resizing, sorting, or hash table
   manipulation, are disallowed when a list is being iterated over.
   (Note that a hashtable is a list, of sorts.) A stateful sort
   operation (cwal_array_sort_stateful()) locks a list from
   being iterated while it is running.

   @see cwal_value_is_iterating_list()
   @see cwal_value_may_iterate()
*/
bool cwal_value_may_iterate_list( cwal_value const * const c );

/**
   Copies all non-hidden properties from src to dest.

   Returns 0 on success. Returns CWAL_RC_MISUSE if either pointer is
   NULL and CWAL_RC_TYPE if cwal_props_can() returns false for either
   src or dest.

   It returns CWAL_RC_IS_VISITING if dest is already being traversed
   (the data model does not support modifying properties during
   iteration).

   All property flags, e.g. CWAL_VAR_F_CONST, *EXCEPT* for
   CWAL_VAR_F_HIDDEN, are carried over from the source to the
   destination. Properties marked as CWAL_VAR_F_HIDDEN are "hidden"
   from iteration routines and therefore are not copied by this
   operation.

   Note that this does not actually _copy_ the properties - only
   references are copied. This function must, however, allocate
   internals to store the new properties, and can fail with
   CWAL_RC_OOM.
*/
int cwal_props_copy( cwal_value * src, cwal_value * dest );

/**
   Removes a property from container-type value c.
   
   If c contains the given key (which must be keyLen bytes long), it
   is removed and 0 is returned. If it is not found, CWAL_RC_NOT_FOUND
   is returned (which can normally be ignored by client code).

   Returns 0 if the given key is found and removed.

   CWAL_RC_MISUSE is returned if obj or key are NULL or key has
   a length of 0.

   Fails with CWAL_RC_IS_VISITING if c is currently being iterated
   over.

   This is functionally equivalent calling
   cwal_prop_set(obj,key,keyLen,NULL).

*/
int cwal_prop_unset( cwal_value * c, char const * key,
                     cwal_midsize_t keyLen );

/**
   Searches the given container value for a string-keyed property
   matching the first keyLen bytes of the given key. If found, it is
   returned. If no match is found, or any arguments are NULL, NULL is
   returned. The returned object is owned by c, and may be invalidated
   by ANY operations which change c's property list (i.e. add or
   remove properties).

   This routine will only ever match property keys for which
   cwal_value_get_cstr() returns non-NULL (i.e. property keys of type
   cwal_string or cwal_buffer). It never compares the key to
   non-string property keys (even though they might compare equivalent
   if the search key was a "real" cwal_string).

   @see cwal_prop_get_kvp()
   @see cwal_prop_get_v()
   @see cwal_prop_get_kvp_v()
*/
cwal_value * cwal_prop_get( cwal_value const * c, char const * key,
                            cwal_midsize_t keyLen );

/**
   Similar to cwal_prop_get() but takes a cwal_value key and may
   compare keys of different types for equivalence. e.g. the lookup
   key (integer 1) will match a property with a key of (double 1) or
   (string "1").

   See cwal_prop_get_kvp_v() for more details, in particular about the
   special handling of boolean-type lookup- and property keys.

   @see cwal_prop_get_kvp()
   @see cwal_prop_get_v()
   @see cwal_prop_get_kvp_v()
*/
cwal_value * cwal_prop_get_v( cwal_value const * c, cwal_value const * key );

/**
   UNTESTED! EXPERIMENTAL! DO NOT USE!

   An alternate form of cwal_prop_get() which returns any found value
   via its final parameter. Returns 0 if a match is found,
   CWAL_RC_NOT_FOUND if no match is found, and CWAL_RC_MISUSE if c or
   key are NULL or c is not a properties-capable type. If rv is not
   NULL, then if a match is found (and thus 0 is returned), *rv is set
   to that value. If any value other than 0 is returned, *rv is not
   modified.

   Achtung: if/when property interceptors are added to the cwal core,
   this function will support them (whereas cwal_prop_get() and
   friends will not). Thus a non-0 return value may hypothetically
   come from a property interceptor function.
*/
int cwal_prop_getX( cwal_value * c, bool processInterceptors,
                    char const * key, cwal_midsize_t keyLen,
                    cwal_value ** rv);

/**
   UNTESTED! EXPERIMENTAL! DO NOT USE!

   Like cwal_prop_getX() but takes a cwal_value key.
*/
int cwal_prop_getX_v( cwal_value * c, bool processInterceptors,
                      cwal_value const * key, cwal_value ** rv );

/**
   cwal_prop_get_kvp() is similar to cwal_prop_get(), but returns
   its result in a more complex form (useful mostly in
   interpreter-level code so that it can get at their flags to
   check for constness and such).

   The container value c is searched as described for cwal_prop_get(),
   but prototypes are only searched if searchProtos is true, in which
   case they are searched recursively if need. If foundIn is not NULL
   then if a match is found then *foundIn is set to the Value in which
   the key is found (it will be c or a prototype of c).

   If a match is found, it is returned.

   ACHTUNG: the returned object is owned by c (or *foundIn) and may be
   invalidated on any modification (or cleanup) of c (or *foundIn).

   Returns 0 if no match is found, !c, or !key.
*/
cwal_kvp * cwal_prop_get_kvp( cwal_value * c, char const * key,
                              cwal_midsize_t keyLen, bool searchProtos,
                              cwal_value ** foundIn );


/**
   cwal_prop_get_kvp_v() works identically to cwal_prop_get_kvp(), but
   takes its key in the form of a cwal_value. Except in the case of a
   boolean lookup key or property key (see below), it performs a
   type-loose comparison for equivalence, not strict equality.
   e.g. (string "1") and (integer 1) will match. In other words, it
   uses the equivalent of cwal_value_compare() for comparisons (and
   the lookup key is always the logical left-hand-side of that
   comparison).

   Returns 0 if no match is found, !c, or !key.

   Prior to 20190706 this routine incorrectly handled lookup/property
   keys of type CWAL_TYPE_BOOL, in that its equivalence comparison
   would, for a lookup key of cwal_value_true(), match the first
   "truthy" property key. Likewise, a property key of
   cwal_value_true() would match any truthy lookup key. Complementary
   comparisons applied for cwal_value_false(). This went unnoticed
   because boolean-type keys are apparently never used. As of
   20190706, when either the lookup key or a property key are of type
   boolean, this routine performs a type-strict comparison, so will
   never match anything but an exact match for those cases. This change
   applies to all of the various property lookup routines, not
   just this function.

   @see cwal_prop_get()
   @see cwal_prop_get_v()
   @see cwal_prop_get_kvp()
   @see cwal_prop_take()
   @see cwal_prop_take_v()
*/
cwal_kvp * cwal_prop_get_kvp_v( cwal_value * c, cwal_value const * key,
                                bool searchProtos,
                                cwal_value ** foundIn );
    
/**
   Similar to cwal_prop_get(), but removes the value from the parent
   container's ownership. This removes the owning container's
   reference point but does not destroy the value if its refcount
   reaches 0. If no item is found then NULL is returned, else the
   object (now owned by the caller or possibly shared with other
   containers) is returned.

   This is functionally similar to adding a ref to the property value,
   removing it from/unsetting it in the container (which removes the
   container's ref), and then cwal_value_unhand()ing it.

   Returns NULL if either c or key are NULL, key has a length of 0, or
   c is-not-a container.

   Note that this does not search through prototypes for a property -
   it only takes properties from the given value.

   See cwal_prop_get() for important details about the lookup/property
   key comparisons.

   If c is currently undergoing traversal, NULL is returned (a limitation
   of the data model).
       
   FIXME: #1: add a keyLen parameter, for symmetry with the rest of
   the API.

   @see cwal_prop_take_v()
   @see cwal_prop_get()
   @see cwal_prop_get_v()
*/
cwal_value * cwal_prop_take( cwal_value * c, char const * key );

/**
   Similar to cwal_prop_take() but also optionally takes over
   ownership of the key as well. If takeKeyAsWell is not NULL then
   ownership of the key is taken away, eactly as for the result value,
   and assigned to *takeKeyAsWell, otherwise the key is discarded
   (unreferenced).

   Note that they key passed to this function might be equivalent to
   (cwal_value_compare()), but not be a pointer match for, the key
   found in the the container, thus the passed-in key and
   *takeKeyAsWell may be different pointers on success. To deal with
   that, make sure to pass key to cwal_value_ref() before calling
   this, and to one of cwal_value_unhand() or cwal_value_unref()
   (depending on key's current lifetime requirements) after calling
   this.

   If no entry is found, NULL is returned and *takeKeyAsWell is not
   modified.

   @see cwal_prop_take()
   @see cwal_prop_get()
   @see cwal_prop_get_v()
*/
cwal_value * cwal_prop_take_v( cwal_value * c, cwal_value * key,
                               cwal_value ** takeKeyAsWell );

/**
   Functionally similar to cwal_array_set(), but uses a string key
   as an index. Like arrays, if a value already exists for the given key,
   it is destroyed by this function before inserting the new value.

   c must be a "container type" (capable of holding key/value
   pairs). For the list of types capable of having properties, see
   cwal_props_can().

   If v is NULL then this call is equivalent to
   cwal_prop_unset(c,key,keyLen). Note that (v==NULL) is treated
   differently from v having the special null value
   (cwal_value_null()). In the latter case, the key is set to the
   special null value.

   The key may be encoded as ASCII or UTF8. Results are undefined
   with other encodings, and the errors won't show up here, but may
   show up later, e.g. during output.
   
   The flags argument may be a mask of cwal_var_flags values. When
   in doubt about whether the property is already set and might
   have other flags set, use CWAL_VAR_F_PRESERVE. If it makes no
   difference for your use case, feel free to pass 0.

   Returns 0 on success, non-0 on error. It has the following error
   cases:

   - CWAL_RC_MISUSE: e, c, or key are NULL or !*key.

   - CWAL_RC_TYPE: c is not of a type capable of holding
   properties.

   - CWAL_RC_OOM: an out-of-memory error

   - CWAL_RC_IS_VISITING: The library does not support modifying a
   container which is being visited/iterated over.

   - CWAL_RC_CONST_VIOLATION: cannot (re)set a const property.

   - CWAL_RC_DISALLOW_NEW_PROPERTIES: the container has been flaged
   with the CWAL_CONTAINER_DISALLOW_NEW_PROPERTIES flag and key refers
   to a property which is not in the container.

   - CWAL_RC_DISALLOW_PROP_SET: the container has been flaged with the
   CWAL_CONTAINER_DISALLOW_PROP_SET flag.

   - CWAL_RC_NOT_FOUND if !v and the entry is not found. This can
   normally be ignored as a non-error, but is provided for
   completeness.

   - CWAL_RC_DESTRUCTION_RUNNING if c is currently being finalized.
   This "should" only ever be returned in conjunction with cwal_native
   types if they have a finalizer and that finalizer manipulates the
   cwal_native/cwal_value part it has held on to in parallel.


   On error ownership of v is NOT modified, and the caller may still
   need to clean it up. For the variants of this function
   taking a (cwal_value*) key, ownership of the key is also not
   changed on error.

   For example, the following code will introduce a "leak" (insofar as
   cwal really leaks) if this function fails:

   @code
   cwal_prop_set( myObj, "foo", 3, cwal_new_integer(e, 42) );
   @endcode

   Because the value created by cwal_new_integer() has no owner
   and is not cleaned up. The "more correct" way to do this is:

   @code
   cwal_value * v = cwal_new_integer(e, 42);
   cwal_value_ref(v);
   int rc = cwal_prop_set_with_flags( myObj, "foo", 3, v, CWAL_VAR_F_HIDDEN );
   cwal_value_unref(v);
   // if prop_set worked, v has a reference, else it is cleaned up
   if( 0 != rc ) {
   ... handle error.
   }
   @endcode

   However, because the value in that example is owned by the active
   scope, it will be cleaned up when the scope exits if the user does
   not unref it manually. i.e. it is still "safe" vis-a-vis not
   leaking memory, to use the first (simpler) insertion
   option. Likewise, as cwal_scope_sweep() can also clean up the
   errant integer within the current scope (but often has other
   side-effects, so be careful with that).

   See cwal_prop_get() for important details about the lookup/property
   key comparisons.

   @see cwal_prop_set()
   @see cwal_prop_set_v()
   @see cwal_prop_set_with_flags_v()
*/
int cwal_prop_set_with_flags( cwal_value * c, char const * key,
                              cwal_midsize_t keyLen, cwal_value * v,
                              uint16_t flags );

/**
   Equivalent to calling cwal_prop_set_with_flags() using the same
   parameters, and a flags value of CWAL_VAR_F_PRESERVE.
*/
int cwal_prop_set( cwal_value * c, char const * key,
                   cwal_midsize_t keyLen, cwal_value * v );

/**
   UNTESTED! EXPERIMENTAL! DO NOT USE!
*/
int cwal_prop_setX_with_flags( cwal_value * c,
                               bool processInterceptors,
                               char const * key,
                               cwal_midsize_t keyLen,
                               cwal_value * v,
                               uint16_t flags );

/**
   UNTESTED! EXPERIMENTAL! DO NOT USE!
*/
int cwal_prop_setX_with_flags_v( cwal_value * c, bool processInterceptors,
                                 cwal_value * key,
                                 cwal_value * v, uint16_t flags );

/**
   Returns non-0 (true) if c is of a type capable of containing
   per-instance properties, else 0 (false).

   The following value types will return true from this function:

   CWAL_TYPE_ARRAY, CWAL_TYPE_OBJECT, CWAL_TYPE_FUNCTION,
   CWAL_TYPE_EXCEPTION, CWAL_TYPE_NATIVE, CWAL_TYPE_HASH,
   CWAL_TYPE_BUFFER (as of 20141217).

   All others return false.

   Note that properties referred to by this function are independent
   of an array's indexed entries or a hashtable's entries.
*/
bool cwal_props_can( cwal_value const * c );

/**
   Returns true if cwal_props_can() returns true and if the value
   actually has any properties (not including those in prototype
   values). This is an O(1) operation.
*/
bool cwal_props_has_any( cwal_value const * c );

/**
   Returns true if c is a value type which is suitable for use as a
   key for object properties or hashtable entries. As of 2021-07-09,
   types whose equivalence comparison takes their mutable state into
   account are disallowed as property keys. That includes
   CWAL_TYPE_BUFFER and CWAL_TYPE_TUPLE.  Prior to that, any data type
   could be used as a property key, but that was a fundamentally
   flawed design decision.
*/
bool cwal_prop_key_can( cwal_value const * c );

/**
   Returns the number of properties in c (not including prototypes),
   or 0 if c is not a properties-capable type. This is an O(1)
   operation of built with CWAL_OBASE_ISA_HASH, else it's an O(N)
   operation.

   Note that this count includes properties flagged with
   CWAL_VAR_F_HIDDEN.
*/
cwal_midsize_t cwal_props_count( cwal_value const * c );

/**
   Like cwal_prop_set_with_flags() but takes a cwal_value key.

   Note that this routine does a type-loose comparison for the lookup
   key and property keys, with the exception of boolean-type keys
   (which only compare type-strictly: see cwal_prop_get_kvp_v() for
   details). That is, the same comparison as cwal_value_compare() is
   performed.

   If cwal_prop_key_can() returns false for the given key,
   CWAL_RC_TYPE is returned. Prior to 2021-07-09, any data type could
   be used as a property key, but that was a fundamentally flawed
   design decision.
*/
int cwal_prop_set_with_flags_v( cwal_value * c, cwal_value * key,
                                cwal_value * v,
                                uint16_t flags );

/**
   Equivalent to calling cwal_prop_set_with_flags_v() using the same
   parameters, and a flags value of CWAL_VAR_F_PRESERVE.
*/
int cwal_prop_set_v( cwal_value * c, cwal_value * key, cwal_value * v );

/**
   Like cwal_prop_unset() but takes a cwal_value key.

   Fails with CWAL_RC_IS_VISITING if c is currently being iterated
   over.
*/
int cwal_prop_unset_v( cwal_value * c, cwal_value * key );

/**
   Returns true (non-zero) if the value v contains the given
   property key. If searchPrototype is true then the search
   continues up the prototype chain if the property is not found,
   otherwise only v is checked.

   See cwal_prop_get_kvp_v() for details about the lookup/property key
   comparisons.
*/
bool cwal_prop_has( cwal_value const * v, char const * key,
                    cwal_midsize_t keyLen,
                    bool searchPrototype );

/**
   Like cwal_prop_has() but takes a cwal_value key.

   See cwal_prop_get_kvp_v() for details about the lookup/property key
   comparisons.
*/
bool cwal_prop_has_v( cwal_value const * v, cwal_value const * key,
                      bool searchPrototype );
    
/**
   Returns the virtual type of v, or CWAL_TYPE_UNDEF if !v.
*/
cwal_type_id cwal_value_type_id( cwal_value const * v );

/**
   If v is not NULL, returns the internal string name of v's
   concrete type, else it returns NULL. If no type name proxy is
   installed (see cwal_engine_type_name_proxy()) then the returned
   bytes are guaranteed to be static, else they are gauranteed (by
   the proxy implementation) to live at least as long as v.

   If it returns non-NULL and len is not NULL then *len will hold
   the length of the returned string.

   LIMITATION: if v is a builtin value then it has no engine
   associated with it, meaning we cannot get proxied name. We can
   fix that by adding an engine parameter to this function.
*/
char const * cwal_value_type_name2( cwal_value const * v,
                                    cwal_size_t * len);

    
/**
   Equivalent to cwal_value_type_name2(v, NULL).
*/
char const * cwal_value_type_name( cwal_value const * v );

/**
   For the given cwal_type_id value, if that value represents
   a client-instantiable type, this function returns the same
   as cwal_value_type_name() would for an instance of that type,
   else returns NULL.
*/
char const * cwal_type_id_name( cwal_type_id id );

/**
   For the given type ID, returns its "base sizeof()," which
   has a slightly different meaning for different types:

   CWAL_TYPE_BOOL, UNDEF, NULL: are never allocated and return 0.

   CWAL_TYPE_LISTMEM: are internal metrics-counting markers and have
   no sizes, so return 0.

   CWAL_TYPE_STRING/XSTRING/ZSTREAM: returns the base size of string
   values (which may differ for each string sub-type), not including
   their string bytes or trailing NUL.

   CWAL_TYPE_INTEGER, DOUBLE, ARRAY, OBJECT, NATIVE, BUFFER, FUNCTION,
   EXCEPTION, HASH: returns the allocated size of the Value part plus
   its high-level type representation (e.g. cwal_object), not
   including any dynamic data such as hash table/array memory or
   properties.

   CWAL_TYPE_WEAKREF, KVP: returns sizeof(cwal_weak_ref)
   resp. sizeof(cwal_kvp).

   CWAL_TYPE_SCOPE: sizeof(cwal_scope). Though the API technically
   supports dynamically-allocated scopes, in practice their usage fits
   perfectly with stack allocation.

   Returns 0 if passed an unknown value.
*/
cwal_size_t cwal_type_id_sizeof( cwal_type_id id );

/** Returns true if v is null, v->api is NULL, or v holds the special undefined value. */
bool cwal_value_is_undef( cwal_value const * v );
/** Returns true if v contains a null value. */
bool cwal_value_is_null( cwal_value const * v );
/** Returns true if v contains a bool value. */
bool cwal_value_is_bool( cwal_value const * v );
/** Returns true if v contains an integer value. */
bool cwal_value_is_integer( cwal_value const * v );
/** Returns true if v contains a double value. */
bool cwal_value_is_double( cwal_value const * v );
/** Returns true if v contains a number (double, integer, bool) value. */
bool cwal_value_is_number( cwal_value const * v );
/** Returns true if v contains a cwal_string value. */
bool cwal_value_is_string( cwal_value const * v );
/** Returns true if v contains an cwal_array value. */
bool cwal_value_is_array( cwal_value const * v );
/** Returns true if v contains an cwal_object value. */
bool cwal_value_is_object( cwal_value const * v );
/** Returns true if v contains a cwal_native value. */
bool cwal_value_is_native( cwal_value const * v );
/** Returns true if v contains a cwal_buffer value. */
bool cwal_value_is_buffer( cwal_value const * v );
/** Returns true if v contains a cwal_exception value. */
bool cwal_value_is_exception( cwal_value const * v );
/** Returns true if v contains a cwal_hash value. */
bool cwal_value_is_hash( cwal_value const * v );
/** Returns true if v contains a "unique" value
    (of type CWAL_TYPE_UNIQUE). */
bool cwal_value_is_unique( cwal_value const * v );
/** Returns true if v contains a cwal_tuple value. */
bool cwal_value_is_tuple( cwal_value const * v );

/**
   A special-purpose function which upscopes v into s if v is owned by
   a lower (newer) scope. If v is owned by s, or a higher scope, then
   this function has no side-effects. This is necessary when clients
   create values which they need to survive the current scope. In such
   cases they should pass the scope they want to (potentially)
   reparent the value into.

   Note that this reparenting is only for lifetime management
   purposes, and has nothing at all to do with "scope variables". It
   does not affect the script-side visibility of v.

   Returns 0 on success, CWAL_RC_MISUSE if any argument is 0, and
   is believed to be infalible (hah!) as long as the arguments are
   legal and their underlying cwal_engine is in a legal state.

   This is a no-op (returning 0) if v's current owning scope is older
   than s or if cwal_value_is_builtin(v). It may assert() that neither
   argument is NULL.
*/
int cwal_value_rescope( cwal_scope * s, cwal_value * v );

/**
   Given a pointer, returns true (non-0) if m lives in the memory
   region used for built-in/constant/shared (cwal_value*) instances,
   else returns 0.  Is tolerant of NULL (returns 0). This
   determination is O(1) or 2x O(1) (only pointer range comparisons),
   depending on whether we have to check both sets of builtins.
   
   If this returns true (non-0), m MUST NOT EVER be cwal_free()d
   because it refers to stack memory! If m refers to a (cwal_value*)
   instance, that value is a built-in and it MAY be (harmlessly)
   passed to any public Value-lifetime-management routines (e.g.
   cwal_value_ref(), cwal_value_unref(), and cwal_value_unhand()), all
   of which are no-ops for built-in values. Note that this returns
   true for any address in a range which covers more than just
   cwal_value instances, but in practice it is used only to check
   whether a cwal_value is a built-in. For example, if cwal is
   compiled with length-1 ASCII strings as built-ins then:

   @code
   cwal_value_is_builtin(
   cwal_value_get_cstr(
   cwal_new_string_value(e,"a",1), 0
   )
   )
   @endcode

   will return true, as those string bytes are in the built-in block
   of memory.
*/
bool cwal_value_is_builtin( void const * m );
    
/**
   Creates a new value which refers to a client-provided "native"
   object of an arbitrary type. N is the native object to bind.
   dtor is the optional finalizer function to call when the new
   value is finalized. typeID is an arbitrary pointer which is
   used later to verify that a given cwal_native refers to a
   native of a specific type.

   A stack-allocated native pointer is only legal if it can be
   _guaranteed_ to out-live the wrapping value handle.

   This function returns NULL if (e, N, typeID) are NULL or on
   allocation error.On success it returns a new value, initially
   owned by the current scope.

   Clients can fetch the native value later using
   cwal_native_get(). The typeID passed here can be passed to
   cwal_native_get() to allow cwal to confirm that the caller is
   getting the type of pointer he wants. In practice the typeID is
   a pointer to an app/library-wide value of any type. Its
   contents are irrelevant, only its _address_ is relevant. While
   it might seem intutive to use a string as the type ID,
   compilers may (or may not) combine identical string constants
   into a single string instance, which may or may not foul up
   such usage. If one needs/wants to use a string, set it in 1
   place, e.g.  via a file-scope variable, and expose its address
   to any client code which needs it (as opposed to them each
   inlining the string, which might or might not work at runtime,
   depending on whether the strings get compacted into a single
   instance).
       
   When the returned value is finalized (at a time determined by
   the cwal engine), if dtor is not NULL then dtor(e,N) is called
   to free the value after any Object-level properties are
   destructed. If N was allocated using cwal_malloc() or
   cwal_realloc() and it has no special cleanup requirements then
   cwal_free can be passed as the dtor value. Finalizer functions
   "might currently be prohibited" from performing "certain
   operations" with the cwal API during cleanup, but which ones
   those are (or should be) are not yet known.

   Note that it is perfectly legal to use a static value for N,
   assuming the finalizer function (if any) does not actually try
   to free() it. In the case of a static, the value could be used
   as its own typeID (but since the client has that pointer, and
   it's static, there doesn't seem to be much use for having a
   cwal_native for that case!).
       
   See cwal_new_VALUE() for more details about the return value.
       
   @see cwal_new_VALUE()
   @see cwal_new_native()       
   @see cwal_value_get_native()
*/
cwal_value * cwal_new_native_value( cwal_engine * e, void * N,
                                    cwal_finalizer_f dtor,
                                    void const * typeID );

/**
   Equvalent to passing the return value of
   cwal_new_native_value() to cwal_value_get_native().
*/
cwal_native * cwal_new_native( cwal_engine * e, void * n,
                               cwal_finalizer_f dtor,
                               void const * typeID );

/**
   A special-purpose component for binding native state to
   cwal_natives and cwal_functions, necessary when native bindings
   hold Value pointers whose lifetimes are not managed via
   Object-level properties of the native.

   This callback is called whenever the Value part of the
   function/native is moved to an older scope (for lifetime management
   purposes, not script visibility purposes) and is passed the
   following arguments:

   s: the scope to potentially be rescoped to.

   v: the cwal_value part of the Native or Function. Use
   cwal_value_get_native() or cwal_value_get_function(), as
   appropriate, to get the higher-level part. DO NOT use
   cwal_value_native_part() resp. cwal_value_function_part(),
   as those may return parts of _other_ values higher up
   in v's prototype chain!

   Rescopers MUST do the following:

   a) For each "unmanaged" Value, call cwal_value_rescope(s,
   theValue). They MUST NOT pass a NULL value to cwal_value_rescope(),
   or an assertion may be triggered.

   d) It must not rescope the Native/Function passed to it, as that
   value is in the process of rescoping when this is called.

   Implementations must return 0 on success and any error is
   tantamount to an assertion, leading to undefined results
   in cwal from here on out.

   Implementations MUST NOT perform any work which might allocate
   values. (Potential TODO: disable the allocator during this
   operation, to enforce that requirements.)

   Note that it is often necessary to make such hidden/internal values
   vacuum-proof by using cwal_value_make_vacuum_proof() on it (for
   containers) or adding them to a hidden/internal container which is
   itself vacuum-proofed. Alternately, such refs can be held as hidden
   properties of the cwal_native, perhaps using unique keys (via
   cwal_new_unique()) to keep client code from every being able to
   address them. When stored in the cwal_native's properties, no extra
   Value rescoping/ownership management is necessary on the client
   side.

   @see cwal_native_set_rescoper()
   @see cwal_function_set_rescoper()
*/
typedef int (*cwal_value_rescoper_f)( cwal_scope * s, cwal_value * v );

/**
   A special-case function which is necessary when client-side
   natives manage Values which are not visible to the native's
   Object parent (i.e. they are not tracked as properties). When
   creating such natives, after calling cwal_new_native() or
   cwal_new_native_value(), call this function and pass it your
   rescoper implementation. 

   This function returns 0 unless nv is NULL, in which case it
   returns CWAL_RC_MISUSE.

   When the given rescoper is called, cwal_value_get_native() can be
   called on its argument to get the nv pointer which was passed to
   this function (it will never be NULL when the rescoper is called
   from cwal). DO NOT use cwal_value_native_part()!

   @see cwal_value_rescoper_f()
*/
int cwal_native_set_rescoper( cwal_native * nv,
                              cwal_value_rescoper_f rescoper );

/**
   A special-case function which is necessary when client-side
   function state Values which are not visible to the function's
   Object parent (i.e. they are not tracked as properties). When
   creating such natives, after calling cwal_new_function() (or
   equivalent), call this function and pass it your rescoper
   implementation.

   This function returns 0 unless f is NULL, in which case it
   returns CWAL_RC_MISUSE.

   When the given rescoper is called, cwal_value_get_function() can be
   called on its argument to get the nv pointer which was passed to
   this function (it will never be NULL when the rescoper is called
   from cwal). DO NOT use cwal_value_function_part()!

   @see cwal_value_rescoper_f()
*/
int cwal_function_set_rescoper( cwal_function * f,
                                cwal_value_rescoper_f rescoper );

    
/**
   Returns the cwal_value form of n, or 0 if !n.
*/
cwal_value * cwal_native_value( cwal_native const * n );

/**
   If val is of type CWAL_TYPE_NATIVE then this function
   assigns *n (if n is not NULL) to its cwal_native handle
   and returns 0, else it returns CWAL_RC_TYPE and does not
   modify *n.

   In practice this is never used by clients - see
   cwal_value_get_native().

   @see cwal_value_get_native()
   @see cwal_value_native_part()
*/
int cwal_value_fetch_native( cwal_value const * val, cwal_native ** n);

/**
   If v is of type CWAL_TYPE_NATIVE then this function returns its
   native handle, else it returns 0. This is a simplified form of
   cwal_value_fetch_native().
*/
cwal_native * cwal_value_get_native( cwal_value const * v );
    
/**
   Fetches the a raw "native" value (void pointer) associated with
   n.
       
   If (0==typeID) or n's type ID is the same as typeID then *dest (if dest is not NULL)
   is assigned to n's raw native value and 0 is returned, else...

   CWAL_RC_TYPE: typeID does not match.

   CWAL_RC_MISUSE: n is NULL.

   Note that clients SHOULD pass a value for typeID to ensure that
   they are getting back the type of value they expect. The API
   recognizes, however, that the type ID might not be available or
   might be irrelevant to a particular piece of code, and
   therefore allows (but only grudgingly) typeID to be NULL to
   signify that the client knows WTF he is doing and is getting a
   non-type-checked pointer back (via *dest).
*/
int cwal_native_fetch( cwal_native const * n, void const * typeID, void ** dest);

/**
   Convenience form of cwal_native_fetch() which returns NULL if
   n's type ID does not match typeID.
*/
void * cwal_native_get( cwal_native const * n, void const * typeID);    

/**
   Clears the underlying native part of n, such that future calls
   to cwal_native_get() will return NULL. If callFinalizer is true
   (non-0) then the native's finalizer, if not NULL, is called,
   otherwise we assume the caller knows more about the lifetime of
   the value than we do and the finalizer is not called. As a
   general rule, clients should pass a true value for the second
   parameter.
*/
void cwal_native_clear( cwal_native * n, char callFinalizer );

    
/**
   Creates a new "buffer" value. startingSize is the amount of
   memory to reserve in the buffer by default (0 means not to
   reserve any, of course). If reservation of the buffer fails
   then this function returns NULL.

   See cwal_new_VALUE() for details on the ownership.

   See the cwal_buffer API for how to use buffers.
*/
cwal_value * cwal_new_buffer_value(cwal_engine *e, cwal_size_t startingSize);

/**
   Equvalent to passing the return value of
   cwal_new_buffer_value() to cwal_value_get_buffer().
*/
cwal_buffer * cwal_new_buffer(cwal_engine *e, cwal_size_t startingSize);

/**
   Equivalent to cwal_value_unref( e, cwal_buffer_value(v) ).
*/
int cwal_buffer_unref(cwal_engine *e, cwal_buffer *v);

/**
   This convenience routine takes b's buffered memory and
   transfers it to a new Z-string value (see
   cwal_new_zstring()).

   b may either have been created using cwal_new_buffer() or be a
   "non-value" buffer which the client happens to be using.

   See cwal_new_string() for size limitation notes.

   The new string will have a string byte length of b->used.

   If !b or !e, if b->used is too big, or on allocation error, NULL is
   returned. If b has no memory, the empty string value is
   returned. The returned value is owned by e and (unless it is the
   empty string) will initially be owned by e's current scope. If NULL
   is returned, b's memory is not modified, otherwise after calling
   this b will be an empty buffer (but its lifetime is otherwise
   unaffected).

   After returning, if b->mem is not NULL then b still owns its
   managed buffer (and there was an error, so NULL will have been
   returned). For most use cases, clients should unconditionally pass
   b to cwal_buffer_clear() after calling this, as they presumably had
   no interest in managing b's memory. On success, b->mem's ownership
   will have been transfered to the returned string and b->mem will be
   0.

   For metrics-counting purposes, b->mem's memory is counted by whoever
   allocated it first, and not by z-string metrics.
*/
cwal_string * cwal_buffer_to_zstring(cwal_engine * e, cwal_buffer * b);

/**
   Equivalent to cwal_string_value(cwal_buffer_to_zstring(e,b)).

   @see cwal_buffer_to_zstring()
*/
cwal_value * cwal_buffer_to_zstring_value(cwal_engine * e, cwal_buffer * b);

    
/**
   Equivalent to cwal_value_fetch_object() and friends, but for
   buffer values.

   In practice this is never used by clients - see
   cwal_value_get_buffer().
*/
int cwal_value_fetch_buffer( cwal_value const * val, cwal_buffer ** ar);

/**
   If value is-a Buffer then this returns the cwal_buffer form of the
   value, else it returns 0.
*/
cwal_buffer * cwal_value_get_buffer( cwal_value const * v );

/**
   Returns the cwal_value handle associated with the given buffer,
   or NULL if !s.

   WARNING OH MY GOD SUCH AN IMPORTANT WARNING: NEVER EVER EVER
   pass a cwal_buffer which was NOT created via
   cwal_new_buffer_value() to this function!!! It WILL cause
   invalid memory access if passed e.g. a cwal_buffer which was
   allocated on the stack (or by ANY means other than the
   functions listed above) and might (depending on the state of
   the random memory we're reading) cause the client to get
   invalid memory back (as opposed to NULL).
*/
cwal_value * cwal_buffer_value(cwal_buffer const * s);

/**
   Creates a new "exception" value.
       
   See cwal_new_VALUE() for details on the ownership of the return
   value.

   code is a client-interpreted error code. (Clients are free to
   use the cwal_rc values.) msg is an optional (may be NULL) value
   which stores some form of error message (of an arbitrary value
   type). Exception values may hold key/value pairs, so they may
   be "enriched" with client-specific information like a stack
   trace or source line/column information.

   On success the returned Exception value will contain the
   properties "code" and "message", reflecting the values passed
   here.

   @see cwal_new_exception().
   @see cwal_new_exceptionf()
   @see cwal_new_exceptionfv()
*/
cwal_value * cwal_new_exception_value(cwal_engine *e, int code, cwal_value * msg );

/**
   Equivalent to passing the return value of
   cwal_new_exception_value() to cwal_value_get_exception().
*/
cwal_exception * cwal_new_exception(cwal_engine *e, int code, cwal_value * msg );

/**
   A printf-like form of cwal_new_exception() which uses
   cwal_new_stringf() to create a formatted message to pass to
   cwal_new_exception().  Returns the new Exception value on
   success, NULL on allocation error or if either e is NULL. A
   format string of NULL or "" are treated equivalently as NULL.

   @see cwal_new_exceptionf()
   @see cwal_new_exception()
*/
cwal_exception * cwal_new_exceptionfv(cwal_engine * e, int code, char const * fmt, va_list args );

/**
   Identical to cwal_new_exceptionv() but takes its arguments in ellipsis form.
*/
cwal_exception * cwal_new_exceptionf(cwal_engine * e, int code, char const * fmt, ... );

/**
   Returns true if v is-a Exception, else false.
*/
bool cwal_value_is_exception(cwal_value const *v);

    
/**
   Equivalent to cwal_value_unref( e, cwal_exception_value(v) ).
*/
int cwal_exception_unref(cwal_engine *e, cwal_exception *v);
    
/**
   Equivalent to cwal_value_fetch_object() and friends, but for
   error values.

   In practice this is never used by clients - see
   cwal_value_get_exception().
*/
int cwal_value_fetch_exception( cwal_value const * val, cwal_exception ** ar);

/**
   If value is-a Exception then this returns the cwal_exception
   form of the value, else it returns 0.
*/
cwal_exception * cwal_value_get_exception( cwal_value const * v );

/**
   Returns the cwal_value handle associated with the given error
   value, or NULL if !r.
*/
cwal_value * cwal_exception_value(cwal_exception const * r);

/**
   Returns r's current result code, or some unspecified non-0
   value if !r.
*/
int cwal_exception_code_get( cwal_exception const * r );

/**
   Sets r's result code. Returns 0 on success, CWAL_RC_MISUSE
   if !r.
*/
int cwal_exception_code_set( cwal_exception * r, int code );

/**
   Returns the "message" part of the given error value, NULL if !r
   or r has no message part. The returned value is owned by/shared
   with r via reference counting, and it must not be unref'd by
   the client unless he explicitly references himself.
*/
cwal_value * cwal_exception_message_get( cwal_exception const * r );

/**
   Sets the given msg value to be r's "message" component. Interpretation
   of the message is up to the client.

   Returns 0 on success, CWAL_RC_MISUSE if either e or r are
   NULL. msg may be NULL.

   This function adds a reference to msg and removes a reference
   from its previous message (if any).
*/
int cwal_exception_message_set( cwal_engine * e, cwal_exception * r, cwal_value * msg );
    

/**
   Creates a new value wrapping a function.

   e is the owning engine, callback is the native function to wrap
   (it may not be NULL). state is optional state for the callback
   and may (assuming client application conditions allow for it)
   be NULL.

   The stateTypeID parameter is not directly used by the framework
   but can be used when the callback is called (via
   cwal_function_call() and friends) to determine whether the
   state parameter passed into the function is of a type expected
   by the client (which avoids mis-casting pointers when script
   code criss-crosses methods between object instances and
   classes). See cwal_args_state() for more details.
       
   See cwal_new_VALUE() for details regarding ownership and lifetime
   of the returned value.

   Returns NULL if preconditions are not met (e and callback may
   not be NULL) or on allocation error.

   When the callback is called via cwal_function_call() and
   friends, state->data will be available via the
   cwal_callback_args instance passed to the callback.

   When the returned value is destroyed, if stateDtor is not NULL
   then stateDtor(state) is called at destruction time to clean up
   the state value.
*/
cwal_value * cwal_new_function_value( cwal_engine * e,
                                      cwal_callback_f callback,
                                      void * state,
                                      cwal_finalizer_f stateDtor,
                                      void const * stateTypeID );
/**
   Equvalent to passing the return value of
   cwal_new_function_value() to cwal_value_get_function().
*/
cwal_function * cwal_new_function( cwal_engine * e, cwal_callback_f,
                                   void * state, cwal_finalizer_f stateDtor,
                                   void const * stateTypeID );
/**
   Returns true if v is-a Function, else false.
*/
bool cwal_value_is_function(cwal_value const *v);
/**
   If v is-a Function then this returns that Function handle,
   else it returns 0.
*/
cwal_function * cwal_value_get_function(cwal_value const *v);
/**
   Returns the Value handle part of f, or 0 if !f.
*/
cwal_value * cwal_function_value(cwal_function const *f);
/**
   Equivalent to cwal_value_unref(cwal_function_value(f)).
*/
int cwal_function_unref(cwal_function *f);

/**
   Calls the given function, passing it the given arguments and other
   state via its single cwal_callback_args parameter.

   The given scope is used as the execution context for purposes of
   ownership of new values.
       
   self may technically be 0, but f may require it to be of a specific
   type. Its intention is to be interpreted as the "this" object/value
   for the call (the semantics of which are client-dependent).

   argv must point at at least argc values. Both argv and argc may be
   0. This function takes a reference to each value in the list to
   protect them from being swept up by cwal_engine_sweep() (or
   similar) during the function call, but it does not make them
   vacuum-proof. After the call completes, each reference is released
   using cwal_value_unhand(), as opposed to cwal_value_unref(), so
   that the values survive the return trip to the caller.

   Returns the result from f or CWAL_RC_MISUSE if any arguments are
   invalid. Callback implementors should keep in mind that returning a
   value other than 0 (CWAL_RC_OK) will "usually" (but not always) be
   interpreted as an error condition. The exact details depend on the
   client's use of cwal.

   If resultVal is not NULL then on success *resultVal holds the
   value-level result from the function call (which may be 0, but
   clients typically interpret that as cwal_value_undefined()).
   Clients are recommended to explicitly initialize *resultVal to 0
   before calling this, to avoid potential confusion afterwards. On
   error *resultVal is not modified.

   It is strictly illegal to pop the current scope from within (or
   via) the f->callback(). Subscopes may of course be pushed (and must
   be popped before returning to this function, or an assertion may be
   triggered!).

   If s is not the interpreter's current scope, this function
   artificially changes the current scope, which comes with a
   _potential_ caveat: during the life of the f->callback() call, up
   until the next scope is pushed (if that happens), s is the current
   scope for all intents and purposes. But that's the point of this
   routine. That said: no small amount of practice implies that
   s should always be the engine's current scope.

   If callback hooks have been installed via cwal_callback_hook_set()
   then they are triggered in this function as described in the
   cwal_callback_hook documentation.  The "pre" callback is only
   triggered after it is certain that f will be called (i.e. after
   basic argument validation). If the pre-callback returns non-0 then
   neither f nor the post-callback are triggered. If the pre-callback
   returns 0 then both f and the post-callback are guaranteed to be
   called.

   f and self will be made sweep-proof (via a ref) during the life of
   the call. Both will be made vacuum-proof as well, if they weren't
   already.

   As a special case, if f is an interceptor function which is
   concurrently being called, CWAL_RC_CYCLES_DETECTED is returned to
   avoid endless loops in interceptor invocation. (That said:
   interceptors are an incomplete feature - do not use them.)
*/
int cwal_function_call_in_scope( cwal_scope * s, 
                                 cwal_function * f,
                                 cwal_value * self,
                                 cwal_value ** resultVal,
                                 uint16_t argc,
                                 cwal_value * const * argv );

/**
   Identical to cwal_function_call_in_scope() except that it sets the
   cwal_callback_args::propertyHolder value to the value passed as the
   3rd argument to this function (which may be 0).

   The propertyHolder member is intended to be used by container
   member functions which may need to distinguish between their "this"
   and the "owner" of the property (insofar as any container owns a
   value it contains). Specifically, it was added to support certain
   property interceptor usages. The vast, vast majority of functions
   have no need for it.

   @see cwal_function_call()
*/
int cwal_function_call_in_scope2( cwal_scope * s, 
                                  cwal_function * f,
                                  cwal_value * propertyHolder,
                                  cwal_value * self,
                                  cwal_value ** resultVal,
                                  uint16_t argc,
                                  cwal_value * const * argv );

/**
   Convenience form of cwal_function_call_in_scope() which pushes a
   new scope onto the stack before calling that function.

   Note that the value-level result of the function call might be
   owned by the pushed scope or a subscope, and therefore be cleaned
   up when the function call returns. If resultValue is not NULL then
   the result value of the call() is moved into the scope which was
   active before the call, such that it is guaranteed to survive when
   the scope created for the call() is closed. If resultValue is null,
   scope ownership of the call()'s result is not modified, and it
   "may" be cleaned up as soon as the scope expires.

   Returns the result of cwal_function_call_in_scope(), or some non-0
   CWAL_RC_xxx code if pushing a new scope fails (which can only
   happen if the client has installed a scope-push hook into the
   cwal_engine and that hook fails).
*/
int cwal_function_call( cwal_function * f,
                        cwal_value * self,
                        cwal_value ** resultVal,
                        uint16_t argc,
                        cwal_value * const * argv );

/**
   Identical to cwal_function_call_in_scope() except that it sets the
   cwal_callback_args::propertyHolder value to the value passed as the
   2nd parameter to this function (which may be 0).

   @see cwal_function_call_in_scope2()
*/
int cwal_function_call2( cwal_function * f,
                         cwal_value * propertyHolder,
                         cwal_value * self,
                         cwal_value ** resultVal,
                         uint16_t argc,
                         cwal_value * const * argv );

/**
   A form of cwal_function_call() which takes its arguments in
   the form of a cwal_array (which may be NULL).

   If s is NULL then this acts as a proxy for cwal_function_call(),
   otherwise it behaves like cwal_function_call_in_scope(), using
   s as the call scope.

   Returns 0 on success, non-0 on error.

   Results are undefined if args is NULL.

   This routine makes the args array safe from sweep-up and
   vacuuming (if it was not already so) for the duration of the
   call. (The f parameter will be made so via the proxied
   function.)

   ACHTUNG: any empty entries in the array will be passed to the
   callback as literal NULLs, and experience has shown that most
   callbacks do not generally expect any literal NULLs (because
   script code cannot generate them). So... be careful with that.
*/
int cwal_function_call_array( cwal_scope * s, cwal_function * f,
                              cwal_value * self, cwal_value ** rv,
                              cwal_array * args);


/**
   Works like cwal_function_call() but has very specific requirements
   on the variadic arguments: the list must contain 0 or more
   (cwal_value*) arguments and MUST ALWAYS be terminated by a 0/NULL
   value (NOT a cwal_value, e.g. cwal_value_null(), but a literal 0).

   This function places some "reasonable upper limit" on the number of
   arguments to avoid having to allocate non-stack space for them (the
   limit is defined by CWAL_OPT_MAX_FUNC_CALL_ARGS). It returns
   CWAL_RC_RANGE if that limit is exceeded.
*/
int cwal_function_callv( cwal_function * f,
                         cwal_value * self,
                         cwal_value ** resultVal,
                         va_list args );

/**
   Equivalent to cwal_function_callv() but takes its arguments
   in ellipsis form. BE SURE to read the docs for that function
   regarding the arguments!
*/
int cwal_function_callf( cwal_function * f,
                         cwal_value * self,
                         cwal_value ** resultValue,
                         ... );

/**
   The cwal_function_call_in_scope() counterpart of
   cwal_function_callv(). See those functions for more details.
*/
int cwal_function_call_in_scopef( cwal_scope * s, 
                                  cwal_function * f,
                                  cwal_value * self,
                                  cwal_value ** resultValue,
                                  ... );

/**
   If args is NULL, returns NULL, else it returns the same as passing
   (args->callee, stateTypeID) to cwal_function_state_get().

   @see cwal_function_state_get()
*/
void * cwal_args_state( cwal_callback_args const * args,
                        void const * stateTypeID );

/**
   If f is not NULL and either stateTypeID is NULL or f was created
   with the same stateTypeID as provided in the 2nd argument, then f's
   native state is returned, else NULL is returned.

   Passing a NULL as the 2nd parameter is not recommended, as cwal cannot
   verify the type of the returned pointer. Clients passing NULL are
   assumed to know what they're doing.

   Example usage:

   @code
   // From within a cwal_callback_f implementation...
   MyType * my = (MyType *)cwal_function_state_get(args->callee, MyTypeID);
   if(!my) { ...args->callee was not created with MyTypeID... }
   @endcode

   @see cwal_args_state()
*/
void * cwal_function_state_get( cwal_function * f,
                                void const * stateTypeID );

/**
   Returns a static array of 1001 integers containing the first
   1000 prime numbers (up to and including 7919) followed by a
   trailing 0 (so clients can use that as an iteration controller
   instead of the length).

   Intended to be use for initializing hash tables.
*/
int const * cwal_first_1000_primes(void);

/**
   Creates a new hash table object. These tables can store
   arbitrary cwal_value keys and values and have amortized O(1)
   search, insertion, and removal performance.

   hashSize is the number of elements in the hash, ideally be a
   prime number. It can be changed after creation using
   cwal_hash_resize(), but doing so will require another
   allocation (tisk tisk) to resize the
   table. cwal_first_1000_primes() can be used to get prime
   numbers if you have not got any handy.

   Returns the new hash table on success, NULL on error. It is
   an error if hashSize is 0.
*/
cwal_hash * cwal_new_hash( cwal_engine * e, cwal_size_t hashSize );

/**
   Equivalent to:

   cwal_hash_value(cwal_new_hash(e,hashSize))
*/
cwal_value * cwal_new_hash_value( cwal_engine * e, cwal_size_t hashSize);

/**
   Searches the given hashtable for a key, returning it if found,
   NULL if not found.


   @see cwal_hash_search()
*/
cwal_value * cwal_hash_search_v( cwal_hash * h, cwal_value const * key );

/**
   Equivalent to cwal_hash_search_v() but returns (on a match) a
   cwal_kvp holding the key/value pair. The returned object is owned
   by h and might be invalidated or modified on any change to h, so
   clients must not hold returned kvp wrapper for long.
*/
cwal_kvp * cwal_hash_search_kvp_v( cwal_hash * h, cwal_value const * key );

/**
   Like cwal_hash_search_v() but takes its key in the form of the
   first keyLen bytes of the given key. It will only ever match true
   string keys, not non-string keys which might otherwise compare (via
   cwal_value_compare()) to equivalent.

   Returns NULL if !h or !key. If keyLen is 0 and *key is not then
   the equivalent of strlen(key) is used to find its length.

   @see cwal_hash_search_v()
   @see cwal_hash_search_kvp()
*/
cwal_value * cwal_hash_search( cwal_hash * h, char const * key,
                               cwal_midsize_t keyLen );


/**
   Equivalent to cwal_hash_search() but returns (on a match) a
   cwal_kvp holding the key/value pair, using the first keyLen bytes
   of key as the search key. It only matches String-typed keys.  The
   returned object is owned by h and might be invalidated or modified
   on any change to h, so clients must not hold returned kvp wrapper
   for long. NEVER, EVER change the internals of the returned cwal_kvp
   value, e.g. changing the key or value instances, as that Will Break
   Things.

   @see cwal_hash_search()
*/
cwal_kvp * cwal_hash_search_kvp( cwal_hash * h, char const * key,
                                 cwal_midsize_t keyLen );

/**
   Returns true (non-0) if v is of the concrete type cwal_hash.
*/
bool cwal_value_is_hash( cwal_value const * v );

/**
   If cwal_value_is_hash(v) then this returns the value's
   cwal_hash representation, else it returns NULL.
*/
cwal_hash * cwal_value_get_hash( cwal_value * v );

/**
   Returns the cwal_value part of a cwal_hash value,
   or NULL if !h.
*/
cwal_value * cwal_hash_value( cwal_hash * h );

/**
   Removes all entries from the hashtable. If freeProps is true then
   non-hash properties (those belonging to the object base type) are
   also cleared. After calling this, cwal_hash_entry_count() will be 0
   until new entries are added.  The hashtable size is not modified by
   this routine.
*/
void cwal_hash_clear( cwal_hash * ar, bool freeProps );

    
/**
   Inserts a value into the given hash. 

   Returns 0 on success. If the given key already exists then
   insertion fails and CWAL_RC_ALREADY_EXISTS is returned unless
   allowOverwrite is true (in which case the entry is replaced). If
   allowOverwrite is true but an existing entry is marked with the
   CWAL_VAR_F_CONST flag, CWAL_RC_CONST_VIOLATION is returned.

   On error the key and value acquire no new references.

   This function returns CWAL_RC_IS_VISITING_LIST if called while h's
   hash properties are being iterated over (e.g. via
   cwal_hash_visit_kvp() and friends), as the data structures do not
   support modification during iteration. (Prior to 20191211,
   CWAL_RC_IS_VISITING was returned for this case.)

   The kvpFlags parameter is treated as described for
   cwal_prop_set_with_flags(). When in doubt, pass
   CWAL_VAR_F_PRESERVE.

   ACHTUNG: when overwriting an existing equivalent key, the older key
   is replaced by the newer one. This is to avoid uncomfortable
   questions about the lifetime management of newly-created keys (the
   management of the older/longer-lived ones is simpler!). This means
   that when replacing an entry, the original copy may (depending on
   references) get freed immediately. The moral of this story is:
   don't hold (cwal_value*) to such keys without holding a reference
   to them (and releasing it when done), or they could be invalidated
   (or their memory reused) in the mean time.

   Other error codes:

   - CWAL_RC_TYPE if cwal_prop_key_can() returns false for the given
   key.

   - CWAL_RC_MISUSE if any arguments are invalid.

   - CWAL_RC_DISALLOW_NEW_PROPERTIES: the container has been flaged
   with the CWAL_CONTAINER_DISALLOW_NEW_PROPERTIES flag and key refers
   to a property which is not in the container.

   - CWAL_RC_DISALLOW_PROP_SET: the container has been flaged with the
   CWAL_CONTAINER_DISALLOW_PROP_SET flag. TODO: this is arguable, as
   that flag was initially intended to apply only to object-level
   properties. We might need another flag/code for this, but cannot do
   so without making the same change in s2 (which uses a mix of
   objects and hashes to implement enums with this flag).

   - CWAL_RC_DESTRUCTION_RUNNING: the container is currently being
   destructed. It "should" be impossible for clients to ever trigger
   this.


*/
int cwal_hash_insert_with_flags_v( cwal_hash * h, cwal_value * key, cwal_value * v,
                                   bool allowOverwrite, cwal_flags16_t kvpFlags );

/**
   Equivalent to calling cwal_hash_insert_with_flags_v() with the same arguments,
   passing CWAL_VAR_F_PRESERVE as the final argument.
*/
int cwal_hash_insert_v( cwal_hash * h, cwal_value * key, cwal_value * v,
                        bool allowOverwrite );

/**
   Like cwal_hash_insert_with_flags_v() but takes its key in the form
   of the first keyLen bytes of the given key.

   This routine allocates a new String value for the key (just in
   case there was any doubt about that).
*/
int cwal_hash_insert_with_flags( cwal_hash * h, char const * key,
                                 cwal_midsize_t keyLen,
                                 cwal_value * v, bool allowOverwrite,
                                 cwal_flags16_t kvpFlags );

/**
   Equivalent to calling cwal_hash_insert_with_flags() with the same arguments,
   passing CWAL_VAR_F_PRESERVE as the final argument.
*/
int cwal_hash_insert( cwal_hash * h, char const * key, cwal_midsize_t keyLen,
                      cwal_value * v, bool allowOverwrite );
/**
   Removes the given key from the given hashtable, potentially
   freeing the value (and possibly even the passed-in key,
   depening on ownership conditions).

   Returns 0 on success, CWAL_RC_MISUSE if either argument is
   NULL, or CWAL_RC_NOT_FOUND if the entry is not found.

   This function returns CWAL_RC_IS_VISITING_LIST if called while h's
   hashtable part is being iterated over (e.g. via
   cwal_hash_visit_kvp() and friends), as modifying the hash during
   iteration could potentially lead the memory-related problems. (This
   case was reported as CWAL_RC_IS_VISITING prior to 20191211.)

   Returns CWAL_RC_DISALLOW_PROP_SET if the container has been flagged
   with the CWAL_CONTAINER_DISALLOW_PROP_SET flag.

   Returns CWAL_RC_DESTRUCTION_RUNNING: the container is currently
   being destructed. It "should" be impossible for clients to ever
   trigger this.
*/
int cwal_hash_remove_v( cwal_hash * h, cwal_value * key );

/**
   Like cwal_hash_remove_v(), with the same result codes, but takes
   its key in the form of the first keyLen bytes of the given key. It
   can only match String-type keys, not non-String-type keys which
   might happen to have similar representations. e.g. passing "1" as a
   key will not match an Integer-typed key with the numeric value 1.
*/
int cwal_hash_remove( cwal_hash * h, char const * key, cwal_midsize_t keyLen );

/**
   Returns the number of entries in the given hash,
   or 0 if !h. This is an O(1) operation.
*/
cwal_midsize_t cwal_hash_entry_count(cwal_hash const * h);

/**
   Returns the table size of h, or 0 if !h.
*/
cwal_midsize_t cwal_hash_size( cwal_hash const * h );

/**
   Resizes the table used by h for key/value storage to the new
   size (which most definitely should be a prime number!). On
   success it returns 0 and cwal_hash_size() will return the size
   passed here. This is a no-op (returning 0) if newSize ==
   cwal_hash_size(h).

   On error it returns...

   - CWAL_RC_MISUSE if !h.

   - CWAL_RC_RANGE if !newSize.

   - CWAL_RC_IS_VISITING_LIST if h's hashtable part is currently being
   "visited" (iterated over), as resizing is not legal while that is
   happening. (This case was reported as CWAL_RC_IS_VISITING prior to
   20191211.)

   - CWAL_RC_OOM on an allocation error. If this happens, h is
   left in its previous state (it is not modified). If allocation
   of the new table succeeds, no other ops performed by this call
   which modify h can fail (exception in debug builds:
   lifetime-related assertions are in place which could be
   triggered if any key/value lifetimes appear to have been
   corrupted before this call).
*/
int cwal_hash_resize( cwal_hash * h, cwal_size_t newSize );


/**
   If h's entry count is at least the 2nd value (load*100) percent of
   its table size, the table is resized to be (roughly) within that
   load size. load must be a value greater than 0, and ideally less an
   1.  e.g. 0.80 means an 80% load factor. Unusually large or small
   values may be trimmed to within some min/max range. A value of 0 or
   less will use the library's built-in default.

   Returns:

   - 0 on success (which may mean it did nothing at all).

   - CWAL_RC_OOM if allocation of a larger hash table size fails.

   - CWAL_RC_IS_VISITING_LIST if called while h's hashtable part is
   currently being iterated over or is otherwise locked against
   concurrent modification.

   Results are undefined if h is NULL or otherwise not a valid
   pointer.
*/
int cwal_hash_grow_if_loaded( cwal_hash * h, double load );

/**
   Returns the "next higher" prime number starting at (n+1), where
   "next" is really the next one in a short list of rather arbitrary
   prime numbers.  If n is larger than some value which we internally
   (and almost arbitrarily) define in this code, a value smaller than
   n will be returned. That maximum value is guaranteed to be at least
   as large as the 1000th prime number (7919 resp.
   cwal_first_1000_primes()[999]).

   This function makes no performance guarantees.
*/
cwal_midsize_t cwal_next_prime( cwal_midsize_t n );

/**
   _Moves_ properties from the containter-type value src to the hash
   table dest. overwritePolicy determines how to handle keys which
   dest already contains:

   overwritePolicy<0: keep existing entries and leave the colliding
   key from src in place.

   overwritePolicy==0: trigger CWAL_RC_ALREADY_EXISTS error on key
   collision.

   overwritePolicy>0: overwrite any keys already existing in the hash
   and removes the property from src.

   If a given property, due to the overwrite policy, is not taken then
   it is kept in src, so src need not be empty when this returns unless
   overwritePolicy>0.

   Property-level key/value pair flags, e.g. constness, are not
   retained.

   Sidebar 1: ideally, this routine moves properties in such a way
   that does not require new allocations, but that's currently only
   the case when the build-time CWAL_OBASE_ISA_HASH option is
   disabled.

   Sidebar 2: it is legal for src and dest to be the same cwal_value,
   in which case its object-level properties are moved into its
   hashtable.

   Returns 0 on success or:

   - CWAL_RC_OOM on allocation error.

   - CWAL_RC_ALREADY_EXISTS if overwritePolicy is 0 and a collision is
   found.

   - CWAL_RC_MISUSE if !src or !dest

   - CWAL_RC_TYPE if src is not a container type.

   - CWAL_RC_IS_VISITING if src is currently being visited (the model
   does not support modification during visitation).

   - CWAL_RC_IS_VISITING_LIST if dest's hash properties (as distinct
   from its base object-level properties) are currently being visited
   (the model does not support modification during visitation). (This
   case was reported as CWAL_RC_IS_VISITING prior to 20191211.)

   On error, src's property list may well have been modified so may be
   in an undefined state, with a subset of the properties moved and a
   subset not.
*/
int cwal_hash_take_props( cwal_hash * const dest, cwal_value * const src, int overwritePolicy );


/**
   Similar to cwal_props_visit_kvp() except that it operates on
   the hash table entries of h. See cwal_props_visit_kvp() for the
   semantics of the visitor and its return value.
*/
int cwal_hash_visit_kvp( cwal_hash * h, cwal_kvp_visitor_f f, void * state );

/**
   Equivalent to cwal_props_visit_keys() except that it operates
   on the hash table entries of h, passing each key in the hashtable
   to f (in an indeterminate order).
*/
int cwal_hash_visit_keys( cwal_hash * h, cwal_value_visitor_f f, void * state );

/**
   Equivalent to cwal_props_visit_keys() except that it operates
   on the hash table entries of h, passing each value in the table
   to f (in an indeterminate order).
*/
int cwal_hash_visit_values( cwal_hash * h, cwal_value_visitor_f f, void * state );

/**
   Hashes n bytes of m and returns its hash. It uses an unspecified
   hash algorithm which is not guaranteed to be stable across
   platforms or compilations of this code. It is guaranteed to be
   deterministic within a single compilation of this code.  m must not
   be NULL.
*/
cwal_hash_t cwal_hash_bytes( void const * m, cwal_size_t n );

/**
   Converts v to a string representation and copies it to dest.  dest
   must be valid memory at least *nDest bytes long. On success (*nDest
   is long enough to hold the number and trailing NUL) then *nDest is
   set to the size of the string (minus the trailing NUL) and dest is
   updated with its contents.

   Returns CWAL_RC_OK on success, else:

   CWAL_RC_MISUSE: dest or nDest are NULL.

   CWAL_RC_RANGE: *nDest is not enough to hold the resulting string
   (including terminating NUL). dest is not modified in this case, but
   *nDest is updated to contain the size which would be needed to
   write the full value.

   For normal use cases, a memory length of 30 or less is more
   than sufficient.
*/
int cwal_int_to_cstr( cwal_int_t v, char * dest, cwal_size_t * nDest );

/**
   Functionally identical to cwal_int_to_cstr() but works on a
   double value.

   For normal use cases, a memory length of 128 or less is more
   than sufficient. The largest result i've ever witnessed was
   about 80 bytes long.

   Prior to 20181127, this routine would output "scientific notation"
   for large/precise-enough numbers, but that was an oversight. It now
   always writes in normal decimal form. If it ever tries to write
   
*/
int cwal_double_to_cstr( cwal_double_t v, char * dest, cwal_size_t * nDest );

/**
   Tries to interpret slen bytes of cstr as an integer value,
   optionally prefixed by a '+' or '-' character. On success 0 is
   returned and *dest (if dest is not NULL) will contain the
   parsed value. On error one of the following is returned:

   - CWAL_RC_MISUSE if !slen, !cstr, or !*cstr.

   - CWAL_RC_TYPE if cstr contains any non-numeric characters.

   - CWAL_RC_RANGE if the numeric string is too large for
   cwal_int_t.

   Potential TODOs: hex with leading 0x or 0X, and octal with
   leading 0o.
*/
int cwal_cstr_to_int( char const * cstr, cwal_size_t slen, cwal_int_t * dest );

/**
   Equivalent to cwal_cstr_to_int() but takes a cwal_string value.
   Returns CWAL_RC_MISUSE if !s, else returns as for
   cwal_cstr_to_int().
*/
int cwal_string_to_int( cwal_string const * s, cwal_int_t * dest );

/**
   Behaves as for cwal_cstr_to_int(), but parses an integer or
   literal double (in decimal form) with an optional leading sign.
*/
int cwal_cstr_to_double( char const * cstr, cwal_size_t slen, cwal_double_t * dest );

/**
   The cwal_string counterpart of cwal_cstr_to_double().
*/
int cwal_string_to_double( cwal_string const * s, cwal_double_t * dest );
    
/**
   Compares the two given strings using memcmp() semantics with
   these exceptions:

   if either of len1 or len2 are 0 then the longer of the two
   strings compares de facto (without a string comparison) to
   greater than the other. If both are 0 they are compared
   as equal.

   len1 and len2 MUST point to their respective number of bytes of
   live memory. If they are 0 their corresponding string is not
   touched. i.e. s1 may be NULL only if len1 is 0, and likewise
   for (s2,len2).
*/
int cwal_compare_cstr( char const * s1, cwal_size_t len1,
                       char const * s2, cwal_size_t len2 );

/**
   A cwal_compare_cstr() proxy which compares the given cwal_string
   to the given c-style string.
*/
int cwal_compare_str_cstr( cwal_string const * s1,
                           char const * s2, cwal_size_t len2 );

/**
   Configures e to recycle, at most, n elements for the given
   type.  If the recycle list already contains more than that then
   any extra elements in it are freed by this call. Set it to 0 to
   disable recycling for the given type.

   typeID must be one of:

   CWAL_TYPE_INTEGER, CWAL_TYPE_DOUBLE (see notes below!),
   CWAL_TYPE_OBJECT, CWAL_TYPE_ARRAY, CWAL_TYPE_NATIVE,
   CWAL_TYPE_BUFFER, CWAL_TYPE_KVP, CWAL_TYPE_WEAK_REF,
   CWAL_TYPE_STRING (but see below regarding strings).

   Or, as a special case, CWAL_TYPE_UNDEF means to apply this
   change to all of the above-listed types.

   Also note that any built-in constant values are never
   allocated, and so are not recycled via this mechanism.
       
   Returns 0 on succes, CWAL_RC_MISUSE if !e, and CWAL_RC_TYPE if
   typeID is not refer to one of the recyclable types.

   Notes:

   ACHTUNG: As of 20141129, cwal groups the recycling bins by the size
   of the Value type, and that sizing is platform-dependent and
   determined at runtime. It is not possible for clients to determine
   which types are grouped together, which means that this approach to
   configuring the bin sizes is not as useful as it was before that
   change. e.g. it may well be that Hashes and Buffers share the same
   bin as Arrays, making it impossible to size the bins for exactly
   per type (but giving us better recycling overall). THEREFORE...
   the (probably) best approach to using this function is to call it
   in the order of your preferred priority (highest sizes last), so
   that any shared bins will get the highest size. e.g. passing it
   CWAL_TYPE_INTEGER after CWAL_TYPE_DOUBLE will ensure that the
   INTEGER recycle bin size is used on platforms where those types
   have the same size.

   CWAL_TYPE_KVP is an internal type with no cwal_value
   representation. Each key/value pair in an Object requires one
   instance of cwal_kvp, and clients can control that recycling
   level here.

   CWAL_TYPE_WEAK_REF is an internal type with no cwal_value
   representation. We do, however, recycle them, if they are
   configured for it.

   CWAL_TYPE_XSTRING and CWAL_TYPE_ZSTRING are equivalent here,
   as those types use the same recycling bin.

   CWAL_TYPE_STRING recycling is comparatively limited because a
   string's size plays a factor in its reusability. When choosing
   strings from the recycling pool, only strings with the same
   approximate length will be considered. This means it is
   possible, depending on usage, to fill up the recycle pool with
   strings of sizes we won't ever recycle. Internally, the library
   pads new string sizes up to some common boundary because doing
   so saves memory (somewhat ironically) by improving recylability
   of strings from exact-fit-only to a close-fit.
*/
int cwal_engine_recycle_max( cwal_engine * e, cwal_type_id type, cwal_size_t n );

/**
   For the given cwal value type ID, this function returns the
   maximum number of values of that type which e is configured to
   keep in its recycle bin. Returns 0 if !e or recycling is
   disabled or not allowed for the given type.

   Example:

   @code
   cwal_size_t const x = cwal_engine_recyle_max_get(e, CWAL_TYPE_OBJECT);
   @endcode
*/
cwal_size_t cwal_engine_recycle_max_get( cwal_engine * e, cwal_type_id type );

/**
   Sets up e's memory chunk recycler. It must currently be called
   before any memory has been placed in that recycler (i.e. during
   engine initialization). config's contents are copied into e, so the
   object need not live longer than this call.

   Returns:

   - 0 on success

   - CWAL_RC_MISUSE if !e or !config.

   - CWAL_RC_OOM if growing the table fails.

   The ability to resize it at runtime is on the TODO list. It's
   actually implemented but completely untested.
*/
int cwal_engine_memchunk_config( cwal_engine * e,
                                 cwal_memchunk_config const * config);

/**
   Runs the type-specific equivalence comparison operation for lhs
   and rhs, using memcmp() semantics: returns 0 if lhs and rhs are
   equivalent, less than 0 if lhs is "less than" rhs, and greater
   than 0 if lhs is "greater than" rhs. Note that many types do
   not have any sort of sensible orderings. This API attempts to
   do something close to ECMAScript, but it does not exactly match
   that.

   Note that this function does not guaranty return values of
   exactly -1, 0, or 1, but may return any (perhaps varying)
   negative resp. positive values.

   TODO: find the appropriate place to document the cross-type
   comparisons and weird cases like undefined/null.

   Notes:

   - CWAL_TYPE_NULL and CWAL_TYPE_UNDEF compare equivalently to
   any falsy value. (This was not true before 20140614, but no
   known current code was broken by that change.)
*/
int cwal_value_compare( cwal_value const * lhs, cwal_value const * rhs );
    
#if 0
/* th1 has something like this... */
int cwal_engine_call_scoped( cwal_engine * e,
                             int (*callback)(cwal_engine *e, void * state1, void * state2) ); 
#endif
/**
   A generic interface for callback functions which act as a
   streaming input source for... well, for whatever.

   The arguments are:

   - state: implementation-specific state needed by the function.

   - n: when called, *n will be the number of bytes the function
   should read and copy to dest. The function MUST NOT copy more than
   *n bytes to dest. Before returning, *n must be set to the number of
   bytes actually copied to dest. If that number is smaller than the
   original *n value, the input is assumed to be completed (thus this
   is not useful with non-blocking readers).

   - dest: the destination memory to copy the data to.

   Must return 0 on success, non-0 on error (preferably a value from
   cwal_rc).

   There may be specific limitations imposed upon implementations
   or extra effort required by clients.  e.g. a text input parser
   may need to take care to accommodate that this routine might
   fetch a partial character from a UTF multi-byte character.
*/
typedef int (*cwal_input_f)( void * state, void * dest, cwal_size_t * n );

/**
   A cwal_input_f() implementation which requires the state argument
   to be a readable (FILE*) handle.
*/
int cwal_input_f_FILE( void * state, void * dest, cwal_size_t * n );

/**
   A generic streaming routine which copies data from a
   cwal_input_f() to a cwal_outpuf_f().

   Reads all data from inF() in chunks of an unspecified size and
   passes them on to outF(). It reads until inF() returns fewer
   bytes than requested. Returns the result of the last call to
   outF() or (only if reading fails) inF(). Returns CWAL_RC_MISUSE
   if inF or ouF are NULL.

   Here is an example which basically does the same thing as the
   cat(1) command on Unix systems:

   @code
   cwal_stream( cwal_input_f_FILE, stdin, cwal_output_f_FILE, stdout );
   @endcode

   Or copy a FILE to a buffer:

   @code
   cwal_buffer myBuf = cwal_buffer_empty;
   cwal_output_buffer_state outState;
   outState.b = &myBuf;
   outState.e = myCwalEngine;
   rc = cwal_stream( cwal_input_f_FILE, stdin, cwal_output_f_buffer, &outState );
   // Note that on error myBuf might be partially populated.
   // Eventually clean up the buffer:
   cwal_buffer_clear(&myBuf);
   @endcode
*/
int cwal_stream( cwal_input_f inF, void * inState,
                 cwal_output_f outF, void * outState );


/**
   Reserves the given amount of memory for the given buffer object.

   If n is 0 then buf->mem is freed and its state is set to
   NULL/0 values.

   If buf->capacity is less than or equal to n then 0 is returned and
   buf is not modified.

   If n is larger than buf->capacity then buf->mem is (re)allocated
   and buf->capacity contains the new length. Newly-allocated bytes
   are filled with zeroes.

   On success 0 is returned. On error non-0 is returned and buf is not
   modified.

   buf->mem is owned by buf and must eventually be freed by passing an
   n value of 0 to this function.

   buf->used is never modified by this function unless n is 0, in which case
   it is reset.

   Example:

   @code
   cwal_buffer buf = cwal_buffer_empty; // VERY IMPORTANT: copy initialization!
   int rc = cwal_buffer_reserve( e, &buf, 1234 );
   ...
   cwal_buffer_reserve( e, &buf, 0 ); // frees the memory
   @endcode
*/
int cwal_buffer_reserve( cwal_engine * e, cwal_buffer * buf, cwal_size_t n );

/**
   Fills all bytes of the given buffer with the given character.
   Returns the number of bytes set (buf->capacity), or 0 if
   !buf or buf has no memory allocated to it.
*/
cwal_size_t cwal_buffer_fill( cwal_buffer * buf, unsigned char c );

/**
   Uses a cwal_input_f() function to buffer input into a
   cwal_buffer.

   dest must be a non-NULL, initialized (though possibly empty)
   cwal_buffer object. Its contents, if any, will be overwritten by
   this function, and any memory it holds might be re-used.

   The src function is called, and passed the state parameter, to
   fetch the input. If it returns non-0, this function returns that
   error code. src() is called, possibly repeatedly, until it reports
   that there is no more data.

   Whether or not this function succeeds, dest still owns any memory
   pointed to by dest->mem, and the client must eventually free it by
   calling cwal_buffer_reserve(dest,0).

   dest->mem might (and possibly will) be (re)allocated by this
   function, so any pointers to it held from before this call might be
   invalidated by this call.
   
   On error non-0 is returned and dest has almost certainly been
   modified but its state must be considered incomplete.

   Errors include:

   - dest or src are NULL (CWAL_RC_MISUSE)

   - Allocation error (CWAL_RC_OOM)

   - src() returns an error code (that code is returned).

   Whether or not the state parameter may be NULL depends on
   the src implementation requirements.

   On success dest will contain the contents read from the input
   source. dest->used will be the length of the read-in data, and
   dest->mem will point to the memory. dest->mem is automatically
   NUL-terminated if this function succeeds, but dest->used does not
   count that terminator. On error the state of dest->mem must be
   considered incomplete, and is not guaranteed to be NUL-terminated.

   Example usage:

   @code
   cwal_buffer buf = cwal_buffer_empty;
   int rc = cwal_buffer_fill_from( engine, &buf, cwal_input_f_FILE,
   stdin );
   if( rc ){
   fprintf(stderr,"Error %d (%s) while filling buffer.\n",
   rc, cwal_rc_cstr(rc));
   cwal_buffer_reserve( engine, &buf, 0 ); // might be partially populated
   return ...;
   }
   ... use the contents via buf->mem ...
   ... clean up the buffer ...
   cwal_buffer_reserve( engine, &buf, 0 );
   @endcode

   To take over ownership of the buffer's memory, do:

   @code
   void * mem = buf.mem;
   buf = cwal_buffer_empty;
   @endcode

   In which case the memory must eventually be passed to cwal_free()
   to free it.

   TODO: add a flag which tells it whether to append or overwrite the
   contents, or add a second form of this function which appends
   rather than overwrites.
*/
int cwal_buffer_fill_from( cwal_engine * e, cwal_buffer * dest, cwal_input_f src, void * state );

/**
   A cwal_buffer_fill_from() proxy which overwrite's dest->mem
   with the contents of the given FILE handler (which must be
   opened for read access).  Returns 0 on success, after which
   dest->mem contains dest->used bytes of content from the input
   source. On error dest may be partially filled.
*/
int cwal_buffer_fill_from_FILE( cwal_engine * e, cwal_buffer * dest, FILE * src );

/**
   Wrapper for cwal_buffer_fill_from_FILE() which gets its input
   from the given file name. As a special case it interprets the
   name "-" as stdin.
*/
int cwal_buffer_fill_from_filename( cwal_engine * e, cwal_buffer * dest, char const * filename );    

/**
   Works just like cwal_buffer_fill_from_filename() except that it
   takes a required length for the filename. This routine uses an
   internal buffer to copy (on the stack) the given name and
   NUL-terminate it at the nameLen'th byte. This is intended to
   help protect against potentially non-NUL-terminated input
   strings, e.g. from X- or Z-strings.

   Returns 0 on success, CWAL_RC_MISUSE if any pointer argument is
   0, and CWAL_RC_RANGE if nameLen is larger than the internal
   name buffer (of "some reasonable size").
*/
int cwal_buffer_fill_from_filename2( cwal_engine * e, cwal_buffer * dest,
                                     char const * filename,
                                     cwal_size_t nameLen);


/**
   Sets the "used" size of b to 0 and NULs the first byte of
   b->mem if b->capacity is greater than 0. DOES NOT deallocate
   any memory.

   Returns 0 on success and the only error case is if !b
   (CWAL_RC_MISUSE).

   @see cwal_buffer_reserve()
*/
int cwal_buffer_reset( cwal_buffer * b );

/**
   Similar to cwal_buffer_reserve() except that...

   - It does not free all memory when n==0. Instead it essentially
   makes the memory a length-0, NUL-terminated string.

   - It will try to shrink (realloc) buf's memory if (n<buf->capacity).

   - It sets buf->capacity to (n+1) and buf->used to n. This routine
   allocates one extra byte to ensure that buf is always
   NUL-terminated.

   - On success it always NUL-terminates the buffer at
   offset buf->used.

   Returns 0 on success, CWAL_RC_MISUSE if !buf, CWAL_RC_OOM if
   (re)allocation fails.

   @see cwal_buffer_reserve()
   @see cwal_buffer_clear()
*/
int cwal_buffer_resize( cwal_engine * e, cwal_buffer * buf, cwal_size_t n );


/**
   Convenience equivalent to cwal_buffer_reserve(e, b, 0).
*/
int cwal_buffer_clear( cwal_engine * e, cwal_buffer * b );

/**
   Appends the first n bytes of data to b->mem at position
   b->used, expanding b if necessary. Returns 0 on success. If
   !data then CWAL_RC_MISUSE is returned.  This function
   NUL-terminates b on success.
*/
int cwal_buffer_append( cwal_engine * e, cwal_buffer * b, void const * data, cwal_size_t n );
    
/**
   Appends printf-style formatted bytes to b using
   cwal_printf(). Returns 0 on success.  Always NUL-terminates the
   buffer on success, but that NUL byte does not count against
   b->used's length.

   If it detects an error while appending to the buffer, it resets
   b->used to the length it had before calling this, and
   NUL-terminates b->mem (if not NULL) at that position. i.e. the
   visible effect on the buffer is as if this has not been called.
*/
int cwal_buffer_printf( cwal_engine * e, cwal_buffer * b, char const * fmt, ... );

/**
   Equivalent to cwal_buffer_printf() but takes a va_list instead
   of ellipsis.
*/
int cwal_buffer_printfv( cwal_engine * e, cwal_buffer * b, char const * fmt, va_list );


/**
   A string formatting function similar to Java's
   java.lang.String.format(), with similar formatting rules.  It
   uses a formatting string to describe how to convert its
   arguments to a formatted string, and appends the output to a
   cwal_buffer instance.

   Overview of formatting rules:

   A format specifier has this syntax:

   %N$[flags][[-]width][.precision][type]

   "%%" is interpreted as a single "%" character, not a format
   specifier.

   N = the 1-based argument (argv) index. It is 1-based because
   that is how java.lang.String.format() does it. The argv value
   at that index is expected to be of the type(s) specified by the
   format specifier, or convertible to that type.

   How the width and precision are handled varies by type. TODO:
   document the various behaviours and ensure semantic
   compatibility (or close) with java.lang.String.format().
       
   [type] must one of the following:

   - b: treat the argument as a boolean, evaluate to "true" or
   "false". Width and precision are ignored. (TODO: treat
   width/precision as padding/truncation, as for strings.)
       
   - B: "blobifies" the argument (which must be a Buffer or
   String), encoding it as a series of hex values, two hex
   characters per byte of length. The precision specifies the
   maximum number of byte pairs to output (so the formatted length
   will be twice the precision).
       
   - d, o, x, X: means interpret the result as an integer in
   decimal, octal, hex (lower-case), or hex (upper-case),
   respectively. If a width is specified and starts with a '0'
   then '0' (instead of ' ') is used for left-side padding if the
   number is shorter than the specified width.  Precision is
   ignored(?).

   - f: double value. Width and precision work like cwal_outputf()
   and friends.

   - J: runs the value through cwal_json_output() to convert it to
   a JSON string. The width can be used to specify
   indentation. Positive values indent by that many spaces per
   level, negative values indicate that many hard tabs per
   level. The precision is ignored.
      
   - N, U: interpret the value as "null" or "undefined",
   respectively. Width and precision are ignored.
       
   - p: evaluates to a string in the form TYPE_NAME\@ADDRESS, using
   the hex notation form of the value's address. Width and
   precision are ignored.

   - q: expects a string or NULL value. Replaces single-quote
   characters with two single-quote characters and interpets NULL
   values as "(NULL)" (without the quotes).

   - Q: like 'q' but surrounds string ouput with single quotes and
   interprets NULL values as "NULL" (without the quotes).
       
   - s: string or buffer value. The precision determines the
   maximum length. The width determines the minimum length.  If
   the string is shorter (in bytes!) than the absolute value of
   the width then a positive width will left-pad the string with
   spaces and a negative width will left-pad the string with
   spaces.  FIXME: USE UTF8 CHARS for precision and width!

   - y: evaluates to cwal_value_type_name(argv[theIndex]). Width
   and precision are ignored.

   The flags field may currently only be a '+', which forces
   numeric conversions to include a sign character. This sign
   character does not count against the width/precision.
       
   Anything which is not a format specifier is appended as-is to
   tgt.

   Note that tgt is appended to by this function, so when re-using
   a buffer one may either need to set tgt->used=0 before calling
   this or the caller should copy tgt->used before calling this
   function and treating (tgt->mem + preCallUsed) as the start of the
   output and (tgt->used - preCallUsed) as its length.

   Note that this function might reallocate tgt->mem, so any
   pointers to it may be invalidated.

   Returns 0 on success. On error it returns non-0 and may replace
   the contents of tgt->mem with an error string. It will do this
   for all cases exception invalid arguments being passed to this
   function (CWAL_RC_MISUSE) or an allocation error
   (CWAL_RC_OOM). For all others, on error it writes an error
   message (NUL-terminated) to (tgt->mem + (tgt->used when this
   function was called)).


   TODO: refactor this to take a cwal_output_f() instead of a
   buffer then reimplement this function on top of that one.
*/
int cwal_buffer_format( cwal_engine * e, cwal_buffer * tgt,
                        char const * fmt, cwal_size_t fmtLen,
                        uint16_t argc, cwal_value * const * const argv);

/**
   Searches the give buffer for byte sequences matching the first
   needleLen bytes of needle with the first replLen bytes of repl.
   needle may not be NULL and needleLen must be greater than 0.
   replLen may be 0 and repl may not be NULL unless replLen is 0.

   needle is expected to be valid UTF8, but repl is not strictly
   required to be. Results are undefined if needle is not valid UTF8
   (e.g. if needle starts part-way through a multi-byte character or
   if needleLen truncates needle part-way through one).

   If limit>0 then that specifies the maximum number of replacements
   to make. If limit is 0 then all matches are replaced.

   If changeCount is not NULL then *changeCount it is set to the
   number of changes made (regardless of success or failure).

   ACHTUNG: this function needs to create a temporary buffer to work
   on and it will (on success) swap out buf's contents with those of
   the working buffer. Thus any pointers to buf->mem held before this
   call will almost certainly (except in a couple rare corner cases)
   be invalidated by this call.

   On success, returns 0 and replaces any matches in the buffer (up to
   the specified limit, if any). On error, buf's contents/state are
   not modified and non-0 is returned:

   - CWAL_RC_OOM if a memory allocation fails.

   - CWAL_RC_MISUSE if any of (e, buf, needle) are NULL or (!repl &&
   replLen>0).

   - CWAL_RC_RANGE if needleLen==0 (replLen may be 0).


   @see cwal_buffer_replace_byte()
*/
int cwal_buffer_replace_str( cwal_engine * e, cwal_buffer * buf,
                             unsigned char const * needle, cwal_size_t needleLen,
                             unsigned char const * repl, cwal_size_t replLen,
                             cwal_size_t limit,
                             cwal_size_t * changeCount);

/**
   Replaces instances of the given needle byte in the given buffer
   with the given repl byte.

   If limit is 0, all matching bytes are replaced, else only the first
   limit matches are replaced.

   If changeCount is not NULL then *changeCount is assigned to the
   number of changes made (regardless of success or failure). As a
   special case, if needle==repl then no changes are made.

   Returns 0 on success or CWAL_RC_MISUSE if either of (e, buf) are
   NULL.

   Unlike cwal_buffer_replace_str(), this function modifies the input
   buffer in-place and does not risk modifying (via reallocation) the
   buf->mem address.

   @see cwal_buffer_replace_str()
*/
int cwal_buffer_replace_byte( cwal_engine * e, cwal_buffer * buf,
                              unsigned char needle, unsigned char repl,
                              cwal_size_t limit,
                              cwal_size_t * changeCount);


/**
   Client-configurable options for the cwal_json_output() family of
   functions.
*/
struct cwal_json_output_opt{
  /**
     Specifies how to indent (or not) output. The values
     are:

     (0) == no extra indentation.
       
     (-N) == -N TAB character for each level.

     (N) == N SPACES for each level.

     TODO: replace or supplement this with a ((char const *),
     length) pair.
  */
  int indent;
    
  /**
     indentString offers a more flexible indentation method over the
     (older) indent member. cwal_json_output() will use indentString
     instead of indent if indentString.str is not NULL.
  */
  struct {
    /**
       String to use for each level of indentation. The client
       must ensure that these bytes outlive this object or
       behaviour is undefined.
    */
    char const * str;
    /**
       Number of bytes of this.str to use for each level of
       indentation.
    */
    cwal_size_t len;
  } indentString;

  /**
     Maximum object/array depth to traverse. Traversing deeply can
     be indicative of cycles in the containers, and this value is
     used to figure out when to abort the traversal. If JSON output
     is triggered by this constraint, the result code will be
     CWAL_RC_RANGE.
  */
  unsigned short maxDepth;
    
  /**
     If true, a newline will be added to the end of the generated
     output, else not.
  */
  char addNewline;

  /**
     If true, a space will be added after the colon operator
     in objects' key/value pairs.
  */
  char addSpaceAfterColon;

  /**
     If true, a space will be appended after commas in array/object
     lists, else no space will be appended.
  */
  char addSpaceAfterComma;

  /**
     If set to 1 then objects/arrays containing only a single value
     will not indent an extra level for that value (but will indent
     on subsequent levels if that value contains multiple values).
  */
  char indentSingleMemberValues;

  /**
     The JSON format allows, but does not require, JSON generators
     to backslash-escape forward slashes. This option enables/disables
     that feature. According to JSON's inventor, Douglas Crockford:

     (quote)
     It is allowed, not required. It is allowed so that JSON can be
     safely embedded in HTML, which can freak out when seeing
     strings containing "</". JSON tolerates "<\/" for this reason.
     (/quote)

     (from an email on 2011-04-08)

     The default value is 0 (because escaped forward slashes are
     just damned ugly).
  */
  char escapeForwardSlashes;

  /**
     If true, cyclic structures will not cause an error, but will
     instead be replaced by a symbolic (but useless) placeholder
     string indicating which value cycled. Useful primarily for
     debugging, and not for generating usable JSON output.
  */
  char cyclesAsStrings;

  /**
     If true, Function values will be output as objects, otherwise
     they will trigger a CWAL_RC_TYPE error.
  */
  char functionsAsObjects;
};
typedef struct cwal_json_output_opt cwal_json_output_opt;

/** @def cwal_json_output_opt_empty_m

    Empty-initialized cwal_json_output_opt object. Example
    usage:

    @code
    struct {
    ...
    cwal_json_output_opt opt;
    } blah = {
    ...
    cwal_json_output_opt_empty_m
    };
    @endcode
*/
#define cwal_json_output_opt_empty_m { 0/*indent*/, \
    {/*indentString*/0,0},                          \
      15/*maxDepth*/,                               \
      0/*addNewline*/,                            \
      1/*addSpaceAfterColon*/,                    \
      1/*addSpaceAfterComma*/,                    \
      0/*indentSingleMemberValues*/,              \
      0/*escapeForwardSlashes*/,                  \
      0/*cyclesAsStrings*/,                       \
      1/*functionsAsObjects*/                     \
    }

/** @var cwal_json_output_opt_empty

    Empty-initialized cwal_json_output_opt object, intended
    to be used for initializing all client-side cwal_json_output_opt
    objects, e.g.:

    @code
    cwal_json_output_opt opt = cwal_json_output_opt_empty;
    @endcode

    The cwal_json_output_opt_empty_m macro is the equivalent for
    initializing in-struct members of this type.
*/
extern const cwal_json_output_opt cwal_json_output_opt_empty;

/**
   Outputs the given NON-GRAPH value in JSON format (insofar as
   possible) via the given output function. The state argument is
   passed as the first argument to f(). If f() returns non-0,
   output stops and returns that value to the caller. Note that
   f() will be called very often, so it should be relatively
   efficient.
   
   If fmt is NULL some default is used.

   This function is intended for emitting Objects and Arrays, but
   it can also do the immutable types (just don't try to hand them
   off to a downstream client as a valid JSON object).

   Note that conversion to JSON is fundamentally a const
   operation, and the value is not visibly modified, but in order
   for this code to catch cycles it must mark containers it
   visits. (It unmarks each one as it finishes traversing it.)

   Returns 0 on success.

   Returns CWAL_RC_CYCLES_DETECTED if cycles are detected while
   traversing src (TODO: this should be changed to
   CWAL_RC_IS_VISITING, as that is more precise). Returns
   CWAL_RC_RANGE if the maximum output depth level (as specified in
   the fmt argument or its default) is exceeded.
       
   ACHTUNG: this implementation assumes that all cwal_string
   values are UTF8 and may fail in mysterious ways with other
   encodings.
*/
int cwal_json_output( cwal_value * src, cwal_output_f f,
                      void * state, cwal_json_output_opt const * fmt );
/**
   A wrapper around cwal_json_output() which sends the output via
   cwal_output().
*/
int cwal_json_output_engine( cwal_engine * e, cwal_value * src,
                             cwal_json_output_opt const * fmt );
    
/**
   Wrapper around cwal_json_output() which sends its output to the given
   file handle, which must be opened in write/append mode. If fmt is NULL
   some default is used.

   Minor achtung: if fmt is NULL, this function uses a different default
   than cwal_json_output() does, and it forces the addNewline option
   to be set. If you don't want that, pass in a non-NULL fmt object.
*/
int cwal_json_output_FILE( cwal_value * src, FILE * dest,
                           cwal_json_output_opt const * fmt );

/**
   Convenience wrapper around cwal_json_output_FILE(). This function
   does NOT create directories in the given filename/path, and will
   fail if given a name which refers to a non-existing directory.

   The file name "-" is interpreted as stdout.
*/
int cwal_json_output_filename( cwal_value * src, char const * dest,
                               cwal_json_output_opt const * fmt );

/**
   Wrapper around cwal_json_output() which sends its output to the given
   buffer, which must be opened in write/append mode. If fmt is NULL
   some default is used.
*/
int cwal_json_output_buffer( cwal_engine * e, cwal_value * src,
                             cwal_buffer * dest,
                             cwal_json_output_opt const * fmt );

/**
   A class for holding JSON parser information. It is primarily
   intended for finding the nature and position of a parse error.
*/
struct cwal_json_parse_info {
  /**
     1-based line number, used for error reporting.
  */
  cwal_size_t line;
  /**
     0-based column number, used for error reporting.
  */
  cwal_size_t col;
  /**
     Length, in bytes, parsed. On error this will be "very close to"
     the error position.
  */
  cwal_size_t length;
  /**
     Error code of the parse run (0 for no error).
  */
  int errorCode;
};
typedef struct cwal_json_parse_info cwal_json_parse_info;

/**
   Empty-initialized cwal_json_parse_info object.
*/
#define cwal_json_parse_info_empty_m {          \
    1/*line*/,                                  \
    0/*col*/,                                 \
    0/*length*/,                              \
    0/*errorCode*/                            \
  }

/**
   Empty-initialized cwal_json_parse_info object. Should be copied
   by clients when they initialize an instance of this type.
*/
extern const cwal_json_parse_info cwal_json_parse_info_empty;
    
/**
   Parses input from src as a top-level JSON Object/Array value.

   The state parameter has no meaning for this function but is
   passed on to src(), so state must be compatible with the given
   src implementation.

   The pInfo parameter may be NULL. If it is not then its state is
   updated with parsing information, namely the error location.
   It is modified on success and for any parser-level error, but
   its contents on success are not likely to be useful. Likewise,
   its contents are not useful for errors triggered due to invalid
   arguments or during initial setup of the parser. The caller
   should initialize pInfo by copying cwal_json_parse_info_empty
   over it. After this returns, if pInfo->errorCode is not 0, then
   the failure was either during parsing or an allocation failed
   during parsing.

   On success, 0 is returned and *tgt is assigned to the root
   object/array of the tree (it is initially owned by the
   currently active scope). On success *tgt is guaranteed to be
   either of type Object or Array (i.e. _one_ of
   cwal_value_get_object() or cwal_value_get_array() will return
   non-NULL).

   On error non-0 is returned and *tgt is not modified. pInfo
   will, if not NULL, contain the location of the parse error (if
   any).


   ACHTUNG: if the build-time configuration option
   CWAL_ENABLE_JSON_PARSER is set to 0 then the whole family of
   cwal_json_parse() functions returns CWAL_RC_UNSUPPORTED when
   called, but they will do so after doing any normal argument
   validation, so those codes are still valid in such builds.
*/
int cwal_json_parse( cwal_engine * e, cwal_input_f src,
                     void * state, cwal_value ** tgt,
                     cwal_json_parse_info * pInfo );

/**
   Convenience form of cwal_json_parse() which reads its contents
   from the given opened/readable file handle.
*/
int cwal_json_parse_FILE( cwal_engine * e, FILE * src,
                          cwal_value ** tgt,
                          cwal_json_parse_info * pInfo );

/**
   Convenience form of cwal_json_parse() which reads its contents from
   the given file name.

   The file name "-" is interpreted as stdin.
*/
int cwal_json_parse_filename( cwal_engine * e, char const * src,
                              cwal_value ** tgt,
                              cwal_json_parse_info * pInfo );

/**
   Convenience form of cwal_json_parse() which reads its contents
   from (at most) the first len bytes of the given string.
*/
int cwal_json_parse_cstr( cwal_engine * e, char const * src,
                          cwal_size_t len, cwal_value ** tgt,
                          cwal_json_parse_info * pInfo );

/**
   Sets the current trace mask and returns the old mask. mask is
   interpreted as a bitmask of cwal_trace_flags_e values. If mask ==
   -1 then it returns the current mask without setting it, otherwise
   it sets the trace mask to the given value and returns the previous
   value.

   If !e or tracing is disabled at built-time, returns -1.
*/
int32_t cwal_engine_trace_flags( cwal_engine * e, int32_t mask );

/**
   Sets v's prototype value. Both v and prototype must be
   container types (those compatible with cwal_prop_set() and
   friends), and prototype may be NULL.

   If either v or prototype are not a container type, or v is a
   built-in value, CWAL_RC_TYPE is returned.

   If (v==prototype), CWAL_RC_MISUSE is returned.

   If v has the CWAL_CONTAINER_DISALLOW_PROTOTYPE_SET flag,
   CWAL_RC_DISALLOW_PROTOTYPE_SET is returned. (Added 20191210.)

   Returns CWAL_RC_CYCLES_DETECTED if v appears anywhere in the
   given prototype's prototype chain, with the special allowance
   of prototype already being v's prototype (see above).

   If none of the above-listed conditions apply and if prototype is
   already v's prototype then this is a harmless no-op.

   If v already has a different prototype, it is un-ref'd during
   replacement.

   On success, v adds a reference to the prototype object.

   On success, 0 is returned.
*/
int cwal_value_prototype_set( cwal_value * v, cwal_value * prototpe );

/**
   If v is a type capable of having a prototype, its prototype
   (possibly NULL) is returned, otherwise it is equivalent to
   cwal_value_prototype_base_get(e,cwal_value_type_id(v)) is
   returned.
       
   Reminder to self: the engine argument is only required so that
   this can integrate with cwal_prototype_base_get().
*/
cwal_value * cwal_value_prototype_get( cwal_engine * e, cwal_value const * v );

/**
   Maps the given client-specified prototype value to be the
   prototype for new values of type t. This adds a reference to
   proto and moves it to e's top-most scope so that it will live
   as long as e has scopes.

   Returns 0 on success, CWAL_RC_MISUSE if !e, and CWAL_RC_OOM
   if insertion of the prototype mapping could not allocate
   memory.

   All instances of the given type created after this is called
   will, if they are container types (meaning, by extension,
   capable of having a prototype) have proto assigned as their
   prototype as part of their construction process.

   Note that cwal does not assign prototypes by default - this is
   reserved solely for client-side use.

   Results are of course undefined if t is not a valid type ID (e.g.
   cast from an out-of-range integer).

   Potential uses:

   - Mapping common functions, e.g. toString() implementations,
   for types which cannot normally have prototypes (meaning
   non-container types).

   - A central place to plug in client-defined prototypes, such
   that new instances will inherit their prototypes (having had
   this feature would have saved th1ish a bit of code).

   @see cwal_prototype_base_get()
*/
int cwal_prototype_base_set( cwal_engine * e, cwal_type_id t, cwal_value * proto );

/**
   Returns a prototype value set via cwal_prototype_base_set(),
   or NULL if !e or no entry has been set by the client.
*/
cwal_value * cwal_prototype_base_get( cwal_engine * e, cwal_type_id t );

/**
   Returns true (non-0) if v==proto or v has proto in its
   prototype chain. Returns 0 if any argument is NULL.

   Reminder to self: the engine argument is only necessary so that
   this can integrate with cwal_prototype_base_get().
*/
bool cwal_value_derives_from( cwal_engine * e,
                              cwal_value const * v,
                              cwal_value const * proto );

    
/**
   Reparents v into one scope up from e's current scope, if possible
   and necessary in order to keep v alive (it is not moved if it
   already belongs in an older scope.  Returns CWAL_RC_MISUSE if !v or
   if v has no associated cwal_engine, 0 if v is already in a
   top-level scope. This is a no-op for built-in constant values
   (which do not participate in lifetime tracking).
*/
int cwal_value_upscope( cwal_value * v );

/**
   Returns a handle to v's originating cwal_engine, or NULL
   if !v.
*/
cwal_engine * cwal_value_engine( cwal_value const * v );

/**
   Returns the current owning scope of v, or NULL if !v.

   Note that this is always 0 for values for which
   cwal_value_is_builtin() returns true.
*/
cwal_scope * cwal_value_scope( cwal_value const * v );


/**
   Possibly reallocates self->list, changing its size. This
   function ensures that self->list has at least n entries. If n
   is 0 then the list is deallocated (but the self object is not),
   BUT THIS DOES NOT DO ANY TYPE-SPECIFIC CLEANUP of the items. If
   n is less than or equal to self->alloced then there are no side
   effects. If n is greater than self->alloced, self->list is
   reallocated and self->alloced is adjusted to be at least n (it
   might be bigger - this function may pre-allocate a larger
   value).

   Passing an n of 0 when self->alloced is 0 is a no-op.

   Newly-allocated slots will be initialized with NUL bytes.
   
   Returns the total number of items allocated for self->list.  On
   success, the value will be equal to or greater than n (in the
   special case of n==0, 0 is returned). Thus a return value smaller
   than n is an error. Note that if n is 0 or self is NULL then 0 is
   returned.

   The return value should be used like this:

   @code
   cwal_size_t const n = number of bytes to allocate;
   if( n > cwal_list_reserve( e, myList, n ) ) { ... error ... }
   // Or the other way around:
   if( cwal_list_reserve( e, myList, n ) < n ) { ... error ... }
   @endcode
*/
cwal_size_t cwal_list_reserve( cwal_engine * e, cwal_list * self, cwal_size_t n );

/**
   Appends a bitwise copy of cp to self->list, expanding the list as
   necessary and adjusting self->count.
       
   Ownership of cp is unchanged by this call. cp may not be NULL.

   Returns 0 on success, CWAL_RC_MISUSE if any argument is NULL,
   or CWAL_RC_OOM on allocation error.
*/
int cwal_list_append( cwal_engine * e, cwal_list * self, void * cp );

/** @typedef typedef int (*cwal_list_visitor_f)(void * p, void * visitorState )
   
    Generic visitor interface for cwal_list lists.  Used by
    cwal_list_visit(). p is the pointer held by that list entry
    and visitorState is the 4th argument passed to
    cwal_list_visit().

    Implementations must return 0 on success. Any other value
    causes looping to stop and that value to be returned, but
    interpration of the value is up to the caller (it might or
    might not be an error, depending on the context). Note that
    client code may use custom values, and is not restricted to
    CWAL_RC_xxx values.
*/
typedef int (*cwal_list_visitor_f)(void * obj, void * visitorState );

/**
   For each item in self->list, visitor(item,visitorState) is called.
   The item is owned by self. The visitor function MUST NOT free the
   item, but may manipulate its contents if application rules do not
   specify otherwise.

   If order is 0 or greater then the list is traversed from start to
   finish, else it is traverse from end to begin.

   Returns 0 on success, non-0 on error.

   If visitor() returns non-0 then looping stops and that code is
   returned.
*/
int cwal_list_visit( cwal_list * self, int order,
                     cwal_list_visitor_f visitor, void * visitorState );

/**
   Works similarly to the visit operation without the _p suffix except
   that the pointer the visitor function gets is a (**) pointing back
   to the entry within this list. That means that callers can assign
   the entry in the list to another value during the traversal process
   (e.g. set it to 0). If shiftIfNulled is true then if the callback
   sets the list's value to 0 then it is removed from the list and
   self->count is adjusted (self->alloced is not changed).
*/
int cwal_list_visit_p( cwal_list * self, int order, char shiftIfNulled,
                       cwal_list_visitor_f visitor, void * visitorState );


/**
   Parses command-line-style arguments into a cwal object tree.

   argc and argv are expected to be values from main() (or
   similar, possibly adjusted to remove argv[0]).

   It expects arguments to be in any of these forms, and any
   number of leading dashes are treated identically:

   -key : Treats key as a boolean with a true value.

   +key : Treats key as a boolean with a false value.

   -key=VAL : Treats VAL as a boolean, double, integer, string, null,
   or undefined (see cwal_value_from_arg()).

   -key= : Treats key as a cwal null (not literal NULL) value.

   +key=val : identical to -key=val. This "should" produce and
   error, but this routine is intentionally lax.

   All such properties are accumulated in the (*tgt).flags Object
   property. The flags object has no prototype, to avoid any
   unintentional property lookups.

   Arguments not starting with a dash or '+' are treated as
   "non-flags" and are accumulated in the (*tgt).nonFlags array
   property.
   
   Each key/value pair is inserted into the (*tgt).flags object.  If a
   given key appears more than once then only the final entry is
   actually stored.

   Any NULL entries in the argument list are skipped over.

   tgt must be either a pointer to NULL or a pointer to a
   client-provided container value. If (NULL==*tgt) then this function
   allocates a new object and on success it stores the new object in
   *tgt (it is owned by the caller). If (NULL!=*tgt) then it is
   assumed to be a properly allocated object. DO NOT pass a pointer to
   unitialized memory, as that will fool this function into thinking
   it is a valid object and Undefined Behaviour will ensue. If *tgt is
   not NULL (i.e. the caller passes in their own target) then this
   routine does not modify the refcount of the passed-in object. If
   this routine allocates *tgt then (on success) the new object is
   given to the caller with a refcount of 0.

   If *tgt is provided by the caller and is an array
   (cwal_value_get_array() returns non-NULL), then each argument in
   the provided argument list is appended as-is to that array.

   On success:

   - 0 is returned.

   - If (*tgt==NULL) then *tgt is assigned to a newly-allocated
   object, owned by the caller, with a refcount of 0. Note that even
   if no arguments are parsed, the object is still created.

   On error:

   - non-0 is returned: CWAL_RC_MISUSE if e or tgt are
   NULL. CWAL_RC_MISUSE if *tgt is not NULL but cwal_props_can(*tgt)
   returns false (i.e. if *tgt is not a property container type).
   CWAL_RC_RANGE if argc is negative.

   - If (*tgt==NULL) then it is not modified.

   - If (*tgt!=NULL) (i.e. the caller provides his own object) then
   it might contain partial results.

   @see cwal_value_from_arg()
*/
int cwal_parse_argv_flags( cwal_engine * e,
                           int argc, char const * const * argv,
                           cwal_value ** tgt );

/**
   A helper function intended for use in implementing utilities
   like cwal_parse_argv_flags(). This function tries to evaluate
   arg as follows:

   - If it looks like a number, return a numeric value. If the
   number-looking value is too large (would overflow during
   conversion), it is treated like a string instead. (Reminder
   to self: that seems to work properly for integers but there
   are likely ranges of doubles for which overflow is not
   properly noticed, in which case the conversion may truncate
   the resuling double-typed value.)

   - If it is "true" or "false", return the equivalent boolean value.

   - If it is NULL or "null", return the special null value.

   - If it is "undefined", return the special undefined value.

   - Else treat it like a string. If it starts and ends in matching
   quotes (single or double), those are removed from the resulting
   string.

   It is up to the caller to cwal_value_ref() the returned value.

   Returns NULL only on allocation error or if !e.

   Prior to 20181107, this function did not attempt to parse args
   which started with leading '+' or '-' as numbers, but it now
   does. If the input represents a massive number that is too large
   for cwal_int, the internal attempt to convert it to an integer will
   fail and this routine will fall back to treating it like a string.

   @see cwal_parse_argv_flags()
*/
cwal_value * cwal_value_from_arg(cwal_engine * e, char const *arg);

/**
   Creates a new weak reference for the given value. The return
   value can be passed to cwal_weak_ref_value() to find out if the
   value referenced by the cwal_weak_ref is still valid.

   Returns NULL if !v or on allocation error. If recycling
   is enabled for the CWAL_TYPE_WEAK_REF type then this will
   re-use recyclable memory if any is available.

   Results are strictly undefined if v is not valid at the time
   this is called (e.g. if it has already been destroyed and is a
   dangling pointer).

   The caller must eventually pass the returned instance to
   cwal_weak_ref_free() to clean it up. Note that cwal_weak_refs
   are not owned by scopes, like values are, so they will not be
   pulled out from under the client if a weak ref survives past
   the cwal_scope under which it is created.

   Minor achtung: weak refs are themselves reference-counted, and
   all weak refs to the same value (assuming it really _is_ the
   same value when all weak refs are created) will be the same
   weak ref instance. However, UNLIKE VALUES, they start life with
   a refcount of 1 instead of 0 (a currently-necessary side-effect
   of the sharing). That, however, is an implementation detail
   which clients must not rely on. i.e. the must pass each
   returned value from this function to cwal_weak_ref_free(), even
   though this function may return the same value multiple times.

   If v is one of the built-in values then this function might return
   a shared cwal_weak_ref instance, but this is an optimization and
   implementation detail, and clients should not rely on it.

   The above refcounting and sharing is mentioned here primarily
   in case someone happens to notice this function returning
   duplicate pointers and thinks its a bug. It's not a bug, it
   just means that v is one of the special built-in constants or a
   multiply-weak-ref'd value. For built-ins, the weak reference
   will never become invalidated because the built-in values are
   neither allocated nor freed (and thus valid for the life of the
   program).

   @see cwal_weak_ref_free()
   @see cwal_weak_ref_value()
   @see cwal_weak_ref_custom_new()
*/
cwal_weak_ref * cwal_weak_ref_new( cwal_value * v );

/**
   If r was created by cwal_weak_ref_new() and r's value is
   still alive then this function returns it, else it returns
   NULL. Will return NULL after the referenced value has been
   destroyed via the normal value lifetime processes.

   Returns NULL if !r.

   @see cwal_weak_ref_new()
   @see cwal_weak_ref_free()
*/
cwal_value * cwal_weak_ref_value( cwal_weak_ref * r );

/**
   Frees (or recycles) the memory associated with a weak
   reference created by cwal_weak_ref_new() or
   cwal_weak_ref_custom_new(). If the client fails to do so, the
   reference will effectively leak until the engine is cleaned
   up, at which point it will reap the memory of all dangling
   weak references (at which point it becomes illegal for the
   client to try to do so because both the cwal_engine and the
   weak reference are invalid!).

   cwal_engine_recycle_max() can be used to configure the size of
   the weak reference recycling pool by passing CWAL_TYPE_WEAK_REF
   as its second parameter.

   @see cwal_weak_ref_new()
*/
void cwal_weak_ref_free( cwal_engine * e, cwal_weak_ref * r );

/**
   Creates a weak reference which "monitors" p. A call to
   cwal_weak_ref_custom_invalidate(e,p) will "invalidate" any
   weak references pointing to, such that
   cwal_weak_ref_custom_check() and cwal_weak_ref_custom_ptr()
   for references to that memory will return NULL.

   Note that this function recycles cwal_weak_ref instances for
   any given value of p, meaning that this function may return
   the same instance multiple times when passed the same
   parameters. However, it reference counts them and each
   instance should still be treated as unique and passed to
   cwal_weak_ref_free() when the client is done with it.

   Clients must at some point call
   cwal_weak_ref_custom_invalidate() to remove any entries they
   "map" via weak references. Ideally they should do this in the
   moment before their native memory is being finalized or
   otherwise unassociated with script-space. If clients do not do
   so then weak references to that memory will (incorrectly)
   still think it is alive because cwal still holds a copy of
   that pointer.

   @see cwal_weak_ref_custom_invalidate()
   @see cwal_weak_ref_custom_check()
   @see cwal_weak_ref_custom_ptr()
*/
cwal_weak_ref * cwal_weak_ref_custom_new( cwal_engine * e, void * p );

/**
   "Invalidates" p, in that future calls to
   cwal_weak_ref_custom_check(e,p) or cwal_weak_ref_custom_ptr()
   will return NULL.

   Returns 0 (false) if it does not find p in e's weak ref
   mapping or non-0 (true) if it does (and thereby invalidates
   existing weak refs to it).

   @see cwal_weak_ref_custom_new()
*/
bool cwal_weak_ref_custom_invalidate( cwal_engine * e, void * p );

/**
   Searches e to see if p is being monitored by weak references
   created via cwal_weak_ref_custom_new(e,p). If one is found
   then then p is returned, else NULL is returned. Note that a
   call to cwal_weak_ref_custom_invalidate() "erases" monitored
   pointers, and if p has been passed to it then this function
   will return NULL. This is essentially an O(1) operation (a
   hashtable lookup).
*/
void * cwal_weak_ref_custom_check( cwal_engine * e, void * p );

/**
   If r was created by cwal_weak_ref_custom_new() and has not
   been invalidated then this function returns r's native memory
   pointer (of a type known only to whoever created r, if at
   all). Otherwise it returns NULL. This is faster than
   cwal_weak_ref_custom_check() (O(1) vs. a slower O(1)).
*/
void * cwal_weak_ref_custom_ptr( cwal_weak_ref * r );

/**
   Returns true (non-0) if p has been registered as
   weakly-referenced memory with e, else false (0). Note that p
   is intended to be a client-side native memory address or
   cwal_value pointer, and NOT one of the concrete higher-level types
   like cwal_object, nor a cwal_weak_ref instance.

   p "should" be a const pointer, but some internals disallow
   that (we don't do anything non-consty with it, though). In
   script bindings, however, const pointers are fairly rare
   because bound data are rarely const.
*/
bool cwal_is_weak_referenced( cwal_engine * e, void * p );


/**
   Tokenizes an input string on a given separator. Inputs are:

   - (inp) = is a pointer to the pointer to the start of the input.

   - (separator) = the separator character

   - (end) = a pointer to NULL. i.e. (*end == NULL)

   This function scans *inp for the given separator char or a NULL char.
   Successive separators at the start of *inp are skipped. The effect is
   that, when this function is called in a loop, all neighboring
   separators are ignored. e.g. the string "aa.bb...cc" will tokenize to
   the list (aa,bb,cc) if the separator is '.' and to (aa.,...cc) if the
   separator is 'b'.

   Returns 0 (false) if it finds no token, else non-0 (true).

   Output:

   - (*inp) will be set to the first character of the next token.

   - (*end) will point to the one-past-the-end point of the token.

   If (*inp == *end) then the end of the string has been reached
   without finding a token.

   Post-conditions:

   - (*end == *inp) if no token is found.

   - (*end > *inp) if a token is found.

   It is intolerant of NULL values for (inp, end), and will assert() in
   debug builds if passed NULL as either parameter.

   When looping, one must be sure to re-set the inp and end
   parameters on each iterator. For example:

   @code
   char const * head = "/a/b/c";
   char const * tail = NULL;
   while( cwal_strtok( &inp, '/', &tail ) ) {
   ...
   head = tail;
   tail = NULL;
   }
   @endcode

   If the loop calls 'continue', it must be careful to
   ensure that the parameters are re-set, to avoid an endless
   loop. This can be simplified with a goto:

   @code
   while( cwal_strtok( &head, '/', &tail ) ) {
   if( some condition ) {
   ... do something ...
   ... then fall through ... 
   }
   else {
   ... do something ...
   ... then fall through ... 
   }
   // setup next iteration:
   head = tail;
   tail = NULL;
   }
   @endcode

   or a for loop:

   @code
   for( ; cwal_strtok(&head, '/', &tail);
   head = tail, tail = NULL){
   ...
   }
   @endcode

   TODO: an implementation which takes a UTF8 char separator, or a
   UTF8 string separator (we have the code in th1ish and s2).
*/
bool cwal_strtok( char const ** inp, char separator,
                  char const ** end );

/**
   Returns the first Function in v's prototype chain, including v.
*/
cwal_function * cwal_value_function_part( cwal_engine * e,
                                          cwal_value * v );

/**
   Returns the first Object in v's prototype chain, including v.
*/
cwal_object * cwal_value_object_part( cwal_engine * e,
                                      cwal_value * v );

/**
   Returns the first Array in v's prototype chain, including v.
*/
cwal_array * cwal_value_array_part( cwal_engine * e,
                                    cwal_value * v );
/**
   Returns the first Hash in v's prototype chain, including v.
*/
cwal_hash * cwal_value_hash_part( cwal_engine * e,
                                  cwal_value * v );

/**
   Returns the first Buffer in v's prototype chain, including v.
*/
cwal_buffer * cwal_value_buffer_part( cwal_engine * e,
                                      cwal_value * v );
/**
   Returns the first Exception in v's prototype chain, including v.
*/
cwal_exception * cwal_value_exception_part( cwal_engine * e,
                                            cwal_value * v );

/**
   Returns the first String in v's prototype chain, including v.
*/
cwal_string * cwal_value_string_part( cwal_engine * e,
                                      cwal_value * v );

/**
   If the 3rd parameter is NULL, this returns the first Native
   in v's prototype chain, including v. If the 3rd param
   is not NULL then it returns the first Native in the
   chain with a matching typeID.

   Returns 0 if no match is found.
*/
cwal_native * cwal_value_native_part( cwal_engine * e,
                                      cwal_value * v,
                                      void const * typeID );

/**
   Returns v, or the first value from v's prototype chain which is
   capable of containing properties. Returns 0 if !v or if no
   prototype exists which can hold properties.
*/
cwal_value * cwal_value_container_part( cwal_engine * e, cwal_value * v );

/**
   Installs or removes a callback hook. If h is not NULL, its
   contents are bitwise copied into space owned by e, replacing
   any existing callback hook. If h is NULL, any installed
   callback hook is cleared (with no notification to the hooks!).

   @see cwal_callback_hook
*/
int cwal_callback_hook_set(cwal_engine * e, cwal_callback_hook const * h );


/**
   Dumps e's internalized strings table to e's output channel.  If
   showEntries is true it lists all entries. If includeStrings is not
   0 then strings of that length or less are also output (longer ones
   are not shown). If includeStrings is 0 then the strings are not
   output. Note that the strings are listed in an unspecified order
   (actually orded by (hash page number/hash code), ascending, but
   that's an implementation detail).
*/
void cwal_dump_interned_strings_table( cwal_engine * e,
                                       char showEntries,
                                       cwal_size_t includeStrings );
    
/**
   Dumps some allocation-related metrics to e's output channel.
   Intended only for optimization and debugging purposes.
*/
void cwal_dump_allocation_metrics( cwal_engine * e );


/**
   Marks v as being exempted (or not) from vacuum operations, but
   otherwise does not affect its lifetimes. Values marked as being
   exempted, and any values they contain/reference (which includes all
   array/tuple entries and property/hash table keys and values), will
   be treated as script-visible, named variables for purposes of
   cwal_engine_vacuum() (that is, a vacuum will not destroy them).

   If the 2nd argument is true, the value is marked as vacuum-proof,
   otherwise it is unmarked, making it _potentially_ (based on its
   exactly place in the universe) subject to subsequent vacuuming.

   Returns 0 on success or if v is a built-in value (they are
   inherently vacuum-proof), CWAL_RC_MISUSE if v is 0.

   The intent of this function is only to make internal Values which
   are not accessible via script code and which need to stay
   alive. Such values require a reference (see cwal_value_ref()) and
   to be vacuum-proofed via this function. As of this writing, in the
   whole cwal/th1ish/s2 constellation, only a small handful of values
   are marked as vacuum-proof: (A) cwal's internal list of prototypes
   (only the list, not the prototypes) and (B) a piece of th1sh's and
   s2's internals where it stashes its own non-script visible
   values. Any values reachable via a vacuum-proof container are safe
   from vacuuming, thanks to side-effects of cwal's lifetime
   management.

   Achtung: if v is ever to be made visible to script code, it most
   certainly should be set to NOT vacuum-proof, by passing 0 as this
   function's 2nd argument, or else it won't ever be able to be
   vacuumed up if it gets orphaned (with cycles) in a script. If it
   gets no cycles and all references are gone, it can still be reaped
   immediately or (depending on other conditions) swept up later.

   @see cwal_engine_vacuum()
*/
int cwal_value_make_vacuum_proof( cwal_value * v, char yes );

/**
   Returns true if v has explicitly been made vacuum-proof using
   cwal_value_make_vacuum_proof() OR if it is a built-in constant
   value, else false. A value which is not explicitly vacuum-proof may still
   be implicitly vacuum-proofed via a container which creates a path
   leading to the value.
*/
bool cwal_value_is_vacuum_proof( cwal_value const * v );

/**
   Adjusts (optional) metrics (only) which keep track of how much
   memory has been allocated by a client for use with cwal. This
   information is generally only useful for debugging and
   reporting purposes, and cannot be used to enforce any sort of
   memory caps.

   The second parameter is the amount of memory to report (for a
   positive value) or recall (a negative value). Clients who wish
   to use this should pass it a positive value when allocating
   memory and a negative value while cleaning up. A realloc may
   require first passing the negative value of the old size
   followed by the positive value of the new size.

   The point of this routine is to allow higher-level clients to
   help account for memory they allocate for use with their cwal
   bindings. e.g. s2 reports the memory allocated for Functions
   this way, so that they can be counted via the engine's metrics.

   Clients should not use this to report changes in memory to
   cwal_buffer instances, as those are handled at the library
   level. Its primary intended use is for tracking allocation
   totals for memory allocated on behalf of client-side types,
   e.g. the C part of a cwal_native value, which cwal knows is
   there (because it holds a (void*) to it), but does not have any
   information regarding its size or semantics.

   If amount is negative and its absolute value is larger than the
   currently declared allocation total, the total is reduced to 0,
   as opposed to underflowing.

   As of 20141214, cwal_malloc() and friends optionally track all
   memory they manage, such that they can report the exact amount of
   memory they've been requested to process. That is part of the
   cwal_memcap_config mechanism, and when it is enabled, any
   cwal_malloc()-allocated memory which clients report here is
   effectively counted double (once by the allocator and once by the
   client) by cwal_dump_allocation_metrics(). No harm done, though,
   other than double counting of that memory in metrics dumps.
*/
void cwal_engine_adjust_client_mem( cwal_engine * e, cwal_int_t amount );

/**
   Sets client-side flags on the container value v.

   A container is defined as: cwal_props_can() returns true for the
   value.

   If v is a container value, its flags are set to the given flags and
   the old flags value is returned.

   If !v or v is not a container then 0 is returned (which is also
   the default flags value.

   Notes about these flags:

   - They are reserved for cwal client use. Whether that means a
   scripting engine on top of cwal or a client above that is up to the
   layer between cwal and the higher-level client.

   - Their interpretation is of course client-dependent.

   - There are only 16 of them. We can't have more without increasing
   the sizeof() for container values.

   - These are independent of flags set via, e.g.
   cwal_prop_set_with_flags_v().

   @see cwal_container_client_flags_get()
   @see cwal_container_flags_get()
*/
cwal_flags16_t cwal_container_client_flags_set( cwal_value * v, cwal_flags16_t flags );

/**
   Gets any flags set using cwal_container_client_flags_set(), or 0 if !v or
   v is-not-a Container type. Note that 0 is also a legal return value
   for a container, and is the default if no flags have been
   explicitly set on the value.

   @see cwal_container_client_flags_set()
   @see cwal_container_flags_set()
*/
cwal_flags16_t cwal_container_client_flags_get( cwal_value const * v );

/**
   Sets a bitmask of values from the cwal_container_flags enum
   as the given value's flags.

   A container is defined as: cwal_props_can() returns true for the
   value.

   If v is a container value, its flags are set to the given flags and
   the old flags value is returned.

   If !v or v is not a container then 0 is returned (which is also
   the default flags value.

   @see cwal_container_flags_get()
   @see cwal_container_client_flags_set()
   @see cwal_container_client_flags_get()
*/
cwal_flags16_t cwal_container_flags_set( cwal_value * v, cwal_flags16_t flags );

/**
   Gets any flags set using cwal_container_flags_set(), or 0 if !v or
   v is-not-a Container type. Note that 0 is also a legal return value
   for a container, and is the default if no flags have been
   explicitly set on the value.

   @see cwal_container_flags_set()
   @see cwal_container_client_flags_set()
*/
cwal_flags16_t cwal_container_flags_get( cwal_value const * v );


/**
   Works like cwal_printfv(), but appends all output to a
   dynamically-allocated string, expanding the string as necessary to
   collect all formatted data. The returned null-terminated string is
   owned by the caller and it must be cleaned up using cwal_free(). If !fmt
   or if the expanded string evaluates to empty, null is returned, not
   a 0-byte string.
*/
char * cwal_printfv_cstr( cwal_engine * e, char const * fmt, va_list vargs );

/**
   Equivalent to cwal_printfv_cstr(), but takes elipsis arguments instead
   of a va_list.
*/
char * cwal_printf_cstr( cwal_engine * e, char const * fmt, ... );

/**
   Returns the value of the CWAL_VERSION_STRING build-time
   configuration macro (from static memory). If the length param is
   not NULL then the length, in bytes, of the string is written in
   (*length).
*/
char const * cwal_version_string(cwal_size_t * length);

/**
   Returns the value of the CWAL_CPPFLAGS build-time
   configuration macro (from static memory). If the length param is
   not NULL then the length, in bytes, of the string is written in
   (*length).
*/
char const * cwal_cppflags(cwal_size_t * length);

/**
   Returns the value of the CWAL_CFLAGS build-time configuration
   configuration macro (from static memory). If the length param is
   not NULL then the length, in bytes, of the string is written in
   (*length).
*/
char const * cwal_cflags(cwal_size_t * length);

/**
   Returns the value of the CWAL_CXXFLAGS build-time
   configuration macro (from static memory). If the length param is
   not NULL then the length, in bytes, of the string is written in
   (*length).
*/
char const * cwal_cxxflags(cwal_size_t * length);

/**
   If e is NULL, returns CWAL_RC_MISUSE, else returns 0 unless the
   cwal APIs have internally flagged e as being "dead". This state
   ONLY happens when conditions which are normally assert()ed in debug
   builds are noticed in non-debug builds. Once this flag has been
   set, then e is in a corrupt state and might have leaked memory to
   avoid touching memory it believes to be corrupted.

   In debug builds, this will ALWAYS return 0 (if e is valid) because
   the conditions which set this flag all assert() in debug builds,
   crashing the app outright.

   If this function returns non-0 for a valid argument, there is
   generically no recovery option other than letting e's memory
   leak and exiting the app, leaving cleanup to the OS. If
   e is passed to cwal_engine_destroy(), that destruction might
   try to step on some of the apparently corrupted memory, so
   the results are undefined.

   Corruption of the type which triggers such conditions are
   essentially always caused by unref'ing cwal_value pointers too many
   times (i.e. after cwal has stuck it in the recycler or freed it).

   TODO: this flag is not currently set for all assertions, but
   it basically needs to be.
*/
int cwal_is_dead(cwal_engine const * e);

/**
   Creates a new "tuple" value with the given length. Tuples are
   basically fixed-length arrays which cannot hold properties.

   Returns NULL if !e or on allocation error.

   The cwal_value_type_id() for tuples is CWAL_TYPE_TUPLE and their
   cwal_type_id_name() is "tuple".

   @see cwal_new_tuple_value()
   @see cwal_tuple_set()
   @see cwal_tuple_get()
   @see cwal_tuple_length()

*/
cwal_tuple * cwal_new_tuple(cwal_engine * e, uint16_t n);

/**
   Equivalent to passing the result of cwal_new_tuple() to
   cwal_tuple_value().

   @see cwal_new_tuple_value()
*/
cwal_value * cwal_new_tuple_value(cwal_engine * e, uint16_t n);

/**
   Returns the cwal_value part of tp, or NULL if !tp.

   @see cwal_new_tuple()
*/
cwal_value * cwal_tuple_value(cwal_tuple const *tp);

/**
   Returns the length of the given tuple (the number of slots
   it has for storing elements).

   @see cwal_new_tuple_value()
*/
uint16_t cwal_tuple_length(cwal_tuple const * tp);

/**
   Gets the value stored at the given index in the given
   tuple. Returns NULL if n is out of range (not less than
   cwal_tuple_length()) or if that slot has no value.

   @see cwal_new_tuple()
   @see cwal_tuple_set()
*/
cwal_value * cwal_tuple_get(cwal_tuple const * tp, uint16_t n);

/**
   Sets the value at the given index in the given
   tuple. Returns NULL if n is out of range (not less than
   cwal_tuple_length()) or if that slot has no value.

   This might (depending on refcounts) destroy any prior item held in
   that slot.

   Returns 0 on success, CWAL_RC_MISUSE if !p, CWAL_RC_RANGE if n is
   not less than cwal_tuple_length(tp).

   Note that, unlike cwal_array_set(), this function never has to
   allocate, so cannot fail if its arguments are valid.

   @see cwal_new_tuple()
   @see cwal_tuple_get()
   @see cwal_tuple_length()
*/
int cwal_tuple_set(cwal_tuple * tp, uint16_t n, cwal_value * v);

/**
   Works identically to cwal_array_visit().

   @see cwal_new_tuple_value()
*/
int cwal_tuple_visit( cwal_tuple * tp, cwal_value_visitor_f f, void * state );

/**
   If v was created using cwal_new_tuple_value() or cwal_new_tuple(),
   this function returns its cwal_tuple part, else it returns NULL.

   @see cwal_new_tuple_value()
   @see cwal_new_tuple()
*/
cwal_tuple * cwal_value_get_tuple( cwal_value * v );

/* tuple is not a property container, thus it cannot be a prototype, thus no:
   cwal_tuple * cwal_tuple_part( cwal_engine * e, cwal_value * v );
*/

/**
   A type for making build-time configuration data of the library
   easily available to clients.

   Use cwal_build_info() to get at this info.
*/
struct cwal_build_info_t {

  /* Config sizes... */
  cwal_size_t const size_t_bits;
  cwal_size_t const int_t_bits;
  cwal_size_t const maxStringLength;

  /* Strings... */
  char const * const versionString;
  char const * const cppFlags;
  char const * const cFlags;
  char const * const cxxFlags;

  /* Booleans... */
  char const isJsonParserEnabled;
  char const isDebug;

  /* sizeof()s... */
  struct {
    cwal_size_t builtinValues;
    cwal_size_t cwalValue;        
    cwal_size_t voidPointer;
  } sizeofs;
};
typedef struct cwal_build_info_t cwal_build_info_t;

/**
   Returns the library's shared/static cwal_build_info_t object.
*/
cwal_build_info_t const * cwal_build_info(void);

/**
   Returns the new value (an Object) on success, 0 on error. The only
   error cases are misuse (e is NULL or does not have an active scope)
   or allocation error.

   The new object is returned without any references - the caller
   effectively takes "ownership" (given what that means in this
   framework).

   @see cwal_callback_f_build_info()
*/
cwal_value * cwal_build_info_object(cwal_engine * e);

/**
   A cwal_callback_f() implementation which wraps
   cwal_build_info_object().

   On success, returns 0 and sets *rv to the build info object. On
   error CWAL_RC_OOM is returned.
*/
int cwal_callback_f_build_info(cwal_callback_args const * args, cwal_value ** rv);

/**
   Sets err's state to the given code/string combination, using
   cwal_buffer_printf() formatting.

   If fmt is 0 or !*fmt then any existing error message is reset.

   As a special case, if code==CWAL_RC_OOM, it behaves as if fmt is
   0 to avoid allocating any new memory.

   The e argument is required for its allocator - this function does
   not directly modify e's state, only err's (noting that err may well
   be embedded in e, so indirect modification of e is possible).

   If the 2nd argument is NULL, e's error state is used.

   On success it returns the 3nd argument (NOT 0!). On error it returns
   some other non-0 code.
*/
int cwal_error_setv( cwal_engine * e, cwal_error * err, int code, char const * fmt, va_list );

/**
   Elipses counterpart of cwal_error_setv().
*/
int cwal_error_set( cwal_engine * e, cwal_error * err, int code, char const * fmt, ... );

/**
   Copies src's error state over to dest, reusing dest's buffer memory
   if possible.

   If src is NULL then e's error state is used, otherwise if dest is NULL
   then e's error state is used. That is, ONE of the 2nd or 3rd
   arguments may be NULL, but not both.

   Returns 0 on success, CWAL_RC_MISUSE if src==dest, CWAL_RC_OOM on
   allocation error.
*/
int cwal_error_copy( cwal_engine * e, cwal_error const * src, cwal_error * dest );

/**
   Resets any any state in err, but keeps any memory in place for
   re-use.
*/
void cwal_error_reset( cwal_error * err );

/**
   Resets e's error state using cwal_error_reset().
*/
void cwal_engine_error_reset( cwal_engine * e );

/**
   Returns a pointer to e's error state object. The pointer is owned
   by e. When other cwal APIs refer to a cwal_engine's "error state,"
   they's referring to this object unless specified otherwise.

   Note that the core library does not actually use this object. It's
   intended as a convenience for downstream code, and was added to the
   API to support decoupling of certain downstream code.
*/
cwal_error * cwal_engine_errstate( cwal_engine * e );

#if 0
/* needed? */
/**
   Const-correct counterpart of cwal_engine_errstate().
*/
cwal_error const * cwal_engine_errstate_c( cwal_engine const * e );
#endif

/**
   Moves the error state from one s2_error object to another, intended
   as an allocation optimization when propagating error state up the
   API.

   This "uplifts" an error from the 'from' object to the 'to'
   object. After this returns 'to' will contain the prior error state
   of 'from' and 'from' will contain the old error message memory of
   'to'. 'from' will be re-set to the non-error state (its buffer
   memory is kept intact for later reuse, though).

   Results are undefined if either parameter is NULL or either is not
   properly initialized. i.e. neither may refer to uninitialized
   memory. Copying s2_error_empty at declaration-time is a simple way
   to ensure that instances are cleanly initialized.
*/
void cwal_error_move( cwal_error * from, cwal_error * to );

/**
   Frees all memory owned by err, but does not free err. The e
   argument is required for its underlying allocator. If err is NULL
   then e's error state is cleared.
*/
void cwal_error_clear( cwal_engine * e, cwal_error * err );

/**
   If err->code is not 0, *msg and *msgLen (if they are not NULL)
   are assigned to the message string and its length, respectively.
   It is legal for the returned *msg value to be NULL, which simply
   indicates that no error string was provided when the error state
   was set.

   Returns err->code.

   If it returns 0 (err has no error state) then it does not modify
   msg or msgLen.
*/
int cwal_error_get( cwal_error const * err, char const ** msg, cwal_size_t * msgLen );

/**
   Works like cwal_error_get(), using e's error state as the source.
*/
int cwal_engine_error_get( cwal_engine const * e, char const ** msg, cwal_size_t * msgLen );

/**
   Takes err's error string and uses it to create a new Exception
   value. If !err, e's error state is used. On success, it returns a
   new Exception with the err->code error code and a "message"
   property containing err's error string. If err->msg is empty, then
   a generic message is created based on err->code.

   If scriptName is not 0 and (*scriptName) then the exception
   gets a "script" string property with that value.

   If (line>0) then the exception gets line/column properties
   holding the line/col values.

   Returns 0 on error (OOM).

   This does not set e's exception state, but it does (on success)
   clear err's error state (so that we can give the string directly to
   cwal instead of copying it). err may or may not still own memory
   buffer memory after this call, so it must (eventually) be cleaned
   up using cwal_error_clear().
*/
cwal_value * cwal_error_exception( cwal_engine * se,
                                   cwal_error * err,
                                   char const * scriptName,
                                   int line, int col );

/**
   Converts err (or e's error state, if err is NULL) to an exception
   value using cwal_error_exception() (see that func for important
   details) then set's e's exception state to that exception.
    
   Like cwal_exception_set(), this function returns CWAL_RC_EXCEPTION
   on success or some other non-0 code if creation of the exception
   fails (generally speaking, probably CWAL_RC_OOM).

   If line<=0 then err->line and err->col are used in place of the given
   line/column parameters.

   If script is 0 and err->script is populated, that value is used
   instead.
*/
int cwal_error_throw( cwal_engine * se, cwal_error * err,
                      char const * script,
                      int line, int col );


/* LICENSE

   This software's source code, including accompanying documentation
   and demonstration applications, are licensed under the following
   conditions...

   Certain files are imported from external projects and have their
   own licensing terms. Namely, the JSON_parser.* files. See their
   files for their official licenses, but the summary is "do what you
   want [with them] but leave the license text and copyright in
   place."

   The author (Stephan G. Beal
   [https://wanderinghorse.net/home/stephan/]) explicitly disclaims
   copyright in all jurisdictions which recognize such a
   disclaimer. In such jurisdictions, this software is released into
   the Public Domain.

   In jurisdictions which do not recognize Public Domain property
   (e.g. Germany as of 2011), this software is Copyright (c)
   2011-2021 by Stephan G. Beal, and is released under the terms of
   the MIT License (see below).

   In jurisdictions which recognize Public Domain property, the user
   of this software may choose to accept it either as 1) Public
   Domain, 2) under the conditions of the MIT License (see below), or
   3) under the terms of dual Public Domain/MIT License conditions
   described here, as they choose.

   The MIT License is about as close to Public Domain as a license
   can get, and is described in clear, concise terms at:

   https://en.wikipedia.org/wiki/MIT_License

   The full text of the MIT License follows:

   --
   Copyright (c) 2011-2021 Stephan G. Beal
   (https://wanderinghorse.net/home/stephan/)

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use,
   copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following
   conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   --END OF MIT LICENSE--
*/

#if defined(__cplusplus)
} /*extern "C"*/
#endif

#endif /* WANDERINGHORSE_NET_CWAL_H_INCLUDED */
/* end of file include/wh/cwal/cwal.h */
/* start of file include/wh/cwal/cwal_printf.h */
#ifndef WANDERINGHORSE_NET_CWAL_APPENDF_H_INCLUDED
#define WANDERINGHORSE_NET_CWAL_APPENDF_H_INCLUDED 1
#ifdef _MSC_VER
    #define _CRT_NONSTDC_NO_DEPRECATE
#endif
#include <stdarg.h>
#include <stdio.h> /* FILE handle */
#ifdef __cplusplus
extern "C" {
#endif
/** @page cwal_printf_page_main cwal_printf printf-like API

   This API contains a printf-like implementation which supports
   aribtrary data destinations.

   Authors: many, probably. This code supposedly goes back to the
   early 1980's.

   Current maintainer: Stephan Beal (https://wanderinghorse.net/home/stephan)

   License: Public Domain.

   The primary functions of interest are cwal_printfv() and cwal_printf(), which works
   similarly to printf() except that they take a callback function which they
   use to send the generated output to arbitrary destinations. e.g. one can
   supply a callback to output formatted text to a UI widget or a C++ stream
   object.
*/

/**
   @typedef int (*cwal_printf_appender_f)( void * arg, char const * data, unsigned n )

   The cwal_printf_appender_f typedef is used to provide
   cwal_printfv() with a flexible output routine, so that it can be
   easily send its output to arbitrary targets.

   The policies which implementations need to follow are:

   - arg is an implementation-specific pointer (may be 0) which is
   passed to cwal_printfv(). cwal_printfv() doesn't know what this
   argument is but passes it to its cwal_printf_appender_f
   argumnt. Typically it will be an object or resource handle to which
   string data is pushed or output.

   - The 'data' parameter is the data to append. If it contains
   embedded nulls, this function will stop at the first one. Thus
   it is not binary-safe.

   - n is the number of bytes to read from data.

   - Returns 0 on success, some non-0 value on error. Ideally it
   should return a value from the cwal_rc_e enum or a value which is
   guaranteed not to collide with that enum.
*/
typedef int (*cwal_printf_appender_f)( void * arg,
                                     char const * data,
                                     unsigned int n );

/**
  This function works similarly to classical printf implementations,
  but instead of outputing somewhere specific, it uses a callback
  function to push its output somewhere. This allows it to be used for
  arbitrary external representations. It can be used, for example, to
  output to an external string, a UI widget, or file handle (it can
  also emulate printf by outputing to stdout this way).

 INPUTS:

 pfAppend : The is a cwal_printf_appender_f function which is
 responsible for accumulating the output. If pfAppend returns non-zero
 then processing stops immediately and that code is returned.

 pfAppendArg : is ignored by this function but passed as the first
 argument to pfAppend. pfAppend will presumably use it as a data
 store for accumulating its string.

 fmt : This is the format string, as in the usual printf().

 ap : This is a pointer to a list of arguments.  Same as in
 vprintf() and friends.

 OUTPUTS:

 Returns 0 on success. (Years of practice have shown that classical
 printf() return semantics don't make terribly much sense for this
 API.) On error, it is not generically possible to know how much, if any
 output was generated.

 CURRENT (documented) exceptions to conventional PRINTF format
 specifiers:

 %%n IS NOT SUPPORTED. Years of practice have shown that the classical
 return semantics of printf() are not useful for this particular API,
 and thus the semantics of this API were changed to something more
 useful (which does not support the notion of %%n).

 %%z (DISABLED IN THE CWAL BUILD!) works like %%s, but takes a
 non-const (char *) and free()s the string after appending it to the
 output. NEVER EVER EVER pass memory allocated via cwal_alloc() to
 %%z, as cwal_alloc() may, depending on various runtime options,
 manage memory in a way incompatible with free().

 %%h (HTML) works like %s but converts certain characters (like '<'
 and '&' to their HTML escaped equivalents.

 %%t (URL encode) works like %%s but converts certain characters into
 a representation suitable for use in an HTTP URL. (e.g. ' ' gets
 converted to %%20)

 %%T (URL decode) does the opposite of %t - it decodes URL-encoded
 strings.

 %%r requires an int and renders it in "ordinal form". That is, the
 number 1 converts to "1st" and 398 converts to "398th".

 %%q quotes a string as required for SQL. That is, '\'' characters get
 doubled.

 %%Q as %%q, but includes the outer '\'' characters and null pointers
 replaced by SQL NULL.

 (The %%q and %%Q specifiers are options inherited from this printf
 implementation's sqlite3 genes.)

 %%j JSON-escapes a string. %%!j does the same but adds outer double
 quotes. It treats NULL as an empty string.

 These extensions may be disabled by setting certain macros when
 compiling the implementation file (see that file for details).
*/
int cwal_printfv(cwal_printf_appender_f pfAppend,
                  void * pfAppendArg,
                  const char *fmt,
                  va_list ap);

/**
   The elipsis counterpart of cwal_printfv().
*/
int cwal_printf(cwal_printf_appender_f pfAppend,
                void * pfAppendArg,
                const char *fmt,
                ... );

/**
   Emulates fprintf() using cwal_printfv().
*/
int cwal_printf_FILE( FILE * fp, char const * fmt, ... );

/**
   va_list variant of cwal_printf_FILE().
*/
int cwal_printfv_FILE( FILE * fp, char const * fmt, va_list args );

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* WANDERINGHORSE_NET_CWAL_APPENDF_H_INCLUDED */
/* end of file include/wh/cwal/cwal_printf.h */
#endif/*!defined(WANDERINGHORSE_NET_CWAL_AMALGAMATION_H_INCLUDED)*/
/* end of file /home/stephan/fossil/cwal/cwal_amalgamation.h */
/* start of file s2_config.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */ 
#if !defined(WANDERINGHORSE_NET_CWAL_S2_CONFIG_H_INCLUDED)
#define WANDERINGHORSE_NET_CWAL_S2_CONFIG_H_INCLUDED 1
/**
   s2_config.h is intended to be auto-generated by a configure script
   in order to tell us whether certain system-level features are
   available. If the configuration process is unavailable for some reason,
   toggling all config options to off (with a value of 0) "should" work
   in a pinch.

   We have a fundamental conflict of interest here in terms of
   embedding these options here and accessing them from the loadable
   modules (all of which build against the amalgamation header).

   We disable all of the configurable options by default when
   building in amalgamation mode so as to avoid forcing 3rd-party
   prerequisites on users of s2_amalgamation.[ch].

   When building with the amalgamation build (where these entries
   cannot be easily/maintainably replaced), these defines can be
   passed on via the compiler flags or by \#include'ing a separate
   header beforehand which sets them. cwal_config.h (which gets
   injected as the top-most section in cwal_amalgamation.h) supports
   the flag HAVE_CONFIG_H, which tells it to #include "config.h", so
   overrides can be put there. Note, however, that THIS file gets
   added to the amalgamation *after* that one, so overriding
   cwal-level options cannot work from this file.
*/

#if !defined(CWAL_VERSION_STRING)
#  define CWAL_VERSION_STRING "cwal 601b514dd3f0cf7cde97274b9fa46b2826d54ccc 2021-07-24 09:36:48 configured 2021-07-24 10:37"
#endif
#if !defined(CWAL_CFLAGS)
#  define CWAL_CFLAGS "-Werror -Wall -Wextra -Wsign-compare -fPIC -std=c99 -g -Wpedantic"
#endif
#if !defined(CWAL_CPPFLAGS)
#  define CWAL_CPPFLAGS "-I/home/stephan/include"
#endif

#if !defined(S2_OS_WINDOWS) && !defined(S2_OS_UNIX)
#  if defined(_WIN32)
#    define S2_OS_WINDOWS
#  else
#    define S2_OS_UNIX
#  endif
#endif

#if !defined(S2_HAVE_USLEEP)
#  if defined(S2_AMALGAMATION_BUILD)
#    define S2_HAVE_USLEEP 0
#  else
#    define S2_HAVE_USLEEP 1 /* @ HAVE_USLEEP@ */
#  endif
#endif

#if !defined(S2_HAVE_CLOCK_GETTIME)
#  if defined(S2_AMALGAMATION_BUILD)
#    define S2_HAVE_CLOCK_GETTIME 0
#  else
#    define S2_HAVE_CLOCK_GETTIME 1 /* @ HAVE_CLOCK_GETTIME@ */
#  endif
#endif

#if !defined(S2_HAVE_REGCOMP)
#  if defined(S2_AMALGAMATION_BUILD)
#    define S2_HAVE_REGCOMP 0
#  else
#    define S2_HAVE_REGCOMP 1 /* @ HAVE_REGCOMP@ */
#  endif
#endif

#if !defined(S2_HAVE_STAT)
#  if defined(S2_AMALGAMATION_BUILD)
#    define S2_HAVE_STAT 0
#  else
#    define S2_HAVE_STAT 1 /* @ HAVE_STAT@ */
#  endif
#endif

#if !defined(S2_HAVE_MKDIR)
#  if defined(S2_AMALGAMATION_BUILD)
#    define S2_HAVE_MKDIR 0
#  else
#    define S2_HAVE_MKDIR 1 /* @ HAVE_MKDIR@ */
#  endif
#endif

#if !defined(S2_HAVE_LSTAT)
#  if defined(S2_AMALGAMATION_BUILD)
#    define S2_HAVE_LSTAT 0
#  else
#    define S2_HAVE_LSTAT 1 /* @ HAVE_LSTAT@ */
#  endif
#endif

#if !defined(S2_HAVE_CHDIR)
#  if defined(S2_AMALGAMATION_BUILD)
#    define S2_HAVE_CHDIR 0
#  else
#    define S2_HAVE_CHDIR 1 /* @ HAVE_CHDIR@ */
#  endif
#endif

#if !defined(S2_HAVE_GETCWD)
#  if defined(S2_AMALGAMATION_BUILD)
#    define S2_HAVE_GETCWD 0
#  else
#    define S2_HAVE_GETCWD 1 /* @ HAVE_GETCWD@ */
#  endif
#endif

#if !defined(S2_HAVE_REALPATH)
#  if defined(S2_AMALGAMATION_BUILD)
#    define S2_HAVE_REALPATH 0
#  else
#    define S2_HAVE_REALPATH 1 /* @ HAVE_REALPATH@ */
#  endif
#endif

#if !defined(S2_HAVE_DLOPEN)
#  if defined(S2_AMALGAMATION_BUILD)
#    define S2_HAVE_DLOPEN 0
#  else
#    define S2_HAVE_DLOPEN 1 /* @ HAVE_DLOPEN@ */
#  endif
#endif

/* Ensure that only 1 of S2_HAVE_DLOPEN and S2_HAVE_LTDLOPEN are set,
   so that the build doesn't get confused about which to use. The
   makefile(s) must also use this same selection process.
*/
#if S2_HAVE_DLOPEN
#  undef S2_HAVE_LTDLOPEN
#  define S2_HAVE_LTDLOPEN 0
#elif !defined(S2_HAVE_LTDLOPEN)
#  if defined(S2_AMALGAMATION_BUILD)
#    define S2_HAVE_LTDLOPEN 0
#  else
#    define S2_HAVE_LTDLOPEN 0 /* @ HAVE_LTDL@ */
#  endif
#endif

#if !defined(S2_INTERNAL_MINIZ)
#  define S2_INTERNAL_MINIZ 0
#endif

#if defined(S2_OS_UNIX)
/************************************************************************
Massage various defines to try to import specific features which our
local man pages claim we get via such massaging...
************************************************************************/
#  if !defined(_XOPEN_SOURCE)
  /** Linux: _XOPEN_SOURCE
      >=700 for usleep()
      >=500 for lstat(), chdir(), realpath()
  */
#    define _XOPEN_SOURCE 700
#  endif
#  ifndef _XOPEN_SOURCE_EXTENDED
  /* Linux:
     lstat()
  */
#    define _XOPEN_SOURCE_EXTENDED
#  endif
#  ifndef _BSD_SOURCE
  /* Linux: _BSD_SOURCE:
     chdir() (glibc <= 2.19)
     realpath() (glibc <= 2.19)
  */
#    define _BSD_SOURCE
#  endif
#  ifndef _DEFAULT_SOURCE
  /* Linux: _DEFAULT_SOURCE:
     realpath() (glibc >= 2.19)
     >= 200112L for lstat() (glibc 2.20+)
  */
#    define _DEFAULT_SOURCE
#  endif
#  if !defined(_POSIX_C_SOURCE)
  /* Linux: _POSIX_C_SOURCE:
     >= 200112L for lstat() (glibc 2.10+)
     >= 200809L for chdir()
  */
#    define _POSIX_C_SOURCE 200809L /*200112L*/ /*199309L*/
#  endif
#endif /* S2_OS_UNIX */


#if !defined(S2_HAVE_SIGACTION)
#  if defined(S2_AMALGAMATION_BUILD)
#    if defined(S2_OS_UNIX)
#      define S2_HAVE_SIGACTION 1
#    else
#      define S2_HAVE_SIGACTION 0
#    endif
#  else
#    define S2_HAVE_SIGACTION 1 /* @ HAVE_SIGACTION@ */
#  endif
#endif


#if defined(S2_OS_WINDOWS)
#  define S2_DIRECTORY_SEPARATOR "\\"
#else
#  define S2_DIRECTORY_SEPARATOR "/"
#endif

#endif /* WANDERINGHORSE_NET_CWAL_S2_CONFIG_H_INCLUDED */
/* end of file s2_config.h */
/* start of file t10n.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#ifndef NET_WANDERINGHORSE_CWAL_S2_T10N_H_INCLUDED_
#define NET_WANDERINGHORSE_CWAL_S2_T10N_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif
typedef struct s2_ptoker s2_ptoker;
typedef struct s2_ptoken s2_ptoken;

/**
   Represents a token ID in the s2_ptoken/s2_cptoken APIs. Values
   above 0 always represent valid IDs and most contexts treat 0 as EOF
   (often a "virtual" EOF for a block construct). It's tempting to use
   a 16-bit type for this, but the s2 amalgamated unit tests (as of
   20200106) use up about half of that range (2/3rds if we retain
   "junk" tokens), so it's easy to conceive of overflowing that.

   It's also somewhat tempting to use 18 bits (262144) for the ID and
   the remaining 14 (16348) for the token type.
*/
typedef uint32_t s2_token_id;

/**
   A sentinel s2_token_id meaning "no ID." This "really should" be
   an extern const s2_token_id, but we then cannot use it in struct
   initializers :/. Its value MUST be 0.
*/
#define s2_token_id_none (0)

/**
   Numeric type used for counting script line and column numbers.
   Note that we aim for a 16-bit type to shave a few bytes from
   oft-used token types. As of this writing (20200105), the single
   largest s2 script (its amalgamated unit tests) is right at 5400 lines
   (size=160kb), so any dreams of scripts with more than 64k lines
   would seem to be... ah... somewhat ambitious.
*/
typedef uint16_t s2_linecol_t;

/**
   s2 token type and operator IDs.

   Values >=0 and <=127 can be translated literally to their
   equivalent char value. Values over 127 are symbolic, not
   necessarily mapping to a single byte nor a Unicode code point with
   the same value. (There are very likely numerous collisions with
   Unicode code points in this enum.)

   @see s2_ttype_cstr()
*/
enum s2_token_types {
/**
   Used as the token type by s2_ptoker_next_token() when a
   tokenization-level error is encountered.
*/
S2_T_TokErr = -2,

/**
   The generic EOF marker. Used by s2_ptoker_next_token() when the
   end of the tokenizer's input range is reached. Note that this
   token is also used for "virtual" EOF and does NOT necessarily map
   to a NUL byte in the input. e.g. when sub-parsing part of a
   larger expression, the subexpression will get a subset of the
   parent range to parse, and its virtual EOF will be part of its
   parent parser's input range.
*/
S2_T_EOF = -1,

/**
   S2_T_INVALID is guaranteed by the API to be the entry in this
   enum with the value 0, whereas the concrete values for other
   non-ASCII-range tokens is unspecified except that they are
   guaranteed to be non-0.
*/
S2_T_INVALID = 0,

S2_T_Tab = 9,
S2_T_NL = 10,
S2_T_VTab = 11,
S2_T_FF = 12,
S2_T_CR = 13,
S2_T_At = 64 /* \@ */,
/**
   Generic EOL token, for \\r, \\n, and \\r\\n.

   Whether or not newlines end an expression is (or should be)
   context-dependent, and may depend on what token(s) lie(s)
   before it in the parsing process.
*/
S2_T_EOL = 213,
/** ASCII 32d, but runs of spaces are translated to
    S2_T_Blank. */
S2_T_Space = 32 /* ' ' */,
/** Generic token for runs of s2_is_blank() characters. */
S2_T_Blank = 132,
S2_T_Whitespace = 232,
S2_T_UTFBOM = 332 /* UTF byte-order marker (0xEF 0xBB 0xBF) */,

S2_T_OpNot = 33 /* ! */,

S2_T_OpHash = 35 /* # */,
S2_T_Shebang = 135 /* #!... */,

S2_T_OpModulo = 37 /* % */,
S2_T_OpModuloAssign = 237 /* %= */,
S2_T_OpModuloAssign3 = 337 /* X.Y %= Z*/,

S2_T_OpAndBitwise = 38 /* & */,
S2_T_OpAnd = 238 /* && */,
S2_T_OpAndAssign = 338 /* &= */,
S2_T_OpAndAssign3 = 438 /* X.Y &= Z */,

S2_T_ParenOpen = 40 /* ( */,
S2_T_ParenGroup = 140 /* (...) */,
S2_T_ParenClose = 41 /* ) */,

S2_T_OpMultiply = 42 /* * */,
S2_T_OpMultiplyAssign = 242 /* *= */,
S2_T_OpMultiplyAssign3 = 342 /* X.Y*=Z */,

S2_T_OpPlus = 43 /* + */,
S2_T_OpPlusUnary = 243 /* + */,
S2_T_OpPlusAssign = 343 /* += */,
S2_T_OpPlusAssign3 = 443 /* X.Y+=Z */,
S2_T_OpIncr = 543 /* ++ */,
S2_T_OpIncrPre = 643 /* ++ */,
S2_T_OpIncrPost = 843 /* ++ */,

S2_T_Comma = 44 /* , */,
S2_T_RHSEval = 144 /* internal-use-only pseudo-operator */,

S2_T_OpMinus = 45 /* - */,
S2_T_OpMinusUnary = 245 /* - */,
S2_T_OpMinusAssign = 345 /* -= */,
S2_T_OpMinusAssign3 = 445 /* X.Y-=y */,
S2_T_OpDecr = 545  /* -- */,
S2_T_OpDecrPre = 645  /* -- */,
S2_T_OpDecrPost = 745  /* -- */,

S2_T_OpDot = 46 /* . */,
S2_T_OpArrow = 146 /* -> */,
S2_T_OpArrow2 = 246 /* => */,
S2_T_OpDotDot = 346 /* .. */,
S2_T_OpDotLength = 446 /* .# */,

S2_T_OpDivide = 47 /* X/Y */,
S2_T_OpDivideAssign = 147 /* X/=Y */,
S2_T_OpDivideAssign3 = 247 /* X.Y/=Z */,

S2_T_Colon = 58 /* : */,
S2_T_Colon2 = 258 /* :: */,
S2_T_OpColon2 = S2_T_Colon2,
S2_T_OpColonEqual = 358 /* := */,

S2_T_Semicolon = 59 /* ; */,
/** Generic end-of-expression token. */
S2_T_EOX = 159,

S2_T_CmpLT = 60 /* < */,
S2_T_CmpLE = 260 /* <= */,
S2_T_OpShiftLeft = 360 /* << */,
S2_T_OpShiftLeftAssign = 460 /* <<= */,
S2_T_OpShiftLeftAssign3 = 560 /* X.Y<<=Z */,
S2_T_HeredocStart = 660 /* <<< */,
S2_T_Heredoc = 670 /* A "fully slurped" heredoc. Prior to 2020-08-27,
                      heredocs here a special cse of S2_T_SquigglyBlock
                      for historical reasons. */,

S2_T_OpAssign = 61 /* = */,
S2_T_OpAssign3 = 161 /* = */,
S2_T_CmpEq = 261 /* == */,
S2_T_CmpNotEq = 361 /* != */,
S2_T_CmpEqStrict = 461 /* === */,
S2_T_CmpNotEqStrict = 561 /* !== */,
S2_T_OpInherits = 661 /* inherits */,
S2_T_OpNotInherits = 761 /* !inherits */,
S2_T_OpContains = 861 /* =~ */,
S2_T_OpNotContains = 961 /* !~ */,
S2_T_OpAssignConst3 = 1061 /* X.Y:=Z */,

S2_T_CmpGT = 62 /* > */,
S2_T_CmpGE = 262 /* >= */,
S2_T_OpShiftRight = 362 /* >> */,
S2_T_OpShiftRightAssign = 462 /* >>= */,
S2_T_OpShiftRightAssign3 = 562 /* X.Y>>=Z */,

S2_T_Question = 63 /* ? */,
S2_T_QDot = 163 /* ?. reserved for potential future use */,

S2_T_BraceOpen = 91 /* [ */,
S2_T_BraceGroup = 191 /* [...] */,
S2_T_Backslash = 92 /* \\ */,
S2_T_BraceClose = 93 /* ] */,

S2_T_OpXOr = 94 /* ^ */,
S2_T_OpXOrAssign = 294 /* ^= */,
S2_T_OpXOrAssign3 = 394 /* X.Y^=Z */,

S2_T_SquigglyOpen = 123 /* { */,
S2_T_SquigglyBlock = 223 /* {...} */,

S2_T_OpOrBitwise = 124 /* | */,
S2_T_OpOr = 224 /* || */,
S2_T_OpOr3 = 324 /* ||| */,
S2_T_OpElvis = 424 /* ?: */,
S2_T_OpOrAssign = 524 /* |= */,
S2_T_OpOrAssign3 = 624 /* X.Y|=Z */,
S2_T_SquigglyClose = 125 /* } */,
S2_T_OpNegateBitwise = 126 /* ~ */,


S2_T_Literal__ = 1000,
S2_T_LiteralInt,
S2_T_LiteralIntDec,
S2_T_LiteralIntHex,
S2_T_LiteralIntOct,
S2_T_LiteralIntBin,
S2_T_LiteralDouble,
S2_T_LiteralStringDQ,
S2_T_LiteralStringSQ,
S2_T_LiteralString /* for "untranslated" strings */,
S2_T_PropertyKey /* special case of LiteralString */,
S2_T_Identifier,


S2_T_ValueTypes__ = 2000,
S2_T_Value,
S2_T_Undefined,
S2_T_Null,
S2_T_False,
S2_T_True,
S2_T_Object,
S2_T_Array,
S2_T_Function,

S2_T_Keyword__ = 3000,
S2_T_KeywordAffirm,
S2_T_KeywordAssert,
S2_T_KeywordBREAKPOINT,
S2_T_KeywordBreak,
S2_T_KeywordCOLUMN,
S2_T_KeywordCatch,
S2_T_KeywordClass,
S2_T_KeywordConst,
S2_T_KeywordContinue,
S2_T_KeywordDefine,
S2_T_KeywordDefined,
S2_T_KeywordDelete,
S2_T_KeywordDo,
S2_T_KeywordEcho,
S2_T_KeywordEnum,
S2_T_KeywordEval,
S2_T_KeywordException,
S2_T_KeywordExit,
S2_T_KeywordFILE,
S2_T_KeywordFILEDIR,
S2_T_KeywordFalse,
S2_T_KeywordFatal,
S2_T_KeywordFor,
S2_T_KeywordForEach,
S2_T_KeywordFunction,
S2_T_KeywordIf,
S2_T_KeywordImport,
S2_T_KeywordInclude,
S2_T_KeywordInterface,
S2_T_KeywordIs,
S2_T_KeywordIsA,
S2_T_KeywordLINE,
S2_T_KeywordNameof,
S2_T_KeywordNew,
S2_T_KeywordNull,
S2_T_KeywordPragma,
S2_T_KeywordPrivate,
S2_T_KeywordProc,
S2_T_KeywordProtected,
S2_T_KeywordPublic,
S2_T_KeywordRefcount,
S2_T_KeywordReturn,
S2_T_KeywordS2Out,
S2_T_KeywordSRCPOS,
S2_T_KeywordScope,
S2_T_KeywordStatic,
S2_T_KeywordThrow,
S2_T_KeywordTrue,
S2_T_KeywordTry,
S2_T_KeywordTypeinfo,
S2_T_KeywordTypename,
S2_T_KeywordUndefined,
S2_T_KeywordUnset,
S2_T_KeywordUKWD,
S2_T_KeywordUsing /* using() function-like keyword, as opposed to
                     function() using(...) {} */,
S2_T_KeywordVar,
S2_T_KeywordWhile,

S2_T_Comment__ = 4000,
S2_T_CommentC,
S2_T_CommentCpp,

S2_T_Mark__ = 5000,
S2_T_MarkVariadicStart,

S2_T_Misc__ = 6000,
/**
   A pseudo-token used internally to translate empty [] blocks to a
   PHP-style array-append operation.

   The parser current only allows this op in the context of an assignment
*/
S2_T_ArrayAppend,
S2_T_Foo,

S2_T_comma_kludge_
};


#if 0
typedef struct s2_byte_range s2_byte_range;
/**
   Holds a pair of pointers indicating a range
   to an abstract string data source.
*/
struct s2_byte_range {
  /**
     The starting position of source.
  */
  char const * begin;
  /**
     One-past-the-end position.
  */
  char const * end;
};
#define s2_byte_range_empty_m {0,0}
extern const s2_byte_range s2_byte_range_empty;
#endif

/**
   A "parser token" - tokens used by the s2 tokenization and
   evaluation process.
*/
struct s2_ptoken{
  /**
     If set to non-0, this token is proxying an s2_cptoken with this
     ID. This is part of the experimental framework for adding
     optional support for "compiled" tokens to s2's eval engine.
  */
  s2_token_id id;

  /**
     A s2_token_types value.
  */
  int16_t ttype;

  /**
     1-based line number relative to the s2_ptoker which
     sets this via s2_ptoker_next_token().

     It turns out that we can't reliably count this unless (slightly
     over-simplified) the tokenizer moves only forward. Once
     downstream code starts manipulating s2_ptoken::begin and
     s2_ptoken::end, the counting gets messed up (and we have lots
     of cases which do that).
  */
  s2_linecol_t line;

  /**
     0-based column number relative to the s2_ptoker which
     sets this via s2_ptoker_next_token().
  */
  s2_linecol_t column;
  
  /**
     The starting point of the token, relative to its containing
     script. Invalid tokens have a NULL begin value.

     202008: direct access to this is being phased out in favor of an
     ongoing abstraction. Use s2_ptoker_begin() to access it.
  */
  char const * _begin;

  /**
     The one-after-the-end point for the token. When tokenizing
     iteratively, each next token starts at the end position of the
     previous token.

     202008: direct access to this is being phased out in favor of an
     ongoing abstraction. Use s2_ptoker_end() to access it.
  */
  char const * _end;

  /**
     Some token types "trim" their bytes to some subset of [begin,
     end). For such token types, the range [adjBegin, adjEnd) should
     be used for fetching their "inner" bytes, while [begin, end)
     will hold the full token bytes.

     Currently the types for which this is done include:

     S2_T_SquigglyBlock, S2_T_Heredoc, S2_T_BraceGroup,
     S2_T_ParenGroup.

     202008: direct access to this is being phased out in favor of an
     ongoing abstraction. Use s2_ptoker_adjbegin() to access it.
  */
  char const * _adjBegin;

  /**
     The one-after-the-end counterpart of adjBegin.

     202008: direct access to this is being phased out in favor of an
     ongoing abstraction. Use s2_ptoker_adjend() to access it.
  */
  char const * _adjEnd;
};

/**
   Empty-initialized s2_ptoken structure, intended for
   const-copy initialization.
*/
#define s2_ptoken_empty_m {0,S2_T_INVALID,0,0,0,0,0,0,}

/**
   Empty-initialized s2_ptoken structure, intended for
   copy initialization.
*/
extern const s2_ptoken s2_ptoken_empty;

/**
   An internal implementation detail to speed up line-counting
   (something s2 has to do quite often).
*/
struct s2_ptoker_lccache_entry {
  char const * pos;
  int line;
  int col;
};
typedef struct s2_ptoker_lccache_entry s2_ptoker_lccache_entry;
/**
   Empty-initialized s2_ptoker_lccache_entry structure, intended for
   const-copy initialization.
*/
#define s2_ptoker_lccache_entry_empty_m {0,0,0}
/**
   An internal implementation detail to speed up line-counting
   (something s2 has to do inordinately often).
*/
struct s2_ptoker_lccache {
  /** Current position in this->lines. */
  volatile int cursor;
  /** "Size" of each cache slot, based on the size of the
      tokenizer's input range. */
  int slotSize;
  /** Cache of line-counting position results. */
  volatile s2_ptoker_lccache_entry lines[
     10
     /* reminders to self: 10 or 20 actually, in the 20191228
        amalgamated s2 unit tests, perform slightly better than 60
        does. Multiple tests show 10 to be an all-around good
        value. */
  ];
};
typedef struct s2_ptoker_lccache s2_ptoker_lccache;
/**
   Empty-initialized s2_ptoker_lccache structure, intended for
   const-copy initialization.
*/
#define s2_ptoker_lccache_empty_m \
  {0,0,{                             \
    s2_ptoker_lccache_entry_empty_m, s2_ptoker_lccache_entry_empty_m,  \
    s2_ptoker_lccache_entry_empty_m, s2_ptoker_lccache_entry_empty_m,  \
    s2_ptoker_lccache_entry_empty_m, s2_ptoker_lccache_entry_empty_m,  \
    s2_ptoker_lccache_entry_empty_m, s2_ptoker_lccache_entry_empty_m,  \
    s2_ptoker_lccache_entry_empty_m, s2_ptoker_lccache_entry_empty_m  \
 }}

/**
   The s2_ptoker class is a simple basis for a tokenizer, largely
   syntax- and language-independent. Its origins go back many years
   and several projects.

   This tokenizer requires that all input be available in advance of
   tokenization and remain valid for its own lifetime.

   @see s2_ptoker_init()
   @see s2_ptoker_next_token()
   @see s2_ptoker_lookahead()
   @see s2_ptoker_putback()
   @see s2_ptoker_next_token_set()
*/
struct s2_ptoker {
  /**
     Used for memory management in "v2" operations. Will be NULL for
     "historical-style" instances.

     We might want to change this to an s2_engine, but then we'd have
     a circular dependency, and those make me lose sleep.
  */
  cwal_engine * e;

  /**
     Starting position of input.

     The full input range is [begin, end).
  */
  char const * begin;

  /**
     One-past-the-end position of the input (i.e. the position
     where the NUL byte normally is).
  */
  char const * end;

  /**
     Error string (static memory) from tokenization
     errors. Set by s2_ptoker_next_token().
  */
  char const * errMsg;

  /**
     NUL-terminated string used for error reporting. May be a file
     name or a descriptive name like "eval script". The bytes are
     owned by "someone else" - not this object - and must outlive this
     object.
  */
  char const * name;

  /**
     Used for calculating line/col info for sub-parsing errors.
  */
  s2_ptoker const * parent;

  /**
     For ongoing token compilation experimentation. If this->parent is
     non-NULL then this pointer is assumed to belong to the top-most
     parent in the chain, else it is assumed to be owned by this
     instance.
  */
  void * compiled;

  /**
     Used for capturing line/column offset info for "distant child"
     tokenizers, which "know" they derive from another but have no
     access to it (it may be long gone).
  */
  s2_linecol_t lineOffset;

  /**
     Column counterpart of lineOffset.
  */
  s2_linecol_t colOffset;

  /**
     1-based current tokenization line number.

     This is maintained by s2_ptoker_next_token(), updated
     as it crosses newline boundaries.

     This can only be tracked properly as long as
     s2_ptoker_next_token() is called linearly. Once clients (e.g. s2
     internals) starts jumping around the input, this counting breaks.
  */
  s2_linecol_t currentLine;
  /**
     0-based current tokenization column number.

     This is maintained by s2_ptoker_next_token(). See
     notes in this->currentLine.
  */
  s2_linecol_t currentCol;

  /**
     Flags which may change how this object tokenizes.
  */
  cwal_flags32_t flags;
  
  /**
     The current token. Its state is set up thusly:

     Initially, token.begin must be this->begin and token.end
     must be 0. That state is used by s2_ptoker_next_token() to
     recognize the initial token state and DTRT.

     During tokenization, this object's state is updated to reflect
     the range from [this->begin, this->end) matching a token (or
     an error position, in the case of a tokenization error).
  */
  s2_ptoken token;

  /**
     The put-back token. s2_ptoker_next_token() copies this->token to
     this object before attempting any tokenization.
     s2_ptoker_putback() copies _pbToken over this->token and clears
     _pbToken.

     Do not manipulate this directly. use s2_ptoker_putback(),
     s2_ptoker_putback_get(), and (if really needed)
     s2_ptoker_putback_set().
  */
  s2_ptoken _pbToken;

  /**
     An experiment in cutting down on tokenization. Token lookahead
     ops, and some client code, set this after they have done a
     lookahead, conditionally setting this to that looked-ahead
     token. s2_ptoker_next_token() will, if this is set, use this
     token and skip the tokenization phase. This member is cleared by
     s2_ptoker_token_set() and s2_ptoker_next_token().

     Do not manipulate this directly: use s2_ptoker_next_token_set()
     to modify it.
  */
  s2_ptoken _nextToken;

  /**
     Used for marking an error position, which is part of the line/col
     counting mechanism used for error reporting.

     Do not manipulate this directly. Use s2_ptoker_errtoken_get(),
     s2_ptoker_errtoken_set(), and s2_ptoker_errtoken_has().
  */
  s2_ptoken _errToken;

  /**
     These tokens are used to capture a range of tokens solely for
     their string content. _Some_ APIs set this to a range encompasing
     all input which they consume. e.g. it can be used to record the
     whole result of multiple s2_ptoker_next_token() calls by setting
     capture.begin to the start of the first token and capture.end the
     end of the last token captured. The s2_t10n-internal APIs do not
     manipulate this member.

     We model capturing as a token range, rather than a simple byte
     range, with the hopes that this approach will work with the
     eventual addition of compiled tokens.
  */
  struct {
    /** First token in the capture range. */
    s2_ptoken begin;
    /**
       The one-after-the-end token in the capture range. Note that
       this means that the string range of the capture is
       [this->begin.begin, this->end.begin). For completely empty
       ranges this token will be the same as this->begin.
    */
    s2_ptoken end;
  } capture;

  
  /**
     Internal implementation detail for the line-counting results
     cache. Full disclosure: in order to avoid having to modify the
     signatures of a metric boatload of functions to make their
     (s2_ptoker const *) parameters non-const, this member may be
     modified in/via one routine where this s2_ptoker instance is
     otherwise const. Const-correct behaviour would require that we
     make a whole family of downstream functions non-const just to
     account for this internal optimization, which i'm not willing to
     do (as much as i love my const). This is where we could use the
     "mutable" keyword in C++, but, alas, this is C89.
  */
  s2_ptoker_lccache _lcCache;
};
/** Empty-initialized s2_ptoker object. */
#define s2_ptoker_empty_m {                     \
    0/*e*/, 0/*begin*/,0/*end*/,                 \
    0/*errMsg*/,                              \
    0/*name*/,                                \
    0/*parent*/,                              \
    0/*compiled*/,                              \
    0/*lineOffset*/,0/*colOffset*/,               \
    1/*currentLine*/,0/*currentCol*/,0/*flags*/,  \
    s2_ptoken_empty_m/*token*/,               \
    s2_ptoken_empty_m/*_pbToken*/,             \
    s2_ptoken_empty_m/*_nextToken*/,             \
    s2_ptoken_empty_m/*_errToken*/,              \
    {s2_ptoken_empty_m,s2_ptoken_empty_m}/*capture*/, \
    s2_ptoker_lccache_empty_m/*_lcCache*/          \
  }
/** Empty-initialized s2_ptoker object. */
extern const s2_ptoker s2_ptoker_empty;

/**
   Flags for use with s2_ptoker::flags and possibly
   related contexts.
*/
enum s2_t10n_flags {
/**
   Sentinel value. Must be 0.
*/
S2_T10N_F_NONE = 0,
/**
   If set, the '-' character is considered a legal identifier by
   s2_read_identifier2() except when the '-' appears at the start of
   the input.
*/
S2_T10N_F_IDENTIFIER_DASHES = 1,

/**
   An internal-only flag which signifies to error-handling routines
   that the given s2_ptoker is only ever used in a strictly linear
   fashion, allowing such routines to take advantage of line/column
   state of the tokenizer and avoid counting lines for error
   reporting. Note that the overwhelming majority of s2_ptokers in s2
   are NOT purely linear and it's normally impossible to know in
   advance whether one will be or not. In such cases it will use the
   position of either its error token or current token, in that order.

   Reminder to self: this was added to support s2_cptoker
   experimentation, as s2_cptoker internally uses s2_ptoker and
   "compilation" but tokenizes all the input before "compiling" it.
   This flag allows errors in that step to be reported without an
   extra line-counting step.
*/
S2_T10N_F_LINEAR_TOKER = 0x10
};

/**
   Must be passed a s2_ptoker and its input source. If len
   is negative then the equivalent of strlen() is used to calculate
   its length.

   Returns 0 on success, CWAL_RC_MISUSE if !t or !src. It has no other
   error contditions, so "cannot fail" if its arguments are valid.

   Use s2_ptoker_next_token() to fetch the next token, s2_ptoker_lookahead()
   to "peek" at the next token, and s2_ptoker_putback() to put a just-fetched
   token back.

   An s2_ptoker instance initialized via this interface does not
   strictly need to be passed to s2_ptoker_finalize(), but there is no
   harm in doing so.
*/
int s2_ptoker_init( s2_ptoker * t, char const * src, cwal_int_t len );

/**
   INCOMPLETE - DO NOT USE.

   Must be passed a s2_ptoker and its input source. If len is negative
   then the equivalent of strlen() is used to calculate its
   length. This routine, unlike s2_ptoker_init(), may allocate memory
   and may pre-parse its input, and can therefore fail in a number of
   ways. Thus its result code must always be checked.

   Calling this obligates the caller to eventually pass t to
   s2_ptoker_finalize(), regardless of whether this routine succeeds
   or not, to free up any resources this routine may have allocated.

   Returns 0 on success, CWAL_RC_MISUSE if !t or !src.

   Use s2_ptoker_next_token() to fetch the next token, s2_ptoker_lookahead()
   to "peek" at the next token, and s2_ptoker_putback() to put a just-fetched
   token back.
*/
int s2_ptoker_init_v2( cwal_engine * e, s2_ptoker * t, char const * src, cwal_int_t len,
                       uint32_t flagsCurrentlyUnused );

/**
   Resets the tokenization state so that the next call to
   s2_ptoker_next_token() will start at the beginning of the
   input. This clears the putback and error token state, and between
   this call and the next call to s2_ptoker_next_token() (or similar),
   the current token state is invalid.
*/
void s2_ptoker_reset( s2_ptoker * t );

/**
   Clears t's state and frees any memory is may be using. This does
   not free t, but will free any memory allocated on its behalf.

   Note that if any sub-tokenizers which derive from t (i.e. have t
   set as their s2_ptoker::parent value), they MUST be cleaned up
   first, as they may refer to memory owned by a parent.

   It is safe to pass this function an instance which was not passed
   to s2_ptoker_init() or s2_ptoker_init_v2() if (and only if) the
   instance was copy-initialized from s2_ptoker_empty or
   s2_ptoker_empty_m.
*/
void s2_ptoker_finalize( s2_ptoker * t );

/**
   Initializes t as a sub-tokenizer of parent, using parent->token
   as t's input range.

   Returns CWAL_RC_RANGE if parent->token does not have a valid
   byte range.

   Returns 0 on success.

   On success, [t->begin, t->end) point to the sub-tokenization range
   and t->parent points to parent.

   Results are undefined if either argument is NULL or points to
   uninitialized memory.

   If this function succeeds, the caller is obligated to eventually
   pass t to s2_ptoker_finalize(). If it fails, t will be finalized if
   needed. If this routine fails, passing t to s2_ptoker_finalize() is
   a harmless no-op if t was initially copy-initialized from
   s2_ptoker_empty or s2_ptoker_empty_m (i.e. it's in a well-defined
   state).
*/
int s2_ptoker_sub_from_toker( s2_ptoker const * parent,
                              s2_ptoker * dest);

/**
   Initializes sub as a sub-tokenizer of the given parent
   tokenizer/token combintation, using the given token as sub's input
   range.

   Returns CWAL_RC_RANGE if parent does not have a valid byte range.

   Returns 0 on success.

   On success, [t->begin, t->end) point to the sub-tokenization range
   and t->parent points to parent.

   Results are undefined if either argument is NULL or points to
   uninitialized memory.
*/
int s2_ptoker_sub_from_token( s2_ptoker const * parent, s2_ptoken const * token,
                              s2_ptoker * sub );

/* s2_ptoker const * s2_ptoker_root( s2_ptoker const * t ); */

/**
   Returns the top-most object from t->parent, or t if !t->parent.
*/
s2_ptoker const * s2_ptoker_top_parent( s2_ptoker const * t );

/**
   Returns either t->name or the first name encountered while climbing
   up the t->parent chain. Returns 0 if no name is found. If len is
   not 0 then if this function returns non-0, len is set to that
   name's length.
*/
char const * s2_ptoker_name_first( s2_ptoker const * t, cwal_size_t * len );
/**
   Returns the top-most name from t and its parent chain.
   Returns 0 if no name is found.

   FIXME: add (cwal_size_t * len) parameter.
*/
char const * s2_ptoker_name_top( s2_ptoker const * t );

/**
   Returns the first explicit non-0 error position marker from t or
   its nearest ancestor (via t->parent). If the 2nd argument is not
   NULL, *foundIn is assigned to the tokenizer in which the error
   position was found.

   @see s2_ptoker_err_pos()
*/
char const * s2_ptoker_err_pos_first( s2_ptoker const * t,
                                      s2_ptoker const ** foundIn);

/**
   Tries to return an "error position" for the given tokenizer, under
   the assumption that something has just "gone wrong" and the client
   wants to know where (or whereabouts) it went wrong. It first tries
   s2_ptoker_err_pos_first(). If that returns NULL, it returns either
   the position of pt's current token or putback token. If those are
   NULL, pt->begin is returned (indicating that tokenization has not
   yet started or failed on the first token).

   @see s2_ptoker_err_pos_first()
*/
char const * s2_ptoker_err_pos( s2_ptoker const * pt );

/**
   Fetches the next token from t. t must have been successfully
   intialized using s2_ptoker_init().

   This function is intended to be called repeatedly until it either
   returns 0 (success) AND has (t->ttype==S2_T_EOF) or until it
   returns non-0 (error). On each iteration, clients should collect
   any of the token information they need from t->token before calling
   this again (which changes t's state).

   Note that this is a lower-level function than s2_next_token().
   That one builds off of this one.
   
   On success 0 is returned and t is updated as follows:

   t->token.begin points to the start of the token. t->token.end
   points to the one-past-the-end character of the token, so the
   length of the token is (t->token.end - t->token.begin). t->ttype
   will be set to one of the S2_T_xxx constants.

   At the end of input, t->ttype will be S2_T_EOF and the token
   length with be 0 (t->begin==t->end).

   On error non-0 is returned, t->ttype will be S2_T_TokErr, and
   t->errMsg will contain a basic description of the error. On success
   t->errMsg will be 0, so clients may use that to check for errors
   instead of checking the result code or token type S2_T_TokErr. The
   bytes in t->errMsg are guaranteed to be static. On error
   t->token.begin will point to the starting position of the erroneous
   or unrecognized token.

   The underlying tokenizer is fairly grammar-agnostic but tokenizes
   many constructs as they exist in C-like languages, e.g. ++ is a
   single token (as opposed to two + tokens), and >>= is also a single
   token. A 1-byte character in the range (1,127), which is not
   otherwise already tagged with a type, gets its character's ASCII
   value set as its t->ttype.

   This function saves the pre-call current token state to a putback
   token, and the token can be "put back" by calling s2_ptoker_putback().

   Use s2_ptoker_lookahead() to "peek" at the next token while keeping
   the token iterator in place.

   Any tokenization error is assumed to be unrecoverable, and it is
   not normally useful to call this again (after an error) without
   re-initializing the tokenizer first.
*/
int s2_ptoker_next_token( s2_ptoker * t );

/**
   Sets pt's next token to be a bitwise copy of tk, such that the next
   call to s2_ptoker_next_token() will evaluate to that token. It is
   up to the caller to ensure that tk is valid for pt (e.g. by having
   read tk via a call to s2_ptoker_lookahead()).

   This is an optimization useful when one has looked-ahead in pt and
   determined that the looked-ahead token should be the next-consumed
   token. Using s2_ptoker_putback() would also be an option, but this
   approach is more efficient because it eliminates the
   re-tokenization of the put-back token.

   Note that this does not modify the putback token. If
   s2_ptoker_next_token() is called immediately after this, the
   putback token will be set to whichever token was most recently
   consumed before *this* routine was called (i.e. exactly the same as
   would happen without this routine having been called).

   @see s2_ptoker_next_token()
*/
void s2_ptoker_next_token_set( s2_ptoker * const pt, s2_ptoken const * const tk );

/**
   For a given s2_token_types values, this returns a unique
   string representation of its type ID. The returned bytes
   are static. Returns 0 for an unknown value.

   This function serves two main purposes:

   1) So that we can let gcc warn us if any values in s2_token_types
   collide with one another. (Implementing it caught two collisions.)

   2) Help with debugging. The strings returned from this function are
   intended for "informational" or "entertainment" value only, not
   (de)serialization.
*/
char const * s2_ttype_cstr( int ttype );

/**
   Similar to s2_ptoker_next_token(), but it skips over any tokens for
   which s2_ttype_is_junk() returns true. On returning, st->token
   holds the last-tokenized position.

   After this call, the putback token will be the previous token
   read before this call. i.e. the intervening junk tokens are not
   placed into the putback token.
*/
int s2_ptoker_next_token_skip_junk( s2_ptoker * st );

/**
   If st has no putback token, 0 (false) is returned and this function
   has no side-effects, otherwise st->token is replaced by the putback
   token, the putback token is cleared, and non-0 (true) is returned.

   Note that s2_ptoker_next_token_set() is often a more efficient
   option, provided semantics allow for it (which they don't always
   do). After calling this routine, s2_ptoker_next_token() must
   re-tokenize the next token, whereas s2_ptoker_next_token_set()
   allows s2_ptoker_next_token() to bypass that tokenization step.
*/
char s2_ptoker_putback( s2_ptoker * st );

/**
   Sets a bitwise copy of tok as the current putback token. It is up
   to the caller to ensure that tok is valid for the given tokenizer's
   current state.
*/
void s2_ptoker_putback_set( s2_ptoker * const st, s2_ptoken const * const tok );

/**
   Returns st's current putback token, which should be bitwise
   copied by the caller because its state may change on any calls
   to the s2_ptoker API.
*/
s2_ptoken const * s2_ptoker_putback_get( s2_ptoker const * const st );

/**
   Returns st's current error token. It never returns NULL - the
   memory is owned by st and may be modified by any future calls into
   its API. The token will have a ttype of S2_T_INVALID, and no
   begin/end values, if there is no error.
*/
s2_ptoken const * s2_ptoker_errtoken_get( s2_ptoker const * st );

/**
   Bitwise copies tok to be st's current error token. If tok is NULL,
   the error token is cleared, else it is up to the caller to ensure
   that the token is valid for st's current state.

   Note that setting an error token does not trigger an error - it is
   intended only to mark the part of the tokenizer which might be
   associated with an error, for downstream error reporting of that
   position.
*/
void s2_ptoker_errtoken_set( s2_ptoker * const st, s2_ptoken const * const tok );

/**
   Returns non-0 if st has an error token set which points to somewhere
   in st's range, else returns 0.
*/
char s2_ptoker_errtoken_has( s2_ptoker const * st );

/**
   Uses s2_ptoker_next_token() to fetch the next token, sets *tgt to the
   state of that token, resets the tokenizer position to its
   pre-call state, and returns the result of the s2_ptoker_next_token()
   call. After calling this, both the current token position and the
   putback token will be as they were before this function was
   called, but st->errMsg might contain error details if non-0 is
   returned.


   Pedantic note: the _contents_ of st->token and st->_pbToken will
   change during the life of this call, but they will be reverted
   before it returned. The point being: don't rely on pointers held
   within those two members being stable between before and after
   this call, and always reference the addresses directly from
   the current state of st->token.

   @see s2_ptoker_lookahead_skip()
*/
int s2_ptoker_lookahead( s2_ptoker * st, s2_ptoken * tgt );


/**
   A function signature for predicates which tell the caller whether
   a s2_token_types value meets (or does not meet) a certain
   condition.

   Implementations must return ttype if ttype meets their
   predicate condition(s), else false (0). Note that S2_T_INVALID
   is guaranteed by the API to be 0.
*/
typedef int (*s2_ttype_predicate_f)( int ttype );

/**
   Similar to s2_ptoker_lookahead(), but it skips over any leading
   tokens for which pred() returns true. On success *tgt contains
   the content of the token which either failed the predicate or is
   an EOF token. The client can force st to that tokenization
   position by passing it to s2_ptoker_token_set().

   Before this function returns, st->token and st->_pbToken are
   restored to their pre-call state and st->_nextToken is cleared. It
   is legal for tgt to be st->_nextToken, and that will take precedence
   over clearing that value.
*/
int s2_ptoker_lookahead_skip( s2_ptoker * st, s2_ptoken * tgt,
                              s2_ttype_predicate_f pred );

  
/**
   Works like s2_ptoker_lookahead_skip(), but inverts the meaning of
   the predicate: it stops at the first token for which pred()
   returns true.
*/
int s2_ptoker_lookahead_until( s2_ptoker * st, s2_ptoken * tgt,
                               s2_ttype_predicate_f pred );


/**
   Sets st->_pbToken to st->token, st->token to *t, and clears
   st->_nextToken.
*/
void s2_ptoker_token_set( s2_ptoker * st, s2_ptoken const * t );

#if 0
/**
   Returns a pointer to st's current token, the state of which may
   change on any any future calls into the s2_ptoker API. Never
   returns NULL.

   Though the pointer to this token currently (20200105) never changes
   (only its state does), clients are encouraged to behave as if it
   might change, as "planned potential changes" to the API may well
   make that the case.
*/
s2_ptoken const * s2_ptoker_token_get( s2_ptoker const * st );
/**
   Returns the token type (ttype) of st's current token.
*/
int s2_ptoker_ttype( s2_ptoker const * st );
#endif

/**
   Returns st->token.ttype if st's current token represents an EOF.
*/
int s2_ptoker_is_eof( s2_ptoker const * st );

/**
   Returns st->token.ttype if st's current token represents an
   end-of-expression.
*/
int s2_ptoker_is_eox( s2_ptoker const * st );

/**
   Returns ttype if ttype is an end-of-line token.
*/
int s2_ttype_is_eol( int ttype );

/**
   Returns ttype if ttype represents a "space" token
   (in any of its various incarnations).
*/
int s2_ttype_is_space( int ttype );

/**
   Returns ttype if the given token type is considered a "junk" token
   (with no syntactical meaning).

   Junk includes the following token types:

   ASCII 32d (SPACE), ASCII 13d (CR), ASCII 9d (TAB),
   S2_T_Blank, S2_T_CommentC, S2_T_CommentCpp,
   S2_T_Whitespace, S2_T_Shebang, S2_T_UTFBOM

   Note that S2_T_NL/S2_T_EOL (newline/EOL) is not considered junk
   here, as it is an expression separator in some contexts and
   skippable in others.

   Potential TODO: treat S2_T_CommentCpp as an EOL because this type
   of token implies one.
*/
int s2_ttype_is_junk( int ttype );

/**
   Returns ttype if ttype is a basic assignment op:

   S2_T_OpAssign, S2_T_ArrayAppend (internally treated as
   assignment).
*/
int s2_ttype_is_assignment( int ttype );

/**
   Returns ttype if ttype refers to one of the "combo assignment"
   operators, e.g. +=, -=, *=, etc.
*/
int s2_ttype_is_assignment_combo( int ttype );

/**
   Returns ttype if op is 0 or represents an operator which
   may legally directly proceed a unary operator.
*/
int s2_ttype_may_precede_unary( int ttype );

/**
   Returns ttype if ttype represents a symbol that unambiguously marks
   the end of an expression:

   S2_T_EOF, S2_T_EOX, S2_T_Semicolon
*/
int s2_ttype_is_eox( int ttype );

/**
   Returns ttype if ttype represents an EOF (or virtual EOF) token.
*/
int s2_ttype_is_eof( int ttype );

/**
   Returns ttype if ttype presents a "group" type:

   S2_T_ParenGroup, S2_T_BraceGroup, S2_T_SquigglyBlock
*/
int s2_ttype_is_group( int ttype );

/**
   Returns ttype if it respends a token whose value can be converted
   to a cwal_value with ease.
*/
int s2_ttype_is_object_keyable( int ttype );

/**
   Returns ttype if it represents an operator which
   is (in principal) capable of short-circuiting part
   of its arguments:

   S2_T_OpOr, S2_T_OpAnd, S2_T_Question (in the context of ternary
   if).
*/
int s2_ttype_short_circuits( int ttype );

/**
   Returns ttype if ttype represents a type which is a unary operator
   which requires an *identifier* as its operand:

   S2_T_OpIncr, S2_T_OpDecr, S2_T_OpIncrPre, S2_T_OpIncrPost,
   S2_T_OpDecrPre, S2_T_OpDecrPost

   Noting that the pre/post operator name pairs initially represent
   the same token (e.g. S2_T_OpIncrPre (++x) vs S2_T_OpIncrPost
   (x++)), requiring eval-time context to be able to distinguish
   between prefix and postfix forms.

   If it is not, 0 is returned.
*/
int s2_ttype_is_identifier_prefix( int ttype );

/**
   Returns ttype if ttype is S2_T_LiteralIntDec or one of its
   non-decimal counterparts, else returns 0.
*/
int s2_ttype_is_int( int ttype );

/**
   Works like s2_ttype_is_int(), but includes S2_T_LiteralDouble as a
   matching type.
*/
int s2_ttype_is_number( int ttype );

/**
   If pos is in the range [src,end) then this function calculates
   the line (1-based) and column (0-based) of pos within [src,end)
   and sets line/col to those values if those pointers are not
   NULL. If pos is out of range CWAL_RC_RANGE is returned and
   this function has no side-effects. Returns 0 on success.

   Note that s2 globally follows emacs line/column conventions:
   lines are 1-based and columns are 0-based.
*/
int s2_count_lines( char const * src, char const * end_,
                    char const * pos_,
                    s2_linecol_t *line, s2_linecol_t *col );

/**
   Wrapper around s2_count_lines(), which uses [pt->begin, pt->end)
   as the source range.
*/
int s2_ptoker_count_lines( s2_ptoker const * pt, char const * pos,
                           s2_linecol_t * line, s2_linecol_t * col );

/**
   Incomplete/untested. Don't use.

   tok must be a token from pt.

   This variant tries to use line/column info embedded in tok before
   falling back to counting "the hard way". Note that tok->line and
   tok->col are relative to pt->begin, and pt (or one of its parents)
   may have line/column offset info to apply to the results, making
   the line/col relative to the top-most s2_ptoker in the hierarchy.

   Bug: the tok->line/column counting is known to not work as expected
   because of how we hop around while parsing :/.
*/
int s2_ptoker_count_lines2( s2_ptoker const * pt,
                            s2_ptoken const * tok,
                            s2_linecol_t * line, s2_linecol_t * col );

#if 0
/**
   Collects info from pt which is useful in error reporting. pos is expected
   to be a position within [pt->begin,pt->end). Its line/column position
   is calculated as for s2_count_lines() (so *line will be 0 if pos is out
   of range). If pos is 0 then pt->errPos is used.

   If name is not NULL, *name is set to the value returned from
   s2_ptoker_name_top().

   Any of the (name, line, col) parameters may be 0.
*/
void s2_ptoker_err_info( s2_ptoker const * pt,
                         char const ** name,
                         char const * pos,
                         int * line, int * col );
#endif


/**
   Unescapes the raw source stream defined by [begin,end) and copies
   it to dest (_appending_ to any existing content in dest). Returns 0
   on success. On success, dest->used is set to the length of the
   unescaped content plus its old length, not counting the trailing
   NUL (but the buffer is NUL-terminated).

   Unescapes the following backslash-escaped characters: '0' (zero),
   'b' (backspace), 't' (tab), 'n' (newline), 'r' (carriage return),
   'f' (form-feed), 'v' (vertical tab), uXXXX (2-byte Unicode
   sequences), UXXXXXXXX (4-byte Unicode), backslash (reduces to a
   single backslash), single- and double-quotes[1].

   All other characters (including a NUL byte or a single slash
   appearing at the end of the input string) treat a preceding
   backslash as if it is a normal character (they retain it). The
   reason for this is to avoid that certain client code (e.g. the
   poreg (POSIX Regex) module) does not have to double-escape strings
   which have to be escaped for underlying C libraries.
   
   This is safe to use on an empty string (begin==end), in which case
   the first byte of the result will be the trailing NUL byte.

   Because this appends its results to dest, the caller may (depending
   on how he is using the buffer) need to remember the value of
   dest->used before this is called, as that will mark the point at
   which this function starts appending data.

   Returns 0 on success, CWAL_RC_MISUSE if passed invalid arguments,
   CWAL_RC_RANGE if \\uXXXX and \\UXXXXXXXX are not given 4 resp. 8 hex
   digits or if those digits resolve to a non-UTF-8 character.

   [1] = note that this routine does not know which quotes (if any)
   wrap up the input string, so it cannot know that only single- _or_
   double-quotes need unescaping. So it unescapes both.
*/
int s2_unescape_string( cwal_engine * e,
                        char const * begin,
                        char const * end,
                        cwal_buffer * dest );

/**
   Assumes that zPos is the start of an identifier and reads until the
   next non-identifier character. zEnd must be the logical EOF for
   zPos. On returning, *zIdEnd is set to the one-after-the-end
   position of the read identifier (which will be (*zIdEnd-pos) bytes
   long).

   Expects the input to be valid ASCII/UTF-8, else results are
   undefined.

   s2 treats ANY UTF-8 character outside the ASCII range as an
   identifier character.

   Returns the number of identifier characters read.

   @see s2_read_identifier2()
*/
int s2_read_identifier( char const * zPos, char const * zEnd,
                        char const ** zIdEnd );

/**
   An alternate form of s2_read_identifier() which honors the
   S2_T10N_F_IDENTIFIER_DASHES flag if it is set in the final
   argument. If that flag is not set it behaves as documented for
   s2_read_identifier().

   @see s2_read_identifier()
*/
int s2_read_identifier2( char const * zPos, char const * zEnd,
                         char const ** zIdEnd,
                         uint32_t flags );

/**
   Returns true if ch is one of:
       
   ' ' (ASCII 32d), \\t (ASCII 9d)
*/
char s2_is_blank( int ch );

/**
   Returns true if s2_is_blank(ch) is true of if ch is one of:

   \\n (ASCII 10d), \\r (13d), \\v (11d), \\f (12d)
*/
char s2_is_space( int ch );

/**
   Returns true if ch is-a digit character (0..9).
*/
char s2_is_digit( int ch );

/**
   Returns non-0 if ch is-a hexidecimal digit character (0..9,
   a..F, A..F), else returns 0.
*/
char s2_is_xdigit( int ch );

/**
   Returns non-0 if character ch is an octal digit character (0..7),
   else returns 0.
*/
char s2_is_octaldigit( int ch );

/**
   Returns non-0 if ch is an ASCII alphabetic character (a..z,
   A..Z), else returns 0.
*/
char s2_is_alpha( int ch );

/**
   Returns non-0 if s2_is_alpha(ch) or s2_is_digit(ch), else returns
   0.
*/
char s2_is_alnum( int ch );

/**
   Checks whether tok's range contains only "junk" tokens or not. If
   tok's range contains only noise tokens, 0 is returned, otherwise
   the token type ID of the first non-noise token is returned. Note
   that it also returns 0 if there is a tokenization error. This is
   primarily used to determine whether "super-tokens" (e.g.
   parenthesis/brace groups) contain any usable content before
   attempting to evaluate them.
*/
int s2_ptoken_has_content( s2_ptoken const * tok );

/**
   If token->begin is not 0 and less than token->end, then (token->end
   - token->begin) is returned, else 0 is returned (which is not a
   valid token length except for an EOF token).
*/
#define s2_ptoken_len(TOK) \
  ((cwal_size_t)((s2_ptoken_begin(TOK)            \
    && s2_ptoken_end(TOK) > s2_ptoken_begin(TOK)) \
  ? (cwal_size_t)(s2_ptoken_end(TOK) - s2_ptoken_begin(TOK)) \
                 : 0))
/*cwal_size_t s2_ptoken_len( s2_ptoken const * token );*/

/**
   If the given token has an "adjusted" begin/end range, this function
   returns the length of that range, else it behaves identically to
   s2_ptoken_len().

   Typically only group-level tokens and heredocs have an adjusted
   range.
*/
#define s2_ptoken_len2(TOK)                         \
  ((cwal_size_t)(((s2_ptoken_adjbegin(TOK) &&       \
    s2_ptoken_adjbegin(TOK)<=s2_ptoken_adjend(TOK)) \
    ? (cwal_size_t)(s2_ptoken_adjend(TOK) - s2_ptoken_adjbegin(TOK)) \
                  : s2_ptoken_len(TOK))))
/*cwal_size_t s2_ptoken_len2( s2_ptoken const * token );*/

/**
   If t is the result of s2_ptoker_next_token() and is any of the
   following types then this sets *rc (if rc is not NULL) to its
   integer representation and returns true: S2_T_LiteralIntOct,
   S2_T_LiteralIntDec, S2_T_LiteralIntHex, S2_T_LiteralIntBin. Results
   are undefined if t's state does not conform to the internal
   requirements for one of the above-listed t->ttype values (i.e. t's
   state must have been set up by s2_ptoker_next_token() or a relative
   of that function).

   Returns false (0) if all conditions are not met or if the
   conversion catches a syntax error which the tokenizer did not (but
   that "should not happen", and may trigger an assert() in debug
   builds).

   ACHTUNG: this does not handle a leading sign, as the sign is, at
   this level of the tokenization API, a separate token.

   @see s2_ptoken_parse_double()
   @see s2_cstr_parse_double()
   @see s2_cstr_parse_int()
*/
char s2_ptoken_parse_int( s2_ptoken const * t, cwal_int_t * rc );

/**
   The double counterpart of s2_ptoken_parse_int(), but only parses
   tokens with ttype S2_T_LiteralDouble. Results are undefined if
   t->ttype is S2_T_LiteralDouble but the byte range referred to by t
   is not a valid double token.

   ACHTUNG: this does not handle a leading sign, as the sign is, at
   this level of the tokenization API, a separate token.

   @see s2_ptoken_parse_int()
   @see s2_cstr_parse_double()
   @see s2_cstr_parse_int()
*/
char s2_ptoken_parse_double( s2_ptoken const * t, cwal_double_t * rc );

/**
   Works like s2_ptoken_parse_int() except that:

   - slen specifies the input length. If slen <0 then cwal_strlen(str)
     is used to calculate the input length.

   - It uses all bytes in the range [str, str+slen) as input and will
   fail if the input contains anything other than a numeric value,
   optionally with leading space.

   - It accepts a leading sign character, regardless of the integer
   notation (decimal, hex, octal). It ignores spaces around the sign
   character.

   @see s2_ptoken_parse_double()
   @see s2_ptoken_parse_int()
   @see s2_cstr_parse_double()
*/
char s2_cstr_parse_int( char const * str, cwal_int_t slen, cwal_int_t * result );

/**
   The double counterpart of s2_cstr_parse_int().

   @see s2_ptoken_parse_double()
   @see s2_ptoken_parse_int()
   @see s2_cstr_parse_int()
*/
char s2_cstr_parse_double( char const * str, cwal_int_t slen,
                           cwal_double_t * result );

/**
   Expects a filename-like string in the first slen bytes of str, with
   directory components separated by either '/' or '\\' (it uses the
   first of those it finds, in that order, as the separator). If any
   separator is found, a pointer to the last instance of it in str is
   returned, otherwise 0 is returned.
*/
char const * s2_last_path_sep(char const * str, cwal_size_t slen );

/**
   If tok->ttype is S2_T_Identifier and the token's contents are one
   of (true, false, null, undefined) then the corresponding built-in
   cwal value is returned (e.g. cwal_value_true() or
   cwal_value_null()), else NULL is returned. tok may not be NULL. The
   returned value, if not NULL, is guaranteed to be a shared instance
   which lives outside of the normal cwal_value lifetime management.
*/
cwal_value * s2_ptoken_is_tfnu( s2_ptoken const * tok );

/**
   Returns a pointer to the token's contents, setting its length to
   *len if len is not NULL. tok must not be NULL. Note that the
   returned string is almost certainly not NUL-terminated (or
   terminates at the very end of the s2_ptoker from which tok
   originates), thus capturing the length is normally required.

   This does not strip any leading/spaces of tokens which have been
   "adjusted" to do so. i.e. the whole token's contents are part of
   the returned byte range.
*/
char const * s2_ptoken_cstr( s2_ptoken const * tok,
                             cwal_size_t * len );

/**
   If tok has an "adjusted" begin/end range, this returns a pointer to
   the start of that range and len, if not NULL, is assigned the
   length of that range. If it has no adjusted range, it functions
   identically to s2_ptoken_cstr().

   Typically only group-level tokens and heredocs have an adjusted
   range.
*/
char const * s2_ptoken_cstr2( s2_ptoken const * tok,
                              cwal_size_t * len );

/**
   If st->capture is set up properly, this returns a pointer to the
   start of the range and *len (if not NULL) is set to the length of
   the captured range. If no capture is set, or it appears to be
   invalid, NULL is returned.
*/
char const * s2_ptoker_capture_cstr( s2_ptoker const * st,
                                     cwal_size_t * len );

/**
   A helper type for tokenizing conventional PATH-style strings.
   Initialize them with s2_path_toker_init() and iterate over them
   with s2_path_toker_next().
*/
struct s2_path_toker {
  /** Begining of the input range. */
  char const * begin;
  /** One-after-the-end of the input range. */
  char const * end;
  /** Position for the next token lookup. */
  char const * pos;
  /** List of token separator characters (ASCII only). */
  char const * separators;
};
typedef struct s2_path_toker s2_path_toker;
/**
   Default-initialized s2_path_toker instance, intended for const-copy
   initialization. On Windows builds its separators member is set to
   ";" and on other platforms it's set to ":;".
*/
#if defined(S2_OS_WINDOWS)
#  define s2_path_toker_empty_m {NULL,NULL,NULL,";"}
#else
#  define s2_path_toker_empty_m {NULL,NULL,NULL,":;"}
#endif

/**
   Default-initialized s2_path_toker instance, intended for
   copy initialization.

   @see s2_path_toker_empty_m
*/
extern const s2_path_toker s2_path_toker_empty;

/**
   Wipes out pt's current state by copying s2_path_toker_empty over it
   and initializes pt to use the given path as its input. If len is 0
   or more then it must be the length of the string, in bytes. If len
   is less than 0, cwal_strlen() is used to determine the path's
   length.  (When dealing with inputs which are not NUL-terminated,
   it's critical that the user pass the correct non-negative length.)

   If the client wants to modify pt->separators, it must be done so
   *after* calling this.

   Use s2_path_toker_next() to iterate over the path entries.
*/
void s2_path_toker_init( s2_path_toker * pt, char const * path, cwal_int_t len );

/**
   Given a s2_path_toker which was formerly initialized using
   s2_path_toker_init(), this iterates over the next-available path
   component in the input, skipping over empty entries (consecutive
   separator characters). 

   The separator characters are specified by pt->separators, which must
   be a NUL-terminated string of 1 or more characters.

   If a non-empty entry is found then:

   - *token is set to the first byte of the entry.

   - *len is assigned to the byte length of the entry.

   If no entry is found then:

   - *token, and *len are not modified.

   - CWAL_RC_NOT_FOUND is returned if the end of the path was found
   while tokenizing.

   - CWAL_RC_MISUSE is returned if pt->separators is NULL or empty or
   contains any non-ASCII characters.

   - CWAL_RC_RANGE is returned if called after the previous case, or
   if the input object's path has a length of 0.

   In any non-0-return case, it's not a fatal error, it's simply
   information about why tokenization cannot continue, and can
   normally be ignored. After non-0 is returned, the tokenizer must be
   re-initialized if it is to be used again.

   Example:

   @code
   char const * t = 0;
   cwal_size_t tLen = 0;
   s2_path_toker pt = s2_path_toker_empty;
   s2_path_toker_init(&pt, path, pathLen);
   while(0==s2_path_toker_next(&pt, &t, &tLen)){
      // The next element is the tLen bytes of memory starting at t:
      printf("Path element: %.*s\n", (int)tLen, t);
   }
   @endcode
*/
int s2_path_toker_next( s2_path_toker * pt, char const ** token, cwal_size_t * len );

/*
  The following macros are part of an ongoing porting/abstraction
  effort to eliminate direct access to token members, the eventual
  goal being to permit the token type to be easily swapped out so that
  we can enable "compiled" (pre-parsed) chains of tokens (which would
  require far more memory than s2 currently uses but would be tons
  faster for most scripts).
*/
#define s2_ptoken_begin(P) ((P)->_begin)
#define s2_ptoken_begin2(P) ((P)->_adjBegin ? (P)->_adjBegin : (P)->_begin)
#define s2_ptoken_begin_set(P,X) (P)->_begin = (X)
#define s2_ptoken_end(P) ((P)->_end)
#define s2_ptoken_end2(P) ((P)->_adjEnd ? (P)->_adjEnd : (P)->_end)
#define s2_ptoken_end_set(P,X) (P)->_end = (X)
#define s2_ptoken_adjbegin(P) ((P)->_adjBegin)
#define s2_ptoken_adjbegin_set(P,X) (P)->_adjBegin = (X)
#define s2_ptoken_adjbegin_incr(P,X) (P)->_adjBegin += (X)
#define s2_ptoken_adjend(P) ((P)->_adjEnd)
#define s2_ptoken_adjend_set(P,X) (P)->_adjEnd = (X)
#define s2_ptoken_adjend_incr(P,X) (P)->_adjEnd += (X)

#define s2_ptoker_begin(PT) ((PT)->begin)
#define s2_ptoker_end(PT) ((PT)->end)
#define s2_ptoker_token(PT) (PT)->token
#define s2_ptoker_len(PT) ((PT)->end - (PT)->begin)

#ifdef __cplusplus
}/*extern "C"*/
#endif
#endif
/* include guard */
/* end of file t10n.h */
/* start of file s2.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
/*
  License: same as cwal. See cwal.h resp. cwal_amalgamation.h for details.
*/
#ifndef NET_WANDERINGHORSE_CWAL_S2_H_INCLUDED_
#define NET_WANDERINGHORSE_CWAL_S2_H_INCLUDED_
/** @file */ /* for doxygen */


/** @page page_s2 s2 scripting language

   s2 is a light-weight yet flexible scripting language based on the
   cwal (Scripting Engine Without A Language) library. cwal provides
   the abstract Value types, memory lifetime management, and garbage
   collection systems, and s2 provides a scripting language on top of
   that.

   A brief overview of properties of s2 which are of most interest to
   potential clients:

   - Library licensing: dual Public Domain/MIT, with one optional
   BSD-licensed part (the JSON input parser) which can be compiled
   out if that license is too restrictive.

   - A lightweight, portable[1] C API for binding client-defined
   functionality to script-space, making it scriptable and trackable
   by the script's garbage collector. Tests so far put s2 on par
   with lua in terms of memory usage.

   - s2's primary distributable is two C files: one header and one
   implementation file, called the "amalgamation build," intended for
   direct drop-in use in arbitrary client source trees. The canonical
   build tree is intended primarily for development of cwal/s2, not
   for development of client applications. The amalgamation includes
   the cwal amalgamation, so it need not be acquired separately.

   - Has an expression-oriented syntax with a distinct JavaScript
   flavor, but distinctly different scoping/lifetime rules.

   - cwal's garbage collector guarantees that client-specific
   finalizers (for client-specified types and, optionally, for
   function bindings) get called, provided it is used properly, though
   it cannot always guaranty the order of destruction.

   - cwal and s2 are developed with the compiler set to pedantic
   warning/error levels (and them some), and the s2 test suite
   includes valgrind-based testing and reporting. New code rarely hits
   the trunk without having run the whole test suite through valgrind.

   - Extending s2: the default s2 shell (s2sh) provides not only a
   copy/paste bootstrap for creating client applications, but can be
   extended by clients via script code or C (linking their features in
   directly in or loading them via DLLs) without modifying its
   sources, as described in the manual linked to below.


   [1] = Portable means: C99 (as of July 2021). The canonical copy is
   developed using the highest level of compiler warnings, treating
   all warnings as errors (i.e. -Wall -Werror -Wpedantic -Wextra).

   In addition to these API docs, s2 as a whole is described in gross
   amounts of detail in the s2/manual directory of its canonical
   source tree, browsable online at:

   https://fossil.wanderinghorse.net/r/cwal/doc/ckout/s2/index.md

   That covers both the scripting language itself (in detail) and how
   to use it in C (mainly via overviews and links to working code).
*/
#include <time.h> /* struct tm */
#include <stdio.h> /* FILE * */

#ifdef __cplusplus
extern "C" {
#endif
typedef struct s2_engine s2_engine;
typedef struct s2_scope s2_scope;
typedef struct s2_stoken s2_stoken;
typedef struct s2_stoken_stack s2_stoken_stack;
typedef struct s2_estack s2_estack;
typedef struct s2_sweep_guard s2_sweep_guard;
typedef struct s2_func_state s2_func_state;
/** Convenience typedef. */
typedef struct s2_strace_entry s2_strace_entry;

enum s2_rc_e {
S2_RC_placeholder = CWAL_RC_CLIENT_BEGIN,
/**
   Used internally by routines which visit lists of keys/values
   to provide a mechanism for aborting traversal early without
   triggering an error.
*/
S2_RC_END_EACH_ITERATION,

/**
   To be used only by the 'toss' keyword, if it ever gets added.
*/
S2_RC_TOSS,

/** Sentinel. */
S2_RC_end,
/**
   Client-thrown exceptions which use their own error codes "should
   not" use codes below this value. Put differently, result codes
   starting at this value are guaranteed not to collide with
   CWAL_RC_xxx and S2_RC_xxx codes. Keep in mind that numerous
   CWAL_RC_xxx and S2_RC_xxx codes have very specific meanings in
   various contexts, so returning, from cwal/s2-bound client-side
   code, values which just happen to collide with those may confuse s2
   and/or cwal into Doing The Wrong Thing. (Examples: CWAL_RC_OOM is
   used solely to propagate out-of-memory (OOM) conditions, and
   handling of CWAL_RC_RETURN is context-specific.)
*/
S2_RC_CLIENT_BEGIN = S2_RC_end + 2000 - (S2_RC_end % 1000)
};

/**

   Enum specifying the precedences of operators in s2.

   Derived from:

   http://n.ethz.ch/~werdemic/download/week3/C++%20Precedence.html (now 404)

   http://en.cppreference.com/w/cpp/language/operator_precedence

   Regarding operator associativity:

   Wikipedia:

   http://en.wikipedia.org/wiki/Operator_associativity

   Says:

   "To prevent cases where operands would be associated with two
   operators, or no operator at all, operators with the same
   precedence must have the same associativity."

   We don't strictly follow that advice (by accident) but have not had
   any unexpected problems in that regard.
*/
enum s2_precedents {
S2_PR__start = 1,

/**
   Parens precedence is actually irrelevant here, as we parse parens
   groups as atomic values instead of operators.
*/
S2_PR_ParensOpen,
S2_PR_Comma,

/**
   Internal pseudo-operator for RHS evaluation in some case.

   No longer used - can be removed.
*/
S2_PR_RHSEval,

/**
   =  +=  -=  *=   /=  <<=  >>=  %=   &=  ^=  |=
*/
S2_PR_Assignment__,
/**
   The = operator.
*/
S2_PR_OpAssign =S2_PR_Assignment__,
/**
   PHP-style array-append. Essentially works like (array DOT index =
   ...), where the index is the array's current length. Only usable in
   assignment contexts.
*/
S2_PR_ArrayAppend = S2_PR_Assignment__,

/* S2_PR_Conditional__, */
/**
   In JavaScript ternary if has a higher precedence than
   assignment, but we're going to go with the C/C++ precedence
   here.
*/
S2_PR_TernaryIf = S2_PR_Assignment__,

S2_PR_Logical__,
S2_PR_LogicalOr = S2_PR_Logical__ /* || */,
S2_PR_LogicalOr3 = S2_PR_Logical__ /* ||| */,
S2_PR_LogicalOrElvis = S2_PR_Logical__ /* ?: */,
S2_PR_LogicalAnd = S2_PR_Logical__ /* && */,

S2_PR_Bitwise__,
S2_PR_BitwiseOr = S2_PR_Bitwise__,
S2_PR_BitwiseXor = S2_PR_Bitwise__ + 1,
S2_PR_BitwiseAnd = S2_PR_Bitwise__ + 2,

S2_PR_Equality__,
S2_PR_CmpEq = S2_PR_Equality__,
S2_PR_CmpEqStrict = S2_PR_Equality__,
S2_PR_CmpNotEq = S2_PR_Equality__,
S2_PR_CmpNotEqStrict = S2_PR_Equality__,

S2_PR_Relational__,
S2_PR_CmpLT = S2_PR_Relational__,
S2_PR_CmpGT = S2_PR_Relational__,
S2_PR_CmpLE = S2_PR_Relational__,
S2_PR_CmpGE = S2_PR_Relational__,
S2_PR_OpInherits = S2_PR_Relational__,

/**
   '=~'. Should this have Equality precedence?
*/
S2_PR_Contains = S2_PR_Relational__,
/**
   '!~'. Should this have Equality precedence?
*/
S2_PR_NotContains = S2_PR_Relational__,

/*
  TODO?

  <== and >== for strict-type LE resp GE

  ==< resp ==> for strict LT/GT
*/

S2_PR_Bitshift__,
S2_PR_ShiftLeft = S2_PR_Bitshift__,
S2_PR_ShiftRight = S2_PR_Bitshift__,

S2_PR_Additive__,
S2_PR_Plus = S2_PR_Additive__,
S2_PR_Minus = S2_PR_Additive__,

S2_PR_Multiplicative__,
S2_PR_Multiply = S2_PR_Multiplicative__,
S2_PR_Divide = S2_PR_Multiplicative__,
S2_PR_Modulo = S2_PR_Multiplicative__,

S2_PR_Unary__,
S2_PR_PlusUnary = S2_PR_Unary__,
S2_PR_MinusUnary = S2_PR_Unary__,
S2_PR_LogicalNot = S2_PR_Unary__,
S2_PR_BitwiseNegate = S2_PR_Unary__,
S2_PR_Keyword  = S2_PR_Unary__,
S2_PR_IncrDecr  = S2_PR_Unary__,
/**
   Used by 'foreach' to tell evaluation to stop at this operator.  Its
   precedence must be higher than S2_PR_Comma but lower than anything
   else.
*/
S2_PR_OpArrow2 /* => */ = S2_PR_Unary__,
/* C++: S2_PR_Unary__ ==>

   sizeof, new, delete, & (addr of), * (deref),
   (typeCast), 
*/

/* S2_PR_Kludge__, */
/* S2_PR_DotDot = S2_PR_Kludge__, */

S2_PR_Primary__,
S2_PR_FuncCall = S2_PR_Primary__,
S2_PR_Subscript = S2_PR_Primary__,
S2_PR_Braces = S2_PR_Primary__,
S2_PR_DotDeref = S2_PR_Primary__,
S2_PR_DotDot = S2_PR_DotDeref,
S2_PR_OpColon2 = S2_PR_DotDeref,
S2_PR_OpArrow = S2_PR_DotDeref,


/*
  C++: S2_PR_Primary__ ==>

  typeid(), xxx_cast

*/


S2_PR_Specials__,
S2_PR_ParensClose,
S2_PR_NamespaceCpp /* :: */,
S2_PR_end__
};

/**
   @internal

   Represents a combination value/operator for an s2_engine.  Each
   token represents one operand or operator for an s2 evaluation
   stack. They get allocated often, but recycled by their associated
   s2_engine, so allocations after the first few stack-pops are
   O(1) and cost no new memory.

   Token instances must not be in use more than once concurrently,
   e.g. a token may not be in more than one stack at a time, nor may
   it be in the same stack multiple times. Multiple entries may
   reference the same value, provided it is otherwise safe from being
   swept/vacuumed up.
*/
struct s2_stoken{
  /**
     A s2_token_types value.
  */
  int ttype;
  /**
     Certain token types have a value associated with them.  The
     tokenization process will create these, but will not add a
     reference to them (because doing so complicates lifetimes, in
     particular for result values which need up-scoping). This means
     the client must be careful when using them, to ensure that they
     get a ref if one is needed, and to either clean them up or
     leave them to the GC if they don't want them.
  */
  cwal_value * value;

  /**
     Used for creating chains (e.g. a stack).
  */
  s2_stoken * next;

  /**
     Used by the parser to communicate source code location
     information to the stack machine. This is necessary so that
     errors generated at the stack machine level (the operator
     implementations) can report the location information, though the
     stack machine does not otherwise know anything about the source
     code.
  */
  s2_ptoken srcPos;
};


/**
   Empty-initialized s2_stoken structure, intended for
   const-copy initialization.
*/
#define s2_stoken_empty_m {                     \
    S2_T_INVALID/*ttype*/,                      \
    0/*value*/,                               \
    0/*next*/,                                \
    s2_ptoken_empty_m/*srcPos*/               \
  }

/**
   Empty-initialized s2_stoken structure, intended for
   copy initialization.
*/
extern const s2_stoken s2_stoken_empty;
/**
   Holds a stack of s2_stokens.
*/
struct s2_stoken_stack {
  /**
     The top of the stack.

     Maintained via s2_stoken_stack_push(),
     s2_stoken_stack_pop(), and friends.
  */
  s2_stoken * top;
  /**
     Number of items in the stack.
  */
  int size;
};

/**
   Empty-initialized s2_stoken_stack structure, intended for
   const-copy initialization.
*/
#define s2_stoken_stack_empty_m {0,0}

/**
   Empty-initialized s2_stoken_stack structure, intended for
   copy initialization.
*/
extern const s2_stoken_stack s2_stoken_stack_empty;

/**
   An "evaluation stack," a thin wrapper over two s2_stoken_stacks,
   intended to simplify swapping the stacks in and out of an s2_engine
   while parsing subscripts. s2 internally uses a distinct stack per
   sub-eval, rather than one massive stack, as it's simply easier to
   manage. When the main eval loop is (re)entered, a new eval stack is
   put on the C stack (a local var of the eval function) and s2_engine
   is pointed to it so we know which stack is current.

   Reminders to self:

   - 20160131 this type was designed to _not_ ref/unref values added
   to the stack, but that may have turned out to be a mistake. It
   simplifies much other code but string interning combined with our
   internal use of "temp" values leads to unmanagable/incorrect
   refcounts for interned string values in some cases, triggering
   cwal-level corruption detection assertions. Rewiring s2 so that
   this type does much of the reference count managent for a given
   (sub)expression is certainly worth considering, but could require a
   weekend or more of intense concentration :/. Until then, the main
   eval routine uses a "ref holder" array to keep all being-eval'd
   temporaries alive and valid for the duration of the eval. That
   requires an extra cwal_array, but it isn't all that costly if
   memory recycling is on (which it should be except when testing new
   code (as recycling often hides memory misuse)).

   Current (20160215) plans:

   - expand this class to include:

   - Add a (cwal_engine*) or (s2_engine *) handle, as deps allow for
   (see below, regarding s2_engine::skipMode, for why we may need the
   latter).

   - Done: Add a (cwal_array *), allocated in the cwal_scope which is
   active when the stack was initialized. Each token pushed into
   this->vals is added to the array as a NULL entry if it has no
   cwal_value, else that value is placed in the array. This array is
   made vacuum-safe, which would eliminate s2's need for its vacsafe
   flags. It would also hold refs, so no sweep-up during the stack's
   useful lifetime.  When this->vals is popped, we either unhand or
   unref the value, depending on whether we want to (semantically)
   discard it or not. See below for a novella on this topic (it's got
   a happy ending, at least in theory).

   - We'd need to fix a few places in s2's eval engine which
   create/push a token and then add the value themselves. They would
   need to make the s2_estack aware of the token. We could do that by
   limiting token allocation to the s2_estack-internal API.

   - Need explicit init/finalizer funcs for s2_stoken_stack, to clean
   up the stash array.

   - Pushing a value token to this->vals adds it to the current stash
   array at the same index position as the stack length-1. This allows
   us to use the array as a ref holder and vacuum-safe environment for
   the values. We don't want to use it as a replacement for
   s2_stoken_stack because that needs to hold non-cwal_value state
   (and making that type manage ref handling would require a rather
   more invasive round of refactoring to clean up ref handling
   _everywhere_ else because moving ref handling into that type would
   truly bugger some stuff up (essentially all operator handlers, and
   most of the keywords, would need some touching).

   Pros and such:

   - All in-use values get refs and can be made vac-safe. This should
   eliminate any of s2's "vacsafe" flag handling (which is relatively
   invasive). It can eliminates the array used by s2_eval_ptoker(), at
   least in theory. No... i think we still need that one to ensure
   that the pending result value (between expressions) is vacuum-safe.

   - optimization: if an s2_estack starts life when
   (s2_engine::skipLevel>0) then it doesn't need a stash array because
   skipLevel will never (legaly) return to 0 within the lifetime of
   that s2_estack (because skipLevel is also incremented/decremented
   in a stack-like fashion). We need a flag for this or simply check
   this->se->skipLevel in the appropriate places. If we instead add
   the creation of the stash array as a init-time option, we can
   remove a ref on s2_engine to this class (but we'd need a
   cwal_engine ref, i any case). Having such an option might
   internally allow a simpler migration, as we could use the no-array
   setting for the first refactoring step to keep behaviour the same.

   - With that in place, we can remove a deep-seated cwal-core
   property which really annoys me: because of a less-complete
   understanding (on my part) of the intricacies of refcounting, in
   particular the handling of temp vals (in particular in conjunction
   with string interning), th1ish required the following behaviour of
   cwal: during scope cleanup (and only at scope cleanup), when
   being-finalized containers reference a value in an older scope and
   that value has (because of this container's cleanup) a refcount of
   0, the value is not destroyed, but re-temp'd back in its own scope.
   This was required to accommodate return propagation in many
   cases. It is not required, however, if the client simply does the
   right things vis-a-vis refcounting and rescoping. So...  with this
   in place, once we prove that s2 doesn't need that cwal-level
   feature, we can remove that. (i, just now, ran s2's full test suite
   (with various recycling/memory usage option combinations) without
   this behaviour, and s2 handled it fine). However, that would also
   require abandoning th1ish altogether because fixing it to do proper
   ref/unref everywhere would be far more effort than it is worth (the
   only place th1ish is used, AFAIK, is demo apps). (In my defense, at
   the time i _thought_ th1ish _was_ doing the right thing. i have
   since, via s2, discovered that that was not the case.)

   Cons (maybe):

   - We need to add a layer of push/pop APIs and keep the s2 core from
   using the s2_stoken_stack push/pop, as those don't partake in ref
   lifetime tracking.

   - Adding a stash array adds potental allocation errors for push()
   cases which are currently fail-safe. e.g. pushing a value cannot
   currently fail because it's a linked list, but adding it to an
   array might require allocating the array (on the first push) or
   allocation memory for the underlying cwal_value list (on any push).

   - We'd need a hack to keep the stash array from freeing its list
   memory when popped, because often fill/empty the same value stack
   throughout an expression. (i think... maybe...)  Ideally, we could
   use cwal_array_length_set(ar, poppedStackLength) to simply trim the
   list as it's popped, but setting the length to 0 implicitly frees
   (or recycles) the underlying (cwal_value*) list memory. We'd want
   to, as an allocation optimization, avoid that. Maybe not use
   length_set() at all, but simply cwal_array_set(ar, pos, 0), and
   keep a potentially ever-growing array.

   - It will cost an array per subexpression because
   s2_eval_expr_impl() (the main eval loop) stashes/replaces the
   s2_estack each time it is entered/leaves. We cannot change that in
   conjunction with a "stash" array because value lifetimes would go
   pear shaped quickly: the "stash" array would be global-scope (or
   close to it), which would implicity rescope all stack-pushed values
   into that scope. It would be a worst-case scenario for garbage
   collection. Experimentation with adding such an array to
   s2_eval_expr_impl() shows that it works but that the array
   allocation counts (or alloc requests) shoot through the roof, and
   real alloc counts skyrocket if value recycling is disabled. We need
   (for cwal_value lifetime reasons) a new instance for each pushed
   cwal_scope. Aha... we could have s2_estacks have a parent
   (s2_estack*), add a (cwal_scope*) to keep track of which scope it
   was initially initialized in, and inherit (via normal ref counting)
   the array. Yes. Yesssss...  that just might work nicely. We'd also
   need to track exactly which stack created the array, so we know
   which one needs to... no, we don't...  refcounts will clean it
   up. Indeed. Since we already effectively push/pop s2_estack
   instances in s2_eval_expr_impl(), the code is already structured to
   make adding/referencing a parent pointer easy to do. The stash
   array's allocation must NOT be done lazily: it must be done in the
   cwal_scope active when the stack was initialized (and within which
   it must be finalized) because the array must be allocated in a
   scope appropriate for its values, and it might not create any or
   might create them after a couple scopes have been pushed. Hmmm. In
   which case we'd need to be sure to rescope it (right after
   allocation) to s2_estack::scope (new member, a (cwal_scope*)).
   When allocating the array, we can check if the (grand...)parent's
   scope is the same. If it is, borrow its array (if it has one), else
   alloc a local array and assign it back to the parent (only if it's
   in the same scope) as a optimization for the parent's further use
   (as well as for use by a downstream child-s2_estack of that
   scope). DAMN... if we do all that, we'd need s2_estack to track the
   length the array had when it first got ahold of it, and only add
   items at that position and subsequent, and then clean up to its
   starting position when done. Hmmm. Ugly. i'd rather not share the
   arrays than do that :/. It would save some memory, though. Maybe an
   RFC after the simpler (array instance per s2_estack) approach is
   tried out and measured for allocations.
*/
struct s2_estack{
  /** The value stack. */
  s2_stoken_stack vals;
  /** The operator stack. */
  s2_stoken_stack ops;
};

/**
   Empty-initialized s2_estack structure, intended for const-copy
   initialization.
*/
#define s2_estack_empty_m {s2_stoken_stack_empty_m, s2_stoken_stack_empty_m}

/**
   Empty-initialized s2_estack structure, intended for copy
   initialization.
*/
extern const s2_estack s2_estack_empty;

/** @internal

    An internal helper type for swapping an s2_engine's sweep-guard
    state in and out at certain points.

    The docs for this struct assume only one context: that this is
    used embedded in an s2_engine struct.
*/
struct s2_sweep_guard {
  /**
     If greater than 0, s2_engine_sweep() will neither sweep nor
     vacuum.

     Reminder to self: we will likely never be able to use recursive
     sweep. It's inherently dangerous due to how we (mis)handle
     refs during eval.
  */
  int sweep;
  /**
     If greater than 0, s2_engine_sweep() will not vacuum, but will
     fall back to sweep mode (if not disabled) instead. This HAS to
     be >0 if the client makes use of any non-script-visible values
     which are not otherwise vacuum-proofed and may be needed at a
     time when script code may trigger s2_engine_sweep() (see
     cwal_value_make_vacuum_proof()).
  */
  int vacuum;

  /**
     Helps keeps track of when to sweep and vacuum - incremented once
     per call to s2_engine_sweep().
  */
  int sweepTick;
};

/** @internal

    Empty-initialized s2_sweep_guard state, intended for const
    initialization.
*/
#define s2_sweep_guard_empty_m {0,0,0}

/** @internal

   Empty-initialized s2_sweep_guard state, intended for non-const
   copy initialization.
*/
extern const s2_sweep_guard s2_sweep_guard_empty;

/** @internal

   A strictly internal type used for swapping out certain internal
   state when crossing certain subexpression boundaries (namely, block
   constructs).
*/
struct s2_subexpr_savestate {
  int ternaryLevel;
  /* Others are pending, but i've forgotten what they might need
     to be. */
};
typedef struct s2_subexpr_savestate s2_subexpr_savestate;
#define s2_subexpr_savestate_empty_m {0}

/**
   An abstraction layer over cwal_scope so that we can store
   more per-scope metadata.

   Potential TODO (20190911): add a cwal_native wrapper to each scope
   to allow script-side code to potentially do some interesting things
   with scopes. Each instance would have a prototype with methods like
   get(), isDeclared(), declare(key,val[,asConst]), and a const
   property named parent which points to the parent scope native.
   It's not 100% clear what we might want to do with such a beast,
   other than add a __scope keyword which accesses it.
*/
struct s2_scope {
  /**
     The cwal_scope counterpart of this scope. It's owned by cwal and
     must outlive this object. This binding gets set/unset via the
     cwal_scope_push()/pop() hooks.
  */
  cwal_scope * cwalScope;
  
  /**
     Current sweep/vacuum-guard info.
  */
  s2_sweep_guard sguard;

  /**
     Some internal state which gets saved/cleared when a scope is
     pushed and restored when it is popped.
  */
  s2_subexpr_savestate saved;

  /**
     A "safe zone" to store values which are currently undergoing
     evaluation and need a reference held to them. NEVER upscope this
     array to a different scope: it is internal-only, owned by this
     scope. We store this in the scope, rather than in the eval()
     routine, as an allocation optimization. This placement makes only
     a tiny difference when recycling is enabled, but when it's
     disabled, total allocation counts for the current (20171113) unit
     tests are cut by more than a third, from ~35k allocs down to
     ~21k, saving more than 1M of total alloc'd memory. (Woot! A
     _whole megabyte!_ ;)

     This member is only initialized via the main eval loop and only
     freed via the s2-side cwal_scope_pop() hook.

     Summary of why this is needed: the eval engine does not
     historically directly hold refs to temp values which are floating
     around in the eval stack while an expression is being eval'd.
     While that's not normally a problem, there are cases where such
     temp values can get freed up, in particular in the face of string
     interning, where multiple cwal_value pointers are all pointing at
     the same value yet the number of references is lower than the
     number of pointers. Fixing this properly, i.e. holding references
     to everything we put in the stack, would seemingly be a major
     undertaking. This evalHolder is a simpler workaround, but has the
     disadvantage of memory cost (whereas refs via the eval engine
     would not cost us anything we don't already have). "One of these
     days" i'd like to sit down and go patch the world such that the
     eval stack maintains references to its values, at which point we
     can get rid of evalHolder. That would require a fairly major
     effort to get all of the operators and other actors ironed out,
     and would likely introduce a very long tail of bugs while stray
     refs/unrefs were discovered. The eval holder works well, the only
     down-side being its memory cost (a few kb for s2's largest test
     scripts, _possibly_ as much as 8 or 10k on highly recursive tests
     involving require.s2).

     20181123: reminder to self: with this piece in place, a recursive
     sweep/vacuum might become legal. Hmmm. Something to try. If we
     can recursively vacuum then we can eliminate, i believe, all of
     the remaining "GC deathtrap" cases (barring malicious cases
     implemented in C to specifically bypass vacuuming by making
     values unnecessarily vacuum-proof).

     @see s2_eval_hold()
  */
  cwal_array * evalHolder;
  
  /**
     Internal flags.
  */
  uint16_t flags;
};

#define s2_scope_empty_m {\
    NULL/*cwalScope*/,                      \
    s2_sweep_guard_empty_m/*sguard*/,         \
    s2_subexpr_savestate_empty_m/*saved*/,    \
    0/*evalHolder*/,                      \
    0U/*flags*/                           \
}
extern const s2_scope s2_scope_empty;

/**
   This class encapsulates a basic stack engine which uses
   the cwal_value type as its generic operand type.

   Each s2_engine must be initialized with a cwal_engine,
   which the s2_engine owns and uses for all memory management,
   as well as the core Value Type system.

   @see s2_engine_alloc()
   @see s2_engine_init()
   @see s2_engine_finalize()
*/
struct s2_engine{
  /**
     The associated cwal_engine.
  */
  cwal_engine * e;

  /**
     A marker which tells us whether s2_engine_finalize() needs to
     free this instance or not.
  */
  void const * allocStamp;

  /**
     The stacks of operators and values. This container gets
     continually swapped out by the eval engine, such that each
     expression has its own clean stack (this is internally easier to
     handle, or seems to be).
  */
  s2_estack st;

  /**
     A general-purpose buffer for internal (re)use. Internal routines
     must be certain never to simply reset this buffer: it might be in
     use by higher-up calls in the stack. Always locally store
     buffer.used, use memory at offsets (buffer.mem+oldUsed), then
     re-set buffer.used=oldUsed and buffer.mem[oldUsed]=0 when
     finished, so that higher-up callers don't get the rug pulled out
     from under them. Growing the buffer is fine as long as all
     (internal) users properly address buffer.mem (and never hold a
     pointer to it).
  */
  cwal_buffer buffer;

  /**
     If greater than 0, "skip-mode" must be honored by all evaluation
     code. Skip-mode basically means "consume as much as you normally
     would, but have (if possible) no side-effects while doing so."
     That allows us to consume tokens with or without actually
     evaluating the results as we go (the engine pretends to evaluate,
     but uses the 'undefined' value for everything, so it doesn't
     actually allocate any values). This is the basis of short-circuit
     evaluation.
  */
  int skipLevel;

  /**
     Every this-many calls to s2_engine_sweep() should either sweep or
     vacuum. When 0, sweeping is disabled (generally not a good
     idea). The lowest-level evaluation routine disables sweeping
     during evaluation of an expression to keep the lifetimes of
     temporaries safe. s2_eval_ptoker(), a high-level eval routine,
     sweeps up after every expression, but only 1-in-skipInterval
     sweep-ups has an effect.

     Reminder to self: something to try: if s2_eval_expr_impl() uses
     an array to control lifetimes of temps (like s2_eval_ptoker()
     does, except that eval needs to keep more than one value at a
     time), we would almost not need sweeping at all, except to
     cleanup those pesky ignored return values, orphaned cycles, and
     such. That might also make a recursive sweep safe (recursive
     vacuum is theoretically not safe).

     A sweepInterval of 1 is very aggressive, but is recommended
     during testing/development because it triggers problems related
     to value lifetime mismanagement more quickly, in particular if
     vacuumInterval is also 1.

     Very basic tests on a large script with a milliseconds-precision
     and massif's instruction counter show only a relatively small
     impact, both in speed (no appreciable change) and total CPU
     instructions (less than 0.5%), when comparing sweepInterval of 1
     vs 7. The former used 90kb RAM and 215M CPU instructions, and the
     latter 92kb RAM and 214M instructions. i.e. there is (so far) no
     compelling argument for increasing this above 1.

     [Much later on... 20181123] That said, since all of the above
     text was written, it seems that all of the outlier cases
     involving temporarily stray values have been eliminated, and
     sweeping is essentially not happening in the whole s2 unit test
     suite (a total of 2 sweeps, as of this writing, meaning there's
     basically nothing to sweep). Vacuuming, on the other hand, is
     picking up stray cyclic values (and vacuuming is the only way to
     eliminate those (but only within the currently active
     scope)). That would argue for increasing the sweepInterval
     considerably, and maybe lowering the vacuumInterval to 1 or 2, so
     that every sweep becomes a vacuum (noting that vacuuming is
     computationally far more expensive). Even so... a sweepInterval
     of 1 helps detect ref/unref misuse more quickly, and the sooner
     such a problem is triggered, the easier it is to track down.  Few
     things are more irritating than an unpredictable crash caused by
     delayed garbage collection tripping over a value which was
     misused several scopes back.
  */
  int sweepInterval;

  /**
     Every this-many sweep attempts will be replaced by a vaccuum
     instead if this->scopes.current->sguard.vacuum is 0. There is no
     single optimal value. 1 is aggressive (always vacuuming instead
     of sweeping, potentially very costly). In generic tests in
     th1ish, 3-5 seemed to be a good compromise, and then only 1 in 10
     or 15 vacuum runs was cleaning up more than sweeping was, because
     vacuuming only does its real magic when there are orphaned cyclic
     structures laying around (which doesn't happen often). In scripts
     with short-lived scopes, a value of 0 here is fine because scope
     cleanup will also get those orphans, provided they're not
     propagated up out of the scope (via explicit propagation or
     containment in a higher-scoped container).
  */
  int vacuumInterval;

  /**
     Total number of s2_engine_sweep() calls which led to a sweep or
     vacuum.
  */
  int sweepTotal;

  /**
     Some sort of container used by s2_stash_get() and
     s2_stash_set().  This is where we keep internally-allocated
     Values which must not be garbage-collected. This value is made
     vacuum-proof.
  */
  cwal_value * stash;

  /**
     Internal holder for script function filename strings. This
     mechanism provides script functions with a lifetime-safe (and
     vacuum-safe) copy of their filename strings, while sharing
     like-stringed values with other function instances. It moves
     those strings into the top scope, and there's currently no
     mechanism for removing them. That's a bummer, but not a huge
     deal. The alternative would be duplicating the same (long)
     filename strings across all functions from a given file.
  */
  cwal_hash * funcStash;

  /**
     Holds the current ternary-if depth to allow the eval loop to
     provide better handling of (and error reporting for) ':' tokens.
  */
  int ternaryLevel;

  /**
     Used to communicate the "argv" value between the interpreter
     parts which call functions and the pre-call hook called by
     cwal.
  */
  cwal_array * callArgV;

  /**
     Gets set by the stack layer when an operator (A) triggers an
     error and (B) has its srcPos set. Used to communicate
     operator-triggered error location information back to the parser
     layer.
  */
  char const * opErrPos;

  /**
     Used for collecting error location info during the parse/eval
     phase of the current script, for places where we don't have
     a direct local handle to the script.
  */
  s2_ptoker const * currentScript;

  /**
     An experiment.

     20160125: reminder to self: the next time i write that without an
     explanation for my future self to understand, please flog
     me. Currently trying to figure out what this is for an am
     at a loss. It doesn't appear to do anything useful.

     20191209: used for implementing the experimental __using keyword
     to access "using" properties from inside a script function.
  */
  s2_func_state const * currentScriptFunc;

  /**
     A pointer to an app-internal type for managing "user-defined
     keywords".
   */
  void * ukwd;

  /**
     Various commonly-used values which the engine stashes away for
     its own use. Their lifetimes are managed via this->stash.
  */
  struct {
    /** The word "prototype". */
    cwal_value * keyPrototype;
    /** The word "this". */
    cwal_value * keyThis;
    /** The word "argv". */
    cwal_value * keyArgv;
    /** The word "__typename" (deprecated). */
    cwal_value * keyTypename;
    /** The word "script". */
    cwal_value * keyScript;
    /** The word "line". */
    cwal_value * keyLine;
    /** The word "column". */
    cwal_value * keyColumn;
    /** The word "stacktrace". */
    cwal_value * keyStackTrace;
    /**
       Name of the ctor function for the 'new' keyword ("__new").
    */
    cwal_value * keyCtorNew;
    /** The word "value". */
    cwal_value * keyValue;
    /** The word "name". */
    cwal_value * keyName;
    /** Part of an ongoing experiment - the word "interceptee". */
    cwal_value * keyInterceptee;
    /** The value for the s2out keyword. Initialized on its first use. */
    cwal_value * s2out;
    /** The value for the import keyword. Initialized on its first use. */
    cwal_value * import;
    /** The key for the import.doPathSearch flag. */
    cwal_value * keyImportFlag;
    /** The value for the define keyword. Initialized on its first use. */
    cwal_value * define;
  } cache;

  /**
     Stack-trace state.
  */
  struct {
    /**
       Stored as an optimization for sizing the target
       array when collecting stack traces.
    */
    cwal_size_t count;
    /**
       s2 will bail out if this call stack limit is reached, under the
       assumption that infinite recursion is going on. Set to 0 to
       disable the limt.

       Notes:

       - Only script-called functions count for this purpose, not
       calls to Functions (be they script functions or not) called via
       native code (cwal_function_call() and friends).

       - The unit test suite, as of 2014106, can get by with values
       under 10. The require.s2 tests cap out somewhere between 20 and
       25 levels. Because require.s2 is inherently deep-stacked,
       prudence dictates a 'max' value notably higher than that.
    */
    cwal_size_t max;
    /**
       Head of the current stack trace.
    */
    s2_strace_entry * head;
    /**
       tail of the current stack trace.
    */
    s2_strace_entry * tail;
  } strace;

  /**
     State for various internal recycling bins.
  */
  struct {
    /**
       Recycle bin for stokens.
    */
    s2_stoken_stack stok;

    /**
       The max number of items to keep in the stok recycler stack.
    */
    int maxSTokens;

    /**
       Recycle bin for script-function state.
    */
    struct {
      /** Head of the recyling list. */
      s2_func_state * head;
      /** Current number of entries in the list. */
      int count;
      /** Max allows entry count before we stop accepting new
          items into the list, and free them instead. */
      int max;
    } scriptFuncs;
  } recycler;

  /**
     Holds cwal_outputer instances managed by the s2_ob_push() family
     of functions.
  */
  cwal_list ob;
  /**
     Holds DLL/module handles so that the interpreter
     can close them when it cleans up.
  */
  cwal_list modules;

  /**
     State for propagating dot-operator metadata between the main
     eval() parts and the operator implementations.
  */
  struct {
    /**
       The dot and # operators sets this to the LHS container resp.
       hash. Extreme care must be taken to ensure that this does not
       point to a stale value.
    */
    cwal_value * lhs;

    /**
       Used to differentiate between prop access wrt to types which
       should bind as 'this' in calls to props resolved as
       functions. Namely:

       obj.FUNC() // binds 'this'

       array.NUMBER // does not

       hash # KEY // does not

       All three set dotOp.lhs and dotOp.key. Only "this-respecting"
       ops (namely, the DOT op) sets dotOp.self.
    */
    cwal_value * self;

    /**
       Set by the dot operator to its RHS (key) part. Used by
       assignment and/or the unset op to get access to the property
       name, as opposed to the resolved property value which the dot
       operator evaluates to.
    */
    cwal_value * key;
  } dotOp;

  /**
     Internal state for keeping s2's scopes in sync with cwal's via
     cwal_engine_vtab's cwal_scope_hook_push_f() and
     cwal_scope_hook_pop_f() mechanism.

     This addition allowed us to remove several now-obsolete
     s2-specific scope management functions and keep the
     cwal_scope/s2_scope stacks in perfect harmony, eliminating some
     weird potential/hypothetical error cases (or confusion-causing
     cases) where the scope stacks didn't line up as desired. This
     change really should have been made years ago. The only real cost
     for the s2 client is memory needed for a block of s2_scopes (via
     the 'list' member of this struct). Aside from that allocation
     (potentially, depending on the scripts, a small handful of
     reallocations totaling anywhere from 400 bytes to a few kb), it's
     been a 100% win.

     Added 20181123.
  */
  struct {
    /**
       Manages an array of s2_scope in response to cwal_scope push/pop
       events.

       How this memory is managed is not guaranteed by the API, but it
       is currently handled via cwal_realloc(), which means that it's
       counted among the memory counted in
       cwal_engine::memcap::currentMem.

       The engine will grow this list as necessary, but won't shrink
       it until the engine is finalized, at which point the memory is
       (of course) freed.

       Reminder to self (20181123): s2's various test scripts/suites
       have a max scope depth of anywhere from 9 to 22.
    */
    s2_scope * list;

    /**
       Points to memory in this->list for the current s2_scope. Note
       that it is generally NOT SAFE to keep a pointer to an s2_scope
       from this->list because any resize of the list can invalidate
       it, but this member is only modified in the routines which
       manager the list's size.
    */
    s2_scope * current;

    /**
       The number of entries reserved in this->list.
    */
    cwal_size_t alloced;

    /**
       A convenience counter which must always be the same as
       s2_engine::e::curent->level.
     */
    cwal_size_t level;

    /**
       We need a cwal_scope to act as our top scope. This is that
       scope.
    */
    cwal_scope topScope;

    /**
       An internal flag used only by s2_scope_push_with_flags() for
       passing flags to the scope-pushed hook.
    */
    uint16_t nextFlags;
  } scopes;
  
  /**
     Various internal flags.
  */
  struct {
    /**
       If greater than 0 then some debug/tracing output of the stack
       machine is generated. Use higher levels for more output. Note
       that this output goes to stdout, not s2's output channel.
    */
    int traceTokenStack;
    /**
       >0 means to trace PASSED assertions to cwal_output().
       >1 means to also trace FAILED assertions.
    */
    int traceAssertions;

    /**
       If true, sweeping keeps metrics (==performance hit) and
       outputs them to stdout. Only for debugging, of course.
    */
    int traceSweeps;

    /**
       If true (the default), creating exceptions generates a stack
       trace, else no stack trace is generated. This is simply a
       performance tweak: stack traces are normally exceedingly
       helpful but for test code which intentionally throws lots of
       exceptions, they are costly.

       Potential TODO: interpret this as a maximum depth, with 0 being
       unlimited and negative being disabled.
    */
    int exceptionStackTrace;
    
    /**
       To avoid that certain constellations miss an "interrupt"
       request, we need a flag for "was interrupted" which lives
       outside of the this->err state. TODO: also propagate
       OOM/EXIT/FATAL this way?
       
       TODO: do not clear this via s2_engine_err_reset(), and add
       a separate API for that. We really want interruption to trump
       everything.
    */
    volatile int interrupted;

    /**
       A bitmask of features which are explicitly disabled, from the
       s2_disabled_features enum.
    */
    cwal_flags32_t disable;
  } flags;

  /**
     Various metrics.
  */
  struct {
    /**
       Total number of s2_stoken_alloc() calls for this
       s2_engine instance.
    */
    unsigned int tokenRequests;
    /**
       Total number of calls into cwal_malloc() to allocate
       an s2_stoken.
    */
    unsigned int tokenAllocs;
    /**
       Number of tokens currently allocated but not yet
       freed nor recycled.
    */
    unsigned int liveTokenCount;
    /**
       Maximum number of s2_stokens alive throughout the life
       of this object.
    */
    unsigned int peakLiveTokenCount;

    /**
       Number of script-side assert()ions which have been run.
    */
    unsigned int assertionCount;
    /**
       Current sub-expression (e.g. parens/brace group) parsing
       leve.
    */
    int subexpDepth;
    /**
       The highest-ever sub-expression depth.
    */
    int peakSubexpDepth;
      
    /**
       Maximum number of cwal_scope levels deep concurrently.
    */
    cwal_size_t maxScopeDepth;
      
    /**
       Number of script-side functions created.
    */
    unsigned int funcStateRequests;

    /**
       Number of script-side functions for which we had
       to allocate an s2_func_state instance.
    */
    unsigned int funcStateAllocs;
    /**
       Total memory allocated for the internal state
       for script-side functions, NOT including
       their sourceInfo bits, as those are already
       recorded in the cwal metrics.
    */
    unsigned int funcStateMemory;

    /**
       The number of calls to s2_next_token().
     */
    unsigned int nextTokenCalls;

    /**
       The number of times which s2_engine::scopes::list is
       successfully grown to add new scopes. It does not increment on
       (re)allocation errors.
    */
    unsigned short totalScopeAllocs;

    /**
       The total number of cwal_scopes pushed via cwal_scope_push()
       and friends. This does not track the top-level scope which the
       cwal engine pushes before s2_engine can take over (which
       s2_engine immediately pops and re-pushes so that is can sync
       its scope levels with cwal's).
    */
    unsigned int totalScopesPushed;

    /**
       Counts how many times we reuse a script filename from
       this->funcStash.
    */
    unsigned int totalReusedFuncStash;
    /** Total number of times which either keyword lookup fell back to
        checking for a UKWD or the defined() keyword (or similar) made
        an explicit check for a UKWD. */
    unsigned ukwdLookups;
    /** Total number of ukwdLookups which resulted in a hit. */
    unsigned ukwdHits;
  } metrics;
};

/** @def S2_ENGINE_SWEEP_VACUUM_INTERVALS

   Two comma-separated integers representing the default values for
   s2_engine::sweepInterval and s2_engine::vacuumInterval (in that
   order).

   Long story short: the sweepInterval represent how often (measured
   in full expressions) s2_engine will "sweep", and the vacuumInterval
   represents every how many "sweeps" will be converted to a "vacuum"
   (a more costly operation, but it can weed out stray/unreachable
   cyclic structures). A sweepInterval of 1 is best when testing new
   code, as it potentially helps uncover ref/unref misuse more quickly
   than higher values do. vacuumInterval should only be 1 if
   sweepInterval is relatively high (e.g. 5-10).
*/
#if !defined(S2_ENGINE_SWEEP_VACUUM_INTERVALS)
#  if defined(NDEBUG)
#    define S2_ENGINE_SWEEP_VACUUM_INTERVALS 5/*sweepInterval*/, 2/*vacuumInterval*/
#  else
#    define S2_ENGINE_SWEEP_VACUUM_INTERVALS 1/*sweepInterval*/, 5/*vacuumInterval*/
#  endif
#endif

/** @def s2_engine_empty_m

   Empty-initialized s2_engine structure, intended for
   const-copy initialization.
*/
#define s2_engine_empty_m {                                     \
    0/*e*/,                                                     \
    0/*allocStamp*/,                                          \
    s2_estack_empty_m/*st*/,                                  \
    cwal_buffer_empty_m/*buffer*/,                            \
    0/*skipLevel*/,                                           \
    S2_ENGINE_SWEEP_VACUUM_INTERVALS,                          \
    0/*sweepTotal*/,                                          \
    0/*stash*/,                                               \
    0/*funcStash*/,\
    0/*ternaryLevel*/,                                      \
    0/*callArgV*/,                                            \
    0/*opErrPos*/,                                            \
    0/*currentScript*/,                                       \
    0/*ukwd*/,                                               \
    0/*currentScriptFunc*/,                                    \
    {/*cache*/\
      0/*keyPrototype*/,\
      0/*keyThis*/,\
      0/*keyArgv*/,\
      0/*keyTypename*/,\
      0/*keyScript*/,\
      0/*keyLine*/,\
      0/*keyColumn*/,\
      0/*keyStackTrace*/,\
      0/*keyCtorNew*/,\
      0/*keyValue*/,\
      0/*keyName*/,\
      0/*keyInterceptee*/,\
      0/*s2out*/,\
      0/*import*/,0/*keyImportFlag*/,                       \
      0/*define*/                                           \
    },                                                        \
    {/*strace*/0/*count*/, 100/*max*/, 0/*head*/,0/*tail*/},      \
    {/*recycler*/                                               \
      s2_stoken_stack_empty_m/*stok*/,                          \
      50 /*maxSTokens*/,                                      \
      {/*scriptFuncs*/ 0/*head*/, 0/*count*/, 20 /*max*/ }    \
    },                                                          \
    cwal_list_empty_m/*ob*/,                                  \
    cwal_list_empty_m/*modules*/,\
    {/*dotOp*/\
      0/*lhs*/,                                        \
      0/*self*/,                                       \
      0/*key*/                                           \
    },                                                      \
    {/*scopes*/                                                 \
      NULL/*list*/,                                             \
      NULL/*current*/,                                        \
      0/*alloced*/,                                           \
      0/*level*/,                                           \
      cwal_scope_empty_m/*topScope*/,                         \
      0/*nextFlags*/                                         \
    },                                                          \
    {/*flags*/                                                  \
      0/*traceTokenStack*/,                                     \
      0/*traceAssertions*/,                                   \
      0/*traceSweeps*/,                                       \
      1/*exceptionStackTrace*/,                             \
      0/*interrupted*/,                                     \
      0U/*disable*/                                        \
    },                                                      \
    {/*metrics*/                                                \
      0/*tokenRequests*/,                                       \
      0/*tokenAllocs*/,                                     \
      0/*liveTokenCount*/,                                \
      0/*peakLiveTokenCount*/,                              \
      0/*assertionCount*/,                                    \
      0/*subexpDepth*/,                                     \
      0/*peakSubexpDepth*/,                                 \
      0/*maxScopeDepth*/,\
      0/*funcStateRequests*/,                               \
      0/*funcStateAllocs*/,                                 \
      0/*funcStateMemory*/,                                \
      0/*nextTokenCalls*/,                                \
      0/*totalScopeAllocs*/,                              \
      0/*totalScopesPushed*/,                             \
      0/*totalReusedFuncStash*/,                        \
      0/*ukwdLookups*/,0/*ukwdHits*/              \
    }                                       \
  }

/**
   Empty-initialized s2_engine structure, intended for
   copy initialization.
*/
extern const s2_engine s2_engine_empty;


/**
   Initializes se and transfers ownership of e to it.

   se must be a cleanly-initialized s2_engine instance, allocated via
   s2_engine_alloc() or stack-allocated and copy-initialized from
   s2_engine_empty or s2_engine_empty_m (same contents, slightly
   different usage contexts).

   e must be a freshly-cwal_engine_init() instance and the client MUST
   NOT install any new functionality to it before this call because:
   in order to gain control of all the scopes, s2 must destroy all of
   e's scopes (it pushes a top scope upon initialization) before
   continuing with its own initialization.

   Returns 0 on success, a CWAL_RC_xxx code on error, the only
   potential ones at this phase of the setup being blatant misuse
   (passing in a NULL pointer) an allocation error.

   After returning:

   A) Ownership of se is not modified.

   B) If this function fails because either of the arguments is 0,
   ownership of e is not modified and CWAL_RC_MISUSE is returned,
   otherwise ownership of e is transferred to se regardless of success
   or failure (because there is no "detach and recover" strategy for
   any errors after the first couple allocations). On success or
   error, if both arguments are non-NULL, (se->e == e) will hold
   after this function returns.

   C) The caller must pass eventually se to s2_engine_finalize() to
   clean it up, regardless of success or error.

*/
int s2_engine_init( s2_engine * se, cwal_engine * e );

/**
   Allocates a new s2_engine instance using e's allocator. Returns 0
   on error.

   In practice this is not used: s2_engine instances are typically
   stack-allocated or embedded as a property of a larger object. In
   both such cases s2_engine_empty or s2_engine_empty_m should be used
   to empty-initialize their state before using them in any way.

   @see s2_engine_finalize()
   @see s2_engine_init()
   @see s2_engine_empty_m
   @see s2_engine_empty
*/
s2_engine * s2_engine_alloc( cwal_engine * e );

/**
   Frees up all resources owned by se. If se was allocated using
   s2_engine_alloc() then se is also freed, otherwise it is assumed
   to have been allocated by the caller (possibly on the stack) and
   is cleaned up but not freed.

   This call pops all cwal scopes from the stack, destroying any and
   all cwal_value instances owned by the engine, as well as cleaning
   up any memory owned/managed by those values (e.g. client-allocated
   memory managed via a cwal_native instance).

   @see s2_engine_alloc()
*/
void s2_engine_finalize( s2_engine * se );


/**
   Proxy for cwal_error_set().
*/
int s2_error_set( s2_engine * se, cwal_error * err, int code, char const * fmt, ... );

/**
   A proxy for cwal_error_exception().
*/
cwal_value * s2_error_exception( s2_engine * se,
                                 cwal_error * err,
                                 char const * scriptName,
                                 int line, int col );

/**
   Converts err (or se's error state, if err is NULL) to an Exception
   value using s2_error_exception() (see that func for important
   details) then set's se's exception state to that exception.
    
   Like cwal_exception_set(), this function returns CWAL_RC_EXCEPTION
   on success or some other non-0 code if creation of the exception
   fails (generally speaking, probably CWAL_RC_OOM).

   If line<=0 then err->line and err->col are used in place of the given
   line/column parameters.

   If script is 0 and err->script is populated, that value is used
   instead.

   To convert an error set via s2_engine_err_set() (or equivalent) to
   a cwal-level exception, pass 0 for all arguments after the first.
*/
int s2_throw_err( s2_engine * se, cwal_error * err,
                  char const * script,
                  int line, int col );

/**
   A convenience function for cwal_callback_f() implementations bound
   to/via an s2_engine-managed cwal_engine. It is (almost) equivalent
   to calling cwal_exception_setf() and passing args->engine as the
   first argument and passing on the rest of the arguments as-is. It
   returns CWAL_RC_EXCEPTION on success, some other "more critical"
   error code (e.g. CWAL_RC_OOM) if an exception cannot be thrown. The
   result is intended to be immediately returned from the callback,
   without the callback processing any additional work (other than
   cleanup/error recovery).
*/
int s2_cb_throw( cwal_callback_args const * args, int code,
                 char const * fmt, ... );


/**
   Sets se->e's error state (via cwal_error_setv()) and exception state
   (via cwal_exception_setf()) and returns CWAL_RC_EXCEPTION on
   success or a "more fatal" non-0 code on error.

   Special case: if the given code is CWAL_RC_OOM, this function has
   no side-effects and returns code as-is.
*/
int s2_throw( s2_engine * se, int code, char const * fmt, ... );

/**
   A proxy for cwal_engine_error_setv(). This does not
   set or modify the _exception_ state.
*/
int s2_engine_err_setv( s2_engine * se, int code, char const * fmt, va_list );

/**
   A proxy for cwal_engine_error_set().
*/
int s2_engine_err_set( s2_engine * se, int code, char const * fmt, ... );

/**
   If se has error state, that error code is returned, else if
   an exception is pending, CWAL_RC_EXCEPTION is returned, else
   0 is returned.
*/
int s2_engine_err_has( s2_engine const * se );

/**
   Resets se's state, as per cwal_error_reset(), plus clears any
   s2-level interruption flag. Does not clear any propagating
   exception.
*/
void s2_engine_err_reset( s2_engine * se );

/**
   Works like s2_engine_err_reset(), but also clears any propagating
   exception or "return"-style value.
*/
void s2_engine_err_reset2( s2_engine * se );

/**
   Clears se's state, as per cwal_error_clear(), plus
   clears any interruption flag.
*/
void s2_engine_err_clear( s2_engine * se );

/**
   Returns se's error state, as per cwal_error_get(), with one special
   case: if se has been "interrupted" via s2_interrupt(),
   CWAL_RC_INTERRUPTED is returned, possibly without a message (*msg
   and *msgLen may be be set to NULL resp. 0 if the interruption does
   not include a message).
*/
int s2_engine_err_get( s2_engine const * se, char const ** msg, cwal_size_t * msgLen );

/**
   A utility function implementing unary and binary addition and
   subtraction of numbers (in the form of cwal_values).

   This routine does not take overloading into account.

   To do binary operations:

   - Both lhs and rhs must be valid values. They will be converted
   to a number if needed. lhs is the left-hand-side argument and rhs
   is the right-hand-side.

   - Set doAdd to true to perform addition, 0 to perform
   subtraction.

   For unary operations:

   - As for binary ops, but lhs must be NULL. The operation is
   applied solely to rhs.

   The result value is assigned to *rv, and rv may not be NULL.

   Returns 0 on success, non-0 on misuse or allocation error.

   If lhs (binary) or rhs (unary) is a double value then the result
   (except for cases listed below) will be a double, otherwise it
   will be an integer.

   This routine makes some optimizations to avoid allocating memory
   when it knows it does not have to. These optimizations are
   internal details, but may change the expected type of the result,
   and so are listed here:

   - If !lhs and doAdd is true (i.e. unary addition), then the
   result is rhs.

   - Binary ops: if either argument is 0 resp. 0.0 then the result
   is the other argument. i.e. A+0===A and 0+A===A.

   Returns 0 on success, non-0 on error. Errors can come in the form
   of CWAL_RC_OOM or, for overloaded operators, any sort of error a
   script-side function can cause. On non-OOM error, se's error
   state will be update or an exception may be propagated.
*/
int s2_values_addsub( s2_engine * se, char doAdd,
                      cwal_value * lhs, cwal_value * rhs,
                      cwal_value **rv );


/**
   The multiply/divide/modulo counterpart of s2_values_addsub(),
   this routine applies one of those three operations to
   its (lhs, rhs) arguments and assigns the result to *rv.

   This routine does not take overloading into account.

   If mode is negative, the division operation is applied,
   If mode is 0, modulo is applies. If mode is greater than 0
   then multiplication is applied.

   Returns 0 on success, a non-0 CWAL_RC_xxx value on error.
   Returns CWAL_SCR_DIV_BY_ZERO for either division or modulo by
   0. On error, se's error state will be update or (for overloaded
   operators) an exception may be propagated.

   This routine applies several optimizations which might change
   the expected result type:

   Modulo:

   - Always has an integer result except on modulo-by-0.

   Multiplication:

   - (0 * 1) === 0
   - (0.0 * 1) === 0.0
   - (N * 1) === N
   - If either the lhs or rhs is a double then the result will be a
   double unless a more specific optimization applies.
*/
int s2_values_multdivmod( s2_engine * se, int mode,
                          cwal_value * lhs, cwal_value * rhs,
                          cwal_value **rv );

/**
   Performs bitwise and bitshift operations on one or two values.

   This routine does not take overloading into account.

   This function has two modes:

   Unary: op must be S2_T_OpNegateBitwise and lhs must be NULL.
   The bitwise negation operation is applied to rhs and the result
   is stored in *rv.

   Binary: op must be one of the many S2_T_OpXXXAssign,
   S2_T_OpXXXAssign3,
   S2_T_Op{AndBitwise,OrBitwise,XOr,ShiftLeft,ShiftRight}. The binary
   operation is applied to lhs and rhs and the result is stored in
   *rv.

   The resulting value is always of type CWAL_TYPE_INTEGER, but it may
   be optimized away to the lhs or rhs instance (as opposed to
   creating a new one).

   Returns 0 on success. On error *rv is not modified.
*/
int s2_values_bitwiseshift( s2_engine * se, int op, cwal_value * lhs,
                            cwal_value * rhs, cwal_value ** rv );


/**
   Prints out a cwal_printf()-style message to stderr and abort()s the
   application.  Does not return. Intended for use in place of
   assert() in test code. The library only uses this internally to
   confirm internal invariants in some places where an assert() would
   be triggered in debug builds.
*/
void s2_fatal( int code, char const * fmt, ... );


/**
   Flags for customizing s2_cb_print_helper()'s behaviour.
*/
enum s2_cb_print_helper_options {
S2_PRINT_OPT_NONE = 0x00,
/**
   Tells s2_cb_print_helper() to output one space character
   (ASCII 0x20) between arguments. It does not output a space
   at the start or end of the arguments.
*/
S2_PRINT_OPT_SPACE = 0x01,
/**
   Tells s2_cb_print_helper() to output one newline character (ASCII 0x0A)
   At the end of the arguments, even if it does not output any arguments.
*/
S2_PRINT_OPT_NEWLINE = 0x02,
/**
   Tells s2_cb_print_helper() to apply s2_value_unwrap() to each argument. That
   will "unwrap" one level of CWAL_TYPE_UNIQUE wrapper, but have no effect on
   non-Unique values.
*/
S2_PRINT_OPT_UNWRAP = 0x04,
/**
   Tells s2_cb_print_helper() to "return" (in the cwal_callback_f()
   sense) the callee Function value, rather than the undefined value.
*/
S2_PRINT_OPT_RETURN_CALLEE = 0x08,
/**
   Tells s2_cb_print_helper() to "return" (in the cwal_callback_f()
   sense) the call's "this" value, rather than the undefined value. If
   both this flag and S2_PRINT_OPT_RETURN_CALLEE are specified, an
   exception is triggered.

   Note that when s2 calls a function "standalone" (outside the
   context of an object property access), the function is its own
   "this", so this flag will have the same effect as
   S2_PRINT_OPT_RETURN_CALLEE in such cases.
*/
S2_PRINT_OPT_RETURN_THIS = 0x10
};

/**
   This is intended to be a proxy for print()-like cwal_callback_f()
   implementations. The first and second arguments should be passed in
   as-is from the wrapping callback's first and second arguments. The
   fourth argument should be a mask of s2_cb_print_helper_options
   flags which customize its output. The third argument...

   If skipArgCount is > 0 then that many arguments will be skipped
   over (ignored by this routine). The intent is that certain helper
   functions might accept "prefix" arguments which it processes itself
   before passing the rest on to this function.

   On success, *rv will, by default, be set to cwal_value_undefined(),
   but that may be modified by including the
   S2_PRINT_OPT_RETURN_CALLEE or S2_PRINT_OPT_RETURN_THIS flag, or
   setting *rv explicitly from the wrapping callback _after_ calling
   this.

   All output generated by this function is emitted via cwal_output(),
   so it ends up in the s2 engine's configured output channel.

   Returns 0 on success, else an error code suitable for returning
   from the wrapping callback. Potential errors include, but are not
   limited to, I/O problems and out-of-memory.

   If both the S2_PRINT_OPT_RETURN_CALLEE and S2_PRINT_OPT_RETURN_THIS
   flags are specified, an exception is triggered and
   CWAL_RC_EXCEPTION is returned, which the caller of this routine is
   expected to propagate back to its own caller. That said, the caller
   will necessarily be C code and should not pass both of those flags
   to this function.
*/
int s2_cb_print_helper( cwal_callback_args const * args,
                        cwal_value **rv,
                        uint16_t skipArgCount,
                        cwal_flags32_t flags );

/**
   A cwal_callback_f() implementation performing basic
   output-to-console. All arguments are output with a single space
   between them and a newline at the end. Output goes to
   cwal_output(), so it might not go to the console.

   The callback returns args->callee via *rv.
*/
int s2_cb_print( cwal_callback_args const * args, cwal_value **rv );

/**
   Works just like s2_cb_print() except that it does not add
   extra whitespace around its arguments, nor a newline at
   the end of the line.

   The callback returns args->callee via *rv.
*/
int s2_cb_write( cwal_callback_args const * args, cwal_value **rv );

/**
   A cwal_callback_f() implementation which sends a flush request to
   args->engine's configured output channel.  Returns 0 on success,
   throws on error.
*/
int s2_cb_flush( cwal_callback_args const * args, cwal_value **rv );

/**
   A cwal_callback_f() which implements a simple script file execution
   method. Script usage:

   thisFunc(filename)

   Runs the script using s2_eval_filename() and assigns the result of
   the last expression to *rv (or the undefined value if the script
   evaluates to NULL (which can happen for a number of reason)).

   Requires that s2_engine_from_args() returns non-0 (i.e. that
   args->engine was initialized along with a s2_engine instance).
*/
int s2_cb_import_script(cwal_callback_args const * args, cwal_value ** rv);

/**
   Returns the prototype object for just about everything. That
   instance gets stashed away in se. Ownership of the returned pointer
   is unchanged.  The caller MUST NOT unreference it
   (cwal_value_unref() or cwal_value_unhand()) unless he explicitly
   obtains a reference.
*/
cwal_value * s2_prototype_object( s2_engine * se );
cwal_value * s2_prototype_function( s2_engine * se );
cwal_value * s2_prototype_array( s2_engine * se );
cwal_value * s2_prototype_exception( s2_engine * se );
cwal_value * s2_prototype_hash( s2_engine * se );
cwal_value * s2_prototype_string( s2_engine * se );
cwal_value * s2_prototype_double( s2_engine * se );
cwal_value * s2_prototype_integer( s2_engine * se );
cwal_value * s2_prototype_buffer( s2_engine * se );
cwal_value * s2_prototype_enum(s2_engine * se);
cwal_value * s2_prototype_tuple( s2_engine * se );

/**
   Returns the first value v of v's prototype chain
   which is an enum value created by the enum keyword,
   or 0 if none are.
*/
cwal_value * s2_value_enum_part( s2_engine * se, cwal_value * v );

/**
   Returns true (non-0) if v (not including prototypes) is an
   enum-type value created by the enum keyword.
*/
int s2_value_is_enum( cwal_value const * v );

/**
   Adds a persistent value to the interpreter. These are stored, for
   lifetime purposes, under the top-most scope with one reference to
   it, and they are not visible to script code. They will be made
   vacuum-proof so long as they are in the stash.

   This is where clients might store, e.g. references to their custom
   native-side prototype objects (optionally (and preferably), they
   may set them as normal variables, but that is not always feasible).

   key must be NUL-terminated.

   Returns 0 on success.
*/
int s2_stash_set( s2_engine * se, char const * key, cwal_value * v );

/**
   Equivalent to s2_stash_set() but takes its key in the form of a
   Value instance.
*/
int s2_stash_set_v( s2_engine * se, cwal_value * key, cwal_value * v );

/**
   Fetches a value set with s2_stash_set(). Returns NULL if not found,
   if !se, or if (!key || !*key). key must be NUL-terminated.
*/
cwal_value * s2_stash_get( s2_engine * se, char const * key );

/**
   Identical to s2_stash_get() but accepts the length of the key, in bytes.
   If keyLen==0 and *key, then cwal_strlen(key) is used to calculate the
   length.
*/
cwal_value * s2_stash_get2( s2_engine * se, char const * key, cwal_size_t keyLen );

/**
   Identical to s2_stash_get2() but returns the matching cwal_kvp on a
   match (else NULL).
*/
cwal_kvp * s2_stash_get2_kvp( s2_engine * se, char const * key,
                              cwal_size_t keyLen);

/**
   Identical to s2_stash_get() but takes its key in the form of a Value
   instance.
*/
cwal_value * s2_stash_get_v( s2_engine * se, cwal_value const * key );

/**
   Wrapper around cwal_var_decl_v().

   Notable error codes:

   CWAL_RC_ALREADY_EXISTS = key already exists
*/
int s2_var_decl_v( s2_engine * se, cwal_value * key,
                   cwal_value * v, uint16_t flags );

/**
   The C-string counterpart of s2_var_decl().
*/
int s2_var_decl( s2_engine * se, char const * name,
                 cwal_size_t nameLen,
                 cwal_value * v, uint16_t flags );

/**
   Searches for a named variable in the given interpreter
   engine. scopeDepth is as described for cwal_scope_search_v()
   (normally 0 or -1 would be correct).

   Returns the found value on success, NULL if no entry is found or if
   any arguments are invalid (!se, !key).
*/
cwal_value * s2_var_get( s2_engine * se, int scopeDepth,
                         char const * key, cwal_size_t keyLen );

/**
   Functionally identical to s2_var_get(), but takes its
   key as a cwal_value.
*/
cwal_value * s2_var_get_v( s2_engine * se, int scopeDepth,
                           cwal_value const * key );

/**
   If self is not NULL then this performs a lookup in the current 
   scope (recursively up the stack) for a variable with the given 
   key. If self is not NULL then it performs a property search on 
   self, recursive up the prototype chain if necessary.  On success 
   the result is stored in *rv and 0 is returned. See s2_set_v() for 
   how Arrays are (sometimes) handled differently.
   
   If no entry is found:

   - If self has the S2_VAL_F_DISALLOW_UNKNOWN_PROPS container client
   flag then CWAL_RC_NOT_FOUND is returned and se's error state
   contains the details. e.g. enums do that.

   - Otherwise 0 is returned and *rv is set to 0.
   
   @see s2_set_v()
*/
int s2_get_v( s2_engine * se, cwal_value * self,
              cwal_value * key, cwal_value ** rv );

/**
   C-string variant of s2_get_v().
*/
int s2_get( s2_engine * se, cwal_value * self,
             char const * key, cwal_size_t keyLen,
             cwal_value ** rv );

/**
   Sets a named variable in the given interpreter engine. scopeDepth
   is as described for cwal_scope_chain_set_v() (normally 0 would be
   correct). Use a value of NULL to unset an entry.

   Returns 0 on success, CWAL_RC_MISUSE if !ie or !key, and
   potentially other internal/low-level error codes, e.g. CWAL_RC_OOM
   if allocation of space for the property fails.
*/
int s2_var_set( s2_engine * se, int scopeDepth,
                 char const * key, cwal_size_t keyLen,
                 cwal_value * v );

/**
   Functionally identical to s2_var_set(), but takes its
   key as a cwal_value.
*/
int s2_var_set_v( s2_engine * se, int scopeDepth,
                  cwal_value * key, cwal_value * v );

/**
   If self is NULL then this behaves as a proxy for
   cwal_scope_chain_set_with_flags_v(), using the current scope,
   otherwise...

   self must be NULL or a container type (!cwal_props_can(self)),
   or non-0 (not sure _which_ non-0 at the moment) is returned.

   v may be NULL, indicating an "unset" operation.

   If self is an a Array or has one in its prototype: if key is-a
   Integer then cwal_array_set() is used to set the property, else
   cwal_prop_set_v(self,key,v) is used.

   If self is-a Hashtable and s2_hash_dot_like_object() (or
   equivalent) has been used to set its "like-an-object" flag, then
   this function manipulates the hash entries, not object properties
   (hashes can have both).

   Otherwise the key is set as an object-level property of self.

   Returns the result of the underlying setter call (of which there
   are several posibilities).

   If !v then this is an "unset" operation. In that case, if !self
   and key is not found in the scope chain, CWAL_RC_NOT_FOUND is
   returned, but it can normally be ignored as a non-error.

   @see s2_get_v()
*/
int s2_set_with_flags_v( s2_engine * se, cwal_value * self,
                         cwal_value * key, cwal_value * v,
                         uint16_t kvpFlags );

/**
   Identical to s2_set_with_flags_v(), passing CWAL_VAR_F_PRESERVE as
   the final parameter.
*/
int s2_set_v( s2_engine * se, cwal_value * self,
              cwal_value * key, cwal_value * v );

/**
   C-string variant of s2_set_with_flags_v().
*/
int s2_set_with_flags( s2_engine * se, cwal_value * self,
                       char const * key, cwal_size_t keyLen,
                       cwal_value * v, uint16_t flags );

/**
   Equivalent to calling s2_set_with_flags() with the same
   arguments, passing CWAL_VAR_F_PRESERVE as the final
   argument.
*/
int s2_set( s2_engine * se, cwal_value * self,
            char const * key, cwal_size_t keyLen,
            cwal_value * v );

/**
   Installs a core set of Value prototypes into se.

   Returns 0 on success.
*/
int s2_install_core_prototypes(s2_engine * se);

enum s2_next_token_flags {
/**
   Specifies that s2_next_token() should treat EOL tokens as
   significant (non-junk) when looking for the next token.
*/
S2_NEXT_NO_SKIP_EOL = 0x01,
/**
   Specifies that s2_next_token() should not "post-process" any
   tokens. e.g. it normally slurps up whole blocks of parens and
   braces, and re-tags certain token types, but this flag disables
   that. The implication is that the caller of s2_next_token() "really
   should" use a non-null 4th argument, so that the main input parser
   does not get confused by partially-consumed block constructs or
   mis-tagged semicolons.
*/
S2_NEXT_NO_POSTPROCESS = 0x02
};

/**
   A form for s2_ptoker_next() which is specific to s2_engine
   evaluation, performing various token skipping and
   post-processing.  It searches for the next non-junk token, then
   perhaps post-processes it before returning.

   If tgt is NULL then the token is consumed as normal and st->token
   contains its state after returning. If tgt is not-NULL then the
   this works like a lookahead: the consumed token is copied to *tgt
   and st's token/putback state are restored to their pre-call state
   (regardless of success or failure).

   The flags parameter must be 0 or a bitmask of s2_next_token_flags
   values. Note that by default newlines are treated as junk tokens
   and ignored.

   Returns 0 on success. On error the input must be considered
   unparsable/unrecoverable unless st is a sub-parser of a compound
   token in a larger parser (e.g. a (list) or {script} or [braces]),
   in which case parsing may continue outside of the subparser
   (because we know, at that point, that the group tokenizes as an
   atomic token).

   This function does not throw exceptions in se, but instead
   updates se's error state on error.
*/
int s2_next_token( s2_engine * se, s2_ptoker * st, int flags, s2_ptoken * tgt );

/**
   Uses s2_next_token(se,pr,nextFlags,...) to peek at the next token
   in pr. If the next token has the type ttype, this function returns
   ttype, else it returns 0. If consumeOnMatch is true AND the next
   token matches, pr->token is set to the consumed token.
*/
int s2_ptoker_next_is_ttype( s2_engine * se, s2_ptoker * pr, int nextFlags,
                             int ttype, char consumeOnMatch );

/**
   Creates a value for a token. This basically just takes the t->src
   and makes a string from it, but has different handling for certain
   token types. Specifically:

   - Integers (decimal, octal, and hex) and doubles are created as
   the corresponding cwal numeric types.

   - Strings literals get unescaped.

   - Heredocs are not unescaped, but their opening/closing markers are
   trimmed, as are any whitespace implied by their modifier flag (if
   any).

   - S2_T_SquigglyBlock is, for historical reasons, returned as
   a string, but it does not get unescaped like a string literal.
   It does get left/right trimmed like a heredoc, though.

   - Any other token type is returned as-is as a string value,
   without unescaping.

   On success *rv contains the new value.

   Returns 0 on success, CWAL_RC_OOM on OOM, and may return
   CWAL_RC_RANGE if unescaping string content fails (so far only ever
   caused by ostensibly invalid inputs, e.g. \U sequences).

   Note: the 2nd argument is only here for error reporting in one
   corner case (unescaping a string fails).

   ACHTUNG: a returned string value might not be a new instance, and
   it is CRITICAL that the client not call cwal_value_unref() on it
   without calling cwal_value_ref() first. Not following this advice
   can lead to the caller pulling the returned string value out from
   under other code. Conversely, failing to call cwal_value_unref()
   may (depending on other conditions) lead to a cwal_scope-level leak
   until that scope is swept up or popped.
*/
int s2_ptoken_create_value( s2_engine * se,
                            s2_ptoker const * pr,
                            s2_ptoken const * t,
                            cwal_value ** rv );

/**
   Sets se's error state, as for s2_engine_err_set(), and returns that
   call's result value. If fmt is NULL or !*fm, st->errMsg is used (if
   set) as the message. If se->opErrPos or s2_ptoker_err_pos(st) (in
   that order) are set to a position within [st->begin,st->end) then
   line/column information, relative to st->begin, is appended to the
   message. Sometimes it is necessary for callers to set or tweak st
   error position explicitly before calling this (see
   s2_ptoker_errtoken_set()).

   Returns the error code, not 0, on success! Returns some other
   non-0 code on error.

   This routine takes pains not to allocate any memory, which also
   means not generating an error message, if the given error code is
   CWAL_RC_OOM.
*/
int s2_err_ptoker( s2_engine * se, s2_ptoker const * st,
                   int code, char const * fmt, ... );


/**
   Similar to s2_err_ptoker(), but clears se's error state and sets
   se's Exception state. One exception (as it were) is if the code is
   CWAL_RC_OOM, in which case se's error state is set but no exception
   is thrown and no formatted message is generated because doing
   either would require allocating memory.
*/
int s2_throw_ptoker( s2_engine * se, s2_ptoker const * pr, int code,
                     char const * fmt, ... );

/**
   Throws se's error state (which must be non-OK), using pr (if not
   NULL, else se->currentScript) as the source for error location
   information. It tries to determine the position of the error
   based on se's and pr's state at the time this is called. It is
   particularly intended to be called from code which:

   a) is calling non-throwing, error-producing code (e.g. op-stack processing).

   b) wants to convert those errors to script-side exceptions.

   Returns CWAL_RC_EXCEPTION on success, some "more serious" non-0
   code on errors like allocation failure (CWAL_RC_OOM).
*/
int s2_throw_err_ptoker( s2_engine * se, s2_ptoker const * pr );

/**
   Throws a value as an exception in se.

   If pr is not NULL, it is used for collecting error location
   information, which is set as properties of the exception. If pr is
   NULL, se's current script is used by default. If both are NULL,
   no script-related information is added to the exception.

   If v is-a Exception (including inheriting one), it is assumed that
   the exception already contains its own error state: errCode is
   ignored and script location information is only added if v does not
   appear to have/inherit that information.

   If v is-not-a Exception, v is used as the "message" property of a
   newly-created exception, and errCode its "code" property.

   20191228: behaviour changed slightly for the case that v is-a
   Exception, in that script location information are no longer always
   harvested because doing so normally leads to having duplicated
   error state (most notably the stack trace) in the inherited
   exception and v. If, however, v appears to inherit no script
   location information (which can happen if the inherited exception
   was born in native space, as opposed to via the script-side
   exception() keyword), script info is harvested and installed in v
   (not in its inherited exception object).
*/
int s2_throw_value( s2_engine * se, s2_ptoker const * pr, int errCode, cwal_value *v );


/**
   Flags for eventual use with s2_eval_expr() and friends.
*/
enum s2_eval_flags_t {
/**
   Treat the eval-parsing as a lookahead of the expression token(s)
   instead of consuming them. It may still _evaluate_ the contents
   on its way to finding the end of the expression, unless
   S2_EVAL_SKIP is used as well.

   Reminder to self: it seems that this is no longer used internally,
   so it's a candidate for removal.
*/
S2_EVAL_NO_CONSUME = 0x01,
/**
   Forces the parsing (if not already done so by a higher-level
   parse) into "skip mode."

   Used for short-circuit evaluation, which only evaluates the
   tokens for syntactical correctness, without having side-effects
   which affect the script's result. It is legal to use S2_EVAL_SKIP
   in conjunction with S2_EVAL_NO_CONSUME, as this can be used to
   confirm syntactic correctness (at least for the top-most level of
   expression) and find the end of the expression point without
   "really" evaluating it. i.e. for short-circuit logic.
*/
S2_EVAL_SKIP = 0x02,
/**
   NOT YET IMPLEMENTED. May never be.

   Treat comma tokens as end-of-expression instead of as a binary
   operator.
*/
S2_EVAL_COMMA_EOX = 0x04,

/**
   Inidicates that a new cwal_scope should be pushed onto the (cwal)
   stack before expression parsing starts, and popped from the stack
   when it ends. The result value of the expression will be up-scoped
   into the calling (cwal) scope.
*/
S2_EVAL_PUSH_SCOPE = 0x10,

/**
   An internal-use flag, and bits at this location and higher this are
   reserved for internal use.
*/
S2_EVAL_flag_bits = 16
};

/**
   Tokenizes and optionally evaluates one complete expression's worth
   of token state from st. This is the core-most evaluator for s2 -
   there is not a lower-level one in the public API. Care must be
   taken with lifetimes, in particular with regards to sweeping and
   vacuuming, when using this routine.

   If flags contains S2_EVAL_NO_CONSUME, it behaves as is if it
   tokenized an expression and then re-set st's token state. This
   can be used to check expressions for eval'ability without
   consuming them. Be careful not to use that flag in a loop, as it
   will continually loop over the same tokens unless the caller
   manually adjusts st->token between calls.

   If rv is not NULL then on success the result of the expression
   is written to *rv. Lifetime/ownership of the expression is not
   modified from its original source (which is indeterminate at this
   level), and the client must take a reference if needed.

   If rv is not NULL and the expression does not generate a value
   (e.g. an empty expression or EOF), *rv is set to 0.

   On any sort of error, *rv is not modified.

   Sweeping is disabled while an expression is running, to avoid that
   any values used in the expression can live as long as they need to
   without requiring explicit references everywhere.

   If flags contains S2_EVAL_SKIP then this function behaves
   slightly differently: it parses the expression and runs it
   through the normal evaluation channels, but it does so with "as
   few side-effects as possible," meaning it can be used to skip
   over expressions without (in effect) evaluating them. When
   running in "skip mode," all operations are aware that they should
   perform no real work (e.g. not allocating any new values), and
   should instead simply consume all their inputs without doing
   anything significant. This allows us to pseudo-evaluate an
   expression to find out if it could be evaluated. In skip mode, if
   rv is not NULL then any result value written to *rv "should" (by
   library convention) be 0 (for an empty expression) or
   cwal_value_undefined(). Achtung: if se->skipLevel is positive
   then this function always behaves as if S2_EVAL_SKIP is set,
   whether it is or not. All this flag does is temporarily
   increments se->skipLevel.

   Returns 0 on success or any number of CWAL_RC_xxx values on
   error. On error, se's error state is updated and/or a cwal-level
   exception is thrown (depending on exactly what type of error and
   when/where it happened).

   On success st->token holds the token which caused expression
   tokenization to terminate. On non-error, if st->token is not at
   its EOF, evaluation may continue by calling this again. When
   running in consuming mode (i.e. not using S2_EVAL_NO_CONSUME),
   then this routine sets st's putback token to the pre-call
   st->token (i.e.  the start of the expression). That means that a
   s2_ptoker_putback() will put back the whole expression.

   Upon returning (regardless of success or error), st.capture will
   point to the range of bytes captured (or partially captured
   before an error) by this expression.

   If st->token is an EOF or end-of-expression (EOX) token after this
   is called, *rv might still be non-0, indicating the expression
   ended at an EOF/EOX. This is unfortunate (leads to more work after
   calling this), but true.

   Before evaluation, se's current eval stack is moved aside, and it
   is restored before returning. This means that calls to this
   function have no relationship with one another vis-a-vis the stack
   machine. Any such relationships must be built up in downstream
   code, e.g. by calling this twice and using their combined result
   values. This property allows the engine to recover from syntax
   errors at the expression boundary level without the stack
   manipulation code getting out of hand. It also means that
   subexpressions cannot corrupt the stack parts used by the parent
   (or LHS) expression(s).

   Garbage collection: this routine cannot safely sweep up while it is
   running (and disables sweep mode for the duration of the
   expression, in case a subexpression triggers a
   sweep). s2_eval_ptoker() and friends can, though. Any temporaries
   created in the current scope by this routine may be swept up after
   it is called, provided the client has references in place wherever
   he needs them (namely, on *rv).  If the S2_EVAL_PUSH_SCOPE flag is
   used, sweepup is not necessary because only *rv will survive past
   the pushed scope. That said, pushing a scope for a single
   expression is just a tad bit of overkill, and not really
   recommended. Much Later: as of late 2017, this routine doesn't leak
   any temporaries by itself, but the arbitrary script code it calls
   might hypothetically do so via native code.
*/
int s2_eval_expr( s2_engine * se, s2_ptoker * st,
                  int flags, cwal_value ** rv);

/**
   Flags for use with s2_eval_ptoker() and (potentially) friends.
*/
enum s2_eval_flags2_t {
/**
   Guaranteed to be 0, indicating no special flags.
 */
S2_EVALP_F_NONE = 0,
/**
   Indicates that s2_eval_ptoker() should propagate CWAL_RC_RETURN
   unconditionally. Normally it will transate that result into a
   "return" value for the caller, the return 0. With this flag, it
   will leave the s2-level return-handling semantics to the caller.
*/
S2_EVALP_F_PROPAGATE_RETURN = 0x01
};

/**
   Evaluates all expressions (iteratively) in the s2 script code
   wrapped by pt. pt contains the source code range, its optional
   name, and its tokenizer lineage (if used in a sub-parser context).

   e2Flags may be 0, indicating no special evaluation flags, or a
   bitmask of values from the s2_eval_flags2_t enum.

   If rv is NULL then any result value from the parsed expressions
   is ignored.

   If rv is not NULL then the final result of the script is stored
   in *rv. Its ownership is unspecified - it might be a new
   temporary awaiting a reference (or to be discarded) or it might
   be a long-lived value which made its way back from the global
   scope. We just can't know at this point. What this means is: if
   the caller needs to use *rv, he needs to do so immediately
   (before the next sweep-up in the current scope or the current
   scope ending). He may obtain a reference in "any of the usual
   ways."

   Returns 0 on success, a CWAL_RC_xxx value on error. On error, se's
   error state and/or se->e's exception state will be set (depending on
   what caused the error) and *rv will not be modified.

   Garbage collection: while iterating over expressions, this routine
   briefly holds a reference to the pending result value, and sweeps
   up temporaries using s2_engine_sweep() (meaning that it may or may
   not periodically clean up temporaries). The reference to *rv is
   released (without destroying *rv) before this function returns,
   meaning that *rv may have been returned to a probationary
   (temporary) state by this call. If the caller needs to ensure its
   safety vis-a-vis sweepup, he must obtain a reference to it. If
   clients are holding temporaries in the current scope, they need to
   push a cwal scope before running this, and pop that scope
   afterwards, upscoping *rv to the previous if necessary (see
   cwal_scope_pop2() and cwal_value_rescope()).

   Nuances:

   - See s2_eval_expr() for lots more details about the parsing and
   evaluation process.

   - If pr->name is set then it is used as the name of the script,
   otherwise a name will be derived from pr->parent (if set),
   recursively.

   - A value followed by an end-of-expression (EOX: semicolon,
   end-of-line, or (in some cases) end-of-file) results to that
   value. A second "hard EOX" (i.e. a semicolon) will set the result
   to NULL. For purposes of counting the first EOX, the EOL token is
   considered an EOX, but multiple EOLs are not treated as multiple
   EOX. Examples: "3;" === Integer 3, but "3;;" === C-level NULL.
   Adding newlines between (or after) the final value and the
   semicolons does not change the result.

   - *rv (if not 0) may be assigned to 0 even if the script
   succeeds. This means either an empty script or a series of
   semicolon/EOX tokens have removed the result from the stack (as
   explained above).

   - If the expression triggers a CWAL_RC_RETURN result AND pt->parent
   is NULL and e2Flags does _not_ have the S2_EVALP_F_PROPAGATE_RETURN
   bit set, then the "return" result is treated as a legal value, and
   any pending/propagating result is passed on to the caller via
   *rv. If pt->parent is not NULL (or S2_EVALP_F_PROPAGATE_RETURN)
   then CWAL_RC_RETURN is (needs to be) propagated up, and is returned
   as an error (it is treated as one until it hits and handler which
   accepts "return" results and knows how to fiddle s2_engine's
   internals when doing so).

   Notes about "return" handling in s2:

   Return semantics are implemented by doing a combination of:

   - Calling s2_propagating_set() to set a "propagating" value. That
   part keeps the value propagating up the popping scope stack.

   - Returning CWAL_RC_RETURN from the routine in question. When this
   value is returned, consumers expecting it normally assert() that
   s2_propagating_get() returns non-NULL, as they expect it to have
   been set to implement 'return' keyword semantics.

   The point being: if callers want to propagate a 'return' from
   the called script, they must:

   - Pass S2_EVALP_F_PROPAGATE_RETURN in the e2Flags bits.

   - Accept the result code of CWAL_RC_RETURN as a non-error.

   - Must call s2_propagating_take(se) to take over the returned value
   or s2_propagating_set(se,0) to clear it (_potentially_ destroying
   it immediately). It might be a temporary, it might not be - its
   origin is indeterminate. If the caller needs to ensure it is kept
   alive, he must arrange to do so, e.g. via getting a reference via
   cwal_value_ref() and potentially (depending on his needs) making it
   vacuum-safe by inserting it into a vacuum-safe container or using
   cwal_value_make_vacuum_proof(). (Note, however, that vacuum-safing
   is a rare need, intended only for use with values which will be
   held around in C-space and never exposed to the script world.)

   - If the caller wants to propagate the result value further up the
   cwal scope stack, he should use cwal_value_rescope() (or
   s2_value_upscope()) to move it one scope up the stack before
   popping his scope. (Alternatively, cwal_scope_pop2() can pop the
   scope and rescope a single propagating value at the same time.)

   Similar handling is used for exit and break keywords, with the
   CWAL_RC_EXIT and CWAL_RC_BREAK result codes. The throw keyword
   returns the CWAL_RC_EXCEPTION code and its exception value is
   fetched/reset using cwal_exception_get(), cwal_exception_set(), and
   cwal_exception_take(), but exceptions are triggered in many places
   from C code, as opposed to only via the throw keyword (which uses
   the same mechanism).
*/
int s2_eval_ptoker( s2_engine * se, s2_ptoker * pt, int e2Flags,
                    cwal_value **rv );

/**
   Functionally equivalent to using s2_eval_ptoker() with a
   s2_ptoker initialized to use the source range
   [src,src+srcLen). The name parameter may be 0 - it is used when
   generating error location information. If srcLen is negative,
   cwal_strlen() is used to calculate src's length.

   If newScope is true, a new scope is used, otherwise the code is
   eval'd in the current scope, which can have unwanted side effects
   if the evaluation sweeps or (more particularly) vacuums up while
   the calling code has non-sweep/vacuum-safe values laying around
   (which it might, without knowing it). Unless you are 100% certain
   about the current state of the scripting engine and all client-side
   values owned by that engine, always use a new scope when running
   this function. (Pro tip: you can almost never be 100% certain about
   those conditions!)

   If newScope is true, rv is not NULL, and evaluation is successful,
   *rv gets rescoped (if needed) into the calling scope before
   returning. On error, *rv is not modified.

   @see s2_eval_buffer()
   @see s2_eval_cstr_with_var()
*/
int s2_eval_cstr( s2_engine * se,
                  char newScope,
                  char const * name,
                  char const * src, int srcLen,
                  cwal_value **rv );

/**
   This is a convenience wrapper around s2_eval_cstr() which does the
   following:

   1) Pushes a new cwal scope, returning immediately if that fails.

   2) Declares a scope-local variable with the name/value provided by
   the 2nd and 3rd arguments.

   3) Evaluates the given script code. (Presumably this script
   references the local new variable.)

   4) Pops the new scope. On success, if rv is not NULL, the result
   value of the eval is written to *rv. If rv is NULL, or on error,
   the result value (if any) is discarded.

   All parameters except for the 2nd and 3rd function as described for
   s2_eval_cstr().

   It is important that the caller hold a reference to the 3rd
   argument before calling this.

   This function fills a relatively common niche where a module wants
   to perform some of its initialization in script form and only needs
   a single local var reference to do it.

   @see s2_eval_buffer()
   @see s2_eval_cstr()
*/
int s2_eval_cstr_with_var( s2_engine * se,
                           char const * varName,
                           cwal_value * varValue,
                           char const * scriptName,
                           char const * src, int srcLen,
                           cwal_value **rv );

/**
   A convenience wrapper around s2_eval_cstr(). An empty
   buffer is treated like a an empty string ("").
*/
int s2_eval_buffer( s2_engine * se,
                    char newScope,
                    char const * name,
                    cwal_buffer const * buf,
                    cwal_value **rv );

/**
   Like s2_eval_cstr(), but evaluates the contents of a file. fname is
   the name of the file. fnlen is the length of the filename, which
   must be non-0, but cwal_strlen() is used if fnlen is negative.

   If pushScope is true then the contents are run in a new scope,
   otherwise they are run in the current scope (quite possibly not
   what you want, but go ahead if you want). If rv is not NULL then
   the result value of the evaluation (if any) is assigned to *rv.  If
   *rv is not 0 upon returning then *rv will (if needed) have been
   moved into the calling cwal scope when this returns.

   If the script triggers a CWAL_RC_RETURN code then this function
   treats that as a success result, sets *rv (if rv is not NULL) to
   the 'return' value, and stops automatic propagation of that
   value. Most non-exception errors get converted to exceptions, so as
   to not be fatal to the importing script.

   Returns 0 on success. On error it may return a number of things:

   - CWAL_RC_OOM indicates an allocation failure.

   - CWAL_RC_EXCEPTION indicates that the file's script contents
   threw or propagated an exception, which is available via
   cwal_exception_get().

   - CWAL_RC_FATAL: means 'fatal' was called (which implicitly
   triggers an exception). Its pending exception can be found in
   cwal_exception_get().

   - CWAL_RC_EXIT: means the 'exit' keyword was called. Its result value
   can be found in cwal_propagating_get().

   - Most other non-0 codes cause se's error state to be updated with
   more information (see s2_engine_err_get()).
*/
int s2_eval_filename( s2_engine * se, char pushScope,
                      char const * fname,
                      cwal_int_t fnlen,
                      cwal_value ** rv );


/**
   Appends the given value to the given buffer in string form.
   Returns 0 on success.

   Objects and Arrays/Tuples are buffered in JSON form, and this
   function will fail if traversing them discovers cycles.
*/
int s2_value_to_buffer( cwal_engine *e, cwal_buffer * buf,
                        cwal_value * arg );

/**
   A cwal_callback_f() impl which uses s2_value_to_buffer() to convert
   args->self to a string. The other arguments are ignored.
*/
int s2_cb_value_to_string( cwal_callback_args const * args, cwal_value **rv );

/**
   Returns se's underlying cwal_engine instance (used by
   much of the lower-level scripting engine API). It is owned
   by se.
*/
cwal_engine * s2_engine_engine(s2_engine * se);

/**
   cwal_callback_f() implementation which acts as a proxy for
   cwal_value_compare().

   If passed two values it passes those to cwal_value_compare(), else
   it passes args->self and args->argv[0] to it (in that order).

   If both the lhs/rhs values are the same pointer, they of course
   compare as equivalent (without calling cwal_value_compare()).

   If passed 3 values and the 3rd is truthy, it checks to see if the
   values have the same type. If they do not, it returns an arbitrary
   (but stable) non-0 value. If they do have the same type, or if the
   3rd parameter is falsy, it behaves as if passed 2 parameters.

   Throws on usage error, else returns the result of the comparison
   via *rv.

   Script signatures:

   integer compare(value rhs); // requires a 'this'

   integer compare(value lhs, value rhs[, boolean typeStrict = false]);

   The latter form works independently of the current 'this'.
*/
int s2_cb_value_compare( cwal_callback_args const * args, cwal_value **rv );


/**
   Behaves more or less like the access(2) C function (_access() on
   Windows builds).

   Returns true if the given filename is readable (writeable if
   checkForWriteAccess is true), else false.
*/
char s2_file_is_accessible( char const * fn, char checkForWriteAccess );

/**
   Checks for the existence of a directory with the given NUL-terminated
   name. If passed a true 2nd argument then it also checks whether the
   directory is writeable.

   Returns true (non-0) if the directory exists and (if
   checkForWriteAccess is true) writeable. If checkForWriteAccess is
   false, it returns true if the directory can be stat()ed.

   Returns 0 if the given name cannot be stat()ed or if
   checkForWriteAccess is true and the directory is not writeable.

   Currently on works on Unix platforms. On others it always returns
   0.
*/
char s2_is_dir( char const * name, char checkForWriteAccess );


/**
   cwal_callback_f() impl binding s2_file_is_accessible() in scriptable form:

   fileIsAccessible(string filename [, bool checkWriteMode=false])

   This function throws an exception if s2_disable_check() for the flag
   S2_DISABLE_FS_STAT fails.
*/
int s2_cb_file_accessible( cwal_callback_args const * args, cwal_value **rv );

/**
   The directory counterpart of s2_cb_file_accessible().
*/
int s2_cb_dir_accessible( cwal_callback_args const * args, cwal_value **rv );

/**
   cwal_callback_f() impl binding realpath(3). Script usage:

   string realpath(path)

   Returns the undefined value if it cannot resolve the name. Throws
   on error or if the library is built without realpath(3) support.
*/
int s2_cb_realpath( cwal_callback_args const * args, cwal_value **rv );

  
/**
   A cwal_callback_f() impl which passes args->self through
   cwal_json_output() to produce JSON output. Assigns the resulting
   string value to *rv. If args->argc is not 0 then args->argv[0] is
   used to specify the indentation, as per cwal_json_output_opt.

   Script usage depends on whether or not args->self is-a (or inherits)
   Buffer. If not, then the function's usage looks like:

   string t = self.toJSONToken([indentation=0 [, cyclesAsStrings=false]])

   and returns the JSON-ified from of self.
   
   If self is-a Buffer, it looks like:

   self.toJSONToken(Value v [, indentation=0 [, cyclesAsStrings=false]])

   It appends the JSON form of v to self and returns self.

   If cyclesAsStrings is true, recursion/cycles are rendered in some
   useless (debugging only) string form, otherwise cycles cause an
   exception to be thrown.
*/
int s2_cb_this_to_json_token( cwal_callback_args const * args, cwal_value **rv );

/**
   A cwal_callback_f() which passes its first argument through
   cwal_json_output() to produce JSON output. Assigns the resulting
   string value to *rv. If args->argc is greater than 1 then the
   second argument specifies the indentation: a positive number for
   that many spaces per level and a negative number for that many hard
   tabs per level.


   string t = toJSONToken(value, [indentation=0 [, cyclesAsStrings=false]])

   and returns the JSON-ified from of the value.

   If cyclesAsStrings is true, recursion/cycles are rendered in some
   useless (debugging only) string form, otherwise cycles cause an
   exception to be thrown.
*/
int s2_cb_arg_to_json_token( cwal_callback_args const * args, cwal_value **rv );


/**
   A cwal_callback_f() implementation which parses JSON string input.

   Script usage:

   @code
   var json = '{"a":"hi!"}';
   var obj = thisFunction(json)
   @endcode
*/
int s2_cb_json_parse_string( cwal_callback_args const * args, cwal_value **rv );

/**
   The file-based counterpart of s2_cb_json_parse_string(). It works
   identically except that it takes a filename as input instead of a
   JSON string.

   This callback honors the S2_DISABLE_FS_READ limitation, but it also
   suggests that we may want a less-strict fs-read-disabling option
   which allows JSON (since JSON is known to not allow execution of
   foreign code nor loading of non-JSON content).
*/
int s2_cb_json_parse_file( cwal_callback_args const * args, cwal_value **rv );

/**
   cwal_callback_f() impl which works like getenv(3). Expects one
   string argument (throws if it does not get one) and returns (via
   *rv) either a string or the undefined value.
*/
int s2_cb_getenv( cwal_callback_args const * args, cwal_value **rv );

/**
   Installs JSON functionality into the given target value or (if (key
   && *key)) a new Object property (with the given name) of that target
   value. The target must be a container type.

   The functions installed:

   Object parse(String|Buffer jsonString)

   Object parseFile(String filename)

   string stringify(Value [, int indentation=0])

   mixed clone(object|array) is equivalent to parse(stringify(value)),
   but is defined in such a way that its "this" need not be this JSON
   module. i.e. the function reference can be copied and used
   independently of the target object, regardless of whether
   target[parse] and target[stringify] are currently visible symbols.

   Returns CWAL_RC_TYPE if target is not a container-capable type.

   Returns 0 on success.
*/
int s2_install_json( s2_engine * se, cwal_value * target,
                     char const * key);

/**
   If e was created in conjunction with an s2_engine and bound as its
   client state using &s2_engine_empty as the type ID, this function
   returns it, else it returns 0. That binding happens during
   s2_engine_init(), so it will be set if e was successfully processed
   via that routine.
*/
s2_engine * s2_engine_from_state( cwal_engine * e );

/**
   Equivalent to s2_engine_from_state(args->engine).

   This is intended for use in cwal_callback_f() implementations, for
   the case that they need the underlying s2_engine (most don't).
*/
s2_engine * s2_engine_from_args( cwal_callback_args const * args );


/**
   The "Path Finder" class is a utility for searching the filesystem
   for files matching a set of common prefixes and/or suffixes
   (i.e. directories and file extensions).

   @see s2_new_pf()
   @see s2_pf_value()
   @see s2_value_pf()
   @see s2_value_pf_part()
   @see s2_pf_dir_add()
   @see s2_pf_dir_add_v()
   @see s2_pf_dirs()
   @see s2_pf_dirs_set()
   @see s2_pf_ext_add()
   @see s2_pf_ext_add_v()
   @see s2_pf_exts_set()
   @see s2_pf_exts()
   @see s2_pf_search()
*/
typedef struct s2_pf s2_pf;

/**
   Creates a new PathFinder instance. PathFinders are bound to cwal as
   cwal_native instances and are initially owned by the currently
   active scope. Returns NULL on allocation error.
*/
s2_pf * s2_pf_new(s2_engine * se);

/**
   Returns the underlying cwal_value which acts as pf's "this".

   pf may not be NULL.

   @see s2_value_pf()
   @see s2_value_pf_part()
*/
cwal_value * s2_pf_value(s2_pf const * pf);

/**
   If v is-a PathFinder or derives from it, this function returns the
   s2_pf part of v or one of its prototypes.

   It is legal for v to be NULL.

   @see s2_value_pf()
   @see s2_pf_value()
*/
s2_pf * s2_value_pf_part(cwal_value const *v);

/**
   If v was created via s2_pf_new() then this function returns
   its s2_pf counterpart, else it returns NULL.

   It is legal for v to be NULL.

   @see s2_pf_value()
   @see s2_value_pf_part()
*/
s2_pf * s2_value_pf(cwal_value const * v);

/**
   Adds a directory to pf's search path. dir must be at least dirLen bytes
   and may be an empty but may not be NULL.

   Returns 0 on success.

   @see s2_pf_dir_add_v()
*/
int s2_pf_dir_add( s2_pf * pf, char const * dir, cwal_size_t dirLen);

/**
   Adds a file suffix (extension) to pf's search path. ext must be at
   least extLen bytes and may be an empty but may not be NULL.

   Returns 0 on success.

   @see s2_pf_ext_add_v()
*/
int s2_pf_ext_add( s2_pf * pf, char const * ext, cwal_size_t extLen);

/**
   Variant of s2_pf_dir_add() which takes its directory part in the
   form of a cwal_value.

   Returns 0 on success.
*/
int s2_pf_dir_add_v( s2_pf * pf, cwal_value * v );

/**
   Variant of s2_pf_ext_add() which takes its directory part in the
   form of a cwal_value.

   Returns 0 on success.
*/
int s2_pf_ext_add_v( s2_pf * pf, cwal_value * v );

/**
   Replaces pf's directory list with the given one.

   Returns 0 on success.
*/
int s2_pf_dirs_set( s2_pf * pf, cwal_array * ar );

/**
   Replaces pf's extension/suffix list with the given one.

   Returns 0 on success.
*/
int s2_pf_exts_set( s2_pf * pf, cwal_array * ar );

/**
   Symbolic values for use with s2_pf_search()'s final
   parameter.
 */
enum s2_pf_search_policy {
/**
   Indicates that ONLY directory names will be considered as matches.
*/
S2_PF_SEARCH_DIRS = -1,
/**
   Indicates that ONLY file (not directory) names will be considered
   as matches.
*/
S2_PF_SEARCH_FILES = 0,
/**
   Indicates that both file and directory names will be considered as
   matches.
*/
S2_PF_SEARCH_FILES_DIRS = 1
};

/**
   Searches for a file whose name can be constructed by some
   combination of pf's directory/suffix list and the given base name.

   The 5th argument specificies whether searching is allowed to match
   directory names or not. A value of 0 means only files (not
   directories) will be considered for matching purposes. A value
   greater than zero means both files and directories may be
   considered for matching purposes. A value less than zero means only
   directories (not files) may be considered a match.  (See the
   s2_pf_search_policy enum for symbolic names for this policy.)

   BUG: the directory policy does not currently work on non-Unix
   platforms because we don't have the code to check if a file name is
   a directory for such platforms (patches are welcomed!).

   Returns NULL if !pf, !base, !*base, !baseLen, or on allocation
   error (it uses/recycles a buffer to hold its path combinations).

   On success it returns a pointer to the (NUL-terminaed) path under
   which it found the item and rcLen (if not NULL) will be set to the
   length of the returned string. The bytes of the returned string are
   only valid until the next operation on pf, so copy them if you need
   them.

   If no match is found, rcLen is not modified.

   By default the host platform's customary path separator is used to
   separate directory/file parts ('\\' on Windows and '/' everywhere
   else). To change this, set the "separator" property of pf to a
   string value (even an empty one, in which case the directory paths
   added to pf should have the trailing separator added to them in
   order for searching to work).

   Pedantic sidebar: if the search path is empty, a match can still be
   found if the base name by itself, or in combination with one of the
   configured extensions, matches an allowed type of filesystem entry
   (as designated via the final argument).

   @see s2_pf_search_policy
*/
char const * s2_pf_search( s2_pf * pf, char const * base,
                           cwal_size_t baseLen, cwal_size_t * rcLen,
                           int directoryPolicy);


/**
   Returns pf's list of directories, creating it if needed. Only
   returns NULL if !pf or on allocation error.

   In script space this value is available via the "prefix" property.
*/
cwal_array * s2_pf_dirs(s2_pf *pf);

/**
   Returns pf's list of extensions/suffixes, creating it if needed. Only returns
   NULL if !pf or on allocation error.

   In script space this value is available via the "suffix" property.
*/
cwal_array * s2_pf_exts(s2_pf *pf);

/**
   A cwal_callback_f() implementing a constructor of PathFinder (s2_pf)
   instances. On success, assigns the new instance to *rv.

   Requires that s2_engine_from_args() returns non-NULL.

   Script usage:

   var pf = ThisFunction()

   it optionally takes up to two array arguments for the
   directory/extension lists, respectively.

   This is a holdover from before the 'new' keyword was added.
*/
int s2_cb_pf_new( cwal_callback_args const * args, cwal_value **rv );

/**
   Installs an Object named PathFinder (the s2_prototype_pf() object)
   into the given value (which must be a container type). Returns 0 on
   success, CWAL_RC_MISUSE if !se or !ns, CWAL_RC_TYPE if ns is not a
   container, and CWAL_RC_OOM if allocating any component fails.
*/
int s2_install_pf( s2_engine * se, cwal_value * ns );

/**
   Returns the prototype object for PathFinder instances. That
   instance gets stashed away in se. Ownership of the returned pointer
   is unchanged.  The caller MUST NOT unreference it
   (cwal_value_unref() or cwal_value_unhand()) unless he explicitly
   obtains a reference.
*/
cwal_value * s2_prototype_pf(s2_engine *se);

/**
   Callback signature for s2 module import routines.

   When called by s2_module_load(), s2_module_init(), or similar, this
   function type is passed the associated s2_engine instance and the
   client-provided module result value address.

   Implementations "should" (by convention) return their module by
   assigning it to *module. Optionally, they may use the s2_engine's
   facilities to store the functionality long-term (in terms of value
   lifetimes), e.g. using s2_stash_set(), or even forcing them into
   the top-most scope. In any case, they should, on success, assign
   some result value to *module, even if it's NULL. Note, however,
   that NULL is not generally a useful result value. Most modules
   return a "namespace object" which contains the module's
   functionality.

   When assigning to *module, the API expects that this function will
   not hold any extraneous references to the returned value. i.e. if
   it's a new Value with no circular references, its refcount "should"
   be zero when *module is assigned to and 0 is returned. The numerous
   sample modules provide examples of how to do this properly.

   @see s2_module_load()
*/
typedef int (*s2_module_init_f)( s2_engine * se, cwal_value ** module );

/**
   Holds information for mapping a s2_module_init_f to a name.
   Its purpose is to get installed by the S2_MODULE_xxx family of
   macros and referenced later via a module-loading mechanism.
*/
struct s2_loadable_module{
  /**
     Symbolic name of the module.
  */
  char const * name;

  /**
     The initialization routine for the module.
  */
  s2_module_init_f init;
};

/** Convenience typedef. */
typedef struct s2_loadable_module s2_loadable_module;

/**
   If compiled without S2_ENABLE_MODULES then this function always
   returns CWAL_RC_UNSUPPORTED and updates the error state of its
   first argument with information about that code.

   Its first argument is the controlling s2_engine.

   Its second argument is the name of a DLL file.

   Its third argument is the name of a symbol in the given DLL which
   resolves to a s2_loadable_module pointer. It may be NULL, in which
   case a default symbol name is used (which is only useful when
   plugins are built one per DLL).

   The final parameter is the target for the module's result value,
   and it may not be NULL (but the value it points to should initially
   be NULL, as it will be overwritten). It is passed directly to the
   module's s2_loadable_module::init() function, which is responsible
   for (on success) assigning *mod to the value the module wants to
   return.

   This function tries to open a DLL named fname using the system's
   DLL loader. If none is found, CWAL_RC_NOT_FOUND is returned and the
   s2_engine's error state is populated with info about the error. If
   one is found, it looks for a symbol in the DLL: if symName is not
   NULL and is not empty then the symbol "s2_module_symName" is
   sought, else "s2_module". (e.g. if symName is "foo" then it
   searches for a symbol names "s2_module_foo".) If no such symbol is
   found then CWAL_RC_NOT_FOUND (again) is returned and the
   s2_engine's error state is populated, else the symbol is assumed to
   be a (s2_loadable_module*), its init() function is called, and its
   result is returned to the caller of this function.

   On error, this routine generally updates the s2_engine's error
   state with more info (e.g. the name of the symbol on a symbol
   lookup failure). Not all errors update the engine's error state,
   only those with more information to convey than the result code. On
   error, the result Value (final parameter) is not modified.

   Returns 0 on success.

   Note that the API provides no mechanism for unloading DLLs because
   it is not generically possible to know if it is safe to do
   so. Closing a DLL whose resources (e.g. a native class definition
   for a client-bound type) are still in use leads, of course, to
   undefined results. The caveat, however, is that because dlopen()
   and friends allocate memory when we open DLLs, and we don't close
   them, valgrind reports this (rightfully) as a leak. It is not so
   much a leak as it is a required safety net. That said, the
   interpreter will close all DLLs it opened (or believes it opened)
   when it is finalized. That, however, opens up another potential
   problem: interpreters will close a DLL one time for each time they
   opened it. How the underlying (system-level) module API deals with
   that is up to that API. The dlopen()-based and lt_dlopen()-based
   implementations are safe in that regard (at least on Linux,
   according to their man pages and a peek at their sources).

   In practice this routine is not called by client C code, but is
   instead called indirectly via s2_cb_module_load(), which is a
   script-side binding of this function.

   @see s2_cb_module_load()
   @see S2_MODULE_DECL
   @see S2_MODULE_IMPL
   @see S2_MODULE_REGISTER
   @see S2_MODULE_REGISTER_
*/
int s2_module_load( s2_engine * se, char const * fname,
                    char const * symName, cwal_value ** mod );

/**
   Behaves similarly to s2_module_load(), and its first 3 parameters
   are used as documented for that function, but this variant does not
   invoke the init() method of the module before returning that module
   via *mod.

   On success *mod is set to the module object. Its ownship is kinda
   murky: it lives in memory made available via the module loader. It
   remains valid memory until the DLL is closed.

   Returns 0 on success. On error, se's error state may contain more
   information.

   After calling this, the next call would typically be
   s2_module_init().

   @see s2_module_load()
   @see s2_module_init()
*/
int s2_module_extract( s2_engine * se,
                       char const * dllFileName,
                       char const * symName,
                       s2_loadable_module const ** mod );

/**
   This function pushes a new cwal scope, calls mod->init(), and
   propagates any result value from that routine back out of that new
   scope via *rv. On error *rv is not modified. If rv is NULL then the
   Value result of the module init is ignored (destroyed before this
   routine returns unless the module stores is somewhere long-lived),
   but any integer result code of the init routine is propagated back
   to the caller of this function.

   @see s2_module_load()
   @see s2_module_extract()
*/
int s2_module_init( s2_engine * se,
                    s2_loadable_module const * mod,
                    cwal_value ** rv);

/** @def S2_MODULE_DECL

   Declares an extern (s2_loadable_module*) symbol called
   s2_module_#\#NAME.

   Use S2_MODULE_IMPL to create the matching implementation
   code.
   
   This macro should be used in the C or H file for a loadable module.
   It may be compined in a file with a single S2_MODULE_IMPL1()
   declaration with the same name, such that the module can be loaded
   both with and without the explicit symbol name.

   @see S2_MODULE_IMPL

*/
#define S2_MODULE_DECL(NAME)                            \
    extern const s2_loadable_module * s2_module_##NAME

/** @def S2_MODULE_IMPL
   
   Intended to be used to implement module declarations.  If a module
   has both C and H files, S2_MODULE_DECL(NAME) should be used in the
   H file and S2_MODULE_IMPL() should be used in the C file. If the
   DLL has only a C file (or no public H file), S2_MODULE_DECL is
   unnecessary.

   Implements a static s2_loadable_module object named
   s2_module_#\#NAME#\#_impl and a non-static (s2_loadable_module*)
   named s2_module_#\#NAME which points to
   s2_module_#\#NAME#\#_impl. (The latter symbol may optionally be
   declared in a header file via S2_MODULE_DECL.)

   INIT_F must be a s2_module_init_f() function pointer. That function
   is called when s2_module_load() loads the module.

   This macro may be combined in a file with a single
   S2_MODULE_IMPL1() declaration using the same NAME value, such that
   the module can be loaded both with and without the explicit symbol
   name.

   Example usage, in a module's header file, if any:

   @code
   S2_MODULE_DECL(cpdo);
   @endcode

   (The declaration is not strictly necessary - it is more of a matter
   of documentation.)
   
   And in the C file:

   @code
   S2_MODULE_IMPL(cpdo,cpdo_module_init);
   @endcode

   If it will be the only module in the target DLL, one can also add
   this:
   
   @code
   S2_MODULE_IMPL1(cpdo,cpdoish_install_to_interp);
   // _OR_ (every so slightly different):
   S2_MODULE_STANDALONE(cpdo,cpdoish_install_to_interp);
   @endcode

   Which simplifies client-side module loading by allowing them to
   leave out the module name when loading, but that approach only
   works if modules are compiled one per DLL (as opposed to being
   packaged together in one DLL).
   
   @see S2_MODULE_DECL
   @see S2_MODULE_IMPL1
*/
#define S2_MODULE_IMPL(NAME,INIT_F)                                     \
  static const s2_loadable_module                                       \
  s2_module_##NAME##_impl = { #NAME, INIT_F };                          \
  const s2_loadable_module * s2_module_##NAME = &s2_module_##NAME##_impl


/** @def S2_MODULE_IMPL1

   Implements a static "v1-style" s2_loadable_module symbol called
   s2_module_impl and a non-static (s2_loadable_module*) named
   s2_module which points to s2_module_impl

   INIT_F must be a s2_module_init_f.
   
   This macro must only be used in the C file for a loadable module
   when that module is to be the only one in the resuling DLL. Do not
   use it when packaging multiple modules into one DLL: use
   S2_MODULE_IMPL for those cases (S2_MODULE_IMPL can also be used
   together with this macro).

   @see S2_MODULE_IMPL
   @see S2_MODULE_DECL
   @see S2_MODULE_STANDALONE_IMPL
*/
#define S2_MODULE_IMPL1(NAME,INIT_F)                                \
  static const s2_loadable_module                                   \
  s2_module_impl = { #NAME, INIT_F };                               \
  const s2_loadable_module * s2_module = &s2_module_impl

/** @def S2_MODULE_STANDALONE_IMPL

    S2_MODULE_STANDALONE_IMPL() works like S2_MODULE_IMPL1() but is
    only fully expanded if the preprocessor variable
    S2_MODULE_STANDALONE is defined (to any value).  If
    S2_MODULE_STANDALONE is not defined, this macro expands to a dummy
    placeholder which does nothing (but has to expand to something to
    avoid leaving a trailing semicolon in the C code, which upsets the
    compiler (the other alternative would be to not require a
    semicolon after the macro call, but that upsets emacs' sense of
    indentation)).

    This macro may be used in the same source file as S2_MODULE_IMPL.

    The intention is that DLLs prefer this option over
    S2_MODULE_IMPL1, to allow that the DLLs can be built as standalone
    DLLs, multi-plugin DLLs, and compiled directly into a project (in
    which case the code linking it in needs to resolve and call the
    s2_loadable_module entry for each built-in module).

   @see S2_MODULE_IMPL1
   @see S2_MODULE_REGISTER
*/
#if defined(S2_MODULE_STANDALONE)
#  define S2_MODULE_STANDALONE_IMPL(NAME,INIT_F) S2_MODULE_IMPL1(NAME,INIT_F)
#else
#  define S2_MODULE_STANDALONE_IMPL(NAME,INIT_F) \
  extern void _s2_module_dummy_does_not_exist_()
#endif

/**
   Performs all the necessary setup for a v2-style module, including
   declaration and definition. NAME is the name of the module. This is
   normally called immediately after defining the plugin's init func
   (which is passed as the 2nd argument to this macro).

   See S2_MODULE_IMPL() and S2_MODULE_STANDALONE_IMPL() for
   the fine details.
*/
#define S2_MODULE_REGISTER(NAME,INIT_F)  \
  S2_MODULE_IMPL(NAME,INIT_F);           \
  S2_MODULE_STANDALONE_IMPL(NAME,INIT_F)

/**
   Functionally equivalent to:
   S2_MODULE_REGISTER(NAME, s2_module_init_#\#NAME).
*/
#define S2_MODULE_REGISTER_(NAME)                             \
  S2_MODULE_IMPL(NAME,s2_module_init_##NAME);          \
  S2_MODULE_STANDALONE_IMPL(NAME,s2_module_init_##NAME)


/**
   cwal_callback_f() impl which wraps s2_module_load().

   Script-side usages:

   // For single-module DLLs:
   var module = loadModule("filename");
   // Or, for multi-module DLLs:
   var module = loadModule("filename", "symbolName");

   On success it returns the module's value (which can be anything),
   or the undefined value if the module returns nothing (which would be
   unusual).

   If passed no symbol name, it assumes that the DLL is a
   single-module DLL and uses the symbol name "s2_module". If passed a
   symbol name, it looks for "s2_module_SYMBOL_NAME". If such a symbol
   is found it is assumed to be a (s2_loadable_module const *) and its
   init() function is called.

   On success 0 is returned. On error it throws or returns a
   lower-level error code (e.g. CWAL_RC_OOM).

   Achtung: there is no module caching going on here, and loading a
   module multiple times may be expensive or confusing (the returned
   objects from separate calls will, unless the module itself somehow
   caches results, be different instances).

   Achtung: this binding requires that s2_engine_from_args() return
   non-0 (it will do so if args->engine is managed by an s2_engine
   instance).
*/
int s2_cb_module_load( cwal_callback_args const * args,
                       cwal_value **rv );

#if 0
/* The FFI API was removed because it's unmaintained: see
   mod/_attic/s2_ffi.c */

/**
   A cwal_callback_f() implementation which allows calls to
   near-arbitrary C functions using FFI (Foreign Function Interface:
   https://en.wikipedia.org/wiki/Foreign_function_interface).

   Achtung: this is probably the most dangerous thing that a scripting
   engine could ever be allowed to do, and is implemented purely for
   educational purposes.

   Script-side usage:

   ffiCall("symbolName"); // for void function with no args
   ffiCall("symbolName", returnType); // for functions with no args
   ffiCall("symbolName", [ returnType, argType... ], args...);
   // e.g.
   ffiCall("open", [ FFI_INT, FFI_PTR, FFI_INT], "/etc/fstab", 0);

   On success this calls the foreign function and returns its result.
   On error it throws and returns an error code.
*/
int s2_cb_ffi_exec( cwal_callback_args const * args, cwal_value **rv );
#endif

/**
   Pushes one level of output buffer into se's output buffer stack.
   Buffering works similarly to PHP's ob_start() (and friends) support.
   While a buffer is active, all output send to cwal_engine_output()
   and friends is redirected to a buffer. The various s2_ob_xxx()
   functions can be used to:

   - fetch or discard the contents
   - push a new buffer onto the stack
   - pop the buffer from the stack (discarding its contents)

   When the interpreter is shut down it automatically removes any
   pushed buffers, but clients should call s2_ob_pop() once
   for each time they call s2_ob_push()

   Returns 0 on success, CWAL_RC_MISUSE if !se, CWAL_RC_RANGE if there
   has been no corresponding call to s2_ob_push().

   Results of the whole s2_ob_XXX() API are undefined if another API
   manipulates the contents of the underlying cwal_engine's output
   redirection bits (i.e. cwal_engine_vtab::outputer via
   cwal_engine::vtab).
   
   @see s2_ob_pop()
   @see s2_ob_get()
   @see s2_ob_take()
   @see s2_ob_clear()
   @see s2_ob_level()
   @see s2_ob_flush()
*/
int s2_ob_push( s2_engine * se );

/**
   Attempts to reserve at least reserveBufSize bytes of memory for the
   current buffering level. This does not change the buffering level.

   Returns 0 on success, CWAL_RC_MISUSE if !se, CWAL_RC_RANGE if
   s2_ob_push() has not previously been called, and CWAL_RC_OOM if
   allocation of new memory fails.

   @see s2_ob_push()
 */
int s2_ob_reserve( s2_engine * se, cwal_size_t reserveBufSize );

/**
   Removes the current level of output buffer from ie.

   Returns 0 on success, CWAL_RC_MISUSE if !ie, CWAL_RC_RANGE if there
   has been no corresponding call to s2_ob_push().
*/
int s2_ob_pop( s2_engine * se );

/**
   Returns the current buffering level, or 0 if !ie or ie is
   not in buffering mode.

   @see s2_ob_push()
   @see s2_ob_pop()
*/
cwal_size_t s2_ob_level( s2_engine * se );

/**
   Gets a pointer to the raw buffer owned by the current level of
   output buffer, assigning it to *tgt. The buffer is owned by the OB
   layer and its contents may be modified on any API routines which
   end up calling cwal_engine_output() or the other s2_ob_xxx()
   APIs. The caller is intended to copy/use the buffer's contents
   immediately, and not hold on to it past the current operation.

   Returns 0 on success, CWAL_RC_MISUSE if !ie or !tgt, CWAL_RC_RANGE
   if there has been no corresponding call to s2_ob_push().
*/
int s2_ob_get( s2_engine * se, cwal_buffer ** tgt );

/**
   Like s2_ob_get(), but moves the contents of the current
   buffer layer into tgt, clearing the OB buffer but leaving
   it on the buffer stack for later use.

   Returns 0 on success, CWAL_RC_MISUSE if !se or !tgt, CWAL_RC_RANGE
   if there has been no corresponding call to s2_ob_push().

   tgt must be empty-initialized or the caller must call
   cwal_buffer_reserve(..., tgt, 0) before calling this or memory may
   leak. On success ownership of the memory in tgt->mem is transfered
   to the caller. If tgt was created via cwal_new_buffer() or
   cwal_new_buffer_value() then tgt and tgt->mem are owned by se->e.
*/
int s2_ob_take( s2_engine * se, cwal_buffer * tgt );

/**
   Clears the contents of the current buffering layer. If
   releaseBufferMem is true (non-0) then the buffer memory is
   deallocated, otherwise it is just reset for later use by the OB
   layer. If it is deallocated, it will be re-allocated later if more
   output is buffered.

   Returns 0 on success, CWAL_RC_MISUSE if !ie, CWAL_RC_RANGE
   if there has been no corresponding call to s2_ob_push().
*/
int s2_ob_clear( s2_engine * se, char releaseBufferMem );

/**
   Pushes the current contents of the output buffer layer to the next
   output destination in the stack and the current level is cleared of
   contents (but stays on the stack). If the next outputer is a buffer
   then the current buffer is appended to it, otherwise it is sent to
   the originally configured output destination.

   Returns 0 on success, CWAL_RC_MISUSE if !ie, CWAL_RC_RANGE
   if there has been no corresponding call to s2_ob_push(),
   and potentially some other error if flushing to the lower-level
   implementation fails.

   @see s2_ob_push()
   @see s2_ob_pop()
*/
int s2_ob_flush( s2_engine * se );


/**
   cwal_callback_f() impl wrapping s2_ob_push(). Requires that
   args->state be a (s2_engine*). Returns argv->self.

   Accepts an optional integer argument which specifies an amount of
   memory to pre-allocate for the buffer (see s2_ob_reserve()).

   On error this function returns with an unchanged buffer level.
*/
int s2_cb_ob_push( cwal_callback_args const * args, cwal_value **rv );

/**
   cwal_callback_f() impl wrapping s2_ob_pop(). Requires
   that args->state be a (s2_engine*).

   Script signature:

   @code
   mixed pop([int takePolicy=0])
   @endcode

   If passed no args or a 0/falsy value, it discards any buffered
   output. If passed numeric greater than 0 then it returns (via *rv)
   the content as a Buffer. If passed numeric negative then it returns
   the contents as a String.

*/
int s2_cb_ob_pop( cwal_callback_args const * args, cwal_value **rv );

/**
   cwal_callback_f() impl wrapping s2_ob_reserve(). Requires
   that args->state be a (s2_engine*). Returns argv->self.
*/
int s2_cb_ob_reserve( cwal_callback_args const * args, cwal_value **rv );

/**
   cwal_callback_f() impl wrapping s2_ob_get(). Requires
   that args->state be a (s2_engine*).

   Assigns *rv to the string contents of the buffer layer.
*/
int s2_cb_ob_get( cwal_callback_args const * args, cwal_value **rv );
/**
   cwal_callback_f() impl wrapping s2_ob_clear(). Requires
   that args->state be a (s2_engine*). Returns argv->self.
*/
int s2_cb_ob_clear( cwal_callback_args const * args, cwal_value **rv );

/**
   cwal_callback_f() impl wrapping s2_ob_take(). Requires
   that args->state be a (s2_engine*).

   Assigns *rv to the string contents of the buffer layer.

   Design note: the returned string is actually a z-string to avoid
   having to make another copy of the data.
*/
int s2_cb_ob_take_string( cwal_callback_args const * args, cwal_value **rv );

/**
   Functionally identical to s2_cb_ob_take_string() except that it
   returns (via *rv) a cwal_buffer value (owned by args->engine).
*/
int s2_cb_ob_take_buffer( cwal_callback_args const * args, cwal_value **rv );

/**
   cwal_callback_f() impl wrapping s2_ob_flush(). Requires
   that args->state be a (s2_engine*). Returns argv->self.
*/
int s2_cb_ob_flush( cwal_callback_args const * args, cwal_value **rv );    

/**
   cwal_callback_f() impl for...

   mixed capture(string|function callback
                 [, int captureMode=-1 | buffer captureTarget])

   Which does:

   1) Push an OB level.

   2) Runs the given callback. If it's a function, it is call()ed. If it
   is a string, it is eval'd. Any other type, including a buffer, triggers
   an error.

   3) If the 2nd argument is a buffer, all captured output is appended
   to that buffer and that buffer is returned. If it's not a buffer,
   it's interpreted as an integer with the same semantics as pop()'s
   argument but with a different default value: if it's negative (the
   default) then the captured buffered output is returned as a string,
   positive returns the result as a new buffer, and 0 means to simply
   discard the result.

   4) Pops its buffer.

   If the callback leaves the buffer stack count with fewer levels
   than than what were active when the callback was triggered, steps
   (3) and (4) are skipped and an exception is triggered. If it pushes
   extra levels, they are assumed to be part of the output: this
   function flushes and pops each one, and captures or discards the
   cumulative output.

   The main advantage to this approach to capturing output, over
   manually pushing and popping OB levels, is that this function keeps
   the levels in sync even in the face of an s2-level
   assert/exit/fatal call, cwal/s2 OOM condition, s2_interrupt(), or
   similar "flow-control event."
*/
int s2_cb_ob_capture( cwal_callback_args const * args, cwal_value **rv );

/**
   Installs the following functions into tgt (which must be a property
   container type), all of which correspond to a similarly named
   s2_ob_XXX() resp. s2_cb_ob_XXX() function:

   push(), pop(), getString(), takeString(), takeBuffer(), clear(),
   flush(), capture()

   Returns 0 on success. On error tgt might have been partially
   populated.

   Returns CWAL_RC_MISUSE if !ie or !tgt, CWAL_RC_TYPE if tgt
   is not a container type.
*/
int s2_install_ob( s2_engine * se, cwal_value * tgt );

 /**
   Variant of s2_install_ob() which installs the OB functionallity
   into a new object with the given name, and places that object in
   tgt. Returns 0 on success.
*/
int s2_install_ob_2( s2_engine * se, cwal_value * tgt,
                     char const * name );


 /**
   Installs various io-related APIs. If name is not NULL and not
   empty, then the APIs get installed in a new object named tgt[name],
   else the functions are installed directly in tgt (which must be a
   container type).

   Returns 0 on success.

   Functions installed by this:

   print(), output(), flush()
*/
int s2_install_io( s2_engine * se, cwal_value * tgt,
                   char const * name );

 /**
   Installs various filesystem-related APIs which are largely
   platform-dependent. If name is not NULL and not empty, then the
   APIs get installed in a new object named tgt[name], else the
   functions are installed directly in tgt (which must be a container
   type).

   Returns 0 on success.

   Functions installed by this:

   chdir(), getcwd(), mkdir(), realpath(), stat(), fileIsAccessible(),
   dirIsAccessible().

   All of these functions are affected by the s2_disable_get() flags
   S2_DISABLE_FS_STAT, and mkdir() is also affected by
   S2_DISABLE_FS_WRITE.
*/
int s2_install_fs( s2_engine * se, cwal_value * tgt,
                   char const * name );

/**
   A cwal_callback_f() which expects its first argument to be an
   integer. It tries to sleep for at least that many seconds and
   returns the number of seconds left to sleep if it is interrupted
   (as per sleep(3)).

   @see s2_install_time()
 */
int s2_cb_sleep(cwal_callback_args const * args, cwal_value ** rv);

/**
   A cwal_callback_f() which expects its first argument to be an
   integer. It sleeps for that many milliseconds. It throws an
   exception if usleep(3) fails or if the library is built without
   usleep(3) support. It returns the undefined value.

   @see s2_install_time()
*/
int s2_cb_mssleep(cwal_callback_args const * args, cwal_value ** rv);

/**
   Installs the following script-side functions:

   sleep(), mssleep(), time(), mstime(), strftime()

   If name is not 0 and *name is not 0 then a new property with that
   name, of type object, is installed to tgt, and the functions are
   stored there, otherwise the functions are installed directly into
   tgt. Returns 0 on success. On error non-0 is returned (a
   CWAL_RC_xxx value) and, depending on where the error happened, the
   module may or may not have been partially installed in the target
   object. The only "likely" (for a given definition of "likely")
   error from this function, assuming all arguments are valid, is
   CWAL_RC_OOM, indicating that it ran out of memory. Returns
   CWAL_RC_MISUSE if either the first or second argument are NULL, and
   CWAL_RC_TYPE if !cwal_props_can(tgt).

   @see s2_cb_sleep()
   @see s2_cb_mssleep()
   @see s2_cb_time()
   @see s2_cb_mstime()
   @see s2_cb_strftime()
*/
int s2_install_time( s2_engine * se, cwal_value * tgt, char const * name );



/**
   Flags for use with s2_tmpl_opt::flags.
*/
enum s2_tmpl_flags_e {
/**
   Indicates that the output function header definition
   which checks for and optionally defines the function
   TMPLOUT (used by the processed template to output
   its content) should be elided (i.e. not output).
 */
S2_TMPL_ELIDE_TMPLOUT = 0x01
};

typedef struct s2_tmpl_opt s2_tmpl_opt;
/**
   Holds options for the s2_tmpl_to_code() function.  Clients
   must initialize them by copying either s2_tmpl_opt_empty or
   (for const contexts) s2_tmpl_opt_empty_m.  */
struct s2_tmpl_opt {
    /**
       0 (for no flags) or a bitmask of values from
       the s2_tmpl_flags_e enum.
     */
    int flags;
    /**
       If this is not 0 and not empty then:

       (A) the flag S2_TMPL_ELIDE_TMPLOUT
       is implied

       (B) this specifies the script function name which will be
       called when the processed template is eval'd, to emit its
       output.

       If 0 then "TMPLOUT" is used and (A) does not apply.
    */
    char const * outputSymbolPublic;

    /**
       In the processed output, the outputSymbolPublic name is only
       used in the header and aliased to a shorter symbol (so that the
       output will, for non-trivial cases, be shorter). This member
       specifies the name it uses after initialization. If it is 0 or
       starts with a NUL byte then some unspecified (but short)
       default is used. It is recommended that clients (if they use
       this) use weird non-ASCII UTF8 character combinations to avoid
       any potential symbol collisions.

       If this is not NULL and has a length greater than 0 then this
       alias is used in place of the default, and is initialized as
       a scope-local variable in the header of the template if
       it has not previously been declared in that scope.

       The likely only reason this should be overridden is that
       1-in-a-gazillion chance that a template actually uses a symbol
       which collides with this one's default value.
    */
    char const * outputSymbolInternal;

    /**
       Similar to outputSymbolInternal, this specifies the name of the
       processed-template-internal heredoc delimiter. By default (if
       this is 0) some cryptic combination of non-ASCII UTF8 character
       is used.
     */
    char const * heredocId;

    /**
       The opening tag for "code" blocks. Default is "<?".
       If set, then tagCodeClose must also be set. Must differ
       from all other tag open/close entries.
     */
    char const * tagCodeOpen;

    /**
       The opening tag for "code" blocks. Default is "?>".
       If set, then tagCodeOpen must also be set. Must differ
       from all other tag open/close entries.
    */
    char const * tagCodeClose;

    /**
       The opening tag for "value" blocks. Default is "<%".
       If set, then tagValueClose must also be set. Must differ
       from all other tag open/close entries.
    */
    char const * tagValueOpen;

    /**
       The opening tag for "value" blocks. Default is "%>".
       If set, then tagValueOpen must also be set. Must differ
       from all other tag open/close entries.
    */
    char const * tagValueClose;
};

/**
   An initialized-with-defaults instance of s2_tmpl_opt,
   intended for const-copy initialization.
*/
#define s2_tmpl_opt_empty_m {0,0,0,0,0,0,0,0}

/**
   An initialized-with-defaults instance of s2_tmpl_opt,
   intended for copy initialization.
*/
extern const s2_tmpl_opt s2_tmpl_opt_empty;

/**
   Implements a very basic text template processing mechanism for
   th1ish.

   The e arg must be a valid cwal_engine instance.

   src must be the template source code to process. It is treated as
   nearly-opaque text input which may contain markup tags (described
   below) to embed either code blocks or values into the output.

   dest is where all output is appended (the buffer is not reset by
   this function).

   The opt parameter may be 0 (for default options) or an object which
   configures certain parts of the template processing, as described
   in the s2_tmpl_opt docs.

   Returns 0 on success, non-0 on error. Error codes include:

   - CWAL_RC_MISUSE if !e, !src, or !dest.

   - CWAL_RC_RANGE if any of the open/close tags specified in the opt
   parameter are invalid (empty strings or validate rules described in
   the s2_tmpl_opt docs).

   - CWAL_RC_OOM on allocation errors.

   - CWAL_RC_EXCEPTION if it is reporting an error via a cwal
   exception. On code generation errors it throws an exception in
   the context of e, containing information about the nature and
   location (in the original source) of the problem.

   That said, it may not catch many conceivable malformed content
   cases and in such cases may generate malformed (as in not
   eval'able) code.


   Template processing...

   (Note that while these docs use fixed tag names, the exact
   tags can be configured via the opt parameter.)

   The output starts with a document prefix which sets up output of
   the text parts of the page.

   All non-code parts of src are filtered to be output wrapped in
   individual HEREDOCs embedded in the output script. All code parts
   of src are handled as follows:

   '<?' (without the quotes) starts a code block, running until and
   closing '?>' tag. This ends any current HEREDOC and passes through
   the code as-is to dest.  It does not generate any output in the
   processed document unless the embedded code generates it.

   '<%' (without the quotes) starts a "value block," which is
   processed a little bit differently. The contents between that and
   the next '%>' tag are simply passed to the configured output
   routine (see below).

   An example input document should clear this up:

   @code
   Hi, world!
   <? var x = 1, y = 2 ?>
   x = <%x%>, y = <% y %>, x+y=<% x + y %>
   @endcode

   The generated code is an s2 script which, when run (via eval,
   scope, catch...), outputs a processed document. All non-script
   parts get wrapped in HEREDOCs for output.

   The generated code "should" evaluated in a scope of its own, but it
   can be run in the current scope if desired. The code relies on an
   output function being defined (resolvable in the evalution scope).
   That function, if not specified via the opt parameter, is called
   TMPLOUT. No name is specified and the symbol TMPLOUT is undefined
   (when the processed template is eval'd), it uses s2out as its
   default output function (prior to 20191210 it uses
   s2.io.output). The function must accept any number of Value type
   parameters and output them "in its conventional string form"
   (whatever that is). It must not perform any formatting such as
   spaces between the entries or newlines afterwards. It may define
   formatting conventions for values passed to it (e.g. it may feel
   free to reformat doubles to a common representation).

   The generator outputs some weird/cryptic UTF8 symbols as heredoc
   markers. It's conceivable, though very unlikely, that these could
   collide with symbols in the document for heredoc processing
   purposes.

   Whitespace handling:

   - If the script starts with <? or <%, any whitespace leading up to
   that are discarded, otherwise all leading whitespace is retained.

   - Replacement of <% %> and <? ?> blocks retains whitespace to the
   left of the openener and right of the closer, so {abc<%x%>def} will
   form a single output token (provided 'x' evaluates to such), where
   {abc <%x%> def} will generate three. Inside the <? ?> blocks, all
   whitespace is retained. Inside <% %> blocks, the contents are
   treated as if they were inside a normal HEREDOC, so their
   leading/trailing spaces are stripped BUT they are not significant -
   the _result_ of evaluating the <% %> content gets output when
   executed, not the content itself.

   TODOs:

   - a variant which takes a cwal_output_f() instead of a buffer.
*/
int s2_tmpl_to_code( cwal_engine * e, cwal_buffer const * src,
                     cwal_buffer * dest, s2_tmpl_opt const * opt );

/**
   A cwal_callback_f() binding for s2_tmpl_to_code(). It
   expects one string/buffer argument containing tmplish code and it
   returns a new Buffer value containing the processed code. Throws on
   error.

   Script usage:

   var compiled = thisFunction(templateSource [, optObject])

   If optObject is-a Object then the following properties may
   influence template processing:

   - valueOpen and valueClose specify the open/close tags for
   Value Blocks.

   - codeOpen and codeClose specify the open/close tags for
   Code Blocks.

   - outputSymbol sets the name of the symbol the generated tmpl()
   code will (when eval'd) use for output. It may be a compound
   symbol, e.g. "s2.io.output" or even a function call, e.g.
   "proc(){return s2.io.output}()" - anything which is legal as the
   right-hand side of an assignment is (syntactically) legal
   here. That assignment will be called once each time the resulting
   template script is eval'd.
*/
int s2_cb_tmpl_to_code( cwal_callback_args const * args, cwal_value ** rv );

/**
   A cwal_callback_f() impl binding the C-standard time(3).

   @see s2_install_time()
*/
int s2_cb_time( cwal_callback_args const * args, cwal_value **rv );

/**
   A cwal_callback_f() impl returning the current time in milliseconds
   since the start of the Unix epoch. This requires platform-specific
   calls and throws an exception, with code CWAL_RC_UNSUPPORTED, if
   built on a platform with the required API.

   @see s2_install_time()
*/
int s2_cb_mstime( cwal_callback_args const * args, cwal_value **rv );

/**
   A strftime() implementation.

   dest must be valid memory at least destLen bytes long. The result
   will be written there.

   fmt must contain the format string. See the file toys/strftime.s2
   (or strftime.c, if you're more into C) for the complete list of
   format specifiers and their descriptions.

   timeptr must be the time the caller wants to format.

   Returns 0 if any arguments are NULL.
   
   On success it returns the number of bytes written to dest, not
   counting the terminating NUL byte (which it also writes). It
   returns 0 on any error, and the client may need to distinguish
   between real errors and (destLen==0 or !*fmt), both of which could
   also look like errors.

   TODOs:

   - Refactor this to take a callback or a cwal_buffer, so that we can
   format arbitrarily long output.

   - Refactor it to return an integer error code.

   (i didn't write this implementation - it is derived from public domain
   sources dating back to the early 1990's.)

*/
cwal_midsize_t s2_strftime(char *dest, cwal_midsize_t destLen,
                           const char *format, const struct tm *timeptr);

    
/**
   A cwal_callback_f() which wraps s2_strftime().

   Script usage:

   @code
   var tm = time();
   var str = strftime("%Y-%m-%d %H:%M:%S", tm);
   @endcode

   The default time value, if no second argument is passed in or a
   negative value is passed in, is the current time.

   Note that this implementation has a limit on the length of the
   result string (because s2_strftime() works that way), and
   throws if that length is violated. It's suitable for "usual"
   time strings but not for formatting whole sentences.

   This function takes an optional boolean 3rd argument: if truthy,
   local time is used, else GMT is used (the default).
*/
int s2_cb_strftime( cwal_callback_args const * args, cwal_value **rv );

/**
   Tries to convert an errno value to an equivalent CWAL_RC_xxx value.
   First argument must be the current errno value. Returns an
   equivalent, or dflt is no sematic equivalent is known. If errNo is
   0 then this function evaluates the global errno in its place. If
   both are 0 then this function returns CWAL_RC_OK.
*/
int s2_errno_to_cwal_rc(int errNo, int dflt);

/**
   A cwal_callback_f() impl when behaves like rand(3). It will call
   srand(3), with some pseudo-random seed, the first time it is
   called.

*/
int s2_cb_rand_int( cwal_callback_args const * args, cwal_value **rv );


/**
   cwal_callback_f() implementing fork(). On non-Unix builds this function
   triggers an exception.

   Script usage:

   fork(Function)
   fork(bool,Function)

   The parent process returns from that call. The child process runs
   the given Function and then exits the interpreter as if the 'exit'
   keyword had been used. If the first arg is a boolean true then then
   child process returns (in its own process) instead of exiting, passing
   back the result of the 2nd parameter (the callback function).
*/
int s2_cb_fork( cwal_callback_args const * args, cwal_value **rv );

/**
   A cwal_callback_f() implementation which returns a hashtable value
   which maps all of the cwal/s2-defined result code integers
   (CWAL_RC_xxx and S2_RC_xxx) to strings in the same form as the
   corresponding enum entry. e.g. it maps 0 to "CWAL_RC_OK". It also
   holds the reverse mappings, so "CWAL_RC_OK" ==> 0.

   This function creates the hash the first time it is called and
   caches its result in the s2_engine which owns args->engine, thus
   the second and subsequent calls will return the same value. 
*/
int s2_cb_rc_hash( cwal_callback_args const * args, cwal_value **rv );

/**
   A cwal_callback_f() implementation for formatting "this"
   using cwal_buffer_format (resp. s2.Buffer.appendf()).

   Requires args->argv[0] to be part of a formatting string for
   s2.Buffer.appendf(), namely the part after "$1%". It prepends "$1%"
   to the arg string and passes that string and the argument to
   cwal_buffer_format() to generate the result string.

   Triggers an exception if cwal_buffer_format() returns an error.
*/
int s2_cb_format_self_using_arg( cwal_callback_args const * args, cwal_value **rv );

/**
   A callback which creates and returns a new "unique" value, as per
   cwal_new_unique(). If args->argc then args->argv[0] is passed to
   cwal_new_unique().
*/
int s2_cb_new_unique( cwal_callback_args const * args, cwal_value **rv );

/**
   Experimental!

   Intended to be called by app-level code and be passed its
   shared/global s2_engine instance. This function uses global
   state and is NOT thread-safe in any way, shape, or form. It
   stores a copy of se.

   If se is not NULL then this installs a SIGINT handler which behaves
   like s2_interrupt() (but uses a different message string). If se is
   NULL then it removes any previous binding (it does not remove its
   SIGINT handler, but the handler becomes a no-op if no engine is set
   to be interrupted).
*/
void s2_set_interrupt_handlable( s2_engine * se );

/**
   This sets se's error state to CWAL_RC_INTERRUPTED, a flag it checks
   for at various points during evaluation and which causes the
   interpretter to behave essentially as if 'exit' had been used (but
   without a result value). Calling this is not a guaranty that the
   engine will stop processing its current script - there are corner
   cases where the flag can get "lost" during evaluation.

   On success, returns CWAL_RC_INTERRUPTED. On error (an allocation
   error generating a message string), it will return another non-0
   code (likely CWAL_RC_OOM).

   This is not strictly thread safe, but "should" be okay to call from
   a separate thread (e.g. a UI) in most cases, though (depending on
   timing) it might not have an effect. Known potential race
   conditions include, but are not necessarily limited to:

   - se is clearing its error state (which will clear the
     is-interrupted flag). It resets its error state internally for
     "non-error errors" which propagate a ways, like "return" and
     "throw", but those keywords "should" catch and propagate this
     condition, trumping their own. The lowest-level eval handler
     checks at every sensible opportunity.

   - se is cleaning up (inside s2_engine_finalize()), in which case
     accessing it might (depending on the timing)lead to an illegal
     memory access (if dynamically allocated) or a useless but
     harmless[1] access if it's stack-allocated. [1]=so long as the
     memory itself is still legal to access (e.g. app-level/static).

   - Third-party bindings may clear se's error state (which includes
     this flag) indescriminately, without being aware of this
     condition.


   There are likely others.
*/
int s2_interrupt( s2_engine * se );

/**
   Tries to parse a given C-string as an integer or double value.

   srcLen must be the string length of str or a negative value
   (in which case cwal_strlen() is used to count the length).

   If the string can be parsed to an integer or double value, *rv is
   set to the newly-created value and 0 is returned.  If no conversion
   can be made, *rv is set to 0 and 0 is returned.  On OOM error,
   CWAL_RC_OOM is returned.
*/
int s2_cstr_parse_number( cwal_engine * e, char const * str,
                          cwal_int_t srcLen, cwal_value ** rv );

/**
   The reverse of s2_rc_cstr(), this function tries to find a
   CWAL_RC_xxx or S2_RC_xxx error code for a string form of an enum
   entry's name, e.g. "CWAL_RC_OOM". On success it returns the code
   value via *code and returns non-0 (true), else it does not modify
   *code and returns 0 (false).

   This is currently an O(1) operation, performing one hash
   calculation and (at most) one string comparison.
*/
char s2_cstr_to_rc(char const *str, cwal_int_t len, int * code);

/**
   Rescopes v (if necessary per the scoping rules) to one scope
   up from the current cwal scope. This is almost never what you
   want to do.
*/
void s2_value_upscope( s2_engine * se, cwal_value * v );

/**
   If cwal_value_is_unique() is true for v then this returns the value
   of passing v to cwal_unique_wrapped_get(), else it returns v.

   In s2, the cwal_value_is_unique() type is the type used for enum
   entries.

   @see s2_value_cstr()
*/
cwal_value * s2_value_unwrap( cwal_value * v );

/**
   Const-friendly brother of s2_value_unwrap().
*/
cwal_value const * s2_value_unwrap_c( cwal_value const * v );

/**
   Works like cwal_value_cstr() unless cwal_value_is_unique(v) is
   true, in which case it uses v's wrapped value instead of v
   itself. In s2, "unique" values are most often enum entries.

   @see s2_value_unwrap()
*/
char const * s2_value_cstr( cwal_value const * v, cwal_size_t * len );

/**
   A helper for callbacks which honor the do-not-set-properties
   flag on container values.

   If v is tagged with the flag CWAL_CONTAINER_DISALLOW_PROP_SET then
   this function triggers a CWAL_RC_DISALLOW_PROP_SET exception and
   returns non-0 (intended to be propagated back out of the
   callback). If it does not have that flag, 0 is returned.

   If throwIt is true (non-0), the error is transformed to an exception,
   otherwise it is set as a non-exception error.
*/
int s2_immutable_container_check( s2_engine * se, cwal_value const * v, int throwIt );

/**
   Functionally identical to s2_immutable_container_check() but extracts the
   s2_engine from s2_engine_from_args(args) and always passes true
   as the 3rd argument to s2_immutable_container_check().
*/
int s2_immutable_container_check_cb( cwal_callback_args const * args, cwal_value const * v );

/**
   Configures various s2- and cwal-level flags for the given
   container-type value. Returns 0 if it sets/clears the flag(s),
   CWAL_RC_TYPE if v is not a container (not counting prototypes).

   The options are:

   allowPropSet: if true (the default) then the container mutation
   APIs work as normal, otherwise those which set properties will
   fail with a CWAL_RC_DISALLOW_PROP_SET error code.

   allowNewProps: if true (the default) new properties are created
   normally via the various setter operations. If false, trying to set
   a non-existing property will fail with a
   CWAL_RC_DISALLOW_NEW_PROPERTIES error code.

   allowGetUnknownProps: if true (the default) then unknown properties
   resolve to the undefined value or NULL (depending on the
   context). If false, s2_get_v() and friends will trigger a
   CWAL_RC_NOT_FOUND error for unknown properties. Note that
   properties which resolve through a prototype are still "known" for
   this purpose (they must be, or inheritance of methods could
   not work).
*/
int s2_container_config( cwal_value * v, char allowPropSet,
                         char allowNewProps,
                         char allowGetUnknownProps );

/**
   Experimenting with with ideas for things we "could" use
   per(-container-type)-Value flag bits (16 of them) in s2.  cwal
   gives us 16 bits per Container Value instance (not POD types, as we
   can't tag those further without increasing their sizeofs, dropping
   the built-in constants, or splitting the values into two
   allocations, such that we could do a CoW of the built-ins if they
   get flagged). See cwal_container_client_flags_get() and
   cwal_container_client_flags_set().
*/
enum s2_just_thinking_out_loud {

/**
   An alternate encoding might be to have the high (say) 4 (or 8) bits
   control the interpretation of the (say) bottom 12. e.g.

   MODE_CLASS = 0x1000,
   F_CLASS_CONST = MODE_CLASS | 0x01,
   F_CLASS_STATIC = MODE_CLASS | 0x02
   MODE_FUNCTION = 0x2000,
   F_FUNC_CTOR = MODE_FUNCTION | 0x01,
   ...

   Except that each value could have at most 1 mode. Unless we split
   into multiple mode groups:

   MODE_4_MASK = 0x8F00,
   MODE_3_MASK = 0x40F0,
   MODE_2_MASK = 0x200F,
   MODE_1_MASK = 0x1FFF,

   i.e. 3 modes, each with 4 bits, and a fallback mode with 12. Except
   that that gains us nothing (or very little).
*/

/**
  4 mutually exclusive modes, each with the bottom 8 bits reserved for
  itself.
*/
S2_VAL_F_MODE_MASK  = 0xF000U,
S2_VAL_F_MODE_FUNC  = 0x1000,
S2_VAL_F_MODE_CLASS = 0x2000,
S2_VAL_F_MODE_DOT = 0x4000,
S2_VAL_F_MODE_4 = 0x8000,
/**
   Bits (9-12) are "shared" (independent of the mode).
*/
S2_VAL_F_COMMON_MASK = 0x0F00,

/**
   Functions with this tag would get special handling when called by
   the 'new' keyword.
*/
S2_VAL_F_FUNC_CTOR  = S2_VAL_F_MODE_FUNC | 0x01,

/**
   For potential use in creating property interceptors.
*/
S2_VAL_F_FUNC_GETTER  = S2_VAL_F_MODE_FUNC | 0x02,
S2_VAL_F_FUNC_SETTER  = S2_VAL_F_MODE_FUNC | 0x04,
S2_VAL_F_FUNC_INTERCEPTOR  = S2_VAL_F_FUNC_SETTER | S2_VAL_F_FUNC_GETTER,

/**
   Objects created via the 'class' keyword. Get special
   property lookup treatment.
*/
S2_VAL_F_CLASS = S2_VAL_F_MODE_CLASS | 0x01,
/* Problem with some words, like const, is that we can only tag container types.
   So we'd need to use property-level constness. */
S2_VAL_F_CLASS_CONST = S2_VAL_F_MODE_CLASS | 0x02,
S2_VAL_F_CLASS_STATIC = S2_VAL_F_MODE_CLASS | 0x04,
/* It's unlikely that s2's engine can currently support the concept of
   private/protected vars/properties, but for the sake of bitmask
   planning... */
S2_VAL_F_CLASS_PRIVATE = S2_VAL_F_MODE_CLASS | 0x08,  /* or PUBLIC, if we default to private and can enforce it */
S2_VAL_F_CLASS_PROTECTED = S2_VAL_F_MODE_CLASS | 0x10,

/**
   Objects with this tag (created by passing a S2_VAL_F_CLASS-tagged
   Object to the 'new' keyword) might get special property lookup
   (and assignment) semantics, e.g.  limit them to class-defined
   properties.
*/
S2_VAL_F_CLASS_INSTANCE = S2_VAL_F_MODE_CLASS | 0x20,
/**
   Indicates the container is an enum.
*/
S2_VAL_F_CLASS_ENUM     = S2_VAL_F_MODE_CLASS | 0x40,
/**
   Hmm. The enum entry type (cwal type "unique") cannot have
   flags, so this is apparently unusued.
*/
S2_VAL_F_CLASS_ENUM_ENTRY  = S2_VAL_F_MODE_CLASS | 0x80,

/**
   This client container flag (cwal_container_client_flags_get() and
   friends)) causes s2_get_v() and friends to trigger an exception if
   a property request on a container with this flag does not find an
   entry.

   s2_set_v() does this differently to avoid having to do duplicate
   lookups. This feature was added to the core as the
   CWAL_CONTAINER_DISALLOW_NEW_PROPERTIES container flag and
   the CWAL_RC_DISALLOW_NEW_PROPERTIES result code.
*/
S2_VAL_F_DISALLOW_UNKNOWN_PROPS = S2_VAL_F_COMMON_MASK &  0x100,

/**
   This client container flag (cwal_container_client_flags_get() and
   friends) gets temporarily set on values created by the 'new'
   keyword, for the duration of the constructor call.
*/
S2_VAL_F_IS_NEWING = S2_VAL_F_COMMON_MASK & 0x0200,

/**
   Denotes an enum instance. May be a hash or an object.
*/
S2_VAL_F_ENUM = S2_VAL_F_CLASS_ENUM | S2_VAL_F_DISALLOW_UNKNOWN_PROPS,

/**
   Means something like: this Value should be treated like an
   Object for most purposes.

   We currently (experimentally, 20141202) use this on certain Hash
   Table instances. Its interpretation is that the dot op should use
   the Value's hash entries instead of its object properties. We
   somehow need to accommodate inherited methods, though. So we first
   check the hashtable, then object-level properties, then up the
   prototype chain. Any prototypes with this flag would of course
   apply the same logic.
*/
S2_VAL_F_DOT_LIKE_OBJECT = S2_VAL_F_MODE_DOT | 0x002,
/* S2_VAL_F_DOT_ARROW_LIKE_DOT = S2_VAL_F_MODE_DOT | 0x004, */

/**
   A mask of all bits, just as a reminder that we'd be limited to 16
   bits (because cwal has no more space to spare without increasing
   the sizeof() for all properties-capable Values).
*/
S2_VAL_F_MASK = 0xFFFF
};

/**
   Utility type for use with s2_install_functions(), allowing simple
   installation of a series of callbacks in one go.
*/
struct s2_func_def {
  char const * name;
  cwal_callback_f callback;
  void * state;
  cwal_finalizer_f stateDtor;
  void const * stateTypeID;
  /**
     A mask of S2_VAL_F_xxx vars
  */
  uint16_t cwalContainerFlags;
};
#define S2_FUNC6(NAME,CALLBACK,STATE,StateDtor,StateTypeId,CwalContainerFlags)  \
  {NAME,CALLBACK,STATE,StateDtor,StateTypeId, CwalContainerFlags}
/**
   Convenience macro for initializing an s2_func_def entry.
*/
#define S2_FUNC5(NAME,CALLBACK,STATE,StateDtor,StateTypeId) \
  S2_FUNC6(NAME,CALLBACK,STATE,StateDtor,StateTypeId, 0)
/**
   Convenience macro for initializing an s2_func_def entry.
*/
#define S2_FUNC2(NAME,CALLBACK) S2_FUNC5(NAME,CALLBACK,0,0,0)
/**
   EXPERIMENTAL: don't use.

   Convenience macro for initializing an s2_func_def entry.
*/
#define S2_FUNC2_INTERCEPTOR(NAME,CALLBACK) S2_FUNC6(NAME,CALLBACK,0,0,0,S2_VAL_F_FUNC_INTERCEPTOR)

/**
   Empty-initialized const s2_func_def struct.
*/
#define s2_func_def_empty_m {0,0,0,0,0,0}
/** Convenience typedef. */
typedef struct s2_func_def s2_func_def;

/**
   Installs a list of cwal callback functions into the given target
   container value. defs must be an array of s2_func_def objects
   terminated by an entry with a NULL name field (most simply, use
   s2_func_def_empty_m to intialize the final element). All member
   pointers in each entry must be valid, and 0 is (generally speaking)
   valid for all but the name and callback fields.

   If propertyFlags is not 0 then each property gets set with those
   flags, as per cwal_prop_set_with_flags().

   If tgt is NULL then the functions are installed into the _current_
   cwal scope. Note that outside of initialization of the engine, the
   current scope is very likely not the top-most, and may well
   disappear soon (e.g. using this from within a cwal_callback_f()
   implementation will only install these for the duration of the
   current function call!). (Potential TODO: add
   s2_install_functions_in_scope(), which takes a scope pointer)

   Returns 0 on success, a non-0 CWAL_RC_xxx code on error.

   Example:

   @code
   const s2_func_def funcs[] = {
     S2_FUNC2("myFunc1", my_callback_1),
     S2_FUNC2("myFunc2", my_callback_2),
     s2_func_def_empty_m // IMPORTANT that the list end with this!
   };
   int rc = s2_install_functions(se, myObj, funcs, CWAL_VAR_F_CONST);
   @endcode
*/
int s2_install_functions( s2_engine *se, cwal_value * tgt,
                          s2_func_def const * defs,
                          uint16_t propertyFlags );
/**
   A convenience function to install value v into the tgt object, or
   into the current scope if tgt is NULL. If nameLen is <0 then
   cwal_strlen() is used to calculate its length. propertyFlags may be
   0 or a mask of CWAL_VAR_F_xxx flags.

   Returns 0 on success or a CWAL_RC_xxx code on error.
 */
int s2_install_value( s2_engine *se, cwal_value * tgt,
                      cwal_value * v,
                      char const * name,
                      int nameLen,
                      uint16_t propertyFlags );

/**
   A (somewhat) convenience form of s2_install_value() which passes
   the (callback, state, stateDtor, stateTypeID) flags to
   cwal_new_function() and installs the resulting function int the tgt
   container value (or the current scope if tgt is NULL).

   If nameLen is <0 then cwal_strlen() is used to calculate its
   length. propertyFlags may be 0 or a mask of CWAL_VAR_F_xxx flags.

   Returns 0 on success or a CWAL_RC_xxx code on error.
 */
int s2_install_callback( s2_engine *se, cwal_value * tgt,
                         cwal_callback_f callback,
                         char const * name,
                         int nameLen, 
                         uint16_t propertyFags,
                         void * state,
                         cwal_finalizer_f stateDtor,
                         void const * stateTypeID );

/**
   A utility class for creating s2 script-side enums from C.

   When declaring/allocating these, be sure to ensure a sane default
   state by copying the global s2_enum_builder_empty object. e.g.

   @code
   s2_enum_builder eb = s2_enum_builder_empty;
   @endcode

   @see s2_enum_builder_init()
   @see s2_enum_builder_append()
   @see s2_enum_builder_seal()
   @see s2_enum_builder_cleanup()
*/
struct s2_enum_builder {
  /**
     The owning/managing s2_engine instance.
  */
  s2_engine * se;
  /**
     Internal flags.
  */
  cwal_flags16_t flags;
  /**
     Current entry count. This only counts the "primary" mappings
     (entry name to entry value), not the reverse mappings (value to
     name).
  */
  cwal_size_t entryCount;
  /**
     The underlying storage for the enum entries. As of 2020-02-21,
     this is a hash (it was previously an object or hash, depending on
     how many entries it held, but that proved to be more trouble than
     it was worth in downstream code).

     The state of this value is in flux until s2_enum_builder_seal()
     is used to "seal" it.
  */
  cwal_value * entries;
};
/** Convenience typedef. */
typedef struct s2_enum_builder s2_enum_builder;

/**
   An initialized-with-defaults s2_enum_builder instance, intended to
   be copied from when initializing instances on the stack.
*/
extern const s2_enum_builder s2_enum_builder_empty;

/**
   Initializes an s2_enum_builder instance, which MUST have been
   initially initialized via copy-construction from
   s2_enum_builder_empty (or results are undefined).

   entryCountHint is a hint to the system about how many entries are
   to be expected. This allows it to size its storage appropriately.
   If passed 0, it makes a conservative estimate. This routine chooses
   a prime-number hash table size based on the hint, so the hint
   itself need not be prime. (In any case, the storage may be resized
   when s2_enum_builder_seal() is used to finalize the construction
   process.)

   If typeName is not NULL and has a non-0 length then it will be used
   as the generated enum's typename value. If not NULL, typeName MUST
   be NUL-terminated.

   If this function returns 0, the s2_enum_builder instance must
   eventually be passed to s2_enum_builder_seal() or
   s2_enum_builder_cleanup() to free any resources it owns.

   Returns 0 on success and CWAL_RC_OOM if any allocation fails. On
   error, eb gets cleaned up if needed.

   A reference is added to eb->entries, and it is made vacuum-proof,
   until eb is cleaned up or sealed. HOWEVER...

   ACHTUNG:

   1) The builder class is intended primarily to be used during setup
   of a module or "atomically", e.g. in a single call to a
   cwal_callback_f() binding. Specifically, it is not intended to be
   run concurrently with script code due to...

   2) Lifetime considerations: during/after initialization, the
   under-construction enum value is initially managed by the cwal
   scope which is active at the time the enum is initialized (via this
   function). If that scope is popped before the enum is sealed or
   cleaned up, the enum's underlying storage (eb->entries) will be
   destroyed during the finalization of that scope and will leave
   eb->entries pointing to a stale pointer. cwal's recycling
   mechanisms may re-use that pointer soon afterwards, thus
   eb->entries could point to repurposed memory. i.e. Undefined
   Behaviour. Short version: create and seal your enum in the lifetime
   of the cwal/s2 scope it is initialized in unless you want to manage
   eb->entries' scope manually (tip: you _don't_ want to do that).

   3) 2020-02-21: the order of the last two arguments was swapped
   solely to force client-side breakage, rather than having the change
   of semantics for the entryCountHint parameter go unnoticed.

   @see s2_enum_builder_cleanup()
   @see s2_enum_builder_append()
   @see s2_enum_builder_seal()
*/
int s2_enum_builder_init( s2_engine * se, s2_enum_builder * eb,
                          char const * typeName,
                          cwal_size_t entryCountHint );

/**
   Every s2_enum_builder which gets passed to s2_enum_builder_init()
   must eventually be passed to this routine to free any memory owned
   by the builder. If the caller needs to keep eb->entries alive after
   this call, they must take a reference to it (see
   s2_enum_builder_seal()) and they may need to manually rescope it (a
   topic beyond the scope (as it were) of this function's
   documentation).

   It is a harmless no-op to call this multiple times on the same
   instance and it can safely be called after s2_enum_builder_seal()
   (which cleans up the enum in certain circumstances).

   @see s2_enum_builder_seal()
   @see s2_enum_builder_append()
   @see s2_enum_builder_init()
*/
void s2_enum_builder_cleanup( s2_enum_builder * eb );

/**
   Appends a new entry to an under-construction enum.

   eb must have been initialized using s2_enum_builder_init() and must
   not yet have been sealed via s2_enum_builder_seal().

   entryName is the NUL-terminated name of the enum entry. val is the
   optional value associated with the entry (it may be NULL).

   Returns 0 on success, CWAL_RC_OOM if any allocation fails,
   CWAL_RC_MISUSE if entryName is NULL or eb does not seem to be
   properly initialized or has already been sealed (see
   s2_enum_builder_seal()). On CWAL_RC_OOM, eb must be considered to
   be in an undefined state and must not be used further except to
   pass it to s2_enum_builder_cleanup().

   @see s2_enum_builder_seal()
   @see s2_enum_builder_cleanup()
   @see s2_enum_builder_init()
*/
int s2_enum_builder_append( s2_enum_builder * eb, char const * entryName,
                            cwal_value * val);

/**
   Works just like s2_enum_builder_append() but takes its key as a
   cwal_value rather than a c-string.
 */
int s2_enum_builder_append_v( s2_enum_builder * eb,
                              cwal_value * key,
                              cwal_value * wrappedVal );

/**
   "Seals" the given under-construction enum, such that it can no
   more entries can be added to it.

   The caller "should" pass a non-NULL rv value, in which case:

   1) *rv is assigned to the newly-created enum value. It has an
   initial refcount of 0, so the client needs to reference/unreference
   it (or equivalent).

   2) eb is cleaned up (see s2_enum_builder_cleanup()) to avoid
   further (mis)use. Despite this, it is safe to pass it to
   s2_enum_builder_cleanup(), so no special-case code branches
   are needed to accommodate this case.

   If rv is NULL then eb does not get cleaned up and the caller
   must eventually:

   1) Take a reference to eb->entries, if needed.

   2) Pass eb to s2_enum_builder_cleanup(). This will destroy eb->entries
   unless the caller has acquired a reference to it.

   The new enum would typically be stored in a cwal scope to keep it
   it alive, and it may be destroyed if it is not stored in a
   container or scope before its own managing scope is destroyed. (Its
   initial managing scope is the one which is active when the
   s2_enum_builder_init() is called.)

   Returns 0 on success. On error, CWAL_RC_RANGE if no entries have
   been added to the enum (via s2_enum_builder_append()), CWAL_RC_OOM
   on allocation error, and CWAL_RC_MISUSE if eb has not been properly
   initialized or has already been sealed. On error *rv is not
   modified. On error eb must be considered to be in an undefined
   state and must not be used further except to pass it to
   s2_enum_builder_cleanup().

   @see s2_enum_builder_append()
   @see s2_enum_builder_cleanup()
   @see s2_enum_builder_init()   
*/
int s2_enum_builder_seal( s2_enum_builder * eb, cwal_value **rv );

/**
   Toggles the S2_VAL_F_DOT_LIKE_OBJECT flag on the given value,
   which is assumed to be a hash table. The second parameter determines
   whether the flag is toggled on or off.
*/
void s2_hash_dot_like_object( cwal_value * hash, int dotLikeObj );

/**
   Returns true (non-0) if key's string value is the string "value".
*/
char s2_value_is_value_string( s2_engine const * se, cwal_value const * key );

/**
   Returns true (non-0) if key's string value is the string "prototype".
*/
char s2_value_is_prototype_string( s2_engine const * se, cwal_value const * key );


/**
   ACHTUNG: only lightly tested and known to break with certain
   script constructs.

   "Minifies" s2 source code in the given source, appending it to the
   given destination. This strips out all "junk" tokens and most
   newlines, as well as runs of contiguous spaces/tabs.

   This does not do any semantic analysis on the input - it only
   tokenizes the input. Unknown token types, unmatched
   braces/parens/quotes, etc. will cause it to fail.

   Returns 0 on success.

   There's not yet a guaranty that minified code can be eval'd... it's
   a learning process.

   src and dest _must_, on error, return a non-0 code from the
   CWAL_RC_xxx family of codes, e.g. CWAL_RC_IO might be appropriate
   (falling back to CWAL_RC_ERROR if there's no better match).
*/
int s2_minify_script( s2_engine * se, cwal_input_f src, void * srcState,
                      cwal_output_f dest, void * destState );
/**
   Equivalent to s2_minify_script(), using src as the input and dest
   as the output destination. src must not be dest. dest gets appended
   to, so be sure to cwal_buffer_reset() it, if needed, when looping
   over a single output buffer.
*/
int s2_minify_script_buffer( s2_engine * se, cwal_buffer const * src,
                             cwal_buffer * dest );

/**
   A cwal_callback_f() implementation binding s2_minify_script(). It
   expects either 1 or 2 arguments: (srcStringOrBuffer) or
   (srcStringOrBuffer, destBuffer). It returns, via *rv, its second
   argument or a new Buffer instance.
*/
int s2_cb_minify_script( cwal_callback_args const * args, cwal_value ** rv );

/**
   A convenience routine to bind a callback function as a constructor
   for use with s2's "new" keyword.
   
   Installs method as a hidden/const property named "__new" in the
   given container. The method parameter is assumed to conform to the
   "new" keyword's constructor conventions.

   This has the same effect as setting the property oneself except
   that this routine uses a cached copy of the key string and sets the
   property to hidden/const.

   Returns 0 on success, else:

   - CWAL_RC_MISUSE if any argument is NULL.

   - CWAL_RC_OOM on allocation error.

   - CWAL_RC_TYPE if !cwal_props_can(container).

   - CWAL_RC_CONST_VIOLATION if container already has a constructor
   property and it is const (otherwise it is overwritten).

   An example of customizing the constructor with state:

   @code
   int rc;
   cwal_function * f;
   cwal_value * fv;
   f = cwal_new_function( se->e, my_callback_f, my_callback_state,
                          my_callback_state_finalizer_f, my_type_id );
   if(!f) return CWAL_RC_OOM;
   fv = cwal_function_value(fv);
   assert(f == cwal_value_get_function(fv)); // will always be the case if f is valid
   cwal_value_ref(fv);
   rc = s2_ctor_method_set( se, myContainer, f );
   cwal_value_unref(fv);
   return rc;
   @endcode

   In such a setup, from inside the my_callback() implementation,
   cwal_args_state(args->engine, my_type_id) can be used to
   (type-safely) fetch the my_callback_state pointer. The s2_engine
   instance associated with the call can be fetched via
   s2_engine_from_args(args).

   @see s2_ctor_callback_set()
*/
int s2_ctor_method_set( s2_engine * se, cwal_value * container, cwal_function * method );

/**
   A convenience form of s2_ctor_method_set() which instantiates a
   cwal_function from the its 3rd argument using cwal_new_function().
   Returns CWAL_RC_MISUSE if any argument is NULL, else returns as for
   s2_ctor_method_set().
*/
int s2_ctor_callback_set( s2_engine * se, cwal_value * container, cwal_callback_f method );

/**
   Invokes a "constructor" function in the same manner as the 'new' keyword
   does. If ctor is NULL then s2 looks in operand (which must be a container
   type) for a property named "__new". If that property is not found or
   is not a Function, se's error state is set and non-0 is returned.

   All arguments except args and ctor must be non-NULL.

   This routine abstractly does the following:

   - creates a new Object.

   - sets operand to be the new object's prototype.

   - calls the ctor, passing on the new object as the "this" and any
   arguments in the args array (which may be NULL or empty).

   - if that ctor returns successfully: if it returns a cwal_value
   other than cwal_value_undefined() then that value is assigned to
   *rv, otherwise the newly-created Object is set to *rv. Thus a
   "return this", "return undefined", "return" and an implicit return
   all cause the newly-created object to be returned.  The intention
   is to allow constructors to return a different object which should
   then be substituted in the original's place (this is what "new"
   does with it).

   This all happens in an internally-pushed scope, but *rv will be
   rescoped to the caller's scope (for lifetime management purposes)
   if necessary.

   ACHTUNG: make sure you have a ref point on all parameters, else
   this function might end up cleaning any one of them up. If passed,
   e.g. an args array with no reference point, the passed-in array
   may be cleaned up by this function.

   On error, non-0 is returned and *rv will be assigned to 0.

   TODO: a variant of this which takes a C array of cwal_value
   pointers and the length of that array, as that's generally easier
   to work with in client code (but an array-based impl is what the
   "new" keyword needs). We "could" expose that array directly from a
   cwal public API but we don't because manipulating it from client
   code could be disastrous.
*/
int s2_ctor_apply( s2_engine * se, cwal_value * operand,
                   cwal_function * ctor, cwal_array * args,
                   cwal_value **rv );

/**
   Returns true (non-0) if v is legal for use as an operand do the
   "new" keyword. This only returns true if v is a container which
   holds (not counting properties inherited from prototypes) a key
   named "__new" with a value which is-a/has-a Function.
*/
char s2_value_is_newable( s2_engine * se, cwal_value * v );

/**
   Returns true (non-0) if the given value is currently being
   initialized as the "this" value of a constructor function via s2's
   "new" keyword. e.g. it can be used in the context of a C-native
   constructor to figure out if the function was used in a constructor
   context or not (if that makes any difference to how it functions).
*/
char s2_value_is_newing(cwal_value const * v);

/**
   Returns true if this function believes that mem (which must be at
   least len bytes of valid memory long) appears to have been
   compressed by s2_buffer_compress() or equivalent. This is not a
   100% reliable check - it could potentially have false positives on
   certain inputs, but that is thought to be unlikely (at least for
   text data). A false positive has never been witnessed in many
   thousands of tests in the libfossil tree. Knock on wood.

   Returns 0 if mem is NULL.

   @see s2_buffer_compress()
   @see s2_buffer_uncompress()
   @see s2_uncompressed_size()
*/
char s2_is_compressed(unsigned char const * mem, cwal_size_t len);

/**
   Equivalent to s2_is_compressed(buf->mem, buf->used), except that
   it returns 0 if !buf.

   @see s2_buffer_compress()
   @see s2_buffer_uncompress()
*/
char s2_buffer_is_compressed(cwal_buffer const *buf);

/**
   If s2_is_compressed(mem,len) returns true then this function
   returns the uncompressed size of the data, else it returns
   (uint32_t)-1. (Remember that an uncompressed size of 0 is legal!)
*/
uint32_t s2_uncompressed_size(unsigned char const *mem,
                              cwal_size_t len);

/**
   The cwal_buffer counterpart of s2_uncompressed_size().
*/
uint32_t s2_buffer_uncompressed_size(cwal_buffer const * b);

/**
   If built without zlib or miniz support, this function always
   returns CWAL_RC_UNSUPPORTED without side-effects.

   Compresses the first pIn->used bytes of pIn to pOut. It is ok for
   pIn and pOut to be the same blob.

   pOut must either be the same as pIn or else a properly
   initialized buffer. Any prior contents will be freed or their
   memory reused.

   Results are undefined if any argument is NULL.

   Returns 0 on success, CWAL_RC_OOM on allocation error, and
   CWAL_RC_ERROR if the lower-level compression routines fail.

   Use s2_buffer_uncompress() to uncompress the data.

   The data is encoded with a big-endian, unsigned 32-bit length as
   the first four bytes, and then the data as compressed by a
   "zlib-compatible" mechanism (which may or may not be zlib or
   miniz).

   After returning 0, pOut->used will hold the new, compressed size
   and s2_buffer_uncompressed_size() can be passed pOut to get the
   original size.

   Special cases:

   - If s2_buffer_is_compressed(pIn) and (pIn != pOut), then pIn is
   simply copied to pOut, replacing any existing content.

   - If s2_buffer_is_compressed(pIn) and (pIn == pOut), it has
   no side effects and returns 0.


   Minor achtung: the underlying compression library (which is
   specified as being "zlib-compatible", without saying it's actually
   zlib) allocates and frees memory from outside of e's memory
   management system, which means that it is not accounted for in,
   e.g. cwal-level metrics and memory capping.

   @see s2_buffer_uncompress()
   @see s2_buffer_is_compressed()
*/
int s2_buffer_compress(cwal_engine * e, cwal_buffer const *pIn,
                       cwal_buffer *pOut);

/**
   If built without zlib or miniz support, this function always
   returns CWAL_RC_UNSUPPORTED without side-effects.

   Uncompresses buffer pIn and stores the result in pOut. It is ok for
   pIn and pOut to be the same buffer, in which case the old contents
   will, on success, be destroyed. Returns 0 on success. On error pOut
   is not modified (whether or not pIn==pOut).

   pOut must be either cleanly initialized/empty or the same as pIn.

   Results are undefined if any argument is NULL or its memory is
   invalid.

   If (pIn == pOut) and !s2_buffer_is_compressed(pIn) then this
   function returns 0 without side-effects.

   Returns 0 on success, CWAL_RC_OOM on allocation error, and
   CWAL_RC_ERROR if the lower-level decompression routines fail.

   @see s2_buffer_compress()
   @see s2_buffer_compress2()
   @see s2_buffer_is_compressed()
*/
int s2_buffer_uncompress(cwal_engine * e, cwal_buffer const *pIn,
                         cwal_buffer *pOut);


/**
   Swaps left/right's contents. It retains (does not swap) the
   left->self/right->self pointers (swapping those _will_ corrupt
   their memory at some point). Results are undefined if (left==right)
   or if either argument is NULL or points to invalid memory.
*/
void s2_buffer_swap( cwal_buffer * left, cwal_buffer * right );

/**
    (Mostly) internal debugging tool which dumps out info about v (may
    be NULL), with an optional descriptive message. Expects file, func and
    line to be the __FILE__, __func__ resp.  __LINE__ macros. Use the
    s2_dump_val() macro to simplify that.

    Reminder: v cannot be const b/c some types go through JSON output,
    which requires non-const so that it can catch cycles.
*/
void s2_dump_value( cwal_value * v, char const * msg, char const * file,
                    char const * func, int line );

/**
   Equivalent to s2_dump_value(V, MSG, __func__, __LINE__).
*/
#define s2_dump_val(V, MSG) s2_dump_value((V), (MSG), __FILE__, __func__, __LINE__)

/**
   Prints out a cwal_printf()-style message to stderr and exit()s
   the application.  Does not return. Intended for use in place of
   assert() in test code. The library does not use this internally.
*/
void s2_fatal( int code, char const * fmt, ... );

/**
   Exists only to avoid adding an s2-internal-API dep in s2sh.

   Returns sizeof(s2_func_state).
*/
unsigned int s2_sizeof_script_func_state(void);


/**
   Removes the specially-propopagating "return" value (if any)
   from its special propagation mode, effectively transferring
   to the caller (with all the usual caveats about refcounts,
   unknowable ownership, etc.) See cwal_propagating_take()
   for details.

   In s2, the following keywords use specially-propagating values:
   return, break, exit (, fatal???). Exceptions use a separate
   propagation slot dedicated to the currently-thrown exception.
*/
cwal_value * s2_propagating_take( s2_engine * se );

/**
   s2 proxy for cwal_propagating_get() for details.
*/
cwal_value * s2_propagating_get( s2_engine * se );

/**
   s2 proxy for cwal_propagating_set() for details.
*/
cwal_value * s2_propagating_set( s2_engine * se, cwal_value * v );


/**
   Might or might not cwal_engine_sweep() or cwal_engine_vacuum() on
   se->e, depending on the state of se's various counters and
   guards. In any case, this function may increase a counter internal
   to the s2_engine instance.

   Returns 0 on success and "should" never fail.

   In debug builds, this routine asserts for any sort of problems. In
   non-debug builds it returns CWAL_RC_MISUSE if se has no current
   scope. It's conceivable that cwal_engine_vacuum() fails (returns
   non-0), but that can only happens if it detects memory corruption
   caused by mismanagement of Values, in which case an assert() is is
   triggered in debug builds. That said, a "real" failure of vacuum,
   at a time where a vacuum should be legal, has never been witnessed,
   so handling of a non-0 result code is largely a hypothetical
   problem. Feel free to ingore the result code.
*/
int s2_engine_sweep( s2_engine * se );


/**
   Intended to be used (if at all, then) for storing prototype
   values long-term in an s2_engine.

   Stores the given value in se's stash using the given name as a
   suffix for some larger unique key reserved for the various base
   prototypes. This implicitly moves proto into the global scope (for
   lifetime purpose) and makes it vacuum-proof.

   This "really" should only to be used by the various 
   s2_prototype_xxx() functions, but libfossil currently also makes 
   use of it, so it can't currently be made an internal API.
*/
int s2_prototype_stash( s2_engine * se, char const * typeName,
                        cwal_value * proto );

/**
   If the given name string was used to stash a prototype with
   s2_prototype_stash(), this function returns that value, else it
   returns 0. Ownership of the returned value is not modified.
*/
cwal_value * s2_prototype_stashed( s2_engine * se, char const * typeName );

/**
	Sets the __typename property on the given container to the given 
	value, with the (CWAL_VAR_F_CONST | CWAL_VAR_F_HIDDEN) flags set.
	This is more efficient than directly setting that property because
	s2_engine caches the __typename key.
*/
int s2_typename_set_v( s2_engine * se, cwal_value * container, cwal_value * name );

/**
   The C-string counterpart of s2_typename_set_v(), differing
   only in that it takes the type name in the form
   of the first nameLen bytes of name.
*/
int s2_typename_set( s2_engine * se, cwal_value * container,
                     char const * name, cwal_size_t nameLen );


/**
	If a byte-exact match of needle is found in haystack, its offset in
	the haystack is returned, else 0 is returned.
*/
char const * s2_strstr( char const * haystack, cwal_size_t hayLen,
                        char const * needle, cwal_size_t needleLen );


/**
   Evaluates the given script code in a new scope and stores the
   result of that script in the given container, using the given
   property name (which must be propNameLen bytes long). This is
   intended to simplify installation of small/embedded
   script-implemented functions from C code.

   If srcLen is negative, cwal_strlen() is used to calculate
   src's length.

   If the script triggers a 'return' then this routine captures
   that return'd value as the result.

   Returns 0 on success, a CWAL_RC_xxx or S2_RC_xxx value on error,
   and can be anything script evaluation could return.
*/
int s2_set_from_script( s2_engine * se, char const * src,
                        int srcLen, cwal_value * addResultTo,
                        char const * propName,
                        cwal_size_t propNameLen );

/**
   Works identically to s2_set_from_script() except that it takes
   its property name as a cwal_value.
*/
int s2_set_from_script_v( s2_engine * se, char const * src,
                          int srcLen, cwal_value * addResultTo,
                          cwal_value * propName );

/**
   A proxy for getcwd(2) which appends the current wording directory's
   name to the end of the given buffer. This function expands buffer
   by some relatively large amount to be able to handle long
   paths. Because of this, it's recommended that a shared/recycled
   buffer be used to handle calls to this function
   (e.g. s2_engine::buffer).

   On success, tgt gets appended to, updating tgt's members as
   appropriate, and 0 is returned. On error, non-0 is returned (the
   exact code may depend on the errno set by getcwd(2)).

   ACHTUNG: this function is only implemented for Unix systems. On
   Windows builds it returns CWAL_RC_UNSUPPORTED because i don't have
   a Windows system to implement this on.
*/
int s2_getcwd( cwal_engine * e, cwal_buffer * tgt );

/**
   A cwal_callback_f() implementation wrapping s2_getcwd(). On
   success, it returns (via *rv) the current wording directory's name
   as a string. If script passed an argument from script code, it is
   interpreted as a boolean: if true, the directory separator is
   appended to the result, else it is not. The default is not to
   append the directory separator to the result.
*/
int s2_cb_getcwd( cwal_callback_args const * args, cwal_value ** rv );

/**
   Works like mkdir(2). Returns 0 on success, else a CWAL_RC_xxx value
   approximating the underlying errno result. If errMsg is not NULL
   then on error, *errMsg will point to an error message string owned
   by the C library. Its contents may be modified by calls to
   strerror(3), so must be copied if it should be retained.

   LIMITATIONS:

   1) Returns CWAL_RC_UNSUPPORTED on builds which don't have mkdir(2).

   2) Does not create intermediate directories, so the parent dir
   of the new directory must already exist. See s2_mkdir_p().

   3) The message string returned by strerror() may be modified by
   calls to that function from other threads, so the errMsg argument
   is only known to be useful for single-threaded clients.

   @see s2_mkdir_p()
*/
int s2_mkdir( char const * name, int mode, char const ** errMsg );

/**
   A convenience wrapper around s2_mkdir() which creates parent
   directories, if needed, for the target directory name. e.g.  if
   passed "a/b/c" and "a" and/or "b" do not exist, they are created
   before creating "c". Fails if creation of any part of the path
   fails.

   It requires a well-formed Unix-style directory name (relative or
   absolute). Returns 0 on success, else non-0 and an error message,
   as documented for s2_mkdir().

   @see s2_mkdir()
*/
int s2_mkdir_p( char const * name, int mode, char const ** errMsg );

/**
   A cwal_callback_f() implementation wrapping s2_mkdir(). It mkdir() is
   not available in this built, throws an exception with code
   CWAL_RC_UNSUPPORTED.

   Script-side signatures:

   (string dirName [, bool makeParentDirs=false [, int mode = 0750]])
   (string dirName [, int mode = 0750])

   Noting that the access mode may be modified by the OS, e.g. to
   apply the umask.

   If a directory with the given name already exists, this function
   has no side-effects. Note that the access mode is ignored for
   purposes of that check.

   Throws a CWAL_RC_ACCESS exception if s2_disable_check() for the
   flag S2_DISABLE_FS_WRITE | S2_DISABLE_FS_STAT fails.

   Throws on error. On success, returns the undefined value.
*/
int s2_cb_mkdir( cwal_callback_args const * args, cwal_value ** rv );

/**
   File/directory types for use with s2_fstat_t.
*/
enum s2_fstat_types {
S2_FSTAT_TYPE_UNKNOWN = 0,
S2_FSTAT_TYPE_REGULAR = 0x01,
S2_FSTAT_TYPE_DIR = 0x02,
S2_FSTAT_TYPE_LINK = 0x04,
S2_FSTAT_TYPE_BLOCK = 0x08,
S2_FSTAT_TYPE_CHAR = 0x10,
S2_FSTAT_TYPE_FIFO = 0x20,
S2_FSTAT_TYPE_SOCKET = 0x40
};
/**
   Filesystem entry info for use with s2_fstat().
*/
struct s2_fstat_t {
  /**
     The type of a given entry.
  */
  enum s2_fstat_types type;
  /**
     Change time (includes metadata changes, e.g. permissions and
     such).
  */
  uint64_t ctime;
  /**
     Modification time (does not account for metadata changes,
     e.g. permissions and such).
  */
  uint64_t mtime;
  /** Size, in bytes. */
  uint64_t size;
  /** Unix permissions. */
  int perm;
};
typedef struct s2_fstat_t s2_fstat_t;
/**
   Intended to be used as a copy-construction source for local s2_fstat_t copies,
   to ensure a clean state.
 */
extern const s2_fstat_t s2_fstat_t_empty;
/**
   Cleanly-initialized s2_fstat_t entry, intended for const copy
   initialization.
*/
#define s2_fstat_t_empty_m {0,0,0,0,0}

/**
   Wrapper for stat(2) and lstat(2). The filename is taken from the
   first fnLen bytes of the given filename string. (We take the length
   primarily because cwal X-strings and Z-strings need not be
   NUL-terminated.) If tgt is not NULL then the results of the stat
   are written there. If derefSymlinks is 1 then stat() is used, else
   lstat() is used, which fetches information about a symlink, rather
   than the file the symlink points to.

   On success, returns 0 and, if tgt is not NULL, populates tgt.  On
   error, tgt is not modified.

   If stat() is not available in this build, CWAL_RC_UNSUPPORTED
   is returned.

   If lstat() is not available in this build, it returns
   CWAL_RC_UNSUPPORTED if derefSymlinks is false.

   If tgt is NULL and 0 is returned, it means that stat(2) succeeded.

   If stat(2) or lstat(2) fail, non-0 is returned (a CWAL_RC_xxx value
   approximating the errno from the failed call).

   Note that this function does not (cannot) honor
   s2_disabled_features flags because it has no s2_engine to check
   against.
*/
int s2_fstat( char const * filename,
              cwal_size_t fnLen,
              s2_fstat_t * tgt,
              char derefSymlinks );

/**
   A cwal_callback_f() implementation wrapping s2_fstat().

   Script signatures:

   1) object stat( string filename, [derefSymlinks=true] )

   Returns an object representing the stat() info (see
   s2_fstat_to_object() for the structure. Throws an exception if
   stat() fails.

   2) bool stat(string filename, undedfined [, derefSymlinks=true])

   If the argument count is greater than 1 and the the second argument
   has the undefined value then this function returns a boolean
   instead of an object. In that case, it returns false, instead of
   throwing, if stat() fails. That's admittedly a horribly awkward way
   to distinguish between the two return modes, and it may well be
   changed at some point.
*/
int s2_cb_fstat( cwal_callback_args const * args, cwal_value ** rv );

/**
   Converts a populated s2_fstat_t struct to an Object with properties describing
   the s2_fstat_t values:

   @code
   {
     ctime: integer, // state change time (includes perms changes)
     mtime: integer, // modification time
     perm: integer, // Unix permissions bits
     size: integer, // size, in bytes
     type: string // "unknown", "file", "dir", "link", "block", "char", "fifo", "socket"
   }
   @endcode

   On success, assigns *rv to the newly-created object and returns 0. On error
   *rv is not modified and returns non-zero (likely CWAL_RC_OOM).

   Just FYI: this function caches the keys (via s2_stash_set()) used
   for the returned object's properties, so it's not quite as
   expensive as it may initially seem.
*/
int s2_fstat_to_object( s2_engine * se, s2_fstat_t const * fst, cwal_value ** rv );

/**
   A wrapper around chdir(2). The first dirLen bytes of dir are used
   as a directory name passed to chdir(2). Returns 0 on success, non-0
   (a CWAL_RC_xxx code) on error. If this build does not have chdir(2)
   then CWAL_RC_UNSUPPORTED is returned.
 */
int s2_chdir( char const * dir, cwal_size_t dirLen );

/**
   A cwal_callack_f() impl wrapping s2_chdir().

   Script signature:

   void chdir(string dirname)

   Throws on error.
*/
int s2_cb_chdir( cwal_callback_args const * args, cwal_value **rv );

/**
   Glob policies for use with s2_glob_matches_str().
*/
enum s2_glob_style {
/**
   Match glob using conventional wildcard semantics.
*/
S2_GLOB_WILDCARD = -1,
/**
   Match glob using SQL LIKE (case-insensitive) semantics.
*/
S2_GLOB_LIKE_NOCASE = 0,
/**
   Match glob using SQL LIKE (case-sensitive) semantics.
*/
S2_GLOB_LIKE = 1
};

/**
   Return true (non-zero) if the NUL-terminated string z matches the
   NUL-terminated glob pattern zGlob, or zero if the pattern does not
   match. Always returns 0 if either argument is NULL. Supports a
   subset of common glob rules, very similar to those supported by
   sqlite3 (via sqlite3_strglob(), from which this code derives).

   Globbing rules for policy S2_GLOB_WILDCARD:

        '*'       Matches any sequence of zero or more characters.

        '?'       Matches exactly one character.

       [...]      Matches one character from the enclosed list of
                  characters.

       [^...]     Matches one character not in the enclosed list.

   With the [...] and [^...] matching, a ']' character can be included
   in the list by making it the first character after '[' or '^'.  A
   range of characters can be specified using '-'.  Example:
   "[a-z]" matches any single lower-case letter.  To match a '-', make
   it the last character in the list.


   Matching rules for policies S2_GLOB_LIKE and S2_GLOB_LIKE_NOCASE:
   
        '%'       Matches any sequence of zero or more characters

       '_'       Matches any one character

   The difference between those policies is that S2_GLOB_LIKE_NOCASE
   is case-insensitive.

*/
char s2_glob_matches_str(const char *zGlob,
                         const char *z,
                         enum s2_glob_style globStyle);

/**
   cwal_callback_f() wrapper for s2_glob_matches_str().

   Script-side usage:

   bool glob( string glob, string haystack [, int policy=-1] )

   Glob matching policies (3rd glob() param):

   <0 = (default) wildcard-style (case sensitive)

   0 = SQL LIKE style (case insensitive)

   >0 = SQL LIKE style (case sensitive)
*/
int s2_cb_glob_matches_str( cwal_callback_args const * args,
                            cwal_value ** rv );


/**
   A cwal_callback_f() implementation which tokenizes its input into
   an array of s11n tokens, but supporting only a subset of its core
   token types: numbers, strings (literals, heredocs, and, for
   historical reasons, {blocks}), and the built-in constanst
   bool/null/undefined values.

   Its intended purpose is to tokenize single lines of "commands" for
   use in command-based dispatching in scripts.

   Script usage:

   array tokenizeLine(string input)

   e.g.:

   tokenizeLine('1 2.3 "hi there" true null')

   would result in an array: [1, 2.3, 'hi there', true, null]

   This function does not parse higher-level constructs like objects,
   arrays, or functions, and cannot do so without significant
   reworking of the evaluation engine (where the logic for such
   constructs is embedded).

   For historical reasons, {blocks} are internally strings, which
   means that {a b c} will be tokenized as the string 'a b c', except
   that: 1) it will not get unescaped like a string literal and 2) its
   leading/trailing spaces will be removed.

   If the input string is empty, it resolves to the undefined value,
   not an empty array (though that decision is up for reconsideration,
   depending on what pending experience suggests).
*/
int s2_cb_tokenize_line(cwal_callback_args const * args, cwal_value ** rv);

/**
   Works just like the C-standard fopen() except that if zName is "-"
   then it returns either stdin or stdout, depending on whether zMode
   contains a "w", "a", or "+" (meaning stdout). Neither argument may
   be NULL.

   Returns a newly-opened file handle on success, else NULL. The
   handle should eventually be passed to s2_fclose() to close it.  It
   may optinoally be passed to fclose() instead, but that routine will
   unconditionally close the handle, whereas this one is a no-op for
   the standard streams.

   @see s2_fclose()
*/
FILE *s2_fopen(const char *zName, const char *zMode);

/**
   If the given FILE handle is one of (stdin, stdout, stderr), this is
   a no-op, else it passes the file handle to fclose().

   @see s2_fopen()
*/
void s2_fclose(FILE *);

/**
   Reads the given file (synchronously) until EOF and streams its
   contents (in chunks of an unspecified size) to cwal_output() using
   the given cwal_engine. Returns 0 on success, non-0 (likely
   CWAL_RC_IO) on error. Does not modify ownership of the passed-in
   pointers.
*/
int s2_passthrough_FILE( cwal_engine * e, FILE * file );

/**
   A convenience wrapper around s2_passthrough_FILE() which uses
   s2_fopen() to open the given filename and (on success) stream that
   file's contents to cwal_output(). Returns CWAL_RC_IO if the fopen()
   fails, else returns as documented for s2_passthrough_FILE().
*/
int s2_passthrough_filename( cwal_engine * e, char const * filename );


/**
   This odd function is intended to assist in certain types of
   bindings which want to store, e.g. a custom Prototype somewhere
   other than in s2_prototype_stash(). This function creates a new key
   using cwal_new_unique() and uses it store 'what' in 'where'
   with the CWAL_VAR_F_HIDDEN and CWAL_VAR_F_CONST flags. This binding
   will ensure that 'what' gets referenced and rescoped as necessary
   to keep it alive along with 'where'. One caveat: if
   cwal_props_clear(where) is called, it will delete that binding,
   possibly leading to unexpected behaviour in other places which are
   expecting 'what' to be alive.

   A concrete example: custom modules often need a custom prototype,
   but don't really have a good place to store it. One option is to
   attach it as state to the Function which acts as that type's
   constructor (because the prototype is typically only invoked, at
   the C level, from there). However, Function state is of type
   (void*), and thus is not "Value-aware" and cannot be rescoped as
   needed.  One workaround to keep that prototype alive is to store it
   as a Value in the Function. Because the key is unknown, it cannot
   be looked up, but it can (with a little extra work) be accessed via
   cwal_function_state_get() and friends. The binding created by
   _this_ function keeps that state alive so long as the property does
   not get removed. Several of the modules in the s2 source tree
   demonstrate this function's use.

   Reminder to self: it might be interesting to add a new
   CWAL_VAR_F_xxx flag which tells even cwal_props_clear() not to
   remove the property.  That would be somewhat more invasive and
   special-case than i'm willing to hack right now, though. :/
*/
int s2_stash_hidden_member( cwal_value * where, cwal_value * what );

/**
   This functions provides cwal_callback_f() implementations the
   possibility of cleanly exiting the current script, as if the "exit"
   keyword had been triggered. Its result is intended to be returned
   from such a callback.

   It uses the given value as the script's exit result. The value may
   be NULL, which is interpreted as cwal_value_undefined().

   This function replaces any pending propagating result (from any
   in-progress return/break/continue/exit) with the given result
   value. Be aware that if you need the old propagating value then you
   must, before calling this, s2_propagating_take() it or
   s2_propagating_get() it and cwal_value_ref() it.

   This function always returns CWAL_RC_EXIT, which gets interpreted
   by the eval engine like the 'exit' keyword. (Note that simply
   returning CWAL_RC_EXIT from a callback will fail in other ways,
   possibly triggering an assert() in s2, because setting up the
   'exit' involves more than just this result code.)

   Design note: this function takes a cwal_callack_args instead of a
   cwal_engine (or s2_engine) only to emphasize that it's only
   intended to be called from cwal_callback_f() implementations. (It
   also relies on s2's specific interpretation of the CWAL_RC_EXIT
   result code.)

   Example:

   @code
   int my_callback(cwal_callback_args const * args, cwal_value ** ){
     ... do something ...;
     return s2_trigger_exit(args, 0);
   }
   @endcode
*/
int s2_trigger_exit( cwal_callback_args const * args, cwal_value * result );

/**
   If passed a container-type value, it either "seals" or "unseals"
   the value by setting resp. unsetting the
   CWAL_CONTAINER_DISALLOW_PROP_SET and
   CWAL_CONTAINER_DISALLOW_PROTOTYPE_SET flags on it. When set, the
   engine will refuse to set properties or change prototypes on the
   object, and will error out with CWAL_RC_DISALLOW_PROP_SET
   resp. CWAL_RC_DISALLOW_PROTOTYPE_SET if an attempt is made. This
   only affects future property/prototype operations, not existing
   values.

   If passed a truthy 2nd argument, the container is sealed, else it
   is unsealed. Note that unsealing is considered to be a rare
   corner-case, and is not intended to be used willy-nilly.

   Returns 0 on succes, CWAL_RC_MISUSE if v is NULL, and CWAL_RC_TYPE
   if the value is not a container.

   The "seal" restrictions do not currently (20191210) apply to
   array/tuple indexes, but probably should (noting that tuples
   currently have no flags which would allow us to mark them as
   sealed). That behaviour may change in the future.
*/
int s2_seal_container( cwal_value * v, char sealIt );

/**
   A script-bindable callback which expects to be passed one or more
   container-type values. For each one it calls s2_seal_container(),
   passing true as the 2nd argument (so it supports sealing, but not
   unsealing, as unsealing would allow script code to do unsightly
   things with enums and other sealed containers).

   This callback throws if passed no arguments or if any argument is
   not a container type.

   On success it returns, via *rv, the last container it modifies.
*/
int s2_cb_seal_object( cwal_callback_args const * args, cwal_value ** rv );

/**
   A cwal_callback_f() implementation which tokenizes conventional
   PATH-style strings. Script-side usage:

   array f( string path [, array target] )

   Each element in the input path is appended to the target array (or
   a new array if no array is passed in) and that array is returned.

   It triggers an exception if passed any invalid arguments, noting
   that an empty input string is not an error but will cause an empty
   list to be returned (resp. no entries to be added to the target
   list).
*/
int s2_cb_tokenize_path( cwal_callback_args const * args, cwal_value **rv );

/**
   Tokenizes a conventional PATH-style string into an array.

   *tgt must either be NULL or point to an array owned by the
   caller. If it is NULL, this function creates a new array and (on
   success) assigns it to *tgt. Each entry in the path is added to the
   array.

   None of the pointer-type arguments, including tgt (as opposed to
   *tgt), may be NULL.

   The 4th argument must be the length, in bytes, of the given path
   string. If it is negative, the equivalent of strlen() is used to
   calculate its length. If the argument, or its resulting calculated
   value, is 0 then the result will be that the target array will be
   created, if needed, but will get no entries added to it.

   On success, 0 is returned and 0 or more entries will have been
   added to the target array. If *tgt was NULL when this function was
   called, *tgt will be assigned to a new array, with a refcount of 0,
   which is owned by the caller.

   Assuming all arguments are valid, the only plausible error this
   function will return is CWAL_RC_OOM, indicating that allocation of
   the target array, an entry for the array, or space within the array
   for an entry, failed. On error, if *tgt was NULL when this function
   was called, *tgt is not modified, otherwise *tgt may have been
   modified before the allocation error occurred.

   Example:

   @code
   cwal_array * ar = 0;
   int rc = s2_tokenize_path_to_array(e, &ar, "foo;bar", -1);
   @code

   On success, ar will be non-NULL and owned by that code.
   Contrast with:

   @code
   int rc;
   cwal_array * ar = cwal_new_array(e);
   if(!ar) return CWAL_RC_OOM;
   rc = s2_tokenize_path_to_array(e, &ar, "foo;bar", -1);
   @code

   In that case, any entries in the given path string will be appended
   to the array provided by the caller.


   @see s2_path_toker
   @see s2_path_toker_init()
 */
int s2_tokenize_path_to_array( cwal_engine * e, cwal_array ** tgt,
                               char const * path,
                               cwal_int_t pathLen );

/**
   Never, ever use this unless you are testing s2's internals and
   happen to know that it's potentially useful.
*/
int s2_cb_internal_experiment( cwal_callback_args const * args, cwal_value **rv );

/**
   Returns the path to s2's "home directory," or NULL if it is
   unknown.

   Currently, the home directory is specified via the S2_HOME
   environment variable. If that environment variable is set, this
   function returns its value in memory which will live as long as the
   current app instance. If that variable is not set, NULL is
   returned.

   If non-NULL is returned and len is not NULL, *len is assigned the
   length of the string, in bytes.

   In the future there might or might not be other ways to specify the
   s2 home, perhaps on a per-engine-instance basis (and thus the
   s2_engine parameter for this function, though it is currently
   unused).
*/
char const * s2_home_get(s2_engine * se, cwal_size_t * len);

/**
   Installs a user-defined pseudo-keyword into the given engine,
   granting fast acess to the value and a lifetime as long as the
   engine is running.

   The given string must be a legal s2 identifier with a length equal
   to the 3rd argument. Its bytes are copied, so it need not have a
   well-defined lifetime. If the given length is negative,
   cwal_strlen() is used to calculate the key's length.

   "User Keywords," or UKWDs, as they're colloquially known, use s2's
   keyword infrastructure for their lookups, meaning that they bypass
   all scope-level lookups and have an amortized faster search time
   than var/const symbols. The determination of whether a given symbol
   is a UKWD an average O(log n) operation (n=number of UKWDs), and
   then finding the associated value for that requires a hashtable
   lookup (average O(1)).

   These are not "real" keywords, in that they cannot interact
   directly with s2's parser. They simply resolve to a single value of
   an arbitrary type which can be quickly resolved in the
   keyword-lookup phase of script evaluation.

   Returns:

   - 0 on success.

   - CWAL_RC_RANGE if name is empty.

   - CWAL_SCR_SYNTAX if name is not strictly an identifier token.

   - CWAL_RC_ACCESS if a real keyword with that name exists.

   - CWAL_RC_ALREADY_EXISTS if an entry with that name already exists

   - CWAL_RC_RANGE if the name is empty or "too long" (there is a
   near-arbitrary upper limit).

   - CWAL_RC_UNSUPPORTED if v is NULL or is the undefined value, both
   of which are reserved for *potential* "undefine" support.

   - CWAL_RC_OOM on allocation error.

   On error, se's error state is updated with an explanation of the
   problem.

   Caveats:

   1) Installing these takes up memory (more than a simple property
   mapping), so don't go crazy with 100 of them. Likewise, adding many
   keyword changes the duration of that "1" in "O(1)". Not by terribly
   much, but keyword lookups happen *all the time* internally (every
   time an identifier is seen), so it adds up.

   2) They cannot be re-set or uninstalled once they have been
   installed.
*/
int s2_define_ukwd(s2_engine * se, char const * name,
                   cwal_int_t nameLen, cwal_value * v);

/**
   A set of *advisory* flags for features which well-behaved s2 APIs
   should honor. The flag names listed for each entry are for use with
   s2_disable_set_cstr().

   This is generally only intended to apply to script-side APIs, not
   their equivalent C APIs. e.g. s2_eval_filename() does not honor
   these flags, but its script-binding counterparts,
   s2_cb_import_script() and the import keyword, do.
*/
enum s2_disabled_features {
/**
   The disable-nothing flag.

   Flag name: "none"
*/
S2_DISABLE_NONE = 0,
/**
   Disables stat()'ing of files, as well as checking/changing the
   current directory, in compliant APIs.

   Flag name: "fs-stat"
*/
S2_DISABLE_FS_STAT = 1,

/**
   Disables opening/reading of files in compliant APIs.

   Note that APIs must also normally (but not always) OR this with
   S2_DISABLE_FS_STAT, else a file may be opened for reading without
   stat()'ing it first (i.e. without checking whether it exists).

   Flag name: "fs-read"

   Reminder to self: we may want a separate flag which indicates that
   JSON files are an exception to this limitation. JSON is known to
   not allow execution of foreign code nor loading of non-JSON
   content, so it's "safer" that generic filesystem-read access.
*/
S2_DISABLE_FS_READ = 1 << 1,

/**
   Disables opening/writing and creation of new files in compliant
   APIs.

   Flag name: "fs-write"
*/
S2_DISABLE_FS_WRITE = 1 << 2,

/**
   Flag name: "fs-io"
*/
S2_DISABLE_FS_IO = S2_DISABLE_FS_READ | S2_DISABLE_FS_WRITE,

/**
   Flag name: "fs-all"
*/
S2_DISABLE_FS_ALL = S2_DISABLE_FS_IO | S2_DISABLE_FS_STAT
};

/**
   Assigns the set of API-disabling flags on se.

   @see s2_disable_get()
   @see s2_disable_check()
*/
void s2_disable_set( s2_engine * se, cwal_flags32_t flags );

/**
   A variant of s2_disable_set() which accepts a
   comma-and/or-space-separated list of flag names to set as its 2nd
   argument. The 3rd argument specifies the length of the 2nd, in
   bytes. If the 3rd argument is negative, cwal_strlen() is used to
   determine the 2nd argument's length. A length of 0 causes the flags
   to be set to 0.

   On success, 0 is returned and if result is not NULL, the result of
   the flags is assigned to *result, as well as being applied to se.

   On error, non-0 is returned and neither se nor *result are modified
   other than to set se's error state. An unknown flag is treated as
   an error.

   The flag names are listed in the docs for the s2_disabled_features
   enum entries. Each entry gets OR'd to the result, with one
   exception: the "none" flag resets the flags to 0.
*/
int s2_disable_set_cstr( s2_engine * se, char const * str, cwal_int_t strLen,
                         cwal_flags32_t * result );

/**
   Returns the set of API-disabling flags from se

   @see s2_disable_set()
   @see s2_disable_check()
*/
cwal_flags32_t s2_disable_get( s2_engine const * se );

/**
   If any of se's advisory feature-disable flags match the given
   flags, se's error state is set and non-0 is returned, else se is
   not modified and 0 is returned. If this returns non-0, its
   state can be converted to an exception with s2_throw_err(),
   passing it 0 for all arguments after the first.

   @see s2_disable_set()
   @see s2_disable_get()
   @see s2_disable_check_throw()
   @see s2_cb_disable_check()
*/
int s2_disable_check( s2_engine * se, cwal_flags32_t f );

/**
   Equivalent to s2_disable_check(), but transforms any error state
   to an exception.
*/
int s2_disable_check_throw( s2_engine * se, cwal_flags32_t f );

/**
   Convenience form of s2_disable_check_throw(), intended for use in
   cwal_callback_f() implementations, which checks the given flags
   against the s2_engine returned by s2_engine_from_args().
*/
int s2_cb_disable_check( cwal_callback_args const * args, cwal_flags32_t f );

/**
   "Should" be called ONE TIME from the application's main() in order
   to perform certain static initialization, e.g. install a
   cwal_rc_cstr_f fallback for s2-related result codes.

   If it is not called beforehand, it will be called the first time an
   s2_engine instance is initialized, but that involves a very narrow
   window for a mostly-harmless race condition.
*/
void s2_static_init(void);

/**
   S2_TRY_INTERCEPTORS: _HIGHLY EXPERIMENTAL_ property interceptor
   support. DO NOT USE THIS. It's not known to work, and will very
   likely be removed because of its performance implications.
*/
#define S2_TRY_INTERCEPTORS 0

#if S2_TRY_INTERCEPTORS
cwal_function * s2_value_is_interceptor( cwal_value const * v );
#endif

#if S2_TRY_INTERCEPTORS
/**
   _HIGHLY EXPERIMENTAL_. Do not use. While these basically work, they
   are not going to be enabled because the performance hit is too high
   for non-interceptor cases (i.e. the vast majority).
*/
int s2_cb_add_interceptor( cwal_callback_args const * args, cwal_value **rv );
#endif

#ifdef __cplusplus
}/*extern "C"*/
#endif

#endif
/* include guard */
/* end of file s2.h */
/* start of file s2_internal.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
/**
  License: same as cwal. See cwal.h resp. cwal_amalgamation.h for details.

  This file holds s2-internal declarations and types. These are not
  intended for client-side use.
*/
#ifndef NET_WANDERINGHORSE_CWAL_S2_INTERNAL_H_INCLUDED_
#define NET_WANDERINGHORSE_CWAL_S2_INTERNAL_H_INCLUDED_
/** @file */ /* for doxygen */

#if !defined(DOXYGEN)
#ifdef __cplusplus
extern "C" {
#endif

/** @internal

   S2_UNUSED_VAR exists only to squelch, in non-debug builds, warnings
   about the existence of vars which are only used in assert()
   expressions (and thus get filtered out in non-debug builds).
*/
#define S2_UNUSED_VAR __attribute__((__unused__))
/**
   S2_UNUSED_ARG is used to squelch warnings about interface-level
   function parameters which are not used in a given implementation:

   @code
   int some_interface(type1 arg1, type2 arg2){
     S2_UNUSED_ARG arg1;
     ...
     return 0;
   }
   @code
*/
#define S2_UNUSED_ARG (void)

typedef struct s2_op s2_op;

/** @internal

   Internal state for Object.eachProperty() and similar functions.
*/
struct s2_kvp_each_state {
  /** Interpreter engine for the visitor. */
  cwal_engine * e;
  /** The 'this' for the visitor. */
  cwal_value * self;
  /** The function to call for each iteration of the visit. */
  cwal_function * callback;
  /** If true, the callback gets the value as the first arg. */
  char valueArgFirst;
};
typedef struct s2_kvp_each_state s2_kvp_each_state;
/**
   An empty-initialized s2_kvp_each_state instance, intended for
   copying over new instances to cleany initialize their state.
 */
extern const s2_kvp_each_state s2_kvp_each_state_empty;


/** @internal
   Allocates a new token using se's cwal-level allocator. The value
   must eventually be cleaned up using s2_stoken_free(). Returns 0 on
   allocation error (e.g. if !se or !se->e).
*/
s2_stoken * s2_stoken_alloc( s2_engine * se );

/** @internal
   Convenience form of s2_stoken_alloc() which sets a token's type
   and value. type may be any value and v may be NULL.
*/
s2_stoken * s2_stoken_alloc2( s2_engine * se, int type, cwal_value * v );

/** @internal
   "frees" the given token. If allowRecycle is false, or if se's
   token recycling feature is disabled or has reached its capacity,
   then t is immediate freed using se's allocator, otherwise t will
   be placed into a recycling bin for later re-use via
   s2_stoken_alloc().

   TODO: see if we really use that 3rd argument, and remove it if we
   don't.
*/
void s2_stoken_free( s2_engine * se, s2_stoken * t, char allowRecycle );

/** @internal
   Pushes t to the given stack and transfers ownership of t
   to ts.
*/
void s2_stoken_stack_push( s2_stoken_stack * ts, s2_stoken * t );

/** @internal
   If ts has any tokens, the top-most one is removed and that token
   is returned. Ownership of the returned value is transfered to the
   caller. Returns 0 on error (!ts or ts is empty).
*/
s2_stoken * s2_stoken_stack_pop( s2_stoken_stack * ts );

/** @internal
   Frees all entries owned by ts by popping each one and passing it
   to s2_stoken_free(se, theToken, allowRecycle). See s2_stoken_free()
   for the semantics of the se and allowRecycle parameters.
*/
void s2_stoken_stack_clear( s2_engine * se, s2_stoken_stack * ts, char allowRecycle );

/** @internal
   Works like s2_stoken_stack_clear(), but operates on both stacks owned by st.
*/
void s2_estack_clear( s2_engine * e, s2_estack * st, char allowRecycle );

/** @internal
   Swaps the contents of lhs and rhs (neither may be NULL).
*/
void s2_estack_swap( s2_estack * lhs, s2_estack * rhs );

/** @internal
   Pushes the given token to se's value stack and transfers its
   ownership to se. The only error condition is if either argument is
   0 or points to invalid memory.

   Reminder to self: this API does not internally add/remove refs to
   s2_stokens. This works, but only accidentally. It "should" be
   refactored to take a ref when pushing a value, unhand(?) when
   popping a value at the client's request, and unref when cleaning up
   on its own without the client having taken ahold of
   s2_stoken::value.
*/
void s2_engine_push_valtok( s2_engine * se, s2_stoken * t );

/** @internal
   The operator-stack counterpart of s2_engine_push_valtok().
*/
void s2_engine_push_op( s2_engine * se, s2_stoken * t );

/** @internal
   Creates a new token with the given type, appends it to se,
   and returns it. Return 0 on allocation error.
*/
s2_stoken * s2_engine_push_ttype( s2_engine * se, int i );

/** @internal
   Pushes a new token of type S2_T_Value to se, and assigns v to
   that token's value. It does not change the reference count or
   ownership of v.

   Returns 0 on allocation error or if se or v are 0. On success
   it returns the token pushed onto the stack.
*/
s2_stoken * s2_engine_push_val( s2_engine * se, cwal_value * v );

/** @internal
   Creates a new cwal_value value of type integer and pushes it onto
   se as described for s2_engine_push_val(). Returns 0 on allocation
   error, or the pushed token on success.
*/
s2_stoken * s2_engine_push_int( s2_engine * se, cwal_int_t i );

/** @internal
   Pushes a new token onto VALUE stack, with the given type and
   the given value (which may be NULL but likely should not be).
*/
s2_stoken * s2_engine_push_tv( s2_engine * se, int ttype, cwal_value * v );

/** @internal
   If se's stack contains any tokens, the top-most token is removed.
   If returnEntry is false then the token is placed in se's recycle
   bin and 0 is returned. If returnEntry is true, the token is
   returned to the caller, as is ownership of that token. If se has
   no stack entries, 0 is returned.

*/
s2_stoken * s2_engine_pop_token( s2_engine * se, char returnEntry );


/** @internal
   Similar to s2_engine_pop_token(), this destroys the top-most
   token and returns its value (if any), transfering ownership (or
   stewardship) of that value to the caller.
*/
cwal_value * s2_engine_pop_value( s2_engine * se );

/** @internal
   The operator-stack counterpart of s2_engine_pop_token().
*/
s2_stoken * s2_engine_pop_op( s2_engine * se, char returnEntry );

/** @internal
   Returns the top-most token in se's token stack, without modifying
   the stack. Returns 0 if se's token stack is empty.
*/
s2_stoken * s2_engine_peek_token( s2_engine * se );

/** @internal
   Equivalent to s2_engine_peek_token(se)->value, but returns 0
   (instead of segfaulting) if the value stack is empty. Does not
   modify ownership of the returned value.
*/
cwal_value * s2_engine_peek_value( s2_engine * se );

/** @internal
   Swaps the contents of se->st with st. Neither argument
   may be NULL.
*/
void s2_engine_stack_swap( s2_engine * se, s2_estack * st );

/** @internal
   Clears all entries from se->st, recycling them if possible,
   freeing them if not.
*/
void s2_engine_reset_stack( s2_engine * se );

/** @internal

    Runs cwal_engine_vacuum() on se->e. DO NOT CALL THIS
    unless you are 100% absolutely sure you want to and that it is safe
    to do so (which is very dependent on local conditions).
*/
void s2_engine_vacuum( s2_engine * se );

/** @internal
   The op-stack counterpart of s2_engine_peek_token().
*/
s2_stoken * s2_engine_peek_op( s2_engine * se );

/** @internal
   Pushes t onto either the token stack or (if it represents an
   operator) the operator stack.

   @see s2_ttype_op()
   @see s2_stoken_op()
*/
void s2_engine_push( s2_engine * se, s2_stoken * t );

/** @internal
   Processes the top-most operator on the stack and pushes the
   result value back onto the stack.

   [i don't think this is true anymore:] A small number of "marker"
   operators do not generate a result, and do not push a result onto
   the stack, but leave the value stack as they found it. It is not
   yet clear whether or not this API needs to provide a way for
   clients to know about that. Currently those operators are handled
   directly by the higher-level parser code or are handled as part of
   another operation (e.g. the S2_T_MarkVariadicStart token marks the
   end of N-ary call ops).

   Returns 0 on success, non-0 on error. On error, the stack state
   is not well-defined unless CWAL_RC_TYPE is returned (indicating
   that the top token is not an operator).
*/
int s2_process_top( s2_engine * se );

/** @internal
   A lower-level form of s2_process_top(), this variant does not
   modify the operator stack. It is assumed that op is one which was
   just popped from it by the caller, or "would have" become the top
   of the stack but the push is being elided as an optimization.

   It is up to the caller to ensure that se's stack is set up
   appropriately for the given op before calling this.

   Returns 0 on success, a non-0 CWAL_RC_xxx value on error.
*/
int s2_process_op( s2_engine * se, s2_op const * op );

/** @internal
   Equivalent to s2_process_op(se, s2_ttype_op(ttype)), except that
   it returns CWAL_RC_TYPE if ttype is not an operator.
*/
int s2_process_op_type( s2_engine * se, int ttype );

/** @internal
   Clears any pending tokens from se's stack(s).
*/
void s2_engine_reset_stack( s2_engine * se );

/** @internal

    Post-processor for converting a single "opener" token into
    a larger token containing the whole braced group, writing
    the resulting token to the given output token pointer.

    Requires st->token to be a S2_T_SquigglyOpen, S2_T_BraceOpen, or
    S2_T_ParenOpen token. It "slurps" the content part, from the
    starting brace up to (and include) the ending brace. It uses
    s2_next_token() to parse the group, meaning that it must
    contain tokenizable code. We cannot simply do a byte-scan through
    it because doing so would exclude any constructs (e.g. strings)
    which might themselves contain tokens which would look like
    closing tokens here.

    If !out, then st->token is used as the output destination. If out
    is not NULL then after returning, st->token and st->pbToken will
    have their pre-call state (regardless of error or success).

    Returns 0 on success. On error it will set se's error state.

    On success:

    - out->ttype is set to one of: S2_T_SquigglyBlock,
      S2_T_ParenGroup, S2_T_BraceGroup.

    - [out->begin, out->end) will point to the whole token, including
    its leading and trailing opener/closer tokens. That range will
    have a length of at least 2 (one each for the opening and closing
    token). [out->adjBegin, out->adjEnd) will point to the range
    encompassing the body of the open/close block, stripped of any
    leading or trailing whitespaces. That range may be empty.

*/
int s2_slurp_braces( s2_engine *se, s2_ptoker * st,
                     s2_ptoken * out );


/** @internal

    Post-processor for S2_T_HeredocStart tokens. se is the interpreter, st is
    the current tokenizer state. tgt is where the results are written. If tgt
    is NULL, st->token is used.

    Requires that st->token be a S2_T_HeredocStart token. This function
    scans st for the opening and closing heredoc tokens.

    On error a non-0 CWAL_RC_xxx value is returned and se's error
    state will be updated. On error st->token is in an unspecified
    state.

    On success tgt->ttype is set to S2_T_SquigglyBlock and
    [out->begin, out->end) will point to the whole token proper,
    including its leading and trailing opener/closer tokens. That
    range will have a length of at least 2 (one each for the opening
    and closing token). [out->adjBegin, out->adjEnd) will point to
    the range encompassing the body of the open/close block, stripped
    of any leading or trailing spaces, as specified below. That range
    may be empty.

    Returns 0 on success and sets se's error state on error. 

    Syntax rules:

    Assuming that the S2_T_HeredocStart token is '<<<', heredocs
    can be constructed as follows:

    <<<EOF blah blah blah EOF
    <<<EOF blah blah blahEOF // equivalent!
    <<<'EOF' ... 'EOF'
    <<<"EOF" ... "EOF"
    <<<:EOF ... EOF // colon changes space-skipping rules

    Failure to find a match for the heredoc identifier will result in
    an error. Identifiers may optionally be quoted, but both the
    opener and closer must use the same quote character.

    Anything between the opening and closing identifiers belongs to
    the heredoc body, but leading/trailing whitespace gets trimmed as
    follows:

    Space-skipping rules:

    - If the first character after the heredoc token is a colon,
    then the heredoc skips exactly 1 newline or space from the beginning
    and end of the heredoc.

    - Otherewise all leading and trailing whitespace characters are
    trimmed from the result.

    Note that the result token's [begin,end) range spans the _whole_
    heredoc, whereas [adjBegin,adjEnd) encompasses the heredoc body.
    i.e. trimming changes the token's adjBegin and adjEnd ranges.

*/
int s2_slurp_heredoc( s2_engine * se, s2_ptoker * st,
                      s2_ptoken * tgt );


/** @internal

    If an exception is pending AND it has not yet been decorated
    with script location information, that is done here. If pr is
    not 0 then it is used for the location information, else
    se->currentScript is used. If se->currentScript is also 0, or no
    error location is recorded somewhere in se->opErrPos or the
    script chain then this function has no side effects.

    Returns 0 for a number of not-strictly-error conditions:

    - !pr && !se->currentScript

    - if there is no pending exception, or if that exception value
    may not have properties (it is not a container type)

    - OR if the pending exception already contains location
    information.
*/
int s2_exception_add_script_props( s2_engine * se, s2_ptoker const * pr );

/** @internal

   Like s2_add_script_props2(), but calculates the script name, line, and column
   as follows:

   - The name comes from s2_ptoker_name_top()

   - The script code position (presumably an error location) comes
   from s2_ptoker_err_pos_first().

*/
int s2_add_script_props( s2_engine * se, cwal_value * v, s2_ptoker const * pr );

/** @internal

   If v is a container, this adds the properties "script", "line",
   and "column" to v, using the values passed in. If scriptName is
   NULL or empty, it is elided.  Likewise, the line/column
   properties are only set if (line>0).

   If se has stack trace info, it is also injected into v. Potential
   TODO: separate this routine into a couple different bits. The
   current behaviour assumes that v is (or will be in a moment) an
   Exception.

   @see s2_count_lines()
*/
int s2_add_script_props2( s2_engine * se, cwal_value * v,
                          char const * scriptName, int line, int col);

/** @internal

    Internal cwal_value_visitor_f() implementation which requires
    state to be a (cwal_array*). This function appends v to that array
    and returns the result.
*/
int s2_value_visit_append_to_array( cwal_value * v, void * state );


/** @internal
   State for script-side functions. Each script function gets
   one of these attached to it.
*/
struct s2_func_state {
  /**
     The source code for this function. For "empty" functions this
     will be NULL. It gets rescoped along with the containing Function
     value.
  */
  cwal_value * vSrc;

  /**
     An Object which holds all "imported" symbols associated with this
     function via the "using" pseudo-keyword when defining a function
     or via Function.importSymbols(). When this function gets called,
     all of these symbols get imported into the call()'s scope.

     20190911: it "might be interesting" to add a keyword, or even a
     local variable, to access this list from inside a
     function. Slightly tricky would be to only resolve that symbol
     from inside the body of the function it applies to (similar to
     how the break keyword only works from within a loop).

     20191209: the using() keyword now provides access to this. Part
     of that change is that this value's prototype is removed. We've
     never needed it in native code and remove it now so that script
     code does not inadvertently do anything silly with it or trip
     over inherited properties.
  */
  cwal_value * vImported;

  /**
     The name of the function, if any, gets defined as a scope-local
     symbol when the function is called, just like this->vImported
     does.
  */
  cwal_value * vName;

  /**
     A hashtable key for the script's name. This key points to a
     string in se->funcStash, and is shared by all functions which
     have the same script name. Unfortunately, those names must
     currently stay in the hash for the life of the interpreter, but
     it is not as though that that poses any real problem other than
     (potentially) unused script names being kept in in the hash. The
     problem is that we don't know when it's safe to remove the entry
     (don't know when the last function-held ref to it is gone, and
     string interning can confuse the matter). A proper fix (i.e.
     which does not keep these strings permanently owned by the global
     scope) requires keeping the name and a counter (number of
     functions using that name).

     TODO?: now that we have rescopers for functions, we could have
     each function store/rescope this itself, without stashing it.
     String interning might make this cheap (and might not - filenames
     can be long and long strings aren't interned). That could get
     expensive for utilities which return lots of small functions
     (e.g. utils which bind values for later use).
  */
  cwal_value * keyScriptName;

  /**
     For the recycling subsystem: this is the next item in the
     recycling list.
  */
  s2_func_state * next;

  /**
     The line (1-based) of the function declaration. Used for
     adjusting script location information.
  */
  uint16_t line;

  /**
     The column (0-based) of the function declaration. Used for
     adjusting script location information.
  */
  uint16_t col;

  /* Internal flags. */
  cwal_flags16_t flags;

};

/**
   Empty-initialized s2_func_state instance. Used as a type ID
   in a few places.
*/
extern const s2_func_state s2_func_state_empty;

/** @internal

   Internal, experimental, and incomplete.

   Intended to be called at various (many) points during s2's
   evaluation process, and passed the (CWAL_RC_xxx) result code of the
   local, just-run operation. If that code is a fatal one, it returns
   that code without side effects. If it is not a fatal one then this
   function returns CWAL_RC_INTERRUPTED _if_ s2_interrupt() has been
   called, else it returns localRc.

   If passed 0 and it returns non-0 then s2_interrupt() was called.

   i.e. a "really bad" result code trumps both localRc and
   the is-interrupted check, and the is-interrupted check trumps
   localRc.

   The "really bad" codes (which are returned as-is) include:
   CWAL_RC_FATAL, CWAL_RC_EXIT, CWAL_RC_ASSERT, CWAL_RC_OOM,
   CWAL_RC_INTERRUPTED.

   TODO (2021-06-26): strongly consider moving the is-interrupted
   check into the cwal core.
*/
int s2_check_interrupted( s2_engine * se, int localRc );

/** @internal

    If f is an s2 script function then its s2 state part
    is returned. The state is owned by s2 and MUST NOT
    be modified or held for longer than f is.
*/
s2_func_state * s2_func_state_for_func(cwal_function * f);

/** @internal

   Looks for a constructor function from operand, _not_ looking in
   prototypes (because ctors should generally not be inherited).

   If pr is not NULL then it is assumed to be the currently-evaluating
   code and will be used for error reporting purposes if errPolicy !=
   0.

   errPolicy determines error handling (meaning no ctor found):

   0 = return error code but set no error state

   <0 = return error code after throwing error state (so the result
   will be CWAL_RC_EXCEPTION unless a lower-level (allocation) failure
   trumps it).

   >0 = return error code after setting non-exception error state.


   If a valid ctor is found:

   - if rv is not NULL, *rv is set to the ctor (in all other cases
   *rv is not modified).

   - 0 is returned.

   The error code returned when no viable ctor found is found is
   CWAL_RC_NOT_FOUND, but the errorPolicy may translate that to
   an exception, returning CWAL_RC_EXCEPTION.
*/
int s2_ctor_fetch( s2_engine * se, s2_ptoker const * pr,
                   cwal_value * operand, cwal_function **rv,
                   int errPolicy );



/** @internal

   Saves some of se's internal state to the 2nd parameter and
   resets it to its initial state in se. If this is called, the
   caller is obligated to call s2_engine_subexpr_restore(),
   passing the same two parameters, before control returns to
   an outer block.
*/
void s2_engine_subexpr_save(s2_engine * se, s2_subexpr_savestate * to);

/** @internal

   Restores state pushed by s2_engine_subexpr_save().
*/
void s2_engine_subexpr_restore(s2_engine * se, s2_subexpr_savestate const * from);

/** @internal

   Flags for use with s2_scope_push_with_flags(). They must fit in a
   uint16_t.
*/
enum s2_scope_flags {
/**
   Sentinel entry. Must be 0.
*/
S2_SCOPE_F_NONE = 0,
/**
   This tells s2_scope_push_with_flags() to retain any current
   s2_engine::ternaryLevel, rather than saving/resetting it when
   pushing and restoring it when popping. It is intended for "inlined"
   (non-block) expressions which push a scope, so that in-process
   ternary op handling can DTRT with a ':' in the RHS of such
   expressions. e.g. (foo ? catch someFunc() : blah). Without this
   flag, the catch expression will try to consume the ':' and fail
   because it doesn't see that there's a ternary-if being
   processed. This flag makes scope/catch/etc. aware that a ':' is
   part of a pending ternary op. Note that (foo ? catch
   {someFunc():blah()}), where the ":" is inside {...} still triggers
   a syntax error.
*/
S2_SCOPE_F_KEEP_TERNARY_LEVEL = 0x0001,

/**
   Not yet quite sure what this is for. It might go away.
*/
S2_SCOPE_F_IS_FUNC_CALL = 0x0002
};

/** @internal

    This is equivalent to calling cwal_scope_push(), then setting
    the given flags on the new s2_scope which gets added to the
    stack via the cwal_engine's scope push hook.

    Returns 0 on success. On error non-0 is returned (a CWAL_RC_xxx
    code) then pushing of the scope has failed (likely CWAL_RC_OOM)
    and something is Seriously Wrong.
*/
int s2_scope_push_with_flags( s2_engine * se, cwal_scope * tgt, uint16_t flags );


/** @internal

   Returns se's current s2-level scope, or 0 if no scope is
   active. ACHTUNG: the returned pointer can be invalidated by future
   pushes/pops of the cwal stack, so it MUST NOT be held on to across
   push/pop boundaries. Rather than holding an (s2_scope*) across
   push/pop boundaries, call this as needed to get the current
   pointer.

   @see cwal_scope_push()
   @see cwal_scope_pop()
   @see cwal_scope_pop2()
*/
s2_scope * s2_scope_current( s2_engine const * se );


/** @internal

   Returns the s2-level scope for the current cwal scope level, or
   NULL if level is out of bounds. cwal scoping starts at level 1 and
   increases by 1 for each new scope popped on the stack. i.e passing
   this a level of 1 will return the top-most scope. The current
   scoping level can be found in s2_engine::scopes::level or
   via the 'level' member of the cwal_scope object returned by
   cwal_scope_current_get().

   Note that the scopes have very strict lifetime rules, and clients
   must never modify their state or clean them up or anything silly
   like that.

   The cwal_scope counterpart of the returned object can be obtained
   via its cwalScope member.

   Scope levels are managed by cwal, but s2 keeps its own scopes
   (s2_scope) for holding s2-specific scope-level state. These are
   kept in sync via the cwal-level scope push/pop hooks.

   IMPORTANT: the returned pointer can be invalidated by future pushes
   onto, and pops from, the scope stack, and thus it must never be
   held across scope-push/pop boundaries. The poitner should be
   fetched as needed and then discard.

   @see cwal_scope_push()
*/
s2_scope * s2_scope_for_level( s2_engine const * se, cwal_size_t level );

/** @internal

   Sets up se's internals so that it knows the most recent dot-op
   self/lhs/key (or is not, if passed NULLs for the last 3
   arguments). If self is not NULL then self is considered to be
   "this" in the corresponding downstream handling (i.e. it's a
   property access which binds "this"), else it does not. self is
   always lhs or 0. lhs is either the container for a property lookup
   (if self!=0) or a scope var lookup. key is the associated property
   key.

   The property/scope variable operations set these up and several
   places in the eval engine (namely function calls) consume and/or
   clear/reset them (to avoid leaving stale entries around).

   Pass all 0's to reset it.

   Reminder to self: before calling this, MAKE SURE you've got ref to
   any local values which are about to returned/passed along. If
   needed, ref before calling this and unhand afterwards. If a value
   about to be returned/upscoped is one of
   se->dotOpThis/dotOp.key/dotOp.lhs then that ref will save the app's
   life.
*/
void s2_dotop_state( s2_engine * se, cwal_value * self,
                     cwal_value * lhs, cwal_value * key );


/** @internal

   An internal operator/keyword-level helper to be passed the result
   code from the s2_set() family of funcs, to allow those routines to
   report error information, including location, for the failure
   point.  Returns rc or a modified form of it when moving
   s2_engine::err state to an exception.

   If rc is not 0 then s2_throw_err_ptoker() may (with a small few
   exceptions) may be called to propagate any error info, in which
   case se->err is assumed to have been set up by s2_set() (and
   friends). Some few error codes are simply returned as-is, without
   collecting error location info: CWAL_RC_OOM, CWAL_RC_EXCEPTION,
   CWAL_RC_NOT_FOUND.

   If !pr then se->currentScript is used. If both are NULL, or !rc,
   this is a no-op.
*/
int s2_handle_set_result( s2_engine * se, s2_ptoker const * pr, int rc );

/** @internal

   The getter analog of s2_handle_set_result().
*/
int s2_handle_get_result( s2_engine * se, s2_ptoker const * pr, int rc );


/** @internal
   Callback function for operator implementations.
     
   The framework will pass the operator, the underlying engine, the
   number of operatand it was given (argc).

   Preconditions:

   - se's token stack will contain (argc) tokens intended
   for this operator.

   - The top of se operator stack will not be this operator
   (or not this invocation of it).

   - rv will not be NULL, but may point to a NULL pointer.

   Implementation requirements:

   - Implementations MUST pop EXACTLY (argc) tokens from se's
   token stack before returning or calling into any API which
   might modify se's token stack further. They must treat those
   values as if they are owned by the caller.

   - A result value, if any, must be assigned to *rv. If the
   operation has no result, assign it to 0. The API will never pass
   a NULL rv to this routine. If the operation creates a scope and
   receives *rv from that scope then it must be sure to upscope *rv
   before returning, to ensure that *rv is still valid after the
   scope is popped. As a rule, the result value is ignored/discarded
   if non-0 (error) is returned.

   - Must return 0 on success, and a non-0 CWAL_RC_xxx code on
   error.

   - If se->skipLevel is greater than 0, then the operator must pop
   all arguments from the stack, do as little work as possible
   (e.g. no allocations or calculations, and no visible side effects),
   assign *rv to cwal_value_undefined(), and return 0. This is used
   for implementing quasi-short-circuit logic, in that we allow the
   operators to run, but skip-mode indicates that we really are only
   interested in getting past the operator and its arguments, without
   having side-effects like creating new values.

   Certain result codes will be treated specially by the framework and
   have certain pre- and post-conditions (not described in detail here):

   CWAL_RC_OOM triggers a fatal OOM error.

   CWAL_RC_EXCEPTION means the function triggered an exception
   and set the engine's exception state.

   CWAL_RC_EXIT, CWAL_RC_FATAL, and CWAL_RC_INTERRUPTED trigger and
   end of the current evaluation and set the engine's error state.
*/
typedef int (*s2_op_f)( s2_op const * self, s2_engine * se,
                        int argc, cwal_value **rv );

/** @internal
   Represents a stack machine operation. Each operation in s2 equates
   to a shared/const instance of this class.
*/
struct s2_op {
  /**
     Generic symbolic representation, not necessarily what appears
     in script code. Need not be unique, either. Primarily intended
     for use in debugging and error messages.
  */
  char const * sym;

  /**
     Operator type ID. Must be one of the s2_token_types
     values.
  */
  int id;

  /**
     Number of expected operands. Negative value means any number, and the
     syntax will have to determine where to stop looking for operands.
  */
  int8_t arity;

  /**
     Associativity:

     <0 = left

     0 = non-associative

     >0 = right
  */
  int8_t assoc;

  /**
     Precedence. Higher numbers represent a higher precedence.
  */
  int8_t prec;

  /**
     Describes where the operator "sits" in relation to its
     operand(s).

     <0 = has no operands or sits on the left of its
     operand(s). Arity must be >=0.

     0 = sits between its 2 operands. arity must == 2.

     >0 = sits on the right of its 1 operand. arity must == 1.

  */
  int8_t placement;

  /**
     The operator's implementation function.
  */
  s2_op_f call;

  /**
     Not yet used.

     An experiment in inferring compound comparison operators from
     simpler operators, e.g. infering '<' and '==' from '<=' and using
     them if the latter op is not available but the former two are.

     inferLeft is the LHS op id, inferRight is the RHS id.
  */
  int inferLeft;
  /**
     Not yet used.
  */
  int inferRight;

  /**
     Not currently used. Intended to replace the inferLeft/inferRight
     bits (which also aren't currently used).

     For "assignment combo" operators (+=, -=, etc). s2 "should" be
     able to derive an implementation if the non-assignment part of
     that has been overloaded (e.g. if operator+ is overloaded, s2
     should be able to derive operator+= from that). We could (in
     theory) safely always evaluate such ops to their own "this"
     value, and wouldn't(?) even need to perform the actual assignment
     because such overloads always need to return their own "this" in
     order to work properly (and the overload presumably applies the
     operator to itself, as opposed to a new copy, which would break
     assignments). If someone really wants to return non-this, they
     could overload it explicitly, which would of course trump a
     derived version.

     Anyway... for assignment combo ops this "should" be the s2_op::id
     of the related non-assignment operator which "could" be used to
     automatically implement the combo op.
   */
  int derivedFromOp;
};

/** @internal
   Empty-initialized s2_op structure, intended for
   const-copy initialization. Only used internally.
*/
#define s2_op_empty_m {                         \
    0/*sym*/, 0/*id*/, 0/*arity*/,              \
    0/*assoc*/, 0/*prec*/,                    \
    0/*placement*/, 0/*call()*/,              \
    0/*inferLeft*/, 0/*inferRight*/,        \
    0/*derivedFromOp*/                    \
  }

#if 0
/** @internal
   Empty-initialized s2_op structure, intended for
   copy initialization.
*/
extern const s2_op s2_op_empty;
#endif

/** @internal
   If ttype (a s2_token_types value) represents a known Operator
   then that operator's shared/static/const instance is returned,
   otherwise NULL is returned.
*/
s2_op const * s2_ttype_op( int ttype );

/** @internal
   Equivalent to s2_stoken_op(t->type).
*/
s2_op const * s2_stoken_op( s2_stoken const * t );

int s2_op_is_math( s2_op const * op );
int s2_op_is_expr_border( s2_op const * op );
int s2_op_is_unary_prefix( s2_op const * op );

/** @internal
   Equivalent to s2_ttype_short_circuits(op ? op->id : 0).
*/
int s2_op_short_circuits( s2_op const * op );

/** @internal

    Rescopes, if needed, v to lhs's scope. Intended for use just after
    v is created and v is held on to by a native value associated with
    lhs (e.g. when lhs is bound to s2_func_state and v is one of
    s2_func_state::vSrc, vImported, or vName). When creating such
    values, it's almost always critical to adjust the new value's
    scope just after creating it. Once the underlying binding is in
    place, the native-level rescoper is responsible for future
    rescoping of the value.
*/
void s2_value_to_lhs_scope( cwal_value const * lhs, cwal_value * v);


/** @internal
   Intended to be passed the result of s2_set_v() (or similar).
   This routine upgrades certain result codes to engine-level
   error state and returns rc or some other error code. If !rc,
   it always returns rc.
*/
int s2_handle_set_result( s2_engine * se, s2_ptoker const * pr, int rc );


/** @internal
   Intended to be passed the result of s2_get_v() (or similar).
   This routine upgrades certain result codes to engine-level
   error state and returns rc or some other error code. If !rc,
   it always returns rc.
*/
int s2_handle_get_result( s2_engine * se, s2_ptoker const * pr, int rc );

/** @internal

   Fetches fst's "imported symbols" holder object, initializing it if
   needed (which includes rescoping it to theFunc's scope). theFunc
   _must_ be the Function Value to which fst is bound.

   If this function creates the imported symbols holder, its prototype
   is removed.

   Is highly intolerant of NULL arguments.

   Returns NULL on OOM, else the properties object (owned by fst
   resp. theFunc).
*/
cwal_value * s2_func_import_props( s2_engine * se,
                                   cwal_value const * theFunc,
                                   s2_func_state * fst );


/** @internal

    A kludge/workaround for the eval engine's lack of references on
    its being-eval'd values (string interning, in particular, can bite
    us, leading to cwal-level assert() failures when lifetime rules
    are violated). VERY LONG STORY made very short: this routine adds
    v (if it's not NULL and not a builtin value) to
    s2->currentScope->evalHolder (if that list is not NULL) to keep a
    reference to them.

    Returns 0 if it does nothing or if adding v to the list succeeds,
    non-0 on serious error (almost certainly CWAL_RC_OOM, as that's
    the only realistic error result).
    See s2_scope::evalHolder for more details.
*/
int s2_eval_hold( s2_engine * se, cwal_value * v );

/** @internal
   
    Flags for s2_func_state::flags.
*/
enum s2_func_state_flags_t {
S2_FUNCSTATE_F_NONE = 0,
/**
   Indicates that the function's parameter list contains no non-junk
   tokens, so it need not be evaluated at call()-time.
*/
S2_FUNCSTATE_F_EMPTY_PARAMS = 0x01,
/**
   Indicates that the function's body contains no non-junk tokens, so
   it need not be evaluated at call()-time.
*/
S2_FUNCSTATE_F_EMPTY_BODY = 0x02,
/**
   This flag indicates that the "using" clause of a function
   definition was tagged to indicate that any imported symbols (via
   "using" or Function.importSymbols()) should not be declared as
   call-local variables. Instead, they may be accessed in the function
   via the "using" keyword. e.g.:

   proc()using.{a: 1}{
    assert !typeinfo(islocal a);
    s2out << using.a << '\n';
   };

   Without the "." modifier, that assert would fail.

   Mnemonic: "using." is exactly how the symbols nee
*/
S2_FUNCSTATE_F_NO_USING_DECL = 0x04,
/**
   Reserved.
*/
S2_FUNCSTATE_F_CLASS = 0x08,
/**
   EXPERIMENTAL and likely to go away:

   When a function has this flag, if its body contains the
   string s2_engine::cache::keyInterceptee then this flag
   gets set.
*/
S2_FUNCSTATE_F_INTERCEPTEE = 0x10
};

typedef struct s2_keyword s2_keyword;

/** @internal

    Keyword handler signature.
   
*/
typedef int (*s2_keyword_f)( s2_keyword const * kw, s2_engine * se, s2_ptoker * pr, cwal_value **rv);

/** @internal

    Holds state for a single keyword.
*/
struct s2_keyword {
  int /* non-const needed internally */ id;
  char const * word;
  unsigned short /*non-const needed for ukwd */ wordLen;
  s2_keyword_f call;
  /**
     If true, then LHS invocations of this keyword will, if they terminate
     in a {block}, treat an EOL immediately following as an EOX. This is
     to make 'if' and friends easier on the eyes, as nobody wants to be forced
     to add a semicolon to:

     if(...){

     };
  */
  char allowEOLAsEOXWhenLHS;
#if 0
  /**
     An internal detail of the ukwd bits. Doh - adding this would
     require editing the inlined initialization structs of all the
     built-in keywords :/. Time to go macro-fy that, it seems.
  */
  cwal_value * ukwd;
#endif
};

/**
   Structure for holding a list of "user keywords" (UKWDs, as they're
   colloquially known).
*/
struct s2_ukwd {
  /**
     List of keyword object.
  */
  s2_keyword * list;
  /**
     Hash of UKWD names to their values.
  */
  cwal_hash * h;
  /**
     Number of entries in this->list which are currently in use.
  */
  cwal_size_t count;
  /**
     Number of entries allocated for this->list.
  */
  cwal_size_t alloced;
};
typedef struct s2_ukwd s2_ukwd;

/** @internal

    Frees any memory held by se->ukwd, including se->ukwd itself.
*/
void s2_ukwd_free(s2_engine * se);

/** @internal

   A "perfect hasher" for s2 keywords. This algo uses a simple hashing
   mechanism which is known to produce collision-free hashes for the
   s2 built-in keywords and typeinfo/pragma operators.

   It hashes the first n bytes of src and returns the hash.

   We have a hash code limit of 32 bits because these values are used
   in switch/case statements, and we cannot portably use values >32
   bits as a case value.

   It returns 0 if the input length is 0 or the input contains any
   characters with a value less than '$' or greater than 'z', which
   includes all non-ASCII UTF8, noting that UKWDs are not hashed this
   way, so they may still contain any s2-legal identifier characters.

   Maintenance WARNING: this algo MUST be kept in sync with the one in
   s2-keyword-hasher.s2, as that script generates the C code
   for our keyword/typeinfo/pragma bits.
*/
uint32_t s2_hash_keyword( char const * src, cwal_size_t n );

#ifdef __cplusplus
}/*extern "C"*/
#endif
#endif /* DOXYGEN */

#endif
/* include guard */
/* end of file s2_internal.h */
#endif/*!defined(WANDERINGHORSE_NET_CWAL_S2_AMALGAMATION_H_INCLUDED)*/
