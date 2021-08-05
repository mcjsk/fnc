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
/**************************************************************************
  This file houses the code for the fsl_content_xxx() APIS.
*/
#include "fossil-scm/fossil-internal.h"
#include "fossil-scm/fossil-hash.h"
#include "fossil-scm/fossil-checkout.h"
#include <assert.h>
#include <memory.h> /* memcmp() */

/* Only for debugging */
#include <stdio.h>
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)


fsl_int_t fsl_content_size( fsl_cx * f, fsl_id_t blobRid ){
  fsl_db * dbR = f ? fsl_cx_db_repo(f) : NULL;
  if(!f) return -3;
  else if(blobRid<=0) return -4;
  else if(!dbR) return -5;
  else{
    int rc;
    fsl_int_t rv = -2;
    fsl_stmt * q = NULL;
    rc = fsl_db_prepare_cached(dbR, &q,
                               "SELECT size FROM blob "
                               "WHERE rid=? "
                               "/*%s()*/",__func__);
    if(!rc){
      rc = fsl_stmt_bind_id(q, 1, blobRid);
      if(!rc){
        if(FSL_RC_STEP_ROW==fsl_stmt_step(q)){
          rv = (fsl_int_t)fsl_stmt_g_int64(q, 0);
        }
      }
      fsl_stmt_cached_yield(q);
    }
    return rv;
  }
}

bool fsl_content_is_available(fsl_cx * f, fsl_id_t rid){
  fsl_id_t srcid = 0;
  int rc = 0, depth = 0 /* Limit delta recursion depth */;
  while( depth++ < 100000 ){
    if( fsl_id_bag_contains(&f->cache.arty.missing, rid) ){
      return false;
    }else if( fsl_id_bag_contains(&f->cache.arty.available, rid) ){
      return true;
    }else if( fsl_content_size(f, rid)<0 ){
      fsl_id_bag_insert(&f->cache.arty.missing, rid)
        /* ignore possible OOM error */;
      return false;
    }
    rc = fsl_delta_src_id(f, rid, &srcid);
    if(rc) break;
    else if( 0==srcid ){
      fsl_id_bag_insert(&f->cache.arty.available, rid);
      return true;
    }
    rid = srcid;
  }
  if(0==rc){
    /* This "cannot happen" (never has historically, and would be
       indicative of what amounts to corruption in the repo). */
    fsl_fatal(FSL_RC_RANGE,"delta-loop in repository");
  }
  return false;
}



int fsl_content_blob( fsl_cx * f, fsl_id_t blobRid, fsl_buffer * tgt ){
  fsl_db * dbR = f ? fsl_cx_db_repo(f) : NULL;
  if(!f || !tgt) return FSL_RC_MISUSE;
  else if(blobRid<=0) return FSL_RC_RANGE;
  else if(!dbR) return FSL_RC_NOT_A_REPO;
  else{
    int rc;
    fsl_stmt * q = NULL;
    rc = fsl_db_prepare_cached( dbR, &q,
                                "SELECT content, size FROM blob "
                                "WHERE rid=?"
                                "/*%s()*/",__func__);
    if(!rc){
      rc = fsl_stmt_bind_id(q, 1, blobRid);
      if(!rc && (FSL_RC_STEP_ROW==(rc=fsl_stmt_step(q)))){
        void const * mem = NULL;
        fsl_size_t memLen = 0;
        if(fsl_stmt_g_int64(q, 1)<0){
          rc = fsl_cx_err_set(f, FSL_RC_PHANTOM,
                              "Cannot fetch content for phantom "
                              "blob #%"FSL_ID_T_PFMT".",
                              blobRid);
        }else{
          tgt->used = 0;
          fsl_stmt_get_blob(q, 0, &mem, &memLen);
          if(mem && memLen){
            rc = fsl_buffer_append(tgt, mem, memLen);
            if(!rc && fsl_buffer_is_compressed(tgt)){
              rc = fsl_buffer_uncompress(tgt, tgt);
            }
          }
        }
      }else if(FSL_RC_STEP_DONE==rc){
        rc = fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                            "No blob found for rid %"FSL_ID_T_PFMT".",
                            blobRid);
      }
      fsl_stmt_cached_yield(q);
    }
    if(rc && !f->error.code && dbR->error.code){
      fsl_cx_uplift_db_error(f, dbR);
    }
    return rc;
  }
}


bool fsl_content_is_private(fsl_cx * f, fsl_id_t rid){
  fsl_stmt * s1 = NULL;
  fsl_db * db = fsl_cx_db_repo(f);
  int rc = db
    ? fsl_db_prepare_cached(db, &s1,
                            "SELECT 1 FROM private "
                            "WHERE rid=?"
                            "/*%s()*/",__func__)
    : FSL_RC_MISUSE;
  if(!rc){
    rc = fsl_stmt_bind_id(s1, 1, rid);
    if(!rc) rc = fsl_stmt_step(s1);
    fsl_stmt_cached_yield(s1);
  }
  return rc==FSL_RC_STEP_ROW ? true : false;
}


