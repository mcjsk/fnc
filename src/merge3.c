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
#include "fossil-scm/fossil.h"
#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h> /* memmove()/strlen() */

#if 0
#define FDEBUG(X)  X
#define ISFDEBUG 1
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)
#else
#define FDEBUG(X)
#define ISFDEBUG 0
#define MARKER(pfexp) (void)0
#endif

// Only for testing/debugging..

/* The minimum of two integers */
#define mymin(A,B)  ((A)<(B)?(A):(B))

/**
   Compare N lines of text from pV1 and pV2. If the lines are the
   same, return true. Return false if one or more of the N lines are
   different.

   The cursors on both pV1 and pV2 is unchanged by this comparison.
*/
static
bool sameLines(fsl_buffer const *pV1, fsl_buffer const *pV2,
               int N){
  unsigned char const *z1;
  unsigned char const *z2;
  fsl_size_t i;
  unsigned char c;
  if( !N ) return true;
  z1 = pV1->mem + pV1->cursor;
  z2 = pV2->mem + pV2->cursor;
  for(i=0; (c=z1[i])==z2[i]; ++i){
    if( c=='\n' || !c ){
      --N;
      if( !N || !c ) return true;
    }
  }
  return false;
}

/**
   Look at the next edit triple in both aC1 and aC2.  (An "edit triple" is
   three integers describing the number of copies, deletes, and inserts in
   moving from the original to the edited copy of the file.) If the three
   integers of the edit triples describe an identical edit, then return 1.
   If the edits are different, return 0.

   aC1 = the array of edit integers for pV1.

   aC2 = the array of edit integers for pV2.
*/
static
bool sameEdit(int const *aC1, int const *aC2,
              fsl_buffer const *pV1, fsl_buffer const *pV2){
#if 0
  if( aC1[0]!=aC2[0] ) return 0;
  if( aC1[1]!=aC2[1] ) return 0;
  if( aC1[2]!=aC2[2] ) return 0;
  if( sameLines(pV1, pV2, aC1[2]) ) return 1;
  return 0;
#else
  if( aC1[0]!=aC2[0]
      || aC1[1]!=aC2[1]
      || aC1[2]!=aC2[2] ) return false;
  return sameLines(pV1, pV2, aC1[2]);
#endif
}


/**
   The aC[] array contains triples of integers. Within each triple,
   the elements are:

   (0)  The number of lines to copy
   (1)  The number of lines to delete
   (2)  The number of liens to insert

   Suppose we want to advance over sz lines of the original file.
   This routine returns true if that advance would land us on a copy
   operation. It returns false if the advance would end on a delete.
*/
static
bool ends_at_CPY(int *aC, int sz){
  while( sz>0 && (aC[0]>0 || aC[1]>0 || aC[2]>0) ){
    if( aC[0]>=sz ) return true;
    sz -= aC[0];
    if( aC[1]>sz ) return false;
    sz -= aC[1];
    aC += 3;
  }
  return true;
}

/**
   pSrc contains an edited file where aC[] describes the edit.  Part
   of pSrc has already been output.  This routine outputs additional
   lines of pSrc - lines that correspond to the next sz lines of the
   original unedited file.

   Note that sz counts the number of lines of text in the original
   file, but text is output from the edited file, so the number of
   lines transfer to pOut might be different from sz. Fewer lines
   appear in pOut if there are deletes. More lines appear if there
   are inserts.

   The aC[] array is updated and the new index into aC[] is returned
   via the final argument.

   Returns 0 on success, FSL_RC_OOM on allocation error.
*/
static
int output_one_side(fsl_buffer *pOut,
                    fsl_buffer *pSrc,
                    int *aC,
                    int i,
                    int sz,
                    int *newIndex){
  int rc = 0;
  while( sz>0 ){
    if( aC[i]==0 && aC[i+1]==0 && aC[i+2]==0 ) break;
    if( aC[i]>=sz ){
      rc = fsl_buffer_copy_lines(pOut, pSrc, sz);
      if(rc) break;
      aC[i] -= sz;
      break;
    }
    rc = fsl_buffer_copy_lines(pOut, pSrc, aC[i]);
    if(!rc) rc = fsl_buffer_copy_lines(pOut, pSrc, aC[i+2]);
    if(rc) break;
    sz -= aC[i] + aC[i+1];
    i += 3;
  }
  if(!rc) *newIndex = i;
  return rc;
}

