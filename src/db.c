/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
/*
  Copyright 2013-2021 Stephan Beal (https://wanderinghorse.net).


  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the Simplified BSD License (also
  known as the "2-Clause License" or "FreeBSD License".)
  
  This program is distributed in the hope that it will be useful,
  but without any warranty; without even the implied warranty of
  merchantability or fitness for a particular purpose.
  
  *****************************************************************************
  This file contains the fsl_db_xxx() and fsl_stmt_xxx() parts of the
  API.
  
  Maintenance reminders:
  
  When returning dynamically allocated memory to the client, it needs
  to come from fsl_malloc(), as opposed to sqlite3_malloc(), so that
  it is legal to pass to fsl_free().
*/
#include "fossil-scm/fossil.h"
#include "fossil-scm/fossil-internal.h"
#include <assert.h>
#include <stddef.h> /* NULL on linux */
#include <time.h> /* time() and friends */

/* Only for debugging */
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)

#if 0
/**
    fsl_list_visitor_f() impl which requires that obj be NULL or
    a (fsl_st*), which it passed to fsl_stmt_finalize().
 */
static int fsl_list_v_fsl_stmt_finalize(void * obj, void * visitorState ){
  if(obj) fsl_stmt_finalize( (fsl_stmt*)obj );
  return 0;
}
#endif

void fsl_db_clear_strings(fsl_db * db, bool alsoErrorState ){
  fsl_free(db->filename);
  db->filename = NULL;
  fsl_free(db->name);
  db->name = NULL;
  if(alsoErrorState) fsl_error_clear(&db->error);
}

int fsl_db_err_get( fsl_db const * db, char const ** msg, fsl_size_t * len ){
  return db
    ? fsl_error_get(&db->error, msg, len)
    : FSL_RC_MISUSE;
}

fsl_db * fsl_stmt_db( fsl_stmt * stmt ){
  return stmt ? stmt->db : NULL;
}

/**
    Resets db->error state based on the given code and the current
    error string from the db driver. Returns FSL_RC_DB on success,
    some other non-0 value on error (most likely FSL_RC_OOM while
    allocating the error string - that's the only other error case as
    long as db is opened). Results are undefined if !db or db is not
    opened.
 */
static int fsl_err_from_db( fsl_db * db, int dbCode ){
  assert(db && db->dbh);
  db->error.msg.used =0 ;
  return fsl_error_set(&db->error, FSL_RC_DB,
                       "Db error #%d: %s",
                       dbCode, sqlite3_errmsg(db->dbh));
}

char const * fsl_stmt_sql( fsl_stmt * stmt, fsl_size_t * len ){
  return stmt
    ? fsl_buffer_cstr2(&stmt->sql, len)
    : NULL;
}

char const * fsl_db_filename(fsl_db const * db, fsl_size_t * len){
  if(!db) return NULL;
  if(len && db->filename) *len = fsl_strlen(db->filename);
  return db->filename;
}

fsl_id_t fsl_db_last_insert_id(fsl_db *db){
  return (db && db->dbh)
    ? (fsl_id_t)sqlite3_last_insert_rowid(db->dbh)
    : -1;
}

/**
    Cleans up db->beforeCommit and its contents.
 */
static void fsl_db_cleanup_beforeCommit( fsl_db * db ){
  fsl_list_visit( &db->beforeCommit, -1, fsl_list_v_fsl_free, NULL );
  fsl_list_reserve(&db->beforeCommit, 0);
}

fsl_size_t fsl_db_stmt_cache_clear(fsl_db * db){
  fsl_size_t rc = 0;
  if(db && db->cacheHead){
    fsl_stmt * st;
    fsl_stmt * next = 0;
    for( st = db->cacheHead; st; st = next, ++rc ){
      next = st->next;
      st->next = 0;
      fsl_stmt_finalize( st );
    }
    db->cacheHead = 0;
  }
  return rc;
}

int fsl_db_close( fsl_db * db ){
  if(!db) return FSL_RC_MISUSE;
  else{
    void const * allocStamp = db->allocStamp;
    fsl_cx * f = db->f;
    fsl_db_stmt_cache_clear(db);
    if(db->f && db->f->dbMain==db){
      /*
         Horrible, horrible dependency, and only necessary if the
         fsl_cx API gets sloppy or repo/checkout/config DBs are
         otherwised closed improperly (i.e. not via the fsl_cx API).
      */
      assert(0 != db->role);
      f->dbMain = NULL;
    }
    while(db->beginCount>0){
      fsl_db_transaction_end(db, 1);
    }
    if(0!=db->openStatementCount){
      MARKER(("WARNING: %d open statement(s) left on db [%s].\n",
              (int)db->openStatementCount, db->filename));
    }
    if(db->dbh){
      sqlite3_close(db->dbh);
      /* ignoring results in the style of "destructors may not
         throw". */
    }
    fsl_db_clear_strings(db, 1);
    fsl_db_cleanup_beforeCommit(db);
    *db = fsl_db_empty;
    if(&fsl_db_empty == allocStamp){
      fsl_free( db );
    }else{
      db->allocStamp = allocStamp;
      db->f = f;
    }
    return 0;
  }
}

void fsl_db_err_reset( fsl_db * db ){
  if(db && (db->error.code||db->error.msg.used)){
    fsl_error_reset(&db->error);
  }
}


int fsl_db_attach(fsl_db * db, const char *zDbName, const char *zLabel){
  return (db && db->dbh && zDbName && *zDbName && zLabel && *zLabel)
    ? fsl_db_exec(db, "ATTACH DATABASE %Q AS %s", zDbName, zLabel)
    : FSL_RC_MISUSE;
}
int fsl_db_detach(fsl_db * db, const char *zLabel){
  return (db && db->dbh && zLabel && *zLabel)
    ? fsl_db_exec(db, "DETACH DATABASE %s /*%s()*/", zLabel, __func__)
    : FSL_RC_MISUSE;
}

char const * fsl_db_name(fsl_db const * db){
  return db ? db->name : NULL;
}

/**
    Returns the db name for the given role.
 */
const char * fsl_db_role_label(fsl_dbrole_e r){
  switch(r){
    case FSL_DBROLE_CONFIG:
      return "cfg";
    case FSL_DBROLE_REPO:
      return "repo";
    case FSL_DBROLE_CKOUT:
      return "ckout";
    case FSL_DBROLE_MAIN:
      return "main";
    case FSL_DBROLE_NONE:
    default:
      assert(!"cannot happen/not legal");
      return NULL;
  }
}

char * fsl_db_julian_to_iso8601( fsl_db * db, double j,
                                 char msPrecision,
                                 char localTime){
  char * s = NULL;
  fsl_stmt * st = NULL;
  if(db && db->dbh && (j>=0.0)){
    char const * sql;
    if(msPrecision){
      sql = localTime
        ? "SELECT strftime('%%Y-%%m-%%dT%%H:%%M:%%f',?, 'localtime')"
        : "SELECT strftime('%%Y-%%m-%%dT%%H:%%M:%%f',?)";
    }else{
      sql = localTime
        ? "SELECT strftime('%%Y-%%m-%%dT%%H:%%M:%%S',?, 'localtime')"
        : "SELECT strftime('%%Y-%%m-%%dT%%H:%%M:%%S',?)";
    }
    fsl_db_prepare_cached(db, &st, sql);
    if(st){
      fsl_stmt_bind_double( st, 1, j );
      if( FSL_RC_STEP_ROW==fsl_stmt_step(st) ){
        s = fsl_strdup(fsl_stmt_g_text(st, 0, NULL));
      }
      fsl_stmt_cached_yield(st);
    }
  }
  return s;
}

char * fsl_db_unix_to_iso8601( fsl_db * db, fsl_time_t t, char localTime ){
  char * s = NULL;
  fsl_stmt st = fsl_stmt_empty;
  if(db && db->dbh && (t>=0)){
    char const * sql = localTime
      ? "SELECT datetime(?, 'unixepoch', 'localtime')/*%s()*/"
      : "SELECT datetime(?, 'unixepoch')/*%s()*/"
      ;
    int const rc = fsl_db_prepare(db, &st, sql,__func__);
    if(!rc){
      fsl_stmt_bind_int64( &st, 1, t );
      if( FSL_RC_STEP_ROW==fsl_stmt_step(&st) ){
        fsl_size_t n = 0;
        char const * v = fsl_stmt_g_text(&st, 0, &n);
        s = (v&&n) ? fsl_strndup(v, (fsl_int_t)n) : NULL;
      }
      fsl_stmt_finalize(&st);
    }
  }
  return s;
}

enum fsl_stmt_flags_e {
/**
    fsl_stmt::flags bit indicating that fsl_db_preparev_cache() has
    doled out this statement, effectively locking it until
    fsl_stmt_cached_yield() is called to release it.
 */
FSL_STMT_F_CACHE_HELD = 0x01,

/**
   Propagates our intent to "statically" prepare a given statement
   through various internal API calls.
*/
FSL_STMT_F_PREP_CACHE = 0x10
};