int fsl_content_get( fsl_cx * f, fsl_id_t rid, fsl_buffer * tgt ){
  fsl_db * db = fsl_cx_db_repo(f);
  if(!tgt) return FSL_RC_MISUSE;
  else if(rid<=0){
    return fsl_cx_err_set(f, FSL_RC_RANGE,
                          "RID %"FSL_ID_T_PFMT" is out of range.",
                          rid);
  }
  else if(!db){
    return fsl_cx_err_set(f, FSL_RC_NOT_A_REPO,
                          "Fossil has no repo opened.");
  }
  else{
    int rc;
    bool gotIt = 0;
    fsl_id_t nextRid;
    fsl_acache * const ac = &f->cache.arty;
    fsl_buffer_reuse(tgt);
    if(fsl_id_bag_contains(&ac->missing, rid)){
      /* Early out if we know the content is not available */
      return FSL_RC_NOT_FOUND;
    }

    /* Look for the artifact in the cache first */
    if( fsl_id_bag_contains(&ac->inCache, rid) ){
      fsl_size_t i;
      fsl_acache_line * line;
      for(i=0; i<ac->used; ++i){
        line = &ac->list[i];
        if( line->rid==rid ){
          rc = fsl_buffer_copy(&line->content, tgt);
          line->age = ac->nextAge++;
          return rc;
        }
      }
    }

    nextRid = 0;
    rc = fsl_delta_src_id(f, rid, &nextRid);
    /* MARKER(("rc=%d, nextRid=%"FSL_ID_T_PFMT"\n", rc, nextRid)); */
    if(rc) return rc;
    if( nextRid == 0 ){
      /* This is not a delta, so get its raw content. */
      rc = fsl_content_blob(f, rid, tgt);
      gotIt = 0==rc;
    }else{
      /* Looks like a delta, so let's expand it... */
      fsl_int_t n           /* number of used entries in 'a' */;
      fsl_int_t nAlloc = 10 /* number it items allocated in 'a' */;
      fsl_id_t * a = NULL    /* array of rids we expand */;
      fsl_int_t mx;
      fsl_buffer delta = fsl_buffer_empty;
      fsl_buffer next = fsl_buffer_empty  /* delta-applied content */ ;
      assert(nextRid>0);
      a = fsl_malloc( sizeof(a[0]) * nAlloc );
      if(!a) return FSL_RC_OOM;
      a[0] = rid;
      a[1] = nextRid;
      n = 1;
      while( !fsl_id_bag_contains(&ac->inCache, nextRid)
             && !fsl_delta_src_id(f, nextRid, &nextRid)
             && (nextRid>0)){
        /* Figure out how big n needs to be... */
        ++n;
        if( n >= nAlloc ){
          /* Expand 'a' */
          void * remem;
          if( n > fsl_db_g_int64(db, 0,
                                "SELECT max(rid) FROM blob")){
            rc = fsl_cx_err_set(f, FSL_RC_RANGE,
                                "Infinite loop in delta table.");
            goto end_delta;
          }
          nAlloc = nAlloc * 2;
          remem = fsl_realloc(a, nAlloc*sizeof(a[0]));
          if(!remem){
            rc = FSL_RC_OOM;
            goto end_delta;
          }
          a = (fsl_id_t*)remem;
        }
        a[n] = nextRid;
      }
      /**
         Recursively expand deltas to get the content...
      */
      mx = n;
      rc = fsl_content_get( f, a[n], tgt );
      /* MARKER(("Getting content for rid #%"FSL_ID_T_PFMT", rc=%d\n", a[n], rc)); */
      --n;
      for( ; !rc && (n>=0); --n){
        rc = fsl_content_blob(f, a[n], &delta);
        /* MARKER(("Getting/applying delta rid #%"FSL_ID_T_PFMT", rc=%d\n", a[n], rc)); */
        if(rc) goto end_delta;
        if(!delta.used){
          assert(!"Is this possible? The fossil tree has a similar "
                 "condition but i naively don't believe it's necessary.");
          continue;
        }
        next = fsl_buffer_empty;
        rc = fsl_buffer_delta_apply2(tgt, &delta, &next, &f->error);
        if(rc) goto end_delta;
#if 1
        /*
           In my (very simple) tests this cache costs us more than it
           saves. TODO: re-test this once we can do a 'rebuild', or
           something more intensive than processing a single
           manifest's R-card. At that point we can set a f->flags bit
           to enable or disable this block for per-use-case
           optimization purposes.

           We also probably want to cache fsl_deck instances instead
           of Manifest blobs (fsl_buffer) like fossil(1) does,
           otherwise this cache really doesn't save us much
           work/memory.

           2021-03-24: in a debug build, running:

           f-parseparty -t c -c -q

           (i.e.: parse and crosslink all checkin artifacts)

           on the libfossil repo with 2003 checkins takes:

           10.5s without this cache
           5.2s with this cache

           We shave another 0.5s if we always cache instead of using
           this mysterious (mx-n)%8 heuristic.
        */
        //MARKER(("mx=%d, n=%d, (mx-n)%%8=%d\n",
        //(int)mx, (int)n, (int)(mx-n)%8));
        //MARKER(("nAlloc=%d\n", (int)nAlloc));
        if( (mx-n)%8==0 ){
          //MARKER(("Caching artifact %d\n", (int)a[n+1]));
          rc = fsl_acache_insert( ac, a[n+1], tgt );
          if(rc){
            fsl_buffer_clear(&next);
            goto end_delta;
          }
          assert(!tgt->mem && "Passed to artifact cache.");
        }else{
          fsl_buffer_clear(tgt);
        }
#else
        if(mx){/*unused var*/}
        fsl_buffer_clear(tgt);
#endif
        *tgt = next;
      }
      end_delta:
      fsl_buffer_clear(&delta);
      fsl_free(a);
      gotIt = 0==rc;
    }

    if(!rc){
      rc = fsl_id_bag_insert(gotIt
                             ? &f->cache.arty.available
                             : &f->cache.arty.missing,
                             rid);
    }
    return rc;
  }
}

int fsl_content_get_sym( fsl_cx * f, char const * sym, fsl_buffer * tgt ){
  int rc;
  fsl_db * db = f ? fsl_needs_repo(f) : NULL;
  fsl_id_t rid = 0;
  if(!f || !sym || !tgt) return FSL_RC_MISUSE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  rc = fsl_sym_to_rid(f, sym, FSL_SATYPE_ANY, &rid);
  return rc ? rc : fsl_content_get(f, rid, tgt);
}

/**
    Mark artifact rid as being available now. Update f's cache to show
    that everything that was formerly unavailable because rid was
    missing is now available. Returns 0 on success. f must have
    an opened repo and rid must be valid.
 */
static int fsl_content_mark_available(fsl_cx * f, fsl_id_t rid){
  fsl_id_bag pending = fsl_id_bag_empty;
  int rc;
  fsl_stmt * st = NULL;
  fsl_db * db = fsl_cx_db_repo(f);
  assert(f);
  assert(db);
  assert(rid>0);
  if( fsl_id_bag_contains(&f->cache.arty.available, rid) ) return 0;
  rc = fsl_id_bag_insert(&pending, rid);
  if(rc) goto end;
  while( (rid = fsl_id_bag_first(&pending))!=0 ){
    fsl_id_bag_remove(&pending, rid);
    rc = fsl_id_bag_insert(&f->cache.arty.available, rid);
    if(rc) goto end;
    fsl_id_bag_remove(&f->cache.arty.missing, rid);
    if(!st){
      rc = fsl_db_prepare_cached(db, &st,
                                 "SELECT rid FROM delta "
                                 "WHERE srcid=?"
                                 "/*%s()*/",__func__);
      if(rc) goto end;
    }
    rc = fsl_stmt_bind_id(st, 1, rid);
    while( !rc && (FSL_RC_STEP_ROW==fsl_stmt_step(st)) ){
      fsl_id_t const nx = fsl_stmt_g_id(st,0);
      assert(nx>0);
      rc = fsl_id_bag_insert(&pending, nx);
    }

  }
  end:
  if(st) fsl_stmt_cached_yield(st);
  fsl_id_bag_clear(&pending);
  return rc;
}

