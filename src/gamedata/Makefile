# gamedata/Makefile: build instructions for the game data.
#
# The game data is a binary representation of the initial deck
# ordering for each game. The game data is built from a text
# representation via a short C program.
#
# Since this program is separate and standalone, this makefile is kept
# separate from the rest of the process so that it doesn't need to
# inherit the main program's build system.

.PHONY: clean cclean

# If we are building is a cross-compiling environment, switch back to
# the host environment for this process.
ifdef HOST_PATH
  PATH = $(HOST_PATH)
  CC = $(HOST_CC)
  CFLAGS = $(HOST_CFLAGS)
endif
CC = gcc
CFLAGS = -Wall -Wextra

# The game data is built by the program reading the input data.
gamedata.bin: gamedata.txt mkgames
	./mkgames $< $@

mkgames: mkgames.c

clean:
	rm -f mkgames gamedata.bin

cclean:
	rm -f mkgames
