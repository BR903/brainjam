# sdl/sprites/Makefile

# sdl/alerts.png contains all the graphics used as alerts and general
# displays of program state.

ALERTSLAYOUT := sdl/sprites/alerts.txt
ALERTSOUTPUT := sdl/alerts.png sdl/alertids.h sdl/alertpos.h
ALERTIMGLIST := \
    $(shell sed -n 's,^\([^\#][^ ]*\).*,sdl/sprites/\1.png,p' $(ALERTSLAYOUT))

$(ALERTSOUTPUT): $(ALERTSLAYOUT) $(ALERTIMGLIST) sdl/sprites/mkimage.py
	sdl/sprites/mkimage.py sdl/sprites $(ALERTSOUTPUT) < $<

# sdl/labels.png contains all the graphics used on pushbuttons.

LABELSLAYOUT := sdl/sprites/labels.txt
LABELSOUTPUT := sdl/labels.png sdl/labelids.h sdl/labelpos.h
LABELIMGLIST := \
    $(shell sed -n 's,^\([^\#][^ ]*\).*,sdl/sprites/\1.png,p' $(LABELSLAYOUT))

$(LABELSOUTPUT): $(LABELSLAYOUT) $(LABELIMGLIST) sdl/sprites/mkimage.py
	sdl/sprites/mkimage.py sdl/sprites $(LABELSOUTPUT) < $<
