/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).
*/

#include "fossil-scm/fossil-internal.h"
#include "fossil-scm/fossil-cli.h"

/* Only for debugging */
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)

/** Convenience form of FCLI_VN for level-3 verbosity. */
#define FCLI_V3(pfexp) FCLI_VN(3,pfexp)
#define fcli_empty_m {    \
  NULL/*appHelp*/,  \
  NULL/*cliFlags*/,      \
  NULL/*f*/,      \
  NULL/*argv*/,      \
  0/*argc*/,      \
  NULL/*appName*/,      \
  {/*clientFlags*/ \
    "."/*checkoutDir*/,      \
    0/*verbose*/,            \
    false/*dryRun*/ \
  }, \
  {/*transient*/      \
    NULL/*repoDb*/,      \
    NULL/*userArg*/, \
    0/*helpRequested*/                   \
  },                                                            \
  {/*config*/      \
    -1/*traceSql*/,      \
    fsl_outputer_empty_m      \
  },                          \
  fsl_error_empty_m/*err*/  \
}

const fcli_t fcli_empty = fcli_empty_m;
fcli_t fcli = fcli_empty_m;
const fcli_cliflag fcli_cliflag_empty = fcli_cliflag_empty_m;
static fsl_timer_state fcliTimer = fsl_timer_state_empty_m;

void fcli_printf(char const * fmt, ...){
  va_list args;
  va_start(args,fmt);
  if(fcli.f){
    fsl_outputfv(fcli.f, fmt, args);
  }else{
    fsl_fprintfv(stdout, fmt, args);
  }
  va_end(args);
}

/**
   Outputs app-level help. How it does this depends on the state of
   the fcli object, namely fcli.cliFlags and the verbosity
   level. Normally this is triggered automatically by the CLI flag
   handling in fcli_setup().
*/
static void fcli_help(void);

unsigned short fcli_is_verbose(void){
  return fcli.clientFlags.verbose;
}

fsl_cx * fcli_cx(void){
  return fcli.f;
}

static int fcli_open(void){
  int rc = 0;
  fsl_cx * f = fcli.f;
  assert(f);
  if(fcli.transient.repoDbArg){
    FCLI_V3(("Trying to open repo db file [%s]...\n", fcli.transient.repoDbArg));
    rc = fsl_repo_open( f, fcli.transient.repoDbArg );
  }
  else if(fcli.clientFlags.checkoutDir){
    fsl_buffer dir = fsl_buffer_empty;
    char const * dirName;
    rc = fsl_file_canonical_name(fcli.clientFlags.checkoutDir,
                                 &dir, 0);
    assert(!rc);
    dirName = (char const *)fsl_buffer_cstr(&dir);
    FCLI_V3(("Trying to open checkout from [%s]...\n",
             dirName));
    rc = fsl_ckout_open_dir(f, dirName, true);
    FCLI_V3(("checkout open rc=%s\n", fsl_rc_cstr(rc)));

    /* if(FSL_RC_NOT_FOUND==rc) rc = FSL_RC_NOT_A_CKOUT; */
    if(rc){
      if(!fsl_cx_err_get(f,NULL,NULL)){
        rc = fsl_cx_err_set(f, rc, "Opening of checkout under "
                            "[%s] failed with code %d (%s).",
                            dirName, rc, fsl_rc_cstr(rc));
      }
    }
    fsl_buffer_reserve(&dir, 0);
    if(rc) return rc;
  }
  if(!rc){
    if(fcli.clientFlags.verbose>1){
      fsl_db * dbC = fsl_cx_db_ckout(f);
      fsl_db * dbR = fsl_cx_db_repo(f);
      if(dbC){
        FCLI_V3(("Checkout DB name: %s\n", f->ckout.db.filename));
      }
      if(dbR){
        FCLI_V3(("Opened repo db: %s\n", f->repo.db.filename));
        FCLI_V3(("Repo user name: %s\n", f->repo.user));
      }
    }
#if 0
    /*
      Only(?) here for testing purposes.

       We don't really need/want to update the repo db on each
       open of the checkout db, do we? Or do we?
     */
    fsl_repo_record_filename(f) /* ignore rc - not critical */;
#endif
  }
  return rc;
}


#define fcli__error (fcli.f ? &fcli.f->error : &fcli.err)
fsl_error * fcli_error(void){
  return fcli__error;
}

void fcli_err_reset(void){
  fsl_error_reset(fcli__error);
}


static struct TempFlags {
  bool traceSql;
  bool doTimer;
} TempFlags = {
false,
false
};

static struct {
  fsl_list list;
}  FCliFree = {
fsl_list_empty_m
};

