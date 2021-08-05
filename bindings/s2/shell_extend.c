/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
/**
   This file is meant to act as a client-side way to extend the default
   s2 shell app. It demonstrates examples of connecting C code to
   the s2 scripting environment.

   If shell.c is built with S2_SHELL_EXTEND defined, then it will assume
   this function will be linked in by the client:

   int s2_shell_extend(s2_engine * se, int argc, char const * const * argv);

   When built with the macro S2_SHELL_EXTEND, it calls that function right
   after it installs its own core functionality.
*/
#include <assert.h>
#if defined(S2_AMALGAMATION_BUILD)
#  include "s2_amalgamation.h"
#else
#  include "s2.h"
#endif
#include "fossil-scm/fossil.h"

#ifdef S2_OS_UNIX
# include <unistd.h> /* F_OK and friends */
#elif defined(S2_OS_WINDOWS)
# include <windows.h>
#endif

/* Only for debuggering... */
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)


/**
   Type IDs for type-safely mapping (void*) to cwal_native
   instances. Their types and values are irrelevant - we use only their
   addresses.
*/
static const int cwal_type_id_fsl_cx = 0;
static const int cwal_type_id_fsl_db = 0;
static const int cwal_type_id_fsl_stmt = 0;
#define FSL_TYPEID(X) (&cwal_type_id_##X)

#define VERBOSE_FINALIZERS 0

static void cwal_finalizer_f_fsl_db( cwal_engine * e, void * m ){
  if(m){
#if VERBOSE_FINALIZERS
    MARKER(("Finalizing fsl_db @%p\n", m));
#endif
    fsl_db_close( (fsl_db*)m );
  }
}

void cwal_finalizer_f_fsl_stmt( cwal_engine * e, void * m ){
  if(m){
#if VERBOSE_FINALIZERS
    MARKER(("Finalizing fsl_stmt @%p\n", m));
#endif
    fsl_stmt_finalize( (fsl_stmt*)m );
  }
}

static void cwal_finalizer_f_fsl_cx( cwal_engine * e, void * m ){
  if(m){
#if VERBOSE_FINALIZERS
    MARKER(("Finalizing fsl_cx @%p\n", m));
#endif
    fsl_cx_finalize( (fsl_cx*)m );
  }
}

#undef VERBOSE_FINALIZERS

static int cb_toss( cwal_callback_args const * args, int code,
                    char const * fmt, ... ){
  int rc;
  va_list vargs;
  s2_engine * se = s2_engine_from_args(args);
  cwal_native * nat = cwal_value_native_part(args->engine, args->self,
                                             FSL_TYPEID(fsl_cx) );
  cwal_value * natV = nat ? cwal_native_value(nat) : NULL;
  fsl_cx * f = natV ? cwal_native_get( nat, FSL_TYPEID(fsl_cx) ) : NULL;
  assert(se);
  va_start(vargs,fmt);
  rc = cwal_exception_setfv(args->engine, code, fmt, vargs );
  if(f){
    /* Ensure that script-triggered errors do not unduly
       lie around, potentially propagating long after
       they are valid.
    */
    fsl_cx_err_reset(f);
  }
  va_end(vargs);
  return rc;
}

static int cb_toss_fsl( cwal_callback_args const * args,
                        fsl_cx * f ){
  int rc;
  assert(f && f->error.code);
  rc = (FSL_RC_OOM==f->error.code)
    ? CWAL_RC_OOM
    : cb_toss(args,
              f->error.code,
              "%.*s", (int)f->error.msg.used,
              (char const *)f->error.msg.mem );
  fsl_cx_err_reset(f);
  return rc;
}

static int cb_toss_db( cwal_callback_args const * args,
                       fsl_db * db ){
  int rc;
  assert(db && db->error.code);
  rc = (FSL_RC_OOM==db->error.code)
    ? CWAL_RC_OOM
    : cb_toss(args, db->error.code,
              "%s", (char const *)db->error.msg.mem );
  fsl_db_err_reset(db);
  return rc;
}

static cwal_value * fsl_cx_prototype( s2_engine * se );
static cwal_value * fsl_db_prototype( s2_engine * se );
static cwal_value * fsl_stmt_prototype( s2_engine * se );

/*
** Copies fb's state into cb.
*/
static int fsl_buffer_to_cwal_buffer( cwal_engine * e,
                                      fsl_buffer const * fb,
                                      cwal_buffer * cb ){
  cb->used = 0;
  return cwal_buffer_append(e, cb, fb->mem, (cwal_size_t)fb->used);
}

#if 0
/*
** Shallowly copies cb's state into fb. fb MUST NOT be modified
** via the fsl allocator while this is in place, because cwal's
** allocator is not guaranteed to be compatible (and won't be
** if certain memory capping options are enabled).
*/
static void cwal_buffer_to_fsl_buffer( cwal_buffer const * cb,
                                       fsl_buffer * fb ){
  fb->mem  = cb->mem;
  fb->capacity = cb->capacity;
  fb->used = cb->used;
  fb->cursor = 0;
}
#endif

/**
   Script usage:

   const sha1 = bufferInstance.sha1()
*/
static int cb_buffer_sha1_self( cwal_callback_args const * args,
                                cwal_value **rv ){
  cwal_buffer * buf = cwal_value_buffer_part(args->engine, args->self);
  char * sha;
  if(!buf){
    return cb_toss(args, CWAL_RC_TYPE,
                   "'this' is-not-a Buffer.");
  }
  sha = fsl_sha1sum_cstr( (char const*)buf->mem, (int)buf->used);
  if(!sha) return CWAL_RC_OOM;
  assert(sha[0] && sha[FSL_STRLEN_SHA1-1] && !sha[FSL_STRLEN_SHA1]);
  *rv = cwal_new_string_value(args->engine, sha, FSL_STRLEN_SHA1);
  /* Reminder: can't use zstring here b/c of potentially different
     allocators. */
  fsl_free(sha);
  if(*rv) return 0;
  else{
    return CWAL_RC_OOM;
  }
}
static int cb_buffer_sha3_self( cwal_callback_args const * args,
                                cwal_value **rv ){
  cwal_buffer * buf = cwal_value_buffer_part(args->engine, args->self);
  char * sha;
  if(!buf){
    return cb_toss(args, CWAL_RC_TYPE,
                   "'this' is-not-a Buffer.");
  }
  sha = fsl_sha3sum_cstr( (char const*)buf->mem, (int)buf->used);
  if(!sha) return CWAL_RC_OOM;
  assert(sha[0] && sha[FSL_STRLEN_K256-1] && !sha[FSL_STRLEN_K256]);
  *rv = cwal_new_string_value(args->engine, sha, FSL_STRLEN_K256);
  /* Reminder: can't use zstring here b/c of potentially different
     allocators. */
  fsl_free(sha);
  if(*rv) return 0;
  else{
    return CWAL_RC_OOM;
  }
}

/**
   Script usage:

   const md5 = bufferInstance.md5()
*/
static int cb_buffer_md5_self( cwal_callback_args const * args,
                               cwal_value **rv ){
  cwal_buffer * buf = cwal_value_buffer_part(args->engine, args->self);
  char * hash;
  if(!buf){
    return cb_toss(args, CWAL_RC_TYPE,
                   "'this' is-not-a Buffer.");
  }
  hash = fsl_md5sum_cstr( (char const*)buf->mem, (int)buf->used);
  if(!hash) return CWAL_RC_OOM;
  assert(hash[0] && hash[FSL_STRLEN_MD5-1] && !hash[FSL_STRLEN_MD5]);
  *rv = cwal_new_string_value(args->engine, hash, FSL_STRLEN_MD5);
  fsl_free(hash);
  if(*rv) return 0;
  else{
    return CWAL_RC_OOM;
  }
}

/*
** fsl_output_f() impl which forwards to cwal_output().
** state must be a (cwal_engine *). Reminder: must return
** a FSL_RC_xxx value, not CWAL_RC_xxx.
*/
static int fsl_output_f_cwal_output( void * state,
                                     void const * src,
                                     fsl_size_t n ){
  cwal_engine * e = (cwal_engine *)state;
  return cwal_output( e, src, (cwal_size_t)n )
    ? FSL_RC_IO
    : 0;
}

/*
** fsl_flush_f() impl which forwards to cwal_output_flush(). state
** must be a (cwal_engine *).
*/
static int fsl_flush_f_cwal_out( void * state ){
  cwal_engine * e = (cwal_engine *)state;
  return cwal_output_flush(e);
}

static int s2_fsl_openmode_to_flags(char const * openMode, int startFlags){
  int openFlags = startFlags;
  for( ; *openMode; ++openMode ){
    switch(*openMode){
      case 'r': openFlags |= FSL_OPEN_F_RO;
        break;
      case 'w': openFlags |= FSL_OPEN_F_RW;
        break;
      case 'c': openFlags |= FSL_OPEN_F_CREATE;
        break;
      case 'v': openFlags |= FSL_OPEN_F_SCHEMA_VALIDATE;
        break;
      case 'T': openFlags |= FSL_OPEN_F_TRACE_SQL;
        break;
      default:
        break;
    }
  }
  return openFlags;
}

/*
** Searches v and its prototype chain for a fsl_cx binding. If found,
** it is returned, else NULL is returned.
*/
static fsl_cx * cwal_value_fsl_cx_part( cwal_engine * e,
                                        cwal_value * v ){
  if(!v) return NULL;
  else {
    cwal_native * nv;
    fsl_cx * f;
    while(v){
      nv = cwal_value_get_native(v);
      f = nv
        ? (fsl_cx *)cwal_native_get( nv, FSL_TYPEID(fsl_cx) )
        : NULL;
      if(f) return f;
      else v = cwal_value_prototype_get(e, v);
    } while(v);
    return NULL;
  }
}


/*
** Creates a new cwal_value (cwal_native) wrapper for the given fsl_db
** instance. If addDtor is true then a finalizer is installed for the
** instance, else it is assumed to live native-side and gets no
** destructor installed (in which case we will eventually have a problem
** when such a db is destroyed outside of the script API, unless we
** rewrite these to use weak references instead, but that might require
** one more level of struct indirection). The role of the db
** is important so that we can get the proper filename. A role of
** FSL_DBROLE_NONE should be used for non-fossil-core db handles.
**
** On success, returns 0 and assigns *rv to the new Db value.
**
** If 0!=db->filename.used then the new value gets a 'name' property
** set to the contents of db->filename.
**
** Maintenance reminder: this routine must return cwal_rc_t codes, not
** fsl_rc_e codes.
**
** TODO? Wrap up cwal_weak_ref of db handle, instead of the db handle
** itself? We only need this if Fossil instances will have their DB
** instances manipulated from outside of script-space.
*/
static int fsl_db_new_native( s2_engine * se, fsl_cx * f,
                              fsl_db * db, char addDtor,
                              cwal_value **rv,
                              fsl_dbrole_e role){
  cwal_native * n;
  cwal_value * nv;
  int rc = 0;
  char const * fname = NULL;
  fsl_size_t nameLen = 0;
  cwal_value * tmpV = NULL;
  assert(se && db && rv);
  n = cwal_new_native(se->e, db,
                      addDtor ? cwal_finalizer_f_fsl_db : NULL,
                      FSL_TYPEID(fsl_db));
  if(!n) return CWAL_RC_OOM;
  nv = cwal_native_value(n);
  cwal_value_ref(nv);
  /* Set up  "filename" property. It's problematic because
     of libfossil's internal DB juggling :/.
  */
  if(f) fname = fsl_cx_db_file_for_role(f, role, &nameLen);
  if(!fname){
    fname = fsl_db_filename(db, &nameLen);
  }
  if(fname){
    tmpV = cwal_new_string_value(se->e, fname, (cwal_size_t)nameLen);
    cwal_value_ref(tmpV);
    rc = tmpV
      ? cwal_prop_set(nv, "filename", 8, tmpV)
      : CWAL_RC_OOM;
    cwal_value_unref(tmpV);
    tmpV = 0;
    if(rc){
      goto end;
    }
    fname = 0;
  }

  /* Set up  "name" property. */
  if(f) fname = fsl_cx_db_name_for_role(f, role, &nameLen);
  if(!fname){
    fname = fsl_db_name(db);
    nameLen = fname ? cwal_strlen(fname) : 0;
  }
  if(fname){
    tmpV =
      cwal_new_string_value(se->e, fname, (cwal_size_t)nameLen);
    cwal_value_ref(tmpV);
    rc = tmpV
      ? cwal_prop_set(nv, "name", 4, tmpV)
      : CWAL_RC_OOM;
    cwal_value_unref(tmpV);
    tmpV = NULL;
    if(rc){
      goto end;
    }
  }
  end:
  if(!rc){
    *rv = nv;
    cwal_value_unhand(nv);
    cwal_value_prototype_set( nv, fsl_db_prototype(se) );
  }else{
    /*
      Achtung: if addDtor then on error the dtor will be called
      here.
    */
    cwal_value_unref(nv);
  }
  return rc;
}


/* TODO: fix corner case: inheritence via multiple levels of Native
   types will break this. */
#define THIS_DB s2_engine * se = s2_engine_from_args(args);             \
  cwal_native * nat = cwal_value_native_part(args->engine, args->self, FSL_TYPEID(fsl_db)); \
  cwal_value * natV = nat ? cwal_native_value(nat) : NULL;              \
  fsl_db * db = natV ? (fsl_db*)cwal_native_get( nat, FSL_TYPEID(fsl_db) ) : NULL; \
  assert(se);          \
  if(!se){/*potentially unused var*/} \
  if(!db){ return cb_toss(args, FSL_RC_TYPE, \
                          "'this' is not (or is no longer) "            \
                          "a Db instance."); } (void)0

#define THIS_STMT s2_engine * se = s2_engine_from_args(args);           \
  cwal_native * nat = cwal_value_native_part(args->engine, args->self,FSL_TYPEID(fsl_stmt)); \
  cwal_value * natV = nat ? cwal_native_value(nat) : NULL;              \
  fsl_stmt * stmt = natV ? (fsl_stmt*)cwal_native_get( nat, FSL_TYPEID(fsl_stmt) ) : NULL; \
  if(!se){/*potentially unused var*/} \
  assert(se);          \
  if(!stmt){ return cb_toss(args, FSL_RC_TYPE, \
                            "'this' is not (or is no longer) "          \
                            "a Stmt instance."); } (void)0

#define THIS_F s2_engine * se = s2_engine_from_args(args); \
  cwal_native * nat = cwal_value_native_part(args->engine, args->self,FSL_TYPEID(fsl_cx)); \
  cwal_value * natV = nat ? cwal_native_value(nat) : NULL;                   \
  fsl_cx * f = natV ? (fsl_cx*)cwal_native_get( nat, FSL_TYPEID(fsl_cx) ) : NULL; \
  if(!se){/*potentially unused var*/} \
  assert(se); \
  if(!f){ return cb_toss(args, FSL_RC_TYPE, \
                         "'this' is not (or is no longer) "         \
                         "a Fossil Context instance."); } (void)0


static int cb_fsl_db_finalize( cwal_callback_args const * args,
                               cwal_value **rv ){
  THIS_DB;
  cwal_native_clear( nat, 1 );
  return 0;
}

static int cb_fsl_stmt_finalize( cwal_callback_args const * args,
                                 cwal_value **rv ){
  THIS_STMT;
  cwal_native_clear( nat, 1 );
  return 0;
}


