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
/*****************************************************************************
  This file houses some context-independent API routines as well as
  some of the generic helper functions and types.
*/

#include "fossil-scm/fossil-internal.h"
#include "fossil-scm/fossil-repo.h"
#include "fossil-scm/fossil-checkout.h"
#include "fossil-scm/fossil-hash.h"
#include <assert.h>
#include <stdlib.h> /* malloc() and friends, qsort() */
#include <memory.h> /* memset() */
#include <time.h> /* strftime() and gmtime() */

#if defined(_WIN32) || defined(WIN32)
# include <io.h>
#define isatty(h) _isatty(h)
#else
# include <unistd.h> /* isatty() */
#endif
/* extern int isatty(int); */

#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)

/*
  Please keep all fsl_XXX_empty initializers in one place (here)
  and lexically sorted.
*/
const fsl_acache fsl_acache_empty = fsl_acache_empty_m;
const fsl_branch_opt fsl_branch_opt_empty = fsl_branch_opt_empty_m;
const fsl_buffer fsl_buffer_empty = fsl_buffer_empty_m;
const fsl_card_F fsl_card_F_empty = fsl_card_F_empty_m;
const fsl_card_F_list fsl_card_F_list_empty = fsl_card_F_list_empty_m;
const fsl_card_J fsl_card_J_empty = fsl_card_J_empty_m;
const fsl_card_Q fsl_card_Q_empty = fsl_card_Q_empty_m;
const fsl_card_T fsl_card_T_empty = fsl_card_T_empty_m;
const fsl_checkin_opt fsl_checkin_opt_empty = fsl_checkin_opt_empty_m;
const fsl_ckout_manage_opt fsl_ckout_manage_opt_empty =
  fsl_ckout_manage_opt_empty_m;
const fsl_ckout_unmanage_opt fsl_ckout_unmanage_opt_empty =
  fsl_ckout_unmanage_opt_empty_m;
const fsl_ckup_opt fsl_ckup_opt_empty = fsl_ckup_opt_m;
const fsl_confirmer fsl_confirmer_empty = fsl_confirmer_empty_m;
const fsl_cx fsl_cx_empty = fsl_cx_empty_m;
const fsl_cx_config fsl_cx_config_empty = fsl_cx_config_empty_m;
const fsl_cx_init_opt fsl_cx_init_opt_default = fsl_cx_init_opt_default_m;
const fsl_cx_init_opt fsl_cx_init_opt_empty = fsl_cx_init_opt_empty_m;
const fsl_db fsl_db_empty = fsl_db_empty_m;
const fsl_deck fsl_deck_empty = fsl_deck_empty_m;
const fsl_confirm_detail fsl_confirm_detail_empty =
  fsl_confirm_detail_empty_m;
const fsl_confirm_response fsl_confirm_response_empty =
  fsl_confirm_response_empty_m;
const fsl_error fsl_error_empty = fsl_error_empty_m;
const fsl_checkin_queue_opt fsl_checkin_queue_opt_empty =
  fsl_checkin_queue_opt_empty_m;
const fsl_fstat fsl_fstat_empty = fsl_fstat_empty_m;
const fsl_list fsl_list_empty = fsl_list_empty_m;
const fsl_mcache fsl_mcache_empty = fsl_mcache_empty_m;
const fsl_outputer fsl_outputer_FILE = fsl_outputer_FILE_m;
const fsl_outputer fsl_outputer_empty = fsl_outputer_empty_m;
const fsl_pathfinder fsl_pathfinder_empty = fsl_pathfinder_empty_m;
const fsl_pq fsl_pq_empty = fsl_pq_empty_m;
const fsl_repo_create_opt fsl_repo_create_opt_empty =
  fsl_repo_create_opt_empty_m;
const fsl_repo_extract_opt fsl_repo_extract_opt_empty =
  fsl_repo_extract_opt_empty_m;
const fsl_repo_extract_state fsl_repo_extract_state_empty =
  fsl_repo_extract_state_empty_m;
const fsl_repo_open_ckout_opt fsl_repo_open_ckout_opt_empty =
  fsl_repo_open_ckout_opt_m;
const fsl_ckout_revert_opt fsl_ckout_revert_opt_empty =
  fsl_ckout_revert_opt_empty_m;
