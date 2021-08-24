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
/***********************************************************************
  This file implements most of the fsl_repo_xxx() APIs.
*/
#include "fossil-scm/fossil-internal.h"
#include "fossil-scm/fossil-repo.h"
#include "fossil-scm/fossil-checkout.h"
#include "fossil-scm/fossil-hash.h"
#include "fossil-scm/fossil-confdb.h"
#include <assert.h>
#include <memory.h> /* memcpy() */
#include <time.h> /* time() */
#include <errno.h>

/* Only for debugging */
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)

/**
   Calculate the youngest ancestor of the given blob.rid value that is a member of
   branch zBranch.

   Returns the blob.id value of the matching record, 0 if not found,
   or a negative value on error.

   Potential TODO: do we need this in the public API?
*/
static fsl_id_t fsl_youngest_ancestor_in_branch(fsl_cx * f, fsl_id_t rid,
                                                const char *zBranch){
  fsl_db * const db = fsl_needs_repo(f);
  if(!db) return (fsl_id_t)-1;
  return fsl_db_g_id(db, 0,
    "WITH RECURSIVE "
    "  ancestor(rid, mtime) AS ("
    "    SELECT %"FSL_ID_T_PFMT", "
    "      mtime FROM event WHERE objid=%"FSL_ID_T_PFMT
    "    UNION "
    "    SELECT plink.pid, event.mtime"
    "      FROM ancestor, plink, event"
    "     WHERE plink.cid=ancestor.rid"
    "       AND event.objid=plink.pid"
    "     ORDER BY mtime DESC"
    "  )"
    "  SELECT ancestor.rid FROM ancestor"
    "   WHERE EXISTS(SELECT 1 FROM tagxref"
                    " WHERE tagid=%d AND tagxref.rid=ancestor.rid"
                    "   AND value=%Q AND tagtype>0)"
    "  LIMIT 1",
    rid, rid, FSL_TAGID_BRANCH, zBranch
  );
}

/**
   TODO: figure out if this needs to be in the public API and, if it does,
   change its signature to:

   int fsl_branch_of_rid(fsl_cx *f, fsl_int_t rid, char **zOut )

   So that we can distinguish "not found" from OOM errors.
*/
static char * fsl_branch_of_rid(fsl_cx *f, fsl_int_t rid){
  char *zBr = 0;
  fsl_db * const db = fsl_cx_db_repo(f);
  fsl_stmt * st = 0;
  int rc;
  assert(db);
  rc = fsl_db_prepare_cached(db, &st,
      "SELECT value FROM tagxref "
      "WHERE rid=? AND tagid=%d "
      "AND tagtype>0 "
      "/*%s()*/", FSL_TAGID_BRANCH,__func__);
  if(rc) return 0;
  rc = fsl_stmt_bind_id(st, 1, rid);
  if(rc) goto end;
  if( fsl_stmt_step(st)==FSL_RC_STEP_ROW ){
    zBr = fsl_strdup(fsl_stmt_g_text(st,0,0));
    if(!zBr) rc = FSL_RC_OOM;
  }
  end:
  fsl_stmt_cached_yield(st);
  if( !rc && zBr==0 ){
    zBr = fsl_config_get_text(f, FSL_CONFDB_REPO, "main-branch", 0);
  }
  return zBr;
}

/**
   morewt ==> most recent event with tag

   Comments from original fossil implementation:

   Find the RID of the most recent object with symbolic tag zTag and
   having a type that matches zType.

   Return 0 if there are no matches.

   This is a tricky query to do efficiently.  If the tag is very
   common (ex: "trunk") then we want to use the query identified below
   as Q1 - which searching the most recent EVENT table entries for the
   most recent with the tag.  But if the tag is relatively scarce
   (anything other than "trunk", basically) then we want to do the
   indexed search show below as Q2.
*/
static fsl_id_t fsl_morewt(fsl_cx * const f, const char *zTag, fsl_satype_e type){
  char const * zType = fsl_satype_event_cstr(type);
  return fsl_db_g_id(fsl_cx_db_repo(f), 0,
    "SELECT objid FROM ("
      /* Q1:  Begin by looking for the tag in the 30 most recent events */
      "SELECT objid"
       " FROM (SELECT * FROM event ORDER BY mtime DESC LIMIT 30) AS ex"
      " WHERE type GLOB '%q'"
        " AND EXISTS(SELECT 1 FROM tagxref, tag"
                     " WHERE tag.tagname='sym-%q'"
                       " AND tagxref.tagid=tag.tagid"
                       " AND tagxref.tagtype>0"
                       " AND tagxref.rid=ex.objid)"
      " ORDER BY mtime DESC LIMIT 1"
    ") UNION ALL SELECT * FROM ("
      /* Q2: If the tag is not found in the 30 most recent events, then using
      ** the tagxref table to index for the tag */
      "SELECT event.objid"
       " FROM tag, tagxref, event"
      " WHERE tag.tagname='sym-%q'"
        " AND tagxref.tagid=tag.tagid"
        " AND tagxref.tagtype>0"
        " AND event.objid=tagxref.rid"
        " AND event.type GLOB '%q'"
      " ORDER BY event.mtime DESC LIMIT 1"
    ") LIMIT 1;",
    zType, zTag, zTag, zType
  );
}

/**
   Modes for fsl_start_of_branch().
*/
enum fsl_stobr_type {
/**
   The check-in of the parent branch off of which
   the branch containing RID originally diverged.
*/
FSL_STOBR_ORIGIN = 0,
/**
   The first check-in of the branch that contains RID.
*/
FSL_STOBR_FIRST_CI = 1,
/**
   The youngest ancestor of RID that is on the branch from which the
   branch containing RID diverged.
*/
FSL_STOBR_YOAN = 2
};

/*
** Return the RID that is the "root" of the branch that contains
** check-in "rid".  Details depending on eType. If not found, rid is
** returned.
*/
static fsl_id_t fsl_start_of_branch(fsl_cx * f, fsl_id_t rid,
                                    enum fsl_stobr_type eType){
  fsl_db * db;
  fsl_stmt q = fsl_stmt_empty;
  int rc;
  fsl_id_t ans = rid;
  char *zBr = fsl_branch_of_rid(f, rid);
  if(!zBr){
    goto oom;
  }
  db = fsl_cx_db_repo(f);
  assert(db);
  rc = fsl_db_prepare(db, &q,
    "SELECT pid, EXISTS(SELECT 1 FROM tagxref"
                       " WHERE tagid=%d AND tagtype>0"
                       "   AND value=%Q AND rid=plink.pid)"
    "  FROM plink"
    " WHERE cid=? AND isprim",
    FSL_TAGID_BRANCH, zBr
  );
  fsl_free(zBr);
  zBr = 0;
  if(rc){
    ans = -2;
    fsl_cx_uplift_db_error(f, db);
    MARKER(("Internal error: fsl_db_prepare() says: %s\n", fsl_rc_cstr(rc)));
    goto end;
  }
  do{
    fsl_stmt_reset(&q);
    fsl_stmt_bind_id(&q, 1, ans);
    rc = fsl_stmt_step(&q);
    if( rc!=FSL_RC_STEP_ROW ) break;
    if( eType==FSL_STOBR_FIRST_CI && fsl_stmt_g_int32(&q,1)==0 ){
      break;
    }
    ans = fsl_stmt_g_id(&q, 0);
  }while( fsl_stmt_g_int32(&q, 1)==1 && ans>0 );
  fsl_stmt_finalize(&q);
  end:
  if( ans>0 && eType==FSL_STOBR_YOAN ){
    zBr = fsl_branch_of_rid(f, ans);
    if(zBr){
      ans = fsl_youngest_ancestor_in_branch(f, rid, zBr);
      fsl_free(zBr);
    }else{
      goto oom;
    }
  }
  return ans;
  oom:
  fsl_cx_err_set(f, FSL_RC_OOM, NULL);
  return -1;
}

