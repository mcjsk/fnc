/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
/*
  Copyright 2013-2021 Stephan Beal (https://wanderinghorse.net).


  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the Simplified BSD License (also
  known as the "2-Clause License" or "FreeBSD License".)
  
  This program is distributed in the hope that it will be useful,
  but without any warranty; without even the implied warranty of
  merchantability or fitness for a particular purpose.
  
  *****************************************************************************
  This file houses the manifest/control-artifact-related APIs.
*/
#include "fossil-scm/fossil-internal.h"
#include "fossil-scm/fossil-hash.h"
#include "fossil-scm/fossil-forum.h"
#include "fossil-scm/fossil-confdb.h"
#include <assert.h>
#include <stdlib.h> /* qsort() */
#include <memory.h> /* memcmp() */

/* Only for debugging */
#define MARKER(pfexp)                                               \
  do{ printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__);   \
    printf pfexp;                                                   \
  } while(0)

typedef int StaticAssertMCacheArraySizes[
 ((sizeof(fsl_mcache_empty.aAge)
  /sizeof(fsl_mcache_empty.aAge[0]))
 == (sizeof(fsl_mcache_empty.decks)
     /sizeof(fsl_mcache_empty.decks[0])))
 ? 1 : -1
];

enum fsl_card_F_list_flags_e {
FSL_CARD_F_LIST_NEEDS_SORT = 0x01
};

/**
   Transfers the contents of d into f->cache.mcache. If d is
   dynamically allocated then it is also freed. In any case, after
   calling this the caller must behave as if the deck had been passed
   to fsl_deck_finalize().

   If manifest caching is disabled for f, d is immediately finalized.
*/
static void fsl_cx_mcache_insert(fsl_cx *f, fsl_deck * d){
  if(!(f->flags & FSL_CX_F_MANIFEST_CACHE)){
    fsl_deck_finalize(d);
    return;
  }
  const unsigned cacheLen =
    (unsigned)(sizeof(fsl_mcache_empty.aAge)
               /sizeof(fsl_mcache_empty.aAge[0]));
  fsl_mcache * const mc = &f->cache.mcache;
  while( d ){
    unsigned i;
    fsl_deck *pBaseline = d->B.baseline;
    d->B.baseline = 0;
    for(i=0; i<cacheLen; ++i){
      if( !mc->decks[i].rid ) break;
    }
    if( i>=cacheLen ){
      unsigned oldest = 0;
      unsigned oldestAge = mc->aAge[0];
      for(i=1; i<cacheLen; ++i){
        if( mc->aAge[i]<oldestAge ){
          oldest = i;
          oldestAge = mc->aAge[i];
        }
      }
      fsl_deck_finalize(&mc->decks[oldest]);
      i = oldest;
    }
    mc->aAge[i] = ++mc->nextAge;
    mc->decks[i] = *d;
    *d = fsl_deck_empty;
    if(&fsl_deck_empty == mc->decks[i].allocStamp){
      /* d was fsl_deck_malloc()'d so we need to free it, but cannot
         send it through fsl_deck_finalize() because that would try to
         clean up the memory we just transferred ownership of to
         mc->decks[i]. So... */
      mc->decks[i].allocStamp = 0;
      fsl_free(d);
    }
    d = pBaseline;
  }
}

/**
   Searches f->cache.mcache for a deck with the given RID. If found,
   it is bitwise copied over tgt, that entry is removed from the
   cache, and true is returned. If no match is found, tgt is not
   modified and false is returned.

   If manifest caching is disabled for f, false is immediately
   returned without causing side effects.
*/
static bool fsl_cx_mcache_search(fsl_cx * f, fsl_id_t rid, fsl_deck * tgt){
  if(!(f->flags & FSL_CX_F_MANIFEST_CACHE)) return false;
  const unsigned cacheLen =
    (int)(sizeof(fsl_mcache_empty.aAge)
          /sizeof(fsl_mcache_empty.aAge[0]));
  unsigned i;
  assert(cacheLen ==
         (unsigned)(sizeof(fsl_mcache_empty.decks)
                    /sizeof(fsl_mcache_empty.decks[0])));
  for(i=0; i<cacheLen; ++i){
    if( f->cache.mcache.decks[i].rid==rid ){
      *tgt = f->cache.mcache.decks[i];
      f->cache.mcache.decks[i] = fsl_deck_empty;
      ++f->cache.mcache.hits;
      return true;
    }
  }
  ++f->cache.mcache.misses;
  return false;
}

/**
   If mem is NULL or inside d->content.mem then this function does
   nothing, else it passes mem to fsl_free(). Intended to be used to
   clean up d->XXX string members (or sub-members) which have been
   optimized away via d->content.
*/
static void fsl_deck_free_string(fsl_deck * d, char * mem){
  assert(d);
  if(mem
     && (!d->content.used
         || !(((unsigned char const *)mem >=d->content.mem)
              &&
              ((unsigned char const *)mem < (d->content.mem+d->content.capacity)))
         )){
    fsl_free(mem);
  }/* else do nothing - the memory is NULL or owned by d->content. */
}

/**
   fsl_list_visitor_f() impl which frees fsl_list-of-(char*) card entries
   in ((fsl_deck*)visitorState).
*/
static int fsl_list_v_card_string_free(void * mCard, void * visitorState ){
  fsl_deck_free_string( (fsl_deck*)visitorState, mCard );
  return 0;
}

/** Evals to a pointer to the F-card at the given index
    in the given fsl_card_F_list pointer. Each arg is
    evaluated only once. */
#define F_at(LISTP,NDX) (&(LISTP)->list[NDX])

static int fsl_card_F_list_reserve2( fsl_card_F_list * li ){
  return (li->used<li->capacity)
    ? 0
    : fsl_card_F_list_reserve(li, li->capacity
                               ? li->capacity*4/3+1
                               : 50);
}

static void fsl_card_F_clean( fsl_card_F * f ){
  if(!f->deckOwnsStrings){
    fsl_free(f->name);
    fsl_free(f->uuid);
    fsl_free(f->priorName);
  }
  *f = fsl_card_F_empty;
}

/**
   Cleans up the F-card at li->list[ndx] and shifts all F-cards to its
   right one entry to the left.
*/
static void fsl_card_F_list_remove(fsl_card_F_list * li,
                                   uint32_t ndx){
  uint32_t i;
  assert(li->used);
  assert(ndx<li->used);
  fsl_card_F_clean(F_at(li,ndx));
  for( i = ndx; i < li->used - 1; ++i ){
    li->list[i] = li->list[i+1];
  }
  li->list[li->used] = fsl_card_F_empty;
  --li->used;
}

void fsl_card_F_list_finalize( fsl_card_F_list * li ){
  uint32_t i;
  for(i=0; i < li->used; ++i){
    fsl_card_F_clean(F_at(li,i));  
  }
  li->used = li->capacity = 0;
  fsl_free(li->list);
  *li = fsl_card_F_list_empty;
}

int fsl_card_F_list_reserve( fsl_card_F_list * li, uint32_t n ){
  if(li->capacity>=n) return 0;
  else if(n==0){
    fsl_card_F_list_finalize(li);
    return 0;
  }else{
    fsl_card_F * re = fsl_realloc(li->list, n * sizeof(fsl_card_F));
    if(re){
      li->list = re;
      li->capacity = n;
    }
    return re ? 0 : FSL_RC_OOM;
  }
}

/**
   Adjusts the end of the give list by +1, reserving more space if
   needed, and returns the next available F-card in a cleanly-wiped
   state. Returns NULL on alloc error.
*/
static fsl_card_F * fsl_card_F_list_push( fsl_card_F_list * li ){
  if(li->used==li->capacity && fsl_card_F_list_reserve2(li)) return NULL;
  li->list[li->used] = fsl_card_F_empty;
  if(li->used){
    li->flags |= FSL_CARD_F_LIST_NEEDS_SORT/*pessimistic assumption*/;
  }
  return &li->list[li->used++];
}
/**
   Chops the last entry off of the given list, freeing any resources
   owned by that entry. Decrements li->used. Asserts that li->used is
   positive.
*/
static void fsl_card_F_list_pop( fsl_card_F_list * li ){
  assert(li->used);
  if(li->used) fsl_card_F_clean(F_at(li, --li->used));
}

fsl_card_Q * fsl_card_Q_malloc(fsl_cherrypick_type_e type,
                               fsl_uuid_cstr target,
                               fsl_uuid_cstr baseline){
  int const targetLen = target ? fsl_is_uuid(target) : 0;
  int const baselineLen = baseline ? fsl_is_uuid(baseline) : 0;
  if((type!=FSL_CHERRYPICK_ADD && type!=FSL_CHERRYPICK_BACKOUT)
     || !target || !targetLen
     || (baseline && !baselineLen)) return NULL;
  else{
    fsl_card_Q * c =
      (fsl_card_Q*)fsl_malloc(sizeof(fsl_card_Q));
    if(c){
      int rc = 0;
      *c = fsl_card_Q_empty;
      c->type = type;
      c->target = fsl_strndup(target, targetLen);
      if(!c->target) rc = FSL_RC_OOM;
      else if(baseline){
        c->baseline = fsl_strndup(baseline, baselineLen);
        if(!c->baseline) rc = FSL_RC_OOM;
      }
      if(rc){
        fsl_card_Q_free(c);
        c = NULL;
      }
    }
    return c;
  }
}

void fsl_card_Q_free( fsl_card_Q * cp ){
  if(cp){
    fsl_free(cp->target);
    fsl_free(cp->baseline);
    *cp = fsl_card_Q_empty;
    fsl_free(cp);
  }
}

fsl_card_J * fsl_card_J_malloc(bool isAppend,
                               char const * field,
                               char const * value){
  if(!field || !*field) return NULL;
  else{
    fsl_card_J * c =
      (fsl_card_J*)fsl_malloc(sizeof(fsl_card_J));
    if(c){
      int rc = 0;
      fsl_size_t const lF = fsl_strlen(field);
      fsl_size_t const lV = value ? fsl_strlen(value) : 0;
      *c = fsl_card_J_empty;
      c->append = isAppend ? 1 : 0;
      c->field = fsl_strndup(field, (fsl_int_t)lF);
      if(!c->field) rc = FSL_RC_OOM;
      else if(value && *value){
        c->value = fsl_strndup(value, (fsl_int_t)lV);
        if(!c->value) rc = FSL_RC_OOM;
      }
      if(rc){
        fsl_card_J_free(c);
        c = NULL;
      }
    }
    return c;
  }
}

void fsl_card_J_free( fsl_card_J * cp ){
  if(cp){
    fsl_free(cp->field);
    fsl_free(cp->value);
    *cp = fsl_card_J_empty;
    fsl_free(cp);
  }
}

/**
    fsl_list_visitor_f() impl which requires that obj be-a (fsl_card_T*),
    which this function passes to fsl_card_T_free().
*/
static int fsl_list_v_card_T_free(void * obj, void * visitorState ){
  if(obj) fsl_card_T_free( (fsl_card_T*)obj );
  return 0;
}

static int fsl_list_v_card_Q_free(void * obj, void * visitorState ){
  if(obj) fsl_card_Q_free( (fsl_card_Q*)obj );
  return 0;
}

static int fsl_list_v_card_J_free(void * obj, void * visitorState ){
  if(obj) fsl_card_J_free( (fsl_card_J*)obj );
  return 0;
}

fsl_deck * fsl_deck_malloc(){
  fsl_deck * rc = (fsl_deck *)fsl_malloc(sizeof(fsl_deck));
  if(rc){
    *rc = fsl_deck_empty;
    rc->allocStamp = &fsl_deck_empty;
  }
  return rc;
}

void fsl_deck_init( fsl_cx * f, fsl_deck * cards, fsl_satype_e type ){
  void const * allocStamp = cards->allocStamp;
  *cards = fsl_deck_empty;
  cards->allocStamp = allocStamp;
  cards->f = f;
  cards->type = type;
}

void fsl_card_J_list_free(fsl_list * li, bool alsoListMem){
  if(li->used) fsl_list_visit(li, 0, fsl_list_v_card_J_free, NULL);
  if(alsoListMem) fsl_list_reserve(li, 0);
  else li->used = 0;
}

/* fsl_deck cleanup helpers... */
#define SFREE(X) fsl_deck_clean_string(m, &m->X)
#define SLIST(X) fsl_list_clear(&m->X, fsl_list_v_card_string_free, m)
#define CBUF(X) fsl_buffer_clear(&m->X)
static void fsl_deck_clean_string(fsl_deck *m, char **member){
  fsl_deck_free_string(m, *member);
  *member = 0;
}
static void fsl_deck_clean_version(fsl_deck *m){
  fsl_deck_clean_string(m, &m->uuid);
  m->rid = 0;
}
static void fsl_deck_clean_A(fsl_deck *m){
  SFREE(A.name);
  SFREE(A.tgt);
  SFREE(A.src);
}
static void fsl_deck_clean_B(fsl_deck *m){
  if(m->B.baseline){
    assert(!m->B.baseline->B.uuid && "Baselines cannot have a B-card. API misuse?");
    fsl_deck_finalize(m->B.baseline);
    m->B.baseline = NULL;
  }
  SFREE(B.uuid);
}
static void fsl_deck_clean_C(fsl_deck *m){
  fsl_deck_clean_string(m, &m->C);
}
static void fsl_deck_clean_E(fsl_deck *m){
  fsl_deck_clean_string(m, &m->E.uuid);
  m->E = fsl_deck_empty.E;
}
static void fsl_deck_clean_F(fsl_deck *m){
  if(m->F.list){
    fsl_card_F_list_finalize(&m->F);
    m->F = fsl_deck_empty.F;
  }
}
static void fsl_deck_clean_G(fsl_deck *m){
  fsl_deck_clean_string(m, &m->G);
}
static void fsl_deck_clean_H(fsl_deck *m){
  fsl_deck_clean_string(m, &m->H);
}
static void fsl_deck_clean_I(fsl_deck *m){
  fsl_deck_clean_string(m, &m->I);
}
static void fsl_deck_clean_J(fsl_deck *m, bool alsoListMem){
  fsl_card_J_list_free(&m->J, alsoListMem);
}
static void fsl_deck_clean_K(fsl_deck *m){
  fsl_deck_clean_string(m, &m->K);
}
static void fsl_deck_clean_L(fsl_deck *m){
  fsl_deck_clean_string(m, &m->L);
}
static void fsl_deck_clean_M(fsl_deck *m){
  SLIST(M);
}
static void fsl_deck_clean_N(fsl_deck *m){
  fsl_deck_clean_string(m, &m->N);
}
static void fsl_deck_clean_P(fsl_deck *m){
  fsl_list_clear(&m->P, fsl_list_v_card_string_free, m);
}
static void fsl_deck_clean_Q(fsl_deck *m){
  fsl_list_clear(&m->Q, fsl_list_v_card_Q_free, NULL);
}
static void fsl_deck_clean_R(fsl_deck *m){
  fsl_deck_clean_string(m, &m->R);
}
static void fsl_deck_clean_T(fsl_deck *m){
  fsl_list_clear(&m->T, fsl_list_v_card_T_free, NULL);
}
static void fsl_deck_clean_U(fsl_deck *m){
  fsl_deck_clean_string(m, &m->U);
}
static void fsl_deck_clean_W(fsl_deck *m){
  CBUF(W);
}

void fsl_deck_clean2(fsl_deck *m, fsl_buffer *xferBuf){
  if(!m) return;
  fsl_deck_clean_version(m);  
  fsl_deck_clean_A(m);
  fsl_deck_clean_B(m);
  fsl_deck_clean_C(m);
  m->D = 0.0;
  fsl_deck_clean_E(m);
  fsl_deck_clean_F(m);
  fsl_deck_clean_G(m);
  fsl_deck_clean_H(m);
  fsl_deck_clean_I(m);
  fsl_deck_clean_J(m,true);
  fsl_deck_clean_K(m);
  fsl_deck_clean_L(m);
  fsl_deck_clean_M(m);
  fsl_deck_clean_N(m);
  fsl_deck_clean_P(m);
  fsl_deck_clean_Q(m);
  fsl_deck_clean_R(m);
  fsl_deck_clean_T(m);
  fsl_deck_clean_U(m);
  fsl_deck_clean_W(m);
  if(xferBuf){
    fsl_buffer_swap(&m->content, xferBuf);
    fsl_buffer_reuse(xferBuf);
  }
  CBUF(content) /* content must be after all cards because some point
                   back into it and we need this memory intact in
                   order to know that!
                */;
  {
    void const * const allocStampKludge = m->allocStamp;
    fsl_cx * const f = m->f;
    *m = fsl_deck_empty;
    m->allocStamp = allocStampKludge;
    m->f = f;
  }
}
#undef CBUF
#undef SFREE
#undef SLIST

void fsl_deck_clean(fsl_deck *m){
  fsl_deck_clean2(m, NULL);
}

void fsl_deck_finalize(fsl_deck *m){
  void const * allocStamp;
  if(!m) return;
  allocStamp = m->allocStamp;
  fsl_deck_clean(m);
  if(allocStamp == &fsl_deck_empty){
    fsl_free(m);
  }else{
    m->allocStamp = allocStamp;
  }
}

int fsl_card_is_legal( fsl_satype_e t, char card ){
  /*
    Implements this table:
    
    https://fossil-scm.org/index.html/doc/trunk/www/fileformat.wiki#summary
  */
  if('Z'==card) return 1;
  else switch(t){
    case FSL_SATYPE_ANY:
      switch(card){
        case 'A': case 'B': case 'C': case 'D':
        case 'E': case 'F': case 'J': case 'K':
        case 'L': case 'M': case 'N': case 'P':
        case 'Q': case 'R': case 'T': case 'U':
        case 'W':
          return -1;
        default:
          return 0;
      }
    case FSL_SATYPE_ATTACHMENT:
      switch(card){
        case 'A': case 'D':
          return 1;
        case 'C': case 'N': case 'U':
          return -1;
        default:
          return 0;
      };
    case FSL_SATYPE_CLUSTER:
      return 'M'==card ? 1 : 0;
    case FSL_SATYPE_CONTROL:
      switch(card){
        case 'D': case 'T': case 'U':
          return 1;
        default:
          return 0;
      };
    case FSL_SATYPE_EVENT:
      switch(card){
        case 'D': case 'E':
        case 'W':
          return 1;
        case 'C': case 'N':
        case 'P': case 'T':
        case 'U':
          return -1;
        default:
          return 0;
      };
    case FSL_SATYPE_CHECKIN:
      switch(card){
        case 'C': case 'D':
        case 'U':
          return 1;
        case 'B': case 'F': 
        case 'N': case 'P':
        case 'Q': case 'R':
        case 'T': 
          return -1;
        default:
          return 0;
      };
    case FSL_SATYPE_TICKET:
      switch(card){
        case 'D': case 'J':
        case 'K': case 'U':
          return 1;
        default:
          return 0;
      };
    case FSL_SATYPE_WIKI:
      switch(card){
        case 'D': case 'L':
        case 'U': case 'W':
          return 1;
        case 'C':
        case 'N': case 'P':
          return -1;
        default:
          return 0;
      };
    case FSL_SATYPE_FORUMPOST:
      switch(card){
        case 'D': case 'U': case 'W':
          return 1;
        case 'G': case 'H': case 'I':
        case 'N': case 'P':
          return -1;
        default:
          return 0;
      };
    default:
      assert(!"Invalid fsl_satype_e.");
      return 0;
  };
}

