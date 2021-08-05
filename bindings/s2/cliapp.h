/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#ifndef NET_WH_CLIAPP_H_INCLUDED
#define NET_WH_CLIAPP_H_INCLUDED
/**
   A mini-framework for handling some of the grunt work required by
   CLI apps. It's main intent is to provide a halfway sane system for
   handling CLI flags. It also provides an abstraction for CLI
   editing, supporting either libreadline or liblinenoise, but only
   supporting the most basic of editing facilities, not
   library-specific customizations (e.g. custom key bindings).

   This API has no required dependencies beyond the C89 standard
   libraries and requires no dynamic memory unless it's configured to
   use an interactive line-reading backend.

   License: Public Domain

   Author: Stephan Beal <stephan@wanderinghorse.net>
*/

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Flags for use with the CliAppSwitch::pflags field to modify
   how cliapp_process_argv() handles the switch.
*/
enum cliapp_switch_flags {
/**
   Indicates that a CliAppSwitch is only allowed to be provided once.
   If encountered more than once by cliapp_process_argv(), an error
   is triggered.
*/
CLIAPP_F_ONCE = 1,
/**
   Indicates that a given CliAppSwitch requires a value and accepts
   its value either in the form (-switch=value) or (-switch value).
   When such a flag is encountered, cliapp_process_argv() will report
   an error if the switch has no value (even if the flag appears at
   the end of the arguments list or immediately before the
   special-case "--" flag).

   Without this flag, switch values are only recognized in the form
   (-switch=flag).
*/
CLIAPP_F_SPACE_VALUE = 2
/*TODO?: CLIAPP_F_VALUE_REQUIRED = 4 (implied by CLIAPP_F_SPACE_VALUE) */
};

/**
   Result codes used by various library routines.
*/
enum cliapp_rc {
/** The non-error code */
CLIAPP_RC_OK = 0,
/** Indicates an error in flag processing. */
CLIAPP_RC_FLAG = -1,
/** Indicates a range-related error, e.g. buffer overrun or too many
    CLI arguments. */
CLIAPP_RC_RANGE = -2,
/** Indicates that an unsupported operation was requested. */
CLIAPP_RC_UNSUPPORTED = -3,
/** Indicates some sort of I/O error. */
CLIAPP_RC_IO = -4
};


/**
   Holds state for a single CLI argument, be it a flag or non-flag.
*/
struct CliAppArg {
  /**
     Must be 1 for single-dash flags, 2 for double-dash flags, and -1
     for '+' flags, and 0 for non-flags.
  */
  int dash;
  /**
     Fow switches, this holds the switch's key, without dashes. For
     non-switches, it holds the argument's value.
  **/
  char const * key;
  /**
     For switches, the description of their value (if any) is stored here.
     For non-switches, this is 0.
  */
  char const * value;
  /**
     Arbitrary value which may be set/used by the client. It's not
     used/modifed by this API.
  */
  int opaque;
};
typedef struct CliAppArg CliAppArg;

struct CliAppSwitch;

/**
   A callback type used by cliapp_process_argv() to notify the app
   when a CLI argument is processed which matches one of the app's
   defined flags. It is passed the argument index, the switch (if any)
   and the argument.

   Note that when processing switches flagged with
   CLIAPP_F_SPACE_VALUE, the indexes passed to this function might
   have gaps, as they skip over the VAL part of (-f VAL), instead
   effectively transforming that to (-f=VAL) before calling the
   callback for the -f switch.

   If the switch argument is NULL, the argument will be a non-flag
   value. Note that arguments starting at the special-case "--" flag
   are not passed on to the callback. Instead, such arguments get
   reported via the cliApp.doubleDash member.

   It gets passed two NULL values one time at the end of processing in
   order to allow the client code to do any final validation.

   If it returns non-0, cliapp_process_argv() will fail and return the
   callback's result.
*/
typedef int (*CliAppSwitch_callback_f)(int ndx, struct CliAppSwitch const * appSwitch,
                                       CliAppArg * arg);
