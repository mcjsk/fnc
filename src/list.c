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
  This file houses the implementations for the fsl_list routines.
*/

#include "fossil-scm/fossil-internal.h"
#include <assert.h>
#include <stdlib.h> /* malloc() and friends, qsort() */
#include <memory.h> /* memset() */
int fsl_list_reserve( fsl_list * self, fsl_size_t n )
{
  if( !self ) return FSL_RC_MISUSE;
  else if(0 == n){
    if(0 == self->capacity) return 0;
    fsl_free(self->list);
    *self = fsl_list_empty;
    return 0;
  }
  else if( self->capacity >= n ){
    return 0;
  }
  else{
    size_t const sz = sizeof(void*) * n;
    void* * m = (void**)fsl_realloc( self->list, sz );
    if( !m ) return FSL_RC_OOM;
    memset( m + self->capacity, 0, (sizeof(void*)*(n-self->capacity)));
    self->capacity = n;
    self->list = m;
    return 0;
  }
}

void fsl_list_swap( fsl_list * lhs, fsl_list * rhs ){
  fsl_list tmp = *lhs;
  *rhs = *lhs;
  *lhs = tmp;
}

int fsl_list_append( fsl_list * self, void* cp ){
  if( !self ) return FSL_RC_MISUSE;
  assert(self->used <= self->capacity);
  if(self->used == self->capacity){
    int rc;
    fsl_size_t const cap = self->capacity
      ? (self->capacity * 2)
      : 10;
    rc = fsl_list_reserve(self, cap);
    if(rc) return rc;
  }
  self->list[self->used++] = cp;
  if(self->used<self->capacity) self->list[self->used]=NULL;
  return 0;
}

int fsl_list_v_fsl_free(void * obj, void * visitorState ){
  if(obj) fsl_free( obj );
  return 0;
}

int fsl_list_clear( fsl_list * self, fsl_list_visitor_f childFinalizer,
                    void * finalizerState ){
  /*
    TODO: manually traverse the list and set each list entry for which
    the finalizer succeeds to NULL, so that we can provide
    well-defined behaviour if childFinalizer() fails and we abort the
    loop.
   */
  int rc = fsl_list_visit(self, 0, childFinalizer, finalizerState );
  if(!rc) fsl_list_reserve(self, 0);
  return rc;
}

void fsl_list_visit_free( fsl_list * self, bool freeListMem ){
  fsl_list_visit(self, 0, fsl_list_v_fsl_free, NULL );
  if(freeListMem) fsl_list_reserve(self, 0);
  else self->used = 0;
}


int fsl_list_visit( fsl_list const * self, int order,
                    fsl_list_visitor_f visitor,
                    void * visitorState ){
  int rc = FSL_RC_OK;
  if( self && self->used && visitor ){
    fsl_int_t i = 0;
    fsl_int_t pos = (order<0) ? self->used-1 : 0;
    fsl_int_t step = (order<0) ? -1 : 1;
    for( rc = 0; (i < (fsl_int_t)self->used)
           && (0 == rc); ++i, pos+=step ){
      void* obj = self->list[pos];
      if(obj) rc = visitor( obj, visitorState );
      if( obj != self->list[pos] ){
        --i;
        if(order>=0) pos -= step;
      }
    }
  }
  return rc;
}


int fsl_list_visit_p( fsl_list * self, int order,
                      bool shiftIfNulled,
                      fsl_list_visitor_f visitor, void * visitorState )
{
  int rc = FSL_RC_OK;
  if( self && self->used && visitor ){
    fsl_int_t i = 0;
    fsl_int_t pos = (order<0) ? self->used-1 : 0;
    fsl_int_t step = (order<0) ? -1 : 1;
    for( rc = 0; (i < (fsl_int_t)self->used)
           && (0 == rc); ++i, pos+=step ){
      void* obj = self->list[pos];
      if(obj) {
        assert((order<0) && "TEST THAT THIS WORKS WITH IN-ORDER!");
        rc = visitor( &self->list[pos], visitorState );
        if( shiftIfNulled && !self->list[pos]){
          fsl_int_t x = pos;
          fsl_int_t const to = self->used-pos;

          assert( to < (fsl_int_t) self->capacity );
          for( ; x < to; ++x ) self->list[x] = self->list[x+1];
          if( x < (fsl_int_t)self->capacity ) self->list[x] = 0;
          --i;
          --self->used;
          if(order>=0) pos -= step;
        }
      }
    }
  }
  return rc;
}

void fsl_list_sort( fsl_list * li,
                    fsl_generic_cmp_f cmp){
  if(li && li->used>1){
    qsort( li->list, li->used, sizeof(void*), cmp );
  }
}



fsl_int_t fsl_list_index_of( fsl_list const * li,
                             void const * key,
                             fsl_generic_cmp_f cmpf ){
  fsl_size_t i;
  void const * p;
  for(i = 0; i < li->used; ++i){
    p = li->list[i];
    if(!p){
      if(!key) return (fsl_int_t)i;
      else continue;
    }else if((p==key) ||
            (0==cmpf( key, p )) ){
      return (fsl_int_t)i;
    }
  }
  return -1;
}


fsl_int_t fsl_list_index_of_cstr( fsl_list const * li,
                                  char const * key ){
  return fsl_list_index_of(li, key, fsl_strcmp_cmp);
}
