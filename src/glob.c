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
  This file contains some of the APIs dealing with globs.
*/
#include "fossil-scm/fossil-internal.h"
#include <assert.h>

bool fsl_str_glob(const char *zGlob, const char *z){
#if 1
  return (zGlob && z)
    ? (sqlite3_strglob(zGlob,z) ? 0 : 1)
    : 0;
#else
  int c, c2;
  int invert;
  int seen;
  while( (c = (*(zGlob++)))!=0 ){
    if( c=='*' ){
      while( (c=(*(zGlob++))) == '*' || c=='?' ){
        if( c=='?' && (*(z++))==0 ) return 0;
      }
      if( c==0 ){
        return 1;
      }else if( c=='[' ){
        while( *z && fsl_str_glob(zGlob-1,z)==0 ){
          z++;
        }
        return (*z)!=0;
      }
      while( (c2 = (*(z++)))!=0 ){
        while( c2!=c ){
          c2 = *(z++);
          if( c2==0 ) return 0;
        }
        if( fsl_str_glob(zGlob,z) ) return 1;
      }
      return 0;
    }else if( c=='?' ){
      if( (*(z++))==0 ) return 0;
    }else if( c=='[' ){
      int prior_c = 0;
      seen = 0;
      invert = 0;
      c = *(z++);
      if( c==0 ) return 0;
      c2 = *(zGlob++);
      if( c2=='^' ){
        invert = 1;
        c2 = *(zGlob++);
      }
      if( c2==']' ){
        if( c==']' ) seen = 1;
        c2 = *(zGlob++);
      }
      while( c2 && c2!=']' ){
        if( c2=='-' && zGlob[0]!=']' && zGlob[0]!=0 && prior_c>0 ){
          c2 = *(zGlob++);
          if( c>=prior_c && c<=c2 ) seen = 1;
          prior_c = 0;
        }else{
          if( c==c2 ){
            seen = 1;
          }
          prior_c = c2;
        }
        c2 = *(zGlob++);
      }
      if( c2==0 || (seen ^ invert)==0 ) return 0;
    }else{
      if( c!=(*(z++)) ) return 0;
    }
  }
  return *z==0;
#endif
}


int fsl_glob_list_parse( fsl_list * tgt, char const * zPatternList ){
  fsl_size_t i;             /* Loop counters */
  char const *z = zPatternList;
  char * cp;
  char delimiter;    /* '\'' or '\"' or 0 */
  int rc = 0;
  char const * end;
  if( !tgt || !zPatternList ) return FSL_RC_MISUSE;
  else if(!*zPatternList) return 0;
  end = zPatternList + fsl_strlen(zPatternList);
  while( (z<end) && z[0] ){
    while( fsl_isspace(z[0]) || z[0]==',' ){
      ++z;  /* Skip leading commas, spaces, and newlines */
    }
    if( z[0]==0 ) break;
    if( z[0]=='\'' || z[0]=='"' ){
      delimiter = z[0];
      ++z;
    }else{
      delimiter = ',';
    }
    /* Find the next delimter (or the end of the string). */
    for(i=0; z[i] && z[i]!=delimiter; i++){
      if( delimiter!=',' ) continue; /* If quoted, keep going. */
      if( fsl_isspace(z[i]) ) break; /* If space, stop. */
    }
    if( !i ) break;
    cp = fsl_strndup(z, (fsl_int_t)i);
    if(!cp) return FSL_RC_OOM;
    else{
      rc = fsl_list_append(tgt, cp);
      if(rc){
        fsl_free(cp);
        break;
      }
      cp[i]=0;
    }
    z += i+1;
  }
  return rc;
}


char const * fsl_glob_list_matches( fsl_list const * globList, char const * zHaystack ){
  if(!globList || !zHaystack || !*zHaystack || !globList->used) return NULL;
  else{
    char const * glob;
    fsl_size_t i = 0;
    for( ; i < globList->used; ++i){
      glob = (char const *)globList->list[i];
      if( fsl_str_glob( glob, zHaystack ) ) return glob;
    }
    return NULL;
  }
}

int fsl_glob_list_append( fsl_list * tgt, char const * zGlob ){
  if(!tgt || !zGlob || !*zGlob) return FSL_RC_MISUSE;
  else{
    char * cp = fsl_strdup(zGlob);
    int rc = cp ? 0 : FSL_RC_OOM;
    if(!rc){
      rc = fsl_list_append(tgt, cp);
      if(rc) fsl_free(cp);
    }
    return rc;
  }
}


void fsl_glob_list_clear( fsl_list * globList ){
  if(globList) fsl_list_visit_free(globList, 1);
}

