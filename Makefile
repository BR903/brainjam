# Project makefile for Brain Jam.
#
# This makefile is mostly provided as a convenience, to allow "make"
# and "make install" work when run from the project root directory.
# The real makefile is under the src directory. That said, some extra
# targets are provided natively by this makefile.
#
# The list of targets:
#
# make [all]    = build the program binary
# make install  = install the program and man page
# make dist     = build the distribution tarball
# make clean    = delete all files created by the configuration & build process
# make spotless = delete all files created from other files

.PHONY: all install dist clean spotless

# The configuration include file defines NAME and VERSION.
include src/cfg.mk

# "make all" is delegated to the src makefile.
all:
	$(MAKE) -C src $@

# "make install" is delegated to the src makefile.
install:
	$(MAKE) -C src $@

# "make clean" deletes the usual files, but also files created by ./configure.
clean:
	$(MAKE) -C src $@
	rm -f src/cfg.mk

# "make spotless" wipes out everything that can be recreated.
spotless: clean
	rm -rf configure autom4te.cache config.*
	rm -f $(NAME)-*.tar.gz

# "make dist" builds the distribution tarball.
dist:
	rm -f $(NAME)-$(VERSION).tar.gz
	mkdir $(NAME)-$(VERSION)
	tar -cf- `cat MANIFEST` | tar -C $(NAME)-$(VERSION) -xf-
	tar -czf $(NAME)-$(VERSION).tar.gz $(NAME)-$(VERSION)
	rm -r $(NAME)-$(VERSION)
