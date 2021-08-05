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

#include "fossil-scm/fossil-util.h" /* MUST come first b/c of config macros */
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
FSL_DBROLE_MAIN = 0x08
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
FSL_EXPORT int fsl_cx_err_report( fsl_cx * f, char addNewline );

/**
   Unconditionally Moves db->error's state into f. If db is NULL then
   f's primary db connection is used. Returns FSL_RC_MISUSE if !f or
   (!db && f-is-not-opened). On success it returns f's new error code.

   The main purpose of this function is to propagate db-level
   errors up to higher-level code which deals directly with the f
   object but not the underlying db(s).

   @see fsl_cx_uplift_db_error2()
*/
FSL_EXPORT int fsl_cx_uplift_db_error( fsl_cx * f, fsl_db * db );

/**
   If rc is not 0 and f has no error state but db does, this calls
   fsl_cx_uplift_db_error() and returns its result, else returns
   rc. If db is NULL, f's main db connection is used. It is intended
   to be called immediately after calling a db operation which might
   have failed, and passed that operation's result.

   Results are undefined if db is NULL and f has no main db
   connection.
*/
FSL_EXPORT int fsl_cx_uplift_db_error2(fsl_cx *f, fsl_db * db, int rc);

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
FSL_EXPORT fsl_db * fsl_cx_db( fsl_cx * f );

/**
   If f is not NULL and has had its repo opened via
   fsl_repo_open(), fsl_ckout_open_dir(), or similar, this
   returns a pointer to that database, else it returns NULL.

   @see fsl_cx_db()
*/
FSL_EXPORT fsl_db * fsl_cx_db_repo( fsl_cx * f );

/**
   If f is not NULL and has had a checkout opened via
   fsl_ckout_open_dir() or similar, this returns a pointer to that
   database, else it returns NULL.

   @see fsl_cx_db()
*/
FSL_EXPORT fsl_db * fsl_cx_db_ckout( fsl_cx * f );

/**
   A helper which fetches f's repository db. If f has no repo db
   then it sets f's error state to FSL_RC_NOT_A_REPO with a message
   describing the requirement, then returns NULL.  Returns NULL if
   !f.

   @see fsl_cx_db()
   @see fsl_cx_db_repo()
   @see fsl_needs_ckout()
*/
FSL_EXPORT fsl_db * fsl_needs_repo(fsl_cx * f);

/**
   The checkout-db counterpart of fsl_needs_repo().

   @see fsl_cx_db()
   @see fsl_needs_repo()
   @see fsl_cx_db_ckout()
*/
FSL_EXPORT fsl_db * fsl_needs_ckout(fsl_cx * f);

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
FSL_EXPORT fsl_db * fsl_cx_db_config( fsl_cx * f );

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
