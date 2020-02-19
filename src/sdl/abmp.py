#!/usr/bin/env python
#
# Usage: abmp.py IMAGE.PNG [OUT.BMP]
#
# Translate a PNG into a Windows bitmap with an alpha channel (32 bits
# per pixel).
#
# Support for an alpha channel was a late addition to the Windows
# bitmap file format. As a result, many programs that otherwise work
# with Windows bitmaps do not recognize bitmaps that include an alpha
# channel. It therefore makes sense to work with the images as PNG
# files, and only make the translation to Windows bitmaps at build
# time. Unfortunately, the ImageMagick tools also fail to support
# Windows bitmaps with alpha channels. So this script exists to do the
# translation of PNGs to bitmap files directly. Fortunately, the
# Window bitmap file format is very simple, consisting of a 54-byte
# header followed by the raw pixel values. Each pixel is four bytes
# long, in BGRA order, and read left-to-right, bottom-to-top.

from PIL import Image
import os
import sys

# Format a 32-bit value as a 4-byte little-endian string.
def int32(number):
    return bytearray([ number & 0xFF,
                       (number >> 8) & 0xFF,
                       (number >> 16) & 0xFF,
                       (number >> 24) & 0xFF ])

# Write out the header of a 32-bit bitmap with the given dimensions.
def writebmpheader(out, width, height):
    magic = bytearray([ 66, 77 ])
    headersize = 0x36
    bpp32flag = 0x00200001
    filesize = headersize + 4 * width * height
    out.write(bytearray(magic))
    out.write(int32(filesize))
    out.write(int32(0))
    out.write(int32(headersize))
    out.write(int32(0x28))
    out.write(int32(width))
    out.write(int32(height))
    out.write(int32(bpp32flag))
    out.write(int32(0) * 6)

# Write out the given image as a 32-bit bitmap.
def writebmp(out, image):
    width, height = image.size
    writebmpheader(out, width, height)
    pixels = image.load()
    lines = []
    for y in range(0, height):
        line = bytearray()
        for x in range(0, width):
            r, g, b, a = pixels[x, y]
            line.extend([ b, g, r, a ])
        lines.insert(0, line)
    for line in lines:
        out.write(line)

# Copy a png input file to a 32-bit bitmap output file. Output to
# stdout if no destination filename is given.
def pngtobmp(pngfilename, bmpfilename=None):
    try:
        image = Image.open(pngfilename)
    except IOError as e:
        sys.exit('%s: %s\n' % (pngfilename, e.strerror))
    try:
        if bmpfilename:
            outfile = open(bmpfilename, 'wb')
        else:
            outfile = os.fdopen(sys.stdout.fileno(), 'wb')
    except IOError as e:
        sys.exit('%s: %s\n' % (bmpfilename or 'stdout', e.strerror))
    writebmp(outfile, image)


if len(sys.argv) < 2 or len(sys.argv) > 3:
    sys.exit('Usage: abmp.py PNGFILENAME [BMPFILENAME]')
pngtobmp(*sys.argv[1:])
