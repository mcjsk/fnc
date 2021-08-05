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
/*************************************************************************
  This file houses most of the context-related APIs.
*/
#include "fossil-scm/fossil-internal.h"
#include "fossil-scm/fossil-checkout.h"
#include "fossil-scm/fossil-confdb.h"
#include "fossil-scm/fossil-hash.h"
#include "fossil-ext_regexp.h"
#include "sqlite3.h"
#include <assert.h>

#if defined(_WIN32)
# include <windows.h>
# define F_OK 0
# define W_OK 2
#else
# include <unistd.h> /* F_OK */
#endif

#include <stdlib.h>
#include <string.h>

/* Only for debugging */
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)

/** Number of fsl_cx::scratchpads buffers. */
#define FSL_CX_NSCRATCH \
  ((int)(sizeof(fsl_cx_empty.scratchpads.buf) \
         /sizeof(fsl_cx_empty.scratchpads.buf[0])))
const int StaticAssert_scratchpadsCounts[
     (FSL_CX_NSCRATCH==
      ((int)(sizeof(fsl_cx_empty.scratchpads.used)
             /sizeof(fsl_cx_empty.scratchpads.used[0]))))
     ? 1 : -1
];
/**
   Used for setup and teardown of sqlite3_auto_extension().
*/
static volatile long sg_autoregctr = 0;

int fsl_cx_init( fsl_cx ** tgt, fsl_cx_init_opt const * param ){
  static fsl_cx_init_opt paramDefaults = fsl_cx_init_opt_default_m;
  int rc = 0;
  fsl_cx * f;
  extern int fsl_cx_install_timeline_crosslinkers(fsl_cx *f);
  if(!tgt) return FSL_RC_MISUSE;
  else if(!param){
    if(!paramDefaults.output.state.state){
      paramDefaults.output.state.state = stdout;
    }
    param = &paramDefaults;
  }
  if(*tgt){
    void const * allocStamp = (*tgt)->allocStamp;
    fsl_cx_reset(*tgt, 1) /* just to be safe */;
    f = *tgt;
    *f = fsl_cx_empty;
    f->allocStamp = allocStamp;
  }else{
    f = fsl_cx_malloc();
    if(!f) return FSL_RC_OOM;

    *tgt = f;
  }
  f->output = param->output;
  f->cxConfig = param->config;

  enum {
    /* Because testing shows a lot of re-allocs via some of the
       lower-level stat()-related bits, we pre-allocate this many
       bytes into f->scratchpads.buf[].  Curiously, there is almost no
       difference in (re)allocation behaviour until this size goes
       above about 200.

       We ignore allocation errors here, as they're not critical (but
       upcoming ops will fail when _they_ run out of memory).
    */
    InitialScratchCapacity = 256
  };
  assert(FSL_CX_NSCRATCH
         == (sizeof(f->scratchpads.used)/sizeof(f->scratchpads.used[0])));
  for(int i = 0; i < FSL_CX_NSCRATCH; ++i){
    f->scratchpads.buf[i] = fsl_buffer_empty;
    f->scratchpads.used[i] = false;
    fsl_buffer_reserve(&f->scratchpads.buf[i], InitialScratchCapacity);
  }
  /* We update f->error.msg often, so go ahead and pre-allocate that, too,
     also ignoring any OOM error at this point. */
  fsl_buffer_reserve(&f->error.msg, InitialScratchCapacity);

#if defined(SQLITE_THREADSAFE) && SQLITE_THREADSAFE>0
  sqlite3_initialize(); /*the SQLITE_MUTEX_STATIC_MASTER will not cause autoinit of sqlite for some reason*/
  sqlite3_mutex_enter(sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_MASTER));
#endif
    if ( 1 == ++sg_autoregctr )
    {
      /*register our statically linked extensions to be auto-init'ed at the appropriate time*/
      sqlite3_auto_extension((void(*)(void))(sqlite3_regexp_init));     /*sqlite regexp extension*/
      atexit(sqlite3_reset_auto_extension)
        /* Clean up pseudo-leak valgrind complains about:
           https://www.sqlite.org/c3ref/auto_extension.html */;
    }
#if defined(SQLITE_THREADSAFE) && SQLITE_THREADSAFE>0
  sqlite3_mutex_leave(sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_MASTER));
#endif
  f->dbMem.f = f /* so that dbMem gets fsl_xxx() SQL funcs installed. */;
  rc = fsl_db_open( &f->dbMem, "", 0 );
  if(!rc){
    /* Attempt to ensure that the TEMP tables/indexes use FILE storage. */
    rc = fsl_db_exec(&f->dbMem, "PRAGMA temp_store=FILE;");
  }
  if(!rc){
    rc = fsl_cx_install_timeline_crosslinkers(f);
  }
  if(rc){
    if(f->dbMem.error.code){
      fsl_cx_uplift_db_error(f, &f->dbMem);
    }
  }else{
    f->dbMain = &f->dbMem;
    f->dbMem.role = FSL_DBROLE_MAIN;
  }
  return rc;
}

static void fsl_cx_mcache_clear(fsl_cx *f){
  const unsigned cacheLen =
    (unsigned)(sizeof(fsl_mcache_empty.aAge)
               /sizeof(fsl_mcache_empty.aAge[0]));
  for(unsigned i = 0; i < cacheLen; ++i){
    fsl_deck_finalize(&f->cache.mcache.decks[i]);
  }
  f->cache.mcache = fsl_mcache_empty;
}

void fsl_cx_reset(fsl_cx * f, bool closeDatabases){
  fsl_checkin_discard(f);
#define SFREE(X) fsl_free(X); X = NULL
  if(closeDatabases){
    fsl_cx_close_dbs(f);
    /*
      Reminder: f->dbMem is NOT closed here: it's an internal detail,
      not public state. We could arguably close and reopen it here,
      but then we introduce a potenital error case (OOM) where we
      currently have none (thus the void return).
    */
    SFREE(f->ckout.dir);
    f->ckout.dirLen = 0;
    /* assert(NULL==f->dbMain); */
    assert(!f->repo.db.dbh);
    assert(!f->ckout.db.dbh);
    assert(!f->config.db.dbh);
    assert(!f->repo.db.filename);
    assert(!f->ckout.db.filename);
    assert(!f->config.db.filename);
  }
  SFREE(f->repo.user);
  SFREE(f->ckout.uuid);
  SFREE(f->cache.projectCode);
#undef SFREE
  fsl_error_clear(&f->error);
  fsl_card_J_list_free(&f->ticket.customFields, 1);
  fsl_buffer_clear(&f->fileContent);
  for(int i = 0; i < FSL_CX_NSCRATCH; ++i){
    fsl_buffer_clear(&f->scratchpads.buf[i]);
    f->scratchpads.used[i] = false;
  }
  fsl_acache_clear(&f->cache.arty);
  fsl_id_bag_clear(&f->cache.leafCheck);
  fsl_id_bag_clear(&f->cache.toVerify);
  fsl_cx_clear_mf_seen(f);
#define SLIST(L) fsl_list_visit_free(L, 1)
#define GLOBL(X) SLIST(&f->cache.globs.X)
  GLOBL(ignore);
  GLOBL(binary);
  GLOBL(crnl);
#undef GLOBL
#undef SLIST
  fsl_cx_mcache_clear(f);
  f->cache = fsl_cx_empty.cache;
}

void fsl_cx_clear_mf_seen(fsl_cx * f){
  fsl_id_bag_clear(&f->cache.mfSeen);
}

void fsl_cx_finalize( fsl_cx * f ){
  void const * allocStamp = f ? f->allocStamp : NULL;
  if(!f) return;

  if(f->xlinkers.list){
#if 0
    /* Potential TODO: add client-specified finalizer for xlink
       callback state, using a fsl_state to replace the current
       (void*) for x->state. Seems like overkill for the time being.
     */
    fsl_size_t i;
    for( i = 0; i < f->xlinkers.used; ++i ){
      fsl_xlinker * x = f->xlinkers.list + i;
      if(x->state.finalize.f){
        x->state.finalize.f(x->state.finalize.state, x->state.state);
      }
    }
#endif    
    fsl_free(f->xlinkers.list);
    f->xlinkers = fsl_xlinker_list_empty;
  }

  if(f->clientState.finalize.f){
    f->clientState.finalize.f( f->clientState.finalize.state,
                               f->clientState.state );
  }
  f->clientState = fsl_state_empty;
  if(f->output.state.finalize.f){
    f->output.state.finalize.f( f->output.state.finalize.state,
                                f->output.state.state );
  }
  f->output = fsl_outputer_empty;
  fsl_cx_reset(f, 1);
  fsl_db_close(&f->dbMem);
  *f = fsl_cx_empty;
  if(&fsl_cx_empty == allocStamp){
    fsl_free(f);
  }else{
    f->allocStamp = allocStamp;
  }

  /* clean up the auto extension; not strictly necessary, but pleases debug malloc's */
#if defined(SQLITE_THREADSAFE) && SQLITE_THREADSAFE>0
  sqlite3_mutex_enter(sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_MASTER));
#endif
  if ( 0 == --sg_autoregctr )
  {
    /*register our statically linked extensions to be auto-init'ed at the appropriate time*/
    sqlite3_cancel_auto_extension((void(*)(void))(sqlite3_regexp_init));     /*sqlite regexp extension*/
  }
  assert(sg_autoregctr>=0);
#if defined(SQLITE_THREADSAFE) && SQLITE_THREADSAFE>0
  sqlite3_mutex_leave(sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_MASTER));
#endif
}

void fsl_cx_err_reset(fsl_cx * f){
  if(f){
    fsl_error_reset(&f->error);
    fsl_db_err_reset(&f->dbMem);
    fsl_db_err_reset(&f->repo.db);
    fsl_db_err_reset(&f->config.db);
    fsl_db_err_reset(&f->ckout.db);
  }
}

int fsl_cx_err_set_e( fsl_cx * f, fsl_error * err ){
  if(!f) return FSL_RC_MISUSE;
  else if(!err){
    return fsl_cx_err_set(f, 0, NULL);
  }else{
    fsl_error_move(err, &f->error);
    fsl_error_clear(err);
    return f->error.code;
  }
}

