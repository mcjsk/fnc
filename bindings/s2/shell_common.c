/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */

/************************************************************************

This file is intended to be included directly into shell.c
(a.k.a. s2sh) and shell2.c (a.k.a. s2sh2), not compiled by itself. It
contains all of the code which is common to both versions of the shell
(which is everything but their --help bits).

************************************************************************/

#ifndef S2SH_VERSION
#  error define S2SH_VERSION to 1 or 2 before including this file.
#elif (S2SH_VERSION!=1) && (S2SH_VERSION!=2)
#  error S2SH_VERSION must bet to 1 or 2.
#endif

#include <assert.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#if defined(S2_AMALGAMATION_BUILD) || 2==S2SH_VERSION
#  include "s2_amalgamation.h"
#else
#  include "s2.h"
#endif
/* ^^^^ may include config stuff used by system headers */

#if !defined(S2SH_FOR_UNIT_TESTS)
/* This option enables options which are only in place for
   purposes of s2's own unit test suite. e.g. to communicate
   compile-time options which may change test paths.
*/
#  define S2SH_FOR_UNIT_TESTS 0
#endif

#include "cliapp.h"

#if defined(S2_OS_UNIX)
#  include <unistd.h> /* isatty() and friends */
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <locale.h> /* setlocale() */

/* Must be defined by shell.c/shell2.c in a way compatible
   with cliApp.switches. */
static CliAppSwitch const * s2sh_cliapp_switches();

/**
   ID values and flags for CLI switches. Some are only used by one of
   s2sh or s2sh2.
*/
enum s2sh_switch_ids {
/** Mask of the bits used as flag IDs. The remainder are flag/modifier
    bits. */
SW_ID_MASK = 0xFFFF,
SW_ID_MASK_bits = 16,
#define ID(X) (X<< SW_ID_MASK_bits)

/**
   An entry flagged as SW_INFOLINE is intended to be a standalone line
   of information unrelated to a specific flag. If it needs to span
   lines, it must embed \n characters. --help outputs its "brief"
   member.
*/
SW_INFOLINE = ID(1),
/**
   An entry flagged with SW_CONTINUE must immediately follow either an
   SW_INFOLINE, a switch entry (SW_ID_MASK), or another entry with
   this flag. --help outputs its "brief" member, indented or not,
   depending on the previous entry.
*/
SW_CONTINUE = ID(2),

/**
   SW_GROUP_x are used for grouping related elements into tiers.
*/
SW_GROUP_0 = ID(4),
SW_GROUP_1 = ID(8),
SW_GROUP_2 = ID(0x10),
SW_GROUPS = SW_GROUP_0 | SW_GROUP_1 | SW_GROUP_2,

/**
   Switches with this bit set are not output by --help.
   (Workaround for s2sh -help/--help/-? having 3 options!)
*/
SW_UNDOCUMENTED = ID(0x20),

#undef ID

SW_switch_start = 0,
/*
  Entries for individual CLI switches...

  DO NOT bitmask the SW_GROUP entries directly with the
  switch-specific entries. They are handled in a separate step. Long
  story.

  The SW_ID_MASK bits of the switch ID values need not be bitmasks.
  i.e. they may have bits which overlap with other entries, provided that
  the SW_ID_MASK part is unique.

  All switches which *require* a value, as opposed to optionally
  accept a value, should, for consistency, have their pflags member
  OR'd with CLIAPP_F_SPACE_VALUE.

  All switches which may only be provided once must have their pflags
  field OR'd with CLIAPP_F_ONCE.
*/
SW_CLAMPDOWN/*v1. No longer allowed.*/,
SW_CLEANROOM,
SW_DISABLE,
SW_ESCRIPT,
SW_HELP,
SW_INFILE,
SW_INTERACTIVE,
SW_INTERNAL_FU,
SW_METRICS,
SW_OUTFILE,
SW_SHOW_SIZEOFS,
SW_SWEEP_INTERVAL,
SW_VAC_INTERVAL,
SW_VERBOSE,
SW_VERSION,

/**
  The entries ending with 0 or 1 are toggles. The #1 part of each pair
  MUST have a value equal to the corresponding #0 entry plus 1.
*/
SW_MEM_FAST0, SW_MEM_FAST1,
/** SW_RE_xxx = recycling-related options */
/* [no-]recycle-chunks */
SW_RE_C0, SW_RE_C1,
/* [no-]string-interning */
SW_RE_S0, SW_RE_S1,
/* [no-]recycle-values */
SW_RE_V0, SW_RE_V1,
/* [no-]scope-hashes */
SW_SCOPE_H0, SW_SCOPE_H1,
/* --T/-T */
SW_TRACE_STACKS_0, SW_TRACE_STACKS_1,
/* v1: --W/-w, v2 */
SW_TRACE_SWEEP_0, SW_TRACE_SWEEP_1,

/* v1: --s2.shell/-s2.shell, v2: na/--s2.shell */
SW_SHELL_API_0, SW_SHELL_API_1,
/* v1: --M/-M, v2: --no-module-init/na */
SW_MOD_INIT_0, SW_MOD_INIT_1,
/** v1: --A/-A */
SW_ASSERT_0, SW_ASSERT_1,
/** v1: --h/-h */
SW_HIST_FILE_0, SW_HIST_FILE_1,
/** v1: --a/-a, v2: na/-I
*/
SW_AUTOLOAD_0, SW_AUTOLOAD_1,

SW_TRACE_CWAL,

/* Memory-capping options... */
SW_MCAP_TOTAL_ALLOCS,
SW_MCAP_TOTAL_BYTES,
SW_MCAP_CONC_ALLOCS,
SW_MCAP_CONC_BYTES,
SW_MCAP_SINGLE_BYTES,

SW_end /* MUST be the last entry */
};


#if !defined(S2_SHELL_EXTEND_FUNC_NAME)
#  define S2_SHELL_EXTEND_FUNC_NAME s2_shell_extend
#endif
/*
  If S2_SHELL_EXTEND is defined, the client must define
  s2_shell_extend() (see shell_extend.c for one implementation) and
  link it in with this app. It will be called relatively early in the
  initialization process so that any auto-loaded scripts can make use
  of it. It must, on error, return one of the non-0 CWAL_RC_xxx error
  codes (NOT a code from outside the range! EVER!). An error is
  treated as fatal to the shell.

  See shell_extend.c for a documented example of how to use this
  approach to extending this app.
*/
#if defined(S2_SHELL_EXTEND)
extern int S2_SHELL_EXTEND_FUNC_NAME(s2_engine * se, cwal_value * mainNamespace,
                                     int argc, char const * const * argv);
#endif

/*
  MARKER((...)) is an Internally-used output routine which
  includes file/line/column info.
*/
#define MARKER(pfexp) \
  if(1) cliapp_print("%s:%d:  ",__FILE__,__LINE__); \
  if(1) cliapp_print pfexp
#define MESSAGE(pfexp) cliapp_print pfexp
#define WARN(pfexp) \
    if(1) cliapp_warn("%s:  ",App.appName); \
    if(1) cliapp_warn pfexp
#define VERBOSE(LEVEL,pfexp) if(App.verbosity >= LEVEL) { cliapp_print("verbose: "); MESSAGE(pfexp); }(void)0

/**
   S2_AUTOINIT_STATIC_MODULES tells the shell whether to initialize, or
   not, statically-linked-in modules by default.
*/
#if !defined(S2_AUTOINIT_STATIC_MODULES)
# define S2_AUTOINIT_STATIC_MODULES 1
#endif

/**
   S2SH_MAX_SCRIPTS = max number of scripts to run, including the -e
   scripts and the main script filename, but not the autoload script.
*/
#define S2SH_MAX_SCRIPTS 10

/**
   Global app-level state.
*/
static struct {
  char const * appName;
  char enableArg0Autoload;
  int enableValueRecycling;
  /**
     Max chunk size for mem chunk recycler. Set to 0 to disable.  Must
     allocate sizeof(void*) times this number, so don't set it too
     high.
  */
  cwal_size_t maxChunkCount;
  int enableStringInterning;
  int showMetrics;
  int verbosity;
  int traceAssertions;
  int traceStacks;
  int traceSweeps;
  char scopesUseHashes;
  char const * inFile;
  char const * outFile;
  char const * const * scriptArgv;
  int scriptArgc;
  int addSweepInterval;
  int addVacuumInterval;
  /**
     Name of interactive most history file. Currently
     looks in/writes to only the current dir.
  */
  char const * editHistoryFile;
  /**
     Default prompt string for the line-reading functions.
  */
  char const * lnPrompt;
  /**
     0=non-interactive mode, >0=interactive mode, <0=as-yet-unknown.
  */
  int interactive;
  /**
     This is the "s2" global object. It's not generally kosher to hold
     global cwal_values, but (A) we're careful in how we do so and (B)
     it simplifies a couple of our implementations.
  */
  cwal_value * s2Global;
  /**
     When shellExitFlag!=0, interactive readline looping stops.
  */
  int shellExitFlag;

  /**
     If true, do not install prototypes or global functionality.  Only
     "raw s2" is left.
   */
  int cleanroom;

  /**
     Flags for use with s2_disable_set_cstr().
  */
  char const * zDisableFlags;

  /**
     If true AND if "static modules" have been built in, this
     initializes those modules.
  */
  int initStaticModules;

