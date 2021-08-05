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
/*  
  *****************************************************************************
  This file houses the code for checkout-level APIS.
*/
#include <assert.h>

#include "fossil-scm/fossil-internal.h"
#include "fossil-scm/fossil-core.h"
#include "fossil-scm/fossil-checkout.h"
#include "fossil-scm/fossil-hash.h"
#include "fossil-scm/fossil-confdb.h"
#include <string.h> /* memcmp() */

/* Only for debugging */
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)


/**
    Kludge for type-safe strncmp/strnicmp inconsistency.
*/
static int fsl_strnicmp_int(char const *zA, char const * zB, fsl_size_t nByte){
  return fsl_strnicmp( zA, zB, (fsl_int_t)nByte);
}

int fsl_ckout_filename_check( fsl_cx * f, bool relativeToCwd,
                              char const * zOrigName, fsl_buffer * pOut ){
  int rc;
  if(!zOrigName || !*zOrigName) return FSL_RC_MISUSE;
  else if(!fsl_needs_ckout(f)/* will update f's error state*/){
    return FSL_RC_NOT_A_CKOUT;
  }
#if 0
  /* Is this sane? */
  else if(fsl_is_simple_pathname(zOrigName,1)){
    rc = 0;
    if(pOut){
      rc = fsl_buffer_append(pOut, zOrigName, fsl_strlen(zOrigName));
    }
  }
#endif
  else{
    char const * zLocalRoot;
    char const * zFull;
    fsl_size_t nLocalRoot;
    fsl_size_t nFull;
    fsl_buffer * full = fsl_cx_scratchpad(f);
    int (*xCmp)(char const *, char const *,fsl_size_t);
    bool endsWithSlash;
    assert(f->ckout.dir);
    zLocalRoot = f->ckout.dir;
    assert(zLocalRoot);
    assert(*zLocalRoot);
    nLocalRoot = f->ckout.dirLen;
    assert(nLocalRoot);
    assert('/' == zLocalRoot[nLocalRoot-1]);
    rc = fsl_file_canonical_name2(relativeToCwd ? NULL : zLocalRoot,
                                  zOrigName, full, 1);
#if 0
    MARKER(("canon2: %p (%s) %s ==> %s\n", (void const *)full->mem,
            relativeToCwd ? "cwd" : "ckout", zOrigName, fsl_buffer_cstr(full)));
#endif
    if(rc){
      if(FSL_RC_OOM != rc){
        rc = fsl_cx_err_set(f, rc, "Error #%d (%s) canonicalizing "
                            "file name: %s\n",
                            rc, fsl_rc_cstr(rc),
                            zOrigName);
      }
      goto end;
    }
    zFull = fsl_buffer_cstr2(full, &nFull);
    xCmp = fsl_cx_is_case_sensitive(f)
      ? fsl_strncmp
      : fsl_strnicmp_int;
    assert(zFull);
    assert(nFull>0);
    endsWithSlash = '/' == zFull[nFull-1];
    if( ((nFull==nLocalRoot-1 || (nFull==nLocalRoot && endsWithSlash))
         && xCmp(zLocalRoot, zFull, nFull)==0)
        || (nFull==1 && zFull[0]=='/' && nLocalRoot==1 && zLocalRoot[0]=='/') ){
      /* Special case.  zOrigName refers to zLocalRoot directory.

         Outputing "." instead of nothing is a historical decision
         which may be worth re-evaluating. Currently fsl_cx_stat() relies
         on it.
      */
      if(pOut){
        char const * zOut;
        fsl_size_t nOut;
        if(endsWithSlash){ /* retain trailing slash */
          zOut = "./";
          nOut = 2;
        }else{
          zOut = ".";
          nOut = 1;
        };
        rc = fsl_buffer_append(pOut, zOut, nOut);
      }else{
        rc = 0;
      }
      goto end;
    }

    if( nFull<=nLocalRoot || xCmp(zLocalRoot, zFull, nLocalRoot) ){
      rc = fsl_cx_err_set(f, FSL_RC_RANGE,
                          "File is outside of checkout tree: %s",
                          zOrigName);
      goto end;
    }

    if(pOut){
      rc = fsl_buffer_append(pOut, zFull + nLocalRoot, nFull - nLocalRoot);
    }

    end:
    fsl_cx_scratchpad_yield(f, full);
  }
  return rc;
}


/**
    Returns a fsl_ckout_change_e value for the given
    fsl_vfile_change_e value.

    Why are these not consolidated into one enum?  2021-03-13: because
    there are more checkout-level change codes than vfile-level
    changes. We could still consolidate them, giving the vfile changes
    their hard-coded values and leaving room in the enum for upward
    growth of that set.
*/
static fsl_ckout_change_e fsl_vfile_to_ckout_change(int vChange){
  switch((fsl_vfile_change_e)vChange){
#define EE(X) case FSL_VFILE_CHANGE_##X: return FSL_CKOUT_CHANGE_##X
    EE(NONE);
    EE(MOD);
    EE(MERGE_MOD);
    EE(MERGE_ADD);
    EE(INTEGRATE_MOD);
    EE(INTEGRATE_ADD);
    EE(IS_EXEC);
    EE(BECAME_SYMLINK);
    EE(NOT_EXEC);
    EE(NOT_SYMLINK);
#undef EE
    default:
       assert(!"Unhandled fsl_vfile_change_e value!");
      return FSL_CKOUT_CHANGE_NONE;
  }
}

int fsl_ckout_changes_visit( fsl_cx * f, fsl_id_t vid,
                             bool doScan,
                             fsl_ckout_changes_f visitor,
                             void * state ){
  int rc;
  fsl_db * db;
  fsl_stmt st = fsl_stmt_empty;
  int count = 0;
  fsl_ckout_change_e coChange;
  fsl_fstat fstat;
  if(!f || !visitor) return FSL_RC_MISUSE;
  db = fsl_needs_ckout(f);
  if(!db) return FSL_RC_NOT_A_CKOUT;
  if(vid<0){
    vid = f->ckout.rid;
    assert(vid>=0);
  }
  if(doScan){
    rc = fsl_vfile_changes_scan(f, vid, 0);
    if(rc) goto end;
  }
  rc = fsl_db_prepare(db, &st,
                      "SELECT chnged, deleted, rid, "
                      "pathname, origname "
                      "FROM vfile WHERE vid=%" FSL_ID_T_PFMT
                      " /*%s()*/",
                      vid,__func__);
  assert(!rc);
  while( FSL_RC_STEP_ROW == fsl_stmt_step(&st) ){
    int const changed = fsl_stmt_g_int32(&st, 0);
    int const deleted = fsl_stmt_g_int32(&st,1);
    fsl_id_t const vrid = fsl_stmt_g_id(&st,2);
    char const * name;
    char const * oname = NULL;
    name = fsl_stmt_g_text(&st, 3, NULL);
    oname = fsl_stmt_g_text(&st,4,NULL);
    if(oname && (0==fsl_strcmp(name, oname))){
      /* Work around a fossil oddity which sets origname=pathname
         during a 'mv' operation.
      */
      oname = NULL;
    }
    coChange = FSL_CKOUT_CHANGE_NONE;
    if(deleted){
      coChange = FSL_CKOUT_CHANGE_REMOVED;
    }else if(0==vrid){
      coChange = FSL_CKOUT_CHANGE_ADDED;
    }else if(!changed && NULL != oname){
      /* In fossil ^^, the "changed" state trumps the "renamed" state
       for status view purposes, so we'll do that here. */
      coChange = FSL_CKOUT_CHANGE_RENAMED;
    }else{
      fstat = fsl_fstat_empty;
      if( fsl_cx_stat(f, false, name, &fstat ) ){
        coChange = FSL_CKOUT_CHANGE_MISSING;
        fsl_cx_err_reset(f) /* keep FSL_RC_NOT_FOUND from bubbling
                               up to the client! */;
      }else if(!changed){
        continue;
      }else{
        coChange = fsl_vfile_to_ckout_change(changed);
      }
    }
    if(!coChange){
      MARKER(("INTERNAL ERROR: unhandled vfile.chnged "
              "value %d for file [%s]\n",
              changed, name));
      continue;
    }
    ++count;
    rc = visitor(state, coChange, name, oname);
    if(rc){
      if(FSL_RC_BREAK==rc){
        rc = 0;
        break;
      }else if(!f->error.code && (FSL_RC_OOM!=rc)){
        fsl_cx_err_set(f, rc, "Error %s returned from changes callback.",
                       fsl_rc_cstr(rc));
      }
      break;
    }
  }
  end:
  fsl_stmt_finalize(&st);
  if(rc && db->error.code && !f->error.code){
    fsl_cx_uplift_db_error(f, db);
  }

  return rc;
}

static bool fsl_co_is_in_vfile(fsl_cx *f,
                               char const *zFilename){
  return fsl_db_exists(fsl_cx_db_ckout(f),
                       "SELECT 1 FROM vfile"
                       " WHERE vid=%"FSL_ID_T_PFMT
                       " AND pathname=%Q %s",
                       f->ckout.rid, zFilename,
                       fsl_cx_filename_collation(f));
}
/**
   Internal machinery for fsl_ckout_manage(). zFilename MUST
   be a checkout-relative file which is known to exist. fst MUST
   be an object populated by fsl_stat()'ing zFilename. isInVFile
   MUST be the result of having passed zFilename to fsl_co_is_in_vfile().
 */
static int fsl_ckout_manage_impl( fsl_cx * f, char const *zFilename,
                                       fsl_fstat const *fst,
                                       bool isInVFile){
  int rc = 0;
  fsl_db * const db = fsl_needs_ckout(f);
  assert(fsl_is_simple_pathname(zFilename, true));
  if( isInVFile ){
    rc = fsl_db_exec(db, "UPDATE vfile SET deleted=0,"
                     " mtime=%"PRIi64
                     " WHERE vid=%"FSL_ID_T_PFMT
                     " AND pathname=%Q %s",
                     (int64_t)fst->mtime,
                     f->ckout.rid, zFilename,
                     fsl_cx_filename_collation(f));
  }else{
    int const chnged = FSL_VFILE_CHANGE_MOD
      /* fossil(1) sets chnged=0 on 'add'ed vfile records, but then the 'status'
         command updates the field to 1. To avoid down-stream inconsistencies
         (such as the ones which lead me here), we'll go ahead and set it to
         1 here.
      */;
    rc = fsl_db_exec(db,
                     "INSERT INTO "
                     "vfile(vid,chnged,deleted,rid,mrid,pathname,isexe,islink,mtime)"
                     "VALUES(%"FSL_ID_T_PFMT",%d,0,0,0,%Q,%d,%d,%"PRIi64")",
                     f->ckout.rid, chnged, zFilename,
                     (FSL_FSTAT_PERM_EXE==fst->perm) ? 1 : 0,
                     (FSL_FSTAT_TYPE_LINK==fst->type) ? 1 : 0,
                     (int64_t)fst->mtime
                     );
  }
  if(rc) rc = fsl_cx_uplift_db_error2(f, db, rc);
  return rc;
}

/**
   Internal state for the recursive file-add process.
*/
struct CoAddState {
  fsl_cx * f;
  fsl_ckout_manage_opt * opt;
  fsl_buffer * absBuf; // absolute path of file to check
  fsl_buffer * coRelBuf; // checkout-relative path of absBuf
  fsl_fstat fst; // fsl_stat() state of absBuf's file
};
typedef struct CoAddState CoAddState;
static const CoAddState CoAddState_empty =
  {NULL, NULL, NULL, NULL, fsl_fstat_empty_m};

/**
   fsl_dircrawl_f() impl for recursively adding files to a
   repo. state must be a (CoAddState*)/
*/
static int fsl_dircrawl_f_add(fsl_dircrawl_state const *);

/**
   Attempts to add file or directory (recursively) cas->absBuf to the
   current repository. isCrawling must be true if this is a
   fsl_dircrawl()-invoked call, else false.
*/
static int co_add_one(CoAddState * cas, bool isCrawling){
  int rc = 0;
  fsl_buffer_reuse(cas->coRelBuf);
  rc = fsl_cx_stat2(cas->f, cas->opt->relativeToCwd,
                    fsl_buffer_cstr(cas->absBuf), &cas->fst,
                    fsl_buffer_reuse(cas->coRelBuf), false)
    /* Reminder: will fail if file is outside of the checkout tree */;
  if(rc) return rc;
  switch(cas->fst.type){
    case FSL_FSTAT_TYPE_FILE:{
      bool skipped = false;
      char const * zCoRel = fsl_buffer_cstr(cas->coRelBuf);
      bool const isInVFile = fsl_co_is_in_vfile(cas->f, zCoRel);
      if(!isInVFile){
        if(fsl_reserved_fn_check(cas->f, zCoRel,-1,false)){
          /* ^^^ we need to use fsl_reserved_fn_check(), instead of
             fsl_is_reserved_fn(), so that we will inherit any
             new checks which require a context object. If that
             check fails, though, it updates cas->f with an error
             message which we need to suppress here to avoid it
             accidentally propagating and causing downstream
             confusion. */
          fsl_cx_err_reset(cas->f);
          skipped = true;
        }else if(cas->opt->checkIgnoreGlobs){
          char const * m =
            fsl_cx_glob_matches(cas->f, FSL_GLOBS_IGNORE, zCoRel);
          if(m) skipped = true;
        }
        if(!skipped && cas->opt->callback){
          bool yes = false;
          rc = cas->opt->callback( zCoRel, &yes,
                                   cas->opt->callbackState );
          if(rc) goto end;
          else if(!yes) skipped = true;
        }
      }
      if(skipped){
        ++cas->opt->counts.skipped;
      }else{
        rc = fsl_ckout_manage_impl(cas->f, zCoRel, &cas->fst,
                                        isInVFile);
        if(!rc){
          if(isInVFile) ++cas->opt->counts.updated;
          else ++cas->opt->counts.added;
        }
      }
      break;
    }
    case FSL_FSTAT_TYPE_DIR:
      if(!isCrawling){
        /* Reminder to self: fsl_dircrawl() copies its first argument
           for canonicalizing it, so this is safe even though
           cas->absBuf may be reallocated during the recursive
           call. We're done with these particular contents of
           cas->absBuf at this point. */
        rc = fsl_dircrawl(fsl_buffer_cstr(cas->absBuf),
                          fsl_dircrawl_f_add, cas);
        if(rc && !cas->f->error.code){
          rc = fsl_cx_err_set(cas->f, rc, "fsl_dircrawl() returned %s.",
                              fsl_rc_cstr(rc));
        }
      }else{
        assert(!"Cannot happen - caught higher up");
        fsl_fatal(FSL_RC_ERROR, "Internal API misuse in/around %s().",
                  __func__);
      }
      break;
    default:
      rc = fsl_cx_err_set(cas->f, FSL_RC_TYPE,
                          "Unhandled filesystem entry type: "
                          "fsl_fstat_type_e #%d", cas->fst.type);
      break;
  }
  end:
  return rc;
}

