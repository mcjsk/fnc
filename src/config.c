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
/**************************************************************************
  This file implements (most of) the fsl_xxx APIs related to handling
  configuration data from the db(s).
*/
#include "fossil-scm/fossil-internal.h"
#include "fossil-scm/fossil-confdb.h"
#include <assert.h>
#include <stdlib.h> /* bsearch() */
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)

/**
   File-local macro used to ensure that all cached statements used by
   the fsl_config_get_xxx() APIs use an equivalent string (so that
   they use the same underlying cache fsl_stmt handle).  The first %s
   represents one of the config table names (config, vvfar,
   global_config). Normally we wouldn't use %s in a cached statement,
   but we're only expecting 3 values for table here and each one will
   only be cached once. The 2nd %s must be __FILE__.
*/
#define SELECT_FROM_CONFIG "SELECT value FROM %s WHERE name=?/*%s*/"

char const * fsl_config_table_for_role(fsl_confdb_e mode){
  switch(mode){
    case FSL_CONFDB_REPO: return "config";
    case FSL_CONFDB_CKOUT: return "vvar";
    case FSL_CONFDB_GLOBAL: return "global_config";
    case FSL_CONFDB_VERSIONABLE: return NULL;
    default:
      assert(!"Invalid fsl_confdb_e value");
      return NULL;
  }
}

fsl_db * fsl_config_for_role(fsl_cx * f, fsl_confdb_e mode){
  switch(mode){
    case FSL_CONFDB_REPO: return fsl_cx_db_repo(f);
    case FSL_CONFDB_CKOUT: return fsl_cx_db_ckout(f);
    case FSL_CONFDB_GLOBAL: return fsl_cx_db_config(f);
    case FSL_CONFDB_VERSIONABLE: return fsl_cx_db(f);
    default:
      assert(!"Invalid fsl_confdb_e value");
      return NULL;
  }
}

int fsl_config_versionable_filename(fsl_cx *f, char const * key,
                                    fsl_buffer *b){
  if(!f || !fsl_needs_ckout(f)) return FSL_RC_NOT_A_CKOUT;
  else if(!key || !*key || !fsl_is_simple_pathname(key, true)){
    return FSL_RC_MISUSE;
  }
  fsl_buffer_reuse(b);
  return fsl_buffer_appendf(b, "%s.fossil-settings/%s",
                            f->ckout.dir, key);
}


int fsl_config_unset( fsl_cx * f, fsl_confdb_e mode, char const * key ){
  fsl_db * db = fsl_config_for_role(f, mode);
  if(!db || !key || !*key) return FSL_RC_MISUSE;
  else if(mode==FSL_CONFDB_VERSIONABLE) return FSL_RC_UNSUPPORTED;
  else{
    char const * table = fsl_config_table_for_role(mode);
    assert(table);
    return fsl_db_exec(db, "DELETE FROM %s WHERE name=%Q", table, key);
  }
}

int32_t fsl_config_get_int32( fsl_cx * f, fsl_confdb_e mode,
                              int32_t dflt, char const * key ){
  int32_t rv = dflt;
  switch(mode){
    case FSL_CONFDB_VERSIONABLE:{
      char * val = fsl_config_get_text(f, mode, key, NULL);
      if(val){
        rv = (int32_t)atoi(val);
        fsl_free(val);
      }
      break;
    }
    default: {
      fsl_db * db = fsl_config_for_role(f, mode);
      char const * table = fsl_config_table_for_role(mode);
      assert(table);
      if(db){
        fsl_stmt * st = NULL;
        fsl_db_prepare_cached(db, &st, SELECT_FROM_CONFIG,
                              table, __FILE__);
        if(st){
          fsl_stmt_bind_text(st, 1, key, -1, 0);
          if(FSL_RC_STEP_ROW==fsl_stmt_step(st)){
            rv = fsl_stmt_g_int32(st, 0);
          }
          fsl_stmt_cached_yield(st);
        }
      }
      break;
    }
  }
  return rv;
}

int64_t fsl_config_get_int64( fsl_cx * f, fsl_confdb_e mode,
                              int64_t dflt, char const * key ){
  int64_t rv = dflt;
  switch(mode){
    case FSL_CONFDB_VERSIONABLE:{
      char * val = fsl_config_get_text(f, mode, key, NULL);
      if(val){
        rv = (int64_t)strtoll(val, NULL, 10);
        fsl_free(val);
      }
      break;
    }
    default: {
      fsl_db * db = fsl_config_for_role(f, mode);
      char const * table = fsl_config_table_for_role(mode);
      assert(table);
      if(db){
        fsl_stmt * st = NULL;
        fsl_db_prepare_cached(db, &st, SELECT_FROM_CONFIG,
                              table, __FILE__);
        if(st){
          fsl_stmt_bind_text(st, 1, key, -1, 0);
          if(FSL_RC_STEP_ROW==fsl_stmt_step(st)){
            rv = fsl_stmt_g_int64(st, 0);
          }
          fsl_stmt_cached_yield(st);
        }
      }
      break;
    }
  }
  return rv;
}

