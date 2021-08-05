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
  This file houses Fossil's diff-generation routines (as opposed to
  the delta-generation).
*/
#include "fossil-scm/fossil.h"
#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h> /* for memmove()/strlen() */

typedef uint64_t u64;
typedef void ReCompiled /* porting crutch. i would strongly prefer to
                           replace the regex support with a stateful
                           predicate callback.
                        */;
#define TO_BE_STATIC /* Porting crutch for unused static funcs */

#define DIFF_CONTEXT_MASK ((u64)0x0000ffff) /* Lines of context. Default if 0 */
#define DIFF_WIDTH_MASK   ((u64)0x00ff0000) /* side-by-side column width */
#define DIFF_IGNORE_EOLWS ((u64)0x01000000) /* Ignore end-of-line whitespace */
#define DIFF_IGNORE_ALLWS ((u64)0x03000000) /* Ignore all whitespace */
#define DIFF_SIDEBYSIDE   ((u64)0x04000000) /* Generate a side-by-side diff */
#define DIFF_VERBOSE      ((u64)0x08000000) /* Missing shown as empty files */
#define DIFF_BRIEF        ((u64)0x10000000) /* Show filenames only */
#define DIFF_HTML         ((u64)0x20000000) /* Render for HTML */
#define DIFF_LINENO       ((u64)0x40000000) /* Show line numbers */
#define DIFF_NOOPT        (((u64)0x01)<<32) /* Suppress optimizations (debug) */
#define DIFF_INVERT       (((u64)0x02)<<32) /* Invert the diff (debug) */
#define DIFF_CONTEXT_EX   (((u64)0x04)<<32) /* Use context even if zero */
#define DIFF_NOTTOOBIG    (((u64)0x08)<<32) /* Only display if not too big */
#define DIFF_STRIP_EOLCR  (((u64)0x10)<<32) /* Strip trailing CR */

/* Annotation flags (any DIFF flag can be used as Annotation flag as well) */
#define ANN_FILE_VERS   (((u64)0x20)<<32) /* Show file vers rather than commit vers */
#define ANN_FILE_ANCEST (((u64)0x40)<<32) /* Prefer check-ins in the ANCESTOR table */

/**
    Maximum length of a line in a text file, in bytes.  (2**13 = 8192 bytes)
 */
#define LENGTH_MASK_SZ  13
#define LENGTH_MASK     ((1<<LENGTH_MASK_SZ)-1)


/*
  ANSI escape codes: https://en.wikipedia.org/wiki/ANSI_escape_code
*/
#define ANSI_COLOR_BLACK(BOLD) ((BOLD) ? "\x1b[30m" : "\x1b[30m")
#define ANSI_COLOR_RED(BOLD) ((BOLD) ? "\x1b[31;1m" : "\x1b[31m")
#define ANSI_COLOR_GREEN(BOLD) ((BOLD) ? "\x1b[32;1m" : "\x1b[32m")
#define ANSI_COLOR_YELLOW(BOLD) ((BOLD) ? "\x1b[33;1m" : "\x1b[33m")
#define ANSI_COLOR_BLUE(BOLD) ((BOLD) ? "\x1b[34;1m" : "\x1b[34m")
#define ANSI_COLOR_MAGENTA(BOLD) ((BOLD) ? "\x1b[35;1m" : "\x1b[35m")
#define ANSI_COLOR_CYAN(BOLD) ((BOLD) ? "\x1b[36;1m" : "\x1b[36m")
#define ANSI_COLOR_WHITE(BOLD) ((BOLD) ? "\x1b[37;1m" : "\x1b[37m")
#define ANSI_DIFF_ADD(BOLD) ANSI_COLOR_GREEN(BOLD)
#define ANSI_DIFF_RM(BOLD) ANSI_COLOR_RED(BOLD)
#define ANSI_DIFF_MOD(BOLD) ANSI_COLOR_BLUE(BOLD)
#define ANSI_BG_BLACK(BOLD) ((BOLD) ? "\x1b[40;1m" : "\x1b[40m")
#define ANSI_BG_RED(BOLD) ((BOLD) ? "\x1b[41;1m" : "\x1b[41m")
#define ANSI_BG_GREEN(BOLD) ((BOLD) ? "\x1b[42;1m" : "\x1b[42m")
#define ANSI_BG_YELLOW(BOLD) ((BOLD) ? "\x1b[43;1m" : "\x1b[43m")
#define ANSI_BG_BLUE(BOLD) ((BOLD) ? "\x1b[44;1m" : "\x1b[44m")
#define ANSI_BG_MAGENTA(BOLD) ((BOLD) ? "\x1b[45;1m" : "\x1b[45m")
#define ANSI_BG_CYAN(BOLD) ((BOLD) ? "\x1b[46;1m" : "\x1b[46m")
#define ANSI_BG_WHITE(BOLD) ((BOLD) ? "\x1b[47;1m" : "\x1b[47m")
#define ANSI_RESET_COLOR   "\x1b[39;49m"
#define ANSI_RESET_ALL     "\x1b[0m"
#define ANSI_RESET ANSI_RESET_ALL
/*#define ANSI_BOLD     ";1m"*/

/**
    Extract the number of lines of context from diffFlags.
 */
static int diff_context_lines(uint64_t diffFlags){
  int n = diffFlags & DIFF_CONTEXT_MASK;
  if( n==0 && (diffFlags & DIFF_CONTEXT_EX)==0 ) n = 5;
  return n;
}

/*
   Extract the width of columns for side-by-side diff.  Supply an
   appropriate default if no width is given.
*/
static int diff_width(uint64_t diffFlags){
  int w = (diffFlags & DIFF_WIDTH_MASK)/(DIFF_CONTEXT_MASK+1);
  if( w==0 ) w = 80;
  return w;
}

/**
    Converts mask of public fsl_diff_flag_t (32-bit) values to the
    Fossil-internal 64-bit bitmask used by the DIFF_xxx macros. Why?
    (A) fossil(1) uses the macro approach and a low-level encoding of
    data in the bitmask (e.g. the context lines count). The public API
    hides the lower-level flags and allows the internal API to take
    care of the encoding.
*/
static uint64_t fsl_diff_flags_convert( int mask ){
  uint64_t rc = 0U;
#define DO(F) if( (mask & F)==F ) rc |= (((u64)F) << 24)
  DO(FSL_DIFF_IGNORE_EOLWS);
  DO(FSL_DIFF_IGNORE_ALLWS);
  DO(FSL_DIFF_SIDEBYSIDE);
  DO(FSL_DIFF_VERBOSE);
  DO(FSL_DIFF_BRIEF);
  DO(FSL_DIFF_HTML);
  DO(FSL_DIFF_LINENO);
  DO(FSL_DIFF_NOOPT);
  DO(FSL_DIFF_INVERT);
  DO(FSL_DIFF_NOTTOOBIG);
  DO(FSL_DIFF_STRIP_EOLCR);
#undef DO
  return rc;
}

/*
   Information about each line of a file being diffed.
  
   The lower LENGTH_MASK_SZ bits of the hash (DLine.h) are the length
   of the line.  If any line is longer than LENGTH_MASK characters,
   the file is considered binary.
*/
typedef struct DLine DLine;
struct DLine {
  const char *z;        /* The text of the line */
  unsigned int h;       /* Hash of the line */
  unsigned short indent;  /* Indent of the line. Only !=0 with -w/-Z option */
  unsigned short n;     /* number of bytes */
  unsigned int iNext;   /* 1+(Index of next line with same the same hash) */

  /* an array of DLine elements serves two purposes.  The fields
     above are one per line of input text.  But each entry is also
     a bucket in a hash table, as follows: */
  unsigned int iHash;   /* 1+(first entry in the hash chain) */
};

/*
   Length of a dline
*/
#define LENGTH(X)   ((X)->n)

/*
   Return an array of DLine objects containing a pointer to the
   start of each line and a hash of that line.  The lower
   bits of the hash store the length of each line.
  
   Trailing whitespace is removed from each line.  2010-08-20:  Not any
   more.  If trailing whitespace is ignored, the "patch" command gets
   confused by the diff output.  Ticket [a9f7b23c2e376af5b0e5b]
  
   If the file is binary or contains a line that is too long, or is
   empty, it sets *pOut to NULL, *pnLine to 0. It returns 0 if no
   lines are found, FSL_RC_TYPE if the input is binary, and
   FSL_RC_RANGE if a some number (e.g. the number of columns) is out
   of range.
  
   Profiling shows that in most cases this routine consumes the bulk
   of the CPU time on a diff.
  
   TODO: enhance the error reporting: add a (fsl_error*) arg.
*/
static int break_into_lines(const char *z, int n, int *pnLine,
                            DLine **pOut, int diffFlags){
  int nLine, i, j, k, s, x;
  unsigned int h, h2;
  DLine *a = NULL;
  /* Count the number of lines.  Allocate space to hold
     the returned array.
  */
  for(i=j=0, nLine=1; i<n; i++, j++){
    int c = z[i];
    if( c==0 ){
      /* printf("BINARY DATA AT BYTE %d\n", i); */
      /* goto gotNone; */
      *pOut = NULL;
      return FSL_RC_TYPE;
    }
    if( c=='\n' && z[i+1]!=0 ){
      nLine++;
      if( j>LENGTH_MASK ){
        /* printf("TOO LONG AT COLUMN %d\n", j); */
        /* goto gotNone; */
        *pOut = NULL;
        return FSL_RC_RANGE;
      }
      j = 0;
    }
  }
  if( j>LENGTH_MASK ){
    /* printf("TOO LONG AGAIN AT COLUMN %d\n", j); */
    /* goto gotNone; */
    *pOut = NULL;
    return FSL_RC_RANGE;
  }
  a = fsl_malloc( nLine*sizeof(a[0]) );
  if(!a) return FSL_RC_OOM;
  memset(a, 0, nLine*sizeof(a[0]) );
  if( n==0 ){
    /* gotNone: */
    *pnLine = 0;
    *pOut = a;
    return 0;
  }

  /* Fill in the array */
  for(i=0; i<nLine; i++){
    for(j=0; z[j] && z[j]!='\n'; j++){}
    a[i].z = z;
    k = j;
    if( diffFlags & DIFF_STRIP_EOLCR ){
      if( k>0 && z[k-1]=='\r' ){ k--; }
    }
    a[i].n = k;
    s = 0;
    if(diffFlags & DIFF_IGNORE_EOLWS){
      while( k>0 && fsl_isspace(z[k-1]) ){ k--; }
    }
    if( (diffFlags & DIFF_IGNORE_ALLWS)==DIFF_IGNORE_ALLWS ){
      while( s<k && fsl_isspace(z[s]) ){ s++; }
    }
    a[i].indent = s;
    for(h=0, x=s; x<k; x++){
      h = h ^ (h<<2) ^ z[x];
    }
    a[i].h = h = (h<<LENGTH_MASK_SZ) | (k-s);
    h2 = h % nLine;
    a[i].iNext = a[h2].iHash;
    a[h2].iHash = i+1;
    z += j+1;
  }

  /* Return results */
  *pnLine = nLine;
  *pOut = a;
  return 0;
}