int fsl_sym_to_rid( fsl_cx * f, char const * sym, fsl_satype_e type,
                    fsl_id_t * rv ){
  fsl_id_t rid = 0;
  fsl_id_t vid;
  fsl_size_t symLen;
  /* fsl_int_t i; */
  fsl_db * dbR = fsl_cx_db_repo(f);
  fsl_db * dbC = fsl_cx_db_ckout(f);
  bool startOfBranch = 0;
  int rc = 0;

  if(!f || !sym || !*sym || !rv) return FSL_RC_MISUSE;
  else if(!dbR) return FSL_RC_NOT_A_REPO;

  if(FSL_SATYPE_BRANCH_START==type){
    /* The original implementation takes a (char const *) for the
       type, and treats "b" (branch?) as a special case of
       FSL_SATYPE_CHECKIN, resets the type to "ci", then sets
       startOfBranch to 1. We introduced the FSL_SATYPE_BRANCH
       pseudo-type for that purpose. That said: the original code
       base does not, as of this writing (2021-02-15) appear to actually
       use this feature anywhere. */
    type = FSL_SATYPE_CHECKIN;
    startOfBranch = 1;
  }

  /* special keyword: "tip" */
  if( 0==fsl_strcmp(sym,"tip")
      && (FSL_SATYPE_ANY==type || FSL_SATYPE_CHECKIN==type)){
    rid = fsl_db_g_id(dbR, 0,
                      "SELECT objid FROM event"
                      " WHERE type='ci'"
                      " ORDER BY event.mtime DESC"
                      " LIMIT 1");
    if(rid>0) goto gotit;
  }
  /* special keywords: "prev", "previous", "current", and "next".
     These require a checkout.
  */
  vid = dbC ? f->ckout.rid : 0;
  //MARKER(("has vid=%"FSL_ID_T_PFMT"\n", vid));
  if( vid>0){
    if( 0==fsl_strcmp(sym, "current") ){
      rid = vid;
    }
    else if( 0==fsl_strcmp(sym, "prev")
             || 0==fsl_strcmp(sym, "previous") ){
      rid = fsl_db_g_id(dbR, 0,
                        "SELECT pid FROM plink WHERE "
                        "cid=%"FSL_ID_T_PFMT" AND isprim",
                        (fsl_id_t)vid);
    }
    else if( 0==fsl_strcmp(sym, "next") ){
      rid = fsl_db_g_id(dbR, 0,
                        "SELECT cid FROM plink WHERE "
                        "pid=%"FSL_ID_T_PFMT
                        " ORDER BY isprim DESC, mtime DESC",
                        (fsl_id_t)vid);
    }
    if(rid>0) goto gotit;
  }

  /* Date and times */
  if( 0==memcmp(sym, "date:", 5) ){
    rid = fsl_db_g_id(dbR, 0, 
                      "SELECT objid FROM event"
                      " WHERE mtime<=julianday(%Q,'utc')"
                      " AND type GLOB '%q'"
                      " ORDER BY mtime DESC LIMIT 1",
                      sym+5, fsl_satype_event_cstr(type));
    *rv = rid;
    return 0;
  }
  if( fsl_str_is_date(sym) ){
    rid = fsl_db_g_id(dbR, 0, 
                      "SELECT objid FROM event"
                      " WHERE mtime<=julianday(%Q,'utc')"
                      " AND type GLOB '%q'"
                      " ORDER BY mtime DESC LIMIT 1",
                      sym, fsl_satype_event_cstr(type));
    if(rid>0) goto gotit;
  }

  /* Deprecated time formats elided: local:..., utc:... */

  /* "tag:" + symbolic-name */
  if( memcmp(sym, "tag:", 4)==0 ){
    rid = fsl_morewt(f, sym+4, type);
    if(rid>0 && startOfBranch){
      rid = fsl_start_of_branch(f, rid, FSL_STOBR_FIRST_CI);
    }
    goto gotit;
  }

  /* root:TAG -> The origin of the branch */
  if( memcmp(sym, "root:", 5)==0 ){
    rc = fsl_sym_to_rid(f, sym+5, type, &rid);
    if(!rc && rid>0){
      rid = fsl_start_of_branch(f, rid, FSL_STOBR_ORIGIN);
    }
    goto gotit;
  }  

  /* merge-in:TAG -> Most recent merge-in for the branch */
  if( memcmp(sym, "merge-in:", 9)==0 ){
    rc = fsl_sym_to_rid(f, sym+9, type, &rid);
    if(!rc){
      rid = fsl_start_of_branch(f, rid, FSL_STOBR_YOAN);
    }
    goto gotit;
  }  
  
  symLen = fsl_strlen(sym);
  /* SHA1/SHA3 hash or prefix */
  if( symLen>=4
      && symLen<=FSL_STRLEN_K256
      && fsl_validate16(sym, symLen) ){
    fsl_stmt q = fsl_stmt_empty;
    char zUuid[FSL_STRLEN_K256+1];
    memcpy(zUuid, sym, symLen);
    zUuid[symLen] = 0;
    fsl_canonical16(zUuid, symLen);
    rid = 0;
    /* Reminder to self: caching these queries would be cool but it
       can't work with the GLOBs.
    */
    if( FSL_SATYPE_ANY==type ){
      fsl_db_prepare(dbR, &q,
                       "SELECT rid FROM blob WHERE uuid GLOB '%s*'",
                       zUuid);
    }else{
      fsl_db_prepare(dbR, &q,
                     "SELECT blob.rid"
                     "  FROM blob, event"
                     " WHERE blob.uuid GLOB '%s*'"
                     "   AND event.objid=blob.rid"
                     "   AND event.type GLOB '%q'",
                     zUuid, fsl_satype_event_cstr(type) );
    }
    if( fsl_stmt_step(&q)==FSL_RC_STEP_ROW ){
      int64_t r64 = 0;
      fsl_stmt_get_int64(&q, 0, &r64);
      if( fsl_stmt_step(&q)==FSL_RC_STEP_ROW ) rid = -1
        /* Ambiguous results */
        ;
      else rid = (fsl_id_t)r64;
    }
    fsl_stmt_finalize(&q);
    if(rid<0){
      fsl_cx_err_set(f, FSL_RC_AMBIGUOUS,
                     "Symbolic name is ambiguous: %s",
                     sym);
    }
    goto gotit
      /* None of the further checks against the sym can pass. */
      ;
  }

  if(FSL_SATYPE_WIKI==type){
    rid = fsl_db_g_id(dbR, 0,
                    "SELECT event.objid, max(event.mtime)"
                    "  FROM tag, tagxref, event"
                    " WHERE tag.tagname='sym-%q' "
                    "   AND tagxref.tagid=tag.tagid AND tagxref.tagtype>0 "
                    "   AND event.objid=tagxref.rid "
                    "   AND event.type GLOB '%q'",
                    sym, fsl_satype_event_cstr(type)
    );
  }else{
    rid = fsl_morewt(f, sym, type);
    //MARKER(("morewt(%s,%s) == %d\n", sym, fsl_satype_cstr(type), (int)rid));
  }

  if( rid>0 ){
    if(startOfBranch) rid = fsl_start_of_branch(f, rid,
                                                FSL_STOBR_FIRST_CI);
    goto gotit;
  }

  /* Undocumented: rid:### ==> rid */
  if(symLen>4 && 0==fsl_strncmp("rid:",sym,4)){
    int i;
    char const * oldSym = sym;
    sym += 4;
    for(i=0; fsl_isdigit(sym[i]); i++){}
    if( sym[i]==0 ){
      if( FSL_SATYPE_ANY==type ){
        rid = fsl_db_g_id(dbR, 0, 
                          "SELECT rid"
                          "  FROM blob"
                          " WHERE rid=%s",
                          sym);
      }else{
        rid = fsl_db_g_id(dbR, 0, 
                          "SELECT event.objid"
                          "  FROM event"
                          " WHERE event.objid=%s"
                          "   AND event.type GLOB '%q'",
                          sym, fsl_satype_event_cstr(type));
      }
      if( rid>0 ) goto gotit;
    }
    sym = oldSym;
  }

  gotit:
  if(rid<=0){
    return f->error.code
      ? f->error.code
      : fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                       "Could not resolve symbolic name "
                       "'%s' as artifact type '%s'.",
                       sym, fsl_satype_event_cstr(type) );
  }
  assert(0==rc);
  *rv = rid;
  return rc;
}

