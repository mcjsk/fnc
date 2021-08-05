/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include "cliapp.h"
#include <string.h>
#include <assert.h>
#include <stdio.h> /* vsprintf() */
#include <stdlib.h> /* abort() */

#ifndef CLIAPP_ENABLE_READLINE
#  define CLIAPP_ENABLE_READLINE 0
#endif

#ifndef CLIAPP_ENABLE_LINENOISE
#  define CLIAPP_ENABLE_LINENOISE 0
#endif

#if CLIAPP_ENABLE_READLINE && CLIAPP_ENABLE_LINENOISE
#  error "Use *one* of CLIAPP_ENABLE_READLINE or CLIAPP_ENABLE_LINENOISE, not both."
#endif

#if CLIAPP_ENABLE_LINENOISE
#  include "linenoise/linenoise.h"
#elif CLIAPP_ENABLE_READLINE
#  include "readline/readline.h"
#  include "readline/history.h"
#endif


#if 1
#define MARKER(pfexp) if(1) printf("%s:%d:\t",__FILE__,__LINE__); \
  if(1) printf pfexp
#else
static void noop_printf(char const * fmt, ...) {}
#define MARKER(pfexp) if(0) noop_printf pfexp
#endif
#define MESSAGE(pfexp) printf pfexp

struct CliApp cliApp = {
0/*argc*/, 0/*argv*/,
0/*cursorNonflag*/,
0/*errMsg*/,
0/*flags*/,
vprintf/*print*/,
0/*argCallback*/,
{/*doubleDash*/
  0/*argc*/,
  0/*argv*/
},
{/*lineread*/
  CLIAPP_ENABLE_LINENOISE ? 1 : (CLIAPP_ENABLE_READLINE ? 2 : 0) /*enabled*/,
  0 /* historyFile */, 0 /*needsSave*/
}
};

enum {
/** Buffer size for cliapp_errmsg() */
CLIAPP_MSGBUF_SIZE = 1024 * 4,
/** Buffer size for cliapp_process_argv() argument keys. */
CLIAPP_ARGV_KBUF_SIZE = 1024 * 2,
/** Maximum cliapp_process_argv() arg count. */
CLIAPP_ARGV_COUNT = (int)(1024 * 2 / sizeof(CliAppArg))
};

static char cliAppErrBuf[CLIAPP_MSGBUF_SIZE] = {0};

static char const * cliapp_verrmsg(char const * fmt, va_list args){
  int rc;
  char * buf = cliAppErrBuf;
  rc = vsprintf(buf, fmt, args)
    /* Noting that vsnprintf() requires C99 and s2 aims to compile in
       strict C89 mode. */;
  if(rc >= CLIAPP_MSGBUF_SIZE){
    fprintf(stderr,"%s:%d: Internal misuse of error message buffer. "
            "Dangerous buffer overrun!\n",
            __FILE__, __LINE__);
    abort();
  }
  return cliApp.errMsg = buf;
}
static char const * cliapp_errmsg(char const * fmt, ...){
  /** Stores error messages, at least until it's overwritten. */
  char const * rc;
  va_list args;
  va_start(args,fmt);
  rc = cliapp_verrmsg(fmt, args);
  va_end(args);
  return rc;
}

void cliapp_err_clear(){
  cliApp.errMsg = 0;
  cliAppErrBuf[0] = 0;
}

char const * cliapp_err_get(){
  return cliApp.errMsg;
}

static const CliAppSwitch CliAppSwitch_end = CliAppSwitch_sentinel;
#define cliapp__switch_is_end(S) \
  (0==memcmp(S, &CliAppSwitch_end, sizeof(CliAppSwitch)))
int cliapp_switch_is_end(CliAppSwitch const *s){
  return cliapp__switch_is_end(s);
}

CliAppSwitch const * cliapp_switch_for_arg(CliAppArg const * arg,
                                           int alsoFlag){
  CliAppSwitch const * a = cliApp.switches;
  for(; !cliapp__switch_is_end(a); ++a){
    if((alsoFlag==0 || arg->dash==a->dash)
       && 0==strcmp(a->key, arg->key)){
       return a;
    }
  }
  return 0;
}

CliAppArg const * cliapp_arg_next_same(CliAppArg const * arg){
  CliAppArg const * a = arg;
  CliAppArg const * tail = cliApp.argv + cliApp.argc;
  if(arg < cliApp.argv || arg >= tail){
    return 0;
  }
  for( ++a; a < tail; ++a ){
    if(a->key && 0==strcmp(a->key, arg->key)) return a;
  }
  return 0;
}

CliAppArg const * cliapp_arg_nonflag(){
  CliAppArg const * p = 0;
  for(; cliApp.cursorNonflag < cliApp.argc;){
    p = cliApp.argv + cliApp.cursorNonflag;
    ++cliApp.cursorNonflag;
    if(p->key && 0==p->dash){
      return p;
    }
  }
  return 0;
}

