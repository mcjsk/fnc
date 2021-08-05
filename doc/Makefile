#!/usr/bin/make -f
include ../subdir-inc.make

.NOTPARALLEL: # stop (make doc upload) from racing

OUT_PREFIX := libfossil-API-
clean-doxy:
	-rm -fr $(OUT_PREFIX)*

DISTCLEAN_FILES += Doxygen
clean: clean-doxy

Doxyfile: Doxyfile.in
	@echo "ERROR: need to reconfigure to create Doxyfile!"; \
	exit 1

doc doxy: Doxyfile
	@echo "Generating API docs..."
	@doxygen
	@echo "Output is in: "; ls -1td $(OUT_PREFIX)*

all:
	@echo "Run 'make doc' to generate the API docs with doxygen."

upload:
	cd libfossil-API-html || exit $$?; \
	rsync -rzq -e ssh --delete . wh:www.f/doxygen/libfossil/