fsl_id_t fsl_uuid_to_rid2( fsl_cx * f, fsl_uuid_cstr uuid,
                           fsl_phantom_e mode ){
    if(!f) return -1;
    else if(!fsl_is_uuid(uuid)){
      fsl_cx_err_set(f, FSL_RC_MISUSE,
                     "fsl_uuid_to_rid2() requires a "
                     "full UUID. Got: %s", uuid);
      return -2;
    }else{
      fsl_id_t rv;
      rv = fsl_uuid_to_rid(f, uuid);
      if((0==rv) && (FSL_PHANTOM_NONE!=mode)
         && 0!=fsl_content_new(f, uuid,
                               (FSL_PHANTOM_PRIVATE==mode),
                               &rv)){
        assert(f->error.code);
        rv = -3;
      }
      return rv;
    }
}

int fsl_sym_to_uuid( fsl_cx * f, char const * sym, fsl_satype_e type,
                     fsl_uuid_str * rv, fsl_id_t * rvId ){
  fsl_id_t rid = 0;
  fsl_db * dbR = fsl_needs_repo(f);
  fsl_uuid_str rvv = NULL;
  int rc = dbR
    ? fsl_sym_to_rid(f, sym, type, &rid)
    : FSL_RC_NOT_A_REPO;
  if(!rc){
    if(rvId) *rvId = rid;
    rvv = fsl_rid_to_uuid(f, rid)
      /* TODO: use a cached "exists" check if !rv, to avoid allocating
         rvv if we don't need it.
      */;
    if(!rvv){
      if(!f->error.code){
        rc = fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                            "Cannot find UUID for RID %"FSL_ID_T_PFMT".",
                            rid);
      }
    }
    else if(rv){
      *rv = rvv;
    }else{
      fsl_free( rvv );
    }
  }
  return rc;
}

fsl_id_t fsl_uuid_to_rid( fsl_cx * f, char const * uuid ){
  fsl_db * const db = fsl_needs_repo(f);
  fsl_size_t const uuidLen = (uuid && db) ? fsl_strlen(uuid) : 0;
  if(!f || !uuid || !uuidLen) return -1;
  else if(!db){
    /* f's error state has already been set */
    return -2;
  }
  else if(!fsl_validate16(uuid, uuidLen)){
    fsl_cx_err_set(f, FSL_RC_RANGE, "Invalid UUID (prefix): %s", uuid);
    return -3;
  }
  else if(uuidLen>FSL_STRLEN_K256){
    fsl_cx_err_set(f, FSL_RC_RANGE, "UUID is too long: %s", uuid);
    return -4;
  }
  else {
    fsl_id_t rid = -5;
    fsl_stmt q = fsl_stmt_empty;
    fsl_stmt * qS = NULL;
    int rc;
    rc = fsl_is_uuid_len((int)uuidLen)
      /* Optimization for the common internally-used case.

         FIXME: there is an *astronomically small* chance of a prefix
         collision on a v1-length uuidLen against a v2-length
         blob.uuid value, leading to no match found for an existing v2
         uuid here. Like... a *REALLY* small chance.
      */
      ? fsl_db_prepare_cached(db, &qS,
                              "SELECT rid FROM blob WHERE "
                              "uuid=? /*%s()*/",__func__)
      : fsl_db_prepare(db, &q,
                       "SELECT rid FROM blob WHERE "
                       "uuid GLOB '%s*'",
                       uuid);
    if(!rc){
      fsl_stmt * st = qS ? qS : &q;
      if(qS){
        rc = fsl_stmt_bind_text(qS, 1, uuid, (fsl_int_t)uuidLen, 0);
      }
      if(!rc){
        rc = fsl_stmt_step(st);
        switch(rc){
          case FSL_RC_STEP_ROW:
            rc = 0;
            rid = fsl_stmt_g_id(st, 0);
            if(!qS){
              /*
                Check for an ambiguous result. We don't need this for
                the (qS==st) case because that one does an exact match
                on a unique key.
              */
              rc = fsl_stmt_step(st);
              switch(rc){
                case FSL_RC_STEP_ROW:
                  rc = 0;
                  fsl_cx_err_set(f, FSL_RC_AMBIGUOUS,
                                 "UUID prefix is ambiguous: %s",
                                 uuid);
                  rid = -6;
                break;
                case FSL_RC_STEP_DONE:
                  /* Unambiguous UUID */
                  rc = 0;
                  break;
                default:
                  assert(st->db->error.code);
                  /* fall through and uplift the db error below... */
              }
            }
            break;
          case FSL_RC_STEP_DONE:
            /* No entry found */
            rid = 0;
            rc = 0;
            break;
          default:
            assert(st->db->error.code);
            rid = -7;
            break;
        }
      }
      if(rc && db->error.code && !f->error.code){
        fsl_cx_uplift_db_error(f, db);
      }
      if(qS) fsl_stmt_cached_yield(qS);
      else fsl_stmt_finalize(&q);
    }
    return rid;
  }
}

fsl_id_t fsl_repo_filename_fnid( fsl_cx * f, char const * fn ){
  fsl_id_t rv = 0;
  int const rc = fsl_repo_filename_fnid2(f, fn, &rv, false);
  return rv>=0 ? rv : (rc>0 ? -rc : rc);
}

int fsl_repo_filename_fnid2( fsl_cx * f, char const * fn, fsl_id_t * rv, bool createNew ){
  fsl_db * db = fsl_cx_db_repo(f);
  fsl_id_t fnid = 0;
  fsl_stmt * qSel = NULL;
  int rc;
  assert(f);
  assert(db);
  assert(rv);
  if(!fn || !fsl_is_simple_pathname(fn, 1)){
    return fsl_cx_err_set(f, FSL_RC_RANGE,
                          "Filename is not a \"simple\" path: %s",
                          fn);
  }
  *rv = 0;
  rc = fsl_db_prepare_cached(db, &qSel,
                             "SELECT fnid FROM filename "
                             "WHERE name=? "
                             "/*%s()*/",__func__);
  if(rc){
      fsl_cx_uplift_db_error(f, db);
      return rc;
  }
  rc = fsl_stmt_bind_text(qSel, 1, fn, -1, 0);
  if(rc){
    fsl_stmt_cached_yield(qSel);
  }else{
    rc = fsl_stmt_step(qSel);
    if( FSL_RC_STEP_ROW == rc ){
      rc = 0;
      fnid = fsl_stmt_g_id(qSel, 0);
      assert(fnid>0);
    }else if(FSL_RC_STEP_DONE == rc){
      rc = 0;
    }
    fsl_stmt_cached_yield(qSel);
    if(!rc && (fnid==0) && createNew){
      fsl_stmt * qIns = NULL;
      rc = fsl_db_prepare_cached(db, &qIns,
                                 "INSERT INTO filename(name) "
                                 "VALUES(?) /*%s()*/",__func__);
      if(!rc){
        rc = fsl_stmt_bind_text(qIns, 1, fn, -1, 0);
        if(!rc){
          rc = fsl_stmt_step(qIns);
          if(FSL_RC_STEP_DONE==rc){
            rc = 0;
            fnid = fsl_db_last_insert_id(db);
          }
        }
        fsl_stmt_cached_yield(qIns);
      }
    }
  }
  if(!rc){
    assert(!createNew || (fnid>0));
    *rv = fnid;
  }else if(db->error.code){
    fsl_cx_uplift_db_error(f, db);
  }
  return rc;
}

