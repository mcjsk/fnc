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
  This file contains some of the APIs dealing with the checkout state.
*/
#include "fossil-scm/fossil-internal.h"
#include "fossil-scm/fossil-hash.h"
#include "fossil-scm/fossil-checkout.h"
#include "fossil-scm/fossil-confdb.h"
#include <assert.h>

/* Only for debugging */
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)

int fsl_vfile_load(fsl_cx * f, fsl_id_t vid,
                   bool clearOtherVersions,
                   uint32_t * missingCount){
  fsl_db * dbC = f ? fsl_needs_ckout(f) : NULL;
  fsl_db * dbR = dbC ? fsl_needs_repo(f) : NULL;
  fsl_deck d = fsl_deck_empty;
  fsl_stmt qIns = fsl_stmt_empty;
  fsl_stmt qRid = fsl_stmt_empty;
  int rc;
  bool alreadyHad;
  fsl_card_F const * fc;
  assert(dbC && "Must only be called when a checkout is opened.");
  assert(dbR && "Must only be called when a repo is opened.");
  if(!dbC) return FSL_RC_NOT_A_CKOUT;
  else if(!dbR) return FSL_RC_NOT_A_REPO;
  if(vid<=0) vid = f->ckout.rid;
  assert(vid>=0);

  rc = fsl_db_transaction_begin(dbC);
  if(rc) return rc;
  alreadyHad = fsl_db_exists(dbC,
                             "SELECT 1 FROM vfile WHERE vid=%"FSL_ID_T_PFMT,
                             vid);
  if(clearOtherVersions){
    /* Reminder to self: DO NOT clear vmerge here. Doing so will break
       merge tracking in the checkin process. */
    rc = fsl_vfile_unload_except(f, vid);
    if(rc) goto end;
  }
  if(alreadyHad){
    /* Already done. */
    rc = 0;
    goto end;
  }
  if(rc) goto end;

  if(0==vid){
    /* This is either misuse or an empty/initial repo with no
       checkins.  Let's assume the latter, since that's what triggered
       the addition of this check. */
    goto end;
  }

  rc = fsl_deck_load_rid(f, &d, vid, FSL_SATYPE_CHECKIN);
  if(rc) goto end;
  assert(d.rid==vid);
  rc = fsl_deck_F_rewind(&d);
  if(rc) goto end;
  rc = fsl_db_prepare(dbC, &qIns,
                      "INSERT INTO vfile"
                      "(vid,isexe,islink,rid,mrid,pathname,mhash) "
                      "VALUES(:vid,:isexe,:islink,:id,:id,:name,null)");
  if(rc) goto end;
  rc = fsl_db_prepare(dbR, &qRid,
                      "SELECT rid,size FROM blob WHERE uuid=?");
  if(rc) goto end;
  rc = fsl_stmt_bind_id_name(&qIns, ":vid", vid);
  while( !rc && !(rc=fsl_deck_F_next(&d, &fc)) && fc){
    fsl_id_t rid;
    int64_t size;
    assert(fc->uuid && "We couldn't get F-card deletions via fsl_deck_F_next()");
    if(fsl_uuid_is_shunned(f,fc->uuid)) continue;
    rc = fsl_stmt_bind_text(&qRid, 1, fc->uuid, -1, 0);
    if(rc) break;
    rc = fsl_stmt_step(&qRid);
    if(FSL_RC_STEP_ROW==rc){
      rid = fsl_stmt_g_id(&qRid,0);
      size = fsl_stmt_g_int64(&qRid,1);
    }else if(FSL_RC_STEP_DONE==rc){
      rid = 0;
      size = 0;
    }else{
      assert(qRid.db->error.code);
      rc = fsl_cx_uplift_db_error(f, qRid.db);
      break;
    }
    fsl_stmt_reset(&qRid);
    if( !rid || size<0 ){
      if(missingCount) ++*missingCount;
      continue;
    }
    fsl_stmt_bind_int32_name(&qIns, ":isexe",
                             (FSL_FILE_PERM_EXE & fc->perm) ? 1 : 0);
    fsl_stmt_bind_int32_name(&qIns, ":islink",
                             (FSL_FILE_PERM_LINK & fc->perm) ? 1 : 0);
    fsl_stmt_bind_id_name(&qIns, ":id", rid);
    rc = fsl_stmt_bind_text_name(&qIns, ":name", fc->name, -1, 0);
    if(rc) break;
    rc = fsl_stmt_step(&qIns);
    if(FSL_RC_STEP_DONE!=rc) break;
    else rc = 0;
    fsl_stmt_reset(&qIns);
  }
  
  end:
  fsl_stmt_finalize(&qIns);
  fsl_stmt_finalize(&qRid);
  /* Update f->ckout state and some db bits we need
     when changing the checkout. */
  if(!rc && vid>0){
    if(!alreadyHad){
      assert(d.rid>0);
      assert(d.uuid);
    }
  }
  fsl_deck_finalize(&d);
  if(rc) fsl_db_transaction_rollback(dbC);
  else rc = fsl_db_transaction_commit(dbC);
  if(rc && !f->error.code){
    if(dbC->error.code) fsl_cx_uplift_db_error(f, dbC);
    else if(dbR->error.code) fsl_cx_uplift_db_error(f, dbR);
  }
  return rc;
}

