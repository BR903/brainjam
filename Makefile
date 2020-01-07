# ./Makefile: the top-level build instructions.
#
# This makefile sets up the variables used by the build rules, and
# then allows each module to append to them and add its own rules. It
# then uses the ./depend.sh script to build the dependency data.
#
# The list of top-level targets:
#
# make [all]   = build the program binary
# make install = install the program
# make clean   = delete all files created by the build process
# make cclean  = delete created object files but keep created data files

.PHONY: all clean cclean install

# Define the configuration symbols.
include ./config.mk

# The name of the game.
PROG := $(NAME)

# The default target builds the program.
all: $(PROG)

#
# Build tools.
#

# The tools that are used to build the program.
CC := gcc
PKG_CONFIG := pkg-config
SDL2_CONFIG := sdl2-config

# The default build tool options are pretty tame.
CPPFLAGS := 
CFLAGS := -Wall -Wextra
ASFLAGS := -Wall -Wextra
LDFLAGS := -Wall -Wextra
LDLIBS := 

# If debugging symbols are requested, add the necessary options.
# Otherwise, turn on optimization.
ifdef ENABLE_DEBUG
override CFLAGS += -ggdb -Og
override ASFLAGS += -ggdb
override LDFLAGS += -ggdb
else
override CFLAGS += -O2
override LDFLAGS += -s
endif

#
# Modules.
#

# Each directory (including the top-level directory) is a module that
# makes up the complete program.
MODULES := . configs solutions files game redo curses sdl

# Source files in all modules are built from this directory.
override CPPFLAGS += -I.

# The lists of source files and resource files, initially empty.
SRC :=
GENRES :=

# Every module provides an include file, called module.mk, which adds
# their own files to these lists (and optionally specifies other
# configuration info, such as adding to CFLAGS or defining special
# build rules for generated resources).
include $(patsubst %,%/module.mk,$(MODULES))

# The complete list of object files is generated from the source file list.
OBJ := $(patsubst %.c,%.o,$(filter %.c,$(SRC))) \
       $(patsubst %.S,%.o,$(filter %.S,$(SRC)))

#
# The build rules.
#

# The dependency include files are built from the source files.
%.d: %.c
	./depend.sh $< $@ $(CPPFLAGS) $(CFLAGS)
%.d: %.S
	./depend.sh $< $@ $(CPPFLAGS) $(ASFLAGS)

# Include the dependency data. (The dash prefix suppresses error
# messages when the .d files haven't been generated yet.)
-include $(OBJ:.o=.d)

# The program is built from the object files.
$(PROG): $(OBJ)

# The install rule copies the program and the man page.
install: $(PROG)
	install ./$(PROG) $(bindir)
	install ./$(NAME).6 $(mandir)/man6

# The clean rule deletes the program, object files, dependency files,
# and generated resources.
clean:
	rm -f $(PROG) $(OBJ) $(OBJ:.o=.d) $(GENRES)

# Alternately, the cclean rule only removes the program and object files.
cclean:
	rm -f $(PROG) $(OBJ)

# This won't update automatically; replace with something better.
./version.h:
	echo '#define VERSION_ID "$(VERSION)"' > $@
