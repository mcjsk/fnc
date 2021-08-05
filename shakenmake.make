########################################################################
# ShakeNMake is a GNU Makefile mini-framework for small C/C++ source
# trees (with a small handful of directories, building libraries
# and/or applications. It relies very much on GNU Make 3.81+ and its
# dependencies generation relies on GCC (for the preprocessor only),
# even if GCC will not be used be used as the primary compiler.
all:
########################################################################
# Maintainer's/hacker's notes:
#
# Vars names starting with ShakeNMake are mostly internal to this
# makefile and are considered "private" unless documented otherwise.
# Notable exceptions are most of the ShakeNMake.CALL entries, which
# are $(call)able functions, and ShakeNMake.EVAL entries, which are
# $(eval)able code.
#
########################################################################


########################################################################
# Calculate the top of the source tree, in case this is included
# from a subdir.
ShakeNMake.MAKEFILE := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
$(ShakeNMake.MAKEFILE):# avoid breaking some deps checks if someone renames this file (been there, done that)
TOP_SRCDIR_REL ?= $(patsubst %/,%,$(dir $(ShakeNMake.MAKEFILE)))
#TOP_SRCDIR := $(shell cd -P $(TOP_SRCDIR_REL) && pwd)
TOP_SRCDIR := $(realpath $(TOP_SRCDIR_REL))
#$(error TOP_SRCDIR_REL=$(TOP_SRCDIR_REL)   TOP_SRCDIR=$(TOP_SRCDIR))
#TOP_INCDIR := $(TOP_SRCDIR_REL)/include
#TOP_INCDIR := $(TOP_SRCDIR)/include
#INCLUDES += -I. -I$(TOP_INCDIR) -I$(HOME)/include
#CPPFLAGS += $(INCLUDES)

#TOP_DIR ?= .#$(PWD)
#ShakeNMake.CISH_SOURCES += $(wildcard *.c)

PACKAGE.MAKEFILE := $(firstword $(MAKEFILE_LIST))# normally either Makefile or GNUmakefile
$(PACKAGE.MAKEFILE):


########################################################################
# ShakeNMake.CALL.FIND_FILE call()able function:
# $1 = app name
# $2 = optional path ($(PATH) is always included)
ShakeNMake.CALL.FIND_FILE = $(firstword $(wildcard $(addsuffix /$(1),$(subst :, ,$(2) $(PATH)))))
########################################################################
ShakeNMake.BINS.AR := $(call ShakeNMake.CALL.FIND_FILE,$(AR))
ShakeNMake.BINS.GCC := $(call ShakeNMake.CALL.FIND_FILE,gcc)


ifneq (,$(COMSPEC))
$(warning Setting ShakeNMake.SMELLS.LIKE.WINDOWS to 1)
ShakeNMake.SMELLS.LIKE.WINDOWS := 1
ShakeNMake.EXTENSIONS.DLL = .dll
ShakeNMake.EXTENSIONS.LIB = .lib
ShakeNMake.EXTENSIONS.EXE = .exe
else
ShakeNMake.SMELLS.LIKE.WINDOWS := 0
ShakeNMake.EXTENSIONS.DLL = .so
ShakeNMake.EXTENSIONS.LIB = .a
ShakeNMake.EXTENSIONS.EXE =# no whitespace, please
endif

########################################################################
# Cleanup rules.
# To ensure proper cleanup order, define deps on clean-., like so:
# clean-.: clean-mysubdir
# clean-mysubdir: clean-prereqsubdir
#...
.PHONY: clean distclean clean-. distclean-.
clean-%:
	@$(MAKE) --no-print-directory -C $* clean
clean-.:
	@-echo "Cleaning up $(CURDIR)..."; \
	rm -f $@.$$$$ $(CLEAN_FILES) 

clean: clean-.

distclean-%:
	@$(MAKE) --no-print-directory -C $* distclean
distclean-.:
	@-echo "Dist-cleaning up $(CURDIR)..."; \
	rm -f $@.$$$$ $(CLEAN_FILES) $(DISTCLEAN_FILES) 
distclean: distclean-.


########################################################################
# An internal hack to enable "quiet" output. $(1) is a string which
# is shown ONLY if ShakeNMake.QUIET!=1
ShakeNMake.QUIET ?= 0
define ShakeNMake.CALL.SETX
if test x1 = "x$(ShakeNMake.QUIET)"; then echo $(1); else set -x; fi
endef
########################################################################