/*
   Return true if two DLine elements are identical.
*/
static int same_dline(DLine const *pA, DLine const *pB){
  return pA->h==pB->h && memcmp(pA->z+pA->indent,pB->z+pB->indent,
                                pA->h & LENGTH_MASK)==0;
}


/*
** Return true if two DLine elements are identical, ignoring
** all whitespace. The indent field of pA/pB already points
** to the first non-space character in the string.
*/
static int same_dline_ignore_allws(const DLine *pA, const DLine *pB){
  int a = pA->indent, b = pB->indent;
  if( pA->h==pB->h ){
    while( a<pA->n || b<pB->n ){
      if( a<pA->n && b<pB->n && pA->z[a++] != pB->z[b++] ) return 0;
      while( a<pA->n && fsl_isspace(pA->z[a])) ++a;
      while( b<pB->n && fsl_isspace(pB->z[b])) ++b;
    }
    return pA->n-a == pB->n-b;
  }
  return 0;
}

/*
   A context for running a raw diff.
  
   The aEdit[] array describes the raw diff.  Each triple of integers in
   aEdit[] means:
  
     (1) COPY:   Number of lines aFrom and aTo have in common
     (2) DELETE: Number of lines found only in aFrom
     (3) INSERT: Number of lines found only in aTo
  
   The triples repeat until all lines of both aFrom and aTo are accounted
   for.
*/
typedef struct DContext DContext;
struct DContext {
  int *aEdit;        /* Array of copy/delete/insert triples */
  int nEdit;         /* Number of integers (3x num of triples) in aEdit[] */
  int nEditAlloc;    /* Space allocated for aEdit[] */
  DLine *aFrom;      /* File on left side of the diff */
  int nFrom;         /* Number of lines in aFrom[] */
  DLine *aTo;        /* File on right side of the diff */
  int nTo;           /* Number of lines in aTo[] */
  int (*same_fn)(const DLine *, const DLine *); /* Function to be used for comparing */
};

/**
    Holds output state for diff generation.
 */
struct DiffOutState {
  /** Output callback. */
  fsl_output_f out;
  /** State for this->out(). */
  void * oState;
  /** For propagating output errors. */
  int rc;
  char ansiColor;
};
typedef struct DiffOutState DiffOutState;
static const DiffOutState DiffOutState_empty = {
NULL/*out*/,
NULL/*oState*/,
0/*rc*/,
0/*useAnsiColor*/
};

/**
    Internal helper. Sends src to o->out(). If n is negative, fsl_strlen()
    is used to determine the length.
 */
static int diff_out( DiffOutState * o, void const * src, fsl_int_t n ){
  return o->rc = n
    ? o->out(o->oState, src, (n<0)?fsl_strlen((char const *)src):(fsl_size_t)n)
    : 0;
}


/**
    fsl_appendf_f() impl for use with diff_outf(). state must be a
    (DiffOutState*).
 */
static fsl_int_t fsl_appendf_f_diff_out( void * state, char const * s,
                                         fsl_int_t n ){
  DiffOutState * os = (DiffOutState *)state;
  diff_out(os, s, n);
  return os->rc ? -1 : n;
}

static int diff_outf( DiffOutState * o, char const * fmt, ... ){
  va_list va;
  va_start(va,fmt);
  fsl_appendfv(fsl_appendf_f_diff_out, o, fmt, va);
  va_end(va);
  return o->rc;
}

/*
   Append a single line of context-diff output to pOut.
*/
static int appendDiffLine(
  DiffOutState *pOut,         /* Where to write the line of output */
  char cPrefix,       /* One of " ", "+",  or "-" */
  DLine *pLine,       /* The line to be output */
  int html,           /* True if generating HTML.  False for plain text */
  ReCompiled *pRe     /* Colorize only if line matches this Regex */
){
  int rc = 0;
  char const * ansiPrefix =
    !pOut->ansiColor
    ? NULL 
    : (('+'==cPrefix)
       ? ANSI_DIFF_ADD(0)
       : (('-'==cPrefix) ? ANSI_DIFF_RM(0) : NULL))
    ;
  if(ansiPrefix) rc = diff_out(pOut, ansiPrefix, -1 );
  if(!rc) rc = diff_out(pOut, &cPrefix, 1);
  if(rc) return rc;
  else if( html ){
#if 0
    if( pRe /*MISSING: && re_dline_match(pRe, pLine, 1)==0 */ ){
      cPrefix = ' ';
    }else
#endif
    if( cPrefix=='+' ){
      rc = diff_out(pOut, "<span class=\"fsl-diff-add\">", -1);
    }else if( cPrefix=='-' ){
      rc = diff_out(pOut, "<span class=\"fsl-diff-rm\">", -1);
    }
    if(!rc){
      /* unsigned short n = pLine->n; */
      /* while( n>0 && (pLine->z[n-1]=='\n' || pLine->z[n-1]=='\r') ) n--; */
      rc = pOut->rc = fsl_htmlize(pOut->out, pOut->oState,
                                  pLine->z, pLine->n);
      if( !rc && cPrefix!=' ' ){
        rc = diff_out(pOut, "</span>", -1);
      }
    }
  }else{
    rc = diff_out(pOut, pLine->z, pLine->n);
  }
  if(!rc){
    if(ansiPrefix){
      rc = diff_out(pOut, ANSI_RESET, -1 );
    }
    if(!rc) rc = diff_out(pOut, "\n", 1);
  }
  return rc;
}


/*
   Add two line numbers to the beginning of an output line for a context
   diff.  One or the other of the two numbers might be zero, which means
   to leave that number field blank.  The "html" parameter means to format
   the output for HTML.
*/
static int appendDiffLineno(DiffOutState *pOut, int lnA,
                            int lnB, int html){
  int rc = 0;
  if( html ){
    rc = diff_out(pOut, "<span class=\"fsl-diff-lineno\">", -1);
  }
  if(!rc){
    if( lnA>0 ){
      rc = diff_outf(pOut, "%6d ", lnA);
    }else{
      rc = diff_out(pOut, "       ", 7);
    }
  }
  if(!rc){
    if( lnB>0 ){
      rc = diff_outf(pOut, "%6d  ", lnB);
    }else{
      rc = diff_out(pOut, "        ", 8);
    }
    if( !rc && html ){
      rc = diff_out(pOut, "</span>", -1);
    }
  }
  return rc;
}


/*
   Minimum of two values
*/
#define minInt(a,b) (((a)<(b)) ? (a) : (b))


/**
    Compute the optimal longest common subsequence (LCS) using an
    exhaustive search.  This version of the LCS is only used for
    shorter input strings since runtime is O(N*N) where N is the
    input string length.
 */
static void optimalLCS(
  DContext *p,               /* Two files being compared */
  int iS1, int iE1,          /* Range of lines in p->aFrom[] */
  int iS2, int iE2,          /* Range of lines in p->aTo[] */
  int *piSX, int *piEX,      /* Write p->aFrom[] common segment here */
  int *piSY, int *piEY       /* Write p->aTo[] common segment here */
){
  int mxLength = 0;          /* Length of longest common subsequence */
  int i, j;                  /* Loop counters */
  int k;                     /* Length of a candidate subsequence */
  int iSXb = iS1;            /* Best match so far */
  int iSYb = iS2;            /* Best match so far */

  for(i=iS1; i<iE1-mxLength; i++){
    for(j=iS2; j<iE2-mxLength; j++){
      if( !p->same_fn(&p->aFrom[i], &p->aTo[j]) ) continue;
      if( mxLength && !p->same_fn(&p->aFrom[i+mxLength], &p->aTo[j+mxLength]) ){
        continue;
      }
      k = 1;
      while( i+k<iE1 && j+k<iE2 && p->same_fn(&p->aFrom[i+k],&p->aTo[j+k]) ){
        k++;
      }
      if( k>mxLength ){
        iSXb = i;
        iSYb = j;
        mxLength = k;
      }
    }
  }
  *piSX = iSXb;
  *piEX = iSXb + mxLength;
  *piSY = iSYb;
  *piEY = iSYb + mxLength;
}

/**
    Compare two blocks of text on lines iS1 through iE1-1 of the aFrom[]
    file and lines iS2 through iE2-1 of the aTo[] file.  Locate a sequence
    of lines in these two blocks that are exactly the same.  Return
    the bounds of the matching sequence.
   
    If there are two or more possible answers of the same length, the
    returned sequence should be the one closest to the center of the
    input range.
   
    Ideally, the common sequence should be the longest possible common
    sequence.  However, an exact computation of LCS is O(N*N) which is
    way too slow for larger files.  So this routine uses an O(N)
    heuristic approximation based on hashing that usually works about
    as well.  But if the O(N) algorithm doesn't get a good solution
    and N is not too large, we fall back to an exact solution by
    calling optimalLCS().
 */
