/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_CHECKOUT_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_CHECKOUT_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).
*/

/** @file fossil-checkout.h

    fossil-checkout.h declares APIs specifically dealing with
    checkout-side state, as opposed to purely repository-db-side state
    or non-content-related APIs.
*/

#include "fossil-db.h" /* MUST come first b/c of config macros */
#include "fossil-repo.h"

#if defined(__cplusplus)
extern "C" {
#endif


/**
   Returns version information for the current checkout.

   If f has an opened checkout then...

   If uuid is not NULL then *uuid is set to the UUID of the opened
   checkout, or NULL if there is no checkout. If rid is not NULL, *rid
   is set to the record ID of that checkout, or 0 if there is no
   checkout (or the current checkout is from an empty repository). The
   returned uuid bytes and rid are owned by f and valid until the
   library updates its checkout state to a newer checkout version
   (essentially unpredictably). When in doubt about lifetime issues,
   copy the UUID immediately after calling this if they will be needed
   later.

   Corner case: a new repo with no checkins has an RID of 0 and a UUID
   of NULL. That does not happen with fossil-generated repositories,
   as those always "seed" the database with an initial commit artifact
   containing no files.
*/
FSL_EXPORT void fsl_ckout_version_info(fsl_cx *f, fsl_id_t * rid,
                                       fsl_uuid_cstr * uuid );

/**
   Given a fsl_cx with an opened checkout, and a filename, this
   function canonicalizes zOrigName to a form suitable for use as
   an in-repo filename, _appending_ the results to pOut. If pOut is
   NULL, it performs its normal checking but does not write a
   result, other than to return 0 for success.

   As a special case, if zOrigName refers to the top-level checkout
   directory, it resolves to either "." or "./", depending on whether
   zOrigName contains a trailing slash.

   If relativeToCwd is true then the filename is canonicalized
   based on the current working directory (see fsl_getcwd()),
   otherwise f's current checkout directory is used as the virtual
   root.

   If the input name contains a trailing slash, it is retained in
   the output sent to pOut except in the top-dir case mentioned
   above.

   Returns 0 on success, meaning that the value appended to pOut
   (if not NULL) is a syntactically valid checkout-relative path.

   Returns FSL_RC_RANGE if zOrigName points to a path outside
   of f's current checkout root.

   Returns FSL_RC_NOT_A_CKOUT if f has no checkout opened.

   Returns FSL_RC_MISUSE if !zOrigName, FSL_RC_OOM on an allocation
   error.

   This function does not validate whether or not the file actually
   exists, only that its name is potentially valid as a filename
   for use in a checkout (though other, downstream rules might prohibit that, e.g.
   the filename "..../...." is not valid but is not seen as invalid by
   this function). (Reminder to self: we could run the end result through
   fsl_is_simple_pathname() to catch that?)
*/
FSL_EXPORT int fsl_ckout_filename_check( fsl_cx * f, bool relativeToCwd,
                                         char const * zOrigName, fsl_buffer * pOut );

/**
   Callback type for use with fsl_ckout_manage_opt(). It should
   inspect the given filename using whatever criteria it likes, set
   *include to true or false to indicate whether the filename is okay
   to include the current add-file-to-repo operation, and return 0.

   If it returns non-0 the add-file-to-repo process will end and that
   error code will be reported to its caller. Such result codes must
   come from the FSL_RC_xxx family.

   It will be passed a name which is relative to the top-most checkout
   directory.

   The final argument is not used by the library, but is passed on
   as-is from the fsl_ckout_manage_opt::callbackState pointer which
   is passed to fsl_ckout_manage().
*/
typedef int (*fsl_ckout_manage_f)(const char *zFilename,
                                    bool *include,
                                    void *state);
/**
   Options for use with fsl_ckout_manage().
*/
struct fsl_ckout_manage_opt {
  /**
     The file or directory name to add. If it is a directory, the add
     process will recurse into it.
  */
  char const * filename;
  /**
     Whether to evaluate the given name as relative to the current working
     directory or to the current checkout root.

     This makes a subtle yet important difference in how the name is
     resolved. CLI apps which take file names from the user from
     within a checkout directory will generally want to set
     relativeToCwd to true. GUI apps, OTOH, will possibly need it to
     be false, depending on how they resolve and pass on the
     filenames.
  */
  bool relativeToCwd;
  /**
     Whether or not to check the name(s) against the 'ignore-globs'
     config setting (if set).
  */     
  bool checkIgnoreGlobs;
  /**
     Optional predicate function which may be called for each
     to-be-added filename. It is only called if:

     - It is not NULL (obviously) and...

     - The file is not already in the checkout database and...

     - The is-internal-name check passes (see
     fsl_reserved_fn_check()) and...

     - If checkIgnoreGlobs is false or the name does not match one of
     the ignore-globs values.

     The name it is passed is relative to the checkout root.

     Because the callback is called only if other options have not
     already excluded the file, the client may use the callback to
     report to the user (or otherwise record) exactly which files
     get added.
  */
  fsl_ckout_manage_f callback;

  /**
     State to be passed to this->callback.
  */
  void * callbackState;

  /**
     These counts are updated by fsl_ckout_manage() to report
     what it did.
  */
  struct {
    /**
       Number of files actually added by fsl_ckout_manage().
    */
    uint32_t added;
    /**
       Number of files which were requested to be added but were only
       updated because they had previously been added. Updates set the
       vfile table entry's current mtime, executable-bit state, and
       is-it-a-symlink state. (That said, this code currently ignores
       symlinks altogether!)
    */
    uint32_t updated;
    /**
       The number of files skipped over for addition. This includes
       files which meet any of these criteria:

       - fsl_reserved_fn_check() fails.

       - If the checkIgnoreGlobs option is true and a filename matches
         any of those globs.

       - The client-provided callback says not to include the file.
    */
    uint32_t skipped;
  } counts;
};
typedef struct fsl_ckout_manage_opt fsl_ckout_manage_opt;
/**
   Initialized-with-defaults fsl_ckout_manage_opt instance,
   intended for use in const-copy initialization.
*/
#define fsl_ckout_manage_opt_empty_m {\
    NULL/*filename*/, true/*relativeToCwd*/, true/*checkIgnoreGlobs*/, \
    NULL/*callback*/, NULL/*callbackState*/,                         \
    {/*counts*/ 0/*added*/, 0/*updated*/, 0/*skipped*/}               \
  }
/**
   Initialized-with-defaults fsl_ckout_manage_opt instance,
   intended for use in non-const copy initialization.
*/
FSL_EXPORT const fsl_ckout_manage_opt fsl_ckout_manage_opt_empty;

/**
   Adds the given filename or directory (recursively) to the current
   checkout vfile list of files as a to-be-added file, or updates an
   existing record if one exists.

   This function ensures that opt->filename gets canonicalized and can
   be found under the checkout directory, and fails if no such file
   exists (checking against the canonicalized name). Filenames are all
   filtered through fsl_reserved_fn_check() and may have other filters
   applied to them, as determined by the options object.

   Each filename which passes through the filters is passed to
   the opt->callback (if not NULL), which may perform a final
   filtering check and/or alert the client about the file being
   queued.

   The options object is non-const because this routine updates
   opt->counts when it adds, updates, or skips a file. On each call,
   it updated opt->counts without resetting it (as this function is
   typically called in a loop). This function does not modify any
   other entries of that object and it requires that the object not be
   modified (e.g. via opt->callback()) while it is recursively
   processing. To reset the counts between calls, if needed:

   @code
   opt->counts = fsl_ckout_manage_opt_empty.counts;
   @endcode

   Returns 0 on success, non-0 on error.

   Files queued for addition this way can be unqueued before they are
   committed using fsl_ckout_unmanage().

   @see fsl_ckout_unmanage()
   @see fsl_reserved_fn_check()
*/
FSL_EXPORT int fsl_ckout_manage( fsl_cx * f,
                                   fsl_ckout_manage_opt * opt );


/**
   Callback type for use with fsl_ckout_unmanage(). It is called
   by the removal process, immediately after a file is "removed"
   from SCM management (a.k.a. when the file becomes "unmanaged").

   If it returns non-0 the unmanage process will end and that
   error code will be reported to its caller. Such result codes must
   come from the FSL_RC_xxx family.

   It will be passed a name which is relative to the top-most checkout
   directory. The client is free to unlink the file from the filesystem
   if they like - the library does not do so automatically

   The final argument is not used by the library, but is passed on
   as-is from the callbackState pointer which
   is passed to fsl_ckout_unmanage().
*/
typedef int (*fsl_ckout_unmanage_f)(const char *zFilename, void *state);

/**
   Options for use with fsl_ckout_unmanage().
*/
struct fsl_ckout_unmanage_opt {
  /**
     The file or directory name to add. If it is a directory, the add
     process will recurse into it. See also this->vfileIds.
  */
  char const * filename;
  /**
     An alternative to assigning this->filename is to point
     this->vfileIds to a bag of vfile.id values. If this member is not
     NULL, fsl_ckout_revert() will ignore this->filename.

     @see fsl_filename_to_vfile_ids()
  */
  fsl_id_bag const * vfileIds;
  /**
     Whether to evaluate this->filename as relative to the current
     working directory (true) or to the current checkout root
     (false). This is ignored when this->vfileIds is not NULL.

     This makes a subtle yet important difference in how the name is
     resolved. CLI apps which take file names from the user from
     within a checkout directory will generally want to set
     relativeToCwd to true. GUI apps, OTOH, will possibly need it to
     be false, depending on how they resolve and pass on the
     filenames.
  */
  bool relativeToCwd;
  /**
     If true, fsl_vfile_changes_scan() is called to ensure that
     the filesystem and vfile tables agree. If the client code has
     called that function, or its equivalent, since any changes were
     made to the checkout then this may be set to false to speed up
     the rm process.
  */
  bool scanForChanges;
  /**
     Optional predicate function which will be called after each
     file is made unmanaged.

     The name it is passed is relative to the checkout root.
  */
  fsl_ckout_unmanage_f callback;
  /**
     State to be passed to this->callback.
  */
  void * callbackState;

};
typedef struct fsl_ckout_unmanage_opt fsl_ckout_unmanage_opt;
/**
   Initialized-with-defaults fsl_ckout_unmanage_opt instance,
   intended for use in const-copy initialization.
*/
#define fsl_ckout_unmanage_opt_empty_m {\
    NULL/*filename*/, NULL/*vfileIds*/,\
    true/*relativeToCwd*/,true/*scanForChanges*/, \
    NULL/*callback*/, NULL/*callbackState*/ \
}
/**
   Initialized-with-defaults fsl_ckout_unmanage_opt instance,
   intended for use in non-const copy initialization.
*/
FSL_EXPORT const fsl_ckout_unmanage_opt fsl_ckout_unmanage_opt_empty;

