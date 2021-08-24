/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_INTERNAL_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_INTERNAL_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

  *****************************************************************************
  This file declares library-level internal APIs which are shared
  across the library.
*/

#include "fossil-repo.h" /* a fossil-xxx header MUST come first b/c of config macros */

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct fsl_acache fsl_acache;
typedef struct fsl_acache_line fsl_acache_line;
typedef struct fsl_pq fsl_pq;
typedef struct fsl_pq_entry fsl_pq_entry;

/** @internal

    Queue entry type for the fsl_pq class.

    Potential TODO: we don't currently use the (data) member. We can
    probably remove it.
*/
struct fsl_pq_entry {
  /** RID of the entry. */
  fsl_id_t id;
  /** Raw data associated with this entry. */
  void * data;
  /** Priority of this element. */
  double priority;
};
/** @internal
    Empty-initialized fsl_pq_entry structure.
*/
#define fsl_pq_entry_empty_m {0,NULL,0.0}

/** @internal

    A simple priority queue class. Instances _must_ be initialized
    by copying fsl_pq_empty or fsl_pq_empty_m (depending on where
    the instance lives).
*/
struct fsl_pq {
  /** Number of items allocated in this->list. */
  uint16_t capacity;
  /** Number of items used in this->list. */
  uint16_t used;
  /** The queue. It is kept sorted by entry->priority. */
  fsl_pq_entry * list;
};

/** @internal
    Empty-initialized fsl_pq struct, intended for const-copy initialization.
*/
#define fsl_pq_empty_m {0,0,NULL}

/** @internal
    Empty-initialized fsl_pq struct, intended for copy initialization.
*/
extern  const fsl_pq fsl_pq_empty;

/** @internal

    Clears the contents of p, freeing any memory it owns, but not
    freeing p. Results are undefined if !p.
*/
void fsl_pq_clear(fsl_pq * p);

/** @internal

    Insert element e into the queue. Returns 0 on success, FSL_RC_OOM
    on error. Results are undefined if !p. pData may be NULL.
*/
int fsl_pq_insert(fsl_pq *p, fsl_id_t e,
                  double v, void *pData);

/** @internal

    Extracts (removes) the first element from the queue (the element
    with the smallest value) and return its ID.  Return 0 if the queue
    is empty. If pp is not NULL then *pp is (on success) assigned to
    opaquedata pointer mapped to the entry.
*/
fsl_id_t fsl_pq_extract(fsl_pq *p, void **pp);

/** @internal

    Holds one "line" of a fsl_acache cache.
*/
struct fsl_acache_line {
  /**
     RID of the cached record.
  */
  fsl_id_t rid;
  /**
     Age. Newer is larger.
  */
  fsl_int_t age;
  /**
     Content of the artifact.
  */
  fsl_buffer content;
};
/** @internal

    Empty-initialized fsl_acache_line structure.
*/
#define fsl_acache_line_empty_m { 0,0,fsl_buffer_empty_m }


/** @internal

    A cache for tracking the existence of artifacts while the
    internal goings-on of control artifacts are going on.

    Currently the artifact cache is unused because it costs much
    more than it gives us. Once the library supports certain
    operations (like rebuild and sync) caching will become more
    useful.

    Historically fossil caches artifacts as their blob content, but
    libfossil will likely (at some point) to instead cache fsl_deck
    instances, which contain all of the same data in pre-parsed form.
    It cost more memory, though. That approach also precludes caching
    non-structural artifacts (i.e. opaque client blobs).

    Potential TODO: the limits of the cache size are hard-coded in
    fsl_acache_insert. Those really should be part of this struct.
*/
struct fsl_acache {
  /**
     Total amount of buffer memory (in bytes) used by cached content.
     This does not account for memory held by this->list.
  */
  fsl_size_t szTotal;
  /**
     Limit on the (approx.) amount of memory (in bytes) which can be
     taken up by the cached buffers at one time. Fossil's historical
     value is 50M.
  */
  fsl_size_t szLimit;
  /**
     Number of entries "used" in this->list.
  */
  uint16_t used;
  /**
     Approximate upper limit on the number of entries in this->list.
     This limit may be violated slightly.

     This number should ideally be relatively small: 3 digits or less.
     Fossil's historical value is 500.
  */
  uint16_t usedLimit;
  /**
     Number of allocated slots in this->list.
  */
  uint16_t capacity;
  /**
     Next cache counter age. Higher is newer.
  */
  fsl_int_t nextAge;
  /**
     List of cached content, ordered by age.
  */
  fsl_acache_line * list;
  /**
     All artifacts currently in the cache.
  */
  fsl_id_bag inCache;
  /**
     Cache of known-missing content.
  */
  fsl_id_bag missing;
  /**
     Cache of of known-existing content.
  */
  fsl_id_bag available;
};
/** @internal

    Empty-initialized fsl_acache structure, intended
    for const-copy initialization.
*/
#define fsl_acache_empty_m {                \
  0/*szTotal*/,                             \
  20000000/*szLimit. Historical fossil value=50M*/, \
  0/*used*/,300U/*usedLimit. Historical fossil value=500*/,\
  0/*capacity*/,                            \
  0/*nextAge*/,NULL/*list*/,                \
  fsl_id_bag_empty_m/*inCache*/,            \
  fsl_id_bag_empty_m/*missing*/,            \
  fsl_id_bag_empty_m/*available*/           \
}
/** @internal

    Empty-initialized fsl_acache structure, intended
    for copy initialization.
*/
extern const fsl_acache fsl_acache_empty;

/** @internal

   Very internal.

   "Manifest cache" for fsl_deck entries encountered during
   crosslinking. This type is intended only to be embedded in fsl_cx.

   The routines for managing this cache are static in deck.c:
   fsl_cx_mcache_insert() and fsl_cx_mcache_search().

   The array members in this struct MUST have the same length
   or results are undefined.
*/
struct fsl_mcache {
  /** Next age value. No clue how the cache will react once this
      overflows. */
  unsigned nextAge;
  /** The virtual age of each deck in the cache. They get evicted
      oldest first. */
  unsigned aAge[4];
  /** Counts the number of cache hits. */
  unsigned hits;
  /** Counts the number of cache misses. */
  unsigned misses;
  /**
     Stores bitwise copies of decks. Storing a fsl_deck_malloc() deck
     into the cache requires bitwise-copying is contents, wiping out
     its contents via assignment from fsl_deck_empty, then
     fsl_free()'ing it (as opposed to fsl_deck_finalize(), which would
     attempt to clean up memory which now belongs to the cache's
     copy).

     Array sizes of 6 and 10 do not appreciably change the hit rate
     compared to 4, at least not for current (2021-03-26) uses.
  */
  fsl_deck decks[4];
};

/** Convenience typedef. */
typedef struct fsl_mcache fsl_mcache;

/** Initialized-with-defaults fsl_mcache structure, intended for
    const-copy initialization. */
#define fsl_mcache_empty_m {\
  0,                  \
  {0,0,0,0},\
  0,0, \
  {fsl_deck_empty_m,fsl_deck_empty_m,fsl_deck_empty_m,   \
  fsl_deck_empty_m} \
}

/** Initialized-with-defaults fsl_mcache structure, intended for
    non-const copy initialization. */
extern const fsl_mcache fsl_mcache_empty;


/* The fsl_cx class is documented in main public header. */
struct fsl_cx {
  /**
     A pointer to the "main" db handle. Exactly which db IS the
     main db is, because we have three DBs, not generally knowble.

     As of this writing (20141027) the following applies:

     dbMain always points to &this->dbMem (a ":memory:" db opened by
     fsl_cx_init()), and the repo/ckout/config DBs get ATTACHed to
     that one. Their separate handles (this->{repo,ckout,config}.db)
     are used to store the name and file path to each one (even though
     they have no real db handle associated with them).

     Internal code should rely as little as possible on the actual
     arrangement of internal DB handles, and should use
     fsl_cx_db_repo(), fsl_cx_db_ckout(), and fsl_cx_db_config() to
     get a handle to the specific db they want. Currently they will
     always return NULL or the same handle, but that design decision
     might change at some point, so the public API treats them as
     separate entities.
  */
  fsl_db * dbMain;

