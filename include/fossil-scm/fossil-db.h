/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_FSL_DB_H_INCLUDED)
#define ORG_FOSSIL_SCM_FSL_DB_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).


  ******************************************************************************
  This file declares public APIs for working with fossil's database
  abstraction layer.
*/

#include "fossil-core.h" /* MUST come first b/c of config macros */
/*
  We don't _really_ want to include sqlite3.h at this point, but if we
  do not then we have to typedef the sqlite3 struct here and that
  breaks when client code includes both this file and sqlite3.h.
*/
#include "sqlite3.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
   Potential TODO. Maybe not needed - v1 uses only(?) 1 hook and we
   can do that w/o hooks.
*/
typedef int (*fsl_commit_hook_f)( void * state );
/** Potential TODO */
struct fsl_commit_hook {
  fsl_commit_hook_f hook;
  int sequence;
  void * state;
};
#define fsl_commit_hook_empty_m {NULL,0,NULL}
typedef struct fsl_commit_hook fsl_commit_hook;
/* extern const fsl_commit_hook fsl_commit_hook_empty; */

/**
   Potential TODO.
*/
FSL_EXPORT int fsl_db_before_commit_hook( fsl_db * db, fsl_commit_hook_f f,
                               int sequence, void * state );


#if 0
/* We can't do this because it breaks when clients include both
   this header and sqlite3.h. Is there a solution which lets us
   _not_ include sqlite3.h from this file and also compiles when
   clients include both?
*/
#if !defined(SQLITE_OK)
/**
   Placeholder for sqlite3/4 type. We currently use v3 but will
   almost certainly switch to v4 at some point. Before we can do
   that we need an upgrade/migration path.
*/
typedef struct sqlite3 sqlite3;
#endif
#endif

/**
   A level of indirection to "hide" the actual db driver
   implementation from the public API. Whether or not the API
   uses/will use sqlite3 or 4 is "officially unspecified."  We
   currently use 3 because (A) it bootstraps development and
   testing by letting us use existing fossil repos for and (B) it
   reduces the number of potential problems when porting SQL-heavy
   code from the v1 tree. Clients should try not to rely on the
   underlying db driver API, but may need it for some uses
   (e.g. binding custom SQL functions).
*/
typedef sqlite3 fsl_dbh_t;

/**
   Db handle wrapper class. Each instance wraps a single sqlite
   database handle.

   Fossil is built upon sqlite3, but this abstraction is intended
   to hide that, insofar as possible, from clients so as to
   simplify an eventual port from v3 to v4. Clients should avoid
   relying on the underlying db being sqlite (or at least not rely
   on a specific version), but may want to register custom
   functions with the driver (or perform similar low-level
   operations) and the option is left open for them to access that
   handle via the fsl_db::dbh member.

   @see fsl_db_open();
   @see fsl_db_close();
   @see fsl_stmt
*/
struct fsl_db {
  /**
     Fossil Context on whose behalf this instance is operating, if
     any. Certain db operations behave differently depending
     on whether or not this is NULL.
  */
  fsl_cx * f;

  /**
     Describes what role(s) this db connection plays in fossil (if
     any). This is a bitmask of fsl_dbrole_e values, and a db
     connection may have multiple roles. This is only used by the
     fsl_cx-internal API.
  */
  int role;

  /**
     Underlying db driver handle.
  */
  fsl_dbh_t * dbh;

  /**
     Holds error state from the underlying driver.  fsl_db and
     fsl_stmt operations which fail at the driver level "should"
     update this state to include error info from the driver.
     fsl_cx APIs which fail at the DB level uplift this (using
     fsl_error_move()) so that they can pass it on up the call chain.
  */
  fsl_error error;

  /**
     Holds the file name used when opening this db. Might not refer to
     a real file (e.g. might be ":memory:" or "" (similar to
     ":memory:" but may swap to temp storage).

     Design note: we currently hold the name as it is passed to the
     db-open routine, without canonicalizing it. That is very possibly
     a mistake, as it makes it impossible to properly compare the name
     to another arbitrary checkout-relative name for purposes of
     fsl_reserved_fn_check(). For purposes of fsl_cx we currently
     (2021-03-12) canonicalize db's which we fsl_db_open(), but not
     those which we ATTACH (which includes the repo and checkout
     dbs). We cannot reasonably canonicalize the repo db filename
     because it gets written into the checkout db so that the checkout
     knows where to find the repository. History has shown that that
     path needs to be stored exactly as a user entered it, which is
     often relative.
  */
  char * filename;

  /**
     Holds the database name for use in creating queries.
     Might or might not be set/needed, depending on
     the context.
  */
  char * name;

  /**
     Debugging/test counter. Closing a db with opened statements
     might assert() or trigger debug output when the db is closed.
  */
  int openStatementCount;

  /**
     Counter for fsl_db_transaction_begin/end().
  */
  int beginCount;

  /**
     Internal flag for communicating rollback state through the
     call stack. If this is set to a true value,
     fsl_db_transaction_end() calls will behave like a rollback
     regardless of the value of the 2nd argument passed to that
     function. i.e. it propagates a rollback through nested
     transactions.

     Potential TODO: instead of treating this like a boolean, store
     the error number which caused the rollback here.  We'd have to
     go fix a lot of code for that, though :/.
  */
  int doRollback;

  /**
     Internal change counter. Set when a transaction is
     started/committed.

     Maintenance note: it's an int because that's what
     sqlite3_total_changes() returns.
  */
  int priorChanges;

  /**
     List of SQL commands (char *) which should be executed prior
     to a commit. This list is cleared when the transaction counter
     drops to zero as the result of fsl_db_transaction_end()
     or fsl_db_rollback_force().

     TODO? Use (fsl_stmt*) objects instead of strings? Depends on
     how much data we need to bind here (want to avoid an extra
     copy if we need to bind big stuff). That was implemented in
     [9d9375ac2d], but that approach prohibits multi-statement
     pre-commit triggers, so it was not trunked. It's still unknown
     whether we need multi-statement SQL in this context
     (==fossil's infrastructure).

     @see fsl_db_before_commit()
  */
  fsl_list beforeCommit;

  /**
     An internal cache of "static" queries - those which do not
     rely on call-time state unless that state can be
     bind()ed. Holds a linked list of (fsl_stmt*) instances.

     @see fsl_db_prepare_cached()
  */
  fsl_stmt * cacheHead;

  /**
     A marker which tells fsl_db_close() whether or not
     fsl_db_malloc() allocated this instance (in which case
     fsl_db_close() will fsl_free() it) or not (in which case it
     does not free() it).
  */
  void const * allocStamp;
};
/**
   Empty-initialized fsl_db structure, intended for const-copy
   initialization.
*/
#define fsl_db_empty_m {                        \
    NULL/*f*/,                                  \
      FSL_DBROLE_NONE,                         \
      NULL/*dbh*/,                              \
      fsl_error_empty_m /*error*/,              \
      NULL/*filename*/,                         \
      NULL/*name*/,                             \
      0/*openStatementCount*/,                  \
      0/*beginCount*/,                          \
      0/*doRollback*/,                          \
      0/*priorChanges*/,                        \
      fsl_list_empty_m/*beforeCommit*/,         \
      NULL/*cacheHead*/,                        \
      NULL/*allocStamp*/                        \
      }

/**
   Empty-initialized fsl_db structure, intended for copy
   initialization.
*/
FSL_EXPORT const fsl_db fsl_db_empty;

/**
   If db is not NULL then this function returns its name (the one
   used to open it). The bytes are valid until the db connection is
   closed or until someone mucks with db->filename. If len is not
   NULL then *len is (on success) assigned to the length of the
   returned string, in bytes.  The string is NUL-terminated, so
   fetching the length (by passing a non-NULL 2nd parameter) is
   optional but sometimes helps improve efficiency be removing
   the need for a downstream call to fsl_strlen().

   Returns NULL if !f or f has no checkout opened.
*/
FSL_EXPORT char const * fsl_db_filename(fsl_db const * db, fsl_size_t * len);

