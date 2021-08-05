/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_PAGES_H_INCLUDED)
#define ORG_FOSSIL_SCM_PAGES_H_INCLUDED
/*
  Copyright 2013-2021 The Libfossil Authors, see LICENSES/BSD-2-Clause.txt

  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
  SPDX-FileCopyrightText: 2021 The Libfossil Authors
  SPDX-ArtifactOfProjectName: Libfossil
  SPDX-FileType: Code

  Heavily indebted to the Fossil SCM project (https://fossil-scm.org).

  *****************************************************************************
  This file contains only Doxygen-format documentation, split up into
  Doxygen "pages", each covering some topic at a high level.  This is
  not the place for general code examples - those belong with their
  APIs.
*/

/** @mainpage libfossil

    Forewarning: this API assumes one is familiar with the Fossil SCM,
    ideally in detail. The Fossil SCM can be found at:

    https://fossil-scm.org

    libfossil is an experimental/prototype library API for the Fossil
    SCM. This API concerns itself only with the components of fossil
    which do not need user interaction or the display of UI components
    (including HTML and CLI output). It is intended only to model the
    core internals of fossil, off of which user-level applications
    could be built.

    The project's repository and additional information can be found at:

    https://fossil.wanderinghorse.net/r/libfossil/

    This code is 100% hypothetical/potential, and does not represent
    any Official effort of the Fossil project. It is up for any amount
    of change at any time and does not yet have a stable API.

    All Fossil users are encouraged to participate in its development,
    but if you are reading this then you probably already knew that
    :).

    This effort does not represent "Fossil Version 2", but provides an
    alternate method of accessing and manipulating fossil(1)
    repositories. Whereas fossil(1) is a monolithic binary, this API
    provides library-level access to (some level of) the fossil(1)
    feature set (that level of support grows approximately linearly
    with each new commit).

    Current status: alpha. Some bits are basically finished but there
    is a lot of work left to do. The scope is pretty much all
    Fossil-related functionality which does not require a user
    interface or direct user interaction, plus some range of utilities
    to support those which require a UI/user.
*/

/** @page page_terminology Fossil Terminology

    See also: https://fossil-scm.org/home/doc/trunk/www/concepts.wiki

    The libfossil API docs normally assume one is familiar with
    Fossil-internal terminology, which is of course a silly assumption
    to make. Indeed, one of libfossil's goals is to make Fossil more
    accessible, partly be demystifying it. To that end, here is a
    collection of terms one may come across in the API, along with
    their meanings in the context of Fossil...


    - REPOSITORY (a.k.a. "repo) is an sqlite database file which
    contains all content for a given "source tree." (We will use the
    term "source tree" to mean any tree of "source" (documents,
    whatever) a client has put under Fossil's supervision.)

    - CHECKOUT (a.k.a. "local source tree" or "working copy") refers
    to (A) the action of pulling a specific version of a repository's
    state from that repo into the local filesystem, and (B) a local
    copy "checked out" of a repo. e.g. "he checked out the repo," and
    "the changes are in his [local] checkout."

    - ARTIFACT is the generic term for anything stored in a repo. More
    specifically, ARTIFACT refers to "control structures" Fossil uses
    to internally track changes. These artifacts are stored as blobs
    in the database, just like any other content. For complete details
    and examples, see:
    https://fossil-scm.org/home/doc/tip/www/fileformat.wiki

    - A MANIFEST is a specific type of ARTIFACT - the type which
    records all metadata for a COMMIT operation (which files, which
    user, the timestamp, checkin comment, lineage, etc.). For
    historical reasons, MANIFEST is sometimes used as a generic term
    for ARTIFACT because what the fossil(1)-internal APIs originally
    called a Manifest eventually grew into other types of artifacts
    but kept the Manifest naming convention. In Fossil developer
    discussion, "manifest" most often means what this page calls
    ARTIFACT (probably because that how the C code is modelled).  The
    libfossil API calls uses the term "deck" instead of "manifest" to
    avoid ambiguity/confusion (or to move the confusion somewhere
    else, at least).

    - CHECKIN is the term libfossil prefers to use for COMMIT
    MANIFESTS. It is also the action of "checking in"
    (a.k.a. "committing") file changes to a repository.  A CHECKIN
    ARTIFACT can be one of two types: a BASELINE MANIFEST (or BASELINE
    CHECKIN) contains a list of all files in that version of the
    repository, including their file permissions and the UUIDs of
    their content. A DELTA MANFIEST is a checkin record which derives
    from a BASELINE MANIFEST and it lists only the file-level changes
    which happened between the baseline and the delta, recording any
    changes in content, permisions, or name, and recording
    deletions. Note that this inheritance of deltas from baselines is
    an internal optimization which has nothing to do with checkin
    version inheritance - the baseline of any given delta is normally
    _not_ its direct checkin version parent.

    - BRANCH, FORK, and TAG are all closely related in Fossil and are
    explained in detail (with pictures!) at:
    https://fossil-scm.org/home/doc/trunk/www/concepts.wiki
    In short: BRANCHes and FORKs are two names for the same thing, and
    both are just a special-case usage of TAGs.

    - MERGE or MERGING: the process of integrating one version of
    source code into another version of that source code, using a
    common parent version as the basis for comparison. This is
    normally fully automated, but occasionally human (and sometimes
    Divine) intervention is required to resolve so-called "merge
    conflicts," where two versions of a file change the same parts of
    a common parent version.

    - RID (Record ID) is a reference to the blob.rid field in a
    repository DB. RIDs are used extensively throughout the API for
    referencing content records, but they are transient values local
    to a given copy of a given repository at a given point in
    time. They _can_ change, even for the same content, (e.g. a
    rebuild can hypothetically change them, though it might not, and
    re-cloning a repo may very well change some RIDs). Clients must
    never rely on them for long-term reference to SCM'd data - always use
    the full UUID of such data. Even though they normally appear to be
    static, they are most explicitly NOT guaranteed to be. Nor are
    their values guaranteed to imply any meaning, e.g. "higher is
    newer" is not necessarily true because synchronization can import
    new remote content in an arbitrary order and a rebuild might
    import it in random order. The API uses RIDs basically as handles
    to arbitrary blob content and, like most C-side handles, must be
    considered transient in nature. That said, within the db, records
    are linked to each other exclusively using RIDs, so they do have
    some persistence guarantees for a given db instance.
*/


