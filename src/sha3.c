/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
/*
** Copyright (c) 2017 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)
**
** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
**
** This file contains an implementation of SHA3 (Keccak) hashing.
*/
/**
   This copy was modified slightly for use with the libfossil API.
*/
#include "fossil-scm/fossil.h"
#include "fossil-scm/fossil-hash.h"
#include <assert.h>
#include <string.h> /* strlen() */
#include <stddef.h> /* NULL on linux */
#include <sys/types.h>
#include <assert.h>

#if 0
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)
#endif

/*
** Macros to determine whether the machine is big or little endian,
** and whether or not that determination is run-time or compile-time.
**
** For best performance, an attempt is made to guess at the byte-order
** using C-preprocessor macros.  If that is unsuccessful, or if
** -DSHA3_BYTEORDER=0 is set, then byte-order is determined
** at run-time.
*/
#ifndef SHA3_BYTEORDER
# if defined(i386)     || defined(__i386__)   || defined(_M_IX86) ||    \
     defined(__x86_64) || defined(__x86_64__) || defined(_M_X64)  ||    \
     defined(_M_AMD64) || defined(_M_ARM)     || defined(__x86)   ||    \
     defined(__arm__)
#   define SHA3_BYTEORDER    1234
# elif defined(sparc)    || defined(__ppc__)
#   define SHA3_BYTEORDER    4321
# else
#   define SHA3_BYTEORDER 0
# endif
#endif