int cb_fsl_db_ctor( cwal_callback_args const * args,
                    cwal_value **rv ){
  cwal_value * v = NULL;
  int rc;
  s2_engine * se = s2_engine_from_args(args);
  char const * fn;
  char const * openMode;
  cwal_size_t fnLen = 0;
  int openFlags = 0;
  fsl_db * dbP = 0;
  fsl_cx * f = cwal_value_fsl_cx_part(args->engine, args->self)
    /* It's okay if this is NULL. It will only be set when we
       aFossilContext.dbOpen() is called.
    */;
  assert(se);
  fn = args->argc
    ? cwal_value_get_cstr(args->argv[0], &fnLen)
    : NULL;
  if(!fn){
    return cb_toss(args, FSL_RC_MISUSE,
                   "Expecting a string (db file name) argument.");
  }
  openMode = (args->argc>1)
    ? cwal_value_get_cstr(args->argv[1], NULL)
    : NULL;
  if(!openMode){
    openFlags = FSL_OPEN_F_RWC;
  }else{
    openFlags = s2_fsl_openmode_to_flags( openMode, openFlags );
  }
  dbP = fsl_db_malloc();
  if(!dbP) return CWAL_RC_OOM;
  dbP->f = f;
  rc = fsl_db_open( dbP, fn, openFlags );
  if(rc){
    if(dbP->error.msg.used){
      rc = cb_toss(args, FSL_RC_ERROR,
                   "Db open failed: code #%d: %s",
                   dbP->error.code,
                   (char const *)dbP->error.msg.mem);
    }else{
      rc = cb_toss(args, FSL_RC_ERROR,
                   "Db open failed "
                   "with code #%d (%s).", rc,
                   fsl_rc_cstr(rc));
    }
    fsl_db_close(dbP);
  }
  else{
    rc = fsl_db_new_native(se, f, dbP, 1, &v, FSL_DBROLE_NONE);
    if(rc){
      fsl_db_close(dbP);
      dbP = NULL;
      assert(!v);
    }
  }
  if(rc){
    assert(!v);
  }else{
    assert(v);
    *rv  = v;
  }
  return rc;
}


/**
   Returns a new cwal_array value containing the result column names
   of the given statement . Returns NULL on error, else an array value.
*/
static cwal_value * s2_fsl_stmt_col_names( cwal_engine * e,
                                           fsl_stmt * st ){
  cwal_value * aryV = NULL;
  cwal_array * ary = NULL;
  char const * colName = NULL;
  int i = 0;
  int rc = 0;
  cwal_value * newVal = NULL;
  assert(st);
  if( ! st->colCount ) return NULL;
  ary = cwal_new_array(e);
  if( ! ary ) return NULL;
  rc = cwal_array_reserve(ary, (cwal_size_t)st->colCount);
  if(rc) return NULL;
  aryV = cwal_array_value(ary);
  assert(ary);
  for( i = 0; (0==rc) && (i < st->colCount); ++i ){
    colName = fsl_stmt_col_name(st, i);
    if( ! colName ) rc = CWAL_RC_OOM;
    else{
      newVal = cwal_new_string_value(e, colName,
                                     cwal_strlen(colName));
      if( NULL == newVal ){
        rc = CWAL_RC_OOM;
      }
      else{
        rc = cwal_array_set( ary, i, newVal );
        if( rc ) cwal_value_unref( newVal );
      }
    }
  }
  if( 0 == rc ) return aryV;
  else{
    cwal_value_unref(aryV);
    return NULL;
  }
}

static int s2_fsl_setup_new_stmt(s2_engine * se,
                                 cwal_value * nv,
                                 fsl_stmt * st){
  int rc;
  cwal_value * v;
  cwal_engine * e = se->e;
#define STMT_INCLUDE_SQL 0
#if STMT_INCLUDE_SQL
  fsl_size_t sqlLen = 0;
  char const * sql;
#endif
#define VCHECK if(!v){ rc = CWAL_RC_OOM; goto end; } (void)0
#define SET(K) VCHECK; rc = cwal_prop_set(nv, K, cwal_strlen(K), v); \
  if(rc) goto end;
#if STMT_INCLUDE_SQL
  /* Too costly, and so far never used. */
  sql = fsl_buffer_cstr2(&st->sql, &sqlLen);
  v = cwal_new_string_value(e, sql, (cwal_size_t)sqlLen)
    /* Reminder to self: we could get away with an X-string for 99.9%
       of use cases, but if someone holds a ref to that string
       after the statement is gone, blammo - cwal-level assertion
       at some point.
    */;
  SET("sql");
#endif
#undef STMT_INCLUDE_SQL
  v = cwal_new_integer(e, (cwal_int_t)st->colCount);
  SET("columnCount");
  v = cwal_new_integer(e, (cwal_int_t)st->paramCount);
  SET("parameterCount");
  if(st->colCount){
    v = s2_fsl_stmt_col_names(e, st);
    SET("columnNames");
  }
  cwal_value_prototype_set(nv, fsl_stmt_prototype(se));
#undef SET
#undef VCHECK
  assert(!rc);
  end:
  return rc;
}

static int cb_fsl_db_prepare( cwal_callback_args const * args,
                              cwal_value **rv ){
  fsl_stmt st = fsl_stmt_empty;
  int rc;
  char const * sql;
  cwal_size_t sqlLen = 0;
  cwal_value * nv = NULL;
  cwal_engine * e = args->engine;
  fsl_stmt * st2 = NULL;
  THIS_DB;
  sql = args->argc
    ? cwal_value_get_cstr(args->argv[0], &sqlLen)
    : NULL;
  if(!sql || !sqlLen){
    return cwal_exception_setf(e,
                               FSL_RC_MISUSE,
                               "Expecting a non-empty string "
                               "argument (SQL).");
  }
  assert(!st.stmt);
  rc = fsl_db_prepare( db, &st, "%.*s", (int)sqlLen, sql);
  if(rc){
    rc = cb_toss_db(args, db);
    assert(!st.stmt);
  }else{
    st2 = fsl_stmt_malloc();
    nv = st2
      ? cwal_new_native_value(e,
                              st2, cwal_finalizer_f_fsl_stmt,
                              FSL_TYPEID(fsl_stmt))
      : NULL;
    if(!nv){
      if(st2) fsl_free(st2);
      fsl_stmt_finalize(&st);
      st2 = NULL;
      rc = CWAL_RC_OOM;
    }else{
      void const * kludge = st2->allocStamp;
      *st2 = st;
      st2->allocStamp = kludge;
      rc = s2_fsl_setup_new_stmt(se, nv, st2);
      if(!rc) *rv = nv;
    }
  }
  if(rc && nv){
    cwal_value_unref(nv);
  }        
  return rc;
}

static int cb_fsl_db_filename( cwal_callback_args const * args,
                               cwal_value **rv ){
  char const * fname = NULL;
  fsl_size_t nameLen = 0;
  THIS_DB;
  fname = fsl_db_filename(db, &nameLen);
  *rv = fname
    ? cwal_new_string_value(args->engine, fname, nameLen)
    : cwal_value_null();
  return *rv ? 0 : CWAL_RC_OOM;
}

static int cb_fsl_db_name( cwal_callback_args const * args,
                               cwal_value **rv ){
  char const * fname = NULL;
  THIS_DB;
  fname = fsl_db_name(db);
  *rv = fname
    ? cwal_new_string_value(args->engine, fname, fsl_strlen(fname))
    : cwal_value_null();
  return *rv ? 0 : CWAL_RC_OOM;
}

/**
   Extracts result column ndx from st and returns a "the closest
   approximation" of its type in cwal_value form by assigning it to
   *rv. Returns 0 on success. On errror *rv is not modified.
   Does not trigger a th1ish/cwal exception on error.
*/
static int s2_fsl_stmt_to_value( cwal_engine * e,
                                 fsl_stmt * st,
                                 int ndx,
                                 cwal_value ** rv ){
  int vtype = sqlite3_column_type(st->stmt, ndx);
  switch( vtype ){
    case SQLITE_NULL:
      *rv = cwal_value_null();
      return 0;
    case SQLITE_INTEGER:
      *rv = cwal_new_integer( e,
                              (cwal_int_t)fsl_stmt_g_int64(st,ndx) );
      break;
    case SQLITE_FLOAT:
      *rv = cwal_new_double( e,
                             (cwal_double_t)fsl_stmt_g_double(st, ndx) );
      break;
    case SQLITE_BLOB: {
      int rc;
      fsl_size_t slen = 0;
      void const * bl = 0;
      rc = fsl_stmt_get_blob(st, ndx, &bl, &slen);
      if(rc){
        /* FIXME: we need a fsl-to-cwal rc converter (many of them line up). */
        return CWAL_RC_ERROR;
      }else if(!bl){
        *rv = cwal_value_null();
      }else{
        cwal_buffer * buf = cwal_new_buffer(e, (cwal_size_t)(slen+1));
        if(!buf) return CWAL_RC_OOM;
        rc = cwal_buffer_append( e, buf, bl, (cwal_size_t)slen );
        if(rc){
          assert(!*rv);
          cwal_value_unref(cwal_buffer_value(buf));
        }else{
          *rv = cwal_buffer_value(buf);
        }
      }
      break;
    }
    case SQLITE_TEXT: {
      fsl_size_t slen = 0;
      char const * str = 0;
      str = fsl_stmt_g_text( st, ndx, &slen );
      *rv = cwal_new_string_value(e, slen ? str : NULL,
                                  (cwal_size_t) slen);
      break;
    }
    default:
      return cwal_exception_setf(e, CWAL_RC_TYPE,
                                 "Unknown db column type (%d).",
                                 vtype);
  }
  return *rv ? 0 : CWAL_RC_OOM;
}

/**
   Extracts all columns from st into a new cwal_object value.
   colNames is expected to hold the same number of entries
   as st has columns, and in the same order. The entries in
   colNames are used as the object's field keys.

   Returns NULL on error.
*/
static cwal_value * s2_fsl_stmt_row_to_object2( cwal_engine * e,
                                                fsl_stmt * st,
                                                cwal_array const * colNames ){
  int colCount;
  cwal_value * objV;
  cwal_object * obj;
  cwal_string * colName;
  int i;
  int rc;
  if( ! st ) return NULL;
  colCount = st->colCount;
  if( !colCount || (colCount>(int)cwal_array_length_get(colNames)) ) {
    return NULL;
  }
  obj = cwal_new_object(e);
  if( ! obj ) return NULL;
  objV = cwal_object_value( obj );
  cwal_value_ref(objV);
  for( i = 0; i < colCount; ++i ){
    cwal_value * v = NULL;
    cwal_value * colNameV = cwal_array_get( colNames, i );
    colName = cwal_value_get_string( colNameV );
    if( ! colName ) goto error;
    rc = s2_fsl_stmt_to_value( e, st, i, &v );
    if( rc ) goto error;
    cwal_value_ref(v);
    rc = cwal_prop_set_v( objV, colNameV, v );
    cwal_value_unref(v);
    if( rc ){
      goto error;
    }
  }
  cwal_value_unhand(objV);
  return objV;
  error:
  cwal_value_unref( objV );
  return NULL;
}

/**
   Extracts all columns from st into a new cwal_object value,
   using st's column names as the keys.

   Returns NULL on error.
*/
static cwal_value * s2_fsl_stmt_row_to_object( cwal_engine * e,
                                               fsl_stmt * st ){
  cwal_value * objV;
  cwal_object * obj;
  char const * colName;
  int i;
  int rc;
  if( ! st || !st->colCount ) return NULL;
  obj = cwal_new_object(e);
  if( ! obj ) return NULL;
  objV = cwal_object_value(obj);
  cwal_value_ref(objV);
  for( i = 0; i < st->colCount; ++i ){
    cwal_value * v = NULL;
    colName = fsl_stmt_col_name(st, i);
    if( ! colName ) goto error;
    rc = s2_fsl_stmt_to_value( e, st, i, &v );
    if( rc ) goto error;
    cwal_value_ref(v);
    rc = cwal_prop_set( objV, colName, cwal_strlen(colName), v );
    cwal_value_unref(v);
    if( rc ){
      goto error;
    }
  }
  cwal_value_unhand(objV);
  return objV;
  error:
  cwal_value_unref( objV );
  return NULL;
}

/**
   Appends all result columns from st to ar.
*/
static int s2_fsl_stmt_row_to_array2( cwal_engine * e,
                                      fsl_stmt * st,
                                      cwal_array * ar)
{
  int i;
  int rc = 0;
  for( i = 0; i < st->colCount; ++i ){
    cwal_value * v = NULL;
    rc = s2_fsl_stmt_to_value( e, st, i, &v );
    if( rc ) goto end;
    cwal_value_ref(v);
    rc = cwal_array_append( ar, v );
    cwal_value_unref(v);
    if( rc ){
      goto end;
    }
  }
  end:
  return rc;
}

/**
   Converts all column values from st into a cwal_array
   value. Returns NULL on error, else an array value.
*/
static cwal_value * s2_fsl_stmt_row_to_array( cwal_engine * e,
                                              fsl_stmt * st )
{
  cwal_array * ar;
  int rc = 0;
  if( ! st || !st->colCount ) return NULL;
  ar = cwal_new_array(e);
  if( ! ar ) return NULL;
  rc = s2_fsl_stmt_row_to_array2( e, st, ar );
  if(rc){
    cwal_array_unref(ar);
    return NULL;
  }else{
    return cwal_array_value(ar);
  }
}

/**
   mode: 0==Object, >0==Array, <0==no row data (just boolean
   indicator)
*/
static int s2_fsl_stmt_step_impl( cwal_callback_args const * args,
                                  cwal_value **rv,
                                  int mode ){
  int rc;
  int scode;
  THIS_STMT;
  scode = fsl_stmt_step( stmt );
  switch(scode){
    case FSL_RC_STEP_DONE:
      rc = 0;
      *rv = cwal_value_undefined();
      break;
    case FSL_RC_STEP_ROW:{
      if(0==mode){
        /* Object mode */
        cwal_array const * colNames =
          cwal_value_get_array( cwal_prop_get(args->self,
                                              "columnNames", 11) );
        *rv = colNames
          ? s2_fsl_stmt_row_to_object2(args->engine,
                                       stmt, colNames)
          : s2_fsl_stmt_row_to_object(args->engine, stmt);
      }else if(mode>0){
        /* Array mode */
        *rv = s2_fsl_stmt_row_to_array(args->engine, stmt);
      }else{
        /* "Normal" mode */
        *rv = cwal_value_true();
      }
      rc = *rv ? 0 : CWAL_RC_OOM;
      break;
    }
    default:
      rc = cb_toss_db(args, stmt->db);
      break;
  }
  return rc;
}

static int cb_fsl_stmt_step_array( cwal_callback_args const * args, cwal_value **rv ){
  return s2_fsl_stmt_step_impl( args, rv, 1 );
}

static int cb_fsl_stmt_step_object( cwal_callback_args const * args, cwal_value **rv ){
  return s2_fsl_stmt_step_impl( args, rv, 0 );
}

static int cb_fsl_stmt_step( cwal_callback_args const * args, cwal_value **rv ){
  return s2_fsl_stmt_step_impl( args, rv, -1 );
}

static int cb_fsl_stmt_reset( cwal_callback_args const * args, cwal_value **rv ){
  char resetCounter = 0;
  int rc;
  THIS_STMT;
  if(1 < args->argc) resetCounter = cwal_value_get_bool(args->argv[1]);
  rc = fsl_stmt_reset2(stmt, resetCounter);
  if(!rc) *rv = args->self;
  return rc
    ? cb_toss_db( args, stmt->db )
    : 0;
}

static int cb_fsl_stmt_row_to_array( cwal_callback_args const * args,
                                     cwal_value **rv ){
  int rc;
  THIS_STMT;
  if(!stmt->rowCount) return cb_toss(args, FSL_RC_MISUSE,
                                     "Cannot fetch row data from an "
                                     "unstepped statement.");
  *rv = s2_fsl_stmt_row_to_array(args->engine, stmt);
  if(*rv) rc = 0;
  else rc = cb_toss(args, CWAL_RC_ERROR,
                    "Unknown error converting statement "
                    "row to Array.");
  return rc;
}

static int cb_fsl_stmt_row_to_object( cwal_callback_args const * args,
                                      cwal_value **rv ){
  int rc;
  THIS_STMT;
  if(!stmt->rowCount) return cb_toss(args, FSL_RC_MISUSE,
                                     "Cannot fetch row data from "
                                     "an unstepped statement.");
  *rv = s2_fsl_stmt_row_to_object(args->engine, stmt);
  if(*rv) rc = 0;
  else rc = cb_toss(args, CWAL_RC_ERROR,
                    "Unknown error converting statement row "
                    "to Object.");
  return rc;
}


