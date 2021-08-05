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
  This file houses Fossil's delta generation and application
  routines. This code is functionally independent of the rest of the
  library, relying only on fsl_malloc(), fsl_free(), and the integer
  typedefs defined by the configuration process. i.e. it can easily
  be pulled out and used in arbitrary projects.
*/
#include "fossil-scm/fossil.h"
#include <memory.h>
#include <stdlib.h>

/**
   2021-03-10: The delta checksum self-test is a significant run-time
   sink when processing many deltas. Fossil does not enable this
   feature by default so we'll leave it off by default, too.
*/
#if !defined(FSL_OMIT_DELTA_CKSUM_TEST)
#  define FSL_OMIT_DELTA_CKSUM_TEST
#endif

/*
   Macros for turning debugging printfs on and off
*/
#if 0
# define DEBUG1(X) X
#else
# define DEBUG1(X)
#endif
#if 0
#define DEBUG2(X) X
/*
   For debugging:
   Print 16 characters of text from zBuf
*/
static const char *print16(const char *z){
  int i;
  static char zBuf[20];
  for(i=0; i<16; i++){
    if( z[i]>=0x20 && z[i]<=0x7e ){
      zBuf[i] = z[i];
    }else{
      zBuf[i] = '.';
    }
  }
  zBuf[i] = 0;
  return zBuf;
}
#else
# define DEBUG2(X)
#endif

/*
   The width of a hash window in bytes.  The algorithm only works if this
   is a power of 2.
*/
#define NHASH 16

/*
   The current state of the rolling hash.
  
   z[] holds the values that have been hashed.  z[] is a circular buffer.
   z[i] is the first entry and z[(i+NHASH-1)%NHASH] is the last entry of 
   the window.
  
   Hash.a is the sum of all elements of hash.z[].  Hash.b is a weighted
   sum.  Hash.b is z[i]*NHASH + z[i+1]*(NHASH-1) + ... + z[i+NHASH-1]*1.
   (Each index for z[] should be module NHASH, of course.  The %NHASH operator
   is omitted in the prior expression for brevity.)
*/
typedef struct fsl_delta_hash fsl_delta_hash;
struct fsl_delta_hash {
  uint16_t a, b;         /* Hash values */
  uint16_t i;            /* Start of the hash window */
  unsigned char z[NHASH];    /* The values that have been hashed */
};

/*
   Initialize the rolling hash using the first NHASH characters of z[]
*/
static void fsl_delta_hash_init(fsl_delta_hash *pHash,
                                unsigned char const *z){
  uint16_t a, b, i;
  a = b = 0;
  for(i=0; i<NHASH; i++){
    a += z[i];
    b += a;
  }
  memcpy(pHash->z, z, NHASH);
  pHash->a = a & 0xffff;
  pHash->b = b & 0xffff;
  pHash->i = 0;
}

/*
   Advance the rolling hash by a single character "c"
*/
static void fsl_delta_hash_next(fsl_delta_hash *pHash, int c){
  uint16_t old = pHash->z[pHash->i];
  pHash->z[pHash->i] = c;
  pHash->i = (pHash->i+1)&(NHASH-1);
  pHash->a = pHash->a - old + c;
  pHash->b = pHash->b - NHASH*old + pHash->a;
}

/*
   Return a 32-bit hash value
*/
static uint32_t fsl_delta_hash_32bit(fsl_delta_hash *pHash){
  return (pHash->a & 0xffff) | (((uint32_t)(pHash->b & 0xffff))<<16);
}

/**
   Compute a hash on NHASH bytes.

   This routine is intended to be equivalent to:
   fsl_delta_hash h;
   fsl_delta_hash_init(&h, zInput);
   return fsl_delta_hash_32bit(&h);
*/
static uint32_t fsl_delta_hash_once(unsigned const char *z){
  uint16_t a = 0, b = 0, i = 0;
  for(i=0; i<NHASH; ++i){
    a += z[i];
    b += a;
  }
  return a | (((uint32_t)b)<<16);
}

/*
   Write an base-64 integer into the given buffer. Return its length.
*/
static unsigned int fsl_delta_int_put(uint32_t v, unsigned char **pz){
  static const char zDigits[] = 
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~";
  /*  123456789 123456789 123456789 123456789 123456789 123456789 123 */
  int i, j;
  unsigned char zBuf[20];
  fsl_size_t rc = 0;
  if( v==0 ){
    *(*pz)++ = '0';
    rc = 1;
  }else{
    for(i=0; v>0; ++rc, ++i, v>>=6){
      zBuf[i] = zDigits[v&0x3f];
    }
    zBuf[i]=0;
    for(j=i-1; j>=0; j--){
      *(*pz)++ = zBuf[j];
    }
  }
  return rc;
}