/**
   When a record is converted from a phantom to a real record, if that
   record has other records that are derived by delta, then call
   fsl_deck_crosslink() on those other records.

   If the formerly phantom record or any of the other records derived
   by delta from the former phantom are a baseline manifest, then also
   invoke fsl_deck_crosslink() on the delta-manifests associated with
   that baseline.

   Tail recursion is used to minimize stack depth.

   Returns 0 on success, any number of non-0 results on error.

   The 3rd argument must always be false except in recursive calls to
   this function.
*/
static int fsl_after_dephantomize(fsl_cx * f, fsl_id_t rid, bool doCrosslink){
  int rc = 0;
  unsigned nChildAlloc = 0;
  fsl_id_t * aChild = 0;
  fsl_buffer bufChild = fsl_buffer_empty;
  fsl_db * const db = fsl_cx_db_repo(f);
  fsl_stmt q = fsl_stmt_empty;

  MARKER(("WARNING: fsl_after_dephantomization() is UNTESTED.\n"));
  if(f->cache.ignoreDephantomizations) return 0;
  while(rid){
    unsigned nChildUsed = 0;
    unsigned i = 0;

    /* Parse the object rid itself */
    if(doCrosslink){
      fsl_deck deck = fsl_deck_empty;
      rc = fsl_deck_load_rid(f, &deck, rid, FSL_SATYPE_ANY);
      if(!rc){
        assert(aChild[i]==deck.rid);
        rc = fsl_deck_crosslink(&deck);
      }
      fsl_deck_finalize(&deck);
      if(rc) break;
    }
    /* Parse all delta-manifests that depend on baseline-manifest rid */
    rc = fsl_db_prepare(db, &q,
                        "SELECT rid FROM orphan WHERE baseline=%"FSL_ID_T_PFMT,
                        rid);
    if(rc) break;
    while(FSL_RC_STEP_ROW==fsl_stmt_step(&q)){
      fsl_id_t const child = fsl_stmt_g_id(&q, 0);
      if(nChildUsed>=nChildAlloc){
        nChildAlloc = nChildAlloc ? nChildAlloc*2 : 10;
        rc = fsl_buffer_reserve(&bufChild, sizeof(fsl_id_t)*nChildAlloc);
        if(rc) goto end;
        aChild = (fsl_id_t*)bufChild.mem;
      }
      aChild[nChildUsed++] = child;
    }
    fsl_stmt_finalize(&q);
    for(i=0; i<nChildUsed; ++i){
      fsl_deck deck = fsl_deck_empty;
      rc = fsl_deck_load_rid(f, &deck, aChild[i], FSL_SATYPE_ANY);
      if(!rc){
        assert(aChild[i]==deck.rid);
        rc = fsl_deck_crosslink(&deck);
      }
      fsl_deck_finalize(&deck);
      if(rc) goto end;
    }
    if( nChildUsed ){
      rc = fsl_db_exec_multi(db,
                             "DELETE FROM orphan WHERE baseline=%"FSL_ID_T_PFMT,
                             rid);
      if(rc){
        rc = fsl_cx_uplift_db_error(f, db);
      }
      break;
    }
    /* Recursively dephantomize all artifacts that are derived by
    ** delta from artifact rid and which have not already been
    ** cross-linked.  */
    nChildUsed = 0;
    rc = fsl_db_prepare(db, &q,
                        "SELECT rid FROM delta WHERE srcid=%"FSL_ID_T_PFMT
                        " AND NOT EXISTS(SELECT 1 FROM mlink WHERE mid=delta.rid)",
                        rid);
    if(rc){
      rc = fsl_cx_uplift_db_error(f, db);
      break;
    }
    while( FSL_RC_STEP_ROW==fsl_stmt_step(&q) ){
      fsl_id_t const child = fsl_stmt_g_id(&q, 0);
      if(nChildUsed>=nChildAlloc){
        nChildAlloc = nChildAlloc ? nChildAlloc*2 : 10;
        rc = fsl_buffer_reserve(&bufChild, sizeof(fsl_id_t)*nChildAlloc);
        if(rc) goto end;
        aChild = (fsl_id_t*)bufChild.mem;
      }
      aChild[nChildUsed++] = child;
    }
    fsl_stmt_finalize(&q);
    for(i=1; i<nChildUsed; ++i){
      rc = fsl_after_dephantomize(f, aChild[i], true);
      if(rc) break;
    }
    /* Tail recursion for the common case where only a single artifact
    ** is derived by delta from rid...
    ** (2021-06-06: this libfossil impl is not tail-recursive due to
    ** necessary cleanup) */
    rid = nChildUsed>0 ? aChild[0] : 0;
    doCrosslink = true;
  }
  end:
  fsl_stmt_finalize(&q);
  fsl_buffer_clear(&bufChild);
  return rc;
}