/**
   Models a single flag/switch for a CLI app. It's not called
   CliAppFlag because that proved confusing together with CliAppArg.
*/
struct CliAppSwitch {
  /**
     Can be used by clients to, e.g. group help items by type or
     set various levels of help verbosity.
  */
  int opaque;
  /**
     As documented for CliAppArg::dash.
  */
  int dash;
  /**
     The flag name, without leading dashes.
  */
  char const * key;
  /**
     A human-readable description of its expected value, or 0 if the
     flag does not require a value.

     BUG? It may still be assigned a value by the caller. We
     currently require that behaviour for a special-case arg handler
     in s2sh2.
  */
  char const * value;
  /**
     Brief help text.
  */
  char const * brief;
  /**
     Optional detailed help text.
  */
  char const * details;

  /**
     Optional callback to be passed a CliAppArg instance after it's been
     initialized and confirmed as being a valid arg (defined in
     cliApp.switches). If cliApp.argCallack is also used, both
     callbacks are called, but this one is called first.
  */
  CliAppSwitch_callback_f callback;

  /**
     Reserved for future use by the cliapp interface, e.g. marking
     "has seen this flag before" in order to implement only-once
     behaviour.
  */
  int pflags;
};
typedef struct CliAppSwitch CliAppSwitch;

/**
   Client-defined CliApp.argv arrays MUST end with an entry identical
   to this one. The iteration-related APIs treat any entry which
   memcmp()'s as equivalent to this entry as being the end of th list.

   @see cliapp_switch_is_end()
*/
#define CliAppSwitch_sentinel {0,0,0,0,0,0,0,0}

/**
   Returns true (non-0) if the given object memcmp()'s as equivalent
   to CliAppSwitch_sentinel.
 */
int cliapp_switch_is_end(CliAppSwitch const *s);

/**
   vprintf()-compatible logging/printing interface for use with
   CliApp.
*/
typedef int (*CliApp_print_f)(char const *, va_list);

/**
   A callback for use with cliapp_switches_visit(). It is passed the
   switch object and an arbitrary state pointer provided by the
   caller of that function.   
*/
typedef int (*CliAppSwitch_visitor_f)(CliAppSwitch const *, void *);

/**
   Global app state. This class is intended to represent a singleton,
   the cliApp object.
*/
struct CliApp {
  /**
     Number of arguments in this->argv. It is modified as
     cliapp_process_argv() executes and only counts arguments
     up to, but not including the special-case "--" flag.
  */
  int argc;

  /**
     Arguments processed by cliapp_process_argv(). Contains
     this->argc entries. This memory is not valid until
     cliapp_process_argv() has succeeded.
  */
  CliAppArg * argv;

  /**
     Internal cursor for traversing non-flag arguments using
     cliapp_arg_nonflag(). Holds the *next* index to be used by that
     function.
  */
  int cursorNonflag;

  /**
     May be set to an error description by certain APIs and it may
     point to memory which can mutate.
  */
  char const * errMsg;

  /**
    Must be set up by the client *before* calling
    cliapp_process_argv() and its final entry MUST be an object for
    which cliapp_switch_is_end() returns true (that's how we know when
    to stop processing).
  */
  CliAppSwitch const * switches;

  /**
     If this is non-NULL, cliapp_print() and friends will use it for
     output, otherwise they will elide all output. This defaults to
     vprintf().
  */
  CliApp_print_f print;

  /**
     If set, it gets called each time cliapp_process_argv() processes
     an argument. If it returns non-0, processing fails.

     After processing successfully completes, the callback is called
     one final time with NULL arguments so that the callback can
     perform any end-of-list validation or whatnot.

     Using this callback effectively turns cliapp_process_argv() into
     a push parser, which turns out to be a pretty convenient way to
     handle CLI flags.
  */
  CliAppSwitch_callback_f argCallack;