  /**
     A flag used to tell us not to save the history - we avoid saving
     when the session has no commands, so that we don't get empty
     history files laying around. Once s2sh_history_add() is called,
     this flag gets set to non-0.

  */
  int saveHistory;

  /**
     A flag to force installation of the s2.shell API, even in non-interactive
     mode. <0 = determine automatically. 0 = force off. >0 = force on unless
     -cleanroom trumps it.
  */
  int installShellApi;

  /**
     Don't look at this unless you know _exactly_ what you're doing
     with s2's internals. It's for my eyes only.
   */
  int enableInternalFu;

  /**
     Memory-capping configuration. Search this file for App.memcap for
     how to set it up.
  */
  cwal_memcap_config memcap;

  /**
     Scripts for the -e/-f flags.
  */
  struct {
    int nScripts /*current # of entries in this->e*/;
    struct {
      /* True if this->src is a filename, else it's script code. */
      int isFile;
      /* Filename or script code. */
      char const * src;
    } e[S2SH_MAX_SCRIPTS];
  } scripts;
  int32_t cwalTraceFlags;
} App = {
0/*appName*/,
1/*enableArg0Autoload*/,
1/*enableValueRecycling*/,
50/*maxChunkCount*/,
1/*enableStringInterning*/,
0/*showMetrics*/,
0/*verbosity*/,
0/*traceAssertions*/,
0/*traceStacks*/,
0/*traceSweeps*/,
0/*scopesUseHashes*/,
0/*inFile*/,
0/*outFile*/,
0/*scriptArgv*/,
0/*scriptArgc*/,
0/*addSweepInterval*/,
0/*addVacuumInterval*/,
"s2sh.history"/*editHistoryFile*/,
#if 1==S2SH_VERSION
"s2sh> "/*lnPrompt*/,
#else
"s2sh2> "/*lnPrompt*/,
#endif
-1/*interactive*/,
0/*s2Global*/,
0/*shellExitFlag*/,
0/*cleanroom*/,
0/*zDisableFlags*/,
S2_AUTOINIT_STATIC_MODULES/*initStaticModules*/,
0/*saveHistory*/,
-1/*installShellApi*/,
0/*enableInternalFu*/,
cwal_memcap_config_empty_m/*memcap*/,
{0/*nScripts*/, {/*e*/{0,0}}},
0/*cwalTraceFlags*/
};


static cwal_engine * printfEngine = 0;
static void s2sh_print(char const * fmt, ...) {
  va_list args;
  va_start(args,fmt);
  if(printfEngine){
    cwal_outputfv(printfEngine, fmt, args);
  }else{
      cliapp_print(fmt,args);
  }
  va_end(args);
}

/**
   Called by cwal_engine_init(). This does the lower-level (pre-s2)
   parts of the cwal_engine configuration. This part must not create
   any values, as s2 will destroy them right after this is called so
   that it can take over ownership of the top-most scope. Thus only
   configurable options are set here, and new functionality is added
   during the s2 part of the initialization.
*/
static int s2sh_init_engine(cwal_engine *e, cwal_engine_vtab * vtab){
  int rc = 0;
  int32_t featureFlags = 1
    ? 0
    : CWAL_FEATURE_ZERO_STRINGS_AT_CLEANUP;

  cwal_engine_trace_flags(e, App.cwalTraceFlags);
  if(App.enableStringInterning){
    featureFlags |= CWAL_FEATURE_INTERN_STRINGS;
  }
  if(App.scopesUseHashes){
    /* If enabled, scopes will use hashtables for property storage.
       This are potentially more costly, in terms of memory, but
       are much faster. */
    featureFlags |= CWAL_FEATURE_SCOPE_STORAGE_HASH;
    if(vtab){/*avoid unused param warning*/}
  }else{
    featureFlags &= ~CWAL_FEATURE_SCOPE_STORAGE_HASH;
  }
  cwal_engine_feature_flags(e, featureFlags);

  {
    /* Configure the memory chunk recycler... */
    cwal_memchunk_config conf = cwal_memchunk_config_empty;
    conf.maxChunkCount = App.maxChunkCount;
    conf.maxChunkSize = 1024 * 32;
#if 16 == CWAL_SIZE_T_BITS
    conf.maxTotalSize = 1024 * 63;
#else
    conf.maxTotalSize = 1024 * 256;
#endif
    conf.useForValues = 0 /* micro-optimization: true tends towards
                             sub-1% reduction in mallocs but
                             potentially has aggregate slower value
                             allocation for most cases */ ;
    rc = cwal_engine_memchunk_config(e, &conf);
    assert(!rc);
  }

#define REMAX(T,N) cwal_engine_recycle_max( e, CWAL_TYPE_ ## T, (N) )
  REMAX(UNDEF,0) /* disables recycling for all types */;
  if(App.enableValueRecycling){
    /* A close guess based on the post-20141129 model...  List them in
       "priority order," highest priority last.  Lower prio ones might
       get trumped by a higher prio one: they get grouped based on the
       platform's sizeof() of their concrete underlying bytes.
    */
    REMAX(UNIQUE,20)/* will end up being trumped by integer (32-bit)
                       and/or double (64-bit) */;
    REMAX(KVP,80) /* guaranteed individual recycler */;
    REMAX(WEAK_REF,30) /* guaranteed individual recycler */;
    REMAX(STRING,50) /* guaranteed individual recycler */;
    REMAX(EXCEPTION,3);
    REMAX(HASH,15) /* might also include: function, native, buffer */;
    REMAX(BUFFER,20) /* might also include: function, native, buffer, hash */ ;
    REMAX(XSTRING,20 /* also Z-strings and tuples, might also include doubles */);
    REMAX(NATIVE,20)  /* might also include: function, hash */;
    REMAX(DOUBLE,50)/* might also include z-/x-strings,
                       integer (64-bit), unique (64-bit)*/;
    REMAX(FUNCTION,50) /* might include: hash, native, buffer */;
    REMAX(ARRAY,30);
    REMAX(OBJECT,30) /* might include: buffer */;
    REMAX(INTEGER,80) /* might include: double, unique */;
    REMAX(TUPLE,30) /* also x-/z-strings, might include: double */;
  }
#undef REMAX
  /*
    Reminder to self: we cannot install functions from here because
    the s2_engine's Function prototype will not get installed by that
    time, so the functions we added would not have an 'undefined'
    prototype property. We must wait until the s2_engine is set up.

    s2 will also nuke the top scope so that it can push one of its
    own, so don't allocate any values here.
  */
  return rc;
}

/** String-is-internable predicate for cwal_engine. */
static bool cstr_is_internable( void * state,
                                char const * str_,
                                cwal_size_t len ){
  enum { MaxLen = 32U };
  unsigned char const * str = (unsigned char const *)str_;
  assert(str);
  assert(len>0);
  if(len>MaxLen){
    if(state){/*avoid unused param warning*/}
    return 0;
  }else if(1==len){
    /*if(*str<32) return 0;
      else
    */
    return (*str<=127) ? 1 : 0;
  }
  /* else if(len<4) return 1; */
  else{
    /* Read a UTF8 identifier... */
    char const * zEnd = str_+len;
    char const * tail = str_;
    s2_read_identifier(str_, zEnd, &tail);
    if((tail>str_)
       && (len==(cwal_size_t)(tail-str_))
       ){
      /* MARKER(("internable: %.*s\n", (int)len, str_)); */
      return 1;
    }
    /* MARKER(("Not internable: %.*s\n", (int)len, str_)); */
    return 0;
  }
}

static int s2_cb_s2sh_line_read(cwal_callback_args const * args, cwal_value ** rv){
  char const * prompt = 0;
  char * line = 0;
  if(args->argc){
    if(!(prompt = cwal_value_get_cstr(args->argv[0], 0))){
      return s2_cb_throw(args, CWAL_RC_MISUSE,
                         "Expecting a string or no arguments.");
    }
  }else{
    prompt = "prompt >";
  }
  if(!(line = cliapp_lineedit_read(prompt))){
    *rv = cwal_value_undefined();
    return 0;
  }else{
    *rv = cwal_new_string_value(args->engine, line, cwal_strlen(line));
    cliapp_lineedit_free(line)
      /* if we knew with absolute certainty that args->engine is
         configured for the same allocator as the readline bits, we
         could use a Z-string and save a copy. But we don't, generically
         speaking, even though it's likely to be the same in all but the
         weirdest configurations. Even so, cwal may "massage" memory a
         bit to add some metadata, so we cannot pass it memory from
         another allocator, even if we know they both use the same
         underlying allocator.
      */;
    return *rv ? 0 : CWAL_RC_OOM;
  }
}

/**
   Callback implementation for s2_cb_s2sh_history_(load|add|save)().
*/
static int s2_cb_s2sh_history_las(cwal_callback_args const * args,
                                  cwal_value ** rv,
                                  char const * descr,
                                  int (*func)(char const * str) ){
  char const * str = args->argc
    ? cwal_value_get_cstr(args->argv[0], 0)
    : 0;
  if(!str){
    return s2_cb_throw(args, CWAL_RC_MISUSE,
                       "Expecting a string argument.");
  }
  else{
    int rc = (*func)(str);
    if(rc){
      rc = s2_cb_throw(args, rc,
                       "Native %s op failed with code %d/%s.",
                       descr, rc, cwal_rc_cstr(rc));
    }else{
      *rv = cwal_value_undefined();
    }
    return rc;
  }
}