static void longestCommonSequence(
  DContext *p,               /* Two files being compared */
  int iS1, int iE1,          /* Range of lines in p->aFrom[] */
  int iS2, int iE2,          /* Range of lines in p->aTo[] */
  int *piSX, int *piEX,      /* Write p->aFrom[] common segment here */
  int *piSY, int *piEY       /* Write p->aTo[] common segment here */
){
  int i, j, k;               /* Loop counters */
  int n;                     /* Loop limit */
  DLine *pA, *pB;            /* Pointers to lines */
  int iSX, iSY, iEX, iEY;    /* Current match */
  int skew = 0;              /* How lopsided is the match */
  int dist = 0;              /* Distance of match from center */
  int mid;                   /* Center of the span */
  int iSXb, iSYb, iEXb, iEYb;   /* Best match so far */
  int iSXp, iSYp, iEXp, iEYp;   /* Previous match */
  sqlite3_int64 bestScore;      /* Best score so far */
  sqlite3_int64 score;          /* Score for current candidate LCS */
  int span;                     /* combined width of the input sequences */

  span = (iE1 - iS1) + (iE2 - iS2);
  bestScore = -10000;
  score = 0;
  iSXb = iSXp = iS1;
  iEXb = iEXp = iS1;
  iSYb = iSYp = iS2;
  iEYb = iEYp = iS2;
  mid = (iE1 + iS1)/2;
  for(i=iS1; i<iE1; i++){
    int limit = 0;
    j = p->aTo[p->aFrom[i].h % p->nTo].iHash;
    while( j>0
      && (j-1<iS2 || j>=iE2 || !p->same_fn(&p->aFrom[i], &p->aTo[j-1]))
    ){
      if( limit++ > 10 ){
        j = 0;
        break;
      }
      j = p->aTo[j-1].iNext;
    }
    if( j==0 ) continue;
    assert( i>=iSXb && i>=iSXp );
    if( i<iEXb && j>=iSYb && j<iEYb ) continue;
    if( i<iEXp && j>=iSYp && j<iEYp ) continue;
    iSX = i;
    iSY = j-1;
    pA = &p->aFrom[iSX-1];
    pB = &p->aTo[iSY-1];
    n = minInt(iSX-iS1, iSY-iS2);
    for(k=0; k<n && p->same_fn(pA,pB); k++, pA--, pB--){}
    iSX -= k;
    iSY -= k;
    iEX = i+1;
    iEY = j;
    pA = &p->aFrom[iEX];
    pB = &p->aTo[iEY];
    n = minInt(iE1-iEX, iE2-iEY);
    for(k=0; k<n && p->same_fn(pA,pB); k++, pA++, pB++){}
    iEX += k;
    iEY += k;
    skew = (iSX-iS1) - (iSY-iS2);
    if( skew<0 ) skew = -skew;
    dist = (iSX+iEX)/2 - mid;
    if( dist<0 ) dist = -dist;
    score = (iEX - iSX)*(sqlite3_int64)span - (skew + dist);
    if( score>bestScore ){
      bestScore = score;
      iSXb = iSX;
      iSYb = iSY;
      iEXb = iEX;
      iEYb = iEY;
    }else if( iEX>iEXp ){
      iSXp = iSX;
      iSYp = iSY;
      iEXp = iEX;
      iEYp = iEY;
    }
  }
  if( iSXb==iEXb && (int64_t)(iE1-iS1)*(iE2-iS2)<400 ){
    /* If no common sequence is found using the hashing heuristic and
       the input is not too big, use the expensive exact solution */
    optimalLCS(p, iS1, iE1, iS2, iE2, piSX, piEX, piSY, piEY);
  }else{
    *piSX = iSXb;
    *piSY = iSYb;
    *piEX = iEXb;
    *piEY = iEYb;
  }
}

/**
    Expand the size of p->aEdit array to hold at least nEdit elements.
 */
static int expandEdit(DContext *p, int nEdit){
  void * re = fsl_realloc(p->aEdit, nEdit*sizeof(int));
  if(!re) return FSL_RC_OOM;
  else{
    p->aEdit = (int*)re;
    p->nEditAlloc = nEdit;
    return 0;
  }
}

/**
    Append a new COPY/DELETE/INSERT triple.
   
    Returns 0 on success, FSL_RC_OOM on OOM.
 */
static int appendTriple(DContext *p, int nCopy, int nDel, int nIns){
  /* printf("APPEND %d/%d/%d\n", nCopy, nDel, nIns); */
  if( p->nEdit>=3 ){
    if( p->aEdit[p->nEdit-1]==0 ){
      if( p->aEdit[p->nEdit-2]==0 ){
        p->aEdit[p->nEdit-3] += nCopy;
        p->aEdit[p->nEdit-2] += nDel;
        p->aEdit[p->nEdit-1] += nIns;
        return 0;
      }
      if( nCopy==0 ){
        p->aEdit[p->nEdit-2] += nDel;
        p->aEdit[p->nEdit-1] += nIns;
        return 0;
      }
    }
    if( nCopy==0 && nDel==0 ){
      p->aEdit[p->nEdit-1] += nIns;
      return 0;
    }
  }
  if( p->nEdit+3>p->nEditAlloc ){
    int const rc = expandEdit(p, p->nEdit*2 + 15);
    if(rc) return rc;
    else if( p->aEdit==0 ) return 0;
  }
  p->aEdit[p->nEdit++] = nCopy;
  p->aEdit[p->nEdit++] = nDel;
  p->aEdit[p->nEdit++] = nIns;
  return 0;
}


/**
    Do a single step in the difference.  Compute a sequence of
    copy/delete/insert steps that will convert lines iS1 through iE1-1
    of the input into lines iS2 through iE2-1 of the output and write
    that sequence into the difference context.
   
    The algorithm is to find a block of common text near the middle of
    the two segments being diffed.  Then recursively compute
    differences on the blocks before and after that common segment.
    Special cases apply if either input segment is empty or if the two
    segments have no text in common.
 */
static int diff_step(DContext *p, int iS1, int iE1, int iS2, int iE2){
  int iSX, iEX, iSY, iEY;
  int rc = 0;
  if( iE1<=iS1 ){
    /* The first segment is empty */
    if( iE2>iS2 ){
      rc = appendTriple(p, 0, 0, iE2-iS2);
    }
    return rc;
  }
  if( iE2<=iS2 ){
    /* The second segment is empty */
    return appendTriple(p, 0, iE1-iS1, 0);
  }

  /* Find the longest matching segment between the two sequences */
  longestCommonSequence(p, iS1, iE1, iS2, iE2, &iSX, &iEX, &iSY, &iEY);

  if( iEX>iSX ){
    /* A common segment has been found.
       Recursively diff either side of the matching segment */
    rc = diff_step(p, iS1, iSX, iS2, iSY);
    if( !rc){
      if(iEX>iSX){
        rc = appendTriple(p, iEX - iSX, 0, 0);
      }
      if(!rc) rc = diff_step(p, iEX, iE1, iEY, iE2);
    }
  }else{
    /* The two segments have nothing in common.  Delete the first then
       insert the second. */
    rc = appendTriple(p, 0, iE1-iS1, iE2-iS2);
  }
  return rc;
}

/**
    Compute the differences between two files already loaded into
    the DContext structure.
   
    A divide and conquer technique is used.  We look for a large
    block of common text that is in the middle of both files.  Then
    compute the difference on those parts of the file before and
    after the common block.  This technique is fast, but it does
    not necessarily generate the minimum difference set.  On the
    other hand, we do not need a minimum difference set, only one
    that makes sense to human readers, which this algorithm does.
   
    Any common text at the beginning and end of the two files is
    removed before starting the divide-and-conquer algorithm.
   
    Returns 0 on succes, FSL_RC_OOM on an allocation error.
 */
static int diff_all(DContext *p){
  int mnE, iS, iE1, iE2;
  int rc = 0;
  /* Carve off the common header and footer */
  iE1 = p->nFrom;
  iE2 = p->nTo;
  while( iE1>0 && iE2>0 && p->same_fn(&p->aFrom[iE1-1], &p->aTo[iE2-1]) ){
    iE1--;
    iE2--;
  }
  mnE = iE1<iE2 ? iE1 : iE2;
  for(iS=0; iS<mnE && p->same_fn(&p->aFrom[iS],&p->aTo[iS]); iS++){}

  /* do the difference */
  if( iS>0 ){
    rc = appendTriple(p, iS, 0, 0);
    if(rc) return rc;
  }
  rc = diff_step(p, iS, iE1, iS, iE2);
  if(rc) return rc;
  if( iE1<p->nFrom ){
    rc = appendTriple(p, p->nFrom - iE1, 0, 0);
    if(rc) return rc;
  }

  /* Terminate the COPY/DELETE/INSERT triples with three zeros */
  rc = expandEdit(p, p->nEdit+3);
  if(rc) return rc;
  else if( p->aEdit ){
    p->aEdit[p->nEdit++] = 0;
    p->aEdit[p->nEdit++] = 0;
    p->aEdit[p->nEdit++] = 0;
  }
  return rc;
}


/*
   Attempt to shift insertion or deletion blocks so that they begin and
   end on lines that are pure whitespace.  In other words, try to transform
   this:
  
        int func1(int x){
           return x*10;
       +}
       +
       +int func2(int x){
       +   return x*20;
        }
  
        int func3(int x){
           return x/5;
        }
  
   Into one of these:
  
        int func1(int x){              int func1(int x){
           return x*10;                   return x*10;
        }                              }
       +
       +int func2(int x){             +int func2(int x){
       +   return x*20;               +   return x*20;
       +}                             +}
                                      +
        int func3(int x){              int func3(int x){
           return x/5;                    return x/5;
        }                              }
*/
static void diff_optimize(DContext *p){
  int r;       /* Index of current triple */
  int lnFrom;  /* Line number in p->aFrom */
  int lnTo;    /* Line number in p->aTo */
  int cpy, del, ins;

  lnFrom = lnTo = 0;
  for(r=0; r<p->nEdit; r += 3){
    cpy = p->aEdit[r];
    del = p->aEdit[r+1];
    ins = p->aEdit[r+2];
    lnFrom += cpy;
    lnTo += cpy;

    /* Shift insertions toward the beginning of the file */
    while( cpy>0 && del==0 && ins>0 ){
      DLine *pTop = &p->aFrom[lnFrom-1];  /* Line before start of insert */
      DLine *pBtm = &p->aTo[lnTo+ins-1];  /* Last line inserted */
      if( p->same_fn(pTop, pBtm)==0 ) break;
      if( LENGTH(pTop+1)+LENGTH(pBtm)<=LENGTH(pTop)+LENGTH(pBtm-1) ) break;
      lnFrom--;
      lnTo--;
      p->aEdit[r]--;
      p->aEdit[r+3]++;
      cpy--;
    }

    /* Shift insertions toward the end of the file */
    while( r+3<p->nEdit && p->aEdit[r+3]>0 && del==0 && ins>0 ){
      DLine *pTop = &p->aTo[lnTo];       /* First line inserted */
      DLine *pBtm = &p->aTo[lnTo+ins];   /* First line past end of insert */
      if( p->same_fn(pTop, pBtm)==0 ) break;
      if( LENGTH(pTop)+LENGTH(pBtm-1)<=LENGTH(pTop+1)+LENGTH(pBtm) ) break;
      lnFrom++;
      lnTo++;
      p->aEdit[r]++;
      p->aEdit[r+3]--;
      cpy++;
    }

    /* Shift deletions toward the beginning of the file */
    while( cpy>0 && del>0 && ins==0 ){
      DLine *pTop = &p->aFrom[lnFrom-1];     /* Line before start of delete */
      DLine *pBtm = &p->aFrom[lnFrom+del-1]; /* Last line deleted */
      if( p->same_fn(pTop, pBtm)==0 ) break;
      if( LENGTH(pTop+1)+LENGTH(pBtm)<=LENGTH(pTop)+LENGTH(pBtm-1) ) break;
      lnFrom--;
      lnTo--;
      p->aEdit[r]--;
      p->aEdit[r+3]++;
      cpy--;
    }

    /* Shift deletions toward the end of the file */
    while( r+3<p->nEdit && p->aEdit[r+3]>0 && del>0 && ins==0 ){
      DLine *pTop = &p->aFrom[lnFrom];     /* First line deleted */
      DLine *pBtm = &p->aFrom[lnFrom+del]; /* First line past end of delete */
      if( p->same_fn(pTop, pBtm)==0 ) break;
      if( LENGTH(pTop)+LENGTH(pBtm-1)<=LENGTH(pTop)+LENGTH(pBtm) ) break;
      lnFrom++;
      lnTo++;
      p->aEdit[r]++;
      p->aEdit[r+3]--;
      cpy++;
    }

    lnFrom += del;
    lnTo += ins;
  }
}


