/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_REPO_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_REPO_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).
*/

/** @file fossil-repo.h

    fossil-repo.h declares APIs specifically dealing with
    repository-db-side state, as opposed to specifically checkout-side
    state or non-content-related APIs.
*/

#include "fossil-db.h" /* MUST come first b/c of config macros */
#include "fossil-hash.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct fsl_card_F fsl_card_F;
typedef struct fsl_card_J fsl_card_J;
typedef struct fsl_card_Q fsl_card_Q;
typedef struct fsl_card_T fsl_card_T;
typedef struct fsl_checkin_opt fsl_checkin_opt;
typedef struct fsl_deck fsl_deck;

/**
   This function is a programmatic interpretation of
   this table:

   https://fossil-scm.org/index.html/doc/trunk/www/fileformat.wiki#summary

   For a given control artifact type and a card name in the form of
   the card name's letter (e.g. 'A', 'W', ...), this function
   returns 0 (false) if that card type is not permitted in this
   control artifact type, a negative value if the card is optional
   for this artifact type, and a positive value if the card type is
   required for this artifact type.

   As a special case, if t==FSL_SATYPE_ANY then this function
   always returns a negative value as long as card is a valid card
   letter.

   Another special case: when t==FSL_SATYPE_CHECKIN and card=='F',
   this returns a negative value because the table linked to above
   says F-cards are optional. In practice we have yet to find a use
   for checkins with no F-cards, so this library currently requires
   F-cards at checkin-time even though this function reports that
   they are optional.
*/
FSL_EXPORT int fsl_card_is_legal( fsl_satype_e t, char card );

/**
   Artifact tag types used by the Fossil framework. Their values
   are a hard-coded part of the Fossil format, and not subject to
   change (only extension, possibly).
*/
enum fsl_tagtype_e {
/**
   Sentinel value for use with constructors/initializers.
*/
FSL_TAGTYPE_INVALID = -1,
/**
   The "cancel tag" indicator, a.k.a. an anti-tag.
*/
FSL_TAGTYPE_CANCEL = 0,
/**
   The "add tag" indicator, a.k.a. a singleton tag.
*/
FSL_TAGTYPE_ADD = 1,
/**
   The "propagating tag" indicator.
*/
FSL_TAGTYPE_PROPAGATING = 2
};
typedef enum fsl_tagtype_e fsl_tagtype_e;

/**
   Hard-coded IDs used by the 'tag' table of repository DBs. These
   values get installed as part of the base Fossil db schema in new
   repos, and they must never change.
*/
enum fsl_tagid_e {
/**
   DB string tagname='bgcolor'.
*/
FSL_TAGID_BGCOLOR = 1,
/**
   DB: tag.tagname='comment'.
*/
FSL_TAGID_COMMENT = 2,
/**
   DB: tag.tagname='user'.
*/
FSL_TAGID_USER = 3,
/**
   DB: tag.tagname='date'.
*/
FSL_TAGID_DATE = 4,
/**
   DB: tag.tagname='hidden'.
*/
FSL_TAGID_HIDDEN = 5,
/**
   DB: tag.tagname='private'.
*/
FSL_TAGID_PRIVATE = 6,
/**
   DB: tag.tagname='cluster'.
*/
FSL_TAGID_CLUSTER = 7,
/**
   DB: tag.tagname='branch'.
*/
FSL_TAGID_BRANCH = 8,
/**
   DB: tag.tagname='closed'.
*/
FSL_TAGID_CLOSED = 9,
/**
   DB: tag.tagname='parent'.
*/
FSL_TAGID_PARENT = 10,
/**
   DB: tag.tagname='note'

   Extra text appended to a check-in comment.
*/
FSL_TAGID_NOTE = 11,

/**
   Largest tag ID reserved for internal use.
*/
FSL_TAGID_MAX_INTERNAL = 99
};


/**
   Returns one of '-', '+', or '*' for a valid input parameter, 0
   for any other value.
*/
FSL_EXPORT char fsl_tag_prefix_char( fsl_tagtype_e t );


/**
   A list of fsl_card_F objects. F-cards used a custom list type,
   instead of the framework's generic fsl_list, because experience has
   shown that the number of (de)allocations we need for F-card lists
   has a large negative impact when parsing and creating artifacts en
   masse. This list type, unlike fsl_list, uses a conventional object
   array approach to storage, as opposed to an array of pointers (each
   entry of which has to be separately allocated).

   These lists, and F-cards in generally, are typically maintained
   internally in the library. There's probably "no good reason" for
   clients to manipulate them.
*/
struct fsl_card_F_list {
  /**
     The list of F-cards. The first this->used elements are in-use.
     This pointer may change any time the list is reallocated.a
  */
  fsl_card_F * list;
  /**
     The number of entries in this->list which are in use.
  */
  uint32_t used;
  /**
     The number of entries currently allocated in this->list.
  */
  uint32_t capacity;
  /**
     An internal cursor into this->list, used primarily for
     properly traversing the file list in delta manifests.

     Maintenance notes: internal updates to this member are the only
     reason some of the deck APIs require a non-const deck. This type
     needs to be signed for compatibility with some of the older
     algos, e.g. fsl_deck_F_seek_base().
  */
  int32_t cursor;
  /**
     Internal flags. Never, ever modify these from client code.
  */
  uint32_t flags;
};
typedef struct fsl_card_F_list fsl_card_F_list;
/** Empty-initialized fsl_card_F instance for const copy
    initialization */
#define fsl_card_F_list_empty_m {NULL, 0, 0, 0, 0}
/** Empty-initialized fsl_card_F instance for non-const copy
    initialization */
FSL_EXPORT const fsl_card_F_list fsl_card_F_list_empty;

/**
   A "deck" stores (predictably enough) a collection of "cards."
   Cards are constructs embedded within Fossil's Structural Artifacts
   to denote various sorts of changes in a Fossil repository, and a
   Deck encapsulates the cards for a single Structural Artifact of an
   arbitrary type, e.g. Manifest (a.k.a. "checkin") or Cluster. A card
   is basically a command with a single-letter name and a well-defined
   signature for its arguments. Each card is represented by a member
   of this struct whose name is the same as the card type
   (e.g. fsl_card::C holds a C-card and fsl_card::F holds a list of
   F-card). Each type of artifact only allows certain types of
   card. The complete list of valid card/construct combinations can be
   found here:

   https://fossil-scm.org/home/doc/trunk/www/fileformat.wiki#summary

   fsl_card_is_legal() can be used determine if a given card type
   is legal (per the above chart) with a given Control Artifiact
   type (as stored in the fsl_deck::type member).

   The type member is used by some algorithms to determine which
   operations are legal on a given artifact type, so that they can
   fail long before the user gets a chance to add a malformed artifact
   to the database. Clients who bypass the fsl_deck APIs and
   manipulate the deck's members "by hand" (so to say) effectively
   invoke undefined behaviour.

   The various routines to add/set cards in the deck are named
   fsl_deck_CARDNAME_add() resp. fsl_deck_CARDNAME_set(). The "add"
   functions represent cards which may appear multiple times
   (e.g. the 'F' card) or have multiple values (the 'P' card), and
   those named "set" represent unique or optional cards. The R-card
   is the outlier, with fsl_deck_R_calc(). NEVER EVER EVER directly
   modify a member of this struct - always use the APIs. The
   library performs some optimizations which can lead to corrupt
   memory and invalid free()s if certain members' values are
   directly replaced by the client (as opposed to via the APIs).

   Note that the 'Z' card is not in this structure because it is a
   hash of the other inputs and is calculated incrementally and
   appended automatically by fsl_deck_output().

   All non-const pointer members of this structure are owned by the
   structure instance unless noted otherwise (the fsl_deck::f member
   being the notable exception).

   Maintenance reminder: please keep the card members alpha sorted to
   simplify eyeball-searching through their docs.

   @see fsl_deck_malloc()
   @see fsl_deck_init()
   @see fsl_deck_parse()
   @see fsl_deck_load_rid()
   @see fsl_deck_finalize()
   @see fsl_deck_clean()
   @see fsl_deck_save()
   @see fsl_deck_A_set()
   @see fsl_deck_B_set()
   @see fsl_deck_D_set()
   @see fsl_deck_E_set()
   @see fsl_deck_F_add()
   @see fsl_deck_J_add()
   @see fsl_deck_K_set()
   @see fsl_deck_L_set()
   @see fsl_deck_M_add()
   @see fsl_deck_N_set()
   @see fsl_deck_P_add()
   @see fsl_deck_Q_add()
   @see fsl_deck_R_set()
   @see fsl_deck_T_add()
   @see fsl_deck_branch_set()
   @see fsl_deck_U_set()
   @see fsl_deck_W_set()
*/
struct fsl_deck {
  /**
     Specifies the the type (or eventual type) of this
     artifact. The function fsl_card_is_legal() can be used to
     determined if a given card type is legal for a given value of
     this member. APIs which add/set cards use that to determine if
     the operation requested by the client is semantically legal.
  */
  fsl_satype_e type;

  /**
     DB repo.blob.rid value. Normally set by fsl_deck_parse().
  */
  fsl_id_t rid;

  /**
     Gets set by fsl_deck_parse() to the hash/UUID of the
     manifest it parsed.
  */
  fsl_uuid_str uuid;

  /**
     The Fossil context responsible for this deck. Though this data
     type is normally, at least conceptually, free of any given fossil
     context, many related algorithms need a context in order to
     perform db- or caching-related work, as well as to simplify error
     message propagation. We store this as a struct member to keep all
     such algorithms from redundantly requiring both pieces of
     information as arguments.

     This object does not own the context and the context object must
     outlive this deck instance.
  */
  fsl_cx * f;

  /**
     The 'A' (attachment) card. Only used by FSL_SATYPE_ATTACHMENT
     decks. The spec currently specifies only 1 A-card per
     manifest, but conceptually this could/should be a list.
  */
  struct {
    /**
       Filename of the A-card.
    */
    char * name;

    /**
       Name of wiki page, or UUID of ticket or event (technote), to
       which the attachment applies.
    */
    char * tgt;

    /**
       UUID of the file being attached via the A-card.
    */
    fsl_uuid_str src;
  } A;

  struct {
    /**
       The 'B' (baseline) card holds the UUID of baseline manifest.
       This is empty for baseline manifests and holds the UUID of
       the parent for delta manifests.
    */
    fsl_uuid_str uuid;

    /**
       Baseline manifest corresponding to this->B. It is loaded on
       demand by routines which need it, typically by calling
       fsl_deck_F_rewind() (unintuitively enough!). The
       parent/child relationship in Fossil is the reverse of
       conventional - children own their parents, not the other way
       around. i.e. this->baseline will get cleaned up
       (recursively) when this instance is cleaned up (when the
       containing deck is cleaned up).
    */
    fsl_deck * baseline;
  } B;
  /**
     The 'C' (comment) card.
  */
  char * C;

  /**
     The 'D' (date) card, in Julian format.
  */
  double D;

  /**
     The 'E' (event) card.
  */
  struct {
    /**
       The 'E' card's date in Julian Day format.
    */
    double julian;

    /**
       The 'E' card's UUID.
    */
    fsl_uuid_str uuid;
  } E;

  /**
     The 'F' (file) card container.
  */
  fsl_card_F_list F;
  
  /**
     UUID for the 'G' (forum thread-root) card.
  */
  fsl_uuid_str G;

  /**
     The H (forum title) card.
  */
  char * H;

  /**
     UUID for the 'I' (forum in-response-to) card.
  */
  fsl_uuid_str I;

  /**
     The 'J' card specifies changes to "value" of "fields" in
     tickets (FSL_SATYPE_TICKET).

     Holds (fsl_card_J*) entries.
  */
  fsl_list J;

  /**
     UUID for the 'K' (ticket) card.
  */
  fsl_uuid_str K;

  /**
     The 'L' (wiki name/title) card.
  */
  char * L;

  /**
     List of UUIDs (fsl_uuid_str) in a cluster ('M' cards).
  */
  fsl_list M;

  /**
     The 'N' (comment mime type) card. Note that this is only
     ostensibly supported by fossil, but fossil does not (as of
     2021-04-13) honor this value and always assumes that its value is
     "text/x-fossil-wiki".
  */
  char * N;

  /**
     List of UUIDs of parents ('P' cards). Entries are of type
     (fsl_uuid_str).
  */
  fsl_list P;

  /**
     'Q' (cherry pick) cards. Holds (fsl_card_Q*) entries.
  */
  fsl_list Q;

  /**
     The R-card holds an MD5 hash which is calculated based on the
     names, sizes, and contents of the files included in a
     manifest. See the class-level docs for a link to a page which
     describes how this is calculated.
  */
  char * R;

  /**
     List of 'T' (tag) cards. Holds (fsl_card_T*) instances.
  */
  fsl_list T;

  /**
     The U (user) card.
  */
  char * U;

  /**
     The W (wiki content) card.
  */
  fsl_buffer W;

  /**
     This is part of an optimization used when parsing fsl_deck
     instances from source text. For most types of card we re-use
     string values in the raw source text rather than duplicate them,
     and that requires storing the original text (as passed to
     fsl_deck_parse()). This requires that clients never tinker
     directly with values in a fsl_deck, in particular never assign
     over them or assume they know who allocated the memory for that
     bit.
  */
  fsl_buffer content;

  /**
     To potentially be used for a manifest cache.
  */
  fsl_deck * next;

  /**
     A marker which tells fsl_deck_finalize() whether or not
     fsl_deck_malloc() allocated this instance (in which case
     fsl_deck_finalize() will fsl_free() it) or not (in which case
     it does not fsl_free() it).
  */
  void const * allocStamp;
};

/**
   Initialized-with-defaults fsl_deck structure, intended for copy
   initialization.
*/
FSL_EXPORT const fsl_deck fsl_deck_empty;