typedef sqlite3_stmt fsl_stmt_t;
/**
   Represents a prepared statement handle.
   Intended usage:

   @code
   fsl_stmt st = fsl_stmt_empty;
   int rc = fsl_db_prepare( db, &st, "..." );
   if(rc){ // Error!
   assert(!st.stmt);
   // db->error might hold driver-level error details.
   }else{
   // use st and eventually finalize it:
   fsl_stmt_finalize( &st );
   }
   @endcode


   Script binding implementations can largely avoid exposing the
   statement handle (and its related cleanup ordering requirements)
   to script code. They need to have some mechanism for binding
   values to SQL (or implement all the escaping themselves), but
   that can be done without exposing all of the statement class if
   desired. For example, here's some hypothetical script code:

   @code
   var st = db.prepare(".... where i=:i and x=:x");
   // st is-a Statement, but we need not add script bindings for
   // the whole Statement.bind() API. We can instead simplify that
   // to something like:
   try {
   st.exec( {i: 42, x: 3} )
   // or, for a SELECT query:
   st.each({
   bind{i:42, x:3},
   rowType: 'array', // or 'object'
   callback: function(row,state,colNames){ print(row.join('\t')); },
   state: {...callback function state...}
   });
   } finally {
   st.finalize();
   // It is critical that st gets finalized before its DB, and
   // that'shard to guaranty if we leave st to the garbage collector!
   }
   // see below for another (less messy) alternative
   @endcode

   Ideally, script code should not have direct access to the
   Statement because managing lifetimes can be difficult in the
   face of flow-control changes caused by exceptions (as the above
   example demonstrates). Statements can be completely hidden from
   clients if the DB wrapper is written to support it. For example,
   in pseudo-JavaScript that might look like:

   @code
   db.exec("...where i=? AND x=?", 42, 3);
   db.each({sql:"select ... where id<?", bind:[10],
   rowType: 'array', // or 'object'
   callback: function(row,state,colNames){ print(row.join('\t')); },
   state: {...arbitrary state for the callback...}
   });
   @endcode
*/
struct fsl_stmt {
  /**
     The db which prepared this statement.
  */
  fsl_db * db;

  /**
     Underlying db driver-level statement handle. Clients should
     not rely on the specify concrete type if they can avoid it, to
     simplify an eventual port from sqlite3 to sqlite4.
  */
  fsl_stmt_t * stmt;

  /**
     SQL used to prepare this statement.
  */
  fsl_buffer sql;

  /**
     Number of result columns in this statement. Cached when the
     statement is prepared. Is a signed type because the underlying
     API does it this way.
  */
  int colCount;

  /**
     Number of bound parameter indexes in this statement. Cached
     when the statement is prepared. Is a signed type because the
     underlying API does it this way.
  */
  int paramCount;

  /**
     The number of times this statement has fetched a row via
     fsl_stmt_step().
  */
  fsl_size_t rowCount;

  /**
     Internal state flags.
  */
  int flags;

  /**
     For _internal_ use in creating linked lists. Clients _must_not_
     modify this field.
   */
  fsl_stmt * next;

  /**
     A marker which tells fsl_stmt_finalize() whether or not
     fsl_stmt_malloc() allocated this instance (in which case
     fsl_stmt_finalize() will fsl_free() it) or not (in which case
     it does not free() it).
  */
  void const * allocStamp;
};
/**
   Empty-initialized fsl_stmt instance, intended for use as an
   in-struct initializer.
*/
#define fsl_stmt_empty_m {                      \
    NULL/*db*/,                                 \
      NULL/*stmt*/,                             \
      fsl_buffer_empty_m/*sql*/,                \
      0/*colCount*/,                            \
      0/*paramCount*/,                          \
      0/*rowCount*/,                            \
      1/*flags*/,                               \
      NULL/*next*/,                             \
      NULL/*allocStamp*/                        \
      }

/**
   Empty-initialized fsl_stmt instance, intended for
   copy-constructing.
*/
FSL_EXPORT const fsl_stmt fsl_stmt_empty;

/**
   Allocates a new, cleanly-initialized fsl_stmt instance using
   fsl_malloc(). The returned pointer must eventually be passed to
   fsl_stmt_finalize() to free it (whether or not it is ever passed
   to fsl_db_prepare()).

   Returns NULL on allocation error.
*/
FSL_EXPORT fsl_stmt * fsl_stmt_malloc();


/**
   If db is not NULL this behaves like fsl_error_get(), using the
   db's underlying error state. If !db then it returns
   FSL_RC_MISUSE.
*/
FSL_EXPORT int fsl_db_err_get( fsl_db const * db, char const ** msg, fsl_size_t * len );

/**
   Resets any error state in db, but might keep the string
   memory allocated for later use.
*/
FSL_EXPORT void fsl_db_err_reset( fsl_db * db );

/**
   Prepares an SQL statement for execution. On success it returns
   0, populates tgt with the statement's state, and the caller is
   obligated to eventually pass tgt to fsl_stmt_finalize(). tgt
   must have been cleanly initialized, either via allocation via
   fsl_stmt_malloc() or by copy-constructing fsl_stmt_empty
   resp. fsl_stmt_empty_m (depending on the context).

   On error non-0 is returned and tgt is not modified. If
   preparation of the statement fails at the db level then
   FSL_RC_DB is returned f's error state (fsl_cx_err_get())
   "should" contain more details about the problem. Returns
   FSL_RC_MISUSE if !db, !callback, or !sql. Returns
   FSL_RC_NOT_FOUND if db is not opened. Returns FSL_RC_RANGE if
   !*sql.

   The sql string and the following arguments get routed through
   fsl_appendf(), so any formatting options supported by that
   routine may be used here. In particular, the %%q and %%Q
   formatting options are intended for use in escaping SQL for
   routines such as this one.

   Compatibility note: in sqlite, empty SQL code evaluates
   successfully but with a NULL statement. This API disallows empty
   SQL because it uses NULL as a "no statement" marker and because
   empty SQL is arguably not a query at all.

   Tips:

   - fsl_stmt_col_count() can be used to determine whether a
   statement is a fetching query (fsl_stmt_col_count()>0) or not
   (fsl_stmt_col_count()==0) without having to know the contents
   of the query.

   - fsl_db_prepare_cached() can be used to cache often-used or
   expensive-to-prepare queries within the context of their parent
   db handle.
*/
FSL_EXPORT int fsl_db_prepare( fsl_db *db, fsl_stmt * tgt, char const * sql, ... );

/**
   va_list counterpart of fsl_db_prepare().
*/
FSL_EXPORT int fsl_db_preparev( fsl_db *db, fsl_stmt * tgt, char const * sql, va_list args );

