# sdl/module.mk: build rules for the sdl module.

ifdef WITH_SDL

SRC += sdl/sdl.c sdl/font.c sdl/getpng.c
SRC += sdl/images.c sdl/button.c sdl/scroll.c sdl/help.c sdl/list.c sdl/game.c
SRCRES += sdl/banner.png sdl/cardset.png sdl/headline.png sdl/images.png

override CFLAGS += $(shell pkg-config --cflags sdl2 SDL2_ttf libpng)
override LDLIBS += $(shell pkg-config --libs sdl2 SDL2_ttf libpng)

# Skip this if the fontconfig library is not being used.
ifdef WITH_FONTCONFIG
override CFLAGS += -D_WITH_FONTCONFIG
override CFLAGS += $(shell pkg-config --cflags fontconfig)
override LDLIBS += $(shell pkg-config --libs fontconfig)
endif

# sdl/sprites contains the images that are built into sdl/images.png
include sdl/sprites/module.mk

else

# An empty module to use if sdl support is removed.
SRC += sdl/stub.c

endif