  /**
     Marker which tells us whether fsl_cx_finalize() needs
     to fsl_free() this instance or not.
  */
  void const * allocStamp;

  /**
     A ":memory:" (or "") db to work around
     open-vs-attach-vs-main-vs-real-name problems wrt to the
     repo/ckout/config dbs. This db handle gets opened automatically
     at startup and all others which a fsl_cx manages get ATTACHed to
     it. Thus the other internal fsl_db objects, e.g. this->repo.db,
     may hold state, such as the path to the current repo db, but they
     do NOT hold an sqlite3 db handle. Assigning them the handle of
     this->dbMain would indeed simplify certain work but it would
     require special care to ensure that we never sqlite3_close()
     those out from under this->dbMain.
  */
  fsl_db dbMem;

  /**
     Holds info directly related to a checkout database.
  */
  struct {
    /**
       Handle to the currently opened checkout database IF the checkout
       is the main db.
    */
    fsl_db db;
    /**
       Possibly not needed, but useful for doing absolute-to-relative
       path conversions for checking file lists.

       The directory part of an opened checkout db.  This is currently
       only set by fsl_ckout_open_dir(). It contains a trailing slash,
       largely because that simplifies porting fossil(1) code and
       appending filenames to this directory name to create absolute
       paths.
    */
    char * dir;
    /**
       Optimization: fsl_strlen() of dir. Guaranteed to be set to
       dir's length if dir is not NULL.
    */
    fsl_size_t dirLen;
    /**
       The rid of the current checkout. May be 0 for an empty
       repo/checkout. Must be negative if not yet known.
    */
    fsl_id_t rid;

    /**
       The UUID of the current checkout. Only set if this->rid
       is positive.
    */
    fsl_uuid_str uuid;

    /**
       Julian mtime of the checkout version, as reported by the
       [event] table.
    */
    double mtime;
  } ckout;

  /**
     Holds info directly related to a repo database.
  */
  struct {
    /**
       Handle to the currently opened repository database IF repo
       is the main db.
    */
    fsl_db db;
    /**
       The default user name, for operations which need one.
       See fsl_cx_user_set().
    */
    char * user;
  } repo;

  /**
     Holds info directly related to a global config database.
  */
  struct {
    /**
       Handle to the currently opened global config database IF config
       is the main db.
    */
    fsl_db db;
  } config;

  /**
     State for incrementally proparing a checkin operation.
  */
  struct {
    /**
       Holds a list of "selected files" in the form
       of vfile.id values.
    */
    fsl_id_bag selectedIds;

    /**
       The deck used for incrementally building certain parts of a
       checkin.
    */
    fsl_deck mf;
  } ckin;

  /**
     Confirmation callback.
  */
  fsl_confirmer confirmer;

  /**
     Output channel used by fsl_output() and friends.
  */
  fsl_outputer output;

  /**
     Can be used to tie client-specific data to the context. Its
     finalizer is called when fsl_cx_finalize() cleans up.
  */
  fsl_state clientState;

  /**
     Holds error state. As a general rule, this information is updated
     only by routines which need to return more info than a simple
     integer error code. e.g. this is often used to hold
     db-driver-provided error state. It is not used by "simple"
     routines for which an integer code always suffices. APIs which
     set this should denote it with a comment like "updates the
     context's error state on error."
  */
  fsl_error error;

  /**
     A place for temporarily holding file content. We use this in
     places where we have to loop over files and read their entire
     contents, so that we can reuse this buffer's memory if possible.
     The loop and the reading might be happening in different
     functions, though, and some care must be taken to avoid use in
     two functions concurrently.
  */
  fsl_buffer fileContent;

  /**
     Reuseable scratchpads for low-level file canonicalization
     buffering and whatnot. Not intended for huge content: use
     this->fileContent for that. This list must stay relatively
     short. As of this writing, most buffers ever active at once was
     5, but 1-2 is more common.

     @see fsl_cx_scratchpad()
     @see fsl_cx_scratchpad_yield()
  */
  struct {
    /**
       Strictly-internal temporary buffers we intend to reuse many
       times, mostly for filename canonicalization, holding hash
       values, and small encoding/decoding tasks. These must never be
       used for values which will be long-lived, nor are they intended
       to be used for large content, e.g. reading files, with the
       possible exception of holding versioned config settings, as
       those are typically rather small.

       If needed, the lengths of this->buf[] and this->used[] may be
       extended, but anything beyond 8, maybe 10, seems a bit extreme.
       They should only be increased if we find code paths which
       require it. As of this writing (2021-03-17), the peak
       concurrently used was 5. In any case fsl_cx_scratchpad() fails
       fatally if it needs more than it has, so we won't fail to
       overlook such a case.
    */
    fsl_buffer buf[8];
    /**
       Flags telling us which of this->buf is currenly in use.
    */
    bool used[8];
    /**
       A cursor _hint_ to try to speed up fsl_cx_scratchpad() by about
       half a nanosecond, making it O(1) instead of O(small N) for the
       common case.
    */
    short next;
  } scratchpads;

  /**
     A copy of the config object passed to fsl_cx_init() (or some
     default).
  */
  fsl_cx_config cxConfig;

  /**
     Flags, some (or one) of which is runtime-configurable by the
     client (see fsl_cx_flags_e). We can get rid of this and add the
     flags to the cache member along with the rest of them.
  */
  int flags;

  /**
     List of callbacks for deck crosslinking purposes.
  */
  fsl_xlinker_list xlinkers;

  /**
     A place for caching generic things.
  */
  struct {
    /**
       If true, SOME repository-level file-name comparisons/searches
       will work case-insensitively.
    */
    bool caseInsensitive;

    /**
       If true, skip "dephantomization" of phantom blobs.  This is a
       detail from fossil(1) with as-yet-undetermined utility. It's
       apparently only used during the remote-sync process, which this
       API does not (as of 2021-02) yet have.
    */
    bool ignoreDephantomizations;

    /**
       Whether or not a running commit process should be marked as
       private.
    */
    bool markPrivate;

    /**
       True if fsl_crosslink_begin() has been called but
       fsl_crosslink_end() is still pending.
    */
    bool isCrosslinking;

    /**
       Flag indicating that only cluster control artifacts should be
       processed by manifest crosslinking. This will only be relevant
       if/when the sync protocol is implemented.
    */
    bool xlinkClustersOnly;

    /**
       Is used to tell the content-save internals that a "final
       verification" (a.k.a. verify-before-commit) is underway.
    */
    bool inFinalVerify;

    /**
       Cached copy of the allow-symlinks config option, because it is
       (hypothetically) needed on many stat() call. Negative
       value=="not yet determined", 0==no, positive==yes. The negative
       value means we need to check the repo config resp. the global
       config to see if this is on.

       As of late 2020, fossil(1) is much more restrictive with
       symlinks due to vulnerabilities which were discovered by a
       security researcher, and we definitely must not default any
       symlink-related features to enabled/on. As of Feb. 2021, my
       personal preference, and very likely plan of attack, is to only
       treat SCM'd symlinks as if symlinks support is disabled. It's
       very unlikely that i will implement "real" symlink support but
       would, *solely* for compatibility with fossil(1), be convinced
       to permit such changes if someone else wants to implement them.
    */
    short allowSymlinks;

    /**
       Indicates whether or not this repo has ever seen a delta
       manifest. If none has ever been seen then the repository will
       prefer to use baseline (non-delta) manifests. Once a delta is
       seen in the repository, the checkin algorithm is free to choose
       deltas later on unless its otherwise prohibited, e.g. by the
       forbid-delta-manifests config db setting.

       This article provides an overview to the topic delta manifests
       and essentially debunks their ostensible benefits:

       https://fossil-scm.org/home/doc/tip/www/delta-manifests.md

       Values: negative==undetermined, 0==no, positive==yes. This is
       updated when a repository is first opened and when new content
       is written to it.
    */
    short seenDeltaManifest;

    /**
       Records whether this repository has an FTS search
       index. <0=undetermined, 0=no, >0=yes.
     */
    short searchIndexExists;