fsl_id_t fsl_config_get_id( fsl_cx * f, fsl_confdb_e mode,
                            fsl_id_t dflt, char const * key ){
  return (sizeof(fsl_id_t)==sizeof(int32_t))
    ? (fsl_id_t)fsl_config_get_int32(f, mode, dflt, key)
    : (fsl_id_t)fsl_config_get_int64(f, mode, dflt, key);
}

double fsl_config_get_double( fsl_cx * f, fsl_confdb_e mode,
                              double dflt, char const * key ){
  double rv = dflt;
  switch(mode){
    case FSL_CONFDB_VERSIONABLE:{
      char * val = fsl_config_get_text(f, mode, key, NULL);
      if(val){
        rv = strtod(val, NULL);
        fsl_free(val);
      }
      break;
    }
    default: {
      fsl_db * db = fsl_config_for_role(f, mode);
      if(!db) break/*e.g. global config is not opened*/;
      fsl_stmt * st = NULL;
      char const * table = fsl_config_table_for_role(mode);
      assert(table);
      fsl_db_prepare_cached(db, &st, SELECT_FROM_CONFIG,
                            table, __FILE__);
      if(st){
        fsl_stmt_bind_text(st, 1, key, -1, 0);
        if(FSL_RC_STEP_ROW==fsl_stmt_step(st)){
          rv = fsl_stmt_g_double(st, 0);
        }
        fsl_stmt_cached_yield(st);
      }
      break;
    }
  }
  return rv;
}

char * fsl_config_get_text( fsl_cx * f, fsl_confdb_e mode,
                            char const * key, fsl_size_t * len ){
  char * rv = NULL;
  fsl_buffer val = fsl_buffer_empty;
  if(fsl_config_get_buffer(f, mode, key, &val)){
    fsl_cx_err_reset(f);
    if(len) *len = 0;
    fsl_buffer_clear(&val)/*in case of partial read failure*/;
  }else{
    if(len) *len = val.used;
    rv = fsl_buffer_take(&val);
  }
  return rv;
}

int fsl_config_get_buffer( fsl_cx * f, fsl_confdb_e mode,
                           char const * key, fsl_buffer * b ){
  int rc = FSL_RC_NOT_FOUND;
  switch(mode){
    case FSL_CONFDB_VERSIONABLE:{
      if(!fsl_needs_ckout(f)){
        rc = FSL_RC_NOT_A_CKOUT;
        break;
      }
      fsl_buffer * fname = fsl_cx_scratchpad(f);
      rc = fsl_config_versionable_filename(f, key, fname);
      if(!rc){
        char const * zFile = fsl_buffer_cstr(fname);
        rc = fsl_stat(zFile, 0, false);
        if(rc){
          rc = fsl_cx_err_set(f, rc, "Could not stat file: %s",
                              zFile);
        }else{
          rc = fsl_buffer_fill_from_filename(b, zFile);
        }
      }
      fsl_cx_scratchpad_yield(f,fname);
      break;
    }
    default: {
      char const * table = fsl_config_table_for_role(mode);
      assert(table);
      fsl_db * const db = fsl_config_for_role(f, mode);
      if(!db) break;
      fsl_stmt * st = NULL;
      rc = fsl_db_prepare_cached(db, &st, SELECT_FROM_CONFIG,
                                 table, __FILE__);
      if(rc){
        rc = fsl_cx_uplift_db_error2(f, db, rc);
        break;
      }
      fsl_stmt_bind_text(st, 1, key, -1, 0);
      if(FSL_RC_STEP_ROW==fsl_stmt_step(st)){
        fsl_size_t len = 0;
        char const * s = fsl_stmt_g_text(st, 0, &len);
        rc = s ? fsl_buffer_append(b, s, len) : 0;
      }else{
        rc = FSL_RC_NOT_FOUND;
      }
      fsl_stmt_cached_yield(st);
      break;
    }
  }
  return rc;
}

