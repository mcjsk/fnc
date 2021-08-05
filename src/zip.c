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
*/
/**
   This copy has been modified slightly, and expanded, for use
   with the libfossil project.
*/
#include "fossil-scm/fossil-internal.h"
#include <assert.h>
#include <zlib.h>
#include <stdlib.h> /* atoi() and friends */
#include <memory.h> /* memset() */

#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)


/*
   Write a 16- or 32-bit integer as little-endian into the given buffer.
*/
static void fzip_put16(char *z, int v){
  z[0] = v & 0xff;
  z[1] = (v>>8) & 0xff;
}
static void fzip_put32(char *z, int v){
  z[0] = v & 0xff;
  z[1] = (v>>8) & 0xff;
  z[2] = (v>>16) & 0xff;
  z[3] = (v>>24) & 0xff;
}

/**
    Set the date and time values from an ISO8601 date string.
 */
static void fzip_timestamp_from_str(fsl_zip_writer *z, const char *zDate){
  int y, m, d;
  int H, M, S;

  y = atoi(zDate);
  m = atoi(&zDate[5]);
  d = atoi(&zDate[8]);
  H = atoi(&zDate[11]);
  M = atoi(&zDate[14]);
  S = atoi(&zDate[17]);
  z->dosTime = (H<<11) + (M<<5) + (S>>1);
  z->dosDate = ((y-1980)<<9) + (m<<5) + d;
}

fsl_buffer const * fsl_zip_body( fsl_zip_writer const * z ){
  return z ? &z->body : NULL;
}

void fsl_zip_timestamp_set_julian(fsl_zip_writer *z, double rDate){
  char buf[20] = {0};
  fsl_julian_to_iso8601(rDate, buf, 0);
  fzip_timestamp_from_str(z, buf);
  z->unixTime = (fsl_time_t)((rDate - 2440587.5)*86400.0);
}

void fsl_zip_timestamp_set_unix(fsl_zip_writer *z, fsl_time_t epochTime){
  char buf[20] = {0};
  fsl_julian_to_iso8601(fsl_unix_to_julian(epochTime), buf, 0);
  fzip_timestamp_from_str(z, buf);
  z->unixTime = epochTime;
}


/**
    Adds all directories for the given file to the zip if they are not
    in there already. Returns 0 on success, non-0 on error (namely
    OOM).
 */
static int fzip_mkdir(fsl_zip_writer * z, char const *zName);

/**
    Adds a file entry to zw's zip output. zName is the virtual name of
    the file or directory. If pSrc is NULL then it is assumed that we
    are creating a directory, otherwise the zip's entry is populated
    from pSrc. mPerms specify the fossil-specific permission flags
    from the fsl_fileperm_e enum. If doMkDirs is true then fzip_mkdir()
    is called to create the directory entries for zName, otherwise
    they are not.
 */