    /**
       Cache for the "manifest" config setting, as used by
       fsl_ckout_manifest_setting(), with the caveat that
       if the setting changes after it is cached, we won't necessarily
       see that here!
    */
    short manifestSetting;

    /**
       Record ID of rcvfrom entry during commits. This is likely to
       remain unused in libf until/unless the sync protocol is
       implemented.
    */
    fsl_id_t rcvId;
    
    /**
       Artifact cache used during processing of manifests.
    */
    fsl_acache arty;
    /**
       Used during manifest parsing to keep track of artifacts we have
       seen. Whether that's really necessary or is now an unnecessary
       porting artifact (haha) is unclear.
    */
    fsl_id_bag mfSeen;
    /**
       Used during the processing of manifests to keep track of
       "leaf checks" which need to be done downstream.
    */
    fsl_id_bag leafCheck;
    /**
       Holds the RID of every record awaiting verification
       during the verify-at-commit checks.
    */
    fsl_id_bag toVerify;
    /**
       Infrastructure for fsl_mtime_of_manifest_file(). It
       remembers the previous RID so that it knows when it has to
       invalidate/rebuild its ancestry cache.
    */
    fsl_id_t mtimeManifest;
    /**
       The "project-code" config option. We do not currently (2021-03)
       use this but it will be important if/when the sync protocol is
       implemented or we want to create hashes, e.g. for user
       passwords, which depend in part on the project code.
    */
    char * projectCode;

    /**
       Internal optimization to avoid duplicate stat() calls across
       two functions in some cases.
    */
    fsl_fstat fstat;

    /**
       Parsed-deck cache.
    */
    fsl_mcache mcache;
    
    /**
       Holds various glob lists. That said... these features are
       actually app-level stuff which the library itself does not
       resp. should not enforce. We can keep track of these for users
       but the library internals _generall_ have no business using
       them.

       _THAT_ said... these have enough generic utility that we can
       justify storing them and _optionally_ applying them. See
       fsl_checkin_opt for an example of where we do this.
    */
    struct {
      /**
         Holds the "ignore-glob" globs.
      */
      fsl_list ignore;
      /**
         Holds the "binary-glob" globs.
      */
      fsl_list binary;
      /**
         Holds the "crnl-glob" globs.
      */
      fsl_list crnl;
    } globs;
  } cache;

  /**
     Ticket-related information.
  */
  struct {
    /**
       Holds a list of (fsl_card_J*) records representing custom
       ticket table fields available in the db.

       Each entry's flags member denote (using fsl_card_J_flags)
       whether that field is used by the ticket or ticketchng
       tables.

       TODO, eventually: add a separate type for these entries.  We
       use fsl_card_J because the infrastructure is there and they
       provide what we need, but fsl_card_J::flags only exists for
       this list. A custom type would be smaller than fsl_card_J
       (only two members) but adding it requires adding some
       infrastructure which isn't worth the effort at the moment.
    */
    fsl_list customFields;

    /**
       Gets set to true (at some point) if the client has the
       ticket db table.
    */
    bool hasTicket;
    /**
       Gets set to true (at some point) if the client has the
       ticket.tkt_ctime db field.
    */
    bool hasCTime;

    /**
       Gets set to true (at some point) if the client has the
       ticketchng db table.
    */
    bool hasChng;
    /**
       Gets set to true (at some point) if the client has the
       ticketchng.rid db field.
    */
    bool hasChngRid;
  } ticket;

  /*
    Note: no state related to server/user/etc. That is higher-level
    stuff. We might need to allow the user to set a default user
    name to avoid that he has to explicitly set it on all of the
    various Control Artifact-generation bits which need it.
  */
};

/** @internal

    Initialized-with-defaults fsl_cx struct.
*/
#define fsl_cx_empty_m {                                \
    NULL /*dbMain*/,                                    \
    NULL/*allocStamp*/,                               \
    fsl_db_empty_m /* dbMem */,                       \
    {/*ckout*/                                        \
      fsl_db_empty_m /*db*/,                          \
      NULL /*dir*/, 0/*dirLen*/,                    \
      -1/*rid*/, NULL/*uuid*/, 0/*mtime*/        \
    },                                            \
    {/*repo*/ fsl_db_empty_m /*db*/,                  \
       0/*user*/                                     \
    },                                            \
    {/*config*/ fsl_db_empty_m /*db*/ },              \
    {/*ckin*/                                         \
      fsl_id_bag_empty_m/*selectedIds*/,            \
      fsl_deck_empty_m/*mf*/                     \
    },                                            \
    fsl_confirmer_empty_m/*confirmer*/,           \
    fsl_outputer_FILE_m /*output*/,                 \
    fsl_state_empty_m /*clientState*/,            \
    fsl_error_empty_m /*error*/,                  \
    fsl_buffer_empty_m /*fileContent*/,           \
    {/*scratchpads*/ \
      {fsl_buffer_empty_m,fsl_buffer_empty_m,     \
      fsl_buffer_empty_m,fsl_buffer_empty_m,      \
      fsl_buffer_empty_m,fsl_buffer_empty_m},     \
      {false,false,false,false,false,false},      \
      0/*next*/                                   \
    },                                            \
    fsl_cx_config_empty_m /*cxConfig*/,           \
    FSL_CX_F_DEFAULTS/*flags*/,                   \
    fsl_xlinker_list_empty_m/*xlinkers*/,         \
    {/*cache*/                                    \
      false/*caseInsensitive*/,                  \
      false/*ignoreDephantomizations*/,          \
      false/*markPrivate*/,                         \
      false/*isCrosslinking*/,                      \
      false/*xlinkClustersOnly*/,                   \
      false/*inFinalVerify*/,                       \
      -1/*allowSymlinks*/,                      \
      -1/*seenDeltaManifest*/,                  \
      -1/*searchIndexExists*/,                  \
      -1/*manifestSetting*/,\
      0/*rcvId*/,                               \
      fsl_acache_empty_m/*arty*/,               \
      fsl_id_bag_empty_m/*mfSeen*/,           \
      fsl_id_bag_empty_m/*leafCheck*/,        \
      fsl_id_bag_empty_m/*toVerify*/,         \
      0/*mtimeManifest*/,                     \
      NULL/*projectCode*/,                    \
      fsl_fstat_empty_m/*fstat*/,             \
      fsl_mcache_empty_m/*mcache*/,           \
      {/*globs*/                              \
        fsl_list_empty_m/*ignore*/,           \
        fsl_list_empty_m/*binary*/,         \
        fsl_list_empty_m/*crnl*/            \
      }                                     \
    }/*cache*/,                             \
    {/*ticket*/                             \
      fsl_list_empty_m/*customFields*/,     \
      0/*hasTicket*/,                       \
      0/*hasCTime*/,                        \
      0/*hasChng*/,                         \
      0/*hasCngRid*/                        \
    }                                       \
  }

/** @internal
    Initialized-with-defaults fsl_cx instance.
*/
FSL_EXPORT const fsl_cx fsl_cx_empty;

/*
  TODO:

  int fsl_buffer_append_getenv( fsl_buffer * b, char const * env )

  Fetches the given env var and appends it to b. Returns FSL_RC_NOT_FOUND
  if the env var is not set. The primary use for this would be to simplify
  the Windows implementation of fsl_find_home_dir().
*/


/** @internal

    Expires the single oldest entry in c. Returns true if it removes
    an item, else false.
*/
FSL_EXPORT bool fsl_acache_expire_oldest(fsl_acache * c);

/** @internal

    Add an entry to the content cache.

    This routines transfers the contents of pBlob over to c,
    regardless of success or failure.  The cache will deallocate the
    memory when it has finished with it.

    If the cache cannot add the entry due to cache-internal
    constraints, as opposed to allocation errors, it clears the buffer
    (for consistency's sake) and returned 0.

    Returns 0 on success, FSL_RC_OOM on allocation error. Has undefined
    behaviour if !c, rid is not semantically valid, !pBlob. An empty
    blob is normally semantically illegal but is not strictly illegal
    for this cache's purposes.
*/
FSL_EXPORT int fsl_acache_insert(fsl_acache * c, fsl_id_t rid, fsl_buffer *pBlob);

