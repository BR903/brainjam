# test/Makefile: build instructions for the unit tests.
#
# The test program exercises the functions in game/state.c, ensuring
# that its functions never create an inconsistent state in the
# gameplayinfo fields.
#
# The test program takes advantage of the fact that the code it is
# testing has a limited set of dependencies on other modules, and so
# it can link the test code with just those dependencies.

CC := gcc
CFLAGS := -Wall -Wextra -I..
LDFLAGS := -Wall

PROG := runtests

# The list of object files containing unit tests. Each of these
# corresponds to a C file that contains a single function of the same
# name that runs the unit tests, asserting if any tests fail.
OBJ := chklogic.o chkredo.o

# Since this makefile is not really part of the rest of the build
# system, it depends on the external object files having already been
# built before it is invoked. All we really want is game/state.o, but
# the others are required for game/state.o to link.
EXTOBJ := ../game/state.o ../game/game.o ../decks.o ../gen.o ../redo/redo.o

.PHONY: check clean cclean

check: $(PROG)
	./$(PROG)

$(PROG): $(PROG).c $(OBJ) $(EXTOBJ)

# Since running the tests just requires calling the lone extern
# function in each test suite, the main() function is generated from
# the list of object files.
$(PROG).c:
	echo $(patsubst %.o,"extern void %(void);",$(OBJ)) > $@
	echo "int main(void){" >> $@
	echo $(patsubst %.o,"%();",$(OBJ)) >> $@
	echo "return 0;}" >> $@

clean:
	rm -f $(PROG) $(PROG).c $(OBJ)

cclean: clean
