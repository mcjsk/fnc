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
  This file houses the code for checkin-level APIS.
*/
#include <assert.h>

#include "fossil-scm/fossil-internal.h"
#include "fossil-scm/fossil-checkout.h"
#include "fossil-scm/fossil-confdb.h"

/* Only for debugging */
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)


/**
   Expects f to have an opened checkout. Assumes zRelName is a
   checkout-relative simple path. It loads the file's contents and
   stores them into the blob table. If rid is not NULL, *rid is
   assigned the blob.rid (possibly new, possilbly re-used!). If uuid
   is not NULL then *uuid is assigned to the content's UUID. The *uuid
   bytes are owned by the caller, who must eventually fsl_free()
   them. If content with the same UUID already exists, it does not get
   re-imported but rid/uuid will (if not NULL) contain the old values.

   If parentRid is >0 then it must refer to the previous version of
   zRelName's content. The parent version gets deltified vs the new
   one. Note that deltification is a suggestion which the library will
   ignore if (e.g.) the parent content is already a delta of something
   else.

   The wise caller will have a transaction in place when calling this.

   Returns 0 on success. On error rid and uuid are not modified.
*/
static int fsl_checkin_import_file( fsl_cx * f, char const * zRelName,
                                    fsl_id_t parentRid,
                                    bool allowMergeConflict,
                                    fsl_id_t *rid, fsl_uuid_str * uuid){
  fsl_buffer * nbuf = fsl_cx_scratchpad(f);
  fsl_size_t const oldSize = nbuf->used;
  fsl_buffer * fbuf = &f->fileContent;
  char const * fn;
  int rc;
  fsl_id_t fnid = 0;
  fsl_id_t rcRid = 0;
  assert(!fbuf->used && "Misuse of f->fileContent");
  assert(f->ckout.dir);
  rc = fsl_repo_filename_fnid2(f, zRelName, &fnid, 1);
  if(rc) goto end;
  assert(fnid>0);

  rc = fsl_buffer_appendf(nbuf, "%s%s", f->ckout.dir, zRelName);
  nbuf->used = oldSize;
  if(rc) goto end;
  fn = fsl_buffer_cstr(nbuf) + oldSize;
  rc = fsl_buffer_fill_from_filename( fbuf, fn );
  if(rc){
    fsl_cx_err_set(f, rc, "Error %s importing file: %s",
                   fsl_rc_cstr(rc), fn);
    goto end;
  }else if(!allowMergeConflict &&
           fsl_buffer_contains_merge_marker(fbuf)){
    rc = fsl_cx_err_set(f, FSL_RC_CONFLICT,
                        "File contains a merge conflict marker: %s",
                        zRelName);
    goto end;
  }

  rc = fsl_content_put( f, fbuf, &rcRid );
  if(!rc){
    assert(rcRid > 0);
    if(parentRid>0){
      rc = fsl_content_deltify(f, parentRid, rcRid, 0);
    }
    if(!rc){
      if(rid) *rid = rcRid;
      if(uuid){
        *uuid = fsl_rid_to_uuid(f, rcRid);
        if(!*uuid) rc = (f->error.code ? f->error.code : FSL_RC_OOM);
      }
    }
  }
  end:
  fsl_cx_scratchpad_yield(f, nbuf);
  fsl_cx_content_buffer_yield(f);
  assert(0==fbuf->used);
  return rc;
}

int fsl_filename_to_vfile_ids( fsl_cx * f, fsl_id_t vid,
                               fsl_id_bag * dest, char const * zName,
                               bool changedOnly){
  fsl_stmt st = fsl_stmt_empty;
  fsl_db * const db = fsl_needs_ckout(f);
  int rc;
  fsl_buffer * sql = 0;
  if(!db) return FSL_RC_NOT_A_CKOUT;
  sql = fsl_cx_scratchpad(f);
  if(0>=vid) vid = f->ckout.rid;
  if(zName && *zName
     && !('.'==*zName && !zName[1])){
    rc = fsl_buffer_appendf(sql,
                            "SELECT id FROM vfile WHERE vid=%"
                            FSL_ID_T_PFMT
                            " AND fsl_match_vfile_or_dir(pathname,%Q)",
                            vid, zName);
  }else{
    rc = fsl_buffer_appendf(sql,
                            "SELECT id FROM vfile WHERE vid=%" FSL_ID_T_PFMT,
                            vid);
  }
  if(rc) goto end;
  else if(changedOnly){
    rc = fsl_buffer_append(sql, " AND (chnged OR deleted OR rid=0 "
                           "OR (origname IS NOT NULL AND "
                           "    origname<>pathname))", -1);
    if(rc) goto end;
  }
  rc = fsl_buffer_appendf(sql, " /* %s() */", __func__);
  if(rc) goto end;
  rc = fsl_db_prepare(db, &st, "%b", sql);
  while(!rc && (FSL_RC_STEP_ROW == (rc=fsl_stmt_step(&st)))){
    rc = fsl_id_bag_insert( dest, fsl_stmt_g_id(&st, 0) );
  }
  if(FSL_RC_STEP_DONE==rc) rc = 0;
  end:
  fsl_cx_scratchpad_yield(f, sql);
  fsl_stmt_finalize(&st);
  if(rc && !f->error.code && db->error.code){
    fsl_cx_uplift_db_error(f, db);
  }
  return rc;
}

int fsl_filename_to_vfile_id( fsl_cx * f, fsl_id_t vid, char const * zName, fsl_id_t * vfid ){
  fsl_db * db = fsl_needs_ckout(f);
  int rc;
  fsl_stmt st = fsl_stmt_empty;
  assert(db);
  if(!db) return FSL_RC_NOT_A_CKOUT;
  else if(!zName || !fsl_is_simple_pathname(zName, true)){
    return fsl_cx_err_set(f, FSL_RC_RANGE,
                          "Filename is not a \"simple\" path: %s",
                          zName);
  }
  if(0>=vid) vid = f->ckout.rid;
  rc = fsl_db_prepare(db, &st,
                      "SELECT id FROM vfile WHERE vid=%" FSL_ID_T_PFMT
                      " AND pathname=%Q %s /*%s()*/",
                      vid, zName, 
                      fsl_cx_filename_collation(f),
                      __func__);
  if(!rc){
    rc = fsl_stmt_step(&st);
    switch(rc){
      case FSL_RC_STEP_ROW:
        rc = 0;
        *vfid = fsl_stmt_g_id(&st, 0);
        break;
      case FSL_RC_STEP_DONE:
        rc = 0;
        /* fall through */
      default:
        *vfid = 0;
    }
    fsl_stmt_finalize(&st);
  }
  if(rc){
    rc = fsl_cx_uplift_db_error2(f, db, rc);
  }
  return rc;
}