bool fsl_config_get_bool( fsl_cx * f, fsl_confdb_e mode,
                          bool dflt, char const * key ){
  bool rv = dflt;
  switch(mode){
    case FSL_CONFDB_VERSIONABLE:{
      char * val = fsl_config_get_text(f, mode, key, NULL);
      if(val){
        rv = fsl_str_bool(val);
        fsl_free(val);
      }
      break;
    }
    default:{
      int rc;
      fsl_stmt * st = NULL;
      char const * table = fsl_config_table_for_role(mode);
      fsl_db * db;
      if(!f || !key || !*key) break;
      db = fsl_config_for_role(f, mode);
      if(!db) break;
      assert(table);
      rc = fsl_db_prepare_cached(db, &st, SELECT_FROM_CONFIG,
                                 table, __FILE__);
      if(!rc){
        fsl_stmt_bind_text(st, 1, key, -1, 0);
        if(FSL_RC_STEP_ROW==fsl_stmt_step(st)){
          char const * col = fsl_stmt_g_text(st, 0, NULL);
          rv = col ? fsl_str_bool(col) : dflt /* 0? */;
        }
        fsl_stmt_cached_yield(st);
      }
      break;
    }
  }
  return rv;
}

/**
    Sets up a REPLACE statement for the given config db and key. On
    success 0 is returned and *st holds the cached statement. The caller
    must bind() parameter #2 and step() the statement, then
    fsl_stmt_cached_yield() it.
   
    Returns non-0 on error.
*/
static int fsl_config_set_prepare( fsl_cx * f, fsl_stmt **st,
                                   fsl_confdb_e mode, char const * key ){
  char const * table = fsl_config_table_for_role(mode);
  fsl_db * db = fsl_config_for_role(f,mode);
  assert(table);
  if(!db || !key) return FSL_RC_MISUSE;
  else if(!*key) return FSL_RC_RANGE;
  else{
    const char * sql = FSL_CONFDB_REPO==mode
      ? "REPLACE INTO %s(name,value,mtime) VALUES(?,?,now())/*%s()*/"
      : "REPLACE INTO %s(name,value) VALUES(?,?)/*%s()*/";
    int rc = fsl_db_prepare_cached(db, st, sql,  table, __func__);
    if(!rc) rc = fsl_stmt_bind_text(*st, 1, key, -1, 1);
    if(rc && !f->error.code){
      fsl_cx_uplift_db_error(f, db);
    }
    return rc;
  }
}


/*
  TODO/FIXME: the fsl_config_set_xxx() routines all use the same basic
  structure, differing only in the concrete bind() op they call. They
  should be consolidated somehow.
*/

/**
   Writes valLen bytes of val to a versioned-setting file. Returns 0
   on success. Requires a checkout db.
*/
static int fsl_config_set_versionable( fsl_cx * f, char const * key,
                                       char const * val,
                                       fsl_size_t valLen){
  assert(key && *key);
  if(!fsl_needs_ckout(f)){
    return FSL_RC_NOT_A_CKOUT;
  }
  fsl_buffer * fName = fsl_cx_scratchpad(f);
  int rc = fsl_config_versionable_filename(f, key, fName);
  if(!rc){
    fsl_buffer fake = fsl_buffer_empty;
    fake.mem = (void*)val;
    fake.capacity = fake.used = valLen;
    rc = fsl_buffer_to_filename(&fake, fsl_buffer_cstr(fName));
  }
  fsl_cx_scratchpad_yield(f, fName);
  return rc;
}

  
int fsl_config_set_text( fsl_cx * f, fsl_confdb_e mode,
                         char const * key, char const * val ){
  if(!key) return FSL_RC_MISUSE;
  else if(!*key) return FSL_RC_RANGE;
  else if(FSL_CONFDB_VERSIONABLE==mode){
    return fsl_config_set_versionable(f, key, val,
                                      val ? fsl_strlen(val) : 0);
  }
  fsl_db * db = fsl_config_for_role(f,mode);
  if(!db) return FSL_RC_MISUSE;
  fsl_stmt * st = NULL;
  int rc = fsl_config_set_prepare(f, &st, mode, key);
  //MARKER(("config-set mode=%d k=%s v=%s\n",
  //        mode, key, val));
  if(!rc){
    if(val){
      rc = fsl_stmt_bind_text(st, 2, val, -1, 0);
    }else{
      rc = fsl_stmt_bind_null(st, 2);
    }
    if(!rc){
      rc = fsl_stmt_step(st);
    }
    fsl_stmt_cached_yield(st);
    if(FSL_RC_STEP_DONE==rc) rc = 0;
  }
  if(rc && !f->error.code) fsl_cx_uplift_db_error(f, db);
  return rc;
}