/*
   Given a raw diff p[] in which the p->aEdit[] array has been filled
   in, compute a context diff into pOut.
*/
static int contextDiff(
  DContext *p,      /* The difference */
  DiffOutState *pOut,       /* Output a context diff to here */
  ReCompiled *pRe,  /* Only show changes that match this regex */
  u64 diffFlags     /* Flags controlling the diff format */
){
  DLine *A;     /* Left side of the diff */
  DLine *B;     /* Right side of the diff */
  int a = 0;    /* Index of next line in A[] */
  int b = 0;    /* Index of next line in B[] */
  int *R;       /* Array of COPY/DELETE/INSERT triples */
  int r;        /* Index into R[] */
  int nr;       /* Number of COPY/DELETE/INSERT triples to process */
  int mxr;      /* Maximum value for r */
  int na, nb;   /* Number of lines shown from A and B */
  int i, j;     /* Loop counters */
  int m;        /* Number of lines to output */
  int skip;     /* Number of lines to skip */
  static int nChunk = 0;  /* Number of diff chunks seen so far */
  int nContext;    /* Number of lines of context */
  int showLn;      /* Show line numbers */
  int html;        /* Render as HTML */
  int showDivider = 0;  /* True to show the divider between diff blocks */
  int rc = 0;
  nContext = diff_context_lines(diffFlags);
  showLn = (diffFlags & DIFF_LINENO)!=0;
  html = (diffFlags & DIFF_HTML)!=0;
  A = p->aFrom;
  B = p->aTo;
  R = p->aEdit;
  mxr = p->nEdit;
  while( mxr>2 && R[mxr-1]==0 && R[mxr-2]==0 ){ mxr -= 3; }
  for(r=0; r<mxr; r += 3*nr){
    /* Figure out how many triples to show in a single block */
    for(nr=1; R[r+nr*3]>0 && R[r+nr*3]<nContext*2; nr++){}
    /* printf("r=%d nr=%d\n", r, nr); */

    /* If there is a regex, skip this block (generate no diff output)
       if the regex matches or does not match both insert and delete.
       Only display the block if one side matches but the other side does
       not.
    */
#if 0
    /* MISSING: re. i would prefer a predicate function,
       anyway.
    */
    if( pRe ){
      int hideBlock = 1;
      int xa = a, xb = b;
      for(i=0; hideBlock && i<nr; i++){
        int c1, c2;
        xa += R[r+i*3];
        xb += R[r+i*3];
        c1 = re_dline_match(pRe, &A[xa], R[r+i*3+1]);
        c2 = re_dline_match(pRe, &B[xb], R[r+i*3+2]);
        hideBlock = c1==c2;
        xa += R[r+i*3+1];
        xb += R[r+i*3+2];
      }
      if( hideBlock ){
        a = xa;
        b = xb;
        continue;
      }
    }
#endif
    /* For the current block comprising nr triples, figure out
       how many lines of A and B are to be displayed
    */
    if( R[r]>nContext ){
      na = nb = nContext;
      skip = R[r] - nContext;
    }else{
      na = nb = R[r];
      skip = 0;
    }
    for(i=0; i<nr; i++){
      na += R[r+i*3+1];
      nb += R[r+i*3+2];
    }
    if( R[r+nr*3]>nContext ){
      na += nContext;
      nb += nContext;
    }else{
      na += R[r+nr*3];
      nb += R[r+nr*3];
    }
    for(i=1; i<nr; i++){
      na += R[r+i*3];
      nb += R[r+i*3];
    }

    /* Show the header for this block, or if we are doing a modified
       context diff that contains line numbers, show the separator from
       the previous block.
    */
    nChunk++;
    if( showLn ){
      if( !showDivider ){
        /* Do not show a top divider */
        showDivider = 1;
      }else if( html ){
        rc = diff_outf(pOut, "<span class=\"fsl-diff-hr\">%.80c</span>\n", '.');
      }else{
        rc = diff_outf(pOut, "%.80c\n", '.');
      }
      if( !rc && html ){
        rc = diff_outf(pOut, "<span class=\"fsl-diff-chunk-%d\"></span>", nChunk);
      }
    }else{
      char const * ansi1 = "";
      char const * ansi2 = "";
      char const * ansi3 = "";
      if( html ) rc = diff_outf(pOut, "<span class=\"fsl-diff-lineno\">");
      else if(0 && pOut->ansiColor){
        /* Turns out this just confuses the output */
        ansi1 = ANSI_DIFF_RM(0);
        ansi2 = ANSI_DIFF_ADD(0);
        ansi3 = ANSI_RESET;
      }
      /*
       * If the patch changes an empty file or results in an empty file,
       * the block header must use 0,0 as position indicator and not 1,0.
       * Otherwise, patch would be confused and may reject the diff.
       */
      if(!rc) rc = diff_outf(pOut,"@@ %s-%d,%d %s+%d,%d%s @@",
                             ansi1, na ? a+skip+1 : 0, na,
                             ansi2, nb ? b+skip+1 : 0, nb,
                             ansi3);
      if( !rc ){
        if( html ) rc = diff_outf(pOut, "</span>");
        if(!rc) rc = diff_out(pOut, "\n", 1);
      }
    }
    if(rc) return rc;

    /* Show the initial common area */
    a += skip;
    b += skip;
    m = R[r] - skip;
    for(j=0; !rc && j<m; j++){
      if( showLn ) rc = appendDiffLineno(pOut, a+j+1, b+j+1, html);
      if(!rc) rc = appendDiffLine(pOut, ' ', &A[a+j], html, 0);
    }
    if(rc) return rc;
    a += m;
    b += m;

    /* Show the differences */
    for(i=0; i<nr; i++){
      m = R[r+i*3+1];
      for(j=0; !rc && j<m; j++){
        if( showLn ) rc = appendDiffLineno(pOut, a+j+1, 0, html);
        if(!rc) rc = appendDiffLine(pOut, '-', &A[a+j], html, pRe);
      }
      if(rc) return rc;
      a += m;
      m = R[r+i*3+2];
      for(j=0; !rc && j<m; j++){
        if( showLn ) rc = appendDiffLineno(pOut, 0, b+j+1, html);
        if(!rc) rc = appendDiffLine(pOut, '+', &B[b+j], html, pRe);
      }
      if(rc) return rc;
      b += m;
      if( i<nr-1 ){
        m = R[r+i*3+3];
        for(j=0; !rc && j<m; j++){
          if( showLn ) rc = appendDiffLineno(pOut, a+j+1, b+j+1, html);
          if(!rc) rc = appendDiffLine(pOut, ' ', &A[a+j], html, 0);
        }
        if(rc) return rc;
        b += m;
        a += m;
      }
    }

    /* Show the final common area */
    assert( nr==i );
    m = R[r+nr*3];
    if( m>nContext ) m = nContext;
    for(j=0; !rc && j<m; j++){
      if( showLn ) rc = appendDiffLineno(pOut, a+j+1, b+j+1, html);
      if(!rc) rc = appendDiffLine(pOut, ' ', &A[a+j], html, 0);
    }
  }/*big for() loop*/
  return rc;
}

/*
   Status of a single output line
*/
typedef struct SbsLine SbsLine;
struct SbsLine {
  fsl_buffer *apCols[5];   /* Array of pointers to output columns */
  int width;               /* Maximum width of a column in the output */
  unsigned char escHtml;   /* True to escape html characters */
  int iStart;              /* Write zStart prior to character iStart */
  const char *zStart;      /* A <span> tag */
  int iEnd;                /* Write </span> prior to character iEnd */
  int iStart2;             /* Write zStart2 prior to character iStart2 */
  const char *zStart2;     /* A <span> tag */
  int iEnd2;               /* Write </span> prior to character iEnd2 */
  ReCompiled *pRe;         /* Only colorize matching lines, if not NULL */
  DiffOutState * pOut;
};

/*
   Column indices for SbsLine.apCols[]
*/
#define SBS_LNA  0     /* Left line number */
#define SBS_TXTA 1     /* Left text */
#define SBS_MKR  2     /* Middle separator column */
#define SBS_LNB  3     /* Right line number */
#define SBS_TXTB 4     /* Right text */

/*
   Append newlines to all columns.
*/
static int sbsWriteNewlines(SbsLine *p){
  int i;
  int rc = 0;
  for( i=p->escHtml ? SBS_LNA : SBS_TXTB; !rc && i<=SBS_TXTB; i++ ){
    rc = fsl_buffer_append(p->apCols[i], "\n", 1);
  }
  return rc;
}

/*
   Append n spaces to the column.
*/
static int sbsWriteSpace(SbsLine *p, int n, int col){
  return fsl_buffer_appendf(p->apCols[col], "%*s", n, "");
}

