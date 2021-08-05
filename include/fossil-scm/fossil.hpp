/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_LIBFOSSIL_HPP_INCLUDED)
#define ORG_FOSSIL_SCM_LIBFOSSIL_HPP_INCLUDED
/**
  Copyright 2013-2021 Stephan Beal (https://wanderinghorse.net).

  Derived heavily from previous work:

  Copyright (c) 2013 D. Richard Hipp (https://www.hwaci.com/drh/)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the Simplified BSD License (also
  known as the "2-Clause License" or "FreeBSD License".)

  This program is distributed in the hope that it will be useful,
  but without any warranty; without even the implied warranty of
  merchantability or fitness for a particular purpose.
*/

/*
   fossil.h MUST be included first so we can set some portability
   flags and config-dependent typedefs!
*/
#include "fossil-scm/fossil.h"
#include <stdexcept>
#include <string>
#include <sstream>
#include <stdarg.h>
#if 0
#include <cstdint> /* FIXME: use this if available (C++11) */
#endif

/**
   fsl is the primary namespace of the libfossil C++ API. The C++ API
   wraps much of the C-level function and simplifies its usage
   somewhat by using exceptions extensively for error reporting.

   A brief note about exceptions: functions and methods with do not
   throw are marked as throw(). Any others may very well throw. Though
   in practice the library APIs "simply do not fail" if used properly,
   there are gazillions of _potential_ error cases which the
   underlying C library _may_ propagate up to the client (in this case
   the C++ API), and this C++ wrapper treats almost every C-level
   error as an Exception, with only a few exceptions to simplify usage
   (e.g. cleanup-related functions never throw because they are
   generally used in destructors, and dtors are conventionally
   prohibited from throwing). The base exception type used by the
   library is fsl::Exception, which derives from std::exception per
   long-standing convention.

   While the API is not STL-centric, it does provide some basic
   STL-relatated support, e.g. the fsl::FslOutputFStream and
   fsl::BufferOStream classes.

*/
namespace fsl {

  /**
     The base exception type used by the API. It follows the libfossil
     convention of returning errors as (if possible) an error code
     from the fsl_rc_e enum and a descriptive string (which generally
     defaults to the string-form of the FSL_RC_xxx constant value (see
     fsl_rc_cstr()).
  */
  class Exception : public std::exception {
    public:
    /**
       Cleans up any error string memory.
    */
    virtual ~Exception() throw();

    /** Copies the error message state from other. */
    Exception(Exception const &other) throw();

    /** Copies the error message state from other. */
    Exception & operator=(Exception const &other) throw();

    /**
       Sets the error state from the given arguments. If msg.empty()
       then fsl_rc_cstr(code) is used as a message string.
    */
    Exception(int code, std::string const & msg) throw();

    /**
       Equivalent to Exception(code, fsl_rc_cstr(code)).
    */
    explicit Exception(int code) throw();

    /**
       A default-constructed exception with no message string
       and an error code of FSL_RC_ERROR. Intended to be used
       in conjunction with the (fsl_error*) implicit conversion
       and C APIs taking a fsl_error pointer.
     */
    Exception() throw();

    /**
       Moves err's contents into this object, effectively upgrading it
       to something we can throw. It is safe to pass a local
       stack-allocated fsl_error object provided it is properly
       initialized (via copy-construction from fsl_error_empty
       resp. fsl_error_empty_m) because this function takes its memory
       from it.
    */
    explicit Exception( fsl_error & err ) throw();

    /**
       Copies err's contents into this object, effectively upgrading
       it to something we can throw. Is generally intended to be
       passed the return value from, e.g. fsl_cx_err_get_e().
    */
    explicit Exception( fsl_error const * err ) throw();

    /**
       Sets the error state from the given code (generally assumed to
       be a fsl_rc_e value!) and a formatted string, supporting the
       same formatting options as fsl_appendf() and friends.

       As a special case, if code==FSL_RC_OOM then the message
       string is ignored to avoid allocating memory for the
       error message.

       When passing on strings from external sources, it is safest to
       pass "%s" as the format string and the foreign string as the
       first variadic argument.  That ensures that percent signs in
       the foreign input do not get processed as format specifiers
       (which expect further arguments in the variadic list, which
       will likely lead to undefined behaviour).
    */
    Exception(int code, char const * fmt, ...) throw();

    /**
       Equivalent to Exception(code,fmt,...) except that it
       takes a va_list.
    */
    Exception(int code, char const * fmt, va_list) throw();

    /**
       Returns the message string provided to the ctor.
       It's not called message() because this interface
       is inherited from std::exception.
    */
    virtual char const * what() const throw();

    /**
       Alias (for API consistency's sake) for what().
    */
    char const * messsage() const throw();

    /**
       Returns the code passed to the constructor.
    */
    int code() const throw();

    /**
       Equivalent to fsl_rc_cstr(this->code()).
    */
    char const * codeString() const throw();

    /**
       Implicit conversion to simplify some integration with
       the C APIs.
    */
    operator fsl_error * () throw();

    /**
       Const-correct overload.
    */
    operator fsl_error const * () const throw();

    private:
    fsl_error err;
    /** Internal code consolidator. */
    void error(int code, char const * fmt, va_list vargs) throw();
  };

  /**
     Out-of-memory exception.
  */
  class OOMException : public Exception{
    public:
    /**
       Sets the code FSL_RC_OOM and uses no error string (to avoid
       allocating more memory).
    */
    OOMException() throw();
  };

  /**
     A very thin varnish over ::fsl_buffer, the primary advantage
     being that it frees its memory when it destructs, so it's easier
     to keep exception-safe than a raw fsl_buffer.

     It implements an implicit conversion to (fsl_buffer*), making it
     trivial to use with the C-level fsl_buffer APIs.

     Note that the underlying buffer APIs guaranty that they
     NUL-terminates the buffer when data is appended to it, so it can
     easily be used to create dynamic strings (which is in fact one of
     its primary uses). As for strlen() and conventional string
     classes (e.g. std::string), the automatically-added NUL byte is
     never counted as part of the "effective length" of the buffer
     (Buffer::used() resp.  fsl_buffer::used).

     Example usage:

     @code
     Buffer b;
     b.appendf(b, "hi, %s!", "world"); // throws on buffer (re)allocation error
     std::cout << b << '\n';
     @endcode

  */
  class Buffer {
  private:
    fsl_buffer buf;
  public:
    /**
       Initializes the buffer. If startingSize is greater than
       zero it reserves that amount of memory, throwing an
       OOMException if that fails.
    */
    explicit Buffer(fsl_size_t startingSize);
    /**
       Initializes an empty buffer.
    */
    Buffer();
    /**
       Frees all memory owned by the buffer.
    */
    ~Buffer() throw();

    /**
       Replaces the current buffer contents with a copy of those from
       the given buffer. It re-uses the existing buffer memory if
       enough is available, and will shrink itself to fit if re-using
       a buffer would waste "too much" memory.
    */
    Buffer & operator=(Buffer const & other);

    /**
       Copies the contents of the other buffer.
    */
    Buffer(Buffer const & other);

    /**
       Implicit conversion to (fsl_buffer *) to simplify usage with
       the C API.

       NEVER EVER use/rely upon this conversion in the following
       context:

       - As a fsl_appendf() (or similar) argument when using the %b/%B
       (fsl_buffer-specific) format specifiers. Use handle()
       instead or the wrong conversion will be used because
       compile-time doesn't know this conversion should be used at
       that point (leading to undefined results).
    */
    operator fsl_buffer * () throw();

    /**
       Const-correct overload.
    */
    operator fsl_buffer const * () const throw();

