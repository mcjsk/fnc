/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#include "fossil-scm/fossil.hpp"
#include <cassert>

/* only for debugging */
#include <iostream>
#define CERR std::cerr << __FILE__ << ":" << std::dec << __LINE__ << " : "

namespace fsl {
  Stmt::~Stmt() throw(){
    fsl_stmt_finalize(*this);
  }

  Stmt::Stmt(Db & db) throw() :
    db(db),
    stmt(fsl_stmt_empty){
  }

  Stmt::operator fsl_stmt * () throw(){
    return &this->stmt;
  }

  Stmt::operator fsl_stmt const * () const throw() {
    return &this->stmt;
  }


  void Stmt::assertPrepared() const{
    if(!this->stmt.stmt){
      throw Exception(FSL_RC_MISUSE,
                      "Statement is not prepared.");
    }
    assert(this->stmt.db);
  }

  void Stmt::assertRange(short col, short base) const{
    this->assertPrepared();
    char const * errMsg = NULL;
    if(col<0){
      errMsg = "column/parameter index %d is invalid";
    }else if(base){
      assert(1==base);
      if(!col || col > fsl_stmt_param_count(*this)){
        errMsg = "parameter index %d is out of range";
      }
    }else{
      assert(0==base);
      if(col >= fsl_stmt_col_count(*this)){
        errMsg = "column index %d is out of range";
      }
    }
    if(errMsg) throw Exception(FSL_RC_RANGE, errMsg, col+base);
  }

  void Stmt::propagateError() const{
    this->db.propagateError();
  }

  void Stmt::assertRC(char const * context, int rc) const{
    if(rc){
      this->propagateError();
      throw Exception(rc, "%s: %s", context, fsl_rc_cstr(rc));
    }
  }

  Stmt & Stmt::prepare(std::string const & sql){
    return this->prepare("%s", sql.c_str());
  }

  Stmt & Stmt::prepare(Buffer const & sql){
    return this->prepare("%s", sql.c_str());
  }

  Stmt & Stmt::prepare(char const * sql, ... ){
    va_list vargs;
    if(this->stmt.stmt) throw Exception(FSL_RC_MISUSE,
                                        "Statement is prepared and not "
                                        "yet finalized. Cowardly refusing "
                                        "to re-prepare() non-finalized statement: %s",
                                        this->sql());
    int rc = 0;    
    va_start(vargs,sql);
    rc = fsl_db_preparev( this->db.handle(), *this,
                          sql, vargs );
    va_end(vargs);
    if(rc){
      this->propagateError();
      throw Exception(rc,"SQL preparation failed for: %s", sql);
    }else{
      assert(this->stmt.db == this->db.handle());
      return *this;
    }
  }

  int Stmt::stepCount() const throw(){
    return this->stmt.rowCount;
  }

  int Stmt::paramCount() const throw(){
    return this->stmt.paramCount;
  }

  int Stmt::columnCount() const throw(){
    return this->stmt.colCount;
  }

  char const * Stmt::sql() const throw() {
    return stmt.stmt
      ? fsl_buffer_cstr(&stmt.sql)
      : NULL;
  }

  fsl_stmt * Stmt::handle() throw(){
    return &this->stmt;
  }
  
  fsl_stmt const * Stmt::handle() const throw() {
    return &this->stmt;
  }

  Stmt & Stmt::reset(bool resetStepCounterToo){
    this->assertRC("reset()",
                   fsl_stmt_reset2(*this,
                                   resetStepCounterToo));
    return *this;
  }

  Stmt & Stmt::finalize() throw(){
    fsl_stmt_finalize(*this);
    return *this;
  }

  bool Stmt::step(){
    this->assertPrepared();
    int const rc = fsl_stmt_step(*this);
    switch(rc){
      case FSL_RC_STEP_ROW: return true;
      case FSL_RC_STEP_DONE: return false;
      default:
        this->propagateError();
        throw Exception(rc,"No idea what went wrong.");
    }
  }

  Stmt & Stmt::stepExpectDone(){
    if(this->step()){
      throw Exception(FSL_RC_ERROR,
                      "Expecting statement to return no rows: %s",
                      this->sql());
    }
    return *this;
  }

  int32_t Stmt::getInt32(short col){
    this->assertRange(col, 0);
    return fsl_stmt_g_int32( *this, col );
  }

  int64_t Stmt::getInt64(short col){
    this->assertRange(col, 0);
    return fsl_stmt_g_int64( *this, col );
  }

  double Stmt::getDouble(short col){
    this->assertRange(col, 0);
    return fsl_stmt_g_double( *this, col );
  }

  fsl_id_t Stmt::getId(short col){
    this->assertRange(col, 0);
    return fsl_stmt_g_id( *this, col );
  }

  char const * Stmt::columnName(short col){
    this->assertRange(col, 0);
    return fsl_stmt_col_name(*this, col);
  }

  char const * Stmt::getText(short col, fsl_size_t * length){
    this->assertRange(col, 0);
    return fsl_stmt_g_text(*this, col, length);
  }