/**
   Tries to bind v to the given parameter column (1-based) of
   nv->stmt.  Returns 0 on success, triggers a script-side exception
   on error.
*/
static int s2_fsl_stmt_bind( cwal_engine * e,
                             fsl_stmt * st,
                             int ndx,
                             cwal_value * v ){
  int rc;
  int const vtype = v ? cwal_value_type_id(v) : CWAL_TYPE_NULL;
  if(ndx<1) {
    return cwal_exception_setf(e, FSL_RC_RANGE,
                               "Bind index %d is invalid: indexes are 1-based.",
                               ndx);
  }
  else if(ndx > st->paramCount) {
    return cwal_exception_setf(e, FSL_RC_RANGE,
                               "Bind index %d is out of range. Range=(1..%d).",
                               ndx, st->paramCount);
  }
  /*MARKER(("Binding %s to column #%u\n", cwal_value_type_name(v), ndx));*/
  switch( vtype ){
    case CWAL_TYPE_NULL:
    case CWAL_TYPE_UNDEF:
      rc = fsl_stmt_bind_null(st, ndx);
      break;
    case CWAL_TYPE_BOOL:
      rc = fsl_stmt_bind_int32(st, ndx, cwal_value_get_bool(v));
      break;
    case CWAL_TYPE_INTEGER:
      /* We have no way of knowing which type (32/64-bit) to bind
         here, so we'll guess. We could check the range, i guess,
         but for sqlite it makes little or no difference, anyway.
      */
      rc = fsl_stmt_bind_int64(st, ndx,
                               (int64_t)cwal_value_get_integer(v));
      break;
    case CWAL_TYPE_DOUBLE:
      rc = fsl_stmt_bind_double(st, ndx, (double)cwal_value_get_double(v));
      break;
    case CWAL_TYPE_BUFFER:
    case CWAL_TYPE_STRING: {
      /* FIXME: bind using the fsl_stmt_bind_xxx() APIs!!!
         string/buffer binding is currently missing.
      */
      cwal_size_t slen = 0;
      char const * cstr = cwal_value_get_cstr(v, &slen);
      if(!cstr){
        /* Will only apply to empty buffers (Strings are never
           NULL). But it's also possible that a buffer with
           length 0 has a non-NULL memory buffer. So we cannot,
           without further type inspection, clearly
           differentiate between a NULL and empty BLOB
           here. This distinction would seem to be (?) 
           unimportant for this particular use case, so
           fixing/improving it can wait.
        */
        rc = fsl_stmt_bind_null(st, ndx);
      }
#if 1
      else if(CWAL_TYPE_BUFFER==vtype){
        rc = fsl_stmt_bind_blob(st, ndx, cstr, (int)slen, 1);
      }
#endif
      else{
        rc = fsl_stmt_bind_text(st, ndx, cstr, (int)slen, 1);
      }
      break;
    }
    default:
      return cwal_exception_setf(e, FSL_RC_TYPE,
                                 "Unhandled data type (%s) for binding "
                                 "column %d.",
                                 cwal_value_type_name(v), ndx);
  }
  if(rc){
    fsl_size_t msgLen = 0;
    char const * msg = NULL;
    int const dbRc = fsl_db_err_get(st->db, &msg, &msgLen);
    rc = cwal_exception_setf(e, FSL_RC_DB,
                             "Binding column %d failed with "
                             "sqlite code #%d: %.*s", ndx,
                             dbRc, (int)msgLen, msg);
  }
  return rc;
}

static int s2_fsl_stmt_bind_values_a( cwal_engine * e,
                                      fsl_stmt * st,
                                      cwal_array const * src){
  int const n = (int)cwal_array_length_get(src);
  int i = 0;
  int rc = 0;
  for( ; !rc && (i < n); ++i ){
    rc = s2_fsl_stmt_bind( e, st, i+1,
                           cwal_array_get(src,(cwal_size_t)i) );
  }
  return rc;
}

/**
   Internal helper for binding by name.
*/
typedef struct {
  fsl_stmt * st;
  cwal_engine * e;
} BindByNameState;

/**
   Property visitor helper for binding parameters by name.
*/
static int cwal_kvp_visitor_f_bind_by_name( cwal_kvp const * kvp, void * state ){
  cwal_size_t keyLen = 0;
  cwal_value const * v = cwal_kvp_key(kvp);
  char const * key = cwal_value_get_cstr(v, &keyLen);
  BindByNameState const * bbn = (BindByNameState const *)state;
  int ndx;
  if(!key){
    if(!cwal_value_is_integer(v)){
      return 0 /* non-string property. */;
    }else{
      ndx = (int)cwal_value_get_integer(v);
    }
  }else{
    ndx = fsl_stmt_param_index(bbn->st, key);
    if(ndx<=0){
      return cwal_exception_setf(bbn->e, CWAL_RC_RANGE, "Parameter name '%.*s' "
                                 "does not resolve to an index. (Maybe missing "
                                 "the leading ':' or '$' part of the param name?)",
                                 (int)keyLen, key);
    }
  }
  return s2_fsl_stmt_bind(bbn->e, bbn->st, ndx, cwal_kvp_value(kvp));
}

/**
   Internal helper for binding values to statements.

   bind may be either an Array of values to bind, a container of param
   names (incl. ':' or '$' prefix), or a single value, to bind at the
   given index. For arrays/containers, the given index is ignored. As
   a special case, if bind === cwal_value_undefined() then it is
   simply ignored.
*/
static int s2_fsl_stmt_bind_proxy( cwal_engine * e,
                                   fsl_stmt * st,
                                   int ndx,
                                   cwal_value * bind){
  int rc = 0;
  if(cwal_value_undefined() == bind) rc = 0;
  else if(cwal_value_is_array(bind)){
    rc = s2_fsl_stmt_bind_values_a(e, st,
                                   cwal_value_get_array(bind));
  }else if(cwal_props_can(bind)){
    BindByNameState bbn;
    bbn.e = e;
    bbn.st = st;
    rc = cwal_props_visit_kvp(bind, cwal_kvp_visitor_f_bind_by_name,
                              &bbn);
  }else{
    rc = s2_fsl_stmt_bind( e, st, ndx, bind);
  }
  return rc;
}

#if 0
static int s2_fsl_stmt_bind_from_args( cwal_engine * e,
                                       fsl_stmt *st,
                                       cwal_callback_args const * args,
                                       uint16_t startAtArg ){
  int rc = 0;
  cwal_value * bind = (args->argc < startAtArg)
    ? args->argv[startAtArg]
    : NULL;
  if(!bind) return 0;
  if(cwal_value_is_array(bind)){
    rc = s2_fsl_stmt_bind_values_a(e, st,
                                   cwal_value_get_array(bind));
  }else if(cwal_value_is_object(bind)){
    MARKER(("TODO: binding by name"));
    rc = CWAL_RC_ERROR;
  }else if(!cwal_value_is_undef(bind) && (args->argc > startAtArg)){
    int i = startAtArg;
    for( ; !rc && (i < (int)args->argc); ++i ){
      rc = s2_fsl_stmt_bind( e, st, i, args->argv[i]);
    }
  }
  return rc;
}
#endif

static int cb_fsl_stmt_bind( cwal_callback_args const * args,
                             cwal_value **rv ){
  cwal_int_t ndx = -999;
  int rc;
  THIS_STMT;
  if(!args->argc){
    return cb_toss(args, FSL_RC_MISUSE,
                   "Expecting (integer Index [,Value=null]) or "
                   "(Array|Object|undefined) arguments.");
  }
  if(cwal_value_undefined() == args->argv[0]){
    /* Special case to simplify some script code */
    *rv = args->self;
    return 0;
  }else if(cwal_value_is_integer(args->argv[0])){
    ndx = cwal_value_get_integer(args->argv[0]);
    if(1>ndx){
      return cb_toss(args, FSL_RC_RANGE,
                     "SQL bind() indexes are 1-based.");
    }
  }
  rc = s2_fsl_stmt_bind_proxy(args->engine, stmt,
                              (int)(-999==ndx ? 1 : ndx),
                              (-999==ndx)
                              ? args->argv[0]
                              : ((args->argc>1)
                                 ? args->argv[1]
                                 : NULL));
  if(!rc) *rv = args->self;
  return rc;
}

static int cb_fsl_stmt_get( cwal_callback_args const * args, cwal_value **rv ){
  int rc;
  cwal_int_t ndx;
  THIS_STMT;
  if(!args->argc || !cwal_value_is_integer(args->argv[0])){
    return cb_toss(args, FSL_RC_MISUSE,
                   "Expecting Index arguments.");
  }
  else if(!stmt->colCount){
    return cb_toss(args, FSL_RC_MISUSE,
                   "This is not a fetch-style statement.");
  }
  ndx = cwal_value_get_integer(args->argv[0]);
  if((ndx<0) || (ndx>=stmt->colCount)){
    return cb_toss(args, CWAL_RC_RANGE,
                   "Column index %d is out of range. "
                   "Valid range is (%d..%d).", ndx,
                   0, stmt->colCount-1);
  }
  rc = s2_fsl_stmt_to_value( args->engine, stmt,
                             (uint16_t)ndx, rv );
  if(rc && (CWAL_RC_EXCEPTION!=rc) && (CWAL_RC_OOM!=rc)){
    rc = cb_toss( args, rc, "Get-by-index failed with code %d (%s).",
                  rc, cwal_rc_cstr(rc));
  }
  return rc;
}

static int cb_fsl_db_exec_impl( cwal_callback_args const * args,
                                cwal_value **rv,
                                char isMulti ){
  int rc = 0;
  int argIndex = 0;
  THIS_DB;
  do{ /* loop on the arguments, expecting SQL for each one... */
    cwal_size_t sqlLen = 0;
    char const * sql = (args->argc>argIndex)
      ? cwal_value_get_cstr(args->argv[argIndex], &sqlLen)
      : NULL;
    if(!sql || !sqlLen){
      rc = (argIndex>0)
        ? 0
        : cb_toss(args,
                  FSL_RC_MISUSE,
                  "Expecting a non-empty string/buffer "
                  "argument (SQL).");
      break;
    }
    /* MARKER(("SQL:<<<%.*s>>>\n", (int)sqlLen, sql)); */
    rc = isMulti
      ? fsl_db_exec_multi( db, "%.*s", (int)sqlLen, sql )
      : fsl_db_exec( db, "%.*s", (int)sqlLen, sql )
      ;
    if(rc) rc = cb_toss_db( args, db );
  }while(!rc && (++argIndex < args->argc));
  if(!rc) *rv = args->self;
  return rc;
}

static int cb_fsl_db_exec_multi( cwal_callback_args const * args,
                                 cwal_value **rv ){
  return cb_fsl_db_exec_impl( args, rv, 1 );
}

static int cb_fsl_db_exec( cwal_callback_args const * args,
                           cwal_value **rv ){
  return cb_fsl_db_exec_impl( args, rv, 0 );
}

static int cb_fsl_db_last_insert_id( cwal_callback_args const * args,
                                     cwal_value **rv ){
  THIS_DB;
  *rv = cwal_new_integer(args->engine,
                         (cwal_int_t)fsl_db_last_insert_id(db));
  return *rv ? 0 : CWAL_RC_OOM;
}

/**
   If !cb, this is a no-op, else if cb is-a Function, it is call()ed,
   if it is a String/Buffer, it is eval'd, else an exception is
   thrown.  self is the 'this' for the call(). Result is placed in
   *rv.  Returns 0 on success.
   */
static int s2_fsl_stmt_each_call_proxy( s2_engine * se, fsl_stmt * st,
                                        cwal_value * cb, cwal_value * self,
                                        cwal_value **rv ){
  char const * cstr;
  cwal_size_t strLen = 0;
  char const useNewScope = 1
    /*
      We need a new scope to avoid that evaluation of a string callback
      vacuums up self.
    */;
  if(!cb) return 0;
  else if((cstr = cwal_value_get_cstr(cb,&strLen))){
    /** TODO: tokenize this only the first time. */
    s2_ptoker pr = s2_ptoker_empty;
    cwal_scope s2Scope = cwal_scope_empty;
    int rc;
    /**
       Bug Reminder: in stack traces and assertion traces the
       script name/location info is wrong in this case (empty and
       relative to cstr, respectively) because s2 doesn't have
       a way to map this string back to a source location at this
       point.
    */
    if(useNewScope){
      rc = cwal_scope_push2(se->e, &s2Scope);
      if(rc) return rc;
    }
    s2_ptoker_init(&pr, cstr, (int)strLen);
    pr.name = "Db.each() callback";
    rc = s2_eval_ptoker(se, &pr, 0, rv);
    if(CWAL_RC_RETURN==rc){
      rc = 0;
      *rv = cwal_propagating_take(se->e);
    }
    if(useNewScope){
      assert(se->e->current == &s2Scope);
      cwal_scope_pop2(se->e, rc ? 0 : *rv);
    }
    return rc;
  }else if(cwal_value_is_function(cb)){
    cwal_function * f = cwal_value_get_function(cb);
    if(useNewScope){
      return cwal_function_call(f, self, rv, 0, NULL );
    }else{
      cwal_scope * sc = cwal_scope_current_get(se->e);
      return cwal_function_call_in_scope(sc, f, self, rv,
                                         0, NULL );
    }
  }else {
    return cwal_exception_setf(se->e, FSL_RC_MISUSE,
                               "Don't now how to handle callback "
                               "of type '%s'.",
                               cwal_value_type_name(cb));
  }
}