int fsl_config_set_blob( fsl_cx * f, fsl_confdb_e mode, char const * key,
                         void const * val, fsl_int_t len ){
  if(!key) return FSL_RC_MISUSE;
  else if(!*key) return FSL_RC_RANGE;
  else if(FSL_CONFDB_VERSIONABLE==mode){
    return fsl_config_set_versionable(f, key, val,
                                      (val && len<0)
                                      ? fsl_strlen((char const *)val)
                                      : (fsl_size_t)len);
  }
  fsl_db * db = fsl_config_for_role(f,mode);
  if(!db) return FSL_RC_MISUSE;
  fsl_stmt * st = NULL;
  int rc = fsl_config_set_prepare(f, &st, mode, key);
  if(!rc){
    if(val){
      if(len<0) len = fsl_strlen((char const *)val);
      rc = fsl_stmt_bind_blob(st, 2, val, len, 0);
    }else{
      rc = fsl_stmt_bind_null(st, 2);
    }
    if(!rc){
      rc = fsl_stmt_step(st);
    }
    fsl_stmt_cached_yield(st);
    if(FSL_RC_STEP_DONE==rc) rc = 0;
  }
  if(rc && !f->error.code) fsl_cx_uplift_db_error(f, db);
  return rc;
}

int fsl_config_set_int32( fsl_cx * f, fsl_confdb_e mode,
                          char const * key, int32_t val ){
  if(!key) return FSL_RC_MISUSE;
  else if(!*key) return FSL_RC_RANGE;
  else if(FSL_CONFDB_VERSIONABLE==mode){
    char buf[64] = {0};
    fsl_snprintf(buf, sizeof(buf), "%" PRIi32 "\n", val);
    return fsl_config_set_versionable(f, key, buf,
                                      fsl_strlen(buf));
  }
  fsl_db * db = fsl_config_for_role(f,mode);
  if(!db) return FSL_RC_MISUSE;
  fsl_stmt * st = NULL;
  int rc = fsl_config_set_prepare(f, &st, mode, key);
  if(!rc){
    rc = fsl_stmt_bind_int32(st, 2, val);
    if(!rc){
      rc = fsl_stmt_step(st);
    }
    fsl_stmt_cached_yield(st);
    if(FSL_RC_STEP_DONE==rc) rc = 0;
  }
  if(rc && !f->error.code) fsl_cx_uplift_db_error(f, db);
  return rc;
}

int fsl_config_set_int64( fsl_cx * f, fsl_confdb_e mode,
                          char const * key, int64_t val ){
  if(!key) return FSL_RC_MISUSE;
  else if(!*key) return FSL_RC_RANGE;
  else if(FSL_CONFDB_VERSIONABLE==mode){
    char buf[64] = {0};
    fsl_snprintf(buf, sizeof(buf), "%" PRIi64 "\n", val);
    return fsl_config_set_versionable(f, key, buf,
                                      fsl_strlen(buf));
  }
  fsl_db * db = fsl_config_for_role(f,mode);
  if(!db) return FSL_RC_MISUSE;
  fsl_stmt * st = NULL;
  int rc = fsl_config_set_prepare(f, &st, mode, key);
  if(!rc){
    rc = fsl_stmt_bind_int64(st, 2, val);
    if(!rc){
      rc = fsl_stmt_step(st);
    }
    fsl_stmt_cached_yield(st);
    if(FSL_RC_STEP_DONE==rc) rc = 0;
  }
  if(rc && !f->error.code) fsl_cx_uplift_db_error(f, db);
  return rc;
}

int fsl_config_set_id( fsl_cx * f, fsl_confdb_e mode,
                          char const * key, fsl_id_t val ){
  if(!key) return FSL_RC_MISUSE;
  else if(!*key) return FSL_RC_RANGE;
  else if(FSL_CONFDB_VERSIONABLE==mode){
    char buf[64] = {0};
    fsl_snprintf(buf, sizeof(buf), "%" FSL_ID_T_PFMT "\n", val);
    return fsl_config_set_versionable(f, key, buf,
                                      fsl_strlen(buf));
  }
  fsl_db * db = fsl_config_for_role(f,mode);
  if(!db) return FSL_RC_MISUSE;
  fsl_stmt * st = NULL;
  int rc = fsl_config_set_prepare(f, &st, mode, key);
  if(!rc){
    rc = fsl_stmt_bind_id(st, 2, val);
    if(!rc){
      rc = fsl_stmt_step(st);
    }
    fsl_stmt_cached_yield(st);
    if(FSL_RC_STEP_DONE==rc) rc = 0;
  }
  if(rc && !f->error.code) fsl_cx_uplift_db_error(f, db);
  return rc;
}

int fsl_config_set_double( fsl_cx * f, fsl_confdb_e mode,
                           char const * key, double val ){
  if(!key) return FSL_RC_MISUSE;
  else if(!*key) return FSL_RC_RANGE;
  else if(FSL_CONFDB_VERSIONABLE==mode){
    char buf[128] = {0};
    fsl_snprintf(buf, sizeof(buf), "%f\n", val);
    return fsl_config_set_versionable(f, key, buf,
                                      fsl_strlen(buf));
  }
  fsl_db * db = fsl_config_for_role(f,mode);
  if(!db) return FSL_RC_MISUSE;
  fsl_stmt * st = NULL;
  int rc = fsl_config_set_prepare(f, &st, mode, key);
  if(!rc){
    rc = fsl_stmt_bind_double(st, 2, val);
    if(!rc){
        rc = fsl_stmt_step(st);
    }
    fsl_stmt_cached_yield(st);
    if(FSL_RC_STEP_DONE==rc) rc = 0;
  }
  if(rc && !f->error.code) fsl_cx_uplift_db_error(f, db);
  return rc;
}

