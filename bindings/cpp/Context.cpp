/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#include "fossil-scm/fossil.hpp"
#include <cassert>

/* only for debugging */
#include <iostream>
#define CERR std::cerr << __FILE__ << ":" << std::dec << __LINE__ << " : "

namespace fsl {

  Context::~Context(){
    if(this->ownsCx){
      fsl_cx_finalize(this->f);
    }
  }

  void Context::setup(fsl_cx_init_opt const * opt){
    //this->f = fsl_cx_malloc();
    //if(!this->f) throw OOMException();
    assert(!this->f);
    int const rc = fsl_cx_init(&this->f, opt);
    if(rc){
      fsl_error err = fsl_error_empty;
      if(this->f){
        fsl_error_move( &this->f->error, &err );
        fsl_cx_finalize(this->f);
        this->f = NULL;
      }else{
        fsl_error_set( &err, rc,
                       "fsl_cx_init() failed with code %s",
                       fsl_rc_cstr(rc) );
      }
      throw Exception(err);
    }
  }

  Context::Context()
    : f(NULL),
      ownsCx(true),
      dbCkout(),
      dbRe(),
      dbMain()
  {
    this->setup(NULL);
  }
  
  Context::Context(fsl_cx_init_opt const & opt)
    : f(NULL),
      ownsCx(true),
      dbCkout(),
      dbRe(),
      dbMain()
  {
    this->setup(&opt);
  }

  Context::Context(fsl_cx * f, bool ownsHandle)
    : f(f),
      ownsCx(ownsHandle),
      dbCkout(),
      dbRe(),
      dbMain()
  {
    
  }

  Context::operator fsl_cx * () throw(){
    return this->f;
  }

  Context::operator fsl_cx const * () const throw() {
    return this->f;
  }

  void Context::propagateError() const{
    if(this->f){
      fsl_error const * err = fsl_cx_err_get_e(this->f);
      if(err->code) throw Exception(err);
    }
  }

  void Context::assertRC(char const * context, int rc) const{
    if(rc){
      this->propagateError();
      throw Exception(rc, "%s: %s", context, fsl_rc_cstr(rc));
    }
  }

  void Context::assertHasRepo(){
    assert(this->f);
    if(!fsl_needs_repo(this->f)){
      fsl_error const * err = fsl_cx_err_get_e(this->f);
      assert(err->code);
      throw Exception(err);
    }
  }

  void Context::assertHasCheckout(){
    assert(this->f);
    if(!fsl_needs_ckout(this->f)){
      fsl_error const * err = fsl_cx_err_get_e(this->f);
      assert(err->code);
      throw Exception(err);
    }
  }

  fsl_cx * Context::handle() throw(){
    return this->f;
  }

  fsl_cx const * Context::handle() const throw(){
    return this->f;
  }

  Db & Context::db() throw() {
    if(!this->dbMain.handle()){
      fsl_db * db = fsl_cx_db(*this);
      if(db) this->dbMain.handle(db, false);
    }
    return this->dbMain;
  }


  Db & Context::dbRepo() throw() {
    if(!this->dbRe.handle()){
      fsl_db * db = fsl_cx_db_repo(*this);
      if(db) this->dbRe.handle(db, false);
    }
    return this->dbRe;
  }

  Db & Context::dbCheckout() throw() {
    if(!this->dbCkout.handle()){
      fsl_db * db = fsl_cx_db_ckout(*this);
      if(db) this->dbCkout.handle(db, false);
    }
    return this->dbCkout;
  }

  Context & Context::openCheckout( char const * dirName ){
    this->assertRC( "openCheckout()",
                    fsl_ckout_open_dir(this->f, dirName, true) );
    return *this;
  }

  Context & Context::openRepo( char const * dbName ){
    this->assertRC( "openRepo()",
                    fsl_repo_open(this->f, dbName) );
    return *this;
  }