static int s2_cb_s2sh_history_save(cwal_callback_args const * args, cwal_value ** rv){
  return s2_cb_s2sh_history_las( args, rv, "history-save", cliapp_lineedit_save );
}

static int s2_cb_s2sh_history_load(cwal_callback_args const * args, cwal_value ** rv){
  return s2_cb_s2sh_history_las( args, rv, "history-load", cliapp_lineedit_load);
}

static int s2_cb_s2sh_history_add(cwal_callback_args const * args, cwal_value ** rv){
  return s2_cb_s2sh_history_las( args, rv, "history-add",cliapp_lineedit_add);
}

/**
   cwal_value_visitor_f() impl. Internal helper for s2_dump_metrics().
*/
static int s2sh_metrics_dump_funcStash_keys( cwal_value * v, void * state ){
  cwal_size_t sz = 0;
  char const * n = cwal_value_get_cstr(v, &sz);
  if(state){/*unused*/}
  s2sh_print("\t%.*s\n", (int)sz, n);
  return 0;
}

static void s2_dump_metrics(s2_engine * se){
  printfEngine = se->e;
#define OUT(pfexp) s2sh_print pfexp
  if(App.verbosity){
    OUT(("\n"));
    OUT(("cwal-level allocation metrics:\n"));
    cwal_dump_allocation_metrics(se->e);
    OUT(("\n"));
    if(App.verbosity>2 && App.enableStringInterning){
      cwal_dump_interned_strings_table(se->e, 1, 32);
    }
  }
  if(se->metrics.tokenRequests){
    OUT(("s2-side metrics:\n"));
    OUT(("Peak cwal_scope level: %d with %d s2_scopes pushed/popped\n",
         se->metrics.maxScopeDepth,
         se->metrics.totalScopesPushed));
    OUT(("%d s2_scopes (sizeof %u) were alloc'd across "
         "%d (re)alloc(s) with a peak of %d bytes "
         "(counted among cwal_engine::memcap::currentMem).\n",
         (int)se->scopes.alloced,
         (unsigned int)sizeof(s2_scope),
         se->metrics.totalScopeAllocs,
         (int)(se->scopes.alloced * sizeof(s2_scope))));
    OUT(("Peak eval depth: %d\n",
         se->metrics.peakSubexpDepth));
    OUT(("Total s2_stokens (sizeof=%u) requested=%u, "
         "allocated=%u (=%u bytes), alive=%u, peakAlive=%u, "
         "currently in recycle bin=%d\n",
         (unsigned)sizeof(s2_stoken), se->metrics.tokenRequests,
         se->metrics.tokenAllocs, (unsigned)(se->metrics.tokenAllocs * (unsigned)sizeof(s2_stoken)),
         se->metrics.liveTokenCount, se->metrics.peakLiveTokenCount,
         se->recycler.stok.size));
    OUT(("Total calls to s2_next_token(): %u\n",
         se->metrics.nextTokenCalls));
    if(se->metrics.tokenAllocs < se->metrics.tokenRequests){
      OUT(("Saved %u bytes and %u allocs via stack token recycling :D.\n",
           (unsigned)((se->metrics.tokenRequests * (unsigned)sizeof(s2_stoken))
                      - (se->metrics.tokenAllocs * (unsigned)sizeof(s2_stoken))),
           se->metrics.tokenRequests - se->metrics.tokenAllocs
           ));
    }
  }
  OUT(("Scratch buffer (used by many String APIs): %u byte(s)\n",
       (unsigned)se->buffer.capacity));

  if(se->metrics.ukwdLookups){
    OUT(("se->metrics.ukwdLookups=%u, "
         "se->metrics.ukwdHits=%u\n",
         se->metrics.ukwdLookups,
         se->metrics.ukwdHits));
  }
  if(se->metrics.funcStateAllocs){
    OUT(("Allocated state for %u of %u script-side function(s) "
         "(sizeof %u), using %u bytes.\n",
         se->metrics.funcStateAllocs,
         se->metrics.funcStateRequests,
         s2_sizeof_script_func_state(),
         se->metrics.funcStateMemory));
  }
  if(se->funcStash){
    OUT(("Function script name hash: hash size=%d, entry count=%d. "
         "Re-used name count: %d\n",
         (int)cwal_hash_size(se->funcStash),
         (int)cwal_hash_entry_count(se->funcStash),
         (int)se->metrics.totalReusedFuncStash
         ));
    if(App.verbosity>2){
      OUT(("Function script name hash entries:\n"));
      cwal_hash_visit_keys(se->funcStash,
                           s2sh_metrics_dump_funcStash_keys, 0);
    }
  }
  if(se->stash){
    cwal_hash const * const h = cwal_value_get_hash(se->stash);
    OUT(("s2_engine::stash hash size=%d, entry count=%d\n",
         (int)cwal_hash_size(h), (int)cwal_hash_entry_count(h)));
  }
  if(se->metrics.assertionCount
     && (App.showMetrics || App.traceAssertions)
     ){
    OUT(("script-side 'assert' checks: %u\n",
         se->metrics.assertionCount));
  }
  OUT(("(end of s2 metrics)\n"));
  printfEngine = 0;
#undef OUT
}

static int s2sh_cb_dump_metrics(cwal_callback_args const * args, cwal_value ** rv){
  s2_engine * se = s2_engine_from_args(args);
  int const oldVerbosity = App.verbosity;
  int newVerbose = args->argc
    ? (int)cwal_value_get_integer(args->argv[0])
    : 0;
  App.verbosity = newVerbose;
  s2_dump_metrics(se);
  App.verbosity = oldVerbosity;
  *rv = cwal_value_undefined();
  return 0;
}

static int s2sh_report_engine_error( s2_engine * se ){
  int rc = 0;
  cwal_error * const err = &se->e->err;
  if(err->code || se->flags.interrupted){
    char const * msg = 0;
    rc = s2_engine_err_get(se, &msg, 0);
    if(err->line>0){
      fprintf(stderr,
              "s2_engine says error #%d (%s)"
              " @ script [%s], line %d, column %d: %s\n",
              rc, cwal_rc_cstr(rc),
              (char const *)err->script.mem,
              err->line, err->col, msg);
    }else{
      fprintf(stderr,
              "s2_engine says error #%d (%s)%s%s\n",
              rc, cwal_rc_cstr(rc),
              (msg && *msg) ? ": " : "",
              (msg && *msg) ? msg : "");
    }
  }
  return rc;
}

/**
   General-purpose result reporting function. rc should be the rc from
   the function we got *rv from. This function takes over
   responsibility of *rv from the caller (who should not take a
   reference to *rv). rv may be 0, as may *rv.

   After reporting the error, s2_engine_sweep() is run, which _might_
   clean up *rv (along with any other temporary values in the current
   scope).
*/
static int s2sh_report_result(s2_engine * se, int rc, cwal_value **rv ){
  char const * label = "result";
  cwal_value * rvTmp = 0;
  char showedMessage = 0;
  if(!rv) rv = &rvTmp;
  switch(rc){
    case 0: break;
    case CWAL_RC_RETURN:
      *rv = cwal_propagating_take(se->e);
      assert(*rv);
      break;
    case CWAL_RC_EXCEPTION:
      label = "EXCEPTION";
      *rv = cwal_exception_get(se->e);
      assert(*rv);
      cwal_value_ref(*rv);
      cwal_exception_set(se->e, 0);
      cwal_value_unhand(*rv);
      break;
    case CWAL_RC_EXIT:
      rc = 0;
      *rv = s2_propagating_take(se);
      break;
    case CWAL_RC_BREAK:
      label = "UNHANDLED BREAK";
      *rv = s2_propagating_take(se);
      break;
    case CWAL_RC_CONTINUE:
      label = "UNHANDLED CONTINUE";
      *rv = 0;
      break;
    case CWAL_RC_FATAL:
      label = "FATAL";
      *rv = s2_propagating_take(se);
      assert(*rv);
      assert(se->e->err.code);
      CWAL_SWITCH_FALL_THROUGH;
    default:
      ;
      if(s2sh_report_engine_error(se)){
        showedMessage = 1;
      }else{
        WARN(("Unusual result code (no error details): %d/%s\n",
              rc,cwal_rc_cstr2(rc)));
      }
      break;
  }
  if(*rv){
    assert((cwal_value_scope(*rv) || cwal_value_is_builtin(*rv))
           && "Seems like we've cleaned up too early.");
    if(rc || App.verbosity){
      if(rc && !showedMessage){
        fprintf(stderr,"rc=%d (%s)\n", rc, cwal_rc_cstr(rc));
      }
      s2_dump_value(*rv, label, 0, 0, 0);
    }
    /* We are uncertain of v's origin but want to clean it up if
       possible, so... */
#if 1
    /*
      Disable this and use -W to see more cleanup in action.  This
      ref/unref combo ensures that if it's a temporary, it gets
      cleaned up by this unref, but if it's not, then we effectively
      do nothing to it (because someone else is holding/containing
      it).
    */
    cwal_refunref(*rv);
#endif
    *rv = 0;
  }
  s2_engine_err_reset2(se);
  s2_engine_sweep(se);
  return rc;
}

static int s2_cb_s2sh_exit(cwal_callback_args const * args, cwal_value ** rv){
  if(args){/*avoid unused param warning*/}
  App.shellExitFlag = 1;
  *rv = cwal_value_undefined();
  return 0;
}