int fsl_db_preparev( fsl_db *db, fsl_stmt * tgt, char const * sql, va_list args ){
  if(!db || !tgt || !sql) return FSL_RC_MISUSE;
  else if(!db->dbh){
    return fsl_error_set(&db->error, FSL_RC_NOT_FOUND, "Db is not opened.");
  }else if(!*sql){
    return fsl_error_set(&db->error, FSL_RC_RANGE, "SQL is empty.");
  }else if(tgt->stmt){
    return fsl_error_set(&db->error, FSL_RC_ALREADY_EXISTS,
                         "Error: attempt to re-prepare "
                         "active statement.");
  }
  else{
    int rc;
    fsl_buffer buf = fsl_buffer_empty;
    fsl_stmt_t * liteStmt = NULL;
    rc = fsl_buffer_appendfv( &buf, sql, args );
    if(!rc){
#if 0
      /* Arguably improves readability of some queries.
         And breaks some caching uses. */
      fsl_simplify_sql_buffer(&buf);
#endif
      sql = fsl_buffer_cstr(&buf);
      if(!sql || !*sql){
        rc = fsl_error_set(&db->error, FSL_RC_RANGE,
                           "Input SQL is empty.");
      }else{
        /*
          Achtung: if sql==NULL here, or evaluates to a no-op
          (e.g. only comments or spaces), prepare_v2 succeeds but has
          a NULL liteStmt, which is why we handle the empty-SQL case
          specially. We don't want that specific behaviour leaking up
          through the API. Though doing so would arguably more correct
          in a generic API, for this particular API we have no reason
          to be able to handle empty SQL. Were we do let through
          through we'd have to add a flag to fsl_stmt to tell us
          whether it's really prepared or not, since checking of
          st->stmt would no longer be useful.
        */
        rc = sqlite3_prepare_v3(db->dbh, sql, (int)buf.used,
                                (FSL_STMT_F_PREP_CACHE & tgt->flags)
                                ? SQLITE_PREPARE_PERSISTENT
                                : 0,
                                &liteStmt, NULL);
        if(rc){
          rc = fsl_error_set(&db->error, FSL_RC_DB,
                             "Db statement preparation failed. "
                             "Error #%d: %s. SQL: %.*s",
                             rc, sqlite3_errmsg(db->dbh),
                             (int)buf.used, (char const *)buf.mem);
        }else if(!liteStmt){
          /* SQL was empty. In sqlite this is allowed, but this API will
             disallow this because it leads to headaches downstream.
          */
          rc = fsl_error_set(&db->error, FSL_RC_RANGE,
                             "Input SQL is empty.");
        }
      }
    }
    if(!rc){
      assert(liteStmt);
      ++db->openStatementCount;
      tgt->stmt = liteStmt;
      tgt->db = db;
      tgt->sql = buf /*transfer ownership*/;
      tgt->colCount = sqlite3_column_count(tgt->stmt);
      tgt->paramCount = sqlite3_bind_parameter_count(tgt->stmt);
    }else{
      assert(!liteStmt);
      fsl_buffer_clear(&buf);
      /* TODO: consider _copying_ the error state to db->f->error if
         db->f is not NULL. OTOH, we don't _always_ want to propagate
         a db error to the parent fossil context, and doing so here
         could lead to a "stale" error laying around downstream and
         getting evaluated later on (incorrectly) in a success
         context.
      */
    }
    return rc;
  }
}

int fsl_db_prepare( fsl_db *db, fsl_stmt * tgt, char const * sql, ... ){
  int rc;
  va_list args;
  va_start(args,sql);
  rc = fsl_db_preparev( db, tgt, sql, args );
  va_end(args);
  return rc;
}

int fsl_db_preparev_cached( fsl_db * db, fsl_stmt ** rv,
                            char const * sql, va_list args ){
  int rc = 0;
  fsl_buffer buf = fsl_buffer_empty;
  fsl_stmt * st = NULL;
  fsl_stmt * cs = NULL;
  if(!db || !rv || !sql) return FSL_RC_MISUSE;
  else if(!*sql) return FSL_RC_RANGE;
  rc = fsl_buffer_appendfv( &buf, sql, args );
  if(rc) goto end;
  for( cs = db->cacheHead; cs; cs = cs->next ){
    if(0==fsl_buffer_compare(&buf, &cs->sql)){
      if(cs->flags & FSL_STMT_F_CACHE_HELD){
        rc = fsl_error_set(&db->error, FSL_RC_ACCESS,
                           "Cached statement is already in use. "
                           "Do not use cached statements if recursion "
                           "involving the statement is possible, and use "
                           "fsl_stmt_cached_yield() to release them "
                           "for further (re)use. SQL: %b",
                           &cs->sql);
        goto end;
      }
      cs->flags |= FSL_STMT_F_CACHE_HELD;
      *rv = cs;
      goto end;
    }
  }
  st = fsl_stmt_malloc();
  if(!st){
    rc = FSL_RC_OOM;
    goto end;
  }
  st->flags |= FSL_STMT_F_PREP_CACHE;
  rc = fsl_db_prepare( db, st, "%b", &buf );
  if(rc){
    fsl_free(st);
    st = 0;
  }else{
    st->next = db->cacheHead;
    db->cacheHead = st;
    st->flags = FSL_STMT_F_CACHE_HELD;
    *rv = st;
  }
  end:
  fsl_buffer_clear(&buf);
  return rc;
}

int fsl_db_prepare_cached( fsl_db * db, fsl_stmt ** st, char const * sql, ... ){
  int rc;
  va_list args;
  va_start(args,sql);
  rc = fsl_db_preparev_cached( db, st, sql, args );
  va_end(args);
  return rc;
}

int fsl_stmt_cached_yield( fsl_stmt * st ){
  if(!st || !st->db || !st->stmt) return FSL_RC_MISUSE;
  else if(!(st->flags & FSL_STMT_F_CACHE_HELD)) {
    return fsl_error_set(&st->db->error, FSL_RC_MISUSE,
                         "fsl_stmt_cached_yield() was passed a "
                         "statement which is not marked as cached. "
                         "SQL: %b",
                         &st->sql);
  }else{
    fsl_stmt_reset(st);
    st->flags &= ~FSL_STMT_F_CACHE_HELD;
    return 0;
  }
}

int fsl_db_before_commitv( fsl_db * db, char const * sql,
                           va_list args ){
  int rc = 0;
  char * cp = NULL;
  if(!db || !sql) return FSL_RC_MISUSE;
  else if(!*sql) return FSL_RC_RANGE;
  cp = fsl_mprintfv(sql, args);
  if(cp){
    rc = fsl_list_append(&db->beforeCommit, cp);
    if(rc) fsl_free(cp);
  }else{
    rc = FSL_RC_OOM;
  }
  return rc;
}

int fsl_db_before_commit( fsl_db *db, char const * sql, ... ){
  int rc;
  va_list args;
  va_start(args,sql);
  rc = fsl_db_before_commitv( db, sql, args );
  va_end(args);
  return rc;
}

int fsl_stmt_finalize( fsl_stmt * stmt ){
  if(!stmt) return FSL_RC_MISUSE;
  else{
    void const * allocStamp = stmt->allocStamp;
    fsl_db * db = stmt->db;
    if(db){
      if(stmt->sql.mem){
        /* ^^^ b/c that buffer is set at the same time
           that openStatementCount is incremented.
        */
        --stmt->db->openStatementCount;
      }
      if(allocStamp && db->cacheHead){
        /* It _might_ be cached - let's remove it.
           We use allocStamp as a check here only
           because most statements allocated on the
           heap currently come from caching.
        */
        fsl_stmt * s;
        fsl_stmt * prev = 0;
        for( s = db->cacheHead; s; prev = s, s = s->next ){
          if(s == stmt){
            if(prev){
              assert(prev->next == s);
              prev->next = s->next;
            }else{
              assert(s == db->cacheHead);
              db->cacheHead = s->next;
            }
            s->next = 0;
            break;
          }
        }
      }
    }
    fsl_buffer_clear(&stmt->sql);
    if(stmt->stmt){
      sqlite3_finalize( stmt->stmt );
    }
    *stmt = fsl_stmt_empty;
    if(&fsl_stmt_empty==allocStamp){
      fsl_free(stmt);
    }else{
      stmt->allocStamp = allocStamp;
    }
    return 0;
  }
}

int fsl_stmt_step( fsl_stmt * stmt ){
  if(!stmt || !stmt->stmt) return FSL_RC_MISUSE;
  else{
    int const rc = sqlite3_step(stmt->stmt);
    assert(stmt->db);
    switch( rc ){
      case SQLITE_ROW:
        ++stmt->rowCount;
        return FSL_RC_STEP_ROW;
      case SQLITE_DONE:
        return FSL_RC_STEP_DONE;
      default:
        fsl_error_set(&stmt->db->error, FSL_RC_STEP_ERROR,
                      "sqlite error #%d: %s",
                      rc, sqlite3_errmsg(stmt->db->dbh));
        return FSL_RC_STEP_ERROR;
    }
  }
}

int fsl_db_eachv( fsl_db * db, fsl_stmt_each_f callback,
                  void * callbackState, char const * sql, va_list args ){
  if(!db || !db->dbh || !callback || !sql) return FSL_RC_MISUSE;
  else if(!*sql) return FSL_RC_RANGE;
  else{
    fsl_stmt st = fsl_stmt_empty;
    int rc;
    rc = fsl_db_preparev( db, &st, sql, args );
    if(!rc){
      rc = fsl_stmt_each( &st, callback, callbackState );
      fsl_stmt_finalize( &st );
    }
    return rc;
  }
}

int fsl_db_each( fsl_db * db, fsl_stmt_each_f callback,
                 void * callbackState, char const * sql, ... ){
  int rc;
  va_list args;
  va_start(args,sql);
  rc = fsl_db_eachv( db, callback, callbackState, sql, args );
  va_end(args);
  return rc;
}

int fsl_stmt_each( fsl_stmt * stmt, fsl_stmt_each_f callback,
                   void * callbackState ){
  if(!stmt || !callback) return FSL_RC_MISUSE;
  else{
    int strc;
    int rc = 0;
    char doBreak = 0;
    while( !doBreak && (FSL_RC_STEP_ROW == (strc=fsl_stmt_step(stmt)))){
      rc = callback( stmt, callbackState );
      switch(rc){
        case 0: continue;
        case FSL_RC_BREAK:
          rc = 0;
          /* fall through */
        default:
          doBreak = 1;
          break;
      }
    }
    return rc
      ? rc
      : ((FSL_RC_STEP_ERROR==strc)
         ? FSL_RC_DB
         : 0);
  }
}

int fsl_stmt_reset2( fsl_stmt * stmt, bool resetRowCounter ){
  if(!stmt || !stmt->stmt || !stmt->db) return FSL_RC_MISUSE;
  else{
    int const rc = sqlite3_reset(stmt->stmt);
    if(resetRowCounter) stmt->rowCount = 0;
    assert(stmt->db);
    return rc
      ? fsl_err_from_db(stmt->db, rc)
      : 0;
  }
}

int fsl_stmt_reset( fsl_stmt * stmt ){
  return fsl_stmt_reset2(stmt, 0);
}

int fsl_stmt_col_count( fsl_stmt const * stmt ){
  return (!stmt || !stmt->stmt)
    ? -1
    : stmt->colCount
    ;
}

char const * fsl_stmt_col_name(fsl_stmt * stmt, int index){
  return (stmt && stmt->stmt && (index>=0 && index<stmt->colCount))
    ? sqlite3_column_name(stmt->stmt, index)
    : NULL;
}