static void fcli_shutdown(void){
  fsl_cx * f = fcli.f;
  int rc = 0;
 
  fsl_error_clear(&fcli.err);
  fsl_free(fcli.argv)/*contents are in the FCliFree list*/;

  if(f){
    while(fsl_cx_transaction_level(f)){
      MARKER(("WARNING: open db transaction at shutdown-time. "
              "Rolling back.\n"));
      fsl_cx_transaction_end(f, true);
    }
    if(1 &&
       fsl_cx_db_ckout(f)){
      /* For testing/demo only: this is implicit
         when we call fsl_cx_finalize().
      */
      rc = fsl_ckout_close(f);
      FCLI_V3(("Closed checkout/repo db(s). rc=%s\n", fsl_rc_cstr(rc)));
      //assert(0==rc);
    }
  }
  fsl_list_clear(&FCliFree.list, fsl_list_v_fsl_free, 0);
  fsl_list_reserve(&FCliFree.list, 0);
  if(f){
    FCLI_V3(("Finalizing fsl_cx @%p\n", (void const *)f));
    fsl_cx_finalize( f );
  }
  fcli = fcli_empty;
  if(TempFlags.doTimer){
    double const runTime =
      ((int64_t)fsl_timer_stop(&fcliTimer)) / 1000.0;
    f_out("Total fcli run time: %f seconds of CPU time\n",
          runTime/1000);
  }
}

static struct {
  fcli_cliflag const * flags;
} FCliHelpState = {
NULL
};


static int fcli_flag_f_nocheckoutDir(fcli_cliflag const *f){
  if(f){/*unused*/}
  fcli.clientFlags.checkoutDir = 0;
  return 0;
}
static int fcli_flag_f_verbose(fcli_cliflag const *f){
  if(f){/*unused*/}
  ++fcli.clientFlags.verbose;
  return FCLI_RC_FLAG_AGAIN;
}
static int fcli_flag_f_help(fcli_cliflag const *f){
  if(f){/*unused*/}
  ++fcli.transient.helpRequested;
  return FCLI_RC_FLAG_AGAIN;
}

static const fcli_cliflag FCliFlagsGlobal[] = {
  FCLI_FLAG_BOOL_X("?","help",NULL,
                   fcli_flag_f_help,
                   "Show app help. Also triggered if the first non-flag is \"help\"."),
  FCLI_FLAG("R","repo","REPO-FILE",&fcli.transient.repoDbArg,
            "Selects a specific repository database, ignoring the one "
            "used by the current directory's checkout (if any)."),
  FCLI_FLAG_BOOL("D","dry-run",&fcli.clientFlags.dryRun,
                 "Enable dry-run mode (not supported by all apps)."),
  FCLI_FLAG("U","user","username",&fcli.transient.userArg,
            "Sets the name of the fossil user name for this session."),
  FCLI_FLAG_BOOL_X("C", "no-checkout",NULL,fcli_flag_f_nocheckoutDir,
                   "Disable automatic attempt to open checkout."),
  FCLI_FLAG(0,"checkout-dir","DIRECTORY", &fcli.clientFlags.checkoutDir,
            "Open the given directory as a checkout, instead of the current dir."),
  FCLI_FLAG_BOOL_X("V","verbose",NULL,fcli_flag_f_verbose,
              "Increases the verbosity level by 1. May be used multiple times."),
  FCLI_FLAG_BOOL(0,"trace-sql",&TempFlags.traceSql,
                 "Enable SQL tracing."),
  FCLI_FLAG_BOOL(0,"timer",&TempFlags.doTimer,
                 "At the end of successful app execution, output how long it took "
                 "from the call to fcli_setup() until the end of main()."),
  fcli_cliflag_empty_m
};

void fcli_cliflag_help(fcli_cliflag const *defs){
  fcli_cliflag const * f;
  const char * tab = "  ";
  for( f = defs; f->flagShort || f->flagLong; ++f ){
    const char * s = f->flagShort;
    const char * l = f->flagLong;
    const char * fvl = f->flagValueLabel;
    const char * valLbl = 0;
    switch(f->flagType){
      case FCLI_FLAG_TYPE_BOOL:
      case FCLI_FLAG_TYPE_BOOL_INVERT: break;
      case FCLI_FLAG_TYPE_INT32: valLbl = fvl ? fvl : "int32"; break;
      case FCLI_FLAG_TYPE_INT64: valLbl = fvl ? fvl : "int64"; break;
      case FCLI_FLAG_TYPE_ID: valLbl = fvl ? fvl : "db-record-id"; break;
      case FCLI_FLAG_TYPE_DOUBLE: valLbl = fvl ? fvl : "double"; break;
      case FCLI_FLAG_TYPE_CSTR: valLbl = fvl ? fvl : "string"; break;
      default:
        break;
    }
    f_out("%s%s%s%s%s%s%s%s",
          tab,
          s?"-":"", s?s:"", (s&&l)?"|":"",
          l?"--":"",l?l:"",
          valLbl ? "=" : "", valLbl);
    if(f->helpText){
      f_out("\n%s%s%s", tab, tab, f->helpText);
    }
    f_out("\n\n");
  }
}