static int s2_install_shell_api( s2_engine * se, cwal_value * tgt,
                                 char const * name ){
  cwal_value * sub = 0;
  int rc = 0;
  if(!se || !tgt) return CWAL_RC_MISUSE;
  else if(!cwal_props_can(tgt)) return CWAL_RC_TYPE;

  if(name && *name){
    sub = cwal_new_object_value(se->e);
    if(!sub) return CWAL_RC_OOM;
    if( (rc = cwal_prop_set(tgt, name, cwal_strlen(name), sub)) ){
      cwal_value_unref(sub);
      return rc;
    }
  }else{
    sub = tgt;
  }

  {
    s2_func_def const funcs[] = {
    /* S2_FUNC2("experiment", s2_cb_container_experiment), */
    S2_FUNC2("tokenizeLine", s2_cb_tokenize_line),
    S2_FUNC2("readLine", s2_cb_s2sh_line_read),
    S2_FUNC2("historySave", s2_cb_s2sh_history_save),
    S2_FUNC2("historyLoad", s2_cb_s2sh_history_load),
    S2_FUNC2("historyAdd", s2_cb_s2sh_history_add),
    S2_FUNC2("exit", s2_cb_s2sh_exit),
    s2_func_def_empty_m
    };
    rc = s2_install_functions(se, sub, funcs, 0);
  }
#undef FUNC2
#undef SET
  return 0;
}

static int s2sh_run_file(s2_engine * se, char const * filename){
  cwal_value * xrv = 0;
  int rc = s2_eval_filename(se, 1, filename, -1, &xrv);
  return s2sh_report_result(se, rc, &xrv);
}

#if 0
cwal_value * s2_shell_global(){
  return App.s2Global;
}
#endif

/**
   Given a module name (via *name) and target object, this function
   creates any parent objects of that name, if needed. Parents are
   specified by using a underscore-delimited name. If the name
   contains a delimiter, each entry except that last one is treated as
   a parent, and they are created (if needed) recursively.

   On success:

   - *name is set to point to the tail part of the module name.
   e.g. the "foo" in "foo" and "bar" in "foo_bar".

   - *newTgt is set to new target container for the tail part of the
   module name (which this routine does not install - that's up to the
   caller).

   - 0 is returned.

   The only errors are allocation-related, so a non-0 return should be
   considered fatal.
*/
static int s2_shell_install_mod_parents( s2_engine * se,
                                         char const * separators,
                                         char const ** name,
                                         cwal_value * tgt,
                                         cwal_value **newTgt){
  int rc = 0;
  s2_path_toker pt = s2_path_toker_empty;
  char const * token = 0;
  int count = 0, i;
  cwal_size_t nT = 0, nName = cwal_strlen(*name);
  s2_path_toker_init(&pt, *name, (cwal_int_t)nName);
  pt.separators = separators;
  while(0==s2_path_toker_next(&pt, &token, &nT)){
    ++count;
  }
  assert(count>0);
  if(count>1){
    token = 0;
    nT = 0;
    s2_path_toker_init(&pt, *name, (cwal_int_t)nName);
    pt.separators = separators;
  }
  for(i = 0; i < count-1; ++i ){
    cwal_value * sub = 0;
    s2_path_toker_next(&pt, &token, &nT);
    sub = cwal_prop_get(tgt, token, nT);
    if(!sub){
      sub = cwal_new_object_value(se->e);
      if(!sub) return CWAL_RC_OOM;
      cwal_value_ref(sub);
      rc = cwal_prop_set(tgt, token, nT, sub);
      cwal_value_unref(sub);
      if(rc) return rc;
    }
    tgt = sub;
  }
  if(i==count-1){
    s2_path_toker_next(&pt, &token, &nT);
  }
  *name = token;
  *newTgt = tgt;
  return 0;
}

/**
   Internal helper for s2_setup_static_modules(). s2_module_init()'s
   the given module then sets its result (or wrapper object, for v1
   modules) to tgt[name]. Returns the result of the init call.

   If installName is not NULL and not the same as the given module
   name then it is assumed to be a dot-delimited list of properties
   (with 0 or more dots) as which the module should be installed
   in/under the given target. e.g. a name of "fooBarBaz" and
   installName of "foo.bar.baz" would install module fooBarBaz as
   tgt.foo.bar.baz. If the names are the same, or installName is NULL,
   the module is installed as tgt[name].

   As a compatibility crutch, if the installName and name differ, and
   installName installs sub-properties, this routine *also* installs
   the module as s2[name] for compatibilty with the numerous scripts
   which assuming a conventional module mapping of s2.moduleName.
*/
static int s2_shell_mod_init( s2_engine * se,
                              s2_loadable_module const * const mod,
                              char const * name,
                              char const * installName,
                              cwal_value * tgt){
  cwal_value * mrv = 0;
  int rc = 0;
  char const * originalName;
  char const * propName;
  char const * initLabel = "initialization";
#if 1 == S2SH_VERSION
  /* See comments below */
  cwal_value * const originalTgt = tgt;
#endif
  if(!name) name = mod->name;
  if(!installName) installName=name;
  originalName = name;
  propName = installName;
  if(strcmp(propName,name)){
    rc = s2_shell_install_mod_parents(se, ".", &propName, tgt, &tgt);
  }
  if(!rc) rc = s2_module_init(se, mod, &mrv);
  cwal_value_ref(mrv);
#if 1 == S2SH_VERSION
  top:
#endif
  if(!rc){
    rc = s2_set( se, tgt, propName, cwal_strlen(propName),
                 mrv ? mrv : cwal_value_undefined() );
  }else{
    assert(!mrv && "Module init must not return non-NULL on error.");
  }
  VERBOSE(2,("static module [%s%s%s] %s: %s\n",
             originalName,
             propName==installName ? "" : " ==> ",
             propName==installName ? "" : installName,
             initLabel,
             rc ? "FAILED" : "OK"));
#if 1 == S2SH_VERSION
  /**
     2020-01-31: kludge/crutch: we have many scripts which expect
     modules to be installed as s2[moduleName], but this code now
     supports installing modules as sub-properties, e.g. s2.regex.js
     instead of s2.regex_js. To keep the various scripts working, this
     check will install the module to its historical s2[moduleName]
     location if that was not where the first run-through of this
     function installed it.
  */
  if(!rc && originalTgt!=tgt){
    initLabel = "alias";
    installName = propName = name;
    tgt = originalTgt;
    goto top;
  }
#endif
  cwal_value_unref(mrv);
  return rc;
}

/**
   This holds a list of all module names which are being initialized
   via the "static module" process. We use this to give us a way to
   disable activation of built-in APIs which are being installed via
   their static module form.

   The names used here must be the canonical name of the module, not
   the "installation name". e.g. "regex_js" instead of "regex.js".
*/
static char const * const AppStaticModNames[] = {
#if defined(S2SH_MODULE_INITS)
#  define M(X) # X,
#  define M2(X,Y) " " # X,
S2SH_MODULE_INITS
#  undef M
#  undef M2
#endif
0 /* list sentinel */
};

/**
   A single-string form of AppModNames. It's exists only for
   showing the user in verbose mode.
*/
static char const * const AppStaticModNamesString = ""
#if defined(S2SH_MODULE_INITS)
#define M(X) " " # X
#define M2(X,Y) " " # Y
  S2SH_MODULE_INITS
#undef M
#undef M2
#endif
  ;

/**
   Returns true (non-0) if a static module with the name n was
   compiled in, else false (0).
*/
static int s2sh_has_static_mod( char const * n ){
  char const * const * p = AppStaticModNames;
  for( ; *p; ++p ){
    if(!strcmp(n,*p)) return 1;
  }
  return 0;
}
  