    /**
       Returns the underlying fsl_buffer this object proxies.
       See operator fsl_buffer *().
    */
    fsl_buffer * handle() throw();

    /**
       Const-correct overload.
    */
    fsl_buffer const * handle() const throw();

    /**
       Returns true if 0==used().
    */
    bool empty() const throw();

    /**
       Returns the "used" number of bytes in the buffer.
    */
    fsl_size_t used() const throw();

    /**
       Returns the current capacity of the buffer.
    */
    fsl_size_t capacity() const throw();

    /**
       Returns a pointer to the current buffer, which points to
       used() bytes of memory.  Returns NULL for an empty()
       buffer. The returned pointer may be invalidated on any changes
       to the buffer.
    */
    unsigned char * mem() throw();

    /**
       Const-correct overload.
    */
    unsigned char const * mem() const throw();

    /**
       Equivalent to fsl_buffer_clear(*this).
    */
    Buffer & clear() throw();

    /**
       Equivalent to fsl_buffer_reset(*this).
    */
    Buffer & reset() throw();

    /**
       Equivalent to fsl_buffer_reserve(*this, n), but throws on error
       and returns this object on success.
    */
    Buffer & reserve(fsl_size_t n);

    /**
       Equivalent to fsl_buffer_resize(*this, n), but throws on error
       and returns this object on success.
    */
    Buffer & resize(fsl_size_t n);

    /**
       STL-style iterator for the buffer's memory.
    */
    typedef unsigned char * iterator;

    /**
       STL-style const iterator for the buffer's memory.
    */
    typedef unsigned char const * const_iterator;

    /**
       Returns the starting memory position iterator,
       or NULL if empty(). It may be invalidated by
       any changes to the buffer.
    */
    iterator begin() throw();

    /**
       Returns the one-after-the-end of the memory position iterator
       (the "used" space, not necessarily the capacity), or NULL if
       empty(). It may be invalidated by any changes to the buffer.
    */
    iterator end() throw();

    /**
       Const-correct overload.
    */
    const_iterator begin() const throw();

    /**
       Const-correct overload.
    */
    const_iterator end() const throw();

    /**
       Basically equivalent to fsl_buffer_appendf(*this,...)
       except that it throws if that function fails.
    */
    Buffer & appendf(char const * fmt, ...);

    /**
       Returns a pointer to the buffer member. It may be
       invalidated by any changes to the buffer. Returns
       NULL if the buffer has never has any contents,
       but after that this will return an empty string if
       the buffer is empty.
    */
    char const * c_str() const throw();

    /**
       Throws an Exception using the given error code and the contents
       of the buffer as the message.
    */
    void toss(int errorCode) const;
  };

  /**
     Writes the first b.used() bytes of b.mem() to os and returns os.
  */
  std::ostream & operator<<( std::ostream & os, Buffer const & b );

  /**
     Accepts any type compatible with std::ostream<<v, converts
     it to a string (via std::ostringstream), then appends
     that result to b. Returns b.

  */
  template <typename ValueType>
  Buffer & operator<<( Buffer & b, ValueType const v ){
    std::ostringstream os;
    os << v;
    std::string const & s(os.str());
    if(!s.empty()){
      int const rc = fsl_buffer_append(b, s.data(),
                                       (fsl_size_t)s.size());
      if(rc) throw Exception(rc);
    }
    return b;
  }

  class Db;
  /**
     A prepared statement, the C++ counterpart to ::fsl_stmt.

     The vast majority of the members (those not marked with throw())
     throw an Exception on error.

     Sample usage:

     @code
     Stmt st(myDb);
     st.prepare("SELECT blah FROM foo WHERE id=?")
         .bind(1,42)
         .eachRow( MyStepFunctor() )
         .finalize();
     @endcode

     While they are normally created on the stack, they may live on
     the heap as long as they do not outlive their database.
  */
  class Stmt {
  public:
    /**
       Sets up initial state. Most of the member functions will throw
       until this statement is prepared (via Db::prepare()).
    */
    explicit Stmt(Db & db) throw();
    /**
       Frees any underlying resources.
     */
    ~Stmt() throw();

    /**
       Implicit conversion to (fsl_stmt *) to simplify usage with
       the C API.

       ABSOLUTELY DO NOT:

       - ... use this conversion to pass this object to
       fsl_stmt_finalize()!!!  Doing so will lead to a dangling
       pointer and an eventual segfault and/or double-free().

       - ... expect this conversion to be picked up when a function
       takes a void pointer argument.
    */
    operator fsl_stmt * () throw();

    /**
       Const-correct overload.
    */
    operator fsl_stmt const * () const throw();

    /**
       Basically a proxy for fsl_db_prepare(), this routine sets this
       object's statement up from the given formatted SQL string. See
       fsl_appendf() for the supported formatting specifiers.

       When passing on strings from external sources, it is safest to
       pass "%s" as the format string and the foreign string as the
       first variadic argument.  That ensures that percent signs in
       the foreign input do not get processed as format specifiers
       (which expect further arguments in the variadic list, which
       will likely lead to undefined behaviour).

       Throws on error. Returns this object.
    */
    Stmt & prepare(char const * sql, ... );

    /**
       Equivalent to this->prepare(db, "%s", sql.c_str()).
    */
    Stmt & prepare(std::string const & sql);

    /**
       Equivalent to this->prepare(db, "%s", sql.c_str()).
     */
    Stmt & prepare(Buffer const & sql);

    /**
       Steps one row. Returns true if it fetches a row, false at the
       end of the result set (and for non-fetching queries like
       INSERT/UPDATE/DELETE), and throws on error.
    */
    bool step();

    /**
       step()s the statement one time and throws if the step() call
       does _not_ return false. This is intended for stepping inserts,
       updates, and other non-fetching queries. Throws on error,
       returns this object on success.
    */
    Stmt & stepExpectDone();

    /**
       "Resets" the statement so it can be executed again, as per
       fsl_stmt_reset(). If resetStepCounterToo is true then the
       step-counter is also reset to 0 (see fsl_stmt_reset2() and
       stepCount()).

       Returns this object.
    */
    Stmt & reset(bool resetStepCounterToo = false);

    /**
       Frees any underlying resources owned by this statement.  It can
       be prepared() again after calling this.
    */
    Stmt & finalize() throw();

    /**
       Returns the number of times step() has fetched a row
       of data since this counter was last reset.
    */
    int stepCount() const throw();

    /**
       Returns the number of bound parameters in the statement.
    */
    int paramCount() const throw();

    /**
       Returns the number of "fetchable" columns in the statement.
    */
    int columnCount() const throw();

    /**
       Returns the SQL of the statement, or NULL if the statement is
       not prepared.
    */
    char const * sql() const throw();

    /**
       Returns this object's C-level fsl_stmt handle.

       Provided so that client APIs can use fsl_stmt_xxx() functions
       not wrapped by the C++ APIs. DO NOT, under ANY CIRCUMSTANCES,
       delete, free, fsl_stmt_finalize(), nor otherwise mess with this
       handle's ownership. It is owned by this object.

       Returns NULL if not yet prepared.
    */
    fsl_stmt * handle() throw();

    /**
       Const-correct overload.
    */
    fsl_stmt const * handle() const throw();

    /** Analog to fsl_stmt_g_int64() */
    int32_t getInt32(short colZeroBased);

    /** Analog to fsl_stmt_g_int64() */
    int64_t getInt64(short colZeroBased);

    /** Analog to fsl_stmt_g_id() */
    fsl_id_t getId(short colZeroBased);

    /** Analog to fsl_stmt_g_double() */
    double getDouble(short colZeroBased);

    /** Analog to fsl_stmt_g_text() */
    char const * getText(short colZeroBased, fsl_size_t * length = NULL);