static int fsl_dircrawl_f_add(fsl_dircrawl_state const *dst){
  if(FSL_FSTAT_TYPE_FILE!=dst->entryType) return 0;
  CoAddState * cas = (CoAddState*)dst->callbackState;
  int rc = fsl_buffer_appendf(fsl_buffer_reuse(cas->absBuf),
                              "%s/%s", dst->absoluteDir, dst->entryName);
  if(!rc) rc = co_add_one(cas, true);
  return rc;
}

int fsl_ckout_manage( fsl_cx * f, fsl_ckout_manage_opt * opt_ ){
  int rc = 0;
  CoAddState cas = CoAddState_empty;
  fsl_ckout_manage_opt opt;
  if(!f) return FSL_RC_MISUSE;
  else if(!fsl_needs_ckout(f)) return FSL_RC_NOT_A_CKOUT;
  assert(f->ckout.rid>=0);
  opt = *opt_
    /*use a copy in case the user manages to modify
      opt_ from a callback. */;
  cas.absBuf = fsl_cx_scratchpad(f);
  cas.coRelBuf = fsl_cx_scratchpad(f);
  rc = fsl_file_canonical_name(opt.filename, cas.absBuf, false);
  if(!rc){
    cas.f = f;
    cas.opt = &opt;
    rc = co_add_one(&cas, false);
    opt_->counts = opt.counts;
  }
  fsl_cx_scratchpad_yield(f, cas.absBuf);
  fsl_cx_scratchpad_yield(f, cas.coRelBuf);
  return rc;
}

/**
   Creates, if needed, a TEMP TABLE named [tableName] with a single
   [id] field and populates it with all ids from the given bag.

   Returns 0 on success, any number of non-0 codes on error.
*/
static int fsl_ckout_bag_to_ids(fsl_cx *f, fsl_db * db,
                                char const * tableName,
                                fsl_id_bag const * bag){
  fsl_stmt insId = fsl_stmt_empty;
  int rc = fsl_db_exec_multi(db,
                             "CREATE TEMP TABLE IF NOT EXISTS "
                             "[%s](id); "
                             "DELETE FROM [%s] /* %s() */;",
                             tableName, tableName, __func__);
  if(rc) goto dberr;
  rc = fsl_db_prepare(db, &insId,
                      "INSERT INTO [%s](id) values(?1) "
                      "/* %s() */", tableName, __func__);
  if(rc) goto dberr;
  for(fsl_id_t e = fsl_id_bag_first(bag);
      e; e = fsl_id_bag_next(bag, e)){
    fsl_stmt_bind_id(&insId, 1, e);
    rc = fsl_stmt_step(&insId);
    switch(rc){
      case FSL_RC_STEP_DONE:
        rc = 0;
        break;
      default:
        fsl_stmt_finalize(&insId);
        goto dberr;
    }
    fsl_stmt_reset(&insId);
  }
  assert(!rc);
  end:
  fsl_stmt_finalize(&insId);
  return rc;
  dberr:
  assert(rc);
  rc = fsl_cx_uplift_db_error2(f, db, rc);
  goto end;
}

int fsl_ckout_unmanage(fsl_cx * f, fsl_ckout_unmanage_opt const * opt){
  int rc;
  fsl_db * const db = fsl_needs_ckout(f);
  fsl_buffer * fname = 0;
  fsl_id_t const vid = f->ckout.rid;
  fsl_stmt q = fsl_stmt_empty;
  bool inTrans = false;
  if(!db) return FSL_RC_NOT_A_CKOUT;
  else if((!opt->filename || !*opt->filename)
          && !opt->vfileIds){
    return fsl_cx_err_set(f, FSL_RC_MISUSE,
                          "Empty file set is not legal for %s()",
                          __func__);
  }
  assert(vid>=0);
  rc = fsl_db_transaction_begin(db);
  if(rc) goto dberr;
  inTrans = true;
  if(opt->vfileIds){
    rc = fsl_ckout_bag_to_ids(f, db, "fx_unmanage_id", opt->vfileIds);
    if(rc) goto end;
    rc = fsl_db_exec(db,
                     "UPDATE vfile SET deleted=1 "
                     "WHERE vid=%" FSL_ID_T_PFMT " "
                     "AND NOT deleted "
                     "AND id IN fx_unmanage_id /* %s() */",
                     vid, __func__);
    if(rc) goto dberr;
    if(opt->callback){
      rc = fsl_db_prepare(db,&q,
                          "SELECT pathname FROM vfile "
                          "WHERE vid=%" FSL_ID_T_PFMT " "
                          "AND deleted "
                          "AND id IN fx_unmanage_id "
                          "/* %s() */",
                          vid, __func__);
      if(rc) goto dberr;
    }
  }else{// Process opt->filename
    fname = fsl_cx_scratchpad(f);
    rc = fsl_ckout_filename_check(f, opt->relativeToCwd,
                                  opt->filename, fname);
    if(rc) goto end;
    char const * zNorm = fsl_buffer_cstr(fname);
    /* MARKER(("fsl_ckout_unmanage(%d, %s) ==> %s\n", relativeToCwd, zFilename, zNorm)); */
    assert(zNorm);
    if(fname->used){
      fsl_buffer_strip_slashes(fname);
      if(1==fname->used && '.'==*zNorm){
        /* Special case: handle "." from ckout root intuitively */
        fsl_buffer_reuse(fname);
        assert(0==*zNorm);
      }
    }
    rc = fsl_db_exec(db,
                     "UPDATE vfile SET deleted=1 "
                     "WHERE vid=%" FSL_ID_T_PFMT " "
                     "AND NOT deleted "
                     "AND CASE WHEN %Q='' THEN 1 "
                     "ELSE fsl_match_vfile_or_dir(pathname,%Q) "
                     "END /*%s()*/",
                     vid, zNorm, zNorm, __func__);
    if(rc) goto dberr;
    if(opt->callback){
      rc = fsl_db_prepare(db,&q,
                          "SELECT pathname FROM vfile "
                          "WHERE vid=%" FSL_ID_T_PFMT " "
                          "AND deleted "
                          "AND CASE WHEN %Q='' THEN 1 "
                          "ELSE fsl_match_vfile_or_dir(pathname,%Q) "
                          "END "
                          "UNION "
                          "SELECT pathname FROM vfile "
                          "WHERE vid=%" FSL_ID_T_PFMT " "
                          "AND rid=0 AND deleted "
                          "/*%s()*/",
                          vid, zNorm, zNorm, vid, __func__);
      if(rc) goto dberr;
    }
  }/*opt->filename*/

  if(q.stmt){
    while(FSL_RC_STEP_ROW==fsl_stmt_step(&q)){
      char const * fn = fsl_stmt_g_text(&q, 0, NULL);
      rc = opt->callback(fn, opt->callbackState);
      if(rc) goto end;
    }
    fsl_stmt_finalize(&q);
  }
  /* Remove rm'd ADDed-but-not-yet-committed entries... */
  rc = fsl_db_exec(db,
                   "DELETE FROM vfile WHERE vid=%" FSL_ID_T_PFMT
                   " AND rid=0 AND deleted",
                   vid);
  if(rc) goto dberr;
  end:
  if(fname) fsl_cx_scratchpad_yield(f, fname);
  fsl_stmt_finalize(&q);
  if(opt->vfileIds){
    fsl_db_exec(db, "DROP TABLE IF EXISTS fx_unmanage_id /* %s() */",
                __func__)
      /* Ignoring result code */;
  }
  if(inTrans){
    int const rc2 = fsl_db_transaction_end(db, !!rc);
    if(!rc) rc = rc2;
  }
  return rc;
  dberr:
  assert(rc);
  rc = fsl_cx_uplift_db_error2(f, db, rc);
  goto end;

}

int fsl_ckout_changes_scan(fsl_cx * f){
  return fsl_vfile_changes_scan(f, -1, 0);
}

int fsl_ckout_install_schema(fsl_cx *f, bool dropIfExists){
  char const * tNames[] = {
  "vvar", "vfile", "vmerge", 0
  };
  int rc;
  fsl_db * const db = fsl_needs_ckout(f);
  if(!db) return f->error.code;
  if(dropIfExists){
    char const * t;
    int i;
    char const * dbName = fsl_db_role_label(FSL_DBROLE_CKOUT);
    for(i=0; 0!=(t = tNames[i]); ++i){
      rc = fsl_db_exec(db, "DROP TABLE IF EXISTS %s.%s /*%s()*/",
                       dbName, t, __func__);
      if(rc) break;
    }
    if(!rc){
      rc = fsl_db_exec(db, "DROP TRIGGER IF EXISTS "
                       "%s.vmerge_ck1 /*%s()*/",
                       dbName, __func__);
    }
  }else{
    if(fsl_db_table_exists(db, FSL_DBROLE_CKOUT,
                           tNames[0])){
      return 0;
    }
  }
  rc = fsl_db_exec_multi(db, "%s", fsl_schema_ckout());
  return fsl_cx_uplift_db_error2(f, db, rc);
}

bool fsl_ckout_has_changes(fsl_cx *f){
  fsl_db * const db = fsl_cx_db_ckout(f);
  if(!db) return false;
  return fsl_db_exists(db,
                       "SELECT 1 FROM vfile WHERE chnged "
                       "OR coalesce(origname != pathname, 0) "
                       "/*%s()*/", __func__)
    || fsl_db_exists(db,"SELECT 1 FROM vmerge /*%s()*/", __func__);
}

int fsl_ckout_clear_merge_state( fsl_cx *f ){
  fsl_db * const d = fsl_needs_ckout(f);
  int rc;
  if(d){
    rc = fsl_db_exec(d,"DELETE FROM vmerge /*%s()*/", __func__);
    rc = fsl_cx_uplift_db_error2(f, d, rc);
  }else{
    rc = FSL_RC_NOT_A_CKOUT;
  }
  return rc;
}

int fsl_ckout_clear_db(fsl_cx *f){
  fsl_db * const db = fsl_needs_ckout(f);
  if(!db) return f->error.code;
  return fsl_db_exec_multi(db,
                           "DELETE FROM vfile;"
                           "DELETE FROM vmerge;"
                           "DELETE FROM vvar WHERE name IN"
                           "('checkout','checkout-hash') "
                           "/*%s()*/", __func__);
}

fsl_db * fsl_cx_db_for_role(fsl_cx *, fsl_dbrole_e)
  /* defined in cx.c */;

/**
   Updates f->ckout.dir and dirLen based on the current state of
   f->ckout.db. Returns 0 on success, FSL_RC_OOM on allocation error,
   some other code if canonicalization of the name fails
   (e.g. filesystem error or cwd cannot be resolved).
*/
static int fsl_update_ckout_dir(fsl_cx *f){
  int rc;
  fsl_buffer ckDir = fsl_buffer_empty;
  fsl_db * dbC = fsl_cx_db_for_role(f, FSL_DBROLE_CKOUT);
  assert(dbC->filename);
  assert(*dbC->filename);
  rc = fsl_file_canonical_name(dbC->filename, &ckDir, false);
  if(rc) return rc;
  char * zCanon = fsl_buffer_take(&ckDir);
  //MARKER(("dbC->filename=%s\n", dbC->filename));
  //MARKER(("zCanon=%s\n", zCanon));
  rc = fsl_file_dirpart(zCanon, -1, &ckDir, true);
  fsl_free(zCanon);
  if(rc){
    fsl_buffer_clear(&ckDir);
  }else{
    fsl_free(f->ckout.dir);
    f->ckout.dirLen = ckDir.used;
    f->ckout.dir = fsl_buffer_take(&ckDir);
    assert('/'==f->ckout.dir[f->ckout.dirLen-1]);
    /*MARKER(("Updated ckout.dir: %d %s\n",
      (int)f->ckout.dirLen, f->ckout.dir));*/
  }
  return rc;
}


