# ./module.mk: build rules for the top-level module.

SRC += brainjam.c gen.c decks.c settings.c ui.c

# mkgames creates a compressed binary representation of the full set
# of available games.
gamedata/gamedata.bin: gamedata/gamedata.txt gamedata/mkgames.c
	$(MAKE) -C gamedata