/** @internal

    Frees all memory held by c, and clears out c's state, but does
    not free c. Results are undefined if !c.
*/
FSL_EXPORT void fsl_acache_clear(fsl_acache * c);

/** @internal

    Checks f->cache.arty to see if rid is available in the
    repository opened by f.

    Returns 0 if the content for the given rid is available in the
    repo or the cache. Returns FSL_RC_NOT_FOUND if it is not in the
    repo nor the cache. Returns some other non-0 code for "real
    errors," e.g. FSL_RC_OOM if a cache allocation fails. This
    operation may update the cache's contents.

    If this function detects a loop in artifact lineage, it fails an
    assert() in debug builds and returns FSL_RC_CONSISTENCY in
    non-debug builds. That doesn't happen in real life, though.
*/
FSL_EXPORT int fsl_acache_check_available(fsl_cx * f, fsl_id_t rid);

/** @internal

    This is THE ONLY routine which adds content to the blob table.

    This writes the given buffer content into the repository
    database's blob tabe. It Returns the record ID via outRid (if it
    is not NULL).  If the content is already in the database (as
    determined by a lookup of its hash against blob.uuid), this
    routine fetches the RID (via *outRid) but has no side effects in
    the repo.

    If srcId is >0 then pBlob must contain delta content from
    the srcId record. srcId might be a phantom.  

    pBlob is normally uncompressed text, but if uncompSize>0 then
    the pBlob value is assumed to be compressed (via fsl_buffer_compress()
    or equivalent) and uncompSize is
    its uncompressed size. If uncompSize>0 then zUuid must be valid
    and refer to the hash of the _uncompressed_ data (which is why
    this routine does not calculate it for the client).

    Sidebar: we "could" use fsl_buffer_is_compressed() and friends
    to determine if pBlob is compressed and get its decompressed
    size, then remove the uncompSize parameter, but that would
    require that this function decompress the content to calculate
    the hash. Since the caller likely just compressed it, that seems
    like a huge waste.

    zUuid is the UUID of the artifact, if it is not NULL.  When
    srcId is specified then zUuid must always be specified.  If
    srcId is zero, and zUuid is zero then the correct zUuid is
    computed from pBlob.  If zUuid is not NULL then this function
    asserts (in debug builds) that fsl_is_uuid() returns true for
    zUuid.

    If isPrivate is true, the blob is created as a private record.

    If the record already exists but is a phantom, the pBlob content
    is inserted and the phatom becomes a real record.

    The original content of pBlob is not disturbed.  The caller continues
    to be responsible for pBlob.  This routine does *not* take over
    responsibility for freeing pBlob.

    If outRid is not NULL then on success *outRid is assigned to the
    RID of the underlying blob record.

    Returns 0 on success and there are too many potential error cases
    to name - this function is a massive beast.

    Potential TODO: we don't really need the uncompSize param - we
    can deduce it, if needed, based on pBlob's content. We cannot,
    however, know the UUID of the decompressed content unless the
    client passes it in to us.

    @see fsl_content_put()
*/
FSL_EXPORT int fsl_content_put_ex( fsl_cx * f, fsl_buffer const * pBlob,
                                   fsl_uuid_cstr zUuid, fsl_id_t srcId,
                                   fsl_size_t uncompSize, bool isPrivate,
                                   fsl_id_t * outRid);
/** @internal

    Equivalent to fsl_content_put_ex(f,pBlob,NULL,0,0,0,newRid).

    This must only be used for saving raw (non-delta) content.

    @see fsl_content_put_ex()
*/
FSL_EXPORT int fsl_content_put( fsl_cx * f, fsl_buffer const * pBlob,
                                fsl_id_t * newRid);


/** @internal

    If the given blob ID refers to deltified repo content, this routine
    undeltifies it and replaces its content with its expanded
    form.

    Returns 0 on success, FSL_RC_MISUSE if !f, FSL_RC_NOT_A_REPO if
    f has no opened repository, FSL_RC_RANGE if rid is not positive,
    and any number of other potential errors during the db and
    content operations. This function treats already unexpanded
    content as success.

    @see fsl_content_deltify()
*/
FSL_EXPORT int fsl_content_undeltify(fsl_cx * f, fsl_id_t rid);


/** @internal

    The converse of fsl_content_undeltify(), this replaces the storage
    of the given blob record so that it is a delta of srcid.

    If rid is already a delta from some other place then no
    conversion occurs and this is a no-op unless force is true.

    If rid's contents are not available because the the rid is a
    phantom or depends to one, no delta is generated and 0 is
    returned.

    It never generates a delta that carries a private artifact into
    a public artifact. Otherwise, when we go to send the public
    artifact on a sync operation, the other end of the sync will
    never be able to receive the source of the delta.  It is OK to
    delta private->private, public->private, and public->public.
    Just no private->public delta. For such cases this function
    returns 0, as opposed to FSL_RC_ACCESS or some similar code, and
    leaves the content untouched.

    If srcid is a delta that depends on rid, then srcid is
    converted to undelta'd text.

    If either rid or srcid contain less than some "small,
    unspecified number" of bytes (currently 50), or if the resulting
    delta does not achieve a compression of at least 25%, the rid is
    left untouched.

    Returns 0 if a delta is successfully made or none needs to be
    made, non-0 on error.

    @see fsl_content_undeltify()
*/
FSL_EXPORT int fsl_content_deltify(fsl_cx * f, fsl_id_t rid,
                                   fsl_id_t srcid, bool force);


/** @internal

    Creates a new phantom blob with the given UUID and return its
    artifact ID via *newId. Returns 0 on success, FSL_RC_MISUSE if
    !f or !uuid, FSL_RC_RANGE if fsl_is_uuid(uuid) returns false,
    FSL_RC_NOT_A_REPO if f has no repository opened, FSL_RC_ACCESS
    if the given uuid has been shunned, and about 20 other potential
    error codes from the underlying db calls. If isPrivate is true
    _or_ f has been flagged as being in "private mode" then the new
    content is flagged as private. newId may be NULL, but if it is
    then the caller will have to find the record id himself by using
    the UUID (see fsl_uuid_to_rid()).
*/
FSL_EXPORT int fsl_content_new( fsl_cx * f, fsl_uuid_cstr uuid, bool isPrivate,
                                fsl_id_t * newId );

/** @internal

    Check to see if checkin "rid" is a leaf and either add it to the LEAF
    table if it is, or remove it if it is not.

    Returns 0 on success, FSL_RC_MISUSE if !f or f has no repo db
    opened, FSL_RC_RANGE if pid is <=0. Other errors
    (e.g. FSL_RC_DB) may indicate that db is not a repo. On error
    db's error state may be updated.
*/
FSL_EXPORT int fsl_repo_leaf_check(fsl_cx * f, fsl_id_t pid);  

/** @internal

    Schedules a leaf check for "rid" and its parents. Returns 0 on
    success.
*/
FSL_EXPORT int fsl_repo_leaf_eventually_check( fsl_cx * f, fsl_id_t rid);

/** @internal

    Perform all pending leaf checks. Returns 0 on success or if it
    has nothing to do.
*/
FSL_EXPORT int fsl_repo_leaf_do_pending_checks(fsl_cx *f);

/** @internal

    Inserts a tag into f's repo db. It does not create the related
    control artifact - use fsl_tag_add_artifact() for that.

    rid is the artifact to which the tag is being applied.

    srcId is the artifact that contains the tag. It is often, but
    not always, the same as rid. This is often the RID of the
    manifest containing tags added as part of the commit, in which
    case rid==srcId. A Control Artifact which tags a different
    artifact will have rid!=srcId.

    mtime is the Julian timestamp for the tag. Defaults to the
    current time if mtime <= 0.0.

    If outRid is not NULL then on success *outRid is assigned the
    record ID of the generated tag (the tag.tagid db field).

    If a more recent (compared to mtime) entry already exists for
    this tag/rid combination then its tag.tagid is returned via
    *outRid (if outRid is not NULL) and no new entry is created.

    Returns 0 on success, and has a huge number of potential error
    codes.
*/
FSL_EXPORT int fsl_tag_insert( fsl_cx * f,
                    fsl_tagtype_e tagtype,
                    char const * zTag,
                    char const * zValue,
                    fsl_id_t srcId,
                    double mtime,
                    fsl_id_t rid,
                    fsl_id_t *outRid );