/** @page page_APIs High-level API Overview

    The primary end goals of this project are to eventually cover the
    following feature areas:

    - Provide embeddable SCM to local apps using sqlite storage.
    - Provide a network layer on top of that for synchronization.
    - Provide apps on top of those to allow administration of repos.

    To those ends, the fossil APIs cover the following categories of
    features:

    Filesystem:

    - Conversions of strings from OS-native encodings to UTF.
    fsl_utf8_to_unicode(), fsl_filename_to_utf8(), etc. These are
    primarily used internally but may also be useful for applications
    working with files (as most clients will). Actually... most of
    these bits are only needed for portability across Windows
    platforms.

    - Locating a user's home directory: fsl_find_home_dir()

    - Normalizing filenames/paths. fsl_file_canonical_name() and friends.

    - Checking for existence, size, and type (file vs directory) with
    fsl_is_file() and fsl_dir_check(), or the more general-purpose
    fsl_stat().


    Databases (sqlite):

    - Opening/closing sqlite databases and running queries on them,
    independent of version control features. See fsl_db_open() and
    friends. The actual sqlite-level DB handle type is abstracted out
    of the public API, largely to simplify an eventual port from
    sqlite3 to sqlite4 or (hypothetically) to other storage back-ends
    (not gonna happen - too much work).

    - There are lots of utility functions for oft-used operations,
    e.g. fsl_config_get_int32() and friends to fetch settings from one
    of several different configuration areas (global, repository,
    checkout, and "versionable" settings).

    - Pseudo-recusive transactions: fsl_db_transaction_begin() and
    fsl_db_transaction_end(). sqlite does not support truly nested
    transactions, but they can be simulated quite effectively so long
    as certain conventions are adhered to.

    - Cached statements (an optimization for oft-used queries):
    fsl_db_prepare_cached() and friends.


    The DB API is (as Brad Harder put so well) "very present" in the
    public API. While the core API provides access to the underlying
    repository data, it cannot begin to cover even a small portion of
    potential use cases. To that end, it exposes the DB API so that
    clients who want to custruct their own data can do so. It does
    require research into the underlying schemas, but gives
    applications the ability to do _anything_ with their repositories
    which the core API does not account for. Historically, the ability
    to create ad-hoc data structures as needed, in the form of SQL
    queries, has accounted for much of Fossil's feature flexibility.


    Deltas:

    - Creation and application of raw deltas, using Fossil's delta
    format, independent of version control features. See
    fsl_delta_create() and friends. These are normally used only at
    the deepest internal levels of fossil, but the APIs are exposed so
    that clients can, if they wish, use them to deltify their own
    content independently of fossil's internally-applied
    deltification. Doing so is remarkably easy, but completely
    unnecessary for content which will be stored in a repo, as Fossil
    creates deltas as needed.


    SCM:

    - A "context" type (fsl_cx) which manages a repository db and,
    optionally, a checkout db. Read-only operations on the DB are
    working and write functionality (adding repo content) is
    ongoing. See fsl_cx, fsl_cx_init(), and friends.

    - The fsl_deck class assists in parsing, creating, and outputing
    "artifacts" (manifests, control (tags), events, etc.). It gets its
    name from it being container for "a collection of cards" (which is
    what a Fossil artifact is).

    - fsl_content_get() expands a (possibly) deltified blob into its
    full form, and fsl_content_blob() can be used to fetch a raw blob
    (possibly a raw delta).

    - A number of routines exist for converting symbol names to RIDs
    (fsl_sym_to_rid()), UUIDs to RIDs (fsl_uuid_to_rid(),
    and similar commonly-needed lookups.


    Input/Output:

    - The API defines several abstractions for i/o interfaces, e.g.
    fsl_input_f() and fsl_output_f(), which allow us to accept/emit
    data from/to arbitrary streamable (as opposed to random-access)
    sources/destinations. A fsl_cx instance is configured with an
    output channel, the intention being that all clients of that
    context should generate any output through that channel, so that
    all compatible apps can cooperate more easily in terms of i/o. For
    example, the s2 script binding for libfossil routes fsl_output()
    through the script engine's i/o channels, so that any output
    generated by libfossil-using code it links to can take advantage
    of the script-side output features (such as output buffering,
    which is needed for any non-trivial CGI output). That said: the
    library-level code does not actually generate output to that
    channel, but higher-level code like fcli does, and clients are
    encouraged to in order to enable their app's output to be
    redirected to an arbitrary UI element, be it a console or UI
    widget.


    Utilities:

    - fsl_buffer, a generic buffer class, is used heavily by the
    library.  See fsl_buffer and friends.

    - fsl_appendf() provides printf()-like functionality, but sends
    its output to a callback function (optionally stateful), making it
    the one-stop-shop for string formatting within the library.

    - The fsl_error class is used to propagate error information
    between the libraries various levels and the client.

    - The fsl_list class acts as a generic container-of-pointers, and
    the API provides several convenience routines for managing them,
    traversing them, and cleaning them up.

    - Hashing: there are a number of routines for calculating SHA1,
    SHA3, and MD5 hashes. See fsl_sha1_cx, fsl_sha3_cx, fsl_md5_cx,
    and friends.

    - zlib compression is used for storing artifacts. See
    fsl_data_is_compressed(), fsl_buffer_compress(), and friends.
    These are never needed at the client level, but are exposed "just
    in case" a given client should want them.
*/