static int fsl_vfile_unload_impl(fsl_cx * f, fsl_id_t vid,
                                 bool oneVersion){
  fsl_db * const db = fsl_needs_ckout(f);
  if(!db) return FSL_RC_NOT_A_CKOUT;
  if(vid<=0) vid = f->ckout.rid;
  int const rc = fsl_db_exec(db, "DELETE FROM vfile "
                             "WHERE vid%s%" FSL_ID_T_PFMT
                             " /* %s() */",
                             oneVersion ? "=" : "<>",
                             vid, __func__);
  return rc ? fsl_cx_uplift_db_error2(f, db, rc) : 0;
}
int fsl_vfile_unload(fsl_cx * f, fsl_id_t vid){
  return fsl_vfile_unload_impl(f, vid, true);
}
int fsl_vfile_unload_except(fsl_cx * f, fsl_id_t vid){
  return fsl_vfile_unload_impl(f, vid, false);
}

/**
   Internal code de-duplifier for places which need to re-check a
   file's hash in order to be sure whether it was really
   modified. hashLen must be the length of the previous (db-side) hash
   of the file. This routine will hash that file using the same hash
   type. The new hash is appended to pTgt.

   Returns 0 on success.
*/
static int fsl_vfile_recheck_file_hash( fsl_cx * f, const char * zName,
                                        fsl_size_t hashLen, fsl_buffer * pTgt ){
  bool errReported = false;
  int rc = 0;
  if((fsl_size_t)FSL_STRLEN_SHA1==hashLen){
    rc = fsl_sha1sum_filename(zName, pTgt);
  }else if((fsl_size_t)FSL_STRLEN_K256==hashLen){
    rc = fsl_sha3sum_filename(zName, pTgt);
  }else{
    assert(!"This \"cannot happen\".");
    rc = fsl_cx_err_set(f, FSL_RC_CHECKSUM_MISMATCH,
                        "Cannot determine which hash to use for file: %s",
                        zName);
    errReported = true;
  }
  if(rc && !errReported && FSL_RC_OOM != rc){
    rc = fsl_cx_err_set(f, rc, "Error %s while hashing file: %s",
                        fsl_rc_cstr(rc), zName);
  }
  //if(!rc) assert(fsl_is_uuid_len(pTgt->used));
  return rc;
}