/**
   Initialized-with-defaults fsl_deck structure, intended for
   in-struct and const copy initialization.
*/
#define fsl_deck_empty_m {                                  \
    FSL_SATYPE_ANY /*type*/,                                \
    0/*rid*/,                                            \
    NULL/*uuid*/,                                         \
    NULL/*f*/,                                            \
    {/*A*/ NULL /* name */,                               \
           NULL /* tgt */,                                   \
           NULL /* src */},                                  \
    {/*B*/ NULL /*uuid*/,                                 \
           NULL /*baseline*/},                               \
    NULL /* C */,                                       \
    0.0 /*D*/,                                        \
    {/*E*/ 0.0 /* julian */,                          \
           NULL /* uuid */},                             \
    /*F*/ fsl_card_F_list_empty_m,  \
    0/*G*/,0/*H*/,0/*I*/,                           \
    fsl_list_empty_m /* J */,                       \
    NULL /* K */,                                 \
    NULL /* L */,                                 \
    fsl_list_empty_m /* M */,                     \
    NULL /* N */,                                 \
    fsl_list_empty_m /* P */,                     \
    fsl_list_empty_m /* Q */,                     \
    NULL /* R */,                                 \
    fsl_list_empty_m /* T */,                     \
    NULL /* U */,                                 \
    fsl_buffer_empty_m /* W */,                   \
    fsl_buffer_empty_m/*content*/,                \
    NULL/*next*/,                                 \
    NULL/*allocStamp*/                            \
  }


/**
   Allocates a new fsl_deck instance. Returns NULL on allocation
   error. The returned value must eventually be passed to
   fsl_deck_finalize() to free its resources.

   @see fsl_deck_finalize()
   @see fsl_deck_clean()
*/
FSL_EXPORT fsl_deck * fsl_deck_malloc();

/**
   Frees all resources belonging to the given deck's members
   (including its parents, recursively), and wipes deck clean of most
   state, but does not free() deck. Is a no-op if deck is NULL. As a
   special case, the (allocStamp, f) members of deck are kept intact.

   @see fsl_deck_finalize()
   @see fsl_deck_malloc()
   @see fsl_deck_clean2()
*/
FSL_EXPORT void fsl_deck_clean(fsl_deck *deck);

/**
   A variant of fsl_deck_clean() which "returns" its content buffer
   for re-use by transferring, after ensuring proper cleanup of its
   internals, its own content buffer's bytes into the given target
   buffer. Note that decks created "manually" do not have any content
   buffer contents, but those loaded via fsl_deck_load_rid() do. This
   function will fsl_buffer_swap() the contents of the given buffer
   (if any) with its own buffer, clean up its newly-acquired memory
   (tgt's previous contents, if any), and fsl_buffer_reuse() the
   output buffer.

   If tgt is NULL, this behaves exactly like fsl_deck_clean().
*/
FSL_EXPORT void fsl_deck_clean2(fsl_deck *deck, fsl_buffer *tgt);

/**
   Frees all memory owned by deck (see fsl_deck_clean()).  If deck
   was allocated using fsl_deck_malloc() then this function
   fsl_free()'s it, otherwise it does not free it.

   @see fsl_deck_malloc()
   @see fsl_deck_clean()
*/
FSL_EXPORT void fsl_deck_finalize(fsl_deck *deck);

/**
   Sets the A-card for an Attachment (FSL_SATYPE_ATTACHMENT)
   deck. Returns 0 on success.

   Returns FSL_RC_MISUSE if any of (mf, filename, target) are NULL,
   FSL_RC_RANGE if !*filename or if uuidSrc is not NULL and
   fsl_is_uuid(uuidSrc) returns false.

   Returns FSL_RC_TYPE if mf is not (as determined by its mf->type
   member) of a deck type capable of holding 'A' cards. (Only decks
   of type FSL_SATYPE_ATTACHMENT may hold an 'A' card.) If uuidSrc
   is NULL or starts with a NUL byte then it is ignored, otherwise
   the same restrictions apply to it as to target.

   The target parameter represents the "name" of the
   wiki/ticket/event record to which the attachment applies. For
   wiki pages this is their normal name (e.g. "MyWikiPage"). For
   events and tickets it is their full 40-byte UUID.

   uuidSrc is the UUID of the attachment blob itself. If it is NULL
   or empty then this card indicates that the attachment will be
   "deleted" (insofar as anything is ever deleted in Fossil).
*/
FSL_EXPORT int fsl_deck_A_set( fsl_deck * mf, char const * filename,
                               char const * target,
                               fsl_uuid_cstr uuidSrc);

/**
   Sets or unsets (if uuidBaseline is NULL or empty) the B-card for
   the given manifest to a copy of the given UUID. Returns 0 on
   success, FSL_RC_MISUSE if !mf, FSL_RC_OOM on allocation
   error. Setting this will free any prior values in mf->B, including
   a previously loaded mf->B.baseline.

   If uuidBaseline is not NULL and fsl_is_uuid() returns false,
   FSL_RC_SYNTAX is returned. If it is NULL the current value is
   freed (semantically, though the deck may still own the memory), the
   B card is effectively removed, and 0 is returned.

   Returns FSL_RC_TYPE if mf is not syntactically allowed to have
   this card card (as determined by
   fsl_card_is_legal(mf->type,...)).

   Sidebar: the ability to unset this card is unusual within this API,
   and is a requirement the library-internal delta manifest creation
   process. Most of the card-setting APIs, even when they are
   described as working like this one, do not accept NULL hash values.
*/
FSL_EXPORT int fsl_deck_B_set( fsl_deck * mf, fsl_uuid_cstr uuidBaseline);

/**
   Semantically identical to fsl_deck_B_set() but sets the C-card and
   does not place a practical limit on the comment's length.  comment
   must be the comment text for the change being applied.  If the
   given length is negative, fsl_strlen() is used to determine its
   length.
*/
FSL_EXPORT int fsl_deck_C_set( fsl_deck * mf, char const * comment, fsl_int_t cardLen);

/**
   Sets mf's D-card as a Julian Date value. Returns FSL_RC_MISUSE if
   !mf, FSL_RC_RANGE if date is negative, FSL_RC_TYPE if a D-card is
   not valid for the given deck, else 0. Passing a value of 0
   effectively unsets the card.
*/
FSL_EXPORT int fsl_deck_D_set( fsl_deck * mf, double date);

/**
   Sets the E-card in the given deck. date may not be negative -
   use fsl_db_julian_now() or fsl_julian_now() to get a default
   time if needed.  Retursn FSL_RC_MISUSE if !mf or !uuid,
   FSL_RC_RANGE if date is not positive, FSL_RC_RANGE if uuid is
   not a valid UUID string.

   Note that the UUID for an event, unlike most other UUIDs, need
   not be calculated - it may be a random hex string, but it must
   pass the fsl_is_uuid() test. Use fsl_db_random_hex() to generate
   random UUIDs. When editing events, e.g. using the HTML UI, only
   the most recent event with the same UUID is shown. So when
   updating events, be sure to apply the same UUID to the edited
   copies before saving them.
*/
FSL_EXPORT int fsl_deck_E_set( fsl_deck * mf, double date, fsl_uuid_cstr uuid);

/**
   Adds a new F-card to the given deck. The uuid argument is required
   to be NULL or pass the fsl_is_uuid() test. The name must be a
   "simplified path name" (as determined by fsl_is_simple_pathname()),
   or FSL_RC_RANGE is returned. Note that a NULL uuid is only valid
   when constructing a delta manifest, and this routine will return
   FSL_RC_MISUSE and update d->f's error state if uuid is NULL and
   d->B.uuid is also NULL.

   perms should be one of the fsl_fileperm_e values (0 is the usual
   case).

   priorName must only be non-NULL when renaming a file, and it must
   follow the same naming rules as the name parameter.

   Returns 0 on success.

   @see fsl_deck_F_set()
*/
FSL_EXPORT int fsl_deck_F_add( fsl_deck * d, char const * name,
                               fsl_uuid_cstr uuid,
                               fsl_fileperm_e perm, 
                               char const * priorName);

/**
   Works mostly like fsl_deck_F_add() except that:

   1) It enables replacing an existing F-card with a new one matching
   the same name.

   2) It enables removing an F-card by passing a NULL uuid.

   3) It refuses to work on a deck for which d->uuid is not NULL or
   d->rid!=0, returning FSL_RC_MISUSE if either of those apply.

   If d contains no F-card matching the given name (case-sensitivity
   depends on d->f's fsl_cx_is_case_sensitive() value) then:

   - If the 3rd argument is NULL, it returns FSL_RC_NOT_FOUND with
     (effectively) no side effects (aside, perhaps, from sorting d->F
     if needed to perform the search).

   - If the 3rd argument is not NULL then it behaves identically to
     fsl_deck_F_add().

   If a match is found, then:

   - If the 3rd argument is NULL, it removes that entry from the
     F-card list and returns 0.

   - If the 3rd argument is not NULL, the fields of the resulting
     F-card are modified to match the arguments passed to this
     function, copying the values of all C-string arguments. (Sidebar:
     we may need to copy the name, despite already knowing it, because
     of how fsl_deck instances manage F-card memory.)

   In all cases, if the 3rd argument is NULL then the 4th and 5th
   arguments are ignored.

   Returns 0 on success, FSL_RC_OOM if an allocation fails.  See
   fsl_deck_F_add() for other failure modes. On error, d's F-card list
   may be left in an inconsistent state and it must not be used
   further.

   @see fsl_deck_F_add()
   @see fsl_deck_F_set_content()
*/
FSL_EXPORT int fsl_deck_F_set( fsl_deck * d, char const * name,
                               fsl_uuid_cstr uuid,
                               fsl_fileperm_e perm, 
                               char const * priorName);
/**
   UNDER CONSTRUCTION! EXPERIMENTAL!

   This variant of fsl_deck_F_set() accepts a buffer of content to
   store as the file's contents. Its hash is derived from that
   content, using fsl_repo_blob_lookup() to hash the given content.
   Thus this routine can replace existing F-cards and save their
   content at the same time. When doing so, it will try to make the
   parent version (if this is a replacement F-card) a delta of the new
   content version (it may refuse to do so for various resources, but
   any heuristics which forbid that will not trigger an error).

   The intended use of this routine is for adding or replacing content
   in a deck which has been prepared using fsl_deck_derive().

   Returns 0 on success, else an error code propagated by
   fsl_deck_F_set(), fsl_repo_blob_lookup(), or some other lower-level
   routine. This routine requires that a transaction is active and
   returns FSL_RC_MISUSE if none is active. For any non-trivial
   error's, d->f's error state will be updated with a description of
   the problem.

   TODO: add a fsl_cx-level or fsl_deck-level API for marking content
   saved this way as private. This type of content is intended for use
   cases which do not have a checkout, and thus cannot be processed
   with fsl_checkin_commit() (which includes a flag to mark its
   content as private).

   @see fsl_deck_F_set()
   @see fsl_deck_F_add()
   @see fsl_deck_derive()
*/
FSL_EXPORT int fsl_deck_F_set_content( fsl_deck * d, char const * name,
                                       fsl_buffer const * src,
                                       fsl_fileperm_e perm, 
                                       char const * priorName);

/**
   UNDER CONSTRUCTION! EXPERIMENTAL!

   This routine rewires d such that it becomes the basis for a derived
   version of itself. Requires that d be a loaded
   from a repository, complete with a UUID and an RID, else
   FSL_RC_MISUSE is returned.

   In short, this function peforms the following:

   - Clears d->P
   - Moves d->uuid into d->P
   - Clears d->rid
   - Clears any other members which need to be (re)set by the new
     child/derived version.
   - It specifically keeps d->F intact OR creates a new one (see below).

   Returns 0 on success, FSL_RC_OOM on an allocation error,
   FSL_RC_MISUSE if d->uuid is NULL or d->rid<=0. If d->type is not
   FSL_SATYPE_CHECKIN, FSL_RC_TYPE is returned. On error, d may be
   left in an inconsistent state and must not be used further except
   to pass it to fsl_deck_finalize().

   The intention of this function is to simplify creation of decks
   which are to be used for creating checkins without requiring a
   checkin.

   To avoid certain corner cases, this function does not allow
   creation of delta manifests. If d has a B-card then it is a delta.
   This function clears its B-card and recreates the F-card list using
   the B-card's F-card list and any F-cards from the current delta. In
   other words, it creates a new baseline manifest.

   TODO: extend this to support other inheritable deck types, e.g.
   wiki, forum posts, and technotes.

   @see fsl_deck_F_set_content()
*/
FSL_EXPORT int fsl_deck_derive(fsl_deck * d);

/**
   Callback type for use with fsl_deck_F_foreach() and
   friends. Implementations must return 0 on success, FSL_RC_BREAK
   to abort looping without an error, and any other value on error.
*/
typedef int (*fsl_card_F_visitor_f)(fsl_card_F const * fc,
                                     void * state);

/**
   For each F-card in d, cb(card,visitorState) is called. Returns
   the result of that loop. If cb returns FSL_RC_BREAK, the
   visitation loop stops immediately and this function returns
   0. If cb returns any other non-0 code, looping stops and that
   code is returned immediately.

   This routine calls fsl_deck_F_rewind() to reset the F-card cursor
   and/or load d's baseline manifest (if any). If loading the baseline
   fails, an error code from fsl_deck_baseline_fetch() is returned.

   The F-cards will be visited in the order they are declared in
   d. For loaded-from-a-repo manifests this is always lexical order
   (for delta manifests, consistent across the delta and
   baseline). For hand-created decks which have not yet been
   fsl_deck_unshuffle()'d, the order is unspecified.
*/
FSL_EXPORT int fsl_deck_F_foreach( fsl_deck * d, fsl_card_F_visitor_f cb,
                                   void * visitorState );

/**
   Fetches the next F-card entry from d. fsl_deck_F_rewind() must
   have be successfully executed one time before calling this, as
   that routine ensures that the baseline is loaded (if needed),
   which is needed for proper iteration over delta manifests.

   This routine always assigns *f to NULL before starting its work, so
   the client can be assured that it will never contain the same value
   as before calling this (unless that value was NULL).

   On success 0 is returned and *f is assigned to the next F-card.
   If *f is NULL when returning 0 then the end of the list has been
   reached (fsl_deck_F_rewind() can be used to re-set it).

   Example usage:

   @code
   int rc;
   fsl_card_F const * fc = NULL;
   rc = fsl_deck_F_rewind(d);
   if(!rc) while( !(rc=fsl_deck_F_next(d, &fc)) && fc) {...}
   @endcode

   Note that files which were deleted in a given version are not
   recorded in baseline manifests but are in deltas. To avoid
   inconsistencies, this routine does NOT include deleted files in its
   results, regardless of whether d is a baseline or delta. (It used
   to, but that turned out to be a design flaw.)

   Implementation notes: for baseline manifests this is a very
   fast and simple operation. For delta manifests it gets
   rather complicated.
*/
FSL_EXPORT int fsl_deck_F_next( fsl_deck * d, fsl_card_F const **f );