void cliapp_arg_nonflag_rewind(){
  cliApp.cursorNonflag = 1;
}

CliAppArg const * cliapp_arg_flag(char const * key1,
                                  char const * key2,
                                  int *atPos){
  int i = atPos ? *atPos : 1;
  assert(key1 || key2);
  if(!key1 & !key2) return 0;
  for( ; i < cliApp.argc; ++i ){
    CliAppArg const * p = cliApp.argv + i;
    if(!p->key) continue;
    else if((key1 && 0==strcmp(key1,p->key))
             ||
             (key2 && 0==strcmp(key2,p->key))){
      if(atPos) *atPos = i;
      return p;
    }
  }
  return 0;
}

char const * cliapp_flag_prefix( int flag ){
  char const * flags[4] = {"+",  "", "-", "--"};
  assert(flag>=-1 && flag<=2);
  return flag>=-1 && flag<=2 ? flags[flag+1] : "";
}

void cliapp_switches_visit( CliAppSwitch_visitor_f visitor,
                            void * state ){
  CliAppSwitch const * s = cliApp.switches;
  assert(s);
  for( ; !cliapp__switch_is_end(s); ++s){
    if(visitor(s, state)) break;
  }  
}

void cliapp_args_visit( CliAppArg_visitor_f visitor, void * state,
                        unsigned short skipArgs ){
  int i = skipArgs;
  CliAppArg const * a = i <  cliApp.argc
    ? &cliApp.argv[i] : NULL;
  for( ; i < cliApp.argc; ++i, ++a ){
    if(a->key && visitor(a, i, state)) break;
  }
}

void cliapp_printv(char const *fmt, va_list vargs){
  if(cliApp.print){
    cliApp.print(fmt, vargs);
  }
}

void cliapp_print(char const *fmt, ...){
  if(cliApp.print){
    va_list vargs;
    va_start(vargs,fmt);
    cliApp.print(fmt, vargs);
    va_end(vargs);
  }    
}

void cliapp_warn(char const *fmt, ...){
    va_list vargs;
    va_start(vargs,fmt);
    vfprintf(stderr, fmt, vargs);
    va_end(vargs);
}

#if 0
static void cliapp_perr(char const *fmt, ...){
    va_list vargs;
    va_start(vargs,fmt);
    cliapp_verrmsg(fmt,vargs);
    va_end(vargs);
    va_start(vargs,fmt);
    vfprintf(stderr, fmt, vargs);
    va_end(vargs);
}
#endif

/**
   The list of arguments pointed to by cliApp.argv.
*/
static CliAppArg cliAppArgv[CLIAPP_ARGV_COUNT];
/**
   An internal buffer used to store --flag keys for those keys which
   require transformation. This is: for --flag=value, we copy the
   "flag" part to this buffer so we can NUL-terminate it. For flags
   with no '=' we simply refer to the original argv string pointers,
   as those are NUL terminated. (We "could" modify the original
   globals instead, but the thought of doing so makes me a ill.)
*/
static char cliAppKeyBuf[CLIAPP_ARGV_KBUF_SIZE]
/* Store all NUL-terminated flag keys here, stripped of their
   leading dashes and terminated at their '=' (if they had
   one). */;

/**
   Internal helper for cliapp_process_argv() which checks list to see
   if s is contained in it. list must be terminated by a NULL pointer.
*/
static char cliapp_check_seen( CliAppSwitch const * const * list,
                               CliAppSwitch const * s ){
  int i = 0;
  CliAppSwitch const * p;
  assert(s);
  while(1){
    p = list[i++];
    if(p == s) return 1;
    else if(!p) break;
  }
  return 0;
}