int fsl_content_put_ex( fsl_cx * f, fsl_buffer const * pBlob,
                        fsl_uuid_cstr zUuid,
                        fsl_id_t srcId,
                        fsl_size_t uncompSize,
                        bool isPrivate,
                        fsl_id_t * outRid){
  fsl_size_t size;
  fsl_id_t rid;
  fsl_stmt * s1 = NULL;
  fsl_buffer cmpr = fsl_buffer_empty;
  fsl_buffer hash = fsl_buffer_empty;
  bool markAsUnclustered = false;
  bool markAsUnsent = true;
  bool isDephantomize = false;
  fsl_db * dbR = fsl_cx_db_repo(f);
  int const zUuidLen = zUuid ? fsl_is_uuid(zUuid) : 0;
  int rc = 0;
  bool inTrans = false;
  assert(f);
  assert(dbR);
  assert(pBlob);
  assert(srcId==0 || zUuid!=NULL);
  assert(!zUuid || zUuidLen);
  if(!dbR) return FSL_RC_NOT_A_REPO;
  static const fsl_size_t MaxSize = 0x70000000;
  if(pBlob->used>=MaxSize || uncompSize>=MaxSize){
    /* fossil(1) uses int for all blob sizes, and therefore has a
       hard-coded limit of 2GB max size per blob. That property of the
       API is well-entrenched, and correcting it properly, including
       all algorithms which access blobs using integer indexes, would
       require a large coding effort with a non-trivial risk of
       lingering, difficult-to-trace bugs.

       For compatibility, we limit ourselves to 2GB, but to ensure a
       bit of leeway, we set our limit slightly less than 2GB.
    */
    return fsl_cx_err_set(f, FSL_RC_RANGE,
                          "For compatibility with fossil(1), "
                          "blobs may not exceed %d bytes in size.",
                          (int)MaxSize);
  }
  if(!zUuid){
    assert(0==uncompSize);
    /* "auxiliary hash" bits from:
       https://fossil-scm.org/fossil/file?ci=c965636958eb58aa&name=src%2Fcontent.c&ln=527-537
    */
    /* First check the auxiliary hash to see if there is already an artifact
    ** that uses the auxiliary hash name */
    /* 2021-04-13: we can now use fsl_repo_blob_lookup() to do this,
       but the following code is known to work, so touching it is a
       low priority. */
    rc = fsl_cx_hash_buffer(f, true, pBlob, &hash);
    if(FSL_RC_UNSUPPORTED==rc) rc = 0;
    else if(rc) goto end;
    assert(hash.used==0 || hash.used>=FSL_STRLEN_SHA1);
    rid = hash.used ? fsl_uuid_to_rid(f, fsl_buffer_cstr(&hash)) : 0;
    assert(rid>=0 && "Cannot have malformed/ambiguous UUID at this point.");
    if(!rid){
      /* No existing artifact with the auxiliary hash name.  Therefore, use
      ** the primary hash name. */
      hash.used = 0;
      rc = fsl_cx_hash_buffer(f, false, pBlob, &hash);
      if(rc) goto end;
      assert(hash.used>=FSL_STRLEN_SHA1);
    }
  }else{
    rc = fsl_buffer_append(&hash, zUuid, zUuidLen);
    if(rc) goto end;
  }
  assert(!rc);
  if(uncompSize){
    /* pBlob is assumed to be compressed. */
    assert(fsl_buffer_is_compressed(pBlob));
    size = uncompSize;
  }else{
    size = pBlob->used;
    if(srcId>0){
      rc = fsl_delta_applied_size(pBlob->mem, pBlob->used, &size);
      if(rc) goto end;
    }
  }
  rc = fsl_db_transaction_begin(dbR);
  if(rc) goto end;
  inTrans = true;
  if( f->cxConfig.hashPolicy==FSL_HPOLICY_AUTO && hash.used>FSL_STRLEN_SHA1 ){
    fsl_cx_err_reset(f);
    fsl_cx_hash_policy_set(f, FSL_HPOLICY_SHA3);
    if((rc = f->error.code)){
      goto end;
    }
  }
  /* Check to see if the entry already exists and if it does whether
     or not the entry is a phantom. */
  rc = fsl_db_prepare_cached(dbR, &s1,
                             "SELECT rid, size FROM blob "
                             "WHERE uuid=?"
                             "/*%s()*/",__func__);
  if(rc) goto end;
  rc = fsl_stmt_bind_step( s1, "b", &hash);
  switch(rc){
    case FSL_RC_STEP_ROW:
      rc = 0;
      rid = fsl_stmt_g_id(s1, 0);
      if( fsl_stmt_g_int64(s1, 1)>=0 ){
        /* The entry is not a phantom. There is nothing for us to do
           other than return the RID.
        */
        /*
          Reminder: the do-nothing-for-empty-phantom behaviour is
          arguable (but historical). There is a corner case there
          involving an empty file. So far, so good, though. After
          all...  all empty files have the same hash.
        */
        fsl_stmt_cached_yield(s1);
        assert(inTrans);
        fsl_db_transaction_end(dbR,0);
        if(outRid) *outRid = rid;
        fsl_buffer_clear(&hash);
        return 0;
      }
      break;
    case 0:
      /* No entry with the same UUID currently exists */
      rid = 0;
      markAsUnclustered = true;
      break;
    default:
      goto end;
  }
  if(s1){
    fsl_stmt_cached_yield(s1);
    s1 = NULL;
  }
  if(rc) goto end;

#if 0
  /* Requires app-level data. We might need a client hook mechanism or
     other metadata here.
  */
  /* Construct a received-from ID if we do not already have one */
  if( f->cache.rcvid <= 0 ){
    /* FIXME: use cached statement. */
    rc = fsl_db_exec(dbR, 
       "INSERT INTO rcvfrom(uid, mtime, nonce, ipaddr)"
       "VALUES(%d, julianday('now'), %Q, %Q)",
       g.userUid, g.zNonce, g.zIpAddr
    );
    f->cache.rcvid = fsl_db_last_insert_id(dbR);
  }
#endif

  if( uncompSize ){
    cmpr = *pBlob;
  }else{
    rc = fsl_buffer_compress(pBlob, &cmpr);
    if(rc) goto end;
  }

  if( rid>0 ){
#if 0
    assert(!"NYI: adding data to phantom. Requires some missing pieces.");
    rc = fsl_cx_err_set(f, FSL_RC_NYI,
                        "NYI: adding data to phantom. "
                        "Requires missing rcvId pieces.");
    goto end;
#else
    /* We are just adding data to a phantom */
    rc = fsl_db_prepare_cached(dbR, &s1,
                               "UPDATE blob SET "
                               "rcvid=?, size=?, content=? "
                               "WHERE rid=?"
                               "/*%s()*/",__func__);
    if(rc) goto end;
    rc = fsl_stmt_bind_step(s1, "RIBR", f->cache.rcvId, (int64_t)size,
                            &cmpr, rid);
    if(!rc){
      rc = fsl_db_exec(dbR, "DELETE FROM phantom "
                       "WHERE rid=%"FSL_ID_T_PFMT, rid
                       /* FIXME? use cached statement? */);
      if( !rc && (srcId==0 ||
                  0==fsl_acache_check_available(f, srcId)) ){
        isDephantomize = true;
        rc = fsl_content_mark_available(f, rid);
      }
    }
    fsl_stmt_cached_yield(s1);
    s1 = NULL;
    if(rc) goto end;
#endif
  }else{
    /* We are creating a new entry */
    rc = fsl_db_prepare_cached(dbR, &s1,
                               "INSERT INTO blob "
                               "(rcvid,size,uuid,content) "
                               "VALUES(?,?,?,?)"
                               "/*%s()*/",__func__);
    if(rc) goto end;
    rc = fsl_stmt_bind_step(s1, "RIbB", f->cache.rcvId, (int64_t)size,
                            &hash, &cmpr);
    if(!rc){
      rid = fsl_db_last_insert_id(dbR);
      if(!pBlob ){
        rc = fsl_db_exec_multi(dbR,/* FIXME? use cached statement? */
                               "INSERT OR IGNORE INTO phantom "
                               "VALUES(%"FSL_ID_T_PFMT")",
                               rid);
        markAsUnsent = false;
      }
      if( !rc && (f->cache.markPrivate || isPrivate) ){
        rc = fsl_db_exec_multi(dbR,/* FIXME? use cached statement? */
                               "INSERT INTO private "
                               "VALUES(%"FSL_ID_T_PFMT")",
                               rid);
        markAsUnclustered = false;
        markAsUnsent = false;
      }
    }
    if(rc) rc = fsl_cx_uplift_db_error2(f, dbR, rc);
    fsl_stmt_cached_yield(s1);
    s1 = NULL;
    if(rc) goto end;
  }

  /* If the srcId is specified, then the data we just added is
     really a delta. Record this fact in the delta table.
  */
  if( srcId ){
    rc = fsl_db_prepare_cached(dbR, &s1,
                               "REPLACE INTO delta(rid,srcid) "
                               "VALUES(?,?)"
                               "/*%s()*/",__func__);
    if(!rc){
      rc = fsl_stmt_bind_step(s1, "RR", rid, srcId);
      if(rc) rc = fsl_cx_uplift_db_error2(f, dbR, rc);
      fsl_stmt_cached_yield(s1);
      s1 = NULL;
    }
    if(rc) goto end;
  }
  if( !isDephantomize
      && fsl_id_bag_contains(&f->cache.arty.missing, rid) && 
      (srcId==0 || (0==fsl_acache_check_available(f,srcId)))){
    /*
      TODO: document what this is for.
      TODO: figure out what that is.
    */
    rc = fsl_content_mark_available(f, rid);
    if(rc) goto end;
  }
  if( isDephantomize ){
    rc = fsl_after_dephantomize(f, rid, false);
    if(rc) goto end;
  }

  /* Add the element to the unclustered table if has never been
     previously seen.
  */
  if( markAsUnclustered ){
    /* FIXME: use a cached statement. */
    rc = fsl_db_exec_multi(dbR,
                           "INSERT OR IGNORE INTO unclustered VALUES"
                           "(%"FSL_ID_T_PFMT")", rid);
    if(rc) goto end;
  }

  if( markAsUnsent ){
    /* FIXME: use a cached statement. */
    rc = fsl_db_exec(dbR, "INSERT OR IGNORE INTO unsent "
                     "VALUES(%"FSL_ID_T_PFMT")", rid);
    if(rc) goto end;
  }
  
  rc = fsl_repo_verify_before_commit(f, rid);
  if(rc) goto end /* FSL_RC_OOM is basically the "only possible" failure
                     after this point. */;
  /* Code after end: relies on the following 2 lines: */
  rc = fsl_db_transaction_end(dbR, false);
  inTrans = false;
  if(!rc){
    if(outRid) *outRid = rid;
  }
  end:
  if(inTrans){
    assert(0!=rc);
    fsl_db_transaction_end(dbR,true);
  }
  fsl_buffer_clear(&hash);
  if(!uncompSize){
    fsl_buffer_clear(&cmpr);
  }/* else cmpr.mem (if any) belongs to pBlob */
  return rc;
}