int fsl_config_set_bool( fsl_cx * f, fsl_confdb_e mode,
                         char const * key, bool val ){
  if(!key) return FSL_RC_MISUSE;
  else if(!*key) return FSL_RC_RANGE;
  char buf[4] = {'o','n','\n','\n'};
  if(!val){
    buf[1] = buf[2] = 'f';
  }
  if(FSL_CONFDB_VERSIONABLE==mode){
    return fsl_config_set_versionable(f, key, buf,
                                      val ? 3 : 4);
  }
  fsl_db * db = fsl_config_for_role(f,mode);
  if(!db) return FSL_RC_MISUSE;
  fsl_stmt * st = NULL;
  int rc = fsl_config_set_prepare(f, &st, mode, key);
  if(!rc){
    rc = fsl_stmt_bind_text(st, 2, buf, val ? 2 : 3, false);
    if(!rc){
      rc = fsl_stmt_step(st);
    }
    fsl_stmt_cached_yield(st);
    if(FSL_RC_STEP_DONE==rc) rc = 0;
  }
  if(rc && !f->error.code) fsl_cx_uplift_db_error(f, db);
  return rc;
}

int fsl_config_transaction_begin(fsl_cx * f, fsl_confdb_e mode){
  fsl_db * db = fsl_config_for_role(f,mode);
  if(!db) return FSL_RC_MISUSE;
  else{
    int const rc = fsl_db_transaction_begin(db);
    if(rc) fsl_cx_uplift_db_error(f, db);
    return rc;
  }
}

int fsl_config_transaction_end(fsl_cx * f, fsl_confdb_e mode, char rollback){
  fsl_db * db = fsl_config_for_role(f,mode);
  if(!db) return FSL_RC_MISUSE;
  else{
    int const rc = fsl_db_transaction_end(db, rollback);
    if(rc) fsl_cx_uplift_db_error(f, db);
    return rc;
  }
}

int fsl_config_globs_load(fsl_cx * f, fsl_list * li, char const * key){
  int rc = 0;
  char * val = NULL;
  if(!f || !li || !key || !*key) return FSL_RC_MISUSE;
  else if(fsl_cx_db_ckout(f)){
    /* Try versionable settings... */
    fsl_buffer buf = fsl_buffer_empty;
    rc = fsl_config_get_buffer(f, FSL_CONFDB_VERSIONABLE, key, &buf);
    if(rc){
      switch(rc){
        case FSL_RC_NOT_FOUND:
          fsl_cx_err_reset(f);
          rc = 0;
          break;
        default:
          fsl_buffer_clear(&buf);
          return rc;
      }
      /* Fall through and try the next option. */
    }else{
      if(buf.mem){
        val = fsl_buffer_take(&buf);
      }else{
        /* Empty but existing list, so it trumps the
           repo/global settings. */;
        fsl_buffer_clear(&buf);
      }
      goto gotone;
    }
  }
  if(fsl_cx_db_repo(f)){
    /* See if the repo can serve us... */
    val = fsl_config_get_text(f, FSL_CONFDB_REPO, key, NULL);
    if(val) goto gotone;
    /* Else fall through and try global config... */
  }
  if(fsl_cx_db_config(f)){
    /*FIXME?: we arguably should open the global config for this if it
      is not already opened.*/
    val = fsl_config_get_text(f, FSL_CONFDB_GLOBAL, key, NULL);
    if(val) goto gotone;
  }
  gotone:
  if(val){
      rc = fsl_glob_list_parse( li, val );
      fsl_free(val);
      val = 0;
      return rc;
  }
  return rc;
}

/*
  TODO???: the infrastructure from fossil's configure.c and db.c which
  deals with the config db and the list of known/allowed settings.

  ==================

static const struct {
  const char *zName;   / * Name of the configuration set * /
  int groupMask;       / * Mask for that configuration set * /
  const char *zHelp;   / * What it does * /

} fslConfigGroups[] = {
  { "/email",        FSL_CONFIGSET_ADDR,  "Concealed email addresses in tickets" },
  { "/project",      FSL_CONFIGSET_PROJ,  "Project name and description"         },
  { "/skin",         FSL_CONFIGSET_SKIN | FSL_CONFIGSET_CSS,
                                      "Web interface appearance settings"    },
  { "/css",          FSL_CONFIGSET_CSS,   "Style sheet"                          },
  { "/shun",         FSL_CONFIGSET_SHUN,  "List of shunned artifacts"            },
  { "/ticket",       FSL_CONFIGSET_TKT,   "Ticket setup",                        },
  { "/user",         FSL_CONFIGSET_USER,  "Users and privilege settings"         },
  { "/xfer",         FSL_CONFIGSET_XFER,  "Transfer setup",                      },
  { "/all",          FSL_CONFIGSET_ALL,   "All of the above"                     },
  {NULL, 0, NULL}
};
 */