int fsl_vfile_changes_scan(fsl_cx * f, fsl_id_t vid, unsigned cksigFlags){
  fsl_stmt * stUpdate = NULL;
  fsl_stmt q = fsl_stmt_empty;
  int rc = 0;
  fsl_db * const db = fsl_needs_ckout(f);
  fsl_fstat fst = fsl_fstat_empty;
  fsl_size_t rootLen;
  fsl_buffer * fileCksum = fsl_cx_scratchpad(f);
  bool const useMtime = (cksigFlags & FSL_VFILE_CKSIG_HASH)==0
    && fsl_config_get_bool(f, FSL_CONFDB_REPO, true, "mtime-changes");
  if(!db) return FSL_RC_NOT_A_CKOUT;
  assert(f->ckout.dir);
  if(vid<=0) vid = f->ckout.rid;
  assert(vid>=0);
  rootLen = fsl_strlen(f->ckout.dir);
  assert(rootLen);

  rc = fsl_db_transaction_begin(db);
  if(rc) return rc;
  if(f->ckout.rid != vid){
    rc = fsl_vfile_load(f, vid,
                                 (FSL_VFILE_CKSIG_KEEP_OTHERS & cksigFlags)
                                 ? false : true,
                                 NULL);
  }
  if(rc) goto end;

#if 0
  MARKER(("changed/deleted vfile contents post load-from-rid:\n"));
  fsl_db_each( fsl_cx_db_ckout(f), fsl_stmt_each_f_dump, NULL,
               "SELECT vf.id, substr(b.uuid,0,8) hash, chnged, "
               "deleted, vf.pathname "
               "FROM vfile vf LEFT JOIN blob b "
               "ON b.rid=vf.rid "
               "WHERE vf.vid=%"FSL_ID_T_PFMT" "
               "AND (chnged<>0 OR pathname<>origname OR deleted<>0)"
               "ORDER BY vf.id", vid);
#endif

  rc = fsl_db_prepare(db, &q, "SELECT "
                      /*0*/"id,"
                      /*1*/"%Q || pathname,"
                      /*2*/"vfile.mrid,"
                      /*3*/"deleted,"
                      /*4*/"chnged,"
                      /*5*/"uuid,"
                      /*6*/"size,"
                      /*7*/"mtime,"
                      /*8*/"isexe,"
                      /*9*/"islink, "
                      /*10*/"CASE WHEN isexe THEN %d "
                            "WHEN islink THEN %d ELSE %d END "
                      "FROM vfile LEFT JOIN blob ON vfile.mrid=blob.rid "
                      "WHERE vid=%"FSL_ID_T_PFMT,
                      f->ckout.dir,
                      FSL_FILE_PERM_EXE, FSL_FILE_PERM_LINK,
                      FSL_FILE_PERM_REGULAR,
                      (fsl_id_t)vid);
  if(rc) goto end;
  while( fsl_stmt_step(&q) == FSL_RC_STEP_ROW ){
    fsl_id_t id, rid;
    char const * zName;
#ifndef _WIN32
    //char const * relName;
#endif
    fsl_size_t nName = 0;
    int isDeleted;
    int64_t currentSize;
    int64_t origSize;
    int changed, oldChanged;
    //int isExe;
    fsl_time_t oldMtime, currentMtime;
#if !defined(_WIN32)
    int origPerm;
    int currentPerm;
#endif
    id = fsl_stmt_g_id(&q, 0);
    assert(id>0);
    zName = fsl_stmt_g_text(&q, 1, &nName);
    rid = fsl_stmt_g_id(&q, 2);
    isDeleted = fsl_stmt_g_int32(&q, 3);
    oldChanged = changed = fsl_stmt_g_int32(&q, 4);
    origSize = fsl_stmt_g_int64(&q, 6);
    oldMtime = (fsl_time_t)fsl_stmt_g_int64(&q, 7);
    //isExe = fsl_stmt_g_int32(&q, 8);
    rc = fsl_cx_stat( f, false, zName, &fst );
    currentSize = rc ? -1 : (int64_t)fst.size;
    currentMtime = rc ? 0 : fst.mtime;
    if(rc){
      fsl_cx_err_reset(f);
      rc = 0;
    }
#if !defined(_WIN32)
    //relName = zName + rootLen;
    origPerm = fsl_stmt_g_int32(&q, 10);
    currentPerm = (FSL_FSTAT_PERM_EXE==fst.perm
                   ? FSL_FILE_PERM_EXE
                   : FSL_FILE_PERM_REGULAR)
                   /*(FSL_FSTAT_TYPE_LINK==fst.type
                      ? FSL_FILE_PERM_LINK
                      : FSL_FILE_PERM_REGULAR)*/
      /* ^^^ FIXME: this isn't right for symlinks. For those we have
         to treat them as FSL_FILE_PERM_LINK when the repo has symlink
         support enabled, else FSL_FILE_PERM_REGULAR. That's to fix
         if/when we ever support SCM'd symlinks in the library. */
      ;
#endif
    if(!changed && (isDeleted || !rid)){
      /* ADD and REMOVE operations always change the file */
      changed = FSL_VFILE_CHANGE_MOD;
    }
    else if( currentSize>=0
             && !(FSL_FSTAT_TYPE_FILE==fst.type
                  || FSL_FSTAT_TYPE_LINK==fst.type)){
      if( FSL_VFILE_CKSIG_ENOTFILE & cksigFlags ){
        rc = fsl_cx_err_set(f, FSL_RC_TYPE,
                            "Not an ordinary file or symlink: %s",
                            zName);
        goto end;
      }
      changed = FSL_VFILE_CHANGE_MOD;
    } 
    if(origSize!=currentSize){
      changed = FSL_VFILE_CHANGE_MOD;
      /* A file size change is definitive - the file has changed. No
         need to check the mtime or hash */
    }else if( changed==FSL_VFILE_CHANGE_MOD && rid!=0 && !isDeleted ){
      /* File is believed to have changed but it is the same size.
         Double check that it really has changed by looking at its
         content. */
      fsl_size_t nUuid = 0;
      char const * uuid;
      fsl_buffer_reuse(fileCksum);
      assert( origSize==currentSize );
      uuid = fsl_stmt_g_text(&q, 5, &nUuid);
      assert(uuid && fsl_is_uuid_len((int)nUuid));
      rc = fsl_vfile_recheck_file_hash(f, zName, (int)nUuid, fileCksum);
      if(rc) goto end;
      assert(fsl_is_uuid_len((int)fileCksum->used));
      if( 0 == fsl_uuidcmp(fsl_buffer_cstr(fileCksum), uuid) ){
        changed = 0;
      }
    }else if( (changed==FSL_VFILE_CHANGE_NONE
               || changed==FSL_VFILE_CHANGE_MERGE_MOD
               || changed==FSL_VFILE_CHANGE_INTEGRATE_MOD)
              && (!useMtime || currentMtime!=oldMtime) ){
      /* For files that were formerly believed to be unchanged or that
         were changed by merging, if their mtime changes, or
         unconditionally if FSL_VFILE_CKSIG_SETMTIME is used, check to
         see if they have been edited by looking at their hash sum */
      fsl_size_t nUuid = 0;
      char const * uuid;
      assert( origSize==currentSize );
      uuid = fsl_stmt_g_text(&q, 5, &nUuid);
      assert(uuid && fsl_is_uuid_len((int)nUuid));
      fsl_buffer_reuse(fileCksum);
      rc = fsl_vfile_recheck_file_hash(f, zName, nUuid, fileCksum);
      if(rc) goto end;
      assert(fsl_is_uuid_len((int)fileCksum->used));
      if( fsl_uuidcmp(fsl_buffer_cstr(fileCksum), uuid) ){
        changed = FSL_VFILE_CHANGE_MOD;
      }
      /* MARKER(("SHA compare says %d: %s\n", changed, zName)); */
    }
    if( (cksigFlags & FSL_VFILE_CKSIG_SETMTIME)
        && (changed==FSL_VFILE_CHANGE_NONE
            || changed==FSL_VFILE_CHANGE_MERGE_MOD
            || changed==FSL_VFILE_CHANGE_INTEGRATE_MOD) ){
      fsl_time_t desiredMtime = 0;
      if( 0==fsl_mtime_of_manifest_file(f, vid, rid, &desiredMtime)){
        if( currentMtime != desiredMtime ){
          fsl_file_mtime_set(zName, desiredMtime);
          currentMtime = fsl_file_mtime(zName);
        }
      }
    }
    /* Check for perms differences. */
#if !defined(_WIN32)
    if( origPerm!=FSL_FILE_PERM_LINK && currentPerm==FSL_FILE_PERM_LINK ){
       /* Changing to a symlink takes priority over all other change types. */
       changed = FSL_VFILE_CHANGE_BECAME_SYMLINK;
    }else if( changed==0
              || changed==FSL_VFILE_CHANGE_IS_EXEC
              || changed==FSL_VFILE_CHANGE_BECAME_SYMLINK
              || changed==FSL_VFILE_CHANGE_NOT_EXEC
              || changed==FSL_VFILE_CHANGE_NOT_SYMLINK ){
       /* Confirm metadata change types. */
      if( origPerm==currentPerm ){
        changed = 0;
      }else if( currentPerm==FSL_FILE_PERM_EXE ){
        changed = FSL_VFILE_CHANGE_IS_EXEC;
      }else if( origPerm==FSL_FILE_PERM_EXE ){
        changed = FSL_VFILE_CHANGE_NOT_EXEC;
      }else if( origPerm==FSL_FILE_PERM_LINK ){
        changed = FSL_VFILE_CHANGE_NOT_SYMLINK;
      }
    }
#endif
    if( currentMtime!=oldMtime || changed!=oldChanged ){
      if(!stUpdate){
        rc = fsl_db_prepare_cached(db, &stUpdate,
                                   "UPDATE vfile SET "
                                   "mtime=?1, chnged=?2 "
                                   "WHERE id=?3 "
                                   "/*%s()*/",__func__);
        if(rc) goto end;
      }else{
        fsl_stmt_reset(stUpdate);
      }
      fsl_stmt_bind_int64(stUpdate, 1, currentMtime);
      fsl_stmt_bind_int32(stUpdate, 2, changed);
      fsl_stmt_bind_id(stUpdate, 3, id);
      rc = fsl_stmt_step(stUpdate);
      if(FSL_RC_STEP_DONE!=rc) goto end;
      rc = 0;
      /* MARKER(("UPDATED vfile.(mtime,chnged) for: %s\n", zName)); */
    }
  }/*while(step)*/

#if 0
  MARKER(("changed/deleted vfile contents post vfile scan:\n"));
  fsl_db_each( fsl_cx_db_ckout(f), fsl_stmt_each_f_dump, NULL,
               "SELECT vf.id, substr(b.uuid,0,8) hash, chnged, "
               "deleted, vf.pathname "
               "FROM vfile vf LEFT JOIN blob b "
               "ON b.rid=vf.rid "
               "WHERE vf.vid=%"FSL_ID_T_PFMT" "
               "AND (chnged<>0 OR pathname<>origname OR deleted<>0)"
               "ORDER BY vf.id", vid);
#endif
  end:
  fsl_cx_scratchpad_yield(f, fileCksum);
  if(!rc && (cksigFlags & FSL_VFILE_CKSIG_WRITE_CKOUT_VERSION)
     && (f->ckout.rid != vid)){
    rc = fsl_ckout_version_write(f, vid, 0);
  }else if(rc){
    rc = fsl_cx_uplift_db_error2(f, db, rc);
  }
  if(rc) {
    fsl_db_transaction_rollback(db);
  }else{
    rc = fsl_db_transaction_commit(db);
    if(rc){
      rc = fsl_cx_uplift_db_error2(f, db, rc);
    }
  }
  fsl_stmt_cached_yield(stUpdate);
  fsl_stmt_finalize(&q);
  return rc;
}

