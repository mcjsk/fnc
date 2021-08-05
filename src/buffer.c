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
#include <string.h> /* strlen() */
#include <stddef.h> /* NULL on linux */
#include <errno.h>

#include <zlib.h>


#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)


fsl_buffer * fsl_buffer_reuse( fsl_buffer * b ){
  if(b->capacity){
    assert(b->mem);
    b->mem[0] = 0;
  }
  b->used = b->cursor = 0;
  return b;
}

void fsl_buffer_clear( fsl_buffer * buf ){
  if(buf){
    if(buf->mem) fsl_free(buf->mem);
    *buf = fsl_buffer_empty;
  }
}

int fsl_buffer_reserve( fsl_buffer * buf, fsl_size_t n ){
  if( ! buf ) return FSL_RC_MISUSE;
  else if( 0 == n ){
    fsl_free(buf->mem);
    *buf = fsl_buffer_empty;
    return 0;
  }else if( buf->capacity >= n ){
    return 0;
  }else{
    unsigned char * x;
    assert((buf->used < n) && "Buffer in-use greater than capacity!");
    x = (unsigned char *)fsl_realloc( buf->mem, n );
    if( ! x ) return FSL_RC_OOM;
    memset( x + buf->used, 0, n - buf->used );
    buf->mem = x;
    buf->capacity = n;
    return 0;
  }
}

int fsl_buffer_resize( fsl_buffer * buf, fsl_size_t n ){
  if( !buf ) return FSL_RC_MISUSE;
  else if(n && (buf->capacity == n+1)){
    buf->used = n;
    buf->mem[n] = 0;
    return 0;
  }else{
    unsigned char * x = (unsigned char *)fsl_realloc( buf->mem,
                                                      n+1/*NUL*/ );
    if( ! x ) return FSL_RC_OOM;
    if(n > buf->capacity){
      /* zero-fill new parts */
      memset( x + buf->capacity, 0, n - buf->capacity +1/*NUL*/ );
    }
    buf->capacity = n + 1 /*NUL*/;
    buf->used = n;
    buf->mem = x;
    buf->mem[buf->used] = 0;
    return 0;
  }
}

int fsl_buffer_compare(fsl_buffer const * lhs, fsl_buffer const * rhs){
  fsl_size_t const szL = lhs->used;
  fsl_size_t const szR = rhs->used;
  fsl_size_t const sz = (szL<szR) ? szL : szR;
  int rc = memcmp(lhs->mem, rhs->mem, sz);
  if(0 == rc){
    rc = (szL==szR)
      ? 0
      : ((szL<szR) ? -1 : 1);
  }
  return rc;
}

/*
   Compare two blobs in constant time and return zero if they are equal.
   Constant time comparison only applies for blobs of the same length.
   If lengths are different, immediately returns 1.
*/
int fsl_buffer_compare_O1(fsl_buffer const * lhs, fsl_buffer const * rhs){
  fsl_size_t const szL = lhs->used;
  fsl_size_t const szR = rhs->used;
  fsl_size_t i;
  unsigned char const *buf1;
  unsigned char const *buf2;
  unsigned char rc = 0;
  if( szL!=szR || szL==0 ) return 1;
  buf1 = lhs->mem;
  buf2 = rhs->mem;
  for( i=0; i<szL; i++ ){
    rc = rc | (buf1[i] ^ buf2[i]);
  }
  return rc;
}


int fsl_buffer_append( fsl_buffer * b,
                       void const * data,
                       fsl_int_t len ){
  if(!b || !data) return FSL_RC_MISUSE;
  else{
    fsl_size_t sz = b->used;
    int rc = 0;
    assert(b->capacity ? !!b->mem : !b->mem);
    assert(b->used <= b->capacity);
    if(len<0){
      len = (fsl_int_t)fsl_strlen((char const *)data);
    }
    sz += len + 1/*NUL*/;
    rc = fsl_buffer_reserve( b, sz );
    if(!rc){
      assert(b->capacity >= sz);
      if(len>0) memcpy(b->mem + b->used, data, (size_t)len);
      b->used += len;
      b->mem[b->used] = 0;
    }
    return rc;
  }
}

/*
   Internal helper for implementing fsl_buffer_appendf()
*/
typedef struct BufferAppender {
  fsl_buffer * b;
  /*
     Result code of the appending process.
  */
  int rc;
} BufferAppender;

