/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  
  *****************************************************************************
  This file implements tag-related parts of the library.
*/
#include "fossil-scm/fossil-internal.h"
#include <assert.h>

/* Only for debugging */
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)


void fsl_card_T_clean(fsl_card_T *t){
  if(t){
    fsl_free(t->uuid);
    t->uuid = NULL;
    fsl_free(t->name);
    t->name = NULL;
    fsl_free(t->value);
    t->value = NULL;
    *t = fsl_card_T_empty;
  }
}

void fsl_card_T_free(fsl_card_T *t){
  if(t){
    fsl_card_T_clean(t);
    fsl_free(t);
  }
}

fsl_card_T * fsl_card_T_malloc(fsl_tagtype_e tagType,
                               char const * uuid,
                               char const * name,
                               char const * value){
  fsl_card_T * t;
  int const uuidLen = uuid ? fsl_is_uuid(uuid) : 0;
  if(uuid && !uuidLen) return NULL;
  t = (fsl_card_T *)fsl_malloc(sizeof(fsl_card_T));
  if(t){
    int rc = 0;
    *t = fsl_card_T_empty;
    t->type = tagType;
    if(uuid && *uuid){
      t->uuid = fsl_strndup(uuid, uuidLen);
      if(!t->uuid) rc = FSL_RC_OOM;
    }
    if(!rc && name && *name){
      t->name = fsl_strdup(name);
      if(!t->name){
        rc = FSL_RC_OOM;
      }
    }
    if(!rc && value && *value){
      t->value = fsl_strdup(value);
      if(!t->value){
        rc = FSL_RC_OOM;
      }
    }
    if(rc){
      fsl_card_T_free(t);
      t = NULL;
    }
  }
  return t;
}

fsl_id_t fsl_tag_id( fsl_cx * f, char const * tag, bool create ){
  fsl_db * db = fsl_cx_db_repo(f);
  int64_t id = 0;
  int rc;
  if(!db || !tag) return FSL_RC_MISUSE;
  else if(!*tag) return FSL_RC_RANGE;
  rc = fsl_db_get_int64( db, &id,
                         "SELECT tagid FROM tag WHERE tagname=%Q",
                         tag);
  if(!rc && (0==id) && create){
    /* Not found - create one. */
    rc = fsl_db_exec(db, "INSERT INTO tag(tagname) VALUES(%Q)",
                     tag);
    if(!rc) id = fsl_db_last_insert_id(db);
  }

  if(rc){
    assert(0==id);
    fsl_cx_uplift_db_error( f, db );
    id = -1;
  }
  return id;
    
}