/**
   Internal helper for fsl_checkin_enqueue() and
   fsl_checkin_dequeue(). Prepares, if needed, st with a query to
   fetch a vfile entry where vfile.id=vfid, then passes that name on
   to opt->callback(). Returns 0 on success.
*/
static int fsl_xqueue_callback(fsl_cx * f, fsl_db * db, fsl_stmt * st,
                               fsl_id_t vfid,
                               fsl_checkin_queue_opt const * opt){

  int rc;
  assert(opt->callback);
  if(!st->stmt){
    rc = fsl_db_prepare(db, st,
                        "SELECT pathname FROM vfile "
                        "WHERE id=?1");
    if(rc) return fsl_cx_uplift_db_error2(f, db, rc);
  }
  fsl_stmt_bind_id(st, 1, vfid);
  rc = fsl_stmt_step(st);
  switch(rc){
    case FSL_RC_STEP_ROW:{
      char const * zName = fsl_stmt_g_text(st, 0, NULL);
      rc = opt->callback(zName, opt->callbackState);
      break;
    }
    case FSL_RC_STEP_DONE:
      rc = fsl_cx_err_set(f, rc, "Very unexpectedly did not find "
                          "vfile.id which we just found.");
      break;
    default:
      rc = fsl_cx_uplift_db_error2(f, db, rc);
      break;
  }
  fsl_stmt_reset(st);
  return rc;
}

int fsl_checkin_enqueue(fsl_cx * f, fsl_checkin_queue_opt const * opt){
  fsl_db * const db = fsl_needs_ckout(f);
  if(!db) return FSL_RC_NOT_A_CKOUT;
  fsl_buffer * const canon = opt->vfileIds ? 0 : fsl_cx_scratchpad(f);
  fsl_stmt qName = fsl_stmt_empty;
  fsl_id_bag _vfileIds = fsl_id_bag_empty;
  fsl_id_bag const * const vfileIds =
    opt->vfileIds ? opt->vfileIds : &_vfileIds;
  int rc = fsl_db_transaction_begin(db);
  if(rc) return fsl_cx_uplift_db_error2(f, db, rc);
  if(opt->vfileIds){
    if(!fsl_id_bag_count(opt->vfileIds)){
      rc = fsl_cx_err_set(f, FSL_RC_MISUSE,
                          "fsl_checkin_queue_opt::vfileIds "
                          "may not be empty.");
      goto end;
    }
  }else{
    rc = fsl_ckout_filename_check(f, opt->relativeToCwd,
                                  opt->filename, canon);
    if(rc) goto end;
    fsl_buffer_strip_slashes(canon);
  }
  if(opt->scanForChanges){
    rc = fsl_vfile_changes_scan(f, -1, 0);
    if(rc) goto end;
  }
  if(opt->vfileIds){
    assert(vfileIds == opt->vfileIds);
  }else{
    assert(vfileIds == &_vfileIds);
    rc = fsl_filename_to_vfile_ids(f, 0, &_vfileIds,
                                   fsl_buffer_cstr(canon),
                                   opt->onlyModifiedFiles);
  }
  if(rc) goto end;
  /* Walk through each found ID and queue up any which are not already
     enqueued. */
  for(fsl_id_t vfid = fsl_id_bag_first(vfileIds);
      !rc && vfid; vfid = fsl_id_bag_next(vfileIds, vfid)){
    fsl_size_t const entryCount = f->ckin.selectedIds.entryCount;
    rc = fsl_id_bag_insert(&f->ckin.selectedIds, vfid);
    if(!rc
       && entryCount < f->ckin.selectedIds.entryCount
       /* Was enqueued */
       && opt->callback){
      rc = fsl_xqueue_callback(f, db, &qName, vfid, opt);
    }
  }
  end:
  if(opt->vfileIds){
    assert(!canon);
    assert(!_vfileIds.list);
  }else{
    assert(canon);
    fsl_cx_scratchpad_yield(f, canon);
    fsl_id_bag_clear(&_vfileIds);
  }
  fsl_stmt_finalize(&qName);
  if(rc) fsl_db_transaction_rollback(db);
  else{
    rc = fsl_cx_uplift_db_error2(f, db, fsl_db_transaction_commit(db));
  }
  return rc;
}

int fsl_checkin_dequeue(fsl_cx * f, fsl_checkin_queue_opt const * opt){
  fsl_db * const db = fsl_needs_ckout(f);
  if(!db) return FSL_RC_NOT_A_CKOUT;
  int rc = fsl_db_transaction_begin(db);
  if(rc) return fsl_cx_uplift_db_error2(f, db, rc);
  fsl_id_bag list = fsl_id_bag_empty;
  fsl_buffer * canon = 0;
  char const * fn;
  fsl_stmt qName = fsl_stmt_empty;
  if(opt->filename && *opt->filename){
    canon = fsl_cx_scratchpad(f);
    rc = fsl_ckout_filename_check(f, opt->relativeToCwd,
                                  opt->filename, canon);
    if(rc) goto end;
    else fsl_buffer_strip_slashes(canon);
  }
  fn = canon ? fsl_buffer_cstr(canon) : opt->filename;
  rc = fsl_filename_to_vfile_ids(f, 0, &list, fn, false);
  if(!rc && list.entryCount){
    /* Walk through each found ID and dequeue up any which are
       enqueued. */
    for( fsl_id_t nid = fsl_id_bag_first(&list);
         !rc && nid;
         nid = fsl_id_bag_next(&list, nid)){
      if(fsl_id_bag_remove(&f->ckin.selectedIds, nid)
         && opt->callback){
        rc = fsl_xqueue_callback(f, db, &qName, nid, opt);
      }
    }
  }
  end:
  if(canon) fsl_cx_scratchpad_yield(f, canon);
  fsl_stmt_finalize(&qName);
  fsl_id_bag_clear(&list);
  if(rc) fsl_db_transaction_rollback(db);
  else{
    rc = fsl_cx_uplift_db_error2(f, db, fsl_db_transaction_commit(db));
  }
  return rc;
}