/**
   Script usage:

   db.each({
   sql: "SQL CODE", // required
   bind: X, // parameter value or array of values to bind.
            // Passing the undefined value is the same as
            // not passing any value.
   mode: 0, // 0==rows as objects, else as arrays (default)
   callback: string | function // called for each row
   })

   Only the 'sql' property is required, and 'bind' must be set
   if the SQL contains any binding placeholders.

   In the scope of the callback, 'this' will resolve to the current
   row data, either in object or array form (depending on the 'mode'
   property). If the callback throws, that exception is propagated.
   If it returns a literal false (as opposed to another falsy value)
   then iteration stops without an error.

   In addition, the following scope-level variables are set:

   - rowNumber: 1-based number of the row (the iteration count).

   - columnNames: array of column names for the result set.
  
   Example callbacks:

   <<<EOF
     print(rowNumber, columnNames, this);
     print(this.0 + this.1); // array-mode column access
   EOF

   proc(){
     print(rowNumber, columnNames, this);
     print(this.colA + this.colB); // object-mode column access
   }

   Using the string form should be ever so slightly more efficient.
*/
static int cb_fsl_db_each( cwal_callback_args const * args, cwal_value **rv ){
  int rc = 0;
  cwal_value const * sql;
  char const * csql;
  cwal_size_t sqlLen = 0;
  fsl_stmt st = fsl_stmt_empty;
  int scode;
  cwal_value * vMode;
  int mode;
  cwal_value * props;
  cwal_value * bind;
  cwal_value * callback;
  cwal_value * cbSelf = NULL;
  cwal_array * cbArray = NULL;
  cwal_value * colNamesV = 0;
  cwal_array * colNames = 0;
  cwal_int_t rowNum;
  cwal_engine * e = args->engine;
  THIS_DB;
  if(!args->argc || !cwal_value_is_object(args->argv[0])){
    return cwal_exception_setf(e, FSL_RC_MISUSE,
                               "Expecting Object parameter.");
  }
  props = args->argv[0];
  sql = cwal_prop_get(props, "sql", 3 );
  csql = sql ? cwal_value_get_cstr(sql, &sqlLen) : NULL;
  if(!csql){
    return cb_toss(args, FSL_RC_MISUSE,
                   "Missing 'sql' string/buffer property.");
  }
  vMode = cwal_prop_get( props, "mode", 4 );
  mode = vMode ? cwal_value_get_integer(vMode) : 1;
  bind = cwal_prop_get( props, "bind", 4 );
  callback = cwal_prop_get( props, "callback", 8 );
  if(callback){
    if(!cwal_value_is_function(callback)
       && !cwal_value_get_cstr(callback, 0)){
      callback = NULL;
    }
  }
  rc = fsl_db_prepare(db, &st, "%.*s", (int)sqlLen, csql);
  if(rc){
    return cb_toss_db(args, db);
  }
  if(bind && !cwal_value_is_undef(bind)){
    rc = s2_fsl_stmt_bind_proxy( e, &st, 1, bind );
    if(rc) goto end;
  }
  if(st.colCount){
    colNamesV = s2_fsl_stmt_col_names(e,&st);
    if(!colNamesV){
      rc = CWAL_RC_OOM;
      goto end;
    }
    cwal_value_ref(colNamesV);
    rc = s2_var_set( se, 0, "columnNames", 11, colNamesV );
    cwal_value_unref(colNamesV);
    if(rc){
      colNamesV = NULL;
      goto end;
    }
    colNames = cwal_value_get_array(colNamesV);
    assert(cwal_array_length_get(colNames) == (cwal_size_t)st.colCount);
  }
  rc = s2_var_set( se, 0, "columnCount", 11,
                   cwal_new_integer(e, (cwal_int_t)st.colCount) );
  if(rc) goto end;
    
  /*
    Step through each row and call the given callback function/string...
  */
  for( rowNum = 1;
       FSL_RC_STEP_ROW == (scode = fsl_stmt_step( &st ));
       ++rowNum){
    if(callback && !cbSelf){
      /** Init callback info if needed */
      cbArray = (0==mode)
        ? NULL
        : cwal_new_array(e);
      cbSelf = cbArray
        ? cwal_array_value(cbArray)
        : cwal_new_object_value(e);
      if(!cbSelf){
        rc = CWAL_RC_OOM;
        break;
      }else{
        cwal_value_ref(cbSelf);
        rc = s2_var_set(se, 0, "this", 4, cbSelf);
        cwal_value_unref(cbSelf);
        if(rc){
          goto end;
        }
      }
    }
    if(cbSelf){
      /**
         Set up this.XXX to each column's value, where
         XXX is either the column index (for array mode)
         or column name (for object mode).
      */
      cwal_value * frv = 0;
      int i = 0;
      for( ; !rc && (i < st.colCount); ++i ){
        cwal_value * cv;
        cv = NULL;
        rc = s2_fsl_stmt_to_value(e, &st, i, &cv);
        if(rc && (CWAL_RC_EXCEPTION!=rc) && (CWAL_RC_OOM!=rc)){
          rc = cwal_exception_setf(e, rc,
                                   "Conversion from db column to "
                                   "value failed with code "
                                   "%d (%s).",
                                   rc, fsl_rc_cstr(rc));
        }
        cwal_value_ref(cv);
        if(cbArray) rc = cwal_array_set( cbArray, i, cv);
        else{
          cwal_value * key;
          assert(colNames);
          key = cwal_array_get(colNames, i);
          assert(key);
          rc = cwal_prop_set_v( cbSelf, key, cv);
        }
        cwal_value_unref(cv);
      }
      if(rc) goto end;
      rc = s2_var_set( se, 0, "rowNumber", 9,
                       cwal_new_integer(e, rowNum) );
      if(rc) goto end /* leave potentially leaked new integer for the
                         scope to clean up */;
      rc = s2_fsl_stmt_each_call_proxy(se, &st, callback, cbSelf, &frv);
      if(rc) goto end;
      else if(frv == cwal_value_false()/*yes, a ptr comparison*/){
        /* If the function returns literal false, stop
           looping without an error. */
        goto end;
      }
      cbSelf = 0 /* Causes the next iteration to create a new
                  * array/object per row, overwriting "this". */;
    }
  }
  if(FSL_RC_STEP_ERROR==scode){
    rc = cwal_exception_setf(e, FSL_RC_DB,
                             "Stepping cursor failed.");
  }
  end:
  if(st.stmt){
    fsl_stmt_finalize( &st );
  }
  if(!rc) *rv = args->self;
  return rc;
}

/**
   Value DB.selectValue(string SQL [, bind = undefined [,defaultResult=undefined]])
*/
static int cb_fsl_db_select_value( cwal_callback_args const * args, cwal_value **rv ){
  int rc = 0, dbrc = 0;
  char const * sql;
  cwal_size_t sqlLen = 0;
  fsl_stmt st = fsl_stmt_empty;
  cwal_value * bind = 0;
  cwal_value * dflt = 0;
  THIS_DB;
  sql = args->argc ? cwal_value_get_cstr(args->argv[0], &sqlLen) : 0;
  if(!sql || !sqlLen){
    return cwal_exception_setf(args->engine, FSL_RC_MISUSE,
                               "Expecting SQL string/buffer parameter.");
  }
  bind = args->argc>1 ? args->argv[1] : 0;

  dbrc = fsl_db_prepare( db, &st, "%.*s", (int)sqlLen, sql );
  if(dbrc){
    rc = cb_toss_db(args, db);
    assert(rc);
    goto end;
  }

  if(bind){
    if(2==args->argc && !st.paramCount){
      dflt = bind;
    }else{
      rc = s2_fsl_stmt_bind_proxy(args->engine, &st, 1, bind);
    }
    bind = 0;
  }

  if( !rc && (FSL_RC_STEP_ROW == (dbrc=fsl_stmt_step(&st)))){
    cwal_value * xrv = 0;
    rc = s2_fsl_stmt_to_value(args->engine, &st, 0, &xrv);
    if(!rc){
      *rv = xrv ? xrv : cwal_value_undefined();
    }
  }else if(FSL_RC_STEP_DONE == dbrc){
    *rv = dflt ? dflt : cwal_value_undefined();
  }else if(FSL_RC_STEP_ERROR == dbrc){
    assert(st.db->error.code);
    rc = cb_toss_db(args, st.db);
  }
  end:
  fsl_stmt_finalize(&st);
  return rc;  
}


/**
   Array Db.selectValues(string|buffer SQL [, bind = undefined])
*/
static int cb_fsl_db_select_values( cwal_callback_args const * args, cwal_value **rv ){
  int rc = 0, dbrc = 0;
  char const * sql;
  cwal_size_t sqlLen = 0;
  fsl_stmt st = fsl_stmt_empty;
  cwal_value * bind = 0;
  cwal_array * ar = 0;
  cwal_value * arV = 0;
  THIS_DB;
  sql = args->argc ? cwal_value_get_cstr(args->argv[0], &sqlLen) : 0;
  if(!sql || !sqlLen){
    return cwal_exception_setf(args->engine, FSL_RC_MISUSE,
                               "Expecting SQL string/buffer parameter.");
  }
  bind = args->argc>1 ? args->argv[1] : 0;

  dbrc = fsl_db_prepare( db, &st, "%.*s", (int)sqlLen, sql );
  if(dbrc){
    rc = cb_toss_db(args, db);
    assert(rc);
    goto end;
  }

  ar = cwal_new_array(args->engine);
  if(!ar){
    rc = CWAL_RC_OOM;
    goto end;
  }
  arV = cwal_array_value(ar);
  cwal_value_ref(arV);

  if(bind){
    rc = s2_fsl_stmt_bind_proxy(args->engine, &st, 1, bind);
    bind = 0;
  }

  while( !rc && (FSL_RC_STEP_ROW == (dbrc=fsl_stmt_step(&st)))){
    cwal_value * col = 0;
    rc = s2_fsl_stmt_to_value(args->engine, &st, 0, &col);
    if(!rc){
      cwal_value_ref(col);
      rc = cwal_array_append(ar, col);
      cwal_value_unref(col);
    }
  }

  end:
  fsl_stmt_finalize(&st);
  if(rc){
    cwal_value_unref(arV);
  }else if(arV){
    *rv = arV;
    cwal_value_unhand(arV);
  }
  return rc;  
}

static int cb_fsl_db_trans_begin( cwal_callback_args const * args, cwal_value **rv ){
  int rc;
  THIS_DB;
  rc = fsl_db_transaction_begin(db);
  if(rc){
    rc = cb_toss_db(args, db);
  }
  if(!rc) *rv = args->self;
  return rc;
}

static int cb_fsl_db_trans_end( cwal_callback_args const * args, cwal_value **rv, int mode ){
  int rc;
  THIS_DB;
  if(db->beginCount<=0){
    return cb_toss( args, CWAL_RC_RANGE, "No transaction is active.");
  }
  if(mode < 0){
    rc = fsl_db_rollback_force(db);
  }else{
    rc = fsl_db_transaction_end(db, mode ? 1 : 0);
  }
  if(rc){
    if(db->error.code){
      rc = cb_toss_db(args, db);
    }else{
      rc = cb_toss( args, CWAL_RC_ERROR,
                    "fsl error code during %s: %d (%s)",
                    (mode<0
                     ? "forced rollback"
                     : (mode>0
                        ? "rollback"
                        : "db commit" /* as opposed to repo checkin/commit */)
                     ),
                    rc, fsl_rc_cstr(rc) );
    }
  }else{
    *rv = args->self;
  }
  return rc;
}

static int cb_fsl_db_trans_commit( cwal_callback_args const * args, cwal_value **rv ){
  return cb_fsl_db_trans_end( args, rv, 0 );
}

static int cb_fsl_db_trans_rollback( cwal_callback_args const * args, cwal_value **rv ){
  return cb_fsl_db_trans_end( args, rv,
                              (args->argc
                               && cwal_value_get_bool(args->argv[0]))
                              ? -1 /* force immediate rollback */
                              : 1 );
}

static int cb_fsl_db_trans_state( cwal_callback_args const * args, cwal_value **rv ){
  int bc;
  THIS_DB;
  bc = db->beginCount > 0 ? db->beginCount : 0;
  *rv = cwal_new_integer(args->engine,
                         (cwal_int_t)(db->doRollback ? -bc : bc));
  return *rv ? 0 : CWAL_RC_OOM;
}

static int cb_fsl_db_trans( cwal_callback_args const * args, cwal_value **rv ){
  int rc;
  cwal_function * func;
  THIS_DB;
  func = args->argc ? cwal_value_function_part(args->engine, args->argv[0]) : 0;
  if(!func){
    return cb_toss(args, CWAL_RC_MISUSE, "Expecting a Function argument.");
  }
  rc = fsl_db_transaction_begin( db );
  if(rc){
    return db->error.code
      ? cb_toss_db(args, db)
      : cb_toss(args, CWAL_RC_ERROR,
                "Error #%d (%s) while starting transaction.",
                rc, fsl_rc_cstr(rc));
  }
  rc = cwal_function_call( func, args->self, 0, 0, 0 );
  if(rc){
    fsl_db_transaction_end( db, 1 );
#if 0
    /* Just proving to myself that this rollback is still called in
       the face of an s2-level exit()/fatal()/assert(). It does. We
       can in fact preempt a FATAL result here, but doing so is a bad
       idea.
    */
    if(CWAL_RC_FATAL==rc){
      cwal_exception_set(args->engine, cwal_propagating_take(args->engine));
      rc = CWAL_RC_EXCEPTION;
    }
#endif
  }else{
    rc = fsl_db_transaction_end( db, 0 );
    if(rc){
      rc = db->error.code
        ? cb_toss_db(args, db)
        : cb_toss(args, CWAL_RC_ERROR,
                  "Error #%d (%s) while committing transaction.",
                  rc, fsl_rc_cstr(rc));
    }else{
      *rv = args->self;
    }
  }
  return rc;
}


/**
 ** If cx has a property named "db", it is returned, else if
 ** createIfNotExists is true then a new native fsl_db Object named
 ** "db" is inserted into cx and returned. Returns NULL on allocation
 ** error or if no such property exists and createIfNotExists is
 ** false.
 */
static cwal_value * fsl_cx_db_prop( fsl_cx * f,
                                    cwal_value * fv,
                                    s2_engine * se,
                                    char createIfNotExists){
  cwal_value * rv;
  fsl_db * db = fsl_cx_db(f);
  /* assert(db); */
  rv = db ? cwal_prop_get(fv, "db", 2) : NULL;
  if(db && !rv && createIfNotExists){
    int rc = fsl_db_new_native(se, f, db, 0, &rv, FSL_DBROLE_MAIN);
    if(rc) return NULL;
    cwal_value_ref(rv);
    if(rv){
      rc = cwal_prop_set(fv, "db", 2, rv);
      cwal_value_unref(rv);
      if(rc) rv = NULL;
    }
  }
  return rv;
}


/**
 ** Internal helper which adds vSelf->db->repo/checkout/config
 ** handles. Returns 0 on success. MUST return CWAL_RC_xxx codes, NOT
 ** FSL_RC_xxx codes.
 */
static int fsl_cx_add_db_handles(fsl_cx * f, cwal_value * vSelf,
                                 s2_engine * se){
  cwal_value * dbV = fsl_cx_db_prop(f, vSelf, se, 1);
  cwal_value * d = NULL;
  fsl_db * db;
  int rc;
  cwal_size_t nameLen;
  if(!dbV) return FSL_RC_OOM;
#define DBH(NAME, GETTER, ROLE)                 \
  nameLen = cwal_strlen(NAME); \
  d = cwal_prop_get(dbV, NAME, nameLen); \
  if(!d && (db = GETTER(f))){ \
    rc = fsl_db_new_native(se, f, db, 0, &d, ROLE); \
    if(rc) return rc; \
    cwal_value_ref(d); \
    assert(d); \
    rc = cwal_prop_set(dbV, NAME, nameLen, d); \
    cwal_value_unref(d); \
    if(rc) return rc; \
  }
  /*DBH("main", fsl_cx_db, FSL_DBROLE_MAIN);*/
  DBH("repo", fsl_cx_db_repo, FSL_DBROLE_REPO);
  DBH("checkout", fsl_cx_db_ckout, FSL_DBROLE_CKOUT);
  DBH("config", fsl_cx_db_config, FSL_DBROLE_CONFIG);
#undef DBH
  return 0;    
}

static int cb_fsl_config_open( cwal_callback_args const * args,
                               cwal_value **rv ){
  char const * dbName;
  int rc;
  THIS_F;
  dbName = args->argc
    ? cwal_value_get_cstr(args->argv[0], NULL)
    : NULL;
  if(args->argc && (!dbName || !*dbName)){
    return cb_toss(args, FSL_RC_MISUSE,
                   "Expecting a non-empty string argument.");
  }
  rc = fsl_config_open( f, (dbName && *dbName) ? dbName : 0 );
  if(rc){
    rc = cb_toss_fsl(args, f);
  }else{
    rc = fsl_cx_add_db_handles(f, args->self, se);
    if(!rc) *rv = args->self;
  }
  return rc;
}

#if 0
/* this gets trickier than i'd like because of the script-side
   db handles.
*/
static int cb_fsl_config_close( cwal_callback_args const * args,
                                cwal_value **rv ){
  THIS_F;
  fsl_config_close(f);
  *rv = args->self;
  return 0;
}
#endif

static int cb_fsl_repo_open( cwal_callback_args const * args,
                             cwal_value **rv ){
  char const * dbName;
  int rc;
  THIS_F;
  dbName = args->argc
    ? cwal_value_get_cstr(args->argv[0], NULL)
    : NULL;
  if(!dbName || !*dbName){
    return cb_toss(args, FSL_RC_MISUSE,
                   "Expecting a non-empty string argument.");
  }
  rc = fsl_repo_open( f, dbName );
  if(rc){
    rc = cb_toss_fsl(args, f);
  }else{
    rc = fsl_cx_add_db_handles(f, args->self, se);
    if(!rc) *rv = args->self;
  }
  return rc;
}

static int cb_fsl_ckout_open_dir( cwal_callback_args const * args,
                                  cwal_value **rv ){
  char const * dbName = 0;
  cwal_size_t nameLen = 0;
  int rc;
  int checkParentDirs;
  THIS_F;
  dbName = args->argc
    ? cwal_value_get_cstr(args->argv[0], &nameLen)
    : 0;
  checkParentDirs = args->argc>1
    ? (cwal_value_get_bool(args->argv[1]) ? 1 : 0)
    : 1;
  rc = fsl_ckout_open_dir( f, dbName, checkParentDirs );
  if(rc){
    rc = cb_toss_fsl(args, f);
  }else{
    rc = fsl_cx_add_db_handles(f, args->self, se);
    if(!rc) *rv = args->self;
  }
  return rc;
}