/**
   The converse of fsl_ckout_manage(), this queues a file for removal
   from the current checkout. Unlike fsl_ckout_manage(), this routine
   does not ensure that opt->filename actually exists - it only
   normalizes zFilename into its repository-friendly form and passes
   it through the vfile table.

   If opt->filename refers to a directory then this operation queues
   all files under that directory (recursively) for removal. In this
   case, it is irrelevant whether or not opt->filename ends in a
   trailing slash.

   Returns 0 on success, any of a number of non-0 codes on error.
   Returns FSL_RC_MISUSE if !opt->filename or !*opt->filename.
   Returns FSL_RC_NOT_A_CKOUT if f has no opened checkout.

   If opt->callback is not NULL, it is called for each
   newly-unamanaged entry. The intention is to provide it the
   opportunity to notify the user, record the filename for later use,
   remove the file from the filesystem, etc. If it returns non-0, the
   unmanaging process will fail with that code and any pending
   transaction will be placed into a rollback state.

   This routine does not actually remove any files from the
   filesystem, it only modifies the vfile table entry so that the
   file(s) will be removed from the SCM by the commit process. If
   opt->filename is an entry which was previously
   fsl_ckout_manage()'d, but not yet committed, or any such entries
   are found under directory opt->filename, they are removed from the
   vfile table. i.e. this effective undoes the add operation.

   @see fsl_ckout_manage()
*/
FSL_EXPORT int fsl_ckout_unmanage( fsl_cx * f,
                                  fsl_ckout_unmanage_opt const * opt );

/**
   Hard-coded range of values of the vfile.chnged db field.
   These values are part of the fossil schema and must not
   be modified.
*/
enum fsl_vfile_change_e {
  /** File is unchanged. */
  FSL_VFILE_CHANGE_NONE = 0,
  /** File edit. */
  FSL_VFILE_CHANGE_MOD = 1,
  /** File changed due to a merge. */
  FSL_VFILE_CHANGE_MERGE_MOD = 2,
  /** File added by a merge. */
  FSL_VFILE_CHANGE_MERGE_ADD = 3,
  /** File changed due to an integrate merge. */
  FSL_VFILE_CHANGE_INTEGRATE_MOD = 4,
  /** File added by an integrate merge. */
  FSL_VFILE_CHANGE_INTEGRATE_ADD = 5,
  /** File became executable but has unmodified contents. */
  FSL_VFILE_CHANGE_IS_EXEC = 6,
  /** File became a symlink whose target equals its old contents. */
  FSL_VFILE_CHANGE_BECAME_SYMLINK = 7,
  /** File lost executable status but has unmodified contents. */
  FSL_VFILE_CHANGE_NOT_EXEC = 8,
  /** File lost symlink status and has contents equal to its old target. */
  FSL_VFILE_CHANGE_NOT_SYMLINK = 9
};
typedef enum fsl_vfile_change_e fsl_vfile_change_e;

/**
   Change-type flags for use with fsl_ckout_changes_visit() and
   friends.

   TODO: consolidate this with fsl_vfile_change_e insofar as possible.
   There are a few checkout change statuses not reflected in
   fsl_vfile_change_e.
*/
enum fsl_ckout_change_e {
/**
   Sentinel placeholder value.
*/
FSL_CKOUT_CHANGE_NONE = 0,
/**
   Indicates that a file was modified in some unspecified way.
*/
FSL_CKOUT_CHANGE_MOD = FSL_VFILE_CHANGE_MOD,
/**
   Indicates that a file was modified as the result of a merge.
*/
FSL_CKOUT_CHANGE_MERGE_MOD = FSL_VFILE_CHANGE_MERGE_MOD,
/**
   Indicates that a file was added as the result of a merge.
*/
FSL_CKOUT_CHANGE_MERGE_ADD = FSL_VFILE_CHANGE_MERGE_ADD,
/**
   Indicates that a file was modified as the result of an
   integrate-merge.
*/
FSL_CKOUT_CHANGE_INTEGRATE_MOD = FSL_VFILE_CHANGE_INTEGRATE_MOD,
/**
   Indicates that a file was added as the result of an
   integrate-merge.
*/
FSL_CKOUT_CHANGE_INTEGRATE_ADD = FSL_VFILE_CHANGE_INTEGRATE_ADD,
/**
   Indicates that the file gained the is-executable trait
   but is otherwise unmodified.
*/
FSL_CKOUT_CHANGE_IS_EXEC = FSL_VFILE_CHANGE_IS_EXEC,
/**
   Indicates that the file has changed to a symlink.
*/
FSL_CKOUT_CHANGE_BECAME_SYMLINK = FSL_VFILE_CHANGE_BECAME_SYMLINK,
/**
   Indicates that the file lost the is-executable trait
   but is otherwise unmodified.
*/
FSL_CKOUT_CHANGE_NOT_EXEC = FSL_VFILE_CHANGE_NOT_EXEC,
/**
   Indicates that the file was previously a symlink but is
   now a plain file.
*/
FSL_CKOUT_CHANGE_NOT_SYMLINK = FSL_VFILE_CHANGE_NOT_SYMLINK,
/**
   Indicates that a file was added.
*/
FSL_CKOUT_CHANGE_ADDED = FSL_CKOUT_CHANGE_NOT_SYMLINK + 1000,
/**
   Indicates that a file was removed from SCM management.
*/
FSL_CKOUT_CHANGE_REMOVED,
/**
   Indicates that a file is missing from the local checkout.
*/
FSL_CKOUT_CHANGE_MISSING,
/**
   Indicates that a file was renamed.
*/
FSL_CKOUT_CHANGE_RENAMED
};

typedef enum fsl_ckout_change_e fsl_ckout_change_e;

/**
   This is equivalent to calling fsl_vfile_changes_scan() with the
   arguments (f, -1, 0).

   @see fsl_ckout_changes_visit()
   @see fsl_vfile_changes_scan()
*/
FSL_EXPORT int fsl_ckout_changes_scan(fsl_cx * f);

/**
   A typedef for visitors of checkout status information via
   fsl_ckout_changes_visit(). Implementions will receive the
   last argument passed to fsl_ckout_changes_visit() as their
   first argument. The second argument indicates the type of change
   and the third holds the repository-relative name of the file.

   If changes is FSL_CKOUT_CHANGE_RENAMED then origName will hold
   the original name, else it will be NULL.

   Implementations must return 0 on success, non-zero on error. On
   error any looping performed by fsl_ckout_changes_visit() will
   stop and this function's result code will be returned.

   @see fsl_ckout_changes_visit()
*/
typedef int (*fsl_ckout_changes_f)(void * state, fsl_ckout_change_e change,
                                      char const * filename,
                                      char const * origName);

/**
   Compares the changes of f's local checkout against repository
   version vid (checkout version if vid is negative). For each
   change detected it calls visitor(state,...) to report the
   change.  If visitor() returns non-0, that code is returned from
   this function. If doChangeScan is true then
   fsl_ckout_changes_scan() is called by this function before
   iterating, otherwise it is assumed that the caller has called
   that or has otherwise ensured that the checkout db's vfile table
   has been populated.

   If the callback returns FSL_RC_BREAK, this function stops iteration
   and returns 0.

   Returns 0 on success.

   @see fsl_ckout_changes_scan()
*/
FSL_EXPORT int fsl_ckout_changes_visit( fsl_cx * f, fsl_id_t vid,
                                           bool doChangeScan,
                                           fsl_ckout_changes_f visitor,
                                           void * state );
/**
   A bitmask of flags for fsl_vfile_changes_scan().
*/
enum fsl_ckout_sig_e {
/**
   The empty flags set.
*/
FSL_VFILE_CKSIG_NONE = 0,

/**
   Non-file/non-link FS objects trigger an error.
*/
FSL_VFILE_CKSIG_ENOTFILE = 0x001,
/**
   Verify file content using hashing, regardless of whether or not
   file timestamps differ.
*/
FSL_VFILE_CKSIG_HASH = 0x002,
/**
   For unchanged or changed-by-merge files, set the mtime to last
   check-out time, as determined by fsl_mtime_of_manifest_file().
*/
FSL_VFILE_CKSIG_SETMTIME = 0x004,
/**
   Indicates that when populating the vfile table, it should be not be
   cleared of entries for other checkins. Normally we want to clear
   all versions except for the one we're working with, but at least
   a couple of use cases call for having multiple versions in vfile at
   once. Many algorithms generally assume only a single checkin's
   worth of state is in vfile and can get confused if that is not the
   case.
*/
FSL_VFILE_CKSIG_KEEP_OTHERS = 0x008,

/**
   If set and fsl_vfile_changes_scan() is passed a version other than
   the pre-call checkout version, it will, when finished, write the
   given version in the "checkout" setting of the ckout.vvar table,
   effectively switching the checkout to that version. It does not do
   this be default because it is sometimes necessary to have two
   versions in the vfile table at once and the operation doing so
   needs to control which version number is the current checkout.
*/
FSL_VFILE_CKSIG_WRITE_CKOUT_VERSION = 0x010
};