    /** Analog to fsl_stmt_g_blob() */
    void const * getBlob(short colZeroBased, fsl_size_t * length = NULL);

    /** Analog to fsl_stmt_col_name() */
    char const * columnName(short colZeroBased);

    /** Analog to fsl_stmt_param_index() */
    int paramIndex(char const * name);

    /** Analog to fsl_stmt_bind_null() */
    Stmt & bind(short colOneBased);

    /** Analog to fsl_stmt_bind_int32() */
    Stmt & bind(short colOneBased, int32_t v);

    /** Analog to fsl_stmt_bind_int64() */
    Stmt & bind(short colOneBased, int64_t v);

    /** Analog to fsl_stmt_bind_int64() */
    Stmt & bind(short colOneBased, double v);

    /** Analog to fsl_stmt_bind_text() */
    Stmt & bind(short colOneBased, char const * str,
                fsl_int_t len = -1, bool copyBytes = true);

    /** Equivalent to bind(colOneBased, str.c_str(), str.size()); */
    Stmt & bind(short colOneBased, std::string const & str);

    /** Analog to fsl_stmt_bind_blob() */
    Stmt & bind(short colOneBased, void const * blob, fsl_size_t len,
                bool copyBytes = true);

    /** Analog to fsl_stmt_bind_null_name() */
    Stmt & bind(char const * col);

    /** Analog to fsl_stmt_bind_int32_name() */
    Stmt & bind(char const * col, int32_t v);

    /** Analog to fsl_stmt_bind_int64_name() */
    Stmt & bind(char const * col, int64_t v);

    /** Analog to fsl_stmt_bind_double_name() */
    Stmt & bind(char const * col, double v);

    /** Analog to fsl_stmt_bind_text_name() */
    Stmt & bind(char const * col, char const * str,
                fsl_int_t len = -1, bool copyBytes = true);

    /** Equivalent to bind(col, str.c_str(), str.size()); */
    Stmt & bind(char const * col, std::string const & str);

    /** Analog to fsl_stmt_bind_blob_name() */
    Stmt & bind(char const * col, void const * blob,
                fsl_size_t len, bool copyBytes = true);

    /**
       Runs a loop over func(*this), calling it once for each time
       this->step() returns true.
    */
    template <typename Func>
    Stmt & eachRow( Func const & func ){
      while(this->step()){
        func(*this);
      }
      return *this;
    }

    /**
       Runs a loop over func(*this, state), calling it once for each
       time this->step() returns true.
    */
    template <typename State, typename Func>
    Stmt & eachRow( Func const & func, State & state ){
      while(this->step()){
        func(*this, state);
      }
      return *this;
    }

    /**
       Binds each entry of the input iterator range [begin,end),
       starting at index 1 and moving up.
    */
    template <typename IteratorT>
    Stmt & bindIter( IteratorT const & begin, IteratorT const & end ){
      IteratorT it(begin);
      short col = 0;
      for( ; it != end; ++it ){
        this->bind( ++col, *it );
      }
      return *this;
    }
    /**
       Binds each entry of the given std::list-like type. Each entry
       in the list is bound starting at index 1 and moving up.
    */
    template <typename ListType>
    Stmt & bindList( ListType const & li ){
      return bindIter( li.begin(), li.end() );
    }

    /**
       Runs a loop over func(this->handle(), &state), calling it
       once for each time this->step() returns true. If func() returns
       0 the loop continues. If it returns FSL_RC_BREAK then the loop
       stops without an error. If some other value is returned an
       exception is thrown. Basically a templated form of
       fsl_stmt_each().
    */
    template <typename State>
    Stmt & eachRow( fsl_stmt_each_f func, State & state ){
      int rc = 0;
      while(this->step()){
        rc = func(&this->stmt, &state);
        switch(rc){
          case 0: continue;
          case FSL_RC_BREAK: break;
          default:
            this->propagateError();
            throw Exception(rc, "Statement eachRow() callback says: %s\n",
                            fsl_rc_cstr(rc));
        }
      }
      return *this;
    }

  private:
    Db & db;
    fsl_stmt stmt;
    friend class Db;
    /** Not implemented. Copying not allowed. */
    Stmt(Stmt const &);
    /** Not implemented. Copying not allowed. */
    Stmt & operator=(Stmt const &);

    /**
       Throws an Exception if col is not in the legal bind/get column
       range. base is the counting index (0 for "get", 1 for
       "bind"). It must be 0 or 1.
    */
    void assertRange(short col, short base) const;
    /**
       If rc is not 0 is throws an Exception. If rc==the error
       code of the underlying db handle then that error info
       is propagated.
    */
    void assertRC(char const * context, int rc) const;

    /**
       If this->stmt.db is not NULL and this->stmt.db->error.code is
       not 0 then it gets propagated as an Exception, else this is a
       no-op.
    */
    void propagateError() const;

    /**
       Throws a FSL_RC_MISUSE excception if this statement has not
       been prepared.
    */
    void assertPrepared() const;
  };// Stmt class

  /**
     A utility class for binding values to statements.

     Example usage:

     @code
     // Assume we have an INSERT or UPDATE statement
     // with 3 columns to bind:
     StmtBinder b(stmt);
     b
         (value1)(value2)(value3)
         .insert()
         (42)("hi")(someBlob, blobSize)
         .insert()
         ;
     @endcode
  */
  class StmtBinder{
  private:
    Stmt & st;
    short col;
  public:
    StmtBinder(Stmt &s);
    ~StmtBinder();

    /**
       Equivalent to stmt().bind(v,len,copyBytes),
       except that it returns this object.
     */
    StmtBinder & operator()(char const * v, fsl_int_t len = -1,
                            bool copyBytes = true);

    /**
       Equivalent to stmt().bind(v,len,copyBytes),
       except that it returns this object.
    */
    StmtBinder & operator()(void const * v, fsl_size_t len,
                            bool copyBytes = true);

    /**
       A generic bind() op which simply calls stmt().bind(X, v), where
       X is the next bind column for this object. Throws if the column
       is out of range or binding fails.

       Returns this object.
    */
    StmtBinder & operator()();
    template <typename ValT>
    StmtBinder & operator()(ValT v){
      st.bind(++this->col, v);
      return *this;
    }

    template <typename ListType>
    StmtBinder & bindList( ListType const & li ){
      this->st.bindList(li);
      return *this;
    }


    /**
       Resets this object's bind column counter. Calls stmt().reset()
       if alsoStatement is true. Returns this object.
    */
    StmtBinder & reset(bool alsoStatement = true);

    /**
       Returns the underlying statement handle.
    */
    Stmt & stmt();

    /**
       Equivalent to stmt().step().
    */
    bool step();

    /**
       Calls stepExpectDone() on the underlying statement, calls
       this->reset(), and returns this object. Can be used for any
       type of statement which is expected to trigger an
       FSL_RC_STEP_DONE from the underlying fsl_stmt API. Basically
       this means INSERT, UPDATE, REPLACE, and DELETE statements.

       The convenience factor of this function is that
       stepExpectDone() will throw if fsl_stmt_step()ing the
       underlying statement does _not_ return FSL_RC_STEP_DONE.
    */
    StmtBinder & once();

    /** Alias for once(), for readability. */
    StmtBinder & insert(){return this->once();}
    /** Alias for once(), for readability. */
    StmtBinder & update(){return this->once();}
  };

  /**
     C++ counterpart to ::fsl_db.

     The vast majority of the members (those not marked with throw())
     throw an Exception on error.
  */
  class Db {
    public:
    /**
       Initializes internal state for a closed db.  Use open() to open
       it or handle() to import a foreign fsl_db handle.
    */
    Db();