static int cb_fsl_login_cookie_name( cwal_callback_args const * args,
                                     cwal_value ** rv){
  int rc;
  char * name;
  THIS_F;
  name = fsl_repo_login_cookie_name(f);
  if(name){
    assert( name[22] );
    assert( !name[23] );
    *rv = cwal_string_value(cwal_new_stringf(args->engine, "%.23s", name))
      /* z-string is only legal if libfossil and s2 use the same
         allocator, so we'll be pedantically correct here and
         copy it. String interning might make this a no-op.
      */;
    rc = *rv ? 0 : CWAL_RC_OOM;
    fsl_free(name);
  }else{
    *rv = cwal_value_undefined();
    rc = 0;
    /*cb_toss(args, CWAL_RC_ERROR,
      "Unexpected FossilContext state: "
      "no login cookie name.");*/
  }
  return rc;
}

/*
** Constructor function for fsl_cx instances.
**
** Script-side usage:
**
** var f = ThisFunc(object{...})
**
**
** The parameter object is optional. The only supported option at the
** moment is the traceSql boolean, which enables or disabled SQL
** tracing output.
**
** On success it returns (via *rv) a new cwal_native which is bound to
** a new fsl_cx instance. The fsl_cx will be initialized such that
** fsl_output() and friends will be redirected through cwal_output(),
** to allow fsl_output()-generated output to take advantage of
** th1ish's output buffering and whatnot.
*/
static int cb_fsl_cx_ctor( cwal_callback_args const * args,
                           cwal_value **rv ){
  fsl_cx * f = NULL;
  cwal_value * v;
  int rc;
  fsl_cx_init_opt init = fsl_cx_init_opt_empty;
  s2_engine * se = s2_engine_from_args(args);
  assert(se);

  init.output.out = fsl_output_f_cwal_output;
  init.output.flush = fsl_flush_f_cwal_out;
  init.output.state.state = args->engine;
  if(args->argc && cwal_props_can(args->argv[0])){
    init.config.traceSql =
      cwal_value_get_bool(cwal_prop_get(args->argv[0],
                                        "traceSql", 8));
  }
  rc = fsl_cx_init( &f, &init );
  if(rc){
    fsl_cx_finalize( f );
    return cb_toss(args, FSL_RC_ERROR,
                   "Fossil context initialization failed "
                   "with code #%d (%s).", rc,
                   fsl_rc_cstr(rc));
  }
  v = cwal_new_native_value(args->engine, f,
                            cwal_finalizer_f_fsl_cx,
                            FSL_TYPEID(fsl_cx));
  if(!v){
    fsl_cx_finalize( f );
    return CWAL_RC_OOM;
  }
  cwal_value_prototype_set( v, fsl_cx_prototype(se) );
  fsl_cx_db_prop( f, v, se, 1 ) /* initialize this->db */;
  *rv  = v;
  return 0;
}

/**
   If parent contains a Db property with the given name, that property
   is disconnected from its native and removed from parent, but its Db
   finalizer is not called (ownership lies elsewhere for the fsl_db
   parts of properties). The db's Fossil context pointer (if any) is
   also set to 0.

*/
static void cb_fsl_clear_handles(cwal_value * parent,
                                 char const * dbName){
  cwal_engine * e = cwal_value_engine(parent);
  cwal_size_t const dbNameLen = cwal_strlen(dbName);
  cwal_value * v = e ? cwal_prop_get(parent, dbName, dbNameLen) : NULL;
  assert(e);
  if(v){
    cwal_native * nat = cwal_value_native_part(e, v, FSL_TYPEID(fsl_db));
    fsl_db * db = nat ? cwal_native_get( nat, FSL_TYPEID(fsl_db) ) : NULL;
    assert(nat ? !!db : 1);
    if(db){
      db->f = 0 /* avoid holding a stale pointer. */;
      cwal_native_clear(nat, 0);
      cwal_prop_unset(parent, dbName, dbNameLen);
    }
  }
}

static int cb_fsl_cx_close( cwal_callback_args const * args,
                         cwal_value **rv ){
  cwal_value * dbNs;
  THIS_F;
  /* It's important that we clear the Value/Native bindings
     to all of the context's databases because clients can do this:

     var db = f.db.repo;
     f.close();'
     db.something(...)

     Which, if we're not careful here, can lead to them having
     not only a stale fsl_db handle, but one which points back
     to a stale fsl_cx pointer.

     Thank you, valgrind.
  */
  dbNs = fsl_cx_db_prop(f, natV, se, 0);
  if(dbNs){
    cb_fsl_clear_handles(dbNs, "repo");
    cb_fsl_clear_handles(dbNs, "checkout");
    cb_fsl_clear_handles(dbNs, "config");
    cb_fsl_clear_handles(natV, "db");
  }
  if(fsl_cx_db_config(f)){
    fsl_config_close(f);
  }
  if(fsl_cx_db_ckout(f)){
    fsl_ckout_close(f);
    /* also closes its repo db */
  }else if(fsl_cx_db_repo(f)){
    fsl_repo_close(f);
  }
  *rv = args->self;
  return 0;
}


static int cb_fsl_cx_finalize( cwal_callback_args const * args,
                              cwal_value **rv ){
  THIS_F;
  if(fsl_cx_db_prop(f, args->self, se, 0)){
    cb_fsl_cx_close(args, rv);
  }
  cwal_native_clear( nat, 1 );
  return 0;
}

static int cb_fsl_cx_user_name( cwal_callback_args const * args,
                             cwal_value **rv ){
  char const * uname;
  THIS_F;
  if(! (uname = fsl_cx_user_get(f)) ){
    uname = fsl_guess_user_name();
  }
  *rv = (uname && *uname)
    ? cwal_new_string_value(args->engine, uname, cwal_strlen(uname))
    : cwal_value_undefined();
  return *rv ? 0 : CWAL_RC_OOM;
}



static int cb_fsl_cx_resolve_sym( cwal_callback_args const * args,
                                  cwal_value **rv,
                                  char toUuid ){
  char * uuid = NULL;
  char const * sym;
  int rc;
  cwal_int_t argRid = 0;
  fsl_id_t rid = 0;
  THIS_F;
  sym = args->argc
    ? cwal_value_get_cstr(args->argv[0], NULL)
    : NULL;
  if((!sym || !*sym) && args->argc){
    argRid = cwal_value_get_integer(args->argv[0]);
  }
  if(argRid<=0 && (!sym || !*sym)){
    return cb_toss(args, FSL_RC_MISUSE,
                   "Expecting a positive integer or non-empty "
                   "string argument.");
  }
  rc = toUuid
    ? fsl_sym_to_uuid( f, sym, FSL_SATYPE_ANY, &uuid, &rid )
    : fsl_sym_to_rid( f, sym, FSL_SATYPE_ANY, &rid );
  if(rc){
    /* Undecided: throw or return undefined if no entry found? */
#if 1
    if(FSL_RC_NOT_FOUND==rc){
      *rv = cwal_value_undefined();
      rc = 0;
    }else{
      rc = cb_toss_fsl(args, f);
    }
#else
    rc = cb_toss_fsl(args, f);
#endif
  }else{
    if(toUuid){
      assert(uuid);
      *rv = cwal_new_string_value(args->engine, uuid, cwal_strlen(uuid));
      fsl_free(uuid);
      if(!*rv){
        rc = CWAL_RC_OOM;
      }
    }else{
      assert(!uuid);
      assert(rid>0);
      *rv = cwal_new_integer(args->engine, (cwal_int_t)rid);
      if(!*rv) rc = CWAL_RC_OOM;
    }
  }
  return rc;
}


static int cb_fsl_cx_sym2uuid( cwal_callback_args const * args,
                                   cwal_value **rv ){
  return cb_fsl_cx_resolve_sym(args, rv, 1);
}

static int cb_fsl_cx_sym2rid( cwal_callback_args const * args,
                              cwal_value **rv ){
  return cb_fsl_cx_resolve_sym(args, rv, 0);
}

static int cb_fsl_cx_content_get( cwal_callback_args const * args,
                                  cwal_value **rv ){
  int rc;
  fsl_id_t rid = 0;
  char const * sym = NULL;
  fsl_buffer fbuf = fsl_buffer_empty;
  THIS_F;
  if(!args->argc) goto usage;
  else{
    cwal_value * arg = args->argv[0];
    if(cwal_value_is_integer(arg)){
      rid = (fsl_id_t)cwal_value_get_integer(arg);
      if(rid<=0) goto usage;
    }else{
      sym = cwal_value_get_cstr(arg, NULL);
      if(!sym || !*sym) goto usage;
    }
  }
  rc = (rid>0)
    ? fsl_content_get(f, rid, &fbuf)
    : fsl_content_get_sym(f, sym, &fbuf);
  if(rc){
    assert(f->error.code);
    rc = cb_toss_fsl(args, f);
  }else{
    cwal_buffer * cbuf = cwal_new_buffer(args->engine, 0);
    if(!cbuf) rc = CWAL_RC_OOM;
    else{
      cwal_value * cbufV = cwal_buffer_value(cbuf);
      cwal_value_ref(cbufV);
      rc = fsl_buffer_to_cwal_buffer(args->engine, &fbuf, cbuf);
      if(rc){
        cwal_value_unref(cbufV);
      }else{
        cwal_value_unhand(cbufV);
        *rv = cbufV;
      }
    }
  }
  fsl_buffer_clear(&fbuf);
  return rc;
  usage:
  return cb_toss(args, FSL_RC_MISUSE,
                 "Expecting one symbolic name (string) or "
                 "RID (positive integer) argument.");
}

typedef struct card_visitor_F_to_object {
  cwal_engine * e;
  cwal_array * dest;
} card_visitor_F_to_object;

/**
   fsl_card_F_visitor_f() impl which converts fc to a cwal Object
   representation and appends it to
   ((card_visitor_F_to_object*)state)->dest.
*/
static int fsl_card_F_visitor_fc_to_object(fsl_card_F const * fc,
                                           void * state){
  card_visitor_F_to_object * vs = (card_visitor_F_to_object*)state;
  int rc;
  cwal_value * ov = cwal_new_object_value(vs->e);
  char const * typeLabel = NULL;
  if(!ov) return CWAL_RC_OOM;
  cwal_value_ref(ov);
  rc = cwal_array_append(vs->dest, ov);
  cwal_value_unref(ov);
  if(rc){
    return rc;
  }
#define vset(K,KL,V) cwal_prop_set(ov, K, KL, V)
  vset("name", 4, cwal_new_string_value(vs->e, fc->name,
                                        cwal_strlen(fc->name)));
  if(fc->priorName){
    vset("priorName", 9,
         cwal_new_string_value(vs->e, fc->priorName,
                               cwal_strlen(fc->priorName)));
  }
  if(fc->uuid){
    vset("uuid", 4, cwal_new_string_value(vs->e, fc->uuid,
                                          cwal_strlen(fc->uuid)));
  }
  switch(fc->perm){
    case FSL_FILE_PERM_EXE: typeLabel = "x"; break;
    case FSL_FILE_PERM_LINK: typeLabel = "l"; break;
    default: typeLabel = "f";
  }
  vset("perm", 4, cwal_new_string_value(vs->e, typeLabel, 1)
       /* good case for cwal's string interning! */);
#undef vset
  return rc;
}

/**
   Converts a fsl_deck to an Object representation, assigning *rv
   to the new object (new, with no live references).

   Must return a CWAL_RC value, not FSL_RC!
*/
static int s2_deck_to_object( cwal_engine * e, fsl_deck * mf,
                              char includeBaselineObj,
                              cwal_value ** rv ){
  int rc = 0;
  cwal_value * ov = NULL;
  cwal_value * card = NULL;
  cwal_array * ar = NULL;
  cwal_value * av = NULL;
  cwal_size_t i;
  assert(e);
  assert(mf);
  assert(mf->uuid);
  assert(mf->rid);
  rc = fsl_deck_F_rewind(mf);
  if(rc) goto end;
  ov = cwal_new_object_value(e);
  if(!ov) return CWAL_RC_OOM;
  cwal_value_ref(ov);
#define vset2(OBJ,K,V) cwal_prop_set(OBJ, K, cwal_strlen(K), V)
#define dset(K,V) vset2(ov,K,V)
#define cset(K,V) vset2(card,K,V)
#define vornull(VP,V) ((VP) ? (V) : cwal_value_null())
#define strval2(CSTR,LEN) vornull((CSTR), cwal_new_string_value(e, (char const *)(CSTR), (cwal_size_t)(LEN)))
#define strval(CSTR) strval2((CSTR), cwal_strlen(CSTR))
  dset("type", cwal_new_integer(e, mf->type));
  dset("rid", cwal_new_integer(e, (cwal_int_t)mf->rid));
  dset("uuid", strval2(mf->uuid, cwal_strlen(mf->uuid)));
  
  if(mf->A.src){
    card = cwal_new_object_value(e);
    dset("A", card);
    cset("name", strval(mf->A.name));
    cset("tgt", strval(mf->A.tgt));
    cset("uuid", strval2(mf->A.src, cwal_strlen(mf->A.src)));
  }
  if(mf->B.uuid){
    card = cwal_new_object_value(e);
    dset("B", card);
    cset("uuid", strval2(mf->B.uuid,cwal_strlen(mf->B.uuid)));
    if(includeBaselineObj){
      rc = fsl_deck_F_rewind(mf) /* load baseline if needed */;
      if(!rc){
        cwal_value * v = NULL;
        assert(mf->B.baseline);
        rc = s2_deck_to_object( e, mf->B.baseline, 0, &v );
        if(v){
          cset("baseline", v);
        }
        else{
          assert(rc);
          goto end;
        }
      }
    }
  }
  if(mf->C){
    dset("C", strval(mf->C));
  }
  if(mf->D > 0){
    dset("D", cwal_new_double(e, mf->D));
  }
  if(mf->E.julian > 0){
    card = cwal_new_object_value(e);
    dset("E", card);
    cset("julian", cwal_new_double(e, mf->E.julian));
    cset("uuid", strval2(mf->E.uuid, cwal_strlen(mf->E.uuid)));
  }
  if(mf->F.used || mf->B.uuid){
    card_visitor_F_to_object vstate;
    cwal_size_t const reserveSize = mf->F.used
      + (mf->B.baseline ? mf->B.baseline->F.used : 0);
    assert(fsl_card_is_legal(mf->type, 'F'));
    ar = cwal_new_array(e);
    card = av = cwal_array_value(ar);
    dset("F", card);
    cwal_array_reserve(ar, reserveSize);
    vstate.e = e;
    vstate.dest = ar; 
    rc = fsl_deck_F_foreach(mf, fsl_card_F_visitor_fc_to_object,
                            &vstate);
    if(rc) goto end;
  }/* end of F-card */
  if(mf->G){
    dset("G", strval2(mf->G, cwal_strlen(mf->G)));
  }
  if(mf->H){
    dset("H", strval(mf->H));
  }
  if(mf->I){
    dset("I", strval2(mf->I, cwal_strlen(mf->I)));
  }
#define append(V) cwal_array_append(ar, (V))
  if(mf->J.used){
    ar = cwal_new_array(e);
    card = av = cwal_array_value(ar);
    dset("J", card);
    append(strval2("TODO",4));
    for(i = 0; i < mf->J.used; ++i ){
      /* TODO */
    }
  }
  if(mf->K){
    dset("K", strval2(mf->K, cwal_strlen(mf->K)));
  }
  if(mf->L){
    dset("L", strval(mf->L));
  }
  
  if(mf->M.used){
    ar = cwal_new_array(e);
    card = av = cwal_array_value(ar);
    dset("M", card);
    for(i = 0; i < mf->M.used; ++i ){
      fsl_uuid_cstr uuid = (fsl_uuid_cstr)mf->M.list[i];
      assert(uuid);
      append(strval2(uuid,cwal_strlen(uuid)));
    }
  }

  if(mf->P.used){
    ar = cwal_new_array(e);
    card = av = cwal_array_value(ar);
    dset("P", card);
    for(i = 0; i < mf->P.used; ++i ){
      fsl_uuid_cstr uuid = (fsl_uuid_cstr)mf->P.list[i];
      assert(uuid);
      append(strval2(uuid,cwal_strlen(uuid)));
    }
  }

  if(mf->Q.used){
    ar = cwal_new_array(e);
    card = av = cwal_array_value(ar);
    dset("Q", card);
    append(strval2("TODO",4));
    for(i = 0; i < mf->P.used; ++i ){
      fsl_card_Q const * qc = (fsl_card_Q const *)mf->Q.list[i];
      assert(qc);
      /* TODO */
    }
  }
  
  if(mf->U){
      dset("U", strval(mf->U));
  }

  if(mf->W.used){
    dset("W", vornull(mf->W.used, strval2(mf->W.mem,mf->W.used)));
  }else if(0 < fsl_card_is_legal(mf->type, 'W') /* required card */){
    /* corner-case: empty wiki page */
    dset("W", strval2("",0));
  }

#undef append
#undef strval
#undef strval2
#undef vornull
#undef cset
#undef dset
#undef vset2
  end:
  if(!rc && rv){
    assert(ov);
    cwal_value_unhand(ov);
    *rv = ov;
  }
  else if(ov){
    cwal_value_unref(ov);
  }
  return rc;
}

