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
  This file contains routines related to working with user authentication.
*/
#include "fossil-scm/fossil-internal.h"
#include "fossil-scm/fossil-hash.h"
#include "fossil-scm/fossil-confdb.h"


FSL_EXPORT char * fsl_sha1_shared_secret( fsl_cx * f, char const * zLoginName,
                                          char const * zPw ){
    if(!f || !zPw || !zLoginName) return 0;
    else{
        fsl_sha1_cx hash = fsl_sha1_cx_empty;
        unsigned char zResult[20];
        char zDigest[41];
        if(!f->cache.projectCode){
            f->cache.projectCode = fsl_config_get_text(f, FSL_CONFDB_REPO,
                                                       "project-code", 0);
            /*
              fossil(1) returns a copy of zPw here if !f->cache.projectCode,
              with the following comment:
            */
            /* On the first xfer request of a clone, the project-code is not yet
            ** known.  Use the cleartext password, since that is all we have.
            */
            if(!f->cache.projectCode) return 0;
        }
        fsl_sha1_update(&hash, f->cache.projectCode,
                        fsl_strlen(f->cache.projectCode));
        fsl_sha1_update(&hash, "/", 1);
        fsl_sha1_update(&hash, zLoginName, fsl_strlen(zLoginName));
        fsl_sha1_update(&hash, "/", 1);
        fsl_sha1_update(&hash, zPw, fsl_strlen(zPw));
        fsl_sha1_final(&hash, zResult);
        fsl_sha1_digest_to_base16(zResult, zDigest);
        return fsl_strndup( zDigest, FSL_STRLEN_SHA1 );
    }
}

FSL_EXPORT char * fsl_repo_login_group_name(fsl_cx * f){
  return f
    ? fsl_config_get_text(f, FSL_CONFDB_REPO,
                          "login-group-name", 0)
    : 0;
}

FSL_EXPORT char * fsl_repo_login_cookie_name(fsl_cx * f){
  fsl_db * db;
  if(!f || !(db = fsl_cx_db_repo(f))) return 0;
  else{
    char const * sql =
      "SELECT 'fossil-' || substr(value,1,16)"
      "  FROM config"
      " WHERE name IN ('project-code','login-group-code')"
      " ORDER BY name /*sort*/";
    return fsl_db_g_text(db, 0, sql);
  }
}

FSL_EXPORT int fsl_repo_login_search_uid(fsl_cx * f, char const * zUsername,
                                         char const * zPasswd,
                                         fsl_id_t * userId){
  int rc;
  char * zSecret;
  fsl_db * db;
  if(!f || !userId
     || !zUsername || !*zUsername
     || !zPasswd /*??? || !*zPasswd*/){
    return FSL_RC_MISUSE;
  }
  else if(!(db = fsl_needs_repo(f))){
    return FSL_RC_NOT_A_REPO;
  }
  *userId = 0;
  zSecret = fsl_sha1_shared_secret(f, zUsername, zPasswd );
  if(!zSecret) return FSL_RC_OOM;
  rc = fsl_db_get_id(db, userId,
                     "SELECT uid FROM user"
                     " WHERE login=%Q"
                     "   AND length(cap)>0 AND length(pw)>0"
                     "   AND login NOT IN ('anonymous','nobody','developer','reader')"
                     "   AND (pw=%Q OR (length(pw)<>40 AND pw=%Q))",
                     zUsername, zSecret, zPasswd);
  fsl_free(zSecret);
  return rc;
}

FSL_EXPORT int fsl_repo_login_clear( fsl_cx * f, fsl_id_t userId ){
  fsl_db * db;
  if(!f) return FSL_RC_MISUSE;
  else if(!(db = fsl_needs_repo(f))) return FSL_RC_NOT_A_REPO;
  else{
    int const rc = fsl_db_exec(db,
                       "UPDATE user SET cookie=NULL, ipaddr=NULL, "
                       " cexpire=0 WHERE "
                       " CASE WHEN %"FSL_ID_T_PFMT">=0 THEN uid=%"FSL_ID_T_PFMT""
                       " ELSE uid>0 END"
                       " AND login NOT IN('anonymous','nobody',"
                       " 'developer','reader')",
                       (fsl_id_t)userId, (fsl_id_t)userId);
    if(rc){
      fsl_cx_uplift_db_error(f, db);
    }
    return rc;
  }
}

#undef MARKER