/**
   A special-purpose variant of fsl_db_prepare() which caches
   statements based on their SQL code. This works very much like
   fsl_db_prepare() and friends except that it can return the same
   statement (via *st) multiple times (statements with identical
   SQL are considered equivalent for caching purposes). Clients
   need not explicitly pass the returned statement to
   fsl_stmt_finalize() - the db holds these statements and will
   finalize them when it is closed. It is legal to pass them to
   finalize, in which case they will be cleaned up immediately but
   that also invalidates _all_ pointers to the shared instances.

   If client code does not call fsl_stmt_finalize(), it MUST pass
   the statement pointer to fsl_stmt_cached_yield(st) after is done
   with it. That makes the query available for use again with this
   routine. If a cached query is not yielded via
   fsl_stmt_cached_yield() then this routine will return
   FSL_RC_ACCESS on subsequent requests for that SQL to prevent
   that recursive (mis)use of the statement causes problems.

   This routine is intended to be used in oft-called routines
   where the cost of re-creating statements on each execution could
   be prohibitive (or at least a bummer).

   Returns 0 on success, FSL_RC_MISUSE if any arguments are
   invalid. On error, *st is not written to.  On other error's
   db->error might be updated with more useful information.  See the
   Caveats section below for more details.

   Its intended usage looks like:

   @code
   fsl_stmt * st = NULL;
   int rc = fsl_db_prepare_cached(myDb, &st, "SELECT ...");
   if(rc) { assert(!st); ...error... }
   else {
   ...use it, and _be sure_ to yield it when done:...
   fsl_stmt_cached_yield(st);
   }
   @endcode

   Though this function allows a formatted SQL string, caching is
   generally only useful with statements which have "static" SQL,
   i.e. no call-dependent values embedded within the SQL. It _can_,
   however, contain bind() placeholders which get reset for each
   use. Note that fsl_stmt_cached_yield() resets the statement, so
   most uses of cached statements do not require that the client
   explicitly reset cached statements (doing so is harmless,
   however).

   Caveats:

   Cached queries must not be used in contexts where recursion
   might cause the same query to be returned from this function
   while it is being processed at another level in the execution
   stack. Results would be undefined. Caching is primarily intended
   for often-used routines which bind and fetch simple values, and
   not for queries which bind large inlined values or might invoke
   recursion. Because of the potential for recursive breakage, this
   function flags queries it doles out and requires that clients
   call fsl_stmt_cached_yield() to un-flag them for re-use. It will
   return FSL_RC_ACCESS if an attempt is made to (re)prepare a
   statement for which a fsl_stmt_cached_yield() is pending, and
   db->error will be populated with a (long) error string
   descripting the problem and listing the SQL which caused the
   collision/misuse.


   Design note: for the recursion/parallel use case we "could"
   reimplement this to dole out a new statement (e.g. by appending
   " -- a_number" to the SQL to bypass the collision) and free it in
   fsl_stmt_cached_yield(), but that (A) gets uglier than it needs
   to be and (B) is not needed unless/until we really need cached
   queries in spots which would normally break them. The whole
   recursion problem is still theoretical at this point but could
   easily affect small, often-used queries without recursion.

   @see fsl_db_stmt_cache_clear()
   @see fsl_stmt_cached_yield()
*/
FSL_EXPORT int fsl_db_prepare_cached( fsl_db * db, fsl_stmt ** st, char const * sql, ... );

/**
   The va_list counterpart of fsl_db_prepare_cached().
*/
FSL_EXPORT int fsl_db_preparev_cached( fsl_db * db, fsl_stmt ** st, char const * sql,
                            va_list args );

/**
   "Yields" a statement which was prepared with
   fsl_db_prepare_cached(), such that that routine can once again
   use/re-issue that statement. Statements prepared this way must
   be yielded in order to prevent that recursion causes
   difficult-to-track errors when a given cached statement is used
   concurrently in different code contexts.

   If st is not NULL then this also calls fsl_stmt_reset() on the
   statement (because that simplifies usage of cached statements).

   Returns 0 on success, FSL_RC_MISUSE if !st or if st does not
   appear to have been doled out from fsl_db_prepare_cached().

   @see fsl_db_prepare_cached()
   @see fsl_db_stmt_cache_clear()
*/
FSL_EXPORT int fsl_stmt_cached_yield( fsl_stmt * st );

/**
   Immediately cleans up all cached statements.  Returns the number
   of statements cleaned up. It is illegal to call this while any
   of the cached statements are actively being used (have not been
   fsl_stmt_cached_yield()ed), and doing so will lead to undefined
   results if the statement(s) in question are used after this
   function completes.

   @see fsl_db_prepare_cached()
   @see fsl_stmt_cached_yield()
*/
FSL_EXPORT fsl_size_t fsl_db_stmt_cache_clear(fsl_db * db);

/**
   A special-purposes utility which schedules SQL to be executed
   the next time fsl_db_transaction_end() commits a transaction for
   the given db. A commit or rollback will clear all before-commit
   SQL whether it executes them or not. This should not be used as
   a general-purpose trick, and is intended only for use in very
   limited parts of the Fossil infrastructure.

   Before-commit code is only executed if the db has made changes
   since the transaction began. If no changes are recorded
   then before-commit triggers are _not_ run. This is a historical
   behaviour which is up for debate.

   This function does not prepare the SQL, so it does not catch
   errors which happen at prepare-time. Preparation is done (if
   ever) just before the next transaction is committed.

   Returns 0 on success, non-0 on error.

   Potential TODO: instead of storing the raw SQL, prepare the
   statements here and store the statement handles. The main
   benefit would be that this routine could report preport
   preparation errors (which otherwise cause the the commit to
   fail). The down-side is that it prohibits the use of
   multi-statement pre-commit code. We have an implementation of
   this somewhere early on in the libfossil tree, but it was not
   integrated because of the inability to use multi-statement SQL
   with it.
*/
FSL_EXPORT int fsl_db_before_commit( fsl_db *db, char const * sql, ... );

/**
   va_list counterpart to fsl_db_before_commit().
*/
FSL_EXPORT int fsl_db_before_commitv( fsl_db *db, char const * sql, va_list args );


/**
   Frees memory associated with stmt but does not free stmt unless
   it was allocated by fsl_stmt_malloc() (these objects are
   normally stack-allocated, and such object must be initialized by
   copying fsl_stmt_empty so that this function knows whether or
   not to fsl_free() them). Returns FSL_RC_MISUSE if !stmt or it
   has already been finalized (but was not freed).
*/
FSL_EXPORT int fsl_stmt_finalize( fsl_stmt * stmt );

/**
   "Steps" the given SQL cursor one time and returns one of the
   following: FSL_RC_STEP_ROW, FSL_RC_STEP_DONE, FSL_RC_STEP_ERROR.
   On a db error this will update the underlying db's error state.
   This function increments stmt->rowCount by 1 if it returns
   FSL_RC_STEP_ROW.

   Returns FSL_RC_MISUSE if !stmt or stmt has not been prepared.

   It is only legal to call the fsl_stmt_g_xxx() and
   fsl_stmt_get_xxx() functions if this functon returns
   FSL_RC_STEP_ROW. FSL_RC_STEP_DONE is returned upon successfully
   ending iteration or if there is no iteration to perform (e.g. a
   UPDATE or INSERT).


   @see fsl_stmt_reset()
   @see fsl_stmt_reset2()
   @see fsl_stmt_each()
*/
FSL_EXPORT int fsl_stmt_step( fsl_stmt * stmt );

/**
   A callback interface for use with fsl_stmt_each() and
   fsl_db_each(). It will be called one time for each row fetched,
   passed the statement object and the state parameter passed to
   fsl_stmt_each() resp. fsl_db_each().  If it returns non-0 then
   iteration stops and that code is returned UNLESS it returns
   FSL_RC_BREAK, in which case fsl_stmt_each() stops iteration and
   returns 0. i.e. implementations may return FSL_RC_BREAK to
   prematurly end iteration without causing an error.

   This callback is not called for non-fetching queries or queries
   which return no results, though it might (or might not) be
   interesting for it to do so, passing a NULL stmt for that case.

   stmt->rowCount can be used to determine how many times the
   statement has called this function. Its counting starts at 1.

   It is strictly illegal for a callback to pass stmt to
   fsl_stmt_step(), fsl_stmt_reset(), fsl_stmt_finalize(), or any
   similar routine which modifies its state. It must only read the
   current column data (or similar metatdata, e.g. column names)
   from the statement, e.g. using fsl_stmt_g_int32(),
   fsl_stmt_get_text(), or similar.
*/
typedef int (*fsl_stmt_each_f)( fsl_stmt * stmt, void * state );

/**
   Calls the given callback one time for each result row in the
   given statement, iterating over stmt using fsl_stmt_step(). It
   applies no meaning to the callbackState parameter, which gets
   passed as-is to the callback. See fsl_stmt_each_f() for the
   semantics of the callback.

   Returns 0 on success. Returns FSL_RC_MISUSE if !stmt or
   !callback.
*/
FSL_EXPORT int fsl_stmt_each( fsl_stmt * stmt, fsl_stmt_each_f callback,
                              void * callbackState );