/**
   Script usage:

   const deck = Fossil.loadManifest(
       version [, includeBaselineObject]

   The deck object's struct matches fsl_deck pretty
   closely.
*/
static int cb_fsl_cx_deck_load( cwal_callback_args const * args,
                                cwal_value **rv ){
  int rc;
  fsl_deck mf = fsl_deck_empty;
  fsl_id_t rid = 0;
  char const * sym = NULL;
  fsl_satype_e mfType = FSL_SATYPE_ANY;
  char includeBaselineObj;
  THIS_F;
  if(!args->argc) goto usage;
  else if(cwal_value_is_number(args->argv[0])){
    rid = (fsl_id_t)cwal_value_get_integer(args->argv[0]);
    if(rid<=0) goto usage;
  }else{
    sym = cwal_value_get_cstr(args->argv[0], NULL);
    if(!sym || !*sym) goto usage;
  }
  includeBaselineObj = (args->argc>1)
    ? cwal_value_get_bool(args->argv[1])
    : 0;
  /* TODO: script mapping for fsl_satype_e values for 3nd arg.
   */
  rc = sym
    ? fsl_deck_load_sym(f, &mf, sym, mfType)
    : fsl_deck_load_rid(f, &mf, rid, mfType)
    ;
  if(rc){
    if(f->error.code) rc = cb_toss_fsl(args, f);
    else{
      rc = sym
        ? cb_toss(args, rc, "Loading manifest '%s' failed with code %d/%s",
                  sym, rc, fsl_rc_cstr(rc))
        : cb_toss(args, rc, "Loading manifest RID %d failed with code %d/%s",
                  (int)rid, rc, fsl_rc_cstr(rc));
    }
    goto end;
  }

  rc = s2_deck_to_object(args->engine, &mf,
                         includeBaselineObj, rv);
  end:
  fsl_deck_finalize(&mf);
  return rc;
  usage:
  return cb_toss(args, FSL_RC_MISUSE,
                 "Expecting one symbolic name (string) or "
                 "RID (positive integer) argument.");
}

/**
   Script usage:

   var x = thisFunc(fromVersion, toVersion [, Object options])

   Options:

   integer contextLines

   integer sbsWidth

   bool html

   bool text

   bool inline

   bool invert

   Result is a Buffer containing the generated diff.
*/
static int cb_fsl_cx_adiff( cwal_callback_args const * args,
                            cwal_value **rv ){
  char const * sym;
  fsl_id_t rid1, rid2;
  cwal_value * arg;
  int diffFlags = 0;
  int rc;
  short contextLines = 3;
  short sbsWidth = 0;
  fsl_buffer c1 = fsl_buffer_empty;
  fsl_buffer c2 = fsl_buffer_empty;
  fsl_buffer delta = fsl_buffer_empty;
  THIS_F;
  if(args->argc<2){
    misuse:
    return cb_toss(args, FSL_RC_MISUSE, "Expecting two artifact ID arguments.");
  }
  /* The v1 arg... */
  arg = args->argv[0];
  if(cwal_value_is_integer(arg)){
    rid1 = (fsl_id_t)cwal_value_get_integer(arg);
  }else if(!(sym = cwal_value_get_cstr(arg,NULL))){
    goto misuse;
  }else{
    rc = fsl_sym_to_rid(f, sym, FSL_SATYPE_ANY, &rid1);
    if(rc) return cb_toss_fsl(args, f);
  }

  /* The v2 arg... */
  arg = args->argv[1];
  if(cwal_value_is_integer(arg)){
    rid2 = (fsl_id_t)cwal_value_get_integer(arg);
  }else if(!(sym = cwal_value_get_cstr(arg,NULL))){
    goto misuse;
  }else{
    rc = fsl_sym_to_rid(f, sym, FSL_SATYPE_ANY, &rid2);
    if(rc) return cb_toss_fsl(args, f);
  }

  assert(rid1>0);
  assert(rid2>0);

  rc = fsl_content_get(f, rid1, &c1);
  if(!rc) rc = fsl_content_get(f, rid2, &c2);
  if(rc){
    rc = cb_toss_fsl(args, f);
    goto end;
  }
  diffFlags = FSL_DIFF_LINENO | FSL_DIFF_SIDEBYSIDE;
  if((args->argc>2) && cwal_props_can((arg = args->argv[2]))){
    do{
      /* Collect diff flags from object */
      cwal_value * v;
      v = cwal_prop_get(arg, "contextLines", 12);
      if(v) contextLines = (short)cwal_value_get_integer(v);
      v = cwal_prop_get(arg, "html", 4);
      if(v && cwal_value_get_bool(v)){
        diffFlags |= FSL_DIFF_HTML;
      }
      v = cwal_prop_get(arg, "inline", 6);
      if(v && cwal_value_get_bool(v)){
        diffFlags &= ~FSL_DIFF_SIDEBYSIDE;
        sbsWidth = 0;
      }
      v = cwal_prop_get(arg, "invert", 6);
      if(v && cwal_value_get_bool(v)){
        diffFlags |= FSL_DIFF_INVERT;
      }
      v = cwal_prop_get(arg, "sbsWidth", 8);
      if(v){
        sbsWidth = (short)cwal_value_get_integer(v);
      }
      v = cwal_prop_get(arg, "text", 4);
      if(v && cwal_value_get_bool(v)){
        diffFlags &= ~FSL_DIFF_HTML;
      }
    } while(0);
  }

  rc = fsl_diff_text(&c1, &c2, fsl_output_f_buffer, &delta,
                     contextLines, sbsWidth, diffFlags);
  if(rc){
    if(FSL_RC_OOM==rc) rc = CWAL_RC_OOM;
    else rc = cb_toss(args, rc, "Error %s while creating diff.",
                      fsl_rc_cstr(rc));
  }else{
    cwal_value * v = cwal_new_buffer_value(args->engine, 0);
    cwal_buffer * bv = v ? cwal_value_buffer_part(args->engine, v) : NULL;
    if(!v){
      rc = CWAL_RC_OOM;
      goto end;
    }else{
      cwal_value_ref(v);
      assert(!bv->mem);
      rc = fsl_buffer_to_cwal_buffer(args->engine, &delta, bv);
      if(rc){
        cwal_value_unref(v);
      }else{
        assert(bv->used == delta.used);
        cwal_value_unhand(v);
        *rv = v;
      }
    }
  }
  end:
  fsl_buffer_clear(&c1);
  fsl_buffer_clear(&c2);
  fsl_buffer_clear(&delta);
  return rc;
}


cwal_value * fsl_stmt_prototype( s2_engine * se ){
  int rc = 0;
  cwal_value * proto;
  cwal_value * v;
  char const * pKey = "Fossil.Stmt";
  proto = s2_prototype_stashed(se, pKey);
  if(proto) return proto;
  proto = cwal_new_object_value(se->e);
  if(!proto){
    rc = CWAL_RC_OOM;
    goto end;
  }
  rc = s2_prototype_stash( se, pKey, proto );
  if(rc) goto end;

#define VCHECK if(!v){ rc = CWAL_RC_OOM; goto end; } (void)0
#define SET(NAME)                                           \
  VCHECK;                                                   \
  cwal_value_ref(v);                                        \
  rc = cwal_prop_set( proto, NAME, cwal_strlen(NAME), v );  \
  cwal_value_unref(v);                                      \
  if(rc) {goto end;} (void)0

  v = cwal_new_string_value(se->e, "Stmt", 4);
  SET("__typename");

  {
    s2_func_def const funcs[] = {
      S2_FUNC2("stepObject", cb_fsl_stmt_step_object),
      S2_FUNC2("stepArray", cb_fsl_stmt_step_array),
      S2_FUNC2("step", cb_fsl_stmt_step),
      S2_FUNC2("rowToObject", cb_fsl_stmt_row_to_object),
      S2_FUNC2("rowToArray", cb_fsl_stmt_row_to_array),
      S2_FUNC2("reset", cb_fsl_stmt_reset),
      S2_FUNC2("get", cb_fsl_stmt_get),
      S2_FUNC2("finalize", cb_fsl_stmt_finalize),
      S2_FUNC2("bind", cb_fsl_stmt_bind),
      s2_func_def_empty_m
    };
    rc = s2_install_functions(se, proto, funcs, 0);
  }
  {
    /* Stmt.each() impl. */
    char const * src =
      "proc(_){"
        "affirm typeinfo(iscallable _) && typeinfo(iscallable _.call); "
        "while(this.step()) false === _.call(this) && break;"
        "return this"
      "}";
      int const srcLen = (int)cwal_strlen(src);
      rc = s2_set_from_script(se, src, srcLen, proto, "each", 4);
  }

#undef SET
#undef VCHECK
  end:
  return rc ? NULL : proto;
}

cwal_value * fsl_db_prototype( s2_engine * se ){
  int rc = 0;
  cwal_value * proto;
  cwal_value * v;
  char const * pKey = "Fossil.Db";
  cwal_engine * e;
  proto = s2_prototype_stashed(se, pKey);
  if(proto) return proto;
  e = s2_engine_engine(se);
  /* cwal_value * fv; */
  assert(se && e);
  proto = cwal_new_object_value(e);
  if(!proto){
    rc = CWAL_RC_OOM;
    goto end;
  }
  rc = s2_prototype_stash( se, pKey, proto );
  if(rc) goto end;
#define VCHECK if(!v){ rc = CWAL_RC_OOM; goto end; } (void)0
#define SET(NAME)                                           \
  VCHECK;                                                   \
  cwal_value_ref(v);                                        \
  rc = cwal_prop_set( proto, NAME, cwal_strlen(NAME), v );  \
  cwal_value_unref(v);                                      \
  if(rc) {goto end; } (void)0
  
  v = cwal_new_string_value(e, "Db", 2);
  SET("__typename");

  {
    s2_func_def const funcs[] = {
      S2_FUNC2("transactionState", cb_fsl_db_trans_state),
      S2_FUNC2("transaction", cb_fsl_db_trans),
      S2_FUNC2("selectValues", cb_fsl_db_select_values),
      S2_FUNC2("selectValue", cb_fsl_db_select_value),
      S2_FUNC2("rollback", cb_fsl_db_trans_rollback),
      S2_FUNC2("prepare", cb_fsl_db_prepare),
      S2_FUNC2("open", cb_fsl_db_ctor),
      S2_FUNC2("lastInsertId", cb_fsl_db_last_insert_id),
      S2_FUNC2("getName", cb_fsl_db_name),
      S2_FUNC2("getFilename", cb_fsl_db_filename),
      S2_FUNC2("execMulti", cb_fsl_db_exec_multi),
      S2_FUNC2("exec", cb_fsl_db_exec),
      S2_FUNC2("each", cb_fsl_db_each),
      S2_FUNC2("commit", cb_fsl_db_trans_commit),
      S2_FUNC2("close", cb_fsl_db_finalize),
      S2_FUNC2("begin", cb_fsl_db_trans_begin),
      s2_func_def_empty_m
    };
    rc = s2_install_functions(se, proto, funcs, 0);
    if(rc) goto end;
  }
  v = cwal_prop_get(proto, "open", 4);
  assert(v && "We just installed this!");
  rc = s2_ctor_method_set(se, proto, cwal_value_get_function(v));
  if(rc) goto end;

#if 0
  /* TODO: port in stepTuple() from the main sqlite3 s2 mod
     and then add these... */     
  {
    /* selectRow(sql,bind,asTuple) impl. */
    char const * src =
      "proc(s,b,t){"/*sql, bind, asTuple*/
        "return this.prepare(s).bind(b)"
        "[t?'stepTuple':'stepObject']()"
        /* return will indirectly finalize() the anonymous stmt. */
      "}";
    rc = s2_set_from_script(se, src, (int)cwal_strlen(src),
                            proto, "selectRow", 9);
    if(rc) goto end;
    /* selectRows(sql,bind,asTuple) impl: */
    src = "proc(s,b,t){"/*sql, bind, asTuple*/
        "const S = this.prepare(s).bind(b), m = t?'stepTuple':'stepObject', rc=[];"
        "var v; while(v=S[m]())rc[]=v; S.finalize();"
        "return rc;"
      "}";
    rc = s2_set_from_script(se, src, (int)cwal_strlen(src),
                            proto, "selectRows", 10);
  }
#endif

  v = fsl_stmt_prototype(se);
  VCHECK;
  if( (rc = cwal_prop_set_with_flags( proto, "Stmt", 4, v,
                                      CWAL_VAR_F_CONST)) ){
    goto end;
  }

#undef SET
#undef VCHECK
  end:
  return rc ? NULL : proto;
}

cwal_value * fsl_cx_prototype( s2_engine * se ){
  int rc = 0;
  cwal_value * proto;
  cwal_value * v;
  char const * pKey = "Fossil.Context";
  cwal_engine * e;
  proto = s2_prototype_stashed(se, pKey);
  if(proto) return proto;
  e = s2_engine_engine(se);
  assert(se && e);
  proto = cwal_new_object_value(e);
  if(!proto){
    rc = CWAL_RC_OOM;
    goto end;
  }
  rc = s2_prototype_stash( se, pKey, proto );
  if(rc) goto end;
#define VCHECK if(!v){ rc = CWAL_RC_OOM; goto end; } (void)0
#define SET(NAME)                                           \
  VCHECK;                                                   \
  cwal_value_ref(v);                                        \
  rc = cwal_prop_set( proto, NAME, cwal_strlen(NAME), v );  \
  cwal_value_unref(v);                                      \
  if(rc) {goto end; } (void)0
  
  v = cwal_new_string_value(e, "Context", 7);
  SET("__typename");
  {
    s2_func_def const funcs[] = {
      S2_FUNC2("symToUuid", cb_fsl_cx_sym2uuid),
      S2_FUNC2("symToRid", cb_fsl_cx_sym2rid),
      S2_FUNC2("openRepo", cb_fsl_repo_open),
      S2_FUNC2("openDb", cb_fsl_db_ctor),
      S2_FUNC2("openConfig", cb_fsl_config_open),
      S2_FUNC2("openCheckout", cb_fsl_ckout_open_dir),
      S2_FUNC2("loginCookieName", cb_fsl_login_cookie_name),
      S2_FUNC2("loadManifest",cb_fsl_cx_deck_load),
      S2_FUNC2("loadBlob", cb_fsl_cx_content_get),
      S2_FUNC2("getUserName", cb_fsl_cx_user_name),
      S2_FUNC2("finalize", cb_fsl_cx_finalize),
      /* S2_FUNC2("closeConfig", cb_fsl_config_close), */
      S2_FUNC2("close", cb_fsl_cx_close),
      S2_FUNC2("artifactDiff", cb_fsl_cx_adiff),
      s2_func_def_empty_m
    };
    rc = s2_install_functions(se, proto, funcs, 0);
    if(rc) goto end;
  }
  rc = s2_ctor_callback_set(se, proto, cb_fsl_cx_ctor);

#undef SET
#undef VCHECK
  end:
  return rc ? NULL : proto;
}


