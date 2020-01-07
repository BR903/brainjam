# configs/module.mk: build rules for the configs module.

SRC += configs/configs.c configs/resource.S
GENRES += configs/configs.bin
SRCRES += configs/configs.txt configs/mkconfigs.py

# mkconfigs.py creates a compressed binary representation containing
# the full set of available games.
configs/configs.bin: configs/configs.txt configs/mkconfigs.py
	configs/mkconfigs.py $< > $@