int fsl_cx_err_setv( fsl_cx * f, int code, char const * fmt,
                     va_list args ){
  return f
    ? fsl_error_setv( &f->error, code, fmt, args )
    : FSL_RC_MISUSE;
}

int fsl_cx_err_set( fsl_cx * f, int code, char const * fmt,
                    ... ){
  if(!f) return FSL_RC_MISUSE;
  else{
    int rc;
    va_list args;
    va_start(args,fmt);
    rc = fsl_error_setv( &f->error, code, fmt, args );
    va_end(args);
    return rc;
  }
}

int fsl_cx_err_get( fsl_cx * f, char const ** str, fsl_size_t * len ){
  return f
    ? fsl_error_get( &f->error, str, len )
    : FSL_RC_MISUSE;
}

fsl_id_t fsl_cx_last_insert_id(fsl_cx * f){
  return (f && f->dbMain && f->dbMain->dbh)
    ? fsl_db_last_insert_id(f->dbMain)
    : -1;
}

fsl_cx * fsl_cx_malloc(){
  fsl_cx * rc = (fsl_cx *)fsl_malloc(sizeof(fsl_cx));
  if(rc) {
    *rc = fsl_cx_empty;
    rc->allocStamp = &fsl_cx_empty;
  }
  return rc;
}

int fsl_cx_err_report( fsl_cx * f, char addNewline ){
  if(!f) return FSL_RC_MISUSE;
  else if(f->error.code){
    char const * msg = f->error.msg.used
      ? (char const *)f->error.msg.mem
      : fsl_rc_cstr(f->error.code)
      ;
    return fsl_outputf(f, "Error #%d: %s%s",
                       f->error.code, msg,
                       addNewline ? "\n" : "");
  }
  else return 0;
}

int fsl_cx_uplift_db_error( fsl_cx * f, fsl_db * db ){
  assert(f);
  if(!f) return FSL_RC_MISUSE;
  if(!db){
    db = f->dbMain;
    assert(db && "misuse: no DB handle to uplift error from!");
    if(!db) return FSL_RC_MISUSE;
  }
  fsl_error_move( &db->error, &f->error );
  return f->error.code;
}

int fsl_cx_uplift_db_error2(fsl_cx *f, fsl_db * db, int rc){
  if(!db) db = f->dbMain;
  assert(db);
  if(rc && FSL_RC_OOM!=rc && !f->error.code && db->error.code){
    rc = fsl_cx_uplift_db_error(f, db);
  }
  return rc;
}

fsl_db * fsl_cx_db_config( fsl_cx * f ){
  if(!f) return NULL;
  else if(f->config.db.dbh) return &f->config.db;
  else if(f->dbMain && (FSL_DBROLE_CONFIG & f->dbMain->role)) return f->dbMain;
  else return NULL;
}

fsl_db * fsl_cx_db_repo( fsl_cx * f ){
  if(!f) return NULL;
  else if(f->repo.db.dbh) return &f->repo.db;
  else if(f->dbMain && (FSL_DBROLE_REPO & f->dbMain->role)) return f->dbMain;
  else return NULL;
}

fsl_db * fsl_needs_repo(fsl_cx * f){
  fsl_db * const db = fsl_cx_db_repo(f);
  if(!db){
    fsl_cx_err_set(f, FSL_RC_NOT_A_REPO,
                   "Fossil context has no opened repository db.");
  }
  return db;
}

fsl_db * fsl_needs_ckout(fsl_cx * f){
  fsl_db * const db = fsl_cx_db_ckout(f);
  if(!db){
    fsl_cx_err_set(f, FSL_RC_NOT_A_CKOUT,
                   "Fossil context has no opened checkout db.");
  }
  return db;
}

fsl_db * fsl_cx_db_ckout( fsl_cx * f ){
  if(!f) return NULL;
  else if(f->ckout.db.dbh) return &f->ckout.db;
  else if(f->dbMain && (FSL_DBROLE_CKOUT & f->dbMain->role)) return f->dbMain;
  else return NULL;
}

fsl_db * fsl_cx_db( fsl_cx * f ){
  return f ? f->dbMain : NULL;
}
/** @internal

    Returns one of f->db{Config,Repo,Ckout,Mem}
    or NULL.

    ACHTUNG and REMINDER TO SELF: the current (2021-03) design means
    that none of these handles except for FSL_DBROLE_MAIN actually has
    an sqlite3 db handle assigned to it. This returns a handle to the
    "level of abstraction" we need to keep track of each db's name and
    db-specific other state.

    e.g. passing a role of FSL_DBROLE_CKOUT this does NOT return
    the same thing as fsl_cx_db_ckout().
*/
fsl_db * fsl_cx_db_for_role(fsl_cx * f, fsl_dbrole_e r){
  switch(r){
    case FSL_DBROLE_CONFIG:
      return &f->config.db;
    case FSL_DBROLE_REPO:
      return &f->repo.db;
    case FSL_DBROLE_CKOUT:
      return &f->ckout.db;
    case FSL_DBROLE_MAIN:
      return &f->dbMem;
    case FSL_DBROLE_NONE:
    default:
      return NULL;
  }
}

/**
    Detaches the given db role from f->dbMain and removes the role
    from f->dbMain->role.
*/
static int fsl_cx_detach_role(fsl_cx * f, fsl_dbrole_e r){
  if(!f || !f->dbMain) return FSL_RC_MISUSE;
  else if(!(r & f->dbMain->role)){
    assert(!"Misuse: cannot detach unattached role.");
    return FSL_RC_NOT_FOUND;
  }
  else{
    fsl_db * db = fsl_cx_db_for_role(f,r);
    int rc;
    if(!db) return FSL_RC_RANGE;
    assert(f->dbMain != db);
    f->dbMain->role &= ~r;
    rc = fsl_db_detach( f->dbMain, fsl_db_role_label(r) );
    //MARKER(("rc=%s %s %s\n", fsl_rc_cstr(rc), fsl_db_role_label(r),
    //        fsl_buffer_cstr(&f->dbMain->error.msg)));
    fsl_free(db->filename);
    fsl_free(db->name);
    db->filename = NULL;
    db->name = NULL;
    return rc;
  }
}


/** @internal

    Attaches the given db file to f with the given role. This function "should"
    be static but we need it in fsl_repo.c when creating a new repository.
*/
int fsl_cx_attach_role(fsl_cx * f, const char *zDbName, fsl_dbrole_e r){
  char const * label = fsl_db_role_label(r);
  fsl_db * db = fsl_cx_db_for_role(f, r);
  char ** nameDest = NULL;
  int rc;
  if(!f->dbMain){
    assert(!"Misuse: f->dbMain has not been set: cannot attach role.");
    return FSL_RC_MISUSE;
  }
  else if(r & f->dbMain->role){
    assert(!"Misuse: role is already attached.");
    return fsl_cx_err_set(f, FSL_RC_MISUSE,
                          "Db role %s is already attached.",
                          label);                          
  }
#if 0
  MARKER(("r=%s db=%p, ckout.db=%p\n", label,
          (void*)db, (void*)&f->ckout.db));
  MARKER(("r=%s db=%p, repo.db=%p\n", label,
          (void*)db, (void*)&f->repo.db));
  MARKER(("r=%s db=%p, dbMain=%p\n", label,
          (void*)db, (void*)f->dbMain));
#endif
  assert(db);
  assert(label);
  assert(f->dbMain != db);
  assert(!db->filename);
  assert(!db->name);
  nameDest = &db->filename;
  switch(r){
    case FSL_DBROLE_CONFIG:
    case FSL_DBROLE_REPO:
    case FSL_DBROLE_CKOUT:
      break;
    case FSL_DBROLE_MAIN:
    case FSL_DBROLE_NONE:
    default:
      assert(!"cannot happen/not legal");
      return FSL_RC_RANGE;
  }
  *nameDest = fsl_strdup(zDbName);
  db->name = *nameDest ? fsl_strdup(label) : NULL;
  if(!db->name){
    rc = FSL_RC_OOM;
    /* Design note: we do the strdup() before the ATTACH because if
       the attach succeeds and strdup fails, detaching the db will
       almost certainly fail because it must allocate for its prepared
       statement and other internals. We would end up having to leave
       the db attached and returning a failure, which could lead to a
       memory leak (or worse) downstream.
    */
  }else{
    /*MARKER(("Attached %p role %d %s %s\n",
      (void const *)db, r, db->name, db->filename));*/
    rc = fsl_db_attach(f->dbMain, zDbName, label);
    if(rc){
      fsl_cx_uplift_db_error(f, f->dbMain);
    }else{
      //MARKER(("Attached db %p %s from %s\n",
      //  (void*)db, label, db->filename));
      f->dbMain->role |= r;
    }
  }
  return rc;
}