int fsl_tag_propagate(fsl_cx *f, fsl_tagtype_e tagType,
                      fsl_id_t pid, fsl_id_t tagid,
                      fsl_id_t origId, const char *zValue,
                      double mtime){
  int rc;
  fsl_pq queue = fsl_pq_empty     /* Queue of artifacts to be tagged */;
  fsl_stmt s = fsl_stmt_empty     /* Query the children of :pid to which to propagate */;
  fsl_stmt ins = fsl_stmt_empty   /* INSERT INTO tagxref */;
  fsl_stmt eventupdate = fsl_stmt_empty  /* UPDATE event */;
  fsl_db * const db = fsl_needs_repo(f);

  assert(FSL_TAGTYPE_CANCEL==tagType || FSL_TAGTYPE_PROPAGATING==tagType);
  assert(f);
  assert(db);
  assert(pid>0);
  assert(tagid>0);

  if((pid<=0 || tagid<=0)
     ||(FSL_TAGTYPE_PROPAGATING!=tagType && FSL_TAGTYPE_CANCEL!=tagType)
     || (FSL_TAGTYPE_PROPAGATING==tagType && origId<=0)){
    return FSL_RC_RANGE;
  }
  else if(!db) return FSL_RC_NOT_A_REPO;

  rc = fsl_pq_insert(&queue, pid, 0.0, NULL);
  if(rc) return rc;

  rc = fsl_db_prepare(db, &s,
                      "SELECT cid, plink.mtime,"
                      "       coalesce(srcid=0 AND "
                      "       tagxref.mtime<:mtime, %d) AS doit"
                      "  FROM plink LEFT JOIN tagxref "
                      "       ON cid=rid AND tagid=%"FSL_ID_T_PFMT
                      " WHERE pid=:pid AND isprim",
                      tagType==FSL_TAGTYPE_PROPAGATING,
                      (fsl_id_t)tagid);
  if(rc) goto end;
  rc = fsl_stmt_bind_double_name(&s, ":mtime", mtime);

  if(FSL_TAGTYPE_PROPAGATING==tagType){
    /* Set the propagated tag marker on artifact :rid */
    assert(origId>0);
    rc = fsl_db_prepare(db, &ins,
                        "REPLACE INTO tagxref("
                        "  tagid, tagtype, srcid, "
                        "  origid, value, mtime, rid"
                        ") VALUES("
                        "%"FSL_ID_T_PFMT"," /* tagid */
                        "%d,"  /*tagtype*/
                        "0," /*srcid*/
                        "%"FSL_ID_T_PFMT"," /*origId */
                        "%Q," /* zValue */
                        ":mtime,"
                        ":rid"
                        ")",
                        (fsl_id_t)tagid,
                        (int)FSL_TAGTYPE_PROPAGATING,
                        (fsl_id_t)origId, zValue);
    if(!rc) rc = fsl_stmt_bind_double_name(&ins, ":mtime", mtime);
  }else{
    /* Remove all references to the tag from checkin :rid */
    zValue = NULL;
    rc = fsl_db_prepare(db, &ins,
                        "DELETE FROM tagxref WHERE "
                        "tagid=%"FSL_ID_T_PFMT
                        " AND rid=:rid", (fsl_id_t)tagid);
  }
  if(rc) goto end;
  if( tagid==FSL_TAGID_BGCOLOR ){
    rc = fsl_db_prepare(db, &eventupdate,
                        "UPDATE event SET bgcolor=%Q "
                        "WHERE objid=:rid", zValue);
    if(rc) goto end;
  }

  while( 0 != (pid = fsl_pq_extract(&queue,NULL))){
    fsl_stmt_bind_id_name(&s, ":pid", pid);
#if 0
    MARKER(("Walking over pid %"FSL_ID_T_PFMT
            ", queue.used=%"FSL_SIZE_T_PFMT"\n", pid, queue.used));
#endif
    while( !rc && (FSL_RC_STEP_ROW == fsl_stmt_step(&s)) ){
      int32_t const doit = fsl_stmt_g_int32(&s, 2);
      if(doit){
        fsl_id_t cid = fsl_stmt_g_id(&s, 0);
        double mtime = fsl_stmt_g_double(&s,1);
        assert(cid>0);
        assert(mtime>0.0);
        rc = fsl_pq_insert(&queue, cid, mtime, NULL);
        if(!rc) rc = fsl_stmt_bind_id_name(&ins, ":rid", cid);
        if(rc) goto end;
        else {
          rc = fsl_stmt_step(&ins);
          if(FSL_RC_STEP_DONE != rc) goto end;
          rc = 0;
        }
        fsl_stmt_reset(&ins);
        if( FSL_TAGID_BGCOLOR == tagid ){
          rc = fsl_stmt_bind_id_name(&eventupdate, ":rid", cid);
          if(!rc){
            rc = fsl_stmt_step(&eventupdate);
            if(FSL_RC_STEP_DONE != rc) goto end;
            rc = 0;
          }
          fsl_stmt_reset(&eventupdate);
        }else if( FSL_TAGID_BRANCH == tagid ){
          rc = fsl_repo_leaf_eventually_check(f, cid);
        }
      }
    }
    fsl_stmt_reset(&s);
  }
  end:
  fsl_stmt_finalize(&s);
  fsl_stmt_finalize(&ins);
  fsl_stmt_finalize(&eventupdate);
  fsl_pq_clear(&queue);
  return rc;
  
}