/**
   Resets the given statement, analog to sqlite3_reset(). Should be
   called one time between fsl_stmt_step() iterations when running
   multiple INSERTS, UPDATES, etc. via the same statement. If
   resetRowCounter is true then the statement's row counter
   (st->rowCount) is also reset to 0, else it is left
   unmodified. (Most use cases don't use the row counter.)

   Returns 0 on success, FSL_RC_MISUSE if !stmt or stmt has not
   been prepared, FSL_RC_DB if the underlying reset fails (in which
   case the error state of the stmt->db handle is updated to
   contain the error information).

   @see fsl_stmt_db()
   @see fsl_stmt_reset()
*/
FSL_EXPORT int fsl_stmt_reset2( fsl_stmt * stmt, bool resetRowCounter );

/**
   Equivalent to fsl_stmt_reset2(stmt, 0).
*/
FSL_EXPORT int fsl_stmt_reset( fsl_stmt * stmt );

/**
   Returns the db handle which prepared the given statement, or
   NULL if !stmt or stmt has not been prepared.
*/
FSL_EXPORT fsl_db * fsl_stmt_db( fsl_stmt * stmt );

/**
   Returns the SQL string used to prepare the given statement, or
   NULL if !stmt or stmt has not been prepared. If len is not NULL
   then *len is set to the length of the returned string (which is
   NUL-terminated). The returned bytes are owned by stmt and are
   invalidated when it is finalized.
*/
FSL_EXPORT char const * fsl_stmt_sql( fsl_stmt * stmt, fsl_size_t * len );

/**
   Returns the name of the given 0-based result column index, or
   NULL if !stmt, stmt is not prepared, or index is out out of
   range. The returned bytes are owned by the statement object and
   may be invalidated shortly after this is called, so the caller
   must copy the returned value if it needs to have any useful
   lifetime guarantees. It's a bit more complicated than this, but
   assume that any API calls involving the statement handle might
   invalidate the column name bytes.

   The API guarantees that the returned value is either NULL or
   NUL-terminated.

   @see fsl_stmt_param_count()
   @see fsl_stmt_col_count()
*/
FSL_EXPORT char const * fsl_stmt_col_name(fsl_stmt * stmt, int index);

/**
   Returns the result column count for the given statement, or -1 if
   !stmt or it has not been prepared. Note that this value is cached
   when the statement is created. Note that non-fetching queries
   (e.g. INSERT and UPDATE) have a column count of 0. Some non-SELECT
   constructs, e.g. PRAGMA table_info(tname), behave like SELECT
   and have a positive column count.

   @see fsl_stmt_param_count()
   @see fsl_stmt_col_name()
*/
FSL_EXPORT int fsl_stmt_col_count( fsl_stmt const * stmt );

/**
   Returns the bound parameter count for the given statement, or -1
   if !stmt or it has not been prepared. Note that this value is
   cached when the statement is created.

   @see fsl_stmt_col_count()
   @see fsl_stmt_col_name()
*/
FSL_EXPORT int fsl_stmt_param_count( fsl_stmt const * stmt );

/**
   Returns the index of the given named parameter for the given
   statement, or -1 if !stmt or stmt is not prepared.
*/
FSL_EXPORT int fsl_stmt_param_index( fsl_stmt * stmt, char const * param);

/**
   Binds NULL to the given 1-based parameter index.  Returns 0 on
   succcess. Sets the DB's error state on error.
*/
FSL_EXPORT int fsl_stmt_bind_null( fsl_stmt * stmt, int index );

/**
   Equivalent to fsl_stmt_bind_null_name() but binds to
   a named parameter.
*/
FSL_EXPORT int fsl_stmt_bind_null_name( fsl_stmt * stmt, char const * param );

/**
   Binds v to the given 1-based parameter index.  Returns 0 on
   succcess. Sets the DB's error state on error.
*/
FSL_EXPORT int fsl_stmt_bind_int32( fsl_stmt * stmt, int index, int32_t v );

/**
   Equivalent to fsl_stmt_bind_int32() but binds to a named
   parameter.
*/
FSL_EXPORT int fsl_stmt_bind_int32_name( fsl_stmt * stmt, char const * param, int32_t v );

/**
   Binds v to the given 1-based parameter index.  Returns 0 on
   succcess. Sets the DB's error state on error.
*/
FSL_EXPORT int fsl_stmt_bind_int64( fsl_stmt * stmt, int index, int64_t v );

/**
   Equivalent to fsl_stmt_bind_int64() but binds to a named
   parameter.
*/
FSL_EXPORT int fsl_stmt_bind_int64_name( fsl_stmt * stmt, char const * param, int64_t v );

/**
   Binds v to the given 1-based parameter index.  Returns 0 on
   succcess. Sets the Fossil context's error state on error.
*/
FSL_EXPORT int fsl_stmt_bind_double( fsl_stmt * stmt, int index, double v );

/**
   Equivalent to fsl_stmt_bind_double() but binds to a named
   parameter.
*/
FSL_EXPORT int fsl_stmt_bind_double_name( fsl_stmt * stmt, char const * param, double v );

/**
   Binds v to the given 1-based parameter index.  Returns 0 on
   succcess. Sets the DB's error state on error.
*/
FSL_EXPORT int fsl_stmt_bind_id( fsl_stmt * stmt, int index, fsl_id_t v );

/**
   Equivalent to fsl_stmt_bind_id() but binds to a named
   parameter.
*/
FSL_EXPORT int fsl_stmt_bind_id_name( fsl_stmt * stmt, char const * param, fsl_id_t v );

/**
   Binds the first n bytes of v as text to the given 1-based bound
   parameter column in the given statement. If makeCopy is true then
   the binding makes an copy of the data. Set makeCopy to false ONLY
   if you KNOW that the bytes will outlive the binding.

   Returns 0 on success. On error stmt's underlying db's error state
   is updated, hopefully with a useful error message.
*/
FSL_EXPORT int fsl_stmt_bind_text( fsl_stmt * stmt, int index,
                                   char const * v, fsl_int_t n,
                                   bool makeCopy );

/**
   Equivalent to fsl_stmt_bind_text() but binds to a named
   parameter.
*/
FSL_EXPORT int fsl_stmt_bind_text_name( fsl_stmt * stmt, char const * param,
                                        char const * v, fsl_int_t n,
                                        bool makeCopy );
/**
   Binds the first n bytes of v as a blob to the given 1-based bound
   parameter column in the given statement. See fsl_stmt_bind_text()
   for the semantics of the makeCopy parameter and return value.
*/
FSL_EXPORT int fsl_stmt_bind_blob( fsl_stmt * stmt, int index,
                                   void const * v, fsl_size_t len,
                                   bool makeCopy );

/**
   Equivalent to fsl_stmt_bind_blob() but binds to a named
   parameter.
*/
FSL_EXPORT int fsl_stmt_bind_blob_name( fsl_stmt * stmt, char const * param,
                                        void const * v, fsl_int_t len,
                                        bool makeCopy );

/**
   Gets an integer value from the given 0-based result set column,
   assigns *v to that value, and returns 0 on success.

   Returns FSL_RC_RANGE if index is out of range for stmt.
*/
FSL_EXPORT int fsl_stmt_get_int32( fsl_stmt * stmt, int index, int32_t * v );

/**
   Gets an integer value from the given 0-based result set column,
   assigns *v to that value, and returns 0 on success.

   Returns FSL_RC_RANGE if index is out of range for stmt.
*/
FSL_EXPORT int fsl_stmt_get_int64( fsl_stmt * stmt, int index, int64_t * v );

/**
   The fsl_id_t counterpart of fsl_stmt_get_int32(). Depending on
   the sizeof(fsl_id_t), it behaves as one of fsl_stmt_get_int32()
   or fsl_stmt_get_int64().
*/
FSL_EXPORT int fsl_stmt_get_id( fsl_stmt * stmt, int index, fsl_id_t * v );

