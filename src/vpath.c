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
/**********************************************************************
  This file contains routines related to working with "paths" through
  Fossil SCM version history.
*/
#include "fossil-scm/fossil-internal.h"
#include "fossil-scm/fossil-vpath.h"
#include <assert.h>

/* Only for debugging */
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)


static const fsl_vpath_node fsl_vpath_node_empty = {0,0,0,0,0,{0},0};
const fsl_vpath fsl_vpath_empty = fsl_vpath_empty_m;

void fsl_vpath_clear(fsl_vpath *path){
  fsl_vpath_node * p;
  while(path->pAll){
    p = path->pAll;
    path->pAll = p->pAll;
    fsl_free(p);
  }
  fsl_id_bag_clear(&path->seen);
  *path = fsl_vpath_empty;
}

fsl_vpath_node * fsl_vpath_first(fsl_vpath *p){
  return p->pStart;
}

fsl_vpath_node * fsl_vpath_last(fsl_vpath *p){
  return p->pEnd;
}

int fsl_vpath_length(fsl_vpath const * p){
  return p->nStep;
}

fsl_vpath_node * fsl_vpath_next(fsl_vpath_node *p){
  return p->u.pTo;
}

fsl_vpath_node * fsl_vpath_midpoint(fsl_vpath * path){
  if( path->nStep<2 ) return 0;
  else{
    fsl_vpath_node *p;
    int i;
    int const max = path->nStep/2;
    for(p=path->pEnd, i=0; p && i<max; p=p->pFrom, i++){}
    return p;
  }
}


void fsl_vpath_reverse(fsl_vpath * path){
  fsl_vpath_node *p;
  assert( path->pEnd!=0 );
  for(p=path->pEnd; p && p->pFrom; p = p->pFrom){
    p->pFrom->u.pTo = p;
  }
  path->pEnd->u.pTo = 0;
  assert( p==path->pStart );
}

/**
   Adds a new node to path and returns it. Returns 0 on allocation error.
   path must not be 0. rid must be greater than 0. pFrom may be 0. If
   pFrom is not 0 then isParent must be true if pFrom is a parent of
   rid.

   On success, sets the returned node as path->pCurrent, sets its
   pFrom to the given pFrom, and sets rc->u.pPeer to the prior
   path->pCurrent value.
*/
static fsl_vpath_node * fsl_vpath_new_node(fsl_vpath * path, fsl_id_t rid,
                                         fsl_vpath_node * pFrom, bool isParent){
  fsl_vpath_node * rc = 0;
  assert(path);
  assert(rid>0);
  if(0 != fsl_id_bag_insert(&path->seen, rid)) return 0;
  rc = (fsl_vpath_node*)fsl_malloc(sizeof(fsl_vpath_node));
  if(!rc){
    fsl_id_bag_remove(&path->seen, rid);
    return 0;
  }
  *rc = fsl_vpath_node_empty;
  rc->rid = rid;
  rc->fromIsParent = pFrom ? isParent : 0;
  rc->pFrom = pFrom;
  rc->u.pPeer = path->pCurrent;
  path->pCurrent = rc;
  rc->pAll = path->pAll;
  path->pAll = rc;
  return rc;
}