  void const * Stmt::getBlob(short col, fsl_size_t * length){
    void const * v = NULL;
    this->assertRange(col, 0);
    fsl_stmt_get_blob(*this, col, &v, length);
    return v;
  }

  Stmt & Stmt::bind(short col){
    this->assertRange(col, 1);
    this->assertRC("bind NULL",
                   fsl_stmt_bind_null(*this, col));
    return *this;
  }
  
  Stmt & Stmt::bind(short col, int32_t v){
    this->assertRange(col, 1);
    this->assertRC("bind int32",
                   fsl_stmt_bind_int32(*this, col, v));
    return *this;
  }

  Stmt & Stmt::bind(short col, int64_t v){
    this->assertRange(col, 1);
    this->assertRC("bind int64",
                   fsl_stmt_bind_int64(*this, col, v));
    return *this;
  }

  Stmt & Stmt::bind(short col, double v){
    this->assertRange(col, 1);
    this->assertRC("bind double",
                   fsl_stmt_bind_double(*this, col, v));
    return *this;
  }

  Stmt & Stmt::bind(short col, char const * str,
                    fsl_int_t len, bool copyBytes){
    this->assertRange(col, 1);
    this->assertRC("bind text",
                   fsl_stmt_bind_text(*this, col, str, len, copyBytes));
    return *this;
  }

  Stmt & Stmt::bind(short col, std::string const & str){
    return this->bind(col, str.c_str(), (fsl_int_t)str.size(), 1);
  }
  
  Stmt & Stmt::bind(short col, void const * v,
                    fsl_size_t len, bool copyBytes){
    this->assertRange(col, 1);
    this->assertRC("bind blob",
                   fsl_stmt_bind_blob(*this, col, v, len, copyBytes));
    return *this;
  }

  int Stmt::paramIndex(char const * name){
    this->assertPrepared();
    return fsl_stmt_param_index(*this, name);
  }

  Stmt & Stmt::bind(char const * col){
    this->assertRC("bind NULL",
                   fsl_stmt_bind_null_name(*this, col));
    return *this;
  }
  
  Stmt & Stmt::bind(char const * col, int32_t v){
    this->assertRC("bind int32",
                   fsl_stmt_bind_int32_name(*this, col, v));
    return *this;
  }

  Stmt & Stmt::bind(char const * col, int64_t v){
    this->assertRC("bind int64",
                   fsl_stmt_bind_int64_name(*this, col, v));
    return *this;
  }

  Stmt & Stmt::bind(char const * col, double v){
    this->assertRC("bind double",
                   fsl_stmt_bind_double_name(*this, col, v));
    return *this;
  }

  Stmt & Stmt::bind(char const * col, char const * str,
                    fsl_int_t len, bool copyBytes){
    this->assertRC("bind text",
                   fsl_stmt_bind_text_name(*this, col, str, len, copyBytes));
    return *this;
  }

  Stmt & Stmt::bind(char const * col, std::string const & str){
    return this->bind(col, str.c_str(), (fsl_int_t)str.size(), 1);
  }
  
  Stmt & Stmt::bind(char const * col, void const * v,
                    fsl_size_t len, bool copyBytes){
    this->assertRC("bind blob",
                   fsl_stmt_bind_blob_name(*this, col, v, len, copyBytes));
    return *this;
  }

  StmtBinder::~StmtBinder(){}

  StmtBinder::StmtBinder(Stmt &s) : st(s), col(0)
  {}

  Stmt & StmtBinder::stmt(){
    return this->st;
  }

  StmtBinder & StmtBinder::operator()(){
    st.bind(++this->col);
    return *this;
  }

  StmtBinder & StmtBinder::operator()(char const * v, fsl_int_t len,
                                      bool copyBytes){
    st.bind(++this->col, v, len, copyBytes);
    return *this;
  }

  StmtBinder & StmtBinder::operator()(void const * v, fsl_size_t len,
                                      bool copyBytes){
    st.bind(++this->col, v, len, copyBytes);
    return *this;
  }

  StmtBinder & StmtBinder::reset(bool alsoStatement) {
    this->col = 0;
    if(alsoStatement) this->st.reset();
    return *this;
  }

  bool StmtBinder::step(){
    return this->st.step();
  }

  StmtBinder & StmtBinder::once(){
    this->st.stepExpectDone();
    return this->reset();
  }

  Db::~Db() throw(){
    this->close();
  }

  void Db::setup(){
    assert(!this->db);
    this->db = fsl_db_malloc();
    if(!this->db) throw OOMException();
    this->ownsDb = true;
  }

  Db::Db(): db(NULL),
            ownsDb(true){
  }

  Db::Db(char const * filename, int openFlags)
    : db(NULL),
      ownsDb(true){
    try{
      this->open(filename, openFlags);
    }catch(...){
      if(this->db) fsl_db_close(this->db);
      throw;
    }
  }

  void Db::propagateError() const{
    if(this->db && this->db->error.code){
      throw Exception(this->db->error);
    }
  }