/*
   The following is a list of settings that we are willing to
   transfer.
  
   Setting names that begin with an alphabetic characters refer to
   single entries in the CONFIG table.  Setting names that begin with
   "@" are for special processing.
*/
static struct {
  const char *zName;   /* Name of the configuration parameter */
  int groupMask;       /* Which config groups is it part of */
} fslConfigXfer[] = {
  { "css",                    FSL_CONFIGSET_CSS  },

  { "header",                 FSL_CONFIGSET_SKIN },
  { "footer",                 FSL_CONFIGSET_SKIN },
  { "logo-mimetype",          FSL_CONFIGSET_SKIN },
  { "logo-image",             FSL_CONFIGSET_SKIN },
  { "background-mimetype",    FSL_CONFIGSET_SKIN },
  { "background-image",       FSL_CONFIGSET_SKIN },
  { "index-page",             FSL_CONFIGSET_SKIN },
  { "timeline-block-markup",  FSL_CONFIGSET_SKIN },
  { "timeline-max-comment",   FSL_CONFIGSET_SKIN },
  { "timeline-plaintext",     FSL_CONFIGSET_SKIN },
  { "adunit",                 FSL_CONFIGSET_SKIN },
  { "adunit-omit-if-admin",   FSL_CONFIGSET_SKIN },
  { "adunit-omit-if-user",    FSL_CONFIGSET_SKIN },

  { "th1-setup",              FSL_CONFIGSET_ALL },

  { "tcl",                    FSL_CONFIGSET_SKIN|FSL_CONFIGSET_TKT|FSL_CONFIGSET_XFER },
  { "tcl-setup",              FSL_CONFIGSET_SKIN|FSL_CONFIGSET_TKT|FSL_CONFIGSET_XFER },

  { "project-name",           FSL_CONFIGSET_PROJ },
  { "project-description",    FSL_CONFIGSET_PROJ },
  { "manifest",               FSL_CONFIGSET_PROJ },
  { "binary-glob",            FSL_CONFIGSET_PROJ },
  { "clean-glob",             FSL_CONFIGSET_PROJ },
  { "ignore-glob",            FSL_CONFIGSET_PROJ },
  { "keep-glob",              FSL_CONFIGSET_PROJ },
  { "crnl-glob",              FSL_CONFIGSET_PROJ },
  { "encoding-glob",          FSL_CONFIGSET_PROJ },
  { "empty-dirs",             FSL_CONFIGSET_PROJ },
  { "allow-symlinks",         FSL_CONFIGSET_PROJ },

  { "ticket-table",           FSL_CONFIGSET_TKT  },
  { "ticket-common",          FSL_CONFIGSET_TKT  },
  { "ticket-change",          FSL_CONFIGSET_TKT  },
  { "ticket-newpage",         FSL_CONFIGSET_TKT  },
  { "ticket-viewpage",        FSL_CONFIGSET_TKT  },
  { "ticket-editpage",        FSL_CONFIGSET_TKT  },
  { "ticket-reportlist",      FSL_CONFIGSET_TKT  },
  { "ticket-report-template", FSL_CONFIGSET_TKT  },
  { "ticket-key-template",    FSL_CONFIGSET_TKT  },
  { "ticket-title-expr",      FSL_CONFIGSET_TKT  },
  { "ticket-closed-expr",     FSL_CONFIGSET_TKT  },
  { "@reportfmt",             FSL_CONFIGSET_TKT  },

  { "@user",                  FSL_CONFIGSET_USER },

  { "@concealed",             FSL_CONFIGSET_ADDR },

  { "@shun",                  FSL_CONFIGSET_SHUN },

  { "xfer-common-script",     FSL_CONFIGSET_XFER },
  { "xfer-push-script",       FSL_CONFIGSET_XFER },
};

