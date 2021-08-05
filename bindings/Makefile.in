#!/usr/bin/make # help out emacs
all:
SUBDIRS :=
# Subdir cleanup rules and deps list must come before shakenmake.make is included
# or they must be set up manually afterwards...
ifneq (,$(CXX))
clean-.: clean-cpp
distclean-.: distclean-cpp
SUBDIRS += cpp
all: subdir-cpp
endif

clean-.: clean-s2
distclean-.: distclean-s2
include ../subdir-inc.make
SUBDIRS += s2
$(eval $(call ShakeNMake.CALL.SUBDIRS,$(SUBDIRS)))
all: subdir-s2