static int fzip_file_add(fsl_zip_writer *zw, char const * zName,
                         fsl_buffer const * pSrc, int mPerm,
                         char doMkDirs){
  int rc = 0;
  z_stream stream;
  fsl_size_t nameLen;
  int toOut = 0;
  int iStart;
  int iCRC = 0;
  int nByte = 0;
  int nByteCompr = 0;
  int nBlob;                 /* Size of the blob */
  int iMethod;               /* Compression method. */
  int iMode = 0644;          /* Access permissions */
  char *z;
  char zHdr[30];
  char zExTime[13];
  char zBuf[100];
  char zOutBuf[/*historical: 100000*/ 1024 * 16];

  /* Fill in as much of the header as we know.
  */
  nBlob = pSrc ? (int)pSrc->used : 0;
  if( pSrc ){ /* a file entry */
    iMethod = pSrc->used ? 8 : 0 /* don't compress 0-byte files */;
    switch( mPerm ){
      case FSL_FILE_PERM_LINK:  iMode = 0120755;   break;
      case FSL_FILE_PERM_EXE:   iMode = 0100755;   break;
      default:         iMode = 0100644;   break;
    }
  }else{ /* a directory entry */
    iMethod = 0;
    iMode = 040755;
  }
  if(doMkDirs){
    rc = fzip_mkdir(zw, zName)
      /* This causes an extraneous run of fzip_mkdir(),
         but it is harmless other than the waste of search
         time */;
    if(rc) return rc;
  }

  if(zw->rootDir){
    zw->scratch.used = 0;
    rc = fsl_buffer_appendf(&zw->scratch, "%s%s", zw->rootDir, zName);
    if(rc){
      assert(FSL_RC_OOM==rc);
      return rc;
    }
    zName = fsl_buffer_cstr(&zw->scratch);
  }

  nameLen = fsl_strlen(zName);
  memset(zHdr, 0, sizeof(zHdr));
  fzip_put32(&zHdr[0], 0x04034b50);
  fzip_put16(&zHdr[4], 0x000a);
  fzip_put16(&zHdr[6], 0x0800);
  fzip_put16(&zHdr[8], iMethod);
  fzip_put16(&zHdr[10], zw->dosTime);
  fzip_put16(&zHdr[12], zw->dosDate);
  fzip_put16(&zHdr[26], nameLen);
  fzip_put16(&zHdr[28], 13);

  fzip_put16(&zExTime[0], 0x5455);
  fzip_put16(&zExTime[2], 9);
  zExTime[4] = 3;
  fzip_put32(&zExTime[5], zw->unixTime);
  fzip_put32(&zExTime[9], zw->unixTime);


  /* Write the header and filename.
  */
  iStart = (int)zw->body.used;
  fsl_buffer_append(&zw->body, zHdr, 30);
  fsl_buffer_append(&zw->body, zName, nameLen);
  fsl_buffer_append(&zw->body, zExTime, 13);

  if( nBlob>0 ){
    /* Write the compressed file.  Compute the CRC as we progress.
    */
    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;
    stream.opaque = 0;
    stream.avail_in = pSrc->used;
    stream.next_in = /* (unsigned char*) */pSrc->mem;
    stream.avail_out = sizeof(zOutBuf);
    stream.next_out = (unsigned char*)zOutBuf;
    deflateInit2(&stream, 9, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
    iCRC = crc32(0, stream.next_in, stream.avail_in);
    while( stream.avail_in>0 ){
      deflate(&stream, 0);
      toOut = sizeof(zOutBuf) - stream.avail_out;
      fsl_buffer_append(&zw->body, zOutBuf, toOut);
      stream.avail_out = sizeof(zOutBuf);
      stream.next_out = (unsigned char*)zOutBuf;
    }
    do{
      stream.avail_out = sizeof(zOutBuf);
      stream.next_out = (unsigned char*)zOutBuf;
      deflate(&stream, Z_FINISH);
      toOut = sizeof(zOutBuf) - stream.avail_out;
      fsl_buffer_append(&zw->body, zOutBuf, toOut);
    }while( stream.avail_out==0 );
    nByte = stream.total_in;
    nByteCompr = stream.total_out;
    deflateEnd(&stream);

    /* Go back and write the header, now that we know the compressed file size.
    */
    z = (char *)zw->body.mem + iStart/* &blob_buffer(&body)[iStart] */;
    fzip_put32(&z[14], iCRC);
    fzip_put32(&z[18], nByteCompr);
    fzip_put32(&z[22], nByte);
  }

  /* Make an entry in the tables of contents
  */
  memset(zBuf, 0, sizeof(zBuf));
  fzip_put32(&zBuf[0], 0x02014b50);
  fzip_put16(&zBuf[4], 0x0317);
  fzip_put16(&zBuf[6], 0x000a);
  fzip_put16(&zBuf[8], 0x0800);
  fzip_put16(&zBuf[10], iMethod);
  fzip_put16(&zBuf[12], zw->dosTime);
  fzip_put16(&zBuf[14], zw->dosDate);
  fzip_put32(&zBuf[16], iCRC);
  fzip_put32(&zBuf[20], nByteCompr);
  fzip_put32(&zBuf[24], nByte);
  fzip_put16(&zBuf[28], nameLen);
  fzip_put16(&zBuf[30], 9);
  fzip_put16(&zBuf[32], 0);
  fzip_put16(&zBuf[34], 0);
  fzip_put16(&zBuf[36], 0);
  fzip_put32(&zBuf[38], ((unsigned)iMode)<<16);
  fzip_put32(&zBuf[42], iStart);
  fsl_buffer_append(&zw->toc, zBuf, 46);
  fsl_buffer_append(&zw->toc, zName, nameLen);
  fzip_put16(&zExTime[2], 5);
  fsl_buffer_append(&zw->toc, zExTime, 9);
  ++zw->entryCount;

  return rc;
}

int fzip_mkdir(fsl_zip_writer * z, char const *zName){
  fsl_size_t i;
  fsl_size_t j;
  int rc = 0;
  char const * dirName;
  fsl_size_t nDir = z->dirs.used;
  for(i=0; zName[i]; i++){
    if( zName[i]=='/' ){
      while(zName[i+1]=='/') ++i /* Skip extra slashes */;
      for(j=0; j<nDir; j++){
        /* See if we know this dir already... */
        dirName = (char const *)z->dirs.list[j];
        if( fsl_strncmp(zName, dirName, i)==0 ) break;
      }
      if( j>=nDir ){
        char * cp = fsl_strndup(zName, (fsl_int_t)i+1);
        rc = cp ? fsl_list_append(&z->dirs, cp) : FSL_RC_OOM;
        if(cp && rc){
          fsl_free(cp);
        }else{
          rc = fzip_file_add(z, cp, NULL, 0, 0);
        }
      }
    }
  }
  return rc;
}

int fsl_zip_file_add(fsl_zip_writer *z, char const * zName,
                     fsl_buffer const * pSrc, int mPerm){
  return fzip_file_add(z, zName, pSrc, mPerm, 1);
}

int fsl_zip_root_set(fsl_zip_writer * z, char const * zRoot ){
  if(!z) return FSL_RC_MISUSE;
  else if(zRoot && *zRoot && fsl_is_absolute_path(zRoot)){
    return FSL_RC_RANGE;
  }else{
    fsl_free(z->rootDir);
    z->rootDir = NULL;
    if(zRoot && *zRoot){
      /*
        Problem: we have to mkdir zRoot before we assign z->rootDir to
        avoid an interesting ROOT/ROOT dir entry on an otherwise empty
        ZIP. We create the dirs here, instead of during the first file
        insertion (after z->rootDir is set), to work around that.
      */
      char * cp;
      fsl_size_t n = fsl_strlen(zRoot);
      if('/'==zRoot[n-1]){
        /* Keep the slash */
        cp = fsl_strndup(zRoot, (fsl_int_t)n);
      }else{
        /* Add a slash to our copy... */
        cp = (char *)fsl_malloc(n+2);
        if(cp){
          memcpy( cp, zRoot, n );
          cp[n] = '/';
          cp[n+1] = 0;
          ++n;
        }
      }
      if(!cp) return FSL_RC_OOM;
      else{
        int rc;
        n = fsl_file_simplify_name(cp, (fsl_int_t)n, 1);
        assert(n);
        assert('/'==cp[n-1]);
        cp[n-1] = 0;
        rc = fsl_is_simple_pathname(cp, 1);
        cp[n-1] = '/';
        rc = rc
          ? fzip_mkdir(z, cp)
          : FSL_RC_RANGE;
        z->rootDir = cp /* transfer ownership on error as well and let
                           normal downstream clean it up. */;
        return rc;
      }
    }
    return 0;
  }
}

static void fsl_zip_finalize_impl(fsl_zip_writer * z, char alsoBody){
  if(z){
    fsl_buffer_clear(&z->toc);
    fsl_buffer_clear(&z->scratch);
    fsl_list_visit_free(&z->dirs, 1);
    assert(NULL==z->dirs.list);
    fsl_free(z->rootDir);
    if(alsoBody){
      fsl_buffer_clear(&z->body);
      *z = fsl_zip_writer_empty;
    }else{
      fsl_buffer cp = z->body;
      *z = fsl_zip_writer_empty;
      z->body = cp;
    }
  }
}

void fsl_zip_finalize(fsl_zip_writer * z){
  fsl_zip_finalize_impl(z, 1);
}


int fsl_zip_end( fsl_zip_writer * z ){
  int rc;
  fsl_int_t iTocStart;
  fsl_int_t iTocEnd;
  char zBuf[30];

  iTocStart = (fsl_int_t)z->body.used;
  rc = fsl_buffer_append(&z->body, z->toc.mem, z->toc.used);
  if(rc) return rc;
  fsl_buffer_clear(&z->toc);
  iTocEnd = (fsl_int_t)z->body.used;

  memset(zBuf, 0, sizeof(zBuf));
  fzip_put32(&zBuf[0], 0x06054b50);
  fzip_put16(&zBuf[4], 0);
  fzip_put16(&zBuf[6], 0);
  fzip_put16(&zBuf[8], (int)z->entryCount);
  fzip_put16(&zBuf[10], (int)z->entryCount);
  fzip_put32(&zBuf[12], iTocEnd - iTocStart);
  fzip_put32(&zBuf[16], iTocStart);
  fzip_put16(&zBuf[20], 0);

  rc = fsl_buffer_append(&z->body, zBuf, 22);
  fsl_zip_finalize_impl(z, 0);
  assert(z->body.used);
  return rc;
}

int fsl_zip_end_take( fsl_zip_writer * z, fsl_buffer * dest ){
  if(!z) return FSL_RC_MISUSE;
  else{
    int rc;
    if(!dest){
      rc = FSL_RC_MISUSE;
    }else{
      rc = fsl_zip_end(z);
      if(!rc){
        fsl_buffer_swap( &z->body, dest );
      }
    }
    fsl_zip_finalize( z );
    return rc;
  }
}

int fsl_zip_end_to_filename( fsl_zip_writer * z, char const * filename ){
  if(!z) return FSL_RC_MISUSE;
  else{
    int rc;
    if(!filename || !*filename){
      rc = FSL_RC_MISUSE;
    }else{
      rc = fsl_zip_end(z);
      if(!rc){
        rc = fsl_buffer_to_filename(&z->body, filename);
      }
    }
    fsl_zip_finalize( z );
    return rc;
  }
}



struct ZipState{
  fsl_cx * f;
  fsl_id_t vid;
  fsl_card_F_visitor_f progress;
  void * progressState;
  fsl_zip_writer z;
  fsl_buffer cbuf;
};
typedef struct ZipState ZipState;
static const ZipState ZipState_empty = {
NULL, 0, NULL, NULL,
fsl_zip_writer_empty_m,
fsl_buffer_empty_m
};

static int fsl_card_F_visitor_zip(fsl_card_F const * fc,
                                   void * state){
  ZipState * zs = (ZipState *)state;
  fsl_id_t frid;
  int rc = 0;
  if(!fc->uuid) return 0 /* file was removed in this (delta) manifest */;
  else if(zs->progress){
    rc = (*zs->progress)(fc, zs->progressState);
    if(rc) return rc;
  }else if(FSL_FILE_PERM_LINK == fc->perm){
    return fsl_cx_err_set(zs->f, FSL_RC_NYI,
                          "Symlinks are not yet supported "
                          "in ZIP output.");
  }
  frid = fsl_uuid_to_rid(zs->f, fc->uuid);
  if(frid<0){
    rc = zs->f->error.code;
  }else if(!frid){
      assert(zs->f->error.code);
      rc = zs->f->error.code;
  }else{
    fsl_time_t mTime = 0;
    rc = fsl_mtime_of_manifest_file(zs->f, zs->vid, frid, &mTime);
    if(!rc){
      fsl_zip_timestamp_set_unix(&zs->z, mTime);
      zs->cbuf.used = 0;
      rc = fsl_content_get(zs->f, frid, &zs->cbuf);
      if(!rc){
        rc = fsl_zip_file_add(&zs->z, fc->name, &zs->cbuf,
                              FSL_FILE_PERM_REGULAR);
        if(rc){
          fsl_cx_err_set(zs->f, rc,
                         "Error %s adding file [%s] "
                         "to zip.", fsl_rc_cstr(rc),
                         fc->name);
        }
      }
    }
  }
  return rc;
}


int fsl_repo_zip_sym_to_filename( fsl_cx * f, char const * sym,
                                  char const *  rootDir,
                                  char const * fileName,
                                  fsl_card_F_visitor_f progress,
                                  void * progressState ){
  int rc;
  fsl_deck mf = fsl_deck_empty;
  ZipState zs = ZipState_empty;
  if(!f || !sym || !fileName || !*sym || !*fileName) return FSL_RC_MISUSE;
  else if(!fsl_needs_repo(f)) return FSL_RC_NOT_A_REPO;

  rc = fsl_deck_load_sym( f, &mf, sym, FSL_SATYPE_CHECKIN );
  if(rc) goto end;

  if(rootDir && *rootDir){
    fsl_time_t rootTime = 0;
    rc = fsl_mtime_of_manifest_file(f, mf.rid, 0, &rootTime);
    if(rc) return rc;
    fsl_zip_timestamp_set_unix(&zs.z, rootTime);
    rc = fsl_zip_root_set( &zs.z, rootDir );
    if(rc) goto end;
  }

  rc = fsl_deck_F_rewind(&mf);
  if(rc) goto end;

  zs.f = f;
  zs.vid = mf.rid;
  zs.progress = progress;
  zs.progressState = progressState;
  rc = fsl_deck_F_foreach( &mf, fsl_card_F_visitor_zip, &zs);
  if(!rc){
    if(!zs.z.entryCount){
      if(rootDir && *rootDir){
        rc = fsl_zip_file_add( &zs.z, rootDir, NULL, FSL_FILE_PERM_REGULAR );
      }else{
        rc = fsl_cx_err_set(f, FSL_RC_RANGE, "Cowardly refusing to create "
                            "empty ZIP file for repo version [%.*s].",
                            12, mf.uuid);
      }
      if(rc) goto end;
    }
    rc = fsl_zip_end( &zs.z );
    if(!rc) rc = fsl_buffer_to_filename( fsl_zip_body(&zs.z), fileName );
  }
  end:
  if(rc && !f->error.code){
    fsl_cx_err_set(f, rc, "Error #%d (%s) during ZIP.",
                   rc, fsl_rc_cstr(rc));
  }
  fsl_buffer_clear(&zs.cbuf);
  fsl_zip_finalize(&zs.z);
  fsl_deck_clean(&mf);
  return rc;
}



#undef MARKER