/**
   Rewinds d's F-card traversal iterator and loads d's baseline
   manifest, if it has one (i.e. if d->B.uuid is not NULL) and it is
   not loaded already (i.e. if d->B.baseline is NULL). Returns 0 on
   success. The only error condition is if loading of the a baseline
   manifest fails, noting that only delta manifests have baselines.

   Results are undefined if d->f is NULL, and that may trigger an
   assert() in debug builds.
*/
FSL_EXPORT int fsl_deck_F_rewind( fsl_deck * d );

/**
   Looks for a file in a manifest or (for a delta manifest) its
   baseline. No normalization of the given filename is performed -
   it is assumed to be relative to the root of the checkout.

   It requires that d->type be FSL_SATYPE_CHECKIN and that d be
   loaded from a stored manifest or have been fsl_deck_unshuffle()'d
   (if called on an under-construction deck). Specifically, this
   routine requires that d->F be sorted properly or results are
   undefined.

   d->f is assumed to be the fsl_cx instance which deck was loaded
   from, which impacts the search process as follows:

   - The search take's d->f's underlying case-insensitive option into
   account. i.e. if case-insensitivy is on then files in any case
   will match.

   - If no match is found in d and is a delta manifest (d->B.uuid
   is set) then d's baseline is lazily loaded (if needed) and
   the search continues there. (Delta manifests are only one level
   deep, so this is not recursive.)

   Returns NULL if !d, !d->f, or d->type!=FSL_SATYPE_CHECKIN, if no
   entry is found, or if delayed loading of the parent manifest (if
   needed) of a delta manifest fails (in which case d->f's error
   state should hold more information about the problem).

   In debug builds this function asserts that d is not NULL.

   Design note: d "should" be const, but search optimizations for
   the typical use case require potentially lazy-loading
   d->B.baseline and updating d->F.
*/
FSL_EXPORT fsl_card_F const * fsl_deck_F_search(fsl_deck *d, const char *zName);

/**
   Given two F-card instances, this function compares their names
   (case-insensitively). Returns a negative value if lhs is
   lexically less than rhs, a positive value if lhs is lexically
   greater than rhs, and 0 if they are lexically equivalent (or are
   the same pointer).

   Results are undefined if either argument is NULL.
*/
FSL_EXPORT int fsl_card_F_compare( fsl_card_F const * lhs,
                                   fsl_card_F const * rhs);

/**
   If fc->uuid refers to a blob in f's repository database then that
   content is placed into dest (as per fsl_content_get()) and 0 is
   returned. Returns FSL_RC_NOT_FOUND if fc->uuid is not
   found. Returns FSL_RC_MISUSE if any argument is NULL.  If
   fc->uuid is NULL (indicating that it refers to a file deleted in
   a delta manifest) then FSL_RC_RANGE is returned. Returns
   FSL_RC_NOT_A_REPO if f has no repository opened.

   On any error but FSL_RC_MISUSE (basic argument validation) f's
   error state is updated to describe the error.

   @see fsl_content_get()
*/
FSL_EXPORT int fsl_card_F_content( fsl_cx * f, fsl_card_F const * fc,
                                   fsl_buffer * dest );

/**
   Sets the 'G' card on a forum-post deck to a copy of the given
   UUID.
*/
FSL_EXPORT int fsl_deck_G_set( fsl_deck * mf, fsl_uuid_cstr uuid);
/**
   Sets the 'H' card on a forum-post deck to a copy of the given
   comment. If cardLen is negative then fsl_strlen() is used to
   calculate its length.
 */
FSL_EXPORT int fsl_deck_H_set( fsl_deck * mf, char const * comment, fsl_int_t cardLen);
/**
   Sets the 'I' card on a forum-post deck to a copy of the given
   UUID.
*/
FSL_EXPORT int fsl_deck_I_set( fsl_deck * mf, fsl_uuid_cstr uuid);

/**
   Adds a J-card to the given deck, setting/updating the given ticket
   property key to the given value. The key is required but the value
   is optional (may be NULL). If isAppend then the value is appended
   to any existing value, otherwise it replaces any existing value.

   It is currently unclear whether it is legal to include multiple
   J cards for the same key in the same control artifact, in
   particular if their isAppend values differ.

   Returns 0 on success, FSL_RC_MISUSE if !mf or !key, FSL_RC_RANGE
   if !*field, FSL_RC_TYPE if mf is of a type for which J cards are
   not legal (see fsl_card_is_legal()), FSL_RC_OOM on allocation
   error.
*/
FSL_EXPORT int fsl_deck_J_add( fsl_deck * mf, char isAppend,
                               char const * key, char const * value );

/**
   Semantically identical fsl_deck_B_set() but sets the K-card and
   does not accept a NULL value.  uuid must be the UUID of the ticket
   this change is being applied to.
*/
FSL_EXPORT int fsl_deck_K_set( fsl_deck * mf, fsl_uuid_cstr uuid);

/**
   Semantically identical fsl_deck_B_set() but sets the L-card.
   title must be the wiki page title text of the wiki page this
   change is being applied to.
*/
FSL_EXPORT int fsl_deck_L_set( fsl_deck * mf, char const *title, fsl_int_t len);

/**
   Adds the given UUID as an M-card entry. Returns 0 on success, or:

   FSL_RC_MISUSE if !mf or !uuid

   FSL_RC_TYPE if fsl_deck_check_type(mf,'M') returns false.

   FSL_RC_RANGE if !fsl_is_uuid(uuid).

   FSL_RC_OOM if memory allocation fails while adding the entry.
*/
FSL_EXPORT int fsl_deck_M_add( fsl_deck * mf, fsl_uuid_cstr uuid );

/**
   Semantically identical to fsl_deck_B_set() but sets the N card.
   mimeType must be the content mime type for comment text of the
   change being applied.
*/
FSL_EXPORT int fsl_deck_N_set( fsl_deck * mf, char const *mimeType, fsl_int_t len);

/**
   Adds the given UUID as a parent of the given change record. If len
   is less than 0 then fsl_strlen(parentUuid) is used to determine
   its length. Returns FSL_RC_MISUE if !mf, !parentUuid, or
   !*parentUuid. Returns FSL_RC_RANGE if parentUuid is not 40
   bytes long.

   The first P-card added to a deck MUST be the UUID of its primary
   parent (one which was not involved in a merge operation). All
   others (from merges) are considered "non-primary."

*/
FSL_EXPORT int fsl_deck_P_add( fsl_deck * mf, fsl_uuid_cstr parentUuid);

/**
   If d contains a P card with the given index, this returns the
   RID corresponding to the UUID at that index. Returns a negative
   value on error, 0 if there is no for that index or the index
   is out of bounds.
*/
FSL_EXPORT fsl_id_t fsl_deck_P_get_id(fsl_deck * d, int index);

/**
   Adds a Q-card record to the given deck. The type argument must
   be negative for a backed-out change, positive for a cherrypicked
   change.  target must be a valid UUID string. If baseline is not
   NULL then it also must be a valid UUID.

   Returns 0 on success, non-0 on error. FSL_RC_MISUSE if !mf
   or !target, FSL_RC_RANGE if target/baseline are not valid
   UUID strings (baseline may be NULL).
*/
FSL_EXPORT int fsl_deck_Q_add( fsl_deck * mf, int type,
                    fsl_uuid_cstr target,
                    fsl_uuid_cstr baseline );

/**
   Functionally identical to fsl_deck_B_set() except that it sets
   the R-card. Returns 0 on succes, FSL_RC_RANGE if md5 is not NULL
   or exactly FSL_STRLEN_MD5 bytes long (not including trailing
   NUL). If md5==NULL the current R value is cleared.

   It would be highly unusual to have to set the R-card manually,
   as its calculation is quite intricate/intensive. See
   fsl_deck_R_calc() and fsl_deck_unshuffle() for details
*/
FSL_EXPORT int fsl_deck_R_set( fsl_deck * mf, char const *md5);

/**
   Adds a new T-card (tag) entry to the given deck.

   If uuid is not NULL and fsl_is_uuid(uuid) returns false then
   this function returns FSL_RC_RANGE. If uuid is NULL then it is
   assumed to be the UUID of the currently-being-constructed
   artifact in which the tag is contained (which appears as the '*'
   character in generated artifacts).

   Returns 0 on success. Returns FSL_RC_MISUE if !mf or
   !name. Returns FSL_RC_TYPE (and update's mf's error state with a
   message) if the T card is not legal for mf (see
   fsl_card_is_legal()).  Returns FSL_RC_RANGE if !*name, tagType
   is invalid, or if uuid is not NULL and fsl_is_uuid(uuid)
   return false. Returns FSL_RC_OOM if an allocation fails.
*/
FSL_EXPORT int fsl_deck_T_add( fsl_deck * mf, fsl_tagtype_e tagType,
                               fsl_uuid_cstr uuid, char const * name,
                               char const * value);

/**
   Adds the given tag instance to the given manifest.
   Returns 0 on success, FSL_RC_MISUSE if either argument
   is NULL, FSL_RC_OOM if appending the tag to the list
   fails.

   On success ownership of t is passed to mf. On error ownership is
   not modified.
*/
FSL_EXPORT int fsl_deck_T_add2( fsl_deck * mf, fsl_card_T * t);

/**
   A convenience form of fsl_deck_T_add() which adds two propagating
   tags to the given deck: "branch" with a value of branchName and
   "sym-branchName" with no value.

   Returns 0 on success. Returns FSL_RC_OOM on allocation error and
   FSL_RC_RANGE if branchName is empty or contains any characters with
   ASCII values <=32d. It natively assumes that any characters >=128
   are part of multibyte UTF8 characters.
*/
FSL_EXPORT int fsl_deck_branch_set( fsl_deck * d, char const * branchName );

/**
   Calculates the value of d's R-card based on its F-cards and updates
   d->R. It may also, as a side-effect, sort d->F.list lexically (a
   requirement of a R-card calculation).

   Returns 0 on success. Requires that d->f have an opened
   repository db, else FSL_RC_NOT_A_REPO is returned. If d's type is
   not legal for an R-card then FSL_RC_TYPE is returned and d->f's
   error state is updated with a description of the error. If d is of
   type FSL_SATYPE_CHECKIN and has no F-cards then the R-card's value
   is that of the initial MD5 hash state. Various other codes can be
   returned if fetching file content from the db fails.

   Note that this calculation is exceedingly memory-hungry. While
   Fossil originally required R-cards, the cost of calculation
   eventually caused the R-card to be made optional. This API
   allows the client to decide on whether to use them (for more
   (admittedly redundant!) integrity checking) or not (much faster
   but "not strictly historically correct"), but defaults to having
   them enabled for symmetry with fossil(1).

   @see fsl_deck_R_calc2()
*/
FSL_EXPORT int fsl_deck_R_calc(fsl_deck * d);

/**
   A variant of fsl_deck_R_calc() which calculates the given deck's
   R-card but does not assign it to the deck, instead returning it
   via the 2nd argument:

   If *tgt is not NULL when this function is called, it is required to
   point to at least FSL_STRLEN_MD5+1 bytes of memory to which the
   NUL-terminated R-card hash will be written. If *tgt is NULL then
   this function assigns (on success) *tgt to a dynamically-allocated
   R-card hash and transfers ownership of it to the caller (who must
   eventually fsl_free() it). On error, *tgt is not modified.

   Results are undefined if either argument is NULL.
   
   Returns 0 on success. See fsl_deck_R_calc() for information about
   possible errors, with the addition that FSL_RC_OOM is returned
   if *tgt is NULL and allocating a new *tgt value fails.

   Calculating the R-card necessarily requires that d's F-card list be
   sorted, which this routine does if it seems necessary. The
   calculation also necessarily mutates the deck's F-card-traversal
   cursor, which requires loading the deck's B-card, if it has
   one. Aside from the F-card sorting, and potentially B-card, and the
   cursor resets, this routine does not modify the deck. On success,
   the deck's F-card iteration cursor (and that of d->B, if it's
   loaded) is rewound.
*/
FSL_EXPORT int fsl_deck_R_calc2(fsl_deck *d, char ** tgt);

/**
   Semantically identical fsl_deck_B_set() but sets the U-card.
   userName must be the user who's name should be recorded for
   this change.
*/
FSL_EXPORT int fsl_deck_U_set( fsl_deck * mf, char const *userName);

/**
   Semantically identical fsl_deck_B_set() but sets the W-card.
   content must be the page content of the Wiki page or Event this
   change is being applied to.
*/
FSL_EXPORT int fsl_deck_W_set( fsl_deck * mf, char const *content, fsl_int_t len);

/**
   Must be called to initialize a newly-created/allocated deck
   instance. This function clears out all contents of the d
   parameter except for its (f, type, allocStamp) members, sets its
   (f, type) members, and leaves d->allocStamp intact.
*/
FSL_EXPORT void fsl_deck_init( fsl_cx * cx, fsl_deck * d, fsl_satype_e type );

/**
   Returns true if d contains data for all _required_ cards, as
   determined by the value of d->type, else returns false. It returns
   false if d->type==FSL_SATYPE_ANY, as that is a placeholder value
   intended to be re-set by the deck's user.

   If it returns false, d->f's error state will help a description of
   the problem.

   The library calls this as needed, but clients may, if they want
   to. Note, however, that for FSL_SATYPE_CHECKIN decks it may fail
   if the deck has not been fsl_deck_unshuffle()d yet because the
   R-card gets calculated there (if needed).

   As a special case, d->f is configured to calculate R-cards,
   d->type==FSL_SATYPE_CHECKIN, AND d->R is not set, this will fail
   (with a descriptive error message).

   Another special case: for FSL_SATYPE_CHECKIN decks, if no
   F-cards are in th deck then an R-card is required to avoid a
   potental (admittedly harmless) syntactic ambiguity with
   FSL_SATYPE_CONTROL artifacts. The only legal R-card for a
   checkin with no F-cards has the initial MD5 hash state value
   (defined in the constant FSL_MD5_INITIAL_HASH), and that
   precondition is checked in this routine. fsl_deck_unshuffle()
   recognizes this case and adds the initial-state R-card, so
   clients normally need not concern themselves with this. If d has
   F-cards, whether or not an R-card is required depends on
   whether d->f is configured to require them or not.

   Enough about the R-card. In all other cases not described above,
   R-cards are not required (and they are only ever required on
   FSL_SATYPE_CHECKIN manifests).

   Though fossil(1) does not technically require F-cards in
   FSL_SATYPE_CHECKIN decks, so far none of the Fossil developers
   have found a use for a checkin without F-cards except the
   initial empty checkin. Additionally, a checkin without F-cards
   is potentially syntactically ambiguous (it could be an EVENT or
   ATTACHMENT artifact if it has no F- or R-card). So... this
   library _normally_ requires that CHECKIN decks have at least one
   F-card. This function, however, does not consider F-cards to be
   strictly required.
*/
FSL_EXPORT bool fsl_deck_has_required_cards( fsl_deck const * d );