/*
   fsl_appendf_f() impl which requires arg to be a (fsl_buffer*).
   It appends the data to arg. Returns the number of bytes written
   on success, a negative value on error. Always NUL-terminates the
   buffer on success.
*/
static fsl_int_t fsl_appendf_f_buffer( void * arg,
                                       char const * data, fsl_int_t n ){
  BufferAppender * ba = (BufferAppender*)arg;
  fsl_buffer * sb = ba->b;
  if( !sb || (n<0) ) return -1;
  else if( ! n ) return 0;
  else{
    fsl_int_t rc;
    fsl_size_t npos = sb->used + n;
    if( npos >= sb->capacity ){
      const size_t asz = npos ? ((4 * npos / 3) + 1) : 16;
      if( asz < npos ) {
        ba->rc = FSL_RC_RANGE;
        return -1; /* overflow */
      }
      else{
        rc = fsl_buffer_reserve( sb, asz );
        if(rc) {
          ba->rc = FSL_RC_OOM;
          return -1;
        }
      }
    }
    rc = 0;
    for( ; rc < n; ++rc, ++sb->used ){
      sb->mem[sb->used] = data[rc];
    }
    sb->mem[sb->used] = 0;
    return rc;
  }
}

int fsl_buffer_appendfv( fsl_buffer * b,
                         char const * fmt, va_list args){
  if(!b || !fmt) return FSL_RC_MISUSE;
  else{
    BufferAppender ba;
    ba.b = b;
    ba.rc = 0;
    fsl_appendfv( fsl_appendf_f_buffer, &ba, fmt, args );
    return ba.rc;
  }
}


int fsl_buffer_appendf( fsl_buffer * b,
                        char const * fmt, ... ){
  if(!b || !fmt) return FSL_RC_MISUSE;
  else{
    int rc;
    va_list args;
    va_start(args,fmt);
    rc = fsl_buffer_appendfv( b, fmt, args );
    va_end(args);
    return rc;
  }
}

char const * fsl_buffer_cstr(fsl_buffer const *b){
  return b ? (char const *)b->mem : NULL;
}

char const * fsl_buffer_cstr2(fsl_buffer const *b, fsl_size_t * len){
  char const * rc = NULL;
  if(b){
    rc = (char const *)b->mem;
    if(len) *len = b->used;
  }
  return rc;
}

char * fsl_buffer_str(fsl_buffer const *b){
  return b ? (char *)b->mem : NULL;
}


fsl_size_t fsl_buffer_size(fsl_buffer const * b){
  return b ? b->used : 0U;
}

fsl_size_t fsl_buffer_capacity(fsl_buffer const * b){
  return b ? b->capacity : 0;
}

bool fsl_data_is_compressed(unsigned char const * mem, fsl_size_t len){
  if(!mem || (len<6)) return 0;
#if 0
  else return ('x'==mem[4])
    && (0234==mem[5]);
  /*
    This check fails for one particular artifact in the tcl core.
    Notes gathered while debugging...

    https://core.tcl.tk/tcl/

    Delta manifest #5f37dcc3 while processing file #687
    (1-based):

    FSL_RC_RANGE: "Delta: copy extends past end of input"

    To reproduce from tcl repo:

    f-acat 5f37dcc3 | f-mfparse -r

    More details:

    Filename: library/encoding/gb2312-raw.enc
    Content: dba09c670f24d47b95d12d4bb9704391b81dda9a

    That artifact is a delta of bccc899015b688d5c426bc791c2fcde3a03a3eb5,
    which is actually two files:

    library/encoding/euc-cn.enc
    library/encoding/gb2312.enc

    When we go to apply the delta, the contents of bccc8 appear to
    be badly compressed data. They have the 'x' at byte offset
    4 but not the 0234 at byte offset 5.

    Turns out it is the fsl_buffer_is_compressed() impl which fails
    for that one.
  */
#else
  else{
    /**
       Adapted from:
       
       https://blog.2of1.org/2011/03/03/decompressing-zlib-images/

       Remember that fossil-compressed data has a 4-byte big-endian
       header holding the uncompressed size of the data, so we skip
       those first 4 bytes.

       See also:

       https://tools.ietf.org/html/rfc6713

       search for "magic number".
    */
    int16_t const head = (((int16_t)mem[4]) << 8) | mem[5];
    /* MARKER(("isCompressed header=%04x\n", head)); */
    switch(head){
      case 0x083c: case 0x087a: case 0x08b8: case 0x08f6:
      case 0x1838: case 0x1876: case 0x18b4: case 0x1872:
      case 0x2834: case 0x2872: case 0x28b0: case 0x28ee:
      case 0x3830: case 0x386e: case 0x38ac: case 0x38ea:
      case 0x482c: case 0x486a: case 0x48a8: case 0x48e6:
      case 0x5828: case 0x5866: case 0x58a4: case 0x58e2:
      case 0x6824: case 0x6862: case 0x68bf: case 0x68fd:
      case 0x7801: case 0x785e: case 0x789c: case 0x78da:
        return true;
      default:
        return false;
    }
  }
#endif
}