/*
   Write the text of pLine into column iCol of p.
  
   If outputting HTML, write the full line.  Otherwise, only write the
   width characters.  Translate tabs into spaces.  Add newlines if col
   is SBS_TXTB.  Translate HTML characters if escHtml is true.  Pad the
   rendering to width bytes if col is SBS_TXTA and escHtml is false.
  
   This comment contains multibyte unicode characters (�, �, �) in order
   to test the ability of the diff code to handle such characters.
*/
static int sbsWriteText(SbsLine *p, DLine *pLine, int col){
  fsl_buffer *pCol = p->apCols[col];
  int rc = 0;
  int n = pLine->n;
  int i;   /* Number of input characters consumed */
  int k;   /* Cursor position */
  int needEndSpan = 0;
  const char *zIn = pLine->z;
  int w = p->width;
  int colorize = p->escHtml;
#if 0
  /* MISSING: re bits, but want to replace those with a predicate. */
  if( colorize && p->pRe && re_dline_match(p->pRe, pLine, 1)==0 ){
    colorize = 0;
  }
#endif
  for(i=k=0; !rc && (p->escHtml || k<w) && i<n; i++, k++){
    char c = zIn[i];
    if( colorize ){
      if( i==p->iStart ){
        rc = fsl_buffer_append(pCol, p->zStart, -1);
        if(rc) break;
        needEndSpan = 1;
        if( p->iStart2 ){
          p->iStart = p->iStart2;
          p->zStart = p->zStart2;
          p->iStart2 = 0;
        }
      }else if( i==p->iEnd ){
        rc = fsl_buffer_append(pCol, "</span>", 7);
        if(rc) break;
        needEndSpan = 0;
        if( p->iEnd2 ){
          p->iEnd = p->iEnd2;
          p->iEnd2 = 0;
        }
      }
    }
    if( c=='\t' && !p->escHtml ){
      rc = fsl_buffer_append(pCol, " ", 1);
      while( !rc && (k&7)!=7 && (p->escHtml || k<w) ){
        rc = fsl_buffer_append(pCol, " ", 1);
        k++;
      }
    }else if( c=='\r' || c=='\f' ){
      rc = fsl_buffer_append(pCol, " ", 1);
    }else if( c=='<' && p->escHtml ){
      rc = fsl_buffer_append(pCol, "&lt;", 4);
    }else if( c=='&' && p->escHtml ){
      rc = fsl_buffer_append(pCol, "&amp;", 5);
    }else if( c=='>' && p->escHtml ){
      rc = fsl_buffer_append(pCol, "&gt;", 4);
    }else if( c=='"' && p->escHtml ){
      rc = fsl_buffer_append(pCol, "&quot;", 6);
    }else{
      rc = fsl_buffer_append(pCol, &zIn[i], 1);
      if( (c&0xc0)==0x80 ) k--;
    }
  }
  if( !rc && needEndSpan ){
    rc = fsl_buffer_append(pCol, "</span>", 7);
  }
  if(!rc){
    if( col==SBS_TXTB ){
      rc = sbsWriteNewlines(p);
    }else if( !p->escHtml ){
      rc = sbsWriteSpace(p, w-k, SBS_TXTA);
    }
  }
  return rc;
}


/*
   Append a column to the final output blob.
*/
static int sbsWriteColumn(DiffOutState *pOut, fsl_buffer const *pCol, int col){
  return diff_outf(pOut,
    "<td><div class=\"fsl-diff-%s-col\">\n"
    "<pre>\n"
    "%b"
    "</pre>\n"
    "</div></td>\n",
    col % 3 ? (col == SBS_MKR ? "separator" : "text") : "lineno",
    pCol
  );
}


/*
   Append a separator line to column iCol
*/
static int sbsWriteSep(SbsLine *p, int len, int col){
  char ch = '.';
  if( len<1 ){
    len = 1;
    ch = ' ';
  }
  return fsl_buffer_appendf(p->apCols[col],
                            "<span class=\"fsl-diff-hr\">%.*c</span>\n",
                            len, ch);
}


/*
   Append the appropriate marker into the center column of the diff.
*/
static int sbsWriteMarker(SbsLine *p, const char *zTxt,
                                 const char *zHtml){
  return fsl_buffer_append(p->apCols[SBS_MKR], p->escHtml ? zHtml : zTxt, -1);
}

/*
   Append a line number to the column.
*/
static int sbsWriteLineno(SbsLine *p, int ln, int col){
  int rc;
  if( p->escHtml ){
    rc = fsl_buffer_appendf(p->apCols[col], "%d", ln+1);
  }else{
    char zLn[8];
    fsl_snprintf(zLn, 8, "%5d ", ln+1);
    rc = fsl_buffer_appendf(p->apCols[col], "%s ", zLn);
  }
  return rc;
}


/*
   The two text segments zLeft and zRight are known to be different on
   both ends, but they might have  a common segment in the middle.  If
   they do not have a common segment, return 0.  If they do have a large
   common segment, return 1 and before doing so set:
  
     aLCS[0] = start of the common segment in zLeft
     aLCS[1] = end of the common segment in zLeft
     aLCS[2] = start of the common segment in zLeft
     aLCS[3] = end of the common segment in zLeft
  
   This computation is for display purposes only and does not have to be
   optimal or exact.
*/
static int textLCS(
  const char *zLeft,  int nA,       /* String on the left */
  const char *zRight,  int nB,      /* String on the right */
  int *aLCS                         /* Identify bounds of LCS here */
){
  const unsigned char *zA = (const unsigned char*)zLeft;    /* left string */
  const unsigned char *zB = (const unsigned char*)zRight;   /* right string */
  int nt;                    /* Number of target points */
  int ti[3];                 /* Index for start of each 4-byte target */
  unsigned int target[3];    /* 4-byte alignment targets */
  unsigned int probe;        /* probe to compare against target */
  int iAS, iAE, iBS, iBE;    /* Range of common segment */
  int i, j;                  /* Loop counters */
  int rc = 0;                /* Result code.  1 for success */

  if( nA<6 || nB<6 ) return 0;
  memset(aLCS, 0, sizeof(int)*4);
  ti[0] = i = nB/2-2;
  target[0] = (zB[i]<<24) | (zB[i+1]<<16) | (zB[i+2]<<8) | zB[i+3];
  probe = 0;
  if( nB<16 ){
    nt = 1;
  }else{
    ti[1] = i = nB/4-2;
    target[1] = (zB[i]<<24) | (zB[i+1]<<16) | (zB[i+2]<<8) | zB[i+3];
    ti[2] = i = (nB*3)/4-2;
    target[2] = (zB[i]<<24) | (zB[i+1]<<16) | (zB[i+2]<<8) | zB[i+3];
    nt = 3;
  }
  probe = (zA[0]<<16) | (zA[1]<<8) | zA[2];
  for(i=3; i<nA; i++){
    probe = (probe<<8) | zA[i];
    for(j=0; j<nt; j++){
      if( probe==target[j] ){
        iAS = i-3;
        iAE = i+1;
        iBS = ti[j];
        iBE = ti[j]+4;
        while( iAE<nA && iBE<nB && zA[iAE]==zB[iBE] ){ iAE++; iBE++; }
        while( iAS>0 && iBS>0 && zA[iAS-1]==zB[iBS-1] ){ iAS--; iBS--; }
        if( iAE-iAS > aLCS[1] - aLCS[0] ){
          aLCS[0] = iAS;
          aLCS[1] = iAE;
          aLCS[2] = iBS;
          aLCS[3] = iBE;
          rc = 1;
        }
      }
    }
  }
  return rc;
}


/*
   Try to shift iStart as far as possible to the left.
*/
static void sbsShiftLeft(SbsLine *p, const char *z){
  int i, j;
  while( (i=p->iStart)>0 && z[i-1]==z[i] ){
    for(j=i+1; j<p->iEnd && z[j-1]==z[j]; j++){}
    if( j<p->iEnd ) break;
    p->iStart--;
    p->iEnd--;
  }
}

/*
   Simplify iStart and iStart2:
  
      *  If iStart is a null-change then move iStart2 into iStart
      *  Make sure any null-changes are in canonoical form.
      *  Make sure all changes are at character boundaries for
         multi-byte characters.
*/
static void sbsSimplifyLine(SbsLine *p, const char *z){
  if( p->iStart2==p->iEnd2 ){
    p->iStart2 = p->iEnd2 = 0;
  }else if( p->iStart2 ){
    while( p->iStart2>0 && (z[p->iStart2]&0xc0)==0x80 ) p->iStart2--;
    while( (z[p->iEnd2]&0xc0)==0x80 ) p->iEnd2++;
  }
  if( p->iStart==p->iEnd ){
    p->iStart = p->iStart2;
    p->iEnd = p->iEnd2;
    p->zStart = p->zStart2;
    p->iStart2 = 0;
    p->iEnd2 = 0;
  }
  if( p->iStart==p->iEnd ){
    p->iStart = p->iEnd = -1;
  }else if( p->iStart>0 ){
    while( p->iStart>0 && (z[p->iStart]&0xc0)==0x80 ) p->iStart--;
    while( (z[p->iEnd]&0xc0)==0x80 ) p->iEnd++;
  }
}