/*
   Read bytes from *pz and convert them into a positive integer.  When
   finished, leave *pz pointing to the first character past the end of
   the integer.  The *pLen parameter holds the length of the string
   in *pz and is decremented once for each character in the integer.
*/
static fsl_size_t fsl_delta_int_get(unsigned char const **pz, fsl_int_t *pLen){
  static const signed char zValue[] = {
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,    8,  9, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, 16,   17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32,   33, 34, 35, -1, -1, -1, -1, 36,
    -1, 37, 38, 39, 40, 41, 42, 43,   44, 45, 46, 47, 48, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 58, 59,   60, 61, 62, -1, -1, -1, 63, -1,
  };
  fsl_size_t v = 0;
  fsl_int_t c;
  unsigned char const *z = (unsigned char const*)*pz;
  unsigned char const *zStart = z;
  while( (c = zValue[0x7f&*(z++)])>=0 ){
     v = (v<<6) + c;
  }
  z--;
  *pLen -= z - zStart;
  *pz = z;
  return v;
}

/*
   Return the number digits in the base-64 representation of a positive integer
*/
static int fsl_delta_digit_count(fsl_int_t v){
  unsigned int x;
  int i;
  for(i=1, x=64; v>=(fsl_int_t)x; i++, x <<= 6){}
  return i;
}

/*
   Compute a 32-bit checksum on the N-byte buffer.  Return the result.
*/
static unsigned int fsl_delta_checksum(void const *zIn, fsl_size_t N){
  const unsigned char *z = (const unsigned char *)zIn;
  unsigned sum0 = 0;
  unsigned sum1 = 0;
  unsigned sum2 = 0;
  unsigned sum3 = 0;
  while(N >= 16){
    sum0 += ((unsigned)z[0] + z[4] + z[8] + z[12]);
    sum1 += ((unsigned)z[1] + z[5] + z[9] + z[13]);
    sum2 += ((unsigned)z[2] + z[6] + z[10]+ z[14]);
    sum3 += ((unsigned)z[3] + z[7] + z[11]+ z[15]);
    z += 16;
    N -= 16;
  }
  while(N >= 4){
    sum0 += z[0];
    sum1 += z[1];
    sum2 += z[2];
    sum3 += z[3];
    z += 4;
    N -= 4;
  }
  sum3 += (sum2 << 8) + (sum1 << 16) + (sum0 << 24);
  switch(N){
    case 3:   sum3 += (z[2] << 8);
    case 2:   sum3 += (z[1] << 16);
    case 1:   sum3 += (z[0] << 24);
    default:  ;
  }
  return sum3;
}