/**
   Prepares the given deck for output by ensuring that cards
   which need to be sorted are sorted, and it may run some
   last-minute validation checks.

   The cards which get sorted are: F, J, M, Q, T. The P-card list is
   _not_ sorted - the client is responsible for ensuring that the
   primary parent is added to that list first, and after that the
   ordering is largely irrelevant. It is not possible for the library
   to determine a proper order for P-cards, nor to validate that order
   at input-time.

   If calculateRCard is true and fsl_card_is_legal(d,'R') then this
   function calculates the R-card for the deck. The R-card
   calculation is _extremely_ memory-hungry but adds another level
   of integrity checking to Fossil. If d->type is not
   FSL_SATYPE_MANIFEST then calculateRCard is ignored.

   If calculateRCard is true but no F-cards are present AND d->type is
   FSL_SATYPE_CHECKIN then the R-card is set to the initial MD5 hash
   state (the only legal R-card value for an empty F-card list). (This
   is necessary in order to prevent a deck-type ambiguity in one
   corner case.)

   The R-card, if used, must be calculated before
   fsl_deck_output()ing a deck containing F-cards. Clients may
   alternately call fsl_deck_R_calc() to calculate the R card
   separately, but there is little reason to do so. There are rare
   cases where the client can call fsl_deck_R_set()
   legally. Historically speaking the R-card was required when
   F-cards were used, but it was eventually made optional because
   (A) the memory cost and (B) it's part of a 3rd or 4th level of
   integrity-related checks, and is somewhat superfluous.

   @see fsl_deck_output()
   @see fsl_deck_save()
*/
FSL_EXPORT int fsl_deck_unshuffle( fsl_deck * d, bool calculateRCard );

/**
   Renders the given control artifact's contents to the given output
   function and calculates any cards which cannot be calculated until
   the contents are complete (namely the R-card and Z-card).

   The given deck is "logically const" but traversal over F-cards and
   baselines requires non-const operations. To keep this routine from
   requiring an undue amount of pre-call effort on the client's part,
   it also takes care of calling fsl_deck_unshuffle() to ensure that
   all of the deck's cards are in order. (If the deck has no R card,
   but has F-cards, and d->f is configured to generate R-cards, then
   unshuffling will also calculate the R-card.)

   Returns 0 on success, FSL_RC_MISUSE if !d or !d->f or !out. If
   out() returns non-0, output stops and that code is
   returned. outputState is passed as the first argument to out() and
   out() may be called an arbitrary number of times by this routine.

   Returns FSL_RC_SYNTAX if fsl_deck_has_required_cards()
   returns false.

   On errors more serious than argument validation, the deck's
   context's (d->f) error state is updated.

   The exact structure of the ouput depends on the value of
   mf->type, and FSL_RC_TYPE is returned if this function cannot
   figure out what to do with the given deck's type.

   @see fsl_deck_unshuffle()
   @see fsl_deck_save()
*/
FSL_EXPORT int fsl_deck_output( fsl_deck * d, fsl_output_f out,
                                void * outputState );


/**
   Saves the given deck into f's repository database as new control
   artifact content. If isPrivate is true then the content is
   marked as private, otherwise it is not. Note that isPrivate is a
   suggestion and might be trumped by existing state within f or
   its repository, and such a trumping is not treated as an
   error. e.g. tags are automatically private when they tag private
   content.

   Before saving, the deck is passed through fsl_deck_unshuffle()
   and fsl_deck_output(), which will fail for a variety of
   easy-to-make errors such as the deck missing required cards.
   For unshuffle purposes, the R-card gets calculated if the deck
   has any F-cards AND if the caller has not already set/calculated
   it AND if f's FSL_CX_F_CALC_R_CARD flag is set (it is on by
   default for historical reasons, but this may change at some
   point).

   Returns 0 on success, the usual non-0 suspects on error.

   If d->rid and d->uuid are set when this is called, it is assumed
   that we are saving existing or phantom content, and in that
   case:

   - An existing phantom is populated with the new content.

   - If an existing record is found with a non-0 size then it is
   not modified but this is currently not treated as an error (for
   historical reasons, though one could argue that it should result
   in FSL_RC_ALREADY_EXISTS).

   If d->rid and d->uuid are not set when this is called then... on
   success, d->rid and d->uuid will contain the values held by
   their counterparts in the blob table. They will only be set on
   success because they would otherwise refer to db records which
   get destroyed when the transaction rolls back.

   After saving, the deck will be cross-linked to update any
   relationships described by the deck.

   The save operation happens within a transaction, of course, and
   on any sort of error, db-side changes are rolled back. Note that
   it _is_ legal to start the transaction before calling this,
   which effectively makes this operation part of that transaction.

   This function will fail with FSL_RC_ACCESS if d is a delta manifest
   (has a B-card) d->f's forbid-delta-manifests configuration option
   is set to a truthy value. See fsl_repo_forbids_delta_manifests().

   Maintenance reminder: this function also does a small bit of
   artifact-type-specific processing.

   @see fsl_deck_output()
   @see fsl_content_put_ex()
*/
FSL_EXPORT int fsl_deck_save( fsl_deck * d, bool isPrivate );

/**
    This starts a transaction (possibly nested) on the repository db
    and initializes some temporary db state needed for the
    crosslinking certain artifact types. It "should" (see below) be
    called at the start of the crosslinking process. Crosslinking
    *can* work without this but certain steps for certain (subject to
    change) artifact types will be skipped, possibly leading to
    unexpected timeline data or similar funkiness. No permanent
    SCM-relevant state will be missing, but the timeline might not be
    updated and tickets might not be fully processed. This should be
    used before crosslinking any artifact types, but will only have
    significant side effects for certain (subject to change) types.

    Returns 0 on success.

    If this function succeeds, the caller is OBLIGATED to either call
    fsl_crosslink_end() or fsl_db_transaction_rollback(), depending
    on whether the work done after this call succeeds
    resp. fails. This process may install temporary tables and/or
    triggers, so failing to call one or the other of those will result
    in misbehavior.

    @see fsl_deck_crosslink()
*/
int fsl_crosslink_begin(fsl_cx * f);

/**
    Must not be called unless fsl_crosslink_begin() has
    succeeded.  This performs crosslink post-processing on certain
    artifact types and cleans up any temporary db state initialized by
    fsl_crosslink_begin().

    Returns 0 on success. On error it initiates (or propagates) a
    rollback for the current transaction.
*/
int fsl_crosslink_end(fsl_cx * f);

/**
   Parses src as Control Artifact content and populates d with it.

   d will be cleaned up before parsing if it has any contents,
   retaining its d->f member (which must be non-NULL for
   error-reporting purposes).

   This function _might_ take over the contents of the source
   buffer on success or it _might_ leave it for the caller to clean
   up or re-use, as he sees fit. If the caller does not intend to
   re-use the buffer, he should simply pass it to
   fsl_buffer_clear() after calling this (no need to check if it
   has contents or not first).

   When taking over the contents then on success, after returning
   src->mem will be NULL, and all other members will be reset to
   their default state. This function only takes over the contents
   if it decides to implement certain memory optimizations.

   Ownership of src itself is never changed by this function, only
   (possibly!) the ownership of its contents.

   In any case, the content of the source buffer is modified by
   this function because (A) that simplifies tokenization greatly,
   (B) saves us having to make another copy to work on, (C) the
   original implementation did it that way, (D) because in
   historical use the source is normally thrown away after parsing,
   anyway, and (E) in combination with taking ownership of src's
   contents it allows us to optimize away some memory allocations
   by re-using the internal memory of the buffer. This function
   never changes src's size, but it mutilates its contents
   (injecting NUL bytes as token delimiters).

   If d->type is _not_ FSL_SATYPE_ANY when this is called, then
   this function requires that the input to be of that type. We can
   fail relatively quickly in that case, and this can be used to
   save some downstream code some work. Note that the initial type
   for decks created using fsl_deck_malloc() or copy-initialized
   from ::fsl_deck_empty is FSL_SATYPE_ANY, so normally clients do
   not need to set this (unless they want to, as a small
   optimization).

   On success it returns 0 and d will be updated with the state
   from the input artifact. (Ideally, outputing d via
   fsl_deck_output() will produce a lossless copy of the original.)
   d->uuid will be set to the SHA1 of the input artifact, ignoring
   any surrounding PGP signature for hashing purposes.

   If d->f has an opened repository db and the parsed artifact has a
   counterpart in the database (determined via a hash match) then
   d->rid is set to the record ID.

   On error, if there is error information to propagate beyond the
   result code then it is stored in d->f (if that is not NULL),
   else in d->error. Whether or not such error info is propagated
   depends on the type of error, but anything more trivial than
   invalid arguments will be noted there.

   d might be partially populated on error, so regardless of success
   or failure, the client must eventually pass d to
   fsl_deck_finalize() to free its memory.

   Error result codes include:

   - FSL_RC_MISUSE if any pointer argument is NULL.

   - FSL_RC_SYNTAX on syntax errors.

   - FSL_RC_CONSISTENCY if validation of a Z-card fails.

   - Any number of errors coming from the allocator, database, or
   fsl_deck APIs used here.
*/
FSL_EXPORT int fsl_deck_parse(fsl_deck * d, fsl_buffer * src);

/**
   Quickly determines whether the content held by the given buffer
   "might" be a structural artifact. It performs a fast sanity check
   for prominent features which can be checked either in O(1) or very
   short O(N) time (with a fixed N). If it returns false then the
   given buffer's contents are, with 100% certainty, *not* a
   structural artifact. If it returns true then they *might* be, but
   being 100% certain requires passing the contents to
   fsl_deck_parse() to fully parse them.
*/
FSL_EXPORT bool fsl_might_be_artifact(fsl_buffer const * src);

/**
   Loads the content from given rid and tries to parse it as a
   Fossil artifact. If rid==0 the current checkout (if opened) is
   used. (Trivia: there can never be a checkout with rid==0 but
   rid==0 is sometimes valid for an new/empty repo devoid of
   commits). If type==FSL_SATYPE_ANY then it will allow any type of
   control artifact, else it returns FSL_RC_TYPE if the loaded
   artifact is of the wrong type.

   Returns 0 on success.

   d may be partially populated on error, and the caller must
   eventually pass it to fsl_deck_finalize() resp. fsl_deck_clean()
   regardless of success or error. This function "could" clean it
   up on error, but leaving it partially populated makes debugging
   easier. If the error was an artifact type mismatch then d will
   "probably" be properly populated but will not hold the type of
   artifact requested. It "should" otherwise be well-formed because
   parsing errors occur before the type check can happen, but
   parsing of invalid manifests might also trigger a FSL_RC_TYPE
   error of a different nature. The morale of the storage is: if
   this function returns non-0, assume d is useless and needs to be
   cleaned up.

   f's error state may be updated on error (for anything more
   serious than basic argument validation errors).

   On success d->f is set to f.

   @see fsl_deck_load_sym()
*/
FSL_EXPORT int fsl_deck_load_rid( fsl_cx * f, fsl_deck * d,
                                  fsl_id_t rid, fsl_satype_e type );

/**
   A convenience form of fsl_deck_load_rid() which uses
   fsl_sym_to_rid() to convert symbolicName into an artifact RID.  See
   fsl_deck_load_rid() for the symantics of the first, second, and
   fourth arguments, as well as the return value. See fsl_sym_to_rid()
   for the allowable values of symbolicName.

   @see fsl_deck_load_rid()
*/
FSL_EXPORT int fsl_deck_load_sym( fsl_cx * f, fsl_deck * d,
                                  char const * symbolicName,
                                  fsl_satype_e type );

/**
   Loads the baseline manifest specified in d->B.uuid, if any and if
   necessary. Returns 0 on success. If d->B.baseline is already loaded
   or d->B.uuid is NULL (in which case there is no baseline), it
   returns 0 and has no side effects.

   Neither argument may be NULL and d must be a fully-populated
   object, complete with a proper d->rid, before calling this.

   On success 0 is returned. If d->B.baseline is NULL then
   it means that d has no baseline manifest (and d->B.uuid will be NULL
   in that case). If d->B.baseline is not NULL then it is owned by
   d and will be cleaned up when d is cleaned/finalized.

   Error codes include, but are not limited to:

   - FSL_RC_MISUSE if !d->f.

   - FSL_RC_NOT_A_REPO if d->f has no opened repo db.

   - FSL_RC_RANGE if d->rid<=0, but that code might propagate up from
   a lower-level call as well.

   On non-trivial errors d->f's error state will be updated to hold
   a description of the problem.

   Some misuses trigger assertions in debug builds.
*/
FSL_EXPORT int fsl_deck_baseline_fetch( fsl_deck * d );

/**
   A callback interface for manifest crosslinking, so that we can farm
   out the updating of the event table. Each callback registered via
   fsl_xlink_listener() will be called at the end of the so-called
   crosslinking process, which is run every time a control artifact is
   processed for d->f's repository database, passed the deck being
   crosslinked and the client-provided state which was registered with
   fsl_xlink_listener(). Note that the deck object itself holds other
   state useful for crosslinking, like the blob.rid value of the deck
   and its fsl_cx instance.

   If an implementation is only interested in a specific type of
   artifact, it must check d->type and return 0 if it's an
   "uninteresting" type.

   Implementations must return 0 on success or some other fsl_rc_e
   value on error. Returning non-0 causes the database transaction
   for the crosslinking operation to roll back, effectively
   cancelling whatever pending operation triggered the
   crosslink. If any callback fails, processing stops immediately -
   no other callbacks are executed.

   Implementations which want to report more info than an integer
   should call fsl_cx_err_set() to set d->f's error state, as that
   will be propagated up to the code which initiated the failed
   crosslink.

   ACHTUNG and WARNING: the fsl_deck parameter "really should" be
   const, but certain operations on a deck are necessarily non-const
   operations. That includes, but may not be limited to:

   - Iterating over F-cards, which requires calling
     fsl_deck_F_rewind() before doing so.

   - Loading a checkin's baseline (required for F-card iteration and
   performed automatically by fsl_deck_F_rewind()).

   Aside from such iteration-related mutable state, it is STRICTLY
   ILLEGAL to modify a deck's artifact-related state while it is
   undergoing crosslinking. It is legal to modify its error state.


   Potential TODO: add some client-opaque state to decks so that they
   can be flagged as "being crosslinked" and fail mutation operations
   such as card adders/setters.

   @see fsl_xlink_listener()
*/
typedef int (*fsl_deck_xlink_f)(fsl_deck * d, void * state);