int fsl_content_put( fsl_cx * f, fsl_buffer const * pBlob, fsl_id_t * newRid){
  return fsl_content_put_ex(f, pBlob, NULL, 0, 0, 0, newRid);
}

int fsl_uuid_is_shunned(fsl_cx * f, fsl_uuid_cstr zUuid){
  fsl_db * db = fsl_cx_db_repo(f);
  if( !db || zUuid==0 || zUuid[0]==0 ) return 0;
  else if(FSL_HPOLICY_SHUN_SHA1==f->cxConfig.hashPolicy
          && FSL_STRLEN_SHA1==fsl_is_uuid(zUuid)){
    return 1;
  }
  /* TODO? cached query */
  return 1==fsl_db_g_int32( db, 0,
                            "SELECT 1 FROM shun WHERE uuid=%Q",
                            zUuid);
}

int fsl_content_new( fsl_cx * f, fsl_uuid_cstr uuid, bool isPrivate,
                     fsl_id_t * newId ){
  fsl_id_t rid = 0;
  int rc;
  fsl_db * db = fsl_cx_db_repo(f);
  fsl_stmt * s1 = NULL, * s2 = NULL;
  int const uuidLen = uuid ? fsl_is_uuid(uuid) : 0;
  if(!f || !uuid) return FSL_RC_MISUSE;
  else if(!uuidLen) return FSL_RC_RANGE;
  if(!db) return FSL_RC_NOT_A_REPO;
  if( fsl_uuid_is_shunned(f, uuid) ){
    return fsl_cx_err_set(f, FSL_RC_ACCESS,
                          "UUID is shunned: %s", uuid)
      /* need new error code? */;
  }
  rc = fsl_db_transaction_begin(db);
  if(rc) return rc;

  rc = fsl_db_prepare_cached(db, &s1,
                             "INSERT INTO blob(rcvid,size,uuid,content)"
                             "VALUES(0,-1,?,NULL)"
                             "/*%s()*/",__func__);
  if(rc) goto end;
  rc = fsl_stmt_bind_text(s1, 1, uuid, uuidLen, 0);
  if(!rc) rc = fsl_stmt_step(s1);
  fsl_stmt_cached_yield(s1);
  if(FSL_RC_STEP_DONE!=rc) goto end;
  else rc = 0;
  rid = fsl_db_last_insert_id(db);
  assert(rid>0);
  rc = fsl_db_prepare_cached(db, &s2,
                             "INSERT INTO phantom VALUES (?)"
                             "/*%s()*/",__func__);
  if(rc) goto end;
  rc = fsl_stmt_bind_id(s2, 1, rid);
  if(!rc) rc = fsl_stmt_step(s2);
  fsl_stmt_cached_yield(s2);
  if(FSL_RC_STEP_DONE!=rc) goto end;
  else rc = 0;

  if( f->cache.markPrivate || isPrivate ){
    /* Should be seldom enough that we don't need to cache
       this statement. */
    rc = fsl_db_exec(db,
                     "INSERT INTO private VALUES(%"FSL_ID_T_PFMT")",
                     (fsl_id_t)rid);
  }else{
    fsl_stmt * s3 = NULL;
    rc = fsl_db_prepare_cached(db, &s3,
                               "INSERT INTO unclustered VALUES(?)");
    if(!rc){
      rc = fsl_stmt_bind_id(s3, 1, rid);
      if(!rc) rc = fsl_stmt_step(s3);
      fsl_stmt_cached_yield(s3);
      if(FSL_RC_STEP_DONE!=rc) goto end;
      else rc = 0;
    }
  }

  if(!rc) rc = fsl_id_bag_insert(&f->cache.arty.missing, rid);
  
  end:
  if(rc){
    if(db->error.code && !f->error.code){
      fsl_cx_uplift_db_error(f, db);
    }
    fsl_db_transaction_rollback(db);
  }
  else{
    rc = fsl_db_transaction_commit(db);
    if(!rc && newId) *newId = rid;
    else if(rc && !f->error.code){
      fsl_cx_uplift_db_error(f, db);
    }
  }
  return rc;
}