/**
   Script usage:


   const m = globMatches("*.*", "some.string")

   Or:

   const m = "*.*".globMatches(someString)

   (The second form is not installed by default!)

 */
static int cb_strglob( cwal_callback_args const * args,
                       cwal_value **rv ){
  int rc = 0;
  char const * glob =
    (args->argc>0) ? cwal_value_get_cstr(args->argv[0], NULL) : NULL;
  char const * str =
    (args->argc>1) ? cwal_value_get_cstr(args->argv[1], NULL) : NULL;
  char const * sSelf = cwal_value_get_cstr(args->self, NULL);
  if(sSelf){
    str = glob;
    glob = sSelf;
  }
  if(!glob){
    rc = cb_toss(args, FSL_RC_MISUSE,
                 "Expecting (glob,string) arguments "
                 "OR stringInstance.thisFunc(otherString) "
                 "usage.");
  }
  else if(!str){
    *rv = cwal_value_false();
  }else{
    *rv = fsl_str_glob(glob, str)
      ? cwal_value_true() : cwal_value_false();
  }
  return rc;
}

static int cb_is_uuid( cwal_callback_args const * args,
                       cwal_value **rv ){
  cwal_size_t slen = 0;
  char const * str =
    (args->argc>0) ? cwal_value_get_cstr(args->argv[0], &slen) : NULL;
  *rv = (str && fsl_is_uuid(str))
    ? cwal_value_true()
    : cwal_value_false();
  return 0;
}

static fsl_timer_state RunTimer = fsl_timer_state_empty_m;

/**
   Retuns the number of microseconds of _CPU_ time (not wall clock
   time) used since s2_shell_extend() was run.
*/
static int cb_run_timer_fetch( cwal_callback_args const * args,
                               cwal_value **rv ){
  uint64_t t = fsl_timer_fetch(&RunTimer);
  *rv = cwal_new_integer(args->engine, (cwal_int_t)t);
  return *rv ? 0 : CWAL_RC_OOM;
}

static int cb_fsl_delta_create( cwal_callback_args const * args,
                                cwal_value **rv ){
  char const * s1 = NULL;
  char const * s2 = NULL;
  cwal_size_t len1, len2;
  fsl_size_t outLen = 0;
  cwal_buffer * cb;
  cwal_value * cbV = 0;
  int rc;
  if(args->argc>1){
    s1 = cwal_value_get_cstr(args->argv[0], &len1);
    s2 = cwal_value_get_cstr(args->argv[1], &len2);
  }
  if(!s1 || !s2){
    return cb_toss(args, FSL_RC_MISUSE,
                   "Expecting two non-empty string/buffer "
                   "arguments.");
  }
  cb = cwal_new_buffer(args->engine, len2+61);
  if(!cb) return CWAL_RC_OOM;
  cbV = cwal_buffer_value(cb);
  cwal_value_ref(cbV);
  assert(cb->capacity > (len2+60));
  rc = fsl_delta_create( (unsigned char const *)s1, (fsl_size_t)len1,
                         (unsigned char const *)s2, (fsl_size_t)len2,
                         cb->mem, &outLen);
#if 0
  if(!rc){
    /* Resize the buffer to fit. */
    rc = cwal_buffer_resize(args->engine, cb, (cwal_size_t)outLen);
  }
#endif
  if(rc) cwal_value_unref(cbV);
  else{
    cwal_value_unhand(cbV);
    *rv = cbV;
  }
  return rc;
}

static int cb_fsl_delta_applied_len( cwal_callback_args const * args,
                                     cwal_value **rv ){
  char const * src;
  cwal_size_t srcLen;
  fsl_size_t appliedLen = 0;
  int rc;
  src = (args->argc>0)
    ? cwal_value_get_cstr(args->argv[0], &srcLen)
    : NULL;
  if(!src){
    return cb_toss(args, FSL_RC_MISUSE,
                   "Expecting one delta string "
                   "argument.");
  }
  rc = fsl_delta_applied_size((unsigned char const *)src,
                              (fsl_size_t)srcLen, &appliedLen);
  if(rc){
    return cb_toss(args, rc,
                   "Input does not appear to be a "
                   "delta. Error #%d (%s).",
                   rc, fsl_rc_cstr(rc));
  }else{
    *rv = cwal_new_integer(args->engine,
                           (cwal_int_t)appliedLen);
    return *rv ? 0 : CWAL_RC_OOM;
  }
}

static int cb_fsl_delta_apply( cwal_callback_args const * args,
                               cwal_value **rv ){
  char const * s1 = NULL;
  char const * s2 = NULL;
  char const * src;
  char const * delta;
  cwal_size_t len1, len2, srcLen, dLen;
  fsl_size_t appliedLen = 0;
  cwal_buffer * cb;
  cwal_value * cbV;
  int rc;
  if(args->argc>1){
    s1 = cwal_value_get_cstr(args->argv[0], &len1);
    s2 = cwal_value_get_cstr(args->argv[1], &len2);
  }
  if(!s1 || !s2){
    return cb_toss(args, FSL_RC_MISUSE,
                   "Expecting two non-empty string/buffer "
                   "arguments.");
  }
  rc = fsl_delta_applied_size((unsigned char const *)s2,
                              (fsl_size_t)len2, &appliedLen);
  if(!rc){
    src = s1;
    srcLen = len1;
    delta = s2;
    dLen = len2;
  }else{ /* Check if the user perhaps swapped the args. */
    rc = fsl_delta_applied_size((unsigned char const *)s1,
                                (fsl_size_t)len1, &appliedLen);
    if(rc){
      return cb_toss(args, FSL_RC_MISUSE,
                     "Expecting a delta string/buffer as one "
                     "of the first two arguments.");
    }
    src = s2;
    srcLen = len2;
    delta = s1;
    dLen = len1;
  }
  cb = cwal_new_buffer(args->engine, appliedLen+1);
  if(!cb) return CWAL_RC_OOM;
  cbV = cwal_buffer_value(cb);
  cwal_value_ref(cbV);
  assert(cb->capacity > appliedLen);
  rc = fsl_delta_apply( (unsigned char const *)src, (fsl_size_t)srcLen,
                        (unsigned char const *)delta, (fsl_size_t)dLen,
                        cb->mem);
  if(!rc){
    assert(0==cb->mem[appliedLen]);
#if 0
    if(cb->capacity > (cb->used * 4 / 3)){
      rc = cwal_buffer_resize(args->engine, cb, (cwal_size_t)appliedLen);
    }
#endif
  }else{
    rc = cb_toss(args, rc,
                 "Application of delta failed with "
                 "code #%d (%s).", rc,
                 fsl_rc_cstr(rc));
  }
  if(rc) cwal_value_unref(cbV);
  else {
    cwal_value_unhand(cbV);
    *rv = cbV;
  }
  return rc;
}

static int fsl_add_delta_funcs( cwal_engine * e, cwal_value * ns ){
  cwal_value * func;
  cwal_value * v;
  int rc;
  func = cwal_new_object_value(e);
  if(!func) return CWAL_RC_OOM;
  cwal_value_ref(func);
  rc = cwal_prop_set(ns, "delta", 5, func);
  cwal_value_unref(func);
  if(rc){
    return rc;
  }
#define VCHECK if(!v){ rc = CWAL_RC_OOM; goto end; } (void)0
#define SET(NAME)                                              \
  VCHECK;                                                         \
  cwal_value_ref(v);                                               \
  rc = cwal_prop_set( func, NAME, cwal_strlen(NAME), v );          \
  cwal_value_unref(v);                                             \
  if(rc){goto end;}(void)0
#define FUNC(NAME,FP)                    \
  v = cwal_new_function_value( e, FP, 0, 0, 0 );  \
  SET(NAME)
  FUNC("appliedLength", cb_fsl_delta_applied_len);
  FUNC("apply", cb_fsl_delta_apply);
  FUNC("create", cb_fsl_delta_create);
#undef SET
#undef FUNC
#undef VCHECK
  end:
  return rc;
}

/**
   Script usage:

   string canonicalName(string filename [, bool keepSlash = false])
   string canonicalName(string rootDirName, string filename[, bool keepSlash = false])
*/
static int cb_fs_file_canonical( cwal_callback_args const * args,
                                 cwal_value **rv ){
  int rc;
  char const * fn = 0;
  char const * root = 0;
  fsl_buffer buf = fsl_buffer_empty;
  char slash = 0;
  if(1==args->argc){
    fn = cwal_value_get_cstr(args->argv[0], 0);
  }else if(args->argc>1){
    if(cwal_value_is_bool(args->argv[1])){
      fn = cwal_value_get_cstr(args->argv[0], 0);
      slash = cwal_value_get_bool(args->argv[1]) ? 1 : 0;
    }else{
      root = cwal_value_get_cstr(args->argv[0], 0);
      fn = cwal_value_get_cstr(args->argv[1], 0);
      if(args->argc>2) slash = cwal_value_get_bool(args->argv[2]);
    }
  }
  if(!fn) return cb_toss(args, FSL_RC_MISUSE,
                         "Expecting a non-empty string "
                         "argument.");
  rc = fsl_file_canonical_name2( root, fn, &buf, slash );
  if(FSL_RC_OOM==rc) rc = CWAL_RC_OOM;
  else if(!rc){
    *rv = cwal_new_string_value(args->engine,
                                buf.used ? (char const *)buf.mem : 0,
                                (cwal_size_t)buf.used);
    if(!*rv){
      rc = CWAL_RC_OOM;
    }
  }
  fsl_buffer_clear(&buf);
  return rc;
}

/**
   Script binding to fsl_file_dirpart():

   var x = thisFunc(path, leaveSlash=true)
 */
static int cb_fs_file_dirpart( cwal_callback_args const * args,
                               cwal_value **rv ){
  int rc;
  char const * fn;
  fsl_buffer buf = fsl_buffer_empty;
  cwal_size_t fnLen = 0;
  char leaveSlash;
  fn = args->argc
    ? cwal_value_get_cstr(args->argv[0], &fnLen)
    : NULL;
  if(!fn || !*fn) return cb_toss(args, FSL_RC_MISUSE,
                                 "Expecting non-empty string "
                                 "argument.");
  leaveSlash = args->argc>1
    ? cwal_value_get_bool(args->argv[1])
    : 1;

  rc = fsl_file_dirpart(fn, (fsl_size_t)fnLen, &buf, leaveSlash);
  if(FSL_RC_OOM==rc) rc = CWAL_RC_OOM;
  else if(!rc){
    *rv = cwal_new_string_value(args->engine,
                                buf.used ? (char const *)buf.mem : 0,
                                buf.used);
    if(!*rv){
      rc = CWAL_RC_OOM;
    }
  }
  fsl_buffer_clear(&buf);
  return rc;
}

/**
   Script usage:

   const size = Fossil.file.size(filename)
*/
static int cb_fs_file_size( cwal_callback_args const * args,
                            cwal_value **rv ){
  char const * fn;
  fsl_size_t sz;
  fn = args->argc
    ? cwal_value_get_cstr(args->argv[0], NULL)
    : NULL;
  if(!fn) return cb_toss(args, FSL_RC_MISUSE,
                         "Expecting non-empty string "
                         "argument.");
  sz = fsl_file_size(fn);
  if((fsl_size_t)-1 == sz){
    return cb_toss(args, FSL_RC_IO,
                   "Could not stat file: %s", fn);
  }
  *rv = (sz > (int64_t)CWAL_INT_T_MAX)
    ? cwal_new_double(args->engine, (cwal_double_t)sz)
    : cwal_new_integer(args->engine, (cwal_int_t)sz)
    ;
  return *rv ? 0 : CWAL_RC_OOM;
}

/**
   Script usage:

   const tm = Fossil.file.unlink(filename)
*/
static int cb_fs_file_unlink( cwal_callback_args const * args,
                              cwal_value **rv ){
  char const * fn;
  int rc;
  fn = args->argc
    ? cwal_value_get_cstr(args->argv[0], NULL)
    : NULL;
  if(!fn || !*fn) return cb_toss(args, FSL_RC_MISUSE,
                                 "Expecting non-empty string "
                                 "argument.");
  rc = fsl_file_unlink( fn );
  if(!rc) *rv = args->self;
  return rc
    ? cb_toss(args, rc, "Got %s error while "
              "trying to remove file: %s",
              fsl_rc_cstr(rc), fn)
    : 0;
}

static int cb_fs_file_chdir( cwal_callback_args const * args,
                             cwal_value **rv ){
  char const * fn;
  int rc;
  fn = args->argc
    ? cwal_value_get_cstr(args->argv[0], NULL)
    : NULL;
  if(!fn || !*fn) return cb_toss(args, FSL_RC_MISUSE,
                                 "Expecting non-empty string "
                                 "argument.");
  rc = fsl_chdir( fn );
  if(!rc) *rv = args->self;
  return rc
    ? cb_toss(args, rc, "Got %s error while "
              "trying to remove file: %s",
              fsl_rc_cstr(rc))
    : 0;
}

static int cb_fs_file_cwd( cwal_callback_args const * args,
                           cwal_value **rv ){
  int rc;
  enum { BufSize = 512 * 5 };
  fsl_size_t nLen = 0;
  char const slash = args->argc ? cwal_value_get_bool(args->argv[0]) : 0;
  char buf[BufSize] = {0};
  rc = fsl_getcwd(buf, BufSize, &nLen);
  if(rc) return cb_toss(args, rc, "Got error %d (%s) while "
                        "trying to getcwd()",
                        rc, fsl_rc_cstr(rc));
  if(slash && nLen<(fsl_size_t)BufSize){
    buf[nLen++] = '/';
  }
  *rv = cwal_new_string_value(args->engine, buf, (cwal_size_t)nLen);
  return *rv ? 0 : CWAL_RC_OOM;
}


/**
   Script usage:

   const tm = Fossil.file.mtime(filename)
*/
static int cb_fs_file_mtime( cwal_callback_args const * args,
                             cwal_value **rv ){
  char const * fn;
  fsl_time_t sz;
  fn = args->argc
    ? cwal_value_get_cstr(args->argv[0], NULL)
    : NULL;
  if(!fn) return cb_toss(args, FSL_RC_MISUSE,
                         "Expecting non-empty string "
                         "argument.");
  sz = fsl_file_mtime(fn);
  if(-1 == sz){
    return cb_toss(args, FSL_RC_IO,
                   "Could not stat() file: %s", fn);
  }
  *rv = cwal_new_integer(args->engine, (cwal_int_t)sz);
  return *rv ? 0 : CWAL_RC_OOM;
}

/**
   Script usage:

   const bool = Fossil.file.isFile(filename)
*/
static int cb_fs_file_isfile( cwal_callback_args const * args,
                              cwal_value **rv ){
  char const * fn;
  fn = args->argc
    ? cwal_value_get_cstr(args->argv[0], NULL)
    : NULL;
  if(!fn) return cb_toss(args, FSL_RC_MISUSE,
                         "Expecting non-empty string "
                         "argument.");
  *rv = fsl_is_file(fn)
    ? cwal_value_true()
    : cwal_value_false();
  return 0;
}

/**
   Script usage:

   const bool = Fossil.file.isDir(filename)
*/
static int cb_fs_file_isdir( cwal_callback_args const * args,
                             cwal_value **rv ){
  char const * fn;
  fn = args->argc
    ? cwal_value_get_cstr(args->argv[0], NULL)
    : NULL;
  if(!fn) return cb_toss(args, FSL_RC_MISUSE,
                         "Expecting non-empty string "
                         "argument.");
  *rv = (fsl_dir_check(fn) > 0)
    ? cwal_value_true()
    : cwal_value_false();
  return 0;
}


/**
   Script usage:

   bool Fossil.file.access(filename [, bool mustBeADir=false)
*/
static int cb_fs_file_access( cwal_callback_args const * args,
                              cwal_value **rv ){
  char const * fn;
  char checkWrite;
  fn = args->argc
    ? cwal_value_get_cstr(args->argv[0], NULL)
    : NULL;
  if(!fn) return cb_toss(args, FSL_RC_MISUSE,
                         "Expecting non-empty string "
                         "argument.");
  checkWrite = (args->argc>1)
    ? cwal_value_get_bool(args->argv[1])
    : 0;

  *rv = fsl_file_access( fn, checkWrite ? W_OK : F_OK )
    /*0==OK*/
    ? cwal_value_false()
    : cwal_value_true();
  return 0;
}


