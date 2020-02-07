# sdl/module.mk: build rules for the sdl module.

ifdef WITH_SDL

SDLIMAGES = sdl/banner.png sdl/cardset.png sdl/headline.png sdl/images.png

SRC += sdl/sdl.c sdl/zrwops.c sdl/font.c
SRC += sdl/images.c sdl/button.c sdl/scroll.c sdl/help.c sdl/list.c sdl/game.c
GENRES += $(SDLIMAGES:.png=.bmp.gz)
SRCRES += $(SDLIMAGES) sdl/abmp.py

override CFLAGS += $(shell pkg-config --cflags sdl2 SDL2_ttf zlib)
override LDLIBS += $(shell pkg-config --libs sdl2 SDL2_ttf zlib)

# Skip this if the fontconfig library is not being used.
ifdef WITH_FONTCONFIG
override CFLAGS += -D_WITH_FONTCONFIG
override CFLAGS += $(shell pkg-config --cflags fontconfig)
override LDLIBS += $(shell pkg-config --libs fontconfig)
endif

# Images are stored in the binary as compressed bitmaps.
%.bmp.gz: %.png sdl/abmp.py
	sdl/abmp.py $< | gzip -9 > $@

# sdl/sprites contains the images that are built into sdl/images.png
include sdl/sprites/module.mk

else

# An empty module to use if sdl support is removed.
SRC += sdl/stub.c

endif