/**
    A type for holding a callback/state pair for manifest
    crosslinking callbacks.
*/
struct fsl_xlinker {
  char const * name;
  /** Callback function. */
  fsl_deck_xlink_f f;
  /** State for this->f's last argument. */
  void * state;
};
typedef struct fsl_xlinker fsl_xlinker;

/** Empty-initialized fsl_xlinker struct, intended for const-copy
    intialization. */
#define fsl_xlinker_empty_m {NULL,NULL,NULL}

/** Empty-initialized fsl_xlinker struct, intended for copy intialization. */
extern const fsl_xlinker fsl_xlinker_empty;

/**
    A list of fsl_xlinker instances.
*/
struct fsl_xlinker_list {
  /** Number of used items in this->list. */
  fsl_size_t used;
  /** Number of slots allocated in this->list. */
  fsl_size_t capacity;
  /** Array of this->used elements. */
  fsl_xlinker * list;
};
typedef struct fsl_xlinker_list fsl_xlinker_list;

/** Empty-initializes fsl_xlinker_list struct, intended for
    const-copy intialization. */
#define fsl_xlinker_list_empty_m {0,0,NULL}

/** Empty-initializes fsl_xlinker_list struct, intended for copy intialization. */
extern const fsl_xlinker_list fsl_xlinker_list_empty;

/**
    Searches f's crosslink callbacks for an entry with the given
    name and returns that entry, or NULL if no match is found.  The
    returned object is owned by f.
*/
fsl_xlinker * fsl_xlinker_by_name( fsl_cx * f, char const * name );

/**
   Adds the given function as a "crosslink callback" for the given
   Fossil context. The callback is called at the end of a
   successfull fsl_deck_crosslink() operation and provides a way
   for the client to perform their own work based on the app having
   crosslinked an artifact. Crosslinking happens when artifacts are
   saved or upon a rebuild operation.

   Crosslink callbacks are called at the end of the core crosslink
   steps, in the order they are registered, with the caveat that if a
   listener is overwritten by another with the same name, the new
   entry retains the older one's position in the list. The library may
   register its own before the client gets a chance to.

   If _any_ crosslinking callback fails (returns non-0) then the
   _whole_ crosslinking fails and is rolled back (which may very
   well include pending tags/commits/whatever getting rolled back).

   The state parameter has no meaning for this function, but is
   passed on as the final argument to cb(). If not NULL, cbState
   "may" be required to outlive f, depending on cbState's exact
   client-side internal semantics/use, as there is currently no API
   to remove registered crosslink listeners.

   The name must be non-NULL/empty. If a listener is registered with a
   duplicate name then the first one is replaced. This function does
   not copy the name bytes - they are assumed to be static or
   otherwise to live at least as long as f. The name may be
   arbitrarily long, but must have a terminating NUL byte. It is
   recommended that clients choose a namespace/prefix to apply to the
   names they register. The library reserves the prefix "fsl/" for
   its own use, and will happily overwrite client-registered entries
   with the same names. The name string need not be stable across
   application sessions and maybe be a randomly-generated string.

   Caveat: some obscure artifact crosslinking steps do not happen
   unless crosslinking takes place in the context of a
   fsl_crosslink_begin() and fsl_crosslink_end()
   session. Thus, at the time client-side crosslinker callbacks are
   called, certain crosslinking state in the database may still be
   pending. It is as yet unclear how best to resolve that minor
   discrepancy, or whether it even needs resolving.


   Default (overrideable) crosslink handlers:

   The library internally splits crosslinking of artifacts into two
   parts: the main one (which clients cannot modify) handles the
   database-level linking of relational state implied by a given
   artifact. The secondary one adds an entry to the "event" table,
   which is where Fossil's timeline lives. The crosslinkers for the
   timeline updates may be overridden by clients by registering
   a crosslink listener with the following names:

   - Attachment artifacts: "fsl/attachment/timeline"

   - Checkin artifacts: "fsl/checkin/timeline"

   - Control artifacts: "fsl/control/timeline"

   - Forum post artifacts: "fsl/forumpost/timeline"

   - Technote artifacts: "fsl/technote/timeline"

   - Wiki artifacts: "fsl/wiki/timeline"

   A context registers listeners under those names when it
   initializes, and clients may override them at any point after that.

   Caveat: updating the timeline requires a bit of knowledge about the
   Fossil DB schema and/or conventions. Updates for certain types,
   e.g. attachment/control/forum post, is somewhat more involved and
   updating the timeline for wiki comments requires observing a "quirk
   of conventions" for labeling such comments, such that they will
   appear properly when the main fossil app renders them. That said,
   the only tricky parts of those updates involve generating the
   "correct" comment text. So long as the non-comment parts are
   updated properly (that part is easy to do), fossil can function
   with it.  The timeline comment text/links are soley for human
   consumption. Fossil makes much use of the "event" table internally,
   however, so the rest of that table must be properly populated.

   Because of that caveat, clients may, rather than overriding the
   defaults, install their own crosslink listners which ammend the
   state applied by the default ones. e.g. add a listener which
   watches for checkin updates and replace the default-installed
   comment with one suitable for your application, leaving the rest of
   the db state in place. At its simplest, that looks more or less
   like the following code (inside a fsl_deck_xlink_f() callback):

   @code
   int rc = fsl_db_exec(fsl_cx_db_repo(deck->f),
                        "UPDATE event SET comment=%Q "
                        "WHERE objid=%"FSL_ID_T_PFMT,
                        "the new comment.", deck->rid);
   @endcode
*/
FSL_EXPORT int fsl_xlink_listener( fsl_cx * f, char const * name,
                                   fsl_deck_xlink_f cb, void * cbState );


/**
   For the given blob.rid value, returns the blob.size value of
   that record via *rv. Returns 0 or higher on success, -1 if a
   phantom record is found, -2 if no entry is found, or a smaller
   negative value on error (dig around the sources to decode them -
   this is not expected to fail unless the system is undergoing a
   catastrophe).

   @see fsl_content_blob()
   @see fsl_content_get()
*/
FSL_EXPORT fsl_int_t fsl_content_size( fsl_cx * f, fsl_id_t blobRid );

/**
   For the given blob.rid value, fetches the content field of that
   record and overwrites tgt's contents with it (reusing tgt's
   memory if it has any and if it can). The blob's contents are
   uncompressed if they were stored in compressed form. This
   extracts a raw blob and does not apply any deltas - use
   fsl_content_get() to fully expand a delta-stored blob.

   Returns 0 on success. On error tgt might be partially updated,
   e.g. it might be populated with compressed data instead of
   uncompressed. On error tgt's contents should be recycled
   (e.g. fsl_buffer_reuse()) or discarded (e.g. fsl_buffer_clear())
   by the client.

   @see fsl_content_get()
   @see fsl_content_size()
*/
FSL_EXPORT int fsl_content_blob( fsl_cx * f, fsl_id_t blobRid, fsl_buffer * tgt );

/**
   Functionally similar to fsl_content_blob() but does a lot of
   work to ensure that the returned blob is expanded from its
   deltas, if any. The tgt buffer's memory, if any, will be
   replaced/reused if it has any.

   Returns 0 on success. There are no less than 50 potental
   different errors, so we won't bother to list them all. On error
   tgt might be partially populated. The basic error cases are:

   - FSL_RC_MISUSE if !tgt or !f.

   - FSL_RC_RANGE if rid<=0 or if an infinite loop is discovered in
   the repo delta table links (that is a consistency check to avoid
   an infinite loop - that condition "cannot happen" because the
   verify-before-commit logic catches that error case).

   - FSL_RC_NOT_A_REPO if f has no repo db opened.

   - FSL_RC_NOT_FOUND if the given rid is not in the repo db.

   - FSL_RC_OOM if an allocation fails.


   @see fsl_content_blob()
   @see fsl_content_size()
*/
FSL_EXPORT int fsl_content_get( fsl_cx * f, fsl_id_t blobRid, fsl_buffer * tgt );

/**
   Uses fsl_sym_to_rid() to convert sym to a record ID, then
   passes that to fsl_content_get(). Returns 0 on success.
*/
FSL_EXPORT int fsl_content_get_sym( fsl_cx * f, char const * sym, fsl_buffer * tgt );

/**
   Returns true if the given rid is marked as PRIVATE in f's current
   repository. Returns false (0) on error or if the content is not
   marked as private.
*/
FSL_EXPORT bool fsl_content_is_private(fsl_cx * f, fsl_id_t rid);

/**
   Marks the given rid public, if it was previously marked as
   private. Returns 0 on success, non-0 on error.

   Note that it is not possible to make public content private.
*/
FSL_EXPORT int fsl_content_make_public(fsl_cx * f, fsl_id_t rid);

/**
   Generic callback interface for visiting decks. The interface
   does not generically require that d survive after this call
   returns.

   Implementations must return 0 on success, non-0 on error. Some
   APIs using this interface may specify that FSL_RC_BREAK can be
   used to stop iteration over a loop without signaling an error.
   In such cases the APIs will translate FSL_RC_BREAK to 0 for
   result purposes, but will stop looping over whatever it is they
   are looping over.
*/
typedef int (*fsl_deck_visitor_f)( fsl_cx * f, fsl_deck const * d,
                                   void * state );

/**
   For each unique wiki page name in f's repostory, this calls
   cb(), passing it the manifest of the most recent version of that
   page. The callback should return 0 on success, FSL_RC_BREAK to
   stop looping without an error, or any other non-0 code
   (preferably a value from fsl_rc_e) on error.

   The 3rd parameter has no meaning for this function but it is
   passed on as-is to the callback.

   ACHTUNG: the deck passed to the callback is transient and will
   be cleaned up after the callback has returned, so the callback
   must not hold a pointer to it or its contents.

   @see fsl_wiki_load_latest()
   @see fsl_wiki_latest_rid()
   @see fsl_wiki_names_get()
   @see fsl_wiki_page_exists()
*/
FSL_EXPORT int fsl_wiki_foreach_page( fsl_cx * f, fsl_deck_visitor_f cb, void * state );

/**
   Fetches the most recent RID for the given wiki page name and
   assigns *newId (if it is not NULL) to that value. Returns 0 on
   success, FSL_RC_MISUSE if !f or !pageName, FSL_RC_RANGE if
   !*pageName, and a host of other potential db-side errors
   indicating more serious problems. If no such page is found,
   newRid is not modified and this function returns 0 (as opposed
   to FSL_RC_NOT_FOUND) because that simplifies usage (so far).

   On error *newRid is not modified.

   @see fsl_wiki_load_latest()
   @see fsl_wiki_foreach_page()
   @see fsl_wiki_names_get()
   @see fsl_wiki_page_exists()
*/
FSL_EXPORT int fsl_wiki_latest_rid( fsl_cx * f, char const * pageName, fsl_id_t * newRid );

/**
   Loads the artifact for the most recent version of the given wiki page,
   populating d with its contents.

   Returns 0 on success. On error d might be partially populated,
   so it needs to be passed to fsl_deck_finalize() regardless of
   whether this function succeeds or fails.

   Returns FSL_RC_NOT_FOUND if no page with that name is found.

   @see fsl_wiki_latest_rid()
   @see fsl_wiki_names_get()
   @see fsl_wiki_page_exists()
*/
FSL_EXPORT int fsl_wiki_load_latest( fsl_cx * f, char const * pageName, fsl_deck * d );

/**
   Returns true (non-0) if f's repo database contains a page with the
   given name, else false.

   @see fsl_wiki_load_latest()
   @see fsl_wiki_latest_rid()
   @see fsl_wiki_names_get()
   @see fsl_wiki_names_get()
*/
FSL_EXPORT bool fsl_wiki_page_exists(fsl_cx * f, char const * pageName);

/**
   A helper type for use with fsl_wiki_save(), intended primarily
   to help client-side code readability somewhat.
*/
enum fsl_wiki_save_mode_t {
/**
   Indicates that fsl_wiki_save() must only allow the creation of
   a new page, and must fail if such an entry already exists.
*/
FSL_WIKI_SAVE_MODE_CREATE = -1,
/**
   Indicates that fsl_wiki_save() must only allow the update of an
   existing page, and will not create a branch new page.
*/
FSL_WIKI_SAVE_MODE_UPDATE = 0,
/**
   Indicates that fsl_wiki_save() must allow both the update and
   creation of pages. Trivia: "upsert" is a common SQL slang
   abbreviation for "update or insert."
*/
FSL_WIKI_SAVE_MODE_UPSERT = 1
};

typedef enum fsl_wiki_save_mode_t fsl_wiki_save_mode_t;

/**
   Saves wiki content to f's repository db.

   pageName is the name of the page to update or create.

   b contains the content for the page.

   userName specifies the user name to apply to the change. If NULL
   or empty then fsl_cx_user_get() or fsl_guess_user_name() are
   used (in that order) to determine the name.

   mimeType specifies the mime type for the content (may be NULL).
   Mime type names supported directly by fossil(1) include (as of
   this writing): text/x-fossil-wiki, text/x-markdown,
   text/plain

   Whether or not this function is allowed to create a new page is
   determined by creationPolicy. If it is
   FSL_WIKI_SAVE_MODE_UPDATE, this function will fail with
   FSL_RC_NOT_FOUND if no page with the given name already exists.
   If it is FSL_WIKI_SAVE_MODE_CREATE and a previous version _does_
   exist, it fails with FSL_RC_ALREADY_EXISTS. If it is
   FSL_WIKI_SAVE_MODE_UPSERT then both the save-exiting and
   create-new cases are allowed. In summary:

   - use FSL_WIKI_SAVE_MODE_UPDATE to allow updates to existing pages
   but disallow creation of new pages,

   - use FSL_WIKI_SAVE_MODE_CREATE to allow creating of new pages
   but not of updating an existing page.

   - FSL_WIKI_SAVE_MODE_UPSERT allows both updating and creating
   a new page on demand.

   Returns 0 on success, or any number fsl_rc_e codes on error. On
   error no content changes are saved, and any transaction is
   rolled back or a rollback is scheduled if this function is
   called while a transaction is active.


   Potential TODO: add an optional (fsl_id_t*) output parameter
   which gets set to the new record's RID.

   @see fsl_wiki_page_exists()
   @see fsl_wiki_names_get()
*/
FSL_EXPORT int fsl_wiki_save(fsl_cx * f, char const * pageName,
                  fsl_buffer const * b, char const * userName,
                  char const * mimeType, fsl_wiki_save_mode_t creationPolicy );

