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
   This copy has been modified slightly for use with the libfossil
   API.
*/
#include "fossil-scm/fossil.h"
#include "fossil-scm/fossil-internal.h"

#include <assert.h>
#include <stddef.h> /* NULL on linux */
#include <ctype.h>

#ifdef _WIN32
# include <windows.h>
#else
#ifdef __CYGWIN__
# include <sys/cygwin.h>
# define CP_UTF8 65001
  __declspec(dllimport) extern __stdcall int WideCharToMultiByte(int, int,
      const char *, int, const char *, int, const char *, const char *);
  __declspec(dllimport) extern __stdcall int MultiByteToWideChar(int, int,
      const char *, int, wchar_t*, int);
#endif
/* /Cygwin */
/* Assume Unix */
#include <stdlib.h> /* getenv() */
#endif


#if defined(__APPLE__) && !defined(WITHOUT_ICONV)
# include <iconv.h>
#endif

#ifdef _WIN32
char *fsl_mbcs_to_utf8(const char *zMbcs){
  extern char *sqlite3_win32_mbcs_to_utf8(const char*);
  return sqlite3_win32_mbcs_to_utf8(zMbcs);
}

void fossil_mbcs_free(char *zOld){
  sqlite3_free(zOld);
}
#endif /* _WIN32 */

void fsl_unicode_free(void *p){
  if(p) fsl_free(p);
}

char *fsl_unicode_to_utf8(const void *zUnicode){
#if defined(_WIN32) || defined(__CYGWIN__)
  int nByte = WideCharToMultiByte(CP_UTF8, 0, zUnicode, -1, 0, 0, 0, 0);
  char *zUtf = (char *)fsl_malloc( nByte );
  if( zUtf ){
    WideCharToMultiByte(CP_UTF8, 0, zUnicode, -1, zUtf, nByte, 0, 0);
  }
  return zUtf;
#else
  return fsl_strdup((char const *)zUnicode);  /* TODO: implement for unix */
#endif
}

void *fsl_utf8_to_unicode(const char *zUtf8){
#if defined(_WIN32) || defined(__CYGWIN__)
  int nByte = MultiByteToWideChar(CP_UTF8, 0, zUtf8, -1, 0, 0);
  wchar_t *zUnicode = (wchar_t *)fsl_malloc( nByte * 2 );
  if( zUnicode ){
    MultiByteToWideChar(CP_UTF8, 0, zUtf8, -1, zUnicode, nByte);
  }
  return zUnicode;
#else
  return fsl_strdup(zUtf8);  /* TODO: implement for unix */
#endif
}

/*
   We find that the built-in isspace() function does not work for
   some international character sets.  So here is a substitute.
*/
char fsl_isspace(int c){
  return c==' ' || (c<='\r' && c>='\t');
}

/*
   Other replacements for ctype.h functions.
*/
char fsl_islower(int c){ return c>='a' && c<='z'; }
char fsl_isupper(int c){ return c>='A' && c<='Z'; }
char fsl_isdigit(int c){ return c>='0' && c<='9'; }
int fsl_tolower(int c){
  return fsl_isupper(c) ? c - 'A' + 'a' : c;
}
int fsl_toupper(int c){
  return fsl_islower(c) ? c - 'a' + 'A' : c;
}
char fsl_isalpha(int c){
  return (c>='a' && c<='z') || (c>='A' && c<='Z');
}
char fsl_isalnum(int c){
  return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9');
}


void fsl_filename_free(void *pOld){
#if defined(_WIN32)
  fsl_free(pOld);
#elif (defined(__APPLE__) && !defined(WITHOUT_ICONV)) || defined(__CYGWIN__)
  fsl_free(pOld);
#else
  /* No-op on all other unix */
#endif
}

char *fsl_filename_to_utf8(const void *zFilename){
#if defined(_WIN32)
  int nByte = WideCharToMultiByte(CP_UTF8, 0, zFilename, -1, 0, 0, 0, 0);
  char *zUtf = fsl_malloc( nByte );
  char *pUtf, *qUtf;
  if( zUtf==0 ){
    return 0;
  }
  WideCharToMultiByte(CP_UTF8, 0, zFilename, -1, zUtf, nByte, 0, 0);
  pUtf = qUtf = zUtf;
  while( *pUtf ) {
    if( *pUtf == (char)0xef ){
      wchar_t c = ((pUtf[1]&0x3f)<<6)|(pUtf[2]&0x3f);
      /* Only really convert it when the resulting char is in range. */
      if ( c && ((c < ' ') || wcschr(L"\"*:<>?|", c)) ){
        *qUtf++ = c; pUtf+=3; continue;
      }
    }
    *qUtf++ = *pUtf++;
  }
  *qUtf = 0;
  return zUtf;
#elif defined(__CYGWIN__)
  char *zOut;
  zOut = fsl_strdup(zFilename)
    /*
      Required for consistency with fsl_utf8_to_filename(),
      so that fsl_filename_free() can DTRT.
    */;
  return zOut;
#elif defined(__APPLE__) && !defined(WITHOUT_ICONV)
  char *zIn = (char*)zFilename;
  char *zOut;
  iconv_t cd;
  size_t n, x;
  for(n=0; zIn[n]>0 && zIn[n]<=0x7f; n++){}
  if( zIn[n]!=0 && (cd = iconv_open("UTF-8", "UTF-8-MAC"))!=(iconv_t)-1 ){
    char *zOutx;
    char *zOrig = zIn;
    size_t nIn, nOutx;
    nIn = n = fsl_strlen(zIn);
    nOutx = nIn+100;
    zOutx = zOut = (char *)fsl_malloc( nOutx+1 );
    if(!zOutx) return NULL;
    x = iconv(cd, &zIn, &nIn, &zOutx, &nOutx);
    if( x==(size_t)-1 ){
      fsl_free(zOut);
      zOut = fsl_strdup(zOrig);
    }else{
      zOut[n+100-nOutx] = 0;
    }
    iconv_close(cd);
  }else{
    zOut = fsl_strdup(zFilename);
  }
  return zOut;
#else
  return (char *)zFilename;  /* No-op on non-mac unix */
#endif
}

