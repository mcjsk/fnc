/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_HASH_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_HASH_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

  *****************************************************************************
  This file declares public APIs relating to hashing.
*/

#include "fossil-util.h" /* MUST come first b/c of config macros */
#if !defined(FSL_SHA1_HARDENED)
#  define FSL_SHA1_HARDENED 1
#endif
#if defined(__cplusplus)
extern "C" {
#endif

/**
   Various set-in-stone constants used by the API.
*/
enum fsl_hash_constants {
/**
   The length, in bytes, of fossil's hex-form SHA1 UUID strings.
*/
FSL_STRLEN_SHA1 = 40,
/**
   The length, in bytes, of fossil's hex-form SHA3-256 UUID strings.
*/
FSL_STRLEN_K256 = 64,
/**
   The length, in bytes, of a hex-form MD5 hash.
*/
FSL_STRLEN_MD5 = 32,

/** Minimum length of a full UUID. */
FSL_UUID_STRLEN_MIN = FSL_STRLEN_SHA1,
/** Maximum length of a full UUID. */
FSL_UUID_STRLEN_MAX = FSL_STRLEN_K256
};

/**
   Unique IDs for artifact hash types supported by fossil.
*/
enum fsl_hash_types_e {
/** Invalid hash type. */
FSL_HTYPE_ERROR = 0,
/** SHA1. */
FSL_HTYPE_SHA1 = 1,
/** SHA3-256. */
FSL_HTYPE_K256 = 2
};
typedef enum fsl_hash_types_e fsl_hash_types_e;

typedef struct fsl_md5_cx fsl_md5_cx;
typedef struct fsl_sha1_cx fsl_sha1_cx;
typedef struct fsl_sha3_cx fsl_sha3_cx;

/**
   The hash string of the initial MD5 state. Used as an
   optimization for some places where we need an MD5 but know it
   will not hash any data.

   Equivalent to what the md5sum command outputs for empty input:

   @code
   # md5sum < /dev/null
   d41d8cd98f00b204e9800998ecf8427e  -
   @endcode
*/
#define FSL_MD5_INITIAL_HASH "d41d8cd98f00b204e9800998ecf8427e"

/**
   Holds state for MD5 calculations. It is intended to be used like
   this:

   @code
   unsigned char digest[16];
   char hex[FSL_STRLEN_MD5+1];
   fsl_md5_cx cx = fsl_md5_cx_empty;
   // alternately: fsl_md5_init(&cx);
   ...call fsl_md5_update(&cx,...) any number of times to
   ...incrementally calculate the hash.
   fsl_md5_final(&cx, digest); // ends the calculation
   fsl_md5_digest_to_base16(digest, hex);
   // digest now contains the raw 16-byte MD5 digest.
   // hex now contains the 32-byte MD5 + a trailing NUL
   @endcode
*/
struct fsl_md5_cx {
  int isInit;
  uint32_t buf[4];
  uint32_t bits[2];
  unsigned char in[64];
};
#define fsl_md5_cx_empty_m {                                  \
    1/*isInit*/,                                              \
    {/*buf*/0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 }, \
    {/*bits*/0,0},                                            \
    {0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,                \
     0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,                \
     0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,                \
     0,0,0,0}}

/**
   A fsl_md5_cx instance which holds the initial state
   used for md5 calculations. Instances must either be
   copy-initialized from this instance or they must be
   passed to fsl_md5_init() before they are used.
*/
FSL_EXPORT const fsl_md5_cx fsl_md5_cx_empty;

/**
   Initializes the given context pointer. It must not be NULL.  This
   must be the first routine called on any fsl_md5_cx instances.
   Alternately, copy-constructing fsl_md5_cx_empty has the same effect.

   @see fsl_md5_update()
   @see fsl_md5_final()
*/
FSL_EXPORT void fsl_md5_init(fsl_md5_cx *cx);

/**
   Updates cx's state to reflect the addition of the data
   specified by the range (buf, buf+len]. Neither cx nor buf may
   be NULL. This may be called an arbitrary number of times between
   fsl_md5_init() and fsl_md5_final().

   @see fsl_md5_init()
   @see fsl_md5_final()
*/
FSL_EXPORT void fsl_md5_update(fsl_md5_cx *cx, void const * buf, fsl_size_t len);