int fsl_delta_src_id( fsl_cx * f, fsl_id_t deltaRid, fsl_id_t * rv ){
  fsl_db * const dbR = fsl_cx_db_repo(f);
  if(!rv) return FSL_RC_MISUSE;
  else if(deltaRid<=0) return FSL_RC_RANGE;
  else if(!dbR) return FSL_RC_NOT_A_REPO;
  else {
    int rc;
    fsl_stmt * q = NULL;
    rc = fsl_db_prepare_cached(dbR, &q,
                               "SELECT srcid FROM delta "
                               "WHERE rid=? /*%s()*/",__func__);
    if(!rc){
      rc = fsl_stmt_bind_id(q, 1, deltaRid);
      if(!rc){
        if(FSL_RC_STEP_ROW==(rc=fsl_stmt_step(q))){
          rc = 0;
          *rv = fsl_stmt_g_id(q, 0);
        }else if(FSL_RC_STEP_DONE==rc){
          rc = 0;
          *rv = 0;
        }
      }
      fsl_stmt_cached_yield(q);
    }
    return rc;
  }
}



int fsl_repo_verify_before_commit( fsl_cx * f, fsl_id_t rid ){
  if(0){
    /*
       v1 adds a commit hook here on the first entry, but it only
       seems to ever use one commit hook, so the infrastructure seems
       like overkill here. Thus this final verification is called from
       the commit (that's where v1 calls the hook).

       If we eventually add commit hooks, this is the place to do it.
    */
  }
  assert( fsl_cx_db_repo(f)->beginCount > 0 );
  return rid>0
    ? fsl_id_bag_insert(&f->cache.toVerify, rid)
    : FSL_RC_RANGE;    
}

void fsl_repo_verify_cancel( fsl_cx * f ){
  fsl_id_bag_clear(&f->cache.toVerify);
}

int fsl_rid_to_uuid2(fsl_cx * f, fsl_id_t rid, fsl_buffer *uuid){
  fsl_db * db = f ? fsl_cx_db_repo(f) : NULL;
  if(!f || !db || (rid<=0)){
    return fsl_cx_err_set(f, FSL_RC_MISUSE,
                          "fsl_rid_to_uuid2() requires "
                          "an opened repository and a "
                          "positive RID value. rid=%" FSL_ID_T_PFMT,
                          rid);
  }else{
    fsl_stmt * st = NULL;
    int rc;
    fsl_buffer_reuse(uuid);
    rc = fsl_db_prepare_cached(db, &st,
                               "SELECT uuid FROM blob "
                               "WHERE rid=? "
                               "/*%s()*/", __func__);
    if(!rc){
      rc = fsl_stmt_bind_id(st, 1, rid);
      if(!rc){
        rc = fsl_stmt_step(st);
        if(FSL_RC_STEP_ROW==rc){
          fsl_size_t len = 0;
          char const * x = fsl_stmt_g_text(st, 0, &len);
          rc = fsl_buffer_append(uuid, x, (fsl_int_t)len);
        }else if(FSL_RC_STEP_DONE){
          rc = fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                              "No blob found for rid %" FSL_ID_T_PFMT ".",
                              rid);
        }
      }
      fsl_stmt_cached_yield(st);
      if(rc && !f->error.code){
        assert(db->error.code);
        fsl_cx_uplift_db_error(f, db);
      }
    }
    return rc;
  }
}

fsl_uuid_str fsl_rid_to_uuid(fsl_cx * f, fsl_id_t rid){
  fsl_buffer uuid = fsl_buffer_empty;
  fsl_rid_to_uuid2(f, rid, &uuid);
  return fsl_buffer_take(&uuid);
}

fsl_uuid_str fsl_rid_to_artifact_uuid(fsl_cx * f, fsl_id_t rid, fsl_satype_e type){
  fsl_db * db = f ? fsl_cx_db_repo(f) : NULL;
  if(!f || !db || (rid<=0)) return NULL;
  else{
    char * rv = NULL;
    fsl_stmt * st = NULL;
    int rc;
    rc = fsl_db_prepare_cached(db, &st,
                               "SELECT uuid FROM blob "
                               "WHERE rid=?1 AND EXISTS "
                               "(SELECT 1 FROM event"
                               " WHERE event.objid=?1 "
                               " AND event.type GLOB %Q)"
                               "/*%s()*/",
                               fsl_satype_event_cstr(type),
                               __func__);
    if(!rc){
      rc = fsl_stmt_bind_id(st, 1, rid);
      if(!rc){
        rc = fsl_stmt_step(st);
        if(FSL_RC_STEP_ROW==rc){
          fsl_size_t len = 0;
          char const * x = fsl_stmt_g_text(st, 0, &len);
          rv = x ? fsl_strndup(x, (fsl_int_t)len ) : NULL;
          if(x && !rv){
            fsl_cx_err_set(f, FSL_RC_OOM, NULL);
          }
        }else if(FSL_RC_STEP_DONE){
          fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                         "No %s artifact found with rid %"FSL_ID_T_PFMT".",
                         fsl_satype_cstr(type), (fsl_id_t) rid);
        }
      }
      fsl_stmt_cached_yield(st);
      if(rc && !f->error.code){
        fsl_cx_uplift_db_error(f, db);
      }
    }
    return rv;
  }
}


/**
    Load the record identified by rid. Make sure we can reproduce it
    without error.
   
    Return non-0 and set f's error state if anything goes wrong.  If
    this procedure returns 0 it means that everything looks OK.
 */
static int fsl_repo_verify_rid(fsl_cx * f, fsl_id_t rid){
  fsl_uuid_str uuid = NULL;
  fsl_buffer hash = fsl_buffer_empty;
  fsl_buffer content = fsl_buffer_empty;
  int rc;
  fsl_db * db;
  if( fsl_content_size(f, rid)<0 ){
    return 0 /* No way to verify phantoms */;
  }
  db = fsl_cx_db_repo(f);
  assert(db);
  uuid = fsl_rid_to_uuid(f, rid);
  if(!uuid){
    rc = fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                        "Could not find blob record for "
                        "rid #%"FSL_ID_T_PFMT".",
                        rid);
  }
  else{
    int const uuidLen = fsl_is_uuid(uuid);
    if(!uuidLen){
      rc = fsl_cx_err_set(f, FSL_RC_RANGE,
                          "Invalid uuid for rid #%"FSL_ID_T_PFMT": %s",
                          (fsl_id_t)rid, uuid);
    }
    else if( 0==(rc=fsl_content_get(f, rid, &content)) ){
      /* This test can fail for artifacts which have an SHA1 hash in a
         repo with an SHA3 policy. A test case from the main fossil
         repo: c7dd1de9f9539a5a859c2b41fe4560604a774476

         This test hashes it (in that repo) as SHA3. As a workaround,
         if the hash is an SHA1 the we will temporarily force the hash
         policy to SHA1, and similarly for SHA3. Lame, but nothing
         better currently comes to mind.

         TODO: change the signature of fsl_cx_hash_buffer() to
         optionally take a forced policy, or supply a similar function
         which does what we're doing below.
      */
      fsl_hashpolicy_e const oldHashP = f->cxConfig.hashPolicy;
      f->cxConfig.hashPolicy = (uuidLen==FSL_STRLEN_SHA1)
        ? FSL_HPOLICY_SHA1 : FSL_HPOLICY_SHA3;
      rc = fsl_cx_hash_buffer(f, 0, &content, &hash);
      f->cxConfig.hashPolicy = oldHashP;
      if( !rc && 0!=fsl_uuidcmp(uuid, fsl_buffer_cstr(&hash)) ){
        rc = fsl_cx_err_set(f, FSL_RC_CONSISTENCY,
                            "Hash of rid %"FSL_ID_T_PFMT" (%b) "
                            "does not match its uuid (%s)",
                            (fsl_id_t)rid, &hash, uuid);
      }
    }
  }
  fsl_free(uuid);
  fsl_buffer_clear(&hash);
  fsl_buffer_clear(&content);
  return rc;
}


int fsl_repo_verify_at_commit( fsl_cx * f ){
  fsl_id_t rid;
  int rc = 0;
  fsl_id_bag * bag = &f->cache.toVerify;
  /* v1 does content_cache_clear() here. */
  f->cache.inFinalVerify = 1;
  rid = fsl_id_bag_first(bag);
  if(f->cxConfig.traceSql){
    fsl_db_exec(f->dbMain,
                "SELECT 'Starting verify-at-commit.'");
  }
  while( !rc && rid>0 ){
    rc = fsl_repo_verify_rid(f, rid);
    if(!rc) rid = fsl_id_bag_next(bag, rid);
  }
  fsl_id_bag_clear(bag);
  f->cache.inFinalVerify = 0;
  if(rc && !f->error.code){
    fsl_cx_err_set(f, rc,
                   "Error #%d (%s) in fsl_repo_verify_at_commit()",
                   rc, fsl_rc_cstr(rc));
  }
  return rc;
}


