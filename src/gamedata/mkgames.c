/* mkgames.c: Build Brain Jam's binary game data from a text source.
 *
 * This program accepts a text file describing the game's decks, and
 * outputs this information in a compressed binary format, which is
 * then built into the Brain Jam executable. Since it is a simple
 * one-shot utility, it shares no header files or other dependencies
 * with the actual program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A deck contains 52 cards.
 */
#define NCARDS 52

/* Each card is encoded as a single letter, with capital letters
 * representing the first 26 and lowercase letters the last 26.
 */
#define lettertoindex(c) ((c) < 'a' ? (c) - 'A' : (c) - 'a' + 26)

/* This global variable is used in outputting error messages.
 */
static char const *filename;

/* Output a value as a part of a bitstream. The bitlen argument
 * indicates the number of bits to use from value. The bits are output
 * going from most to least significant bit. Thus, if the first call
 * to this function has value set to 6 and bitlen set to 3, the first
 * byte of output would be set to 110x xxxx, with the xs representing
 * bits to be set by subsequent calls. A call with bitlen set to zero
 * indicates the end of the bitstream. (Any pending bits are flushed
 * at this time, but this is not expected to happen.) The return value
 * is false if the write to fp fails.
 */
static int outputbits(FILE *fp, int value, int bitlen)
{
    static int bitbuf = 0;
    static int bitpos = 0x80;
    int mask;

    if (!bitlen) {
        if (bitpos == 0x80)
            return 1;
        fprintf(stderr, "warning: unset bits at the end of the output\n");
        mask = bitbuf;
        bitbuf = 0;
        bitpos = 0x80;
        return fputc(mask, fp) != EOF;
    }

    for (mask = 1 << (bitlen - 1) ; mask ; mask >>= 1) {
        if (value & mask)
            bitbuf |= bitpos;
        bitpos >>= 1;
        if (!bitpos) {
            if (fputc(bitbuf, fp) == EOF)
                return 0;
            bitbuf = 0;
            bitpos = 0x80;
        }
    }
    return 1;
}

/* Translate a deck sequence from a string of 52 letters into a
 * sequence of 51 numbers of varying bit lengths. Each number gives
 * the index of the card after removal of the cards that precede it.
 * (Thus for example the character 'C' would translate to 2 if it
 * appeared first in the string, or to 1 if either 'A' or 'B' appeared
 * before 'C', or to 0 if both did.) The first numbers in the sequence
 * occupy six bits of output, but as the maximum possible value for
 * the number drops to 32, subsequent numbers are stored in five bits.
 * The pattern continues until the 51st number is output as one bit.
 * (The 52nd number is omitted since it is always zero.)
 */
static int outputdeckseq(FILE *outfile, char const *deck)
{
    char avail[NCARDS];
    int index, size;
    int c, i, n;

    size = 6;
    memset(avail, 1, sizeof avail);
    for (n = 0 ; n < NCARDS - 1 ; ++n) {
        index = lettertoindex(deck[n]);
        if (((NCARDS - n) & (NCARDS - n - 1)) == 0)
            --size;
        c = 0;
        for (i = 0 ; i < index ; ++i)
            c += avail[i];
        if (!outputbits(outfile, c, size))
            return 0;
        avail[index] = 0;
    }
    return 1;
}

/* Parse an input line and translate it to a sequence to output. Each
 * line of input consists of three items separated by spaces: a game
 * ID number, a sequence of cards encoded as 52 letters, and the size
 * of the best possible solution for that game. The solution size is
 * output to the bit stream as a seven-bit value (minus 52, since no
 * solution can be shorter than 52 moves), followed by the sequence of
 * cards in the deck. Each game configuration thus takes up 128 bits
 * (or 32 bytes) in the bit stream output.
 */
static int outputconfig(FILE *outfile, char const *line)
{
    int best, offset;

    if (sscanf(line, "%*u %n%*52[A-Za-z] %d", &offset, &best) < 1) {
        fprintf(stderr, "%s: syntax error!\n", filename);
        return 0;
    }
    return outputbits(outfile, best - NCARDS, 7) &&
           outputdeckseq(outfile, line + offset);
}

/* Given the names of an input and output file, translate the text
 * encoding of the full set of game configurations into the compressed
 * binary form. The input file is presumed to list one game
 * configuration on each line of input (save empty and commented
 * lines). False is returned if an error occurs.
 */
static int translate(char const *infilename, char const *outfilename)
{
    FILE *infile;
    FILE *outfile;
    char *line;
    size_t size;

    infile = fopen(infilename, "r");
    if (!infile) {
        perror(infilename);
        return 0;
    }
    outfile = fopen(outfilename, "wb");
    if (!outfile) {
        perror(outfilename);
        return 0;
    }

    filename = infilename;
    line = NULL;
    size = 0;
    while (getline(&line, &size, infile) >= 0) {
        if (*line == '\0' || *line == '\n' || *line == '#')
            continue;
        if (!outputconfig(outfile, line)) {
            perror(outfilename);
            return 0;
        }
    }
    free(line);
    if (!outputbits(outfile, 0, 0))
        return 0;
    if (fclose(outfile)) {
        perror(outfilename);
        return 0;
    }

    return 1;
}

/* main() validates the argument list and relays the final outcome.
 */
int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: mkcfgs INPUT.TXT OUTPUT.BIN\n");
        return 0;
    }
    return translate(argv[1], argv[2]) ? 0 : EXIT_FAILURE;
}
