/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#include "fossil-scm/fossil.hpp"
#include <cassert>

/* only for debugging */
#include <iostream>
#define CERR std::cerr << __FILE__ << ":" << std::dec << __LINE__ << " : "

namespace fsl {

  Deck::~Deck() throw(){
    delete this->deltaBase;
    if(this->ownsDeck){
      assert(this->d->allocStamp);
      fsl_deck_finalize(this->d);
    }
  }

  void Deck::setup(fsl_deck * mf, fsl_satype_e type){
    if(!mf){
      assert(!this->d);
      assert(this->ownsDeck);
      this->d = fsl_deck_malloc();
      if(!this->d) throw OOMException();
    }else{
      assert(this->d == mf);
      if(d && d->f && (d->f != this->cx.handle())){
        throw Exception(FSL_RC_MISUSE,
                        "Mis-matched fsl_cx contexts for deck.");
      }
    }
    if(ownsDeck){
      fsl_deck_init( this->cx, d, type );
    }
  }

  Deck::Deck(Context & cx, fsl_satype_e type)
    : cx(cx),
      d(NULL),
      deltaBase(NULL),
      ownsDeck(true){
    this->setup( NULL, type );
  }

  Deck::Deck(Context & cx, fsl_deck * d, bool ownsDeck)
    : cx(cx),
      d(d),
      deltaBase(NULL),
      ownsDeck(ownsDeck){
    if(!d){
      throw Exception(FSL_RC_MISUSE,
                      "A proxied Deck may not be NULL.");
    }
    this->setup( d, d->type );
  }

  void Deck::propagateError() const{
    fsl_error const * err = fsl_cx_err_get_e(this->cx);
    if(err->code){
      throw Exception(err);
    }
  }

  void Deck::assertRC(char const * context, int rc) const{
    if(rc){
      this->propagateError();
      throw Exception(rc, "%s: %s", context, fsl_rc_cstr(rc));
    }
  }

  Deck & Deck::cleanup() throw(){
    fsl_deck_clean(*this);
    return *this;
  }

  fsl_satype_e Deck::type() const throw(){
    return this->d->type;
  }

  fsl_id_t Deck::rid() const throw(){
    return this->d->rid;
  }

  fsl_uuid_cstr Deck::uuid() const throw(){
    return this->d->uuid;
  }

  Deck::operator fsl_deck *() throw(){
    return this->d;
  }

  Deck::operator fsl_deck const *() const throw(){
    return this->d;
  }

  Context & Deck::context() throw() { return this->cx; }
  Context const & Deck::context() const throw() { return this->cx; }

  fsl_deck * Deck::handle() throw(){
    return this->d;
  }

  fsl_deck const * Deck::handle() const throw(){
    return this->d;
  }

  bool Deck::hasAllRequiredCards() const throw(){
    return fsl_deck_has_required_cards(*this);
  }

  Deck const & Deck::assertHasRequiredCards() const{
    if(!fsl_deck_has_required_cards(*this)){
      throw Exception(fsl_cx_err_get_e(this->cx));
    }
    else return *this;
  }

  bool Deck::cardIsLegal(char cardLetter) const throw(){
    return fsl_card_is_legal( this->d->type, cardLetter );
  }

  Deck & Deck::unshuffle(bool calcRCard){
    int const rc = fsl_deck_unshuffle(*this, calcRCard);
    if(rc){
      this->propagateError();
      throw Exception(rc);
    }else return *this;
  }

  Deck const & Deck::output( fsl_output_f f, void * outState ) const{
    int const rc = fsl_deck_output( this->d, f, outState );
    if(rc) throw Exception(this->d->f->error);
    else return *this;
  }

  Deck const & Deck::output( std::ostream & os ) const{
    return this->output( fsl_output_f_std_ostream, &os );
  }

  Deck & Deck::save(bool isPrivate){
    int const rc = fsl_deck_save( *this, isPrivate );
    if(rc){
      this->propagateError();
      throw Exception(rc,"fsl_deck_save() failed: %s",
                      fsl_rc_cstr(rc));
    }
    else return *this;
  }

  Deck & Deck::load( fsl_id_t rid, fsl_satype_e type ){
    this->cleanup();
    int const rc = fsl_deck_load_rid( this->cx, *this,
                                      rid, type );
    if(rc){
      this->propagateError();
      throw Exception(rc, "fsl_deck_load_rid() failed "
                      "with %s for symbol: %" FSL_ID_T_PFMT,
                      fsl_rc_cstr(rc), (fsl_id_t)rid);
    }else return *this;
  }

