# gamedata/module.mk: build rules for the gamedata module.

RES += gamedata/gamedata.bin

# mkgames.py creates a compressed binary representation of the full
# set of available games.
gamedata/gamedata.bin: gamedata/gamedata.txt gamedata/mkgames.py
	gamedata/mkgames.py $< $@

