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
  This file implements the generic i/o-related parts of the library.
*/
#include "fossil-scm/fossil-internal.h"
#include <assert.h>
#include <errno.h>
#include <string.h> /* memcmp() */

/* Only for debugging */
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)

/**
    fsl_appendf_f() impl which sends its output to fsl_output(). state
    must be a (fsl_cx*).
 */
static fsl_int_t fsl_appendf_f_fsl_output( void * state, char const * s,
                                           fsl_int_t n ){
  return fsl_output( (fsl_cx *)state, s, (fsl_size_t)n )
    ? -1
    : n;
}

int fsl_outputfv( fsl_cx * f, char const * fmt, va_list args ){
  if(!f || !fmt) return FSL_RC_MISUSE;
  else if(!*fmt) return FSL_RC_RANGE;
  else{
    long const prc = fsl_appendfv( fsl_appendf_f_fsl_output,
                                   f, fmt, args );
    return (prc>=0) ? 0 : FSL_RC_IO;
  }
}

    
int fsl_outputf( fsl_cx * f, char const * fmt, ... ){
  if(!f || !fmt) return FSL_RC_MISUSE;
  else if(!*fmt) return FSL_RC_RANGE;
  else{
    int rc;
    va_list args;
    va_start(args,fmt);
    rc = fsl_outputfv( f, fmt, args );
    va_end(args);
    return rc;
  }
}


int fsl_output( fsl_cx * cx, void const * src, fsl_size_t n ){
  if(!cx || !src) return FSL_RC_MISUSE;
  else if(!n || !cx->output.out) return 0;
  else return cx->output.out( cx->output.state.state,
                              src, n );
}

int fsl_flush( fsl_cx * f ){
  return f
    ? (f->output.flush
       ? f->output.flush(f->output.state.state)
       : 0)
    : FSL_RC_MISUSE;
}


int fsl_flush_f_FILE(void * _FILE){
  return _FILE
    ? (fflush((FILE*)_FILE) ? fsl_errno_to_rc(errno, FSL_RC_IO) : 0)
    : FSL_RC_MISUSE;
}

int fsl_output_f_FILE( void * state,
                       void const * src, fsl_size_t n ){
  if(!state || !src) return FSL_RC_MISUSE;
  else if(!n) return 0;
  else return (1 == fwrite(src, n, 1, state ? (FILE*)state : stdout))
         ? 0
         : FSL_RC_IO;
}

int fsl_input_f_FILE( void * state, void * dest, fsl_size_t * n ){
  FILE * f = (FILE*) state;
  if( !state || !dest || !n ) return FSL_RC_MISUSE;
  else if( !*n ) return FSL_RC_RANGE;
  *n = (fsl_size_t)fread( dest, 1, *n, f );
  return *n
    ? 0
    : (feof(f) ? 0 : FSL_RC_IO);
}

void fsl_finalizer_f_FILE( void * state, void * mem ){
  if(mem){
    fsl_fclose((FILE*)mem);
  }
}

int fsl_stream( fsl_input_f inF, void * inState,
                fsl_output_f outF, void * outState ){
  if(!inF || !outF) return FSL_RC_MISUSE;
  else{
    int rc = 0;
    enum { BufSize = 1024 * 4 };
    unsigned char buf[BufSize];
    fsl_size_t rn = BufSize;
    for( ; !rc &&
           (rn==BufSize)
           && (0==(rc=inF(inState, buf, &rn)));
         rn = BufSize){
      if(rn) rc = outF(outState, buf, rn);
      else break;
    }
    return rc;
  }
}

int fsl_stream_compare( fsl_input_f in1, void * in1State,
                        fsl_input_f in2, void * in2State ){
  enum { BufSize = 1024 * 2 };
  unsigned char buf1[BufSize];
  unsigned char buf2[BufSize];
  fsl_size_t rn1 = BufSize;
  fsl_size_t rn2 = BufSize;
  int rc;
  while(1){
    rc = in1(in1State, buf1, &rn1);
    if(rc) return -1;
    rc = in2(in2State, buf2, &rn2);
    if(rc) return 1;
    else if(rn1!=rn2){
      rc = (rn1<rn2) ? -1 : 1;
      break;
    }
    else if(0==rn1 && 0==rn2) return 0;
    rc = memcmp( buf1, buf2, rn1 );
    if(rc) break;
    rn1 = rn2 = BufSize;
  }
  return rc;
}

#undef MARKER
