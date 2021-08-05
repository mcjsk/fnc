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
/**************************************************************************
  This file houses some encoding/decoding API routines.
*/
#include <assert.h>

#include "fossil-scm/fossil.h"
/* Only for debugging */
#include <stdio.h>

/*
   An array for translating single base-16 characters into a value.
   Disallowed input characters have a value of 64.
*/
static const char zDecode[] = {
  64, 64, 64, 64, 64, 64, 64, 64,  64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64,  64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64,  64, 64, 64, 64, 64, 64, 64, 64,
   0,  1,  2,  3,  4,  5,  6,  7,   8,  9, 64, 64, 64, 64, 64, 64,
  64, 10, 11, 12, 13, 14, 15, 64,  64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64,  64, 64, 64, 64, 64, 64, 64, 64,
  64, 10, 11, 12, 13, 14, 15, 64,  64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64,  64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64,  64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64,  64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64,  64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64,  64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64,  64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64,  64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64,  64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64,  64, 64, 64, 64, 64, 64, 64, 64
};

int fsl_decode16(const unsigned char *zIn, unsigned char *pOut,
                 fsl_size_t N){
  fsl_int_t i, j;
  if( (N&1)!=0 ) return FSL_RC_RANGE;
  for(i=j=0; i<(fsl_int_t)N; i += 2, j++){
    fsl_int_t v1, v2, a;
    a = zIn[i];
    if( (a & 0x80)!=0 || (v1 = zDecode[a])==64 ) return FSL_RC_RANGE;
    a = zIn[i+1];
    if( (a & 0x80)!=0 || (v2 = zDecode[a])==64 ) return FSL_RC_RANGE;
    pOut[j] = (v1<<4) + v2;
  }
  return 0;
}


bool fsl_validate16(const char *zIn, fsl_size_t nIn){
  fsl_size_t i;
  for(i=0; i<nIn; i++, zIn++){
    if( zDecode[zIn[0]&0xff]>63 ){
      return zIn[0]==0 ? true : false;
    }
  }
  return true;
}

/*
   The array used for encoding
*/                           /* 123456789 12345  */
static const char zEncode[] = "0123456789abcdef"; 

int fsl_encode16(const unsigned char *pIn, unsigned char *zOut, fsl_size_t N){
  fsl_size_t i;
  if(!pIn || !zOut) return FSL_RC_MISUSE;
  for(i=0; i<N; i++){
    *(zOut++) = zEncode[pIn[i]>>4];
    *(zOut++) = zEncode[pIn[i]&0xf];
  }
  *zOut = 0;
  return 0;
}

void fsl_canonical16(char *z, fsl_size_t n){
  while( *z && n-- ){
    *z = zEncode[zDecode[(*z)&0x7f]&0x1f];
    ++z;
  }
}

void fsl_bytes_defossilize( unsigned char * z, fsl_size_t * resultLen ){
  fsl_size_t i, j, c;
  for(i=0; (c=z[i])!=0 && c!='\\'; i++){}
  if( c==0 ) {
    if(resultLen) *resultLen = i;
    return;
  }
  for(j=i; (c=z[i])!=0; i++){
    if( c=='\\' && z[i+1] ){
      i++;
      switch( z[i] ){
        case 'n':  c = '\n';  break;
        case 's':  c = ' ';   break;
        case 't':  c = '\t';  break;
        case 'r':  c = '\r';  break;
        case 'v':  c = '\v';  break;
        case 'f':  c = '\f';  break;
        case '0':  c = 0;     break;
        case '\\': c = '\\';  break;
        default:   c = z[i];  break;
      }
    }
    z[j++] = c;
  }
  if( z[j] ) z[j] = 0;
  if(resultLen) *resultLen = j;
}

