# ./config.mk: build customization settings.

# The thing to build.
NAME := brainjam
VERSION := 0.7

# Define to include the text-terminal version of the UI.
# Undefine to remove dependency on the ncursesw library.
WITH_NCURSES=1

# Define to include the graphical version of the UI.
# Undefine to remove dependency on SDL2, SDL2_ttf, and zlib.
WITH_SDL=1

# Define to allow the program to automatically find a system font.
# Undefine to remove dependency on the fontconfig library.
WITH_FONTCONFIG=1

# Define to build with debugging symbols. Undefine to compile with
# optimizations.
ENABLE_DEBUG=1

# Change these symbols to control where files get installed.
prefix := /usr/local
exec_prefix := ${prefix}
datarootdir := ${prefix}/share
bindir := $(DESTDIR)${exec_prefix}/games
mandir := $(DESTDIR)${datarootdir}/man
