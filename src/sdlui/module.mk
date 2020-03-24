# sdlui/module.mk: build rules for the sdlui module.

ifdef WITH_SDL

SRC += sdlui/sdlui.c sdlui/font.c sdlui/getpng.c sdlui/image.c
SRC += sdlui/button.c sdlui/scroll.c sdlui/help.c sdlui/list.c sdlui/game.c
RES += sdlui/banner.png sdlui/cardset.png sdlui/headline.png
RES += sdlui/alerts.png sdlui/labels.png

override CFLAGS += $(shell pkg-config --cflags sdl2 SDL2_ttf libpng)
override LDLIBS += $(shell pkg-config --libs sdl2 SDL2_ttf libpng)

# Skip this if the fontconfig library is not being used.
ifdef WITH_FONTCONFIG
override CFLAGS += -D_WITH_FONTCONFIG
override CFLAGS += $(shell pkg-config --cflags fontconfig)
override LDLIBS += $(shell pkg-config --libs fontconfig)
endif

# sdlui/gfx contains the images that are built into sdlui/alerts.png
# and sdlui/labels.png. It is not actually a separate module; it's
# just too messy to have its contents in the same directory.
include sdlui/gfx/module.mk

else

# An empty module to use if SDL support is removed.
SRC += sdlui/stub.c

endif