int fsl_vpath_shortest( fsl_cx * f, fsl_vpath * path,
                        fsl_id_t iFrom, fsl_id_t iTo,
                        bool directOnly, bool oneWayOnly ){
  fsl_stmt s = fsl_stmt_empty;
  fsl_db * db = fsl_needs_repo(f);
  int rc = 0;
  fsl_vpath_node * pPrev;
  fsl_vpath_node * p;
  assert(db);
  if(!db) return FSL_RC_NOT_A_REPO;
  else if(iFrom<=0){
    return fsl_cx_err_set(f, FSL_RC_RANGE,
                          "Invalid 'from' RID: %d", (int)iFrom);
  }else if(iTo<=0){
    /*
      Possible TODO: if iTo==0, use... what? Checkout? Tip of current
      checkout branch? Trunk? The multitude of options make it impossible
      to decide :/.      
    */
    return fsl_cx_err_set(f, FSL_RC_RANGE,
                          "Invalid 'to' RID: %d", (int)iTo);
  }

  fsl_vpath_clear(path);
  path->pStart = fsl_vpath_new_node(path, iFrom, 0, 0);
  if(!path->pStart){
    return fsl_cx_err_set(f, FSL_RC_OOM, 0);
  }
  if( iTo == iFrom ){
    path->pEnd = path->pStart;
    return 0;
  }

  if( oneWayOnly ){
    if(directOnly){
      rc = fsl_db_prepare(db, &s,
                          "SELECT cid, 1 FROM plink WHERE pid=?1 AND isprim");
    }else{
      rc = fsl_db_prepare(db, &s,
                          "SELECT cid, 1 FROM plink WHERE pid=?1");
    }
  }else if( directOnly ){
    rc = fsl_db_prepare(db, &s,
                        "SELECT cid, 1 FROM plink WHERE pid=?1 AND isprim "
                        "UNION ALL "
                        "SELECT pid, 0 FROM plink WHERE cid=?1 AND isprim");
  }else{
    rc = fsl_db_prepare(db, &s,
                        "SELECT cid, 1 FROM plink WHERE pid=?1 "
                        "UNION ALL "
                        "SELECT pid, 0 FROM plink WHERE cid=?1");
  }
  if(rc){
    fsl_cx_uplift_db_error(f, db);
    assert(f->error.code);
    goto end;
  }

  while(path->pCurrent){
    ++path->nStep;
    pPrev = path->pCurrent;
    path->pCurrent = 0;
    while( pPrev ){
      rc = fsl_stmt_bind_id(&s, 1, pPrev->rid);
      assert(0==rc);
      while( FSL_RC_STEP_ROW == fsl_stmt_step(&s) ){
        fsl_id_t const cid = fsl_stmt_g_id(&s, 0);
        int const isParent = fsl_stmt_g_int32(&s, 1);
        assert((cid>0) && "fsl_id_bag_find() asserts this.");
        if( fsl_id_bag_contains(&path->seen, cid) ) continue;
        p = fsl_vpath_new_node(path, cid, pPrev, isParent ? 1 : 0);
        if(!p){
          rc = fsl_cx_err_set(f, FSL_RC_OOM, 0);
          goto end;
        }
        if( cid == iTo ){
          fsl_stmt_finalize(&s);
          path->pEnd = p;
          fsl_vpath_reverse( path );
          return 0;
        }
      }
      fsl_stmt_reset(&s);
      pPrev = pPrev->u.pPeer;
    }
  }
  end:
  fsl_stmt_finalize(&s);
  fsl_vpath_clear(path);
  return rc;
}

/*
** A record of a file rename operation.
*/
typedef struct NameChange NameChange;
struct NameChange {
  fsl_id_t origName;        /* Original name of file */
  fsl_id_t curName;         /* Current name of the file */
  fsl_id_t newName;         /* Name of file in next version */
  NameChange *pNext;   /* List of all name changes */
};
const NameChange NameChange_empty =  {0,0,0,0};

