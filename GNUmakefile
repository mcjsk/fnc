#
# FNC GNU Makefile
#

# CONFIGURATION
include fnc.bld.mk

UNAME :=	$(shell uname -s)

# OSX has ncursesw, and needs iconv.
ifeq ($(UNAME),Darwin)
# OSX
FNC_LDFLAGS +=	-lncurses -lpanel -liconv
else
# Linux (tested on Debian), OpenBSD, FreeBSD
FNC_LDFLAGS +=	-lncursesw -lpanelw
endif