/** @internal

    Propagate all propagatable tags in artifact pid to the children of
    pid. Returns 0 on... non-error. Returns FSL_RC_RANGE if pid<=0.
*/
FSL_EXPORT int fsl_tag_propagate_all(fsl_cx * f, fsl_id_t pid);

/** @internal

    Propagates a tag through the various internal pipelines.

    pid is the artifact id to whose children the tag should be
    propagated.

    tagid is the id of the tag to propagate (the tag.tagid db value).

    tagType is the type of tag to propagate. Must be either
    FSL_TAGTYPE_CANCEL or FSL_TAGTYPE_PROPAGATING. Note that
    FSL_TAGTYPE_ADD is not permitted.  The tag-handling internals
    (other than this function) translate ADD to CANCEL for propagation
    purposes. A CANCEL tag is used to stop propagation. (That's a
    historical behaviour inherited from fossil(1).) A potential TODO
    is for this function to simply treat ADD as CANCEL, without
    requiring that the caller be sure to never pass an ADD tag.

    origId is the artifact id of the origin tag if tagType ==
    FSL_TAGTYPE_PROPAGATING, otherwise it is ignored.

    zValue is the optional value for the tag. May be NULL.

    mtime is the Julian timestamp for the tag. Must be a valid time
    (no defaults here).

    This function is unforgiving of invalid values/ranges, and may assert
    in debug mode if passed invalid ids (values<=0), a NULL f, or if f has
    no opened repo.
*/
FSL_EXPORT int fsl_tag_propagate(fsl_cx *f,
                                 fsl_tagtype_e tagType,
                                 fsl_id_t pid,
                                 fsl_id_t tagid,
                                 fsl_id_t origId,
                                 const char *zValue,
                                 double mtime );

/** @internal

    Remove the PGP signature from a raw artifact, if there is one.

    Expects *pz to point to *pn bytes of string memory which might
    or might not be prefixed by a PGP signature.  If the string is
    enveloped in a signature, then upon returning *pz will point to
    the first byte after the end of the PGP header and *pn will
    contain the length of the content up to, but not including, the
    PGP footer.

    If *pz does not look like a PGP header then this is a no-op.

    Neither pointer may be NULL and *pz must point to *pn bytes of
    valid memory. If *pn is initially less than 59, this is a no-op.
*/
FSL_EXPORT void fsl_remove_pgp_signature(unsigned char const **pz, fsl_size_t *pn);

/** @internal

    Clears the "seen" cache used by manifest parsing. Should be
    called by routines which initialize parsing, but not until their
    work has finished all parsing (so that recursive parsing can
    use it).
*/
FSL_EXPORT void fsl_cx_clear_mf_seen(fsl_cx * f);

/** @internal

    Generates an fsl_appendf()-formatted message to stderr and
    fatally aborts the application by calling exit(). This is only
    (ONLY!) intended for us as a placeholder for certain test cases
    and is neither thread-safe nor reantrant.

    fmt may be empty or NULL, in which case only the code and its
    fsl_rc_cstr() representation are output.

    This function does not return.
*/
FSL_EXPORT void fsl_fatal( int code, char const * fmt, ... )
#ifdef __GNUC__
  __attribute__ ((noreturn))
#endif
  ;

/** @internal

    Translate a normalized, repo-relative filename into a
    filename-id (fnid). Create a new fnid if none previously exists
    and createNew is true. On success returns 0 and sets *rv to the
    filename.fnid record value. If createNew is false and no match
    is found, 0 is returned but *rv will be set to 0. Returns non-0
    on error.  Results are undefined if any parameter is NULL.


    In debug builds, this function asserts that no pointer arguments
    are NULL and that f has an opened repository.
*/
FSL_EXPORT int fsl_repo_filename_fnid2( fsl_cx * f, char const * filename,
                             fsl_id_t * rv, bool createNew );


/** @internal

    Clears and frees all (char*) members of db but leaves the rest
    intact. If alsoErrorState is true then the error state is also
    freed, else it is kept as well.
*/
FSL_EXPORT void fsl_db_clear_strings(fsl_db * db, bool alsoErrorState );

/** @internal

    Returns 0 if db appears to have a current repository schema, 1
    if it appears to have an out of date schema, and -1 if it
    appears to not be a repository. Results are undefined if db is
    NULL or not opened.
*/
FSL_EXPORT int fsl_db_repo_verify_schema(fsl_db * db);


/** @internal

    Flags for APIs which add phantom blobs to the repository.  The
    values in this enum derive from fossil(1) code and should not be
    changed without careful forethought and (afterwards) testing.  A
    phantom blob is a blob about whose existence we know but for which
    we have no content. This normally happens during sync or rebuild
    operations, but can also happen when artifacts are stored directly
    as files in a repo (like this project's repository does, storing
    artifacts from *other* projects for testing purposes).
*/
enum fsl_phantom_e {
/**
   Indicates to fsl_uuid_to_rid2() that no phantom artifact
   should be created.
*/
FSL_PHANTOM_NONE = 0,
/**
   Indicates to fsl_uuid_to_rid2() that a public phantom
   artifact should be created if no artifact is found.
*/
FSL_PHANTOM_PUBLIC = 1,
/**
   Indicates to fsl_uuid_to_rid2() that a private phantom
   artifact should be created if no artifact is found.
*/
FSL_PHANTOM_PRIVATE = 2
};
typedef enum fsl_phantom_e fsl_phantom_e;

/** @internal

    Works like fsl_uuid_to_rid(), with these differences:

    - uuid is required to be a complete UUID, not a prefix.

    - If it finds no entry and the mode argument specifies so then
    it will add either a public or private phantom entry and return
    its new rid. If mode is FSL_PHANTOM_NONE then this this behaves
    just like fsl_uuid_to_rid().

    Returns a positive value on success, 0 if it finds no entry and
    mode==FSL_PHANTOM_NONE, and a negative value on error (e.g.  if
    fsl_is_uuid(uuid) returns false). Errors which happen after
    argument validation will "most likely" update f's error state
    with details.
*/
FSL_EXPORT fsl_id_t fsl_uuid_to_rid2( fsl_cx * f, fsl_uuid_cstr uuid,
                           fsl_phantom_e mode );

/** @internal

    Schedules the given rid to be verified at the next commit. This
    is used by routines which add artifact records to the blob
    table.

    The only error case, assuming the arguments are valid, is an
    allocation error while appending rid to the internal to-verify
    queue.

    @see fsl_repo_verify_at_commit()
    @see fsl_repo_verify_cancel()
*/
FSL_EXPORT int fsl_repo_verify_before_commit( fsl_cx * f, fsl_id_t rid );

/** @internal

    Clears f's verify-at-commit list of RIDs.

    @see fsl_repo_verify_at_commit()
    @see fsl_repo_verify_before_commit()
*/
FSL_EXPORT void fsl_repo_verify_cancel( fsl_cx * f );

/** @internal

    Processes all pending verify-at-commit entries and clears the
    to-verify list. Returns 0 on success. On error f's error state
    will likely be updated.

    ONLY call this from fsl_db_transaction_end() or its delegate (if
    refactored).

    Verification calls fsl_content_get() to "unpack" content added in
    the current transaction. If fetching the content (which applies
    any deltas it may need to) fails or a checksum does not match then
    this routine fails and returns non-0. On error f's error state
    will be updated.

    @see fsl_repo_verify_cancel()
    @see fsl_repo_verify_before_commit()
*/
FSL_EXPORT int fsl_repo_verify_at_commit( fsl_cx * f );

/** @internal

    Removes all entries from the repo's blob table which are listed
    in the shun table. Returns 0 on success. This operation is
    wrapped in a transaction. Delta contant which depend on
    to-be-shunned content are replaced with their undeltad forms.

    Returns 0 on success.
*/
FSL_EXPORT int fsl_repo_shun_artifacts(fsl_cx * f);