/** @page page_is_isnot Fossil is/is not...

    Through porting the main fossil application into library form,
    the following things have become very clear (or been reinforced)...

    Fossil is...

    - _Exceedingly_ robust. Not only is sqlite literally the single
    most robust application-agnostic container file format on the
    planet, but Fossil goes way out of its way to ensure that what
    gets put in is what gets pulled out. It cuts zero corners on data
    integrity, even adding in checks which seem superfluous but
    provide another layer of data integrity (i'm primarily talking
    about the R-card here, but there are other validation checks). It
    does this at the cost of memory and performance (that said, it's
    still easily fast enough for its intended uses). "Robust" doesn't
    mean that it never crashes nor fails, but that it does so with
    (insofar as is technically possible) essentially zero chance of
    data loss/corruption.

    - Long-lived: the underlying data format is independent of its
    storage format. It is, in principal, usable by systems as yet
    unconceived by the next generation of programmers. This
    implementation is based on sqlite, but the model can work with
    arbitrary underlying storage.

    - Amazingly space-efficient. The size of a repository database
    necessarily grows as content is modified. However, Fossil's use of
    zlib-compressed deltas, using a very space-efficient delta format,
    leads to tremendous compression ratios. As of this writing (March,
    2021), the main Fossil repo contains approximately 5.36GB of
    content, were we to check out every single version in its
    history. Its repository database is only 64MB, however, equating
    to a 83:1 compression ration. Ratios in the range of 20:1 to 40:1
    are common, and more active repositories tend to have higher
    ratios. The TCL core repository, with just over 15 years of code
    history (imported, of course, as Fossil was introduced in 2007),
    is (as of September 2013) only 187MB, with 6.2GB of content and a
    33:1 compression ratio.

    Fossil is not...

    - Memory-light. Even very small uses can easily suck up 1MB of RAM
    and many operations (verification of the R card, for example) can
    quickly allocate and free up hundreds of MB because they have to
    compose various versions of content on their way to a specific
    version. To be clear, that is total RAM usage, not _peak_ RAM
    usage. Peak usage is normally a function of the content it works
    with at a given time, often in direct relation to (but
    significantly more than) the largest single file processed in a
    given session. For any given delta application operation, Fossil
    needs the original content, the new content, and the delta all in
    memory at once, and may go through several such iterations while
    resolving deltified content. Verification of its 'R-card' alone
    can require a thousand or more underlying DB operations and
    hundreds of delta applications. The internals use caching where it
    would save us a significant amount of db work relative to the
    operation in question, but relatively high memory costs are
    unavoidable. That's not to say we can't optimize a bit, but first
    make it work, then optimize it. The library takes care to re-use
    memory buffers where it is feasible (and not too intrusive) to do
    so, but there is yet more RAM to be optimized away in this regard.
*/