/*
   Write out lines that have been edited.  Adjust the highlight to cover
   only those parts of the line that actually changed.
*/
static int sbsWriteLineChange(
  SbsLine *p,          /* The SBS output line */
  DLine *pLeft,        /* Left line of the change */
  int lnLeft,          /* Line number for the left line */
  DLine *pRight,       /* Right line of the change */
  int lnRight          /* Line number of the right line */
){
  int rc = 0;
  int nLeft;           /* Length of left line in bytes */
  int nRight;          /* Length of right line in bytes */
  int nShort;          /* Shortest of left and right */
  int nPrefix;         /* Length of common prefix */
  int nSuffix;         /* Length of common suffix */
  const char *zLeft;   /* Text of the left line */
  const char *zRight;  /* Text of the right line */
  int nLeftDiff;       /* nLeft - nPrefix - nSuffix */
  int nRightDiff;      /* nRight - nPrefix - nSuffix */
  int aLCS[4];         /* Bounds of common middle segment */
  static const char zClassRm[]   = "<span class=\"fsl-diff-rm\">";
  static const char zClassAdd[]  = "<span class=\"fsl-diff-add\">";
  static const char zClassChng[] = "<span class=\"fsl-diff-change\">";

  nLeft = pLeft->n;
  zLeft = pLeft->z;
  nRight = pRight->n;
  zRight = pRight->z;
  nShort = nLeft<nRight ? nLeft : nRight;

  nPrefix = 0;
  while( nPrefix<nShort && zLeft[nPrefix]==zRight[nPrefix] ){
    nPrefix++;
  }
  if( nPrefix<nShort ){
    while( nPrefix>0 && (zLeft[nPrefix]&0xc0)==0x80 ) nPrefix--;
  }
  nSuffix = 0;
  if( nPrefix<nShort ){
    while( nSuffix<nShort && zLeft[nLeft-nSuffix-1]==zRight[nRight-nSuffix-1] ){
      nSuffix++;
    }
    if( nSuffix<nShort ){
      while( nSuffix>0 && (zLeft[nLeft-nSuffix]&0xc0)==0x80 ) nSuffix--;
    }
    if( nSuffix==nLeft || nSuffix==nRight ) nPrefix = 0;
  }
  if( nPrefix+nSuffix > nShort ) nPrefix = nShort - nSuffix;

  /* A single chunk of text inserted on the right */
  if( nPrefix+nSuffix==nLeft ){
    rc = sbsWriteLineno(p, lnLeft, SBS_LNA);
    if(rc) return rc;
    p->iStart2 = p->iEnd2 = 0;
    p->iStart = p->iEnd = -1;
    rc = sbsWriteText(p, pLeft, SBS_TXTA);
    if( !rc && nLeft==nRight && zLeft[nLeft]==zRight[nRight] ){
      rc = sbsWriteMarker(p, "   ", "");
    }else{
      rc = sbsWriteMarker(p, " | ", "|");
    }
    if(!rc){
      rc = sbsWriteLineno(p, lnRight, SBS_LNB);
      if(!rc){
        p->iStart = nPrefix;
        p->iEnd = nRight - nSuffix;
        p->zStart = zClassAdd;
        rc = sbsWriteText(p, pRight, SBS_TXTB);
      }
    }
    return rc;
  }

  /* A single chunk of text deleted from the left */
  if( nPrefix+nSuffix==nRight ){
    /* Text deleted from the left */
    rc = sbsWriteLineno(p, lnLeft, SBS_LNA);
    if(!rc){
      p->iStart2 = p->iEnd2 = 0;
      p->iStart = nPrefix;
      p->iEnd = nLeft - nSuffix;
      p->zStart = zClassRm;
      rc = sbsWriteText(p, pLeft, SBS_TXTA);
      if(!rc){
        rc = sbsWriteMarker(p, " | ", "|");
        if(!rc){
          rc = sbsWriteLineno(p, lnRight, SBS_LNB);
          if(!rc){
            p->iStart = p->iEnd = -1;
            sbsWriteText(p, pRight, SBS_TXTB);
          }
        }
      }
    }
    return rc;
  }

  /* At this point we know that there is a chunk of text that has
     changed between the left and the right.  Check to see if there
     is a large unchanged section in the middle of that changed block.
  */
  nLeftDiff = nLeft - nSuffix - nPrefix;
  nRightDiff = nRight - nSuffix - nPrefix;
  if( p->escHtml
   && nLeftDiff >= 6
   && nRightDiff >= 6
   && textLCS(&zLeft[nPrefix], nLeftDiff, &zRight[nPrefix], nRightDiff, aLCS)
  ){
    rc = sbsWriteLineno(p, lnLeft, SBS_LNA);
    if(rc) return rc;
    p->iStart = nPrefix;
    p->iEnd = nPrefix + aLCS[0];
    if( aLCS[2]==0 ){
      sbsShiftLeft(p, pLeft->z);
      p->zStart = zClassRm;
    }else{
      p->zStart = zClassChng;
    }
    p->iStart2 = nPrefix + aLCS[1];
    p->iEnd2 = nLeft - nSuffix;
    p->zStart2 = aLCS[3]==nRightDiff ? zClassRm : zClassChng;
    sbsSimplifyLine(p, zLeft+nPrefix);
    rc = sbsWriteText(p, pLeft, SBS_TXTA);
    if(!rc) rc = sbsWriteMarker(p, " | ", "|");
    if(!rc) rc = sbsWriteLineno(p, lnRight, SBS_LNB);
    if(rc) return rc;
    p->iStart = nPrefix;
    p->iEnd = nPrefix + aLCS[2];
    if( aLCS[0]==0 ){
      sbsShiftLeft(p, pRight->z);
      p->zStart = zClassAdd;
    }else{
      p->zStart = zClassChng;
    }
    p->iStart2 = nPrefix + aLCS[3];
    p->iEnd2 = nRight - nSuffix;
    p->zStart2 = aLCS[1]==nLeftDiff ? zClassAdd : zClassChng;
    sbsSimplifyLine(p, zRight+nPrefix);
    rc = sbsWriteText(p, pRight, SBS_TXTB);
    return rc;
  }

  /* If all else fails, show a single big change between left and right */
  rc = sbsWriteLineno(p, lnLeft, SBS_LNA);
  if(!rc){
    p->iStart2 = p->iEnd2 = 0;
    p->iStart = nPrefix;
    p->iEnd = nLeft - nSuffix;
    p->zStart = zClassChng;
    rc = sbsWriteText(p, pLeft, SBS_TXTA);
    if(!rc){
      rc = sbsWriteMarker(p, " | ", "|");
      if(!rc){
        rc = sbsWriteLineno(p, lnRight, SBS_LNB);
        if(!rc){
          p->iEnd = nRight - nSuffix;
          sbsWriteText(p, pRight, SBS_TXTB);
        }
      }
    }
  }
  return rc;
}


/*
   Return the number between 0 and 100 that is smaller the closer pA and
   pB match.  Return 0 for a perfect match.  Return 100 if pA and pB are
   completely different.
  
   The current algorithm is as follows:
  
   (1) Remove leading and trailing whitespace.
   (2) Truncate both strings to at most 250 characters
   (3) Find the length of the longest common subsequence
   (4) Longer common subsequences yield lower scores.
*/
static int match_dline(DLine *pA, DLine *pB){
  const char *zA;            /* Left string */
  const char *zB;            /* right string */
  int nA;                    /* Bytes in zA[] */
  int nB;                    /* Bytes in zB[] */
  int avg;                   /* Average length of A and B */
  int i, j, k;               /* Loop counters */
  int best = 0;              /* Longest match found so far */
  int score;                 /* Final score.  0..100 */
  unsigned char c;           /* Character being examined */
  unsigned char aFirst[256]; /* aFirst[X] = index in zB[] of first char X */
  unsigned char aNext[252];  /* aNext[i] = index in zB[] of next zB[i] char */

  zA = pA->z;
  zB = pB->z;
  nA = pA->n;
  nB = pB->n;
  while( nA>0 && fsl_isspace(zA[0]) ){ nA--; zA++; }
  while( nA>0 && fsl_isspace(zA[nA-1]) ){ nA--; }
  while( nB>0 && fsl_isspace(zB[0]) ){ nB--; zB++; }
  while( nB>0 && fsl_isspace(zB[nB-1]) ){ nB--; }
  if( nA>250 ) nA = 250;
  if( nB>250 ) nB = 250;
  avg = (nA+nB)/2;
  if( avg==0 ) return 0;
  if( nA==nB && memcmp(zA, zB, nA)==0 ) return 0;
  memset(aFirst, 0, sizeof(aFirst));
  zA--; zB--;   /* Make both zA[] and zB[] 1-indexed */
  for(i=nB; i>0; i--){
    c = (unsigned char)zB[i];
    aNext[i] = aFirst[c];
    aFirst[c] = i;
  }
  best = 0;
  for(i=1; i<=nA-best; i++){
    c = (unsigned char)zA[i];
    for(j=aFirst[c]; j>0 && j<nB-best; j = aNext[j]){
      int limit = minInt(nA-i, nB-j);
      for(k=1; k<=limit && zA[k+i]==zB[k+j]; k++){}
      if( k>best ) best = k;
    }
  }
  score = (best>avg) ? 0 : (avg - best)*100/avg;

#if 0
  fprintf(stderr, "A: [%.*s]\nB: [%.*s]\nbest=%d avg=%d score=%d\n",
  nA, zA+1, nB, zB+1, best, avg, score);
#endif

  /* Return the result */
  return score;
}


/*
   There is a change block in which nLeft lines of text on the left are
   converted into nRight lines of text on the right.  This routine computes
   how the lines on the left line up with the lines on the right.
  
   The return value is a buffer of unsigned characters, obtained from
   fsl_malloc().  (The caller needs to free the return value using
   fsl_free().)  Entries in the returned array have values as follows:
  
      1.  Delete the next line of pLeft.
      2.  Insert the next line of pRight.
      3.  The next line of pLeft changes into the next line of pRight.
      4.  Delete one line from pLeft and add one line to pRight.
  
   Values larger than three indicate better matches.
  
   The length of the returned array will be just large enough to cause
   all elements of pLeft and pRight to be consumed.
  
   Algorithm:  Wagner's minimum edit-distance algorithm, modified by
   adding a cost to each match based on how well the two rows match
   each other.  Insertion and deletion costs are 50.  Match costs
   are between 0 and 100 where 0 is a perfect match 100 is a complete
   mismatch.
*/
static unsigned char *sbsAlignment(
   DLine *aLeft, int nLeft,       /* Text on the left */
   DLine *aRight, int nRight      /* Text on the right */
){
  int i, j, k;                 /* Loop counters */
  int *a;                      /* One row of the Wagner matrix */
  int *pToFree;                /* Space that needs to be freed */
  unsigned char *aM;           /* Wagner result matrix */
  int nMatch, iMatch;          /* Number of matching lines and match score */
  int mnLen;                   /* MIN(nLeft, nRight) */
  int mxLen;                   /* MAX(nLeft, nRight) */
  int aBuf[100];               /* Stack space for a[] if nRight not to big */

  aM = (unsigned char *)fsl_malloc( (nLeft+1)*(nRight+1) );
  if(!aM) return NULL;
  if( nLeft==0 ){
    memset(aM, 2, nRight);
    return aM;
  }
  if( nRight==0 ){
    memset(aM, 1, nLeft);
    return aM;
  }

  /* This algorithm is O(N**2).  So if N is too big, bail out with a
     simple (but stupid and ugly) result that doesn't take too long. */
  mnLen = nLeft<nRight ? nLeft : nRight;
  if( nLeft*nRight>100000 ){
    memset(aM, 4, mnLen);
    if( nLeft>mnLen )  memset(aM+mnLen, 1, nLeft-mnLen);
    if( nRight>mnLen ) memset(aM+mnLen, 2, nRight-mnLen);
    return aM;
  }

  if( nRight < (int)(sizeof(aBuf)/sizeof(aBuf[0]))-1 ){
    pToFree = 0;
    a = aBuf;
  }else{
    a = pToFree = fsl_malloc( sizeof(a[0])*(nRight+1) );
    if(!a){
      fsl_free(aM);
      return NULL;
    }
  }

  /* Compute the best alignment */
  for(i=0; i<=nRight; i++){
    aM[i] = 2;
    a[i] = i*50;
  }
  aM[0] = 0;
  for(j=1; j<=nLeft; j++){
    int p = a[0];
    a[0] = p+50;
    aM[j*(nRight+1)] = 1;
    for(i=1; i<=nRight; i++){
      int m = a[i-1]+50;
      int d = 2;
      if( m>a[i]+50 ){
        m = a[i]+50;
        d = 1;
      }
      if( m>p ){
        int score = match_dline(&aLeft[j-1], &aRight[i-1]);
        if( (score<=63 || (i<j+1 && i>j-1)) && m>p+score ){
          m = p+score;
          d = 3 | score*4;
        }
      }
      p = a[i];
      a[i] = m;
      aM[j*(nRight+1)+i] = d;
    }
  }

  /* Compute the lowest-cost path back through the matrix */
  i = nRight;
  j = nLeft;
  k = (nRight+1)*(nLeft+1)-1;
  nMatch = iMatch = 0;
  while( i+j>0 ){
    unsigned char c = aM[k];
    if( c>=3 ){
      assert( i>0 && j>0 );
      i--;
      j--;
      nMatch++;
      iMatch += (c>>2);
      aM[k] = 3;
    }else if( c==2 ){
      assert( i>0 );
      i--;
    }else{
      assert( j>0 );
      j--;
    }
    k--;
    aM[k] = aM[j*(nRight+1)+i];
  }
  k++;
  i = (nRight+1)*(nLeft+1) - k;
  memmove(aM, &aM[k], i);

  /* If:
       (1) the alignment is more than 25% longer than the longest side, and
       (2) the average match cost exceeds 15
     Then this is probably an alignment that will be difficult for humans
     to read.  So instead, just show all of the right side inserted followed
     by all of the left side deleted.
    
     The coefficients for conditions (1) and (2) above are determined by
     experimentation.
  */
  mxLen = nLeft>nRight ? nLeft : nRight;
  if( i*4>mxLen*5 && (nMatch==0 || iMatch/nMatch>15) ){
    memset(aM, 4, mnLen);
    if( nLeft>mnLen )  memset(aM+mnLen, 1, nLeft-mnLen);
    if( nRight>mnLen ) memset(aM+mnLen, 2, nRight-mnLen);
  }

  /* Return the result */
  fsl_free(pToFree);
  return aM;
}