    /**
       Initializes internal state and calls Calls
       this->open(filename,openFlags).
    */
    Db(char const * filename, int openFlags);

    /**
       Calls this->close().
    */
    virtual ~Db() throw();

    /**
       Counterpart of fsl_db_open(), with the same semantics for the
       arguments, except that this function throws on error.

       Throws on error, else returns this object.

       Throws if the db is currently opened.
    */
    Db & open(char const * filename, int openFlags);


    /**
       Implicit conversion to (fsl_db *) to simplify usage with
       the C API.

       ABSOLUTELY DO NOT:

       - ... use this conversion to pass this object to
       fsl_db_close()!!!  Doing so will lead to a dangling pointer and
       an eventual segfault and/or double-free().

       - ... expect this conversion to be picked up when a function
       takes a void pointer argument.
    */
    operator fsl_db * () throw();

    /**
       Const-correct overload.
    */
    operator fsl_db const * () const throw();

    /**
       If this->ownsHandle() is true and this object has an opened db,
       it fsl_db_close() that db, (possibly) freeing the handle and
       (definitely) any db-owned resources.  If !this->ownsHandle()
       this this clears this object's reference to the underlying db
       but does _not_ fsl_db_close() it.  If this object has no Db
       handle then this is a harmless no-op.
    */
    Db & close() throw();

    /**
       Calls this->close() and replaces the internal db handle with
       the given one.

       If ownsHandle is true then this object takes over ownership of
       db. It is CRITICAL that there never be more than one owner (and
       that owner may be at the C level, unaware of this class). It is
       also critical that the "owning" Db instance (or exteran fsl_db
       handle not associated with an owning Db instance) outlive any
       shared Db instances using that handle. (We can't currently
       ensure that with a shared pointer due to C-level ownership
       internals.)

       If ownsHandle is false then this object becomes a proxy for the
       given handle but will never close the handle - its memory is
       assumed to live somewhere else and the fsl_db instance MUST
       OUTLIVE THIS OBJECT.

       It is legal to pass NULL db, which triggers a close() and then
       clears the Db handle. In that case, this object will create a
       new handle if open() is called on it while it has none.

       As a special case, if this->handle()==db, this is a no-op.

       The primarily intention of this mechanism is so that
       fsl::Context, which proxies up to three db handles, can do so
       without having to fight the C-level API for ownership of those
       handles.
    */
    Db & handle( fsl_db * db, bool ownsHandle ) throw();

    /**
       Returns the filename used to open the DB or NULL
       if it is not opened.
    */
    char const * filename() throw();

    /**
       Returns true if this object has an opened db handle,
       else false.
    */
    bool isOpened() const throw();

    /**
       Returns this object's C-level fsl_db handle.

       Provided so that client APIs can use fsl_db_xxx() functions
       not wrapped by the C++ APIs. DO NOT, under ANY CIRCUMSTANCES,
       delete, free, fsl_db_close(), nor otherwise mess with this
       handle's ownership. It is owned by this object.

       Returns NULL if not yet prepared.
    */
    fsl_db * handle() throw();

    /**
       Const-correct overload.
    */
    fsl_db const * handle() const throw();

    /**
       Returns true if this object owns its underlying db handle, else
       false. Note that it may legally return true even when
       handle() returns NULL, meaning that this object is prepared
       to create a handle of its own if needed.
    */
    bool ownsHandle() const throw();

    /**
       Pushes a level onto the pseudo-recursive transaction stack. See
       fsl_db_transaction_begin().

       Throws on error. Returns this object on success.

       DO NOT EVER use transactions in the API without this function.
       For example, NEVER exec("BEGIN") because that bypasses the
       recursive transaction support.

       For an exception-safer alternative to managing transaction
       lifetimes, see the Db::Transaction helper class.
    */
    Db & begin();

    /**
       Pops one level from the transaction stack as "successful,"
       commiting the transaction only once all levels have been
       popped.

       See fsl_db_transaction_commit().

       Throws on error. Returns this object on success.

       DO NOT EVER commit a transaction without this function.  For
       example, NEVER exec("COMMIT") because that bypasses the
       recursive transaction support.

    */
    Db & commit();

    /**
       Pops one level from the transaction stack and marks the whole
       transaction as failed. It will not actually roll back the
       transaction until all levels have been popped, but further
       commit() calls will implicitly fail until the top-most
       transaction level is committed, at which point it will
       recognize the failure and perform a rollback instead.

       See fsl_db_transaction_rollback().

       Throws on error. Returns this object on success.

       DO NOT EVER roll back a transaction without this function.  For
       example, NEVER exec("ROLLBACK") because that bypasses the
       recursive transaction support.
    */
    Db & rollback() throw();

    /**
       Executes the given single fsl_appendf()-formatted SQL
       statement. See fsl_appendf() for the formatting options.

       Throws on error. Returns this object.

       See Stmt::prepare() for notes about how to sanely deal with
       unsanitized SQL strings from foreign sources.
    */
    Db & exec(char const * sql, ...);

    /**
       Equivalent to this->exec("%s", sql.c_str()).
    */
    Db & exec(std::string const & sql);

    /**
       Executes one or more SQL statements provided in the
       fsl_appendf()-formatted SQL string.  See fsl_db_exec_multi()
       for details. This can be used to import whole schemas at once,
       for example.

       Throws on error. Returns this object.
    */
    Db & execMulti(char const * sql,...);

    /**
       Equivalent to this->execMulti("%s", sql.c_str()).
    */
    Db & execMulti(std::string const & sql);

    /** Analog to fsl_db_attach(), but throws on error. */
    Db & attach(char const * filename, char const * label);

    /** Analog to fsl_db_detach(), but throws on error. */
    Db & detach(char const * label);

    /**
       Returns the current number of transactions pushed onto the
       transaction stack. If this is greater than 0 then a transaction
       is active (though it might have a failure from a rollback()
       performed higher up in the stack).
    */
    int transactionLevel() const throw();

    /**
       A utility to simplify db transaction lifetimes.

       Sample usage:

       @code
       Db::Transaction tr(db);
       ...lots of stuff which might throw...
       tr.commit();
       @endcode

       If commit() is not called, the transaction will be rolled back
       then the Transaction instance destructs.
    */
    class Transaction {
    private:
      Db & db;
      int inTrans;
      //! Not copyable.
      Transaction & operator=(Transaction const &);
      //! Not copyable.
      Transaction(Transaction const &);
    public:

      /**
         Calls db.begin().
      */
      Transaction(Db & db);

      /**
         If neither commit() nor rollback() have been called,
         this calls rollback(), otherwise it does nothing.
      */
      ~Transaction() throw();

      /**
         Commits the transaction started by the ctor.
      */
      void commit();

      /**
         Rolls back the transaction started by the ctor.
      */
      void rollback() throw();

      /**
         Returns the current transaction level for the underlying db.
      */
      int level() const throw();
    };


    private:

    friend class Stmt;

    /**
       Only mutable so that we can steal db.error's contents when
       propagating exceptions. If it's not mutable then we have to
       copy that state at throw-time.

       FIXME: we really need a weak pointer and/or a shared ptr with a
       configurable finalizer.
    */
    mutable fsl_db * db;
    /**
       Whether or not this instance owns this->db.
    */
    bool ownsDb;

    /**
       Not implemented - copying not allowed.
    */
    Db(Db const &);

    /**
       Not implemented - copying not allowed.
    */
    Db & operator=(Db const &);

    void setup();

    /** Throws if rc is not 0. */
    void assertRC(char const * context, int rc) const;

    /** Throws if the db is not opened. */
    void assertOpened() const;

