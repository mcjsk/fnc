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
  This file houses the code for the fsl_id_bag class.
*/
#include <assert.h>
#include <string.h> /* memset() */
#include "fossil-scm/fossil.h"
#include "fossil-scm/fossil-internal.h"

const fsl_id_bag fsl_id_bag_empty = fsl_id_bag_empty_m;

/* Only for debugging */
#include <stdio.h>

void fsl_id_bag_clear(fsl_id_bag *p){
  if(p->list) fsl_free(p->list);
  *p = fsl_id_bag_empty;
}

/*
   The hash function
*/
#define fsl_id_bag_hash(i)  (i*101)

/*
   Change the size of the hash table on a bag so that
   it contains N slots
  
   Completely reconstruct the hash table from scratch.  Deleted
   entries (indicated by a -1) are removed. When finished,
   p->entryCount==p->used and p->capacity==newSize.

   Returns on on success, FSL_RC_OOM on allocation error.
*/
static int fsl_id_bag_resize(fsl_id_bag *p, fsl_size_t newSize){
  fsl_size_t i;
  fsl_id_bag old;
  fsl_size_t nDel = 0;   /* Number of deleted entries */
  fsl_size_t nLive = 0;  /* Number of live entries */
  fsl_id_t * newList;
  assert( newSize > p->entryCount );
  newList = (fsl_id_t*)fsl_malloc( sizeof(p->list[0])*newSize );
  if(!newList) return FSL_RC_OOM;
  old = *p;
  p->list = newList;
  p->capacity = newSize;
  memset(p->list, 0, sizeof(p->list[0])*newSize );
  for(i=0; i<old.capacity; i++){
    fsl_id_t e = old.list[i];
    if( e>0 ){
      unsigned h = fsl_id_bag_hash(e)%newSize;
      while( p->list[h] ){
        h++;
        if( h==newSize ) h = 0;
      }
      p->list[h] = e;
      nLive++;
    }else if( e<0 ){
      nDel++;
    }
  }
  assert( p->entryCount == nLive );
  assert( p->used == nLive+nDel );
  p->used = p->entryCount;
  fsl_id_bag_clear(&old);
  return 0;
}

void fsl_id_bag_reset(fsl_id_bag *p){
  p->entryCount = p->used = 0;
}


int fsl_id_bag_insert(fsl_id_bag *p, fsl_id_t e){
  fsl_size_t h;
  int rc = 0;
  assert( e>0 );
  if( p->used+1 >= p->capacity/2 ){
    fsl_size_t n = p->capacity ? p->capacity*2 : 30;
    rc = fsl_id_bag_resize(p,  n );
    if(rc) return rc;
  }
  h = fsl_id_bag_hash(e)%p->capacity;
  while( p->list[h]>0 && p->list[h]!=e ){
    h++;
    if( h>=p->capacity ) h = 0;
  }
  if( p->list[h]<=0 ){
    if( p->list[h]==0 ) ++p->used;
    p->list[h] = e;
    ++p->entryCount;
    rc = 0;
  }
  return rc;
}

bool fsl_id_bag_contains(fsl_id_bag const *p, fsl_id_t e){
  fsl_size_t h;
  assert( e>0 );
  if( p->capacity==0 || 0==p->used ){
    return false;
  }
  assert(p->list);
  h = fsl_id_bag_hash(e)%p->capacity;
  while( p->list[h] && p->list[h]!=e ){
    h++;
    if( h>=p->capacity ) h = 0
      /*loop around to the start*/
      ;
  }
  return p->list[h]==e;
}

bool fsl_id_bag_remove(fsl_id_bag *p, fsl_id_t e){
  fsl_size_t h;
  bool rv = false;
  assert( e>0 );
  if( !p->capacity || !p->used ) return rv;
  assert(p->list);
  h = fsl_id_bag_hash(e)%p->capacity;
  while( p->list[h] && p->list[h]!=e ){
    ++h;
    if( h>=p->capacity ) h = 0;
  }
  rv = p->list[h]==e;
  if( p->list[h] ){
    fsl_size_t nx = h+1;
    if( nx>=p->capacity ) nx = 0;
    if( p->list[nx]==0 ){
      p->list[h] = 0;
      --p->used;
    }else{
      p->list[h] = -1;
    }
    --p->entryCount;
    if( p->entryCount==0 ){
      memset(p->list, 0, p->capacity*sizeof(p->list[0]));
      p->used = 0;
    }else if( p->capacity>40 && p->entryCount<p->capacity/8 ){
      fsl_id_bag_resize(p, p->capacity/2)
        /* ignore realloc error and keep the old size. */;
    }
  }
  return rv;
}

fsl_id_t fsl_id_bag_first(fsl_id_bag const *p){
  if( p->capacity==0 || 0==p->used ){
    return 0;
  }else{
    fsl_size_t i;
    for(i=0; i<p->capacity && p->list[i]<=0; ++i){}
    if( i<p->capacity ){
      return p->list[i];
    }else{
      return 0;
    }
  }
}

fsl_id_t fsl_id_bag_next(fsl_id_bag const *p, fsl_id_t e){
  fsl_size_t h;
  assert( p->capacity>0 );
  assert( e>0 );
  assert(p->list);
  h = fsl_id_bag_hash(e)%p->capacity;
  while( p->list[h] && p->list[h]!=e ){
    ++h;
    if( h>=p->capacity ) h = 0;
  }
  assert( p->list[h] );
  h++;
  while( h<p->capacity && p->list[h]<=0 ){
    h++;
  }
  return h<p->capacity ? p->list[h] : 0;
}

fsl_size_t fsl_id_bag_count(fsl_id_bag const *p){
  return p->entryCount;
}

void fsl_id_bag_swap(fsl_id_bag *lhs, fsl_id_bag *rhs){
  fsl_id_bag x = *lhs;
  *lhs = *rhs;
  *rhs = x;
}

#undef fsl_id_bag_hash