int fsl_tag_propagate_all(fsl_cx * f, fsl_id_t pid){
  fsl_stmt q = fsl_stmt_empty;
  int rc;
  fsl_db * const db = fsl_cx_db_repo(f);
  if(!f) return FSL_RC_MISUSE;
  else if(pid<=0) return FSL_RC_RANGE;
  assert(db);
  rc = fsl_db_prepare(db, &q,
                      "SELECT tagid, tagtype, mtime, "
                      "value, origid FROM tagxref"
                      " WHERE rid=%"FSL_ID_T_PFMT,
                      pid);
  while( !rc && (FSL_RC_STEP_ROW == fsl_stmt_step(&q)) ){
    fsl_id_t const tagid = fsl_stmt_g_id(&q, 0);
    int32_t tagtype = fsl_stmt_g_int32(&q, 1);
    double const mtime = fsl_stmt_g_double(&q, 2);
    const char *zValue = fsl_stmt_g_text(&q, 3, NULL);
    fsl_id_t const origid = fsl_stmt_g_id(&q, 4);
    if( FSL_TAGTYPE_ADD==tagtype ) tagtype = FSL_TAGTYPE_CANCEL
      /* For propagating purposes */;
    rc = fsl_tag_propagate(f, tagtype, pid, tagid,
                           origid, zValue, mtime);
  }
  fsl_stmt_finalize(&q);
  return rc;
}