const fsl_sha1_cx fsl_sha1_cx_empty = fsl_sha1_cx_empty_m;
const fsl_state fsl_state_empty = fsl_state_empty_m;
const fsl_stmt fsl_stmt_empty = fsl_stmt_empty_m;
const fsl_timer_state fsl_timer_state_empty = fsl_timer_state_empty_m;
const fsl_xlinker fsl_xlinker_empty = fsl_xlinker_empty_m;
const fsl_xlinker_list fsl_xlinker_list_empty = fsl_xlinker_list_empty_m;
const fsl_zip_writer fsl_zip_writer_empty = fsl_zip_writer_empty_m;

const fsl_allocator fsl_allocator_stdalloc = {
fsl_realloc_f_stdalloc,
NULL
};

fsl_lib_configurable_t fsl_lib_configurable = {
  {/*allocator*/ fsl_realloc_f_stdalloc, NULL}
};

void * fsl_malloc( fsl_size_t n ){
  return n
    ? fsl_realloc(NULL, n)
    : NULL;
}

void fsl_free( void * mem ){
  if(mem) fsl_realloc(mem, 0);
}

void * fsl_realloc( void * mem, fsl_size_t n ){
#define FLCA fsl_lib_configurable.allocator
  if(!mem){
    /* malloc() */
    return n
      ? FLCA.f(FLCA.state, NULL, n)
      : NULL;
  }else if(!n){
    /* free() */
    FLCA.f(FLCA.state, mem, 0);
    return NULL;
  }else{
    /* realloc() */
    return FLCA.f(FLCA.state, mem, n);
  }
#undef FLCA
}

void * fsl_realloc_f_stdalloc(void * state, void * mem, fsl_size_t n){
  if(!mem){
    return malloc(n);
  }else if(!n){
    free(mem);
    return NULL;
  }else{
    return realloc(mem, n);
  }
}

int fsl_is_uuid(char const * str){
  fsl_size_t const len = fsl_strlen(str);
  if(FSL_STRLEN_SHA1==len){
    return fsl_validate16(str, FSL_STRLEN_SHA1) ? FSL_STRLEN_SHA1 : 0;
  }else if(FSL_STRLEN_K256==len){
    return fsl_validate16(str, FSL_STRLEN_K256) ? FSL_STRLEN_K256 : 0;
  }else{
    return 0;
  }
}
int fsl_is_uuid_len(int x){
  switch(x){
    case FSL_STRLEN_SHA1:
    case FSL_STRLEN_K256:
      return x;
    default:
      return 0;
  }
}
void fsl_error_clear( fsl_error * err ){
  if(err){
    fsl_buffer_clear(&err->msg);
    *err = fsl_error_empty;
  }
}

void fsl_error_reset( fsl_error * err ){
  if(err){
    err->code = 0;
    err->msg.used = err->msg.cursor = 0;
    if(err->msg.mem) err->msg.mem[0] = 0;
  }
}


int fsl_error_copy( fsl_error const * src, fsl_error * dest ){
  if(!src || !dest || (src==dest)) return FSL_RC_MISUSE;
  else {
    int rc = 0;
    dest->msg.used = dest->msg.cursor = 0;
    dest->code = src->code;
    if(FSL_RC_OOM!=src->code){
      rc = fsl_buffer_append( &dest->msg, src->msg.mem, src->msg.used );
    }
    return rc;
  }
}

void fsl_error_move( fsl_error * lower, fsl_error * higher ){
  fsl_error const err = *lower;
  *lower = *higher;
  lower->code = 0;
  lower->msg.used = lower->msg.cursor = 0;
  *higher = err;
}

int fsl_error_setv( fsl_error * err, int code, char const * fmt,
                    va_list args ){
  if(!err) return FSL_RC_MISUSE;
  else if(!code){ /* clear error state */
    err->code = 0;
    err->msg.used = err->msg.cursor = 0;
    if(err->msg.mem){
      err->msg.mem[0] = 0;
    }
    return 0;
  }else{
    int rc = 0;
    err->msg.used = err->msg.cursor = 0;
    err->code = code;
    if(FSL_RC_OOM!=code){
      rc = fmt
        ? fsl_buffer_appendfv(&err->msg, fmt, args)
        : fsl_buffer_append(&err->msg, fsl_rc_cstr(code), -1);
      if(rc) err->code = rc;
    }
    return rc ? rc : code;
  }
}

int fsl_error_set( fsl_error * err, int code, char const * fmt,
                   ... ){
  int rc;
  va_list args;
  va_start(args,fmt);
  rc = fsl_error_setv(err, code, fmt, args);
  va_end(args);
  return rc;
}