/*
  2020-03-04: this function has, since the switch to using ATTACH for
  all major dbs, largely been supplanted by fsl_cx_attach_role(), but
  should we ever return to a multi-db-handle world, this
  implementation (or something close to it) is what we'll want.

  Analog to fossil's db_open_or_attach().
  
  This function is still very much up for reconsideration. i'm not
  terribly happy with the "db roles" here - i'd prefer to have them
  each in their own struct, but understand that the roles may play a
  part in query generation, so i haven't yet ruled them out. On
  20140724 we got a fix which allows us to publish concrete names for
  all dbs, regardless of which one is the "main", so much of the
  reason for that concern has been alleviated. In late October, 2014,
  Dave figured out that multi-attach leads to locking problems, so
  the innards are being rewritten to use a :memory: db as the
  permanent "main", and attaching the others.

  Opens or attaches the given db file. zDbName is the
  file name of the DB. role is the role of that db
  in the framework (that determines its db name in SQL).
  
  The first time this is called, f->dbMain is set to point
  to fsl_cx_db_for_role(f, role).
  
  If f->dbMain is already set then the db is attached using a
  role-dependent name and role is added to f->dbMain->role's bitmask.
  
  If f->dbMain is not open then it is opened here and its role is set
  to the given role. It is _also_ ATTACHED using its role name. This
  allows us to publish the names of all Fossil-managed db handles
  regardless of which one is "main". e.g. if repo is opened first, it
  is addressable from SQL as both "main" or "repo".
  
  The given db zDbName is the name of a database file.  If f->dbMain
  is not opened then that db is assigned to the given db file,
  otherwise attach attach zDbName using the name zLabel.
  
  If pWasAttached is not NULL then it is set to a true value if the
  db gets attached, else a false value. It is only modified on success.
  
  It is an error to open the same db role more than once, and trying
  to do so results in a FSL_RC_ACCESS error (f's error state is
  updated).
*/
static int fsl_cx_db_main_open_or_attach( fsl_cx * f,
                                          const char *zDbName,
                                          /* const char *zLabel, */
                                          fsl_dbrole_e role,
                                          int *pWasAttached){
  int rc;
  int wasAttached = 0;
  fsl_db * db;
  assert(f);
  assert(zDbName && *zDbName);
  assert((FSL_DBROLE_CONFIG==role)
         || (FSL_DBROLE_CKOUT==role)
         || (FSL_DBROLE_REPO==role));
  if(!f || !zDbName) return FSL_RC_MISUSE;
  else if((FSL_DBROLE_NONE==role) || !*zDbName) return FSL_RC_RANGE;
  switch(role){
    case FSL_DBROLE_REPO:
    case FSL_DBROLE_CKOUT:
    case FSL_DBROLE_CONFIG:
      db = fsl_cx_db_for_role(f, role);
      break;
    default:
      assert(!"not possible");
      db = NULL;
      /* We'll fall through and segfault in a moment... */
  }
  assert(db);
  try_again:
  if(!f->dbMain) {
    assert(!"Not possible since moving to a :memory: main db.");
    /* This is the first db. It is now our main db. */
    assert( FSL_DBROLE_NONE==db->role );
    assert( NULL==db->dbh );
    assert( role == FSL_DBROLE_CONFIG
            || role == FSL_DBROLE_REPO 
            || role == FSL_DBROLE_CKOUT );
    f->dbMain = db;
    db->f = f;
    rc = fsl_db_open( db, zDbName, FSL_OPEN_F_RW );
    if(!rc){
      db->role = role;
      assert(!db->name);
#if 0
      db->name = fsl_strdup( fsl_db_role_label(FSL_DBROLE_MAIN) );
      if(!db->name) rc = FSL_RC_OOM;
#else
      /* Many thanks to Simon Slavin, native resident of the sqlite3
         mailing list, for this... we apply the db's "real" name via
         an ATTACH, meaning that all the client-side kludgery to get the
         proper DB name is now obsolete.
      */
      rc = fsl_db_attach( db, zDbName, fsl_db_role_label(role) );
      if(!rc){
        db->name = fsl_strdup( fsl_db_role_label(role) );
        if(!db->name) rc = FSL_RC_OOM;
      }
#endif
    }
    /* MARKER(("db->role=%d\n",db->role)); */
    /* g.db = fsl_db_open(zDbName); */
    /* g.zMainDbType = zLabel; */
    /* f->mainDbType = zLabel; */
    /* f->dbMain.role = FSL_DBROLE_MAIN; */
  }else{
    /* Main db has already been opened. Attach this one to it... */
    assert( (int)FSL_DBROLE_NONE!=f->dbMain->role );
    assert( f == f->dbMain->f );
    if(NULL == f->dbMain->dbh){
      /* It was closed in the meantime. Re-assign dbMain... */
      MARKER(("Untested: re-assigning f->dbMain after it has been closed.\n"));
      assert(!"Broken by addition of f->dbMem.");
      f->dbMain = NULL;
      goto try_again;
    }
    if((int)role == db->role){
      return fsl_cx_err_set(f, FSL_RC_ACCESS,
                            "Cannot open/attach db role "
                            "#%d (%s) more than once.",
                            (int)role, fsl_db_role_label(role));
    }
    rc = fsl_cx_attach_role(f, zDbName, role);
    wasAttached = 1;
  }
  if(!rc){
    if(pWasAttached) *pWasAttached = wasAttached;
  }else{
    if(db->error.code){
      fsl_cx_uplift_db_error(f, db);
    }
    if(db==f->dbMain) f->dbMain = NULL;
    fsl_db_close(db);
  }
  return rc;
}

int fsl_config_close( fsl_cx * f ){
  if(!f) return FSL_RC_MISUSE;
  else{
    int rc;
    fsl_db * db = &f->config.db;
    if(db->dbh){
      /* Config is our main db. Close them all */
      assert(!"Not possible since f->dbMem added.");
      assert(f->dbMain==db);
      fsl_ckout_close(f);
      fsl_repo_close(f);
      rc = fsl_db_close(db);
      f->dbMain = NULL;
    }else if(f->dbMain && (FSL_DBROLE_CONFIG & f->dbMain->role)){
      /* Config db is ATTACHed. */
      assert(f->dbMain!=db);
      rc = fsl_cx_detach_role(f, FSL_DBROLE_CONFIG);
    }
    else rc = FSL_RC_NOT_FOUND;
    assert(!db->dbh);
    fsl_db_clear_strings(db, 1);
    return rc;
  }
}

int fsl_repo_close( fsl_cx * f ){
  if(fsl_cx_transaction_level(f)){
    return fsl_cx_err_set(f, FSL_RC_MISUSE,
                          "Cannot close repo with opened transaction.");
  }else{
    int rc;
    fsl_db * db = &f->repo.db;
    if(db->dbh){
      /* Repo is our main db. Close them all */
      assert(!"Not possible since f->dbMem added.");
      assert(f->dbMain==db);
      fsl_config_close(f);
      fsl_ckout_close(f);
      rc = fsl_db_close(db);
      f->dbMain = NULL;
    }else if(f->dbMain && (FSL_DBROLE_REPO & f->dbMain->role)){
      /* Repo db is ATTACHed. */
      if(FSL_DBROLE_CKOUT & f->dbMain->role){
        rc = fsl_cx_err_set(f, FSL_RC_MISUSE,
                            "Cannot close repo while checkout is "
                            "opened.");
      }else{
        assert(f->dbMain!=db);
        rc = fsl_cx_detach_role(f, FSL_DBROLE_REPO);
      }
    }
    else rc = FSL_RC_NOT_FOUND;
    assert(!db->dbh);
    fsl_db_clear_strings(db, true);
    return rc;
  }
}

int fsl_ckout_close( fsl_cx * f ){
  if(fsl_cx_transaction_level(f)){
    return fsl_cx_err_set(f, FSL_RC_MISUSE,
                          "Cannot close checkout with opened transaction.");
  }else{
    int rc;
    fsl_db * db = &f->ckout.db;
#if 0
    fsl_repo_close(f)
        /*
          Ignore error - it might not be opened.  Close it first
          because it was (if things went normally) attached to
          f->ckout.db resp. HOWEVER: we have a bug-in-waiting here,
          potentially. If repo becomes the main db for an attached
          checkout (which isn't current possibly because opening a
          checkout closes/(re)opens the repo) then we stomp on our db
          handle here. Hypothetically.

          If the repo is dbMain:

          a) we cannot have a checkout because fsl_repo_open()
          fails if a repo is already opened.

          b) fsl_repo_close() will clear f->dbMain,
          leaving the code below to return FSL_RC_NOT_FOUND,
          which would be misleading.

          But that's all hypothetical - best we make it impossible
          for the public API to have a checkout without a repo
          or a repo/checkout mismatch.
        */
        ;
#endif
    if(db->dbh){
      /* checkout is our main db. Close them all. */
      assert(!"Not possible since f->dbMem added.");
      assert(!f->repo.db.dbh);
      assert(f->dbMain==db);
      fsl_repo_close(f);
      fsl_config_close(f);
      rc = fsl_db_close(db);
      f->dbMain = NULL;
    }
    else if(f->dbMain && (FSL_DBROLE_CKOUT & f->dbMain->role)){
      /* Checkout db is ATTACHed. */
      assert(f->dbMain!=db);
      rc = fsl_cx_detach_role(f, FSL_DBROLE_CKOUT);
      fsl_repo_close(f) /*
                          Because the repo is implicitly opened, we
                          "should" implicitly close it. This is
                          debatable but "probably almost always"
                          desired. i can't currently envisage a
                          reasonable use-case which requires closing
                          the checkout but keeping the repo opened.
                          The repo can always be re-opened by itself.
                        */;
    }
    else{
      rc = FSL_RC_NOT_FOUND;
    }
    fsl_free(f->ckout.uuid);
    f->ckout.uuid = NULL;
    f->ckout.rid = 0;
    assert(!db->dbh);
    fsl_db_clear_strings(db, true);
    return rc;
  }
}

/**
   If zDbName is a valid checkout database file, open it and return 0.
   If it is not a valid local database file, return a non-0 code.
*/
static int fsl_cx_ckout_open_db(fsl_cx * f, const char *zDbName){
  /* char *zVFileDef; */
  int rc;
  fsl_int_t const lsize = fsl_file_size(zDbName);
  if( -1 == lsize  ){
    return FSL_RC_NOT_FOUND /* might be FSL_RC_ACCESS? */;
  }
  if( lsize%1024!=0 || lsize<4096 ){
    return fsl_cx_err_set(f, FSL_RC_RANGE,
                          "File's size is not correct for a "
                          "checkout db: %s",
                          zDbName);
  }
  rc = fsl_cx_db_main_open_or_attach(f, zDbName,
                                     FSL_DBROLE_CKOUT,
                                     NULL);
  return rc;
#if 0
  /* Historical bits: no longer needed(?). */

  zVFileDef = fsl_db_text(0, "SELECT sql FROM %s.sqlite_master"
                          " WHERE name=='vfile'",
                          fsl_cx_db_name("localdb")); /* ==> "ckout" */
  if( zVFileDef==0 ) return 0;

  /* If the "isexe" column is missing from the vfile table, then
     add it now.   This code added on 2010-03-06.  After all users have
     upgraded, this code can be safely deleted.
  */
  if( !fsl_str_glob("* isexe *", zVFileDef) ){
    fsl_db_multi_exec("ALTER TABLE vfile ADD COLUMN isexe BOOLEAN DEFAULT 0");
  }

  /* If "islink"/"isLink" columns are missing from tables, then
     add them now.   This code added on 2011-01-17 and 2011-08-27.
     After all users have upgraded, this code can be safely deleted.
  */
  if( !fsl_str_glob("* islink *", zVFileDef) ){
    db_multi_exec("ALTER TABLE vfile ADD COLUMN islink BOOLEAN DEFAULT 0");
    if( db_local_table_exists_but_lacks_column("stashfile", "isLink") ){
      db_multi_exec("ALTER TABLE stashfile ADD COLUMN isLink BOOL DEFAULT 0");
    }
    if( db_local_table_exists_but_lacks_column("undo", "isLink") ){
      db_multi_exec("ALTER TABLE undo ADD COLUMN isLink BOOLEAN DEFAULT 0");
    }
    if( db_local_table_exists_but_lacks_column("undo_vfile", "islink") ){
      db_multi_exec("ALTER TABLE undo_vfile ADD COLUMN islink BOOL DEFAULT 0");
    }
  }
#endif
  return 0;
}


