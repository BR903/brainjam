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
echo "* Building the Windows binary distribution ..."
echo

# The list of DLLs that the windows binary depends on.
export DLLS='libfreetype-6.dll SDL2_ttf.dll SDL2.dll libpng16-16.dll zlib1.dll'

# Always rebuild from scratch.
make clean
./src/windows/cross-build.sh configure --enable-windows
./src/windows/cross-build.sh make

# Copy the program and the necessary DLLs into a directory and zip it up.
mkdir $DIR
cp -a src/brainjam $DIR/brainjam.exe
cat ./README | sed 's/$/\r/' > $DIR/README.txt
cp -a $(./src/windows/cross-build.sh exec /bin/bash -c "type -fp $DLLS") $DIR/.
rm -f $ZIPFILE
zip -r $ZIPFILE $DIR
rm -r $DIR

# Return tree to default configuration.
make clean
./configure

echo
echo "* Building the source tarball distribution ..."
echo

mkdir $DIR
tar -cf- $(cat ./MANIFEST) | tar -C $DIR -xf-
rm -f $TARFILE
tar -czvf $TARFILE $DIR
rm -r $DIR

echo
echo "* Done."
echo
ls -lh $TARFILE $ZIPFILE
echo
