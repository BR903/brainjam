# sdl/sprites/module.mk: build rules for the sprite sheet image.
#
# Despite appearances, this subdirectory is not actually a separate
# module. Its output is an image file (and some headers). It is in its
# own subdirectory simply to reduce clutter.

# The output from this directory are a PNG and two header files.
SPRITEOUTPUTS := sdl/images.png sdl/imageids.h sdl/layout.h

# The data file giving the placement of all of the individual sprites.
SPRITELAYOUT := sdl/sprites/layout.txt

# Parse the layout file to generate the list of image dependencies.
SPRITELIST := \
    $(shell sed -n 's,^\([a-z][^ ]*\).*,sdl/sprites/\1.png,p' $(SPRITELAYOUT))

# The outputs of these build rules are not included in the list of
# generated resources, and they will not be removed by "make clean".
# This is done to avoid having the build process require the python
# image library to be installed. But if you do wish to have them
# treated the same as other generated resources, uncomment the
# following lines.
#GENRES += $(SPRITEOUTPUTS)
#SRCRES += $(SPRITELIST) sdl/sprites/mkimage.py

# Compile the individual images into a single sprite sheet.
$(SPRITEOUTPUTS): $(SPRITELAYOUT) $(SPRITELIST) sdl/sprites/mkimage.py
	sdl/sprites/mkimage.py sdl/sprites $(SPRITEOUTPUTS) < $<