  /**
     If cliapp_process_argv() encounters the "--" flag, and additional
     arguments follow it, this object gets filled out with information
     about them.

     Note that encountering "--" with no following arugments is not
     considered an error.
  */
  struct {
    /**
       If cliapp_process_argv() encounters "--", this value gets set
       to the number of arguments available in the original argv array
       immediately following (but not including) the "--" flag
    */
    int argc;
    /**
       If cliapp_process_argv() encounters "--", and there are
       arguments after it, this value is set to the list of arguments
       (from the original argv array) immediately following the "--"
       flag. If "--" is not encountered, or there are no arguments
       after it, this member's value is 0.
    */
    char const * const * argv;
  } doubleDash;
  
  /**
     State related to interactive line-editing/reading.
  */
  struct {
    /**
       If enabled at compile-time, this has a value of 1 (for
       linenoise) or 2 (for readline), else it has a value of 0.

       To use libreadline, compile this code's C file with
       CLIAPP_ENABLE_READLINE set to a true value. To use linenoise,
       build with CLIAPP_ENABLE_LINENOISE set to a true value.
    */
    int const enabled;
    /**
       If non-NULL, cliapp_lineedit_save(NULL) will use this name for
       saving.
    */
    char const * historyFile;
    /**
       Specifies whether or not the line editing history has been
       modified since the last save.

       This initially has a value of 0 and it gets set to non-0 if
       cliapp_lineedit_add() is called.
    */
    int needsSave;
  } lineread;
};

/**
   Behold! The One True Instance of CliApp!
*/
extern struct CliApp cliApp;

/**
   Visits all switches in cliApp.switches, calling
   visitor(theSwitch,state) for each one. If the visitor returns
   non-0, visitation halts without an error.

   It stops iterating when it encounters an entry for which
   cliapp_switch_is_end() returns true.
*/
void cliapp_switches_visit( CliAppSwitch_visitor_f visitor,
                            void * state );

/**
   Callback signature for use with cliapp_args_visit().

   It gets passed the CLI argument, the index of that argument in
   cliApp.argv, and an optional client-specified state pointer.
*/
typedef int (*CliAppArg_visitor_f)(CliAppArg const *, int ndx, void *);

/**
   Visits all args in cliApp.argv, calling visitor(theSwitch,itsIndex,state)
   for each one. If skipArgs is greater than 0, that many are skipped
   over before visiting. Behaviour is undefined if a visitor modifies
   cliApp.argv or cliApp.argc. If the visitor returns non-0,
   visitation halts without an error.

   CliAppArg entries with a NULL key are skipped over, under the assumption
   that the client app has marked them as "removed".
*/
void cliapp_args_visit( CliAppArg_visitor_f visitor, void * state,
                        unsigned short skipArgs );