int fsl_stmt_param_count( fsl_stmt const * stmt ){
  return (!stmt || !stmt->stmt)
    ? -1
    : stmt->paramCount;
}

int fsl_stmt_bind_fmtv( fsl_stmt * st, char const * fmt, va_list args ){
  int rc = 0, ndx;
  char const * pos = fmt;
  if(!fmt ||
     !(st && st->stmt && st->db && st->db->dbh)) return FSL_RC_MISUSE;
  else if(!*fmt) return FSL_RC_RANGE;
  for( ndx = 1; !rc && *pos; ++pos, ++ndx ){
    if(' '==*pos){
      --ndx;
      continue;
    }
    if(ndx > st->paramCount){
      rc = fsl_error_set(&st->db->error, FSL_RC_RANGE,
                         "Column index %d is out of bounds.", ndx);
      break;
    }
    switch(*pos){
      case '-':
        va_arg(args,void const *) /* skip arg */;
        rc = fsl_stmt_bind_null(st, ndx);
        break;
      case 'i':
        rc = fsl_stmt_bind_int32(st, ndx, va_arg(args,int32_t));
        break;
      case 'I':
        rc = fsl_stmt_bind_int64(st, ndx, va_arg(args,int64_t));
        break;
      case 'R':
        rc = fsl_stmt_bind_id(st, ndx, va_arg(args,fsl_id_t));
        break;
      case 'f':
        rc = fsl_stmt_bind_double(st, ndx, va_arg(args,double));
        break;
      case 's':{/* C-string as TEXT or NULL */
        char const * s = va_arg(args,char const *);
        rc = s
          ? fsl_stmt_bind_text(st, ndx, s, -1, false)
          : fsl_stmt_bind_null(st, ndx);
        break;
      }
      case 'S':{ /* C-string as BLOB or NULL */
        char const * s = va_arg(args,char const *);
        rc = s
          ? fsl_stmt_bind_blob(st, ndx, s, fsl_strlen(s), false)
          : fsl_stmt_bind_null(st, ndx);
        break;
      }
      case 'b':{ /* fsl_buffer as TEXT or NULL */
        fsl_buffer const * b = va_arg(args,fsl_buffer const *);
        rc = (b && b->mem)
          ? fsl_stmt_bind_text(st, ndx, (char const *)b->mem,
                               (fsl_int_t)b->used, false)
          : fsl_stmt_bind_null(st, ndx);
        break;
      }
      case 'B':{ /* fsl_buffer as BLOB or NULL */
        fsl_buffer const * b = va_arg(args,fsl_buffer const *);
        rc = (b && b->mem)
          ? fsl_stmt_bind_blob(st, ndx, b->mem, b->used, false)
          : fsl_stmt_bind_null(st, ndx);
        break;
      }
      default:
        rc = fsl_error_set(&st->db->error, FSL_RC_RANGE,
                           "Invalid format character: '%c'", *pos);
        break;
    }
  }
  return rc;
}

/**
   The elipsis counterpart of fsl_stmt_bind_fmtv().
*/
int fsl_stmt_bind_fmt( fsl_stmt * st, char const * fmt, ... ){
  int rc;
  va_list args;
  va_start(args,fmt);
  rc = fsl_stmt_bind_fmtv(st, fmt, args);
  va_end(args);
  return rc;
}

int fsl_stmt_bind_stepv( fsl_stmt * st, char const * fmt,
                         va_list args ){
  int rc;
  fsl_stmt_reset(st);
  rc = fsl_stmt_bind_fmtv(st, fmt, args);
  if(!rc){
    rc = fsl_stmt_step(st);
    switch(rc){
      case FSL_RC_STEP_DONE:
        rc = 0;
        fsl_stmt_reset(st);
        break;
      case FSL_RC_STEP_ROW:
        /* Don't reset() for ROW b/c that clears the column
           data! */
        break;
      default:
        rc = fsl_error_set(&st->db->error, rc,
                           "Error stepping statement.");
        break;
    }
  }
  return rc;
}

int fsl_stmt_bind_step( fsl_stmt * st, char const * fmt, ... ){
  int rc;
  va_list args;
  va_start(args,fmt);
  rc = fsl_stmt_bind_stepv(st, fmt, args);
  va_end(args);
  return rc;
}


#define BIND_PARAM_CHECK \
  if(!(stmt && stmt->stmt && stmt->db && stmt->db->dbh)) return FSL_RC_MISUSE; else
#define BIND_PARAM_CHECK2 BIND_PARAM_CHECK \
  if(ndx<1 || ndx>stmt->paramCount) return FSL_RC_RANGE; else
int fsl_stmt_bind_null( fsl_stmt * stmt, int ndx ){
  BIND_PARAM_CHECK2 {
    int const rc = sqlite3_bind_null( stmt->stmt, ndx );
    return rc ? fsl_err_from_db(stmt->db, rc) : 0;
  }
}

int fsl_stmt_bind_int32( fsl_stmt * stmt, int ndx, int32_t v ){
  BIND_PARAM_CHECK2 {
    int const rc = sqlite3_bind_int( stmt->stmt, ndx, (int)v );
    return rc ? fsl_err_from_db(stmt->db, rc) : 0;
  }
}

int fsl_stmt_bind_int64( fsl_stmt * stmt, int ndx, int64_t v ){
  BIND_PARAM_CHECK2 {
    int const rc = sqlite3_bind_int64( stmt->stmt, ndx, (sqlite3_int64)v );
    return rc ? fsl_err_from_db(stmt->db, rc) : 0;
  }
}

int fsl_stmt_bind_id( fsl_stmt * stmt, int ndx, fsl_id_t v ){
  BIND_PARAM_CHECK2 {
    int const rc = sqlite3_bind_int64( stmt->stmt, ndx, (sqlite3_int64)v );
    return rc ? fsl_err_from_db(stmt->db, rc) : 0;
  }
}

int fsl_stmt_bind_double( fsl_stmt * stmt, int ndx, double v ){
  BIND_PARAM_CHECK2 {
    int const rc = sqlite3_bind_double( stmt->stmt, ndx, (double)v );
    return rc ? fsl_err_from_db(stmt->db, rc) : 0;
  }
}

int fsl_stmt_bind_blob( fsl_stmt * stmt, int ndx, void const * src,
                        fsl_size_t len, bool makeCopy ){
  BIND_PARAM_CHECK2 {
    int rc;
    rc = sqlite3_bind_blob( stmt->stmt, ndx, src, (int)len,
                            makeCopy ? SQLITE_TRANSIENT : SQLITE_STATIC );
    return rc ? fsl_err_from_db(stmt->db, rc) : 0;
  }
}

int fsl_stmt_bind_text( fsl_stmt * stmt, int ndx, char const * src,
                        fsl_int_t len, bool makeCopy ){
  BIND_PARAM_CHECK {
    int rc;
    if(len<0) len = fsl_strlen((char const *)src);
    rc = sqlite3_bind_text( stmt->stmt, ndx, src, len,
                            makeCopy ? SQLITE_TRANSIENT : SQLITE_STATIC );
    return rc ? fsl_err_from_db(stmt->db, rc) : 0;
  }
}

int fsl_stmt_bind_null_name( fsl_stmt * stmt, char const * param ){
  BIND_PARAM_CHECK{
    return fsl_stmt_bind_null( stmt,
                               sqlite3_bind_parameter_index( stmt->stmt,
                                                             param) );
  }
}

int fsl_stmt_bind_int32_name( fsl_stmt * stmt, char const * param, int32_t v ){
  BIND_PARAM_CHECK {
    return fsl_stmt_bind_int32( stmt,
                                sqlite3_bind_parameter_index( stmt->stmt,
                                                              param),
                                v);
  }
}

int fsl_stmt_bind_int64_name( fsl_stmt * stmt, char const * param, int64_t v ){
  BIND_PARAM_CHECK {
    return fsl_stmt_bind_int64( stmt,
                                sqlite3_bind_parameter_index( stmt->stmt,
                                                              param),
                                v);
  }
}

int fsl_stmt_bind_id_name( fsl_stmt * stmt, char const * param, fsl_id_t v ){
  BIND_PARAM_CHECK {
    return fsl_stmt_bind_id( stmt,
                             sqlite3_bind_parameter_index( stmt->stmt,
                                                           param),
                             v);
  }
}

int fsl_stmt_bind_double_name( fsl_stmt * stmt, char const * param, double v ){
  BIND_PARAM_CHECK {
    return fsl_stmt_bind_double( stmt,
                                 sqlite3_bind_parameter_index( stmt->stmt,
                                                               param),
                                 v);
  }
}

int fsl_stmt_bind_text_name( fsl_stmt * stmt, char const * param,
                             char const * v, fsl_int_t n,
                             bool makeCopy ){
  BIND_PARAM_CHECK {
    return fsl_stmt_bind_text(stmt,
                              sqlite3_bind_parameter_index( stmt->stmt,
                                                            param),
                              v, n, makeCopy);
  }
}

int fsl_stmt_bind_blob_name( fsl_stmt * stmt, char const * param,
                             void const * v, fsl_int_t len,
                             bool makeCopy ){
  BIND_PARAM_CHECK {
    return fsl_stmt_bind_blob(stmt,
                         sqlite3_bind_parameter_index( stmt->stmt,
                                                       param),
                              v, len, makeCopy);
  }
}

int fsl_stmt_param_index( fsl_stmt * stmt, char const * param){
  return (stmt && stmt->stmt)
    ? sqlite3_bind_parameter_index( stmt->stmt, param)
    : -1;
}

#undef BIND_PARAM_CHECK
#undef BIND_PARAM_CHECK2

#define GET_CHECK if(!stmt || !stmt->colCount) return FSL_RC_MISUSE; \
  else if((ndx<0) || (ndx>=stmt->colCount)) return FSL_RC_RANGE; else

int fsl_stmt_get_int32( fsl_stmt * stmt, int ndx, int32_t * v ){
  GET_CHECK {
    if(v) *v = (int32_t)sqlite3_column_int(stmt->stmt, ndx);
    return 0;
  }
}
int fsl_stmt_get_int64( fsl_stmt * stmt, int ndx, int64_t * v ){
  GET_CHECK {
    if(v) *v = (int64_t)sqlite3_column_int64(stmt->stmt, ndx);
    return 0;
  }
}

