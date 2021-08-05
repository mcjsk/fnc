all:
include ../../subdir-inc.make

CPPFLAGS += -I$(TOP_SRCDIR)/src# workaround for in-tree sqlite3.h
CPPFLAGS += -fPIC
CPPFLAGS+=-DCWAL_ENABLE_TRACE=0

LIBFOSSIL.LDFLAGS := -L$(TOP_SRCDIR) -lfossil
ifeq (1,1)
  # In static build, we need some extra flags...
  LIBFOSSIL.LDFLAGS += $(LDFLAGS_MODULE_LOADER) -lpthread -lz
endif

ifeq (,$(strip $(filter distclean clean,$(MAKECMDGOALS))))
$(LIBFOSSIL.LDFLAGS):
	$(MAKE) -C ..
endif

S2_CLIENT_LDFLAGS := $(LIBFOSSIL.LDFLAGS) $(LDFLAGS_MODULE_LOADER)

########################################################################
# INC_SEARCH: $(call)able function. $(1) should be the the name of
# a C header file to search for under $(INCLUDES_PATH).
INCLUDES_PATH ?= $(HOME)/include /usr/local/include /usr/include
define INC_SEARCH
$(call ShakeNMake.CALL.FIND_FILE,$(1),$(INCLUDES_PATH))
endef

#$(error ltdl=$(HAVE_LT_DLOPEN) dl=$(HAVE_DLOPEN) mods=$(ENABLE_MODULES))
########################################################################
# Bins and libs...

########################################################################
# Check for readline
ENABLE_READLINE := $(FSL_ENABLE_READLINE)
ifeq (1,$(ENABLE_READLINE))
  READLINE_H := $(call INC_SEARCH,readline/readline.h)
  ifneq (,$(READLINE_H))
#    READLINE_IMPL := readline
    $(info Enabling GNU Readline)
    LDFLAGS_READLINE := -lreadline
  else
    $(info Not enabling GNU Readline)
    ENABLE_READLINE := 0
  endif
endif
# /readline
########################################################################

########################################
# Linenoise (terminal input lib)...
LN_OBJ :=
ENABLE_LINENOISE := 1
ifeq (1,$(ENABLE_READLINE))
  $(info Not enabling linenoise)
  ENABLE_LINENOISE := 0
