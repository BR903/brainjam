# cursesui/module.mk: build rules for the cursesui module.

ifdef WITH_NCURSES

SRC += cursesui/cursesui.c cursesui/list.c cursesui/game.c cursesui/help.c

override CFLAGS += $(shell pkg-config --cflags ncursesw)
override LDLIBS += $(shell pkg-config --libs ncursesw)

else

# An empty module to use if ncurses support is removed.
SRC += cursesui/stub.c

endif