bool fsl_buffer_is_compressed(fsl_buffer const *buf){
  return fsl_data_is_compressed( buf->mem, buf->used );
}

fsl_int_t fsl_data_uncompressed_size(unsigned char const *mem,
                                     fsl_size_t len){
  return fsl_data_is_compressed(mem,len)
    ? ((mem[0]<<24) + (mem[1]<<16) + (mem[2]<<8) + mem[3])
    : -1;
}

fsl_int_t fsl_buffer_uncompressed_size(fsl_buffer const * b){
  return fsl_data_uncompressed_size(b->mem, b->used);
}

int fsl_buffer_compress(fsl_buffer const *pIn, fsl_buffer *pOut){
  unsigned int nIn = pIn->used;
  unsigned int nOut = 13 + nIn + (nIn+999)/1000;
  fsl_buffer temp = fsl_buffer_empty;
  int rc = fsl_buffer_resize(&temp, nOut+4);
  if(rc) return rc;
  else{
    unsigned long int nOut2;
    unsigned char *outBuf;
    unsigned long int outSize;
    outBuf = temp.mem;
    outBuf[0] = nIn>>24 & 0xff;
    outBuf[1] = nIn>>16 & 0xff;
    outBuf[2] = nIn>>8 & 0xff;
    outBuf[3] = nIn & 0xff;
    nOut2 = (long int)nOut;
    rc = compress(&outBuf[4], &nOut2,
                  pIn->mem, pIn->used);
    if(rc){
      fsl_buffer_clear(&temp);
      return FSL_RC_ERROR;
    }
    outSize = nOut2+4;
    rc = fsl_buffer_resize(&temp, outSize);
    if(rc){
      fsl_buffer_clear(&temp);
    }else{
      fsl_buffer_swap_free(&temp, pOut, -1);
      assert(0==temp.used);
      assert(outSize==pOut->used);
    }
    return rc;
  }
}

int fsl_buffer_compress2(fsl_buffer const *pIn1,
                         fsl_buffer const *pIn2, fsl_buffer *pOut){
  unsigned int nIn = pIn1->used + pIn2->used;
  unsigned int nOut = 13 + nIn + (nIn+999)/1000;
  fsl_buffer temp = fsl_buffer_empty;
  int rc;
  rc = fsl_buffer_resize(&temp, nOut+4);
  if(rc) return rc;
  else{
    unsigned char *outBuf;
    z_stream stream;
    outBuf = temp.mem;
    outBuf[0] = nIn>>24 & 0xff;
    outBuf[1] = nIn>>16 & 0xff;
    outBuf[2] = nIn>>8 & 0xff;
    outBuf[3] = nIn & 0xff;
    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;
    stream.opaque = 0;
    stream.avail_out = nOut;
    stream.next_out = &outBuf[4];
    deflateInit(&stream, 9);
    stream.avail_in = pIn1->used;
    stream.next_in = pIn1->mem;
    deflate(&stream, 0);
    stream.avail_in = pIn2->used;
    stream.next_in = pIn2->mem;
    deflate(&stream, 0);
    deflate(&stream, Z_FINISH);
    rc = fsl_buffer_resize(&temp, stream.total_out + 4);
    deflateEnd(&stream);
    if(!rc){
      temp.used = stream.total_out + 4;
      if( pOut==pIn1 ) fsl_buffer_reserve(pOut, 0);
      else if( pOut==pIn2 ) fsl_buffer_reserve(pOut, 0);
      assert(!pOut->mem);
      *pOut = temp;
    }else{
      fsl_buffer_reserve(&temp, 0);
    }
    return rc;
  }
}

