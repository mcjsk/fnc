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
#include "fossil-scm/fossil.h"
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