int fsl_vfile_to_ckout(fsl_cx * f, fsl_id_t vfileId,
                       int * wasWritten){
  int rc = 0;
  fsl_db * const db = fsl_needs_ckout(f);
  fsl_stmt q = fsl_stmt_empty;
  int counter = 0;
  fsl_buffer content = fsl_buffer_empty;
  char const * sql;
  fsl_id_t qArg;
  fsl_fstat * const fst = &f->cache.fstat;
  if(!db) return FSL_RC_NOT_A_CKOUT;
  assert(f->ckout.rid);
  if(vfileId){
    sql = "SELECT v.id, "
      "%Q || v.pathname, "
      "v.mrid, "
      "v.isexe, v.islink, b.uuid, b.size "
      "FROM vfile v, blob b"
      " WHERE v.id=%" FSL_ID_T_PFMT
      " AND v.mrid>0 "
      " AND v.mrid=b.rid /*%s()*/";
    qArg = vfileId;
  }else{
    sql = "SELECT v.id, "
      "%Q || v.pathname, "
      "v.mrid, "
      "v.isexe, v.islink, b.uuid, b.size "
      "FROM vfile v, blob b"
      " WHERE v.vid=%" FSL_ID_T_PFMT
      " AND v.mrid>0 "
      " AND v.mrid=b.rid /*%s()*/";
    qArg = f->ckout.rid;
  }
#undef VFILE_NAMEPART
  assert(qArg>=0);
  rc = fsl_db_prepare(db, &q, sql, f->ckout.dir, qArg, __func__);
  if(rc){
    rc = fsl_cx_uplift_db_error2(f, db, rc);
    goto end;
  }
  while(FSL_RC_STEP_ROW==(rc = fsl_stmt_step(&q))){
    //fsl_id_t const id = fsl_stmt_g_id(&q, 0);
    fsl_id_t const rid = fsl_stmt_g_id(&q, 2);
    int32_t const isExe = fsl_stmt_g_int32(&q, 3);
    int32_t const isLink = fsl_stmt_g_int32(&q, 4);
    int64_t const sz = fsl_stmt_g_int64(&q, 6);
    fsl_size_t nameLen = 0;
    char const * zName = fsl_stmt_g_text(&q, 1, &nameLen);
    fsl_size_t hashLen = 0;
    char const * zHash = fsl_stmt_g_text(&q, 5, &hashLen);
    char const * zRelName = &zName[f->ckout.dirLen];
    int isMod = 0;
    ++counter;
    assert(nameLen > f->ckout.dirLen);
    rc = fsl_ckout_safe_file_check(f, zName);
    if(rc) break;
    assert(fsl_is_uuid_len(hashLen));
    f->cache.fstat = fsl_fstat_empty;
    rc = fsl_is_locally_modified(f, zName, sz, zHash,
                                 (fsl_int_t)hashLen,
                                 isExe ? FSL_FILE_PERM_EXE :
                                 (isLink
                                  ? FSL_FILE_PERM_LINK
                                  : FSL_FILE_PERM_REGULAR),
                                 &isMod)
      /* that updates f->cache.fstat */;
    if(rc) break;
    else if(FSL_FSTAT_TYPE_DIR==fst->type){
      /* Fossil checks for this but if this happens then
         we have an invalid vfile entry or someone replaced
         a file with a dir. */         
      rc = fsl_cx_err_set(f, FSL_RC_TYPE,
                          "Cannot overwrite a directory: %s",
                          zRelName);
      break;
    }
    else if(!isMod) continue;
    else if((rc=fsl_mkdir_for_file(zName, true))){
      rc = fsl_cx_err_set(f, rc, "mkdir() failed for file: %s",
                          zName);
      break;
    }
    if(FSL_LOCALMOD_LINK & isMod){
      assert(((isLink && FSL_FILE_PERM_LINK!=fst->perm)
              ||(!isLink && FSL_FILE_PERM_LINK==fst->perm))
             && "Expected fsl_is_locally_modified() to set this.");
      rc = fsl_file_unlink(zName);
      if(rc){
        rc = fsl_cx_err_set(f, rc,
                            "Error removing target to replace it: %s",
                            zRelName);
        break;
      }
    }
    if(isLink || (isMod & (FSL_LOCALMOD_NOTFOUND
                           | FSL_LOCALMOD_LINK
                           | FSL_LOCALMOD_CONTENT))){
      /* switched link type, content changed, or was not found in the
         filesystem. */
      rc = fsl_content_get(f, rid, &content);
      if(rc) break;
    }
    if(isLink){
      rc = fsl_ckout_symlink_create(f, zName,
                                    fsl_buffer_cstr(&content));
      if(wasWritten && !rc) *wasWritten = 2;
    }else if(isMod & (FSL_LOCALMOD_NOTFOUND | FSL_LOCALMOD_CONTENT)){
      /* Not found locally or its contents differ. */
      rc = fsl_buffer_to_filename(&content, zName);
      if(rc){
        rc = fsl_cx_err_set(f, rc, "Error writing to file: %s",
                            zRelName);
      }else if(wasWritten){
        *wasWritten = 2;
      }
    }else if(wasWritten && (isMod & FSL_LOCALMOD_PERM)){
      *wasWritten = 1;
    }
    if(rc) break;
    fsl_file_exec_set(zName, !!isExe);
    fsl_buffer_reuse(&content);
  }/*step() loop*/
  switch(rc){
    case FSL_RC_STEP_DONE:
      if(counter){
        rc = 0;
      }else{
        rc = fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                            "No entry found: vfile.id=%" FSL_ID_T_PFMT,
                            vfileId);
      }
      break;
    default: break;
  }    
  end:
  fsl_buffer_clear(&content);
  fsl_stmt_finalize(&q);
  return rc;
}

#undef MARKER
