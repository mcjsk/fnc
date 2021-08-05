/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/*
   The code is modified for use in fossil (then libfossil).  The
   original header comment follows:
*/
/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */
#include <string.h>
#include <stdio.h>
#include "fossil-scm/fossil.h"
#include <errno.h>

#if defined(__i386__) || defined(__x86_64__) || defined(_WIN32)
# define byteReverse(A,B)
#else
/*
 * Convert an array of integers to little-endian.
 * Note: this code is a no-op on little-endian machines.
 */
static void byteReverse (unsigned char *buf, unsigned longs){
    uint32_t t;
    do {
        t = (uint32_t)((unsigned)buf[3]<<8 | buf[2]) << 16 |
            ((unsigned)buf[1]<<8 | buf[0]);
        *(uint32_t *)buf = t;
        buf += 4;
    } while (--longs);
}
#endif

/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f, w, x, y, z, data, s)                         \
    ( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
static void MD5Transform(uint32_t buf[4], const uint32_t in[16]){
    register uint32_t a, b, c, d;

    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];

    MD5STEP(F1, a, b, c, d, in[ 0]+0xd76aa478,  7);
    MD5STEP(F1, d, a, b, c, in[ 1]+0xe8c7b756, 12);
    MD5STEP(F1, c, d, a, b, in[ 2]+0x242070db, 17);
    MD5STEP(F1, b, c, d, a, in[ 3]+0xc1bdceee, 22);
    MD5STEP(F1, a, b, c, d, in[ 4]+0xf57c0faf,  7);
    MD5STEP(F1, d, a, b, c, in[ 5]+0x4787c62a, 12);
    MD5STEP(F1, c, d, a, b, in[ 6]+0xa8304613, 17);
    MD5STEP(F1, b, c, d, a, in[ 7]+0xfd469501, 22);
    MD5STEP(F1, a, b, c, d, in[ 8]+0x698098d8,  7);
    MD5STEP(F1, d, a, b, c, in[ 9]+0x8b44f7af, 12);
    MD5STEP(F1, c, d, a, b, in[10]+0xffff5bb1, 17);
    MD5STEP(F1, b, c, d, a, in[11]+0x895cd7be, 22);
    MD5STEP(F1, a, b, c, d, in[12]+0x6b901122,  7);
    MD5STEP(F1, d, a, b, c, in[13]+0xfd987193, 12);
    MD5STEP(F1, c, d, a, b, in[14]+0xa679438e, 17);
    MD5STEP(F1, b, c, d, a, in[15]+0x49b40821, 22);

    MD5STEP(F2, a, b, c, d, in[ 1]+0xf61e2562,  5);
    MD5STEP(F2, d, a, b, c, in[ 6]+0xc040b340,  9);
    MD5STEP(F2, c, d, a, b, in[11]+0x265e5a51, 14);
    MD5STEP(F2, b, c, d, a, in[ 0]+0xe9b6c7aa, 20);
    MD5STEP(F2, a, b, c, d, in[ 5]+0xd62f105d,  5);
    MD5STEP(F2, d, a, b, c, in[10]+0x02441453,  9);
    MD5STEP(F2, c, d, a, b, in[15]+0xd8a1e681, 14);
    MD5STEP(F2, b, c, d, a, in[ 4]+0xe7d3fbc8, 20);
    MD5STEP(F2, a, b, c, d, in[ 9]+0x21e1cde6,  5);
    MD5STEP(F2, d, a, b, c, in[14]+0xc33707d6,  9);
    MD5STEP(F2, c, d, a, b, in[ 3]+0xf4d50d87, 14);
    MD5STEP(F2, b, c, d, a, in[ 8]+0x455a14ed, 20);
    MD5STEP(F2, a, b, c, d, in[13]+0xa9e3e905,  5);
    MD5STEP(F2, d, a, b, c, in[ 2]+0xfcefa3f8,  9);
    MD5STEP(F2, c, d, a, b, in[ 7]+0x676f02d9, 14);
    MD5STEP(F2, b, c, d, a, in[12]+0x8d2a4c8a, 20);

    MD5STEP(F3, a, b, c, d, in[ 5]+0xfffa3942,  4);
    MD5STEP(F3, d, a, b, c, in[ 8]+0x8771f681, 11);
    MD5STEP(F3, c, d, a, b, in[11]+0x6d9d6122, 16);
    MD5STEP(F3, b, c, d, a, in[14]+0xfde5380c, 23);
    MD5STEP(F3, a, b, c, d, in[ 1]+0xa4beea44,  4);
    MD5STEP(F3, d, a, b, c, in[ 4]+0x4bdecfa9, 11);
    MD5STEP(F3, c, d, a, b, in[ 7]+0xf6bb4b60, 16);
    MD5STEP(F3, b, c, d, a, in[10]+0xbebfbc70, 23);
    MD5STEP(F3, a, b, c, d, in[13]+0x289b7ec6,  4);
    MD5STEP(F3, d, a, b, c, in[ 0]+0xeaa127fa, 11);
    MD5STEP(F3, c, d, a, b, in[ 3]+0xd4ef3085, 16);
    MD5STEP(F3, b, c, d, a, in[ 6]+0x04881d05, 23);
    MD5STEP(F3, a, b, c, d, in[ 9]+0xd9d4d039,  4);
    MD5STEP(F3, d, a, b, c, in[12]+0xe6db99e5, 11);
    MD5STEP(F3, c, d, a, b, in[15]+0x1fa27cf8, 16);
    MD5STEP(F3, b, c, d, a, in[ 2]+0xc4ac5665, 23);

    MD5STEP(F4, a, b, c, d, in[ 0]+0xf4292244,  6);
    MD5STEP(F4, d, a, b, c, in[ 7]+0x432aff97, 10);
    MD5STEP(F4, c, d, a, b, in[14]+0xab9423a7, 15);
    MD5STEP(F4, b, c, d, a, in[ 5]+0xfc93a039, 21);
    MD5STEP(F4, a, b, c, d, in[12]+0x655b59c3,  6);
    MD5STEP(F4, d, a, b, c, in[ 3]+0x8f0ccc92, 10);
    MD5STEP(F4, c, d, a, b, in[10]+0xffeff47d, 15);
    MD5STEP(F4, b, c, d, a, in[ 1]+0x85845dd1, 21);
    MD5STEP(F4, a, b, c, d, in[ 8]+0x6fa87e4f,  6);
    MD5STEP(F4, d, a, b, c, in[15]+0xfe2ce6e0, 10);
    MD5STEP(F4, c, d, a, b, in[ 6]+0xa3014314, 15);
    MD5STEP(F4, b, c, d, a, in[13]+0x4e0811a1, 21);
    MD5STEP(F4, a, b, c, d, in[ 4]+0xf7537e82,  6);
    MD5STEP(F4, d, a, b, c, in[11]+0xbd3af235, 10);
    MD5STEP(F4, c, d, a, b, in[ 2]+0x2ad7d2bb, 15);
    MD5STEP(F4, b, c, d, a, in[ 9]+0xeb86d391, 21);

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

const fsl_md5_cx fsl_md5_cx_empty = fsl_md5_cx_empty_m;
/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void fsl_md5_init(fsl_md5_cx *ctx){
    *ctx = fsl_md5_cx_empty;
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void fsl_md5_update(fsl_md5_cx *ctx, void const * buf_, fsl_size_t len){
    const unsigned char * buf = (const unsigned char *)buf_;
    uint32_t t;
    
    /* Update bitcount */
    
    t = ctx->bits[0];
    if ((ctx->bits[0] = t + ((uint32_t)len << 3)) < t)
        ctx->bits[1]++; /* Carry from low to high */
    ctx->bits[1] += len >> 29;
    
    t = (t >> 3) & 0x3f;    /* Bytes already in shsInfo->data */

    /* Handle any leading odd-sized chunks */

    if ( t ) {
        unsigned char *p = (unsigned char *)ctx->in + t;

        t = 64-t;
        if (len < t) {
            memcpy(p, buf, len);
            return;
        }
        memcpy(p, buf, t);
        byteReverse(ctx->in, 16);
        MD5Transform(ctx->buf, (uint32_t *)ctx->in);
        buf += t;
        len -= t;
    }

    /* Process data in 64-byte chunks */

    while (len >= 64) {
        memcpy(ctx->in, buf, 64);
        byteReverse(ctx->in, 16);
        MD5Transform(ctx->buf, (uint32_t *)ctx->in);
        buf += 64;
        len -= 64;
    }

    /* Handle any remaining bytes of data. */

    memcpy(ctx->in, buf, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern 
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void fsl_md5_final(fsl_md5_cx * ctx, unsigned char digest[16]){
    unsigned count;
    unsigned char *p;

    /* Compute number of bytes mod 64 */
    count = (ctx->bits[0] >> 3) & 0x3F;

    /* Set the first char of padding to 0x80.  This is safe since there is
       always at least one byte free */
    p = ctx->in + count;
    *p++ = 0x80;

    /* Bytes of padding needed to make 64 bytes */
    count = 64 - 1 - count;

    /* Pad out to 56 mod 64 */
    if (count < 8) {
        /* Two lots of padding:  Pad the first block to 64 bytes */
        memset(p, 0, count);
        byteReverse(ctx->in, 16);
        MD5Transform(ctx->buf, (uint32_t *)ctx->in);

        /* Now fill the next block with 56 bytes */
        memset(ctx->in, 0, 56);
    } else {
        /* Pad block to 56 bytes */
        memset(p, 0, count-8);
    }
    byteReverse(ctx->in, 14);

    /* Append length in bits and transform */
    memcpy(&ctx->in[14*sizeof(uint32_t)], ctx->bits, 2*sizeof(uint32_t));

    MD5Transform(ctx->buf, (uint32_t *)ctx->in);
    byteReverse((unsigned char *)ctx->buf, 4);
    memcpy(digest, ctx->buf, 16);
    memset(ctx, 0, sizeof(*ctx));    /* In case it's sensitive */
}

void fsl_md5_digest_to_base16(unsigned char *digest, char *zBuf){
    static char const zEncode[] = "0123456789abcdef";
    int i, j;

    for(j=i=0; i<16; i++){
        int a = digest[i];
        zBuf[j++] = zEncode[(a>>4)&0xf];
        zBuf[j++] = zEncode[a & 0xf];
    }
    zBuf[j] = 0;
}

#if 0
/* The symlink-hashing code here may be needed at some point... */
int fsl_md5sum_file(const char *zFilename, fsl_buffer *pCksum){
  if(!zFilename || !pCksum) return FSL_RC_MISUSE;
  else{
    /* Requires v1 code which has not yet been ported in. */
    FILE *in;
    fsl_md5_cx ctx;
    unsigned char zResult[20];
    char zBuf[10240];

    if( fsl_wd_islink(zFilename) ){
      /* Instead of file content, return md5 of link destination path */
      Blob destinationPath;
      int rc;
      
      blob_read_link(&destinationPath, zFilename);
      rc = sha1sum_blob(&destinationPath, pCksum);
      blob_reset(&destinationPath);
      return rc;
    }

    in = fossil_fopen(zFilename,"rb");
    if( in==0 ){
      return 1;
    }
    fsl_sha1_init(&ctx);
    for(;;){
      int n;
      n = fread(zBuf, 1, sizeof(zBuf), in);
      if( n<=0 ) break;
      fsl_sha1_update(&ctx, (unsigned char*)zBuf, (unsigned)n);
    }
    fsl_fclose(in);
    blob_zero(pCksum);
    blob_resize(pCksum, 40);
    fsl_sha1_final(&ctx, zResult);
    fsl_sha1_digest_to_base16(zResult, blob_buffer(pCksum));
    return 0;
  }
}
#endif

int fsl_md5sum_buffer(fsl_buffer const *pIn, fsl_buffer *pCksum){
  if(!pIn || !pCksum) return FSL_RC_MISUSE;
  else{
    fsl_md5_cx ctx = fsl_md5_cx_empty;
    unsigned char zResult[20];
    int rc;
    fsl_md5_update(&ctx, pIn->mem, pIn->used);
    fsl_buffer_reuse(pCksum);
    rc = fsl_buffer_resize(pCksum, FSL_STRLEN_MD5/*resize() adds 1 for NUL*/);
    if(!rc){
      fsl_md5_final(&ctx, zResult);
      fsl_md5_digest_to_base16(zResult, fsl_buffer_str(pCksum));
    }
    return rc;
  }
}

char *fsl_md5sum_cstr(const char *zIn, fsl_int_t len){
  if(!zIn || !len) return NULL;
  else{
    fsl_md5_cx ctx;
    unsigned char zResult[20];
    char * zDigest = (char *)fsl_malloc(FSL_STRLEN_MD5+1);
    if(!zDigest) return NULL;
    fsl_md5_init(&ctx);
    fsl_md5_update(&ctx, zIn,
                    (len<0) ? fsl_strlen(zIn) : (fsl_size_t)len);
    fsl_md5_final(&ctx, zResult);
    fsl_md5_digest_to_base16(zResult, zDigest);
    return zDigest;
  }
}

int fsl_md5sum_stream(fsl_input_f src, void * srcState, fsl_buffer *pCksum){
    fsl_md5_cx ctx;
    int rc;
    unsigned char zResult[20];
    enum { BufSize = 1024 * 4 };
    unsigned char zBuf[BufSize];
    if(!src || !pCksum) return FSL_RC_MISUSE;
    fsl_md5_init(&ctx);
    for(;;){
      fsl_size_t read = (fsl_size_t)BufSize;
      rc = src(srcState, zBuf, &read);
      if(rc) return rc;
      else if(read) fsl_md5_update(&ctx, (unsigned char*)zBuf, read);
      if(read < (fsl_size_t)BufSize) break;
    }
    fsl_buffer_reuse(pCksum);
    rc = fsl_buffer_resize(pCksum, FSL_STRLEN_MD5);
    if(!rc){
      fsl_md5_final(&ctx, zResult);
      fsl_md5_digest_to_base16(zResult, fsl_buffer_str(pCksum));
    }
    return rc;
}



#if 0
void fsl_md5_to_base16(fsl_md5_cx const * cx, char *zBuf){
    static char const zEncode[] = "0123456789abcdef";
    int i, j;

    for(j=i=0; i<16; i++){
        int a = digest[i];
        zBuf[j++] = zEncode[(a>>4)&0xf];
        zBuf[j++] = zEncode[a & 0xf];
    }
    zBuf[j] = 0;
}
#endif


#if 0
/*
   Add the content of a blob to the incremental MD5 checksum.
*/
void md5sum_step_blob(Blob *p){
    md5sum_step_text(blob_buffer(p), blob_size(p));
}
#endif

int fsl_md5sum_filename(const char *zFilename, fsl_buffer *pCksum){
  if(!zFilename || !pCksum) return FSL_RC_MISUSE;
  else{
    int rc;
    FILE *in = fsl_fopen(zFilename, "rb");
    if(!in) rc = FSL_RC_IO;
    else{
      rc = fsl_md5sum_stream(fsl_input_f_FILE, in, pCksum);
      fsl_fclose(in);
    }
    return rc;
  }
}

void fsl_md5_update_buffer(fsl_md5_cx *cx, fsl_buffer const * b){
  if(b->used) fsl_md5_update(cx, b->mem, b->used);
}

void fsl_md5_update_cstr(fsl_md5_cx *cx, char const * str, fsl_int_t len){
  if(len<0) len = fsl_strlen(str);
  if(len>0) fsl_md5_update(cx, str, (fsl_size_t)len);
}

int fsl_md5_update_stream(fsl_md5_cx *ctx,
                          fsl_input_f src, void * srcState){
    int rc;
    enum { BufSize = 1024 * 4 };
    unsigned char zBuf[BufSize];
    if(!ctx || !src) return FSL_RC_MISUSE;
    for(;;){
      fsl_size_t read = (fsl_size_t)BufSize;
      rc = src(srcState, zBuf, &read);
      if(rc) return rc;
      else if(read) fsl_md5_update(ctx, (unsigned char*)zBuf, read);
      if(read < (fsl_size_t)BufSize) break;
    }
    return 0;
}

int fsl_md5_update_filename(fsl_md5_cx *cx, char const * fname){
  if(!cx || !fname) return FSL_RC_MISUSE;
  else{
    int rc;
    FILE *in = fsl_fopen(fname, "rb");
    if(in) rc = fsl_errno_to_rc(errno,FSL_RC_IO);
    else {
      rc = fsl_md5_update_stream(cx, fsl_input_f_FILE, in);
      fsl_fclose(in);
    }
    return rc;
  }
}


#undef F1
#undef F2
#undef F3
#undef F4
#undef MD5STEP
#undef byteReverse
