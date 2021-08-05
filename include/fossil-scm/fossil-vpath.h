/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_VPATH_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_VPATH_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

  *****************************************************************************
  This file declares public APIs relating to calculating paths via
  Fossil SCM version history.
*/

#include "fossil-internal.h" /* MUST come first b/c of config macros */

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct fsl_vpath_node fsl_vpath_node;
typedef struct fsl_vpath fsl_vpath;

/**
   Holds information for a single node in a path of checkin versions.

   @see fsl_vpath
*/
struct fsl_vpath_node {
  /** ID for this node */
  fsl_id_t rid;
  /** True if pFrom is the parent of rid */
  bool fromIsParent;
  /** True if primary side of common ancestor */
  bool isPrimary;
  /* HISTORICAL: Abbreviate output in "fossil bisect ls" */
  bool isHidden;
  /** Node this one came from. */
  fsl_vpath_node *pFrom;
  union {
    /** List of nodes of the same generation */
    fsl_vpath_node *pPeer;
    /** Next on path from beginning to end */
    fsl_vpath_node *pTo;
  } u;
  /** List of all nodes */
  fsl_vpath_node *pAll;
};

/**
   A utility type for collecting "paths" between two checkin versions.
*/
struct fsl_vpath{
  /** Current generation of nodes */
  fsl_vpath_node *pCurrent;
  /** All nodes */
  fsl_vpath_node *pAll;
  /** Nodes seen before */
  fsl_id_bag seen;
  /** Number of steps from first to last. */
  int nStep;
  /** Earliest node in the path. */
  fsl_vpath_node *pStart;
  /** Common ancestor of pStart and pEnd */
  fsl_vpath_node *pPivot;
  /** Most recent node in the path. */
  fsl_vpath_node *pEnd;
};

/**
   An empty-initialize fsl_vpath object, intended for const-copy
   initialization.
*/
#define fsl_vpath_empty_m {0,0,fsl_id_bag_empty_m,0,0,0,0}

/**
   An empty-initialize fsl_vpath object, intended for copy
   initialization.
*/
FSL_EXPORT const fsl_vpath fsl_vpath_empty;


/**
   Returns the first node in p's path.

   The returned node is owned by path may be invalidated by any APIs
   which manipulate path.
*/
FSL_EXPORT fsl_vpath_node * fsl_vpath_first(fsl_vpath *p);

/**
   Returns the last node in p's path.

   The returned node is owned by path may be invalidated by any APIs
   which manipulate path.
*/
FSL_EXPORT fsl_vpath_node * fsl_vpath_last(fsl_vpath *p);

/**
   Returns the next node p's path.

   The returned node is owned by path may be invalidated by any APIs
   which manipulate path.

   Intended to be used like this:

   @code
   for( p = fsl_vpath_first(path) ;
   p ;
   p = fsl_vpath_next(p)){
   ...
   }
   @endcode
*/
FSL_EXPORT fsl_vpath_node * fsl_vpath_next(fsl_vpath_node *p);

/**
   Returns p's path length.
*/
FSL_EXPORT int fsl_vpath_length(fsl_vpath const * p);

/**
   Frees all nodes in path (which must not be NULL) and resets all
   state in path. Does not free path.
*/
FSL_EXPORT void fsl_vpath_clear(fsl_vpath *path);

/**
   Find the mid-point of the path.  If the path contains fewer than
   2 steps, returns 0. The returned node is owned by path may be
   invalidated by any APIs which manipulate path.
*/
FSL_EXPORT fsl_vpath_node * fsl_vpath_midpoint(fsl_vpath * path);


/**
   Computes the shortest path from checkin versions iFrom to iTo
   (inclusive), storing the result state in path. If path has
   state before this is called, it is cleared by this call.

   iFrom and iTo must both be valid checkin version RIDs.

   If directOnly is true, then use only the "primary" links from
   parent to child.  In other words, ignore merges.

   On success, returns 0 and path->pStart will point to the
   beginning of the path (the iFrom node). If pStart is 0 then no
   path could be found but 0 is still returned.

   Elements of the path can be traversed like so:

   @code
   fsl_vpath path = fsl_vpath_empty;
   fsl_vpath_node * n = 0;
   int rc = fsl_vpath_shortest(f, &path, versionFrom, versionTo, 1, 0);
   if(rc) { ... error ... }
   for( n = fsl_vpath_first(&path); n; n = fsl_vpath_next(n) ){
   ...
   }
   fsl_vpath_clear(&path);
   @endcode

   On error, f's error state may be updated with a description of the
   problem.
*/
FSL_EXPORT int fsl_vpath_shortest( fsl_cx * f, fsl_vpath * path,
                                   fsl_id_t iFrom, fsl_id_t iTo,
                                   bool directOnly, bool oneWayOnly );

/**
   Reconstructs path from path->pStart to path->pEnd, reversing its
   order by fiddling with the u->pTo fields.

   Unfortunately does not reverse after the initial creation/reversal
   :/.
*/
FSL_EXPORT void fsl_vpath_reverse(fsl_vpath * path);


#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_VPATH_H_INCLUDED */