int fsl_tag_insert( fsl_cx * f, fsl_tagtype_e tagtype,
                    char const * zTag, char const * zValue,
                    fsl_id_t srcId, double mtime,
                    fsl_id_t rid, fsl_id_t *outRid ){
  fsl_db * db = f ? fsl_cx_db_repo(f) : NULL;
  fsl_stmt q = fsl_stmt_empty;
  fsl_id_t tagid;
  int rc = 0;
  char const * zCol;
  if(!f || !zTag) return FSL_RC_MISUSE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  tagid = fsl_tag_id(f, zTag, 1);
  if(tagid<0){
    assert(f->error.code);
    return f->error.code;
  }
  if( mtime<=0.0 ){
    mtime = fsl_db_julian_now(db);
    if(mtime<0) return FSL_RC_DB;
  }

  rc = fsl_db_prepare(db, &q, /* TODO: cached query */
                      "SELECT 1 FROM tagxref"
                      " WHERE tagid=%"FSL_ID_T_PFMT
                      "   AND rid=%"FSL_ID_T_PFMT
                      "   AND mtime>=?",
                      (fsl_id_t)tagid, (fsl_id_t)rid);
  if(!rc) rc = fsl_stmt_bind_double(&q, 1, mtime);
  if(!rc) rc = fsl_stmt_step(&q);
  if( FSL_RC_STEP_ROW == rc ){
    /*
      Another entry that is more recent already exists. Do nothing.

      Reminder: the above policy is from the original implementation.
      We might(?) want to return FSL_RC_ACCESS or
      FSL_RC_ALREADY_EXISTS here. The current behaviour seems harmless
      enough, though.
    */
    if(outRid) *outRid = tagid;
    fsl_stmt_finalize(&q);
    return 0;
  }else if(FSL_RC_STEP_DONE != rc){
    goto end;
  }
  fsl_stmt_finalize(&q);
  rc = fsl_db_prepare(db, &q,
                      "REPLACE INTO tagxref"
                      "(tagid,tagtype,srcId,origid,value,mtime,rid) "
                      "VALUES("
                      "%"FSL_ID_T_PFMT"," /* tagid */
                      "%d," /* tagtype */
                      "%"FSL_ID_T_PFMT"," /* srcid */
                      "%"FSL_ID_T_PFMT"," /* rid */
                      "%Q," /* zValue */
                      "?," /* mtime */
                      "%"FSL_ID_T_PFMT")" /* rid again */,
                      (fsl_id_t)tagid, (int)tagtype,
                      (fsl_id_t)srcId,
                      (fsl_id_t)rid, zValue, (fsl_id_t)rid
                      );
  if(!rc) fsl_stmt_bind_double(&q, 1, mtime);
  if(!rc) rc = fsl_stmt_step(&q);
  if(FSL_RC_STEP_DONE != rc) goto end;
  rc = 0;
  fsl_stmt_finalize(&q);

  if(FSL_TAGID_BRANCH == tagid ){
    rc = fsl_repo_leaf_eventually_check(f, rid);
    if(rc) goto end;
  }
#if 0
  /* Historical: we have valid use cases for the
     value here.
  */
  else if(FSL_TAGTYPE_CANCEL==tagtype){
    zValue = NULL;
  }
#endif

  zCol = NULL;
  switch(tagid){
    case FSL_TAGID_BGCOLOR:
      zCol = "bgcolor";
      break;
    case FSL_TAGID_COMMENT:
      zCol = "ecomment";
      break;
    case FSL_TAGID_USER: {
      zCol = "euser";
      break;
    }
    case FSL_TAGID_PRIVATE:
      rc = fsl_db_exec(db,
                       "INSERT OR IGNORE INTO "
                       "private(rid) VALUES"
                       "(%"FSL_ID_T_PFMT");",
                       (fsl_id_t)rid );
      if(rc) goto end;
      else break;
  }
  if( zCol ){
    rc = fsl_db_exec(db, "UPDATE event SET %s=%Q "
                     "WHERE objid=%"FSL_ID_T_PFMT,
                     zCol, zValue, (fsl_id_t)rid);
    if(rc) goto end;
#if 0
    /*
      Legacy: i don't want this behaviour in the lib right now
      (possibly never). And don't want to port it yet, either :/.
     */
    if( tagid==FSL_TAGID_COMMENT ){
      char *zCopy = fsl_strdup(zValue);
      wiki_extract_links(zCopy, rid, 0, mtime, 1, WIKI_INLINE);
      free(zCopy);
    }
#endif
  }

  if( FSL_TAGID_DATE == tagid ){
    rc = fsl_db_exec(db, "UPDATE event "
                     "SET mtime=julianday(%Q),"
                     "    omtime=coalesce(omtime,mtime)"
                     "WHERE objid=%"FSL_ID_T_PFMT,
                     zValue, (fsl_id_t)rid);
    if(rc) goto end;
  }
  if( FSL_TAGTYPE_ADD == tagtype ) tagtype = FSL_TAGTYPE_CANCEL
    /* For propagation purposes */;
  rc = fsl_tag_propagate(f, tagtype, rid, tagid,
                         rid, zValue, mtime);
  end:
  if(rc){
    fsl_stmt_finalize(&q);
  }else{
    assert(!q.stmt);
    if(outRid) *outRid = tagid;
  }
  return rc;
}

int fsl_tag_sym( fsl_cx * f,
                 fsl_tagtype_e tagType,
                 char const * symToTag,
                 char const * tagName,
                 char const * tagValue,
                 char const * userName,
                 double mtime,
                 fsl_id_t * outId ){
  if(!f || !tagName || !symToTag || !userName) return FSL_RC_MISUSE;
  else if(!*tagName || !*userName || !*symToTag) return FSL_RC_RANGE;
  else{
    fsl_id_t resolvedRid = 0;
    int rc;
    rc = fsl_sym_to_rid( f, symToTag, FSL_SATYPE_ANY, &resolvedRid );
    if(!rc){
      assert(resolvedRid>0);
      rc = fsl_tag_an_rid(f, tagType, resolvedRid,
                          tagName, tagValue, userName,
                          mtime, outId);
    }
    return rc;
  }
}