  void Db::assertRC(char const * context, int rc) const{
    if(rc){
      this->propagateError();
      throw Exception(rc, "%s: %s", context, fsl_rc_cstr(rc));
    }
  }

  void Db::assertOpened() const{
    if(!this->db || !this->db->dbh){
      throw Exception(FSL_RC_MISUSE,
                      "Db is not opened.");
    }
  }

  Db::operator fsl_db * () throw(){
    return this->db;
  }

  Db::operator fsl_db const * () const throw() {
    return this->db;
  }
  
  Db & Db::handle( fsl_db * db, bool ownsHandle )throw(){
    if(this->db != db){
      this->close();
      this->db = db;
      this->ownsDb = ownsHandle;
    }
    return *this;
  }

  Db & Db::close() throw(){
    if(this->db){
      if(this->ownsDb){
        fsl_db_close(*this);
      }
      this->db = NULL;
    }
    return *this;
  }

  char const * Db::filename() throw(){
    return this->db
      ? fsl_db_filename(*this, NULL)
      : NULL;
  }

  Db & Db::open(char const * filename, int openFlags){
    if(!this->db) this->setup();
    else if(this->db->dbh){
      throw Exception(FSL_RC_MISUSE,
                      "Db is already opened: %s",
                      this->filename());
    }
    int const rc = fsl_db_open(*this, filename, openFlags);
    if(rc){
      /*
         The problem here is that open() can be called from the ctor,
         and if the ctor throws then the dtor is not called, so we
         have to free up this->db. Oh, wait...  the open() ctor does
         that for us, so this gets easier.

         But... if it throws from outside the ctor then we DO have to
         clean up.
       */
      Exception const & ex = this->db->error.code
        ? Exception(this->db->error)
        : Exception(rc,"fsl_db_open(%s) failed: %s",
                    filename, fsl_rc_cstr(rc));
      this->close();
      throw ex;
    }
    return *this;
  }

  bool Db::isOpened() const throw(){
    return (this->db && this->db->dbh) ? true : false;
  }

  bool Db::ownsHandle() const throw(){
    return this->ownsDb;
  }

  fsl_db * Db::handle() throw(){
    return this->db;
  }
  
  fsl_db const * Db::handle() const throw() {
    return this->db;
  }

  Db & Db::begin(){
    this->assertOpened();
    this->assertRC( 
                   "begin()",
                    fsl_db_transaction_begin(*this) );
    return *this;
  }

  Db & Db::commit(){
    this->assertOpened();
    this->assertRC( "commit()",
                    fsl_db_transaction_end(*this, 0) );
    return *this;
  }

  Db & Db::rollback() throw() {
    this->assertOpened();
    fsl_db_transaction_end(*this, 1);
    return *this;
  }

  Db & Db::exec(std::string const & sql){
    return this->exec("%s", sql.c_str());
  }

  Db & Db::exec(char const * sql, ...){
    this->assertOpened();
    va_list vargs;
    int rc = 0;    
    va_start(vargs,sql);
    rc = fsl_db_execv( *this, sql, vargs );
    va_end(vargs);
    if(rc){
      this->propagateError();
      throw Exception(rc,"SQL execution failed for: %s", sql);
    }
    else return *this;
  }

  Db & Db::execMulti(char const * sql, ...){
    this->assertOpened();
    va_list vargs;
    int rc = 0;    
    va_start(vargs,sql);
    rc = fsl_db_exec_multiv( *this, sql, vargs );
    va_end(vargs);
    if(rc){
      this->propagateError();
      throw Exception(rc,"SQL multi-exec failed for: %s", sql);
    }
    else return *this;
  }

  Db & Db::execMulti(std::string const & sql){
    return this->execMulti("%s", sql.c_str());
  }


  Db & Db::attach(char const * filename, char const * label){
    this->assertRC( "attach()", fsl_db_attach(*this, filename, label) );
    return *this;
  }

  Db & Db::detach(char const * label){
    this->assertRC( "detach()", fsl_db_detach(*this, label) );
    return *this;
  }

  int Db::transactionLevel() const throw(){
    return this->db
      ? this->db->beginCount
      : 0;
  }

  Db::Transaction::Transaction(Db & db)
  : db(db), inTrans(false){
    db.begin();
    inTrans = db.transactionLevel();
  }

  Db::Transaction::~Transaction() throw(){
    if(inTrans) this->rollback();
  }

  void Db::Transaction::commit(){
    assert(inTrans);
    if(inTrans>0){
      inTrans = 0;
      db.commit();
    }
  }

  void Db::Transaction::rollback() throw(){
    assert(inTrans);
    if(inTrans){
#if 1
      inTrans = 0;
      db.rollback();
#else
      /* sane? */
      while(inTrans < db.transactionLevel()){
        db.rollback();
      }
#endif
    }
  }

  int Db::Transaction::level() const throw(){
    return db.transactionLevel();
  }

}// namespace fsl

#undef CERR