/*
** A single step of the Keccak mixing function for a 1600-bit state
*/
static void KeccakF1600Step(fsl_sha3_cx *p){
  int i;
  uint64_t B0, B1, B2, B3, B4;
  uint64_t C0, C1, C2, C3, C4;
  uint64_t D0, D1, D2, D3, D4;
  static const uint64_t RC[] = {
    0x0000000000000001ULL,  0x0000000000008082ULL,
    0x800000000000808aULL,  0x8000000080008000ULL,
    0x000000000000808bULL,  0x0000000080000001ULL,
    0x8000000080008081ULL,  0x8000000000008009ULL,
    0x000000000000008aULL,  0x0000000000000088ULL,
    0x0000000080008009ULL,  0x000000008000000aULL,
    0x000000008000808bULL,  0x800000000000008bULL,
    0x8000000000008089ULL,  0x8000000000008003ULL,
    0x8000000000008002ULL,  0x8000000000000080ULL,
    0x000000000000800aULL,  0x800000008000000aULL,
    0x8000000080008081ULL,  0x8000000000008080ULL,
    0x0000000080000001ULL,  0x8000000080008008ULL
  };
# define A00 (p->u.s[0])
# define A01 (p->u.s[1])
# define A02 (p->u.s[2])
# define A03 (p->u.s[3])
# define A04 (p->u.s[4])
# define A10 (p->u.s[5])
# define A11 (p->u.s[6])
# define A12 (p->u.s[7])
# define A13 (p->u.s[8])
# define A14 (p->u.s[9])
# define A20 (p->u.s[10])
# define A21 (p->u.s[11])
# define A22 (p->u.s[12])
# define A23 (p->u.s[13])
# define A24 (p->u.s[14])
# define A30 (p->u.s[15])
# define A31 (p->u.s[16])
# define A32 (p->u.s[17])
# define A33 (p->u.s[18])
# define A34 (p->u.s[19])
# define A40 (p->u.s[20])
# define A41 (p->u.s[21])
# define A42 (p->u.s[22])
# define A43 (p->u.s[23])
# define A44 (p->u.s[24])
# define ROL64(a,x) ((a<<x)|(a>>(64-x)))

  for(i=0; i<24; i+=4){
    C0 = A00^A10^A20^A30^A40;
    C1 = A01^A11^A21^A31^A41;
    C2 = A02^A12^A22^A32^A42;
    C3 = A03^A13^A23^A33^A43;
    C4 = A04^A14^A24^A34^A44;
    D0 = C4^ROL64(C1, 1);
    D1 = C0^ROL64(C2, 1);
    D2 = C1^ROL64(C3, 1);
    D3 = C2^ROL64(C4, 1);
    D4 = C3^ROL64(C0, 1);

    B0 = (A00^D0);
    B1 = ROL64((A11^D1), 44);
    B2 = ROL64((A22^D2), 43);
    B3 = ROL64((A33^D3), 21);
    B4 = ROL64((A44^D4), 14);
    A00 =   B0 ^((~B1)&  B2 );
    A00 ^= RC[i];
    A11 =   B1 ^((~B2)&  B3 );
    A22 =   B2 ^((~B3)&  B4 );
    A33 =   B3 ^((~B4)&  B0 );
    A44 =   B4 ^((~B0)&  B1 );

    B2 = ROL64((A20^D0), 3);
    B3 = ROL64((A31^D1), 45);
    B4 = ROL64((A42^D2), 61);
    B0 = ROL64((A03^D3), 28);
    B1 = ROL64((A14^D4), 20);
    A20 =   B0 ^((~B1)&  B2 );
    A31 =   B1 ^((~B2)&  B3 );
    A42 =   B2 ^((~B3)&  B4 );
    A03 =   B3 ^((~B4)&  B0 );
    A14 =   B4 ^((~B0)&  B1 );

    B4 = ROL64((A40^D0), 18);
    B0 = ROL64((A01^D1), 1);
    B1 = ROL64((A12^D2), 6);
    B2 = ROL64((A23^D3), 25);
    B3 = ROL64((A34^D4), 8);
    A40 =   B0 ^((~B1)&  B2 );
    A01 =   B1 ^((~B2)&  B3 );
    A12 =   B2 ^((~B3)&  B4 );
    A23 =   B3 ^((~B4)&  B0 );
    A34 =   B4 ^((~B0)&  B1 );

    B1 = ROL64((A10^D0), 36);
    B2 = ROL64((A21^D1), 10);
    B3 = ROL64((A32^D2), 15);
    B4 = ROL64((A43^D3), 56);
    B0 = ROL64((A04^D4), 27);
    A10 =   B0 ^((~B1)&  B2 );
    A21 =   B1 ^((~B2)&  B3 );
    A32 =   B2 ^((~B3)&  B4 );
    A43 =   B3 ^((~B4)&  B0 );
    A04 =   B4 ^((~B0)&  B1 );

    B3 = ROL64((A30^D0), 41);
    B4 = ROL64((A41^D1), 2);
    B0 = ROL64((A02^D2), 62);
    B1 = ROL64((A13^D3), 55);
    B2 = ROL64((A24^D4), 39);
    A30 =   B0 ^((~B1)&  B2 );
    A41 =   B1 ^((~B2)&  B3 );
    A02 =   B2 ^((~B3)&  B4 );
    A13 =   B3 ^((~B4)&  B0 );
    A24 =   B4 ^((~B0)&  B1 );

    C0 = A00^A20^A40^A10^A30;
    C1 = A11^A31^A01^A21^A41;
    C2 = A22^A42^A12^A32^A02;
    C3 = A33^A03^A23^A43^A13;
    C4 = A44^A14^A34^A04^A24;
    D0 = C4^ROL64(C1, 1);
    D1 = C0^ROL64(C2, 1);
    D2 = C1^ROL64(C3, 1);
    D3 = C2^ROL64(C4, 1);
    D4 = C3^ROL64(C0, 1);

    B0 = (A00^D0);
    B1 = ROL64((A31^D1), 44);
    B2 = ROL64((A12^D2), 43);
    B3 = ROL64((A43^D3), 21);
    B4 = ROL64((A24^D4), 14);
    A00 =   B0 ^((~B1)&  B2 );
    A00 ^= RC[i+1];
    A31 =   B1 ^((~B2)&  B3 );
    A12 =   B2 ^((~B3)&  B4 );
    A43 =   B3 ^((~B4)&  B0 );
    A24 =   B4 ^((~B0)&  B1 );

    B2 = ROL64((A40^D0), 3);
    B3 = ROL64((A21^D1), 45);
    B4 = ROL64((A02^D2), 61);
    B0 = ROL64((A33^D3), 28);
    B1 = ROL64((A14^D4), 20);
    A40 =   B0 ^((~B1)&  B2 );
    A21 =   B1 ^((~B2)&  B3 );
    A02 =   B2 ^((~B3)&  B4 );
    A33 =   B3 ^((~B4)&  B0 );
    A14 =   B4 ^((~B0)&  B1 );

    B4 = ROL64((A30^D0), 18);
    B0 = ROL64((A11^D1), 1);
    B1 = ROL64((A42^D2), 6);
    B2 = ROL64((A23^D3), 25);
    B3 = ROL64((A04^D4), 8);
    A30 =   B0 ^((~B1)&  B2 );
    A11 =   B1 ^((~B2)&  B3 );
    A42 =   B2 ^((~B3)&  B4 );
    A23 =   B3 ^((~B4)&  B0 );
    A04 =   B4 ^((~B0)&  B1 );

    B1 = ROL64((A20^D0), 36);
    B2 = ROL64((A01^D1), 10);
    B3 = ROL64((A32^D2), 15);
    B4 = ROL64((A13^D3), 56);
    B0 = ROL64((A44^D4), 27);
    A20 =   B0 ^((~B1)&  B2 );
    A01 =   B1 ^((~B2)&  B3 );
    A32 =   B2 ^((~B3)&  B4 );
    A13 =   B3 ^((~B4)&  B0 );
    A44 =   B4 ^((~B0)&  B1 );

    B3 = ROL64((A10^D0), 41);
    B4 = ROL64((A41^D1), 2);
    B0 = ROL64((A22^D2), 62);
    B1 = ROL64((A03^D3), 55);
    B2 = ROL64((A34^D4), 39);
    A10 =   B0 ^((~B1)&  B2 );
    A41 =   B1 ^((~B2)&  B3 );
    A22 =   B2 ^((~B3)&  B4 );
    A03 =   B3 ^((~B4)&  B0 );
    A34 =   B4 ^((~B0)&  B1 );

    C0 = A00^A40^A30^A20^A10;
    C1 = A31^A21^A11^A01^A41;
    C2 = A12^A02^A42^A32^A22;
    C3 = A43^A33^A23^A13^A03;
    C4 = A24^A14^A04^A44^A34;
    D0 = C4^ROL64(C1, 1);
    D1 = C0^ROL64(C2, 1);
    D2 = C1^ROL64(C3, 1);
    D3 = C2^ROL64(C4, 1);
    D4 = C3^ROL64(C0, 1);

    B0 = (A00^D0);
    B1 = ROL64((A21^D1), 44);
    B2 = ROL64((A42^D2), 43);
    B3 = ROL64((A13^D3), 21);
    B4 = ROL64((A34^D4), 14);
    A00 =   B0 ^((~B1)&  B2 );
    A00 ^= RC[i+2];
    A21 =   B1 ^((~B2)&  B3 );
    A42 =   B2 ^((~B3)&  B4 );
    A13 =   B3 ^((~B4)&  B0 );
    A34 =   B4 ^((~B0)&  B1 );

    B2 = ROL64((A30^D0), 3);
    B3 = ROL64((A01^D1), 45);
    B4 = ROL64((A22^D2), 61);
    B0 = ROL64((A43^D3), 28);
    B1 = ROL64((A14^D4), 20);
    A30 =   B0 ^((~B1)&  B2 );
    A01 =   B1 ^((~B2)&  B3 );
    A22 =   B2 ^((~B3)&  B4 );
    A43 =   B3 ^((~B4)&  B0 );
    A14 =   B4 ^((~B0)&  B1 );

    B4 = ROL64((A10^D0), 18);
    B0 = ROL64((A31^D1), 1);
    B1 = ROL64((A02^D2), 6);
    B2 = ROL64((A23^D3), 25);
    B3 = ROL64((A44^D4), 8);
    A10 =   B0 ^((~B1)&  B2 );
    A31 =   B1 ^((~B2)&  B3 );
    A02 =   B2 ^((~B3)&  B4 );
    A23 =   B3 ^((~B4)&  B0 );
    A44 =   B4 ^((~B0)&  B1 );

    B1 = ROL64((A40^D0), 36);
    B2 = ROL64((A11^D1), 10);
    B3 = ROL64((A32^D2), 15);
    B4 = ROL64((A03^D3), 56);
    B0 = ROL64((A24^D4), 27);
    A40 =   B0 ^((~B1)&  B2 );
    A11 =   B1 ^((~B2)&  B3 );
    A32 =   B2 ^((~B3)&  B4 );
    A03 =   B3 ^((~B4)&  B0 );
    A24 =   B4 ^((~B0)&  B1 );

    B3 = ROL64((A20^D0), 41);
    B4 = ROL64((A41^D1), 2);
    B0 = ROL64((A12^D2), 62);
    B1 = ROL64((A33^D3), 55);
    B2 = ROL64((A04^D4), 39);
    A20 =   B0 ^((~B1)&  B2 );
    A41 =   B1 ^((~B2)&  B3 );
    A12 =   B2 ^((~B3)&  B4 );
    A33 =   B3 ^((~B4)&  B0 );
    A04 =   B4 ^((~B0)&  B1 );

    C0 = A00^A30^A10^A40^A20;
    C1 = A21^A01^A31^A11^A41;
    C2 = A42^A22^A02^A32^A12;
    C3 = A13^A43^A23^A03^A33;
    C4 = A34^A14^A44^A24^A04;
    D0 = C4^ROL64(C1, 1);
    D1 = C0^ROL64(C2, 1);
    D2 = C1^ROL64(C3, 1);
    D3 = C2^ROL64(C4, 1);
    D4 = C3^ROL64(C0, 1);

    B0 = (A00^D0);
    B1 = ROL64((A01^D1), 44);
    B2 = ROL64((A02^D2), 43);
    B3 = ROL64((A03^D3), 21);
    B4 = ROL64((A04^D4), 14);
    A00 =   B0 ^((~B1)&  B2 );
    A00 ^= RC[i+3];
    A01 =   B1 ^((~B2)&  B3 );
    A02 =   B2 ^((~B3)&  B4 );
    A03 =   B3 ^((~B4)&  B0 );
    A04 =   B4 ^((~B0)&  B1 );

    B2 = ROL64((A10^D0), 3);
    B3 = ROL64((A11^D1), 45);
    B4 = ROL64((A12^D2), 61);
    B0 = ROL64((A13^D3), 28);
    B1 = ROL64((A14^D4), 20);
    A10 =   B0 ^((~B1)&  B2 );
    A11 =   B1 ^((~B2)&  B3 );
    A12 =   B2 ^((~B3)&  B4 );
    A13 =   B3 ^((~B4)&  B0 );
    A14 =   B4 ^((~B0)&  B1 );

    B4 = ROL64((A20^D0), 18);
    B0 = ROL64((A21^D1), 1);
    B1 = ROL64((A22^D2), 6);
    B2 = ROL64((A23^D3), 25);
    B3 = ROL64((A24^D4), 8);
    A20 =   B0 ^((~B1)&  B2 );
    A21 =   B1 ^((~B2)&  B3 );
    A22 =   B2 ^((~B3)&  B4 );
    A23 =   B3 ^((~B4)&  B0 );
    A24 =   B4 ^((~B0)&  B1 );

    B1 = ROL64((A30^D0), 36);
    B2 = ROL64((A31^D1), 10);
    B3 = ROL64((A32^D2), 15);
    B4 = ROL64((A33^D3), 56);
    B0 = ROL64((A34^D4), 27);
    A30 =   B0 ^((~B1)&  B2 );
    A31 =   B1 ^((~B2)&  B3 );
    A32 =   B2 ^((~B3)&  B4 );
    A33 =   B3 ^((~B4)&  B0 );
    A34 =   B4 ^((~B0)&  B1 );

    B3 = ROL64((A40^D0), 41);
    B4 = ROL64((A41^D1), 2);
    B0 = ROL64((A42^D2), 62);
    B1 = ROL64((A43^D3), 55);
    B2 = ROL64((A44^D4), 39);
    A40 =   B0 ^((~B1)&  B2 );
    A41 =   B1 ^((~B2)&  B3 );
    A42 =   B2 ^((~B3)&  B4 );
    A43 =   B3 ^((~B4)&  B0 );
    A44 =   B4 ^((~B0)&  B1 );
  }
# undef A00
# undef A01
# undef A02
# undef A03
# undef A04
# undef A10
# undef A11
# undef A12
# undef A13
# undef A14
# undef A20
# undef A21
# undef A22
# undef A23
# undef A24
# undef A30
# undef A31
# undef A32
# undef A33
# undef A34
# undef A40
# undef A41
# undef A42
# undef A43
# undef A44
# undef ROL64
}