static int fsl_repo_create_default_users(fsl_db * db, char addOnlyUser,
                                         char const * defaultUser ){
  int rc = fsl_db_exec(db,
                       "INSERT OR IGNORE INTO user(login, info) "
                       "VALUES(%Q,'')", defaultUser);
  if(!rc){
    rc = fsl_db_exec(db,
                     "UPDATE user SET cap='s', pw=lower(hex(randomblob(3)))"
                     " WHERE login=%Q", defaultUser);
    if( !rc && !addOnlyUser ){
      fsl_db_exec_multi(db,
                        "INSERT OR IGNORE INTO user(login,pw,cap,info)"
                        "   VALUES('anonymous',hex(randomblob(8)),'hmncz',"
                        "          'Anon');"
                        "INSERT OR IGNORE INTO user(login,pw,cap,info)"
                        "   VALUES('nobody','','gjor','Nobody');"
                        "INSERT OR IGNORE INTO user(login,pw,cap,info)"
                        "   VALUES('developer','','dei','Dev');"
                        "INSERT OR IGNORE INTO user(login,pw,cap,info)"
                        "   VALUES('reader','','kptw','Reader');"
                        );
    }
  }
  return rc;                       
}

int fsl_repo_create(fsl_cx * f, fsl_repo_create_opt const * opt ){
  fsl_db * db = 0;
  fsl_cx F = fsl_cx_empty /* used if !f */;
  int rc = 0;
  char const * userName = 0;
  fsl_time_t const unixNow = (fsl_time_t)time(0);
  char fileExists;
  char inTrans = 0;
  extern int fsl_cx_attach_role(fsl_cx * f, const char *zDbName, fsl_dbrole_e r)
    /* Internal routine from fsl_cx.c */;
  if(!opt || !opt->filename) return FSL_RC_MISUSE;
  fileExists = 0 == fsl_file_access(opt->filename,0);
  if(fileExists && !opt->allowOverwrite){
    return f
      ? fsl_cx_err_set(f, FSL_RC_ALREADY_EXISTS,
                       "File already exists and "
                       "allowOverwrite is false: %s",
                       opt->filename)
      : FSL_RC_ALREADY_EXISTS;
  }
  if(f){
    rc = fsl_ckout_close(f)
      /* Will fail if a transaction is active! */;
    switch(rc){
      case 0:
      case FSL_RC_NOT_FOUND:
        rc = 0;
        break;
      default:
        return rc;
    }
  }else{
    f = &F;
    rc = fsl_cx_init( &f, NULL );
    if(rc){
      fsl_cx_finalize(f);
      return rc;
    }
  }
  /* We probably should truncate/unlink the file here
     before continuing, to ensure a clean slate.
  */
  if(fileExists){
#if 0
    FILE * file = fsl_fopen(opt->filename, "w"/*truncates it*/);
    if(!file){
      rc = fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_IO),
                          "Cannot open '%s' for writing.",
                          opt->filename);
      goto end2;    
    }else{
      fsl_fclose(file);
    }
#else
    rc = fsl_file_unlink(opt->filename);
    if(rc){
      rc = fsl_cx_err_set(f, rc, "Cannot unlink existing repo file: %s",
                          opt->filename);
      goto end2;
    }
#endif
  }
  rc = fsl_cx_attach_role(f, opt->filename, FSL_DBROLE_REPO);
  if(rc){
    goto end2;
  }
  db = fsl_cx_db(f);
  if(!f->repo.user){
    f->repo.user = fsl_guess_user_name()
      /* Ignore OOM error here - we'll use 'root'
         by default (but if we're really OOM here then
         the next op will fail).
      */;
  }
  userName = opt->username;

  rc = fsl_db_transaction_begin(db);
  if(rc) goto end1;
  inTrans = 1;
  /* Install the schemas... */
  rc = fsl_db_exec_multi(db, "%s; %s; %s; %s",
                         fsl_schema_repo1(),
                         fsl_schema_repo2(),
                         fsl_schema_ticket(),
                         fsl_schema_ticket_reports());
  if(rc) goto end1;

  if(1){
    /*
      Set up server-code and project-code...

      in fossil this is optional, so we will presumably eventually
      have to make it so here as well. Not yet sure where this routine
      is used in fossil (i.e. whether the option is actually
      exercised).
    */
    rc = fsl_db_exec_multi(db,
                           "INSERT INTO repo.config (name,value,mtime) "
                           "VALUES ('server-code',"
                           "lower(hex(randomblob(20))),"
                           "%"PRIi64");"
                           "INSERT INTO repo.config (name,value,mtime) "
                           "VALUES ('project-code',"
                           "lower(hex(randomblob(20))),"
                           "%"PRIi64");",
                           (int64_t)unixNow,
                           (int64_t)unixNow
                           );
    if(rc) goto end1;
  }

  
  /* Set some config vars ... */
  {
    fsl_stmt st = fsl_stmt_empty;
    rc = fsl_db_prepare(db, &st,
                        "INSERT INTO repo.config (name,value,mtime) "
                        "VALUES (?,?,%"PRIi64")",
                        (int64_t)unixNow);
    if(!rc){
      fsl_stmt_bind_int64(&st, 3, unixNow);
#define DBSET_STR(KEY,VAL) \
      fsl_stmt_bind_text(&st, 1, KEY, -1, 0);    \
      fsl_stmt_bind_text(&st, 2, VAL, -1, 0); \
      fsl_stmt_step(&st); \
      fsl_stmt_reset(&st)
      DBSET_STR("content-schema",FSL_CONTENT_SCHEMA);
      DBSET_STR("aux-schema",FSL_AUX_SCHEMA);
#undef DBSET_STR

#define DBSET_INT(KEY,VAL) \
      fsl_stmt_bind_text(&st, 1, KEY, -1, 0 );    \
      fsl_stmt_bind_int32(&st, 2, VAL); \
      fsl_stmt_step(&st); \
      fsl_stmt_reset(&st)

      DBSET_INT("autosync",1);
      DBSET_INT("localauth",0);
      DBSET_INT("timeline-plaintext", 1);
      
#undef DBSET_INT
      fsl_stmt_finalize(&st);
    }
  }

  rc = fsl_repo_create_default_users(db, 0, userName);
  if(rc) goto end1;

  end1:
  if(db->error.code && !f->error.code){
    rc = fsl_cx_uplift_db_error(f, db);
  }
  if(inTrans){
    if(!rc) rc = fsl_db_transaction_end(db, 0);
    else fsl_db_transaction_end(db, 1);
    inTrans = 0;
  }
  fsl_cx_close_dbs(f);
  db = 0;
  if(rc) goto end2;

  /**
      In order for injection of the first commit to go through
      cleanly (==without any ugly kludging of f->dbMain), we
      need to now open the new db so that it gets connected
      to f properly...
   */
  rc = fsl_repo_open( f, opt->filename );
  if(rc) goto end2;
  db = fsl_cx_db_repo(f);
  assert(db);
  assert(db == f->dbMain);

  if(!userName || !*userName){
    userName = fsl_cx_user_get(f);
    if(!userName || !*userName){
      userName = "root" /* historical value */;
    }
  }

  /*
    Copy config...

    This is done in the second phase because...

    "cannot ATTACH database within transaction"

    and installing the initial schemas outside a transaction is
    horribly slow.
  */
  if( opt->configRepo && *opt->configRepo ){
    bool inTrans2 = false;
    char * inopConfig = fsl_config_inop_rhs(FSL_CONFIGSET_ALL);
    char * inopDb = inopConfig ? fsl_db_setting_inop_rhs() : NULL;
    if(!inopConfig || !inopDb){
      fsl_free(inopConfig);
      rc = FSL_RC_OOM;
      goto end2;
    }
    rc = fsl_db_attach(db, opt->configRepo, "settingSrc");
    if(rc){
      fsl_cx_uplift_db_error(f, db);
      goto end2;
    }
    rc = fsl_db_transaction_begin(db);
    if(rc){
      fsl_cx_uplift_db_error(f, db);
      goto detach;
    }
    inTrans2 = 1;
    /*
       Copy all settings from the supplied template repository.
    */
    rc = fsl_db_exec(db,
                     "INSERT OR REPLACE INTO repo.config"
                     " SELECT name,value,mtime FROM settingSrc.config"
                     "  WHERE (name IN %s OR name IN %s)"
                     "    AND name NOT GLOB 'project-*';",
                     inopConfig, inopDb);
    if(rc) goto detach;
    rc = fsl_db_exec(db,
                     "REPLACE INTO repo.reportfmt "
                     "SELECT * FROM settingSrc.reportfmt;");
    if(rc) goto detach;

    /*
       Copy the user permissions, contact information, last modified
       time, and photo for all the "system" users from the supplied
       template repository into the one being setup.  The other
       columns are not copied because they contain security
       information or other data specific to the other repository.
       The list of columns copied by this SQL statement may need to be
       revised in the future.
    */
    rc = fsl_db_exec(db, "UPDATE repo.user SET"
      "  cap = (SELECT u2.cap FROM settingSrc.user u2"
      "         WHERE u2.login = user.login),"
      "  info = (SELECT u2.info FROM settingSrc.user u2"
      "          WHERE u2.login = user.login),"
      "  mtime = (SELECT u2.mtime FROM settingSrc.user u2"
      "           WHERE u2.login = user.login),"
      "  photo = (SELECT u2.photo FROM settingSrc.user u2"
      "           WHERE u2.login = user.login)"
      " WHERE user.login IN ('anonymous','nobody','developer','reader');"
    );

    detach:
    fsl_free(inopConfig);
    fsl_free(inopDb);
    if(inTrans2){
      if(!rc) rc = fsl_db_transaction_end(db,0);
      else fsl_db_transaction_end(db,1);
    }
    fsl_db_detach(db, "settingSrc");
    if(rc) goto end2;
  }

  if(opt->commitMessage && *opt->commitMessage){
    /*
      Set up initial commit. Because of the historically empty P-card
      on the first commit, we can't create that one using the fsl_deck
      API unless we elide the P-card (not as fossil does) and insert
      an empty R-card (as fossil does). We need one of P- or R-card to
      unambiguously distinguish this MANIFEST from a CONTROL artifact.

      Reminder to self: fsl_deck has been adjusted to deal with the
      initial-checkin(-like) case in the mean time. But this code
      works, so no need to go changing it...
    */
    fsl_deck d = fsl_deck_empty;
    fsl_cx_err_reset(f);
    fsl_deck_init(f, &d, FSL_SATYPE_CHECKIN);
    rc = fsl_deck_C_set(&d, opt->commitMessage, -1);
    if(!rc) rc = fsl_deck_D_set(&d, fsl_db_julian_now(db));
    if(!rc) rc = fsl_deck_R_set(&d, FSL_MD5_INITIAL_HASH);
    if(!rc && opt->commitMessageMimetype && *opt->commitMessageMimetype){
      rc = fsl_deck_N_set(&d, opt->commitMessageMimetype, -1);
    }
    /* Reminder: setting tags in "wrong" (unsorted) order to
       test/assert that the sorting gets done automatically. */
    if(!rc) rc = fsl_deck_T_add(&d, FSL_TAGTYPE_PROPAGATING, NULL,
                                "sym-trunk", NULL);
    if(!rc) rc = fsl_deck_T_add(&d, FSL_TAGTYPE_PROPAGATING, NULL,
                                "branch", "trunk");
    if(!rc) rc =fsl_deck_U_set(&d, userName);
    if(!rc){
      rc = fsl_deck_save(&d, 0);
    }
    fsl_deck_finalize(&d);
  }
  
  end2:
  if(f == &F){
    fsl_cx_finalize(f);
    if(rc) fsl_file_unlink(opt->filename);
  }
  return rc;
}