int fsl_tag_an_rid( fsl_cx * f,
                    fsl_tagtype_e tagType,
                    fsl_id_t idToTag,
                    char const * tagName,
                    char const * tagValue,
                    char const * userName,
                    double mtime,
                    fsl_id_t * outId ){
  fsl_db * dbR = f ? fsl_cx_db_repo(f) : NULL;
  fsl_deck c = fsl_deck_empty;
  char * resolvedUuid = NULL;
  fsl_buffer mfout = fsl_buffer_empty;

  int rc;
  if(!f || !tagName || !userName) return FSL_RC_MISUSE;
  else if(!*tagName || !*userName || (idToTag<=0)) return FSL_RC_RANGE;
  else if(!dbR) return FSL_RC_NOT_A_REPO;

  if(mtime<=0) mtime = fsl_db_julian_now(dbR);

  resolvedUuid = fsl_rid_to_uuid(f, idToTag);
  if(!resolvedUuid){
    return fsl_cx_err_set(f, FSL_RC_RANGE,
                          "Could not resolve UUID for "
                          "rid %"FSL_ID_T_PFMT".",
                          (fsl_id_t)idToTag);
  }
  assert(fsl_is_uuid(resolvedUuid));

#if 0
  tagRid = fsl_tag_id(f, tagName, 1);
  if(tagRid<=0){
    rc = f->error.rc
      ? f->error.rc
      : fsl_cx_err_set(f, FSL_RC_ERROR,
                       "Unknown error while fetching "
                       "ID for tag [%s].",
                       tagName);
    goto end;
  }
#endif

  fsl_deck_init(f, &c, FSL_SATYPE_CONTROL);
  rc = fsl_deck_T_add( &c, tagType, resolvedUuid,
                       tagName, tagValue );
  if(rc) goto end;

  rc = fsl_deck_D_set( &c, mtime );
  if(rc) goto end;

  rc = fsl_deck_U_set( &c, userName );
  if(rc) goto end;

  rc = fsl_deck_save( &c, fsl_content_is_private(f, idToTag) );
  end:
  fsl_free(resolvedUuid);
  fsl_buffer_clear(&mfout);
  if(!rc && outId){
    assert(c.rid>0);
    *outId = c.rid;
  }
  fsl_deck_clean(&c);
  return rc;
}