bool fsl_deck_has_required_cards( fsl_deck const * d ){
  if(!d) return 0;
  switch(d->type){
    case FSL_SATYPE_ANY:
      return 0;
#define NEED(CARD,COND) \
      if(!(COND)) {                                         \
        fsl_cx_err_set(d->f, FSL_RC_SYNTAX,                 \
                       "Required %c-card is missing or invalid.", \
                       *#CARD);                                   \
        return false;                                             \
      } (void)0
    case FSL_SATYPE_ATTACHMENT:
      NEED(A,d->A.name);
      NEED(A,d->A.tgt);
      NEED(D,d->D > 0);
      return 1;
    case FSL_SATYPE_CLUSTER:
      NEED(M,d->M.used);
      return 1;
    case FSL_SATYPE_CONTROL:
      NEED(D,d->D > 0);
      NEED(U,d->U);
      NEED(T,d->T.used>0);
      return 1;
    case FSL_SATYPE_EVENT:
      NEED(D,d->D > 0);
      NEED(E,d->E.julian>0);
      NEED(E,d->E.uuid);
      NEED(W,d->W.used);
      return 1;
    case FSL_SATYPE_CHECKIN:
      /*
        Historically we need both or neither of F- and R-cards, but
        the R-card has become optional because it's so expensive to
        calculate and verify.

        Manifest #1 has an empty file list and an R-card with a
        constant (repo/manifest-independent) hash
        (d41d8cd98f00b204e9800998ecf8427e, the initial MD5 hash
        state).

        R-card calculation is runtime-configurable option. Rather
        than rely on d->f being set (so d->f->flags can tell us
        whether or not to calculate an R-card), we'll rely on
        downstream code to check for an R-card if needed.
      */
      NEED(D,d->D > 0);
      NEED(C,d->C);
      NEED(U,d->U);
#if 0
      /* It turns out that because the R-card is optional,
         we can have a legal manifest with no F-cards. */
      NEED(F,d->F.used || d->R/*with initial-state md5 hash!*/);
#endif
      if(!d->R
         && (FSL_CX_F_CALC_R_CARD & d->f->flags)){
        fsl_cx_err_set(d->f, FSL_RC_SYNTAX,
                       "%s deck is missing an R-card, "
                       "yet R-card calculation is enabled.",
                       fsl_satype_cstr(d->type));
        return 0;
      }else if(d->R
               && !d->F.used
               && 0!=fsl_strcmp(d->R, FSL_MD5_INITIAL_HASH)
               ){
        fsl_cx_err_set(d->f, FSL_RC_SYNTAX,
                       "Deck has no F-cards, so we expect its "
                       "R-card is to have the initial-state MD5 "
                       "hash (%.12s...). Instead we got: %s",
                       FSL_MD5_INITIAL_HASH, d->R);
        return 0;
      }
      return 1;
    case FSL_SATYPE_TICKET:
      NEED(D,d->D > 0);
      NEED(K,d->K);
      NEED(U,d->U);
      NEED(J,d->J.used)
        /* Is a J strictly required?  Spec is not clear but DRH
           confirms the current fossil(1) code expects a J card. */;
      return 1;
    case FSL_SATYPE_WIKI:
      NEED(D,d->D > 0);
      NEED(L,d->L);
      NEED(U,d->U);
      /*NEED(W,d->W.used);*/
      return 1;
    case FSL_SATYPE_FORUMPOST:
      NEED(D,d->D > 0);
      NEED(U,d->U);
      /*NEED(W,d->W.used);*/
      return 1;
    case FSL_SATYPE_INVALID:
    default:
      assert(!"Invalid fsl_satype_e.");
      return 0;
  }
#undef NEED
}


char const * fsl_satype_cstr(fsl_satype_e t){
  switch(t){
#define C(X) case FSL_SATYPE_##X: return #X
    C(ANY);
    C(CHECKIN);
    C(CLUSTER);
    C(CONTROL);
    C(WIKI);
    C(TICKET);
    C(ATTACHMENT);
    C(EVENT);
    C(FORUMPOST);
    C(INVALID);
    C(BRANCH_START);
    default:
      assert(!"UNHANDLED fsl_satype_e");
      return "!UNKNOWN!";
  }
}

char const * fsl_satype_event_cstr(fsl_satype_e t){
  switch(t){
    case FSL_SATYPE_ANY: return "*";
    case FSL_SATYPE_BRANCH_START:
    case FSL_SATYPE_CHECKIN: return "ci";
    case FSL_SATYPE_EVENT: return "e";
    case FSL_SATYPE_CONTROL: return "g";
    case FSL_SATYPE_TICKET: return "t";
    case FSL_SATYPE_WIKI: return "w";
    case FSL_SATYPE_FORUMPOST: return "f";
    default:
      return NULL;
  }
}


/**
    If fsl_card_is_legal(d->type, card), returns true, else updates
    d->f->error with a description of the constraint violation and
    returns 0.
 */
static bool fsl_deck_check_type( fsl_deck * d, char card ){
  if(fsl_card_is_legal(d->type, card)) return true;
  else{
    fsl_cx_err_set(d->f, FSL_RC_TYPE,
                   "Card type '%c' is not allowed "
                   "in artifacts of type %s.",
                   card, fsl_satype_cstr(d->type));
    return false;
  }
}

/**
   If the first n bytes of the given string contain any values <=32,
   returns FSL_RC_SYNTAX, else returns 0. mf->f's error state is
   updated no error. n<0 means to use fsl_strlen() to count the
   length.
*/
static int fsl_deck_strcheck_ctrl_chars(fsl_deck * mf, char cardName, char const * v, fsl_int_t n){
  const char * z = v;
  int rc = 0;
  if(v && n<0) n = fsl_strlen(v);
  for( ; v && z < v+n; ++z ){
    if(*z <= 32){
      rc = fsl_cx_err_set(mf->f, FSL_RC_SYNTAX,
                         "Invalid character in %c-card.", cardName);
      break;
    }
  }
  return rc;
}

/*
  Implements fsl_deck_LETTER_set() for certain letters: those
  implemented as a fsl_uuid_str or an md5, holding a single hex string
  value.
  
  The function returns FSL_RC_SYNTAX if
  (valLen!=ASSERTLEN). ASSERTLEN is assumed to be either an SHA1,
  SHA3, or MD5 hash value and it is validated against
  fsl_validate16(value,valLen), returning FSL_RC_SYNTAX if that
  check fails. In debug builds, the expected ranges are assert()ed.

  If value is NULL then it is removed from the card instead
  (semantically freed), *mfMember is set to NULL, and 0 is returned.
*/
static int fsl_deck_sethex_impl( fsl_deck * mf, fsl_uuid_cstr value,
                                 char letter,
                                 fsl_size_t assertLen,
                                 char ** mfMember ){
  assert(mf);
  assert( value ? (assertLen==FSL_STRLEN_SHA1
                   || assertLen==FSL_STRLEN_K256
                   || assertLen==FSL_STRLEN_MD5)
          : 0==assertLen );
  if(value && !fsl_deck_check_type(mf,letter)) return mf->f->error.code;
  else if(!value){
    fsl_deck_free_string(mf, *mfMember);
    *mfMember = NULL;
    return 0;
  }else if(fsl_strlen(value) != assertLen){
    return fsl_cx_err_set(mf->f, FSL_RC_SYNTAX,
                          "Invalid length for %c-card: expecting %d.",
                          letter, (int)assertLen);
  }else if(!fsl_validate16(value, assertLen)) {
    return fsl_cx_err_set(mf->f, FSL_RC_SYNTAX,
                          "Invalid hexadecimal value for %c-card.", letter);
  }else{
    fsl_deck_free_string(mf, *mfMember);
    *mfMember = fsl_strndup(value, assertLen);
    return *mfMember ? 0 : FSL_RC_OOM;
  }
}

/**
    Implements fsl_set_set_XXX() where XXX is a fsl_buffer member of fsl_deck.
 */
static int fsl_deck_b_setuffer_impl( fsl_deck * mf, char const * value,
                                     fsl_int_t valLen,
                                     char letter, fsl_buffer * buf){
  assert(mf);  
  if(!fsl_deck_check_type(mf,letter)) return mf->f->error.code;
  else if(valLen<0) valLen = (fsl_int_t)fsl_strlen(value);
  buf->used = 0;
  if(value && (valLen>0)){
    return fsl_buffer_append( buf, value, valLen );
  }else{
    if(buf->mem) buf->mem[0] = 0;
    return 0;
  }
}

int fsl_deck_B_set( fsl_deck * mf, fsl_uuid_cstr uuidBaseline){
  if(!mf) return FSL_RC_MISUSE;
  else{
    int const bLen = uuidBaseline ? fsl_is_uuid(uuidBaseline) : 0;
    if(uuidBaseline && !bLen){
      return fsl_cx_err_set(mf->f, FSL_RC_SYNTAX,
                            "Invalid B-card value: %s", uuidBaseline);
    }
    if(mf->B.baseline){
      fsl_deck_finalize(mf->B.baseline);
      mf->B.baseline = NULL;
    }
    return fsl_deck_sethex_impl(mf, uuidBaseline, 'B',
                                bLen, &mf->B.uuid);
  }
}

/**
    Internal impl for card setters which consist of a simple (char *)
    member. Replaces and frees any prior value. Passing NULL for the
    4th argument unsets the given card (assigns NULL to it).
 */
static int fsl_deck_set_string( fsl_deck * mf, char letter, char ** member, char const * v, fsl_int_t n ){
  if(!fsl_deck_check_type(mf, letter)) return mf->f->error.code;
  fsl_deck_free_string(mf, *member);
  *member = v ? fsl_strndup(v, n) : NULL;
  if(v && !*member) return FSL_RC_OOM;
  else return 0;
}

int fsl_deck_C_set( fsl_deck * mf, char const * v, fsl_int_t n){
  return fsl_deck_set_string( mf, 'C', &mf->C, v, n );
}

int fsl_deck_G_set( fsl_deck * mf, fsl_uuid_cstr uuid){
  int const uLen = fsl_is_uuid(uuid);
  return uLen
    ? fsl_deck_sethex_impl(mf, uuid, 'G', uLen, &mf->G)
    : FSL_RC_SYNTAX;
}

int fsl_deck_H_set( fsl_deck * mf, char const * v, fsl_int_t n){
  if(v && mf->I) return FSL_RC_SYNTAX;
  return fsl_deck_set_string( mf, 'H', &mf->H, v, n );
}

int fsl_deck_I_set( fsl_deck * mf, fsl_uuid_cstr uuid){
  if(uuid && mf->H) return FSL_RC_SYNTAX;
  int const uLen = uuid ? fsl_is_uuid(uuid) : 0;
  return fsl_deck_sethex_impl(mf, uuid, 'I', uLen, &mf->I);
}

int fsl_deck_J_add( fsl_deck * mf, char isAppend,
                    char const * field, char const * value){
  if(!field) return FSL_RC_MISUSE;
  else if(!*field) return FSL_RC_SYNTAX;
  else if(!fsl_deck_check_type(mf,'J')) return mf->f->error.code;
  else{
    int rc;
    fsl_card_J * cp = fsl_card_J_malloc(isAppend, field, value);
    if(!cp) rc = FSL_RC_OOM;
    else if( 0 != (rc = fsl_list_append(&mf->J, cp))){
      fsl_card_J_free(cp);
    }
    return rc;
  }
}


int fsl_deck_K_set( fsl_deck * mf, fsl_uuid_cstr uuid){
  int const uLen = fsl_is_uuid(uuid);
  return uLen
    ? fsl_deck_sethex_impl(mf, uuid, 'K', uLen, &mf->K)
    : FSL_RC_SYNTAX;
}

int fsl_deck_L_set( fsl_deck * mf, char const * v, fsl_int_t n){
  return mf
    ? fsl_deck_set_string(mf, 'L', &mf->L, v, n)
    : FSL_RC_SYNTAX;
}

int fsl_deck_M_add( fsl_deck * mf, char const *uuid){
  int rc;
  char * dupe;
  int const uLen = uuid ? fsl_is_uuid(uuid) : 0;
  if(!uuid) return FSL_RC_MISUSE;
  else if(!fsl_deck_check_type(mf, 'M')) return mf->f->error.code;
  else if(!uLen) return FSL_RC_SYNTAX;
  dupe = fsl_strndup(uuid, uLen);
  if(!dupe) rc = FSL_RC_OOM;
  else{
    rc = fsl_list_append( &mf->M, dupe );
    if(rc){
      fsl_free(dupe);
    }
  }
  return rc;
}

int fsl_deck_N_set( fsl_deck * mf, char const * v, fsl_int_t n){
  int rc = 0;
  if(v && n!=0){
    if(n<0) n = fsl_strlen(v);
    rc = fsl_deck_strcheck_ctrl_chars(mf, 'N', v, n);
  }
  return rc ? rc : fsl_deck_set_string( mf, 'N', &mf->N, v, n );
}


int fsl_deck_P_add( fsl_deck * mf, char const *parentUuid){
  int rc;
  char * dupe;
  int const uLen = parentUuid ? fsl_is_uuid(parentUuid) : 0;
  if(!mf || !parentUuid || !*parentUuid) return FSL_RC_MISUSE;
  else if(!fsl_deck_check_type(mf, 'P')) return mf->f->error.code;
  else if(!uLen){
    return fsl_cx_err_set(mf->f, FSL_RC_SYNTAX,
                         "Invalid UUID for P-card.");
  }
  dupe = fsl_strndup(parentUuid, uLen);
  if(!dupe) rc = FSL_RC_OOM;
  else{
    rc = fsl_list_append( &mf->P, dupe );
    if(rc){
      fsl_free(dupe);
    }
  }
  return rc;
}

fsl_id_t fsl_deck_P_get_id(fsl_deck * d, int index){
  if(!d->f) return -1;
  else if(index>(int)d->P.used) return 0;
  else return fsl_uuid_to_rid(d->f, (char const *)d->P.list[index]);
}


int fsl_deck_Q_add( fsl_deck * mf, int type,
                    fsl_uuid_cstr target,
                    fsl_uuid_cstr baseline ){
  if(!target) return FSL_RC_MISUSE;
  else if(!fsl_deck_check_type(mf,'Q')) return mf->f->error.code;
  else if(!type || !fsl_is_uuid(target)
          || (baseline && !fsl_is_uuid(baseline))) return FSL_RC_SYNTAX;
  else{
    int rc;
    fsl_card_Q * cp = fsl_card_Q_malloc(type, target, baseline);
    if(!cp) rc = FSL_RC_OOM;
    else if( 0 != (rc = fsl_list_append(&mf->Q, cp))){
      fsl_card_Q_free(cp);
    }
    return rc;
  }
}

/**
    A comparison routine for qsort(3) which compares fsl_card_F
    instances in a lexical manner based on their names.  The order is
    important for card ordering in generated manifests.

    It expects that each argument is a (fsl_card_F const *).
*/
static int fsl_card_F_cmp( void const * lhs, void const * rhs ){
  fsl_card_F const * const l = (fsl_card_F const *)lhs;
  fsl_card_F const * const r = (fsl_card_F const *)rhs;
  /* Compare NULL as larger so that NULLs move to the right. That said,
     we aren't expecting any NULLs. */
  assert(l);
  assert(r);
  if(!l) return r ? 1 : 0;
  else if(!r) return -1;
  else return fsl_strcmp(l->name, r->name);
}

static void fsl_card_F_list_sort(fsl_card_F_list * li){
  if(FSL_CARD_F_LIST_NEEDS_SORT & li->flags){
    qsort(li->list, li->used, sizeof(fsl_card_F),
          fsl_card_F_cmp );
    li->flags &= ~FSL_CARD_F_LIST_NEEDS_SORT;
  }
}

static void fsl_deck_F_sort(fsl_deck * mf){
  fsl_card_F_list_sort(&mf->F);
}

int fsl_card_F_compare( fsl_card_F const * lhs,
                        fsl_card_F const * rhs){
  return (lhs == rhs) ? 0 : fsl_card_F_cmp( lhs, rhs );
}

int fsl_deck_R_set( fsl_deck * mf, fsl_uuid_cstr md5){
  return mf
    ? fsl_deck_sethex_impl(mf, md5, 'R', md5 ? FSL_STRLEN_MD5 : 0, &mf->R)
    : FSL_RC_MISUSE;
}

int fsl_deck_R_calc2(fsl_deck *mf, char ** tgt){
  fsl_cx * f = mf->f;
  char const * theHash = 0;
  char hex[FSL_STRLEN_MD5+1];
  if(!f) return FSL_RC_MISUSE;
  else if(!fsl_needs_repo(f)){
    return FSL_RC_NOT_A_REPO;
  }else if(!fsl_deck_check_type(mf,'R')) {
    assert(mf->f->error.code);
    return mf->f->error.code;
  }else if(!mf->F.used){
    theHash = FSL_MD5_INITIAL_HASH;
    /* fall through and set hash */
  }else{
    int rc = 0;
    fsl_card_F const * fc;
    fsl_id_t fileRid;
    fsl_buffer * buf = &f->fileContent;
    unsigned char digest[16];
    fsl_md5_cx md5 = fsl_md5_cx_empty;
    enum { NumBufSize = 40 };
    char numBuf[NumBufSize] = {0};
    assert(!buf->used && "Misuse of f->fileContent buffer.");
    rc = fsl_deck_F_rewind(mf);
    if(rc) goto end;
    fsl_deck_F_sort(mf);
    /*
      TODO:

      Missing functionality:

      - The "wd" (working directory) family of functions, needed for
      symlink handling.
    */
    while(1){
      rc = fsl_deck_F_next(mf, &fc);
      /* MARKER(("R rc=%s file #%d: %s %s\n", fsl_rc_cstr(rc), ++i, fc ? fc->name : "<END>", fc ? fc->uuid : NULL)); */
      if(rc || !fc) break;
      assert(fc->uuid && "We no longer iterate over deleted entries.");
      if(FSL_FILE_PERM_LINK==fc->perm){
        rc = fsl_cx_err_set(f, FSL_RC_UNSUPPORTED,
                            "This code does not yet properly handle "
                            "F-cards of symlinks.");
        goto end;
      }
      fileRid = fsl_uuid_to_rid( f, fc->uuid );
      if(0==fileRid){
        rc = fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                            "Cannot resolve RID for F-card UUID [%s].",
                            fc->uuid);
        goto end;
      }else if(fileRid<0){
        assert(f->error.code);
        rc = f->error.code
          ? f->error.code
          : fsl_cx_err_set(f, FSL_RC_ERROR,
                           "Error resolving RID for F-card UUID [%s].",
                           fc->uuid);
        goto end;
      }
      fsl_md5_update_cstr(&md5, fc->name, -1);
      rc = fsl_content_get(f, fileRid, buf);
      if(rc){
        goto end;
      }
      numBuf[0] = 0;
      fsl_snprintf(numBuf, NumBufSize,
                   " %"FSL_SIZE_T_PFMT"\n",
                   (fsl_size_t)buf->used);
      fsl_md5_update_cstr(&md5, numBuf, -1);
      fsl_md5_update_buffer(&md5, buf);
    }
    if(!rc){
      fsl_md5_final(&md5, digest);
      fsl_md5_digest_to_base16(digest, hex);
    }
    end:
    fsl_cx_content_buffer_yield(f);
    assert(0==buf->used);
    if(rc) return rc;
    fsl_deck_F_rewind(mf);
    theHash = hex;
  }
  assert(theHash);
  if(*tgt){
    memcpy(*tgt, theHash, FSL_STRLEN_MD5);
    (*tgt)[FSL_STRLEN_MD5+1] = 0;
    return 0;
  }else{
    char * x = fsl_strdup(theHash);
    if(x) *tgt = x;
    return x ? 0 : FSL_RC_OOM;
  }
}

int fsl_deck_R_calc(fsl_deck * mf){
  char R[FSL_STRLEN_MD5+1] = {0};
  char * r = R;
  int rc = fsl_deck_R_calc2(mf, &r);
  return rc ? rc : fsl_deck_R_set(mf, r);
}

int fsl_deck_T_add2( fsl_deck * mf, fsl_card_T * t){
  if(!t) return FSL_RC_MISUSE;
  else if(!fsl_deck_check_type(mf, 'T')){
    return mf->f->error.code;
  }else if(FSL_SATYPE_CONTROL==mf->type && NULL==t->uuid){
    return fsl_cx_err_set(mf->f, FSL_RC_SYNTAX,
                            "CONTROL artifacts may not have "
                            "self-referential tags.");
  }else if(FSL_SATYPE_TECHNOTE==mf->type){
    if(NULL!=t->uuid){
      return fsl_cx_err_set(mf->f, FSL_RC_SYNTAX,
                              "TECHNOTE artifacts may not have "
                              "tags which refer to other objects.");
    }else if(FSL_TAGTYPE_ADD!=t->type){
      return fsl_cx_err_set(mf->f, FSL_RC_SYNTAX,
                            "TECHNOTE artifacts may only have "
                            "ADD-type tags.");
    }
  }
  if(!t->name || !*t->name){
    return fsl_cx_err_set(mf->f, FSL_RC_SYNTAX,
                         "Tag name may not be empty.");
  }else if(fsl_validate16(t->name, fsl_strlen(t->name))){
    return fsl_cx_err_set(mf->f, FSL_RC_SYNTAX,
                         "Tag name may not be hexadecimal.");
  }else if(t->uuid && !fsl_is_uuid(t->uuid)){
    return fsl_cx_err_set(mf->f, FSL_RC_SYNTAX,
                         "Invalid UUID in tag.");
  }
  return fsl_list_append(&mf->T, t);
}

int fsl_deck_T_add( fsl_deck * mf, fsl_tagtype_e tagType,
                    char const * uuid, char const * name,
                    char const * value){
  if(!name) return FSL_RC_MISUSE;
  else if(!fsl_deck_check_type(mf, 'T')) return mf->f->error.code;
  else if(!*name || (uuid &&!fsl_is_uuid(uuid))) return FSL_RC_SYNTAX;
  else switch(tagType){
    case FSL_TAGTYPE_CANCEL:
    case FSL_TAGTYPE_ADD:
    case FSL_TAGTYPE_PROPAGATING:{
      int rc;
      fsl_card_T * t;
      t = fsl_card_T_malloc(tagType, uuid, name, value);
      if(!t) return FSL_RC_OOM;
      rc = fsl_deck_T_add2( mf, t );
      if(rc) fsl_card_T_free(t);
      return rc;
    }
    default:
      assert(!"Invalid tagType value");
      return fsl_cx_err_set(mf->f, FSL_RC_SYNTAX,
                            "Invalid tag-type value: %d",
                            (int)tagType);
  }
}

/**
   Returns true if the NUL-terminated string contains only
   "reasonable" branch name character, with the native assumption that
   anything <=32d is "unreasonable" and anything >=128 is part of a
   multibyte UTF8 character.
*/
static bool fsl_is_valid_branchname(char const * z_){
  unsigned char const * z = (unsigned char const*)z_;
  unsigned len = 0;
  for(; z[len]; ++len){
    if(z[len] <= 32) return false;
  }
  return len>0;
}

int fsl_deck_branch_set( fsl_deck * d, char const * branchName ){
  if(!fsl_is_valid_branchname(branchName)){
    return fsl_cx_err_set(d->f, FSL_RC_RANGE, "Branch name contains "
                          "invalid characters.");
  }
  int rc= fsl_deck_T_add(d, FSL_TAGTYPE_PROPAGATING, NULL,
                         "branch", branchName);
  if(!rc){
    char * sym = fsl_mprintf("sym-%s", branchName);
    if(sym){
      rc = fsl_deck_T_add(d, FSL_TAGTYPE_PROPAGATING, NULL,
                          sym, NULL);
      fsl_free(sym);
    }else{
      rc = FSL_RC_OOM;
    }
  }
  return rc;
}

int fsl_deck_U_set( fsl_deck * mf, char const * v){
  return fsl_deck_set_string( mf, 'U', &mf->U, v, -1 );
}

int fsl_deck_W_set( fsl_deck * mf, char const * v, fsl_int_t n){
  return fsl_deck_b_setuffer_impl(mf, v, n, 'W', &mf->W);
}

int fsl_deck_A_set( fsl_deck * mf, char const * name,
                    char const * tgt,
                    char const * uuidSrc ){
  int const uLen = (uuidSrc && *uuidSrc) ? fsl_is_uuid(uuidSrc) : 0;
  if(!name || !tgt) return FSL_RC_MISUSE;
  else if(!fsl_deck_check_type(mf, 'A')) return mf->f->error.code;
  else if(!*tgt){
    return fsl_cx_err_set(mf->f, FSL_RC_SYNTAX,
                          "Invalid target name in A card.");
  }
  /* TODO: validate tgt based on mf->type and require UUID
     for types EVENT/TICKET.
  */
  else if(uuidSrc && *uuidSrc && !uLen){
    return fsl_cx_err_set(mf->f, FSL_RC_SYNTAX,
                          "Invalid source UUID in A card.");
  }
  else{
    int rc = 0;
    fsl_deck_free_string(mf, mf->A.tgt);
    fsl_deck_free_string(mf, mf->A.src);
    fsl_deck_free_string(mf, mf->A.name);
    mf->A.name = mf->A.src = NULL;
    if(! (mf->A.tgt = fsl_strdup(tgt))) rc = FSL_RC_OOM;
    else if( !(mf->A.name = fsl_strdup(name))) rc = FSL_RC_OOM;
    else if(uLen){
      mf->A.src = fsl_strndup(uuidSrc,uLen);
      if(!mf->A.src) rc = FSL_RC_OOM
        /* Leave mf->A.tgt/name for downstream cleanup. */;
    }
    return rc;
  }
}


int fsl_deck_D_set( fsl_deck * mf, double date){
  if(date<0) return FSL_RC_RANGE;
  else if(date>0 && !fsl_deck_check_type(mf, 'D')){
    return mf->f->error.code;
  }else{
    mf->D = date;
    return 0;
  }
}

int fsl_deck_E_set( fsl_deck * mf, double date, char const * uuid){
  int const uLen = uuid ? fsl_is_uuid(uuid) : 0;
  if(!mf || !uLen) return FSL_RC_MISUSE;
  else if(date<=0){
    return fsl_cx_err_set(mf->f, FSL_RC_RANGE,
                          "Invalid date value for E card.");
  }else if(!uLen){
    return fsl_cx_err_set(mf->f, FSL_RC_RANGE,
                          "Invalid UUID for E card.");
  }
  else{
    mf->E.julian = date;
    fsl_deck_free_string(mf, mf->E.uuid);
    mf->E.uuid = fsl_strndup(uuid, uLen);
    return mf->E.uuid ? 0 : FSL_RC_OOM;
  }
}

int fsl_deck_F_add( fsl_deck * mf, char const * name,
                    char const * uuid,
                    fsl_fileperm_e perms, 
                    char const * oldName){
  int const uLen = uuid ? fsl_is_uuid(uuid) : 0;
  if(!mf || !name) return FSL_RC_MISUSE;
  else if(!uuid && !mf->B.uuid){
    return fsl_cx_err_set(mf->f, FSL_RC_MISUSE,
                         "NULL UUID is not valid for baseline "
                          "manifests.");
  }
  else if(!fsl_deck_check_type(mf, 'F')) return mf->f->error.code;
  else if(!*name){
    return fsl_cx_err_set(mf->f, FSL_RC_RANGE,
                         "F-card name may not be empty.");
  }
  else if(!fsl_is_simple_pathname(name, 1)
          || (oldName && !fsl_is_simple_pathname(oldName, 1))){
    return fsl_cx_err_set(mf->f, FSL_RC_RANGE,
                          "Invalid filename for F-card (simple form required): "
                          "name=[%s], oldName=[%s].", name, oldName);
  }
  else if(uuid && !uLen){
    return fsl_cx_err_set(mf->f, FSL_RC_RANGE,
                          "Invalid UUID for F-card.");
  }
  else {
    int rc = 0;
    fsl_card_F * t;
    switch(perms){
      case FSL_FILE_PERM_EXE:
      case FSL_FILE_PERM_LINK:
      case FSL_FILE_PERM_REGULAR:
        break;
      default:
        assert(!"Invalid fsl_fileperm_e value");
        return fsl_cx_err_set(mf->f, FSL_RC_RANGE,
                              "Invalid fsl_fileperm_e value "
                              "(%d) for file [%s].",
                              perms, name);
    }
    t = fsl_card_F_list_push(&mf->F);
    if(!t) return FSL_RC_OOM;
    assert(mf->F.used>1
           ? (FSL_CARD_F_LIST_NEEDS_SORT & mf->F.flags)
           : 1);
    assert(!t->name);
    assert(!t->uuid);
    assert(!t->priorName);
    assert(!t->deckOwnsStrings);
    t->perm = perms;
    if(0==(t->name = fsl_strdup(name))){
      rc = FSL_RC_OOM;
    }else if(uuid && 0==(t->uuid = fsl_strdup(uuid))){
      rc = FSL_RC_OOM;
    }else if(oldName && 0==(t->priorName = fsl_strdup(oldName))){
      rc = FSL_RC_OOM;
    }
    if(rc){
      fsl_card_F_list_pop(&mf->F);
    }
    return rc;
  }
}

int fsl_deck_F_foreach( fsl_deck * d, fsl_card_F_visitor_f cb, void * visitorState ){
  if(!cb) return FSL_RC_MISUSE;
  else{
    fsl_card_F const * fc;
    int rc = fsl_deck_F_rewind(d);
    while( !rc && !(rc=fsl_deck_F_next(d, &fc)) && fc) {
      rc = cb( fc, visitorState );
    }
    return (FSL_RC_BREAK==rc) ? 0 : rc;
  }
}

  
/**
    Output state for fsl_appendf_f_mf() and friends. Used for managing
    the output of a fsl_deck.
 */
struct fsl_deck_out_state {
  /**
      The set of cards being output. We use this to delegate certain
      output bits.
   */
  fsl_deck const * d;
  /**
      Output routine to send manifest to.
   */
  fsl_output_f out;
  /**
      State to pass as the first arg of this->out().
   */
  void * outState;

  /**
     The previously-visited card, for confirming that all cards are in
     proper lexical order.
   */
  fsl_card_F const * prevCard;
  /**
      f() result code, so that we can feed the code back through the
      fsl_appendf() layer. If this is non-0, processing must stop.  We
      "could" use this->error.code instead, but this is simple.
   */
  int rc;
  /**
      Counter for list-visiting routines. Must be re-set before each
      visit loop if the visitor makes use of this (most do not).
   */
  fsl_int_t counter;

  /**
      Incrementally-calculated MD5 sum of all output sent via
      fsl_appendf_f_mf().
   */
  fsl_md5_cx md5;

  /* Holds error state for propagating back to the client. */
  fsl_error error;

  /**
      Scratch buffer for fossilizing bytes and other temporary work.
      This value comes from fsl_cx_scratchpad().
  */
  fsl_buffer * scratch;
};
typedef struct fsl_deck_out_state fsl_deck_out_state;
static const fsl_deck_out_state fsl_deck_out_state_empty = {
NULL/*d*/,
NULL/*out*/,
NULL/*outState*/,
NULL/*prevCard*/,
0/*rc*/,
0/*counter*/,
fsl_md5_cx_empty_m/*md5*/,
fsl_error_empty_m/*error*/,
NULL/*scratch*/
};

/**
    fsl_appendf_f() impl which forwards its data to arg->out(). arg
    must be a (fsl_deck_out_state *). Updates arg->rc to the result of
    calling arg->out(fp->fState, data, n). If arg->out() succeeds then
    arg->md5 is updated to reflect the given data. i.e. this is where
    the Z-card gets calculated incrementally during output of a deck.
*/
static fsl_int_t fsl_appendf_f_mf( void * arg,
                                   char const * data,
                                   fsl_int_t n ){
  fsl_deck_out_state * os = (fsl_deck_out_state *)arg;
  if((n>0)
     && !(os->rc = os->out(os->outState, data, (fsl_size_t)n))
     && (os->md5.isInit)){
    fsl_md5_update( &os->md5, data, (fsl_size_t)n );
  }
  return (0==os->rc)
    ? n
    : -1;
}

/**
    Internal helper for fsl_deck_output(). Appends formatted output to
    os->out() via fsl_appendf_f_mf(). Returns os->rc (0 on success).
 */
static int fsl_deck_append( fsl_deck_out_state * os,
                            char const * fmt, ... ){
  fsl_int_t rc;
  va_list args;
  assert(os);
  assert(fmt && *fmt);
  va_start(args,fmt);
  rc = fsl_appendfv( fsl_appendf_f_mf, os, fmt, args);
  va_end(args);
  if(rc<0 && !os->rc) os->rc = FSL_RC_IO;
  return os->rc;
}

/**
    Fossilizes (inp, inp+len] bytes to os->scratch,
    overwriting any existing contents.
    Updates and returns os->rc.
 */
static int fsl_deck_fossilize( fsl_deck_out_state * os,
                               unsigned char const * inp,
                               fsl_int_t len){
  fsl_buffer_reuse(os->scratch);
  return os->rc = len
    ? fsl_bytes_fossilize(inp, len, os->scratch)
    : 0;
}

/** Confirms that the given card letter is valid for od->d->type, and
    updates os->rc and os->error if it's not. Returns true if it's
    valid.
*/
static bool fsl_deck_out_tcheck(fsl_deck_out_state * os, char letter){
  if(!fsl_card_is_legal(os->d->type, letter)){
    os->rc = fsl_error_set(&os->error, FSL_RC_TYPE,
                           "%c-card is not valid for deck type %s.",
                           letter, fsl_satype_cstr(os->d->type));
  }
  return os->rc ? false : true;
}

/* Appends a UUID-valued card to os from os->d->{{card}} if the given
   UUID is not NULL, else this is a no-op. */
static int fsl_deck_out_uuid( fsl_deck_out_state * os, char card, fsl_uuid_str uuid ){
  if(uuid && fsl_deck_out_tcheck(os, card)){
    if(!fsl_is_uuid(uuid)){
      os->rc = fsl_error_set(&os->error, FSL_RC_RANGE,
                             "Malformed UUID in %c card.", card);
    }else{
      fsl_deck_append(os, "%c %s\n", card, uuid);
    }
  }
  return os->rc;
}

/* Appends the B card to os from os->d->B. */
static int fsl_deck_out_B( fsl_deck_out_state * os ){
  return fsl_deck_out_uuid(os, 'B', os->d->B.uuid);
}


/* Appends the A card to os from os->d->A. */
static int fsl_deck_out_A( fsl_deck_out_state * os ){
  if(os->d->A.name && fsl_deck_out_tcheck(os, 'A')){
    if(!os->d->A.name || !*os->d->A.name){
      os->rc = fsl_error_set(&os->error, FSL_RC_SYNTAX,
                             "A-card is missing its name property");

    }else if(!os->d->A.tgt || !*os->d->A.tgt){
      os->rc = fsl_error_set(&os->error, FSL_RC_SYNTAX,
                             "A-card is missing its tgt property: %s",
                             os->d->A.name);

    }else if(os->d->A.src && !fsl_is_uuid(os->d->A.src)){
      os->rc = fsl_error_set(&os->error, FSL_RC_TYPE,
                             "Invalid src UUID in A-card: name=%s, "
                             "invalid uuid=%s",
                             os->d->A.name, os->d->A.src);
    }else{
      fsl_deck_append(os, "A %F %F",
                      os->d->A.name, os->d->A.tgt);
      if(!os->rc){
        if(os->d->A.src){
          fsl_deck_append(os, " %s", os->d->A.src);
        }
        if(!os->rc) fsl_deck_append(os, "\n");
      }
    }
  }
  return os->rc;
}

/**
    Internal helper for outputing cards which are simple strings.
    str is the card to output (NULL values are ignored), letter is
    the card letter being output. If doFossilize is true then
    the output gets fossilize-formatted.
 */
