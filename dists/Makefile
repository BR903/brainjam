# This Makefile is here mainly to provide symbolic names for the
# various distributions, so that the user doesn't have to specify
# actual file names on the command line.
#
# Despite its location, this Makefile assumes that it is being run
# from the top-level directory.

DISTS := source homebrew win32 win64

.PHONY: all $(DISTS) clean

all: $(DISTS)

source homebrew:
	dists/mkdist.sh $@

win32:
	dists/mkdist.sh win32
	dists/mkdist.sh winstall32

win64:
	dists/mkdist.sh win64
	dists/mkdist.sh winstall64
