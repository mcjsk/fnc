/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_CONFDB_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_CONFDB_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).
*/
/** @file fossil-confdb.h

    fossil-confdb.h declares APIs dealing with fossil's various
    configuration option storage backends.
*/

#include "fossil-core.h" /* MUST come first b/c of config macros */

#if defined(__cplusplus)
extern "C" {
#endif

/**
   A flag type for specifying which configuration backend a given API
   should be applied to. Used by most of the fsl_config_XXX() APIS,
   e.g. fsl_config_get_int32() and friends.

   This type is named fsl_confdb_e (not the "db" part) because 3 of
   the 4 fossil-supported config storage backends are databases. The
   "versioned setting" backend, which deals with non-db local files,
   was encapsulated into this API long after the db-related options
   were
*/
enum fsl_confdb_e {
/**
   Signfies the global-level (per system user) configuration area.
*/
FSL_CONFDB_GLOBAL = 1,
/**
   Signfies the repository-level configuration area.
*/
FSL_CONFDB_REPO = 2,
/**
   Signfies the checkout-level (a.k.a. "local") configuration area.
*/
FSL_CONFDB_CKOUT = 3,

/**
   The obligatory special case...

   Versionable settings are stored directly in SCM-controlled files,
   each of which has the same name as the setting and lives in the
   .fossil-settings directory of a checkout. Though versionable
   settings _can_ be read from a non-checked-out repository, doing so
   requires knowning which version to fetch and is horribly
   inefficient, so there are currently no APIs for doing so.

   Note that the APIs which read and write versioned settings do not
   care whether those settings are valid for fossil(1).

   Reminder to self: SQL to find the checkins in which a versioned
   settings file was added or modified (ignoring renames and branches
   and whatnot).

   @code
select strftime('%Y-%m-%d %H:%M:%S',e.mtime) mtime,
b.rid, b.uuid from mlink m, filename f, blob b, event e
where f.fnid=m.fnid
and m.mid=b.rid
and b.rid=e.objid
and f.name='.fossil-settings/ignore-glob'
order by e.mtime desc 
;
   @endcode

*/
FSL_CONFDB_VERSIONABLE = 4
};
typedef enum fsl_confdb_e fsl_confdb_e;

/**
   Returns the name of the db table associated with the given
   mode. Results are undefined if mode is an invalid value. The
   returned bytes are static and constant.

   Returns NULL for the role FSL_CONFDB_VERSIONABLE.
*/
FSL_EXPORT char const * fsl_config_table_for_role(fsl_confdb_e mode);

/**
   Returns a handle to the db associates with the given fsl_confdb_e
   value. Returns NULL if !f or if f has no db opened for that
   configuration role. Results are undefined if mode is an invalid
   value.

   For FSL_CONFDB_VERSIONABLE it returns the results of fsl_cx_db(),
   even though there is no database-side support for versionable files
   (which live in files in a checkout).
*/
FSL_EXPORT fsl_db * fsl_config_for_role(fsl_cx * f, fsl_confdb_e mode);

/**
   Returns the int32 value of a property from one of f's config
   dbs, as specified by the mode parameter. Returns dflt if !f, f
   does not have the requested config db opened, no entry is found,
   or on db-level errors.
*/
FSL_EXPORT int32_t fsl_config_get_int32( fsl_cx * f, fsl_confdb_e mode,
                                         int32_t dflt, char const * key );
/**
   int64_t counterpart of fsl_config_get_int32().
*/
FSL_EXPORT int64_t fsl_config_get_int64( fsl_cx * f, fsl_confdb_e mode,
                                         int64_t dflt, char const * key );

/**
   fsl_id_t counterpart of fsl_config_get_int32().
*/
FSL_EXPORT fsl_id_t fsl_config_get_id( fsl_cx * f, fsl_confdb_e mode,
                                       fsl_id_t dflt, char const * key );
/**
   double counterpart of fsl_config_get_int32().
*/
FSL_EXPORT double fsl_config_get_double( fsl_cx * f, fsl_confdb_e mode,
                                         double dflt, char const * key );


/**
   Boolean countertpart of fsl_config_get_int32().

   fsl_str_bool() is used to determine the booleanness (booleanity?)
   of a given config option.
*/
FSL_EXPORT bool fsl_config_get_bool( fsl_cx * f, fsl_confdb_e mode,
                                     bool dflt, char const * key );

/**
   A convenience form of fsl_config_get_buffer().  If it finds a
   config entry it returns its value. If *len is not NULL then *len is
   assigned the length of the returned string, in bytes (and is set to
   0 if NULL is returned). Any errors encounters while looking for
   the entry are suppressed and NULL is returned.

   The returned memory must eventually be freed using fsl_free(). If
   len is not NULL then it is set to the length of the returned
   string. Returns NULL for any sort of error or for a NULL db value.

   If capturing error state is important for a given use case, use
   fsl_config_get_buffer() instead, which provides the same features
   as this one but propagates any error state.
*/
FSL_EXPORT char * fsl_config_get_text( fsl_cx * f, fsl_confdb_e mode,
                                       char const * key,
                                       fsl_size_t * len );

/**
   The fsl_buffer-type counterpart of fsl_config_get_int32().

   Replaces the contents of the given buffer (re-using any memory it
   might have) with a value from a config region. Returns 0 on
   success, FSL_RC_NOT_FOUND if no entry was found or the requested db
   is not opened, FSL_RC_OOM on allocation errror.

   If mode is FSL_CONFDB_VERSIONABLE, this operation requires
   a checkout and returns (if possible) the contents of the file
   named {CHECKOUT_ROOT}/.fossil-settings/{key}.

   On any sort of error, including the inability to find or open
   a versionable-settings file, non-0 is returned:

   - FSL_RC_OOM on allocation error

   - FSL_RC_NOT_FOUND if either the database referred to by mode is
   not opened or a matching setting cannot be found.

   - FSL_RC_ACCESS is a versionable setting file is found but cannot
   be opened for reading.

   - FSL_RC_NOT_A_CKOUT if mode is FSL_CONFDB_VERSIONABLE and no
   checkout is opened.

   - Potentially one of several db-related codes if reading a
   non-versioned setting fails.

   In the grand scheme of things, the inability to load a setting is
   not generally an error, so clients are not expected to treat it as
   fatal unless perhaps it returns FSL_RC_OOM, in which case it likely
   is.

   @see fsl_config_has_versionable()
*/
FSL_EXPORT int fsl_config_get_buffer( fsl_cx * f, fsl_confdb_e mode,
                                      char const * key, fsl_buffer * b );


/**
   If f has an opened checkout, this replaces b's contents (re-using
   any existing memory) with an absolute path to the filename for that
   setting: {CHECKOUT_ROOT}/.fossil-settings/{key}

   This routine neither verifies that the given key is a valid
   versionable setting name nor that the file exists: it's purely a
   string operation.

   Returns 0 on success, FSL_RC_NOT_A_CKOUT if f has no checkout
   opened, FSL_RC_MISUSE if key is NULL, empty, or if
   fsl_is_simple_pathname() returns false for the key, and FSL_RC_OOM
   if appending to the buffer fails.
 */
FSL_EXPORT int fsl_config_versionable_filename(fsl_cx *f, char const * key, fsl_buffer *b);

/**
   Sets a configuration variable in one of f's config databases, as
   specified by the mode parameter. Returns 0 on success.  val may
   be NULL. Returns FSL_RC_MISUSE if !f, f does not have that
   database opened, or !key, FSL_RC_RANGE if !key.

   If mode is FSL_CONFDB_VERSIONABLE, it attempts to write/overwrite
   the corresponding file in the current checkout.

   If mem is NULL and mode is not FSL_CONFDB_VERSIONABLE then an SQL
   NULL is bound instead of an empty blob. For FSL_CONFDB_VERSIONABLE
   an empty file will be written for that case.

   If mode is FSL_CONFDB_VERSIONABLE, this does NOT queue any
   newly-created versionable setting file for inclusion into the
   SCM. That is up to the caller. See
   fsl_config_versionable_filename() for info about an additional
   potential usage error case with FSL_CONFDB_VERSIONABLE.

   Pedantic side-note: the input text is saved as-is. No trailing
   newline is added when saving to FSL_CONFDB_VERSIONABLE because
   doing so would require making a copy of the input bytes just to add
   a newline to it. The non-text fsl_config_set_XXX() APIs add a
   newline when writing to their values out to a versionable config
   file because it costs them nothing to do so and text files "should"
   have a trailing newline.

   Potential TODO: if mode is FSL_CONFDB_VERSIONABLE and the key
   contains directory components, e,g, "app/x", we should arguably use
   fsl_mkdir_for_file() to create those components. As of this writing
   (2021-03-14), no such config keys have ever been used in fossil.

   @see fsl_config_versionable_filename()
*/
FSL_EXPORT int fsl_config_set_text( fsl_cx * f, fsl_confdb_e mode, char const * key, char const * val );

/**
   The blob counterpart of fsl_config_set_text(). If len is
   negative then fsl_strlen(mem) is used to determine the length of
   the memory.

   If mem is NULL and mode is not FSL_CONFDB_VERSIONABLE then an SQL
   NULL is bound instead of an empty blob. For FSL_CONFDB_VERSIONABLE
   an empty file will be written for that case.
*/
FSL_EXPORT int fsl_config_set_blob( fsl_cx * f, fsl_confdb_e mode, char const * key,
                                    void const * mem, fsl_int_t len );
/**
   int32 counterpart of fsl_config_set_text().
*/
FSL_EXPORT int fsl_config_set_int32( fsl_cx * f, fsl_confdb_e mode,
                                     char const * key, int32_t val );
/**
   int64 counterpart of fsl_config_set_text().
*/
FSL_EXPORT int fsl_config_set_int64( fsl_cx * f, fsl_confdb_e mode,
                                     char const * key, int64_t val );
/**
   fsl_id_t counterpart of fsl_config_set_text().
*/
FSL_EXPORT int fsl_config_set_id( fsl_cx * f, fsl_confdb_e mode,
                                  char const * key, fsl_id_t val );
/**
   fsl_double counterpart of fsl_config_set_text().
*/
FSL_EXPORT int fsl_config_set_double( fsl_cx * f, fsl_confdb_e mode,
                                      char const * key, double val );
/**
   Boolean counterpart of fsl_config_set_text().

   For compatibility with fossil conventions, the value will be saved
   in the string form "on" or "off". When mode is
   FSL_CONFDB_VERSIONABLE, that value will include a trailing newline.
*/
FSL_EXPORT int fsl_config_set_bool( fsl_cx * f, fsl_confdb_e mode,
                                    char const * key, bool val );

/**
   "Unsets" (removes) the given key from the given configuration database.
   It is not considered to be an error if the config table does not
   contain that key.

   Returns FSL_RC_UNSUPPORTED, without side effects, if mode is
   FSL_CONFDB_VERSIONABLE. It "could" hypothetically remove a
   checked-out copy of a versioned setting, then queue the file for
   removal in the next checkin, but it does not do so. It might, in
   the future, be changed to do so, or at least to remove the local
   settings file.
*/
FSL_EXPORT int fsl_config_unset( fsl_cx * f, fsl_confdb_e mode,
                                 char const * key );

/**
   Begins (or recurses) a transaction on the given configuration
   database.  Returns 0 on success, non-0 on error. On success,
   fsl_config_transaction_end() must eventually be called with the
   same parameters to pop the transaction stack. Returns
   FSL_RC_MISUSE if no db handle is opened for the given
   configuration mode. Assuming all arguments are valid, this
   returns the result of fsl_db_transaction_end() and propagates
   any db-side error into the f object's error state.

   This is primarily intended as an optimization when an app is
   making many changes to a config database. It is not needed when
   the app is only making one or two changes.

   @see fsl_config_transaction_end()
   @see fsl_db_transaction_begin()
*/
FSL_EXPORT int fsl_config_transaction_begin(fsl_cx * f, fsl_confdb_e mode);

/**
   Pops the transaction stack pushed by
   fsl_config_transaction_begin(). If rollback is true then the
   transaction is set roll back, otherwise it is allowed to
   continue (if recursive) or committed immediately (if not
   recursive).  Returns 0 on success, non-0 on error. Returns
   FSL_RC_MISUSE if no db handle is opened for the given
   configuration mode. Assuming all arguments are valid, this
   returns the result of fsl_db_transaction_end() and propagates
   any db-side error into the f object's error state.

   @see fsl_config_transaction_begin()
   @see fsl_db_transaction_end()
*/
FSL_EXPORT int fsl_config_transaction_end(fsl_cx * f, fsl_confdb_e mode, char rollback);

/**
   Populates li as a glob list from the given configuration key.
   Uses (versionable/repo/global) config settings, in that order.
   It is not an error if one or more of those sources is missing -
   they are simply skipped.

   Note that gets any new globs appended to it, as per
   fsl_glob_list_append(), as opposed to replacing any existing
   contents.

   Returns 0 on success, but that only means that there were no
   errors, not that any entries were necessarily added to li.

   Arguably a bug: this function does not open the global config if
   it was not already opened, but will use it if it is opened. This
   function should arbuably open and close it in that case.
*/
FSL_EXPORT int fsl_config_globs_load(fsl_cx * f, fsl_list * li, char const * key);

/**
   Fetches the preferred name of the "global" db file for the current
   user by assigning it to *zOut. Returns 0 on success, in which case
   *zOut is updated and non-0 on error, in which case *zOut is not
   modified.  On success, ownership of *zOut is transferred to the
   caller, who must eventually free it using fsl_free().

   The locations searched for the database file are
   platform-dependent...

   Unix-like systems are searched in the following order:

   1) If the FOSSIL_HOME environment var is set, use
   $FOSSIL_HOME/.fossil.

   2) If $HOME/.fossil already exists, use that.

   3) If XDG_CONFIG_HOME environment var is set, use
   $XDG_CONFIG_HOME/fossil.db.

   4) If $HOME/.config is a directory, use $HOME/.config/fossil.db

   5) Fall back to $HOME/.fossil (historical name).

   Except where listed above, this function does not check whether the
   file already exists or is a database.

   Windows:

   - We need a Windows port of this routine. Currently it simply uses
   the Windows home directory + "/_fossil" or "/.fossil", depending on
   the build-time environment.
*/
FSL_EXPORT int fsl_config_global_preferred_name(char ** zOut);

#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_CONFDB_H_INCLUDED */
