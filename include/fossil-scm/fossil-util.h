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

#include "fossil-config.h" /* MUST come first b/c of config macros */
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
typedef int (*fsl_output_f)( void * state,
                             void const * src, fsl_size_t n );


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
   writeable (FILE*) handle. Is a no-op (returning 0) if
   !n. Returns FSL_RC_MISUSE if !state or !src.
*/
FSL_EXPORT int fsl_output_f_FILE( void * state, void const * src, fsl_size_t n );

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
   Uses fsl_appendf() to append formatted output to the given
   buffer.  Returns 0 on success, FSL_RC_MISUSE if !f or !dest, and
   FSL_RC_OOM if an allocation fails while expanding dest.

   @see fsl_buffer_append()
   @see fsl_buffer_reserve()
*/
FSL_EXPORT int fsl_buffer_appendf( fsl_buffer * dest,
                                   char const * fmt, ... );

/** va_list counterpart to fsl_buffer_appendfv(). */
FSL_EXPORT int fsl_buffer_appendfv( fsl_buffer * dest,
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
   Outputs at most n bytes to dest and returns the number of bytes
   output. Returns a negative value if !dest or !fmt. Returns 0
   without side-effects if !n or !*fmt.

   If the destination buffer is long enough (this function returns
   a non-negative value less than n), this function NUL-terminates it.
   If it returns n then there was no space for the terminator.
*/
FSL_EXPORT fsl_int_t fsl_snprintf( char * dest, fsl_size_t n, char const * fmt, ... );

/**
   va_list counterpart to fsl_snprintf()
*/
FSL_EXPORT fsl_int_t fsl_snprintfv( char * dest, fsl_size_t n, char const * fmt, va_list args );

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
   @typedef fsl_int_t (*fsl_appendf_f)( void * state, char const * data, fsl_int_t n )

   The fsl_appendf_f typedef is used to provide fsl_appendfv() with
   a flexible output routine, so that it can be easily send its
   output to arbitrary targets.

   The policies which implementations need to follow are:

   - state is an implementation-specific pointer (may be 0) which
   is passed to fsl_appendf(). fsl_appendfv() doesn't know what
   this argument is but passes it to its fsl_appendf_f argument.
   Typically this pointer will be an object or resource handle to
   which string data is pushed.

   - The 'data' parameter is the data to append. The API does not
   currently guaranty that data containing embeded NULs will survive
   the ride through fsl_appendf() and its delegates.

   - n is the number of bytes to read from data. The fact that n is
   of a signed type is historical. It can be treated as an unsigned
   type for purposes of fsl_appendf().

   - Returns, on success, the number of bytes appended (may be 0).

   - Returns, on error, an implementation-specified negative
   number.  Returning a negative error code will cause
   fsl_appendfv() to stop processing and return. Note that 0 is a
   success value (some printf format specifiers do not add anything
   to the output).
*/
typedef fsl_int_t (*fsl_appendf_f)( void * state, char const * data,
                                    fsl_int_t n );

/**
   This function works similarly to classical printf
   implementations, but instead of outputing somewhere specific, it
   uses a callback function to push its output somewhere. This
   allows it to be used for arbitrary external representations. It
   can be used, for example, to output to an external string, a UI
   widget, or file handle (it can also emulate printf by outputing
   to stdout this way).

   INPUTS:

   pfAppend: The is a fsl_appendf_f function which is responsible
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

   The return value is the total number of characters sent to the
   function "func", or a negative number on a pre-output error. If
   this function returns an integer greater than 1 it is in general
   impossible to know if all of the elements were output. As such
   failure can only happen if the callback function returns an
   error or if one of the formatting options needs to allocate
   memory and cannot. Both of those cases are very rare in a
   printf-like context, so this is not considered to be a
   significant problem. (The same is true for any classical printf
   implementations.) Clients may use their own state objects which
   can propagate errors from their own internals back to the
   caller, but generically speaking it is difficult to trace errors
   back through this routine. Then again, in practice that has
   never proven to be a problem.

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

   FIXME? fsl_appendf_f() is an artifact of older code from which
   this implementation derives. The first parameter should arguably
   be replaced with fsl_output_f(), which does the same thing _but_
   has different return semantics (more reliable, because the
   current semantics report partial success as success in some
   cases). Doing this would require us to change the return
   semantics of this function, but that wouldn't necessarily be a
   bad thing (we don't rely on sprintf()-like return semantics all
   that much, if at all). Or we just add a proxy which forwards to
   a fsl_output_f(). (Oh, hey, that's what fsl_outputf() does.) But
   that doesn't catch certain types of errors (namely allocation
   failures) which can happen as side-effects of some formatting
   operations.

   Potential TODO: add fsl_bytes_fossilize_out() which works like
   fsl_bytes_fossilize() but sends its output to an fsl_output_f()
   and fsl_appendf_f(), so that this routine doesn't need to alloc
   for that case.
*/
FSL_EXPORT fsl_int_t fsl_appendfv(fsl_appendf_f pfAppend, void * pfAppendArg,
                                  const char *fmt, va_list ap );

/**
   Identical to fsl_appendfv() but takes an ellipses list (...)
   instead of a va_list.
*/
FSL_EXPORT fsl_int_t fsl_appendf(fsl_appendf_f pfAppend,
                      void * pfAppendArg,
                      const char *fmt,
                      ... )
#if 0
/* Would be nice, but complains about our custom format options: */
  __attribute__ ((__format__ (__printf__, 3, 4)))
#endif
  ;

/**
   A fsl_appendf_f() impl which requires that state be an opened,
   writable (FILE*) handle.
*/
FSL_EXPORT fsl_int_t fsl_appendf_f_FILE( void * state, char const * s,
                                         fsl_int_t n );


/**
   Emulates fprintf() using fsl_appendf(). Returns the result of
   passing the data through fsl_appendf() to the given file handle.
*/
FSL_EXPORT fsl_int_t fsl_fprintf( FILE * fp, char const * fmt, ... );

/**
   The va_list counterpart of fsl_fprintf().
*/
FSL_EXPORT fsl_int_t fsl_fprintfv( FILE * fp, char const * fmt, va_list args );


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

   Returns NULL for invalidate arguments or allocation error.
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
enum fsl_diff_flag_t {
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
   internal flag value. That's a good thing, because we'll be out
   of flags once Jan's done tinkering in fossil(1) ;).
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
FSL_EXPORT char fsl_iso8601_to_julian( char const * zDate, double * pOut );

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
FSL_EXPORT char fsl_julian_to_iso8601( double J, char * pOut, bool addMs );

/**
   Returns the Julian Day time J value converted to a Unix Epoch
   timestamp. It assumes 86400 seconds per day and does not account
   for any sort leap seconds, leap years, leap frogs, or any other
   kind of leap, up to and including a leap of faith.
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
