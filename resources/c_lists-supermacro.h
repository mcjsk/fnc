/*
  This file is a "supermacro", intended to be included multiple times
  by related client code.

  Inputs:

  #define LIST_T list_type
  #define VALUE_T value_type
  #define VALUE_T_IS_PTR 1 // IF value_type is ptr-qualified
  #define LIST_DECLARE   // decls will be emitted.
  #define LIST_IMPLEMENT // impls will be emitted.

  VALUE_T_IS_PTR removes some routines, but is needed for handling
  (T**) lists propertly.

  list_type must be a struct with at least the following members:

  struct list_type {
  VALUE_T list; // or (VALUE_T*) if VALUE_T_IS_PTR
  cwal_size_t used; // number of used entries in the list
  cwal_size_t capacity; // number of items allocated in the list
  };

  Additional members are optional, and unused by this code.
  
  This file provides routines for (de)allocating, growing,
  visiting, and cleaning up list contents.

  The generated functions are named LIST_T_funcname(),
  e.g. list_type_reserve().
*/

#if !defined(VALUE_T_IS_PTR)
#  define VALUE_T_IS_PTR 0
#endif
#define SYM2(X,Y) X ## Y
#define SYM(X,Y) SYM2(X,Y)
#define STR2(T) # T
#define STR(T) STR2(T)

#if 0 && defined(LIST_IMPLEMENT)
#include <string.h> /* memset() */
#include <assert.h>
#endif

#ifdef LIST_DECLARE
/**
   Possibly reallocates self->list, changing its size. This function
   ensures that self->list has at least n entries. If n is 0 then
   the list is deallocated (but the self object is not), BUT THIS DOES NOT
   DO ANY TYPE-SPECIFIC CLEANUP of the items. If n is less than or equal
   to self->capacity then there are no side effects. If n is greater
   than self->capacity, self->list is reallocated and self->capacity
   is adjusted to be at least n (it might be bigger - this function may
   pre-allocate a larger value).

   Passing an n of 0 when self->capacity is 0 is a no-op.

   Newly-allocated items will be initialized with NUL bytes.
   
   Returns the total number of items allocated for self->list.  On
   success, the value will be equal to or greater than n (in the
   special case of n==0, 0 is returned). Thus a return value smaller
   than n is an error. Note that if n is 0 or self is NULL then 0 is
   returned.

   The return value should be used like this:

   @code
   cwal_size_t const n = number of bytes to allocate;
   if( n > my_list_t_reserve( myList, n ) ) { ... error ... }
   // Or the other way around:
   if( my_list_t_reserve( myList, n ) < n ) { ... error ... }
   @endcode
*/
cwal_size_t SYM(LIST_T,_reserve)( cwal_engine * e, LIST_T * self, cwal_size_t n );

/**
   Appends a bitwise copy of cp to self->list, expanding the list as
   necessary and adjusting self->used.

   Ownership of cp is unchanged by this call. cp may not be NULL.

   Returns 0 on success, CWAL_RC_MISUSE if any argument is NULL,
   or CWAL_RC_OOM on allocation error.
*/
int SYM(LIST_T,_append)( cwal_engine * e, LIST_T * self, VALUE_T cp );

/** @typedef typedef int (*LIST_TYPE_visitor_f)(void * p, void * visitorState )
   
   Generic visitor interface for cwal_list lists.  Used by
   LIST_TYPE_visit(). p is the pointer held by that list entry and
   visitorState is the 4th argument passed to cwal_list_visit().

   Implementations must return 0 on success. Any other value causes
   looping to stop and that value to be returned, but interpration of
   the value is up to the caller (it might or might not be an error,
   depending on the context). Note that client code may use custom
   values, and is not restricted to cwal_rc values.
*/

#if VALUE_T_IS_PTR
typedef int (*SYM(LIST_T,_visitor_f))(VALUE_T obj, void * visitorState );
#else
typedef int (*SYM(LIST_T,_visitor_f))(VALUE_T * obj, void * visitorState );
#endif

/**
   For each item in self->list, visitor(item,visitorState) is called.
   The item is owned by self. The visitor function MUST NOT free the
   item, but may manipulate its contents if application rules do not
   specify otherwise.

   If order is 0 or greater then the list is traversed from start to
   finish, else it is traverse from end to begin.

   Returns 0 on success, non-0 on error.

   If visitor() returns non-0 then looping stops and that code is
   returned.
*/
int SYM(LIST_T,_visit)( LIST_T * self, char order,
                        SYM(LIST_T,_visitor_f) visitor, void * visitorState );
#if VALUE_T_IS_PTR
/**
   Works similarly to the visit operation without the _p suffix except
   that the pointer the visitor function gets is a (**) pointing back
   to the entry within this list. That means that callers can assign
   the entry in the list to another value during the traversal process
   (e.g. set it to 0). If shiftIfNulled is true then if the callback
   sets the list's value to 0 then it is removed from the list and
   self->used is adjusted (self->capacity is not changed).
*/
int SYM(LIST_T,_visit_p)( LIST_T * self, char order, char shiftIfNulled,
                          SYM(LIST_T,_visitor_f) visitor, void * visitorState );
