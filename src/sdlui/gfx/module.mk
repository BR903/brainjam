# sdlui/gfx/module.mk: build rules for the sprite sheets.

# sdlui/alerts.png contains all the graphics used as alerts and
# general displays of program state.

ALERTSLAYOUT := sdlui/gfx/alerts.txt
ALERTSOUTPUT := sdlui/alerts.png sdlui/alertids.h sdlui/alertpos.h
ALERTIMGLIST := \
    $(shell sed -n 's,^\([^\#][^ ]*\).*,sdlui/gfx/\1.png,p' $(ALERTSLAYOUT))

$(ALERTSOUTPUT): $(ALERTSLAYOUT) $(ALERTIMGLIST) sdlui/gfx/mkimage.py
	sdlui/gfx/mkimage.py sdlui/gfx $(ALERTSOUTPUT) < $<

# sdlui/labels.png contains all the graphics used on pushbuttons.

LABELSLAYOUT := sdlui/gfx/labels.txt
LABELSOUTPUT := sdlui/labels.png sdlui/labelids.h sdlui/labelpos.h
LABELIMGLIST := \
    $(shell sed -n 's,^\([^\#][^ ]*\).*,sdlui/gfx/\1.png,p' $(LABELSLAYOUT))

$(LABELSOUTPUT): $(LABELSLAYOUT) $(LABELIMGLIST) sdlui/gfx/mkimage.py
	sdlui/gfx/mkimage.py sdlui/gfx $(LABELSOUTPUT) < $<