int fsl_cx_prepare( fsl_cx *f, fsl_stmt * tgt, char const * sql,
                      ... ){
  if(!f || !f->dbMain) return FSL_RC_MISUSE;
  else{
    int rc;
    va_list args;
    va_start(args,sql);
    rc = fsl_db_preparev( f->dbMain, tgt, sql, args );
    va_end(args);
    return rc;
  }
}

int fsl_cx_preparev( fsl_cx *f, fsl_stmt * tgt, char const * sql,
                       va_list args ){
  return (f && f->dbMain && tgt)
    ? fsl_db_preparev(f->dbMain, tgt, sql, args)
    : FSL_RC_MISUSE;
}

/**
    Passes the fsl_schema_config() SQL code through a new/truncated
    file named dbName. If the file exists before this call, it is
    unlink()ed and fails if that operation fails.

    FIXME: this was broken by the addition of "cfg." prefix on the
    schema's tables.
 */
static int fsl_config_file_reset(fsl_cx * f, char const * dbName){
  fsl_db DB = fsl_db_empty;
  fsl_db * db = &DB;
  int rc = 0;
  bool isAttached = false;
  const char * zPrefix = fsl_db_role_label(FSL_DBROLE_CONFIG);
  if(-1 != fsl_file_size(dbName)){
    rc = fsl_file_unlink(dbName);
    if(rc){
      return fsl_cx_err_set(f, rc,
                            "Error %s while removing old config file (%s)",
                            fsl_rc_cstr(rc), dbName);
    }
  }
  /**
     Hoop-jumping: because the schema file has a cfg. prefix for the
     table(s), and we cannot assign an arbitrary name to an open()'d
     db, we first open the db (making the the "main" db), then
     ATTACH it to itself to provide the fsl_db_role_label() alias.
  */
  rc = fsl_db_open(db, dbName, FSL_OPEN_F_RWC);
  if(rc) goto end;
  rc = fsl_db_attach(db, dbName, zPrefix);
  if(rc) goto end;
  isAttached = true;
  rc = fsl_db_exec_multi(db, "%s", fsl_schema_config());
  end:
  rc = fsl_cx_uplift_db_error2(f, db, rc);
  if(isAttached) fsl_db_detach(db, zPrefix);
  fsl_db_close(db);
  return rc;
}

int fsl_config_global_preferred_name(char ** zOut){
  char * zEnv = 0;
  char * zRc = 0;
  int rc = 0;
  fsl_buffer buf = fsl_buffer_empty;

#if FSL_PLATFORM_IS_WINDOWS
#  error "TODO: port in fossil(1) db.c:db_configdb_name() Windows bits"
#else
  
#endif

  /* Option 1: $FOSSIL_HOME/.fossil */
  zEnv = fsl_getenv("FOSSIL_HOME");
  if(zEnv){
    zRc = fsl_mprintf("%s/.fossil", zEnv);
    if(!zRc) rc = FSL_RC_OOM;
    goto end;
  }
  /* Option 2: if $HOME/.fossil exists, use that */  
  rc = fsl_find_home_dir(&buf, 0);
  if(rc) goto end;
  rc = fsl_buffer_append(&buf, "/.fossil", 8);
  if(rc) goto end;
  if(fsl_file_size(fsl_buffer_cstr(&buf))>1024*3){
    zRc = fsl_buffer_take(&buf);
    goto end;
  }
  /* Option 3: $XDG_CONFIG_HOME/fossil.db */
  fsl_filename_free(zEnv);
  zEnv = fsl_getenv("XDG_CONFIG_HOME");
  if(zEnv){
    zRc = fsl_mprintf("%s/fossil.db", zEnv);
    if(!zRc) rc = FSL_RC_OOM;
    goto end;
  }
  /* Option 4: If $HOME/.config is a directory,
     use $HOME/.config/fossil.db */
  buf.used -= 8 /* "/.fossil" */;
  buf.mem[buf.used] = 0;
  rc = fsl_buffer_append(&buf, "/.config", 8);
  if(rc) goto end;
  if(fsl_dir_check(fsl_buffer_cstr(&buf))>0){
    zRc = fsl_mprintf("%b/fossil.db", &buf);
    if(!zRc) rc = FSL_RC_OOM;
    goto end;
  }
  /* Option 5: fall back to $HOME/.fossil */
  buf.used -= 8 /* "/.config" */;
  buf.mem[buf.used] = 0;
  rc = fsl_buffer_append(&buf, "/.fossil", 8);
  if(!rc) zRc = fsl_buffer_take(&buf);

  end:
  if(zEnv) fsl_filename_free(zEnv);
  if(!rc){
    assert(zRc);
    *zOut = zRc;
  }
  fsl_buffer_clear(&buf);
  return rc;
}

int fsl_config_open( fsl_cx * f, char const * openDbName ){
  int rc = 0;
  const char * zDbName = 0;
  char * zPrefName = 0;
  bool useAttach = false /* TODO? Move this into a parameter?
                            Do we need the fossil behaviour? */;
  if(!f) return FSL_RC_MISUSE;
  else if(f->config.db.dbh){
    fsl_config_close(f);
  }
  if(openDbName && *openDbName){
    zDbName = openDbName;
  }else{
    rc = fsl_config_global_preferred_name(&zPrefName);
    if(rc) goto end;
    zDbName = zPrefName;
  }
  {
    fsl_int_t const fsize = fsl_file_size(zDbName);
    if( -1==fsize || (fsize<1024*3) ){
      rc = fsl_config_file_reset(f, zDbName);
      if(rc) goto end;
    }
  }
#if defined(_WIN32) || defined(__CYGWIN__)
  /* TODO: Jan made some changes in this area in fossil(1) in
     January(?) 2014, such that only the config file needs to be
     writable, not the directory. Port that in.
  */
  if( fsl_file_access(zDbName, W_OK) ){
    rc = fsl_cx_err_set(f, FSL_RC_ACCESS,
                        "Configuration database [%s] "
                        "must be writeable.", zDbName);
    goto end;
  }
#endif

  if( useAttach ){
    rc = fsl_cx_attach_role(f, zDbName, FSL_DBROLE_CONFIG);
  }else{
    rc = fsl_cx_db_main_open_or_attach(f, zDbName,
                                       FSL_DBROLE_CONFIG,
                                       NULL);
    /* rc = fsl_db_open(&f->config.db, zDbName, FSL_OPEN_F_RWC ); */
  }
  if(!rc && !f->dbMain){
    assert(!"Not possible(?) since addition of f->dbMem");
    f->dbMain = &f->config.db;
  }
  end:
  fsl_free(zPrefName);
  return rc;
}

static void fsl_cx_username_from_repo(fsl_cx * f){
  fsl_db * dbR = fsl_cx_db_repo(f);
  char * u;
  assert(dbR);
  u = fsl_db_g_text(fsl_cx_db_repo(f), NULL,
                    "SELECT login FROM user WHERE uid=1");
  if(u){
    fsl_free(f->repo.user);
    f->repo.user = u;
  }
}

static int fsl_cx_load_glob_lists(fsl_cx * f){
  int rc;
  rc = fsl_config_globs_load(f, &f->cache.globs.ignore, "ignore-glob");
  if(!rc) rc = fsl_config_globs_load(f, &f->cache.globs.binary, "binary-glob");
  if(!rc) rc = fsl_config_globs_load(f, &f->cache.globs.crnl, "crnl-glob");
  return rc;
}

/**
   To be called after a repo or checkout/repo combination has been
   opened. This updates some internal cached info based on the
   checkout and/or repo.
*/
static int fsl_cx_after_open(fsl_cx * f){
  int rc = fsl_ckout_version_fetch(f);
  if(!rc) rc = fsl_cx_load_glob_lists(f);
  return rc;
}


static void fsl_cx_fetch_hash_policy(fsl_cx * f){
  int const iPol =
    fsl_config_get_int32( f, FSL_CONFDB_REPO,
                          FSL_HPOLICY_AUTO, "hash-policy");
  fsl_hashpolicy_e p;
  switch(iPol){
    case FSL_HPOLICY_SHA3: p = FSL_HPOLICY_SHA3; break;
    case FSL_HPOLICY_SHA3_ONLY: p = FSL_HPOLICY_SHA3_ONLY; break;
    case FSL_HPOLICY_SHA1: p = FSL_HPOLICY_SHA1; break;
    case FSL_HPOLICY_SHUN_SHA1: p = FSL_HPOLICY_SHUN_SHA1; break;
    default: p = FSL_HPOLICY_AUTO; break;
  }
  f->cxConfig.hashPolicy = p;
}

