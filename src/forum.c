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

/***************************************************************************
  This file houses the code for forum-level APIS.
*/
#include <assert.h>

#include "fossil-scm/fossil-internal.h"

/* Only for debugging */
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)

int fsl_repo_install_schema_forum(fsl_cx *f){
  int rc;
  fsl_db * db = fsl_needs_repo(f);
  if(!db) return FSL_RC_NOT_A_REPO;
  if(fsl_db_table_exists(db, FSL_DBROLE_REPO, "forumpost")){
    return 0;
  }
  MARKER(("table not exists?\n"));
  rc = fsl_db_exec_multi(db, "%s",fsl_schema_forum());
  if(rc){
    rc = fsl_cx_uplift_db_error(f, db);
  }
  return rc;
}



#undef MARKER