bool fsl_checkin_is_enqueued(fsl_cx * f, char const * zName,
                             bool relativeToCwd){
  fsl_db * db;
  if(!f || !zName || !*zName) return 0;
  else if(!(db = fsl_needs_ckout(f))) return 0;
  else if(!f->ckin.selectedIds.entryCount){
    /* Behave like fsl_is_enqueued() SQL function. */
    return true;
  }
  else {
    bool rv = false;
    fsl_buffer * const canon = fsl_cx_scratchpad(f);
    int rc = fsl_ckout_filename_check(f, relativeToCwd, zName, canon);
    if(!rc){
      fsl_id_t vfid = 0;
      rc = fsl_filename_to_vfile_id(f, 0, fsl_buffer_cstr(canon),
                                    &vfid);
      rv = (rc && (vfid>0))
        ? false
        : ((vfid>0)
           ? fsl_id_bag_contains(&f->ckin.selectedIds, vfid)
           /* ^^^^ asserts that arg2!=0*/
           : false);
    }
    fsl_cx_scratchpad_yield(f, canon);
    return rv;
  }
}


void fsl_checkin_discard(fsl_cx * f){
  if(f){
    fsl_id_bag_clear(&f->ckin.selectedIds);
    fsl_deck_finalize(&f->ckin.mf);
  }
}

/**
   Adds the given rid to the "unsent" db list, Returns 0 on success,
   updates f's error state on error.
*/
static int fsl_checkin_add_unsent(fsl_cx * f, fsl_id_t rid){
  fsl_db * const r = fsl_cx_db_repo(f);
  int rc;
  assert(r);
  rc = fsl_db_exec(r,"INSERT OR IGNORE INTO unsent "
                   "VALUES(%" FSL_ID_T_PFMT ")", rid);
  if(rc){
    fsl_cx_uplift_db_error(f, r);
  }
  return rc;
}