int fsl_repo_open( fsl_cx * f, char const * repoDbFile/* , bool readOnlyCurrentlyIgnored */ ){
  if(!f || !repoDbFile || !*repoDbFile) return FSL_RC_MISUSE;
  else if(fsl_cx_db_repo(f)){
    return fsl_cx_err_set(f, FSL_RC_ACCESS,
                          "Context already has an opened repository.");
  }
  else {
    int rc;
    if(0!=fsl_file_access( repoDbFile, F_OK )){
      rc = fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                          "Repository db [%s] not found or cannot be read.",
                          repoDbFile);
    }else{
      rc = fsl_cx_db_main_open_or_attach(f, repoDbFile,
                                         FSL_DBROLE_REPO,
                                         NULL);
      if(!rc && !(FSL_CX_F_IS_OPENING_CKOUT & f->flags)){
        rc = fsl_cx_after_open(f);
      }
      if(!rc){
        fsl_db * const db = fsl_cx_db_repo(f);
        fsl_cx_username_from_repo(f);
        f->cache.allowSymlinks =
          fsl_config_get_bool(f, FSL_CONFDB_REPO,
                              false,
                              "allow-symlinks");
        f->cache.seenDeltaManifest =
          fsl_config_get_int32(f, FSL_CONFDB_REPO, -1,
                               "seen-delta-manifest");
        fsl_cx_fetch_hash_policy(f);
        if(f->cxConfig.hashPolicy==FSL_HPOLICY_AUTO){
          if(fsl_db_exists(db, "SELECT 1 FROM blob WHERE length(uuid)>40")
             || !fsl_db_exists(db, "SELECT 1 FROM blob WHERE length(uuid)==40")){
            f->cxConfig.hashPolicy = FSL_HPOLICY_SHA3;
          }
        }
      }
    }
    return rc;
  }
}

/**
    Tries to open the repository from which the current checkout
    derives. Returns 0 on success.
*/
static int fsl_repo_open_for_ckout(fsl_cx * f){
  char * repoDb = NULL;
  int rc;
  fsl_buffer nameBuf = fsl_buffer_empty;
  fsl_db * db = fsl_cx_db_ckout(f);
  assert(f);
  assert(f->ckout.dir);
  assert(db);
  rc = fsl_db_get_text(db, &repoDb, NULL,
                       "SELECT value FROM vvar "
                       "WHERE name='repository'");
  if(rc) fsl_cx_uplift_db_error( f, db );
  else if(repoDb){
    if(!fsl_is_absolute_path(repoDb)){
      /* Make it relative to the checkout db dir */
      rc = fsl_buffer_appendf(&nameBuf, "%s/%s", f->ckout.dir, repoDb);
      fsl_free(repoDb);
      if(rc) {
        fsl_buffer_clear(&nameBuf);
        return rc;
      }
      repoDb = (char*)nameBuf.mem /* transfer ownership */;
      nameBuf = fsl_buffer_empty;
    }
    rc = fsl_file_canonical_name(repoDb, &nameBuf, 0);
    fsl_free(repoDb);
    if(!rc){
      repoDb = fsl_buffer_str(&nameBuf);
      assert(repoDb);
      rc = fsl_repo_open(f, repoDb);
    }
    fsl_buffer_reserve(&nameBuf, 0);
  }else{
    /* This can only happen if we are not using a proper
       checkout db or someone has removed the repo link.
    */
    rc = fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                        "Could not determine this checkout's "
                        "repository db file.");
  }
  return rc;
}

static void fsl_ckout_mtime_set(fsl_cx * const f){
  f->ckout.mtime = f->ckout.rid>0
    ? fsl_db_g_double(fsl_cx_db_repo(f), 0.0,
                      "SELECT mtime FROM event "
                      "WHERE objid=%" FSL_ID_T_PFMT,
                      f->ckout.rid)
    : 0.0;
}

int fsl_ckout_version_fetch( fsl_cx *f ){
  fsl_id_t rid = 0;
  int rc = 0;
  fsl_db * dbC = fsl_cx_db_ckout(f);
  fsl_db * dbR = dbC ? fsl_needs_repo(f) : NULL;
  assert(!dbC || (dbC && dbR));
  fsl_free(f->ckout.uuid);
  f->ckout.rid = -1;
  f->ckout.uuid = NULL;
  f->ckout.mtime = 0.0;
  if(!dbC){
    return 0;
  }
  fsl_cx_err_reset(f);
  rid = fsl_config_get_id(f, FSL_CONFDB_CKOUT, -1, "checkout");
  //MARKER(("rc=%s rid=%d\n",fsl_rc_cstr(f->error.code), (int)rid));
  if(rid>0){
    f->ckout.uuid = fsl_rid_to_uuid(f, rid);
    if(!f->ckout.uuid){
      assert(f->error.code);
      if(!f->error.code){
        rc = fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                            "Could not load UUID for RID %"FSL_ID_T_PFMT,
                            (fsl_id_t)rid);
      }
    }else{
      assert(fsl_is_uuid(f->ckout.uuid));
    }
    f->ckout.rid = rid;
    fsl_ckout_mtime_set(f);
  }else if(rid==0){
    /* This is a legal case not possible before libfossil (and only
       afterwards possible in fossil(1)) - an empty repo without an
       active checkin. [Much later:] that capability has since been
       removed from fossil.
    */
    f->ckout.rid = 0;
  }else{
    rc = fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                        "Cannot determine checkout version.");
  }
  return rc;
}

/** @internal

    Sets f->ckout.rid to the given rid (which must be 0 or a valid
    RID) and f->ckout.uuid to a copy of the given uuid. If uuid is
    NULL and rid is not 0 then the uuid is fetched using
    fsl_rid_to_uuid(), else if uuid is not NULL then it is assumed to
    be the UUID for the given RID and is copies to f->ckout.uuid.

    Returns 0 on success, FSL_RC_OOM if copying uuid fails, or some
    error from fsl_rid_to_uuid() if that fails.

    Does not write the changes to disk. Use fsl_ckout_version_write()
    for that. That routine also calls this one, so there's no need to
    call both.
*/
static int fsl_cx_ckout_version_set(fsl_cx *f, fsl_id_t rid,
                                    fsl_uuid_cstr uuid){
  char * u = 0;
  assert(rid>=0);
  u = uuid
    ? fsl_strdup(uuid)
    : (rid ? fsl_rid_to_uuid(f, rid) : 0);
  if(rid && !u) return FSL_RC_OOM;
  f->ckout.rid = rid;
  fsl_free(f->ckout.uuid);
  f->ckout.uuid = u;
  fsl_ckout_mtime_set(f);
  return 0;
}

int fsl_ckout_version_write( fsl_cx *f, fsl_id_t vid,
                             fsl_uuid_cstr hash ){
  int rc = 0;
  if(!fsl_needs_ckout(f)) return FSL_RC_NOT_A_CKOUT;
  else if(vid<0){
    return fsl_cx_err_set(f, FSL_RC_MISUSE,
                          "Invalid vid for fsl_ckout_version_write()");
  }
  if(f->ckout.rid!=vid){
    rc = fsl_cx_ckout_version_set(f, vid, hash);
  }
  if(!rc){
    rc = fsl_config_set_id(f, FSL_CONFDB_CKOUT,
                           "checkout", f->ckout.rid);
    if(!rc){
      rc = fsl_config_set_text(f, FSL_CONFDB_CKOUT,
                               "checkout-hash", f->ckout.uuid);
    }
  }
  if(!rc){
    char * zFingerprint = 0;
    rc = fsl_repo_fingerprint_search(f, 0, &zFingerprint);
    if(!rc){
      rc = fsl_config_set_text(f, FSL_CONFDB_CKOUT,
                               "fingerprint", zFingerprint);
      fsl_free(zFingerprint);
    }
  }
  if(!rc){
    int const mode = vid ? -1 : 0;
    rc = fsl_ckout_manifest_write(f, mode, mode, mode, 0);
  }
  return rc;
}

void fsl_ckout_version_info(fsl_cx *f, fsl_id_t * rid, fsl_uuid_cstr * uuid ){
  if(uuid) *uuid = f->ckout.uuid;
  if(rid) *rid = f->ckout.rid>=0 ? f->ckout.rid : 0;
}

int fsl_ckout_db_search( char const * dirName, bool checkParentDirs,
                         fsl_buffer * pOut ){
  int rc;
  fsl_int_t dLen = 0, i;
  enum { DbCount = 2 };
  const char aDbName[DbCount][10] = { "_FOSSIL_", ".fslckout" };
  fsl_buffer Buf = fsl_buffer_empty;
  fsl_buffer * buf = &Buf;
  buf->used = 0;
  if(dirName){
    dLen = fsl_strlen(dirName);
    if(0==dLen) return FSL_RC_RANGE;
    rc = fsl_buffer_reserve( buf, (fsl_size_t)(dLen + 10) );
    if(!rc) rc = fsl_buffer_append( buf, dirName, dLen );
    if(rc){
      fsl_buffer_clear(buf);
      return rc;
    }
  }else{
    char zPwd[4000];
    fsl_size_t pwdLen = 0;
    rc = fsl_getcwd( zPwd, sizeof(zPwd)/sizeof(zPwd[0]), &pwdLen );
    if(rc){
      fsl_buffer_clear(buf);
#if 0
      return fsl_cx_err_set(f, rc,
                            "Could not determine current directory. "
                            "Error code %d (%s).",
                            rc, fsl_rc_cstr(rc));
#else
      return rc;
#endif
    }
    if(1 == pwdLen && '/'==*zPwd) *zPwd = '.'
      /* When in the root directory (or chroot) then change dir name
         name to something we can use.
      */;
    rc = fsl_buffer_append(buf, zPwd, pwdLen);
    if(rc){
      fsl_buffer_clear(buf);
      return rc;
    }
    dLen = (fsl_int_t)pwdLen;
  }
  if(rc){
    fsl_buffer_clear(buf);
    return rc;
  }
  assert(buf->capacity>=buf->used);
  assert((buf->used == (fsl_size_t)dLen) || (1==buf->used && (int)'.'==(int)buf->mem[0]));
  assert(0==buf->mem[buf->used]);

  while(dLen>0){
    /*
      Loop over the list in aDbName, appending each one to
      the dir name in the search for something we can use.
    */
    fsl_int_t lenMarker = dLen /* position to re-set to on each
                                  sub-iteration. */ ;
    /* trim trailing slashes on this part, so that we don't end up
       with multiples between the dir and file in the final output. */
    while( dLen && ((int)'/'==(int)buf->mem[dLen-1])) --dLen;
    for( i = 0; i < DbCount; ++i ){
      char const * zName;
      buf->used = (fsl_size_t)lenMarker;
      dLen = lenMarker;
      rc = fsl_buffer_appendf( buf, "/%s", aDbName[i]);
      if(rc){
        fsl_buffer_clear(buf);
        return rc;
      }
      zName = fsl_buffer_cstr(buf);
      if(0==fsl_file_access(zName, 0)){
        if(pOut) rc = fsl_buffer_append( pOut, buf->mem, buf->used );
        fsl_buffer_clear(buf);
        return rc;
      }
      if(!checkParentDirs){
        dLen = 0;
        break;
      }else{
        /* Traverse up one dir and try again. */
        --dLen;
        while( dLen>0 && (int)buf->mem[dLen]!=(int)'/' ){ --dLen; }
        while( dLen>0 && (int)buf->mem[dLen-1]==(int)'/' ){ --dLen; }
        if(dLen>lenMarker){
          buf->mem[dLen] = 0;
        }
      }
    }
  }
  fsl_buffer_clear(buf);
  return FSL_RC_NOT_FOUND;
}

