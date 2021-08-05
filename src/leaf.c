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

/************************************************************************
  This file houses some of the "leaf"-related APIs.
*/
#include <assert.h>

#include "fossil-scm/fossil-internal.h"

/* Only for debugging */
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)

int fsl_repo_leaves_rebuild(fsl_cx * f){
  fsl_db * db = f ? fsl_cx_db_repo(f) : NULL;
  return !db ? FSL_RC_MISUSE : fsl_db_exec_multi(db,
    "DELETE FROM leaf;"
    "INSERT OR IGNORE INTO leaf"
    "  SELECT cid FROM plink"
    "  EXCEPT"
    "  SELECT pid FROM plink"
    "   WHERE coalesce((SELECT value FROM tagxref"
                       " WHERE tagid=%d AND rid=plink.pid),'trunk')"
         " == coalesce((SELECT value FROM tagxref"
                       " WHERE tagid=%d AND rid=plink.cid),'trunk')",
    FSL_TAGID_BRANCH, FSL_TAGID_BRANCH
  );
}

fsl_int_t fsl_count_nonbranch_children(fsl_cx * f, fsl_id_t rid){
  int32_t rv = 0;
  int rc;
  fsl_db * db = f ? fsl_cx_db_repo(f) : NULL;
  if(!db || !db->dbh || (rid<=0)) return -1;
  rc = fsl_db_get_int32(db, &rv,
                        "SELECT count(*) FROM plink "
                        "WHERE pid=%"FSL_ID_T_PFMT" "
                        "AND isprim "
                        "AND coalesce((SELECT value FROM tagxref "
                        "WHERE tagid=%d AND rid=plink.pid), 'trunk')"
                        "=coalesce((SELECT value FROM tagxref "
                        "WHERE tagid=%d AND rid=plink.cid), 'trunk')",
                        rid, FSL_TAGID_BRANCH, FSL_TAGID_BRANCH);
  return rc ? -2 : rv;
}

bool fsl_rid_is_leaf(fsl_cx * f, fsl_id_t rid){
  int rv = -1;
  int rc;
  fsl_db * db = f ? fsl_cx_db_repo(f) : NULL;
  fsl_stmt * st = NULL;
  if(!db || !db->dbh || (rid<=0)) return 0;
  rc = fsl_db_prepare_cached(db, &st,
       "SELECT 1 FROM plink "
       "WHERE pid=?1 "
       "AND coalesce("
           "(SELECT value FROM tagxref "
            "WHERE tagid=%d AND rid=?1), "
          //"(SELECT value FROM config WHERE name='main-branch'), "
             "'trunk')"
           "=coalesce((SELECT value FROM tagxref "
             "WHERE tagid=%d "
             "AND rid=plink.cid), "
           //"(SELECT value FROM config WHERE name='main-branch'), "
             "'trunk')"
       "/*%s()*/",
       FSL_TAGID_BRANCH, FSL_TAGID_BRANCH, __func__);
  if(!rc){
    rc = fsl_stmt_bind_step(st, "R", rid);
    switch(rc){
      case FSL_RC_STEP_ROW:
        rv = 0;
        rc = 0;
        break;
      case 0:
        rv = 1;
        rc = 0;
        break;
      default:
        break;
    }
    fsl_stmt_cached_yield(st);
    assert(0==rv || 1==rv);
  }
  return rc ? 0 : (rv==1);
}

int fsl_repo_leaf_check(fsl_cx * f, fsl_id_t rid){
  fsl_db * const db = f ? fsl_cx_db_repo(f) : NULL;
  if(!db || !db->dbh) return FSL_RC_MISUSE;
  else if(rid<=0) return FSL_RC_RANGE;
  else {
    int rc = 0;
    bool isLeaf;
    fsl_cx_err_reset(f);
    isLeaf = fsl_rid_is_leaf(f, rid);
    rc = fsl_cx_err_get(f, NULL, NULL);
    if(!rc){
      fsl_stmt * st = NULL;
      if( isLeaf ){
        rc = fsl_db_prepare_cached(db, &st,
                                   "INSERT OR IGNORE INTO leaf VALUES"
                                   "(?) /*%s()*/",__func__);
      }else{
        rc = fsl_db_prepare_cached(db, &st,
                                   "DELETE FROM leaf WHERE rid=?"
                                   "/*%s()*/",__func__);
      }
      if(!rc && st){
        rc = fsl_stmt_bind_step(st, "R", rid);
        fsl_stmt_cached_yield(st);
        if(rc) rc = fsl_cx_uplift_db_error2(f, db, rc);
      }
    }
    return rc;
  }
}

