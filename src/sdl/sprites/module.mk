# sdl/sprites/Makefile

# sdl/alerts.png contains all the graphics used as alerts and general
# displays of program state.

ALERTSLAYOUT := sdl/sprites/alerts.txt
ALERTSOUTPUT := sdl/alerts.png sdl/alertids.h sdl/alertpos.h
ALERTIMGLIST := \
    $(shell sed -n 's,^\([^\#][^ ]*\).*,sdl/sprites/\1.png,p' $(ALERTSLAYOUT))

$(ALERTSOUTPUT): $(ALERTSLAYOUT) $(ALERTIMGLIST) sdl/sprites/mkimage.py
	sdl/sprites/mkimage.py sdl/sprites $(ALERTSOUTPUT) < $<

# sdl/bicons.png contains all the graphics used on pushbuttons.

BICONSLAYOUT := sdl/sprites/bicons.txt
BICONSOUTPUT := sdl/bicons.png sdl/biconids.h sdl/biconpos.h
BICONIMGLIST := \
    $(shell sed -n 's,^\([^\#][^ ]*\).*,sdl/sprites/\1.png,p' $(BICONSLAYOUT))

$(BICONSOUTPUT): $(BICONSLAYOUT) $(BICONIMGLIST) sdl/sprites/mkimage.py
	sdl/sprites/mkimage.py sdl/sprites $(BICONSOUTPUT) < $<
