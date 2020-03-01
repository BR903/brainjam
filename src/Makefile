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

.PHONY: all install clean cclean

# Define the configuration symbols.
-include ./cfg.mk

# The name of the game.
PROG := $(NAME)

# The default target builds the program.
all: $(PROG)

#
# Build tools.
#

# The default build tool options are pretty tame.
CPPFLAGS := 
CFLAGS := -Wall -Wextra
LDFLAGS := -Wall -Wextra
LDLIBS := 

# If debugging symbols are requested, add the necessary options.
# Otherwise, turn on optimization.
ifdef ENABLE_DEBUG
override CFLAGS += -g -O0
override LDFLAGS += -g
else
override CFLAGS += -O2
override LDFLAGS += -s
endif

#
# Modules.
#

# Each directory (including the top-level directory) is a module that
# makes up the complete program.
MODULES := . solutions files game redo curses sdl

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

#
# Dependency generation.
#

# The complete list of object files is generated from the source file list.
OBJ := $(patsubst %.c,%.o,$(filter %.c,$(SRC))) \
       $(patsubst %.S,%.o,$(filter %.S,$(SRC)))

# Some extra tasks for making a windows build.
ifdef ENABLE_WINDOWS
include windows/windows.mk
endif

# The dependency include files are built from the source files.
%.d: %.c
	./depend.sh $< $@ $(CC) $(CPPFLAGS) $(CFLAGS)

# Include the dependency data. (The dash prefix suppresses error
# messages when the .d files haven't been generated yet.)
-include $(OBJ:.o=.d)

# Special rule that copies the version number to a C header file.
./version.h: ./cfg.mk
	sed -n 's/^VERSION := \(.*\)/#define VERSION_ID \"\1\"/p' $^ > $@

#
# The top-level build rules.
#

# The program is built from the object files.
$(PROG): $(OBJ)

# The install rule copies the program and the man page.
install: $(PROG)
	install -d $(bindir)
	install -d $(mandir)/man6
	install ./$(PROG) $(bindir)
	install ./$(NAME).6 $(mandir)/man6

# The clean rule deletes the program, object files, dependency files,
# and generated resources.
clean:
	rm -f $(PROG) $(OBJ) $(OBJ:.o=.d) $(GENRES) ./version.h
	$(MAKE) -C gamedata $@

# Alternately, the cclean rule only removes the program and object files.
cclean:
	rm -f $(PROG) $(OBJ)
	$(MAKE) -C gamedata $@