    /**
       Throws db->error state if db is opened and db->error.code is
       not 0, otherwise this is a no-op.
    */
    void propagateError() const;
  };

  /**
     C++ counterpart of ::fsl_cx, but generally requires less code to
     use because it throws exceptions for any notable errors.

     Example:

     @code
     Context cx;
     cx.openCheckout();

     fsl_uuid_cstr uuid = NULL;
     fsl_id_it rid = 0;
     fsl_ckout_version_info( cx, &rid, &uuid );
     // ^^^ note, that's a C function!
     FslOutputFStream os(cx);
     os << "Checkout version: "<<rid<< ' ' << uuid << '\n';
     @endcode

     The implicit conversion to ::fsl_cx makes it possible to pass
     instances to any C function taking such an argument (and legal to
     do so for most function).
  */
  class Context{
    public:
    /**
       Initializes a new fsl_cx instance, owned by this object, using
       the default initialization options.

       Throws on error.
    */
    Context();

    /**
       Initializes a new fsl_cx instance, owned by this object, using
       the given initialization options.

       Throws on error.
    */
    explicit Context(fsl_cx_init_opt const & opt);

    /**
       Initializes this object as a wrapper of the given initialized
       (via fsl_cx_init()) fsl_cx handle. If ownsHandle is true then
       ownership of f is transfered to this object. If ownsHandle is
       false then this object is just a "thin" proxy for f and f MUST
       OUTLIVE THIS OBJECT.
    */
    Context(fsl_cx * f, bool ownsHandle);

    /**
       If this object owns its context handle, it fsl_cx_finalize()s
       it, otherwise it does nothing.

       Note that this is not virtual. It's not expected that
       subclassing will be all that useful for this class.
    */
    ~Context();

    /**
       Implicit conversion to (fsl_cx *) to simplify usage with
       the C API.

       ABSOLUTELY DO NOT use this conversion...

       - ... to pass this object to fsl_cx_finalize()!  Doing so will
       lead to a dangling pointer and an eventual segfault and/or
       double-free().

       - ... with fsl_ckout_close() or fsl_repo_close() because
       pointer ownership may get confused. Use closeDbs() instead.  It
       will appear to work at times, but certain combinations of
       operations via the C API (e.g. opening a checkout, closing it,
       then opening a standalone repo) might get some C++-side pointers
       cross-wired (theoretically/hypothetically).

       - ... expect this conversion to be picked up when a function
       takes a void pointer argument.
    */
    operator fsl_cx * () throw();

    /**
       Const-correct overload.
    */
    operator fsl_cx const * () const throw();

    /**
       Returns this object's C-level fsl_cx handle.

       See operator fsl_cx *() for details.
    */
    fsl_cx * handle() throw();

    /**
       Returns true if this object owns its underlying db handle, else
       false. Note that it may legally return true even when
       handle() returns NULL, meaning that this object is prepared
       to create a handle of its own if needed.
    */
    bool ownsHandle() const throw();

    /**
       Const-correct overload.
    */
    fsl_cx const * handle() const throw();

    /**
       Counterpart of fsl_ckout_open_dir(). Throws on error,
       returns this object on success.
    */
    Context & openCheckout( char const * dirName = NULL );

    /**

     */
    Context & openRepo( char const * dbFile );

    /**
       Closes any opened repo/checkout/config databases. ACHTUNG: this
       may invalidate any Db handles pointing to them!

       Returns this object.

       ACHTUNG: because of ownership issues, clients must always use
       this function, instead of the C APIs, for closing repositories
       and checkouts.
    */
    Context & closeDbs() throw();

    /**
       Like fsl_rid_to_uuid(*this, rid), but returns the result as a
       std::string and throws on error or if no entry is found.
    */
    std::string ridToUuid(fsl_id_t rid);

    /**
       Like fsl_rid_to_artifact_uuid(*this, rid, type), but returns
       the result as a std::string and throws on error or if no entry
       is found.
    */
    std::string ridToArtifactUuid(fsl_id_t rid,
                                  fsl_satype_e type = FSL_SATYPE_ANY);

    /**
       Like fsl_sym_to_rid(*this,symbolicName), but throws on error
       or if no ID is found.
    */
    fsl_id_t symToRid(char const * symbolicName,
                      fsl_satype_e type = FSL_SATYPE_ANY);

    /**
       Equivalent to symToRid(symbolicName.c_str(), type);
    */
    fsl_id_t symToRid(std::string const & symbolicName,
                      fsl_satype_e type = FSL_SATYPE_ANY);

    /**
       Like fsl_sym_to_uuid(*this,...), but returns the result as a
       std::string and throws on error or if no entry is found. If rid
       is not NULL then on success the RID corresponding to the
       returned UUID is returned via *rid.
    */
    std::string symToUuid(char const * symbolicName,
                          fsl_id_t * rid = NULL,
                          fsl_satype_e type = FSL_SATYPE_ANY);

    /*Context & handle( fsl_db * db, bool ownsHandle ) throw();*/

    /**
       This returns a handle to the repository database. It might
       not be opened. The handle and its db connection are
       owned by this object resp. by lower levels of the API.
    */
    Db & dbRepo() throw();

    /**
       This returns a handle to the checkout database. It might
       not be opened. The handle and its db connection are
       owned by this object resp. by lower levels of the API.

       Note that if a checkout is opened, an repo will also be opened,
       but not necessarily the other way around.
    */
    Db & dbCheckout() throw();

    /**
       This returns a handle to the context's "main" database. It
       might not be opened. The handle and its db connection are owned
       by this object resp. by lower levels of the API.
    */
    Db & db() throw();

    /**
       A utility class for managing transactions for a Context-managed
       database (regardless of whether the checkout or repo db).
     */
    class Transaction {
      private:
      Db::Transaction tr;
      int level;
    public:
      /**
         Starts a transaction in cx.db(). Throws on error.
      */
      Transaction(Context &cx);
      /**
         If commit() has not been called, this rolls back the
         transaction, otherwise it does nothing.
       */
      ~Transaction() throw();
      /**
         Commits the transaction resp. pushes this level of the
         transaction stack off the stack.
      */
      void commit();
    };

    /**
       Analog to fsl_content_get(), but throws an error and returns
       this object.
    */
    Context & getContent( fsl_id_t rid, Buffer & dest );

    /**
       Analog to fsl_content_get_sym(), but throws an error and returns
       this object.
    */
    Context & getContent( char const * sym, Buffer & dest );

    /**
       Equivalent to getContent(sym.c_str(), dest).
    */
    Context & getContent( std::string const & sym, Buffer & dest );

    private:
    /**
       FIXME: we really need a weak pointer and/or a shared ptr with a
       configurable finalizer.
     */
    fsl_cx * f;
    bool ownsCx;
    Db dbCkout;
    Db dbRe;
    Db dbMain;
    /**
       Not implemented - copying not allowed.
    */
    Context(Context const &);
    /**
       Not implemented - copying not allowed.
    */
    Context & operator=(Context const &);

    void assertRC(char const * context, int rc) const;
    void assertHasRepo();
    void assertHasCheckout();
    void propagateError() const;

    void setup(fsl_cx_init_opt const * opt);
  };