static int fsl_repo_dir_names_rid( fsl_cx * f, fsl_id_t rid, fsl_list * tgt,
                                   bool addSlash){
  fsl_db * dbR = fsl_needs_repo(f);
  fsl_deck D = fsl_deck_empty;
  fsl_deck * d = &D;
  int rc = 0;
  fsl_stmt st = fsl_stmt_empty;
  fsl_buffer tname = fsl_buffer_empty;
  int count = 0;
  fsl_card_F const * fc;
  /*
    This is a poor-man's impl. A more efficient one would calculate
    the directory names without using the database.
  */
  assert(rid>0);
  assert(dbR);
  rc = fsl_deck_load_rid( f, d, rid, FSL_SATYPE_CHECKIN);
  if(rc){
    fsl_deck_clean(d);
    return rc;
  }
  rc = fsl_buffer_appendf(&tname,
                          "tmp_filelist_for_rid_%d",
                          (int)rid);
  if(rc) goto end;
  rc = fsl_deck_F_rewind(d);
  while( !rc && !(rc=fsl_deck_F_next(d, &fc)) && fc ){
    //if(!fc->name) continue;
    assert(fc->name && *fc->name);
    if(!st.stmt){
      rc = fsl_db_exec(dbR, "CREATE TEMP TABLE IF NOT EXISTS "
                       "%b(n TEXT UNIQUE ON CONFLICT IGNORE)",
                       &tname);
      if(!rc){
        rc = fsl_db_prepare(dbR, &st,
                            "INSERT INTO %b(n) "
                            "VALUES(fsl_dirpart(?,%d))",
                            &tname, addSlash ? 1 : 0);
      }
      if(rc) goto end;
      assert(st.stmt);
    }
    rc = fsl_stmt_bind_text(&st, 1, fc->name, -1, 0);
    if(!rc){
      rc = fsl_stmt_step(&st);
      if(FSL_RC_STEP_DONE==rc){
        ++count;
        rc = 0;
      }
    }
    fsl_stmt_reset(&st);
    fc = 0;
  }

  if(!rc && (count>0)){
    fsl_stmt_finalize(&st);
    rc = fsl_db_prepare(dbR, &st,
                        "SELECT n FROM %b WHERE n "
                        "IS NOT NULL ORDER BY n %s",
                        &tname,
                        fsl_cx_filename_collation(f));
    while( !rc && (FSL_RC_STEP_ROW==(rc=fsl_stmt_step(&st))) ){
      fsl_size_t nLen = 0;
      char const * name = fsl_stmt_g_text(&st, 0, &nLen);
      rc = 0;
      if(name){
        char * cp;
        assert(nLen);
        cp = fsl_strndup( name, (fsl_int_t)nLen );
        if(!cp){
          rc = FSL_RC_OOM;
          break;
        }
        rc = fsl_list_append(tgt, cp);
        if(rc){
          fsl_free(cp);
          break;
        }
      }
    }
    if(FSL_RC_STEP_DONE==rc) rc = 0;
  }

  end:
  if(rc && !f->error.code && dbR->error.code){
    fsl_cx_uplift_db_error(f, dbR);
  }
  fsl_stmt_finalize(&st);
  fsl_deck_clean(d);
  if(tname.used){
    fsl_db_exec(dbR, "DROP TABLE IF EXISTS %b", &tname);
  }
  fsl_buffer_clear(&tname);
  return rc;
}

