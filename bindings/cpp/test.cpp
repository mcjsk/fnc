/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#include "fossil-scm/fossil.hpp" /* MUST come first b/c of config bits. */
#include <cassert>
#include <iostream>
#include <fstream>
#include <list>
#include <cstdlib> /* EXIT_SUCCESS, EXIT_FAILURE */

#define CERR std::cerr << __FILE__ << ":" << std::dec << __LINE__ << " : "
#define COUT std::cout << __FILE__ << ":" << std::dec << __LINE__ << " : "

#define RCStr(C) fsl_rc_cstr(C)

/**
   A functor for use with fsl::Stmt::eachRow().
*/
struct RowDumper {
  std::ostream & os;
  char const * separator;
  bool showHeader;
  explicit RowDumper(std::ostream & os = std::cout)
    : os(os),
      separator("\t"),
      showHeader(true)
  {}
  void operator()(fsl::Stmt &s) const{
    int i;
    int const n = s.columnCount();
    if(this->showHeader && (1==s.stepCount())){
      for(i = 0; i < n; ++i ){
        this->os << (i ? this->separator : "") << s.columnName(i);
      }
      this->os << '\n';
    }
    for(i = 0; i < n; ++i ){
      this->os << (i ? this->separator : "") << s.getText(i);
    }
    this->os << '\n';
  }
};

static void test_db_1(){
  namespace f = fsl;
  char const * dbName = 0
    ? "/fail"
    : ":memory:"
    ;
  //using fsl::Exception;
  //throw Exception(FSL_RC_NYI, "Just %s", "testing");
  f::Db db;
  f::Stmt st(db);
  db.open(dbName, FSL_OPEN_F_RWC);
  COUT << "Opened db: "<<db.filename()<<'\n';
  db.begin();
  st.prepare("CREATE TABLE t(a INTEGER,b TEXT)").stepExpectDone();
  
  st.finalize().prepare("INSERT INTO t(a,b) VALUES(?1,?2)");
  // Try out the StmtBinder for insertions...
  f::StmtBinder b(st);
  b
    (1)("once").insert()
    (2)("twice").insert()
    (3.3)("Quite possibly thrice-point-thrice")
    .insert();

  int threw = 0;
  try{
    b(7)(8)
      ("must throw - too many bind() values");
  }catch(f::Exception const & ex){
    threw = ex.code();
  }
  assert(FSL_RC_RANGE==threw);

  // Bind a std::list of parameters... (not terribly useful, probably)
  typedef std::list<std::string> LI;
  LI li;
  li.push_back("4");
  li.push_back("fource. Or fice.");
  b.bindList(li).insert();

  st.finalize()
    .prepare("SELECT rowid, a, b FROM t ORDER BY a")
    .eachRow( RowDumper() )
    .finalize();

  f::Buffer sb;
  (sb << "SELECT ").appendf("%Q", "percent 'Q'");
  st.prepare(sb)
    .eachRow( RowDumper() );

}

static void test_stream_1(fsl::Context & cx){
  namespace f = fsl;

  f::ContextOStream fout(cx);
  {
    char const * sym = "rid:1";
    fout << "UUID of ["<<sym<<"]: " << cx.symToUuid(sym)<<'\n';
  }

  {
    fsl_uuid_cstr uuid = NULL;
    fsl_id_t rid = 0;
    fsl_ckout_version_info(cx, &rid, &uuid);
    fout << "Checkout version: "<<rid << " ==> "<<uuid<<'\n';
  }

  char const * filename = 1
    ? __FILE__
    : "/fail"
    ;
  std::ifstream is(filename);
  if(!is.good()){
    throw f::Exception(FSL_RC_IO,"Cannot open input file: %s",
                       filename);
  }
  f::Buffer buf;
  int rc = fsl_buffer_fill_from( buf,
                                 f::fsl_input_f_std_istream,
                                 &is );
  if(rc){
    throw f::Exception(rc,"Error (%d) %s reading from file: %s",
                       rc, fsl_rc_cstr(rc), filename);
  }
  assert(buf.used());
  COUT << "Read "<< buf.used()
       <<" bytes from "<<filename
       <<" via std::istream proxy.\n";
  buf.reset();
  assert(0==buf.used());
  assert(0<buf.capacity());
  buf << "Hi, world! ";
  buf.appendf("%Q", "sql 'escaped'") << '\n';
  COUT << "Buffer contents: " << buf;

  buf.reset();
  assert(0==buf.used());
  assert(0<buf.capacity());
  f::FslOutputFStream ops(fsl_output_f_buffer, buf.handle());
  ops << "Hi, world!";
  assert(10 == buf.used());

  buf.clear();
  assert(0==buf.used());
  assert(0==buf.capacity());
  assert(NULL==buf.mem());
}