/**
   Calculates the F-cards for deck d based on the commit file
   selection list and the contents of the vfile table (where vid==the
   vid parameter). vid is the version to check against, and this code
   assumes that the vfile table has been populated with that version
   and its state represents a recent scan (with no filesystem-level
   changes made since the scan).

   If pBaseline is not NULL then d is calculated as being a delta
   from pBaseline, but d->B is not modified by this routine.

   On success, d->F.list will contain "all the F-cards it needs."

   If changeCount is not NULL, then on success it is set to the number
   of F-cards added to d due to changes queued via the checkin process
   (as opposed to those added solely for delta inheritance reasons).
*/
static
int fsl_checkin_calc_F_cards2( fsl_cx * f, fsl_deck * d,
                               fsl_deck * pBaseline, fsl_id_t vid,
                               fsl_size_t * changeCount,
                               fsl_checkin_opt const * ciOpt){
  int rc = 0;
  fsl_db * dbR = fsl_needs_repo(f);
  fsl_db * dbC = fsl_needs_ckout(f);
  fsl_stmt stUpdateFileRid = fsl_stmt_empty;
  fsl_stmt stmt = fsl_stmt_empty;
  fsl_stmt * q = &stmt;
  char * fUuid = NULL;
  fsl_card_F const * pFile = NULL;
  fsl_size_t changeCounter = 0;
  if(!f) return FSL_RC_MISUSE;
  else if(!dbR) return FSL_RC_NOT_A_REPO;
  else if(!dbC) return FSL_RC_NOT_A_CKOUT;
  assert( (!pBaseline || !pBaseline->B.uuid) && "Baselines must not have a baseline." );
  assert( d->B.baseline ? (!pBaseline || pBaseline==d->B.baseline) : 1 );
  assert(vid>=0);
#define RC if(rc) goto end

  if(pBaseline){
    assert(!d->B.baseline);
    assert(0!=vid);
    rc = fsl_deck_F_rewind(pBaseline);
    RC;
    fsl_deck_F_next( pBaseline, &pFile );
  }

  rc = fsl_db_prepare(dbC, &stUpdateFileRid,
                      "UPDATE vfile SET mrid=?1, rid=?1, "
                      "mhash=NULL WHERE id=?2");
  RC;

  rc = fsl_db_prepare( dbC, q,
                       "SELECT "
                       /*0*/"fsl_is_enqueued(vf.id) as isSel, "
                       /*1*/"vf.id,"
                       /*2*/"vf.vid,"
                       /*3*/"vf.chnged,"
                       /*4*/"vf.deleted,"
                       /*5*/"vf.isexe,"
                       /*6*/"vf.islink,"
                       /*7*/"vf.rid,"
                       /*8*/"mrid,"
                       /*9*/"pathname,"
                       /*10*/"origname, "
                       /*11*/"b.rid, "
                       /*12*/"b.uuid "
                       "FROM vfile vf LEFT JOIN blob b ON vf.mrid=b.rid "
                       "WHERE"
                       " vf.vid=%"FSL_ID_T_PFMT" AND"
#if 0
                       /* Historical (fossil(1)). This introduces an interesting
                          corner case which i would like to avoid here because
                          it causes a "no files changed" error in the checkin
                          op. The behaviour is actually correct (and the deletion
                          is picked up) but fsl_checkin_commit() has no mechanism
                          for catching this particular case. So we'll try a
                          slightly different approach...
                       */
                       " (NOT deleted OR NOT isSel)"
#else
                       " ((NOT deleted OR NOT isSel)"
                       "  OR (deleted AND isSel))" /* workaround to allow
                                                     us to count deletions via
                                                     changeCounter. */
#endif
                       " ORDER BY fsl_if_enqueued(vf.id, pathname, origname)",
                       (fsl_id_t)vid);
  RC;
  /* MARKER(("SQL:\n%s\n", (char const *)q->sql.mem)); */
  while( FSL_RC_STEP_ROW==fsl_stmt_step(q) ){
    int const isSel = fsl_stmt_g_int32(q,0);
    fsl_id_t const id = fsl_stmt_g_id(q,1);
#if 0
    fsl_id_t const vid = fsl_stmt_g_id(q,2);
#endif
    int const changed = fsl_stmt_g_int32(q,3);
    int const deleted = fsl_stmt_g_int32(q,4);
    int const isExe = fsl_stmt_g_int32(q,5);
    int const isLink = fsl_stmt_g_int32(q,6);
    fsl_id_t const rid = fsl_stmt_g_id(q,7);
    fsl_id_t const mergeRid = fsl_stmt_g_id(q,8);
    char const * zName = fsl_stmt_g_text(q, 9, NULL);
    char const * zOrig = fsl_stmt_g_text(q, 10, NULL);
    fsl_id_t const frid = fsl_stmt_g_id(q,11);
    char const * zUuid = fsl_stmt_g_text(q, 12, NULL);
    fsl_fileperm_e perm = FSL_FILE_PERM_REGULAR;
    int cmp;
    fsl_id_t fileBlobRid = rid;
    int const renamed = (zOrig && *zOrig) ? fsl_strcmp(zName,zOrig) : 0
      /* For some as-yet-unknown reason, some fossil(1) code
         sets (origname=pathname WHERE origname=NULL). e.g.
         the 'mv' command does that.
      */;
    if(zOrig && !renamed) zOrig = NULL;
    fUuid = NULL;
    if(!isSel && !zUuid){
      assert(!rid);
      assert(!mergeRid);
      /* An unselected ADDed file. Skip it. */
      continue;
    }

    if(isExe) perm = FSL_FILE_PERM_EXE;
    else if(isLink){
      fsl_fatal(FSL_RC_NYI, "This code does not yet deal "
                "with symlinks. file: %s", zName)
        /* does not return */;
      perm = FSL_FILE_PERM_LINK;
    }
    /*
      TODO: symlinks
    */

    if(!f->cache.markPrivate){
      rc = fsl_content_make_public(f, frid);
      if(rc) break;
    }

#if 0
    if(mergeRid && (mergeRid != rid)){
      fsl_fatal(FSL_RC_NYI, "This code does not yet deal "
                "with merges. file: %s", zName)
        /* does not return */;
    }
#endif
    while(pFile && fsl_strcmp(pFile->name, zName)<0){
      /* Baseline has files with lexically smaller names.
         Interesting corner case:

         f-rm th1ish/makefile.gnu
         f-checkin ... th1ish/makefile.gnu

         makefile.gnu does not get picked up by the historical query
         but gets picked up here. We really need to ++changeCounter in
         that case, but we don't know we're in that case because we're
         now traversing a filename which is not in the result set.
         The end result (because we don't increment changeCounter) is
         that fsl_checkin_commit() thinks we have no made any changes
         and errors out. If we ++changeCounter for all deletions we
         have a different corner case, where a no-change commit is not
         seen as such because we've counted deletions from (other)
         versions between the baseline and the checkout.
      */
      rc = fsl_deck_F_add(d, pFile->name, NULL, pFile->perm, NULL);
      if(rc) break;
      fsl_deck_F_next(pBaseline, &pFile);
    }
    if(rc) goto end;
    else if(isSel && (changed || deleted || renamed)){
      /* MARKER(("isSel && (changed||deleted||renamed): %s\n", zName)); */
      ++changeCounter;
      if(deleted){
        zOrig = NULL;
      }else if(changed){
        rc = fsl_checkin_import_file(f, zName, rid,
                                     ciOpt->allowMergeConflict,
                                     &fileBlobRid, &fUuid);
        if(!rc) rc = fsl_checkin_add_unsent(f, fileBlobRid);
        RC;
        /* MARKER(("New content: %d / %s / %s\n", (int)fileBlobRid, fUuid, zName)); */
        if(0 != fsl_uuidcmp(zUuid, fUuid)){
          zUuid = fUuid;
        }
        fsl_stmt_reset(&stUpdateFileRid);
        fsl_stmt_bind_id(&stUpdateFileRid, 1, fileBlobRid);
        fsl_stmt_bind_id(&stUpdateFileRid, 2, id);
        if(FSL_RC_STEP_DONE!=fsl_stmt_step(&stUpdateFileRid)){
          rc = fsl_cx_uplift_db_error(f, stUpdateFileRid.db);
          assert(rc);
          goto end;
        }
      }else{
        assert(renamed);
        assert(zOrig);
      }
    }
    assert(!rc);
    cmp = 1;
    if(!pFile
       || (cmp = fsl_strcmp(pFile->name,zName))!=0
       /* ^^^^ the cmp assignment must come right after (!pFile)! */
       || deleted
       || (perm != pFile->perm)/* permissions change */
       || fsl_strcmp(pFile->uuid, zUuid)!=0
       /* ^^^^^ file changed somewhere between baseline and delta */
       ){
      if(isSel && deleted){
        if(pBaseline /* d is-a delta */){
          /* Deltas mark deletions with F-cards having only 
             a file name (no UUID or permission).
          */
          rc = fsl_deck_F_add(d, zName, NULL, perm, NULL);
        }/*else elide F-card to mark a deletion in a baseline.*/
      }else{
        if(zOrig && !isSel){
          /* File is renamed in vfile but is not being committed, so
             make sure we use the original name for the F-card.
          */
          zName = zOrig;
          zOrig = NULL;
        }
        assert(zUuid);
        assert(fileBlobRid);
        if( !zOrig || !renamed ){
          rc = fsl_deck_F_add(d, zName, zUuid, perm, NULL);
        }else{
          /* Rename this file */
          rc = fsl_deck_F_add(d, zName, zUuid, perm, zOrig);
        }
      }
    }
    fsl_free(fUuid);
    fUuid = NULL;
    RC;
    if( 0 == cmp ){
      fsl_deck_F_next(pBaseline, &pFile);
    }
  }/*while step()*/

  while( !rc && pFile ){
    /* Baseline has remaining files with lexically larger names. Let's import them. */
    rc = fsl_deck_F_add(d, pFile->name, NULL, pFile->perm, NULL);
    if(!rc) fsl_deck_F_next(pBaseline, &pFile);
  }

  end:
#undef RC
  fsl_free(fUuid);
  fsl_stmt_finalize(q);
  fsl_stmt_finalize(&stUpdateFileRid);
  if(!rc && changeCount) *changeCount = changeCounter;
  return rc;
}