int fsl_repo_open_ckout(fsl_cx *f, const fsl_repo_open_ckout_opt *opt){
  fsl_db *dbC = 0;
  fsl_buffer *cwd = 0;
  int rc = 0;

  if(!opt) return FSL_RC_MISUSE;
  else if(!fsl_needs_repo(f)){
    return f->error.code;
  }else if(fsl_cx_db_ckout(f)){
    return fsl_cx_err_set(f, FSL_RC_MISUSE,
                          "A checkout is already attached.");
  }
  if(opt->targetDir && *opt->targetDir){
    if(fsl_chdir(opt->targetDir)){
      return fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                            "Directory not found or inaccessible: %s",
                            opt->targetDir);
    }
  }
  cwd = fsl_cx_scratchpad(f);
  assert(!cwd->used);
  if((rc = fsl_cx_getcwd(f, cwd))){
    assert(!cwd->used);
    fsl_cx_scratchpad_yield(f, cwd);
    return fsl_cx_err_set(f, rc, "Error %d [%s]: unable to "
                          "determine current directory.",
                          rc, fsl_rc_cstr(rc));
  }
  /**
     AS OF HERE: do not use 'return'. Use goto end so that we can
     chdir() back to our original cwd!
  */
  if(!fsl_dir_is_empty("."/*we've already chdir'd if
                            we were going to*/)) {
    switch(opt->fileOverwritePolicy){
      case FSL_OVERWRITE_ALWAYS:
      case FSL_OVERWRITE_NEVER: break;
      default:
        assert(FSL_OVERWRITE_ERROR==opt->fileOverwritePolicy);
        rc = fsl_cx_err_set(f, FSL_RC_ACCESS,
                            "Directory is not empty and "
                            "fileOverwritePolicy is "
                            "FSL_OVERWRITE_ERROR: "
                            "%b", cwd);
        goto end;
    }
  }
  if(opt->checkForOpenedCkout){
    /* Check target and parent dirs for a checkout and bail out if we
       find one. If opt->checkForOpenedCkout is false then we will use
       the dbOverwritePolicy to determine what to do if we find a
       checkout db in cwd (as opposed to a parent). */
    fsl_buffer * const foundAt = fsl_cx_scratchpad(f);
    if (!fsl_ckout_db_search(fsl_buffer_cstr(cwd), true, foundAt)) {
      rc = fsl_cx_err_set(f, FSL_RC_ALREADY_EXISTS,
                          "There is already a checkout db at %b",
                          foundAt);
    }
    fsl_cx_scratchpad_yield(f, foundAt);
    if(rc) goto end;
  }

  /**
     Create and attach ckout db...
  */
  assert(!fsl_cx_db_ckout(f));
  const char * dbName = opt->ckoutDbFile
    ? opt->ckoutDbFile : fsl_preferred_ckout_db_name();
  fsl_cx_err_reset(f);
  int fsl_cx_attach_role(fsl_cx *, const char *, fsl_dbrole_e)
    /* defined in cx.c */;
  rc = fsl_cx_attach_role(f, dbName, FSL_DBROLE_CKOUT);
  if(rc) goto end;
  fsl_db * const theDbC = fsl_cx_db_ckout(f);
  dbC = fsl_cx_db_for_role(f, FSL_DBROLE_CKOUT);
  assert(theDbC != dbC && "Not anymore.");
  assert(theDbC == f->dbMain);
  assert(!f->error.code);
  assert(dbC->name);
  assert(dbC->filename);
  rc = fsl_ckout_install_schema(f, opt->dbOverwritePolicy);
  if(!rc){
    rc = fsl_db_exec(theDbC,"INSERT OR IGNORE INTO "
                     "%s.vvar (name,value) "
                     "VALUES('checkout',0),"
                     "('checkout-hash',null)",
                     dbC->name);
  }
  if(rc) rc = fsl_cx_uplift_db_error(f, theDbC);
  end:
  if(opt->targetDir && *opt->targetDir && cwd->used){
    fsl_chdir(fsl_buffer_cstr(cwd))
      /* Ignoring error because we have no recovery strategy! */;
  }
  fsl_cx_scratchpad_yield(f, cwd);
  if(!rc){
    fsl_db * const dbR = fsl_cx_db_for_role(f, FSL_DBROLE_REPO);
    assert(dbR);
    assert(dbR->filename && *dbR->filename);
    rc = fsl_config_set_text(f, FSL_CONFDB_CKOUT, "repository",
                             dbR->filename);
  }
  if(!rc) rc = fsl_update_ckout_dir(f);
  fsl_buffer_clear(cwd);
  return rc;
}

int fsl_is_locally_modified(fsl_cx * f, const char * zFilename,
                            fsl_size_t origSize,
                            const char * zOrigHash,
                            fsl_int_t zOrigHashLen,
                            fsl_fileperm_e origPerm,
                            int * isModified){
  int rc = 0;
  int const hashLen = zOrigHashLen>=0
    ? zOrigHashLen : fsl_is_uuid(zOrigHash);
  fsl_buffer * hash = 0;
  fsl_buffer * fname = fsl_cx_scratchpad(f);
  fsl_fstat * const fst = &f->cache.fstat;
  int mod = 0;
  if(!fsl_is_uuid_len(hashLen)){
    return fsl_cx_err_set(f, FSL_RC_RANGE, "%s(): invalid hash length "
                          "%d for file: %s", __func__, hashLen, zFilename);
  }else if(!f->ckout.dir){
    return fsl_cx_err_set(f, FSL_RC_NOT_A_CKOUT,
                          "%s() requires a checkout.", __func__);
  }
  if(!fsl_is_absolute_path(zFilename)){
    rc = fsl_file_canonical_name2(f->ckout.dir, zFilename, fname, false);
    if(rc) goto end;
    zFilename = fsl_buffer_cstr(fname);
  }
  rc = fsl_stat(zFilename, fst, false);
  if(0==rc){
    if(origSize!=fst->size){
      mod |= 0x02;
    }
    if((FSL_FILE_PERM_EXE==origPerm &&
        FSL_FSTAT_PERM_EXE!=fst->perm)
       || (FSL_FILE_PERM_EXE!=origPerm &&
           FSL_FSTAT_PERM_EXE==fst->perm)){
      mod |= 0x01;
    }else if((FSL_FILE_PERM_LINK==origPerm &&
              FSL_FSTAT_TYPE_LINK!=fst->type)
             || (FSL_FILE_PERM_LINK!=origPerm &&
                 FSL_FSTAT_TYPE_LINK==fst->type)){
      mod |= 0x04;
    }
    if(mod & 0x06) goto end;
    /* ^^^^^^^^^^ else we unfortunately need, for behavioral
       consistency, to fall through and determine whether the file
       contents differ. */
  }else{
    if(FSL_RC_NOT_FOUND==rc){
      rc = 0;
      mod = 0x10;
    }else{
      rc = fsl_cx_err_set(f, rc, "%s(): stat() failed for file: %s",
                          __func__, zFilename);
    }
    goto end;
  }
  hash = fsl_cx_scratchpad(f);
  switch(hashLen){
    case FSL_STRLEN_SHA1:
      rc = fsl_sha1sum_filename(zFilename, hash);
      break;
    case FSL_STRLEN_K256:
      rc = fsl_sha3sum_filename(zFilename, hash);
      break;
    default:
      fsl_fatal(FSL_RC_UNSUPPORTED, "This cannot happen. %s()",
                __func__);
  }
  if(rc){
    rc = fsl_cx_err_set(f, rc, "%s: error hashing file: %s",
                        __func__, zFilename);
  }else{
    assert(hashLen==(int)hash->used);
    mod |= memcmp(hash->mem, zOrigHash, (size_t)hashLen)
      ? 0x02 : 0;
    /*MARKER(("%d: %s %s %s\n", *isModified, zOrigHash,
      (char const *)hash.mem, zFilename));*/
  }
  end:
  if(!rc && isModified) *isModified = mod;
  fsl_cx_scratchpad_yield(f, fname);
  if(hash) fsl_cx_scratchpad_yield(f, hash);
  return rc;
}

/**
   Infrastructure for fsl_repo_ckout() and
   fsl_ckout_update().
*/
typedef struct {
  /** The pre-checkout vfile.vid. 0 if no version was
      checked out. */
  fsl_id_t originRid;
  fsl_repo_extract_opt const * eOpt;
  fsl_ckup_opt const * cOpt;
  /* Checkout root. We re-use this when internally converting to
     absolute paths. */
  fsl_buffer * tgtDir;
  /* Initial length of this->tgtDir, including trailing slash */
  fsl_size_t tgtDirLen;
  /* Number of files we've written out so far. Used for adapting
     some error reporting. */
  fsl_size_t fileWriteCount;
  /* Stores the most recent fsl_cx_confirm() answer for questions
     about overwriting/removing modified files. (Exactly which answer
     it represents depends on the current phase of processing.)
  */
  fsl_confirm_response confirmAnswer;
  /* Is-changed vis-a-vis vfile query. */
  fsl_stmt stChanged;
  /* Is-same-filename-and-rid-in-vfile query. */
  fsl_stmt stIsInVfile;
  /* blob.size for vfile.rid query. */
  fsl_stmt stRidSize;
} RepoExtractCkup;

static const RepoExtractCkup RepoExtractCkup_empty = {
0/*originRid*/,NULL/*eOpt*/, NULL/*cOpt*/,
NULL/*tgtDir*/, 0/*tgtDirLen*/,
0/*fileWriteCount*/,
fsl_confirm_response_empty_m/*confirmAnswer*/,
fsl_stmt_empty_m/*stChanged*/,
fsl_stmt_empty_m/*stIsInVfile*/,
fsl_stmt_empty_m/*stRidSize*/
};

static const fsl_ckup_state fsl_ckup_state_empty = {
NULL/*xState*/, NULL/*callbackState*/,
FSL_CKUP_FCHANGE_INVALID/*fileChangeType*/,
FSL_CKUP_RM_NOT/*fileRmInfo*/,
0/*mtime*/,0/*size*/,
false/*dryRun*/
};

/**
   File modification types reported by
   fsl_reco_is_file_modified().
 */
typedef enum {
// Sentinel value
FSL_RECO_MOD_UNKNOWN,
// Not modified
FSL_RECO_MOD_NO,
// Modified
FSL_RECO_MOD_YES,
// "Unmanaged replaced by managed"
FSL_RECO_MOD_UnReMa
} fsl_ckup_localmod_e;

/**
   Determines whether the file referred to by the given
   checkout-root-relative file name, which is assumed to be known to
   exist, has been modified. It simply looks to the vfile state,
   rather than doing its own filesystem-level comparison. Returns 0 on
   success and stores its answer in *modType. Errors must be
   considered unrecoverable.
*/
static int fsl_reco_is_file_modified(fsl_cx *f, fsl_stmt * st,
                                     char const *zName,
                                     fsl_ckup_localmod_e * modType){
  int rc = 0;
  if(!st->stmt){ // no prior version
    *modType = FSL_RECO_MOD_NO;
    return 0;
  }
  fsl_stmt_reset(st);
  rc = fsl_stmt_bind_text(st, 1, zName, -1, false);
  if(rc){
    return fsl_cx_uplift_db_error2(f, st->db, rc);
  }
  rc = fsl_stmt_step(st);
  switch(rc){
    case FSL_RC_STEP_DONE:
      /* This can happen when navigating from a version in which a
         file was SCM-removed/unmanaged, but on disk, to a version
         where that file was in SCM. For now we'll mark these as
         modified but we need a better way of handling this case, and
         maybe a new FSL_CEVENT_xxx ID. */
      *modType = FSL_RECO_MOD_UnReMa;
      rc = 0;
      break;
    case FSL_RC_STEP_ROW:
      *modType = fsl_stmt_g_int32(st,0)>0
        ? FSL_RECO_MOD_YES : FSL_RECO_MOD_NO;
      rc = 0;
      break;
    default:
      rc = fsl_cx_uplift_db_error2(f, st->db, rc);
      break;
  }
  return rc;
}

/**
   Sets *isInVfile to true if the given combination of filename and
   file content RID are in the vfile table, as per
   RepoExtractCkup::stIsInVfile, else false. Returns non-0 on
   catastrophic failure.
*/
static int fsl_repo_co_is_in_vfile(fsl_stmt * st,
                                   char const *zFilename,
                                   fsl_id_t fileRid,
                                   bool *isInVfile){
  int rc = 0;
  if(st->stmt){
    fsl_stmt_reset(st);
    rc = fsl_stmt_bind_text(st, 1, zFilename, -1, false);
    if(!rc) rc = fsl_stmt_bind_id(st, 2, fileRid);
    if(!rc) *isInVfile = (FSL_RC_STEP_ROW==fsl_stmt_step(st));
  }else{ // no prior version
    *isInVfile = false;
  }
  return rc;
}