/** @page page_threading Threads and Fossil

    It is strictly illegal to use a given fsl_cx instance from more
    than one thread. Period.

    It is legal for multiple contexts to be running in multiple
    threads, but only if those contexts use different
    repository/checkout databases. Though access to the storage is,
    through sqlite, protected via a mutex/lock, this library does not
    have a higher-level mutex to protect multiple contexts from
    colliding during operations. So... don't do that. One context, one
    repo/checkout.

    Multiple application instances may each use one fsl_cx instance to
    share repo/checkout db files, but must be prepared to handle
    locking-related errors in such cases. e.g. db operations which
    normally "always work" may suddenly pause for a few seconds before
    giving up while waiting on a lock when multiple applications use
    the same database files. sqlite's locking behaviours are
    documented in great detail at https://sqlite.org.
 */

/** @page page_artifacts Creating Artifacts

    A brief overview of artifact creating using this API. This is targeted
    at those who are familiar with how artifacts are modelled and generated
    in fossil(1).

    Primary artifact reference:

    https://fossil-scm.org/home/doc/trunk/www/fileformat.wiki

    In fossil(1), artifacts are generated via the careful crafting of
    a memory buffer (large string) in the format described in the
    document above. While it's relatively straightforward to do, there
    are lots of potential gotchas, and a bug can potentially inject
    "bad data" into the repo (though the verify-before-commit process
    will likely catch any problems before the commit is allowed to go
    through). The libfossil API uses a higher-level (OO) approach,
    where the user describes a "deck" of cards and then tells the
    library to save it in the repo (fsl_deck_save()) or output it to
    some other channel (fsl_deck_output()). The API ensures that the
    deck's cards get output in the proper order and that any cards
    which require special treatment get that treatment (e.g. the
    "fossilize" encoding of certain text fields). The "deck" concept
    is equivalent to Artifact in fossil(1), but we use the word deck
    because (A) Artifact is highly ambiguous in this context and (B)
    deck is arguably the most obvious choice for the name of a type
    which acts as a "container of cards."

    Ideally, client-level code will never have to create an artifact
    via the fsl_deck API (because doing so requires a fairly good
    understanding of what the deck is for in the first place,
    including the individual Cards). The public API strives to hide
    those levels of details, where feasible, or at least provide
    simpler/safer alternatives for basic operations. Some operations
    may require some level of direct work with a fsl_deck
    instance. Likewise, much read-only functionality directly exposes
    fsl_deck to clients, so some familiarity with the type and its
    APIs will be necessary for most clients.

    The process of creating an artifact looks a lot like the following
    code example. We have elided error checking for readability
    purposes, but in fact this code has undefined behaviour if error
    codes are not checked and appropriately reacted to.

    @code
    fsl_deck deck = fsl_deck_empty;
    fsl_deck * d = &deck; // for typing convenience
    fsl_deck_init( fslCtx, d, FSL_SATYPE_CONTROL ); // must come first
    fsl_deck_D_set( d, fsl_julian_now() );
    fsl_deck_U_set( d, "your-fossil-name", -1 );
    fsl_deck_T_add( d, FSL_TAGTYPE_ADD, "...uuid being tagged...",
                   "tag-name", "optional tag value");
    ...
    // Now output it to stdout:
    fsl_deck_output( f, d, fsl_output_f_FILE, stdout );
    // See also: fsl_deck_save(), which stores it in the db and
    // "crosslinks" it.
    fsl_deck_finalize(d);
    @endcode

    The order the cards are added to the deck is irrelevant - they
    will be output in the order specified by the Fossil specs
    regardless of their insertion order. Each setter/adder function
    knows, based on the deck's type (set via fsl_deck_init()), whether
    the given card type is legal, and will return an error (probably
    FSL_RC_TYPE) if an attempt is made to add a card which is illegal
    for that deck type. Likewise, fsl_deck_output() and
    fsl_deck_save() confirm that the decks they are given contain (A)
    only allowed cards and (B) have all required
    cards. fsl_deck_output() will "unshuffle" the cards, making sure
    they're in the correct order.

    Sidebar: normally outputing a structure can use a const form of
    that structure, but the traversal of F-cards in a deck requires
    (for the sake of delta manifests) using a non-const cursor. Thus
    outputing a deck requires a non-const instance. If it weren't for
    delta manifests, we could be "const-correct" here.
*/