void fcli_help(void){
  if(fcli.appHelp){
    if(fcli.appHelp->briefUsage){
      f_out("Usage: %s [options] %s\n", fcli.appName, fcli.appHelp->briefUsage);
    }
    if(fcli.appHelp->briefDescription){
      f_out("\n%s\n", fcli.appHelp->briefDescription);
    }
  }else{
    f_out("Help for %s:\n", fcli.appName);
  }
  const int helpCount = fcli.transient.helpRequested
    + fcli.clientFlags.verbose;
  bool const showGlobal = helpCount>1;
  bool const showApp = (2!=helpCount);
  if(showGlobal){
    f_out("\nFCli global flags:\n\n");
    fcli_cliflag_help(FCliFlagsGlobal);
  }else{
    f_out("\n");
  }
  if(showApp){
    if(FCliHelpState.flags
       && (FCliHelpState.flags[0].flagShort || FCliHelpState.flags[0].flagLong)){
      f_out("App-specific flags:\n\n");
      fcli_cliflag_help(FCliHelpState.flags);
      //f_out("\n");
    }
    if(fcli.appHelp && fcli.appHelp->callback){
      fcli.appHelp->callback();
      f_out("\n");
    }
  }
  if(showGlobal){
    if(!showApp){
      f_out("Invoke --help three times to list "
            "both the framework- and app-level options.\n");
    }else{
      f_out("Invoke --help once to list only the "
            "app-level flags.\n");
    }
  }else{
    f_out("Invoke --help twice to list the framework-level "
          "options. Use --help three times to list both "
          "framework- and app-level options.\n");
  }
  f_out("\nFlags which require values may be passed as "
        "--flag=value or --flag value.\n\n");
}

int fcli_process_flags( fcli_cliflag const * defs ) {
  fcli_cliflag const * f;
  int rc = 0;
  /**
     TODO/FIXME/NICE-TO-HAVE: we "really should" process the CLI flags
     in the order they are provided on the CLI, as opposed to the
     order they're defined in the defs array. The current approach is
     much simpler to process but keeps us from being able to support
     certain useful flag-handling options, e.g.:

     f-tag -a artifact-id-1 --tag x=y --tag y=z -a artifact-id-2 --tag a=b...

     The current approach consumes the -a flags first, leaving us
     unable to match the --tag flags to their corresponding
     (left-hand) -a flag.

     Processing them the other way around, however, requires that we
     keep track of which flags we've already seen so that we can
     reject, where appropriate, duplicate invocations.

     We could, instead of looping on the defs array, loop over the
     head of fcli.argv. If it's a non-flag, move it out of the way
     temporarily (into a new list), else look over the defs array
     looking for a flag match. We don't know, until finding such a
     match, whether the current flag requires a value. If it does, we
     then have to check the current fcli.argv entry to see if it has a
     value (--x=y) or whether the next argv entry is its value (--x
     y). If the current tip has no matching defs entry, we have no
     choice but to skip over it in the hopes that the user can use
     fcli_flag() and friends to consume it, but we cannot know, from
     here, whether such a stray flag requires a value, which means we
     cannot know, for sure, how to process the _next_ argument. The
     best we could do is have a heuristic like "if it starts with a
     dash, assume it's a flag, otherwise assume it's a value for the
     previous flag and skip over it," but whether or not that's sane
     enough for daily use is as yet undetermined.

     If we change the CLI interface to require --flag=value for all
     flags, as opposed to optionally allowing (--flag value), the
     above becomes simpler, but CLI usage suffers. Hmmm. e.g.:

     f-ci -m="message" ...

     simply doesn't fit the age-old muscle memory of:

     svn ci -m ...
     cvs ci -m ...
     fossil ci -m ...
  */
  for( f = defs; f->flagShort || f->flagLong; ++f ){
    if(!f->flagValue && !f->callback){
      /* We accept these for purposes of generating the --help text,
         but we can't otherwise do anything sensible with them and
         assume the app will handle such flags downstream or ignore
         them altogether.*/
      continue;
    }
    char const * v = NULL;
    const char ** passV = f->flagValue ? &v : NULL;
    switch(f->flagType){
      case FCLI_FLAG_TYPE_BOOL:
      case FCLI_FLAG_TYPE_BOOL_INVERT:
        passV = NULL;
        break;
      default: break;
    };
    bool const gotIt = fcli_flag2(f->flagShort, f->flagLong, passV);
    if(fcli__error->code){
      /**
         Corner case. Consider:

         FCLI_FLAG("x","y","xy", &foo, "blah");

         And: my-app -x

         That will cause fcli_flag2() to return false, but it will
         also populate fcli__error for us.
      */
      rc = fcli__error->code;
      break;
    }
    //MARKER(("Got?=%d flag: %s/%s %s\n",gotIt, f->flagShort, f->flagLong, v ? v : ""));
    if(!gotIt){
      continue;
    }
    assert(f->flagValue || f->callback);
    if(f->flagValue) switch(f->flagType){
      case FCLI_FLAG_TYPE_BOOL:
        *((bool*)f->flagValue) = true;
        break;
      case FCLI_FLAG_TYPE_BOOL_INVERT:
        *((bool*)f->flagValue) = false;
        break;
      case FCLI_FLAG_TYPE_CSTR:
        if(!v) goto missing_val;
        *((char const **)f->flagValue) = v;
        break;
      case FCLI_FLAG_TYPE_INT32:
        if(!v) goto missing_val;
        *((int32_t*)f->flagValue) = atoi(v);
        break;
      case FCLI_FLAG_TYPE_INT64:
        if(!v) goto missing_val;
        *((int64_t*)f->flagValue) = atoll(v);
        break;
      case FCLI_FLAG_TYPE_ID:
        if(!v) goto missing_val;
        if(sizeof(fsl_id_t)>32){
          *((fsl_id_t*)f->flagValue) = (fsl_id_t)atoll(v);
        }else{
          *((fsl_id_t*)f->flagValue) = (fsl_id_t)atol(v);
        }
        break;
      case FCLI_FLAG_TYPE_DOUBLE:
        if(!v) goto missing_val;
        *((double*)f->flagValue) = strtod(v, NULL);
        break;
      default:
        MARKER(("As-yet-unhandled flag type for flag %s%s%s.",
                f->flagShort ? f->flagShort : "",
                (f->flagShort && f->flagLong) ? "|" : "",
                f->flagLong ? f->flagLong : ""));
        rc = FSL_RC_MISUSE;
        break;
    }
    if(rc) break;
    else if(f->callback){
      rc = f->callback(f);
      if(rc==FCLI_RC_FLAG_AGAIN){
        rc = 0;
        --f;
      }else if(rc){
        break;
      }
    }
  }
  //MARKER(("fcli__error->code==%s\n", fsl_rc_cstr(fcli__error->code)));
  return rc;
  missing_val:
  rc = fcli_err_set(FSL_RC_MISUSE,"Missing value for flag %s%s%s.",
                    f->flagShort ? f->flagShort : "",
                    (f->flagShort && f->flagLong) ? "|" : "",
                    f->flagLong ? f->flagLong : "");
  return rc;
}