static int fsl_deck_out_letter_str( fsl_deck_out_state * os, char letter,
                                    char const * str, char doFossilize ){
  if(str && fsl_deck_out_tcheck(os, letter)){
    if(doFossilize){
      fsl_deck_fossilize(os, (unsigned char const *)str, -1);
      if(!os->rc){
        fsl_deck_append(os, "%c %b\n", letter, os->scratch);
      }
    }else{
      fsl_deck_append(os, "%c %s\n", letter, str);
    }
  }
  return os->rc;
}

/* Appends the C card to os from os->d->C. */
static int fsl_deck_out_C( fsl_deck_out_state * os ){
  return fsl_deck_out_letter_str( os, 'C', os->d->C, 1 );
}


/* Appends the D card to os from os->d->D. */
static int fsl_deck_out_D( fsl_deck_out_state * os ){
  if((os->d->D > 0.0) && fsl_deck_out_tcheck(os, 'D')){
    char ds[24];
    if(!fsl_julian_to_iso8601(os->d->D, ds, 1)){
      os->rc = fsl_error_set(&os->error, FSL_RC_RANGE,
                             "D-card contains invalid "
                             "Julian Day value.");
    }else{
      fsl_deck_append(os, "D %s\n", ds);
    }
  }
  return os->rc;
}

/* Appends the E card to os from os->d->E. */
static int fsl_deck_out_E( fsl_deck_out_state * os ){
  if(os->d->E.uuid && fsl_deck_out_tcheck(os, 'E')){
    char ds[24];
    char msPrecision = FSL_SATYPE_EVENT!=os->d->type
      /* The timestamps on Events historically have seconds precision,
         not ms.
      */;
    if(!fsl_is_uuid(os->d->E.uuid)){
      os->rc = fsl_error_set(&os->error, FSL_RC_TYPE,
                             "Invalid UUID in E-card: %s",
                             os->d->E.uuid);
    }
    else if(!fsl_julian_to_iso8601(os->d->E.julian, ds, msPrecision)){
      os->rc = fsl_error_set(&os->error, FSL_RC_TYPE,
                             "Invalid Julian Day value in E-card.");
    }
    else{
      fsl_deck_append(os, "E %s %s\n", ds, os->d->E.uuid);
    }
  }
  return os->rc;
}

/* Appends the G card to os from os->d->G. */
static int fsl_deck_out_G( fsl_deck_out_state * os ){
  return fsl_deck_out_uuid(os, 'G', os->d->G);
}

/* Appends the H card to os from os->d->H. */
static int fsl_deck_out_H( fsl_deck_out_state * os ){
  if(os->d->H && os->d->I){
    return os->rc = fsl_error_set(&os->error, FSL_RC_SYNTAX,
                                  "Forum post may not have both H- and I-cards.");
  }
  return fsl_deck_out_letter_str( os, 'H', os->d->H, 1 );
}

/* Appends the I card to os from os->d->I. */
static int fsl_deck_out_I( fsl_deck_out_state * os ){
  if(os->d->I && os->d->H){
    return os->rc = fsl_error_set(&os->error, FSL_RC_SYNTAX,
                                  "Forum post may not have both H- and I-cards.");
  }
  return fsl_deck_out_uuid(os, 'I', os->d->I);
}


static int fsl_deck_out_F_one(fsl_deck_out_state *os,
                              fsl_card_F const * f){
  int rc;
  char hasOldName;
  char const * zPerm;
  assert(f);
  if(os->prevCard){
    int const cmp = fsl_strcmp(os->prevCard->name, f->name);
    if(0==cmp){
      return fsl_error_set(&os->error, FSL_RC_RANGE,
                           "Duplicate F-card name: %s",
                           f->name);
    }else if(cmp>0){
      return fsl_error_set(&os->error, FSL_RC_RANGE,
                           "Out-of-order F-card names: %s before %s",
                           os->prevCard->name, f->name);
    }
  }
  if(!fsl_is_simple_pathname(f->name, true)){
    return fsl_error_set(&os->error, FSL_RC_RANGE,
                         "Filename is invalid as F-card: %s",
                         f->name);
  }
  if(!f->uuid && !os->d->B.uuid){
    return fsl_error_set(&os->error, FSL_RC_MISUSE,
                         "Baseline manifests may not have F-cards "
                         "without UUIDs (file deletion entries). To "
                         "delete files, simply do not inject an F-card "
                         "for them. Delta manifests, however, require "
                         "NULL UUIDs for deletion entries! File: %s",
                         f->name);
  }

  rc = fsl_deck_fossilize(os, (unsigned char const *)f->name, -1);
  if(!rc) rc = fsl_deck_append(os, "F %b", os->scratch);
  if(!rc && f->uuid){
    assert(fsl_is_uuid(f->uuid));
    rc = fsl_deck_append( os, " %s", f->uuid);
    if(rc) return rc;
  }
  if(f->uuid){
    hasOldName = f->priorName && (0!=fsl_strcmp(f->name,f->priorName));
    switch(f->perm){
      case FSL_FILE_PERM_EXE: zPerm = " x"; break;
      case FSL_FILE_PERM_LINK: zPerm = " l"; break;
      default:
        /* When hasOldName, we have to inject an otherwise optional
           'w' to avoid an ambiguity. Or at least that's what the
           fossil F-card-generating code does.
        */
        zPerm = hasOldName ? " w" : ""; break;
    }
    if(*zPerm) rc = fsl_deck_append( os, "%s", zPerm);
    if(!rc && hasOldName){
      assert(*zPerm);
      rc = fsl_deck_fossilize(os, (unsigned char const *)f->priorName, -1);
      if(!rc) rc = fsl_deck_append( os, " %b", os->scratch);
    }
  }
  if(!rc) fsl_appendf_f_mf(os, "\n", 1);
  return os->rc;
}

static int fsl_deck_out_list_obj( fsl_deck_out_state * os,
                                  char letter,
                                  fsl_list const * li,
                                  fsl_list_visitor_f visitor){
  if(li->used && fsl_deck_out_tcheck(os, letter)){
    os->rc = fsl_list_visit( li, 0, visitor, os );
  }
  return os->rc;
}

static int fsl_deck_out_F( fsl_deck_out_state * os ){
  if(os->d->F.used && fsl_deck_out_tcheck(os, 'F')){
    uint32_t i;
    for(i=0; !os->rc && i <os->d->F.used; ++i){
      os->rc = fsl_deck_out_F_one(os, F_at(&os->d->F, i));
    }
  }
  return os->rc;
}


/**
    A comparison routine for qsort(3) which compares fsl_card_J
    instances in a lexical manner based on their names.  The order is
    important for card ordering in generated manifests.
 */
int fsl_qsort_cmp_J_cards( void const * lhs, void const * rhs ){
  fsl_card_J const * l = *((fsl_card_J const **)lhs);
  fsl_card_J const * r = *((fsl_card_J const **)rhs);
  /* Compare NULL as larger so that NULLs move to the right. That said,
     we aren't expecting any NULLs. */
  assert(l);
  assert(r);
  if(!l) return r ? 1 : 0;
  else if(!r) return -1;
  else{
    /* The '+' sorts before any legal field name bits (letters). */
    if(l->append != r->append) return r->append - l->append
      /* Footnote: that will break if, e.g. l->isAppend==2 and
         r->isAppend=1, or some such. Shame C89 doesn't have a true
         boolean.
      */;
    else return fsl_strcmp(l->field, r->field);
  }
}

/**
    fsl_list_visitor_f() impl for outputing J cards. obj must
    be a (fsl_card_J *).
 */
static int fsl_list_v_mf_output_card_J(void * obj, void * visitorState ){
  fsl_deck_out_state * os = (fsl_deck_out_state *)visitorState;
  fsl_card_J const * c = (fsl_card_J const *)obj;
  fsl_deck_fossilize( os, (unsigned char const *)c->field, -1 );
  if(!os->rc){
    fsl_deck_append(os, "J %s%b", c->append ? "+" : "", os->scratch);
    if(!os->rc){
      if(c->value && *c->value){
        fsl_deck_fossilize( os, (unsigned char const *)c->value, -1 );
        if(!os->rc){
          fsl_deck_append(os, " %b\n", os->scratch);
        }
      }else{
        fsl_deck_append(os, "\n");
      }
    }
  }
  return os->rc;
}

static int fsl_deck_out_J( fsl_deck_out_state * os ){
  return fsl_deck_out_list_obj(os, 'J', &os->d->J,
                               fsl_list_v_mf_output_card_J);
}

/* Appends the K card to os from os->d->K. */
static int fsl_deck_out_K( fsl_deck_out_state * os ){
  if(os->d->K && fsl_deck_out_tcheck(os, 'K')){
    if(!fsl_is_uuid(os->d->K)){
      os->rc = fsl_error_set(&os->error, FSL_RC_RANGE,
                             "Invalid UUID in K card.");
    }
    else{
      fsl_deck_append(os, "K %s\n", os->d->K);
    }
  }
  return os->rc;
}


/* Appends the L card to os from os->d->L. */
static int fsl_deck_out_L( fsl_deck_out_state * os ){
  return fsl_deck_out_letter_str(os, 'L', os->d->L, 1);
}

/* Appends the N card to os from os->d->N. */
static int fsl_deck_out_N( fsl_deck_out_state * os ){
  return fsl_deck_out_letter_str( os, 'N', os->d->N, 1 );
}

/**
    fsl_list_visitor_f() impl for outputing P cards. obj must
    be a (fsl_deck_out_state *) and obj->counter must be
    set to 0 before running the visit iteration.
 */
static int fsl_list_v_mf_output_card_P(void * obj, void * visitorState ){
  fsl_deck_out_state * os = (fsl_deck_out_state *)visitorState;
  char const * uuid = (char const *)obj;
  int const uLen = uuid ? fsl_is_uuid(uuid) : 0;
  if(!uLen){
    os->rc = fsl_error_set(&os->error, FSL_RC_RANGE,
                           "Invalid UUID in P card.");
  }
  else if(!os->counter++) fsl_appendf_f_mf(os, "P ", 2);
  else fsl_appendf_f_mf(os, " ", 1);
  /* Reminder: fsl_appendf_f_mf() updates os->rc. */
  if(!os->rc){
    fsl_appendf_f_mf(os, uuid, (fsl_size_t)uLen);
  }
  return os->rc;
}


static int fsl_deck_out_P( fsl_deck_out_state * os ){
  if(!fsl_deck_out_tcheck(os, 'P')) return os->rc;
  else if(os->d->P.used){
    os->counter = 0;
    os->rc = fsl_list_visit( &os->d->P, 0, fsl_list_v_mf_output_card_P, os );
    assert(os->counter);
    if(!os->rc) fsl_appendf_f_mf(os, "\n", 1);
  }
#if 1
  /* Arguable: empty P-cards are harmless but cosmetically unsightly. */
  else if(FSL_SATYPE_CHECKIN==os->d->type){
    /*
      Evil ugly hack, primarily for round-trip compatibility with
      manifest #1, which has an empty P card.

      fossil(1) ignores empty P-cards in all cases, and must continue
      to do so for backwards compatibility with rid #1 in all repos.

      Pedantic note: there must be no space between the 'P' and the
      newline.
    */
    fsl_deck_append(os, "P\n");
  }
#endif
  return os->rc;
}

/**
    A comparison routine for qsort(3) which compares fsl_card_Q
    instances in a lexical manner. The order is important for card
    ordering in generated manifests.
*/
static int qsort_cmp_Q_cards( void const * lhs, void const * rhs ){
  fsl_card_Q const * l = *((fsl_card_Q const **)lhs);
  fsl_card_Q const * r = *((fsl_card_Q const **)rhs);
  /* Compare NULL as larger so that NULLs move to the right. That said,
     we aren't expecting any NULLs. */
  assert(l);
  assert(r);
  if(!l) return r ? 1 : 0;
  else if(!r) return -1;
  else{
    /* Lexical sorting must account for the +/- characters, and a '+'
       sorts before '-', which is why this next part may seem
       backwards at first.
    */
    assert(l->type);
    assert(r->type);
    if(l->type<0 && r->type>0) return 1;
    else if(l->type>0 && r->type<0) return -1;
    else return fsl_strcmp(l->target, r->target);
  }
}

/**
    fsl_list_visitor_f() impl for outputing Q cards. obj must
    be a (fsl_deck_out_state *).
*/
static int fsl_list_v_mf_output_card_Q(void * obj, void * visitorState ){
  fsl_deck_out_state * os = (fsl_deck_out_state *)visitorState;
  fsl_card_Q const * cp = (fsl_card_Q const *)obj;
  char const prefix = (cp->type==FSL_CHERRYPICK_ADD)
    ? '+' : '-';
  assert(cp->type);
  assert(cp->target);
  if(cp->type != FSL_CHERRYPICK_ADD &&
     cp->type != FSL_CHERRYPICK_BACKOUT){
    os->rc = fsl_error_set(&os->error, FSL_RC_RANGE,
                           "Invalid type value in Q-card.");
  }else if(!fsl_card_is_legal(os->d->type, 'Q')){
    os->rc = fsl_error_set(&os->error, FSL_RC_TYPE,
                           "Q-card is not valid for deck type %s",
                           fsl_satype_cstr(os->d->type));
  }else if(!fsl_is_uuid(cp->target)){
    os->rc = fsl_error_set(&os->error, FSL_RC_RANGE,
                           "Invalid target UUID in Q-card: %s",
                           cp->target);
  }else if(cp->baseline){
    if(!fsl_is_uuid(cp->baseline)){
      os->rc = fsl_error_set(&os->error, FSL_RC_RANGE,
                             "Invalid baseline UUID in Q-card: %s",
                             cp->baseline);
    }else{
      fsl_deck_append(os, "Q %c%s %s\n", prefix, cp->target, cp->baseline);
    }
  }else{
    fsl_deck_append(os, "Q %c%s\n", prefix, cp->target);
  }
  return os->rc;
}

static int fsl_deck_out_Q( fsl_deck_out_state * os ){
  return fsl_deck_out_list_obj(os, 'Q', &os->d->Q,
                               fsl_list_v_mf_output_card_Q);
}

/**
    Appends the R card from os->d->R to os.
 */
static int fsl_deck_out_R( fsl_deck_out_state * os ){
  if(os->d->R && fsl_deck_out_tcheck(os, 'R')){
    if((FSL_STRLEN_MD5!=fsl_strlen(os->d->R))
            || !fsl_validate16(os->d->R, FSL_STRLEN_MD5)){
      os->rc = fsl_error_set(&os->error, FSL_RC_RANGE,
                             "Malformed MD5 in R-card.");
    }
    else{
      fsl_deck_append(os, "R %s\n", os->d->R);
    }
  }
  return os->rc;
}

/**
    fsl_list_visitor_f() impl for outputing T cards. obj must
    be a (fsl_deck_out_state *).
 */
static int fsl_list_v_mf_output_card_T(void * obj, void * visitorState ){
  fsl_deck_out_state * os = (fsl_deck_out_state *)visitorState;
  fsl_card_T * t = (fsl_card_T *)obj;
  char prefix = 0;
  switch(os->d->type){
    case FSL_SATYPE_TECHNOTE:
      if( t->uuid ){
        return os->rc = fsl_error_set(&os->error, FSL_RC_SYNTAX,
                                      "Non-self-referential T-card is not "
                                      "permitted in a technote.");
      }else if(FSL_TAGTYPE_ADD!=t->type){
        return os->rc = fsl_error_set(&os->error, FSL_RC_SYNTAX,
                                      "Non-ADD T-card is not permitted "
                                      "in a technote.");
      }
      break;
    case FSL_SATYPE_CONTROL:
      if( !t->uuid ){
        return os->rc = fsl_error_set(&os->error, FSL_RC_SYNTAX,
                                      "Self-referential T-card is not "
                                      "permitted in a control artifact.");
      }
      break;
    default:
      break;
  }
  /* Determine the prefix character... */
  switch(t->type){
    case FSL_TAGTYPE_CANCEL: prefix = '-'; break;
    case FSL_TAGTYPE_ADD: prefix = '+'; break;
    case FSL_TAGTYPE_PROPAGATING: prefix = '*'; break;
    default:
      return os->rc = fsl_error_set(&os->error, FSL_RC_TYPE,
                                    "Invalid tag type #%d in T-card.",
                                    t->type);
  }
  if(!t->name || !*t->name){
    return os->rc = fsl_error_set(&os->error, FSL_RC_SYNTAX,
                                  "T-card name may not be empty.");
  }else if(fsl_validate16(t->name, fsl_strlen(t->name))){
    return os->rc = fsl_error_set(&os->error, FSL_RC_SYNTAX,
                                  "T-card name may not be hexadecimal.");
  }else if(t->uuid && !fsl_is_uuid(t->uuid)){
      return os->rc = fsl_error_set(&os->error, FSL_RC_SYNTAX,
                                    "Malformed UUID in T-card: %s",
                                    t->uuid);
  }
  /*
     Fossilize and output the prefix, name, and uuid, or a '*' if no
     uuid is set (which is only legal when tagging the current
     artifact, as '*' is a placeholder for the current artifact's
     UUID, which is not yet known).
  */
  fsl_buffer_reuse(os->scratch);
  fsl_deck_fossilize(os, (unsigned const char *)t->name, -1);
  if(os->rc) return os->rc;
  os->rc = fsl_deck_append(os, "T %c%s %s", prefix,
                           (char const*)os->scratch->mem,
                           t->uuid ? t->uuid : "*");
  if(os->rc) return os->rc;
  if(/*(t->type != FSL_TAGTYPE_CANCEL) &&*/t->value && *t->value){
    /* CANCEL tags historically don't store a value but
       the spec doesn't disallow it and they are harmless
       for (aren't used by) fossil(1). */
    fsl_deck_fossilize(os, (unsigned char const *)t->value, -1);
    if(!os->rc) fsl_appendf_f_mf(os, " ", 1);
    if(!os->rc) fsl_appendf_f_mf(os, (char const*)os->scratch->mem,
                                 (fsl_int_t)os->scratch->used);
  }
  if(!os->rc){
    fsl_appendf_f_mf(os, "\n", 1);
  }
  return os->rc;
}


char fsl_tag_prefix_char( fsl_tagtype_e t ){
  switch(t){
    case FSL_TAGTYPE_CANCEL: return '-';
    case FSL_TAGTYPE_ADD: return '+';
    case FSL_TAGTYPE_PROPAGATING: return '*';
    default:
      return 0;
  }
}

/**
    A comparison routine for qsort(3) which compares fsl_card_T
    instances in a lexical manner based on (type, name, uuid, value).
    The order of those is important for card ordering in generated
    manifests. Interestingly, CANCEL tags (with a '-' prefix) sort
    last, meaning it is possible to cancel a tag set in the same
    manifest because crosslinking processes them in the order given
    (which will be lexical order for all legal manifests).

    Reminder: lhs and rhs must be (fsl_card_T**), as we use this to
    qsort() such lists. When using it to compare two tags, make sure
    to pass ptr-to-ptr.
*/
static int fsl_card_T_cmp( void const * lhs, void const * rhs ){
  fsl_card_T const * l = *((fsl_card_T const **)lhs);
  fsl_card_T const * r = *((fsl_card_T const **)rhs);
  /* Compare NULL as larger so that NULLs move to the right. That said,
     we aren't expecting any NULLs. */
  assert(l);
  assert(r);
  if(!l) return r ? 1 : 0;
  else if(!r) return -1;
  else if(l->type != r->type){
    char const lc = fsl_tag_prefix_char(l->type);
    char const rc = fsl_tag_prefix_char(r->type);
    return (lc<rc) ? -1 : 1;
  }else{
    int rc = fsl_strcmp(l->name, r->name);
    if(rc) return rc;
    else {
      rc = fsl_uuidcmp(l->uuid, r->uuid);
      return rc
        ? rc
        : fsl_strcmp(l->value, r->value);
    }
  }
}

/**
   Confirms that any T-cards in d are properly sorted. If not,
   returns non-0. If err is not NULL, it is updated with a
   description of the problem.

   Possibly fixme one day: this code permits that the same tag/target
   combination may be added or removed, or added as a normal and
   propagating tag, in the same deck. Though that's not technically
   disallowed, we "should" disallow it. That requires a more thorough
   scan of the cards, though.
*/
static int fsl_deck_T_verify_order( fsl_deck const * d, fsl_error * err ){
  if(d->T.used<2) return 0;
  else{
    fsl_size_t i = 0, j;
    int rc = 0;
    fsl_card_T const * tag;
    fsl_card_T const * prev = NULL;
    for( i = 0; i < d->T.used; ++i, prev = tag, rc = 0){
      tag = (fsl_card_T const *)d->T.list[i];
      if(prev){
        if( (rc = fsl_card_T_cmp(&prev, &tag)) >= 0 ){
          if(!err) rc = FSL_RC_SYNTAX;
          else{
            rc = rc
              ? fsl_error_set(err, FSL_RC_SYNTAX,
                              "Invalid T-card order: "
                              "[%c%s] must precede [%c%s]",
                              fsl_tag_prefix_char(prev->type),
                              prev->name,
                              fsl_tag_prefix_char(tag->type),
                              tag->name)
              : fsl_error_set(err, FSL_RC_SYNTAX,
                              "Duplicate T-card: %c%s",
                              fsl_tag_prefix_char(prev->type),
                              prev->name)
              ;
          }
          break;
        }
      }
    }
    /**
       And now, for bonus points: disallow the same tag name/artifact
       combination appearing twice in the deck. Though that's not
       explicitly disallowed by the fossil specs, we "should" disallow
       it. That requires a more thorough scan of the cards, though.

       This logic is NOT in fossil, and though we don't have any such
       tags in the fossil repo, we may have to disable this for
       compatibility's sake. OTOH, we only check this when outputing
       manifests, and we never (aside from testing) have to output
       manifests which were generated by fossil. Thus... this only
       triggers (except for some tests) on manifests generated by
       libfossil, so we can justify having it.
    */
    for( i=0; !rc && i < d->T.used; ++i ){
      fsl_card_T const * t1 = (fsl_card_T const *)d->T.list[i];
      for( j = 0; j < d->T.used; ++j ){
        if(i==j) continue;
        fsl_card_T const * t2 = (fsl_card_T const *)d->T.list[j];
        if(0==fsl_strcmp(t1->name, t2->name)
           && ((!t1->uuid && !t2->uuid)
               || 0==fsl_strcmp(t1->uuid, t2->uuid))){
              rc = fsl_error_set(err, FSL_RC_SYNTAX,
                                 "An artifact may not contain the same "
                                 "T-card name and target artifact "
                                 "multiple times: "
                                 "name=%s target=%s",
                                 t1->name, t1->uuid ? t1->uuid : "*");
              break;
        }
      }
    }
    return rc;
  }
}

/* Appends the T cards to os from os->d->T. */
static int fsl_deck_out_T( fsl_deck_out_state * os ){
  os->rc = fsl_deck_T_verify_order( os->d, &os->error);
  return os->rc
    ? os->rc
    : fsl_deck_out_list_obj(os, 'T', &os->d->T,
                            fsl_list_v_mf_output_card_T);
}

/* Appends the U card to os from os->d->U. */
static int fsl_deck_out_U( fsl_deck_out_state * os ){
  return fsl_deck_out_letter_str(os, 'U', os->d->U, 1);
}

/* Appends the W card to os from os->d->W. */
static int fsl_deck_out_W( fsl_deck_out_state * os ){
  if(os->d->W.used && fsl_deck_out_tcheck(os, 'W')){
    fsl_deck_append(os, "W %"FSL_SIZE_T_PFMT"\n%b\n",
                    (fsl_size_t)os->d->W.used,
                    &os->d->W );
  }
  return os->rc;
}


/* Appends the Z card to os from os' accummulated md5 hash. */
static int fsl_deck_out_Z( fsl_deck_out_state * os ){
  unsigned char digest[16];
  char md5[FSL_STRLEN_MD5+1];
  fsl_md5_final(&os->md5, digest);
  fsl_md5_digest_to_base16(digest, md5);
  assert(!md5[32]);
  os->md5.isInit = 0 /* Keep further output from updating the MD5 */;
  return fsl_deck_append(os, "Z %.*s\n", FSL_STRLEN_MD5, md5);
}

static int qsort_cmp_strings( void const * lhs, void const * rhs ){
  char const * l = *((char const **)lhs);
  char const * r = *((char const **)rhs);
  return fsl_strcmp(l,r);
}

static int fsl_list_v_mf_output_card_M(void * obj, void * visitorState ){
  fsl_deck_out_state * os = (fsl_deck_out_state *)visitorState;
  char const * m = (char const *)obj;
  return fsl_deck_append(os, "M %s\n", m);
}

static int fsl_deck_output_cluster( fsl_deck_out_state * os ){
  if(!os->d->M.used){
    os->rc = fsl_error_set(&os->error, FSL_RC_RANGE,
                           "M-card list may not be empty.");
  }else{
    fsl_deck_out_list_obj(os, 'M', &os->d->M,
                          fsl_list_v_mf_output_card_M);
  }
  return os->rc;
}


/* Helper for fsl_deck_output_CATYPE() */
#define DOUT(LETTER) rc = fsl_deck_out_##LETTER(os); \
  if(rc || os->rc) return os->rc ? os->rc : rc

static int fsl_deck_output_control( fsl_deck_out_state * os ){
  int rc;
  /* Reminder: cards must be output in strict lexical order. */
  DOUT(D);
  DOUT(T);
  DOUT(U);
  return os->rc;
}

static int fsl_deck_output_event( fsl_deck_out_state * os ){
  int rc = 0;
  /* Reminder: cards must be output in strict lexical order. */
  DOUT(C);
  DOUT(D);
  DOUT(E);
  DOUT(N);
  DOUT(P);
  DOUT(T);
  DOUT(U);
  DOUT(W);
  return os->rc;
}

static int fsl_deck_output_mf( fsl_deck_out_state * os ){
  int rc = 0;
  /* Reminder: cards must be output in strict lexical order. */
  DOUT(B);
  DOUT(C);
  DOUT(D);
  DOUT(F);
  DOUT(K);
  DOUT(L);
  DOUT(N);
  DOUT(P);
  DOUT(Q);
  DOUT(R);
  DOUT(T);
  DOUT(U);
  DOUT(W);
  return os->rc;
}


static int fsl_deck_output_ticket( fsl_deck_out_state * os ){
  int rc;
  /* Reminder: cards must be output in strict lexical order. */
  DOUT(D);
  DOUT(J);
  DOUT(K);
  DOUT(U);
  return os->rc;
}

static int fsl_deck_output_wiki( fsl_deck_out_state * os ){
  int rc;
  /* Reminder: cards must be output in strict lexical order. */
  DOUT(C);
  DOUT(D);
  DOUT(L);
  DOUT(N);
  DOUT(P);
  DOUT(U);
  DOUT(W);
  return os->rc;
}

static int fsl_deck_output_attachment( fsl_deck_out_state * os ){
  int rc = 0;
  /* Reminder: cards must be output in strict lexical order. */
  DOUT(A);
  DOUT(C);
  DOUT(D);
  DOUT(N);
  DOUT(U);
  return os->rc;
}

static int fsl_deck_output_forumpost( fsl_deck_out_state * os ){
  int rc;
  /* Reminder: cards must be output in strict lexical order. */
  DOUT(D);
  DOUT(G);
  DOUT(H);
  DOUT(I);
  DOUT(N);
  DOUT(P);
  DOUT(U);
  DOUT(W);
  return os->rc;
}

