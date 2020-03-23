#!/bin/bash
#
# Usage: mkdist.sh DIST
#
# Note that the windows distributions require the presence of a
# cross-compiling toolchain. The windows installer distributions
# furthermore require the NSIS compiler to be installed.

set -e

# The name of the distribution.
#
PROG=brainjam

# The command to launch the NSIS compiler.
#
MAKENSIS='wine C:\NSIS\makensis.exe'

# Usage information.
#
usage ()
{
  echo "Usage: mkdist DIST"
  echo "Build a distribution of $PROG."
  echo "DIST may be one of the following values:"
  echo ""
  echo "    source      build the basic source tarball"
  echo "    homebrew    build the homebrew formula"
  echo "    win32       build the zipfile of 32-bit windows binaries"
  echo "    win64       build the zipfile of 64-bit windows binaries"
  echo "    winstall32  build the 32-bit windows installer program"
  echo "    winstall64  build the 64-bit windows installer program"
  exit
}

# Function to display an error message and quit.
#
fail ()
{
  echo "mkdist.sh:" "$@" >&2
  exit 1
}

# Build the source tarball. The MANIFEST file contains the complete
# list of files to include.
#
mksrc ()
{
  dir=${PROG}-${VER}
  tarball=dists/$tarfile
  mkdir $dir
  tar -cf- $(cat dists/MANIFEST) | tar -C $dir -xf-
  rm -f $tarball
  tar -czvf $tarball $dir
  rm -r $dir
}

# Build the homebrew formula. This will force the source tarball to be
# created if it doesn't already exit.
#
mkformula ()
{
  formula=dists/${PROG}.rb
  if ! test -f "dists/$tarfile" ; then
    mksrc
  fi
  sha=$(sha256sum "dists/$tarfile" | sed 's/ .*//')
  sed "s/TARFILE/$tarfile/;s/TARSHA/$sha/" <$formula.in >$formula
}

# Build a windows binary distribution. The argument sets whether to
# build for 32-bit windows or 64-bit windows.
#
mkwindows ()
{
  bits=$1
  dir=$PROG
  zipdist=dists/${PROG}-${VER}-win${bits}.zip
  wrapper=./src/windows/cross-build.sh
  make clean
  ./src/windows/cross-build.sh -m$bits configure --enable-windows
  ./src/windows/cross-build.sh -m$bits make
  mkdir $dir
  cp src/${PROG}.exe $dir/
  ./src/windows/cross-build.sh -m$bits make clean
  dlls='libfreetype-6.dll SDL2_ttf.dll SDL2.dll libpng16-16.dll zlib1.dll'
  dllpaths=$(./src/windows/cross-build.sh -m$bits bash type -fp $dlls)
  cp $dllpaths $dir/
  sed 's/$/\r/' ./README >$dir/README.txt
  rm -f $zipdist
  zip -r $zipdist $dir
  rm -r $dir
}

# Build a windows installer. The argument sets whether to build the
# installer with 32-bit binaries or 64-bit binaries. This will force
# the windows distribution to be built if it hasn't already. This
# script invokes the install script compiler, which it is why it is
# broken out separately from the binary distribution. (Note that the
# installer is always created as a 32-bit program, regardless of the
# binaries that it will install.)
#
mkinstaller ()
{
  bits=$1
  zipfile=${PROG}-${VER}-win${bits}.zip
  if ! test -f "dists/$zipfile" ; then
    mkwindows $bits
  fi
  unzip -d dists "dists/$zipfile"
  dir=dists/$PROG
  cp dists/install.nsi $dir/
  ${MAKENSIS:-makensis} -DVER=$VER $dir/install.nsi
  mv $dir/install.exe dists/install-${PROG}-win${bits}.exe
  rm -r $dir
}

#
# Top-level.
#

# All distributions need to know the prorgram's version ID. This is
# pulled from the configure.ac file, since it's guaranteed to always
# be present.
#
if test -f ./configure.ac ; then
  VER=$(sed -n 's/^AC_INIT[^,]*, *\[\([0-9a-z.]*\)\].*/\1/p' ./configure.ac)
else
  fail "configure.ac not found."
fi
test "x$VER" != "x" || fail "couldn't determine version number."

# A few variables are used in more than one recipe.
#
tarfile=${PROG}-${VER}.tar.gz

# Run the requested recipe. 
#
test $# -eq 1 || fail "Invalid arguments. Try --help for more information."
case "$1" in
  source|src|tarball)      mksrc ;;
  brew|homebrew|formula)   mkformula ;;
  win32|windows32)         mkwindows 32 ;;
  win64|windows)           mkwindows 64 ;;
  winstall32)              mkinstaller 32 ;;
  winstall64)              mkinstaller 64 ;;
  --help)                  usage ;;
  --version)               echo "version 2" && exit ;;
  *)                       fail "Unknown distribution type: \"$which\"." ;;
esac

echo
echo done.
