#!/usr/bin/env python
#
# Usage: mkconfigs.py CONFIGS.TXT
#
# Read a list of Brain Jam configurations, and output a representation
# of these configurations as a chunk of binary data. Each
# configuration requires 32 bytes, for a total of 32 * 1500 = 48000
# bytes of output.
#
# Configurations are input as a simple text representation, one
# configuration per line. Each line contains three items, separated by
# spaces: the configuration id, the order of cards in the deck, and
# the size of the shortest possible solution. The cards are
# represented by letters, as described below.
#
# Configurations are output as bitstreams, 256 bits (or 32 bytes) per
# configuration. Bits are read out from each byte going from most
# significant bit to least significant bit, and are stored in the same
# direction. The first 7 bits of a configuration's stream contain the
# size of the shortest possible solution for that configuration, minus
# 52 (since no solution can possibly take fewer than 52 moves). The
# remaining 249 bits in the stream indicate the order of the cards in
# the deck. The first 6 bits specify the top card, as a number between
# 0 and 51 inclusive. The next 6 bits specify the second card, but
# this time with a number between 0 and 50: the previous card is
# removed from the numbering, since it cannot appear twice. The next 6
# bits gives the third card as a number between 0 and 49, and so on.
# The 20th card requires a number between 0 and 31, and so therefore
# only 5 bits are used. The next 15 cards likewise are allocated 5
# bits each. Then 4 bits are used for each of the next 8 cards, 3 bits
# for each of the next 4 cards, and 2 bits for each of the next 2
# cards. The last bit in the stream identifies which of the two
# remaining cards comes next, with the bottom card of the deck being
# the remaining unchosen card.

import os
import sys

# Each card is encoded by a single letter.
cardnames = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'

# Translate a deck sequence from a string of 52 letters into a list of
# 51 pairs of numbers. In each pair, the first number is the index of
# the card after removal of the cards that precede it. (Thus for
# example the character C would translate to 2 if it appeared first in
# the string, or to 1 if either A or B appeared before C, or to 0 if
# both did.) The second number in the pair is the minimum number of
# bits required to represent the range of the first number. Note that
# the returned list only contains 51 elements because the last card
# will always be represented as (0, 0).
def deckseq(deck):
    avail = [1] * 52
    seq = []
    size = 6
    for n in range(0, 51):
        card = cardnames.index(deck[n])
        if (52 - n) & (51 - n) == 0:
            size -= 1
        c = sum(avail[0:card])
        avail[card] = 0
        seq.append((c, size))
    return seq

# Translate a list of numbers into a bitstream. Each element in the
# list supplies an integer to output and the number of bits to use for
# its representation. The bits are stored going from most to least
# significant bit. Thus if the first item in the list is (6, 3), the
# first byte of output would be set to 110x xxxx (with the xs being
# set by the following items). It is an error to request output with
# leftover bits in the last byte.
def outputbits(seq):
    out = bytearray()
    byte = 0
    bitpos = 128
    for (value, size) in seq:
        mask = 1 << size
        while mask > 1:
            mask >>= 1
            if value & mask:
                byte |= bitpos
            bitpos >>= 1
            if bitpos == 0:
                out.append(byte)
                byte = 0
                bitpos = 128
    if bitpos != 128:
        sys.stderr.write('bitpos is %d at end of outputbits.\n' % bitpos)
    return out

# Parse a configuration line and output the data as a bitstream.
def makeconfig(line, out):
    id, deck, best = line.split()
    seq = deckseq(deck)
    seq.insert(0, (int(best) - 52, 7))
    out.write(outputbits(seq))

# Read the input file and output the compact configuration data.
def translate(filename, out):
    f = open(filename)
    for line in f.readlines():
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        makeconfig(line, out)
    f.close()


out = os.fdopen(sys.stdout.fileno(), 'wb')
translate(sys.argv[1], out)