/**
    This function populates (if needed) the vfile table of f's
    checkout db for the given checkin version ID then compares files
    listed in it against files in the checkout directory, updating
    vfile's status for the current checkout version id as its goes. If
    vid is<=0 then the current checkout's RID is used in its place
    (note that 0 is the RID of an initial empty repository!).

    cksigFlags must be 0 or a bitmask of fsl_ckout_sig_e values.

    This is a relatively memory- and filesystem-intensive operation,
    and should not be performed more often than necessary. Many SCM
    algorithms rely on its state being correct, however, so it's
    generally better to err on the side of running it once too often
    rather than once too few times.

    Returns 0 on success, non-0 on error.

    BUG: this does not properly catch one particular corner-case
    change, where a file has been replaced by a same-named non-file
    (symlink or directory).
*/
FSL_EXPORT int fsl_vfile_changes_scan(fsl_cx * f, fsl_id_t vid,
                                      unsigned cksigFlags);

/**
   If f has an opened checkout which has local changes noted in its
   checkout db state (the vfile table), returns true, else returns
   false. Note that this function does not do the filesystem scan to
   check for changes, but checks only the db state. Use
   fsl_vfile_changes_scan() to perform the actual scan (noting that
   library-side APIs which update that state may also record
   individual changes or automatically run a scan).
*/
FSL_EXPORT bool fsl_ckout_has_changes(fsl_cx *f);

/**
   Callback type for use with fsl_checkin_queue_opt for alerting a
   client about exactly which files get enqueued/dequeued via
   fsl_checkin_enqueue() and fsl_checkin_dequeue().

   This function gets passed the checkout-relative name of the file
   being enqueued/dequeued and the client-provided state pointer which
   was passed to the relevant API. It must return 0 on success. If it
   returns non-0, the API on whose behalf this callback is invoked
   will propagate that error code back to the caller.

   The intent of this callback is simply to report changes to the
   client, not to perform validation. Thus such callbacks "really
   should not fail" unless, e.g., they encounter an OOM condition or
   some such. Any validation required by the client should be
   performed before calling fsl_checkin_enqueue()
   resp. fsl_checkin_dequeue().
*/
typedef int (*fsl_checkin_queue_f)(const char * filename, void * state);

/**
   Options object type used by fsl_checkin_enqueue() and
   fsl_checkin_dequeue().
*/
struct fsl_checkin_queue_opt {
  /**
     File or directory name to enqueue/dequeue to/from a pending
     checkin.
  */
  char const * filename;
  /**
     If true, filename (if not absolute) is interpreted as relative to
     the current working directory, else it is assumed to be relative
     to the top of the current checkout directory.
  */
  bool relativeToCwd;

  /**
     If not NULL then this->filename and this->relativeToCwd are
     IGNORED and any to-queue filename(s) is/are added from this
     container. It is an error (FSL_RC_MISUSE) to pass an empty bag.
     (Should that be FSL_RC_RANGE instead?)

     The bag is assumed to contain values from the vfile.id checkout
     db field, refering to one or more files which should be queued
     for the pending checkin. It is okay to pass IDs for unmodified
     files or to queue the same files multiple times. Unmodified files
     may be enqueued but will be ignored by the checkin process if, at
     the time the checkin is processed, they are still unmodified.
     Duplicated entries are simply ignored for the 2nd and subsequent
     inclusion.

     @see fsl_ckout_vfile_ids()
  */
  fsl_id_bag const * vfileIds;

  /**
     If true, fsl_vfile_changes_scan() is called to ensure that the
     filesystem and vfile tables agree. If the client code has called
     that function, or its equivalent, since any changes were made to
     the checkout then this may be set to false to speed up the
     enqueue process. This is only used by fsl_checkin_enqueue(), not
     fsl_checkin_dequeue().
  */
  bool scanForChanges;

  /**
     If true, only flagged-as-modified files will be enqueued by
     fsl_checkin_enqueue(). By and large, this should be set to
     true. Setting this to false is generally only intended/useful for
     testing.
  */
  bool onlyModifiedFiles;

  /**
     It not NULL, is pass passed the checkout-relative filename of
     each enqueued/dequeued file and this->callbackState. See the
     callback type's docs for more details.
  */
  fsl_checkin_queue_f callback;

  /**
     Opaque client-side state for use as the 2nd argument to
     this->callback.
  */
  void * callbackState;
};

/** Convenience typedef. */
typedef struct fsl_checkin_queue_opt fsl_checkin_queue_opt;

/** Initialized-with-defaults fsl_checkin_queue_opt structure, intended for
    const-copy initialization. */
#define fsl_checkin_queue_opt_empty_m { \
  NULL/*filename*/,true/*relativeToCwd*/,    \
  NULL/*vfileIds*/,                                 \
  true/*scanForChanges*/,true/*onlyModifiedFiles*/,   \
  NULL/*callback*/,NULL/*callbackState*/      \
}

/** Initialized-with-defaults fsl_checkin_queue_opt structure, intended for
    non-const copy initialization. */
extern const fsl_checkin_queue_opt fsl_checkin_queue_opt_empty;

/**
   Adds one or more files to f's list of "selected" files - those
   which should be included in the next commit (see
   fsl_checkin_commit()).

   Warning: if this function is not called before
   fsl_checkin_commit(), then fsl_checkin_commit() will select all
   modified, fsl_ckout_manage()'d, fsl_ckout_unmanage()'d, or renamed
   files by default.

   opt->filename must be a non-empty NUL-terminated string. The
   filename is canonicalized via fsl_ckout_filename_check() - see that
   function for the meaning of the opt->relativeToCwd parameter. To
   queue all modified files in a checkout, set opt->filename to ".",
   opt->relativeToCwd to false, and opt->onlyModifiedFiles to true.
   "Modified" includes any which are pending deletion, are
   newly-added, or for which a rename is pending.

   The resolved name must refer to either a single vfile.pathname
   value in the current vfile table or to a checkout-root-relative
   directory. All matching filenames which refer to modified files (as
   recorded in the vfile table) are queued up for the next commit.
   If opt->filename is NULL, empty, or ("." and opt->relativeToCwd is false)
   then all files in the vfile table are checked for changes.

   If opt->scanForChanges is true then fsl_vfile_changes_scan() is
   called before starting to ensure that the vfile entries are up to
   date. If the client app has "recently" run that (or its
   equivalent), that (slow) step can be skipped by setting
   opt->scanForChanges to false before calling this

   Note that after this returns, any given file may still be modified
   by the client before the commit takes place, and the changes on
   disk at the point of the fsl_checkin_commit() are the ones which
   get saved (or not).

   For each resolved entry which actually gets enqueued (i.e. was not
   already enqueued and which is marked as modified), opt->callback
   (if it is not NULL) is passed the checkout-relative file name and
   the opt->callbackState pointer.

   Returns 0 on success, FSL_RC_MISUSE if either pointer is NULL, or
   *zName is NUL. Returns FSL_RC_OOM on allocation error. It is not
   inherently an error for opt->filename to resolve to no queue-able
   entries. A client can check for that case, if needed, by assigning
   opt->callback and incrementing a counter in that callback. If the
   callback is never called, no queue-able entries were found.

   On error f's error state might (depending on the nature of the
   problem) contain more details.

   @see fsl_checkin_is_enqueued()
   @see fsl_checkin_dequeue()
   @see fsl_checkin_discard()
   @see fsl_checkin_commit()
*/
FSL_EXPORT int fsl_checkin_enqueue(fsl_cx * f,
                                   fsl_checkin_queue_opt const * opt);

/**
   The opposite of fsl_checkin_enqueue(), this opt->filename,
   which may resolve to a single name or a directory, from the checkin
   queue. Returns 0 on succes. This function does no validation on
   whether a given file(s) actually exist(s), it simply asks the
   internals to clean up matching strings from the checkout's vfile
   table. Specifically, it does not return an error if this operation
   finds no entries to dequeue.

   If opt->filename is empty or NULL then ALL files are unqueued from
   the pending checkin.

   If opt->relativeToCwd is true (non-0) then opt->filename is
   resolved based on the current directory, otherwise it is resolved
   based on the checkout's root directory.

   If opt->filename is not NULL or empty, this functions runs the
   given path through fsl_ckout_filename_check() and will fail if that
   function fails, propagating any error from that function. Ergo,
   opt->filename must refer to a path within the current checkout.

   @see fsl_checkin_enqueue()
   @see fsl_checkin_is_enqueued()
   @see fsl_checkin_discard()
   @see fsl_checkin_commit()
*/
FSL_EXPORT int fsl_checkin_dequeue(fsl_cx * f,
                                        fsl_checkin_queue_opt const * opt);

/**
   Returns true (non-0) if the file named by zName is in f's current
   file checkin queue.  If NO files are in the current selection
   queue then this routine assumes that ALL files are implicitely
   selected. As long as at least one file is enqueued (via
   fsl_checkin_enqueue()) then this function only returns true
   for files which have been explicitly enqueued.

   If relativeToCwd then zName is resolved based on the current
   directory, otherwise it assumed to be related to the checkout's
   root directory.

   This function returning true does not necessarily indicate that
   the file _will_ be checked in at the next commit. If the file has
   not been modified at commit-time then it will not be part of the
   commit.

   This function honors the fsl_cx_is_case_sensitive() setting
   when comparing names.

   Achtung: this does not resolve directory names like
   fsl_checkin_enqueue() and fsl_checkin_dequeue() do. It
   only works with file names.

   @see fsl_checkin_enqueue()
   @see fsl_checkin_dequeue()
   @see fsl_checkin_discard()
   @see fsl_checkin_commit()
*/
FSL_EXPORT bool fsl_checkin_is_enqueued(fsl_cx * f, char const * zName,
                                        bool relativeToCwd);

/**
   Discards any state accumulated for a pending checking,
   including any files queued via fsl_checkin_enqueue()
   and tags added via fsl_checkin_T_add().

   @see fsl_checkin_enqueue()
   @see fsl_checkin_dequeue()
   @see fsl_checkin_is_enqueued()
   @see fsl_checkin_commit()
   @see fsl_checkin_T_add()
*/
FSL_EXPORT void fsl_checkin_discard(fsl_cx * f);