/**
   oldMode must be true if fcli.cliFlags is NULL, else false.
*/
static int fcli_process_argv( bool oldMode, int argc, char const * const * argv ){
  int i;
  int rc = 0;
  char * cp;
  fcli.appName = argv[0];
  fcli.argc = 0;
  fcli.argv = (char **)fsl_malloc( (argc + 1) * sizeof(char*));
  fcli.argv[argc] = NULL;
  for( i = 1; i < argc; ++i ){
    char const * arg = argv[i];
    if('-'==*arg){
      char const * flag = arg+1;
      while('-'==*flag) ++flag;
#define FLAG(F) if(0==fsl_strcmp(F,flag))
      if(oldMode){
        FLAG("help") {
          ++fcli.transient.helpRequested;
          continue;
        }
        FLAG("?") {
          ++fcli.transient.helpRequested;
          continue;
        }
        FLAG("V") {
          fcli.clientFlags.verbose += 1;
          continue;
        }
        FLAG("VV") {
          fcli.clientFlags.verbose += 2;
          continue;
        }
        FLAG("VVV") {
          fcli.clientFlags.verbose += 3;
          continue;
        }
        FLAG("verbose") {
          fcli.clientFlags.verbose += 1;
          continue;
        }
      }
#undef FLAG
      /* else fall through */
    }
    cp = fsl_strdup(arg);
    if(!cp) return FSL_RC_OOM;
    fcli.argv[fcli.argc++] = cp;
    fcli_fax(cp);
  }
  if(!rc && !oldMode){
    rc = fcli_process_flags(FCliFlagsGlobal);
  }
  return rc;
}

bool fcli_flag(char const * opt, const char ** value){
  int i = 0;
  int remove = 0 /* number of items to remove from argv */;
  bool rc = false /* true if found, else 0 */;
  fsl_size_t optLen = fsl_strlen(opt);
  for( ; i < fcli.argc; ++i ){
    char const * arg = fcli.argv[i];
    char const * x;
    char const * vp = NULL;
    if(!arg || ('-' != *arg)) continue;
    rc = false;
    x = arg+1;
    if('-' == *x) { ++x;}
    if(0 != fsl_strncmp(x, opt, optLen)) continue;
    if(!value){
      if(x[optLen]) continue /* not exact match */;
      /* Treat this as a boolean. */
      rc = true;
      ++remove;
      break;
    }else{
      /* -FLAG VALUE or -FLAG=VALUE */
      if(x[optLen] == '='){
        rc = true;
        vp = x+optLen+1;
        ++remove;
      }
      else if(x[optLen]) continue /* not an exact match */;
      else if(i<(fcli.argc-1)){ /* -FLAG VALUE */
        vp = fcli.argv[i+1];
        if('-'==*vp && vp[1]/*allow "-" by itself!*/){
          // VALUE looks like a flag.
          fcli_err_set(FSL_RC_MISUSE, "Missing value for flag [%s].",
                       opt);
          rc = false;
          assert(!remove);
          break;
        }
        rc = true;
        remove += 2;
      }
      else{
        /*
          --FLAG is expecting VALUE but we're at end of argv.  Leave
          --FLAG in the args and report this as "not found."
        */
        rc = false;
        assert(!remove);
        fcli_err_set(FSL_RC_MISUSE,
                     "Missing value for flag [%s].",
                     opt);
        assert(fcli__error->code);
        //MARKER(("Missing flag value for [%s]\n",opt));
        break;
      }
      if(rc){
        *value = vp;
      }
      break;
    }
  }
  if(remove>0){
    int x;
    for( x = 0; x < remove; ++x ){
      fcli.argv[i+x] = NULL/*memory ownership==>FCliFree*/;
    }
    for( ; i < fcli.argc; ++i ){
      fcli.argv[i] = fcli.argv[i+remove];
    }
    fcli.argc -= remove;
    fcli.argv[i] = NULL;
  }
  //MARKER(("flag %s check rc=%s\n",opt,fsl_rc_cstr(fcli__error->code)));
  return rc;
}