int fsl_cx_getcwd(fsl_cx * f, fsl_buffer * pOut){
  char cwd[FILENAME_MAX] = {0};
  fsl_size_t cwdLen = 0;
  int rc = fsl_getcwd(cwd, (fsl_size_t)sizeof(cwd), &cwdLen);
  if(rc){
    return fsl_cx_err_set(f, rc,
                          "Could not get current working directory!");
  }
  rc = fsl_buffer_append(pOut, cwd, cwdLen);
  return rc
    ? fsl_cx_err_set(f, rc/*must be an OOM*/, NULL)
    : 0;
}

int fsl_ckout_open_dir( fsl_cx * f, char const * dirName,
                        bool checkParentDirs ){
  int rc;
  fsl_buffer Buf = fsl_buffer_empty;
  fsl_buffer * buf = &Buf;
  char const * zName;
  if(fsl_cx_db_ckout(f)){
    return fsl_cx_err_set( f, FSL_RC_ACCESS,
                           "A checkout is already opened. "
                           "Close it before opening another.");
  }
  rc = fsl_ckout_db_search(dirName, checkParentDirs, buf);
  if(rc){
    if(FSL_RC_NOT_FOUND==rc){
      rc = fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                          "Could not find checkout under [%s].",
                          dirName ? dirName : ".");
    }
    fsl_buffer_clear(buf);
    return rc;
  }
  assert(buf->used>1 /* "/<FILENAME>" */);
  zName = fsl_buffer_cstr(buf);
  rc = fsl_cx_ckout_open_db(f, zName);
  if(rc){
    fsl_buffer_clear(buf);
    return rc;
  }else{
    /* Checkout db is now opened. Fiddle some internal
       bits...
    */
    unsigned char * end = buf->mem+buf->used-1;
    /* Find dir part */
    while(end>buf->mem && (unsigned char)'/'!=*end) --end;
    assert('/' == (char)*end && "fsl_ckout_db_search() appends '/<DBNAME>'");
    fsl_free(f->ckout.dir);
    f->ckout.dirLen = end - buf->mem +1 /* for trailing '/' */ ;
    *(end+1) = 0; /* Rather than strdup'ing, we'll just lop off the
                     filename part. Keep the '/' for historical
                     conventions purposes - it simplifies path
                     manipulation later on. */
    f->ckout.dir = fsl_buffer_take(buf);
    assert(!f->ckout.dir[f->ckout.dirLen]);
    assert('/' == f->ckout.dir[f->ckout.dirLen-1]);
    f->flags |= FSL_CX_F_IS_OPENING_CKOUT;
    rc = fsl_repo_open_for_ckout(f);
    f->flags &= ~FSL_CX_F_IS_OPENING_CKOUT;
    if(!rc) rc = fsl_cx_after_open(f);
    if(rc){
      /* Is this sane? Is not doing it sane? */
      fsl_ckout_close(f);
    }
    return rc;
  }
}


char const * fsl_cx_db_file_for_role(fsl_cx const * f,
                                     fsl_dbrole_e r,
                                     fsl_size_t * len){
  fsl_db const * db = fsl_cx_db_for_role((fsl_cx*)f, r);
  char const * rc = db ? db->filename : NULL;
  if(len) *len = fsl_strlen(rc);
  return rc;
}

char const * fsl_cx_db_name_for_role(fsl_cx const * f,
                                     fsl_dbrole_e r,
                                     fsl_size_t * len){
  if(FSL_DBROLE_MAIN == r){
    /* special case to be removed when f->dbMem bits are
       finished. */
    if(len) *len=4;
    return "main";
  }else{
    fsl_db const * db = fsl_cx_db_for_role((fsl_cx*)f, r);
    char const * rc = db ? db->name : NULL;
    if(len) *len = rc ? fsl_strlen(rc) : 0;
    return rc;
  }
}

char const * fsl_cx_db_file_config(fsl_cx const * f,
                                   fsl_size_t * len){
  char const * rc = NULL;
  if(f && f->config.db.filename){
    rc = f->config.db.filename;
    if(len) *len = fsl_strlen(rc);
  }
  return rc;
}

char const * fsl_cx_db_file_repo(fsl_cx const * f,
                                 fsl_size_t * len){
  char const * rc = NULL;
  if(f && f->repo.db.filename){
    rc = f->repo.db.filename;
    if(len) *len = fsl_strlen(rc);
  }
  return rc;
}

char const * fsl_cx_db_file_ckout(fsl_cx const * f,
                                     fsl_size_t * len){
  char const * rc = NULL;
  if(f && f->ckout.db.filename){
    rc = f->ckout.db.filename;
    if(len) *len = fsl_strlen(rc);
  }
  return rc;
}

char const * fsl_cx_ckout_dir_name(fsl_cx const * f,
                                      fsl_size_t * len){
  char const * rc = NULL;
  if(f && f->ckout.dir){
    rc = f->ckout.dir;
    if(len) *len = f->ckout.dirLen;
  }
  return rc;
}

int fsl_cx_flags_get( fsl_cx * f ){
  return f->flags;
}

int fsl_cx_flag_set( fsl_cx * f, int flags, bool enable ){
  if(enable) f->flags |= flags;
  else f->flags &= ~flags;
  return f->flags;
}


fsl_xlinker * fsl_xlinker_by_name( fsl_cx * f, char const * name ){

  fsl_xlinker * rv = NULL;
  fsl_size_t i;
  for( i = 0; i < f->xlinkers.used; ++i ){
    rv = f->xlinkers.list + i;
    if(0==fsl_strcmp(rv->name, name)) return rv;
  }
  return NULL;
}

int fsl_xlink_listener( fsl_cx * f, char const * name,
                        fsl_deck_xlink_f cb, void * cbState ){
  fsl_xlinker * x;
  if(!f || !cb || !name || !*name) return FSL_RC_MISUSE;
  x = fsl_xlinker_by_name(f, name);
  if(x){
    /* Replace existing entry */
    x->f = cb;
    x->state = cbState;
    return 0;
  }else if(f->xlinkers.used <= f->xlinkers.capacity){
    /* Expand the array */
    fsl_size_t const n = f->xlinkers.used ? f->xlinkers.used * 2 : 5;
    fsl_xlinker * re =
      (fsl_xlinker *)fsl_realloc(f->xlinkers.list,
                                 n * sizeof(fsl_xlinker));
    if(!re) return FSL_RC_OOM;
    f->xlinkers.list = re;
    f->xlinkers.capacity = n;
  }
  x = f->xlinkers.list + f->xlinkers.used++;
  *x = fsl_xlinker_empty;
  x->f = cb;
  x->state = cbState;
  x->name = name;
  return 0;
}

int fsl_cx_user_set( fsl_cx * f, char const * userName ){
  if(!f) return FSL_RC_MISUSE;
  else if(!userName || !*userName){
    fsl_free(f->repo.user);
    f->repo.user = NULL;
    return 0;
  }else{
    char * u = fsl_strdup(userName);
    if(!u) return FSL_RC_OOM;
    else{
      fsl_free(f->repo.user);
      f->repo.user = u;
      return 0;
    }    
  }
}

char const * fsl_cx_user_get( fsl_cx const * f ){
  return f ? f->repo.user : NULL;
}

int fsl_cx_schema_ticket(fsl_cx * f, fsl_buffer * pOut){
  fsl_db * db = f ? fsl_needs_repo(f) : NULL;
  if(!f || !pOut) return FSL_RC_MISUSE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  else{
    fsl_size_t const oldUsed = pOut->used;
    int rc = fsl_config_get_buffer(f, FSL_CONFDB_REPO,
                                   "ticket-table", pOut);
    if((FSL_RC_NOT_FOUND==rc)
       || (oldUsed == pOut->used/*found but it was empty*/)
       ){
      rc = fsl_buffer_append(pOut, fsl_schema_ticket(), -1);
    }
    return rc;
  }
}


int fsl_cx_stat2( fsl_cx * f, bool relativeToCwd,
                  char const * zName, fsl_fstat * tgt,
                  fsl_buffer * nameOut, bool fullPath){
  int rc;
  fsl_buffer * b = 0;
  fsl_buffer * bufRel = 0;
  fsl_size_t n;
  assert(f);
  if(!zName || !*zName) return FSL_RC_MISUSE;
  else if(!fsl_needs_ckout(f)) return FSL_RC_NOT_A_CKOUT;
  b = fsl_cx_scratchpad(f);
  bufRel = fsl_cx_scratchpad(f);
#if 1
  rc = fsl_ckout_filename_check(f, relativeToCwd, zName, bufRel);
  if(rc) goto end;
  zName = fsl_buffer_cstr2( bufRel, &n );
#else
  if(!fsl_is_simple_pathname(zName, 1)){
    rc = fsl_ckout_filename_check(f, relativeToCwd, zName, bufRel);
    if(rc) goto end;
    zName = fsl_buffer_cstr2( bufRel, &n );
    /* MARKER(("bufRel=%s\n",zName)); */
  }else{
    n = fsl_strlen(zName);
  }
#endif
  assert(n>0 &&
         "Will fail if fsl_ckout_filename_check() changes "
         "to return nothing if zName==checkout root");
  if(!n
     /* i don't like the "." resp "./" result when zName==checkout root */
     || (1==n && '.'==bufRel->mem[0])
     || (2==n && '.'==bufRel->mem[0] && '/'==bufRel->mem[1])){
    rc = fsl_buffer_appendf(b, "%s%s", f->ckout.dir,
                            (2==n) ? "/" : "");
  }else{
    rc = fsl_buffer_appendf(b, "%s%s", f->ckout.dir, zName);
  }
  if(!rc){
    rc = fsl_stat( fsl_buffer_cstr(b), tgt, false );
    if(rc){
      fsl_cx_err_set(f, rc, "Error %s from fsl_stat(\"%b\")",
                     fsl_rc_cstr(rc), b);
    }else if(nameOut){
      rc = fullPath
        ? fsl_buffer_append(nameOut, b->mem, b->used)
        : fsl_buffer_append(nameOut, zName, n);
    }
  }
  end:
  fsl_cx_scratchpad_yield(f, b);
  fsl_cx_scratchpad_yield(f, bufRel);
  return rc;
}

