# gamedata/Makefile: build instructions for the game data.
#
# The game data is a binary representation of the initial deck
# ordering for each game. The game data is built from a text
# representation using a short standalone C program.
#
# Note that this makefile is not part of the external build system.
# Since it builds a standalone program, is run as its own process.

CC := gcc
CFLAGS := -Wall -Wextra

.PHONY: all clean cclean

all: gamedata.bin

gamedata.bin: gamedata.txt mkgames
	./mkgames $< $@

mkgames: mkgames.c

clean:
	rm -f mkgames gamedata.bin

cclean:
	rm -f mkgames