/**
   Infrastructure for fsl_repo_ckout(). This is the fsl_repo_extract_f
   impl which fsl_repo_extract() calls to give us the pieces we want to
   check out.

   When this is run (once for each row of the new checkout version),
   the vfile table still holds the state for the previous version, and
   we use that to determine whether a file is changed or new.
*/
static int fsl_repo_extract_f_ckout( fsl_repo_extract_state const * xs ){
  int rc = 0;
  fsl_cx * const f = xs->f;
  RepoExtractCkup * const rec = (RepoExtractCkup *)xs->callbackState;
  const char * zFilename;
  fsl_ckup_state coState = fsl_ckup_state_empty;
  fsl_time_t mtime = 0;
  fsl_fstat fst = fsl_fstat_empty;
  fsl_ckup_localmod_e modType = FSL_RECO_MOD_UNKNOWN;
  bool loadedContent = false;
  fsl_buffer * content = &f->fileContent;
  assert(0==content->used
         && "Internal Misuse of fsl_cx::fileContent buffer.");
  //assert(xs->content);
  assert(xs->fCard->uuid && "We shouldn't be getting deletions "
         "via delta manifests.");
  rc = fsl_buffer_append(rec->tgtDir, xs->fCard->name, -1);
  if(rc) return rc;
  fsl_buffer_reuse(content);
  coState.dryRun = rec->cOpt->dryRun;
  coState.fileRmInfo = FSL_CKUP_RM_NOT;
  coState.fileChangeType = FSL_CKUP_FCHANGE_INVALID;
  zFilename = fsl_buffer_cstr(rec->tgtDir);
  rc = fsl_stat(zFilename, &fst, 0);
  switch(rc){
    case 0:
      /* File exists. If it is modified, as reported by vfile, get
         confirmation before overwriting it, otherwise just overwrite
         it (or keep it - that's much more efficient). */
      mtime = fst.mtime;
      if(rec->confirmAnswer.response!=FSL_CRESPONSE_ALWAYS){
        rc = fsl_reco_is_file_modified(f, &rec->stChanged,
                                       xs->fCard->name, &modType);
        if(rc) goto end;
        switch(modType){
          case FSL_RECO_MOD_YES:
          case FSL_RECO_MOD_UnReMa:
            if(rec->confirmAnswer.response!=FSL_CRESPONSE_NEVER){
              fsl_confirm_detail detail = fsl_confirm_detail_empty;
              detail.eventId = FSL_RECO_MOD_YES==modType
                ? FSL_CEVENT_OVERWRITE_MOD_FILE
                : FSL_CEVENT_OVERWRITE_UNMGD_FILE;
              detail.filename = xs->fCard->name;
              rec->confirmAnswer.response = FSL_CRESPONSE_INVALID;
              rc = fsl_cx_confirm(f, &detail, &rec->confirmAnswer);
              if(rc) goto end;
            }
            break;
          case FSL_RECO_MOD_NO:{
            /** If vfile says that the content of this exact
                combination of filename and file RID is unchanged, we
                already have this content. If so, skip rewriting
                it. */
            bool isSameFile = false;
            rc = fsl_repo_co_is_in_vfile(&rec->stIsInVfile, xs->fCard->name,
                                         xs->fileRid, &isSameFile);
            if(rc) goto end;
            rec->confirmAnswer.response = isSameFile
              ? FSL_CRESPONSE_NO // We already have this content
              : FSL_CRESPONSE_YES; // Overwrite it
            coState.fileChangeType = isSameFile
              ? FSL_CKUP_FCHANGE_NONE
              : FSL_CKUP_FCHANGE_UPDATED;
            break;
          }
          default:
            fsl_fatal(FSL_RC_UNSUPPORTED,"Internal error: invalid "
                      "fsl_reco_is_file_modified() response.");
        }
      }
      switch(rec->confirmAnswer.response){
        case FSL_CRESPONSE_NO:
        case FSL_CRESPONSE_NEVER:
          // Keep existing.
          coState.fileChangeType = FSL_CKUP_FCHANGE_NONE;
          goto do_callback;
        case FSL_CRESPONSE_YES:
        case FSL_CRESPONSE_ALWAYS:
          // Overwrite it.
          coState.fileChangeType = FSL_CKUP_FCHANGE_UPDATED;
          break;
        case FSL_CRESPONSE_CANCEL:
          rc = fsl_cx_err_set(f, FSL_RC_BREAK,
                              "Checkout operation cancelled by "
                              "confirmation callback.%s",
                              rec->fileWriteCount
                              ? " Filesystem contents may now be "
                                "in an inconsistent state!"
                              : "");
          goto end;
        default:
          rc = fsl_cx_err_set(f, FSL_RC_MISUSE,
                              "Invalid response from confirmation "
                              "callback.");
          goto end;
      }
      break;
    case FSL_RC_NOT_FOUND:
      rc = 0;
      coState.fileChangeType = FSL_CKUP_FCHANGE_UPDATED;
      // Write it
      break;
    default:
      rc = fsl_cx_err_set(f, rc, "Error %s stat()'ing file: %s",
                          fsl_rc_cstr(rc), zFilename);
      goto end;
  }
  assert(FSL_CKUP_FCHANGE_INVALID != coState.fileChangeType);
  if(coState.dryRun){
    mtime = time(0);
  }else{
    if((rc=fsl_mkdir_for_file(zFilename, true))){
      rc = fsl_cx_err_set(f, rc, "mkdir() failed for file: %s", zFilename);
      goto end;
    }
    assert(!xs->content);
    rc = fsl_card_F_content(f, xs->fCard, content);
    if(rc) goto end;
    else if((rc=fsl_buffer_to_filename(content, zFilename))){
      rc = fsl_cx_err_set(f, rc, "Error %s writing to file: %s",
                          fsl_rc_cstr(rc), zFilename);
      goto end;
    }else{
      loadedContent = true;
      ++rec->fileWriteCount;
      mtime = time(0);
    }
    rc = fsl_file_exec_set(zFilename,
                           FSL_FILE_PERM_EXE == xs->fCard->perm);
    if(rc){
      rc = fsl_cx_err_set(f, rc, "Error %s changing file permissions: %s",
                          fsl_rc_cstr(rc), xs->fCard->name);
      goto end;
    }
  }
  if(rec->cOpt->setMtime){
    rc = fsl_mtime_of_manifest_file(xs->f, xs->checkinRid,
                                    xs->fileRid, &mtime);
    if(rc) goto end;
    if(!coState.dryRun){
      rc = fsl_file_mtime_set(zFilename, mtime);
      if(rc){
        rc = fsl_cx_err_set(f, rc, "Error %s setting mtime of file: %s",
                            fsl_rc_cstr(rc), zFilename);
        goto end;
      }
    }
  }
  do_callback:
  assert(0==rc);
  if(rec->cOpt->callback){
    assert(mtime);
    coState.mtime = mtime;
    coState.extractState = xs;
    coState.callbackState = rec->cOpt->callbackState;
    if(loadedContent){
      coState.size = content->used;
    }else{
      fsl_stmt_reset(&rec->stRidSize);
      fsl_stmt_bind_id(&rec->stRidSize, 1, xs->fileRid);
      coState.size =
        (FSL_RC_STEP_ROW==fsl_stmt_step(&rec->stRidSize))
        ? (fsl_int_t)fsl_stmt_g_int64(&rec->stRidSize, 0)
        : -1;
    }
    rc = rec->cOpt->callback( &coState );
  }
  end:
  fsl_buffer_reuse(content);
  rec->tgtDir->used = rec->tgtDirLen;
  rec->tgtDir->mem[rec->tgtDirLen] = 0;
  return rc;
}

/**
   For each file in vfile(vid=rec->originRid) which is not in the
   current vfile(vid=rec->cOpt->checkinRid), remove it from disk (or
   not, depending on confirmer response). Afterwards, try to remove
   any dangling directories left by that removal.

   Returns 0 on success. Ignores any filesystem-level errors during
   removal because, frankly, we have no recovery strategy for that
   case.

   TODO: do not remove dirs from the 'empty-dirs' config setting.
*/
static int fsl_repo_ckout_rm_list_fini(fsl_cx * f,
                                       RepoExtractCkup * rec){
  int rc;
  fsl_db * db = fsl_cx_db_ckout(f);
  fsl_stmt q = fsl_stmt_empty;
  fsl_buffer * absPath = fsl_cx_scratchpad(f);
  fsl_size_t const ckdirLen = f->ckout.dirLen;
  char const *zAbs;
  int rmCounter = 0;
  fsl_ckup_opt const * cOpt = rec->cOpt;
  fsl_ckup_state cuState = fsl_ckup_state_empty;
  fsl_repo_extract_state rxState = fsl_repo_extract_state_empty;
  fsl_card_F fCard = fsl_card_F_empty;
  
  assert(db);
  rc = fsl_buffer_append(absPath, f->ckout.dir,
                         (fsl_int_t)f->ckout.dirLen);
  if(rc) goto end;
  /* Select files which were in the previous version
     (rec->originRid) but are not in the newly co'd version
     (cOpt->checkinRid). */
  rc = fsl_db_prepare(db, &q,
                      "SELECT "
                      /*0*/"v.rid frid,"
                      /*1*/"v.pathname fn,"
                      /*2*/"b.uuid,"
                      /*3*/"v.isexe,"
                      /*4*/"v.islink,"
                      /*5*/"v.chnged, "
                      /*6*/"b.size "
                      "FROM vfile v, blob b "
                      "WHERE v.vid=%" FSL_ID_T_PFMT " "
                      "AND v.rid=b.rid "
                      "AND fn NOT IN "
                      "(SELECT pathname FROM vfile "
                      " WHERE vid=%" FSL_ID_T_PFMT
                      ") "
                      "ORDER BY fn %s /*%s()*/",
                      rec->originRid,
                      cOpt->checkinRid
                      /*new checkout version resp. update target
                        version*/,
                      fsl_cx_filename_collation(f),
                      __func__);
  if(rc) goto end;

  rec->confirmAnswer.response = FSL_CRESPONSE_INVALID;
  cuState.mtime = 0;
  cuState.size = -1;
  cuState.callbackState = cOpt->callbackState;
  cuState.extractState = &rxState;
  cuState.dryRun = cOpt->dryRun;
  cuState.fileChangeType = FSL_CKUP_FCHANGE_RM;
  rxState.f = f;
  rxState.fCard = &fCard;
  rxState.checkinRid = cOpt->checkinRid;
  while(FSL_RC_STEP_ROW==(rc = fsl_stmt_step(&q))){
    /**
       Each row is one file listed in vfile (the old checkout
       version) which is not in vfile (the new checkout).
    */
    fsl_size_t nFn = 0;
    fsl_size_t hashLen = 0;
    char const * fn = fsl_stmt_g_text(&q, 1, &nFn);
    char const * hash = fsl_stmt_g_text(&q, 2, &hashLen);
    bool const isChanged = fsl_stmt_g_int32(&q, 5)!=0;
    int64_t const fSize = fsl_stmt_g_int64(&q, 6);
    if(FSL_CRESPONSE_ALWAYS!=rec->confirmAnswer.response){
      /**
         If the user has previously responded to
         FSL_CEVENT_RM_MOD_UNMGD_FILE, keep that response, else
         ask again if the file was flagged as changed in the
         vfile table before all of this started.
      */
      if(isChanged){
        // Modified: ask user unless they've already answered NEVER.
        if(FSL_CRESPONSE_NEVER!=rec->confirmAnswer.response){
          fsl_confirm_detail detail = fsl_confirm_detail_empty;
          detail.eventId = FSL_CEVENT_RM_MOD_UNMGD_FILE;
          detail.filename = fn;
          rec->confirmAnswer.response = FSL_CRESPONSE_INVALID;
          rc = fsl_cx_confirm(f, &detail, &rec->confirmAnswer);
          if(rc) goto end;
        }
      }else{
        // Not modified. Nuke it.
        rec->confirmAnswer.response = FSL_CRESPONSE_YES;
      }
    }
    absPath->used = ckdirLen;
    rc = fsl_buffer_append(absPath, fn, nFn);
    if(rc) break;
    zAbs = fsl_buffer_cstr(absPath);
    /* Ignore deletion errors. We cannot roll back previous deletions,
       so failing here, which would roll back the transaction, could
       leave the checkout in a weird state, potentially with some
       files missing and others not. */
    switch(rec->confirmAnswer.response){
      case FSL_CRESPONSE_YES:
      case FSL_CRESPONSE_ALWAYS:
        //MARKER(("Unlinking: %s\n",zAbs));
        if(!cOpt->dryRun && 0==fsl_file_unlink(zAbs)){
          ++rmCounter;
        }
        cuState.fileRmInfo = FSL_CKUP_RM;
        break;
      case FSL_CRESPONSE_NO:
      case FSL_CRESPONSE_NEVER:
        //assert(FSL_RECO_MOD_YES==modType);
        //MARKER(("NOT removing locally-modified file: %s\n", zN));
        cuState.fileRmInfo = FSL_CKUP_RM_KEPT;
        break;
      case FSL_CRESPONSE_CANCEL:
        rc = fsl_cx_err_set(f, FSL_RC_BREAK,
                            "Checkout operation cancelled by "
                            "confirmation callback. "
                            "Filesystem contents may now be "
                            "in an inconsistent state!");
        goto end;
      default:
        fsl_fatal(FSL_RC_UNSUPPORTED,"Internal error: invalid "
                  "fsl_cx_confirm() response #%d.",
                  rec->confirmAnswer.response);
        break;
    }
    if(!cOpt->callback) continue;
    /* Now report the deletion to the callback... */
    fsl_id_t const frid = fsl_stmt_g_id(&q, 0);
    const bool isExe = 0!=fsl_stmt_g_int32(&q, 3);
    const bool isLink = 0!=fsl_stmt_g_int32(&q, 4);
    cuState.size = (FSL_CKUP_RM==cuState.fileRmInfo) ? -1 : fSize;
    rxState.fileRid = frid;
    fCard = fsl_card_F_empty;
    fCard.name = (char *)fn;
    fCard.uuid = (char *)hash;
    fCard.perm = isExe ? FSL_FILE_PERM_EXE :
      (isLink ? FSL_FILE_PERM_LINK : FSL_FILE_PERM_REGULAR);
    rc = cOpt->callback( &cuState );
    if(rc) goto end;
  }
  if(FSL_RC_STEP_DONE==rc) rc = 0;
  else goto end;
  if(rmCounter>0){
    /* Clean up any empty directories left over by removal of
       files... */
    assert(!cOpt->dryRun);
    fsl_stmt_finalize(&q);
    /* Select dirs which were in the previous version
       (rec->originRid) but are not in the newly co'd version
       (cOpt->checkinRid). Any of these may _potentially_
       be empty now. This query could be improved to filter 
       out more in advance. */
    rc = fsl_db_prepare(db, &q,
                        "SELECT DISTINCT(fsl_dirpart(pathname,0)) dir "
                        "FROM vfile "
                        "WHERE vid=%" FSL_ID_T_PFMT " "
                        "AND pathname NOT IN "
                        "(SELECT pathname FROM vfile "
                        "WHERE vid=%" FSL_ID_T_PFMT ") "
                        "AND dir IS NOT NULL "
                        "ORDER BY length(dir) DESC /*%s()*/",
                        /*get deepest dirs first*/
                        rec->originRid, cOpt->checkinRid,
                        __func__);
    if(rc) goto end;
    while(FSL_RC_STEP_ROW==(rc = fsl_stmt_step(&q))){
      fsl_size_t nFn = 0;
      char const * fn = fsl_stmt_g_text(&q, 0, &nFn);
      absPath->used = ckdirLen;
      rc = fsl_buffer_append(absPath, fn, nFn);
      if(rc) break;
      fsl_ckout_rm_empty_dirs(f, absPath)
        /* To see this in action, use (f-co tip) to check out the tip of
           a repo, then use (f-co rid:1) to back up to the initial empty
           checkin. It "should" leave you with a directory devoid of
           anything but .fslckout and any non-SCM'd content.
        */;
    }
    if(FSL_RC_STEP_DONE==rc) rc = 0;
  }
  end:
  fsl_stmt_finalize(&q);
  fsl_cx_scratchpad_yield(f, absPath);
  return fsl_cx_uplift_db_error2(f, db, rc);
}