/**
   Parameters for fsl_checkin_commit().

   Checkins are created in a multi-step process:

   - fsl_checkin_enqueue() queues up a file or directory for
   commit at the next commit.

   - fsl_checkin_dequeue() removes an entry, allowing
   UIs to toggle files in and out of a checkin before
   committing it.

   - fsl_checkin_is_enqueued() can be used to determine whether
   a given name is already enqueued or not.

   - fsl_checkin_T_add() can be used to T-cards (tags) to a
   deck. Branch tags are intended to be applied via the
   fsl_checkin_opt::branch member.

   - fsl_checkin_discard() can be used to cancel any pending file
   enqueuings, effectively cancelling a commit (which can be
   re-started by enqueuing another file).

   - fsl_checkin_commit() creates a checkin for the list of enqueued
   files (defaulting to all modified files in the checkout!). It
   takes an object of this type to specify a variety of parameters
   for the check.

   Note that this API uses the terms "enqueue" and "unqueue" rather
   than "add" and "remove" because those both have very specific
   (and much different) meanings in the overall SCM scheme.
*/
struct fsl_checkin_opt {
  /**
     The commit message. May not be empty - the library
     forbids empty checkin messages.
  */
  char const * message;

  /**
     The optional mime type for the message. Only set
     this if you know what you're doing.
  */
  char const * messageMimeType;

  /**
     The user name for the checkin. If NULL or empty, it defaults to
     fsl_cx_user_get(). If that is NULL, a FSL_RC_RANGE error is
     triggered.
  */
  char const * user;

  /**
     If not NULL, makes the checkin the start of a new branch with
     this name.
  */
  char const * branch;

  /**
     If this->branch is not NULL, this is applied as its "bgcolor"
     propagating property. If this->branch is NULL then this is
     applied as a one-time color tag to the checkin.

     It must be NULL, empty, or in a form usable by HTML/CSS,
     preferably \#RRGGBB form. Length-0 values are ignored (as if
     they were NULL).
  */
  char const * bgColor;

  /**
     If true, the checkin will be marked as private, otherwise it
     will be marked as private or public, depending on whether or
     not it inherits private content.
  */
  bool isPrivate;

  /**
     Whether or not to calculate an R-card. Doing so is very
     expensive (memory and I/O) but it adds another layer of
     consistency checking to manifest files. In practice, the R-card
     is somewhat superfluous and the cost of calculating it has
     proven painful on very large repositories. fossil(1) creates an
     R-card for all checkins but does not require that one be set
     when it reads a manifest.
  */
  bool calcRCard;

  /**
     Tells the checkin to close merged-in branches (merge type of
     0). INTEGRATE merges (type=-4) are always closed by a
     checkin. This does not apply to CHERRYPICK (type=-1) and
     BACKOUT (type=-2) merges.
  */
  bool integrate;

  /**
     If true, allow a file to be checked in if it contains
     fossil-style merge conflict markers, else fail if an attempt is
     made to commit any files with such markers.
  */
  bool allowMergeConflict;

  /**
     A hint to fsl_checkin_commit() about whether it needs to scan the
     checkout for changes. Set this to false ONLY if the calling code
     calls fsl_ckout_changes_scan() (or equivalent,
     e.g. fsl_vfile_changes_scan()) immediately before calling
     fsl_checkin_commit(). fsl_checkin_commit() requires a non-stale
     changes scan in order to function properly, but it's a
     computationally slow operation so the checkin process does not
     want to duplicate it if the application has recently done so.
  */
  bool scanForChanges;

  /**
     NOT YET IMPLEMENTED! TODO!

     If true, files which are removed from the SCM by this checkin
     should be removed from the filesystem.

     Reminder to self: when we do this, incorporate
     fsl_rm_empty_dirs().
  */
  bool rmRemovedFiles;

  /**
     Whether to allow (or try to force) a delta manifest or not. 0
     means no deltas allowed - it will generate a baseline
     manifest. Greater than 0 forces generation of a delta if
     possible (if one can be readily found) even if doing so would not
     save a notable amount of space. Less than 0 means to
     decide via some heuristics.

     A "readily available" baseline means either the current checkout
     is a baseline or has a baseline. In either case, we can use that
     as a baseline for a delta. i.e. a baseline "should" generally be
     available except on the initial checkin, which has neither a
     parent checkin nor a baseline.

     The current behaviour for "auto-detect" mode is: it will generate
     a delta if a baseline is "readily available" _and_ the repository
     has at least one delta already. Once it calculates a delta form,
     it calculates whether that form saves any appreciable
     space/overhead compared to whether a baseline manifest was
     generated. If so, it discards the delta and re-generates the
     manifest as a baseline. The "force" behaviour (deltaPolicy>0)
     bypasses the "is it too big?" test, and is only intended for
     testing, not real-life use.

     Caveat: if the repository has the "forbid-delta-manifests" set to
     a true value, this option is ignored: that setting takes
     priority. Similarly, it will not create a delta in a repository
     unless a delta has been "seen" in that repository before or this
     policy is set to >0. When a checkin is created with a delta
     manifest, that fact gets recorded in the repository's config
     table.

     Note that delta manifests have some advantages and may not
     actually save much (if any) repository space because the
     lower-level delta framework already compresses parent versions of
     artifacts tightly. For more information see:

     https://fossil-scm.org/home/doc/tip/www/delta-manifests.md
  */
  int deltaPolicy;

  /**
     Time of the checkin. If 0 or less, the time of the
     fsl_checkin_commit() call is used.
  */
  double julianTime;

  /**
     If this is not NULL then the committed manifest will include a
     tag which closes the branch. The value of this string will be
     the value of the "closed" tag, and the value may be an empty
     string. The intention is that this gets set to a comment about
     why the branch is closed, but it is in no way mandatory.
  */
  char const * closeBranch;

  /**
     Tells fsl_checkin_commit() to dump the generated manifest to
     this file. Intended only for debugging and testing. Checking in
     will fail if this file cannot be opened for writing.
  */
  char const * dumpManifestFile;
  /*
    fossil(1) has many more options. We might want to wrap some of
    it up in the "incremental" state (f->ckin.mf).

    TODOs:

    A callback mechanism which supports the user cancelling
    the checkin. It is (potentially) needed for ops like
    confirming the commit of CRNL-only changes.

    2021-03-09: we now have fsl_confirmer for this but currently no
    part of the checkin code needs a prompt.
  */
};

/**
   Empty-initialized fsl_checkin_opt instance, intended for use in
   const-copy constructing.
*/
#define fsl_checkin_opt_empty_m {               \
  NULL/*message*/,                            \
  NULL/*messageMimeType*/,                  \
  NULL/*user*/,                             \
  NULL/*branch*/,                           \
  NULL/*bgColor*/,                          \
  false/*isPrivate*/,                           \
  true/*calcRCard*/,                           \
  false/*integrate*/,                           \
  false/*allowMergeConflict*/,\
  true/*scanForChanges*/,\
  false/*rmRemovedFiles*/,\
  0/*deltaPolicy*/,                        \
  0.0/*julianTime*/,                        \
  NULL/*closeBranch*/,                      \
  NULL/*dumpManifestFile*/               \
}

/**
   Empty-initialized fsl_checkin_opt instance, intended for use in
   copy-constructing. It is important that clients copy this value
   (or fsl_checkin_opt_empty_m) to cleanly initialize their
   fsl_checkin_opt instances, as this may set default values which
   (e.g.) a memset() would not.
*/
FSL_EXPORT const fsl_checkin_opt fsl_checkin_opt_empty;

/**
   This creates and saves a "checkin manifest" for the current
   checkout.

   Its primary inputs is a list of files to commit. This list is
   provided by the client by calling fsl_checkin_enqueue() one or
   more times.  If no files are explicitely selected (enqueued) then
   it calculates which local files have changed vs the current
   checkout and selects all of those.

   Non-file inputs are provided via the opt parameter.

   On success, it returns 0 and...

   - If newRid is not NULL, it is assigned the new checkin's RID
   value.

   - If newUuid is not NULL, it is assigned the new checkin's UUID
   value. Ownership of the bytes is passed to the caller, who must
   eventually pass them to fsl_free() to free them.

   Note that the new RID and UUID can also be fetched afterwards by
   calling fsl_ckout_version_info().

   On error non-0 is returned and f's error state may (depending on
   the nature of the problem) contain details about the problem.
   Note, however, that any error codes returned here may have arrived
   from several layers down in the internals, and may not have a
   single specific interpretation here. When possible/practical, f's
   error state gets updated with a human-readable description of the
   problem.

   ACHTUNG: all pending checking state is cleaned if this function
   fails for any reason other than basic argument validation. This
   means any queued files or tags need to be re-applied if the client
   wants to try again. That is somewhat of a bummer, but this
   behaviour is the only way we can ensure that then the pending
   checkin state does not get garbled on a second use. When in doubt
   about the state, the client should call fsl_checkin_discard() to
   clear it before try to re-commit. (Potential TODO: add a
   success/fail state flag to the checkin state and only clean up on
   success? OTOH, since something in the state likely caused the
   problem, we might not want to do that.)

   This operation does all of its db-related work in a transaction, so
   it rolls back any db changes if it fails. To implement a "dry-run"
   mode, simply wrap this call in a transaction started on the
   fsl_cx_db_ckout() db handle (passing it to
   fsl_db_transaction_begin()), then, after this call, either cal;
   fsl_db_transaction_rollback() (to implement dry-run mode) or
   fsl_db_transaction_commit() (for "wet-run" mode). If this function
   returns non-0 due to anything more serious than basic argument
   validation, such a transaction will be in a roll-back state.

   Some of the more notable, potentially not obvious, error
   conditions:

   - Trying to commit against a closed leaf: FSL_RC_ACCESS. Doing so
   is not permitted by fossil(1), so we disallow it here.

   - An empty/NULL user name or commit message, or no files were
   selected which actually changed: FSL_RC_MISSING_INFO. In these
   cases f's error state describes the problem.

   - Some resource is not found (e.g. an expected RID/UUID could not
   be resolved): FSL_RC_NOT_FOUND. This would generally indicate
   some sort of data consistency problem. i.e. it's quite possibly
   very bad if this is returned.

   - If the checkin would result in no file-level changes vis-a-vis
   the current checkout, FSL_RC_NOOP is returned.

   BUGS:

   - It cannot currently properly distinguish a "no-op" commit, one in
   which no files were modified or only their permissions were
   modifed.

   @see fsl_checkin_enqueue()
   @see fsl_checkin_dequeue()
   @see fsl_checkin_discard()
   @see fsl_checkin_T_add()
*/
FSL_EXPORT int fsl_checkin_commit(fsl_cx * f, fsl_checkin_opt const * opt,
                                  fsl_id_t * newRid, fsl_uuid_str * newUuid);