int fsl_error_get( fsl_error const * err, char const ** str, fsl_size_t * len ){
  if(!err) return FSL_RC_MISUSE;
  else{
    if(str) *str = err->msg.used
      ? (char const *)err->msg.mem
      : NULL;
    if(len) *len = err->msg.used;
    return err->code;
  }
}


char const * fsl_rc_cstr(int rc){
  fsl_rc_e const RC = (fsl_rc_e)rc
    /* we do this so that gcc will warn if the switch() below is
       missing any fsl_rc_e entries. */
    ;
  switch(RC){
#define STR(T) case FSL_RC_##T: return "FSL_RC_" #T
    STR(ACCESS);
    STR(ALREADY_EXISTS);
    STR(AMBIGUOUS);
    STR(BREAK);
    STR(SYNTAX);
    STR(CHECKSUM_MISMATCH);
    STR(CONFLICT);
    STR(CONSISTENCY);
    STR(DB);
    STR(DELTA_INVALID_OPERATOR);
    STR(DELTA_INVALID_SEPARATOR);
    STR(DELTA_INVALID_SIZE);
    STR(DELTA_INVALID_TERMINATOR);
    STR(end);
    STR(ERROR);
    STR(IO);
    STR(MISSING_INFO);
    STR(MISUSE);
    STR(NOOP);
    STR(NOT_A_CKOUT);
    STR(NOT_A_REPO);
    STR(NOT_FOUND);
    STR(NYI);
    STR(OK);
    STR(OOM);
    STR(PHANTOM);
    STR(RANGE);
    STR(REPO_MISMATCH);
    STR(REPO_NEEDS_REBUILD);
    STR(REPO_VERSION);
    STR(SIZE_MISMATCH);
    STR(STEP_DONE);
    STR(STEP_ERROR);
    STR(STEP_ROW);
    STR(TYPE);
    STR(UNKNOWN_RESOURCE);
    STR(UNSUPPORTED);
#undef STR
  }
  return "Unknown result code";
}

char const * fsl_library_version(){
  return FSL_LIBRARY_VERSION;
}

bool fsl_library_version_matches(char const * yourLibVersion){
  return 0 == fsl_strcmp(FSL_LIBRARY_VERSION, yourLibVersion);
}

double fsl_unix_to_julian( fsl_time_t unix_ ){
  return (unix_ * 1.0 / 86400.0 ) + 2440587.5;
}

double fsl_julian_now(){
  return fsl_unix_to_julian( time(0) );
}


int fsl_strcmp(const char *zA, const char *zB){
  if( zA==0 ) return zB ? -1 : 0;
  else if( zB==0 ) return 1;
  else{
    int a, b;
    do{
      a = *zA++;
      b = *zB++;
    }while( a==b && a!=0 );
    return ((unsigned char)a) - (unsigned char)b;
  }
}


int fsl_strcmp_cmp( void const * lhs, void const * rhs ){
  return fsl_strcmp((char const *)lhs, (char const *)rhs);
}

int fsl_strncmp(const char *zA, const char *zB, fsl_size_t nByte){
  if( !zA ) return zB ? -1 : 0;
  else if( !zB ) return +1;
  else if(!nByte) return 0;
  else{
    int a, b;
    do{
      a = *zA++;
      b = *zB++;
    }while( a==b && a!=0 && (--nByte)>0 );
    return (nByte>0) ? (((unsigned char)a) - (unsigned char)b) : 0;
  }
}

  
int fsl_uuidcmp( fsl_uuid_cstr lhs, fsl_uuid_cstr rhs ){
  if(!lhs) return rhs ? -1 : 0;
  else if(!rhs) return 1;
  else if(lhs[FSL_STRLEN_SHA1] && rhs[FSL_STRLEN_SHA1]){
    return fsl_strncmp( lhs, rhs, FSL_STRLEN_K256);
  }else if(!lhs[FSL_STRLEN_SHA1] && !rhs[FSL_STRLEN_SHA1]){
    return fsl_strncmp( lhs, rhs, FSL_STRLEN_SHA1 );
  }else{
    return fsl_strcmp(lhs, rhs);
  }      
}

int fsl_strnicmp(const char *zA, const char *zB, fsl_int_t nByte){
  if( zA==0 ){
    if( zB==0 ) return 0;
    return -1;
  }else if( zB==0 ){
    return +1;
  }
  if( nByte<0 ) nByte = (fsl_int_t)fsl_strlen(zB);
  return sqlite3_strnicmp(zA, zB, nByte);
}