/**
   Cancels all symbolic tags (branches) on the given version by
   adding one T-card to d for each active branch tag set on vid.
   When creating a branch, d would represent the branch and vid
   would be the version being branched from.

   Returns 0 on success.
*/
static int fsl_cancel_sym_tags( fsl_deck * d, fsl_id_t vid ){
  int rc;
  fsl_stmt q = fsl_stmt_empty;
  fsl_db * db = fsl_needs_repo(d->f);
  assert(db);
  rc = fsl_db_prepare(db, &q,
                      "SELECT tagname FROM tagxref, tag"
                      " WHERE tagxref.rid=%"FSL_ID_T_PFMT
                      " AND tagxref.tagid=tag.tagid"
                      "   AND tagtype>0 AND tagname GLOB 'sym-*'"
                      " ORDER BY tagname",
                      (fsl_id_t)vid);
  while( !rc && (FSL_RC_STEP_ROW==fsl_stmt_step(&q)) ){
    const char *zTag = fsl_stmt_g_text(&q, 0, NULL);
    rc = fsl_deck_T_add(d, FSL_TAGTYPE_CANCEL,
                        NULL, zTag, "Cancelled by branch.");
  }
  fsl_stmt_finalize(&q);
  return rc;
}

#if 0
static int fsl_leaf_set( fsl_cx * f, fsl_id_t rid, char isLeaf ){
  int rc;
  fsl_stmt * st = NULL;
  fsl_db * db = fsl_needs_repo(f);
  assert(db);
  rc = fsl_db_prepare_cached(db, &st, isLeaf
                             ? "INSERT OR IGNORE INTO leaf(rid) VALUES(?)"
                             : "DELETE FROM leaf WHERE rid=?");
  if(!rc){
    fsl_stmt_bind_id(st, 1, rid);
    fsl_stmt_step(st);
    fsl_stmt_cached_yield(st);
  }
  if(rc){
    fsl_cx_uplift_db_error(f, db);
  }
  return rc;
}
#endif

/**
   Checks vfile for any files (where chnged in (2,3,4,5)), i.e.
   having something to do with a merge. If either all of those
   changes are enqueued for checkin, or none of them are, then
   this function returns 0, otherwise it sets f's error
   state and returns non-0.
*/
static int fsl_check_for_partial_merge(fsl_cx * f){
  if(!f->ckin.selectedIds.entryCount){
    /* All files are considered enqueued. */
    return 0;
  }else{
    fsl_db * db = fsl_cx_db_ckout(f);
    int32_t counter = 0;
    int rc =
      fsl_db_get_int32(db, &counter,
                       "SELECT COUNT(*) FROM ("
#if 1
                       "SELECT DISTINCT fsl_is_enqueued(id)"
                       " FROM vfile WHERE chnged IN (2,3,4,5)"
#else
                       "SELECT fsl_is_enqueued(id) isSel "
                       "FROM vfile WHERE chnged IN (2,3,4,5) "
                       "GROUP BY isSel"
#endif
                       ")"
                       );
    /**
       Result is 0 if no merged files are in vfile, 1 row if isSel is
       the same for all merge-modified files, and 2 if there is a mix
       of selected/unselected merge-modified files.
     */
    if(!rc && (counter>1)){
      assert(2==counter);
      rc = fsl_cx_err_set(f, FSL_RC_MISUSE,
                          "Only Chuck Norris can commit "
                          "a partial merge. Commit either all "
                          "or none of it.");
    }
    return rc;
  }
}