/**
   Works like fsl_deck_T_add(), adding the given tag information to
   the pending checkin state. Returns 0 on success, non-0 on error. A
   checkin may, in principal, have any number of tags, and this may be
   called any number of times to add new tags to the pending
   commit. This list of tags gets cleared by a successful
   fsl_checkin_commit() or by fsl_checkin_discard(). Decks require
   that each tag be distinct from each other (none may compare
   equivalent), but that check is delayed until the deck is output
   into its final artifact form.

   @see fsl_checkin_enqueue()
   @see fsl_checkin_dequeue()
   @see fsl_checkin_commit()
   @see fsl_checkin_discard()
   @see fsl_checkin_T_add2()
*/
FSL_EXPORT int fsl_checkin_T_add( fsl_cx * f, fsl_tagtype_e tagType,
                                   fsl_uuid_cstr uuid, char const * name,
                                   char const * value);

/**
   Works identically to fsl_checkin_T_add() except that it takes its
   argument in the form of a T-card object.

   On success ownership of t is passed to mf. On error (see
   fsl_deck_T_add()) ownership is not modified.

   Results are undefined if either argument is NULL or improperly
   initialized.
*/
FSL_EXPORT int fsl_checkin_T_add2( fsl_cx * f, fsl_card_T * t );

/**
   Clears all contents from f's checkout database, including the vfile
   table, vmerge table, and some of the vvar table. The tables are
   left intact. Returns 0 on success, non-0 if f has no checkout or for
   a database error.
 */
FSL_EXPORT int fsl_ckout_clear_db(fsl_cx *f);

/**
   Returns the base name of the current platform's checkout database
   file. That is "_FOSSIL_" on Windows and ".fslckout" everywhere
   else. The returned bytes are static.

   TODO: an API which takes a dir name and looks for either name
*/
FSL_EXPORT char const *fsl_preferred_ckout_db_name();  

/**
   File-overwrite policy values for use with fsl_ckup_opt and friends.
*/
enum fsl_file_overwrite_policy_e {
/** Indicates that an error should be triggered if a file would be
    overwritten. */
FSL_OVERWRITE_ERROR = 0,
/**
   Indicates that files should always be overwritten by 
*/
FSL_OVERWRITE_ALWAYS,
/**
   Indicates that files should never be overwritten, and silently
   skipped over. This is almost never what one wants to do.
*/
FSL_OVERWRITE_NEVER
};
typedef enum fsl_file_overwrite_policy_e fsl_file_overwrite_policy_e;

/**
   State values for use with fsl_ckup_state::fileRmInfo.
*/
enum fsl_ckup_rm_state_e {
/**
   Indicates that the file was not removed in a given checkout.
   Guaranteed to have the value 0 so that it is treated as boolean
   false. No other entries in this enum have well-defined values.
*/
FSL_CKUP_RM_NOT = 0,
/**
   Indicates that a file was removed from a checkout but kept
   in the filesystem because it was locally modified.
*/
FSL_CKUP_RM_KEPT,
/**
   Indicates that a file was removed from a checkout and the
   filesystem, with the caveat that failed attempts to remove from the
   filesystem are ignored for Reasons but will be reported as if the
   unlink worked.
*/
FSL_CKUP_RM
};
typedef enum fsl_ckup_rm_state_e fsl_ckup_rm_state_e;

/**
  Under construction. Work in progress...

  Options for "opening" a fossil repository database. That is,
  creating a new fossil checkout database and populating its schema,
  _without_ checking out any files. (That latter part is up for
  reconsideration and this API might change in the future to check
  out files after creating/opening the db.)
*/
struct fsl_repo_open_ckout_opt {
  /**
     Name of the target directory, which must already exist. May be
     relative, e.g. ".". The repo-open operation will chdir to this
     directory for the duration of the operation. May be NULL, in
     which case the current directory is assumed and no chdir is
     performed.
  */
  char const * targetDir;

  /**
     The filename, with no directory components, of the desired
     checkout db name. For the time being, always leave this NULL and
     let the library decide. It "might" (but probably won't) be
     interesting at some point to allow the client to specify a
     different name (noting that that would be directly incompatible
     with fossil(1)).
  */
  char const * ckoutDbFile;

  /**
     Policy for how to handle overwrites of files extracted from a
     newly-opened checkout.

     Potential TODO: replace this with a fsl_confirmer, though that
     currently seems like overkill for this particular case.
  */
  fsl_file_overwrite_policy_e fileOverwritePolicy;

  /**
     fsl_repo_open_ckout() installs the fossil checkout schema. If
     this is true it will forcibly replace any existing relevant
     schema components in the checkout db, otherwise it will fail when
     it tries to overwrite an existing schema and cannot.
  */
  bool dbOverwritePolicy;

  /**
     Of true, the checkout-open process will look for an opened
     checkout in the target directory and its parents (recursively)
     and fail with FSL_RC_ALREADY_EXISTS if one is found.
  */
  bool checkForOpenedCkout;
};
typedef struct fsl_repo_open_ckout_opt fsl_repo_open_ckout_opt;

/**
  Empty-initialized fsl_repo_open_ckout_opt const-copy constructer.
*/
#define fsl_repo_open_ckout_opt_m { \
  NULL/*targetDir*/, NULL/*ckoutDbFile*/, \
  FSL_OVERWRITE_ERROR/*fileOverwritePolicy*/, \
  false/*dbOverwritePolicy*/,               \
  -1/*checkForOpenedCkout*/              \
}

/**
  Empty-initialised fsl_repo_open_ckout_opt instance. Clients should copy
  this value (or fsl_repo_open_ckout_opt_empty_m) to initialise
  fsl_repo_open_ckout_opt instances for sane default values.
*/
FSL_EXPORT const fsl_repo_open_ckout_opt fsl_repo_open_ckout_opt_empty;

/**
   Work in progress...

   Opens a checkout db for use with the currently-connected repository
   or creates a new one. If opening an existing one, it gets "stolen"
   from any repository it might have been previously mapped to.

   - Requires that f have an opened repository db and no opened
     checkout. Returns FSL_RC_NOT_A_REPO if no repo is opened and
     FSL_RC_MISUSE if a checkout *is* opened.

   - Creates/re-uses a .fslckout DB in the dir opt->targetDir. The
     directory must be NULL or already exist, else FSL_RC_NOT_FOUND is
     returned. If opt->dbOverwritePolicy is false then it fails with
     FSL_RC_ALREADY_EXISTS if that directory already contains a
     checkout db.

   Note that this does not extract any SCM'd files from the
   repository, it only opens (and possibly creates) the checkout
   database.

   Pending:

   - If opening an existing checkout db for a different repo then
   delete the STASH and UNDO entries, as they're not valid for a
   different repo.
*/
FSL_EXPORT int fsl_repo_open_ckout( fsl_cx * f, fsl_repo_open_ckout_opt const * opt );

typedef struct fsl_ckup_state fsl_ckup_state;
/**
   A callback type for use with fsl_ckup_state.  It gets called via
   fsl_repo_ckout() and fsl_ckout_update() to report progress of the
   extraction process. It gets called after one of those functions has
   successfully extracted a file or skipped over it because the file
   existed and the checkout options specified to leave existing files
   in place. It must return 0 on success, and non-0 will end the
   extraction process, propagating that result code back to the
   caller. If this callback fails, the checkout's contents may be left
   in an undefined state, with some files updated and others not.  All
   database-side data will be consistent (the transaction is rolled
   back) but filesystem-side changes may not be.
*/
typedef int (*fsl_ckup_f)( fsl_ckup_state const * cState );