int fsl_stricmp(const char *zA, const char *zB){
  if( zA==0 ) return zB ? -1 : 0;
  else if( zB==0 ) return 1;
  else{
    fsl_int_t nByte;
    int rc;
    nByte = (fsl_int_t)fsl_strlen(zB);
    rc = sqlite3_strnicmp(zA, zB, nByte);
    return ( rc==0 && zA[nByte] ) ? 1 : rc;
  }
}

int fsl_stricmp_cmp( void const * lhs, void const * rhs ){
  return fsl_stricmp((char const *)lhs, (char const *)rhs);
}

fsl_size_t fsl_strlen( char const * src ){
  fsl_size_t i = 0;
  if(src) for( ; *src; ++i, ++src ){}
  return i;
}

char * fsl_strndup( char const * src, fsl_int_t len ){
  if(!src) return NULL;
  else{
    fsl_buffer b = fsl_buffer_empty;
    if(len<0) len = (fsl_int_t)fsl_strlen(src);
    fsl_buffer_append( &b, src, len );
    return (char*)b.mem;
  }
}

char * fsl_strdup( char const * src ){
  return fsl_strndup(src, -1);
}


/*
   Return TRUE if the string begins with something that looks roughly
   like an ISO date/time string.  The SQLite date/time functions will
   have the final say-so about whether or not the date/time string is
   well-formed.
*/
char fsl_str_is_date(const char *z){
  if(!z || !*z) return 0;
  else if( !fsl_isdigit(z[0]) ) return 0;
  else if( !fsl_isdigit(z[1]) ) return 0;
  else if( !fsl_isdigit(z[2]) ) return 0;
  else if( !fsl_isdigit(z[3]) ) return 0;
  else if( z[4]!='-') return 0;
  else if( !fsl_isdigit(z[5]) ) return 0;
  else if( !fsl_isdigit(z[6]) ) return 0;
  else if( z[7]!='-') return 0;
  else if( !fsl_isdigit(z[8]) ) return 0;
  else if( !fsl_isdigit(z[9]) ) return 0;
  else return 1;
}

int fsl_str_is_date2(const char *z){
  int rc = -1;
  int pos = 0;
  if(!z || !*z) return 0;
  else if( !fsl_isdigit(z[pos++]) ) return 0;
  else if( !fsl_isdigit(z[pos++]) ) return 0;
  else if( !fsl_isdigit(z[pos++]) ) return 0;
  else if( !fsl_isdigit(z[pos++]) ) return 0;
  else if( z[pos]=='-') ++pos;
  else{
    if(fsl_isdigit(z[pos++]) && '-'==z[pos++]){
      rc = 1;
    }else{
      return 0;
    }
  }
  if( !fsl_isdigit(z[pos++]) ) return 0;
  else if( !fsl_isdigit(z[pos++]) ) return 0;
  else if( z[pos++]!='-') return 0;
  else if( !fsl_isdigit(z[pos++]) ) return 0;
  else if( !fsl_isdigit(z[pos++]) ) return 0;
  assert(10==pos || 11==pos);
  return rc;
}

bool fsl_str_bool( char const * s ){
  switch(s ? *s : 0){
    case 0: case '0':
    case 'f': case 'F': // "false"
    case 'n': case 'N': // "no"
      return false;
    case '1':
    case 't': case 'T': // "true"
    case 'y': case 'Y': // "yes"
      return true;
    default: {
      char buf[5] = {0,0,0,0,0};
      int i;
      for( i = 0; (i<5) && *s; ++i, ++s ){
        buf[i] = fsl_tolower(*s);
      }
      if(0==fsl_strncmp(buf, "off", 3)) return false;
      return true;
    }
  }
}

char * fsl_guess_user_name(){
  char const ** e;
  static char const * list[] = {
  "FOSSIL_USER",
#if defined(_WIN32)
  "USERNAME",
#else
  "USER",
  "LOGNAME",
#endif
  NULL /* sentinel */
  };
  char * rv = NULL;
  for( e = list; *e; ++e ){
    rv = fsl_getenv(*e);
    if(rv){
      /*
        Because fsl_getenv() has the odd requirement of needing
        fsl_filename_free(), and we want strings returned from this
        function to be safe for passing to fsl_free(), we have to dupe
        the string. We "could" block this off to happen only on the
        platforms for which fsl_getenv() requires an extra encoding
        step, but that would likely eventually lead to a bug.
      */
      char * kludge = fsl_strdup(rv);
      fsl_filename_free(rv);
      rv = kludge;
      break;
    }
  }
  return rv;
}