/**
    Only for testing/debugging purposes, as it allows constructs which
    are not semantically legal and are CERTAINLY not legal to stuff in
    the database.
 */
static int fsl_deck_output_any( fsl_deck_out_state * os ){
  int rc = 0;
  /* Reminder: cards must be output in strict lexical order. */
  DOUT(B);
  DOUT(C);
  DOUT(D);
  DOUT(E);
  DOUT(F);
  DOUT(J);
  DOUT(K);
  DOUT(L);
  DOUT(N);
  DOUT(P);
  DOUT(Q);
  DOUT(R);
  DOUT(T);
  DOUT(U);
  DOUT(W);
  return os->rc;
}

#undef DOUT


int fsl_deck_unshuffle( fsl_deck * d, bool calculateRCard ){
  fsl_list * li;
  int rc = 0;
  if(!d || !d->f) return FSL_RC_MISUSE;
  fsl_cx_err_reset(d->f);
#define SORT(CARD,CMP) li = &d->CARD; fsl_list_sort(li, CMP)
  SORT(J,fsl_qsort_cmp_J_cards);
  SORT(M,qsort_cmp_strings);
  SORT(Q,qsort_cmp_Q_cards);
  SORT(T,fsl_card_T_cmp);
#undef SORT
  if(FSL_SATYPE_CHECKIN!=d->type){
    assert(!fsl_card_is_legal(d->type,'R'));
    assert(!fsl_card_is_legal(d->type,'F'));
  }else{
    assert(fsl_card_is_legal(d->type, 'R') && "in-lib unit testing");
    if(calculateRCard){
      rc = fsl_deck_R_calc(d) /* F-card list is sorted there */;
    }else{
      fsl_deck_F_sort(d);
      if(!d->R){
        rc = fsl_deck_R_set(d,
                            (d->F.used || d->B.uuid || d->P.used)
                            ? NULL
                            : FSL_MD5_INITIAL_HASH)
          /* Special case: for manifests with no (B,F,P)-cards we inject
             the initial-state R-card, analog to the initial checkin
             (RID 1). We need one of (B,F,P,R) to unambiguously identify
             a MANIFEST from a CONTROL, but RID 1 has an empty P-card,
             no F-cards, and no B-card, so it _needs_ an R-card in order
             to be unambiguously a Manifest. That said, that ambiguity
             is/would be harmless in practice because CONTROLs go
             through most of the same crosslinking processes as
             MANIFESTs (the ones which are important for this purpose,
             anyway).
          */;
      }
    }
  }
  return rc;
}

int fsl_deck_output( fsl_deck * d, fsl_output_f out, void * outputState ){
  static const bool allowTypeAny = false
    /* Only enable for debugging/testing. Allows outputing decks of
       type FSL_SATYPE_ANY, which bypasses some validation checks and
       may trigger other validation assertions. And may allow you to
       inject garbage into the repo. So be careful.
    */;

  fsl_deck_out_state OS = fsl_deck_out_state_empty;
  fsl_deck_out_state * const os = &OS;
  fsl_cx * const f = d->f;
  int rc = 0;
  if(!f || !out) return FSL_RC_MISUSE;
  else if(FSL_SATYPE_ANY==d->type){
    if(!allowTypeAny){
      return fsl_cx_err_set(d->f, FSL_RC_TYPE,
                            "Artifact type ANY cannot be"
                            "output unless it is enabled in this "
                            "code (it's dangerous).");
    }
    /* fall through ... */
  }
  rc = fsl_deck_unshuffle(d,
                          (FSL_CX_F_CALC_R_CARD & f->flags)
                          ? ((d->F.used && !d->R) ? 1 : 0)
                          : 0);
  /* ^^^^ unshuffling might install an R-card, so we have to
     do that before checking whether all required cards are
     set... */  
  if(rc) return rc;
  else if(!fsl_deck_has_required_cards(d)){
    return FSL_RC_SYNTAX;
  }

  os->d = d;
  os->out = out;
  os->outState = outputState;
  os->scratch = fsl_cx_scratchpad(f);
  switch(d->type){
    case FSL_SATYPE_CLUSTER:
      rc = fsl_deck_output_cluster(os);
      break;
    case FSL_SATYPE_CONTROL:
      rc = fsl_deck_output_control(os);
      break;
    case FSL_SATYPE_EVENT:
      rc = fsl_deck_output_event(os);
      break;
    case FSL_SATYPE_CHECKIN:
      rc = fsl_deck_output_mf(os);
      break;
    case FSL_SATYPE_TICKET:
      rc = fsl_deck_output_ticket(os);
      break;
    case FSL_SATYPE_WIKI:
      rc = fsl_deck_output_wiki(os);
      break;
    case FSL_SATYPE_ANY:
      assert(allowTypeAny);
      rc = fsl_deck_output_any(os);
      break;
    case FSL_SATYPE_ATTACHMENT:
      rc = fsl_deck_output_attachment(os);
      break;
    case FSL_SATYPE_FORUMPOST:
      rc = fsl_deck_output_forumpost(os);
      break;
    default:
      rc = fsl_cx_err_set(f, FSL_RC_TYPE,
                          "Invalid/unhandled deck type (#%d).",
                          d->type);
      goto end;
  }
  if(!rc){
    rc = fsl_deck_out_Z( os );
  }
  end:
  fsl_cx_scratchpad_yield(f, os->scratch);
  if(os->rc && os->error.code){
    fsl_error_move(&os->error, &f->error);
  }
  fsl_error_clear(&os->error);
  return os->rc ? os->rc : rc;
}

/* Timestamps might be adjusted slightly to ensure that checkins appear
   on the timeline in chronological order.  This is the maximum amount
   of the adjustment window, in days.
*/
#define AGE_FUDGE_WINDOW      (2.0/86400.0)       /* 2 seconds */

/* This is increment (in days) by which timestamps are adjusted for
   use on the timeline.
*/
#define AGE_ADJUST_INCREMENT  (25.0/86400000.0)   /* 25 milliseconds */

/**
   Adds a record in the pending_xlink temp table, to be processed
   when crosslinking is completed. Returns 0 on success, non-0 for
   db error.
*/
static int fsl_deck_crosslink_add_pending(fsl_cx * f, char cType, fsl_uuid_cstr uuid){
  int rc = 0;
  assert(f->cache.isCrosslinking);
  rc = fsl_db_exec(f->dbMain,
                   "INSERT OR IGNORE INTO pending_xlink VALUES('%c%q')",
                   cType, uuid);
  return fsl_cx_uplift_db_error2(f, 0, rc);
}


/** @internal
   
    Add a single entry to the mlink table.  Also add the filename to
    the filename table if it is not there already.
   
    Parameters:
   
    pmid: Record for parent manifest. Use 0 to indicate no parent.

    zFromUuid: UUID for the content in parent (the new ==mlink.pid). 0
    or "" to add file.

    mid: The record ID of the manifest
   
    zToUuid:UUID for the mlink.fid. "" to delete
   
    zFilename: Filename
   
    zPrior: Previous filename. NULL if unchanged 
   
    isPublic:True if mid is not a private manifest
   
    isPrimary: true if pmid is the primary parent of mid.

    mperm: permissions
 */
static
int fsl_mlink_add_one( fsl_cx * f,
                       fsl_id_t pmid, fsl_uuid_cstr zFromUuid,
                       fsl_id_t mid, fsl_uuid_cstr zToUuid,
                       char const * zFilename,
                       char const * zPrior,
                       bool isPublic,
                       bool isPrimary,
                       fsl_fileperm_e mperm){
  fsl_id_t fnid, pfnid, pid, fid;
  fsl_db * db = fsl_cx_db_repo(f);
  fsl_stmt * s1 = NULL;
  int rc;
  bool doInsert = false;
  assert(f);
  assert(db);
  assert(db->beginCount>0);
  //MARKER(("%s() pmid=%d mid=%d\n", __func__, (int)pmid, (int)mid));

  rc = fsl_repo_filename_fnid2(f, zFilename, &fnid, 1);
  if(rc) return rc;
  if( zPrior && *zPrior ){
    rc = fsl_repo_filename_fnid2(f, zPrior, &pfnid, 1);
    if(rc) return rc;
  }else{
    pfnid = 0;
  }
  if( zFromUuid && *zFromUuid ){
    pid = fsl_uuid_to_rid2(f, zFromUuid, FSL_PHANTOM_PUBLIC);
    if(pid<0){
      assert(f->error.code);
      return f->error.code;
    }
    assert(pid>0);
  }else{
    pid = 0;
  }

  if( zToUuid && *zToUuid ){
    fid = fsl_uuid_to_rid2(f, zToUuid, FSL_PHANTOM_PUBLIC);
    if(fid<0){
      assert(f->error.code);
      return f->error.code;
    }else if( isPublic ){
      rc = fsl_content_make_public(f, fid);
      if(rc) return rc;
    }
  }else{
    fid = 0;
  }

  if(isPrimary){
    doInsert = true;
  }else{
    fsl_stmt * sInsCheck = 0;
    rc = fsl_db_prepare_cached(db, &sInsCheck,
                               "SELECT 1 FROM mlink WHERE "
                               "mid=? AND fnid=? AND NOT isaux"
                               "/*%s()*/",__func__);
    if(rc){
      rc = fsl_cx_uplift_db_error(f, db);
      goto end;
    }
    fsl_stmt_bind_id(sInsCheck, 1, mid);
    fsl_stmt_bind_id(sInsCheck, 2, fnid);
    rc = fsl_stmt_step(sInsCheck);
    fsl_stmt_cached_yield(sInsCheck);
    doInsert = (FSL_RC_STEP_ROW==rc) ? true : false;
    rc = 0;
  }
  if(doInsert){
    rc = fsl_db_prepare_cached(db, &s1,
                               "INSERT INTO mlink("
                               "mid,fid,pmid,pid,"
                               "fnid,pfnid,mperm,isaux"
                               ")VALUES("
                               ":m,:f,:pm,:p,:n,:pfn,:mp,:isaux"
                               ")"
                               "/*%s()*/",__func__);
    if(!rc){
      fsl_stmt_bind_id_name(s1, ":m", mid);
      fsl_stmt_bind_id_name(s1, ":f", fid);
      fsl_stmt_bind_id_name(s1, ":pm", pmid);
      fsl_stmt_bind_id_name(s1, ":p", pid);
      fsl_stmt_bind_id_name(s1, ":n", fnid);
      fsl_stmt_bind_id_name(s1, ":pfn", pfnid);
      fsl_stmt_bind_id_name(s1, ":mp", mperm);    
      fsl_stmt_bind_int32_name(s1, ":isaux", isPrimary ? 0 : 1);    
      rc = fsl_stmt_step(s1);
      fsl_stmt_cached_yield(s1);
      if(FSL_RC_STEP_DONE==rc){
        rc = 0;
      }else{
        fsl_cx_uplift_db_error(f, db);
      }
    }
  }
  if(!rc && pid>0 && fid){
    /* Reminder to self: this costs almost 1ms per checkin in very
       basic tests with 2003 checkins on my NUC unit. */
    rc = fsl_content_deltify(f, pid, fid, 0);
  }
  end:
  return rc;  
}

/**
    Do a binary search to find a file in d->F.list.  
   
    As an optimization, guess that the file we seek is at index
    d->F.cursor.  That will usually be the case.  If it is not found
    there, then do the actual binary search.

    Update d->F.cursor to be the index of the file that is found.

    If d->f is NULL then this perform a case-sensitive search,
    otherwise it uses case-sensitive or case-insensitive,
    depending on f->cache.caseInsensitive.    

    If the 3rd argument is not NULL and non-NULL is returned then
    *atNdx gets set to the d->F.list index of the resulting object.
    If NULL is returned, *atNdx is not modified.

    Reminder to self: if this requires a non-const deck (and it does
    right now) then the whole downstream chain will require a
    non-const instance or they'll have to make local copies to make
    the manipulation of d->F.cursor legal (but that would break
    following of baselines without yet more trickery).

    Reminder to self:

    Fossil(1) added another parameter to this since it was ported,
    indicating whether only an exact match or the "closest match" is
    acceptable, but currently (2021-03-10) only the fusefs module uses
    the closest-match option. It's a trivial code change but currently
    looks like YAGNI.
*/
static fsl_card_F * fsl_deck_F_seek_base(fsl_deck * d,
                                         char const * zName,
                                         uint32_t * atNdx ){
  /* Maintenance reminder: this algo relies on the various
     counters being signed. */
  fsl_int_t lwr, upr;
  int c;
  fsl_int_t i;
  assert(d);
  assert(zName && *zName);
  if(!d->F.used) return NULL;
  else if(FSL_CARD_F_LIST_NEEDS_SORT & d->F.flags){
    fsl_card_F_list_sort(&d->F);
  }
#define FCARD(NDX) F_at(&d->F, (NDX))
  lwr = 0;
  upr = d->F.used-1;
  if( d->F.cursor>=lwr && d->F.cursor<upr ){
    c = (d->f && d->f->cache.caseInsensitive)
      ? fsl_stricmp(FCARD(d->F.cursor+1)->name, zName)
      : fsl_strcmp(FCARD(d->F.cursor+1)->name, zName);
    if( c==0 ){
      if(atNdx) *atNdx = (uint32_t)d->F.cursor+1;
      return FCARD(++d->F.cursor);
    }else if( c>0 ){
      upr = d->F.cursor;
    }else{
      lwr = d->F.cursor+1;
    }
  }
  while( lwr<=upr ){
    i = (lwr+upr)/2;
    c = (d->f && d->f->cache.caseInsensitive)
      ? fsl_stricmp(FCARD(i)->name, zName)
      : fsl_strcmp(FCARD(i)->name, zName);
    if( c<0 ){
      lwr = i+1;
    }else if( c>0 ){
      upr = i-1;
    }else{
      d->F.cursor = i;
      if(atNdx) *atNdx = (uint32_t)i;
      return FCARD(i);
    }
  }
  return NULL;
#undef FCARD
}

fsl_card_F * fsl_deck_F_seek(fsl_deck * const d, const char *zName){
  fsl_card_F *pFile;
  assert(d);
  assert(zName && *zName);
  if(!d || (FSL_SATYPE_CHECKIN!=d->type) || !zName || !*zName
     || !d->F.used) return NULL;
  pFile = fsl_deck_F_seek_base(d, zName, NULL);
  if( !pFile &&
      (d->B.baseline /* we have a baseline or... */
       || (d->f && d->B.uuid) /* we can load the baseline */
       )){
        /* Check baseline manifest...

           Sidebar: while the delta manifest model outwardly appears
           to support recursive delta manifests, fossil(1) does not
           use them and there would seem to be little practical use
           for them (no notable size benefit for the majority of
           cases), so we're not recursing here.
         */
    int const rc = d->B.baseline ? 0 : fsl_deck_baseline_fetch(d);
    if(rc){
      assert(d->f->error.code);
    }else if( d->B.baseline ){
      assert(d->B.baseline->f && "How can this happen?");
      assert((d->B.baseline->f == d->f) &&
             "Universal laws are out of balance.");
      pFile = fsl_deck_F_seek_base(d->B.baseline, zName, NULL);
      if(pFile){
        assert(pFile->uuid &&
               "Per fossil-dev thread with DRH on 20140422, "
               "baselines never have removed files.");
      }
    }
  }
  return pFile;
}

fsl_card_F const * fsl_deck_F_search(fsl_deck *d, const char *zName){
  assert(d);
  return fsl_deck_F_seek(d, zName);
}

int fsl_deck_F_set( fsl_deck * d, char const * zName,
                    char const * uuid,
                    fsl_fileperm_e perms, 
                    char const * priorName){
  uint32_t fcNdx = 0;
  fsl_card_F * fc = 0;
  if(d->uuid || d->rid>0){
    return fsl_cx_err_set(d->f, FSL_RC_MISUSE,
                          "%s() cannot be applied to a saved deck.",
                          __func__);
  }else if(!fsl_deck_check_type(d, 'F')){
    return d->f->error.code;
  }
  fc = fsl_deck_F_seek_base(d, zName, &fcNdx);
  if(!uuid){
    if(fc){
      fsl_card_F_list_remove(&d->F, fcNdx);
      return 0;
    }else{
      return FSL_RC_NOT_FOUND;
    }
  }else if(!fsl_is_uuid(uuid)){
    return fsl_cx_err_set(d->f, FSL_RC_RANGE,
                          "Invalid UUID for F-card.");
  }
  if(fc){
    /* Got a match. Replace its contents. */
    char * n = 0;
    if(!fc->deckOwnsStrings){
      /* We can keep fc->name but need a tiny bit of hoop-jumping
         to do so. */
      n = fc->name;
      fc->name = 0;
    }
    fsl_card_F_clean(fc);
    assert(!fc->deckOwnsStrings);
    if(!(fc->name = n ? n : fsl_strdup(zName))) return FSL_RC_OOM;
    if(!(fc->uuid = fsl_strdup(uuid))) return FSL_RC_OOM;
    if(priorName && *priorName){
      if(!fsl_is_simple_pathname(priorName, 1)){
        return fsl_cx_err_set(d->f, FSL_RC_RANGE,
                              "Invalid priorName for F-card "
                              "(simple form required): %s", priorName);
      }else if(!(fc->priorName = fsl_strdup(priorName))){
        return FSL_RC_OOM;
      }
    }
    fc->perm = perms;
    return 0;
  }else{
    return fsl_deck_F_add(d, zName, uuid, perms, priorName);
  }
}

int fsl_deck_F_set_content( fsl_deck * d, char const * zName,
                            fsl_buffer const * src,
                            fsl_fileperm_e perm, 
                            char const * priorName){
  fsl_uuid_str zHash = 0;
  fsl_id_t rid = 0;
  fsl_id_t prevRid = 0;
  int rc = 0;
  if(d->uuid || d->rid>0){
    return fsl_cx_err_set(d->f, FSL_RC_MISUSE,
                          "%s() cannot be applied to a saved deck.",
                          __func__);
  }else if(!fsl_cx_transaction_level(d->f)){
    return fsl_cx_err_set(d->f, FSL_RC_MISUSE,
                          "%s() requires that a transaction is active.",
                          __func__);
  }
  rc = fsl_repo_blob_lookup(d->f, src, &rid, &zHash);
  if(rc && FSL_RC_NOT_FOUND!=rc) goto end;
  assert(zHash);
  if(!rid){
    fsl_card_F const * fc;
    /* This is new content. Save it, then see if we have a previous version
       to delta against this one. */
    rc = fsl_content_put_ex(d->f, src, zHash, 0, 0, false, &rid);
    if(rc) goto end;
    fc = fsl_deck_F_seek(d, zName);
    if(fc){
      prevRid = fsl_uuid_to_rid(d->f, fc->uuid);
      if(prevRid<0) goto end;
      else if(!prevRid){
        assert(!"cannot happen");
        rc = fsl_cx_err_set(d->f, FSL_RC_NOT_FOUND,
                            "Cannot find RID of file content %s [%s]\n",
                            fc->name, fc->uuid);
        goto end;
      }
      rc = fsl_content_deltify(d->f, prevRid, rid, false);
      if(rc) goto end;
    }
  }
  rc = fsl_deck_F_set(d, zName, zHash, perm, priorName);
  end:
  fsl_free(zHash);
  return rc;
}

void fsl_deck_clean_cards(fsl_deck * d, char const * letters){
  char const * c = letters
    ? letters
    : "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  for( ; *c; ++c ){
    switch(*c){
      case 'A': fsl_deck_clean_A(d); break;
      case 'B': fsl_deck_clean_B(d); break;
      case 'C': fsl_deck_clean_C(d); break;
      case 'D': d->D = 0.0; break;
      case 'E': fsl_deck_clean_E(d); break;
      case 'F': fsl_deck_clean_F(d); break;
      case 'G': fsl_deck_clean_G(d); break;
      case 'H': fsl_deck_clean_H(d); break;
      case 'I': fsl_deck_clean_I(d); break;
      case 'J': fsl_deck_clean_J(d,true); break;
      case 'K': fsl_deck_clean_K(d); break;
      case 'L': fsl_deck_clean_L(d); break;
      case 'M': fsl_deck_clean_M(d); break;
      case 'N': fsl_deck_clean_N(d); break;
      case 'P': fsl_deck_clean_P(d); break;
      case 'Q': fsl_deck_clean_Q(d); break;
      case 'R': fsl_deck_clean_R(d); break;
      case 'T': fsl_deck_clean_T(d); break;
      case 'U': fsl_deck_clean_U(d); break;
      case 'W': fsl_deck_clean_W(d); break;
      default: break;
    }
  }
}

int fsl_deck_derive(fsl_deck * d){
  int rc = 0;
  if(!d->uuid || d->rid<=0) return FSL_RC_MISUSE;
  else if(FSL_SATYPE_CHECKIN!=d->type) return FSL_RC_TYPE;
  fsl_deck_clean_P(d);
  rc = fsl_list_append(&d->P, d->uuid);
  if(rc) return rc;
  d->uuid = NULL;
  d->rid = 0;
  fsl_deck_clean_cards(d, "ACDEGHIJKLMNQRTUW");
  while(d->B.uuid){
    /* This is a delta manifest. Convert this deck into a baseline by
       build a new, complete F-card list. */
    fsl_card_F const * fc;
    fsl_card_F_list flist = fsl_card_F_list_empty;
    uint32_t fCount = 0;
    rc = fsl_deck_F_rewind(d);
    if(rc) return rc;
    while( 0==(rc=fsl_deck_F_next(d, &fc)) && fc ){
      ++fCount;
    }
    rc = fsl_deck_F_rewind(d);
    assert(0==rc
           && "fsl_deck_F_rewind() cannot fail after initial call.");
    assert(0==d->F.cursor);
    assert(0==d->B.baseline->F.cursor);
    rc = fsl_card_F_list_reserve(&flist, fCount);
    if(rc) break;
    while( 1 ){
      rc = fsl_deck_F_next(d, &fc);
      if(rc || !fc) break;
      fsl_card_F * const fNew = fsl_card_F_list_push(&flist);
      assert(fc->uuid);
      assert(fc->name);
      /* We must copy these strings because their ownership is
         otherwise unmanageable. e.g. they might live in d->content
         or d->B.baseline->content. */
      if(!(fNew->name = fsl_strdup(fc->name))
         || !(fNew->uuid = fsl_strdup(fc->uuid))){
        /* Reminder: we do not want/need to copy fc->priorName. Those
           renames were already applied in the parent checkin. */
        rc = FSL_RC_OOM;
        break;
      }
      fNew->perm = fc->perm;
    }
    fsl_deck_clean_B(d);
    fsl_deck_clean_F(d);
    if(rc) fsl_card_F_list_finalize(&flist);
    else d->F = flist/*transfer ownership*/;
    break;
  }
  return rc;
}

/**
    Returns true if repo contains an mlink entry where mid=rid, else
    false.
*/
static bool fsl_repo_has_mlink_mid( fsl_db * repo, fsl_id_t rid ){
#if 0
  return fsl_db_exists(repo,
                       "SELECT 1 FROM mlink WHERE mid=%"FSL_ID_T_PFMT,
                       rid);
#else
  fsl_stmt * st = NULL;
  bool gotone = false;
  int rc = fsl_db_prepare_cached(repo, &st,
                                 "SELECT 1 FROM mlink WHERE mid=?"
                                 "/*%s()*/",__func__);
  
  if(!rc){
    fsl_stmt_bind_id(st, 1, rid);
    rc = fsl_stmt_step(st);
    fsl_stmt_cached_yield(st);
    gotone = rc==FSL_RC_STEP_ROW;
  }
  return gotone;
#endif
}

static bool fsl_repo_has_mlink_pmid_mid( fsl_db * repo, fsl_id_t pmid, fsl_id_t mid ){
  fsl_stmt * st = NULL;
  int rc = fsl_db_prepare_cached(repo, &st,
                                 "SELECT 1 FROM mlink WHERE mid=? "
                                 "AND pmid=?"
                                 "/*%s()*/",__func__);
  if(!rc){
    fsl_stmt_bind_id(st, 1, mid);
    fsl_stmt_bind_id(st, 2, pmid);
    rc = fsl_stmt_step(st);
    fsl_stmt_cached_yield(st);
    if( rc==FSL_RC_STEP_ROW ) rc = 0;
  }
  /* MARKER(("fsl_repo_has_mlink_mid(%d) rc=%d\n", (int)rid, rc)); */
  return rc ? false : true;
}

