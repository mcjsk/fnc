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
/*****************************************************************************
  This file some of the caching-related APIs.
*/
#include "fossil-scm/fossil-internal.h"
#include <assert.h>

/* Only for debugging */
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)

bool fsl_acache_expire_oldest(fsl_acache * c){
  static uint16_t const sentinel = 0xFFFF;
  uint16_t i;
  fsl_int_t mnAge = c->nextAge;
  uint16_t mn = sentinel;
  for(i=0; i<c->used; i++){
    if( c->list[i].age<mnAge ){
      mnAge = c->list[i].age;
      mn = i;
    }
  }
  if( mn<sentinel ){
    fsl_id_bag_remove(&c->inCache, c->list[mn].rid);
    c->szTotal -= (uint32_t)c->list[mn].content.capacity;
    fsl_buffer_clear(&c->list[mn].content);
    --c->used;
    c->list[mn] = c->list[c->used];
  }
  return sentinel!=mn;
}

int fsl_acache_insert(fsl_acache * c, fsl_id_t rid, fsl_buffer *pBlob){
  fsl_acache_line *p;
  if( c->used>c->usedLimit || c->szTotal>c->szLimit ){
    fsl_size_t szBefore;
    do{
      szBefore = c->szTotal;
      fsl_acache_expire_oldest(c);
    }while( c->szTotal>c->szLimit && c->szTotal<szBefore );
  }
  if((!c->usedLimit || !c->szLimit)
     || (c->used+1 >= c->usedLimit)){
    fsl_buffer_clear(pBlob);
    return 0;
  }
  if( c->used>=c->capacity ){
    uint16_t const cap = c->capacity ? (c->capacity*2) : 10;
    void * remem = c->list
      ? fsl_realloc(c->list, cap*sizeof(c->list[0]))
      : fsl_malloc( cap*sizeof(c->list[0]) );
    assert((c->capacity && cap<c->capacity) ? !"Numeric overflow" : 1);
    if(c->capacity && cap<c->capacity){
        fsl_fatal(FSL_RC_RANGE,"Numeric overflow. Bump "
                  "fsl_acache::capacity to a larger int type.");
    }
    if(!remem){
      fsl_buffer_clear(pBlob) /* for consistency */;
      return FSL_RC_OOM;
    }
    c->capacity = cap;
    c->list = (fsl_acache_line*)remem;
  }
  p = &c->list[c->used++];
  p->rid = rid;
  p->age = c->nextAge++;
  c->szTotal += pBlob->capacity;
  p->content = *pBlob /* Transfer ownership */;
  *pBlob = fsl_buffer_empty;
  return fsl_id_bag_insert(&c->inCache, rid);
}


void fsl_acache_clear(fsl_acache * c){
#if 0
  while(fsl_acache_expire_oldest(c)){}
#else
  fsl_size_t i;
  for(i=0; i<c->used; i++){
    fsl_buffer_clear(&c->list[i].content);
  }
#endif
  fsl_free(c->list);
  fsl_id_bag_clear(&c->missing);
  fsl_id_bag_clear(&c->available);
  fsl_id_bag_clear(&c->inCache);
  *c = fsl_acache_empty;
}


int fsl_acache_check_available(fsl_cx * f, fsl_id_t rid){
  fsl_id_t srcid;
  int depth = 0;  /* Limit to recursion depth */
  static const int limit = 10000000 /* historical value */;
  int rc;
  fsl_acache * c = &f->cache.arty;
  assert(f);
  assert(c);
  assert(rid>0);
  assert(fsl_cx_db_repo(f));
  while( depth++ < limit ){  
    fsl_int_t cSize = -1;
    if( fsl_id_bag_contains(&c->missing, rid) ){
      return FSL_RC_NOT_FOUND;
    }
    else if( fsl_id_bag_contains(&c->available, rid) ){
      return 0;
    }
    else if( (cSize=fsl_content_size(f, rid)) <0){
      rc = fsl_id_bag_insert(&c->missing, rid);
      return rc ? rc : FSL_RC_NOT_FOUND;
    }
    srcid = 0;
    rc = fsl_delta_src_id(f, rid, &srcid);
    if(rc) return rc;
    else if( srcid==0 ){
      rc = fsl_id_bag_insert(&c->available, rid);
      return rc ? rc : 0;
    }
    rid = srcid;
  }
  assert(!"delta-loop in repository");
  return fsl_cx_err_set(f, FSL_RC_CONSISTENCY,
                        "Serious problem: delta-loop in repository");
}

#undef MARKER
