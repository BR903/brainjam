# test/Makefile: build instructions for the unit tests.
#
# The test program exercises the functions in game/state.c, ensuring
# that its functions never create an inconsistent state in the
# gameplayinfo fields.
#
# The test program takes advantage of the fact that the code it is
# testing has a limited set of dependencies on other modules, and so
# it can link the test code with just those dependencies (and the code
# actually being tested, of course).

CC := gcc
CFLAGS := -Wall -Wextra -I..
LDFLAGS := -Wall

PROG := chklogic

# Since this makefile is not really part of the rest of the build
# system, it depends on the external object files having already been
# built before it is invoked. All we really want is game/state.o, but
# the others are required for game/state.o to link.
EXTOBJS :=../game/state.o ../game/game.o ../decks.o ../gen.o ../redo/redo.o

.PHONY: check clean cclean

check: $(PROG)
	./$(PROG)

$(PROG): $(PROG).o $(EXTOBJS)

clean:
	rm -f $(PROG) $(PROG).o

cclean: clean