int fsl_repo_ckout(fsl_cx * f, fsl_ckup_opt const * cOpt){
  int rc = 0;
  fsl_id_t const prevRid = f->ckout.rid;
  fsl_db * const dbR = fsl_needs_repo(f);
  RepoExtractCkup rec = RepoExtractCkup_empty;
  fsl_confirmer oldConfirm = fsl_confirmer_empty;
  if(!dbR) return f->error.code;
  else if(!fsl_needs_ckout(f)) return f->error.code;
  rc = fsl_cx_transaction_begin(f);
  if(rc) return rc;
  rec.tgtDir = fsl_cx_scratchpad(f);
  if(cOpt->confirmer.callback){
    fsl_cx_confirmer(f, &cOpt->confirmer, &oldConfirm);
  }
  //MARKER(("ckout.rid=%d\n",(int)prevRid));
  if(prevRid>=0 && cOpt->scanForChanges){
    /* We need to ensure this state is current in order to determine
       whether a given file is locally modified vis-a-vis the
       pre-extract checkout state. */
    rc = fsl_vfile_changes_scan(f, prevRid, 0);
    if(rc) goto end;
  }
  if(0){
    fsl_db_each(dbR,fsl_stmt_each_f_dump, NULL,
                "SELECT * FROM vfile ORDER BY pathname");
  }
  assert(f->ckout.dirLen);
  fsl_repo_extract_opt eOpt = fsl_repo_extract_opt_empty;
  rc = fsl_buffer_append(rec.tgtDir, f->ckout.dir,
                         (fsl_int_t)f->ckout.dirLen);
  if(rc) goto end;
  if(prevRid){
    rc = fsl_db_prepare(dbR, &rec.stChanged,
                        "SELECT chnged FROM vfile "
                        "WHERE vid=%" FSL_ID_T_PFMT
                        " AND pathname=? %s",
                        prevRid,
                        fsl_cx_filename_collation(f));
  }
  if(!rc && prevRid){
    /* Optimization: before we load content for a blob and write it to
       a file, check this query for whether we already have the same
       name/rid combination in vfile, and skip loading/writing the
       content if we do. */
    rc = fsl_db_prepare(dbR, &rec.stIsInVfile,
                        "SELECT 1 FROM vfile "
                        "WHERE vid=%" FSL_ID_T_PFMT
                        " AND pathname=? AND rid=? %s",
                        prevRid, fsl_cx_filename_collation(f));
  }
  if(!rc){
    /* Files for which we don't load content (see rec.stIsInVfile)
       still have a size we need to report via fsl_ckup_state,
       and we fetch that with this query. */
    rc = fsl_db_prepare(dbR, &rec.stRidSize,
                        "SELECT size FROM blob WHERE rid=?");
  }
  if(rc){
    rc = fsl_cx_uplift_db_error2(f, dbR, rc);
    goto end;
  }
  rec.originRid = prevRid;
  rec.tgtDirLen = f->ckout.dirLen;
  eOpt.checkinRid = cOpt->checkinRid;
  eOpt.extractContent = false;
  eOpt.callbackState = &rec;
  eOpt.callback = fsl_repo_extract_f_ckout;
  rec.eOpt = &eOpt;
  rec.cOpt = cOpt;
  rc = fsl_repo_extract(f, &eOpt);
  if(!rc){
    /*
      We need to call fsl_vfile_load(f, cOpt->vid) to
      populate vfile but we also need to call
      fsl_vfile_changes_scan(f, cOpt->vid, 0) to set the vfile.mtime
      fields. The latter calls the former, so...
    */
    rc = fsl_vfile_changes_scan(f, cOpt->checkinRid,
                                FSL_VFILE_CKSIG_WRITE_CKOUT_VERSION
                                |
                                (prevRid==0
                                 ? 0 : FSL_VFILE_CKSIG_KEEP_OTHERS)
                                |
                                (cOpt->setMtime
                                 ? 0 : FSL_VFILE_CKSIG_SETMTIME)
                                /* Note that mtimes were set during
                                   extraction if cOpt->setMtime is
                                   true. */);
    if(rc) goto end;
    assert(f->ckout.rid==cOpt->checkinRid);
    assert(f->ckout.rid ? !!f->ckout.uuid : 1);
  }
  /**
    TODO: Implement db fingerprint.
  */
  if(!rc && prevRid!=0){
    rc = fsl_repo_ckout_rm_list_fini(f, &rec);
    if(rc) goto end;
  }
  rc = fsl_ckout_manifest_write(f, -1, -1, -1, NULL);
  end:
  if(!rc){
    rc = fsl_vfile_unload_except(f, cOpt->checkinRid);
    if(!rc) rc = fsl_ckout_clear_merge_state(f);
  }
  if(cOpt->confirmer.callback){
    fsl_cx_confirmer(f, &oldConfirm, NULL);
  }
  fsl_stmt_finalize(&rec.stChanged);
  fsl_stmt_finalize(&rec.stIsInVfile);
  fsl_stmt_finalize(&rec.stRidSize);
  fsl_cx_scratchpad_yield(f, rec.tgtDir);
  int const rc2 = fsl_cx_transaction_end(f, rc || cOpt->dryRun);
  return rc ? rc : rc2;
}