int fsl_stmt_get_double( fsl_stmt * stmt, int ndx, double * v ){
  GET_CHECK {
    if(v) *v = (double)sqlite3_column_double(stmt->stmt, ndx);
    return 0;
  }
}

int fsl_stmt_get_id( fsl_stmt * stmt, int ndx, fsl_id_t * v ){
  GET_CHECK {
    if(v) *v = (4==sizeof(fsl_id_t))
      ? (fsl_id_t)sqlite3_column_int(stmt->stmt, ndx)
      : (fsl_id_t)sqlite3_column_int64(stmt->stmt, ndx);
    return 0;
  }
}

int fsl_stmt_get_text( fsl_stmt * stmt, int ndx, char const **out,
                       fsl_size_t * outLen ){
  GET_CHECK {
    unsigned char const * t = (out || outLen)
      ? sqlite3_column_text(stmt->stmt, ndx)
      : NULL;
    if(out) *out = (char const *)t;
    if(outLen){
      int const x = sqlite3_column_bytes(stmt->stmt, ndx);
      *outLen = (x>0) ? (fsl_size_t)x : 0;
    }
    return 0;
  }
}

int fsl_stmt_get_blob( fsl_stmt * stmt, int ndx, void const **out,
                       fsl_size_t * outLen ){
  GET_CHECK {
    void const * t = (out || outLen)
      ? sqlite3_column_blob(stmt->stmt, ndx)
      : NULL;
    if(out) *out = t;
    if(outLen){
      if(!t) *outLen = 0;
      else{
        int sz = sqlite3_column_bytes(stmt->stmt, ndx);
        *outLen = (sz>=0) ? (fsl_size_t)sz : 0;
      }
    }
    return 0;
  }
}

#undef GET_CHECK

fsl_id_t fsl_stmt_g_id( fsl_stmt * stmt, int index ){
  fsl_id_t rv = -1;
  fsl_stmt_get_id(stmt, index, &rv);
  return rv;
}
int32_t fsl_stmt_g_int32( fsl_stmt * stmt, int index ){
  int32_t rv = 0;
  fsl_stmt_get_int32(stmt, index, &rv);
  return rv;
}
int64_t fsl_stmt_g_int64( fsl_stmt * stmt, int index ){
  int64_t rv = 0;
  fsl_stmt_get_int64(stmt, index, &rv);
  return rv;
}
double fsl_stmt_g_double( fsl_stmt * stmt, int index ){
  double rv = 0;
  fsl_stmt_get_double(stmt, index, &rv);
  return rv;
}

char const * fsl_stmt_g_text( fsl_stmt * stmt, int index,
                              fsl_size_t * outLen ){
  char const * rv = NULL;
  fsl_stmt_get_text(stmt, index, &rv, outLen);
  return rv;
}


/**
   This function outputs tracing info using fsl_fprintf((FILE*)zFILE,...).
   Defaults to stdout if zFILE is 0.
 */
static void fsl_db_sql_trace(void *zFILE, const char *zSql){
  int const n = fsl_strlen(zSql);
  static int counter = 0;
  /* FIXME: in v1 this uses fossil_trace(), but don't have
     that functionality here yet. */
  fsl_fprintf(zFILE ? (FILE*)zFILE : stdout,
              "SQL TRACE #%d: %s%s\n", ++counter,
              zSql, (n>0 && zSql[n-1]==';') ? "" : ";");
}

/*
   SQL function for debugging.
  
   The print() function writes its arguments to fsl_output()
   if the bound fsl_cx->cxConfig.sqlPrint flag is true.
*/
static void fsl_db_sql_print(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  fsl_cx * f = (fsl_cx*)sqlite3_user_data(context);
  assert(f);
  if( f->cxConfig.sqlPrint ){
    int i;
    for(i=0; i<argc; i++){
      char c = i==argc-1 ? '\n' : ' ';
      fsl_outputf(f, "%s%c", sqlite3_value_text(argv[i]), c);
    }
  }
}

/*
   SQL function to return the number of seconds since 1970.  This is
   the same as strftime('%s','now') but is more compact.
*/
static void fsl_db_now_udf(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  sqlite3_result_int64(context, (sqlite3_int64)time(0));
}

/*
   SQL function to convert a Julian Day to a Unix timestamp.
*/
static void fsl_db_j2u_udf(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  double const jd = (double)sqlite3_value_double(argv[0]);
  sqlite3_result_int64(context, (sqlite3_int64)fsl_julian_to_unix(jd));
}

/*
   SQL function FSL_CKOUT_DIR([bool includeTrailingSlash=1]) returns
   the top-level checkout directory, optionally (by default) with a
   trailing slash. Returns NULL if the fsl_cx instance bound to
   sqlite3_user_data() has no checkout.
*/
static void fsl_db_cx_chkout_dir_udf(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  fsl_cx * f = (fsl_cx*)sqlite3_user_data(context);
  int const includeSlash = argc
    ? sqlite3_value_int(argv[0])
    : 1;
  if(f && f->ckout.dir && f->ckout.dirLen){
    sqlite3_result_text(context, f->ckout.dir,
                        (int)f->ckout.dirLen
                        - (includeSlash ? 0 : 1),
                        SQLITE_TRANSIENT);
  }else{
    sqlite3_result_null(context);
  }
}


/**
    SQL Function to return the check-in time for a file.
    Requires (vid,fid) RID arguments, as described for
    fsl_mtime_of_manifest_file().
 */
static void fsl_db_checkin_mtime_udf(
                                     sqlite3_context *context,
                                     int argc,
                                     sqlite3_value **argv
){
  fsl_cx * f = (fsl_cx*)sqlite3_user_data(context);
  fsl_time_t mtime = 0;
  int rc;
  fsl_id_t vid, fid;
  assert(f);
  vid = (fsl_id_t)sqlite3_value_int(argv[0]);
  fid = (fsl_id_t)sqlite3_value_int(argv[1]);
  rc = fsl_mtime_of_manifest_file(f, vid, fid, &mtime);
  if( rc==0 ){
    sqlite3_result_int64(context, mtime);
  }else{
    sqlite3_result_error(context, "fsl_mtime_of_manifest_file() failed", -1); 
  }
}


static void fsl_db_content_udf(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  fsl_cx * f = (fsl_cx*)sqlite3_user_data(context);
  fsl_id_t rid = 0;
  char const * arg;
  int rc;
  fsl_buffer b = fsl_buffer_empty;
  assert(f);
  if(1 != argc){
    sqlite3_result_error(context, "Expecting one argument", -1);
    return;
  }
  if(SQLITE_INTEGER==sqlite3_value_type(argv[0])){
    rid = (fsl_id_t)sqlite3_value_int64(argv[0]);
    arg = NULL;
  }else{
    arg = (const char*)sqlite3_value_text(argv[0]);
    if(!arg){
      sqlite3_result_error(context, "Invalid argument", -1);
      return;
    }
    rc = fsl_sym_to_rid(f, arg, FSL_SATYPE_CHECKIN, &rid);
    if(rc) goto cx_err;
    else if(!rid){
      sqlite3_result_error(context, "No blob found", -1);
      return;
    }
  }
  rc = fsl_content_get(f, rid, &b);
  if(rc) goto cx_err;
  /* Curiously, i'm seeing no difference in allocation counts here... */
  sqlite3_result_blob(context, b.mem, (int)b.used, fsl_free);
  b = fsl_buffer_empty;
  return;
  cx_err:
  fsl_buffer_clear(&b);
  assert(f->error.msg.used);
  if(FSL_RC_OOM==rc){
    sqlite3_result_error_nomem(context);
  }else{
    assert(f->error.msg.used);
    sqlite3_result_error(context, (char const *)f->error.msg.mem,
                         (int)f->error.msg.used);
  }
}

static void fsl_db_sym2rid_udf(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  fsl_cx * f = (fsl_cx*)sqlite3_user_data(context);
  char const * arg;
  assert(f);
  if(1 != argc){
    sqlite3_result_error(context, "Expecting one argument", -1);
    return;
  }
  arg = (const char*)sqlite3_value_text(argv[0]);
  if(!arg){
    sqlite3_result_error(context, "Expecting a STRING argument", -1);
  }else{
    fsl_id_t rid = 0;
    int const rc = fsl_sym_to_rid(f, arg, FSL_SATYPE_CHECKIN, &rid);
    if(rc){
      if(FSL_RC_OOM==rc){
        sqlite3_result_error_nomem(context);
      }else{
        assert(f->error.msg.used);
        sqlite3_result_error(context, (char const *)f->error.msg.mem,
                             (int)f->error.msg.used);
      }
      fsl_cx_err_reset(f)
        /* This is arguable but keeps this error from poluting
           down-stream code (seen it happen in unit tests).  The irony
           is, it's very possible/likely that the error will propagate
           back up into f->error at some point.
        */;
    }else{
      assert(rid>0);
      sqlite3_result_int64(context, rid);
    }
  }
}

static void fsl_db_dirpart_udf(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  char const * arg;
  int rc;
  fsl_buffer b = fsl_buffer_empty;
  int fSlash = 0;
  if(argc<1 || argc>2){
    sqlite3_result_error(context,
                         "Expecting (string) or (string,bool) arguments",
                         -1);
    return;
  }
  arg = (const char*)sqlite3_value_text(argv[0]);
  if(!arg){
    sqlite3_result_error(context, "Invalid argument", -1);
    return;
  }
  if(argc>1){
    fSlash = sqlite3_value_int(argv[1]);
  }
  rc = fsl_file_dirpart(arg, -1, &b, fSlash ? 1 : 0);
  if(!rc){
    if(b.used && *b.mem){
#if 0
      sqlite3_result_text(context, (char const *)b.mem,
                          (int)b.used, SQLITE_TRANSIENT);
#else
      sqlite3_result_text(context, (char const *)b.mem,
                          (int)b.used, fsl_free);
      b = fsl_buffer_empty /* we passed ^^^^^ on ownership of b.mem */;
#endif
    }else{
      sqlite3_result_null(context);
    }
  }else{
    if(FSL_RC_OOM==rc){
      sqlite3_result_error_nomem(context);
    }else{
      sqlite3_result_error(context, "fsl_dirpart() failed!", -1);
    }
  }
  fsl_buffer_clear(&b);
}