bool fcli_flag2(char const * shortOpt,
                char const * longOpt,
                const char ** value){
  bool rc = 0;
  if(shortOpt) rc = fcli_flag(shortOpt, value);
  if(!rc && longOpt && !fcli__error->code) rc = fcli_flag(longOpt, value);
  //MARKER(("flag %s check rc=%s\n",shortOpt,fsl_rc_cstr(fcli__error->code)));
  return rc;
}

bool fcli_flag_or_arg(char const * shortOpt,
                      char const * longOpt,
                      const char ** value){
  bool rc = fcli_flag(shortOpt, value);
  if(!rc && !fcli__error->code){
    rc = fcli_flag(longOpt, value);
    if(!rc && value){
      const char * arg = fcli_next_arg(1);
      if(arg){
        rc = true;
        *value = arg;
      }
    }
  }
  return rc;
}


/**
    We copy fsl_lib_configurable.allocator as a base allocator.
 */
static fsl_allocator fslAllocOrig;

/**
    Proxies fslAllocOrig.f() and abort()s on OOM conditions.
*/
static void * fsl_realloc_f_failing(void * state, void * mem, fsl_size_t n){
  void * rv = fslAllocOrig.f(fslAllocOrig.state, mem, n);
  if(n && !rv){
    fsl_fatal(FSL_RC_OOM, NULL)/*does not return*/;
  }
  return rv;
}

/**
    Replacement for fsl_memory_allocator() which abort()s on OOM.
    Why? Because fossil(1) has shown how much that can simplify error
    checking in an allocates-often API.
 */
static const fsl_allocator fcli_allocator = {
fsl_realloc_f_failing,
NULL/*state*/
};

void fcli_pre_setup(void){
  static int run = 0;
  if(run++) return;
  fslAllocOrig = fsl_lib_configurable.allocator;
  fsl_lib_configurable.allocator = fcli_allocator
    /* This MUST be done BEFORE the fsl API allocates
       ANY memory! */;
  atexit(fcli_shutdown);
}
/**
   oldMode must be true if fcli.cliFlags is NULL, else false.
*/
static int fcli_setup_common1(bool oldMode, int argc, char const * const *argv){
  static char once = 0;
  int rc = 0;
  if(once++){
    fprintf(stderr,"MISUSE: fcli_setup() must "
            "not be called more than once.");
    return FSL_RC_MISUSE;
  }
  fsl_timer_start(&fcliTimer);
  fcli_pre_setup();
  rc = fcli_process_argv(oldMode, argc, argv);
  if(!rc && fcli.argc && 0==fsl_strcmp("help",fcli.argv[0])){
    fcli_next_arg(1) /* strip argument */;
    ++fcli.transient.helpRequested;
  }
  return rc;
}

static int fcli_setup_common2(void){
  int rc = 0;
  fsl_cx_init_opt init = fsl_cx_init_opt_empty;
  fsl_cx * f = 0;

  init.config.sqlPrint = 1;
  if(fcli.config.outputer.out){
    init.output = fcli.config.outputer;
    fcli.config.outputer = fsl_outputer_empty
      /* To avoid any confusion about ownership */;
  }else{
    init.output = fsl_outputer_FILE;
    init.output.state.state = stdout;
  }
  if(fcli.config.traceSql>0 || TempFlags.traceSql){
    init.config.traceSql = fcli.config.traceSql;
  }
    
  rc = fsl_cx_init( &f, &init );
  fcli.f = f;
#if 0
  /* Just for testing cache size effects... */
  f->cache.arty.szLimit = 1024 * 1024 * 20;
  f->cache.arty.usedLimit = 300;
#endif
  fsl_error_clear(&fcli.err);
  FCLI_V3(("Initialized fsl_cx @0x%p. rc=%s\n",
           (void const *)f, fsl_rc_cstr(rc)));
  if(!rc){
#if 0
    if(fcli.transient.gmtTime){
      fsl_cx_flag_set(f, FSL_CX_F_LOCALTIME_GMT, 1);
    }
#endif
    if(fcli.clientFlags.checkoutDir || fcli.transient.repoDbArg){
      rc = fcli_open();
      FCLI_V3(("fcli_open() rc=%s\n", fsl_rc_cstr(rc)));
      if(!fcli.transient.repoDbArg && fcli.clientFlags.checkoutDir
         && (FSL_RC_NOT_FOUND == rc)){
        /* If [it looks like] we tried an implicit checkout-open but
           didn't find one, suppress the error. */
        rc = 0;
        fcli_err_reset();
      }
    }
  }
  if(!rc){
    char const * userName = fcli.transient.userArg;
    if(userName){
      fsl_cx_user_set(f, userName);
    }else if(!fsl_cx_user_get(f)){
      char * u = fsl_guess_user_name();
      fsl_cx_user_set(f, u);
      fsl_free(u);
    }
  }
  return rc;
}