#endif

#endif
/* LIST_DECLARE */


#ifdef LIST_IMPLEMENT

cwal_size_t SYM(LIST_T,_reserve)( cwal_engine * e, LIST_T * self, cwal_size_t n )
{
    if( !self ) return 0;
    else if(0 == n)
    {
        if(0 == self->capacity) return 0;
        cwal_free(e, self->list);
        self->list = NULL;
        self->capacity = self->used = 0;
        return 0;
    }
    else if( self->capacity >= n )
    {
        return self->capacity;
    }
    else
    {
        size_t const sz = sizeof(VALUE_T) * n;
        VALUE_T * m = (VALUE_T*)cwal_realloc( e, self->list, sz );
        if( ! m ) return self->capacity;
        /* Zero out the new elements... */
        memset( m + self->capacity, 0, (sizeof(VALUE_T)*(n-self->capacity)));
        self->capacity = n;
        self->list = m;
        return n;
    }
}

int SYM(LIST_T,_append)( cwal_engine * e, LIST_T * self, VALUE_T cp )
{
    if( !e || !self || !cp ) return CWAL_RC_MISUSE;
    else if( self->capacity > SYM(LIST_T,_reserve)(e, self, self->used+1) )
    {
        return CWAL_RC_OOM;
    }
    else
    {
        self->list[self->used++] = cp;
#if VALUE_T_IS_PTR
        if(self->used< self->capacity-1) self->list[self->used]=0;
#endif
        return 0;
    }
}

int SYM(LIST_T,_visit)( LIST_T * self, char order,
                        SYM(LIST_T,_visitor_f) visitor, void * visitorState )
{
    int rc = CWAL_RC_OK;
    if( self && self->used && visitor )
    {
        int i = 0;
        int pos = (order<0) ? self->used-1 : 0;
        int step = (order<0) ? -1 : 1;
        for( rc = 0; (i < self->used) && (0 == rc); ++i, pos+=step )
        {
#if VALUE_T_IS_PTR
            VALUE_T obj = self->list[pos];
#else
            VALUE_T * obj = &self->list[pos];
#endif
            if(obj) rc = visitor( obj, visitorState );
#if VALUE_T_IS_PTR
            /* cwal-specific hack b/c obj can be removed during traversal... */
            if( obj != self->list[pos] ){
                --i;
                if(order>=0) pos -= step;
            }
#endif

        }
    }
    return rc;
}

#if VALUE_T_IS_PTR
int SYM(LIST_T,_visit_p)( LIST_T * self, char order,
                          char shiftIfNulled,
                          SYM(LIST_T,_visitor_f) visitor, void * visitorState )
{
    int rc = CWAL_RC_OK;
    if( self && self->used && visitor )
    {
        int i = 0;
        int pos = (order<0) ? self->used-1 : 0;
        int step = (order<0) ? -1 : 1;
        for( rc = 0; (i < (int)self->used) && (0 == rc); ++i, pos+=step )
        {
            VALUE_T obj = self->list[pos];
            if(obj) {
                assert((order<0) && "TEST THAT THIS WORKS WITH IN-ORDER!");
                rc = visitor( &self->list[pos], visitorState );
                if( shiftIfNulled && !self->list[pos]){
                    int x = pos;
                    int const to = self->used-pos;
                    /*MARKER("i=%d pos=%d x=%d to=%d\n",i, pos, x, to);*/
                    assert( to < (int) self->capacity );
                    for( ; x < to; ++x ) self->list[x] = self->list[x+1];
                    if( x < (int)self->capacity ) self->list[x] = 0;
                    --i;
                    --self->used;
                    if(order>=0) pos -= step;
                }
            }
        }
    }
    return rc;
}
#endif


#if !VALUE_T_IS_PTR
/**
   Reduces self->used by 1. Returns 0 on success.
   Errors include:

   (!self) or (0 == self->used)
*/
int SYM(LIST_T,_pop_back)( cwal_engine * e, LIST_T * self, void (*cleaner)(VALUE_T * obj) )
{
    if( !self ) return CWAL_RC_MISUSE;
    else if( ! self->used ) return cson_rc.RangeError;
    else
    {
        cwal_size_t const ndx = --self->used;
        VALUE_T * val = &self->list[ndx];
        if(val && cleaner) cleaner(val);
        return 0;
    }
}
#endif

#endif
/* LIST_IMPLEMENT */

#undef LIST_IMPLEMENT
#undef LIST_DECLARE
#undef SYM
#undef SYM2
#undef LIST_T
#undef VALUE_T
#undef VALUE_T_IS_PTR
#undef STR
#undef STR2