int fsl_repo_dir_names( fsl_cx * f, fsl_id_t rid, fsl_list * tgt,
                        bool addSlash ){
  fsl_db * db = (f && tgt) ? fsl_needs_repo(f) : NULL;
  if(!f || !tgt) return FSL_RC_MISUSE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  else {
    int rc;
    if(rid>=0){
      if(!rid){
        /* Dir list for current checkout version */
        if(f->ckout.rid>0){
          rid = f->ckout.rid;
        }else{
          return fsl_cx_err_set(f, FSL_RC_RANGE,
                                "The rid argument is 0 (indicating "
                                "the current checkout), but there is "
                                "no opened checkout.");
        }
      }
      assert(rid>0);
      rc = fsl_repo_dir_names_rid(f, rid, tgt, addSlash);
    }else{
      /* Dir list across all versions */
      fsl_stmt s = fsl_stmt_empty;
      rc = fsl_db_prepare(db, &s,
                          "SELECT DISTINCT(fsl_dirpart(name,%d)) dname "
                          "FROM filename WHERE dname IS NOT NULL "
                          "ORDER BY dname", addSlash ? 1 : 0);
      if(rc){
        fsl_cx_uplift_db_error(f, db);
        assert(!s.stmt);
        return rc;
      }
      while( !rc && (FSL_RC_STEP_ROW==(rc=fsl_stmt_step(&s)))){
        fsl_size_t len = 0;
        char const * col = fsl_stmt_g_text(&s, 0, &len);
        char * cp = fsl_strndup( col, (fsl_int_t)len );
        if(!cp){
          rc = FSL_RC_OOM;
          break;
        }
        rc = fsl_list_append(tgt, cp);
        if(rc) fsl_free(cp);
      }
      if(FSL_RC_STEP_DONE==rc) rc = 0;
      fsl_stmt_finalize(&s);
    }
    return rc;
  }
}

/* UNTESTED */
char fsl_repo_is_readonly(fsl_cx const * f){
  if(!f || !f->dbMain) return 0;
  else{
    int const roleId = f->ckout.db.dbh ? FSL_DBROLE_MAIN : FSL_DBROLE_REPO
      /* If CKOUT is attached, it is the main DB and REPO is ATTACHed. */
      ;
    char const * zRole = fsl_db_role_label(roleId);
    assert(f->dbMain);
    return sqlite3_db_readonly(f->dbMain->dbh, zRole) ? 1 : 0;
  }
}

int fsl_repo_record_filename(fsl_cx * f){
  fsl_buffer full = fsl_buffer_empty;
  fsl_db * dbR = fsl_needs_repo(f);
  fsl_db * dbC;
  fsl_db * dbConf;
  char const * zCDir;
  char const * zName = dbR ? dbR->filename : NULL;
  int rc;
  if(!dbR) return FSL_RC_NOT_A_REPO;
  assert(zName);
  assert(f);
  rc = fsl_file_canonical_name(zName, &full, 0);
  if(rc){
    fsl_cx_err_set(f, rc, "Error %s canonicalizing filename: %s", zName);
    goto end;
  }

  /*
    If global config is open, write the repo db's name to it.
   */
  dbConf = fsl_cx_db_config(f);
  if(dbConf){
    int const dbRole = (f->dbMain==&f->config.db)
      ? FSL_DBROLE_MAIN : FSL_DBROLE_CONFIG;
    rc = fsl_db_exec(dbConf,
                     "INSERT OR IGNORE INTO %s.global_config(name,value) "
                     "VALUES('repo:%q',1)",
                     fsl_db_role_label(dbRole),
                     fsl_buffer_cstr(&full));
    if(rc) goto end;
  }

  dbC = fsl_cx_db_ckout(f);
  if(dbC && (zCDir=f->ckout.dir)){
    /* If we have a checkout, update its repo's list of checkouts... */
    /* Assumption: if we have an opened checkout, dbR is ATTACHed with
       the role REPO. */
    int ro;
    assert(dbR);
    ro = sqlite3_db_readonly(dbR->dbh,
                             fsl_db_role_label(FSL_DBROLE_REPO));
    assert(ro>=0);
    if(!ro){
      fsl_buffer localRoot = fsl_buffer_empty;
      rc = fsl_file_canonical_name(zCDir, &localRoot, 1);
      if(0==rc){
        if(dbConf){
          /*
            If global config is open, write the checkout db's name to it.
          */
          int const dbRole = (f->dbMain==&f->config.db)
            ? FSL_DBROLE_MAIN : FSL_DBROLE_CONFIG;
          rc = fsl_db_exec(dbConf,
                           "REPLACE INTO INTO %s.global_config(name,value) "
                           "VALUES('ckout:%q',1)",
                           fsl_db_role_label(dbRole),
                           fsl_buffer_cstr(&localRoot));
        }
        if(0==rc){
          /* We know that repo is ATTACHed to ckout here. */
          assert(dbR == dbC);
          rc = fsl_db_exec(dbR,
                           "REPLACE INTO %s.config(name, value, mtime) "
                           "VALUES('ckout:%q', 1, now())",
                           fsl_db_role_label(FSL_DBROLE_REPO),
                           fsl_buffer_cstr(&localRoot));
        }
      }
      fsl_buffer_clear(&localRoot);
    }
  }

  end:
  if(rc && !f->error.code && f->dbMain->error.code){
    fsl_cx_uplift_db_error(f, f->dbMain);
  }
  fsl_buffer_clear(&full);
  return rc;

}

char fsl_rid_is_a_checkin(fsl_cx * f, fsl_id_t rid){
  fsl_db * db = f ? fsl_cx_db_repo(f) : NULL;
  if(!db || (rid<0)) return 0;
  else if(0==rid){
    /* Corner case: empty repo */
    return !fsl_db_exists(db, "SELECT 1 FROM blob WHERE rid>0");
  }
  else{
    fsl_stmt * st = 0;
    char rv = 0;
    int rc = fsl_db_prepare_cached(db, &st,
                                   "SELECT 1 FROM event WHERE "
                                   "objid=? AND type='ci' "
                                   "/*%s()*/",__func__);
    if(!rc){
      rc = fsl_stmt_bind_id( st, 1, rid);
      if(!rc){
        rc = fsl_stmt_step(st);
        if(FSL_RC_STEP_ROW==rc){
          rv = 1;
        }
      }
      fsl_stmt_cached_yield(st);
    }
    if(db->error.code){
      fsl_cx_uplift_db_error(f, db);
    }
    return rv;
  }
}

int fsl_repo_extract( fsl_cx * f, fsl_repo_extract_opt const * opt_ ){
  if(!f || !opt_->callback) return FSL_RC_MISUSE;
  else if(!fsl_needs_repo(f)) return FSL_RC_NOT_A_REPO;
  else if(opt_->checkinRid<=0){
    return fsl_cx_err_set(f, FSL_RC_RANGE, "RID must be positive.");
  }else{
    int rc;
    fsl_deck mf = fsl_deck_empty;
    fsl_buffer * content = opt_->extractContent
      ? &f->fileContent
      : NULL;
    fsl_id_t fid;
    fsl_repo_extract_state xst = fsl_repo_extract_state_empty;
    fsl_card_F const * fc = NULL;
    fsl_repo_extract_opt const opt = *opt_
      /* Copy in case the caller modifies it via their callback. If we
         find an interesting use for such modification then we can
         remove this copy. */;
    assert(!content || (!content->used && "Internal misuse of fsl_cx::fileContent"));
    rc = fsl_deck_load_rid(f, &mf, opt.checkinRid, FSL_SATYPE_CHECKIN);
    if(rc) goto end;
    assert(mf.f==f);
    xst.f = f;
    xst.checkinRid = opt.checkinRid;
    xst.callbackState = opt.callbackState;
    xst.content = opt.extractContent ? content : NULL;
    /* Calculate xst.count.fileCount... */
    assert(0==xst.count.fileCount);
    if(mf.B.uuid){/*delta. The only way to count this reliably
                   is to walk though the whole card list. */
      rc = fsl_deck_F_rewind(&mf);
      while( !rc && !(rc=fsl_deck_F_next(&mf, &fc)) && fc){
        ++xst.count.fileCount;
      }
      if(rc) goto end;
      fc = NULL;
    }else{
      xst.count.fileCount = mf.F.used;
    }
    assert(0==xst.count.fileNumber);
    rc = fsl_deck_F_rewind(&mf);
    while( !rc && !(rc=fsl_deck_F_next(&mf, &fc)) && fc){
      assert(fc->uuid
             && "We shouldn't get F-card deletions via fsl_deck_F_next()");
      ++xst.count.fileNumber;
      fid = fsl_uuid_to_rid(f, fc->uuid);
      if(fid<0){
        assert(f->error.code);
        rc = f->error.code;
      }else if(!fid){
        rc = fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                            "Could not resolve RID for UUID: %s",
                            fc->uuid);
      }else if(opt.extractContent){
        fsl_buffer_reuse(content);
        rc = fsl_content_get(f, fid, content);
      }
      if(!rc){
        /** Call the callback. */
        xst.fCard = fc;
        assert(fid>0);
        xst.content = content;
        xst.fileRid = fid;
        rc = opt.callback( &xst );
        if(FSL_RC_BREAK==rc){
          rc = 0;
          break;
        }
      }
    }/* for-each-F-card loop */
    end:
    fsl_cx_content_buffer_yield(f);
    fsl_deck_finalize(&mf);
    return rc;
  }
}

