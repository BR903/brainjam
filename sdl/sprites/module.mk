# sdl/sprites/module.mk: build rules for the sprite sheet image.

# The output from this directory are a PNG and two header files.
SPRITEOUTPUTS := sdl/images.png sdl/imageids.h sdl/layout.h

# The data file giving the placement of all of the individual sprites.
SPRITELAYOUT := sdl/sprites/layout.txt

# Parse the layout file to generate the list of image dependencies.
SPRITELIST := \
    $(shell sed -n 's,^\([a-z][^ ]*\).*,sdl/sprites/\1.png,p' $(SPRITELAYOUT))

GENRES += $(SPRITEOUTPUTS)
SRCRES += $(SPRITELIST) sdl/sprites/mkimage.py

# Compile the individual images into a single sprite sheet.
$(SPRITEOUTPUTS): $(SPRITELAYOUT) $(SPRITELIST) sdl/sprites/mkimage.py
	sdl/sprites/mkimage.py sdl/sprites $(SPRITEOUTPUTS) < $<