/**
   Fetches the list of all wiki page names in f's current repo db
   and appends them as new (char *) strings to tgt. On error tgt
   might be partially populated (but this will only happen on an
   OOM or serious system-level error).

   It is up to the caller free the entries added to the list. Some
   of the possibilities include:

   @code
   fsl_list_visit( list, 0, fsl_list_v_fsl_free, NULL );
   fsl_list_reserve(list,0);
   // Or:
   fsl_list_clear(list, fsl_list_v_fsl_free, NULL);
   // Or simply:
   fsl_list_visit_free( list, 1 );
   @endcode

*/
FSL_EXPORT int fsl_wiki_names_get( fsl_cx * f, fsl_list * tgt );

/**
   F-cards each represent one file entry in a Manifest Artifact (i.e.,
   a checkin version).

   All of the non-const pointers in this class are owned by the
   respective instance of the class OR by the fsl_deck which created
   it, and must neither be modified nor freed except via the
   appropriate APIs.
*/
struct fsl_card_F {
  /**
     UUID of the underlying blob record for the file. NULL for
     removed entries.
  */
  fsl_uuid_str uuid;
  /**
     Name of the file.
  */
  char * name;
  /**
     Previous name if the file was renamed, else NULL.
  */
  char * priorName;
  /**
     File permissions. Fossil only supports one "permission" per
     file, and it does not necessarily map to a real
     filesystem-level permission.

     @see fsl_fileperm_e
  */
  fsl_fileperm_e perm;

  /**
     An internal optimization. Do not mess with this.  When this is
     true, the various string members of this struct are not owned
     by this struct, but by the deck which created this struct. This
     is used when loading decks from storage - the strings are
     pointed to the original content data, rather than strdup()'d
     copies of it. fsl_card_F_clean() will DTRT and delete the
     strings (or not).
  */
  bool deckOwnsStrings;
};
/**
   Empty-initialized fsl_card_F structure, intended for use in
   initialization when embedding fsl_card_F in another struct or
   copy-initializing a const struct.
*/
#define fsl_card_F_empty_m {   \
  NULL/*uuid*/,                \
  NULL/*name*/,                \
  NULL/*priorName*/,           \
  0/*perm*/,                   \
  false/*deckOwnsStrings*/     \
}
FSL_EXPORT const fsl_card_F fsl_card_F_empty;

/**
   Represents a J card in a Ticket Control Artifact.
*/
struct fsl_card_J {
  /**
     If true, the new value should be appended to any existing one
     with the same key, else it will replace any old one.
  */
  char append;
  /**
     For internal use only.
  */
  unsigned char flags;
  /**
     The ticket field to update. The bytes are owned by this object.
  */
  char * field;
  /**
     The value for the field. The bytes are owned by this object.
  */
  char * value;
};
/** Empty-initialized fsl_card_J struct. */
#define fsl_card_J_empty_m {0,0,NULL, NULL}
/** Empty-initialized fsl_card_J struct. */
FSL_EXPORT const fsl_card_J fsl_card_J_empty;

/**
   Represents a tag in a Manifest or Control Artifact.
*/
struct fsl_card_T {
  /**
     The type of tag.
  */
  fsl_tagtype_e type;
  /**
     UUID of the artifact this tag is tagging. When applying a tag to
     a new checkin, this value is left empty (=NULL) and gets replaced
     by a '*' in the resulting control artifact.
  */
  fsl_uuid_str uuid;
  /**
     The tag's name. The bytes are owned by this object.
  */
  char * name;
  /**
     The tag's value. May be NULL/empty. The bytes are owned by
     this object.
  */
  char * value;
};
/** Defaults-initialized fsl_card_T instance. */
#define fsl_card_T_empty_m {FSL_TAGTYPE_INVALID, NULL, NULL,NULL}
/** Defaults-initialized fsl_card_T instance. */
FSL_EXPORT const fsl_card_T fsl_card_T_empty;

/**
   Types of cherrypick merges.
*/
enum fsl_cherrypick_type_e {
/** Sentinel value. */
FSL_CHERRYPICK_INVALID = 0,
/** Indicates a cherrypick merge. */
FSL_CHERRYPICK_ADD = 1,
/** Indicates a cherrypick backout. */
FSL_CHERRYPICK_BACKOUT = -1
};
typedef enum fsl_cherrypick_type_e fsl_cherrypick_type_e;

/**
   Represents a Q card in a Manifest or Control Artifact.
*/
struct fsl_card_Q {
  /** 0==invalid, negative==backed out, positive=cherrypicked. */
  fsl_cherrypick_type_e type;
  /**
     UUID of the target of the cherrypick. The bytes are owned by
     this object.
  */
  fsl_uuid_str target;
  /**
     UUID of the baseline for the cherrypick. The bytes are owned by
     this object.
  */
  fsl_uuid_str baseline;
};
/** Empty-initialized fsl_card_Q struct. */
#define fsl_card_Q_empty_m {FSL_CHERRYPICK_INVALID, NULL, NULL}
/** Empty-initialized fsl_card_Q struct. */
FSL_EXPORT const fsl_card_Q fsl_card_Q_empty;

/**
   Allocates a new J-card record instance

   On success it returns a new record which must eventually be
   passed to fsl_card_J_free() to free its resources. On
   error (invalid arguments or allocation error) it returns NULL.
   field may not be NULL or empty but value may be either.

   These records are immutable - the API provides no way to change
   them once they are instantiated.
*/
FSL_EXPORT fsl_card_J * fsl_card_J_malloc(bool isAppend,
                                          char const * field,
                                          char const * value);
/**
   Frees a J-card record created by fsl_card_J_malloc().
   Is a no-op if cp is NULL.
*/
FSL_EXPORT void fsl_card_J_free( fsl_card_J * cp );

/**
   Allocates a new fsl_card_T instance. If any of the pointer
   parameters are non-NULL, their values are assumed to be
   NUL-terminated strings, which this function copies.  Returns NULL
   on allocation error.  The returned value must eventually be passed
   to fsl_card_T_clean() or fsl_card_T_free() to free its resources.

   If uuid is not NULL and fsl_is_uuid(uuid) returns false then
   this function returns NULL. If it is NULL and gets assigned
   later, it must conform to fsl_is_uuid()'s rules or downstream
   results are undefined.

   @see fsl_card_T_free()
   @see fsl_card_T_clean()
   @see fsl_deck_T_add()
*/
FSL_EXPORT fsl_card_T * fsl_card_T_malloc(fsl_tagtype_e tagType,
                                          fsl_uuid_cstr uuid,
                                          char const * name,
                                          char const * value);
/**
   If t is not NULL, calls fsl_card_T_clean(t) and then passes t to
   fsl_free().

   @see fsl_card_T_clean()
*/
FSL_EXPORT void fsl_card_T_free(fsl_card_T *t);

/**
   Frees up any memory owned by t and clears out t's state,
   but does not free t.

   @see fsl_card_T_free()
*/
FSL_EXPORT void fsl_card_T_clean(fsl_card_T *t);

/**
   Allocates a new cherrypick record instance. The type argument must
   be one of FSL_CHERRYPICK_ADD or FSL_CHERRYPICK_BACKOUT.  target
   must be a valid UUID string. If baseline is not NULL then it also
   must be a valid UUID.

   On success it returns a new record which must eventually be
   passed to fsl_card_Q_free() to free its resources. On
   error (invalid arguments or allocation error) it returns NULL.

   These records are immutable - the API provides no way to change
   them once they are instantiated.
*/
FSL_EXPORT fsl_card_Q * fsl_card_Q_malloc(fsl_cherrypick_type_e type,
                                          fsl_uuid_cstr target,
                                          fsl_uuid_cstr baseline);
/**
   Frees a cherrypick record created by fsl_card_Q_malloc().
   Is a no-op if cp is NULL.
*/
FSL_EXPORT void fsl_card_Q_free( fsl_card_Q * cp );

/**
   Returns true (non-0) if f is not NULL and f has an opened repo
   which contains a checkin with the given rid, else it returns
   false.

   As a special case, if rid==0 then this only returns true
   if the repository currently has no content in the blob
   table.
*/
FSL_EXPORT char fsl_rid_is_a_checkin(fsl_cx * f, fsl_id_t rid);

/**
   Fetches the list of all directory names for a given checkin record
   id or (if rid is negative) the whole repo over all of its combined
   history. Each name entry in the list is appended to tgt. The
   results are reduced to unique names only and are sorted
   lexically. If addSlash is true then each entry will include a
   trailing slash character, else it will not. The list does not
   include an entry for the top-most directory.

   If rid is less than 0 then the directory list across _all_
   versions is returned. If it is 0 then the current checkout's RID
   is used (if a checkout is opened, otherwise a usage error is
   triggered). If it is positive then only directories for the
   given checkin RID are returned. If rid is specified, it is
   assumed to be the record ID of a commit (manifest) record, and
   it is impossible to distinguish between the results "invalid
   rid" and "empty directory list" (which is a legal result).

   On success it returns 0 and tgt will have a number of (char *)
   entries appended to it equal to the number of subdirectories in
   the repo (possibly 0).

   Returns non-0 on error, FSL_RC_MISUSE if !f, !tgt. On other
   errors error tgt might have been partially populated and the
   list contents should not be considered valid/complete.

   Ownership of the returned strings is transfered to the caller,
   who must eventually free each one using
   fsl_free(). fsl_list_visit_free() is the simplest way to free
   them all at once.
*/
FSL_EXPORT int fsl_repo_dir_names( fsl_cx * f, fsl_id_t rid,
                                   fsl_list * tgt, bool addSlash );


/**
   ZIPs up a copy of the contents of a specific version from f's
   opened repository db. sym is the symbolic name for the checkin
   to ZIP. filename is the name of the ZIP file to output the
   result to. See fsl_zip_writer for details and caveats of this
   library's ZIP creation. If vRootDir is not NULL and not empty
   then each file injected into the ZIP gets that directory
   prepended to its name.

   If progressVisitor is not NULL then it is called once just
   before each file is processed, passed the F-card for the file
   about to be zipped and the progressState parameter. If it
   returns non-0, ZIPping is cancelled and that error code is
   returned. This is intended primarily for providing feedback on
   the update process, but could also be used to cancel the
   operation between files.

   BUG: this function does not honor symlink content in a
   fossil-compatible fashion. If it encounters a symlink entry
   during ZIP generation, it will fail and f's error state will be
   updated with an explanation of this shortcoming.

   @see fsl_zip_writer
   @see fsl_card_F_visitor_f()
*/
FSL_EXPORT int fsl_repo_zip_sym_to_filename( fsl_cx * f, char const * sym,
                                  char const * vRootDir,
                                  char const * fileName,
                                  fsl_card_F_visitor_f progressVisitor,
                                  void * progressState);


/**
   Callback state for use with fsl_repo_extract_f() implementations
   to stream a given version of a repository's file's, one file at a
   time, to a client. Instances are never created by client code,
   only by fsl_repo_extract() and its delegates, which pass them to
   client-provided fsl_repo_extract_f() functions.
*/
struct fsl_repo_extract_state {
  /**
     The associated Fossil context.
  */
  fsl_cx * f;
  /**
     RID of the checkin version for this file. For a given call to
     fsl_repo_extract(), this number will be the same across all
     calls to the callback function.
  */
  fsl_id_t checkinRid;
  /**
     File-level blob.rid for fc. Can be used with, e.g.,
     fsl_mtime_of_manifest_file().
  */
  fsl_id_t fileRid;
  /**
     Client state passed to fsl_repo_extract(). Its interpretation
     is callback-implementation-dependent.
  */
  void * callbackState;
  /**
     The F-card being iterated over. This holds the repo-level
     metadata associated with the file, other than its RID, which is
     available via this->fileRid.

     Deleted files are NOT reported via the extraction process
     because reporting them accurately is trickier and more
     expensive than it could be. Thus this member's uuid field
     will always be non-NULL.

     Certain operations which use this class, e.g. fsl_repo_ckout()
     and fsl_ckout_update(), will temporarily synthesize an F-card to
     represent the state of a file update, in which case this object's
     contents might not 100% reflect any given db-side state. e.g.
     fsl_ckout_update() synthesizes an F-card which reflects the
     current state of a file after applying an update operation to it.
     In such cases, the fCard->uuid may refer to a repository-side
     file even though the hash of the on-disk file contents may differ
     because of, e.g., a merge.
  */
  fsl_card_F const * fCard;

  /**
     If the fsl_repo_extract_opt object which was used to initiate the
     current extraction has the extractContent member set to false,
     this will be a NULL pointer. If it's true, this member points to
     a transient buffer which holds the full, undelta'd/uncompressed
     content of fc's file record. The content bytes are owned by
     fsl_repo_extract() and are invalidated as soon as this callback
     returns, so the callback must copy/consume them immediately if
     needed.
  */
  fsl_buffer const * content;

  /**
     These counters can be used by an extraction callback to calculate
     a progress percentage.
  */
  struct {
    /** The current file number, starting at 1. */
    uint32_t fileNumber;
    /** Total number of files to extract. */
    uint32_t fileCount;
  } count;
};
typedef struct fsl_repo_extract_state fsl_repo_extract_state;

/**
   Initialized-with-defaults fsl_repo_extract_state instance, intended
   for const-copy initialization.
*/
#define fsl_repo_extract_state_empty_m {\
  NULL/*f*/, 0/*checkinRid*/, 0/*fileRid*/, \
  NULL/*state*/, NULL/*fCard*/, NULL/*content*/,    \
  {/*count*/0,0} \
}
/**
   Initialized-with-defaults fsl_repo_extract_state instance,
   intended for non-const copy initialization.
*/
FSL_EXPORT const fsl_repo_extract_state fsl_repo_extract_state_empty;

/**
   A callback type for use with fsl_repo_extract(). See
   fsl_repo_extract_state for the meanings of xstate's various
   members.  The xstate memory must be considered invalidated
   immediately after this function returns, thus implementations
   must copy or consume anything they need from xstate before
   returning.

   Implementations must return 0 on success. As a special case, if
   FSL_RC_BREAK is returned then fsl_repo_extract() will stop
   looping over files but will report it as success (by returning
   0). Any other code causes extraction looping to stop and is
   returned as-is to the caller of fsl_repo_extract().

   When returning an error, the client may use fsl_cx_err_set() to
   populate state->f with a useful error message which will
   propagate back up through the call stack.

   @see fsl_repo_extract()
*/
typedef int (*fsl_repo_extract_f)( fsl_repo_extract_state const * xstate );

