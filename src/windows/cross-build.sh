#!/bin/sh
#
# Usage: cross-build.sh [OPTIONS] VERB [ARG ...]
#
# VERB can be "configure" to run an autotools-based ./configure script,
# "make" to run a recipe from a makefile, or "check" to just print out
# the values that the script will use for the target and prefix.
#
# This script makes a number of semi-educated guesses about where the
# cross-compiling tools should be located on your filesystem. It is by
# no means widely tested. Feel free to fiddle with it to make it work.

#
# Some functions to display text and exit.
#

# Usage output.
usage ()
{
  echo "Usage: cross-build.sh [OPTIONS] configure [CONFIGURE-PARAM ...]"
  echo "       cross-build.sh [OPTIONS] make [MAKE-PARAM ...]"
  echo "       cross-build.sh [OPTIONS] exec [CMD PARAM...]"
  echo "       cross-build.sh [OPTIONS] check"
  echo "Prepare the environment for cross-compiling using mingw32 tools."
  echo ""
  echo "  -m32                  Build a 32-bit target"
  echo "  --target=TARGET       Force the target to TARGET"
  echo "  --prefix=PREFIX       Force the path prefix to PREFIX"
  echo "  --help                Display this help and exit"
  echo "  --version             Display the version information and exit"
  echo ""
  echo "After any options to cross-build.sh should be the verb \"configure\","
  echo "\"make\", \"exec\", or \"check\". This tool should be run from the"
  echo "directory containing the configure script or makefile, as per usual."
  echo "The verb \"exec\" runs an arbitrary shell command. The verb \"check\""
  echo "just validates the target and prefix, and exits."
  exit
}

# Version number.
version ()
{
  echo "cross-build.sh version 1.0"
  exit
}

# Function to display an error message and quit.
fail ()
{
  echo "$@" >&2
  exit 1
}

#
# The script proper.
#

# Parse the command-line options. (Arguments on the command line after
# the verb are passed to the executed tool.)
while test $# -gt 0 ; do
  case "$1" in
    -m32) proc=i686 ;;
    -m64) proc=x86_64 ;;
    --target=*) TARGET="${1#--target=}" ;;
    --prefix=*) prefix="${1#--prefix=}" ;;
    --help) usage ;;
    --version) version ;;
    --) ;;
    -*) fail "invalid option: \"$1\"" ;;
    *) break
  esac
  shift
done

# The first argument after the options is the verb.
test $# -eq 0 && fail "no verb; nothing to do."
verb="$1"
shift

# If the user specified a prefix but not a target, then find a
# target from the directory names.
if test "$prefix" != "" && test -z "$TARGET" ; then
  for dir in $prefix/$proc*mingw* ; do
    TARGET="${dir#$prefix/}"
    break
  done
  test -z "$proc" && proc=${TARGET%%-*}
fi

# A 64-bit target requires a 64-bit host.
if test "$proc" = "x86_64" && test "$(uname -m)" = "x86_64" ; then
  fail "cannot build a 64-bit target"
fi

# Finalize values for TARGET and BUILD.
test -z "$proc" && proc="$(uname -m)"
test -z "$TARGET" && TARGET="${proc}-w64-mingw32"
BUILD="${proc}-linux"

# Determine the prefix for the cross-compilation tools. The tools are
# typically in a system directory named after the target architecture.
# If none of the usual locations pan out, try looking for a set of
# older tools, based on the MSVC libraries, in /usr/local/cross-tools.
if test -z "$prefix" ; then
  for dir in /usr /opt /usr/local /opt/local ; do
    if test -d "$dir/$TARGET" ; then
      prefix="$dir/$TARGET"
      break
    fi
  done
fi
if test -z "$prefix" ; then
  if test -d /usr/local/cross-tools ; then
    for dir in /usr/local/cross-tools/*mingw* ; do
      prefix="/usr/local/cross-tools"
      TARGET="${dir%$prefix/}"
      PATH="$prefix/$TARGET/bin:$PATH"
      export MSVCPREFIX="$prefix/$TARGET"
      break
    done
  fi
fi
test -z "$prefix" && fail "unable to find cross-compilation tools."

# Set the environment.
PATH="$prefix/bin:$PATH"
export PATH
export CC="$TARGET-gcc -static-libgcc"
export CXX="$TARGET-g++ -static-libgcc"
export WINDRES="$TARGET-windres"
if test -d "$prefix/lib/pkgconfig" ; then
  export PKG_CONFIG_PATH="$prefix/lib/pkgconfig"
fi

# Do the requested thing.
case "$verb" in
  configure | ./configure)
    cache="$TARGET-config.cache"
    ./configure --target="$TARGET" --host="$TARGET" --build="$BUILD" \
                --prefix="$prefix" \
                --cache-file="$cache" "$@"
    result=$?
    rm -f "$cache"
    exit $result
    ;;
  make)
    exec ${MAKE-make} "$@"
    ;;
  exec)
    exec "$@"
    ;;
  check)
    echo "Target: $TARGET"
    echo "Prefix: $prefix"
    exit 0
    ;;
  *)
    fail "unrecognized action: \"$verb\"."
esac