/**
   This enum lists the various types of individual file change states
   which can happen during a checkout or update.
*/
enum fsl_ckup_fchange_e {
/** Sentinel value. */
FSL_CKUP_FCHANGE_INVALID = -1,
/** Was unchanged between the previous and updated-to version,
    so no change was made to the on-disk file. This is the
    only entry in the enum which is guaranteed to have a specific
    value: 0, so that it can be used as a boolean false. */
FSL_CKUP_FCHANGE_NONE = 0,
/**
   Added to SCM in the updated-to version.
*/
FSL_CKUP_FCHANGE_ADDED,
/**
   Added to SCM in the current checkout version and carried over into
   the updated-to version.
*/
FSL_CKUP_FCHANGE_ADD_PROPAGATED,
/** Removed from SCM in the updated-to to version
    OR in the checked-out version but not yet commited.
    a.k.a. it became "unmanaged."


   Do we need to differentiate between those cases?
*/
FSL_CKUP_FCHANGE_RM,
/**
   Removed from the checked-out version but not yet commited,
   so was carried over to the updated-to version.
*/
FSL_CKUP_FCHANGE_RM_PROPAGATED,
/** Updated or replaced without a merge by the checkout/update
    process. */
FSL_CKUP_FCHANGE_UPDATED,
/** Merge was not performed because at least one of the inputs appears
    to be binary. The updated-to version overwrites the previous
    version in this case.
*/
FSL_CKUP_FCHANGE_UPDATED_BINARY,
/** Updated with a merge by the update process. */
FSL_CKUP_FCHANGE_MERGED,
/** Special case of FSL_CKUP_FCHANGE_UPDATED. Merge was performed
    and conflicts were detected. The newly-updated file will contain
    conflict markers.

    @see fsl_buffer_contains_merge_marker()
*/
FSL_CKUP_FCHANGE_CONFLICT_MERGED,
/** Added in the current checkout but also contained in the
    updated-to version. The local copy takes precedence.
*/
FSL_CKUP_FCHANGE_CONFLICT_ADDED,
/**
   Added by the updated-to version but a local unmanaged copy exists.
   The local copy is overwritten, per historical fossil(1) convention
   (noting that fossil has undo support to allow one to avoid loss of
   such a file's contents).

   TODO: use confirmer here to ask user whether to overwrite.
*/
FSL_CKUP_FCHANGE_CONFLICT_ADDED_UNMANAGED,
/** Edited locally but removed from updated-to version. Local
    edits will be left in the checkout tree. */
FSL_CKUP_FCHANGE_CONFLICT_RM,
/** Cannot merge if one or both of the update/updating verions of a
    file is a symlink The updated-to version overwrites the previous
    version in this case.

    We probably need a better name for this.
*/
FSL_CKUP_FCHANGE_CONFLICT_SYMLINK,
/** File was renamed in the updated-to version. If a file is both
    modified and renamed, it is flagged as renamed instead
    of modified. */
FSL_CKUP_FCHANGE_RENAMED,
/** Locally modified. This state appears only when
    "updating" a checkout to the same version. */
FSL_CKUP_FCHANGE_EDITED
};
typedef enum fsl_ckup_fchange_e fsl_ckup_fchange_e;

/**
   State to be passed to fsl_ckup_f() implementations via
   calls to fsl_repo_ckout() and fsl_ckout_update().
*/
struct fsl_ckup_state {
  /**
     The core SCM state for the just-extracted file. Note that its
     content member will be NULL: the content is not passed on via
     this interface because it is only loaded for files which require
     overwriting.

     An update process may synthesize content for extractState->fCard
     which do not 100% reflect the file on disk. Of primary note here:

     1) fCard->uuid will refer to the hash of the updated-to
     version, as opposed to the hash of the on-disk file (which may
     differ due to having local edits merged in). 

     2) For the update process, fCard->priorName will be NULL unless
     the file was renamed between the original and updated-to
     versions, in which case priorName will refer to the original
     version's name.
  */
  fsl_repo_extract_state const * extractState;
  /**
     Optional client-dependent state for use in the
     fsl_ckup_f() callback.
  */
  void * callbackState;

  /**
     Vaguely describes the type of change the current call into
     the fsl_ckup_f() represents. The full range of values is
     not valid for all operations. Specifically:

     Checkout only uses:

     FSL_CKUP_FCHANGE_NONE
     FSL_CKUP_FCHANGE_UPDATED
     FSL_CKUP_FCHANGE_RM

     For update operatios all (or most) values are potentially
     possible.

     If this has a value of FSL_CKUP_FCHANGE_RM,
     this->fileRmInfo will provide a bit more detail.
  */
  fsl_ckup_fchange_e fileChangeType;

  /**
     Indicates whether the file was removed by the process:

     - FSL_CKUP_RM_NOT = Was not removed.

     - FSL_CKUP_RM_KEPT = Was removed from the checked-out version but
     left in the filesystem because the confirmer said to.

     - FSL_CKUP_RM = Was removed from the checkout and the filesystem.

     When this->dryRun is true, this specifies whether the file would
     have been removed.
  */
  fsl_ckup_rm_state_e fileRmInfo;
  
  /**
     If fsl_repo_ckout()'s or fsl_ckout_update()'s options specified
     that the mtime should be set on each updated file, this holds
     that time. If the file existed and was not overwritten, it is set
     to that file's time. Else it is set to the current time (which
     may differ by a small fraction of a second from the file-write
     time because we avoid stat()'ing it again after writing). If
     this->fileRmInfo indicates that a file was removed, this might
     (depending on availability of the file in the filesystem at the
     time) be set to 0.

     When running in dry-run mode, this value may be 0, as we may not
     have a file in place which we can stat() to get it, nor a db
     entry from which to fetch it.
  */
  fsl_time_t mtime;

  /**
     The size of the extracted file, in bytes. If the file was removed
     from the filesystem (or removal was at least attempted) then this
     is set to -1.
  */
  fsl_int_t size;

  /**
     True if the current checkout/update is running in dry-run mode,
     else false. Dry-run mode has 
  */
  bool dryRun;
};

/**
   UNDER CONSTRUCTION.

   Options for use with fsl_repo_ckout() and fsl_ckout_update(). i.e.
   for checkout and update operations.
*/
struct fsl_ckup_opt {
  /**
     The version of the repostitory to check out or update. This must
     be the blob.rid of a checkin artifact.
  */
  fsl_id_t checkinRid;

  /**
     Gets called once per checked-out or updated file, passed a
     fsl_ckup_state instance with information about the
     checked-out file and related metadata. May be NULL.
  */
  fsl_ckup_f callback;

  /**
     State to be passed to this->callback via the
     fsl_ckup_state::callbackState member.
   */
  void * callbackState;

  /**
     An optional "confirmer" for answering questions about file
     overwrites and deletions posed by the checkout process.
     By default this confirmer of the associated fsl_cx instance
     is used.

     Caveats:

     - This is not currently used by the update process, only
     checkout.

     - If this->setMTime is true, the mtime is NOT set for any files
     which already exist and are skipped due to the confirmer saying
     to leave them in place.

     - Similarly, if the confirmer says to never overwrite files,
     permissions on existing files are not modified. fsl_repo_ckout()
     does not (re)write unmodified files, and thus may leave such
     files with different permissions. That's on the to-fix list.
  */
  fsl_confirmer confirmer;
  
  /**
     If true, the checkout/update processes will calculate the
     (synthetic) mtime of each extracted file and set its mtime. This
     is a relatively expensive operation which calculates the
     "effective mtime" of each file by calculating it: Fossil does not
     record file timestamps, instead treating files as if they had the
     timestamp of the most recent checkin in which they were added or
     modified.

     It's generally a good idea to let the update process stamp the
     _current_ time on modified files, in order to avoid any hiccups
     with build processes which rely on accurate times
     (e.g. Makefiles). When doing a clean checkout, it's often
     interesting to see the "original" times, though.
  */
  bool setMtime;

  /**
     A hint to fsl_repo_ckout() and fsl_ckout_update() about whether
     it needs to scan the checkout for changes. Set this to false ONLY
     if the calling code calls fsl_ckout_changes_scan() (or
     equivalent, e.g. fsl_vfile_changes_scan()) immediately before
     calling fsl_repo_ckout() or fsl_ckout_update(), as both require a
     non-stale changes scan in order to function properly.
  */
  bool scanForChanges;

  /**
     If true, the extraction process will "go through the motions" but
     will not write any files to disk. It will perform I/O such as
     stat()'ing to see, e.g., if it would have needed to overwrite a
     file.
  */
  bool dryRun;
};
typedef struct fsl_ckup_opt fsl_ckup_opt;

/**
  Empty-initialized fsl_ckup_opt const-copy constructor.
*/
#define fsl_ckup_opt_m {\
  -1/*checkinRid*/, NULL/*callback*/, NULL/*callbackState*/,  \
  fsl_confirmer_empty_m/*confirmer*/,\
  false/*setMtime*/, true/*scanForChanges*/,false/*dryRun*/ \
}

/**
  Empty-initialised fsl_ckup_opt instance. Clients should copy
  this value (or fsl_ckup_opt_empty_m) to initialise
  fsl_ckup_opt instances for sane default values.
*/
FSL_EXPORT const fsl_ckup_opt fsl_ckup_opt_empty;

/**
   A fsl_repo_extract() proxy which extracts the contents of the
   repository version specified by opt->checkinRid to the root
   directory of f's currently-opened checkout. i.e. it performs a
   "checkout" operation.

   For each extracted entry, cOpt->callback (if not NULL) will be
   passed a (fsl_ckup_state const*) which contains a pointer
   to the fsl_repo_extract_state and some additional metadata
   regarding the extraction. The value of cOpt->callbackState will be
   set as the callbackState member of that fsl_ckup_state
   struct, so that the client has a way of passing around app-specific
   state to that callback.

   After successful completion, the process will report (see below)
   any files which were part of the previous checkout version but are
   not part of the current version, optionally removing them from the
   filesystem (depending on the value of opt->rmMissingPolicy). It
   will IGNORE ANY DELETION FAILURE of files it attempts to
   delete. The reason it does not fail on removal error is because
   doing so would require rolling back the transaction, effectively
   undoing the checkout, but it cannot roll back any prior deletions
   which succeeded. Similarly, after all file removal is complete, it
   attempts to remove any now-empty directories left over by that
   process, also silently ignoring any errors. If the cOpt->dryRun
   option is specified, it will "go through the motions" of removing
   files but will not actually attempt filesystem removal. For
   purposes of the callback, however, it will report deletions as
   having happened (but will also set the dryRun flag on the object
   passed to the callback).

   After unpacking the SCM-side files, it may write out one or more
   manifest files, as described for fsl_ckout_manifest_write(), if the
   'manifest' config setting says to do so.

   As part of the file-removal process, AFTER all "existing" files are
   processed, it calls cOpt->callback() (if not NULL) for each removed
   file, noting the following peculiarities in the
   fsl_ckup_state object which is passed to it for those
   calls:

   - It is called after the processing of "existing" files. Thus the
   file names passed during this step may appear "out of order" with
   regards to the others (which are guaranteed to be passed in lexical
   order, though whether it is case-sensitive or not depends on the
   repository's case-sensitivity setting).

   - fileRmInfo will indicate that the file was removed from the
   checkout, and whether it was actually removed or retained in the
   filesystem. This will indicate filesystem-level removal even when
   in dry-run mode, though in that case no filesystem-level removal is
   actually attempted.

   - extractState->fileRid will refer to the file's blob's RID for the
   previous checkout version.

   - extractState->content will be NULL.

   - extractState->callbackState will be NULL. 

   - extractState->fCard will refer to the pre-removal state of the
   file. i.e. the state as it was in the checkout prior to this
   function being called.

   Returns 0 on success. Returns FSL_RC_NOT_A_REPO if f has no opened
   repo, FSL_RC_NOT_A_CKOUT if no checkout is opened. If
   cOpt->callback is not NULL and returns a non-0 result code,
   extraction ends and that result is returned. If it returns non-0 at
   any point after basic argument validation, it rolls back all
   changes or sets the current transaction stack into a rollback
   state.

   @see fsl_repo_ckout_open()
*/
FSL_EXPORT int fsl_repo_ckout(fsl_cx * f, fsl_ckup_opt const * cOpt);