void *fsl_utf8_to_filename(const char *zUtf8){
#ifdef _WIN32
  /**
     Maintenance note 2021-03-24: fossil's counterpart of this has
     been extended since this code was ported:

     void *fossil_utf8_to_path(const char *zUtf8, int isDir)

     That isDir param is only for Windows and its only purpose is to
     ensure that the translated path is not within 12 bytes of
     MAX_PATH. That same effect can be had by simply always assuming
     that bool is true and sacrificing those 12 bytes and that far-edge
     case.

     Also, the newer code jumps through many hoops which seem
     unimportant for fossil, e.g. handling UNC-style paths.

     Porting that latter bit over requires someone who can at least
     test whether it compiles.
  */
  int nChar = MultiByteToWideChar(CP_UTF8, 0, zUtf8, -1, 0, 0);
  wchar_t *zUnicode = fsl_malloc( nChar * 2 );
  wchar_t *wUnicode = zUnicode;
  if( zUnicode==0 ){
    return 0;
  }
  MultiByteToWideChar(CP_UTF8, 0, zUtf8, -1, zUnicode, nChar);
  /* If path starts with "<drive>:/" or "<drive>:\", don't translate the ':' */
  if( fsl_isalpha(zUtf8[0]) && zUtf8[1]==':'
           && (zUtf8[2]=='\\' || zUtf8[2]=='/')) {
    zUnicode[2] = '\\';
    wUnicode += 3;
  }
  while( *wUnicode != '\0' ){
    if ( (*wUnicode < ' ') || wcschr(L"\"*:<>?|", *wUnicode) ){
      *wUnicode |= 0xF000;
    }else if( *wUnicode == '/' ){
      *wUnicode = '\\';
    }
    ++wUnicode;
  }
  return zUnicode;
#elif defined(__CYGWIN__)
  char *zPath, *p;
  if( fsl_isalpha(zUtf8[0]) && (zUtf8[1]==':')
      && (zUtf8[2]=='\\' || zUtf8[2]=='/')) {
    /* win32 absolute path starting with drive specifier. */
    int nByte;
    wchar_t zUnicode[2000];
    wchar_t *wUnicode = zUnicode;
    MultiByteToWideChar(CP_UTF8, 0, zUtf8, -1, zUnicode,
                        sizeof(zUnicode)/sizeof(zUnicode[0]));
    while( *wUnicode != '\0' ){
      if( *wUnicode == '/' ){
        *wUnicode = '\\';
      }
      ++wUnicode;
    }
    nByte = cygwin_conv_path(CCP_WIN_W_TO_POSIX, zUnicode, NULL, 0);
    zPath = (char *)fsl_malloc(nByte);
    if(!zPath) return NULL;
    cygwin_conv_path(CCP_WIN_W_TO_POSIX, zUnicode, zPath, nByte);
  }else{
    zPath = fsl_strdup(zUtf8);
    if(!zPath) return NULL;
    zUtf8 = p = zPath;
    while( (*p = *zUtf8++) != 0){
      if( *p++ == '\\' ) {
        p[-1] = '/';
      }
    }
  }
  return zPath;
#elif defined(__APPLE__) && !defined(WITHOUT_ICONV)
  return fsl_strdup(zUtf8)
    /* Why? Why not just act like Unix? */
    ;
#else
  return (void *)zUtf8;  /* No-op on unix */
#endif
}


char *fsl_getenv(const char *zName){
#ifdef _WIN32
  wchar_t *uName = (wchar_t *)fsl_utf8_to_unicode(zName);
  void *zValue = uName ? (void*)_wgetenv(uName) : NULL;
  fsl_free(uName);
#else
  char *zValue = (char *)getenv(zName);
#endif
  if( zValue ) zValue = fsl_filename_to_utf8(zValue);
  return zValue;
}