/**
   Options for use with fsl_repo_extract().
*/
struct fsl_repo_extract_opt {
  /**
     The version of the repostitory to check out. This must be
     the blob.rid of a checkin artifact.
  */
  fsl_id_t checkinRid;
  /**
     The callback to call for each extracted file in the checkin.
     May not be NULL.
  */
  fsl_repo_extract_f callback;
  /**
     Optional state pointer to pass to the callback when extracting.
     Its interpretation is client-dependent.
  */
  void * callbackState;
  /**
     If true, the fsl_repo_extract_state::content pointer passed to
     the callback will be non-NULL and will contain the content of the
     file. If false, that pointer will be NULL. Such extraction is a
     relatively costly operation, so should only be enabled when
     necessary. Some uses cases can delay this decision until the
     callback and only fetch the content for cases which need it.
  */
  bool extractContent;
};

typedef struct fsl_repo_extract_opt fsl_repo_extract_opt;
/**
   Initialized-with-defaults fsl_repo_extract_opt instance, intended
   for intializing via const-copy initialization.
*/
#define fsl_repo_extract_opt_empty_m \
  {0/*checkinRid*/,NULL/*callback*/, \
   NULL/*callbackState*/,false/*extractContent*/}
/**
   Initialized-with-defaults fsl_repo_extract_opt instance,
   intended for intializing new non-const instances.
*/
FSL_EXPORT const fsl_repo_extract_opt fsl_repo_extract_opt_empty;

/**
   Extracts the contents of a single checkin from a repository,
   sending the appropriate version of each file's contents to a
   client-specified callback.

   For each file in the given checkin, opt->callback() is passed a
   fsl_repo_extract_state instance containing enough information to,
   e.g., unpack the contents to a working directory, add it to a
   compressed archive, or send it to some other destination.

   Returns 0 on success, non-0 on error. It will fail if f has no
   opened repository db.

   If the callback returns any code other than 0 or FSL_RC_BREAK,
   looping over the list of files ends and this function returns
   that value. FSL_RC_BREAK causes looping to stop but 0 is
   returned.

   Files deleted by the given version are NOT reported to the callback
   (because getting sane semantics has proven to be tricker and more
   costly than it's worth).

   See fsl_repo_extract_f() for more details about the semantics of
   the callback. See fsl_repo_extract_opt for the documentation of the
   various options.

   Fossil's internal metadata format guarantees that files will passed
   be passed to the callback in "lexical order" (as defined by
   fossil's manifest format definition). i.e. the files will be passed
   in case-sensitive, alphabetical order. Note that upper-case letters
   sort before lower-case ones.

   Sidebar: this function makes a bitwise copy of the 2nd argument
   before starting its work, just in case the caller gets the crazy
   idea to modify it from the extraction callback. Whether or not
   there are valid/interesting uses for such modification remains to
   be seen. If any are found, this copy behavior may change.
*/
FSL_EXPORT int fsl_repo_extract( fsl_cx * f,
                                 fsl_repo_extract_opt const * opt );

/**
   Equivalent to fsl_tag_rid() except that it takes a symbolic
   artifact name in place of an artifact ID as the third
   argumemnt.

   This function passes symToTag to fsl_sym_to_rid(), and on
   success passes the rest of the parameters as-is to
   fsl_tag_rid(). See that function the semantics of the other
   arguments and the return value, as well as a description of the
   side effects.
*/
FSL_EXPORT int fsl_tag_sym( fsl_cx * f, fsl_tagtype_e tagType,
                 char const * symToTag, char const * tagName,
                 char const * tagValue, char const * userName,
                 double mtime, fsl_id_t * newId );

/**
   Adds a control record to f's repositoriy that either creates or
   cancels a tag.

   artifactRidToTag is the RID of the record to be tagged.

   tagType is the type (add, cancel, or propagate) of tag.

   tagName is the name of the tag. Must not be NULL/empty.

   tagValue is the optional value for the tag. May be NULL.

   userName is the user's name to apply to the artifact. May not be
   empty/NULL. Use fsl_guess_user_name() to try to figure out a
   proper user name based on the environment. See also:
   fsl_cx_user_get(), but note that the application must first
   use fsl_cx_user_set() to set a context's user name.

   mtime is the Julian Day timestamp for the new artifact. Pass a
   value <=0 to use the current time.

   If newId is not NULL then on success the rid of the new tag control
   artifact is assigned to *newId.

   Returns 0 on success and has about a million and thirteen
   possible error conditions. On success a new artifact record is
   written to the db, its RID being written into newId as described
   above.

   If the artifact being tagged is private, the new tag is also
   marked as private.

*/
FSL_EXPORT int fsl_tag_an_rid( fsl_cx * f, fsl_tagtype_e tagType,
                 fsl_id_t artifactRidToTag, char const * tagName,
                 char const * tagValue, char const * userName,
                 double mtime, fsl_id_t * newId );

/**
    Searches for a repo.tag entry given name in the given context's
    repository db. If found, it returns the record's id. If no
    record is found and create is true (non-0) then a tag is created
    and its entry id is returned. Returns 0 if it finds no entry, a
    negative value on error. On db-level error, f's error state is
    updated.
*/
FSL_EXPORT fsl_id_t fsl_tag_id( fsl_cx * f, char const * tag, bool create );


/**
   Returns true if the checkin with the given rid is a leaf, false if
   not. Returns false if f has no repo db opened, the query fails
   (likely indicating that it is not a repository db), or just about
   any other conceivable non-success case.

   A leaf, by the way, is a commit which has no children in the same
   branch.

   Sidebar: this function calculates whether the RID is a leaf, as
   opposed to checking the "static" (pre-calculated) list of leaves in
   the [leaf] table.
*/
FSL_EXPORT bool fsl_rid_is_leaf(fsl_cx * f, fsl_id_t rid);

/**
   Counts the number of primary non-branch children for the given
   check-in.

   A primary child is one where the parent is the primary parent, not
   a merge parent.  A "leaf" is a node that has zero children of any
   kind.  This routine counts only primary children.

   A non-branch child is one which is on the same branch as the parent.

   Returns a negative value on error.
*/
FSL_EXPORT fsl_int_t fsl_count_nonbranch_children(fsl_cx * f, fsl_id_t rid);

/**
   Looks for the delta table record where rid==deltaRid, and
   returns that record's srcid via *rv. Returns 0 on success, non-0
   on error. If no record is found, *rv is set to 0 and 0 is
   returned (as opposed to FSL_RC_NOT_FOUND) because that generally
   simplifies the error checking.
*/
FSL_EXPORT int fsl_delta_src_id( fsl_cx * f, fsl_id_t deltaRid, fsl_id_t * rv );


/**
   Return true if the given artifact ID should is listed in f's
   shun table, else false.
*/
FSL_EXPORT int fsl_uuid_is_shunned(fsl_cx * f, fsl_uuid_cstr zUuid);


/**
   Compute the "mtime" of the file given whose blob.rid is "fid"
   that is part of check-in "vid".  The mtime will be the mtime on
   vid or some ancestor of vid where fid first appears. Note that
   fossil does not track the "real" mtimes of files, it only
   computes reasonable estimates for those files based on the
   timestamps of their most recent checkin in the ancestry of vid.

   On success, if pMTime is not null then the result is written to
   *pMTime.

   If fid is 0 or less then the checkin time of vid is written to
   pMTime (this is a much less expensive operation, by the way).
   In this particular case, FSL_RC_NOT_FOUND is returned if vid is
   not a valid checkin version.

   Returns 0 on success, non-0 on error. Returns FSL_RC_NOT_FOUND
   if fid is not found in vid.

   This routine is much more efficient if used to answer several
   queries in a row for the same manifest (the vid parameter). It
   is least efficient when it is passed intermixed manifest IDs,
   e.g. (1, 3, 1, 4, 1,...). This is a side-effect of the caching
   used in the computation of ancestors for a given vid.
*/
FSL_EXPORT int fsl_mtime_of_manifest_file(fsl_cx * f, fsl_id_t vid, fsl_id_t fid, fsl_time_t *pMTime);

/**
   A convenience form of fsl_mtime_of_manifest_file() which looks up
   fc's RID based on its UUID. vid must be the RID of the checkin
   version fc originates from. See fsl_mtime_of_manifest_file() for
   full details - this function simply calculates the 3rd argument
   for that one.
*/
FSL_EXPORT int fsl_mtime_of_F_card(fsl_cx * f, fsl_id_t vid, fsl_card_F const * fc, fsl_time_t *pMTime);

/**
   Ensures that the given list has capacity for at least n entries. If
   the capacity is currently equal to or less than n, this is a no-op
   unless n is 0, in which case li->list is freed and the list is
   zeroed out. Else li->list is expanded to hold at least n
   elements. Returns 0 on success, FSL_RC_OOM on allocation error.
 */
int fsl_card_F_list_reserve( fsl_card_F_list * li, uint32_t n );

/**
   Frees all memory owned by li and the F-cards it contains. Does not
   free the li pointer.
*/
void fsl_card_F_list_finalize( fsl_card_F_list * li );

/**
   Holds options for use with fsl_branch_create().
*/
struct fsl_branch_opt {
  /**
     The checkin RID from which the branch should originate.
  */
  fsl_id_t basisRid;
  /**
     The name of the branch. May not be NULL or empty.
  */
  char const * name;
  /**
     User name for the branch. If NULL, fsl_cx_user_get() will
     be used.
  */ 
  char const * user;
  /**
     Optional comment (may be NULL). If NULL or empty, a default
     comment is generated (because fossil requires a non-empty
     comment string).
  */
  char const * comment;
  /**
     Optional background color for the fossil(1) HTML timeline
     view.  Must be in \#RRGGBB format, but this API does not
     validate it as such.
  */
  char const * bgColor;
  /**
     The julian time of the branch. If 0 or less, default is the
     current time.
  */
  double mtime;
  /**
     If true, the branch will be marked as private.
  */
  char isPrivate;
};
typedef struct fsl_branch_opt fsl_branch_opt;
#define fsl_branch_opt_empty_m {                \
    0/*basisRid*/, NULL/*name*/,                \
      NULL/*user*/, NULL/*comment*/,            \
      NULL/*bgColor*/,                          \
      0.0/*mtime*/, 0/*isPrivate*/              \
      }
FSL_EXPORT const fsl_branch_opt fsl_branch_opt_empty;

/**
   Creates a new branch in f's repository. The 2nd paramter holds
   the options describing the branch. The 3rd parameter may be
   NULL, but if it is not then on success the RID of the new
   manifest is assigned to *newRid.

   In Fossil branches are implemented as tags. The branch name
   provided by the client will cause the creation of a tag with
   name name plus a "sym-" prefix to be created (if needed).
   "sym-" denotes that it is a "symbolic tag" (fossil's term for
   "symbolic name applying to one or more checkins,"
   i.e. branches).

   Creating a branch cancels all other branch tags which the new
   branch would normally inherit.

   Returns 0 on success, non-0 on error. 
*/
FSL_EXPORT int fsl_branch_create(fsl_cx * f, fsl_branch_opt const * opt, fsl_id_t * newRid );


/**
   Tries to determine the [filename.fnid] value for the given
   filename.  Returns a positive value if it finds one, 0 if it
   finds none, and some unspecified negative value(s) for any sort
   of error. filename must be a normalized, relative filename (as it
   is recorded by a repo).
*/
FSL_EXPORT fsl_id_t fsl_repo_filename_fnid( fsl_cx * f, char const * filename );


/**
   Imports content to f's opened repository's BLOB table using a
   client-provided input source. f must have an opened repository
   db. inFunc is the source of the data and inState is the first
   argument passed to inFunc(). If inFunc() succeeds in fetching all
   data (i.e. if it always returns 0 when called by this function)
   then that data is inserted into the blob table _if_ no existing
   record with the same hash is already in the table. If such a record
   exists, it is assumed that the content is identical and this
   function has no side-effects vis-a-vis the db in that case.

   If rid is not NULL then the BLOB.RID record value (possibly of an
   older record!) is stored in *rid.  If uuid is not NULL then the
   BLOB.UUID record value is stored in *uuid and the caller takes
   ownership of those bytes, which must eventually be passed to
   fsl_free() to release them.

   rid and uuid are only modified on success and only if they are
   not NULL.

   Returns 0 on success, non-0 on error. For errors other than basic
   argument validation and OOM conditions, f's error state is
   updated with a description of the problem. Returns FSL_RC_MISUSE
   if either f or inFunc are NULL. Whether or not inState may be
   NULL depends on inFunc's concrete implementation.

   Be aware that BLOB.RID values can (but do not necessarily) change
   in the life of a repod db (via a reconstruct, a full re-clone, or
   similar, or simply when referring to different clones of the same
   repo). Thus clients should always store the full UUID, as opposed
   to the RID, for later reference. RIDs should, in general, be
   treated as session-transient values. That said, for purposes of
   linking tables in the db, the RID is used exclusively (clients are
   free to link their own extension tables using UUIDs, but doing so
   has a performance penalty comared to RIDs). For long-term storage
   of external links, and to guaranty that the data be usable with
   other copies of the same repo, the UUID is required.

   Note that Fossil may deltify, compress, or otherwise modify
   content on its way into the blob table, and it may even modify
   content long after its insertion (e.g. to make it a delta against
   a newer version). Thus clients should normally never try
   to read back the blob directly from the database, but should
   instead read it using fsl_content_get().

   That said: this routine has no way of associating and older version
   (if any) of the same content with this newly-imported version, and
   therefore cannot delta-compress the older version.

   Maintenance reminder: this is basically just a glorified form of
   the internal fsl_content_put(). Interestingly, fsl_content_put()
   always sets content to public (by default - the f object may
   override that later). It is not yet clear whether this routine
   needs to have a flag to set the blob private or not. Generally
   speaking, privacy is applied to fossil artifacts, as opposed to
   content blobs.

   @see fsl_repo_import_buffer()
*/
FSL_EXPORT int fsl_repo_import_blob( fsl_cx * f, fsl_input_f inFunc,
                                     void * inState, fsl_id_t * rid,
                                     fsl_uuid_str * uuid );

/**
   A convenience form of fsl_repo_import_blob(), equivalent to:

   @code
   fsl_repo_import_blob(f, fsl_input_f_buffer, bIn, rid, uuid )
   @endcode

   except that (A) bIn is const in this call and non-const in the
   other form (due to cursor traversal requirements) and (B) it
   returns FSL_RC_MISUSE if bIn is NULL.
*/
FSL_EXPORT int fsl_repo_import_buffer( fsl_cx * f, fsl_buffer const * bIn,
                                       fsl_id_t * rid, fsl_uuid_str * uuid );