/**
   Initializes any static modules which this build knows about. Each
   one gets installed as tgt[X], where X is the module's name.
   Returns 0 on success. If App.initStaticModules is 0 then this
   is a no-op.
*/
static int s2_setup_static_modules( s2_engine * se, cwal_value * tgt ){
  int rc = 0;
  if(!App.initStaticModules) return 0;
  if(0){
    s2_shell_mod_init(0,0,0,0,tgt)
      /* We need a ref to this func in order for the func
         to be static, and there is otherwise no ref when building
         without statically-imported modules. */;      
  }
  /**
     Initializes the statically-linked module assocatiated with
     S2SH_MODULE_INIT_##MODNAME, installing it under the property
     named #INSTNAME.

     INSTNAME should either be the same as MODNAME or a
     "reformulation" which tells s2sh to install it under a different
     name, possibly somewhere other than the root of s2. e.g. passing
     (foo,bar.foo) would set up the installation of module foo as
     s2.bar.foo. INSTNAME must be a non-string token (this macro
     stringifies it).
  */
#define S2SH_MODULE_INIT2(MODNAME,INSTNAME)                             \
  {                                                                     \
    extern s2_loadable_module const * s2_module_ ## MODNAME;            \
    rc = s2_shell_mod_init(se, s2_module_ ## MODNAME, #MODNAME, #INSTNAME, tgt); \
    if(rc) goto end;                                                    \
  }
#define S2SH_MODULE_INIT(MODNAME)               \
  S2SH_MODULE_INIT2(MODNAME,MODNAME)

#if defined(S2SH_MODULE_INITS)
  /**
     Yet another approach to registering modules...

     Define S2SH_MODULE_INITS (note the trailing 'S') to a list
     in this form:

     M(module1) M(module2) ... M(moduleN)

     e.g.:

     cc -c shell.c ... '-DS2SH_MODULE_INITS=M(cgi) M(sqlite3)'

     Each M(X) will get translated to S2SH_MODULE_INIT(X), setting
     up registration of that module.

     Likewise, M2(X,Y) will be transformed the same way except that Y
     (which must not a quoted string (because we apparently can't(?)
     squeeze the required quotes through Make)) will be used for the
     property name, and a dot-delimited Y will cause the module to be
     installed in a sub-property of s2. e.g. M2(X,a.b.c) will install
     module X as s2.a.b.c, rather than s2.X.

     It is up to the user to link in the appropriate objects/libs for
     each module, as well as provide any linker flags or 3rd-party
     libraries those require.
  */
#  define M S2SH_MODULE_INIT
#  define M2 S2SH_MODULE_INIT2
  S2SH_MODULE_INITS
#  undef M
#  undef M2
#endif

#undef S2SH_MODULE_INIT
#undef S2SH_MODULE_INIT2
  
    if(rc) goto end /* avoid unused goto when no modules are built in */;
  end:
  if(rc){
    cwal_value * dummy = 0;
    s2sh_report_result(se, rc, &dummy)
      /* We do this to report any pending exception
         or se->err state. */;
  }
  return rc;
}

#if 0
static int s2_cb_dump_val(cwal_callback_args const * args, cwal_value ** rv){
  int i = 0;
  for( ; i < args->argc; ++i ){
    s2_dump_val(args->argv[i], "dump_val");
  }
  *rv = cwal_value_undefined();
  return 0;
}
#endif

static int s2sh_setup_s2_global( s2_engine * se ){
  int rc = 0;
  cwal_value * v = 0;
  cwal_engine * const e = se->e;
  cwal_value * s2 = 0;
#define VCHECK if(!v){rc = CWAL_RC_OOM; goto end;} (void)0
#define RC if(rc) goto end

  /*
    Reminder to self: we're not "leaking" anything on error here - it
    will be cleaned up by the scope when it pops during cleanup right
    after the error code is returned.
  */

  v = cwal_new_function_value( e, s2_cb_print, 0, 0, 0 );
  VCHECK;
  /*s2_dump_val(App.s2Global, "App.s2Global");*/
  cwal_value_ref(v);
#if 0
  if(2==S2SH_VERSION){
    rc = s2_define_ukwd(se, "if", 2, v);
    assert(CWAL_RC_ACCESS==rc);
    rc = s2_define_ukwd(se, "print", 5, v);
    RC;
    rc = s2_define_ukwd(se, "print", 5, v);
    assert(CWAL_RC_ALREADY_EXISTS==rc);
    rc = 0;
  }else
#endif
  {
    rc = cwal_var_decl(e, 0, "print", 5, v, 0);
    RC;
  }
  cwal_value_unref(v);

  /* Global s2 object... */
  s2 = v = cwal_new_object_value(e);
  VCHECK;
  cwal_value_ref(v);
  rc = cwal_var_decl(e, 0, "s2", 2, v, CWAL_VAR_F_CONST);
  if(!rc && 2==S2SH_VERSION){
    rc = s2_define_ukwd(se, "S2", 2, v);
  }
  cwal_value_unref(v);
  RC;
  App.s2Global = v;

#define SETV(K)                                     \
  VCHECK; cwal_value_ref(v);                        \
  rc = cwal_prop_set( s2, K, cwal_strlen(K), v );   \
  cwal_value_unref(v);                              \
  RC;

  v = s2_prototype_buffer(se);
  SETV("Buffer");
  v = s2_prototype_hash(se);
  SETV("Hash");

  v = s2_prototype_tuple(se);
  SETV("Tuple");
#undef SETV
    
  rc = s2_install_pf(se, s2);
  RC;

  if(AppStaticModNames[0]){
    VERBOSE(2,("Built-in static module list:%s\n",
               AppStaticModNamesString));
  }
  
  {
    /* Global functions must come before static module init because
       script-implemented modules often affirm/assert that a certain
       API is in place. */
    s2_func_def const funcs[] = {
    /*S2_FUNC2("newUnique", s2_cb_new_unique),*/
    /*S2_FUNC2("getResultCodeHash", s2_cb_rc_hash),*/
    /*S2_FUNC2("isCallable", s2_cb_is_callable),*/
    /*S2_FUNC2("isDerefable", s2_cb_is_derefable),*/
    S2_FUNC2("sealObject", s2_cb_seal_object),
    S2_FUNC2("loadModule", s2_cb_module_load),
    S2_FUNC2("minifyScript", s2_cb_minify_script),
#if 1==S2SH_VERSION
    /* s2.import is no longer needed but we very likely have scripts
       which still expect it. */
    S2_FUNC2("import", s2_cb_import_script),
#endif
    S2_FUNC2("glob", s2_cb_glob_matches_str),
    S2_FUNC2("getenv", s2_cb_getenv),
    /*S2_FUNC2("fork", s2_cb_fork),*/
    /*S2_FUNC2("dumpVal", s2sh_cb_dump_val),*/
    S2_FUNC2("dumpMetrics", s2sh_cb_dump_metrics),
    /*S2_FUNC2("compare", s2_cb_value_compare),*/
    S2_FUNC2("cwalBuildInfo", cwal_callback_f_build_info),
    s2_func_def_empty_m /* end-of-list sentinel */
    };
    if( (rc = s2_install_functions(se, s2,
                                   funcs, CWAL_VAR_F_CONST)) ) {
      goto end;
    }
  }

  /*
    Jump through some hoops to ensure that we don't install a built-in
    API which is getting statically linked in as a loadable module...

    For a module/API named N, if AppModNames contains N, we skip
    installation of that API because its module registration (which
    installs the same API) is pending. If AppModNames has no matching
    entry, we install the API with F(se, s2, N).
  */
#define MCHECK(N,F)                                             \
  if(!s2sh_has_static_mod(N)){                                  \
    if((rc = F(se, s2, N))) goto end;                           \
  }else{                                                        \
    VERBOSE(2,("Skipping install of [%s] API b/c "              \
               "there's a module with the same name.\n", N));   \
  }
  MCHECK("fs", s2_install_fs);
  MCHECK("io", s2_install_io);
  MCHECK("json",s2_install_json);
  MCHECK("ob", s2_install_ob_2);
  MCHECK("time",s2_install_time);
#undef MCHECK
#define MCHECK(N,F)                                                     \
  if(!s2sh_has_static_mod(N)){                                          \
    if((rc = s2_install_callback(se, s2, F, N, -1,                      \
                                 CWAL_VAR_F_CONST, 0, 0, 0))) goto end; \
  }else{                                                                \
    VERBOSE(2,("Skipping install of [%s] API b/c "                      \
               "there's a module with the same name.\n", N));           \
  }
  MCHECK("tmpl", s2_cb_tmpl_to_code);
#undef MCHECK

  if(App.enableInternalFu){
    rc = s2_install_callback(se, s2, s2_cb_internal_experiment,
                             "s2InternalExperiment", -1,
                             CWAL_VAR_F_CONST, 0, 0, 0);
    if(rc) goto end;
  }

  /* Some script-implemented modules rely on s2.ARGV for configuration,
     so it must be set up before modules are initialized. */
  v = cwal_new_array_value(e);
  cwal_value_ref(v);
  rc = cwal_parse_argv_flags(e, App.scriptArgc, App.scriptArgv, &v);
  if(rc){
    cwal_value_unref(v);
    goto end;
  }else{
    assert(v);
    rc = cwal_prop_set(s2, "ARGV", 4, v);
  }
  cwal_value_unref(v);
  v = 0;
  if(rc){
    goto end;
  }
  assert(!rc);

  if(1){
    rc = s2_setup_static_modules(se, s2);
    if(rc) goto end;
  }else{
      cwal_value * mod = cwal_new_object_value(se->e);
      if(!mod){
        rc = CWAL_RC_OOM;
        goto end;
      }
      cwal_value_ref(mod);
      rc = s2_set(se, s2, "mod", 3, mod);
      cwal_value_unref(mod);
      if(!rc) rc = s2_setup_static_modules(se, mod);
      if(rc) goto end;
  }

#if 0
  /*
    EXPERIMENTAL: i've always avoided making scope-owned variable
    storage directly visible to script code, but we're going to
    try this...

    Add global const __GLOBAL which refers directly to the variable
    storage of the top-most scope.

    If we like this, we'll change it to a keyword, which will make it
    many times faster to process.
  */
  v = cwal_scope_properties(cwal_scope_top(cwal_scope_current_get(e)))
    /* Cannot fail: we've already declared vars in this scope, so it's
       already allocated. */;
  assert(cwal_value_get_object(v));
  assert(cwal_value_refcount(v)>0 && "Because its owning scope holds a ref.");
  assert(0==cwal_value_prototype_get(e, v) && "Because cwal does not install prototypes for these!");
  cwal_value_prototype_set( v, s2_prototype_object(se) );
  if(cwal_value_is_hash(v)){
    s2_hash_dot_like_object(v, 1);
  }
  rc = cwal_var_decl(e, 0, "__GLOBAL", 8, v, CWAL_VAR_F_CONST)
    /* that effectively does: global.__GLOBAL = global */;
  RC;
#endif

#if 0
  { /* testing s2_enum_builder... */
    s2_enum_builder eb = s2_enum_builder_empty;
    cwal_value * v;
    char nameBuf[20] = {'e','n','t','r','y',0,0};
    cwal_int_t i;
    rc = s2_enum_builder_init(se, &eb, 17, "TestEnum");
    assert(!rc);
    RC;
    assert(cwal_value_is_vacuum_proof(eb.entries));
    for(i = 0; i < 3; ++i ){
      nameBuf[5] = '1' + i;
      v = cwal_new_integer(se->e, (i+1));
      VCHECK;
      cwal_value_ref(v);
      rc = s2_enum_builder_append(&eb, nameBuf, v);
      cwal_value_unref(v);
      v = 0;
    }
    rc = s2_enum_builder_seal(&eb, &v);
    assert(!rc);
    RC;
    assert(!eb.se);
    assert(!eb.entries);
    assert(v);
    assert(!cwal_value_is_vacuum_proof(v));
    assert(!cwal_value_refcount(v));
    cwal_value_ref(v);
    rc = cwal_var_decl(e, 0, "testEnum", 8, v, 0);
    cwal_value_unref(v);
    RC;
    MARKER(("testEnum (s2_enum_builder) installed! Try:\n"
            "foreach(testEnum=>k,v) print(k,v,v.value,testEnum[v])\n"));
  }
#endif
  
#undef FUNC2
#undef VCHECK
#undef RC
  end:
  return rc;
}

/**
   Callback for cliapp_repl().
*/
static int CliApp_repl_f_s2sh(char const * line, void * state){
  static cwal_hash_t prevHash = 0;
  cwal_size_t const nLine = cwal_strlen(line);
  if(nLine){
    s2_engine * const se = (s2_engine*)(state);
    cwal_value * v = 0;
    cwal_hash_t const lineHash = cwal_hash_bytes(line, nLine);
    int rc = 0;
    if(prevHash!=lineHash){
      /* Only add the line to the history if it seems to be different
         from the previous line. */
      cliapp_lineedit_add(line);
      prevHash = lineHash;
    }
    s2_set_interrupt_handlable( se );
    rc = s2_eval_cstr( se, 0, "shell input", line, (int)nLine, &v );
    s2_set_interrupt_handlable( 0 )
      /* keeps our ctrl-c handler from interrupting line reading or
         accidentially injecting an error state into se. */;
    s2sh_report_result(se, rc, &v);
    s2_engine_err_reset(se);
    if(App.shellExitFlag) return S2_RC_CLIENT_BEGIN;
  }
  return 0;
}

/**
   Enters an interactive-mode REPL loop if it can, otherwise it
   complains loudly about the lack of interactive editing and returns
   CWAL_RC_UNSUPPORTED.
*/
static int s2sh_interactive(s2_engine * se){
  int rc = 0;
  if(!cliApp.lineread.enabled){
      rc = s2_engine_err_set(se, CWAL_RC_UNSUPPORTED,
                             "Interactive mode not supported: "
                             "this shell was built without "
                             "line editing support.");
      s2sh_report_result(se, rc, 0);
      return rc;
  }

  cliapp_lineedit_load(NULL) /* ignore errors */;
  cwal_outputf(se->e, "s2 interactive shell. All commands "
               "are run in the current scope. Use your platform's EOF "
               "sequence on an empty line to exit. (Ctrl-D on Unix, "
               "Ctrl-Z(?) on Windows.) Ctrl-C might work, too.\n\n");
  rc = cliapp_repl( CliApp_repl_f_s2sh, &App.lnPrompt, 0, se );
  if(S2_RC_CLIENT_BEGIN==rc){
    /* Exit signal from the repl callback. */
    rc = 0;
  }
  cliapp_lineedit_save(NULL);
  s2_engine_sweep(se);
  cwal_output(se->e, "\n", 1);
  cwal_output_flush(se->e);
  return rc;
}


static int s2sh_main2( s2_engine * se ){
  int rc = 0;
  int i;
#if 0
  /* just testing post-init reconfig */
  {
    /* Configure the memory chunk recycler... */
    cwal_memchunk_config conf = cwal_memchunk_config_empty;
    conf.maxChunkCount = 0;
    conf.maxChunkSize = 1024 * 32;
    conf.maxTotalSize = 1024 * 256;
    conf.useForValues = 0;
    rc = cwal_engine_memchunk_config(se->e, &conf);
    assert(!rc);
  }
#endif

  if(!App.cleanroom){
    if(App.installShellApi>0
       || (App.interactive>0 && App.installShellApi<0)){
      assert(App.s2Global);
      if((rc = s2_install_shell_api(se, App.s2Global, "shell"))){
        return rc;
      }
    }
  }

  assert(App.scripts.nScripts <= S2SH_MAX_SCRIPTS);
  for( i = 0; i < App.scripts.nScripts; ++i ){
    char const * script = App.scripts.e[i].src;
    /*VERBOSE(2,("Running script #%d: %s\n", i+1, script));*/
    if(!*script) continue;
    if(App.scripts.e[i].isFile){
      rc = s2sh_run_file(se, script);
      cwal_output_flush(se->e);
      /* s2_engine_sweep(se); */
      if(rc) break;
    }else{
      cwal_value * rv = 0;
      rc = s2_eval_cstr( se, 0, "-e script",
                         script, cwal_strlen(script),
                         App.verbosity ? &rv : NULL );
      s2sh_report_result(se, rc, &rv);
      if(rc) break;
    }
  }
  /*MARKER(("Ran %d script(s)\n", i));*/
  if(!rc && App.interactive>0){
    rc = s2sh_interactive(se);
  }
  return rc;
}

/**
   If App.enableArg0Autoload and a file named $S2SH_INIT_SCRIPT or
   (failing that) the same as App.appName, minus any ".exe" or ".EXE"
   extension, but with an ".s2" extension, that script is loaded. If
   not found, 0 is returned. If loading or evaling the (existing) file
   fails, non-0 is returned and the error is reported.
*/
static int s2sh_autoload(s2_engine * se){
  int rc = 0;
  char const * scriptName = 0;
  if(!App.enableArg0Autoload || !App.appName || !*App.appName) return 0;
  else if(S2_DISABLE_FS_READ & s2_disable_get(se)){
    VERBOSE(1,("NOT loading init script because the --s2-disable=fs-read "
               "flag is active.\n"));
    return 0;
  }
  else if( !(scriptName = getenv("S2SH_INIT_SCRIPT")) ){
    /*
      See if $0.s2 exists...

      FIXME: this doesn't work reliably if $0 is relative, e.g.
      found via $PATH. We need to resolve its path to do this
      properly, but realpath(3) is platform-specific.
    */
    enum { BufSize = 1024 * 3 };
    static char fn[BufSize] = {0};
    cwal_size_t slen = 0;
    char const * exe = 0;
    char const * arg0 = App.appName;
    assert(arg0);
    exe = strstr(arg0, ".exe");
    if(!exe) exe = strstr(arg0, ".EXE");
    if(!exe) exe = strstr(arg0, ".Exe");
    slen = cwal_strlen(arg0) - (exe ? 4 : 0);
    rc = sprintf(fn, "%.*s.s2", (int)slen, arg0);
    assert(rc<BufSize-1);
    rc = 0;
    scriptName = fn;
  }
  if(s2_file_is_accessible(scriptName, 0)){
    if(App.cleanroom){
      VERBOSE(1,("Clean-room mode: NOT auto-loading script [%s].\n", scriptName));
    }else{
      cwal_value * rv = 0;
      VERBOSE(2,("Auto-loading script [%s].\n", scriptName));
      rc = s2_eval_filename(se, 1, scriptName, -1, &rv);
      if(rc){
        MESSAGE(("Error auto-loading init script [%s]: "
                 "error #%d (%s).\n", scriptName,
                 rc, cwal_rc_cstr(rc)));
        s2sh_report_result(se, rc, &rv);
      }
      else{
        cwal_refunref(rv);
      }
    }
  }
  return rc;
}

int s2sh_main(int argc, char const * const * argv){
  enum {
  /* If either of these is 0 then the corresponding engine gets
     allocated dynamically. In practice there's generally no reason to
     prefer dynamic allocation over stack allocation. */
  UseStackCwalEngine = 1 /* cwal_engine */,
  UseStackS2Engine = 1 /* s2_engine */
  };
  cwal_engine E = cwal_engine_empty;
  cwal_engine * e = UseStackCwalEngine ? &E : 0;
  cwal_engine_vtab vtab = cwal_engine_vtab_basic;
  s2_engine SE = s2_engine_empty;
  s2_engine * se = UseStackS2Engine ? &SE : 0;
  int rc = 0;

  if(argc || argv){/*possibly unused var*/}
  setlocale(LC_CTYPE,"") /* supposedly important for the JSON-parsing bits? */;

  vtab.tracer = cwal_engine_tracer_FILE;
  vtab.tracer.state = stdout;
  vtab.hook.on_init = s2sh_init_engine;
  vtab.interning.is_internable = cstr_is_internable;
  vtab.outputer.state.data = stdout;
  assert(cwal_finalizer_f_fclose == vtab.outputer.state.finalize);

  if(App.outFile){
    FILE * of = (0==strcmp("-",App.outFile))
      ? stdout
      : fopen(App.outFile,"wb");
    if(!of){
      rc = CWAL_RC_IO;
      MARKER(("Could not open output file [%s].\n", App.outFile));
      goto end;
    }
    else {
      vtab.outputer.state.data = of;
      vtab.outputer.state.finalize = cwal_finalizer_f_fclose;
      vtab.tracer.state = of;
      vtab.tracer.close = NULL
        /* Reminder: we won't want both outputer and tracer
           to close 'of'.
        */;
    }
  }

  vtab.memcap = App.memcap;

  rc = cwal_engine_init( &e, &vtab );
  if(rc){
    cwal_engine_destroy(e);
    goto end;
  }

  se = UseStackS2Engine ? &SE : s2_engine_alloc(e);
  assert(se);
  assert(UseStackS2Engine ? !se->allocStamp : (se->allocStamp == e));

  if((rc = s2_engine_init( se, e ))) goto end;
  if(App.cleanroom){
    VERBOSE(1,("Clean-room mode: NOT installing globals/prototypes.\n"));
  }else{
    if((rc = s2_install_core_prototypes(se))) goto end;
    else if( (rc = s2sh_setup_s2_global(se)) ) goto end;
  }
  if(App.zDisableFlags){
    rc = s2_disable_set_cstr(se, App.zDisableFlags, -1, 0);
    if(rc) goto end;
  }
  se->sweepInterval += App.addSweepInterval;
  se->vacuumInterval += App.addVacuumInterval;
  se->flags.traceTokenStack = App.traceStacks;
  se->flags.traceAssertions = App.traceAssertions;
  se->flags.traceSweeps = App.traceSweeps;
  s2_set_interrupt_handlable( se )
    /* after infrastructure, before the auto-loaded script, in case it
       contains an endless loop or some such and needs to be
       interrupted.
    */;
#if defined(S2_SHELL_EXTEND)
  if(!App.cleanroom){
    if( (rc = S2_SHELL_EXTEND_FUNC_NAME(se, App.s2Global, argc, argv)) ) goto end;
  }else{
    VERBOSE(1,("Clean-room mode: NOT running client-side shell extensions.\n"));
  }
#endif

  if(App.cleanroom){
    VERBOSE(1,("Clean-room mode: NOT auto-loading init script.\n"));
  }else{
    rc=s2sh_autoload(se);
  }
  s2_set_interrupt_handlable( 0 );
  if(rc) goto end;
       
  rc = s2sh_main2(se);
  App.s2Global = 0;

  end:
  s2_set_interrupt_handlable( 0 );
  if(App.showMetrics){
    s2_dump_metrics(se);
  }
#if 1
  s2sh_report_engine_error(se);
#endif
  assert(!se->st.vals.size);
  assert(!se->st.ops.size);
  s2_engine_finalize(se);
  return rc;
}


/**
   Pushes scr to App.scripts if there is space for it, else outputs an
   error message and returns non-0. isFile must be true if scr refers
   to a filename, else it is assumed to hold s2 script code.
*/
static int s2sh_push_script( char const * scr, int isFile ){
  const int n = App.scripts.nScripts;
  if(S2SH_MAX_SCRIPTS==n){
    MARKER(("S2SH_MAX_SCRIPTS (%d) exceeded: give fewer scripts or "
            "rebuild with a higher S2SH_MAX_SCRIPTS.\n",
            S2SH_MAX_SCRIPTS));
    return CWAL_RC_RANGE;
  }
  App.scripts.e[n].isFile = isFile;
  App.scripts.e[n].src = scr;
  VERBOSE(3,("Pushed %s #%d: %s\n", isFile ? "file" : "-e script",
             n+1, App.scripts.e[n].src));
  ++App.scripts.nScripts;
  return 0;
}

static void s2sh_help_header_once(){
  static int once = 0;
  if(once) return;
  once = 1;
  cliapp_print("s2 scripting engine shell #%d\n",
               S2SH_VERSION);
}

static void s2sh_show_version(int detailedMode){
  s2sh_help_header_once();
  cliapp_print("cwal/s2 version: %s\n",
               cwal_build_info()->versionString);
  cliapp_print("Project site: https://fossil.wanderinghorse.net/r/cwal\n");
  if(detailedMode){
    cwal_build_info_t const * const bi = cwal_build_info();
    cliapp_print("\nlibcwal compile-time info:\n\n");
#define STR(MEM) cliapp_print("\t%s = %s\n", #MEM, bi->MEM)
    /*STR(versionString);*/
    STR(cppFlags);
    STR(cFlags);
    STR(cxxFlags);
#undef STR

#define SZ(MEM) cliapp_print("\t%s = %"CWAL_SIZE_T_PFMT"\n", #MEM, (cwal_size_t)bi->MEM)
    SZ(size_t_bits);
    SZ(int_t_bits);
    SZ(maxStringLength);
#undef SZ

#define BUL(MEM) cliapp_print("\t%s = %s\n", #MEM, bi->MEM ? "true" : "false")
    BUL(isJsonParserEnabled);
    BUL(isDebug);
#undef BUL

    cliapp_print("\n");
  }
}

/**
   Parses and returns the value of an unsigned int CLI switch. Assigns
   *rc to 0 on success, non-0 on error.
*/
static cwal_size_t s2sh_parse_switch_unsigned(CliAppSwitch const * s, int * rc){
  cwal_int_t i = 0;
  if(!s2_cstr_parse_int(s->value, -1, &i)){
    WARN(("-cap-%s must have a positive integer value.\n", s->key));
    *rc = CWAL_RC_MISUSE;
    return 0;
  }else if(i<0){
    WARN(("-cap-%s must have a positive value (got %d).\n", s->key, (int)i));
    *rc = CWAL_RC_RANGE;
    return 0;
  }else{
    *rc = 0;
    return (cwal_size_t)i;
  }
}


/**
   The help handler for a single CliAppSwitch. It requires that all
   switches are passed to it in the order they are defined at the app
   level, as it applies non-inuitive semantics to certain entries,
   e.g. collapsing -short and --long-form switches into a single
   entry.
*/
static int s2sh_help_switch_visitor(CliAppSwitch const *s,
                                    void * /* ==>int*  */ verbosity){
  static CliAppSwitch const * skip = 0;
  CliAppSwitch const * next = s+1
    /* this is safe b/c the list ends with an empty sentinel entry */;
  int group = 0;
  static int prevContinue = 0;
  if(s->opaque & SW_UNDOCUMENTED){
    return 0;
  }else if(skip == s){
      skip = 0;
      return 0;
  }
  switch(s->opaque & SW_GROUPS){
    case SW_GROUP_0: break;
    case SW_GROUP_1: group = 1; break;
    case SW_GROUP_2: group = 2; break;
  }
  assert(s->dash>=-1 && s->dash<=2);
  assert(group>=0 && group<3);
  /*MARKER(("group %d (%d) %s\n", group, *((int const*)verbosity), s->key));*/
  if(group>*((int const*)verbosity)){
      return CWAL_RC_BREAK;
  }
  if(s->opaque & SW_GROUPS){
    /* Use s->brief as a label for this section then return. */
    if(s->brief && *s->brief){
      char const * splitter = "===============================";
      cliapp_print("\n%s\n%s\n%s\n", splitter, s->brief, splitter);
    }
    return 0;
  }
  if((s->opaque & SW_ID_MASK) && (next->opaque==s->opaque)){
    /* Collapse long/short options into one entry. We expect the long
       entry to be the first one. */
    /* FIXME? This only handles collapsing 2 options, but s2sh
       help supports -?/-help/--help. */
      skip = next;
      cliapp_print("\n  %s%s | ", cliapp_flag_prefix(skip->dash),
                   skip->key);
  }else if(!next->key){
      skip = 0;
  }
  /*MARKER(("switch entry: brief=%s, details=%s\n", s->brief, s->details));*/
  if(s->key && *s->key){
    cliapp_print("%s%s%s%s%s%s%s\n",
                 skip ? "" : "\n  ",
                 cliapp_flag_prefix(s->dash), s->key,
                 s->value ? " = " : "",
                 s->value ? s->value : "",
                 s->brief ? "\n      ": "",
                 s->brief ? s->brief : "");
    prevContinue = s->opaque;
  }else{
    switch(s->opaque){
      case SW_CONTINUE:
        assert(s->brief && *s->brief);
        if(prevContinue==SW_INFOLINE){
          cliapp_print("%s\n", s->brief);
        }else{
          cliapp_print("      %s\n", s->brief);
        }
        break;
      case SW_INFOLINE:
        assert(s->brief && *s->brief);
        cliapp_print("\n%s\n", s->brief);
        prevContinue = s->opaque;
        break;
      default:
        prevContinue = s->opaque;
        break;
    }
  }
  return 0;
}

static void s2sh_show_help(){
  int maxGroup = App.verbosity;
  char const * splitter = 1 ? "" :
    "------------------------------------------------------------\n";
  int n = 0;
  s2sh_show_version(0);
  cliapp_print("\nUsage: %s [options] [file | -f file]\n",
               App.appName);
  cliapp_print("%s",splitter);
  cliapp_switches_visit( s2sh_help_switch_visitor, &maxGroup );
  cliapp_print("\n%sNotes:\n",splitter); 
  cliapp_print("\n%d) Flags which require a value may be provided as (--flag=value) or "
        "(--flag value).\n", ++n);
  cliapp_print("\n%d) All flags after -- are ignored by the shell but "
               "made available to script code via the s2.ARGV object.\n", ++n);
  cliapp_print("Minor achtung: the script-side arguments parser uses --flag=VALUE, "
               "whereas this app's flags accept (--flag VALUE)!\n");

  if(maxGroup<2){
      cliapp_print("\n%d) Some less obscure options were not shown. "
                   "Use -v once or twice with -? for more options.\n", ++n);
  }
  puts("");
}

static int s2shGotHelpFlag = 0;
static int s2sh_arg_callback_common(int ndx, CliAppSwitch const * s,
                                    CliAppArg * a){
  int const sFlags = s ? s->opaque : 0;
  int const sID = sFlags & SW_ID_MASK;
  int rc = 0;
  if(0 && a){
    MARKER(("switch callback sFlags=%06x, sID=%d, SW_ESCRIPT=%d: #%d %s%s%s%s\n",
            sFlags, sID, SW_ESCRIPT,
            ndx, cliapp_flag_prefix(a->dash), a->key,
            a->value ? "=" : "",
            a->value ? a->value : ""));
  }
  if(!a/* end of args - there's nothing for use to do in this case */){
    return 0;
  }
  switch(sID){
    case SW_ASSERT_0: App.traceAssertions = 0; break;
    case SW_ASSERT_1: ++App.traceAssertions; break;
    case SW_AUTOLOAD_0: App.enableArg0Autoload = 0; break;
    case SW_AUTOLOAD_1: App.enableArg0Autoload = 1; break;
    case SW_CLAMPDOWN:
      WARN(("Clampdown mode has been removed."));
      return CWAL_RC_UNSUPPORTED;
    case SW_CLEANROOM: App.cleanroom = 1; break;
    case SW_DISABLE: App.zDisableFlags = a->value; break;
    case SW_ESCRIPT:
      assert(a->value && "Should have been captured by cliapp.");
      rc = s2sh_push_script( a->value, 0 );
      if(App.interactive<0) App.interactive=0;
      break;
    case SW_HELP: ++s2shGotHelpFlag; break;
    case SW_HIST_FILE_1: cliApp.lineread.historyFile = a->value; break;
    case SW_HIST_FILE_0: cliApp.lineread.historyFile = 0; break;
    case SW_INFILE:
      assert(a->value && "Should have been captured by cliapp.");
      rc = s2sh_push_script( a->value, 1 );
      if(!rc) App.inFile = a->value;
      break;
    case SW_INTERACTIVE: App.interactive = 1; break;
    case SW_INTERNAL_FU: ++App.enableInternalFu; break;
    case SW_MEM_FAST0: App.memcap.forceAllocSizeTracking = 0; break;
    case SW_MEM_FAST1: App.memcap.forceAllocSizeTracking = 1; break;
    case SW_MCAP_TOTAL_ALLOCS:
      App.memcap.maxTotalAllocCount =
        s2sh_parse_switch_unsigned(s, &rc);
      break;
    case SW_MCAP_TOTAL_BYTES:
      App.memcap.maxTotalMem = s2sh_parse_switch_unsigned(s, &rc);
      break;
    case SW_MCAP_CONC_ALLOCS:
      App.memcap.maxConcurrentAllocCount =
        s2sh_parse_switch_unsigned(s, &rc);
      break;
    case SW_MCAP_CONC_BYTES:
      App.memcap.maxConcurrentMem = s2sh_parse_switch_unsigned(s, &rc);
      break;
    case SW_MCAP_SINGLE_BYTES:
      App.memcap.maxSingleAllocSize =
        s2sh_parse_switch_unsigned(s, &rc);
      break;
    case SW_METRICS: App.showMetrics = 1; break;
    case SW_MOD_INIT_0: App.initStaticModules = 0; break;
    case SW_MOD_INIT_1: App.initStaticModules = 1; break;
    case SW_OUTFILE: App.outFile = a->value; break;
    case SW_RE_C0: App.maxChunkCount = 0; break;
    case SW_RE_C1: App.maxChunkCount = 30; break;
    case SW_RE_S0: App.enableStringInterning = 0; break;
    case SW_RE_S1: App.enableStringInterning = 1; break;
    case SW_RE_V0: App.enableValueRecycling = 0; break;
    case SW_RE_V1: App.enableValueRecycling = 1; break;
    case SW_SCOPE_H0: App.scopesUseHashes = 0; break;
    case SW_SCOPE_H1: App.scopesUseHashes = 1; break;
    case SW_SHELL_API_0: App.installShellApi = 0; break;
    case SW_SHELL_API_1: App.installShellApi = 1; break;
    case SW_TRACE_CWAL:
      App.cwalTraceFlags = CWAL_TRACE_SCOPE_MASK
        | CWAL_TRACE_VALUE_MASK
        | CWAL_TRACE_MEM_MASK
        | CWAL_TRACE_FYI_MASK
        | CWAL_TRACE_ERROR_MASK;
      break;
    case SW_TRACE_STACKS_0: App.traceStacks = 0; break;
    case SW_TRACE_STACKS_1: ++App.traceStacks; break;
    case SW_TRACE_SWEEP_0: App.traceSweeps = 0; break;
    case SW_TRACE_SWEEP_1: ++App.traceSweeps; break;
    case SW_VERBOSE: ++App.verbosity; break;
    case SW_SHOW_SIZEOFS:
    default: break;
  }
  return rc;
}

static int dump_args_visitor(CliAppArg const * p, int i, void * state){
  if(state){/*unused*/}
  assert(p->key);
  cliapp_print("argv[%d]  %s%s%s%s\n",i,
               cliapp_flag_prefix(p->dash), p->key,
               p->value ? "=" : "",
               p->value ? p->value : "");
  return 0;
}

static void s2sh_dump_args(){
  puts("CLI arguments:");
  cliapp_args_visit(dump_args_visitor, 0, 1)
    /* Over-engineering, just to test that cliapp code. */;
}

int main(int argc, char const * const * argv){
  int rc = 0;
  char const * arg = 0;
  int gotNoOp = 0
    /* incremented when we get a "no-op" flag. These are
       flags which perform some work without invoking the
       interpreter, and this flag lets us distinguish between
       not getting any options and getting some which did
       something useful (--help, --version, etc)*/;

  s2_static_init();
  App.appName = argv[0];
  cliApp.switches = s2sh_cliapp_switches();
  cliApp.argCallack = s2sh_arg_callback_common;
  assert(!App.shellExitFlag);
  memset(&App.scripts,0,sizeof(App.scripts));
  App.memcap.forceAllocSizeTracking = 0;
  
  if((arg = getenv("S2SH_HISTORY_FILE"))){
    App.editHistoryFile = arg;
  }
  cliApp.lineread.historyFile = App.editHistoryFile;

  rc = cliapp_process_argv(argc, argv, 0);
  if(rc) rc = CWAL_RC_MISUSE
           /* It's *possibly* not a CWAL_RC_ code, to we assume misuse. */;
  if(rc) goto end;
  App.scriptArgc = cliApp.doubleDash.argc;
  App.scriptArgv = cliApp.doubleDash.argv;

  if(0){
    CliAppArg const * p;
    if(0){
      if(!rc){
        s2sh_dump_args();
        while((p=cliapp_arg_nonflag())){
          MARKER(("Non-flag arg: %s\n",p->key));
        }
        cliapp_arg_nonflag_rewind();
      }
    }
  }

  if(s2shGotHelpFlag){
    s2sh_show_help();
    gotNoOp = 1;
  }else if(cliapp_arg_flag("V","version", 0)){
    s2sh_show_version(App.verbosity);
    gotNoOp = 1;
  }else if(!App.inFile){
      /* Treat the first non-flag arg as an -f flag if no -f was
         explicitely specified. */
    CliAppArg const * ca;
    assert(argc>1 ? cliApp.cursorNonflag>0 : 1);
    ca = cliapp_arg_nonflag();
    if(ca){
      rc = s2sh_push_script( ca->key, 1 );
      if(rc) goto end;
      else App.inFile = ca->key;
    }
  }
#if 1 == S2SH_VERSION
  if(cliapp_arg_flag("z",0,0)){
    gotNoOp = 1;
#define SO(T) MESSAGE(("sizeof("#T")=%d\n", (int)sizeof(T)))
    SO(s2_stoken);
    SO(s2_ptoken);
    SO(s2_ptoker);
    /* SO(s2_op); */
    SO(s2_scope);
    SO(cwal_scope);
    SO(s2_engine);
    SO(cwal_engine);
    SO(cwal_memchunk_overlay);
    SO(cwal_list);
#undef SO
    MESSAGE(("sizeof(s2_func_state)=%d\n",
             (int)s2_sizeof_script_func_state()));
  }
#endif /* -z flag */

  if(!gotNoOp && App.inFile && cliapp_arg_nonflag()){
    WARN(("Extraneous non-flag arguments provided.\n"));
    rc = CWAL_RC_MISUSE;
    goto end;
  }

  assert(!rc);
  if(!gotNoOp){
    if(!App.inFile && App.interactive<0
       && !App.scripts.nScripts){
#ifdef S2_OS_UNIX
      if(isatty(STDIN_FILENO)){
        App.interactive = 1;
      }else App.inFile = "-";
#else
      App.inFile = "-";
#endif
    }
    rc = s2sh_main(argc, argv);
  }

  end:
  {
    char const * errMsg = cliapp_err_get();
    if(errMsg){
      WARN(("%s\n", errMsg));
    }
  }
  if(App.verbosity>0 && rc){
    WARN(("rc=%d (%s)\n", rc, cwal_rc_cstr(rc)));
  }
  return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