/**
    Add mlink table entries associated with manifest cid, pChild.  The
    parent manifest is pid, pParent.  One of either pChild or pParent
    will be NULL and it will be computed based on cid/pid.
   
    A single mlink entry is added for every file that changed content,
    name, and/or permissions going from pid to cid.
   
    Deleted files have mlink.fid=0.
   
    Added files have mlink.pid=0.
   
    File added by merge have mlink.pid=-1.

    Edited files have both mlink.pid!=0 and mlink.fid!=0

    Comments from the original implementation:

    Many mlink entries for merge parents will only be added if another
    mlink entry already exists for the same file from the primary
    parent.  Therefore, to ensure that all merge-parent mlink entries
    are properly created:

    (1) Make this routine a no-op if pParent is a merge parent and the
        primary parent is a phantom.

    (2) Invoke this routine recursively for merge-parents if pParent
        is the primary parent.
*/
static int fsl_mlink_add( fsl_cx * f,
                          fsl_id_t pmid, fsl_deck /*const*/ * pParent,
                          fsl_id_t cid, fsl_deck /*const*/ * pChild,
                          bool isPrimary){
  fsl_buffer otherContent = fsl_buffer_empty;
  fsl_id_t otherRid;
  fsl_size_t i = 0;
  int rc = 0;
  fsl_card_F const * pChildFile = NULL;
  fsl_card_F const * pParentFile = NULL;
  fsl_deck dOther = fsl_deck_empty;
  fsl_db * const db = fsl_cx_db_repo(f);
  bool isPublic;
  assert(db);
  assert(db->beginCount>0);
  /* If mlink table entires are already set for pmid/cid, then abort
     early doing no work.
  */
  //MARKER(("%s() pmid=%d cid=%d\n", __func__, (int)pmid, (int)cid));
  if(fsl_repo_has_mlink_pmid_mid(db, pmid, cid)) return 0;
  /* Compute the value of the missing pParent or pChild parameter.
     Fetch the baseline checkins for both.
  */
  assert( pParent==0 || pChild==0 );
  if( pParent ){
    assert(!pChild);
    pChild = &dOther;
    otherRid = cid;
  }else{
    pParent = &dOther;
    otherRid = pmid;
  }

  if(otherRid && !fsl_cx_mcache_search(f, otherRid, &dOther)){
    rc = otherRid ? fsl_content_get(f, otherRid, &otherContent) : 0;
    if(rc){
      /* fossil(1) simply ignores errors here and returns. We'll ignore
         the phantom case because (1) erroring out here would be bad and
         (2) fossil does so. The exact implications of doing so are
         unclear, though. */
      if(FSL_RC_PHANTOM==rc){
        rc = 0;
      }else if(!f->error.msg.used && FSL_RC_OOM!=rc){
        rc = fsl_cx_err_set(f, rc,
                            "Fetching content of rid %"FSL_ID_T_PFMT" failed: %s",
                            otherRid, fsl_rc_cstr(rc));
      }
      goto end;
    }
    if( !otherContent.used ){
      /* ??? fossil(1) ignores this case and returns. */
      fsl_buffer_clear(&otherContent)/*for empty file case*/;
      rc = 0;
      goto end;
    }
    dOther.f = f;
    rc = fsl_deck_parse2(&dOther, &otherContent, otherRid);
    assert(dOther.f);
    if(rc) goto end;
  }
  if( (pParent->f && (rc=fsl_deck_baseline_fetch(pParent)))
      || (pChild->f && (rc=fsl_deck_baseline_fetch(pChild)))){
    goto end;
  }
  isPublic = !fsl_content_is_private(f, cid);

  /* If pParent is not the primary parent of pChild, and the primary
  ** parent of pChild is a phantom, then abort this routine without
  ** doing any work.  The mlink entries will be computed when the
  ** primary parent dephantomizes.
  */
  if( !isPrimary && otherRid==cid ){
    assert(pChild->P.used);
    if(!fsl_db_exists(db,"SELECT 1 FROM blob WHERE uuid=%Q AND size>0",
                      (char const *)pChild->P.list[0])){
      rc = 0;
      fsl_cx_mcache_insert(f, &dOther);
      goto end;
    }
  }

  if(pmid>0){
    /* Try to make the parent manifest a delta from the child, if that
       is an appropriate thing to do.  For a new baseline, make the 
       previous baseline a delta from the current baseline.
    */
    if( (pParent->B.uuid==0)==(pChild->B.uuid==0) ){
      rc = fsl_content_deltify(f, pmid, cid, 0);
    }else if( pChild->B.uuid==NULL && pParent->B.uuid!=NULL ){
      rc = fsl_content_deltify(f, pParent->B.baseline->rid, cid, 0);
    }
    if(rc) goto end;
  }

  /* Remember all children less than a few seconds younger than their parent,
     as we might want to fudge the times for those children.
  */
  if( f->cache.isCrosslinking &&
      (pChild->D < pParent->D+AGE_FUDGE_WINDOW)
  ){
    rc = fsl_db_exec(db, "INSERT OR REPLACE INTO time_fudge VALUES"
                     "(%"FSL_ID_T_PFMT", %"FSL_JULIAN_T_PFMT
                     ", %"FSL_ID_T_PFMT", %"FSL_JULIAN_T_PFMT");",
                     pParent->rid, pParent->D,
                     pChild->rid, pChild->D);
    if(rc) goto end;
  }

  /* First look at all files in pChild, ignoring its baseline.  This
     is where most of the changes will be found.
  */  
#define FCARD(DECK,NDX) \
  ((((NDX)<(DECK)->F.used)) \
   ? F_at(&(DECK)->F,NDX)  \
   : NULL)
  for(i=0, pChildFile=FCARD(pChild,0);
      i<pChild->F.used;
      ++i, pChildFile=FCARD(pChild,i)){
    fsl_fileperm_e const mperm = pChildFile->perm;
    if( pChildFile->priorName ){
      pParentFile = pmid
        ? fsl_deck_F_seek(pParent, pChildFile->priorName)
        : 0;
      if( pParentFile ){
        /* File with name change */
        /*
          libfossil checkin 8625a31eff708dea93b16582e4ec5d583794d1af
          contains these two interesting F-cards:

F src/net/wanderinghorse/libfossil/FossilCheckout.java
F src/org/fossil_scm/libfossil/Checkout.java 6e58a47089d3f4911c9386c25bac36c8e98d4d21 w src/net/wanderinghorse/libfossil/FossilCheckout.java

          Note the placement of FossilCheckout.java (twice).

          Up until then, i thought a delete/rename combination was not possible.
        */
        rc = fsl_mlink_add_one(f, pmid, pParentFile->uuid,
                               cid, pChildFile->uuid, pChildFile->name,
                               pChildFile->priorName, isPublic,
                               isPrimary, mperm);
      }else{
         /* File name changed, but the old name is not found in the parent!
            Treat this like a new file. */
        rc = fsl_mlink_add_one(f, pmid, 0, cid, pChildFile->uuid,
                               pChildFile->name, 0,
                               isPublic, isPrimary, mperm);
      }
    }else if(pmid){
      pParentFile = fsl_deck_F_seek(pParent, pChildFile->name);
      if(!pParentFile || !pParentFile->uuid){
        /* Parent does not have it or it was removed in parent. */
        if( pChildFile->uuid ){
          /* A new or re-added file */
          rc = fsl_mlink_add_one(f, pmid, 0, cid, pChildFile->uuid,
                                 pChildFile->name, 0,
                                 isPublic, isPrimary, mperm);
        }
      }
      else if( fsl_strcmp(pChildFile->uuid, pParentFile->uuid)!=0
                || (pParentFile->perm!=mperm) ){
         /* Changes in file content or permissions */
        rc = fsl_mlink_add_one(f, pmid, pParentFile->uuid,
                               cid, pChildFile->uuid,
                               pChildFile->name, 0,
                               isPublic, isPrimary, mperm);
      }
    }
  } /* end pChild card list loop */
  if(rc) goto end;
  else if( pParent->B.uuid && pChild->B.uuid ){
    /* Both parent and child are delta manifests.  Look for files that
       are deleted or modified in the parent but which reappear or revert
       to baseline in the child and show such files as being added or changed
       in the child. */
    for(i=0, pParentFile=FCARD(pParent,0);
        i<pParent->F.used;
        ++i, pParentFile = FCARD(pParent,i)){
      if( pParentFile->uuid ){
        pChildFile = fsl_deck_F_seek_base(pChild, pParentFile->name, NULL);
        if( !pChildFile || !pChildFile->uuid){
          /* The child file reverts to baseline or is deleted.
             Show this as a change. */
          if(!pChildFile){
            pChildFile = fsl_deck_F_seek(pChild, pParentFile->name);
          }
          if( pChildFile && pChildFile->uuid ){
            rc = fsl_mlink_add_one(f, pmid, pParentFile->uuid, cid,
                                   pChildFile->uuid, pChildFile->name,
                                   0, isPublic, isPrimary,
                                   pChildFile->perm);
          }
        }
      }else{
        /* Was deleted in the parent. */
        pChildFile = fsl_deck_F_seek(pChild, pParentFile->name);
        if( pChildFile && pChildFile->uuid ){
          /* File resurrected in the child after having been deleted in
             the parent.  Show this as an added file. */
          rc = fsl_mlink_add_one(f, pmid, 0, cid, pChildFile->uuid,
                                 pChildFile->name, 0, isPublic,
                                 isPrimary, pChildFile->perm);
        }
      }
      if(rc) goto end;
    }
    assert(0==rc);
  }else if( pmid && !pChild->B.uuid ){
    /* pChild is a baseline with a parent.  Look for files that are
       present in pParent but are missing from pChild and mark them as
       having been deleted. */
    fsl_card_F const * cfc = NULL;
    fsl_deck_F_rewind(pParent);
    while( (0==(rc=fsl_deck_F_next(pParent,&cfc))) && cfc){
      pParentFile = cfc;
      pChildFile = fsl_deck_F_seek(pChild, pParentFile->name);
      if( (!pChildFile || !pChildFile->uuid) && pParentFile->uuid ){
        rc = fsl_mlink_add_one(f, pmid, pParentFile->uuid, cid, 0,
                               pParentFile->name, 0, isPublic,
                               isPrimary, pParentFile->perm);
      }
    }
    if(rc) goto end;
  }

  fsl_cx_mcache_insert(f, &dOther);
  
  /* If pParent is the primary parent of pChild, also run this analysis
  ** for all merge parents of pChild */
  if( pmid && isPrimary ){
    for(i=1; i<pChild->P.used; i++){
      pmid = fsl_uuid_to_rid(f, (char const*)pChild->P.list[i]);
      if( pmid<=0 ) continue;
      rc = fsl_mlink_add(f, pmid, 0, cid, pChild, false);
      if(rc) goto end;
    }
    for(i=0; i<pChild->Q.used; i++){
      fsl_card_Q const * q = (fsl_card_Q const *)pChild->Q.list[i];
      if( q->type>0 && (pmid = fsl_uuid_to_rid(f, q->target))>0 ){
        rc = fsl_mlink_add(f, pmid, 0, cid, pChild, false);
        if(rc) goto end;
      }
    }
  }

  end:
  fsl_deck_finalize(&dOther);
  fsl_buffer_clear(&otherContent);
  if(rc && !f->error.code && db->error.code){
    rc = fsl_cx_uplift_db_error(f, db);
  }
  return rc;
#undef FCARD
}

/**
   Apply all tags defined in deck d. If parentId is >0 then any
   propagating tags from that parent are well and duly propagated.
   Returns 0 on success. Potential TODO: if parentId<=0 and
   d->P.used>0 then use d->P.list[0] in place of parentId.
*/
static int fsl_deck_crosslink_apply_tags(fsl_cx * f, fsl_deck *d,
                                         fsl_db * db, fsl_id_t rid,
                                         fsl_id_t parentId){
  int rc = 0;
  fsl_size_t i;
  fsl_list const * li = &d->T;
  double tagTime = d->D;
  if(li->used && tagTime<=0){
    tagTime = fsl_db_julian_now(db);
    if(tagTime<=0){
      rc = FSL_RC_DB;
      goto end;
    }
  }      
  for( i = 0; !rc && (i < li->used); ++i){
    fsl_id_t tid;
    fsl_card_T const * tag = (fsl_card_T const *)li->list[i];
    assert(tag);
    if(!tag->uuid){
      tid = rid;
    }else{
      tid = fsl_uuid_to_rid( f, tag->uuid);
    }
    if(tid<0){
      assert(f->error.code);
      rc = f->error.code;
      break;
    }else if(0==tid){
      rc = fsl_cx_err_set(f, FSL_RC_RANGE,
                          "Could not get RID for [%.12s].",
                          tag->uuid);
      break;
    }
    rc = fsl_tag_insert(f, tag->type,
                        tag->name, tag->value,
                        rid, tagTime, tid, NULL);
  }
  if( !rc && (parentId>0) ){
    rc = fsl_tag_propagate_all(f, parentId);
  }
  end:
  return rc;
}

/**
   Part of the checkin crosslink machinery: create all appropriate
   plink and mlink table entries for d->P.

   If parentId is not NULL, *parentId gets assigned to the rid of the
   first parent, or 0 if d->P is empty.
*/
static int fsl_deck_add_checkin_linkages(fsl_deck *d, fsl_id_t * parentId){
  int rc = 0;
  fsl_size_t nLink = 0;
  char zBaseId[30] = {0}/*RID of baseline or "NULL" if no baseline */;
  fsl_size_t i;
  fsl_stmt q = fsl_stmt_empty;
  fsl_id_t _parentId = 0;
  fsl_cx * const f = d->f;
  fsl_db * const db = fsl_cx_db_repo(f);
  assert(f && db);
  if(!parentId) parentId = &_parentId;
  if(d->B.uuid){
    fsl_id_t const baseid = d->B.baseline
      ? d->B.baseline->rid
      : fsl_uuid_to_rid(d->f, d->B.uuid);
    if(baseid<0){
      rc = d->f->error.code;
      assert(0 != rc);
      goto end;
    }
    assert(baseid>0);
    fsl_snprintf( zBaseId, sizeof(zBaseId),
                  "%"FSL_ID_T_PFMT,
                  baseid );
        
  }else{
    fsl_snprintf( zBaseId, sizeof(zBaseId), "NULL" );
  }
  *parentId = 0;
  for(i=0; i<d->P.used; ++i){
    char const * parentUuid = (char const *)d->P.list[i];
    fsl_id_t const pid = fsl_uuid_to_rid2(f, parentUuid, FSL_PHANTOM_PUBLIC);
    if(pid<0){
      assert(f->error.code);
      rc = f->error.code;
      goto end;
    }
    rc = fsl_db_exec(db, "INSERT OR IGNORE "
                     "INTO plink(pid, cid, isprim, mtime, baseid) "
                     "VALUES(%"FSL_ID_T_PFMT", %"FSL_ID_T_PFMT
                     ", %d, %"FSL_JULIAN_T_PFMT", %s)",
                     pid, d->rid,
                     ((i==0) ? 1 : 0), d->D, zBaseId);
    if(rc) goto end;
    if(0==i) *parentId = pid;
  }
  rc = fsl_mlink_add(f, *parentId, NULL, d->rid, d, true);
  if(rc) goto end;
  nLink = d->P.used;
  for(i=0; i<d->Q.used; ++i){
    fsl_card_Q const * q = (fsl_card_Q const *)d->Q.list[i];
    if(q->type>0) ++nLink;
  }
  if(nLink>1){
    /* https://www.fossil-scm.org/index.html/info/8e44cf6f4df4f9f0 */
    /* Change MLINK.PID from 0 to -1 for files that are added by merge. */
    rc = fsl_db_exec(db,
                     "UPDATE mlink SET pid=-1"
                     " WHERE mid=%"FSL_ID_T_PFMT
                     "   AND pid=0"
                     "   AND fnid IN "
                     "  (SELECT fnid FROM mlink WHERE mid=%"FSL_ID_T_PFMT
                     " GROUP BY fnid"
                     "    HAVING count(*)<%d)",
                     d->rid, d->rid, (int)nLink
                     );
    if(rc) goto end;
  }
  rc = fsl_db_prepare(db, &q,
                      "SELECT cid, isprim FROM plink "
                      "WHERE pid=%"FSL_ID_T_PFMT,
                      d->rid);
  while( !rc && (FSL_RC_STEP_ROW==(rc=fsl_stmt_step(&q))) ){
    fsl_id_t const cid = fsl_stmt_g_id(&q, 0);
    int const isPrim = fsl_stmt_g_int32(&q, 1);
    /* This block is only hit a couple of times during a fresh rebuild (empty mlink/plink
       tables), but many times on a rebuilds if those tables are not emptied in advance? */
    assert(cid>0);
    rc = fsl_mlink_add(f, d->rid, d, cid, NULL, isPrim ? true : false);
  }
  if(FSL_RC_STEP_DONE==rc) rc = 0;
  fsl_stmt_finalize(&q);
  if(rc) goto end;
  if( !d->P.used ){
    /* For root files (files without parents) add mlink entries
       showing all content as new.

       Historically, fossil has been unable to create such checkins
       because the initial checkin has no files.
    */
    int const isPublic = !fsl_content_is_private(f, d->rid);
    for(i=0; !rc && (i<d->F.used); ++i){
      fsl_card_F const * fc = F_at(&d->F, i);
      rc = fsl_mlink_add_one(f, 0, 0, d->rid, fc->uuid, fc->name, 0,
                             isPublic, 1, fc->perm);
    }
  }
  end:
  return rc;
}

/**
   Applies the value of a "parent" tag (reparent) to the given
   artifact id. zTagVal must be the value of a parent tag (a list of
   full UUIDs). This is only to be run as part of fsl_crosslink_end().

   Returns 0 on success.

   POTENTIAL fixme: perhaps return without side effects if rid is not
   found (like fossil(1) does). That said, this step is only run after
   crosslinking and would only result in a not-found if the tagxref
   table contents is out of date.

   POTENTIAL fixme: fail without error if the tag value is malformed,
   under the assumption that the tag was intended for some purpose
   other than reparenting.
*/
static int fsl_crosslink_reparent(fsl_cx * f, fsl_id_t rid, char const *zTagVal){
  int rc = 0;
  char * zDup = 0;
  char * zPos;
  fsl_size_t maxP, nP = 0;
  fsl_deck d = fsl_deck_empty;
  fsl_list fakeP = fsl_list_empty
    /* fake P-card for purposes of passing the reparented deck through
       fsl_deck_add_checkin_linkages() */;
  maxP = (fsl_strlen(zTagVal)+1) / (FSL_STRLEN_SHA1+1);
  if(!maxP) return FSL_RC_RANGE;
  rc = fsl_list_reserve(&fakeP, maxP);
  if(rc) return rc;
  zDup = fsl_strdup(zTagVal);
  if(!zDup){
    rc = FSL_RC_OOM;
    goto end;
  }
  /* Split zTagVal into list of parent IDs... */
  for( nP = 0, zPos = zDup; *zPos; ){
    char const * zBegin = zPos;
    for( ; *zPos && ' '!=*zPos; ++zPos){}
    if(' '==*zPos){
      *zPos = 0;
      ++zPos;
    }
    if(!fsl_is_uuid(zBegin)){
      rc = fsl_cx_err_set(f, FSL_RC_RANGE,
                          "Invalid value [%s] in reparent tag value "
                          "[%s] for rid %"FSL_ID_T_PFMT".",
                          zBegin, zTagVal, rid);
      goto end;
    }
    fakeP.list[nP++] = (void *)zBegin;
  }
  assert(!rc);
  fakeP.used = nP;
  rc = fsl_deck_load_rid(f, &d, rid, FSL_SATYPE_ANY);
  if(rc) goto end;
  switch(d.type){
    case FSL_SATYPE_CHECKIN:
    case FSL_SATYPE_TECHNOTE:
    case FSL_SATYPE_WIKI:
    case FSL_SATYPE_FORUMPOST:
      break;
    default:
      rc = fsl_cx_err_set(f, FSL_RC_TYPE, "Invalid deck type (%s) "
                          "for use with the 'parent' tag.",
                          fsl_satype_cstr(d.type));
      goto end;
  }
  assert(d.rid==rid);
  assert(d.f);
  fsl_db * const db = fsl_cx_db_repo(f);
  rc = fsl_db_exec_multi(db,
                         "DELETE FROM plink WHERE cid=%"FSL_ID_T_PFMT";"
                         "DELETE FROM mlink WHERE mid=%"FSL_ID_T_PFMT";",
                         rid, rid);
  if(rc) goto end;
  fsl_list const origP = d.P;
  d.P = fakeP;
  rc = fsl_deck_add_checkin_linkages(&d, NULL);
  d.P = origP;
  fsl_deck_finalize(&d);

  end:
  fsl_list_reserve(&fakeP, 0);
  fsl_free(zDup);
  return rc;
}

/**
   Inserts plink entries for FORUM, WIKI, and TECHNOTE manifests. May
   assert for other manifest types. If a parent entry exists, it also
   propagates any tags for that parent. This is a no-op if
   the deck has no parents.
*/
static int fsl_deck_crosslink_fwt_plink(fsl_deck * d){
  int i;
  fsl_id_t parentId = 0;
  fsl_db * db;
  int rc = 0;
  assert(d->type==FSL_SATYPE_WIKI ||
         d->type==FSL_SATYPE_FORUMPOST ||
         d->type==FSL_SATYPE_TECHNOTE);
  assert(d->f);
  assert(d->rid>0);
  if(!d->P.used) return rc;
  db = fsl_cx_db_repo(d->f);
  fsl_phantom_e const fantomMode = fsl_content_is_private(d->f, d->rid)
    ? FSL_PHANTOM_PRIVATE : FSL_PHANTOM_PUBLIC;
  for(i=0; 0==rc && i<(int)d->P.used; ++i){
    fsl_id_t const pid = fsl_uuid_to_rid2(d->f, (char const *)d->P.list[i],
                                          fantomMode);
    if(0==i) parentId = pid;
    rc = fsl_db_exec_multi(db,
                           "INSERT OR IGNORE INTO plink"
                           "(pid, cid, isprim, mtime, baseid)"
                           "VALUES(%"FSL_ID_T_PFMT", %"FSL_ID_T_PFMT", "
                           "%d, %"FSL_JULIAN_T_PFMT", NULL)",
                           pid, d->rid, i==0, d->D);
  }
  if(!rc && parentId){
    rc = fsl_tag_propagate_all(d->f, parentId);
  }
  return rc;
}


/**
   Overrideable crosslink listener which updates the timeline for
   attachment records.
*/
static int fsl_deck_xlink_f_attachment(fsl_deck * d, void * state){
  if(FSL_SATYPE_ATTACHMENT!=d->type) return 0;
  int rc;
  fsl_db * db;
  fsl_buffer comment = fsl_buffer_empty;
  const char isAdd = (d->A.src && *d->A.src) ? 1 : 0;
  char attachToType = 'w'
    /* Assume wiki until we know otherwise, keeping in mind that the
       d->A.tgt might not yet be in the blob table, in which case
       we are unable to know, for certain, what the target is.
       That only affects the timeline (event table), though, not
       the crosslinking of the attachment itself. */;
  db = fsl_cx_db_repo(d->f);
  assert(db);
  if(fsl_is_uuid(d->A.tgt)){
    if( fsl_db_exists(db, "SELECT 1 FROM tag WHERE tagname='tkt-%q'",
                      d->A.tgt)){
      attachToType = 't' /* attach to a known ticket */;
    }else if( fsl_db_exists(db, "SELECT 1 FROM tag WHERE tagname='event-%q'",
                            d->A.tgt)){
      attachToType = 'e' /* attach to a known technote (event) */;
    }
  }
  if('w'==attachToType){
    /* Attachment applies to a wiki page */
    if(isAdd){
      rc = fsl_buffer_appendf(&comment,
                              "Add attachment \"%h\" "
                              "to wiki page [%h]",
                              d->A.name, d->A.tgt);
    }else{
      rc = fsl_buffer_appendf(&comment,
                              "Delete attachment \"%h\" "
                              "from wiki page [%h]",
                              d->A.name, d->A.tgt);
    }
  }else if('e' == attachToType){/*technote*/
    if(isAdd){
      rc = fsl_buffer_appendf(&comment,
                              "Add attachment [/artifact/%!S|%h] to "
                              "tech note [/technote/%!S|%S]",
                              d->A.src, d->A.name, d->A.tgt, d->A.tgt);
    }else{
      rc = fsl_buffer_appendf(&comment,
                              "Delete attachment \"/artifact/%!S|%h\" "
                              "from tech note [/technote/%!S|%S]",
                              d->A.name, d->A.name, d->A.tgt,
                              d->A.tgt);
    }
  }else{
    /* Attachment applies to a ticket */
    if(isAdd){
      rc = fsl_buffer_appendf(&comment,
                              "Add attachment [/artifact/%!S|%h] "
                              "to ticket [%!S|%S]",
                              d->A.src, d->A.name, d->A.tgt, d->A.tgt);
    }else{
      rc = fsl_buffer_appendf(&comment,
                              "Delete attachment \"%h\" "
                              "from ticket [%!S|%S]",
                              d->A.name, d->A.tgt, d->A.tgt);
    }
  }
  if(!rc){
    rc = fsl_db_exec(db,
                     "REPLACE INTO event(type,mtime,objid,user,comment)"
                     "VALUES("
                     "'%c',%"FSL_JULIAN_T_PFMT",%"FSL_ID_T_PFMT","
                     "%Q,%B)",
                     attachToType, d->D, d->rid, d->U, &comment);
  }
  fsl_buffer_clear(&comment);
  return rc;
}

/**
   Overrideable crosslink listener which updates the timeline for
   checkin records.
*/
static int fsl_deck_xlink_f_checkin(fsl_deck * d, void * state){
  if(FSL_SATYPE_CHECKIN!=d->type) return 0;
  int rc;
  fsl_db * db;
  db = fsl_cx_db_repo(d->f);
  assert(db);
  rc = fsl_db_exec(db,
       "REPLACE INTO event(type,mtime,objid,user,comment,"
       "bgcolor,euser,ecomment,omtime)"
       "VALUES('ci',"
       "  coalesce(" /*mtime*/
       "    (SELECT julianday(value) FROM tagxref "
       "      WHERE tagid=%d AND rid=%"FSL_ID_T_PFMT
       "    ),"
       "    %"FSL_JULIAN_T_PFMT""
       "  ),"
       "  %"FSL_ID_T_PFMT","/*objid*/
       "  %Q," /*user*/
#if 1
       "  %Q," /*comment. No, the comment _field_. */
#else
       /* just for testing... */
       "  'xlink: %q'," /*comment. No, the comment _field_. */
#endif
       "  (SELECT value FROM tagxref " /*bgcolor*/
       "    WHERE tagid=%d AND rid=%"FSL_ID_T_PFMT
       "    AND tagtype>0"
       "  ),"
       "  (SELECT value FROM tagxref " /*euser*/
       "    WHERE tagid=%d AND rid=%"FSL_ID_T_PFMT
       "  ),"
       "  (SELECT value FROM tagxref " /*ecomment*/
       "    WHERE tagid=%d AND rid=%"FSL_ID_T_PFMT
       "  ),"
       "  %"FSL_JULIAN_T_PFMT/*omtime*/
       /* RETURNING coalesce(ecomment,comment)
          see comments below about zCom */
       ")",
       /* The casts here are to please the va_list. */
       (int)FSL_TAGID_DATE, d->rid, d->D,
       d->rid, d->U, d->C,
       (int)FSL_TAGID_BGCOLOR, d->rid,
       (int)FSL_TAGID_USER, d->rid,
       (int)FSL_TAGID_COMMENT, d->rid, d->D
  );
  return fsl_cx_uplift_db_error2(d->f, db, rc);
}