#define ARRAYLEN(X) (sizeof(X)/sizeof(X[0]))
/*
   Return a pointer to a string that contains the RHS of an IN
   operator that will select CONFIG table names that are part of the
   configuration that matches iMatch. The returned string must
   eventually be fsl_free()'d.
*/
char *fsl_config_inop_rhs(int iMask){
  fsl_buffer x = fsl_buffer_empty;
  const char *zSep = "";
  const int n = (int)ARRAYLEN(fslConfigXfer);
  int i;
  int rc = fsl_buffer_append(&x, "(", 1);
  for(i=0; !rc && (i<n); i++){
    if( (fslConfigXfer[i].groupMask & iMask)==0 ) continue;
    if( fslConfigXfer[i].zName[0]=='@' ) continue;
    rc = fsl_buffer_appendf(&x, "%s%Q", zSep, fslConfigXfer[i].zName);
    zSep = ",";
  }
  if(!rc){
    rc = fsl_buffer_append(&x, ")", 1);
  }
  if(rc){
    fsl_buffer_clear(&x);
    assert(!x.mem);
  }else{
    fsl_buffer_resize(&x, x.used);
  }
  return (char *)x.mem;
}


/**
   Holds metadata for fossil-defined configuration settings.

   As of 2021, this library does NOT intend to maintain 100%
   config-entry parity with fossil, as the vast majority of its config
   settings are application-level preferences. The library will
   maintain compatibility with versionable settings and a handful of
   settings which make sense for a library-level API
   (e.g. 'ignore-glob' and 'manifest'). The majority of fossil's
   settings, however, are specific to that application and will not be
   treated specially by the library.

   @see fsl_config_ctrl_get()
   @see fsl_config_has_versionable()
   @see fsl_config_key_is_versionable()
   @see fsl_config_key_default_value()
*/  
struct fsl_config_ctrl {
  /** Name of the setting */
  char const *name;
  /**
     Historical (fossil(1)) internal variable name used by
     db_set(). Not currently used by this impl.
  */
  char const *var;
  /**
     Historical (HTML UI). Width of display.  0 for boolean values.
  */
  int width;
  /**
     Is this setting versionable?
  */
  bool versionable;
  /**
     Default value
  */
  char const *defaultValue;
};
typedef struct fsl_config_ctrl fsl_config_ctrl;

/**
   If key is the name of a fossil-defined config key, this returns
   the fsl_config_ctrl value describing that configuration property,
   else it returns NULL.
*/
FSL_EXPORT fsl_config_ctrl const * fsl_config_ctrl_get(char const * key);

/**
   Returns true if key is the name of a config property
   as defined by fossil(1).
*/
FSL_EXPORT bool fsl_config_key_is_fossil(char const * key);


/**
   Returns true if key is the name of a versionable property, as
   defined by fossil(1). Returning false is NOT a guaranty that fossil
   does NOT define that setting (this library does not track _all_
   settings), but returning true is a a guarantee that it is a
   fossil-defined versionable setting.
*/
FSL_EXPORT bool fsl_config_key_is_versionable(char const * key);

/**
   If key refers to a fossil-defined configuration setting, this
   returns its default value as a NUL-terminated string. Its bytes are
   static and immutable. Returns NULL if key is not a known
   configuration key.
*/
FSL_EXPORT char const * fsl_config_key_default_value(char const * key);

/**
   Returns true if f's current checkout contains the given
   versionable configuration setting, else false.

   @see fsl_config_ctrl
*/
FSL_EXPORT bool fsl_config_has_versionable( fsl_cx * f, char const * key );