static void test_deck_1(fsl::Context &cx){
  namespace f = fsl;
  int threw = 0;
  f::Deck d(cx, FSL_SATYPE_CONTROL);
  f::ContextOStream fout(cx);

  try {
    d.assertHasRequiredCards();
  }catch(f::Exception const &ex){
    threw = ex.code();
    fout << "Got expected exception: " << ex.what() << '\n';
  }
  assert(FSL_RC_SYNTAX==threw);

  {
    f::Context::Transaction tr(cx)
      /* We'll roll back everything done here... */
      ;
    const std::string tgtArty(cx.symToUuid("rid:1"));
      /* self-referencing tag in a FSL_SATYPE_CONTROL is not legal, so
         we arbitrarily choose the initial empty checking. */;
    fout << "Custom "<< fsl_satype_cstr(d.type()) << " deck:\n";
    d.setCardD()
      .setCardU()
      .addCardT(FSL_TAGTYPE_ADD, "myTag", tgtArty.c_str(), "its value")
      .addCardT(FSL_TAGTYPE_PROPAGATING, "myOtherTag", tgtArty.c_str())
      .addCardT(FSL_TAGTYPE_CANCEL, "sumdumtag", tgtArty.c_str())
      //.unshuffle().output(fout)
      ;
    d.save();
    fout << "RID/UUID after save (will be rolled back): "<<d.rid()
         << ' ' << d.uuid()<<'\n';

    {

      typedef f::Deck::TCardIterator TagIter;
      TagIter it(d);
      TagIter end;
      fout << "Iterating over tags:";
      for( ; it != end; ++it ){
        fsl_card_T const * tag = *it;
        fout << ' ' << fsl_tag_prefix_char(tag->type)
             << tag->name;
      }
      fout << '\n';
    }

    fout << "Re-read artifact via Deck::load():\n";
    fout << f::Deck(cx).load( d.rid() );

    f::Buffer content;
    cx.getContent( d.rid(), content );
    fout << "And again via Context::getContent():\n" << content;
  }

  if(0){
    fsl_id_t rid = 1;
    fout << "Loading checkin #"<<rid<<"...\n";
    d.load( rid ).output( fout );
  }

  { /* FCardIterator... */
    f::Deck withF(cx);
    withF.load("current");
    f::Deck::FCardIterator fit(withF);
    f::Deck::FCardIterator end;
    fout << "Iterating over F-cards...\n";
    assert(*fit);
    assert(!*end);
    assert(fit!=end);
    int counter = 0;
    for( ; fit != end; ++fit, ++counter ){
      fsl_card_F const * fc = *fit;
      assert(fc);
      assert(fc->name);
      assert(fit->name == fc->name);
    }
    fout << "Traversed "
         <<counter<<" F-card(s) in deck #"
         <<withF.rid()<<".\n";

  }

}

int main(int argc, char const * const * argv ){
  int rc = EXIT_SUCCESS;
  try {
    namespace f = fsl;
    f::Context cx;
    f::ContextOStream fout(cx);
    fout << "ContextOStream...\n";
    f::ContextOStreamBuf cout2(cx, std::cerr);
    f::ContextOStreamBuf cerr2(cx, std::cout);
    CERR << "cerr through fsl_output()\n";
    COUT << "cout through fsl_output()\n";
    cx.openCheckout();

    test_db_1();
    test_stream_1(cx);
    test_deck_1(cx);


    /* Manually closing the Context is not generally required,
       we're just testing an assertion or three... */
    assert(cx.dbRepo().isOpened());
    assert(cx.dbCheckout().isOpened());
    cx.closeDbs();
    assert(!cx.dbRepo().isOpened());
    assert(!cx.dbCheckout().isOpened());
  }catch(fsl::Exception const &fex){
    CERR << "EXCEPTION: " << RCStr(fex.code()) << ": " << fex.what() << '\n';
    rc = EXIT_FAILURE;
  }catch(std::exception const &ex){
    CERR << "EXCEPTION: " << ex.what() << '\n';
    rc = EXIT_FAILURE;
  }
  COUT << "Exiting. Result = " << (rc ? ":`(" : ":-D") << '\n';
  return rc;
}
