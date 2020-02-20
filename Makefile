# Project makefile for Brain Jam.
#
# This makefile is mostly provided as a convenience, to allow "make"
# and "make install" work when run from the project root directory.
# The real makefile is under the src directory.
#
# The list of targets:
#
# make [all]     = build the program binary
# make install   = install the program and man page
# make clean     = delete all files created by the build process
# make distclean = delete files created by the build and configure processes

.PHONY: all install clean distclean

# The usual targets are delegated to the src makefile.
all install clean:
	$(MAKE) -C src $@

# "make distclean" also cleans up the detritus left behind by ./configure.
distclean: clean
	rm -rf autom4te.cache config.* src/cfg.mk