enum fsl_sha3_hash_size fsl_sha3_hash_size_for_int(int n){
    switch(n){
      case 128: return FSL_SHA3_128; case 160: return FSL_SHA3_160;
      case 192: return FSL_SHA3_192; case 224: return FSL_SHA3_224;
      case 256: return FSL_SHA3_256; case 288: return FSL_SHA3_288;
      case 320: return FSL_SHA3_320; case 352: return FSL_SHA3_352;
      case 384: return FSL_SHA3_384; case 416: return FSL_SHA3_416;
      case 448: return FSL_SHA3_448; case 480: return FSL_SHA3_480;
      case 512: return FSL_SHA3_512;
      default: return FSL_SHA3_INVALID;
    }
}

void fsl_sha3_init(fsl_sha3_cx *cx){
  fsl_sha3_init2(cx, FSL_SHA3_DEFAULT);
}
void fsl_sha3_init2(fsl_sha3_cx *p, enum fsl_sha3_hash_size iSize){
  assert(iSize>0);
  memset(p, 0, sizeof(*p));
  p->size = iSize;
  if( iSize>=128 && iSize<=512 ){
    p->nRate = (1600 - ((iSize + 31)&~31)*2)/8;
  }else{
    p->nRate = (1600 - 2*256)/8;
  }
#if SHA3_BYTEORDER==1234
  /* Known to be little-endian at compile-time. No-op */
#elif SHA3_BYTEORDER==4321
  p->ixMask = 7;  /* Big-endian */
#else
  {
    static const unsigned int one = 1;
    if( 1==*(unsigned const char*)&one ){
      /* Little endian.  No byte swapping. */
      p->ixMask = 0;
    }else{
      /* Big endian.  Byte swap. */
      p->ixMask = 7;
    }
  }
#endif
}

