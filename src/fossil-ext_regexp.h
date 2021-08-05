/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
/*
** 2012-11-13
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
******************************************************************************
**
** The code in this file implements a compact but reasonably
** efficient regular-expression matcher for posix extended regular
** expressions against UTF8 text.
**
** This file is an SQLite extension.  It registers a single function
** named "regexp(A,B)" where A is the regular expression and B is the
** string to be matched.  By registering this function, SQLite will also
** then implement the "B regexp A" operator.  Note that with the function
** the regular expression comes first, but with the operator it comes
** second.
**
**  The following regular expression syntax is supported:
**
**     X*      zero or more occurrences of X
**     X+      one or more occurrences of X
**     X?      zero or one occurrences of X
**     X{p,q}  between p and q occurrences of X
**     (X)     match X
**     X|Y     X or Y
**     ^X      X occurring at the beginning of the string
**     X$      X occurring at the end of the string
**     .       Match any single character
**     \c      Character c where c is one of \{}()[]|*+?.
**     \c      C-language escapes for c in afnrtv.  ex: \t or \n
**     \uXXXX  Where XXXX is exactly 4 hex digits, unicode value XXXX
**     \xXX    Where XX is exactly 2 hex digits, unicode value XX
**     [abc]   Any single character from the set abc
**     [^abc]  Any single character not in the set abc
**     [a-z]   Any single character in the range a-z
**     [^a-z]  Any single character not in the range a-z
**     \b      Word boundary
**     \w      Word character.  [A-Za-z0-9_]
**     \W      Non-word character
**     \d      Digit
**     \D      Non-digit
**     \s      Whitespace character
**     \S      Non-whitespace character
**
** A nondeterministic finite automaton (NFA) is used for matching, so the
** performance is bounded by O(N*M) where N is the size of the regular
** expression and M is the size of the input string.  The matcher never
** exhibits exponential behavior.  Note that the X{p,q} operator expands
** to p copies of X following by q-p copies of X? and that the size of the
** regular expression in the O(N*M) performance bound is computed after
** this expansion.
*/
#if !defined NET_FOSSIL_SCM_REGEXP_H_INCLUDED
#define NET_FOSSIL_SCM_REGEXP_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

struct sqlite3_api_routines;

/*(this file is derived from sqlite3 regexp.c; I wanted to expose
the engine to also be used directly*/

typedef struct sqlite3_ReCompiled sqlite3_ReCompiled;


/*
** Compile a textual regular expression in zIn[] into a compiled regular
** expression suitable for us by sqlite3re_match() and return a pointer to the
** compiled regular expression in *ppRe.  Return NULL on success or an
** error message if something goes wrong.
*/
const char* sqlite3re_compile ( sqlite3_ReCompiled** ppRe, const char* zIn, int noCase );


/* Free and reclaim all the memory used by a previously compiled
** regular expression.  Applications should invoke this routine once
** for every call to re_compile() to avoid memory leaks.
*/
void sqlite3re_free ( sqlite3_ReCompiled* pRe );


/* Run a compiled regular expression on the zero-terminated input
** string zIn[].  Return true on a match and false if there is no match.
*/
int sqlite3re_match ( sqlite3_ReCompiled* pRe, const unsigned char* zIn, int nIn );



/*registers the REGEXP extension function into a sqlite3 database instance */
int sqlite3_regexp_init ( sqlite3* db, char** pzErrMsg,
    const struct sqlite3_api_routines* pApi );


#ifdef __cplusplus
}
#endif

#else
#endif  /*#ifndef NET_FOSSIL_SCM_REGEXP_H_INCLUDED*/