########################################################################
# Building a static library:
#
#   myLib.LIB.OBJECTS = foo.o bar.o
#   $(call ShakeNMake.CALL.RULES.LIBS,myLib)
#
# A target named myLib.LIB is created for building the library and
# $(myLib.LIB) holds the name of the compiled lib (typically
# "myLib.a").
#
########################################################################
# ShakeNMake.EVAL.RULES.LIB creates rules to build static library
# $(1).a
define ShakeNMake.EVAL.RULES.LIB
$(1).LIB = $(1).a
$(1).LIB: $$($(1).LIB)
CLEAN_FILES += $$($(1).LIB)
$$($(1).LIB): $$($(1).LIB.OBJECTS)
	@$(call ShakeNMake.CALL.SETX,"AR [$$@]"); \
		$$(ShakeNMake.BINS.AR) crs $$@ $$($(1).LIB.OBJECTS)
endef
define ShakeNMake.CALL.RULES.LIBS
$(foreach liba,$(1),$(eval $(call ShakeNMake.EVAL.RULES.LIB,$(liba))))
endef
# end ShakeNMake.EVAL.RULES.LIB
########################################################################

########################################################################
# ShakeNMake.EVAL.RULES.DLL builds builds
# $(1)$(ShakeNMake.EXTENSIONS.DLL) from object files defined by
# $(1).DLL.OBJECTS and $(1).DLL.SOURCES. Flags passed on to the shared
# library linker ($(CC)) include:
#
#   LDFLAGS, $(1).DLL.LDFLAGS, LDADD, -shared -static-libgcc,
#   $(1).DLL.OBJECTS, $(1).DLL.LDFLAGS
#
# Also defines the var $(1).DLL, which expands to the filename of the DLL,
# (normally $(1)$(ShakeNMake.EXTENSIONS.DLL)).
define ShakeNMake.EVAL.RULES.DLL
$(1).DLL = $(1)$(ShakeNMake.EXTENSIONS.DLL)
ifneq (.DLL,$(ShakeNMake.EXTENSIONS.DLL))
$(1).DLL: $$($(1).DLL)
endif
CLEAN_FILES += $$($(1).DLL)
$$($(1).DLL): $$($(1).DLL.SOURCES) $$($(1).DLL.OBJECTS)
	@test x = "x$$($(1).DLL.OBJECTS)$$($(1).DLL.SOURCES)" && { \
	echo "$(1).DLL.OBJECTS and/or $(1).DLL.SOURCES are/is undefined!"; exit 1; }; \
	$(call ShakeNMake.CALL.SETX,"LD [$$@]"); \
	 $$(CC) -o $$@ -shared $$(LDFLAGS) $(LDLIBS) \
		$$($(1).DLL.OBJECTS) \
		$$($(1).DLL.LDFLAGS)
endef
########################################################################
# $(call ShakeNMake.CALL.RULES.DLLS,[list]) calls and $(eval)s
# ShakeNMake.EVAL.RULES.DLL for each entry in $(1)
define ShakeNMake.CALL.RULES.DLLS
$(foreach dll,$(1),$(eval $(call ShakeNMake.EVAL.RULES.DLL,$(dll))))
endef
# end ShakeNMake.CALL.RULES.DLLS and friends
########################################################################


########################################################################
# ShakeNMake.CALL.RULES.BIN is intended to be called like so:
# $(eval $(call ShakeNMake.CALL.RULES.BIN,MyApp))
#
# It builds a binary named $(1) by running $(CXX) and passing it:
#
# LDFLAGS, $(1).BIN.LDFLAGS, $(1).BIN.OBJECTS
define ShakeNMake.CALL.RULES.BIN
$(1).BIN := $(1)$(ShakeNMake.EXTENSIONS.EXE)
$(1).BIN: $$($(1).BIN)
# Many developers feel that bins should not be cleaned by 'make
# clean', but instead by distclean, but i'm not one of those
# developers. i subscribe more to the school of thought that distclean
# is for cleaning up configure-created files. That said, shake-n-make
# isn't designed to use a configure-like process, so that is probably
# moot here and we probably (maybe?) should clean up bins only in
# distclean. As always: hack it to suit your preference:
CLEAN_FILES += $$($(1).BIN)
$$($(1).BIN): $$($(1).BIN.OBJECTS)
	@test x = "x$$($(1).BIN.OBJECTS)" && { \
	echo "$(1).BIN.OBJECTS is undefined!"; exit 1; }; \
	$(call ShakeNMake.CALL.SETX,"LINK [$$@]"); \
	$$(CC) -o $$@ \
		$$($(1).BIN.OBJECTS) \
		$$(LDFLAGS) $$($(1).BIN.LDFLAGS)
