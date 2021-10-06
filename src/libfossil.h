#if !defined(FSL_AMALGAMATION_BUILD)
#define FSL_AMALGAMATION_BUILD 1
#endif
#if defined(HAVE_CONFIG_H)
#  include "config.h"
#endif
#include "libfossil-config.h"
/* start of file ../include/fossil-scm/fossil-config.h */
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
/* end of file ../include/fossil-scm/fossil-config.h */
/* start of file ../include/fossil-scm/fossil.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_LIBFOSSIL_H_INCLUDED)
#define ORG_FOSSIL_SCM_LIBFOSSIL_H_INCLUDED
/*
  Copyright 2013-2021 Stephan Beal (https://wanderinghorse.net).



  This program is free software; you can redistribute it and/or
  modify it under the terms of the Simplified BSD License (also
  known as the "2-Clause License" or "FreeBSD License".)

  This program is distributed in the hope that it will be useful,
  but without any warranty; without even the implied warranty of
  merchantability or fitness for a particular purpose.

  *****************************************************************************
  This file is the primary header for the public APIs. It includes
  various other header files. They are split into multiple headers
  primarily becuase my poor old netbook is beginning to choke on
  syntax-highlighting them and browsing their (large) Doxygen output.
*/

/*
   config.h MUST be included first so we can set some portability
   flags and config-dependent typedefs!
*/

#endif
/* ORG_FOSSIL_SCM_LIBFOSSIL_H_INCLUDED */
/* end of file ../include/fossil-scm/fossil.h */
/* start of file ../include/fossil-scm/fossil-util.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_UTIL_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_UTIL_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

*/

/** @file fossil-util.h

    This file declares a number of utility classes and routines used by
    libfossil. All of them considered "public", suitable for direct use
    by client code.
*/

#include <stdio.h> /* FILE type */
#include <stdarg.h> /* va_list */
#include <time.h> /* tm struct */
#include <stdbool.h>
#if defined(__cplusplus)
extern "C" {
#endif
typedef struct fsl_allocator fsl_allocator;
typedef struct fsl_buffer fsl_buffer;
typedef struct fsl_error fsl_error;
typedef struct fsl_finalizer fsl_finalizer;
typedef struct fsl_fstat fsl_fstat;
typedef struct fsl_list fsl_list;
typedef struct fsl_outputer fsl_outputer;
typedef struct fsl_state fsl_state;
typedef struct fsl_id_bag fsl_id_bag;

/**
   fsl_uuid_str and fsl_uuid_cstr are "for documentation and
   readability purposes" typedefs used to denote strings which the API
   requires to be in the form of Fossil UUID strings. Such strings are
   exactly FSL_STRLEN_SHA1 or FSL_STRLEN_K256 bytes long plus a
   terminating NUL byte and contain only lower-case hexadecimal
   bytes. Where this typedef is used, the library requires, enforces,
   and/or assumes (at different times) that fsl_is_uuid() returns true
   for such strings (if they are not NULL, though not all contexts
   allow a NULL UUID). These typedef are _not_ used to denote
   arguments which may refer to partial UUIDs or symbolic names, only
   100% bonafide Fossil UUIDs (which are different from RFC4122
   UUIDs).

   The API guarantees that this typedef will always be (char *) and
   that fsl_uuid_cstr will always ben (char const *), and thus it
   is safe/portable to use those type instead of these. These
   typedefs serve only to improve the readability of certain APIs
   by implying (through the use of this typedef) the preconditions
   defined for UUID strings.

   Sidebar: fossil historically used the term UUID for blob IDs, and
   still uses that term in the DB schema, but it has fallen out of
   favor in documentation and discussions, with "hash" being the
   preferred term. Much of the libfossil code was developed before
   that happened, though, so "UUID" is still prevalent in its API and
   documentation.

   @see fsl_is_uuid()
   @see fsl_uuid_cstr
*/
typedef char * fsl_uuid_str;

/**
   The const counterpart of fsl_uuid_str.

   @see fsl_is_uuid()
   @see fsl_uuid_str
*/
typedef char const * fsl_uuid_cstr;

/**
   A typedef for comparison function used by standard C
   routines such as qsort(). It is provided here primarily
   to simplify documentation of other APIs. Concrete
   implementations must compare lhs and rhs, returning negative,
   0, or right depending on whether lhs is less than, equal to,
   or greater than rhs.

   Implementations might need to be able to deal with NULL
   arguments. That depends on the routine which uses the comparison
   function.
*/
typedef int (*fsl_generic_cmp_f)( void const * lhs, void const * rhs );

/**
   If the NUL-terminated input str is exactly FSL_STRLEN_SHA1 or
   FSL_STRLEN_K256 bytes long and contains only lower-case
   hexadecimal characters, returns the length of the string, else
   returns 0.

   Note that Fossil UUIDs are not RFC4122 UUIDs, but are SHA1 or
   SHA3-256 hash strings. Don't let that disturb you. As Tim
   Berners-Lee writes:

   'The assertion that the space of URIs is a universal space
   sometimes encounters opposition from those who feel there should
   not be one universal space. These people need not oppose the
   concept because it is not of a single universal space: Indeed,
   the fact that URIs form universal space does not prevent anyone
   else from forming their own universal space, which of course by
   definition would be able to envelop within it as a subset the
   universal URI space. Therefore the web meets the "independent
   design" test, that if a similar system had been concurrently and
   independently invented elsewhere, in such a way that the
   arbitrary design decisions were made differently, when they met
   later, the two systems could be made to interoperate.'

   Source: https://www.w3.org/DesignIssues/Axioms.html

   (Just mentally translate URI as UUID.)
*/
FSL_EXPORT int fsl_is_uuid(char const * str);

/**
   If x is a valid fossil UUID length, it is returned, else 0 is returned.
*/
FSL_EXPORT int fsl_is_uuid_len(int x);

/**
   Expects str to be a string containing an unsigned decimal
   value. Returns its decoded value, or -1 on error.
*/
FSL_EXPORT fsl_size_t fsl_str_to_size(char const * str);

/**
   Expects str to be a string containing a decimal value,
   optionally with a leading sign. Returns its decoded value, or
   dflt if !str or on error.
*/
FSL_EXPORT fsl_int_t fsl_str_to_int(char const * str, fsl_int_t dflt);


/**
   Generic list container type. This is used heavily by the Fossil
   API for storing arrays of dynamically-allocated objects. It is
   not useful as a non-pointer-array replacement.

   It is up to the APIs using this type to manage the entry count
   member and use fsl_list_reserve() to manage the "capacity"
   member.

   @see fsl_list_reserve()
   @see fsl_list_append()
   @see fsl_list_visit()
*/
struct fsl_list {
  /**
     Array of entries. It contains this->capacity entries,
     this->count of which are "valid" (in use).
  */
  void ** list;
  /**
     Number of "used" entries in the list.
  */
  fsl_size_t used;
  /**
     Number of slots allocated in this->list. Use fsl_list_reserve()
     to modify this. Doing so might move the this->list pointer but
     the values it points to will stay stable.
  */
  fsl_size_t capacity;
};

/**
   Empty-initialized fsl_list structure, intended for const-copy
   initialization.
*/
#define fsl_list_empty_m { NULL, 0, 0 }
/**
   Empty-initialized fsl_list structure, intended for copy
   initialization.
*/
FSL_EXPORT const fsl_list fsl_list_empty;


/**
   Generic interface for finalizing/freeing memory. Intended
   primarily for use as a destructor/finalizer for high-level
   structs. Implementations must semantically behave like free(mem),
   regardless of whether or not they actually free the memory. At
   the very least, they generally should clean up any memory owned by
   mem (e.g. db resources or buffers), even if they do not free() mem.
   some implementations assume that mem is stack-allocated
   and they only clean up resources owned by mem.

   The state parameter is any state needed by the finalizer
   (e.g. a memory allocation context) and mem is the memory which is
   being finalized. 

   The exact interpretaion of the state and mem are of course
   implementation-specific.
*/
typedef void (*fsl_finalizer_f)( void * state, void * mem );

/**
   Generic interface for memory finalizers.
*/
struct fsl_finalizer {
  /**
     State to be passed as the first argument to f().
  */
  void * state;
  /**
     Finalizer function. Should be called like this->f( this->state, ... ).
  */
  fsl_finalizer_f f;
};

/** Empty-initialized fsl_finalizer struct. */
#define fsl_finalizer_empty_m {NULL,NULL}

/**
   fsl_finalizer_f() impl which requires that mem be-a
   (fsl_buffer*).  This function frees all memory associated with
   that buffer and zeroes out the structure, but does not free mem
   (because it is rare that fsl_buffers are created on the
   heap). The state parameter is ignored.
*/
FSL_EXPORT int fsl_finalizer_f_buffer( void * state, void * mem );


/**
   Generic state-with-finalizer holder. Used for binding
   client-specified state to another object, such that a
   client-specified finalizer is called with the other object is
   cleaned up.
*/
struct fsl_state {
  /**
     Arbitrary context-dependent state.
  */
  void * state;
  /**
     Finalizer for this->state. If used, it should be called like:

     @code
     this->finalize.f( this->finalize.state, this->state );
     @endcode

     After which this->state must be treated as if it has been
     free(3)'d.
  */
  fsl_finalizer finalize;
};

/** Empty-initialized fsl_state struct. */
#define fsl_state_empty_m {NULL,fsl_finalizer_empty_m}

/**
   Empty-initialized fsl_state struct, intended for
   copy-initializing.
*/
FSL_EXPORT const fsl_state fsl_state_empty;


/**
   Generic interface for streaming out data. Implementations must
   write n bytes from s to their destination channel and return 0 on
   success, non-0 on error (assumed to be a value from the fsl_rc_e
   enum). The state parameter is the implementation-specified
   output channel.

   Potential TODO: change the final argument to a pointer, with
   semantics similar to fsl_input_f(): at call-time n is the number
   of bytes to output, and on returning n is the number of bytes
   actually written. This would allow, e.g. the fsl_zip_writer APIs
   to be able to stream a ZIP file (they have to know the real size
   of the output, and this interface doesn't support that
   operation).
*/
typedef int (*fsl_output_f)( void * state, void const * src, fsl_size_t n );


/**
   Generic interface for flushing arbitrary output streams.  Must
   return 0 on success, non-0 on error, but the result code
   "should" (to avoid downstream confusion) be one of the fsl_rc_e
   values. When in doubt, return FSL_RC_IO on error. The
   interpretation of the state parameter is
   implementation-specific.
*/
typedef int (*fsl_flush_f)(void * state);

/**
   Generic interface for streaming in data. Implementations must
   read (at most) *n bytes from their input, copy it to dest, assign
   *n to the number of bytes actually read, return 0 on success, and
   return non-0 on error (assumed to be a value from the fsl_rc_e
   enum). When called, *n is the max length to read. On return, *n
   is the actual amount read. The state parameter is the
   implementation-specified input file/buffer/whatever channel.
*/
typedef int (*fsl_input_f)( void * state, void * dest, fsl_size_t * n );

/**
   fsl_output_f() implementation which requires state to be a
   (fsl_cx*) to which this routine simply redirects the output via
   fsl_output().  Is a no-op (returning 0) if !n. Returns
   FSL_RC_MISUSE if !state or !src.
*/
FSL_EXPORT int fsl_output_f_fsl_cx(void * state, void const * src, fsl_size_t n );


/**
   An interface which encapsulates data for managing an output
   destination, primarily intended for use with fsl_output(). Why
   abstract it to this level? So that we can do interesting things
   like output to buffers, files, sockets, etc., using the core
   output mechanism. e.g. so script bindings can send their output
   to the same channel used by the library and other library
   clients.
*/
struct fsl_outputer {
  /**
     Output channel.
  */
  fsl_output_f out;
  /**
     flush() implementation.
  */
  fsl_flush_f flush;
  /**
     State to be used when calling this->out(), namely:
     this->out( this->state.state, ... ).
  */
  fsl_state state;
};
/** Empty-initialized fsl_outputer instance. */
#define fsl_outputer_empty_m {NULL,NULL,fsl_state_empty_m}
/**
   Empty-initialized fsl_outputer instance, intended for
   copy-initializing.
*/
FSL_EXPORT const fsl_outputer fsl_outputer_empty;

/**
   A fsl_outputer instance which is initialized to output to a
   (FILE*). To use it, this value then set the copy's state.state
   member to an opened-for-write (FILE*) handle. By default it will
   use stdout. Its finalizer (if called!) will fclose(3)
   self.state.state if self.state.state is not one of (stdout,
   stderr). To disable the closing behaviour (and not close the
   file), set self.state.finalize.f to NULL (but then be sure that
   the file handle outlives this object and to fclose(3) it when
   finished with it).
*/
FSL_EXPORT const fsl_outputer fsl_outputer_FILE;

/**
   fsl_outputer initializer which uses fsl_flush_f_FILE(),
   fsl_output_f_FILE(), and fsl_finalizer_f_FILE().
*/
#define fsl_outputer_FILE_m {                   \
    fsl_output_f_FILE,                          \
      fsl_flush_f_FILE,                         \
      {/*state*/                                \
        NULL,                                   \
        {NULL,fsl_finalizer_f_FILE}             \
      }                                         \
  }
/**
   Generic stateful alloc/free/realloc() interface.

   Implementations must behave as follows:

   - If 0==n then semantically behave like free(3) and return
   NULL.

   - If 0!=n and !mem then semantically behave like malloc(3), returning
   newly-allocated memory on success and NULL on error.

   - If 0!=n and NULL!=mem then semantically behave like
   realloc(3). Note that realloc specifies: "If n was equal to 0,
   either NULL or a pointer suitable to be passed to free() is
   returned." Which is kind of useless, and thus implementations
   MUST return NULL when n==0.
*/
typedef void *(*fsl_realloc_f)(void * state, void * mem, fsl_size_t n);

/**
   Holds an allocator function and its related state.
*/
struct fsl_allocator {
  /**
     Base allocator function. It must be passed this->state
     as its first parameter.
  */
  fsl_realloc_f f;
  /**
     State intended to be passed as the first parameter to
     this->f().
  */
  void * state;
};

/** Empty-initialized fsl_allocator instance. */
#define fsl_allocator_empty_m {NULL,NULL}


/**
   A fsl_realloc_f() implementation which uses the standard
   malloc()/free()/realloc(). The state parameter is ignored.
*/
FSL_EXPORT void * fsl_realloc_f_stdalloc(void * state, void * mem, fsl_size_t n);


/**
   Semantically behaves like malloc(3), but may introduce instrumentation,
   error checking, or similar.
*/
void * fsl_malloc( fsl_size_t n )
#ifdef __GNUC__
  __attribute__ ((malloc))
#endif
  ;

/**
   Semantically behaves like free(3), but may introduce instrumentation,
   error checking, or similar.
*/
FSL_EXPORT void fsl_free( void * mem );

/**
   Behaves like realloc(3). Clarifications on the behaviour (because
   the standard has one case of unfortunate wording involving what
   it returns when n==0):

   - If passed (NULL, n>0) then it semantically behaves like
   fsl_malloc(f, n).

   - If 0==n then it semantically behaves like free(2) and returns
   NULL (clarifying the aforementioned wording problem).

   - If passed (non-NULL, n) then it semantically behaves like
   realloc(mem,n).

*/
FSL_EXPORT void * fsl_realloc( void * mem, fsl_size_t n );

/**
   A fsl_flush_f() impl which expects _FILE to be-a (FILE*) opened
   for writing, which this function passes the call on to
   fflush(). If fflush() returns 0, so does this function, else it
   returns non-0.
*/
FSL_EXPORT int fsl_flush_f_FILE(void * _FILE);

/**
   A fsl_finalizer_f() impl which requires that mem be-a (FILE*).
   This function passes that FILE to fsl_fclose(). The state
   parameter is ignored.
*/
FSL_EXPORT void fsl_finalizer_f_FILE( void * state, void * mem );

/**
   A fsl_output_f() impl which requires state to be-a (FILE*), which
   this function passes the call on to fwrite(). Returns 0 on
   success, FSL_RC_IO on error.
*/
FSL_EXPORT int fsl_output_f_FILE( void * state, void const * src, fsl_size_t n );

/**
   A fsl_output_f() impl which requires state to be-a (fsl_buffer*),
   which this function passes to fsl_buffer_append(). Returns 0 on
   success, FSL_RC_OOM (probably) on error.
*/
FSL_EXPORT int fsl_output_f_buffer( void * state, void const * src, fsl_size_t n );

/**
   A fsl_input_f() implementation which requires that state be
   a readable (FILE*) handle.
*/
FSL_EXPORT int fsl_input_f_FILE( void * state, void * dest, fsl_size_t * n );

/**
   A fsl_input_f() implementation which requires that state be a
   readable (fsl_buffer*) handle. The buffer's cursor member is
   updated to track input postion, but that is the only
   modification made by this routine. Thus the user may need to
   reset the cursor to 0 if he wishes to start consuming the buffer
   at its starting point. Subsequent calls to this function will
   increment the cursor by the number of bytes returned via *n.
   The buffer's "used" member is used to determine the logical end
   of input.

   Returns 0 on success and has no error conditions except for
   invalid arguments, which result in undefined beavhiour. Results
   are undefined if any argument is NULL.

   Tip (and warning): sometimes a routine might have a const buffer
   handle which it would like to use in conjunction with this
   routine but cannot withou violating constness. Here's a crude
   workaround:

   @code
   fsl_buffer kludge = *originalConstBuffer; // normally this is dangerous!
   rc = some_func( fsl_input_f_buffer, &kludge, ... );
   assert(kludge.mem==originalConstBuffer->mem); // See notes below.
   // DO NOT clean up the kludge buffer. Memory belongs to the original!
   @endcode

   That is ONLY (ONLY! ONLY!! ONLY!!!) legal because this routine
   modifies only fsl_buffer::cursor. Such a workaround is STRICLY
   ILLEGAL if there is ANY CHANCE WHATSOEVER that the buffer's
   memory will be modified, in particular if it will be resized,
   and such use will eventually leak and/or corrupt memory.
*/
FSL_EXPORT int fsl_input_f_buffer( void * state, void * dest, fsl_size_t * n );


/**
   A generic streaming routine which copies data from an
   fsl_input_f() to an fsl_outpuf_f().

   Reads all data from inF() in chunks of an unspecified size and
   passes them on to outF(). It reads until inF() returns fewer
   bytes than requested. Returns the result of the last call to
   outF() or (only if reading fails) inF(). Returns FSL_RC_MISUSE
   if inF or ouF are NULL.

   Here is an example which basically does the same thing as the
   cat(1) command on Unix systems:

   @code
   fsl_stream( fsl_input_f_FILE, stdin, fsl_output_f_FILE, stdout );
   @endcode

   Or copy a FILE to a buffer:

   @code
   fsl_buffer myBuf = fsl_buffer_empty;
   rc = fsl_stream( fsl_input_f_FILE, stdin, fsl_output_f_buffer, &myBuf );
   // Note that on error myBuf might be partially populated.
   // Eventually clean up the buffer:
   fsl_buffer_clear(&myBuf);
   @endcode

*/
FSL_EXPORT int fsl_stream( fsl_input_f inF, void * inState,
                fsl_output_f outF, void * outState );

/**
   Consumes two input streams looking for differences.  It stops
   reading as soon as either or both streams run out of input or a
   byte-level difference is found.  It consumes input in chunks of
   an unspecified size, and after this returns the input cursor of
   the streams is not well-defined.  i.e. the cursor probably does
   not point to the exact position of the difference because this
   level of abstraction does not allow that unless we read byte by
   byte.

   Returns 0 if both streams emit the same amount of output and
   that ouput is bitwise identical, otherwise it returns non-0.
*/
FSL_EXPORT int fsl_stream_compare( fsl_input_f in1, void * in1State,
                        fsl_input_f in2, void * in2State );


/**
   A general-purpose buffer class, analog to Fossil v1's Blob
   class, but it is not called fsl_blob to avoid confusion with
   DB-side blobs. Buffers are used extensively in fossil to do
   everything from reading files to compressing artifacts to
   creating dynamically-formatted strings. Because they are such a
   pervasive low-level type, and have such a simple structure,
   their members (unlike most other structs in this API) may be
   considered public and used directly by client code (as long as
   they do not mangle their state, e.g. by setting this->capacity
   smaller than this->used!).

   General conventions of this class:

   - ALWAYS initialize them by copying fsl_buffer_empty or
   (depending on the context) fsl_buffer_empty_m. Failing to
   initialize them properly leads to undefined behaviour.

   - ALWAYS fsl_buffer_clear() buffers when done with
   them. Remember that failed routines which output to buffers
   might partially populate the buffer, so be sure to clean up on
   error cases.

   - The 'capacity' member specifies how much memory the buffer
   current holds in its 'mem' member.

   - The 'used' member specifies how much of the memory is actually
   "in use" by the client.

   - As a rule, the API tries to keep (used<capacity) and always
   (unless documented otherwise) tries to keep the memory buffer
   NUL-terminated (if it has any memory at all).

   - Use fsl_buffer_reuse() to keep memory around and reset the
   'used' amount to 0. Most rountines which write to buffers will
   re-use that memory if they can.

   This example demonstrates the difference between 'used' and
   'capacity' (error checking reduced to assert()ions for clarity):

   @code
   fsl_buffer b = fsl_buffer_empty;
   int rc = fsl_buffer_reserve(&b, 20);
   assert(0==rc);
   assert(b.capacity>=20); // it may reserve more!
   assert(0==b.used);
   rc = fsl_buffer_append(&b, "abc", 3);
   assert(0==rc);
   assert(3==b.used);
   assert(0==b.mem[b.used]); // API always NUL-terminates
   @endcode

   Potential TODO: add an allocator member which gets internally used
   for allocation of the buffer member. fossil(1) uses this approach,
   and swaps the allocator out as needed, to support a buffer pointing
   to memory it does not own, e.g. a slice of another buffer or to
   static memory, and then (re)allocate as necessary, e.g. to switch
   from static memory to dynamic. That may be useful in order to
   effectively port over some of the memory-intensive algos such as
   merging. That would not affect [much of] the public API, just how
   the buffer internally manages the memory. Certain API members would
   need to specify that the memory is not owned by the blob and needs
   to outlive the blob, though.

   @see fsl_buffer_reserve()
   @see fsl_buffer_append()
   @see fsl_buffer_appendf()
   @see fsl_buffer_cstr()
   @see fsl_buffer_size()
   @see fsl_buffer_capacity()
   @see fsl_buffer_clear()
   @see fsl_buffer_reuse()
*/
struct fsl_buffer {
  /**
     The raw memory owned by this buffer. It is this->capacity bytes
     long, of which this->used are considered "used" by the client.
     The difference beween (this->capacity - this->used) represents
     space the buffer has available for use before it will require
     another expansion/reallocation.
  */
  unsigned char * mem;
  /**
     Number of bytes allocated for this buffer.
  */
  fsl_size_t capacity;
  /**
     Number of "used" bytes in the buffer. This is generally
     interpreted as the string length of this->mem, and the buffer
     APIs which add data to a buffer always ensure that
     this->capacity is large enough to account for a trailing NUL
     byte in this->mem.

     Library routines which manipulate buffers must ensure that
     (this->used<=this->capacity) is always true, expanding the
     buffer if necessary. Much of the API assumes that precondition
     is always met, and any violation of it opens the code to
     undefined behaviour (which is okay, just don't ever break that
     precondition). Most APIs ensure that (used<capacity) is always
     true (as opposed to used<=capacity) because they add a
     trailing NUL byte which is not counted in the "used" length.
  */
  fsl_size_t used;

  /**
     Used by some routines to keep a cursor into this->mem.

     TODO: factor this back out and let those cases keep their own
     state. This is only used by fsl_input_f_buffer() (and that
     function cannot be implemented unless we add the cursor here
     or add another layer of state type specifically for it).

     TODO: No, don't do ^^^^. It turns out that the merge algo
     wants this as well.
  */
  fsl_size_t cursor;
};

/** Empty-initialized fsl_buffer instance, intended for const-copy
    initialization. */
#define fsl_buffer_empty_m {NULL,0U,0U,0U}

/** Empty-initialized fsl_buffer instance, intended for copy
    initialization. */
FSL_EXPORT const fsl_buffer fsl_buffer_empty;

/**
   A container for storing generic error state. It is used to
   propagate error state between layers of the API back to the
   client. i.e. they act as basic exception containers.

   @see fsl_error_set()
   @see fsl_error_get()
   @see fsl_error_move()
   @see fsl_error_clear()
*/
struct fsl_error {
  /**
     Error message text is stored in this->msg.mem. The usable text
     part is this->msg.used bytes long.
  */
  fsl_buffer msg;
  /**
     Error code, generally assumed to be a fsl_rc_e value.
  */
  int code;
};

/** Empty-initialized fsl_error instance, intended for const-copy initialization. */
#define fsl_error_empty_m {fsl_buffer_empty_m,0}

/** Empty-initialized fsl_error instance, intended for copy initialization. */
FSL_EXPORT const fsl_error fsl_error_empty;

/**
   Populates err with the given code and formatted string, replacing
   any existing state. If fmt==NULL then fsl_rc_cstr(rc) is used to
   get the error string.

   Returns code on success, some other non-0 code on error.

   As a special case, if 0==code then fmt is ignored and the error
   state is cleared. This will not free any memory held by err but
   will re-set its string to start with a NUL byte, ready for
   re-use later on.

   As a special case, if code==FSL_RC_OOM then fmt is ignored
   to avoid a memory allocation (which would presumably fail).

   @see fsl_error_get()
   @see fsl_error_clear()
   @see fsl_error_move()
*/
FSL_EXPORT int fsl_error_set( fsl_error * err, int code, char const * fmt,
                   ... );

/**
   va_list counterpart to fsl_error_set().
*/
FSL_EXPORT int fsl_error_setv( fsl_error * err, int code, char const * fmt,
                    va_list args );

/**
   Fetches the error state from err. If !err it returns
   FSL_RC_MISUSE without side-effects, else it returns err's current
   error code.

   If str is not NULL then *str will be assigned to the raw
   (NUL-terminated) error string (which might be empty or even
   NULL). The memory for the string is owned by err and may be
   invalidated by any calls which take err as a non-const parameter
   OR which might modify it indirectly through a container object,
   so the client is required to copy it if it is needed for later
   on.

   If len is not NULL then *len will be assigned to the length of
   the returned string (in bytes).

   @see fsl_error_set()
   @see fsl_error_clear()
   @see fsl_error_move()
*/
FSL_EXPORT int fsl_error_get( fsl_error const * err, char const ** str, fsl_size_t * len );

/**
   Frees up any resources owned by err and sets its error code to 0,
   but does not free err. This is harmless no-op if !err or if err
   holds no dynamically allocated no memory.

   @see fsl_error_set()
   @see fsl_error_get()
   @see fsl_error_move()
   @see fsl_error_reset()
*/
FSL_EXPORT void fsl_error_clear( fsl_error * err );

/**
   Sets err->code to 0 and resets its buffer, but keeps any
   err->msg memory around for later re-use.

   @see fsl_error_clear()
*/
FSL_EXPORT void fsl_error_reset( fsl_error * err );

/**
   Copies the error state from src to dest. If dest contains state, it is
   cleared/recycled by this operation.

   Returns 0 on success, FSL_RC_MISUSE if either argument is NULL
   or if (src==dest), and FSL_RC_OOM if allocation of the message
   string fails.

   As a special case, if src->code==FSL_RC_OOM, then the code is
   copied but the message bytes (if any) are not (under the
   assumption that we have no more memory).
*/
FSL_EXPORT int fsl_error_copy( fsl_error const * src, fsl_error * dest );

/**
   Moves the error state from one fsl_error object to
   another, intended as an allocation optimization when
   propagating error state up the API.

   This "uplifts" an error from the 'from' object to the 'to'
   object. After this returns 'to' will contain the prior error state
   of 'from' and 'from' will contain the old error message memory of
   'to'. 'from' will be re-set to the non-error state (its buffer
   memory is kept intact for later reuse, though).

   Results are undefined if either parameter is NULL or either is
   not properly initialized. i.e. neither may refer to uninitialized
   memory. Copying fsl_error_empty at declaration-time is a simple
   way to ensure that instances are cleanly initialized.
*/
FSL_EXPORT void fsl_error_move( fsl_error * from, fsl_error * to );

/**
   Returns the given Unix Epoch timestamp value as its approximate
   Julian Day value. Note that the calculation does not account for
   leap seconds.
*/
FSL_EXPORT double fsl_unix_to_julian( fsl_time_t unixEpoch );

/**
   Returns the current Unix Epoch time converted to its approximate
   Julian form. Equivalent to fsl_unix_to_julian(time(0)). See
   fsl_unix_to_julian() for details. Note that the returned time
   has seconds, not milliseconds, precision.
*/
FSL_EXPORT double fsl_julian_now();

#if 0
/** UNTESTED, possibly broken vis-a-vis timezone conversion.

    Returns the given Unix Epoch time value formatted as an ISO8601
    string.  Returns NULL on allocation error, else a string 19
    bytes long plus a terminating NUL
    (e.g. "2013-08-19T20:35:49"). The returned memory must
    eventually be freed using fsl_free().
*/
FSL_EXPORT char * fsl_unix_to_iso8601( fsl_time_t j );
#endif

/**
   Returns non-0 (true) if the first 10 digits of z _appear_ to
   form the start of an ISO date string (YYYY-MM-DD). Whether or
   not the string is really a valid date is left for downstream
   code to determine. Returns 0 (false) in all other cases,
   including if z is NULL.
*/
FSL_EXPORT char fsl_str_is_date(const char *z);


/**
   Checks if z is syntactically a time-format string in the format:

   [Y]YYYY-MM-DD

   (Yes, the year may be five-digits, left-padded with a zero for
   years less than 9999.)

   Returns a positive value if the YYYYY part has five digits, a
   negative value if it has four. It returns 0 (false) if z does not
   match that pattern.

   If it returns a negative value, the MM part of z starts at byte offset
   (z+5), and a positive value means the MM part starts at (z+6).

   z need not be NUL terminated - this function does not read past
   the first invalid byte. Thus is can be used on, e.g., full
   ISO8601-format strings. If z is NULL, 0 is returned.
*/
FSL_EXPORT int fsl_str_is_date2(const char *z);


/**
   Reserves at least n bytes of capacity in buf. Returns 0 on
   success, FSL_RC_OOM if allocation fails, FSL_RC_MISUSE if !buf.

   This does not change buf->used, nor will it shrink the buffer
   (reduce buf->capacity) unless n is 0, in which case it
   immediately frees buf->mem and sets buf->capacity and buf->used
   to 0.

   @see fsl_buffer_resize()
   @see fsl_buffer_clear()
*/
FSL_EXPORT int fsl_buffer_reserve( fsl_buffer * buf, fsl_size_t n );

/**
   Convenience equivalent of fsl_buffer_reserve(buf,0).
   This a no-op if buf==NULL.
*/
FSL_EXPORT void fsl_buffer_clear( fsl_buffer * buf );

/**
   Resets buf->used to 0 and sets buf->mem[0] (if buf->mem is not
   NULL) to 0. Does not (de)allocate memory, only changes the
   logical "used" size of the buffer. Returns its argument.

   Achtung for fossil(1) porters: this function's semantics are much
   different from the fossil's blob_reset(). To get those semantics,
   use fsl_buffer_reserve(buf, 0) or its convenience form
   fsl_buffer_clear(). (This function _used_ to be called
   fsl_buffer_reset(), but it was renamed in the hope of avoiding
   related confusion.)
*/
FSL_EXPORT fsl_buffer * fsl_buffer_reuse( fsl_buffer * buf );

/**
   Similar to fsl_buffer_reserve() except that...

   - It does not free all memory when n==0. Instead it essentially
   makes the memory a length-0, NUL-terminated string.

   - It will try to shrink (realloc) buf's memory if (n<buf->capacity).

   - It sets buf->capacity to (n+1) and buf->used to n. This routine
   allocates one extra byte to ensure that buf is always
   NUL-terminated.

   - On success it always NUL-terminates the buffer at
   offset buf->used.

   Returns 0 on success, FSL_RC_MISUSE if !buf, FSL_RC_OOM if
   (re)allocation fails.

   @see fsl_buffer_reserve()
   @see fsl_buffer_clear()
*/
FSL_EXPORT int fsl_buffer_resize( fsl_buffer * buf, fsl_size_t n );

/**
   Swaps the contents of the left and right arguments. Results are
   undefined if either argument is NULL or points to uninitialized
   memory.
*/
FSL_EXPORT void fsl_buffer_swap( fsl_buffer * left, fsl_buffer * right );

/**
   Similar fsl_buffer_swap() but it also optionally frees one of
   the buffer's memories after swapping them. If clearWhich is
   negative then the left buffer (1st arg) is cleared _after_
   swapping (i.e., the NEW left hand side gets cleared). If
   clearWhich is greater than 0 then the right buffer (2nd arg) is
   cleared _after_ swapping (i.e. the NEW right hand side gets
   cleared). If clearWhich is 0, this function behaves identically
   to fsl_buffer_swap().

   A couple examples should clear this up:

   @code
   fsl_buffer_swap_free( &b1, &b2, -1 );
   @endcode

   Swaps the contents of b1 and b2, then frees the contents
   of the left-side buffer (b1).

   @code
   fsl_buffer_swap_free( &b1, &b2, 1 );
   @endcode

   Swaps the contents of b1 and b2, then frees the contents
   of the right-side buffer (b2).
*/
FSL_EXPORT void fsl_buffer_swap_free( fsl_buffer * left, fsl_buffer * right,
                                      int clearWhich );  

/**
   Appends the first n bytes of src, plus a NUL byte, to b,
   expanding b as necessary and incrementing b->used by n. If n is
   less than 0 then the equivalent of fsl_strlen((char const*)src)
   is used to calculate the length.

   If n is 0 (or negative and !*src), this function ensures that
   b->mem is not NULL and is NUL-terminated, so it may allocate
   to have space for that NUL byte.

   src may only be NULL if n==0. If passed (src==NULL, n!=0) then
   FSL_RC_RANGE is returned.

   Returns 0 on success, FSL_RC_MISUSE if !f, !b, or !src,
   FSL_RC_OOM if allocation of memory fails.

   If this function succeeds, it guarantees that it NUL-terminates
   the buffer (but that the NUL terminator is not counted in
   b->used). 

   @see fsl_buffer_appendf()
   @see fsl_buffer_reserve()
*/
FSL_EXPORT int fsl_buffer_append( fsl_buffer * b,
                                  void const * src, fsl_int_t n );

/**
   Uses fsl_appendf() to append formatted output to the given buffer.
   Returns 0 on success and FSL_RC_OOM if an allocation fails while
   expanding dest. Results are undefined if either of the first two
   arguments are NULL.

   @see fsl_buffer_append()
   @see fsl_buffer_reserve()
*/
FSL_EXPORT int fsl_buffer_appendf( fsl_buffer * const dest,
                                   char const * fmt, ... );

/** va_list counterpart to fsl_buffer_appendf(). */
FSL_EXPORT int fsl_buffer_appendfv( fsl_buffer * const dest,
                                    char const * fmt, va_list args );

/**
   Compresses the first pIn->used bytes of pIn to pOut. It is ok for
   pIn and pOut to be the same blob.

   pOut must either be the same as pIn or else a properly
   initialized buffer. Any prior contents will be freed or their
   memory reused.

   Results are undefined if any argument is NULL.

   Returns 0 on success, FSL_RC_OOM on allocation error, and FSL_RC_ERROR
   if the lower-level compression routines fail.

   Use fsl_buffer_uncompress() to uncompress the data. The data is
   encoded with a big-endian, unsigned 32-bit length as the first four
   bytes (holding its uncomressed size), and then the data as
   compressed by zlib.

   TODO: if pOut!=pIn1 then re-use pOut's memory, if it has any.

   @see fsl_buffer_compress2()
   @see fsl_buffer_uncompress()
   @see fsl_buffer_is_compressed()
*/
FSL_EXPORT int fsl_buffer_compress(fsl_buffer const *pIn, fsl_buffer *pOut);

/**
   Compress the concatenation of a blobs pIn1 and pIn2 into pOut.

   pOut must be either empty (cleanly initialized or newly
   recycled) or must be the same as either pIn1 or pIn2.

   Results are undefined if any argument is NULL.

   Returns 0 on success, FSL_RC_OOM on allocation error, and FSL_RC_ERROR
   if the lower-level compression routines fail.

   TODO: if pOut!=(pIn1 or pIn2) then re-use its memory, if it has any.

   @see fsl_buffer_compress()
   @see fsl_buffer_uncompress()
   @see fsl_buffer_is_compressed()
*/
FSL_EXPORT int fsl_buffer_compress2(fsl_buffer const *pIn1,
                                    fsl_buffer const *pIn2,
                                    fsl_buffer *pOut);

/**
   Uncompress buffer pIn and store the result in pOut. It is ok for
   pIn and pOut to be the same buffer. Returns 0 on success. On
   error pOut is not modified.

   pOut must be either cleanly initialized/empty or the same as pIn.

   Results are undefined if any argument is NULL.

   Returns 0 on success, FSL_RC_OOM on allocation error, and
   FSL_RC_ERROR if the lower-level decompression routines fail.

   TODO: if pOut!=(pIn1 or pIn2) then re-use its memory, if it has any.

   @see fsl_buffer_compress()
   @see fsl_buffer_compress2()
   @see fsl_buffer_is_compressed()
*/
FSL_EXPORT int fsl_buffer_uncompress(fsl_buffer const *pIn, fsl_buffer *pOut);

/**
   Returns true if this function believes that mem (which must be
   at least len bytes of valid memory long) appears to have been
   compressed by fsl_buffer_compress() or equivalent. This is not a
   100% reliable check - it could potentially have false positives
   on certain inputs, but that is thought to be unlikely (at least
   for text data).

   Returns 0 if mem is NULL.
*/
FSL_EXPORT bool fsl_data_is_compressed(unsigned char const * mem, fsl_size_t len);

/**
   Equivalent to fsl_data_is_compressed(buf->mem, buf->used).
*/
FSL_EXPORT bool fsl_buffer_is_compressed(fsl_buffer const * buf);

/**
   If fsl_data_is_compressed(mem,len) returns true then this function
   returns the uncompressed size of the data, else it returns a negative
   value.
*/
FSL_EXPORT fsl_int_t fsl_data_uncompressed_size(unsigned char const *mem, fsl_size_t len);

/**
   The fsl_buffer counterpart of fsl_data_uncompressed_size().
*/
FSL_EXPORT fsl_int_t fsl_buffer_uncompressed_size(fsl_buffer const * b);

/**
   Equivalent to ((char const *)b->mem), but returns NULL if
   !b. The returned string is effectively b->used bytes long unless
   the user decides to apply his own conventions. Note that the buffer APIs
   generally assure that buffers are NUL-terminated, meaning that strings
   returned from this function can (for the vast majority of cases)
   assume that the returned string is NUL-terminated (with a string length
   of b->used _bytes_).

   @see fsl_buffer_str()
   @see fsl_buffer_cstr2()
*/
FSL_EXPORT char const * fsl_buffer_cstr(fsl_buffer const *b);

/**
   If buf is not NULL and has any memory allocated to it, that
   memory is returned. If both b and len are not NULL then *len is
   set to b->used. If b has no dynamic memory then NULL is returned
   and *len (if len is not NULL) is set to 0.

   @see fsl_buffer_str()
   @see fsl_buffer_cstr()
*/
FSL_EXPORT char const * fsl_buffer_cstr2(fsl_buffer const *b, fsl_size_t * len);

/**
   Equivalent to ((char *)b->mem). The returned memory is effectively
   b->used bytes long unless the user decides to apply their own
   conventions.
*/
FSL_EXPORT char * fsl_buffer_str(fsl_buffer const *b);
/**
   "Takes" the memory refered to by the given buffer, transfering
   ownership to the caller. After calling this, b's state will be
   empty.
*/
FSL_EXPORT char * fsl_buffer_take(fsl_buffer *b);

/**
   Returns the "used" size of b, or 0 if !b.
*/
FSL_EXPORT fsl_size_t fsl_buffer_size(fsl_buffer const * b);

/**
   Returns the current capacity of b, or 0 if !b.
*/
FSL_EXPORT fsl_size_t fsl_buffer_capacity(fsl_buffer const * b);

/**
   Compares the contents of buffers lhs and rhs using memcmp(3)
   semantics. Return negative, zero, or positive if the first
   buffer is less then, equal to, or greater than the second.
   Results are undefined if either argument is NULL.

   When buffers of different length match on the first N bytes,
   where N is the shorter of the two buffers' lengths, it treats the
   shorter buffer as being "less than" the longer one.
*/
FSL_EXPORT int fsl_buffer_compare(fsl_buffer const * lhs, fsl_buffer const * rhs);

/**
   Bitwise-compares the contents of b against the file named by
   zFile.  Returns 0 if they have the same size and contents, else
   non-zero.  This function has no way to report if zFile cannot be
   opened, and any error results in a non-0 return value. No
   interpretation/canonicalization of zFile is performed - it is
   used as-is.

   This resolves symlinks and returns non-0 if zFile refers (after
   symlink resolution) to a non-file.

   If zFile does not exist, is not readable, or has a different
   size than b->used, non-0 is returned without opening/reading the
   file contents. If a content comparison is performed, it is
   streamed in chunks of an unspecified (but relatively small)
   size, so it does not need to read the whole file into memory
   (unless it is smaller than the chunk size).
*/
FSL_EXPORT int fsl_buffer_compare_file( fsl_buffer const * b, char const * zFile );

/**
   Compare two buffers in constant (a.k.a. O(1)) time and return
   zero if they are equal.  Constant time comparison only applies
   for buffers of the same length.  If lengths are different,
   immediately returns 1. This operation is provided for cases
   where the timing/duration of fsl_buffer_compare() (or an
   equivalent memcmp()) might inadvertently leak security-relevant
   information.  Specifically, it address the concern that
   attackers can use timing differences to check for password
   misses, to narrow down an attack to passwords of a specific
   length or content properties.
*/
FSL_EXPORT int fsl_buffer_compare_O1(fsl_buffer const * lhs, fsl_buffer const * rhs);

/**
   Overwrites dest's contents with a copy of those from src
   (reusing dest's memory if it has any). Results are undefined if
   either pointer is NULL or invalid. Returns 0 on success,
   FSL_RC_OOM on allocation error.
*/
FSL_EXPORT int fsl_buffer_copy( fsl_buffer const * src, fsl_buffer * dest );


/**
   Apply the delta in pDelta to the original content pOriginal to
   generate the target content pTarget. All three pointers must point
   to properly initialized memory.

   If pTarget==pOriginal then this is a destructive operation,
   replacing the original's content with its new form.

   Return 0 on success.

   @see fsl_buffer_delta_apply()
   @see fsl_delta_apply()
   @see fsl_delta_apply2()
*/
FSL_EXPORT int fsl_buffer_delta_apply( fsl_buffer const * pOriginal,
                                       fsl_buffer const * pDelta,
                                       fsl_buffer * pTarget);

/**
   Identical to fsl_buffer_delta_apply() except that if delta
   application fails then any error messages/codes are written to
   pErr if it is not NULL. It is rare that delta application fails
   (only if the inputs are invalid, e.g. do not belong together or
   are corrupt), but when it does, having error information can be
   useful.

   @see fsl_buffer_delta_apply()
   @see fsl_delta_apply()
   @see fsl_delta_apply2()
*/
FSL_EXPORT int fsl_buffer_delta_apply2( fsl_buffer const * pOriginal,
                                        fsl_buffer const * pDelta,
                                        fsl_buffer * pTarget,
                                        fsl_error * pErr);


/**
   Uses a fsl_input_f() function to buffer input into a fsl_buffer.

   dest must be a non-NULL, initialized (though possibly empty)
   fsl_buffer object. Its contents, if any, will be overwritten by
   this function, and any memory it holds might be re-used.

   The src function is called, and passed the state parameter, to
   fetch the input. If it returns non-0, this function returns that
   error code. src() is called, possibly repeatedly, until it
   reports that there is no more data.

   Whether or not this function succeeds, dest still owns any memory
   pointed to by dest->mem, and the client must eventually free it
   by calling fsl_buffer_reserve(dest,0).

   dest->mem might (and possibly will) be (re)allocated by this
   function, so any pointers to it held from before this call might
   be invalidated by this call.

   On error non-0 is returned and dest may bge partially populated.

   Errors include:

   dest or src are NULL (FSL_RC_MISUSE)

   Allocation error (FSL_RC_OOM)

   src() returns an error code

   Whether or not the state parameter may be NULL depends on the src
   implementation requirements.

   On success dest will contain the contents read from the input
   source. dest->used will be the length of the read-in data, and
   dest->mem will point to the memory. dest->mem is automatically
   NUL-terminated if this function succeeds, but dest->used does not
   count that terminator. On error the state of dest->mem must be
   considered incomplete, and is not guaranteed to be
   NUL-terminated.

   Example usage:

   @code
   fsl_buffer buf = fsl_buffer_empty;
   int rc = fsl_buffer_fill_from( &buf,
   fsl_input_f_FILE,
   stdin );
   if( rc ){
   fprintf(stderr,"Error %d (%s) while filling buffer.\n",
   rc, fsl_rc_cstr(rc));
   fsl_buffer_reserve( &buf, 0 );
   return ...;
   }
   ... use the buf->mem ...
   ... clean up the buffer ...
   fsl_buffer_reserve( &buf, 0 );
   @endcode

   To take over ownership of the buffer's memory, do:

   @code
   void * mem = buf.mem;
   buf = fsl_buffer_empty;
   @endcode

   In which case the memory must eventually be passed to fsl_free()
   to free it.
*/
FSL_EXPORT int fsl_buffer_fill_from( fsl_buffer * dest, fsl_input_f src, void * state );

/**
   A fsl_buffer_fill_from() proxy which overwrite's dest->mem with
   the contents of the given FILE handler (which must be opened for
   read access).  Returns 0 on success, after which dest->mem
   contains dest->used bytes of content from the input source. On
   error dest may be partially filled.
*/
FSL_EXPORT int fsl_buffer_fill_from_FILE( fsl_buffer * dest, FILE * src );

/**
   A wrapper for fsl_buffer_fill_from_FILE() which gets its input
   from the given file name. It uses fsl_fopen() to open the file,
   so it supports "-" as an alias for stdin.

   Uses fsl_fopen() to open the file, so it supports the name '-'
   as an alias for stdin.
*/
FSL_EXPORT int fsl_buffer_fill_from_filename( fsl_buffer * dest, char const * filename );    

/**
   Writes the given buffer to the given filename. Returns 0 on success,
   FSL_RC_MISUSE if !b or !fname, FSL_RC_IO if opening or writing fails.

   Uses fsl_fopen() to open the file, so it supports the name '-'
   as an alias for stdout.
*/
FSL_EXPORT int fsl_buffer_to_filename( fsl_buffer const * b, char const * fname );

/**
   Copy N lines of text from pFrom into pTo. The copy begins at the
   current pFrom->cursor position. pFrom->cursor is left pointing at
   the first character past the last \\n copied. (Modification of the
   cursor is why pFrom is not const.)

   If pTo==NULL then this routine simply skips over N lines.

   Returns 0 if it copies lines or does nothing (because N is 0 or
   pFrom's contents have been exhausted). Copying fewer lines than
   requested (because of EOF) is not an error. Returns non-0 only on
   allocation error. Results are undefined if pFrom is NULL or not
   properly initialized.

   @see fsl_buffer_stream_lines()
*/
FSL_EXPORT int fsl_buffer_copy_lines(fsl_buffer *pTo, fsl_buffer *pFrom, fsl_size_t N);

/**
   Works identically to fsl_buffer_copy_lines() except that it sends
   its output to the fTo output function. If fTo is NULL then it
   simply skips over N lines.

   @see fsl_buffer_copy_lines()
*/
FSL_EXPORT int fsl_buffer_stream_lines(fsl_output_f fTo, void * toState,
                                       fsl_buffer *pFrom, fsl_size_t N);


/**
   Works like fsl_appendfv(), but appends all output to a
   dynamically-allocated string, expanding the string as necessary
   to collect all formatted data. The returned NUL-terminated string
   is owned by the caller and it must be cleaned up using
   fsl_free(...). If !fmt, NULL is returned. It is conceivable that
   it returns NULL on a zero-length formatted string, e.g.  (%.*s)
   with (0,"...") as arguments, but it will only do that if the
   whole format string resolves to empty.
*/
FSL_EXPORT char * fsl_mprintf( char const * fmt, ... );

/**
   va_list counterpart to fsl_mprintf().
*/
FSL_EXPORT char * fsl_mprintfv(char const * fmt, va_list vargs );

/**
   An sprintf(3) clone which uses fsl_appendf() for the formatting.
   Outputs at most n bytes to dest. Returns 0 on success, non-0
   on error. Returns 0 without side-effects if !n or !*fmt.

   If the destination buffer is long enough, this function
   NUL-terminates it.
*/
FSL_EXPORT int fsl_snprintf( char * dest, fsl_size_t n, char const * fmt, ... );

/**
   va_list counterpart to fsl_snprintf()
*/
FSL_EXPORT int fsl_snprintfv( char * dest, fsl_size_t n, char const * fmt, va_list args );

/**
   Equivalent to fsl_strndup(src,-1).
*/
FSL_EXPORT char * fsl_strdup( char const * src );

/**
   Similar to strndup(3) but returns NULL if !src.  The returned
   memory must eventually be passed to fsl_free(). Returns NULL on
   allocation error. If len is less than 0 and src is not NULL then
   fsl_strlen() is used to calculate its length.

   If src is not NULL but len is 0 then it will return an empty
   (length-0) string, as opposed to NULL.
*/
FSL_EXPORT char * fsl_strndup( char const * src, fsl_int_t len );

/**
   Equivalent to strlen(3) but returns 0 if src is NULL.
   Note that it counts bytes, not UTF characters.
*/
FSL_EXPORT fsl_size_t fsl_strlen( char const * src );

/**
   Like strcmp(3) except that it accepts NULL pointers.  NULL sorts
   before all non-NULL string pointers.  Also, this routine
   performs a binary comparison that does not consider locale.
*/
FSL_EXPORT int fsl_strcmp( char const * lhs, char const * rhs );

/**
   Equivalent to fsl_strcmp(), but with a signature suitable
   for use as a generic comparison function (e.g. for use with
   qsort() and search algorithms).
*/
FSL_EXPORT int fsl_strcmp_cmp( void const * lhs, void const * rhs );

/**
   Case-insensitive form of fsl_strcmp().

   @implements fsl_generic_cmp_f()
*/
FSL_EXPORT int fsl_stricmp(const char *zA, const char *zB);

/**
   Equivalent to fsl_stricmp(), but with a signature suitable
   for use as a generic comparison function (e.g. for use with
   qsort() and search algorithms).

   @implements fsl_generic_cmp_f()
*/
FSL_EXPORT int fsl_stricmp_cmp( void const * lhs, void const * rhs );

/**
   fsl_strcmp() variant which compares at most nByte bytes of the
   given strings, case-insensitively.  If nByte is less than 0 then
   fsl_strlen(zB) is used to obtain the length for comparision
   purposes.
*/
FSL_EXPORT int fsl_strnicmp(const char *zA, const char *zB, fsl_int_t nByte);

/**
   fsl_strcmp() variant which compares at most nByte bytes of the
   given strings, case-sensitively. Returns 0 if nByte is 0.
*/
FSL_EXPORT int fsl_strncmp(const char *zA, const char *zB, fsl_size_t nByte);

/**
   Equivalent to fsl_strncmp(lhs, rhs, X), where X is either
   FSL_STRLEN_SHA1 or FSL_STRLEN_K256: if both lhs and rhs are
   longer than FSL_STRLEN_SHA1 then they are assumed to be
   FSL_STRLEN_K256 bytes long and are compared as such, else they
   are assumed to be FSL_STRLEN_SHA1 bytes long and compared as
   such.

   Potential FIXME/TODO: if their lengths differ, i.e. one is v1 and
   one is v2, compare them up to their common length then, if they
   still compare equivalent, treat the shorter one as less-than the
   longer.
*/
FSL_EXPORT int fsl_uuidcmp( fsl_uuid_cstr lhs, fsl_uuid_cstr rhs );

/**
   Returns false if s is NULL or starts with any of (0 (NUL), '0'
   (ASCII character zero), 'f', 'n', "off"), case-insensitively,
   else it returns true.
*/
FSL_EXPORT bool fsl_str_bool( char const * s );

/**
   Flags for use with fsl_db_open() and friends.
*/
enum fsl_open_flags {
/**
   The "no flags" value.
*/
FSL_OPEN_F_NONE = 0,
/**
   Flag for fsl_db_open() specifying that the db should be opened
   in read-only mode.
*/
FSL_OPEN_F_RO = 0x01,
/**
   Flag for fsl_db_open() specifying that the db should be opened
   in read-write mode, but should not create the db if it does
   not already exist.
*/
FSL_OPEN_F_RW = 0x02,
/**
   Flag for fsl_db_open() specifying that the db should be opened in
   read-write mode, creating the db if it does not already exist.
*/
FSL_OPEN_F_CREATE = 0x04,
/**
   Shorthand for RW+CREATE flags.
*/
FSL_OPEN_F_RWC = FSL_OPEN_F_RW | FSL_OPEN_F_CREATE,
/**
   Tells fsl_repo_open_xxx() to confirm that the db
   is a repository.
*/
FSL_OPEN_F_SCHEMA_VALIDATE = 0x20,

/**
   Used by fsl_db_open() to to tell 1the underlying db connection to
   trace all SQL to stdout. This is often useful for testing,
   debugging, and learning about what's going on behind the scenes.
*/
FSL_OPEN_F_TRACE_SQL = 0x40
};


/**
   _Almost_ equivalent to fopen(3) but:

   - expects name to be UTF8-encoded.

   - If name=="-", it returns one of stdin or stdout, depending on
   the mode string: stdout is returned if 'w' or '+' appear,
   otherwise stdin.

   If it returns NULL, the errno "should" contain a description of
   the problem unless the problem was argument validation. Pass it
   to fsl_errno_to_rc() to convert that into an API-conventional
   error code.

   If at all possible, use fsl_close() (as opposed to fclose()) to
   close these handles, as it has logic to skip closing the
   standard streams.

   Potential TODOs:

   - extend mode string to support 'x', meaning "exclusive", analog
   to open(2)'s O_EXCL flag. Barring race conditions, we have
   enough infrastructure to implement that. (It turns out that
   glibc's fopen() supports an 'x' with exactly this meaning.)

   - extend mode to support a 't', meaning "temporary". The idea
   would be that we delete the file from the FS right after
   opening, except that Windows can't do that.
*/
FSL_EXPORT FILE * fsl_fopen(char const * name, char const *mode);

/**
   Passes f to fclose(3) unless f is NULL or one of the C-standard
   (stdin, stdout, stderr) handles, in which cases it does nothing
   at all.
*/
FSL_EXPORT void fsl_fclose(FILE * f);

/**
   This function works similarly to classical printf
   implementations, but instead of outputing somewhere specific, it
   uses a callback function to push its output somewhere. This
   allows it to be used for arbitrary external representations. It
   can be used, for example, to output to an external string, a UI
   widget, or file handle (it can also emulate printf by outputing
   to stdout this way).

   INPUTS:

   pfAppend: The is a fsl_output_f function which is responsible
   for accumulating the output. If pfAppend returns a negative
   value then processing stops immediately.

   pfAppendArg: is ignored by this function but passed as the first
   argument to pfAppend. pfAppend will presumably use it as a data
   store for accumulating its string.

   fmt: This is the format string, as in the usual printf(3), except
   that it supports more options (detailed below).

   ap: This is a pointer to a list of arguments.  Same as in
   vprintf() and friends.


   OUTPUTS:

   ACHTUNG: this interface changed significantly in 2021-09:

   - The output function was changed from a type specific to this
   interface to fsl_output_f().

   - The return semantics where changed from printf()-like to
   0 on success, non-0 on error.

   Most printf-style specifiers work as they do in standard printf()
   implementations. There might be some very minor differences, but
   the more common format specifiers work as most developers expect
   them to. In addition...

   Current (documented) printf extensions:

   (If you are NOT reading this via doxygen-processed sources: the
   percent signs below are doubled for the sake of doxygen, and
   each pair refers to only a single percent sign in the format
   string.)

   %%z works like %%s, but takes a non-const (char *) and deletes
   the string (using fsl_free()) after appending it to the output.

   %%h (HTML) works like %%s but converts certain characters (namely
   '<' and '&') to their HTML escaped equivalents.

   %%t (URL encode) works like %%s but converts certain characters
   into a representation suitable for use in an HTTP URL. (e.g. ' ' 
   gets converted to %%20)

   %%T (URL decode) does the opposite of %%t - it decodes
   URL-encoded strings and outputs their decoded form. ACHTUNG:
   fossil(1) interprets this the same as %%t except that it leaves
   '/' characters unescaped (did that change at some point? This
   code originally derived from that one some years ago!). It is
   still to be determined whether we "really need" that behaviour
   (we don't really need either one, seeing as the library is not
   CGI-centric like fossil(1) is).

   %%r requires an int and renders it in "ordinal form". That is,
   the number 1 converts to "1st" and 398 converts to "398th".

   %%q quotes a string as required for SQL. That is, '\''
   characters get doubled. It does NOT included the outer quotes
   and NULL values get replaced by the string "(NULL) (without
   quotes). See %%Q...

   %%Q works like %%q, but includes the outer '\'' characters and
   NULL pointers get output as the string literal "NULL" (without
   quotes), i.e. an SQL NULL.

   %%S works like %%.16s. It is intended for fossil hashes. The '!'
   modifier removes the length limit, resulting in the whole hash
   (making this formatting option equivalent to %%s).  (Sidebar: in
   fossil(1) this length is runtime configurable but that requires
   storing that option in global state, which is not an option for
   this implementation.)

   %%/: works like %%s but normalizes path-like strings by
   replacing backslashes with the One True Slash.

   %%b: works like %%s but takes its input from a (fsl_buffer
   const*) argument.

   %%B: works like %%Q but takes its input from a (fsl_buffer
   const*) argument.

   %%F: works like %%s but runs the output through
   fsl_bytes_fossilize().  This requires dynamic memory allocation, so
   is less efficient than re-using a client-provided buffer with
   fsl_bytes_fossilize() if the client needs to fossilize more than
   one element.

   %%j: works like %%s but JSON-encodes the string. It does not
   include the outer quotation marks by default, but using the '!'
   flag, i.e. %%!j, causes those to be added. The length and precision
   flags are NOT supported for this format. Results are undefined if
   given input which is not legal UTF8. By default non-ASCII
   characters with values less than 0xffff are emitted as as literal
   characters (no escaping), but the '#' modifier flag will cause it
   to emit such characters in the \\u#### form. It always encodes
   characters above 0xFFFF as UTF16 surrogate pairs (as JSON
   requires). Invalid UTF8 characters may get converted to '?' or may
   produce invalid JSON output. As a special case, if the value is NULL
   pointer, it resolves to "null" without quotes (regardless of the '!'
   modifier).

   Some of these extensions may be disabled by setting certain macros
   when compiling appendf.c (see that file for details).

   Potential TODO: add fsl_bytes_fossilize_out() which works like
   fsl_bytes_fossilize() but sends its output to an fsl_output_f(), so
   that this routine doesn't need to alloc for that case.
*/
FSL_EXPORT int fsl_appendfv(fsl_output_f pfAppend, void * pfAppendArg,
                            const char *fmt, va_list ap );

/**
   Identical to fsl_appendfv() but takes an ellipses list (...)
   instead of a va_list.
*/
FSL_EXPORT int fsl_appendf(fsl_output_f pfAppend,
                           void * pfAppendArg,
                           const char *fmt,
                           ... )
#if 0
/* Would be nice, but complains about our custom format options: */
  __attribute__ ((__format__ (__printf__, 3, 4)))
#endif
  ;

/**
   A fsl_output_f() impl which requires that state be an opened,
   writable (FILE*) handle.
*/
FSL_EXPORT int fsl_output_f_FILE( void * state, void const * s,
                                  fsl_size_t n );


/**
   Emulates fprintf() using fsl_appendf(). Returns the result of
   passing the data through fsl_appendf() to the given file handle.
*/
FSL_EXPORT int fsl_fprintf( FILE * fp, char const * fmt, ... );

/**
   The va_list counterpart of fsl_fprintf().
*/
FSL_EXPORT int fsl_fprintfv( FILE * fp, char const * fmt, va_list args );


/**
   Possibly reallocates self->list, changing its size. This function
   ensures that self->list has at least n entries. If n is 0 then
   the list is deallocated (but the self object is not), BUT THIS
   DOES NOT DO ANY TYPE-SPECIFIC CLEANUP of the items. If n is less
   than or equal to self->capacity then there are no side effects. If
   n is greater than self->capacity, self->list is reallocated and
   self->capacity is adjusted to be at least n (it might be bigger -
   this function may pre-allocate a larger value).

   Passing an n of 0 when self->capacity is 0 is a no-op.

   Newly-allocated slots will be initialized with NULL pointers.

   Returns 0 on success, FSL_RC_MISUSE if !self, FSL_RC_OOM if
   reservation of new elements fails.

   The return value should be used like this:

   @code
   fsl_size_t const n = number of bytes to allocate;
   int const rc = fsl_list_reserve( myList, n );
   if( rc ) { ... error ... }
   @endcode

   @see fsl_list_clear()
   @see fsl_list_visit_free()
*/
FSL_EXPORT int fsl_list_reserve( fsl_list * self, fsl_size_t n );

/**
   Appends a bitwise copy of cp to self->list, expanding the list as
   necessary and adjusting self->used.

   Ownership of cp is unchanged by this call. cp may not be NULL.

   Returns 0 on success, FSL_RC_MISUSE if any argument is NULL, or
   FSL_RC_OOM on allocation error.
*/
FSL_EXPORT int fsl_list_append( fsl_list * self, void * cp );

/**
   Swaps all contents of both lhs and rhs. Results are undefined if
   lhs or rhs are NULL or not properly initialized (via initial copy
   initialization from fsl_list_empty resp. fsl_list_empty_m).
*/
FSL_EXPORT void fsl_list_swap( fsl_list * lhs, fsl_list * rhs );

/** @typedef typedef int (*fsl_list_visitor_f)(void * p, void * visitorState )

    Generic visitor interface for fsl_list lists.  Used by
    fsl_list_visit(). p is the pointer held by that list entry and
    visitorState is the 4th argument passed to fsl_list_visit().

    Implementations must return 0 on success. Any other value causes
    looping to stop and that value to be returned, but interpration
    of the value is up to the caller (it might or might not be an
    error, depending on the context). Note that client code may use
    custom values, and is not strictly required to use FSL_RC_xxx
    values. HOWEVER...  all of the libfossil APIs which take these
    as arguments may respond differently to some codes (most notable
    FSL_RC_BREAK, which they tend to treat as a
    stop-iteration-without-error result), so clients are strongly
    encourage to return an FSL_RC_xxx value on error.
*/
typedef int (*fsl_list_visitor_f)(void * obj, void * visitorState );

/**
   A fsl_list_visitor_f() implementation which requires that obj be
   arbitrary memory which can legally be passed to fsl_free()
   (which this function does). The visitorState parameter is
   ignored.
*/
FSL_EXPORT int fsl_list_v_fsl_free(void * obj, void * visitorState );


/**
   For each item in self->list, visitor(item,visitorState) is
   called.  The item is owned by self. The visitor function MUST
   NOT free the item (unless the visitor is a finalizer!), but may
   manipulate its contents if application rules do not specify
   otherwise.

   If order is 0 or greater then the list is traversed from start
   to finish, else it is traverse from end to begin.

   Returns 0 on success, non-0 on error.

   If visitor() returns non-0 then looping stops and that code is
   returned.
*/
FSL_EXPORT int fsl_list_visit( fsl_list const * self, int order,
                               fsl_list_visitor_f visitor, void * visitorState );

/**
   A list clean-up routine which takes a callback to clean up its
   contents.

   Passes each element in the given list to
   childFinalizer(item,finalizerState). If that returns non-0,
   processing stops and that value is returned, otherwise
   fsl_list_reserve(list,0) is called and 0 is returned.

   WARNING: if cleanup fails because childFinalizer() returns non-0,
   the returned object is effectively left in an undefined state and
   the client has no way (unless the finalizer somehow accounts for it)
   to know which entries in the list were cleaned up. Thus it is highly
   recommended that finalizer functions follow the conventional wisdom
   of "destructors do not throw."

   @see fsl_list_visit_free()
*/
FSL_EXPORT int fsl_list_clear( fsl_list * list, fsl_list_visitor_f childFinalizer,
                               void * finalizerState );
/**
   Similar to fsl_list_clear(list, fsl_list_v_fsl_free, NULL), but
   only frees list->list if the second argument is true, otherwise
   it sets the list's length to 0 but keep the list->list memory
   intact for later use. Note that this function never frees the
   list argument, only its contents.

   Be sure only to use this on lists of types for which fsl_free()
   is legal. i.e. don't use it on a list of fsl_deck objects or
   other types which have their own finalizers.

   Results are undefined if list is NULL.

   @see fsl_list_clear()
*/
FSL_EXPORT void fsl_list_visit_free( fsl_list * list, bool freeListMem );

/**
   Works similarly to the visit operation without the _p suffix
   except that the pointer the visitor function gets is a (**)
   pointing back to the entry within this list. That means that
   callers can assign the entry in the list to another value during
   the traversal process (e.g. set it to 0). If shiftIfNulled is
   true then if the callback sets the list's value to 0 then it is
   removed from the list and self->used is adjusted (self->capacity
   is not changed).
*/
FSL_EXPORT int fsl_list_visit_p( fsl_list * self, int order, bool shiftIfNulled,
                                 fsl_list_visitor_f visitor, void * visitorState );


/**
   Sorts the given list using the given comparison function. Neither
   argument may be NULL. The arugments passed to the comparison function
   will be pointers to pointers to the original entries, and may (depending
   on how the list is used) point to NULL.
*/
FSL_EXPORT void fsl_list_sort( fsl_list * li, fsl_generic_cmp_f cmp);

/**
   Searches for a value in the given list, using the given
   comparison function to determine equivalence. The comparison
   function will never be passed a NULL value by this function - if
   value is NULL then only a NULL entry will compare equal to it.
   Results are undefined if li or cmpf are NULL.

   Returns the index in li of the entry, or a negative value if no
   match is found.
*/
FSL_EXPORT fsl_int_t fsl_list_index_of( fsl_list const * li,
                                        void const * value,
                                        fsl_generic_cmp_f cmpf);

/**
   Equivalent to fsl_list_index_of(li, key, fsl_strcmp_cmp).
*/
FSL_EXPORT fsl_int_t fsl_list_index_of_cstr( fsl_list const * li,
                                             char const * key );


/**
   Returns 0 if the given file is readable. Flags may be any values
   accepted by the access(2) resp. _waccess() system calls.
*/
FSL_EXPORT int fsl_file_access(const char *zFilename, int flags);

/**
   Computes a canonical pathname for a file or directory. Makes the
   name absolute if it is relative. Removes redundant / characters.
   Removes all /./ path elements. Converts /A/../ to just /. If the
   slash parameter is non-zero, the trailing slash, if any, is
   retained.

   If zRoot is not NULL then it is used for transforming a relative
   zOrigName into an absolute path. If zRoot is NULL fsl_getcwd()
   is used to determine the virtual root directory. If zRoot is
   empty (starts with a NUL byte) then this function effectively
   just sends zOrigName through fsl_file_simplify_name().

   Returns 0 on success, FSL_RC_MISUSE if !zOrigName or !pOut,
   FSL_RC_OOM if an allocation fails.

   pOut, if not NULL, is _appended_ to, so be sure to set pOut->used=0
   (or pass it to fsl_buffer_reuse()) before calling this to start
   writing at the beginning of a re-used buffer. On error pOut might
   conceivably be partially populated, but that is highly
   unlikely. Nonetheless, be sure to fsl_buffer_clear() it at some
   point regardless of success or failure.

   This function does no actual filesystem-level processing unless
   zRoot is NULL or empty (and then only to get the current
   directory). This does not confirm whether the resulting file
   exists, nor that it is strictly a valid filename for the current
   filesystem. It simply transforms a potentially relative path
   into an absolute one.

   Example:
   @code
   int rc;
   char const * zRoot = "/a/b/c";
   char const * zName = "../foo.bar";
   fsl_buffer buf = fsl_buffer_empty;
   rc = fsl_file_canonical_name2(zRoot, zName, &buf, 0);
   if(rc){
     fsl_buffer_clear(&buf);
     return rc;
   }
   assert(0 == fsl_strcmp( "/a/b/foo.bar, fsl_buffer_cstr(&buf)));
   fsl_buffer_clear(&buf);
   @endcode
*/
FSL_EXPORT int fsl_file_canonical_name2(const char *zRoot,
                                        const char *zOrigName,
                                        fsl_buffer *pOut, bool slash);

/**
   Equivalent to fsl_file_canonical_name2(NULL, zOrigName, pOut, slash).

   @see fsl_file_canonical_name2()
*/

FSL_EXPORT int fsl_file_canonical_name(const char *zOrigName,
                                       fsl_buffer *pOut, bool slash);

/**
   Calculates the "directory part" of zFilename and _appends_ it to
   pOut. The directory part is all parts up to the final path
   separator ('\\' or '/'). If leaveSlash is true (non-0) then the
   separator part is appended to pOut, otherwise it is not. This
   function only examines the first nLen bytes of zFilename.  If
   nLen is negative then fsl_strlen() is used to determine the
   number of bytes to examine.

   If zFilename ends with a slash then it is considered to be its
   own directory part. i.e.  the dirpart of "foo/" evaluates to
   "foo" (or "foo/" if leaveSlash is true), whereas the dirpart of
   "foo" resolves to nothing (empty - no output except a NUL
   terminator sent to pOut).

   Returns 0 on success, FSL_RC_MISUSE if !zFilename or !pOut,
   FSL_RC_RANGE if 0==nLen or !*zFilename, and FSL_RC_OOM if
   appending to pOut fails. If zFilename contains only a path
   separator and leaveSlash is false then only a NUL terminator is
   appended to pOut if it is not already NUL-terminated.

   This function does no filesystem-level validation of the the
   given path - only string evaluation.
*/
FSL_EXPORT int fsl_file_dirpart(char const * zFilename, fsl_int_t nLen,
                                fsl_buffer * pOut, bool leaveSlash);


/**
   Writes the absolute path name of the current directory to zBuf,
   which must be at least nBuf bytes long (nBuf includes the space
   for a trailing NUL terminator).

   Returns FSL_RC_RANGE if the name would be too long for nBuf,
   FSL_RC_IO if it cannot determine the current directory (e.g. a
   side effect of having removed the directory at runtime or similar
   things), and 0 on success.

   On success, if outLen is not NULL then the length of the string
   written to zBuf is assigned to *outLen. The output string is
   always NUL-terminated.

   On Windows, the name is converted from unicode to UTF8 and all '\\'
   characters are converted to '/'.  No conversions are needed on
   Unix.
*/
FSL_EXPORT int fsl_getcwd(char *zBuf, fsl_size_t nBuf, fsl_size_t * outLen);


/**
   Return true if the filename given is a valid filename
   for a file in a repository. Valid filenames follow all of the
   following rules:

   -  Does not begin with "/"
   -  Does not contain any path element named "." or ".."
   -  Does not contain "/..." (special case)
   -  Does not contain any of these characters in the path: "\"
   -  Does not end with "/".
   -  Does not contain two or more "/" characters in a row.
   -  Contains at least one character

   Invalid UTF8 characters result in a false return if bStrictUtf8 is
   true.  If bStrictUtf8 is false, invalid UTF8 characters are silently
   ignored. See https://en.wikipedia.org/wiki/UTF-8#Invalid_byte_sequences
   and https://en.wikipedia.org/wiki/Unicode (for the noncharacters).

   Fossil compatibility note: the bStrictUtf8 flag must be true
   when parsing new manifests but is false when parsing legacy
   manifests, for backwards compatibility.

   z must be NUL terminated. Results are undefined if !z.

   Note that periods in and of themselves are valid filename
   components, with the special exceptions of "." and "..", one
   implication being that "...." is, for purposes of this function,
   a valid simple filename.
*/
FSL_EXPORT bool fsl_is_simple_pathname(const char *z, bool bStrictUtf8);

/**
   Return the size of a file in bytes. Returns -1 if the file does
   not exist or is not stat(2)able.
*/
FSL_EXPORT fsl_int_t fsl_file_size(const char *zFilename);

/**
   Return the modification time for a file.  Return -1 if the file
   does not exist or is not stat(2)able.
*/
FSL_EXPORT fsl_time_t fsl_file_mtime(const char *zFilename);

#if 0
/**
   Don't use this. The wd (working directory) family of functions
   might or might-not be necessary and in any case they require
   a fsl_cx context argument because they require repo-specific
   "allow-symlinks" setting.

   Return TRUE if the named file is an ordinary file or symlink
   and symlinks are allowed.

   Return false for directories, devices, fifos, etc.
*/
FSL_EXPORT bool fsl_wd_isfile_or_link(const char *zFilename);
#endif

/**
   Returns true if the named file is an ordinary file. Returns false
   for directories, devices, fifos, symlinks, etc. The name
   may be absolute or relative to the current working dir.
*/
FSL_EXPORT bool fsl_is_file(const char *zFilename);

/**
   Returns true if the given file is a symlink, else false. The name
   may be absolute or relative to the current working dir.
*/
FSL_EXPORT bool fsl_is_symlink(const char *zFilename);

/**
   Returns true if the given filename refers to a plain file or
   symlink, else returns false. The name may be absolute or relative
   to the current working dir.
*/
FSL_EXPORT bool fsl_is_file_or_link(const char *zFilename);

/**
   Returns true if the given path appears to be absolute, else
   false. On Unix a path is absolute if it starts with a '/'.  On
   Windows a path is also absolute if it starts with a letter, a
   colon, and either a backslash or forward slash.
*/
FSL_EXPORT bool fsl_is_absolute_path(const char *zPath);

/**
   Simplify a filename by:

   * converting all \ into / on windows and cygwin
   * removing any trailing and duplicate /
   * removing /./
   * removing /A/../

   Changes are made in-place.  Return the new name length.  If the
   slash parameter is non-zero, the trailing slash, if any, is
   retained. If n is <0 then fsl_strlen() is used to calculate the
   length.
*/
FSL_EXPORT fsl_size_t fsl_file_simplify_name(char *z, fsl_int_t n_, bool slash);

/**
   Return true (non-zero) if string z matches glob pattern zGlob
   and zero if the pattern does not match. Always returns 0 if
   either argument is NULL. Supports all globbing rules
   supported by sqlite3_strglob().
*/
FSL_EXPORT bool fsl_str_glob(const char *zGlob, const char *z);

/**
   Parses zPatternList as a comma-and/or-fsl_isspace()-delimited
   list of glob patterns (as supported by fsl_str_glob()). Each
   pattern in that list is copied and appended to tgt in the form
   of a new (char *) owned by that list.

   Returns 0 on success, FSL_RC_OOM if copying a pattern to tgt
   fails, FSL_RC_MISUSE if !tgt or !zPatternList. An empty
   zPatternList is not considered an error (to simplify usage) and
   has no side-effects. On allocation error, tgt might be partially
   populated.

   Elements of the glob list may be optionally enclosed in single
   or double-quotes.  This allows a comma to be part of a glob
   pattern. 

   Leading and trailing spaces on unquoted glob patterns are
   ignored.

   Note that there is no separate "glob list" class. A "glob list"
   is simply a fsl_list whose list entries are glob-pattern strings
   owned by that list.

   Examples of a legal value for zPatternList:
   @code
   "*.c *.h, *.sh, '*.in'"
   @endcode

   @see fsl_glob_list_append()
   @see fsl_glob_list_matches()
   @see fsl_glob_list_clear()
*/
FSL_EXPORT int fsl_glob_list_parse( fsl_list * tgt, char const * zPatternList );

/**
   Appends a single blob pattern to tgt, in the form of a new (char *)
   owned by tgt. This function copies zGlob and appends that copy
   to tgt.

   Returns 0 on success, FSL_RC_MISUSE if !tgt or !zGlob or
   !*zGlob, FSL_RC_OOM if appending to the list fails.

   @see fsl_glob_list_parse()
   @see fsl_glob_list_matches()
   @see fsl_glob_list_clear()
*/
FSL_EXPORT int fsl_glob_list_append( fsl_list * tgt, char const * zGlob );

/**
   Assumes globList is a list of (char [const] *) glob values and
   tries to match each one against zHaystack using
   fsl_str_glob(). If any glob matches, it returns a pointer to the
   matched globList->list entry. If no matches are found, or if any
   argument is invalid, NULL is returned.

   The returned bytes are owned by globList and may be invalidated at
   its leisure. It is primarily intended to be used as a boolean,
   for example:

   @code
   if( fsl_glob_list_matches(myGlobs, someFilename) ) { ... }
   @endcode

   @see fsl_glob_list_parse()
   @see fsl_glob_list_append()
   @see fsl_glob_list_clear()
*/
FSL_EXPORT char const * fsl_glob_list_matches( fsl_list const * globList, char const * zHaystack );

/**
   If globList is not NULL this is equivalent to
   fsl_list_visit_free(globList, 1), otherwise it is a no-op.

   Note that this does not free the globList object itself, just
   its underlying list entries and list memory. (In practice, lists
   are either allocated on the stack or as part of a higher-level
   structure, and not on the heap.)

   @see fsl_glob_list_parse()
   @see fsl_glob_list_append()
   @see fsl_glob_list_matches()
*/
FSL_EXPORT void fsl_glob_list_clear( fsl_list * globList );


/**
   Returns true if the given letter is an ASCII alphabet character.
*/
FSL_EXPORT char fsl_isalpha(int c);

/**
   Returns true if c is a lower-case ASCII alphabet character.
*/
FSL_EXPORT char fsl_islower(int c);

/**
   Returns true if c is an upper-case ASCII alphabet character.
*/
FSL_EXPORT char fsl_isupper(int c);

/**
   Returns true if c is ' ', '\\r' (ASCII 13dec), or '\\t' (ASCII 9
   dec).
*/
FSL_EXPORT char fsl_isspace(int c);

/**
   Returns true if c is an ASCII digit in the range '0' to '9'.
*/
FSL_EXPORT char fsl_isdigit(int c);

/**
   Equivalent to fsl_isdigit(c) || fsl_isalpha().
*/
FSL_EXPORT char fsl_isalnum(int c);

/**
   Returns the upper-case form of c if c is an ASCII alphabet
   letter, else returns c.
*/
FSL_EXPORT int fsl_tolower(int c);

/**
   Returns the lower-case form of c if c is an ASCII alphabet
   letter, else returns c.
*/
FSL_EXPORT int fsl_toupper(int c);

#ifdef _WIN32
/**
   Translate MBCS to UTF-8.  Return a pointer to the translated
   text. ACHTUNG: Call fsl_mbcs_free() (not fsl_free()) to
   deallocate any memory used to store the returned pointer when
   done.
*/
FSL_EXPORT char * fsl_mbcs_to_utf8(char const * mbcs);

/**
   Frees a string allocated from fsl_mbcs_to_utf8(). Results are undefined
   if mbcs was allocated using any other mechanism.
*/
FSL_EXPORT void fsl_mbcs_free(char * mbcs);
#endif
/* _WIN32 */

/**
   Deallocates the given memory, which must have been allocated
   from fsl_unicode_to_utf8(), fsl_utf8_to_unicode(), or any
   function which explicitly documents this function as being the
   proper finalizer for its returned memory.
*/
FSL_EXPORT void fsl_unicode_free(void *);


/**
   Translate UTF-8 to Unicode for use in system calls. Returns a
   pointer to the translated text. The returned value must
   eventually be passed to fsl_unicode_free() to deallocate any
   memory used to store the returned pointer when done.

   This function exists only for Windows. On other platforms
   it behaves like fsl_strdup().

   The returned type is (wchar_t*) on Windows and (char*)
   everywhere else.
*/
FSL_EXPORT void *fsl_utf8_to_unicode(const char *zUtf8);

/**
   Translates Unicode text into UTF-8.  Return a pointer to the
   translated text. Call fsl_unicode_free() to deallocate any
   memory used to store the returned pointer when done.

   This function exists only for Windows. On other platforms it
   behaves like fsl_strdup().
*/
FSL_EXPORT char *fsl_unicode_to_utf8(const void *zUnicode);

/**
   Translate text from the OS's character set into UTF-8. Return a
   pointer to the translated text. Call fsl_filename_free() to
   deallocate any memory used to store the returned pointer when
   done.

   This function must not convert '\' to '/' on Windows/Cygwin, as
   it is used in places where we are not sure it's really filenames
   we are handling, e.g. fsl_getenv() or handling the argv
   arguments from main().

   On Windows, translate some characters in the in the range
   U+F001 - U+F07F (private use area) to ASCII. Cygwin sometimes
   generates such filenames. See:
   <https://cygwin.com/cygwin-ug-net/using-specialnames.html>
*/
FSL_EXPORT char *fsl_filename_to_utf8(const void *zFilename);

/**
   Translate text from UTF-8 to the OS's filename character set.
   Return a pointer to the translated text. Call
   fsl_filename_free() to deallocate any memory used to store the
   returned pointer when done.

   On Windows, characters in the range U+0001 to U+0031 and the
   characters '"', '*', ':', '<', '>', '?' and '|' are invalid in
   filenames. Therefore, translate those to characters in the in the
   range U+F001 - U+F07F (private use area), so those characters
   never arrive in any Windows API. The filenames might look
   strange in Windows explorer, but in the cygwin shell everything
   looks as expected.

   See: <https://cygwin.com/cygwin-ug-net/using-specialnames.html>

   The returned type is (wchar_t*) on Windows and (char*)
   everywhere else.
*/
FSL_EXPORT void *fsl_utf8_to_filename(const char *zUtf8);


/**
   Deallocate pOld, which must have been allocated by
   fsl_filename_to_utf8(), fsl_utf8_to_filename(), fsl_getenv(), or
   another routine which explicitly documents this function as
   being the proper finalizer for its returned memory.
*/
FSL_EXPORT void fsl_filename_free(void *pOld);

/**
   Returns a (possible) copy of the environment variable with the
   given key, or NULL if no entry is found. The returned value must
   be passed to fsl_filename_free() to free it. ACHTUNG: DO NOT
   MODIFY the returned value - on Unix systems it is _not_ a
   copy. That interal API inconsistency "should" be resolved
   (==return a copy from here, but that means doing it everywhere)
   to avoid memory ownership problems later on.

   Why return a copy? Because native strings from at least one of
   the more widespread OSes often have to be converted to something
   portable and this requires allocation on such platforms, but
   not on Unix. For API transparency, that means all platforms get
   the copy(-like) behaviour.
*/
FSL_EXPORT char *fsl_getenv(const char *zName);

/**
   Returns a positive value if zFilename is a directory, 0 if
   zFilename does not exist, or a negative value if zFilename
   exists but is something other than a directory. Results are
   undefined if zFilename is NULL.

   This function expects zFilename to be a well-formed path -
   it performs no normalization on it.
*/
FSL_EXPORT int fsl_dir_check(const char *zFilename);

/**
   Check the given path to determine whether it is an empty directory.
   Returns 0 on success (i.e., directory is empty), <0 if the provided
   path is not a directory or cannot be opened, and >0 if the
   directory is not empty.
*/
FSL_EXPORT int fsl_dir_is_empty(const char *path);

/**
   Deletes the given file from the filesystem. Returns 0 on
   success. If the component is a directory, this operation will
   fail. If zFilename refers to a symlink, the link (not its target)
   is removed.

   Results are undefined if zFilename is NULL.

   Potential TODO: if it refers to a dir, forward the call to
   fsl_rmdir().
*/
FSL_EXPORT int fsl_file_unlink(const char *zFilename);


/**
   Renames zFrom to zTo, as per rename(3). Returns 0 on success.  On
   error it returns FSL_RC_OOM on allocation error when converting the
   names to platforms-specific character sets (on platforms where that
   happens) or an FSL_RC_xxx value approximating an errno value, as
   per fsl_errno_to_rc().
*/
FSL_EXPORT int fsl_file_rename(const char *zFrom, const char *zTo);

/**
   Deletes an empty directory from the filesystem. Returns 0
   on success. There are any number of reasons why deletion
   of a directory can fail, some of which include:

   - It is not empty or permission denied (FSL_RC_ACCESS).

   - Not found (FSL_RC_NOT_FOUND).

   - Is not a directory or (on Windows) is a weird pseudo-dir type for
     which rmdir() does not work (FSL_RC_TYPE).

   - I/O error (FSL_RC_IO).

   @see fsl_dir_is_empty()
*/
FSL_EXPORT int fsl_rmdir(const char *zFilename);

/**
   Sets the mtime (Unix epoch) for a file. Returns 0 on success,
   non-0 on error. If newMTime is less than 0 then the current
   time(2) is used. This routine does not create non-existent files
   (e.g. like a Unix "touch" command).
*/
FSL_EXPORT int fsl_file_mtime_set(const char *zFilename, fsl_time_t newMTime);

/**
   On non-Windows platforms, this function sets or unsets the
   executable bits on the given filename. All other permissions are
   retained as-is. Returns 0 on success. On Windows this is a no-op,
   returning 0.

   If the target is a directory or a symlink, this is a no-op and
   returns 0.
*/
FSL_EXPORT int fsl_file_exec_set(const char *zFilename, bool isExec);


/**
   Create the directory with the given name if it does not already
   exist. If forceFlag is true, delete any prior non-directory
   object with the same name.

   Return 0 on success, non-0 on error.

   If the directory already exists then 0 is returned, not an error
   (FSL_RC_ALREADY_EXISTS), because that simplifies usage. If
   another filesystem entry with this name exists and forceFlag is
   true then that entry is deleted before creating the directory,
   and this operation fails if deletion fails. If forceFlag is
   false and a non-directory entry already exists, FSL_RC_TYPE is
   returned.

   For recursively creating directories, use fsl_mkdir_for_file().

   Bug/corner case: if zFilename refers to a symlink to a
   non-existent directory, this function gets slightly confused,
   tries to make a dir with the symlink's name, and returns
   FSL_RC_ALREADY_EXISTS. How best to resolve that is not yet
   clear. The problem is that stat(2)ing the symlink says "nothing
   is there" (because the link points to a non-existing thing), so
   we move on to the underlying mkdir(), which then fails because
   the link exists with that name.
*/
FSL_EXPORT int fsl_mkdir(const char *zName, bool forceFlag);

/**
   A convenience form of fsl_mkdir() which can recursively create
   directories. If zName has a trailing slash then the last
   component is assumed to be a directory part, otherwise it is
   assumed to be a file part (and no directory is created for that
   part). zName may be either an absolute or relative path.

   Returns 0 on success (including if all directories already exist).
   Returns FSL_RC_OOM if there is an allocation error. Returns
   FSL_RC_TYPE if one of the path components already exists and is not
   a directory. Returns FSL_RC_RANGE if zName is NULL or empty. If
   zName is only 1 byte long, this is a no-op.

   On systems which support symlinks, a link to a directory is
   considered to be a directory for purposes of this function.

   If forceFlag is true and a non-directory component is found in
   the filesystem where zName calls for a directory, that component
   is removed (and this function fails if removal fails).

   Examples:

   "/foo/bar" creates (if needed) /foo, but assumes "bar" is a file
   component. "/foo/bar/" creates /foo/bar. However "foo" will not
   create a directory - because the string has no path component,
   it is assumed to be a filename.

   Both "/foo/bar/my.doc" and "/foo/bar/" result in the directories
   /foo/bar.

*/
FSL_EXPORT int fsl_mkdir_for_file(char const *zName, bool forceFlag);


/**
   Uses fsl_getenv() to look for the environment variables
   (FOSSIL_USER, (Windows: USERNAME), (Unix: USER, LOGNAME)). If
   it finds one it returns a copy of that value, which must
   eventually be passed to fsl_free() to free it (NOT
   fsl_filename_free(), though fsl_getenv() requires that one). If
   it finds no match, or if copying the entry fails, it returns
   NULL.

   @see fsl_cx_user_set()
   @see fsl_cx_user_get()
*/
FSL_EXPORT char * fsl_guess_user_name();

/**
   Tries to find the user's home directory. If found, 0 is
   returned, tgt's memory is _overwritten_ (not appended) with the
   path, and tgt->used is set to the path's string length.  (Design
   note: the overwrite behaviour is inconsistent with most of the
   API, but the implementation currently requires this.)

   If requireWriteAccess is true then the directory is checked for
   write access, and FSL_RC_ACCESS is returned if that check
   fails. For historical (possibly techinical?) reasons, this check
   is only performed on Unix platforms. On others this argument is
   ignored. When writing code on Windows, it may be necessary to
   assume that write access is necessary on non-Windows platform,
   and to pass 1 for the second argument even though it is ignored
   on Windows.

   On error non-0 is returned and tgt is updated with an error
   string OR (if the error was an allocation error while appending
   to the path or allocating MBCS strings for Windows), it returns
   FSL_RC_OOM and tgt "might" be updated with a partial path (up to
   the allocation error), and "might" be empty (if the allocation
   error happens early on).

   This routine does not canonicalize/transform the home directory
   path provided by the environment, other than to convert the
   string byte encoding on some platforms. i.e. if the environment
   says that the home directory is "../" then this function will
   return that value, possibly to the eventual disappointment of
   the caller.

   Result codes include:

   - FSL_RC_OK (0) means a home directory was found and tgt is
   populated with its path.

   - FSL_RC_NOT_FOUND means the home directory (platform-specific)
   could not be found.

   - FSL_RC_ACCESS if the home directory is not writable and
   requireWriteAccess is true. Unix platforms only -
   requireWriteAccess is ignored on others.

   - FSL_RC_TYPE if the home (as determined via inspection of the
   environment) is not a directory.

   - FSL_RC_OOM if a memory (re)allocation fails.
*/
FSL_EXPORT int fsl_find_home_dir( fsl_buffer * tgt, bool requireWriteAccess );

/**
   Values for use with the fsl_fstat::type field.
*/
enum fsl_fstat_type_e {
/** Sentinel value for unknown/invalid filesystem entry types. */
FSL_FSTAT_TYPE_UNKNOWN = 0,
/** Indicates a directory filesystem entry. */
FSL_FSTAT_TYPE_DIR,
/** Indicates a non-directory, non-symlink filesystem entry.
    Because fossil's scope is limited to SCM work, it assumes that
    "special files" (sockets, etc.) are just files, and makes no
    special effort to handle them.
*/
FSL_FSTAT_TYPE_FILE,
/** Indicates a symlink filesystem entry. */
FSL_FSTAT_TYPE_LINK
};
typedef enum fsl_fstat_type_e fsl_fstat_type_e;

/**
   Bitmask values for use with the fsl_fstat::perms field.

   Only permissions which are relevant for fossil are listed here.
   e.g. read-vs-write modes are irrelevant for fossil as it does not
   track them. It manages only the is-executable bit. In in the
   contexts of fossil manifests, it also treats "is a symlink" as a
   permission flag.
*/
enum fsl_fstat_perm_e {
/**
   Sentinel value.
*/
FSL_FSTAT_PERM_UNKNOWN = 0,
/**
   The executable bit, as understood by Fossil. Fossil does not
   differentiate between different +x values for user/group/other.
*/
FSL_FSTAT_PERM_EXE = 0x01
};
typedef enum fsl_fstat_perm_e fsl_fstat_perm_e;

/**
   A simple wrapper around the stat(2) structure resp. _stat/_wstat
   (on Windows). It exposes only the aspects of stat(2) info which
   Fossil works with, and not any platform-/filesystem-specific
   details except the executable bit for the permissions mode and some
   handling of symlinks.
*/
struct fsl_fstat {
  /**
     Indicates the type of filesystem object.
  */
  fsl_fstat_type_e type;
  /**
     The time of the last file metadata change (owner, permissions,
     etc.). The man pages (neither for Linux nor Windows) do not
     specify exactly what unit this is. Let's assume seconds since the
     start of the Unix Epoch.
  */
  fsl_time_t ctime;
  /**
     Last modification time.
  */
  fsl_time_t mtime;
  /**
     The size of the stat'd file, in bytes.
  */
  fsl_size_t size;
  /**
     Contains the filesystem entry's permissions as a bitmask of
     fsl_fstat_perm_e values. Note that only the executable bit for
     _files_ (not directories) is exposed here.
  */
  int perm;
};

/** Empty-initialized fsl_fstat structure, intended for const-copy
    construction. */
#define fsl_fstat_empty_m {FSL_FSTAT_TYPE_UNKNOWN,0,0,-1,0}

/** Empty-initialized fsl_fstat instance, intended for non-const copy
    construction. */
FSL_EXPORT const fsl_fstat fsl_fstat_empty;

/**
   Runs the OS's stat(2) equivalent to populate fst with
   information about the given file.

   Returns 0 on success, FSL_RC_MISUSE if zFilename is NULL, and
   FSL_RC_RANGE if zFilename starts with a NUL byte. Returns
   FSL_RC_NOT_FOUND if no filesystem entry is found for the given
   name. Returns FSL_RC_IO if the underlying stat() (or equivalent)
   fails for undetermined reasons inside the underlying
   stat()/_wstati64() call. Note that the fst parameter may be
   NULL, in which case the return value will be 0 if the name is
   stat-able, but will return no other information about it.

   The derefSymlinks argument is ignored on non-Unix platforms.  On
   Unix platforms, if derefSymlinks is true then stat(2) is used, else
   lstat(2) (if available on the platform) is used. For most cases
   clients should pass true. They should only pass false if they need
   to differentiate between symlinks and files.

   The fsl_fstat_type_e family of flags can be used to determine the
   type of the filesystem object being stat()'d (file, directory, or
   symlink). It does not apply any special logic for platform-specific
   oddities other than symlinks (e.g. character devices and such).
*/
FSL_EXPORT int fsl_stat(const char *zFilename, fsl_fstat * fst,
                        bool derefSymlinks);

/**
   Create a new delta between the memory zIn and zOut.

   The delta is written into a preallocated buffer, zDelta, which
   must be at least 60 bytes longer than the target memory, zOut.
   The delta string will be NUL-terminated, but it might also
   contain embedded NUL characters if either the zSrc or zOut files
   are binary.

   On success this function returns 0 and the length of the delta
   string, in bytes, excluding the final NUL terminator character,
   is written to *deltaSize.

   Returns FSL_RC_MISUSE if any of the pointer arguments are NULL
   and FSL_RC_OOM if memory allocation fails during generation of
   the delta. Returns FSL_RC_RANGE if lenSrc or lenOut are "too
   big" (if they cause an overflow in the math).

   Output Format:

   The delta begins with a base64 number followed by a newline.
   This number is the number of bytes in the TARGET file.  Thus,
   given a delta file z, a program can compute the size of the
   output file simply by reading the first line and decoding the
   base-64 number found there.  The fsl_delta_applied_size()
   routine does exactly this.

   After the initial size number, the delta consists of a series of
   literal text segments and commands to copy from the SOURCE file.
   A copy command looks like this:

   (Achtung: extra backslashes are for Doxygen's benefit - not
   visible in the processsed docs.)

   NNN\@MMM,

   where NNN is the number of bytes to be copied and MMM is the
   offset into the source file of the first byte (both base-64).
   If NNN is 0 it means copy the rest of the input file.  Literal
   text is like this:

   NNN:TTTTT

   where NNN is the number of bytes of text (base-64) and TTTTT is
   the text.

   The last term is of the form

   NNN;

   In this case, NNN is a 32-bit bigendian checksum of the output
   file that can be used to verify that the delta applied
   correctly.  All numbers are in base-64.

   Pure text files generate a pure text delta.  Binary files
   generate a delta that may contain some binary data.

   Algorithm:

   The encoder first builds a hash table to help it find matching
   patterns in the source file.  16-byte chunks of the source file
   sampled at evenly spaced intervals are used to populate the hash
   table.

   Next we begin scanning the target file using a sliding 16-byte
   window.  The hash of the 16-byte window in the target is used to
   search for a matching section in the source file.  When a match
   is found, a copy command is added to the delta.  An effort is
   made to extend the matching section to regions that come before
   and after the 16-byte hash window.  A copy command is only
   issued if the result would use less space that just quoting the
   text literally. Literal text is added to the delta for sections
   that do not match or which can not be encoded efficiently using
   copy commands.

   @see fsl_delta_applied_size()
   @see fsl_delta_apply()
*/
FSL_EXPORT int fsl_delta_create( unsigned char const *zSrc, fsl_size_t lenSrc,
                      unsigned char const *zOut, fsl_size_t lenOut,
                      unsigned char *zDelta, fsl_size_t * deltaSize);

/**
   Works identically to fsl_delta_create() but sends its output to
   the given output function. out(outState,...) may be called any
   number of times to emit delta output. Each time it is called it
   should append the new bytes to its output channel.

   The semantics of the return value and the first four arguments
   are identical to fsl_delta_create(), with these ammendments
   regarding the return value:

   - Returns FSL_RC_MISUSE if any of (zSrc, zOut, out) are NULL.

   - If out() returns non-0 at any time, delta generation is
   aborted and that code is returned.

   Example usage:

   @code
   int rc = fsl_delta_create( v1, v1len, v2, v2len,
   fsl_output_f_FILE, stdout);
   @endcode
*/
FSL_EXPORT int fsl_delta_create2( unsigned char const *zSrc, fsl_size_t lenSrc,
                       unsigned char const *zOut, fsl_size_t lenOut,
                       fsl_output_f out, void * outState);

/**
   A fsl_delta_create() wrapper which uses the first two arguments
   as the original and "new" content versions to delta, and outputs
   the delta to the 3rd argument (overwriting any existing contents
   and re-using any memory it had allocated).

   If the output buffer (delta) is the same as src or newVers,
   FSL_RC_MISUSE is returned, and results are undefined if delta
   indirectly refers to the same buffer as either src or newVers.

   Returns 0 on success.
*/
FSL_EXPORT int fsl_buffer_delta_create( fsl_buffer const * src,
                             fsl_buffer const * newVers,
                             fsl_buffer * delta);

/**
   Apply a delta created using fsl_delta_create().

   The output buffer must be big enough to hold the whole output
   file and a NUL terminator at the end. The
   fsl_delta_applied_size() routine can be used to determine that
   size.

   zSrc represents the original sources to apply the delta to.
   It must be at least lenSrc bytes of valid memory.

   zDelta holds the delta (created using fsl_delta_create()),
   and it must be lenDelta bytes long.

   On success this function returns 0 and writes the applied delta
   to zOut.

   Returns FSL_RC_MISUSE if any pointer argument is NULL. Returns
   FSL_RC_RANGE if lenSrc or lenDelta are "too big" (if they cause
   an overflow in the math). Invalid delta input can cause any of
   FSL_RC_RANGE, FSL_RC_DELTA_INVALID_TERMINATOR,
   FSL_RC_CHECKSUM_MISMATCH, FSL_RC_SIZE_MISMATCH, or
   FSL_RC_DELTA_INVALID_OPERATOR to be returned.

   Refer to the fsl_delta_create() documentation above for a
   description of the delta file format.

   @see fsl_delta_applied_size()
   @see fsl_delta_create()
   @see fsl_delta_apply2()
*/
FSL_EXPORT int fsl_delta_apply( unsigned char const *zSrc, fsl_size_t lenSrc,
                     unsigned char const *zDelta, fsl_size_t lenDelta,
                     unsigned char *zOut );

/**
   Functionally identical to fsl_delta_apply() but any errors generated
   during application of the delta are described in more detail
   in pErr. If pErr is NULL this behaves exactly as documented for
   fsl_delta_apply().
*/
FSL_EXPORT int fsl_delta_apply2( unsigned char const *zSrc,
                      fsl_size_t lenSrc,
                      unsigned char const *zDelta,
                      fsl_size_t lenDelta,
                      unsigned char *zOut,
                      fsl_error * pErr);
/*
  Calculates the size (in bytes) of the output from applying a the
  given delta. On success 0 is returned and *appliedSize will be
  updated with the amount of memory required for applying the
  delta. zDelta must point to lenDelta bytes of memory in the
  format emitted by fsl_delta_create(). It is legal for appliedSize
  to point to the same memory as the 2nd argument.

  Returns FSL_RC_MISUSE if any pointer argument is NULL. Returns
  FSL_RC_RANGE if lenDelta is too short to be a delta. Returns
  FSL_RC_DELTA_INVALID_TERMINATOR if the delta's encoded length
  is not properly terminated.

  This routine is provided so that an procedure that is able to
  call fsl_delta_apply() can learn how much space is required for
  the output and hence allocate nor more space that is really
  needed.

  TODO?: consolidate 2nd and 3rd parameters into one i/o parameter?

  @see fsl_delta_apply()
  @see fsl_delta_create()
*/
FSL_EXPORT int fsl_delta_applied_size(unsigned char const *zDelta,
                           fsl_size_t lenDelta,
                           fsl_size_t * appliedSize);

/**
   "Fossilizes" the first len bytes of the given input string. If
   (len<0) then fsl_strlen(inp) is used to calculate its length.
   The output is appended to out, which is expanded as needed and
   out->used is updated accordingly.  Returns 0 on success,
   FSL_RC_MISUSE if !inp or !out. Returns 0 without side-effects if
   0==len or (!*inp && len<0). Returns FSL_RC_OOM if reservation of
   the output buffer fails (it is expanded, at most, one time by
   this function).

   Fossilization replaces the following bytes/sequences with the
   listed replacements:

   (Achtung: usage of doubled backslashes here it just to please
   doxygen - they will show up as single slashes in the processed
   output.)

   - Backslashes are doubled.

   - (\\n, \\r, \\v, \\t, \\f) are replaced with \\\\X, where X is the
   conventional encoding letter for that escape sequence.

   - Spaces are replaced with \\s.

   - Embedded NULs are replaced by \\0 (numeric 0, not character
   '0').
*/
FSL_EXPORT int fsl_bytes_fossilize( unsigned char const * inp, fsl_int_t len,
                         fsl_buffer * out );
/**
   "Defossilizes" bytes encoded by fsl_bytes_fossilize() in-place.
   inp must be a string encoded by fsl_bytes_fossilize(), and the
   decoding processes stops at the first unescaped NUL terminator.
   It has no error conditions except for !inp or if inp is not
   NUL-terminated, both of which invoke in undefined behaviour.

   If resultLen is not NULL then *resultLen is set to the resulting string
   length.

*/
FSL_EXPORT void fsl_bytes_defossilize( unsigned char * inp, fsl_size_t * resultLen );

/**
   Defossilizes the contents of b. Equivalent to:
   fsl_bytes_defossilize( b->mem, &b->used );
*/
FSL_EXPORT void fsl_buffer_defossilize( fsl_buffer * b );

/**
   Returns true if the input string contains only valid lower-case
   base-16 digits. If any invalid characters appear in the string,
   false is returned.
*/
FSL_EXPORT bool fsl_validate16(const char *zIn, fsl_size_t nIn);

/**
   The input string is a base16 value.  Convert it into its canonical
   form.  This means that digits are all lower case and that conversions
   like "l"->"1" and "O"->"0" occur.
*/
FSL_EXPORT void fsl_canonical16(char *z, fsl_size_t n);

/**
   Decode a N-character base-16 number into base-256.  N must be a 
   multiple of 2. The output buffer must be at least N/2 characters
   in length. Returns 0 on success.
*/
FSL_EXPORT int fsl_decode16(const unsigned char *zIn, unsigned char *pOut, fsl_size_t N);

/**
   Encode a N-digit base-256 in base-16. N is the byte length of pIn
   and zOut must be at least (N*2+1) bytes long (the extra is for a
   terminating NUL). Returns zero on success, FSL_RC_MISUSE if !pIn
   or !zOut.
*/
FSL_EXPORT int fsl_encode16(const unsigned char *pIn, unsigned char *zOut, fsl_size_t N);


/**
   Tries to convert the value of errNo, which is assumed to come
   from the global errno, to a fsl_rc_e code. If it can, it returns
   something approximating the errno value, else it returns dflt.

   Example usage:

   @code
   FILE * f = fsl_fopen("...", "...");
   int rc = f ? 0 : fsl_errno_to_rc(errno, FSL_RC_IO);
   ...
   @endcode

   Why require the caller to pass in errno, instead of accessing it
   directly from this function? To avoid the the off-chance that
   something changes errno between the call and the conversion
   (whether or not that's possible is as yet undetermined). It can
   also be used by clients to map to explicit errno values to
   fsl_rc_e values, e.g. fsl_errno_to_rc(EROFS,-1) returns
   FSL_RC_ACCESS.

   A list of the errno-to-fossil conversions:

   - EINVAL: FSL_RC_MISUSE (could arguably be FSL_RC_RANGE, though)

   - ENOMEM: FSL_RC_OOM

   - EACCES, EBUSY, EPERM: FSL_RC_ACCESS

   - EISDIR, ENOTDIR: FSL_RC_TYPE

   - ENAMETOOLONG, ELOOP: FSL_RC_RANGE

   - ENOENT: FSL_RC_NOT_FOUND

   - EEXIST: FSL_RC_ALREADY_EXISTS

   - EIO: FSL_RC_IO

   Any other value for errNo causes dflt to be returned.

*/
FSL_EXPORT int fsl_errno_to_rc(int errNo, int dflt);

/**
   Make the given string safe for HTML by converting every "<" into
   "&lt;", every ">" into "&gt;", every "&" into "&amp;", and
   encode '"' as &quot; so that it can appear as an argument to
   markup.

   The escaped output is send to out(oState,...).

   Returns 0 on success or if there is nothing to do (input has a
   length of 0, in which case out() is not called). Returns
   FSL_RC_MISUSE if !out or !zIn. If out() returns a non-0 code
   then that value is returned to the caller.

   If n is negative, fsl_strlen() is used to calculate zIn's length.
*/
FSL_EXPORT int fsl_htmlize(fsl_output_f out, void * oState,
                const char *zIn, fsl_int_t n);

/**
   Functionally equivalent to fsl_htmlize() but optimized to perform
   only a single allocation.

   Returns 0 on success or if there is nothing to do (input has a
   length of 0). Returns FSL_RC_MISUSE if !p or !zIn, and
   FSL_RC_OOM on allocation error.

   If n is negative, fsl_strlen() is used to calculate zIn's length.
*/
FSL_EXPORT int fsl_htmlize_to_buffer(fsl_buffer *p, const char *zIn, fsl_int_t n);

/**
   Equivalent to fsl_htmlize_to_buffer() but returns the result as a
   new string which must eventually be fsl_free()d by the caller.

   Returns NULL for invalid arguments or allocation error.
*/
FSL_EXPORT char *fsl_htmlize_str(const char *zIn, fsl_int_t n);

/**
   If c is a character Fossil likes to HTML-escape, assigns *xlate
   to its transformed form, else set it to NULL. Returns 1 for
   untransformed characters and the strlen of *xlate for others.
   Bytes returned via xlate are static and immutable.

   Results are undefined if xlate is NULL.
*/
FSL_EXPORT fsl_size_t fsl_htmlize_xlate(int c, char const ** xlate);

/**
   Flags for use with text-diff generation APIs,
   e.g. fsl_diff_text().

   Maintenance reminder: these values are holy and must not be
   changed without also changing the corresponding code in
   diff.c.
*/
enum fsl_diff_flag_e {
/** Ignore end-of-line whitespace */
FSL_DIFF_IGNORE_EOLWS = 0x01,
/** Ignore end-of-line whitespace */
FSL_DIFF_IGNORE_ALLWS = 0x03,
/** Generate a side-by-side diff */
FSL_DIFF_SIDEBYSIDE =   0x04,
/** Missing shown as empty files */
FSL_DIFF_VERBOSE =      0x08,
/** Show filenames only. Not used in this impl! */
FSL_DIFF_BRIEF =        0x10,
/** Render HTML. */
FSL_DIFF_HTML =         0x20,
/** Show line numbers. */
FSL_DIFF_LINENO =       0x40,
/** Suppress optimizations (debug). */
FSL_DIFF_NOOPT =        0x0100,
/** Invert the diff (debug). */
FSL_DIFF_INVERT =       0x0200,
/** Only display if not "too big." */
FSL_DIFF_NOTTOOBIG =    0x0800,
/** Strip trailing CR */
FSL_DIFF_STRIP_EOLCR =    0x1000,
/**
   This flag tells text-mode diff generation to add ANSI color
   sequences to some output.  The colors are currently hard-coded
   and non-configurable. This has no effect for HTML output, and
   that flag trumps this one. It also currently only affects
   unified diffs, not side-by-side.

   Maintenance reminder: this one currently has no counterpart in
   fossil(1), is not tracked in the same way, and need not map to an
   internal flag value.
*/
FSL_DIFF_ANSI_COLOR =     0x2000
};

/**
   Generates a textual diff from two text inputs and writes
   it to the given output function.

   pA and pB are the buffers to diff.

   contextLines is the number of lines of context to output. This
   parameter has a built-in limit of 2^16, and values larger than
   that get truncated. A value of 0 is legal, in which case no
   surrounding context is provided. A negative value translates to
   some unspecified default value.

   sbsWidth specifies the width (in characters) of the side-by-side
   columns. If sbsWidth is not 0 then this function behaves as if
   diffFlags contains the FSL_DIFF_SIDEBYSIDE flag. If sbsWidth is
   negative, OR if diffFlags explicitly contains
   FSL_DIFF_SIDEBYSIDE and sbsWidth is 0, then some default width
   is used. This parameter has a built-in limit of 255, and values
   larger than that get truncated to 255.

   diffFlags is a mask of fsl_diff_flag_t values. Not all of the
   fsl_diff_flag_t flags are yet [sup]ported.

   The output is sent to out(outState,...). If out() returns non-0
   during processing, processing stops and that result is returned
   to the caller of this function.

   Returns 0 on success, FSL_RC_OOM on allocation error,
   FSL_RC_MISUSE if any arguments are invalid, FSL_RC_TYPE if any
   of the content appears to be binary (contains embedded NUL
   bytes), FSL_RC_RANGE if some range is exceeded (e.g. the maximum
   number of input lines).

   None of (pA, pB, out) may be NULL.

   TODOs:

   - Add a predicate function for outputing only matching
   differences, analog to fossil(1)'s regex support (but more
   flexible).

   - Expose the raw diff-generation bits via the internal API
   to facilitate/enable the creation of custom diff formats.

   @see fsl_diff_v2()
*/
FSL_EXPORT int fsl_diff_text(fsl_buffer const *pA, fsl_buffer const *pB,
                             fsl_output_f out, void * outState,
                             short contextLines, short sbsWidth,
                             int diffFlags );

/**
   Functionally equivalent to:

   @code:
   fsl_diff_text(pA, pB, fsl_output_f_buffer, pOut,
   contextLines, sbsWidth, diffFlags);
   @endcode

   Except that it returns FSL_RC_MISUSE if !pOut.
*/
FSL_EXPORT int fsl_diff_text_to_buffer(fsl_buffer const *pA, fsl_buffer const *pB,
                                       fsl_buffer *pOut, short contextLines,
                                       short sbsWidth, int diffFlags );

/**
   Flags for use with the 2021-era text-diff generation APIs
   (fsl_diff_builder and friends). This set of flags may still change
   considerably.

   Maintenance reminder: these values are holy and must not be
   changed without also changing the corresponding code in
   diff2.c.
*/
enum fsl_diff2_flag_e {
/** Ignore end-of-line whitespace. Applies to
    all diff builders. */
FSL_DIFF2_IGNORE_EOLWS = 0x01,
/** Ignore end-of-line whitespace. Applies to
    all diff builders.  */
FSL_DIFF2_IGNORE_ALLWS = 0x03,
/** Suppress optimizations (debug). Applies to
    all diff builders. */
FSL_DIFF2_NOOPT =        0x0100,
/** Invert the diff. Applies to all diff builders. */
FSL_DIFF2_INVERT =       0x0200,
/** Use context line count even if it's zero. Applies to all diff
    builders. Normally a value of 0 is treated as the built-in
    default. */
FSL_DIFF2_CONTEXT_ZERO =  0x0400,
/** Only calculate diff if it's not "too big." Applies to all diff
    builders and will cause the public APIs which hit this to return
    FSL_RC_RANGE.  */
FSL_DIFF2_NOTTOOBIG =    0x0800,
/** Strip trailing CR before diffing. Applies to all diff builders. */
FSL_DIFF2_STRIP_EOLCR =    0x1000,
/** More precise but slower side-by-side diff algorithm, for diffs
    which use that. */
FSL_DIFF2_SLOW_SBS =       0x2000,
/**
   Tells diff builders which support it to include line numbers in
   their output.
*/
FSL_DIFF2_LINE_NUMBERS = 0x10000,
/**
   Tells diff builders which optionally support an "index" line
   to NOT include it in their output.
*/
FSL_DIFF2_NOINDEX = 0x20000,

/**
   Tells the TCL diff builder that the complete output and each line
   should be wrapped in {...}.
*/
FSL_DIFF2_TCL_BRACES = 0x40000
};

/**
   UNDER CONSTRUCTION: part of an ongoing porting effort.

   An instance of this class is used to convey certain state to
   fsl_diff_builder objects. Some of this state is configuration
   provided by the client and some is persistent state used during the
   diff generation process.

   Certain fsl_diff_builder implementations may require that some
   ostensibly optional fields be filled out. Documenting that is TODO,
   as the builders get developed.
*/
struct fsl_diff_opt {
  /**
     Flags from the fsl_diff2_flag_e enum.
  */
  uint32_t diffFlags;
  /**
     Number of lines of diff context (number of lines common to the
     LHS and RHS of the diff). Library-wide default is 5.
  */
  unsigned short nContext;
  /**
     Column width for side-by-side, a.k.a. split, diffs.
  */
  short columnWidth;
  /**
     Number of files seen in the diff so far (incremented by routines
     which delegate to fsl_diff_builder instances).
  */
  uint32_t fileCount;
  /**
     The filename of the object represented by the LHS of the
     diff. This is intentended for, e.g., generating header-style
     output. This
     may be NULL.
  */
  const char * nameLHS;
  /**
     The hash of the object represented by the LHS of the diff. This
     is intentended for, e.g., generating header-style output. This
     may be NULL.
  */
  const char * hashLHS;
  /**
     The filename of the object represented by the LHS of the
     diff. This is intentended for, e.g., generating header-style
     output. If this is NULL but nameLHS is not then they are assumed
     to have the same name.
  */
  const char * nameRHS;
  /**
     The hash of the object represented by the RHS of the diff. This
     is intentended for, e.g., generating header-style output. This
     may be NULL.
  */
  const char * hashRHS;
  /**
     Output destination. Any given builder might, depending on how it
     actually constructs the diff, buffer output and delay calling
     this until its finish() method is called.
  */
  fsl_output_f out;
  /**
     State for this->out(). Ownership is unspecified by this
     interface: it is for use by this->out() but what is supposed to
     happen to it after this object is done with it depends on
     higher-level code.
  */
  void * outState;
};

/** Convenience typedef. */
typedef struct fsl_diff_opt fsl_diff_opt;

/** Initialized-with-defaults fsl_diff_opt structure, intended for
    const-copy initialization. */
#define fsl_diff_opt_empty_m {\
  0/*diffFlags*/, 5/*nContext*/, 0/*columnWidth*/,\
  0/*fileCount*/, NULL/*hashLHS*/, NULL/*hashRHS*/,\
  NULL/*out*/, NULL/*outState*/ \
}

/** Initialized-with-defaults fsl_diff_opt structure, intended for
    non-const copy initialization. */
extern const fsl_diff_opt fsl_diff_opt_empty;

/**
   UNDER CONSTRUCTION: part of an ongoing porting effort.

   Information about each line of a file being diffed.

   This type is only in the public API for use by the fsl_diff_builder
   interface. It is not otherwise intended for public use.
*/
struct fsl_dline {
  /**  The text of the line. Owned by higher-level code. */
  const char *z;
  /** Hash of the line. Lower X bits are the length. */
  unsigned int h;
  /** Indent of the line. Only !=0 with certain options */
  unsigned short indent;
  /** number of bytes */
  unsigned short n;
  /** 1+(Index of next line with same the same hash) */
  unsigned int iNext;
  /**
     an array of fsl_dline elements serves two purposes.  The fields
     above are one per line of input text.  But each entry is also
     a bucket in a hash table, as follows: */
  unsigned int iHash;   /* 1+(first entry in the hash chain) */
};

/**
   Convenience typedef.
*/
typedef struct fsl_dline fsl_dline;
/** Initialized-with-defaults fsl_dline structure, intended for
    const-copy initialization. */
#define fsl_dline_empty_m {NULL,0U,0U,0U,0U,0U}
/** Initialized-with-defaults fsl_dline structure, intended for
    non-const copy initialization. */
extern const fsl_dline fsl_dline_empty;

/**
   Maximum number of change spans for fsl_dline_change.
*/
#define fsl_dline_change_max_spans 8

/**
   This "mostly-internal" type describes zero or more (up to
   fsl_dline_change_max_spans) areas of difference between two lines
   of text. This type is only in the public API for use with concrete
   fsl_diff_builder implementations.
*/
struct fsl_dline_change {
  /** Number of change spans (number of used elements in this->a). */
  unsigned char n;
  /** Array of change spans, in left-to-right order */
  struct fsl_dline_change_span {
    /* Reminder: int instead of uint b/c some ported-in algos use
       negatives. */
    /** Byte offset to start of a change on the left */
    int iStart1;
    /** Length of the left change in bytes */
    int iLen1;
    /** Byte offset to start of a change on the right */
    int iStart2;
    /** Length of the change on the right in bytes */
    int iLen2;
    /** True if this change is known to have no useful subdivs */
    int isMin;
  } a[fsl_dline_change_max_spans];
};

/**
   Convenience typedef.
*/
typedef struct fsl_dline_change fsl_dline_change;

/** Initialized-with-defaults fsl_dline_change structure, intended for
    const-copy initialization. */
#define fsl_dline_change_empty_m { \
  0, {                                                \
    {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, \
    {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0} \
  } \
}

/** Initialized-with-defaults fsl_dline_change structure, intended for
    non-const copy initialization. */
extern const fsl_dline_change fsl_dline_change_empty;

/**
   Given two lines of a diff, this routine computes a set of changes
   between those lines for display purposes and writes a description
   of those changes into the 3rd argument. After returning, p->n
   contains the number of elements in p->a which were populated by
   this routine.

   This function is only in the public API for use with
   fsl_diff_builder objects. It is not a requirement for such objects
   but can be used to provide more detailed diff changes than marking
   whole lines as simply changed or not.
*/
FSL_EXPORT void fsl_dline_change_spans(const fsl_dline *pLeft,
                                       const fsl_dline *pRight,
                                       fsl_dline_change * const p);

/**
   Compares two fsl_dline instances using memcmp() semantics,
   returning 0 if they are equivalent.

   @see fsl_dline_cmp_ignore_ws()
*/
FSL_EXPORT int fsl_dline_cmp(const fsl_dline * const pA,
                             const fsl_dline * const pB);

/**
   Counterpart of fsl_dline_cmp() but ignores all whitespace
   when comparing for equivalence.
*/
FSL_EXPORT int fsl_dline_cmp_ignore_ws(const fsl_dline * const pA,
                                       const fsl_dline * const pB);

/**
   Breaks the first n bytes of z into an array of fsl_dline records,
   each of which refers back to z (so it must remain valid for their
   lifetime). If n is negative, fsl_strlen() is used to calculate
   z's length.

   The final argument may be any flags from the fsl_diff2_flag_e
   enum, but only the following flags are honored:

   - FSL_DIFF2_STRIP_EOLCR
   - FSL_DIFF2_IGNORE_EOLWS
   - FSL_DIFF2_IGNORE_ALLWS

   On success, returns 0, assigns *pnLine to the number of lines, and
   sets *pOut to the array of fsl_dline objects, transfering ownership
   to the caller, who must eventually pass it to fsl_free() to free
   it.

   On error, neither *pnLine nor *pOut are modified and returns one
   of:

   - FSL_RC_DIFF_BINARY if the input appears to be non-text.
   - FSL_RC_OOM on allocation error.
*/
FSL_EXPORT int fsl_break_into_dlines(const char *z, fsl_int_t n,
                                     uint32_t *pnLine,
                                     fsl_dline **pOut, uint64_t diff2Flags);

/** Convenience typedef. */
typedef struct fsl_diff_builder fsl_diff_builder;

/**
   UNDER CONSTRUCTION: part of an ongoing porting effort to
   include the 2021-09 fossil diff-rendering improvements.

   A diff builder is an object responsible for formatting low-level
   diff info another form, typically for human readability but also
   for machine readability (patches).

   The internal APIs which drive each instance of this class guaranty
   that if any method of this class returns non-0 (an error code) then
   no futher methods will be called except for finalize().

   TODOs:

   - The current API does not allow these objects to be re-used: each
   one is responsible for a single diff run, and that's it. We "should"
   add a reset() method to each. The workflow would look something like:

   1. Client creates builder.
   2. Pass it to the diff builder driver (this library).
   3. builder->start()
   4. ... various methods...
   5. builder->finish()
   6. Return to caller, who may then call builder->reset(), tweak
      builder->cfg as needed for next set of inputs, and return to
      step 2 for the next file.
   7. builder->finalize()
*/
struct fsl_diff_builder {
  /** Config info, owned by higher-level routines. Every diff builder
      requires one of these. */
  fsl_diff_opt * cfg;
  /**
     If not NULL, this is called once per diff to give the builder a
     chance to perform any bootstrapping initialization or header
     output. At the point this is called, this->cfg is assumed to have
     been filled out properly. Diff builder implementations which
     require dynamic resource allocation may perform it here or in
     their factory routine(s).

     This method should also reset any dynamic state of a builder so
     that it may be reused for subsequent diffs. This enables the API
     to use a single builder for a collection of logically grouped
     files without having to destroy and reallocate the builder.

     Must return 0 on success, non-0 on error. If it returns non-0,
     the only other method of the instance which may be legally called
     is finalize().
  */
  int (*start)(fsl_diff_builder* const);

  /**
     If this is not NULL, it is called one time at the start of each
     chunk of diff for a given file and is passed the line number of
     each half of the diff and the number of lines in that chunk for
     that half (including insertions and deletions). This is primarily
     intended for generating conventional unified diff chunk headers
     in the form:

     @code
     @@ -A,B +C,D @@
     @endcode

     The inclusion of this method in an object might preclude certain
     other diff formatting changes which might otherwise apply.
     Notably, if the span between two diff chunks is smaller than the
     context lines count, the diff builder driver prefers to merge
     those two chunks together. That "readability optimization" is
     skipped when this method is set because this method may otherwise
     report that lines are being skipped which then subsequently get
     output by the driver.

     Must return 0 on success, non-0 on error.
  */
  int (*chunkHeader)(fsl_diff_builder* const,
                     uint32_t A, uint32_t B,
                     uint32_t C, uint32_t D);

  /**
     Tells the builder that n lines of common output are to be
     skipped. How it represents this is up to the impl. If the 3rd
     argument is true, this block represents the final part of the
     diff. Must return 0 on success, non-0 on error.
  */
  int (*skip)(fsl_diff_builder* const, uint32_t n, bool isFinal);
  /**
     Tells the builder that the given line represents one line of
     common output. Must return 0 on success, non-0 on error.
  */
  int (*common)(fsl_diff_builder* const, fsl_dline const * line);
  /**
     Tells the builder that the given line represents an "insert" into
     the RHS. Must return 0 on success, non-0 on error.
  */
  int (*insertion)(fsl_diff_builder* const, fsl_dline const * line);
  /**
     Tells the builder that the given line represents a "deletion" - a
     line removed from the LHS. Must return 0 on success, non-0 on
     error.
  */
  int (*deletion)(fsl_diff_builder* const, fsl_dline const * line);
  /**
     Tells the builder that the given line represents a replacement
     from the LHS to the RHS. Must return 0 on success, non-0 on
     error.
  */
  int (*replacement)(fsl_diff_builder* const, fsl_dline const * lineLhs,
                     fsl_dline const * lineRhs);
  /**
     Tells the builder that the given line represents an "edit" from
     the LHS to the RHS. Must return 0 on success, non-0 on
     error. Builds are free, syntax permitting, to usse the
     fsl_dline_change_spans() API to elaborate on edits for display
     purposes or to the treat this as a single pair of calls to
     this->deletion() an dthis->insertion(). In the latter case they
     simply need to pass lineLhs to this->deletion() and lineRhs to
     this->insertion().
  */
  int (*edit)(fsl_diff_builder* const, fsl_dline const * lineLhs,
              fsl_dline const * lineRhs);
  /**
     Must "finish" the diff process. Depending on the diff impl, this
     might flush any pending output or may be a no-op. This is only
     called if the rest of the diff was generated without producing an
     error result.

     This member may be NULL.

     Any given implementation is free to collect its output in an
     internal representation and delay flushing it until this routine
     is called.

     Must return 0 on success, non-0 on error (e.g. output flushing
     fails).
  */
  int (*finish)(fsl_diff_builder* const);
  /**
     Must free any state owned by this builder, including the builder
     object. It must not generate any output.

     Must return 0 on success, non-0 on error (e.g. output flushing
     fails), but must clean up its own state and the builder object in
     either case.
  */
  void (*finalize)(fsl_diff_builder* const);
  /** Impl-specific diff-generation state. If it is owned by this
      instance then this->finalize() must clean it up. */
  void * pimpl;
  /** Impl-specific int for tracking basic output state, e.g.  of
      opening/closing tags. This must not be modified by clients. */
  unsigned int implFlags;
  /* Trivia: a cursory examination of the diff builders in fossil(1)
     suggests that we cannot move the management of
     lnLHS/lnRHS from the concrete impls into the main API. */
  /** Number of lines seen of the LHS content. It is up to
      the concrete builder impl to update this if it's needed. */
  uint32_t lnLHS;
  /** Number of lines seen of the RHS content. It is up to
      the concrete builder impl to update this if it's needed. */
  uint32_t lnRHS;
};

/** Initialized-with-defaults fsl_diff_builder structure, intended for
    const-copy initialization. */
#define fsl_diff_builder_empty_m { \
  NULL/*cfg*/,                                                        \
  NULL/*start()*/,NULL/*chunkHeader()*/,NULL/*skip()*/, NULL/*common()*/, \
  NULL/*insertion()*/,NULL/*deletion()*/, NULL/*replacement()*/, \
  NULL/*edit()*/, NULL/*finish()*/, NULL/*finalize()*/,        \
  NULL/*pimpl*/, 0U/*implFlags*/,\
  0/*lnLHS*/,0/*lnRHS*/ \
}

/** Initialized-with-defaults fsl_diff_builder structure, intended for
    non-const copy initialization. */
extern const fsl_diff_builder fsl_diff_builder_empty;

/**
   Type IDs for use with fsl_diff_builder_factory().
*/
enum fsl_diff_builder_e {
/**
   A "dummy" diff builder intended only for testing the
   fsl_diff_builder interface and related APIs. It does not produce
   output which is generically useful.
*/
FSL_DIFF_BUILDER_DEBUG = 1,
/**
   Generates diffs in a compact low(ist)-level form originally
   designed for use by diff renderers implemented in JavaScript.

   This diff builder outputs a JSON object with the following
   properties:

   - hashLHS, hashRHS: the hashes of the LHS/RHS content.

   - nameLHS, nameRHS: the filenames of the LHS/RHS. By convention, if
     the RHS is NULL but the LHS is not, both sides have the same
     name.

   - diff: raw diff content, an array with the structure described
     below.

   Note that it is legal for the names and hashes to be "falsy" (null,
   not set, or empty strings).

   The JSON array consists of integer opcodes with each opcode
   followed by zero or more arguments:

   @code
   Syntax        Mnemonic    Description

   -----------   --------    --------------------------
   0             END         This is the end of the diff.
   1  INTEGER    SKIP        Skip N lines from both files.
   2  STRING     COMMON      The line STRING is in both files.
   3  STRING     INSERT      The line STRING is in only the right file.
   4  STRING     DELETE      The line STRING is in only the left file.
   5  SUBARRAY   EDIT        One line is different on left and right.
   @endcode

   The SUBARRAY is an array of 3*N+1 strings with N>=0.  The triples
   represent common-text, left-text, and right-text.  The last string
   in SUBARRAY is the common-suffix.  Any string can be empty if it
   does not apply.
*/
FSL_DIFF_BUILDER_JSON1,

/**
   A diff builder which produces output compatible with the patch(1)
   command. Its output is functionally identical to fossil(1)'s
   default diff output except that by default includes an Index line
   at the top of each file (use the FSL_DIFF2_NOINDEX flag in its
   fsl_diff_opt::diffFlags to disable that).

   This diff builder supports the FSL_DIFF2_LINE_NUMBERS flag.
*/
FSL_DIFF_BUILDER_UNIFIED_TEXT,

/**
   A diff builder which outputs a description of the diff in a
   TCL-readable form. It requires external TCL code in order to
   function.

   TODO: a flag which includes the tcl/tk script as part of the
   output. We first need to compile fossil's diff.tcl into the
   library.
*/
FSL_DIFF_BUILDER_TCL,
//! NYI
FSL_DIFF_BUILDER_SBS_TEXT
};
typedef enum fsl_diff_builder_e fsl_diff_builder_e;

/**
   A factory for creating fsl_diff_builder instances of types which
   are built in to the library. This does not preclude the creation of
   client-side diff builders (e.g. ones which write to ncurses widgets
   or similar special-case output).

   On success, returns 0 and assigns *pOut to a new builder instance
   which must eventually be freed by calling its pOut->finalize()
   method. On error, returns non-0 and *pOut is not modified. Error
   codes include FSL_RC_OOM (alloc failed) and FSL_RC_TYPE (unknown
   type ID), FSL_RC_TYPE (type is not (or not yet) implemented).
*/
FSL_EXPORT int fsl_diff_builder_factory( fsl_diff_builder_e type,
                                         fsl_diff_builder **pOut );

/**
   This counterpart of fsl_diff_text() defines its output format in
   terms of a fsl_diff_builder instance which the caller must provide.
   The caller is responsible for pointing pBuilder->cfg to a
   configuration object suitable for the desired diff. In particular,
   pBuilder->cfg->out and (if necessary) pBuilder->cfg->outState must
   be set to non-NULL values.

   This function generates a low-level diff of two versions of content,
   contained in the given buffers, and passes that diff through the
   given diff builder to format it.

   Returns 0 on success. On error, it is not generally knowable whether
   or not any diff output was generated.

   The builder may produce any error codes it wishes, in which case
   they are propagated back to the caller. Common error codes include:

   - FSL_RC_OOM if an allocation fails.

   - FSL_RC_RANGE if the diff is "too big" and
   pBuilder->config->diffFlags contains the FSL_DIFF2_NOTTOOBIG flag.

   - FSL_RC_DIFF_BINARY if the to-diff content appears to be binary,
   noting that "appears to be" is heuristric-driven and subject to
   false positives. Specifically, files with extremely long lines will
   be recognized as binary (and are, in any case, generally less than
   useful for most graphical diff purposes).

   @see fsl_diff_builder_factory()
   @see fsl_diff_text()
   @see fsl_diff_raw_v2()
*/
FSL_EXPORT int fsl_diff_v2(fsl_buffer const * pv1,
                           fsl_buffer const * pv2,
                           fsl_diff_builder * const pBuilder);

/**
   Performs a diff, as for fsl_diff_v2(), but returns the results in
   the form of an array of COPY, DELETE, INSERT triples terminated by
   3 entries with the value 0.

   Each triple in the list specifies how many *lines* of each half of
   the diff (the first 2 arguments to this function) to COPY as-is
   (common code), DELETE (exists in the LHS but not in the RHS), and
   INSERT (exists in the RHS but not in the LHS). By breaking the
   input into lines and following these values, a line-level text-mode
   diff of the two blobs can be generated.

   See fsl_diff_v2() for the details, all of which apply except for
   the output:

   - cfg->out is ignored.

   - On success, *outRaw is assigned to the output array and ownership
     of it is transfered to the caller, who must eventually pass it to
     fsl_free() to free it.

*/
FSL_EXPORT int fsl_diff_v2_raw(fsl_buffer const * pv1,
                               fsl_buffer const * pv2,
                               fsl_diff_opt const * const cfg,
                               int **outRaw );


/**
   If zDate is an ISO8601-format string, optionally with a .NNN
   fractional suffix, then this function returns true and sets
   *pOut (if pOut is not NULL) to the corresponding Julian
   value. If zDate is not an ISO8601-format string then this
   returns false (0) and pOut is not modified.

   This function does NOT confirm that zDate ends with a NUL
   byte. i.e.  if passed a valid date string which has trailing
   bytes after it then those are simply ignored. This is so that it
   can be used to read subsets of larger strings.

   Achtung: this calculation may, due to voodoo-level
   floating-point behaviours, differ by a small fraction of a point
   (at the millisecond level) for a given input compared to other
   implementations (e.g. sqlite's strftime() _might_ differ by a
   millisecond or two or _might_ not). Thus this routine should not
   be used when 100% round-trip fidelity is required, but is close
   enough for routines which do not require 100% millisecond-level
   fidelity in time conversions.

   @see fsl_julian_to_iso8601()
*/
FSL_EXPORT bool fsl_iso8601_to_julian( char const * zDate, double * pOut );

/** 
    Converts the Julian Day J to an ISO8601 time string. If addMs is
    true then the string includes the '.NNN' fractional part, else
    it will not. This function writes (on success) either 20 or 24
    bytes (including the terminating NUL byte) to pOut, depending on
    the value of addMs, and it is up to the caller to ensure that
    pOut is at least that long.

    Returns true (non-0) on success and the only error conditions
    [it can catch] are if pOut is NULL, J is less than 0, or
    evaluates to a time value which does not fit in ISO8601
    (e.g. only years 0-9999 are supported).

    @see fsl_iso8601_to_julian()
*/
FSL_EXPORT bool fsl_julian_to_iso8601( double J, char * pOut, bool addMs );

/**
   Returns the Julian Day time J value converted to a Unix Epoch
   timestamp. It assumes 86400 seconds per day and does not account
   for leap seconds, leap years, leap frogs, or any other kind of
   leap, up to and including leaps of faith.
*/
FSL_EXPORT fsl_time_t fsl_julian_to_unix( double J );

/**
   Performs a chdir() to the directory named by zChDir.

   Returns 0 on success. On error it tries to convert the
   underlying errno to one of the FSL_RC_xxx values, falling
   back to FSL_RC_IO if it cannot figure out anything more
   specific.
*/
FSL_EXPORT int fsl_chdir(const char *zChDir);

/**
   A strftime() implementation.

   dest must be valid memory at least destLen bytes long. The result
   will be written there.

   fmt must contain the format string. See the file fsl_strftime.c
   for the complete list of format specifiers and their descriptions.

   timeptr must be the time the caller wants to format.

   Returns 0 if any arguments are NULL.

   On success it returns the number of bytes written to dest, not
   counting the terminating NUL byte (which it also writes). It
   returns 0 on any error, and the client may need to distinguish
   between real errors and (destLen==0 or !*fmt), both of which could
   also look like errors.

   TODOs:

   - Refactor this to take a callback or a fsl_buffer, so that we can
   format arbitrarily long output.

   - Refactor it to return an integer error code.

   (This implementation is derived from public domain sources
   dating back to the early 1990's.)
*/
FSL_EXPORT fsl_size_t fsl_strftime(char *dest, fsl_size_t destLen,
                        const char *format, const struct tm *timeptr);

/**
   A convenience form of fsl_strftime() which takes its timestamp in
   the form of a Unix Epoch time. See fsl_strftime() for the
   semantics of the first 3 arguments and the return value. If
   convertToLocal is true then epochTime gets converted to local
   time (via, oddly enough, localtime(3)), otherwise gmtime(3) is
   used for the conversion.

   BUG: this function uses static state and is not thread-safe.
*/
FSL_EXPORT fsl_size_t fsl_strftime_unix(char * dest, fsl_size_t destLen, char const * format,
                             fsl_time_t epochTime, bool convertToLocal);


/**
   A convenience form of fsl_strftime() which assumes that the
   formatted string is of "some reasonable size" and appends its
   formatted representation to b. Returns 0 on success, non-0 on
   error. If any argument is NULL or !*format then FSL_RC_MISUSE is
   returned. FSL_RC_RANGE is returned if the underlying call to
   fsl_strftime() fails (which it will if the format string
   resolves to something "unususually long"). It returns FSL_RC_OOM
   if appending to b fails due to an allocation error.
*/
FSL_EXPORT int fsl_buffer_strftime(fsl_buffer * b, char const * format, const struct tm *timeptr);

/**
   "whence" values for use with fsl_buffer_seek.
*/
enum fsl_buffer_seek_e {
FSL_BUFFER_SEEK_SET = 1,
FSL_BUFFER_SEEK_CUR = 2,
FSL_BUFFER_SEEK_END = 3
};
typedef enum fsl_buffer_seek_e fsl_buffer_seek_e;

/**
   "Seeks" b's internal cursor to a position specified by the given offset
   from either the current cursor position (FSL_BUFFER_SEEK_CUR), the start
   of the buffer (FSL_BUFFER_SEEK_SET), or the end (FSL_BUFFER_SEEK_END).
   If the cursor would be placed out of bounds, it will be placed at the start
   resp. end of the buffer.

   The "end" of a buffer is the value of its fsl_buffer::used member
   (i.e.  its one-after-the-end).

   Returns the new position.

   Note that most buffer algorithms, e.g. fsl_buffer_append(), do not
   modify the cursor. Only certain special-case algorithms use it.

   @see fsl_buffer_tell()
   @see fsl_buffer_rewind()
*/
FSL_EXPORT fsl_size_t fsl_buffer_seek(fsl_buffer * b, fsl_int_t offset,
                                      fsl_buffer_seek_e  whence);
/**
   Returns the buffer's current cursor position.

   @see fsl_buffer_rewind()
   @see fsl_buffer_seek()
*/
FSL_EXPORT fsl_size_t fsl_buffer_tell(fsl_buffer const *b);
/**
   Resets b's cursor to the beginning of the buffer.

   @see fsl_buffer_tell()
   @see fsl_buffer_seek()
*/
FSL_EXPORT void fsl_buffer_rewind(fsl_buffer *b);

/**
   The "Path Finder" class is a utility class for searching the
   filesystem for files matching a set of common prefixes and/or
   suffixes (i.e. directories and file extensions).

   Example usage:

   @code
   fsl_pathfinder pf = fsl_pathfinder_empty;
   int rc;
   char const * found = NULL;
   rc = fsl_pathfinder_ext_add( &pf, ".doc" );
   if(rc) { ...error... }
   // The following error checks are elided for readability:
   rc = fsl_pathfinder_ext_add( &pf, ".txt" );
   rc = fsl_pathfinder_ext_add( &pf, ".wri" );
   rc = fsl_pathfinder_dir_add( &pf, "." );
   rc = fsl_pathfinder_dir_add( &pf, "/my/doc/dir" );
   rc = fsl_pathfinder_dir_add( &pf, "/other/doc/dir" );

   rc = fsl_pathfinder_search( &pf, "MyDoc", &found, NULL);
   if(0==rc){ assert(NULL!=found); }

   // Eventually clean up:
   fsl_pathfinder_clear(&pf);
   @endcode

   @see fsl_pathfinder_dir_add()
   @see fsl_pathfinder_ext_add()
   @see fsl_pathfinder_clear()
   @see fsl_pathfinder_search()
*/
struct fsl_pathfinder {
  /**
     Holds the list of search extensions. Each entry
     is a (char *) owned by this object.
  */
  fsl_list ext;
  /**
     Holds the list of search directories. Each entry is a (char *)
     owned by this object.
  */
  fsl_list dirs;
  /**
     Used to build up a path string during fsl_pathfinder_search(),
     and holds the result of a successful search. We use a buffer,
     as opposed to a simple string, because (A) it simplifies the
     search implementation and (B) reduces allocations (it gets
     reused for each search).
  */
  fsl_buffer buf;
};

typedef struct fsl_pathfinder fsl_pathfinder;
/**
   Initialized-with-defaults fsl_pathfinder instance, intended for
   const copy initialization.
*/
#define fsl_pathfinder_empty_m {fsl_list_empty_m/*ext*/,fsl_list_empty_m/*dirs*/,fsl_buffer_empty_m/*buf*/}

/**
   Initialized-with-defaults fsl_pathfinder instance, intended for
   copy initialization.
*/
FSL_EXPORT const fsl_pathfinder fsl_pathfinder_empty;

/**
   Frees all memory associated with pf, but does not free pf.
   Is a no-op if pf is NULL.
*/
FSL_EXPORT void fsl_pathfinder_clear(fsl_pathfinder * pf);

/**
   Adds the given directory to pf's search path. Returns 0 on
   success, FSL_RC_MISUSE if !pf or !dir (dir _may_ be an empty
   string), FSL_RC_OOM if copying the string or adding it to the
   list fails.

   @see fsl_pathfinder_ext_add()
   @see fsl_pathfinder_search() 
*/
FSL_EXPORT int fsl_pathfinder_dir_add(fsl_pathfinder * pf, char const * dir);

/**
   Adds the given directory to pf's search extensions. Returns 0 on
   success, FSL_RC_MISUSE if !pf or !dir (dir _may_ be an empty
   string), FSL_RC_OOM if copying the string or adding it to the
   list fails.

   Note that the client is responsible for adding a "." to the
   extension, if needed, as this API does not apply any special
   meaning to any characters in a search extension. e.g. "-journal"
   and "~" are both perfectly valid extensions for this purpose.

   @see fsl_pathfinder_dir_add()
   @see fsl_pathfinder_search() 
*/
FSL_EXPORT int fsl_pathfinder_ext_add(fsl_pathfinder * pf, char const * ext);

/**
   Searches for a file whose name can be constructed by some
   combination of pf's directory/suffix list and the given base
   name.

   It searches for files in the following manner:

   If the 2nd parameter exists as-is in the filesystem, it is
   treated as a match, otherwise... Loop over all directories
   in pf->dirs. Create a path with DIR/base, or just base if
   the dir entry is empty (length of 0). Check for a match.
   If none is found, then... Loop over each extension in
   pf->ext, creating a path named DIR/baseEXT (note that it
   does not add any sort of separator between the base and the
   extensions, so "~" and "-foo" are legal extensions). Check
   for a match.

   On success (a readable filesystem entry is found):

   - It returns 0.

   - If pOut is not NULL then *pOut is set to the path it
   found. The bytes of the returned string are only valid until the
   next search operation on pf, so copy them if you need them.
   Note that the returned path is _not_ normalized via
   fsl_file_canonical_name() or similar, and it may very well
   return a relative path (if base or one of pf->dirs contains a
   relative path part).

   - If outLen is not NULL, *outLen will be set to the
   length of the returned string. 

   On error:

   - Returns FSL_RC_MISUSE if !pf, !base, !*base.

   - Returns FSL_RC_OOM on allocation error (it uses a buffer to
   hold its path combinations and return value).

   - Returns FSL_RC_NOT_FOUND if it finds no entry.

   The host platform's customary path separator is used to separate
   directory/file parts ('\\' on Windows and '/' everywhere else).

   Note that it _is_ legal for pOut and outLen to both be NULL, in
   which case a return of 0 signals that an entry was found, but
   the client has no way of knowing what path it might be (unless,
   of course, he relies on internal details of the fsl_pathfinder
   API, which he most certainly should not do).

   Tip: if the client wants to be certain that this function will
   not allocate memory, simply use fsl_buffer_reserve() on pf->buf
   to reserve the desired amount of space in advance. As long as
   the search paths never extend that length, this function will
   not need to allocate. (Until/unless the following TODO is
   implemented...)

   Potential TODO: use fsl_file_canonical_name() so that the search
   dirs themselves do not need to be entered using
   platform-specific separators. The main reason it's not done now
   is that it requires another allocation. The secondary reason is
   because it's sometimes useful to use relative paths in this
   context (based on usage in previous trees from which this code
   derives).

   @see fsl_pathfinder_dir_add()
   @see fsl_pathfinder_ext_add()
   @see fsl_pathfinder_clear()
*/
FSL_EXPORT int fsl_pathfinder_search(fsl_pathfinder * pf, char const * base,
                                     char const ** pOut, fsl_size_t * outLen );


/**
   A utility class for creating ZIP-format archives. All members
   are internal details and must not be mucked about with by the
   client.  See fsl_zip_file_add() for an example of how to use it.

   Note that it creates ZIP content in memory, as opposed to
   streaming it (it is not yet certain if abstractly streaming a
   ZIP is possible), so creating a ZIP file this way is exceedingly
   memory-hungry.

   @see fsl_zip_file_add()
   @see fsl_zip_timestamp_set_julian()
   @see fsl_zip_timestamp_set_unix()
   @see fsl_zip_end()
   @see fsl_zip_body()
   @see fsl_zip_finalize()
*/
struct fsl_zip_writer {
  /**
     Number of entries (files + dirs) added to the zip file so far.
  */
  fsl_size_t entryCount;
  /**
     Current DOS-format time of the ZIP.
  */
  int32_t dosTime;
  /**
     Current DOS-format date of the ZIP.
  */
  int32_t dosDate;
  /**
     Current Unix Epoch time of the ZIP.
  */
  fsl_time_t unixTime;
  /**
     An artificial root directory which gets prefixed
     to all inserted filenames.
  */
  char * rootDir;
  /**
     The buffer for the table of contents.
  */
  fsl_buffer toc;
  /**
     The buffer for the ZIP file body.
  */
  fsl_buffer body;
  /**
     Internal scratchpad for ops which often allocate
     small buffers.
  */
  fsl_buffer scratch;
  /**
     The current list of directory entries (as (char *)).
  */
  fsl_list dirs;
};
typedef struct fsl_zip_writer fsl_zip_writer;

/**
   An initialized-with-defaults fsl_zip_writer instance, intended
   for in-struct or const-copy initialization.
*/
#define fsl_zip_writer_empty_m {                \
  0/*entryCount*/,                            \
  0/*dosTime*/,                             \
  0/*dosDate*/,                             \
  0/*unixTime*/,                            \
  NULL/*rootDir*/,                          \
  fsl_buffer_empty_m/*toc*/,                \
  fsl_buffer_empty_m/*body*/,               \
  fsl_buffer_empty_m/*scratch*/,            \
  fsl_list_empty_m/*dirs*/                  \
}

/**
   An initialized-with-defaults fsl_zip_writer instance,
   intended for copy-initialization.
*/
FSL_EXPORT const fsl_zip_writer fsl_zip_writer_empty;

/**
   Sets a virtual root directory in z, such that all files added
   with fsl_zip_file_add() will get this directory prefixed to
   it.

   If zRoot is NULL or empty then this clears the virtual root,
   otherwise is injects any directory levels it needs to into the
   being-generated ZIP. Note that zRoot may contain multiple levels
   of directories, e.g. "foo/bar/baz", but it must be legal for
   use in a ZIP file.

   This routine copies zRoot's bytes, so they may be transient.

   Returns 0 on success, FSL_RC_ERROR if !z, FSL_RC_OOM on
   allocation error. Returns FSL_RC_RANGE if zRoot is an absolute
   path or if zRoot cannot be normalized to a "simplified name" (as
   per fsl_is_simple_pathname(), with the note that this routine
   will pass a copy of zRoot through fsl_file_simplify_name()
   first).

   @see fsl_zip_finalize()
*/
FSL_EXPORT int fsl_zip_root_set(fsl_zip_writer * z, char const * zRoot );

/**
   Adds a file or directory to the ZIP writer z. zFilename is the
   virtual name of the file or directory. If pContent is NULL then
   it is assumed that we are creating one or more directories,
   otherwise the ZIP's entry is populated from pContent. The
   permsFlag argument specifies the fossil-specific permission
   flags from the fsl_fileperm_e enum, but currently ignores the
   permsFlag argument for directories. Not that this function
   creates directory entries for any files automatically, so there
   is rarely a need for client code to create them (unless they
   specifically want to ZIP an empty directory entry).

   Notes of potential interest:

   - The ZIP is created in memory, and thus creating ZIPs with this
   API is exceedingly memory-hungry.

   - The timestamp of any given file must be set separately from
   this call using fsl_zip_timestamp_set_unix() or
   fsl_zip_timestamp_set_julian(). That value is then used for
   subsequent file-adds until a new time is set.

   - If a root directory has been set using fsl_zip_root_set() then
   that name, plus '/' (if the root does not end with one) gets
   prepended to all files added via this routine.

   An example of the ZIP-generation process:

   @code
   int rc;
   fsl_zip_writer z = fsl_zip_writer_empty;
   fsl_buffer buf = fsl_buffer_empty;
   fsl_buffer const * zipBody;

   // ...fill the buf buffer (not shown here)...

   // Optionally set a virtual root dir for new files:
   rc = fsl_zip_root_set( &z, "myRootDir" ); // trailing slash is optional
   if(rc) { ... error ...; goto end; }

   // We must set a timestamp which will be used until we set another:
   fsl_zip_timestamp_set_unix( &z, time(NULL) );

   // Add a file:
   rc = fsl_zip_file_add( &z, "foo/bar.txt", &buf, FSL_FILE_PERM_REGULAR );
   // Clean up our content:
   fsl_buffer_reuse(&buf); // only needed if we want to re-use the buffer's memory
   if(rc) goto end;

   // ... add more files the same way (not shown) ...

   // Now "seal" the ZIP file:
   rc = fsl_zip_end( &z );
   if(rc) goto end;

   // Fetch the ZIP content:
   zipBody = fsl_zip_body( &z );
   // zipBody now points to zipBody->used bytes of ZIP file content
   // which can be sent to an arbitrary destination, e.g.:
   rc = fsl_buffer_to_filename( zipBody, "my.zip" );

   end:
   fsl_buffer_clear(&buf);
   // VERY important, once we're done with z:
   fsl_zip_finalize( &z );
   if(rc){...we had an error...}
   @endcode

   @see fsl_zip_timestamp_set_julian()
   @see fsl_zip_timestamp_set_unix()
   @see fsl_zip_end()
   @see fsl_zip_body()
   @see fsl_zip_finalize()
*/
FSL_EXPORT int fsl_zip_file_add( fsl_zip_writer * z, char const * zFilename,
                                 fsl_buffer const * pContent, int permsFlag );

/**
   Ends the ZIP-creation process, padding all buffers, writing all
   final required values, and freeing up most of the memory owned
   by z. After calling this, z->body contains the full generated
   ZIP file.

   Returns 0 on success. On error z's contents may still be
   partially intact (for debugging purposes) and z->body will not
   hold complete/valid ZIP file contents. Results are undefined if
   !z or z has not been properly initialized.

   The caller must eventually pass z to fsl_zip_finalize() to free
   up any remaining resources.

   @see fsl_zip_timestamp_set_julian()
   @see fsl_zip_timestamp_set_unix()
   @see fsl_zip_file_add()
   @see fsl_zip_body()
   @see fsl_zip_finalize()
   @see fsl_zip_end_take()
*/
FSL_EXPORT int fsl_zip_end( fsl_zip_writer * z );

/**
   This variant of fsl_zip_end() transfers the current contents
   of the zip's body to dest, replacing (freeing) any contents it may
   hold when this is called, then passes z to fsl_zip_finalize()
   to free any other resources (which are invalidated by the removal
   of the body).

   Returns 0 on success, FSL_RC_MISUSE if either pointer is NULL,
   some non-0 code if the proxied fsl_zip_end() call fails. On
   error, the transfer of contents to dest does NOT take place, but
   z is finalized (if it is not NULL) regardless of success or
   failure (even if dest is NULL). i.e. on error z is still cleaned
   up.
*/
FSL_EXPORT int fsl_zip_end_take( fsl_zip_writer * z, fsl_buffer * dest );

/**
   This variant of fsl_zip_end_take() passes z to fsl_zip_end(),
   write's the ZIP body to the given filename, passes
   z to fsl_zip_finalize(), and returns the result of
   either end/save combination. Saving is not attempted
   if ending the ZIP fails.

   On success 0 is returned and the contents of the ZIP are in the
   given file. On error z is STILL cleaned up, and the file might
   have been partially populated (only on I/O error after writing
   started). In either case, z is cleaned up and ready for re-use or
   (in the case of a heap-allocated instance) freed.
*/
FSL_EXPORT int fsl_zip_end_to_filename( fsl_zip_writer * z, char const * filename );


/**
   Returns a pointer to z's ZIP content buffer. The contents
   are ONLY valid after fsl_zip_end() returns 0.

   @see fsl_zip_timestamp_set_julian()
   @see fsl_zip_timestamp_set_unix()
   @see fsl_zip_file_add()
   @see fsl_zip_end()
   @see fsl_zip_end_take()
   @see fsl_zip_finalize()
*/
FSL_EXPORT fsl_buffer const * fsl_zip_body( fsl_zip_writer const * z );

/**
   Frees all memory owned by z and resets it to a clean state, but
   does not free z. Any fsl_zip_writer instance which has been
   modified via the fsl_zip_xxx() family of functions MUST
   eventually be passed to this function to clean up any contents
   it might have accumulated during its life. After this returns,
   z is legal for re-use in creating a new ZIP archive.

   @see fsl_zip_timestamp_set_julian()
   @see fsl_zip_timestamp_set_unix()
   @see fsl_zip_file_add()
   @see fsl_zip_end()
   @see fsl_zip_body()
*/
FSL_EXPORT void fsl_zip_finalize(fsl_zip_writer * z);

/**
   Set z's date and time from a Julian Day number. Results are
   undefined if !z. Results will be invalid if rDate is negative. The
   timestamp is applied to all fsl_zip_file_add() operations until it
   is re-set.

   @see fsl_zip_timestamp_set_unix()
   @see fsl_zip_file_add()
   @see fsl_zip_end()
   @see fsl_zip_body()
*/
FSL_EXPORT void fsl_zip_timestamp_set_julian(fsl_zip_writer *z, double rDate);

/**
   Set z's date and time from a Unix Epoch time. Results are
   undefined if !z. Results will be invalid if rDate is negative. The
   timestamp is applied to all fsl_zip_file_add() operations until it
   is re-set.
*/
FSL_EXPORT void fsl_zip_timestamp_set_unix(fsl_zip_writer *z, fsl_time_t epochTime);

/**
   State for the fsl_timer_xxx() family of functions.

   @see fsl_timer_start()
   @see fsl_timer_reset()
   @see fsl_timer_stop()
*/
struct fsl_timer_state {
  /**
     The amount of time (microseconds) spent in "user space."
  */
  uint64_t user;
  /**
     The amount of time (microseconds)spent in "kernel space."
  */
  uint64_t system;
};
typedef struct fsl_timer_state fsl_timer_state;

/**
   Initialized-with-defaults fsl_timer_state_empty instance,
   intended for const copy initialization.
*/
#define fsl_timer_state_empty_m {0,0}

/**
   Initialized-with-defaults fsl_timer_state_empty instance,
   intended for copy initialization.
*/
FSL_EXPORT const fsl_timer_state fsl_timer_state_empty;

/**
   Sets t's counter state to the current CPU timer usage, as
   determined by the OS.

   Achtung: timer support is only enabled if the library is built
   with the FSL_CONFIG_ENABLE_TIMER macro set to a true value (it is
   on by default).

   @see fsl_timer_reset()
   @see fsl_timer_stop()
*/
FSL_EXPORT void fsl_timer_start(fsl_timer_state * t);

/**
   Returns the difference in _CPU_ times in microseconds since t was
   last passed to fsl_timer_start() or fsl_timer_reset().  It might
   return 0 due to system-level precision restrictions. Note that this
   is not useful for measuring wall times.
*/
FSL_EXPORT uint64_t fsl_timer_fetch(fsl_timer_state const * t);

/**
   Resets t to the current time and returns the number of microseconds
   since t was started or last reset.

   @see fsl_timer_start()
   @see fsl_timer_reset()
*/
FSL_EXPORT uint64_t fsl_timer_reset(fsl_timer_state * t);

/**
   Clears t's state and returns the difference (in uSec) between the
   last time t was started or reset, as per fsl_timer_fetch().

   @see fsl_timer_start()
   @see fsl_timer_reset()
*/
FSL_EXPORT uint64_t fsl_timer_stop(fsl_timer_state *t);

/**
   For the given red/green/blue values (all in the range of 0 to
   255, or truncated to be so!) this function returns the RGB
   encoded in the lower 24 bits of a single number. See
   fsl_gradient_color() for an explanation and example.

   For those asking themselves, "why does an SCM API have a function
   for encoding RGB colors?" the answer is: fossil(1) has a long
   history of using HTML color codes to set the color of branches,
   and this is provided in support of such features.

   @see fsl_rgb_decode()
   @see fsl_gradient_color()
*/
FSL_EXPORT unsigned int fsl_rgb_encode( int r, int g, int b );

/**
   Given an RGB-encoded source value, this function decodes
   the lower 24 bits into r, g, and b. Any of r, g, and b may
   be NULL to skip over decoding of that part.

   @see fsl_rgb_encode()
   @see fsl_gradient_color()
*/
FSL_EXPORT void fsl_rgb_decode( unsigned int src, int *r, int *g, int *b );

/**
   For two color values encoded as RRGGBB values (see below for the
   structure), this function computes a gradient somewhere between
   those colors. c1 and c2 are the edges of the gradient.
   numberOfSteps is the number of steps in the gradient. stepNumber
   is a number less than numberOfSteps which specifies the "degree"
   of the gradients. If either numberOfSteps or stepNumber is 0, c1
   is returned. stepNumber of equal to or greater than c2 returns
   c2.

   The returns value is an RGB-encoded value in the lower 24 bits,
   ordered in big-endian. In other words, assuming rc is the return
   value:
     
   - red   = (rc&0xFF0000)>>16
   - green = (rc&0xFF00)>>8
   - blue  = (rc&0xFF)

   Or use fsl_rgb_decode() to break it into its component parts.

   It can be passed directly to a printf-like function, using the
   hex-integer format specifier, e.g.:

   @code
   fsl_buffer_appendf(&myBuf, "#%06x", rc);
   @endcode

   Tip: for a given HTML RRGGBB value, its C representation is
   identical: HTML \#F0D0A0 is 0xF0D0A0 in C.

   @see fsl_rgb_encode()
   @see fsl_rgb_decode()
*/
FSL_EXPORT unsigned int fsl_gradient_color(unsigned int c1, unsigned int c2,
                                           unsigned int numberOfSteps,
                                           unsigned int stepNumber);

/**
   "Simplifies" an SQL string by making the following modifications
   inline:

   - Consecutive non-newline spaces outside of an SQL string are
   collapsed into one space.

   - Consecutive newlines outside of an SQL string are collapsed into
   one space.

   Contents of SQL strings are not transformed in any way.

   len must be the length of the sql string. If it is negative,
   fsl_strlen(sql) is used to calculate the length.

   Returns the number of bytes in the modified string (its strlen) and
   NUL-terminates it at the new length. Thus the input string must be
   at least one byte longer than its virtual length (its NUL
   terminator byte suffices, provided it is NUL-terminated, as we can
   safely overwrite that byte).

   If !sql or its length resolves to 0, this function returns 0
   without side effects.
*/
FSL_EXPORT fsl_size_t fsl_simplify_sql( char * sql, fsl_int_t len );

/**
   Convenience form of fsl_simplify_sql() which assumes b holds an SQL
   string. It gets processed by fsl_simplify_sql() and its 'used'
   length potentially gets adjusted to match the adjusted SQL string.
*/
FSL_EXPORT fsl_size_t fsl_simplify_sql_buffer( fsl_buffer * b );


/**
   Returns the result of calling the platform's equivalent of
   isatty(fd). e.g. on Windows this is _isatty() and on Unix
   isatty(). i.e. it returns a true value (non-0) if it thinks that
   the given file descriptor value is attached to an interactive
   terminal, else it returns false.
*/
FSL_EXPORT char fsl_isatty(int fd);


/**

   A container type for lists of db record IDs. This is used in
   several places as a cache for record IDs, to keep track of ones
   we know about, ones we know that we don't know about, and to
   avoid duplicate processing in some contexts.
*/
struct fsl_id_bag {
  /**
     Number of entries of this->list which are in use (have a
     positive value). They need not be contiguous!  Must be <=
     capacity.
  */
  fsl_size_t entryCount;
  /**
     The number of elements allocated for this->list.
  */
  fsl_size_t capacity;
  /**
     The number of elements in this->list which have a zero or
     positive value. Must be <= capacity.
  */
  fsl_size_t used;
  /**
     Array of IDs this->capacity elements long. "Used" elements
     have a positive value. Unused ones are set to 0.
  */
  fsl_id_t * list;
};

/**
   Initialized-with-defaults fsl_id_bag structure,
   intended for copy initialization.
*/
FSL_EXPORT const fsl_id_bag fsl_id_bag_empty;

/**
   Initialized-with-defaults fsl_id_bag structure,
   intended for in-struct initialization.
*/
#define fsl_id_bag_empty_m {                    \
    0/*entryCount*/, 0/*capacity*/,             \
    0/*used*/, NULL/*list*/ }

/**

   Return the number of elements in the bag.
*/
FSL_EXPORT fsl_size_t fsl_id_bag_count(fsl_id_bag const *p);

/**

   Remove element e from the bag if it exists in the bag. If e is not
   in the bag, this is a no-op. Returns true if it removes an element,
   else false.

   e must be positive. Results are undefined if e<=0.
*/
FSL_EXPORT bool fsl_id_bag_remove(fsl_id_bag *p, fsl_id_t e);

/**
   Returns true if e is in the given bag. Returns false if it is
   not. It is illegal to pass an e value of 0, and that will trigger
   an assertion in debug builds. In non-debug builds, behaviour if
   passed 0 is undefined.
*/
FSL_EXPORT bool fsl_id_bag_contains(fsl_id_bag const *p, fsl_id_t e);

/**
   Insert element e into the bag if it is not there already.  Returns
   0 if it actually inserts something or if it already contains such
   an entry, and some other value on error (namely FSL_RC_OOM on
   allocation error).

   e must be positive or an assertion is triggered in debug builds. In
   non-debug builds, behaviour if passed 0 is undefined.
*/
FSL_EXPORT int fsl_id_bag_insert(fsl_id_bag *p, fsl_id_t e);

/**
   Returns the ID of the first element in the bag.  Returns 0 if the
   bag is empty.

   Example usage:

   @code
   fsl_id_t nid;
   for( nid = fsl_id_bag_first(&list);
   nid > 0;
   nid = fsl_id_bag_next(&list, nid)){
     
   ...do something...
   }
   @endcode
*/
FSL_EXPORT fsl_id_t fsl_id_bag_first(fsl_id_bag const *p);

/**
   Returns the next element in the bag after e.  Return 0 if e is
   the last element in the bag.  Any insert or removal from the bag
   might reorder the bag. It is illegal to pass this 0 (and will
   trigger an assertion in debug builds). For the first call, pass
   it the non-0 return value from fsl_id_bag_first(). For
   subsequent calls, pass the previous return value from this
   function.

   @see fsl_id_bag_first()
*/
FSL_EXPORT fsl_id_t fsl_id_bag_next(fsl_id_bag const *p, fsl_id_t e);

/**
   Swaps the contents of the given bags.
*/
FSL_EXPORT void fsl_id_bag_swap(fsl_id_bag *lhs, fsl_id_bag *rhs);

/**
   Frees any memory owned by p, but does not free p.
*/
FSL_EXPORT void fsl_id_bag_clear(fsl_id_bag *p);

/**
   Resets p's internal list, effectively emptying it for re-use, but
   does not free its memory. Immediately after calling this
   fsl_id_bag_count() will return 0.
*/
FSL_EXPORT void fsl_id_bag_reset(fsl_id_bag *p);


/**
   Returns true if p contains a fossil-format merge conflict marker,
   else returns false.
   
   @see fsl_buffer_merge3()
*/
FSL_EXPORT bool fsl_buffer_contains_merge_marker(fsl_buffer const *p);

/**
   Performs a three-way merge.
   
   The merge is an edit against pV2. Both pV1 and pV2 have a common
   origin at pPivot. Apply the changes of pPivot ==> pV1 to pV2,
   appending them to pOut. (Pedantic side-note: the input buffers are
   not const because we need to manipulate their cursors, but their
   buffered memory is not modified.)

   If merge conflicts are encountered, it continues as best as it can
   and injects "indiscrete" markers in the output to denote the nature
   of each conflict. If conflictCount is not NULL then on success the
   number of merge conflicts is written to *conflictCount.

   Returns 0 on success, FSL_RC_OOM on OOM, FSL_RC_TYPE if any input
   appears to be binary. 

   @see fsl_buffer_contains_merge_marker()
*/
FSL_EXPORT int fsl_buffer_merge3(fsl_buffer *pPivot, fsl_buffer *pV1, fsl_buffer *pV2,
                                 fsl_buffer *pOut, unsigned int *conflictCount);

/**
   Appends the first n bytes of string z to buffer b in the form of
   TCL-format string literal. If n<0 then fsl_strlen() is used to
   determine the length. Returns 0 on success, FSL_RC_OOM on error.
 */
FSL_EXPORT int fsl_buffer_append_tcl_literal(fsl_buffer * const b,
                                             char const * z, fsl_int_t n);


/**
   Event IDs for use with fsl_confirm_callback_f implementations.

   The idea here is to send, via callback, events from the library to
   the client when a potentially interactive response is necessary.
   We define a bare minimum of information needed for the client to
   prompt a user for a response. To that end, the interface passes on
   2 pieces of information to the client: the event ID and a filename.
   It is up to the application to translate that ID into a
   user-readable form, get a response (using a well-defined set of
   response IDs), and convey that back to the
   library via the callback's result pointer interface.

   This enum will be extended as the library develops new requirements
   for interactive use.

   @see fsl_confirm_response_e
*/
enum fsl_confirm_event_e {
/**
   Sentinal value.
*/
FSL_CEVENT_INVALID = 0,
/**
   An operation requests permission to overwrite a locally-modified
   file. e.g. when performing a checkout over a locally-edited
   version. Overwrites of files which are known to be in the previous
   (being-overwritten) checkout version are automatically overwritten.
*/
FSL_CEVENT_OVERWRITE_MOD_FILE = 1,
/**
   An operation requests permission to overwrite an SCM-unmanaged file
   with one which is managed by SCM. This can happen, e.g., when
   switching from a version which did not contain file X, but had file
   X on disk, to a version which contains file X.
*/
FSL_CEVENT_OVERWRITE_UNMGD_FILE = 2,
/**
   An operation requests permission to remove a LOCALLY-MODIFIED file
   which has been removed from SCM management. e.g. when performing a
   checkout over a locally-edited version and an edited file was
   removed from the SCM somewhere between those two versions.
   UMODIFIED files which are removed from the SCM between two
   checkouts are automatically removed on the grounds that it poses no
   data loss risk because the other version is "somewhere" in the SCM.
*/
FSL_CEVENT_RM_MOD_UNMGD_FILE = 3,

/**
   Indicates that the library cannot determine which of multiple
   potential versions to choose from and requires the user to
   select one.
*/
FSL_CEVENT_MULTIPLE_VERSIONS = 4

};
typedef enum fsl_confirm_event_e fsl_confirm_event_e;

/**
   Answers to questions posed to clients via the
   fsl_confirm_callback_f() interface.

   This enum will be extended as the library develops new requirements
   for interactive use.

   @see fsl_confirm_event_e
*/
enum fsl_confirm_response_e {
/**
   Sentinel/default value - not a valid answer. Guaranteed to have a
   value of 0. No other entries in this enum are guaranteed to have
   well-known/stable values: always use the enum symbols instead of
   integer values.
*/
FSL_CRESPONSE_INVALID = 0,
/**
   Accept the current event and continue processes.
*/
FSL_CRESPONSE_YES = 1,
/**
   Reject the current event and continue processes.
*/
FSL_CRESPONSE_NO = 2,
/**
   Reject the current event and stop processesing. Cancellation is
   generally considered to be a recoverable error.
*/
FSL_CRESPONSE_CANCEL = 3,
/**
   Accept the current event and all identical event types for the
   current invocation of this particular SCM operation.
*/
FSL_CRESPONSE_ALWAYS = 5,
/**
   Reject the current event and all identical event types for the
   current invocation of this particular SCM operation.
*/
FSL_CRESPONSE_NEVER = 6,
/**
   For events which are documented as being multiple-choice,
   this answer indicates that the client has set the index of
   their choice in the fsl_confirm_response::multipleChoice
   field:

   - FSL_CEVENT_MULTIPLE_VERSIONS
*/
FSL_CRESPONSE_MULTI = 7
};
typedef enum fsl_confirm_response_e fsl_confirm_response_e;

/**
   A response for use with the fsl_confirmer API. It is intended to
   encapsulate, with a great deal of abstraction, answers to typical
   questions which the library may need to interactively query a user
   for. e.g. confirmation about whether to overwrite a file or which
   one of 3 versions to select.

   This type will be extended as the library develops new requirements
   for interactive use.
*/
struct fsl_confirm_response {
  /**
     Client response to the current fsl_confirmer question.
  */
  fsl_confirm_response_e response;
  /**
     If this->response is FSL_CRESPONSE_MULTI then this must be set to
     the index of the client's multiple-choice answer.

     Events which except this in their response:

     - FSL_CEVENT_MULTIPLE_VERSIONS
  */
  uint16_t multipleChoice;
};
/**
   Convenience typedef.
*/
typedef struct fsl_confirm_response fsl_confirm_response;

/**
   Empty-initialized fsl_confirm_detail instance to be used for
   const copy initialization.
*/
#define fsl_confirm_response_empty_m {FSL_CRESPONSE_INVALID, -1}

/**
   Empty-initialized fsl_confirm_detail instance to be used for
   non-const copy initialization.
*/
FSL_EXPORT const fsl_confirm_response fsl_confirm_response_empty;

/**
   A struct for passing on interactive questions to
   fsl_confirmer_callback_f implementations.
*/
struct fsl_confirm_detail {
  /**
     The message ID of this confirmation request. This value
     determines how the rest of this struct's values are to
     be interpreted.
  */
  fsl_confirm_event_e eventId;
  /**
     Depending on the eventId, this might be NULL or might refer to a
     filename. This will be a filename for following confirmations
     events:

     - FSL_CEVENT_OVERWRITE_MOD_FILE
     - FSL_CEVENT_OVERWRITE_UNMGD_FILE
     - FSL_CEVENT_RM_MOD_UNMGD_FILE

     For all others it will be NULL.

     Whether this name refers to an absolute or relative path is
     context-dependent, and not specified by this API. In general,
     relative paths should be used if/when what they are relative to
     (e.g. a checkout root) is/should be clear to the user. The intent
     is that applications can display that name to the user in a UI
     control, so absolute paths "should" "generally" be avoided
     because they can be arbitrarily long.
  */
  const char * filename;
  /**
     Depending on the eventId, this might be NULL or might
     refer to a list of details of a type specified in the
     documentation for that eventId.

     Implementation of such an event is still TODO, but we have at
     least one use case lined up (asking a user which of several
     versions is intended when the checkout-update operation is given
     an ambiguous hash prefix).

     Events for which this list will be populated:

     - FSL_CEVENT_MULTIPLE_VERSIONS: each list entry will be a (char
     const*) with a version number, branch name, or similar, perhaps
     with relevant metadata such as a checkin timestamp. The client is
     expected to pick one answer, set its list index to the
     fsl_confirm_response::multipleChoice member, and to set
     fsl_confirm_response::response to FSL_CRESPONSE_MULTI.

     In all cases, a response of FSL_CRESPONSE_CANCEL will trigger a
     cancellation.

     In all cases, the memory for the items in this list is owned by
     (or temporarily operated on the behalf of) the routine which has
     launched this query. fsl_confirm_callback_f implements must never
     manipulate the list's or its content's state.
  */
  const fsl_list * multi;
};
typedef struct fsl_confirm_detail fsl_confirm_detail;

/**
   Empty-initialized fsl_confirm_detail instance to be used for
   const-copy initialization.
*/
#define fsl_confirm_detail_empty_m \
  {FSL_CEVENT_INVALID, NULL, NULL}

/**
   Empty-initialized fsl_confirm_detail instance to be used for
   non-const-copy initialization.
*/
FSL_EXPORT const fsl_confirm_detail fsl_confirm_detail_empty;

/**
   Should present the user (if appropriate) with an option of how to
   handle the given event write that answer to
   outAnswer->response. Return 0 on success, non-0 on error, in which
   case the current operation will fail with that result code.
   Answering with FSL_CRESPONSE_CANCEL is also considered failure but
   recoverably so, whereas a non-cancel failure is considered
   unrecoverable.
*/
typedef int (*fsl_confirm_callback_f)(fsl_confirm_detail const * detail,
                                      fsl_confirm_response *outAnswer,
                                      void * confirmerState);

/**
   A fsl_confirm_callback_f and its callback state, packaged into a
   neat little struct for easy copy/replace/restore of confirmers.
*/
struct fsl_confirmer {
  /**
     Callback which can be used for basic interactive confirmation
     purposes, within the very libfossil-centric limits of the
     interface.
  */
  fsl_confirm_callback_f callback;
  /**
     Opaque state pointer for this->callback. Its lifetime is not
     managed by this object and it is assumed, if not NULL, to live at
     least as long as this object.
  */
  void * callbackState;
};
typedef struct fsl_confirmer fsl_confirmer;
/** Empty-initialized fsl_confirmer instance for const-copy
    initialization. */
#define fsl_confirmer_empty_m {NULL,NULL}
/** Empty-initialized fsl_confirmer instance for non-const-copy
    initialization. */
FSL_EXPORT const fsl_confirmer fsl_confirmer_empty;

/**
   State for use with fsl_dircrawl_f() callbacks.

   @see fsl_dircrawl()
*/
struct fsl_dircrawl_state {
  /**
     Absolute directory name of the being-visited directory.
  */
  char const *absoluteDir;
  /**
     Name (no path part) of the entry being visited.
  */
  char const *entryName;
  /**
     Filesystem entry type.
  */
  fsl_fstat_type_e entryType;
  /**
     Opaque client-specified pointer which was passed to
     fsl_dircrawl().
  */
  void * callbackState;

  /**
     Directory depth of the crawl process, starting at 1 with
     the directory passed to fsl_dircrawl().
  */
  unsigned int depth;
};
typedef struct fsl_dircrawl_state fsl_dircrawl_state;

/**
   Callback type for use with fsl_dircrawl(). It gets passed the
   absolute name of the target directory, the name of the directory
   entry (no path part), the type of the entry, and the client state
   pointer which is passed to that routine. It must return 0 on
   success or another FSL_RC_xxx value on error. Returning
   FSL_RC_BREAK will cause directory-crawling to stop without an
   error.

   All pointers in the state argument are owned by fsl_dircrawl() and
   will be invalidated as soon as the callback returns, thus they must
   be copied if they are needed for later.
*/
typedef int (*fsl_dircrawl_f)(fsl_dircrawl_state const *);

/**
   Recurses into a directory and calls a callback for each filesystem
   entry.  It does not change working directories, but callbacks are
   free to do so as long as they restore the working directory before
   returning.

   The first argument is the name of the directory to crawl. In order
   to avoid any dependence on a specific working directory, if it is
   not an absolute path then this function will expand it to an
   absolute path before crawling begins. For each entry under the
   given directory, it calls the given callback, passing it a
   fsl_dircrawl_state object holding various state. All pointers in
   that object, except for the callbackState pointer, are owned by
   this function and may be invalidated as soon as the callback
   returns.

   For each directory entry, it recurses into that directory,
   depth-first _after_ passing it to the callback.

   It DOES NOT resolve/follow symlinks, instead passing them on to the
   callback for processing. Note that passing a symlink to this
   function will not work because this function does not resolve
   symlinks. Thus it provides no way to traverse symlinks, as its
   scope is only features suited for the SCM and symlinks have no
   business being in an SCM. (Fossil supports symlinks, more or less,
   but libfossil does not.)

   It silently skips any files for which stat() fails or is not of a
   "basic" file type (e.g. character devices and such).

   Returns 0 on success, FSL_RC_TYPE if the given name is not a
   directory, and FSL_RC_RANGE if it recurses "too deep," (some
   "reasonable" internally hard-coded limit), in order to prevent a
   stack overflow.

   If the callback returns non-0, iteration stops and returns that
   result code unless the result is FSL_RC_BREAK, which stops
   iteration but causes 0 to be returned from this function.
*/
FSL_EXPORT int fsl_dircrawl(char const * dirName, fsl_dircrawl_f callback,
                            void * callbackState);

/**
   Strips any trailing slashes ('/') from the given string by
   assigning those bytes to NUL and returns the number of slashes
   NUL'd out. nameLen must be the length of the string. If nameLen is
   negative, fsl_strlen() is used to calculate its length.
*/
FSL_EXPORT fsl_size_t fsl_strip_trailing_slashes(char * name, fsl_int_t nameLen);

/**
   A convenience from of fsl_strip_trailing_slashes() which strips
   trailing slashes from the given buffer and changes its b->used
   value to account for any stripping. Results are undefined if b is
   not properly initialized.
*/
FSL_EXPORT void fsl_buffer_strip_slashes(fsl_buffer * b);

/**
   Appends each ID from the given bag to the given buffer using the given
   separator string. Returns FSL_RC_OOM on allocation error.
*/
FSL_EXPORT int fsl_id_bag_to_buffer(fsl_id_bag const * bag, fsl_buffer * b,
                                    char const * separator);

#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_UTIL_H_INCLUDED */
/* end of file ../include/fossil-scm/fossil-util.h */
/* start of file ../include/fossil-scm/fossil-core.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_CORE_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_CORE_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

*/

/* @file fossil-core.h

  This file declares the core SCM-related public APIs.
*/

#include <time.h> /* struct tm, time_t */
#if defined(__cplusplus)
/**
  The fsl namespace is reserved for an eventual C++ wrapper for the API.
*/
namespace fsl {}
extern "C" {
#endif

/**
   @struct fsl_cx

   The main Fossil "context" type. This is the first argument to
   many Fossil library API routines, and holds all state related
   to a checkout and/or repository and/or global fossil configuration
   database(s).

   An instance's lifetime looks something like this:

   @code
   int rc;
   fsl_cx * f = NULL; // ALWAYS initialize to NULL or results are undefined
   rc = fsl_cx_init( &f, NULL );
   assert(!rc);
   rc = fsl_repo_open( f, "myrepo.fsl" );
   ...use the context, and clean up when done...
   fsl_cx_finalize(f);
   @endcode

   The contents of an fsl_cx instance are strictly private, for use
   only by APIs in this library. Any client-side dependencies on
   them will lead to undefined behaviour at some point.

   Design note: this type is currently opaque to client
   code. Having it non-opaque also has advantages, though, and i'd
   generally prefer that (to allow client-side allocation and
   embedding in other structs). Binary compatibility concerns might
   force us to keep it opaque.

*/

typedef struct fsl_cx fsl_cx;
typedef struct fsl_cx_config fsl_cx_config;
typedef struct fsl_db fsl_db;
typedef struct fsl_cx_init_opt fsl_cx_init_opt;
typedef struct fsl_stmt fsl_stmt;

/**
   This enum defines type ID tags with which the API tags fsl_db
   instances so that the library can figure out which DB is
   which. This is primarily important for certain queries, which
   need to know whether they are accessing the repo or config db,
   for example.

   All that said, i'm not yet fully convinced that a straight port of
   the fossil model is the best option for how we internally manage
   DBs, so this is subject to eventual change or removal.

   @see fsl_db_role_label()
   @see fsl_cx_db_name_for_role()
*/
enum fsl_dbrole_e {
/**
   Sentinel "no role" value.
*/
FSL_DBROLE_NONE = 0,
/**
   Analog to fossil's "configdb".
*/
FSL_DBROLE_CONFIG = 0x01,
/**
   Analog to fossil's "repository".
*/
FSL_DBROLE_REPO = 0x02,
/**
   Analog to fossil's "localdb".
*/
FSL_DBROLE_CKOUT = 0x04,
/**
   Analog to fossil's "main", which is basically an alias for the
   first db opened. This API opens an in-memory db to act as the main
   db and attaches the repo/checkout/config databases separately so
   that it has complete control over their names and internal
   relationships.
*/
FSL_DBROLE_MAIN = 0x08,
/**
   Refers to the "temp" database. This is only used by a very few APIs
   and is outright invalid for most.
*/
FSL_DBROLE_TEMP = 0x10
};
typedef enum fsl_dbrole_e fsl_dbrole_e;

/**
   Bitmask values specifying "configuration sets."  The values in
   this enum come directly from fossil(1), but they are not part of
   the db structure, so may be changed over time.

   It seems very unlikely that these will ever be used at the level of
   this library. They are a "porting artifact" and retained for the
   time being, but will very likely be removed.
*/
enum fsl_configset_e {
/** Sentinel value. */
FSL_CONFSET_NONE = 0x000000,
/** Style sheet only */
FSL_CONFIGSET_CSS = 0x000001,
/** WWW interface appearance */
FSL_CONFIGSET_SKIN = 0x000002,
/** Ticket configuration */
FSL_CONFIGSET_TKT = 0x000004,
/** Project name */
FSL_CONFIGSET_PROJ = 0x000008,
/** Shun settings */
FSL_CONFIGSET_SHUN = 0x000010,
/** The USER table */
FSL_CONFIGSET_USER = 0x000020,
/** The CONCEALED table */
FSL_CONFIGSET_ADDR = 0x000040,
/** Transfer configuration */
FSL_CONFIGSET_XFER = 0x000080,
/** Everything */
FSL_CONFIGSET_ALL = 0x0000ff,
/** Causes overwrite instead of merge */
FSL_CONFIGSET_OVERWRITE = 0x100000,
/** Use the legacy format */
FSL_CONFIGSET_OLDFORMAT = 0x200000
};
typedef enum fsl_configset_e fsl_configset_e;

/**
   Runtime-configurable flags for a fsl_cx instance.
*/
enum fsl_cx_flags_e {
FSL_CX_F_NONE = 0,
/**
   Tells us whether or not we want to calculate R-cards by default.
   Historically they were initially required but eventually made
   optional due largely to their memory costs.
*/
FSL_CX_F_CALC_R_CARD = 0x01,

/**
   When encounting artifact types in the crosslinking phase which
   the library does not currently support crosslinking for, skip over
   them instead of generating an error.
*/
FSL_CX_F_SKIP_UNKNOWN_CROSSLINKS = 0x02,

/**
   By default, fsl_reserved_fn_check() will fail if the given filename
   is reserved on Windows platforms because such filenames cannot be
   checked out on Windows.  This flag removes that limitation. It
   should only be used, if at all, for repositories which will _never_
   be used on Windows.
 */
FSL_CX_F_ALLOW_WINDOWS_RESERVED_NAMES = 0x04,

/**
   If on (the default) then an internal cache will be used for
   artifact loading to speed up operations which do lots of that.
*/
FSL_CX_F_MANIFEST_CACHE = 0x08,

/**
   Internal use only to prevent duplicate initialization of some
   bits.
*/
FSL_CX_F_IS_OPENING_CKOUT = 0x100,

/**
   Default flags for all fsl_cx instances.
*/
FSL_CX_F_DEFAULTS = FSL_CX_F_MANIFEST_CACHE

};
typedef enum fsl_cx_flags_e fsl_cx_flags_e;

/**
    List of hash policy values. New repositories should generally use
    only SHA3 hashes, but older repos may contain SHA1 hashes (perhaps
    only SHA1), so we have to support those. Repositories may contain
    a mix of hash types.

    ACHTUNG: this enum's values must align with those from fossil(1).
*/
enum fsl_hashpolicy_e {
/* Use only SHA1 hashes. */
FSL_HPOLICY_SHA1 = 0,
/* Accept SHA1 hashes but auto-promote to SHA3. */
FSL_HPOLICY_AUTO = 1,
/* Use SHA3 hashes. */
FSL_HPOLICY_SHA3 = 2,
/* Use SHA3 hashes exclusively. */
FSL_HPOLICY_SHA3_ONLY = 3,
/* With this policy, fsl_uuid_is_shunned() will always return true for
   SHA1 hashes. */
FSL_HPOLICY_SHUN_SHA1 = 4
};
typedef enum fsl_hashpolicy_e fsl_hashpolicy_e;

/**
   Most functions in this API which return an int type return error
   codes from the fsl_rc_e enum.  None of these entries are
   (currently) guaranteed to have a specific value across Fossil
   versions except for FSL_RC_OK, which is guaranteed to always be
   0 (and the API guarantees that no other code shall have a value
   of zero).

   The only reasons numbers are hard-coded to the values (or some of
   them) is to simplify debugging during development. Clients may use
   fsl_rc_cstr() to get some human-readable (or programmer-readable)
   form for any given value in this enum.

   Maintenance reminder: as entries are added/changed, update
   fsl_rc_cstr().
*/
enum fsl_rc_e {
/**
   The quintessential not-an-error value.
*/
FSL_RC_OK = 0,
/**
   Generic/unknown error.
*/
FSL_RC_ERROR = 100,
/**
   A placeholder return value for "not yet implemented" functions.
*/
FSL_RC_NYI = 101,
/**
   Out of memory. Indicates that a resource allocation request
   failed.
*/
FSL_RC_OOM = 102,
/*
  API misuse (invalid args)
*/
FSL_RC_MISUSE = 103,
/**
   Some range was violated (function argument, UTF character, etc.).
*/
FSL_RC_RANGE = 104,
/**
   Indicates that access to or locking of a resource was denied
   by some security mechanism or other.
*/
FSL_RC_ACCESS = 105,
/**
   Indicates an I/O error. Whether it was reading or writing is
   context-dependent.
*/
FSL_RC_IO = 106,
/**
   requested resource not found
*/
FSL_RC_NOT_FOUND = 107,
/**
   Indicates that a to-be-created resource already exists.
*/
FSL_RC_ALREADY_EXISTS = 108,
/**
   Data consistency problem
*/
FSL_RC_CONSISTENCY = 109,

/**
   Indicates that the requested repo needs to be rebuilt.
*/
FSL_RC_REPO_NEEDS_REBUILD = 110,

/**
   Indicates that the requested repo is not, in fact, a repo. Also
   used by some APIs to indicate that they require a repository db
   but none has been opened.
*/
FSL_RC_NOT_A_REPO = 111,

/**
   Indicates an attempt to open a too-old or too-new repository db.
*/
FSL_RC_REPO_VERSION = 112,

/**
   Indicates db-level error (e.g. statement prep failed). In such
   cases, the error state of the related db handle (fsl_db) or
   Fossilc context (fsl_cx) will be updated to contain more
   information directly from the db driver.
*/
FSL_RC_DB = 113,

/**
   Used by some iteration routines to indicate that iteration should
   stop prematurely without an error.
*/
FSL_RC_BREAK = 114,

/**
   Indicates that fsl_stmt_step() has fetched a row and the cursor
   may be used to access the current row state (e.g. using
   fsl_stmt_get_int32() and friends). It is strictly illegal to use
   the fsl_stmt_get_xxx() APIs unless fsl_stmt_step() has returned
   this code.
*/
FSL_RC_STEP_ROW = 115,

/**
   Indicates that fsl_stmt_step() has reached the end of the result
   set and that there is no row data to process. This is also the
   result for non-fetching queries (INSERT and friends). It is strictly
   illegal to use the fsl_stmt_get_xxx() APIs after fsl_stmt_step() has
   returned this code.
*/
FSL_RC_STEP_DONE = 116,

/**
   Indicates that a db-level error occurred during a
   fsl_stmt_step() iteration.
*/
FSL_RC_STEP_ERROR = 117,

/**
   Indicates that some data type or logical type is incorrect
   (e.g. an invalid card type in conjunction with a given
   fsl_deck).
*/
FSL_RC_TYPE = 118,

/**
   Indicates that an operation which requires a checkout does not
   have a checkout to work on.
*/
FSL_RC_NOT_A_CKOUT = 119,

/**
   Indicates that a repo and checkout do not belong together.
*/
FSL_RC_REPO_MISMATCH = 120,
/**
   Indicates that a checksum comparison failed, possibly indicating
   that corrupted or unexpected data was just read.
*/
FSL_RC_CHECKSUM_MISMATCH = 121,

/**
   Indicates that a merge conflict, or some other context-dependent
   type of conflict, was detected.
*/
FSL_RC_CONFLICT,

/**
   This is a special case of FSL_RC_NOT_FOUND, intended specifically
   to differentiate from "file not found in filesystem"
   (FSL_RC_NOT_FOUND) and "fossil does not know about this file" in
   routines for which both might be an error case. An example is a
   an operation which wants to update a repo file with contents
   from the filesystem - the file might not exist or it might not be
   in the current repo db.

   That said, this can also be used for APIs which search for other
   resources (UUIDs, tickets, etc.), but FSL_RC_NOT_FOUND is already
   fairly well entrenched in those cases and is unambiguous, so this
   code is only needed by APIs for which both cases described above
   might happen.
*/
FSL_RC_UNKNOWN_RESOURCE,

/**
   Indicates that a size comparison check failed.

   TODO: remove this if it is not used.
*/
FSL_RC_SIZE_MISMATCH,

/**
   Indicates that an invalid separator was encountered while
   parsing a delta.
*/
FSL_RC_DELTA_INVALID_SEPARATOR,

/**
   Indicates that an invalid size value was encountered while
   parsing a delta.
*/
FSL_RC_DELTA_INVALID_SIZE,

/**
   Indicates that an invalid operator was encountered while parsing
   a delta.
*/
FSL_RC_DELTA_INVALID_OPERATOR,

/**
   Indicates that an invalid terminator was encountered while
   parsing a delta.
*/
FSL_RC_DELTA_INVALID_TERMINATOR,

/**
   Indicates a generic syntax error in a structural artifact. Some
   types of manifest-releated errors are reported with more specific
   error codes, e.g. FSL_RC_RANGE if a given card type appears too
   often.
*/
FSL_RC_SYNTAX,

/**
   Indicates that some value or expression is ambiguous. Typically
   caused by trying to resolve ambiguous symbolic names or hash
   prefixes to their full hashes.
*/
FSL_RC_AMBIGUOUS,

/**
   Used by fsl_checkin_commit(), and similar operations, to indicate
   that they're failing because they would be no-ops. That would
   normally indicate a "non-error," but a condition the caller
   certainly needs to know about.
*/
FSL_RC_NOOP,
/**
   A special case of FSL_RC_NOT_FOUND which indicates that the
   requested repository blob could not be loaded because it is a
   phantom. That is, the record is found but its contents are not
   available. Phantoms are blobs which fossil knows should exist,
   because it's seen references to their hashes, but for which it does
   not yet have any content.
*/
FSL_RC_PHANTOM,

/**
   Indicates that the requested operation is unsupported.
*/
FSL_RC_UNSUPPORTED,

/**
   Indicates that the requested operation is missing certain required
   information.
*/
FSL_RC_MISSING_INFO,

/**
   Special case of FSL_RC_TYPE triggered in some diff APIs, indicating
   that the API cannot diff what appears to be binary data.
*/
FSL_RC_DIFF_BINARY,
/**
   Triggered by some diff APIs to indicate that only whitespace
   changes we found and the diff was requested to ignore whitespace.
 */
FSL_RC_DIFF_WS_ONLY,

/**
   Must be the final entry in the enum. Used for creating client-side
   result codes which are guaranteed to live outside of this one's
   range.
*/
FSL_RC_end
};
typedef enum fsl_rc_e fsl_rc_e;

/**
   File permissions flags supported by fossil manifests. Their numeric
   values are a hard-coded part of the Fossil architecture and must
   not be changed. Note that these refer to manifest-level permissions
   and not filesystem-level permissions (though they translate to/from
   filesystem-level meanings at some point).
*/
enum fsl_fileperm_e {
/** Indicates a regular, writable file. */
FSL_FILE_PERM_REGULAR = 0,
/** Indicates a regular file with the executable bit set. */
FSL_FILE_PERM_EXE = 0x1,
/**
   Indicates a symlink. Note that symlinks do not have the executable
   bit set separately on Unix systems. Also note that libfossil does
   NOT YET IMPLEMENT symlink support like fossil(1) does - it
   currently treats symlinks (mostly) as Unix treats symlinks.
*/
FSL_FILE_PERM_LINK = 0x2
};
typedef enum fsl_fileperm_e fsl_fileperm_e;

/**
   Returns a "standard" string form for a fsl_rc_e code.  The string
   is primarily intended for debugging purposes.  The returned bytes
   are guaranteed to be static and NUL-terminated. They are not
   guaranteed to contain anything useful for any purposes other than
   debugging and tracking down problems.
*/
FSL_EXPORT char const * fsl_rc_cstr(int);

/**
   Returns the value of FSL_LIBRARY_VERSION used to compile the
   library. If this value differs from the value the caller was
   compiled with, Chaos might ensue.

   The API does not yet have any mechanism for determining
   compatibility between repository versions and it also currently
   does no explicit checking to disallow incompatible versions.
*/
FSL_EXPORT char const * fsl_library_version();

/**
   Returns true (non-0) if yourLibVersion compares lexically
   equal to FSL_LIBRARY_VERSION, else it returns false (0).
*/
FSL_EXPORT bool fsl_library_version_matches(char const * yourLibVersion);

/**
   This type, accessible to clients via the ::fsl_lib_configurable
   global, contains configuration-related data for the library
   which can be swapped out by clients.
*/
struct fsl_lib_configurable_t {
  /**
     Library-wide allocator. It may be replaced by the client IFF
     it is replaced before the library allocates any memory. The
     default implementation uses the C-standard
     de/re/allocators. Modifying this member while any memory
     allocated through it is still "live" leads to undefined
     results. There is an exception: a "read-only" middleman proxy
     which does not change how the memory is allocated or
     intepreted can safely be swapped in or out at any time
     provided the underlying allocator stays the same and the
     client can ensure that there are no thread-related race
     conditions. e.g. it is legal to swap this out with a proxy
     which logs allocation requests and then forwards the call on
     to the original implementation, and it is legal to do so at
     essentially any time. The important thing this that all of the
     library-allocated memory goes through a single underlying
     (de)allocator for the lifetime of the application.
  */
  fsl_allocator allocator;
};
typedef struct fsl_lib_configurable_t fsl_lib_configurable_t;
FSL_EXPORT fsl_lib_configurable_t fsl_lib_configurable;

/**
   A part of the configuration used by fsl_cx_init() and friends.

*/
struct fsl_cx_config {
  /**
     If true, all SQL which goes through the fossil engine
     will be traced to the fsl_output()-configured channel.
  */
  int traceSql;
  /**
     If true, the fsl_print() SQL function will output its output to the
     fsl_output()-configured channel, else it is a no-op.
  */
  int sqlPrint;

  /**
     Specifies the default hash policy.
  */
  fsl_hashpolicy_e hashPolicy;
};

/**
   fsl_cx_config instance initialized with defaults, intended for
   in-struct initialization.
*/
#define fsl_cx_config_empty_m {                 \
    0/*traceSql*/,                              \
    0/*sqlPrint*/,                            \
    FSL_HPOLICY_SHA3/*hashPolicy*/ \
}

/**
   fsl_cx_config instance initialized with defaults, intended for
   copy-initialization.
*/
FSL_EXPORT const fsl_cx_config fsl_cx_config_empty;

/**
   Parameters for fsl_cx_init().
*/
struct fsl_cx_init_opt {
  /**
     The output channel for the Fossil instance.
  */
  fsl_outputer output;
  /**
     Basic configuration parameters.
  */
  fsl_cx_config config;
};


/** Empty-initialized fsl_cx_init_opt instance. */
#define fsl_cx_init_opt_empty_m {fsl_outputer_empty_m, fsl_cx_config_empty_m}
/**
   fsl_cx_init_opt instance initialized to use stdout for output and
   the standard system memory allocator.
*/
#define fsl_cx_init_opt_default_m {fsl_outputer_FILE_m, fsl_cx_config_empty_m}

/** Empty-initialized fsl_cx_init_opt instance. */
FSL_EXPORT const fsl_cx_init_opt fsl_cx_init_opt_empty;

/**
   fsl_cx_init_opt instance initialized to use stdout for output and
   the standard system memory allocator. Used as the default when
   fsl_cx_init() is passed a NULL value for this parameter.
*/
FSL_EXPORT const fsl_cx_init_opt fsl_cx_init_opt_default;

/**
   Allocates a new fsl_cx instance, which must eventually
   be passed to fsl_cx_finalize() to clean it up.
   Normally clients do not need this - they can simply pass
   a pointer to NULL as the first argument to fsl_cx_init()
   to let it allocate an instance for them.
*/
FSL_EXPORT fsl_cx * fsl_cx_malloc();

/**
   Initializes a fsl_cx instance. tgt must be a pointer to NULL,
   e.g.:

   @code
   fsl_cxt * f = NULL; // NULL is important - see below
   int rc = fsl_cx_init( &f, NULL );
   @endcode

   It is very important that f be initialized to NULL _or_ to an
   instance which has been properly allocated and empty-initialized
   (e.g. via fsl_cx_malloc()). If *tgt is NULL, this routine
   allocates the context, else it assumes the caller did. If f
   points to unitialized memory then results are undefined.

   If the second parameter is NULL then default implementations are
   used for the context's output routine and other options. If it
   is not NULL then param->allocator and param->output must be
   initialized properly before calling this function. The contents
   of param are bitwise copied by this function and ownership of
   the returned value is transfered to *tgt in all cases except
   one:

   If passed a pointer to a NULL context and this function cannot
   allocate it, it returns FSL_RC_OOM and does not modify *tgt. In
   this one case, ownership of the context is not changed (as there's
   nothing to change!). On any other result (including errors),
   ownership of param's contents are transfered to *tgt and the client
   is responsible for passing *tgt ot fsl_cxt_finalize() when he is
   done with it. Note that (like in sqlite3), *tgt may be valid memory
   even if this function fails, and the caller must pass it to
   fsl_cx_finalize() whether or not this function succeeds unless it
   fails at the initial OOM (which the client can check by seeing if
   (*tgt) is NULL, but only if he set it to NULL before calling this).

   Returns 0 on success, FSL_RC_OOM on an allocation error,
   FSL_RC_MISUSE if (!tgt). If this function fails, it is illegal to
   use the context object except to pass it to fsl_cx_finalize(), as
   explained above.

   @see fsl_cx_finalize()
   @see fsl_cx_reset()
*/
FSL_EXPORT int fsl_cx_init( fsl_cx ** tgt, fsl_cx_init_opt const * param );

/**
   Clears (most) dynamic state in f, but does not free f and does
   not free "static" state (that set up by the init process). If
   closeDatabases is true then any databases managed by f are
   closed, else they are kept open.

   Client code will not normally need this - it is intended for a
   particular potential memory optimization case. If (and only if)
   closeDatabases is true then after calling this, f may be legally
   re-used as a target for fsl_cx_init().

   This function does not trigger any finializers set for f's
   client state or output channel. It _does_ clear any user name
   set via fsl_cx_user_set().

   Results are undefined if !f or f's memory has not been properly
   initialized.
*/
FSL_EXPORT void fsl_cx_reset( fsl_cx * f, bool closeDatabases );

/**
   Frees all memory associated with f, which must have been
   allocated/initialized using fsl_cx_malloc(), fsl_cx_init(), or
   equivalent, or created on the stack and properly initialized
   (via fsl_cx_init() or copy-constructed from fsl_cx_empty).

   This function triggers any finializers set for f's client state
   or output channel.

   This is a no-op if !f and is effectively a no-op if f has no
   state to destruct.
*/
FSL_EXPORT void fsl_cx_finalize( fsl_cx * f );


/**
   Sets or unsets one or more option flags on the given fossil
   context.  flags is the flag or a bitmask of flags to set (from
   the fsl_cx_flags_e enum).  If enable is true the flag(s) is (are)
   set, else it (they) is (are) unset. Returns the new set of
   flags.
*/
FSL_EXPORT int fsl_cx_flag_set( fsl_cx * f, int flags, bool enable );

/**
   Returns f's flags.
*/
FSL_EXPORT int fsl_cx_flags_get( fsl_cx * f );

/**
   Sets the Fossil error state to the given error code and
   fsl_appendf()-style format string/arguments. On success it
   returns the code parameter. It does not return 0 unless code is
   0, and if it returns a value other than code then something went
   seriously wrong (e.g. allocation error: FSL_RC_OOM) or the
   arguments were invalid: !f results in FSL_RC_MISUSE.

   If !fmt then fsl_rc_cstr(code) is used to create the
   error string.

   As a special case, if code is FSL_RC_OOM, no error string is
   allocated (because it would likely fail, assuming the OOM
   is real).

   As a special case, if code is 0 (the non-error value) then fmt is
   ignored and any error state is cleared.
*/
FSL_EXPORT int fsl_cx_err_set( fsl_cx * f, int code, char const * fmt, ... );

/**
   va_list counterpart to fsl_cx_err_set().
*/
FSL_EXPORT int fsl_cx_err_setv( fsl_cx * f, int code, char const * fmt,
                                va_list args );

/**
   Fetches the error state from f. See fsl_error_get() for the semantics
   of the parameters and return value.
*/
FSL_EXPORT int fsl_cx_err_get( fsl_cx * f, char const ** str, fsl_size_t * len );

/**
   Returns f's error state object. This pointer is guaranteed by the
   API to be stable until f is finalized, but its contents are
   modified my routines as part of the error reporting process.

   Returns NULL if !f.
*/
FSL_EXPORT fsl_error const * fsl_cx_err_get_e(fsl_cx const * f);

/**
   Resets's f's error state, basically equivalent to
   fsl_cx_err_set(f,0,NULL). Is a no-op if f is NULL.  This may be
   necessary for apps if they rely on looking at fsl_cx_err_get()
   at the end of their app/routine, because error state survives
   until it is cleared, even if the error held there was caught and
   recovered. This function might keep error string memory around
   for re-use later on.
*/
FSL_EXPORT void fsl_cx_err_reset(fsl_cx * f);

/**
   Replaces f's error state with the contents of err, taking over
   any memory owned by err (but not err itself). Returns the new
   error state code (the value of err->code before this call) on
   success. The only error case is if !f (FSL_RC_MISUSE). If err is
   NULL then f's error state is cleared and 0 is returned. err's
   error state is cleared by this call.
*/
FSL_EXPORT int fsl_cx_err_set_e( fsl_cx * f, fsl_error * err );

/**
   If f has error state then it outputs its error state to its
   output channel and returns the result of fsl_output(). Returns
   FSL_RC_MISUSE if !f, 0 if f has no error state our output of the
   state succeeds. If addNewline is true then it adds a trailing
   newline to the output, else it does not.

   This is intended for testing and debugging only, and not as an
   error reporting mechanism for a full-fledged application.
*/
FSL_EXPORT int fsl_cx_err_report( fsl_cx * const f, bool addNewline );

/**
   Unconditionally Moves db->error's state into f. If db is NULL then
   f's primary db connection is used. Returns FSL_RC_MISUSE if !f or
   (!db && f-is-not-opened). On success it returns f's new error code.

   The main purpose of this function is to propagate db-level
   errors up to higher-level code which deals directly with the f
   object but not the underlying db(s).

   @see fsl_cx_uplift_db_error2()
*/
FSL_EXPORT int fsl_cx_uplift_db_error( fsl_cx * const f, fsl_db * db );

/**
   If rc is not 0 and f has no error state but db does, this calls
   fsl_cx_uplift_db_error() and returns its result, else returns
   rc. If db is NULL, f's main db connection is used. It is intended
   to be called immediately after calling a db operation which might
   have failed, and passed that operation's result.

   Results are undefined if db is NULL and f has no main db
   connection.
*/
FSL_EXPORT int fsl_cx_uplift_db_error2(fsl_cx * const f, fsl_db * db, int rc);

/**
   Outputs the first n bytes of src to f's configured output
   channel. Returns 0 on success, FSL_RC_MISUSE if (!f || !src),
   0 (without side effects) if !n, else it returns the result of
   the underlying output call. This is a harmless no-op if f is
   configured with no output channel.

   @see fsl_outputf()
   @see fsl_flush()
*/
FSL_EXPORT int fsl_output( fsl_cx * f, void const * src, fsl_size_t n );

/**
   Flushes f's output channel. Returns 0 on success, FSL_RC_MISUSE
   if !f. If the flush routine is NULL then this is a harmless
   no-op.

   @see fsl_outputf()
   @see fsl_output()
*/
FSL_EXPORT int fsl_flush( fsl_cx * f );

/**
   Uses fsl_appendf() to append formatted output to the channel
   configured for use with fsl_output(). Returns 0 on success,
   FSL_RC_MISUSE if !f or !fmt, FSL_RC_RANGE if !*fmt, and
   FSL_RC_IO if the underlying fsl_appendf() operation fails.

   Note, however, that due to the printf()-style return semantics
   of fsl_appendf(), it is not generically possible to distinguish
   a partially-successful (i.e. failed in the middle) write from
   success. e.g. if fmt contains a format specifier which performs
   memory allocation and that allocation fails, it is unlikely that
   this function will be able to be aware of that error. The only
   way to fix that is to change the return semantics of
   fsl_appendf() (and adjust any existing code which relies on
   them).

   @see fsl_output()
   @see fsl_flush()
*/
FSL_EXPORT int fsl_outputf( fsl_cx * f, char const * fmt, ... );

/**
   va_list counterpart to fsl_outputf().
*/
FSL_EXPORT int fsl_outputfv( fsl_cx * f, char const * fmt, va_list args );

/**
   Opens the given db file name as f's repository. Returns 0 on
   success. On error it sets f's error state and returns that code
   unless the error was FSL_RC_MISUSE (which indicates invalid
   arguments and it does not set the error state).

   Fails with FSL_RC_MISUSE if !f, !repoDbFile, !*repoDbFile. Returns
   FSL_RC_ACCESS if f already has an opened repo db.

   Returns FSL_RC_NOT_FOUND if repoDbFile is not found, as this
   routine cannot create a new repository db.

   When a repository is opened, the fossil-level user name
   associated with f (if any) is overwritten with the default user
   from the repo's login table (the one with uid=1). Thus
   fsl_cx_user_get() may return a value even if the client has not
   called fsl_cx_user_set().

   It would be nice to have a parameter specifying that the repo
   should be opened read-only. That's not as straightforward as it
   sounds because of how the various dbs are internally managed
   (via one db handle). Until then, the permissions of the
   underlying repo file will determine how it is opened. i.e. a
   read-only repo will be opened read-only.


   Potentially interesting side-effects:

   - On success this re-sets several bits of f's configuration to
   match the repository-side settings.

   @see fsl_repo_create()
   @see fsl_repo_close()
*/
FSL_EXPORT int fsl_repo_open( fsl_cx * f, char const * repoDbFile/*, char readOnlyCurrentlyIgnored*/ );

/**
   If fsl_repo_open_xxx() or fsl_ckout_open_dir() has been used to
   open a respository db, this call closes that db and returns
   0. Returns FSL_RC_MISUSE if f has any transactions pending,
   FSL_RC_NOT_FOUND if f has not opened a repository.

   If a repository is opened "indirectly" via fsl_ckout_open_dir()
   then attempting to close it using this function will result in
   FSL_RC_MISUSE and f's error state will hold a description of the
   problem. Such a repository will be closed implicitly when the
   checkout db is closed.

   @see fsl_repo_open()
   @see fsl_repo_create()
*/
FSL_EXPORT int fsl_repo_close( fsl_cx * f );

/**
   Sets or clears (if userName is NULL or empty) the default
   repository user name for operations which require one.

   Returns 0 on success, FSL_RC_MISUSE if f is NULL,
   FSL_RC_OOM if copying of the userName fails.

   Example usage:
   @code
   char * u = fsl_guess_user_name();
   int rc = fsl_cx_user_set(f, u);
   fsl_free(u);
   @endcode

   (Sorry about the extra string copy there, but adding a function
   which passes ownership of the name string seems like overkill.)
*/
FSL_EXPORT int fsl_cx_user_set( fsl_cx * f, char const * userName );

/**
   Returns the name set by fsl_cx_user_set(), or NULL if f has no
   default user name set. The returned bytes are owned by f and may
   be invalidated by any call to fsl_cx_user_set().
*/
FSL_EXPORT char const * fsl_cx_user_get( fsl_cx const * f );

/**
   Configuration parameters for fsl_repo_create().  Always
   copy-construct these from fsl_repo_create_opt_empty
   resp. fsl_repo_create_opt_empty_m in order to ensure proper
   behaviour vis-a-vis default values.

   TODOs:

   - Add project name/description, and possibly other
   configuration bits.

   - Allow client to set password for default user (currently set
   randomly, as fossil(1) does).
*/
struct fsl_repo_create_opt {
  /**
     The file name for the new repository.
  */
  char const * filename;
  /**
     Fossil user name for the admin user in the new repo.  If NULL,
     defaults to the Fossil context's user (see
     fsl_cx_user_get()). If that is NULL, it defaults to
     "root" for historical reasons.
  */
  char const * username;

  /**
     The comment text used for the initial commit. If NULL or empty
     (starts with a NUL byte) then no initial check is
     created. fossil(1) is largely untested with that scenario (but
     it seems to work), so for compatibility it is not recommended
     that this be set to NULL.

     The default value (when copy-initialized) is "egg". There's a
     story behind the use of "egg" as the initial checkin comment,
     and it all started with a typo: "initial chicken"
  */
  char const * commitMessage;

  /**
     Mime type for the commit message (manifest N-card). Manifests
     support this but fossil(1) has never (as of 2021-02) made use of
     it. It is provided for completeness but should, for
     compatibility's sake, probably not be set, as the fossil UI may
     not honor it. The implied default is text/x-fossil-wiki. Other
     ostensibly legal values include text/plain and text/x-markdown.
     This API will accept any value, but results are technically
     undefined with any values other than those listed above.
  */
  char const * commitMessageMimetype;

  /**
     If not NULL and not empty, fsl_repo_create() will use this
     repository database to copy the configuration, copying over
     the following settings:

     - The reportfmt table, overwriting any existing entries.

     - The user table fields (cap, info, mtime, photo) are copied
     for the "system users".  The system users are: anonymous,
     nobody, developer, reader.

     - The vast majority of the config table is copied, arguably
     more than it should (e.g. the 'manifest' setting).
  */
  char const * configRepo;

  /**
     If false, fsl_repo_create() will fail if this->filename
     already exists.
  */
  bool allowOverwrite;
  
};
typedef struct fsl_repo_create_opt fsl_repo_create_opt;

/** Initialized-with-defaults fsl_repo_create_opt struct, intended
    for in-struct initialization. */
#define fsl_repo_create_opt_empty_m {           \
    NULL/*filename*/,                           \
    NULL/*username*/,                         \
    "egg"/*commitMessage*/,                   \
    NULL/*commitMessageMimetype*/,            \
    NULL/*configRepo*/,                       \
    false/*allowOverwrite*/                     \
    }

/** Initialized-with-defaults fsl_repo_create_opt struct, intended
    for copy-initialization. */
FSL_EXPORT const fsl_repo_create_opt fsl_repo_create_opt_empty;

/**
   Creates a new repository database using the options provided in the
   second argument. If f is not NULL, it must be a valid context
   instance, though it need not have an opened checkout/repository. If
   f has an opened repo or checkout, this routine closes them but that
   closing _will fail_ if a transaction is currently active!

   If f is NULL, a temporary context is used for creating the
   repository, in which case the caller will not have access to
   detailed error information (only the result code) if this operation
   fails. In that case, the resulting repository file will, on
   success, be found at the location referred to by opt.filename.

   The opt argument may not be NULL.

   If opt->allowOverwrite is false (0) and the file exists, it fails
   with FSL_RC_ALREADY_EXISTS, otherwise is creates/overwrites the
   file. This is a destructive operation if opt->allowOverwrite is
   true, so be careful: the existing database will be truncated and
   re-created.

   This operation installs the various "static" repository schemas
   into the db, sets up some default settings, and installs a
   default user.

   This operation always closes any repository/checkout opened by f
   because setting up the new db requires wiring it to f to set up
   some of the db-side infrastructure. The one exception is if
   argument validation fails, in which case f's repo/checkout-related
   state are not modified. Note that closing will fail if a
   transaction is currently active and that, in turn, will cause this
   operation to fail.

   See the fsl_repo_create_opt docs for more details regarding the
   creation options.

   On success, 0 is returned and f (if not NULL) is left with the
   new repository opened and ready for use. On error, f's error
   state is updated and any number of the FSL_RC_xxx codes may be
   returned - there are no less than 30 different _potential_ error
   conditions on the way to creating a new repository.

   If initialization of the repository fails, this routine will
   attempt to remove its partially-initialize corpse from the
   filesystem but will ignore any errors encountered while doing so.

   Example usage:

   @code
   fsl_repo_create_opt opt = fsl_repo_create_opt_empty;
   int rc;
   opt.filename = "my.fossil";
   // ... any other opt.xxx you want to set, e.g.:
   // opt.user = "fred";
   // Assume fsl is a valid fsl_cx instance:
   rc = fsl_repo_create(fsl, &opt );
   if(rc) { ...error... }
   else {
     fsl_db * db = fsl_cx_db_repo(f);
     assert(db); // == the new repo db
   ...
   }
   @endcode

   @see fsl_repo_open()
   @see fsl_repo_close()
*/
FSL_EXPORT int fsl_repo_create(fsl_cx * f, fsl_repo_create_opt const * opt );

/**
   UNTESTED.

   Returns true if f has an opened repository database which is
   opened in read-only mode, else returns false.
*/
FSL_EXPORT char fsl_repo_is_readonly(fsl_cx const * f);

/**
   Tries to open a checked-out fossil repository db in the given
   directory. The (dirName, checkParentDirs) parameters are passed on
   as-is to fsl_ckout_db_search() to find a checkout db, so see that
   routine for how it searches.

   If this routine finds/opens a checkout, it also tries to open
   the repository database from which the checkout derives (and
   fails if it cannot).

   Returns 0 on success. If there is an error opening or validating
   the checkout or its repository db, f's error state will be
   updated. Error codes/conditions include:

   - FSL_RC_MISUSE if f is NULL.

   - FSL_RC_ACCESS if f already has and opened checkout.

   - FSL_RC_OOM if an allocation fails.

   - FSL_RC_NOT_FOUND if no checkout is foud or if a checkout's
   repository is not found.

   - FSL_RC_RANGE if dirname is not NULL but has a length of 0,
   either because 0 was passed in for dirNameLen or because
   dirNameLen was negative and *dirName is a NUL byte.

   - Various codes from fsl_getcwd() (if dirName is NULL).

   - Various codes if opening the associated repository DB fails.

   TODO: there's really nothing in the architecture which restricts
   a checkout db to being in the same directory as the checkout,
   except for some historical bits which "could" be refactored. It
   "might be interesting" to eventually provide a variant which
   opens a checkout db file directly. We have the infrastructure,
   just need some refactoring. We would need to add the working
   directory path to the checkout db's config, but should otherwise
   require no trickery or incompatibilities with fossil(1).
*/
FSL_EXPORT int fsl_ckout_open_dir( fsl_cx * f, char const * dirName,
                                   bool checkParentDirs );


/**
   Searches the given directory (or the current directory if dirName
   is 0) for a fossil checkout database file named one of (_FOSSIL_,
   .fslckout).  If it finds one, it returns 0 and appends the file's
   path to pOut if pOut is not 0.  If neither is found AND if
   checkParentDirs is true (non-0) an then it moves up the path one
   directory and tries again, until it hits the root of the dirPath
   (see below for a note/caveat).

   If dirName is NULL then it behaves as if it had been passed the
   absolute path of the current directory (as determined by
   fsl_getcwd()).

   This function does no normalization of dirName. Because of that...

   Achtung: if dirName is relative, this routine might not find a
   checkout where it would find one if given an absolute path (because
   it traverses the path string given it instead of its canonical
   form). Wether this is a bug or a feature is not yet clear. When in
   doubt, use fsl_file_canonical_name() to normalize the directory
   name before passing it in here. If it turns out that we always want
   that behaviour, this routine will be modified to canonicalize the
   name.

   This routine can return at least the following error codes:

   - FSL_RC_NOT_FOUND: either no checkout db was found or the given
   directory was not found.

   - FSL_RC_RANGE if dirName is an empty string. (We could arguably
   interpret this as a NULL string, i.e. the current directory.)

   - FSL_RC_OOM if allocation of a filename buffer fails.

*/
FSL_EXPORT int fsl_ckout_db_search( char const * dirName,
                                    bool checkParentDirs,
                                    fsl_buffer * pOut );


/**
   If fsl_ckout_open_dir() (or similar) has been used to open a
   checkout db, this call closes that db and returns 0. Returns
   FSL_RC_MISUSE if f has any transactions pending, FSL_RC_NOT_FOUND
   if f has not opened a checkout (which can safely be ignored and
   does not update f's error state).

   This also closes the repository which was implicitly opened for the
   checkout.
*/
FSL_EXPORT int fsl_ckout_close( fsl_cx * f );

/**
   Attempts to Closes any opened databases (repo/checkout/config). This will
   fail if any transactions are pending. Any databases which are already
   closed are silently skipped.
*/
FSL_EXPORT int fsl_cx_close_dbs( fsl_cx * f );

/**
   If f is not NULL and has a checkout db opened then this function
   returns its name. The bytes are valid until that checkout db
   connection is closed. If len is not NULL then *len is (on
   success) assigned to the length of the returned string, in
   bytes. The string is NUL-terminated, so fetching the length (by
   passing a non-NULL 2nd parameter) is optional.

   Returns NULL if !f or f has no checkout opened.

   @see fsl_ckout_open_dir()
   @see fsl_cx_ckout_dir_name()
   @see fsl_cx_db_file_config()
   @see fsl_cx_db_file_repo()
*/
FSL_EXPORT char const * fsl_cx_db_file_ckout(fsl_cx const * f,
                                             fsl_size_t * len);

/**
   Equivalent to fsl_ckout_db_file() except that
   it applies to the name of the opened repository db,
   if any.

   @see fsl_cx_db_file_ckout()
   @see fsl_cx_db_file_config()
*/
FSL_EXPORT char const * fsl_cx_db_file_repo(fsl_cx const * f,
                                            fsl_size_t * len);

/**
   Equivalent to fsl_ckout_db_file() except that
   it applies to the name of the opened config db,
   if any.

   @see fsl_cx_db_file_ckout()
   @see fsl_cx_db_file_repo()
*/
FSL_EXPORT char const * fsl_cx_db_file_config(fsl_cx const * f,
                                              fsl_size_t * len);

/**
   Similar to fsl_cx_db_file_ckout() and friends except that it
   applies to db file implied by the specified role (2nd
   parameter). If no such role is opened, or the role is invalid,
   NULL is returned.

   Note that the role of FSL_DBROLE_TEMP is invalid here.
*/
FSL_EXPORT char const * fsl_cx_db_file_for_role(fsl_cx const * f,
                                                fsl_dbrole_e r,
                                                fsl_size_t * len);

/**
   Similar to fsl_cx_db_file_ckout() and friends except that it
   applies to DB name (as opposed to DB _file_ name) implied by the
   specified role (2nd parameter). If no such role is opened, or the
   role is invalid, NULL is returned.

   If the 3rd argument is not NULL, it is set to the length, in bytes,
   of the returned string. The returned strings are static and
   NUL-terminated.

   This is the "easiest" way to figure out the DB name of the given
   role, independent of what order f's databases were opened
   (because the first-opened DB is always called "main").

   The Fossil-standard names of its primary databases are: "localdb"
   (checkout), "repository", and "configdb" (global config DB), but
   libfossil uses "ckout", "repo", and "cfg", respective. So long as
   queries use table names which unambiguously refer to a given
   database, the DB name is normally not needed. It is needed when
   creating new non-TEMP db tables and views. By default such
   tables/views would go into the "main" DB, which is actually a
   transient DB in this API, so it's important to use the correct DB
   name when creating such constructs.

   Note that the role of FSL_DBROLE_TEMP is invalid here.
*/
FSL_EXPORT char const * fsl_cx_db_name_for_role(fsl_cx const * f,
                                                fsl_dbrole_e r,
                                                fsl_size_t * len);

/**
   If f has an opened checkout db (from fsl_ckout_open_dir()) then
   this function returns the directory part of the path for the
   checkout, including (for historical and internal convenience
   reasons) a trailing slash. The returned bytes are valid until that
   db connection is closed. If len is not NULL then *len is (on
   success) assigned to the length of the returned string, in bytes.
   The string is NUL-terminated, so fetching the length by passing a
   non-NULL 2nd parameter is optional.

   Returns NULL if !f or f has no checkout opened.

   @see fsl_ckout_open_dir()
   @see fsl_ckout_db_file()
*/
FSL_EXPORT char const * fsl_cx_ckout_dir_name(fsl_cx const * f,
                                              fsl_size_t * len);

/**
   Returns a handle to f's main db (which may or may not have any
   relationship to the repo/checkout/config databases - that's
   unspecified!), or NULL if !f. The returned object is owned by f and
   the client MUST NOT do any of the following:

   - Close the db handle.

   - Use transactions without using fsl_db_transaction_begin()
   and friends.

   - Fiddle with the handle's internals. Doing so might confuse its
   owning context.

   Clients MAY add new user-defined functions, use the handle with
   fsl_db_prepare(), and other "mundane" db-related tasks.

   Design notes:

   The current architecture uses an in-memory db as the "main" db and
   attaches the repo, checkout, and config dbs using well-defined
   names. Even so, it uses separate fsl_db instances to track each one
   so that we "could," if needed, switch back to a multi-db-instance
   approach if needed.

   @see fsl_cx_db_repo()
   @see fsl_cx_db_ckout()
*/
FSL_EXPORT fsl_db * fsl_cx_db( fsl_cx * const f );

/**
   If f is not NULL and has had its repo opened via
   fsl_repo_open(), fsl_ckout_open_dir(), or similar, this
   returns a pointer to that database, else it returns NULL.

   @see fsl_cx_db()
*/
FSL_EXPORT fsl_db * fsl_cx_db_repo( fsl_cx * const f );

/**
   If f is not NULL and has had a checkout opened via
   fsl_ckout_open_dir() or similar, this returns a pointer to that
   database, else it returns NULL.

   @see fsl_cx_db()
*/
FSL_EXPORT fsl_db * fsl_cx_db_ckout( fsl_cx * const f );

/**
   A helper which fetches f's repository db. If f has no repo db
   then it sets f's error state to FSL_RC_NOT_A_REPO with a message
   describing the requirement, then returns NULL.  Returns NULL if
   !f.

   @see fsl_cx_db()
   @see fsl_cx_db_repo()
   @see fsl_needs_ckout()
*/
FSL_EXPORT fsl_db * fsl_needs_repo(fsl_cx * const f);

/**
   The checkout-db counterpart of fsl_needs_repo().

   @see fsl_cx_db()
   @see fsl_needs_repo()
   @see fsl_cx_db_ckout()
*/
FSL_EXPORT fsl_db * fsl_needs_ckout(fsl_cx * const f);

/**
   Opens the given database file as f's configuration database. If f
   already has a config database opened, it is closed before opening
   the new one. The database is created and populated with an
   initial schema if needed.

   If dbName is NULL or empty then it uses a default db name,
   "probably" under the user's home directory. To get the name of
   the database after it has been opened/attached, use
   fsl_cx_db_file_config().

   TODO: strongly consider supporting non-attached
   (i.e. sqlite3_open()'d) use of the config db. Comments in fossil(1)
   suggest that it is possible to lock the config db for other apps
   when it is attached to a long-running op by a fossil process.

   @see fsl_cx_db_config()
   @see fsl_config_close()
*/
FSL_EXPORT int fsl_config_open( fsl_cx * f, char const * dbName );

/**
   Closes/detaches the database connection opened by
   fsl_config_open(). Returns 0 on succes, FSL_RC_MISUSE if !f,
   FSL_RC_NOT_FOUND if no config db connection is opened/attached.

   @see fsl_cx_db_config()
   @see fsl_config_open()
*/
FSL_EXPORT int fsl_config_close( fsl_cx * f );

/**
   If f has an opened/attached configuration db then its handle is
   returned, else 0 is returned.

   @see fsl_config_open()
   @see fsl_config_close()
*/
FSL_EXPORT fsl_db * fsl_cx_db_config( fsl_cx * const f );

/**
   Convenience form of fsl_db_prepare() which uses f's main db.
   Returns 0 on success, FSL_RC_MISUSE if !f or !sql, FSL_RC_RANGE
   if !*sql.
*/
FSL_EXPORT int fsl_cx_prepare( fsl_cx *f, fsl_stmt * tgt, char const * sql, ... );

/**
   va_list counterpart of fsl_cx_prepare().
*/
FSL_EXPORT int fsl_cx_preparev( fsl_cx *f, fsl_stmt * tgt, char const * sql, va_list args );

/**
   Wrapper around fsl_db_last_insert_id() which uses f's main
   database. Returns -1 if !f or f has no opened db.

   @see fsl_cx_db()
*/
FSL_EXPORT fsl_id_t fsl_cx_last_insert_id(fsl_cx *f);


/**
   Works similarly to fsl_stat(), except that zName must refer to a
   path under f's current checkout directory. Note that this stats
   local files, not repository-level content.

   If relativeToCwd is true then the filename is
   resolved/canonicalized based on the current working directory (see
   fsl_getcwd()), otherwise f's current checkout directory is used as
   the virtual root. This makes a subtle yet important difference in
   how the name is resolved. Applications taking input from users
   (e.g. CLI apps) will normally want to resolve from the current
   working dir (assuming the filenames were passed in from the
   CLI). In a GUI environment, where the current directory is likely
   not the checkout root, resolving based on the checkout root
   (i.e. relativeToCwd=false) is probably saner.

   Returns 0 on success. Errors include, but are not limited to:

   - FSL_RC_MISUSE if !zName.

   - FSL_RC_NOT_A_CKOUT if f has no opened checkout.

   - If fsl_is_simple_pathname(zName) returns false then
   fsl_ckout_filename_check() is used to normalize the name. If
   that fails, its failure code is returned.

   - As for fsl_stat().

   See fsl_stat() for more details regarding the tgt parameter.

   TODO: fossil-specific symlink support. Currently it does not
   distinguish between symlinks and non-links.

   @see fsl_cx_stat2()
*/
FSL_EXPORT int fsl_cx_stat( fsl_cx * f, bool relativeToCwd,
                            char const * zName, fsl_fstat * tgt );

/**
   This works identically to fsl_cx_stat(), but provides more
   information about the file being stat'd.

   If nameOut is not NULL then the resolved/normalized path to to
   that file is appended to nameOut. If fullPath is true then an
   absolute path is written to nameOut, otherwise a
   checkout-relative path is written.

   Returns 0 on success. On stat() error, nameOut is not updated,
   but after stat()'ing, allocation of memory for nameOut's buffer
   may fail.

   If zName ends with a trailing slash, that slash is retained in
   nameOut.

   This function DOES NOT resolve symlinks, stat()nig the link instead
   of what it points to.

   @see fsl_cx_stat()
*/
FSL_EXPORT int fsl_cx_stat2( fsl_cx * f, bool relativeToCwd, char const * zName,
                             fsl_fstat * tgt, fsl_buffer * nameOut, bool fullPath);


/**
   Sets the case-sensitivity flag for f to the given value. This
   flag alters how some filename-search/comparison operations
   operate. This option is only intended to have an effect on
   plaforms with case-insensitive filesystems.

   @see fsl_cx_is_case_sensitive()
*/
FSL_EXPORT void fsl_cx_case_sensitive_set(fsl_cx * f, bool caseSensitive);

/**
   Returns true (non-0) if f is set for case-sensitive filename
   handling, else 0. Returns 0 if !f.

   @see fsl_cx_case_sensitive_set()
*/
FSL_EXPORT bool fsl_cx_is_case_sensitive(fsl_cx const * f);

/**
   If f is set to use case-sensitive filename handling,
   returns a pointer to an empty string, otherwise a pointer
   to the string "COLLATE nocase" is returned.
   Results are undefined if f is NULL. The returned bytes
   are static. 

   @see fsl_cx_case_sensitive_set()
   @see fsl_cx_is_case_sensitive()
*/
FSL_EXPORT char const * fsl_cx_filename_collation(fsl_cx const * f);

/**
   An enumeration of the types of structural artifacts used by
   Fossil. The numeric values of all entries before FSL_SATYPE_count,
   with the exception of FSL_SATYPE_INVALID, are a hard-coded part of
   the Fossil db architecture and must never be changed. Any after
   FSL_SATYPE_count are libfossil extensions.
*/
enum fsl_satype_e {
/**
   Sentinel value used for some error reporting.
*/
FSL_SATYPE_INVALID = -1,
/**
   Sentinel value used to mark a deck as being "any" type. This is
   a placeholder on a deck's way to completion.
*/
FSL_SATYPE_ANY = 0,
/**
   Indicates a "manifest" artifact (a checkin record).
*/
FSL_SATYPE_CHECKIN = 1,
/**
   Indicates a "cluster" artifact. These are used during synchronization.
*/
FSL_SATYPE_CLUSTER = 2,
/**
   Indicates a "control" artifact (a tag change).
*/
FSL_SATYPE_CONTROL = 3,
/**
   Indicates a "wiki" artifact.
*/
FSL_SATYPE_WIKI = 4,
/**
   Indicates a "ticket" artifact.
*/
FSL_SATYPE_TICKET = 5,
/**
   Indicates an "attachment" artifact (used in the ticketing
   subsystem).
*/
FSL_SATYPE_ATTACHMENT = 6,
/**
   Indicates a technote (formerly "event") artifact (kind of like a
   blog entry).
*/
FSL_SATYPE_TECHNOTE = 7,
/** Historical (deprecated) name for FSL_SATYPE_TECHNOTE. */
FSL_SATYPE_EVENT = 7,
/**
   Indicates a forum post artifact (a close relative of wiki pages).
*/
FSL_SATYPE_FORUMPOST = 8,
/**
   The number of CATYPE entries. Must be last in the enum. Used for loop
   control.
*/
FSL_SATYPE_count,

/**
   A pseudo-type for use with fsl_sym_to_rid() which changes the
   behavior of checkin lookups to return the RID of the start of the
   branch rather than the tip, with the caveat that the results are
   unspecified if the given symbolic name refers to multiple
   branches.

   fsl_satype_event_cstr() returns the same as FSL_SATYPE_CHECKIN for
   this entry.

   This entry IS NOT VALID for most APIs which require a fsl_satype_e
   value.
*/
FSL_SATYPE_BRANCH_START = 100 // MUST come after FSL_SATYPE_count
};
typedef enum fsl_satype_e fsl_satype_e;

/**
   Returns some arbitrary but distinct string for the given
   fsl_satype_e. The returned bytes are static and
   NUL-terminated. Intended primarily for debugging and informative
   purposes, not actual user output.
*/
FSL_EXPORT char const * fsl_satype_cstr(fsl_satype_e t);

/**
   For a given artifact type, it returns the key string used in the
   event.type db table. Returns NULL if passed an unknown value or
   a type which is not used in the event table, otherwise the
   returned bytes are static and NUL-terminated.

   The returned strings for a given type are as follows:

   - FSL_SATYPE_ANY returns "*"
   - FSL_SATYPE_CHECKIN and FSL_SATYPE_BRANCH_START return "ci"
   - FSL_SATYPE_WIKI returns "w"
   - FSL_SATYPE_TAG returns "g"
   - FSL_SATYPE_TICKET returns "t"
   - FSL_SATYPE_EVENT returns "e"

   The other control artifact types to not have representations
   in the event table, and NULL is returned for them.

   All of the returned values can be used in comparison clauses in
   queries on the event table's 'type' field (but use GLOB instead
   of '=' so that the "*" returned by FSL_ATYPE_ANY can match!).
   For example, to get the comments from the most recent 5 commits:

   @code
   SELECT
   datetime(mtime),
   coalesce(ecomment,comment),
   user
   FROM event WHERE type='ci'
   ORDER BY mtime DESC LIMIT 5; 
   @endcode

   Where 'ci' in the SQL is the non-NULL return value from this
   function. When escaping this value via fsl_buffer_appendf() (or
   anything functionally similar), use the %%q/%%Q format
   specifiers to escape it.
*/
FSL_EXPORT char const * fsl_satype_event_cstr(fsl_satype_e t);  
/**
   A collection of bitmaskable values indicating categories
   of fossil-standard glob sets. These correspond to the following
   configurable settings:

   ignore-glob, crnl-glob, binary-glob
*/
enum fsl_glob_category_t{
/** Corresponds to the ignore-glob config setting. */
FSL_GLOBS_IGNORE = 0x01,
/** Corresponds to the crnl-glob config setting. */
FSL_GLOBS_CRNL = 0x02,
/** Corresponds to the binary-glob config setting. */
FSL_GLOBS_BINARY = 0x04,
/** A superset of all config-level glob categories. */
FSL_GLOBS_ANY = 0xFF
};
typedef enum fsl_glob_category_t fsl_glob_category_t;

/**
   Checks one or more of f's configurable glob lists to see if str
   matches one of them. If it finds a match, it returns a pointer to
   the matching glob (as per fsl_glob_list_matches()), the bytes
   of which are owned by f and may be invalidated via modification
   or reloading of the underlying glob list. In generally the return
   value can be used as a boolean - clients generally do not need
   to know exactly which glob matched.

   gtype specifies the glob list(s) to check in the form of a
   bitmask of fsl_glob_category_t values. Note that the order of the
   lists is unspecified, so if that is important for you then be
   sure that gtype only specifies one glob list
   (e.g. FSL_GLOBS_IGNORE) and call it again (e.g. passing
   FSL_GLOBS_BINARY) if you need to distinguish between those two
   cases.

   str must be a non-NULL, non-empty empty string.

   Returns NULL if !f, !str, !*str, gtype does not specify any known
   glob list(s), or no glob match is found.

   Performance is, abstractly speaking, horrible, because we're
   comparing arbitrarily long lists of glob patterns against an
   arbitrary string. That said, it's fast enough for our purposes.
*/
FSL_EXPORT char const * fsl_cx_glob_matches( fsl_cx * f, int gtype,
                                             char const * str );

/**
   Sets f's hash policy and returns the previous value. If f has a
   repository db open then the setting is stored there and any error
   in setting it is placed into f's error state but otherwise ignored
   for purposes of this call.

   If p is FSL_HPOLICY_AUTO *and* the current repository contains any
   SHA3-format hashes, the policy is interpreted as FSL_HPOLICY_SHA3.

   This value is a *suggestion*, and may be trumped by various
   conditions, in particular in repositories containing older (SHA1)
   hashes.
*/
FSL_EXPORT fsl_hashpolicy_e fsl_cx_hash_policy_set(fsl_cx *f, fsl_hashpolicy_e p);

/**
   Returns f's current hash policy.
*/
FSL_EXPORT fsl_hashpolicy_e fsl_cx_hash_policy_get(fsl_cx const*f);

/**
   Returns a human-friendly name for the given policy, or NULL for an
   invalid policy value. The returned strings are the same ones used
   by fossil's hash-policy command.
*/
FSL_EXPORT char const * fsl_hash_policy_name(fsl_hashpolicy_e p);

/**
   Hashes all of pIn, appending the hash to pOut. Returns 0 on succes,
   FSL_RC_OOM if allocation of space in pOut fails. The hash algorithm
   used depends on the given fossil context's current hash policy and
   the value of the 2nd argument:

   If the 2nd argument is false, the hash is performed per the first
   argument's current hash policy. If the 2nd argument is true, the
   hash policy is effectively inverted. e.g. if the context prefers
   SHA3 hashes, the alternate form will use SHA1.

   Returns FSL_RC_UNSUPPORTED, without updating f's error state, if
   the hash is not possible due to conflicting values for the policy
   and its alternate. e.g. a context with policy FSL_HPOLICY_SHA3_ONLY
   will refuse to apply an SHA1 hash. Whether or not this result can
   be ignored is context-dependent, but it normally can be. This
   result is only possible when the 2nd argument is true.

   Returns 0 on success.
*/
FSL_EXPORT int fsl_cx_hash_buffer( const fsl_cx * f, bool useAlternate,
                                   fsl_buffer const * pIn,
                                   fsl_buffer * pOut);
/**
   The file counterpart of fsl_cx_hash_buffer(), behaving exactly the
   same except that its data source is a file and it may return
   various error codes from fsl_buffer_fill_from_filename(). Note that
   the contents of the file, not its name, are hashed.
*/
FSL_EXPORT int fsl_cx_hash_filename( fsl_cx * f, bool useAlternate,
                                     const char * zFilename, fsl_buffer * pOut);

/**
   Works like fsl_getcwd() but updates f's error state on error and
   appends the current directory's name to the given buffer. Returns 0
   on success.
*/
FSL_EXPORT int fsl_cx_getcwd(fsl_cx * f, fsl_buffer * pOut);

/**
   Returns the same as passing fsl_cx_db() to
   fsl_db_transaction_level(), or 0 if f has no db opened.

   @see fsl_cx_db()
*/
FSL_EXPORT int fsl_cx_transaction_level(fsl_cx * f);
/**
   Returns the same as passing fsl_cx_db() to
   fsl_db_transaction_begin().
*/
FSL_EXPORT int fsl_cx_transaction_begin(fsl_cx * f);
/**
   Returns the same as passing fsl_cx_db() to
   fsl_db_transaction_end().
*/
FSL_EXPORT int fsl_cx_transaction_end(fsl_cx * f, bool doRollback);

/**
   Installs or (if f is NULL) uninstalls a confirmation callback for
   use by operations on f which require user confirmation. The exact
   implications of *not* installing a confirmer depend on the
   operation in question: see fsl_cx_confirm().

   The 2nd argument bitwise copied into f's internal confirmer
   object. If the 2nd argument is NULL, f's confirmer is cleared,
   which will cause fsl_cx_confirm() to use certain default responses
   (see that function for details).

   If the final argument is not NULL then the previous confirmer is
   bitwise copied to it.

   @see fsl_confirm_callback_f
   @see fsl_cx_confirm()
   @see fsl_cx_confirmer_get()
*/
FSL_EXPORT void fsl_cx_confirmer(fsl_cx * f,
                                 fsl_confirmer const * newConfirmer,
                                 fsl_confirmer * prevConfirmer);
/**
   Stores a bitwise copy of f's current confirmer object into *dest. Can
   be used to save the confirmer before temporarily swapping it out.

   @see fsl_cx_confirmer()
*/
FSL_EXPORT void fsl_cx_confirmer_get(fsl_cx const * f, fsl_confirmer * dest);

/**
   If fsl_cx_confirmer() was used to install a confirmer callback in f
   then this routine calls that confirmer and returns its result code
   and its answer via *outAnswer. If no confirmer is currently
   installed, it responds with default answers, depending on the
   eventId:

   - FSL_CEVENT_OVERWRITE_MOD_FILE: FSL_CRESPONSE_NEVER

   - FSL_CEVENT_OVERWRITE_UNMGD_FILE: FSL_CRESPONSE_NEVER

   - FSL_CEVENT_RM_MOD_UNMGD_FILE: FSL_CRESPONSE_NEVER

   - FSL_CEVENT_MULTIPLE_VERSIONS: FSL_CRESPONSE_CANCEL

   Those are not 100% set in stone and are up for reconsideration.

   If a confirmer has been installed, this function does not modify
   outAnswer->response if the installed confirmer does not. Thus
   routines should set it to some acceptable default/sentinel value
   before calling this, to account for callbacks which ignore the
   given detail->eventId.

   If a confirmer callback responds with FSL_CRESPONSE_ALWAYS or
   FSL_CRESPONSE_NEVER, the code which is requesting confirmation must
   honor that by *NOT* calling the callback again for the current
   processing step of that eventId. e.g. if a loop asks for
   confirmation of FSL_CEVENT_RM_MOD_FILE and any response is one of
   the above, that one loop must not ask for confirmation again, and
   must instead accept that response for future queries within the
   same logical library operation (e.g. one checkout-update
   cycle). This is particularly important for applications which
   interactively present the question to the user for confirmation so
   that users have a way to *not* get spammed with a confirmation
   message showing up for each and every one of an arbitrary number of
   confirmations.

   @see fsl_confirm_callback_f
   @see fsl_cx_confirmer()
*/
FSL_EXPORT int fsl_cx_confirm(fsl_cx *f, fsl_confirm_detail const * detail,
                              fsl_confirm_response *outAnswer);

#if 0
/**
   DO NOT USE - not yet tested and ready.

   Returns the result of either localtime(clock) or gmtime(clock),
   depending on f:

   - If f is NULL, returns localtime(clock).

   - If f has had its FSL_CX_F_LOCALTIME_GMT flag set (see
   fsl_cx_flag_set()) then returns gmtime(clock), else
   localtime(clock).

   If clock is NULL, NULL is returned.

   Note that fsl_cx instances default to using UTC for everything,
   which is the opposite of fossil(1).
*/
FSL_EXPORT struct tm * fsl_cx_localtime( fsl_cx const * f, const time_t * clock );

/**
   Equivalent to fsl_cx_localtime(NULL, clock).
*/
FSL_EXPORT struct tm * fsl_localtime( const time_t * clock );

/**
   DO NOT USE - not yet tested and ready.

   This function passes (f, clock) to fsl_cx_localtime(),
   then returns the result of mktime(3) on it. So...
   it adjusts a UTC Unix timestamp to either the same UTC
   local timestamp or to the local time.
*/
FSL_EXPORT time_t fsl_cx_time_adj(fsl_cx const * f, time_t clock);
#endif

#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_CORE_H_INCLUDED */
/* end of file ../include/fossil-scm/fossil-core.h */
/* start of file ../include/fossil-scm/fossil-db.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_DB_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_DB_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).


  ******************************************************************************
  This file declares public APIs for working with fossil's database
  abstraction layer.
*/

/*
  We don't _really_ want to include sqlite3.h at this point, but if we
  do not then we have to typedef the sqlite3 struct here and that
  breaks when client code includes both this file and sqlite3.h.
*/
#include "sqlite3.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
   Potential TODO. Maybe not needed - v1 uses only(?) 1 hook and we
   can do that w/o hooks.
*/
typedef int (*fsl_commit_hook_f)( void * state );
/** Potential TODO */
struct fsl_commit_hook {
  fsl_commit_hook_f hook;
  int sequence;
  void * state;
};
#define fsl_commit_hook_empty_m {NULL,0,NULL}
typedef struct fsl_commit_hook fsl_commit_hook;
/* extern const fsl_commit_hook fsl_commit_hook_empty; */

/**
   Potential TODO.
*/
FSL_EXPORT int fsl_db_before_commit_hook( fsl_db * db, fsl_commit_hook_f f,
                               int sequence, void * state );


#if 0
/* We can't do this because it breaks when clients include both
   this header and sqlite3.h. Is there a solution which lets us
   _not_ include sqlite3.h from this file and also compiles when
   clients include both?
*/
#if !defined(SQLITE_OK)
/**
   Placeholder for sqlite3/4 type. We currently use v3 but will
   almost certainly switch to v4 at some point. Before we can do
   that we need an upgrade/migration path.
*/
typedef struct sqlite3 sqlite3;
#endif
#endif

/**
   A level of indirection to "hide" the actual db driver
   implementation from the public API. Whether or not the API
   uses/will use sqlite3 or 4 is "officially unspecified."  We
   currently use 3 because (A) it bootstraps development and
   testing by letting us use existing fossil repos for and (B) it
   reduces the number of potential problems when porting SQL-heavy
   code from the v1 tree. Clients should try not to rely on the
   underlying db driver API, but may need it for some uses
   (e.g. binding custom SQL functions).
*/
typedef sqlite3 fsl_dbh_t;

/**
   Db handle wrapper class. Each instance wraps a single sqlite
   database handle.

   Fossil is built upon sqlite3, but this abstraction is intended
   to hide that, insofar as possible, from clients so as to
   simplify an eventual port from v3 to v4. Clients should avoid
   relying on the underlying db being sqlite (or at least not rely
   on a specific version), but may want to register custom
   functions with the driver (or perform similar low-level
   operations) and the option is left open for them to access that
   handle via the fsl_db::dbh member.

   @see fsl_db_open();
   @see fsl_db_close();
   @see fsl_stmt
*/
struct fsl_db {
  /**
     Fossil Context on whose behalf this instance is operating, if
     any. Certain db operations behave differently depending
     on whether or not this is NULL.
  */
  fsl_cx * f;

  /**
     Describes what role(s) this db connection plays in fossil (if
     any). This is a bitmask of fsl_dbrole_e values, and a db
     connection may have multiple roles. This is only used by the
     fsl_cx-internal API.
  */
  int role;

  /**
     Underlying db driver handle.
  */
  fsl_dbh_t * dbh;

  /**
     Holds error state from the underlying driver.  fsl_db and
     fsl_stmt operations which fail at the driver level "should"
     update this state to include error info from the driver.
     fsl_cx APIs which fail at the DB level uplift this (using
     fsl_error_move()) so that they can pass it on up the call chain.
  */
  fsl_error error;

  /**
     Holds the file name used when opening this db. Might not refer to
     a real file (e.g. might be ":memory:" or "" (similar to
     ":memory:" but may swap to temp storage).

     Design note: we currently hold the name as it is passed to the
     db-open routine, without canonicalizing it. That is very possibly
     a mistake, as it makes it impossible to properly compare the name
     to another arbitrary checkout-relative name for purposes of
     fsl_reserved_fn_check(). For purposes of fsl_cx we currently
     (2021-03-12) canonicalize db's which we fsl_db_open(), but not
     those which we ATTACH (which includes the repo and checkout
     dbs). We cannot reasonably canonicalize the repo db filename
     because it gets written into the checkout db so that the checkout
     knows where to find the repository. History has shown that that
     path needs to be stored exactly as a user entered it, which is
     often relative.
  */
  char * filename;

  /**
     Holds the database name for use in creating queries.
     Might or might not be set/needed, depending on
     the context.
  */
  char * name;

  /**
     Debugging/test counter. Closing a db with opened statements
     might assert() or trigger debug output when the db is closed.
  */
  int openStatementCount;

  /**
     Counter for fsl_db_transaction_begin/end().
  */
  int beginCount;

  /**
     Internal flag for communicating rollback state through the
     call stack. If this is set to a true value,
     fsl_db_transaction_end() calls will behave like a rollback
     regardless of the value of the 2nd argument passed to that
     function. i.e. it propagates a rollback through nested
     transactions.

     Potential TODO: instead of treating this like a boolean, store
     the error number which caused the rollback here.  We'd have to
     go fix a lot of code for that, though :/.
  */
  int doRollback;

  /**
     Internal change counter. Set when a transaction is
     started/committed.

     Maintenance note: it's an int because that's what
     sqlite3_total_changes() returns.
  */
  int priorChanges;

  /**
     List of SQL commands (char *) which should be executed prior
     to a commit. This list is cleared when the transaction counter
     drops to zero as the result of fsl_db_transaction_end()
     or fsl_db_rollback_force().

     TODO? Use (fsl_stmt*) objects instead of strings? Depends on
     how much data we need to bind here (want to avoid an extra
     copy if we need to bind big stuff). That was implemented in
     [9d9375ac2d], but that approach prohibits multi-statement
     pre-commit triggers, so it was not trunked. It's still unknown
     whether we need multi-statement SQL in this context
     (==fossil's infrastructure).

     @see fsl_db_before_commit()
  */
  fsl_list beforeCommit;

  /**
     An internal cache of "static" queries - those which do not
     rely on call-time state unless that state can be
     bind()ed. Holds a linked list of (fsl_stmt*) instances.

     @see fsl_db_prepare_cached()
  */
  fsl_stmt * cacheHead;

  /**
     A marker which tells fsl_db_close() whether or not
     fsl_db_malloc() allocated this instance (in which case
     fsl_db_close() will fsl_free() it) or not (in which case it
     does not free() it).
  */
  void const * allocStamp;
};
/**
   Empty-initialized fsl_db structure, intended for const-copy
   initialization.
*/
#define fsl_db_empty_m {                        \
    NULL/*f*/,                                  \
      FSL_DBROLE_NONE,                         \
      NULL/*dbh*/,                              \
      fsl_error_empty_m /*error*/,              \
      NULL/*filename*/,                         \
      NULL/*name*/,                             \
      0/*openStatementCount*/,                  \
      0/*beginCount*/,                          \
      0/*doRollback*/,                          \
      0/*priorChanges*/,                        \
      fsl_list_empty_m/*beforeCommit*/,         \
      NULL/*cacheHead*/,                        \
      NULL/*allocStamp*/                        \
      }

/**
   Empty-initialized fsl_db structure, intended for copy
   initialization.
*/
FSL_EXPORT const fsl_db fsl_db_empty;

/**
   If db is not NULL then this function returns its name (the one
   used to open it). The bytes are valid until the db connection is
   closed or until someone mucks with db->filename. If len is not
   NULL then *len is (on success) assigned to the length of the
   returned string, in bytes.  The string is NUL-terminated, so
   fetching the length (by passing a non-NULL 2nd parameter) is
   optional but sometimes helps improve efficiency be removing
   the need for a downstream call to fsl_strlen().

   Returns NULL if !f or f has no checkout opened.
*/
FSL_EXPORT char const * fsl_db_filename(fsl_db const * db, fsl_size_t * len);

typedef sqlite3_stmt fsl_stmt_t;
/**
   Represents a prepared statement handle.
   Intended usage:

   @code
   fsl_stmt st = fsl_stmt_empty;
   int rc = fsl_db_prepare( db, &st, "..." );
   if(rc){ // Error!
   assert(!st.stmt);
   // db->error might hold driver-level error details.
   }else{
   // use st and eventually finalize it:
   fsl_stmt_finalize( &st );
   }
   @endcode


   Script binding implementations can largely avoid exposing the
   statement handle (and its related cleanup ordering requirements)
   to script code. They need to have some mechanism for binding
   values to SQL (or implement all the escaping themselves), but
   that can be done without exposing all of the statement class if
   desired. For example, here's some hypothetical script code:

   @code
   var st = db.prepare(".... where i=:i and x=:x");
   // st is-a Statement, but we need not add script bindings for
   // the whole Statement.bind() API. We can instead simplify that
   // to something like:
   try {
   st.exec( {i: 42, x: 3} )
   // or, for a SELECT query:
   st.each({
   bind{i:42, x:3},
   rowType: 'array', // or 'object'
   callback: function(row,state,colNames){ print(row.join('\t')); },
   state: {...callback function state...}
   });
   } finally {
   st.finalize();
   // It is critical that st gets finalized before its DB, and
   // that'shard to guaranty if we leave st to the garbage collector!
   }
   // see below for another (less messy) alternative
   @endcode

   Ideally, script code should not have direct access to the
   Statement because managing lifetimes can be difficult in the
   face of flow-control changes caused by exceptions (as the above
   example demonstrates). Statements can be completely hidden from
   clients if the DB wrapper is written to support it. For example,
   in pseudo-JavaScript that might look like:

   @code
   db.exec("...where i=? AND x=?", 42, 3);
   db.each({sql:"select ... where id<?", bind:[10],
   rowType: 'array', // or 'object'
   callback: function(row,state,colNames){ print(row.join('\t')); },
   state: {...arbitrary state for the callback...}
   });
   @endcode
*/
struct fsl_stmt {
  /**
     The db which prepared this statement.
  */
  fsl_db * db;

  /**
     Underlying db driver-level statement handle. Clients should
     not rely on the specify concrete type if they can avoid it, to
     simplify an eventual port from sqlite3 to sqlite4.
  */
  fsl_stmt_t * stmt;

  /**
     SQL used to prepare this statement.
  */
  fsl_buffer sql;

  /**
     Number of result columns in this statement. Cached when the
     statement is prepared. Is a signed type because the underlying
     API does it this way.
  */
  int colCount;

  /**
     Number of bound parameter indexes in this statement. Cached
     when the statement is prepared. Is a signed type because the
     underlying API does it this way.
  */
  int paramCount;

  /**
     The number of times this statement has fetched a row via
     fsl_stmt_step().
  */
  fsl_size_t rowCount;

  /**
     Internal state flags.
  */
  int flags;

  /**
     For _internal_ use in creating linked lists. Clients _must_not_
     modify this field.
   */
  fsl_stmt * next;

  /**
     A marker which tells fsl_stmt_finalize() whether or not
     fsl_stmt_malloc() allocated this instance (in which case
     fsl_stmt_finalize() will fsl_free() it) or not (in which case
     it does not free() it).
  */
  void const * allocStamp;
};
/**
   Empty-initialized fsl_stmt instance, intended for use as an
   in-struct initializer.
*/
#define fsl_stmt_empty_m {                      \
    NULL/*db*/,                                 \
      NULL/*stmt*/,                             \
      fsl_buffer_empty_m/*sql*/,                \
      0/*colCount*/,                            \
      0/*paramCount*/,                          \
      0/*rowCount*/,                            \
      1/*flags*/,                               \
      NULL/*next*/,                             \
      NULL/*allocStamp*/                        \
      }

/**
   Empty-initialized fsl_stmt instance, intended for
   copy-constructing.
*/
FSL_EXPORT const fsl_stmt fsl_stmt_empty;

/**
   Allocates a new, cleanly-initialized fsl_stmt instance using
   fsl_malloc(). The returned pointer must eventually be passed to
   fsl_stmt_finalize() to free it (whether or not it is ever passed
   to fsl_db_prepare()).

   Returns NULL on allocation error.
*/
FSL_EXPORT fsl_stmt * fsl_stmt_malloc();


/**
   If db is not NULL this behaves like fsl_error_get(), using the
   db's underlying error state. If !db then it returns
   FSL_RC_MISUSE.
*/
FSL_EXPORT int fsl_db_err_get( fsl_db const * db, char const ** msg, fsl_size_t * len );

/**
   Resets any error state in db, but might keep the string
   memory allocated for later use.
*/
FSL_EXPORT void fsl_db_err_reset( fsl_db * db );

/**
   Prepares an SQL statement for execution. On success it returns
   0, populates tgt with the statement's state, and the caller is
   obligated to eventually pass tgt to fsl_stmt_finalize(). tgt
   must have been cleanly initialized, either via allocation via
   fsl_stmt_malloc() or by copy-constructing fsl_stmt_empty
   resp. fsl_stmt_empty_m (depending on the context).

   On error non-0 is returned and tgt is not modified. If
   preparation of the statement fails at the db level then
   FSL_RC_DB is returned f's error state (fsl_cx_err_get())
   "should" contain more details about the problem. Returns
   FSL_RC_MISUSE if !db, !callback, or !sql. Returns
   FSL_RC_NOT_FOUND if db is not opened. Returns FSL_RC_RANGE if
   !*sql.

   The sql string and the following arguments get routed through
   fsl_appendf(), so any formatting options supported by that
   routine may be used here. In particular, the %%q and %%Q
   formatting options are intended for use in escaping SQL for
   routines such as this one.

   Compatibility note: in sqlite, empty SQL code evaluates
   successfully but with a NULL statement. This API disallows empty
   SQL because it uses NULL as a "no statement" marker and because
   empty SQL is arguably not a query at all.

   Tips:

   - fsl_stmt_col_count() can be used to determine whether a
   statement is a fetching query (fsl_stmt_col_count()>0) or not
   (fsl_stmt_col_count()==0) without having to know the contents
   of the query.

   - fsl_db_prepare_cached() can be used to cache often-used or
   expensive-to-prepare queries within the context of their parent
   db handle.
*/
FSL_EXPORT int fsl_db_prepare( fsl_db *db, fsl_stmt * tgt, char const * sql, ... );

/**
   va_list counterpart of fsl_db_prepare().
*/
FSL_EXPORT int fsl_db_preparev( fsl_db *db, fsl_stmt * tgt, char const * sql, va_list args );

/**
   A special-purpose variant of fsl_db_prepare() which caches
   statements based on their SQL code. This works very much like
   fsl_db_prepare() and friends except that it can return the same
   statement (via *st) multiple times (statements with identical
   SQL are considered equivalent for caching purposes). Clients
   need not explicitly pass the returned statement to
   fsl_stmt_finalize() - the db holds these statements and will
   finalize them when it is closed. It is legal to pass them to
   finalize, in which case they will be cleaned up immediately but
   that also invalidates _all_ pointers to the shared instances.

   If client code does not call fsl_stmt_finalize(), it MUST pass
   the statement pointer to fsl_stmt_cached_yield(st) after is done
   with it. That makes the query available for use again with this
   routine. If a cached query is not yielded via
   fsl_stmt_cached_yield() then this routine will return
   FSL_RC_ACCESS on subsequent requests for that SQL to prevent
   that recursive (mis)use of the statement causes problems.

   This routine is intended to be used in oft-called routines
   where the cost of re-creating statements on each execution could
   be prohibitive (or at least a bummer).

   Returns 0 on success, FSL_RC_MISUSE if any arguments are
   invalid. On error, *st is not written to.  On other error's
   db->error might be updated with more useful information.  See the
   Caveats section below for more details.

   Its intended usage looks like:

   @code
   fsl_stmt * st = NULL;
   int rc = fsl_db_prepare_cached(myDb, &st, "SELECT ...");
   if(rc) { assert(!st); ...error... }
   else {
   ...use it, and _be sure_ to yield it when done:...
   fsl_stmt_cached_yield(st);
   }
   @endcode

   Though this function allows a formatted SQL string, caching is
   generally only useful with statements which have "static" SQL,
   i.e. no call-dependent values embedded within the SQL. It _can_,
   however, contain bind() placeholders which get reset for each
   use. Note that fsl_stmt_cached_yield() resets the statement, so
   most uses of cached statements do not require that the client
   explicitly reset cached statements (doing so is harmless,
   however).

   Caveats:

   Cached queries must not be used in contexts where recursion
   might cause the same query to be returned from this function
   while it is being processed at another level in the execution
   stack. Results would be undefined. Caching is primarily intended
   for often-used routines which bind and fetch simple values, and
   not for queries which bind large inlined values or might invoke
   recursion. Because of the potential for recursive breakage, this
   function flags queries it doles out and requires that clients
   call fsl_stmt_cached_yield() to un-flag them for re-use. It will
   return FSL_RC_ACCESS if an attempt is made to (re)prepare a
   statement for which a fsl_stmt_cached_yield() is pending, and
   db->error will be populated with a (long) error string
   descripting the problem and listing the SQL which caused the
   collision/misuse.


   Design note: for the recursion/parallel use case we "could"
   reimplement this to dole out a new statement (e.g. by appending
   " -- a_number" to the SQL to bypass the collision) and free it in
   fsl_stmt_cached_yield(), but that (A) gets uglier than it needs
   to be and (B) is not needed unless/until we really need cached
   queries in spots which would normally break them. The whole
   recursion problem is still theoretical at this point but could
   easily affect small, often-used queries without recursion.

   @see fsl_db_stmt_cache_clear()
   @see fsl_stmt_cached_yield()
*/
FSL_EXPORT int fsl_db_prepare_cached( fsl_db * db, fsl_stmt ** st, char const * sql, ... );

/**
   The va_list counterpart of fsl_db_prepare_cached().
*/
FSL_EXPORT int fsl_db_preparev_cached( fsl_db * db, fsl_stmt ** st, char const * sql,
                            va_list args );

/**
   "Yields" a statement which was prepared with
   fsl_db_prepare_cached(), such that that routine can once again
   use/re-issue that statement. Statements prepared this way must
   be yielded in order to prevent that recursion causes
   difficult-to-track errors when a given cached statement is used
   concurrently in different code contexts.

   If st is not NULL then this also calls fsl_stmt_reset() on the
   statement (because that simplifies usage of cached statements).

   Returns 0 on success, FSL_RC_MISUSE if !st or if st does not
   appear to have been doled out from fsl_db_prepare_cached().

   @see fsl_db_prepare_cached()
   @see fsl_db_stmt_cache_clear()
*/
FSL_EXPORT int fsl_stmt_cached_yield( fsl_stmt * st );

/**
   Immediately cleans up all cached statements.  Returns the number
   of statements cleaned up. It is illegal to call this while any
   of the cached statements are actively being used (have not been
   fsl_stmt_cached_yield()ed), and doing so will lead to undefined
   results if the statement(s) in question are used after this
   function completes.

   @see fsl_db_prepare_cached()
   @see fsl_stmt_cached_yield()
*/
FSL_EXPORT fsl_size_t fsl_db_stmt_cache_clear(fsl_db * db);

/**
   A special-purposes utility which schedules SQL to be executed
   the next time fsl_db_transaction_end() commits a transaction for
   the given db. A commit or rollback will clear all before-commit
   SQL whether it executes them or not. This should not be used as
   a general-purpose trick, and is intended only for use in very
   limited parts of the Fossil infrastructure.

   Before-commit code is only executed if the db has made changes
   since the transaction began. If no changes are recorded
   then before-commit triggers are _not_ run. This is a historical
   behaviour which is up for debate.

   This function does not prepare the SQL, so it does not catch
   errors which happen at prepare-time. Preparation is done (if
   ever) just before the next transaction is committed.

   Returns 0 on success, non-0 on error.

   Potential TODO: instead of storing the raw SQL, prepare the
   statements here and store the statement handles. The main
   benefit would be that this routine could report preport
   preparation errors (which otherwise cause the the commit to
   fail). The down-side is that it prohibits the use of
   multi-statement pre-commit code. We have an implementation of
   this somewhere early on in the libfossil tree, but it was not
   integrated because of the inability to use multi-statement SQL
   with it.
*/
FSL_EXPORT int fsl_db_before_commit( fsl_db *db, char const * sql, ... );

/**
   va_list counterpart to fsl_db_before_commit().
*/
FSL_EXPORT int fsl_db_before_commitv( fsl_db *db, char const * sql, va_list args );


/**
   Frees memory associated with stmt but does not free stmt unless
   it was allocated by fsl_stmt_malloc() (these objects are
   normally stack-allocated, and such object must be initialized by
   copying fsl_stmt_empty so that this function knows whether or
   not to fsl_free() them). Returns FSL_RC_MISUSE if !stmt or it
   has already been finalized (but was not freed).
*/
FSL_EXPORT int fsl_stmt_finalize( fsl_stmt * stmt );

/**
   "Steps" the given SQL cursor one time and returns one of the
   following: FSL_RC_STEP_ROW, FSL_RC_STEP_DONE, FSL_RC_STEP_ERROR.
   On a db error this will update the underlying db's error state.
   This function increments stmt->rowCount by 1 if it returns
   FSL_RC_STEP_ROW.

   Returns FSL_RC_MISUSE if !stmt or stmt has not been prepared.

   It is only legal to call the fsl_stmt_g_xxx() and
   fsl_stmt_get_xxx() functions if this functon returns
   FSL_RC_STEP_ROW. FSL_RC_STEP_DONE is returned upon successfully
   ending iteration or if there is no iteration to perform (e.g. a
   UPDATE or INSERT).


   @see fsl_stmt_reset()
   @see fsl_stmt_reset2()
   @see fsl_stmt_each()
*/
FSL_EXPORT int fsl_stmt_step( fsl_stmt * stmt );

/**
   A callback interface for use with fsl_stmt_each() and
   fsl_db_each(). It will be called one time for each row fetched,
   passed the statement object and the state parameter passed to
   fsl_stmt_each() resp. fsl_db_each().  If it returns non-0 then
   iteration stops and that code is returned UNLESS it returns
   FSL_RC_BREAK, in which case fsl_stmt_each() stops iteration and
   returns 0. i.e. implementations may return FSL_RC_BREAK to
   prematurly end iteration without causing an error.

   This callback is not called for non-fetching queries or queries
   which return no results, though it might (or might not) be
   interesting for it to do so, passing a NULL stmt for that case.

   stmt->rowCount can be used to determine how many times the
   statement has called this function. Its counting starts at 1.

   It is strictly illegal for a callback to pass stmt to
   fsl_stmt_step(), fsl_stmt_reset(), fsl_stmt_finalize(), or any
   similar routine which modifies its state. It must only read the
   current column data (or similar metatdata, e.g. column names)
   from the statement, e.g. using fsl_stmt_g_int32(),
   fsl_stmt_get_text(), or similar.
*/
typedef int (*fsl_stmt_each_f)( fsl_stmt * stmt, void * state );

/**
   Calls the given callback one time for each result row in the
   given statement, iterating over stmt using fsl_stmt_step(). It
   applies no meaning to the callbackState parameter, which gets
   passed as-is to the callback. See fsl_stmt_each_f() for the
   semantics of the callback.

   Returns 0 on success. Returns FSL_RC_MISUSE if !stmt or
   !callback.
*/
FSL_EXPORT int fsl_stmt_each( fsl_stmt * stmt, fsl_stmt_each_f callback,
                              void * callbackState );

/**
   Resets the given statement, analog to sqlite3_reset(). Should be
   called one time between fsl_stmt_step() iterations when running
   multiple INSERTS, UPDATES, etc. via the same statement. If
   resetRowCounter is true then the statement's row counter
   (st->rowCount) is also reset to 0, else it is left
   unmodified. (Most use cases don't use the row counter.)

   Returns 0 on success, FSL_RC_MISUSE if !stmt or stmt has not
   been prepared, FSL_RC_DB if the underlying reset fails (in which
   case the error state of the stmt->db handle is updated to
   contain the error information).

   @see fsl_stmt_db()
   @see fsl_stmt_reset()
*/
FSL_EXPORT int fsl_stmt_reset2( fsl_stmt * stmt, bool resetRowCounter );

/**
   Equivalent to fsl_stmt_reset2(stmt, 0).
*/
FSL_EXPORT int fsl_stmt_reset( fsl_stmt * stmt );

/**
   Returns the db handle which prepared the given statement, or
   NULL if !stmt or stmt has not been prepared.
*/
FSL_EXPORT fsl_db * fsl_stmt_db( fsl_stmt * stmt );

/**
   Returns the SQL string used to prepare the given statement, or
   NULL if !stmt or stmt has not been prepared. If len is not NULL
   then *len is set to the length of the returned string (which is
   NUL-terminated). The returned bytes are owned by stmt and are
   invalidated when it is finalized.
*/
FSL_EXPORT char const * fsl_stmt_sql( fsl_stmt * stmt, fsl_size_t * len );

/**
   Returns the name of the given 0-based result column index, or
   NULL if !stmt, stmt is not prepared, or index is out out of
   range. The returned bytes are owned by the statement object and
   may be invalidated shortly after this is called, so the caller
   must copy the returned value if it needs to have any useful
   lifetime guarantees. It's a bit more complicated than this, but
   assume that any API calls involving the statement handle might
   invalidate the column name bytes.

   The API guarantees that the returned value is either NULL or
   NUL-terminated.

   @see fsl_stmt_param_count()
   @see fsl_stmt_col_count()
*/
FSL_EXPORT char const * fsl_stmt_col_name(fsl_stmt * stmt, int index);

/**
   Returns the result column count for the given statement, or -1 if
   !stmt or it has not been prepared. Note that this value is cached
   when the statement is created. Note that non-fetching queries
   (e.g. INSERT and UPDATE) have a column count of 0. Some non-SELECT
   constructs, e.g. PRAGMA table_info(tname), behave like SELECT
   and have a positive column count.

   @see fsl_stmt_param_count()
   @see fsl_stmt_col_name()
*/
FSL_EXPORT int fsl_stmt_col_count( fsl_stmt const * stmt );

/**
   Returns the bound parameter count for the given statement, or -1
   if !stmt or it has not been prepared. Note that this value is
   cached when the statement is created.

   @see fsl_stmt_col_count()
   @see fsl_stmt_col_name()
*/
FSL_EXPORT int fsl_stmt_param_count( fsl_stmt const * stmt );

/**
   Returns the index of the given named parameter for the given
   statement, or -1 if !stmt or stmt is not prepared.
*/
FSL_EXPORT int fsl_stmt_param_index( fsl_stmt * stmt, char const * param);

/**
   Binds NULL to the given 1-based parameter index.  Returns 0 on
   succcess. Sets the DB's error state on error.
*/
FSL_EXPORT int fsl_stmt_bind_null( fsl_stmt * stmt, int index );

/**
   Equivalent to fsl_stmt_bind_null_name() but binds to
   a named parameter.
*/
FSL_EXPORT int fsl_stmt_bind_null_name( fsl_stmt * stmt, char const * param );

/**
   Binds v to the given 1-based parameter index.  Returns 0 on
   succcess. Sets the DB's error state on error.
*/
FSL_EXPORT int fsl_stmt_bind_int32( fsl_stmt * stmt, int index, int32_t v );

/**
   Equivalent to fsl_stmt_bind_int32() but binds to a named
   parameter.
*/
FSL_EXPORT int fsl_stmt_bind_int32_name( fsl_stmt * stmt, char const * param, int32_t v );

/**
   Binds v to the given 1-based parameter index.  Returns 0 on
   succcess. Sets the DB's error state on error.
*/
FSL_EXPORT int fsl_stmt_bind_int64( fsl_stmt * stmt, int index, int64_t v );

/**
   Equivalent to fsl_stmt_bind_int64() but binds to a named
   parameter.
*/
FSL_EXPORT int fsl_stmt_bind_int64_name( fsl_stmt * stmt, char const * param, int64_t v );

/**
   Binds v to the given 1-based parameter index.  Returns 0 on
   succcess. Sets the Fossil context's error state on error.
*/
FSL_EXPORT int fsl_stmt_bind_double( fsl_stmt * stmt, int index, double v );

/**
   Equivalent to fsl_stmt_bind_double() but binds to a named
   parameter.
*/
FSL_EXPORT int fsl_stmt_bind_double_name( fsl_stmt * stmt, char const * param, double v );

/**
   Binds v to the given 1-based parameter index.  Returns 0 on
   succcess. Sets the DB's error state on error.
*/
FSL_EXPORT int fsl_stmt_bind_id( fsl_stmt * stmt, int index, fsl_id_t v );

/**
   Equivalent to fsl_stmt_bind_id() but binds to a named
   parameter.
*/
FSL_EXPORT int fsl_stmt_bind_id_name( fsl_stmt * stmt, char const * param, fsl_id_t v );

/**
   Binds the first n bytes of v as text to the given 1-based bound
   parameter column in the given statement. If makeCopy is true then
   the binding makes an copy of the data. Set makeCopy to false ONLY
   if you KNOW that the bytes will outlive the binding.

   Returns 0 on success. On error stmt's underlying db's error state
   is updated, hopefully with a useful error message.
*/
FSL_EXPORT int fsl_stmt_bind_text( fsl_stmt * stmt, int index,
                                   char const * v, fsl_int_t n,
                                   bool makeCopy );

/**
   Equivalent to fsl_stmt_bind_text() but binds to a named
   parameter.
*/
FSL_EXPORT int fsl_stmt_bind_text_name( fsl_stmt * stmt, char const * param,
                                        char const * v, fsl_int_t n,
                                        bool makeCopy );
/**
   Binds the first n bytes of v as a blob to the given 1-based bound
   parameter column in the given statement. See fsl_stmt_bind_text()
   for the semantics of the makeCopy parameter and return value.
*/
FSL_EXPORT int fsl_stmt_bind_blob( fsl_stmt * stmt, int index,
                                   void const * v, fsl_size_t len,
                                   bool makeCopy );

/**
   Equivalent to fsl_stmt_bind_blob() but binds to a named
   parameter.
*/
FSL_EXPORT int fsl_stmt_bind_blob_name( fsl_stmt * stmt, char const * param,
                                        void const * v, fsl_int_t len,
                                        bool makeCopy );

/**
   Gets an integer value from the given 0-based result set column,
   assigns *v to that value, and returns 0 on success.

   Returns FSL_RC_RANGE if index is out of range for stmt.
*/
FSL_EXPORT int fsl_stmt_get_int32( fsl_stmt * stmt, int index, int32_t * v );

/**
   Gets an integer value from the given 0-based result set column,
   assigns *v to that value, and returns 0 on success.

   Returns FSL_RC_RANGE if index is out of range for stmt.
*/
FSL_EXPORT int fsl_stmt_get_int64( fsl_stmt * stmt, int index, int64_t * v );

/**
   The fsl_id_t counterpart of fsl_stmt_get_int32(). Depending on
   the sizeof(fsl_id_t), it behaves as one of fsl_stmt_get_int32()
   or fsl_stmt_get_int64().
*/
FSL_EXPORT int fsl_stmt_get_id( fsl_stmt * stmt, int index, fsl_id_t * v );

/**
   Convenience form of fsl_stmt_get_id() which returns the value
   directly but cannot report errors. It returns -1 on error, but
   that is not unambiguously an error value.
*/
FSL_EXPORT fsl_id_t fsl_stmt_g_id( fsl_stmt * stmt, int index );

/**
   Convenience form of fsl_stmt_get_int32() which returns the value
   directly but cannot report errors. It returns 0 on error, but
   that is not unambiguously an error.
*/
FSL_EXPORT int32_t fsl_stmt_g_int32( fsl_stmt * stmt, int index );

/**
   Convenience form of fsl_stmt_get_int64() which returns the value
   directly but cannot report errors. It returns 0 on error, but
   that is not unambiguously an error.
*/
FSL_EXPORT int64_t fsl_stmt_g_int64( fsl_stmt * stmt, int index );

/**
   Convenience form of fsl_stmt_get_double() which returns the value
   directly but cannot report errors. It returns 0 on error, but
   that is not unambiguously an error.
*/
FSL_EXPORT double fsl_stmt_g_double( fsl_stmt * stmt, int index );

/**
   Convenience form of fsl_stmt_get_text() which returns the value
   directly but cannot report errors. It returns NULL on error, but
   that is not unambiguously an error because it also returns NULL
   if the column contains an SQL NULL value. If outLen is not NULL
   then it is set to the byte length of the returned string.
*/
FSL_EXPORT char const * fsl_stmt_g_text( fsl_stmt * stmt, int index, fsl_size_t * outLen );

/**
   Gets double value from the given 0-based result set column,
   assigns *v to that value, and returns 0 on success.

   Returns FSL_RC_RANGE if index is out of range for stmt.
*/
FSL_EXPORT int fsl_stmt_get_double( fsl_stmt * stmt, int index, double * v );

/**
   Gets a string value from the given 0-based result set column,
   assigns *out (if out is not NULL) to that value, assigns *outLen
   (if outLen is not NULL) to *out's length in bytes, and returns 0
   on success. Ownership of the string memory is unchanged - it is owned
   by the statement and the caller should immediately copy it if
   it will be needed for much longer.

   Returns FSL_RC_RANGE if index is out of range for stmt.
*/
FSL_EXPORT int fsl_stmt_get_text( fsl_stmt * stmt, int index, char const **out,
                       fsl_size_t * outLen );

/**
   The Blob counterpart of fsl_stmt_get_text(). Identical to that
   function except that its output result (3rd paramter) type
   differs, and it fetches the data as a raw blob, without any sort
   of string interpretation.

   Returns FSL_RC_RANGE if index is out of range for stmt.
*/
FSL_EXPORT int fsl_stmt_get_blob( fsl_stmt * stmt, int index, void const **out, fsl_size_t * outLen );

/**
   Executes multiple SQL statements, ignoring any results they might
   collect. Returns 0 on success, non-0 on error.  On error
   db->error might be updated to report the problem.
*/
FSL_EXPORT int fsl_db_exec_multi( fsl_db * db, const char * sql, ...);

/**
   va_list counterpart of db_exec_multi().
*/
FSL_EXPORT int fsl_db_exec_multiv( fsl_db * db, const char * sql, va_list args);

/**
   Executes a single SQL statement, skipping over any results
   it may have. Returns 0 on success. On error db's error state
   may be updated.
*/
FSL_EXPORT int fsl_db_exec( fsl_db * db, char const * sql, ... );

/**
   va_list counterpart of fs_db_exec().
*/
FSL_EXPORT int fsl_db_execv( fsl_db * db, char const * sql, va_list args );

/**
   Begins a transaction on the given db. Nested transactions are
   not directly supported but the db handle keeps track of
   open/close counts, such that fsl_db_transaction_end() will not
   actually do anything until the transaction begin/end counter
   goes to 0. Returns FSL_RC_MISUSE if !db or the db is not
   connected, else the result of the underlying db call(s).

   Transactions are an easy way to implement "dry-run" mode for
   some types of applications. For example:

   @code
   char dryRunMode = ...;
   fsl_db_transaction_begin(db);
   ...do your stuff...
   fsl_db_transaction_end(db, dryRunMode ? 1 : 0);
   @endcode

   Here's a tip for propagating error codes when using
   transactions:

   @code
   ...
   if(rc) fsl_db_transaction_end(db, 1);
   else rc = fsl_db_transaction_end(db, 0);
   @endcode

   That ensures that we propagate rc in the face of a rollback but
   we also capture the rc for a commit (which might yet fail). Note
   that a rollback in and of itself is not an error (though it also
   might fail, that would be "highly unusual" and indicative of
   other problems), and we certainly don't want to overwrite that
   precious non-0 rc with a successful return result from a
   rollback (which would, in effect, hide the error from the
   client).
*/
FSL_EXPORT int fsl_db_transaction_begin(fsl_db * db);

/**
   Equivalent to fsl_db_transaction_end(db, 0).
*/
FSL_EXPORT int fsl_db_transaction_commit(fsl_db * db);

/**
   Equivalent to fsl_db_transaction_end(db, 1).
*/
FSL_EXPORT int fsl_db_transaction_rollback(fsl_db * db);

/**
   Forces a rollback of any pending transaction in db, regardless
   of the internal transaction begin/end counter. Returns
   FSL_RC_MISUSE if !db or db is not opened, else returns the value
   of the underlying ROLLBACK call. This also re-sets/frees any
   transaction-related state held by db (e.g. db->beforeCommit).
   Use with care, as this mucks about with db state in a way which
   is not all that pretty and it may confuse downstream code.

   Returns 0 on success.
*/
FSL_EXPORT int fsl_db_rollback_force(fsl_db * db);

/**
   Decrements the transaction counter incremented by
   fsl_db_transaction_begin() and commits or rolls back the
   transaction if the counter goes to 0.

   If doRollback is true then this rolls back (or schedules a
   rollback of) a transaction started by
   fsl_db_transaction_begin(). If doRollback is false is commits
   (or schedules a commit).

   If db fsl_db_transaction_begin() is used in a nested manner and
   doRollback is true for any one of the nested calls, then that
   value will be remembered, such that the downstream calls to this
   function within the same transaction will behave like a rollback
   even if they pass 0 for the second argument.

   Returns FSL_RC_MISUSE if !db or the db is not opened, 0 if
   the transaction counter is above 0, else the result of the
   (potentially many) underlying database operations.

   Unfortunate low-level co-dependency: if db->f is not NULL and
   (db->role & FSL_DBROLE_REPO) then this function may perform
   extra repository-related post-processing on any commit, and
   checking the result code is particularly important for those
   cases.
*/
FSL_EXPORT int fsl_db_transaction_end(fsl_db * db, bool doRollback);

/**
   Returns the given db's current transaction depth. If the value is
   negative, its absolute value represents the depth but indicates
   that a rollback is pending. If it is positive, the transaction is
   still in a "good" state. If it is 0, no transaction is active.
*/
FSL_EXPORT int fsl_db_transaction_level(fsl_db * db);

/**
   Runs the given SQL query on the given db and returns true if the
   query returns any rows, else false. Returns 0 for any error as
   well.
*/
FSL_EXPORT bool fsl_db_exists(fsl_db * db, char const * sql, ... );

/**
   va_list counterpart of fsl_db_exists().
*/
FSL_EXPORT bool fsl_db_existsv(fsl_db * db, char const * sql, va_list args );

/**
   Runs a fetch-style SQL query against DB and returns the first
   column of the first result row via *rv. If the query returns no
   rows, *rv is not modified. The intention is that the caller sets
   *rv to his preferred default (or sentinel) value before calling
   this.

   The format string (the sql parameter) accepts all formatting
   options supported by fsl_appendf().

   Returns 0 on success. On error db's error state is updated and
   *rv is not modified.

   Returns FSL_RC_MISUSE without side effects if !db, !rv, !sql,
   or !*sql.
*/
FSL_EXPORT int fsl_db_get_int32( fsl_db * const db, int32_t * rv,
                                 char const * sql, ... );

/**
   va_list counterpart of fsl_db_get_int32().
*/
FSL_EXPORT int fsl_db_get_int32v( fsl_db * const db, int32_t * rv,
                                  char const * sql, va_list args);

/**
   Convenience form of fsl_db_get_int32() which returns the value
   directly but provides no way of checking for errors. On error,
   or if no result is found, defaultValue is returned.
*/
FSL_EXPORT int32_t fsl_db_g_int32( fsl_db * const db,
                                   int32_t defaultValue,
                                   char const * sql, ... );

/**
   The int64 counterpart of fsl_db_get_int32(). See that function
   for the semantics.
*/
FSL_EXPORT int fsl_db_get_int64( fsl_db * db, int64_t * rv,
                      char const * sql, ... );

/**
   va_list counterpart of fsl_db_get_int64().
*/
FSL_EXPORT int fsl_db_get_int64v( fsl_db * db, int64_t * rv,
                       char const * sql, va_list args);

/**
   Convenience form of fsl_db_get_int64() which returns the value
   directly but provides no way of checking for errors. On error,
   or if no result is found, defaultValue is returned.
*/
FSL_EXPORT int64_t fsl_db_g_int64( fsl_db * db, int64_t defaultValue,
                            char const * sql, ... );


/**
   The fsl_id_t counterpart of fsl_db_get_int32(). See that function
   for the semantics.
*/
FSL_EXPORT int fsl_db_get_id( fsl_db * db, fsl_id_t * rv,
                   char const * sql, ... );

/**
   va_list counterpart of fsl_db_get_id().
*/
FSL_EXPORT int fsl_db_get_idv( fsl_db * db, fsl_id_t * rv,
                    char const * sql, va_list args);

/**
   Convenience form of fsl_db_get_id() which returns the value
   directly but provides no way of checking for errors. On error,
   or if no result is found, defaultValue is returned.
*/
FSL_EXPORT fsl_id_t fsl_db_g_id( fsl_db * db, fsl_id_t defaultValue,
                      char const * sql, ... );


/**
   The fsl_size_t counterpart of fsl_db_get_int32(). See that
   function for the semantics. If this function would fetch a
   negative value, it returns FSL_RC_RANGE and *rv is not modified.
*/
FSL_EXPORT int fsl_db_get_size( fsl_db * db, fsl_size_t * rv,
                     char const * sql, ... );

/**
   va_list counterpart of fsl_db_get_size().
*/
FSL_EXPORT int fsl_db_get_sizev( fsl_db * db, fsl_size_t * rv,
                      char const * sql, va_list args);

/**
   Convenience form of fsl_db_get_size() which returns the value
   directly but provides no way of checking for errors. On error,
   or if no result is found, defaultValue is returned.
*/
FSL_EXPORT fsl_size_t fsl_db_g_size( fsl_db * db, fsl_size_t defaultValue,
                          char const * sql, ... );


/**
   The double counterpart of fsl_db_get_int32(). See that function
   for the semantics.
*/
FSL_EXPORT int fsl_db_get_double( fsl_db * db, double * rv,
                                  char const * sql, ... );

/**
   va_list counterpart of fsl_db_get_double().
*/
FSL_EXPORT int fsl_db_get_doublev( fsl_db * db, double * rv,
                                   char const * sql, va_list args);

/**
   Convenience form of fsl_db_get_double() which returns the value
   directly but provides no way of checking for errors. On error,
   or if no result is found, defaultValue is returned.
*/
FSL_EXPORT double fsl_db_g_double( fsl_db * db, double defaultValue,
                                   char const * sql, ... );

/**
   The C-string counterpart of fsl_db_get_int32(). On success *rv
   will be set to a dynamically allocated string copied from the
   first column of the first result row. If rvLen is not NULL then
   *rvLen will be assigned the byte-length of that string. If no
   row is found, *rv is set to NULL and *rvLen (if not NULL) is set
   to 0, and 0 is returned. Note that NULL is also a legal result
   (an SQL NULL translates as a NULL string), The caller must
   eventually free the returned string value using fsl_free().
*/
FSL_EXPORT int fsl_db_get_text( fsl_db * db, char ** rv, fsl_size_t * rvLen,
                                char const * sql, ... );

/**
   va_list counterpart of fsl_db_get_text().
*/
FSL_EXPORT int fsl_db_get_textv( fsl_db * db, char ** rv, fsl_size_t * rvLen,
                      char const * sql, va_list args );

/**
   Convenience form of fsl_db_get_text() which returns the value
   directly but provides no way of checking for errors. On error,
   or if no result is found, NULL is returned. The returned string
   must eventually be passed to fsl_free() to free it.  If len is
   not NULL then if non-NULL is returned, *len will be assigned the
   byte-length of the returned string.
*/
FSL_EXPORT char * fsl_db_g_text( fsl_db * db, fsl_size_t * len,
                      char const * sql,
                      ... );

/**
   The Blob counterpart of fsl_db_get_text(). Identical to that
   function except that its output result (2nd paramter) type
   differs, and it fetches the data as a raw blob, without any sort
   of string interpretation. The returned *rv memory must
   eventually be passed to fsl_free() to free it. If len is not
   NULL then on success *len will be set to the byte length of the
   returned blob. If no row is found, *rv is set to NULL and *rvLen
   (if not NULL) is set to 0, and 0 is returned. Note that NULL is
   also a legal result (an SQL NULL translates as a NULL string),
*/
FSL_EXPORT int fsl_db_get_blob( fsl_db * db, void ** rv, fsl_size_t * len,
                     char const * sql, ... );


/**
   va_list counterpart of fsl_db_get_blob().
*/
FSL_EXPORT int fsl_db_get_blobv( fsl_db * db, void ** rv, fsl_size_t * stmtLen,
                      char const * sql, va_list args );

/**
   Convenience form of fsl_db_get_blob() which returns the value
   directly but provides no way of checking for errors. On error,
   or if no result is found, NULL is returned.
*/
FSL_EXPORT void * fsl_db_g_blob( fsl_db * db, fsl_size_t * len,
                      char const * sql,
                      ... );
/**
   Similar to fsl_db_get_text() and fsl_db_get_blob(), but writes
   its result to tgt, overwriting (not appennding to) any existing
   memory it might hold.

   If asBlob is true then the underlying BLOB API is used to
   populate the buffer, else the underlying STRING/TEXT API is
   used.  For many purposes there will be no difference, but if you
   know you might have binary data, be sure to pass a true value
   for asBlob to avoid any potential encoding-related problems.
*/
FSL_EXPORT int fsl_db_get_buffer( fsl_db * db, fsl_buffer * tgt,
                       char asBlob,
                       char const * sql, ... );

/**
   va_list counterpart of fsl_db_get_buffer().
*/
FSL_EXPORT int fsl_db_get_bufferv( fsl_db * db, fsl_buffer * tgt,
                        char asBlob,
                        char const * sql, va_list args );


/**
   Expects sql to be a SELECT-style query which (potentially)
   returns a result set. For each row in the set callback() is
   called, as described for fsl_stmt_each(). Returns 0 on success.
   The callback is _not_ called for queries which return no
   rows. If clients need to know if rows were returned, they can
   add a counter to their callbackState and increment it from the
   callback.

   Returns FSL_RC_MISUSE if !db, db is not opened, !callback,
   !sql. Returns FSL_RC_RANGE if !*sql.
*/
FSL_EXPORT int fsl_db_each( fsl_db * db, fsl_stmt_each_f callback,
                            void * callbackState, char const * sql, ... );

/**
   va_list counterpart to fsl_db_each().
*/
FSL_EXPORT int fsl_db_eachv( fsl_db * db, fsl_stmt_each_f callback,
                             void * callbackState, char const * sql, va_list args );


/**
   Returns the given Julian date value formatted as an ISO8601
   string (with a fractional seconds part if msPrecision is true,
   else without it).  Returns NULL if !db, db is not connected, j
   is less than 0, or on allocation error. The returned memory must
   eventually be freed using fsl_free().

   If localTime is true then the value is converted to the local time,
   otherwise it is not.

   @see fsl_db_unix_to_iso8601()
   @see fsl_julian_to_iso8601()
   @see fsl_iso8601_to_julian()
*/
FSL_EXPORT char * fsl_db_julian_to_iso8601( fsl_db * db, double j,
                                            char msPrecision, char localTime );

/**
   Returns the given Julian date value formatted as an ISO8601
   string (with a fractional seconds part if msPrecision is true,
   else without it).  Returns NULL if !db, db is not connected, j
   is less than 0, or on allocation error. The returned memory must
   eventually be freed using fsl_free().

   If localTime is true then the value is converted to the local time,
   otherwise it is not.

   @see fsl_db_julian_to_iso8601()
   @see fsl_julian_to_iso8601()
   @see fsl_iso8601_to_julian()
*/
FSL_EXPORT char * fsl_db_unix_to_iso8601( fsl_db * db, fsl_time_t j, char localTime );


/**
   Returns the current time in Julian Date format. Returns a negative
   value if !db or db is not opened.
*/
FSL_EXPORT double fsl_db_julian_now(fsl_db * db);

/**
   Uses the given db to convert the given time string to Julian Day
   format. If it cannot be converted, a negative value is returned.
   The str parameter can be anything suitable for passing to sqlite's:

   SELECT julianday(str)

   Note that this routine will escape str for use with SQL - the
   caller must not do so.

   @see fsl_julian_to_iso8601()
   @see fsl_iso8601_to_julian()
*/
FSL_EXPORT double fsl_db_string_to_julian(fsl_db * db, char const * str);

/**
   Opens the given db file and populates db with its handle.  db
   must have been cleanly initialized by copy-initializing it from
   fsl_db_empty (or fsl_db_empty_m) or by allocating it using
   fsl_db_malloc(). Failure to do so will lead to undefined
   behaviour.

   openFlags may be a mask of FSL_OPEN_F_xxx values, but not all
   are used/supported here. If FSL_OPEN_F_CREATE is _not_ set in
   openFlags and dbFile does not exist, it will return
   FSL_RC_NOT_FOUND. The existence of FSL_OPEN_F_CREATE in the
   flags will cause this routine to try to create the file if
   needed. If conflicting flags are specified (e.g. FSL_OPEN_F_RO
   and FSL_OPEN_F_RWC) then which one takes precedence is
   unspecified and possibly unpredictable.

   As a special case, if dbFile is ":memory:" (for an in-memory
   database) or "" (empty string, for a "temporary" database) then it
   is is passed through without any filesystem-related checks and the
   openFlags are ignored.

   See this page for the differences between ":memory:" and "":

   https://www.sqlite.org/inmemorydb.html

   Returns FSL_RC_MISUSE if !db, !dbFile, !*dbFile, or if db->dbh
   is not NULL (i.e. if it is already opened or its memory was
   default-initialized (use fsl_db_empty to cleanly copy-initialize
   new stack-allocated instances).

   On error db->dbh will be NULL, but db->error might contain error
   details.

   Regardless of success or failure, db should be passed to
   fsl_db_close() to free up all memory associated with it. It is
   not closed automatically by this function because doing so cleans
   up the error state, which the caller will presumably want to
   have.

   If db->f is not NULL when this is called then it is assumed that
   db should be plugged in to the Fossil repository system, and the
   following additional things happen:

   - A number of SQL functions are registered with the db. Details
   are below.

   - If FSL_OPEN_F_SCHEMA_VALIDATE is set in openFlags then the
   db is validated to see if it has a fossil schema.  If that
   validation fails, FSL_RC_REPO_NEEDS_REBUILD or FSL_RC_NOT_A_REPO
   will be returned and db's error state will be updated. db->f
   does not need to be set for that check to work.


   The following SQL functions get registered with the db if db->f
   is not NULL when this function is called:

   - NOW() returns the current time as an integer, as per time(2).

   - FSL_USER() returns the current value of fsl_cx_user_get(),
   or NULL if that is not set.

   - FSL_CONTENT(INTEGER|STRING) returns the undeltified,
   uncompressed content for the blob record with the given ID (if
   the argument is an integer) or symbolic name (as per
   fsl_sym_to_rid()), as per fsl_content_get(). If the argument
   does not resolve to an in-repo blob, a db-level error is
   triggered. If passed an integer, no validation is done on its
   validity, but such checking can be enforced by instead passing
   the the ID as a string in the form "rid:ID". Both cases will
   result in an error if the RID is not found, but the error
   reporting is arguably slightly better for the "rid:ID" case.

   - FSL_SYM2RID(STRING) returns a blob RID for the given symbol,
   as per fsl_sym_to_rid(). Triggers an SQL error if fsl_sym_to_rid()
   fails.

   - FSL_DIRPART(STRING[, BOOL=0]) behaves like fsl_file_dirpart(),
   returning the result as a string unless it is empty, in which case
   the result is an SQL NULL.

   Note that functions described as "triggering a db error" will
   propagate that error, such that fsl_db_err_get() can report it
   to the client.


   @see fsl_db_close()
   @see fsl_db_prepare()
   @see fsl_db_malloc()
*/
FSL_EXPORT int fsl_db_open( fsl_db * db, char const * dbFile, int openFlags );

/**
   Closes the given db handle and frees any resources owned by
   db. Returns 0 on success.

   If db was allocated using fsl_db_malloc() (as determined by
   examining db->allocStamp) then this routine also fsl_free()s it,
   otherwise it is assumed to either be on the stack or part of a
   larger struct and is not freed.

   If db has any pending transactions, they are rolled
   back by this function.
*/
FSL_EXPORT int fsl_db_close( fsl_db * db );

/**
   If db is an opened db handle, this registers a debugging
   function with the db which traces all SQL to the given FILE
   handle (defaults to stdout if outStream is NULL).

   This mechanism is only intended for debugging and exploration of
   how Fossil works. Tracing is often as easy way to ensure that a
   given code block is getting run.

   As a special case, if db->f is not NULL _before_ it is is
   fsl_db_open()ed, then this function automatically gets installed
   if the SQL tracing option is enabled for that fsl_cx instance
   before the db is opened.

   This is a no-op if !db or db is not opened.
*/
FSL_EXPORT void fsl_db_sqltrace_enable( fsl_db * db, FILE * outStream );

/**
   Returns the row ID of the most recent insertion,
   or -1 if !db, db is not connected, or 0 if no inserts
   have been performed.
*/
FSL_EXPORT fsl_id_t fsl_db_last_insert_id(fsl_db *db);

/**
   Returns non-0 (true) if the database (which must be open) table
   identified by zTableName has a column named zColName
   (case-sensitive), else returns 0.
*/
FSL_EXPORT char fsl_db_table_has_column( fsl_db * db, char const *zTableName,
                              char const *zColName );

/**
   If a db name has been associated with db then it is returned,
   otherwise NULL is returned. A db has no name by default, but
   fsl_cx-used ones get their database name assigned to them
   (e.g. "main" for the main db).
*/
FSL_EXPORT char const * fsl_db_name(fsl_db const * db);

  
/**
   Returns a db name string for the given fsl_db_role value. The
   string is static, guaranteed to live as long as the app.  It
   returns NULL (or asserts in debug builds) if passed
   FSL_DBROLE_NONE or some value out of range for the enum.
*/
FSL_EXPORT const char * fsl_db_role_label(enum fsl_dbrole_e r);


/**
   Allocates a new fsl_db instance(). Returns NULL on allocation
   error. Note that fsl_db instances can often be used from the
   stack - allocating them dynamically is an uncommon case necessary
   for script bindings.

   Achtung: the returned value's allocStamp member is used for
   determining if fsl_db_close() should free the value or not.  Thus
   if clients copy over this value without adjusting allocStamp back
   to its original value, the library will likely leak the instance.
   Been there, done that.
*/
FSL_EXPORT fsl_db * fsl_db_malloc();

/**
   The fsl_stmt counterpart of fsl_db_malloc(). See that function
   for when you might want to use this and a caveat involving the
   allocStamp member of the returned value. fsl_stmt_finalize() will
   free statements created with this function.
*/
FSL_EXPORT fsl_stmt * fsl_stmt_malloc();


/**
   ATTACHes the file zDbName to db using the databbase name
   zLabel. Returns 0 on success. Returns FSL_RC_MISUSE if any
   argument is NULL or any string argument starts with a NUL byte,
   else it returns the result of fsl_db_exec() which attaches the
   db. On db-level errors db's error state will be updated.
*/
FSL_EXPORT int fsl_db_attach(fsl_db * db, const char *zDbName, const char *zLabel);

/**
   The converse of fsl_db_detach(). Must be passed the same arguments
   which were passed as the 1st and 3rd arguments to fsl_db_attach().
   Returns 0 on success, FSL_RC_MISUSE if !db, !zLabel, or !*zLabel,
   else it returns the result of the underlying fsl_db_exec()
   call.
*/
FSL_EXPORT int fsl_db_detach(fsl_db * db, const char *zLabel);


/**
   Expects fmt to be a SELECT-style query. For each row in the
   query, the first column is fetched as a string and appended to
   the tgt list.

   Returns 0 on success, FSL_RC_MISUSE if !db, !tgt, or !fmt, any
   number of potential FSL_RC_OOM or db-related errors.

   Results rows with a NULL value (resulting from an SQL NULL) are
   added to the list as NULL entries.

   Each entry appended to the list is a (char *) which must
   be freed using fsl_free(). To easiest way to clean up
   the list and its contents is:

   @code
   fsl_list_visit_free(tgt,...);
   @endcode

   On error the list may be partially populated.

   Complete example:
   @code
   fsl_list li = fsl_list_empty;
   int rc = fsl_db_select_slist(db, &li,
   "SELECT uuid FROM blob WHERE rid<20");
   if(!rc){
   fsl_size_t i;
   for(i = 0;i < li.used; ++i){
   char const * uuid = (char const *)li.list[i];
   fsl_fprintf(stdout, "UUID: %s\n", uuid);
   }
   }
   fsl_list_visit_free(&li, 1);
   @endcode

   Of course fsl_list_visit() may be used to traverse the list as
   well, as long as the visitor expects (char [const]*) list
   elements.
*/
FSL_EXPORT int fsl_db_select_slist( fsl_db * db, fsl_list * tgt,
                         char const * fmt, ... );

/**
   The va_list counterpart of fsl_db_select_slist().
*/
FSL_EXPORT int fsl_db_select_slistv( fsl_db * db, fsl_list * tgt,
                          char const * fmt, va_list args );

/**
   Returns n bytes of random lower-case hexidecimal characters
   using the given db as its data source, plus a terminating NUL
   byte. The returned memory must eventually be freed using
   fsl_free(). Returns NULL if !db, !n, or on a db-level error.
*/
FSL_EXPORT char * fsl_db_random_hex(fsl_db * db, fsl_size_t n);

/**
   Returns the "number of database rows that were changed or
   inserted or deleted by the most recently completed SQL statement"
   (to quote the underlying APIs). Returns 0 if !db or if db is not
   opened.


   See: https://sqlite.org/c3ref/changes.html
*/
FSL_EXPORT int fsl_db_changes_recent(fsl_db * db);
  
/**
   Returns "the number of row changes caused by INSERT, UPDATE or
   DELETE statements since the database connection was opened" (to
   quote the underlying APIs). Returns 0 if !db or if db is not
   opened.

   See; https://sqlite.org/c3ref/total_changes.html
*/
FSL_EXPORT int fsl_db_changes_total(fsl_db * db);

/**
   Initializes the given database file. zFilename is the name of
   the db file. It is created if needed, but any directory
   components are not created. zSchema is the base schema to
   install.  The following arguments may be (char const *) SQL
   code, each of which gets run against the db after the main
   schema is called.  The variadic argument list MUST end with NULL
   (0), even if there are no non-NULL entries.

   Returns 0 on success.

   On error, if err is not NULL then it is populated with any error
   state from the underlying (temporary) db handle.
*/
FSL_EXPORT int fsl_db_init( fsl_error * err, char const * zFilename,
                 char const * zSchema, ... );

/**
   A fsl_stmt_each_f() impl, intended primarily for debugging, which
   simply outputs row data in tabular form via fsl_output(). The
   state argument is ignored. This only works if stmt was prepared
   by a fsl_db instance which has an associated fsl_cx instance. On
   the first row, the column names are output.
*/
FSL_EXPORT int fsl_stmt_each_f_dump( fsl_stmt * const stmt, void * state );

/**
   Returns true if the table name specified by the final argument
   exists in the fossil database specified by the 2nd argument on the
   db connection specified by the first argument, else returns false.

   Trivia: this is one of the few libfossil APIs which makes use of
   FSL_DBROLE_TEMP.

   Potential TODO: this is a bit of a wonky interface. Consider
   changing it to eliminate the role argument, which is only really
   needed if we have duplicate table names across attached dbs or if
   we internally mess up and write a table to the wrong db.
*/
FSL_EXPORT bool fsl_db_table_exists(fsl_db * db, fsl_dbrole_e whichDb,
                                    const char *zTable);

/**
   The elipsis counterpart of fsl_stmt_bind_fmtv().
*/
FSL_EXPORT int fsl_stmt_bind_fmt( fsl_stmt * st, char const * fmt, ... );

/**
    Binds a series of values using a formatting string.
   
    The string may contain the following characters, each of which
    refers to the next argument in the args list:
   
    '-': binds a NULL and expects a NULL placeholder
    in the argument list (for consistency's sake).
   
    'i': binds an int32
   
    'I': binds an int64
   
    'R': binds a fsl_id_t ('R' as in 'RID')
   
    'f': binds a double
   
    's': binds a (char const *) as text or NULL.
   
    'S': binds a (char const *) as a blob or NULL.
   
    'b': binds a (fsl_buffer const *) as text or NULL.
   
    'B': binds a (fsl_buffer const *) as a blob or NULL.
   
    ' ': spaces are allowed for readability and are ignored.

    Returns 0 on success, any number of other FSL_RC_xxx codes on
    error.

    ACHTUNG: the "sSbB" bindings assume, because of how this API is
    normally used, that the memory pointed to by the given argument
    will outlive the pending step of the given statement, so that
    memory is NOT copied by the binding. Thus results are undefined if
    such an argument's memory is invalidated before the statement is
    done with it.
*/
FSL_EXPORT int fsl_stmt_bind_fmtv( fsl_stmt * const st, char const * fmt,
                                   va_list args );

/**
   Works like fsl_stmt_bind_fmt() but:

   1) It calls fsl_stmt_reset() before binding the arguments.

   2) If binding succeeds then it steps the given statement a single
   time.

   3) If the result is NOT FSL_RC_STEP_ROW then it also resets the
   statement before returning. It does not do so for FSL_RC_STEP_ROW
   because doing so would remove the fetched columns (and this is why
   it resets in step (1)).

   Returns 0 if stepping results in FSL_RC_STEP_DONE, FSL_RC_STEP_ROW
   if it produces a result row, or any number of other potential non-0
   codes on error. On error, the error state of st->db is updated.

   Design note: the return value for FSL_RC_STEP_ROW, as opposed to
   returning 0, is necessary for proper statement use if the client
   wants to fetch any result data from the statement afterwards (which
   is illegal if FSL_RC_STEP_ROW was not the result). This is also why
   it cannot reset the statement if that result is returned.
*/
FSL_EXPORT int fsl_stmt_bind_stepv( fsl_stmt * const st, char const * fmt,
                                    va_list args );

/**
   The elipsis counterpart of fsl_stmt_bind_stepv().
*/
FSL_EXPORT int fsl_stmt_bind_step( fsl_stmt * st, char const * fmt, ... );

#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_DB_H_INCLUDED */
/* end of file ../include/fossil-scm/fossil-db.h */
/* start of file ../include/fossil-scm/fossil-hash.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_HASH_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_HASH_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

  *****************************************************************************
  This file declares public APIs relating to hashing.
*/

#if !defined(FSL_SHA1_HARDENED)
#  define FSL_SHA1_HARDENED 1
#endif
#if defined(__cplusplus)
extern "C" {
#endif

/**
   Various set-in-stone constants used by the API.
*/
enum fsl_hash_constants {
/**
   The length, in bytes, of fossil's hex-form SHA1 UUID strings.
*/
FSL_STRLEN_SHA1 = 40,
/**
   The length, in bytes, of fossil's hex-form SHA3-256 UUID strings.
*/
FSL_STRLEN_K256 = 64,
/**
   The length, in bytes, of a hex-form MD5 hash.
*/
FSL_STRLEN_MD5 = 32,

/** Minimum length of a full UUID. */
FSL_UUID_STRLEN_MIN = FSL_STRLEN_SHA1,
/** Maximum length of a full UUID. */
FSL_UUID_STRLEN_MAX = FSL_STRLEN_K256
};

/**
   Unique IDs for artifact hash types supported by fossil.
*/
enum fsl_hash_types_e {
/** Invalid hash type. */
FSL_HTYPE_ERROR = 0,
/** SHA1. */
FSL_HTYPE_SHA1 = 1,
/** SHA3-256. */
FSL_HTYPE_K256 = 2
};
typedef enum fsl_hash_types_e fsl_hash_types_e;

typedef struct fsl_md5_cx fsl_md5_cx;
typedef struct fsl_sha1_cx fsl_sha1_cx;
typedef struct fsl_sha3_cx fsl_sha3_cx;

/**
   The hash string of the initial MD5 state. Used as an
   optimization for some places where we need an MD5 but know it
   will not hash any data.

   Equivalent to what the md5sum command outputs for empty input:

   @code
   # md5sum < /dev/null
   d41d8cd98f00b204e9800998ecf8427e  -
   @endcode
*/
#define FSL_MD5_INITIAL_HASH "d41d8cd98f00b204e9800998ecf8427e"

/**
   Holds state for MD5 calculations. It is intended to be used like
   this:

   @code
   unsigned char digest[16];
   char hex[FSL_STRLEN_MD5+1];
   fsl_md5_cx cx = fsl_md5_cx_empty;
   // alternately: fsl_md5_init(&cx);
   ...call fsl_md5_update(&cx,...) any number of times to
   ...incrementally calculate the hash.
   fsl_md5_final(&cx, digest); // ends the calculation
   fsl_md5_digest_to_base16(digest, hex);
   // digest now contains the raw 16-byte MD5 digest.
   // hex now contains the 32-byte MD5 + a trailing NUL
   @endcode
*/
struct fsl_md5_cx {
  int isInit;
  uint32_t buf[4];
  uint32_t bits[2];
  unsigned char in[64];
};
#define fsl_md5_cx_empty_m {                                  \
    1/*isInit*/,                                              \
    {/*buf*/0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 }, \
    {/*bits*/0,0},                                            \
    {0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,                \
     0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,                \
     0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,                \
     0,0,0,0}}

/**
   A fsl_md5_cx instance which holds the initial state
   used for md5 calculations. Instances must either be
   copy-initialized from this instance or they must be
   passed to fsl_md5_init() before they are used.
*/
FSL_EXPORT const fsl_md5_cx fsl_md5_cx_empty;

/**
   Initializes the given context pointer. It must not be NULL.  This
   must be the first routine called on any fsl_md5_cx instances.
   Alternately, copy-constructing fsl_md5_cx_empty has the same effect.

   @see fsl_md5_update()
   @see fsl_md5_final()
*/
FSL_EXPORT void fsl_md5_init(fsl_md5_cx *cx);

/**
   Updates cx's state to reflect the addition of the data
   specified by the range (buf, buf+len]. Neither cx nor buf may
   be NULL. This may be called an arbitrary number of times between
   fsl_md5_init() and fsl_md5_final().

   @see fsl_md5_init()
   @see fsl_md5_final()
*/
FSL_EXPORT void fsl_md5_update(fsl_md5_cx *cx, void const * buf, fsl_size_t len);

/**
   Finishes up the calculation of the md5 for the given context and
   writes a 16-byte digest value to the 2nd parameter.  Use
   fsl_md5_digest_to_base16() to convert the digest output value to
   hexadecimal form.

   @see fsl_md5_init()
   @see fsl_md5_update()
   @see fsl_md5_digest_to_base16()
*/
FSL_EXPORT void fsl_md5_final(fsl_md5_cx * cx, unsigned char * digest);

/**
   Converts an md5 digest value (from fsl_md5_final()'s 2nd
   parameter) to a 32-byte (FSL_STRLEN_MD5) CRC string plus a
   terminating NUL byte. i.e.  zBuf must be at least
   (FSL_STRLEN_MD5+1) bytes long.

   @see fsl_md5_final()
*/
FSL_EXPORT void fsl_md5_digest_to_base16(unsigned char *digest, char *zBuf);

/**
   The md5 counterpart of fsl_sha1sum_buffer(), identical in
   semantics except that its result is an MD5 hash instead of an
   SHA1 hash and the resulting hex string is FSL_STRLEN_MD5 bytes
   long plus a terminating NUL.
*/
FSL_EXPORT int fsl_md5sum_buffer(fsl_buffer const *pIn, fsl_buffer *pCksum);

/**
   The md5 counterpart of fsl_sha1sum_cstr(), identical in
   semantics except that its result is an MD5 hash instead of an
   SHA1 hash and the resulting string is FSL_STRLEN_MD5 bytes long
   plus a terminating NUL.
*/
FSL_EXPORT char *fsl_md5sum_cstr(const char *zIn, fsl_int_t len);

/**
   The MD5 counter part to fsl_sha1sum_stream(), with identical
   semantics except that the generated hash is an MD5 string
   instead of SHA1.
*/
FSL_EXPORT int fsl_md5sum_stream(fsl_input_f src, void * srcState, fsl_buffer *pCksum);

/**
   Reads all input from src() and passes it through fsl_md5_update(cx,...).
   Returns 0 on success, FSL_RC_MISUSE if !cx or !src. If src returns
   a non-0 code, that code is returned from here.
*/
FSL_EXPORT int fsl_md5_update_stream(fsl_md5_cx *cx, fsl_input_f src, void * srcState);

/**
   Equivalent to fsl_md5_update(cx, b->mem, b->used). Results are undefined
   if either pointer is invalid or NULL.
*/
FSL_EXPORT void fsl_md5_update_buffer(fsl_md5_cx *cx, fsl_buffer const * b);

/**
   Passes the first len bytes of str to fsl_md5_update(cx). If len
   is less than 0 then fsl_strlen() is used to calculate the
   length.  Results are undefined if either pointer is invalid or
   NULL. This is a no-op if !len or (len<0 && !*str).
*/
FSL_EXPORT void fsl_md5_update_cstr(fsl_md5_cx *cx, char const * str, fsl_int_t len);

/**
   A fsl_md5_update_stream() proxy which updates cx to include the
   contents of the given file.
*/
FSL_EXPORT int fsl_md5_update_filename(fsl_md5_cx *cx, char const * fname);

/**
   The MD5 counter part to fsl_sha1sum_filename(), with identical
   semantics except that the generated hash is an MD5 string
   instead of SHA1.
*/
FSL_EXPORT int fsl_md5sum_filename(const char *zFilename, fsl_buffer *pCksum);


#if FSL_SHA1_HARDENED
typedef void(*fsl_sha1h_collision_callback)(uint64_t, const uint32_t*, const uint32_t*, const uint32_t*, const uint32_t*);
#endif
/**
   Holds state for SHA1 calculations. It is intended to be used
   like this:

   @code
   unsigned char digest[20]
   char hex[FSL_STRLEN_SHA1+1];
   fsl_sha1_cx cx = fsl_sha1_cx_empty;
   // alternately: fsl_sha1_init(&cx)
   ...call fsl_sha1_update(&cx,...) any number of times to
   ...incrementally calculate the hash.
   fsl_sha1_final(&cx, digest); // ends the calculation
   fsl_sha1_digest_to_base16(digest, hex);
   // digest now contains the raw 20-byte SHA1 digest.
   // hex now contains the 40-byte SHA1 + a trailing NUL
   @endcode
*/
struct fsl_sha1_cx {
#if FSL_SHA1_HARDENED
  uint64_t total;
  uint32_t ihv[5];
  unsigned char buffer[64];
  int bigendian;
  int found_collision;
  int safe_hash;
  int detect_coll;
  int ubc_check;
  int reduced_round_coll;
  fsl_sha1h_collision_callback callback;
  uint32_t ihv1[5];
  uint32_t ihv2[5];
  uint32_t m1[80];
  uint32_t m2[80];
  uint32_t states[80][5];
#else
  unsigned int state[5];
  unsigned int count[2];
  unsigned char buffer[64];
#endif
};
/**
   fsl_sha1_cx instance intended for in-struct copy initialization.
*/
#if FSL_SHA1_HARDENED
#define fsl_sha1_cx_empty_m {0}
#else
#define fsl_sha1_cx_empty_m {                                           \
    {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 },      \
    {0,0},                                                              \
    {0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0, \
        0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0 \
        }                                                               \
  }
#endif
/**
   fsl_sha1_cx instance intended for copy initialization. For build
   config portability, the copied-to object must still be passed to
   fsl_sha1_init() to initialize it.
*/
FSL_EXPORT const fsl_sha1_cx fsl_sha1_cx_empty;

/**
   Initializes the given context with the initial SHA1 state.  This
   must be the first routine called on an SHA1 context, and passing
   this context to other SHA1 routines without first having passed
   it to this will lead to undefined results.

   @see fsl_sha1_update()
   @see fsl_sha1_final()
*/
FSL_EXPORT void fsl_sha1_init(fsl_sha1_cx *context);

/**
   Updates the given context to include the hash of the first len
   bytes of the given data.

   @see fsl_sha1_init()
   @see fsl_sha1_final()
*/
FSL_EXPORT void fsl_sha1_update( fsl_sha1_cx *context, void const *data, fsl_size_t len);

/**
   Add padding and finalizes the message digest. If digest is not NULL
   then it writes 20 bytes of digest to the 2nd parameter. If this
   library is configured with hardened SHA1 hashes, this function
   returns non-0 if a collision was detected while hashing. If it is
   not configured for hardened SHA1, or no collision was detected, it
   returns 0.

   @see fsl_sha1_update()
   @see fsl_sha1_digest_to_base16()
*/
FSL_EXPORT int fsl_sha1_final(fsl_sha1_cx *context, unsigned char * digest);

/**
   A convenience form of fsl_sha1_final() which writes
   FSL_STRLEN_SHA1+1 bytes (hash plus terminating NUL byte) to the
   2nd argument and returns a (const char *)-type cast of the 2nd
   argument.
*/
FSL_EXPORT const char * fsl_sha1_final_hex(fsl_sha1_cx *context, char * zHex);

/**
   Convert a digest into base-16.  digest must be at least 20 bytes
   long and hold an SHA1 digest. zBuf must be at least (FSL_STRLEN_SHA1
   + 1) bytes long, to which FSL_STRLEN_SHA1 characters of
   hexidecimal-form SHA1 hash and 1 NUL byte will be written.

   @see fsl_sha1_final()
*/
FSL_EXPORT void fsl_sha1_digest_to_base16(unsigned char *digest, char *zBuf);

/**
   Computes the SHA1 checksum of pIn and stores the resulting
   checksum in the buffer pCksum.  pCksum's memory is re-used if is
   has any allocated to it. pCksum may == pIn, in which case this
   is a destructive operation (replacing the hashed data with its
   hash code).

   Return 0 on success, FSL_RC_OOM if (re)allocating pCksum fails.
*/
FSL_EXPORT int fsl_sha1sum_buffer(fsl_buffer const *pIn, fsl_buffer *pCksum);

/**
   Computes the SHA1 checksum of the first len bytes of the given
   string.  If len is negative then zIn must be NUL-terminated and
   fsl_strlen() is used to find its length. The result is a
   FSL_UUID_STRLEN-byte string (+NUL byte) returned in memory
   obtained from fsl_malloc(), so it must be passed to fsl_free()
   to free it. If NULL==zIn or !len then NULL is returned.
*/
FSL_EXPORT char *fsl_sha1sum_cstr(const char *zIn, fsl_int_t len);

/**
   Consumes all input from src and calculates its SHA1 hash. The
   result is set in pCksum (its contents, if any, are overwritten,
   not appended to). Returns 0 on success. Returns FSL_RC_MISUSE if
   !src or !pCksum. It keeps consuming input from src() until that
   function reads fewer bytes than requested, at which point EOF is
   assumed. If src() returns non-0, that code is returned from this
   function.
*/
FSL_EXPORT int fsl_sha1sum_stream(fsl_input_f src, void * srcState, fsl_buffer *pCksum);


/**
   A fsl_sha1sum_stream() wrapper which calculates the SHA1 of
   given file.

   Returns FSL_RC_IO if the file cannot be opened, FSL_RC_MISUSE if
   !zFilename or !pCksum, else as per fsl_sha1sum_stream().

   TODO: the v1 impl has special behaviour for symlinks which this
   function lacks. For that support we need a variant of this
   function which takes a fsl_cx parameter (for the allow-symlinks
   setting).
*/
FSL_EXPORT int fsl_sha1sum_filename(const char *zFilename, fsl_buffer *pCksum);

/**
   Legal values for SHA3 hash sizes, in bits: an increment of 32 bits
   in the inclusive range (128..512).

   The hexidecimal-code size, in bytes, of any given bit size in this
   enum is the bit size/4.
*/
enum fsl_sha3_hash_size {
/** Sentinel value. Must be 0. */
FSL_SHA3_INVALID = 0,
FSL_SHA3_128 = 128, FSL_SHA3_160 = 160, FSL_SHA3_192 = 192,
FSL_SHA3_224 = 224, FSL_SHA3_256 = 256, FSL_SHA3_288 = 288,
FSL_SHA3_320 = 320, FSL_SHA3_352 = 352, FSL_SHA3_384 = 384,
FSL_SHA3_416 = 416, FSL_SHA3_448 = 448, FSL_SHA3_480 = 480,
FSL_SHA3_512 = 512,
/* Default SHA3 flavor */
FSL_SHA3_DEFAULT = 256
};

/**
   Type for holding SHA3 processing state. Each instance must be
   initialized with fsl_sha3_init(), populated with fsl_sha3_update(),
   and "sealed" with fsl_sha3_end().

   Sample usage:

   @code
   fsl_sha3_cx cx;
   fsl_sha3_init(&cx, FSL_SHA3_DEFAULT);
   fsl_sha3_update(&cx, memory, lengthOfMemory);
   fsl_sha3_end(&cx);
   printf("Hash = %s\n", (char const *)cx.hex);
   @endcode

   After fsl_sha3_end() is called cx.hex contains the hex-string forms
   of the digest. Note that fsl_sha3_update() may be called an arbitrary
   number of times to feed in chunks of memory (e.g. to stream in
   arbitrarily large data).
*/
struct fsl_sha3_cx {
    union {
        uint64_t s[25];         /* Keccak state. 5x5 lines of 64 bits each */
        unsigned char x[1600];  /* ... or 1600 bytes */
    } u;
    unsigned nRate;        /* Bytes of input accepted per Keccak iteration */
    unsigned nLoaded;      /* Input bytes loaded into u.x[] so far this cycle */
    unsigned ixMask;       /* Insert next input into u.x[nLoaded^ixMask]. */
    enum fsl_sha3_hash_size size; /* Size of the hash, in bits. */
    unsigned char hex[132]; /* Hex form of final digest: 56-128 bytes
                               plus terminating NUL. */
};

/**
   If the given number is a valid fsl_sha3_hash_size value, its enum
   entry is returned, else FSL_SHA3_INVALID is returned.

   @see fsl_sha3_init()
*/
FSL_EXPORT enum fsl_sha3_hash_size fsl_sha3_hash_size_for_int(int);

/**
   Initialize a new hash. The second argument specifies the size of
   the hash in bits. Results are undefined if cx is NULL or sz is not
   a valid positive value.

   After calling this, use fsl_sha3_update() to hash data and
   fsl_sha3_end() to finalize the hashing process and generate a digest.
*/
FSL_EXPORT void fsl_sha3_init2(fsl_sha3_cx *cx, enum fsl_sha3_hash_size sz);

/**
   Equivalent to fsl_sha3_init2(cx, FSL_SHA3_DEFAULT).
*/
FSL_EXPORT void fsl_sha3_init(fsl_sha3_cx *cx);

/**
   Updates cx's state to include the first len bytes of data.

   If cx is NULL results are undefined (segfault!). If mem is not
   NULL then it must be at least n bytes long. If n is 0 then this
   function has no side-effects.

   @see fsl_sha3_init()
   @see fsl_sha3_end()
*/
FSL_EXPORT void fsl_sha3_update( fsl_sha3_cx *cx, void const *data, unsigned int len);

/**
   To be called when SHA3 hashing is complete: finishes the hash
   calculation and populates cx->hex with the final hash code in
   hexidecimal-string form. Returns the binary-form digest value,
   which refers to cx->size/8 bytes of memory which lives in the cx
   object. After this call cx->hex will be populated with cx->size/4
   bytes of lower-case ASCII hex codes plus a terminating NUL byte.

   Potential TODO: change fsl_sha1_final() and fsl_md5_final() to use
   these same return semantics.

   @see fsl_sha3_init()
   @see fsl_sha3_update()
*/
FSL_EXPORT unsigned char const * fsl_sha3_end(fsl_sha3_cx *cx);

/**
   SHA3-256 counterpart of fsl_sha1_digest_to_base16(). digest must be at least
   32 bytes long and hold an SHA3 digest. zBuf must be at least (FSL_STRLEN_K256+1)
   bytes long, to which FSL_STRLEN_K256 characters of
   hexidecimal-form SHA3 hash and 1 NUL byte will be written

   @see fsl_sha3_end().
*/
FSL_EXPORT void fsl_sha3_digest_to_base16(unsigned char *digest, char *zBuf);
/**
   SHA3 counter part of fsl_sha1sum_buffer().
*/
FSL_EXPORT int fsl_sha3sum_buffer(fsl_buffer const *pIn, fsl_buffer *pCksum);
/**
   SHA3 counter part of fsl_sha1sum_cstr().
*/
FSL_EXPORT char *fsl_sha3sum_cstr(const char *zIn, fsl_int_t len);
/**
   SHA3 counterpart of fsl_sha1sum_stream().
 */
FSL_EXPORT int fsl_sha3sum_stream(fsl_input_f src, void * srcState, fsl_buffer *pCksum);
/**
   SHA3 counterpart of fsl_sha1sum_filename().
 */
FSL_EXPORT int fsl_sha3sum_filename(const char *zFilename, fsl_buffer *pCksum);

/**
   Expects zHash to be a full-length hash value of one of the
   fsl_hash_types_t-specified types, and nHash to be the length, in
   bytes, of zHash's contents (which must be the full hash length, not
   a prefix). If zHash can be validated as a hash, its corresponding
   hash type is returned, else FSL_HTYPE_ERROR is returned.
*/
FSL_EXPORT fsl_hash_types_e fsl_validate_hash(const char *zHash, int nHash);

/**
   Expects (zHash, nHash) to refer to a full hash (of a supported
   content hash type) of pIn's contents. This routine hashes pIn's
   contents and, if it compares equivalent to zHash then the ID of the
   hash type is returned.  On a mismatch, FSL_HTYPE_ERROR is returned.
*/
FSL_EXPORT fsl_hash_types_e fsl_verify_blob_hash(fsl_buffer const * pIn,
                                                 const char *zHash, int nHash);

/**
   Returns a human-readable name for the given hash type, or its
   second argument h is not a supported hash type.
 */
FSL_EXPORT const char * fsl_hash_type_name(fsl_hash_types_e h, const char *zUnknown);

#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_HASH_H_INCLUDED */
/* end of file ../include/fossil-scm/fossil-hash.h */
/* start of file ../include/fossil-scm/fossil-repo.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_REPO_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_REPO_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).
*/

/** @file fossil-repo.h

    fossil-repo.h declares APIs specifically dealing with
    repository-db-side state, as opposed to specifically checkout-side
    state or non-content-related APIs.
*/


#if defined(__cplusplus)
extern "C" {
#endif

typedef struct fsl_card_F fsl_card_F;
typedef struct fsl_card_J fsl_card_J;
typedef struct fsl_card_Q fsl_card_Q;
typedef struct fsl_card_T fsl_card_T;
typedef struct fsl_checkin_opt fsl_checkin_opt;
typedef struct fsl_deck fsl_deck;

/**
   This function is a programmatic interpretation of
   this table:

   https://fossil-scm.org/index.html/doc/trunk/www/fileformat.wiki#summary

   For a given control artifact type and a card name in the form of
   the card name's letter (e.g. 'A', 'W', ...), this function
   returns 0 (false) if that card type is not permitted in this
   control artifact type, a negative value if the card is optional
   for this artifact type, and a positive value if the card type is
   required for this artifact type.

   As a special case, if t==FSL_SATYPE_ANY then this function
   always returns a negative value as long as card is a valid card
   letter.

   Another special case: when t==FSL_SATYPE_CHECKIN and card=='F',
   this returns a negative value because the table linked to above
   says F-cards are optional. In practice we have yet to find a use
   for checkins with no F-cards, so this library currently requires
   F-cards at checkin-time even though this function reports that
   they are optional.
*/
FSL_EXPORT int fsl_card_is_legal( fsl_satype_e t, char card );

/**
   Artifact tag types used by the Fossil framework. Their values
   are a hard-coded part of the Fossil format, and not subject to
   change (only extension, possibly).
*/
enum fsl_tagtype_e {
/**
   Sentinel value for use with constructors/initializers.
*/
FSL_TAGTYPE_INVALID = -1,
/**
   The "cancel tag" indicator, a.k.a. an anti-tag.
*/
FSL_TAGTYPE_CANCEL = 0,
/**
   The "add tag" indicator, a.k.a. a singleton tag.
*/
FSL_TAGTYPE_ADD = 1,
/**
   The "propagating tag" indicator.
*/
FSL_TAGTYPE_PROPAGATING = 2
};
typedef enum fsl_tagtype_e fsl_tagtype_e;

/**
   Hard-coded IDs used by the 'tag' table of repository DBs. These
   values get installed as part of the base Fossil db schema in new
   repos, and they must never change.
*/
enum fsl_tagid_e {
/**
   DB string tagname='bgcolor'.
*/
FSL_TAGID_BGCOLOR = 1,
/**
   DB: tag.tagname='comment'.
*/
FSL_TAGID_COMMENT = 2,
/**
   DB: tag.tagname='user'.
*/
FSL_TAGID_USER = 3,
/**
   DB: tag.tagname='date'.
*/
FSL_TAGID_DATE = 4,
/**
   DB: tag.tagname='hidden'.
*/
FSL_TAGID_HIDDEN = 5,
/**
   DB: tag.tagname='private'.
*/
FSL_TAGID_PRIVATE = 6,
/**
   DB: tag.tagname='cluster'.
*/
FSL_TAGID_CLUSTER = 7,
/**
   DB: tag.tagname='branch'.
*/
FSL_TAGID_BRANCH = 8,
/**
   DB: tag.tagname='closed'.
*/
FSL_TAGID_CLOSED = 9,
/**
   DB: tag.tagname='parent'.
*/
FSL_TAGID_PARENT = 10,
/**
   DB: tag.tagname='note'

   Extra text appended to a check-in comment.
*/
FSL_TAGID_NOTE = 11,

/**
   Largest tag ID reserved for internal use.
*/
FSL_TAGID_MAX_INTERNAL = 99
};


/**
   Returns one of '-', '+', or '*' for a valid input parameter, 0
   for any other value.
*/
FSL_EXPORT char fsl_tag_prefix_char( fsl_tagtype_e t );


/**
   A list of fsl_card_F objects. F-cards used a custom list type,
   instead of the framework's generic fsl_list, because experience has
   shown that the number of (de)allocations we need for F-card lists
   has a large negative impact when parsing and creating artifacts en
   masse. This list type, unlike fsl_list, uses a conventional object
   array approach to storage, as opposed to an array of pointers (each
   entry of which has to be separately allocated).

   These lists, and F-cards in generally, are typically maintained
   internally in the library. There's probably "no good reason" for
   clients to manipulate them.
*/
struct fsl_card_F_list {
  /**
     The list of F-cards. The first this->used elements are in-use.
     This pointer may change any time the list is reallocated.a
  */
  fsl_card_F * list;
  /**
     The number of entries in this->list which are in use.
  */
  uint32_t used;
  /**
     The number of entries currently allocated in this->list.
  */
  uint32_t capacity;
  /**
     An internal cursor into this->list, used primarily for
     properly traversing the file list in delta manifests.

     Maintenance notes: internal updates to this member are the only
     reason some of the deck APIs require a non-const deck. This type
     needs to be signed for compatibility with some of the older
     algos, e.g. fsl_deck_F_seek_base().
  */
  int32_t cursor;
  /**
     Internal flags. Never, ever modify these from client code.
  */
  uint32_t flags;
};
typedef struct fsl_card_F_list fsl_card_F_list;
/** Empty-initialized fsl_card_F instance for const copy
    initialization */
#define fsl_card_F_list_empty_m {NULL, 0, 0, 0, 0}
/** Empty-initialized fsl_card_F instance for non-const copy
    initialization */
FSL_EXPORT const fsl_card_F_list fsl_card_F_list_empty;

/**
   A "deck" stores (predictably enough) a collection of "cards."
   Cards are constructs embedded within Fossil's Structural Artifacts
   to denote various sorts of changes in a Fossil repository, and a
   Deck encapsulates the cards for a single Structural Artifact of an
   arbitrary type, e.g. Manifest (a.k.a. "checkin") or Cluster. A card
   is basically a command with a single-letter name and a well-defined
   signature for its arguments. Each card is represented by a member
   of this struct whose name is the same as the card type
   (e.g. fsl_card::C holds a C-card and fsl_card::F holds a list of
   F-card). Each type of artifact only allows certain types of
   card. The complete list of valid card/construct combinations can be
   found here:

   https://fossil-scm.org/home/doc/trunk/www/fileformat.wiki#summary

   fsl_card_is_legal() can be used determine if a given card type
   is legal (per the above chart) with a given Control Artifiact
   type (as stored in the fsl_deck::type member).

   The type member is used by some algorithms to determine which
   operations are legal on a given artifact type, so that they can
   fail long before the user gets a chance to add a malformed artifact
   to the database. Clients who bypass the fsl_deck APIs and
   manipulate the deck's members "by hand" (so to say) effectively
   invoke undefined behaviour.

   The various routines to add/set cards in the deck are named
   fsl_deck_CARDNAME_add() resp. fsl_deck_CARDNAME_set(). The "add"
   functions represent cards which may appear multiple times
   (e.g. the 'F' card) or have multiple values (the 'P' card), and
   those named "set" represent unique or optional cards. The R-card
   is the outlier, with fsl_deck_R_calc(). NEVER EVER EVER directly
   modify a member of this struct - always use the APIs. The
   library performs some optimizations which can lead to corrupt
   memory and invalid free()s if certain members' values are
   directly replaced by the client (as opposed to via the APIs).

   Note that the 'Z' card is not in this structure because it is a
   hash of the other inputs and is calculated incrementally and
   appended automatically by fsl_deck_output().

   All non-const pointer members of this structure are owned by the
   structure instance unless noted otherwise (the fsl_deck::f member
   being the notable exception).

   Maintenance reminder: please keep the card members alpha sorted to
   simplify eyeball-searching through their docs.

   @see fsl_deck_malloc()
   @see fsl_deck_init()
   @see fsl_deck_parse()
   @see fsl_deck_load_rid()
   @see fsl_deck_finalize()
   @see fsl_deck_clean()
   @see fsl_deck_save()
   @see fsl_deck_A_set()
   @see fsl_deck_B_set()
   @see fsl_deck_D_set()
   @see fsl_deck_E_set()
   @see fsl_deck_F_add()
   @see fsl_deck_J_add()
   @see fsl_deck_K_set()
   @see fsl_deck_L_set()
   @see fsl_deck_M_add()
   @see fsl_deck_N_set()
   @see fsl_deck_P_add()
   @see fsl_deck_Q_add()
   @see fsl_deck_R_set()
   @see fsl_deck_T_add()
   @see fsl_deck_branch_set()
   @see fsl_deck_U_set()
   @see fsl_deck_W_set()
*/
struct fsl_deck {
  /**
     Specifies the the type (or eventual type) of this
     artifact. The function fsl_card_is_legal() can be used to
     determined if a given card type is legal for a given value of
     this member. APIs which add/set cards use that to determine if
     the operation requested by the client is semantically legal.
  */
  fsl_satype_e type;

  /**
     DB repo.blob.rid value. Normally set by fsl_deck_parse().
  */
  fsl_id_t rid;

  /**
     Gets set by fsl_deck_parse() to the hash/UUID of the
     manifest it parsed.
  */
  fsl_uuid_str uuid;

  /**
     The Fossil context responsible for this deck. Though this data
     type is normally, at least conceptually, free of any given fossil
     context, many related algorithms need a context in order to
     perform db- or caching-related work, as well as to simplify error
     message propagation. We store this as a struct member to keep all
     such algorithms from redundantly requiring both pieces of
     information as arguments.

     This object does not own the context and the context object must
     outlive this deck instance.
  */
  fsl_cx * f;

  /**
     The 'A' (attachment) card. Only used by FSL_SATYPE_ATTACHMENT
     decks. The spec currently specifies only 1 A-card per
     manifest, but conceptually this could/should be a list.
  */
  struct {
    /**
       Filename of the A-card.
    */
    char * name;

    /**
       Name of wiki page, or UUID of ticket or event (technote), to
       which the attachment applies.
    */
    char * tgt;

    /**
       UUID of the file being attached via the A-card.
    */
    fsl_uuid_str src;
  } A;

  struct {
    /**
       The 'B' (baseline) card holds the UUID of baseline manifest.
       This is empty for baseline manifests and holds the UUID of
       the parent for delta manifests.
    */
    fsl_uuid_str uuid;

    /**
       Baseline manifest corresponding to this->B. It is loaded on
       demand by routines which need it, typically by calling
       fsl_deck_F_rewind() (unintuitively enough!). The
       parent/child relationship in Fossil is the reverse of
       conventional - children own their parents, not the other way
       around. i.e. this->baseline will get cleaned up
       (recursively) when this instance is cleaned up (when the
       containing deck is cleaned up).
    */
    fsl_deck * baseline;
  } B;
  /**
     The 'C' (comment) card.
  */
  char * C;

  /**
     The 'D' (date) card, in Julian format.
  */
  double D;

  /**
     The 'E' (event) card.
  */
  struct {
    /**
       The 'E' card's date in Julian Day format.
    */
    double julian;

    /**
       The 'E' card's UUID.
    */
    fsl_uuid_str uuid;
  } E;

  /**
     The 'F' (file) card container.
  */
  fsl_card_F_list F;
  
  /**
     UUID for the 'G' (forum thread-root) card.
  */
  fsl_uuid_str G;

  /**
     The H (forum title) card.
  */
  char * H;

  /**
     UUID for the 'I' (forum in-response-to) card.
  */
  fsl_uuid_str I;

  /**
     The 'J' card specifies changes to "value" of "fields" in
     tickets (FSL_SATYPE_TICKET).

     Holds (fsl_card_J*) entries.
  */
  fsl_list J;

  /**
     UUID for the 'K' (ticket) card.
  */
  fsl_uuid_str K;

  /**
     The 'L' (wiki name/title) card.
  */
  char * L;

  /**
     List of UUIDs (fsl_uuid_str) in a cluster ('M' cards).
  */
  fsl_list M;

  /**
     The 'N' (comment mime type) card. Note that this is only
     ostensibly supported by fossil, but fossil does not (as of
     2021-04-13) honor this value and always assumes that its value is
     "text/x-fossil-wiki".
  */
  char * N;

  /**
     List of UUIDs of parents ('P' cards). Entries are of type
     (fsl_uuid_str).
  */
  fsl_list P;

  /**
     'Q' (cherry pick) cards. Holds (fsl_card_Q*) entries.
  */
  fsl_list Q;

  /**
     The R-card holds an MD5 hash which is calculated based on the
     names, sizes, and contents of the files included in a
     manifest. See the class-level docs for a link to a page which
     describes how this is calculated.
  */
  char * R;

  /**
     List of 'T' (tag) cards. Holds (fsl_card_T*) instances.
  */
  fsl_list T;

  /**
     The U (user) card.
  */
  char * U;

  /**
     The W (wiki content) card.
  */
  fsl_buffer W;

  /**
     This is part of an optimization used when parsing fsl_deck
     instances from source text. For most types of card we re-use
     string values in the raw source text rather than duplicate them,
     and that requires storing the original text (as passed to
     fsl_deck_parse()). This requires that clients never tinker
     directly with values in a fsl_deck, in particular never assign
     over them or assume they know who allocated the memory for that
     bit.
  */
  fsl_buffer content;

  /**
     To potentially be used for a manifest cache.
  */
  fsl_deck * next;

  /**
     A marker which tells fsl_deck_finalize() whether or not
     fsl_deck_malloc() allocated this instance (in which case
     fsl_deck_finalize() will fsl_free() it) or not (in which case
     it does not fsl_free() it).
  */
  void const * allocStamp;
};

/**
   Initialized-with-defaults fsl_deck structure, intended for copy
   initialization.
*/
FSL_EXPORT const fsl_deck fsl_deck_empty;

/**
   Initialized-with-defaults fsl_deck structure, intended for
   in-struct and const copy initialization.
*/
#define fsl_deck_empty_m {                                  \
    FSL_SATYPE_ANY /*type*/,                                \
    0/*rid*/,                                            \
    NULL/*uuid*/,                                         \
    NULL/*f*/,                                            \
    {/*A*/ NULL /* name */,                               \
           NULL /* tgt */,                                   \
           NULL /* src */},                                  \
    {/*B*/ NULL /*uuid*/,                                 \
           NULL /*baseline*/},                               \
    NULL /* C */,                                       \
    0.0 /*D*/,                                        \
    {/*E*/ 0.0 /* julian */,                          \
           NULL /* uuid */},                             \
    /*F*/ fsl_card_F_list_empty_m,  \
    0/*G*/,0/*H*/,0/*I*/,                           \
    fsl_list_empty_m /* J */,                       \
    NULL /* K */,                                 \
    NULL /* L */,                                 \
    fsl_list_empty_m /* M */,                     \
    NULL /* N */,                                 \
    fsl_list_empty_m /* P */,                     \
    fsl_list_empty_m /* Q */,                     \
    NULL /* R */,                                 \
    fsl_list_empty_m /* T */,                     \
    NULL /* U */,                                 \
    fsl_buffer_empty_m /* W */,                   \
    fsl_buffer_empty_m/*content*/,                \
    NULL/*next*/,                                 \
    NULL/*allocStamp*/                            \
  }


/**
   Allocates a new fsl_deck instance. Returns NULL on allocation
   error. The returned value must eventually be passed to
   fsl_deck_finalize() to free its resources.

   @see fsl_deck_finalize()
   @see fsl_deck_clean()
*/
FSL_EXPORT fsl_deck * fsl_deck_malloc();

/**
   Frees all resources belonging to the given deck's members
   (including its parents, recursively), and wipes deck clean of most
   state, but does not free() deck. Is a no-op if deck is NULL. As a
   special case, the (allocStamp, f) members of deck are kept intact.

   @see fsl_deck_finalize()
   @see fsl_deck_malloc()
   @see fsl_deck_clean2()
*/
FSL_EXPORT void fsl_deck_clean(fsl_deck *deck);

/**
   A variant of fsl_deck_clean() which "returns" its content buffer
   for re-use by transferring, after ensuring proper cleanup of its
   internals, its own content buffer's bytes into the given target
   buffer. Note that decks created "manually" do not have any content
   buffer contents, but those loaded via fsl_deck_load_rid() do. This
   function will fsl_buffer_swap() the contents of the given buffer
   (if any) with its own buffer, clean up its newly-acquired memory
   (tgt's previous contents, if any), and fsl_buffer_reuse() the
   output buffer.

   If tgt is NULL, this behaves exactly like fsl_deck_clean().
*/
FSL_EXPORT void fsl_deck_clean2(fsl_deck *deck, fsl_buffer *tgt);

/**
   Frees all memory owned by deck (see fsl_deck_clean()).  If deck
   was allocated using fsl_deck_malloc() then this function
   fsl_free()'s it, otherwise it does not free it.

   @see fsl_deck_malloc()
   @see fsl_deck_clean()
*/
FSL_EXPORT void fsl_deck_finalize(fsl_deck *deck);

/**
   Sets the A-card for an Attachment (FSL_SATYPE_ATTACHMENT)
   deck. Returns 0 on success.

   Returns FSL_RC_MISUSE if any of (mf, filename, target) are NULL,
   FSL_RC_RANGE if !*filename or if uuidSrc is not NULL and
   fsl_is_uuid(uuidSrc) returns false.

   Returns FSL_RC_TYPE if mf is not (as determined by its mf->type
   member) of a deck type capable of holding 'A' cards. (Only decks
   of type FSL_SATYPE_ATTACHMENT may hold an 'A' card.) If uuidSrc
   is NULL or starts with a NUL byte then it is ignored, otherwise
   the same restrictions apply to it as to target.

   The target parameter represents the "name" of the
   wiki/ticket/event record to which the attachment applies. For
   wiki pages this is their normal name (e.g. "MyWikiPage"). For
   events and tickets it is their full 40-byte UUID.

   uuidSrc is the UUID of the attachment blob itself. If it is NULL
   or empty then this card indicates that the attachment will be
   "deleted" (insofar as anything is ever deleted in Fossil).
*/
FSL_EXPORT int fsl_deck_A_set( fsl_deck * mf, char const * filename,
                               char const * target,
                               fsl_uuid_cstr uuidSrc);

/**
   Sets or unsets (if uuidBaseline is NULL or empty) the B-card for
   the given manifest to a copy of the given UUID. Returns 0 on
   success, FSL_RC_MISUSE if !mf, FSL_RC_OOM on allocation
   error. Setting this will free any prior values in mf->B, including
   a previously loaded mf->B.baseline.

   If uuidBaseline is not NULL and fsl_is_uuid() returns false,
   FSL_RC_SYNTAX is returned. If it is NULL the current value is
   freed (semantically, though the deck may still own the memory), the
   B card is effectively removed, and 0 is returned.

   Returns FSL_RC_TYPE if mf is not syntactically allowed to have
   this card card (as determined by
   fsl_card_is_legal(mf->type,...)).

   Sidebar: the ability to unset this card is unusual within this API,
   and is a requirement the library-internal delta manifest creation
   process. Most of the card-setting APIs, even when they are
   described as working like this one, do not accept NULL hash values.
*/
FSL_EXPORT int fsl_deck_B_set( fsl_deck * mf, fsl_uuid_cstr uuidBaseline);

/**
   Semantically identical to fsl_deck_B_set() but sets the C-card and
   does not place a practical limit on the comment's length.  comment
   must be the comment text for the change being applied.  If the
   given length is negative, fsl_strlen() is used to determine its
   length.
*/
FSL_EXPORT int fsl_deck_C_set( fsl_deck * mf, char const * comment, fsl_int_t cardLen);

/**
   Sets mf's D-card as a Julian Date value. Returns FSL_RC_MISUSE if
   !mf, FSL_RC_RANGE if date is negative, FSL_RC_TYPE if a D-card is
   not valid for the given deck, else 0. Passing a value of 0
   effectively unsets the card.
*/
FSL_EXPORT int fsl_deck_D_set( fsl_deck * mf, double date);

/**
   Sets the E-card in the given deck. date may not be negative -
   use fsl_db_julian_now() or fsl_julian_now() to get a default
   time if needed.  Retursn FSL_RC_MISUSE if !mf or !uuid,
   FSL_RC_RANGE if date is not positive, FSL_RC_RANGE if uuid is
   not a valid UUID string.

   Note that the UUID for an event, unlike most other UUIDs, need
   not be calculated - it may be a random hex string, but it must
   pass the fsl_is_uuid() test. Use fsl_db_random_hex() to generate
   random UUIDs. When editing events, e.g. using the HTML UI, only
   the most recent event with the same UUID is shown. So when
   updating events, be sure to apply the same UUID to the edited
   copies before saving them.
*/
FSL_EXPORT int fsl_deck_E_set( fsl_deck * mf, double date, fsl_uuid_cstr uuid);

/**
   Adds a new F-card to the given deck. The uuid argument is required
   to be NULL or pass the fsl_is_uuid() test. The name must be a
   "simplified path name" (as determined by fsl_is_simple_pathname()),
   or FSL_RC_RANGE is returned. Note that a NULL uuid is only valid
   when constructing a delta manifest, and this routine will return
   FSL_RC_MISUSE and update d->f's error state if uuid is NULL and
   d->B.uuid is also NULL.

   perms should be one of the fsl_fileperm_e values (0 is the usual
   case).

   priorName must only be non-NULL when renaming a file, and it must
   follow the same naming rules as the name parameter.

   Returns 0 on success.

   @see fsl_deck_F_set()
*/
FSL_EXPORT int fsl_deck_F_add( fsl_deck * d, char const * name,
                               fsl_uuid_cstr uuid,
                               fsl_fileperm_e perm, 
                               char const * priorName);

/**
   Works mostly like fsl_deck_F_add() except that:

   1) It enables replacing an existing F-card with a new one matching
   the same name.

   2) It enables removing an F-card by passing a NULL uuid.

   3) It refuses to work on a deck for which d->uuid is not NULL or
   d->rid!=0, returning FSL_RC_MISUSE if either of those apply.

   If d contains no F-card matching the given name (case-sensitivity
   depends on d->f's fsl_cx_is_case_sensitive() value) then:

   - If the 3rd argument is NULL, it returns FSL_RC_NOT_FOUND with
     (effectively) no side effects (aside, perhaps, from sorting d->F
     if needed to perform the search).

   - If the 3rd argument is not NULL then it behaves identically to
     fsl_deck_F_add().

   If a match is found, then:

   - If the 3rd argument is NULL, it removes that entry from the
     F-card list and returns 0.

   - If the 3rd argument is not NULL, the fields of the resulting
     F-card are modified to match the arguments passed to this
     function, copying the values of all C-string arguments. (Sidebar:
     we may need to copy the name, despite already knowing it, because
     of how fsl_deck instances manage F-card memory.)

   In all cases, if the 3rd argument is NULL then the 4th and 5th
   arguments are ignored.

   Returns 0 on success, FSL_RC_OOM if an allocation fails.  See
   fsl_deck_F_add() for other failure modes. On error, d's F-card list
   may be left in an inconsistent state and it must not be used
   further.

   @see fsl_deck_F_add()
   @see fsl_deck_F_set_content()
*/
FSL_EXPORT int fsl_deck_F_set( fsl_deck * d, char const * name,
                               fsl_uuid_cstr uuid,
                               fsl_fileperm_e perm, 
                               char const * priorName);
/**
   UNDER CONSTRUCTION! EXPERIMENTAL!

   This variant of fsl_deck_F_set() accepts a buffer of content to
   store as the file's contents. Its hash is derived from that
   content, using fsl_repo_blob_lookup() to hash the given content.
   Thus this routine can replace existing F-cards and save their
   content at the same time. When doing so, it will try to make the
   parent version (if this is a replacement F-card) a delta of the new
   content version (it may refuse to do so for various resources, but
   any heuristics which forbid that will not trigger an error).

   The intended use of this routine is for adding or replacing content
   in a deck which has been prepared using fsl_deck_derive().

   Returns 0 on success, else an error code propagated by
   fsl_deck_F_set(), fsl_repo_blob_lookup(), or some other lower-level
   routine. This routine requires that a transaction is active and
   returns FSL_RC_MISUSE if none is active. For any non-trivial
   error's, d->f's error state will be updated with a description of
   the problem.

   TODO: add a fsl_cx-level or fsl_deck-level API for marking content
   saved this way as private. This type of content is intended for use
   cases which do not have a checkout, and thus cannot be processed
   with fsl_checkin_commit() (which includes a flag to mark its
   content as private).

   @see fsl_deck_F_set()
   @see fsl_deck_F_add()
   @see fsl_deck_derive()
*/
FSL_EXPORT int fsl_deck_F_set_content( fsl_deck * d, char const * name,
                                       fsl_buffer const * src,
                                       fsl_fileperm_e perm, 
                                       char const * priorName);

/**
   UNDER CONSTRUCTION! EXPERIMENTAL!

   This routine rewires d such that it becomes the basis for a derived
   version of itself. Requires that d be a loaded
   from a repository, complete with a UUID and an RID, else
   FSL_RC_MISUSE is returned.

   In short, this function peforms the following:

   - Clears d->P
   - Moves d->uuid into d->P
   - Clears d->rid
   - Clears any other members which need to be (re)set by the new
     child/derived version.
   - It specifically keeps d->F intact OR creates a new one (see below).

   Returns 0 on success, FSL_RC_OOM on an allocation error,
   FSL_RC_MISUSE if d->uuid is NULL or d->rid<=0. If d->type is not
   FSL_SATYPE_CHECKIN, FSL_RC_TYPE is returned. On error, d may be
   left in an inconsistent state and must not be used further except
   to pass it to fsl_deck_finalize().

   The intention of this function is to simplify creation of decks
   which are to be used for creating checkins without requiring a
   checkin.

   To avoid certain corner cases, this function does not allow
   creation of delta manifests. If d has a B-card then it is a delta.
   This function clears its B-card and recreates the F-card list using
   the B-card's F-card list and any F-cards from the current delta. In
   other words, it creates a new baseline manifest.

   TODO: extend this to support other inheritable deck types, e.g.
   wiki, forum posts, and technotes.

   @see fsl_deck_F_set_content()
*/
FSL_EXPORT int fsl_deck_derive(fsl_deck * d);

/**
   Callback type for use with fsl_deck_F_foreach() and
   friends. Implementations must return 0 on success, FSL_RC_BREAK
   to abort looping without an error, and any other value on error.
*/
typedef int (*fsl_card_F_visitor_f)(fsl_card_F const * fc,
                                     void * state);

/**
   For each F-card in d, cb(card,visitorState) is called. Returns
   the result of that loop. If cb returns FSL_RC_BREAK, the
   visitation loop stops immediately and this function returns
   0. If cb returns any other non-0 code, looping stops and that
   code is returned immediately.

   This routine calls fsl_deck_F_rewind() to reset the F-card cursor
   and/or load d's baseline manifest (if any). If loading the baseline
   fails, an error code from fsl_deck_baseline_fetch() is returned.

   The F-cards will be visited in the order they are declared in
   d. For loaded-from-a-repo manifests this is always lexical order
   (for delta manifests, consistent across the delta and
   baseline). For hand-created decks which have not yet been
   fsl_deck_unshuffle()'d, the order is unspecified.
*/
FSL_EXPORT int fsl_deck_F_foreach( fsl_deck * d, fsl_card_F_visitor_f cb,
                                   void * visitorState );

/**
   Fetches the next F-card entry from d. fsl_deck_F_rewind() must
   have be successfully executed one time before calling this, as
   that routine ensures that the baseline is loaded (if needed),
   which is needed for proper iteration over delta manifests.

   This routine always assigns *f to NULL before starting its work, so
   the client can be assured that it will never contain the same value
   as before calling this (unless that value was NULL).

   On success 0 is returned and *f is assigned to the next F-card.
   If *f is NULL when returning 0 then the end of the list has been
   reached (fsl_deck_F_rewind() can be used to re-set it).

   Example usage:

   @code
   int rc;
   fsl_card_F const * fc = NULL;
   rc = fsl_deck_F_rewind(d);
   if(!rc) while( !(rc=fsl_deck_F_next(d, &fc)) && fc) {...}
   @endcode

   Note that files which were deleted in a given version are not
   recorded in baseline manifests but are in deltas. To avoid
   inconsistencies, this routine does NOT include deleted files in its
   results, regardless of whether d is a baseline or delta. (It used
   to, but that turned out to be a design flaw.)

   Implementation notes: for baseline manifests this is a very
   fast and simple operation. For delta manifests it gets
   rather complicated.
*/
FSL_EXPORT int fsl_deck_F_next( fsl_deck * d, fsl_card_F const **f );

/**
   Rewinds d's F-card traversal iterator and loads d's baseline
   manifest, if it has one (i.e. if d->B.uuid is not NULL) and it is
   not loaded already (i.e. if d->B.baseline is NULL). Returns 0 on
   success. The only error condition is if loading of the a baseline
   manifest fails, noting that only delta manifests have baselines.

   Results are undefined if d->f is NULL, and that may trigger an
   assert() in debug builds.
*/
FSL_EXPORT int fsl_deck_F_rewind( fsl_deck * d );

/**
   Looks for a file in a manifest or (for a delta manifest) its
   baseline. No normalization of the given filename is performed -
   it is assumed to be relative to the root of the checkout.

   It requires that d->type be FSL_SATYPE_CHECKIN and that d be
   loaded from a stored manifest or have been fsl_deck_unshuffle()'d
   (if called on an under-construction deck). Specifically, this
   routine requires that d->F be sorted properly or results are
   undefined.

   d->f is assumed to be the fsl_cx instance which deck was loaded
   from, which impacts the search process as follows:

   - The search take's d->f's underlying case-insensitive option into
   account. i.e. if case-insensitivy is on then files in any case
   will match.

   - If no match is found in d and is a delta manifest (d->B.uuid
   is set) then d's baseline is lazily loaded (if needed) and
   the search continues there. (Delta manifests are only one level
   deep, so this is not recursive.)

   Returns NULL if !d, !d->f, or d->type!=FSL_SATYPE_CHECKIN, if no
   entry is found, or if delayed loading of the parent manifest (if
   needed) of a delta manifest fails (in which case d->f's error
   state should hold more information about the problem).

   In debug builds this function asserts that d is not NULL.

   Design note: d "should" be const, but search optimizations for
   the typical use case require potentially lazy-loading
   d->B.baseline and updating d->F.
*/
FSL_EXPORT fsl_card_F const * fsl_deck_F_search(fsl_deck *d, const char *zName);

/**
   Given two F-card instances, this function compares their names
   (case-insensitively). Returns a negative value if lhs is
   lexically less than rhs, a positive value if lhs is lexically
   greater than rhs, and 0 if they are lexically equivalent (or are
   the same pointer).

   Results are undefined if either argument is NULL.
*/
FSL_EXPORT int fsl_card_F_compare( fsl_card_F const * lhs,
                                   fsl_card_F const * rhs);

/**
   If fc->uuid refers to a blob in f's repository database then that
   content is placed into dest (as per fsl_content_get()) and 0 is
   returned. Returns FSL_RC_NOT_FOUND if fc->uuid is not
   found. Returns FSL_RC_MISUSE if any argument is NULL.  If
   fc->uuid is NULL (indicating that it refers to a file deleted in
   a delta manifest) then FSL_RC_RANGE is returned. Returns
   FSL_RC_NOT_A_REPO if f has no repository opened.

   On any error but FSL_RC_MISUSE (basic argument validation) f's
   error state is updated to describe the error.

   @see fsl_content_get()
*/
FSL_EXPORT int fsl_card_F_content( fsl_cx * f, fsl_card_F const * fc,
                                   fsl_buffer * dest );

/**
   Sets the 'G' card on a forum-post deck to a copy of the given
   UUID.
*/
FSL_EXPORT int fsl_deck_G_set( fsl_deck * mf, fsl_uuid_cstr uuid);
/**
   Sets the 'H' card on a forum-post deck to a copy of the given
   comment. If cardLen is negative then fsl_strlen() is used to
   calculate its length.
 */
FSL_EXPORT int fsl_deck_H_set( fsl_deck * mf, char const * comment, fsl_int_t cardLen);
/**
   Sets the 'I' card on a forum-post deck to a copy of the given
   UUID.
*/
FSL_EXPORT int fsl_deck_I_set( fsl_deck * mf, fsl_uuid_cstr uuid);

/**
   Adds a J-card to the given deck, setting/updating the given ticket
   property key to the given value. The key is required but the value
   is optional (may be NULL). If isAppend then the value is appended
   to any existing value, otherwise it replaces any existing value.

   It is currently unclear whether it is legal to include multiple
   J cards for the same key in the same control artifact, in
   particular if their isAppend values differ.

   Returns 0 on success, FSL_RC_MISUSE if !mf or !key, FSL_RC_RANGE
   if !*field, FSL_RC_TYPE if mf is of a type for which J cards are
   not legal (see fsl_card_is_legal()), FSL_RC_OOM on allocation
   error.
*/
FSL_EXPORT int fsl_deck_J_add( fsl_deck * mf, char isAppend,
                               char const * key, char const * value );

/**
   Semantically identical fsl_deck_B_set() but sets the K-card and
   does not accept a NULL value.  uuid must be the UUID of the ticket
   this change is being applied to.
*/
FSL_EXPORT int fsl_deck_K_set( fsl_deck * mf, fsl_uuid_cstr uuid);

/**
   Semantically identical fsl_deck_B_set() but sets the L-card.
   title must be the wiki page title text of the wiki page this
   change is being applied to.
*/
FSL_EXPORT int fsl_deck_L_set( fsl_deck * mf, char const *title, fsl_int_t len);

/**
   Adds the given UUID as an M-card entry. Returns 0 on success, or:

   FSL_RC_MISUSE if !mf or !uuid

   FSL_RC_TYPE if fsl_deck_check_type(mf,'M') returns false.

   FSL_RC_RANGE if !fsl_is_uuid(uuid).

   FSL_RC_OOM if memory allocation fails while adding the entry.
*/
FSL_EXPORT int fsl_deck_M_add( fsl_deck * mf, fsl_uuid_cstr uuid );

/**
   Semantically identical to fsl_deck_B_set() but sets the N card.
   mimeType must be the content mime type for comment text of the
   change being applied.
*/
FSL_EXPORT int fsl_deck_N_set( fsl_deck * mf, char const *mimeType, fsl_int_t len);

/**
   Adds the given UUID as a parent of the given change record. If len
   is less than 0 then fsl_strlen(parentUuid) is used to determine
   its length. Returns FSL_RC_MISUE if !mf, !parentUuid, or
   !*parentUuid. Returns FSL_RC_RANGE if parentUuid is not 40
   bytes long.

   The first P-card added to a deck MUST be the UUID of its primary
   parent (one which was not involved in a merge operation). All
   others (from merges) are considered "non-primary."

*/
FSL_EXPORT int fsl_deck_P_add( fsl_deck * mf, fsl_uuid_cstr parentUuid);

/**
   If d contains a P card with the given index, this returns the
   RID corresponding to the UUID at that index. Returns a negative
   value on error, 0 if there is no for that index or the index
   is out of bounds.
*/
FSL_EXPORT fsl_id_t fsl_deck_P_get_id(fsl_deck * d, int index);

/**
   Adds a Q-card record to the given deck. The type argument must
   be negative for a backed-out change, positive for a cherrypicked
   change.  target must be a valid UUID string. If baseline is not
   NULL then it also must be a valid UUID.

   Returns 0 on success, non-0 on error. FSL_RC_MISUSE if !mf
   or !target, FSL_RC_RANGE if target/baseline are not valid
   UUID strings (baseline may be NULL).
*/
FSL_EXPORT int fsl_deck_Q_add( fsl_deck * mf, int type,
                    fsl_uuid_cstr target,
                    fsl_uuid_cstr baseline );

/**
   Functionally identical to fsl_deck_B_set() except that it sets
   the R-card. Returns 0 on succes, FSL_RC_RANGE if md5 is not NULL
   or exactly FSL_STRLEN_MD5 bytes long (not including trailing
   NUL). If md5==NULL the current R value is cleared.

   It would be highly unusual to have to set the R-card manually,
   as its calculation is quite intricate/intensive. See
   fsl_deck_R_calc() and fsl_deck_unshuffle() for details
*/
FSL_EXPORT int fsl_deck_R_set( fsl_deck * mf, char const *md5);

/**
   Adds a new T-card (tag) entry to the given deck.

   If uuid is not NULL and fsl_is_uuid(uuid) returns false then
   this function returns FSL_RC_RANGE. If uuid is NULL then it is
   assumed to be the UUID of the currently-being-constructed
   artifact in which the tag is contained (which appears as the '*'
   character in generated artifacts).

   Returns 0 on success. Returns FSL_RC_MISUE if !mf or
   !name. Returns FSL_RC_TYPE (and update's mf's error state with a
   message) if the T card is not legal for mf (see
   fsl_card_is_legal()).  Returns FSL_RC_RANGE if !*name, tagType
   is invalid, or if uuid is not NULL and fsl_is_uuid(uuid)
   return false. Returns FSL_RC_OOM if an allocation fails.
*/
FSL_EXPORT int fsl_deck_T_add( fsl_deck * mf, fsl_tagtype_e tagType,
                               fsl_uuid_cstr uuid, char const * name,
                               char const * value);

/**
   Adds the given tag instance to the given manifest.
   Returns 0 on success, FSL_RC_MISUSE if either argument
   is NULL, FSL_RC_OOM if appending the tag to the list
   fails.

   On success ownership of t is passed to mf. On error ownership is
   not modified.
*/
FSL_EXPORT int fsl_deck_T_add2( fsl_deck * mf, fsl_card_T * t);

/**
   A convenience form of fsl_deck_T_add() which adds two propagating
   tags to the given deck: "branch" with a value of branchName and
   "sym-branchName" with no value.

   Returns 0 on success. Returns FSL_RC_OOM on allocation error and
   FSL_RC_RANGE if branchName is empty or contains any characters with
   ASCII values <=32d. It natively assumes that any characters >=128
   are part of multibyte UTF8 characters.
*/
FSL_EXPORT int fsl_deck_branch_set( fsl_deck * d, char const * branchName );

/**
   Calculates the value of d's R-card based on its F-cards and updates
   d->R. It may also, as a side-effect, sort d->F.list lexically (a
   requirement of a R-card calculation).

   Returns 0 on success. Requires that d->f have an opened
   repository db, else FSL_RC_NOT_A_REPO is returned. If d's type is
   not legal for an R-card then FSL_RC_TYPE is returned and d->f's
   error state is updated with a description of the error. If d is of
   type FSL_SATYPE_CHECKIN and has no F-cards then the R-card's value
   is that of the initial MD5 hash state. Various other codes can be
   returned if fetching file content from the db fails.

   Note that this calculation is exceedingly memory-hungry. While
   Fossil originally required R-cards, the cost of calculation
   eventually caused the R-card to be made optional. This API
   allows the client to decide on whether to use them (for more
   (admittedly redundant!) integrity checking) or not (much faster
   but "not strictly historically correct"), but defaults to having
   them enabled for symmetry with fossil(1).

   @see fsl_deck_R_calc2()
*/
FSL_EXPORT int fsl_deck_R_calc(fsl_deck * d);

/**
   A variant of fsl_deck_R_calc() which calculates the given deck's
   R-card but does not assign it to the deck, instead returning it
   via the 2nd argument:

   If *tgt is not NULL when this function is called, it is required to
   point to at least FSL_STRLEN_MD5+1 bytes of memory to which the
   NUL-terminated R-card hash will be written. If *tgt is NULL then
   this function assigns (on success) *tgt to a dynamically-allocated
   R-card hash and transfers ownership of it to the caller (who must
   eventually fsl_free() it). On error, *tgt is not modified.

   Results are undefined if either argument is NULL.
   
   Returns 0 on success. See fsl_deck_R_calc() for information about
   possible errors, with the addition that FSL_RC_OOM is returned
   if *tgt is NULL and allocating a new *tgt value fails.

   Calculating the R-card necessarily requires that d's F-card list be
   sorted, which this routine does if it seems necessary. The
   calculation also necessarily mutates the deck's F-card-traversal
   cursor, which requires loading the deck's B-card, if it has
   one. Aside from the F-card sorting, and potentially B-card, and the
   cursor resets, this routine does not modify the deck. On success,
   the deck's F-card iteration cursor (and that of d->B, if it's
   loaded) is rewound.
*/
FSL_EXPORT int fsl_deck_R_calc2(fsl_deck *d, char ** tgt);

/**
   Semantically identical fsl_deck_B_set() but sets the U-card.
   userName must be the user who's name should be recorded for
   this change.
*/
FSL_EXPORT int fsl_deck_U_set( fsl_deck * mf, char const *userName);

/**
   Semantically identical fsl_deck_B_set() but sets the W-card.
   content must be the page content of the Wiki page or Event this
   change is being applied to.
*/
FSL_EXPORT int fsl_deck_W_set( fsl_deck * mf, char const *content, fsl_int_t len);

/**
   Must be called to initialize a newly-created/allocated deck
   instance. This function clears out all contents of the d
   parameter except for its (f, type, allocStamp) members, sets its
   (f, type) members, and leaves d->allocStamp intact.
*/
FSL_EXPORT void fsl_deck_init( fsl_cx * cx, fsl_deck * d, fsl_satype_e type );

/**
   Returns true if d contains data for all _required_ cards, as
   determined by the value of d->type, else returns false. It returns
   false if d->type==FSL_SATYPE_ANY, as that is a placeholder value
   intended to be re-set by the deck's user.

   If it returns false, d->f's error state will help a description of
   the problem.

   The library calls this as needed, but clients may, if they want
   to. Note, however, that for FSL_SATYPE_CHECKIN decks it may fail
   if the deck has not been fsl_deck_unshuffle()d yet because the
   R-card gets calculated there (if needed).

   As a special case, d->f is configured to calculate R-cards,
   d->type==FSL_SATYPE_CHECKIN, AND d->R is not set, this will fail
   (with a descriptive error message).

   Another special case: for FSL_SATYPE_CHECKIN decks, if no
   F-cards are in th deck then an R-card is required to avoid a
   potental (admittedly harmless) syntactic ambiguity with
   FSL_SATYPE_CONTROL artifacts. The only legal R-card for a
   checkin with no F-cards has the initial MD5 hash state value
   (defined in the constant FSL_MD5_INITIAL_HASH), and that
   precondition is checked in this routine. fsl_deck_unshuffle()
   recognizes this case and adds the initial-state R-card, so
   clients normally need not concern themselves with this. If d has
   F-cards, whether or not an R-card is required depends on
   whether d->f is configured to require them or not.

   Enough about the R-card. In all other cases not described above,
   R-cards are not required (and they are only ever required on
   FSL_SATYPE_CHECKIN manifests).

   Though fossil(1) does not technically require F-cards in
   FSL_SATYPE_CHECKIN decks, so far none of the Fossil developers
   have found a use for a checkin without F-cards except the
   initial empty checkin. Additionally, a checkin without F-cards
   is potentially syntactically ambiguous (it could be an EVENT or
   ATTACHMENT artifact if it has no F- or R-card). So... this
   library _normally_ requires that CHECKIN decks have at least one
   F-card. This function, however, does not consider F-cards to be
   strictly required.
*/
FSL_EXPORT bool fsl_deck_has_required_cards( fsl_deck const * d );

/**
   Prepares the given deck for output by ensuring that cards
   which need to be sorted are sorted, and it may run some
   last-minute validation checks.

   The cards which get sorted are: F, J, M, Q, T. The P-card list is
   _not_ sorted - the client is responsible for ensuring that the
   primary parent is added to that list first, and after that the
   ordering is largely irrelevant. It is not possible for the library
   to determine a proper order for P-cards, nor to validate that order
   at input-time.

   If calculateRCard is true and fsl_card_is_legal(d,'R') then this
   function calculates the R-card for the deck. The R-card
   calculation is _extremely_ memory-hungry but adds another level
   of integrity checking to Fossil. If d->type is not
   FSL_SATYPE_MANIFEST then calculateRCard is ignored.

   If calculateRCard is true but no F-cards are present AND d->type is
   FSL_SATYPE_CHECKIN then the R-card is set to the initial MD5 hash
   state (the only legal R-card value for an empty F-card list). (This
   is necessary in order to prevent a deck-type ambiguity in one
   corner case.)

   The R-card, if used, must be calculated before
   fsl_deck_output()ing a deck containing F-cards. Clients may
   alternately call fsl_deck_R_calc() to calculate the R card
   separately, but there is little reason to do so. There are rare
   cases where the client can call fsl_deck_R_set()
   legally. Historically speaking the R-card was required when
   F-cards were used, but it was eventually made optional because
   (A) the memory cost and (B) it's part of a 3rd or 4th level of
   integrity-related checks, and is somewhat superfluous.

   @see fsl_deck_output()
   @see fsl_deck_save()
*/
FSL_EXPORT int fsl_deck_unshuffle( fsl_deck * d, bool calculateRCard );

/**
   Renders the given control artifact's contents to the given output
   function and calculates any cards which cannot be calculated until
   the contents are complete (namely the R-card and Z-card).

   The given deck is "logically const" but traversal over F-cards and
   baselines requires non-const operations. To keep this routine from
   requiring an undue amount of pre-call effort on the client's part,
   it also takes care of calling fsl_deck_unshuffle() to ensure that
   all of the deck's cards are in order. (If the deck has no R card,
   but has F-cards, and d->f is configured to generate R-cards, then
   unshuffling will also calculate the R-card.)

   Returns 0 on success, FSL_RC_MISUSE if !d or !d->f or !out. If
   out() returns non-0, output stops and that code is
   returned. outputState is passed as the first argument to out() and
   out() may be called an arbitrary number of times by this routine.

   Returns FSL_RC_SYNTAX if fsl_deck_has_required_cards()
   returns false.

   On errors more serious than argument validation, the deck's
   context's (d->f) error state is updated.

   The exact structure of the ouput depends on the value of
   mf->type, and FSL_RC_TYPE is returned if this function cannot
   figure out what to do with the given deck's type.

   @see fsl_deck_unshuffle()
   @see fsl_deck_save()
*/
FSL_EXPORT int fsl_deck_output( fsl_deck * d, fsl_output_f out,
                                void * outputState );


/**
   Saves the given deck into f's repository database as new control
   artifact content. If isPrivate is true then the content is
   marked as private, otherwise it is not. Note that isPrivate is a
   suggestion and might be trumped by existing state within f or
   its repository, and such a trumping is not treated as an
   error. e.g. tags are automatically private when they tag private
   content.

   Before saving, the deck is passed through fsl_deck_unshuffle()
   and fsl_deck_output(), which will fail for a variety of
   easy-to-make errors such as the deck missing required cards.
   For unshuffle purposes, the R-card gets calculated if the deck
   has any F-cards AND if the caller has not already set/calculated
   it AND if f's FSL_CX_F_CALC_R_CARD flag is set (it is on by
   default for historical reasons, but this may change at some
   point).

   Returns 0 on success, the usual non-0 suspects on error.

   If d->rid and d->uuid are set when this is called, it is assumed
   that we are saving existing or phantom content, and in that
   case:

   - An existing phantom is populated with the new content.

   - If an existing record is found with a non-0 size then it is
   not modified but this is currently not treated as an error (for
   historical reasons, though one could argue that it should result
   in FSL_RC_ALREADY_EXISTS).

   If d->rid and d->uuid are not set when this is called then... on
   success, d->rid and d->uuid will contain the values held by
   their counterparts in the blob table. They will only be set on
   success because they would otherwise refer to db records which
   get destroyed when the transaction rolls back.

   After saving, the deck will be cross-linked to update any
   relationships described by the deck.

   The save operation happens within a transaction, of course, and
   on any sort of error, db-side changes are rolled back. Note that
   it _is_ legal to start the transaction before calling this,
   which effectively makes this operation part of that transaction.

   This function will fail with FSL_RC_ACCESS if d is a delta manifest
   (has a B-card) d->f's forbid-delta-manifests configuration option
   is set to a truthy value. See fsl_repo_forbids_delta_manifests().

   Maintenance reminder: this function also does a small bit of
   artifact-type-specific processing.

   @see fsl_deck_output()
   @see fsl_content_put_ex()
*/
FSL_EXPORT int fsl_deck_save( fsl_deck * d, bool isPrivate );

/**
    This starts a transaction (possibly nested) on the repository db
    and initializes some temporary db state needed for the
    crosslinking certain artifact types. It "should" (see below) be
    called at the start of the crosslinking process. Crosslinking
    *can* work without this but certain steps for certain (subject to
    change) artifact types will be skipped, possibly leading to
    unexpected timeline data or similar funkiness. No permanent
    SCM-relevant state will be missing, but the timeline might not be
    updated and tickets might not be fully processed. This should be
    used before crosslinking any artifact types, but will only have
    significant side effects for certain (subject to change) types.

    Returns 0 on success.

    If this function succeeds, the caller is OBLIGATED to either call
    fsl_crosslink_end() or fsl_db_transaction_rollback(), depending
    on whether the work done after this call succeeds
    resp. fails. This process may install temporary tables and/or
    triggers, so failing to call one or the other of those will result
    in misbehavior.

    @see fsl_deck_crosslink()
*/
int fsl_crosslink_begin(fsl_cx * f);

/**
    Must not be called unless fsl_crosslink_begin() has
    succeeded.  This performs crosslink post-processing on certain
    artifact types and cleans up any temporary db state initialized by
    fsl_crosslink_begin().

    Returns 0 on success. On error it initiates (or propagates) a
    rollback for the current transaction.
*/
int fsl_crosslink_end(fsl_cx * f);

/**
   Parses src as Control Artifact content and populates d with it.

   d will be cleaned up before parsing if it has any contents,
   retaining its d->f member (which must be non-NULL for
   error-reporting purposes).

   This function _might_ take over the contents of the source
   buffer on success or it _might_ leave it for the caller to clean
   up or re-use, as he sees fit. If the caller does not intend to
   re-use the buffer, he should simply pass it to
   fsl_buffer_clear() after calling this (no need to check if it
   has contents or not first).

   When taking over the contents then on success, after returning
   src->mem will be NULL, and all other members will be reset to
   their default state. This function only takes over the contents
   if it decides to implement certain memory optimizations.

   Ownership of src itself is never changed by this function, only
   (possibly!) the ownership of its contents.

   In any case, the content of the source buffer is modified by
   this function because (A) that simplifies tokenization greatly,
   (B) saves us having to make another copy to work on, (C) the
   original implementation did it that way, (D) because in
   historical use the source is normally thrown away after parsing,
   anyway, and (E) in combination with taking ownership of src's
   contents it allows us to optimize away some memory allocations
   by re-using the internal memory of the buffer. This function
   never changes src's size, but it mutilates its contents
   (injecting NUL bytes as token delimiters).

   If d->type is _not_ FSL_SATYPE_ANY when this is called, then
   this function requires that the input to be of that type. We can
   fail relatively quickly in that case, and this can be used to
   save some downstream code some work. Note that the initial type
   for decks created using fsl_deck_malloc() or copy-initialized
   from ::fsl_deck_empty is FSL_SATYPE_ANY, so normally clients do
   not need to set this (unless they want to, as a small
   optimization).

   On success it returns 0 and d will be updated with the state
   from the input artifact. (Ideally, outputing d via
   fsl_deck_output() will produce a lossless copy of the original.)
   d->uuid will be set to the SHA1 of the input artifact, ignoring
   any surrounding PGP signature for hashing purposes.

   If d->f has an opened repository db and the parsed artifact has a
   counterpart in the database (determined via a hash match) then
   d->rid is set to the record ID.

   On error, if there is error information to propagate beyond the
   result code then it is stored in d->f (if that is not NULL),
   else in d->error. Whether or not such error info is propagated
   depends on the type of error, but anything more trivial than
   invalid arguments will be noted there.

   d might be partially populated on error, so regardless of success
   or failure, the client must eventually pass d to
   fsl_deck_finalize() to free its memory.

   Error result codes include:

   - FSL_RC_MISUSE if any pointer argument is NULL.

   - FSL_RC_SYNTAX on syntax errors.

   - FSL_RC_CONSISTENCY if validation of a Z-card fails.

   - Any number of errors coming from the allocator, database, or
   fsl_deck APIs used here.
*/
FSL_EXPORT int fsl_deck_parse(fsl_deck * d, fsl_buffer * src);

/**
   Quickly determines whether the content held by the given buffer
   "might" be a structural artifact. It performs a fast sanity check
   for prominent features which can be checked either in O(1) or very
   short O(N) time (with a fixed N). If it returns false then the
   given buffer's contents are, with 100% certainty, *not* a
   structural artifact. If it returns true then they *might* be, but
   being 100% certain requires passing the contents to
   fsl_deck_parse() to fully parse them.
*/
FSL_EXPORT bool fsl_might_be_artifact(fsl_buffer const * src);

/**
   Loads the content from given rid and tries to parse it as a
   Fossil artifact. If rid==0 the current checkout (if opened) is
   used. (Trivia: there can never be a checkout with rid==0 but
   rid==0 is sometimes valid for an new/empty repo devoid of
   commits). If type==FSL_SATYPE_ANY then it will allow any type of
   control artifact, else it returns FSL_RC_TYPE if the loaded
   artifact is of the wrong type.

   Returns 0 on success.

   d may be partially populated on error, and the caller must
   eventually pass it to fsl_deck_finalize() resp. fsl_deck_clean()
   regardless of success or error. This function "could" clean it
   up on error, but leaving it partially populated makes debugging
   easier. If the error was an artifact type mismatch then d will
   "probably" be properly populated but will not hold the type of
   artifact requested. It "should" otherwise be well-formed because
   parsing errors occur before the type check can happen, but
   parsing of invalid manifests might also trigger a FSL_RC_TYPE
   error of a different nature. The morale of the storage is: if
   this function returns non-0, assume d is useless and needs to be
   cleaned up.

   f's error state may be updated on error (for anything more
   serious than basic argument validation errors).

   On success d->f is set to f.

   @see fsl_deck_load_sym()
*/
FSL_EXPORT int fsl_deck_load_rid( fsl_cx * f, fsl_deck * d,
                                  fsl_id_t rid, fsl_satype_e type );

/**
   A convenience form of fsl_deck_load_rid() which uses
   fsl_sym_to_rid() to convert symbolicName into an artifact RID.  See
   fsl_deck_load_rid() for the symantics of the first, second, and
   fourth arguments, as well as the return value. See fsl_sym_to_rid()
   for the allowable values of symbolicName.

   @see fsl_deck_load_rid()
*/
FSL_EXPORT int fsl_deck_load_sym( fsl_cx * f, fsl_deck * d,
                                  char const * symbolicName,
                                  fsl_satype_e type );

/**
   Loads the baseline manifest specified in d->B.uuid, if any and if
   necessary. Returns 0 on success. If d->B.baseline is already loaded
   or d->B.uuid is NULL (in which case there is no baseline), it
   returns 0 and has no side effects.

   Neither argument may be NULL and d must be a fully-populated
   object, complete with a proper d->rid, before calling this.

   On success 0 is returned. If d->B.baseline is NULL then
   it means that d has no baseline manifest (and d->B.uuid will be NULL
   in that case). If d->B.baseline is not NULL then it is owned by
   d and will be cleaned up when d is cleaned/finalized.

   Error codes include, but are not limited to:

   - FSL_RC_MISUSE if !d->f.

   - FSL_RC_NOT_A_REPO if d->f has no opened repo db.

   - FSL_RC_RANGE if d->rid<=0, but that code might propagate up from
   a lower-level call as well.

   On non-trivial errors d->f's error state will be updated to hold
   a description of the problem.

   Some misuses trigger assertions in debug builds.
*/
FSL_EXPORT int fsl_deck_baseline_fetch( fsl_deck * d );

/**
   A callback interface for manifest crosslinking, so that we can farm
   out the updating of the event table. Each callback registered via
   fsl_xlink_listener() will be called at the end of the so-called
   crosslinking process, which is run every time a control artifact is
   processed for d->f's repository database, passed the deck being
   crosslinked and the client-provided state which was registered with
   fsl_xlink_listener(). Note that the deck object itself holds other
   state useful for crosslinking, like the blob.rid value of the deck
   and its fsl_cx instance.

   If an implementation is only interested in a specific type of
   artifact, it must check d->type and return 0 if it's an
   "uninteresting" type.

   Implementations must return 0 on success or some other fsl_rc_e
   value on error. Returning non-0 causes the database transaction
   for the crosslinking operation to roll back, effectively
   cancelling whatever pending operation triggered the
   crosslink. If any callback fails, processing stops immediately -
   no other callbacks are executed.

   Implementations which want to report more info than an integer
   should call fsl_cx_err_set() to set d->f's error state, as that
   will be propagated up to the code which initiated the failed
   crosslink.

   ACHTUNG and WARNING: the fsl_deck parameter "really should" be
   const, but certain operations on a deck are necessarily non-const
   operations. That includes, but may not be limited to:

   - Iterating over F-cards, which requires calling
     fsl_deck_F_rewind() before doing so.

   - Loading a checkin's baseline (required for F-card iteration and
   performed automatically by fsl_deck_F_rewind()).

   Aside from such iteration-related mutable state, it is STRICTLY
   ILLEGAL to modify a deck's artifact-related state while it is
   undergoing crosslinking. It is legal to modify its error state.


   Potential TODO: add some client-opaque state to decks so that they
   can be flagged as "being crosslinked" and fail mutation operations
   such as card adders/setters.

   @see fsl_xlink_listener()
*/
typedef int (*fsl_deck_xlink_f)(fsl_deck * d, void * state);

/**
    A type for holding a callback/state pair for manifest
    crosslinking callbacks.
*/
struct fsl_xlinker {
  char const * name;
  /** Callback function. */
  fsl_deck_xlink_f f;
  /** State for this->f's last argument. */
  void * state;
};
typedef struct fsl_xlinker fsl_xlinker;

/** Empty-initialized fsl_xlinker struct, intended for const-copy
    intialization. */
#define fsl_xlinker_empty_m {NULL,NULL,NULL}

/** Empty-initialized fsl_xlinker struct, intended for copy intialization. */
extern const fsl_xlinker fsl_xlinker_empty;

/**
    A list of fsl_xlinker instances.
*/
struct fsl_xlinker_list {
  /** Number of used items in this->list. */
  fsl_size_t used;
  /** Number of slots allocated in this->list. */
  fsl_size_t capacity;
  /** Array of this->used elements. */
  fsl_xlinker * list;
};
typedef struct fsl_xlinker_list fsl_xlinker_list;

/** Empty-initializes fsl_xlinker_list struct, intended for
    const-copy intialization. */
#define fsl_xlinker_list_empty_m {0,0,NULL}

/** Empty-initializes fsl_xlinker_list struct, intended for copy intialization. */
extern const fsl_xlinker_list fsl_xlinker_list_empty;

/**
    Searches f's crosslink callbacks for an entry with the given
    name and returns that entry, or NULL if no match is found.  The
    returned object is owned by f.
*/
fsl_xlinker * fsl_xlinker_by_name( fsl_cx * f, char const * name );

/**
   Adds the given function as a "crosslink callback" for the given
   Fossil context. The callback is called at the end of a
   successfull fsl_deck_crosslink() operation and provides a way
   for the client to perform their own work based on the app having
   crosslinked an artifact. Crosslinking happens when artifacts are
   saved or upon a rebuild operation.

   Crosslink callbacks are called at the end of the core crosslink
   steps, in the order they are registered, with the caveat that if a
   listener is overwritten by another with the same name, the new
   entry retains the older one's position in the list. The library may
   register its own before the client gets a chance to.

   If _any_ crosslinking callback fails (returns non-0) then the
   _whole_ crosslinking fails and is rolled back (which may very
   well include pending tags/commits/whatever getting rolled back).

   The state parameter has no meaning for this function, but is
   passed on as the final argument to cb(). If not NULL, cbState
   "may" be required to outlive f, depending on cbState's exact
   client-side internal semantics/use, as there is currently no API
   to remove registered crosslink listeners.

   The name must be non-NULL/empty. If a listener is registered with a
   duplicate name then the first one is replaced. This function does
   not copy the name bytes - they are assumed to be static or
   otherwise to live at least as long as f. The name may be
   arbitrarily long, but must have a terminating NUL byte. It is
   recommended that clients choose a namespace/prefix to apply to the
   names they register. The library reserves the prefix "fsl/" for
   its own use, and will happily overwrite client-registered entries
   with the same names. The name string need not be stable across
   application sessions and maybe be a randomly-generated string.

   Caveat: some obscure artifact crosslinking steps do not happen
   unless crosslinking takes place in the context of a
   fsl_crosslink_begin() and fsl_crosslink_end()
   session. Thus, at the time client-side crosslinker callbacks are
   called, certain crosslinking state in the database may still be
   pending. It is as yet unclear how best to resolve that minor
   discrepancy, or whether it even needs resolving.


   Default (overrideable) crosslink handlers:

   The library internally splits crosslinking of artifacts into two
   parts: the main one (which clients cannot modify) handles the
   database-level linking of relational state implied by a given
   artifact. The secondary one adds an entry to the "event" table,
   which is where Fossil's timeline lives. The crosslinkers for the
   timeline updates may be overridden by clients by registering
   a crosslink listener with the following names:

   - Attachment artifacts: "fsl/attachment/timeline"

   - Checkin artifacts: "fsl/checkin/timeline"

   - Control artifacts: "fsl/control/timeline"

   - Forum post artifacts: "fsl/forumpost/timeline"

   - Technote artifacts: "fsl/technote/timeline"

   - Wiki artifacts: "fsl/wiki/timeline"

   A context registers listeners under those names when it
   initializes, and clients may override them at any point after that.

   Caveat: updating the timeline requires a bit of knowledge about the
   Fossil DB schema and/or conventions. Updates for certain types,
   e.g. attachment/control/forum post, is somewhat more involved and
   updating the timeline for wiki comments requires observing a "quirk
   of conventions" for labeling such comments, such that they will
   appear properly when the main fossil app renders them. That said,
   the only tricky parts of those updates involve generating the
   "correct" comment text. So long as the non-comment parts are
   updated properly (that part is easy to do), fossil can function
   with it.  The timeline comment text/links are soley for human
   consumption. Fossil makes much use of the "event" table internally,
   however, so the rest of that table must be properly populated.

   Because of that caveat, clients may, rather than overriding the
   defaults, install their own crosslink listners which ammend the
   state applied by the default ones. e.g. add a listener which
   watches for checkin updates and replace the default-installed
   comment with one suitable for your application, leaving the rest of
   the db state in place. At its simplest, that looks more or less
   like the following code (inside a fsl_deck_xlink_f() callback):

   @code
   int rc = fsl_db_exec(fsl_cx_db_repo(deck->f),
                        "UPDATE event SET comment=%Q "
                        "WHERE objid=%"FSL_ID_T_PFMT,
                        "the new comment.", deck->rid);
   @endcode
*/
FSL_EXPORT int fsl_xlink_listener( fsl_cx * f, char const * name,
                                   fsl_deck_xlink_f cb, void * cbState );


/**
   For the given blob.rid value, returns the blob.size value of
   that record via *rv. Returns 0 or higher on success, -1 if a
   phantom record is found, -2 if no entry is found, or a smaller
   negative value on error (dig around the sources to decode them -
   this is not expected to fail unless the system is undergoing a
   catastrophe).

   @see fsl_content_blob()
   @see fsl_content_get()
*/
FSL_EXPORT fsl_int_t fsl_content_size( fsl_cx * f, fsl_id_t blobRid );

/**
   For the given blob.rid value, fetches the content field of that
   record and overwrites tgt's contents with it (reusing tgt's
   memory if it has any and if it can). The blob's contents are
   uncompressed if they were stored in compressed form. This
   extracts a raw blob and does not apply any deltas - use
   fsl_content_get() to fully expand a delta-stored blob.

   Returns 0 on success. On error tgt might be partially updated,
   e.g. it might be populated with compressed data instead of
   uncompressed. On error tgt's contents should be recycled
   (e.g. fsl_buffer_reuse()) or discarded (e.g. fsl_buffer_clear())
   by the client.

   @see fsl_content_get()
   @see fsl_content_size()
*/
FSL_EXPORT int fsl_content_blob( fsl_cx * f, fsl_id_t blobRid, fsl_buffer * tgt );

/**
   Functionally similar to fsl_content_blob() but does a lot of
   work to ensure that the returned blob is expanded from its
   deltas, if any. The tgt buffer's memory, if any, will be
   replaced/reused if it has any.

   Returns 0 on success. There are no less than 50 potental
   different errors, so we won't bother to list them all. On error
   tgt might be partially populated. The basic error cases are:

   - FSL_RC_MISUSE if !tgt or !f.

   - FSL_RC_RANGE if rid<=0 or if an infinite loop is discovered in
   the repo delta table links (that is a consistency check to avoid
   an infinite loop - that condition "cannot happen" because the
   verify-before-commit logic catches that error case).

   - FSL_RC_NOT_A_REPO if f has no repo db opened.

   - FSL_RC_NOT_FOUND if the given rid is not in the repo db.

   - FSL_RC_OOM if an allocation fails.


   @see fsl_content_blob()
   @see fsl_content_size()
*/
FSL_EXPORT int fsl_content_get( fsl_cx * f, fsl_id_t blobRid, fsl_buffer * tgt );

/**
   Uses fsl_sym_to_rid() to convert sym to a record ID, then
   passes that to fsl_content_get(). Returns 0 on success.
*/
FSL_EXPORT int fsl_content_get_sym( fsl_cx * f, char const * sym, fsl_buffer * tgt );

/**
   Returns true if the given rid is marked as PRIVATE in f's current
   repository. Returns false (0) on error or if the content is not
   marked as private.
*/
FSL_EXPORT bool fsl_content_is_private(fsl_cx * f, fsl_id_t rid);

/**
   Marks the given rid public, if it was previously marked as
   private. Returns 0 on success, non-0 on error.

   Note that it is not possible to make public content private.
*/
FSL_EXPORT int fsl_content_make_public(fsl_cx * f, fsl_id_t rid);

/**
   Generic callback interface for visiting decks. The interface
   does not generically require that d survive after this call
   returns.

   Implementations must return 0 on success, non-0 on error. Some
   APIs using this interface may specify that FSL_RC_BREAK can be
   used to stop iteration over a loop without signaling an error.
   In such cases the APIs will translate FSL_RC_BREAK to 0 for
   result purposes, but will stop looping over whatever it is they
   are looping over.
*/
typedef int (*fsl_deck_visitor_f)( fsl_cx * f, fsl_deck const * d,
                                   void * state );

/**
   For each unique wiki page name in f's repostory, this calls
   cb(), passing it the manifest of the most recent version of that
   page. The callback should return 0 on success, FSL_RC_BREAK to
   stop looping without an error, or any other non-0 code
   (preferably a value from fsl_rc_e) on error.

   The 3rd parameter has no meaning for this function but it is
   passed on as-is to the callback.

   ACHTUNG: the deck passed to the callback is transient and will
   be cleaned up after the callback has returned, so the callback
   must not hold a pointer to it or its contents.

   @see fsl_wiki_load_latest()
   @see fsl_wiki_latest_rid()
   @see fsl_wiki_names_get()
   @see fsl_wiki_page_exists()
*/
FSL_EXPORT int fsl_wiki_foreach_page( fsl_cx * f, fsl_deck_visitor_f cb, void * state );

/**
   Fetches the most recent RID for the given wiki page name and
   assigns *newId (if it is not NULL) to that value. Returns 0 on
   success, FSL_RC_MISUSE if !f or !pageName, FSL_RC_RANGE if
   !*pageName, and a host of other potential db-side errors
   indicating more serious problems. If no such page is found,
   newRid is not modified and this function returns 0 (as opposed
   to FSL_RC_NOT_FOUND) because that simplifies usage (so far).

   On error *newRid is not modified.

   @see fsl_wiki_load_latest()
   @see fsl_wiki_foreach_page()
   @see fsl_wiki_names_get()
   @see fsl_wiki_page_exists()
*/
FSL_EXPORT int fsl_wiki_latest_rid( fsl_cx * f, char const * pageName, fsl_id_t * newRid );

/**
   Loads the artifact for the most recent version of the given wiki page,
   populating d with its contents.

   Returns 0 on success. On error d might be partially populated,
   so it needs to be passed to fsl_deck_finalize() regardless of
   whether this function succeeds or fails.

   Returns FSL_RC_NOT_FOUND if no page with that name is found.

   @see fsl_wiki_latest_rid()
   @see fsl_wiki_names_get()
   @see fsl_wiki_page_exists()
*/
FSL_EXPORT int fsl_wiki_load_latest( fsl_cx * f, char const * pageName, fsl_deck * d );

/**
   Returns true (non-0) if f's repo database contains a page with the
   given name, else false.

   @see fsl_wiki_load_latest()
   @see fsl_wiki_latest_rid()
   @see fsl_wiki_names_get()
   @see fsl_wiki_names_get()
*/
FSL_EXPORT bool fsl_wiki_page_exists(fsl_cx * f, char const * pageName);

/**
   A helper type for use with fsl_wiki_save(), intended primarily
   to help client-side code readability somewhat.
*/
enum fsl_wiki_save_mode_t {
/**
   Indicates that fsl_wiki_save() must only allow the creation of
   a new page, and must fail if such an entry already exists.
*/
FSL_WIKI_SAVE_MODE_CREATE = -1,
/**
   Indicates that fsl_wiki_save() must only allow the update of an
   existing page, and will not create a branch new page.
*/
FSL_WIKI_SAVE_MODE_UPDATE = 0,
/**
   Indicates that fsl_wiki_save() must allow both the update and
   creation of pages. Trivia: "upsert" is a common SQL slang
   abbreviation for "update or insert."
*/
FSL_WIKI_SAVE_MODE_UPSERT = 1
};

typedef enum fsl_wiki_save_mode_t fsl_wiki_save_mode_t;

/**
   Saves wiki content to f's repository db.

   pageName is the name of the page to update or create.

   b contains the content for the page.

   userName specifies the user name to apply to the change. If NULL
   or empty then fsl_cx_user_get() or fsl_guess_user_name() are
   used (in that order) to determine the name.

   mimeType specifies the mime type for the content (may be NULL).
   Mime type names supported directly by fossil(1) include (as of
   this writing): text/x-fossil-wiki, text/x-markdown,
   text/plain

   Whether or not this function is allowed to create a new page is
   determined by creationPolicy. If it is
   FSL_WIKI_SAVE_MODE_UPDATE, this function will fail with
   FSL_RC_NOT_FOUND if no page with the given name already exists.
   If it is FSL_WIKI_SAVE_MODE_CREATE and a previous version _does_
   exist, it fails with FSL_RC_ALREADY_EXISTS. If it is
   FSL_WIKI_SAVE_MODE_UPSERT then both the save-exiting and
   create-new cases are allowed. In summary:

   - use FSL_WIKI_SAVE_MODE_UPDATE to allow updates to existing pages
   but disallow creation of new pages,

   - use FSL_WIKI_SAVE_MODE_CREATE to allow creating of new pages
   but not of updating an existing page.

   - FSL_WIKI_SAVE_MODE_UPSERT allows both updating and creating
   a new page on demand.

   Returns 0 on success, or any number fsl_rc_e codes on error. On
   error no content changes are saved, and any transaction is
   rolled back or a rollback is scheduled if this function is
   called while a transaction is active.


   Potential TODO: add an optional (fsl_id_t*) output parameter
   which gets set to the new record's RID.

   @see fsl_wiki_page_exists()
   @see fsl_wiki_names_get()
*/
FSL_EXPORT int fsl_wiki_save(fsl_cx * f, char const * pageName,
                  fsl_buffer const * b, char const * userName,
                  char const * mimeType, fsl_wiki_save_mode_t creationPolicy );

/**
   Fetches the list of all wiki page names in f's current repo db
   and appends them as new (char *) strings to tgt. On error tgt
   might be partially populated (but this will only happen on an
   OOM or serious system-level error).

   It is up to the caller free the entries added to the list. Some
   of the possibilities include:

   @code
   fsl_list_visit( list, 0, fsl_list_v_fsl_free, NULL );
   fsl_list_reserve(list,0);
   // Or:
   fsl_list_clear(list, fsl_list_v_fsl_free, NULL);
   // Or simply:
   fsl_list_visit_free( list, 1 );
   @endcode

*/
FSL_EXPORT int fsl_wiki_names_get( fsl_cx * f, fsl_list * tgt );

/**
   F-cards each represent one file entry in a Manifest Artifact (i.e.,
   a checkin version).

   All of the non-const pointers in this class are owned by the
   respective instance of the class OR by the fsl_deck which created
   it, and must neither be modified nor freed except via the
   appropriate APIs.
*/
struct fsl_card_F {
  /**
     UUID of the underlying blob record for the file. NULL for
     removed entries.
  */
  fsl_uuid_str uuid;
  /**
     Name of the file.
  */
  char * name;
  /**
     Previous name if the file was renamed, else NULL.
  */
  char * priorName;
  /**
     File permissions. Fossil only supports one "permission" per
     file, and it does not necessarily map to a real
     filesystem-level permission.

     @see fsl_fileperm_e
  */
  fsl_fileperm_e perm;

  /**
     An internal optimization. Do not mess with this.  When this is
     true, the various string members of this struct are not owned
     by this struct, but by the deck which created this struct. This
     is used when loading decks from storage - the strings are
     pointed to the original content data, rather than strdup()'d
     copies of it. fsl_card_F_clean() will DTRT and delete the
     strings (or not).
  */
  bool deckOwnsStrings;
};
/**
   Empty-initialized fsl_card_F structure, intended for use in
   initialization when embedding fsl_card_F in another struct or
   copy-initializing a const struct.
*/
#define fsl_card_F_empty_m {   \
  NULL/*uuid*/,                \
  NULL/*name*/,                \
  NULL/*priorName*/,           \
  0/*perm*/,                   \
  false/*deckOwnsStrings*/     \
}
FSL_EXPORT const fsl_card_F fsl_card_F_empty;

/**
   Represents a J card in a Ticket Control Artifact.
*/
struct fsl_card_J {
  /**
     If true, the new value should be appended to any existing one
     with the same key, else it will replace any old one.
  */
  char append;
  /**
     For internal use only.
  */
  unsigned char flags;
  /**
     The ticket field to update. The bytes are owned by this object.
  */
  char * field;
  /**
     The value for the field. The bytes are owned by this object.
  */
  char * value;
};
/** Empty-initialized fsl_card_J struct. */
#define fsl_card_J_empty_m {0,0,NULL, NULL}
/** Empty-initialized fsl_card_J struct. */
FSL_EXPORT const fsl_card_J fsl_card_J_empty;

/**
   Represents a tag in a Manifest or Control Artifact.
*/
struct fsl_card_T {
  /**
     The type of tag.
  */
  fsl_tagtype_e type;
  /**
     UUID of the artifact this tag is tagging. When applying a tag to
     a new checkin, this value is left empty (=NULL) and gets replaced
     by a '*' in the resulting control artifact.
  */
  fsl_uuid_str uuid;
  /**
     The tag's name. The bytes are owned by this object.
  */
  char * name;
  /**
     The tag's value. May be NULL/empty. The bytes are owned by
     this object.
  */
  char * value;
};
/** Defaults-initialized fsl_card_T instance. */
#define fsl_card_T_empty_m {FSL_TAGTYPE_INVALID, NULL, NULL,NULL}
/** Defaults-initialized fsl_card_T instance. */
FSL_EXPORT const fsl_card_T fsl_card_T_empty;

/**
   Types of cherrypick merges.
*/
enum fsl_cherrypick_type_e {
/** Sentinel value. */
FSL_CHERRYPICK_INVALID = 0,
/** Indicates a cherrypick merge. */
FSL_CHERRYPICK_ADD = 1,
/** Indicates a cherrypick backout. */
FSL_CHERRYPICK_BACKOUT = -1
};
typedef enum fsl_cherrypick_type_e fsl_cherrypick_type_e;

/**
   Represents a Q card in a Manifest or Control Artifact.
*/
struct fsl_card_Q {
  /** 0==invalid, negative==backed out, positive=cherrypicked. */
  fsl_cherrypick_type_e type;
  /**
     UUID of the target of the cherrypick. The bytes are owned by
     this object.
  */
  fsl_uuid_str target;
  /**
     UUID of the baseline for the cherrypick. The bytes are owned by
     this object.
  */
  fsl_uuid_str baseline;
};
/** Empty-initialized fsl_card_Q struct. */
#define fsl_card_Q_empty_m {FSL_CHERRYPICK_INVALID, NULL, NULL}
/** Empty-initialized fsl_card_Q struct. */
FSL_EXPORT const fsl_card_Q fsl_card_Q_empty;

/**
   Allocates a new J-card record instance

   On success it returns a new record which must eventually be
   passed to fsl_card_J_free() to free its resources. On
   error (invalid arguments or allocation error) it returns NULL.
   field may not be NULL or empty but value may be either.

   These records are immutable - the API provides no way to change
   them once they are instantiated.
*/
FSL_EXPORT fsl_card_J * fsl_card_J_malloc(bool isAppend,
                                          char const * field,
                                          char const * value);
/**
   Frees a J-card record created by fsl_card_J_malloc().
   Is a no-op if cp is NULL.
*/
FSL_EXPORT void fsl_card_J_free( fsl_card_J * cp );

/**
   Allocates a new fsl_card_T instance. If any of the pointer
   parameters are non-NULL, their values are assumed to be
   NUL-terminated strings, which this function copies.  Returns NULL
   on allocation error.  The returned value must eventually be passed
   to fsl_card_T_clean() or fsl_card_T_free() to free its resources.

   If uuid is not NULL and fsl_is_uuid(uuid) returns false then
   this function returns NULL. If it is NULL and gets assigned
   later, it must conform to fsl_is_uuid()'s rules or downstream
   results are undefined.

   @see fsl_card_T_free()
   @see fsl_card_T_clean()
   @see fsl_deck_T_add()
*/
FSL_EXPORT fsl_card_T * fsl_card_T_malloc(fsl_tagtype_e tagType,
                                          fsl_uuid_cstr uuid,
                                          char const * name,
                                          char const * value);
/**
   If t is not NULL, calls fsl_card_T_clean(t) and then passes t to
   fsl_free().

   @see fsl_card_T_clean()
*/
FSL_EXPORT void fsl_card_T_free(fsl_card_T *t);

/**
   Frees up any memory owned by t and clears out t's state,
   but does not free t.

   @see fsl_card_T_free()
*/
FSL_EXPORT void fsl_card_T_clean(fsl_card_T *t);

/**
   Allocates a new cherrypick record instance. The type argument must
   be one of FSL_CHERRYPICK_ADD or FSL_CHERRYPICK_BACKOUT.  target
   must be a valid UUID string. If baseline is not NULL then it also
   must be a valid UUID.

   On success it returns a new record which must eventually be
   passed to fsl_card_Q_free() to free its resources. On
   error (invalid arguments or allocation error) it returns NULL.

   These records are immutable - the API provides no way to change
   them once they are instantiated.
*/
FSL_EXPORT fsl_card_Q * fsl_card_Q_malloc(fsl_cherrypick_type_e type,
                                          fsl_uuid_cstr target,
                                          fsl_uuid_cstr baseline);
/**
   Frees a cherrypick record created by fsl_card_Q_malloc().
   Is a no-op if cp is NULL.
*/
FSL_EXPORT void fsl_card_Q_free( fsl_card_Q * cp );

/**
   Returns true (non-0) if f is not NULL and f has an opened repo
   which contains a checkin with the given rid, else it returns
   false.

   As a special case, if rid==0 then this only returns true
   if the repository currently has no content in the blob
   table.
*/
FSL_EXPORT char fsl_rid_is_a_checkin(fsl_cx * f, fsl_id_t rid);

/**
   Fetches the list of all directory names for a given checkin record
   id or (if rid is negative) the whole repo over all of its combined
   history. Each name entry in the list is appended to tgt. The
   results are reduced to unique names only and are sorted
   lexically. If addSlash is true then each entry will include a
   trailing slash character, else it will not. The list does not
   include an entry for the top-most directory.

   If rid is less than 0 then the directory list across _all_
   versions is returned. If it is 0 then the current checkout's RID
   is used (if a checkout is opened, otherwise a usage error is
   triggered). If it is positive then only directories for the
   given checkin RID are returned. If rid is specified, it is
   assumed to be the record ID of a commit (manifest) record, and
   it is impossible to distinguish between the results "invalid
   rid" and "empty directory list" (which is a legal result).

   On success it returns 0 and tgt will have a number of (char *)
   entries appended to it equal to the number of subdirectories in
   the repo (possibly 0).

   Returns non-0 on error, FSL_RC_MISUSE if !f, !tgt. On other
   errors error tgt might have been partially populated and the
   list contents should not be considered valid/complete.

   Ownership of the returned strings is transfered to the caller,
   who must eventually free each one using
   fsl_free(). fsl_list_visit_free() is the simplest way to free
   them all at once.
*/
FSL_EXPORT int fsl_repo_dir_names( fsl_cx * f, fsl_id_t rid,
                                   fsl_list * tgt, bool addSlash );


/**
   ZIPs up a copy of the contents of a specific version from f's
   opened repository db. sym is the symbolic name for the checkin
   to ZIP. filename is the name of the ZIP file to output the
   result to. See fsl_zip_writer for details and caveats of this
   library's ZIP creation. If vRootDir is not NULL and not empty
   then each file injected into the ZIP gets that directory
   prepended to its name.

   If progressVisitor is not NULL then it is called once just
   before each file is processed, passed the F-card for the file
   about to be zipped and the progressState parameter. If it
   returns non-0, ZIPping is cancelled and that error code is
   returned. This is intended primarily for providing feedback on
   the update process, but could also be used to cancel the
   operation between files.

   As of 2021-09-05 this routine automatically adds the files
   (manifest, manifest.uuid, manifest.tags) to the zip file,
   regardless of repository-level settings regarding those
   pseudo-files (see fsl_ckout_manifest_write()). As there are no
   F-cards associated with those non-files, the progressVisitor is not
   called for those.

   BUG: this function does not honor symlink content in a
   fossil-compatible fashion. If it encounters a symlink entry
   during ZIP generation, it will fail and f's error state will be
   updated with an explanation of this shortcoming.

   @see fsl_zip_writer
   @see fsl_card_F_visitor_f()
*/
FSL_EXPORT int fsl_repo_zip_sym_to_filename( fsl_cx * f, char const * sym,
                                  char const * vRootDir,
                                  char const * fileName,
                                  fsl_card_F_visitor_f progressVisitor,
                                  void * progressState);


/**
   Callback state for use with fsl_repo_extract_f() implementations
   to stream a given version of a repository's file's, one file at a
   time, to a client. Instances are never created by client code,
   only by fsl_repo_extract() and its delegates, which pass them to
   client-provided fsl_repo_extract_f() functions.
*/
struct fsl_repo_extract_state {
  /**
     The associated Fossil context.
  */
  fsl_cx * f;
  /**
     RID of the checkin version for this file. For a given call to
     fsl_repo_extract(), this number will be the same across all
     calls to the callback function.
  */
  fsl_id_t checkinRid;
  /**
     File-level blob.rid for fc. Can be used with, e.g.,
     fsl_mtime_of_manifest_file().
  */
  fsl_id_t fileRid;
  /**
     Client state passed to fsl_repo_extract(). Its interpretation
     is callback-implementation-dependent.
  */
  void * callbackState;
  /**
     The F-card being iterated over. This holds the repo-level
     metadata associated with the file, other than its RID, which is
     available via this->fileRid.

     Deleted files are NOT reported via the extraction process
     because reporting them accurately is trickier and more
     expensive than it could be. Thus this member's uuid field
     will always be non-NULL.

     Certain operations which use this class, e.g. fsl_repo_ckout()
     and fsl_ckout_update(), will temporarily synthesize an F-card to
     represent the state of a file update, in which case this object's
     contents might not 100% reflect any given db-side state. e.g.
     fsl_ckout_update() synthesizes an F-card which reflects the
     current state of a file after applying an update operation to it.
     In such cases, the fCard->uuid may refer to a repository-side
     file even though the hash of the on-disk file contents may differ
     because of, e.g., a merge.
  */
  fsl_card_F const * fCard;

  /**
     If the fsl_repo_extract_opt object which was used to initiate the
     current extraction has the extractContent member set to false,
     this will be a NULL pointer. If it's true, this member points to
     a transient buffer which holds the full, undelta'd/uncompressed
     content of fc's file record. The content bytes are owned by
     fsl_repo_extract() and are invalidated as soon as this callback
     returns, so the callback must copy/consume them immediately if
     needed.
  */
  fsl_buffer const * content;

  /**
     These counters can be used by an extraction callback to calculate
     a progress percentage.
  */
  struct {
    /** The current file number, starting at 1. */
    uint32_t fileNumber;
    /** Total number of files to extract. */
    uint32_t fileCount;
  } count;
};
typedef struct fsl_repo_extract_state fsl_repo_extract_state;

/**
   Initialized-with-defaults fsl_repo_extract_state instance, intended
   for const-copy initialization.
*/
#define fsl_repo_extract_state_empty_m {\
  NULL/*f*/, 0/*checkinRid*/, 0/*fileRid*/, \
  NULL/*state*/, NULL/*fCard*/, NULL/*content*/,    \
  {/*count*/0,0} \
}
/**
   Initialized-with-defaults fsl_repo_extract_state instance,
   intended for non-const copy initialization.
*/
FSL_EXPORT const fsl_repo_extract_state fsl_repo_extract_state_empty;

/**
   A callback type for use with fsl_repo_extract(). See
   fsl_repo_extract_state for the meanings of xstate's various
   members.  The xstate memory must be considered invalidated
   immediately after this function returns, thus implementations
   must copy or consume anything they need from xstate before
   returning.

   Implementations must return 0 on success. As a special case, if
   FSL_RC_BREAK is returned then fsl_repo_extract() will stop
   looping over files but will report it as success (by returning
   0). Any other code causes extraction looping to stop and is
   returned as-is to the caller of fsl_repo_extract().

   When returning an error, the client may use fsl_cx_err_set() to
   populate state->f with a useful error message which will
   propagate back up through the call stack.

   @see fsl_repo_extract()
*/
typedef int (*fsl_repo_extract_f)( fsl_repo_extract_state const * xstate );

/**
   Options for use with fsl_repo_extract().
*/
struct fsl_repo_extract_opt {
  /**
     The version of the repostitory to check out. This must be
     the blob.rid of a checkin artifact.
  */
  fsl_id_t checkinRid;
  /**
     The callback to call for each extracted file in the checkin.
     May not be NULL.
  */
  fsl_repo_extract_f callback;
  /**
     Optional state pointer to pass to the callback when extracting.
     Its interpretation is client-dependent.
  */
  void * callbackState;
  /**
     If true, the fsl_repo_extract_state::content pointer passed to
     the callback will be non-NULL and will contain the content of the
     file. If false, that pointer will be NULL. Such extraction is a
     relatively costly operation, so should only be enabled when
     necessary. Some uses cases can delay this decision until the
     callback and only fetch the content for cases which need it.
  */
  bool extractContent;
};

typedef struct fsl_repo_extract_opt fsl_repo_extract_opt;
/**
   Initialized-with-defaults fsl_repo_extract_opt instance, intended
   for intializing via const-copy initialization.
*/
#define fsl_repo_extract_opt_empty_m \
  {0/*checkinRid*/,NULL/*callback*/, \
   NULL/*callbackState*/,false/*extractContent*/}
/**
   Initialized-with-defaults fsl_repo_extract_opt instance,
   intended for intializing new non-const instances.
*/
FSL_EXPORT const fsl_repo_extract_opt fsl_repo_extract_opt_empty;

/**
   Extracts the contents of a single checkin from a repository,
   sending the appropriate version of each file's contents to a
   client-specified callback.

   For each file in the given checkin, opt->callback() is passed a
   fsl_repo_extract_state instance containing enough information to,
   e.g., unpack the contents to a working directory, add it to a
   compressed archive, or send it to some other destination.

   Returns 0 on success, non-0 on error. It will fail if f has no
   opened repository db.

   If the callback returns any code other than 0 or FSL_RC_BREAK,
   looping over the list of files ends and this function returns
   that value. FSL_RC_BREAK causes looping to stop but 0 is
   returned.

   Files deleted by the given version are NOT reported to the callback
   (because getting sane semantics has proven to be tricker and more
   costly than it's worth).

   See fsl_repo_extract_f() for more details about the semantics of
   the callback. See fsl_repo_extract_opt for the documentation of the
   various options.

   Fossil's internal metadata format guarantees that files will passed
   be passed to the callback in "lexical order" (as defined by
   fossil's manifest format definition). i.e. the files will be passed
   in case-sensitive, alphabetical order. Note that upper-case letters
   sort before lower-case ones.

   Sidebar: this function makes a bitwise copy of the 2nd argument
   before starting its work, just in case the caller gets the crazy
   idea to modify it from the extraction callback. Whether or not
   there are valid/interesting uses for such modification remains to
   be seen. If any are found, this copy behavior may change.
*/
FSL_EXPORT int fsl_repo_extract( fsl_cx * f,
                                 fsl_repo_extract_opt const * opt );

/**
   Equivalent to fsl_tag_rid() except that it takes a symbolic
   artifact name in place of an artifact ID as the third
   argumemnt.

   This function passes symToTag to fsl_sym_to_rid(), and on
   success passes the rest of the parameters as-is to
   fsl_tag_rid(). See that function the semantics of the other
   arguments and the return value, as well as a description of the
   side effects.
*/
FSL_EXPORT int fsl_tag_sym( fsl_cx * f, fsl_tagtype_e tagType,
                 char const * symToTag, char const * tagName,
                 char const * tagValue, char const * userName,
                 double mtime, fsl_id_t * newId );

/**
   Adds a control record to f's repositoriy that either creates or
   cancels a tag.

   artifactRidToTag is the RID of the record to be tagged.

   tagType is the type (add, cancel, or propagate) of tag.

   tagName is the name of the tag. Must not be NULL/empty.

   tagValue is the optional value for the tag. May be NULL.

   userName is the user's name to apply to the artifact. May not be
   empty/NULL. Use fsl_guess_user_name() to try to figure out a
   proper user name based on the environment. See also:
   fsl_cx_user_get(), but note that the application must first
   use fsl_cx_user_set() to set a context's user name.

   mtime is the Julian Day timestamp for the new artifact. Pass a
   value <=0 to use the current time.

   If newId is not NULL then on success the rid of the new tag control
   artifact is assigned to *newId.

   Returns 0 on success and has about a million and thirteen
   possible error conditions. On success a new artifact record is
   written to the db, its RID being written into newId as described
   above.

   If the artifact being tagged is private, the new tag is also
   marked as private.

*/
FSL_EXPORT int fsl_tag_an_rid( fsl_cx * f, fsl_tagtype_e tagType,
                 fsl_id_t artifactRidToTag, char const * tagName,
                 char const * tagValue, char const * userName,
                 double mtime, fsl_id_t * newId );

/**
    Searches for a repo.tag entry given name in the given context's
    repository db. If found, it returns the record's id. If no
    record is found and create is true (non-0) then a tag is created
    and its entry id is returned. Returns 0 if it finds no entry, a
    negative value on error. On db-level error, f's error state is
    updated.
*/
FSL_EXPORT fsl_id_t fsl_tag_id( fsl_cx * f, char const * tag, bool create );


/**
   Returns true if the checkin with the given rid is a leaf, false if
   not. Returns false if f has no repo db opened, the query fails
   (likely indicating that it is not a repository db), or just about
   any other conceivable non-success case.

   A leaf, by the way, is a commit which has no children in the same
   branch.

   Sidebar: this function calculates whether the RID is a leaf, as
   opposed to checking the "static" (pre-calculated) list of leaves in
   the [leaf] table.
*/
FSL_EXPORT bool fsl_rid_is_leaf(fsl_cx * f, fsl_id_t rid);

/**
   Counts the number of primary non-branch children for the given
   check-in.

   A primary child is one where the parent is the primary parent, not
   a merge parent.  A "leaf" is a node that has zero children of any
   kind.  This routine counts only primary children.

   A non-branch child is one which is on the same branch as the parent.

   Returns a negative value on error.
*/
FSL_EXPORT fsl_int_t fsl_count_nonbranch_children(fsl_cx * f, fsl_id_t rid);

/**
   Looks for the delta table record where rid==deltaRid, and
   returns that record's srcid via *rv. Returns 0 on success, non-0
   on error. If no record is found, *rv is set to 0 and 0 is
   returned (as opposed to FSL_RC_NOT_FOUND) because that generally
   simplifies the error checking.
*/
FSL_EXPORT int fsl_delta_src_id( fsl_cx * f, fsl_id_t deltaRid, fsl_id_t * rv );


/**
   Return true if the given artifact ID should is listed in f's
   shun table, else false.
*/
FSL_EXPORT int fsl_uuid_is_shunned(fsl_cx * f, fsl_uuid_cstr zUuid);


/**
   Compute the "mtime" of the file given whose blob.rid is "fid"
   that is part of check-in "vid".  The mtime will be the mtime on
   vid or some ancestor of vid where fid first appears. Note that
   fossil does not track the "real" mtimes of files, it only
   computes reasonable estimates for those files based on the
   timestamps of their most recent checkin in the ancestry of vid.

   On success, if pMTime is not null then the result is written to
   *pMTime.

   If fid is 0 or less then the checkin time of vid is written to
   pMTime (this is a much less expensive operation, by the way).
   In this particular case, FSL_RC_NOT_FOUND is returned if vid is
   not a valid checkin version.

   Returns 0 on success, non-0 on error. Returns FSL_RC_NOT_FOUND
   if fid is not found in vid.

   This routine is much more efficient if used to answer several
   queries in a row for the same manifest (the vid parameter). It
   is least efficient when it is passed intermixed manifest IDs,
   e.g. (1, 3, 1, 4, 1,...). This is a side-effect of the caching
   used in the computation of ancestors for a given vid.
*/
FSL_EXPORT int fsl_mtime_of_manifest_file(fsl_cx * f, fsl_id_t vid, fsl_id_t fid, fsl_time_t *pMTime);

/**
   A convenience form of fsl_mtime_of_manifest_file() which looks up
   fc's RID based on its UUID. vid must be the RID of the checkin
   version fc originates from. See fsl_mtime_of_manifest_file() for
   full details - this function simply calculates the 3rd argument
   for that one.
*/
FSL_EXPORT int fsl_mtime_of_F_card(fsl_cx * f, fsl_id_t vid, fsl_card_F const * fc, fsl_time_t *pMTime);

/**
   Ensures that the given list has capacity for at least n entries. If
   the capacity is currently equal to or less than n, this is a no-op
   unless n is 0, in which case li->list is freed and the list is
   zeroed out. Else li->list is expanded to hold at least n
   elements. Returns 0 on success, FSL_RC_OOM on allocation error.
 */
int fsl_card_F_list_reserve( fsl_card_F_list * li, uint32_t n );

/**
   Frees all memory owned by li and the F-cards it contains. Does not
   free the li pointer.
*/
void fsl_card_F_list_finalize( fsl_card_F_list * li );

/**
   Holds options for use with fsl_branch_create().
*/
struct fsl_branch_opt {
  /**
     The checkin RID from which the branch should originate.
  */
  fsl_id_t basisRid;
  /**
     The name of the branch. May not be NULL or empty.
  */
  char const * name;
  /**
     User name for the branch. If NULL, fsl_cx_user_get() will
     be used.
  */ 
  char const * user;
  /**
     Optional comment (may be NULL). If NULL or empty, a default
     comment is generated (because fossil requires a non-empty
     comment string).
  */
  char const * comment;
  /**
     Optional background color for the fossil(1) HTML timeline
     view.  Must be in \#RRGGBB format, but this API does not
     validate it as such.
  */
  char const * bgColor;
  /**
     The julian time of the branch. If 0 or less, default is the
     current time.
  */
  double mtime;
  /**
     If true, the branch will be marked as private.
  */
  char isPrivate;
};
typedef struct fsl_branch_opt fsl_branch_opt;
#define fsl_branch_opt_empty_m {                \
    0/*basisRid*/, NULL/*name*/,                \
      NULL/*user*/, NULL/*comment*/,            \
      NULL/*bgColor*/,                          \
      0.0/*mtime*/, 0/*isPrivate*/              \
      }
FSL_EXPORT const fsl_branch_opt fsl_branch_opt_empty;

/**
   Creates a new branch in f's repository. The 2nd paramter holds
   the options describing the branch. The 3rd parameter may be
   NULL, but if it is not then on success the RID of the new
   manifest is assigned to *newRid.

   In Fossil branches are implemented as tags. The branch name
   provided by the client will cause the creation of a tag with
   name name plus a "sym-" prefix to be created (if needed).
   "sym-" denotes that it is a "symbolic tag" (fossil's term for
   "symbolic name applying to one or more checkins,"
   i.e. branches).

   Creating a branch cancels all other branch tags which the new
   branch would normally inherit.

   Returns 0 on success, non-0 on error. 
*/
FSL_EXPORT int fsl_branch_create(fsl_cx * f, fsl_branch_opt const * opt, fsl_id_t * newRid );


/**
   Tries to determine the [filename.fnid] value for the given
   filename.  Returns a positive value if it finds one, 0 if it
   finds none, and some unspecified negative value(s) for any sort
   of error. filename must be a normalized, relative filename (as it
   is recorded by a repo).
*/
FSL_EXPORT fsl_id_t fsl_repo_filename_fnid( fsl_cx * f, char const * filename );


/**
   Imports content to f's opened repository's BLOB table using a
   client-provided input source. f must have an opened repository
   db. inFunc is the source of the data and inState is the first
   argument passed to inFunc(). If inFunc() succeeds in fetching all
   data (i.e. if it always returns 0 when called by this function)
   then that data is inserted into the blob table _if_ no existing
   record with the same hash is already in the table. If such a record
   exists, it is assumed that the content is identical and this
   function has no side-effects vis-a-vis the db in that case.

   If rid is not NULL then the BLOB.RID record value (possibly of an
   older record!) is stored in *rid.  If uuid is not NULL then the
   BLOB.UUID record value is stored in *uuid and the caller takes
   ownership of those bytes, which must eventually be passed to
   fsl_free() to release them.

   rid and uuid are only modified on success and only if they are
   not NULL.

   Returns 0 on success, non-0 on error. For errors other than basic
   argument validation and OOM conditions, f's error state is
   updated with a description of the problem. Returns FSL_RC_MISUSE
   if either f or inFunc are NULL. Whether or not inState may be
   NULL depends on inFunc's concrete implementation.

   Be aware that BLOB.RID values can (but do not necessarily) change
   in the life of a repod db (via a reconstruct, a full re-clone, or
   similar, or simply when referring to different clones of the same
   repo). Thus clients should always store the full UUID, as opposed
   to the RID, for later reference. RIDs should, in general, be
   treated as session-transient values. That said, for purposes of
   linking tables in the db, the RID is used exclusively (clients are
   free to link their own extension tables using UUIDs, but doing so
   has a performance penalty comared to RIDs). For long-term storage
   of external links, and to guaranty that the data be usable with
   other copies of the same repo, the UUID is required.

   Note that Fossil may deltify, compress, or otherwise modify
   content on its way into the blob table, and it may even modify
   content long after its insertion (e.g. to make it a delta against
   a newer version). Thus clients should normally never try
   to read back the blob directly from the database, but should
   instead read it using fsl_content_get().

   That said: this routine has no way of associating and older version
   (if any) of the same content with this newly-imported version, and
   therefore cannot delta-compress the older version.

   Maintenance reminder: this is basically just a glorified form of
   the internal fsl_content_put(). Interestingly, fsl_content_put()
   always sets content to public (by default - the f object may
   override that later). It is not yet clear whether this routine
   needs to have a flag to set the blob private or not. Generally
   speaking, privacy is applied to fossil artifacts, as opposed to
   content blobs.

   @see fsl_repo_import_buffer()
*/
FSL_EXPORT int fsl_repo_import_blob( fsl_cx * f, fsl_input_f inFunc,
                                     void * inState, fsl_id_t * rid,
                                     fsl_uuid_str * uuid );

/**
   A convenience form of fsl_repo_import_blob(), equivalent to:

   @code
   fsl_repo_import_blob(f, fsl_input_f_buffer, bIn, rid, uuid )
   @endcode

   except that (A) bIn is const in this call and non-const in the
   other form (due to cursor traversal requirements) and (B) it
   returns FSL_RC_MISUSE if bIn is NULL.
*/
FSL_EXPORT int fsl_repo_import_buffer( fsl_cx * f, fsl_buffer const * bIn,
                                       fsl_id_t * rid, fsl_uuid_str * uuid );

/**
   Resolves client-provided symbol as an artifact's db record ID.
   f must have an opened repository db, and some symbols can only
   be looked up if it has an opened checkout (see the list below).

   Returns 0 and sets *rv to the id if it finds an unambiguous
   match.

   Returns FSL_RC_MISUSE if !f, !sym, !*sym, or !rv.

   Returns FSL_RC_NOT_A_REPO if f has no opened repository.

   Returns FSL_RC_AMBIGUOUS if sym is a partial UUID which matches
   multiple full UUIDs.

   Returns FSL_RC_NOT_FOUND if it cannot find anything.

   Symbols supported by this function:

   - SHA1/3 hash
   - SHA1/3 hash prefix of at least 4 characters
   - Symbolic Name
   - "tag:" + symbolic name
   - Date or date-time 
   - "date:" + Date or date-time
   - symbolic-name ":" date-time
   - "tip"

   - "rid:###" resolves to the hash of blob.rid ### if that RID is in
   the database

   The following additional forms are available in local checkouts:

   - "current"
   - "prev" or "previous"
   - "next"

   The following prefix may be applied to the above to modify how
   they are resolved:

   - "root:" prefix resolves to the checkin of the parent branch from
   which the record's branch divered. i.e. the version from which it
   was branched. In the trunk this will always resolve to the first
   checkin.

   - "merge-in:" TODO - document this once its implications are
   understood.

   If type is not FSL_SATYPE_ANY then it will only match artifacts
   of the specified type. In order to resolve arbitrary UUIDs, e.g.
   those of arbitrary blob content, type needs to be
   FSL_SATYPE_ANY.

*/
FSL_EXPORT int fsl_sym_to_rid( fsl_cx * f, char const * sym, fsl_satype_e type,
                               fsl_id_t * rv );

/**
   Similar to fsl_sym_to_rid() but on success it returns a UUID string
   by assigning it to *rv (if rv is not NULL). If rid is not NULL then
   on success the db record ID corresponding to the returned UUID is
   assigned to *rid. The caller must eventually free the returned
   string memory by passing it to fsl_free(). Returns 0 if it finds a
   match and any number of result codes on error.
*/
FSL_EXPORT int fsl_sym_to_uuid( fsl_cx * f, char const * sym,
                                fsl_satype_e type, fsl_uuid_str * rv,
                                fsl_id_t * rid );


/**
   Searches f's repo database for the a blob with the given uuid
   (any unique UUID prefix). On success a positive record ID is
   returned. On error one of several unspecified negative values is
   returned. If no uuid match is found 0 is returned.

   Error cases include: either argument is NULL, uuid does not
   appear to be a full or partial UUID (or is too long),
   uuid is ambiguous (try providing a longer one)

   This implementation is more efficient when given a full,
   valid UUID (one for which fsl_is_uuid() returns true).
*/
FSL_EXPORT fsl_id_t fsl_uuid_to_rid( fsl_cx * f, char const * uuid );

/**
   The opposite of fsl_uuid_to_rid(), this returns the UUID string
   of the given blob record ID. Ownership of the string is passed
   to the caller and it must eventually be freed using
   fsl_free(). Returns NULL on error (invalid arguments or f has no
   repo opened) or if no blob record is found. If no record is
   found, f's error state is updated with an explanation of the
   problem.
*/
FSL_EXPORT fsl_uuid_str fsl_rid_to_uuid(fsl_cx * f, fsl_id_t rid);

/**
   Works like fsl_rid_to_uuid() but assigns the UUID to the given
   buffer, re-using its memory, if any. Returns 0 on success,
   FSL_RC_MISUSE if rid is not positive, FSL_RC_OOM on allocation
   error, and FSL_RC_NOT_FOUND if no blob entry matching the given rid
   is found.
*/
FSL_EXPORT int fsl_rid_to_uuid2(fsl_cx * f, fsl_id_t rid, fsl_buffer *uuid);

/**
   This works identically to fsl_rid_to_uuid() except that it will
   only resolve to a UUID if an artifact matching the given type has
   that UUID. If no entry is found, f's error state gets updated
   with a description of the problem.

   This can be used to distinguish artifact UUIDs from file blob
   content UUIDs by passing the type FSL_SATYPE_ANY. A non-artifact
   blob will return NULL in that case, but any artifact type will
   match (assuming rid is valid).
*/
FSL_EXPORT fsl_uuid_str fsl_rid_to_artifact_uuid(fsl_cx * f, fsl_id_t rid,
                                                 fsl_satype_e type);
/**
   Returns the raw SQL code for a Fossil global config database.

   TODO: add optional (fsl_size_t*) to return the length.
*/
FSL_EXPORT char const * fsl_schema_config();

/**
   Returns the raw SQL code for the "static" parts of a Fossil
   repository database. These are the parts which are immutable
   (for the most part) between Fossil versions. They change _very_
   rarely.

   TODO: add optional (fsl_size_t*) to return the length.
*/
FSL_EXPORT char const * fsl_schema_repo1();

/**
   Returns the raw SQL code for the "transient" parts of a Fossil
   repository database - any parts which can be calculated via data
   held in the primary "static" schemas. These parts are
   occassionally recreated, e.g. via a 'rebuild' of a repository.

   TODO: add optional (fsl_size_t*) to return the length.
*/
FSL_EXPORT char const * fsl_schema_repo2();

/**
   Returns the raw SQL code for a Fossil checkout database.

   TODO: add optional (fsl_size_t*) to return the length.
*/
FSL_EXPORT char const * fsl_schema_ckout();

/**
   Returns the raw SQL code for a Fossil checkout db's
   _default_ core ticket-related tables.

   TODO: add optional (fsl_size_t*) to return the length.

   @see fsl_cx_schema_ticket()
*/
FSL_EXPORT char const * fsl_schema_ticket();

/**
   Returns the raw SQL code for the "forum" parts of a Fossil
   repository database.

   TODO: add optional (fsl_size_t*) to return the length.
*/
FSL_EXPORT char const * fsl_schema_forum();

/**
   If f's opened repository has a non-empty config entry named
   'ticket-table', this returns its text via appending it to
   pOut. If no entry is found, fsl_schema_ticket() is appended to
   pOut.

   Returns 0 on success. On error the contents of pOut must not be
   considered valid but pOut might be partially populated.
*/
FSL_EXPORT int fsl_cx_schema_ticket(fsl_cx * f, fsl_buffer * pOut);

/**
   Returns the raw SQL code for Fossil ticket reports schemas.
   This gets installed as needed into repository databases.

   TODO: add optional (fsl_size_t*) to return the length.
*/
FSL_EXPORT char const * fsl_schema_ticket_reports();

/**
   This is a wrapper around fsl_cx_hash_buffer() which looks for a
   matching artifact for the given input blob. It first hashes src
   using f's "alternate" hash and then, if no match is found, tries
   again with f's preferred hash.

   On success (a match is found):

   - Returns 0.

   - If ridOut is not NULL, *ridOut is set to the RID of the matching blob.

   - If hashOut is not NULL, *hashOut is set to the hash of the
   blob. Its ownership is transferred to the caller, who must
   eventually pass it to fsl_free().

   If no matching blob is found in the repository, FSL_RC_NOT_FOUND is
   returned (but f's error state is not annotated with more
   information). Returns FSL_RC_NOT_A_REPO if f has no repository
   opened. For more serious errors, e.g. allocation error or db
   problems, another (more serious) result code is returned,
   e.g. FSL_RC_OOM or FSL_RC_DB.

   If FSL_RC_NOT_FOUND is returned and hashOut is not NULL, *hashOut
   is set to the value of f's preferred hash. *ridOut is only modified
   if 0 is returned, in which case *ridOut will have a positive value.
*/
FSL_EXPORT int fsl_repo_blob_lookup( fsl_cx * f, fsl_buffer const * src, fsl_id_t * ridOut,
                                     fsl_uuid_str * hashOut );

/**
   Returns true if the specified file name ends with any reserved
   name, e.g.: _FOSSIL_ or .fslckout.

   For the sake of efficiency, zFilename must be a canonical name,
   e.g. an absolute or checkout-relative path using only forward slash
   ('/') as a directory separator.

   On Windows builds, this also checks for reserved Windows filenames,
   e.g. "CON" and "PRN".

   nameLen must be the length of zFilename. If it is negative,
   fsl_strlen() is used to calculate it.
*/
FSL_EXPORT bool fsl_is_reserved_fn(const char *zFilename,
                                   fsl_int_t nameLen );

/**
   Uses fsl_is_reserved_fn() to determine whether the filename part of
   zPath is legal for use as an in-repository filename. If it is, 0 is
   returned, else FSL_RC_RANGE (or FSL_RC_OOM) is returned and f's
   error state is updated to indicate the nature of the problem. nFile
   is the length of zPath. If negative, fsl_strlen() is used to
   determine its length.

   If relativeToCwd is true then zPath, if not absolute, is
   canonicalized as if were relative to the current working directory
   (see fsl_getcwd()), else it is assumed to be relative to the
   current checkout (if any - falling back to the current working
   directory). This flag is only relevant if zPath is not absolute and
   if f has a checkout opened. An absolute zPath is used as-is and if
   no checkout is opened then relativeToCwd is always treated as if it
   were true.

   This routine does not validate that zPath lives inside a checkout
   nor that the file actually exists. It does only name comparison and
   only uses the filesystem for purposes of canonicalizing (if needed)
   zPath.

   This routine does not require that f have an opened repo, but if it
   does then this routine compares the canonicalized forms of both the
   repository db and the given path and fails if zPath refers to the
   repository db. Be aware that the relativeToCwd flag may influence
   that test.

   TODO/FIXME: if f's 'manifest' config setting is set to true AND
   zPath refers to the top of the checkout root, treat the files
   (manifest, manifest.uuid, manifest.tags) as reserved. If it is a
   string with any of the letters "r", "u", or "t", check only the
   file(s) which those letters represent (see
   add.c:fossil_reserved_name() in fossil). Apply these only at the top
   of the tree - allow them in subdirectories.
*/
FSL_EXPORT int fsl_reserved_fn_check(fsl_cx *f, const char *zPath,
                                     fsl_int_t nFile, bool relativeToCwd);

/**
   Recompute/rebuild the entire repo.leaf table. This is not normally
   needed, as leaf tracking is part of the crosslinking process, but
   "just in case," here it is.

   This can supposedly be expensive (in time) for a really large
   repository. Testing implies otherwise.

   Returns 0 on success. Error may indicate that f has no repo db
   opened.  On error f's error state may be updated.
*/
FSL_EXPORT int fsl_repo_leaves_rebuild(fsl_cx * f);  

/**
   Flags for use with fsl_leaves_compute().
*/
enum fsl_leaves_compute_e {
/**
   Compute all leaves regardless of the "closed" tag.
*/
FSL_LEAVES_COMPUTE_ALL = 0,
/**
   Compute only leaves without the "closed" tag.
*/
FSL_LEAVES_COMPUTE_OPEN = 1,
/**
   Compute only leaves with the "closed" tag.
*/
FSL_LEAVES_COMPUTE_CLOSED = 2
};
typedef enum fsl_leaves_compute_e fsl_leaves_compute_e;

/**
   Creates a temporary table named "leaves" if it does not already
   exist, else empties it. Populates that table with the RID of all
   check-ins that are leaves which are descended from the checkin
   referred to by vid.

   A "leaf" is a check-in that has no children in the same branch.
   There is a separate permanent table named [leaf] that contains all
   leaves in the tree. This routine is used to compute a subset of
   that table consisting of leaves that are descended from a single
   check-in.
   
   The leafMode flag determines behavior associated with the "closed"
   tag, as documented for the fsl_leaves_compute_e enum.

   If vid is <=0 then this function, after setting up or cleaning out
   the [leaves] table, simply copies the list of leaves from the
   repository's pre-computed [leaf] table (see
   fsl_repo_leaves_rebuild()).

   @see fsl_leaves_computed_has()
   @see fsl_leaves_computed_count()
   @see fsl_leaves_computed_latest()
   @see fsl_leaves_computed_cleanup()
*/
FSL_EXPORT int fsl_leaves_compute(fsl_cx * f, fsl_id_t vid,
                                  fsl_leaves_compute_e leafMode);

/**
   Requires that a prior call to fsl_leaves_compute() has succeeded,
   else results are undefined.

   Returns true if the leaves list computed by fsl_leaves_compute() is
   not empty, else false. This is more efficient than checking
   against fsl_leaves_computed_count()>0.
*/
FSL_EXPORT bool fsl_leaves_computed_has(fsl_cx * f);

/**
   Requires that a prior call to fsl_leaves_compute() has succeeded,
   else results are undefined.

   Returns a count of the leaves list computed by
   fsl_leaves_compute(), or a negative value if a db-level error is
   encountered. On errors other than FSL_RC_OOM, f's error state will
   be updated with information about the error.
*/
FSL_EXPORT fsl_int_t fsl_leaves_computed_count(fsl_cx * f);

/**
   Requires that a prior call to fsl_leaves_compute() has succeeded,
   else results are undefined.

   Returns the RID of the most recent checkin from those computed by
   fsl_leaves_compute(), 0 if no entries are found, or a negative
   value if a db-level error is encountered. On errors other than
   FSL_RC_OOM, f's error state will be updated with information about
   the error.
*/
FSL_EXPORT fsl_id_t fsl_leaves_computed_latest(fsl_cx * f);

/**
   Cleans up any db-side resources created by fsl_leaves_compute().
   e.g. drops the temporary table created by that routine. Any errors
   are silenty ignored.
*/
FSL_EXPORT void fsl_leaves_computed_cleanup(fsl_cx * f);

/**
   Returns true if f's current repository has the
   forbid-delta-manifests setting set to a truthy value. Results are
   undefined if f has no opened repository. Some routines behave
   differently if this setting is enabled. e.g. fsl_checkin_commit()
   will never generate a delta manifest and fsl_deck_save() will
   refuse to save a delta. This does not affect parsing or deltas or
   those which are injected into the db via lower-level means (e.g. a
   direct blob import or from a remote sync).

   Results are undefined if f has no opened repository.
*/
FSL_EXPORT bool fsl_repo_forbids_delta_manifests(fsl_cx * f);

/**
   This is a variant of fsl_ckout_manifest_write() which writes data
   regarding the given manifest RID to the given blobs. If manifestRid
   is 0 or less then the current checkout is assumed and
   FSL_RC_NOT_A_CKOUT is returned if no checkout is opened (or
   FSL_RC_RANGE if an empty checkout is opened - a freshly-created
   repository with no checkins).

   For each buffer argument which is not NULL, the corresponding
   checkin-related data are appended to it. All such blobs will end
   in a terminating newline character.

   Returns 0 on success, any of numerious non-0 fsl_rc_e codes on
   error.
*/
FSL_EXPORT int fsl_repo_manifest_write(fsl_cx *f,
                                       fsl_id_t manifestRid,
                                       fsl_buffer * const manifest,
                                       fsl_buffer * const manifestUuid,
                                       fsl_buffer * const manifestTags );

/**
   UNDER CONSTRUCTION. Configuration for use with fsl_annotate().
*/
struct fsl_annotate_opt {
  /**
     The repository-root-relative NUL-terminated filename to annotate.
  */
  char const * filename;
  /**
     The checkin from which the file's version should be selected. A
     value of 0 or less means the current checkout, if in a checkout,
     and is otherwise an error.
  */
  fsl_id_t versionRid;
  /**
     The origin checkin version. A value of 0 or less means the "root of the
     tree."

     TODO: figure out and explain the difference between versionRid
     and originRid.
  */
  fsl_id_t originRid;
  /**
     The maximum number of versions to search through.
  */
  unsigned int limit;
  /**
     - 0 = do not ignore any spaces.
     - <0 = ignore trailing end-of-line spaces.
     - >1 = ignore all spaces
  */
  int spacePolicy;
  /**
     If true, include the name of the user for which each
     change is attributed (noting that merges show whoever
     merged the change, which may differ from the original
     committer. If false, show only version information.

     This option is alternately known as "blame".
  */
  bool praise;
  /**
     Output file blob versions, instead of checkin versions.
  */
  bool fileVersions;
  /**
     The output channel for the resulting annotation.
  */
  fsl_output_f out;
  /**
     State for passing as the first argument to this->out().
  */
  void * outState;
};
/** Convenience typedef. */
typedef struct fsl_annotate_opt fsl_annotate_opt;

/** Initialized-with-defaults fsl_annotate_opt structure, intended for
    const-copy initialization. */
#define fsl_annotate_opt_empty_m {\
  NULL/*filename*/, \
  0/*versionRid*/,0/*originRid*/,    \
  0U/*limit*/, 0/*spacePolicy*/, \
  false/*praise*/, \
  NULL/*out*/, NULL/*outState*/ \
}

/** Initialized-with-defaults fsl_annotate_opt structure, intended for
    non-const copy initialization. */
extern const fsl_annotate_opt fsl_annotate_opt_empty;

/**
   UNDER CONSTRUCTION, not yet implemented.

   Runs an "annotation" of an SCM-controled file and sends the results
   to opt->out().

   Returns 0 on success. On error, returns one of:

   - FSL_RC_OOM on OOM

   - FSL_RC_NOT_A_CKOUT if opt->versionRid<=0 and f has no opened checkout.

   - FSL_RC_NOT_FOUND if the given filename cannot be found in the
     repository OR a given version ID does not resolve to a blob. (Sorry
     about this ambiguity!)

   - FSL_RC_PHANTOM if a phantom blob is encountered while trying to
     annotate.

   opt->out() may return arbitrary non-0 result codes, in which case
   the returned code is propagated to the caller of this function.
*/
FSL_EXPORT int fsl_annotate( fsl_cx * const f,
                             fsl_annotate_opt const * const opt );

#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_REPO_H_INCLUDED */
/* end of file ../include/fossil-scm/fossil-repo.h */
/* start of file ../include/fossil-scm/fossil-checkout.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_CHECKOUT_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_CHECKOUT_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).
*/

/** @file fossil-checkout.h

    fossil-checkout.h declares APIs specifically dealing with
    checkout-side state, as opposed to purely repository-db-side state
    or non-content-related APIs.
*/


#if defined(__cplusplus)
extern "C" {
#endif


/**
   Returns version information for the current checkout.

   If f has an opened checkout then...

   If uuid is not NULL then *uuid is set to the UUID of the opened
   checkout, or NULL if there is no checkout. If rid is not NULL, *rid
   is set to the record ID of that checkout, or 0 if there is no
   checkout (or the current checkout is from an empty repository). The
   returned uuid bytes and rid are owned by f and valid until the
   library updates its checkout state to a newer checkout version
   (essentially unpredictably). When in doubt about lifetime issues,
   copy the UUID immediately after calling this if they will be needed
   later.

   Corner case: a new repo with no checkins has an RID of 0 and a UUID
   of NULL. That does not happen with fossil-generated repositories,
   as those always "seed" the database with an initial commit artifact
   containing no files.
*/
FSL_EXPORT void fsl_ckout_version_info(fsl_cx *f, fsl_id_t * rid,
                                       fsl_uuid_cstr * uuid );

/**
   Given a fsl_cx with an opened checkout, and a filename, this
   function canonicalizes zOrigName to a form suitable for use as
   an in-repo filename, _appending_ the results to pOut. If pOut is
   NULL, it performs its normal checking but does not write a
   result, other than to return 0 for success.

   As a special case, if zOrigName refers to the top-level checkout
   directory, it resolves to either "." or "./", depending on whether
   zOrigName contains a trailing slash.

   If relativeToCwd is true then the filename is canonicalized
   based on the current working directory (see fsl_getcwd()),
   otherwise f's current checkout directory is used as the virtual
   root.

   If the input name contains a trailing slash, it is retained in
   the output sent to pOut except in the top-dir case mentioned
   above.

   Returns 0 on success, meaning that the value appended to pOut
   (if not NULL) is a syntactically valid checkout-relative path.

   Returns FSL_RC_RANGE if zOrigName points to a path outside
   of f's current checkout root.

   Returns FSL_RC_NOT_A_CKOUT if f has no checkout opened.

   Returns FSL_RC_MISUSE if !zOrigName, FSL_RC_OOM on an allocation
   error.

   This function does not validate whether or not the file actually
   exists, only that its name is potentially valid as a filename
   for use in a checkout (though other, downstream rules might prohibit that, e.g.
   the filename "..../...." is not valid but is not seen as invalid by
   this function). (Reminder to self: we could run the end result through
   fsl_is_simple_pathname() to catch that?)
*/
FSL_EXPORT int fsl_ckout_filename_check( fsl_cx * f, bool relativeToCwd,
                                         char const * zOrigName, fsl_buffer * pOut );

/**
   Callback type for use with fsl_ckout_manage_opt(). It should
   inspect the given filename using whatever criteria it likes, set
   *include to true or false to indicate whether the filename is okay
   to include the current add-file-to-repo operation, and return 0.

   If it returns non-0 the add-file-to-repo process will end and that
   error code will be reported to its caller. Such result codes must
   come from the FSL_RC_xxx family.

   It will be passed a name which is relative to the top-most checkout
   directory.

   The final argument is not used by the library, but is passed on
   as-is from the fsl_ckout_manage_opt::callbackState pointer which
   is passed to fsl_ckout_manage().
*/
typedef int (*fsl_ckout_manage_f)(const char *zFilename,
                                    bool *include,
                                    void *state);
/**
   Options for use with fsl_ckout_manage().
*/
struct fsl_ckout_manage_opt {
  /**
     The file or directory name to add. If it is a directory, the add
     process will recurse into it.
  */
  char const * filename;
  /**
     Whether to evaluate the given name as relative to the current working
     directory or to the current checkout root.

     This makes a subtle yet important difference in how the name is
     resolved. CLI apps which take file names from the user from
     within a checkout directory will generally want to set
     relativeToCwd to true. GUI apps, OTOH, will possibly need it to
     be false, depending on how they resolve and pass on the
     filenames.
  */
  bool relativeToCwd;
  /**
     Whether or not to check the name(s) against the 'ignore-globs'
     config setting (if set).
  */     
  bool checkIgnoreGlobs;
  /**
     Optional predicate function which may be called for each
     to-be-added filename. It is only called if:

     - It is not NULL (obviously) and...

     - The file is not already in the checkout database and...

     - The is-internal-name check passes (see
     fsl_reserved_fn_check()) and...

     - If checkIgnoreGlobs is false or the name does not match one of
     the ignore-globs values.

     The name it is passed is relative to the checkout root.

     Because the callback is called only if other options have not
     already excluded the file, the client may use the callback to
     report to the user (or otherwise record) exactly which files
     get added.
  */
  fsl_ckout_manage_f callback;

  /**
     State to be passed to this->callback.
  */
  void * callbackState;

  /**
     These counts are updated by fsl_ckout_manage() to report
     what it did.
  */
  struct {
    /**
       Number of files actually added by fsl_ckout_manage().
    */
    uint32_t added;
    /**
       Number of files which were requested to be added but were only
       updated because they had previously been added. Updates set the
       vfile table entry's current mtime, executable-bit state, and
       is-it-a-symlink state. (That said, this code currently ignores
       symlinks altogether!)
    */
    uint32_t updated;
    /**
       The number of files skipped over for addition. This includes
       files which meet any of these criteria:

       - fsl_reserved_fn_check() fails.

       - If the checkIgnoreGlobs option is true and a filename matches
         any of those globs.

       - The client-provided callback says not to include the file.
    */
    uint32_t skipped;
  } counts;
};
typedef struct fsl_ckout_manage_opt fsl_ckout_manage_opt;
/**
   Initialized-with-defaults fsl_ckout_manage_opt instance,
   intended for use in const-copy initialization.
*/
#define fsl_ckout_manage_opt_empty_m {\
    NULL/*filename*/, true/*relativeToCwd*/, true/*checkIgnoreGlobs*/, \
    NULL/*callback*/, NULL/*callbackState*/,                         \
    {/*counts*/ 0/*added*/, 0/*updated*/, 0/*skipped*/}               \
  }
/**
   Initialized-with-defaults fsl_ckout_manage_opt instance,
   intended for use in non-const copy initialization.
*/
FSL_EXPORT const fsl_ckout_manage_opt fsl_ckout_manage_opt_empty;

/**
   Adds the given filename or directory (recursively) to the current
   checkout vfile list of files as a to-be-added file, or updates an
   existing record if one exists.

   This function ensures that opt->filename gets canonicalized and can
   be found under the checkout directory, and fails if no such file
   exists (checking against the canonicalized name). Filenames are all
   filtered through fsl_reserved_fn_check() and may have other filters
   applied to them, as determined by the options object.

   Each filename which passes through the filters is passed to
   the opt->callback (if not NULL), which may perform a final
   filtering check and/or alert the client about the file being
   queued.

   The options object is non-const because this routine updates
   opt->counts when it adds, updates, or skips a file. On each call,
   it updated opt->counts without resetting it (as this function is
   typically called in a loop). This function does not modify any
   other entries of that object and it requires that the object not be
   modified (e.g. via opt->callback()) while it is recursively
   processing. To reset the counts between calls, if needed:

   @code
   opt->counts = fsl_ckout_manage_opt_empty.counts;
   @endcode

   Returns 0 on success, non-0 on error.

   Files queued for addition this way can be unqueued before they are
   committed using fsl_ckout_unmanage().

   @see fsl_ckout_unmanage()
   @see fsl_reserved_fn_check()
*/
FSL_EXPORT int fsl_ckout_manage( fsl_cx * f,
                                   fsl_ckout_manage_opt * opt );


/**
   Callback type for use with fsl_ckout_unmanage(). It is called
   by the removal process, immediately after a file is "removed"
   from SCM management (a.k.a. when the file becomes "unmanaged").

   If it returns non-0 the unmanage process will end and that
   error code will be reported to its caller. Such result codes must
   come from the FSL_RC_xxx family.

   It will be passed a name which is relative to the top-most checkout
   directory. The client is free to unlink the file from the filesystem
   if they like - the library does not do so automatically

   The final argument is not used by the library, but is passed on
   as-is from the callbackState pointer which
   is passed to fsl_ckout_unmanage().
*/
typedef int (*fsl_ckout_unmanage_f)(const char *zFilename, void *state);

/**
   Options for use with fsl_ckout_unmanage().
*/
struct fsl_ckout_unmanage_opt {
  /**
     The file or directory name to add. If it is a directory, the add
     process will recurse into it. See also this->vfileIds.
  */
  char const * filename;
  /**
     An alternative to assigning this->filename is to point
     this->vfileIds to a bag of vfile.id values. If this member is not
     NULL, fsl_ckout_revert() will ignore this->filename.

     @see fsl_filename_to_vfile_ids()
  */
  fsl_id_bag const * vfileIds;
  /**
     Whether to evaluate this->filename as relative to the current
     working directory (true) or to the current checkout root
     (false). This is ignored when this->vfileIds is not NULL.

     This makes a subtle yet important difference in how the name is
     resolved. CLI apps which take file names from the user from
     within a checkout directory will generally want to set
     relativeToCwd to true. GUI apps, OTOH, will possibly need it to
     be false, depending on how they resolve and pass on the
     filenames.
  */
  bool relativeToCwd;
  /**
     If true, fsl_vfile_changes_scan() is called to ensure that
     the filesystem and vfile tables agree. If the client code has
     called that function, or its equivalent, since any changes were
     made to the checkout then this may be set to false to speed up
     the rm process.
  */
  bool scanForChanges;
  /**
     Optional predicate function which will be called after each
     file is made unmanaged.

     The name it is passed is relative to the checkout root.
  */
  fsl_ckout_unmanage_f callback;
  /**
     State to be passed to this->callback.
  */
  void * callbackState;

};
typedef struct fsl_ckout_unmanage_opt fsl_ckout_unmanage_opt;
/**
   Initialized-with-defaults fsl_ckout_unmanage_opt instance,
   intended for use in const-copy initialization.
*/
#define fsl_ckout_unmanage_opt_empty_m {\
    NULL/*filename*/, NULL/*vfileIds*/,\
    true/*relativeToCwd*/,true/*scanForChanges*/, \
    NULL/*callback*/, NULL/*callbackState*/ \
}
/**
   Initialized-with-defaults fsl_ckout_unmanage_opt instance,
   intended for use in non-const copy initialization.
*/
FSL_EXPORT const fsl_ckout_unmanage_opt fsl_ckout_unmanage_opt_empty;

/**
   The converse of fsl_ckout_manage(), this queues a file for removal
   from the current checkout. Unlike fsl_ckout_manage(), this routine
   does not ensure that opt->filename actually exists - it only
   normalizes zFilename into its repository-friendly form and passes
   it through the vfile table.

   If opt->filename refers to a directory then this operation queues
   all files under that directory (recursively) for removal. In this
   case, it is irrelevant whether or not opt->filename ends in a
   trailing slash.

   Returns 0 on success, any of a number of non-0 codes on error.
   Returns FSL_RC_MISUSE if !opt->filename or !*opt->filename.
   Returns FSL_RC_NOT_A_CKOUT if f has no opened checkout.

   If opt->callback is not NULL, it is called for each
   newly-unamanaged entry. The intention is to provide it the
   opportunity to notify the user, record the filename for later use,
   remove the file from the filesystem, etc. If it returns non-0, the
   unmanaging process will fail with that code and any pending
   transaction will be placed into a rollback state.

   This routine does not actually remove any files from the
   filesystem, it only modifies the vfile table entry so that the
   file(s) will be removed from the SCM by the commit process. If
   opt->filename is an entry which was previously
   fsl_ckout_manage()'d, but not yet committed, or any such entries
   are found under directory opt->filename, they are removed from the
   vfile table. i.e. this effective undoes the add operation.

   @see fsl_ckout_manage()
*/
FSL_EXPORT int fsl_ckout_unmanage( fsl_cx * f,
                                  fsl_ckout_unmanage_opt const * opt );

/**
   Hard-coded range of values of the vfile.chnged db field.
   These values are part of the fossil schema and must not
   be modified.
*/
enum fsl_vfile_change_e {
  /** File is unchanged. */
  FSL_VFILE_CHANGE_NONE = 0,
  /** File edit. */
  FSL_VFILE_CHANGE_MOD = 1,
  /** File changed due to a merge. */
  FSL_VFILE_CHANGE_MERGE_MOD = 2,
  /** File added by a merge. */
  FSL_VFILE_CHANGE_MERGE_ADD = 3,
  /** File changed due to an integrate merge. */
  FSL_VFILE_CHANGE_INTEGRATE_MOD = 4,
  /** File added by an integrate merge. */
  FSL_VFILE_CHANGE_INTEGRATE_ADD = 5,
  /** File became executable but has unmodified contents. */
  FSL_VFILE_CHANGE_IS_EXEC = 6,
  /** File became a symlink whose target equals its old contents. */
  FSL_VFILE_CHANGE_BECAME_SYMLINK = 7,
  /** File lost executable status but has unmodified contents. */
  FSL_VFILE_CHANGE_NOT_EXEC = 8,
  /** File lost symlink status and has contents equal to its old target. */
  FSL_VFILE_CHANGE_NOT_SYMLINK = 9
};
typedef enum fsl_vfile_change_e fsl_vfile_change_e;

/**
   Change-type flags for use with fsl_ckout_changes_visit() and
   friends.

   TODO: consolidate this with fsl_vfile_change_e insofar as possible.
   There are a few checkout change statuses not reflected in
   fsl_vfile_change_e.
*/
enum fsl_ckout_change_e {
/**
   Sentinel placeholder value.
*/
FSL_CKOUT_CHANGE_NONE = 0,
/**
   Indicates that a file was modified in some unspecified way.
*/
FSL_CKOUT_CHANGE_MOD = FSL_VFILE_CHANGE_MOD,
/**
   Indicates that a file was modified as the result of a merge.
*/
FSL_CKOUT_CHANGE_MERGE_MOD = FSL_VFILE_CHANGE_MERGE_MOD,
/**
   Indicates that a file was added as the result of a merge.
*/
FSL_CKOUT_CHANGE_MERGE_ADD = FSL_VFILE_CHANGE_MERGE_ADD,
/**
   Indicates that a file was modified as the result of an
   integrate-merge.
*/
FSL_CKOUT_CHANGE_INTEGRATE_MOD = FSL_VFILE_CHANGE_INTEGRATE_MOD,
/**
   Indicates that a file was added as the result of an
   integrate-merge.
*/
FSL_CKOUT_CHANGE_INTEGRATE_ADD = FSL_VFILE_CHANGE_INTEGRATE_ADD,
/**
   Indicates that the file gained the is-executable trait
   but is otherwise unmodified.
*/
FSL_CKOUT_CHANGE_IS_EXEC = FSL_VFILE_CHANGE_IS_EXEC,
/**
   Indicates that the file has changed to a symlink.
*/
FSL_CKOUT_CHANGE_BECAME_SYMLINK = FSL_VFILE_CHANGE_BECAME_SYMLINK,
/**
   Indicates that the file lost the is-executable trait
   but is otherwise unmodified.
*/
FSL_CKOUT_CHANGE_NOT_EXEC = FSL_VFILE_CHANGE_NOT_EXEC,
/**
   Indicates that the file was previously a symlink but is
   now a plain file.
*/
FSL_CKOUT_CHANGE_NOT_SYMLINK = FSL_VFILE_CHANGE_NOT_SYMLINK,
/**
   Indicates that a file was added.
*/
FSL_CKOUT_CHANGE_ADDED = FSL_CKOUT_CHANGE_NOT_SYMLINK + 1000,
/**
   Indicates that a file was removed from SCM management.
*/
FSL_CKOUT_CHANGE_REMOVED,
/**
   Indicates that a file is missing from the local checkout.
*/
FSL_CKOUT_CHANGE_MISSING,
/**
   Indicates that a file was renamed.
*/
FSL_CKOUT_CHANGE_RENAMED
};

typedef enum fsl_ckout_change_e fsl_ckout_change_e;

/**
   This is equivalent to calling fsl_vfile_changes_scan() with the
   arguments (f, -1, 0).

   @see fsl_ckout_changes_visit()
   @see fsl_vfile_changes_scan()
*/
FSL_EXPORT int fsl_ckout_changes_scan(fsl_cx * f);

/**
   A typedef for visitors of checkout status information via
   fsl_ckout_changes_visit(). Implementions will receive the
   last argument passed to fsl_ckout_changes_visit() as their
   first argument. The second argument indicates the type of change
   and the third holds the repository-relative name of the file.

   If changes is FSL_CKOUT_CHANGE_RENAMED then origName will hold
   the original name, else it will be NULL.

   Implementations must return 0 on success, non-zero on error. On
   error any looping performed by fsl_ckout_changes_visit() will
   stop and this function's result code will be returned.

   @see fsl_ckout_changes_visit()
*/
typedef int (*fsl_ckout_changes_f)(void * state, fsl_ckout_change_e change,
                                      char const * filename,
                                      char const * origName);

/**
   Compares the changes of f's local checkout against repository
   version vid (checkout version if vid is negative). For each
   change detected it calls visitor(state,...) to report the
   change.  If visitor() returns non-0, that code is returned from
   this function. If doChangeScan is true then
   fsl_ckout_changes_scan() is called by this function before
   iterating, otherwise it is assumed that the caller has called
   that or has otherwise ensured that the checkout db's vfile table
   has been populated.

   If the callback returns FSL_RC_BREAK, this function stops iteration
   and returns 0.

   Returns 0 on success.

   @see fsl_ckout_changes_scan()
*/
FSL_EXPORT int fsl_ckout_changes_visit( fsl_cx * f, fsl_id_t vid,
                                           bool doChangeScan,
                                           fsl_ckout_changes_f visitor,
                                           void * state );
/**
   A bitmask of flags for fsl_vfile_changes_scan().
*/
enum fsl_ckout_sig_e {
/**
   The empty flags set.
*/
FSL_VFILE_CKSIG_NONE = 0,

/**
   Non-file/non-link FS objects trigger an error.
*/
FSL_VFILE_CKSIG_ENOTFILE = 0x001,
/**
   Verify file content using hashing, regardless of whether or not
   file timestamps differ.
*/
FSL_VFILE_CKSIG_HASH = 0x002,
/**
   For unchanged or changed-by-merge files, set the mtime to last
   check-out time, as determined by fsl_mtime_of_manifest_file().
*/
FSL_VFILE_CKSIG_SETMTIME = 0x004,
/**
   Indicates that when populating the vfile table, it should be not be
   cleared of entries for other checkins. Normally we want to clear
   all versions except for the one we're working with, but at least
   a couple of use cases call for having multiple versions in vfile at
   once. Many algorithms generally assume only a single checkin's
   worth of state is in vfile and can get confused if that is not the
   case.
*/
FSL_VFILE_CKSIG_KEEP_OTHERS = 0x008,

/**
   If set and fsl_vfile_changes_scan() is passed a version other than
   the pre-call checkout version, it will, when finished, write the
   given version in the "checkout" setting of the ckout.vvar table,
   effectively switching the checkout to that version. It does not do
   this be default because it is sometimes necessary to have two
   versions in the vfile table at once and the operation doing so
   needs to control which version number is the current checkout.
*/
FSL_VFILE_CKSIG_WRITE_CKOUT_VERSION = 0x010
};

/**
    This function populates (if needed) the vfile table of f's
    checkout db for the given checkin version ID then compares files
    listed in it against files in the checkout directory, updating
    vfile's status for the current checkout version id as its goes. If
    vid is<=0 then the current checkout's RID is used in its place
    (note that 0 is the RID of an initial empty repository!).

    cksigFlags must be 0 or a bitmask of fsl_ckout_sig_e values.

    This is a relatively memory- and filesystem-intensive operation,
    and should not be performed more often than necessary. Many SCM
    algorithms rely on its state being correct, however, so it's
    generally better to err on the side of running it once too often
    rather than once too few times.

    Returns 0 on success, non-0 on error.

    BUG: this does not properly catch one particular corner-case
    change, where a file has been replaced by a same-named non-file
    (symlink or directory).
*/
FSL_EXPORT int fsl_vfile_changes_scan(fsl_cx * f, fsl_id_t vid,
                                      unsigned cksigFlags);

/**
   If f has an opened checkout which has local changes noted in its
   checkout db state (the vfile table), returns true, else returns
   false. Note that this function does not do the filesystem scan to
   check for changes, but checks only the db state. Use
   fsl_vfile_changes_scan() to perform the actual scan (noting that
   library-side APIs which update that state may also record
   individual changes or automatically run a scan).
*/
FSL_EXPORT bool fsl_ckout_has_changes(fsl_cx *f);

/**
   Callback type for use with fsl_checkin_queue_opt for alerting a
   client about exactly which files get enqueued/dequeued via
   fsl_checkin_enqueue() and fsl_checkin_dequeue().

   This function gets passed the checkout-relative name of the file
   being enqueued/dequeued and the client-provided state pointer which
   was passed to the relevant API. It must return 0 on success. If it
   returns non-0, the API on whose behalf this callback is invoked
   will propagate that error code back to the caller.

   The intent of this callback is simply to report changes to the
   client, not to perform validation. Thus such callbacks "really
   should not fail" unless, e.g., they encounter an OOM condition or
   some such. Any validation required by the client should be
   performed before calling fsl_checkin_enqueue()
   resp. fsl_checkin_dequeue().
*/
typedef int (*fsl_checkin_queue_f)(const char * filename, void * state);

/**
   Options object type used by fsl_checkin_enqueue() and
   fsl_checkin_dequeue().
*/
struct fsl_checkin_queue_opt {
  /**
     File or directory name to enqueue/dequeue to/from a pending
     checkin.
  */
  char const * filename;
  /**
     If true, filename (if not absolute) is interpreted as relative to
     the current working directory, else it is assumed to be relative
     to the top of the current checkout directory.
  */
  bool relativeToCwd;

  /**
     If not NULL then this->filename and this->relativeToCwd are
     IGNORED and any to-queue filename(s) is/are added from this
     container. It is an error (FSL_RC_MISUSE) to pass an empty bag.
     (Should that be FSL_RC_RANGE instead?)

     The bag is assumed to contain values from the vfile.id checkout
     db field, refering to one or more files which should be queued
     for the pending checkin. It is okay to pass IDs for unmodified
     files or to queue the same files multiple times. Unmodified files
     may be enqueued but will be ignored by the checkin process if, at
     the time the checkin is processed, they are still unmodified.
     Duplicated entries are simply ignored for the 2nd and subsequent
     inclusion.

     @see fsl_ckout_vfile_ids()
  */
  fsl_id_bag const * vfileIds;

  /**
     If true, fsl_vfile_changes_scan() is called to ensure that the
     filesystem and vfile tables agree. If the client code has called
     that function, or its equivalent, since any changes were made to
     the checkout then this may be set to false to speed up the
     enqueue process. This is only used by fsl_checkin_enqueue(), not
     fsl_checkin_dequeue().
  */
  bool scanForChanges;

  /**
     If true, only flagged-as-modified files will be enqueued by
     fsl_checkin_enqueue(). By and large, this should be set to
     true. Setting this to false is generally only intended/useful for
     testing.
  */
  bool onlyModifiedFiles;

  /**
     It not NULL, is pass passed the checkout-relative filename of
     each enqueued/dequeued file and this->callbackState. See the
     callback type's docs for more details.
  */
  fsl_checkin_queue_f callback;

  /**
     Opaque client-side state for use as the 2nd argument to
     this->callback.
  */
  void * callbackState;
};

/** Convenience typedef. */
typedef struct fsl_checkin_queue_opt fsl_checkin_queue_opt;

/** Initialized-with-defaults fsl_checkin_queue_opt structure, intended for
    const-copy initialization. */
#define fsl_checkin_queue_opt_empty_m { \
  NULL/*filename*/,true/*relativeToCwd*/,    \
  NULL/*vfileIds*/,                                 \
  true/*scanForChanges*/,true/*onlyModifiedFiles*/,   \
  NULL/*callback*/,NULL/*callbackState*/      \
}

/** Initialized-with-defaults fsl_checkin_queue_opt structure, intended for
    non-const copy initialization. */
extern const fsl_checkin_queue_opt fsl_checkin_queue_opt_empty;

/**
   Adds one or more files to f's list of "selected" files - those
   which should be included in the next commit (see
   fsl_checkin_commit()).

   Warning: if this function is not called before
   fsl_checkin_commit(), then fsl_checkin_commit() will select all
   modified, fsl_ckout_manage()'d, fsl_ckout_unmanage()'d, or renamed
   files by default.

   opt->filename must be a non-empty NUL-terminated string. The
   filename is canonicalized via fsl_ckout_filename_check() - see that
   function for the meaning of the opt->relativeToCwd parameter. To
   queue all modified files in a checkout, set opt->filename to ".",
   opt->relativeToCwd to false, and opt->onlyModifiedFiles to true.
   "Modified" includes any which are pending deletion, are
   newly-added, or for which a rename is pending.

   The resolved name must refer to either a single vfile.pathname
   value in the current vfile table or to a checkout-root-relative
   directory. All matching filenames which refer to modified files (as
   recorded in the vfile table) are queued up for the next commit.
   If opt->filename is NULL, empty, or ("." and opt->relativeToCwd is false)
   then all files in the vfile table are checked for changes.

   If opt->scanForChanges is true then fsl_vfile_changes_scan() is
   called before starting to ensure that the vfile entries are up to
   date. If the client app has "recently" run that (or its
   equivalent), that (slow) step can be skipped by setting
   opt->scanForChanges to false before calling this

   Note that after this returns, any given file may still be modified
   by the client before the commit takes place, and the changes on
   disk at the point of the fsl_checkin_commit() are the ones which
   get saved (or not).

   For each resolved entry which actually gets enqueued (i.e. was not
   already enqueued and which is marked as modified), opt->callback
   (if it is not NULL) is passed the checkout-relative file name and
   the opt->callbackState pointer.

   Returns 0 on success, FSL_RC_MISUSE if either pointer is NULL, or
   *zName is NUL. Returns FSL_RC_OOM on allocation error. It is not
   inherently an error for opt->filename to resolve to no queue-able
   entries. A client can check for that case, if needed, by assigning
   opt->callback and incrementing a counter in that callback. If the
   callback is never called, no queue-able entries were found.

   On error f's error state might (depending on the nature of the
   problem) contain more details.

   @see fsl_checkin_is_enqueued()
   @see fsl_checkin_dequeue()
   @see fsl_checkin_discard()
   @see fsl_checkin_commit()
*/
FSL_EXPORT int fsl_checkin_enqueue(fsl_cx * f,
                                   fsl_checkin_queue_opt const * opt);

/**
   The opposite of fsl_checkin_enqueue(), this opt->filename,
   which may resolve to a single name or a directory, from the checkin
   queue. Returns 0 on succes. This function does no validation on
   whether a given file(s) actually exist(s), it simply asks the
   internals to clean up matching strings from the checkout's vfile
   table. Specifically, it does not return an error if this operation
   finds no entries to dequeue.

   If opt->filename is empty or NULL then ALL files are unqueued from
   the pending checkin.

   If opt->relativeToCwd is true (non-0) then opt->filename is
   resolved based on the current directory, otherwise it is resolved
   based on the checkout's root directory.

   If opt->filename is not NULL or empty, this functions runs the
   given path through fsl_ckout_filename_check() and will fail if that
   function fails, propagating any error from that function. Ergo,
   opt->filename must refer to a path within the current checkout.

   @see fsl_checkin_enqueue()
   @see fsl_checkin_is_enqueued()
   @see fsl_checkin_discard()
   @see fsl_checkin_commit()
*/
FSL_EXPORT int fsl_checkin_dequeue(fsl_cx * f,
                                        fsl_checkin_queue_opt const * opt);

/**
   Returns true (non-0) if the file named by zName is in f's current
   file checkin queue.  If NO files are in the current selection
   queue then this routine assumes that ALL files are implicitely
   selected. As long as at least one file is enqueued (via
   fsl_checkin_enqueue()) then this function only returns true
   for files which have been explicitly enqueued.

   If relativeToCwd then zName is resolved based on the current
   directory, otherwise it assumed to be related to the checkout's
   root directory.

   This function returning true does not necessarily indicate that
   the file _will_ be checked in at the next commit. If the file has
   not been modified at commit-time then it will not be part of the
   commit.

   This function honors the fsl_cx_is_case_sensitive() setting
   when comparing names.

   Achtung: this does not resolve directory names like
   fsl_checkin_enqueue() and fsl_checkin_dequeue() do. It
   only works with file names.

   @see fsl_checkin_enqueue()
   @see fsl_checkin_dequeue()
   @see fsl_checkin_discard()
   @see fsl_checkin_commit()
*/
FSL_EXPORT bool fsl_checkin_is_enqueued(fsl_cx * f, char const * zName,
                                        bool relativeToCwd);

/**
   Discards any state accumulated for a pending checking,
   including any files queued via fsl_checkin_enqueue()
   and tags added via fsl_checkin_T_add().

   @see fsl_checkin_enqueue()
   @see fsl_checkin_dequeue()
   @see fsl_checkin_is_enqueued()
   @see fsl_checkin_commit()
   @see fsl_checkin_T_add()
*/
FSL_EXPORT void fsl_checkin_discard(fsl_cx * f);

/**
   Parameters for fsl_checkin_commit().

   Checkins are created in a multi-step process:

   - fsl_checkin_enqueue() queues up a file or directory for
   commit at the next commit.

   - fsl_checkin_dequeue() removes an entry, allowing
   UIs to toggle files in and out of a checkin before
   committing it.

   - fsl_checkin_is_enqueued() can be used to determine whether
   a given name is already enqueued or not.

   - fsl_checkin_T_add() can be used to T-cards (tags) to a
   deck. Branch tags are intended to be applied via the
   fsl_checkin_opt::branch member.

   - fsl_checkin_discard() can be used to cancel any pending file
   enqueuings, effectively cancelling a commit (which can be
   re-started by enqueuing another file).

   - fsl_checkin_commit() creates a checkin for the list of enqueued
   files (defaulting to all modified files in the checkout!). It
   takes an object of this type to specify a variety of parameters
   for the check.

   Note that this API uses the terms "enqueue" and "unqueue" rather
   than "add" and "remove" because those both have very specific
   (and much different) meanings in the overall SCM scheme.
*/
struct fsl_checkin_opt {
  /**
     The commit message. May not be empty - the library
     forbids empty checkin messages.
  */
  char const * message;

  /**
     The optional mime type for the message. Only set
     this if you know what you're doing.
  */
  char const * messageMimeType;

  /**
     The user name for the checkin. If NULL or empty, it defaults to
     fsl_cx_user_get(). If that is NULL, a FSL_RC_RANGE error is
     triggered.
  */
  char const * user;

  /**
     If not NULL, makes the checkin the start of a new branch with
     this name.
  */
  char const * branch;

  /**
     If this->branch is not NULL, this is applied as its "bgcolor"
     propagating property. If this->branch is NULL then this is
     applied as a one-time color tag to the checkin.

     It must be NULL, empty, or in a form usable by HTML/CSS,
     preferably \#RRGGBB form. Length-0 values are ignored (as if
     they were NULL).
  */
  char const * bgColor;

  /**
     If true, the checkin will be marked as private, otherwise it
     will be marked as private or public, depending on whether or
     not it inherits private content.
  */
  bool isPrivate;

  /**
     Whether or not to calculate an R-card. Doing so is very
     expensive (memory and I/O) but it adds another layer of
     consistency checking to manifest files. In practice, the R-card
     is somewhat superfluous and the cost of calculating it has
     proven painful on very large repositories. fossil(1) creates an
     R-card for all checkins but does not require that one be set
     when it reads a manifest.
  */
  bool calcRCard;

  /**
     Tells the checkin to close merged-in branches (merge type of
     0). INTEGRATE merges (type=-4) are always closed by a
     checkin. This does not apply to CHERRYPICK (type=-1) and
     BACKOUT (type=-2) merges.
  */
  bool integrate;

  /**
     If true, allow a file to be checked in if it contains
     fossil-style merge conflict markers, else fail if an attempt is
     made to commit any files with such markers.
  */
  bool allowMergeConflict;

  /**
     A hint to fsl_checkin_commit() about whether it needs to scan the
     checkout for changes. Set this to false ONLY if the calling code
     calls fsl_ckout_changes_scan() (or equivalent,
     e.g. fsl_vfile_changes_scan()) immediately before calling
     fsl_checkin_commit(). fsl_checkin_commit() requires a non-stale
     changes scan in order to function properly, but it's a
     computationally slow operation so the checkin process does not
     want to duplicate it if the application has recently done so.
  */
  bool scanForChanges;

  /**
     NOT YET IMPLEMENTED! TODO!

     If true, files which are removed from the SCM by this checkin
     should be removed from the filesystem.

     Reminder to self: when we do this, incorporate
     fsl_rm_empty_dirs().
  */
  bool rmRemovedFiles;

  /**
     Whether to allow (or try to force) a delta manifest or not. 0
     means no deltas allowed - it will generate a baseline
     manifest. Greater than 0 forces generation of a delta if
     possible (if one can be readily found) even if doing so would not
     save a notable amount of space. Less than 0 means to
     decide via some heuristics.

     A "readily available" baseline means either the current checkout
     is a baseline or has a baseline. In either case, we can use that
     as a baseline for a delta. i.e. a baseline "should" generally be
     available except on the initial checkin, which has neither a
     parent checkin nor a baseline.

     The current behaviour for "auto-detect" mode is: it will generate
     a delta if a baseline is "readily available" _and_ the repository
     has at least one delta already. Once it calculates a delta form,
     it calculates whether that form saves any appreciable
     space/overhead compared to whether a baseline manifest was
     generated. If so, it discards the delta and re-generates the
     manifest as a baseline. The "force" behaviour (deltaPolicy>0)
     bypasses the "is it too big?" test, and is only intended for
     testing, not real-life use.

     Caveat: if the repository has the "forbid-delta-manifests" set to
     a true value, this option is ignored: that setting takes
     priority. Similarly, it will not create a delta in a repository
     unless a delta has been "seen" in that repository before or this
     policy is set to >0. When a checkin is created with a delta
     manifest, that fact gets recorded in the repository's config
     table.

     Note that delta manifests have some advantages and may not
     actually save much (if any) repository space because the
     lower-level delta framework already compresses parent versions of
     artifacts tightly. For more information see:

     https://fossil-scm.org/home/doc/tip/www/delta-manifests.md
  */
  int deltaPolicy;

  /**
     Time of the checkin. If 0 or less, the time of the
     fsl_checkin_commit() call is used.
  */
  double julianTime;

  /**
     If this is not NULL then the committed manifest will include a
     tag which closes the branch. The value of this string will be
     the value of the "closed" tag, and the value may be an empty
     string. The intention is that this gets set to a comment about
     why the branch is closed, but it is in no way mandatory.
  */
  char const * closeBranch;

  /**
     Tells fsl_checkin_commit() to dump the generated manifest to
     this file. Intended only for debugging and testing. Checking in
     will fail if this file cannot be opened for writing.
  */
  char const * dumpManifestFile;
  /*
    fossil(1) has many more options. We might want to wrap some of
    it up in the "incremental" state (f->ckin.mf).

    TODOs:

    A callback mechanism which supports the user cancelling
    the checkin. It is (potentially) needed for ops like
    confirming the commit of CRNL-only changes.

    2021-03-09: we now have fsl_confirmer for this but currently no
    part of the checkin code needs a prompt.
  */
};

/**
   Empty-initialized fsl_checkin_opt instance, intended for use in
   const-copy constructing.
*/
#define fsl_checkin_opt_empty_m {               \
  NULL/*message*/,                            \
  NULL/*messageMimeType*/,                  \
  NULL/*user*/,                             \
  NULL/*branch*/,                           \
  NULL/*bgColor*/,                          \
  false/*isPrivate*/,                           \
  true/*calcRCard*/,                           \
  false/*integrate*/,                           \
  false/*allowMergeConflict*/,\
  true/*scanForChanges*/,\
  false/*rmRemovedFiles*/,\
  0/*deltaPolicy*/,                        \
  0.0/*julianTime*/,                        \
  NULL/*closeBranch*/,                      \
  NULL/*dumpManifestFile*/               \
}

/**
   Empty-initialized fsl_checkin_opt instance, intended for use in
   copy-constructing. It is important that clients copy this value
   (or fsl_checkin_opt_empty_m) to cleanly initialize their
   fsl_checkin_opt instances, as this may set default values which
   (e.g.) a memset() would not.
*/
FSL_EXPORT const fsl_checkin_opt fsl_checkin_opt_empty;

/**
   This creates and saves a "checkin manifest" for the current
   checkout.

   Its primary inputs is a list of files to commit. This list is
   provided by the client by calling fsl_checkin_enqueue() one or
   more times.  If no files are explicitely selected (enqueued) then
   it calculates which local files have changed vs the current
   checkout and selects all of those.

   Non-file inputs are provided via the opt parameter.

   On success, it returns 0 and...

   - If newRid is not NULL, it is assigned the new checkin's RID
   value.

   - If newUuid is not NULL, it is assigned the new checkin's UUID
   value. Ownership of the bytes is passed to the caller, who must
   eventually pass them to fsl_free() to free them.

   Note that the new RID and UUID can also be fetched afterwards by
   calling fsl_ckout_version_info().

   On error non-0 is returned and f's error state may (depending on
   the nature of the problem) contain details about the problem.
   Note, however, that any error codes returned here may have arrived
   from several layers down in the internals, and may not have a
   single specific interpretation here. When possible/practical, f's
   error state gets updated with a human-readable description of the
   problem.

   ACHTUNG: all pending checking state is cleaned if this function
   fails for any reason other than basic argument validation. This
   means any queued files or tags need to be re-applied if the client
   wants to try again. That is somewhat of a bummer, but this
   behaviour is the only way we can ensure that then the pending
   checkin state does not get garbled on a second use. When in doubt
   about the state, the client should call fsl_checkin_discard() to
   clear it before try to re-commit. (Potential TODO: add a
   success/fail state flag to the checkin state and only clean up on
   success? OTOH, since something in the state likely caused the
   problem, we might not want to do that.)

   This operation does all of its db-related work in a transaction, so
   it rolls back any db changes if it fails. To implement a "dry-run"
   mode, simply wrap this call in a transaction started on the
   fsl_cx_db_ckout() db handle (passing it to
   fsl_db_transaction_begin()), then, after this call, either cal;
   fsl_db_transaction_rollback() (to implement dry-run mode) or
   fsl_db_transaction_commit() (for "wet-run" mode). If this function
   returns non-0 due to anything more serious than basic argument
   validation, such a transaction will be in a roll-back state.

   Some of the more notable, potentially not obvious, error
   conditions:

   - Trying to commit against a closed leaf: FSL_RC_ACCESS. Doing so
   is not permitted by fossil(1), so we disallow it here.

   - An empty/NULL user name or commit message, or no files were
   selected which actually changed: FSL_RC_MISSING_INFO. In these
   cases f's error state describes the problem.

   - Some resource is not found (e.g. an expected RID/UUID could not
   be resolved): FSL_RC_NOT_FOUND. This would generally indicate
   some sort of data consistency problem. i.e. it's quite possibly
   very bad if this is returned.

   - If the checkin would result in no file-level changes vis-a-vis
   the current checkout, FSL_RC_NOOP is returned.

   BUGS:

   - It cannot currently properly distinguish a "no-op" commit, one in
   which no files were modified or only their permissions were
   modifed.

   @see fsl_checkin_enqueue()
   @see fsl_checkin_dequeue()
   @see fsl_checkin_discard()
   @see fsl_checkin_T_add()
*/
FSL_EXPORT int fsl_checkin_commit(fsl_cx * f, fsl_checkin_opt const * opt,
                                  fsl_id_t * newRid, fsl_uuid_str * newUuid);

/**
   Works like fsl_deck_T_add(), adding the given tag information to
   the pending checkin state. Returns 0 on success, non-0 on error. A
   checkin may, in principal, have any number of tags, and this may be
   called any number of times to add new tags to the pending
   commit. This list of tags gets cleared by a successful
   fsl_checkin_commit() or by fsl_checkin_discard(). Decks require
   that each tag be distinct from each other (none may compare
   equivalent), but that check is delayed until the deck is output
   into its final artifact form.

   @see fsl_checkin_enqueue()
   @see fsl_checkin_dequeue()
   @see fsl_checkin_commit()
   @see fsl_checkin_discard()
   @see fsl_checkin_T_add2()
*/
FSL_EXPORT int fsl_checkin_T_add( fsl_cx * f, fsl_tagtype_e tagType,
                                   fsl_uuid_cstr uuid, char const * name,
                                   char const * value);

/**
   Works identically to fsl_checkin_T_add() except that it takes its
   argument in the form of a T-card object.

   On success ownership of t is passed to mf. On error (see
   fsl_deck_T_add()) ownership is not modified.

   Results are undefined if either argument is NULL or improperly
   initialized.
*/
FSL_EXPORT int fsl_checkin_T_add2( fsl_cx * f, fsl_card_T * t );

/**
   Clears all contents from f's checkout database, including the vfile
   table, vmerge table, and some of the vvar table. The tables are
   left intact. Returns 0 on success, non-0 if f has no checkout or for
   a database error.
 */
FSL_EXPORT int fsl_ckout_clear_db(fsl_cx *f);

/**
   Returns the base name of the current platform's checkout database
   file. That is "_FOSSIL_" on Windows and ".fslckout" everywhere
   else. The returned bytes are static.

   TODO: an API which takes a dir name and looks for either name
*/
FSL_EXPORT char const *fsl_preferred_ckout_db_name();  

/**
   File-overwrite policy values for use with fsl_ckup_opt and friends.
*/
enum fsl_file_overwrite_policy_e {
/** Indicates that an error should be triggered if a file would be
    overwritten. */
FSL_OVERWRITE_ERROR = 0,
/**
   Indicates that files should always be overwritten by 
*/
FSL_OVERWRITE_ALWAYS,
/**
   Indicates that files should never be overwritten, and silently
   skipped over. This is almost never what one wants to do.
*/
FSL_OVERWRITE_NEVER
};
typedef enum fsl_file_overwrite_policy_e fsl_file_overwrite_policy_e;

/**
   State values for use with fsl_ckup_state::fileRmInfo.
*/
enum fsl_ckup_rm_state_e {
/**
   Indicates that the file was not removed in a given checkout.
   Guaranteed to have the value 0 so that it is treated as boolean
   false. No other entries in this enum have well-defined values.
*/
FSL_CKUP_RM_NOT = 0,
/**
   Indicates that a file was removed from a checkout but kept
   in the filesystem because it was locally modified.
*/
FSL_CKUP_RM_KEPT,
/**
   Indicates that a file was removed from a checkout and the
   filesystem, with the caveat that failed attempts to remove from the
   filesystem are ignored for Reasons but will be reported as if the
   unlink worked.
*/
FSL_CKUP_RM
};
typedef enum fsl_ckup_rm_state_e fsl_ckup_rm_state_e;

/**
  Under construction. Work in progress...

  Options for "opening" a fossil repository database. That is,
  creating a new fossil checkout database and populating its schema,
  _without_ checking out any files. (That latter part is up for
  reconsideration and this API might change in the future to check
  out files after creating/opening the db.)
*/
struct fsl_repo_open_ckout_opt {
  /**
     Name of the target directory, which must already exist. May be
     relative, e.g. ".". The repo-open operation will chdir to this
     directory for the duration of the operation. May be NULL, in
     which case the current directory is assumed and no chdir is
     performed.
  */
  char const * targetDir;

  /**
     The filename, with no directory components, of the desired
     checkout db name. For the time being, always leave this NULL and
     let the library decide. It "might" (but probably won't) be
     interesting at some point to allow the client to specify a
     different name (noting that that would be directly incompatible
     with fossil(1)).
  */
  char const * ckoutDbFile;

  /**
     Policy for how to handle overwrites of files extracted from a
     newly-opened checkout.

     Potential TODO: replace this with a fsl_confirmer, though that
     currently seems like overkill for this particular case.
  */
  fsl_file_overwrite_policy_e fileOverwritePolicy;

  /**
     fsl_repo_open_ckout() installs the fossil checkout schema. If
     this is true it will forcibly replace any existing relevant
     schema components in the checkout db, otherwise it will fail when
     it tries to overwrite an existing schema and cannot.
  */
  bool dbOverwritePolicy;

  /**
     Of true, the checkout-open process will look for an opened
     checkout in the target directory and its parents (recursively)
     and fail with FSL_RC_ALREADY_EXISTS if one is found.
  */
  bool checkForOpenedCkout;
};
typedef struct fsl_repo_open_ckout_opt fsl_repo_open_ckout_opt;

/**
  Empty-initialized fsl_repo_open_ckout_opt const-copy constructer.
*/
#define fsl_repo_open_ckout_opt_m { \
  NULL/*targetDir*/, NULL/*ckoutDbFile*/, \
  FSL_OVERWRITE_ERROR/*fileOverwritePolicy*/, \
  false/*dbOverwritePolicy*/,               \
  -1/*checkForOpenedCkout*/              \
}

/**
  Empty-initialised fsl_repo_open_ckout_opt instance. Clients should copy
  this value (or fsl_repo_open_ckout_opt_empty_m) to initialise
  fsl_repo_open_ckout_opt instances for sane default values.
*/
FSL_EXPORT const fsl_repo_open_ckout_opt fsl_repo_open_ckout_opt_empty;

/**
   Work in progress...

   Opens a checkout db for use with the currently-connected repository
   or creates a new one. If opening an existing one, it gets "stolen"
   from any repository it might have been previously mapped to.

   - Requires that f have an opened repository db and no opened
     checkout. Returns FSL_RC_NOT_A_REPO if no repo is opened and
     FSL_RC_MISUSE if a checkout *is* opened.

   - Creates/re-uses a .fslckout DB in the dir opt->targetDir. The
     directory must be NULL or already exist, else FSL_RC_NOT_FOUND is
     returned. If opt->dbOverwritePolicy is false then it fails with
     FSL_RC_ALREADY_EXISTS if that directory already contains a
     checkout db.

   Note that this does not extract any SCM'd files from the
   repository, it only opens (and possibly creates) the checkout
   database.

   Pending:

   - If opening an existing checkout db for a different repo then
   delete the STASH and UNDO entries, as they're not valid for a
   different repo.
*/
FSL_EXPORT int fsl_repo_open_ckout( fsl_cx * f, fsl_repo_open_ckout_opt const * opt );

typedef struct fsl_ckup_state fsl_ckup_state;
/**
   A callback type for use with fsl_ckup_state.  It gets called via
   fsl_repo_ckout() and fsl_ckout_update() to report progress of the
   extraction process. It gets called after one of those functions has
   successfully extracted a file or skipped over it because the file
   existed and the checkout options specified to leave existing files
   in place. It must return 0 on success, and non-0 will end the
   extraction process, propagating that result code back to the
   caller. If this callback fails, the checkout's contents may be left
   in an undefined state, with some files updated and others not.  All
   database-side data will be consistent (the transaction is rolled
   back) but filesystem-side changes may not be.
*/
typedef int (*fsl_ckup_f)( fsl_ckup_state const * cState );

/**
   This enum lists the various types of individual file change states
   which can happen during a checkout or update.
*/
enum fsl_ckup_fchange_e {
/** Sentinel value. */
FSL_CKUP_FCHANGE_INVALID = -1,
/** Was unchanged between the previous and updated-to version,
    so no change was made to the on-disk file. This is the
    only entry in the enum which is guaranteed to have a specific
    value: 0, so that it can be used as a boolean false. */
FSL_CKUP_FCHANGE_NONE = 0,
/**
   Added to SCM in the updated-to version.
*/
FSL_CKUP_FCHANGE_ADDED,
/**
   Added to SCM in the current checkout version and carried over into
   the updated-to version.
*/
FSL_CKUP_FCHANGE_ADD_PROPAGATED,
/** Removed from SCM in the updated-to to version
    OR in the checked-out version but not yet commited.
    a.k.a. it became "unmanaged."


   Do we need to differentiate between those cases?
*/
FSL_CKUP_FCHANGE_RM,
/**
   Removed from the checked-out version but not yet commited,
   so was carried over to the updated-to version.
*/
FSL_CKUP_FCHANGE_RM_PROPAGATED,
/** Updated or replaced without a merge by the checkout/update
    process. */
FSL_CKUP_FCHANGE_UPDATED,
/** Merge was not performed because at least one of the inputs appears
    to be binary. The updated-to version overwrites the previous
    version in this case.
*/
FSL_CKUP_FCHANGE_UPDATED_BINARY,
/** Updated with a merge by the update process. */
FSL_CKUP_FCHANGE_MERGED,
/** Special case of FSL_CKUP_FCHANGE_UPDATED. Merge was performed
    and conflicts were detected. The newly-updated file will contain
    conflict markers.

    @see fsl_buffer_contains_merge_marker()
*/
FSL_CKUP_FCHANGE_CONFLICT_MERGED,
/** Added in the current checkout but also contained in the
    updated-to version. The local copy takes precedence.
*/
FSL_CKUP_FCHANGE_CONFLICT_ADDED,
/**
   Added by the updated-to version but a local unmanaged copy exists.
   The local copy is overwritten, per historical fossil(1) convention
   (noting that fossil has undo support to allow one to avoid loss of
   such a file's contents).

   TODO: use confirmer here to ask user whether to overwrite.
*/
FSL_CKUP_FCHANGE_CONFLICT_ADDED_UNMANAGED,
/** Edited locally but removed from updated-to version. Local
    edits will be left in the checkout tree. */
FSL_CKUP_FCHANGE_CONFLICT_RM,
/** Cannot merge if one or both of the update/updating verions of a
    file is a symlink The updated-to version overwrites the previous
    version in this case.

    We probably need a better name for this.
*/
FSL_CKUP_FCHANGE_CONFLICT_SYMLINK,
/** File was renamed in the updated-to version. If a file is both
    modified and renamed, it is flagged as renamed instead
    of modified. */
FSL_CKUP_FCHANGE_RENAMED,
/** Locally modified. This state appears only when
    "updating" a checkout to the same version. */
FSL_CKUP_FCHANGE_EDITED
};
typedef enum fsl_ckup_fchange_e fsl_ckup_fchange_e;

/**
   State to be passed to fsl_ckup_f() implementations via
   calls to fsl_repo_ckout() and fsl_ckout_update().
*/
struct fsl_ckup_state {
  /**
     The core SCM state for the just-extracted file. Note that its
     content member will be NULL: the content is not passed on via
     this interface because it is only loaded for files which require
     overwriting.

     An update process may synthesize content for extractState->fCard
     which do not 100% reflect the file on disk. Of primary note here:

     1) fCard->uuid will refer to the hash of the updated-to
     version, as opposed to the hash of the on-disk file (which may
     differ due to having local edits merged in). 

     2) For the update process, fCard->priorName will be NULL unless
     the file was renamed between the original and updated-to
     versions, in which case priorName will refer to the original
     version's name.
  */
  fsl_repo_extract_state const * extractState;
  /**
     Optional client-dependent state for use in the
     fsl_ckup_f() callback.
  */
  void * callbackState;

  /**
     Vaguely describes the type of change the current call into
     the fsl_ckup_f() represents. The full range of values is
     not valid for all operations. Specifically:

     Checkout only uses:

     FSL_CKUP_FCHANGE_NONE
     FSL_CKUP_FCHANGE_UPDATED
     FSL_CKUP_FCHANGE_RM

     For update operatios all (or most) values are potentially
     possible.

     If this has a value of FSL_CKUP_FCHANGE_RM,
     this->fileRmInfo will provide a bit more detail.
  */
  fsl_ckup_fchange_e fileChangeType;

  /**
     Indicates whether the file was removed by the process:

     - FSL_CKUP_RM_NOT = Was not removed.

     - FSL_CKUP_RM_KEPT = Was removed from the checked-out version but
     left in the filesystem because the confirmer said to.

     - FSL_CKUP_RM = Was removed from the checkout and the filesystem.

     When this->dryRun is true, this specifies whether the file would
     have been removed.
  */
  fsl_ckup_rm_state_e fileRmInfo;
  
  /**
     If fsl_repo_ckout()'s or fsl_ckout_update()'s options specified
     that the mtime should be set on each updated file, this holds
     that time. If the file existed and was not overwritten, it is set
     to that file's time. Else it is set to the current time (which
     may differ by a small fraction of a second from the file-write
     time because we avoid stat()'ing it again after writing). If
     this->fileRmInfo indicates that a file was removed, this might
     (depending on availability of the file in the filesystem at the
     time) be set to 0.

     When running in dry-run mode, this value may be 0, as we may not
     have a file in place which we can stat() to get it, nor a db
     entry from which to fetch it.
  */
  fsl_time_t mtime;

  /**
     The size of the extracted file, in bytes. If the file was removed
     from the filesystem (or removal was at least attempted) then this
     is set to -1.
  */
  fsl_int_t size;

  /**
     True if the current checkout/update is running in dry-run mode,
     else false. Dry-run mode has 
  */
  bool dryRun;
};

/**
   UNDER CONSTRUCTION.

   Options for use with fsl_repo_ckout() and fsl_ckout_update(). i.e.
   for checkout and update operations.
*/
struct fsl_ckup_opt {
  /**
     The version of the repostitory to check out or update. This must
     be the blob.rid of a checkin artifact.
  */
  fsl_id_t checkinRid;

  /**
     Gets called once per checked-out or updated file, passed a
     fsl_ckup_state instance with information about the
     checked-out file and related metadata. May be NULL.
  */
  fsl_ckup_f callback;

  /**
     State to be passed to this->callback via the
     fsl_ckup_state::callbackState member.
   */
  void * callbackState;

  /**
     An optional "confirmer" for answering questions about file
     overwrites and deletions posed by the checkout process.
     By default this confirmer of the associated fsl_cx instance
     is used.

     Caveats:

     - This is not currently used by the update process, only
     checkout.

     - If this->setMTime is true, the mtime is NOT set for any files
     which already exist and are skipped due to the confirmer saying
     to leave them in place.

     - Similarly, if the confirmer says to never overwrite files,
     permissions on existing files are not modified. fsl_repo_ckout()
     does not (re)write unmodified files, and thus may leave such
     files with different permissions. That's on the to-fix list.
  */
  fsl_confirmer confirmer;
  
  /**
     If true, the checkout/update processes will calculate the
     (synthetic) mtime of each extracted file and set its mtime. This
     is a relatively expensive operation which calculates the
     "effective mtime" of each file by calculating it: Fossil does not
     record file timestamps, instead treating files as if they had the
     timestamp of the most recent checkin in which they were added or
     modified.

     It's generally a good idea to let the update process stamp the
     _current_ time on modified files, in order to avoid any hiccups
     with build processes which rely on accurate times
     (e.g. Makefiles). When doing a clean checkout, it's often
     interesting to see the "original" times, though.
  */
  bool setMtime;

  /**
     A hint to fsl_repo_ckout() and fsl_ckout_update() about whether
     it needs to scan the checkout for changes. Set this to false ONLY
     if the calling code calls fsl_ckout_changes_scan() (or
     equivalent, e.g. fsl_vfile_changes_scan()) immediately before
     calling fsl_repo_ckout() or fsl_ckout_update(), as both require a
     non-stale changes scan in order to function properly.
  */
  bool scanForChanges;

  /**
     If true, the extraction process will "go through the motions" but
     will not write any files to disk. It will perform I/O such as
     stat()'ing to see, e.g., if it would have needed to overwrite a
     file.
  */
  bool dryRun;
};
typedef struct fsl_ckup_opt fsl_ckup_opt;

/**
  Empty-initialized fsl_ckup_opt const-copy constructor.
*/
#define fsl_ckup_opt_m {\
  -1/*checkinRid*/, NULL/*callback*/, NULL/*callbackState*/,  \
  fsl_confirmer_empty_m/*confirmer*/,\
  false/*setMtime*/, true/*scanForChanges*/,false/*dryRun*/ \
}

/**
  Empty-initialised fsl_ckup_opt instance. Clients should copy
  this value (or fsl_ckup_opt_empty_m) to initialise
  fsl_ckup_opt instances for sane default values.
*/
FSL_EXPORT const fsl_ckup_opt fsl_ckup_opt_empty;

/**
   A fsl_repo_extract() proxy which extracts the contents of the
   repository version specified by opt->checkinRid to the root
   directory of f's currently-opened checkout. i.e. it performs a
   "checkout" operation.

   For each extracted entry, cOpt->callback (if not NULL) will be
   passed a (fsl_ckup_state const*) which contains a pointer
   to the fsl_repo_extract_state and some additional metadata
   regarding the extraction. The value of cOpt->callbackState will be
   set as the callbackState member of that fsl_ckup_state
   struct, so that the client has a way of passing around app-specific
   state to that callback.

   After successful completion, the process will report (see below)
   any files which were part of the previous checkout version but are
   not part of the current version, optionally removing them from the
   filesystem (depending on the value of opt->rmMissingPolicy). It
   will IGNORE ANY DELETION FAILURE of files it attempts to
   delete. The reason it does not fail on removal error is because
   doing so would require rolling back the transaction, effectively
   undoing the checkout, but it cannot roll back any prior deletions
   which succeeded. Similarly, after all file removal is complete, it
   attempts to remove any now-empty directories left over by that
   process, also silently ignoring any errors. If the cOpt->dryRun
   option is specified, it will "go through the motions" of removing
   files but will not actually attempt filesystem removal. For
   purposes of the callback, however, it will report deletions as
   having happened (but will also set the dryRun flag on the object
   passed to the callback).

   After unpacking the SCM-side files, it may write out one or more
   manifest files, as described for fsl_ckout_manifest_write(), if the
   'manifest' config setting says to do so.

   As part of the file-removal process, AFTER all "existing" files are
   processed, it calls cOpt->callback() (if not NULL) for each removed
   file, noting the following peculiarities in the
   fsl_ckup_state object which is passed to it for those
   calls:

   - It is called after the processing of "existing" files. Thus the
   file names passed during this step may appear "out of order" with
   regards to the others (which are guaranteed to be passed in lexical
   order, though whether it is case-sensitive or not depends on the
   repository's case-sensitivity setting).

   - fileRmInfo will indicate that the file was removed from the
   checkout, and whether it was actually removed or retained in the
   filesystem. This will indicate filesystem-level removal even when
   in dry-run mode, though in that case no filesystem-level removal is
   actually attempted.

   - extractState->fileRid will refer to the file's blob's RID for the
   previous checkout version.

   - extractState->content will be NULL.

   - extractState->callbackState will be NULL. 

   - extractState->fCard will refer to the pre-removal state of the
   file. i.e. the state as it was in the checkout prior to this
   function being called.

   Returns 0 on success. Returns FSL_RC_NOT_A_REPO if f has no opened
   repo, FSL_RC_NOT_A_CKOUT if no checkout is opened. If
   cOpt->callback is not NULL and returns a non-0 result code,
   extraction ends and that result is returned. If it returns non-0 at
   any point after basic argument validation, it rolls back all
   changes or sets the current transaction stack into a rollback
   state.

   @see fsl_repo_ckout_open()
*/
FSL_EXPORT int fsl_repo_ckout(fsl_cx * f, fsl_ckup_opt const * cOpt);


/**
   UNDER CONSTRUCTION.

   Performs an "update" operation on f's currenly-opened
   checkout. Performing an update is similar to performing a checkout,
   the primary difference being that an update will merge local file
   modifications into any newly-updated files, whereas a checkout will
   overwrite them.

   TODO?: fossil(1)'s update permits a list of files, in which case it
   behaves differently: it updates the given files to the version
   requested but leaves the checkout at its current version. To be
   able to implement that we either need clients to call this in a
   loop, changing opt->filename on each call (like how we do
   fsl_ckout_manage()) or we need a way for them to pass on the list
   of files/dir in the opt object.

   @see fsl_repo_ckout().
*/
FSL_EXPORT int fsl_ckout_update(fsl_cx * f, fsl_ckup_opt const *opt);


/**
   Tries to calculate a version to update the current checkout version
   to, preferring the tip of the current checkout's branch.

   On success, 0 is returned and *outRid is set to the calculated RID,
   which may be 0, indicating that no errors were encountered but no
   version could be calculated.

   On error, non-0 is returned, outRid is not modified, and f's error
   state is updated.

   Returns FSL_RC_NOT_A_CKOUT if f has no checkout opened and
   FSL_RC_NOT_A_REPO if no repo is opened.

   If it calculates that there are multiple viable descendants it
   returns FSL_RC_AMBIGUOUS and f's error state will contain a list of
   the UUIDs (or UUID prefixes) of those descendants.

   Sidebar: to get the absolute latest version, irrespective of the
   branch, use fsl_sym_to_rid() to resolve the symbolic name "tip".
*/
FSL_EXPORT int fsl_ckout_calc_update_version(fsl_cx * f, fsl_id_t * outRid);

/**
   Bitmask used by fsl_ckout_manifest_setting() and
   fsl_ckout_manifest_write().
*/
enum fsl_cx_manifest_mask_e {
FSL_MANIFEST_MAIN = 0x001,
FSL_MANIFEST_UUID = 0x010,
FSL_MANIFEST_TAGS = 0x100
};
typedef enum fsl_cx_manifest_mask_e fsl_cx_manifest_mask_e;

/**
   Returns a bitmask representing which manifest files, if any, will
   be written when opening or updating a checkout directory, as
   specified by the repository's 'manifest' configuration setting, and
   sets *m to a bitmask indicating which of those are enabled. It
   first checks for a versioned setting then, if no versioned setting
   is found, a repository-level setting.

   A truthy setting value (1, "on", "true") means to write the
   manifest and manifest.uuid files. A string with any of the letters
   'r', 'u', or 't' means to write the [r]aw, [u]uid, and/or [t]ags
   file(s), respectively.

   If the manifest setting is falsy or not set, *m is set to 0, else
   *m is set to a bitmask representing which file(s) are considered to
   be auto-generated for this repository:

   - FSL_MANIFEST_MAIN = manifest
   - FSL_MANIFEST_UUID = manifest.uuid
   - FSL_MANIFEST_TAGS = manifest.tags

   Any db-related or allocation errors while trying to fetch the
   setting are silently ignored.

   For performance's sake, since this is potentially called often from
   fsl_reserved_fn_check(), this setting is currently cached by this
   routine (in the fsl_cx object), but that ignores the fact that the
   manifest setting can be modified at any time, either in a versioned
   setting file or the repository db, and may be modified from outside
   the library. There's a tiny back-door for working around that: if m
   is NULL, the cache will be flushed and no other work will be
   performed. Thus the following approach can be used to force a fresh
   check for that setting:

   @code
   fsl_ckout_manifest_setting(f, NULL); // clears caches, does nothing else
   fsl_ckout_manifest_setting(f, &myInt); // loads/caches the setting
   @endcode
*/
FSL_EXPORT void fsl_ckout_manifest_setting(fsl_cx *f, int *m);

/**
   Might write out the files manifest, manifest.uuid, and/or
   manifest.tags for the current checkout to the the checkout's root
   directory. The 2nd-4th arguments are interpreted as follows:

   0: Do not write that file.

   >0: Always write that file.

   <0: Use the value of the "manifest" config setting (see
   fsl_ckout_manifest_setting()) to determine whether or not to write
   that file.

   As each file is written, its mtime is set to that of the checkout
   version. (Forewarning: that behaviour may change if it proves to be
   problematic vis a vis build processes.)

   Returns 0 on success, non-0 on error:

   - FSL_RC_NOT_A_CKOUT if no checkout is opened.

   - FSL_RC_RANGE if the current checkout RID is 0 (indicating a fresh,
   empty repository).

   - Various potential DB/IO-related error codes.

   If the final argument is not NULL then it will be updated to
   contain a bitmask representing which files, if any, were written:
   see fsl_ckout_manifest_setting() for the values. It is updated
   regardless of success or failure and will indicate which file(s)
   was/were written before the error was triggered.

   Each file implied by the various manifest settings which is NOT
   written by this routine and is also not part of the current
   checkout (i.e. not listed in the vfile table) will be removed from
   disk, but a failure while attempting to do so will be silently
   ignored.

   @see fsl_repo_manifest_write()
*/
FSL_EXPORT int fsl_ckout_manifest_write(fsl_cx *f,
                                        int manifest,
                                        int manifestUuid,
                                        int manifestTags,
                                        int *wroteWhat );

/**
   Returns true if f has an opened checkout and the given absolute
   path is rooted in that checkout, else false. As a special case, it
   returns false if the path _is_ the checkout root unless zAbsPath
   has a trailing slash. (The checkout root is always stored with a
   trailing slash because that simplifies its internal usage.)

   Note that this is strictly a string comparison, not a
   filesystem-level operation.
*/
FSL_EXPORT bool fsl_is_rooted_in_ckout(fsl_cx *f, char const *zAbsPath);

/**
   Works like fsl_is_rooted_in_ckout() except that it returns 0 on
   success, and on error updates f with a description of the problem
   and returns non-0: FSL_RC_RANGE or (if updating the error state
   fails) FSL_RC_OOM.
 */
FSL_EXPORT int fsl_is_rooted_in_ckout2(fsl_cx *f, char const *zAbsPath);

/**
   Change-type values for use with fsl_ckout_revert_f() callbacks.
*/
enum fsl_ckout_revert_e {
/** Sentinel value. */
FSL_REVERT_NONE = 0,
/**
   File was previously queued for addition but unqueued
   by the revert process.
*/
FSL_REVERT_UNMANAGE,
/**
   File was previously queued for removal but unqueued by the revert
   process. If the file's contents or permissions were also reverted
   then the file is reported as FSL_REVERT_PERMISSIONS or
   FSL_REVERT_CONTENTS instead.
*/
FSL_REVERT_REMOVE,
/**
   File was previously scheduled to be renamed, but the rename was
   reverted. The name reported to the callback is the original one.
   If a file was both modified and renamed, it will be flagged as
   renamed instead of modified, for consistency with the usage of
   fsl_ckup_fchange_e's FSL_CKUP_FCHANGE_RENAMED.

   FIXME: this does not mean that the file on disk was actually
   renamed (if needed). That is TODO, pending addition of code to
   perform renames.
*/
FSL_REVERT_RENAME,
/** File's permissions (only) were reverted. */
FSL_REVERT_PERMISSIONS,
/**
   File's contents reverted. This value trumps any others in this
   enum. Thus if a file's permissions and contents were reverted,
   or it was un-renamed and its contents reverted, it will be
   reported using this enum entry.
*/
FSL_REVERT_CONTENTS
};
typedef enum fsl_ckout_revert_e fsl_ckout_revert_e;
/**
   Callback type for use with fsl_ckout_revert(). For each reverted
   file it gets passed the checkout-relative filename, type of change,
   and the callback state pointer which was passed to
   fsl_ckout_revert(). If it returns non-0, the revert process will
   end in an error and that code will be propagated back to the
   caller.  In such cases, any files reverted up until that point will
   still be reverted on disk but the reversion in the database will be
   rolled back. A change scan (e.g. fsl_ckout_changes_scan()) will
   restore balance to that equation, but these callbacks should only
   return non-0 in for catastrophic failure.
*/
typedef int (*fsl_ckout_revert_f)( char const *zFilename,
                                   fsl_ckout_revert_e changeType,
                                   void * callbackState );

/**
   Options for passing to fsl_ckout_revert().
*/
struct fsl_ckout_revert_opt {
  /**
     File or directory name to revert. See also this->vfileIds.
  */
  char const * filename;
  /**
     An alternative to assigning this->filename is to point
     this->vfileIds to a bag of vfile.id values. If this member is not
     NULL, fsl_ckout_revert() will ignore this->filename.

     @see fsl_filename_to_vfile_ids()
  */
  fsl_id_bag const * vfileIds;
  /**
     Interpret filename as relative to cwd if true, else relative to
     the current checkout root. This is ignored when this->vfileIds is
     not NULL.
  */
  bool relativeToCwd;
  /**
     If true, fsl_vfile_changes_scan() is called to ensure that
     the filesystem and vfile tables agree. If the client code has
     called that function, or its equivalent, since any changes were
     made to the checkout then this may be set to false to speed up
     the revert process.
  */
  bool scanForChanges;
  /**
     Optional callback to notify the client of what gets reverted.
  */
  fsl_ckout_revert_f callback;
  /**
     State for this->callback.
  */
  void * callbackState;
};
typedef struct fsl_ckout_revert_opt fsl_ckout_revert_opt;
/**
   Initialized-with-defaults fsl_ckout_revert_opt instance,
   intended for use in const-copy initialization.
*/
#define fsl_ckout_revert_opt_empty_m { \
  NULL/*filename*/,NULL/*vfileIds*/,true/*relativeToCwd*/,true/*scanForChanges*/, \
  NULL/*callback*/,NULL/*callbackState*/      \
}
/**
   Initialized-with-defaults fsl_ckout_revert_opt instance,
   intended for use in non-const copy initialization.
*/
FSL_EXPORT const fsl_ckout_revert_opt fsl_ckout_revert_opt_empty;

/**
   Reverts changes to checked-out files, replacing their
   on-disk versions with the current checkout's version.

   If zFilename refers to a directory, all managed files under that
   directory are reverted (if modified). If zFilename is NULL or
   empty, all modifications in the current checkout are reverted.

   If a file has been added but not yet committed, the add
   is un-queued but the file is otherwise untouched. If the
   file has been queued for removal, this removes it from
   that queue as well as restores its contents.

   If a rename is pending for the given filename, the name may match
   either its original name or new name. Whether or not that will
   actually work when file A is renamed to B and file C is renamed to
   A is anyone's guess. (Noting that (fossil mv) won't allow that
   situation to exist in the vfile table for a single checkout
   version, so it seems safe enough.)

   If the 4th argument is not NULL then it is called for each revert
   (_after_ the revert happens) to report what was done with that
   file. It gets passed the checkout-relative name of each reverted
   file. The 5th argument is not interpreted by this function but is
   passed on as-is as the final argument to the callback. If the
   callback returns non-0, the revert process is cancelled, any
   pending transaction is set to roll back, and that error code is
   returned. Note that cancelling a revert mid-process will leave file
   changes made by the revert so far in place, and thus the checkout
   db and filesystem will be in an inconsistent state until
   fsl_vfile_changes_scan() (or equivalent) is called to restore
   balance to the world.

   Files which are not actually reverted because their contents or
   permissions were not modified on disk are not reported to the
   callback unless the reversion was the un-queuing of an ADD or
   REMOVE operation.

   Returns 0 on success, any number of non-0 results on error.
*/
FSL_EXPORT int fsl_ckout_revert( fsl_cx * f,
                                 fsl_ckout_revert_opt const * opt );

/**
   Expects f to have an opened checkout and zName to be the name of
   an entry in the vfile table where vfile.vid == vid. If vid<=0 then
   the current checkout RID is used. This function does not do any
   path resolution or normalization on zName and checks only for an
   exact match (honoring f's case-sensitivity setting - see
   fsl_cx_case_sensitive_set()).

   On success it returns 0 and assigns *vfid to the vfile.id value of
   the matching file.  If no match is found, 0 is returned and *vfile
   is set to 0. Returns FSL_RC_NOT_A_CKOUT if no checkout is opened,
   FSL_RC_RANGE if zName is not a simple path (see
   fsl_is_simple_pathname()), and any number of codes for db-related
   errors.

   This function matches only vfile.pathname, not vfile.origname,
   because it is possible for a given name to be in both fields (in
   different records) at the same time.
*/
FSL_EXPORT int fsl_filename_to_vfile_id( fsl_cx * f, fsl_id_t vid,
                                         char const * zName,
                                         fsl_id_t * vfid );

/**
   Searches the vfile table where vfile.vid=vid for a name which
   matches zName or all vfile entries found under a subdirectory named
   zName (with no trailing slash). zName must be relative to the
   checkout root. As a special case, if zName is NULL, empty, or "."
   then all files in vfile with the given vid are selected. For each
   entry it finds, it adds the vfile.id to dest. If vid<=0 then the
   current checkout RID is used.

   If changedOnly is true then only entries which have been marked in
   the vfile table as having some sort of change are included, so if
   true then fsl_ckout_changes_scan() (or equivalent) must have been
   "recently" called to ensure that state is up to do.

   This search honors the context-level case-sensitivity setting (see
   fsl_cx_case_sensitive_set()).

   Returns 0 on success. Not finding anything is not treated as an
   error, though we could arguably return FSL_RC_NOT_FOUND for the
   cases which use this function. In order to determine whether or
   not any results were found, compare dest->entryCount before and
   after calling this.

   This function matches only vfile.pathname, not vfile.origname,
   because it is possible for a given name to be in both fields (in
   different records) at the same time.

   @see fsl_ckout_vfile_ids()
*/
FSL_EXPORT int fsl_filename_to_vfile_ids( fsl_cx * f, fsl_id_t vid,
                                          fsl_id_bag * dest,
                                          char const * zName,
                                          bool changedOnly);

/**
   This is a variant of fsl_filename_to_vfile_ids() which accepts
   filenames in a more flexible form than that routine. This routine
   works exactly like that one except for the following differences:

   1) The given filename and the relativeToCwd arguments are passed to
   by fsl_ckout_filename_check() to canonicalize the name and ensure
   that it points to someplace within f's current checkout.

   2) Because of (1), zName may not be NULL or empty. To fetch all of
   the vfile IDs for the current checkout, pass a zName of "."  and
   relativeToCwd=false.

   Returns 0 on success, FSL_RC_MISUSE if zName is NULL or empty,
   FSL_RC_OOM on allocation error, FSL_RC_NOT_A_CKOUT if f has no
   opened checkout.
*/
FSL_EXPORT int fsl_ckout_vfile_ids( fsl_cx * f, fsl_id_t vid,
                                    fsl_id_bag * dest, char const * zName,
                                    bool relativeToCwd, bool changedOnly );


/**
   This "mostly internal" routine (re)populates f's checkout vfile
   table with all files from the given checkin manifest. If
   manifestRid is 0 or less then the current checkout's RID is
   used. If vfile already contains any content for the given checkin,
   it is left intact (and several processes rely on that behavior to
   keep it from nuking, e.g., as-yet-uncommitted queued add/rm
   entries).

   Returns 0 on success, any number of codes on any of many potential
   errors.

   f must not be NULL and must have opened checkout and repository
   databases. In debug builds it will assert that that is so.

   If the 3rd argument is true, any entries in vfile for checkin
   versions other than the one specified in the 2nd argument are
   cleared from the vfile table. That is _almost_ always the desired
   behavior, but there are rare cases where vfile needs to temporarily
   (for the duration of a single transaction) hold state for multiple
   versions.

   If the 4th argument is not NULL, it gets assigned the number of
   blobs from the given version which are currently missing from the
   repository due to being phantoms (as opposed to being shunned).

   Returns 0 on success, FSL_RC_NOT_A_CKOUT if no checkout is opened,
   FSL_RC_OOM on allocation error, FSL_RC_DB for db-related problems,
   et.al.

   Misc. notes:

   - This does NOT update the "checkout" vvar table entry because this
   routine is sometimes used in contexts where we need to briefly
   maintain two vfile versions and keep the previous checkout version.

   - Apps must take care to not leave more than one version in the
   vfile table for longer than absolutely needed. They "really should"
   use fsl_vfile_unload() to clear out any version they load with this
   routine.

   @see fsl_vfile_unload()
   @see fsl_vfile_unload_except()
*/
FSL_EXPORT int fsl_vfile_load(fsl_cx * f, fsl_id_t manifestRid,
                              bool clearOtherVersions,
                              uint32_t * missingCount);

/**
   Clears out all entries in the current checkout's vfile table with
   the given vfile.vid value. If vid<=0 then the current checkout RID
   is used (which is never a good idea from client-side code!).

   ACHTUNG: never do this without understanding the consequences. It
   can ruin the current checkout state.

   Returns 0 on success, FSL_RC_NOT_A_CKOUT if f has no checkout
   opened, FSL_RC_DB on any sort of db-related error (in which case
   f's error state is updated with a description of the problem).

   @see fsl_vfile_load()
   @see fsl_vfile_unload_except()
*/
FSL_EXPORT int fsl_vfile_unload(fsl_cx * f, fsl_id_t vid);

/**
   A counterpart of fsl_vfile_unload() which removes all vfile
   entries where vfile.vid is not the given vid. If vid is <=0 then
   the current checkout RID is used.

   Returns 0 on success, FSL_RC_NOT_A_CKOUT if f has no checkout
   opened, FSL_RC_DB on any sort of db-related error (in which case
   f's error state is updated with a description of the problem).

   @see fsl_vfile_load()
   @see fsl_vfile_unload()
*/
FSL_EXPORT int fsl_vfile_unload_except(fsl_cx * f, fsl_id_t vid);


/**
   Performs a "fingerprint check" between f's current checkout and
   repository databases. Returns 0 if either there is no mismatch or
   it is impossible to determine because the checkout is missing a
   fingerprint (which is legal for "older" checkout databases).

   If a mismatch is found, FSL_RC_REPO_MISMATCH is returned. Returns
   some other non-0 code on a lower-level error (db access, OOM,
   etc.).

   A mismatch can happen when the repository to which a checkout
   belongs is replaced, either with a completely different repository
   or a copy/clone of that same repository. Each repository copy may
   have differing blob.rid values, and those are what the checkout
   database uses to refer to repository-side data. If those RIDs
   change, then the checkout is left pointing to data other than what
   it should be.

   TODO: currently the library offers no automated recovery mechanism
   from a mismatch, the only remedy being to close the checkout
   database, destroy it, and re-create it. fossil(1) is able, in some cases,
   to automatically recover from this situation.
*/
FSL_EXPORT int fsl_ckout_fingerprint_check(fsl_cx * f);

#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_CHECKOUT_H_INCLUDED */
/* end of file ../include/fossil-scm/fossil-checkout.h */
/* start of file ../include/fossil-scm/fossil-confdb.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_CONFDB_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_CONFDB_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).
*/
/** @file fossil-confdb.h

    fossil-confdb.h declares APIs dealing with fossil's various
    configuration option storage backends.
*/


#if defined(__cplusplus)
extern "C" {
#endif

/**
   A flag type for specifying which configuration backend a given API
   should be applied to. Used by most of the fsl_config_XXX() APIS,
   e.g. fsl_config_get_int32() and friends.

   This type is named fsl_confdb_e (not the "db" part) because 3 of
   the 4 fossil-supported config storage backends are databases. The
   "versioned setting" backend, which deals with non-db local files,
   was encapsulated into this API long after the db-related options
   were
*/
enum fsl_confdb_e {
/**
   Signfies the global-level (per system user) configuration area.
*/
FSL_CONFDB_GLOBAL = 1,
/**
   Signfies the repository-level configuration area.
*/
FSL_CONFDB_REPO = 2,
/**
   Signfies the checkout-level (a.k.a. "local") configuration area.
*/
FSL_CONFDB_CKOUT = 3,

/**
   The obligatory special case...

   Versionable settings are stored directly in SCM-controlled files,
   each of which has the same name as the setting and lives in the
   .fossil-settings directory of a checkout. Though versionable
   settings _can_ be read from a non-checked-out repository, doing so
   requires knowning which version to fetch and is horribly
   inefficient, so there are currently no APIs for doing so.

   Note that the APIs which read and write versioned settings do not
   care whether those settings are valid for fossil(1).

   Reminder to self: SQL to find the checkins in which a versioned
   settings file was added or modified (ignoring renames and branches
   and whatnot).

   @code
select strftime('%Y-%m-%d %H:%M:%S',e.mtime) mtime,
b.rid, b.uuid from mlink m, filename f, blob b, event e
where f.fnid=m.fnid
and m.mid=b.rid
and b.rid=e.objid
and f.name='.fossil-settings/ignore-glob'
order by e.mtime desc 
;
   @endcode

*/
FSL_CONFDB_VERSIONABLE = 4
};
typedef enum fsl_confdb_e fsl_confdb_e;

/**
   Returns the name of the db table associated with the given
   mode. Results are undefined if mode is an invalid value. The
   returned bytes are static and constant.

   Returns NULL for the role FSL_CONFDB_VERSIONABLE.
*/
FSL_EXPORT char const * fsl_config_table_for_role(fsl_confdb_e mode);

/**
   Returns a handle to the db associates with the given fsl_confdb_e
   value. Returns NULL if !f or if f has no db opened for that
   configuration role. Results are undefined if mode is an invalid
   value.

   For FSL_CONFDB_VERSIONABLE it returns the results of fsl_cx_db(),
   even though there is no database-side support for versionable files
   (which live in files in a checkout).
*/
FSL_EXPORT fsl_db * fsl_config_for_role(fsl_cx * f, fsl_confdb_e mode);

/**
   Returns the int32 value of a property from one of f's config
   dbs, as specified by the mode parameter. Returns dflt if !f, f
   does not have the requested config db opened, no entry is found,
   or on db-level errors.
*/
FSL_EXPORT int32_t fsl_config_get_int32( fsl_cx * f, fsl_confdb_e mode,
                                         int32_t dflt, char const * key );
/**
   int64_t counterpart of fsl_config_get_int32().
*/
FSL_EXPORT int64_t fsl_config_get_int64( fsl_cx * f, fsl_confdb_e mode,
                                         int64_t dflt, char const * key );

/**
   fsl_id_t counterpart of fsl_config_get_int32().
*/
FSL_EXPORT fsl_id_t fsl_config_get_id( fsl_cx * f, fsl_confdb_e mode,
                                       fsl_id_t dflt, char const * key );
/**
   double counterpart of fsl_config_get_int32().
*/
FSL_EXPORT double fsl_config_get_double( fsl_cx * f, fsl_confdb_e mode,
                                         double dflt, char const * key );


/**
   Boolean countertpart of fsl_config_get_int32().

   fsl_str_bool() is used to determine the booleanness (booleanity?)
   of a given config option.
*/
FSL_EXPORT bool fsl_config_get_bool( fsl_cx * f, fsl_confdb_e mode,
                                     bool dflt, char const * key );

/**
   A convenience form of fsl_config_get_buffer().  If it finds a
   config entry it returns its value. If *len is not NULL then *len is
   assigned the length of the returned string, in bytes (and is set to
   0 if NULL is returned). Any errors encounters while looking for
   the entry are suppressed and NULL is returned.

   The returned memory must eventually be freed using fsl_free(). If
   len is not NULL then it is set to the length of the returned
   string. Returns NULL for any sort of error or for a NULL db value.

   If capturing error state is important for a given use case, use
   fsl_config_get_buffer() instead, which provides the same features
   as this one but propagates any error state.
*/
FSL_EXPORT char * fsl_config_get_text( fsl_cx * f, fsl_confdb_e mode,
                                       char const * key,
                                       fsl_size_t * len );

/**
   The fsl_buffer-type counterpart of fsl_config_get_int32().

   Replaces the contents of the given buffer (re-using any memory it
   might have) with a value from a config region. Returns 0 on
   success, FSL_RC_NOT_FOUND if no entry was found or the requested db
   is not opened, FSL_RC_OOM on allocation errror.

   If mode is FSL_CONFDB_VERSIONABLE, this operation requires
   a checkout and returns (if possible) the contents of the file
   named {CHECKOUT_ROOT}/.fossil-settings/{key}.

   On any sort of error, including the inability to find or open
   a versionable-settings file, non-0 is returned:

   - FSL_RC_OOM on allocation error

   - FSL_RC_NOT_FOUND if either the database referred to by mode is
   not opened or a matching setting cannot be found.

   - FSL_RC_ACCESS is a versionable setting file is found but cannot
   be opened for reading.

   - FSL_RC_NOT_A_CKOUT if mode is FSL_CONFDB_VERSIONABLE and no
   checkout is opened.

   - Potentially one of several db-related codes if reading a
   non-versioned setting fails.

   In the grand scheme of things, the inability to load a setting is
   not generally an error, so clients are not expected to treat it as
   fatal unless perhaps it returns FSL_RC_OOM, in which case it likely
   is.

   @see fsl_config_has_versionable()
*/
FSL_EXPORT int fsl_config_get_buffer( fsl_cx * f, fsl_confdb_e mode,
                                      char const * key, fsl_buffer * b );


/**
   If f has an opened checkout, this replaces b's contents (re-using
   any existing memory) with an absolute path to the filename for that
   setting: {CHECKOUT_ROOT}/.fossil-settings/{key}

   This routine neither verifies that the given key is a valid
   versionable setting name nor that the file exists: it's purely a
   string operation.

   Returns 0 on success, FSL_RC_NOT_A_CKOUT if f has no checkout
   opened, FSL_RC_MISUSE if key is NULL, empty, or if
   fsl_is_simple_pathname() returns false for the key, and FSL_RC_OOM
   if appending to the buffer fails.
 */
FSL_EXPORT int fsl_config_versionable_filename(fsl_cx *f, char const * key, fsl_buffer *b);

/**
   Sets a configuration variable in one of f's config databases, as
   specified by the mode parameter. Returns 0 on success.  val may
   be NULL. Returns FSL_RC_MISUSE if !f, f does not have that
   database opened, or !key, FSL_RC_RANGE if !key.

   If mode is FSL_CONFDB_VERSIONABLE, it attempts to write/overwrite
   the corresponding file in the current checkout.

   If mem is NULL and mode is not FSL_CONFDB_VERSIONABLE then an SQL
   NULL is bound instead of an empty blob. For FSL_CONFDB_VERSIONABLE
   an empty file will be written for that case.

   If mode is FSL_CONFDB_VERSIONABLE, this does NOT queue any
   newly-created versionable setting file for inclusion into the
   SCM. That is up to the caller. See
   fsl_config_versionable_filename() for info about an additional
   potential usage error case with FSL_CONFDB_VERSIONABLE.

   Pedantic side-note: the input text is saved as-is. No trailing
   newline is added when saving to FSL_CONFDB_VERSIONABLE because
   doing so would require making a copy of the input bytes just to add
   a newline to it. The non-text fsl_config_set_XXX() APIs add a
   newline when writing to their values out to a versionable config
   file because it costs them nothing to do so and text files "should"
   have a trailing newline.

   Potential TODO: if mode is FSL_CONFDB_VERSIONABLE and the key
   contains directory components, e,g, "app/x", we should arguably use
   fsl_mkdir_for_file() to create those components. As of this writing
   (2021-03-14), no such config keys have ever been used in fossil.

   @see fsl_config_versionable_filename()
*/
FSL_EXPORT int fsl_config_set_text( fsl_cx * f, fsl_confdb_e mode, char const * key, char const * val );

/**
   The blob counterpart of fsl_config_set_text(). If len is
   negative then fsl_strlen(mem) is used to determine the length of
   the memory.

   If mem is NULL and mode is not FSL_CONFDB_VERSIONABLE then an SQL
   NULL is bound instead of an empty blob. For FSL_CONFDB_VERSIONABLE
   an empty file will be written for that case.
*/
FSL_EXPORT int fsl_config_set_blob( fsl_cx * f, fsl_confdb_e mode, char const * key,
                                    void const * mem, fsl_int_t len );
/**
   int32 counterpart of fsl_config_set_text().
*/
FSL_EXPORT int fsl_config_set_int32( fsl_cx * f, fsl_confdb_e mode,
                                     char const * key, int32_t val );
/**
   int64 counterpart of fsl_config_set_text().
*/
FSL_EXPORT int fsl_config_set_int64( fsl_cx * f, fsl_confdb_e mode,
                                     char const * key, int64_t val );
/**
   fsl_id_t counterpart of fsl_config_set_text().
*/
FSL_EXPORT int fsl_config_set_id( fsl_cx * f, fsl_confdb_e mode,
                                  char const * key, fsl_id_t val );
/**
   fsl_double counterpart of fsl_config_set_text().
*/
FSL_EXPORT int fsl_config_set_double( fsl_cx * f, fsl_confdb_e mode,
                                      char const * key, double val );
/**
   Boolean counterpart of fsl_config_set_text().

   For compatibility with fossil conventions, the value will be saved
   in the string form "on" or "off". When mode is
   FSL_CONFDB_VERSIONABLE, that value will include a trailing newline.
*/
FSL_EXPORT int fsl_config_set_bool( fsl_cx * f, fsl_confdb_e mode,
                                    char const * key, bool val );

/**
   "Unsets" (removes) the given key from the given configuration database.
   It is not considered to be an error if the config table does not
   contain that key.

   Returns FSL_RC_UNSUPPORTED, without side effects, if mode is
   FSL_CONFDB_VERSIONABLE. It "could" hypothetically remove a
   checked-out copy of a versioned setting, then queue the file for
   removal in the next checkin, but it does not do so. It might, in
   the future, be changed to do so, or at least to remove the local
   settings file.
*/
FSL_EXPORT int fsl_config_unset( fsl_cx * f, fsl_confdb_e mode,
                                 char const * key );

/**
   Begins (or recurses) a transaction on the given configuration
   database.  Returns 0 on success, non-0 on error. On success,
   fsl_config_transaction_end() must eventually be called with the
   same parameters to pop the transaction stack. Returns
   FSL_RC_MISUSE if no db handle is opened for the given
   configuration mode. Assuming all arguments are valid, this
   returns the result of fsl_db_transaction_end() and propagates
   any db-side error into the f object's error state.

   This is primarily intended as an optimization when an app is
   making many changes to a config database. It is not needed when
   the app is only making one or two changes.

   @see fsl_config_transaction_end()
   @see fsl_db_transaction_begin()
*/
FSL_EXPORT int fsl_config_transaction_begin(fsl_cx * f, fsl_confdb_e mode);

/**
   Pops the transaction stack pushed by
   fsl_config_transaction_begin(). If rollback is true then the
   transaction is set roll back, otherwise it is allowed to
   continue (if recursive) or committed immediately (if not
   recursive).  Returns 0 on success, non-0 on error. Returns
   FSL_RC_MISUSE if no db handle is opened for the given
   configuration mode. Assuming all arguments are valid, this
   returns the result of fsl_db_transaction_end() and propagates
   any db-side error into the f object's error state.

   @see fsl_config_transaction_begin()
   @see fsl_db_transaction_end()
*/
FSL_EXPORT int fsl_config_transaction_end(fsl_cx * f, fsl_confdb_e mode, char rollback);

/**
   Populates li as a glob list from the given configuration key.
   Uses (versionable/repo/global) config settings, in that order.
   It is not an error if one or more of those sources is missing -
   they are simply skipped.

   Note that gets any new globs appended to it, as per
   fsl_glob_list_append(), as opposed to replacing any existing
   contents.

   Returns 0 on success, but that only means that there were no
   errors, not that any entries were necessarily added to li.

   Arguably a bug: this function does not open the global config if
   it was not already opened, but will use it if it is opened. This
   function should arbuably open and close it in that case.
*/
FSL_EXPORT int fsl_config_globs_load(fsl_cx * f, fsl_list * li, char const * key);

/**
   Fetches the preferred name of the "global" db file for the current
   user by assigning it to *zOut. Returns 0 on success, in which case
   *zOut is updated and non-0 on error, in which case *zOut is not
   modified.  On success, ownership of *zOut is transferred to the
   caller, who must eventually free it using fsl_free().

   The locations searched for the database file are
   platform-dependent...

   Unix-like systems are searched in the following order:

   1) If the FOSSIL_HOME environment var is set, use
   $FOSSIL_HOME/.fossil.

   2) If $HOME/.fossil already exists, use that.

   3) If XDG_CONFIG_HOME environment var is set, use
   $XDG_CONFIG_HOME/fossil.db.

   4) If $HOME/.config is a directory, use $HOME/.config/fossil.db

   5) Fall back to $HOME/.fossil (historical name).

   Except where listed above, this function does not check whether the
   file already exists or is a database.

   Windows:

   - We need a Windows port of this routine. Currently it simply uses
   the Windows home directory + "/_fossil" or "/.fossil", depending on
   the build-time environment.
*/
FSL_EXPORT int fsl_config_global_preferred_name(char ** zOut);

#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_CONFDB_H_INCLUDED */
/* end of file ../include/fossil-scm/fossil-confdb.h */
/* start of file ../include/fossil-scm/fossil-vpath.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_VPATH_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_VPATH_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

  *****************************************************************************
  This file declares public APIs relating to calculating paths via
  Fossil SCM version history.
*/


#if defined(__cplusplus)
extern "C" {
#endif

typedef struct fsl_vpath_node fsl_vpath_node;
typedef struct fsl_vpath fsl_vpath;

/**
   Holds information for a single node in a path of checkin versions.

   @see fsl_vpath
*/
struct fsl_vpath_node {
  /** ID for this node */
  fsl_id_t rid;
  /** True if pFrom is the parent of rid */
  bool fromIsParent;
  /** True if primary side of common ancestor */
  bool isPrimary;
  /* HISTORICAL: Abbreviate output in "fossil bisect ls" */
  bool isHidden;
  /** Node this one came from. */
  fsl_vpath_node *pFrom;
  union {
    /** List of nodes of the same generation */
    fsl_vpath_node *pPeer;
    /** Next on path from beginning to end */
    fsl_vpath_node *pTo;
  } u;
  /** List of all nodes */
  fsl_vpath_node *pAll;
};

/**
   A utility type for collecting "paths" between two checkin versions.
*/
struct fsl_vpath{
  /** Current generation of nodes */
  fsl_vpath_node *pCurrent;
  /** All nodes */
  fsl_vpath_node *pAll;
  /** Nodes seen before */
  fsl_id_bag seen;
  /** Number of steps from first to last. */
  int nStep;
  /** Earliest node in the path. */
  fsl_vpath_node *pStart;
  /** Common ancestor of pStart and pEnd */
  fsl_vpath_node *pPivot;
  /** Most recent node in the path. */
  fsl_vpath_node *pEnd;
};

/**
   An empty-initialize fsl_vpath object, intended for const-copy
   initialization.
*/
#define fsl_vpath_empty_m {0,0,fsl_id_bag_empty_m,0,0,0,0}

/**
   An empty-initialize fsl_vpath object, intended for copy
   initialization.
*/
FSL_EXPORT const fsl_vpath fsl_vpath_empty;


/**
   Returns the first node in p's path.

   The returned node is owned by path may be invalidated by any APIs
   which manipulate path.
*/
FSL_EXPORT fsl_vpath_node * fsl_vpath_first(fsl_vpath *p);

/**
   Returns the last node in p's path.

   The returned node is owned by path may be invalidated by any APIs
   which manipulate path.
*/
FSL_EXPORT fsl_vpath_node * fsl_vpath_last(fsl_vpath *p);

/**
   Returns the next node p's path.

   The returned node is owned by path may be invalidated by any APIs
   which manipulate path.

   Intended to be used like this:

   @code
   for( p = fsl_vpath_first(path) ;
   p ;
   p = fsl_vpath_next(p)){
   ...
   }
   @endcode
*/
FSL_EXPORT fsl_vpath_node * fsl_vpath_next(fsl_vpath_node *p);

/**
   Returns p's path length.
*/
FSL_EXPORT int fsl_vpath_length(fsl_vpath const * p);

/**
   Frees all nodes in path (which must not be NULL) and resets all
   state in path. Does not free path.
*/
FSL_EXPORT void fsl_vpath_clear(fsl_vpath *path);

/**
   Find the mid-point of the path.  If the path contains fewer than 2
   steps, returns 0. The returned node is owned by path and may be
   invalidated by any APIs which manipulate path.
*/
FSL_EXPORT fsl_vpath_node * fsl_vpath_midpoint(fsl_vpath * path);


/**
   Computes the shortest path from checkin versions iFrom to iTo
   (inclusive), storing the result state in path. If path has
   state before this is called, it is cleared by this call.

   iFrom and iTo must both be valid checkin version RIDs.

   If directOnly is true, then use only the "primary" links from
   parent to child.  In other words, ignore merges.

   On success, returns 0 and path->pStart will point to the
   beginning of the path (the iFrom node). If pStart is 0 then no
   path could be found but 0 is still returned.

   Elements of the path can be traversed like so:

   @code
   fsl_vpath path = fsl_vpath_empty;
   fsl_vpath_node * n = 0;
   int rc = fsl_vpath_shortest(f, &path, versionFrom, versionTo, 1, 0);
   if(rc) { ... error ... }
   for( n = fsl_vpath_first(&path); n; n = fsl_vpath_next(n) ){
   ...
   }
   fsl_vpath_clear(&path);
   @endcode

   On error, f's error state may be updated with a description of the
   problem.
*/
FSL_EXPORT int fsl_vpath_shortest( fsl_cx * const f, fsl_vpath * const path,
                                   fsl_id_t iFrom, fsl_id_t iTo,
                                   bool directOnly, bool oneWayOnly );

/**
   This variant of fsl_vpath_shortest() stores the shortest direct
   path from version iFrom to version iTo in the ANCESTOR temporary
   table using f's current repo db handle. That table gets created, if
   needed, else cleared by this call.

   The ANCESTOR temp table has the following interface:

   @code
   rid INT UNIQUE
   generation INTEGER PRIMARY KEY
   @endcode

   Where [rid] is a checkin version RID and [generation] is the
   1-based number of steps from iFrom, including iFrom (so iFrom's own
   generation is 1).

   On error returns 0 and, if pSteps is not NULL, assigns *pSteps to
   the number of entries added to the ancestor table. On error, pSteps
   is never modifed, any number of various non-0 codes may be
   returned, and f's error state will, if possible (not an OOM), be
   updated to describe the problem.

   This function's name is far too long and descriptive. We might want
   to consider something shorter.

   Maintenance reminder: this impl swaps the 2nd and 3rd parameters
   compared to fossil(1)'s version!
*/
FSL_EXPORT int fsl_vpath_shortest_store_in_ancestor(fsl_cx * const f,
                                                    fsl_id_t iFrom,
                                                    fsl_id_t iTo,
                                                    uint32_t *pSteps);


/**
   Reconstructs path from path->pStart to path->pEnd, reversing its
   order by fiddling with the u->pTo fields.

   Unfortunately does not reverse after the initial creation/reversal
   :/.
*/
FSL_EXPORT void fsl_vpath_reverse(fsl_vpath * path);


#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_VPATH_H_INCLUDED */
/* end of file ../include/fossil-scm/fossil-vpath.h */
/* start of file ../include/fossil-scm/fossil-internal.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_INTERNAL_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_INTERNAL_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

  *****************************************************************************
  This file declares library-level internal APIs which are shared
  across the library.
*/


#if defined(__cplusplus)
extern "C" {
#endif

typedef struct fsl_acache fsl_acache;
typedef struct fsl_acache_line fsl_acache_line;
typedef struct fsl_pq fsl_pq;
typedef struct fsl_pq_entry fsl_pq_entry;

/** @internal

    Queue entry type for the fsl_pq class.

    Potential TODO: we don't currently use the (data) member. We can
    probably remove it.
*/
struct fsl_pq_entry {
  /** RID of the entry. */
  fsl_id_t id;
  /** Raw data associated with this entry. */
  void * data;
  /** Priority of this element. */
  double priority;
};
/** @internal
    Empty-initialized fsl_pq_entry structure.
*/
#define fsl_pq_entry_empty_m {0,NULL,0.0}

/** @internal

    A simple priority queue class. Instances _must_ be initialized
    by copying fsl_pq_empty or fsl_pq_empty_m (depending on where
    the instance lives).
*/
struct fsl_pq {
  /** Number of items allocated in this->list. */
  uint16_t capacity;
  /** Number of items used in this->list. */
  uint16_t used;
  /** The queue. It is kept sorted by entry->priority. */
  fsl_pq_entry * list;
};

/** @internal
    Empty-initialized fsl_pq struct, intended for const-copy initialization.
*/
#define fsl_pq_empty_m {0,0,NULL}

/** @internal
    Empty-initialized fsl_pq struct, intended for copy initialization.
*/
extern  const fsl_pq fsl_pq_empty;

/** @internal

    Clears the contents of p, freeing any memory it owns, but not
    freeing p. Results are undefined if !p.
*/
void fsl_pq_clear(fsl_pq * p);

/** @internal

    Insert element e into the queue. Returns 0 on success, FSL_RC_OOM
    on error. Results are undefined if !p. pData may be NULL.
*/
int fsl_pq_insert(fsl_pq *p, fsl_id_t e,
                  double v, void *pData);

/** @internal

    Extracts (removes) the first element from the queue (the element
    with the smallest value) and return its ID.  Return 0 if the queue
    is empty. If pp is not NULL then *pp is (on success) assigned to
    opaquedata pointer mapped to the entry.
*/
fsl_id_t fsl_pq_extract(fsl_pq *p, void **pp);

/** @internal

    Holds one "line" of a fsl_acache cache.
*/
struct fsl_acache_line {
  /**
     RID of the cached record.
  */
  fsl_id_t rid;
  /**
     Age. Newer is larger.
  */
  fsl_int_t age;
  /**
     Content of the artifact.
  */
  fsl_buffer content;
};
/** @internal

    Empty-initialized fsl_acache_line structure.
*/
#define fsl_acache_line_empty_m { 0,0,fsl_buffer_empty_m }


/** @internal

    A cache for tracking the existence of artifacts while the
    internal goings-on of control artifacts are going on.

    Currently the artifact cache is unused because it costs much
    more than it gives us. Once the library supports certain
    operations (like rebuild and sync) caching will become more
    useful.

    Historically fossil caches artifacts as their blob content, but
    libfossil will likely (at some point) to instead cache fsl_deck
    instances, which contain all of the same data in pre-parsed form.
    It cost more memory, though. That approach also precludes caching
    non-structural artifacts (i.e. opaque client blobs).

    Potential TODO: the limits of the cache size are hard-coded in
    fsl_acache_insert. Those really should be part of this struct.
*/
struct fsl_acache {
  /**
     Total amount of buffer memory (in bytes) used by cached content.
     This does not account for memory held by this->list.
  */
  fsl_size_t szTotal;
  /**
     Limit on the (approx.) amount of memory (in bytes) which can be
     taken up by the cached buffers at one time. Fossil's historical
     value is 50M.
  */
  fsl_size_t szLimit;
  /**
     Number of entries "used" in this->list.
  */
  uint16_t used;
  /**
     Approximate upper limit on the number of entries in this->list.
     This limit may be violated slightly.

     This number should ideally be relatively small: 3 digits or less.
     Fossil's historical value is 500.
  */
  uint16_t usedLimit;
  /**
     Number of allocated slots in this->list.
  */
  uint16_t capacity;
  /**
     Next cache counter age. Higher is newer.
  */
  fsl_int_t nextAge;
  /**
     List of cached content, ordered by age.
  */
  fsl_acache_line * list;
  /**
     All artifacts currently in the cache.
  */
  fsl_id_bag inCache;
  /**
     Cache of known-missing content.
  */
  fsl_id_bag missing;
  /**
     Cache of of known-existing content.
  */
  fsl_id_bag available;
};
/** @internal

    Empty-initialized fsl_acache structure, intended
    for const-copy initialization.
*/
#define fsl_acache_empty_m {                \
  0/*szTotal*/,                             \
  20000000/*szLimit. Historical fossil value=50M*/, \
  0/*used*/,300U/*usedLimit. Historical fossil value=500*/,\
  0/*capacity*/,                            \
  0/*nextAge*/,NULL/*list*/,                \
  fsl_id_bag_empty_m/*inCache*/,            \
  fsl_id_bag_empty_m/*missing*/,            \
  fsl_id_bag_empty_m/*available*/           \
}
/** @internal

    Empty-initialized fsl_acache structure, intended
    for copy initialization.
*/
extern const fsl_acache fsl_acache_empty;

/** @internal

   Very internal.

   "Manifest cache" for fsl_deck entries encountered during
   crosslinking. This type is intended only to be embedded in fsl_cx.

   The routines for managing this cache are static in deck.c:
   fsl_cx_mcache_insert() and fsl_cx_mcache_search().

   The array members in this struct MUST have the same length
   or results are undefined.
*/
struct fsl_mcache {
  /** Next age value. No clue how the cache will react once this
      overflows. */
  unsigned nextAge;
  /** The virtual age of each deck in the cache. They get evicted
      oldest first. */
  unsigned aAge[4];
  /** Counts the number of cache hits. */
  unsigned hits;
  /** Counts the number of cache misses. */
  unsigned misses;
  /**
     Stores bitwise copies of decks. Storing a fsl_deck_malloc() deck
     into the cache requires bitwise-copying is contents, wiping out
     its contents via assignment from fsl_deck_empty, then
     fsl_free()'ing it (as opposed to fsl_deck_finalize(), which would
     attempt to clean up memory which now belongs to the cache's
     copy).

     Array sizes of 6 and 10 do not appreciably change the hit rate
     compared to 4, at least not for current (2021-03-26) uses.
  */
  fsl_deck decks[4];
};

/** Convenience typedef. */
typedef struct fsl_mcache fsl_mcache;

/** Initialized-with-defaults fsl_mcache structure, intended for
    const-copy initialization. */
#define fsl_mcache_empty_m {\
  0,                  \
  {0,0,0,0},\
  0,0, \
  {fsl_deck_empty_m,fsl_deck_empty_m,fsl_deck_empty_m,   \
  fsl_deck_empty_m} \
}

/** Initialized-with-defaults fsl_mcache structure, intended for
    non-const copy initialization. */
extern const fsl_mcache fsl_mcache_empty;


/* The fsl_cx class is documented in main public header. */
struct fsl_cx {
  /**
     A pointer to the "main" db handle. Exactly which db IS the
     main db is, because we have three DBs, not generally knowble.

     As of this writing (20141027) the following applies:

     dbMain always points to &this->dbMem (a ":memory:" db opened by
     fsl_cx_init()), and the repo/ckout/config DBs get ATTACHed to
     that one. Their separate handles (this->{repo,ckout,config}.db)
     are used to store the name and file path to each one (even though
     they have no real db handle associated with them).

     Internal code should rely as little as possible on the actual
     arrangement of internal DB handles, and should use
     fsl_cx_db_repo(), fsl_cx_db_ckout(), and fsl_cx_db_config() to
     get a handle to the specific db they want. Currently they will
     always return NULL or the same handle, but that design decision
     might change at some point, so the public API treats them as
     separate entities.
  */
  fsl_db * dbMain;

  /**
     Marker which tells us whether fsl_cx_finalize() needs
     to fsl_free() this instance or not.
  */
  void const * allocStamp;

  /**
     A ":memory:" (or "") db to work around
     open-vs-attach-vs-main-vs-real-name problems wrt to the
     repo/ckout/config dbs. This db handle gets opened automatically
     at startup and all others which a fsl_cx manages get ATTACHed to
     it. Thus the other internal fsl_db objects, e.g. this->repo.db,
     may hold state, such as the path to the current repo db, but they
     do NOT hold an sqlite3 db handle. Assigning them the handle of
     this->dbMain would indeed simplify certain work but it would
     require special care to ensure that we never sqlite3_close()
     those out from under this->dbMain.
  */
  fsl_db dbMem;

  /**
     Holds info directly related to a checkout database.
  */
  struct {
    /**
       Handle to the currently opened checkout database IF the checkout
       is the main db.
    */
    fsl_db db;
    /**
       Possibly not needed, but useful for doing absolute-to-relative
       path conversions for checking file lists.

       The directory part of an opened checkout db.  This is currently
       only set by fsl_ckout_open_dir(). It contains a trailing slash,
       largely because that simplifies porting fossil(1) code and
       appending filenames to this directory name to create absolute
       paths.
    */
    char * dir;
    /**
       Optimization: fsl_strlen() of dir. Guaranteed to be set to
       dir's length if dir is not NULL.
    */
    fsl_size_t dirLen;
    /**
       The rid of the current checkout. May be 0 for an empty
       repo/checkout. Must be negative if not yet known.
    */
    fsl_id_t rid;

    /**
       The UUID of the current checkout. Only set if this->rid
       is positive.
    */
    fsl_uuid_str uuid;

    /**
       Julian mtime of the checkout version, as reported by the
       [event] table.
    */
    double mtime;
  } ckout;

  /**
     Holds info directly related to a repo database.
  */
  struct {
    /**
       Handle to the currently opened repository database IF repo
       is the main db.
    */
    fsl_db db;
    /**
       The default user name, for operations which need one.
       See fsl_cx_user_set().
    */
    char * user;
  } repo;

  /**
     Holds info directly related to a global config database.
  */
  struct {
    /**
       Handle to the currently opened global config database IF config
       is the main db.
    */
    fsl_db db;
  } config;

  /**
     State for incrementally proparing a checkin operation.
  */
  struct {
    /**
       Holds a list of "selected files" in the form
       of vfile.id values.
    */
    fsl_id_bag selectedIds;

    /**
       The deck used for incrementally building certain parts of a
       checkin.
    */
    fsl_deck mf;
  } ckin;

  /**
     Confirmation callback.
  */
  fsl_confirmer confirmer;

  /**
     Output channel used by fsl_output() and friends.
  */
  fsl_outputer output;

  /**
     Can be used to tie client-specific data to the context. Its
     finalizer is called when fsl_cx_finalize() cleans up.
  */
  fsl_state clientState;

  /**
     Holds error state. As a general rule, this information is updated
     only by routines which need to return more info than a simple
     integer error code. e.g. this is often used to hold
     db-driver-provided error state. It is not used by "simple"
     routines for which an integer code always suffices. APIs which
     set this should denote it with a comment like "updates the
     context's error state on error."
  */
  fsl_error error;

  /**
     A place for temporarily holding file content. We use this in
     places where we have to loop over files and read their entire
     contents, so that we can reuse this buffer's memory if possible.
     The loop and the reading might be happening in different
     functions, though, and some care must be taken to avoid use in
     two functions concurrently.
  */
  fsl_buffer fileContent;

  /**
     Reuseable scratchpads for low-level file canonicalization
     buffering and whatnot. Not intended for huge content: use
     this->fileContent for that. This list must stay relatively
     short. As of this writing, most buffers ever active at once was
     5, but 1-2 is more common.

     @see fsl_cx_scratchpad()
     @see fsl_cx_scratchpad_yield()
  */
  struct {
    /**
       Strictly-internal temporary buffers we intend to reuse many
       times, mostly for filename canonicalization, holding hash
       values, and small encoding/decoding tasks. These must never be
       used for values which will be long-lived, nor are they intended
       to be used for large content, e.g. reading files, with the
       possible exception of holding versioned config settings, as
       those are typically rather small.

       If needed, the lengths of this->buf[] and this->used[] may be
       extended, but anything beyond 8, maybe 10, seems a bit extreme.
       They should only be increased if we find code paths which
       require it. As of this writing (2021-03-17), the peak
       concurrently used was 5. In any case fsl_cx_scratchpad() fails
       fatally if it needs more than it has, so we won't fail to
       overlook such a case.
    */
    fsl_buffer buf[8];
    /**
       Flags telling us which of this->buf is currenly in use.
    */
    bool used[8];
    /**
       A cursor _hint_ to try to speed up fsl_cx_scratchpad() by about
       half a nanosecond, making it O(1) instead of O(small N) for the
       common case.
    */
    short next;
  } scratchpads;

  /**
     A copy of the config object passed to fsl_cx_init() (or some
     default).
  */
  fsl_cx_config cxConfig;

  /**
     Flags, some (or one) of which is runtime-configurable by the
     client (see fsl_cx_flags_e). We can get rid of this and add the
     flags to the cache member along with the rest of them.
  */
  int flags;

  /**
     List of callbacks for deck crosslinking purposes.
  */
  fsl_xlinker_list xlinkers;

  /**
     A place for caching generic things.
  */
  struct {
    /**
       If true, SOME repository-level file-name comparisons/searches
       will work case-insensitively.
    */
    bool caseInsensitive;

    /**
       If true, skip "dephantomization" of phantom blobs.  This is a
       detail from fossil(1) with as-yet-undetermined utility. It's
       apparently only used during the remote-sync process, which this
       API does not (as of 2021-02) yet have.
    */
    bool ignoreDephantomizations;

    /**
       Whether or not a running commit process should be marked as
       private.
    */
    bool markPrivate;

    /**
       True if fsl_crosslink_begin() has been called but
       fsl_crosslink_end() is still pending.
    */
    bool isCrosslinking;

    /**
       Flag indicating that only cluster control artifacts should be
       processed by manifest crosslinking. This will only be relevant
       if/when the sync protocol is implemented.
    */
    bool xlinkClustersOnly;

    /**
       Is used to tell the content-save internals that a "final
       verification" (a.k.a. verify-before-commit) is underway.
    */
    bool inFinalVerify;

    /**
       Cached copy of the allow-symlinks config option, because it is
       (hypothetically) needed on many stat() call. Negative
       value=="not yet determined", 0==no, positive==yes. The negative
       value means we need to check the repo config resp. the global
       config to see if this is on.

       As of late 2020, fossil(1) is much more restrictive with
       symlinks due to vulnerabilities which were discovered by a
       security researcher, and we definitely must not default any
       symlink-related features to enabled/on. As of Feb. 2021, my
       personal preference, and very likely plan of attack, is to only
       treat SCM'd symlinks as if symlinks support is disabled. It's
       very unlikely that i will implement "real" symlink support but
       would, *solely* for compatibility with fossil(1), be convinced
       to permit such changes if someone else wants to implement them.
    */
    short allowSymlinks;

    /**
       Indicates whether or not this repo has ever seen a delta
       manifest. If none has ever been seen then the repository will
       prefer to use baseline (non-delta) manifests. Once a delta is
       seen in the repository, the checkin algorithm is free to choose
       deltas later on unless its otherwise prohibited, e.g. by the
       forbid-delta-manifests config db setting.

       This article provides an overview to the topic delta manifests
       and essentially debunks their ostensible benefits:

       https://fossil-scm.org/home/doc/tip/www/delta-manifests.md

       Values: negative==undetermined, 0==no, positive==yes. This is
       updated when a repository is first opened and when new content
       is written to it.
    */
    short seenDeltaManifest;

    /**
       Records whether this repository has an FTS search
       index. <0=undetermined, 0=no, >0=yes.
     */
    short searchIndexExists;

    /**
       Cache for the "manifest" config setting, as used by
       fsl_ckout_manifest_setting(), with the caveat that
       if the setting changes after it is cached, we won't necessarily
       see that here!
    */
    short manifestSetting;

    /**
       Record ID of rcvfrom entry during commits. This is likely to
       remain unused in libf until/unless the sync protocol is
       implemented.
    */
    fsl_id_t rcvId;
    
    /**
       Artifact cache used during processing of manifests.
    */
    fsl_acache arty;
    /**
       Used during manifest parsing to keep track of artifacts we have
       seen. Whether that's really necessary or is now an unnecessary
       porting artifact (haha) is unclear.
    */
    fsl_id_bag mfSeen;
    /**
       Used during the processing of manifests to keep track of
       "leaf checks" which need to be done downstream.
    */
    fsl_id_bag leafCheck;
    /**
       Holds the RID of every record awaiting verification
       during the verify-at-commit checks.
    */
    fsl_id_bag toVerify;
    /**
       Infrastructure for fsl_mtime_of_manifest_file(). It
       remembers the previous RID so that it knows when it has to
       invalidate/rebuild its ancestry cache.
    */
    fsl_id_t mtimeManifest;
    /**
       The "project-code" config option. We do not currently (2021-03)
       use this but it will be important if/when the sync protocol is
       implemented or we want to create hashes, e.g. for user
       passwords, which depend in part on the project code.
    */
    char * projectCode;

    /**
       Internal optimization to avoid duplicate stat() calls across
       two functions in some cases.
    */
    fsl_fstat fstat;

    /**
       Parsed-deck cache.
    */
    fsl_mcache mcache;
    
    /**
       Holds various glob lists. That said... these features are
       actually app-level stuff which the library itself does not
       resp. should not enforce. We can keep track of these for users
       but the library internals _generall_ have no business using
       them.

       _THAT_ said... these have enough generic utility that we can
       justify storing them and _optionally_ applying them. See
       fsl_checkin_opt for an example of where we do this.
    */
    struct {
      /**
         Holds the "ignore-glob" globs.
      */
      fsl_list ignore;
      /**
         Holds the "binary-glob" globs.
      */
      fsl_list binary;
      /**
         Holds the "crnl-glob" globs.
      */
      fsl_list crnl;
    } globs;
  } cache;

  /**
     Ticket-related information.
  */
  struct {
    /**
       Holds a list of (fsl_card_J*) records representing custom
       ticket table fields available in the db.

       Each entry's flags member denote (using fsl_card_J_flags)
       whether that field is used by the ticket or ticketchng
       tables.

       TODO, eventually: add a separate type for these entries.  We
       use fsl_card_J because the infrastructure is there and they
       provide what we need, but fsl_card_J::flags only exists for
       this list. A custom type would be smaller than fsl_card_J
       (only two members) but adding it requires adding some
       infrastructure which isn't worth the effort at the moment.
    */
    fsl_list customFields;

    /**
       Gets set to true (at some point) if the client has the
       ticket db table.
    */
    bool hasTicket;
    /**
       Gets set to true (at some point) if the client has the
       ticket.tkt_ctime db field.
    */
    bool hasCTime;

    /**
       Gets set to true (at some point) if the client has the
       ticketchng db table.
    */
    bool hasChng;
    /**
       Gets set to true (at some point) if the client has the
       ticketchng.rid db field.
    */
    bool hasChngRid;
  } ticket;

  /*
    Note: no state related to server/user/etc. That is higher-level
    stuff. We might need to allow the user to set a default user
    name to avoid that he has to explicitly set it on all of the
    various Control Artifact-generation bits which need it.
  */
};

/** @internal

    Initialized-with-defaults fsl_cx struct.
*/
#define fsl_cx_empty_m {                                \
    NULL /*dbMain*/,                                    \
    NULL/*allocStamp*/,                               \
    fsl_db_empty_m /* dbMem */,                       \
    {/*ckout*/                                        \
      fsl_db_empty_m /*db*/,                          \
      NULL /*dir*/, 0/*dirLen*/,                    \
      -1/*rid*/, NULL/*uuid*/, 0/*mtime*/        \
    },                                            \
    {/*repo*/ fsl_db_empty_m /*db*/,                  \
       0/*user*/                                     \
    },                                            \
    {/*config*/ fsl_db_empty_m /*db*/ },              \
    {/*ckin*/                                         \
      fsl_id_bag_empty_m/*selectedIds*/,            \
      fsl_deck_empty_m/*mf*/                     \
    },                                            \
    fsl_confirmer_empty_m/*confirmer*/,           \
    fsl_outputer_FILE_m /*output*/,                 \
    fsl_state_empty_m /*clientState*/,            \
    fsl_error_empty_m /*error*/,                  \
    fsl_buffer_empty_m /*fileContent*/,           \
    {/*scratchpads*/ \
      {fsl_buffer_empty_m,fsl_buffer_empty_m,     \
      fsl_buffer_empty_m,fsl_buffer_empty_m,      \
      fsl_buffer_empty_m,fsl_buffer_empty_m},     \
      {false,false,false,false,false,false},      \
      0/*next*/                                   \
    },                                            \
    fsl_cx_config_empty_m /*cxConfig*/,           \
    FSL_CX_F_DEFAULTS/*flags*/,                   \
    fsl_xlinker_list_empty_m/*xlinkers*/,         \
    {/*cache*/                                    \
      false/*caseInsensitive*/,                  \
      false/*ignoreDephantomizations*/,          \
      false/*markPrivate*/,                         \
      false/*isCrosslinking*/,                      \
      false/*xlinkClustersOnly*/,                   \
      false/*inFinalVerify*/,                       \
      -1/*allowSymlinks*/,                      \
      -1/*seenDeltaManifest*/,                  \
      -1/*searchIndexExists*/,                  \
      -1/*manifestSetting*/,\
      0/*rcvId*/,                               \
      fsl_acache_empty_m/*arty*/,               \
      fsl_id_bag_empty_m/*mfSeen*/,           \
      fsl_id_bag_empty_m/*leafCheck*/,        \
      fsl_id_bag_empty_m/*toVerify*/,         \
      0/*mtimeManifest*/,                     \
      NULL/*projectCode*/,                    \
      fsl_fstat_empty_m/*fstat*/,             \
      fsl_mcache_empty_m/*mcache*/,           \
      {/*globs*/                              \
        fsl_list_empty_m/*ignore*/,           \
        fsl_list_empty_m/*binary*/,         \
        fsl_list_empty_m/*crnl*/            \
      }                                     \
    }/*cache*/,                             \
    {/*ticket*/                             \
      fsl_list_empty_m/*customFields*/,     \
      0/*hasTicket*/,                       \
      0/*hasCTime*/,                        \
      0/*hasChng*/,                         \
      0/*hasCngRid*/                        \
    }                                       \
  }

/** @internal
    Initialized-with-defaults fsl_cx instance.
*/
FSL_EXPORT const fsl_cx fsl_cx_empty;

/*
  TODO:

  int fsl_buffer_append_getenv( fsl_buffer * b, char const * env )

  Fetches the given env var and appends it to b. Returns FSL_RC_NOT_FOUND
  if the env var is not set. The primary use for this would be to simplify
  the Windows implementation of fsl_find_home_dir().
*/


/** @internal

    Expires the single oldest entry in c. Returns true if it removes
    an item, else false.
*/
FSL_EXPORT bool fsl_acache_expire_oldest(fsl_acache * c);

/** @internal

    Add an entry to the content cache.

    This routines transfers the contents of pBlob over to c,
    regardless of success or failure.  The cache will deallocate the
    memory when it has finished with it.

    If the cache cannot add the entry due to cache-internal
    constraints, as opposed to allocation errors, it clears the buffer
    (for consistency's sake) and returned 0.

    Returns 0 on success, FSL_RC_OOM on allocation error. Has undefined
    behaviour if !c, rid is not semantically valid, !pBlob. An empty
    blob is normally semantically illegal but is not strictly illegal
    for this cache's purposes.
*/
FSL_EXPORT int fsl_acache_insert(fsl_acache * c, fsl_id_t rid, fsl_buffer *pBlob);

/** @internal

    Frees all memory held by c, and clears out c's state, but does
    not free c. Results are undefined if !c.
*/
FSL_EXPORT void fsl_acache_clear(fsl_acache * c);

/** @internal

    Checks f->cache.arty to see if rid is available in the
    repository opened by f.

    Returns 0 if the content for the given rid is available in the
    repo or the cache. Returns FSL_RC_NOT_FOUND if it is not in the
    repo nor the cache. Returns some other non-0 code for "real
    errors," e.g. FSL_RC_OOM if a cache allocation fails. This
    operation may update the cache's contents.

    If this function detects a loop in artifact lineage, it fails an
    assert() in debug builds and returns FSL_RC_CONSISTENCY in
    non-debug builds. That doesn't happen in real life, though.
*/
FSL_EXPORT int fsl_acache_check_available(fsl_cx * f, fsl_id_t rid);

/** @internal

    This is THE ONLY routine which adds content to the blob table.

    This writes the given buffer content into the repository
    database's blob tabe. It Returns the record ID via outRid (if it
    is not NULL).  If the content is already in the database (as
    determined by a lookup of its hash against blob.uuid), this
    routine fetches the RID (via *outRid) but has no side effects in
    the repo.

    If srcId is >0 then pBlob must contain delta content from
    the srcId record. srcId might be a phantom.  

    pBlob is normally uncompressed text, but if uncompSize>0 then
    the pBlob value is assumed to be compressed (via fsl_buffer_compress()
    or equivalent) and uncompSize is
    its uncompressed size. If uncompSize>0 then zUuid must be valid
    and refer to the hash of the _uncompressed_ data (which is why
    this routine does not calculate it for the client).

    Sidebar: we "could" use fsl_buffer_is_compressed() and friends
    to determine if pBlob is compressed and get its decompressed
    size, then remove the uncompSize parameter, but that would
    require that this function decompress the content to calculate
    the hash. Since the caller likely just compressed it, that seems
    like a huge waste.

    zUuid is the UUID of the artifact, if it is not NULL.  When
    srcId is specified then zUuid must always be specified.  If
    srcId is zero, and zUuid is zero then the correct zUuid is
    computed from pBlob.  If zUuid is not NULL then this function
    asserts (in debug builds) that fsl_is_uuid() returns true for
    zUuid.

    If isPrivate is true, the blob is created as a private record.

    If the record already exists but is a phantom, the pBlob content
    is inserted and the phatom becomes a real record.

    The original content of pBlob is not disturbed.  The caller continues
    to be responsible for pBlob.  This routine does *not* take over
    responsibility for freeing pBlob.

    If outRid is not NULL then on success *outRid is assigned to the
    RID of the underlying blob record.

    Returns 0 on success and there are too many potential error cases
    to name - this function is a massive beast.

    Potential TODO: we don't really need the uncompSize param - we
    can deduce it, if needed, based on pBlob's content. We cannot,
    however, know the UUID of the decompressed content unless the
    client passes it in to us.

    @see fsl_content_put()
*/
FSL_EXPORT int fsl_content_put_ex( fsl_cx * f, fsl_buffer const * pBlob,
                                   fsl_uuid_cstr zUuid, fsl_id_t srcId,
                                   fsl_size_t uncompSize, bool isPrivate,
                                   fsl_id_t * outRid);
/** @internal

    Equivalent to fsl_content_put_ex(f,pBlob,NULL,0,0,0,newRid).

    This must only be used for saving raw (non-delta) content.

    @see fsl_content_put_ex()
*/
FSL_EXPORT int fsl_content_put( fsl_cx * f, fsl_buffer const * pBlob,
                                fsl_id_t * newRid);


/** @internal

    If the given blob ID refers to deltified repo content, this routine
    undeltifies it and replaces its content with its expanded
    form.

    Returns 0 on success, FSL_RC_MISUSE if !f, FSL_RC_NOT_A_REPO if
    f has no opened repository, FSL_RC_RANGE if rid is not positive,
    and any number of other potential errors during the db and
    content operations. This function treats already unexpanded
    content as success.

    @see fsl_content_deltify()
*/
FSL_EXPORT int fsl_content_undeltify(fsl_cx * f, fsl_id_t rid);


/** @internal

    The converse of fsl_content_undeltify(), this replaces the storage
    of the given blob record so that it is a delta of srcid.

    If rid is already a delta from some other place then no
    conversion occurs and this is a no-op unless force is true.

    If rid's contents are not available because the the rid is a
    phantom or depends to one, no delta is generated and 0 is
    returned.

    It never generates a delta that carries a private artifact into
    a public artifact. Otherwise, when we go to send the public
    artifact on a sync operation, the other end of the sync will
    never be able to receive the source of the delta.  It is OK to
    delta private->private, public->private, and public->public.
    Just no private->public delta. For such cases this function
    returns 0, as opposed to FSL_RC_ACCESS or some similar code, and
    leaves the content untouched.

    If srcid is a delta that depends on rid, then srcid is
    converted to undelta'd text.

    If either rid or srcid contain less than some "small,
    unspecified number" of bytes (currently 50), or if the resulting
    delta does not achieve a compression of at least 25%, the rid is
    left untouched.

    Returns 0 if a delta is successfully made or none needs to be
    made, non-0 on error.

    @see fsl_content_undeltify()
*/
FSL_EXPORT int fsl_content_deltify(fsl_cx * f, fsl_id_t rid,
                                   fsl_id_t srcid, bool force);


/** @internal

    Creates a new phantom blob with the given UUID and return its
    artifact ID via *newId. Returns 0 on success, FSL_RC_MISUSE if
    !f or !uuid, FSL_RC_RANGE if fsl_is_uuid(uuid) returns false,
    FSL_RC_NOT_A_REPO if f has no repository opened, FSL_RC_ACCESS
    if the given uuid has been shunned, and about 20 other potential
    error codes from the underlying db calls. If isPrivate is true
    _or_ f has been flagged as being in "private mode" then the new
    content is flagged as private. newId may be NULL, but if it is
    then the caller will have to find the record id himself by using
    the UUID (see fsl_uuid_to_rid()).
*/
FSL_EXPORT int fsl_content_new( fsl_cx * f, fsl_uuid_cstr uuid, bool isPrivate,
                                fsl_id_t * newId );

/** @internal

    Check to see if checkin "rid" is a leaf and either add it to the LEAF
    table if it is, or remove it if it is not.

    Returns 0 on success, FSL_RC_MISUSE if !f or f has no repo db
    opened, FSL_RC_RANGE if pid is <=0. Other errors
    (e.g. FSL_RC_DB) may indicate that db is not a repo. On error
    db's error state may be updated.
*/
FSL_EXPORT int fsl_repo_leaf_check(fsl_cx * f, fsl_id_t pid);  

/** @internal

    Schedules a leaf check for "rid" and its parents. Returns 0 on
    success.
*/
FSL_EXPORT int fsl_repo_leaf_eventually_check( fsl_cx * f, fsl_id_t rid);

/** @internal

    Perform all pending leaf checks. Returns 0 on success or if it
    has nothing to do.
*/
FSL_EXPORT int fsl_repo_leaf_do_pending_checks(fsl_cx *f);

/** @internal

    Inserts a tag into f's repo db. It does not create the related
    control artifact - use fsl_tag_add_artifact() for that.

    rid is the artifact to which the tag is being applied.

    srcId is the artifact that contains the tag. It is often, but
    not always, the same as rid. This is often the RID of the
    manifest containing tags added as part of the commit, in which
    case rid==srcId. A Control Artifact which tags a different
    artifact will have rid!=srcId.

    mtime is the Julian timestamp for the tag. Defaults to the
    current time if mtime <= 0.0.

    If outRid is not NULL then on success *outRid is assigned the
    record ID of the generated tag (the tag.tagid db field).

    If a more recent (compared to mtime) entry already exists for
    this tag/rid combination then its tag.tagid is returned via
    *outRid (if outRid is not NULL) and no new entry is created.

    Returns 0 on success, and has a huge number of potential error
    codes.
*/
FSL_EXPORT int fsl_tag_insert( fsl_cx * f,
                    fsl_tagtype_e tagtype,
                    char const * zTag,
                    char const * zValue,
                    fsl_id_t srcId,
                    double mtime,
                    fsl_id_t rid,
                    fsl_id_t *outRid );
/** @internal

    Propagate all propagatable tags in artifact pid to the children of
    pid. Returns 0 on... non-error. Returns FSL_RC_RANGE if pid<=0.
*/
FSL_EXPORT int fsl_tag_propagate_all(fsl_cx * f, fsl_id_t pid);

/** @internal

    Propagates a tag through the various internal pipelines.

    pid is the artifact id to whose children the tag should be
    propagated.

    tagid is the id of the tag to propagate (the tag.tagid db value).

    tagType is the type of tag to propagate. Must be either
    FSL_TAGTYPE_CANCEL or FSL_TAGTYPE_PROPAGATING. Note that
    FSL_TAGTYPE_ADD is not permitted.  The tag-handling internals
    (other than this function) translate ADD to CANCEL for propagation
    purposes. A CANCEL tag is used to stop propagation. (That's a
    historical behaviour inherited from fossil(1).) A potential TODO
    is for this function to simply treat ADD as CANCEL, without
    requiring that the caller be sure to never pass an ADD tag.

    origId is the artifact id of the origin tag if tagType ==
    FSL_TAGTYPE_PROPAGATING, otherwise it is ignored.

    zValue is the optional value for the tag. May be NULL.

    mtime is the Julian timestamp for the tag. Must be a valid time
    (no defaults here).

    This function is unforgiving of invalid values/ranges, and may assert
    in debug mode if passed invalid ids (values<=0), a NULL f, or if f has
    no opened repo.
*/
FSL_EXPORT int fsl_tag_propagate(fsl_cx *f,
                                 fsl_tagtype_e tagType,
                                 fsl_id_t pid,
                                 fsl_id_t tagid,
                                 fsl_id_t origId,
                                 const char *zValue,
                                 double mtime );

/** @internal

    Remove the PGP signature from a raw artifact, if there is one.

    Expects *pz to point to *pn bytes of string memory which might
    or might not be prefixed by a PGP signature.  If the string is
    enveloped in a signature, then upon returning *pz will point to
    the first byte after the end of the PGP header and *pn will
    contain the length of the content up to, but not including, the
    PGP footer.

    If *pz does not look like a PGP header then this is a no-op.

    Neither pointer may be NULL and *pz must point to *pn bytes of
    valid memory. If *pn is initially less than 59, this is a no-op.
*/
FSL_EXPORT void fsl_remove_pgp_signature(unsigned char const **pz, fsl_size_t *pn);

/** @internal

    Clears the "seen" cache used by manifest parsing. Should be
    called by routines which initialize parsing, but not until their
    work has finished all parsing (so that recursive parsing can
    use it).
*/
FSL_EXPORT void fsl_cx_clear_mf_seen(fsl_cx * f);

/** @internal

    Generates an fsl_appendf()-formatted message to stderr and
    fatally aborts the application by calling exit(). This is only
    (ONLY!) intended for us as a placeholder for certain test cases
    and is neither thread-safe nor reantrant.

    fmt may be empty or NULL, in which case only the code and its
    fsl_rc_cstr() representation are output.

    This function does not return.
*/
FSL_EXPORT void fsl_fatal( int code, char const * fmt, ... )
#ifdef __GNUC__
  __attribute__ ((noreturn))
#endif
  ;

/** @internal

    Translate a normalized, repo-relative filename into a
    filename-id (fnid). Create a new fnid if none previously exists
    and createNew is true. On success returns 0 and sets *rv to the
    filename.fnid record value. If createNew is false and no match
    is found, 0 is returned but *rv will be set to 0. Returns non-0
    on error.  Results are undefined if any parameter is NULL.


    In debug builds, this function asserts that no pointer arguments
    are NULL and that f has an opened repository.
*/
FSL_EXPORT int fsl_repo_filename_fnid2( fsl_cx * f, char const * filename,
                             fsl_id_t * rv, bool createNew );


/** @internal

    Clears and frees all (char*) members of db but leaves the rest
    intact. If alsoErrorState is true then the error state is also
    freed, else it is kept as well.
*/
FSL_EXPORT void fsl_db_clear_strings(fsl_db * db, bool alsoErrorState );

/** @internal

    Returns 0 if db appears to have a current repository schema, 1
    if it appears to have an out of date schema, and -1 if it
    appears to not be a repository. Results are undefined if db is
    NULL or not opened.
*/
FSL_EXPORT int fsl_db_repo_verify_schema(fsl_db * db);


/** @internal

    Flags for APIs which add phantom blobs to the repository.  The
    values in this enum derive from fossil(1) code and should not be
    changed without careful forethought and (afterwards) testing.  A
    phantom blob is a blob about whose existence we know but for which
    we have no content. This normally happens during sync or rebuild
    operations, but can also happen when artifacts are stored directly
    as files in a repo (like this project's repository does, storing
    artifacts from *other* projects for testing purposes).
*/
enum fsl_phantom_e {
/**
   Indicates to fsl_uuid_to_rid2() that no phantom artifact
   should be created.
*/
FSL_PHANTOM_NONE = 0,
/**
   Indicates to fsl_uuid_to_rid2() that a public phantom
   artifact should be created if no artifact is found.
*/
FSL_PHANTOM_PUBLIC = 1,
/**
   Indicates to fsl_uuid_to_rid2() that a private phantom
   artifact should be created if no artifact is found.
*/
FSL_PHANTOM_PRIVATE = 2
};
typedef enum fsl_phantom_e fsl_phantom_e;

/** @internal

    Works like fsl_uuid_to_rid(), with these differences:

    - uuid is required to be a complete UUID, not a prefix.

    - If it finds no entry and the mode argument specifies so then
    it will add either a public or private phantom entry and return
    its new rid. If mode is FSL_PHANTOM_NONE then this this behaves
    just like fsl_uuid_to_rid().

    Returns a positive value on success, 0 if it finds no entry and
    mode==FSL_PHANTOM_NONE, and a negative value on error (e.g.  if
    fsl_is_uuid(uuid) returns false). Errors which happen after
    argument validation will "most likely" update f's error state
    with details.
*/
FSL_EXPORT fsl_id_t fsl_uuid_to_rid2( fsl_cx * f, fsl_uuid_cstr uuid,
                           fsl_phantom_e mode );

/** @internal

    Schedules the given rid to be verified at the next commit. This
    is used by routines which add artifact records to the blob
    table.

    The only error case, assuming the arguments are valid, is an
    allocation error while appending rid to the internal to-verify
    queue.

    @see fsl_repo_verify_at_commit()
    @see fsl_repo_verify_cancel()
*/
FSL_EXPORT int fsl_repo_verify_before_commit( fsl_cx * f, fsl_id_t rid );

/** @internal

    Clears f's verify-at-commit list of RIDs.

    @see fsl_repo_verify_at_commit()
    @see fsl_repo_verify_before_commit()
*/
FSL_EXPORT void fsl_repo_verify_cancel( fsl_cx * f );

/** @internal

    Processes all pending verify-at-commit entries and clears the
    to-verify list. Returns 0 on success. On error f's error state
    will likely be updated.

    ONLY call this from fsl_db_transaction_end() or its delegate (if
    refactored).

    Verification calls fsl_content_get() to "unpack" content added in
    the current transaction. If fetching the content (which applies
    any deltas it may need to) fails or a checksum does not match then
    this routine fails and returns non-0. On error f's error state
    will be updated.

    @see fsl_repo_verify_cancel()
    @see fsl_repo_verify_before_commit()
*/
FSL_EXPORT int fsl_repo_verify_at_commit( fsl_cx * f );

/** @internal

    Removes all entries from the repo's blob table which are listed
    in the shun table. Returns 0 on success. This operation is
    wrapped in a transaction. Delta contant which depend on
    to-be-shunned content are replaced with their undeltad forms.

    Returns 0 on success.
*/
FSL_EXPORT int fsl_repo_shun_artifacts(fsl_cx * f);

/** @internal.

    Return a pointer to a string that contains the RHS of an SQL IN
    operator which will select config.name values that are part of
    the configuration that matches iMatch (a bitmask of
    fsl_configset_e values). Ownership of the returned string is
    passed to the caller, who must eventually pass it to
    fsl_free(). Returns NULL on allocation error.

    Reminder to self: this is part of the infrastructure for copying
    config state from an existing repo when creating new repo.
*/
FSL_EXPORT char *fsl_config_inop_rhs(int iMask);

/** @internal

    Return a pointer to a string that contains the RHS of an IN
    operator that will select config.name values that are in the
    list of control settings. Ownership of the returned string is
    passed to the caller, who must eventually pass it to
    fsl_free(). Returns NULL on allocation error.

    Reminder to self: this is part of the infrastructure for copying
    config state from an existing repo when creating new repo.
*/
FSL_EXPORT char *fsl_db_setting_inop_rhs();

/** @internal

    Creates the ticket and ticketchng tables in f's repository db,
    DROPPING them if they already exist. The schema comes from
    fsl_schema_ticket().

    FIXME: add a flag specifying whether to drop or keep existing
    copies.

    Returns 0 on success.
*/
FSL_EXPORT int fsl_cx_ticket_create_table(fsl_cx * f);

/** @internal

    Frees all J-card entries in the given list.

    li is assumed to be empty or contain (fsl_card_J*)
    instances. If alsoListMem is true then any memory owned
    by li is also freed. li itself is not freed.

    Results are undefined if li is NULL.
*/
FSL_EXPORT void fsl_card_J_list_free( fsl_list * li, bool alsoListMem );

/** @internal

    Values for fsl_card_J::flags.
*/
enum fsl_card_J_flags {
/**
   Sentinel value.
*/
FSL_CARD_J_INVALID = 0,
/**
   Indicates that the field is used by the ticket table.
*/
FSL_CARD_J_TICKET = 0x01,
/**
   Indicates that the field is used by the ticketchng table.
*/
FSL_CARD_J_CHNG = 0x02,
/**
   Indicates that the field is used by both the ticket and
   ticketchng tables.
*/
FSL_CARD_J_BOTH = FSL_CARD_J_TICKET | FSL_CARD_J_CHNG
};

/** @internal

    Loads all custom/customizable ticket fields from f's repo's
    ticket table info f. If f has already loaded the list and
    forceReload is false, this is a no-op.

    Returns 0 on success.

    @see fsl_cx::ticket::customFields
*/
FSL_EXPORT int fsl_cx_ticket_load_fields(fsl_cx * f, bool forceReload);

/** @internal

    A comparison routine for qsort(3) which compares fsl_card_J
    instances in a lexical manner based on their names. The order is
    important for card ordering in generated manifests.

    This routine expects to get passed (fsl_card_J**) (namely from
    fsl_list entries), and will not work on an array of J-cards.
*/
FSL_EXPORT int fsl_qsort_cmp_J_cards( void const * lhs, void const * rhs );

/** @internal

    The internal version of fsl_deck_parse(). See that function
    for details regarding everything but the 3rd argument.

    If you happen to know the _correct_ RID for the deck being
    parsed, pass it as the rid argument, else pass 0. A negative
    value will result in a FSL_RC_RANGE error. This value is (or
    will be) only used as an optimization in other places and only
    if d->f is not NULL.  Passing a positive value has no effect on
    how the content is parsed or on the result - it only affects
    internal details/optimizations.
*/
FSL_EXPORT int fsl_deck_parse2(fsl_deck * d, fsl_buffer * src, fsl_id_t rid);

/** @internal

    This function updates the repo and/or global config databases
    with links between the dbs intended for various fossil-level
    bookkeeping and housecleaning. These links are not essential to
    fossil's functionality but assist in certain "global"
    operations.

    If no checkout is opened but a repo is, the global config (if
    opened) is updated to know about the opened repo db.

    If a checkout is opened, global config (if opened) and the
    repo are updated to point to the checked-out db.
*/
FSL_EXPORT int fsl_repo_record_filename(fsl_cx * f);

/** @internal

    Updates f->ckout.uuid and f->ckout.rid to reflect the current
    checkout state. If no checkout is opened, the uuid is freed/NULLed
    and the rid is set to 0. Returns 0 on success. If it returns an
    error (OOM or db-related), the f->ckout state is left in a
    potentially inconsistent state, and it should not be relied upon
    until/unless the error is resolved.

    This is done when a checkout db is opened, when performing a
    checkin, and otherwise as needed, and so calling it from other
    code is normally not necessary.

    @see fsl_ckout_version_write()
*/
FSL_EXPORT int fsl_ckout_version_fetch( fsl_cx *f );

/** @internal

    Updates f->ckout's state to reflect the given version info and
    writes the 'checkout' and 'checkout-hash' properties to the
    currently-opened checkout db. Returns 0 on success,
    FSL_RC_NOT_A_CKOUT if no checkout is opened (may assert() in that
    case!), or some other code if writing to the db fails.

    If vid is 0 then the version info is null'd out. Else if uuid is
    NULL then fsl_rid_to_uuid() is used to fetch the UUID for vid.

    If the RID differs from f->ckout.rid then f->ckout's version state
    is updated to the new values.

    This routine also updates or removes the checkout's manifest
    files, as per fsl_ckout_manifest_write(). If vid is 0 then it
    removes any such files which themselves are not part of the
    current checkout.

    @see fsl_ckout_version_fetch()
    @see fsl_cx_ckout_version_set()
*/
FSL_EXPORT int fsl_ckout_version_write( fsl_cx *f, fsl_id_t vid,
                                        fsl_uuid_cstr uuid );

/**
   @internal
   
   Exports the file with the given [vfile].[id] to the checkout,
   overwriting (if possible) anything which gets in its way. If
   the file is determined to have not been modified, it is
   unchanged.

   If the final argument is not NULL then it is set to 0 if the file
   was not modified, 1 if only its permissions were modified, and 2 if
   its contents were updated (which also requires resetting its
   permissions to match their repo-side state).

   Returns 0 on success, any number of potential non-0 codes on
   error, including, but not limited to:

   - FSL_RC_NOT_A_CKOUT - no opened checkout.
   - FSL_RC_NOT_FOUND - no matching vfile entry.
   - FSL_RC_OOM - we cannot escape this eventuality.

   Trivia:

   - fossil(1)'s vfile_to_disk() is how it exports a whole vfile, or a
   single vfile entry, to disk. e.g. it performs a checkout that way,
   whereas we currently perform a checkout using the "repo extraction"
   API. The checkout mechanism was probably the first major core
   fossil feature which was structured radically differently in
   libfossil, compared to the feature's fossil counterpart, when it
   was ported over.

   - This routine always writes to the vfile.pathname entry, as
   opposed to vfile.origname.

   Maintenance reminders: internally this code supports handling
   multiple files at once, but (A) that's not part of the interface
   and may change and (B) the 3rd parameter makes little sense in that
   case unless maybe we change it to a callback, which seems like
   overkill for our use cases.
*/
FSL_EXPORT int fsl_vfile_to_ckout(fsl_cx * f, fsl_id_t vfileId,
                                  int * wasWritten);

/** @internal

    On Windows platforms (only), if fsl_isalpha(*zFile)
    and ':' == zFile[1] then this returns zFile+2,
    otherwise it returns zFile.
*/
FSL_EXPORT char * fsl_file_without_drive_letter(char * zFile);

/** @internal

    This is identical to the public-API member fsl_deck_F_search(),
    except that it returns a non-const F-card.

    Locate a file named zName in d->F.list.  Return a pointer to the
    appropriate fsl_card_F object. Return NULL if not found.
   
    If d->f is set (as it is when loading decks via
    fsl_deck_load_rid() and friends), this routine works even if p is
    a delta-manifest. The pointer returned might be to the baseline
    and d->B.baseline is loaded on demand if needed.
   
    If the returned card's uuid member is NULL, it means that the file
    was removed in the checkin represented by d.

    If !d, zName is NULL or empty, or FSL_SATYPE_CHECKIN!=d->type, it
    asserts in debug builds and returns NULL in non-debug builds.

    We assume that filenames are in sorted order and use a binary
    search. As an optimization, to support the most common use case,
    searches through a deck update d->F.cursor to the last position a
    search was found. Because searches are normally done in lexical
    order (because of architectural reasons), this is normally an O(1)
    operation. It degrades to O(N) if out-of-lexical-order searches
    are performed.
*/
FSL_EXPORT fsl_card_F * fsl_deck_F_seek(fsl_deck * const d, const char *zName);

/** @internal

    Part of the fsl_cx::fileContent optimization. This sets
    f->fileContent.used to 0 and if its capacity is over a certain
    (unspecified, unconfigurable) size then it is trimmed to that
    size.
*/
FSL_EXPORT void fsl_cx_content_buffer_yield(fsl_cx * f);

/** @internal

   Currently disabled (always returns 0) pending resolution of a
   "wha???" result from one of the underlying queries.

   Queues up the given artifact for a search index update. This is
   only intended to be called from crosslinking steps and similar
   content updates. Returns 0 on success.

   The final argument is intended only for wiki titles (the L-card of
   a wiki post).

   If the repository database has no search index or the given content
   is marked as private, this function returns 0 and makes no changes
   to the db.
*/
FSL_EXPORT int fsl_search_doc_touch(fsl_cx *f, fsl_satype_e saType, fsl_id_t rid,
                                    const char * docName);

/** @internal

   Performs the same job as fsl_diff_text() but produces the results
   in the low-level form of an array of "copy/delete/insert triples."
   This is primarily intended for internal use in other
   library-internal algorithms, not for client code. Note all
   FSL_DIFF_xxx flags apply to this form.

   Returns 0 on success, any number of non-0 codes on error. On
   success *outRaw will contain the resulting array, which must
   eventually be fsl_free()'d by the caller. On error *outRaw is not
   modified.
*/
FSL_EXPORT int fsl_diff_text_raw(fsl_buffer const *p1, fsl_buffer const *p2,
                                 int diffFlags, int ** outRaw);

/** @internal

   If the given file name is a reserved filename (case-insensitive) on
   Windows platforms, a pointer to the reserved part of the name, else
   NULL is returned.

   zPath must be a canonical path with forward-slash directory
   separators. nameLen is the length of zPath. If negative, fsl_strlen()
   is used to determine its length.
*/
FSL_EXPORT bool fsl_is_reserved_fn_windows(const char *zPath, fsl_int_t nameLen);

/** @internal

   Clears any pending merge state from the checkout db's vmerge table.
   Returns 0 on success.
*/
FSL_EXPORT int fsl_ckout_clear_merge_state( fsl_cx *f );


/** @internal

   Installs or reinstalls the checkout database schema into f's open
   checkout db. Returns 0 on success, FSL_RC_NOT_A_CKOUT if f has
   no opened checkout, or an code if a lower-level operation fails.

   If dropIfExists is true then all affected tables are dropped
   beforehand if they exist. "It's the only way to be sure."

   If dropIfExists is false and the schema appears to already exists
   (without actually validating its validity), 0 is returned.
*/
FSL_EXPORT int fsl_ckout_install_schema(fsl_cx *f, bool dropIfExists);

/** @internal

   Attempts to remove empty directories from under a checkout,
   starting with tgtDir and working upwards until it either cannot
   remove one or it reaches the top of the checkout dir.

   The second argument must be the canonicalized absolute path to some
   directory under the checkout root. The contents of the buffer may,
   for efficiency's sake, be modified by this routine as it traverses
   the directory tree. It will never grow the buffer but may mutate
   its memory's contents.

   Returns the number of directories it is able to remove.

   Results are undefined if tgtDir is not an absolute path rooted in
   f's current checkout.

   There are any number of valid reasons removal of a directory might
   fail, and this routine stops at the first one which does.
*/
FSL_EXPORT unsigned int fsl_ckout_rm_empty_dirs(fsl_cx * f, fsl_buffer * tgtDir);

/** @internal

   This is intended to be passed the name of a file which was just
   deleted and "might" have left behind an empty directory. The name
   _must_ an absolute path based in f's current checkout. This routine
   uses fsl_file_dirpart() to strip path components from the string
   and remove directories until either removing one fails or the top
   of the checkout is reach. Since removal of a directory can fail for
   any given reason, this routine ignores such errors. It returns 0 on
   success, FSL_RC_OOM if allocation of the working buffer for the
   filename hackery fails, and FSL_RC_MISUSE if zFilename is not
   rooted in the checkout (in which case it may assert(), so don't do
   that).

   @see fsl_is_rooted_in_ckout()
   @see fsl_rm_empty_dirs()
*/
FSL_EXPORT int fsl_ckout_rm_empty_dirs_for_file(fsl_cx * f, char const *zAbsPath);

/** @internal

    If f->cache.seenDeltaManifest<=0 then this routine sets it to 1
    and sets the 'seen-delta-manifest' repository config setting to 1,
    else this has no side effects. Returns 0 on success, non-0 if
    there is an error while writing to the repository config.
*/
FSL_EXPORT int fsl_cx_update_seen_delta_mf(fsl_cx *f);

/** @internal

   Very, VERY internal.

   Returns the next available buffer from f->scratchpads. Fatally
   aborts if there are no free buffers because "that should not
   happen."  Calling this obligates the caller to eventually pass
   its result to fsl_cx_scratchpad_yield().

   This function guarantees the returned buffer's 'used' member will be
   set to 0.

   Maintenance note: the number of buffers is hard-coded in the
   fsl_cx::scratchpads anonymous struct.
*/
FSL_EXPORT fsl_buffer * fsl_cx_scratchpad(fsl_cx *f);

/** @internal

   Very, VERY internal.

   "Yields" a buffer which was returned from fsl_cx_scratchpad(),
   making it available for re-use. The caller must treat the buffer as
   if this routine frees it: using the buffer after having passed it
   to this function will internally be flagged as explicit misuse and
   will lead to a fatal crash the next time that buffer is fetched via
   fsl_cx_scratchpad(). So don't do that.
*/
FSL_EXPORT void fsl_cx_scratchpad_yield(fsl_cx *f, fsl_buffer * b);


/** @internal

   Run automatically by fsl_deck_save(), so it needn't normally be run
   aside from that, at least not from average client code.

   Runs postprocessing on the Structural Artifact represented by
   d. d->f must be set, d->rid must be set and valid and d's contents
   must accurately represent the stored manifest for the given
   rid. This is normally run just after the insertion of a new
   manifest, but is sometimes also run after reading a deck from the
   database (in order to rebuild all db relations and add/update the
   timeline entry).

   Returns 0 on succes, FSL_RC_MISUSE if !d or !d->f, FSL_RC_RANGE if
   d->rid<=0, FSL_RC_MISUSE (with more error info in f) if d does not
   contain all required cards for its d->type value. It may return
   various other codes from the many routines it delegates work to.

   Crosslinking of ticket artifacts is currently (2021-03) missing.

   Design note: d "really should" be const here but some internals
   (d->F.cursor and delayed baseline loading) prohibit it.

   @see fsl_deck_crosslink_one()
*/
FSL_EXPORT int fsl_deck_crosslink( fsl_deck /* const */ * d );

/** @internal

   Run automatically by fsl_deck_save(), so it needn't normally be run
   aside from that, at least not from average client code.

   This is a convience form of crosslinking which must only be used
   when a single deck (and only a single deck) is to be crosslinked.
   This function wraps the crosslinking in fsl_crosslink_begin()
   and fsl_crosslink_end(), but otherwise behaves the same as
   fsl_deck_crosslink(). If crosslinking fails, any in-progress
   transaction will be flagged as failed.

   Returns 0 on success.
*/
FSL_EXPORT int fsl_deck_crosslink_one( fsl_deck * d );

/** @internal

   Checks whether the given filename is "safe" for writing to within
   f's current checkout.

   zFilename must be in canonical form: only '/' directory separators.
   If zFilename is not absolute, it is assumed to be relative to the top
   of the current checkout, else it must point to a file under the current
   checkout.

   Checks made on the filename include:

   - It must refer to a file under the current checkout.

   - Ensure that each directory listed in the file's path is actually
   a directory, and fail if any part other than the final one is a
   non-directory.

   If the name refers to something not (yet) in the filesystem, that
   is not considered an error.

   Returns 0 on success. On error f's error state is updated with
   information about the problem.
*/
FSL_EXPORT int fsl_ckout_safe_file_check(fsl_cx *f, char const * zFilename);

/** @internal
   UNTESTED!

   Creates a file named zLinkFile and populates its contents with a
   single line: zTgtFile. This behaviour corresponds to how fossil
   manages SCM'd symlink entries on Windows and on other platforms
   when the 'allow-symlinks' repo-level config setting is disabled.
   (In late 2020 fossil defaulted that setting to disabled and made it
   non-versionable.)

   zLinkFile may be an absolute path rooted at f's current checkout or
   may be a checkout-relative path.

   Returns 0 on success, non-0 on error:

   - FSL_RC_NOT_A_CKOUT if f has no opened checkout.

   - FSL_RC_MISUSE if zLinkFile refers to a path outside of the
   current checkout.

   Potential TODO (maybe!): handle symlinks as described above or
   "properly" on systems which natively support them iff f's
   'allow-symlinks' repo-level config setting is true. That said: the
   addition of symlinks support into fossil was, IMHO, a poor decision
   for $REASONS. That might (might) be reflected long-term in this API
   by only supporting them in the way fossil does for platforms which
   do not support symlinks.
*/
FSL_EXPORT int fsl_ckout_symlink_create(fsl_cx * f, char const *zTgtFile,
                                        char const * zLinkFile);


/**
   Compute all file name changes that occur going from check-in iFrom
   to check-in iTo. Requires an opened repository.

   If revOK is true, the algorithm is free to move backwards in the
   chain. This is the opposite of the oneWayOnly parameter for
   fsl_vpath_shortest().

   On success, the number of name changes is written into *pnChng.
   For each name change, two integers are allocated for *piChng. The
   first is the filename.fnid for the original name as seen in
   check-in iFrom and the second is for new name as it is used in
   check-in iTo. If *pnChng is 0 then *aiChng will be NULL.

   On error returns non-0, pnChng and aiChng are not modified, and
   f's error state might (depending on the error) contain a description
   of the problem.

   Space to hold *aiChng is obtained from fsl_malloc() and must
   be released by the caller.
*/
FSL_EXPORT int fsl_cx_find_filename_changes(fsl_cx * f,
                                            fsl_id_t iFrom,
                                            fsl_id_t iTo,
                                            bool revOK,
                                            uint32_t *pnChng,
                                            fsl_id_t **aiChng);

/**
   Bitmask of file change types for use with
   fsl_is_locally_modified().
 */
enum fsl_localmod_e {
/** Sentinel value. */
FSL_LOCALMOD_NONE = 0,
/**
   Permissions changed.
*/
FSL_LOCALMOD_PERM = 0x01,
/**
   File size or hash (i.e. content) differ.
*/
FSL_LOCALMOD_CONTENT = 0x02,
/**
   The file type was switched between symlink and normal file.  In
   this case, no check for content change, beyond the file size
   change, is performed.
*/
FSL_LOCALMOD_LINK = 0x04,
/**
   File was not found in the local checkout.
 */
FSL_LOCALMOD_NOTFOUND = 0x10
};
typedef enum fsl_localmod_e fsl_localmod_e;
/** @internal

   Checks whether the given file has been locally modified compared to
   a known size, hash value, and permissions. Requires that f has an
   opened checkout.

   If zFilename is not an absolute path, it is assumed to be relative
   to the checkout root (as opposed to the current directory) and is
   canonicalized into an absolute path for purposes of this function.

   fileSize is the "original" version's file size.  zOrigHash is the
   initial hash of the file to use as a basis for comparison.
   zOrigHashLen is the length of zOrigHash, or a negative value if
   this function should use fsl_is_uuid() to determine the length. If
   the hash length is not that of one of the supported hash types,
   FSL_RC_RANGE is returned and f's error state is updated. This
   length is used to determine which hash to use for the comparison.

   If the file's current size differs from the given size, it is
   quickly considered modified, otherwise the file's contents get
   hashed and compared to zOrigHash.

   Because this is used for comparing local files to their state from
   the fossil database, where files have no timestamps, the local
   file's timestamp is never considered for purposes of modification
   checking.

   If isModified is not NULL then on success it is set to a bitmask of
   values from the fsl_localmod_e enum specifying the type(s) of
   change(s) detected:

   - FSL_LOCALMOD_PERM = permissions changed.

   - FSL_LOCALMOD_CONTENT = file size or hash (i.e. content) differ.

   - FSL_LOCALMOD_LINK = the file type was switched between symlink
     and normal file. In this case, no check for content change,
     beyond the file size change, is performed.

   - FSL_LOCALMOD_NOFOUND = file was not found in the local checkout.

   Noting that:

   - Combined values of (FSL_LOCALMOD_PERM | FSL_LOCALMOD_CONTENT) are
   possible, but FSL_LOCALMOD_NOFOUND will never be combined with one
   of the other values.

   If stat() fails for any reason other than file-not-found
   (e.g. permissions), an error is triggered.

   Returns 0 on success. On error, returns non-0 and f's error state
   will be updated and isModified...  isNotModified. Errors include,
   but are not limited to:

   - Invalid hash length: FSL_RC_RANGE
   - f has no opened checkout: FSL_RC_NOT_A_CKOUT
   - Cannot find the file: FSL_RC_NOT_FOUND
   - Error accessing the file: FSL_RC_ACCESS
   - Allocation error: FSL_RC_OOM
   - I/O error during hashing: FSL_RC_IO

   And potentially other errors, roughly translated from errno values,
   for corner cases such as passing a directory name instead of a
   file.

   Results are undefined if any pointer argument is NULL or invalid.

   This function currently does NOT follow symlinks for purposes of
   resolving zFilename, but that behavior may change in the future or
   may become dependent on the repository's 'allow-symlinks' setting.

   Internal detail, not relevant for clients: this updates f's
   cache stat entry.
*/
FSL_EXPORT int fsl_is_locally_modified(fsl_cx * f,
                                       const char * zFilename,
                                       fsl_size_t fileSize,
                                       const char * zOrigHash,
                                       fsl_int_t zOrigHashLen,
                                       fsl_fileperm_e origPerm,
                                       int * isModified);

/** @internal

   This routine cleans up the state of selected cards in the given
   deck. The 2nd argument is an list of upper-case letters
   representing the cards which should be cleaned up, e.g. "ADG". If
   it is NULL, all cards are cleaned up but d has non-card state
   which is not cleaned up by this routine. Unknown letters are simply
   ignored.
*/
FSL_EXPORT void fsl_deck_clean_cards(fsl_deck * d, char const * letters);

/** @internal

   Searches the current repository database for a fingerprint and
   returns it as a string in *zOut.

   If rcvid<=0 then the fingerprint matches the last entry in the
   [rcvfrom] table, where "last" means highest-numbered rcvid (as
   opposed to most recent mtime, for whatever reason). If rcvid>0 then
   it searches for an exact match.

   Returns 0 on non-error, where finding no matching rcvid causes
   FSL_RC_NOT_FOUND to be returned. If 0 is returned then *zOut will
   be non-NULL and ownership of that value is transferred to the
   caller, who must eventually pass it to fsl_free(). On error, *zOut
   is not modified.

   Returns FSL_RC_NOT_A_REPO if f has no opened repository, FSL_RC_OOM
   on allocation error, or any number of potential db-related codes if
   something goes wrong at the db level.

   This API internally first checks for "version 1" fossil
   fingerprints and falls back to "version 0" fingerprint if a v1
   fingerprint is not found. Version 0 was very short-lived and is not
   expected to be in many repositories which are accessed via this
   library. Practice has, however, revealed some.

   @see fsl_ckout_fingerprint_check()
*/
FSL_EXPORT int fsl_repo_fingerprint_search(fsl_cx *f, fsl_id_t rcvid, char ** zOut);

/**
   A context for running a raw diff.
  
   The aEdit[] array describes the raw diff.  Each triple of integers in
   aEdit[] means:
  
     (1) COPY:   Number of lines aFrom and aTo have in common
     (2) DELETE: Number of lines found only in aFrom
     (3) INSERT: Number of lines found only in aTo
  
   The triples repeat until all lines of both aFrom and aTo are accounted
   for.
*/
struct fsl_diff_cx {
  /*TODO unsigned*/ int *aEdit;        /* Array of copy/delete/insert triples */
  /*TODO unsigned*/ int nEdit;         /* Number of integers (3x num of triples) in aEdit[] */
  /*TODO unsigned*/ int nEditAlloc;    /* Space allocated for aEdit[] */
  fsl_dline *aFrom;      /* File on left side of the diff */
  /*TODO unsigned*/ int nFrom;         /* Number of lines in aFrom[] */
  fsl_dline *aTo;        /* File on right side of the diff */
  /*TODO unsigned*/ int nTo;           /* Number of lines in aTo[] */
  int (*cmpLine)(const fsl_dline * const, const fsl_dline *const); /* Function to be used for comparing */
};
/**
   Convenience typeef.
*/
typedef struct fsl_diff_cx fsl_diff_cx;
/** Initialized-with-defaults fsl_diff_cx structure, intended for
    const-copy initialization. */
#define fsl_diff_cx_empty_m {\
  NULL,0,0,NULL,0,NULL,0,fsl_dline_cmp \
}
/** Initialized-with-defaults fsl_diff_cx structure, intended for
    non-const copy initialization. */
extern const fsl_diff_cx fsl_diff_cx_empty;



/** @internal

    Compute the differences between two files already loaded into
    the fsl_diff_cx structure.
   
    A divide and conquer technique is used.  We look for a large
    block of common text that is in the middle of both files.  Then
    compute the difference on those parts of the file before and
    after the common block.  This technique is fast, but it does
    not necessarily generate the minimum difference set.  On the
    other hand, we do not need a minimum difference set, only one
    that makes sense to human readers, which this algorithm does.
   
    Any common text at the beginning and end of the two files is
    removed before starting the divide-and-conquer algorithm.
   
    Returns 0 on succes, FSL_RC_OOM on an allocation error.
*/
int fsl__diff_all(fsl_diff_cx * const p);

/** @internal
 */
void fsl__diff_optimize(fsl_diff_cx * const p);

/** @internal
 */
void fsl__diff_cx_clean(fsl_diff_cx * const cx);

/** @internal

    Undocumented. For internal debugging only.
 */
void fsl__dump_triples(fsl_diff_cx const * const p,
                       char const * zFile, int ln );

#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_INTERNAL_H_INCLUDED */
/* end of file ../include/fossil-scm/fossil-internal.h */
/* start of file ../include/fossil-scm/fossil-auth.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_AUTH_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_AUTH_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

  ******************************************************************************
  This file declares public APIs for handling fossil
  authentication-related tasks.
*/


#if defined(__cplusplus)
extern "C" {
#endif

/**
   If f has an opened repository, this function forms a hash from:

   "ProjectCode/zLoginName/zPw"

   (without the quotes)

   where ProjectCode is a repository-instance-dependent series of
   random bytes. The returned string is owned by the caller, who
   must eventually fsl_free() it. The project code is stored in
   the repository's config table under the key 'project-code', and
   this routine fetches that key if necessary.

   Potential TODO:

   - in fossil(1), this function generates a different result (it
   returns a copy of zPw) if the project code is not set, under
   the assumption that this is "the first xfer request of a
   clone."  Whether or not that will apply at this level to
   libfossil remains to be seen.

   TODO? Does fossil still use SHA1 for this?
*/
FSL_EXPORT char * fsl_sha1_shared_secret( fsl_cx * f, char const * zLoginName, char const * zPw );

/**
   Fetches the login group name (if any) for the given context's
   current repositorty db. If f has no opened repo, 0 is returned.

   If the repo belongs to a login group, its name is returned in the
   form of a NUL-terminated string. The returned value (which may be
   0) is owned by the caller, who must eventually fsl_free() it. The
   value (unlike in fossil(1)) is not cached because it may change
   via modification of the login group.
*/
FSL_EXPORT char * fsl_repo_login_group_name(fsl_cx * f);

/**
   Fetches the login cookie name associated with the current repository
   db, or 0 if no repository is opened.

   The returned (NUL-terminated) string is owned by the caller, who
   must eventually fsl_free() it. The value is not cached in f because
   it may change during the lifetime of a repo (if a login group is
   set or removed).

   The login cookie name is a string in the form "fossil-XXX", where
   XXX is the first 16 hex digits of either the repo's
   'login-group-code' or 'project-code' config values (in that order).
*/
FSL_EXPORT char * fsl_repo_login_cookie_name(fsl_cx * f);

/**
   Searches for a user ID (from the repo.user.uid DB field) for a given
   username and password. The password may be either its hashed form or
   non-hashed form (if it is not exactly 40 bytes long, that is!).

   On success, 0 is returned and *pId holds the ID of the
   user found (if any).  *pId will be set to 0 if no match for the
   name/password was found, or positive if a match was found.

   If any of the arguments are NULL, FSL_RC_MISUSE is returned. f must
   have an opened repo, else FSL_RC_NOT_A_REPO is returned.

*/
FSL_EXPORT int fsl_repo_login_search_uid(fsl_cx * f, char const * zUsername,
                                         char const * zPasswd, fsl_id_t * pId);

/**
   Clears all login state for the given user ID. If the ID is <=0 then
   ALL logins are cleared. Has no effect on the built-in pseudo-users.

   Returns non-0 on error, and not finding a matching user ID is not
   considered an error.

   f must have an opened repo, or FSL_RC_NOT_A_REPO is returned.

   TODO: there are currently no APIs for _setting_ the state this
   function clears!
*/
FSL_EXPORT int fsl_repo_login_clear( fsl_cx * f, fsl_id_t userId );


#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_AUTH_H_INCLUDED */
/* end of file ../include/fossil-scm/fossil-auth.h */
/* start of file ../include/fossil-scm/fossil-forum.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_FORUM_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_FORUM_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

  ******************************************************************************
  This file declares public APIs for working with fossil-managed forum
  content.
*/


#if defined(__cplusplus)
extern "C" {
#endif

/**
   If the given fossil context has a db opened, this function
   installs, if needed, the forum-related schema and returns 0 on
   success (or if no installation was needed). If f has no repository
   opened, FSL_RC_NOT_A_REPO is returned. Some other FSL_RC_xxx value
   is returned if there is a db-level error during installation.
*/
int fsl_repo_install_schema_forum(fsl_cx *f);


#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_FORUM_H_INCLUDED */
/* end of file ../include/fossil-scm/fossil-forum.h */
/* start of file ../include/fossil-scm/fossil-pages.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_PAGES_H_INCLUDED)
#define ORG_FOSSIL_SCM_PAGES_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

  *****************************************************************************
  This file contains only Doxygen-format documentation, split up into
  Doxygen "pages", each covering some topic at a high level.  This is
  not the place for general code examples - those belong with their
  APIs.
*/

/** @mainpage libfossil

    Forewarning: this API assumes one is familiar with the Fossil SCM,
    ideally in detail. The Fossil SCM can be found at:

    https://fossil-scm.org

    libfossil is an experimental/prototype library API for the Fossil
    SCM. This API concerns itself only with the components of fossil
    which do not need user interaction or the display of UI components
    (including HTML and CLI output). It is intended only to model the
    core internals of fossil, off of which user-level applications
    could be built.

    The project's repository and additional information can be found at:

    https://fossil.wanderinghorse.net/r/libfossil/

    This code is 100% hypothetical/potential, and does not represent
    any Official effort of the Fossil project. It is up for any amount
    of change at any time and does not yet have a stable API.

    All Fossil users are encouraged to participate in its development,
    but if you are reading this then you probably already knew that
    :).

    This effort does not represent "Fossil Version 2", but provides an
    alternate method of accessing and manipulating fossil(1)
    repositories. Whereas fossil(1) is a monolithic binary, this API
    provides library-level access to (some level of) the fossil(1)
    feature set (that level of support grows approximately linearly
    with each new commit).

    Current status: alpha. Some bits are basically finished but there
    is a lot of work left to do. The scope is pretty much all
    Fossil-related functionality which does not require a user
    interface or direct user interaction, plus some range of utilities
    to support those which require a UI/user.
*/

/** @page page_terminology Fossil Terminology

    See also: https://fossil-scm.org/home/doc/trunk/www/concepts.wiki

    The libfossil API docs normally assume one is familiar with
    Fossil-internal terminology, which is of course a silly assumption
    to make. Indeed, one of libfossil's goals is to make Fossil more
    accessible, partly be demystifying it. To that end, here is a
    collection of terms one may come across in the API, along with
    their meanings in the context of Fossil...


    - REPOSITORY (a.k.a. "repo) is an sqlite database file which
    contains all content for a given "source tree." (We will use the
    term "source tree" to mean any tree of "source" (documents,
    whatever) a client has put under Fossil's supervision.)

    - CHECKOUT (a.k.a. "local source tree" or "working copy") refers
    to (A) the action of pulling a specific version of a repository's
    state from that repo into the local filesystem, and (B) a local
    copy "checked out" of a repo. e.g. "he checked out the repo," and
    "the changes are in his [local] checkout."

    - ARTIFACT is the generic term for anything stored in a repo. More
    specifically, ARTIFACT refers to "control structures" Fossil uses
    to internally track changes. These artifacts are stored as blobs
    in the database, just like any other content. For complete details
    and examples, see:
    https://fossil-scm.org/home/doc/tip/www/fileformat.wiki

    - A MANIFEST is a specific type of ARTIFACT - the type which
    records all metadata for a COMMIT operation (which files, which
    user, the timestamp, checkin comment, lineage, etc.). For
    historical reasons, MANIFEST is sometimes used as a generic term
    for ARTIFACT because what the fossil(1)-internal APIs originally
    called a Manifest eventually grew into other types of artifacts
    but kept the Manifest naming convention. In Fossil developer
    discussion, "manifest" most often means what this page calls
    ARTIFACT (probably because that how the C code is modelled).  The
    libfossil API calls uses the term "deck" instead of "manifest" to
    avoid ambiguity/confusion (or to move the confusion somewhere
    else, at least).

    - CHECKIN is the term libfossil prefers to use for COMMIT
    MANIFESTS. It is also the action of "checking in"
    (a.k.a. "committing") file changes to a repository.  A CHECKIN
    ARTIFACT can be one of two types: a BASELINE MANIFEST (or BASELINE
    CHECKIN) contains a list of all files in that version of the
    repository, including their file permissions and the UUIDs of
    their content. A DELTA MANFIEST is a checkin record which derives
    from a BASELINE MANIFEST and it lists only the file-level changes
    which happened between the baseline and the delta, recording any
    changes in content, permisions, or name, and recording
    deletions. Note that this inheritance of deltas from baselines is
    an internal optimization which has nothing to do with checkin
    version inheritance - the baseline of any given delta is normally
    _not_ its direct checkin version parent.

    - BRANCH, FORK, and TAG are all closely related in Fossil and are
    explained in detail (with pictures!) at:
    https://fossil-scm.org/home/doc/trunk/www/concepts.wiki
    In short: BRANCHes and FORKs are two names for the same thing, and
    both are just a special-case usage of TAGs.

    - MERGE or MERGING: the process of integrating one version of
    source code into another version of that source code, using a
    common parent version as the basis for comparison. This is
    normally fully automated, but occasionally human (and sometimes
    Divine) intervention is required to resolve so-called "merge
    conflicts," where two versions of a file change the same parts of
    a common parent version.

    - RID (Record ID) is a reference to the blob.rid field in a
    repository DB. RIDs are used extensively throughout the API for
    referencing content records, but they are transient values local
    to a given copy of a given repository at a given point in
    time. They _can_ change, even for the same content, (e.g. a
    rebuild can hypothetically change them, though it might not, and
    re-cloning a repo may very well change some RIDs). Clients must
    never rely on them for long-term reference to SCM'd data - always use
    the full UUID of such data. Even though they normally appear to be
    static, they are most explicitly NOT guaranteed to be. Nor are
    their values guaranteed to imply any meaning, e.g. "higher is
    newer" is not necessarily true because synchronization can import
    new remote content in an arbitrary order and a rebuild might
    import it in random order. The API uses RIDs basically as handles
    to arbitrary blob content and, like most C-side handles, must be
    considered transient in nature. That said, within the db, records
    are linked to each other exclusively using RIDs, so they do have
    some persistence guarantees for a given db instance.
*/


/** @page page_APIs High-level API Overview

    The primary end goals of this project are to eventually cover the
    following feature areas:

    - Provide embeddable SCM to local apps using sqlite storage.
    - Provide a network layer on top of that for synchronization.
    - Provide apps on top of those to allow administration of repos.

    To those ends, the fossil APIs cover the following categories of
    features:

    Filesystem:

    - Conversions of strings from OS-native encodings to UTF.
    fsl_utf8_to_unicode(), fsl_filename_to_utf8(), etc. These are
    primarily used internally but may also be useful for applications
    working with files (as most clients will). Actually... most of
    these bits are only needed for portability across Windows
    platforms.

    - Locating a user's home directory: fsl_find_home_dir()

    - Normalizing filenames/paths. fsl_file_canonical_name() and friends.

    - Checking for existence, size, and type (file vs directory) with
    fsl_is_file() and fsl_dir_check(), or the more general-purpose
    fsl_stat().


    Databases (sqlite):

    - Opening/closing sqlite databases and running queries on them,
    independent of version control features. See fsl_db_open() and
    friends. The actual sqlite-level DB handle type is abstracted out
    of the public API, largely to simplify an eventual port from
    sqlite3 to sqlite4 or (hypothetically) to other storage back-ends
    (not gonna happen - too much work).

    - There are lots of utility functions for oft-used operations,
    e.g. fsl_config_get_int32() and friends to fetch settings from one
    of several different configuration areas (global, repository,
    checkout, and "versionable" settings).

    - Pseudo-recusive transactions: fsl_db_transaction_begin() and
    fsl_db_transaction_end(). sqlite does not support truly nested
    transactions, but they can be simulated quite effectively so long
    as certain conventions are adhered to.

    - Cached statements (an optimization for oft-used queries):
    fsl_db_prepare_cached() and friends.


    The DB API is (as Brad Harder put so well) "very present" in the
    public API. While the core API provides access to the underlying
    repository data, it cannot begin to cover even a small portion of
    potential use cases. To that end, it exposes the DB API so that
    clients who want to custruct their own data can do so. It does
    require research into the underlying schemas, but gives
    applications the ability to do _anything_ with their repositories
    which the core API does not account for. Historically, the ability
    to create ad-hoc data structures as needed, in the form of SQL
    queries, has accounted for much of Fossil's feature flexibility.


    Deltas:

    - Creation and application of raw deltas, using Fossil's delta
    format, independent of version control features. See
    fsl_delta_create() and friends. These are normally used only at
    the deepest internal levels of fossil, but the APIs are exposed so
    that clients can, if they wish, use them to deltify their own
    content independently of fossil's internally-applied
    deltification. Doing so is remarkably easy, but completely
    unnecessary for content which will be stored in a repo, as Fossil
    creates deltas as needed.


    SCM:

    - A "context" type (fsl_cx) which manages a repository db and,
    optionally, a checkout db. Read-only operations on the DB are
    working and write functionality (adding repo content) is
    ongoing. See fsl_cx, fsl_cx_init(), and friends.

    - The fsl_deck class assists in parsing, creating, and outputing
    "artifacts" (manifests, control (tags), events, etc.). It gets its
    name from it being container for "a collection of cards" (which is
    what a Fossil artifact is).

    - fsl_content_get() expands a (possibly) deltified blob into its
    full form, and fsl_content_blob() can be used to fetch a raw blob
    (possibly a raw delta).

    - A number of routines exist for converting symbol names to RIDs
    (fsl_sym_to_rid()), UUIDs to RIDs (fsl_uuid_to_rid(),
    and similar commonly-needed lookups.


    Input/Output:

    - The API defines several abstractions for i/o interfaces, e.g.
    fsl_input_f() and fsl_output_f(), which allow us to accept/emit
    data from/to arbitrary streamable (as opposed to random-access)
    sources/destinations. A fsl_cx instance is configured with an
    output channel, the intention being that all clients of that
    context should generate any output through that channel, so that
    all compatible apps can cooperate more easily in terms of i/o. For
    example, the s2 script binding for libfossil routes fsl_output()
    through the script engine's i/o channels, so that any output
    generated by libfossil-using code it links to can take advantage
    of the script-side output features (such as output buffering,
    which is needed for any non-trivial CGI output). That said: the
    library-level code does not actually generate output to that
    channel, but higher-level code like fcli does, and clients are
    encouraged to in order to enable their app's output to be
    redirected to an arbitrary UI element, be it a console or UI
    widget.


    Utilities:

    - fsl_buffer, a generic buffer class, is used heavily by the
    library.  See fsl_buffer and friends.

    - fsl_appendf() provides printf()-like functionality, but sends
    its output to a callback function (optionally stateful), making it
    the one-stop-shop for string formatting within the library.

    - The fsl_error class is used to propagate error information
    between the libraries various levels and the client.

    - The fsl_list class acts as a generic container-of-pointers, and
    the API provides several convenience routines for managing them,
    traversing them, and cleaning them up.

    - Hashing: there are a number of routines for calculating SHA1,
    SHA3, and MD5 hashes. See fsl_sha1_cx, fsl_sha3_cx, fsl_md5_cx,
    and friends.

    - zlib compression is used for storing artifacts. See
    fsl_data_is_compressed(), fsl_buffer_compress(), and friends.
    These are never needed at the client level, but are exposed "just
    in case" a given client should want them.
*/

/** @page page_is_isnot Fossil is/is not...

    Through porting the main fossil application into library form,
    the following things have become very clear (or been reinforced)...

    Fossil is...

    - _Exceedingly_ robust. Not only is sqlite literally the single
    most robust application-agnostic container file format on the
    planet, but Fossil goes way out of its way to ensure that what
    gets put in is what gets pulled out. It cuts zero corners on data
    integrity, even adding in checks which seem superfluous but
    provide another layer of data integrity (i'm primarily talking
    about the R-card here, but there are other validation checks). It
    does this at the cost of memory and performance (that said, it's
    still easily fast enough for its intended uses). "Robust" doesn't
    mean that it never crashes nor fails, but that it does so with
    (insofar as is technically possible) essentially zero chance of
    data loss/corruption.

    - Long-lived: the underlying data format is independent of its
    storage format. It is, in principal, usable by systems as yet
    unconceived by the next generation of programmers. This
    implementation is based on sqlite, but the model can work with
    arbitrary underlying storage.

    - Amazingly space-efficient. The size of a repository database
    necessarily grows as content is modified. However, Fossil's use of
    zlib-compressed deltas, using a very space-efficient delta format,
    leads to tremendous compression ratios. As of this writing (March,
    2021), the main Fossil repo contains approximately 5.36GB of
    content, were we to check out every single version in its
    history. Its repository database is only 64MB, however, equating
    to a 83:1 compression ration. Ratios in the range of 20:1 to 40:1
    are common, and more active repositories tend to have higher
    ratios. The TCL core repository, with just over 15 years of code
    history (imported, of course, as Fossil was introduced in 2007),
    is (as of September 2013) only 187MB, with 6.2GB of content and a
    33:1 compression ratio.

    Fossil is not...

    - Memory-light. Even very small uses can easily suck up 1MB of RAM
    and many operations (verification of the R card, for example) can
    quickly allocate and free up hundreds of MB because they have to
    compose various versions of content on their way to a specific
    version. To be clear, that is total RAM usage, not _peak_ RAM
    usage. Peak usage is normally a function of the content it works
    with at a given time, often in direct relation to (but
    significantly more than) the largest single file processed in a
    given session. For any given delta application operation, Fossil
    needs the original content, the new content, and the delta all in
    memory at once, and may go through several such iterations while
    resolving deltified content. Verification of its 'R-card' alone
    can require a thousand or more underlying DB operations and
    hundreds of delta applications. The internals use caching where it
    would save us a significant amount of db work relative to the
    operation in question, but relatively high memory costs are
    unavoidable. That's not to say we can't optimize a bit, but first
    make it work, then optimize it. The library takes care to re-use
    memory buffers where it is feasible (and not too intrusive) to do
    so, but there is yet more RAM to be optimized away in this regard.
*/

/** @page page_threading Threads and Fossil

    It is strictly illegal to use a given fsl_cx instance from more
    than one thread. Period.

    It is legal for multiple contexts to be running in multiple
    threads, but only if those contexts use different
    repository/checkout databases. Though access to the storage is,
    through sqlite, protected via a mutex/lock, this library does not
    have a higher-level mutex to protect multiple contexts from
    colliding during operations. So... don't do that. One context, one
    repo/checkout.

    Multiple application instances may each use one fsl_cx instance to
    share repo/checkout db files, but must be prepared to handle
    locking-related errors in such cases. e.g. db operations which
    normally "always work" may suddenly pause for a few seconds before
    giving up while waiting on a lock when multiple applications use
    the same database files. sqlite's locking behaviours are
    documented in great detail at https://sqlite.org.
 */

/** @page page_artifacts Creating Artifacts

    A brief overview of artifact creating using this API. This is targeted
    at those who are familiar with how artifacts are modelled and generated
    in fossil(1).

    Primary artifact reference:

    https://fossil-scm.org/home/doc/trunk/www/fileformat.wiki

    In fossil(1), artifacts are generated via the careful crafting of
    a memory buffer (large string) in the format described in the
    document above. While it's relatively straightforward to do, there
    are lots of potential gotchas, and a bug can potentially inject
    "bad data" into the repo (though the verify-before-commit process
    will likely catch any problems before the commit is allowed to go
    through). The libfossil API uses a higher-level (OO) approach,
    where the user describes a "deck" of cards and then tells the
    library to save it in the repo (fsl_deck_save()) or output it to
    some other channel (fsl_deck_output()). The API ensures that the
    deck's cards get output in the proper order and that any cards
    which require special treatment get that treatment (e.g. the
    "fossilize" encoding of certain text fields). The "deck" concept
    is equivalent to Artifact in fossil(1), but we use the word deck
    because (A) Artifact is highly ambiguous in this context and (B)
    deck is arguably the most obvious choice for the name of a type
    which acts as a "container of cards."

    Ideally, client-level code will never have to create an artifact
    via the fsl_deck API (because doing so requires a fairly good
    understanding of what the deck is for in the first place,
    including the individual Cards). The public API strives to hide
    those levels of details, where feasible, or at least provide
    simpler/safer alternatives for basic operations. Some operations
    may require some level of direct work with a fsl_deck
    instance. Likewise, much read-only functionality directly exposes
    fsl_deck to clients, so some familiarity with the type and its
    APIs will be necessary for most clients.

    The process of creating an artifact looks a lot like the following
    code example. We have elided error checking for readability
    purposes, but in fact this code has undefined behaviour if error
    codes are not checked and appropriately reacted to.

    @code
    fsl_deck deck = fsl_deck_empty;
    fsl_deck * d = &deck; // for typing convenience
    fsl_deck_init( fslCtx, d, FSL_SATYPE_CONTROL ); // must come first
    fsl_deck_D_set( d, fsl_julian_now() );
    fsl_deck_U_set( d, "your-fossil-name", -1 );
    fsl_deck_T_add( d, FSL_TAGTYPE_ADD, "...uuid being tagged...",
                   "tag-name", "optional tag value");
    ...
    // Now output it to stdout:
    fsl_deck_output( f, d, fsl_output_f_FILE, stdout );
    // See also: fsl_deck_save(), which stores it in the db and
    // "crosslinks" it.
    fsl_deck_finalize(d);
    @endcode

    The order the cards are added to the deck is irrelevant - they
    will be output in the order specified by the Fossil specs
    regardless of their insertion order. Each setter/adder function
    knows, based on the deck's type (set via fsl_deck_init()), whether
    the given card type is legal, and will return an error (probably
    FSL_RC_TYPE) if an attempt is made to add a card which is illegal
    for that deck type. Likewise, fsl_deck_output() and
    fsl_deck_save() confirm that the decks they are given contain (A)
    only allowed cards and (B) have all required
    cards. fsl_deck_output() will "unshuffle" the cards, making sure
    they're in the correct order.

    Sidebar: normally outputing a structure can use a const form of
    that structure, but the traversal of F-cards in a deck requires
    (for the sake of delta manifests) using a non-const cursor. Thus
    outputing a deck requires a non-const instance. If it weren't for
    delta manifests, we could be "const-correct" here.
*/

/** @page page_transactions DB Transactions

    The fsl_db_transaction_begin() and fsl_db_transaction_end()
    functions implement a basic form of recursive transaction,
    allowing the library to start and end transactions at any level
    without having to know whether a transaction is already in
    progress (sqlite3 does not natively support nested
    transactions). A rollback triggered in a lower-level transaction
    will propagate the error back through the transaction stack and
    roll back the whole transaction, providing us with excellent error
    recovery capabilities (meaning we can always leave the db in a
    well-defined state).

    It is STRICTLY ILLEGAL to EVER begin a transaction using "BEGIN"
    or end a transaction by executing "COMMIT" or "ROLLBACK" directly
    on a fsl_db instance. Doing so bypasses internal state which needs
    to be kept abreast of things and will cause Grief and Suffering
    (on the client's part, not mine).

    Tip: implementing a "dry-run" mode for most fossil operations is
    trivial by starting a transaction before performing the
    operations. Many operations run in a transaction, but if the
    client starts one of his own they can "dry-run" any op by simply
    rolling back the transaction he started. Abstractly, that looks
    like this pseudocode:

    @code
    db.begin();
    fsl.something();
    fsl.somethingElse();
    if( dryRun ) db.rollback();
    else db.commit();
    @endcode
*/

/** @page page_code_conventions Code Conventions

    Project and Code Conventions...

    Foreward: all of this more or less evolved organically or was
    inherited from fossil(1) (where it evolved organically, or was
    inherited from sqilte (where it evol...)), and is written up here
    more or less as a formality. Historically i've not been a fan of
    coding conventions, but as someone else put it to me, "the code
    should look like it comes from a single source," and the purpose
    of this section is to help orient those looking to hack in the
    sources. Note that most of what is said below becomes obvious
    within a few minutes of looking at the sources - there's nothing
    earth-shatteringly new nor terribly controversial here.

    The Rules/Suggestions/Guidelines/etc. are as follows...


    - C99 is the basis. It was C89 until 2021-02-12.

    - The canonical build environment uses the most restrictive set of
    warning/error levels possible. It is highly recommended that
    non-canonical build environments do the same. Adding -Wall -Werror
    -pedantic does _not_ guaranty that all C compliance/portability
    problems can be caught by the compiler, but it goes a long way in
    helping us to write clean code. The clang compiler is particularly
    good at catching subtle foo-foo's such as uninitialized variables.

    - API docs (as you may have already noticed), does not (any
    longer) follow Fossil's comment style, but instead uses
    Doxygen-friendly formatting. Each comment block MUST start with
    two or more asterisks, or '*!', or doxygen apparently doesn't
    understand it
    (https://www.stack.nl/~dimitri/doxygen/manual/docblocks.html). When
    adding code snippets and whatnot to docs, please use doxygen
    conventions if it is not too much of an inconvenience. All public
    APIs must be documented with a useful amount of detail. If you
    hate documenting, let me know and i'll document it (it's what i do
    for fun).

    - Public API members have a fsl_ or FSL_ prefix (fossil_ seems too
    long). For private/static members, anything goes. Optional or
    "add-on" APIs (e.g. ::fcli) may use other prefixes, but are
    encouraged use an "f-word" (as it were), simply out of deference
    to long-standing software naming conventions.

    - Public-API structs and functions use lower_underscore_style().
    Static/internal APIs may use different styles. It's not uncommon
    to see UpperCamelCase for file-scope structs.

    - Overall style, especially scope blocks and indentation, should
    follow Fossil's.  We are _not at all_ picky about whether or not
    there is a space after/before parens in if( foo ), and similar
    small details, just the overall code pattern.

    - Structs and enums all get the optional typedef so that they do
    not need to be qualified with 'struct' resp. 'enum' when
    used. Because of how doxygen tracks those, the typedef should be
    separate from the struct declaration, rather than combinding
    those.

    - Function typedefs are named fsl_XXX_f. Implementations of such
    typedefs/interfaces are typically named fsl_XXX_f_SUFFIX(), where
    SUFFIX describes the implementation's
    specialization. e.g. fsl_output_f() is a callback
    typedef/interface and fsl_output_f_FILE() is a concrete
    implementation for FILE handles.

    - Enums tend to be named fsl_XXX_e.

    - Functions follow the naming pattern prefix_NOUN_VERB(), rather
    than the more C-conventional prefix_VERB_NOUN(),
    e.g. fsl_foo_get() and fsl_foo_set() rather than fsl_get_foo() and
    fsl_get_foo(). The primary reasons are (A) sortability for
    document processors and (B) they more naturally match with OO API
    conventions, e.g.  noun.verb(). A few cases knowingly violate this
    convention for the sake of readability or sorting of several
    related functions (e.g. fsl_db_get_TYPE() instead of
    fsl_db_TYPE_get()).

    - Structs intended to be creatable on the stack are accompanied by
    a const instance named fsl_STRUCT_NAME_empty, and possibly by a
    macro named fsl_STRUCT_NAME_empty_m, both of which are
    "default-initialized" instances of that struct. This is superiour
    to using memset() for struct initialization because we can define
    (and document) arbitrary default values and all clients who
    copy-construct them are unaffected by many types of changes to the
    struct's signature (though they may need a recompile). The
    intention of the fsl_STRUCT_NAME_empty_m macro is to provide a
    struct-embeddable form for use in other structs or
    copy-initialization of const structs, and the _m macro is always
    used to initialize its const struct counterpart. e.g. the library
    guarantees that fsl_cx_empty_m (a macro representing an empty
    fsl_cx instance) holds the same default values as fsl_cx_empty (a
    const fsl_cx value).

    - Returning int vs fsl_int_t vs fsl_size_t: int is used as a
    conventional result code. fsl_int_t is often used as a signed
    length-style result code (e.g. printf() semantics). Unsigned
    ranges use fsl_size_t. Ints are (also) used as a "triplean" (3
    potential values, e.g. <0, 0, >0). fsl_int_t also guarantees that
    it will be 64-bit if available, so can be used for places where
    large values are needed but a negative value is legal (or handy),
    e.g. fsl_strndup()'s second argument. The use of the fsl_xxx_t
    typedefs, rather than (unsigned) int, is primarily for
    readability/documentation, e.g. so that readers can know
    immediately that the function uses a given argument or return
    value following certain API-wide semantics. It also allows us to
    better define platform-portable printf/scanf-style format
    modifiers for them (analog to C99's PRIi32 and friends), which
    often come in handy.

    - Signed vs. unsigned types for size/length arguments: use the
    fsl_int_t (signed) argument type when the client may legally pass
    in a negative value as a hint that the API should use fsl_strlen()
    (or similar) to determine a byte array's length. Use fsl_size_t
    when no automatic length determination is possible (or desired),
    to "force" the client to pass the proper length. Internally
    fsl_int_t is used in some places where fsl_size_t "should" be used
    because some ported-in logic relies on loop control vars being
    able to go negative. Additionally, fossil internally uses negative
    blob lengths to mark phantom blobs, and care must be taken when
    using fsl_size_t with those.

    - Functions taking elipses (...) are accompanied by a va_list
    counterpart named the same as the (...) form plus a trailing
    'v'. e.g. fsl_appendf() and fsl_appendfv(). We do not use the
    printf()/vprintf() convention because that hoses sorting of the
    functions in generated/filtered API documentation.

    - Error handling/reporting: please keep in mind that the core code
    is a library, not an application.  The main implication is that
    all lib-level code needs to check for errors whereever they can
    happen (e.g. on every single memory allocation, of which there are
    many) and propagate errors to the caller, to be handled at his
    discretion. The app-level code (::fcli) is not particularly strict
    in this regard, and installs its own allocator which abort()s on
    allocation error, which simplifies app-side code somewhat
    vis-a-vis lib-level code. When reporting an error can be improved
    by the inclusion of an error string, functions like
    fsl_cx_err_set() can be used to report the error. Several of the
    high-level types in the API have fsl_error object member which
    contains such error state. The APIs which use that state take care
    to use-use the error string memory whenever possible, so setting
    an error string is often a non-allocating operation.
*/


/** @page page_fossil_arch Fossil Architecture Overview

    An introduction to the Fossil architecture. These docs
    are basically just a reformulation of other, more detailed,
    docs which can be found via the main Fossil site, e.g.:

    - https://fossil-scm.org/home/doc/trunk/www/concepts.wiki

    - https://fossil-scm.org/home/doc/trunk/www/fileformat.wiki

    Fossil's internals are fundamentally broken down into two basic
    parts. The first is a "collection of blobs."  The simplest way to
    think of this (and it's not far from the full truth) is a
    directory containing lots of files, each one named after a hash of
    its contents. This pool contains ALL content required for a
    repository - all other data can be generated from data contained
    here. Included in the blob pool are so-called Artifacts. Artifacts
    are simple text files with a very strict format, which hold
    information regarding the idententies of, relationships involving,
    and other metadata for each type of blob in the pool. The most
    fundamental Artifact type is called a Manifest, and a Manifest
    tells us, amongst other things, which of the hash-based file names
    has which "real" file name, which version the parent (or parents!)
    is (or are), and other data required for a "commit" operation.

    The blob pool and the Manifests are all a Fossil repository really
    needs in order to function. On top of that basis, other forms of
    Artifacts provide features such as tagging (which is the basis of
    branching and merging), wiki pages, and tickets. From those
    Artifacts, Fossil can create/calculate all sorts of
    information. For example, as new Artifacts are inserted it
    transforms the Artifact's metadata into a relational model which
    sqlite can work with. That leads us to what is conceptually the
    next-higher-up level, but is in practice a core-most component...

    Storage. Fossil's core model is agnostic about how its blobs are
    stored, but libfossil and fossil(1) both make heavy use of sqlite
    to implement many of their features. These include:

    - Transaction-capable storage. It's almost impossible to corrupt a
    Fossil db in normal use. sqlite3 offers literally the most robust
    general-purpose file format on the planet.

    - The storage of the raw blobs.

    - Artifact metadata is transformed into various DB structures
    which allow libfossil to traverse historical data much more
    efficiently than would be possible without a db-like
    infrastructure (and everything that implies). These structures are
    kept up to date as new Artifacts are stored in a repository,
    either via local edits or synching in remote content. These data
    are incrementally updated as changes are made to a repo.

    - A tremendous amount of the "leg-work" in processing the
    repository state is handled by SQL queries, without which the
    library would easily require 5-10x more code in the form of
    equivalent hard-coded data structures and corresponding
    functionality. The db approach allows us to ad-hoc structures as
    we need them, providing us a great deal of flexibility.

    All content in a Fossil repository is in fact stored in a single
    database file. Fossil additionally uses another database (a
    "checkout" db) to keep track of local changes, but the repo
    contains all "fossilized" content. Each copy of a repo is a
    full-fledged repo, each capable of acting as a central copy for
    any number of clones or checkouts.

    That's really all there is to understand about Fossil. How it does
    its magic, keeping everything aligned properly, merging in
    content, how it stores content, etc., is all internal details
    which most clients will not need to know anything about in order
    to make use of fossil(1). Using libfossil effectively, though,
    does require learning _some_ amount of how Fossil works. That will
    require taking some time with _other_ docs, however: see the
    links at the top of this section for some starting points.


    Sidebar:

    - The only file-level permission Fossil tracks is the "executable"
    (a.k.a. "+x") bit. It internally marks symlinks as a permission
    attribute, but that is applied much differently than the
    executable bit and only does anything useful on platforms which
    support symlinks.

*/

#endif
/* ORG_FOSSIL_SCM_PAGES_H_INCLUDED */
/* end of file ../include/fossil-scm/fossil-pages.h */
/* start of file ../include/fossil-scm/fossil-cli.h */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(_ORG_FOSSIL_SCM_FCLI_H_INCLUDED_)
#define _ORG_FOSSIL_SCM_FCLI_H_INCLUDED_
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

  *****************************************************************************
  This file provides a basis for basic libfossil-using apps. It is intended
  to be used as the basis for test/demo apps, and not necessarily full-featured
  applications.
*/

/* Force assert() to always work... */
#if defined(NDEBUG)
#undef NDEBUG
#define DEBUG 1
#endif
#include <assert.h> /* for the benefit of test apps */
#include <stdlib.h> /* EXIT_SUCCESS and friends */

/** @page page_fcli fcli (formerly FossilApp)

    ::fcli (formerly FossilApp) provides a small framework for
    bootstrapping simple libfossil applications which only need a
    single fsl_cx instance managing a single checkout and/or
    repository. It is primarily intended for use with CLI apps
    implementing features similar to those in fossil(1), but can also
    be used with GUI apps. It provides the following basic services to
    applications:

    - The global ::fcli struct holds global state.

    - fcli_setup() bootstraps the environment. This must be the
    first call made into the API, as this replaces the libfossil
    memory allocator with a fail-fast variant to simplify app-level
    code a bit (by removing the need to check for OOM errors). This
    also registers an atexit(3) handler to clean up fcli-owned
    resources at app shutdown.

    - Automatically tries to open a checkout (and its associated
    repository) under the current directory, but not finding one
    is not an error (the app needs to check for this if it should
    be an error: use fsl_cx_db_repo() and fsl_cx_db_ckout()).

    - fcli_flag(), fcli_next_arg(), and friends provide uniform access
    to CLI arguments.

    - A (very) basic help subsystem, triggered by the --help or -? CLI
    flags, or if the first non-flag argument is "help".  Applications
    may optionally assign fcli.appHelp to a function which outputs
    app-specific help.

    - Basic error reporting mechanism. See fcli_err_report().

    The source tree contains several examples of using fcli
    in the files named f-*.c.
*/

/**
   Ouputs the given printf-type expression (2nd arg) to
   fcli_printf() if fcli.verbose>=N, else it does nothing.

   Reminder: this uses a while(0) loop so that the macro can end
   with a semicolon without triggering a warning.
*/
#define FCLI_VN(N,pfexp)                        \
  if(fcli.clientFlags.verbose>=(N)) do {        \
      fcli_printf("VERBOSE %d: ", (int)(N));    \
      fcli_printf pfexp;                        \
  } while(0)

/**
   Convenience form of FCLI_VN for level-2 verbosity.
*/
#define FCLI_V2(pfexp) FCLI_VN(2,pfexp)

/**
   Convenience form of FCLI_VN for level-1 verbosity. Levels 1 and 2
   are intended for application-level use. Levels 3+ are intended for
   fcli use.
*/
#define FCLI_V(pfexp) FCLI_VN(1,pfexp)

#if defined(__cplusplus)
extern "C" {
#endif


/**
   Result codes specific to the fcli API.
*/
enum fcli_rc_e {
/**
   For use with fcli_flag_callback_f() implementations to indicate
   that the flag processor should check for that flag again.
*/
FCLI_RC_FLAG_AGAIN = FSL_RC_end + 1,
/**
   Returned from fcli_setup() if flag processing invokes the help
   system. This is an indication that the app should exit immediately
   with a 0 result code.
*/
FCLI_RC_HELP
};

/**
   Types for use with the fcli_cliflag::flagType field.  The value of
   that field determines how the CLI flag handling interprets the
   fcli_cliflag::flagValue void pointer.
*/
enum fcli_cliflag_type_e {
/** Sentinel placeholder. */
FCLI_FLAG_TYPE_INVALID = 0,
/** Represents bool. */
FCLI_FLAG_TYPE_BOOL,
/** Represents bool, but gets set to false if the flag is set. */
FCLI_FLAG_TYPE_BOOL_INVERT,
/** Represents int32_t. */
FCLI_FLAG_TYPE_INT32,
/** Represents int64_t. */
FCLI_FLAG_TYPE_INT64,
/** Represents fsl_id_t, which might be either int32_t or int64_t. */
FCLI_FLAG_TYPE_ID,
/** Represents double. */
FCLI_FLAG_TYPE_DOUBLE,
/** Represents (char const *). */
FCLI_FLAG_TYPE_CSTR
};

typedef struct fcli_cliflag fcli_cliflag;
/**
   Callback handler for CLI flag handling. It is passed the flag in
   question, containing its state after fcli has processed its
   flagValue part.

   If the callback returns FCLI_RC_FLAG_AGAIN, the flag handler will
   check for that flag again (for as long as the handler keeps
   returning that value and as long as the flag repeats in the
   argument list). This can be used to implement repeatable flags.

   If it returns any other non-0 value, fcli_setup()
   resp. fcli_process_flags() will fail with that result code.

   Achtung: during fcli_setup(), at the time these are processed, fcli
   will not yet have opened up a repository or checkout, so the
   callback will not have access to things like symbolic-name-to-UUID
   conversion at this level. Apps which need to do such work on their
   arguments must first queue up the input and process it after
   fcli_setup() returns. Apps which process the flags using
   fcli_process_flags() after setup is complete will have access to
   such features.
*/
typedef int (*fcli_flag_callback_f)(fcli_cliflag const *);

/**
   Under construction. A reworking of fcli's CLI flag handling,
   primarily so that we can unify the generation of app help text and
   make it consistent across apps.

   @see fcli_process_flags()
*/
struct fcli_cliflag {
  /**
     "Short-form" for this flag, noting that there's no restriction on
     its length. Either of flagShort and flagLong, but not both, may be
     NULL.
  */
  const char * flagShort;
  /**
     "Long-form" for this flag, noting that there's no restriction on
     its length.
  */
  const char * flagLong;

  /**
     Specifies how to interpret the data pointed to by
     this->flagValue. See that member for details.
  */
  enum fcli_cliflag_type_e flagType;

  /**
     If not NULL, the member is the target of the flag's value.
     Its exact interpretation depends on the value of this->flagType:

     - FCLI_FLAG_TYPE_BOOL: a pointer to a bool. Existence
     of the flag causes it to be set to true.

     - FCLI_FLAG_TYPE_BOOL_INVERT: a pointer to a bool, but
     existence of the flag causes it to be set to false.

     - FCLI_FLAG_TYPE_INT32: a pointer to an int32_t. The flag
     will be converted from string to int using atoi().

     - FCLI_FLAG_TYPE_INT64: a pointer to an int64_t. The flag
     will be converted from string to int using atoll().

     - FCLI_FLAG_TYPE_ID: a pointer to a fsl_id_t, which
     can be either int32_t or int64_t.

     - FCLI_FLAG_TYPE_DOUBLE: a pointer to a double. The flag
     will be converted from string to int using strtod().

     - FCLI_FLAG_TYPE_CSTR: a pointer to a (const char *). The flag's
     value will be assigned on without further interpretation.

     If this member is not NULL: If the flag is set, this value will
     be assigned to. If the flag is not set in the CLI args, this
     value is not written to.

     If this member is NULL then fcli_process_flags() will simply skip
     over it, but the help-text generator will process it. This can be
     used to set up flags which will appear in the --help text but
     which are processed separately (or outright ignored) by the app.

     The underlying flag value's string memory is owned by fcli and is
     valid until the app exits.
  */
  void * flagValue;

  /**
     Optional descriptive label to be used when rendering help text:

     -x|--x-flag=ABCD

     If it is NULL, the --help text generator will choose a value of
     the ABCD part which depends on this->flagType.

     This is only used if this->flagType is not one of
     (FCLI_FLAG_TYPE_BOOL, FCLI_FLAG_TYPE_BOOL_INVERT). For those
     types (which have no client-given values), this member is
     ignored when generating help text.
  */
  const char * flagValueLabel;

  /**
     Optional callback which gets passed the flag, if it is set, after
     fcli has assigned the flagValue entry (if it is not NULL).
     See this data type's docs for more details.
  */
  fcli_flag_callback_f callback;

  /**
     Help text for the flag. Intended to be displayed in the context
     of a --help listing of flags.
  */
  const char * helpText;
};
#define fcli_cliflag_empty_m {\
    0/*flagShort*/,0/*flagLong*/,\
    FCLI_FLAG_TYPE_INVALID/*flagType*/,\
    NULL/*flagValue*/,\
    NULL/*flagValueLabel*/,NULL/*callback*/,NULL/*helpText*/}
/** Non-const-copyable counterpart of fcli_cliflag_empty_m. */
FSL_EXPORT const fcli_cliflag fcli_cliflag_empty;

/** @def FCLI_FLAG_xxx

   The various FCLI_FLAG_xxx macros are convenience-form initializers
   for fcli_cliflag instances for use in initializing a fcli_cliflag
   array. Example usage:

   @code
   bool flag1 = true, flag2 = false;
   int32_t flag3 = 0;
   const char * flag4 = 0;
   const fcli_cliflag cliFlags[] = {
     FCLI_FLAG_BOOL("x","xyz",&flag1,"Flag 1."),
     FCLI_FLAG_BOOL_INVERT(NULL,"yzx",&flag2,"Flag 2."),
     FCLI_FLAG_INT32("z",NULL,"value",&flag3,"Flag 3"),
     FCLI_FLAG("f","file","filename",&flag4,
               "Input file. May optionally be passed as the first "
               "non-flag argument."),
     fcli_cliflag_empty_m // list MUST end with this (or equivalent)
   };
   fcli.cliFlags = cliFlags;
   @endcode

   BE CAREFUL with the data types, as we're using (void*) to access
   data of an arbitrary type, the type being defined by a separate
   field in the fcli_cliflag object.
*/
#define FCLI_FLAG_xxx /* for doc purposes only */
//Members:
//{short, long, type, tgt, valueDescr., callback, help}
/** Bool-type flag. */
#define FCLI_FLAG_BOOL(S,L,TGT,HELP) \
  {S,L,FCLI_FLAG_TYPE_BOOL,TGT,NULL,NULL,HELP}
/** Bool-type flag, but set to false if flag is set. */
#define FCLI_FLAG_BOOL_INVERT(S,L,TGT,HELP) \
  {S,L,FCLI_FLAG_TYPE_BOOL_INVERT,TGT,NULL,NULL,HELP}
/** Bool-type flag with a callback. */
#define FCLI_FLAG_BOOL_X(S,L,TGT,CALLBACK,HELP) \
  {S,L,FCLI_FLAG_TYPE_BOOL,TGT,NULL,CALLBACK,HELP}
/** Bool-type flag with a callback, but set to false if the flag is set. */
#define FCLI_FLAG_BOOL_INVERT_X(S,L,TGT,CALLBACK,HELP) \
  {S,L,FCLI_FLAG_TYPE_BOOL_INVERT,TGT,NULL,CALLBACK,HELP}
/** String-type flag. */
#define FCLI_FLAG_CSTR(S,L,LBL,TGT,HELP) \
  {S,L,FCLI_FLAG_TYPE_CSTR,TGT,LBL,NULL,HELP}
#define FCLI_FLAG FCLI_FLAG_CSTR
/** String-type flag with a callback. */
#define FCLI_FLAG_CSTR_X(S,L,LBL,TGT,CALLBACK,HELP) \
  {S,L,FCLI_FLAG_TYPE_CSTR,TGT,LBL,CALLBACK,HELP}
#define FCLI_FLAG_X FCLI_FLAG_CSTR_X
/** int32-type flag. */
#define FCLI_FLAG_I32(S,L,LBL,TGT,HELP) \
  {S,L,FCLI_FLAG_TYPE_INT32,TGT,LBL,NULL,HELP}
/** int32-type flag with a callback. */
#define FCLI_FLAG_I32_X(S,L,LBL,TGT,CALLBACK,HELP) \
  {S,L,FCLI_FLAG_TYPE_INT32,TGT,LBL,CALLBACK,HELP}
/** int32-type flag. */
#define FCLI_FLAG_I64(S,L,LBL,TGT,HELP) \
  {S,L,FCLI_FLAG_TYPE_INT64,TGT,LBL,NULL,HELP}
/** int32-type flag with a callback. */
#define FCLI_FLAG_I64_X(S,L,LBL,TGT,CALLBACK,HELP) \
  {S,L,FCLI_FLAG_TYPE_INT64,TGT,LBL,CALLBACK,HELP}
/** fsl_id_t-type flag. */
#define FCLI_FLAG_ID(S,L,LBL,TGT,HELP) \
  {S,L,FCLI_FLAG_TYPE_ID,TGT,LBL,NULL,HELP}
/** fsl_id_t-type flag with a callback. */
#define FCLI_FLAG_ID_X(S,L,LBL,TGT,CALLBACK,HELP) \
  {S,L,FCLI_FLAG_TYPE_ID,TGT,LBL,CALLBACK,HELP}
/** double-type flag. */
#define FCLI_FLAG_DBL(S,L,LBL,TGT,HELP) \
  {S,L,FCLI_FLAG_TYPE_DOUBLE,TGT,LBL,NULL,HELP}
/** double-type flag with a callback. */
#define FCLI_FLAG_DBL_X(S,L,LBL,TGT,CALLBACK,HELP) \
  {S,L,FCLI_FLAG_TYPE_DOUBLE,TGT,LBL,CALLBACK,HELP}

/**
   Structure for holding app-level --help info.
*/
struct fcli_help_info {
  /** Brief description of what the app does. */
  char const * briefDescription;
  /** Brief usage text. It will be prefixed by the app's name and
      "[options]". e.g. "file1 ... fileN".
  */
  char const * briefUsage;
  /**
     If not 0 then this is called after outputing any flags' help.
     It should output any additional help text using f_out().
  */
  void (*callback)(void);
};
typedef struct fcli_help_info fcli_help_info;

/**
   The fcli_t type, accessed by clients via the ::fcli
   global instance, contains various data for managing a basic
   Fossil SCM application build using libfossil.

   Usage notes:

   - All non-const pointer members are owned and managed by the
   fcli API. Clients may reference them but must be aware of
   lifetimes if they plan to hold the reference for long.

*/
struct fcli_t {
  /**
     If not NULL, it must be a pointer to fcli_help_info holding help
     info for the app.  It will be formated and output when --help
     is triggered.

     Should be set by client applications BEFORE calling fcli_setup()
     so that the ::fcli help subsystem can integrate the
     app. fcli.appName will be set by the time this is used.
  */
  fcli_help_info const * appHelp;

  /**
     May be set to an array of CLI flag objects, which fcli_setup()
     will use for parsing the CLI flags. The array MUST end with an
     entry which has NULL values for its (flagShort, flagLong)
     members. When creating the array it is simplest to use
     fcli_cliflag_empty_m as the initializer for that sentinel entry.

     The elements in this array are traversed in the order they are
     provided, and any which have a callback which returns
     FCLI_RC_FLAG_AGAIN will be traversed repeatedly before moving on
     to the next flag. This ordering may have side-effects on how the
     app sets up its flag handling. In particular, this arrangement
     makes the common cases easy to implement but makes certain more
     complicated situations effectively impossible to implement. For
     example:

     @code
     -x=1 -y=2 -x=3 -y=4
     @endcode

     Assuming the definition of the -y flag is first in this array and
     has a callback which returns FCLI_RC_FLAG_AGAIN, the two -x flags
     will be processed before either of the -y flags. Thus it is
     impossible (using this method of traversal) to change the
     behavior of the -y flags based on the left-closest -x value.

     We "could," but probably never will, instead walk the input argv
     looking for things which look like flags, then looking back into
     this list for a match, rather than the other way around. That
     would require a new args processing implementation, though, for
     relatively little benefit.
  */
  const fcli_cliflag * cliFlags;

  /**
     The shared fsl_cx instance. It gets initialized by
     fcli_setup() and cleaned up post-main().

     this->f is owned by this object and will be cleaned up at app
     shutdown (post-main).
  */
  fsl_cx * f;
  /**
     The current list of CLI arguments. This list gets modified by
     fcli_flag() and friends. Its memory is owned by fcli.
  */
  char ** argv;
  /**
     Current number of items in this->argv.
  */
  int argc;
  /**
     Application's name. Currently argv[0] but needs to be
     adjusted for Windows platforms.
  */
  char const * appName;
  /**
     The flags in this struct are "public," meaning that client
     applications may query and safely manipulate them directly if
     they know what they're doing.
  */
  struct {
    /**
       If not NULL then fcli_setup() will attempt to open the
       checkout for the given dir, including its associated repo
       db. By default this is "." (the current directory).

       Applications can set this to NULL _before_ calling
       fcli_setup() in order to disable the automatic attemp to
       open a checkout under the current directory.  Doing so is
       equivalent to using the --no-checkout|-C flags. The global
       --checkout-dir flag will trump that setting, though.
    */
    char const * checkoutDir;

    /**
       A verbosity level counter. Starts at 0 (no verbosity) and goes
       up for higher verbosity levels. Currently levels 1 and 2 are
       intended for app-level use and level 3 for library-level use.
    */
    unsigned short verbose;

    /**
       True if the -D|--dry-run flag is seen during initial arguments
       processing. ::fcli does not use this itself - it is
       intended as a convenience for applications.
    */
    bool dryRun;
  } clientFlags;
  /**
     Transient settings and flags. These are bits which are used
     during (or very shortly after) fcli_setup() but have no effect
     if modified after that.
  */
  struct {
    /**
       repo db name string from -R/--repo CLI flag.
    */
    const char * repoDbArg;
    /**
       User name from the -U/--user CLI flag.
    */
    const char * userArg;
    /**
       Incremented if fcli_setup() detects -? or --help in the
       argument list, or if the first non-flag argument is "help".
    */
    short helpRequested;
  } transient;
  /**
     Holds bits which can/should be configured by clients BEFORE
     calling fcli_setup().
  */
  struct {
    /**
       Whether or not to enable fossil's SQL tracing.  This should
       start with a negative value, which helps fcli_setup() process
       it. Setting this after initialization has no effect.
    */
    int traceSql;
    /**
       This output channel is used when initializing this->f. The
       default implementation uses fsl_outputer_FILE to output to
       stdout.
    */
    fsl_outputer outputer;

  } config;
  /**
     For holding pre-this->f-init error state. Once this->f is
     initialized, all errors reported via fcli_err_set() are stored in
     that object's error state.
  */
  fsl_error err;
};
typedef struct fcli_t fcli_t;

/** @var fcli

    This fcli_t instance is intended to act as a singleton.  It holds
    all fcli-related global state.  It gets initialized with
    default/empty state at app startup and gets fully initialized via
    fcli_setup(). See that routine for an example of how it is
    typically initialized.

    fcli_cx() returns the API's fsl_cx instance. It will be non-NULL
    (but might not have an opened checkout/repository) if fsl_setup()
    succeeds.
*/
FSL_EXPORT fcli_t fcli;

/**
   Should be called early on in main(), passed the arguments passed
   to main(). Returns 0 on success.  Sets up the ::fcli instance
   and opens a checkout in the current dir by default.

   MUST BE CALLED BEFORE fsl_malloc() and friends are used, as this
   swaps out the allocator with one which aborts on OOM. (But see
   fcli_pre_setup() for a workaround for that.)

   If argument processing finds either of the (--help, -?) flags,
   or the first non-flag argument is "help", it sets
   fcli.transient.helpRequested to a true value, calls fcli_help(),
   and returns FCLI_RC_HELP, in which case the application should
   exit/return from main with code 0 immediately.

   This function behaves significantly differently if fcli.cliFlags
   has been set before it is called. In that case, it parses the CLI
   flags using that type's rules and sets up fcli_help() to use those
   flags for generating the help. It parses the global flags first,
   then the app-specific flags.

   Returns 0 on success. Results other than FCLI_RC_HELP should be
   treated as fatal to the app, and fcli.f's error state _might_
   contain info about the error. If this function returns non-0, the
   convention is that the app immediately returns the result of
   fcli_end_of_main(THE_RESULT_CODE) from main(). That function will
   treat FCLI_RC_HELP as a non-error and will report any error state
   pending in the fcli_cx() object.

   Example of intended basic usage:

   @code
   int main(int argc, char const * const * argv){
     ...
     // Optional fcli_cliflag setup:
     fcli_cliflag const cliFlags[] = {
       ...,
       fcli_cliflag_empty_m
     };
     fcli.cliFlags = cliFlags;
     // Optional fcli_help_info setup:
     fcli_help_info const help = {
       "Fnoobs the borts and rustles the feathers.",
       "file1 [... fileN]",
       NULL // optional callback to display extra help
     };
     fcli.appHelp = &help;
     // Initialize...
     int rc = fcli_setup(argc, argv);
     if(rc) goto end;

     ... app logic ...

     end:
     return fcli_end_of_main(rc);
   }
   @endcode

   @see fcli_pre_setup()
*/
FSL_EXPORT int fcli_setup(int argc, char const * const * argv );

/**
   The first time this is called, it swaps out libfossil's default
   allocator with a fail-fast one which abort()s on allocation
   error. This is normally called by fcli_setup(), but that also means
   that it's illegal to use fsl_malloc() and friends before calling
   that routine. If an application really needs to use fsl_malloc()
   before calling fcli_setup(), it must call this first in order to
   get the allocator initialization out of the way.

   Calls after the first are no-ops, but the check for that is not
   thread-safe. Neither is fcli, though, so that's okay.
*/
FSL_EXPORT void fcli_pre_setup(void);

/**
   Returns the libfossil context associated with the fcli API.
   This will be NULL until fcli_setup() is called.
*/
FSL_EXPORT fsl_cx * fcli_cx(void);

/**
   Works like printf() but sends its output to fsl_outputf() using
   the fcli.f fossil conext (if set) or fsl_fprintf() (to
   stdout).
*/
FSL_EXPORT void fcli_printf(char const * fmt, ...)
#if 0
/* Would be nice, but complains about our custom format options: */
  __attribute__ ((__format__ (__printf__, 1, 2)))
#endif
  ;

/**
   f_out() is a shorthand for fcli_printf().
*/
#define f_out fcli_printf  

/**
   Returns the verbosity level set via CLI args. 0 is no verbosity,
   and one level is added each time the --verbose/-V CLI flag is
   encountered by fcli_setup().
*/
FSL_EXPORT unsigned short fcli_is_verbose(void);

/**
   Searches fcli.argv for the given flag (pass it without leading
   dashes). If found, this function returns true, else it returns
   false. If value is not NULL then the flag, if found, is assumed to
   have a value, otherwise the flag is assumed to be a boolean. A flag
   with a value may take either one of these forms:

   -flag=value
   -flag value

   *value gets assigned to a COPY OF the value part of the first form
   or a COPY OF the subsequent argument for the second form (copies
   are required in order to avoid trickier memory management
   here). That copy is owned by fcli and will be cleaned up at app
   exit. On success it removes the flag (and its value, if any) from
   fcli.argv.  Thus by removing all flags early on, the CLI arguments
   are left only with non-flag arguments to sift through.

   Flags may start with either one or two dashes - they are
   equivalent.

   This function may update the fcli error state if. Specifically, if
   passed a non-NULL 2nd argument and the flag is found at the end of
   the argument list or the value immediately after the flag starts
   with '-' then the error state will be updated and false will be
   returned. Apps don't normally need to be quite that picky with
   their error checking after looking for a flag, but the state is
   there if needed. It may get reset by any future calls into the API,
   though. fcli_process_flags() does check this state and will fail if
   such an error is triggered.
*/
FSL_EXPORT bool fcli_flag(char const * opt, const char ** value);

/**
   Works like fcli_flag() but tries two argument forms, in order. It
   is intended to be passed short and long forms, but can be passed
   two aliases or similar. It accepts NULL for either form.

   @code
   const char * v = NULL;
   fcli_flag2("n", "limit", &v);
   if(v) { ... }
   @endcode
*/
FSL_EXPORT bool fcli_flag2(char const * opt1, char const * opt2,
                           const char ** value);

/**
   Works similarly to fcli_flag2(), but if no flag is found and
   value is not NULL then *value is assigned to the return value of
   fcli_next_arg(true). In that case:

   - The return value will specify whether or not fcli_next_arg()
   returned a value or not.

   - If it returns true then *value is owned by fcli and will be
   cleaned up at app exit.

   - If it returns false, *value is not modified.

   The opt2 parameter may be NULL, but op1 may not.
*/
FSL_EXPORT bool fcli_flag_or_arg(char const * opt1, char const * opt2,
                                 const char ** value);


/**
   Clears any error state in fcli.f.
*/
FSL_EXPORT void fcli_err_reset(void);

/**
   Sets fcli.f's error state, analog to fsl_cx_err_set().
   Returns the code argument on success, some other non-0 value on
   a more serious error (e.g. FSL_RC_OOM when formatting the
   string).
*/
FSL_EXPORT int fcli_err_set(int code, char const * fmt, ...);

/**
   Returns the internally-used fsl_error instance which is used for
   propagating errors.  The object is owned by ::fcli and MUST NOT
   be freed or otherwise abused by clients. It may, however, be
   passed to routines which take a fsl_error parameter to report
   errors (e.g. fsl_deck_output().

   Returns NULL if fcli_setup() has not yet been called or after
   fcli has been cleaned up (post-main()).

*/
FSL_EXPORT fsl_error * fcli_error(void);

/**
   If ::fcli has any error state, this outputs it and returns the
   error code, else returns 0. If clear is true the error state is
   cleared/reset, otherwise it is left in place. Returns 0 if
   ::fcli has not been initialized. The 2nd and 3rd arguments are
   assumed to be the __FILE__ and __LINE__ macro values of the call
   point. See fcli_err_report() for a convenience form of this
   function.

   The format of the output depends partially on fcli_is_verbose(). In
   verbose mode, the file/line info is included, otherwise it is
   elided.

   @see fcli_err_report()
*/
FSL_EXPORT int fcli_err_report2(bool clear, char const * file, int line);

/**
   Convenience macro for using fcli_err_report2().
*/
#define fcli_err_report(CLEAR) fcli_err_report2((CLEAR), __FILE__, __LINE__)

/**
   Peeks at or takes the next argument from the CLI args.  If the
   argument is true, it is removed from the args list.  It is owned by
   fcli and will be freed when the app exits.
*/
FSL_EXPORT const char * fcli_next_arg(bool remove);

/**
   If fcli.argv contains what looks like any flag arguments, this
   updates the fossil error state and returns FSL_RC_MISUSE, else
   returns 0. If outputError is true and an unused flag is found
   then the error state is immediately output (but not cleared).
*/
FSL_EXPORT int fcli_has_unused_flags(bool outputError);
/**
   If fcli.argv contains any entries, returns FSL_RC_MISUSE and
   updates the error state with a message about unusued extra
   arguments, else returns 0. If outputError is true and an unconsumed
   argument is found then the error state is immediately output (but
   not cleared).
*/
FSL_EXPORT int fcli_has_unused_args(bool outputError);

typedef struct fcli_command fcli_command;
/**
   Typedef for general-purpose fcli call-by-name commands.

   It gets passed its own command definition, primarily for each of
   access to the flags member for CLI flags processing.

   @see fcli_dispatch_commands()
*/
typedef int (*fcli_command_f)(fcli_command const *);

/**
   Describes a named callback command.

   @see fcli_dispatch_commands()
*/
struct fcli_command {
  /** The name of the command. */
  char const * name;
  /** Brief description, for use in generating help text. */
  char const * briefDescription;
  /** The callback for this command. */
  fcli_command_f f;
  /**
     Must be NULL or an array compatible with fcli_process_flags().
     Can be used from within this->f for command-specific flag
     dispatching, as well as help text generation.
  */
  fcli_cliflag const * flags;
};

/**
   Expects an array of fcli_commands which contain a trailing
   sentry entry with a NULL name and callback. It searches the list
   for a command matching fcli_next_arg(). If found, it
   removes that argument from the list, calls the callback, and
   returns its result. If no command is found FSL_RC_NOT_FOUND is
   returned, the argument list is not modified, and the error state
   is updated with a description of the problem and a list of all
   command names in cmdList.

   If reportErrors is true then on error this function outputs
   the error result but it keeps the error state in place
   for the downstream use.

   As a special case: when a command matches the first argument and
   that object has a non-NULL flags member, this function checks the
   _next_ argument, and if it is "help" then this function passes
   that flags member to fcli_command_help() to output help, then
   returns 0.
*/
FSL_EXPORT int fcli_dispatch_commands( fcli_command const * cmdList,
                                       bool reportErrors);

/**
   A minor helper function intended to be passed the pending result
   code of the main() routine. This function outputs any pending
   error state in fcli. Returns one of EXIT_SUCCESS if mainRc is 0
   and fcli had no pending error report, otherwise it returns
   EXIT_FAILURE. This function does not clean up fcli - that is
   handled via an atexit() handler.

   It is intended to be called once at the very end of main:

   @code
   int main(){
   int rc;

   ...set up fcli...assign rc...

   return fcli_end_of_main(rc);
   }
   @endcode

   As a special case, if mainRc is FCLI_RC_HELP, it is assumed to
   be the result of the fcli --help flag handling, and is treated as
   if it were 0.

   @see fcli_error()
   @see fcli_err_set()
   @see fcli_err_report()
   @see fcli_err_reset()
*/
FSL_EXPORT int fcli_end_of_main(int mainRc);

/**
   If mem is not NULL, this routine appends mem to a list of pointers
   which will be passed to fsl_free() during the atexit() shutdown
   phase of the app. Because fcli uses a fail-fast allocator, failure
   to append the entry will itself cause a crash. This is only useful
   for values for which fsl_free() suffices to clean them up, not
   complex values like multi-dimensional arrays.

   "fax" is short for "free at exit."

   Results are undefined if the same address or overlapping addresses
   are queued more than once. Once an entry is in this queue, there is
   no way to remove it.
*/
FSL_EXPORT void fcli_fax(void * mem);

/**
   Requires an array of fcli_cliflag objects terminated with an
   instance with NULL values for the (flagShort, flagLong) members
   (fcli_cliflag_empty_m is an easy way to get that).

   If fcli.cliFlags is set before fcli_setup() is called, this routine
   is called and passed those flags. Thus most apps can simply assign
   their flags there and let setup() do the work. Apps which have
   multiple dispatch paths, each with differing flags, may find it
   easier to use this.

   As a special case, if a given entry has NULL values for both of its
   (flagValue, callback) members, it is assumed to exist purely for
   use with the help-generating mechanisms and the flag is NOT
   processed or consumed by fcli_process_flags(). That can be used
   when the client needs to process the flag in ways beyond what is
   capable via this routine. In such cases, add an appropriate entry
   to the fcli_cliflag array for --help purposes and then process the
   flag using fcli_flag() (or similar) either before or after calling
   this.

   Returns 0 on success. Returns non-0 only if a callback() member of
   one of the entries returns a value other than FCLI_RC_FLAG_AGAIN.
*/
FSL_EXPORT int fcli_process_flags( fcli_cliflag const * defs );

/**
   Requires an array of fcli_cliflag objects as described for
   fcli_process_flags(). This routine outputs their flags
   and help text in a framework-conventional manner.

   If fcli.cliFlags is set before fcli_setup() is called, this routine
   is called and passed those flags. Thus most apps can simply assign
   their flags there and let fcli_setup() do the work. Apps which have
   multiple dispatch paths, each with differing flags, may find it
   easier to use this.

   Such apps, in order to get the full help listing for all dispatch
   paths, may need to assign fcli.appHelp to a helper which
   dispatches to an app-local help routine which, in turn, passes each
   of their separate fcli_cliflag lists to this routine.
*/
FSL_EXPORT void fcli_cliflag_help(fcli_cliflag const *defs);

/**
   Requires that cmd be an array of fcli_command objects with a
   trailing entry which has a NULL name. This function iterates over
   them and outputs help text based on each one's (name,
   briefDescription, flags) members.

   If the 2nd argument is true, only the help for the single given
   object is output, not any adjacent array members (if any).
*/
FSL_EXPORT void fcli_command_help(fcli_command const * cmd, bool onlyOne);

/**
   If fcli has a checkout opened, this dumps various info about it
   to its output channel. Returns 0 on success, FSL_RC_NOT_A_CKOUT
   if no checkout is opened, or some other non-0 code on error.

   If the useUtc argument is true, it uses UTC timestamps, else
   localtime. (Potential TODO: add an fcli-level CLI flag for that
   instead?)
*/
FSL_EXPORT int fcli_ckout_show_info(bool useUtc);

/**
   Given a hash prefix, lists (via f_out()) all blob table entries
   which have this prefix. This is intended to be used by apps which
   accept a from a user and fsl_sym_to_rid() returns FSL_RC_AMBIGUOUS.

   The first argument is an optional output label/header to output.
   If it is NULL, a default is used. If it is "" then no header is
   output.
*/
FSL_EXPORT void fcli_list_ambiguous_artifacts(char const * label, char const *prefix);

/**
   If fcli has an opened checkout, that db handle is returned, else NULL
   is returned.
*/
FSL_EXPORT fsl_db * fcli_db_ckout(void);

/**
   If fcli has an opened repository, that db handle is returned, else
   NULL is returned.
*/
FSL_EXPORT fsl_db * fcli_db_repo(void);

/**
   If fcli has an opened checkout, that db handle is returned, else NULL
   is returned and fcli's error state is updated with a description
   of the problem.   
*/
FSL_EXPORT fsl_db * fcli_needs_ckout(void);

/**
   If fcli has an opened repository, that db handle is returned, else NULL
   is returned and fcli's error state is updated with a description
   of the problem.   
*/
FSL_EXPORT fsl_db * fcli_needs_repo(void);

/**
   Processes all remaining CLI arguments as potential file or directory
   names, collects their vfile.id values, and stores them in the given
   target bag. It requires an opened checkout.

   vid is the vfile.vid value to filter on. If vid<=0 then the current
   checkout version is used. (Unless the app has explicitly loaded
   another version, that will be the only option available.)

   If relativeToCwd is true then each argument is resolved as if
   referenced from the current working directory, else each is assumed
   to be relative to the top of the checkout directory. (For CLI apps,
   a value of true is almost always the right choice.)

   If changedFilesOnly is true then only files which are "changed",
   according to the vfile table (as opposed to a filesystem check) are
   considered for addition. For that to work, vfile must be up to
   date, so fsl_vfile_changes_scan() must have been recently called to
   update that state. This function does not call it automatically
   because it's relatively slow and many apps already have to call it
   on their own.

   This function matches only vfile.pathname, not vfile.origname,
   because it is possible for a given name to be in both fields (in
   different records) at the same time.

   Returns 0 on success. If there are no more CLI arguments when it is
   called then it returns FSL_RC_MISUSE and updates the fcli error
   state with a description of the problem. It may return any number
   of non-0 codes from the underlying operations.

   Sidebar: fsl_filename_to_vfile_ids() requires that directory names
   passed to it have no trailing slashes, and routine strips trailing
   slashes from its arguments before passing them on to that routine,
   so they may be entered with slashes without ill effect.

   @see fsl_filename_to_vfile_ids()
*/
FSL_EXPORT int fcli_args_to_vfile_ids(fsl_id_bag *tgt, fsl_id_t vid,
                                      bool relativeToCwd,
                                      bool changedFilesOnly);

/**
   Performs a "fingerprint check" on the current checkout/repo
   combination, as per fsl_ckout_fingerprint_check(). If the check
   fails and reportImmediately is true then an error report is
   immediately output. Returns 0 if the fingerprint check is okay,
   else a non-0 value as per fsl_ckout_fingerprint_check().

   Passing true here may output more information than the underlying
   fsl_cx-level error state provides. e.g. it may provide a hint about
   how to recover.
*/
FSL_EXPORT int fcli_fingerprint_check(bool reportImmediately);

/**
   Returns the "tail" part of the argv[0] string which was passed to
   fcli_setup() or fcli_setup2(), or NULL if neither of those have yet
   been called. The "tail" part is the part immediately after the
   final '/' or '\\' character.
*/
FSL_EXPORT char const * fcli_progname();

#if defined(__cplusplus)
} /*extern "C"*/
#endif


#endif
/* _ORG_FOSSIL_SCM_FCLI_H_INCLUDED_ */
/* end of file ../include/fossil-scm/fossil-cli.h */
