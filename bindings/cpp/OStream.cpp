/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#include "fossil-scm/fossil.hpp"

/**

 */
namespace fsl {


  fsl_int_t fsl_appendf_f_std_ostream( void * state,
                                       char const * data,
                                       fsl_int_t n ){
    std::ostream * os = static_cast<std::ostream *>(state);
    try{
      os->write( data, (std::streamsize)n );
      return (*os) ? 0 : FSL_RC_IO;
    }catch(...){
      return FSL_RC_IO;
    }
  }

  int fsl_output_f_std_ostream( void * state,
                                void const * data,
                                fsl_size_t n ){
    std::ostream * os = static_cast<std::ostream *>(state);
    try{
      os->write( (char const *)data, (std::streamsize)n );
      return (*os) ? 0 : FSL_RC_IO;
    }catch(...){
      return FSL_RC_IO;
    }
  }


  ContextOStreamBuf::~ContextOStreamBuf() throw(){
    this->sync();
    if( this->m_os ){
      this->m_os->rdbuf( this->m_old );
    }
  }

  void ContextOStreamBuf::setup( fsl_cx * fx ){
    if(!fx) throw Exception(FSL_RC_MISUSE,
                            "ContextOStreamBuf requires "
                            "a non-NULL fsl_cx.");
    this->f = fx;
    this->setp( 0, 0 );
    this->setg( 0, 0, 0 );
    if(m_os) m_os->rdbuf( this );
  }

  ContextOStreamBuf::ContextOStreamBuf( fsl_cx * f, std::ostream & os )
    : f(NULL),
      m_os(&os),
      m_old(os.rdbuf()){
    this->setup(f);
  }

  ContextOStreamBuf::ContextOStreamBuf( fsl_cx * f )
    : f(NULL),
      m_os(NULL),
      m_old(NULL){
    this->setup(f);
  }

  ContextOStreamBuf::ContextOStreamBuf( Context & cx, std::ostream & os )
    : f(NULL),
      m_os(&os),
      m_old(os.rdbuf()){
    this->setup(cx.handle());
  }

  ContextOStreamBuf::ContextOStreamBuf( Context & cx )
    : f(NULL),
      m_os(NULL),
      m_old(NULL){
    this->setup(cx.handle());
  }


  int ContextOStreamBuf::overflow( int c ){
    char const ch = c & 0xFF;
    int const rc = fsl_output(this->f, &ch, 1);
    if(rc) throw Exception(FSL_RC_IO,
                           "fsl_output() failed with code %d (%s)",
                           rc, fsl_rc_cstr(rc));
    else return 0;
  }

  int ContextOStreamBuf::sync(){
    return this->f
      ? fsl_flush(this->f)
      : FSL_RC_IO;
  }

  ContextOStream::~ContextOStream() throw(){
    delete this->sb;
  }

  ContextOStream::ContextOStream( fsl_cx * f )
    :f(f),
     sb(new ContextOStreamBuf(f, *this)){
  }

  ContextOStream::ContextOStream( Context & cx )
    :f(cx.handle()),
     sb(new ContextOStreamBuf(f, *this)){
  }

  ContextOStream & ContextOStream::appendf(char const * fmt, ...){
    va_list vargs;
    int rc = 0;
    va_start(vargs,fmt);
    rc = fsl_outputfv(this->f, fmt, vargs);
    va_end(vargs);
    if(rc){
      throw Exception(rc, "fsl_outputfv() failed: %s",
                      fsl_rc_cstr(rc));
    }
    return *this;
  }

  FslOutputFStreamBuf::~FslOutputFStreamBuf() throw(){
    if( this->m_os ){
      this->m_os->rdbuf( this->m_old );
    }
  }

  void FslOutputFStreamBuf::setup( fsl_output_f f, void * state ){
    if(!f) throw Exception(FSL_RC_MISUSE,
                           "FslOutputFStream output function may "
                           "not be NULL.");
    this->out = f;
    this->outState = state;
    this->setp( 0, 0 );
    this->setg( 0, 0, 0 );
    if(m_os) m_os->rdbuf( this );
  }

  FslOutputFStreamBuf::FslOutputFStreamBuf( fsl_output_f f, void * state )
    : out(NULL), outState(NULL),
      m_os(NULL), m_old(NULL)
  {
    this->setup(f, state);
  }

  FslOutputFStreamBuf::FslOutputFStreamBuf( fsl_output_f f, void * state,
                                            std::ostream & os)
    : out(NULL), outState(NULL),
      m_os(&os), m_old(os.rdbuf())
  {
    this->setup(f, state);
  }

  int FslOutputFStreamBuf::overflow( int c ){
    char const ch = c & 0xFF;
    int const rc = this->out( this->outState, &ch, 1);
    if(rc) throw Exception(FSL_RC_IO,
                           "fsl_output_f() proxy failed with code "
                           "%d (%s)", rc, fsl_rc_cstr(rc));
    else return 0;
  }

  int FslOutputFStreamBuf::sync(){
    return 0;
  }

  FslOutputFStream::~FslOutputFStream() throw(){
    delete this->sb;
  }

  FslOutputFStream::FslOutputFStream( fsl_output_f out, void * outState )
    : sb(new FslOutputFStreamBuf(out, outState, *this)){
  }

  fsl_int_t FslOutputFStream::fslAppendfF( void * state,
                                           char const * s, fsl_int_t n ){
    FslOutputFStream * out = static_cast<FslOutputFStream*>(state);
    if(out->good()){
      out->write( s, (std::streamsize)n );
      return out->good() ? 0 : -1;
    }else return -1;
  }

  FslOutputFStream & FslOutputFStream::appendf(char const * fmt, ...){
    va_list vargs;
    fsl_int_t rc = 0;
    va_start(vargs,fmt);
    rc = fsl_appendfv(FslOutputFStream::fslAppendfF, this, fmt, vargs);
    va_end(vargs);
    if(rc<0) throw Exception(FSL_RC_IO,
                             "fsl_appendfv() failed mysteriously.");
    return *this;
  }

  BufferOStream::BufferOStream(fsl_buffer * b) :
    FslOutputFStream(fsl_output_f_buffer, b){
    if(!b) throw Exception(FSL_RC_MISUSE,
                           "fsl_buffer argument may not be NULL.");
  }

  BufferOStream::~BufferOStream() throw(){
  }

} // namespace fsl
