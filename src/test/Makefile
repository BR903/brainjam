# test/Makefile: build instructions for the unit tests.
#
# The test program contains a (overlong) solution for one of the
# games. When run it will play through the solution, verifying that
# the game state never enters an inconsistent state.
#
# The test program takes advantage of the fact that the code it is
# testing has a limited set of dependencies on the rest of the code,
# and so it can link the test code with just those dependencies (and
# the code actually being tested, of course).
#
# Since this makefile is not really part of the rest of the build
# system, it depends on the external object files having already been
# built before it is invoked.

CC := gcc
CFLAGS := -Wall -Wextra -I..
LDFLAGS := -Wall

PROG := chklogic
EXTOBJS := ../redo/redo.o ../game/game.o ../game/state.o ../decks.o ../gen.o

.PHONY: check clean cclean

check: $(PROG)
	./$(PROG)

$(PROG): $(PROG).o $(EXTOBJS)

clean:
	rm -f $(PROG) $(PROG).o

cclean: clean