  Context & Context::closeDbs() throw(){
    fsl_cx_close_dbs(this->f)
      /*
        Reminder to self: the 3 fsl_cx db handles are stored as
        complete fsl_db instances (not pointers) in fsl_cx, with the
        exception of the "main" db, which is just a pointer to one of
        the other 3. What does that mean? It means that when we use
        fsl_cx_close_dbs(), this->dbRe and friends will (if
        initialized) still be pointing to those pointers...  which are
        (due to internal details) actually still valid, they just
        refer to closed fsl_db handles.

        That's actually good for us here, except that certain
        combinations of C-level ops "might" get our checkout/repo db
        pointers cross a bit.
      */
      ;
    assert(!this->dbRe.ownsHandle());
    assert(!this->dbMain.ownsHandle());
    assert(!this->dbCkout.ownsHandle());
    this->dbRe.close();
    this->dbCkout.close();
    this->dbMain.close();
    if(this->dbRe.handle()){
      assert(!this->dbRe.handle()->dbh);
    }
    return *this;
  }

  bool Context::ownsHandle() const throw(){
    return this->ownsCx;
  }

  std::string Context::ridToArtifactUuid(fsl_id_t rid,
                                         fsl_satype_e type){
    this->assertHasRepo();
    fsl_uuid_str uuid = fsl_rid_to_artifact_uuid(*this, rid, type);
    if(!uuid){
      this->propagateError();
      throw Exception(FSL_RC_NOT_FOUND,
                      "Could not resolve RID %" FSL_ID_T_PFMT
                      " as artifact type %s.",
                      (fsl_id_t)rid, fsl_satype_cstr(type));
    }
    std::string const & rc = uuid;
    fsl_free(uuid);
    return rc;
  }

  std::string Context::ridToUuid(fsl_id_t rid){
    this->assertHasRepo();
    fsl_uuid_str uuid = fsl_rid_to_uuid(*this, rid);
    if(!uuid){
      this->propagateError();
      throw Exception(FSL_RC_NOT_FOUND, "Could not resolve RID %" FSL_ID_T_PFMT ".",
                      (fsl_id_t)rid);
    }
    std::string const & rc = uuid;
    fsl_free(uuid);
    return rc;
  }

  std::string Context::symToUuid(char const * symbolicName,
                                 fsl_id_t * rid,
                                 fsl_satype_e type){
    this->assertHasRepo();
    fsl_uuid_str uuid = NULL;
    int const rc = fsl_sym_to_uuid(*this, symbolicName, type, &uuid, rid);
    if(rc){
      this->propagateError();
      throw Exception(rc);
    }
    std::string const & rv = uuid;
    fsl_free(uuid);
    return rv;
  }


  fsl_id_t Context::symToRid(char const * symbolicName, fsl_satype_e type){
    this->assertHasRepo();
    fsl_id_t rv = 0;
    int const rc = fsl_sym_to_rid(*this, symbolicName, type, &rv);
    if(rc){
      this->propagateError();
      throw Exception(rc);
    }
    assert(rv>0);
    return rv;
  }

  fsl_id_t Context::symToRid(std::string const & symbolicName,
                             fsl_satype_e type){
    return this->symToRid( symbolicName.c_str(), type );
  }

  Context::Transaction::Transaction(Context &cx)
    : tr( cx.db() ),
      level(tr.level()){
  }

  Context::Transaction::~Transaction() throw(){
    if(this->level) this->tr.rollback();
  }

  void Context::Transaction::commit(){
    if(this->level){
      this->level = 0;
      this->tr.commit();
    }else{
      throw Exception(FSL_RC_MISUSE,
                      "commit() called multiple times.");
    }
  }

  Context & Context::getContent( fsl_id_t rid, Buffer & dest ){
    int const rc = fsl_content_get( *this, rid, dest );
    if(rc){
      this->propagateError();
      throw Exception(rc);
    }
    return *this;
  }

  Context & Context::getContent( char const * sym, Buffer & dest ){
    int const rc = fsl_content_get_sym( *this, sym, dest );
    if(rc){
      this->propagateError();
      throw Exception(rc);
    }
    return *this;
  }

  Context & Context::getContent( std::string const & sym, Buffer & dest ){
    return this->getContent( sym.c_str(), dest );
  }

} // namespace fsl

#undef CERR