/**
   Return true if the input blob contains any CR/LF pairs on the first
   ten lines. This should be enough to detect files that use mainly
   CR/LF line endings without causing a performance impact for LF only
   files.
*/
static
bool contains_crlf(fsl_buffer const *p){
  fsl_size_t i;
  fsl_size_t j = 0;
  const uint16_t maxL = 10; //Max lines to check
  unsigned const char *z = p->mem;
  fsl_size_t const n = p->used+1;
  for(i=1; i<n; ){
    if( z[i-1]=='\r' && z[i]=='\n' ) return true;
    while( i<n && z[i]!='\n' ){ ++i; }
    ++j;
    if( j>maxL ) break;
  }
  return false;
}

/**
   Ensure that the text in p, if not empty, ends with a new line. If
   useCrLf is true adds "\r\n" otherwise "\n".  Returns 0 on success
   or p is empty, and FSL_RC_OOM on OOM.
*/
static
int ensure_line_end(fsl_buffer *p, bool useCrLf){
  int rc = 0;
  if( !p->used ) return 0;
  if( p->mem[p->used-1]!='\n' ){
    rc = fsl_buffer_append(p, useCrLf ? "\r\n" : "\n", useCrLf ? 2 : 1);
  }
  return rc;
}

/**
   Returns an array of bytes representing the byte-order-mark for
   UTF-8. If pnByte is not NULL, the number of bytes in the BOM (3)
   is written there.
*/
//static
const unsigned char *get_utf8_bom(unsigned int *pnByte){
  static const unsigned char bom[] = {
    0xef, 0xbb, 0xbf, 0x00, 0x00, 0x00
  };
  if( pnByte ) *pnByte = 3;
  return bom;
}

/**
   Returns true if given blob starts with a UTF-8
   byte-order-mark (BOM).
*/
static
bool starts_with_utf8_bom(fsl_buffer const *p, unsigned int *n){
  unsigned const char *z = p->mem;
  unsigned int bomSize = 0;
  const unsigned char *bom = get_utf8_bom(&bomSize);
  if( n ) *n = bomSize;
  return (p->used<bomSize)
    ? false
    : memcmp(z, bom, bomSize)==0;
}

/**
   Text of boundary markers for merge conflicts.

   NEVER, EVER change these. They MUST match the ones used
   by fossil.
*/
static
const char *const mergeMarker[] = {
 /*123456789 123456789 123456789 123456789 123456789 123456789 123456789*/
  "<<<<<<< BEGIN MERGE CONFLICT: local copy shown first <<<<<<<<<<<<<<<",
  "||||||| COMMON ANCESTOR content follows ||||||||||||||||||||||||||||",
  "======= MERGED IN content follows ==================================",
  ">>>>>>> END MERGE CONFLICT >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
};

static fsl_int_t assert_mema_lengths(){
  static const fsl_int_t mmLen = 68;
  static bool once = true;
  if(once){
    once = false;
    assert(sizeof(mergeMarker)/sizeof(mergeMarker[0]) == 4);
    assert((fsl_int_t)fsl_strlen(mergeMarker[0])==mmLen);
    assert((fsl_int_t)fsl_strlen(mergeMarker[1])==mmLen);
    assert((fsl_int_t)fsl_strlen(mergeMarker[2])==mmLen);
    assert((fsl_int_t)fsl_strlen(mergeMarker[3])==mmLen);
  }
  return mmLen;
}