/**
   Convenience form of fsl_stmt_get_id() which returns the value
   directly but cannot report errors. It returns -1 on error, but
   that is not unambiguously an error value.
*/
FSL_EXPORT fsl_id_t fsl_stmt_g_id( fsl_stmt * stmt, int index );

/**
   Convenience form of fsl_stmt_get_int32() which returns the value
   directly but cannot report errors. It returns 0 on error, but
   that is not unambiguously an error.
*/
FSL_EXPORT int32_t fsl_stmt_g_int32( fsl_stmt * stmt, int index );

/**
   Convenience form of fsl_stmt_get_int64() which returns the value
   directly but cannot report errors. It returns 0 on error, but
   that is not unambiguously an error.
*/
FSL_EXPORT int64_t fsl_stmt_g_int64( fsl_stmt * stmt, int index );

/**
   Convenience form of fsl_stmt_get_double() which returns the value
   directly but cannot report errors. It returns 0 on error, but
   that is not unambiguously an error.
*/
FSL_EXPORT double fsl_stmt_g_double( fsl_stmt * stmt, int index );

/**
   Convenience form of fsl_stmt_get_text() which returns the value
   directly but cannot report errors. It returns NULL on error, but
   that is not unambiguously an error because it also returns NULL
   if the column contains an SQL NULL value. If outLen is not NULL
   then it is set to the byte length of the returned string.
*/
FSL_EXPORT char const * fsl_stmt_g_text( fsl_stmt * stmt, int index, fsl_size_t * outLen );

/**
   Gets double value from the given 0-based result set column,
   assigns *v to that value, and returns 0 on success.

   Returns FSL_RC_RANGE if index is out of range for stmt.
*/
FSL_EXPORT int fsl_stmt_get_double( fsl_stmt * stmt, int index, double * v );

/**
   Gets a string value from the given 0-based result set column,
   assigns *out (if out is not NULL) to that value, assigns *outLen
   (if outLen is not NULL) to *out's length in bytes, and returns 0
   on success. Ownership of the string memory is unchanged - it is owned
   by the statement and the caller should immediately copy it if
   it will be needed for much longer.

   Returns FSL_RC_RANGE if index is out of range for stmt.
*/
FSL_EXPORT int fsl_stmt_get_text( fsl_stmt * stmt, int index, char const **out,
                       fsl_size_t * outLen );

/**
   The Blob counterpart of fsl_stmt_get_text(). Identical to that
   function except that its output result (3rd paramter) type
   differs, and it fetches the data as a raw blob, without any sort
   of string interpretation.

   Returns FSL_RC_RANGE if index is out of range for stmt.
*/
FSL_EXPORT int fsl_stmt_get_blob( fsl_stmt * stmt, int index, void const **out, fsl_size_t * outLen );

/**
   Executes multiple SQL statements, ignoring any results they might
   collect. Returns 0 on success, non-0 on error.  On error
   db->error might be updated to report the problem.
*/
FSL_EXPORT int fsl_db_exec_multi( fsl_db * db, const char * sql, ...);

/**
   va_list counterpart of db_exec_multi().
*/
FSL_EXPORT int fsl_db_exec_multiv( fsl_db * db, const char * sql, va_list args);

/**
   Executes a single SQL statement, skipping over any results
   it may have. Returns 0 on success. On error db's error state
   may be updated.
*/
FSL_EXPORT int fsl_db_exec( fsl_db * db, char const * sql, ... );

/**
   va_list counterpart of fs_db_exec().
*/
FSL_EXPORT int fsl_db_execv( fsl_db * db, char const * sql, va_list args );

/**
   Begins a transaction on the given db. Nested transactions are
   not directly supported but the db handle keeps track of
   open/close counts, such that fsl_db_transaction_end() will not
   actually do anything until the transaction begin/end counter
   goes to 0. Returns FSL_RC_MISUSE if !db or the db is not
   connected, else the result of the underlying db call(s).

   Transactions are an easy way to implement "dry-run" mode for
   some types of applications. For example:

   @code
   char dryRunMode = ...;
   fsl_db_transaction_begin(db);
   ...do your stuff...
   fsl_db_transaction_end(db, dryRunMode ? 1 : 0);
   @endcode

   Here's a tip for propagating error codes when using
   transactions:

   @code
   ...
   if(rc) fsl_db_transaction_end(db, 1);
   else rc = fsl_db_transaction_end(db, 0);
   @endcode

   That ensures that we propagate rc in the face of a rollback but
   we also capture the rc for a commit (which might yet fail). Note
   that a rollback in and of itself is not an error (though it also
   might fail, that would be "highly unusual" and indicative of
   other problems), and we certainly don't want to overwrite that
   precious non-0 rc with a successful return result from a
   rollback (which would, in effect, hide the error from the
   client).
*/
FSL_EXPORT int fsl_db_transaction_begin(fsl_db * db);

/**
   Equivalent to fsl_db_transaction_end(db, 0).
*/
FSL_EXPORT int fsl_db_transaction_commit(fsl_db * db);

/**
   Equivalent to fsl_db_transaction_end(db, 1).
*/
FSL_EXPORT int fsl_db_transaction_rollback(fsl_db * db);

/**
   Forces a rollback of any pending transaction in db, regardless
   of the internal transaction begin/end counter. Returns
   FSL_RC_MISUSE if !db or db is not opened, else returns the value
   of the underlying ROLLBACK call. This also re-sets/frees any
   transaction-related state held by db (e.g. db->beforeCommit).
   Use with care, as this mucks about with db state in a way which
   is not all that pretty and it may confuse downstream code.

   Returns 0 on success.
*/
FSL_EXPORT int fsl_db_rollback_force(fsl_db * db);

/**
   Decrements the transaction counter incremented by
   fsl_db_transaction_begin() and commits or rolls back the
   transaction if the counter goes to 0.

   If doRollback is true then this rolls back (or schedules a
   rollback of) a transaction started by
   fsl_db_transaction_begin(). If doRollback is false is commits
   (or schedules a commit).

   If db fsl_db_transaction_begin() is used in a nested manner and
   doRollback is true for any one of the nested calls, then that
   value will be remembered, such that the downstream calls to this
   function within the same transaction will behave like a rollback
   even if they pass 0 for the second argument.

   Returns FSL_RC_MISUSE if !db or the db is not opened, 0 if
   the transaction counter is above 0, else the result of the
   (potentially many) underlying database operations.

   Unfortunate low-level co-dependency: if db->f is not NULL and
   (db->role & FSL_DBROLE_REPO) then this function may perform
   extra repository-related post-processing on any commit, and
   checking the result code is particularly important for those
   cases.
*/
FSL_EXPORT int fsl_db_transaction_end(fsl_db * db, bool doRollback);

/**
   Returns the given db's current transaction depth. If the value is
   negative, its absolute value represents the depth but indicates
   that a rollback is pending. If it is positive, the transaction is
   still in a "good" state. If it is 0, no transaction is active.
*/
FSL_EXPORT int fsl_db_transaction_level(fsl_db * db);

/**
   Runs the given SQL query on the given db and returns true if the
   query returns any rows, else false. Returns 0 for any error as
   well.
*/
FSL_EXPORT bool fsl_db_exists(fsl_db * db, char const * sql, ... );

/**
   va_list counterpart of fsl_db_exists().
*/
FSL_EXPORT bool fsl_db_existsv(fsl_db * db, char const * sql, va_list args );

/**
   Runs a fetch-style SQL query against DB and returns the first
   column of the first result row via *rv. If the query returns no
   rows, *rv is not modified. The intention is that the caller sets
   *rv to his preferred default (or sentinel) value before calling
   this.

   The format string (the sql parameter) accepts all formatting
   options supported by fsl_appendf().

   Returns 0 on success. On error db's error state is updated and
   *rv is not modified.

   Returns FSL_RC_MISUSE without side effects if !db, !rv, !sql,
   or !*sql.
*/
FSL_EXPORT int fsl_db_get_int32( fsl_db * db, int32_t * rv,
                      char const * sql, ... );