int fsl_cx_stat(fsl_cx * f, bool relativeToCwd, char const * zName,
                fsl_fstat * tgt){
  return fsl_cx_stat2(f, relativeToCwd, zName, tgt, NULL, 0);
}


void fsl_cx_case_sensitive_set(fsl_cx * f, bool caseSensitive){
  f->cache.caseInsensitive = caseSensitive;
}

bool fsl_cx_is_case_sensitive(fsl_cx const * f){
  return !f->cache.caseInsensitive;
}

char const * fsl_cx_filename_collation(fsl_cx const * f){
  return f->cache.caseInsensitive ? "COLLATE nocase" : "";
}


void fsl_cx_content_buffer_yield(fsl_cx * f){
  enum { MaxSize = 1024 * 1024 * 2 };
  assert(f);
  if(f->fileContent.capacity>MaxSize){
    fsl_buffer_resize(&f->fileContent, MaxSize);
    assert(f->fileContent.capacity<=MaxSize+1);
  }
  fsl_buffer_reuse(&f->fileContent);
}

fsl_error const * fsl_cx_err_get_e(fsl_cx const * f){
  return f ? &f->error : NULL;
}

int fsl_cx_close_dbs( fsl_cx * f ){
  if(fsl_cx_transaction_level(f)){
    return fsl_cx_err_set(f, FSL_RC_MISUSE,
                          "Cannot close the databases when a "
                          "transaction is pending.");
  }
  int rc = 0, rc1;
  rc1 = fsl_ckout_close(f);
  if(rc1) rc = rc1;
  rc1 = fsl_repo_close(f);
  if(rc1) rc = rc1;
  rc1 = fsl_config_close(f);
  if(rc1) rc = rc1;
  assert(!f->repo.db.dbh);
  assert(!f->ckout.db.dbh);
  assert(!f->config.db.dbh);
  return rc;
}

char const * fsl_cx_glob_matches( fsl_cx * f, int gtype,
                                  char const * str ){
  int i, count = 0;
  char const * rv = NULL;
  fsl_list const * lists[] = {0,0,0};
  if(!f || !str || !*str) return NULL;
  if(gtype & FSL_GLOBS_IGNORE) lists[count++] = &f->cache.globs.ignore;
  if(gtype & FSL_GLOBS_CRNL) lists[count++] = &f->cache.globs.crnl;
  /*CRNL/BINARY together makes little sense, but why strictly prohibit
    it?*/
  if(gtype & FSL_GLOBS_BINARY) lists[count++] = &f->cache.globs.binary;
  for( i = 0; i < count; ++i ){
    if( (rv = fsl_glob_list_matches( lists[i], str )) ) break;
  }
  return rv;
}

int fsl_output_f_fsl_cx(void * state, void const * src, fsl_size_t n ){
  return (state && src && n)
    ? fsl_output((fsl_cx*)state, src, n)
    : (n ? FSL_RC_MISUSE : 0);
}

int fsl_cx_hash_buffer( fsl_cx const * f, bool useAlternate,
                        fsl_buffer const * pIn, fsl_buffer * pOut){
  /* fossil(1) counterpart: hname_hash() */
  if(useAlternate){
    switch(f->cxConfig.hashPolicy){
      case FSL_HPOLICY_AUTO:
      case FSL_HPOLICY_SHA1:
        return fsl_sha3sum_buffer(pIn, pOut);
      case FSL_HPOLICY_SHA3:
        return fsl_sha1sum_buffer(pIn, pOut);
      default: return FSL_RC_UNSUPPORTED;
    }
  }else{
    switch(f->cxConfig.hashPolicy){
      case FSL_HPOLICY_SHA1:
      case FSL_HPOLICY_AUTO:
        return fsl_sha1sum_buffer(pIn, pOut);
      case FSL_HPOLICY_SHA3:
      case FSL_HPOLICY_SHA3_ONLY:
      case FSL_HPOLICY_SHUN_SHA1:
        return fsl_sha3sum_buffer(pIn, pOut);
    }
  }
  assert(!"not reached");
  return FSL_RC_RANGE;
}

int fsl_cx_hash_filename( fsl_cx * f, bool useAlternate,
                          const char * zFilename, fsl_buffer * pOut){
  /* FIXME: reimplement this to stream the content in bite-sized
     chunks. That requires duplicating most of fsl_buffer_fill_from()
     and fsl_cx_hash_buffer(). */
  fsl_buffer * content = &f->fileContent;
  int rc;
  assert(!content->used && "Internal recursive misuse of fsl_cx::fileContent");
  fsl_buffer_reuse(content);
  rc = fsl_buffer_fill_from_filename(content, zFilename);
  if(!rc){
    rc = fsl_cx_hash_buffer(f, useAlternate, content, pOut);
  }
  fsl_buffer_reuse(content);
  return rc;
}

char const * fsl_hash_policy_name(fsl_hashpolicy_e p){
  switch(p){
    case FSL_HPOLICY_SHUN_SHA1: return "shun-sha1";
    case FSL_HPOLICY_SHA3: return "sha3";
    case FSL_HPOLICY_SHA3_ONLY: return "sha3-only";
    case FSL_HPOLICY_SHA1: return "sha1";
    case FSL_HPOLICY_AUTO: return "auto";
    default: return NULL;
  }
}

fsl_hashpolicy_e fsl_cx_hash_policy_set(fsl_cx *f, fsl_hashpolicy_e p){
  fsl_hashpolicy_e const old = f->cxConfig.hashPolicy;
  fsl_db * const dbR = fsl_cx_db_repo(f);
  if(dbR){
    /* Write it regardless of whether it's the same as the old policy
       so that we're sure the db knows the policy. */
    if(FSL_HPOLICY_AUTO==p &&
       fsl_db_exists(dbR,"SELECT 1 FROM blob WHERE length(uuid)>40")){
      p = FSL_HPOLICY_SHA3;
    }
    fsl_config_set_int32(f, FSL_CONFDB_REPO, "hash-policy", p);
  }
  f->cxConfig.hashPolicy = p;
  return old;
}

fsl_hashpolicy_e fsl_cx_hash_policy_get(fsl_cx const*f){
  return f->cxConfig.hashPolicy;
}

int fsl_cx_transaction_level(fsl_cx * f){
  return f->dbMain
    ? fsl_db_transaction_level(f->dbMain)
    : 0;
}

int fsl_cx_transaction_begin(fsl_cx * f){
  return fsl_db_transaction_begin(f->dbMain);
}

int fsl_cx_transaction_end(fsl_cx * f, bool doRollback){
  return fsl_db_transaction_end(f->dbMain, doRollback);
}

void fsl_cx_confirmer(fsl_cx * f,
                      fsl_confirmer const * newConfirmer,
                      fsl_confirmer * prevConfirmer){
  if(prevConfirmer) *prevConfirmer = f->confirmer;
  f->confirmer = newConfirmer ? *newConfirmer : fsl_confirmer_empty;
}

void fsl_cx_confirmer_get(fsl_cx const * f, fsl_confirmer * dest){
  *dest = f->confirmer;
}

int fsl_cx_confirm(fsl_cx *f, fsl_confirm_detail const * detail,
                   fsl_confirm_response *outAnswer){
  if(f->confirmer.callback){
    return f->confirmer.callback(detail, outAnswer,
                                 f->confirmer.callbackState);
  }
  /* Default answers... */
  switch(detail->eventId){
    case FSL_CEVENT_OVERWRITE_MOD_FILE:
    case FSL_CEVENT_OVERWRITE_UNMGD_FILE:
      outAnswer->response =  FSL_CRESPONSE_NEVER;
      break;
    case FSL_CEVENT_RM_MOD_UNMGD_FILE:
      outAnswer->response = FSL_CRESPONSE_NEVER;
      break;
    case FSL_CEVENT_MULTIPLE_VERSIONS:
      outAnswer->response = FSL_CRESPONSE_CANCEL;
      break;
    default:
      assert(!"Unhandled fsl_confirm_event_e value");
      fsl_fatal(FSL_RC_UNSUPPORTED,
                "Unhandled fsl_confirm_event_e value: %d",
                detail->eventId)/*does not return*/;
  }
  return 0;
}

int fsl_cx_update_seen_delta_mf(fsl_cx *f){
  int rc = 0;
  fsl_db * const d = fsl_cx_db_repo(f);
  if(d && f->cache.seenDeltaManifest <= 0){
    f->cache.seenDeltaManifest = 1;
    rc = fsl_config_set_bool(f, FSL_CONFDB_REPO,
                             "seen-delta-manifest", 1);
  }
  return rc;
}

