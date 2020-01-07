# curses/module.mk: build rules for the curses module.

ifdef WITH_NCURSES

SRC += curses/curses.c curses/list.c curses/game.c curses/help.c
CFLAGS += $(shell $(PKG_CONFIG) --cflags ncursesw)
LDLIBS += $(shell $(PKG_CONFIG) --libs ncursesw)

else

# An empty module to use if ncurses support is removed.
SRC += curses/stub.c

endif