int cliapp_process_argv(int argc, char const * const * argv,
                        unsigned int reserved ){
  /*static CliAppSwitch const * sPending = 0;
    static CliAppArg * aPending = 0;*/
  int i, rc = 0, doubleDashPos = 0;
  char * k = &cliAppKeyBuf[0];
  char const *errMsg = 0;
  CliAppSwitch const * switchPendingVal = 0
    /* Switch with the CLIAPP_F_SPACE_VALUE which is expecting
       a value in the next argument. */;
  CliAppArg * argPendingVal = 0
    /* arg counterpart of switchPendingVal */;
  CliAppSwitch const * appSwitch = 0;
  CliAppSwitch const * seenList[CLIAPP_ARGV_COUNT+1]
    /* switches we've seen so far, so we can check for/enforce
       the CLIAPP_F_ONCE flag */;
  int seenCount = 0 /* number of entries in seenList */;
  if(reserved){/*unused*/}
  memset(seenList, 0, sizeof(seenList));
  assert(cliApp.switches
         && "cliApp.switches must be set by the client "
         "before calling this.");
  cliApp.argv = &cliAppArgv[0];
  memset(&cliAppArgv[0], 0, sizeof(cliAppArgv));
#define KCHECK if(k>=&cliAppKeyBuf[0]+CLIAPP_ARGV_KBUF_SIZE) goto err_overflow
#define BADFLAG(MSG) errMsg=MSG; goto misuse
  for(i = 0; i < argc && 0==doubleDashPos; ++i){
    CliAppArg * p;
    char const * arg;
    int dashes = 0 /* number of dashes on the current flag */;
    int callbackIndex = i
      /* index to pass to callbacks. Gets modified in one case */;
    if(i == CLIAPP_ARGV_COUNT){
      cliapp_errmsg("Too many (%d) arguments: internal buffer "
                    "limit (%d) would be reached.", argc,
                    CLIAPP_ARGV_COUNT);
      return CLIAPP_RC_RANGE;
    }
    appSwitch = 0;
    p = cliApp.argv + i;
    arg = argv[i];
    if(*arg=='+'){
      dashes = -1;
      ++arg;
    }
    while(*arg && '-'==*arg && dashes<4){
      if(-1==dashes){
        dashes = 3 /* trigger error below */;
        break;
      }
      ++dashes;
      ++arg;
    }
    if(dashes>2){
      BADFLAG("Too many dashes on flag.");
    }
    p->dash = dashes;
    p->key = dashes ? k : arg
      /* dashed args get stored, without dashes, in cliAppKeyBuf so
         that we can terminate them with a NUL at their '='.  Args
         with no dashes are referenced to as-is - there's no need to
         copy them for termination purposes.  We don't know, at this
         point, whether a '=' is pending, but we pessimistically
         (optimistically?) assume there is.
      */;
    while(*arg && '='!=*arg){
      if(dashes){
        *k++ = *arg++;
        KCHECK;
      }      
      else ++arg;
    }
    if(*arg && !*p->key){
      BADFLAG("Empty flag name.");
    }
    KCHECK;
    if(*arg){
      assert('='==*arg);
      if(-1==dashes && '='==*arg){
        BADFLAG("+flags may not have a value.");
      }
      p->value = ++arg;
    }else if(!*p->key && 2==dashes && !doubleDashPos){
      doubleDashPos = i;
      p->key = p->value = 0;
    }
    if(dashes){
      *k++ = 0;
    }
    if(doubleDashPos && !switchPendingVal){
      /* Once we've encountered --, we must not continue to process
         flags because a flag after -- is typically intended for some
         downstream process, not the app on whose behalf we're
         running, and might semantically collide with flags from our
         own app. Also, such flags might have syntaxes we cannot
         handle, so even processing them as flags is potentially a
         bug. Rather than process them, we point cliApp.doubleDash.*
         to that state so the client can deal with it (possibly be
         passing it back into this function!).

         If switchPendingVal is not NULL we need to fall through to
         catch the error case of a missing value at the end of the
         arguments.
      */
      break;
    }else if(!doubleDashPos){
      ++cliApp.argc;
    }
    if(0){
      MARKER(("Arg #%d %d %s %s %s\n",
              i, p->dash, p->key,
              p->value ? "=" : "",
              p->value ? p->value : ""));
    }
    if(switchPendingVal){
      assert(argPendingVal);
      if(p->dash){
        appSwitch = switchPendingVal /* for error string */;
        BADFLAG("Got a flag while the previous flag was expecting "
                "a value");
      }else{
        argPendingVal->value = p->key;
        p->key = p->value = 0;
        p->dash = 0;
        p = argPendingVal /* for the upcoming callback(s) */;
        argPendingVal = 0;
        switchPendingVal = 0;
        --callbackIndex;
      }
    }
    assert(!doubleDashPos);
    if(p->dash && p->key){ /* -flag/--flag/+flag */
#if defined(DEBUG)
      static int check = 0;
      check = callbackIndex;
      assert(cliapp_arg_flag(p->key, 0, &check));
      assert(check == callbackIndex);
#endif
      appSwitch = cliapp_switch_for_arg(p, 1);
      if(!appSwitch){
        BADFLAG("Unknown flag");
      }
      else if((appSwitch->pflags & CLIAPP_F_ONCE)
              && cliapp_check_seen(seenList, appSwitch)){
        BADFLAG("Flag may only be provided once.");
      }
      else if(appSwitch->value && !p->value){
        if(i==argc-1){
          BADFLAG("Flag expecting a value at the end of the arguments");
        }
        switchPendingVal = appSwitch;
        argPendingVal = p;
      }else if(appSwitch->callback){
        rc = appSwitch->callback(callbackIndex, appSwitch, p);
        if(rc) return rc;
      }
      if(!switchPendingVal){
        seenList[seenCount++] = appSwitch;
      }
    }else{
      assert(cliapp_arg_nonflag()==p);
    }
    if(!switchPendingVal && cliApp.argCallack){
      rc = cliApp.argCallack(callbackIndex, appSwitch, p);
      if(rc) return rc;
    }
  }/*for(each arg)*/
  if(cliApp.argCallack){
    rc = cliApp.argCallack(i, 0, 0);
    if(rc) return rc;
  }

#undef BADFLAG
#undef KCHECK
  cliApp.cursorNonflag = 1 /* skip argv[0] */;
  if(doubleDashPos && doubleDashPos+1<argc){
    cliApp.doubleDash.argc = argc - doubleDashPos - 1;
    cliApp.doubleDash.argv = &argv[doubleDashPos+1];
  }
  return 0;
  misuse:
  cliapp_errmsg("Argument #%d: %s%s%s" /* (1) */
                "%s" /*(2)*/
                "%s%.*s" /*(3)*/
                "%s%.*s" /*(4)*/,
                /*(1)*/
                i+1, errMsg, (i<argc) ? ": " : "",
                (i<argc) ? argv[i] : "",
                /*(2)*/ 
                appSwitch ? ", flag: " : "",
                /*(3)*/ 
                appSwitch ? cliapp_flag_prefix(appSwitch->dash) : "",
                (CLIAPP_MSGBUF_SIZE/4),
                appSwitch ? appSwitch->key : "",
                /*(4)*/ 
                appSwitch && appSwitch->value ? "=" : "",
                CLIAPP_MSGBUF_SIZE/4,
                appSwitch ? appSwitch->value : "");
  return CLIAPP_RC_FLAG;
  err_overflow:
  cliapp_errmsg("Too many CLI flag keys: their accumulated names "
                "would overrun our %d-byte buffer.",
                CLIAPP_ARGV_KBUF_SIZE);
  return CLIAPP_RC_RANGE;
}