/** @internal.

    Return a pointer to a string that contains the RHS of an SQL IN
    operator which will select config.name values that are part of
    the configuration that matches iMatch (a bitmask of
    fsl_configset_e values). Ownership of the returned string is
    passed to the caller, who must eventually pass it to
    fsl_free(). Returns NULL on allocation error.

    Reminder to self: this is part of the infrastructure for copying
    config state from an existing repo when creating new repo.
*/
FSL_EXPORT char *fsl_config_inop_rhs(int iMask);

/** @internal

    Return a pointer to a string that contains the RHS of an IN
    operator that will select config.name values that are in the
    list of control settings. Ownership of the returned string is
    passed to the caller, who must eventually pass it to
    fsl_free(). Returns NULL on allocation error.

    Reminder to self: this is part of the infrastructure for copying
    config state from an existing repo when creating new repo.
*/
FSL_EXPORT char *fsl_db_setting_inop_rhs();

/** @internal

    Creates the ticket and ticketchng tables in f's repository db,
    DROPPING them if they already exist. The schema comes from
    fsl_schema_ticket().

    FIXME: add a flag specifying whether to drop or keep existing
    copies.

    Returns 0 on success.
*/
FSL_EXPORT int fsl_cx_ticket_create_table(fsl_cx * f);

/** @internal

    Frees all J-card entries in the given list.

    li is assumed to be empty or contain (fsl_card_J*)
    instances. If alsoListMem is true then any memory owned
    by li is also freed. li itself is not freed.

    Results are undefined if li is NULL.
*/
FSL_EXPORT void fsl_card_J_list_free( fsl_list * li, bool alsoListMem );

/** @internal

    Values for fsl_card_J::flags.
*/
enum fsl_card_J_flags {
/**
   Sentinel value.
*/
FSL_CARD_J_INVALID = 0,
/**
   Indicates that the field is used by the ticket table.
*/
FSL_CARD_J_TICKET = 0x01,
/**
   Indicates that the field is used by the ticketchng table.
*/
FSL_CARD_J_CHNG = 0x02,
/**
   Indicates that the field is used by both the ticket and
   ticketchng tables.
*/
FSL_CARD_J_BOTH = FSL_CARD_J_TICKET | FSL_CARD_J_CHNG
};

/** @internal

    Loads all custom/customizable ticket fields from f's repo's
    ticket table info f. If f has already loaded the list and
    forceReload is false, this is a no-op.

    Returns 0 on success.

    @see fsl_cx::ticket::customFields
*/
FSL_EXPORT int fsl_cx_ticket_load_fields(fsl_cx * f, bool forceReload);

/** @internal

    A comparison routine for qsort(3) which compares fsl_card_J
    instances in a lexical manner based on their names. The order is
    important for card ordering in generated manifests.

    This routine expects to get passed (fsl_card_J**) (namely from
    fsl_list entries), and will not work on an array of J-cards.
*/
FSL_EXPORT int fsl_qsort_cmp_J_cards( void const * lhs, void const * rhs );

/** @internal

    The internal version of fsl_deck_parse(). See that function
    for details regarding everything but the 3rd argument.

    If you happen to know the _correct_ RID for the deck being
    parsed, pass it as the rid argument, else pass 0. A negative
    value will result in a FSL_RC_RANGE error. This value is (or
    will be) only used as an optimization in other places and only
    if d->f is not NULL.  Passing a positive value has no effect on
    how the content is parsed or on the result - it only affects
    internal details/optimizations.
*/
FSL_EXPORT int fsl_deck_parse2(fsl_deck * d, fsl_buffer * src, fsl_id_t rid);

/** @internal

    This function updates the repo and/or global config databases
    with links between the dbs intended for various fossil-level
    bookkeeping and housecleaning. These links are not essential to
    fossil's functionality but assist in certain "global"
    operations.

    If no checkout is opened but a repo is, the global config (if
    opened) is updated to know about the opened repo db.

    If a checkout is opened, global config (if opened) and the
    repo are updated to point to the checked-out db.
*/
FSL_EXPORT int fsl_repo_record_filename(fsl_cx * f);

/** @internal

    Updates f->ckout.uuid and f->ckout.rid to reflect the current
    checkout state. If no checkout is opened, the uuid is freed/NULLed
    and the rid is set to 0. Returns 0 on success. If it returns an
    error (OOM or db-related), the f->ckout state is left in a
    potentially inconsistent state, and it should not be relied upon
    until/unless the error is resolved.

    This is done when a checkout db is opened, when performing a
    checkin, and otherwise as needed, and so calling it from other
    code is normally not necessary.

    @see fsl_ckout_version_write()
*/
FSL_EXPORT int fsl_ckout_version_fetch( fsl_cx *f );

/** @internal

    Updates f->ckout's state to reflect the given version info and
    writes the 'checkout' and 'checkout-hash' properties to the
    currently-opened checkout db. Returns 0 on success,
    FSL_RC_NOT_A_CKOUT if no checkout is opened (may assert() in that
    case!), or some other code if writing to the db fails.

    If vid is 0 then the version info is null'd out. Else if uuid is
    NULL then fsl_rid_to_uuid() is used to fetch the UUID for vid.

    If the RID differs from f->ckout.rid then f->ckout's version state
    is updated to the new values.

    This routine also updates or removes the checkout's manifest
    files, as per fsl_ckout_manifest_write(). If vid is 0 then it
    removes any such files which themselves are not part of the
    current checkout.

    @see fsl_ckout_version_fetch()
    @see fsl_cx_ckout_version_set()
*/
FSL_EXPORT int fsl_ckout_version_write( fsl_cx *f, fsl_id_t vid,
                                        fsl_uuid_cstr uuid );

/**
   @internal
   
   Exports the file with the given [vfile].[id] to the checkout,
   overwriting (if possible) anything which gets in its way. If
   the file is determined to have not been modified, it is
   unchanged.

   If the final argument is not NULL then it is set to 0 if the file
   was not modified, 1 if only its permissions were modified, and 2 if
   its contents were updated (which also requires resetting its
   permissions to match their repo-side state).

   Returns 0 on success, any number of potential non-0 codes on
   error, including, but not limited to:

   - FSL_RC_NOT_A_CKOUT - no opened checkout.
   - FSL_RC_NOT_FOUND - no matching vfile entry.
   - FSL_RC_OOM - we cannot escape this eventuality.

   Trivia:

   - fossil(1)'s vfile_to_disk() is how it exports a whole vfile, or a
   single vfile entry, to disk. e.g. it performs a checkout that way,
   whereas we currently perform a checkout using the "repo extraction"
   API. The checkout mechanism was probably the first major core
   fossil feature which was structured radically differently in
   libfossil, compared to the feature's fossil counterpart, when it
   was ported over.

   - This routine always writes to the vfile.pathname entry, as
   opposed to vfile.origname.

   Maintenance reminders: internally this code supports handling
   multiple files at once, but (A) that's not part of the interface
   and may change and (B) the 3rd parameter makes little sense in that
   case unless maybe we change it to a callback, which seems like
   overkill for our use cases.
*/
FSL_EXPORT int fsl_vfile_to_ckout(fsl_cx * f, fsl_id_t vfileId,
                                  int * wasWritten);

/** @internal

    On Windows platforms (only), if fsl_isalpha(*zFile)
    and ':' == zFile[1] then this returns zFile+2,
    otherwise it returns zFile.
*/
FSL_EXPORT char * fsl_file_without_drive_letter(char * zFile);

/** @internal

    This is identical to the public-API member fsl_deck_F_search(),
    except that it returns a non-const F-card.

    Locate a file named zName in d->F.list.  Return a pointer to the
    appropriate fsl_card_F object. Return NULL if not found.
   
    If d->f is set (as it is when loading decks via
    fsl_deck_load_rid() and friends), this routine works even if p is
    a delta-manifest. The pointer returned might be to the baseline
    and d->B.baseline is loaded on demand if needed.
   
    If the returned card's uuid member is NULL, it means that the file
    was removed in the checkin represented by d.

    If !d, zName is NULL or empty, or FSL_SATYPE_CHECKIN!=d->type, it
    asserts in debug builds and returns NULL in non-debug builds.

    We assume that filenames are in sorted order and use a binary
    search. As an optimization, to support the most common use case,
    searches through a deck update d->F.cursor to the last position a
    search was found. Because searches are normally done in lexical
    order (because of architectural reasons), this is normally an O(1)
    operation. It degrades to O(N) if out-of-lexical-order searches
    are performed.
*/
FSL_EXPORT fsl_card_F * fsl_deck_F_seek(fsl_deck * const d, const char *zName);