int fsl_repo_import_blob( fsl_cx * f, fsl_input_f in, void * inState,
                          fsl_id_t * rid, fsl_uuid_str * uuid ){
  fsl_db * db = f ? fsl_needs_repo(f) : NULL;
  if(!f || !in) return FSL_RC_MISUSE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  else{
    int rc;
    fsl_buffer buf = fsl_buffer_empty;
    rc = fsl_buffer_fill_from(&buf, in, inState);
    if(rc){
      rc = fsl_cx_err_set(f, rc,
                          "Error filling buffer from input source.");
    }else{
      fsl_id_t theRid = 0;
      rc = fsl_content_put_ex( f, &buf, NULL, 0, 0, 0, &theRid);
      if(!rc){
        if(rid) *rid = theRid;
        if(uuid){
          *uuid = fsl_rid_to_uuid(f, theRid);
          if(!uuid) rc = FSL_RC_OOM;
        }
      }
    }
    fsl_buffer_clear(&buf);
    return rc;
  }
}

int fsl_repo_import_buffer( fsl_cx * f, fsl_buffer const * in,
                            fsl_id_t * rid, fsl_uuid_str * uuid ){
  if(!f || !in) return FSL_RC_MISUSE;
  else{
    /* Workaround: input ptr is const and input needs to modify
       (only) the cursor. So we'll cheat rather than require a non-const
       input...
    */
    fsl_buffer cursorKludge = *in;
    cursorKludge.cursor = 0;
    int const rc = fsl_repo_import_blob(f, fsl_input_f_buffer, &cursorKludge,
                                        rid, uuid );
    assert(cursorKludge.mem == in->mem);
    return rc;
  }
}


int fsl_repo_blob_lookup( fsl_cx * f, fsl_buffer const * src, fsl_id_t * ridOut,
                          fsl_uuid_str * hashOut ){
  int rc;
  fsl_buffer hash_ = fsl_buffer_empty;
  fsl_buffer * hash;
  fsl_id_t rid = 0;
  if(!fsl_cx_db_repo(f)) return FSL_RC_NOT_A_REPO;
  hash = hashOut ? &hash_ : fsl_cx_scratchpad(f);
  /* First check the auxiliary hash to see if there is already an artifact
     that uses the auxiliary hash name */
  rc = fsl_cx_hash_buffer(f, true, src, hash);
  if(FSL_RC_UNSUPPORTED==rc){
    // The auxiliary hash option is incompatible with our hash policy.
    rc = 0;
  }
  else if(rc) goto end;
  rid = hash->used ? fsl_uuid_to_rid(f, fsl_buffer_cstr(hash)) : 0;
  if(!rid){
    /* No existing artifact with the auxiliary hash name.  Therefore, use
       the primary hash name. */
    fsl_buffer_reuse(hash);
    rc = fsl_cx_hash_buffer(f, false, src, hash);
    if(rc) goto end;
    rid = fsl_uuid_to_rid(f, fsl_buffer_cstr(hash));
    if(!rid){
      rc = FSL_RC_NOT_FOUND;
    }
    if(rid<0){
      rc = f->error.code;
    }
  }
  end:
  if(!rc || rc==FSL_RC_NOT_FOUND){
    if(hashOut){
      assert(hash == &hash_);
      *hashOut = fsl_buffer_take(hash)/*transfer*/;
    }
  }
  if(!rc && ridOut){
    *ridOut = rid;
  }
  if(hash == &hash_){
    fsl_buffer_clear(hash);
  }else{
    assert(!hash_.mem);
    fsl_cx_scratchpad_yield(f, hash);
  }
  return rc;
}

int fsl_repo_fingerprint_search( fsl_cx *f, fsl_id_t rcvid, char ** zOut,
                                 bool oldVersion ){
  int rc = 0;
  fsl_db * const db = fsl_needs_repo(f);
  if(!db) return FSL_RC_NOT_A_REPO; 
  fsl_buffer * const sql = fsl_cx_scratchpad(f);
  fsl_stmt q = fsl_stmt_empty;
  /*
   * If oldVersion is set, use original fingerprint query; from Fossil db.c:
   * The original fingerprint algorithm used "quote(mtime)".  But this could
   * give slightly different answers depending on how the floating-point
   * hardware is configured.  For example, it gave different answers on
   * native Linux versus running under valgrind.
   */
  if(oldVersion){
    rc = fsl_buffer_append(sql,
                          "SELECT rcvid, quote(uid), quote(mtime), "
                          "quote(nonce), quote(ipaddr) "
                          "FROM rcvfrom ", -1);
  }else{
    rc = fsl_buffer_append(sql,
                          "SELECT rcvid, quote(uid), datetime(mtime), "
                          "quote(nonce), quote(ipaddr) "
                          "FROM rcvfrom ", -1);
  }
  if(rc) goto end;
  rc = (rcvid>0)
    ? fsl_buffer_appendf(sql, "WHERE rcvid=%" FSL_ID_T_PFMT, rcvid)
    : fsl_buffer_append(sql, "ORDER BY rcvid DESC LIMIT 1", -1);
  if(rc) goto end;
  rc = fsl_db_prepare(db, &q, "%b", sql);
  if(rc) goto end;
  rc = fsl_stmt_step(&q);
  switch(rc){
    case FSL_RC_STEP_ROW:{
      fsl_md5_cx hash = fsl_md5_cx_empty;
      fsl_size_t len = 0;
      fsl_id_t const rvid = fsl_stmt_g_id(&q, 0);
      unsigned char digest[16] = {0};
      char hex[FSL_STRLEN_MD5+1] = {0};
      for(int i = 1; i <= 4; ++i){
        char const * z = fsl_stmt_g_text(&q, i, &len);
        fsl_md5_update(&hash, z, len);
      }
      fsl_md5_final(&hash, digest);
      fsl_md5_digest_to_base16(digest, hex);
      *zOut = fsl_mprintf("%" FSL_ID_T_PFMT "/%s", rvid, hex);
      rc = *zOut ? 0 : FSL_RC_OOM;
      break;
    }
    case FSL_RC_STEP_DONE:
      rc = FSL_RC_NOT_FOUND;
      break;
    default:
      rc = fsl_cx_uplift_db_error2(f, db, rc);
      break;
  }
  end:
  fsl_cx_scratchpad_yield(f, sql);
  fsl_stmt_finalize(&q);
  return rc;
}


/**
   NOT YET IMPLEMENTED. (We have the infrastructure, just need to glue
   it together.)

   Re-crosslinks all artifacts of the given type (or all artifacts if
   the 2nd argument is FSL_SATYPE_ANY). This is an expensive
   operation, involving dropping the contents of any corresponding
   auxiliary tables, loading and parsing the appropriate artifacts,
   and re-creating the auxiliary tables.

   TODO: add a way for callers to get some sort of progress feedback
   and abort the process by returning non-0 from that handler. We can
   possibly do that via defining an internal-use crosslink listener
   which carries more state, e.g. for calculating completion progress.
*/
//FSL_EXPORT int fsl_repo_relink_artifacts(fsl_cx *f, void * someOptionsType);


#undef MARKER
