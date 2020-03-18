# gamedata/module.mk: build rules for the gamedata module.

GENRES += gamedata/gamedata.bin gamedata/mkgames
SRCRES += gamedata/gamedata.txt

# mkgames creates a compressed binary representation of the full set
# of available games. Since it is a standalone program and not part of
# brainjam, it is built in a separate process, with its own compiler
# settings. However, the main build process still needs to manage the
# dependencies, so they are declared here.
gamedata/gamedata.bin: gamedata/gamedata.txt gamedata/mkgames.c
	$(MAKE) -C gamedata gamedata.bin