/**
   Finishes up the calculation of the md5 for the given context and
   writes a 16-byte digest value to the 2nd parameter.  Use
   fsl_md5_digest_to_base16() to convert the digest output value to
   hexidecimal form.

   @see fsl_md5_init()
   @see fsl_md5_update()
   @see fsl_md5_digest_to_base16()
*/
FSL_EXPORT void fsl_md5_final(fsl_md5_cx * cx, unsigned char * digest);

/**
   Converts an md5 digest value (from fsl_md5_final()'s 2nd
   parameter) to a 32-byte (FSL_STRLEN_MD5) CRC string plus a
   terminating NUL byte. i.e.  zBuf must be at least
   (FSL_STRLEN_MD5+1) bytes long.

   @see fsl_md5_final()
*/
FSL_EXPORT void fsl_md5_digest_to_base16(unsigned char *digest, char *zBuf);

/**
   The md5 counterpart of fsl_sha1sum_buffer(), identical in
   semantics except that its result is an MD5 hash instead of an
   SHA1 hash and the resulting hex string is FSL_STRLEN_MD5 bytes
   long plus a terminating NUL.
*/
FSL_EXPORT int fsl_md5sum_buffer(fsl_buffer const *pIn, fsl_buffer *pCksum);

/**
   The md5 counterpart of fsl_sha1sum_cstr(), identical in
   semantics except that its result is an MD5 hash instead of an
   SHA1 hash and the resulting string is FSL_STRLEN_MD5 bytes long
   plus a terminating NUL.
*/
FSL_EXPORT char *fsl_md5sum_cstr(const char *zIn, fsl_int_t len);

/**
   The MD5 counter part to fsl_sha1sum_stream(), with identical
   semantics except that the generated hash is an MD5 string
   instead of SHA1.
*/
FSL_EXPORT int fsl_md5sum_stream(fsl_input_f src, void * srcState, fsl_buffer *pCksum);

/**
   Reads all input from src() and passes it through fsl_md5_update(cx,...).
   Returns 0 on success, FSL_RC_MISUSE if !cx or !src. If src returns
   a non-0 code, that code is returned from here.
*/
FSL_EXPORT int fsl_md5_update_stream(fsl_md5_cx *cx, fsl_input_f src, void * srcState);

/**
   Equivalent to fsl_md5_update(cx, b->mem, b->used). Results are undefined
   if either pointer is invalid or NULL.
*/
FSL_EXPORT void fsl_md5_update_buffer(fsl_md5_cx *cx, fsl_buffer const * b);

/**
   Passes the first len bytes of str to fsl_md5_update(cx). If len
   is less than 0 then fsl_strlen() is used to calculate the
   length.  Results are undefined if either pointer is invalid or
   NULL. This is a no-op if !len or (len<0 && !*str).
*/
FSL_EXPORT void fsl_md5_update_cstr(fsl_md5_cx *cx, char const * str, fsl_int_t len);

/**
   A fsl_md5_update_stream() proxy which updates cx to include the
   contents of the given file.
*/
FSL_EXPORT int fsl_md5_update_filename(fsl_md5_cx *cx, char const * fname);

/**
   The MD5 counter part to fsl_sha1sum_filename(), with identical
   semantics except that the generated hash is an MD5 string
   instead of SHA1.
*/
FSL_EXPORT int fsl_md5sum_filename(const char *zFilename, fsl_buffer *pCksum);


