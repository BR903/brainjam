# sdl/sprites/Makefile

ALERTSLAYOUT := sdl/sprites/alerts.txt
ALERTSOUTPUT := sdl/alerts.png sdl/alertids.h sdl/alertpos.h
ALERTIMGLIST := \
    $(shell sed -n 's,^\([^\#][^ ]*\).*,sdl/sprites/\1.png,p' $(ALERTSLAYOUT))

BICONSLAYOUT := sdl/sprites/bicons.txt
BICONSOUTPUT := sdl/bicons.png sdl/biconids.h sdl/biconpos.h
BICONIMGLIST := \
    $(shell sed -n 's,^\([^\#][^ ]*\).*,sdl/sprites/\1.png,p' $(BICONSLAYOUT))

$(ALERTSOUTPUT): $(ALERTSLAYOUT) $(ALERTIMGLIST) sdl/sprites/mkimage.py
	sdl/sprites/mkimage.py sdl/sprites $(ALERTSOUTPUT) < $<

$(BICONSOUTPUT): $(BICONSLAYOUT) $(BICONIMGLIST) sdl/sprites/mkimage.py
	sdl/sprites/mkimage.py sdl/sprites $(BICONSOUTPUT) < $<
