dnl * configure.in: The autoconf configuration script.
dnl *

AC_INIT([Brain Jam], [0.7], [breadbox@muppetlabs.com], [brainjam],
        [http://www.muppetlabs.com/~breadbox/software/brainjam.html])


dnl * Validate the C environment.
dnl *

AC_PROG_CC
AC_HEADER_STDC
AC_CHECK_FUNC(getopt, [],
              [AC_MSG_ERROR([could not find the getopt() library function.])])


dnl * Verify that our other build tools are present.
dnl *

AC_CHECK_PROG(check_prog, [sed], [yes], [no])
if test "$check_prog" != "yes" ; then
  AC_MSG_ERROR([could not find sed on the path.])
fi

AC_CHECK_PROG(check_prog, [gzip], [yes], [no])
if test "$check_prog" != "yes" ; then
  AC_MSG_ERROR([could not find the gzip utility.])
fi

AC_CHECK_PROG(check_prog, [python], [yes], [no])
if test "$check_prog" != "yes" ; then
  AC_MSG_ERROR([could not find the python interpreter.])
fi

AC_CHECK_PROG(check_prog, [pkg-config], [yes], [no])
if test "$check_prog" != "yes" ; then
  AC_MSG_ERROR([could not find the pkg-config utility.])
fi


dnl * The option --enable-debug creates a debug build.
dnl *

AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug], [Build with debugging info]),
	      [:], [enable_debug=no])
if test $enable_debug = no ; then
  DEBUG_LINE="#ENABLE_DEBUG=1"
else
  DEBUG_LINE="ENABLE_DEBUG=1"
fi


dnl * Define the options that create library dependencies.
dnl *

AC_ARG_WITH(ncurses,
            AC_HELP_STRING([--with-ncurses], [Link with ncursesw (default)]),
	    [], [with_ncurses=check])
AC_ARG_WITH(sdl,
            AC_HELP_STRING([--with-sdl], [Link with SDL2 (default)]),
	    [], [with_sdl=check])
AC_ARG_WITH(fontconfig,
         AC_HELP_STRING([--with-fontconfig], [Link with fontconfig (default)]),
	    [], [with_fontconfig=check])


dnl * Check for the presence of dependent libraries.
dnl *

if test "$with_ncurses" != "no" ; then
  AC_CHECK_LIB([ncursesw], [initscr], [with_ncurses=yes],
               [if test "$with_ncurses" = "check" ; then
                  with_ncurses=no
                else
                  AC_MSG_ERROR([the ncursesw library was not found.])
                fi])
fi

if test "$with_sdl" != "no" ; then
  AC_CHECK_LIB([SDL2], [SDL_Init], [with_sdl=yes],
               [if test "$with_sdl" = "check" ; then
                  with_sdl=no
                else
                  AC_MSG_ERROR([the SDL2 library was not found.])
                fi])
fi

if test "$with_sdl" = "no" ; then
  if test "$with_fontconfig" = "yes" ; then
    AC_MSG_WARN([fontconfig is only used with SDL; ignoring with-fontconfig.])
  fi
  with_fontconfig=no
fi

if test "$with_fontconfig" != "no" ; then
  AC_CHECK_LIB([fontconfig], [FcInit], [with_fontconfig=yes],
               [if test "$with_fontconfig" = "check" ; then
                  with_fontconfig=no
                else
                  AC_MSG_ERROR([the fontconfig library was not found.])
                fi])
fi


dnl * Ensure that the requested dependencies are functional.
dnl *

if test "$with_ncurses" = "yes" ; then
  pkg-config ncursesw || AC_MSG_ERROR([the ncursesw library was not found.])
fi

if test "$with_sdl" = "yes" ; then
  pkg-config sdl2 || AC_MSG_ERROR([the SDL2 library could not be found.])
  pkg-config SDL2_ttf || AC_MSG_ERROR([the SDL2_ttf library was not found.])
  pkg-config zlib || AC_MSG_ERROR([zlib could not be found.])
fi

if test "$with_fontconfig" = "yes" ; then
  pkg-config fontconfig || AC_MSG_ERROR([libfontconfig was not found.])
fi


dnl * Translate the flag variables into makefile lines.
dnl *

if test "$with_ncurses" = "yes" ; then
  NCURSES_LINE="WITH_NCURSES=1"
else
  NCURSES_LINE="#WITH_NCURSES=1"
fi

if test "$with_sdl" = "yes" ; then
  SDL_LINE="WITH_SDL=1"
else
  SDL_LINE="#WITH_SDL=1"
fi

if test "$with_fontconfig" = "yes" ; then
  FONTCONFIG_LINE="WITH_FONTCONFIG=1"
else
  FONTCONFIG_LINE="#WITH_FONTCONFIG=1"
fi


dnl * Output the configuration.
dnl *

AC_CONFIG_FILES(src/cfg.mk:cfg.mk.in)

AC_SUBST(SDL2_CONFIG)
AC_SUBST(DEBUG_LINE)
AC_SUBST(NCURSES_LINE)
AC_SUBST(SDL_LINE)
AC_SUBST(FONTCONFIG_LINE)

AC_OUTPUT
