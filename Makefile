#
# FNC Makefile
#

# CONFIGURATION
.include "fnc.bld.mk"

UNAME !=	uname -s

# OSX has ncursesw, and needs iconv.
.if $(UNAME) == Darwin
# OSX
FNC_LDFLAGS +=	-lncurses -lpanel -liconv
.else
# Linux (tested on Debian), OpenBSD, FreeBSD
FNC_LDFLAGS +=	-lncursesw -lpanelw
.endif