/**
   va_list counterpart of fsl_db_get_int32().
*/
FSL_EXPORT int fsl_db_get_int32v( fsl_db * db, int32_t * rv,
                       char const * sql, va_list args);

/**
   Convenience form of fsl_db_get_int32() which returns the value
   directly but provides no way of checking for errors. On error,
   or if no result is found, defaultValue is returned.
*/
FSL_EXPORT int32_t fsl_db_g_int32( fsl_db * db, int32_t defaultValue,
                            char const * sql, ... );

/**
   The int64 counterpart of fsl_db_get_int32(). See that function
   for the semantics.
*/
FSL_EXPORT int fsl_db_get_int64( fsl_db * db, int64_t * rv,
                      char const * sql, ... );

/**
   va_list counterpart of fsl_db_get_int64().
*/
FSL_EXPORT int fsl_db_get_int64v( fsl_db * db, int64_t * rv,
                       char const * sql, va_list args);

/**
   Convenience form of fsl_db_get_int64() which returns the value
   directly but provides no way of checking for errors. On error,
   or if no result is found, defaultValue is returned.
*/
FSL_EXPORT int64_t fsl_db_g_int64( fsl_db * db, int64_t defaultValue,
                            char const * sql, ... );


/**
   The fsl_id_t counterpart of fsl_db_get_int32(). See that function
   for the semantics.
*/
FSL_EXPORT int fsl_db_get_id( fsl_db * db, fsl_id_t * rv,
                   char const * sql, ... );

/**
   va_list counterpart of fsl_db_get_id().
*/
FSL_EXPORT int fsl_db_get_idv( fsl_db * db, fsl_id_t * rv,
                    char const * sql, va_list args);

/**
   Convenience form of fsl_db_get_id() which returns the value
   directly but provides no way of checking for errors. On error,
   or if no result is found, defaultValue is returned.
*/
FSL_EXPORT fsl_id_t fsl_db_g_id( fsl_db * db, fsl_id_t defaultValue,
                      char const * sql, ... );


/**
   The fsl_size_t counterpart of fsl_db_get_int32(). See that
   function for the semantics. If this function would fetch a
   negative value, it returns FSL_RC_RANGE and *rv is not modified.
*/
FSL_EXPORT int fsl_db_get_size( fsl_db * db, fsl_size_t * rv,
                     char const * sql, ... );

/**
   va_list counterpart of fsl_db_get_size().
*/
FSL_EXPORT int fsl_db_get_sizev( fsl_db * db, fsl_size_t * rv,
                      char const * sql, va_list args);

/**
   Convenience form of fsl_db_get_size() which returns the value
   directly but provides no way of checking for errors. On error,
   or if no result is found, defaultValue is returned.
*/
FSL_EXPORT fsl_size_t fsl_db_g_size( fsl_db * db, fsl_size_t defaultValue,
                          char const * sql, ... );


/**
   The double counterpart of fsl_db_get_int32(). See that function
   for the semantics.
*/
FSL_EXPORT int fsl_db_get_double( fsl_db * db, double * rv,
                                  char const * sql, ... );

/**
   va_list counterpart of fsl_db_get_double().
*/
FSL_EXPORT int fsl_db_get_doublev( fsl_db * db, double * rv,
                                   char const * sql, va_list args);

/**
   Convenience form of fsl_db_get_double() which returns the value
   directly but provides no way of checking for errors. On error,
   or if no result is found, defaultValue is returned.
*/
FSL_EXPORT double fsl_db_g_double( fsl_db * db, double defaultValue,
                                   char const * sql, ... );

/**
   The C-string counterpart of fsl_db_get_int32(). On success *rv
   will be set to a dynamically allocated string copied from the
   first column of the first result row. If rvLen is not NULL then
   *rvLen will be assigned the byte-length of that string. If no
   row is found, *rv is set to NULL and *rvLen (if not NULL) is set
   to 0, and 0 is returned. Note that NULL is also a legal result
   (an SQL NULL translates as a NULL string), The caller must
   eventually free the returned string value using fsl_free().
*/
FSL_EXPORT int fsl_db_get_text( fsl_db * db, char ** rv, fsl_size_t * rvLen,
                                char const * sql, ... );

/**
   va_list counterpart of fsl_db_get_text().
*/
FSL_EXPORT int fsl_db_get_textv( fsl_db * db, char ** rv, fsl_size_t * rvLen,
                      char const * sql, va_list args );

/**
   Convenience form of fsl_db_get_text() which returns the value
   directly but provides no way of checking for errors. On error,
   or if no result is found, NULL is returned. The returned string
   must eventually be passed to fsl_free() to free it.  If len is
   not NULL then if non-NULL is returned, *len will be assigned the
   byte-length of the returned string.
*/
FSL_EXPORT char * fsl_db_g_text( fsl_db * db, fsl_size_t * len,
                      char const * sql,
                      ... );

/**
   The Blob counterpart of fsl_db_get_text(). Identical to that
   function except that its output result (2nd paramter) type
   differs, and it fetches the data as a raw blob, without any sort
   of string interpretation. The returned *rv memory must
   eventually be passed to fsl_free() to free it. If len is not
   NULL then on success *len will be set to the byte length of the
   returned blob. If no row is found, *rv is set to NULL and *rvLen
   (if not NULL) is set to 0, and 0 is returned. Note that NULL is
   also a legal result (an SQL NULL translates as a NULL string),
*/
FSL_EXPORT int fsl_db_get_blob( fsl_db * db, void ** rv, fsl_size_t * len,
                     char const * sql, ... );


/**
   va_list counterpart of fsl_db_get_blob().
*/
FSL_EXPORT int fsl_db_get_blobv( fsl_db * db, void ** rv, fsl_size_t * stmtLen,
                      char const * sql, va_list args );

/**
   Convenience form of fsl_db_get_blob() which returns the value
   directly but provides no way of checking for errors. On error,
   or if no result is found, NULL is returned.
*/
FSL_EXPORT void * fsl_db_g_blob( fsl_db * db, fsl_size_t * len,
                      char const * sql,
                      ... );
/**
   Similar to fsl_db_get_text() and fsl_db_get_blob(), but writes
   its result to tgt, overwriting (not appennding to) any existing
   memory it might hold.

   If asBlob is true then the underlying BLOB API is used to
   populate the buffer, else the underlying STRING/TEXT API is
   used.  For many purposes there will be no difference, but if you
   know you might have binary data, be sure to pass a true value
   for asBlob to avoid any potential encoding-related problems.
*/
FSL_EXPORT int fsl_db_get_buffer( fsl_db * db, fsl_buffer * tgt,
                       char asBlob,
                       char const * sql, ... );

/**
   va_list counterpart of fsl_db_get_buffer().
*/
FSL_EXPORT int fsl_db_get_bufferv( fsl_db * db, fsl_buffer * tgt,
                        char asBlob,
                        char const * sql, va_list args );


/**
   Expects sql to be a SELECT-style query which (potentially)
   returns a result set. For each row in the set callback() is
   called, as described for fsl_stmt_each(). Returns 0 on success.
   The callback is _not_ called for queries which return no
   rows. If clients need to know if rows were returned, they can
   add a counter to their callbackState and increment it from the
   callback.

   Returns FSL_RC_MISUSE if !db, db is not opened, !callback,
   !sql. Returns FSL_RC_RANGE if !*sql.
*/
FSL_EXPORT int fsl_db_each( fsl_db * db, fsl_stmt_each_f callback,
                            void * callbackState, char const * sql, ... );

/**
   va_list counterpart to fsl_db_each().
*/
FSL_EXPORT int fsl_db_eachv( fsl_db * db, fsl_stmt_each_f callback,
                             void * callbackState, char const * sql, va_list args );