int fsl_buffer_uncompress(fsl_buffer const *pIn, fsl_buffer *pOut){
  unsigned int nOut;
  unsigned char *inBuf;
  unsigned int nIn = pIn->used;
  fsl_buffer temp = fsl_buffer_empty;
  int rc;
  unsigned long int nOut2;
  if( nIn<=4 ){
    return FSL_RC_RANGE;
  }
  inBuf = pIn->mem;
  nOut = (inBuf[0]<<24) + (inBuf[1]<<16) + (inBuf[2]<<8) + inBuf[3];
  /* MARKER(("decompress size: %u\n", nOut)); */
  rc = fsl_buffer_reserve(&temp, nOut+1);
  if(rc) return rc;
  nOut2 = (long int)nOut;
  rc = uncompress(temp.mem, &nOut2,
                  &inBuf[4], nIn - 4)
    /* valgrind says there's an uninitialized memory access
       somewhere under uncompress(), _presumably_ for one of
       these arguments, but i can't find it. fsl_buffer_reserve()
       always memsets() new bytes to 0.

       Turns out it's a known problem:

       https://www.zlib.net/zlib_faq.html#faq36
    */;
  if( rc!=Z_OK ){
    fsl_buffer_reserve(&temp, 0);
    return FSL_RC_ERROR;
  }
  rc = fsl_buffer_resize(&temp, nOut2);
  if(!rc){
    temp.used = (fsl_size_t)nOut2;
    if( pOut==pIn ){
      fsl_buffer_reserve(pOut, 0);
    }
    assert(!pOut->mem);
    *pOut = temp;
  }else{
    fsl_buffer_reserve(&temp, 0);
  }
  return rc;
}


int fsl_buffer_fill_from( fsl_buffer * dest, fsl_input_f src, void * state )
{
  int rc;
  enum { BufSize = 512 * 8 };
  char rbuf[BufSize];
  fsl_size_t total = 0;
  fsl_size_t rlen = 0;
  if( !dest || ! src ) return FSL_RC_MISUSE;
  fsl_buffer_reuse(dest);
  while(1){
    rlen = BufSize;
    rc = src( state, rbuf, &rlen );
    if( rc ) break;
    total += rlen;
    if(total<rlen){
      /* Overflow! */
      rc = FSL_RC_RANGE;
      break;
    }
    if( dest->capacity < (total+1) ){
      rc = fsl_buffer_reserve( dest,
                               total + ((rlen<BufSize) ? 1 : BufSize)
                               );
      if( 0 != rc ) break;
    }
    memcpy( dest->mem + dest->used, rbuf, rlen );
    dest->used += rlen;
    if( rlen < BufSize ) break;
  }
  if( !rc && dest->used ){
    assert( dest->used < dest->capacity );
    dest->mem[dest->used] = 0;
  }
  return rc;
}

int fsl_buffer_fill_from_FILE( fsl_buffer * dest, FILE * src ){
  return (!dest || !src)
          ? FSL_RC_MISUSE
          : fsl_buffer_fill_from( dest, fsl_input_f_FILE, src );
}          


int fsl_buffer_fill_from_filename( fsl_buffer * dest, char const * filename ){
  if(!dest || !filename || !*filename) return FSL_RC_MISUSE;
  else{
    int rc;
    FILE * src;
    fsl_fstat st = fsl_fstat_empty;
    /* This stat() is only an optimization to reserve all needed
       memory up front.
    */
    rc = fsl_stat( filename, &st, 1 );
    if(!rc && st.size>0){
      rc = fsl_buffer_reserve(dest, st.size +1/*NUL terminator*/);
      if(rc) return rc;
    } /* Else it might not be a real file, e.g. "-", so we'll try anyway... */
    src = fsl_fopen(filename,"rb");
    if(!src) rc = fsl_errno_to_rc(errno, FSL_RC_IO);
    else {
      rc = fsl_buffer_fill_from( dest, fsl_input_f_FILE, src );
      fsl_fclose(src);
    }
    return rc;
  }
}

void fsl_buffer_swap( fsl_buffer * left, fsl_buffer * right ){
  fsl_buffer const tmp = *left;
  *left = *right;
  *right = tmp;
}

void fsl_buffer_swap_free( fsl_buffer * left, fsl_buffer * right, int clearWhich ){
  fsl_buffer_swap(left, right);
  if(0 != clearWhich) fsl_buffer_reserve((clearWhich<0) ? left : right, 0);
}

int fsl_buffer_copy( fsl_buffer const * src, fsl_buffer * dest ){
  fsl_buffer_reuse(dest);
  return src->used
    ? fsl_buffer_append( dest, src->mem, src->used )
    : 0;
}