/**
   Initializes the argument-processing parts of the cliApp global
   object with. It is intended to be passed the conventional argc/argv
   arguments which are passed to the application's main().

   The final parameter is reserved for future use in providing flags
   to change this function's behaviour. A value of 0 is reserved as
   meaning "the default behaviour."

   cliApp.switches must have been assigned to non-NULL before calling
   this, or behaviour is undefined. If any given switch has a callback
   assigned to it, it will be called when that switch is processed,
   and processing fails if it returns non-0. (Potential TODO: allow a
   NULL switches value to simply treat all flags a known switches.)

   If cliApp.argCallack is not-NULL, it is called for every
   argument. It will be passed the CLI argument and, if it's a flag,
   its corresponding CliAppSwitch instance (extracted from
   cliApp.switches). For non-flag arguments, a NULL CliAppSwitch is
   passed to it. If it returns non-0, processing fails. If processes
   completes successfully, the callback is called one additional time
   with NULL pointer values to indicate that the end has been
   reached. This can be used to handle post-argument cleanup, perform
   app-specific argument validation, or similar.

   If callbacks are set both on the switch and cliApp, both are called
   in that order, but only the cliApp callback is called one final
   time after processing is done.

   If this function returns 0, the client may manipulate the contents
   of cliApp.argv, within reason, but must be certain to keep
   cliApp.argc in sync with that list's entries.

   On error a non-0 code is returned, either propagated from a
   callback or (if the error originates from this function) an entry
   from the cliapp_rc enum. In the latter case, cliapp_err_get() will
   contain information about why it failed.

   Encountering an argument which is neither a non-flag nor a flag
   defined in cliApp.switches results in an error.

   Quirks:

   - Arguments after "--" are NOT processed by this
   function. Processing them would be a bug-in-waiting because those
   flags might collide with app-level flags and/or require syntaxes
   which this code treats as an error, e.g. using three dashes instead
   of 1 or 2. Instead, if "--" is encounter, cliApp.doubleDash is
   populated with information about the flags so the client may deal
   with them (which might mean passing them back into this routine!).

   - All argv-related cliApp state is reset on each call, so if this
   function is called multiple times, any client-side pointers
   referring to cliApp's state may then point to different information
   than they expect and/or may become stale pointers. (cliApp-held
   data, e.g. cliApp.argv, keeps the same pointers but re-populates
   the state, but the lifetime of external pointers,
   e.g. cliApp.doubleDash.argv, is client-dependent.)
*/
int cliapp_process_argv(int argc, char const * const * argv,
                        unsigned int reserved);

/**
   If cliApp.print is not NULL, this passes on its arguments to that
   function, else this is a no-op.
*/
void cliapp_printv(char const *fmt, va_list);

/**
   Elipses-args form of cliapp_printv().
*/
void cliapp_print(char const *fmt, ...);

/**
   Outputs a printf-formatted message to stderr.
*/
void cliapp_warn(char const *fmt, ...);

/**
   Returns the next entry in cliApp.argv which is a non-flag argument,
   skipping over argv[0]. Returns 0 when the end of the list is
   reached.
*/
CliAppArg const * cliapp_arg_nonflag();

/**
   Resets the traversal of cliapp_arg_nonflag() to start from
   the beginning.
*/
void cliapp_arg_nonflag_rewind();

/**
   If the given argument matches an app-configured flag, that flag is
   returned, else 0 is returned.

   If alsoFlag is true, the first argument and the corresponding
   switch must also have matching flag values to be considered a
   match.
*/
CliAppSwitch const * cliapp_switch_for_arg(CliAppArg const * arg,
                                           int alsoFlag);

/**
   Searches for a flag matching one of the given keys. Each entry
   in cliApp.argv is checked, in order, against both of the given
   keys, in the order they are provided.

   The conventional way to call it is to pass the short-form flag,
   then the long-form flag, but that's just a convention.

   Either of the first two arguments may be NULL but both may not be
   NULL.

   If the 3rd parameter is not NULL then:

   1) *atPos indicates an index position to start the search at. (Note
   that it should initially be 1, not 0, in order to skip over the
   app's name, stored in argv[0].)

   2) If non-NULL is returned, *atPos is set to the index at which the
   argument was found. If NULL is returned, *argPos is not modified.

   Thus atPos can be used to iterate through multiple copies of a
   flag, noting that its value points to the index at which the
   previous entry was found, so needs to be incremented by 1 before
   each subsequent iteration

   On a match, the corresponding CliAppArg is returned, else 0 is
   returned.
*/
CliAppArg const * cliapp_arg_flag(char const * key1, char const * key2,
                                  int * atPos);

/**
   Given a flag value for a CliAppArg or CliAppSwitch, this
   returns a prefix string depending on that value:

   1 = "-", 2 = "--", 3 = "+"

   Anything else = "". The returned bytes are static.
*/
char const * cliapp_flag_prefix( int flag );