/** @page page_transactions DB Transactions

    The fsl_db_transaction_begin() and fsl_db_transaction_end()
    functions implement a basic form of recursive transaction,
    allowing the library to start and end transactions at any level
    without having to know whether a transaction is already in
    progress (sqlite3 does not natively support nested
    transactions). A rollback triggered in a lower-level transaction
    will propagate the error back through the transaction stack and
    roll back the whole transaction, providing us with excellent error
    recovery capabilities (meaning we can always leave the db in a
    well-defined state).

    It is STRICTLY ILLEGAL to EVER begin a transaction using "BEGIN"
    or end a transaction by executing "COMMIT" or "ROLLBACK" directly
    on a fsl_db instance. Doing so bypasses internal state which needs
    to be kept abreast of things and will cause Grief and Suffering
    (on the client's part, not mine).

    Tip: implementing a "dry-run" mode for most fossil operations is
    trivial by starting a transaction before performing the
    operations. Many operations run in a transaction, but if the
    client starts one of his own they can "dry-run" any op by simply
    rolling back the transaction he started. Abstractly, that looks
    like this pseudocode:

    @code
    db.begin();
    fsl.something();
    fsl.somethingElse();
    if( dryRun ) db.rollback();
    else db.commit();
    @endcode
*/

/** @page page_code_conventions Code Conventions

    Project and Code Conventions...

    Foreward: all of this more or less evolved organically or was
    inherited from fossil(1) (where it evolved organically, or was
    inherited from sqilte (where it evol...)), and is written up here
    more or less as a formality. Historically i've not been a fan of
    coding conventions, but as someone else put it to me, "the code
    should look like it comes from a single source," and the purpose
    of this section is to help orient those looking to hack in the
    sources. Note that most of what is said below becomes obvious
    within a few minutes of looking at the sources - there's nothing
    earth-shatteringly new nor terribly controversial here.

    The Rules/Suggestions/Guidelines/etc. are as follows...


    - C99 is the basis. It was C89 until 2021-02-12.

    - The canonical build environment uses the most restrictive set of
    warning/error levels possible. It is highly recommended that
    non-canonical build environments do the same. Adding -Wall -Werror
    -pedantic does _not_ guaranty that all C compliance/portability
    problems can be caught by the compiler, but it goes a long way in
    helping us to write clean code. The clang compiler is particularly
    good at catching subtle foo-foo's such as uninitialized variables.

    - API docs (as you may have already noticed), does not (any
    longer) follow Fossil's comment style, but instead uses
    Doxygen-friendly formatting. Each comment block MUST start with
    two or more asterisks, or '*!', or doxygen apparently doesn't
    understand it
    (https://www.stack.nl/~dimitri/doxygen/manual/docblocks.html). When
    adding code snippets and whatnot to docs, please use doxygen
    conventions if it is not too much of an inconvenience. All public
    APIs must be documented with a useful amount of detail. If you
    hate documenting, let me know and i'll document it (it's what i do
    for fun).

    - Public API members have a fsl_ or FSL_ prefix (fossil_ seems too
    long). For private/static members, anything goes. Optional or
    "add-on" APIs (e.g. ::fcli) may use other prefixes, but are
    encouraged use an "f-word" (as it were), simply out of deference
    to long-standing software naming conventions.

    - Public-API structs and functions use lower_underscore_style().
    Static/internal APIs may use different styles. It's not uncommon
    to see UpperCamelCase for file-scope structs.

    - Overall style, especially scope blocks and indentation, should
    follow Fossil's.  We are _not at all_ picky about whether or not
    there is a space after/before parens in if( foo ), and similar
    small details, just the overall code pattern.

    - Structs and enums all get the optional typedef so that they do
    not need to be qualified with 'struct' resp. 'enum' when
    used. Because of how doxygen tracks those, the typedef should be
    separate from the struct declaration, rather than combinding
    those.

    - Function typedefs are named fsl_XXX_f. Implementations of such
    typedefs/interfaces are typically named fsl_XXX_f_SUFFIX(), where
    SUFFIX describes the implementation's
    specialization. e.g. fsl_output_f() is a callback
    typedef/interface and fsl_output_f_FILE() is a concrete
    implementation for FILE handles.

    - Enums tend to be named fsl_XXX_e.

    - Functions follow the naming pattern prefix_NOUN_VERB(), rather
    than the more C-conventional prefix_VERB_NOUN(),
    e.g. fsl_foo_get() and fsl_foo_set() rather than fsl_get_foo() and
    fsl_get_foo(). The primary reasons are (A) sortability for
    document processors and (B) they more naturally match with OO API
    conventions, e.g.  noun.verb(). A few cases knowingly violate this
    convention for the sake of readability or sorting of several
    related functions (e.g. fsl_db_get_TYPE() instead of
    fsl_db_TYPE_get()).

    - Structs intended to be creatable on the stack are accompanied by
    a const instance named fsl_STRUCT_NAME_empty, and possibly by a
    macro named fsl_STRUCT_NAME_empty_m, both of which are
    "default-initialized" instances of that struct. This is superiour
    to using memset() for struct initialization because we can define
    (and document) arbitrary default values and all clients who
    copy-construct them are unaffected by many types of changes to the
    struct's signature (though they may need a recompile). The
    intention of the fsl_STRUCT_NAME_empty_m macro is to provide a
    struct-embeddable form for use in other structs or
    copy-initialization of const structs, and the _m macro is always
    used to initialize its const struct counterpart. e.g. the library
    guarantees that fsl_cx_empty_m (a macro representing an empty
    fsl_cx instance) holds the same default values as fsl_cx_empty (a
    const fsl_cx value).

    - Returning int vs fsl_int_t vs fsl_size_t: int is used as a
    conventional result code. fsl_int_t is often used as a signed
    length-style result code (e.g. printf() semantics). Unsigned
    ranges use fsl_size_t. Ints are (also) used as a "triplean" (3
    potential values, e.g. <0, 0, >0). fsl_int_t also guarantees that
    it will be 64-bit if available, so can be used for places where
    large values are needed but a negative value is legal (or handy),
    e.g. fsl_strndup()'s second argument. The use of the fsl_xxx_t
    typedefs, rather than (unsigned) int, is primarily for
    readability/documentation, e.g. so that readers can know
    immediately that the function uses a given argument or return
    value following certain API-wide semantics. It also allows us to
    better define platform-portable printf/scanf-style format
    modifiers for them (analog to C99's PRIi32 and friends), which
    often come in handy.

    - Signed vs. unsigned types for size/length arguments: use the
    fsl_int_t (signed) argument type when the client may legally pass
    in a negative value as a hint that the API should use fsl_strlen()
    (or similar) to determine a byte array's length. Use fsl_size_t
    when no automatic length determination is possible (or desired),
    to "force" the client to pass the proper length. Internally
    fsl_int_t is used in some places where fsl_size_t "should" be used
    because some ported-in logic relies on loop control vars being
    able to go negative. Additionally, fossil internally uses negative
    blob lengths to mark phantom blobs, and care must be taken when
    using fsl_size_t with those.

    - Functions taking elipses (...) are accompanied by a va_list
    counterpart named the same as the (...) form plus a trailing
    'v'. e.g. fsl_appendf() and fsl_appendfv(). We do not use the
    printf()/vprintf() convention because that hoses sorting of the
    functions in generated/filtered API documentation.

    - Error handling/reporting: please keep in mind that the core code
    is a library, not an application.  The main implication is that
    all lib-level code needs to check for errors whereever they can
    happen (e.g. on every single memory allocation, of which there are
    many) and propagate errors to the caller, to be handled at his
    discretion. The app-level code (::fcli) is not particularly strict
    in this regard, and installs its own allocator which abort()s on
    allocation error, which simplifies app-side code somewhat
    vis-a-vis lib-level code. When reporting an error can be improved
    by the inclusion of an error string, functions like
    fsl_cx_err_set() can be used to report the error. Several of the
    high-level types in the API have fsl_error object member which
    contains such error state. The APIs which use that state take care
    to use-use the error string memory whenever possible, so setting
    an error string is often a non-allocating operation.
*/