/*
   Implement the user() SQL function.  user() takes no arguments and
   returns the user ID of the current user.
*/
static void fsl_db_user_udf(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  fsl_cx * f = (fsl_cx*)sqlite3_user_data(context);
  assert(f);
  if(f->repo.user){
    sqlite3_result_text(context, f->repo.user, -1, SQLITE_STATIC);
  }else{
    sqlite3_result_null(context);
  }
}

/**
   SQL function:

   fsl_is_enqueued(vfile.id)
   fsl_if_enqueued(vfile.id, X, Y)

   On the commit command, when filenames are specified (in order to do
   a partial commit) the vfile.id values for the named files are
   loaded into the fsl_cx state.  This function looks at that state to
   see if a file is named in that list.

   In the first form (1 argument) return TRUE if either no files are
   named (meaning that all changes are to be committed) or if id is
   found in the list.

   In the second form (3 arguments) return argument X if true and Y if
   false unless Y is NULL, in which case always return X.
*/
static void fsl_db_selected_for_checkin_udf(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  int rc = 0;
  fsl_cx * f = (fsl_cx*)sqlite3_user_data(context);
  fsl_id_bag * bag = &f->ckin.selectedIds;
  assert(argc==1 || argc==3);
  if( bag->entryCount ){
    fsl_id_t const iId = (fsl_id_t)sqlite3_value_int64(argv[0]);
    rc = iId ? (fsl_id_bag_contains(bag, iId) ? 1 : 0) : 0;
  }else{
    rc = 1;
  }
  if(1==argc){
    sqlite3_result_int(context, rc);
  }else{
    assert(3 == argc);
    assert( rc==0 || rc==1 );
    if( sqlite3_value_type(argv[2-rc])==SQLITE_NULL ) rc = 1-rc;
    sqlite3_result_value(context, argv[2-rc]);
  }
}

/**
   fsl_match_vfile_or_dir(p1,p2)

   A helper for resolving expressions like:

   WHERE pathname='X' C OR
      (pathname>'X/' C AND pathname<'X0' C)

   i.e. is 'X' a match for the LHS or is it a directory prefix of
   LHS?

   C = empty or COLLATE NOCASE, depending on the case-sensitivity
   setting of the fsl_cx instance associated with
   sqlite3_user_data(context). p1 is typically vfile.pathname or
   vfile.origname, and p2 is the string being compared against that.

   Resolves to NULL if either argument is NULL, 0 if the comparison
   shown above is false, 1 if the comparison is an exact match, or 2
   if p2 is a directory prefix part of p1.
*/
static void fsl_db_match_vfile_or_dir(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  fsl_cx * f = (fsl_cx*)sqlite3_user_data(context);
  char const * p1;
  char const * p2;
  fsl_buffer * b = 0;
  int rc = 0;
  assert(f);
  if(2 != argc){
    sqlite3_result_error(context, "Expecting two arguments", -1);
    return;
  }
  p1 = (const char*)sqlite3_value_text(argv[0]);
  p2 = (const char*)sqlite3_value_text(argv[1]);
  if(!p1 || !p2){
    sqlite3_result_null(context);
    return;
  }
  int (*cmp)(char const *, char const *) =
    f->cache.caseInsensitive ? fsl_stricmp : fsl_strcmp;
  if(0==cmp(p1, p2)){
    sqlite3_result_int(context, 1);
    return;
  }
  b = fsl_cx_scratchpad(f);
  rc = fsl_buffer_appendf(b, "%s/", p2);
  if(rc) goto oom;
  else if(cmp(p1, fsl_buffer_cstr(b))>0){
    b->mem[b->used-1] = '0';
    if(cmp(p1, fsl_buffer_cstr(b))<0)
    rc = 2;
  }
  assert(0==rc || 2==rc);
  sqlite3_result_int(context, rc);
  end:
  fsl_cx_scratchpad_yield(f, b);
  return;
  oom:
  sqlite3_result_error_nomem(context);
  goto end;
}

fsl_db * fsl_db_malloc(){
  fsl_db * rc = (fsl_db *)fsl_malloc(sizeof(fsl_db));
  if(rc){
    *rc = fsl_db_empty;
    rc->allocStamp = &fsl_db_empty;
  }
  return rc;
}

fsl_stmt * fsl_stmt_malloc(){
  fsl_stmt * rc = (fsl_stmt *)fsl_malloc(sizeof(fsl_stmt));
  if(rc){
    *rc = fsl_stmt_empty;
    rc->allocStamp = &fsl_stmt_empty;
  }
  return rc;
}


/**
    Return true if the schema is out-of-date. db must be an opened
    repo db.
 */
static char fsl_db_repo_schema_is_outofdate(fsl_db *db){
  return fsl_db_exists(db, "SELECT 1 FROM config "
                       "WHERE name='aux-schema' "
                       "AND value<>'%s'",
                       FSL_AUX_SCHEMA);
}

/*
   Returns 0 if db appears to have a current repository schema, 1 if
   it appears to have an out of date schema, and -1 if it appears to
   not be a repository.
*/
int fsl_db_repo_verify_schema(fsl_db * db){
  if(fsl_db_repo_schema_is_outofdate(db)) return 1;
  else return fsl_db_exists(db,
                            "SELECT 1 FROM config "
                            "WHERE name='project-code'")
    ? 0 : -1;
}

/**
   Callback for use with sqlite3_commit_hook(). The argument must be a
   (fsl_db*). This function returns 0 only if it surmises that
   fsl_db_transaction_end() triggered the COMMIT. On error it might
   assert() or abort() the application, so this really is just a
   sanity check for something which "must not happen."
*/
static int fsl_db_verify_begin_was_not_called(void * db_fsl){
#if 0
  return 0;
#else
  /* i cannot explain why, but the ptr i'm getting here is most definately
     not a proper fsl_db. */
  fsl_db * db = (fsl_db *)db_fsl;
  assert(db && "What else could it be?");
  assert(db->dbh && "Else we can't have been called by sqlite3, could we have?");
  if(db->beginCount>0){
    fsl_fatal(FSL_RC_MISUSE,"SQL: COMMIT was called from "
              "outside of fsl_db_transaction_end() while a "
              "fsl_db_transaction_begin()-started transaction "
              "is pending.");
    return 2;
  }
  /* we have no context: sqlite3_result_error(context, "fsl_mtime_of_manifest_file() failed", -1);  */
  else return 0;
#endif
}