#if FSL_SHA1_HARDENED
typedef void(*fsl_sha1h_collision_callback)(uint64_t, const uint32_t*, const uint32_t*, const uint32_t*, const uint32_t*);
#endif
/**
   Holds state for SHA1 calculations. It is intended to be used
   like this:

   @code
   unsigned char digest[20]
   char hex[FSL_STRLEN_SHA1+1];
   fsl_sha1_cx cx = fsl_sha1_cx_empty;
   // alternately: fsl_sha1_init(&cx)
   ...call fsl_sha1_update(&cx,...) any number of times to
   ...incrementally calculate the hash.
   fsl_sha1_final(&cx, digest); // ends the calculation
   fsl_sha1_digest_to_base16(digest, hex);
   // digest now contains the raw 20-byte SHA1 digest.
   // hex now contains the 40-byte SHA1 + a trailing NUL
   @endcode
*/
struct fsl_sha1_cx {
#if FSL_SHA1_HARDENED
  uint64_t total;
  uint32_t ihv[5];
  unsigned char buffer[64];
  int bigendian;
  int found_collision;
  int safe_hash;
  int detect_coll;
  int ubc_check;
  int reduced_round_coll;
  fsl_sha1h_collision_callback callback;
  uint32_t ihv1[5];
  uint32_t ihv2[5];
  uint32_t m1[80];
  uint32_t m2[80];
  uint32_t states[80][5];
#else
  unsigned int state[5];
  unsigned int count[2];
  unsigned char buffer[64];
#endif
};
/**
   fsl_sha1_cx instance intended for in-struct copy initialization.
*/
#if FSL_SHA1_HARDENED
#define fsl_sha1_cx_empty_m {0}
#else
#define fsl_sha1_cx_empty_m {                                           \
    {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 },      \
    {0,0},                                                              \
    {0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0, \
        0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0 \
        }                                                               \
  }
#endif
/**
   fsl_sha1_cx instance intended for copy initialization. For build
   config portability, the copied-to object must still be passed to
   fsl_sha1_init() to initialize it.
*/
FSL_EXPORT const fsl_sha1_cx fsl_sha1_cx_empty;

/**
   Initializes the given context with the initial SHA1 state.  This
   must be the first routine called on an SHA1 context, and passing
   this context to other SHA1 routines without first having passed
   it to this will lead to undefined results.

   @see fsl_sha1_update()
   @see fsl_sha1_final()
*/
FSL_EXPORT void fsl_sha1_init(fsl_sha1_cx *context);

/**
   Updates the given context to include the hash of the first len
   bytes of the given data.

   @see fsl_sha1_init()
   @see fsl_sha1_final()
*/
FSL_EXPORT void fsl_sha1_update( fsl_sha1_cx *context, void const *data, fsl_size_t len);

/**
   Add padding and finalizes the message digest. If digest is not NULL
   then it writes 20 bytes of digest to the 2nd parameter. If this
   library is configured with hardened SHA1 hashes, this function
   returns non-0 if a collision was detected while hashing. If it is
   not configured for hardened SHA1, or no collision was detected, it
   returns 0.

   @see fsl_sha1_update()
   @see fsl_sha1_digest_to_base16()
*/
FSL_EXPORT int fsl_sha1_final(fsl_sha1_cx *context, unsigned char * digest);

/**
   A convenience form of fsl_sha1_final() which writes
   FSL_STRLEN_SHA1+1 bytes (hash plus terminating NUL byte) to the
   2nd argument and returns a (const char *)-type cast of the 2nd
   argument.
*/
FSL_EXPORT const char * fsl_sha1_final_hex(fsl_sha1_cx *context, char * zHex);

/**
   Convert a digest into base-16.  digest must be at least 20 bytes
   long and hold an SHA1 digest. zBuf must be at least (FSL_STRLEN_SHA1
   + 1) bytes long, to which FSL_STRLEN_SHA1 characters of
   hexidecimal-form SHA1 hash and 1 NUL byte will be written.

   @see fsl_sha1_final()
*/
FSL_EXPORT void fsl_sha1_digest_to_base16(unsigned char *digest, char *zBuf);

/**
   Computes the SHA1 checksum of pIn and stores the resulting
   checksum in the buffer pCksum.  pCksum's memory is re-used if is
   has any allocated to it. pCksum may == pIn, in which case this
   is a destructive operation (replacing the hashed data with its
   hash code).

   Return 0 on success, FSL_RC_OOM if (re)allocating pCksum fails.
*/
FSL_EXPORT int fsl_sha1sum_buffer(fsl_buffer const *pIn, fsl_buffer *pCksum);