void fsl_sha3_update( fsl_sha3_cx *p, void const *aData_, unsigned int nData ){
  unsigned char const * aData = (unsigned char const *)aData_;
  unsigned int i = 0;
#if SHA3_BYTEORDER==1234
  if( (p->nLoaded % 8)==0 && ((aData - (const unsigned char*)0)&7)==0 ){
    for(; i+7<nData; i+=8){
      p->u.s[p->nLoaded/8] ^= *(uint64_t*)&aData[i];
      p->nLoaded += 8;
      if( p->nLoaded>=p->nRate ){
        KeccakF1600Step(p);
        p->nLoaded = 0;
      }
    }
  }
#endif
  for(; i<nData; i++){
#if SHA3_BYTEORDER==1234
    p->u.x[p->nLoaded] ^= aData[i];
#elif SHA3_BYTEORDER==4321
    p->u.x[p->nLoaded^0x07] ^= aData[i];
#else
    p->u.x[p->nLoaded^p->ixMask] ^= aData[i];
#endif
    p->nLoaded++;
    if( p->nLoaded==p->nRate ){
      KeccakF1600Step(p);
      p->nLoaded = 0;
    }
  }
}

/*
** Convert a digest into base-16. must be at least nBytes long
** and zBuf must be at least nBytes*2 bytes long. This routine
** writes nBytes*2 hex-encoded bytes to zBuf, but does not write
** a terminating NUL byte.
*/
static void DigestToBase16(unsigned char *digest, unsigned char *zBuf, unsigned int nByte){
  static const unsigned char zEncode[] = "0123456789abcdef";
  unsigned int ix;
  for(ix=0; ix<nByte; ++ix){
    *zBuf++ = zEncode[(*digest>>4)&0xf];
    *zBuf++ = zEncode[*digest++ & 0xf];
  }
  /* *zBuf = '\0'; */
}