int fsl_content_undeltify(fsl_cx * f, fsl_id_t rid){
  int rc;
  fsl_db * db = f ? fsl_cx_db_repo(f) : NULL;
  fsl_id_t srcid = 0;
  fsl_buffer x = fsl_buffer_empty;
  fsl_stmt s = fsl_stmt_empty;
  if(!f) return FSL_RC_MISUSE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  else if(rid<=0) return FSL_RC_RANGE;
  rc = fsl_db_transaction_begin(db);
  if(rc) return fsl_cx_uplift_db_error2(f, db, rc);
  /* Reminder: the original impl does not do this in a
     transaction, _possibly_ because it's only done from places
     where a transaction is active (that's unconfirmed).
     Nested transactions are very cheap, though.
  */
  rc = fsl_delta_src_id( f, rid, &srcid );
  if(rc || srcid<=0) goto end;
  rc = fsl_content_get(f, rid, &x);
  if( rc || !x.used ) goto end;
  /* TODO? use cached statements */
  rc = fsl_db_prepare(db, &s,
                      "UPDATE blob SET content=?,"
                      " size=%" FSL_SIZE_T_PFMT
                      " WHERE rid=%" FSL_ID_T_PFMT,
                      x.used, rid);
  if(rc) goto dberr;
  rc = fsl_buffer_compress(&x, &x);
  if(rc) goto end;
  rc = fsl_stmt_bind_blob(&s, 1, x.mem,
                          (fsl_int_t)x.used, 0);
  if(rc) goto dberr;
  rc = fsl_stmt_step(&s);
  if(FSL_RC_STEP_DONE==rc) rc = 0;
  else goto dberr;
  rc = fsl_db_exec(db, "DELETE FROM delta "
                   "WHERE rid=%"FSL_ID_T_PFMT,
                   (fsl_id_t)rid);
  if(rc) goto dberr;
#if 0
  /*
    fossil does not do this, but that seems like an inconsistency.

    On that topic Richard says:

    "When you undelta an artifact, however, it is then stored as
    plain text.  (Actually, as zlib compressed plain text.)  There
    is no possibility of delta loops or bugs in the delta encoder or
    missing source artifacts.  And so there is much less of a chance
    of losing content.  Hence, I didn't see the need to verify the
    content of artifacts that are undelta-ed."

    Potential TODO: f->flags FSL_CX_F_PEDANTIC_VERIFICATION, which
    enables the R-card and this check, and any similarly superfluous
    ones.
  */
  if(!rc) fsl_repo_verify_before_commit(f, rid);
#endif
  end:
  fsl_buffer_clear(&x);
  fsl_stmt_finalize(&s);
  if(rc) fsl_db_transaction_rollback(db);
  else rc = fsl_db_transaction_commit(db);
  return rc;
  dberr:
  assert(rc);
  rc = fsl_cx_uplift_db_error2(f, db, rc);
  goto end;
}

int fsl_content_deltify(fsl_cx * f, fsl_id_t rid,
                        fsl_id_t srcid, bool force){
  fsl_id_t s;
  fsl_buffer data = fsl_buffer_empty;
  fsl_buffer src = fsl_buffer_empty;
  fsl_buffer delta = fsl_buffer_empty;
  fsl_db * db = f ? fsl_cx_db_repo(f) : NULL;
  int rc = 0;
  enum { MinSizeThreshold = 50 };
  if(!f) return FSL_RC_MISUSE;
  else if(rid<=0 || srcid<=0) return FSL_RC_RANGE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  else if( srcid==rid ) return 0;
  else if(!fsl_content_is_available(f, rid)){
    return 0;
  }
  if(!force){
    fsl_id_t tmpRid = 0;
    rc = fsl_delta_src_id(f, rid, &tmpRid);
    if(tmpRid>0){
      /*
        We already have a delta, it seems. Nothing left to do
        :-D. Should we return FSL_RC_ALREADY_EXISTS here?
      */
      return 0;
    }
    else if(rc) return rc;
  }

  if( fsl_content_is_private(f, srcid)
      && !fsl_content_is_private(f, rid) ){
    /*
      See API doc comments about crossing the private/public
      boundaries. Do we want to report okay here or
      FSL_RC_ACCESS? Not yet sure how this routine is used.

      Since delitifying is an internal optimization/implementation
      detail, it seems best to return 0 for this case.
    */
    return 0;
  }
  /**
     Undeltify srcid if needed...
  */
  s = srcid;
  while( (0==(rc=fsl_delta_src_id(f, s, &s)))
         && (s>0) ){
    if( s==rid ){
      rc = fsl_content_undeltify(f, srcid);
      break;
    }
  }
  if(rc) return rc;
  /* As of here, don't return on error. Use (goto end) instead, or be
     really careful, b/c buffers might need cleaning. */
  rc = fsl_content_get(f, srcid, &src);
  if(rc
     || (src.used < MinSizeThreshold)
     /* See API doc comments about minimum size to delta/undelta. */
     ) goto end;
  rc = fsl_content_get(f, rid, &data);
  if(rc || (data.used < MinSizeThreshold)) goto end;
  rc = fsl_buffer_delta_create(&src, &data, &delta);
  if( !rc && (delta.used <= (data.used * 3 / 4 /* 75% */))){
    fsl_stmt * s1 = NULL;
    fsl_stmt * s2 = NULL;
    rc = fsl_buffer_compress(&delta, &delta);
    if(rc) goto end;
    rc = fsl_db_prepare_cached(db, &s1,
                               "UPDATE blob SET content=? "
                               "WHERE rid=?/*%s()*/",__func__);
    if(!rc){
      fsl_stmt_bind_id(s1, 2, rid);
      rc = fsl_stmt_bind_blob(s1, 1, delta.mem, delta.used, 0);
      if(!rc){
        rc = fsl_db_prepare_cached(db, &s2,
                                   "REPLACE INTO delta(rid,srcid) "
                                   "VALUES(?,?)/*%s()*/",__func__);
        if(!rc){
          fsl_stmt_bind_id(s2, 1, rid);
          fsl_stmt_bind_id(s2, 2, srcid);
          rc = fsl_db_transaction_begin(db);
          if(!rc){
            rc = fsl_stmt_step(s1);
            if(FSL_RC_STEP_DONE==rc){
              rc = fsl_stmt_step(s2);
              if(FSL_RC_STEP_DONE==rc) rc = 0;
            }
            if(!rc) rc = fsl_db_transaction_end(db, 0);
            else fsl_db_transaction_end(db, 1) /* keep rc intact */;
          }
        }
      }
    }
    fsl_stmt_cached_yield(s1);
    fsl_stmt_cached_yield(s2);
    if(!rc) fsl_repo_verify_before_commit(f, rid);
  }
  end:
  if(rc && db->error.code && !f->error.code){
    fsl_cx_uplift_db_error(f,db);
  }
  fsl_buffer_clear(&src);
  fsl_buffer_clear(&data);
  fsl_buffer_clear(&delta);
  return rc;
}

/**
    Removes all entries from the repo's blob table which are listed in
    the shun table.
 */