int fsl_delta_create2( unsigned char const *zSrc, fsl_size_t lenSrc,
                       unsigned char const *zOut, fsl_size_t lenOut,
                       fsl_output_f out, void * outState){
  enum { IntegerBufSize = 50 /* buffer size for integer conversions. */};
  unsigned int i, base;
  unsigned int nHash;          /* Number of hash table entries */
  unsigned int *landmark;      /* Primary hash table */
  unsigned int *collide = NULL;  /* Collision chain */
  int lastRead = -1;           /* Last byte of zSrc read by a COPY command */
  int rc;                      /* generic return code checker. */
  unsigned int olen = 0;       /* current output length. */
  unsigned int total = 0;      /* total byte count. */
  fsl_delta_hash h;
  unsigned char theBuf[IntegerBufSize] = {0,};
  unsigned char * intBuf = theBuf;
  if(!zSrc || !zOut || !out) return FSL_RC_MISUSE;
  /* Add the target file size to the beginning of the delta
  */
#ifdef OUT
#undef OUT
#endif
#define OUT(BLOB,LEN) rc=out(outState, BLOB, LEN); if(0 != rc) {fsl_free(collide); return rc;} else total += LEN
#define OUTCH(CHAR) OUT(CHAR,1)
#define PINT(I) intBuf = theBuf; olen=fsl_delta_int_put(I, &intBuf); OUT(theBuf,olen)
  PINT(lenOut);
  OUTCH("\n");

  /* If the source file is very small, it means that we have no
     chance of ever doing a copy command.  Just output a single
     literal segment for the entire target and exit.
  */
  if( lenSrc<=NHASH ){
    PINT(lenOut);
    OUTCH(":");
    OUT(zOut,lenOut);
    PINT((fsl_delta_checksum(zOut, lenOut)));
    OUTCH(";");
    return 0;
  }

  /* Compute the hash table used to locate matching sections in the
     source file.
  */
  nHash = lenSrc/NHASH;
  collide = (unsigned int *)malloc( nHash*2*sizeof(int) );
  if(!collide){
    return FSL_RC_OOM;
  }
  landmark = &collide[nHash];
  memset(landmark, -1, nHash*sizeof(int));
  memset(collide, -1, nHash*sizeof(int));
  for(i=0; i<lenSrc-NHASH; i+=NHASH){
    uint32_t const hv = fsl_delta_hash_once(&zSrc[i]) % nHash;
    collide[i/NHASH] = landmark[hv];
    landmark[hv] = i/NHASH;
  }

  /* Begin scanning the target file and generating copy commands and
     literal sections of the delta.
  */
  base = 0;    /* We have already generated everything before zOut[base] */
  while( base+NHASH<lenOut ){
    fsl_int_t iSrc;
    int iBlock
      /* WEIRD: if i change this from int to fsl_int_t
         we end up in an infinite loop somewhere. int
         and short both work*/;
    fsl_int_t bestCnt, bestOfst=0, bestLitsz=0;
    fsl_delta_hash_init(&h, &zOut[base]);
    i = 0;     /* Trying to match a landmark against zOut[base+i] */
    bestCnt = 0;
    while( 1 ){
      uint32_t hv;
      int limit = 250;

      hv = fsl_delta_hash_32bit(&h) % nHash;
      DEBUG2( printf("LOOKING: %4d [%s]\n", base+i, print16(&zOut[base+i])); )
      iBlock = (int)landmark[hv];
      while( iBlock>=0 && (limit--)>0 ){
        /*
           The hash window has identified a potential match against 
           landmark block iBlock.  But we need to investigate further.
           
           Look for a region in zOut that matches zSrc. Anchor the search
           at zSrc[iSrc] and zOut[base+i].  Do not include anything prior to
           zOut[base] or after zOut[outLen] nor anything after zSrc[srcLen].
          
           Set cnt equal to the length of the match and set ofst so that
           zSrc[ofst] is the first element of the match.  litsz is the number
           of characters between zOut[base] and the beginning of the match.
           sz will be the overhead (in bytes) needed to encode the copy
           command.  Only generate copy command if the overhead of the
           copy command is less than the amount of literal text to be copied.
        */
        fsl_int_t cnt, ofst, litsz;
        fsl_int_t j, k, x, y;
        fsl_int_t sz;
        fsl_int_t limitX;

        /* Beginning at iSrc, match forwards as far as we can.  j counts
           the number of characters that match */
        iSrc = iBlock*NHASH;
        y = base + i;
        limitX = ( lenSrc-iSrc <= lenOut-y ) ? lenSrc : iSrc + lenOut - y;
        for(x=iSrc; x<limitX; ++x, ++y){
          if( zSrc[x]!=zOut[y] ) break;
        }
        j = x - iSrc - 1;

        /* Beginning at iSrc-1, match backwards as far as we can.  k counts
           the number of characters that match */
        for(k=1; k<iSrc && k<=i; ++k){
          if( zSrc[iSrc-k]!=zOut[base+i-k] ) break;
        }
        --k;

        /* Compute the offset and size of the matching region */
        ofst = iSrc-k;
        cnt = j+k+1;
        litsz = i-k;  /* Number of bytes of literal text before the copy */
        DEBUG2( printf("MATCH %d bytes at %d: [%s] litsz=%d\n",
                        cnt, ofst, print16(&zSrc[ofst]), litsz); )
        /* sz will hold the number of bytes needed to encode the "insert"
           command and the copy command, not counting the "insert" text */
        sz = fsl_delta_digit_count(i-k)
          +fsl_delta_digit_count(cnt)
          +fsl_delta_digit_count(ofst)
          +3;
        if( cnt>=sz && cnt>bestCnt ){
          /* Remember this match only if it is the best so far and it
             does not increase the file size */
          bestCnt = cnt;
          bestOfst = iSrc-k;
          bestLitsz = litsz;
          DEBUG2( printf("... BEST SO FAR\n"); )
        }

        /* Check the next matching block */
        iBlock = collide[iBlock];
      }

      /* We have a copy command that does not cause the delta to be larger
         than a literal insert.  So add the copy command to the delta.
      */
      if( bestCnt>0 ){
        if( bestLitsz>0 ){
          /* Add an insert command before the copy */
          PINT(bestLitsz);
          OUTCH(":");
          OUT(zOut+base, bestLitsz);
          base += bestLitsz;
          DEBUG2( printf("insert %d\n", bestLitsz); )
        }
        base += bestCnt;
        PINT(bestCnt);
        OUTCH("@");
        PINT(bestOfst);
        DEBUG2( printf("copy %d bytes from %d\n", bestCnt, bestOfst); )
        OUTCH(",");
        if( bestOfst + bestCnt -1 > lastRead ){
          lastRead = bestOfst + bestCnt - 1;
          DEBUG2( printf("lastRead becomes %d\n", lastRead); )
        }
        bestCnt = 0;
        break;
      }

      /* If we reach this point, it means no match is found so far */
      if( base+i+NHASH>=lenOut ){
        /* We have reached the end of the input and have not found any
           matches.  Do an "insert" for everything that does not match */
        PINT(lenOut-base);
        OUTCH(":");
        OUT(zOut+base, lenOut-base);
        base = lenOut;
        break;
      }

      /* Advance the hash by one character.  Keep looking for a match */
      fsl_delta_hash_next(&h, zOut[base+i+NHASH]);
      i++;
    }
  }
  fsl_free(collide);
  /* Output a final "insert" record to get all the text at the end of
     the file that does not match anything in the source file.
  */
  if( base<lenOut ){
    PINT(lenOut-base);
    OUTCH(":");
    OUT(zOut+base, lenOut-base);
  }
  /* Output the final checksum record. */
  PINT(fsl_delta_checksum(zOut, lenOut));
  OUTCH(";");
  return 0;
#undef PINT
#undef OUT
#undef OUTCH
}