endef
########################################################################
# $(call ShakeNMake.EVAL.RULES.BINS,[list]) calls and $(eval)s
# ShakeNMake.CALL.RULES.BIN for each entry in $(1)
define ShakeNMake.CALL.RULES.BINS
$(foreach bin,$(1),$(eval $(call ShakeNMake.CALL.RULES.BIN,$(bin))))
endef
# end ShakeNMake.CALL.RULES.BIN and friends
########################################################################

########################################################################
# Sets up rules for subdir $1.
.PHONY: subdir-.
subdir-.:
define ShakeNMake.CALL.SUBDIR
.PHONY: $(1)
subdir-$(1):
	+$(MAKE) -C $(1)
clean: clean-$(1)
clean-.: clean-$(1)
distclean-.: distclean-$(1)
endef
define ShakeNMake.CALL.SUBDIRS
$(foreach dir,$(1),$(eval $(call ShakeNMake.CALL.SUBDIR,$(dir))))
endef
# end ShakeNMake.CALL.RULES.DLLS and friends
########################################################################


########################################################################
# Automatic dependencies generation for C/C++ code...
# To disable deps generation, set ShakeNMake.USE_MKDEPS=0 *before*
# including this file.
ShakeNMake.USE_MKDEPS := 1
ifeq (,$(ShakeNMake.BINS.GCC))
ShakeNMake.USE_MKDEPS ?= 0
else
ShakeNMake.USE_MKDEPS ?= 1
endif
#$(warning ShakeNMake.USE_MKDEPS=$(ShakeNMake.USE_MKDEPS));
ifeq (1,$(ShakeNMake.USE_MKDEPS))
ShakeNMake.CISH_SOURCES ?= $(wildcard *.cpp *.c *.c++ *.h *.hpp *.h++ *.hh)
#$(warning ShakeNMake.CISH_SOURCES=$(ShakeNMake.CISH_SOURCES))

ifneq (,$(ShakeNMake.CISH_SOURCES))
ShakeNMake.CISH_DEPS_FILE := .make.c_deps
ShakeNMake.BINS.MKDEP = gcc -w -E -MM $(CPPFLAGS)
#$(ShakeNMake.CISH_SOURCES): $(filter-out $(ShakeNMake.CISH_DEPS_FILE),$(MAKEFILE_LIST))
CLEAN_FILES += $(ShakeNMake.CISH_DEPS_FILE)
$(ShakeNMake.CISH_DEPS_FILE): $(PACKAGE.MAKEFILE) $(ShakeNMake.MAKEFILE) $(ShakeNMake.CISH_SOURCES)
	@touch $@; test x = "x$(ShakeNMake.CISH_SOURCES)" && exit 0; \
	echo "Generating dependencies..."; \
	$(ShakeNMake.BINS.MKDEP) $(ShakeNMake.CISH_SOURCES) 2>/dev/null > $@ || exit

#	perl -i -pe 's|^(.+)\.o:\s*((\w+/)*)|$(OBJ.DIR)/$$1.o: $(filter-out $(ShakeNMake.CISH_DEPS_FILE),$(MAKEFILE_LIST))\n$(OBJ.DIR)/$$1.o: $$2|' $@
# That perl bit is a big, ugly, tree-specific hack for libfossil!
# gcc -E -MM strips directory parts (why, i cannot imagine), so we've got to
# hack the output.
# Example: input= src/Foo.cpp
# Generated deps:
#  Foo.o: src/Foo.cpp ...
# So we re-extract that first path and prepend it to Foo.o using the
# above perl.  We also add $(MAKEFILE_LIST) has a prereq of all src
# files because i belong to the school of tought which believes that
# changes to the arbitrary build options set in the makefiles should
# cause a rebuild.
#	if [[ x = "x$(TOP_DIR)" ]]; then echo "REMOVE THIS TREE-SPECIFIC HACK!"; exit 1; fi;\
#ifeq (,$(TOP_DIR))
#$(error Tree-specific hack in place here. See the comments and remove these lines or set TOP_DIR)
#endif
# normally we also want:
#  || $(ShakeNMake.BINS.RM) -f $@ 2>/dev/null
# because we don't want a bad generated makefile to kill the build, but gcc -E
# is returning 1 when some generated files do not yet exist.
deps: $(ShakeNMake.CISH_DEPS_FILE)