/*
   R[] is an array of six integer, two COPY/DELETE/INSERT triples for a
   pair of adjacent differences.  Return true if the gap between these
   two differences is so small that they should be rendered as a single
   edit.
*/
static int smallGap(int *R){
  return R[3]<=2 || R[3]<=(R[1]+R[2]+R[4]+R[5])/8;
}

/*
   Given a diff context in which the aEdit[] array has been filled
   in, compute a side-by-side diff into pOut.
*/
static int sbsDiff(
  DContext *p,       /* The computed diff */
  DiffOutState *pOut,        /* Write the results here */
  ReCompiled *pRe,   /* Only show changes that match this regex */
  u64 diffFlags      /* Flags controlling the diff */
){
  DLine *A;     /* Left side of the diff */
  DLine *B;     /* Right side of the diff */
  int rc = 0;
  int a = 0;    /* Index of next line in A[] */
  int b = 0;    /* Index of next line in B[] */
  int *R;       /* Array of COPY/DELETE/INSERT triples */
  int r;        /* Index into R[] */
  int nr;       /* Number of COPY/DELETE/INSERT triples to process */
  int mxr;      /* Maximum value for r */
  int na, nb;   /* Number of lines shown from A and B */
  int i, j;     /* Loop counters */
  int m, ma, mb;/* Number of lines to output */
  int skip;     /* Number of lines to skip */
  static int nChunk = 0; /* Number of chunks of diff output seen so far */
  SbsLine s;    /* Output line buffer */
  int nContext; /* Lines of context above and below each change */
  int showDivider = 0;  /* True to show the divider */
  fsl_buffer unesc = fsl_buffer_empty;
  fsl_buffer aCols[5] = { /* Array of column blobs */
  fsl_buffer_empty_m, fsl_buffer_empty_m, fsl_buffer_empty_m,
  fsl_buffer_empty_m, fsl_buffer_empty_m
  }; 

  memset(&s, 0, sizeof(s));
  s.pOut = pOut;
  s.width = diff_width(diffFlags);
  nContext = diff_context_lines(diffFlags);
  s.escHtml = (diffFlags & DIFF_HTML)!=0;
  if( s.escHtml ){
    for(i=SBS_LNA; i<=SBS_TXTB; i++){
      s.apCols[i] = &aCols[i];
    }
  }else{
    for(i=SBS_LNA; i<=SBS_TXTB; i++){
      s.apCols[i] = &unesc;
    }
  }
  s.pRe = pRe;
  s.iStart = -1;
  s.iStart2 = 0;
  s.iEnd = -1;
  A = p->aFrom;
  B = p->aTo;
  R = p->aEdit;
  mxr = p->nEdit;
  while( mxr>2 && R[mxr-1]==0 && R[mxr-2]==0 ){ mxr -= 3; }

  for(r=0; r<mxr; r += 3*nr){
    /* Figure out how many triples to show in a single block */
    for(nr=1; R[r+nr*3]>0 && R[r+nr*3]<nContext*2; nr++){}
    /* printf("r=%d nr=%d\n", r, nr); */
#if 0
    /* MISSING: re/predicate bits. */
    /* If there is a regex, skip this block (generate no diff output)
       if the regex matches or does not match both insert and delete.
       Only display the block if one side matches but the other side does
       not.
    */
    if( pRe ){
      int hideBlock = 1;
      int xa = a, xb = b;
      for(i=0; hideBlock && i<nr; i++){
        int c1, c2;
        xa += R[r+i*3];
        xb += R[r+i*3];
        c1 = re_dline_match(pRe, &A[xa], R[r+i*3+1]);
        c2 = re_dline_match(pRe, &B[xb], R[r+i*3+2]);
        hideBlock = c1==c2;
        xa += R[r+i*3+1];
        xb += R[r+i*3+2];
      }
      if( hideBlock ){
        a = xa;
        b = xb;
        continue;
      }
    }
#endif
    /* For the current block comprising nr triples, figure out
       how many lines of A and B are to be displayed
    */
    if( R[r]>nContext ){
      na = nb = nContext;
      skip = R[r] - nContext;
    }else{
      na = nb = R[r];
      skip = 0;
    }
    for(i=0; i<nr; i++){
      na += R[r+i*3+1];
      nb += R[r+i*3+2];
    }
    if( R[r+nr*3]>nContext ){
      na += nContext;
      nb += nContext;
    }else{
      na += R[r+nr*3];
      nb += R[r+nr*3];
    }
    for(i=1; i<nr; i++){
      na += R[r+i*3];
      nb += R[r+i*3];
    }

    /* Draw the separator between blocks */
    if( showDivider ){
      if( s.escHtml ){
        char zLn[10];
        fsl_snprintf(zLn, sizeof(zLn), "%d", a+skip+1);
        rc = sbsWriteSep(&s, strlen(zLn), SBS_LNA);
        if(!rc) rc = sbsWriteSep(&s, s.width, SBS_TXTA);
        if(!rc) rc = sbsWriteSep(&s, 0, SBS_MKR);
        if(!rc){
          fsl_snprintf(zLn, sizeof(zLn), "%d", b+skip+1);
          rc = sbsWriteSep(&s, strlen(zLn), SBS_LNB);
          if(!rc) rc = sbsWriteSep(&s, s.width, SBS_TXTB);
        }
      }else{
        diff_outf(pOut, "%.*c\n", s.width*2+16, '.');
      }
      if(rc) goto end;
    }
    showDivider = 1;
    nChunk++;
    if( s.escHtml ){
      rc = fsl_buffer_appendf(s.apCols[SBS_LNA],
                              "<span class=\"fsl-diff-chunk-%d\"></span>",
                              nChunk);
    }

    /* Show the initial common area */
    a += skip;
    b += skip;
    m = R[r] - skip;
    for(j=0; !rc && j<m; j++){
      rc = sbsWriteLineno(&s, a+j, SBS_LNA);
      if(rc) break;
      s.iStart = s.iEnd = -1;
      rc = sbsWriteText(&s, &A[a+j], SBS_TXTA);
      if(!rc) rc = sbsWriteMarker(&s, "   ", "");
      if(!rc) rc = sbsWriteLineno(&s, b+j, SBS_LNB);
      if(!rc) rc = sbsWriteText(&s, &B[b+j], SBS_TXTB);
    }
    if(rc) goto end;
    a += m;
    b += m;

    /* Show the differences */
    for(i=0; i<nr; i++){
      unsigned char *alignment;
      ma = R[r+i*3+1];   /* Lines on left but not on right */
      mb = R[r+i*3+2];   /* Lines on right but not on left */

      /* If the gap between the current diff and then next diff within the
         same block is not too great, then render them as if they are a
         single diff. */
      while( i<nr-1 && smallGap(&R[r+i*3]) ){
        i++;
        m = R[r+i*3];
        ma += R[r+i*3+1] + m;
        mb += R[r+i*3+2] + m;
      }

      alignment = sbsAlignment(&A[a], ma, &B[b], mb);
      if(!alignment){
        rc = FSL_RC_OOM;
        goto end;
      }
      for(j=0; !rc && ma+mb>0; j++){
        if( alignment[j]==1 ){
          /* Delete one line from the left */
          rc = sbsWriteLineno(&s, a, SBS_LNA);
          if(rc) goto end_align;
          s.iStart = 0;
          s.zStart = "<span class=\"fsl-diff-rm\">";
          s.iEnd = LENGTH(&A[a]);
          rc = sbsWriteText(&s, &A[a], SBS_TXTA);
          if(!rc) rc = sbsWriteMarker(&s, " <", "&lt;");
          if(!rc) rc = sbsWriteNewlines(&s);
          if(rc) goto end_align;
          assert( ma>0 );
          ma--;
          a++;
        }else if( alignment[j]==3 ){
          /* The left line is changed into the right line */
          rc = sbsWriteLineChange(&s, &A[a], a, &B[b], b);
          if(rc) goto end_align;
          assert( ma>0 && mb>0 );
          ma--;
          mb--;
          a++;
          b++;
        }else if( alignment[j]==2 ){
          /* Insert one line on the right */
          if( !s.escHtml ){
            rc = sbsWriteSpace(&s, s.width + 7, SBS_TXTA);
          }
          if(!rc) rc = sbsWriteMarker(&s, " > ", "&gt;");
          if(!rc) rc = sbsWriteLineno(&s, b, SBS_LNB);
          if(rc) goto end_align;
          s.iStart = 0;
          s.zStart = "<span class=\"fsl-diff-add\">";
          s.iEnd = LENGTH(&B[b]);
          rc = sbsWriteText(&s, &B[b], SBS_TXTB);
          if(rc) goto end_align;
          assert( mb>0 );
          mb--;
          b++;
        }else{
          /* Delete from the left and insert on the right */
          rc = sbsWriteLineno(&s, a, SBS_LNA);
          if(rc) goto end_align;
          s.iStart = 0;
          s.zStart = "<span class=\"fsl-diff-rm\">";
          s.iEnd = LENGTH(&A[a]);
          rc = sbsWriteText(&s, &A[a], SBS_TXTA);
          if(!rc) rc = sbsWriteMarker(&s, " | ", "|");
          if(!rc) rc = sbsWriteLineno(&s, b, SBS_LNB);
          if(rc) goto end_align;
          s.iStart = 0;
          s.zStart = "<span class=\"fsl-diff-add\">";
          s.iEnd = LENGTH(&B[b]);
          rc = sbsWriteText(&s, &B[b], SBS_TXTB);
          if(rc) goto end_align;
          ma--;
          mb--;
          a++;
          b++;
        }
      }
      end_align:
      fsl_free(alignment);
      if(rc) goto end;
      if( i<nr-1 ){
        m = R[r+i*3+3];
        for(j=0; !rc && j<m; j++){
          rc = sbsWriteLineno(&s, a+j, SBS_LNA);
          s.iStart = s.iEnd = -1;
          if(!rc) rc = sbsWriteText(&s, &A[a+j], SBS_TXTA);
          if(!rc) rc = sbsWriteMarker(&s, "   ", "");
          if(!rc) rc = sbsWriteLineno(&s, b+j, SBS_LNB);
          if(!rc) rc = sbsWriteText(&s, &B[b+j], SBS_TXTB);
        }
        if(rc) goto end;
        b += m;
        a += m;
      }
    }

    /* Show the final common area */
    assert( nr==i );
    m = R[r+nr*3];
    if( m>nContext ) m = nContext;
    for(j=0; !rc && j<m; j++){
      rc = sbsWriteLineno(&s, a+j, SBS_LNA);
      s.iStart = s.iEnd = -1;
      if(!rc) rc = sbsWriteText(&s, &A[a+j], SBS_TXTA);
      if(!rc) rc = sbsWriteMarker(&s, "   ", "");
      if(!rc) rc = sbsWriteLineno(&s, b+j, SBS_LNB);
      if(!rc) rc = sbsWriteText(&s, &B[b+j], SBS_TXTB);
    }
    if(rc) goto end;
  } /* diff triplet loop */
  assert(!rc);
  
  if( s.escHtml && (s.apCols[SBS_LNA]->used>0) ){
    rc = diff_out(pOut, "<table class=\"fsl-sbsdiff-cols\"><tr>\n", -1);
    for(i=SBS_LNA; !rc && i<=SBS_TXTB; i++){
      rc = sbsWriteColumn(pOut, s.apCols[i], i);
    }
    rc = diff_out(pOut, "</tr></table>\n", -1);
  }else if(unesc.used){
    rc = pOut->out(pOut->oState, unesc.mem, unesc.used);
  }

  end:
  for( i = 0; i < (int)(sizeof(aCols)/sizeof(aCols[0])); ++i ){
    fsl_buffer_clear(&aCols[i]);
  }
  fsl_buffer_clear(&unesc);
  return rc;
}