/**
   Given a CliAppArg, presumably one from cliapp_arg_flag() or
   cliapp_arg_nonflag(), this searches for the next argument with the
   same key.

   If the given argument is from outside cliApp.argv's memory range,
   or is the last element in that list, 0 is returned.

   Bug? For non-flag arguments this does not update the internal
   non-flag traversal cursor.
*/
CliAppArg const * cliapp_arg_next_same(CliAppArg const * arg);

/**
   Clears any error state in the cliApp object.
*/
void cliapp_err_clear();

/**
   If cliApp has a current error message set, it is returned, else 0
   is returned. The memory is static and its contents may be modified
   by any calls into this API.
*/
char const * cliapp_err_get();


/**
   Tries to save the line-editing history to the given filename, or to
   cliApp.lineedit.historyFile if fname is NULL. If both are NULL or
   empty, or if cliApp.lineedit.needsSave is 0, this is a no-op and
   returns 0. Returns CLIAPP_RC_UNSUPPORTED if line-editing is not
   enabled.
*/
int cliapp_lineedit_save(char const * fname);

/**
   Adds the given line to the line-edit history. If this function
   returns 0, it also sets cliApp.lineedit.needsSave to a non-0 value.

   Returns 0 on success or CLIAPP_RC_UNSUPPORTED if line-editing is
   not enabled.
*/
int cliapp_lineedit_add(char const * line);

/**
   Tries to load the line-editing history from the given filename, or
   to cliApp.lineedit.historyFile if fname is NULL. If both are NULL
   or empty, this is a no-op. Returns CLIAPP_RC_UNSUPPORTED if
   line-editing is not enabled. If the underlying line-editing backend
   returns an error, CLIAPP_RC_IO is returned, under the assumption
   that there was a problem with reading the file (e.g. unreadable),
   as opposed to an allocation error or similar.
*/
int cliapp_lineedit_load(char const * fname);

/**
   If cliApp.lineedit.enabled is true, this function passes its
   argument to free(3), else it will (in debug builds) trigger an
   assert if passed non-NULL. This must be called once for each line
   fetched via cliapp_lineedit_read().
*/
void cliapp_lineedit_free(char * line);

/**
   If line-editing is enabled, this reads a single line using that
   back-end and returns the new string, which must be passed to
   cliapp_lineedit_free() after the caller is done with it.

   Returns 0 if line-editing is not enabled or if the caller taps the
   platform's EOF sequence (Ctrl-D on Unix) at the start of the
   line. Returns an empty string if the user simply taps ENTER.

   TODO: if no line-editing backend is built in, fall back to fgets()
   on stdin. It ain't pretty, but it'll do in a pinch.
*/
char * cliapp_lineedit_read(char const * prompt);

/**
   Callback type for use with cliapp_repl().
*/
typedef int (*CliApp_repl_f)(char const * line, void * state);

/**
   Enters a REPL (Read, Eval, Print Loop). Each iteration does
   the following:

   1) Fetch an input line using cliapp_lineedit_read(), passing it
   *prompt. If that returns NULL, this function returns 0.

   2) If addHistoryPolicy is <0 then the read line is added to the
   history.

   3) Calls callback(theReadLine, state).

   4) If (3) returns 0 and addHistoryPolicy is >0, the read line
   is added to the history.

   5) Passes the read line to cliapp_lineedit_free().

   6) If (3) returns non-0, this function returns that value.

   Notes:

   - The prompt is a pointer to a pointer so that the caller may
   modify it between loop iterations. This function derefences *prompt
   on each iteration.

   - An addHistoryPolicy of 0 means that this function will not
   automatically add input lines to the history. The callback is free
   to do so.

   - This function never passes a NULL line value to the callback but
   it may pass an empty line.
*/
int cliapp_repl(CliApp_repl_f callback, char const * const * prompt,
                int addHistoryPolicy, void * state);

#ifdef __cplusplus
}
#endif
#endif /* NET_WH_CLIAPP_H_INCLUDED */