  Deck & Deck::load( char const * symbolicName, fsl_satype_e type){
    this->cleanup();
    int const rc = fsl_deck_load_sym( this->cx, *this,
                                      symbolicName, type );
    if(rc){
      this->propagateError();
      throw Exception(rc, "fsl_deck_load_sym() failed "
                      "with %s for symbol: %s",
                      fsl_rc_cstr(rc), symbolicName);
    }else return *this;
  }

  Deck & Deck::load( std::string const & symbolicName, fsl_satype_e type){
    return this->load( symbolicName.c_str(), type );
  }

  Deck & Deck::setCardA( char const * name,
                         char const * tgt,
                         fsl_uuid_cstr uuid ){
    int const rc = fsl_deck_A_set(*this, name, tgt, uuid);
    this->assertRC("fsl_deck_A_set()", rc);
    return *this;
  }

  Deck & Deck::setCardB(fsl_uuid_cstr uuid){
    if(this->deltaBase){
      delete this->deltaBase;
      this->deltaBase = NULL;
    }
    int const rc = fsl_deck_B_set(*this, uuid);
    this->assertRC("fsl_deck_B_add()", rc);
    return *this;
  }


  Deck & Deck::setCardC( char const * comment ){
    int const rc = fsl_deck_C_set(*this, comment, -1);
    this->assertRC("fsl_deck_C_set()", rc);
    return *this;
  }

  Deck & Deck::setCardD(double julianDay){
    int const rc = fsl_deck_D_set(*this,
                                  julianDay<0
                                  ? fsl_julian_now()
                                  : julianDay);
    this->assertRC("fsl_deck_D_set()", rc);
    return *this;
  }

  Deck & Deck::setCardE( fsl_uuid_cstr uuid, double julian ){
    int const rc = fsl_deck_E_set(*this,
                                  julian<0
                                  ? fsl_julian_now()
                                  : julian,
                                  uuid );
    this->assertRC("fsl_deck_E_set()", rc);
    return *this;
  }


  Deck & Deck::addCardF(char const * name,
                        fsl_uuid_cstr uuid,
                        fsl_fileperm_e perm, 
                        char const * oldName ){
    int const rc = fsl_deck_F_add(*this, name, uuid, perm, oldName);
    this->assertRC("fsl_deck_F_add()", rc);
    return *this;
  }

  Deck & Deck::addCardJ( char isAppend, char const * key, char const * value ){
    int const rc = fsl_deck_J_add(*this, isAppend, key, value);
    this->assertRC("fsl_deck_J_add()", rc);
    return *this;
  }


  Deck & Deck::setCardK(fsl_uuid_cstr uuid){
    int const rc = fsl_deck_K_set(*this, uuid);
    this->assertRC("fsl_deck_K_set()", rc);
    return *this;
  }

  Deck & Deck::setCardL( char const * title ){
    int const rc = fsl_deck_L_set(*this, title, -1);
    this->assertRC("fsl_deck_L_set()", rc);
    return *this;
  }

  Deck & Deck::addCardM(fsl_uuid_cstr uuid){
    int const rc = fsl_deck_M_add(*this, uuid);
    this->assertRC("fsl_deck_M_add()", rc);
    return *this;
  }


  Deck & Deck::setCardN(char const * name){
    int const rc = fsl_deck_N_set(*this, name, -1);
    this->assertRC("fsl_deck_N_set()", rc);
    return *this;
  }

  Deck & Deck::addCardP(fsl_uuid_cstr uuid){
    int const rc = fsl_deck_P_add(*this, uuid);
    this->assertRC("fsl_deck_P_add()", rc);
    return *this;
  }

  Deck & Deck::addCardQ(char type, fsl_uuid_cstr target,
                        fsl_uuid_cstr baseline){
    int const rc = fsl_deck_Q_add(*this, type, target, baseline);
    this->assertRC("fsl_deck_Q_add()", rc);
    return *this;
  }

  Deck & Deck::addCardT(fsl_tagtype_e tagType,
                        char const * name,
                        fsl_uuid_cstr uuid,
                        char const * value){
    int const rc = fsl_deck_T_add( *this, tagType, uuid, name, value );
    this->assertRC("fsl_deck_T_add()", rc);
    return *this;
  }