static int fsl_deck_xlink_f_control(fsl_deck * d, void * state){
  if(FSL_SATYPE_CONTROL!=d->type) return 0;
  /*
    Create timeline event entry for all tags in this control
    construct. Note that we are using a lot of historical code which
    hard-codes english-lanuage text and links which only work in
    fossil(1). i would prefer to farm this out to a crosslink
    callback, and provide a default implementation which more or
    less mimics fossil(1).
  */
  int rc = 0;
  fsl_buffer comment = fsl_buffer_empty;
  fsl_size_t i;
  const char *zName;
  const char *zValue;
  const char *zUuid;
  int branchMove = 0;
  int const uuidLen = 8;
  fsl_card_T const * tag = NULL;
  fsl_card_T const * prevTag = NULL;
  fsl_list const * li = &d->T;
  fsl_db * const db = fsl_cx_db_repo(d->f);
  double mtime = (d->D>0)
    ? d->D
    : fsl_db_julian_now(db);
  assert(db);
  /**
     Reminder to self: fossil(1) has a comment here:

     // Next loop expects tags to be sorted on UUID, so sort it.
     qsort(p->aTag, p->nTag, sizeof(p->aTag[0]), tag_compare);

     That sort plays a role in hook code execution and is needed to
     avoid duplicate hook execution in some cases. libfossil
     outsources that type of thing to crosslink callbacks, though,
     so we won't concern ourselves with it here. We also don't
     really want to modify the deck during crosslinking. The only
     reason the deck is not const in this routine is because of the
     fsl_deck::F::cursor bits inherited from fossil(1), largely
     worth its cost except that many routines can no longer be
     const. Shame C doesn't have C++'s "mutable" keyword.

     That said, sorting by UUID would have a nice side-effect on the
     output of grouping tags by the UUID they tag. So far
     (201404) such groups of tags have not appeared in the wild
     because fossil(1) has no mechanism for creating them.
  */
  for( i = 0; !rc && (i < li->used); ++i, prevTag = tag){
    char isProp = 0, isAdd = 0, isCancel = 0;
    tag = (fsl_card_T const *)li->list[i];
    zUuid = tag->uuid;
    if(!zUuid /*tag on self*/) continue;
    if( i==0 || 0!=fsl_uuidcmp(tag->uuid, prevTag->uuid)){
      rc = fsl_buffer_appendf(&comment,
                              " Edit [%.*s]:", uuidLen, zUuid);
      branchMove = 0;
    }
    if(rc) goto end;
    isProp = FSL_TAGTYPE_PROPAGATING==tag->type;
    isAdd = FSL_TAGTYPE_ADD==tag->type;
    isCancel = FSL_TAGTYPE_CANCEL==tag->type;
    assert(isProp || isAdd || isCancel);
    zName = tag->name;
    zValue = tag->value;
    if( isProp && 0==fsl_strcmp(zName, "branch")){
      rc = fsl_buffer_appendf(&comment,
                              " Move to branch %s"
                              "[/timeline?r=%h&nd&dp=%.*s | %h].",
                              zValue, zValue, uuidLen, zUuid, zValue);
      branchMove = 1;
    }else if( isProp && fsl_strcmp(zName, "bgcolor")==0 ){
      rc = fsl_buffer_appendf(&comment,
                              " Change branch background color to \"%h\".", zValue);
    }else if( isAdd && fsl_strcmp(zName, "bgcolor")==0 ){
      rc = fsl_buffer_appendf(&comment,
                              " Change background color to \"%h\".", zValue);
    }else if( isCancel && fsl_strcmp(zName, "bgcolor")==0 ){
      rc = fsl_buffer_appendf(&comment, " Cancel background color.");
    }else if( isAdd && fsl_strcmp(zName, "comment")==0 ){
      rc = fsl_buffer_appendf(&comment, " Edit check-in comment.");
    }else if( isAdd && fsl_strcmp(zName, "user")==0 ){
      rc = fsl_buffer_appendf(&comment, " Change user to \"%h\".", zValue);
    }else if( isAdd && fsl_strcmp(zName, "date")==0 ){
      rc = fsl_buffer_appendf(&comment, " Timestamp %h.", zValue);
    }else if( isCancel && memcmp(zName, "sym-",4)==0 ){
      if( !branchMove ){
        rc = fsl_buffer_appendf(&comment, " Cancel tag %h.", zName+4);
      }
    }else if( isProp && memcmp(zName, "sym-",4)==0 ){
      if( !branchMove ){
        rc = fsl_buffer_appendf(&comment, " Add propagating tag \"%h\".", zName+4);
      }
    }else if( isAdd && memcmp(zName, "sym-",4)==0 ){
      rc = fsl_buffer_appendf(&comment, " Add tag \"%h\".", zName+4);
    }else if( isCancel && memcmp(zName, "sym-",4)==0 ){
      rc = fsl_buffer_appendf(&comment, " Cancel tag \"%h\".", zName+4);
    }else if( isAdd && fsl_strcmp(zName, "closed")==0 ){
      rc = fsl_buffer_append(&comment, " Marked \"Closed\"", -1);
      if( !rc && zValue && *zValue ){
        rc = fsl_buffer_appendf(&comment, " with note \"%h\"", zValue);
      }
      if(!rc) rc = fsl_buffer_append(&comment, ".", 1);
    }else if( isCancel && fsl_strcmp(zName, "closed")==0 ){
      rc = fsl_buffer_append(&comment, " Removed the \"Closed\" mark", -1);
      if( !rc && zValue && *zValue ){
        rc = fsl_buffer_appendf(&comment, " with note \"%h\"", zValue);
      }
      if(!rc) rc = fsl_buffer_append(&comment, ".", 1);
    }else {
      if( isCancel ){
        rc = fsl_buffer_appendf(&comment, " Cancel \"%h\"", zName);
      }else if( isAdd ){
        rc = fsl_buffer_appendf(&comment, " Add \"%h\"", zName);
      }else{
        assert(isProp);
        rc = fsl_buffer_appendf(&comment, " Add propagating \"%h\"", zName);
      }
      if(rc) goto end;
      if( zValue && zValue[0] ){
        rc = fsl_buffer_appendf(&comment, " with value \"%h\".", zValue);
      }else{
        rc = fsl_buffer_append(&comment, ".", 1);
      }
    }
  } /* foreach tag loop */
  if(!rc){
    /* TODO: cached statement */
    rc = fsl_db_exec(db,
                     "REPLACE INTO event"
                     "(type,mtime,objid,user,comment) "
                     "VALUES('g',"
                     "%"FSL_JULIAN_T_PFMT","
                     "%"FSL_ID_T_PFMT","
                     "%Q,%Q)",
                     mtime, d->rid, d->U,
                     (comment.used>1)
                     ? (fsl_buffer_cstr(&comment)
                        +1/*leading space on all entries*/)
                     : NULL);
  }

  end:
  fsl_buffer_clear(&comment);
  return rc;

}

static int fsl_deck_xlink_f_forum(fsl_deck * d, void * state){
  if(FSL_SATYPE_FORUMPOST!=d->type) return 0;
  int rc = 0;
  fsl_db * const db = fsl_cx_db_repo(d->f);
  assert(db);
  fsl_cx * const f = d->f;
  fsl_id_t const froot = d->G ? fsl_uuid_to_rid(f, d->G) : d->rid;
  fsl_id_t const fprev = d->P.used ? fsl_uuid_to_rid(f, (char const *)d->P.list[0]): 0;
  fsl_id_t const firt = d->I ? fsl_uuid_to_rid(f, d->I) : 0;
  if( 0==firt ){
    /* This is the start of a new thread, either the initial entry
    ** or an edit of the initial entry. */
    const char * zTitle = d->H;
    const char * zFType;
    if(!zTitle || !*zTitle){
      zTitle = "(Deleted)";
    }
    zFType = fprev ? "Edit" : "Post";
    /* FSL-MISSING:
       assert( manifest_event_triggers_are_enabled ); */
    rc = fsl_db_exec_multi(db,
        "REPLACE INTO event(type,mtime,objid,user,comment)"
        "VALUES('f',%"FSL_JULIAN_T_PFMT",%" FSL_ID_T_PFMT
        ",%Q,'%q: %q')",
        d->D, d->rid, d->U, zFType, zTitle);
    if(rc) goto dberr;
      /*
      ** If this edit is the most recent, then make it the title for
      ** all other entries for the same thread
      */
    if( !fsl_db_exists(db,"SELECT 1 FROM forumpost "
                       "WHERE froot=%" FSL_ID_T_PFMT " AND firt=0"
                       " AND fpid!=%" FSL_ID_T_PFMT
                       " AND fmtime>%"FSL_JULIAN_T_PFMT,
                       froot, d->rid, d->D)){
        /* This entry establishes a new title for all entries on the thread */
      rc = fsl_db_exec_multi(db,
          "UPDATE event"
          " SET comment=substr(comment,1,instr(comment,':')) || ' %q'"
          " WHERE objid IN (SELECT fpid FROM forumpost WHERE froot=% " FSL_ID_T_PFMT ")",
          zTitle, froot);
      if(rc) goto dberr;
    }
  }else{
      /* This is a reply to a prior post.  Take the title from the root. */
    char const * zFType = 0;
    char * zTitle = fsl_db_g_text(
           db, 0, "SELECT substr(comment,instr(comment,':')+2)"
           "  FROM event WHERE objid=%"FSL_ID_T_PFMT, froot);
    if( zTitle==0 ){
      zTitle = fsl_strdup("<i>Unknown</i>");
      if(!zTitle){
        rc = FSL_RC_OOM;
        goto end;
      }
    }
    if( !d->W.used ){
      zFType = "Delete reply";
    }else if( fprev ){
      zFType = "Edit reply";
    }else{
      zFType = "Reply";
    }
    /* FSL-MISSING:
       assert( manifest_event_triggers_are_enabled ); */
    rc = fsl_db_exec_multi(db,
        "REPLACE INTO event(type,mtime,objid,user,comment)"
        "VALUES('f',%"FSL_JULIAN_T_PFMT
        ",%"FSL_ID_T_PFMT",%Q,'%q: %q')",
        d->D, d->rid, d->U, zFType, zTitle);
    fsl_free(zTitle);
    if(rc) goto end;
    if( d->W.used ){
      /* FSL-MISSING:
         backlink_extract(&d->W, d->N, d->rid, BKLNK_FORUM, d->D, 1); */
    }
  }
  end:
  return rc;
  dberr:
  assert(rc);
  assert(db->error.code);
  return fsl_cx_uplift_db_error(f, db);
}


static int fsl_deck_xlink_f_technote(fsl_deck * d, void * state){
  if(FSL_SATYPE_TECHNOTE!=d->type) return 0;
  char buf[FSL_STRLEN_K256 + 7 /* event-UUID\0 */] = {0};
  fsl_id_t tagid;
  char const * zTag;
  int rc = 0;
  fsl_cx * const f = d->f;
  fsl_db * const db = fsl_cx_db_repo(d->f);
  fsl_snprintf(buf, sizeof(buf), "event-%s", d->E.uuid);
  zTag = buf;
  tagid = fsl_tag_id( f, zTag, 1 );
  if(tagid<=0){
    return f->error.code ? f->error.code :
      fsl_cx_err_set(f, FSL_RC_RANGE,
                     "Got unexpected RID (%"FSL_ID_T_PFMT") "
                     "for tag [%s].",
                     tagid, zTag);
  }
  fsl_id_t const subsequent
    = fsl_db_g_id(db, 0,
                  "SELECT rid FROM tagxref"
                  " WHERE tagid=%"FSL_ID_T_PFMT
                  " AND mtime>=%"FSL_JULIAN_T_PFMT
                  " AND rid!=%"FSL_ID_T_PFMT
                  " ORDER BY mtime",
                  tagid, d->D, d->rid);
  if(subsequent<0){
    rc = fsl_cx_uplift_db_error(d->f, db);
  }else{
    rc = fsl_db_exec(db,
                     "REPLACE INTO event("
                     "type,mtime,"
                     "objid,tagid,"
                     "user,comment,bgcolor"
                     ")VALUES("
                     "'e',%"FSL_JULIAN_T_PFMT","
                     "%"FSL_ID_T_PFMT",%"FSL_ID_T_PFMT","
                     "%Q,%Q,"
                     "  (SELECT value FROM tagxref WHERE "
                     "   tagid=%d"
                     "   AND rid=%"FSL_ID_T_PFMT")"
                     ");",
                     d->E.julian, d->rid, tagid,
                     d->U, d->C, 
                     (int)FSL_TAGID_BGCOLOR, d->rid);
  }
  return rc;
}

static int fsl_deck_xlink_f_wiki(fsl_deck * d, void * state){
  if(FSL_SATYPE_WIKI!=d->type) return 0;
  int rc;
  char const * zWiki;
  fsl_size_t nWiki = 0;
  char cPrefix = 0;
  char * zTag = fsl_mprintf("wiki-%s", d->L);
  if(!zTag) return FSL_RC_OOM;
  fsl_id_t const tagid = fsl_tag_id( d->f, zTag, 1 );
  if(tagid<=0){
    rc = fsl_cx_err_set(d->f, FSL_RC_ERROR,
                        "Tag [%s] must have been added by main wiki crosslink step.",
                        zTag);
    goto end;
  }
  /* Some of this is duplicated in the main wiki crosslinking code :/. */
  zWiki = d->W.used ? fsl_buffer_cstr(&d->W) : "";
  while( *zWiki && fsl_isspace(*zWiki) ){
    ++zWiki;
    /* Historical behaviour: strip leading spaces. */
  }
  /* As of late 2020, fossil changed the conventions for how wiki
     entries are to be added to the timeline. They requrie a prefix
     character which tells the timeline display and email notification
     generator code what type of change this is: create/update/delete */
  nWiki = fsl_strlen(zWiki);
  if(!nWiki) cPrefix = '-';
  else if( !d->P.used ) cPrefix = '+';
  else cPrefix = ':';
  fsl_db * const db = fsl_cx_db_repo(d->f);
  rc = fsl_db_exec(db,
                   "REPLACE INTO event(type,mtime,objid,user,comment) "
                   "VALUES('w',%"FSL_JULIAN_T_PFMT
                   ",%"FSL_ID_T_PFMT",%Q,'%c%q%q%q');",
                   d->D, d->rid, d->U, cPrefix, d->L,
                   ((d->C && *d->C) ? ": " : ""),
                   ((d->C && *d->C) ? d->C : ""));
  /* Note that wiki pages optionally support d->C (change comment),
     but it's historically unused because it was a late addition to
     the artifact format and is not supported by older fossil
     versions. */
  rc = fsl_cx_uplift_db_error2(d->f, db, rc);
  end:
  fsl_free(zTag);
  return rc;
}


/** @internal

    Installs the core overridable crosslink listeners. "The plan" is
    to do all updates to the event (timeline) table via these
    crosslinkers and perform the core, UI-agnostic, crosslinking bits
    in the internal fsl_deck_crosslink_XXX() functions. That should
    allow clients to override how the timeline is updated without
    requiring them to understand the rest of the required schema
    updates.
*/
int fsl_cx_install_timeline_crosslinkers(fsl_cx *f){
  int rc;
  assert(!f->xlinkers.used);
  assert(!f->xlinkers.list);
  rc = fsl_xlink_listener(f, "fsl/attachment/timeline",
                          fsl_deck_xlink_f_attachment, 0);
  if(!rc) rc = fsl_xlink_listener(f, "fsl/checkin/timeline",
                          fsl_deck_xlink_f_checkin, 0);
  if(!rc) rc = fsl_xlink_listener(f, "fsl/control/timeline",
                          fsl_deck_xlink_f_control, 0);
  if(!rc) rc = fsl_xlink_listener(f, "fsl/forumpost/timeline",
                          fsl_deck_xlink_f_forum, 0);
  if(!rc) rc = fsl_xlink_listener(f, "fsl/technote/timeline",
                          fsl_deck_xlink_f_technote, 0);
  if(!rc) rc = fsl_xlink_listener(f, "fsl/wiki/timeline",
                          fsl_deck_xlink_f_wiki, 0);
  return rc;
}


static int fsl_deck_crosslink_checkin(fsl_deck * const d,
                                      fsl_id_t *parentid ){
  int rc = 0;
  fsl_cx * const f = d->f;
  fsl_db * const db = fsl_cx_db_repo(f);

  /* TODO: convert these queries to cached statements, for
     the sake of rebuild and friends. And bind() doubles
     instead of %FSL_JULIAN_T_PFMT'ing them.
  */
  if(d->Q.used && fsl_db_table_exists(db, FSL_DBROLE_REPO,
                                      "cherrypick")){
    fsl_size_t i;
    for(i=0; i < d->Q.used; ++i){
      fsl_card_Q const * q = (fsl_card_Q const *)d->Q.list[i];
      rc = fsl_db_exec(db,
          "REPLACE INTO cherrypick(parentid,childid,isExclude)"
          " SELECT rid, %"FSL_ID_T_PFMT", %d"
          " FROM blob WHERE uuid=%Q",
          d->rid, q->type<0 ? 1 : 0, q->target
      );
      if(rc) goto end;
    }
  }
  if(!fsl_repo_has_mlink_mid(db, d->rid)){
    rc = fsl_deck_add_checkin_linkages(d, parentid);
    if(rc) goto end;
    /* FSL-MISSING:
       assert( manifest_event_triggers_are_enabled ); */
    rc = fsl_search_doc_touch(f, d->type, d->rid, 0);
    if(rc) goto end;
    /* If this is a delta-manifest, record the fact that this repository
       contains delta manifests, to free the "commit" logic to generate
       new delta manifests. */
    if(d->B.uuid){
      rc = fsl_cx_update_seen_delta_mf(f);
      if(rc) goto end;
    }
    assert(!rc);
  }/*!exists mlink*/
  end:
  if(rc && !f->error.code && db->error.code){
    fsl_cx_uplift_db_error(f, db);
  }
  return rc;
}

static int fsl_deck_crosslink_wiki(fsl_deck *d){
  char zLength[40] = {0};
  fsl_id_t prior = 0;
  char const * zWiki;
  fsl_size_t nWiki = 0;
  int rc;
  char * zTag = fsl_mprintf("wiki-%s", d->L);
  fsl_cx * const f = d->f;
  fsl_db * const db = fsl_cx_db_repo(f);
  if(!zTag){
    return FSL_RC_OOM;
  }
  assert(f && db);
  zWiki = d->W.used ? fsl_buffer_cstr(&d->W) : "";
  while( *zWiki && fsl_isspace(*zWiki) ){
    ++zWiki;
    /* Historical behaviour: strip leading spaces. */
  }
  nWiki = fsl_strlen(zWiki)
    /* Reminder: use strlen instead of d->W.used just in case that
       one contains embedded NULs in the content. "Shouldn't
       happen," but the API doesn't explicitly prohibit it.
    */;
  fsl_snprintf(zLength, sizeof(zLength), "%"FSL_SIZE_T_PFMT,
               (fsl_size_t)nWiki);
  rc = fsl_tag_insert(f, FSL_TAGTYPE_ADD, zTag, zLength,
                      d->rid, d->D, d->rid, NULL );
  if(rc) goto end;
  if(d->P.used){
    prior = fsl_uuid_to_rid(f, (const char *)d->P.list[0]);
  }
  if(prior>0){
    rc = fsl_content_deltify(f, prior, d->rid, 0);
    if(rc) goto end;
  }
  rc = fsl_search_doc_touch(f, d->type, d->rid, d->L);
  if(rc) goto end;
  if( f->cache.isCrosslinking ){
    rc = fsl_deck_crosslink_add_pending(f, 'w',d->L);
    if(rc) goto end;
  }else{
    /* FSL-MISSING:
       backlink_wiki_refresh(d->L); */
  }
  assert(0==rc);
  rc = fsl_deck_crosslink_fwt_plink(d);
  end:
  fsl_free(zTag);
  return rc;
}

static int fsl_deck_crosslink_attachment(fsl_deck * const d){
  int rc;
  fsl_cx * const f = d->f;
  fsl_db * const db = fsl_cx_db_repo(f);

  rc = fsl_db_exec(db,
                   /* REMINDER: fossil(1) uses INSERT here, but that
                      breaks libfossil crosslinking tests due to a
                      unique constraint violation on attachid. */
                   "REPLACE INTO attachment(attachid, mtime, src, target,"
                   "filename, comment, user) VALUES("
                   "%"FSL_ID_T_PFMT",%"FSL_JULIAN_T_PFMT","
                   "%Q,%Q,%Q,"
                   "%Q,%Q);",
                   d->rid, d->D,
                   d->A.src, d->A.tgt, d->A.name,
                   (d->C ? d->C : ""), d->U);
  if(!rc){
    rc = fsl_db_exec(db,
                   "UPDATE attachment SET isLatest = (mtime=="
                   "(SELECT max(mtime) FROM attachment"
                   "  WHERE target=%Q AND filename=%Q))"
                   " WHERE target=%Q AND filename=%Q",
                   d->A.tgt, d->A.name,
                   d->A.tgt, d->A.name);
  }
  return rc;  
}

static int fsl_deck_crosslink_cluster(fsl_deck * const d){
  /* Clean up the unclustered table... */
  fsl_size_t i;
  fsl_stmt * st = NULL;
  int rc;
  fsl_cx * const f = d->f;
  fsl_db * const db = fsl_cx_db_repo(f);

  rc = fsl_db_prepare_cached(db, &st,
                             "DELETE FROM unclustered WHERE rid=?"
                             "/*%s()*/",__func__);
  if(rc) return fsl_cx_uplift_db_error(f, db);
  assert(st);
  for( i = 0; i < d->M.used; ++i ){
    fsl_id_t mid;
    char const * uuid = (char const *)d->M.list[i];
    mid = fsl_uuid_to_rid(f, uuid);
    if(mid>0){
      fsl_stmt_bind_id(st, 1, mid);
      if(FSL_RC_STEP_DONE!=fsl_stmt_step(st)){
        rc = fsl_cx_uplift_db_error(f, db);
        break;
      }
      fsl_stmt_reset(st);
    }
  }
  fsl_stmt_cached_yield(st);
  return rc;
}

#if 0
static int fsl_deck_crosslink_control(fsl_deck *const d){
}
#endif

static int fsl_deck_crosslink_forum(fsl_deck * const d){
  int rc = 0;
  fsl_cx * const f = d->f;
  rc = fsl_repo_install_schema_forum(f);
  if(rc) return rc;
  fsl_db * const db = fsl_cx_db_repo(f);
  fsl_id_t const froot = d->G ? fsl_uuid_to_rid(f, d->G) : d->rid;
  fsl_id_t const fprev = d->P.used ? fsl_uuid_to_rid(f, (char const *)d->P.list[0]): 0;
  fsl_id_t const firt = d->I ? fsl_uuid_to_rid(f, d->I) : 0;
  assert(f && db);
  rc = fsl_db_exec_multi(db,
      "REPLACE INTO forumpost(fpid,froot,fprev,firt,fmtime)"
      "VALUES(%" FSL_ID_T_PFMT ",%" FSL_ID_T_PFMT ","
      "nullif(%" FSL_ID_T_PFMT ",0),"
      "nullif(%" FSL_ID_T_PFMT ",0),%"FSL_JULIAN_T_PFMT")",
      d->rid, froot, fprev, firt, d->D
  );
  rc = fsl_cx_uplift_db_error2(f, db, rc);
  if(!rc){
    rc = fsl_search_doc_touch(f, d->type, d->rid, 0);
  }
  if(!rc){
    rc = fsl_deck_crosslink_fwt_plink(d);
  }
  return rc;
}

static int fsl_deck_crosslink_technote(fsl_deck * const d){
  char buf[FSL_STRLEN_K256 + 7 /* event-UUID\0 */] = {0};
  char zLength[40] = {0};
  fsl_id_t tagid;
  fsl_id_t prior = 0, subsequent;
  char const * zWiki;
  char const * zTag;
  fsl_size_t nWiki = 0;
  int rc;
  fsl_cx * const f = d->f;
  fsl_db * const db = fsl_cx_db_repo(f);
  fsl_snprintf(buf, sizeof(buf), "event-%s", d->E.uuid);
  zTag = buf;
  tagid = fsl_tag_id( f, zTag, 1 );
  if(tagid<=0){
    rc = f->error.code ? f->error.code :
      fsl_cx_err_set(f, FSL_RC_RANGE,
                     "Got unexpected RID (%"FSL_ID_T_PFMT") "
                     "for tag [%s].",
                     tagid, zTag);
    goto end;
  }
  zWiki = d->W.used ? fsl_buffer_cstr(&d->W) : "";
  while( *zWiki && fsl_isspace(*zWiki) ){
    ++zWiki;
    /* Historical behaviour: strip leading spaces. */
  }
  nWiki = fsl_strlen(zWiki);
  fsl_snprintf( zLength, sizeof(zLength), "%"FSL_SIZE_T_PFMT,
                (fsl_size_t)nWiki);
  rc = fsl_tag_insert(f, FSL_TAGTYPE_ADD, zTag, zLength,
                      d->rid, d->D, d->rid, NULL );
  if(rc) goto end;
  if(d->P.used){
    prior = fsl_uuid_to_rid(f, (const char *)d->P.list[0]);
    if(prior<0){
      assert(f->error.code);
      rc = f->error.code;
      goto end;
    }
  }
  subsequent = fsl_db_g_id(db, 0,
  /* BUG: see:
     https://fossil-scm.org/forum/forumpost/c58fd8de53 */
                           "SELECT rid FROM tagxref"
                           " WHERE tagid=%"FSL_ID_T_PFMT
                           " AND mtime>=%"FSL_JULIAN_T_PFMT
                           " AND rid!=%"FSL_ID_T_PFMT
                           " ORDER BY mtime",
                           tagid, d->D, d->rid);
  if(subsequent<0){
    assert(db->error.code);
    rc = fsl_cx_uplift_db_error(f, db);
    goto end;
  }
  else if( prior > 0 ){
    rc = fsl_content_deltify(f, prior, d->rid, 0);
    if( !rc && !subsequent ){
      rc = fsl_db_exec(db,
                       "DELETE FROM event"
                       " WHERE type='e'"
                       "   AND tagid=%"FSL_ID_T_PFMT
                       "   AND objid IN"
                       " (SELECT rid FROM tagxref "
                       " WHERE tagid=%"FSL_ID_T_PFMT")",
                       tagid, tagid);
    }
  }
  if(rc) goto end;
  if( subsequent>0 ){
    rc = fsl_content_deltify(f, d->rid, subsequent, 0);
  }else{
    /* timeline update is deferred to another crosslink
       handler */
    rc = fsl_search_doc_touch(f, d->type, d->rid, 0);
    /* FSL-MISSING:
       assert( manifest_event_triggers_are_enabled ); */
  }
  if(!rc){
    rc = fsl_deck_crosslink_fwt_plink(d);
  }
  end:
  return rc;
}

static int fsl_deck_crosslink_ticket(fsl_deck * const d){
  int rc;
  fsl_cx * const f = d->f;
#if 0
  fsl_db * const db = fsl_cx_db_repo(f);
#endif
  /*
    TODO: huge block from manifest_crosslink().  A full port
    requires other infrastructure for collapsing relatively close
    time values into the same time for timeline purposes. i'd prefer
    to farm this out to a crosslink callback.  Even then, the future
    of tickets in libfossil is uncertain, but we should crosslink
    them so that repos stay compatible with fossil(1) without
    requiring a rebuild using fossil(1).
  */
  if(f->flags & FSL_CX_F_SKIP_UNKNOWN_CROSSLINKS){
    return 0;
  }
  rc = fsl_cx_err_set(f, FSL_RC_NYI,
                      "MISSING: a huge block of TICKET stuff from "
                      "manifest_crosslink(). It requires infrastructure "
                      "libfossil does not yet have.");
  return rc;
}

int fsl_deck_crosslink_one( fsl_deck * d ){
  int rc;
  if(!d || !d->f) return FSL_RC_MISUSE;
  rc = fsl_crosslink_begin(d->f);
  if(rc) return rc;
  rc = fsl_deck_crosslink(d);
  if(rc){
    fsl_db_transaction_rollback(fsl_cx_db_repo(d->f))
      /* Ignore result - keep existing error state */;
    d->f->cache.isCrosslinking = false;
  }else{
    assert(fsl_db_transaction_level(fsl_cx_db_repo(d->f)));
    rc = fsl_crosslink_end(d->f);
  }
  return rc;
}

int fsl_deck_crosslink( fsl_deck /* const */ * d ){
  int rc = 0;
  fsl_cx * f = d->f;
  fsl_db * db = f ? fsl_needs_repo(f) : NULL;
  fsl_id_t parentid = 0;
  fsl_int_t const rid = d->rid;
  if(!f) return FSL_RC_MISUSE;
  else if(rid<=0){
    return fsl_cx_err_set(f, FSL_RC_RANGE,
                          "Invalid RID for crosslink: %"FSL_ID_T_PFMT,
                          rid);
  }
  else if(!db) return FSL_RC_NOT_A_REPO;
  else if(!fsl_deck_has_required_cards(d)){
    assert(d->f->error.code);
    return d->f->error.code;
  }else if(f->cache.xlinkClustersOnly && (FSL_SATYPE_CLUSTER!=d->type)){
    /* is it okay to bypass the registered xlink listeners here?  The
       use case called for by this is not yet implemented in
       libfossil. */
    return 0;
  }
  if(FSL_SATYPE_CHECKIN==d->type){
    if(d->B.uuid && !d->B.baseline){
      rc = fsl_deck_baseline_fetch(d);
      if(rc) goto end;
      assert(d->B.baseline);
    }
  }
  rc = fsl_db_transaction_begin(db);
  if(rc) goto end;
  switch(d->type){
    case FSL_SATYPE_CHECKIN:
      rc = fsl_deck_crosslink_checkin(d, &parentid);
      break;
    case FSL_SATYPE_CLUSTER:
      rc = fsl_deck_crosslink_cluster(d);
      break;
    default:
      break;
  }
  if(rc) goto end;
  switch(d->type){
    case FSL_SATYPE_CONTROL:
    case FSL_SATYPE_CHECKIN:
    case FSL_SATYPE_TECHNOTE:
      rc = fsl_deck_crosslink_apply_tags(f, d, db, rid, parentid);
      break;
    default:
      break;
  }
  if(rc) goto end;
  switch(d->type){
    case FSL_SATYPE_WIKI:
      rc = fsl_deck_crosslink_wiki(d);
      break;
    case FSL_SATYPE_FORUMPOST:
      rc = fsl_deck_crosslink_forum(d);
      break;
    case FSL_SATYPE_TECHNOTE:
      rc = fsl_deck_crosslink_technote(d);
      break;
    case FSL_SATYPE_TICKET:
      rc = fsl_deck_crosslink_ticket(d);
      break;
    case FSL_SATYPE_ATTACHMENT:
      rc = fsl_deck_crosslink_attachment(d);
      break;
    /* FSL_SATYPE_CONTROL is handled above except for the timeline
       update, which is handled by a callback below */
    default:
      break;
  }
  if(rc) goto end;

  /* Call any crosslink callbacks... */
  if(f->xlinkers.list){
    fsl_size_t i;
    fsl_xlinker * xl = NULL;
    for( i = 0; !rc && (i < f->xlinkers.used); ++i ){
      xl = f->xlinkers.list+i;
      rc = xl->f( d, xl->state );
    }
    if(rc){
      assert(xl);
      if(!f->error.code){
        fsl_cx_err_set(f, rc, "Crosslink callback handler "
                       "'%s' failed with code %d (%s) for "
                       "artifact [%.12s].",
                       xl->name, rc, fsl_rc_cstr(rc),
                       d->uuid);
      }
    }
  }/*end crosslink callbacks*/
  end:
  if(!rc){
    rc = fsl_db_transaction_end(db, false);
  }else{
    if(db->error.code && !f->error.code){
      fsl_cx_uplift_db_error(f,db);
    }
    fsl_db_transaction_end(db, true);
  }
  return rc;
}/*end fsl_deck_crosslink()*/