/**
   Resolves client-provided symbol as an artifact's db record ID.
   f must have an opened repository db, and some symbols can only
   be looked up if it has an opened checkout (see the list below).

   Returns 0 and sets *rv to the id if it finds an unambiguous
   match.

   Returns FSL_RC_MISUSE if !f, !sym, !*sym, or !rv.

   Returns FSL_RC_NOT_A_REPO if f has no opened repository.

   Returns FSL_RC_AMBIGUOUS if sym is a partial UUID which matches
   multiple full UUIDs.

   Returns FSL_RC_NOT_FOUND if it cannot find anything.

   Symbols supported by this function:

   - SHA1/3 hash
   - SHA1/3 hash prefix of at least 4 characters
   - Symbolic Name
   - "tag:" + symbolic name
   - Date or date-time 
   - "date:" + Date or date-time
   - symbolic-name ":" date-time
   - "tip"

   - "rid:###" resolves to the hash of blob.rid ### if that RID is in
   the database

   The following additional forms are available in local checkouts:

   - "current"
   - "prev" or "previous"
   - "next"

   The following prefix may be applied to the above to modify how
   they are resolved:

   - "root:" prefix resolves to the checkin of the parent branch from
   which the record's branch divered. i.e. the version from which it
   was branched. In the trunk this will always resolve to the first
   checkin.

   - "merge-in:" TODO - document this once its implications are
   understood.

   If type is not FSL_SATYPE_ANY then it will only match artifacts
   of the specified type. In order to resolve arbitrary UUIDs, e.g.
   those of arbitrary blob content, type needs to be
   FSL_SATYPE_ANY.

*/
FSL_EXPORT int fsl_sym_to_rid( fsl_cx * f, char const * sym, fsl_satype_e type,
                               fsl_id_t * rv );

/**
   Similar to fsl_sym_to_rid() but on success it returns a UUID string
   by assigning it to *rv (if rv is not NULL). If rid is not NULL then
   on success the db record ID corresponding to the returned UUID is
   assigned to *rid. The caller must eventually free the returned
   string memory by passing it to fsl_free(). Returns 0 if it finds a
   match and any number of result codes on error.
*/
FSL_EXPORT int fsl_sym_to_uuid( fsl_cx * f, char const * sym,
                                fsl_satype_e type, fsl_uuid_str * rv,
                                fsl_id_t * rid );


/**
   Searches f's repo database for the a blob with the given uuid
   (any unique UUID prefix). On success a positive record ID is
   returned. On error one of several unspecified negative values is
   returned. If no uuid match is found 0 is returned.

   Error cases include: either argument is NULL, uuid does not
   appear to be a full or partial UUID (or is too long),
   uuid is ambiguous (try providing a longer one)

   This implementation is more efficient when given a full,
   valid UUID (one for which fsl_is_uuid() returns true).
*/
FSL_EXPORT fsl_id_t fsl_uuid_to_rid( fsl_cx * f, char const * uuid );

/**
   The opposite of fsl_uuid_to_rid(), this returns the UUID string
   of the given blob record ID. Ownership of the string is passed
   to the caller and it must eventually be freed using
   fsl_free(). Returns NULL on error (invalid arguments or f has no
   repo opened) or if no blob record is found. If no record is
   found, f's error state is updated with an explanation of the
   problem.
*/
FSL_EXPORT fsl_uuid_str fsl_rid_to_uuid(fsl_cx * f, fsl_id_t rid);

/**
   Works like fsl_rid_to_uuid() but assigns the UUID to the given
   buffer, re-using its memory, if any. Returns 0 on success,
   FSL_RC_MISUSE if rid is not positive, FSL_RC_OOM on allocation
   error, and FSL_RC_NOT_FOUND if no blob entry matching the given rid
   is found.
*/
FSL_EXPORT int fsl_rid_to_uuid2(fsl_cx * f, fsl_id_t rid, fsl_buffer *uuid);

/**
   This works identically to fsl_rid_to_uuid() except that it will
   only resolve to a UUID if an artifact matching the given type has
   that UUID. If no entry is found, f's error state gets updated
   with a description of the problem.

   This can be used to distinguish artifact UUIDs from file blob
   content UUIDs by passing the type FSL_SATYPE_ANY. A non-artifact
   blob will return NULL in that case, but any artifact type will
   match (assuming rid is valid).
*/
FSL_EXPORT fsl_uuid_str fsl_rid_to_artifact_uuid(fsl_cx * f, fsl_id_t rid,
                                                 fsl_satype_e type);
/**
   Returns the raw SQL code for a Fossil global config database.

   TODO: add optional (fsl_size_t*) to return the length.
*/
FSL_EXPORT char const * fsl_schema_config();

/**
   Returns the raw SQL code for the "static" parts of a Fossil
   repository database. These are the parts which are immutable
   (for the most part) between Fossil versions. They change _very_
   rarely.

   TODO: add optional (fsl_size_t*) to return the length.
*/
FSL_EXPORT char const * fsl_schema_repo1();

/**
   Returns the raw SQL code for the "transient" parts of a Fossil
   repository database - any parts which can be calculated via data
   held in the primary "static" schemas. These parts are
   occassionally recreated, e.g. via a 'rebuild' of a repository.

   TODO: add optional (fsl_size_t*) to return the length.
*/
FSL_EXPORT char const * fsl_schema_repo2();

/**
   Returns the raw SQL code for a Fossil checkout database.

   TODO: add optional (fsl_size_t*) to return the length.
*/
FSL_EXPORT char const * fsl_schema_ckout();

/**
   Returns the raw SQL code for a Fossil checkout db's
   _default_ core ticket-related tables.

   TODO: add optional (fsl_size_t*) to return the length.

   @see fsl_cx_schema_ticket()
*/
FSL_EXPORT char const * fsl_schema_ticket();

/**
   Returns the raw SQL code for the "forum" parts of a Fossil
   repository database.

   TODO: add optional (fsl_size_t*) to return the length.
*/
FSL_EXPORT char const * fsl_schema_forum();

/**
   If f's opened repository has a non-empty config entry named
   'ticket-table', this returns its text via appending it to
   pOut. If no entry is found, fsl_schema_ticket() is appended to
   pOut.

   Returns 0 on success. On error the contents of pOut must not be
   considered valid but pOut might be partially populated.
*/
FSL_EXPORT int fsl_cx_schema_ticket(fsl_cx * f, fsl_buffer * pOut);

/**
   Returns the raw SQL code for Fossil ticket reports schemas.
   This gets installed as needed into repository databases.

   TODO: add optional (fsl_size_t*) to return the length.
*/
FSL_EXPORT char const * fsl_schema_ticket_reports();

/**
   This is a wrapper around fsl_cx_hash_buffer() which looks for a
   matching artifact for the given input blob. It first hashes src
   using f's "alternate" hash and then, if no match is found, tries
   again with f's preferred hash.

   On success (a match is found):

   - Returns 0.

   - If ridOut is not NULL, *ridOut is set to the RID of the matching blob.

   - If hashOut is not NULL, *hashOut is set to the hash of the
   blob. Its ownership is transferred to the caller, who must
   eventually pass it to fsl_free().

   If no matching blob is found in the repository, FSL_RC_NOT_FOUND is
   returned (but f's error state is not annotated with more
   information). Returns FSL_RC_NOT_A_REPO if f has no repository
   opened. For more serious errors, e.g. allocation error or db
   problems, another (more serious) result code is returned,
   e.g. FSL_RC_OOM or FSL_RC_DB.

   If FSL_RC_NOT_FOUND is returned and hashOut is not NULL, *hashOut
   is set to the value of f's preferred hash. *ridOut is only modified
   if 0 is returned, in which case *ridOut will have a positive value.
*/
FSL_EXPORT int fsl_repo_blob_lookup( fsl_cx * f, fsl_buffer const * src, fsl_id_t * ridOut,
                                     fsl_uuid_str * hashOut );

/**
   Returns true if the specified file name ends with any reserved
   name, e.g.: _FOSSIL_ or .fslckout.

   For the sake of efficiency, zFilename must be a canonical name,
   e.g. an absolute or checkout-relative path using only forward slash
   ('/') as a directory separator.

   On Windows builds, this also checks for reserved Windows filenames,
   e.g. "CON" and "PRN".

   nameLen must be the length of zFilename. If it is negative,
   fsl_strlen() is used to calculate it.
*/
FSL_EXPORT bool fsl_is_reserved_fn(const char *zFilename,
                                   fsl_int_t nameLen );

/**
   Uses fsl_is_reserved_fn() to determine whether the filename part of
   zPath is legal for use as an in-repository filename. If it is, 0 is
   returned, else FSL_RC_RANGE (or FSL_RC_OOM) is returned and f's
   error state is updated to indicate the nature of the problem. nFile
   is the length of zPath. If negative, fsl_strlen() is used to
   determine its length.

   If relativeToCwd is true then zPath, if not absolute, is
   canonicalized as if were relative to the current working directory
   (see fsl_getcwd()), else it is assumed to be relative to the
   current checkout (if any - falling back to the current working
   directory). This flag is only relevant if zPath is not absolute and
   if f has a checkout opened. An absolute zPath is used as-is and if
   no checkout is opened then relativeToCwd is always treated as if it
   were true.

   This routine does not validate that zPath lives inside a checkout
   nor that the file actually exists. It does only name comparison and
   only uses the filesystem for purposes of canonicalizing (if needed)
   zPath.

   This routine does not require that f have an opened repo, but if it
   does then this routine compares the canonicalized forms of both the
   repository db and the given path and fails if zPath refers to the
   repository db. Be aware that the relativeToCwd flag may influence
   that test.

   TODO/FIXME: if f's 'manifest' config setting is set to true AND
   zPath refers to the top of the checkout root, treat the files
   (manifest, manifest.uuid, manifest.tags) as reserved. If it is a
   string with any of the letters "r", "u", or "t", check only the
   file(s) which those letters represent (see
   add.c:fossil_reserved_name() in fossil). Apply these only at the top
   of the tree - allow them in subdirectories.
*/
FSL_EXPORT int fsl_reserved_fn_check(fsl_cx *f, const char *zPath,
                                     fsl_int_t nFile, bool relativeToCwd);

/**
   Recompute/rebuild the entire repo.leaf table. This is not normally
   needed, as leaf tracking is part of the crosslinking process, but
   "just in case," here it is.

   This can supposedly be expensive (in time) for a really large
   repository. Testing implies otherwise.

   Returns 0 on success. Error may indicate that f has no repo db
   opened.  On error f's error state may be updated.
*/
FSL_EXPORT int fsl_repo_leaves_rebuild(fsl_cx * f);  

/**
   Flags for use with fsl_leaves_compute().
*/
enum fsl_leaves_compute_e {
/**
   Compute all leaves regardless of the "closed" tag.
*/
FSL_LEAVES_COMPUTE_ALL = 0,
/**
   Compute only leaves without the "closed" tag.
*/
FSL_LEAVES_COMPUTE_OPEN = 1,
/**
   Compute only leaves with the "closed" tag.
*/
FSL_LEAVES_COMPUTE_CLOSED = 2
};
typedef enum fsl_leaves_compute_e fsl_leaves_compute_e;

/**
   Creates a temporary table named "leaves" if it does not already
   exist, else empties it. Populates that table with the RID of all
   check-ins that are leaves which are descended from the checkin
   referred to by vid.

   A "leaf" is a check-in that has no children in the same branch.
   There is a separate permanent table named [leaf] that contains all
   leaves in the tree. This routine is used to compute a subset of
   that table consisting of leaves that are descended from a single
   check-in.
   
   The leafMode flag determines behavior associated with the "closed"
   tag, as documented for the fsl_leaves_compute_e enum.

   If vid is <=0 then this function, after setting up or cleaning out
   the [leaves] table, simply copies the list of leaves from the
   repository's pre-computed [leaf] table (see
   fsl_repo_leaves_rebuild()).

   @see fsl_leaves_computed_has()
   @see fsl_leaves_computed_count()
   @see fsl_leaves_computed_latest()
   @see fsl_leaves_computed_cleanup()
*/
FSL_EXPORT int fsl_leaves_compute(fsl_cx * f, fsl_id_t vid,
                                  fsl_leaves_compute_e leafMode);

/**
   Requires that a prior call to fsl_leaves_compute() has succeeded,
   else results are undefined.

   Returns true if the leaves list computed by fsl_leaves_compute() is
   not empty, else false. This is more efficient than checking
   against fsl_leaves_computed_count()>0.
*/
FSL_EXPORT bool fsl_leaves_computed_has(fsl_cx * f);

/**
   Requires that a prior call to fsl_leaves_compute() has succeeded,
   else results are undefined.

   Returns a count of the leaves list computed by
   fsl_leaves_compute(), or a negative value if a db-level error is
   encountered. On errors other than FSL_RC_OOM, f's error state will
   be updated with information about the error.
*/
FSL_EXPORT fsl_int_t fsl_leaves_computed_count(fsl_cx * f);

/**
   Requires that a prior call to fsl_leaves_compute() has succeeded,
   else results are undefined.

   Returns the RID of the most recent checkin from those computed by
   fsl_leaves_compute(), 0 if no entries are found, or a negative
   value if a db-level error is encountered. On errors other than
   FSL_RC_OOM, f's error state will be updated with information about
   the error.
*/
FSL_EXPORT fsl_id_t fsl_leaves_computed_latest(fsl_cx * f);

/**
   Cleans up any db-side resources created by fsl_leaves_compute().
   e.g. drops the temporary table created by that routine. Any errors
   are silenty ignored.
*/
FSL_EXPORT void fsl_leaves_computed_cleanup(fsl_cx * f);

/**
   Returns true if f's current repository has the
   forbid-delta-manifests setting set to a truthy value. Results are
   undefined if f has no opened repository. Some routines behave
   differently if this setting is enabled. e.g. fsl_checkin_commit()
   will never generate a delta manifest and fsl_deck_save() will
   refuse to save a delta. This does not affect parsing or deltas or
   those which are injected into the db via lower-level means (e.g. a
   direct blob import or from a remote sync).

   Results are undefined if f has no opened repository.
*/
FSL_EXPORT bool fsl_repo_forbids_delta_manifests(fsl_cx * f);

#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_REPO_H_INCLUDED */