ifneq (,$(strip $(filter distclean clean,$(MAKECMDGOALS))))
# $(warning Skipping C/C++ deps generation.)
# ABSOLUTEBOGO := $(shell $(ShakeNMake.BINS.RM) -f $(ShakeNMake.CISH_DEPS_FILE))
else
#$(warning Including C/C++ deps.)
-include $(ShakeNMake.CISH_DEPS_FILE)
endif

endif
# ^^^^ ifneq(,$(ShakeNMake.CISH_SOURCES))
endif
# ^^^^ end $(ShakeNMake.USE_MKDEPS)
########################################################################

########################################################################
# Deps, Tromey's Way:
# $(call ShakeNBake.CALL.MkDep.Tromey,source-file,object-file,depend-file)
ifeq (0,1)
.dfiles.list = $(wildcard *.o.d) $(shell find src -name '*.o.d')
ifneq (,$(.dfiles.list))
# $(error include $(.dfiles.list))
include $(.dfiles.list)
endif

define ShakeNBake.CALL.MkDep.Tromey
$(ShakeNMake.BINS.GCC) -MM \
	-w	\
	-MF $(1).d         \
	-MP            \
	-MT $2         \
	$(CPPFLAGS)    \
	$(TARGET_ARCH) \
	$1
endef

CLEAN_FILES += $(wildcard *.o.d)
%.o: %.c $(mkdir_compdb)
	@$(call ShakeNMake.CALL.SETX,"CC [$@]"); \
	$(COMPILE.c) -MM -MF $(@).d -MP -MT $@.d $(COMPILE.c.OPTIONS) $(OUTPUT_OPTION) $<
#	$(call ShakeNBake.CALL.MkDep.Tromey,$<,$@,$(@).d);

%.o: %.cpp
	@$(call ShakeNMake.CALL.SETX,"C++ [$@]"); \
	$(COMPILE.cc) -MM -MF $@.d -MP -MT $@ $(COMPILE.cc.OPTIONS) $(OUTPUT_OPTION) $<
#	$(COMPILE.cc) $(COMPILE.cc.OPTIONS) $(OUTPUT_OPTION) $<
#	$(call ShakeNBake.CALL.MkDep.Tromey,$<,$@,$(@).d);
else
%.o: %.c $(mkdir_compdb)
	@$(call ShakeNMake.CALL.SETX,"CC [$@]"); \
	$(COMPILE.c) $(COMPILE.c.OPTIONS) $(OUTPUT_OPTION) $<

%.o: %.cpp
	@$(call ShakeNMake.CALL.SETX,"C++ [$@]"); \
	$(COMPILE.cc) $(COMPILE.cc.OPTIONS) $(OUTPUT_OPTION) $<
endif # Tromey's Way

########################################################################
# create emacs tags...
bin_etags := $(call ShakeNMake.CALL.FIND_FILE,etags)
ifeq (.,$(TOP_SRCDIR_REL))
ifneq (,$(bin_etags))
TAGTHESE := $(shell find . -name '*.[ch]' -print)
ifneq (,$(TAGTHESE))
    $(TAGTHESE):
    TAGFILE := TAGS
    $(TAGFILE): $(TAGTHESE) # $(MAKEFILE_LIST)
	@echo "Creating $@..."; \
	for x in $(TAGTHESE); do echo $$x; done | $(bin_etags) -f $@ -; \
		touch $@; true
# ^^^^ Martin G. reports that etags fails with "Unknown option: -" on his
# Ubuntun 18.04 system. Since this isn't a critical feature, we can just
# force this step to succeed.
    tags: $(TAGFILE)
    CLEAN_FILES += $(TAGFILE)
    all: $(TAGFILE)
endif
endif
# ^^^ bin_etags
endif
# ^^^ TOP_SRCDIR_REL==.

#%.o:
#	@$$(call ShakeNMake.CALL.SETX,"CC [$@] ..."); \
#	$(COMPILE.c) $(COMPILE.c.OPTIONS) -o $@