void fsl_fatal( int code, char const * fmt, ... ){
  static bool inFatal = false;
  if(inFatal){
    /* This can only happen if the fsl_appendv() bits
       call this AND trigger it via fsl_fprintf() below,
       neither of which is currently the case.
    */
    assert(!"fsl_fatal() called recursively.");
    abort();
  }else{
    va_list args;
    inFatal = true;
    fsl_fprintf(stderr, "FATAL ERROR: code=%d (%s)\n",
                code, fsl_rc_cstr(code));
    if(fmt){
      va_start(args,fmt);
      fsl_fprintfv(stderr, fmt, args);
      va_end(args);
      fwrite("\n", 1, 1, stderr);
    }
    exit(EXIT_FAILURE);
  }
}

#if 0
char * fsl_unix_to_iso8601( fsl_time_t u ){
  enum { BufSize = 20 };
  char buf[BufSize]= {0,};
  time_t const tt = (time_t)u;
  fsl_strftime( buf, BufSize, "%Y-%m-%dT%H:%M:%S", gmtime(&tt) );
  return fsl_strdup(buf);
}
#endif


char fsl_iso8601_to_julian( char const * zDate, double * out ){
  /* Adapted from this article:

     https://quasar.as.utexas.edu/BillInfo/JulianDatesG.html
  */
  char const * p = zDate;
  int y = 0, m = 0, d = 0;
  int h = 0, mi = 0, s = 0, f = 0;
  double j = 0;
  if(!zDate || !*zDate){
    return 0;
  }
#define DIG(NUM) if(!fsl_isdigit(*p)) return 0; \
  NUM=(NUM*10)+(*(p++)-'0')

  DIG(y);DIG(y);DIG(y);DIG(y);
  if('-'!=*p++) return 0;
  DIG(m);DIG(m);
  if('-'!=*p++) return 0;
  DIG(d);DIG(d);
  if('T' != *p++) return 0;
  DIG(h);DIG(h);
  if(':'!=*p++) return 0;
  DIG(mi);DIG(mi);
  if(':'!=*p++) return 0;
  DIG(s);DIG(s);
  if('.'==*p++){
    DIG(f);DIG(f);DIG(f);
  }
  if(out){
    typedef int64_t TI;
    TI A, B, C, E, F;
    if(m<3){
      --y;
      m += 12;
    }
    A = y/100;
    B = A/4;
    C = 2-A+B;
    E = (TI)(365.25*(y+4716));
    F = (TI)(30.6001*(m+1));
    j = C + d + E + F - 1524.5;
    j += ((1.0*h)/24) + ((1.0*mi)/1440) + ((1.0*s)/86400);
    if(0 != f){
      j += (1.0*f)/86400000;
    }
    *out = j;
  }
  return 1;
#undef DIG
}

fsl_time_t fsl_julian_to_unix( double JD ){
  return (fsl_time_t) ((JD - 2440587.5) * 86400);
}

