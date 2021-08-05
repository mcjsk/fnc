/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_FORUM_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_FORUM_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

  ******************************************************************************
  This file declares public APIs for working with fossil-managed content.
*/

#include "fossil-core.h" /* MUST come first b/c of config macros */

/**
   If the given fossil context has a db opened, this function
   installs, if needed, the forum-related schema and returns 0 on
   success (or if no installation was needed). If f has no repository
   opened, FSL_RC_NOT_A_REPO is returned. Some other FSL_RC_xxx value
   is returned if there is a db-level error during installation.
*/
int fsl_repo_install_schema_forum(fsl_cx *f);


#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_FORUM_H_INCLUDED */