/**
   Populates d with the contents for a FSL_SATYPE_CHECKIN manifest
   based on repository version basedOnVid.

   d is the deck to populate.

   basedOnVid must currently be f->ckout.rid OR the vfile table must
   be current for basedOnVid (see fsl_vfile_changes_scan() and
   fsl_vfile_load()). It "should" work with basedOnVid==0 but
   that's untested so far.

   opt is the options object passed to fsl_checkin_commit().
*/
static int fsl_checkin_calc_manifest( fsl_cx * f, fsl_deck * d,
                                      fsl_id_t basedOnVid,
                                      fsl_checkin_opt const * opt ){
  int rc;
  fsl_db * dbR = fsl_cx_db_repo(f);
  fsl_db * dbC = fsl_cx_db_ckout(f);
  fsl_stmt q = fsl_stmt_empty;
  fsl_deck dBase = fsl_deck_empty;
  char const * zColor;
  int deltaPolicy = opt->deltaPolicy;
  assert(d->f == f);
  assert(FSL_SATYPE_CHECKIN==d->type);
#define RC if(rc) goto end
  /* assert(basedOnVid>0); */
  rc = (opt->message && *opt->message)
    ? fsl_deck_C_set( d, opt->message, -1 )
    : fsl_cx_err_set(f, FSL_RC_MISSING_INFO,
                     "Cowardly refusing to commit with "
                     "empty checkin comment.");
  RC;

  if(deltaPolicy!=0 && fsl_repo_forbids_delta_manifests(f)){
    deltaPolicy = 0;
  }else if(deltaPolicy<0 && f->cache.seenDeltaManifest<=0){
    deltaPolicy = 0;
  }
  {
    char const * zUser = opt->user ? opt->user : fsl_cx_user_get(f);
    rc = (zUser && *zUser)
      ? fsl_deck_U_set( d, zUser )
      : fsl_cx_err_set(f, FSL_RC_MISSING_INFO,
                       "Cowardly refusing to commit without "
                       "a user name.");
    RC;
  }

  rc = fsl_check_for_partial_merge(f);
  RC;

  rc = fsl_deck_D_set( d, (opt->julianTime>0)
                       ? opt->julianTime
                       : fsl_db_julian_now(dbR) );
  RC;

  if(opt->messageMimeType && *opt->messageMimeType){
    rc = fsl_deck_N_set( d, opt->messageMimeType, -1 );
    RC;
  }


  { /* F-cards */
    static char const * errNoFilesMsg =
      "No files have changed. Cowardly refusing to commit.";
    static int const errNoFilesRc = FSL_RC_NOOP;
    fsl_deck * pBase = NULL /* baseline for delta generation purposes */;
    fsl_size_t szD = 0, szB = 0 /* see commentary below */;
    if(basedOnVid && deltaPolicy!=0){
      /* Figure out a baseline for a delta manifest... */
      rc = fsl_deck_load_rid(f, &dBase, basedOnVid, FSL_SATYPE_CHECKIN);
      RC;
      if(dBase.B.uuid){
        /* dBase is a delta. Let's use its baseline for manifest
           generation.
        */
        fsl_id_t const baseRid = fsl_uuid_to_rid(f, dBase.B.uuid);
        fsl_deck_finalize(&dBase);
        assert(baseRid>0);
        rc = fsl_deck_load_rid(f, &dBase, baseRid,
                               FSL_SATYPE_CHECKIN);
        RC;
      }else{
        /* dBase version is a suitable baseline. */
      }
      pBase = &dBase;
      /* MARKER(("Baseline = %d / %s\n", (int)pBase->rid, pBase->uuid)); */
      rc = fsl_deck_B_set(d, pBase->uuid);
      RC;
    }
    rc = fsl_checkin_calc_F_cards2(f, d, pBase, basedOnVid,
                                   &szD, opt);
    /*MARKER(("szD=%d\n", (int)szD));*/
    RC;
    if(basedOnVid && !szD){
      rc = fsl_cx_err_set(f, errNoFilesRc, errNoFilesMsg);
      goto end;
    }
    szB = pBase ? pBase->F.used : 0;
    /* The following text was copied verbatim from fossil(1). It does
       not apply 100% here (because we use a slightly different
       manifest generation approach) but it clearly describes what's
       going on after the comment block....
    */
    /*
    ** At this point, two manifests have been constructed, either of
    ** which would work for this checkin.  The first manifest (held
    ** in the "manifest" variable) is a baseline manifest and the second
    ** (held in variable named "delta") is a delta manifest.  The
    ** question now is: which manifest should we use?
    **
    ** Let B be the number of F-cards in the baseline manifest and
    ** let D be the number of F-cards in the delta manifest, plus one for
    ** the B-card.  (B is held in the szB variable and D is held in the
    ** szD variable.)  Assume that all delta manifests adds X new F-cards.
    ** Then to minimize the total number of F- and B-cards in the repository,
    ** we should use the delta manifest if and only if:
    **
    **      D*D < B*X - X*X
    **
    ** X is an unknown here, but for most repositories, we will not be
    ** far wrong if we assume X=3.
    */
    ++szD /* account for the d->B card */;
    if(pBase){
      /* For this calculation, i believe the correct approach is to
         simply count the F-cards, including those changed between the
         baseline and the delta, as opposed to only those changed in
         the delta itself.
      */
      szD = 1 + d->F.used;
    }
    /* MARKER(("szB=%d szD=%d\n", (int)szB, (int)szD)); */
    if(pBase && (deltaPolicy<0/*non-force-mode*/
                 && !(((int)(szD*szD)) < (((int)szB*3)-9))
                 /* ^^^ see comments above */
                 )
       ){
      /* Too small of a delta to be worth it. Re-calculate
         F-cards with no baseline.

         Maintenance reminder: i initially wanted to update vfile's
         status incrementally as F-cards are calculated, but this
         discard/retry breaks on the retry because vfile's state has
         been modified. Thus instead of updating vfile incrementally,
         we re-scan it after the checkin completes.
      */
      fsl_deck tmp = fsl_deck_empty;
      /* Free up d->F using a kludge... */
      tmp.F = d->F;
      d->F = fsl_deck_empty.F;
      fsl_deck_finalize(&tmp);
      fsl_deck_B_set(d, NULL);
      /* MARKER(("Delta is too big - re-calculating F-cards for a baseline.\n")); */
      szD = 0;
      rc = fsl_checkin_calc_F_cards2(f, d, NULL, basedOnVid,
                                     &szD, opt);
      RC;
      if(basedOnVid && !szD){
        rc = fsl_cx_err_set(f, errNoFilesRc, errNoFilesMsg);
        goto end;
      }
    }
  }/* F-cards */

  /* parents... */
  if( basedOnVid ){
    char * zParentUuid = fsl_rid_to_artifact_uuid(f, basedOnVid, FSL_SATYPE_CHECKIN);
    if(!zParentUuid){
      assert(f->error.code);
      rc = f->error.code
        ? f->error.code
        : fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                         "Could not find checkin UUID "
                         "for RID %"FSL_ID_T_PFMT".",
                         basedOnVid);
      goto end;
    }
    rc = fsl_deck_P_add(d, zParentUuid)
      /* pedantic side-note: we could alternately transfer ownership
         of zParentUuid by fsl_list_append()ing it to d->P, but that
         would bypass e.g. any checking that routine chooses to apply.
      */;
    fsl_free(zParentUuid);
    /* if(!rc) rc = fsl_leaf_set(f, basedOnVid, 0); */
    /* TODO:
       if( p->verifyDate ) checkin_verify_younger(vid, zParentUuid, zDate); */
    RC;
    rc = fsl_db_prepare(dbC, &q, "SELECT merge FROM vmerge WHERE id=0 OR id<-2");
    RC;
    while( FSL_RC_STEP_ROW == fsl_stmt_step(&q) ){
      char *zMergeUuid;
      fsl_id_t const mid = fsl_stmt_g_id(&q, 0);
      //MARKER(("merging? %d\n", (int)mid));
      if( (mid == basedOnVid)
          || (!f->cache.markPrivate && fsl_content_is_private(f,mid))){
        continue;
      }
      zMergeUuid = fsl_rid_to_uuid(f, mid)
        /* FIXME? Adjust the query to join on blob and return the UUID? */
        ;
      //MARKER(("merging %d %s\n", (int)mid, zMergeUuid));
      if(zMergeUuid){
        rc = fsl_deck_P_add(d, zMergeUuid);
        fsl_free(zMergeUuid);
      }
      RC;
      /* TODO:
         if( p->verifyDate ) checkin_verify_younger(mid, zMergeUuid, zDate); */
    }
    fsl_stmt_finalize(&q);
  }

  { /* Q-cards... */
    rc = fsl_db_prepare(dbR, &q,
                        "SELECT "
                        "CASE vmerge.id WHEN -1 THEN '+' ELSE '-' END || mhash,"
                        "  merge"
                        "  FROM vmerge"
                        " WHERE (vmerge.id=-1 OR vmerge.id=-2)"
                        " ORDER BY 1");
    while( !rc && (FSL_RC_STEP_ROW==fsl_stmt_step(&q)) ){
      fsl_id_t const mid = fsl_stmt_g_id(&q, 1);
      if( mid != basedOnVid ){
        const char *zCherrypickUuid = fsl_stmt_g_text(&q, 0, NULL);
        int const qType = '+'==*(zCherrypickUuid++) ? 1 : -1;
        rc = fsl_deck_Q_add( d, qType, zCherrypickUuid, NULL );
      }
    }
    fsl_stmt_finalize(&q);
    RC;
  }

  zColor = opt->bgColor;
  if(opt->branch && *opt->branch){
    char * sym = fsl_mprintf("sym-%s", opt->branch);
    if(!sym){
      rc = FSL_RC_OOM;
      goto end;
    }
    rc = fsl_deck_T_add( d, FSL_TAGTYPE_PROPAGATING,
                         NULL, sym, NULL );
    fsl_free(sym);
    RC;
    if(opt->bgColor && *opt->bgColor){
      zColor = NULL;
      rc = fsl_deck_T_add( d, FSL_TAGTYPE_PROPAGATING,
                           NULL, "bgcolor", opt->bgColor);
      RC;
    }
    rc = fsl_deck_T_add( d, FSL_TAGTYPE_PROPAGATING,
                         NULL, "branch", opt->branch );
    RC;
    if(basedOnVid){
      rc = fsl_cancel_sym_tags(d, basedOnVid);
    }
  }
  if(zColor && *zColor){
    /* One-shot background color */
    rc = fsl_deck_T_add( d, FSL_TAGTYPE_ADD,
                         NULL, "bgcolor", opt->bgColor);
    RC;
  }

  if(opt->closeBranch){
    rc = fsl_deck_T_add( d, FSL_TAGTYPE_ADD,
                         NULL, "closed",
                         *opt->closeBranch
                         ? opt->closeBranch
                         : NULL);
    RC;
  }

  {
    /*
      Close any INTEGRATE merges if !op->integrate, or type-0 and
      integrate merges if opt->integrate.
    */
    rc = fsl_db_prepare(dbC, &q,
                        "SELECT mhash, merge FROM vmerge "
                        " WHERE id %s ORDER BY 1",
                        opt->integrate ? "IN(0,-4)" : "=(-4)");
    while( !rc && (FSL_RC_STEP_ROW==fsl_stmt_step(&q)) ){
      fsl_id_t const rid = fsl_stmt_g_id(&q, 1);
      //MARKER(("Integrating %d? opt->integrate=%d\n",(int)rid, opt->integrate));
      if( fsl_rid_is_leaf(f, rid)
          && !fsl_db_exists(dbR, /* Is not closed already... */
                            "SELECT 1 FROM tagxref "
                            "WHERE tagid=%d AND rid=%"FSL_ID_T_PFMT
                            " AND tagtype>0",
                            FSL_TAGID_CLOSED, rid)){
        const char *zIntegrateUuid = fsl_stmt_g_text(&q, 0, NULL);
        //MARKER(("Integrating %d %s\n",(int)rid, zIntegrateUuid));
        rc = fsl_deck_T_add( d, FSL_TAGTYPE_ADD, zIntegrateUuid,
                             "closed", "Closed by integrate-merge." );
      }
    }
    fsl_stmt_finalize(&q);
    RC;
  }

  end:
#undef RC
  fsl_stmt_finalize(&q);
  fsl_deck_finalize(&dBase);
  d->B.baseline = NULL /* if it was set, it was &dBase */;
  if(rc && !f->error.code){
    if(dbR->error.code) fsl_cx_uplift_db_error(f, dbR);
    else if(dbC->error.code) fsl_cx_uplift_db_error(f, dbC);
    else if(f->dbMain->error.code) fsl_cx_uplift_db_error(f, f->dbMain);
  }
  return rc;
}

int fsl_checkin_T_add2( fsl_cx * f, fsl_card_T * t){
  return fsl_deck_T_add2( &f->ckin.mf, t );
}

int fsl_checkin_T_add( fsl_cx * f, fsl_tagtype_e tagType,
                       fsl_uuid_cstr uuid, char const * name,
                       char const * value){
  return fsl_deck_T_add( &f->ckin.mf, tagType, uuid, name, value );
}

/**
   Returns true if the given blob RID is has a "closed" tag. This is
   generally intended only to be passed the RID of the current
   checkout, before attempting to perform a commit against it.
*/
static bool fsl_leaf_is_closed(fsl_cx * f, fsl_id_t rid){
  fsl_db * const dbR = fsl_needs_repo(f);
  return dbR
    ? fsl_db_exists(dbR, "SELECT 1 FROM tagxref"
                    " WHERE tagid=%d "
                    " AND rid=%"FSL_ID_T_PFMT" AND tagtype>0",
                    FSL_TAGID_CLOSED, rid)
    : false;
}

/**
   Returns true if the given name is the current branch
   for the given checkin version.
 */
static bool fsl_is_current_branch(fsl_db * dbR, fsl_id_t vid,
                                  char const * name){
  return fsl_db_exists(dbR,
                       "SELECT 1 FROM tagxref"
                       " WHERE tagid=%d AND rid=%"FSL_ID_T_PFMT
                       " AND tagtype>0"
                       " AND value=%Q",
                       FSL_TAGID_BRANCH, vid, name);
}

