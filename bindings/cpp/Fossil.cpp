/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#include "fossil-scm/fossil.hpp"

/**

 */
namespace fsl {


  Buffer::~Buffer() throw(){
    this->clear();
  }

  Buffer::Buffer(fsl_size_t startingSize)
    : buf(fsl_buffer_empty)
  {
    if(startingSize){
      if(fsl_buffer_reserve(&this->buf, startingSize)){
        throw OOMException();
      }
    }
  }

  Buffer::Buffer()
    : buf(fsl_buffer_empty)
  {}

  Buffer & Buffer::operator=(Buffer const & other){
    fsl_size_t const oldSize = this->buf.capacity;
    this->reset();
    if((&other != this)
       && !other.empty()
       && fsl_buffer_append(*this, other.mem(), other.used())
       ){
      throw OOMException();
    }
    /* A rather arbitrary heuristic to determine whether or not
       we should free up some spce by to resizing buf()
       after re-using it... */
    if((this->buf.capacity == oldSize)
       && (this->buf.capacity > 20)
       && (this->buf.used < (this->buf.capacity/2))){
      fsl_buffer_resize( *this, this->buf.used );
    }
    return *this;
  }

  Buffer::Buffer(Buffer const & other)
  : buf() {
    if(!other.empty()
       && fsl_buffer_append(*this, other.mem(), other.used())
       ){
      throw OOMException();
    }
  }

  Buffer::operator fsl_buffer *() throw(){
    return &this->buf;
  }

  Buffer::operator fsl_buffer const *() const throw(){
    return &this->buf;
  }

  fsl_buffer * Buffer::handle() throw(){
    return &this->buf;
  }
  
  fsl_buffer const * Buffer::handle() const throw(){
    return &this->buf;
  }

  bool Buffer::empty() const throw() { return 0==this->buf.used; }

  fsl_size_t Buffer::used() const throw() { return this->buf.used; }

  fsl_size_t Buffer::capacity() const throw() { return this->buf.capacity; }

  unsigned char const * Buffer::mem() const throw() {
    return this->buf.used
      ? this->buf.mem
      : NULL;
  }

  unsigned char * Buffer::mem() throw() {
    return this->buf.used
      ? this->buf.mem
      : NULL;
  }

  Buffer & Buffer::clear() throw() {
    fsl_buffer_clear(*this);
    return *this;
  }

  Buffer & Buffer::reset() throw() {
    fsl_buffer_reuse(*this);
    return *this;
  }

  Buffer & Buffer::reserve(fsl_size_t n){
    int const rc = fsl_buffer_reserve(*this, n);
    if(rc) throw Exception(rc, "Buffer::reserve() failed");
    return *this;
  }

  Buffer & Buffer::resize(fsl_size_t n){
    int const rc = fsl_buffer_resize(*this, n);
    if(rc) throw Exception(rc, "Buffer::resize() failed");
    return *this;
  }

  Buffer::iterator Buffer::begin() throw() {
    return this->buf.used
      ? this->buf.mem
      : NULL;
  }

  Buffer::iterator Buffer::end() throw() {
    return this->buf.used
      ? this->buf.mem + this->buf.used
      : NULL;
  }

  Buffer::const_iterator Buffer::begin() const throw() {
    return this->buf.mem;
  }

  Buffer::const_iterator Buffer::end() const throw() {
    return this->buf.used
      ? this->buf.mem + this->buf.used
      : NULL;
  }

  std::ostream & operator<<( std::ostream & os, Buffer const & b ){
    fsl_size_t n = 0;
    char const * s = fsl_buffer_cstr2(b, &n);
    if(n) os.write(s, n);
    return os;
  }

  Buffer & Buffer::appendf(char const * fmt, ...){
    va_list vargs;
    int rc = 0;
    va_start(vargs,fmt);
    rc = fsl_buffer_appendfv(*this, fmt, vargs);
    va_end(vargs);
    if(rc){
      throw Exception(rc, "fsl_appendfv() failed: %s",
                      fsl_rc_cstr(rc));
    }
    return *this;
  }

  char const * Buffer::c_str() const throw(){
    return fsl_buffer_cstr(*this);
  }

  void Buffer::toss(int errorCode) const{
    throw Exception(errorCode, "%s", fsl_buffer_cstr(*this));
  }

  int fsl_input_f_std_istream( void * state, void * dest, fsl_size_t * n ){
    std::istream * is = static_cast<std::istream *>(state);
    try{
      /**
         We have to read byte-by-byte to fullfil the requirement that
         we write the number of read bytes to *n. std::istream::read()
         does not give us a (direct) way to know exactly how many
         bytes were read before EOF.
      */
      int rc;
      fsl_size_t i;
      unsigned char * out = (unsigned char *) dest;
      for( i = 0; i < *n; ++i ){
        rc = is->get();
        if(is->eof()){
          *n = i;
          return 0;
        }else if(!is->good()){
          return FSL_RC_IO;
        }else{
          *out++ = rc & 0xFF;
        }
      }
      return 0;
    }catch(...){
      return FSL_RC_IO;
    }
  }

} // namespace fsl