int fsl_buffer_delta_apply2( fsl_buffer const * orig,
                             fsl_buffer const * pDelta,
                             fsl_buffer * pTarget,
                             fsl_error * pErr){
  int rc;
  fsl_size_t n = 0;
  fsl_buffer out = fsl_buffer_empty;
  rc = fsl_delta_applied_size( pDelta->mem, pDelta->used, &n);
  if(rc){
    if(pErr){
      fsl_error_set(pErr, rc, "fsl_delta_applied_size() failed.");
    }
    return rc;
  }
  rc = fsl_buffer_resize( &out, n );
  if(rc) return rc;
  rc = fsl_delta_apply2( orig->mem, orig->used,
                        pDelta->mem, pDelta->used,
                        out.mem, pErr);
  if(rc){
    fsl_buffer_clear(&out);
  }else{
    fsl_buffer_clear(pTarget);
    *pTarget = out;
  }
  return rc;
}

int fsl_buffer_delta_apply( fsl_buffer const * orig,
                            fsl_buffer const * pDelta,
                            fsl_buffer * pTarget){
  return fsl_buffer_delta_apply2(orig, pDelta, pTarget, NULL);
}

void fsl_buffer_defossilize( fsl_buffer * b ){
  if(b){
    fsl_bytes_defossilize( b->mem, &b->used );
  }
}

int fsl_buffer_to_filename( fsl_buffer const * b, char const * fname ){
  FILE * f;
  int rc = 0;
  if(!b || !fname) return FSL_RC_MISUSE;
  f = fsl_fopen(fname, "wb");
  if(!f) rc = fsl_errno_to_rc(errno, FSL_RC_IO);
  else{
    if(b->used) {
      size_t const frc = fwrite(b->mem, b->used, 1, f);
      rc = (1==frc) ? 0 : FSL_RC_IO;
    }
    fsl_fclose(f);
  }
  return rc;
}

int fsl_buffer_delta_create( fsl_buffer const * src,
                             fsl_buffer const * newVers,
                             fsl_buffer * delta){
  if(!src || !newVers || !delta) return FSL_RC_MISUSE;
  else if((src == newVers)
          || (src==delta)
          || (newVers==delta)) return FSL_RC_MISUSE;
  else{
    int rc = fsl_buffer_reserve( delta, newVers->used + 60 );
    if(!rc){
      delta->used = 0;
      rc = fsl_delta_create( src->mem, src->used,
                             newVers->mem, newVers->used,
                             delta->mem, &delta->used );
      if(!rc){
        rc = fsl_buffer_resize( delta, delta->used );
      }
    }
    return rc;
  }
}


int fsl_output_f_buffer( void * state,
                         void const * src, fsl_size_t n ){
  return (!state || !src)
    ? FSL_RC_MISUSE
    : fsl_buffer_append((fsl_buffer*)state, src, n);
}

int fsl_finalizer_f_buffer( void * state, void * mem ){
  fsl_buffer * b = (fsl_buffer*)mem;
  fsl_buffer_reserve(b, 0);
  *b = fsl_buffer_empty;
  return 0;
}

int fsl_buffer_strftime(fsl_buffer * b, char const * format, const struct tm *timeptr){
  if(!b || !format || !*format || !timeptr) return FSL_RC_MISUSE;
  else{
    enum {BufSize = 128};
    char buf[BufSize];
    fsl_size_t len = fsl_strftime(buf, BufSize, format, timeptr);
    if(!len) return FSL_RC_RANGE;
    return fsl_buffer_append(b, buf, len);
  }
}

int fsl_buffer_stream_lines(fsl_output_f fTo, void * toState,
                            fsl_buffer *pFrom, fsl_size_t N){
  char *z = (char *)pFrom->mem;
  fsl_size_t i = pFrom->cursor;
  fsl_size_t n = pFrom->used;
  fsl_size_t cnt = 0;
  int rc = 0;
  if( N==0 ) return 0;
  while( i<n ){
    if( z[i]=='\n' ){
      cnt++;
      if( cnt==N ){
        i++;
        break;
      }
    }
    i++;
  }
  if( fTo ){
    rc = fTo(toState, &pFrom->mem[pFrom->cursor], i - pFrom->cursor);
  }
  if(!rc){
    pFrom->cursor = i;
  }
  return rc;
}