struct DeltaOutputString {
  unsigned char * mem;
  unsigned int cursor;
};
typedef struct DeltaOutputString DeltaOutputString;

/** fsl_output_f() impl which requires state to be a
    (DeltaOutputString*). Copies the first n bytes of src to
    state->mem, increments state->cursor by n, and returns 0.
 */
static int fsl_output_f_ostring( void * state, void const * src,
                                 fsl_size_t n ){
  DeltaOutputString * os = (DeltaOutputString*)state;
  memcpy( os->mem + os->cursor, src, n );
  os->cursor += n;
  return 0;
}

int fsl_delta_create( unsigned char const *zSrc, fsl_size_t lenSrc,
                      unsigned char const *zOut, fsl_size_t lenOut,
                      unsigned char *zDelta, fsl_size_t * deltaSize){
  int rc;
  DeltaOutputString os;
  if(!zSrc || !zOut || !zDelta || !deltaSize) return FSL_RC_MISUSE;
  os.mem = (unsigned char *)zDelta;
  os.cursor = 0;
  rc = fsl_delta_create2( zSrc, lenSrc, zOut, lenOut,
                          fsl_output_f_ostring, &os );
  if(!rc){
    os.mem[os.cursor] = 0;
    if(deltaSize) *deltaSize = os.cursor;
  }
  return rc;
}

/*
   Calculates the size (in bytes) of the output from applying a
   delta. On success 0 is returned and *deltaSize will be updated with
   the amount of memory required for applying the delta.
  
   This routine is provided so that an procedure that is able
   to call fsl_delta_apply() can learn how much space is required
   for the output and hence allocate nor more space that is really
   needed.
*/
int fsl_delta_applied_size(unsigned char const *zDelta, fsl_size_t lenDelta_,
                           fsl_size_t * deltaSize){
  if(!zDelta || (lenDelta_<2) || !deltaSize) return FSL_RC_MISUSE;
  else{
    fsl_size_t size;
    fsl_int_t lenDelta = (fsl_int_t)lenDelta_;
    size = fsl_delta_int_get(&zDelta, &lenDelta);
    if( *zDelta!='\n' ){
      /* ERROR: size integer not terminated by "\n" */
      return FSL_RC_DELTA_INVALID_TERMINATOR;
    }
    *deltaSize = size;
    return 0;
  }
}