unsigned char const *fsl_sha3_end(fsl_sha3_cx *p){
  unsigned int i;
  if( p->nLoaded==p->nRate-1 ){
    const unsigned char c1 = 0x86;
    fsl_sha3_update(p, &c1, 1);
  }else{
    const unsigned char c2 = 0x06;
    const unsigned char c3 = 0x80;
    fsl_sha3_update(p, &c2, 1);
    p->nLoaded = p->nRate - 1;
    fsl_sha3_update(p, &c3, 1);
  }
  for(i=0; i<p->nRate; i++){
    p->u.x[i+p->nRate] = p->u.x[i^p->ixMask];
  }
  DigestToBase16( &p->u.x[p->nRate], p->hex, (int)p->size/8 );
  assert(0 == p->hex[(int)p->size/4+1]);
  return &p->u.x[p->nRate];
}

void fsl_sha3_digest_to_base16(unsigned char *digest, char *zBuf){
  static char const zEncode[] = "0123456789abcdef";
  int ix;
  for(ix=0; ix<FSL_STRLEN_K256/2; ix++){
    *zBuf++ = zEncode[(*digest>>4)&0xf];
    *zBuf++ = zEncode[*digest++ & 0xf];
  }
  *zBuf = '\0';
}


int fsl_sha3sum_stream(fsl_input_f src, void * srcState, fsl_buffer *pCksum){
  fsl_sha3_cx ctx;
  int rc;
  enum { BufSize = 1024 * 4 };
  unsigned char zBuf[BufSize];
  if(!src || !pCksum) return FSL_RC_MISUSE;
  fsl_sha3_init(&ctx);
  for(;;){
    fsl_size_t read = (fsl_size_t)BufSize;
    rc = src(srcState, zBuf, &read);
    if(rc) return rc;
    else if(read) fsl_sha3_update(&ctx, (unsigned char*)zBuf, read);
    if(read < (fsl_size_t)BufSize) break;
  }
  fsl_sha3_end(&ctx);
  fsl_buffer_reuse(pCksum);
  return fsl_buffer_append(pCksum, ctx.hex, fsl_strlen((const char *)ctx.hex));
}