int fsl_checkin_commit(fsl_cx * f, fsl_checkin_opt const * opt,
                       fsl_id_t * newRid, fsl_uuid_str * newUuid ){
  int rc;
  fsl_deck deck = fsl_deck_empty;
  fsl_deck *d = &deck;
  fsl_db * dbC;
  fsl_db * dbR;
  char inTrans = 0;
  char oldPrivate;
  int const oldFlags = f ? f->flags : 0;
  fsl_id_t const vid = f ? f->ckout.rid : 0;
  if(!f || !opt) return FSL_RC_MISUSE;
  else if(!(dbC = fsl_needs_ckout(f))) return FSL_RC_NOT_A_CKOUT;
  else if(!(dbR = fsl_needs_repo(f))) return FSL_RC_NOT_A_REPO;
  assert(vid>=0);
  /**
     Do not permit a checkin to a closed leaf unless opt->branch would
     switch us to a new branch.
  */
  if( fsl_leaf_is_closed(f, vid)
      && (!opt->branch || !*opt->branch
          || fsl_is_current_branch(dbR, vid, opt->branch))){
    return fsl_cx_err_set(f, FSL_RC_ACCESS,
                          "Only Chuck Norris can commit to "
                          "a closed leaf.");
  }

  if(vid && opt->scanForChanges){
    /* We need to ensure this state is current in order to determine
       whether a given file is locally modified vis-a-vis the
       commit-time vfile state. */
    rc = fsl_vfile_changes_scan(f, vid, 0);
    if(rc) return rc;
  }

  fsl_cx_err_reset(f) /* avoid propagating an older error by accident.
                         Did that in test code. */;

  oldPrivate = f->cache.markPrivate;
  if(opt->isPrivate || fsl_content_is_private(f, vid)){
    f->cache.markPrivate = 1;
  }

#define RC if(rc) goto end
  fsl_deck_init(f, d, FSL_SATYPE_CHECKIN);

  rc = fsl_db_transaction_begin(dbR);
  RC;
  inTrans = 1;
  if(f->ckin.mf.T.used){
    /* Transfer accumulated tags. */
    assert(!f->ckin.mf.content.used);
    d->T = f->ckin.mf.T;
    f->ckin.mf.T = fsl_deck_empty.T;
  }
  rc = fsl_checkin_calc_manifest(f, d, vid, opt);
  RC;
  if(!d->F.used){
    rc = fsl_cx_err_set(f, FSL_RC_NOOP,
                        "Cowardly refusing to generate an empty commit.");
    RC;
  }

  if(opt->calcRCard) f->flags |= FSL_CX_F_CALC_R_CARD;
  else f->flags &= ~FSL_CX_F_CALC_R_CARD;
  rc = fsl_deck_save( d, opt->isPrivate );
  RC;
  assert(d->rid>0);
  assert(d->uuid);
  /* Now get vfile back into shape. We do not do a vfile scan
     because that loses state like add/rm-queued files. */
  rc = fsl_db_exec_multi(dbC,
                         "DELETE FROM vfile WHERE vid<>"
                         "%" FSL_ID_T_PFMT ";"
                         "UPDATE vfile SET vid=%" FSL_ID_T_PFMT ";"
                         "DELETE FROM vfile WHERE deleted AND "
                         "fsl_is_enqueued(id); "
                         "UPDATE vfile SET rid=mrid, mhash=NULL, "
                         "chnged=0, deleted=0, origname=NULL "
                         "WHERE fsl_is_enqueued(id)",
                         vid, d->rid);
  if(!rc) rc = fsl_ckout_version_write(f, d->rid, d->uuid);
  RC;
  assert(d->f == f);
  rc = fsl_checkin_add_unsent(f, d->rid);
  RC;
  rc = fsl_ckout_clear_merge_state(f);
  RC;
  /*
    todo(?) from fossil(1) follows. Most of this seems to be what the
    vfile handling does (above).

    db_multi_exec("PRAGMA %s.application_id=252006673;", db_name("repository"));
    db_multi_exec("PRAGMA %s.application_id=252006674;", db_name("localdb"));

    // Update the vfile and vmerge tables
    db_multi_exec(
      "DELETE FROM vfile WHERE (vid!=%d OR deleted) AND is_selected(id);"
      "DELETE FROM vmerge;"
      "UPDATE vfile SET vid=%d;"
      "UPDATE vfile SET rid=mrid, chnged=0, deleted=0, origname=NULL"
      " WHERE is_selected(id);"
      , vid, nvid
    );
    db_lset_int("checkout", nvid);


    // Update the isexe and islink columns of the vfile table
    db_prepare(&q,
      "UPDATE vfile SET isexe=:exec, islink=:link"
      " WHERE vid=:vid AND pathname=:path AND (isexe!=:exec OR islink!=:link)"
    );
    db_bind_int(&q, ":vid", nvid);
    pManifest = manifest_get(nvid, CFTYPE_MANIFEST, 0);
    manifest_file_rewind(pManifest);
    while( (pFile = manifest_file_next(pManifest, 0)) ){
      db_bind_int(&q, ":exec", pFile->zPerm && strstr(pFile->zPerm, "x"));
      db_bind_int(&q, ":link", pFile->zPerm && strstr(pFile->zPerm, "l"));
      db_bind_text(&q, ":path", pFile->zName);
      db_step(&q);
      db_reset(&q);
    }
    db_finalize(&q);
  */

  if(opt->dumpManifestFile){
    FILE * out;
    /* MARKER(("Dumping generated manifest to file [%s]:\n", opt->dumpManifestFile)); */
    out = fsl_fopen(opt->dumpManifestFile, "w");
    if(out){
      rc = fsl_deck_output( d, fsl_output_f_FILE, out );
      fsl_fclose(out);
    }else{
      rc = fsl_cx_err_set(f, FSL_RC_IO, "Could not open output "
                          "file for writing: %s", opt->dumpManifestFile);
    }
    RC;
  }

  if(d->P.used){
    /* deltify the parent manifest */
    char const * p0 = (char const *)d->P.list[0];
    fsl_id_t const prid = fsl_uuid_to_rid(f, p0);
    /* MARKER(("Deltifying parent manifest #%d...\n", (int)prid)); */
    assert(p0);
    assert(prid>0);
    rc = fsl_content_deltify(f, prid, d->rid, 0);
    RC;
  }

  end:
  f->flags = oldFlags;
#undef RC
  f->cache.markPrivate = oldPrivate;
  /* fsl_buffer_reuse(&f->fileContent); */
  if(inTrans){
    if(rc) fsl_db_transaction_rollback(dbR);
    else{
      rc = fsl_db_transaction_commit(dbR);
      if(!rc){
        if(newRid) *newRid = d->rid;
        if(newUuid){
          *newUuid = d->uuid;
          d->uuid = NULL /* transfer ownership */;
        }
      }
    }
  }
  if(rc && !f->error.code){
    if(dbR->error.code) fsl_cx_uplift_db_error(f, dbR);
    else if(f->dbMain->error.code) fsl_cx_uplift_db_error(f, f->dbMain);
  }
  fsl_checkin_discard(f);
  fsl_deck_finalize(d);
  return rc;
}


#undef MARKER