int fsl_repo_shun_artifacts(fsl_cx * f){
  fsl_stmt q = fsl_stmt_empty;
  int rc;
  fsl_db * db = f ? fsl_cx_db_repo(f) : NULL;
  if(!f) return FSL_RC_MISUSE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  rc = fsl_db_transaction_begin(db);
  if(rc) return rc;
  rc = fsl_db_exec_multi(db,
                         "CREATE TEMP TABLE IF NOT EXISTS "
                         "toshun(rid INTEGER PRIMARY KEY);"
                         "INSERT INTO toshun SELECT rid FROM blob, shun "
                         "WHERE blob.uuid=shun.uuid;"
  );
  if(rc) goto end;
  /* Ensure that deltas generated from the to-be-shunned data
     are unpacked into non-delta form...
  */
  rc = fsl_db_prepare(db, &q,
                      "SELECT rid FROM delta WHERE srcid IN toshun"
                      );
  if(rc) goto end;
  while( !rc && (FSL_RC_STEP_ROW==fsl_stmt_step(&q)) ){
    fsl_id_t const srcid = fsl_stmt_g_id(&q, 0);
    rc = fsl_content_undeltify(f, srcid);
  }
  fsl_stmt_finalize(&q);
  if(!rc){
    rc = fsl_db_exec_multi(db,
            "DELETE FROM delta WHERE rid IN toshun;"
            "DELETE FROM blob WHERE rid IN toshun;"
            "DROP TABLE toshun;"
            "DELETE FROM private "
            "WHERE NOT EXISTS "
            "(SELECT 1 FROM blob WHERE rid=private.rid);"
    );
  }
  end:
  if(!rc) rc = fsl_db_transaction_commit(db);
  else fsl_db_transaction_rollback(db);
  if(rc && db->error.code && !f->error.code){
    rc = fsl_cx_uplift_db_error(f, db);
  }
  return rc;
}

int fsl_content_make_public(fsl_cx * f, fsl_id_t rid){
  int rc;
  fsl_db * db = f ? fsl_cx_db_repo(f) : NULL;
  if(!f) return FSL_RC_MISUSE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  rc = fsl_db_exec(db, "DELETE FROM private "
                   "WHERE rid=%" FSL_ID_T_PFMT, rid);
  return rc ? fsl_cx_uplift_db_error(f, db) : 0;
}

/**
    Load the record ID rid and up to N-1 closest ancestors into
    the "fsl_computed_ancestors" table.
 */
static int fsl_compute_ancestors( fsl_db * db, fsl_id_t rid,
                                  int N, char directOnly ){
  fsl_stmt st = fsl_stmt_empty;
  int rc = fsl_db_prepare(db, &st,
    "WITH RECURSIVE "
    "  ancestor(rid, mtime) AS ("
    "    SELECT ?, mtime "
    "      FROM event WHERE objid=? "
    "    UNION "
    "    SELECT plink.pid, event.mtime"
    "      FROM ancestor, plink, event"
    "     WHERE plink.cid=ancestor.rid"
    "       AND event.objid=plink.pid %s"
    "     ORDER BY mtime DESC LIMIT ?"
    "  )"
    "INSERT INTO fsl_computed_ancestors"
    "  SELECT rid FROM ancestor;",
    directOnly ? "AND plink.isPrim" : ""
  );
  if(!rc){
    fsl_stmt_bind_id(&st, 1, rid);
    fsl_stmt_bind_id(&st, 2, rid);
    fsl_stmt_bind_int32(&st, 3, (int32_t)N);
    rc = fsl_stmt_step(&st);
    if(FSL_RC_STEP_DONE==rc){
      rc = 0;
    }
  }
  fsl_stmt_finalize(&st);
  return rc;
}

int fsl_mtime_of_F_card(fsl_cx * f, fsl_id_t vid, fsl_card_F const * fc, fsl_time_t *pMTime){
  if(!f || !fc) return FSL_RC_MISUSE;
  else if(vid<=0) return FSL_RC_RANGE;
  else if(!fc->uuid){
    if(pMTime) *pMTime = 0;
    return 0;
  }else{
    fsl_id_t fid = fsl_uuid_to_rid(f, fc->uuid);
    if(fid<=0){
      assert(f->error.code);
      return f->error.code;
    }else{
      return fsl_mtime_of_manifest_file(f, vid, fid, pMTime);
    }
  }
}

int fsl_mtime_of_manifest_file(fsl_cx * f, fsl_id_t vid, fsl_id_t fid, fsl_time_t *pMTime){
  fsl_db * db = fsl_needs_repo(f);
  fsl_stmt * q = NULL;
  int rc;
  if(!db) return FSL_RC_NOT_A_REPO;

  if(fid<=0){
    /* Only fetch the checkin time... */
    int64_t i = -1;
    rc = fsl_db_get_int64(db, &i, 
                          "SELECT (mtime-2440587.5)*86400 "
                          "FROM event WHERE objid=%"FSL_ID_T_PFMT
                          " AND type='ci'",
                          (fsl_id_t)vid);
    if(!rc){
      if(i<0) rc = FSL_RC_NOT_FOUND;
      else if(pMTime) *pMTime = (fsl_time_t)i;
    }
    return rc;
  }

  if( f->cache.mtimeManifest != vid ){
    /*
      Computing (and keeping) ancestors is relatively costly, so we
      keep only the copy associated with f->cache.mtimeManifest
      around. For the general case, we will be feeding this function
      files from the same manifest.
    */
    f->cache.mtimeManifest = vid;
    rc = fsl_db_exec_multi(db,"DROP TABLE IF EXISTS temp.fsl_computed_ancestors;"
                           "CREATE TEMP TABLE fsl_computed_ancestors"
                           "(x INTEGER PRIMARY KEY);");
    if(!rc){
      rc = fsl_compute_ancestors(db, vid, 1000000, 1);
    }
    if(rc){
      fsl_cx_uplift_db_error(f, db);
      return rc;
    }
  }
  rc = fsl_db_prepare_cached(db, &q,
    "SELECT (max(event.mtime)-2440587.5)*86400 FROM mlink, event"
    " WHERE mlink.mid=event.objid"
    "   AND mlink.fid=?"
    "   AND +mlink.mid IN fsl_computed_ancestors"
  );
  if(!rc){
    fsl_stmt_bind_id(q, 1, fid);
    rc = fsl_stmt_step(q);
    if( FSL_RC_STEP_ROW==rc ){
      rc = 0;
      if(pMTime) *pMTime = (fsl_time_t)fsl_stmt_g_int64(q, 0);
    }else{
      assert(rc);
      if(FSL_RC_STEP_DONE==rc) rc = FSL_RC_NOT_FOUND;
    }
    fsl_stmt_cached_yield(q);
  }
  return rc;
}

int fsl_card_F_content( fsl_cx * f, fsl_card_F const * fc,
                        fsl_buffer * dest ){
  if(!f || !fc || !dest) return FSL_RC_MISUSE;
  else if(!fc->uuid){
    return fsl_cx_err_set(f, FSL_RC_RANGE,
                          "Cannot fetch content of a deleted file "
                          "because it has no UUID.");
  }
  else if(!fsl_needs_repo(f)) return FSL_RC_NOT_A_REPO;
  else{
    fsl_id_t const rid = fsl_uuid_to_rid(f, fc->uuid);
    if(!rid) return fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                                   "UUID not found: %s",
                                   fc->uuid);
    else if(rid<0){
      assert(f->error.code);
      return f->error.code;
    }else{
      return fsl_content_get(f, rid, dest);
    }
  }
}