char * cliapp_lineedit_read(char const * prompt){
#if CLIAPP_ENABLE_LINENOISE
  return linenoise(prompt);
#elif CLIAPP_ENABLE_READLINE
  return readline(prompt);
#else
  if(prompt){/*avoid unused param warning*/}
  return 0;
#endif
}

void cliapp_lineedit_free(char * line){
#if CLIAPP_ENABLE_LINENOISE || CLIAPP_ENABLE_READLINE
  free(line);
#else
  if(line){
    assert(!"Where did this memory come from?");
  }
#endif
}

int cliapp_lineedit_load(char const * fname){
  if(!fname) fname = cliApp.lineread.historyFile;
  if(!fname || !*fname) return 0;
#if CLIAPP_ENABLE_LINENOISE
  return linenoiseHistoryLoad(fname);
#elif CLIAPP_ENABLE_READLINE
  return read_history(fname);
#else
  if(fname){/*avoid unused param warning*/}
  return CLIAPP_RC_UNSUPPORTED;
#endif
}

int cliapp_lineedit_save(char const * fname){
  int rc = 0;
  if(!fname) fname = cliApp.lineread.historyFile;
  if(!cliApp.lineread.needsSave
     || !fname || !*fname) return 0;
#if CLIAPP_ENABLE_LINENOISE
  rc = linenoiseHistorySave(fname) ? CLIAPP_RC_IO : 0;
#elif CLIAPP_ENABLE_READLINE
  rc = write_history(fname) ? CLIAPP_RC_IO : 0;
#else
  if(fname){/*avoid unused param warning*/}
  rc = CLIAPP_RC_UNSUPPORTED;
#endif
  if(!rc) cliApp.lineread.needsSave = 0;
  return rc;
}

int cliapp_lineedit_add(char const * line){
  assert(line);
#if CLIAPP_ENABLE_LINENOISE
  cliApp.lineread.needsSave = 1;
  linenoiseHistoryAdd(line);
  return 0;
#elif CLIAPP_ENABLE_READLINE
  cliApp.lineread.needsSave = 1;
  add_history(line);
  return 0;
#else
  if(line){/*avoid unused param warning*/}
  return CLIAPP_RC_UNSUPPORTED;
#endif
}

int cliapp_repl(CliApp_repl_f f, char const * const * prompt,
                int addHistoryPolicy, void * state){
  char * line;
  int rc = 0;
  while( !rc && (line = cliapp_lineedit_read(*prompt)) ){
    if(addHistoryPolicy<0) cliapp_lineedit_add(line);
    rc = f(line, state);
    if(!rc && addHistoryPolicy>0) cliapp_lineedit_add(line);
    cliapp_lineedit_free(line);
  }
  return rc;
}

#undef cliapp__switch_is_end
