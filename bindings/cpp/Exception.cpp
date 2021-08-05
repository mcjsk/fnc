/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#include "fossil-scm/fossil.hpp"
#include <cassert>

/* only for debugging */
#include <iostream>
#define CERR std::cerr << __FILE__ << ":" << std::dec << __LINE__ << " : "

namespace fsl {

  Exception::~Exception() throw() {
    fsl_error_clear(*this);
  }

  Exception::Exception(Exception const &other) throw()
    : err(fsl_error_empty){
    fsl_error_copy(&other.err, *this);
    //CERR << "Exception("<<err.code<<"): " << (char const *)err.msg.mem << '\n';
  }

  Exception & Exception::operator=(Exception const &other) throw(){
    if(&other != this){
      fsl_error_clear(*this);
      fsl_error_copy(&other.err, *this);
    }
    //CERR << "Exception("<<err.code<<"): " << (char const *)err.msg.mem << '\n';
    return *this;
  }

  Exception::Exception(int code, std::string const & msg) throw()
    : err(fsl_error_empty){
    fsl_error_set(*this, code, "%s",
                  msg.empty()
                  ? fsl_rc_cstr(code)
                  : msg.c_str());
    //CERR << "Exception("<<err.code<<"): " << (char const *)err.msg.mem << '\n';
    /* Reminder: we have to ignore any error here :/ */
  }

  Exception::Exception(int code) throw()
    : err(fsl_error_empty){
    fsl_error_set(*this, code, "%s", fsl_rc_cstr(code));
  }

  Exception::Exception() throw()
    : err(fsl_error_empty){
    fsl_error_set(*this, FSL_RC_ERROR, NULL);
  }

  Exception::Exception( fsl_error & err ) throw()
    : err(fsl_error_empty)
  {
    assert(err.code);
    fsl_error_move( &err, *this );
  }

  Exception::Exception( fsl_error const * err ) throw()
    : err(fsl_error_empty)
  {
    if(err && err->code){
      fsl_error_copy( err, *this );
    }else{
      fsl_error_set( *this, FSL_RC_MISUSE,
                     "Exception(fsl_error const *) ctor passed a %s!",
                     err ? "fsl_error with code==0" : "NULL");
    }
  }

  void Exception::error(int code, char const * fmt, va_list vargs) throw(){
    fsl_error_setv(*this, code, fmt, vargs);
    //CERR << "Exception("<<err.code<<"): " << (char const *)err.msg.mem << '\n';
  }

  Exception::Exception(int code, char const * fmt, ...) throw()
    : err(fsl_error_empty){
    va_list vargs;
    va_start(vargs,fmt);
    this->error(code, fmt, vargs);
    va_end(vargs);
  }

  Exception::Exception(int code, char const * fmt, va_list vargs) throw()
    : err(fsl_error_empty){
    this->error(code, fmt, vargs);
  }

  Exception::operator fsl_error * () throw(){
    return &this->err;
  }

  Exception::operator fsl_error const * () const throw(){
    return &this->err;
  }

  char const * Exception::messsage() const throw(){
    return this->what();
  }

  char const * Exception::what() const throw() {
    return (FSL_RC_OOM==this->err.code)
      ? fsl_rc_cstr(this->err.code)
      : fsl_buffer_cstr(&this->err.msg);
  }

  char const * Exception::codeString() const throw(){
    return fsl_rc_cstr(this->err.code);
  }

  int Exception::code() const throw(){
    return this->err.code;
  }

  OOMException::OOMException() throw()
    : Exception(FSL_RC_OOM)
  {
  }

} // namespace fsl

#undef CERR