int fsl_db_open( fsl_db * db, char const * dbFile,
                 int openFlags ){
  int rc;
  fsl_dbh_t * dbh = NULL;
  int isMem = 0;
  if(!db || !dbFile) return FSL_RC_MISUSE;
  else if(db->dbh) return FSL_RC_MISUSE;
  else if(!(isMem = (!*dbFile || 0==fsl_strcmp(":memory:", dbFile)))
          && !(FSL_OPEN_F_CREATE & openFlags)
          && fsl_file_access(dbFile, 0)){
    return fsl_error_set(&db->error, FSL_RC_NOT_FOUND,
                         "DB file not found: %s", dbFile);
  }
  else{
    int sOpenFlags = 0;
    if(isMem){
      sOpenFlags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    }else{
      if(FSL_OPEN_F_RO & openFlags){
        sOpenFlags |= SQLITE_OPEN_READONLY;
      }else{
        if(FSL_OPEN_F_RW & openFlags){
          sOpenFlags |= SQLITE_OPEN_READWRITE;
        }
        if(FSL_OPEN_F_CREATE & openFlags){
          sOpenFlags |= SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
        }
        if(!sOpenFlags) sOpenFlags = SQLITE_OPEN_READONLY;
      }
    }
    rc = sqlite3_open_v2( dbFile, &dbh, sOpenFlags, NULL );
    if(rc){
      if(dbh){
        /* By some complete coincidence, FSL_RC_DB==SQLITE_CANTOPEN. */
        rc = fsl_error_set(&db->error, FSL_RC_DB,
                           "Opening db file [%s] failed with "
                           "sqlite code #%d: %s",
                           dbFile, rc, sqlite3_errmsg(dbh));
      }else{
        rc = fsl_error_set(&db->error, FSL_RC_DB,
                           "Opening db file [%s] failed with "
                           "sqlite code #%d",
                           dbFile, rc);
      }
      /* MARKER(("Error msg: %s\n", (char const *)db->error.msg.mem)); */
      goto end;
    }else{
      assert(!db->filename);
      if(!*dbFile || ':'==*dbFile){
        /* assume "" or ":memory:" or some such: don't canonicalize it,
           but copy it nonetheless for consistency. */
        db->filename = fsl_strdup(dbFile);
      }else{
        fsl_buffer tmp = fsl_buffer_empty;
        rc = fsl_file_canonical_name(dbFile, &tmp, 0);
        if(!rc){
          db->filename = (char *)tmp.mem
            /* transfering ownership */;
        }else if(tmp.mem){
          fsl_buffer_clear(&tmp);
        }
      }
      if(rc){
        goto end;
      }else if(!db->filename){
        rc = FSL_RC_OOM;
        goto end;
      }
    }
    db->dbh = dbh;
    if(FSL_OPEN_F_SCHEMA_VALIDATE & openFlags){
      int check;
      check = fsl_db_repo_verify_schema(db);
      if(0 != check){
        rc = (check<0)
          ? fsl_error_set(&db->error, FSL_RC_NOT_A_REPO,
                          "DB file [%s] does not appear to be "
                          "a repository.", dbFile)
          : fsl_error_set(&db->error, FSL_RC_REPO_NEEDS_REBUILD,
                          "DB file [%s] appears to be a fossil "
                          "repsitory, but is out-of-date and needs "
                          "a rebuild.",
                          dbFile)
          ;
        assert(rc == db->error.code);
        goto end;
      }
    }

    if( (openFlags & FSL_OPEN_F_TRACE_SQL)
        || (db->f && db->f->cxConfig.traceSql) ){
      fsl_db_sqltrace_enable(db, stdout);
    }

    if(db->f){
      /*
        Plug in fsl_cx-specific functionality to this one.

        TODO: move this into the fsl_cx code. Its placement here is
        largely historical.
      */
      fsl_cx * f = db->f;
      /* This all comes from db.c:db_open()... */
      /* FIXME: check result codes here. */
      sqlite3_commit_hook(dbh, fsl_db_verify_begin_was_not_called, db);
      sqlite3_busy_timeout(dbh, 5000 /* historical value */);
      sqlite3_wal_autocheckpoint(dbh, 1);  /* Set to checkpoint frequently */
      sqlite3_exec(dbh, "PRAGMA foreign_keys=OFF;", 0, 0, 0);
      sqlite3_create_function(dbh, "now", 0, SQLITE_ANY, 0,
                              fsl_db_now_udf, 0, 0);
      sqlite3_create_function(dbh, "fsl_ci_mtime", 2,
                              SQLITE_ANY | SQLITE_DETERMINISTIC, f,
                              fsl_db_checkin_mtime_udf, 0, 0);
      sqlite3_create_function(dbh, "fsl_user", 0,
                              SQLITE_ANY | SQLITE_DETERMINISTIC, f,
                              fsl_db_user_udf, 0, 0);
      sqlite3_create_function(dbh, "fsl_print", -1,
                              SQLITE_UTF8
                              /* not strictly SQLITE_DETERMINISTIC
                                 because it produces output */,
                              f, fsl_db_sql_print,0,0);
      sqlite3_create_function(dbh, "fsl_content", 1,
                              SQLITE_ANY | SQLITE_DETERMINISTIC, f,
                              fsl_db_content_udf, 0, 0);
      sqlite3_create_function(dbh, "fsl_sym2rid", 1,
                              SQLITE_ANY | SQLITE_DETERMINISTIC, f,
                              fsl_db_sym2rid_udf, 0, 0);
      sqlite3_create_function(dbh, "fsl_dirpart", 1,
                              SQLITE_ANY | SQLITE_DETERMINISTIC, NULL,
                              fsl_db_dirpart_udf, 0, 0);
      sqlite3_create_function(dbh, "fsl_dirpart", 2,
                              SQLITE_ANY | SQLITE_DETERMINISTIC, NULL,
                              fsl_db_dirpart_udf, 0, 0);
      sqlite3_create_function(dbh, "fsl_j2u", 1,
                              SQLITE_ANY | SQLITE_DETERMINISTIC, NULL,
                              fsl_db_j2u_udf, 0, 0);
      /*
        fsl_i[sf]_selected() both require access to the f's list of
        files being considered for commit.
      */
      sqlite3_create_function(dbh, "fsl_is_enqueued", 1, SQLITE_UTF8, f,
                              fsl_db_selected_for_checkin_udf,0,0 );
      sqlite3_create_function(dbh, "fsl_if_enqueued", 3, SQLITE_UTF8, f,
                              fsl_db_selected_for_checkin_udf,0,0 );

      sqlite3_create_function(dbh, "fsl_ckout_dir", -1,
                              SQLITE_ANY /* | SQLITE_DETERMINISTIC ? */,
                              f, fsl_db_cx_chkout_dir_udf,0,0 );
      sqlite3_create_function(dbh, "fsl_match_vfile_or_dir", 2,
                              SQLITE_ANY | SQLITE_DETERMINISTIC,
                              f, fsl_db_match_vfile_or_dir,0,0 );

#if 0
      /* functions registered in v1 by db.c:db_open(). */
      /* porting cgi() requires access to the HTTP/CGI
         layer. i.e. this belongs downstream. */
      sqlite3_create_function(dbh, "cgi", 1, SQLITE_ANY, 0, db_sql_cgi, 0, 0);
      sqlite3_create_function(dbh, "cgi", 2, SQLITE_ANY, 0, db_sql_cgi, 0, 0);
      re_add_sql_func(db) /* Requires the regex bits. */;
#endif
    }/*if(db->f)*/
  }
  end:
  if(rc){
#if 1
    /* This is arguable... */
    if(db->f && db->error.code && !db->f->error.code){
      /* COPY db's error state as f's. */
      fsl_error_copy( &db->error, &db->f->error );
    }
#endif
    if(dbh){
      sqlite3_close(dbh);
      db->dbh = NULL;
    }
  }else{
    assert(db->dbh);
  }
  return rc;
}

int fsl_db_exec_multiv( fsl_db * db, const char * sql, va_list args){
  if(!db || !db->dbh || !sql) return FSL_RC_MISUSE;
  else{
    fsl_buffer buf = fsl_buffer_empty;
    int rc = 0;
    char const * z;
    char const * zEnd = NULL;
    rc = fsl_buffer_appendfv( &buf, sql, args );
    if(rc){
      fsl_buffer_clear(&buf);
      return rc;
    }
    z = fsl_buffer_cstr(&buf);
    while( (SQLITE_OK==rc) && *z ){
      fsl_stmt_t * pStmt = NULL;
      rc = sqlite3_prepare_v2(db->dbh, z, buf.used, &pStmt, &zEnd);
      if( SQLITE_OK != rc ){
        rc = fsl_err_from_db(db, rc);
        break;
      }
      if(pStmt){
        while( SQLITE_ROW == sqlite3_step(pStmt) ){}
        rc = sqlite3_finalize(pStmt);
        if(rc) rc = fsl_err_from_db(db, rc);
      }
      buf.used -= (zEnd-z);
      z = zEnd;
    }
    fsl_buffer_reserve(&buf, 0);
    return rc;
  }
}

int fsl_db_exec_multi( fsl_db * db, const char * sql, ...){
  if(!db || !db->dbh || !sql) return FSL_RC_MISUSE;
  else{
    int rc;
    va_list args;
    va_start(args,sql);
    rc = fsl_db_exec_multiv( db, sql, args );
    va_end(args);
    return rc;
  }
}

int fsl_db_execv( fsl_db * db, const char * sql, va_list args){
  if(!db || !db->dbh || !sql) return FSL_RC_MISUSE;
  else{
    fsl_stmt st = fsl_stmt_empty;
    int rc = 0;
    rc = fsl_db_preparev( db, &st, sql, args );
    if(rc) return rc;
    /* rc = fsl_stmt_step( &st ); */
    while(FSL_RC_STEP_ROW == (rc=fsl_stmt_step(&st))){}
    fsl_stmt_finalize(&st);
    return (FSL_RC_STEP_ERROR==rc)
      ? FSL_RC_DB
      : 0;
  }
}

int fsl_db_exec( fsl_db * db, const char * sql, ...){
  if(!db || !db->dbh || !sql) return FSL_RC_MISUSE;
  else{
    int rc;
    va_list args;
    va_start(args,sql);
    rc = fsl_db_execv( db, sql, args );
    va_end(args);
    return rc;
  }
}

int fsl_db_changes_recent(fsl_db * db){
  return (db && db->dbh)
    ? sqlite3_changes(db->dbh)
    : 0;
}

int fsl_db_changes_total(fsl_db * db){
  return (db && db->dbh)
    ? sqlite3_total_changes(db->dbh)
    : 0;
}

/**
    Sets db->priorChanges to sqlite3_total_changes(db->dbh).
 */
static void fsl_db_reset_change_count(fsl_db * db){
  db->priorChanges = sqlite3_total_changes(db->dbh);
}

int fsl_db_transaction_begin(fsl_db * db){
  if(!db || !db->dbh) return FSL_RC_MISUSE;
  else {
    int rc = (0==db->beginCount)
      ? fsl_db_exec(db,"BEGIN TRANSACTION")
      : 0;
    if(!rc){
      if(1 == ++db->beginCount){
        fsl_db_reset_change_count(db);
      }
    }
    return rc;
  }
}

int fsl_db_transaction_level(fsl_db * db){
  return db->doRollback ? -db->beginCount : db->beginCount;
}

int fsl_db_transaction_commit(fsl_db * db){
  return (db && db->dbh)
    ? fsl_db_transaction_end(db, 0)
    : FSL_RC_MISUSE;
}

int fsl_db_transaction_rollback(fsl_db * db){
  return (db && db->dbh)
    ? fsl_db_transaction_end(db, 1)
    : FSL_RC_MISUSE;
}

int fsl_db_rollback_force( fsl_db * db ){
  if(!db || !db->dbh) return FSL_RC_MISUSE;
  else{
    int rc;
    db->beginCount = 0;
    fsl_db_cleanup_beforeCommit(db);
    rc = fsl_db_exec(db, "ROLLBACK");
    fsl_db_reset_change_count(db);
    return rc;
  }
}

int fsl_db_transaction_end(fsl_db * db, bool doRollback){
  int rc = 0;
  if(!db || !db->dbh) return FSL_RC_MISUSE;
  else if (db->beginCount<=0){
    return fsl_error_set(&db->error, FSL_RC_RANGE,
                         "No transaction is active.");
  }
  if(doRollback) ++db->doRollback
    /* ACHTUNG: note that db->dbRollback is set before
       continuing so that if we return due to a non-0 beginCount
       that the rollback flag propagates through the
       transaction's stack.
    */
    ;
  if(--db->beginCount > 0) return 0;
  assert(0==db->beginCount && "The commit-hook check relies on this.");
  assert(db->doRollback>=0);
  if((0==db->doRollback)
     && (db->priorChanges < sqlite3_total_changes(db->dbh))){
    /* Execute before-commit hooks and leaf checks */
    fsl_size_t x = 0;
    for( ; !rc && (x < db->beforeCommit.used); ++x ){
      char const * sql = (char const *)db->beforeCommit.list[x];
      /* MARKER(("Running before-commit code: [%s]\n", sql)); */
      if(sql) rc = fsl_db_exec_multi( db, "%s", sql );
    }
    if(!rc && db->f && (FSL_DBROLE_REPO & db->role)){
      /*
         i don't like this one bit - this is low-level SCM
         functionality in an otherwise generic routine. Maybe we need
         fsl_cx_transaction_begin/end() instead.

         Much later: we have that routine now but will need to replace
         all relevant calls to fsl_db_transaction_begin()/end() with
         those routines before we can consider moving this there.
      */
      rc = fsl_repo_leaf_do_pending_checks(db->f);
      if(!rc && db->f->cache.toVerify.used){
        rc = fsl_repo_verify_at_commit(db->f);
      }else{
        fsl_repo_verify_cancel(db->f);
      }
    }
    db->doRollback = rc ? 1 : 0;
  }
  fsl_db_cleanup_beforeCommit(db);
  fsl_db_reset_change_count(db);
  rc = fsl_db_exec(db, db->doRollback ? "ROLLBACK" : "COMMIT");
  db->doRollback = 0;
  return rc;
#if 0
  /* original impl, for reference purposes during testing */
  if( g.db==0 ) return;
  if( db.nBegin<=0 ) return;
  if( rollbackFlag ) db.doRollback = 1;
  db.nBegin--;
  if( db.nBegin==0 ){
    int i;
    if( db.doRollback==0 && db.nPriorChanges<sqlite3_total_changes(g.db) ){
      while( db.nBeforeCommit ){
        db.nBeforeCommit--;
        sqlite3_exec(g.db, db.azBeforeCommit[db.nBeforeCommit], 0, 0, 0);
        sqlite3_free(db.azBeforeCommit[db.nBeforeCommit]);
      }
      leaf_do_pending_checks();
    }
    for(i=0; db.doRollback==0 && i<db.nCommitHook; i++){
      db.doRollback |= db.aHook[i].xHook();
    }
    while( db.pAllStmt ){
      db_finalize(db.pAllStmt);
    }
    db_multi_exec(db.doRollback ? "ROLLBACK" : "COMMIT");
    db.doRollback = 0;
  }
#endif  
}