/**
   Computes the SHA1 checksum of the first len bytes of the given
   string.  If len is negative then zIn must be NUL-terminated and
   fsl_strlen() is used to find its length. The result is a
   FSL_UUID_STRLEN-byte string (+NUL byte) returned in memory
   obtained from fsl_malloc(), so it must be passed to fsl_free()
   to free it. If NULL==zIn or !len then NULL is returned.
*/
FSL_EXPORT char *fsl_sha1sum_cstr(const char *zIn, fsl_int_t len);

/**
   Consumes all input from src and calculates its SHA1 hash. The
   result is set in pCksum (its contents, if any, are overwritten,
   not appended to). Returns 0 on success. Returns FSL_RC_MISUSE if
   !src or !pCksum. It keeps consuming input from src() until that
   function reads fewer bytes than requested, at which point EOF is
   assumed. If src() returns non-0, that code is returned from this
   function.
*/
FSL_EXPORT int fsl_sha1sum_stream(fsl_input_f src, void * srcState, fsl_buffer *pCksum);


/**
   A fsl_sha1sum_stream() wrapper which calculates the SHA1 of
   given file.

   Returns FSL_RC_IO if the file cannot be opened, FSL_RC_MISUSE if
   !zFilename or !pCksum, else as per fsl_sha1sum_stream().

   TODO: the v1 impl has special behaviour for symlinks which this
   function lacks. For that support we need a variant of this
   function which takes a fsl_cx parameter (for the allow-symlinks
   setting).
*/
FSL_EXPORT int fsl_sha1sum_filename(const char *zFilename, fsl_buffer *pCksum);

/**
   Legal values for SHA3 hash sizes, in bits: an increment of 32 bits
   in the inclusive range (128..512).

   The hexidecimal-code size, in bytes, of any given bit size in this
   enum is the bit size/4.
*/
enum fsl_sha3_hash_size {
/** Sentinel value. Must be 0. */
FSL_SHA3_INVALID = 0,
FSL_SHA3_128 = 128, FSL_SHA3_160 = 160, FSL_SHA3_192 = 192,
FSL_SHA3_224 = 224, FSL_SHA3_256 = 256, FSL_SHA3_288 = 288,
FSL_SHA3_320 = 320, FSL_SHA3_352 = 352, FSL_SHA3_384 = 384,
FSL_SHA3_416 = 416, FSL_SHA3_448 = 448, FSL_SHA3_480 = 480,
FSL_SHA3_512 = 512,
/* Default SHA3 flavor */
FSL_SHA3_DEFAULT = 256
};

/**
   Type for holding SHA3 processing state. Each instance must be
   initialized with fsl_sha3_init(), populated with fsl_sha3_update(),
   and "sealed" with fsl_sha3_end().

   Sample usage:

   @code
   fsl_sha3_cx cx;
   fsl_sha3_init(&cx, FSL_SHA3_DEFAULT);
   fsl_sha3_update(&cx, memory, lengthOfMemory);
   fsl_sha3_end(&cx);
   printf("Hash = %s\n", (char const *)cx.hex);
   @endcode

   After fsl_sha3_end() is called cx.hex contains the hex-string forms
   of the digest. Note that fsl_sha3_update() may be called an arbitrary
   number of times to feed in chunks of memory (e.g. to stream in
   arbitrarily large data).
*/
struct fsl_sha3_cx {
    union {
        uint64_t s[25];         /* Keccak state. 5x5 lines of 64 bits each */
        unsigned char x[1600];  /* ... or 1600 bytes */
    } u;
    unsigned nRate;        /* Bytes of input accepted per Keccak iteration */
    unsigned nLoaded;      /* Input bytes loaded into u.x[] so far this cycle */
    unsigned ixMask;       /* Insert next input into u.x[nLoaded^ixMask]. */
    enum fsl_sha3_hash_size size; /* Size of the hash, in bits. */
    unsigned char hex[132]; /* Hex form of final digest: 56-128 bytes
                               plus terminating NUL. */
};

/**
   If the given number is a valid fsl_sha3_hash_size value, its enum
   entry is returned, else FSL_SHA3_INVALID is returned.

   @see fsl_sha3_init()
*/
FSL_EXPORT enum fsl_sha3_hash_size fsl_sha3_hash_size_for_int(int);