int fsl_ckout_update(fsl_cx * f, fsl_ckup_opt const *cuOpt){
  fsl_db * const dbR = fsl_needs_repo(f);
  fsl_db * const dbC = dbR ? fsl_needs_ckout(f) : 0;
  if(!dbR) return FSL_RC_NOT_A_REPO;
  else if(!dbC) return FSL_RC_NOT_A_CKOUT;
  int rc = 0, rc2 = 0;
  char const * collation = fsl_cx_filename_collation(f);
  fsl_id_t const ckRid = f->ckout.rid /* current version */;
  fsl_id_t const tid = cuOpt->checkinRid /* target version */;
  fsl_stmt q = fsl_stmt_empty;
  fsl_stmt mtimeXfer = fsl_stmt_empty;
  fsl_stmt mtimeGet = fsl_stmt_empty;
  fsl_stmt mtimeSet = fsl_stmt_empty;
  fsl_buffer * bFullPath = 0;
  fsl_buffer * bFullNewPath = 0;
  fsl_buffer * bFileUuid = 0;
  fsl_repo_extract_opt eOpt = fsl_repo_extract_opt_empty
    /* We won't actually use fsl_repo_extract() here because it's a
       poor fit for the update selection algorithm, but in order to
       consolidate some code between the ckout/update cases we need to
       behave as if we were using it. */;
  fsl_repo_extract_state xState = fsl_repo_extract_state_empty;
  fsl_card_F fCard = fsl_card_F_empty;
  fsl_ckup_state uState = fsl_ckup_state_empty;
  RepoExtractCkup rec = RepoExtractCkup_empty;
  enum { MergeBufCount = 4 };
  fsl_buffer bufMerge[MergeBufCount] = {
    fsl_buffer_empty_m/* pivot: ridv */,
    fsl_buffer_empty_m/* local file to merge into */,
    fsl_buffer_empty_m/* update-to: ridt */,
    fsl_buffer_empty_m/* merged copy */
  };

  rc = fsl_db_transaction_begin(dbC);
  if(rc) return fsl_cx_uplift_db_error2(f, dbC, rc);
  if(cuOpt->scanForChanges){
    rc = fsl_vfile_changes_scan(f, ckRid, FSL_VFILE_CKSIG_ENOTFILE);
    if(rc) goto end;
  }
  if(tid != ckRid){
    uint32_t missingCount = 0;
    rc = fsl_vfile_load(f, tid, false,
                                 &missingCount);
    if(rc) goto end;
    else if(missingCount/* && !forceMissing*/){
      rc = fsl_cx_err_set(f, FSL_RC_PHANTOM,
                          "Unable to update due to missing content in "
                          "%"PRIu32" blob(s).", missingCount);
      goto end;
    }
  }
  /*
  ** The record.fn field is used to match files against each other.  The
  ** FV table contains one row for each each unique filename in
  ** in the current checkout, the pivot, and the version being merged.
  */
  rc = fsl_db_exec_multi(dbC,
    "DROP TABLE IF EXISTS fv;"
    "CREATE TEMP TABLE fv("
    "  fn TEXT %s PRIMARY KEY,"   /* The filename relative to root */
    "  idv INTEGER,"              /* VFILE entry for current version */
    "  idt INTEGER,"              /* VFILE entry for target version */
    "  chnged BOOLEAN,"           /* True if current version has been edited */
    "  islinkv BOOLEAN,"          /* True if current file is a link */
    "  islinkt BOOLEAN,"          /* True if target file is a link */
    "  ridv INTEGER,"             /* Record ID for current version */
    "  ridt INTEGER,"             /* Record ID for target */
    "  isexe BOOLEAN,"            /* Does target have execute permission? */
    "  deleted BOOLEAN DEFAULT 0,"/* File marked by "rm" to become unmanaged */
    "  fnt TEXT %s"               /* Filename of same file on target version */
    ") /*%s()*/;",
    collation, collation, __func__ );
  if(rc) goto dberr;
  /* Add files found in the current version
  */
  rc = fsl_db_exec_multi(dbC,
    "INSERT OR IGNORE INTO fv("
            "fn,fnt,idv,idt,ridv,"
            "ridt,isexe,chnged,deleted"
    ") SELECT pathname, pathname, id, 0, rid, 0, "
       "isexe, chnged, deleted "
       "FROM vfile WHERE vid=%" FSL_ID_T_PFMT
       "/*%s()*/",
    ckRid, __func__
  );
  if(rc) goto dberr;

  /* Compute file name changes on V->T.  Record name changes in files that
  ** have changed locally.
  */
  if( ckRid ){
    uint32_t nChng = 0;
    fsl_id_t * aChng = 0;
    rc = fsl_cx_find_filename_changes(f, ckRid, tid,
                                      true, &nChng, &aChng);
    if(rc){
      assert(!aChng);
      assert(!nChng);
      goto end;
    }
    if( nChng ){
      for(uint32_t i=0; i<nChng; ++i){
        rc = fsl_db_exec_multi(dbC,
          "UPDATE fv"
          "   SET fnt=(SELECT name FROM filename WHERE fnid=%"
              FSL_ID_T_PFMT ")"
          " WHERE fn=(SELECT name FROM filename WHERE fnid=%"
            FSL_ID_T_PFMT ") AND chnged /*%s()*/",
          aChng[i*2+1], aChng[i*2], __func__
        );
        if(rc) goto dberr;
      }
      fsl_free(aChng);
    }else{
      assert(!aChng);
    }
  }/*ckRid!=0*/

  /* Add files found in the target version T but missing from the current
  ** version V.
  */
  rc = fsl_db_exec_multi(dbC,
    "INSERT OR IGNORE INTO fv(fn,fnt,idv,idt,ridv,ridt,isexe,chnged)"
    " SELECT pathname, pathname, 0, 0, 0, 0, isexe, 0 FROM vfile"
    "  WHERE vid=%" FSL_ID_T_PFMT
    "    AND pathname %s NOT IN (SELECT fnt FROM fv) /*%s()*/",
    tid, collation, __func__
  );
  if(rc) goto dberr;

  /*
  ** Compute the file version ids for T
  */
  rc = fsl_db_exec_multi(dbC,
    "UPDATE fv SET"
    " idt=coalesce((SELECT id FROM vfile WHERE vid=%"
                   FSL_ID_T_PFMT " AND fnt=pathname),0),"
    " ridt=coalesce((SELECT rid FROM vfile WHERE vid=%"
                    FSL_ID_T_PFMT " AND fnt=pathname),0) /*%s()*/",
    tid, tid, __func__
  );
  if(rc) goto dberr;

  /*
  ** Add islink information
  */
  rc = fsl_db_exec_multi(dbC,
    "UPDATE fv SET"
    " islinkv=coalesce((SELECT islink FROM vfile"
                       " WHERE vid=%" FSL_ID_T_PFMT
                         " AND fnt=pathname),0),"
    " islinkt=coalesce((SELECT islink FROM vfile"
                       " WHERE vid=%" FSL_ID_T_PFMT
                         " AND fnt=pathname),0) /*%s()*/",
    ckRid, tid, __func__
  );
  if(rc) goto dberr;

  /**
     Right here, fossil(1) permits passing on a subset of
     filenames/dirs to update, but it's apparently a little-used
     feature and we're going to skip it for the time being:

     https://fossil-scm.org/forum/forumpost/1da828facf
   */

  /*
  ** Alter the content of the checkout so that it conforms with the
  ** target
  */
  rc = fsl_db_prepare(dbC, &q,
                      "SELECT fn, idv, ridv, "/* 0..2  */
                      "idt, ridt, chnged, "   /* 3..5  */
                      "fnt, isexe, islinkv, " /* 6..8  */
                      "islinkt, deleted "     /* 9..10 */
                      "FROM fv ORDER BY 1 /*%s()*/",
                      __func__);
  if(rc) goto dberr;
  rc = fsl_db_prepare(dbC, &mtimeXfer,
                      "UPDATE vfile SET mtime=(SELECT mtime FROM vfile "
                      "WHERE id=?1/*idv*/) "
                      "WHERE id=?2/*idt*/ /*%s()*/",
                      __func__);
  if(rc) goto dberr;
  rc = fsl_db_prepare(dbR, &rec.stChanged,
                      "SELECT chnged FROM vfile "
                      "WHERE vid=%" FSL_ID_T_PFMT
                      " AND pathname=? %s /*%s()*/",
                      ckRid, collation, __func__);
  if(rc) goto dberr;
  if(cuOpt->callback){
    /* Queries we need only if we need to collect info for a
       callback... */
    rc = fsl_db_prepare(dbC, &mtimeGet,
                        "SELECT mtime FROM vfile WHERE id=?1"/*idt*/);
    if(rc) goto dberr;
    rc = fsl_db_prepare(dbC, &mtimeSet,
                        "UPDATE vfile SET mtime=?2 WHERE id=?1"/*idt*/);
    if(rc) goto dberr;
    /* Files for which we don't load content still have a size we need
       to report via fsl_ckup_state, and we fetch that with this
       query. */
    rc = fsl_db_prepare(dbR, &rec.stRidSize,
                        "SELECT size FROM blob WHERE rid=?");
    if(rc) goto dberr;
  }

  xState.f = f;
  xState.fCard = &fCard;
  xState.checkinRid = eOpt.checkinRid = tid;
  xState.count.fileCount =
    (uint32_t)fsl_db_g_int32(dbC, 0, "SELECT COUNT(*) FROM vfile "
                             "WHERE vid=%" FSL_ID_T_PFMT,
                             tid);
  uState.extractState = &xState;
  uState.callbackState = cuOpt->callbackState;
  uState.dryRun = cuOpt->dryRun;
  uState.fileRmInfo = FSL_CKUP_RM_NOT;
  rec.originRid = ckRid;
  rec.eOpt = &eOpt;
  rec.cOpt = cuOpt;
  rec.tgtDir = fsl_cx_scratchpad(f);
  rec.tgtDirLen = f->ckout.dirLen;
  rc = fsl_buffer_append(rec.tgtDir, f->ckout.dir,
                         (fsl_int_t)f->ckout.dirLen);
  if(rc) goto end;

  /**
     Missing features from fossil we still need for this include,
     but are not limited to:

     - file_unsafe_in_tree_path() (done, untested)
     - file_nondir_objects_on_path() (done, untested)
     - symlink_create() (done, untested)
     - vfile_to_disk() (done, untested)
     - ...
  */
  bFullPath = fsl_cx_scratchpad(f);
  bFullNewPath = fsl_cx_scratchpad(f);
  bFileUuid = fsl_cx_scratchpad(f);
  rc = fsl_buffer_append(bFullPath, f->ckout.dir,
                         (fsl_int_t)f->ckout.dirLen);
  if(rc) goto end;
  rc = fsl_buffer_append(bFullNewPath, f->ckout.dir,
                         (fsl_int_t)f->ckout.dirLen);
  if(rc) goto end;
  unsigned int nConflict = 0;
  while( FSL_RC_STEP_ROW==fsl_stmt_step(&q) ){
    const char *zName = fsl_stmt_g_text(&q, 0, NULL)
      /* The filename from root */;
    fsl_id_t const idv = fsl_stmt_g_id(&q, 1)
      /* VFILE entry for current */;
    fsl_id_t const ridv = fsl_stmt_g_id(&q, 2)
      /* RecordID for current */;
    fsl_id_t const idt = fsl_stmt_g_id(&q, 3)
      /* VFILE entry for target */;
    fsl_id_t const ridt = fsl_stmt_g_id(&q, 4)
      /* RecordID for target */;
    int const chnged = fsl_stmt_g_int32(&q, 5)
      /* Current is edited */;
    const char *zNewName = fsl_stmt_g_text(&q,6, NULL)
      /* New filename */;
    int const isexe = fsl_stmt_g_int32(&q, 7)
      /* EXE perm for new file */;
    int const islinkv = fsl_stmt_g_int32(&q, 8)
      /* Is current file is a link */;
    int const islinkt = fsl_stmt_g_int32(&q, 9)
      /* Is target file is a link */;
    int const deleted = fsl_stmt_g_int32(&q, 10)
      /* Marked for deletion */;
    char const *zFullPath /* Full pathname of the file */;
    char const *zFullNewPath /* Full pathname of dest */;
    bool const nameChng = !!fsl_strcmp(zName, zNewName)
      /* True if the name changed */;
    int wasWritten = 0
      /* 1=perms written to disk, 2=content written */;
    fsl_fstat fst = fsl_fstat_empty;
    if(chnged || isexe || islinkv || islinkt){/*unused*/}
    
    bFullPath->used = bFullNewPath->used = f->ckout.dirLen;
    rc = fsl_buffer_appendf(bFullPath, zName, -1);
    if(!rc) rc = fsl_buffer_appendf(bFullNewPath, zNewName, -1);
    if(rc) goto end;
    zFullPath = fsl_buffer_cstr(bFullPath);
    zFullNewPath = fsl_buffer_cstr(bFullNewPath);
    uState.mtime = 0;
    uState.fileChangeType = FSL_CKUP_FCHANGE_INVALID;
    uState.fileRmInfo = FSL_CKUP_RM_NOT;
    ++xState.count.fileNumber;
    //MARKER(("#%03u/%03d %s\n", xState.count.fileNumber, xState.count.fileCount, zName));
    if( deleted ){
      /* Carry over pending file deletions from the current version
         into the target version. If the file was already deleted in
         the target version, that will be picked up by the file-deletion
         loop later on. */
      uState.fileChangeType = FSL_CKUP_FCHANGE_RM_PROPAGATED;
      rc = fsl_db_exec(dbC, "UPDATE vfile SET deleted=1 "
                       "WHERE id=%" FSL_ID_T_PFMT" /*%s()*/",
                       idt, __func__);
      if(rc) goto dberr;
    }
    if( idv>0 && ridv==0 && idt>0 && ridt>0 ){
      /* Conflict.  This file has been added to the current checkout
      ** but also exists in the target checkout.  Use the current version.
      */
      uState.fileChangeType = FSL_CKUP_FCHANGE_CONFLICT_ADDED;
      //fossil_print("CONFLICT %s\n", zName);
      nConflict++;
    }else if( idt>0 && idv==0 ){
      /* File added in the target. */
      if( fsl_is_file_or_link(zFullPath) ){
        //fossil_print("ADD %s - overwrites an unmanaged file\n", zName);
        uState.fileChangeType =
          FSL_CKUP_FCHANGE_CONFLICT_ADDED_UNMANAGED;
        //nOverwrite++;
        /* TODO/FIXME: if the files have the same content, treat this
           as FSL_CKUP_FCHANGE_ADDED. If they don't, use confirmer to
           ask the user what to do. */
      }else{
        //fsl_outputf(f, "ADD %s\n", zName);
        uState.fileChangeType = FSL_CKUP_FCHANGE_ADDED;
      }
      //if( !dryRunFlag && !internalUpdate ) undo_save(zName);
      //MARKER(("Here's where we would vfile_to_disk() %s\n", zName));
      if( !cuOpt->dryRun ){
        rc = fsl_vfile_to_ckout(f, idt, &wasWritten);
        if(rc) goto end;
      }
    }else if( idt>0 && idv>0 && ridt!=ridv && (chnged==0 || deleted) ){
      /* The file is unedited.  Change it to the target version */
      if( deleted ){
        //fossil_print("UPDATE %s - change to unmanaged file\n", zName);
        uState.fileChangeType = FSL_CKUP_FCHANGE_RM;
      }else{
        //fossil_print("UPDATE %s\n", zName);
        uState.fileChangeType = FSL_CKUP_FCHANGE_UPDATED;
      }
      if( !cuOpt->dryRun ){
        rc = fsl_vfile_to_ckout(f, idt, &wasWritten);
        if(rc) goto end;
      }
    }else if( idt>0 && idv>0 && !deleted &&
              0!=fsl_stat(zFullPath, NULL, false) ){
      /* The file is missing from the local check-out. Restore it to
      ** the version that appears in the target. */
      uState.fileChangeType = FSL_CKUP_FCHANGE_UPDATED;
      if( !cuOpt->dryRun ){
        rc = fsl_vfile_to_ckout(f, idt, &wasWritten);
        if(rc) goto end;
      }
    }else if( idt==0 && idv>0 ){
      /* Is in the current version but not in the target. */
      if( ridv==0 ){
        /* Added in current checkout.  Continue to hold the file as
        ** as an addition */
        uState.fileChangeType = FSL_CKUP_FCHANGE_ADD_PROPAGATED;
        rc = fsl_db_exec(dbC, "UPDATE vfile SET vid=%" FSL_ID_T_PFMT
                         " WHERE id=%" FSL_ID_T_PFMT " /*%s()*/",
                         tid, idv, __func__);
        if(rc) goto dberr;
      }else if( chnged ){
        /* Edited locally but deleted from the target.  Do not track the
        ** file but keep the edited version around. */
        uState.fileChangeType = FSL_CKUP_FCHANGE_CONFLICT_RM;
        ++nConflict;
        uState.fileRmInfo = FSL_CKUP_RM_KEPT;
        /* Delete idv from vfile so that the post-processing rm
           loop will not delete this file. */
        rc = fsl_db_exec(dbC, "DELETE FROM vfile WHERE id=%"
                         FSL_ID_T_PFMT " /*%s()*/",
                         idv, __func__);
        if(rc) goto dberr;

      }else{
        uState.fileChangeType = FSL_CKUP_FCHANGE_RM;
        if( !cuOpt->dryRun ){
          fsl_file_unlink(zFullPath)/*ignore errors*/;
          /* At this point fossil(1) adds each directory to the
             dir_to_delete table. We can probably use the same
             infrastructure which ckout uses, though. One
             hiccup there is that our infrastructure does not
             handle the locally-modified-removed case from the
             block above this one. */
        }
      }
    }else if( idt>0 && idv>0 && ridt!=ridv && chnged ){
      /* Merge the changes in the current tree into the target version */
      if( islinkv || islinkt ){
        uState.fileChangeType = FSL_CKUP_FCHANGE_CONFLICT_SYMLINK;
        ++nConflict;
      }else{
        unsigned int conflictCount = 0;
        for(int i = 0; i < MergeBufCount; ++i){
          fsl_buffer_reuse(&bufMerge[i]);
        }
        rc = fsl_content_get(f, ridv, &bufMerge[0]);
        if(!rc) rc = fsl_content_get(f, ridt, &bufMerge[2]);
        if(!rc){
          rc = fsl_buffer_fill_from_filename(&bufMerge[1], zFullPath);
        }
        if(rc) goto end;
        rc = fsl_buffer_merge3(&bufMerge[0], &bufMerge[1],
                               &bufMerge[2], &bufMerge[3],
                               &conflictCount);
        if(FSL_RC_TYPE==rc){
          /* Binary content: we can't merge this, so use target
             version. */
          rc = 0;
          uState.fileChangeType = FSL_CKUP_FCHANGE_UPDATED_BINARY;
          if( !cuOpt->dryRun ){
            rc = fsl_buffer_to_filename(&bufMerge[2], zFullNewPath);
            if(!rc) fsl_file_exec_set(zFullNewPath, !!isexe);
          }
        }else if(!rc){
          if( !cuOpt->dryRun ){
            rc = fsl_buffer_to_filename(&bufMerge[3], zFullNewPath);
            if(!rc) fsl_file_exec_set(zFullNewPath, !!isexe);
          }
          uState.fileChangeType = conflictCount
            ? FSL_CKUP_FCHANGE_CONFLICT_MERGED
            : FSL_CKUP_FCHANGE_MERGED;
          if(conflictCount) ++nConflict;
        }
        if(rc) goto end;
      }
      if( nameChng && !cuOpt->dryRun ){
        fsl_file_unlink(zFullPath);
      }
    }else{
      if( chnged ){
        if( !deleted ){
          uState.fileChangeType = FSL_CKUP_FCHANGE_EDITED;
        }else{
          assert(FSL_CKUP_FCHANGE_RM_PROPAGATED==uState.fileChangeType);
        }
      }else{
        uState.fileChangeType = FSL_CKUP_FCHANGE_NONE;
        rc = fsl_stmt_bind_step(&mtimeXfer, "RR", idv, idt);
        if(rc) goto dberr;
      }
    }
    if(wasWritten && cuOpt->setMtime){
      if(0==fsl_mtime_of_manifest_file(f, tid, ridt, &uState.mtime)){
        fsl_file_mtime_set(zFullNewPath, uState.mtime);
        rc = fsl_stmt_bind_step(&mtimeSet, "RI", idt, uState.mtime);
        if(rc) goto dberr;
      }
    }
    assert(FSL_CKUP_FCHANGE_INVALID != uState.fileChangeType);
    assert(!rc);
    if(cuOpt->callback
       && (FSL_CKUP_FCHANGE_RM != uState.fileChangeType)
       /* removals are reported separately in the file
          deletion phase */){
      if(FSL_CKUP_FCHANGE_ADD_PROPAGATED==uState.fileChangeType){
        /* This file is not yet in SCM, so its size is not in
           the db. */
        if(0==fsl_stat(zFullNewPath, &fst, false)){
          uState.size = (fsl_int_t)fst.size;
          uState.mtime = fst.mtime;
        }else{
          uState.size = -1;
        }
      }else{
        /* If we have the record's size in the db, use that. */
        fsl_stmt_bind_id(&rec.stRidSize, 1, ridt);
        if(FSL_RC_STEP_ROW==fsl_stmt_step(&rec.stRidSize)){
          uState.size = fsl_stmt_g_int32(&rec.stRidSize, 0);
        }else{
          uState.size = -1;
        }
        fsl_stmt_reset(&rec.stRidSize);
      }
      if(!uState.mtime){
        fsl_stmt_bind_id(&mtimeGet, 1, idt);
        if(FSL_RC_STEP_ROW==fsl_stmt_step(&mtimeGet)){
          uState.mtime = fsl_stmt_g_id(&mtimeGet, 0);
        }
        if(0==uState.mtime && 0==fsl_stat(zFullNewPath, &fst, false)){
          uState.mtime = fst.mtime;
        }
        fsl_stmt_reset(&mtimeGet);
      }
      xState.fileRid = ridt;
      fCard.name = (char *)zNewName;
      fCard.priorName = (char *)(nameChng ? zName : NULL);
      fCard.perm = islinkt ? FSL_FILE_PERM_LINK
        : (isexe ? FSL_FILE_PERM_EXE : FSL_FILE_PERM_REGULAR);
      if(ridt){
        rc = fsl_rid_to_uuid2(f, ridt, bFileUuid);
        if(rc) goto end;
        fCard.uuid = fsl_buffer_str(bFileUuid);
      }else{
        //MARKER(("ridt=%d uState.fileChangeType=%d name=%s\n",
        //        ridt, uState.fileChangeType, fCard.name));
        assert(FSL_CKUP_FCHANGE_CONFLICT_RM==uState.fileChangeType
               || FSL_CKUP_FCHANGE_ADD_PROPAGATED==uState.fileChangeType
               || FSL_CKUP_FCHANGE_EDITED==uState.fileChangeType
               );
        fCard.uuid = 0;
      }
      rc = cuOpt->callback( &uState );
      if(rc) goto end;
      uState.mtime = 0;
    }
  }/*fsl_stmt_step(&q)*/
  fsl_stmt_finalize(&q);
  if(nConflict){/*unused*/}
  /*
    At this point, fossil(1) does:

    ensure_empty_dirs_created(1);
    checkout_set_all_exe();
  */
  assert(!rc);
  rc = fsl_repo_ckout_rm_list_fini(f, &rec);
  if(!rc){
    rc = fsl_vfile_unload_except(f, tid);
  }
  if(!rc){
    rc = fsl_ckout_version_write(f, tid, 0);
  }
  
  end:
  /* clang bug? If we declare rc2 here, it says "expression expected".
     Moving the decl to the top resolves it. Wha? */
  if(rec.tgtDir) fsl_cx_scratchpad_yield(f, rec.tgtDir);
  if(bFullPath) fsl_cx_scratchpad_yield(f, bFullPath);
  if(bFullNewPath) fsl_cx_scratchpad_yield(f, bFullNewPath);
  if(bFileUuid) fsl_cx_scratchpad_yield(f, bFileUuid);
  for(int i = 0; i < MergeBufCount; ++i){
    fsl_buffer_clear(&bufMerge[i]);
  }
  fsl_stmt_finalize(&rec.stRidSize);
  fsl_stmt_finalize(&rec.stChanged);
  fsl_stmt_finalize(&mtimeGet);
  fsl_stmt_finalize(&mtimeSet);
  fsl_stmt_finalize(&q);
  fsl_stmt_finalize(&mtimeXfer);
  fsl_db_exec(dbC, "DROP TABLE fv /*%s()*/", __func__);
  rc2 = fsl_db_transaction_end(dbC, !!rc);
  return rc ? rc : rc2;
  dberr:
  assert(rc);
  rc = fsl_cx_uplift_db_error2(f, dbC, rc);
  goto end;
}