int fsl_branch_create(fsl_cx * f, fsl_branch_opt const * opt, fsl_id_t * newRid ){
  int rc;
  fsl_deck parent = fsl_deck_empty;
  fsl_deck deck = fsl_deck_empty;
  fsl_db * db = f ? fsl_needs_repo(f) : NULL;
  char const * user;
  char isPrivate;
  if(!f || !opt) return FSL_RC_MISUSE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  else if(opt->basisRid<=0 || !opt->name || !*opt->name){
    return FSL_RC_MISUSE;
  }
  else if(! (user = opt->user ? opt->user : f->repo.user)){
    rc = fsl_cx_err_set(f, FSL_RC_MISUSE,
                        "Could not determine fossil user name "
                        "for new branch [%s].", opt->name);
  }


  rc = fsl_deck_load_rid(f, &parent, opt->basisRid, FSL_SATYPE_CHECKIN);
  if(rc) goto end;

  assert(parent.rid==opt->basisRid);

  fsl_deck_init(f, &deck, FSL_SATYPE_CHECKIN);

  if(parent.B.uuid){
    rc = fsl_deck_B_set(&deck, parent.B.uuid);
  }

  rc = fsl_deck_D_set(&deck, opt->mtime>0 ? opt->mtime : fsl_db_julian_now(db));
  if(rc) goto end;

  /*
    We cannot simply transfer the list of F-cards from parent to deck
    because their content pointers (when the deck is parsed using fsl_deck_parse2())
    points to memory in parent.content;
  */
  for( fsl_size_t i = 0; i < parent.F.used; ++i){
    fsl_card_F const * fc = &parent.F.list[i];
    rc = fsl_deck_F_add(&deck, fc->name, fc->uuid, fc->perm, fc->priorName);
      if(rc) goto end;
  }

  rc = fsl_deck_U_set(&deck, user);
  if(rc) goto end;

  rc = fsl_deck_P_add(&deck, parent.uuid);
  if(rc) goto end;

  if(opt->comment && *opt->comment){
    rc = fsl_deck_C_set(&deck, opt->comment, -1);
  }else{
    fsl_buffer c = fsl_buffer_empty;
    rc = fsl_buffer_appendf(&c, "Created branch [%s].", opt->name);
    if(!rc){
      rc = fsl_deck_C_set(&deck, (char const *)c.mem, (fsl_int_t)c.used);
    }
    fsl_buffer_clear(&c);
  }
  if(rc) goto end;


#if 0
  /* This adds almost 18MB of allocations to my small test code! */
  if(deck.F.list.used){
    rc = fsl_deck_R_calc(&deck);
    if(rc) goto end;
  }
#else
  rc = fsl_deck_R_set(&deck, parent.R);
  if(rc) goto end;
#endif

  isPrivate = fsl_content_is_private(f, parent.rid) ? 1 : opt->isPrivate;
  if(isPrivate){
    rc = fsl_deck_T_add(&deck, FSL_TAGTYPE_ADD,
                        NULL, "private", NULL);
    if(rc) goto end;
  }

  if(opt->bgColor && ('#'==*opt->bgColor)){
    rc = fsl_deck_T_add(&deck, FSL_TAGTYPE_ADD, NULL, "bgcolor",
                        opt->bgColor);
    if(rc) goto end;
  }

  rc = fsl_deck_T_add(&deck, FSL_TAGTYPE_PROPAGATING,
                      NULL, "branch", opt->name);
  if(!rc){
    /* Add tag named sym-BRANCHNAME... */
    fsl_buffer * buf = fsl_cx_scratchpad(f);
    rc = fsl_buffer_appendf(buf, "sym-%s", opt->name);
    if(!rc){
      rc = fsl_deck_T_add(&deck, FSL_TAGTYPE_PROPAGATING,
                          NULL, fsl_buffer_cstr(buf), NULL);
    }
    fsl_cx_scratchpad_yield(f, buf);
  }
  if(rc) goto end;

#if 1
  rc = fsl_db_transaction_begin(db);
  if(rc) goto end;
  else{
    /* cancel all other symbolic tags (branch tags) */
    fsl_stmt q = fsl_stmt_empty;
    rc = fsl_db_prepare(db, &q,
                        "SELECT tagname FROM tagxref, tag"
                        " WHERE tagxref.rid=%"FSL_ID_T_PFMT
                        " AND tagxref.tagid=tag.tagid"
                        "   AND tagtype>0 AND tagname GLOB 'sym-*'"
                        " ORDER BY tagname",
                        (fsl_id_t)parent.rid);
    if(rc) goto end;
    while( FSL_RC_STEP_ROW==fsl_stmt_step(&q) ){
      const char *zTag = fsl_stmt_g_text(&q, 0, NULL);
      rc = fsl_deck_T_add(&deck, FSL_TAGTYPE_CANCEL,
                          NULL, zTag, "cancelled by branch.");
      if(rc) break;
    }
    fsl_stmt_finalize(&q);
    if(!rc){
      rc = fsl_deck_save(&deck, isPrivate);
      if(!rc){
        assert(deck.rid>0);
        rc = fsl_db_exec(db, "INSERT OR IGNORE INTO "
                         "unsent VALUES(%"FSL_ID_T_PFMT")",
                         (fsl_id_t)deck.rid);
        if(!rc){
          /* Make the parent a delta of this one. */
          rc = fsl_content_deltify(f, parent.rid, deck.rid, 0);
        }
      }
    }
    if(!rc) rc = fsl_db_transaction_commit(db);
    else fsl_db_transaction_rollback(db);
    if(rc) goto end;
  }
#else
  MARKER(("Generating (not saving) branch artifact:\n"));
  rc = fsl_deck_unshuffle(&deck, 0);
  if(rc) goto end;
  rc = fsl_deck_output(&deck, fsl_output_f_FILE, stdout, &f->error);
  if(rc) goto end;
#endif

  end:
  if(!rc && newRid) *newRid = deck.rid;
  else if(rc && !f->error.code){
    if(db->error.code) fsl_cx_uplift_db_error(f,db);
  }
  fsl_deck_finalize(&parent);
  fsl_deck_finalize(&deck);
  return rc;
}
                       

#undef MARKER