/**
   Streams a file's contents directly to the script engine's output
   channel, without (unless the file is small) reading the whole file
   into a buffer first.

   Script usage:

   Fossil.file.passthrough(filename)
*/
static int cb_fs_file_passthrough( cwal_callback_args const * args,
                                   cwal_value **rv ){
  char const * fn;
  cwal_size_t len;
  fn = args->argc
    ? cwal_value_get_cstr(args->argv[0], &len)
    : NULL;
  if(!fn){
    return cb_toss(args, FSL_RC_MISUSE,
                   "Expecting non-empty string "
                   "argument.");
  }else if(0!=fsl_file_access(fn, 0)){
    return cb_toss(args, FSL_RC_NOT_FOUND,
                   "Cannot find file: %s", fn);
  }else{
    FILE * fi = fsl_fopen(fn, "r");
    int rc;
    if(!fi){
      rc = cb_toss(args, FSL_RC_IO,
                   "Could not open file for reading: %s",
                   fn);
    }else{
      rc = fsl_stream( fsl_input_f_FILE, fi,
                       fsl_output_f_cwal_output,
                       args->engine );
      fsl_fclose(fi);
    }
    if(!rc) *rv = args->self;
    return rc ? CWAL_RC_IO : 0;
  }
}

/**
   Script binding to fsl_stat(). Returns undefined if
   stat fails, else an object:
   
   {
   name: args->argv[0],
   mtime: unix epoch,
   ctime: unix epoch,
   size: in bytes,
   type: 'file', 'dir', 'link', or 'unknown'
   }

   TODO? Canonicalize the result name?
*/
static int cb_fs_file_stat( cwal_callback_args const * args,
                            cwal_value **rv ){
  char const * fn;
  fn = args->argc
    ? cwal_value_get_cstr(args->argv[0], NULL)
    : NULL;
  if(!fn) return cb_toss(args, FSL_RC_MISUSE,
                         "Expecting non-empty string "
                         "argument.");
  else{
    fsl_fstat fst = fsl_fstat_empty;
    int rc;
    cwal_engine * e = args->engine;
    rc = fsl_stat( fn, &fst, 1 );
    if(rc){
      *rv = cwal_value_undefined();
      rc = 0;
    }else{
      char const * typeName;
      cwal_value * obj = cwal_new_object_value(e);
      switch(fst.type){
        case FSL_FSTAT_TYPE_DIR: typeName = "dir"; break;
        case FSL_FSTAT_TYPE_FILE: typeName = "file"; break;
        case FSL_FSTAT_TYPE_LINK: typeName = "link"; break;
        case FSL_FSTAT_TYPE_UNKNOWN:
        default:
          typeName = "unknown";
          break;
      }
      cwal_prop_set(obj, "type", 4, cwal_new_string_value(e, typeName,
                                                          cwal_strlen(typeName)));
      cwal_prop_set(obj, "name", 4, args->argv[0]);
      cwal_prop_set(obj, "size", 4, cwal_new_integer(e, (cwal_int_t)fst.size));
      cwal_prop_set(obj, "mtime", 5, cwal_new_integer(e, (cwal_int_t)fst.mtime));
      cwal_prop_set(obj, "ctime", 5, cwal_new_integer(e, (cwal_int_t)fst.ctime));
      *rv = obj;
      /* See how little code it is if we ignore malloc errors? */
    }
    return rc;
  }
}

static int fsl_add_file_funcs( cwal_engine * e, cwal_value * ns ){
  cwal_value * func;
  cwal_value * v;
  int rc;
  func = cwal_new_object_value(e);
  if(!func) return CWAL_RC_OOM;
  cwal_value_ref(func);
  rc = cwal_prop_set(ns, "file", 4, func);
  cwal_value_unref(func);
  if(rc){
    return rc;
  }
    
#define VCHECK if(!v){ rc = CWAL_RC_OOM; goto end; } (void)0
#define SET(NAME)                                              \
  VCHECK;                                                      \
  cwal_value_ref(v);                                           \
  rc = cwal_prop_set( func, NAME, cwal_strlen(NAME), v );      \
  cwal_value_unref(v);                                         \
  if(rc) {goto end;}(void)0
#define FUNC(NAME,FP)                        \
  v = cwal_new_function_value( e, FP, 0, 0, 0 );    \
  SET(NAME)

  FUNC("canonicalName", cb_fs_file_canonical);
  FUNC("chdir", cb_fs_file_chdir);
  FUNC("currentDir", cb_fs_file_cwd);
  FUNC("dirPart", cb_fs_file_dirpart);
  FUNC("isAccessible", cb_fs_file_access);
  FUNC("isDir", cb_fs_file_isdir);
  FUNC("isFile", cb_fs_file_isfile);
  FUNC("mtime", cb_fs_file_mtime);
  FUNC("passthrough", cb_fs_file_passthrough);
  FUNC("size", cb_fs_file_size);
  FUNC("stat", cb_fs_file_stat);
  FUNC("unlink", cb_fs_file_unlink);
    
#undef SET
#undef FUNC
#undef VCHECK
    end:

    return rc;
}

static int cb_time_now( cwal_callback_args const * args,
                        cwal_value **rv ){
  char asJulian = 0;
  time_t now;
  asJulian = args->argc && cwal_value_get_bool(args->argv[0]);
  time(&now);
  *rv = asJulian
    ? cwal_new_double(args->engine,
                      fsl_unix_to_julian((fsl_time_t)now))
    : cwal_new_integer(args->engine,
                       (cwal_int_t)now)
    ;
  return *rv ? 0 : CWAL_RC_OOM;
}


static int cb_unix_to_julian( cwal_callback_args const * args,
                              cwal_value **rv ){
  cwal_int_t tm;
  if(!args->argc){
    return cb_toss(args, FSL_RC_MISUSE,
                   "Expecting one integer argument.");
  }
  tm = cwal_value_get_integer(args->argv[0]);
  *rv = cwal_new_double(args->engine,
                        fsl_unix_to_julian( (fsl_time_t)tm ));
  return *rv ? 0 : CWAL_RC_OOM;
}

static int cb_julian_to_unix( cwal_callback_args const * args,
                              cwal_value **rv ){
  cwal_double_t tm;
  if(!args->argc){
    return cb_toss(args, FSL_RC_MISUSE,
                   "Expecting one double argument.");
  }
  tm = cwal_value_get_double(args->argv[0]);
  *rv = cwal_new_integer(args->engine,
                         fsl_julian_to_unix( (double)tm ));
  return *rv ? 0 : CWAL_RC_OOM;
}

static int cb_julian_to_iso8601( cwal_callback_args const * args,
                                 cwal_value **rv ){
  cwal_double_t tm;
  char includeMs;
  char buf[24];
  if(!args->argc){
    return cb_toss(args, FSL_RC_MISUSE,
                   "Expecting one double argument.");
  }
  tm = cwal_value_get_double(args->argv[0]);
  includeMs = (args->argc>1) ? cwal_value_get_bool(args->argv[1]) : 0;
  if(fsl_julian_to_iso8601(tm, buf, includeMs)){
    *rv = cwal_new_string_value(args->engine, buf, includeMs ? 23 : 19);
    return *rv ? 0 : CWAL_RC_OOM;
  }else{
    return cb_toss(args, FSL_RC_MISUSE,
                   "Invalid Julian date value.");
  }
}

static int cb_julian_to_human( cwal_callback_args const * args,
                               cwal_value **rv ){
  cwal_double_t tm;
  char includeMs;
  char buf[24];
  if(!args->argc){
    return cb_toss(args, FSL_RC_MISUSE,
                   "Expecting one double argument.");
  }
  includeMs = (args->argc>1) ? cwal_value_get_bool(args->argv[1]) : 0;
  tm = cwal_value_get_double(args->argv[0]);
  if(fsl_julian_to_iso8601(tm, buf, includeMs)){
    assert('T' == buf[10]);
    buf[10] = ' ';
    *rv = cwal_new_string_value(args->engine, buf, includeMs ? 23 : 19);
    return *rv ? 0 : CWAL_RC_OOM;
  }else{
    return cb_toss(args, FSL_RC_MISUSE,
                   "Invalid Julian data value.");
  }
}

static int fsl_add_time_funcs( cwal_engine * e, cwal_value * ns ){
  cwal_value * func;
  cwal_value * v;
  int rc;
  func = cwal_new_object_value(e);
  if(!func) return CWAL_RC_OOM;
  cwal_value_ref(func);
  rc = cwal_prop_set(ns, "time", 4, func);
  cwal_value_unref(func);
  if(rc){
    return rc;
  }
    
#define VCHECK if(!v){ rc = CWAL_RC_OOM; goto end; } (void)0
#define SET(NAME)                                              \
  VCHECK;                                                         \
  cwal_value_ref(v);                                           \
  rc = cwal_prop_set( func, NAME, cwal_strlen(NAME), v );   \
  cwal_value_unref(v);                                      \
  if(rc) {goto end;} (void)0
#define FUNC(NAME,FP)                   \
  v = cwal_new_function_value( e, FP, 0, 0, 0 );    \
  SET(NAME)

  FUNC("julianToHuman", cb_julian_to_human);
  FUNC("julianToISO8601", cb_julian_to_iso8601);
  FUNC("julianToUnix", cb_julian_to_unix);
  FUNC("now", cb_time_now);
  FUNC("unixToJulian", cb_unix_to_julian);
  FUNC("cpuTime", cb_run_timer_fetch);

#undef SET
#undef FUNC
#undef VCHECK
  end:
  return rc;
}

static int cb_fsl_rc_cstr( cwal_callback_args const * args,
                           cwal_value **rv ){
  if(!args->argc){
    return cb_toss(args,CWAL_RC_MISUSE,
                   "Expecting a single integer argument (FSL_RC_xxx value).");
  }else{
    cwal_int_t const i = cwal_value_get_integer(args->argv[0]);
    char const * msg = fsl_rc_cstr((int)i);
    *rv = msg
      ? cwal_new_xstring_value(args->engine, msg, cwal_strlen(msg))
      : cwal_value_undefined();
    return *rv ? 0 : CWAL_RC_OOM;
  }
}

static cwal_value * s2_fsl_ns( s2_engine * se ){
  int rc = 0;
  cwal_value * ns;
  cwal_value * v;
  char const * pKey = "Fossil";
  cwal_engine * e;
  ns = s2_prototype_stashed(se, pKey);
  if(ns) return ns;
  e = s2_engine_engine(se);
  assert(se && e);
  ns = cwal_new_object_value(e);
  if(!ns){
    rc = CWAL_RC_OOM;
    goto end;
  }
  rc = s2_prototype_stash( se, pKey, ns );
  if(rc) goto end;
#define VCHECK if(!v){ rc = CWAL_RC_OOM; goto end; } (void)0
#define SET(NAME)                                       \
  VCHECK;                                               \
  cwal_value_ref(v);                                    \
  rc = cwal_prop_set( ns, NAME, cwal_strlen(NAME), v ); \
  cwal_value_unref(v);                                  \
  if(rc) {goto end; } (void)0
#define FUNC(NAME,FP)                           \
  v = cwal_new_function_value(e, FP, 0, 0, 0 ); \
  SET(NAME)
  
  v = cwal_new_string_value(e, "Fossil", 6);
  SET("__typename");

  FUNC("globMatches", cb_strglob);
  FUNC("isUuid", cb_is_uuid);
  FUNC("rcString", cb_fsl_rc_cstr);

  v = fsl_db_prototype(se);
  VCHECK;
  if( (rc = cwal_prop_set_with_flags( ns, "Db", 2, v,
                                      CWAL_VAR_F_CONST)) ){
    goto end;
  }

  v = fsl_cx_prototype(se);
  VCHECK;
  if( (rc = cwal_prop_set_with_flags( ns, "Context", 7, v,
                                      CWAL_VAR_F_CONST)) ){
    goto end;
  }

  if( (rc = fsl_add_delta_funcs(e, ns)) ) goto end;
  if( (rc = fsl_add_file_funcs(e, ns)) ) goto end;
  if( (rc = fsl_add_time_funcs(e, ns)) ) goto end;

#undef SET
#undef FUNC
#undef VCHECK
  end:
  return rc ? NULL : ns;
}

/**
    We copy fsl_lib_configurable.allocator as a base allocator.
*/
static fsl_allocator fslAllocOrig;

/**
    Proxies fslAllocOrig() and abort()s on OOM conditions.
 */
static void * fsl_realloc_f_failing(void * state, void * mem, fsl_size_t n){
  void * rv = fslAllocOrig.f(fslAllocOrig.state, mem, n);
  if(n && !rv){
    fprintf(stderr,"\nOUT OF MEMORY\n");
    fflush(stderr);
    s2_fatal(CWAL_RC_OOM,"Out of memory.");
  }
  return rv;
}

/**
    Replacement for fsl_lib_configurable.allocator which abort()s on OOM.
    Why? Because fossil(1) has shown how much that can simplify error
    checking in an allocates-often API.
 */
static const fsl_allocator fcli_allocator = {
fsl_realloc_f_failing,
NULL/*state*/
};


/**
   Gets called by s2sh's init phase to install our bindings.
*/
int s2_shell_extend(s2_engine * se, int argc, char const * const * argv){
  static char once = 0;
  int rc = 0;
  cwal_value * v /* temp value */;
  cwal_engine * e = s2_engine_engine(se)
    /* cwal_engine is used by the core language-agnostic script engine
       (cwal). s2_engine is a higher-level abstraction which uses
       cwal_engine to (A) provide a Value type system and (B) manage
       the lifetimes of memory allocated on behalf of the scripting
       language (s2). s2 has to take some part in managing the
       lifetimes, but it basically just sets everything up how cwal
       expects it to be, and lets cwal do the hard parts wrt memory
       management.
    */
    ;
  if(once){
    assert(!"libfossil/s2 bindings initialized more than once!");
    s2_fatal(CWAL_RC_MISUSE,
             "libfossil/s2 bindings initialized more than once!");
  }
  fslAllocOrig = fsl_lib_configurable.allocator;
  fsl_lib_configurable.allocator = fcli_allocator
    /* This MUST be done BEFORE the fsl API allocates
       ANY memory!
    */;

  if(0){
    /* force hash-based scope property storage for future scopes */
    int32_t featureFlags = cwal_engine_feature_flags(e, -1);
    featureFlags |= CWAL_FEATURE_SCOPE_STORAGE_HASH;
    cwal_engine_feature_flags(e, featureFlags);
  }

#define VCHECK if(!v && (rc = CWAL_RC_OOM)) goto end

  /* MARKER(("Initializing Fossil namespace...\n")); */

  fsl_timer_start(&RunTimer);

  v = s2_fsl_ns(se);
  VCHECK;
#if 1
  if( (rc = s2_define_ukwd(se, "Fossil", 6, v)) ) goto end;
#else
  cwal_scope * dest = cwal_scope_current_get(e)
    /* Where we want to install our functionality to. Note that we're
       not limited to installing in one specific place, but in practice
       that is typical, and the module loader's interface uses that
       approach. */;
  if( (rc = cwal_scope_chain_set_with_flags( dest, 0, "Fossil", 6,
                                             v, CWAL_VAR_F_CONST)) ){
    goto end;
  }
#endif

  {
    /* Extend the built-in Buffer prototype */
    cwal_value * bufProto = s2_prototype_buffer(se);
    assert(bufProto);
#define BFUNC(NAME,FP) cwal_prop_set( bufProto, NAME, cwal_strlen(NAME), \
                                      cwal_new_function_value(se->e, FP, 0, 0, 0))
    BFUNC("md5", cb_buffer_md5_self);
    BFUNC("sha1", cb_buffer_sha1_self);
    BFUNC("sha3", cb_buffer_sha3_self);
  }

#undef VCHECK
  end:
  return rc;    
}

#undef MARKER
#undef THIS_DB
#undef THIS_STMT
#undef THIS_F