/** @internal

    Part of the fsl_cx::fileContent optimization. This sets
    f->fileContent.used to 0 and if its capacity is over a certain
    (unspecified, unconfigurable) size then it is trimmed to that
    size.
*/
FSL_EXPORT void fsl_cx_content_buffer_yield(fsl_cx * f);

/** @internal

   Currently disabled (always returns 0) pending resolution of a
   "wha???" result from one of the underlying queries.

   Queues up the given artifact for a search index update. This is
   only intended to be called from crosslinking steps and similar
   content updates. Returns 0 on success.

   The final argument is intended only for wiki titles (the L-card of
   a wiki post).

   If the repository database has no search index or the given content
   is marked as private, this function returns 0 and makes no changes
   to the db.
*/
FSL_EXPORT int fsl_search_doc_touch(fsl_cx *f, fsl_satype_e saType, fsl_id_t rid,
                                    const char * docName);

/** @internal

   Performs the same job as fsl_diff_text() but produces the results
   in the low-level form of an array of "copy/delete/insert triples."
   This is primarily intended for internal use in other
   library-internal algorithms, not for client code. Note all
   FSL_DIFF_xxx flags apply to this form.

   Returns 0 on success, any number of non-0 codes on error. On
   success *outRaw will contain the resulting array, which must
   eventually be fsl_free()'d by the caller. On error *outRaw is not
   modified.
*/
FSL_EXPORT int fsl_diff_text_raw(fsl_buffer const *p1, fsl_buffer const *p2,
                                 int diffFlags, int ** outRaw);

/** @internal

   If the given file name is a reserved filename (case-insensitive) on
   Windows platforms, a pointer to the reserved part of the name, else
   NULL is returned.

   zPath must be a canonical path with forward-slash directory
   separators. nameLen is the length of zPath. If negative, fsl_strlen()
   is used to determine its length.
*/
FSL_EXPORT bool fsl_is_reserved_fn_windows(const char *zPath, fsl_int_t nameLen);

/** @internal

   Clears any pending merge state from the checkout db's vmerge table.
   Returns 0 on success.
*/
FSL_EXPORT int fsl_ckout_clear_merge_state( fsl_cx *f );


/** @internal

   Installs or reinstalls the checkout database schema into f's open
   checkout db. Returns 0 on success, FSL_RC_NOT_A_CKOUT if f has
   no opened checkout, or an code if a lower-level operation fails.

   If dropIfExists is true then all affected tables are dropped
   beforehand if they exist. "It's the only way to be sure."

   If dropIfExists is false and the schema appears to already exists
   (without actually validating its validity), 0 is returned.
*/
FSL_EXPORT int fsl_ckout_install_schema(fsl_cx *f, bool dropIfExists);

/** @internal

   Attempts to remove empty directories from under a checkout,
   starting with tgtDir and working upwards until it either cannot
   remove one or it reaches the top of the checkout dir.

   The second argument must be the canonicalized absolute path to some
   directory under the checkout root. The contents of the buffer may,
   for efficiency's sake, be modified by this routine as it traverses
   the directory tree. It will never grow the buffer but may mutate
   its memory's contents.

   Returns the number of directories it is able to remove.

   Results are undefined if tgtDir is not an absolute path rooted in
   f's current checkout.

   There are any number of valid reasons removal of a directory might
   fail, and this routine stops at the first one which does.
*/
FSL_EXPORT unsigned int fsl_ckout_rm_empty_dirs(fsl_cx * f, fsl_buffer * tgtDir);

/** @internal

   This is intended to be passed the name of a file which was just
   deleted and "might" have left behind an empty directory. The name
   _must_ an absolute path based in f's current checkout. This routine
   uses fsl_file_dirpart() to strip path components from the string
   and remove directories until either removing one fails or the top
   of the checkout is reach. Since removal of a directory can fail for
   any given reason, this routine ignores such errors. It returns 0 on
   success, FSL_RC_OOM if allocation of the working buffer for the
   filename hackery fails, and FSL_RC_MISUSE if zFilename is not
   rooted in the checkout (in which case it may assert(), so don't do
   that).

   @see fsl_is_rooted_in_ckout()
   @see fsl_rm_empty_dirs()
*/
FSL_EXPORT int fsl_ckout_rm_empty_dirs_for_file(fsl_cx * f, char const *zAbsPath);

/** @internal

    If f->cache.seenDeltaManifest<=0 then this routine sets it to 1
    and sets the 'seen-delta-manifest' repository config setting to 1,
    else this has no side effects. Returns 0 on success, non-0 if
    there is an error while writing to the repository config.
*/
FSL_EXPORT int fsl_cx_update_seen_delta_mf(fsl_cx *f);

/** @internal

   Very, VERY internal.

   Returns the next available buffer from f->scratchpads. Fatally
   aborts if there are no free buffers because "that should not
   happen."  Calling this obligates the caller to eventually pass
   its result to fsl_cx_scratchpad_yield().

   This function guarantees the returned buffer's 'used' member will be
   set to 0.

   Maintenance note: the number of buffers is hard-coded in the
   fsl_cx::scratchpads anonymous struct.
*/
FSL_EXPORT fsl_buffer * fsl_cx_scratchpad(fsl_cx *f);

/** @internal

   Very, VERY internal.

   "Yields" a buffer which was returned from fsl_cx_scratchpad(),
   making it available for re-use. The caller must treat the buffer as
   if this routine frees it: using the buffer after having passed it
   to this function will internally be flagged as explicit misuse and
   will lead to a fatal crash the next time that buffer is fetched via
   fsl_cx_scratchpad(). So don't do that.
*/
FSL_EXPORT void fsl_cx_scratchpad_yield(fsl_cx *f, fsl_buffer * b);


/** @internal

   Run automatically by fsl_deck_save(), so it needn't normally be run
   aside from that, at least not from average client code.

   Runs postprocessing on the Structural Artifact represented by
   d. d->f must be set, d->rid must be set and valid and d's contents
   must accurately represent the stored manifest for the given
   rid. This is normally run just after the insertion of a new
   manifest, but is sometimes also run after reading a deck from the
   database (in order to rebuild all db relations and add/update the
   timeline entry).

   Returns 0 on succes, FSL_RC_MISUSE if !d or !d->f, FSL_RC_RANGE if
   d->rid<=0, FSL_RC_MISUSE (with more error info in f) if d does not
   contain all required cards for its d->type value. It may return
   various other codes from the many routines it delegates work to.

   Crosslinking of ticket artifacts is currently (2021-03) missing.

   Design note: d "really should" be const here but some internals
   (d->F.cursor and delayed baseline loading) prohibit it.

   @see fsl_deck_crosslink_one()
*/
FSL_EXPORT int fsl_deck_crosslink( fsl_deck /* const */ * d );

/** @internal

   Run automatically by fsl_deck_save(), so it needn't normally be run
   aside from that, at least not from average client code.

   This is a convience form of crosslinking which must only be used
   when a single deck (and only a single deck) is to be crosslinked.
   This function wraps the crosslinking in fsl_crosslink_begin()
   and fsl_crosslink_end(), but otherwise behaves the same as
   fsl_deck_crosslink(). If crosslinking fails, any in-progress
   transaction will be flagged as failed.

   Returns 0 on success.
*/
FSL_EXPORT int fsl_deck_crosslink_one( fsl_deck * d );

/** @internal

   Checks whether the given filename is "safe" for writing to within
   f's current checkout.

   zFilename must be in canonical form: only '/' directory separators.
   If zFilename is not absolute, it is assumed to be relative to the top
   of the current checkout, else it must point to a file under the current
   checkout.

   Checks made on the filename include:

   - It must refer to a file under the current checkout.

   - Ensure that each directory listed in the file's path is actually
   a directory, and fail if any part other than the final one is a
   non-directory.

   If the name refers to something not (yet) in the filesystem, that
   is not considered an error.

   Returns 0 on success. On error f's error state is updated with
   information about the problem.
*/
FSL_EXPORT int fsl_ckout_safe_file_check(fsl_cx *f, char const * zFilename);