/** Helper for generating a list of ambiguous leaf UUIDs. */
struct AmbiguousLeavesOutput {
  int count;
  int rc;
  fsl_buffer * buffer;
};
typedef struct AmbiguousLeavesOutput AmbiguousLeavesOutput;
static const AmbiguousLeavesOutput AmbiguousLeavesOutput_empty =
  {0, 0, NULL};

static int fsl_stmt_each_f_ambiguous_leaves( fsl_stmt * stmt, void * state ){
  AmbiguousLeavesOutput * alo = (AmbiguousLeavesOutput*)state;
  if(alo->count++){
    alo->rc = fsl_buffer_append(alo->buffer, ", ", 2);
  }
  if(!alo->rc){
    fsl_size_t n = 0;
    char const * uuid = fsl_stmt_g_text(stmt, 0, &n);
    assert(n==FSL_STRLEN_SHA1 || n==FSL_STRLEN_K256);
    alo->rc = fsl_buffer_append(alo->buffer, uuid, 16);
  }
  return alo->rc;
}

int fsl_ckout_calc_update_version(fsl_cx * f, fsl_id_t * outRid){
  fsl_db * const dbRepo = fsl_needs_repo(f);
  if(!dbRepo) return FSL_RC_NOT_A_REPO;
  else if(!fsl_needs_ckout(f)) return FSL_RC_NOT_A_CKOUT;
  int rc = 0;
  fsl_id_t tgtRid = 0;
  fsl_leaves_compute_e leafMode = FSL_LEAVES_COMPUTE_OPEN;
  fsl_id_t const ckRid = f->ckout.rid;
  rc = fsl_leaves_compute(f, ckRid, leafMode);
  if(rc) goto end;
  if( !fsl_leaves_computed_has(f) ){
    leafMode = FSL_LEAVES_COMPUTE_ALL;
    rc = fsl_leaves_compute(f, ckRid, leafMode);
    if(rc) goto end;
  }
  /* Delete [leaves] entries from any branches other than
     ckRid's... */
  rc = fsl_db_exec_multi(dbRepo,
        "DELETE FROM leaves WHERE rid NOT IN"
        "   (SELECT leaves.rid FROM leaves, tagxref"
        "     WHERE leaves.rid=tagxref.rid AND tagxref.tagid=%d"
        "       AND tagxref.value==(SELECT value FROM tagxref"
                                   " WHERE tagid=%d AND rid=%"
                             FSL_ID_T_PFMT "))",
        FSL_TAGID_BRANCH, FSL_TAGID_BRANCH, ckRid
  );
  if(rc) goto end;
  else if( fsl_leaves_computed_count(f)>1 ){
    AmbiguousLeavesOutput alo = AmbiguousLeavesOutput_empty;
    alo.buffer = fsl_cx_scratchpad(f);
    rc = fsl_buffer_append(alo.buffer,
                           "Multiple viable descendants found: ", -1);
    if(!rc){
      fsl_stmt q = fsl_stmt_empty;
      rc = fsl_db_prepare(dbRepo, &q, "SELECT uuid FROM blob "
                          "WHERE rid IN leaves ORDER BY uuid");
      if(!rc){
        rc = fsl_stmt_each(&q, fsl_stmt_each_f_ambiguous_leaves, &alo);
      }
      fsl_stmt_finalize(&q);
    }
    if(!rc){
      rc = fsl_cx_err_set(f, FSL_RC_AMBIGUOUS, "%b", alo.buffer);
    }
    fsl_cx_scratchpad_yield(f, alo.buffer);
  }
  end:
  if(!rc){
    tgtRid = fsl_leaves_computed_latest(f);
    *outRid = tgtRid;
    fsl_leaves_computed_cleanup(f)
      /* We might want to keep [leaves] around for the case where we
         return FSL_RC_AMBIGUOUS, to give the client a way to access
         that list in its raw form. Higher-level code could join that
         with the event table to give the user more context. */;
  }
  return rc;
}

void fsl_ckout_manifest_setting(fsl_cx *f, int *m){
  if(!m){
    f->cache.manifestSetting = -1;
    return;
  }else if(f->cache.manifestSetting>=0){
    *m = f->cache.manifestSetting;
    return;
  }
  char * str = fsl_config_get_text(f, FSL_CONFDB_VERSIONABLE,
                                   "manifest", NULL);
  if(!str){
    str = fsl_config_get_text(f, FSL_CONFDB_REPO,
                              "manifest", NULL);
  }
  *m = 0;
  if(str){
    char const * z = str;
    if('1'==*z || 0==fsl_strncmp(z,"on",2)
       || 0==fsl_strncmp(z,"true",4)){
      z = "ru"/*historical default*/;
    }else if(!fsl_str_bool(z)){
      z = "";
    }
    for(;*z;++z){
      switch(*z){
        case 'r': *m |= 0x001; break;
        case 'u': *m |= 0x010; break;
        case 't': *m |= 0x100; break;
        default: break;
      }
    }
    fsl_free(str);
  }
  f->cache.manifestSetting = (short)*m;
}

int fsl_ckout_manifest_write(fsl_cx *f, int manifest, int manifestUuid,
                             int manifestTags,
                             int * wrote){
  fsl_db * const db = fsl_needs_ckout(f);
  if(!db) return FSL_RC_NOT_A_CKOUT;
  else if(!f->ckout.rid){
    return fsl_cx_err_set(f, FSL_RC_RANGE,
                          "Checkout RID is 0, so it has no manifest.");
  }
  int W = 0;
  int rc = 0;
  fsl_buffer * b = fsl_cx_scratchpad(f);
  fsl_buffer * content = &f->fileContent;
  char * str = 0;
  fsl_time_t const mtime = f->ckout.mtime>0
    ? fsl_julian_to_unix(f->ckout.mtime)
    : 0;
  fsl_buffer_reuse(content);
  if(manifest<0 || manifestUuid<0 || manifestTags<0){
    int setting = 0;
    fsl_ckout_manifest_setting(f, &setting);
    if(manifest<0 && setting & FSL_MANIFEST_MAIN) manifest=1;
    if(manifestUuid<0 && setting & FSL_MANIFEST_UUID) manifestUuid=1;
    if(manifestTags<0 && setting & FSL_MANIFEST_TAGS) manifestTags=1;
  }
  if(manifest || manifestUuid || manifestTags){
    rc = fsl_buffer_append(b, f->ckout.dir, (fsl_int_t)f->ckout.dirLen);
    if(rc) goto end;
  }
  if(manifest>0){
    rc = fsl_buffer_append(b, "manifest", 8);
    if(rc) goto end;
    rc = fsl_content_get(f, f->ckout.rid, content);
    if(rc) goto end;
    rc = fsl_buffer_to_filename(content, fsl_buffer_cstr(b));
    if(rc){
      rc = fsl_cx_err_set(f, rc, "Error writing file: %b", b);
      goto end;
    }
    if(mtime) fsl_file_mtime_set(fsl_buffer_cstr(b), mtime);
    W |= FSL_MANIFEST_MAIN;
  }else if(!fsl_db_exists(db,
                          "SELECT 1 FROM vfile WHERE "
                          "pathname='manifest' /*%s()*/",
                          __func__)){
    b->used = f->ckout.dirLen;
    rc = fsl_buffer_append(b, "manifest", 8);
    if(rc) goto end;
    fsl_file_unlink(fsl_buffer_cstr(b));
  }

  if(manifestUuid>0){
    b->used = f->ckout.dirLen;
    fsl_buffer_reuse(content);
    rc = fsl_buffer_append(b, "manifest.uuid", 13);
    if(rc) goto end;
    assert(f->ckout.uuid);
    rc = fsl_buffer_append(content, f->ckout.uuid, -1);
    if(!rc) rc = fsl_buffer_append(content, "\n", 1);
    if(rc) goto end;
    rc = fsl_buffer_to_filename(content, fsl_buffer_cstr(b));
    if(rc){
      rc = fsl_cx_err_set(f, rc, "Error writing file: %b", b);
      goto end;
    }
    if(mtime) fsl_file_mtime_set(fsl_buffer_cstr(b), mtime);
    W |= FSL_MANIFEST_UUID;
  }else if(!fsl_db_exists(db,
                          "SELECT 1 FROM vfile WHERE "
                          "pathname='manifest.uuid' /*%s()*/",
                          __func__)){
    b->used = f->ckout.dirLen;
    rc = fsl_buffer_append(b, "manifest.uuid", 13);
    if(rc) goto end;
    fsl_file_unlink(fsl_buffer_cstr(b));
  }

  if(manifestTags>0){
    fsl_stmt q = fsl_stmt_empty;
    fsl_db * const db = fsl_cx_db_repo(f);
    assert(db && "We can't have a checkout w/o a repo.");
    b->used = f->ckout.dirLen;
    fsl_buffer_reuse(content);
    rc = fsl_buffer_append(b, "manifest.tags", 13);
    if(rc) goto end;
    str = fsl_db_g_text(db, NULL, "SELECT VALUE FROM tagxref "
                        "WHERE rid=%" FSL_ID_T_PFMT
                        " AND tagid=%d /*%s()*/",
                        f->ckout.rid, FSL_TAGID_BRANCH, __func__);
    rc = fsl_buffer_appendf(content, "branch %z\n", str);
    str = 0;
    if(rc) goto end;
    rc = fsl_db_prepare(db, &q,
                        "SELECT substr(tagname, 5)"
                        "  FROM tagxref, tag"
                        " WHERE tagxref.rid=%" FSL_ID_T_PFMT
                        "   AND tagxref.tagtype>0"
                        "   AND tag.tagid=tagxref.tagid"
                        "   AND tag.tagname GLOB 'sym-*'"
                        " /*%s()*/",
                        f->ckout.rid, __func__);
    if(rc) goto end;
    while( FSL_RC_STEP_ROW==fsl_stmt_step(&q) ){
      const char *zName = fsl_stmt_g_text(&q, 0, NULL);
      rc = fsl_buffer_appendf(content, "tag %s\n", zName);
      if(rc) break;
    }
    fsl_stmt_finalize(&q);
    if(!rc){
      rc = fsl_buffer_to_filename(content, fsl_buffer_cstr(b));
      if(rc){
        rc = fsl_cx_err_set(f, rc, "Error writing file: %b", b);
      }
    }
    if(mtime) fsl_file_mtime_set(fsl_buffer_cstr(b), mtime);
    W |= FSL_MANIFEST_TAGS;
  }else if(!fsl_db_exists(db,
                          "SELECT 1 FROM vfile WHERE "
                          "pathname='manifest.tags' /*%s()*/",
                          __func__)){
    b->used = f->ckout.dirLen;
    rc = fsl_buffer_append(b, "manifest.tags", 13);
    if(rc) goto end;
    fsl_file_unlink(fsl_buffer_cstr(b));
  }

  end:
  if(wrote) *wrote = W;
  fsl_cx_scratchpad_yield(f, b);
  fsl_buffer_reuse(content);
  return rc;
}

/**
   Check every sub-directory of f's current checkout dir along the
   path to zFilename. If any sub-directory part is really an ordinary file
   or a symbolic link, set *errLen to the length of the prefix of zFilename
   which is the name of that object.

   Returns 0 except on allocation error, in which case it returned FSL_RC_OOM.
   If it finds nothing untowards about the path, *errLen will be set to 0.
   
   Example:  Given inputs
   
   ckout     = /home/alice/project1
   zFilename = /home/alice/project1/main/src/js/fileA.js
   
   Look for objects in the following order:
   
   /home/alice/project/main
   /home/alice/project/main/src
   /home/alice/project/main/src/js
   
   If any of those objects exist and are something other than a
   directory then *errLen will be the length of the name of the first
   non-directory object seen.

   If a given element of the path does not exist in the filesystem,
   traversal stops without an error.
*/
static int fsl_ckout_nondir_file_check(fsl_cx *f, char const * zFilename,
                                       fsl_size_t * errLen);

int fsl_ckout_nondir_file_check(fsl_cx *f, char const * zFilename,
                                fsl_size_t * errLen){
  if(!fsl_needs_ckout(f)) return FSL_RC_NOT_A_CKOUT;
  int rc = 0;
  int frc;
  fsl_buffer * const fn = fsl_cx_scratchpad(f);
  if(!fsl_is_rooted_in_ckout(f, zFilename)){
    assert(!"Misuse of this API. This condition should never fail.");
    rc = fsl_cx_err_set(f, FSL_RC_MISUSE, "Path is not rooted at the "
                        "current checkout directory: %s", zFilename);
    goto end;
  }
  rc = fsl_buffer_append(fn, zFilename, -1);
  if(rc) goto end;
  char * z = fsl_buffer_str(fn);
  fsl_size_t i = f->ckout.dirLen;
  fsl_size_t j;
  fsl_fstat fst = fsl_fstat_empty;
  char const * const zRoot = f->ckout.dir;
  if(i && '/'==zRoot[i-1]) --i;
  *errLen = 0;
  while( z[i]=='/' ){
    for(j=i+1; z[j] && z[j]!='/'; ++j){}
    if( z[j]!='/' ) break;
    z[j] = 0;
    frc = fsl_stat(z, &fst, false);
    if(frc){
      /* A not[-yet]-existing path element is okay */
      break;
    }
    if(FSL_FSTAT_TYPE_DIR!=fst.type){
      *errLen = j;
      break;
    }
    z[j] = '/';
    i = j;
  }
  end:
  fsl_cx_scratchpad_yield(f, fn);
  return rc;
}