else
  LN_DIR := linenoise
  ifneq (,$(wildcard $(LN_DIR)/*.c))
    ENABLE_LINENOISE := 1
    $(info Enabling linenoise)
  else
    $(info Not enabling linenoise)
    ENABLE_LINENOISE := 0
  endif
  ifeq (1,$(ENABLE_LINENOISE))
    LN_OBJ := $(LN_DIR)/linenoise.o $(LN_DIR)/utf8.o
    CLEAN_FILES += $(LN_OBJ)
    $(LN_DIR)/utf8.o: CPPFLAGS+=-std=c99 -DUSE_UTF8=1
    $(LN_DIR)/linenoise.o: CPPFLAGS+=-std=c99 -DUSE_UTF8=1 -D_BSD_SOURCE
  else
    LN_OBJ :=
  endif
endif
# /linenoise
########################################

########################################
# s2sh a.k.a. f-s2sh
shell2.o: CPPFLAGS+=-DS2_SHELL_EXTEND
shell2.o: CPPFLAGS+=-DS2SH_FOR_UNIT_TESTS=1
cliapp.o: CPPFLAGS+=-DCLIAPP_ENABLE_LINENOISE=$(ENABLE_LINENOISE)
cliapp.o: CPPFLAGS+=-DCLIAPP_ENABLE_READLINE=$(ENABLE_READLINE)
shell2.o shell_extend.o: CPPFLAGS+=-DS2_AMALGAMATION_BUILD


f-s2sh.BIN.OBJECTS := shell2.o shell_extend.o cliapp.o $(LN_OBJ) s2_amalgamation.o
f-s2sh.BIN.LDFLAGS := $(S2_CLIENT_LDFLAGS) $(LDFLAGS_READLINE)
$(eval $(call ShakeNMake.CALL.RULES.BINS,f-s2sh))
all: $(f-s2sh.BIN)
# s2's inclusion of miniz breaks its policy of building with
# the (-pedantic -std=c89) flags, and requires that we be less
# stringent with compilation flags :(...
s2_amalgamation.o: CFLAGS=-Wall -Werror -Wsign-compare -g -UNDEBUG -DDEBUG=1
s2_amalgamation.o shell2.o shell_extend.o: CPPFLAGS+=-DS2_ENABLE_ZLIB=1 -DHAVE_CONFIG_H
s2_amalgamation.o: config.h

# end bins and libs
########################################################################

########################################
# Set up file-specific CPPFLAGS/LDFLAGS.
# basic module setup
ifeq (1,$(ENABLE_MODULES))
  s2_amalgamation.o: CPPFLAGS+=-DS2_ENABLE_MODULES=1
  f-s2sh.BIN.LDFLAGS += $(LDFLAGS_MODULE_LOADER)
  ifeq (1,$(HAVE_LIBDL)) # use libdl
    s2_amalgamation.o: CPPFLAGS+=-DS2_HAVE_DLOPEN=1
  else # use libltdl
    s2_amalgamation.o: CPPFLAGS+=-DS2_HAVE_LTDLOPEN=1
  endif
endif
# end modules
########################################################################

########################################################################
# Unit test stuff...
UNIT_SCRIPT_LIST := $(sort $(subst ./,,$(wildcard unit/???-???-*.s2 unit2/???-*.s2)))
$(UNIT_SCRIPT_LIST):
S2SH.SHELL.FLAGS ?= --no-init-script
# -w
UNIT_RUN_CMD = ./$(f-s2sh.BIN) $(S2SH.SHELL.FLAGS)
UNIT_MEGA.S2 := UNIT.s2
UNIT_MEGA2.S2 := UNIT-import.s2
UNIT_GENERATED := $(UNIT_MEGA.S2) $(UNIT_MEGA2.S2)
CLEAN_FILES += $(UNIT_GENERATED)
$(UNIT_MEGA.S2): $(UNIT_SCRIPT_LIST) Makefile
	@echo "Generating $@..."
	@{ \
		false && echo "const INTERN_THESE=['object','array','integer','double','string','function','bool'];"; \
		for i in $(UNIT_SCRIPT_LIST); do \
			echo "scope {/* begin file: $$i */"; \
			cat $$i; \
			echo "/* end file: $$i */;;}"; \
			echo ""; \
		done; \
	} > $@
$(UNIT_MEGA2.S2): $(UNIT_SCRIPT_LIST) Makefile
	@echo "Generating $@..."
	@{ \
		false && echo "const INTERN_THESE=['object','array','integer','double','string','function','bool'];"; \
		for i in $(UNIT_SCRIPT_LIST); do \
			echo "import(false,'$$i');"; \
		done; \
	} > $@

.PHONY: unit unit-proxy unit2
unit-proxy: $(f-s2sh.BIN) $(UNIT_GENERATED)
	@for i in $(UNIT_SCRIPTS_ALL); do \
		cmd="$(UNIT_RUN_CMD) -f $$i"; \
		echo "****************************** Script [$$i]"; \
		echo $$cmd; $$cmd || exit $$?; \
		echo "****************************** Done [$$i]"; \
	done
	@echo "Done running through unit test scripts."

unit: UNIT_SCRIPTS_ALL:=$(UNIT_SCRIPT_LIST) $(UNIT_GENERATED)
unit: unit-proxy
unit2: UNIT_SCRIPTS_ALL:=$(filter unit2/%,$(UNIT_SCRIPT_LIST))
unit2: unit-proxy
.PHONY: unit-r
.PHONY: unit-rc
unit-r: S2SH.SHELL.FLAGS:=--no-init-script -norv -norc
unit-r: unit
unit-s: S2SH.SHELL.FLAGS:=--no-init-script -rc -rc -nosi
unit-s: unit
unit-rc: S2SH.SHELL.FLAGS:=--no-init-script -norv -norc
unit-rc: unit
unit-rsc: S2SH.SHELL.FLAGS:=--no-init-script -norv -norc -nosi
unit-rsc: unit
units:
	@for i in unit unit-r unit-rc unit-s unit-rsc; do \
		echo "Making $$i ..."; \
		$(MAKE) $$i || exit $$?; \
	done
include vg.make