/**
   Initialize a new hash. The second argument specifies the size of
   the hash in bits. Results are undefined if cx is NULL or sz is not
   a valid positive value.

   After calling this, use fsl_sha3_update() to hash data and
   fsl_sha3_end() to finalize the hashing process and generate a digest.
*/
FSL_EXPORT void fsl_sha3_init2(fsl_sha3_cx *cx, enum fsl_sha3_hash_size sz);

/**
   Equivalent to fsl_sha3_init2(cx, FSL_SHA3_DEFAULT).
*/
FSL_EXPORT void fsl_sha3_init(fsl_sha3_cx *cx);

/**
   Updates cx's state to include the first len bytes of data.

   If cx is NULL results are undefined (segfault!). If mem is not
   NULL then it must be at least n bytes long. If n is 0 then this
   function has no side-effects.

   @see fsl_sha3_init()
   @see fsl_sha3_end()
*/
FSL_EXPORT void fsl_sha3_update( fsl_sha3_cx *cx, void const *data, unsigned int len);

/**
   To be called when SHA3 hashing is complete: finishes the hash
   calculation and populates cx->hex with the final hash code in
   hexidecimal-string form. Returns the binary-form digest value,
   which refers to cx->size/8 bytes of memory which lives in the cx
   object. After this call cx->hex will be populated with cx->size/4
   bytes of lower-case ASCII hex codes plus a terminating NUL byte.

   Potential TODO: change fsl_sha1_final() and fsl_md5_final() to use
   these same return semantics.

   @see fsl_sha3_init()
   @see fsl_sha3_update()
*/
FSL_EXPORT unsigned char const * fsl_sha3_end(fsl_sha3_cx *cx);

/**
   SHA3-256 counterpart of fsl_sha1_digest_to_base16(). digest must be at least
   32 bytes long and hold an SHA3 digest. zBuf must be at least (FSL_STRLEN_K256+1)
   bytes long, to which FSL_STRLEN_K256 characters of
   hexidecimal-form SHA3 hash and 1 NUL byte will be written

   @see fsl_sha3_end().
*/
FSL_EXPORT void fsl_sha3_digest_to_base16(unsigned char *digest, char *zBuf);
/**
   SHA3 counter part of fsl_sha1sum_buffer().
*/
FSL_EXPORT int fsl_sha3sum_buffer(fsl_buffer const *pIn, fsl_buffer *pCksum);
/**
   SHA3 counter part of fsl_sha1sum_cstr().
*/
FSL_EXPORT char *fsl_sha3sum_cstr(const char *zIn, fsl_int_t len);
/**
   SHA3 counterpart of fsl_sha1sum_stream().
 */
FSL_EXPORT int fsl_sha3sum_stream(fsl_input_f src, void * srcState, fsl_buffer *pCksum);
/**
   SHA3 counterpart of fsl_sha1sum_filename().
 */
FSL_EXPORT int fsl_sha3sum_filename(const char *zFilename, fsl_buffer *pCksum);

/**
   Expects zHash to be a full-length hash value of one of the
   fsl_hash_types_t-specified types, and nHash to be the length, in
   bytes, of zHash's contents (which must be the full hash length, not
   a prefix). If zHash can be validated as a hash, its corresponding
   hash type is returned, else FSL_HTYPE_ERROR is returned.
*/
FSL_EXPORT fsl_hash_types_e fsl_validate_hash(const char *zHash, int nHash);

/**
   Expects (zHash, nHash) to refer to a full hash (of a supported
   content hash type) of pIn's contents. This routine hashes pIn's
   contents and, if it compares equivalent to zHash then the ID of the
   hash type is returned.  On a mismatch, FSL_HTYPE_ERROR is returned.
*/
FSL_EXPORT fsl_hash_types_e fsl_verify_blob_hash(fsl_buffer const * pIn,
                                                 const char *zHash, int nHash);

/**
   Returns a human-readable name for the given hash type, or its
   second argument h is not a supported hash type.
 */
FSL_EXPORT const char * fsl_hash_type_name(fsl_hash_types_e h, const char *zUnknown);

#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_HASH_H_INCLUDED */