/** @internal
   UNTESTED!

   Creates a file named zLinkFile and populates its contents with a
   single line: zTgtFile. This behaviour corresponds to how fossil
   manages SCM'd symlink entries on Windows and on other platforms
   when the 'allow-symlinks' repo-level config setting is disabled.
   (In late 2020 fossil defaulted that setting to disabled and made it
   non-versionable.)

   zLinkFile may be an absolute path rooted at f's current checkout or
   may be a checkout-relative path.

   Returns 0 on success, non-0 on error:

   - FSL_RC_NOT_A_CKOUT if f has no opened checkout.

   - FSL_RC_MISUSE if zLinkFile refers to a path outside of the
   current checkout.

   Potential TODO (maybe!): handle symlinks as described above or
   "properly" on systems which natively support them iff f's
   'allow-symlinks' repo-level config setting is true. That said: the
   addition of symlinks support into fossil was, IMHO, a poor decision
   for $REASONS. That might (might) be reflected long-term in this API
   by only supporting them in the way fossil does for platforms which
   do not support symlinks.
*/
FSL_EXPORT int fsl_ckout_symlink_create(fsl_cx * f, char const *zTgtFile,
                                        char const * zLinkFile);


/**
   Compute all file name changes that occur going from check-in iFrom
   to check-in iTo. Requires an opened repository.

   If revOK is true, the algorithm is free to move backwards in the
   chain. This is the opposite of the oneWayOnly parameter for
   fsl_vpath_shortest().

   On success, the number of name changes is written into *pnChng.
   For each name change, two integers are allocated for *piChng. The
   first is the filename.fnid for the original name as seen in
   check-in iFrom and the second is for new name as it is used in
   check-in iTo. If *pnChng is 0 then *aiChng will be NULL.

   On error returns non-0, pnChng and aiChng are not modified, and
   f's error state might (depending on the error) contain a description
   of the problem.

   Space to hold *aiChng is obtained from fsl_malloc() and must
   be released by the caller.
*/
FSL_EXPORT int fsl_cx_find_filename_changes(fsl_cx * f,
                                            fsl_id_t iFrom,
                                            fsl_id_t iTo,
                                            bool revOK,
                                            uint32_t *pnChng,
                                            fsl_id_t **aiChng);

/**
   Bitmask of file change types for use with
   fsl_is_locally_modified().
 */
enum fsl_localmod_e {
/** Sentinel value. */
FSL_LOCALMOD_NONE = 0,
/**
   Permissions changed.
*/
FSL_LOCALMOD_PERM = 0x01,
/**
   File size or hash (i.e. content) differ.
*/
FSL_LOCALMOD_CONTENT = 0x02,
/**
   The file type was switched between symlink and normal file.  In
   this case, no check for content change, beyond the file size
   change, is performed.
*/
FSL_LOCALMOD_LINK = 0x04,
/**
   File was not found in the local checkout.
 */
FSL_LOCALMOD_NOTFOUND = 0x10
};
typedef enum fsl_localmod_e fsl_localmod_e;
/** @internal

   Checks whether the given file has been locally modified compared to
   a known size, hash value, and permissions. Requires that f has an
   opened checkout.

   If zFilename is not an absolute path, it is assumed to be relative
   to the checkout root (as opposed to the current directory) and is
   canonicalized into an absolute path for purposes of this function.

   fileSize is the "original" version's file size.  zOrigHash is the
   initial hash of the file to use as a basis for comparison.
   zOrigHashLen is the length of zOrigHash, or a negative value if
   this function should use fsl_is_uuid() to determine the length. If
   the hash length is not that of one of the supported hash types,
   FSL_RC_RANGE is returned and f's error state is updated. This
   length is used to determine which hash to use for the comparison.

   If the file's current size differs from the given size, it is
   quickly considered modified, otherwise the file's contents get
   hashed and compared to zOrigHash.

   Because this is used for comparing local files to their state from
   the fossil database, where files have no timestamps, the local
   file's timestamp is never considered for purposes of modification
   checking.

   If isModified is not NULL then on success it is set to a bitmask of
   values from the fsl_localmod_e enum specifying the type(s) of
   change(s) detected:

   - FSL_LOCALMOD_PERM = permissions changed.

   - FSL_LOCALMOD_CONTENT = file size or hash (i.e. content) differ.

   - FSL_LOCALMOD_LINK = the file type was switched between symlink
     and normal file. In this case, no check for content change,
     beyond the file size change, is performed.

   - FSL_LOCALMOD_NOFOUND = file was not found in the local checkout.

   Noting that:

   - Combined values of (FSL_LOCALMOD_PERM | FSL_LOCALMOD_CONTENT) are
   possible, but FSL_LOCALMOD_NOFOUND will never be combined with one
   of the other values.

   If stat() fails for any reason other than file-not-found
   (e.g. permissions), an error is triggered.

   Returns 0 on success. On error, returns non-0 and f's error state
   will be updated and isModified...  isNotModified. Errors include,
   but are not limited to:

   - Invalid hash length: FSL_RC_RANGE
   - f has no opened checkout: FSL_RC_NOT_A_CKOUT
   - Cannot find the file: FSL_RC_NOT_FOUND
   - Error accessing the file: FSL_RC_ACCESS
   - Allocation error: FSL_RC_OOM
   - I/O error during hashing: FSL_RC_IO

   And potentially other errors, roughly translated from errno values,
   for corner cases such as passing a directory name instead of a
   file.

   Results are undefined if any pointer argument is NULL or invalid.

   This function currently does NOT follow symlinks for purposes of
   resolving zFilename, but that behavior may change in the future or
   may become dependent on the repository's 'allow-symlinks' setting.

   Internal detail, not relevant for clients: this updates f's
   cache stat entry.
*/
FSL_EXPORT int fsl_is_locally_modified(fsl_cx * f,
                                       const char * zFilename,
                                       fsl_size_t fileSize,
                                       const char * zOrigHash,
                                       fsl_int_t zOrigHashLen,
                                       fsl_fileperm_e origPerm,
                                       int * isModified);

/** @internal

   This routine cleans up the state of selected cards in the given
   deck. The 2nd argument is an list of upper-case letters
   representing the cards which should be cleaned up, e.g. "ADG". If
   it is NULL, all cards are cleaned up but d has non-card state
   which is not cleaned up by this routine. Unknown letters are simply
   ignored.
*/
FSL_EXPORT void fsl_deck_clean_cards(fsl_deck * d, char const * letters);

/** @internal

   Searches the current repository database for a fingerprint and
   returns it as a string in *zOut.

   If rcvid<=0 then the fingerprint matches the last entry in the
   [rcvfrom] table, where "last" means highest-numbered rcvid (as
   opposed to most recent mtime, for whatever reason). If rcvid>0 then
   it searches for an exact match.

   If oldVersion is set, use the original Fossil fingerprint algorithm
   (i.e., quote(mtime) sans datetime(mtime) in the SQL query), this may
   be required on older Fossil repositories. It is suggested to call
   this routine first without oldVersion set, and if the subsequent
   fingerprint comparison fails, try again with oldVersion set.

   Returns 0 on non-error, where finding no matching rcvid causes
   FSL_RC_NOT_FOUND to be returned. If 0 is returned then *zOut will
   be non-NULL and ownership of that value is transferred to the
   caller, who must eventually pass it to fsl_free(). On error, *zOut
   is not modified.

   Returns FSL_RC_NOT_A_REPO if f has no opened repository, FSL_RC_OOM
   on allocation error, or any number of potential db-related codes if
   something goes wrong at the db level.

   This API does not support the "version 0" fossil fingerprint. That
   one was very short-lived and is not expected to be in any/many
   repositories which are accessed via this library.

   @see fsl_ckout_fingerprint_check()
*/
FSL_EXPORT int fsl_repo_fingerprint_search(fsl_cx *f, fsl_id_t rcvid,
		char ** zOut, bool oldVersion);

#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_INTERNAL_H_INCLUDED */