static int fcli_setup2(int argc, char const * const * argv,
                       const fcli_cliflag * flags){
  int rc;
  FCliHelpState.flags = flags;
  rc = fcli_setup_common1(false, argc, argv);
  if(rc) return rc;
  assert(!fcli__error->code);
  if(fcli.transient.helpRequested){
    /* Do this last so that we can get the default user name and such
       for display in the help text. */
    fcli_help();
    rc = FCLI_RC_HELP;
  }else{
    rc = fcli_process_flags(flags);
    if(rc) assert(fcli__error->msg.used);
    if(!rc){
      rc = fcli_setup_common2();
    }
  }
  return rc;
}

int fcli_setup(int argc, char const * const * argv ){
  int rc = 0;
  if(fcli.cliFlags){
    return fcli_setup2(argc, argv, fcli.cliFlags);
  }
  rc = fcli_setup_common1(true, argc, argv);
  if(!rc){
    //f_out("fcli.transient.helpRequested=%d\n",fcli.transient.helpRequested);
    if(fcli.transient.helpRequested){
      /* Do this last so that we can get the default user name and such
         for display in the help text. */
      fcli_help();
      rc = FCLI_RC_HELP;
    }else{
      if( fcli_flag2("C", "no-checkout", NULL) ){
        fcli.clientFlags.checkoutDir = NULL;
      }
      fcli_flag2("U","user", &fcli.transient.userArg);
      fcli.config.traceSql =  fcli_flag2("S","trace-sql", NULL);
      fcli.clientFlags.dryRun = fcli_flag2("D","dry-run", NULL);
      fcli_flag2("R", "repo", &fcli.transient.repoDbArg);
      rc = fcli_setup_common2();
    }
  }
  return rc;
}


int fcli_err_report2(bool clear, char const * file, int line){
  int errRc = 0;
  char const * msg = NULL;
  errRc = fsl_error_get( fcli__error, &msg, NULL );
  if(FCLI_RC_HELP==errRc){
    errRc = 0;
  }else if(errRc || msg){
    if(fcli.clientFlags.verbose>0){
      fcli_printf("%s %s:%d: ERROR #%d (%s): %s\n",
                  fcli.appName,
                  file, line, errRc, fsl_rc_cstr(errRc), msg);
    }else{
      fcli_printf("%s: ERROR #%d (%s): %s\n",
                  fcli.appName, errRc, fsl_rc_cstr(errRc), msg);
    }
  }
  if(clear) fcli_err_reset();
  return errRc;
}


const char * fcli_next_arg(bool remove){
  const char * rc = (fcli.argc>0) ? fcli.argv[0] : NULL;
  if(rc && remove){
    int i;
    --fcli.argc;
    for(i = 0; i < fcli.argc; ++i){
      fcli.argv[i] = fcli.argv[i+1];
    }
    fcli.argv[fcli.argc] = NULL/*owned by FCliFree*/;
  }
  return rc;
}

int fcli_has_unused_args(bool outputError){
  int rc = 0;
  if(fcli.argc){
    rc = fsl_cx_err_set(fcli.f, FSL_RC_MISUSE,
                        "Unhandled extra argument: %s",
                        fcli.argv[0]);
    if(outputError){
      fcli_err_report(false);
    }
  }
  return rc;
}
int fcli_has_unused_flags(bool outputError){
  int i;
  for( i = 0; i < fcli.argc; ++i ){
    char const * arg = fcli.argv[i];
    if('-'==*arg){
      int rc = fsl_cx_err_set(fcli.f, FSL_RC_MISUSE,
                              "Unhandled/unknown flag or missing value: %s",
                              arg);
      if(outputError){
        fcli_err_report(false);
      }
      return rc;
    }
  }
  return 0;
}

int fcli_err_set(int code, char const * fmt, ...){
  int rc;
  va_list va;
  va_start(va, fmt);
  rc = fsl_error_setv(fcli__error, code, fmt, va);
  va_end(va);
  return rc;
}