/** @page page_fossil_arch Fossil Architecture Overview

    An introduction to the Fossil architecture. These docs
    are basically just a reformulation of other, more detailed,
    docs which can be found via the main Fossil site, e.g.:

    - https://fossil-scm.org/home/doc/trunk/www/concepts.wiki

    - https://fossil-scm.org/home/doc/trunk/www/fileformat.wiki

    Fossil's internals are fundamentally broken down into two basic
    parts. The first is a "collection of blobs."  The simplest way to
    think of this (and it's not far from the full truth) is a
    directory containing lots of files, each one named after a hash of
    its contents. This pool contains ALL content required for a
    repository - all other data can be generated from data contained
    here. Included in the blob pool are so-called Artifacts. Artifacts
    are simple text files with a very strict format, which hold
    information regarding the idententies of, relationships involving,
    and other metadata for each type of blob in the pool. The most
    fundamental Artifact type is called a Manifest, and a Manifest
    tells us, amongst other things, which of the hash-based file names
    has which "real" file name, which version the parent (or parents!)
    is (or are), and other data required for a "commit" operation.

    The blob pool and the Manifests are all a Fossil repository really
    needs in order to function. On top of that basis, other forms of
    Artifacts provide features such as tagging (which is the basis of
    branching and merging), wiki pages, and tickets. From those
    Artifacts, Fossil can create/calculate all sorts of
    information. For example, as new Artifacts are inserted it
    transforms the Artifact's metadata into a relational model which
    sqlite can work with. That leads us to what is conceptually the
    next-higher-up level, but is in practice a core-most component...

    Storage. Fossil's core model is agnostic about how its blobs are
    stored, but libfossil and fossil(1) both make heavy use of sqlite
    to implement many of their features. These include:

    - Transaction-capable storage. It's almost impossible to corrupt a
    Fossil db in normal use. sqlite3 offers literally the most robust
    general-purpose file format on the planet.

    - The storage of the raw blobs.

    - Artifact metadata is transformed into various DB structures
    which allow libfossil to traverse historical data much more
    efficiently than would be possible without a db-like
    infrastructure (and everything that implies). These structures are
    kept up to date as new Artifacts are stored in a repository,
    either via local edits or synching in remote content. These data
    are incrementally updated as changes are made to a repo.

    - A tremendous amount of the "leg-work" in processing the
    repository state is handled by SQL queries, without which the
    library would easily require 5-10x more code in the form of
    equivalent hard-coded data structures and corresponding
    functionality. The db approach allows us to ad-hoc structures as
    we need them, providing us a great deal of flexibility.

    All content in a Fossil repository is in fact stored in a single
    database file. Fossil additionally uses another database (a
    "checkout" db) to keep track of local changes, but the repo
    contains all "fossilized" content. Each copy of a repo is a
    full-fledged repo, each capable of acting as a central copy for
    any number of clones or checkouts.

    That's really all there is to understand about Fossil. How it does
    its magic, keeping everything aligned properly, merging in
    content, how it stores content, etc., is all internal details
    which most clients will not need to know anything about in order
    to make use of fossil(1). Using libfossil effectively, though,
    does require learning _some_ amount of how Fossil works. That will
    require taking some time with _other_ docs, however: see the
    links at the top of this section for some starting points.


    Sidebar:

    - The only file-level permission Fossil tracks is the "executable"
    (a.k.a. "+x") bit. It internally marks symlinks as a permission
    attribute, but that is applied much differently than the
    executable bit and only does anything useful on platforms which
    support symlinks.

*/

#endif
/* ORG_FOSSIL_SCM_PAGES_H_INCLUDED */