char fsl_julian_to_iso8601( double J, char * out, bool addMs ){
  /* Adapted from this article:

     https://quasar.as.utexas.edu/BillInfo/JulianDatesG.html
  */
  typedef int64_t TI;
  int Y, M, D, H, MI, S, F;
  TI ms;
  char * z = out;
  if(!out || (J<=0)) return 0;
  else{
    double Z;
    TI W, X;
    TI A, B;
    TI C, DD, E, F;

    Z = J + 0.5;
    W = (TI)((Z-1867216.25)/36524.25);
    X = W/4;
    A = (TI)(Z+1+W-X);
    B = A+1524;
    C = (TI)((B-122.1)/365.25);
    DD = (TI)(365.25 * C);
    E = (TI)((B-DD)/30.6001);
    F = (TI)(30.6001 * E);
    D = (int)(B - DD - F);
    M = (E<=13) ? (E-1) : (E-13);
    Y = (M<3) ? (C-4715) : (C-4716);
  }

  if(Y<0 || Y>9999) return 0;
  else if(M<1 || M>12) return 0;
  else if(D<1 || D>31) return 0;

  ms = (TI)((J-(TI)J) * 86400001.0)
    /* number of milliseconds in the fraction part of the JDay. The
       non-0 at the end works around a problem where SS.000 converts
       to (SS-1).999. This will only hide the bug for the cases i've
       seen it, and might introduce other inaccuracies
       elsewhere. Testing it against the current libfossil event table
       produces good results - at most a 1ms round-trip fidelity loss
       for the (currently ~1157) records being checked. The suffix of
       1.0 was found to be a decent value via much testing with the
       libfossil and fossil(1) source repos.
    */;

  if( (H = ms / 3600000) ){
    ms -= H * 3600000;
    H = (H + 12) % 24;
  }else{
    H = 12 /* astronomers start their day at noon. */;
  }
  if( (MI = ms / 60000) ) ms -= MI * 60000;
  if( (S = ms / 1000) ) ms -= S * 1000;
  assert(ms<1000);
  F = (int)(ms);

  assert(H>=0 && H<24);
  assert(MI>=0 && MI<60);
  assert(S>=0 && S<60);
  assert(F>=0 && F<1000);

  if(H<0 || H>23) return 0;
  else if(MI<0 || MI>59) return 0;
  else if(S<0 || S>59) return 0;
  else if(F<0 || F>999) return 0;
#define UGLY_999_KLUDGE 1
  /* The fossil(1) repo has 27 of 10041 records which exhibit the
     SS.999 behaviour commented on above. With this kludge, that
     number drops to 0. But it's still an ugly, ugly kludge.
     OTOH, the chance of the .999 being correct is 1 in 1000,
     whereas we see "correct" behaviour more often (2.7 in 1000)
     with this workaround.
   */
#if UGLY_999_KLUDGE
  if(999==F){
    char oflow = 0;
    int s2 = S, mi2 = MI, h2 = H;
    if(++s2 == 60){ /* Overflow minute */
      s2 = 0;
      if(++mi2 == 60){ /* Overflow hour */
        mi2 = 0;
        if(++h2 == 24){ /* Overflow day */
          /* leave this corner-corner case in place */
          oflow = 1;
        }
      }
    }
    /* MARKER(("UGLY 999 KLUDGE (A): H=%d MI=%d S=%d F=%d\n", H, MI, S, F)); */
    if(!oflow){
      F = 0;
      S = s2;
      MI = mi2;
      H = h2;
      /* MARKER(("UGLY 999 KLUDGE (B): H=%d MI=%d S=%d F=%d\n", H, MI, S, F)); */
    }
  }
#endif
#undef UGLY_999_KLUDGE
  *(z++) = '0'+(Y/1000);
  *(z++) = '0'+(Y%1000/100);
  *(z++) = '0'+(Y%100/10);
  *(z++) = '0'+(Y%10);
  *(z++) = '-';
  *(z++) = '0'+(M/10);
  *(z++) = '0'+(M%10);
  *(z++) = '-';
  *(z++) = '0'+(D/10);
  *(z++) = '0'+(D%10);
  *(z++) = 'T';
  *(z++) = '0'+(H/10);
  *(z++) = '0'+(H%10);
  *(z++) = ':';
  *(z++) = '0'+(MI/10);
  *(z++) = '0'+(MI%10);
  *(z++) = ':';
  *(z++) = '0'+(S/10);
  *(z++) = '0'+(S%10);
  if(addMs){
    *(z++) = '.';
    *(z++) = '0'+(F%1000/100);
    *(z++) = '0'+(F%100/10);
    *(z++) = '0'+(F%10);
  }
  *z = 0;
  return 1;
}

#if FSL_CONFIG_ENABLE_TIMER
/**
   For the fsl_timer_xxx() family of functions...
*/
#ifdef _WIN32
# include <windows.h>
#else
# include <sys/time.h>
# include <sys/resource.h>
# include <unistd.h>
# include <fcntl.h>
# include <errno.h>
#endif
#endif
/* FSL_CONFIG_ENABLE_TIMER */