/**
    Return true if z points to the first character after a blank line.
    Tolerate either \r\n or \n line endings. As this looks backwards
    in z, z must point to at least 3 characters past the beginning of
    a legal string.
 */
static bool fsl_after_blank_line(const char *z){
  if( z[-1]!='\n' ) return false;
  if( z[-2]=='\n' ) return true;
  if( z[-2]=='\r' && z[-3]=='\n' ) return true;
  return false;
}

/**
    Verifies that ca points to at least 35 bytes of memory
    which hold (at the end) a Z card and its hash value.
   
    Returns 0 if the string does not contain a Z card,
    a positive value if it can validate the Z card's hash,
    and a negative value on hash mismatch.
*/
static int fsl_deck_verify_Z_card(unsigned char const * ca, fsl_size_t n){
  if( n<35 ) return 0;
  if( ca[n-35]!='Z' || ca[n-34]!=' ' ) return 0;
  else{
    unsigned char digest[16];
    char hex[FSL_STRLEN_MD5+1];
    unsigned char const * zHash = ca+n-FSL_STRLEN_MD5-1;
    fsl_md5_cx md5 = fsl_md5_cx_empty;
    unsigned char const * zHashEnd =
      ca + n -
      2 /* 'Z ' */
      - FSL_STRLEN_MD5
      - 1 /* \n */;
    assert( 'Z' == (char)*zHashEnd );
    fsl_md5_update(&md5, ca, zHashEnd-ca);
    fsl_md5_final(&md5, digest);
    fsl_md5_digest_to_base16(digest, hex);
    return (0==memcmp(zHash, hex, FSL_STRLEN_MD5))
      ? 1
      : -1;
  }
}

void fsl_remove_pgp_signature(unsigned char const **pz, fsl_size_t *pn){
  unsigned char const *z = *pz;
  fsl_int_t n = (fsl_int_t)*pn;
  fsl_int_t i;
  if( n<59 || memcmp(z, "-----BEGIN PGP SIGNED MESSAGE-----", 34)!=0 ) return;
  for(i=34; i<n && !fsl_after_blank_line((char const *)(z+i)); i++){}
  if( i>=n ) return;
  z += i;
  n -= i;
  *pz = z;
  for(i=n-1; i>=0; i--){
    if( z[i]=='\n' && memcmp(&z[i],"\n-----BEGIN PGP SIGNATURE-", 25)==0 ){
      n = i+1;
      break;
    }
  }
  *pn = (fsl_size_t)n;
  return;
}


/**
    Internal helper for parsing manifests. Holds a source file (memory
    range) and gets updated by fsl_deck_next_token() and friends.
*/
struct fsl_src {
  /**
      First char of the next token.
   */
  unsigned char * z;
  /**
      One-past-the-end of the manifest.
   */
  unsigned char * zEnd;
  /**
      True if z points to the start of a new line.
   */
  char atEol;
};
typedef struct fsl_src fsl_src;
static const fsl_src fsl_src_empty = {NULL,NULL,0};

/**
   Return a pointer to the next token.  The token is zero-terminated.
   Return NULL if there are no more tokens on the current line.  If
   pLen is not NULL and this function returns non-NULL then *pLen is
   set to the byte length of the new token.
*/
static unsigned char *fsl_deck_next_token(fsl_src *p, fsl_size_t *pLen){
  unsigned char *z;
  unsigned char *zStart;
  int c;
  if( p->atEol ) return NULL;
  zStart = z = p->z;
  while( (c=(*z))!=' ' && c!='\n' ){ ++z; }
  *z = 0;
  p->z = &z[1];
  p->atEol = c=='\n';
  if( pLen ) *pLen = z - zStart;
  return zStart;
}

/**
    Return the card-type for the next card. Return 0 if there are no
    more cards or if we are not at the end of the current card.
 */
static unsigned char mf_next_card(fsl_src *p){
  unsigned char c;
  if( !p->atEol || p->z>=p->zEnd ) return 0;
  c = p->z[0];
  if( p->z[1]==' ' ){
    p->z += 2;
    p->atEol = 0;
  }else if( p->z[1]=='\n' ){
    p->z += 2;
    p->atEol = 1;
  }else{
    c = 0;
  }
  return c;
}

/**
    Internal helper for fsl_deck_parse(). Expects l to be an array of
    26 entries, representing the letters of the alphabet (A-Z), with a
    value of 0 if the card was not seen during parsing and a value >0
    if it was. Returns the deduced artifact type.  Returns
    FSL_SATYPE_ANY if the result is ambiguous.

    Note that we cannot reliably guess until we've seen at least 3
    cards. 2 cards is enough for most cases but can lead to
    FSL_SATYPE_CHECKIN being prematurely selected in one case.
   
    It should guess right for any legal manifests, but it does not go
    out of its way to detect incomplete/invalid ones.
 */
static fsl_satype_e fsl_deck_guess_type( const int * l ){
#if 0
  /* For parser testing only... */
  int i;
  assert(!l[26]);
  MARKER(("Cards seen during parse:\n"));
  for( i = 0; i < 26; ++i ){
    if(l[i]) putchar('A'+i);
  }
  putchar('\n');
#endif
  /*
     Now look for combinations of cards which will uniquely
     identify any syntactical legal combination of cards.
     
     A larger brain than mine could probably come up with a hash of
     l[] which could determine this in O(1). But please don't
     reimplement this as such unless mere mortals can maintain it -
     any performance gain is insignificant in the context of the
     underlying SCM/db operations.

     Note that the order of these checks is sometimes significant!
  */
#define L(X) l[X-'A']
  if(L('M')) return FSL_SATYPE_CLUSTER;
  else if(L('E')) return FSL_SATYPE_EVENT;
  else if(L('G') || L('H') || L('I')) return FSL_SATYPE_FORUMPOST;
  else if(L('L') || L('W')) return FSL_SATYPE_WIKI;
  else if(L('J') || L('K')) return FSL_SATYPE_TICKET;
  else if(L('A')) return FSL_SATYPE_ATTACHMENT;
  else if(L('B') || L('C') || L('F')
          || L('P') || L('Q') || L('R')) return FSL_SATYPE_CHECKIN;
  else if(L('D') && L('T') && L('U')) return FSL_SATYPE_CONTROL;
#undef L
  return FSL_SATYPE_ANY;
}

bool fsl_might_be_artifact(fsl_buffer const * src){
  unsigned const char * z = src->mem;
  fsl_size_t n = src->used;
  if(n<36) return 0;
  fsl_remove_pgp_signature(&z, &n);
  if(n<36) return 0;
  else if(z[0]<'A' || z[0]>'Z' || z[1]!=' '
          || z[n-35]!='Z'
          || z[n-34]!=' '
          || !fsl_validate16((const char *)z+n-33, FSL_STRLEN_MD5)){
    return 0;
  }
  return 1;
}