int fsl_sha3sum_buffer(fsl_buffer const *pIn, fsl_buffer *pCksum){
  if(!pIn || !pCksum) return FSL_RC_MISUSE;
  else{
    fsl_sha3_cx ctx;
    int rc;
    fsl_sha3_init(&ctx);
    fsl_sha3_update(&ctx, pIn->mem, pIn->used);
    rc = fsl_buffer_reserve(pCksum, FSL_STRLEN_K256+1/*NUL*/);
    /*^^^^ DO NOT fsl_buffer_resize(), as pCksum is, more often than not,
      a cached reused buffer. */
    if(!rc){
      fsl_buffer_reuse(pCksum);
      fsl_sha3_end(&ctx);
      assert(fsl_strlen((char const*)ctx.hex)==FSL_STRLEN_K256);
      rc = fsl_buffer_append(pCksum, ctx.hex, fsl_strlen((char const*)ctx.hex));
      assert(!rc && "Cannot fail - pre-allocated");
      if(!rc){
        assert(FSL_STRLEN_K256==pCksum->used);
        assert(0==pCksum->mem[FSL_STRLEN_K256]);
      }
    }
    return rc;
  }
}

char *fsl_sha3sum_cstr(const char *zIn, fsl_int_t len){
  if(!zIn || !len) return NULL;
  else{
    fsl_sha3_cx ctx;
    fsl_sha3_init(&ctx);
    fsl_sha3_update(&ctx, zIn,
                    (len<0) ? fsl_strlen(zIn) : (fsl_size_t)len);
    fsl_sha3_end(&ctx);
    return fsl_strdup((char const *)ctx.hex);
  }
}

int fsl_sha3sum_filename(const char *zFilename, fsl_buffer *pCksum){
  if(!zFilename || !pCksum) return FSL_RC_MISUSE;
  else{
#if 1
    int rc;
    FILE *in = fsl_fopen(zFilename, "rb");
    if(!in) rc = FSL_RC_IO;
    else{
      rc = fsl_sha3sum_stream(fsl_input_f_FILE, in, pCksum);
      fsl_fclose(in);
    }
    return rc;
#else
    /* Requires v1 code which has not yet been ported in. */
    FILE *in;
    fsl_sha1_cx ctx;
    char zBuf[10240];
    int rc;

    if( fsl_wd_islink(zFilename) ){
      /* Instead of file content, return sha3 of link destination path */
      Blob destinationPath;
      
      blob_read_link(&destinationPath, zFilename);
      rc = fsl_sha3sum_buffer(&destinationPath, pCksum);
      fsl_buffer_clear(&destinationPath);
      return rc;
    }

    in = fossil_fopen(zFilename,"rb");
    if( in==0 ){
      return 1;
    }
    fsl_sha3_init(&ctx);
    for(;;){
      int n;
      n = fread(zBuf, 1, sizeof(zBuf), in);
      if( n<=0 ) break;
      fsl_sha3_update(&ctx, (unsigned char*)zBuf, (unsigned)n);
    }
    fclose_fclose(in);
    blob_zero(pCksum);
    blob_resize(pCksum, FSL_STRLEN_SHA3);
    fsl_sha3_end(&ctx);
    rc = fsl_buffer_append(pCksum, ctx.hex, fsl_strlen(ctx.hex));
    return rc;
#endif
  }
}


#undef SHA3_BYTEORDER
#undef MARKER