int fcli_end_of_main(int mainRc){
  if(FCLI_RC_HELP==mainRc){
    mainRc = 0;
  }
  if(fcli_err_report(true)){
    return EXIT_FAILURE;
  }else if(mainRc){
    fcli_err_set(mainRc,"Ending with unadorned end-of-app "
                 "error code %d/%s.",
                 mainRc, fsl_rc_cstr(mainRc));
    fcli_err_report(true);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int fcli_dispatch_commands( fcli_command const * cmd,
                            bool reportErrors){
  int rc = 0;
  const char * arg = fcli_next_arg(0);
  fcli_command const * orig = cmd;
  fcli_command const * helpPos = 0;
  int helpState = 0;
  if(!arg){
    return fcli_err_set(FSL_RC_MISUSE,
                        "Missing command argument. Try --help.");
  }
  assert(fcli.f);
  for(; arg && cmd->name; ++cmd ){
    if(cmd==orig && 0==fsl_strcmp(arg,"help")){
      /* Accept either (help command) or (command help) as help. */
      /* Except that it turns out that fcli_setup() will trump the
         former and doesn't have the fcli_command state, so can't do
         this. Maybe we can change that somehow. */
      helpState = 1;
      helpPos = orig;
      arg = fcli_next_arg(1); // consume it
    }else if(0==fsl_strcmp(arg,cmd->name)){
      if(!cmd->f){
        rc = fcli_err_set(FSL_RC_NYI,
                               "Command [%s] has no "
                               "callback function.");
      }else{
        fcli_next_arg(1)/*consume it*/;
        if(helpState){
          assert(1==helpState);
          helpState = 2;
          helpPos = cmd;
          break;
        }
        const char * helpCheck = fcli_next_arg(false);
        if(helpCheck && 0==fsl_strcmp("help",helpCheck)){
          helpState = 3;
          helpPos = cmd;
          break;
        }else{
          rc = cmd->f(cmd);
        }
      }
      break;
    }
  }
  if(helpState){
    f_out("\n");
    fcli_command_help(helpPos, helpState>1);
  }else if(!cmd->name){
    fsl_buffer msg = fsl_buffer_empty;
    int rc2;
    if(!arg){
      rc2 = FSL_RC_MISUSE;
      fsl_buffer_appendf(&msg, "No command provided.");
    }else{
      rc2 = FSL_RC_NOT_FOUND;
      fsl_buffer_appendf(&msg, "Command not found: %s.",arg);
    }
    fsl_buffer_appendf(&msg, " Available commands: ");
    cmd = orig;
    for( ; cmd && cmd->name; ++cmd ){
      fsl_buffer_appendf( &msg, "%s%s",
                          (cmd==orig) ? "" : ", ",
                          cmd->name);
    }
    rc = fcli_err_set(rc2, "%b", &msg);
    fsl_buffer_clear(&msg);
  }
  if(rc && reportErrors){
    fcli_err_report(0);
  }
  return rc;
}

void fcli_command_help(fcli_command const * cmd, bool onlyOne){
  fcli_command const * c = cmd;
  for( ; c->name; ++c ){
    f_out("[%s] command:\n\n", c->name);
    if(c->briefDescription){
      f_out("  %s\n", c->briefDescription);
    }
    if(c->flags){
      f_out("\n");
      fcli_cliflag_help(c->flags);
    }
    if(onlyOne) break;
  }
}

void fcli_fax(void * mem){
  if(mem){
    fsl_list_append( &FCliFree.list, mem );
  }
}

int fcli_ckout_show_info(bool useUtc){
  fsl_cx * const f = fcli_cx();
  int rc = 0;
  fsl_stmt st = fsl_stmt_empty;
  fsl_db * const dbR = fsl_cx_db_repo(f);
  fsl_db * const dbC = fsl_cx_db_ckout(f);
  int lblWidth = -20;
  if(!fsl_needs_ckout(f)){
    return FSL_RC_NOT_A_CKOUT;
  }
  assert(dbR);
  assert(dbC);

  fsl_id_t rid = 0;
  fsl_uuid_cstr uuid = NULL;
  fsl_ckout_version_info(f, &rid, &uuid);
  assert((uuid && (rid>0)) || (!uuid && (0==rid)));

  f_out("%*s %s\n", lblWidth, "repository-db:",
        fsl_cx_db_file_repo(f, NULL));
  f_out("%*s %s\n", lblWidth, "checkout-root:",
        fsl_cx_ckout_dir_name(f, NULL));

  rc = fsl_db_prepare(dbR, &st, "SELECT "
                      /*0*/"datetime(event.mtime%s) AS timestampString, "
                      /*1*/"coalesce(euser, user) AS user, "
                      /*2*/"(SELECT group_concat(substr(tagname,5), ', ') FROM tag, tagxref "
                      "WHERE tagname GLOB 'sym-*' AND tag.tagid=tagxref.tagid "
                      "AND tagxref.rid=blob.rid AND tagxref.tagtype>0) as tags, "
                      /*3*/"coalesce(ecomment, comment) AS comment, "
                      /*4*/"uuid AS uuid "
                      "FROM event JOIN blob "
                      "WHERE "
                      "event.type='ci' "
                      "AND blob.rid=%"FSL_ID_T_PFMT" "
                      "AND blob.rid=event.objid "
                      "ORDER BY event.mtime DESC",
                      useUtc ? "" : ", 'localtime'",
                      rid);
  if(rc) goto dberr;
  if( FSL_RC_STEP_ROW != fsl_stmt_step(&st)){
    /* fcli_err_set(FSL_RC_ERROR, "Event data for checkout not found."); */
    f_out("\nNo 'event' data found. This is only normal for an empty repo.\n");
    goto end;

  }

  f_out("%*s %s %s %s (RID %"FSL_ID_T_PFMT")\n",
        lblWidth, "checkout-version:",
        fsl_stmt_g_text(&st, 4, NULL),
        fsl_stmt_g_text(&st, 0, NULL),
        useUtc ? "UTC" : "local",
        rid );

  {
    /* list parent(s) */
    fsl_stmt stP = fsl_stmt_empty;
    rc = fsl_db_prepare(dbR, &stP, "SELECT "
                        "uuid, pid, isprim "
                        "FROM plink JOIN blob ON pid=rid "
                        "WHERE cid=%"FSL_ID_T_PFMT" "
                        "ORDER BY isprim DESC, mtime DESC /*sort*/",
                        rid);
    if(rc) goto dberr;
    while( FSL_RC_STEP_ROW == fsl_stmt_step(&stP) ){
      char const * zLabel = fsl_stmt_g_int32(&stP,2)
        ? "parent:" : "merged-from:";
      f_out("%*s %s\n", lblWidth, zLabel,
            fsl_stmt_g_text(&stP, 0, NULL));
      
    }
    fsl_stmt_finalize(&stP);
  }
  {
    /* list children */
    fsl_stmt stC = fsl_stmt_empty;
    rc = fsl_db_prepare(dbR, &stC, "SELECT "
                        "uuid, cid, isprim "
                        "FROM plink JOIN blob ON cid=rid "
                        "WHERE pid=%"FSL_ID_T_PFMT" "
                        "ORDER BY isprim DESC, mtime DESC /*sort*/",
                        rid);
    if(rc) goto dberr;
    while( FSL_RC_STEP_ROW == fsl_stmt_step(&stC) ){
      char const * zLabel = fsl_stmt_g_int32(&stC,2)
        ? "child:" : "merged-into:";
      f_out("%*s %s\n", lblWidth, zLabel,
            fsl_stmt_g_text(&stC, 0, NULL));
      
    }
    fsl_stmt_finalize(&stC);
  }

  f_out("%*s %s\n", lblWidth, "user:",
        fsl_stmt_g_text(&st, 1, NULL));

  f_out("%*s %s\n", lblWidth, "tags:",
        fsl_stmt_g_text(&st, 2, NULL));

  f_out("%*s %s\n", lblWidth, "comment:",
        fsl_stmt_g_text(&st, 3, NULL));

  dberr:
  if(rc){
    fsl_cx_uplift_db_error(f, dbR);
  }
  end:
  fsl_stmt_finalize(&st);

  return rc;
}

static int fsl_stmt_each_f_ambiguous( fsl_stmt * stmt, void * state ){
  int rc;
  if(1==stmt->rowCount) stmt->rowCount=0
                          /* HORRIBLE KLUDGE to elide header. */;
  rc = fsl_stmt_each_f_dump(stmt, state);
  if(0==stmt->rowCount) stmt->rowCount = 1;
  return rc;
}

void fcli_list_ambiguous_artifacts(char const * label,
                                   char const *prefix){
  fsl_db * const db = fsl_cx_db_repo(fcli.f);
  assert(db);
  if(!label){
    f_out("Artifacts matching ambiguous prefix: %s\n",prefix);
  }else if(*label){
    f_out("%s\n", label);
  }  
  /* Possible fixme? Do we only want to list checkins
     here? */
  int rc = fsl_db_each(db, fsl_stmt_each_f_ambiguous, 0,
              "SELECT uuid, CASE "
              "WHEN type='ci' THEN 'Checkin' "
              "WHEN type='w'  THEN 'Wiki' "
              "WHEN type='g'  THEN 'Control' "
              "WHEN type='e'  THEN 'Technote' "
              "WHEN type='t'  THEN 'Ticket' "
              "WHEN type='f'  THEN 'Forum' "
              "ELSE '?'||'?'||'?' END " /* '???' ==> trigraph! */
              "FROM blob b, event e WHERE uuid LIKE %Q||'%%' "
              "AND b.rid=e.objid "
              "ORDER BY uuid",
              prefix);
  if(rc){
    fsl_cx_uplift_db_error(fcli.f, db);
    fcli_err_report(false);
  }
}

fsl_db * fcli_db_ckout(void){
  return fcli.f ? fsl_cx_db_ckout(fcli.f) : NULL;
}

fsl_db * fcli_db_repo(void){
  return fcli.f ? fsl_cx_db_repo(fcli.f) : NULL;
}

fsl_db * fcli_needs_ckout(void){
  if(fcli.f) return fsl_needs_ckout(fcli.f);
  fcli_err_set(FSL_RC_NOT_A_CKOUT,
               "No checkout db is opened.");
  return NULL;
}

fsl_db * fcli_needs_repo(void){
  if(fcli.f) return fsl_needs_repo(fcli.f);
  fcli_err_set(FSL_RC_NOT_A_REPO,
               "No repository db is opened.");
  return NULL;
}

int fcli_args_to_vfile_ids(fsl_id_bag *tgt, fsl_id_t vid,
                           bool relativeToCwd,
                           bool changedFilesOnly){
  if(!fcli.argc){
    return fcli_err_set(FSL_RC_MISUSE,
                        "No file/dir name arguments provided.");
  }
  int rc = 0;
  char const * zName;
  while( !rc && (zName = fcli_next_arg(true))){
    FCLI_V3(("Collecting vfile ID(s) for: %s\n", zName));
    rc = fsl_ckout_vfile_ids(fcli.f, vid, tgt, zName,
                             relativeToCwd, changedFilesOnly);
  }
  return rc;
}

int fcli_fingerprint_check(bool reportImmediately){
  int rc = fsl_ckout_fingerprint_check(fcli.f);
  if(rc && reportImmediately){
    f_out("ERROR: repo/checkout fingerprint mismatch detected. "
          "To recover from this, (fossil close) the current checkout, "
          "then re-open it. Be sure to store any modified files somewhere "
          "safe and restore them after re-opening the repository.\n");
  }
  return rc;
}

#undef FCLI_V3
#undef fcli_empty_m
#undef fcli__error
#undef MARKER
