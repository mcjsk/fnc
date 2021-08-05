/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_AUTH_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_AUTH_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

  ******************************************************************************
  This file declares public APIs for handling fossil
  authentication-related tasks.
*/

#include "fossil-core.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
   If f has an opened repository, this function forms a hash from:

   "ProjectCode/zLoginName/zPw"

   (without the quotes)

   where ProjectCode is a repository-instance-dependent series of
   random bytes. The returned string is owned by the caller, who
   must eventually fsl_free() it. The project code is stored in
   the repository's config table under the key 'project-code', and
   this routine fetches that key if necessary.

   Potential TODO:

   - in fossil(1), this function generates a different result (it
   returns a copy of zPw) if the project code is not set, under
   the assumption that this is "the first xfer request of a
   clone."  Whether or not that will apply at this level to
   libfossil remains to be seen.

   TODO? Does fossil still use SHA1 for this?
*/
FSL_EXPORT char * fsl_sha1_shared_secret( fsl_cx * f, char const * zLoginName, char const * zPw );

/**
   Fetches the login group name (if any) for the given context's
   current repositorty db. If f has no opened repo, 0 is returned.

   If the repo belongs to a login group, its name is returned in the
   form of a NUL-terminated string. The returned value (which may be
   0) is owned by the caller, who must eventually fsl_free() it. The
   value (unlike in fossil(1)) is not cached because it may change
   via modification of the login group.
*/
FSL_EXPORT char * fsl_repo_login_group_name(fsl_cx * f);

/**
   Fetches the login cookie name associated with the current repository
   db, or 0 if no repository is opened.

   The returned (NUL-terminated) string is owned by the caller, who
   must eventually fsl_free() it. The value is not cached in f because
   it may change during the lifetime of a repo (if a login group is
   set or removed).

   The login cookie name is a string in the form "fossil-XXX", where
   XXX is the first 16 hex digits of either the repo's
   'login-group-code' or 'project-code' config values (in that order).
*/
FSL_EXPORT char * fsl_repo_login_cookie_name(fsl_cx * f);

/**
   Searches for a user ID (from the repo.user.uid DB field) for a given
   username and password. The password may be either its hashed form or
   non-hashed form (if it is not exactly 40 bytes long, that is!).

   On success, 0 is returned and *pId holds the ID of the
   user found (if any).  *pId will be set to 0 if no match for the
   name/password was found, or positive if a match was found.

   If any of the arguments are NULL, FSL_RC_MISUSE is returned. f must
   have an opened repo, else FSL_RC_NOT_A_REPO is returned.

*/
FSL_EXPORT int fsl_repo_login_search_uid(fsl_cx * f, char const * zUsername,
                                         char const * zPasswd, fsl_id_t * pId);

/**
   Clears all login state for the given user ID. If the ID is <=0 then
   ALL logins are cleared. Has no effect on the built-in pseudo-users.

   Returns non-0 on error, and not finding a matching user ID is not
   considered an error.

   f must have an opened repo, or FSL_RC_NOT_A_REPO is returned.

   TODO: there are currently no APIs for _setting_ the state this
   function clears!
*/
FSL_EXPORT int fsl_repo_login_clear( fsl_cx * f, fsl_id_t userId );


#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_AUTH_H_INCLUDED */