/**
   Get user and kernel times in microseconds.
*/
static void fsl_cpu_times(uint64_t *piUser, uint64_t *piKernel){
#if !FSL_CONFIG_ENABLE_TIMER
  if(piUser) *piUser = 0U;
  if(piKernel) *piKernel = 0U;
#else
#ifdef _WIN32
  FILETIME not_used;
  FILETIME kernel_time;
  FILETIME user_time;
  GetProcessTimes(GetCurrentProcess(), &not_used, &not_used,
                  &kernel_time, &user_time);
  if( piUser ){
     *piUser = ((((uint64_t)user_time.dwHighDateTime)<<32) +
                         (uint64_t)user_time.dwLowDateTime + 5)/10;
  }
  if( piKernel ){
     *piKernel = ((((uint64_t)kernel_time.dwHighDateTime)<<32) +
                         (uint64_t)kernel_time.dwLowDateTime + 5)/10;
  }
#else
  struct rusage s;
  getrusage(RUSAGE_SELF, &s);
  if( piUser ){
    *piUser = ((uint64_t)s.ru_utime.tv_sec)*1000000 + s.ru_utime.tv_usec;
  }
  if( piKernel ){
    *piKernel =
              ((uint64_t)s.ru_stime.tv_sec)*1000000 + s.ru_stime.tv_usec;
  }
#endif
#endif
/* FSL_CONFIG_ENABLE_TIMER */
}


void fsl_timer_start(fsl_timer_state * ft){
  fsl_cpu_times( &ft->user, &ft->system );
}

uint64_t fsl_timer_fetch(fsl_timer_state const * t){
  uint64_t eu = 0, es = 0;
  fsl_cpu_times( &eu, &es );
  return (eu - t->user) + (es - t->system);
}

uint64_t fsl_timer_reset(fsl_timer_state * t){
  uint64_t const rc = fsl_timer_fetch(t);
  fsl_cpu_times( &t->user, &t->system );
  return rc;
}

uint64_t fsl_timer_stop(fsl_timer_state *t){
  uint64_t const rc = fsl_timer_fetch(t);
  *t = fsl_timer_state_empty;
  return rc;
}

unsigned int fsl_rgb_encode( int r, int g, int b ){
  return (unsigned int)(((r&0xFF)<<16) + ((g&0xFF)<<8) + (b&0xFF));
}

void fsl_rgb_decode( unsigned int src, int *r, int *g, int *b ){
  if(r) *r = (src&0xFF0000)>>16;
  if(g) *g = (src&0xFF00)>>8;
  if(b) *b = src&0xFF;
}

unsigned fsl_gradient_color(unsigned c1, unsigned c2, unsigned int n, unsigned int i){
  unsigned c;   /* Result color */
  unsigned x1, x2;
  if( i==0 || n==0 ) return c1;
  else if(i>=n) return c2;
  x1 = (c1>>16)&0xff;
  x2 = (c2>>16)&0xff;
  c = (x1*(n-i) + x2*i)/n<<16 & 0xff0000;
  x1 = (c1>>8)&0xff;
  x2 = (c2>>8)&0xff;
  c |= (x1*(n-i) + x2*i)/n<<8 & 0xff00;
  x1 = c1&0xff;
  x2 = c2&0xff;
  c |= (x1*(n-i) + x2*i)/n & 0xff;
  return c;
}


fsl_size_t fsl_simplify_sql( char * sql, fsl_int_t len ){
  char * wat = sql /* write pos */;
  char * rat = sql /* read pos */;
  char const * end /* one-past-the-end */;
  char inStr = 0 /* in an SQL string? */;
  char prev = 0 /* previous character. Sometimes. */;
  if(!sql || !*sql) return 0;
  else if(len < 0) len = fsl_strlen(sql);
  if(!len) return 0;
  end = sql + len;
  while( *rat && (rat < end) ){
    switch(*rat){
      case 0: break;
      case '\r':
      case '\n':
        /* Bug: we don't handle \r\n pairs. Because nobody
           should never have to :/. */
        if(inStr || (prev!=*rat)){
          /* Keep them as-is */
          prev = *wat++ = *rat++;
        }else{
          /* Collapse multiples into one. */
          ++rat;
        }
        continue;
      case ' ':
      case '\t':
      case '\v':
      case '\f':
        if(inStr){
          /* Keep them as-is */
          prev = *wat++ = *rat++;
        }else{
          /* Reduce to a single space. */
          /* f_out("prev=[%c] rat=[%c]\n", prev, *rat); */
          if(prev != *rat){
            *wat++ = ' ';
            prev = *rat;
          }
          ++rat;
        }
        continue;
      case '\'': /* SQL strings */
        prev = *wat++ = *rat++;
        if(!inStr){
          inStr = 1;
        }else if('\'' == *rat){
          /* Escaped quote */
          *wat++ = *rat++;
        }else{
          /* End of '...' string. */
          inStr = 0;
        }
        continue;
      default:
        prev = *wat++ = *rat++;
        continue;
    }
  }
  *wat = 0;
  return (fsl_size_t)(wat - sql);
}

