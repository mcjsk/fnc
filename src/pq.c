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
  This file houses the priority queue class.
*/
#include <assert.h>

#include "fossil-scm/fossil.h"
#include "fossil-scm/fossil-internal.h"

void fsl_pq_clear(fsl_pq *p){
  fsl_free(p->list);
  *p = fsl_pq_empty;
}

/*
   Change the size of the queue so that it contains N slots
*/
static int fsl_pq_resize(fsl_pq *p, fsl_size_t N){
  void * re = fsl_realloc(p->list, sizeof(fsl_pq_entry)*N);
  if(!re) return FSL_RC_OOM;
  else{
    p->list = (fsl_pq_entry*)re;
    p->capacity = N;
    return 0;
  }
}

/**
   Insert element e into the queue.
*/
int fsl_pq_insert(fsl_pq *p, fsl_id_t e,
                  double v, void *pData){
  fsl_size_t i, j;
  if( p->used+1>p->capacity ){
    int const rc = fsl_pq_resize(p, p->used+5);
    if(rc) return rc;
  }
  for(i=0; i<p->used; ++i){
    if( p->list[i].priority>v ){
      for(j=p->used; j>i; --j){
        p->list[j] = p->list[j-1];
      }
      break;
    }
  }
  p->list[i].id = e;
  p->list[i].data = pData;
  p->list[i].priority = v;
  ++p->used;
  return 0;
}

fsl_id_t fsl_pq_extract(fsl_pq *p, void **pp){
  fsl_id_t e, i;
  if( p->used==0 ){
    if( pp ) *pp = 0;
    return 0;
  }
  e = p->list[0].id;
  if( pp ) *pp = p->list[0].data;
  for(i=0; i<((fsl_id_t)p->used-1); ++i){
    p->list[i] = p->list[i+1];
  }
  --p->used;
  return e;
}