int fsl_bytes_fossilize( unsigned char const * inp,
                         fsl_int_t nIn,
                         fsl_buffer * out ){
  fsl_size_t n, i, j, c;
  unsigned char *zOut;
  int rc;
  fsl_size_t oldUsed;
  fsl_size_t inSz;
  if(!inp || !out) return FSL_RC_MISUSE;
  else if( inp && (nIn<0) ) nIn = (fsl_int_t)fsl_strlen((char const *)inp);
  out->used = 0;
  if(!nIn) return 0;
  inSz = (fsl_size_t)nIn;
  /* Figure out how much space we'll need... */
  for(i=n=0; i<inSz; ++i){
    c = inp[i];
    if( c==0 || c==' ' || c=='\n' || c=='\t' || c=='\r' || c=='\f' || c=='\v'
        || c=='\\') ++n;
  }
  /* Reserve memory... */
  n += nIn;
  oldUsed = out->used;
  rc = fsl_buffer_reserve( out, oldUsed + (fsl_size_t)(n+1));
  if(rc) return rc;
  zOut = out->mem + oldUsed;
  /* Encode it... */
  for(i=j=0; i<(fsl_size_t)nIn; i++){
    unsigned char c = (unsigned char)inp[i];
    if( c==0 ){
      zOut[j++] = '\\';
      zOut[j++] = '0';
    }else if( c=='\\' ){
      zOut[j++] = '\\';
      zOut[j++] = '\\';
    }else if( fsl_isspace(c) ){
      zOut[j++] = '\\';
      switch( c ){
        case '\n':  c = 'n'; break;
        case ' ':   c = 's'; break;
        case '\t':  c = 't'; break;
        case '\r':  c = 'r'; break;
        case '\v':  c = 'v'; break;
        case '\f':  c = 'f'; break;
      }
      zOut[j++] = c;
    }else{
      zOut[j++] = c;
    }
  }
  zOut[j] = 0;
  out->used += j;
  return 0;
}


fsl_size_t fsl_str_to_size(char const * str){
  fsl_size_t size, oldsize, c;
  if(!str) return -1;
  for(oldsize=size=0; (c = str[0])>='0' && c<='9'; str++){
    size = oldsize*10 + c - '0';
    if( size<oldsize ) return -1;
    oldsize = size;
  }
  return size;
}

fsl_int_t fsl_str_to_int(char const * str, fsl_int_t dflt){
  fsl_size_t size, oldsize
    /* We use fsl_size_t for the calculation
       so that we can detect overflow (which is undefined
       for signed types).
    */;
  char c;
  fsl_int_t mult = 1;
  fsl_int_t rc;
  if(!str) return dflt;
  else switch(*str){
    case '+': ++str; break;
    case '-': ++str; mult = -1; break;
  };
  for(oldsize=size=0; (c = str[0])>='0' && c<='9'; str++){
    size = oldsize*10 + c - '0';
    if( size<oldsize ) /* overflow */ return dflt;
    oldsize = size;
  }
  rc = (fsl_int_t)size;
  return ((fsl_size_t)rc == size)
    ? (rc * mult)
    : dflt /* result is too big */;
}

fsl_size_t fsl_htmlize_xlate(int c, char const ** xlate){
  switch( c ){
    case '<': *xlate = "&lt;"; return 4;
    case '>': *xlate = "&gt;"; return 4;
    case '&': *xlate = "&amp;";  return 5;
    case '"': *xlate = "&quot;";  return 6;
    default: *xlate = NULL; return 1;
  }
}

int fsl_htmlize(fsl_output_f out, void * oState,
                const char *zIn, fsl_int_t n){
  int rc = 0;
  int c, i, j, len;
  char const * xlate;
  if(!out || !zIn) return FSL_RC_MISUSE;
  else if( n<0 ) n = fsl_strlen(zIn);
  for(i=j=0; !rc && (i<n); ++i){
    c = zIn[i];
    len = fsl_htmlize_xlate(c, &xlate);
    if(len>1){
      if( j<i ) rc = out(oState, zIn+j, i-j);
      if(!rc) rc = out(oState, xlate, len);
      j = i+1;
    }
  }
  if( !rc && j<i ) rc = out(oState, zIn+j, i-j);
  return rc;
}


int fsl_htmlize_to_buffer(fsl_buffer *p, const char *zIn, fsl_int_t n){
  int rc = 0;
  int c;
  fsl_int_t i = 0;
  fsl_size_t count = 0;
  char const * xl = NULL;
  if(!p || !zIn) return FSL_RC_MISUSE;
  else if( n<0 ) n = fsl_strlen(zIn);
  if(0==n) return 0;
  /* Count how many bytes we need, to avoid reallocs and the
     associated error checking... */
  for( ; i<n && (c = zIn[i])!=0; ++i ){
    count += fsl_htmlize_xlate(c, &xl);
  }
  if(count){
    rc = fsl_buffer_reserve(p, p->used + count + 1);
    if(!rc){
      /* Now none of the fsl_buffer_append()s can fail. */
      rc = fsl_htmlize(fsl_output_f_buffer, p, zIn, n);
    }
  }
  return rc;
}

char *fsl_htmlize_str(const char *zIn, fsl_int_t n){
  int rc;
  fsl_buffer b = fsl_buffer_empty;
  rc = fsl_htmlize_to_buffer(&b, zIn, n);
  if(!rc){
    return (char *)b.mem /* transfer ownership */;
  }else{
    fsl_buffer_clear(&b);
    return NULL;
  }
}