int fsl_cx_find_filename_changes(
  fsl_cx * f,
  fsl_id_t iFrom,          /* Ancestor check-in */
  fsl_id_t iTo,            /* Recent check-in */
  bool revOK,              /* OK to move backwards (child->parent) if true */
  uint32_t *pnChng,        /* Number of name changes along the path */
  fsl_id_t **aiChng        /* Name changes */
){
  fsl_vpath_node *p = 0;   /* For looping over path from iFrom to iTo */
  NameChange *pAll = 0;    /* List of all name changes seen so far */
  NameChange *pChng = 0;   /* For looping through the name change list */
  uint32_t nChng = 0;       /* Number of files whose names have changed */
  fsl_id_t *aChng = 0;     /* Two integers per name change */
  fsl_stmt q1 = fsl_stmt_empty; /* Query of name changes */
  fsl_db * const db = fsl_needs_repo(f);
  fsl_vpath path = fsl_vpath_empty;
  int rc = 0;

  if(!db) return FSL_RC_NOT_A_REPO;
  *pnChng = 0;
  *aiChng = 0;
  if(iFrom<=0){
    return fsl_cx_err_set(f, FSL_RC_MISUSE,
                          "Invalid 'from' RID: %" FSL_ID_T_PFMT, iFrom);
  }else if(0==iTo){
    return fsl_cx_err_set(f, FSL_RC_MISUSE,
                          "Invalid 'to' RID: %" FSL_ID_T_PFMT, iTo);
  }
  if( iFrom==iTo ) return 0;
  rc = fsl_vpath_shortest(f, &path, iFrom, iTo, true, !revOK);
  if(rc) goto end;
  else if(!path.pStart){
    goto end;
  }
  fsl_vpath_reverse(&path);
  rc = fsl_db_prepare(db, &q1,
     "SELECT pfnid, fnid FROM mlink"
     " WHERE mid=?1 AND (pfnid>0 OR fid==0)"
     " ORDER BY pfnid"
  );
  if(rc) goto dberr;
  for(p=path.pStart; p; p=p->u.pTo){
    fsl_id_t fnid = 0, pfnid = 0;
    if( !p->fromIsParent && (p->u.pTo==0 || p->u.pTo->fromIsParent) ){
      /* Skip nodes where the parent is not on the path */
      continue;
    }
    fsl_stmt_bind_id(&q1, 1, p->rid);
    while( FSL_RC_STEP_ROW==fsl_stmt_step(&q1) ){
      pfnid = fsl_stmt_g_id(&q1, 0);
      fnid = fsl_stmt_g_id(&q1, 1);
      if( pfnid==0 ){
        pfnid = fnid;
        fnid = 0;
      }
      if( !p->fromIsParent ){
        fsl_id_t const t = fnid;
        fnid = pfnid;
        pfnid = t;
      }
#if 0
      if( zDebug ){
        fossil_print("%s at %d%s %.10z: %d[%z] -> %d[%z]\n",
           zDebug, p->rid, p->fromIsParent ? ">" : "<",
           db_text(0, "SELECT uuid FROM blob WHERE rid=%d", p->rid),
           pfnid,
           db_text(0, "SELECT name FROM filename WHERE fnid=%d", pfnid),
           fnid,
           db_text(0, "SELECT name FROM filename WHERE fnid=%d", fnid));
      }
#endif
      for(pChng=pAll; pChng; pChng=pChng->pNext){
        if( pChng->curName==pfnid ){
          pChng->newName = fnid;
          break;
        }
      }
      if( pChng==0 && fnid>0 ){
        pChng = (NameChange*)fsl_malloc( sizeof(NameChange) );
        if(!pChng){
          rc = FSL_RC_OOM;
          goto end;
        }
        pChng->pNext = pAll;
        pAll = pChng;
        pChng->origName = pfnid;
        pChng->curName = pfnid;
        pChng->newName = fnid;
        ++nChng;
      }
    }
    for(pChng=pAll; pChng; pChng=pChng->pNext){
      pChng->curName = pChng->newName;
    }
    fsl_stmt_reset(&q1);
  }
  if( nChng ){
    /* Count effective changes. */
    uint32_t n;
    for(pChng=pAll, n=0; pChng; pChng=pChng->pNext){
      if( pChng->newName==0 ) continue;
      if( pChng->origName==0 ) continue;
      ++n;
    }
    nChng = n;
  }
  if(nChng){
    uint32_t i;
    aChng = (fsl_id_t*)fsl_malloc( nChng*2*sizeof(fsl_id_t) );
    if(!aChng){
      rc = FSL_RC_OOM;
      goto end;
    }
    for(pChng=pAll, i=0; pChng; pChng=pChng->pNext){
      if( pChng->newName==0 ) continue;
      if( pChng->origName==0 ) continue;
      aChng[i] = pChng->origName;
      aChng[i+1] = pChng->newName;
#if 0
      if( zDebug ){
        fossil_print("%s summary %d[%z] -> %d[%z]\n",
           zDebug,
           aChng[i],
           db_text(0, "SELECT name FROM filename WHERE fnid=%d", aChng[i]),
           aChng[i+1],
           db_text(0, "SELECT name FROM filename WHERE fnid=%d", aChng[i+1]));
      }
#endif
      i += 2;
    }
    assert(nChng==i/2);
    *pnChng = i/2;
    *aiChng = aChng;
    while( pAll ){
      pChng = pAll;
      pAll = pAll->pNext;
      fsl_free(pChng);
    }
  }else{
    *pnChng = 0;
    *aiChng = 0;
  }
  end:
  fsl_stmt_finalize(&q1);
  if(rc){
    assert(!aChng);
  }
  fsl_vpath_clear(&path);
  return rc;
  dberr:
  assert(rc);
  rc = fsl_cx_uplift_db_error2(f, db, rc);
  goto end;
}


#undef MARKER