int fsl_delta_apply2(
  unsigned char const *zSrc,      /* The source or pattern file */
  fsl_size_t lenSrc_,            /* Length of the source file */
  unsigned char const *zDelta,    /* Delta to apply to the pattern */
  fsl_size_t lenDelta_,          /* Length of the delta */
  unsigned char *zOut,             /* Write the output into this preallocated buffer */
  fsl_error * pErr
){
  fsl_size_t limit;
  fsl_size_t total = 0;
#if !defined(FSL_OMIT_DELTA_CKSUM_TEST)
  unsigned char *zOrigOut = zOut;
#endif
  /* lenSrc/lenDelta are cast to ints to avoid any potential side-effects
     caused by changing the function signature from signed to unsigned
     int types when porting from v1.
  */
  fsl_int_t lenSrc = (fsl_int_t)lenSrc_;
  fsl_int_t lenDelta = (fsl_int_t)lenDelta_;
  if(!zSrc || !zDelta || !zOut) return FSL_RC_MISUSE;
  else if(lenSrc<0 || lenDelta<0) return FSL_RC_RANGE;
  limit = fsl_delta_int_get(&zDelta, &lenDelta);
  if( *zDelta!='\n' ){
    if(pErr){
      fsl_error_set(pErr,
                    FSL_RC_DELTA_INVALID_TERMINATOR,
                    "Delta: size integer not terminated by \\n");
    }
    return FSL_RC_DELTA_INVALID_TERMINATOR;
  }
  zDelta++; lenDelta--;
  while( *zDelta && lenDelta>0 ){
    fsl_int_t cnt, ofst;
    cnt = fsl_delta_int_get(&zDelta, &lenDelta);
    switch( zDelta[0] ){
      case '@': {
        zDelta++; lenDelta--;
        ofst = fsl_delta_int_get(&zDelta, &lenDelta);
        if( lenDelta>0 && zDelta[0]!=',' ){
          /* ERROR: copy command not terminated by ',' */
          if(pErr){
            fsl_error_set(pErr,
                          FSL_RC_DELTA_INVALID_TERMINATOR,
                          "Delta: copy command not terminated by ','");
          }
          return FSL_RC_DELTA_INVALID_TERMINATOR;
        }
        zDelta++; lenDelta--;
        DEBUG1( printf("COPY %d from %d\n", cnt, ofst); )
        total += cnt;
        if( total>limit ){
          if(pErr){
            fsl_error_set(pErr, FSL_RC_RANGE,
                          "Delta: copy exceeds output file size");
          }
          return FSL_RC_RANGE;
        }
        if( ofst+cnt > lenSrc ){
          if(pErr){
            fsl_error_set(pErr, FSL_RC_RANGE,
                          "Delta: copy extends past end of input");
          }
          return FSL_RC_RANGE;
        }
        memcpy(zOut, &zSrc[ofst], cnt);
        zOut += cnt;
        break;
      }
      case ':': {
        zDelta++; lenDelta--;
        total += cnt;
        if( total>limit ){
          if(pErr){
            fsl_error_set(pErr, FSL_RC_RANGE,
                          "Delta: insert command gives an output "
                          "larger than predicted");
          }
          return FSL_RC_RANGE;
        }
        DEBUG1( printf("INSERT %d\n", cnt); )
        if( cnt>lenDelta ){
          if(pErr){
            fsl_error_set(pErr, FSL_RC_RANGE,
                          "Delta: insert count exceeds size of delta");
          }
          return FSL_RC_RANGE;
        }
        memcpy(zOut, zDelta, cnt);
        zOut += cnt;
        zDelta += cnt;
        lenDelta -= cnt;
        break;
      }
      case ';': {
        zDelta++; lenDelta--;
        zOut[0] = 0;
#if !defined(FSL_OMIT_DELTA_CKSUM_TEST)
        if( cnt!=fsl_delta_checksum(zOrigOut, total) ){
          if(pErr){
            fsl_error_set(pErr, FSL_RC_CHECKSUM_MISMATCH,
                          "Delta: bad checksum");
          }
          return FSL_RC_CHECKSUM_MISMATCH;
        }
#endif
        if( total!=limit ){
          if(pErr){
            fsl_error_set(pErr, FSL_RC_SIZE_MISMATCH,
                          "Delta: generated size does not match "
                          "predicted size");
          }
          return FSL_RC_SIZE_MISMATCH;
        }
        return 0;
      }
      default: {
        if(pErr){
          fsl_error_set(pErr, FSL_RC_DELTA_INVALID_OPERATOR,
                        "Delta: unknown delta operator");
        }
        return FSL_RC_DELTA_INVALID_OPERATOR;
      }
    }
  }
  /* ERROR: unterminated delta */
  if(pErr){
    fsl_error_set(pErr, FSL_RC_DELTA_INVALID_TERMINATOR,
                  "Delta: unterminated delta");
  }
  return FSL_RC_DELTA_INVALID_TERMINATOR;
}
int fsl_delta_apply(
  unsigned char const *zSrc,      /* The source or pattern file */
  fsl_size_t lenSrc_,            /* Length of the source file */
  unsigned char const *zDelta,    /* Delta to apply to the pattern */
  fsl_size_t lenDelta_,          /* Length of the delta */
  unsigned char *zOut             /* Write the output into this preallocated buffer */
){
  return fsl_delta_apply2(zSrc, lenSrc_, zDelta, lenDelta_, zOut, NULL);
}

#undef NHASH
#undef DEBUG1
#undef DEBUG2