  Deck & Deck::setCardU(char const * user){
    if(!user || !*user){
      user = fsl_cx_user_get(this->cx);
    }
    if(!user || !*user){
      throw Exception(FSL_RC_MISUSE,
                      "setCardU(): NULL/empty user name is not legal.");
    }
    int const rc = fsl_deck_U_set(*this, user);
    this->assertRC("fsl_deck_U_set()", rc);
    return *this;
  }

  Deck & Deck::setCardW(char const * content, fsl_int_t len){
    int const rc = fsl_deck_W_set(*this, content, len);
    this->assertRC("fsl_deck_W_set()", rc);
    return *this;
  }

  Deck::FCardIterator::~FCardIterator() throw()
  {}

  Deck::FCardIterator::FCardIterator(Deck & d, bool skipDeletedFiles)
    : d(&d),
      fc(NULL),
      skipDeleted(skipDeletedFiles)
  {
    int const rc = fsl_deck_F_rewind(d);
    if(rc) throw Exception(rc, "fsl_deck_F_rewind() failed: %s",
                           fsl_rc_cstr(rc));
    fsl_deck_F_next(d, &this->fc);
  }

  Deck::FCardIterator::FCardIterator() throw()
    : d(NULL),
      fc(NULL),
      skipDeleted(false)
  {}

  void Deck::FCardIterator::assertHasDeck(){
    if(!this->d) throw Exception(FSL_RC_MISUSE,
                                 "Iterator requires a deck object.");
  }

  Deck::FCardIterator & Deck::FCardIterator::operator++(){
    if(this->fc){
      do{
        int const rc = fsl_deck_F_next(*this->d, &this->fc);
        if(rc) throw Exception(rc);
      }while(skipDeleted && (this->fc && !this->fc->uuid));
    }
    return *this;
  }

  fsl_card_F const * Deck::FCardIterator::operator*(){
    return this->fc;
  }

  fsl_card_F const * Deck::FCardIterator::operator->(){
    if(!this->fc) throw Exception(FSL_RC_MISUSE,
                                  "Throwing to avoid "
                                  "dereferencing a NULL fsl_card_F.");
    return this->fc;
  }


  bool Deck::FCardIterator::operator==(FCardIterator const &rhs) const throw(){
    if(this->fc==rhs.fc) return true;
    else if(!this->fc || !rhs.fc) return false;
    return 0==fsl_strcmp(this->fc->name, rhs.fc->name);
  }
    
  bool Deck::FCardIterator::operator!=(FCardIterator const &rhs) const throw(){
    if(this->fc==rhs.fc) return false;
    else if(!this->fc || !rhs.fc) return true;
    return 0!=fsl_strcmp(this->fc->name, rhs.fc->name);
  }

  bool Deck::FCardIterator::operator<(FCardIterator const &rhs) const throw(){
    if(!this->fc) return rhs.fc ? true : false;
    else if(!rhs.fc) return false;
    return 0 > fsl_strcmp(this->fc->name, rhs.fc->name);
  }

  Deck * Deck::baseline(){
    if(FSL_SATYPE_CHECKIN==this->d->type){
      if(this->deltaBase) return this->deltaBase;
      else if(!this->d->B.uuid) return NULL;
      else if(!this->d->B.baseline){
        int const rc = fsl_deck_F_rewind(*this);
        this->assertRC("fsl_deck_rewind()", rc);
        assert(this->d->B.baseline);
      }
      return this->deltaBase = new Deck(this->cx,
                                        this->d->B.baseline,
                                        false);
    }else{
      return NULL;
    }
  }

  Deck::TCardIterator::TCardIterator(Deck & d)
  : ParentType(d.handle()->T)
  {}

  Deck::TCardIterator::TCardIterator()
  : ParentType()
  {}

  Deck::TCardIterator::~TCardIterator() throw()
  {}

  fsl_card_T const * Deck::TCardIterator::operator->() const{
    fsl_card_T const * rv = this->currentValue();
    if( !rv ) throw Exception(FSL_RC_MISUSE,
                              "Throwing to avoid dereferencing a NULL "
                              "fsl_card_T.");
    else return rv;
  }
  std::ostream & operator<<( std::ostream & os, Deck const & d ){
    d.output(os);
    return os;
  }

} // namespace fsl

#undef CERR
