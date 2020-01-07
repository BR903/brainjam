# sdl/module.mk: build rules for the sdl module.

ifdef WITH_SDL

SRC += sdl/sdl.c sdl/zrwops.c sdl/resource.S sdl/getfont.c
SRC += sdl/images.c sdl/button.c sdl/scroll.c sdl/help.c sdl/list.c sdl/game.c
GENRES += $(shell sed -n 's/^\.incbin *\"\(.*\)\"/\1/p' sdl/resource.S)
SRCRES += $(GENRES:.bmp.gz=.png) sdl/abmp.py

override CFLAGS += $(shell $(SDL2_CONFIG) --cflags)
override CFLAGS += $(shell $(PKG_CONFIG) --cflags SDL2_ttf zlib)
override LDLIBS += $(shell $(SDL2_CONFIG) --libs)
override LDLIBS += $(shell $(PKG_CONFIG) --libs SDL2_ttf zlib)

# Skip this if the fontconfig library is not being used.
ifdef WITH_FONTCONFIG
override CFLAGS += -D_WITH_FONTCONFIG
override CFLAGS += $(shell $(PKG_CONFIG) --cflags fontconfig)
override LDLIBS += $(shell $(PKG_CONFIG) --libs fontconfig)
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