int fsl_db_get_int32v( fsl_db * db, int32_t * rv,
                       char const * sql, va_list args){
  /* Potential fixme: the fsl_db_get_XXX() funcs are 95%
     code duplicates. We "could" replace these with a macro
     or supermacro, though the latter would be problematic
     in the context of an amalgamation build.
  */
  if(!db || !db->dbh || !rv || !sql || !*sql) return FSL_RC_MISUSE;
  else{
    fsl_stmt st = fsl_stmt_empty;
    int rc = 0;
    rc = fsl_db_preparev( db, &st, sql, args );
    if(rc) return rc;
    rc = fsl_stmt_step( &st );
    switch(rc){
      case FSL_RC_STEP_ROW:
        *rv = sqlite3_column_int(st.stmt, 0);
        /* Fall through */
      case FSL_RC_STEP_DONE:
        rc = 0;
        break;
      default:
        assert(FSL_RC_STEP_ERROR==rc);
        break;
    }
    fsl_stmt_finalize(&st);
    return rc;
  }
}

int fsl_db_get_int32( fsl_db * db, int32_t * rv,
                      char const * sql,
                      ... ){
  int rc;
  va_list args;
  va_start(args,sql);
  rc = fsl_db_get_int32v(db, rv, sql, args);
  va_end(args);
  return rc;
}

int fsl_db_get_int64v( fsl_db * db, int64_t * rv,
                       char const * sql, va_list args){
  if(!db || !db->dbh || !rv || !sql || !*sql) return FSL_RC_MISUSE;
  else{
    fsl_stmt st = fsl_stmt_empty;
    int rc = 0;
    rc = fsl_db_preparev( db, &st, sql, args );
    if(rc) return rc;
    rc = fsl_stmt_step( &st );
    switch(rc){
      case FSL_RC_STEP_ROW:
        *rv = sqlite3_column_int64(st.stmt, 0);
        /* Fall through */
      case FSL_RC_STEP_DONE:
        rc = 0;
        break;
      default:
        assert(FSL_RC_STEP_ERROR==rc);
        break;
    }
    fsl_stmt_finalize(&st);
    return rc;
  }
}

int fsl_db_get_int64( fsl_db * db, int64_t * rv,
                      char const * sql, ... ){
  int rc;
  va_list args;
  va_start(args,sql);
  rc = fsl_db_get_int64v(db, rv, sql, args);
  va_end(args);
  return rc;
}


int fsl_db_get_idv( fsl_db * db, fsl_id_t * rv,
                       char const * sql, va_list args){
  if(!db || !db->dbh || !rv || !sql || !*sql) return FSL_RC_MISUSE;
  else{
    fsl_stmt st = fsl_stmt_empty;
    int rc = 0;
    rc = fsl_db_preparev( db, &st, sql, args );
    if(rc) return rc;
    rc = fsl_stmt_step( &st );
    switch(rc){
      case FSL_RC_STEP_ROW:
        *rv = (fsl_id_t)sqlite3_column_int64(st.stmt, 0);
        /* Fall through */
      case FSL_RC_STEP_DONE:
        rc = 0;
        break;
      default:
        assert(FSL_RC_STEP_ERROR==rc);
        break;
    }
    fsl_stmt_finalize(&st);
    return rc;
  }
}

int fsl_db_get_id( fsl_db * db, fsl_id_t * rv,
                      char const * sql, ... ){
  int rc;
  va_list args;
  va_start(args,sql);
  rc = fsl_db_get_idv(db, rv, sql, args);
  va_end(args);
  return rc;
}


int fsl_db_get_sizev( fsl_db * db, fsl_size_t * rv,
                      char const * sql, va_list args){
  if(!db || !db->dbh || !rv || !sql || !*sql) return FSL_RC_MISUSE;
  else{
    fsl_stmt st = fsl_stmt_empty;
    int rc = 0;
    rc = fsl_db_preparev( db, &st, sql, args );
    if(rc) return rc;
    rc = fsl_stmt_step( &st );
    switch(rc){
      case FSL_RC_STEP_ROW:{
        sqlite3_int64 const i = sqlite3_column_int64(st.stmt, 0);
        if(i<0){
          rc = FSL_RC_RANGE;
          break;
        }
        *rv = (fsl_size_t)i;
        rc = 0;
        break;
      }
      case FSL_RC_STEP_DONE:
        rc = 0;
        break;
      default:
        assert(FSL_RC_STEP_ERROR==rc);
        break;
    }
    fsl_stmt_finalize(&st);
    return rc;
  }
}

int fsl_db_get_size( fsl_db * db, fsl_size_t * rv,
                      char const * sql, ... ){
  int rc;
  va_list args;
  va_start(args,sql);
  rc = fsl_db_get_sizev(db, rv, sql, args);
  va_end(args);
  return rc;
}


int fsl_db_get_doublev( fsl_db * db, double * rv,
                       char const * sql, va_list args){
  if(!db || !db->dbh || !rv || !sql || !*sql) return FSL_RC_MISUSE;
  else{
    fsl_stmt st = fsl_stmt_empty;
    int rc = 0;
    rc = fsl_db_preparev( db, &st, sql, args );
    if(rc) return rc;
    rc = fsl_stmt_step( &st );
    switch(rc){
      case FSL_RC_STEP_ROW:
        *rv = sqlite3_column_double(st.stmt, 0);
        /* Fall through */
      case FSL_RC_STEP_DONE:
        rc = 0;
        break;
      default:
        assert(FSL_RC_STEP_ERROR==rc);
        break;
    }
    fsl_stmt_finalize(&st);
    return rc;
  }
}

int fsl_db_get_double( fsl_db * db, double * rv,
                      char const * sql,
                      ... ){
  int rc;
  va_list args;
  va_start(args,sql);
  rc = fsl_db_get_doublev(db, rv, sql, args);
  va_end(args);
  return rc;
}


int fsl_db_get_textv( fsl_db * db, char ** rv,
                      fsl_size_t *rvLen,
                      char const * sql, va_list args){
  if(!db || !db->dbh || !rv || !sql || !*sql) return FSL_RC_MISUSE;
  else{
    fsl_stmt st = fsl_stmt_empty;
    int rc = 0;
    rc = fsl_db_preparev( db, &st, sql, args );
    if(rc) return rc;
    rc = fsl_stmt_step( &st );
    switch(rc){
      case FSL_RC_STEP_ROW:{
        char const * str = (char const *)sqlite3_column_text(st.stmt, 0);
        int const len = sqlite3_column_bytes(st.stmt,0);
        if(!str){
          *rv = NULL;
          if(rvLen) *rvLen = 0;
        }else{
          char * x = fsl_strndup(str, len);
          if(!x){
            rc = FSL_RC_OOM;
          }else{
            *rv = x;
            if(rvLen) *rvLen = (fsl_size_t)len;
            rc = 0;
          }
        }
        break;
      }
      case FSL_RC_STEP_DONE:
        *rv = NULL;
        if(rvLen) *rvLen = 0;
        rc = 0;
        break;
      default:
        assert(FSL_RC_STEP_ERROR==rc);
        break;
    }
    fsl_stmt_finalize(&st);
    return rc;
  }
}

int fsl_db_get_text( fsl_db * db, char ** rv,
                     fsl_size_t * rvLen,
                     char const * sql, ... ){
  int rc;
  va_list args;
  va_start(args,sql);
  rc = fsl_db_get_textv(db, rv, rvLen, sql, args);
  va_end(args);
  return rc;
}

int fsl_db_get_blobv( fsl_db * db, void ** rv,
                      fsl_size_t *rvLen,
                      char const * sql, va_list args){
  if(!db || !db->dbh || !rv || !sql || !*sql) return FSL_RC_MISUSE;
  else{
    fsl_stmt st = fsl_stmt_empty;
    int rc = 0;
    rc = fsl_db_preparev( db, &st, sql, args );
    if(rc) return rc;
    rc = fsl_stmt_step( &st );
    switch(rc){
      case FSL_RC_STEP_ROW:{
        fsl_buffer buf = fsl_buffer_empty;
        void const * str = sqlite3_column_blob(st.stmt, 0);
        int const len = sqlite3_column_bytes(st.stmt,0);
        if(!str){
          *rv = NULL;
          if(rvLen) *rvLen = 0;
        }else{
          rc = fsl_buffer_append(&buf, str, len);
          if(!rc){
            *rv = buf.mem;
            if(rvLen) *rvLen = buf.used;
          }
        }
        break;
      }
      case FSL_RC_STEP_DONE:
        *rv = NULL;
        if(rvLen) *rvLen = 0;
        rc = 0;
        break;
      default:
        assert(FSL_RC_STEP_ERROR==rc);
        break;
    }
    fsl_stmt_finalize(&st);
    return rc;
  }
}

int fsl_db_get_blob( fsl_db * db, void ** rv,
                     fsl_size_t * rvLen,
                     char const * sql, ... ){
  int rc;
  va_list args;
  va_start(args,sql);
  rc = fsl_db_get_blobv(db, rv, rvLen, sql, args);
  va_end(args);
  return rc;
}