/** @internal

    Performs a text diff on two buffers, either streaming the
    output to the 3rd argument or returning the results as an
    array of copy/delete/insert triples via the final argument.

    ONE of the 3rd or final arguments must be set and the other
    must be NULL

    If the 3rd argument is not NULL:

    - The 4th argument is the opaque state value passed to
    the 3rd when emitting output.
    
    - contextLines specifies the number of lines of context for the
    diff. A negative contextLines value uses a default.

    - sbsWidth, if not 0, specifies a side-by-side diff width. A
    negative sbsWidth uses a default. A 0 sbsWidth indicates a
    unified-style diff output.

    If the final argument is not NULL then the result array of
    copy/delete/insert triples is assigned to *outRaw. Ownership is
    transfered to the caller, who must eventually pass it to
    fsl_free().

    Returns 0 on success, any number of other codes on error.
*/
static int fsl_diff_text_impl(
  fsl_buffer const *pA,   /* FROM file */
  fsl_buffer const *pB,   /* TO file */
  fsl_output_f out, void * outState,
  /* ReCompiled *pRe, */ /* Only output changes where this Regexp matches */
  short contextLines,
  short sbsWidth,
  int diffFlags_,    /* FSL_DIFF_* flags */
  int ** outRaw
){
  int rc;
  DContext c;
  uint64_t diffFlags = fsl_diff_flags_convert(diffFlags_)
    | DIFF_CONTEXT_EX /* to shoehorn newer 0-handling semantics into
                         older (ported-in) code. */;
  if(!pA || !pB || (out && outRaw) || (!out && !outRaw)) return FSL_RC_MISUSE;
  else if(contextLines<0) contextLines = 5;
  else if(contextLines & ~LENGTH_MASK) contextLines = (int)LENGTH_MASK;
  diffFlags |= (LENGTH_MASK & contextLines);
  /* Encode SBS width... */
  if(sbsWidth<0
     || ((DIFF_SIDEBYSIDE & diffFlags) && !sbsWidth) ) sbsWidth = 80;
  if(sbsWidth) diffFlags |= DIFF_SIDEBYSIDE;
  diffFlags |= ((int)(sbsWidth & 0xFF))<<16;
  if( diffFlags & DIFF_INVERT ){
    fsl_buffer const *pTemp = pA;
    pA = pB;
    pB = pTemp;
  }

  /* Prepare the input files */
  memset(&c, 0, sizeof(c));
  if( (diffFlags & DIFF_IGNORE_ALLWS)==DIFF_IGNORE_ALLWS ){
    c.same_fn = same_dline_ignore_allws;
  }else{
    c.same_fn = same_dline;
  }
  rc = break_into_lines(fsl_buffer_cstr(pA), fsl_buffer_size(pA),
                        &c.nFrom, &c.aFrom, diffFlags);
  if(rc) goto end;
  rc = break_into_lines(fsl_buffer_cstr(pB), fsl_buffer_size(pB),
                        &c.nTo, &c.aTo, diffFlags);
  if(rc) goto end;

  if( c.aFrom==0 || c.aTo==0 ){
    /* Empty diff */
    goto end;
  }

  /* Compute the difference */
  rc = diff_all(&c);
  if(rc) goto end;
  if( (diffFlags & DIFF_NOTTOOBIG)!=0 ){
    int i, m, n;
    int *a = c.aEdit;
    int mx = c.nEdit;
    for(i=m=n=0; i<mx; i+=3){ m += a[i]; n += a[i+1]+a[i+2]; }
    if( !n || n>10000 ){
      rc = FSL_RC_RANGE;
      /* diff_errmsg(pOut, DIFF_TOO_MANY_CHANGES, diffFlags); */
      goto end;;
    }
  }
  if( (diffFlags & DIFF_NOOPT)==0 ){
    diff_optimize(&c);
  }

  if( out ){
    /* Compute a context or side-by-side diff */
    /* MISSING: regex support */
    DiffOutState dos = DiffOutState_empty;
    dos.out = out;
    dos.oState = outState;
    dos.ansiColor = !!(diffFlags_ & FSL_DIFF_ANSI_COLOR);
    if( diffFlags & DIFF_SIDEBYSIDE ){
      rc = sbsDiff(&c, &dos, NULL/*pRe*/, diffFlags);
    }else{
      rc = contextDiff(&c, &dos, NULL/*pRe*/, diffFlags);
    }
  }else if(outRaw){
    /* If a context diff is not requested, then return the
       array of COPY/DELETE/INSERT triples. */
    *outRaw = c.aEdit;
    c.aEdit = NULL;
  }
  end:
  fsl_free(c.aFrom);
  fsl_free(c.aTo);
  fsl_free(c.aEdit);
  return rc;
}

int fsl_diff_text_raw(fsl_buffer const *p1, fsl_buffer const *p2,
                      int diffFlags, int ** outRaw){
  return fsl_diff_text_impl(p1, p2, NULL, NULL, 0, 0, diffFlags, outRaw);
}

int fsl_diff_text(fsl_buffer const *pA, fsl_buffer const *pB,
                  fsl_output_f out, void * outState,
                  short contextLines,
                  short sbsWidth,
                  int diffFlags ){
  return fsl_diff_text_impl(pA, pB, out, outState,
                            contextLines, sbsWidth, diffFlags, NULL );
}



int fsl_diff_text_to_buffer(fsl_buffer const *pA, fsl_buffer const *pB,
                            fsl_buffer * pOut,
                            short contextLines,
                            short sbsWidth,
                            int diffFlags ){
  return (pA && pB && pOut)
    ? fsl_diff_text_impl(pA, pB, fsl_output_f_buffer, pOut,
                         contextLines, sbsWidth, diffFlags, NULL )
    : FSL_RC_MISUSE;
}

#undef TO_BE_STATIC
#undef LENGTH_MASK
#undef LENGTH_MASK_SZ
#undef LENGTH
#undef DIFF_CONTEXT_MASK
#undef DIFF_WIDTH_MASK
#undef DIFF_IGNORE_EOLWS
#undef DIFF_IGNORE_ALLWS
#undef DIFF_SIDEBYSIDE
#undef DIFF_VERBOSE
#undef DIFF_BRIEF
#undef DIFF_HTML
#undef DIFF_LINENO
#undef DIFF_NOOPT
#undef DIFF_INVERT
#undef DIFF_CONTEXT_EX
#undef DIFF_NOTTOOBIG
#undef DIFF_STRIP_EOLCR
#undef minInt
#undef SBS_LNA
#undef SBS_TXTA
#undef SBS_MKR
#undef SBS_LNB
#undef SBS_TXTB
#undef ANN_FILE_VERS
#undef ANN_FILE_ANCEST

#undef ANSI_COLOR_BLACK
#undef ANSI_COLOR_RED
#undef ANSI_COLOR_GREEN
#undef ANSI_COLOR_YELLOW
#undef ANSI_COLOR_BLUE
#undef ANSI_COLOR_MAGENTA
#undef ANSI_COLOR_CYAN
#undef ANSI_COLOR_WHITE
#undef ANSI_BG_BLACK
#undef ANSI_BG_RED
#undef ANSI_BG_GREEN
#undef ANSI_BG_YELLOW
#undef ANSI_BG_BLUE
#undef ANSI_BG_MAGENTA
#undef ANSI_BG_CYAN
#undef ANSI_BG_WHITE
#undef ANSI_RESET_COLOR
#undef ANSI_RESET_ALL
#undef ANSI_RESET
#undef ANSI_DIFF_ADD
#undef ANSI_DIFF_MOD
#undef ANSI_DIFF_RM
