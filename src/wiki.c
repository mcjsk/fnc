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
  This file implements wiki-related parts of the library.
*/
#include "fossil-scm/fossil-internal.h"
#include <assert.h>

/* Only for debugging */
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)


int fsl_wiki_names_get( fsl_cx * f, fsl_list * tgt ){
  fsl_db * db = fsl_needs_repo(f);
  if(!f || !tgt) return FSL_RC_MISUSE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  else {
    int rc = fsl_db_select_slist( db, tgt,
                                  "SELECT substr(tagname,6) AS name "
                                  "FROM tag "
                                  "WHERE tagname GLOB 'wiki-*' "
                                  "ORDER BY lower(name)");
    if(rc && db->error.code && !f->error.code){
      fsl_cx_uplift_db_error(f, db);
    }
    return rc;
  }
}

int fsl_wiki_latest_rid( fsl_cx * f, char const * pageName, fsl_id_t * rid ){
  fsl_db * db = f ? fsl_needs_repo(f) : NULL;
  if(!f || !pageName) return FSL_RC_MISUSE;
  else if(!*pageName) return FSL_RC_RANGE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  else return fsl_db_get_id(db, rid,
                            "SELECT x.rid FROM tag t, tagxref x "
                            "WHERE x.tagid=t.tagid "
                            "AND t.tagname='wiki-%q' "
                            "ORDER BY mtime DESC LIMIT 1",
                            pageName);
}

bool fsl_wiki_page_exists(fsl_cx * f, char const * pageName){
  fsl_id_t rid = 0;
  return (0==fsl_wiki_latest_rid(f, pageName, &rid))
    && (rid>0);
}

int fsl_wiki_load_latest( fsl_cx * f, char const * pageName, fsl_deck * d ){
  fsl_db * db = f ? fsl_needs_repo(f) : NULL;
  if(!f || !pageName || !d) return FSL_RC_MISUSE;
  else if(!*pageName) return FSL_RC_RANGE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  else{
    fsl_id_t rid = 0;
    int rc = fsl_wiki_latest_rid(f, pageName, &rid);
    if(rc) return rc;
    else if(0==rid) return FSL_RC_NOT_FOUND;
    return fsl_deck_load_rid( f, d, rid, FSL_SATYPE_WIKI);
  }
}

int fsl_wiki_foreach_page( fsl_cx * f, fsl_deck_visitor_f cb, void * state ){
  fsl_db * db = f ? fsl_needs_repo(f) : NULL;
  if(!f || !cb) return FSL_RC_MISUSE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  else{
    fsl_stmt st = fsl_stmt_empty;
    fsl_stmt names = fsl_stmt_empty;
    int rc;
    char doBreak = 0;
    rc = fsl_db_prepare(db, &names,
                        "SELECT substr(tagname,6) AS name "
                        "FROM tag "
                        "WHERE tagname GLOB 'wiki-*' "
                        "ORDER BY lower(name)");
    if(rc) return rc;
    while( !doBreak && !rc
           && (FSL_RC_STEP_ROW==fsl_stmt_step(&names))){
      fsl_size_t nameLen = 0;
      char const * pageName = fsl_stmt_g_text(&names, 0, &nameLen);
      if(!st.stmt){
        rc = fsl_db_prepare(db, &st,
                            "SELECT x.rid AS mrid FROM tag t, tagxref x "
                            "WHERE x.tagid=t.tagid "
                            "AND t.tagname='wiki-'||? "
                            "ORDER BY mtime DESC LIMIT 1");
        if(rc) goto end;
      }
      rc = fsl_stmt_bind_text(&st, 1, pageName, (fsl_int_t)nameLen, 0);
      if(rc) break;
      rc = fsl_stmt_step(&st);
      assert(FSL_RC_STEP_ROW==rc);
      if(FSL_RC_STEP_ROW==rc){
        fsl_deck d = fsl_deck_empty;
        fsl_id_t rid = fsl_stmt_g_id(&st, 0);
        rc = fsl_deck_load_rid( f, &d, rid, FSL_SATYPE_WIKI);
        if(!rc){
          rc = cb(f, &d, state);
          if(FSL_RC_BREAK==rc){
            rc = 0;
            doBreak = 1;
          }
        }
        fsl_deck_finalize(&d);
      }
      fsl_stmt_reset(&st);
    }
    end:
    fsl_stmt_finalize(&st);
    fsl_stmt_finalize(&names);
    return rc;
  }
}

int fsl_wiki_save(fsl_cx * f, char const * pageName,
                  fsl_buffer const * b,
                  char const * userName,
                  char const * mimeType,
                  fsl_wiki_save_mode_t createPolicy ){
  fsl_db * db = f ? fsl_needs_repo(f) : NULL;
  if(!f || !pageName || !b) return FSL_RC_MISUSE;
  else if(!*pageName) return FSL_RC_RANGE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  else{
    fsl_deck d = fsl_deck_empty;
    fsl_id_t parentRid = 0;
    int rc = fsl_wiki_latest_rid(f, pageName, &parentRid);
    double mtime;
    if(rc) return rc;
    else if((FSL_WIKI_SAVE_MODE_UPDATE==createPolicy)
            && !parentRid){
      return fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                            "No such wiki page: %s",
                            pageName);
    }
    else if((FSL_WIKI_SAVE_MODE_CREATE==createPolicy)
            && (parentRid>0)){
      return fsl_cx_err_set(f, FSL_RC_ALREADY_EXISTS,
                            "Wiki page already exists: %s",
                            pageName);
    }
    mtime = fsl_db_julian_now(db);
    fsl_deck_init(f, &d, FSL_SATYPE_WIKI);
    rc = fsl_deck_D_set(&d, mtime);
    assert(!rc);
    rc = fsl_deck_L_set(&d, pageName, -1);
    if(!rc && mimeType && *mimeType){
      rc = fsl_deck_N_set(&d, mimeType, -1);
    }
    if( !rc && parentRid ){
      char * zUuid = fsl_rid_to_uuid(f, parentRid);
      if(!zUuid){
        rc = FSL_RC_OOM;
      }else{
        rc = fsl_deck_P_add(&d,zUuid);
        fsl_free(zUuid);
      }
    }
    if(rc) goto end;
    {
      char * u = NULL;
      if(!userName) userName = fsl_cx_user_get(f);
      if(!userName){
        u = fsl_guess_user_name();
        if(!u) rc = FSL_RC_OOM;
      }
      if(!rc) rc = fsl_deck_U_set(&d, u ? u : userName);
      if(u) fsl_free(u);
      if(rc) goto end;
    }
    rc = fsl_deck_W_set(&d, fsl_buffer_cstr(b), (fsl_int_t)b->used);
#if 0
    fsl_deck_output(f, &d, fsl_output_f_FILE, stdout);
#endif
    if(!rc) rc = fsl_deck_save(&d, 0);
    end:
    fsl_deck_finalize(&d);
    return rc;
  }
}

#undef MARKER