int fsl_buffer_copy_lines(fsl_buffer *pTo, fsl_buffer *pFrom, fsl_size_t N){
#if 1
  return fsl_buffer_stream_lines( pTo ? fsl_output_f_buffer : NULL, pTo,
                                  pFrom, N );
#else
  char *z = (char *)pFrom->mem;
  fsl_size_t i = pFrom->cursor;
  fsl_size_t n = pFrom->used;
  fsl_size_t cnt = 0;
  int rc = 0;
  if( N==0 ) return 0;
  while( i<n ){
    if( z[i]=='\n' ){
      ++cnt;
      if( cnt==N ){
        ++i;
        break;
      }
    }
    ++i;
  }
  if( pTo ){
    rc = fsl_buffer_append(pTo, &pFrom->mem[pFrom->cursor], i - pFrom->cursor);
  }
  if(!rc){
    pFrom->cursor = i;
  }
  return rc;
#endif
}

int fsl_input_f_buffer( void * state, void * dest, fsl_size_t * n ){
  fsl_buffer * b = (fsl_buffer*)state;
  fsl_size_t const from = b->cursor;
  fsl_size_t to;
  fsl_size_t c;
  if(from >= b->used){
    *n = 0;
    return 0;
  }
  to = from + *n;
  if(to>b->used) to = b->used;
  c = to - from;
  if(c){
    memcpy(dest, b->mem+from, c);
    b->cursor += c;
  }
  *n = c;
  return 0;
}

int fsl_buffer_compare_file( fsl_buffer const * b, char const * zFile ){
  int rc;
  fsl_fstat fst = fsl_fstat_empty;
  rc = fsl_stat(zFile, &fst, 1);
  if(rc || (FSL_FSTAT_TYPE_FILE != fst.type)) return -1;
  else if(b->used < fst.size) return -1;
  else if(b->used > fst.size) return 1;
  else{
#if 1
    FILE * f;
    f = fsl_fopen(zFile,"r");
    if(!f) rc = -1;
    else{
      fsl_buffer fc = *b /* so fsl_input_f_buffer() can manipulate its
                            cursor */;
      rc = fsl_stream_compare(fsl_input_f_buffer, &fc,
                              fsl_input_f_FILE, f);
      assert(fc.mem==b->mem);
      fsl_fclose(f);
    }

#else
    fsl_buffer fc = fsl_buffer_empty;
    rc = fsl_buffer_fill_from_filename(&fc, zFile);
    if(rc){
      rc = -1;
    }else{
      rc = fsl_buffer_compare(b, &fc);
    }
    fsl_buffer_clear(&fc);
#endif
    return rc;
  }
}

char * fsl_buffer_take(fsl_buffer *b){
  char * z = (char *)b->mem;
  *b = fsl_buffer_empty;
  return z;
}

fsl_size_t fsl_buffer_seek(fsl_buffer * b, fsl_int_t offset, fsl_buffer_seek_e  whence){
  int64_t c = (int64_t)b->cursor;
  switch(whence){
    case FSL_BUFFER_SEEK_SET: c = offset;
    case FSL_BUFFER_SEEK_CUR: c = (int64_t)b->cursor + offset; break;
    case FSL_BUFFER_SEEK_END:
      c = (int64_t)b->used + offset;
      /* ^^^^^ fossil(1) uses (used + offset - 1) but

         That seems somewhat arguable because (used + 0 - 1) is at the
         last-written byte (or 1 before the begining), not the
         one-past-the-end point (which corresponds to the
         "end-of-file" described by the fseek() man page). It then
         goes on, in other algos, to operate on that final byte using
         that position, e.g.  blob_read() after a seek-to-end would
         read that last byte, rather than treating the buffer as being
         at the end.

         So... i'm going to naively remove that -1 bit.
      */
      break;
  }
  if(!b->used || c<0) b->cursor = 0;
  else if((fsl_size_t)c > b->used) b->cursor = b->used;
  else b->cursor = (fsl_size_t)c;
  return b->cursor;
}

fsl_size_t fsl_buffer_tell(fsl_buffer const *b){
  return b->cursor;
}

void fsl_buffer_rewind(fsl_buffer *b){
  b->cursor = 0;
}

int fsl_id_bag_to_buffer(fsl_id_bag const * bag, fsl_buffer * b,
                         char const * separator){
  int i = 0;
  fsl_int_t const sepLen = (fsl_id_t)fsl_strlen(separator);
  int rc = fsl_buffer_reserve(b, b->used + (bag->entryCount * 7)
                              + (bag->entryCount * sepLen));
  for(fsl_id_t e = fsl_id_bag_first(bag);
      !rc && e; e = fsl_id_bag_next(bag, e)){
    if(i++) rc = fsl_buffer_append(b, separator, sepLen);
    if(!rc) rc = fsl_buffer_appendf(b, "%" FSL_ID_T_PFMT, e);
  }
  return rc;
}

#undef MARKER