  /**
     A utility class for iterating over fsl_list instances.

     VT must be a pointer-qualified type because fsl_list only holds
     arrays of pointers.

     Modification of the list invalidates any active iterators
     (semantically, but not technically, so be careful!).

     Example usage:

     @code
     typedef FslListIterator<char const *> Iter;
     // assume d is a fsl_deck instance.
     Iter it(d.P);
     Iter end;
     for( ; it != end; ++it ){
       char const * str = *it;
       if(str){...}
     }
     @endcode

     Note that this is not type-safe per se - it relies on the
     underlying list having only entries of the given type or NULL.

     When used in conjunction with the various fsl_list members of
     fsl_deck, it is important to remember that traversing over
     fsl_deck::F.list will often, but not always, behave much
     differently then FCardIterator because this class traverses only
     the F-cards found directly in that deck, which for delta
     manifests is only a small fraction of the F-cards actually in
     that version (the rest are inherited from its baseline
     manifest). FCardIterator is generally the right way to traverse
     the F-cards (though this approach has its uses as well).
  */
  template <typename VT>
  class FslListIterator{
  private:
    fsl_list const *li;
    fsl_int_t cursor;
    VT current;
  protected:
    VT currentValue() const throw(){
      return this->current;
    }
  public:
    //! STL-compatible value_type typedef.
    typedef VT value_type;
    //! API-conventional ValueType.
    typedef VT ValueType;

    /**
       Initializes this iterator to point to the first item im list
       (if list.used>0) or to be equivalent to the end iterator (if
       the list is empty).

       Changes to the list semantically invalidate all iterators, and
       using them afterwars invokes undefined behaviour.
    */
    explicit FslListIterator(fsl_list const &list)
    : li(&list), cursor(-1), current(NULL){
      if(li->used){
        current = static_cast<value_type>(li->list[0]);
        cursor = 1;
      }
    }

    /**
       Initializes an end iterator.
     */
    FslListIterator()
      : li(NULL), cursor(-1), current(NULL)
    {}

    /**
       Increments the iterator to point at the next list entry. Throws
       if called on an end iterator or if called after the end of the
       list has been reached.
    */
    FslListIterator & operator++(){
      if((cursor<0) || ((fsl_size_t)cursor>li->used)){
        throw Exception(FSL_RC_RANGE,
                        "Cannot increment past end of list.");
      }else if(li->used==(fsl_size_t)cursor){
        current = NULL;
        cursor = -1;
      }else{
        current = static_cast<value_type>(li->list[cursor++]);
      }
      return *this;
    }

    /**
       Returns the current element's value (possibly NULL!). Throws
       for an end iterator.
    */
    ValueType operator*(){
      if(cursor<0){
        throw Exception(FSL_RC_RANGE,
                        "Invalid iterator dereference.");
      }
      return current;
    }

    bool operator==(FslListIterator const &rhs) const throw(){
      return (this->cursor==rhs.cursor);
    }

    bool operator!=(FslListIterator const &rhs) const throw(){
      return (this->cursor!=rhs.cursor);
    }
  };

  /**
     The C++ counterpart to the C-side fsl_deck class.

     Fossil's core metadata syntax calls the entries of the metadata
     "cards." A Deck (or fsl_deck) is a "collection of cards" which
     make up an atomic unit of metadata. In Fossil jargon a deck is
     called an "artifact," but libfossil adopted the name "deck"
     because "artifact" already has several meanings in this context.
  */
  class Deck{
  public:
    /**
       If this instance owns its underlying handle then this cleans up
       all resources owned by this instance, otherwise it does
       nothing.
    */
    ~Deck() throw();

    /**
       Initializes the deck using the given context. The second
       argument is only important when constructing decks, not
       when loading them.
     */
    explicit Deck(Context & cx, fsl_satype_e type = FSL_SATYPE_ANY);

    /**
       Makes this object a wrapper for d. If ownsHandle is true then
       this object takes over ownership of d, otherwise d is assumed
       to be owned elsewhere and it _must_ outlive this object.
    */
    Deck(Context & cx, fsl_deck * d, bool ownsHandle);

    /**
       Implicit conversion to (fsl_deck *) to simplify integration
       with the C API.

       ABSOLUTELY DO NOT use this conversion...

       - ... with fsl_deck_finalize(), as that may (depending on usage)
       steal a pointer out from under C++.

       - ... expect this conversion to be picked up when a function
       takes a void pointer argument.
    */
    operator fsl_deck *() throw();

    /**
       Const-correct overload.
     */
    operator fsl_deck const *() const throw();

    /**
       See operator fsl_deck*().
    */
    fsl_deck * handle() throw();

    /**
       Const-correct overload.
    */
    fsl_deck const * handle() const throw();

    /**
       If this deck was load()ed, returns the loaded artifact's type,
       else returns the type set in the constructor.
    */
    fsl_satype_e type() const throw();

    /**
       If this deck was load()ed, returns the blob.rid
       value, else returns 0.
    */
    fsl_id_t rid() const throw();

    /**
       If this deck was load()ed, returns the UUID string,
       else returns NULL. The bytes are owned by this
       object and may be invalidated by load().
    */
    fsl_uuid_cstr uuid() const throw();

    /**
       Analog to fsl_deck_has_required_cards().
    */
    bool hasAllRequiredCards() const throw();

    /**
       Analog to fsl_deck_required_cards_check(),
       but throws if that fails.
    */
    Deck const & assertHasRequiredCards() const;

    /**
       Analog to fsl_card_is_legal(), passing this->type() as the
       first argument to that function.
    */
    bool cardIsLegal(char cardLetter) const throw();

    /**
       Analog to fsl_deck_load_sym(), but throws on error. Populates this
       object with the loaded state.
    */
    Deck & load( fsl_id_t rid, fsl_satype_e type = FSL_SATYPE_ANY );

    /**
       Analog to fsl_deck_load_sym(), but throws on error. Populates this
       object with the loaded state.
    */
    Deck & load( char const * symbolicName, fsl_satype_e type = FSL_SATYPE_ANY );

    /**
       Equivalent to load(symbolicName.c_str(), type).
    */
    Deck & load( std::string const & symbolicName, fsl_satype_e type = FSL_SATYPE_ANY );

    /**
       This deck's Fossil Context.
    */
    Context & context() throw();

    /**
       Const-correct overload.
    */
    Context const & context() const throw() ;

    /**
       Equivalent to fsl_deck_clean(*this).
    */
    Deck & cleanup() throw();

    /**
       Analog to fsl_deck_output(), sending its output to the given
       stream. Throws on error and returns this object on success.
    */
    Deck const & output( std::ostream & os ) const;

    /**
       Analog to fsl_deck_output(), but throws on error
       and returns this object on success.
    */
    Deck const & output( fsl_output_f f, void * outState ) const;

    /**
       Analog to fsl_deck_save(), but throws on error and returns this
       object on success.
    */
    Deck & save(bool isPrivate = false);

    /**
       Analog to fsl_deck_unshuffle(), but throws on error and returns
       this object on success.

       Reminder: this is only necessary when using output().
    */
    Deck & unshuffle(bool calcRCard = true);

    /**
       Analog to fsl_deck_A_set() but throws on error and returns this
       object on success.
    */
    Deck & setCardA( char const * name,
                     char const * tgt,
                     fsl_uuid_cstr uuid );

    /**
       Analog to fsl_deck_B_set() but throws on error and returns this
       object on success. This destroys any object previously returned
       by baseline().
    */
    Deck & setCardB(fsl_uuid_cstr uuid);

    /**
       Analog to fsl_deck_C_set() but throws on error and returns this
       object on success.
    */
    Deck & setCardC( char const * comment );

    /**
       Analog to fsl_deck_D_set(), but uses the current time
       if (julianDay<0) and throws on error.
    */
    Deck & setCardD(double julianDay = -1.0);

    /**
       Analog to fsl_deck_E_set() but throws on error and returns this
       object on success. If the 2nd argument is less than 0 then
       the current time is used by default.
    */
    Deck & setCardE( fsl_uuid_cstr uuid, double julian = -1.0 );