int fsl_buffer_merge3(fsl_buffer *pPivot, fsl_buffer *pV1, fsl_buffer *pV2, fsl_buffer *pOut,
                      unsigned int *conflictCount){
  int *aC1 = 0;          /* Changes from pPivot to pV1 */
  int *aC2 = 0;          /* Changes from pPivot to pV2 */
  int i1, i2;            /* Index into aC1[] and aC2[] */
  int nCpy, nDel, nIns;  /* Number of lines to copy, delete, or insert */
  int limit1, limit2;    /* Sizes of aC1[] and aC2[] */
  int rc = 0;
  unsigned int nConflict = 0;     /* Number of merge conflicts seen so far */
  bool useCrLf = false;
  const fsl_int_t mmLen = assert_mema_lengths();

#define RC if(rc) { \
    MARKER(("rc=%s\n", fsl_rc_cstr(rc))); goto end; } (void)0
  fsl_buffer_reuse(pOut);         /* Merge results stored in pOut */
  
  /* If both pV1 and pV2 start with a UTF-8 byte-order-mark (BOM),
  ** keep it in the output. This should be secure enough not to cause
  ** unintended changes to the merged file and consistent with what
  ** users are using in their source files.
  */
  if( starts_with_utf8_bom(pV1, 0) && starts_with_utf8_bom(pV2, 0) ){
    rc = fsl_buffer_append(pOut, get_utf8_bom(0), 3);
    RC;
  }

  /* Check once to see if both pV1 and pV2 contains CR/LF endings.
  ** If true, CR/LF pair will be used later to append the
  ** boundary markers for merge conflicts.
  */
  if( contains_crlf(pV1) && contains_crlf(pV2) ){
    useCrLf = true;
  }

  /* Compute the edits that occur from pPivot => pV1 (into aC1)
  ** and pPivot => pV2 (into aC2).  Each of the aC1 and aC2 arrays is
  ** an array of integer triples.  Within each triple, the first integer
  ** is the number of lines of text to copy directly from the pivot,
  ** the second integer is the number of lines of text to omit from the
  ** pivot, and the third integer is the number of lines of text that are
  ** inserted.  The edit array ends with a triple of 0,0,0.
  */
  rc = fsl_diff_text_raw(pPivot, pV1, 0, &aC1);
  if(!rc) rc = fsl_diff_text_raw(pPivot, pV2, 0, &aC2);
  RC;
  assert(aC1 && aC2);

  /* Rewind inputs:  Needed to reconstruct output */
  fsl_buffer_rewind(pV1);
  fsl_buffer_rewind(pV2);
  fsl_buffer_rewind(pPivot);

  /* Determine the length of the aC1[] and aC2[] change vectors */
  for(i1=0; aC1[i1] || aC1[i1+1] || aC1[i1+2]; i1+=3){}
  limit1 = i1;
  for(i2=0; aC2[i2] || aC2[i2+1] || aC2[i2+2]; i2+=3){}
  limit2 = i2;

  FDEBUG(
    for(i1=0; i1<limit1; i1+=3){
      printf("c1: %4d %4d %4d\n", aC1[i1], aC1[i1+1], aC1[i1+2]);
    }
    for(i2=0; i2<limit2; i2+=3){
      printf("c2: %4d %4d %4d\n", aC2[i2], aC2[i2+1], aC2[i2+2]);
    }
  )

  /* Loop over the two edit vectors and use them to compute merged text
  ** which is written into pOut.  i1 and i2 are multiples of 3 which are
  ** indices into aC1[] and aC2[] to the edit triple currently being
  ** processed
  */
  i1 = i2 = 0;
  while( i1<limit1 && i2<limit2 ){
    FDEBUG( printf("%d: %2d %2d %2d   %d: %2d %2d %2d\n",
           i1/3, aC1[i1], aC1[i1+1], aC1[i1+2],
           i2/3, aC2[i2], aC2[i2+1], aC2[i2+2]); )

    if( aC1[i1]>0 && aC2[i2]>0 ){
      /* Output text that is unchanged in both V1 and V2 */
      nCpy = mymin(aC1[i1], aC2[i2]);
      FDEBUG( printf("COPY %d\n", nCpy); )
      rc = fsl_buffer_copy_lines(pOut, pPivot, (fsl_size_t)nCpy);
      if(!rc) rc = fsl_buffer_copy_lines(0, pV1, (fsl_size_t)nCpy);
      if(!rc) rc = fsl_buffer_copy_lines(0, pV2, (fsl_size_t)nCpy);
      RC;
      aC1[i1] -= nCpy;
      aC2[i2] -= nCpy;
    }else
    if( aC1[i1] >= aC2[i2+1] && aC1[i1]>0 && aC2[i2+1]+aC2[i2+2]>0 ){
      /* Output edits to V2 that occurs within unchanged regions of V1 */
      nDel = aC2[i2+1];
      nIns = aC2[i2+2];
      FDEBUG( printf("EDIT -%d+%d left\n", nDel, nIns); )
      rc = fsl_buffer_copy_lines(0, pPivot, (fsl_size_t)nDel);
      if(!rc) rc = fsl_buffer_copy_lines(0, pV1, (fsl_size_t)nDel);
      if(!rc) rc = fsl_buffer_copy_lines(pOut, pV2, (fsl_size_t)nIns);
      RC;
      aC1[i1] -= nDel;
      i2 += 3;
    }else
    if( aC2[i2] >= aC1[i1+1] && aC2[i2]>0 && aC1[i1+1]+aC1[i1+2]>0 ){
      /* Output edits to V1 that occur within unchanged regions of V2 */
      nDel = aC1[i1+1];
      nIns = aC1[i1+2];
      FDEBUG( printf("EDIT -%d+%d right\n", nDel, nIns); )
      rc = fsl_buffer_copy_lines(0, pPivot, (fsl_size_t)nDel);
      if(!rc) fsl_buffer_copy_lines(0, pV2, (fsl_size_t)nDel);
      if(!rc) fsl_buffer_copy_lines(pOut, pV1, (fsl_size_t)nIns);
      aC2[i2] -= nDel;
      i1 += 3;
    }else
    if( sameEdit(&aC1[i1], &aC2[i2], pV1, pV2) ){
      /* Output edits that are identical in both V1 and V2. */
      assert( aC1[i1]==0 );
      nDel = aC1[i1+1];
      nIns = aC1[i1+2];
      FDEBUG( printf("EDIT -%d+%d both\n", nDel, nIns); )
      rc = fsl_buffer_copy_lines(0, pPivot, (fsl_size_t)nDel);
      if(!rc) fsl_buffer_copy_lines(pOut, pV1, (fsl_size_t)nIns);
      if(!rc) fsl_buffer_copy_lines(0, pV2, (fsl_size_t)nIns);
      i1 += 3;
      i2 += 3;
    }else
    {
      /* We have found a region where different edits to V1 and V2 overlap.
      ** This is a merge conflict.  Find the size of the conflict, then
      ** output both possible edits separated by distinctive marks.
      */
      int sz = 1;    /* Size of the conflict in lines */
      ++nConflict;
      while( !ends_at_CPY(&aC1[i1], sz) || !ends_at_CPY(&aC2[i2], sz) ){
        ++sz;
      }
      FDEBUG( printf("CONFLICT %d\n", sz); )
      rc = ensure_line_end(pOut, useCrLf);
      if(!rc) rc = fsl_buffer_append(pOut, mergeMarker[0], mmLen);
      if(!rc) rc = ensure_line_end(pOut, useCrLf);
      RC;
      rc = output_one_side(pOut, pV1, aC1, i1, sz, &i1);
      if(!rc) rc = ensure_line_end(pOut, useCrLf);
      RC;
      rc = fsl_buffer_append(pOut, mergeMarker[1], mmLen);
      if(!rc) rc = ensure_line_end(pOut, useCrLf);
      if(!rc) rc = fsl_buffer_copy_lines(pOut, pPivot, sz);
      if(!rc) rc = ensure_line_end(pOut, useCrLf);
      RC;
      rc = fsl_buffer_append(pOut, mergeMarker[2], mmLen);
      if(!rc) rc = ensure_line_end(pOut, useCrLf);
      if(!rc) rc = output_one_side(pOut, pV2, aC2, i2, sz, &i2);
      if(!rc) rc = ensure_line_end(pOut, useCrLf);
      RC;
      rc = fsl_buffer_append(pOut, mergeMarker[3], mmLen);
      if(!rc) rc = ensure_line_end(pOut, useCrLf);
      RC;
   }

    /* If we are finished with an edit triple, advance to the next
    ** triple.
    */
    if( i1<limit1 && aC1[i1]==0 && aC1[i1+1]==0 && aC1[i1+2]==0 ) i1+=3;
    if( i2<limit2 && aC2[i2]==0 && aC2[i2+1]==0 && aC2[i2+2]==0 ) i2+=3;
  }

  /* When one of the two edit vectors reaches its end, there might still
  ** be an insert in the other edit vector.  Output this remaining
  ** insert.
  */
  FDEBUG( printf("%d: %2d %2d %2d   %d: %2d %2d %2d\n",
         i1/3, aC1[i1], aC1[i1+1], aC1[i1+2],
         i2/3, aC2[i2], aC2[i2+1], aC2[i2+2]); )
  if( i1<limit1 && aC1[i1+2]>0 ){
    FDEBUG( printf("INSERT +%d left\n", aC1[i1+2]); )
    rc = fsl_buffer_copy_lines(pOut, pV1, aC1[i1+2]);
  }else if( i2<limit2 && aC2[i2+2]>0 ){
    FDEBUG( printf("INSERT +%d right\n", aC2[i2+2]); )
    rc = fsl_buffer_copy_lines(pOut, pV2, aC2[i2+2]);
  }

  end:
  fsl_free(aC1);
  fsl_free(aC2);
  if(!rc && conflictCount) *conflictCount = nConflict;
  return rc;
#undef RC
}

/*
** Return true if the input string contains a merge marker on a line by
** itself.
*/
bool fsl_buffer_contains_merge_marker(fsl_buffer const *p){
  fsl_size_t i;
  fsl_size_t const len = (fsl_size_t)assert_mema_lengths();
  if(p->used <= len) return false;
  fsl_size_t j;
  const char * const z = (const char *)p->mem;
  fsl_size_t const n = p->used - len + 1;
  for(i=0; i<n; ){
    for(j=0; j<4; ++j){
      if( (memcmp(&z[i], mergeMarker[j], len)==0)
          && (i+1==n || z[i+len]=='\n' || z[i+len]=='\r') ) return true;
    }
    while( i<n && z[i]!='\n' ){ ++i; }
    while( i<n && (z[i]=='\n' || z[i]=='\r') ){ ++i; }
  }
  return false;
}

#undef mymin
#undef FDEBUG
#undef ISFDEBUG
#undef MARKER