int fsl_ckout_safe_file_check(fsl_cx *f, char const * zFilename){
  if(!fsl_needs_ckout(f)) return FSL_RC_NOT_A_CKOUT;
  int rc = 0;
  fsl_buffer * const fn = fsl_cx_scratchpad(f);
  if(!fsl_is_absolute_path(zFilename)){
    rc = fsl_file_canonical_name2(f->ckout.dir, zFilename, fn, false);
    if(rc) goto end;
    zFilename = fsl_buffer_cstr(fn);
  }else if(!fsl_is_rooted_in_ckout(f, zFilename)){
    rc = fsl_cx_err_set(f, FSL_RC_MISUSE, "Path is not rooted at the "
                        "current checkout directory: %s", zFilename);
    goto end;
  }

  fsl_size_t errLen = 0;
  rc = fsl_ckout_nondir_file_check(f, zFilename, &errLen);
  if(rc) goto end /* OOM */;
  else if(errLen){
    rc = fsl_cx_err_set(f, FSL_RC_TYPE, "Directory part of path refers "
                        "to a non-directory: %.*s",
                        (int)errLen, zFilename);
  }
  end:
  fsl_cx_scratchpad_yield(f, fn);
  return rc;
}

bool fsl_is_rooted_in_ckout(fsl_cx *f, char const *zAbsPath){
  return f->ckout.dir
    ? 0==fsl_strncmp(zAbsPath, f->ckout.dir, f->ckout.dirLen)
    /* ^^^ fossil(1) uses stricmp() there, but that's a bug. However,
       NOT using stricmp() on case-insensitive filesystems is arguably
       also a bug. */
    : false;
}

int fsl_is_rooted_in_ckout2(fsl_cx *f, char const *zAbsPath){
  int rc = 0;
  if(!fsl_is_rooted_in_ckout(f, zAbsPath)){
    rc = fsl_cx_err_set(f, FSL_RC_RANGE, "Path is not rooted "
                        "in the current checkout: %s",
                        zAbsPath);
  }
  return rc;
}

int fsl_ckout_symlink_create(fsl_cx * f, char const *zTgtFile,
                             char const * zLinkFile){
  if(!fsl_needs_ckout(f)) return FSL_RC_NOT_A_CKOUT;
  int rc = 0;
  fsl_buffer * const fn = fsl_cx_scratchpad(f);
  if(!fsl_is_absolute_path(zLinkFile)){
    rc = fsl_file_canonical_name2(f->ckout.dir, zLinkFile, fn, false);
    if(rc) goto end;
    zLinkFile = fsl_buffer_cstr(fn);
  }else if(0!=(rc = fsl_is_rooted_in_ckout2(f, zLinkFile))){
    goto end;
  }
  fsl_buffer * const b = fsl_cx_scratchpad(f);
  rc = fsl_buffer_append(b, zTgtFile, -1);
  if(!rc){
    rc = fsl_buffer_to_filename(b, fsl_buffer_cstr(fn));
  }
  fsl_cx_scratchpad_yield(f, b);
  end:
  fsl_cx_scratchpad_yield(f, fn);
  return rc;
}

/**
   Queues the directory part of the given filename into temp table
   fx_revert_rmdir for an eventual rmdir() attempt on it in
   fsl_revert_rmdir_fini().
*/
static int fsl_revert_rmdir_queue(fsl_cx * f, fsl_db * db, fsl_stmt * st,
                                  char const * zFilename){
  int rc = 0;
  if( !st->stmt ){
    rc = fsl_db_exec(db, "CREATE TEMP TABLE IF NOT EXISTS "
                     "fx_revert_rmdir(n TEXT PRIMARY KEY) "
                     "WITHOUT ROWID /* %s() */", __func__);
    if(rc) goto dberr;
    rc = fsl_db_prepare(db, st, "INSERT OR IGNORE INTO "
                        "fx_revert_rmdir(n) "
                        "VALUES(fsl_dirpart(?,0)) /* %s() */",
                        __func__);
    if(rc) goto dberr;
  }
  rc = fsl_stmt_bind_step(st, "s", zFilename);
  if(rc) goto dberr;
  end:
  return rc;
  dberr:
  rc = fsl_cx_uplift_db_error2(f, db, rc);
  goto end;
}

/**
   Attempts to rmdir all dirs queued by fsl_revert_rmdir_queue(). Silently
   ignores rmdir failure but will return non-0 for db errors.
*/
static int fsl_revert_rmdir_fini(fsl_cx * f, fsl_db * db){
  int rc;
  fsl_stmt st = fsl_stmt_empty;
  fsl_buffer * const b = fsl_cx_scratchpad(f);
  rc = fsl_db_prepare(db, &st,
                      "SELECT fsl_ckout_dir()||n "
                      "FROM fx_revert_rmdir "
                      "ORDER BY length(n) DESC /* %s() */",
                      __func__);
  if(rc) goto dberr;
  while(FSL_RC_STEP_ROW == fsl_stmt_step(&st)){
    fsl_size_t nDir = 0;
    char const * zDir = fsl_stmt_g_text(&st, 0, &nDir);
    fsl_buffer_reuse(b);
    rc = fsl_buffer_append(b, zDir, (fsl_int_t)nDir);
    if(rc) break;
    fsl_ckout_rm_empty_dirs(f, b);
  }
  end:
  fsl_cx_scratchpad_yield(f, b);
  fsl_stmt_finalize(&st);
  return rc;
  dberr:
  rc = fsl_cx_uplift_db_error2(f, db, rc);
  goto end;
}

int fsl_ckout_revert( fsl_cx * f, fsl_ckout_revert_opt const * opt ){
  /**
     Reminder to whoever works on this code: the initial
     implementation was done almost entirely without the benefit of
     looking at fossil's implementation, thus this code is notably
     different from fossil's. If any significant misbehaviors are
     found here, vis a vis fossil, it might be worth reverting (as it
     were) to that implementation.
  */
  int rc;
  fsl_db * const db = fsl_needs_ckout(f);
  fsl_buffer * fname = 0;
  char const * zNorm = 0;
  fsl_id_t const vid = f->ckout.rid;
  bool inTrans = false;
  fsl_stmt q = fsl_stmt_empty;
  fsl_stmt vfUpdate = fsl_stmt_empty;
  fsl_stmt qRmdir = fsl_stmt_empty;
  fsl_buffer * sql = 0;
  if(!db) return FSL_RC_NOT_A_CKOUT;
  assert(vid>=0);
  if(!opt->vfileIds && opt->filename && *opt->filename){
    fname = fsl_cx_scratchpad(f);
    rc = fsl_ckout_filename_check(f, opt->relativeToCwd,
                                  opt->filename, fname);
    if(rc){
      fsl_cx_scratchpad_yield(f, fname);
      return rc;
    }
    zNorm = fsl_buffer_cstr(fname);
    /* MARKER(("fsl_ckout_unmanage(%d, %s) ==> %s\n", opt->relativeToCwd, opt->filename, zNorm)); */
    assert(zNorm);
    if(fname->used) fsl_buffer_strip_slashes(fname);
    if(1==fname->used && '.'==*zNorm){
      /* Special case: handle "." from ckout root intuitively */
      fsl_buffer_reuse(fname);
      assert(0==*zNorm);
    }
  }
  rc = fsl_db_transaction_begin(db);
  if(rc) goto dberr;
  inTrans = true;
  if(opt->scanForChanges){
    rc = fsl_vfile_changes_scan(f, 0, 0);
    if(rc) goto end;
  }
  sql = fsl_cx_scratchpad(f);
  rc = fsl_buffer_appendf(sql, 
                          "SELECT id, rid, deleted, "
                          "fsl_ckout_dir()||pathname, "
                          "fsl_ckout_dir()||origname "
                          "FROM vfile WHERE vid=%" FSL_ID_T_PFMT " ",
                          vid);
  if(rc) goto end;
  if(zNorm && *zNorm){
    rc = fsl_buffer_appendf(sql,
                            "AND CASE WHEN %Q='' THEN 1 "
                            "ELSE ("
                            "     fsl_match_vfile_or_dir(pathname,%Q) "
                            "  OR fsl_match_vfile_or_dir(origname,%Q)"
                            ") END",
                            zNorm, zNorm, zNorm);
    if(rc) goto end;
  }else if(opt->vfileIds){
    rc = fsl_ckout_bag_to_ids(f, db, "fx_revert_id", opt->vfileIds);
    if(rc) goto end;
    rc = fsl_buffer_append(sql, "AND id IN fx_revert_id", -1);
    if(rc) goto end;
  }else{
    rc = fsl_buffer_append(sql,
                           "AND ("
                           " chnged<>0"
                           " OR deleted<>0"
                           " OR rid=0"
                           " OR coalesce(origname,pathname)"
                           "    <>pathname"
                           ")", -1);
  }
  assert(!rc);
  rc = fsl_db_prepare(db, &q, "%b /* %s() */", sql, __func__);
  fsl_cx_scratchpad_yield(f, sql);
  sql = 0;
  if(rc) goto dberr;
  if((!zNorm || !*zNorm) && !opt->vfileIds){
    rc = fsl_ckout_clear_merge_state(f);
    if(rc) goto end;
  }
  while((FSL_RC_STEP_ROW==fsl_stmt_step(&q))){
    fsl_id_t const id = fsl_stmt_g_id(&q, 0);
    fsl_id_t const rid = fsl_stmt_g_id(&q, 1);
    int32_t const deleted = fsl_stmt_g_int32(&q, 2);
    char const * const zName = fsl_stmt_g_text(&q, 3, NULL);
    char const * const zNameOrig = fsl_stmt_g_text(&q, 4, NULL);
    bool const renamed =
      zNameOrig ? !!fsl_strcmp(zName, zNameOrig) : false;
    fsl_ckout_revert_e changeType = FSL_REVERT_NONE;
    if(!rid){ // Added but not yet checked in.
      rc = fsl_db_exec(db, "DELETE FROM vfile WHERE id=%" FSL_ID_T_PFMT,
                       id);
      if(rc) goto dberr;
      changeType = FSL_REVERT_UNMANAGE;
    }else{
      int wasWritten = 0;
      if(renamed){
        if((rc=fsl_mkdir_for_file(zNameOrig, true))){
          rc = fsl_cx_err_set(f, rc, "mkdir() failed for file: %s",
                              zNameOrig);
          break;
        }
        /* Move, if possible, the new name back over the original
           name. This will possibly allow fsl_vfile_to_ckout() to
           avoid having to load that file's contents and overwrite
           it. */
        int mvCheck = fsl_stat(zName, NULL, false);
        if(0==mvCheck || FSL_RC_NOT_FOUND==mvCheck){
          mvCheck = fsl_file_unlink(zNameOrig);
          if(0==mvCheck || FSL_RC_NOT_FOUND==mvCheck){
            if(0==fsl_file_rename(zName, zNameOrig)){
              rc = fsl_revert_rmdir_queue(f, db, &qRmdir, zName);
              if(rc) break;
            }
          }
        }
        /* Ignore any errors: this operation is an optimization,
           not a requirement. Worse case, the entry with the old
           name is left in the filesystem. */
      }
      if(!vfUpdate.stmt){
        rc = fsl_db_prepare(db, &vfUpdate,
                            "UPDATE vfile SET chnged=0, deleted=0, "
                            "pathname=coalesce(origname,pathname), "
                            "origname=NULL "
                            "WHERE id=?1 /*%s()*/", __func__);
        if(rc) goto dberr;
      }
      rc = fsl_stmt_bind_step(&vfUpdate, "R", id)
        /* Has to be done before fsl_vfile_to_ckout() because that
           function writes to vfile.pathname. */;
      if(rc) goto dberr;
      rc = fsl_vfile_to_ckout(f, id, &wasWritten);
      if(rc) break;
      if(opt->callback){
        if(renamed){
          changeType = FSL_REVERT_RENAME;
        }else if(wasWritten){
          changeType = (2==wasWritten)
            ? FSL_REVERT_CONTENTS
            : FSL_REVERT_PERMISSIONS;
        }else if(deleted){
          changeType = FSL_REVERT_REMOVE;
        }
      }
    }
    if(opt->callback && FSL_REVERT_NONE!=changeType){
      char const * name = renamed ? zNameOrig : zName;
      rc = opt->callback(&name[f->ckout.dirLen],
                         changeType, opt->callbackState);
      if(rc) break;
    }
  }/*step() loop*/
  end:
  if(fname) fsl_cx_scratchpad_yield(f, fname);
  if(sql) fsl_cx_scratchpad_yield(f, sql);
  fsl_stmt_finalize(&q);
  fsl_stmt_finalize(&vfUpdate);
  if(qRmdir.stmt){
    fsl_stmt_finalize(&qRmdir);
    if(!rc) rc = fsl_revert_rmdir_fini(f, db);
    fsl_db_exec(db, "DROP TABLE IF EXISTS fx_revert_rmdir /* %s() */",
                __func__);
  }
  if(opt->vfileIds){
    fsl_db_exec_multi(db, "DROP TABLE IF EXISTS fx_revert_id "
                      "/* %s() */", __func__)
      /* Ignoring result code */;
  }
  if(inTrans){
    int const rc2 = fsl_db_transaction_end(db, !!rc);
    if(!rc) rc = rc2;
  }
  return rc;
  dberr:
  assert(rc);
  rc = fsl_cx_uplift_db_error2(f, db, rc);
  goto end;
}

int fsl_ckout_vfile_ids( fsl_cx * f, fsl_id_t vid,
                         fsl_id_bag * dest, char const * zName,
                         bool relativeToCwd, bool changedOnly ) {
  if(!fsl_needs_ckout(f)) return FSL_RC_NOT_A_CKOUT;
  fsl_buffer * const canon = fsl_cx_scratchpad(f);
  int rc = fsl_ckout_filename_check(f, relativeToCwd, zName, canon);
  if(!rc){
    fsl_buffer_strip_slashes(canon);
    rc = fsl_filename_to_vfile_ids(f, vid, dest,
                                   fsl_buffer_cstr(canon),
                                   changedOnly);
  }
  fsl_cx_scratchpad_yield(f, canon);
  return rc;
}

#undef MARKER