int fsl_repo_leaf_eventually_check( fsl_cx * f, fsl_id_t rid){
  fsl_db * db = f ? fsl_cx_db_repo(f) : NULL;
  if(!f) return FSL_RC_MISUSE;
  else if(rid<=0) return FSL_RC_RANGE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  else {
    fsl_stmt * parentsOf = NULL;
    int rc = fsl_db_prepare_cached(db, &parentsOf,
                            "SELECT pid FROM plink WHERE "
                            "cid=? AND pid>0"
                            "/*%s()*/",__func__);
    if(rc) return rc;
    rc = fsl_stmt_bind_id(parentsOf, 1, rid);
    if(!rc){
      rc = fsl_id_bag_insert(&f->cache.leafCheck, rid);
      while( !rc && (FSL_RC_STEP_ROW==fsl_stmt_step(parentsOf)) ){
        rc = fsl_id_bag_insert(&f->cache.leafCheck,
                               fsl_stmt_g_id(parentsOf, 0));
      }
    }
    fsl_stmt_cached_yield(parentsOf);
    return rc;
  }
}


int fsl_repo_leaf_do_pending_checks(fsl_cx *f){
  fsl_id_t rid;
  int rc = 0;
  for(rid=fsl_id_bag_first(&f->cache.leafCheck);
      !rc && rid; rid=fsl_id_bag_next(&f->cache.leafCheck,rid)){
    rc = fsl_repo_leaf_check(f, rid);
  }
  fsl_id_bag_clear(&f->cache.leafCheck);
  return rc;
}

