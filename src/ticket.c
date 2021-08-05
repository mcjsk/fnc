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
  This file implements ticket-related parts of the library.
*/
#include "fossil-scm/fossil-internal.h"
#include <assert.h>
#include <string.h> /* memcmp() */

int fsl_cx_ticket_create_table(fsl_cx * f){
  fsl_db * db = f ? fsl_needs_repo(f) : NULL;
  int rc;
  if(!f) return FSL_RC_MISUSE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  rc = fsl_db_exec_multi(db,
                         "DROP TABLE IF EXISTS ticket;"
                         "DROP TABLE IF EXISTS ticketchng;"
                         );
  if(!rc){
    fsl_buffer * buf = fsl_cx_scratchpad(f);
    rc = fsl_cx_schema_ticket(f, buf);
    if(!rc){
      rc = fsl_db_exec_multi(db, "%b", buf);
    }
    fsl_cx_scratchpad_yield(f, buf);
  }
  if(rc && db->error.code && !f->error.code){
    fsl_cx_uplift_db_error(f, db);
  }
  return rc;
}

static int fsl_tkt_field_id(fsl_list const * jli, const char *zFieldName){
  int i;
  fsl_card_J const * jc;
  for(i=0; i<(int)jli->used; ++i){
    jc = (fsl_card_J const *)jli->list[i];
    if( !fsl_strcmp(zFieldName, jc->field) ) return i;
  }
  return -1;
}

int fsl_cx_ticket_load_fields(fsl_cx * f, bool forceReload){
  fsl_stmt q = fsl_stmt_empty;
  int i, rc = 0;
  fsl_list * li = &f->ticket.customFields;
  fsl_card_J * jc;
  fsl_db * db;
  if(li->used){
    if(!forceReload) return 0;
    fsl_card_J_list_free(li, 0);
    /* Fall through and reload ... */
  }
  if( !(db = fsl_needs_repo(f)) ){
    return FSL_RC_NOT_A_REPO;
  }
  rc = fsl_db_prepare(db, &q, "PRAGMA table_info(ticket)");
  if(!rc) while( FSL_RC_STEP_ROW==fsl_stmt_step(&q) ){
    char const * zFieldName = fsl_stmt_g_text(&q, 1, NULL);
    f->ticket.hasTicket = 1;
    if( 0==memcmp(zFieldName,"tkt_", 4)){
      if( 0==fsl_strcmp(zFieldName,"tkt_ctime")) f->ticket.hasCTime = 1;
      continue;
    }
    jc = fsl_card_J_malloc(0, zFieldName, NULL);
    if(!jc){
      rc = FSL_RC_OOM;
      break;
    }
    jc->flags = FSL_CARD_J_TICKET;
    rc = fsl_list_append(li, jc);
    if(rc){
      fsl_card_J_free(jc);
      break;
    }
  }
  fsl_stmt_finalize(&q);
  if(rc) goto end;

  rc = fsl_db_prepare(db, &q, "PRAGMA table_info(ticketchng)");
  if(!rc) while( FSL_RC_STEP_ROW==fsl_stmt_step(&q) ){
    char const * zFieldName = fsl_stmt_g_text(&q, 1, NULL);
    f->ticket.hasChng = 1;
    if( 0==memcmp(zFieldName,"tkt_", 4)){
      if( 0==fsl_strcmp(zFieldName,"tkt_rid")) f->ticket.hasChngRid = 1;
      continue;
    }
    if( (i=fsl_tkt_field_id(li, zFieldName)) >= 0){
      jc = (fsl_card_J*)li->list[i];
      jc->flags |= FSL_CARD_J_CHNG;
      continue;
    }
    jc = fsl_card_J_malloc(0, zFieldName, NULL);
    if(!jc){
      rc = FSL_RC_OOM;
      break;
    }
    jc->flags = FSL_CARD_J_CHNG;
    rc = fsl_list_append(li, jc);
    if(rc){
      fsl_card_J_free(jc);
      break;
    }
  }
  fsl_stmt_finalize(&q);
  end:
  if(!rc){
    fsl_list_sort(li, fsl_qsort_cmp_J_cards);
  }
  return rc;
}