int fsl_db_get_bufferv( fsl_db * db, fsl_buffer * b,
                        char asBlob, char const * sql,
                        va_list args){
  if(!db || !db->dbh || !b || !sql || !*sql) return FSL_RC_MISUSE;
  else{
    fsl_stmt st = fsl_stmt_empty;
    int rc = 0;
    rc = fsl_db_preparev( db, &st, sql, args );
    if(rc) return rc;
    rc = fsl_stmt_step( &st );
    switch(rc){
      case FSL_RC_STEP_ROW:{
        void const * str = asBlob
          ? sqlite3_column_blob(st.stmt, 0)
          : (void const *)sqlite3_column_text(st.stmt, 0);
        int const len = sqlite3_column_bytes(st.stmt,0);
        rc = 0;
        b->used = 0;
        rc = fsl_buffer_append( b, str, len );
        break;
      }
      case FSL_RC_STEP_DONE:
        rc = 0;
        break;
      default:
        assert(FSL_RC_STEP_ERROR==rc);
        break;
    }
    fsl_stmt_finalize(&st);
    return rc;
  }
}

int fsl_db_get_buffer( fsl_db * db, fsl_buffer * b,
                       char asBlob,
                       char const * sql, ... ){
  int rc;
  va_list args;
  va_start(args,sql);
  rc = fsl_db_get_bufferv(db, b, asBlob, sql, args);
  va_end(args);
  return rc;
}

int32_t fsl_db_g_int32( fsl_db * db, int32_t dflt,
                            char const * sql,
                            ... ){
  int32_t rv = dflt;
  va_list args;
  va_start(args,sql);
  fsl_db_get_int32v(db, &rv, sql, args);
  va_end(args);
  return rv;
}

int64_t fsl_db_g_int64( fsl_db * db, int64_t dflt,
                            char const * sql,
                            ... ){
  int64_t rv = dflt;
  va_list args;
  va_start(args,sql);
  fsl_db_get_int64v(db, &rv, sql, args);
  va_end(args);
  return rv;
}

fsl_id_t fsl_db_g_id( fsl_db * db, fsl_id_t dflt,
                            char const * sql,
                            ... ){
  fsl_id_t rv = dflt;
  va_list args;
  va_start(args,sql);
  fsl_db_get_idv(db, &rv, sql, args);
  va_end(args);
  return rv;
}

fsl_size_t fsl_db_g_size( fsl_db * db, fsl_size_t dflt,
                        char const * sql,
                        ... ){
  fsl_size_t rv = dflt;
  va_list args;
  va_start(args,sql);
  fsl_db_get_sizev(db, &rv, sql, args);
  va_end(args);
  return rv;
}

double fsl_db_g_double( fsl_db * db, double dflt,
                              char const * sql,
                              ... ){
  double rv = dflt;
  va_list args;
  va_start(args,sql);
  fsl_db_get_doublev(db, &rv, sql, args);
  va_end(args);
  return rv;
}

char * fsl_db_g_text( fsl_db * db, fsl_size_t * len,
                      char const * sql,
                      ... ){
  char * rv = NULL;
  va_list args;
  va_start(args,sql);
  fsl_db_get_textv(db, &rv, len, sql, args);
  va_end(args);
  return rv;
}

void * fsl_db_g_blob( fsl_db * db, fsl_size_t * len,
                      char const * sql,
                      ... ){
  void * rv = NULL;
  va_list args;
  va_start(args,sql);
  fsl_db_get_blob(db, &rv, len, sql, args);
  va_end(args);
  return rv;
}

double fsl_db_julian_now(fsl_db * db){
  double rc = -1.0;
  if(db && db->dbh){
    /* TODO? use cached statement? So far not used often enough to
       justify it. */
    fsl_db_get_double( db, &rc, "SELECT julianday('now')");
  }
  return rc;
}

double fsl_db_string_to_julian(fsl_db * db, char const * str){
  double rc = -1.0;
  if(db && db->dbh){
    /* TODO? use cached statement? So far not used often enough to
       justify it. */
    fsl_db_get_double( db, &rc, "SELECT julianday(%Q)",str);
  }
  return rc;
}

bool fsl_db_existsv(fsl_db * db, char const * sql, va_list args ){
  if(!db || !db->dbh || !sql) return 0;
  else if(!*sql) return 0;
  else{
    fsl_stmt st = fsl_stmt_empty;
    bool rv = false;
    if(!fsl_db_preparev(db, &st, sql, args)){
      rv = FSL_RC_STEP_ROW==fsl_stmt_step(&st) ? true : false;
    }
    fsl_stmt_finalize(&st);
    return rv;
  }

}

bool fsl_db_exists(fsl_db * db, char const * sql, ... ){
  bool rc;
  va_list args;
  va_start(args,sql);
  rc = fsl_db_existsv(db, sql, args);
  va_end(args);
  return rc;
}

/*
** Return TRUE if zTable exists.
*/
bool fsl_db_table_exists(fsl_db * db,
                        fsl_dbrole_e whichDb,
                        const char *zTable
){
  const char *zDb = fsl_db_role_label( whichDb );
  int rc = db->dbh
    ? sqlite3_table_column_metadata(db->dbh, zDb, zTable, 0,
                                    0, 0, 0, 0, 0)
    : !SQLITE_OK;
  return rc==SQLITE_OK ? true : false;
}


/*
   Returns non-0 if the database (which must be open) table identified
   by zTableName has a column named zColName (case-sensitive), else
   returns 0.
*/
char fsl_db_table_has_column( fsl_db * db, char const *zTableName, char const *zColName ){
  fsl_stmt q = fsl_stmt_empty;
  int rc = 0;
  char rv = 0;
  if(!db || !zTableName || !*zTableName || !zColName || !*zColName) return 0;
  rc = fsl_db_prepare(db, &q, "PRAGMA table_info(%Q)", zTableName );
  if(!rc) while(FSL_RC_STEP_ROW==fsl_stmt_step(&q)){
    /* Columns: (cid, name, type, notnull, dflt_value, pk) */
    fsl_size_t colLen = 0;
    char const * zCol = fsl_stmt_g_text(&q, 1, &colLen);
    if(0==fsl_strncmp(zColName, zCol, colLen)){
      rv = 1;
      break;
    }
  }
  fsl_stmt_finalize(&q);
  return rv;
}

char * fsl_db_random_hex(fsl_db * db, fsl_size_t n){
  if(!db || !n) return NULL;
  else{
    fsl_size_t rvLen = 0;
    char * rv = fsl_db_g_text(db, &rvLen,
                              "SELECT lower(hex("
                              "randomblob(%"FSL_SIZE_T_PFMT")))",
                              (fsl_size_t)(n/2+1));
    if(rv){
      assert(rvLen>=n);
      rv[n]=0;
    }
    return rv;
  }
}


int fsl_db_select_slistv( fsl_db * db, fsl_list * tgt,
                          char const * fmt, va_list args ){
  if(!db || !tgt || !fmt) return FSL_RC_MISUSE;
  else if(!*fmt) return FSL_RC_RANGE;
  else{
    int rc;
    fsl_stmt st = fsl_stmt_empty;
    fsl_size_t nlen;
    char const * n;
    char * cp;
    rc = fsl_db_preparev(db, &st, fmt, args);
    while( !rc && (FSL_RC_STEP_ROW==fsl_stmt_step(&st)) ){
      nlen = 0;
      n = fsl_stmt_g_text(&st, 0, &nlen);
      cp = n ? fsl_strndup(n, (fsl_int_t)nlen) : NULL;
      if(n && !cp) rc = FSL_RC_OOM;
      else{
        rc = fsl_list_append(tgt, cp);
        if(rc && cp) fsl_free(cp);
      }
    }
    fsl_stmt_finalize(&st);
    return rc;
  }
}

int fsl_db_select_slist( fsl_db * db, fsl_list * tgt,
                         char const * fmt, ... ){
  int rc;
  va_list va;
  va_start (va,fmt);
  rc = fsl_db_select_slistv(db, tgt, fmt, va);
  va_end(va);
  return rc;
}

void fsl_db_sqltrace_enable( fsl_db * db, FILE * outStream ){
  if(db && db->dbh){
    sqlite3_trace(db->dbh, fsl_db_sql_trace, outStream);
  }
}

int fsl_db_init( fsl_error * err,
                 char const * zFilename,
                 char const * zSchema,
                 ... ){
  fsl_db DB = fsl_db_empty;
  fsl_db * db = &DB;
  char const * zSql;
  int rc;
  char inTrans = 0;
  va_list ap;
  rc = fsl_db_open(db, zFilename, 0);
  if(rc) goto end;
  rc = fsl_db_exec(db, "BEGIN EXCLUSIVE");
  if(rc) goto end;
  inTrans = 1;
  rc = fsl_db_exec_multi(db, "%s", zSchema);
  if(rc) goto end;
  va_start(ap, zSchema);
  while( !rc && (zSql = va_arg(ap, const char*))!=NULL ){
    rc = fsl_db_exec_multi(db, "%s", zSql);
  }
  va_end(ap);
  end:
  if(rc){
    if(inTrans) fsl_db_exec(db, "ROLLBACK");
  }else{
    rc = fsl_db_exec(db, "COMMIT");
  }
  if(err){
    if(db->error.code){
      fsl_error_move(&db->error, err);
    }else if(rc){
      err->code = rc;
      err->msg.used = 0;
    }
  }
  fsl_db_close(db);
  return rc;
}

int fsl_stmt_each_f_dump( fsl_stmt * stmt, void * state ){
  int i;
  fsl_cx * f = (stmt && stmt->db) ? stmt->db->f : NULL;
  char const * sep = "\t";
  if(!f) return FSL_RC_MISUSE;
  if(1==stmt->rowCount){
    for( i = 0; i < stmt->colCount; ++i ){
      fsl_outputf(f, "%s%s", fsl_stmt_col_name(stmt, i),
            (i==stmt->colCount-1) ? "" : sep);
    }
    fsl_output(f, "\n", 1);
  }
  for( i = 0; i < stmt->colCount; ++i ){
    char const * val = fsl_stmt_g_text(stmt, i, NULL);
    fsl_outputf(f, "%s%s", val ? val : "NULL",
          (i==stmt->colCount-1) ? "" : sep);
  }
  fsl_output(f, "\n", 1);
  return 0;
}


#undef MARKER