/**
   Returns the given Julian date value formatted as an ISO8601
   string (with a fractional seconds part if msPrecision is true,
   else without it).  Returns NULL if !db, db is not connected, j
   is less than 0, or on allocation error. The returned memory must
   eventually be freed using fsl_free().

   If localTime is true then the value is converted to the local time,
   otherwise it is not.

   @see fsl_db_unix_to_iso8601()
   @see fsl_julian_to_iso8601()
   @see fsl_iso8601_to_julian()
*/
FSL_EXPORT char * fsl_db_julian_to_iso8601( fsl_db * db, double j,
                                            char msPrecision, char localTime );

/**
   Returns the given Julian date value formatted as an ISO8601
   string (with a fractional seconds part if msPrecision is true,
   else without it).  Returns NULL if !db, db is not connected, j
   is less than 0, or on allocation error. The returned memory must
   eventually be freed using fsl_free().

   If localTime is true then the value is converted to the local time,
   otherwise it is not.

   @see fsl_db_julian_to_iso8601()
   @see fsl_julian_to_iso8601()
   @see fsl_iso8601_to_julian()
*/
FSL_EXPORT char * fsl_db_unix_to_iso8601( fsl_db * db, fsl_time_t j, char localTime );


/**
   Returns the current time in Julian Date format. Returns a negative
   value if !db or db is not opened.
*/
FSL_EXPORT double fsl_db_julian_now(fsl_db * db);

/**
   Uses the given db to convert the given time string to Julian Day
   format. If it cannot be converted, a negative value is returned.
   The str parameter can be anything suitable for passing to sqlite's:

   SELECT julianday(str)

   Note that this routine will escape str for use with SQL - the
   caller must not do so.

   @see fsl_julian_to_iso8601()
   @see fsl_iso8601_to_julian()
*/
FSL_EXPORT double fsl_db_string_to_julian(fsl_db * db, char const * str);

/**
   Opens the given db file and populates db with its handle.  db
   must have been cleanly initialized by copy-initializing it from
   fsl_db_empty (or fsl_db_empty_m) or by allocating it using
   fsl_db_malloc(). Failure to do so will lead to undefined
   behaviour.

   openFlags may be a mask of FSL_OPEN_F_xxx values, but not all
   are used/supported here. If FSL_OPEN_F_CREATE is _not_ set in
   openFlags and dbFile does not exist, it will return
   FSL_RC_NOT_FOUND. The existence of FSL_OPEN_F_CREATE in the
   flags will cause this routine to try to create the file if
   needed. If conflicting flags are specified (e.g. FSL_OPEN_F_RO
   and FSL_OPEN_F_RWC) then which one takes precedence is
   unspecified and possibly unpredictable.

   As a special case, if dbFile is ":memory:" (for an in-memory
   database) or "" (empty string, for a "temporary" database) then it
   is is passed through without any filesystem-related checks and the
   openFlags are ignored.

   See this page for the differences between ":memory:" and "":

   https://www.sqlite.org/inmemorydb.html

   Returns FSL_RC_MISUSE if !db, !dbFile, !*dbFile, or if db->dbh
   is not NULL (i.e. if it is already opened or its memory was
   default-initialized (use fsl_db_empty to cleanly copy-initialize
   new stack-allocated instances).

   On error db->dbh will be NULL, but db->error might contain error
   details.

   Regardless of success or failure, db should be passed to
   fsl_db_close() to free up all memory associated with it. It is
   not closed automatically by this function because doing so cleans
   up the error state, which the caller will presumably want to
   have.

   If db->f is not NULL when this is called then it is assumed that
   db should be plugged in to the Fossil repository system, and the
   following additional things happen:

   - A number of SQL functions are registered with the db. Details
   are below.

   - If FSL_OPEN_F_SCHEMA_VALIDATE is set in openFlags then the
   db is validated to see if it has a fossil schema.  If that
   validation fails, FSL_RC_REPO_NEEDS_REBUILD or FSL_RC_NOT_A_REPO
   will be returned and db's error state will be updated. db->f
   does not need to be set for that check to work.


   The following SQL functions get registered with the db if db->f
   is not NULL when this function is called:

   - NOW() returns the current time as an integer, as per time(2).

   - FSL_USER() returns the current value of fsl_cx_user_get(),
   or NULL if that is not set.

   - FSL_CONTENT(INTEGER|STRING) returns the undeltified,
   uncompressed content for the blob record with the given ID (if
   the argument is an integer) or symbolic name (as per
   fsl_sym_to_rid()), as per fsl_content_get(). If the argument
   does not resolve to an in-repo blob, a db-level error is
   triggered. If passed an integer, no validation is done on its
   validity, but such checking can be enforced by instead passing
   the the ID as a string in the form "rid:ID". Both cases will
   result in an error if the RID is not found, but the error
   reporting is arguably slightly better for the "rid:ID" case.

   - FSL_SYM2RID(STRING) returns a blob RID for the given symbol,
   as per fsl_sym_to_rid(). Triggers an SQL error if fsl_sym_to_rid()
   fails.

   - FSL_DIRPART(STRING[, BOOL=0]) behaves like fsl_file_dirpart(),
   returning the result as a string unless it is empty, in which case
   the result is an SQL NULL.

   Note that functions described as "triggering a db error" will
   propagate that error, such that fsl_db_err_get() can report it
   to the client.


   @see fsl_db_close()
   @see fsl_db_prepare()
   @see fsl_db_malloc()
*/
FSL_EXPORT int fsl_db_open( fsl_db * db, char const * dbFile, int openFlags );

/**
   Closes the given db handle and frees any resources owned by
   db. Returns 0 on success.

   If db was allocated using fsl_db_malloc() (as determined by
   examining db->allocStamp) then this routine also fsl_free()s it,
   otherwise it is assumed to either be on the stack or part of a
   larger struct and is not freed.

   If db has any pending transactions, they are rolled
   back by this function.
*/
FSL_EXPORT int fsl_db_close( fsl_db * db );

/**
   If db is an opened db handle, this registers a debugging
   function with the db which traces all SQL to the given FILE
   handle (defaults to stdout if outStream is NULL).

   This mechanism is only intended for debugging and exploration of
   how Fossil works. Tracing is often as easy way to ensure that a
   given code block is getting run.

   As a special case, if db->f is not NULL _before_ it is is
   fsl_db_open()ed, then this function automatically gets installed
   if the SQL tracing option is enabled for that fsl_cx instance
   before the db is opened.

   This is a no-op if !db or db is not opened.
*/
FSL_EXPORT void fsl_db_sqltrace_enable( fsl_db * db, FILE * outStream );

/**
   Returns the row ID of the most recent insertion,
   or -1 if !db, db is not connected, or 0 if no inserts
   have been performed.
*/
FSL_EXPORT fsl_id_t fsl_db_last_insert_id(fsl_db *db);

/**
   Returns non-0 (true) if the database (which must be open) table
   identified by zTableName has a column named zColName
   (case-sensitive), else returns 0.
*/
FSL_EXPORT char fsl_db_table_has_column( fsl_db * db, char const *zTableName,
                              char const *zColName );

/**
   If a db name has been associated with db then it is returned,
   otherwise NULL is returned. A db has no name by default, but
   fsl_cx-used ones get their database name assigned to them
   (e.g. "main" for the main db).
*/
FSL_EXPORT char const * fsl_db_name(fsl_db const * db);

  
/**
   Returns a db name string for the given fsl_db_role value. The
   string is static, guaranteed to live as long as the app.  It
   returns NULL (or asserts in debug builds) if passed
   FSL_DBROLE_NONE or some value out of range for the enum.
*/
FSL_EXPORT const char * fsl_db_role_label(enum fsl_dbrole_e r);


/**
   Allocates a new fsl_db instance(). Returns NULL on allocation
   error. Note that fsl_db instances can often be used from the
   stack - allocating them dynamically is an uncommon case necessary
   for script bindings.

   Achtung: the returned value's allocStamp member is used for
   determining if fsl_db_close() should free the value or not.  Thus
   if clients copy over this value without adjusting allocStamp back
   to its original value, the library will likely leak the instance.
   Been there, done that.
*/
FSL_EXPORT fsl_db * fsl_db_malloc();