/**
   UNDER CONSTRUCTION.

   Performs an "update" operation on f's currenly-opened
   checkout. Performing an update is similar to performing a checkout,
   the primary difference being that an update will merge local file
   modifications into any newly-updated files, whereas a checkout will
   overwrite them.

   TODO?: fossil(1)'s update permits a list of files, in which case it
   behaves differently: it updates the given files to the version
   requested but leaves the checkout at its current version. To be
   able to implement that we either need clients to call this in a
   loop, changing opt->filename on each call (like how we do
   fsl_ckout_manage()) or we need a way for them to pass on the list
   of files/dir in the opt object.

   @see fsl_repo_ckout().
*/
FSL_EXPORT int fsl_ckout_update(fsl_cx * f, fsl_ckup_opt const *opt);


/**
   Tries to calculate a version to update the current checkout version
   to, preferring the tip of the current checkout's branch.

   On success, 0 is returned and *outRid is set to the calculated RID,
   which may be 0, indicating that no errors were encountered but no
   version could be calculated.

   On error, non-0 is returned, outRid is not modified, and f's error
   state is updated.

   Returns FSL_RC_NOT_A_CKOUT if f has no checkout opened and
   FSL_RC_NOT_A_REPO if no repo is opened.

   If it calculates that there are multiple viable descendants it
   returns FSL_RC_AMBIGUOUS and f's error state will contain a list of
   the UUIDs (or UUID prefixes) of those descendants.

   Sidebar: to get the absolute latest version, irrespective of the
   branch, use fsl_sym_to_rid() to resolve the symbolic name "tip".
*/
FSL_EXPORT int fsl_ckout_calc_update_version(fsl_cx * f, fsl_id_t * outRid);

/**
   Bitmask used by fsl_ckout_manifest_setting() and
   fsl_ckout_manifest_write().
*/
enum fsl_cx_manifest_mask_e {
FSL_MANIFEST_MAIN = 0x001,
FSL_MANIFEST_UUID = 0x010,
FSL_MANIFEST_TAGS = 0x100
};
typedef enum fsl_cx_manifest_mask_e fsl_cx_manifest_mask_e;

/**
   Returns a bitmask representing which manifest files, if any, will
   be written when opening or updating a checkout directory, as
   specified by the repository's 'manifest' configuration setting, and
   sets *m to a bitmask indicating which of those are enabled. It
   first checks for a versioned setting then, if no versioned setting
   is found, a repository-level setting.

   A truthy setting value (1, "on", "true") means to write the
   manifest and manifest.uuid files. A string with any of the letters
   'r', 'u', or 't' means to write the [r]aw, [u]uid, and/or [t]ags
   file(s), respectively.

   If the manifest setting is falsy or not set, *m is set to 0, else
   *m is set to a bitmask representing which file(s) are considered to
   be auto-generated for this repository:

   - FSL_MANIFEST_MAIN = manifest
   - FSL_MANIFEST_UUID = manifest.uuid
   - FSL_MANIFEST_TAGS = manifest.tags

   Any db-related or allocation errors while trying to fetch the
   setting are silently ignored.

   For performance's sake, since this is potentially called often from
   fsl_reserved_fn_check(), this setting is currently cached by this
   routine (in the fsl_cx object), but that ignores the fact that the
   manifest setting can be modified at any time, either in a versioned
   setting file or the repository db, and may be modified from outside
   the library. There's a tiny back-door for working around that: if m
   is NULL, the cache will be flushed and no other work will be
   performed. Thus the following approach can be used to force a fresh
   check for that setting:

   @code
   fsl_ckout_manifest_setting(f, NULL); // clears caches, does nothing else
   fsl_ckout_manifest_setting(f, &myInt); // loads/caches the setting
   @endcode
*/
FSL_EXPORT void fsl_ckout_manifest_setting(fsl_cx *f, int *m);

/**
   Might write out the files manifest, manifest.uuid, and/or
   manifest.tags for the current checkout to the the checkout's root
   directory. The 2nd-4th arguments are interpreted as follows:

   0: Do not write that file.

   >0: Always write that file.

   <0: Use the value of the "manifest" config setting (see
   fsl_ckout_manifest_setting()) to determine whether or not to write
   that file.

   As each file is written, its mtime is set to that of the checkout
   version. (Forewarning: that behaviour may change if it proves to be
   problematic vis a vis build processes.)

   Returns 0 on success, non-0 on error:

   - FSL_RC_NOT_A_CKOUT if no checkout is opened.

   - FSL_RC_RANGE if the current checkout RID is 0 (indicating a fresh,
   empty repository).

   - Various potential DB/IO-related error codes.

   If the final argument is not NULL then it will be updated to
   contain a bitmask representing which files, if any, were written:
   see fsl_ckout_manifest_setting() for the values. It is updated
   regardless of success or failure and will indicate which file(s)
   was/were written before the error was triggered.

   Each file implied by the various manifest settings which is NOT
   written by this routine and is also not part of the current
   checkout (i.e. not listed in the vfile table) will be removed from
   disk, but a failure while attempting to do so will be silently
   ignored.
*/
FSL_EXPORT int fsl_ckout_manifest_write(fsl_cx *f,
                                        int manifest,
                                        int manifestUuid,
                                        int manifestTags,
                                        int *wroteWhat );

/**
   Returns true if f has an opened checkout and the given absolute
   path is rooted in that checkout, else false. As a special case, it
   returns false if the path _is_ the checkout root unless zAbsPath
   has a trailing slash. (The checkout root is always stored with a
   trailing slash because that simplifies its internal usage.)

   Note that this is strictly a string comparison, not a
   filesystem-level operation.
*/
FSL_EXPORT bool fsl_is_rooted_in_ckout(fsl_cx *f, char const *zAbsPath);

/**
   Works like fsl_is_rooted_in_ckout() except that it returns 0 on
   success, and on error updates f with a description of the problem
   and returns non-0: FSL_RC_RANGE or (if updating the error state
   fails) FSL_RC_OOM.
 */
FSL_EXPORT int fsl_is_rooted_in_ckout2(fsl_cx *f, char const *zAbsPath);

/**
   Change-type values for use with fsl_ckout_revert_f() callbacks.
*/
enum fsl_ckout_revert_e {
/** Sentinel value. */
FSL_REVERT_NONE = 0,
/**
   File was previously queued for addition but unqueued
   by the revert process.
*/
FSL_REVERT_UNMANAGE,
/**
   File was previously queued for removal but unqueued by the revert
   process. If the file's contents or permissions were also reverted
   then the file is reported as FSL_REVERT_PERMISSIONS or
   FSL_REVERT_CONTENTS instead.
*/
FSL_REVERT_REMOVE,
/**
   File was previously scheduled to be renamed, but the rename was
   reverted. The name reported to the callback is the original one.
   If a file was both modified and renamed, it will be flagged as
   renamed instead of modified, for consistency with the usage of
   fsl_ckup_fchange_e's FSL_CKUP_FCHANGE_RENAMED.

   FIXME: this does not mean that the file on disk was actually
   renamed (if needed). That is TODO, pending addition of code to
   perform renames.
*/
FSL_REVERT_RENAME,
/** File's permissions (only) were reverted. */
FSL_REVERT_PERMISSIONS,
/**
   File's contents reverted. This value trumps any others in this
   enum. Thus if a file's permissions and contents were reverted,
   or it was un-renamed and its contents reverted, it will be
   reported using this enum entry.
*/
FSL_REVERT_CONTENTS
};
typedef enum fsl_ckout_revert_e fsl_ckout_revert_e;
/**
   Callback type for use with fsl_ckout_revert(). For each reverted
   file it gets passed the checkout-relative filename, type of change,
   and the callback state pointer which was passed to
   fsl_ckout_revert(). If it returns non-0, the revert process will
   end in an error and that code will be propagated back to the
   caller.  In such cases, any files reverted up until that point will
   still be reverted on disk but the reversion in the database will be
   rolled back. A change scan (e.g. fsl_ckout_changes_scan()) will
   restore balance to that equation, but these callbacks should only
   return non-0 in for catastrophic failure.
*/
typedef int (*fsl_ckout_revert_f)( char const *zFilename,
                                   fsl_ckout_revert_e changeType,
                                   void * callbackState );

