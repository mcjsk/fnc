#
# FNC Makefile
#

# CONFIGUGRAION
CC ?=		cc
PREFIX ?=	/usr/local
MANDIR ?=	/share/man
VERSION ?=	0.4

# USED BELOW FOR PLATFORM SPECIFIC LDFLAGS
UNAME :=	$(shell uname -s)

# FLAGS NEEDED TO BUILD SQLITE3
SQLITE_CFLAGS =	${CFLAGS} -Wall -Werror -Wno-sign-compare -pedantic -std=c99 \
		-DNDEBUG=1 \
		-DSQLITE_DQS=0 \
		-DSQLITE_THREADSAFE=0 \
		-DSQLITE_DEFAULT_MEMSTATUS=0 \
		-DSQLITE_DEFAULT_WAL_SYNCHRONOUS=1 \
		-DSQLITE_LIKE_DOESNT_MATCH_BLOBS \
		-DSQLITE_OMIT_DECLTYPE \
		-DSQLITE_OMIT_PROGRESS_CALLBACK \
		-DSQLITE_OMIT_SHARED_CACHE \
		-DSQLITE_OMIT_LOAD_EXTENSION \
		-DSQLITE_MAX_EXPR_DEPTH=0 \
		-DSQLITE_USE_ALLOCA \
		-DSQLITE_ENABLE_LOCKING_STYLE=0 \
		-DSQLITE_DEFAULT_FILE_FORMAT=4 \
		-DSQLITE_ENABLE_EXPLAIN_COMMENTS \
		-DSQLITE_ENABLE_FTS4 \
		-DSQLITE_ENABLE_DBSTAT_VTAB \
		-DSQLITE_ENABLE_JSON1 \
		-DSQLITE_ENABLE_FTS5 \
		-DSQLITE_ENABLE_STMTVTAB \
		-DSQLITE_HAVE_ZLIB \
		-DSQLITE_INTROSPECTION_PRAGMAS \
		-DSQLITE_ENABLE_DBPAGE_VTAB \
		-DSQLITE_TRUSTED_SCHEMA=0

# FLAGS NEEDED TO BUILD LIBFOSSIL
FOSSIL_CFLAGS =	${CFLAGS} -Wall -Werror -Wsign-compare -pedantic -std=c99

# FLAGS NEEDED TO BUILD FNC
FNC_CFLAGS =	${CFLAGS} -Wall -Werror -Wsign-compare -pedantic -std=c99 \
		-I./lib -D_XOPEN_SOURCE_EXTENDED -DVERSION=${VERSION}

FNC_LDFLAGS =	${LDFLAGS} -lm -lutil -lz -lpthread -fPIC

# OSX has ncursesw, and needs iconv.
ifeq ($(UNAME),Darwin)
# OSX
FNC_LDFLAGS +=	-lncurses -lpanel -liconv
else
# Linux (tested on Debian), OpenBSD, FreeBSD
FNC_LDFLAGS +=	-lncursesw -lpanelw
endif

ifneq (,$(wildcard /usr/include/ncursesw/ncurses.h))
# On SOME Linux platforms, we have to include wide-char curses from
# here, but linking to to -lncursesw (with no special -L path) works
# fine. Ubuntu 18.04.6 is known to have this quirk.
FNC_CFLAGS += -I/usr/include/ncursesw
endif

all: bin

debug: FNC_CFLAGS += -DDEBUG -g
debug: FOSSIL_CFLAGS += -DDEBUG -g
debug: SQLITE_CFLAGS += -DDEBUG -g
debug: bin

bin: lib/sqlite3.o lib/libfossil.o src/fnc.o src/fnc

lib/sqlite3.o: lib/sqlite3.c lib/sqlite3.h
	${CC} ${SQLITE_CFLAGS} -c $< -o $@

lib/libfossil.o: lib/libfossil.c lib/libfossil.h
	${CC} ${FOSSIL_CFLAGS} -c $< -o $@

src/fnc.o: src/fnc.c
	${CC} ${FNC_CFLAGS} -c $< -o $@

src/fnc: src/fnc.o lib/libfossil.o lib/sqlite3.o
	${CC} -o $@ src/fnc.o lib/libfossil.o lib/sqlite3.o ${FNC_LDFLAGS}

install:
	install -s -m 0755 src/fnc ${PREFIX}/bin/fnc
	install -m 0644 src/fnc.1 ${PREFIX}${MANDIR}/man1/fnc.1

uninstall:
	rm -f ${PREFIX}/bin/fnc ${PREFIX}${MANDIR}/man1/fnc.1

clean:
	rm -f lib/*.o src/*.o src/fnc

release: clean
	tar czvf ../fnc-${VERSION}.tgz -C .. fnc-${VERSION}

.PHONY: clean release