/**
   The fsl_stmt counterpart of fsl_db_malloc(). See that function
   for when you might want to use this and a caveat involving the
   allocStamp member of the returned value. fsl_stmt_finalize() will
   free statements created with this function.
*/
FSL_EXPORT fsl_stmt * fsl_stmt_malloc();


/**
   ATTACHes the file zDbName to db using the databbase name
   zLabel. Returns 0 on success. Returns FSL_RC_MISUSE if any
   argument is NULL or any string argument starts with a NUL byte,
   else it returns the result of fsl_db_exec() which attaches the
   db. On db-level errors db's error state will be updated.
*/
FSL_EXPORT int fsl_db_attach(fsl_db * db, const char *zDbName, const char *zLabel);

/**
   The converse of fsl_db_detach(). Must be passed the same arguments
   which were passed as the 1st and 3rd arguments to fsl_db_attach().
   Returns 0 on success, FSL_RC_MISUSE if !db, !zLabel, or !*zLabel,
   else it returns the result of the underlying fsl_db_exec()
   call.
*/
FSL_EXPORT int fsl_db_detach(fsl_db * db, const char *zLabel);


/**
   Expects fmt to be a SELECT-style query. For each row in the
   query, the first column is fetched as a string and appended to
   the tgt list.

   Returns 0 on success, FSL_RC_MISUSE if !db, !tgt, or !fmt, any
   number of potential FSL_RC_OOM or db-related errors.

   Results rows with a NULL value (resulting from an SQL NULL) are
   added to the list as NULL entries.

   Each entry appended to the list is a (char *) which must
   be freed using fsl_free(). To easiest way to clean up
   the list and its contents is:

   @code
   fsl_list_visit_free(tgt,...);
   @endcode

   On error the list may be partially populated.

   Complete example:
   @code
   fsl_list li = fsl_list_empty;
   int rc = fsl_db_select_slist(db, &li,
   "SELECT uuid FROM blob WHERE rid<20");
   if(!rc){
   fsl_size_t i;
   for(i = 0;i < li.used; ++i){
   char const * uuid = (char const *)li.list[i];
   fsl_fprintf(stdout, "UUID: %s\n", uuid);
   }
   }
   fsl_list_visit_free(&li, 1);
   @endcode

   Of course fsl_list_visit() may be used to traverse the list as
   well, as long as the visitor expects (char [const]*) list
   elements.
*/
FSL_EXPORT int fsl_db_select_slist( fsl_db * db, fsl_list * tgt,
                         char const * fmt, ... );

/**
   The va_list counterpart of fsl_db_select_slist().
*/
FSL_EXPORT int fsl_db_select_slistv( fsl_db * db, fsl_list * tgt,
                          char const * fmt, va_list args );

/**
   Returns n bytes of random lower-case hexidecimal characters
   using the given db as its data source, plus a terminating NUL
   byte. The returned memory must eventually be freed using
   fsl_free(). Returns NULL if !db, !n, or on a db-level error.
*/
FSL_EXPORT char * fsl_db_random_hex(fsl_db * db, fsl_size_t n);

/**
   Returns the "number of database rows that were changed or
   inserted or deleted by the most recently completed SQL statement"
   (to quote the underlying APIs). Returns 0 if !db or if db is not
   opened.


   See: https://sqlite.org/c3ref/changes.html
*/
FSL_EXPORT int fsl_db_changes_recent(fsl_db * db);
  
/**
   Returns "the number of row changes caused by INSERT, UPDATE or
   DELETE statements since the database connection was opened" (to
   quote the underlying APIs). Returns 0 if !db or if db is not
   opened.

   See; https://sqlite.org/c3ref/total_changes.html
*/
FSL_EXPORT int fsl_db_changes_total(fsl_db * db);

/**
   Initializes the given database file. zFilename is the name of
   the db file. It is created if needed, but any directory
   components are not created. zSchema is the base schema to
   install.  The following arguments may be (char const *) SQL
   code, each of which gets run against the db after the main
   schema is called.  The variadic argument list MUST end with NULL
   (0), even if there are no non-NULL entries.

   Returns 0 on success.

   On error, if err is not NULL then it is populated with any error
   state from the underlying (temporary) db handle.
*/
FSL_EXPORT int fsl_db_init( fsl_error * err, char const * zFilename,
                 char const * zSchema, ... );

/**
   A fsl_stmt_each_f() impl, intended primarily for debugging, which
   simply outputs row data in tabular form via fsl_output(). The
   state argument is ignored. This only works if stmt was prepared
   by a fsl_db instance which has an associated fsl_cx instance. On
   the first row, the column names are output.
*/
FSL_EXPORT int fsl_stmt_each_f_dump( fsl_stmt * stmt, void * state );

/**
   Returns true if the table name specified by the final argument
   exists in the fossil database specified by the 2nd argument on the
   db connection specified by the first argument, else returns false.

   Potential TODO: this is a bit of a wonky interface. Consider
   changing it to eliminate the role argument, which is only really
   needed if we have duplicate table names across attached dbs or if
   we internally mess up and write a table to the wrong db.
*/
FSL_EXPORT bool fsl_db_table_exists(fsl_db * db, fsl_dbrole_e whichDb, const char *zTable);

/**
   The elipsis counterpart of fsl_stmt_bind_fmtv().
*/
FSL_EXPORT int fsl_stmt_bind_fmt( fsl_stmt * st, char const * fmt, ... );

/**
    Binds a series of values using a formatting string.
   
    The string may contain the following characters, each of which
    refers to the next argument in the args list:
   
    '-': binds a NULL and expects a NULL placeholder
    in the argument list (for consistency's sake).
   
    'i': binds an int32
   
    'I': binds an int64
   
    'R': binds a fsl_id_t ('R' as in 'RID')
   
    'f': binds a double
   
    's': binds a (char const *) as text or NULL.
   
    'S': binds a (char const *) as a blob or NULL.
   
    'b': binds a (fsl_buffer const *) as text or NULL.
   
    'B': binds a (fsl_buffer const *) as a blob or NULL.
   
    ' ': spaces are allowed for readability and are ignored.

    Returns 0 on success, any number of other FSL_RC_xxx codes on
    error.

    ACHTUNG: the "sSbB" bindings assume, because of how this API is
    normally used, that the memory pointed to by the given argument
    will outlive the pending step of the given statement, so that
    memory is NOT copied by the binding. Thus results are undefined if
    such an argument's memory is invalidated before the statement is
    done with it.
*/
FSL_EXPORT int fsl_stmt_bind_fmtv( fsl_stmt * st, char const * fmt,
                                   va_list args );

/**
   Works like fsl_stmt_bind_fmt() but:

   1) It calls fsl_stmt_reset() before binding the arguments.

   2) If binding succeeds then it steps the given statement a single
   time.

   3) If the result is NOT FSL_RC_STEP_ROW then it also resets the
   statement before returning. It does not do so for FSL_RC_STEP_ROW
   because doing so would remove the fetched columns (and this is why
   it resets in step (1)).

   Returns 0 if stepping results in FSL_RC_STEP_DONE, FSL_RC_STEP_ROW
   if it produces a result row, or any number of other potential non-0
   codes on error. On error, the error state of st->db is updated.

   Design note: the return value for FSL_RC_STEP_ROW, as opposed to
   returning 0, is necessary for proper statement use if the client
   wants to fetch any result data from the statement afterwards (which
   is illegal if FSL_RC_STEP_ROW was not the result). This is also why
   it cannot reset the statement if that result is returned.
*/
FSL_EXPORT int fsl_stmt_bind_stepv( fsl_stmt * st, char const * fmt,
                                    va_list args );

/**
   The elipsis counterpart of fsl_stmt_bind_stepv().
*/
FSL_EXPORT int fsl_stmt_bind_step( fsl_stmt * st, char const * fmt, ... );

#if defined(__cplusplus)
} /*extern "C"*/
#endif
#endif
/* ORG_FOSSIL_SCM_FSL_DB_H_INCLUDED */