    /**
       Analog to fsl_deck_F_add() but throws on error and returns this
       object on success.
    */
    Deck & addCardF(char const * name, fsl_uuid_cstr uuid,
                    fsl_fileperm_e perm = FSL_FILE_PERM_REGULAR, 
                    char const * oldName = NULL);

    /**
       Analog to fsl_deck_J_add() but throws on error and returns this
       object on success.
    */
    Deck & addCardJ( char isAppend, char const * key, char const * value );

    /**
       Analog to fsl_deck_K_set() but throws on error and returns this
       object on success.
    */
    Deck & setCardK(fsl_uuid_cstr uuid);

    /**
       Analog to fsl_deck_L_set() but throws on error and returns this
       object on success.
    */
    Deck & setCardL( char const * title );

    /**
       Analog to fsl_deck_M_add() but throws on error and returns this
       object on success.
    */
    Deck & addCardM(fsl_uuid_cstr uuid);

    /**
       Analog to fsl_deck_N_set() but throws on error and returns this
       object on success.
    */
    Deck & setCardN(char const * name);

    /**
       Analog to fsl_deck_P_add() but throws on error and returns this
       object on success.
    */
    Deck & addCardP(fsl_uuid_cstr uuid);

    /**
       Analog to fsl_deck_Q_add() but throws on error and returns this
       object on success.
    */
    Deck & addCardQ(char type, fsl_uuid_cstr target,
                    fsl_uuid_cstr baseline);

    /**
       Analog to fsl_deck_T_add() but throws on error and returns this
       object on success.
    */
    Deck & addCardT(fsl_tagtype_e tagType,
                    char const * name,
                    fsl_uuid_cstr uuid = NULL,
                    char const * value = NULL);

    /**
       Sets the U-card, analog to fsl_deck_U_set().  If name is NULL
       or !*name then fsl_cx_user_get(this->context()) is used
       to fetch the name. An empty name is not legal.

       Throws on error.
     */
    Deck & setCardU(char const * name = NULL);

    /**
       Analog to fsl_deck_W_set() but throws on error and returns this
       object on success.
    */
    Deck & setCardW(char const * content, fsl_int_t len = -1);

    /**
       If this is a CHECKIN deck and it is a delta manifest then its
       baseline is lazily loaded (if needed) and returned.  Throws on
       loading error. Returns NULL if this is not CHECKIN or not a
       delta manifest. The returned object is owned by this object
       and will be cleaned up when it is or when the B-card is re-set
       (setCardB()).
    */
    Deck * baseline();

    /**
       An iterator type for traversing lists of T-cards (tags) in a
       deck.

       Example usage:

       @code
       Deck::TCardIterator it(myDeck);
       Deck::TCardIterator end;
       for( ; it != end; ++it ){
         std::cout
           << fsl_tag_prefix_char(it->type)
           << it->name << '\n';
       }
       @endcode
    */
    class TCardIterator : public FslListIterator<fsl_card_T const *>{
    private:
      typedef FslListIterator<fsl_card_T const *> ParentType;
    public:
      /**
         Constructs a "begin" iterator for d's T-cards.
       */
      TCardIterator(Deck & d);
      /**
         Constructs an "end" iterator for a T-card list.
      */
      TCardIterator();
      ~TCardIterator() throw();

      /**
         Returns the same as *this, but throws for an end
         iterator.
      */
      fsl_card_T const * operator->() const;
    };

    /**
       An STL-style iterator class for use with traversing the F-cards
       in a Deck object. Because of how delta manifests work, F-cards
       have rather intricate traversal rules. This class helps hide
       those from the client (the only one it exposes is that F-cards
       are required to be in strict lexical order).

       Reminder: client code must be prepared to handle F-cards with
       NULL UUIDs. They appear when a file is removed between a
       baseline manifest and its delta. The delta marks deletions with
       a NULL UUID. A baseline manifest marks deletions by simply not
       including the file in the manifest (no F-card). The library
       "could" skip such entries when iterating, but knowing about
       deleted entries is useful at times.

       Potential TODO: a flag to this class which tells it to
       skip over deleted entries.
    */
    class FCardIterator {
    private:
      Deck * d;
      fsl_card_F const * fc;
      bool skipDeleted;
      void assertHasDeck();
    public:
      /**
         Rewinds d's F-card list and initializes this iterator to point
         to the first F-card in d. Remember that only decks of type
         FSL_SATYPE_CHECKIN have F-cards. Throws if rewinding fails (it
         only fails if lazy loading of a baseline manifest fails).
      */
      explicit FCardIterator(Deck & d, bool skipDeletedFiles = false);

      /**
         Constructs an "end" iterator.
      */
      FCardIterator() throw();

      ~FCardIterator() throw();

      /**
         Prefix increment: advances iterator and returns the new value.

         A no-op for an "end" iterator.

         If the skip-deleted flag was passed to the constructor then
         F-cards which represent deleted entries are skipped during
         traversal.
      */
      FCardIterator & operator++();

      /**
         The current F-card, or NULL at the end of the list.
      */
      fsl_card_F const * operator*();

      /**
         Convenience operator. Throws for an "end" iterator.
      */
      fsl_card_F const * operator->();

      /** Compares this object and rhs by name. */
      bool operator==(FCardIterator const &rhs) const throw();
      /** Compares this object and rhs by name. */
      bool operator!=(FCardIterator const &rhs) const throw();
      /** Compares this object and rhs by name. */
      bool operator<(FCardIterator const &rhs) const throw();
    };

  private:
    Context & cx;
    fsl_deck * d;
    Deck * deltaBase;
    bool ownsDeck;
    void setup(fsl_deck * d, fsl_satype_e type);
    void propagateError() const;
    void assertRC(char const * context, int rc) const;
    /** Not implemented - copying not currently allowed. */
    Deck(Deck const &);
    /** Not implemented - copying not currently allowed. */
    Deck & operator=(Deck const &);
  };

  /** Calls d.output(os) and returns os. */
  std::ostream & operator<<( std::ostream & os, Deck const & d );

  /**
     A fsl_appendf_f() implementation which requires that state be a
     std::ostream pointer. It uses std::ostream::write() to append n
     bytes of the data argument to the output stream. If the write()
     operation throws, this function catches it and returns FSL_RC_IO
     instead (because we propagating exceptions across the C API has
     undefined behaviour). Returns 0 on success, FSL_RC_IO if the
     stream is in an error state after the write.
  */
  fsl_int_t fsl_appendf_f_std_ostream( void * state, char const * data,
                                   fsl_int_t n );

  /**
     A fsl_output_f() implementation which requires that state be a
     std::ostream pointer. It uses std::ostream::write() to append n
     bytes of the data argument to the output stream. If the write()
     operation throws, this function catches it and returns FSL_RC_IO
     instead (because propagating exceptions across the C API has
     undefined behaviour). Returns 0 on success, FSL_RC_IO if the
     stream is in an error state after the write.
  */
  int fsl_output_f_std_ostream( void * state, void const * data,
                                fsl_size_t n );