int fsl_reserved_fn_check(fsl_cx *f, const char *zPath,
                          fsl_int_t nPath, bool relativeToCwd){
  static const int errRc = FSL_RC_RANGE;
  int rc = 0;
  char const * z1 = 0;
  if(nPath<0) nPath = (fsl_int_t)fsl_strlen(zPath);
  if(fsl_is_reserved_fn(zPath, nPath)){
    return fsl_cx_err_set(f, errRc,
                        "Filename is reserved, not legal "
                        "for adding to a repository: %.*s",
                        (int)nPath, zPath);
  }
  if(!(f->flags & FSL_CX_F_ALLOW_WINDOWS_RESERVED_NAMES)
     && fsl_is_reserved_fn_windows(zPath, nPath)){
    return fsl_cx_err_set(f, errRc,
                          "Filename is a Windows reserved name: %.*s",
                          (int)nPath, zPath);
  }
  if((z1 = fsl_cx_db_file_for_role(f, FSL_DBROLE_REPO, NULL))){
    fsl_buffer * c1 = fsl_cx_scratchpad(f);
    fsl_buffer * c2 = fsl_cx_scratchpad(f);
    rc = fsl_file_canonical_name2(relativeToCwd ? NULL : f->ckout.dir/*NULL is okay*/,
                                  z1, c1, false);
    if(!rc) rc = fsl_file_canonical_name2(relativeToCwd ? NULL : f->ckout.dir,
                                          zPath, c2, false);
    //MARKER(("\nzPath=%s\nc1=%s\nc2=%s\n", zPath,
    //fsl_buffer_cstr(c1), fsl_buffer_cstr(c2)));
    if(!rc && c1->used == c2->used &&
       0==fsl_stricmp(fsl_buffer_cstr(c1), fsl_buffer_cstr(c2))){
      rc = fsl_cx_err_set(f, errRc, "File is the repository database: %.*s",
                          (int)nPath, zPath);
    }
    fsl_cx_scratchpad_yield(f, c1);
    fsl_cx_scratchpad_yield(f, c2);
    if(rc) return rc;
  }
  assert(!rc);
  while(f->ckout.dir || relativeToCwd){
    int manifestSetting = 0;
    fsl_ckout_manifest_setting(f, &manifestSetting);
    if(!manifestSetting) break;
    typedef struct {
      short flag;
      char const * fn;
    } MSetting;
    const MSetting M[] = {
    {FSL_MANIFEST_MAIN, "manifest"},
    {FSL_MANIFEST_UUID, "manifest.uuid"},
    {FSL_MANIFEST_TAGS, "manifest.tags"},
    {0,0}
    };
    fsl_buffer * c1 = fsl_cx_scratchpad(f);
    if(f->ckout.dir){
      rc = fsl_ckout_filename_check(f, relativeToCwd, zPath, c1);
    }else{
      rc = fsl_file_canonical_name2("", zPath, c1, false);
    }
    if(rc) goto yield;
    char const * const z = fsl_buffer_cstr(c1);
    //MARKER(("Checking file against manifest setting 0x%03x: %s\n",
    //manifestSetting, z));
    for( MSetting const * m = &M[0]; m->fn; ++m ){
      if((m->flag & manifestSetting)
         && 0==fsl_strcmp(z, m->fn)){
        rc = fsl_cx_err_set(f, errRc,
                            "Filename is reserved due to the "
                            "'manifest' setting: %s",
                            m->fn);
        break;
      }
    }
    yield:
    fsl_cx_scratchpad_yield(f, c1);
    break;
  }
  return rc;
}

fsl_buffer * fsl_cx_scratchpad(fsl_cx *f){
  fsl_buffer * rc = 0;
  int i = (f->scratchpads.next<FSL_CX_NSCRATCH)
    ? f->scratchpads.next : 0;
  for(; i < FSL_CX_NSCRATCH; ++i){
    if(!f->scratchpads.used[i]){
      rc = &f->scratchpads.buf[i];
      f->scratchpads.used[i] = true;
      ++f->scratchpads.next;
      //MARKER(("Doling out scratchpad[%d] w/ capacity=%d next=%d\n",
      //        i, (int)rc->capacity, f->scratchpads.next));
      break;
    }
  }
  if(!rc){
    assert(!"Fatal fsl_cx::scratchpads misuse.");
    fsl_fatal(FSL_RC_MISUSE,
              "Fatal internal fsl_cx::scratchpads misuse: "
              "too many unyielded buffer requests.");
  }else if(0!=rc->used){
    assert(!"Fatal fsl_cx::scratchpads misuse.");
    fsl_fatal(FSL_RC_MISUSE,
              "Fatal internal fsl_cx::scratchpads misuse: "
              "used buffer after yielding it.");
  }
  return rc;
}

void fsl_cx_scratchpad_yield(fsl_cx *f, fsl_buffer * b){
  int i;
  assert(b);
  for(i = 0; i < FSL_CX_NSCRATCH; ++i){
    if(b == &f->scratchpads.buf[i]){
      assert(f->scratchpads.next != i);
      assert(f->scratchpads.used[i] && "Scratchpad misuse.");
      f->scratchpads.used[i] = false;
      fsl_buffer_reuse(b);
      if(f->scratchpads.next>i) f->scratchpads.next = i;
      //MARKER(("Yielded scratchpad[%d] w/ capacity=%d, next=%d\n",
      //        i, (int)b->capacity, f->scratchpads.next));
      return;
    }
  }
  fsl_fatal(FSL_RC_MISUSE,
            "Fatal internal fsl_cx::scratchpads misuse: "
            "passed a non-scratchpad buffer.");
}


/** @internal

   Don't use this. Use fsl_cx_rm_empty_dirs() instead.

   Attempts to remove empty directories from under a checkout,
   starting with tgtDir and working upwards until it either cannot
   remove one or it reaches the top of the checkout dir.

   The first argument must be the canonicalized absolute path to the
   checkout root. The second is the length of coRoot - if it's
   negative then fsl_strlen() is used to calculate it. The third must
   be the canonicalized absolute path to some directory under the
   checkout root. The contents of the buffer may, for efficiency's
   sake, be modified by this routine as it traverses the directory
   tree. It will never grow the buffer but may mutate its memory's
   contents.

   Returns the number of directories it is able to remove.

   Results are undefined if tgtDir is not an absolute path or does not
   have coRoot as its initial prefix.

   There are any number of valid reasons removal of a directory might
   fail, and this routine stops at the first one which does.
*/
static unsigned fsl_rm_empty_dirs(char const *coRoot, fsl_int_t rootLen,
                                  fsl_buffer * tgtDir){
  if(rootLen<0) rootLen = fsl_strlen(coRoot);
  char const * zAbs = fsl_buffer_cstr(tgtDir);
  char const * zCoDirPart = zAbs + rootLen;
  char * zEnd = fsl_buffer_str(tgtDir) + tgtDir->used - 1;
  unsigned rc = 0;
  assert(coRoot);
  if(0!=memcmp(coRoot, zAbs, (size_t)rootLen)){
    assert(!"Misuse of fsl_rm_empty_dirs()");
    return 0;
  }
  if(fsl_rmdir(zAbs)) return rc;
  ++rc;
  /** Now walk up each dir in the path and try to remove each,
      stopping when removal of one fails or we reach coRoot. */
  while(zEnd>zCoDirPart){
    for( ; zEnd>zCoDirPart && '/'!=*zEnd; --zEnd ){}
    if(zEnd==zCoDirPart) break;
    else if('/'==*zEnd){
      *zEnd = 0;
      assert(zEnd>zCoDirPart);
      if(fsl_rmdir(zAbs)) break;
      ++rc;
    }
  }
  return rc;
}

unsigned int fsl_ckout_rm_empty_dirs(fsl_cx * f, fsl_buffer * tgtDir){
  int rc = f->ckout.dir ? 0 : FSL_RC_NOT_A_CKOUT;
  if(!rc){
    rc = fsl_rm_empty_dirs(f->ckout.dir, f->ckout.dirLen, tgtDir);
  }
  return rc;
}

int fsl_ckout_rm_empty_dirs_for_file(fsl_cx * f, char const *zAbsPath){
  if(!fsl_is_rooted_in_ckout(f, zAbsPath)){
    assert(!"Internal API misuse!");
    return FSL_RC_MISUSE;
  }else{
    fsl_buffer * const p = fsl_cx_scratchpad(f);
    fsl_int_t const nAbs = (fsl_int_t)fsl_strlen(zAbsPath);
    int const rc = fsl_file_dirpart(zAbsPath, nAbs, p, false);
    if(!rc) fsl_rm_empty_dirs(f->ckout.dir, f->ckout.dirLen, p);
    fsl_cx_scratchpad_yield(f,p);
    return rc;
  }
}

bool fsl_repo_forbids_delta_manifests(fsl_cx * f){
  return fsl_config_get_bool(f, FSL_CONFDB_REPO, false,
                             "forbid-delta-manifests");
}

int fsl_ckout_fingerprint_check(fsl_cx * f){
  fsl_db * const db = fsl_needs_ckout(f);
  if(!db) return FSL_RC_NOT_A_CKOUT;
  int rc = 0;
  char const * zCkout = 0;
  char * zRepo = 0;
  fsl_id_t rcvCkout = 0;
  fsl_buffer * const buf = fsl_cx_scratchpad(f);
  rc = fsl_config_get_buffer(f, FSL_CONFDB_CKOUT, "fingerprint", buf);
  if(FSL_RC_NOT_FOUND==rc){
    /* Older checkout with no fingerprint. Assume it's okay. */
    rc = 0;
    goto end;
  }else if(rc){
    goto end;
  }
  zCkout = fsl_buffer_cstr(buf);
#if 0
  /* Inject a bogus byte for testing purposes */
  buf->mem[6] = 'x';
#endif
  rcvCkout = (fsl_id_t)atoi(zCkout);
  rc = fsl_repo_fingerprint_search(f, rcvCkout, &zRepo);
  switch(rc){
    case FSL_RC_NOT_FOUND: goto mismatch;
    case 0:
      assert(zRepo);
      if(fsl_strcmp(zRepo,zCkout)) goto mismatch;
      break;
    default:
      break;
  }
  end:
  fsl_cx_scratchpad_yield(f, buf);
  fsl_free(zRepo);
  return rc;
  mismatch:
  rc = fsl_cx_err_set(f, FSL_RC_REPO_MISMATCH,
                      "Mismatch found between repo/checkout "
                      "fingerprints.");
  goto end;
}

#if 0
struct tm * fsl_cx_localtime( fsl_cx const * f, const time_t * clock ){
  if(!clock) return NULL;
  else if(!f) return localtime(clock);
  else return (f->flags & FSL_CX_F_LOCALTIME_GMT)
         ? gmtime(clock)
         : localtime(clock)
         ;
}

struct tm * fsl_localtime( const time_t * clock ){
  return fsl_cx_localtime(NULL, clock);
}

time_t fsl_cx_time_adj(fsl_cx const * f, time_t clock){
  struct tm * tm = fsl_cx_localtime(f, &clock);
  return tm ? mktime(tm) : 0;
}
#endif

#undef MARKER
#undef FSL_CX_NSCRATCH