/**
   Options for passing to fsl_ckout_revert().
*/
struct fsl_ckout_revert_opt {
  /**
     File or directory name to revert. See also this->vfileIds.
  */
  char const * filename;
  /**
     An alternative to assigning this->filename is to point
     this->vfileIds to a bag of vfile.id values. If this member is not
     NULL, fsl_ckout_revert() will ignore this->filename.

     @see fsl_filename_to_vfile_ids()
  */
  fsl_id_bag const * vfileIds;
  /**
     Interpret filename as relative to cwd if true, else relative to
     the current checkout root. This is ignored when this->vfileIds is
     not NULL.
  */
  bool relativeToCwd;
  /**
     If true, fsl_vfile_changes_scan() is called to ensure that
     the filesystem and vfile tables agree. If the client code has
     called that function, or its equivalent, since any changes were
     made to the checkout then this may be set to false to speed up
     the revert process.
  */
  bool scanForChanges;
  /**
     Optional callback to notify the client of what gets reverted.
  */
  fsl_ckout_revert_f callback;
  /**
     State for this->callback.
  */
  void * callbackState;
};
typedef struct fsl_ckout_revert_opt fsl_ckout_revert_opt;
/**
   Initialized-with-defaults fsl_ckout_revert_opt instance,
   intended for use in const-copy initialization.
*/
#define fsl_ckout_revert_opt_empty_m { \
  NULL/*filename*/,NULL/*vfileIds*/,true/*relativeToCwd*/,true/*scanForChanges*/, \
  NULL/*callback*/,NULL/*callbackState*/      \
}
/**
   Initialized-with-defaults fsl_ckout_revert_opt instance,
   intended for use in non-const copy initialization.
*/
FSL_EXPORT const fsl_ckout_revert_opt fsl_ckout_revert_opt_empty;

/**
   Reverts changes to checked-out files, replacing their
   on-disk versions with the current checkout's version.

   If zFilename refers to a directory, all managed files under that
   directory are reverted (if modified). If zFilename is NULL or
   empty, all modifications in the current checkout are reverted.

   If a file has been added but not yet committed, the add
   is un-queued but the file is otherwise untouched. If the
   file has been queued for removal, this removes it from
   that queue as well as restores its contents.

   If a rename is pending for the given filename, the name may match
   either its original name or new name. Whether or not that will
   actually work when file A is renamed to B and file C is renamed to
   A is anyone's guess. (Noting that (fossil mv) won't allow that
   situation to exist in the vfile table for a single checkout
   version, so it seems safe enough.)

   If the 4th argument is not NULL then it is called for each revert
   (_after_ the revert happens) to report what was done with that
   file. It gets passed the checkout-relative name of each reverted
   file. The 5th argument is not interpreted by this function but is
   passed on as-is as the final argument to the callback. If the
   callback returns non-0, the revert process is cancelled, any
   pending transaction is set to roll back, and that error code is
   returned. Note that cancelling a revert mid-process will leave file
   changes made by the revert so far in place, and thus the checkout
   db and filesystem will be in an inconsistent state until
   fsl_vfile_changes_scan() (or equivalent) is called to restore
   balance to the world.

   Files which are not actually reverted because their contents or
   permissions were not modified on disk are not reported to the
   callback unless the reversion was the un-queuing of an ADD or
   REMOVE operation.

   Returns 0 on success, any number of non-0 results on error.
*/
FSL_EXPORT int fsl_ckout_revert( fsl_cx * f,
                                 fsl_ckout_revert_opt const * opt );

/**
   Expects f to have an opened checkout and zName to be the name of
   an entry in the vfile table where vfile.vid == vid. If vid<=0 then
   the current checkout RID is used. This function does not do any
   path resolution or normalization on zName and checks only for an
   exact match (honoring f's case-sensitivity setting - see
   fsl_cx_case_sensitive_set()).

   On success it returns 0 and assigns *vfid to the vfile.id value of
   the matching file.  If no match is found, 0 is returned and *vfile
   is set to 0. Returns FSL_RC_NOT_A_CKOUT if no checkout is opened,
   FSL_RC_RANGE if zName is not a simple path (see
   fsl_is_simple_pathname()), and any number of codes for db-related
   errors.

   This function matches only vfile.pathname, not vfile.origname,
   because it is possible for a given name to be in both fields (in
   different records) at the same time.
*/
FSL_EXPORT int fsl_filename_to_vfile_id( fsl_cx * f, fsl_id_t vid,
                                         char const * zName,
                                         fsl_id_t * vfid );

/**
   Searches the vfile table where vfile.vid=vid for a name which
   matches zName or all vfile entries found under a subdirectory named
   zName (with no trailing slash). zName must be relative to the
   checkout root. As a special case, if zName is NULL, empty, or "."
   then all files in vfile with the given vid are selected. For each
   entry it finds, it adds the vfile.id to dest. If vid<=0 then the
   current checkout RID is used.

   If changedOnly is true then only entries which have been marked in
   the vfile table as having some sort of change are included, so if
   true then fsl_ckout_changes_scan() (or equivalent) must have been
   "recently" called to ensure that state is up to do.

   This search honors the context-level case-sensitivity setting (see
   fsl_cx_case_sensitive_set()).

   Returns 0 on success. Not finding anything is not treated as an
   error, though we could arguably return FSL_RC_NOT_FOUND for the
   cases which use this function. In order to determine whether or
   not any results were found, compare dest->entryCount before and
   after calling this.

   This function matches only vfile.pathname, not vfile.origname,
   because it is possible for a given name to be in both fields (in
   different records) at the same time.

   @see fsl_ckout_vfile_ids()
*/
FSL_EXPORT int fsl_filename_to_vfile_ids( fsl_cx * f, fsl_id_t vid,
                                          fsl_id_bag * dest,
                                          char const * zName,
                                          bool changedOnly);

/**
   This is a variant of fsl_filename_to_vfile_ids() which accepts
   filenames in a more flexible form than that routine. This routine
   works exactly like that one except for the following differences:

   1) The given filename and the relativeToCwd arguments are passed to
   by fsl_ckout_filename_check() to canonicalize the name and ensure
   that it points to someplace within f's current checkout.

   2) Because of (1), zName may not be NULL or empty. To fetch all of
   the vfile IDs for the current checkout, pass a zName of "."  and
   relativeToCwd=false.

   Returns 0 on success, FSL_RC_MISUSE if zName is NULL or empty,
   FSL_RC_OOM on allocation error, FSL_RC_NOT_A_CKOUT if f has no
   opened checkout.
*/
FSL_EXPORT int fsl_ckout_vfile_ids( fsl_cx * f, fsl_id_t vid,
                                    fsl_id_bag * dest, char const * zName,
                                    bool relativeToCwd, bool changedOnly );


/**
   This "mostly internal" routine (re)populates f's checkout vfile
   table with all files from the given checkin manifest. If
   manifestRid is 0 or less then the current checkout's RID is
   used. If vfile already contains any content for the given checkin,
   it is left intact (and several processes rely on that behavior to
   keep it from nuking, e.g., as-yet-uncommitted queued add/rm
   entries).

   Returns 0 on success, any number of codes on any of many potential
   errors.

   f must not be NULL and must have opened checkout and repository
   databases. In debug builds it will assert that that is so.

   If the 3rd argument is true, any entries in vfile for checkin
   versions other than the one specified in the 2nd argument are
   cleared from the vfile table. That is _almost_ always the desired
   behavior, but there are rare cases where vfile needs to temporarily
   (for the duration of a single transaction) hold state for multiple
   versions.

   If the 4th argument is not NULL, it gets assigned the number of
   blobs from the given version which are currently missing from the
   repository due to being phantoms (as opposed to being shunned).

   Returns 0 on success, FSL_RC_NOT_A_CKOUT if no checkout is opened,
   FSL_RC_OOM on allocation error, FSL_RC_DB for db-related problems,
   et.al.

   Misc. notes:

   - This does NOT update the "checkout" vvar table entry because this
   routine is sometimes used in contexts where we need to briefly
   maintain two vfile versions and keep the previous checkout version.

   - Apps must take care to not leave more than one version in the
   vfile table for longer than absolutely needed. They "really should"
   use fsl_vfile_unload() to clear out any version they load with this
   routine.

   @see fsl_vfile_unload()
   @see fsl_vfile_unload_except()
*/
FSL_EXPORT int fsl_vfile_load(fsl_cx * f, fsl_id_t manifestRid,
                              bool clearOtherVersions,
                              uint32_t * missingCount);

/**
   Clears out all entries in the current checkout's vfile table with
   the given vfile.vid value. If vid<=0 then the current checkout RID
   is used (which is never a good idea from client-side code!).

   ACHTUNG: never do this without understanding the consequences. It
   can ruin the current checkout state.

   Returns 0 on success, FSL_RC_NOT_A_CKOUT if f has no checkout
   opened, FSL_RC_DB on any sort of db-related error (in which case
   f's error state is updated with a description of the problem).

   @see fsl_vfile_load()
   @see fsl_vfile_unload_except()
*/
FSL_EXPORT int fsl_vfile_unload(fsl_cx * f, fsl_id_t vid);

/**
   A counterpart of fsl_vfile_unload() which removes all vfile
   entries where vfile.vid is not the given vid. If vid is <=0 then
   the current checkout RID is used.

   Returns 0 on success, FSL_RC_NOT_A_CKOUT if f has no checkout
   opened, FSL_RC_DB on any sort of db-related error (in which case
   f's error state is updated with a description of the problem).

   @see fsl_vfile_load()
   @see fsl_vfile_unload()
*/
FSL_EXPORT int fsl_vfile_unload_except(fsl_cx * f, fsl_id_t vid);


/**
   Performs a "fingerprint check" between f's current checkout and
   repository databases. Returns 0 if either there is no mismatch or
   it is impossible to determine because the checkout is missing a
   fingerprint (which is legal for "older" checkout databases).

   If a mismatch is found, FSL_RC_REPO_MISMATCH is returned. Returns
   some other non-0 code on a lower-level error (db access, OOM,
   etc.).

   A mismatch can happen when the repository to which a checkout
   belongs is replaced, either with a completely different repository
   or a copy/clone of that same repository. Each repository copy may
   have differing blob.rid values, and those are what the checkout
   database uses to refer to repository-side data. If those RIDs
   change, then the checkout is left pointing to data other than what
   it should be.

   TODO: currently the library offers no automated recovery mechanism
   from a mismatch, the only remedy being to close the checkout
   database, destroy it, and re-create it. fossil(1) is able, in some cases,
   to automatically recover from this situation.
*/
FSL_EXPORT int fsl_ckout_fingerprint_check(fsl_cx * f);

#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_CHECKOUT_H_INCLUDED */