  /**
     A fsl_input_f() implementation which requires state to be a
     std::istream pointer. Characters are read from the stream until
     *n bytes are read or EOF is reached. If EOF is reached, *n is set
     to the number of bytes consumed (and written to dest) before
     reaching EOF. If the input operation throws, this function
     catches it and returns FSL_RC_IO instead (because propagating
     exceptions across the C API has undefined behaviour)

     Example usage:

     @code
     char const * filename = "some-file";
     std::ifstream is(filename);
     if(!is.good()) throw Exception(FSL_RC_IO,"Cannot open: %s", filename); 
     fsl_buffer buf = fsl_buffer_empty;
     int rc = fsl_buffer_fill_from( &buf,
                                    fsl_input_f_std_istream,
                                    &is );
     if(rc) {...error...}
     else {...okay...}
     fsl_buffer_clear(&buf); // in error cases it might be partially filled!
     @endcode

     Better yet, use try/catch to better protect the buffer from
     leaks:

     @code
     fsl_buffer buf = fsl_buffer_empty;
     try{
       ... do i/o here ...
     }catch(...){
       fsl_buffer_clear(&buf);
       throw;
     }
     fsl_buffer_clear(&buf);
     @endcode

     _Even better_, use the Buffer class to manage buffer memory
     lifetime, making it inherently exception-safe:

     @code
     std::ifstream is(filename);
     Buffer buf;
     int rc = fsl_buffer_fill_from(buf, fsl_input_f_std_istream, &is );
     if(rc) throw Exception(rc); // now buf will not leak
     ...
     @endcode
  */
  int fsl_input_f_std_istream( void * state, void * dest, fsl_size_t * n );

  /**
     A std::streambuf impl which redirects a std::streambuf to
     fsl_output(). Can be used, e.g. to redirect std::cout and
     std::cerr to fsl_output().
  */
  class ContextOStreamBuf : public std::streambuf {
  private:
    fsl_cx * f;
    std::ostream * m_os;
    std::streambuf * m_old;
    void setup( fsl_cx * f );
  public:
    /**
       Redirects os's buffer to use this object, such that all output
       sent to os will instead go through this buffer to
       fsl_output(f,...). os must outlive this object. When this
       object destructs, os's old buffer is restored.

       Throws if f is NULL.

       Example:

       @code
       ContextOStreamBuf sb(myFossil, std::cout);
       std::cout << "This now goes through fsl_output(myFossil,...).\n";
       @endcode
    */
    ContextOStreamBuf( fsl_cx * f, std::ostream & os );

    /**
       Equivalent to passing cx.handle() to the other two-arg ctor.
    */
    ContextOStreamBuf( Context & cx, std::ostream & os );

    /**
       Redirects all output sent to this buffer to fsl_output(f,...).

       Throws if f is NULL.
    */
    explicit ContextOStreamBuf( fsl_cx * f );

    /**
       Equivalent to passing cx.handle() to the other one-arg ctor.
    */
    explicit ContextOStreamBuf( Context & cx );

    /**
       Flushes the buffer via this->sync();

       If this object wraps a stream, that streams buffer is then
       restored to its prior state.
    */
    virtual ~ContextOStreamBuf() throw();

    /**
       Outputs c as a single char via fsl_output(), using the fsl_cx
       instance passed to the constructor.

       On a write error it throws, else it returns 0.
    */
    virtual int overflow( int c );

    /**
       Falls fsl_flush(), passing it the fsl_cx instance passed to the
       ctor. Returns the result of that call.
    */
    virtual int sync();

  };

  /**
     A std::ostream which redirects its output to the output channel
     configured for a fsl_cx instance.

     Example usage:

     @code
     ContextOStream os(someContext.handle());
     os << "hi, world!\n"; // goes through fsl_output()
     @endcode
  */
  class ContextOStream : public std::ostringstream {
  private:
    fsl_cx * f;
    ContextOStreamBuf * sb;
  public:

    /**
       Sets up this buffer to direct all stream output sent to this
       buffer to fsl_output() instead, using f as the first argument
       to that function.

       Ownership of f is not changed. f must outlive this object.
     */
    explicit ContextOStream( fsl_cx * f );

    /**
       Equivalent to passing cx.handle() to the other ctor.
    */
    explicit ContextOStream( Context & cx );

    /**
       If initialized, it calls fsl_flush(), otherwise it has
       no visible side-effects.
    */
    virtual ~ContextOStream() throw();

    /**
       Appends a formatted string, as per fsl_outputf(), to the
       stream. This is primarily intended for adding SQL-related
       escaping to the buffer using the %q/%Q specifiers.

       Returns this object.
    */
    ContextOStream & appendf(char const * fmt, ...);
  };

  /**
     A std::streambuf impl which redirects a std::streambuf to a
     fsl_output_f(). It can be used, e.g. to redirect std::cout and
     std::cerr to a client-specific callback.
  */
  class FslOutputFStreamBuf : public std::streambuf {
  private:
    fsl_output_f out;
    void * outState;
    std::ostream * m_os;
    std::streambuf * m_old;
    void setup( fsl_output_f f, void * state );
  public:
    /**
       Redirects os's buffer to use this object, such that all output
       sent to os will instead go through this buffer to
       fsl_output(f,...). os must outlive this object. When this
       object destructs, os's old buffer is restored.

       Throws if !out.

       Example:

       @code
       FslOutputFStreamBuf sb(myCallback, callbackState, std::cout);
       std::cout << "This now goes through fsl_output(myFossil,...).\n";
       @endcode
    */
    FslOutputFStreamBuf( fsl_output_f out, void * outState,
                       std::ostream & os );

    /**
       Sets up output sent to this stream to go throug
       out(outState,...).

       Throws if !out.
     */
    FslOutputFStreamBuf( fsl_output_f out, void * outState );

    /**
       If this object wraps a stream, that stream's buffer is restored
       to its prior state.
    */
    virtual ~FslOutputFStreamBuf() throw();

    /**
       Outputs c as a single byte via the output function provided to
       the ctor. Throws on error.
    */
    virtual int overflow( int c );

    /**
       Does nothing. Returns 0.
    */
    virtual int sync();
  };


  /**
     This std::ostream subclass which proxies a fsl_output_f()
     implementation, sending all output to that function.

     The stream throws on output errors.

     Example usage, sending output to a Buffer using stream
     operators:

     @code
     Buffer buf;
     FslOutputFStream os(fsl_output_f_buffer, buf.handle());
     // ^^^ For this particular case MAKE SURE to pass the C
     // fsl_buffer handle, NOT the C++ Buffer handle!
     os << "hi, world!";
     assert(10==buf.used());

     // Or, more simply:
     BufferOStream bos(buf);
     bos << "hi, world!";
     @endcode
  */
  class FslOutputFStream : public std::ostream {
  private:
    /** The underlying proxy buffer. */
    FslOutputFStreamBuf * sb;
    /**
       fsl_appendf_f() impl which requires state to be a
       FslOutputFStream pointer. All output gets sent to this stream's
       proxy function.
    */
    static fsl_int_t fslAppendfF( void * state, char const * s, fsl_int_t n );

  public:

    /**
       Sets up this stream to direct all stream output sent to this
       buffer to out(outState, ...) instead.

       Ownership of outState is not changed. outState, if not NULL,
       must outlive this object.

       Throws if !out.
    */
    explicit FslOutputFStream( fsl_output_f out, void * outState );

    /**
       Cleans up its internal resources.
    */
    virtual ~FslOutputFStream() throw();

    /**
       Appends a formatted string, as per fsl_outputf(), to the
       stream. This is primarily intended for adding SQL-related
       escaping to the buffer using the %q/%Q specifiers.

       Returns this object.
    */
    FslOutputFStream & appendf(char const * fmt, ...);
  };


  class BufferOStream : public FslOutputFStream{
  public:
    /**
       Sets up this object to redirect all stream output to the given
       buffer. Ownership of b is not changed and b must outlive this
       stream. It is legal to implicitly convert a Buffer object for
       this purpose.

       Throws if b is NULL.
    */
    explicit BufferOStream(fsl_buffer * b);
    /**
       Does nothing.
    */
    ~BufferOStream() throw();
  };

}/*namespace fsl*/

#endif
/* ORG_FOSSIL_SCM_LIBFOSSIL_HPP_INCLUDED */
