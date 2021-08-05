#!/do/not/make # to help out emacs
# To be included from subdir GNU Makefiles.
TOP_DIR := $(patsubst %/,%,$(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))
#$(error TOP_DIR=$(TOP_DIR))
include $(TOP_DIR)/config.make
include $(TOP_DIR)/shakenmake.make