int fsl_deck_parse2(fsl_deck * d, fsl_buffer * src, fsl_id_t rid){
#ifdef ERROR
#  undef ERROR
#endif
#define ERROR(RC,MSG) do{ rc = (RC); zMsg = (MSG); goto bailout; } while(0)
#define SYNTAX(MSG) ERROR(rc ? rc : FSL_RC_SYNTAX,MSG)
  bool isRepeat = 0/* , hasSelfRefTag = 0 */;
  int rc = 0;
  fsl_src x = fsl_src_empty;
  char const * zMsg = NULL;
  fsl_id_bag * seen;
  char cType = 0, cPrevType = 0;
  unsigned char * z = src ? src->mem : NULL;
  fsl_size_t tokLen = 0;
  unsigned char * token;
  fsl_size_t n = z ? src->used : 0;
  unsigned char * uuid;
  double ts;
  int cardCount = 0;
  fsl_db * db;
  fsl_cx * f;
  fsl_error * err;
  int stealBuf = 0 /* gets incremented if we need to steal src->mem. */;
  unsigned nSelfTag = 0 /* number of T cards which refer to '*' (this artifact). */;
  unsigned nSimpleTag = 0 /* number of T cards with "+" prefix */;
  /*
    lettersSeen keeps track of the card letters we have seen so that
    we can then relatively quickly figure out what type of manifest we
    have parsed without having to inspect the card contents. Each
    index records a count of how many of that card we've seen.
  */
  int lettersSeen[27] = {0,0,0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0};
  if(!d->f || !z) return FSL_RC_MISUSE;
  /* Every control artifact ends with a '\n' character.  Exit early
     if that is not the case for this artifact. */
  f = d->f;
  err = &f->error;
  if(!*z || !n || ( '\n' != z[n-1]) ){
    return fsl_error_set(err, FSL_RC_SYNTAX, "%s.",
                         n ? "Not terminated with \\n"
                         : "Zero-length input");
  }
  else if(rid<0){
    return fsl_error_set(err, FSL_RC_RANGE,
                         "Invalid (negative) RID %"FSL_ID_T_PFMT
                         " for fsl_deck_parse()", rid);
  }
  db = fsl_cx_db_repo(f);
  seen = f ? &f->cache.mfSeen : NULL;
  if(seen){
    if((0==rid) || fsl_id_bag_contains(seen,rid)){
      isRepeat = 1;
    }else{
      isRepeat = 0;
      rc = fsl_id_bag_insert(seen, rid);
      if(rc){
        assert(FSL_RC_OOM==rc);
        return rc;
      }
    }
  }
  fsl_deck_clean(d);
  fsl_deck_init(f, d, FSL_SATYPE_ANY);
  /* We have to hash BEFORE parsing, as parsing modifies
     the input */
  if(rid){
    assert(rid>0);
    d->rid = rid;
    d->uuid = fsl_rid_to_uuid(f, rid);
    if(!d->uuid){
      rc = f->error.code ? f->error.code : FSL_RC_OOM;
      goto end;
    }
  }else if(db){
    rc = fsl_repo_blob_lookup(f, src, &d->rid, &d->uuid);
    if(FSL_RC_NOT_FOUND==rc){
      /* Non-fatal */
      rc = 0;
    }
  }else{
    fsl_buffer hash = fsl_buffer_empty;
    rc = fsl_cx_hash_buffer(f, false, src, &hash);
    if(rc){
      fsl_buffer_clear(&hash);
    }else{
      d->uuid = fsl_buffer_str(&hash);
    }
  }
  if(rc) return rc;

  /* legacy: not yet clear if we need this:
     if( !isRepeat ) g.parseCnt[0]++; */

  /*
    Verify that the first few characters of the artifact look like a
    control artifact.
  */
  if( !fsl_might_be_artifact(src) ){
    ERROR(FSL_RC_SYNTAX, "Content does not look like "
          "a structural artifact");
  }

  /*
    Strip off the PGP signature if there is one. Example of signed
    manifest:

    https://fossil-scm.org/index.html/artifact/28987096ac
  */
  {
    unsigned char const * zz = z;
    fsl_remove_pgp_signature(&zz, &n);
    z = (unsigned char *)zz;
  }

  /* Verify the Z card */
  if( fsl_deck_verify_Z_card(z, n) < 0 ){
    ERROR(FSL_RC_CONSISTENCY, "Z-card checksum mismatch");
  }

  /*
    Reminder: parsing modifies the input (to simplify the
    tokenization/parsing).

    As of mid-201403, we recycle as much as possible from the source
    buffer and take over ownership _if_ we do so.
  */
  /* Now parse, card by card... */
  x.z = z;
  x.zEnd = z+n;
  x.atEol= 1;

  /* Parsing helpers... */
#define TOKEN(DEFOS) tokLen=0; token = fsl_deck_next_token(&x,&tokLen);    \
  if(token && tokLen && (DEFOS)) fsl_bytes_defossilize(token, &tokLen)
#define TOKEN_EXISTS(MSG_IF_NOT) if(!token){ SYNTAX(MSG_IF_NOT); }(void)0
#define TOKEN_CHECKHEX(MSG) if(token && (int)tokLen!=fsl_is_uuid((char const *)token))\
  { SYNTAX(MSG); }
#define TOKEN_UUID(CARD) TOKEN_CHECKHEX("Malformed UUID in " #CARD "-card")
#define TOKEN_MD5(ERRMSG) if(!token || FSL_STRLEN_MD5!=(int)tokLen) \
  {SYNTAX(ERRMSG);}
  /**
     Reminder: we do not know the type of the manifest at this point,
     so all of the fsl_deck_add/set() bits below can't do their
     validation. We have to determine at parse-time (or afterwards)
     which type of deck it is based on the cards we've seen. We guess
     the type as early as possible to enable during-parse validation,
     and do a post-parse check for the legality of cards added before
     validation became possible.
   */
  
#define SEEN(CARD) lettersSeen[*#CARD - 'A']
  for( cPrevType=1; !rc && (0 < (cType = mf_next_card(&x)));
       cPrevType = cType ){
    ++cardCount;
    if(cType<cPrevType){
      if(d->E.uuid && 'N'==cType && 'P'==cPrevType){
        /* Workaround for a pair of historical fossil bugs
           which synergized to allow malformed technotes to
           be saved:
           https://fossil-scm.org/home/info/023fddeec4029306 */
      }else{
        SYNTAX("Cards are not in strict lexical order");
      }
    }
    assert(cType>='A' && cType<='Z');
    if(cType>='A' && cType<='Z'){
        ++lettersSeen[cType-'A'];
    }else{
      SYNTAX("Invalid card name");
    }
    switch(cType){
      /*
             A <filename> <target> ?<source>?
        
         Identifies an attachment to either a wiki page, a ticket, or
         a technote.  <source> is the artifact that is the attachment.
         <source> is omitted to delete an attachment.  <target> is the
         name of a wiki page, technote, or ticket to which that
         attachment is connected.
      */
      case 'A':{
        unsigned char * name, * src;
        if(1<SEEN(A)){
          ERROR(FSL_RC_RANGE,"Multiple A-cards");
        }
        TOKEN(1);
        TOKEN_EXISTS("Missing filename for A-card");
        name = token;
        if(!fsl_is_simple_pathname( (char const *)name, 0 )){
          SYNTAX("Invalid filename in A-card");
        }          
        TOKEN(1);
        TOKEN_EXISTS("Missing target name in A-card");
        uuid = token;
        TOKEN(0);
        TOKEN_UUID(A);
        src = token;
        d->A.name = (char *)name;
        d->A.tgt = (char *)uuid;
        d->A.src = (char *)src;
        ++stealBuf;
        /*rc = fsl_deck_A_set(d, (char const *)name,
          (char const *)uuid, (char const *)src);*/
        d->type = FSL_SATYPE_ATTACHMENT;
        break;
      }
      /*
            B <uuid>
        
         A B-line gives the UUID for the baseline of a delta-manifest.
      */
      case 'B':{
        if(d->B.uuid){
          SYNTAX("Multiple B-cards");
        }
        TOKEN(0);
        TOKEN_UUID(B);
        d->B.uuid = (char *)token;
        ++stealBuf;
        d->type = FSL_SATYPE_CHECKIN;
        /* rc = fsl_deck_B_set(d, (char const *)token); */
        break;
      }
      /*
             C <comment>
        
         Comment text is fossil-encoded.  There may be no more than
         one C line.  C lines are required for manifests, are optional
         for Events and Attachments, and are disallowed on all other
         control files.
      */
      case 'C':{
        if( d->C ){
          SYNTAX("more than one C-card");
        }
        TOKEN(1);
        TOKEN_EXISTS("Missing comment text for C-card");
        /* rc = fsl_deck_C_set(d, (char const *)token, (fsl_int_t)tokLen); */
        d->C = (char *)token;
        ++stealBuf;
        break;
      }
      /*
             D <timestamp>
        
         The timestamp should be ISO 8601.   YYYY-MM-DDtHH:MM:SS
         There can be no more than 1 D line.  D lines are required
         for all control files except for clusters.
      */
      case 'D':{
#define TOKEN_DATETIME(LETTER,MEMBER)                                     \
        if( d->MEMBER>0.0 ) { SYNTAX("More than one "#LETTER"-card"); } \
        TOKEN(0); \
        TOKEN_EXISTS("Missing date part of "#LETTER"-card"); \
        if(!fsl_str_is_date((char const *)token)){\
          SYNTAX("Malformed date part of "#LETTER"-card"); \
        } \
        if(!fsl_iso8601_to_julian((char const *)token, &ts)){   \
          SYNTAX("Cannot parse date from "#LETTER"-card"); \
        } (void)0

        TOKEN_DATETIME(D,D);
        rc = fsl_deck_D_set(d, ts);
        break;
      }
      /*
             E <timestamp> <uuid>
        
         An "event" card that contains the timestamp of the event in the 
         format YYYY-MM-DDtHH:MM:SS and a unique identifier for the event.
         The event timestamp is distinct from the D timestamp.  The D
         timestamp is when the artifact was created whereas the E timestamp
         is when the specific event is said to occur.
      */
      case 'E':{
        TOKEN_DATETIME(E,E.julian);
        TOKEN(0);
        TOKEN_EXISTS("Missing UUID part of E-card");
        TOKEN_UUID(E);
        d->E.julian = ts;
        d->E.uuid = (char *)token;
        ++stealBuf;
        d->type = FSL_SATYPE_EVENT;
        break;
      }
      /*
             F <filename> ?<uuid>? ?<permissions>? ?<old-name>?
        
         Identifies a file in a manifest.  Multiple F lines are
         allowed in a manifest.  F lines are not allowed in any other
         control file.  The filename and old-name are fossil-encoded.

         In delta manifests, deleted files are denoted by the 1-arg
         form. In baseline manifests, deleted files simply are not in
         the manifest.
      */
      case 'F':{
        char * name;
        char * perms = NULL;
        char * priorName = NULL;
        fsl_fileperm_e perm = FSL_FILE_PERM_REGULAR;
        fsl_card_F * fc = NULL;
        /**
           Basic tests with various repos have shown that the
           approximate number of F-cards in a manifest is rougly the
           manifest size/75. We'll use that as an initial alloc size.
        */
        rc = 0;
        if(!d->F.capacity){
          rc = fsl_card_F_list_reserve(&d->F, src->used/75+10);
        }
        TOKEN(0);
        TOKEN_EXISTS("Missing name for F-card");
        name = (char *)token;
        TOKEN(0);
        TOKEN_UUID(F);
        uuid = token;
        TOKEN(0);
        if(token){
          perms = (char *)token;
          switch(*perms){
            case 0:
              /* Some (maybe only 1) ancient fossil(1) artifact(s) have a trailing
                 space which triggers this. e.g.

                 https://fossil-scm.org/home/info/32b480faa3465591b8549bdfd889d62d7a8d16a8
              */
              break;
            case 'w': perm = FSL_FILE_PERM_REGULAR; break;
            case 'x': perm = FSL_FILE_PERM_EXE; break;
            case 'l': perm = FSL_FILE_PERM_LINK; break;
            default:
              /*MARKER(("Unmatched perms string character: %d / %c !", (int)*perms, *perms));*/
              assert(!"Unmatched perms string character!");
              ERROR(FSL_RC_ERROR,"Internal error: unmatched perms string character");
          }
          TOKEN(0);
          if(token) priorName = (char *)token;
        }
        fsl_bytes_defossilize( (unsigned char *)name, 0 );
        if(priorName) fsl_bytes_defossilize( (unsigned char *)priorName, 0 );
        if(fsl_is_reserved_fn(name, -1)){
          /* Some historical (pre-late-2020) manifests contain files
             they really shouldn't, like _FOSSIL_ and .fslckout.
             Since late 2020, fossil simply skips over these when
             parsing manifests, so we'll do the same. */
          break;
        }
        fc = rc ? 0 : fsl_card_F_list_push(&d->F);
        if(!fc){
          zMsg = "OOM";
          goto bailout;
        }
        ++stealBuf;
        assert(d->F.used>1
               ? (FSL_CARD_F_LIST_NEEDS_SORT & d->F.flags)
               : 1);
        fc->deckOwnsStrings = true;
        fc->name = name;
        fc->priorName = priorName;
        fc->perm = perm;
        fc->uuid = (fsl_uuid_str)uuid;
        d->type = FSL_SATYPE_CHECKIN;
        break;
      }
      /*
        G <uuid>
        
        A G-line gives the UUID for the thread root of a forum post.
      */
      case 'G':{
        if(d->G){
          SYNTAX("Multiple G-cards");
        }
        TOKEN(0);
        TOKEN_EXISTS("Missing UUID in G-card");
        TOKEN_UUID(G);
        d->G = (char*)token;
        ++stealBuf;
        d->type = FSL_SATYPE_FORUMPOST;
        break;
      }
     /*
         H <forum post title>
        
         H text is fossil-encoded.  There may be no more than one H
         line.  H lines are optional for forum posts and are
         disallowed on all other control files.
      */
      case 'H':{
        if( d->H ){
          SYNTAX("more than one H-card");
        }
        TOKEN(1);
        TOKEN_EXISTS("Missing text for H-card");
        d->H = (char *)token;
        ++stealBuf;
        d->type = FSL_SATYPE_FORUMPOST;
        break;
      }
      /*
        I <uuid>
        
        A I-line gives the UUID for the in-response-to UUID for 
        a forum post.
      */
      case 'I':{
        if(d->I){
          SYNTAX("Multiple I-cards");
        }
        TOKEN(0);
        TOKEN_EXISTS("Missing UUID in I-card");
        TOKEN_UUID(I);
        d->I = (char*)token;
        ++stealBuf;
        d->type = FSL_SATYPE_FORUMPOST;
        break;
      }
      /*
             J <name> ?<value>?
        
         Specifies a name value pair for ticket.  If the first character
         of <name> is "+" then the <value> is appended to any preexisting
         value.  If <value> is omitted then it is understood to be an
         empty string.
      */
      case 'J':{
        char const * field;
        bool isAppend = 0;
        TOKEN(1);
        TOKEN_EXISTS("Missing field name for J-card");
        field = (char const *)token;
        if('+'==*field){
          isAppend = 1;
          ++field;
        }
        TOKEN(1);
        rc = fsl_deck_J_add(d, isAppend, field,
                            (char const *)token);
        d->type = FSL_SATYPE_TICKET;
        break;
      }
      /*
            K <uuid>
        
         A K-line gives the UUID for the ticket which this control file
         is amending.
      */
      case 'K':{
        if(d->K){
          SYNTAX("Multiple K-cards");
        }
        TOKEN(0);
        TOKEN_EXISTS("Missing UUID in K-card");
        TOKEN_UUID(K);
        d->K = (char*)token;
        ++stealBuf;
        d->type = FSL_SATYPE_TICKET;
        break;
      }
      /*
             L <wikititle>
        
         The wiki page title is fossil-encoded.  There may be no more than
         one L line.
      */
      case 'L':{
        if(d->L){
          SYNTAX("Multiple L-cards");
        }
        TOKEN(1);
        TOKEN_EXISTS("Missing text for L-card");
        d->L = (char*)token;
        ++stealBuf;
        d->type = FSL_SATYPE_WIKI;
        break;
      }
      /*
            M <uuid>
        
         An M-line identifies another artifact by its UUID.  M-lines
         occur in clusters only.
      */
      case 'M':{
        TOKEN(0);
        TOKEN_EXISTS("Missing UUID for M-card");
        TOKEN_UUID(M);
        ++stealBuf;
        d->type = FSL_SATYPE_CLUSTER;
        rc = fsl_list_append(&d->M, token);
        if( !rc && d->M.used>1 &&
            fsl_strcmp((char const *)d->M.list[d->M.used-2],
                       (char const *)token)>=0 ){
          SYNTAX("M-card in the wrong order");
        }
        break;
      }
      /*
            N <uuid>
        
         An N-line identifies the mimetype of wiki or comment text.
      */
      case 'N':{
        if(1<SEEN(N)){
          ERROR(FSL_RC_RANGE,"Multiple N-cards");
        }
        TOKEN(0);
        TOKEN_EXISTS("Missing UUID on N-card");
        ++stealBuf;
        d->N = (char *)token;
        break;
      }

      /*
             P <uuid> ...
        
         Specify one or more other artifacts which are the parents of
         this artifact.  The first parent is the primary parent.  All
         others are parents by merge.
      */
      case 'P':{
        if(1<SEEN(P)){
          ERROR(FSL_RC_RANGE,"More than one P-card");
        }
        TOKEN(0);
#if 0
        /* The docs all claim that this card does not exist on the first
           manifest, but in fact it does exist but has no UUID,
           which is invalid per all the P-card docs. Skip this
           check (A) for the sake of manifest #1 and (B) because
           fossil(1) does it this way.
        */
        TOKEN_EXISTS("Missing primary parent UUID for P-card");
#endif
        while( token && !rc ){
          TOKEN_UUID(P);
          ++stealBuf;
          rc = fsl_list_append(&d->P, token);
          if(!rc){
            TOKEN(0);
          }
        }
        break;
      }
      /*
             Q (+|-)<uuid> ?<uuid>?
        
         Specify one or a range of checkins that are cherrypicked into
         this checkin ("+") or backed out of this checkin ("-").
      */
      case 'Q':{
        fsl_cherrypick_type_e qType = FSL_CHERRYPICK_INVALID;
        TOKEN(0);
        TOKEN_EXISTS("Missing target UUID for Q-card");
        switch((char)*token){
          case '-': qType = FSL_CHERRYPICK_BACKOUT; break;
          case '+': qType = FSL_CHERRYPICK_ADD; break;
          default:
            SYNTAX("Malformed target UUID in Q-card");
        }
        assert(qType);
        uuid = ++token; --tokLen;
        TOKEN_UUID(Q);
        TOKEN(0);
        if(token){
          TOKEN_UUID(Q);
        }
        d->type = FSL_SATYPE_CHECKIN;
        rc = fsl_deck_Q_add(d, qType, (char const *)uuid,
                            (char const *)token);
        break;
      }
      /*
             R <md5sum>
        
         Specify the MD5 checksum over the name and content of all files
         in the manifest.
      */
      case 'R':{
        if(1<SEEN(R)){
          ERROR(FSL_RC_RANGE,"More than one R-card");
        }
        TOKEN(0);
        TOKEN_EXISTS("Missing MD5 token in R-card");
        TOKEN_MD5("Malformed MD5 token in R-card");
        d->R = (char *)token;
        ++stealBuf;
        d->type = FSL_SATYPE_CHECKIN;
        break;
      }
      /*
            T (+|*|-)<tagname> <uuid> ?<value>?
        
         Create or cancel a tag or property.  The tagname is fossil-encoded.
         The first character of the name must be either "+" to create a
         singleton tag, "*" to create a propagating tag, or "-" to create
         anti-tag that undoes a prior "+" or blocks propagation of of
         a "*".
        
         The tag is applied to <uuid>.  If <uuid> is "*" then the tag is
         applied to the current manifest.  If <value> is provided then 
         the tag is really a property with the given value.
        
         Tags are not allowed in clusters.  Multiple T lines are allowed.
      */
      case 'T':{
        unsigned char * name, * value;
        fsl_tagtype_e tagType = FSL_TAGTYPE_INVALID;
        TOKEN(1);
        TOKEN_EXISTS("Missing name for T-card");
        name = token;
        if( fsl_validate16((char const *)&name[1],
                           fsl_strlen((char const *)&name[1])) ){
          /* Do not allow tags whose names look like a hash */
          SYNTAX("T-card name looks like a hexadecimal hash");
        }
        TOKEN(0);
        TOKEN_EXISTS("Missing UUID on T-card");
        if(fsl_is_uuid_len((int)tokLen)){
          TOKEN_UUID(T);
          uuid = token;
        }else if( 1==tokLen && '*'==(char)*token ){
          /* tag for the current artifact */
          ++nSelfTag;
          uuid = NULL;
        }else{
          SYNTAX("Malformed UUID in T-card");
        }
        TOKEN(1);
        value = token;
        switch(*name){
          case '*': tagType = FSL_TAGTYPE_PROPAGATING; break;
          case '+': tagType = FSL_TAGTYPE_ADD;
            ++nSimpleTag;
            break;
          case '-': tagType = FSL_TAGTYPE_CANCEL; break;
          default: SYNTAX("Malformed tag name");
        }
        ++name /* skip type marker byte */;
        /* Potential todo: add the order check from this commit:

        https://fossil-scm.org/index.html/info/55cacfcace
        */
        rc = fsl_deck_T_add(d, tagType, (fsl_uuid_cstr)uuid,
                            (char const *)name,
                            (char const *)value);
        break;
      }
      /*
             U ?<login>?
        
         Identify the user who created this control file by their
         login.  Only one U line is allowed.  Prohibited in clusters.
         If the user name is omitted, take that to be "anonymous".
      */
      case 'U':{
        if(d->U) SYNTAX("More than one U-card");
        TOKEN(1);
        if(token){
          /* rc = fsl_deck_U_set( d, (char const *)token, (fsl_int_t)tokLen ); */
          ++stealBuf;
          d->U = (char *)token;
        }else{
          rc = fsl_deck_U_set( d, "anonymous" );
        }
        break;
      }
      /*
             W <size>
        
         The next <size> bytes of the file contain the text of the wiki
         page.  There is always an extra \n before the start of the next
         record.
      */
      case 'W':{
        fsl_size_t wlen;
        if(d->W.used){
          SYNTAX("More than one W-card");
        }
        TOKEN(0);
        TOKEN_EXISTS("Missing size token for W-card");
        wlen = fsl_str_to_size((char const *)token);
        if((fsl_size_t)-1==wlen){
          ERROR(FSL_RC_RANGE,"Wiki size token is invalid");
        }
        if( (&x.z[wlen+1]) > x.zEnd){
          SYNTAX("Not enough content after W-card");
        }
        rc = fsl_buffer_append(&d->W, x.z, wlen);
        if(rc) goto bailout;
        x.z += wlen;
        if( '\n' != x.z[0] ){
          SYNTAX("W-card content not \\n terminated");
        }
        x.z[0] = 0;
        ++x.z;
        break;
      }
      /*
             Z <md5sum>
        
         MD5 checksum on this control file.  The checksum is over all
         lines (other than PGP-signature lines) prior to the current
         line.  This must be the last record.
        
         This card is required for all control file types except for
         Manifest. It is not required for manifest only for historical
         compatibility reasons.
      */
      case 'Z':{
        /* We validated the Z card first. We cannot compare against
           the original blob now because we've modified it.
        */
        goto end;
      }
      default:
        rc = fsl_cx_err_set(f, FSL_RC_SYNTAX,
                            "Unknown card '%c' in manifest",
                            cType);
        goto bailout;
    }/*switch(cType)*/
    if(rc) goto bailout;
  }/* for-each-card */

#if 1
  /* Remove these when we are done porting
     resp. we can avoid these unused-var warnings. */
  if(isRepeat){}
#endif

  end:
  assert(0==rc);
  if(cardCount>2 && FSL_SATYPE_ANY==d->type){
    /* See if we need to guess the type now.
       We need(?) at least two card to ensure that this is
       free of ambiguities. */
    d->type = fsl_deck_guess_type(lettersSeen);
    if(FSL_SATYPE_ANY!=d->type){
      assert(FSL_SATYPE_INVALID!=d->type);
#if 0
      MARKER(("Guessed manifest type with %d cards: %s\n",
                cardCount, fsl_satype_cstr(d->type)));
#endif
    }
  }
  /* Make sure all of the cards we put in it belong to that deck
     type. */
  if( !fsl_deck_check_type(d, cType) ){
    rc = d->f->error.code;
    goto bailout;
  }

  if(FSL_SATYPE_ANY==d->type){
    rc = fsl_cx_err_set(f, FSL_RC_ERROR,
                        "Internal error: could not determine type of "
                        "control artifact we just (successfully!) "
                        "parsed.");
    goto bailout;
  }else {
    /*
      Make sure we didn't pick up any cards which were picked up
      before d->type was guessed and are invalid for the post-guessed
      type.
    */
    int i = 0;
    for( ; i < 27; ++i ){
      if((lettersSeen[i]>0) && !fsl_card_is_legal(d->type, 'A'+i )){
        rc = fsl_cx_err_set(f, FSL_RC_SYNTAX,
                            "Determined during post-parse processing that "
                            "the parsed deck (type %s) contains an illegal "
                            "card type (%c).", fsl_satype_cstr(d->type),
                            'A'+i);
        goto bailout;
      }
    }
  }
  assert(FSL_SATYPE_CHECKIN==d->type ||
         FSL_SATYPE_CLUSTER==d->type ||
         FSL_SATYPE_CONTROL==d->type ||
         FSL_SATYPE_WIKI==d->type ||
         FSL_SATYPE_TICKET==d->type ||
         FSL_SATYPE_ATTACHMENT==d->type ||
         FSL_SATYPE_TECHNOTE==d->type ||
         FSL_SATYPE_FORUMPOST==d->type);
  assert(0==rc);

  /* Additional checks based on artifact type */
  switch( d->type ){
    case FSL_SATYPE_CONTROL: {
      if( nSelfTag ){
        SYNTAX("self-referential T-card in control artifact");
      }
      break;
    }
    case FSL_SATYPE_TECHNOTE: {
      if( d->T.used!=nSelfTag ){
        SYNTAX("non-self-referential T-card in technote");
      }else if( d->T.used!=nSimpleTag ){
        SYNTAX("T-card with '*' or '-' in technote");
      }
      break;
    }
    case FSL_SATYPE_FORUMPOST: {
      if( d->H && d->I ){
        SYNTAX("cannot have I-card and H-card in a forum post");
      }else if( d->P.used>1 ){
        SYNTAX("too many arguments to P-card");
      }
      break;
    }
    default: break;
  }

  assert(!d->content.mem);
  if(stealBuf>0){
    /* We stashed something which points to src->mem, so we need to
       steal that memory.
    */
    d->content = *src;
    *src = fsl_buffer_empty;
  }
  d->F.flags &= ~FSL_CARD_F_LIST_NEEDS_SORT/*we know all cards were read in order*/;
  return 0;

  bailout:
  if(stealBuf>0){
    d->content = *src;
    *src = fsl_buffer_empty;
  }
  assert(0 != rc);
  if(zMsg){
    fsl_error_set(err, rc, "%s", zMsg);
  }
  return rc;
#undef SEEN
#undef TOKEN_DATETIME
#undef SYNTAX
#undef TOKEN_CHECKHEX
#undef TOKEN_EXISTS
#undef TOKEN_UUID
#undef TOKEN_MD5
#undef TOKEN
#undef ERROR
}

int fsl_deck_parse(fsl_deck * d, fsl_buffer * src){
  return fsl_deck_parse2(d, src, 0);
}

int fsl_deck_load_rid( fsl_cx * f, fsl_deck * d,
                       fsl_id_t rid, fsl_satype_e type ){
  fsl_buffer buf = fsl_buffer_empty;
  int rc = 0;
  if(!f || !d) return FSL_RC_SYNTAX;
  if(0==rid) rid = f->ckout.rid;
  if(rid<0){
    return fsl_cx_err_set(f, FSL_RC_RANGE,
                          "Invalid RID for fsl_deck_load_rid(): "
                          "%"FSL_ID_T_PFMT, rid);
  }
  fsl_deck_clean(d);
  if(fsl_cx_mcache_search(f, rid, d)){
    assert(d->f);
    if(type!=FSL_SATYPE_ANY && type!=d->type){
      rc = fsl_cx_err_set(f, FSL_RC_ERROR,
                          "Unexpected match of RID #%" FSL_ID_T_PFMT " "
                          "to a different artifact type (%d) "
                          "than requested (%d).",
                          d->type, type);
      fsl_cx_mcache_insert(f, d);
      assert(!d->f);
    }else{
      //MARKER(("Got cached deck: %s\n", d->uuid));
    }
    return rc;
  }  
  rc = fsl_content_get(f, rid, &buf);
  if(rc) goto end;
#if 0
  MARKER(("fsl_content_get(%d) len=%d =\n%.*s\n",
          (int)rid, (int)buf.used, (int)buf.used, (char const*)buf.mem));
#endif
  fsl_deck_init(f, d, FSL_SATYPE_ANY);
#if 0
  /*
    If we set d->type=type, the parser can fail more
    quickly. However, that failure will bypass our more specific
    reporting of the problem (see below).  As the type mismatch case
    is expected to be fairly rare, we'll leave this out for now, but
    it might be worth considering as a small optimization later on.
  */
  d->type = type /* may help parsing fail more quickly if
                    it's not the type we want.*/;
#endif
  rc = fsl_deck_parse(d, &buf);
  if(!rc){
    assert(rid == d->rid);
    if( type!=FSL_SATYPE_ANY && d->type!=type ){
      rc = fsl_cx_err_set(f, FSL_RC_TYPE,
                          "RID %"FSL_ID_T_PFMT" is of type %s, "
                          "but the caller requested type %s.",
                          rid,
                          fsl_satype_cstr(d->type),
                          fsl_satype_cstr(type));
    }
  }
  if( !rc && d->B.uuid ){
    rc = fsl_cx_update_seen_delta_mf(f);
  }
  end:
  fsl_buffer_clear(&buf);
  return rc;
}

int fsl_deck_load_sym( fsl_cx * f, fsl_deck * d,
                       char const * symbolicName, fsl_satype_e type ){
  if(!symbolicName || !d) return FSL_RC_MISUSE;
  else{
    fsl_id_t vid = 0;
    int rc = fsl_sym_to_rid(f, symbolicName, type, &vid);
    if(!rc){
      assert(vid>0);
      rc = fsl_deck_load_rid(f, d, vid, type);
    }
    return rc;
  }
}

  
static int fsl_deck_baseline_load( fsl_deck * d ){
  int rc = 0;
  fsl_deck bl = fsl_deck_empty;
  fsl_id_t rid;
  fsl_cx * f = d ? d->f : NULL;
  fsl_db * db = f ? fsl_needs_repo(f) : NULL;
  assert(d->f);
  assert(d);
  if(!d->f) return FSL_RC_MISUSE;
  else if(d->B.baseline || !d->B.uuid) return 0 /* nothing to do! */;
  else if(!db) return FSL_RC_NOT_A_REPO;
#if 0
  else if(d->rid<=0){
    return fsl_cx_err_set(f, FSL_RC_RANGE,
                          "fsl_deck_baseline_load(): "
                          "fsl_deck::rid is not set.");
  }
#endif
  rid = fsl_uuid_to_rid(f, d->B.uuid);
  if(rid<0){
    assert(f->error.code);
    return f->error.code;
  }
  else if(!rid){
    if(d->rid>0){
      fsl_db_exec(db, 
                  "INSERT OR IGNORE INTO orphan(rid, baseline) "
                  "VALUES(%"FSL_ID_T_PFMT",%"FSL_ID_T_PFMT")",
                  d->rid, rid);
    }
    rc = fsl_cx_err_set(f, FSL_RC_RANGE,
                        "Could not find/load baseline manifest [%s], "
                        "parent of manifest rid #%"FSL_ID_T_PFMT".",
                        d->B.uuid, d->rid);
  }else{
    rc = fsl_deck_load_rid(f, &bl, rid, FSL_SATYPE_CHECKIN);
    if(!rc){
      d->B.baseline = fsl_deck_malloc();
      if(!d->B.baseline){
        fsl_deck_clean(&bl);
        rc = FSL_RC_OOM;
      }else{
        void const * allocStampKludge = d->B.baseline->allocStamp;
        *d->B.baseline = bl /* Transfer ownership */;
        d->B.baseline->allocStamp = allocStampKludge /* But we need this intact
                                                        for deallocation to work */;
        assert(f==d->B.baseline->f);
      }
    }else{
      /* bl might be partially populated */
      fsl_deck_finalize(&bl);
    }
  }
  return rc;
}

int fsl_deck_baseline_fetch( fsl_deck * d ){
  return (d->B.baseline || !d->B.uuid)
    ? 0
    : fsl_deck_baseline_load(d);
}

int fsl_deck_F_rewind( fsl_deck * d ){
  int rc = 0;
  d->F.cursor = 0;
  assert(d->f);
  if(d->B.uuid){
    rc = fsl_deck_baseline_fetch(d);
    if(!rc){
      assert(d->B.baseline);
      d->B.baseline->F.cursor = 0;
    }
  }
  return rc;
}

int fsl_deck_F_next( fsl_deck * d, fsl_card_F const ** rv ){
  assert(d);
  assert(d->f);
  assert(rv);
#define FCARD(DECK,NDX) F_at(&(DECK)->F, NDX)
  *rv = NULL;
  if(!d->B.baseline){
    /* Manifest d is a baseline-manifest.  Just scan down the list
       of files. */
    if(d->B.uuid){
      return fsl_cx_err_set(d->f, FSL_RC_MISUSE,
                            "Deck has a B-card (%s) but no baseline "
                            "loaded. Load the baseline before calling "
                            "%s().",
                            d->B.uuid, __func__)
        /* We "could" just load the baseline from here. */;
    }
    if( d->F.cursor < (int32_t)d->F.used ){
      *rv = FCARD(d, d->F.cursor++);
      assert(*rv);
      assert((*rv)->uuid && "Baseline manifest has deleted F-card entry!");
    }
    return 0;
  }else{
    /* Manifest d is a delta-manifest.  Scan the baseline but amend the
       file list in the baseline with changes described by d.
    */
    fsl_deck * const pB = d->B.baseline;
    int cmp;
    while(1){
      if( pB->F.cursor >= (fsl_int_t)pB->F.used ){
        /* We have used all entries out of the baseline.  Return the next
           entry from the delta. */
        if( d->F.cursor < (fsl_int_t)d->F.used ) *rv = FCARD(d, d->F.cursor++);
        break;
      }else if( d->F.cursor >= (fsl_int_t)d->F.used ){
        /* We have used all entries from the delta.  Return the next
           entry from the baseline. */
        if( pB->F.cursor < (fsl_int_t)pB->F.used ) *rv = FCARD(pB, pB->F.cursor++);
        break;
      }else if( (cmp = fsl_strcmp(FCARD(pB,pB->F.cursor)->name,
                                  FCARD(d, d->F.cursor)->name)) < 0){
        /* The next baseline entry comes before the next delta entry.
           So return the baseline entry. */
        *rv = FCARD(pB, pB->F.cursor++);
        break;
      }else if( cmp>0 ){
        /* The next delta entry comes before the next baseline
           entry so return the delta entry */
        *rv = FCARD(d, d->F.cursor++);
        break;
      }else if( FCARD(d, d->F.cursor)->uuid ){
        /* The next delta entry is a replacement for the next baseline
           entry.  Skip the baseline entry and return the delta entry */
        pB->F.cursor++;
        *rv = FCARD(d, d->F.cursor++);
        break;
      }else{
        assert(0==cmp);
        /*
          The next delta entry is a delete of the next baseline entry.
        */
        /* Skip them both.  Repeat the loop to find the next
           non-delete entry. */
        pB->F.cursor++;
        d->F.cursor++;
        continue;
      }      
    }
    return 0;
  }
#undef FCARD
}

int fsl_deck_save( fsl_deck * d, bool isPrivate ){
  int rc;
  fsl_cx * f = d ? d->f : NULL;
  fsl_db * db = f ? fsl_needs_repo(f) : NULL;
  fsl_buffer buf = fsl_buffer_empty;
  fsl_id_t newRid = 0;
  bool const oldPrivate = f ? f->cache.markPrivate : 0;
  bool const isNew = d && !d->uuid;
  if(!f || !d ) return FSL_RC_MISUSE;
  else if(!db) return FSL_RC_NOT_A_REPO;
  else if( (d->rid>0 && !d->uuid) ||
           (d->rid<=0 && d->uuid)){
    return fsl_cx_err_set(f, FSL_RC_MISUSE,
                          "The input deck must have either rid _and_ UUID "
                          "or neither of rid/UUID. A mixture is ambiguous.");
  }else if(d->B.uuid && fsl_repo_forbids_delta_manifests(f)){
    return fsl_cx_err_set(f, FSL_RC_ACCESS,
                          "This deck is a delta manifest, but this "
                          "repository has disallowed those via the "
                          "forbid-delta-manifests config option.");
  }

  fsl_cx_err_reset(f);
  rc = fsl_deck_output(d, fsl_output_f_buffer, &buf);
  if(rc){
    fsl_buffer_clear(&buf);
    return rc;
  }

  rc = fsl_db_transaction_begin(db);
  if(rc){
    fsl_buffer_clear(&buf);
    return rc;
  }
  if(0){
    MARKER(("Saving deck:\n%s\n", fsl_buffer_cstr(&buf)));
  }

  /* Starting here, don't return, use (goto end) instead. */

  f->cache.markPrivate = isPrivate;
  rc = fsl_content_put_ex(f, &buf, d->uuid, 0,
                          0U, isPrivate, &newRid);
  if(rc) goto end;
  assert(newRid>0);

  rc = fsl_cx_hash_buffer( f, false, &buf, &buf );
  if(rc) goto end;

  /* We need d->uuid and d->rid for crosslinking purposes, but will
     unset them on error (if we set them) because their values will no
     longer be in the db after rollback...
  */
#if 0
  /* practice shows that this is not needed/desired. There are cases
     where round-tripping a manifest results in a 1-millisecond
     difference in the D-card (Julian Day/double), which changes the
     has but otherwise has no difference on the meaning. That said,
     the way we use decks means that we really don't care (for
     purposes of this function) what their initial UUID is except for
     the purpose of fsl_content_put_ex(), above.
  */
  if(d->uuid){
    /* Compare original and resulting hashes. */
    if(0 != fsl_uuidcmp(fsl_buffer_cstr(&buf), d->uuid)){
      rc = fsl_cx_err_set(f, FSL_RC_CONSISTENCY,
                          "Input deck's original UUID and resulting UUID "
                          "do not match: [%s] vs [%b]",
                          d->uuid, &buf);
      goto end;
    }
    /* ??? assert(d->rid==newRid); */
    d->rid = newRid;
  }else
#endif  
  {
    fsl_free(d->uuid);
    d->uuid = fsl_buffer_str(&buf) /* transfer ownership */;
    buf = fsl_buffer_empty;
    d->rid = newRid;
  }

#if 0
  /* Something to consider: if d is new and has a parent, deltify the
     parent. The branch operation does this, but it is not yet clear
     whether that is a general pattern for manifests.
  */
  if(isNew && d->P.used){
    fsl_id_t pid;
    assert(FSL_SATYPE_CHECKIN == d->type);
    pid = fsl_uuid_to_rid(f, (char const *)d->P.list[0]);
    if(pid>0){
      rc = fsl_content_deltify(f, pid, d->rid, 0);
      if(rc) goto end;
    }
  }
#endif

  if(isNew && (FSL_SATYPE_WIKI==d->type)){
    /* Analog to fossil's wiki.c:wiki_put(): */
    /*
      MISSING:
      fossil's wiki.c:wiki_put() handles the moderation bits.
    */
    if(d->P.used){
      fsl_id_t const pid = fsl_deck_P_get_id(d, 0);
      assert(pid>0);
      if(pid<0){
        assert(f->error.code);
        rc = f->error.code;
        goto end;
      }else if(!pid){
        if(!f->error.code){
          rc = fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
                              "Did not find matching RID "
                              "for P-card[0] (%s).",
                              (char const *)d->P.list[0]);
        }
        goto end;
      }

      rc = fsl_content_deltify(f, pid, d->rid, 0);
      if(rc) goto end;
    }
    rc = fsl_db_exec_multi(db,
                           "INSERT OR IGNORE INTO unsent "
                           "VALUES(%"FSL_ID_T_PFMT");"
                           "INSERT OR IGNORE INTO unclustered "
                           "VALUES(%"FSL_ID_T_PFMT");",
                           d->rid, d->rid);
    if(rc){
      fsl_cx_uplift_db_error(f, db);
      goto end;
    }
  }

  rc = f->cache.isCrosslinking
    ? fsl_deck_crosslink(d)
    : fsl_deck_crosslink_one(d);

  end:
  f->cache.markPrivate = oldPrivate;
  if(!rc) rc = fsl_db_transaction_end( db, 0);
  else fsl_db_transaction_end(db, 1);
  if(rc){
    if(isNew){
      fsl_free(d->uuid);
      d->uuid = NULL;
      d->rid = 0;
    }
    if(!f->error.code && db->error.code){
      rc = fsl_cx_uplift_db_error(f, db);
    }
  }
  fsl_buffer_clear(&buf);
  return rc;
}

int fsl_crosslink_end(fsl_cx * f){
  int rc = 0;
  fsl_db * db = fsl_cx_db_repo(f);
  fsl_stmt q = fsl_stmt_empty;
  fsl_stmt u = fsl_stmt_empty;
  int i;
  assert(f);
  assert(db);
  assert(f->cache.isCrosslinking);
  if(!f->cache.isCrosslinking){
    return fsl_cx_err_set(f, FSL_RC_MISUSE,
                          "Crosslink is not running.");
  }
  f->cache.isCrosslinking = false;
  assert(db->beginCount > 0);

  /* Handle any reparenting via tags... */
  rc = fsl_db_prepare(db, &q,
                     "SELECT rid, value FROM tagxref"
                      " WHERE tagid=%d AND tagtype=%d",
                      (int)FSL_TAGID_PARENT, (int)FSL_TAGTYPE_ADD);
  if(rc) goto end;
  while(FSL_RC_STEP_ROW==fsl_stmt_step(&q)){
    fsl_id_t const rid = fsl_stmt_g_id(&q, 0);
    const char *zTagVal = fsl_stmt_g_text(&q, 1, 0);
    rc = fsl_crosslink_reparent(f,rid, zTagVal);
    if(rc) break;
  }
  fsl_stmt_finalize(&q);
  if(rc) goto end;

  /* Process entries from pending_xlink temp table... */
  rc = fsl_db_prepare(db, &q, "SELECT id FROM pending_xlink");
  if(rc) goto end;
  while( FSL_RC_STEP_ROW==fsl_stmt_step(&q) ){
    const char *zId = fsl_stmt_g_text(&q, 0, NULL);
    char cType;
    if(!zId || !*zId) continue;
    cType = zId[0];
    ++zId;
    if('t'==cType){
      /* FSL-MISSING:
         ticket_rebuild_entry(zId) */
      continue;
    }else if('w'==cType){
      /* FSL-MISSING:
         backlink_wiki_refresh(zId) */
      continue;
    }
  }
  fsl_stmt_finalize(&q);
  rc = fsl_db_exec(db, "DROP TABLE pending_xlink");
  if(rc) goto end;
  /* If multiple check-ins happen close together in time, adjust their
     times by a few milliseconds to make sure they appear in chronological
     order.
  */
  rc = fsl_db_prepare(db, &q,
                      "UPDATE time_fudge SET m1=m2-:incr "
                      "WHERE m1>=m2 AND m1<m2+:window"
  );
  if(rc) goto end;
  fsl_stmt_bind_double_name(&q, ":incr", AGE_ADJUST_INCREMENT);
  fsl_stmt_bind_double_name(&q, ":window", AGE_FUDGE_WINDOW);
  rc = fsl_db_prepare(db, &u,
                      "UPDATE time_fudge SET m2="
                      "(SELECT x.m1 FROM time_fudge AS x"
                      " WHERE x.mid=time_fudge.cid)");
  for(i=0; !rc && i<30; i++){ /* where does 30 come from? */
    rc = fsl_stmt_step(&q);
    if(FSL_RC_STEP_DONE==rc) rc=0;
    else break;
    fsl_stmt_reset(&q);
    if( fsl_db_changes_recent(db)==0 ) break;
    rc = fsl_stmt_step(&u);
    if(FSL_RC_STEP_DONE==rc) rc=0;
    else break;
    fsl_stmt_reset(&u);
  }
  fsl_stmt_finalize(&q);
  fsl_stmt_finalize(&u);
  if(!rc && fsl_db_exists(db,"SELECT 1 FROM time_fudge")){
    rc = fsl_db_exec(db, "UPDATE event SET"
                     " mtime=(SELECT m1 FROM time_fudge WHERE mid=objid)"
                     " WHERE objid IN (SELECT mid FROM time_fudge)"
                     " AND (mtime=omtime OR omtime IS NULL)"
                     );
  }
  end:
  rc = fsl_cx_uplift_db_error2(f, db, rc)
    /* Do before drop time_fudge to ensure we don't
       clear the error state by accident. */;
  if(!rc){
    fsl_db_exec(db, "DROP TABLE time_fudge");
  }
  if(rc) fsl_db_transaction_rollback(db);
  else rc = fsl_db_transaction_commit(db);
  return fsl_cx_uplift_db_error2(f, db, rc);
}

int fsl_crosslink_begin(fsl_cx * f){
  int rc;
  fsl_db * db = fsl_cx_db_repo(f);
  assert(f);
  assert(db);
  assert(0==f->cache.isCrosslinking);
  if(f->cache.isCrosslinking){
    return fsl_cx_err_set(f, FSL_RC_MISUSE,
                          "Crosslink is already running.");
  }
  rc = fsl_db_transaction_begin(db);
  if(rc) return fsl_cx_uplift_db_error(f, db);
  rc = fsl_db_exec_multi(db,
     "CREATE TEMP TABLE pending_xlink(id TEXT PRIMARY KEY)WITHOUT ROWID;"
     "CREATE TEMP TABLE time_fudge("
     "  mid INTEGER PRIMARY KEY,"    /* The rid of a manifest */
     "  m1 REAL,"                    /* The timestamp on mid */
     "  cid INTEGER,"                /* A child or mid */
     "  m2 REAL"                     /* Timestamp on the child */
     ");");
  if(!rc){
    f->cache.isCrosslinking = 1;
    return 0;
  }else{
    rc = fsl_cx_uplift_db_error2(f, db, rc);
    fsl_db_transaction_rollback(db);
    return rc;
  }
}

#undef MARKER
#undef AGE_FUDGE_WINDOW
#undef AGE_ADJUST_INCREMENT
#undef F_at