int fsl_leaves_compute(fsl_cx * f, fsl_id_t vid,
                       fsl_leaves_compute_e closeMode){
  fsl_db * const db = fsl_needs_repo(f);
  if(!db) return FSL_RC_NOT_A_REPO;
  int rc = 0;

  /* Create the LEAVES table if it does not already exist.  Make sure
  ** it is empty.
  */
  rc = fsl_db_exec_multi(db,
    "CREATE TEMP TABLE IF NOT EXISTS leaves("
    "  rid INTEGER PRIMARY KEY"
    ");"
    "DELETE FROM leaves;"
  );
  if(rc) goto dberr;
  if( vid <= 0 ){
    rc = fsl_db_exec_multi(db,
      "INSERT INTO leaves SELECT leaf.rid FROM leaf"
    );
    if(rc) goto dberr;
  }
  if( vid>0 ){
    fsl_id_bag seen = fsl_id_bag_empty;     /* Descendants seen */
    fsl_id_bag pending = fsl_id_bag_empty;  /* Unpropagated descendants */
    fsl_stmt q1 = fsl_stmt_empty;      /* Query to find children of a check-in */
    fsl_stmt isBr = fsl_stmt_empty;    /* Query to check to see if a check-in starts a new branch */
    fsl_stmt ins = fsl_stmt_empty;     /* INSERT statement for a new record */

    /* Initialize the bags. */
    rc = fsl_id_bag_insert(&pending, vid);
    if(rc) goto cleanup;

    /* This query returns all non-branch-merge children of check-in :rid.
    **
    ** If a child is a merge of a fork within the same branch, it is
    ** returned.  Only merge children in different branches are excluded.
    */
    rc = fsl_db_prepare(db, &q1,
      "SELECT cid FROM plink"
      " WHERE pid=?1"
      "   AND (isprim"
      "        OR coalesce((SELECT value FROM tagxref"
                        "   WHERE tagid=%d AND rid=plink.pid), 'trunk')"
                        /* FIXME? main-branch? */
                 "=coalesce((SELECT value FROM tagxref"
                        "   WHERE tagid=%d AND rid=plink.cid), 'trunk'))"
                          /* FIXME? main-branch? */
                        ,
      FSL_TAGID_BRANCH, FSL_TAGID_BRANCH
    );
    if(rc) goto cleanup;
    /* This query returns a single row if check-in :rid is the first
    ** check-in of a new branch.
    */
    rc = fsl_db_prepare(db, &isBr,
       "SELECT 1 FROM tagxref"
       " WHERE rid=?1 AND tagid=%d AND tagtype=2"
       "   AND srcid>0",
       FSL_TAGID_BRANCH
    );
    if(rc) goto cleanup;

    /* This statement inserts check-in :rid into the LEAVES table.
    */
    rc = fsl_db_prepare(db, &ins,
                        "INSERT OR IGNORE INTO leaves VALUES(?1)");
    if(rc) goto cleanup;

    while( fsl_id_bag_count(&pending) ){
      fsl_id_t const rid = fsl_id_bag_first(&pending);
      unsigned cnt = 0;
      fsl_id_bag_remove(&pending, rid);
      fsl_stmt_bind_id(&q1, 1, rid);
      while( FSL_RC_STEP_ROW==(rc = fsl_stmt_step(&q1)) ){
        int const cid = fsl_stmt_g_id(&q1, 0);
        rc = fsl_id_bag_insert(&seen, cid);
        if(rc) break;
        rc = fsl_id_bag_insert(&pending, cid);
        if(rc) break;
        fsl_stmt_bind_id(&isBr, 1, cid);
        if( FSL_RC_STEP_DONE==fsl_stmt_step(&isBr) ){
          ++cnt;
        }
        fsl_stmt_reset(&isBr);
      }
      if(FSL_RC_STEP_DONE==rc) rc = 0;
      else if(rc) break;
      fsl_stmt_reset(&q1);
      if( cnt==0 && !fsl_rid_is_leaf(f, rid) ){
        ++cnt;
      }
      if( cnt==0 ){
        fsl_stmt_bind_id(&ins, 1, rid);
        rc = fsl_stmt_step(&ins);
        if(FSL_RC_STEP_DONE!=rc) break;
        rc = 0;
        fsl_stmt_reset(&ins);
      }
    }
    cleanup:
    fsl_stmt_finalize(&ins);
    fsl_stmt_finalize(&isBr);
    fsl_stmt_finalize(&q1);
    fsl_id_bag_clear(&pending);
    fsl_id_bag_clear(&seen);
    if(rc) goto dberr;
  }
  assert(!rc);
  switch(closeMode){
    case FSL_LEAVES_COMPUTE_OPEN:
      rc =
        fsl_db_exec_multi(db,
                          "DELETE FROM leaves WHERE rid IN"
                          "  (SELECT leaves.rid FROM leaves, tagxref"
                          "    WHERE tagxref.rid=leaves.rid "
                          "      AND tagxref.tagid=%d"
                          "      AND tagxref.tagtype>0)",
                          FSL_TAGID_CLOSED);
      if(rc) goto dberr;
      break;
    case FSL_LEAVES_COMPUTE_CLOSED:
      rc = 
        fsl_db_exec_multi(db,
                          "DELETE FROM leaves WHERE rid NOT IN"
                          "  (SELECT leaves.rid FROM leaves, tagxref"
                          "    WHERE tagxref.rid=leaves.rid "
                          "      AND tagxref.tagid=%d"
                          "      AND tagxref.tagtype>0)",
                          FSL_TAGID_CLOSED);
      if(rc) goto dberr;
      break;
    default: break;
  }

  end:
  return rc;
  dberr:
  assert(rc);
  rc = fsl_cx_uplift_db_error2(f, db, rc);
  goto end;
}

bool fsl_leaves_computed_has(fsl_cx * f){
  return fsl_db_exists(fsl_cx_db_repo(f),
                       "SELECT 1 FROM leaves");
}

fsl_int_t fsl_leaves_computed_count(fsl_cx * f){
  int32_t rv = -1;
  fsl_db * const db = fsl_cx_db_repo(f);
  int const rc = fsl_db_get_int32(db, &rv,
                                 "SELECT COUNT(*) FROM leaves");
  if(rc){
    fsl_cx_uplift_db_error2(f, db, rc);
    assert(-1==rv);
  }else{
    assert(rv>=0);
  }
  return rv;
}

fsl_id_t fsl_leaves_computed_latest(fsl_cx * f){
  fsl_id_t rv = 0;
  fsl_db * const db = fsl_cx_db_repo(f);
  int const rc =
    fsl_db_get_id(db, &rv,
                    "SELECT rid FROM leaves, event"
                    " WHERE event.objid=leaves.rid"
                    " ORDER BY event.mtime DESC");
  if(rc){
    fsl_cx_uplift_db_error2(f, db, rc);
    assert(!rv);
  }else{
    assert(rv>=0);
  }
  return rv;
}

void fsl_leaves_computed_cleanup(fsl_cx * f){
  fsl_db_exec(fsl_cx_db_repo(f), "DROP TABLE IF EXISTS leaves");
}

#undef MARKER