/**
   Convenience form of fsl_simplify_sql() which assumes b holds an SQL
   string. It gets processed by fsl_simplify_sql() and its 'used'
   length potentially gets adjusted to match the adjusted SQL string.
*/
fsl_size_t fsl_simplify_sql_buffer( fsl_buffer * b ){
  return b->used = fsl_simplify_sql( (char *)b->mem, (fsl_int_t)b->used );
}

char const *fsl_preferred_ckout_db_name(){
#if FSL_PLATFORM_IS_WINDOWS
  return "_FOSSIL_";
#else
  return ".fslckout";
#endif
}

char fsl_isatty(int fd){
  return isatty(fd) ? 1 : 0;
}

bool fsl_is_reserved_fn_windows(const char *zPath, fsl_int_t nameLen){
  static const char *const azRes[] = {
    "CON", "PRN", "AUX", "NUL", "COM", "LPT"
  };
  unsigned int i;
  char const * zEnd;
  if(nameLen<0) nameLen = (fsl_int_t)fsl_strlen(zPath);
  zEnd = zPath + nameLen;
  while( zPath < zEnd ){
    for(i=0; i<sizeof(azRes)/sizeof(azRes[0]); ++i){
      if( fsl_strnicmp(zPath, azRes[i], 3)==0
       && ((i>=4 && fsl_isdigit(zPath[3])
                 && (zPath[4]=='/' || zPath[4]=='.' || zPath[4]==0))
          || (i<4 && (zPath[3]=='/' || zPath[3]=='.' || zPath[3]==0)))
      ){
        return true;
      }
    }
    while( zPath<zEnd && zPath[0]!='/' ) ++zPath;
    while( zPath<zEnd && zPath[0]=='/' ) ++zPath;
  }
  return false;
}

bool fsl_is_reserved_fn(const char *zFilename, fsl_int_t nameLen){
  fsl_size_t nFilename = nameLen>=0
    ? (fsl_size_t)nameLen : fsl_strlen(zFilename);
  char const * zEnd;
  int gotSuffix = 0;
  assert( zFilename && "API misuse" );
#if FSL_PLATFORM_IS_WINDOWS // || 1
  if(nFilename>2 && fsl_is_reserved_fn_windows(zFilename, nameLen)){
    return true;
  }
#endif
  if( nFilename<8 ) return false; /* strlen("_FOSSIL_") */
  zEnd = zFilename + nFilename;
  if( nFilename>=12 ){ /* strlen("_FOSSIL_-(shm|wal)") */
    /* Check for (-wal, -shm, -journal) suffixes, with an eye towards
    ** runtime speed. */
    if( zEnd[-4]=='-' ){
      if( fsl_strnicmp("wal", &zEnd[-3], 3)
       && fsl_strnicmp("shm", &zEnd[-3], 3) ){
        return false;
      }
      gotSuffix = 4;
    }else if( nFilename>=16 && zEnd[-8]=='-' ){ /*strlen(_FOSSIL_-journal) */
      if( fsl_strnicmp("journal", &zEnd[-7], 7) ) return false;
      gotSuffix = 8;
    }
    if( gotSuffix ){
      assert( 4==gotSuffix || 8==gotSuffix );
      zEnd -= gotSuffix;
      nFilename -= gotSuffix;
      gotSuffix = 1;
    }
    assert( nFilename>=8 && "strlen(_FOSSIL_)" );
    assert( gotSuffix==0 || gotSuffix==1 );
  }
  switch( zEnd[-1] ){
    case '_':{
      if( fsl_strnicmp("_FOSSIL_", &zEnd[-8], 8) ) return false;
      if( 8==nFilename ) return true;
      return zEnd[-9]=='/' ? true : !!gotSuffix;
    }
    case 'T':
    case 't':{
      if( nFilename<9 || zEnd[-9]!='.'
       || fsl_strnicmp(".fslckout", &zEnd[-9], 9) ){
        return false;
      }
      if( 9==nFilename ) return true;
      return zEnd[-10]=='/' ? true : !!gotSuffix;
    }
    default:{
      return false;
    }
  }
}

#undef MARKER
#if defined(_WIN32) || defined(WIN32)
#undef isatty
#endif