static fsl_config_ctrl const fslConfigCtrl[] = {
/*
  These MUST stay sorted by name and the .defaultValue field MUST have
  a non-NULL value so that some API guarantees can be made.

  FIXME: bring this up to date wrt post-2014 fossil. Or abandon it
  altogether: it has since been decided that this library will not
  attempt to enforce application-level config constraints for the vast
  majority of fossil's config settings. The main benefit for us in
  keeping this is to be able to quickly look up versionable settings,
  as we really need to keep compatibility with fossil for those.
*/
  { "access-log",    0,                0, 0, "off"                 },
  { "allow-symlinks",0,                0, 0/*as of late 2020*/,
                                             "off"                 },
  { "auto-captcha",  "autocaptcha",    0, 0, "on"                  },
  { "auto-hyperlink",0,                0, 0, "on"                  },
  { "auto-shun",     0,                0, 0, "on"                  },
  { "autosync",      0,                0, 0, "on"                  },
  { "binary-glob",   0,               40, 1, ""                    },
  { "clearsign",     0,                0, 0, "off"                 },
#if defined(_WIN32) || defined(__CYGWIN__) || defined(__DARWIN__) || defined(__APPLE__)
  { "case-sensitive",0,                0, 0, "off"                 },
#else
  { "case-sensitive",0,                0, 0, "on"                  },
#endif
  { "clean-glob",    0,               40, 1, ""                    },
  { "crnl-glob",     0,               40, 1, ""                    },
  { "default-perms", 0,               16, 0, "u"                   },
  { "diff-binary",   0,                0, 0, "on"                  },
  { "diff-command",  0,               40, 0, ""                    },
  { "dont-push",     0,                0, 0, "off"                 },
  { "dotfiles",      0,                0, 1, "off"                 },
  { "editor",        0,               32, 0, ""                    },
  { "empty-dirs",    0,               40, 1, ""                    },
  { "encoding-glob",  0,              40, 1, ""                    },
  { "gdiff-command", 0,               40, 0, "gdiff"               },
  { "gmerge-command",0,               40, 0, ""                    },
  { "http-port",     0,               16, 0, "8080"                },
  { "https-login",   0,                0, 0, "off"                 },
  { "ignore-glob",   0,               40, 1, ""                    },
  { "keep-glob",     0,               40, 1, ""                    },
  { "localauth",     0,                0, 0, "off"                 },
  { "main-branch",   0,               40, 0, "trunk"               },
  { "manifest",      0,                0, 1, "off"                 },
  { "max-upload",    0,               25, 0, "250000"              },
  { "mtime-changes", 0,                0, 0, "on"                  },
  { "pgp-command",   0,               40, 0, "gpg --clearsign -o " },
  { "proxy",         0,               32, 0, "off"                 },
  { "relative-paths",0,                0, 0, "on"                  },
  { "repo-cksum",    0,                0, 0, "on"                  },
  { "self-register", 0,                0, 0, "off"                 },
  { "ssh-command",   0,               40, 0, ""                    },
  { "ssl-ca-location",0,              40, 0, ""                    },
  { "ssl-identity",  0,               40, 0, ""                    },
  { "tcl",           0,                0, 0, "off"                 },
  { "tcl-setup",     0,               40, 0, ""                    },
  { "th1-setup",     0,               40, 0, ""                    },
  { "web-browser",   0,               32, 0, ""                    },
  { "white-foreground", 0,             0, 0, "off"                 },
  { 0,0,0,0,0 }
};

char *fsl_db_setting_inop_rhs(){
  fsl_buffer x = fsl_buffer_empty;
  const char *zSep = "";
  fsl_config_ctrl const * ct = &fslConfigCtrl[0];
  int rc = fsl_buffer_append(&x, "(", 1);
  for( ; !rc && ct && ct->name; ++ct){
    rc = fsl_buffer_appendf(&x, "%s%Q", zSep, ct->name);
    zSep = ",";
  }
  if(!rc){
    rc = fsl_buffer_append(&x, ")", 1);
  }
  if(rc){
    fsl_buffer_clear(&x);
    assert(!x.mem);
  }else{
    fsl_buffer_resize(&x, x.used);
  }
  return (char *)x.mem;
}

static int fsl_config_ctrl_cmp(const void *lhs, const void *rhs){
  fsl_config_ctrl const * l = (fsl_config_ctrl const *)lhs;
  fsl_config_ctrl const * r = (fsl_config_ctrl const *)rhs;
  if(!l) return r ? -1 : 0;
  else if(!r) return 1;
  else return fsl_strcmp(l->name, r->name);
}

fsl_config_ctrl const * fsl_config_ctrl_get(char const * key){
  fsl_config_ctrl const * fcc;
  fsl_config_ctrl bogo = {0,0,0,0,0};
  bogo.name = key;
  fcc = (fsl_config_ctrl const *)
    bsearch( &bogo, fslConfigCtrl,
             ARRAYLEN(fslConfigCtrl) -1 /* for empty tail entry */,
             sizeof(fsl_config_ctrl),
             fsl_config_ctrl_cmp );
  return (fcc && fcc->name) ? fcc : NULL;
}

bool fsl_config_key_is_fossil(char const * key){
  fsl_config_ctrl const * fcc = fsl_config_ctrl_get(key);
  return (fcc && fcc->name) ? 1 : 0;
}

bool fsl_config_key_is_versionable(char const * key){
  fsl_config_ctrl const * fcc = fsl_config_ctrl_get(key);
  return (fcc && fcc->versionable) ? 1 : 0;
}

char const * fsl_config_key_default_value(char const * key){
  fsl_config_ctrl const * fcc = fsl_config_ctrl_get(key);
  return (fcc && fcc->name) ? fcc->defaultValue : NULL;
}

bool fsl_config_has_versionable( fsl_cx * f, char const * key ){
  if(!f || !key || !*key || !f->ckout.dir) return 0;
  else if(!fsl_config_key_is_fossil(key)) return 0;
  else{
    fsl_buffer * fn = fsl_cx_scratchpad(f);
    int rc = fsl_config_versionable_filename(f, key, fn);
    if(!rc) rc = fsl_stat(fsl_buffer_cstr(fn), NULL, 0);
    fsl_cx_scratchpad_yield(f, fn);
    return 0==rc;
  }
}


#undef SELECT_FROM_CONFIG
#undef MARKER
#undef ARRAYLEN