/**
   UNTESTED (but closely derived from known-working code).

   Expects f to have an opened checkout. Assumes zName is resolvable
   (via fsl_ckout_filename_check() - see that function for the
   meaning of the relativeToCwd argument) to a path under the current
   checkout root. It loads the file's contents and stores them into
   the blob table. If rid is not NULL, *rid is assigned the blob.rid
   (possibly new, possilbly re-used!). If uuid is not NULL then *uuid
   is assigned to the content's UUID. The *uuid bytes are owned by the
   caller, who must eventually fsl_free() them. If content with the
   same UUID already exists, it does not get re-imported but rid/uuid
   will (if not NULL) contain the values of any previous content
   with the same hash.

   ACHTUNG: this function DOES NOT CARE whether or not the file is
   actually part of a checkout or not, nor whether it is actually
   referenced by any checkins, or such, other than that it must
   resolve to something under the checkout root (to avoid breaking any
   internal assumptions in fossil about filenames). It will add new
   repo.filename entries as needed for this function. Thus is can be
   used to import "shadow files" either not known about by fossil or
   not _yet_ known about by fossil.

   If parentRid is >0 then it must refer to the previous version of
   zName's content. The parent version gets deltified vs the new one,
   but deltification is a suggestion which the library will ignore if
   (e.g.) the parent content is already a delta of something else.

   This function does its DB-side work in a transaction, so, e.g.  if
   saving succeeds but deltification of the parent version fails for
   some reason, the whole save operation is rolled back.

   Returns 0 on success. On error rid and uuid are not modified.
*/
int fsl_import_file( fsl_cx * f, char relativeToCwd,
                     char const * zName,
                     fsl_id_t parentRid,
                     fsl_id_t *rid, fsl_uuid_str * uuid ){
  fsl_buffer * canon = 0; // canonicalized filename
  fsl_buffer * nbuf = 0; // filename buffer
  fsl_buffer * fbuf = &f->fileContent; // file content buffer
  char const * fn;
  int rc;
  fsl_id_t fnid = 0;
  fsl_id_t rcRid = 0;
  fsl_db * db = f ? fsl_needs_repo(f) : NULL;
  char inTrans = 0;
  if(!zName || !*zName) return FSL_RC_MISUSE;
  else if(!f->ckout.dir) return FSL_RC_NOT_A_CKOUT;
  else if(!db) return FSL_RC_NOT_A_REPO;
  canon = fsl_cx_scratchpad(f);
  nbuf = fsl_cx_scratchpad(f);

  assert(!fbuf->used && "Misuse of f->fileContent");
  assert(f->ckout.dir);

  /* Normalize the name... i often regret having
     fsl_ckout_filename_check() return checkout-relative paths.
  */
  rc = fsl_ckout_filename_check(f, relativeToCwd, zName, canon);
  if(rc) goto end;

  /* Find or create a repo.filename entry... */
  fn = fsl_buffer_cstr(canon);

  rc = fsl_db_transaction_begin(db);
  if(rc) goto end;
  inTrans = 1;

  rc = fsl_repo_filename_fnid2(f, fn, &fnid, 1);
  if(rc) goto end;

  /* Import the file... */
  assert(fnid>0);
  rc = fsl_buffer_appendf(nbuf, "%s%s", f->ckout.dir, fn);
  if(rc) goto end;
  fn = fsl_buffer_cstr(nbuf);
  rc = fsl_buffer_fill_from_filename( fbuf, fn );
  if(rc){
    fsl_cx_err_set(f, rc, "Error %s importing file: %s",
                   fsl_rc_cstr(rc), fn);
    goto end;
  }
  fn = NULL;
  rc = fsl_content_put( f, fbuf, &rcRid );
  if(!rc){
    assert(rcRid > 0);
    if(parentRid>0){
      /* Make parent version a delta of this one, if possible... */
      rc = fsl_content_deltify(f, parentRid, rcRid, 0);
    }
    if(!rc){
      if(rid) *rid = rcRid;
      if(uuid){
        fsl_cx_err_reset(f);
        *uuid = fsl_rid_to_uuid(f, rcRid);
        if(!*uuid) rc = (f->error.code ? f->error.code : FSL_RC_OOM);
      }
    }
  }

  if(!rc){
    assert(inTrans);
    inTrans = 0;
    rc = fsl_db_transaction_commit(db);
  }

  end:
  fsl_cx_content_buffer_yield(f);
  assert(0==fbuf->used);
  fsl_cx_scratchpad_yield(f, canon);
  fsl_cx_scratchpad_yield(f, nbuf);
  if(inTrans) fsl_db_transaction_rollback(db);
  return rc;
}

fsl_hash_types_e fsl_validate_hash(const char *zHash, int nHash){
  /* fossil(1) counterpart: hname_validate() */
  fsl_hash_types_e rc;
  switch(nHash){
    case FSL_STRLEN_SHA1: rc = FSL_HTYPE_SHA1; break;
    case FSL_STRLEN_K256: rc = FSL_HTYPE_K256; break;
    default: return FSL_HTYPE_ERROR;
  }
  return fsl_validate16(zHash, (fsl_size_t)nHash) ? rc : FSL_HTYPE_ERROR;
}

const char * fsl_hash_type_name(fsl_hash_types_e h, const char *zUnknown){
  /* fossil(1) counterpart: hname_alg() */
  switch(h){
    case FSL_HTYPE_SHA1: return "SHA1";
    case FSL_HTYPE_K256: return "SHA3-256";
    default: return zUnknown;
  }
}

fsl_hash_types_e fsl_verify_blob_hash(fsl_buffer const * pIn,
                                      const char *zHash, int nHash){
  fsl_hash_types_e id = FSL_HTYPE_ERROR;
  switch(nHash){
    case FSL_STRLEN_SHA1:{
      fsl_sha1_cx cx;
      char hex[FSL_STRLEN_SHA1+1] = {0};
      fsl_sha1_init(&cx);
      fsl_sha1_update(&cx, pIn->mem, (unsigned)pIn->used);
      fsl_sha1_final_hex(&cx, hex);
      if(0==memcmp(hex, zHash, FSL_STRLEN_SHA1)){
        id = FSL_HTYPE_SHA1;
      }
      break;
    }
    case FSL_STRLEN_K256:{
      fsl_sha3_cx cx;
      unsigned char const * hex;
      fsl_sha3_init(&cx);
      fsl_sha3_update(&cx, pIn->mem, (unsigned)pIn->used);
      hex = fsl_sha3_end(&cx);
      if(0==memcmp(hex, zHash, FSL_STRLEN_K256)){
        id = FSL_HTYPE_K256;
      }
      break;
    }
    default:
      break;
  }
  return id;
}
#undef MARKER
