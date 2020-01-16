#!/bin/sh
#
# Build the distributions.

set -e

# Extract the current version number.
VERSION=$(sed -n "s/^PACKAGE_VERSION='\(.*\)'$/\1/p" ./configure)

# Use the version number in the names of all the things.
DIR=brainjam-${VERSION}
TARFILE=brainjam-${VERSION}.tar.gz
ZIPFILE=brainjam-${VERSION}-windows.zip

echo
echo "* Building the source tarball distribution ..."
echo

mkdir $DIR
tar -cf- $(cat ./MANIFEST) | tar -C $DIR -xf-
rm -f $TARFILE
tar -czvf $TARFILE $DIR
rm -r $DIR

echo
echo "* Building the Windows binary distribution ..."
echo

# If the brainjam executable does not exist, or is not a Windows
# binary, then build it from scratch.
if test ! -f src/brainjam || $(file src/brainjam | grep -q ELF) ; then
  make clean
  ./src/windows/cross-build.sh configure --enable-windows
  ./src/windows/cross-build.sh make
fi

# Copy the program and the necessary DLLs into a directory and zip it up.
mkdir $DIR
cp -a src/brainjam $DIR/brainjam.exe
cp -a ./README $DIR/.
cp -a $(./src/windows/cross-build.sh exec /bin/bash -c \
           'type -fp libfreetype-6.dll SDL2_ttf.dll SDL2.dll zlib1.dll') $DIR/.
rm -f $ZIPFILE
zip -r $ZIPFILE $DIR
rm -r $DIR

echo
echo "* Done."
echo
ls -lh $TARFILE $ZIPFILE
echo
