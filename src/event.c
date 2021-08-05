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
  This file implements technote (formerly known as event)-related
  parts of the library.
*/
#include "fossil-scm/fossil-internal.h"
#include <assert.h>

/* Only for debugging */
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)


int fsl_event_ids_get( fsl_cx * f, fsl_list * tgt ){
  fsl_db * db = fsl_needs_repo(f);
  if(!f || !tgt) return FSL_RC_MISUSE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  else {
    int rc = fsl_db_select_slist( db, tgt,
                                  "SELECT substr(tagname,7) AS n "
                                  "FROM tag "
                                  "WHERE tagname GLOB 'event-*' "
                                  "ORDER BY n");
    if(rc && db->error.code && !f->error.code){
      fsl_cx_uplift_db_error(f, db);
    }
    return rc;
  }
}


#undef MARKER
